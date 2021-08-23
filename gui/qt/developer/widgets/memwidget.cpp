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

#include "memwidget.h"

#include "../../corewrapper.h"
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

MemWidget::MemWidget(DockedWidget *parent, Area area)
    : mDockedWidget{parent},
      mCharsetNoneText{tr("None")},
      mCharsetTiAsciiText{QStringLiteral("TI ASCII")},
      mCharsetAsciiText{QStringLiteral("ASCII")},
      mSearchHex{true}
{
    mEdtAddr = new QLineEdit;
    mEdtAddr->setFont(Util::monospaceFont());

    cemucore::prop prop;
    qint32 len;
    switch (area)
    {
        case Area::Mem:
            prop = cemucore::CEMUCORE_PROP_MEMORY;
            len = INT32_C(1) << 24;
            break;
        case Area::Flash:
            prop = cemucore::CEMUCORE_PROP_FLASH;
            len = INT32_C(1) << 22;
            break;
        case Area::Ram:
            prop = cemucore::CEMUCORE_PROP_RAM;
            len = INT32_C(0x65800);
            break;
        case Area::Port:
            prop = cemucore::CEMUCORE_PROP_PORT;
            len = INT32_C(1) << 16;
            break;
    }
    mView = new HexWidget{this, prop, len};

    QLabel *lblNumBytes = new QLabel(tr("Bytes per row") + ':');
    QSpinBox *spnNumBytes = new QSpinBox;
    QLabel *lblByteOff = new QLabel(tr("Byte offset") + ':');
    QSpinBox *spnByteOff = new QSpinBox;
    QPushButton *btnGoto = new QPushButton(QIcon(QStringLiteral(":/assets/icons/ok.svg")), tr("Goto"));
    QPushButton *btnSearch = new QPushButton(QIcon(QStringLiteral(":/assets/icons/search.svg")), tr("Search"));
    mBtnCharset = new QPushButton(QIcon(QStringLiteral(":/assets/icons/alphabetical_az.svg")), QString());

    QHBoxLayout *hboxBtns = new QHBoxLayout;
    hboxBtns->addWidget(mEdtAddr);
    hboxBtns->addWidget(btnGoto);
    hboxBtns->addWidget(btnSearch);

    QHBoxLayout *hboxBtmBtns = new QHBoxLayout;
    hboxBtmBtns->addWidget(lblNumBytes);
    hboxBtmBtns->addWidget(spnNumBytes);
    hboxBtmBtns->addWidget(lblByteOff);
    hboxBtmBtns->addWidget(spnByteOff);
    hboxBtmBtns->addStretch();
    hboxBtmBtns->addWidget(mBtnCharset);

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addLayout(hboxBtns);
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

    connect(btnSearch, &QPushButton::clicked, this, &MemWidget::showSearchDialog);
    connect(spnNumBytes, QOverload<int>::of(&QSpinBox::valueChanged), [this, spnByteOff](int value)
    {
        mView->setBytesPerLine(value);
        spnByteOff->setMaximum(value - 1);
    });
    connect(spnByteOff, QOverload<int>::of(&QSpinBox::valueChanged), mView, &HexWidget::setByteOff);
    connect(mBtnCharset, &QPushButton::clicked, this, &MemWidget::selectCharset);

    connect(btnGoto, &QPushButton::clicked, [this]
    {
        mView->gotoAddr(Util::hex2int(mEdtAddr->text()));
    });
    connect(mEdtAddr, &QLineEdit::returnPressed, [this]
    {
        mView->gotoAddr(Util::hex2int(mEdtAddr->text()));
    });
}

DockedWidget *MemWidget::dockedWidget() const
{
    return mDockedWidget;
}

void MemWidget::selectCharset()
{
    QMenu menu;
    menu.addAction(mCharsetAsciiText);
    menu.addAction(mCharsetTiAsciiText);
    menu.addAction(mCharsetNoneText);

    QAction *action = menu.exec(mBtnCharset->mapToGlobal({0, mBtnCharset->height() + 1}));
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

void MemWidget::showSearchDialog()
{
    QDialog dialog;
    int ret;

    enum Target
    {
        Next = 1,
        NextNot,
        Prev,
        PrevNot
    };

    QLabel *lblFind = new QLabel(tr("Find") + ':');
    QLineEdit *edtSearch = new QLineEdit(mSearch);
    QPushButton *btnNext = new QPushButton(tr("Next"));
    QPushButton *btnNextNot = new QPushButton(tr("Next Not"));
    QPushButton *btnPrev = new QPushButton(tr("Prev"));
    QPushButton *btnPrevNot = new QPushButton(tr("Prev Not"));
    QPushButton *btnCancel = new QPushButton(QApplication::style()->standardIcon(QStyle::SP_DialogCancelButton), tr("Cancel"));

    QRadioButton *chkHex = new QRadioButton(tr("Hexadecimal"));
    QRadioButton *chkAscii = new QRadioButton(QStringLiteral("ASCII"));

    QGroupBox *grpOptions = new QGroupBox(tr("Search options"));

    chkHex->setChecked(mSearchHex);
    chkAscii->setChecked(!mSearchHex);

    QGridLayout *gLayout = new QGridLayout;
    gLayout->addWidget(btnNext, 0, 0);
    gLayout->addWidget(btnNextNot, 0, 1);
    gLayout->addWidget(btnPrev, 1, 0);
    gLayout->addWidget(btnPrevNot, 1, 1);
    gLayout->addWidget(btnCancel, 2, 1);

    QHBoxLayout *hFindLayout = new QHBoxLayout;
    hFindLayout->addWidget(lblFind);
    hFindLayout->addWidget(edtSearch);

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addWidget(chkHex);
    vLayout->addWidget(chkAscii);
    grpOptions->setLayout(vLayout);

    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->addWidget(grpOptions);
    hLayout->addLayout(gLayout);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(hFindLayout);
    mainLayout->addLayout(hLayout);

    dialog.setLayout(mainLayout);
    dialog.setWindowTitle(tr("Memory search"));
    dialog.setWindowIcon(QIcon(QStringLiteral(":/assets/icons/search.svg")));

    connect(btnCancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(btnNext, &QPushButton::clicked, [&dialog]{ dialog.done(Target::Next); });
    connect(btnNextNot, &QPushButton::clicked, [&dialog]{ dialog.done(Target::NextNot); });
    connect(btnPrev, &QPushButton::clicked, [&dialog]{ dialog.done(Target::Prev); });
    connect(btnPrevNot, &QPushButton::clicked, [&dialog]{ dialog.done(Target::PrevNot); });

    if ((ret = dialog.exec()))
    {
        mSearchHex = chkHex->isChecked();
        mSearch = edtSearch->text();

        switch (ret)
        {
            default:
            case Target::Next:
            case Target::NextNot:
            case Target::Prev:
            case Target::PrevNot:
                break;
        }
    }
}
