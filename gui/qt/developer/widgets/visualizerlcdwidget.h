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

#ifndef VISUALIZERWIDGET_H
#define VISUALIZERWIDGET_H

#include <QtCore/QTimer>
#include <QtWidgets/QWidget>
#include <QtGui/QStaticText>

class VisualizerLcdWidgetConfig
{
public:
    int mHeight, mWidth, mRefresh;
    uint32_t mBaseAddr, mCtlReg;
    uint32_t *mData, *mDataEnd;
    float mBppStep;
    bool mGrid;
};

class VisualizerLcdWidget : public QWidget
{
  Q_OBJECT

public:
    explicit VisualizerLcdWidget(QWidget *p = nullptr);
    ~VisualizerLcdWidget();
    void setRefreshRate(int rate);
    void setConfig(const VisualizerLcdWidgetConfig &config);

protected:
    virtual void paintEvent(QPaintEvent *event) override;
    virtual void mousePressEvent(QMouseEvent *event) override;

private slots:
    void draw();
    void contextMenu(const QPoint &posa);

private:
    QTimer mRefreshTimer;
    QImage *mImage;

    VisualizerLcdWidgetConfig mConfig;

    QStaticText mUnpoweredText;
    QFont mUnpoweredFont;

};

#endif
