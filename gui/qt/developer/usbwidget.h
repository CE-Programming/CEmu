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

#ifdef HAS_LIBUSB
#ifndef USBWIDGET_H
#define USBWIDGET_H

#include "../dockedwidget.h"
class TableWidget;

#include <libusb.h>

QT_BEGIN_NAMESPACE
class QPushButton;
class QTableWidget;
class QTableWidgetItem;
QT_END_NAMESPACE

class UsbWidget : public DockedWidget
{
    Q_OBJECT

public:
    explicit UsbWidget(CoreWindow *coreWindow);

public slots:
    void usbRefresh();
    void setUsbLocale(const QLocale& locale);

signals:
    void usbHotplug(void *device, bool attached);

private slots:
    void itemPressed(QTableWidgetItem *item);

private:
    enum Column {
        Plug,
        Vid,
        Pid,
        Manufacturer,
        Product,
        Serial,
        Count,
    };

    void usbUpdate(void *opaqueDevice, bool attached = true);
    static int LIBUSB_CALL usbHotplugCallback(libusb_context *context, libusb_device *device,
        libusb_hotplug_event event, void *user_data);

    TableWidget *mTbl;
    QPushButton *mBtnRefresh;

    int mUsbLangID;

    /* todo: temporary until integrated in core */
    libusb_context *mUsbCtx;
    libusb_device *mUsbDev;
};

#endif
#endif
