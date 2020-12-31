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

#include "capturewidget.h"
#include "screenshotwidget.h"

#include <kddockwidgets/DockWidget.h>

#include <QtGui/QClipboard>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QVBoxLayout>

CaptureWidget::CaptureWidget(DockedWidgetList &list)
    : DockedWidget{new KDDockWidgets::DockWidget{QStringLiteral("Screen Capture")}, list}
{
    QLabel *lblFrameskip = new QLabel(tr("Frameskip: "));

    QGroupBox *grpScreenshot = new QGroupBox(tr("Take screenshot"));
    QGroupBox *grpAnimated = new QGroupBox(tr("Record animated"));
    QGroupBox *grpTaken = new QGroupBox(tr("Screenshots taken"));

    QPushButton *btnScreenshot = new QPushButton(tr("Screenshot"));
    QPushButton *btnCopyScreen = new QPushButton(tr("Copy screen"));
    QPushButton *btnRecord = new QPushButton(tr("Record screen"));
    QSpinBox *spnFrameskip = new QSpinBox;

#ifndef HAS_APNG_SUPPORT
    grpAnimated->setEnabled(false);
#endif

    spnFrameskip->setMinimum(0);
    spnFrameskip->setMaximum(5);
    spnFrameskip->setValue(1);

    QHBoxLayout *hboxScreenshot = new QHBoxLayout;
    hboxScreenshot->addWidget(btnScreenshot);
    hboxScreenshot->addStretch();
    hboxScreenshot->addWidget(btnCopyScreen);
    grpScreenshot->setLayout(hboxScreenshot);

    QHBoxLayout *hboxAnimated = new QHBoxLayout;
    hboxAnimated->addWidget(btnRecord);
    hboxAnimated->addStretch();
    hboxAnimated->addWidget(lblFrameskip);
    hboxAnimated->addWidget(spnFrameskip);
    grpAnimated->setLayout(hboxAnimated);

    QScrollArea *scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    QWidget *widget = new QWidget;

    mLayoutTaken = new QVBoxLayout;

    QHBoxLayout *hboxScroll = new QHBoxLayout;
    hboxScroll->addStretch();
    hboxScroll->addLayout(mLayoutTaken);
    hboxScroll->addStretch();

    QVBoxLayout *vboxScroll = new QVBoxLayout;
    vboxScroll->addLayout(hboxScroll);
    vboxScroll->addStretch();
    widget->setLayout(vboxScroll);
    scrollArea->setWidget(widget);

    QHBoxLayout *hboxTaken = new QHBoxLayout;
    hboxTaken->addWidget(scrollArea);
    grpTaken->setLayout(hboxTaken);

    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->addWidget(grpScreenshot);
    hLayout->addWidget(grpAnimated);

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addLayout(hLayout);
    vLayout->addWidget(grpTaken);

    setLayout(vLayout);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    connect(btnScreenshot, &QPushButton::clicked, this, &CaptureWidget::takeScreenshot);
    connect(btnCopyScreen, &QPushButton::clicked, this, &CaptureWidget::copyScreen);
}

void CaptureWidget::takeScreenshot()
{
    ScreenshotWidget *screenshot = new ScreenshotWidget;

    screenshot->setImage(QImage(QStringLiteral(":/assets/test/screen.png")));

    mLayoutTaken->insertWidget(0, screenshot);
}

void CaptureWidget::copyScreen()
{
    QApplication::clipboard()->setImage(QImage(QStringLiteral(":/assets/test/screen.png")), QClipboard::Clipboard);
}
