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

#include "screenwidget.h"

#include <QPainter>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QPushButton>

ScreenWidget::ScreenWidget(QWidget *parent)
    : QWidget(parent)
{
}

ScreenWidget::~ScreenWidget()
{
}

void ScreenWidget::setSkin(const QString &skin)
{
    mSkin = QImage(skin);
}

void ScreenWidget::paintEvent(QPaintEvent */*event*/)
{
    QPainter painter{this};
    const static QRect screen{61, 78, 320, 240};

    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.setTransform(mTransform);

    painter.drawImage(mSkin.rect(), mSkin, mSkin.rect());

    painter.fillRect(screen, Qt::black);
    painter.setPen(Qt::white);
    painter.setFont(QFont("arial", 18));
    painter.drawText(screen, Qt::AlignCenter, tr("LCD OFF"));
}

void ScreenWidget::resizeEvent(QResizeEvent *event)
{
    QSize size{mSkin.size().scaled(event->size(), Qt::KeepAspectRatio)},
        origin{(event->size() - size) / 2};

    qreal m11 = static_cast<qreal>(size.width()) / mSkin.width();
    qreal m22 = static_cast<qreal>(size.height()) / mSkin.height();
    qreal dx = origin.width();
    qreal dy = origin.height() * 2;

    mTransform.setMatrix(m11,  0, 0,
                         0,  m22, 0,
                         dx,  dy, 1);
}
