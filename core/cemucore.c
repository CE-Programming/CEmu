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

#include "cemucore.h"
#include "emu.h"

cemucore_error_t cemucore_load(struct cemucore_load_info *load, struct cemucore_core_info *core)
{
    emu_state_t state = emu_load(load->type, load->path);
    return state == EMU_STATE_VALID ? CEMUCORE_SUCCESS ? CEMUCORE_ERROR_FAILED;
}

cemucore_error_t cemucore_save(struct cemucore_save_info *save)
{
    bool good = emu_save(save->type, save->path);
    return good ? CEMUCORE_SUCCESS : CEMUCORE_ERROR_FAILED;
}

void cemucore_run(uint64_t ticks)
{
    emu_run(ticks);
}

void cemucore_stop(void)
{
    emu_exit();
}

cemucore_error_t cemucore_set_run_rate(uint32_t rate)
{
    bool rc = sched_set_clock(CLOCK_RUN, rate);
    return rc ? CEMUCORE_SUCCESS : CEMUCORE_ERROR_FAILED;
}

cemucore_error_t cemucore_get_run_rate(uint32_t *rate)
{
    if (rate == NULL)
    {
        return CEMUCORE_ERROR_INVALID;
    }

    *rate = sched_get_clock_rate(CLOCK_RUN);
}
