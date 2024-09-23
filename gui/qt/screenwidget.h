/*
 * Copyright (c) 2015-2021 CE Programming.
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

#ifndef SCREENWIDGET_H
#define SCREENWIDGET_H

class CalculatorWidget;

#include <QtCore/QRect>
#include <QtCore/QString>
#include <QtGui/QFont>
#include <QtGui/QImage>
#include <QtGui/QLinearGradient>
#include <QtGui/QPainterPath>
#include <QtGui/QStaticText>
#include <QtGui/QTransform>
#include <QtWidgets/QWidget>
QT_BEGIN_NAMESPACE
class QPaintEvent;
class QResizeEvent;
QT_END_NAMESPACE

class ScreenWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ScreenWidget(CalculatorWidget *parent);
    ~ScreenWidget();

    void setScreen(const QString &skin = QString());
    void setModel(const QString &product, const QString &model, const QString &edition = QString());

    static const QRect sOuterRect, sOuterCorner, sInnerRect, sInnerCorner;

public slots:
    void lcdFrame();

protected:
    virtual void paintEvent(QPaintEvent *event) override;
    virtual void resizeEvent(QResizeEvent *event) override;

    QTransform mTransform;
    QPainterPath mOuterPath, mInnerPath;
    QLinearGradient mGradient;
    QStaticText mProductText, mModelText, mEditionText, mUnpoweredText;
    QFont mProductFont, mModelFont, mEditionFont, mUnpoweredFont;
    QImage mSkin;

private:
    CalculatorWidget *calcWidget();

    void prepareText();
};

#endif
