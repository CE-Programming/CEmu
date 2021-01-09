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

#include "dockedwidget.h"

#include <QtGui/QTextCharFormat>
#include <QtWidgets/QWidget>
QT_BEGIN_NAMESPACE
class QCheckBox;
class QPlainTextEdit;
class QRadioButton;
QT_END_NAMESPACE

class ConsoleWidget : public DockedWidget
{
    Q_OBJECT

public:
    explicit ConsoleWidget(CoreWindow *coreWindow);

public slots:
    void append(const QString &str, const QColor &colorFg, const QColor &colorBg);
    void append(const char *str, int size);
    void setAutoScroll(int state);

private:
    QCheckBox *mChkAuto;
    QPlainTextEdit *mConsole;

    bool mAutoscroll;

    QTextCharFormat mFormat;
};

#endif
