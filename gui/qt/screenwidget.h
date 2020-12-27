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

#ifndef SCREENWIDGET_H
#define SCREENWIDGET_H

#include <QtGui/QPainterPath>
#include <QtGui/QResizeEvent>
#include <QtGui/QStaticText>
#include <QtWidgets/QWidget>

class ScreenWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ScreenWidget(QWidget *parent = nullptr);
    ~ScreenWidget();

    void setModel(const QString &product, const QString &model);

protected:
    virtual void paintEvent(QPaintEvent *event) override;
    virtual void resizeEvent(QResizeEvent *event) override;

    QTransform mTransform;
    QPainterPath mOuterPath, mInnerPath;
    QLinearGradient mGradient;
    QStaticText mProductText, mModelText, mUnpoweredText;
    QFont mProductFont, mModelFont, mUnpoweredFont;
    bool mOn;

    static const QSize sOuterSize, sOuterCorner, sInnerCorner;
    static const QRect sInnerRect;

private:
    void prepareText();
};

#endif
