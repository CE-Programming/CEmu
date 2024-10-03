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

#ifndef SCREENSHOTWIDGET_H
#define SCREENSHOTWIDGET_H

#include "overlaywidget.h"

#include <QtGui/QImage>
#include <QtWidgets/QLabel>
#include <QtWidgets/QWidget>
QT_BEGIN_NAMESPACE
class QPushButton;
QT_END_NAMESPACE

class ScreenshotOverlayWidget : public OverlayWidget
{
    Q_OBJECT

public:
    explicit ScreenshotOverlayWidget(QWidget *parent = nullptr);

signals:
    void saveScreenshot();

protected:
    void enterEvent(QEnterEvent *event) Q_DECL_OVERRIDE;
    void leaveEvent(QEvent *event) Q_DECL_OVERRIDE;

private:
    QPushButton *mBtnSave;
    QPushButton *mBtnRemove;
};

class ScreenshotWidget : public QLabel
{
    Q_OBJECT

public:
    explicit ScreenshotWidget(QWidget *parent = nullptr);
    void setImage(const QImage &image);

public slots:
    void saveImage();

private:
    ScreenshotOverlayWidget *mOverlay;
};

#endif
