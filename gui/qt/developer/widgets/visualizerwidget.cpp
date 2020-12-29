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

#include "visualizerwidget.h"
#include "util.h"

#include "../../core/lcd.h"

#include <cmath>
#include <QtCore/QMimeData>
#include <QtCore/QDir>
#include <QtGui/QMouseEvent>
#include <QtGui/QDrag>
#include <QtGui/QPainter>
#include <QtWidgets/QMenu>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>

VisualizerWidget::VisualizerWidget(QWidget *parent)
    : QWidget{parent},
      mImage{nullptr}
{
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &VisualizerWidget::customContextMenuRequested, this, &VisualizerWidget::contextMenu);

    mImage = new QImage(LCD_WIDTH, LCD_HEIGHT, QImage::Format_RGBX8888);
}

VisualizerWidget::~VisualizerWidget()
{
    delete mImage;
}

void VisualizerWidget::draw()
{
    emu_set_lcd_ptrs(&mData, &mDataEnd, mWidth, mHeight, mUpBase, mControl, false);
    emu_lcd_drawmem(mImage->bits(), mData, mDataEnd, mControl, mSize, 0);
    update();
}

void VisualizerWidget::paintEvent(QPaintEvent*)
{
    QPainter c(this);
    const QRect& cw = c.window();

    c.setRenderHint(QPainter::SmoothPixmapTransform, cw.width() < mWidth);
    if (mImage != nullptr)
    {
        c.drawImage(cw, *mImage);

        // only draw grid if width/height scale >= 200%
        if (mGrid && (cw.width() >= (mWidth * 2) && cw.height() >= (mHeight * 2)))
        {
            QVarLengthArray<QLineF, 100> lines;

            for (int x = cw.left(); x < cw.right(); x += (cw.width() / mWidth))
            {
                lines.append(QLineF(x, cw.top(), x, cw.bottom()));
            }
            for (int y = cw.top(); y < cw.bottom(); y += (cw.height() / mHeight))
            {
                lines.append(QLineF(cw.left(), y, cw.right(), y));
            }

            c.drawLines(lines.data(), lines.size());
        }
    }
}

void VisualizerWidget::mousePressEvent(QMouseEvent *event)
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

void VisualizerWidget::setRefreshRate(int rate)
{
    if (rate == 0 || rate < 0)
    {
        return;
    }

    connect(&mRefreshTimer, SIGNAL(timeout()), this, SLOT(draw()));

    mRefreshTimer.stop();
    mRefreshTimer.setInterval(1000 / rate);
    mRefreshTimer.start();
    mRefresh = rate;
}

void VisualizerWidget::setConfig(float bppstep, int w, int h, uint32_t u, uint32_t c, bool g, uint32_t *d, uint32_t *e)
{
    mBppStep = bppstep;
    mWidth = w;
    mHeight = h;
    mUpBase = u;
    mControl = c;
    mData = d;
    mDataEnd = e;
    mSize = w * h;
    mGrid = g;
    delete mImage;
    mImage = new QImage(w, h, QImage::Format_RGBX8888);
}

void VisualizerWidget::contextMenu(const QPoint& posa)
{
    QString copyStr = tr("Copy Address");
    QString coordStr = tr("Coordinate: ");

    QTransform tr;
    tr.scale(mWidth * 1.0 / width(), mHeight * 1.0 / height());
    QPoint point = tr.map(posa);
    uint32_t x = static_cast<uint32_t>(point.x());
    uint32_t y = static_cast<uint32_t>(point.y());

    QString addr = Util::int2hex(mUpBase + (static_cast<uint32_t>(std::floor((static_cast<float>(x)) / mBppStep))) +
                                (static_cast<uint32_t>(mWidth) * y), 6);

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
