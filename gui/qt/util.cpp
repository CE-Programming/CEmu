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
#include <QtCore/QRegularExpression>
#include <QtGui/QFontDatabase>
#include <QtGui/QFontInfo>
#include <QtGui/QValidator>
#include <QtWidgets/QApplication>

const int Util::addrByteWidth = 6;
const int Util::portByteWidth = 4;

const QString Util::error = QObject::tr("Error");
const QString Util::warning = QObject::tr("Warning");
const QString Util::information = QObject::tr("Information");

qulonglong Util::hex2int(const QString &str)
{
    return static_cast<qulonglong>(strtoull(str.toStdString().c_str(), nullptr, 16));
}

QString Util::int2hex(qulonglong a, quint8 l)
{
    return QString::number(a, 16).rightJustified(l, '0', true).toUpper();
}

bool Util::isHexAddress(const QString &str)
{
    return isHexString(str, 0, 16777215);
}

bool Util::isHexPort(const QString &str)
{
    return isHexString(str, 0, 65535);
}

bool Util::isHexString(const QString &str, int min, int max)
{
    QRegularExpression matcher(QString("^[0-9A-F]"), QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = matcher.match(str);

    if (match.hasMatch())
    {
        int value = hex2int(str);
        if (value <= max && value >= min)
        {
            return true;
        }
    }

    return false;
}

bool Util::isDecString(const QString &str, int min, int max)
{
    QRegularExpression matcher(QString("^[0-9]"), QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = matcher.match(str);

    if (match.hasMatch())
    {
        int value = hex2int(str);
        if (value <= max && value >= min)
        {
            return true;
        }
    }

    return false;
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

QFont Util::monospaceFont()
{
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    if (QFontInfo(font).fixedPitch())
    {
        return font;
    }

    font.setFamily("monospace");
    if (QFontInfo(font).fixedPitch())
    {
        return font;
    }

    font.setStyleHint(QFont::Monospace);
    if (QFontInfo(font).fixedPitch())
    {
        return font;
    }

    font.setStyleHint(QFont::TypeWriter);
    if (QFontInfo(font).fixedPitch())
    {
        return font;
    }

    font.setFamily("courier");
    if (QFontInfo(font).fixedPitch())
    {
        return font;
    }

    return font;
}
