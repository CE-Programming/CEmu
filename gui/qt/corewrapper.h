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

#ifndef COREWRAPPER_H
#define COREWRAPPER_H

#include <QtCore/QByteArray>
#include <QtCore/QMetaType>
#include <QtCore/QObject>

namespace cemucore
{
    Q_NAMESPACE
#include <cemucore.h>
    using sig          = cemucore_sig;
    Q_ENUM_NS(sig)
    using create_flags = cemucore_create_flags;
    Q_FLAG_NS(create_flags)
    using prop         = cemucore_prop;
    Q_ENUM_NS(prop)
    using dev          = cemucore_dev;
    Q_ENUM_NS(dev)
    using reg          = cemucore_reg;
    Q_ENUM_NS(reg)
    using dbg_flags    = cemucore_dbg_flags;
    Q_FLAG_NS(dbg_flags)
}

class CoreWrapper : public QObject
{
    Q_OBJECT

    struct ScopedLock
    {
        cemucore::cemucore *mCore;
        ~ScopedLock();
    };

public:
    CoreWrapper(QObject *parent = nullptr);
    ~CoreWrapper();

    cemucore::cemucore *core();
    const cemucore::cemucore *core() const;

    bool sleep() const;
    void wake() const;
    ScopedLock lock() const;

    qint32 get(cemucore::prop prop, qint32 addr) const;
    QByteArray get(cemucore::prop prop, qint32 addr, qint32 len) const;
    void set(cemucore::prop prop, qint32 addr, qint32 val);
    void set(cemucore::prop prop, qint32 addr, const QByteArray &data);

signals:
    void devChanged(cemucore::dev);
    void lcdFrame();
    void softCmd();

private:
    void signalHandler(cemucore::sig);

    mutable cemucore::cemucore *mCore;
};

#endif
