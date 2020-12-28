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

const QRect ScreenWidget::sOuterRect{0, 0, 450, 360},
            ScreenWidget::sOuterCorner{0, 0, 120, 120},
            ScreenWidget::sInnerRect = sOuterRect - QMargins{32, 60, 32, 16},
            ScreenWidget::sInnerCorner{0, 0, 60, 60};

ScreenWidget::ScreenWidget(QWidget *parent)
    : QWidget(parent),
      mGradient{sOuterRect.topLeft(), sOuterRect.topRight()},
      mUnpoweredText{tr("LCD OFF")},
      mProductFont{QStringLiteral("Arial"), 19, QFont::Black},
      mModelFont{QStringLiteral("Arial"), 19, QFont::Thin},
      mEditionFont{QStringLiteral("Arial"), 12, QFont::Bold},
      mUnpoweredFont{QStringLiteral("Arial"), 16}
{
    mGradient.setColorAt(0, 0x363636);
    mGradient.setColorAt(0 + 100./sOuterRect.width(), 0x1C1C1C);
    mGradient.setColorAt(1 - 100./sOuterRect.width(), 0x1C1C1C);
    mGradient.setColorAt(1, 0x363636);

    mOuterPath.moveTo(sOuterRect.bottomLeft() + QPoint{0, 1});
    mOuterPath.lineTo(sOuterRect.topLeft() + QPoint{0, sOuterCorner.center().y()});
    mOuterPath.arcTo(sOuterCorner.translated(sOuterRect.topLeft()), 180, -90);
    QPoint topRightOffset{sOuterCorner.center().x(), 0},
        topRightPoint = sOuterRect.topRight() + QPoint{1, 0} - topRightOffset;
    mOuterPath.lineTo(topRightPoint);
    mOuterPath.arcTo(sOuterCorner.translated(topRightPoint - topRightOffset), 90, -90);
    mOuterPath.lineTo(sOuterRect.bottomRight() + QPoint{1, 1});

    mInnerPath.addRoundedRect(sInnerRect, sInnerCorner.center().x(), sInnerCorner.center().y());

    mEditionFont.setLetterSpacing(QFont::AbsoluteSpacing, 3);

    prepareText();
}

ScreenWidget::~ScreenWidget()
{
}

void ScreenWidget::setScreen(const QString &skin)
{
    mSkin = QImage(skin);
}

void ScreenWidget::setModel(const QString &product, const QString &model, const QString &edition)
{
    mProductText.setText(product);
    mModelText.setText(model);
    mEditionText.setText(edition);
    prepareText();
}

void ScreenWidget::paintEvent(QPaintEvent */*event*/)
{
    QPainter painter{this};
    painter.setTransform(mTransform);
    painter.setRenderHints(QPainter::Antialiasing |
                           QPainter::TextAntialiasing |
                           QPainter::SmoothPixmapTransform);

    painter.fillPath(mOuterPath, mGradient);
    painter.setPen({QColor{0x0A0A0A}, 3});
    painter.setBrush(QColor{0x161616});
    painter.drawPath(mInnerPath);

    painter.setPen(Qt::white);
    QRect upperRect = sOuterRect;
    upperRect.setBottom(sInnerRect.top());
    qreal topMaxHeight = qMax(mProductText.size().height(), mModelText.size().height());
    QRectF textRect{{}, QSizeF{mProductText.size().width() + mModelText.size().width(), topMaxHeight}};
    if (!mEditionText.text().isNull())
        textRect += QMarginsF{0, 0, 0, mEditionText.size().height()};
    textRect.moveCenter(upperRect.center());
    painter.setFont(mProductFont);
    painter.drawStaticText(textRect.topLeft(), mProductText);
    painter.setFont(mModelFont);
    painter.drawStaticText(textRect.topLeft() + QPointF{mProductText.size().width(), 0}, mModelText);
    painter.setFont(mEditionFont);
    textRect.setWidth(mEditionText.size().width());
    textRect.moveCenter(upperRect.center());
    painter.drawStaticText(textRect.topLeft() + QPointF{0, topMaxHeight}, mEditionText);

    QRect screenRect{sInnerRect.center() - QPoint{160, 120}, QSize{320, 240}};
    if (mSkin.isNull())
    {
        painter.fillRect(screenRect, Qt::black);
        painter.drawStaticText(sInnerRect.center() - QRectF{{}, mUnpoweredText.size()}.center(),
                               mUnpoweredText);
    }
    else
    {
        painter.drawImage(screenRect, mSkin);
    }
}

void ScreenWidget::resizeEvent(QResizeEvent *event)
{
    QSizeF size = QSizeF(sOuterRect.size()).scaled(event->size(), Qt::KeepAspectRatio),
        origin = event->size() - size;

    qreal m11 = size.width() / sOuterRect.width();
    qreal m22 = size.height() / sOuterRect.height();
    qreal dx = .5*origin.width();
    qreal dy = origin.height();

    mTransform.setMatrix(m11,  0    , 0,
                         0,  m22, 0,
                         dx,  dy, 1);
    prepareText();
}

void ScreenWidget::prepareText()
{
    mProductText.prepare(mTransform, mProductFont);
    mModelText.prepare(mTransform, mModelFont);
    mEditionText.prepare(mTransform, mEditionFont);
    mUnpoweredText.prepare(mTransform, mUnpoweredFont);
}
