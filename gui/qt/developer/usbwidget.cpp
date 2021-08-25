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

#ifdef HAS_LIBUSB
#include "usbwidget.h"

#include "../corewrapper.h"
#include "../tablewidget.h"
#include "../util.h"

#include <kddockwidgets/DockWidget.h>

#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSizePolicy>

UsbWidget::UsbWidget(CoreWindow *coreWindow)
    : DockedWidget{new KDDockWidgets::DockWidget{QStringLiteral("USB Devices")},
                   QIcon(QStringLiteral(":/assets/icons/usb.svg")),
                   coreWindow}
{
    mBtnRefresh = new QPushButton{QIcon(QStringLiteral(":/assets/icons/process.svg")), tr("Refresh Device List")};

    mTbl = new TableWidget{0, 6};
    mTbl->setHorizontalHeaderLabels({tr("Plug"), tr("VID"), tr("PID"), tr("Manufacturer"), tr("Product"), tr("Serial Number")});
    mTbl->horizontalHeader()->setStretchLastSection(true);
    mTbl->horizontalHeader()->setDefaultSectionSize(QFontMetrics(Util::monospaceFont()).maxWidth() * 10);
    mTbl->horizontalHeader()->setMinimumSectionSize(mTbl->verticalHeader()->defaultSectionSize());
    mTbl->horizontalHeader()->setSectionResizeMode(Column::Plug, QHeaderView::ResizeToContents);
    mTbl->horizontalHeader()->setSectionResizeMode(Column::Vid, QHeaderView::ResizeToContents);
    mTbl->horizontalHeader()->setSectionResizeMode(Column::Pid, QHeaderView::ResizeToContents);
    mTbl->verticalHeader()->setVisible(false);
    mTbl->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    mTbl->setSelectionMode(QAbstractItemView::NoSelection);

    QHBoxLayout *layoutBtns = new QHBoxLayout;
    layoutBtns->addWidget(mBtnRefresh);
    layoutBtns->addStretch();

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addLayout(layoutBtns);
    layout->addWidget(mTbl);
    setLayout(layout);

    setUsbLocale(QLocale::system());

    connect(mTbl, &TableWidget::itemPressed, this, &UsbWidget::itemPressed);
    if (!libusb_init(&mUsbCtx))
    {
        if (libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG))
        {
            connect(this, &UsbWidget::usbHotplug, this, &UsbWidget::usbUpdate);
            libusb_hotplug_register_callback(mUsbCtx,
                                             libusb_hotplug_event(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED |
                                                                  LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT),
                                             LIBUSB_HOTPLUG_ENUMERATE,
                                             LIBUSB_HOTPLUG_MATCH_ANY,
                                             LIBUSB_HOTPLUG_MATCH_ANY,
                                             LIBUSB_HOTPLUG_MATCH_ANY,
                                             UsbWidget::usbHotplugCallback, this, nullptr);
        }
        else
        {
            usbRefresh();
        }
        connect(mBtnRefresh, &QPushButton::clicked, this, &UsbWidget::usbRefresh);
    }
}

void UsbWidget::itemPressed(QTableWidgetItem *item)
{
    if (item->column() != Column::Plug)
        return;

    for (int i = 0; i < mTbl->rowCount(); ++i)
    {
        QTableWidgetItem *valid = mTbl->item(i, Column::Vid);
        bool plugged = valid->data(Qt::UserRole).toBool();
        valid->setData(Qt::UserRole, i == item->row() ? !plugged : false);
        QTableWidgetItem *connected = mTbl->item(i, Column::Plug);
        if (valid->data(Qt::UserRole).toBool())
        {
            connected->setIcon(QIcon{QStringLiteral(":/assets/icons/broken_link.svg")});
        }
        else
        {
            connected->setIcon(QIcon{});
        }
    }
}

int LIBUSB_CALL UsbWidget::usbHotplugCallback(libusb_context *context, libusb_device *device,
                                               libusb_hotplug_event event, void *user_data)
{
    (void)context;
    if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED ||
        event == LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT)
    {
        emit static_cast<UsbWidget *>(user_data)->
            usbHotplug(device, event == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED);
    }
    return 0;
}

void UsbWidget::usbUpdate(void *opaqueDevice, bool attached)
{
    int row;
    for (row = 0; row != mTbl->rowCount(); ++row)
    {
        if (mTbl->item(row, Column::Plug)->data(Qt::UserRole).value<void *>() == opaqueDevice)
            break;
    }
    libusb_device *device = static_cast<libusb_device *>(opaqueDevice);
    if (attached) {
        if (row != mTbl->rowCount())
            return;

        libusb_device_handle *handle;
        if (libusb_open(device, &handle))
            return;

        libusb_device_descriptor desc;
        libusb_get_device_descriptor(device, &desc);

        unsigned char string[256];
        quint16 langid = 0x0409;
        {
            int size = libusb_get_string_descriptor(handle, 0, 0, string, sizeof(string));
            for (int i = 2; i < size; i += 2) {
                quint16 cur = string[i + 1] << 8 | string[i];
                if (cur == mUsbLangID)
                {
                    langid = cur;
                    break;
                }
                if (i == 2 || !((cur ^ mUsbLangID) & 0x3ff))
                    langid = cur;
            }
        }

        mTbl->setSortingEnabled(false);
        int row = mTbl->rowCount();
        mTbl->setRowCount(row + 1);

        QTableWidgetItem *item = new QTableWidgetItem;
        libusb_ref_device(device);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        item->setData(Qt::UserRole, QVariant::fromValue(opaqueDevice));
        item->setTextAlignment(Qt::AlignCenter);
        mTbl->setItem(row, Column::Plug, item);

        for (int i = Column::Vid; i <= Column::Pid; i++)
        {
            item = new QTableWidgetItem(Util::int2hex((&desc.idVendor)[i - Column::Vid], 4));
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            item->setData(Qt::UserRole, false);
            item->setTextAlignment(Qt::AlignCenter);
            mTbl->setItem(row, i, item);
        }

        for (int i = Column::Manufacturer; i <= Column::Serial; i++)
        {
            if (uint8_t index = (&desc.iManufacturer)[i - Column::Manufacturer])
            {
                int size = libusb_get_string_descriptor(handle, index, langid, reinterpret_cast<unsigned char *>(string), sizeof(string));
                item = new QTableWidgetItem;
                if (size >= 4)
                {
                    string[0] = QChar::ByteOrderMark & 0xFF; string[1] = QChar::ByteOrderMark >> 8;
                    item->setText(QString::fromUtf16(reinterpret_cast<char16_t *>(string), size >> 1));
                }
                if (item->text().simplified().trimmed().isEmpty())
                {
                    item->setText(QStringLiteral("N/A"));
                    item->setForeground(QColor{Qt::darkGray});
                }
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                mTbl->setItem(row, i, item);
            }
        }
        mTbl->setSortingEnabled(true);
        mTbl->resizeColumnsToContents();

        libusb_close(handle);
    }
    else if (row != mTbl->rowCount())
    {
        mTbl->removeRow(row);
        mTbl->resizeColumnsToContents();
        libusb_unref_device(device);
    }
}

void UsbWidget::usbRefresh()
{
    for (int row = 0; row != mTbl->rowCount(); ++row)
    {
        libusb_unref_device(static_cast<libusb_device *>(mTbl->item(row, Column::Plug)->data(Qt::UserRole).value<void *>()));
    }
    mTbl->setRowCount(0);

    libusb_device **devices;
    if (libusb_get_device_list(mUsbCtx, &devices) < 0)
        return;

    for (libusb_device **device = devices; device && *device; device++)
    {
        usbUpdate(*device);
    }
    libusb_free_device_list(devices, 0);
}

void UsbWidget::setUsbLocale(const QLocale& locale)
{
    static const QHash<QLocale, quint16> langIDMap =
    {
        {QLocale::Afrikaans, 0x0436},
        {QLocale::Albanian, 0x041c},
        {{QLocale::Arabic, QLocale::SaudiArabia}, 0x0401},
        {{QLocale::Arabic, QLocale::Iraq}, 0x0801},
        {{QLocale::Arabic, QLocale::Egypt}, 0x0c01},
        {{QLocale::Arabic, QLocale::Libya}, 0x1001},
        {{QLocale::Arabic, QLocale::Algeria}, 0x1401},
        {{QLocale::Arabic, QLocale::Morocco}, 0x1801},
        {{QLocale::Arabic, QLocale::Tunisia}, 0x1c01},
        {{QLocale::Arabic, QLocale::Oman}, 0x2001},
        {{QLocale::Arabic, QLocale::Yemen}, 0x2401},
        {{QLocale::Arabic, QLocale::Syria}, 0x2801},
        {{QLocale::Arabic, QLocale::Jordan}, 0x2c01},
        {{QLocale::Arabic, QLocale::Lebanon}, 0x3001},
        {{QLocale::Arabic, QLocale::Kuwait}, 0x3401},
        {{QLocale::Arabic, QLocale::UnitedArabEmirates}, 0x3801},
        {{QLocale::Arabic, QLocale::Bahrain}, 0x3c01},
        {{QLocale::Arabic, QLocale::Qatar}, 0x4001},
        {QLocale::Armenian, 0x042b},
        {QLocale::Assamese, 0x044d},
        {{QLocale::Azerbaijani, QLocale::LatinScript, QLocale::Azerbaijan}, 0x042c},
        {{QLocale::Azerbaijani, QLocale::CyrillicScript, QLocale::Azerbaijan}, 0x082c},
        {QLocale::Basque, 0x042d},
        {QLocale::Belarusian, 0x0423},
        {QLocale::Bengali, 0x0445},
        {QLocale::Bulgarian, 0x0402},
        {QLocale::Burmese, 0x0455},
        {QLocale::Catalan, 0x0403},
        {{QLocale::Chinese, QLocale::Taiwan}, 0x0404},
        {{QLocale::Chinese, QLocale::China}, 0x0804},
        {{QLocale::Chinese, QLocale::HongKong}, 0x0c04},
        {{QLocale::Chinese, QLocale::Singapore}, 0x1004},
        {{QLocale::Chinese, QLocale::Macau}, 0x1404},
        {QLocale::Croatian, 0x041a},
        {QLocale::Czech, 0x0405},
        {QLocale::Danish, 0x0406},
        {{QLocale::Dutch, QLocale::Netherlands}, 0x0413},
        {{QLocale::Dutch, QLocale::Belgium}, 0x0813},
        {{QLocale::English, QLocale::UnitedStates}, 0x0409},
        {{QLocale::English, QLocale::UnitedKingdom}, 0x0809},
        {{QLocale::English, QLocale::Australia}, 0x0c09},
        {{QLocale::English, QLocale::Canada}, 0x1009},
        {{QLocale::English, QLocale::NewZealand}, 0x1409},
        {{QLocale::English, QLocale::Ireland}, 0x1809},
        {{QLocale::English, QLocale::SouthAfrica}, 0x1c09},
        {{QLocale::English, QLocale::Jamaica}, 0x2009},
        //{{QLocale::English, QLocale::Caribbean}, 0x2409},
        {{QLocale::English, QLocale::Belize}, 0x2809},
        {{QLocale::English, QLocale::TrinidadAndTobago}, 0x2c09},
        {{QLocale::English, QLocale::Zimbabwe}, 0x3009},
        {{QLocale::English, QLocale::Philippines}, 0x3409},
        {QLocale::Estonian, 0x0425},
        {QLocale::Faroese, 0x0438},
        {QLocale::Persian, 0x0429},
        {QLocale::Finnish, 0x040b},
        {QLocale::French, 0x040c},
        {{QLocale::French, QLocale::Belgium}, 0x080c},
        {{QLocale::French, QLocale::Canada}, 0x0c0c},
        {{QLocale::French, QLocale::Switzerland}, 0x100c},
        {{QLocale::French, QLocale::Luxembourg}, 0x140c},
        {{QLocale::French, QLocale::Monaco}, 0x180c},
        {QLocale::Georgian, 0x0437},
        {QLocale::German, 0x0407},
        {{QLocale::German, QLocale::Switzerland}, 0x0807},
        {{QLocale::German, QLocale::Austria}, 0x0c07},
        {{QLocale::German, QLocale::Luxembourg}, 0x1007},
        {{QLocale::German, QLocale::Liechtenstein}, 0x1407},
        {QLocale::Greek, 0x0408},
        {QLocale::Gujarati, 0x0447},
        {QLocale::Hebrew, 0x040d},
        {QLocale::Hindi, 0x0439},
        {QLocale::Hungarian, 0x040e},
        {QLocale::Icelandic, 0x040f},
        {QLocale::Indonesian, 0x0421},
        {QLocale::Italian, 0x0410},
        {{QLocale::Italian, QLocale::Switzerland}, 0x0810},
        {QLocale::Japanese, 0x0411},
        {QLocale::Kannada, 0x044b},
        {{QLocale::Kashmiri, QLocale::India}, 0x0860},
        {QLocale::Kazakh, 0x043f},
        {QLocale::Konkani, 0x0457},
        {QLocale::Korean, 0x0412},
        //{{QLocale::Korean, QLocale::Johab}, 0x0812},
        {QLocale::Latvian, 0x0426},
        {QLocale::Lithuanian, 0x0427},
        //{{QLocale::Lithuanian, QLocale::Classic}, 0x0827},
        {QLocale::Macedonian, 0x042f},
        {{QLocale::Malay, QLocale::Malaysia}, 0x043e},
        {{QLocale::Malay, QLocale::Brunei}, 0x083e},
        {QLocale::Malayalam, 0x044c},
        {QLocale::Manipuri, 0x0458},
        {QLocale::Marathi, 0x044e},
        {{QLocale::Nepali, QLocale::India}, 0x0861},
        {{QLocale::NorwegianBokmal}, 0x0414},
        {{QLocale::NorwegianNynorsk}, 0x0814},
        {QLocale::Oriya, 0x0448},
        {QLocale::Polish, 0x0415},
        {{QLocale::Portuguese, QLocale::Brazil}, 0x0416},
        {QLocale::Portuguese, 0x0816},
        {QLocale::Punjabi, 0x0446},
        {QLocale::Romanian, 0x0418},
        {QLocale::Russian, 0x0419},
        {QLocale::Sanskrit, 0x044f},
        {{QLocale::Serbian, QLocale::CyrillicScript, QLocale::Serbia}, 0x0c1a},
        {{QLocale::Serbian, QLocale::LatinScript, QLocale::Serbia}, 0x081a},
        {QLocale::Sindhi, 0x0459},
        {QLocale::Slovak, 0x041b},
        {QLocale::Slovenian, 0x0424},
        {QLocale::Spanish, 0x040a},
        {{QLocale::Spanish, QLocale::Mexico}, 0x080a},
        //{{QLocale::Spanish, QLocale::ModernSort}, 0x0c0a},
        {{QLocale::Spanish, QLocale::Guatemala}, 0x100a},
        {{QLocale::Spanish, QLocale::CostaRica}, 0x140a},
        {{QLocale::Spanish, QLocale::Panama}, 0x180a},
        {{QLocale::Spanish, QLocale::DominicanRepublic}, 0x1c0a},
        {{QLocale::Spanish, QLocale::Venezuela}, 0x200a},
        {{QLocale::Spanish, QLocale::Colombia}, 0x240a},
        {{QLocale::Spanish, QLocale::Peru}, 0x280a},
        {{QLocale::Spanish, QLocale::Argentina}, 0x2c0a},
        {{QLocale::Spanish, QLocale::Ecuador}, 0x300a},
        {{QLocale::Spanish, QLocale::Chile}, 0x340a},
        {{QLocale::Spanish, QLocale::Uruguay}, 0x380a},
        {{QLocale::Spanish, QLocale::Paraguay}, 0x3c0a},
        {{QLocale::Spanish, QLocale::Bolivia}, 0x400a},
        {{QLocale::Spanish, QLocale::ElSalvador}, 0x440a},
        {{QLocale::Spanish, QLocale::Honduras}, 0x480a},
        {{QLocale::Spanish, QLocale::Nicaragua}, 0x4c0a},
        {{QLocale::Spanish, QLocale::PuertoRico}, 0x500a},
        {QLocale::SouthernSotho, 0x0430},
        {{QLocale::Swahili, QLocale::Kenya}, 0x0441},
        {QLocale::Swedish, 0x041d},
        {{QLocale::Swedish, QLocale::Finland}, 0x081d},
        {QLocale::Tamil, 0x0449},
        //{{QLocale::Tatar, QLocale::Tatarstan}, 0x0444},
        {QLocale::Telugu, 0x044a},
        {QLocale::Thai, 0x041e},
        {QLocale::Turkish, 0x041f},
        {QLocale::Ukrainian, 0x0422},
        {{QLocale::Urdu, QLocale::Pakistan}, 0x0420},
        {{QLocale::Urdu, QLocale::India}, 0x0820},
        {{QLocale::Uzbek, QLocale::LatinScript, QLocale::Uzbekistan}, 0x0443},
        {{QLocale::Uzbek, QLocale::CyrillicScript, QLocale::Uzbekistan}, 0x0843},
        {QLocale::Vietnamese, 0x042a},
    };
    mUsbLangID = langIDMap.value(locale, langIDMap.value(locale.language(), 0x0409));
    libusb_setlocale(locale.name().toUtf8().constData());
}
#endif
