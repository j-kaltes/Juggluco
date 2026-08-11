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
#include "authorization_frame_transform.h"

#include <stdlib.h>
#include <string.h>

static const uint8_t k_norm_dat40c6952[128] = {
    0x01, 0x01, 0x03, 0x01, 0x04, 0x04, 0x03, 0x01, 0x00, 0x00, 0x03, 0x06, 0x00, 0x00, 0x07, 0x06,
    0x02, 0x01, 0x07, 0x02, 0x05, 0x02, 0x02, 0x05, 0x07, 0x06, 0x06, 0x03, 0x00, 0x00, 0x06, 0x03,
    0x06, 0x05, 0x02, 0x01, 0x01, 0x05, 0x02, 0x01, 0x06, 0x03, 0x04, 0x02, 0x04, 0x05, 0x07, 0x05,
    0x01, 0x05, 0x03, 0x04, 0x06, 0x03, 0x03, 0x01, 0x06, 0x03, 0x01, 0x04, 0x00, 0x03, 0x01, 0x02,
    0x06, 0x05, 0x07, 0x07, 0x04, 0x07, 0x01, 0x06, 0x03, 0x05, 0x00, 0x01, 0x00, 0x02, 0x04, 0x05,
    0x00, 0x00, 0x03, 0x06, 0x07, 0x01, 0x02, 0x03, 0x00, 0x07, 0x00, 0x01, 0x05, 0x06, 0x02, 0x00,
    0x05, 0x04, 0x01, 0x00, 0x06, 0x00, 0x00, 0x06, 0x05, 0x05, 0x00, 0x03, 0x03, 0x07, 0x01, 0x00,
    0x03, 0x02, 0x02, 0x05, 0x05, 0x06, 0x04, 0x03, 0x01, 0x06, 0x07, 0x02, 0x04, 0x02, 0x00, 0x05,
};


static const uint8_t k_norm_dat40c8120[16] = {
    0x06, 0x07, 0x01, 0x05, 0x06, 0x00, 0x07, 0x02,
    0x01, 0x01, 0x02, 0x01, 0x00, 0x03, 0x03, 0x03,
};

static const uint8_t k_norm_dat40c69d2[128] = {
    0x07, 0x00, 0x00, 0x07, 0x04, 0x00, 0x00, 0x04, 0x02, 0x01, 0x00, 0x04, 0x06, 0x04, 0x00, 0x04,
    0x02, 0x04, 0x04, 0x07, 0x06, 0x00, 0x03, 0x02, 0x04, 0x06, 0x07, 0x01, 0x00, 0x03, 0x00, 0x02,
    0x00, 0x05, 0x03, 0x00, 0x06, 0x02, 0x01, 0x00, 0x07, 0x06, 0x00, 0x01, 0x01, 0x00, 0x02, 0x03,
    0x07, 0x07, 0x00, 0x04, 0x06, 0x07, 0x03, 0x05, 0x05, 0x01, 0x03, 0x00, 0x00, 0x02, 0x01, 0x05,
    0x05, 0x07, 0x05, 0x05, 0x07, 0x04, 0x03, 0x01, 0x02, 0x00, 0x01, 0x01, 0x05, 0x04, 0x00, 0x00,
    0x03, 0x05, 0x06, 0x00, 0x04, 0x01, 0x06, 0x04, 0x02, 0x03, 0x06, 0x02, 0x02, 0x00, 0x02, 0x04,
    0x04, 0x06, 0x01, 0x03, 0x02, 0x01, 0x06, 0x02, 0x01, 0x01, 0x04, 0x02, 0x05, 0x01, 0x06, 0x00,
    0x07, 0x05, 0x01, 0x03, 0x02, 0x02, 0x03, 0x03, 0x03, 0x05, 0x04, 0x01, 0x05, 0x01, 0x04, 0x06,
};

static int check_transform_tables(const l3_authorization_frame_transform_tables *t) {
    if (!t || !t->dat_f4295564 || !t->dat_f40c6a52) return L3_AUTH_ERR_ARGUMENT;
    if (t->dat_f4295564_len < 0x20000u) return L3_AUTH_ERR_TABLE_SIZE;
    /* processint(6) output_bit_len=0x20 uses selectors within the extracted 0x1300-byte table.
       Partial-bit paths may require a larger contiguous DAT_f40c6a52-family dump. */
    if (t->dat_f40c6a52_len < 0x1300u) return L3_AUTH_ERR_TABLE_SIZE;
    return L3_AUTH_OK;
}

static int table_transform_emit(const l3_authorization_frame_transform_tables *t,
                                  uint32_t param1, uint32_t param2,
                                  const uint8_t *p3, const uint8_t *p4,
                                  uint8_t *out, size_t out_len) {
    uint32_t whole = ((param2 << 14) & 0xffffffffu) >> 18;
    uint32_t rem = param2 >> 18;
    uint32_t base = param1 & 0x3fffffu;
    size_t need = (size_t)whole + (size_t)rem;
    uint8_t state = 0;
    if (!p3 || !p4 || !out) return L3_AUTH_ERR_ARGUMENT;
    if (out_len < need) return L3_AUTH_ERR_ARGUMENT;
    if (t->dat_f40c6a52_len < (size_t)base + need) return L3_AUTH_ERR_TABLE_SIZE;
    for (uint32_t i = 0; i < whole; ++i) {
        uint32_t idx = (((uint32_t)(state & 0xf8u) ^ (uint32_t)p3[i]) |
                        ((uint32_t)p4[i] << 8)) ^
                       ((uint32_t)t->dat_f40c6a52[base + i] << 11);
        state = t->dat_f4295564[idx];
        out[i] = (uint8_t)(state & 7u);
    }
    for (uint32_t i = 0; i < rem; ++i) {
        uint32_t idx = ((uint32_t)(state & 0xf8u) |
                        ((uint32_t)p4[whole + i] << 8)) ^
                       ((uint32_t)t->dat_f40c6a52[base + whole + i] << 11);
        state = t->dat_f4295564[idx];
        out[whole + i] = (uint8_t)(state & 7u);
    }
    return L3_AUTH_OK;
}

static int table_transform_skip_emit(const l3_authorization_frame_transform_tables *t,
                                  uint32_t param1, uint32_t param2,
                                  const uint8_t *p3, const uint8_t *p4,
                                  uint8_t *out, size_t out_len) {
    uint32_t whole = ((param2 << 14) & 0xffffffffu) >> 18;
    uint32_t rem = param2 >> 18;
    uint32_t skip = (param1 >> 22) | ((param2 & 0xfu) << 10);
    uint32_t base = param1 & 0x3fffffu;
    size_t need = (size_t)whole + (size_t)rem;
    uint8_t state = 0;
    if (!p3 || !p4 || !out) return L3_AUTH_ERR_ARGUMENT;
    if (out_len < need) return L3_AUTH_ERR_ARGUMENT;
    if (t->dat_f40c6a52_len < (size_t)base + skip + need) return L3_AUTH_ERR_TABLE_SIZE;
    for (uint32_t i = 0; i < skip; ++i) {
        uint32_t idx = (((uint32_t)(state & 0xf8u) ^ (uint32_t)p3[i]) |
                        ((uint32_t)p4[i] << 8)) ^
                       ((uint32_t)t->dat_f40c6a52[base + i] << 11);
        state = t->dat_f4295564[idx];
    }
    for (uint32_t i = 0; i < whole; ++i) {
        uint32_t idx = (((uint32_t)(state & 0xf8u) ^ (uint32_t)p3[skip + i]) |
                        ((uint32_t)p4[skip + i] << 8)) ^
                       ((uint32_t)t->dat_f40c6a52[base + skip + i] << 11);
        state = t->dat_f4295564[idx];
        out[i] = (uint8_t)(state & 7u);
    }
    for (uint32_t i = 0; i < rem; ++i) {
        uint32_t idx = ((uint32_t)(state & 0xf8u)) ^
                       ((uint32_t)t->dat_f40c6a52[base + skip + whole + i] << 11);
        state = t->dat_f4295564[idx];
        out[whole + i] = (uint8_t)(state & 7u);
    }
    return L3_AUTH_OK;
}

uint32_t l3_encoded_frame_count_for_bits(uint32_t bit_len) {
    return (bit_len + 0x0fu) >> 4;
}

int l3_authorization_expand_frame_state(const l3_authorization_frame_transform_tables *tables,
                                const uint8_t state10a[L3_GENERATED_FRAME_STATE_LEN],
                                uint32_t bit_len,
                                uint8_t *out_frames,
                                size_t out_len,
                                uint32_t *required_frames) {
    int rc = check_transform_tables(tables);
    if (rc) return rc;
    if (!state10a || !required_frames) return L3_AUTH_ERR_ARGUMENT;
    uint32_t frames = l3_encoded_frame_count_for_bits(bit_len);
    *required_frames = frames;
    if (!out_frames) return L3_AUTH_OK;
    if (out_len < (size_t)frames * L3_AUTH_FRAME_LEN) return L3_AUTH_ERR_ARGUMENT;

    uint8_t state[L3_GENERATED_FRAME_STATE_LEN];
    uint8_t tmp[L3_GENERATED_FRAME_STATE_LEN];
    uint8_t partial[L3_AUTH_FRAME_LEN];
    uint8_t local70[0x70];
    memset(state, 0, sizeof(state));
    memset(tmp, 0, sizeof(tmp));
    memset(partial, 0, sizeof(partial));
    memset(local70, 0, sizeof(local70));

    rc = table_transform_emit(tables, 0x0c96u, 0x10a0u,
                                state10a, state10a, state, sizeof(state));
    if (rc) return rc;

    uint32_t rem_bits = bit_len & 0x0fu;
    uint32_t full_frames = bit_len >> 4;

    if (rem_bits != 0) {
        rc = table_transform_emit(tables, 0x144eu, 0x420u,
                                    state, state, partial, sizeof(partial));
        if (rc) return rc;
        if (rem_bits != 0x10u) {
            uint32_t loops = 0x10u - rem_bits;
            for (uint32_t i = 0; i < loops; ++i) {
                memset(local70, 0, sizeof(local70));
                local70[0] = 0x00;
                local70[1] = 0x00;
                local70[2] = 0x00;
                local70[3] = 0x06;
                memcpy(local70 + 0x20, partial, 0x42);
                rc = table_transform_emit(tables, 0x126au, 0x420u,
                                            local70, local70, partial, sizeof(partial));
                if (rc) return rc;
            }
        }
        rc = table_transform_emit(tables, 0x03f2u, 0x420u,
                                    partial, partial,
                                    out_frames + (size_t)full_frames * L3_AUTH_FRAME_LEN,
                                    L3_AUTH_FRAME_LEN);
        if (rc) return rc;
    }

    if (full_frames != 0) {
        for (uint32_t i = 0; i < rem_bits; ++i) {
            rc = table_transform_skip_emit(tables, 0x1000784u, 0x101060u,
                                        state, state, tmp, sizeof(state));
            if (rc) return rc;
            memcpy(state, tmp, sizeof(state));
        }
        uint32_t remain = full_frames;
        uint8_t *dst = out_frames + (size_t)full_frames * L3_AUTH_FRAME_LEN;
        while (remain != 0) {
            dst -= L3_AUTH_FRAME_LEN;
            rc = table_transform_emit(tables, 0x105cu, 0x420u,
                                        state, state, dst, L3_AUTH_FRAME_LEN);
            if (rc) return rc;
            if (remain == 1) break;
            rc = table_transform_skip_emit(tables, 0x100009bcu, 0x1000ca0u,
                                        state, state, tmp, sizeof(state));
            if (rc) return rc;
            memcpy(state, tmp, sizeof(state));
            --remain;
        }
    }
    return L3_AUTH_OK;
}


int l3_authorization_normalize_frame_state(const l3_authorization_frame_transform_tables *tables,
                             const uint8_t raw76[0x4c],
                             uint8_t out_state10a[L3_GENERATED_FRAME_STATE_LEN]) {
    int rc = check_transform_tables(tables);
    if (rc) return rc;
    if (!raw76 || !out_state10a) return L3_AUTH_ERR_ARGUMENT;

    uint8_t stack410[0x13][16];
    uint8_t block2d0[0x13][16];
    uint8_t local90[16];
    uint8_t tmp1[16], tmp2[16], tmp3[16], tmp4[16], tmp5[16];
    memset(stack410, 0, sizeof(stack410));
    memset(block2d0, 0, sizeof(block2d0));

    for (uint32_t j = 0; j < 0x13u; ++j) {
        uint32_t w = ((uint32_t)raw76[j*4u]) |
                     ((uint32_t)raw76[j*4u + 1u] << 8) |
                     ((uint32_t)raw76[j*4u + 2u] << 16) |
                     ((uint32_t)raw76[j*4u + 3u] << 24);
        memset(local90, 0, sizeof(local90));
        local90[0]  = 0x05; local90[1] = 0x05;
        local90[2]  = (uint8_t)( w        & 3u);
        local90[3]  = (uint8_t)((w >> 2)  & 3u);
        local90[4]  = (uint8_t)((w >> 4)  & 3u);
        local90[5]  = (uint8_t)((w >> 6)  & 3u);
        local90[6]  = (uint8_t)((w >> 8)  & 3u);
        local90[7]  = (uint8_t)((w >> 10) & 3u);
        local90[8]  = (uint8_t)((w >> 12) & 3u);
        local90[9]  = (uint8_t)((w >> 14) & 3u);
        local90[10] = (uint8_t)((w >> 16) & 3u);
        local90[11] = (uint8_t)((w >> 18) & 3u);
        local90[12] = (uint8_t)((w >> 20) & 3u);
        local90[13] = (uint8_t)((w >> 22) & 3u);
        local90[14] = (uint8_t)((w >> 24) & 3u);
        local90[15] = (uint8_t)((w >> 26) & 3u);

        rc = table_transform_emit(tables, 0x0142u, 0x0100u, local90, local90, tmp1, sizeof(tmp1));
        if (rc) return rc;
        uint8_t local2c0[16] = {0x00,0x06,0x02,0x00,0x04,0x04,0x07,0x01,0x00,0x04,0x03,0x06,0x05,0x03,0x07,0x02};
        rc = table_transform_emit(tables, 0x0744u, 0x0100u, tmp1, tmp1, local90, sizeof(local90));
        if (rc) return rc;
        rc = table_transform_emit(tables, 0x0754u, 0x0100u, k_norm_dat40c6952 + ((j & 7u) * 16u), k_norm_dat40c6952 + ((j & 7u) * 16u), tmp2, sizeof(tmp2));
        if (rc) return rc;
        rc = table_transform_emit(tables, 0x10d2u, 0x0100u, local90, local90, tmp3, sizeof(tmp3));
        if (rc) return rc;
        for (int r = 0; r < 0x1c; ++r) {
            rc = table_transform_emit(tables, 0x0c86u, 0x0100u, tmp2, local2c0, tmp4, sizeof(tmp4));
            if (rc) return rc;
            rc = table_transform_emit(tables, 0x0164u, 0x0100u, k_norm_dat40c8120, tmp4, tmp5, sizeof(tmp5));
            if (rc) return rc;
            rc = table_transform_emit(tables, 0x05b8u, 0x0100u, local90, tmp5, tmp1, sizeof(tmp1));
            if (rc) return rc;
            rc = table_transform_emit(tables, 0x0764u, 0x0100u, tmp3, tmp1, tmp3, sizeof(tmp3));
            if (rc) return rc;
            rc = table_transform_emit(tables, 0x08a2u, 0x0100u, local90, local90, local90, sizeof(local90));
            if (rc) return rc;
            rc = table_transform_emit(tables, 0x05a8u, 0x0100u, local2c0, local2c0, local2c0, sizeof(local2c0));
            if (rc) return rc;
        }
        rc = table_transform_emit(tables, 0x0892u, 0x0100u, tmp3, local90, tmp4, sizeof(tmp4));
        if (rc) return rc;
        rc = table_transform_emit(tables, 0x10f4u, 0x0100u, tmp4, k_norm_dat40c69d2 + ((j & 7u) * 16u), stack410[j], 16u);
        if (rc) return rc;
    }

    rc = table_transform_emit(tables, 0x12bcu, 0x0100u, stack410[0], stack410[0], local90, sizeof(local90));
    if (rc) return rc;
    for (uint32_t j = 1; j < 0x13u; ++j) {
        rc = table_transform_emit(tables, 0x12ccu, 0x0100u, stack410[j], stack410[j], block2d0[j], 16u);
        if (rc) return rc;
    }
    /* Native packing at f3efe594..f3efe5cc:
       - copy all 16 bytes of the 0x12bc result;
       - for each of the 18 0x12cc results in ascending source order, copy the
         14-byte window beginning at byte 2.
       The native loop writes 268 bytes, while the final 0x10a0 transform
       consumes the first 266 bytes. */
    uint8_t packed[16u + 18u * 14u];
    memcpy(packed, local90, 16u);
    size_t pos = 16u;
    for (uint32_t j = 1; j < 0x13u; ++j) {
        memcpy(packed + pos, block2d0[j] + 2u, 14u);
        pos += 14u;
    }
    return table_transform_emit(tables, 0x0f42u, 0x10a0u, packed, packed, out_state10a, L3_GENERATED_FRAME_STATE_LEN);
}


