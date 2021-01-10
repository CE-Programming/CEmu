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

#ifndef DISASSEMBLYWIDGET_H
#define DISASSEMBLYWIDGET_H

#include "../dockedwidget.h"
class DisassemblerWidget;

QT_BEGIN_NAMESPACE
class QLineEdit;
class QCheckBox;
QT_END_NAMESPACE

class DisassemblyWidget : public DockedWidget
{
    Q_OBJECT

public:
    explicit DisassemblyWidget(CoreWindow *coreWindow);

    void enableDebugWidgets(bool) override;

    bool gotoAddress(const QString &addr);

private:
    DisassemblerWidget *mDisasm;
    QLineEdit *mEdtAddr;
    QCheckBox *mChkAdl;
};

#endif
