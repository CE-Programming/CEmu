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

#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include "../../deps/zdis/zdis.h"
class CoreWrapper;

#include <QtCore/QPair>
#include <QtCore/QString>

class Disassembler
{
public:
    explicit Disassembler();

    QPair<QString, QString> disassemble(const CoreWrapper &core, uint32_t &addr);

private:
    QString strWord(int32_t data, bool il);
    QString strAddr(int32_t data, bool il);

    int zdisRead(uint32_t addr);
    bool zdisPut(zdis_ctx *ctx, zdis_put kind, int32_t val, bool il);

    zdis_ctx mZdis;
    const CoreWrapper *mCore;
    QString mBuffer;
    QString mData;
};

#endif
