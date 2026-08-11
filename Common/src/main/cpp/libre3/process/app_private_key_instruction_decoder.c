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
#include "app_private_key_instruction_decoder.h"

#include <string.h>



l3_private_key_dispatch_op l3_private_key_decode_stage1_instruction(int16_t delta) {
    switch (delta) {
    case 0x27c: return L3_PRIVATE_KEY_DISPATCH_OP_F84C1C;             /* entry point f3f80b44 */
    case 0x244: return L3_PRIVATE_KEY_DISPATCH_OP_F84D3C;             /* entry point f3f80b7c */
    case 0x214: return L3_PRIVATE_KEY_DISPATCH_OP_SHIFT_INSERT_CLEAR; /* inline block f3f80bac */
    case 0x180: return L3_PRIVATE_KEY_DISPATCH_OP_F84E78;             /* entry point f3f80c40 */
    case 0x148: return L3_PRIVATE_KEY_DISPATCH_OP_F84F80;             /* entry point f3f80c78 */
    case 0x110: return L3_PRIVATE_KEY_DISPATCH_OP_F85048;             /* entry point f3f80cb0 */
    case 0x0d8: return L3_PRIVATE_KEY_DISPATCH_OP_F85150;             /* entry point f3f80ce8 */
    case 0x0a0: return L3_PRIVATE_KEY_DISPATCH_OP_F85238;             /* entry point f3f80d20 */
    case 0x068: return L3_PRIVATE_KEY_DISPATCH_OP_F8535C;             /* entry point f3f80d58 */
    case 0x030: return L3_PRIVATE_KEY_DISPATCH_OP_F85494;             /* entry point f3f80d90 */
    default: return L3_PRIVATE_KEY_DISPATCH_OP_INVALID;
    }
}

l3_private_key_dispatch_op l3_private_key_decode_stage2_instruction(int16_t delta) {
    switch (delta) {
    case 0x05c: return L3_PRIVATE_KEY_DISPATCH_OP_F84C1C; /* entry point f3f81618 */
    case 0x020: return L3_PRIVATE_KEY_DISPATCH_OP_F84D3C; /* entry point f3f81654 */
    default: return L3_PRIVATE_KEY_DISPATCH_OP_INVALID;
    }
}




int l3_private_key_update_stage1_shift_register(uint8_t *dst, size_t dst_len,
                                 const uint8_t *src, size_t src_len,
                                 uint32_t word) {
    if (!dst || !src) return L3_PRIVATE_KEY_DISPATCH_ERR_ARGUMENT;
    uint32_t low = word & 0x1ffu;
    uint32_t width = (word >> 9) & 0x1ffu;
    uint32_t hi = (word >> 18) & 0x1ffu;
    uint8_t insert = (uint8_t)((word >> 27) & 7u);
    if (low == 0 || hi > dst_len) return L3_PRIVATE_KEY_DISPATCH_ERR_ARGUMENT;

    if (hi > low) {
        uint32_t src_i = hi - low - 1u;
        for (uint32_t dst_i = hi - 1u;; --dst_i) {
            dst[dst_i] = (src_i < width && src_i < src_len) ? src[src_i] : 0;
            if (dst_i == low) break;
            if (src_i > 0) src_i--; else src_i = 0xffffffffu;
        }
    }
    dst[low - 1u] = insert;
    memset(dst, 0, low - 1u);
    return L3_PRIVATE_KEY_DISPATCH_OK;
}
