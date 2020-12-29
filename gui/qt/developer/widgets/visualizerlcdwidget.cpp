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

#include "visualizerlcdwidget.h"
#include "settings.h"
#include "util.h"

#include "../../core/lcd.h"

#include <cmath>
#include <QtCore/QMimeData>
#include <QtCore/QDir>
#include <QtGui/QClipboard>
#include <QtGui/QMouseEvent>
#include <QtGui/QDrag>
#include <QtGui/QPainter>
#include <QtWidgets/QMenu>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>

VisualizerLcdWidget::VisualizerLcdWidget(QWidget *parent)
    : QWidget{parent},
      mImage{nullptr},
      mUnpoweredText{tr("CALCULATOR OFF")},
      mUnpoweredFont{QStringLiteral("Arial"), 16}
{
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &VisualizerLcdWidget::customContextMenuRequested, this, &VisualizerLcdWidget::contextMenu);
}

VisualizerLcdWidget::~VisualizerLcdWidget()
{
    delete mImage;
}

void VisualizerLcdWidget::draw()
{
    emu_set_lcd_ptrs(&mConfig.mData, &mConfig.mDataEnd, mConfig.mWidth, mConfig.mHeight, mConfig.mBaseAddr, mConfig.mCtlReg, false);
    emu_lcd_drawmem(mImage->bits(), mConfig.mData, mConfig.mDataEnd, mConfig.mCtlReg, mConfig.mWidth * mConfig.mHeight, Settings::boolOption(Settings::EmuLcdSpi));
    update();
}

void VisualizerLcdWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    const QRect& cw = painter.window();

    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    if (mImage != nullptr)
    {
        painter.drawImage(cw, *mImage);

        // only draw grid if width/height scale >= 200%
        if (mConfig.mGrid && (cw.width() >= (mConfig.mWidth * 2) && cw.height() >= (mConfig.mHeight * 2)))
        {
            QVarLengthArray<QLineF, 100> lines;

            for (int x = cw.left(); x < cw.right(); x += (cw.width() / mConfig.mWidth))
            {
                lines.append(QLineF(x, cw.top(), x, cw.bottom()));
            }
            for (int y = cw.top(); y < cw.bottom(); y += (cw.height() / mConfig.mHeight))
            {
                lines.append(QLineF(cw.left(), y, cw.right(), y));
            }

            painter.drawLines(lines.data(), lines.size());
        }
    }
    else
    {
        painter.fillRect(painter.window(), Qt::black);
        painter.drawStaticText(painter.window().center() - QRectF{{}, mUnpoweredText.size()}.center(),
                               mUnpoweredText);
    }
}

void VisualizerLcdWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        QDrag *drag = new QDrag(this);
        QMimeData *mimeData = new QMimeData;
        QImage image = *mImage;
        QPixmap mymap = QPixmap::fromImage(image);
        QString path = QDir::tempPath() + QDir::separator() + QStringLiteral("cemu_") + Util::randomString(6) + QStringLiteral(".png");
        image.save(path, "PNG", 0);
        mimeData->setImageData(image);
        mimeData->setUrls(QList<QUrl>() << QUrl::fromLocalFile(path));
        drag->setMimeData(mimeData);
        drag->setHotSpot(event->pos());
        drag->setPixmap(mymap);
        switch (drag->exec(Qt::CopyAction | Qt::MoveAction, Qt::MoveAction))
        {
            case Qt::IgnoreAction:
            case Qt::CopyAction:
                QFile::remove(path);
                break;
            default:
                break;
        }
        event->accept();
    }
    else
    {
        event->ignore();
    }
}

void VisualizerLcdWidget::setRefreshRate(int rate)
{
    if (rate <= 0)
    {
        return;
    }

    connect(&mRefreshTimer, SIGNAL(timeout()), this, SLOT(draw()));

    mRefreshTimer.stop();
    mRefreshTimer.setInterval(1000 / rate);
    mRefreshTimer.start();
    mConfig.mRefresh = rate;
}

void VisualizerLcdWidget::setConfig(const VisualizerLcdWidgetConfig &config)
{
    mConfig = config;

    delete mImage;
    mImage = new QImage(mConfig.mWidth, mConfig.mHeight, QImage::Format_RGBX8888);
}

void VisualizerLcdWidget::contextMenu(const QPoint& posa)
{
    QString copyStr = tr("Copy Address");
    QString coordStr = tr("Coordinate: ");

    QTransform transform;
    transform.scale(mConfig.mWidth * 1.0 / width(), mConfig.mHeight * 1.0 / height());
    QPoint point = transform.map(posa);
    uint32_t x = static_cast<uint32_t>(point.x());
    uint32_t y = static_cast<uint32_t>(point.y());

    QString addr = Util::int2hex(mConfig.mBaseAddr + (static_cast<uint32_t>(std::floor((static_cast<float>(x)) / mConfig.mBppStep))) +
                                (static_cast<uint32_t>(mConfig.mWidth) * y), 6);

    coordStr += QString::number(x) + QStringLiteral("x") + QString::number(y);
    copyStr += QStringLiteral(" '") + addr + QStringLiteral("'");

    QMenu menu;
    menu.addAction(copyStr);
    menu.addSeparator();
    menu.addAction(coordStr);

    QAction *item = menu.exec(mapToGlobal(posa));
    if (item)
    {
        if (item->text() == copyStr)
        {
            QClipboard *clipboard = QApplication::clipboard();
            clipboard->setText(addr.toLatin1());
        }
    }
}
