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

#include "sliderpanner.h"

#include <QtCore/QDebug>
#include <QtCore/QtGlobal>
#include <QtWidgets/QAbstractSlider>
#include <QtWidgets/QProxyStyle>

SliderPanner::SliderPanner(QAbstractSlider *slider, int interval)
    : QTimer{slider}
{
    setInterval(interval);

    static class : public QProxyStyle
    {
        int styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget,
                      QStyleHintReturn *returnData) const override
        {
            if (hint == QStyle::SH_ScrollBar_LeftClickAbsolutePosition ||
                hint == QStyle::SH_ScrollBar_MiddleClickAbsolutePosition)
            {
                return true;
            }
            if (hint == QStyle::SH_ScrollBar_ContextMenu)
            {
                return false;
            }
            return QProxyStyle::styleHint(hint, option, widget, returnData);
        }
    } style;
    slider->setStyle(&style);
    slider->setTracking(false);

    connect(slider, &QAbstractSlider::sliderPressed, this, QOverload<>::of(&QTimer::start));
    connect(slider, &QAbstractSlider::sliderReleased, this, &QTimer::stop);
    connect(this, &QTimer::timeout, [this]()
    {
        if (int amount = parent()->sliderPosition())
        {
            emit panBy(amount);
        }
    });
    connect(slider, &QAbstractSlider::valueChanged, [this](int amount)
    {
        if (amount)
        {
            emit panBy(amount);
        }
        parent()->setValue(0);
    });
}

QAbstractSlider *SliderPanner::parent() const
{
    return static_cast<QAbstractSlider *>(QTimer::parent());
}
