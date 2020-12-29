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

#include "clockswidget.h"

#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QLabel>

ClocksWidget::ClocksWidget(QWidget *parent)
    : QWidget{parent}
{
#ifdef Q_OS_WIN
    QFont monospaceFont(QStringLiteral("Courier"), 10);
#else
    QFont monospaceFont(QStringLiteral("Monospace"), 10);
#endif

    QGroupBox *grpGpt = new QGroupBox(tr("General Purpose Timers"));
    QGroupBox *grpRtc = new QGroupBox(tr("Real Time Clock"));

    QLabel *lblValue = new QLabel(tr("Value"));
    QLabel *lblReloadValue = new QLabel(tr("Reload Value"));
    QLabel *lblTimer1 = new QLabel(tr("Timer 1"));
    QLabel *lblTimer2 = new QLabel(tr("Timer 2"));
    QLabel *lblTimer3 = new QLabel(tr("Timer 3"));
    QLabel *lblDays = new QLabel(tr("Days"));
    QLabel *lblHrs = new QLabel(tr("Hours"));
    QLabel *lblMin = new QLabel(tr("Minutes"));
    QLabel *lblSec = new QLabel(tr("Seconds"));

    QLineEdit *edtTimer1V = new QLineEdit(QStringLiteral("0"));
    QLineEdit *edtTimer2V = new QLineEdit(QStringLiteral("0"));
    QLineEdit *edtTimer3V = new QLineEdit(QStringLiteral("0"));
    QLineEdit *edtTimer1RV = new QLineEdit(QStringLiteral("0"));
    QLineEdit *edtTimer2RV = new QLineEdit(QStringLiteral("0"));
    QLineEdit *edtTimer3RV = new QLineEdit(QStringLiteral("0"));

    QLineEdit *edtDays = new QLineEdit(QStringLiteral("0"));
    QLineEdit *edtHrs = new QLineEdit(QStringLiteral("0"));
    QLineEdit *edtMin = new QLineEdit(QStringLiteral("0"));
    QLineEdit *edtSec = new QLineEdit(QStringLiteral("0"));

    lblValue->setFont(monospaceFont);
    lblReloadValue->setFont(monospaceFont);
    lblTimer1->setFont(monospaceFont);
    lblTimer2->setFont(monospaceFont);
    lblTimer3->setFont(monospaceFont);
    lblDays->setFont(monospaceFont);
    lblHrs->setFont(monospaceFont);
    lblMin->setFont(monospaceFont);
    lblSec->setFont(monospaceFont);

    edtDays->setFont(monospaceFont);
    edtHrs->setFont(monospaceFont);
    edtMin->setFont(monospaceFont);
    edtSec->setFont(monospaceFont);

    QHBoxLayout *hboxRtc = new QHBoxLayout;
    hboxRtc->addWidget(lblDays);
    hboxRtc->addWidget(edtDays);
    hboxRtc->addWidget(lblHrs);
    hboxRtc->addWidget(edtHrs);
    hboxRtc->addWidget(lblMin);
    hboxRtc->addWidget(edtMin);
    hboxRtc->addWidget(lblSec);
    hboxRtc->addWidget(edtSec);
    grpRtc->setLayout(hboxRtc);

    lblTimer1->setAlignment(Qt::AlignRight);
    lblTimer2->setAlignment(Qt::AlignRight);
    lblTimer3->setAlignment(Qt::AlignRight);
    lblValue->setAlignment(Qt::AlignCenter);
    lblReloadValue->setAlignment(Qt::AlignCenter);

    QGridLayout *gridGpt = new QGridLayout;
    gridGpt->addWidget(lblValue, 0, 1);
    gridGpt->addWidget(lblReloadValue, 0, 2);
    gridGpt->addWidget(lblTimer1, 1, 0);
    gridGpt->addWidget(lblTimer2, 2, 0);
    gridGpt->addWidget(lblTimer3, 3, 0);
    gridGpt->addWidget(edtTimer1V, 1, 1);
    gridGpt->addWidget(edtTimer2V, 2, 1);
    gridGpt->addWidget(edtTimer3V, 3, 1);
    gridGpt->addWidget(edtTimer1RV, 1, 2);
    gridGpt->addWidget(edtTimer2RV, 2, 2);
    gridGpt->addWidget(edtTimer3RV, 3, 2);
    grpGpt->setLayout(gridGpt);

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addStretch(1);
    vLayout->addWidget(grpGpt);
    vLayout->addWidget(grpRtc);
    vLayout->addStretch(1);

    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->addStretch(1);
    hLayout->addLayout(vLayout);
    hLayout->addStretch(1);
    setLayout(hLayout);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}
