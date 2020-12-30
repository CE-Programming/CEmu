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

#include "screenshotwidget.h"

#include <QtGui/QIcon>
#include <QtGui/QPainter>
#include <QtGui/QPalette>
#include <QtGui/QPixmap>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStyle>
#include <QtWidgets/QVBoxLayout>

ScreenshotOverlayWidget::ScreenshotOverlayWidget(QWidget *parent)
    : OverlayWidget{parent}
{
    mBtnSave = new QPushButton("Save");
    mBtnRemove = new QPushButton("Remove");

    mBtnSave->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    mBtnRemove->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

    mBtnSave->setIcon(QIcon(style()->standardIcon(QStyle::SP_DialogSaveButton)));
    mBtnRemove->setIcon(QIcon(style()->standardIcon(QStyle::SP_DialogDiscardButton)));

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setAlignment(Qt::AlignTop);
    layout->addWidget(mBtnSave);
    layout->addStretch();
    layout->addWidget(mBtnRemove);

    setLayout(layout);
    setMouseTracking(true);

    mBtnSave->setVisible(false);
    mBtnRemove->setVisible(false);

    connect(mBtnRemove, &QPushButton::clicked, [this]{ this->parent()->deleteLater(); });
    connect(mBtnSave, &QPushButton::clicked, this, &ScreenshotOverlayWidget::saveScreenshot);
}

void ScreenshotOverlayWidget::enterEvent(QEvent *event)
{
    QWidget::enterEvent(event);
    mBtnSave->setVisible(true);
    mBtnRemove->setVisible(true);
}

void ScreenshotOverlayWidget::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    mBtnSave->setVisible(false);
    mBtnRemove->setVisible(false);
}

ScreenshotWidget::ScreenshotWidget(QWidget *parent)
    : QLabel{parent}
{
    mOverlay = new ScreenshotOverlayWidget(this);

    connect(mOverlay, &ScreenshotOverlayWidget::saveScreenshot, this, &ScreenshotWidget::saveImage);
}

void ScreenshotWidget::setImage(const QImage &image)
{
    setPixmap(QPixmap::fromImage(image));
}

void ScreenshotWidget::saveImage()
{
    QFileDialog dialog(this, tr("Save Screenshot"));
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setDefaultSuffix(QStringLiteral(".png"));

    if (dialog.exec())
    {
        pixmap(Qt::ReturnByValue).save(dialog.selectedFiles().first());
    }
}



