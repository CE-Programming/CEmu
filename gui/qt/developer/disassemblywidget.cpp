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

#include "disassemblywidget.h"

#include "../util.h"
#include "widgets/disassemblyview.h"

#include <kddockwidgets/DockWidget.h>

#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSizePolicy>

DisassemblyWidget::DisassemblyWidget(CoreWindow *coreWindow)
    : DockedWidget{new KDDockWidgets::DockWidget{QStringLiteral("Disassembly")},
                   QIcon(QStringLiteral(":/assets/icons/fine_print.svg")),
                   coreWindow}
{
    mDisasm = new DisassemblyView{this};

    QGroupBox *grpAddr = new QGroupBox(tr("Address"));
    QGroupBox *grpEquates = new QGroupBox(tr("Equates"));

    QLineEdit *editAddr = new QLineEdit;
    editAddr->setFont(Util::monospaceFont());

    QPushButton *btnGoto = new QPushButton(QIcon(QStringLiteral(":/assets/icons/ok.svg")), tr("Goto"));
    QPushButton *btnLoadEquates = new QPushButton(QIcon(QStringLiteral(":/assets/icons/opened_folder.svg")), tr("Load"));
    QPushButton *btnReloadEquates = new QPushButton(QIcon(QStringLiteral(":/assets/icons/process.svg")), tr("Reload"));
    QPushButton *btnRemoveEquates = new QPushButton(QIcon(QStringLiteral(":/assets/icons/cross.svg")), tr("Remove All"));

    QCheckBox *chkAdl = new QCheckBox(tr("ADL"));
    chkAdl->setTristate(true);

    QHBoxLayout *hboxEquates = new QHBoxLayout;
    hboxEquates->addWidget(btnLoadEquates);
    hboxEquates->addStretch();
    hboxEquates->addWidget(btnReloadEquates);
    hboxEquates->addWidget(btnRemoveEquates);
    grpEquates->setLayout(hboxEquates);

    QHBoxLayout *hboxAddr = new QHBoxLayout;
    hboxAddr->addWidget(editAddr);
    hboxAddr->addWidget(btnGoto);
    hboxAddr->addWidget(chkAdl);
    grpAddr->setLayout(hboxAddr);

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addWidget(grpAddr);
    vLayout->addWidget(mDisasm);
    vLayout->addWidget(grpEquates);
    setLayout(vLayout);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    enableDebugWidgets(false);
}

void DisassemblyWidget::enableDebugWidgets(bool enbaled)
{
    setEnabled(enbaled);
}
