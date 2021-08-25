/*
 * Copyright (c) 2015-2020 CE Programming.
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

#include "consolewidget.h"

#include "settings.h"

#include <kddockwidgets/DockWidget.h>

#include <QtCore/QFile>
#include <QtCore/QStringLiteral>
#include <QtCore/QtGlobal>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>

#include <cstdio>

void InputThread::run()
{
    QFile inputFile;
    if (!inputFile.open(stdin, QIODevice::ReadOnly | QIODevice::Text))
    {
        return;
    }
    while (true)
    {
        emit inputLine(QString::fromUtf8(inputFile.readLine()));
    }
}

ConsoleWidget::ConsoleWidget(CoreWindow *coreWindow)
    : DockedWidget{new KDDockWidgets::DockWidget{QStringLiteral("Console")},
                   QIcon(QStringLiteral(":/assets/icons/command_line.svg")),
                   coreWindow}
{
    mConsole = new QPlainTextEdit;
    QPushButton *btnClear = new QPushButton(QIcon(QStringLiteral(":/assets/icons/empty_trash.svg")), tr("Clear"), this);
    QLineEdit *inputLine = new QLineEdit;
    mChkAuto = new QCheckBox(tr("Autoscroll"));

    mConsole->setReadOnly(true);
    mConsole->setMaximumBlockCount(2500);
    mConsole->setMinimumSize(10, 100);
    mFormat.setBackground(mConsole->palette().color(QPalette::Base));
    mFormat.setForeground(mConsole->palette().color(QPalette::Text));

    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->addWidget(inputLine);
    hLayout->addWidget(mChkAuto);
    hLayout->addWidget(btnClear);

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addWidget(mConsole);
    vLayout->addLayout(hLayout);
    setLayout(vLayout);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    if (Settings::boolOption(Settings::ConsoleAutoScroll))
    {
        setAutoScroll(Qt::Checked);
    }
    else
    {
        setAutoScroll(Qt::Unchecked);
    }

    InputThread *inputThread = new InputThread{this};
    connect(inputThread, &InputThread::inputLine, this, &ConsoleWidget::processInputLine);
    inputThread->start();

    connect(inputLine, &QLineEdit::returnPressed, [this, inputLine]()
    {
        processInputLine(inputLine->text());
        inputLine->clear();
    });
    connect(btnClear, &QPushButton::clicked, mConsole, &QPlainTextEdit::clear);
    connect(mChkAuto, &QCheckBox::stateChanged, this, &ConsoleWidget::setAutoScroll);
}

void ConsoleWidget::append(const QString &str, const QTextCharFormat &format)
{
    QTextCursor cur(mConsole->document());
    cur.movePosition(QTextCursor::End);
    cur.insertText(str, format);
    if (mAutoscroll)
    {
        mConsole->setTextCursor(cur);
    }
}

void ConsoleWidget::append(const char *str, int size)
{
    enum
    {
        CONSOLE_ESC,
        CONSOLE_BRACKET,
        CONSOLE_PARSE,
        CONSOLE_BGCOLOR,
        CONSOLE_FGCOLOR,
        CONSOLE_EQUALS,
        CONSOLE_ENDVAL
    };

    if (size == -1)
    {
        size = static_cast<int>(strlen(str));
    }

    const char *tok;
    static const QColor colors[8] =
    {
        Qt::black, Qt::red, Qt::green, Qt::yellow, Qt::blue, Qt::magenta, Qt::cyan, Qt::white
    };

    if ((tok = static_cast<const char*>(memchr(str, '\x1B', size_t(size)))))
    {
        if (tok != str)
        {
            append(QString::fromUtf8(str, int(tok - str)), mFormat);
            size -= tok - str;
        }
        do {
            while(--size > 0)
            {
                char x = *tok++;
                switch (mAnsiEscapeState)
                {
                    case CONSOLE_ESC:
                        if (x == '\x1B')
                        {
                            mAnsiEscapeState = CONSOLE_BRACKET;
                        }
                        break;
                    case CONSOLE_BRACKET:
                        if (x == '[')
                        {
                            mAnsiEscapeState = CONSOLE_PARSE;
                        }
                        else
                        {
                            mAnsiEscapeState = CONSOLE_ESC;
                        }
                        break;
                    case CONSOLE_PARSE:
                        switch (x)
                        {
                            case '0':
                                mAnsiEscapeState = CONSOLE_ENDVAL;
                                mFormat.setBackground(mConsole->palette().color(QPalette::Base));
                                mFormat.setForeground(mConsole->palette().color(QPalette::Text));
                                break;
                            case '3':
                                mAnsiEscapeState = CONSOLE_FGCOLOR;
                                break;
                            case '4':
                                mAnsiEscapeState = CONSOLE_BGCOLOR;
                                break;
                            default:
                                mAnsiEscapeState = CONSOLE_ESC;
                                mFormat.setBackground(mConsole->palette().color(QPalette::Base));
                                mFormat.setForeground(mConsole->palette().color(QPalette::Text));
                                break;
                        }
                        break;
                    case CONSOLE_FGCOLOR:
                        if (x >= '0' && x <= '7')
                        {
                            mAnsiEscapeState = CONSOLE_ENDVAL;
                            mFormat.setForeground(colors[x - '0']);
                        }
                        else
                        {
                            mAnsiEscapeState = CONSOLE_ESC;
                        }
                        break;
                    case CONSOLE_BGCOLOR:
                        if (x >= '0' && x <= '7')
                        {
                            mAnsiEscapeState = CONSOLE_ENDVAL;
                            mFormat.setBackground(colors[x - '0']);
                        }
                        else
                        {
                            mAnsiEscapeState = CONSOLE_ESC;
                        }
                        break;
                    case CONSOLE_ENDVAL:
                        if (x == ';')
                        {
                            mAnsiEscapeState = CONSOLE_PARSE;
                        }
                        else if (x == 'm')
                        {
                            mAnsiEscapeState = CONSOLE_ESC;
                        }
                        else
                        {
                            mAnsiEscapeState = CONSOLE_ESC;
                        }
                        break;
                    default:
                        mAnsiEscapeState = CONSOLE_ESC;
                        break;
                }
                if (mAnsiEscapeState == CONSOLE_ESC)
                {
                    break;
                }
            }
            if (size > 0)
            {
                const char *tokn = static_cast<const char*>(memchr(tok, '\x1B', size_t(size)));
                if (tokn)
                {
                    append(QString::fromUtf8(tok, int(tokn - tok)), mFormat);
                    size -= tokn - tok;
                }
                else
                {
                    append(QString::fromUtf8(tok, int(size)), mFormat);
                }
                tok = tokn;
            }
            else
            {
                tok = nullptr;
            }
        } while (tok);
    }
    else
    {
        append(QString::fromUtf8(str, size), mFormat);
    }
}

void ConsoleWidget::setAutoScroll(int state)
{
    mAutoscroll = state == Qt::CheckState::Checked;

    mChkAuto->setCheckState(mAutoscroll ?
                            Qt::CheckState::Checked :
                            Qt::CheckState::Unchecked);

    Settings::setBoolOption(Settings::ConsoleAutoScroll, mAutoscroll);
}

void ConsoleWidget::processInputLine(QString line)
{
    if (line.isEmpty())
    {
        line = mLastInputLine;
    }
    else
    {
        mLastInputLine = line;
    }
    if (!line.isEmpty())
    {
        QTextCharFormat format;
        format.setBackground(mConsole->palette().color(QPalette::Base));
        format.setForeground(Qt::darkGray);
        append(QStringLiteral("> %1\n").arg(line.trimmed()), format);
        emit inputLine(line);
    }
}
