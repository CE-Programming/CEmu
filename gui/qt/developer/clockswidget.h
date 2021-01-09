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

#ifndef GPTWIDGET_H
#define GPTWIDGET_H

#include "../dockedwidget.h"
class HighlightEditWidget;

class ClocksWidget : public DockedWidget
{
    Q_OBJECT

public:
    explicit ClocksWidget(CoreWindow *coreWindow);

private:
    HighlightEditWidget *mEdtTimer1V;
    HighlightEditWidget *mEdtTimer2V;
    HighlightEditWidget *mEdtTimer3V;
    HighlightEditWidget *mEdtTimer1RV;
    HighlightEditWidget *mEdtTimer2RV;
    HighlightEditWidget *mEdtTimer3RV;
    HighlightEditWidget *mEdtDays;
    HighlightEditWidget *mEdtHrs;
    HighlightEditWidget *mEdtMin;
    HighlightEditWidget *mEdtSec;
};

#endif
