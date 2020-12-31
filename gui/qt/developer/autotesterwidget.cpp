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

#include "autotesterwidget.h"

#include "../util.h"

#include <kddockwidgets/DockWidget.h>

#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSizePolicy>

AutotesterWidget::AutotesterWidget(DockedWidgetList &list)
    : DockedWidget{new KDDockWidgets::DockWidget{QStringLiteral("Autotester")}, list}
{
    QGroupBox *grpTest = new QGroupBox(tr("Launch Test"));
    QGroupBox *grpCfg = new QGroupBox(tr("Test Configuration"));
    QGroupBox *grpCrc = new QGroupBox(tr("Calculate CRC"));
    QGroupBox *grpHash = new QGroupBox(tr("Result"));

    QCheckBox *chkReset = new QCheckBox(tr("Reset before test"));
    QCheckBox *chkClear = new QCheckBox(tr("Press [clear] before test"));

    QPushButton *btnBrowse = new QPushButton(tr("Browse..."));
    QPushButton *btnLaunch = new QPushButton(tr("Launch test"));
    QPushButton *btnLoad = new QPushButton(tr("(Re)Load"));
    QPushButton *btnRefresh = new QPushButton(tr("Refresh"));

    QLabel *lblJson = new QLabel(QStringLiteral("JSON ") + tr("file:"));
    QLabel *lblStart = new QLabel(tr("Start Address:"));
    QLabel *lblSize = new QLabel(tr("Size:"));
    QLabel *lblCrc = new QLabel(QStringLiteral("CRC: "));
    QLabel *lblPreset = new QLabel(tr("... or choose a preset:"));

    QLineEdit *edtJson = new QLineEdit;
    QLineEdit *edtCrc = new QLineEdit;
    QLineEdit *edtStart = new QLineEdit;
    QLineEdit *edtSize = new QLineEdit;

    edtCrc->setReadOnly(true);
    edtCrc->setFont(Util::monospaceFont());
    edtStart->setFont(Util::monospaceFont());
    edtSize->setFont(Util::monospaceFont());

    QComboBox *cmbPreset = new QComboBox;
    cmbPreset->addItems(
    {
        "(no preset)",
        "All VRAM (16bpp)",
        "1st half of VRAM (8bpp)",
        "2nd half of VRAM (8bpp)",
        "textShadow",
        "cmdShadow",
        "pixelShadow",
        "pixelShadow2",
        "cmdPixelShadow",
        "plotSScreen",
        "saveSScreen",
        "lcdPalette",
        "cursorImage",
        "All the RAM"
    });

    QHBoxLayout *hboxJson = new QHBoxLayout;
    hboxJson->addWidget(lblJson);
    hboxJson->addWidget(edtJson);
    hboxJson->addWidget(btnBrowse);
    hboxJson->addWidget(btnLoad);
    hboxJson->addWidget(btnLaunch);

    QHBoxLayout *hboxCfg = new QHBoxLayout;
    hboxCfg->addWidget(chkReset);
    hboxCfg->addWidget(chkClear);
    grpCfg->setLayout(hboxCfg);

    QVBoxLayout *vboxTest = new QVBoxLayout;
    vboxTest->addLayout(hboxJson);
    vboxTest->addWidget(grpCfg);
    grpTest->setLayout(vboxTest);


    QHBoxLayout *hboxCrc = new QHBoxLayout;
    hboxCrc->addWidget(lblStart);
    hboxCrc->addWidget(edtStart);
    hboxCrc->addWidget(lblSize);
    hboxCrc->addWidget(edtSize);

    QHBoxLayout *hboxPreset = new QHBoxLayout;
    hboxPreset->addWidget(lblPreset);
    hboxPreset->addStretch(1);
    hboxPreset->addWidget(cmbPreset);

    QHBoxLayout *hboxHash = new QHBoxLayout;
    hboxHash->addWidget(lblCrc);
    hboxHash->addWidget(edtCrc);
    hboxHash->addWidget(btnRefresh);
    grpHash->setLayout(hboxHash);

    QVBoxLayout *vboxCrc = new QVBoxLayout;
    vboxCrc->addLayout(hboxCrc);
    vboxCrc->addLayout(hboxPreset);
    vboxCrc->addWidget(grpHash);
    grpCrc->setLayout(vboxCrc);

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addStretch(1);
    vLayout->addWidget(grpTest);
    vLayout->addWidget(grpCrc);
    vLayout->addStretch(1);
    setLayout(vLayout);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}
