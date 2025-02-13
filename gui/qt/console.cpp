/*
 * Copyright (c) 2015-2024 CE Programming.
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

#include "console.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QString>

Console::Console(QObject *parent)
    : QObject{parent}
{
}

Console::~Console()
{
}

void Console::outputText(const QString &text)
{
    static_cast<void>(text);
}

void Console::errorText(const QString &text)
{
    static_cast<void>(text);
}
