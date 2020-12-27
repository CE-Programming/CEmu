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

#include <QWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QRadioButton>
#include <QCheckBox>

#include <cstdio>

ConsoleWidget::ConsoleWidget(QWidget *parent)
    : QWidget{parent},
      mNativeConsole{false}
{
    QHBoxLayout *hlayout = new QHBoxLayout();

    QPushButton *btnClear = new QPushButton(tr("Clear"), this);
    QSpacerItem *spacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Preferred);

    mChkAuto = new QCheckBox(tr("Autoscroll"), this);
    mRadDock = new QRadioButton(tr("Dock"), this);
    mRadNative = new QRadioButton(tr("Native"), this);
    mConsole = new QPlainTextEdit(this);

    mConsole->setReadOnly(true);
    mConsole->setMaximumBlockCount(2500);
    mConsole->setMinimumSize(10, 100);

    hlayout->addWidget(mRadDock);
    hlayout->addWidget(mRadNative);
    hlayout->addSpacerItem(spacer);
    hlayout->addWidget(mChkAuto);
    hlayout->addWidget(btnClear);

    QVBoxLayout *vlayout = new QVBoxLayout(this);
    vlayout->addWidget(mConsole);
    vlayout->addLayout(hlayout);
    setLayout(vlayout);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    connect(btnClear, &QPushButton::clicked, this, &ConsoleWidget::clear);
    connect(mChkAuto, &QCheckBox::stateChanged, this, &ConsoleWidget::setAutoScroll);
}

ConsoleWidget::~ConsoleWidget()
{
}


void ConsoleWidget::clear()
{
    if (mNativeConsole)
    {
        int ret;
#ifdef _WIN32
        ret = system("cls");
#else
        ret = system("clear");
#endif
        (void)ret;
    }
    else
    {
        mConsole->clear();
    }
}

void ConsoleWidget::append(const QString &str, const QColor &colorFg, const QColor &colorBg)
{
    if (mNativeConsole)
    {
        fputs(str.toStdString().c_str(), stdout);
    }
    else
    {
        mFormat.setBackground(colorBg);
        mFormat.setForeground(colorFg);
        QTextCursor cur(mConsole->document());
        cur.movePosition(QTextCursor::End);
        cur.insertText(str, mFormat);
        if (mAutoscroll)
        {
            mConsole->setTextCursor(cur);
        }
    }
}

void ConsoleWidget::append(const char *str, int size)
{
    enum {
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
    if (mNativeConsole)
    {
        fwrite(str, sizeof(char), static_cast<size_t>(size), stdout);
    }
    else
    {
        static int state = CONSOLE_ESC;
        static QColor sColorFg = mConsole->palette().color(QPalette::Text);
        static QColor sColorBg = mConsole->palette().color(QPalette::Base);
        const char *tok;
        static const QColor colors[8] =
        {
            Qt::black, Qt::red, Qt::green, Qt::yellow, Qt::blue, Qt::magenta, Qt::cyan, Qt::white
        };

        QColor colorFg = sColorFg;
        QColor colorBg = sColorBg;
        if ((tok = static_cast<const char*>(memchr(str, '\x1B', static_cast<size_t>(size)))))
        {
            if (tok != str)
            {
                append(QString::fromUtf8(str, static_cast<int>(tok - str)), sColorFg, sColorBg);
                size -= tok - str;
            }
            do {
                while(--size > 0)
                {
                    char x = *tok++;
                    switch (state)
                    {
                        case CONSOLE_ESC:
                            if (x == '\x1B')
                            {
                                state = CONSOLE_BRACKET;
                            }
                            break;
                        case CONSOLE_BRACKET:
                            if (x == '[')
                            {
                                state = CONSOLE_PARSE;
                            }
                            else
                            {
                                state = CONSOLE_ESC;
                            }
                            break;
                        case CONSOLE_PARSE:
                            switch (x)
                            {
                                case '0':
                                    state = CONSOLE_ENDVAL;
                                    colorBg = mConsole->palette().color(QPalette::Base);
                                    colorFg = mConsole->palette().color(QPalette::Text);
                                    break;
                                case '3':
                                    state = CONSOLE_FGCOLOR;
                                    break;
                                case '4':
                                    state = CONSOLE_BGCOLOR;
                                    break;
                                default:
                                    state = CONSOLE_ESC;
                                    sColorBg = mConsole->palette().color(QPalette::Base);
                                    sColorFg = mConsole->palette().color(QPalette::Text);
                                    break;
                            }
                            break;
                        case CONSOLE_FGCOLOR:
                            if (x >= '0' && x <= '7')
                            {
                                state = CONSOLE_ENDVAL;
                                colorFg = colors[x - '0'];
                            }
                            else
                            {
                                state = CONSOLE_ESC;
                            }
                            break;
                        case CONSOLE_BGCOLOR:
                            if (x >= '0' && x <= '7')
                            {
                                state = CONSOLE_ENDVAL;
                                colorBg = colors[x - '0'];
                            }
                            else
                            {
                                state = CONSOLE_ESC;
                            }
                            break;
                        case CONSOLE_ENDVAL:
                            if (x == ';')
                            {
                                state = CONSOLE_PARSE;
                            }
                            else if (x == 'm')
                            {
                                sColorBg = colorBg;
                                sColorFg = colorFg;
                                state = CONSOLE_ESC;
                            }
                            else
                            {
                                state = CONSOLE_ESC;
                            }
                            break;
                        default:
                            state = CONSOLE_ESC;
                            break;
                    }
                    if (state == CONSOLE_ESC)
                    {
                        break;
                    }
                }
                if (size > 0)
                {
                    const char *tokn = static_cast<const char*>(memchr(tok, '\x1B', static_cast<size_t>(size)));
                    if (tokn)
                    {
                        append(QString::fromUtf8(tok, static_cast<int>(tokn - tok)), sColorFg, sColorBg);
                        size -= tokn - tok;
                    }
                    else
                    {
                        append(QString::fromUtf8(tok, static_cast<int>(size)), sColorFg, sColorBg);
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
            append(QString::fromUtf8(str, size), sColorFg, sColorBg);
        }
    }
}

void ConsoleWidget::setAutoScroll(int state)
{
    mAutoscroll = state == Qt::CheckState::Checked;

    mChkAuto->setCheckState(mAutoscroll ?
                            Qt::CheckState::Checked :
                            Qt::CheckState::Unchecked);
}

void ConsoleWidget::setNativeConsole(bool nativeConsole)
{
    mNativeConsole = nativeConsole;
}
