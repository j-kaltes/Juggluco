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
#include "app_private_key_whitebox_machine.h"

#include <string.h>

#if defined(__GNUC__) || defined(__clang__)
extern int l3_private_key_decode_object_block(const l3_private_key_tables *tables,
                                              const uint8_t *stage1_program,
                                              size_t stage1_program_len,
                                              const uint8_t *stage2_program,
                                              size_t stage2_program_len,
                                              const uint8_t input16[16],
                                              const uint8_t *object_state,
                                              size_t object_state_len,
                                              uint8_t out42[0x42]) __attribute__((weak));
#else
static int l3_private_key_decode_object_block(const l3_private_key_tables *tables,
                                              const uint8_t *stage1_program,
                                              size_t stage1_program_len,
                                              const uint8_t *stage2_program,
                                              size_t stage2_program_len,
                                              const uint8_t input16[16],
                                              const uint8_t *object_state,
                                              size_t object_state_len,
                                              uint8_t out42[0x42]) {
    (void)tables; (void)stage1_program; (void)stage1_program_len;
    (void)stage2_program; (void)stage2_program_len; (void)input16;
    (void)object_state; (void)object_state_len; (void)out42;
    return L3_PRIVATE_KEY_MACHINE_ERR_NEED_FULL_METHOD;
}
#endif

static int validate_private_key_tables(const l3_private_key_tables *t) {
    if (!t || !t->state_transition_table || !t->mix_program_table || !t->map_program_table) return L3_PRIVATE_KEY_MACHINE_ERR_ARGUMENT;
    if (t->state_transition_table_len < 0x20000u || t->mix_program_table_len < 0x10000u || t->map_program_table_len < 0x20000u) {
        return L3_PRIVATE_KEY_MACHINE_ERR_TABLE_SIZE;
    }
    return L3_PRIVATE_KEY_MACHINE_OK;
}

static uint8_t dat416(const l3_private_key_tables *t, uint32_t off) {
    return t->mix_program_table[off];
}

static uint8_t dat429(const l3_private_key_tables *t, uint32_t idx) {
    return t->state_transition_table[idx & 0x1ffffu];
}

static uint16_t dat43d16(const l3_private_key_tables *t, uint32_t idx) {
    uint32_t off = idx * 2u;
    if (off + 1u >= t->map_program_table_len) return 0;
    return (uint16_t)t->map_program_table[off] | ((uint16_t)t->map_program_table[off + 1] << 8);
}

static uint8_t dat43d_low3(const l3_private_key_tables *t, uint32_t idx) {
    /* FUN_f3f84d3c ends with ldrb [DAT_f43d5968, idx, lsl #1]:
     * it returns the low byte of the same 16-bit table layout used by the
     * preceding ldrh operations.  The initialized native table is 0x88000
     * bytes; high indices must not be truncated to the old 0x20000 capture. */
    uint32_t off = idx * 2u;
    if (off >= t->map_program_table_len) return 0;
    return t->map_program_table[off] & 7u;
}

int l3_private_key_table_mix6_a(const l3_private_key_tables *t, uint32_t k,
                const uint8_t a[6], const uint8_t b[6], uint8_t out[6]) {
    int rc = validate_private_key_tables(t);
    if (rc) return rc;
    if (!a || !b || !out) return L3_PRIVATE_KEY_MACHINE_ERR_ARGUMENT;

    uint8_t b1 = dat416(t, k + 3u);
    uint8_t b2 = dat416(t, k + 4u);
    uint8_t b3 = dat429(t, ((uint32_t)b[0] << 8 | a[0]) ^ ((uint32_t)dat416(t, k + 0u) << 11));
    out[0] = b3 & 7u;

    uint8_t b4 = dat416(t, k + 2u);
    b3 = dat429(t, (((uint32_t)b[1] << 8) | ((b3 & 0xf8u) ^ a[1])) ^ ((uint32_t)dat416(t, k + 1u) << 11));
    out[1] = b3 & 7u;

    b3 = dat429(t, (((uint32_t)b[2] << 8) | ((b3 & 0xf8u) ^ a[2])) ^ ((uint32_t)b4 << 11));
    out[2] = b3 & 7u;

    b4 = dat416(t, k + 5u);
    b1 = dat429(t, (((uint32_t)b[3] << 8) | ((b3 & 0xf8u) ^ a[3])) ^ ((uint32_t)b1 << 11));
    out[3] = b1 & 7u;

    b1 = dat429(t, (((uint32_t)b[4] << 8) | ((b1 & 0xf8u) ^ a[4])) ^ ((uint32_t)b2 << 11));
    out[4] = b1 & 7u;

    out[5] = dat429(t, (((uint32_t)b[5] << 8) | ((b1 & 0xf8u) ^ a[5])) ^ ((uint32_t)b4 << 11)) & 7u;
    return L3_PRIVATE_KEY_MACHINE_OK;
}

int l3_private_key_table_map6(const l3_private_key_tables *t, uint32_t k,
                const uint8_t in[6], uint8_t out[6]) {
    int rc = validate_private_key_tables(t);
    if (rc) return rc;
    if (!in || !out) return L3_PRIVATE_KEY_MACHINE_ERR_ARGUMENT;

    uint8_t b1 = dat416(t, k + 4u);
    uint8_t b2 = dat416(t, k + 5u);

    uint16_t u0 = dat43d16(t, ((uint32_t)in[0] | ((uint32_t)dat416(t, k + 0u) << 12)));
    uint16_t u1 = dat43d16(t, (((uint32_t)(u0 & 0xff8u) ^ (uint32_t)in[1]) |
                                ((uint32_t)dat416(t, k + 1u) << 12)));
    uint16_t u2 = dat43d16(t, (((uint32_t)(u1 & 0xff8u) ^ (uint32_t)in[2]) |
                                ((uint32_t)dat416(t, k + 2u) << 12)));
    uint16_t u = dat43d16(t, (((uint32_t)in[3] ^ (uint32_t)(u2 & 0xff8u)) |
                               ((uint32_t)dat416(t, k + 3u) << 12)));
    out[0] = (uint8_t)u & 7u;

    u = dat43d16(t, (((uint32_t)in[4] ^ (uint32_t)(u & 0xff8u)) | ((uint32_t)b1 << 12)));
    out[1] = (uint8_t)u & 7u;

    b1 = dat416(t, k + 6u);
    u = dat43d16(t, (((uint32_t)in[5] ^ (uint32_t)(u & 0xff8u)) | ((uint32_t)b2 << 12)));
    out[2] = (uint8_t)u & 7u;

    b2 = dat416(t, k + 7u);
    uint8_t b3 = dat416(t, k + 8u);
    u = dat43d16(t, ((uint32_t)in[2] ^ (uint32_t)u ^ ((uint32_t)b1 << 12)));
    out[3] = (uint8_t)u & 7u;

    u = dat43d16(t, ((uint32_t)in[3] ^ (uint32_t)u ^ ((uint32_t)b2 << 12)));
    out[4] = (uint8_t)u & 7u;

    out[5] = dat43d_low3(t, ((uint32_t)in[4] ^ (uint32_t)u ^ ((uint32_t)b3 << 12)));
    return L3_PRIVATE_KEY_MACHINE_OK;
}

int l3_private_key_table_mix6_c(const l3_private_key_tables *t, uint32_t k,
                const uint8_t a[6], const uint8_t b[6], uint8_t out[6]) {
    int rc = validate_private_key_tables(t);
    if (rc) return rc;
    if (!a || !b || !out) return L3_PRIVATE_KEY_MACHINE_ERR_ARGUMENT;

    uint8_t x0 = dat429(t, ((uint32_t)b[0] << 8 | a[0]) ^ ((uint32_t)dat416(t,k+0u) << 11));
    uint8_t x1 = dat429(t, ((uint32_t)((x0 & 0xf8u) ^ a[1]) | ((uint32_t)b[1] << 8)) ^ ((uint32_t)dat416(t,k+1u) << 11));
    uint8_t x2 = dat429(t, ((uint32_t)(a[2] ^ (x1 & 0xf8u)) | ((uint32_t)b[2] << 8)) ^ ((uint32_t)dat416(t,k+2u) << 11));

    out[0] = x2 & 7u;
    uint8_t x = dat429(t, (((uint32_t)b[3] << 8) | (a[3] ^ (x2 & 0xf8u))) ^ ((uint32_t)dat416(t,k+3u) << 11));
    out[1] = x & 7u;

    x = dat429(t, (((uint32_t)b[4] << 8) | (a[4] ^ (x & 0xf8u))) ^ ((uint32_t)dat416(t,k+4u) << 11));
    out[2] = x & 7u;

    x = dat429(t, (((uint32_t)b[5] << 8) | (a[5] ^ (x & 0xf8u))) ^ ((uint32_t)dat416(t,k+5u) << 11));
    out[3] = x & 7u;

    x = dat429(t, (uint32_t)(x & 0xf8u) | ((uint32_t)dat416(t,k+6u) << 11));
    out[4] = x & 7u;

    out[5] = dat429(t, (uint32_t)(x & 0xf8u) | ((uint32_t)dat416(t,k+7u) << 11)) & 7u;
    return L3_PRIVATE_KEY_MACHINE_OK;
}

static uint32_t load_le32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

int l3_private_key_construct_object_state(const uint8_t *manager_a8_selected,
                                size_t manager_a8_selected_len,
                                uint8_t *out_state_0x5a4,
                                size_t out_state_len) {
    static const uint16_t dst_offs[16] = {
        0x004u, 0x05eu, 0x0b8u, 0x112u,
        0x16cu, 0x1c6u, 0x220u, 0x27au,
        0x2d4u, 0x32eu, 0x388u, 0x3e2u,
        0x43cu, 0x496u, 0x4f0u, 0x54au
    };
    if (!manager_a8_selected || !out_state_0x5a4) return L3_PRIVATE_KEY_MACHINE_ERR_ARGUMENT;
    if (manager_a8_selected_len < 4u || out_state_len < 0x5a4u) return L3_PRIVATE_KEY_MACHINE_ERR_ARGUMENT;

    memset(out_state_0x5a4, 0, out_state_len);
    uint32_t count_word = load_le32(manager_a8_selected);
    out_state_0x5a4[0] = manager_a8_selected[0];
    out_state_0x5a4[1] = manager_a8_selected[1];
    out_state_0x5a4[2] = manager_a8_selected[2];
    out_state_0x5a4[3] = manager_a8_selected[3];

    if ((int32_t)count_word < 0) return L3_PRIVATE_KEY_MACHINE_OK;
    size_t rounds = (size_t)count_word + 1u;
    if (manager_a8_selected_len < 4u + rounds * 0x60u) return L3_PRIVATE_KEY_MACHINE_ERR_ARGUMENT;
    if (rounds > 0x40u) return L3_PRIVATE_KEY_MACHINE_ERR_ARGUMENT;

    const uint8_t *src = manager_a8_selected + 4u;
    for (size_t r = 0; r < rounds; ++r) {
        size_t dst_base = r * 6u;
        for (size_t j = 0; j < 16u; ++j) {
            size_t so = r * 0x60u + j * 6u;
            size_t doff = dst_base + dst_offs[j];
            if (doff + 6u > out_state_len) return L3_PRIVATE_KEY_MACHINE_ERR_ARGUMENT;
            memcpy(out_state_0x5a4 + doff, src + so, 6u);
        }
    }
    return L3_PRIVATE_KEY_MACHINE_OK;
}

int l3_private_key_table_transform(const l3_private_key_tables *t, uint32_t param1, uint32_t param2,
                const uint8_t *p3, const uint8_t *p4, uint8_t *out, size_t out_len) {
    int rc = validate_private_key_tables(t);
    if (rc) return rc;
    if (!p3 || !p4 || !out) return L3_PRIVATE_KEY_MACHINE_ERR_ARGUMENT;

    uint32_t first = ((param2 << 14) & 0xffffffffu) >> 18;
    uint32_t rem = param2 >> 18;
    if (out_len < (size_t)first + rem) return L3_PRIVATE_KEY_MACHINE_ERR_ARGUMENT;

    uint8_t state = 0;
    for (uint32_t i = 0; i < first; ++i) {
        uint32_t idx = (((uint32_t)(state & 0xf8u) ^ p3[i]) | ((uint32_t)p4[i] << 8)) ^
                       ((uint32_t)dat416(t, (param1 & 0x3fffffu) + i) << 11);
        state = dat429(t, idx);
        out[i] = state & 7u;
    }

    for (uint32_t i = 0; i < rem; ++i) {
        uint32_t idx = ((uint32_t)(state & 0xf8u) | ((uint32_t)p4[first + i] << 8)) ^
                       ((uint32_t)dat416(t, (param1 & 0x3fffffu) + first + i) << 11);
        state = dat429(t, idx);
        out[first + i] = state & 7u;
    }
    return L3_PRIVATE_KEY_MACHINE_OK;
}




static uint8_t dd_step429(const l3_private_key_tables *t, uint8_t prev, uint8_t a, uint8_t b, uint8_t k) {
    return dat429(t, (((uint32_t)b << 8) | ((uint32_t)(prev & 0xf8u) ^ a)) ^ ((uint32_t)k << 11));
}

int l3_private_key_table_mix6_b(const l3_private_key_tables *t, uint32_t k,
                const uint8_t a[6], const uint8_t b[6], uint8_t out[6]) {
    int rc = validate_private_key_tables(t); if (rc) return rc;
    if (!a || !b || !out) return L3_PRIVATE_KEY_MACHINE_ERR_ARGUMENT;
    uint8_t x = dat429(t, ((uint32_t)b[0] << 8 | a[0]) ^ ((uint32_t)dat416(t,k+0) << 11)); out[0]=x&7;
    x = dd_step429(t,x,a[1],b[1],dat416(t,k+1)); out[1]=x&7;
    x = dd_step429(t,x,a[2],b[2],dat416(t,k+2)); out[2]=x&7;
    x = dd_step429(t,x,a[3],b[3],dat416(t,k+3)); out[3]=x&7;
    x = dat429(t, ((uint32_t)(x & 0xf8u) | ((uint32_t)b[4] << 8)) ^ ((uint32_t)dat416(t,k+4) << 11)); out[4]=x&7;
    out[5] = dat429(t, ((uint32_t)(x & 0xf8u) | ((uint32_t)b[5] << 8)) ^ ((uint32_t)dat416(t,k+5) << 11)) & 7u;
    return L3_PRIVATE_KEY_MACHINE_OK;
}

int l3_private_key_table_mix6_to4_a(const l3_private_key_tables *t, uint32_t k,
                const uint8_t a[6], const uint8_t b[6], uint8_t out[4]) {
    int rc = validate_private_key_tables(t); if (rc) return rc;
    if (!a || !b || !out) return L3_PRIVATE_KEY_MACHINE_ERR_ARGUMENT;
    uint8_t x = dat429(t, ((uint32_t)b[0] << 8 | a[0]) ^ ((uint32_t)dat416(t,k+0) << 11)); out[0]=x&7;
    x = dd_step429(t,x,a[1],b[1],dat416(t,k+1)); out[1]=x&7;
    x = dd_step429(t,x,a[2],b[2],dat416(t,k+2)); out[2]=x&7;
    out[3] = dd_step429(t,x,a[3],b[3],dat416(t,k+3)) & 7u;
    return L3_PRIVATE_KEY_MACHINE_OK;
}

int l3_private_key_table_mix6_to4_b(const l3_private_key_tables *t, uint32_t k,
                const uint8_t a[6], const uint8_t b[6], uint8_t out[4]) {
    int rc = validate_private_key_tables(t); if (rc) return rc;
    if (!a || !b || !out) return L3_PRIVATE_KEY_MACHINE_ERR_ARGUMENT;
    uint8_t x = dat429(t, ((uint32_t)b[0] << 8 | a[0]) ^ ((uint32_t)dat416(t,k+0) << 11));
    x = dd_step429(t,x,a[1],b[1],dat416(t,k+1));
    x = dd_step429(t,x,a[2],b[2],dat416(t,k+2)); out[0]=x&7;
    x = dd_step429(t,x,a[3],b[3],dat416(t,k+3)); out[1]=x&7;
    x = dd_step429(t,x,a[4],b[4],dat416(t,k+4)); out[2]=x&7;
    out[3] = dd_step429(t,x,a[5],b[5],dat416(t,k+5)) & 7u;
    return L3_PRIVATE_KEY_MACHINE_OK;
}

int l3_private_key_table_mix5_to4(const l3_private_key_tables *t, uint32_t k,
                const uint8_t a[5], const uint8_t b[5], uint8_t out[4]) {
    int rc = validate_private_key_tables(t); if (rc) return rc;
    if (!a || !b || !out) return L3_PRIVATE_KEY_MACHINE_ERR_ARGUMENT;
    uint8_t x = dat429(t, ((uint32_t)b[0] << 8 | a[0]) ^ ((uint32_t)dat416(t,k+0) << 11));
    x = dd_step429(t,x,a[1],b[1],dat416(t,k+1)); out[0]=x&7;
    x = dd_step429(t,x,a[2],b[2],dat416(t,k+2)); out[1]=x&7;
    x = dd_step429(t,x,a[3],b[3],dat416(t,k+3)); out[2]=x&7;
    out[3] = dd_step429(t,x,a[4],b[4],dat416(t,k+4)) & 7u;
    return L3_PRIVATE_KEY_MACHINE_OK;
}

int l3_private_key_table_mix6_to4_c(const l3_private_key_tables *t, uint32_t k,
                const uint8_t a[6], const uint8_t b[6], uint8_t out[4]) {
    int rc = validate_private_key_tables(t); if (rc) return rc;
    if (!a || !b || !out) return L3_PRIVATE_KEY_MACHINE_ERR_ARGUMENT;
    uint8_t x = dat429(t, ((uint32_t)b[0] << 8 | a[0]) ^ ((uint32_t)dat416(t,k+0) << 11));
    x = dd_step429(t,x,a[1],b[1],dat416(t,k+1));
    x = dd_step429(t,x,a[2],b[2],dat416(t,k+2));
    x = dd_step429(t,x,a[3],b[3],dat416(t,k+3)); out[0]=x&7;
    x = dd_step429(t,x,a[4],b[4],dat416(t,k+4)); out[1]=x&7;
    x = dd_step429(t,x,a[5],b[5],dat416(t,k+5)); out[2]=x&7;
    out[3] = dat429(t, ((uint32_t)(x & 0xf8u)) | ((uint32_t)dat416(t,k+6) << 11)) & 7u;
    return L3_PRIVATE_KEY_MACHINE_OK;
}

int l3_private_key_table_map6_to4(const l3_private_key_tables *t, uint32_t k,
                const uint8_t in[6], uint8_t out[4]) {
    int rc = validate_private_key_tables(t); if (rc) return rc;
    if (!in || !out) return L3_PRIVATE_KEY_MACHINE_ERR_ARGUMENT;
    uint16_t u = dat43d16(t, ((uint32_t)in[0] | ((uint32_t)dat416(t,k+0) << 12)));
    u = dat43d16(t, (((uint32_t)in[1] ^ (u & 0xff8u)) | ((uint32_t)dat416(t,k+1) << 12)));
    u = dat43d16(t, (((uint32_t)in[2] ^ (u & 0xff8u)) | ((uint32_t)dat416(t,k+2) << 12)));
    u = dat43d16(t, (((uint32_t)in[3] ^ (u & 0xff8u)) | ((uint32_t)dat416(t,k+3) << 12)));
    out[0] = (uint8_t)u & 7u;
    u = dat43d16(t, (((uint32_t)in[4] ^ (u & 0xff8u)) | ((uint32_t)dat416(t,k+4) << 12)));
    out[1] = (uint8_t)u & 7u;
    u = dat43d16(t, (((uint32_t)in[5] ^ (u & 0xff8u)) | ((uint32_t)dat416(t,k+5) << 12)));
    out[2] = (uint8_t)u & 7u;
    out[3] = dat43d_low3(t, (uint32_t)in[2] ^ (uint32_t)u ^
                              ((uint32_t)dat416(t, k + 6u) << 12));
    return L3_PRIVATE_KEY_MACHINE_OK;
}
