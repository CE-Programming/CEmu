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

#include "highlighteditwidget.h"

#include "../../util.h"

#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QSpinBox>

HighlightEditWidget::HighlightEditWidget(const QString &mask, QWidget *parent)
    : QLineEdit{parent}
{
    QColor highlightColor({200, 235, 255});

    setInputMask(mask);
    setFont(Util::monospaceFont());

    mPaletteNoHighlight = palette();
    mPaletteHighlight = palette();
    mPaletteHighlight.setColor(QPalette::Base, highlightColor);
}

void HighlightEditWidget::setString(const QString &str)
{
    if (text() != str)
    {
        setPalette(mPaletteHighlight);
    }
    else
    {
        setPalette(mPaletteNoHighlight);
    }
    setText(str);
}

void HighlightEditWidget::setInt(uint32_t value, uint8_t length)
{
    setString(Util::int2hex(value, length));
}

uint32_t HighlightEditWidget::getInt()
{
    return Util::hex2int(text());
}

void HighlightEditWidget::clearHighlight()
{
    setPalette(mPaletteNoHighlight);
}
