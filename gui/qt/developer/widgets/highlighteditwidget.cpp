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

#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QSpinBox>

HighlightEditWidget::HighlightEditWidget(const QString &mask, QWidget *parent)
    : QLineEdit{parent}
{
#ifdef Q_OS_WIN
    QFont monospaceFont(QStringLiteral("Courier"), 10);
#else
    QFont monospaceFont(QStringLiteral("Monospace"), 10);
#endif

    QColor highlightColor({200, 235, 255});

    setInputMask(mask);
    setFont(monospaceFont);

    mPaletteNoHighlight = palette();
    mPaletteHighlight = palette();
    mPaletteHighlight.setColor(QPalette::Base, highlightColor);
}

void HighlightEditWidget::setText(const QString &newText)
{
    if (text() != newText)
    {
        setPalette(mPaletteHighlight);
    }
    else
    {
        setPalette(mPaletteNoHighlight);
    }
}

void HighlightEditWidget::clearHighlight()
{
    setPalette(mPaletteNoHighlight);
}
