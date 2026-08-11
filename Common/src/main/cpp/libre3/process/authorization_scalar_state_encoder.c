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
#include "authorization_scalar_state_encoder.h"

#include <string.h>

static const uint8_t k_f40c6732[144] = {
    0x01,0x01,0x02,0x02,0x05,0x05,0x01,0x02,0x07,0x06,0x05,0x04,0x05,0x05,0x06,0x06,
    0x02,0x04,0x00,0x06,0x07,0x01,0x04,0x07,0x03,0x02,0x04,0x07,0x05,0x04,0x07,0x02,
    0x06,0x05,0x06,0x01,0x07,0x06,0x02,0x06,0x06,0x02,0x05,0x06,0x04,0x04,0x02,0x00,
    0x02,0x06,0x03,0x06,0x01,0x00,0x01,0x05,0x02,0x06,0x01,0x00,0x04,0x05,0x01,0x01,
    0x03,0x07,0x06,0x06,0x02,0x02,0x06,0x03,0x05,0x00,0x07,0x03,0x02,0x04,0x02,0x05,
    0x07,0x06,0x00,0x05,0x01,0x01,0x07,0x04,0x02,0x04,0x02,0x05,0x03,0x00,0x04,0x00,
    0x00,0x07,0x06,0x05,0x03,0x01,0x00,0x01,0x01,0x05,0x02,0x01,0x07,0x06,0x02,0x03,
    0x06,0x07,0x05,0x07,0x06,0x02,0x05,0x07,0x00,0x00,0x05,0x04,0x07,0x00,0x01,0x00,
    0x07,0x04,0x04,0x05,0x07,0x07,0x06,0x00,0x04,0x06,0x06,0x03,0x05,0x02,0x00,0x06
};

static const uint8_t k_f40c67c2[144] = {
    0x05,0x04,0x03,0x07,0x06,0x03,0x01,0x03,0x05,0x00,0x02,0x01,0x04,0x02,0x00,0x00,
    0x02,0x04,0x06,0x05,0x03,0x05,0x06,0x01,0x00,0x01,0x00,0x03,0x03,0x04,0x01,0x00,
    0x00,0x03,0x03,0x06,0x06,0x06,0x02,0x01,0x02,0x04,0x00,0x01,0x07,0x03,0x07,0x06,
    0x07,0x06,0x04,0x07,0x06,0x03,0x01,0x00,0x04,0x00,0x00,0x01,0x06,0x00,0x00,0x03,
    0x04,0x02,0x03,0x02,0x07,0x06,0x06,0x05,0x07,0x03,0x01,0x00,0x00,0x01,0x01,0x07,
    0x03,0x05,0x04,0x05,0x01,0x02,0x02,0x04,0x03,0x02,0x06,0x02,0x07,0x03,0x03,0x06,
    0x01,0x07,0x05,0x02,0x03,0x04,0x06,0x05,0x05,0x00,0x05,0x06,0x03,0x07,0x02,0x02,
    0x01,0x06,0x01,0x03,0x06,0x01,0x04,0x01,0x03,0x02,0x01,0x04,0x01,0x06,0x05,0x00,
    0x03,0x00,0x02,0x05,0x03,0x03,0x01,0x02,0x02,0x07,0x03,0x03,0x01,0x07,0x07,0x02
};

static const uint8_t k_f40c7efa[18] = {
    0x05,0x01,0x06,0x01,0x02,0x05,0x07,0x01,0x00,0x06,0x00,0x06,0x02,0x02,0x00,0x03,0x06,0x00
};

static const uint8_t k_local58_seed[18] = {
    0x02,0x04,0x03,0x05,0x06,0x00,0x04,0x06,0x02,0x05,0x03,0x06,0x01,0x02,0x02,0x00,0x04,0x03
};

static int check(const l3_authorization_scalar_state_encoder_tables *t) {
    if (!t || !t->dat_f4295564 || !t->dat_f40c6a52) return L3_AUTH_CONSTRUCTOR_STATE_ERR_ARGUMENT;
    if (t->dat_f4295564_len < 0x20000u || t->dat_f40c6a52_len < 0x1300u) return L3_AUTH_CONSTRUCTOR_STATE_ERR_TABLE;
    return L3_AUTH_CONSTRUCTOR_STATE_OK;
}

static int table_transform_emit(const l3_authorization_scalar_state_encoder_tables *t,
                            uint32_t param1, uint32_t param2,
                            const uint8_t *p3, const uint8_t *p4,
                            uint8_t *out, size_t out_len) {
    uint32_t whole = ((param2 << 14) & 0xffffffffu) >> 18;
    uint32_t rem = param2 >> 18;
    uint32_t base = param1 & 0x3fffffu;
    uint8_t state = 0;
    if (!p3 || !p4 || !out) return L3_AUTH_CONSTRUCTOR_STATE_ERR_ARGUMENT;
    if (out_len < (size_t)whole + rem || base + whole + rem > t->dat_f40c6a52_len) return L3_AUTH_CONSTRUCTOR_STATE_ERR_TABLE;
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
    return L3_AUTH_CONSTRUCTOR_STATE_OK;
}

static int table_transform_skip_emit(const l3_authorization_scalar_state_encoder_tables *t,
                            uint32_t param1, uint32_t param2,
                            const uint8_t *p3, const uint8_t *p4,
                            uint8_t *out, size_t out_len) {
    uint32_t whole = ((param2 << 14) & 0xffffffffu) >> 18;
    uint32_t rem = param2 >> 18;
    uint32_t skip = (param1 >> 22) | ((param2 & 0xfu) << 10);
    uint32_t base = param1 & 0x3fffffu;
    uint8_t state = 0;
    if (!p3 || !p4 || !out) return L3_AUTH_CONSTRUCTOR_STATE_ERR_ARGUMENT;
    if (out_len < (size_t)whole + rem || base + skip + whole + rem > t->dat_f40c6a52_len) return L3_AUTH_CONSTRUCTOR_STATE_ERR_TABLE;
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
        uint32_t idx = (uint32_t)(state & 0xf8u) ^
                       ((uint32_t)t->dat_f40c6a52[base + skip + whole + i] << 11);
        state = t->dat_f4295564[idx];
        out[whole + i] = (uint8_t)(state & 7u);
    }
    return L3_AUTH_CONSTRUCTOR_STATE_OK;
}

static uint32_t pack_word_from_local2e0(const uint8_t local2e0[18]) {
    uint8_t b[4] = {0,0,0,0};
    uint32_t uvar1 = 0;
    uint32_t uvar2 = 0;
    do {
        uint32_t shift = uvar1 & 6u;
        uvar1 += 2u;
        b[uvar2 >> 2] = (uint8_t)(b[uvar2 >> 2] | ((local2e0[uvar2 + 2u] & 3u) << shift));
        uvar2 += 1u;
    } while (uvar1 != 0x20u);
    return ((uint32_t)b[0]) | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
}

int l3_authorization_encode_scalar_state_words19(const l3_authorization_scalar_state_encoder_tables *tables,
                                  const uint8_t in10c[L3_AUTH_CONSTRUCTOR_STATE_INPUT_LEN],
                                  uint32_t out_words[L3_AUTH_CONSTRUCTOR_STATE_WORDS]) {
    int rc = check(tables);
    if (rc) return rc;
    if (!in10c || !out_words) return L3_AUTH_CONSTRUCTOR_STATE_ERR_ARGUMENT;

    uint8_t state268[L3_AUTH_CONSTRUCTOR_STATE_INPUT_LEN];
    memset(state268, 0, sizeof(state268));
    rc = table_transform_skip_emit(tables, 0x0b06u, 0x0810a0u, in10c, in10c, state268, sizeof(state268));
    if (rc) return rc;

    uint8_t stack_trans[19u * 16u + 18u];
    memset(stack_trans, 0, sizeof(stack_trans));
    for (unsigned i = 0; i < L3_AUTH_CONSTRUCTOR_STATE_WORDS; ++i) {
        rc = table_transform_emit(tables, 0x10b0u, 0x0100u,
                              state268 + i * 14u, state268 + i * 14u,
                              stack_trans + i * 16u, 16u);
        if (rc) return rc;
    }

    for (unsigned round = 0; round < L3_AUTH_CONSTRUCTOR_STATE_WORDS; ++round) {
        uint8_t local2e0[18];
        uint8_t local1b0[18];
        uint8_t local6a[18];
        uint8_t local7c[18];
        uint8_t local8e[18];
        uint8_t locala0[18];
        uint8_t local58[18];
        unsigned selector = (round & 7u) | ((round & 7u) << 3u);
        size_t coff = (size_t)selector * 2u;

        memcpy(local58, k_local58_seed, sizeof(local58));
        rc = table_transform_emit(tables, 0x1258u, 0x0120u,
                              k_f40c6732 + coff, k_f40c6732 + coff,
                              local2e0, sizeof(local2e0));
        if (rc) return rc;
        memcpy(stack_trans + 19u * 16u, local2e0, sizeof(local2e0));

        rc = table_transform_skip_emit(tables, 0x0152u, 0x080100u,
                              stack_trans + round * 16u, stack_trans + round * 16u,
                              local1b0, sizeof(local1b0));
        if (rc) return rc;
        rc = table_transform_emit(tables, 0x0c12u, 0x0120u, local2e0, local2e0, local6a, sizeof(local6a));
        if (rc) return rc;

        for (unsigned iter = 0; iter < 0x1cu; ++iter) {
            rc = table_transform_emit(tables, 0x0732u, 0x0120u, local1b0, local58, local7c, sizeof(local7c));
            if (rc) return rc;
            rc = table_transform_emit(tables, 0x028eu, 0x0120u, k_f40c7efa, local7c, local8e, sizeof(local8e));
            if (rc) return rc;
            rc = table_transform_emit(tables, 0x10c0u, 0x0120u, local2e0, local8e, locala0, sizeof(locala0));
            if (rc) return rc;
            rc = table_transform_emit(tables, 0x1104u, 0x0120u, local6a, locala0, local6a, sizeof(local6a));
            if (rc) return rc;
            rc = table_transform_emit(tables, 0x070eu, 0x0120u, local2e0, local2e0, local2e0, sizeof(local2e0));
            if (rc) return rc;
            rc = table_transform_emit(tables, 0x10e2u, 0x0120u, local58, local58, local58, sizeof(local58));
            if (rc) return rc;
        }

        rc = table_transform_emit(tables, 0x0720u, 0x0120u, local6a, local2e0, local7c, sizeof(local7c));
        if (rc) return rc;
        rc = table_transform_emit(tables, 0x0444u, 0x0120u, local7c, k_f40c67c2 + coff, local1b0, sizeof(local1b0));
        if (rc) return rc;
        rc = table_transform_emit(tables, 0x109eu, 0x0120u, local1b0, local1b0, local2e0, sizeof(local2e0));
        if (rc) return rc;

        out_words[round] = pack_word_from_local2e0(local2e0);
    }

    return L3_AUTH_CONSTRUCTOR_STATE_OK;
}

int l3_authorization_encode_scalar_state_bytes76(const l3_authorization_scalar_state_encoder_tables *tables,
                                  const uint8_t in10c[L3_AUTH_CONSTRUCTOR_STATE_INPUT_LEN],
                                  uint8_t out76[L3_AUTH_CONSTRUCTOR_STATE_OUTPUT_LEN]) {
    uint32_t w[L3_AUTH_CONSTRUCTOR_STATE_WORDS];
    int rc = l3_authorization_encode_scalar_state_words19(tables, in10c, w);
    if (rc) return rc;
    if (!out76) return L3_AUTH_CONSTRUCTOR_STATE_ERR_ARGUMENT;
    for (unsigned i = 0; i < L3_AUTH_CONSTRUCTOR_STATE_WORDS; ++i) {
        out76[i * 4u + 0u] = (uint8_t)(w[i] & 0xffu);
        out76[i * 4u + 1u] = (uint8_t)((w[i] >> 8) & 0xffu);
        out76[i * 4u + 2u] = (uint8_t)((w[i] >> 16) & 0xffu);
        out76[i * 4u + 3u] = (uint8_t)((w[i] >> 24) & 0xffu);
    }
    return L3_AUTH_CONSTRUCTOR_STATE_OK;
}
