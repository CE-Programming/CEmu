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

#include "emuthread.h"

#include "../../core/emu.h"

void gui_console_clear(void)
{
}

void gui_console_printf(const char *format, ...)
{
    (void)format;
}

void gui_console_err_printf(const char *format, ...)
{
    (void)format;
}

void gui_debug_open(int reason, uint32_t data)
{
    (void)reason;
    (void)data;
}

void gui_debug_close(void)
{
}
