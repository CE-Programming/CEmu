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

#ifndef CPUWIDGET_H
#define CPUWIDGET_H

#include "devwidget.h"
class HighlightEditWidget;

QT_BEGIN_NAMESPACE
class QCheckBox;
class QLineEdit;
QT_END_NAMESPACE

class CpuRegisterFilter : public QObject
{
    Q_OBJECT

public:
    explicit CpuRegisterFilter(QObject *parent = nullptr)
        : QObject(parent) {}

    bool eventFilter(QObject *obj, QEvent *event) override;
};

class CpuWidget : public DevWidget
{
    Q_OBJECT

public:
    explicit CpuWidget(DevWidget *parent = nullptr);

protected:
    virtual void saveState();
    virtual void loadState();

private:
    CpuRegisterFilter *mRegEventFilter;

    QCheckBox *mChkS;
    QCheckBox *mChkZ;
    QCheckBox *mChk5;
    QCheckBox *mChkH;
    QCheckBox *mChk3;
    QCheckBox *mChkPV;
    QCheckBox *mChkN;
    QCheckBox *mChkC;

    QCheckBox *mChkHalt;
    QCheckBox *mChkAdl;
    QCheckBox *mChkMadl;
    QCheckBox *mChkIef2;
    QCheckBox *mChkIef1;

    HighlightEditWidget *mEdtAF;
    HighlightEditWidget *mEdtBC;
    HighlightEditWidget *mEdtDE;
    HighlightEditWidget *mEdtHL;
    HighlightEditWidget *mEdtIX;
    HighlightEditWidget *mEdtAFX;
    HighlightEditWidget *mEdtBCX;
    HighlightEditWidget *mEdtDEX;
    HighlightEditWidget *mEdtHLX;
    HighlightEditWidget *mEdtIY;
    HighlightEditWidget *mEdtPC;
    HighlightEditWidget *mEdtMB;
    HighlightEditWidget *mEdtSPL;
    HighlightEditWidget *mEdtSPS;
    HighlightEditWidget *mEdtI;
    HighlightEditWidget *mEdtR;
    HighlightEditWidget *mEdtIM;
};

#endif
