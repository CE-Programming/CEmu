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

#ifndef MEMORYWIDGET
#define MEMORYWIDGET

class HexWidget;

#include <QtWidgets/QLineEdit>
QT_BEGIN_NAMESPACE
class QCheckBox;
class QLineEdit;
class QSpinBox;
QT_END_NAMESPACE

class MemoryWidgetList
{
public:
    MemoryWidgetList();
    ~MemoryWidgetList();

    MemoryWidgetList *prev() const { return mPrev; }
    MemoryWidgetList *next() const { return mNext; }
    bool empty() const { return mPrev == this; }

protected:
    MemoryWidgetList(MemoryWidgetList *list);
    MemoryWidgetList *mPrev, *mNext;
};

class MemoryWidget : public QWidget, public MemoryWidgetList
{
    Q_OBJECT

public:
    explicit MemoryWidget(QWidget *parent = nullptr);
    MemoryWidget(MemoryWidgetList *list, QWidget *parent = nullptr);

private:
    void init();
    HexWidget *mView;
};

#endif
