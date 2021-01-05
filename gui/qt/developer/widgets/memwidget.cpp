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
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QSpinBox>

MemWidget::MemWidget(QWidget *parent)
    : QWidget{parent},
      mSeachHex{true}
{
    QLineEdit *editAddr = new QLineEdit;
    editAddr->setFont(Util::monospaceFont());

    mView = new HexWidget;

    QLabel *lblNumBytes = new QLabel(tr("Bytes per row") + ':');
    QPushButton *btnGoto = new QPushButton(QIcon(QStringLiteral(":/assets/icons/ok.svg")), tr("Goto"));
    QPushButton *btnSearch = new QPushButton(QIcon(QStringLiteral(":/assets/icons/search.svg")), tr("Search"));
    QPushButton *btnApply = new QPushButton(QIcon(QStringLiteral(":/assets/icons/high_priority.svg")), tr("Apply Changes"));
    QPushButton *btnAscii = new QPushButton(QIcon(QStringLiteral(":/assets/icons/alphabetical_az.svg")), tr("ASCII"));
    QSpinBox *spnNumBytes = new QSpinBox;

    btnAscii->setCheckable(true);
    btnAscii->setChecked(true);

    spnNumBytes->setMinimum(1);
    spnNumBytes->setMaximum(1024);

    QHBoxLayout *hboxBtns = new QHBoxLayout;
    hboxBtns->addWidget(editAddr);
    hboxBtns->addWidget(btnGoto);
    hboxBtns->addWidget(btnSearch);
    hboxBtns->addStretch();
    hboxBtns->addWidget(btnApply);

    QHBoxLayout *hboxBtmBtns = new QHBoxLayout;
    hboxBtmBtns->addWidget(lblNumBytes);
    hboxBtmBtns->addWidget(spnNumBytes);
    hboxBtmBtns->addStretch();
    hboxBtmBtns->addWidget(btnAscii);

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addLayout(hboxBtns);
    vLayout->addWidget(mView);
    vLayout->addLayout(hboxBtmBtns);
    setLayout(vLayout);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    connect(btnSearch, &QPushButton::clicked, this, &MemWidget::showSeachDialog);
}

void MemWidget::showSeachDialog()
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
    QRadioButton *chkAscii = new QRadioButton(tr("ASCII"));

    QGroupBox *grpOptions = new QGroupBox(tr("Search options"));

    chkHex->setChecked(mSeachHex);
    chkAscii->setChecked(!mSeachHex);

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
        mSeachHex = chkHex->isChecked();
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
