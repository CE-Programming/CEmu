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

#ifndef UTIL_H
#define UTIL_H

#include <QtCore/QString>
#include <QtCore/QtGlobal>
#include <QtGui/QFont>

namespace Util
{

extern const int addrByteWidth;
extern const int portByteWidth;

extern const QString error;
extern const QString warning;
extern const QString information;

extern const QString stateExtension;
extern const QString portablePath;

extern bool isHexAddress(const QString &str);
extern bool isHexPort(const QString &str);
extern bool isHexString(const QString &str, int min = 0, int max = INT_MAX);
extern bool isDecString(const QString &str, int min = 0, int max = INT_MAX);
extern qulonglong hex2int(const QString &str);
extern QString int2hex(qulonglong a, quint8 l);
extern QString randomString(const int length);
extern QFont monospaceFont();

}

#endif
