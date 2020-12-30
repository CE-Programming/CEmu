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

#include "memorywidget.h"
#include "widgets/hexwidget.h"
#include "util.h"

#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QSpinBox>

MemoryWidgetList::MemoryWidgetList()
    : mPrev{this},
      mNext{this}
{
}

MemoryWidgetList::MemoryWidgetList(MemoryWidgetList *list)
    : mPrev{qExchange(list->mPrev, this)},
      mNext{list}
{
    mPrev->mNext = this;
}

MemoryWidgetList::~MemoryWidgetList()
{
    mPrev->mNext = mNext;
    mNext->mPrev = mPrev;
}

MemoryWidget::MemoryWidget(QWidget *parent)
    : QWidget{parent}
{
    init();
}

MemoryWidget::MemoryWidget(MemoryWidgetList *list, QWidget *parent)
    : QWidget{parent},
      MemoryWidgetList{list}
{
    init();
}

void MemoryWidget::init()
{
    QLineEdit *editAddr = new QLineEdit;
    editAddr->setFont(Util::monospaceFont());

    mView = new HexWidget;

    QPushButton *btnGoto = new QPushButton(tr("Goto"));
    QPushButton *btnSearch = new QPushButton(tr("Search"));
    QPushButton *btnApply = new QPushButton(tr("Apply Changes"));
    QPushButton *btnAscii = new QPushButton(tr("ASCII"));
    QSpinBox *spnWidth = new QSpinBox;

    btnAscii->setCheckable(true);
    btnAscii->setChecked(true);

    spnWidth->setMinimum(1);
    spnWidth->setMaximum(1024);

    QHBoxLayout *hboxBtns = new QHBoxLayout;
    hboxBtns->addWidget(editAddr);
    hboxBtns->addWidget(btnGoto);
    hboxBtns->addWidget(btnSearch);
    hboxBtns->addStretch();
    hboxBtns->addWidget(spnWidth);
    hboxBtns->addWidget(btnAscii);
    hboxBtns->addWidget(btnApply);

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addLayout(hboxBtns);
    vLayout->addWidget(mView);
    setLayout(vLayout);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}
