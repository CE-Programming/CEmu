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

#include <QtCore/QDebug>
#include <QtGui/QPainter>
#include <QtWidgets/QSizePolicy>

const QSize ScreenWidget::sOuterSize{450, 360}, ScreenWidget::sOuterCorner{60, 60},
    ScreenWidget::sInnerCorner{30, 30};
const QRect ScreenWidget::sInnerRect{32, 60, sOuterSize.width() - 32 - 32,
                                             sOuterSize.height() - 60 - 16};

ScreenWidget::ScreenWidget(QWidget *parent)
    : QWidget(parent),
      mGradient{0, 0, 450, 0},
      mUnpoweredText{tr("LCD OFF")},
      mProductFont{QStringLiteral("Arial"), 18, QFont::Black},
      mModelFont{QStringLiteral("Arial"), 18, QFont::Thin},
      mUnpoweredFont{QStringLiteral("Arial"), 18},
      mOn{false}
{
    mOuterPath.moveTo(0, sOuterSize.height());
    mOuterPath.lineTo(0, sOuterCorner.height());
    mOuterPath.arcTo({{}, sOuterCorner * 2}, 180, -90);
    mOuterPath.lineTo(sOuterSize.width() - sOuterCorner.width(), 0);
    mOuterPath.arcTo({{sOuterSize.width() - 2.*sOuterCorner.width(), 0}, sOuterCorner * 2}, 90, -90);
    mOuterPath.lineTo(sOuterSize.width(), sOuterSize.height());

    mInnerPath.addRoundedRect(sInnerRect, sInnerCorner.height(), sInnerCorner.width());

    mGradient.setColorAt(0, 0x303030);
    mGradient.setColorAt(100./sOuterSize.width(), 0x1C1C1C);
    mGradient.setColorAt((sOuterSize.width() - 100.)/sOuterSize.width(), 0x1C1C1C);
    mGradient.setColorAt(1, 0x303030);

    prepareText();
}

ScreenWidget::~ScreenWidget()
{
}

void ScreenWidget::setModel(const QString &product, const QString &model)
{
    mProductText.setText(product);
    mModelText.setText(model);
    prepareText();
}

void ScreenWidget::paintEvent(QPaintEvent */*event*/)
{
    QPainter painter{this};
    painter.setTransform(mTransform);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.fillPath(mOuterPath, mGradient);
    painter.setPen({QColor{0x0A0A0A}, 2});
    painter.setBrush(QColor{0x161616});
    painter.drawPath(mInnerPath);
    painter.setPen(Qt::white);
    qreal textLeft = .5*(sOuterSize.width() - mProductText.size().width()
                                            - mModelText.size().width());
    painter.setFont(mProductFont);
    painter.drawStaticText(textLeft, .5*(60 - mProductText.size().height()), mProductText);
    painter.setFont(mModelFont);
    painter.drawStaticText(textLeft + mProductText.size().width(),
                           .5*(60 - mModelText.size().height()), mModelText);

    if (mOn) {
    } else {
        painter.fillRect(QRect{sInnerRect.center() - QPoint{160, 120}, QSize{320, 240}}, Qt::black);
        painter.drawStaticText(sInnerRect.center() - QRectF{{}, mUnpoweredText.size()}.center(),
                               mUnpoweredText);
    }
}

void ScreenWidget::resizeEvent(QResizeEvent *event)
{
    QSize size{sOuterSize.scaled(event->size(), Qt::KeepAspectRatio)},
        origin{event->size() - size};

    qreal m11 = qreal(size.width()) / sOuterSize.width();
    qreal m22 = qreal(size.height()) / sOuterSize.height();
    qreal dx = .5*origin.width();
    qreal dy = origin.height();

    mTransform.setMatrix(m11,  0, 0,
                         0,  m22, 0,
                         dx,  dy, 1);
    prepareText();
}

void ScreenWidget::prepareText()
{
    mProductText.prepare(mTransform, mProductFont);
    mModelText.prepare(mTransform, mModelFont);
    mUnpoweredText.prepare(mTransform, mUnpoweredFont);
}
