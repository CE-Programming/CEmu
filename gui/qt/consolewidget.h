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

#ifndef CONSOLEWIDGET_H
#define CONSOLEWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QSpacerItem>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include <QRadioButton>

class ConsoleWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ConsoleWidget(QWidget *parent = nullptr);
    ~ConsoleWidget();

public:
    void setNativeConsole(bool dock);

public slots:
    void clear();
    void append(const QString &str, const QColor &colorFg, const QColor &colorBg);
    void append(const char *str, int size);
    void setAutoScroll(int state);

private:
    QRadioButton *mRadDock;
    QRadioButton *mRadNative;
    QCheckBox *mChkAuto;
    QPlainTextEdit *mConsole;

    bool mNativeConsole;
    bool mAutoscroll;

    QTextCharFormat mFormat;
};

#endif
