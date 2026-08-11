/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2(+), Libre 3(+), Dexcom G7/ONE+,        */
/*      Sibionics GS1Sb and GS3, Accu-Chek SmartGuide, CareSens Air and              */
/*      Aidex X sensors.                                                             */
/*                                                                                   */
/*      Copyright (C) 2021 Jaap Korthals Altes <jaapkorthalsaltes@gmail.com>         */
/*                                                                                   */
/*      Juggluco is free software: you can redistribute it and/or modify             */
/*      it under the terms of the GNU General Public License as published            */
/*      by the Free Software Foundation, either version 3 of the License, or         */
/*      (at your option) any later version.                                          */
/*                                                                                   */
/*      Juggluco is distributed in the hope that it will be useful, but              */
/*      WITHOUT ANY WARRANTY; without even the implied warranty of                   */
/*      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                         */
/*      See the GNU General Public License for more details.                         */
/*                                                                                   */
/*      You should have received a copy of the GNU General Public License            */
/*      along with Juggluco. If not, see <https://www.gnu.org/licenses/>.            */
/*                                                                                   */
/*      Tue Aug 11 16:33:40 CEST 2026                                                */
#include "p256_coordinate_codec.h"

#include <string.h>

static uint32_t load28_le_bits(const uint8_t in[35], unsigned lane) {
    unsigned bit = lane * 28u;
    unsigned byte = bit >> 3;
    unsigned shift = bit & 7u;
    uint64_t acc = 0;
    for (unsigned i = 0; i < 5u && byte + i < 35u; ++i) {
        acc |= (uint64_t)in[byte + i] << (8u * i);
    }
    return (uint32_t)((acc >> shift) & 0x0fffffffu);
}

void l3_p256_decode_coordinate_35_to_10_lanes(const uint8_t in35[35], uint32_t out10[10]) {
    if (!in35 || !out10) return;
    uint32_t x[10];
    for (unsigned i = 0; i < 10u; ++i) x[i] = load28_le_bits(in35, i);

    out10[0] = x[0] * 0x76a1775du + 0x76cba8dbu;
    /*
     * Ghidra renders the native NEON instruction as
     * VectorMultiplyAccumulate(...).  The extracted lanes are also the
     * accumulator, so lanes 1..4 retain these per-lane affine constants.
     */
    out10[1] = x[1] * 0x414f921bu + 0x7e35f794u;
    out10[2] = x[2] * 0x0bc8c375u + 0x890be324u;
    out10[3] = x[3] * 0x6e86ec5fu + 0x74f9345fu;
    out10[4] = x[4] * 0x6ca29251u + 0xb12a6ae2u;
    out10[5] = x[5] * 0x2ce4237fu + 0x6932e193u;
    out10[6] = x[6] * 0x7a015a79u - 0x07e77a96u;
    out10[7] = x[7] * (uint32_t)-0x39a2e1c5 + (uint32_t)-0x365aac8f;
    out10[8] = x[8] * 0x76a1775du + 0x76cba8dbu;
    out10[9] = x[9] * 0x414f921bu + 0x7e35f794u;
}

