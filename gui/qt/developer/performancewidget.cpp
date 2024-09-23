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

#include "performancewidget.h"

#include "../util.h"

#include <kddockwidgets/DockWidget.h>

#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QSpinBox>

PerformanceWidget::PerformanceWidget(CoreWindow *coreWindow)
    : DockedWidget{new KDDockWidgets::DockWidget{QStringLiteral("Cycle Counter")},
                   QIcon(QStringLiteral(":/assets/icons/statistics.svg")),
                   coreWindow}
{
    QGroupBox *grpCycles = new QGroupBox(tr("Cycle Counter"));
    QGroupBox *grpState = new QGroupBox(tr("CPU State"));

    QSpinBox *spnMHz = new QSpinBox;

    spnMHz->setMinimum(0);
    spnMHz->setMaximum(256);
    spnMHz->setSuffix(QStringLiteral(" MHz"));

    QLabel *lblFreq = new QLabel(tr("Frequency: "));

    QLineEdit *edtCycles = new QLineEdit(QStringLiteral("0"));
    QPushButton *btnZeroCycles = new QPushButton(tr("Zero"));
    QCheckBox *chkIncludeDma = new QCheckBox(tr("Include DMA"));

    edtCycles->setFont(Util::monospaceFont());
    spnMHz->setFont(Util::monospaceFont());

    QHBoxLayout *hboxCycles = new QHBoxLayout;
    hboxCycles->addWidget(edtCycles);
    hboxCycles->addWidget(btnZeroCycles);
    hboxCycles->addStretch();
    hboxCycles->addWidget(chkIncludeDma);
    grpCycles->setLayout(hboxCycles);

    QHBoxLayout *hboxState = new QHBoxLayout;
    hboxState->addWidget(lblFreq);
    hboxState->addWidget(spnMHz);
    hboxState->addStretch();
    grpState->setLayout(hboxState);

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addStretch();
    vLayout->addWidget(grpCycles);
    vLayout->addWidget(grpState);
    vLayout->addStretch();

    QVBoxLayout *hLayout = new QVBoxLayout;
    hLayout->addStretch();
    hLayout->addLayout(vLayout);
    hLayout->addStretch();
    setLayout(hLayout);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    connect(btnZeroCycles, &QPushButton::clicked, [edtCycles]{ edtCycles->setText(QStringLiteral("0")); });

    enableDebugWidgets(false);
}

void PerformanceWidget::enableDebugWidgets(bool enbaled)
{
    setEnabled(enbaled);
}

void PerformanceWidget::loadFromCore()
{
}

void PerformanceWidget::storeToCore() const
{
}
