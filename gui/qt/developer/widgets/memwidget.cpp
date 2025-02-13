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

#include "memwidget.h"

#include "../../corethread.h"
#include "../../dockedwidget.h"
#include "../../util.h"
#include "hexwidget.h"

#include <kddockwidgets/DockWidget.h>

#include <QtWidgets/QApplication>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMenu>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QSpinBox>

MemWidget::MemWidget(DockedWidget *parent)
    : QWidget{parent},
      mCharsetNoneText{tr("None")},
      mCharsetTiAsciiText{QStringLiteral("TI ASCII")},
      mCharsetAsciiText{QStringLiteral("ASCII")},
      mAreaRamText{tr("RAM")},
      mAreaFlashText{tr("Flash")},
      mAreaMemZ80Text{tr("Z80 Memory")},
      mAreaMemADLText{tr("ADL Memory")},
      mAreaPortsText{tr("Ports")},
      mSearchHexText{tr("Hex")},
      mSearchAsciiText{tr("ASCII")},
      mSearchHex{true}
{
    mEdtAddr = new QLineEdit;
    mEdtAddr->setFont(Util::monospaceFont());
    mBtnCharset = new QPushButton(QIcon(QStringLiteral(":/assets/icons/alphabetical_az.svg")), QString());
    mBtnArea = new QPushButton(QIcon(QStringLiteral(":/assets/icons/electronics.svg")), QString());

    mView = new HexWidget{this};

    QLabel *lblNumBytes = new QLabel(tr("Bytes per row") + ':');
    QSpinBox *spnNumBytes = new QSpinBox;
    QLabel *lblByteOff = new QLabel(tr("Byte offset") + ':');
    QSpinBox *spnByteOff = new QSpinBox;
    QPushButton *btnGoto = new QPushButton(QIcon(QStringLiteral(":/assets/icons/ok.svg")), tr("Goto"));
    mBtnSearch = new QPushButton(QIcon(QStringLiteral(":/assets/icons/search.svg")), tr("Search"));

    QHBoxLayout *hboxBtns = new QHBoxLayout;
    hboxBtns->addWidget(mEdtAddr);
    hboxBtns->addWidget(btnGoto);
    hboxBtns->addWidget(mBtnSearch);

    QHBoxLayout *hboxBtmBtns = new QHBoxLayout;
    hboxBtmBtns->addWidget(lblNumBytes);
    hboxBtmBtns->addWidget(spnNumBytes);
    hboxBtmBtns->addWidget(lblByteOff);
    hboxBtmBtns->addWidget(spnByteOff);
    hboxBtmBtns->addStretch();
    hboxBtmBtns->addWidget(mBtnCharset);
    hboxBtmBtns->addWidget(mBtnArea);

    QLineEdit *edtSearch = new QLineEdit(mSearch);
    QPushButton *btnNext = new QPushButton(tr("Next"));
    QPushButton *btnPrev = new QPushButton(tr("Previous"));

    mBtnSearchType = new QPushButton(QIcon(QStringLiteral(":/assets/icons/alphabetical_az.svg")), mSearchHexText);
    mBtnSearch->setCheckable(true);

    QHBoxLayout *searchLayout = new QHBoxLayout;
    searchLayout->addWidget(edtSearch);
    searchLayout->addWidget(btnNext);
    searchLayout->addWidget(btnPrev);
    searchLayout->addWidget(mBtnSearchType);

    mGrpSearch = new QGroupBox;
    mGrpSearch->setLayout(searchLayout);
    mGrpSearch->setVisible(false);

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addLayout(hboxBtns);
    vLayout->addWidget(mGrpSearch);
    vLayout->addWidget(mView);
    vLayout->addLayout(hboxBtmBtns);
    setLayout(vLayout);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    spnNumBytes->setMinimum(1);
    spnNumBytes->setMaximum(256);
    spnNumBytes->setValue(mView->bytesPerLine());
    spnByteOff->setMaximum(spnNumBytes->value() - 1);
    switch (mView->charset())
    {
        case HexWidget::Charset::Ascii:
            mBtnCharset->setText(mCharsetAsciiText);
            break;
        case HexWidget::Charset::TIAscii:
            mBtnCharset->setText(mCharsetTiAsciiText);
            break;
        case HexWidget::Charset::None:
            mBtnCharset->setText(mCharsetNoneText);
            break;
    }

    connect(mBtnSearch, &QPushButton::clicked, this, &MemWidget::showSearchGroup);
    connect(spnNumBytes, QOverload<int>::of(&QSpinBox::valueChanged), [this, spnByteOff](int value)
    {
        mView->setBytesPerLine(value);
        spnByteOff->setMaximum(value - 1);
    });
    connect(spnByteOff, QOverload<int>::of(&QSpinBox::valueChanged), mView, &HexWidget::setByteOff);
    connect(mBtnCharset, &QPushButton::clicked, this, &MemWidget::selectCharset);
    connect(mBtnArea, &QPushButton::clicked, this, &MemWidget::selectArea);
    connect(mBtnSearchType, &QPushButton::clicked, this, &MemWidget::selectSearchType);
    /*we
    connect(&core(), &CoreWrapper::flashSizeChanged, [this](qint32 len)
    {
        if (area() == Area::Flash)
        {
            mView->setProp(cemucore::CEMUCORE_PROP_FLASH, len);
        }
    });
    */

    connect(btnGoto, &QPushButton::clicked, [this]
    {
        mView->gotoAddr(Util::hex2int(mEdtAddr->text()));
    });
    connect(mEdtAddr, &QLineEdit::returnPressed, [this]
    {
        mView->gotoAddr(Util::hex2int(mEdtAddr->text()));
    });

    setArea(Area::MemADL);
}

DockedWidget *MemWidget::parent() const
{
    return static_cast<DockedWidget *>(QWidget::parent());
}

auto MemWidget::area() const -> Area
{
    return mArea;
}

void MemWidget::setArea(Area area)
{
    /*
    cemucore::prop prop;
    qint32 len;
    switch (area)
    {
        case Area::MemZ80:
            prop = cemucore::CEMUCORE_PROP_MEM_Z80;
            len = INT32_C(1) << 16;
            mBtnArea->setText(mAreaMemZ80Text);
            break;
        case Area::MemADL:
            prop = cemucore::CEMUCORE_PROP_MEM_ADL;
            len = INT32_C(1) << 24;
            mBtnArea->setText(mAreaMemADLText);
            break;
        case Area::Flash:
            prop = cemucore::CEMUCORE_PROP_FLASH;
            len = core().get(cemucore::CEMUCORE_PROP_FLASH_SIZE, 0);
            mBtnArea->setText(mAreaFlashText);
            break;
        case Area::Ram:
            prop = cemucore::CEMUCORE_PROP_RAM;
            len = INT32_C(0x65800);
            mBtnArea->setText(mAreaRamText);
            break;
        case Area::Port:
            prop = cemucore::CEMUCORE_PROP_PORT;
            len = INT32_C(1) << 16;
            mBtnArea->setText(mAreaPortsText);
            break;
    }
    mView->setProp(prop, len);
    mArea = area;
    */
}

void MemWidget::selectCharset()
{
    QMenu menu;
    menu.addAction(mCharsetAsciiText);
    menu.addAction(mCharsetTiAsciiText);
    menu.addAction(mCharsetNoneText);

    QAction *action = menu.exec(mBtnCharset->mapToGlobal(QPoint{0, mBtnCharset->height() + 1}));
    if (action)
    {
        mBtnCharset->setText(action->text());
        if (action->text() == mCharsetAsciiText)
        {
            mView->setCharset(HexWidget::Charset::Ascii);
        }
        else if (action->text() == mCharsetTiAsciiText)
        {
            mView->setCharset(HexWidget::Charset::TIAscii);
        }
        else if (action->text() == mCharsetNoneText)
        {
            mView->setCharset(HexWidget::Charset::None);
        }
    }
}

void MemWidget::selectArea()
{
    QMenu menu;
    menu.addAction(mAreaMemZ80Text);
    menu.addAction(mAreaMemADLText);
    menu.addAction(mAreaRamText);
    menu.addAction(mAreaFlashText);
    menu.addAction(mAreaPortsText);

    QAction *action = menu.exec(mBtnArea->mapToGlobal(QPoint{0, mBtnArea->height() + 1}));
    if (action)
    {
        mBtnArea->setText(action->text());
        if (action->text() == mAreaMemZ80Text)
        {
            setArea(Area::MemZ80);
        }
        else if (action->text() == mAreaMemADLText)
        {
            setArea(Area::MemADL);
        }
        else if (action->text() == mAreaRamText)
        {
            setArea(Area::Ram);
        }
        else if (action->text() == mAreaFlashText)
        {
            setArea(Area::Flash);
        }
        else if (action->text() == mAreaPortsText)
        {
            setArea(Area::Port);
        }
    }
}

void MemWidget::selectSearchType()
{
    QMenu menu;
    menu.addAction(mSearchHexText);
    menu.addAction(mSearchAsciiText);

    QAction *action = menu.exec(mBtnSearchType->mapToGlobal(QPoint{0, mBtnSearchType->height() + 1}));
    if (action)
    {
        mBtnSearchType->setText(action->text());
    }
}

void MemWidget::showSearchGroup()
{
    mGrpSearch->setVisible(!mGrpSearch->isVisible());
    mBtnSearch->setChecked(mGrpSearch->isVisible());
}
