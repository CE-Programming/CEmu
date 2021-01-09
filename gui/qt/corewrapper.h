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

#include <QtCore/QMetaType>
#include <QtCore/QObject>

namespace cemucore
{
    Q_NAMESPACE
#include <cemucore.h>
    using signal_t       = cemucore_signal_t;
    Q_ENUM_NS(signal_t)
    using create_flags_t = cemucore_create_flags_t;
    Q_FLAG_NS(create_flags_t)
    using prop_t         = cemucore_prop_t;
    Q_ENUM_NS(prop_t)
    using reg_t          = cemucore_reg_t;
    Q_ENUM_NS(reg_t)
    using dbg_flags_t    = cemucore_dbg_flags_t;
    Q_FLAG_NS(dbg_flags_t)
}

class CoreWrapper : public QObject
{
    Q_OBJECT

public:
    CoreWrapper(QObject *parent = nullptr);
    ~CoreWrapper();

    cemucore::cemucore_t *core();
    const cemucore::cemucore_t *core() const;

    int32_t get(cemucore::prop_t prop, int32_t addr) const;
    void set(cemucore::prop_t prop, int32_t addr, int32_t val);

    void sleep();
    void wake();

signals:
    void lcdFrame();
    void softCmd();

private:
    static void signalHandler(cemucore::signal_t, void *);

    cemucore::cemucore_t *mCore;
};

#endif
