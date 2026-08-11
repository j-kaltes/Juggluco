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
#include "authorization_shared_point_tail.h"

#include <string.h>

static uint32_t load_le32(const uint8_t *p) {
    return ((uint32_t)p[0]) | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void store_le32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

int l3_authorization_transform_shared_point_tail(const uint8_t raw40[L3_DERIVED_POINT_TAIL_TRANSFORM_RAW_LEN],
                             uint8_t out76[L3_DERIVED_POINT_TAIL_TRANSFORM_OUTPUT_LEN]) {
    if (!raw40 || !out76) return L3_DERIVED_POINT_TAIL_TRANSFORM_ERR_ARGUMENT;
    memset(out76, 0, L3_DERIVED_POINT_TAIL_TRANSFORM_OUTPUT_LEN);

    /* FUN_f3fde360:
       VectorMultiplyAccumulate(local_f0, {5ecdf547,1691081f,29582c0b,126b1e89}, 4, 0)
       VectorMultiplyAccumulate(local_e0, {62d06051,1cd4cda7,add6f7a9,6e9827a7}, 4, 0)
       scalar low words and fixed native constants are then written into the sparse 0x4c-byte state. */
    static const uint32_t k_f0[4] = {0x5ecdf547u, 0x1691081fu, 0x29582c0bu, 0x126b1e89u};
    static const uint32_t k_e0[4] = {0x62d06051u, 0x1cd4cda7u, 0xadd6f7a9u, 0x6e9827a7u};
    static const uint32_t add_f0[4] = {0x6cb892c6u, 0x2655db8cu, 0xe3a26504u, 0xbe943ae0u};
    static const uint32_t add_e0[4] = {0xb90a28d7u, 0xa75b3f81u, 0x8f094828u, 0xee4dac2du};

    for (unsigned i = 0; i < 4; ++i) {
        uint32_t x = load_le32(raw40 + i * 4u);
        store_le32(out76 + i * 4u, x * k_f0[i] + add_f0[i]);
    }
    for (unsigned i = 0; i < 4; ++i) {
        uint32_t x = load_le32(raw40 + 0x10u + i * 4u);
        store_le32(out76 + 0x10u + i * 4u, x * k_e0[i] + add_e0[i]);
    }

    uint32_t local_d0 = load_le32(raw40 + 0x20u);
    uint32_t local_cc = load_le32(raw40 + 0x24u);
    store_le32(out76 + 0x20u, local_d0 * 0x5ecdf547u + 0x6cb892c6u);
    store_le32(out76 + 0x24u, local_cc * 0x1691081fu + 0x2655db8cu);

    store_le32(out76 + 0x28u, 0xf0286943u);
    store_le32(out76 + 0x2cu, 0x9ec02b72u);
    store_le32(out76 + 0x30u, 0x17674844u);
    store_le32(out76 + 0x34u, 0x8b0fd3ccu);
    store_le32(out76 + 0x38u, 0x2f9ee4c1u);
    store_le32(out76 + 0x3cu, 0xba423bc1u);
    store_le32(out76 + 0x40u, 0x0586d6e2u);
    store_le32(out76 + 0x44u, 0x366cac20u);
    store_le32(out76 + 0x48u, 0xf0286943u);
    return L3_DERIVED_POINT_TAIL_TRANSFORM_OK;
}


