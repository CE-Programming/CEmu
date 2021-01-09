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

#include "controlwidget.h"

#include "../util.h"

#include <kddockwidgets/DockWidget.h>

#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QComboBox>

ControlWidget::ControlWidget(CoreWindow *coreWindow)
    : DockedWidget{new KDDockWidgets::DockWidget{QStringLiteral("Control")},
                   QIcon(QStringLiteral(":/assets/icons/services.svg")),
                   coreWindow},
      mInDebug{false}
{
    int maxMonoWidth = QFontMetrics(Util::monospaceFont()).maxWidth();

    mBtnRun = new QPushButton(QIcon(QStringLiteral(":/assets/icons/run.svg")), tr("Stop"));
    mBtnStep = new QPushButton(QIcon(QStringLiteral(":/assets/icons/right.svg")), tr("Step"));
    mBtnOver = new QPushButton(QIcon(QStringLiteral(":/assets/icons/down_right.svg")), tr("Over"));
    mBtnNext = new QPushButton(QIcon(QStringLiteral(":/assets/icons/down.svg")), tr("Next"));
    mBtnOut = new QPushButton(QIcon(QStringLiteral(":/assets/icons/right_up2.svg")), tr("Out"));
    mCmbMode = new QComboBox;
    mCmbMode->addItems({ tr("ASM"), tr("C"), tr("Disable") });

    mBtnRun->setFont(Util::monospaceFont());
    mBtnStep->setFont(Util::monospaceFont());
    mBtnOver->setFont(Util::monospaceFont());
    mBtnNext->setFont(Util::monospaceFont());
    mBtnOut->setFont(Util::monospaceFont());
    mCmbMode->setFont(Util::monospaceFont());

    mBtnRun->setMinimumWidth(mBtnRun->iconSize().width() + maxMonoWidth * (mBtnRun->text().length() + 2));
    mBtnStep->setMinimumWidth(mBtnStep->iconSize().width() + maxMonoWidth * (mBtnStep->text().length() + 2));
    mBtnOver->setMinimumWidth(mBtnOver->iconSize().width() + maxMonoWidth * (mBtnOver->text().length() + 2));
    mBtnNext->setMinimumWidth(mBtnNext->iconSize().width() + maxMonoWidth * (mBtnNext->text().length() + 2));
    mBtnOut->setMinimumWidth(mBtnOut->iconSize().width() + maxMonoWidth * (mBtnOut->text().length() + 2));

    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->setSizeConstraint(QLayout::SetMaximumSize);
    hLayout->addStretch();
    hLayout->addWidget(mBtnRun);
    hLayout->addWidget(mBtnStep);
    hLayout->addWidget(mBtnOver);
    hLayout->addWidget(mBtnNext);
    hLayout->addWidget(mBtnOut);
    hLayout->addWidget(mCmbMode);
    hLayout->addStretch();

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->setSizeConstraint(QLayout::SetMaximumSize);
    vLayout->addStretch();
    vLayout->addLayout(hLayout);
    vLayout->addStretch();
    setLayout(vLayout);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    connect(mBtnRun, &QPushButton::clicked, [this]
    {
        if (mInDebug)
        {
            emit run();
        }
        else
        {
            emit stop();
        }
    });

    enableDebugWidgets(false);
}

void ControlWidget::enableDebugWidgets(bool enabled)
{
    mInDebug = enabled;

    mBtnStep->setEnabled(mInDebug);
    mBtnOver->setEnabled(mInDebug);
    mBtnNext->setEnabled(mInDebug);
    mBtnOut->setEnabled(mInDebug);

    if (mInDebug)
    {
        mBtnRun->setText(tr("Run"));
        mBtnRun->setIcon(QIcon(QStringLiteral(":/assets/icons/run.svg")));
    }
    else
    {
        mBtnRun->setText(tr("Stop"));
        mBtnRun->setIcon(QIcon(QStringLiteral(":/assets/icons/stop.svg")));
    }
}

