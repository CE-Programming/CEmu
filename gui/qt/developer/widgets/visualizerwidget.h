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
#include <QtGui/QClipboard>

class VisualizerWidget : public QWidget {
  Q_OBJECT

public:
    explicit VisualizerWidget(QWidget *p = nullptr);
    ~VisualizerWidget();
    void setRefreshRate(int rate);
    void setConfig(float bppstep, int w, int h, uint32_t u, uint32_t c, bool g, uint32_t *d, uint32_t *e);

protected:
    virtual void paintEvent(QPaintEvent *event) override;
    virtual void mousePressEvent(QMouseEvent *event) override;

private slots:
    void draw();
    void contextMenu(const QPoint &posa);

private:
    QTimer mRefreshTimer;
    QImage *mImage;
    int mRefresh;

    int mHeight;
    int mWidth;
    int mSize;
    bool mGrid;
    uint32_t mUpBase;
    uint32_t mControl;
    float mBppStep;
    uint32_t *mData;
    uint32_t *mDataEnd;
};

#endif
