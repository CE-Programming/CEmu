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

#ifndef DEVWIDGET_H
#define DEVWIDGET_H

#include <QtWidgets/QWidget>

class DevWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DevWidget(QWidget *parent = nullptr)
        : QWidget{parent} { enable(); }

protected:
    virtual void storeState() {}
    virtual void loadState() {}
    virtual void disable() { setEnabled(false); }
    virtual void enable() { setEnabled(true); }
};

#endif
