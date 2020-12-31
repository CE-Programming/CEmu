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

#ifndef MEMWIDGET_H
#define MEMWIDGET_H

class HexWidget;

namespace KDDockWidgets
{
class DockWidget;
}

#include <QtWidgets/QWidget>

class MemWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MemWidget(QWidget *parent = nullptr);

private:
    HexWidget *mView;
};

#endif
