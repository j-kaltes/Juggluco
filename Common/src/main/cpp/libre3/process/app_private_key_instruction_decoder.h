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
#ifndef L3_APP_PRIVATE_KEY_INSTRUCTION_DECODER_H
#define L3_APP_PRIVATE_KEY_INSTRUCTION_DECODER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define L3_PRIVATE_KEY_DISPATCH_OK 0
#define L3_PRIVATE_KEY_DISPATCH_ERR_ARGUMENT -1
#define L3_PRIVATE_KEY_DISPATCH_ERR_SHORT_OUT -2
#define L3_PRIVATE_KEY_DISPATCH_ERR_UNKNOWN_DELTA -3

/* Native 12-byte dispatch entry produced in DAT_f4369d10 / DAT_f43901d8. */
typedef struct {
    uint32_t k_or_word;
    uint16_t off_a;
    uint16_t off_b;
    uint16_t off_c;
    int16_t next_delta;
} l3_private_key_dispatch_raw_entry;

typedef enum {
    L3_PRIVATE_KEY_DISPATCH_OP_INVALID = 0,
    L3_PRIVATE_KEY_DISPATCH_OP_F84C1C,
    L3_PRIVATE_KEY_DISPATCH_OP_F84D3C,
    L3_PRIVATE_KEY_DISPATCH_OP_SHIFT_INSERT_CLEAR,
    L3_PRIVATE_KEY_DISPATCH_OP_F84E78,
    L3_PRIVATE_KEY_DISPATCH_OP_F84F80,
    L3_PRIVATE_KEY_DISPATCH_OP_F85048,
    L3_PRIVATE_KEY_DISPATCH_OP_F85150,
    L3_PRIVATE_KEY_DISPATCH_OP_F85238,
    L3_PRIVATE_KEY_DISPATCH_OP_F8535C,
    L3_PRIVATE_KEY_DISPATCH_OP_F85494,
} l3_private_key_dispatch_op;

typedef struct {
    l3_private_key_dispatch_raw_entry raw;
    int16_t current_delta;
    l3_private_key_dispatch_op op;
} l3_private_key_dispatch_decoded_entry;

l3_private_key_dispatch_op l3_private_key_decode_stage1_instruction(int16_t delta);
l3_private_key_dispatch_op l3_private_key_decode_stage2_instruction(int16_t delta);



/*
 * Deobfuscated behavior of the f3f80040 0x214-delta inline block.
 * This corresponds to the ARM block at f3f80bac..f3f80c3c.
 *
 * word layout:
 *   low    = word & 0x1ff
 *   width  = (word >> 9) & 0x1ff
 *   hi     = (word >> 18) & 0x1ff
 *   insert = (word >> 27) & 7
 *
 * It clears dst[0..low-2], inserts `insert` at dst[low-1], then copies
 * from src backward into dst[low..hi-1], with zero-fill past `width`.
 * Native does not pre-reject the encoded width against a C buffer length;
 * the host implementation only clamps individual loads to src_len to avoid
 * out-of-bounds reads in tests.
 */
int l3_private_key_update_stage1_shift_register(uint8_t *dst, size_t dst_len,
                                 const uint8_t *src, size_t src_len,
                                 uint32_t word);

#ifdef __cplusplus
}
#endif

#endif
