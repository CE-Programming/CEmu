/*
 * Copyright (c) 2015-2021 CE Programming.
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

#include "disassembler.h"

#include "../../corethread.h"
#include "../../util.h"

#include <utility>

Disassembler::Disassembler()
{
    mZdis.zdis_user_ptr = reinterpret_cast<uint8_t *>(this);
    mZdis.zdis_read = [](zdis_ctx *ctx, uint32_t addr) -> int
    {
        return reinterpret_cast<Disassembler *>(ctx->zdis_user_ptr)->zdisRead(addr);
    };
    mZdis.zdis_put = [](zdis_ctx *ctx, zdis_put kind, int32_t val, bool il) -> bool
    {
        return reinterpret_cast<Disassembler *>(ctx->zdis_user_ptr)->zdisPut(ctx, kind, val, il);
    };
}

QPair<QString, QString> Disassembler::disassemble(uint32_t &addr)
{
    mZdis.zdis_end_addr = addr;
    mZdis.zdis_lowercase = true;
    mZdis.zdis_implicit = false;
    mBuffer.clear();
    mData.clear();

    zdis_put_inst(&mZdis);

    addr = mZdis.zdis_end_addr;

    return {std::move(mBuffer), std::move(mData)};
}

void Disassembler::setAdl(bool enable)
{
    mZdis.zdis_adl = enable;
}

QString Disassembler::strWord(int32_t data, bool il)
{
    return QStringLiteral("$%1").arg(QString::number(data, 16).toUpper(), il ? 6 : 4, QLatin1Char{'0'});
}

QString Disassembler::strAddr(int32_t data, bool il)
{
    return QStringLiteral("$%1").arg(QString::number(data, 16).toUpper(), il ? 6 : 4, QLatin1Char{'0'});
}

int Disassembler::zdisRead(uint32_t addr)
{
    if (addr >= UINT32_C(1) << 24)
    {
        return EOF;
    }
    int data = 0;//mCore->get(cemucore::CEMUCORE_PROP_MEM_ADL, addr);
    mData += Util::int2hex(data, 2);
    return data;
}

bool Disassembler::zdisPut(zdis_ctx *ctx, zdis_put kind, int32_t val, bool il)
{
    switch (kind)
    {
        case ZDIS_PUT_BYTE:
        case ZDIS_PUT_PORT:
            mBuffer += QStringLiteral("$%1").arg(QString::number(val, 16).toUpper(), 2, QLatin1Char{'0'});
            break;
        case ZDIS_PUT_WORD:
            mBuffer += strWord(val, il);
            break;
        case ZDIS_PUT_OFF:
            if (val < 0)
            {
                val = -val;
                mBuffer += QLatin1Char{'-'};
            }
            else
            {
                mBuffer += QLatin1Char{'+'};
            }
            if (val)
            {
                mBuffer += QStringLiteral("$%1").arg(QString::number(val, 16).toUpper(), 2, QLatin1Char{'0'});
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
            mBuffer += QLatin1Char(val);
            break;
        case ZDIS_PUT_MNE_SEP:
            mBuffer += QString{' '};
            break;
        case ZDIS_PUT_ARG_SEP:
            mBuffer += QLatin1Char{','};
            break;
        case ZDIS_PUT_END:
            break;
    }

    return true;
}
