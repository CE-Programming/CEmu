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

#include "corewrapper.h"

CoreWrapper::ScopedLock::~ScopedLock()
{
    void(cemucore::cemucore_wake(mCore));
}

CoreWrapper::CoreWrapper(QObject *parent)
    : QObject{parent},
      mCore{cemucore::cemucore_create(
              cemucore::CEMUCORE_CREATE_FLAG_THREADED,
              [](cemucore::sig sig, void *data)
              {
                  static_cast<CoreWrapper *>(data)->signalHandler(sig);
              }, this)}
{
    qRegisterMetaType<cemucore::sig>();
    qRegisterMetaType<cemucore::create_flags>();
    qRegisterMetaType<cemucore::prop>();
    qRegisterMetaType<cemucore::reg>();
    qRegisterMetaType<cemucore::dbg_flags>();
}

CoreWrapper::~CoreWrapper()
{
    cemucore::cemucore_destroy(qExchange(mCore, nullptr));
}

cemucore::cemucore *CoreWrapper::core()
{
    return mCore;
}

const cemucore::cemucore *CoreWrapper::core() const
{
    return mCore;
}

bool CoreWrapper::sleep() const
{
    return cemucore::cemucore_sleep(mCore);
}

bool CoreWrapper::wake() const
{
    return cemucore::cemucore_wake(mCore);
}

auto CoreWrapper::lock() const -> ScopedLock
{
    return {sleep() ? mCore : nullptr};
}

qint32 CoreWrapper::get(cemucore::prop prop, qint32 addr) const
{
    return cemucore::cemucore_get(mCore, prop, addr);
}

QByteArray CoreWrapper::get(cemucore::prop prop, qint32 addr, qint32 len) const
{
    // FIXME: implement in core, currently not atomic!
    QByteArray data;
    data.reserve(len);
    auto scope = lock();
    for (int i = 0; i < len; ++i)
    {
        data += char(get(prop, addr + i));
    }
    return data;
}

void CoreWrapper::set(cemucore::prop prop, qint32 addr, qint32 val)
{
    cemucore::cemucore_set(mCore, prop, addr, val);
}

void CoreWrapper::set(cemucore::prop prop, qint32 addr, const QByteArray &data)
{
    // FIXME: implement in core
    auto scope = lock();
    for (int i = 0; i < data.length(); ++i)
    {
        set(prop, addr + i, data[i]);
    }
}

void CoreWrapper::signalHandler(cemucore::sig sig)
{
    switch (sig)
    {
    case cemucore::CEMUCORE_SIG_DEV_CHANGED:
        emit devChanged(cemucore::dev(get(cemucore::CEMUCORE_PROP_DEV, 0)));
        break;
    case cemucore::CEMUCORE_SIG_LCD_FRAME:
        emit lcdFrame();
        break;
    case cemucore::CEMUCORE_SIG_SOFT_CMD:
        emit softCmd();
        break;
    }
}
