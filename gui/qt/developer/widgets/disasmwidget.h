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

#ifndef DISASMWIDGET_H
#define DISASMWIDGET_H

#include <QtWidgets/QWidget>

class DisasmWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DisasmWidget(QWidget *parent = nullptr);

    QSize sizeHint() const override;

protected:
    virtual void wheelEvent(QWheelEvent *) override;
    virtual void paintEvent(QPaintEvent *) override;

private:
    typedef enum { None, Label, Inst } addr_type_t;
    typedef struct { uint32_t addr : 24; addr_type_t type : 8; } addr_t;
    addr_t next(addr_t addr);

    addr_t m_baseAddr = { 0, None };
    int m_scroll = 0;
    QHash<uint32_t, QString> m_labels;
};

#endif
