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

#ifndef DISASSEMBLY_H
#define DISASSEMBLY_H

#include "../../corewindow.h"

#include "../../deps/zdis/zdis.h"

#include <string>
#include <unordered_map>

class Disassembly
{
public:
    explicit Disassembly();

    QString disassemble(uint32_t addr, uint32_t *nextAddr);

private:
    char *strWord(int32_t data, bool il);
    char *strAddr(int32_t data, bool il);

    int zdisRead(struct zdis_ctx *ctx, uint32_t addr);
    bool zdisPut(struct zdis_ctx *ctx, enum zdis_put kind, int32_t val, bool il);

    struct zdis_ctx mZdis;

    QString mBuffer;
};

#endif
