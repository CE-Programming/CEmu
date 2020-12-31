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

#ifndef FLASHRAMWIDGET_H
#define FLASHRAMWIDGET_H

#include "devwidget.h"
class MemoryWidget;

class FlashRamWidget : public DevWidget
{
    Q_OBJECT

public:
    explicit FlashRamWidget(DevWidget *parent = nullptr);

private:
    MemoryWidget *mFlash;
    MemoryWidget *mRam;
};

#endif