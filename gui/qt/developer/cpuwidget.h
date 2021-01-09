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

#include "../dockedwidget.h"
class HighlightEditWidget;

QT_BEGIN_NAMESPACE
class QCheckBox;
class QLineEdit;
class QPlainTextEdit;
class QSpinBox;
QT_END_NAMESPACE

class CpuRegisterFilter : public QObject
{
    Q_OBJECT

public:
    explicit CpuRegisterFilter(QObject *parent = nullptr)
        : QObject(parent) {}

    bool eventFilter(QObject *obj, QEvent *event) override;
};

class CpuWidget : public DockedWidget
{
    Q_OBJECT

public:
    explicit CpuWidget(DockedWidgetList &list);

    void loadFromCore(const CoreWrapper &) override;
    void storeToCore(CoreWrapper &) const override;

private:
    void setFlags();

    CpuRegisterFilter *mRegEventFilter;

    QCheckBox *mChkS;
    QCheckBox *mChkZ;
    QCheckBox *mChk5;
    QCheckBox *mChkH;
    QCheckBox *mChk3;
    QCheckBox *mChkP;
    QCheckBox *mChkN;
    QCheckBox *mChkC;

    QCheckBox *mChkHalt;
    QCheckBox *mChkAdl;
    QCheckBox *mChkMadl;
    QCheckBox *mChkIef2;
    QCheckBox *mChkIef1;

    QCheckBox *mChkAdlStack;

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
    QSpinBox *mSpnIM;

    QPlainTextEdit *mEdtStack;

};

#endif
