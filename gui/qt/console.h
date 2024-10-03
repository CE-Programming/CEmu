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

#ifndef CONSOLE_H
#define CONSOLE_H

#include <QtCore/QObject>
#include <QtCore/QTextStream>
#include <QtCore/QString>

QT_BEGIN_NAMESPACE
class QLocalSocket;
class QString;
class QThread;
QT_END_NAMESPACE

class Console : public QObject
{
    Q_OBJECT

public:
    Console(QObject *parent = nullptr);
    ~Console() override;

public slots:
    void outputText(const QString &text);
    void errorText(const QString &text);
};

#endif
