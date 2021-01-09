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

CoreWrapper::CoreWrapper(QObject *parent)
    : QObject{parent},
      mCore{cemucore::cemucore_create(cemucore::CEMUCORE_CREATE_FLAG_THREADED,
                                      &CoreWrapper::signalHandler, this)}
{
    qRegisterMetaType<cemucore::signal_t>();
    qRegisterMetaType<cemucore::create_flags_t>();
    qRegisterMetaType<cemucore::prop_t>();
    qRegisterMetaType<cemucore::reg_t>();
    qRegisterMetaType<cemucore::dbg_flags_t>();
}

CoreWrapper::~CoreWrapper()
{
    cemucore::cemucore_destroy(qExchange(mCore, nullptr));
}

cemucore::cemucore_t *CoreWrapper::core()
{
    return mCore;
}

const cemucore::cemucore_t *CoreWrapper::core() const
{
    return mCore;
}

void CoreWrapper::sleep()
{
    cemucore::cemucore_sleep(mCore);
}

void CoreWrapper::wake()
{
    cemucore::cemucore_wake(mCore);
}

int32_t CoreWrapper::get(cemucore::cemucore_prop_t prop, int32_t addr) const
{
    return cemucore::cemucore_get(mCore, prop, addr);
}

void CoreWrapper::set(cemucore::cemucore_prop_t prop, int32_t addr, int32_t val)
{
    cemucore::cemucore_set(mCore, prop, addr, val);
}

void CoreWrapper::signalHandler(cemucore::signal_t signal, void *data)
{
    CoreWrapper *core = static_cast<CoreWrapper *>(data);
    switch (signal)
    {
    case cemucore::CEMUCORE_SIGNAL_LCD_FRAME:
        emit core->lcdFrame();
        break;
    case cemucore::CEMUCORE_SIGNAL_SOFT_CMD:
        emit core->softCmd();
        break;
    }
}
