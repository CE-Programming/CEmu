/*
 * Copyright (c) 2015-2021 CE Programming.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "console.h"

#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QString>
#include <QtCore/QThread>
#include <QtNetwork/QLocalSocket>
#ifdef Q_OS_WIN
# include <QtCore/QWinEventNotifier>
# define WIN32_LEAN_AND_MEAN
# include <io.h>
# include <windows.h>
# define Notifier QWinEventNotifier
# define getStandard(file) ::GetStdHandle(STD_##file##_HANDLE)
#else
# include <QtCore/QSocketNotifier>
# define Notifier QSocketNotifier
# define STD_INPUT_FILENO  0
# define STD_INPUT_PERM    Notifier::Read
# define STD_OUTPUT_FILENO 1
# define STD_OUTPUT_PERM   Notifier::Write
# define STD_ERROR_FILENO  2
# define STD_ERROR_PERM    Notifier::Write
# define getStandard(file) STD_##file##_FILENO, STD_##file##_PERM
#endif

#include <array>
namespace readline
{
# include <readline/history.h>
# include <readline/readline.h>
extern "C" int _rl_meta_flag, _rl_convert_meta_chars_to_ascii, _rl_output_meta_chars, _rl_utf8locale;
}

#undef ESC
#define ESC "\33"
#define CSI ESC "["

readline::Worker::Worker(QObject *parent)
    : QObject{parent}
    , mUtf8Codec{QTextCodec::codecForName("UTF-8")}
{
    mUtf8State.flags.setFlag(QTextCodec::IgnoreHeader);
}

readline::Worker::~Worker()
{
    if (sInstance != this)
    {
        return;
    }
#ifdef HAS_READLINE
    readline::rl_callback_handler_remove();
#endif
    sInstance = nullptr;
}

void readline::Worker::start(const QString &name)
{
    QLocalSocket *socket = new QLocalSocket{this};
    connect(socket, &QLocalSocket::connected, [this, socket]
    {
#ifdef Q_OS_WIN
        connected(::_wfdopen(::_open_osfhandle(socket->socketDescriptor(), 0), L"wb"));
#else
        connected(::fdopen(socket->socketDescriptor(), "wb"));
#endif
    });
    socket->connectToServer(name);
}

void readline::Worker::addHistoryLine(const QString &name)
{
#ifdef HAS_READLINE
    bool atEnd = !readline::current_history();
    readline::add_history(name.toUtf8().constData());
    if (atEnd)
    {
        readline::next_history();
    }
#endif
}

void readline::Worker::inputReady()
{
#ifdef HAS_READLINE
# ifdef Q_OS_WIN
    std::array<::INPUT_RECORD, 256> events;
    ::DWORD count;
    if (!::ReadConsoleInputW(getStandard(INPUT), events.data(), events.size(), &count))
    {
        return;
    }
    bool pending = false;
    const auto output = [&pending](const char *s)
    {
        while (*s)
        {
            pending = true;
            while (!readline::rl_stuff_char(quint8(*s)))
            {
                readline::rl_callback_read_char();
            }
            s += 1;
        }
    };
    QByteArray utf8;
    for (::DWORD event = 0; event != count; event += 1)
    {
        if (events[event].EventType != KEY_EVENT ||
            !events[event].Event.KeyEvent.bKeyDown)
        {
            continue;
        }
        switch (events[event].Event.KeyEvent.wVirtualKeyCode)
        {
#define CASE(vk, seq) case VK_##vk: utf8 = seq; break
            CASE(RETURN,     "\n" );
            CASE(UP,     CSI "A"  );
            CASE(DOWN,   CSI "B"  );
            CASE(RIGHT,  CSI "C"  );
            CASE(LEFT,   CSI "D"  );
            CASE(INSERT, CSI "2~" );
            CASE(DELETE, CSI "3~" );
            CASE(PRIOR,  CSI "5~" );
            CASE(NEXT,   CSI "6~" );
            CASE(HOME,   CSI "7~" );
            CASE(END,    CSI "8~" );
            CASE(F1,     CSI "11~");
            CASE(F2,     CSI "12~");
            CASE(F3,     CSI "13~");
            CASE(F4,     CSI "14~");
            CASE(F5,     CSI "15~");
            CASE(F6,     CSI "17~");
            CASE(F7,     CSI "18~");
            CASE(F8,     CSI "19~");
            CASE(F9,     CSI "20~");
            CASE(F10,    CSI "21~");
            CASE(F11,    CSI "23~");
            CASE(F12,    CSI "24~");
            CASE(F13,    CSI "25~");
            CASE(F14,    CSI "26~");
            CASE(F15,    CSI "28~");
            CASE(F16,    CSI "29~");
            CASE(F17,    CSI "31~");
            CASE(F18,    CSI "32~");
            CASE(F19,    CSI "33~");
            CASE(F20,    CSI "34~");
#undef CASE
            default:
                QChar c{events[event].Event.KeyEvent.uChar.UnicodeChar};
                if (c.isNull())
                {
                    continue;
                }
                utf8 = mUtf8Codec->fromUnicode(&c, 1, &mUtf8State);
                break;
        }
        for (::WORD repeat = 0; repeat != events[event].Event.KeyEvent.wRepeatCount; repeat += 1)
        {
            if (events[event].Event.KeyEvent.dwControlKeyState & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED) &&
                qint8(utf8.constData()[0]) > 0)
            {
                output(ESC);
            }
            output(utf8.constData());
        }
    }
    if (!pending)
    {
        return;
    }
# endif
    readline::rl_callback_read_char();
#endif
}

void readline::Worker::connected(::FILE *socket)
{
    if (sInstance || !socket)
    {
        return;
    }
    sInstance = this;
#ifdef HAS_READLINE
    readline::rl_outstream = socket;
# ifdef Q_OS_WIN
    readline::rl_prep_term_function = [](int metaFlag)
    {
        readline::_rl_meta_flag = metaFlag;
        readline::_rl_convert_meta_chars_to_ascii = false;
        readline::_rl_output_meta_chars = true;
        readline::_rl_utf8locale = true;
        readline::rl_prep_terminal(metaFlag);
    };
# endif
    readline::rl_callback_handler_install(nullptr, [](char *line)
    {
        if (line)
        {
            emit sInstance->inputLine(QString::fromUtf8(line));
        }
        std::free(line);
    });
#endif
    Notifier *notifier = new Notifier{getStandard(INPUT), this};
    connect(notifier, &Notifier::activated, this, &Worker::inputReady);
    notifier->setEnabled(true);
}

readline::Worker *readline::Worker::sInstance;

readline::Controller::Controller(QObject *parent)
    : QObject{parent}
{
    connect(this, &Controller::start, &mWorker, &Worker::start);
    connect(&mWorker, &Worker::inputLine, this, &Controller::inputLine);
    connect(this, &Controller::addHistoryLine, &mWorker, &Worker::addHistoryLine);
}

readline::Controller::~Controller()
{
    if (mThread)
    {
        mThread->quit();
        mThread->wait();
    }
}

void readline::Controller::connectToServer(const QLocalServer &server)
{
    if (mThread)
    {
        return;
    }
    mThread = new QThread{this};
    mWorker.moveToThread(mThread);
    mThread->start();
    emit start(server.fullServerName());
}

Connection::Connection(QLocalSocket *socket, QObject *parent)
    : QObject{parent}
    , mStream{socket}
{
    mStream.setCodec("UTF-8");
    connect(socket, &QLocalSocket::readyRead, this, &Connection::readyRead);
}

void Connection::readyRead()
{
    QString text = mStream.readAll();
#ifdef Q_OS_WIN
    ::WriteConsole(getStandard(OUTPUT), reinterpret_cast<const wchar_t *>(text.utf16()), text.length(), nullptr, nullptr);
#else
    QByteArray utf8 = text.toUtf8();
    std::fwrite(utf8.constData(), utf8.length(), 1, stdout);
    std::fflush(stdout);
#endif
}

Console::Console(QObject *parent)
    : QObject{parent}
    , mReadline{this}
    , mServer{this}
{
    qRegisterMetaType<QAbstractSocket::SocketState>(); // qt hack
    connect(&mReadline, &readline::Controller::inputLine, this, &Console::inputLine);
    connect(this, &Console::addHistoryLine, &mReadline, &readline::Controller::addHistoryLine);
    connect(&mServer, &QLocalServer::newConnection, this, &Console::connection);
    {
        QCoreApplication *app = QCoreApplication::instance();
        QString name = QStringLiteral("%1.%2").arg(app->applicationName()).arg(app->applicationPid());
        if (!QLocalServer::removeServer(name) ||
            !mServer.listen(name))
        {
            return;
        }
    }
    mReadline.connectToServer(mServer);
}

Console::~Console()
{
}

void Console::outputText(const QString &text)
{
    static_cast<void>(text);
}

void Console::errorText(const QString &text)
{
    static_cast<void>(text);
}

void Console::connection()
{
    if (QLocalSocket *socket = mServer.nextPendingConnection())
    {
        new Connection{socket, this};
    }
}
