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

#ifndef EMUTHREAD_H
#define EMUTHREAD_H

#include <QtCore/QMutex>
#include <QtCore/QQueue>
#include <QtCore/QSemaphore>
#include <QtCore/QThread>
#include <QtCore/QTimer>

class EmuThread : public QThread
{
    Q_OBJECT

public:
    explicit EmuThread(QObject *parent = nullptr);

signals:

public slots:

protected:

private:

};

#endif
