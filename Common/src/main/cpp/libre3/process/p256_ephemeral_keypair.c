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
#include "p256_ephemeral_keypair.h"
#include "p256_ephemeral_keypair_constants.inc"

#include <string.h>

#define BAR5_CONST_BASE 0xf4137c47u
#define BAR5_OFF(addr) ((size_t)((addr##u) - BAR5_CONST_BASE))

static uint32_t load32le(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static void store32le(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}

/* FUN_f3f674c4. All selector-5 invocations use only the first encoded length,
 * but retain the native two-length decoding for completeness. */
static int bar5_674c4(const uint8_t *dat429, size_t dat429_len,
                      uint32_t param1, uint32_t param2,
                      const uint8_t *a, const uint8_t *b, uint8_t *out) {
    const uint32_t n1 = ((param2 << 14) >> 18);
    const uint32_t n2 = param2 >> 18;
    const size_t coff = (size_t)(param1 & 0x3fffffu);
    uint32_t u = 0;
    if (!dat429 || dat429_len < 0x20000u || !b || !out ||
        coff + (size_t)n1 + (size_t)n2 > sizeof(k_bar5_generator_const_4137c47) ||
        (n1 && !a)) return -1;
    for (uint32_t i = 0; i < n1; ++i) {
        const uint32_t idx = (((u & 0xf8u) ^ a[i]) | ((uint32_t)b[i] << 8)) ^
                             ((uint32_t)k_bar5_generator_const_4137c47[coff + i] << 11);
        if (idx >= dat429_len) return -1;
        u = dat429[idx]; out[i] = (uint8_t)(u & 7u);
    }
    for (uint32_t i = 0; i < n2; ++i) {
        const uint32_t idx = ((u & 0xf8u) | ((uint32_t)b[n1 + i] << 8)) ^
                             ((uint32_t)k_bar5_generator_const_4137c47[coff + n1 + i] << 11);
        if (idx >= dat429_len) return -1;
        u = dat429[idx]; out[n1 + i] = (uint8_t)(u & 7u);
    }
    return 0;
}

/* FUN_f3f675b0. */
static int bar5_675b0(const uint8_t *dat429, size_t dat429_len,
                      const uint8_t a[16], const uint8_t b[16], uint8_t out[18]) {
    const size_t coff = BAR5_OFF(0xf4137d99);
    uint32_t u = 0;
    if (!dat429 || dat429_len < 0x20000u || !a || !b || !out) return -1;
    for (uint32_t i = 0; i < 16; ++i) {
        const uint32_t idx = (((u & 0xf8u) ^ a[i]) | ((uint32_t)b[i] << 8)) ^
                             ((uint32_t)k_bar5_generator_const_4137c47[coff + i] << 11);
        if (idx >= dat429_len) return -1;
        u = dat429[idx]; out[i] = (uint8_t)(u & 7u);
    }
    {
        const uint32_t idx16 = (u & 0xf8u) | 0x1b800u;
        if (idx16 >= dat429_len) return -1;
        const uint8_t v = dat429[idx16];
        const uint32_t idx17 = ((uint32_t)v & 0xf8u) | 0x6000u;
        if (idx17 >= dat429_len) return -1;
        out[16] = (uint8_t)(v & 7u);
        out[17] = (uint8_t)(dat429[idx17] & 7u);
    }
    return 0;
}

static int bar5_one_value(const uint8_t *dat429, size_t dat429_len,
                          const uint8_t block16[16], unsigned index,
                          uint32_t *value_out) {
    uint8_t l1e0[18], l140[18], a7a[18], a8c[18], a9e[18], ab0[18], l68[18];
    const unsigned m = index & 7u;
    const size_t e4b = BAR5_OFF(0xf4137e4b) + (size_t)m * 18u;
    const size_t edb = BAR5_OFF(0xf4137edb) + (size_t)m * 18u;
    const size_t f82 = BAR5_OFF(0xf4137f82);
    static const uint8_t init68[18] = {
        0x02,0x07,0x06,0x05,0x07,0x02,0x00,0x03,
        0x03,0x07,0x02,0x01,0x01,0x01,0x00,0x06,
        0x01,0x00
    };
    if (!value_out || e4b + 18u > sizeof(k_bar5_generator_const_4137c47) ||
        edb + 18u > sizeof(k_bar5_generator_const_4137c47) ||
        f82 + 18u > sizeof(k_bar5_generator_const_4137c47)) return -1;
    if (bar5_674c4(dat429, dat429_len, 0x19au, 0x120u,
                   k_bar5_generator_const_4137c47 + e4b,
                   k_bar5_generator_const_4137c47 + e4b, l1e0) != 0 ||
        bar5_675b0(dat429, dat429_len, block16, block16, l140) != 0 ||
        bar5_674c4(dat429, dat429_len, 0x164u, 0x120u, l1e0, l1e0, a7a) != 0)
        return -1;
    memcpy(l68, init68, sizeof(l68));
    for (unsigned round = 0; round < 0x1cu; ++round) {
        if (bar5_674c4(dat429, dat429_len, 0x12u, 0x120u, l140, l68, a8c) != 0 ||
            bar5_674c4(dat429, dat429_len, 0u, 0x120u,
                       k_bar5_generator_const_4137c47 + f82, a8c, a9e) != 0 ||
            bar5_674c4(dat429, dat429_len, 0x24u, 0x120u, l1e0, a9e, ab0) != 0 ||
            bar5_674c4(dat429, dat429_len, 0x188u, 0x120u, a7a, ab0, a7a) != 0 ||
            bar5_674c4(dat429, dat429_len, 0x176u, 0x120u, l1e0, l1e0, l1e0) != 0 ||
            bar5_674c4(dat429, dat429_len, 0x1beu, 0x120u, l68, l68, l68) != 0)
            return -1;
    }
    if (bar5_674c4(dat429, dat429_len, 0x1acu, 0x120u, a7a, l1e0, a8c) != 0 ||
        bar5_674c4(dat429, dat429_len, 0x1f2u, 0x120u, a8c,
                   k_bar5_generator_const_4137c47 + edb, l140) != 0 ||
        bar5_674c4(dat429, dat429_len, 0x1d0u, 0x120u, l140, l140, l1e0) != 0)
        return -1;

    /* Native clears the low word of local_68, masks bStack_1cf (l1e0[17])
     * to two bits, then packs low two bits of l1e0[2..17] into that word. */
    l1e0[17] &= 3u;
    memset(l68, 0, 4u);
    for (unsigned j = 0; j < 16u; ++j) {
        l68[j >> 2] |= (uint8_t)((l1e0[j + 2u] & 3u) << ((2u * j) & 6u));
    }
    *value_out = load32le(l68);
    return 0;
}

int l3_p256_generate_ephemeral_keypair_state(const uint8_t *dat429, size_t dat429_len,
                                    const uint8_t context[0x1eb0],
                                    const uint8_t entropy142[142],
                                    uint8_t state76[76]) {
    uint8_t entropy[142], tmp142[142], src142[142], blocks[10][16];
    uint32_t v[10];
    if (!dat429 || dat429_len < 0x20000u || !context || !entropy142 || !state76)
        return -1;
    for (unsigned i = 0; i < 142u; ++i) entropy[i] = (uint8_t)(entropy142[i] & 7u);
    if (bar5_674c4(dat429, dat429_len, 0xc4u, 0x8e0u,
                   entropy, context + 0x1af0u, tmp142) != 0 ||
        bar5_674c4(dat429, dat429_len, 0x36u, 0x8e0u,
                   tmp142, tmp142, src142) != 0)
        return -1;
    for (unsigned i = 0; i < 10u; ++i) {
        const uint8_t *window = src142 + i * 14u;
        if (bar5_674c4(dat429, dat429_len, 0x1e2u, 0x100u,
                       window, window, blocks[i]) != 0 ||
            bar5_one_value(dat429, dat429_len, blocks[i], i, &v[i]) != 0)
            return -1;
    }

    /* Final affine materialization from FUN_f3f64bc0. C unsigned arithmetic
     * intentionally provides the native 32-bit wraparound semantics. */
    store32le(state76 +  0, v[0] * 0x10c4ca57u + 0xa1cad6aeu);
    store32le(state76 +  4, v[1] * 0xb7fd5ca1u + 0xaa7c3497u);
    store32le(state76 +  8, v[2] * 0x9381cab9u + 0xc574049du);
    store32le(state76 + 12, v[3] * 0x811bc361u + 0x5f447c72u);
    store32le(state76 + 16, v[4] * 0xac33dba3u + 0x21bcb2f6u);
    store32le(state76 + 20, v[5] * 0x56fa5383u + 0x90937437u);
    store32le(state76 + 24, v[6] * 0x160b43b9u + 0x16521bcau);
    store32le(state76 + 28, v[7] * 0x1b745c7du + 0xf0bd9abcu);
    store32le(state76 + 32, v[8] * 0x10c4ca57u + 0xa1cad6aeu);
    store32le(state76 + 36, v[9] * 0xb7fd5ca1u + 0xaa7c3497u);
    store32le(state76 + 40, 0x2272625au);
    store32le(state76 + 44, 0x671d7687u);
    store32le(state76 + 48, 0x1519892cu);
    store32le(state76 + 52, 0xc4c0d952u);
    store32le(state76 + 56, 0xf51d5950u);
    store32le(state76 + 60, 0x333c63d4u);
    store32le(state76 + 64, 0xfea49320u);
    store32le(state76 + 68, 0x507a7804u);
    store32le(state76 + 72, 0x2272625au);
    return 0;
}
