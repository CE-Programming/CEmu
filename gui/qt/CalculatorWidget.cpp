/*
  This file is part of KDDockWidgets.

  SPDX-FileCopyrightText: 2019-2020 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  Author: Sérgio Martins <sergio.martins@kdab.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only

  Contact KDAB at <info@kdab.com> for commercial licensing options.
*/

#include "CalculatorWidget.h"
#include "ScreenWidget.h"

#include <QPainter>
#include <QDebug>
#include <QFile>
#include <QLineEdit>

CalculatorWidget::CalculatorWidget(QWidget *parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
}

CalculatorWidget::~CalculatorWidget()
{
}

