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
#include "app_private_key_instruction_executor.h"

#include <stdlib.h>
#include <string.h>

static uint16_t rd16(const uint8_t *p) { return (uint16_t)p[0] | ((uint16_t)p[1] << 8); }
static int16_t rd16s(const uint8_t *p) { return (int16_t)rd16(p); }
static uint32_t load_table_word_le32(const uint8_t *p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }

static int check_span(size_t off, size_t n, size_t cap) {
    return off <= cap && n <= cap - off;
}

static int execute_stage1_instruction(const l3_private_key_tables *tables, l3_private_key_dispatch_op op,
                           uint32_t k, uint16_t off_a, uint16_t off_b, uint16_t off_c,
                           uint8_t *work, size_t work_len) {
    if (!work || !tables) return L3_PRIVATE_KEY_DISPATCH_ERR_ARGUMENT;
    switch (op) {
    case L3_PRIVATE_KEY_DISPATCH_OP_F84C1C:
        if (!check_span(off_a,6,work_len)||!check_span(off_b,6,work_len)||!check_span(off_c,6,work_len)) return L3_PRIVATE_KEY_DISPATCH_ERR_ARGUMENT;
        return l3_private_key_table_mix6_a(tables, k, work + off_a, work + off_b, work + off_c);
    case L3_PRIVATE_KEY_DISPATCH_OP_F84D3C:
        if (!check_span(off_a,6,work_len)||!check_span(off_b,6,work_len)) return L3_PRIVATE_KEY_DISPATCH_ERR_ARGUMENT;
        return l3_private_key_table_map6(tables, k, work + off_a, work + off_b);
    case L3_PRIVATE_KEY_DISPATCH_OP_SHIFT_INSERT_CLEAR:
        if (!check_span(off_a,6,work_len)||!check_span(off_b,6,work_len)) return L3_PRIVATE_KEY_DISPATCH_ERR_ARGUMENT;
        return l3_private_key_update_stage1_shift_register(work + off_b, work_len - off_b, work + off_a, work_len - off_a, k);
    case L3_PRIVATE_KEY_DISPATCH_OP_F84E78:
        if (!check_span(off_a,6,work_len)||!check_span(off_b,6,work_len)||!check_span(off_c,6,work_len)) return L3_PRIVATE_KEY_DISPATCH_ERR_ARGUMENT;
        return l3_private_key_table_mix6_b(tables, k, work + off_a, work + off_b, work + off_c);
    case L3_PRIVATE_KEY_DISPATCH_OP_F84F80:
        if (!check_span(off_a,6,work_len)||!check_span(off_b,6,work_len)||!check_span(off_c,4,work_len)) return L3_PRIVATE_KEY_DISPATCH_ERR_ARGUMENT;
        return l3_private_key_table_mix6_to4_a(tables, k, work + off_a, work + off_b, work + off_c);
    case L3_PRIVATE_KEY_DISPATCH_OP_F85048:
        if (!check_span(off_a,6,work_len)||!check_span(off_b,6,work_len)||!check_span(off_c,4,work_len)) return L3_PRIVATE_KEY_DISPATCH_ERR_ARGUMENT;
        return l3_private_key_table_mix6_to4_b(tables, k, work + off_a, work + off_b, work + off_c);
    case L3_PRIVATE_KEY_DISPATCH_OP_F85150:
        if (!check_span(off_a,5,work_len)||!check_span(off_b,5,work_len)||!check_span(off_c,4,work_len)) return L3_PRIVATE_KEY_DISPATCH_ERR_ARGUMENT;
        return l3_private_key_table_mix5_to4(tables, k, work + off_a, work + off_b, work + off_c);
    case L3_PRIVATE_KEY_DISPATCH_OP_F85238:
        if (!check_span(off_a,6,work_len)||!check_span(off_b,6,work_len)||!check_span(off_c,4,work_len)) return L3_PRIVATE_KEY_DISPATCH_ERR_ARGUMENT;
        return l3_private_key_table_mix6_to4_c(tables, k, work + off_a, work + off_b, work + off_c);
    case L3_PRIVATE_KEY_DISPATCH_OP_F8535C:
        if (!check_span(off_a,6,work_len)||!check_span(off_b,6,work_len)||!check_span(off_c,6,work_len)) return L3_PRIVATE_KEY_DISPATCH_ERR_ARGUMENT;
        return l3_private_key_table_mix6_c(tables, k, work + off_a, work + off_b, work + off_c);
    case L3_PRIVATE_KEY_DISPATCH_OP_F85494:
        if (!check_span(off_a,6,work_len)||!check_span(off_b,4,work_len)) return L3_PRIVATE_KEY_DISPATCH_ERR_ARGUMENT;
        return l3_private_key_table_map6_to4(tables, k, work + off_a, work + off_b);
    default:
        return L3_PRIVATE_KEY_DISPATCH_ERR_UNKNOWN_DELTA;
    }
}

int l3_private_key_execute_stage1_program(const l3_private_key_tables *tables,
                              const uint8_t *program, size_t program_len,
                              uint8_t *work, size_t work_len) {
    if (!tables || !program || !work) return L3_PRIVATE_KEY_DISPATCH_ERR_ARGUMENT;
    if (work_len < L3_DISPATCH_WORK_F80040) return L3_PRIVATE_KEY_DISPATCH_ERR_SHORT_OUT;
    int16_t cur = 0x244;
    size_t n = program_len / 12u;
    for (size_t i=0; i<n; ++i) {
        const uint8_t *p = program + i*12u;
        uint32_t k = load_table_word_le32(p);
        uint16_t a = rd16(p+4), b = rd16(p+6), c = rd16(p+8);
        int16_t next = rd16s(p+10);
        l3_private_key_dispatch_op op = l3_private_key_decode_stage1_instruction(cur);
        int rc = execute_stage1_instruction(tables, op, k, a, b, c, work, work_len);
        if (rc) return rc;
        cur = next;
        if (cur == 0) return L3_PRIVATE_KEY_DISPATCH_OK;
    }
    return L3_PRIVATE_KEY_DISPATCH_ERR_UNKNOWN_DELTA;
}

int l3_private_key_execute_stage2_program(const l3_private_key_tables *tables,
                              const uint8_t *program, size_t program_len,
                              uint8_t *work, size_t work_len) {
    if (!tables || !program || !work) return L3_PRIVATE_KEY_DISPATCH_ERR_ARGUMENT;
    if (work_len < L3_DISPATCH_WORK_F80F0C) return L3_PRIVATE_KEY_DISPATCH_ERR_SHORT_OUT;
    int16_t cur = 0x020;
    size_t n = program_len / 12u;
    for (size_t i=0; i<n; ++i) {
        const uint8_t *p = program + i*12u;
        uint32_t k = load_table_word_le32(p);
        uint16_t a = rd16(p+4), b = rd16(p+6), c = rd16(p+8);
        int16_t next = rd16s(p+10);
        l3_private_key_dispatch_op op = l3_private_key_decode_stage2_instruction(cur);
        if (op == L3_PRIVATE_KEY_DISPATCH_OP_F84C1C) {
            if (!check_span(a,6,work_len)||!check_span(b,6,work_len)||!check_span(c,6,work_len)) return L3_PRIVATE_KEY_DISPATCH_ERR_ARGUMENT;
            int rc = l3_private_key_table_mix6_a(tables, k, work+a, work+b, work+c);
            if (rc) return rc;
        } else if (op == L3_PRIVATE_KEY_DISPATCH_OP_F84D3C) {
            if (!check_span(a,6,work_len)||!check_span(b,6,work_len)) return L3_PRIVATE_KEY_DISPATCH_ERR_ARGUMENT;
            int rc = l3_private_key_table_map6(tables, k, work+a, work+b);
            if (rc) return rc;
        } else {
            return L3_PRIVATE_KEY_DISPATCH_ERR_UNKNOWN_DELTA;
        }
        cur = next;
        if (cur == 0) return L3_PRIVATE_KEY_DISPATCH_OK;
    }
    return L3_PRIVATE_KEY_DISPATCH_ERR_UNKNOWN_DELTA;
}
