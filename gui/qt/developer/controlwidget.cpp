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

#include "controlwidget.h"

#include "../util.h"

#include <kddockwidgets/DockWidget.h>

#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QComboBox>
#include <QShortcut>

ControlWidget::ControlWidget(CoreWindow *coreWindow)
    : DockedWidget{new KDDockWidgets::QtWidgets::DockWidget{QStringLiteral("Control")},
                   QIcon(QStringLiteral(":/assets/icons/services.svg")),
                   coreWindow}
{
    int maxMonoWidth = QFontMetrics(Util::monospaceFont()).maxWidth();

    mBtnRun = new QPushButton(QIcon(QStringLiteral(":/assets/icons/run.svg")), tr("Stop"));
    mBtnStep = new QPushButton(QIcon(QStringLiteral(":/assets/icons/right.svg")), tr("Step"));
    mBtnOver = new QPushButton(QIcon(QStringLiteral(":/assets/icons/down_right.svg")), tr("Over"));
    mBtnNext = new QPushButton(QIcon(QStringLiteral(":/assets/icons/down.svg")), tr("Next"));
    mBtnOut = new QPushButton(QIcon(QStringLiteral(":/assets/icons/right_up2.svg")), tr("Out"));

    mBtnRun->setFont(Util::monospaceFont());
    mBtnStep->setFont(Util::monospaceFont());
    mBtnOver->setFont(Util::monospaceFont());
    mBtnNext->setFont(Util::monospaceFont());
    mBtnOut->setFont(Util::monospaceFont());

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
    hLayout->addStretch();

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->setSizeConstraint(QLayout::SetMaximumSize);
    vLayout->addStretch();
    vLayout->addLayout(hLayout);
    vLayout->addStretch();
    setLayout(vLayout);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QShortcut *shortcutRun = new QShortcut(QKeySequence(Qt::Key_F10), this);
    QShortcut *shortcutStepIn = new QShortcut(QKeySequence(Qt::Key_F6), this);
    QShortcut *shortcutStepOver = new QShortcut(QKeySequence(Qt::Key_F7), this);
    QShortcut *shortcutStepNext = new QShortcut(QKeySequence(Qt::Key_F8), this);
    QShortcut *shortcutStepOut = new QShortcut(QKeySequence(Qt::Key_F9), this);

    shortcutRun->setAutoRepeat(false);

    connect(shortcutRun, &QShortcut::activated, this, &ControlWidget::toggleDebug);
    connect(shortcutStepIn, &QShortcut::activated, this, &ControlWidget::stepIn);
    connect(shortcutStepOver, &QShortcut::activated, this, &ControlWidget::stepOver);
    connect(shortcutStepNext, &QShortcut::activated, this, &ControlWidget::stepNext);
    connect(shortcutStepOut, &QShortcut::activated, this, &ControlWidget::stepOut);

    connect(mBtnRun, &QPushButton::clicked, this, &ControlWidget::toggleDebug);
    connect(mBtnStep, &QPushButton::clicked, this, &ControlWidget::stepIn);
    connect(mBtnOver, &QPushButton::clicked, this, &ControlWidget::stepOver);
    connect(mBtnNext, &QPushButton::clicked, this, &ControlWidget::stepNext);
    connect(mBtnOut, &QPushButton::clicked, this, &ControlWidget::stepOut);

    enableDebugWidgets(false);
}

void ControlWidget::enableDebugWidgets(bool enabled)
{
    mBtnStep->setEnabled(enabled);
    mBtnOver->setEnabled(enabled);
    mBtnNext->setEnabled(enabled);
    mBtnOut->setEnabled(enabled);

    if (enabled)
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

