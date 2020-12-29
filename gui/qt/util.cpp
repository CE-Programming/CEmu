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

#include "util.h"

#include <QtCore/QRandomGenerator>

int Util::hex2int(const QString &str)
{
    return static_cast<int>(strtol(str.toStdString().c_str(), nullptr, 16));
}

QString Util::int2hex(uint32_t a, uint8_t l)
{
    return QString::number(a, 16).rightJustified(l, '0', true).toUpper();
}

QString Util::randomString(const int length)
{
   const QString possibleCharacters(QStringLiteral("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"));

   QString randomString;
   for(int i = 0; i < length; ++i)
   {
       int index = QRandomGenerator::global()->generate() % possibleCharacters.length();
       QChar nextChar = possibleCharacters.at(index);
       randomString.append(nextChar);
   }

   return randomString;
}
