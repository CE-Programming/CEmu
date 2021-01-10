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

#include <cstring>
#include <unordered_map>

#include "disassembler.h"

Disassembler::Disassembler()
{
    mZdis.zdis_user_ptr = reinterpret_cast<uint8_t*>(this);
    mZdis.zdis_read = [](zdis_ctx *ctx, uint32_t addr) ->
            int { return reinterpret_cast<Disassembler *>(ctx->zdis_user_ptr)->zdisRead(ctx, addr); };
    mZdis.zdis_put = [](zdis_ctx *ctx, zdis_put kind, int32_t val, bool il) ->
            bool { return reinterpret_cast<Disassembler *>(ctx->zdis_user_ptr)->zdisPut(ctx, kind, val, il); };
}

QString Disassembler::disassemble(uint32_t addr, uint32_t *nextAddr)
{
    mZdis.zdis_end_addr = addr;
    mZdis.zdis_adl = 1;
    mZdis.zdis_lowercase = 1;
    mZdis.zdis_implicit = 0;

    mBuffer.clear();

    zdis_put_inst(&mZdis);

    if (nextAddr != nullptr)
    {
        *nextAddr = mZdis.zdis_end_addr;
    }

    return mBuffer;
}

char *Disassembler::strWord(int32_t data, bool il)
{
    static char tmpbuf[20];

    if (il)
    {
        sprintf(tmpbuf, "$%06X", data);
    }
    else
    {
        sprintf(tmpbuf, "$%04X", data);
    }

    return tmpbuf;
}

char *Disassembler::strAddr(int32_t data, bool il)
{
    static char tmpbuf[20];

    if (il)
    {
        sprintf(tmpbuf, "$%06X", data);
    }
    else
    {
        sprintf(tmpbuf, "$%04X", data);
    }

    return tmpbuf;
}

int Disassembler::zdisRead(struct zdis_ctx *ctx, uint32_t addr)
{
    int value = 0x83;

    (void)ctx;
    (void)addr;

    return value;
}

bool Disassembler::zdisPut(struct zdis_ctx *ctx, enum zdis_put kind, int32_t val, bool il)
{
    char tmp[11], sign = '+';
    (void)ctx;

    switch (kind)
    {
        case ZDIS_PUT_BYTE:
        case ZDIS_PUT_PORT:
            snprintf(tmp, 10, "$%02X", val);
            mBuffer += tmp;
            break;
        case ZDIS_PUT_WORD:
            mBuffer += strWord(val, il);
            break;
        case ZDIS_PUT_OFF:
            if (val < 0)
            {
                val = -val;
                sign = '-';
            }
            if (val)
            {
                snprintf(tmp, 11, "%c$%02X", sign, val);
                mBuffer += tmp;
            }
            break;
        case ZDIS_PUT_REL:
            val += ctx->zdis_end_addr;
            [[gnu::fallthrough]];
        case ZDIS_PUT_ADDR:
        case ZDIS_PUT_ABS:
        case ZDIS_PUT_RST:
            mBuffer += strAddr(val, il);
            break;
        case ZDIS_PUT_CHAR:
            mBuffer += static_cast<char>(val);
            break;
        case ZDIS_PUT_MNE_SEP:
            mBuffer += ' ';
            break;
        case ZDIS_PUT_ARG_SEP:
            mBuffer += ',';
            break;
        case ZDIS_PUT_END:
            break;
    }

    return true;
}
