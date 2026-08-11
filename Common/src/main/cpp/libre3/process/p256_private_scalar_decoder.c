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
#include "p256_private_scalar_decoder.h"

#include <string.h>

static const uint32_t k_dat_f4190170[8] = {
    0x3c956e9bu, 0x65ddcec7u, 0x920a7371u, 0xd713ca63u,
    0x1f20e9e5u, 0x9f513ab7u, 0x43f84b8fu, 0x608f118du
};

static const uint32_t k_dat_f4190190[8] = {
    0x629d96d2u, 0x7fc32099u, 0xcd04ede8u, 0x9ea380a5u,
    0x71a61579u, 0xd1458d0bu, 0xa1303a09u, 0x18d85f7bu
};

static const uint32_t k_dat_f41901b0[8] = {
    0x8b03abbbu, 0xb99fc4ffu, 0xd0bf7949u, 0x10d53bb3u,
    0xe0fb8bc5u, 0x20f1b88fu, 0x0a6a1fb7u, 0x7475c45du
};

static const uint32_t k_dat_f41901d0[8] = {
    0xf02c1c2du, 0x8f23f12cu, 0x3c8b0e6bu, 0x133b9334u,
    0x77635476u, 0xfce8dc6eu, 0x9542fa24u, 0xf891b1e4u
};

static const uint32_t k_dat_f419fef8_pairs[16][2] = {
    {0x268270fdu, 0xa2faf429u}, {0xe500c836u, 0xfbb9a533u},
    {0xa37f1f6fu, 0x4478563eu}, {0x6a065023u, 0x93e8ac65u},
    {0x2884a75cu, 0xeca75d70u}, {0xe702fe95u, 0x35660e7au},
    {0xa58155ceu, 0x8e24bf85u}, {0x63ffad07u, 0xd6e37090u},
    {0x2a86ddbbu, 0x2653c6b7u}, {0xe90534f4u, 0x7f1277c1u},
    {0xa7838c2du, 0xc7d128ccu}, {0x6601e366u, 0x108fd9d7u},
    {0x24803a9fu, 0x694e8ae2u}, {0xeb076b53u, 0xb8bee108u},
    {0xa985c28cu, 0x017d9213u}, {0x680419c5u, 0x5a3c431eu}
};

static const uint32_t k_dat_f419ff78_pairs[16][2] = {
    {0x403158aau, 0x06adf370u}, {0x898f381eu, 0x584f89cdu},
    {0xae077575u, 0xa791b9d1u}, {0xd27fb2ccu, 0xf6d3e9d5u},
    {0x1bdd9240u, 0x48758033u}, {0x4055cf97u, 0x97b7b037u},
    {0x64ce0ceeu, 0xe6f9e03bu}, {0x89464a45u, 0x363c103fu},
    {0xd2a429b9u, 0x87dda69cu}, {0xf71c6710u, 0xd71fd6a0u},
    {0x1b94a467u, 0x266206a5u}, {0x64f283dbu, 0x78039d02u},
    {0x896ac132u, 0xc745cd06u}, {0xade2fe89u, 0x1687fd0au},
    {0xf740ddfdu, 0x68299367u}, {0x1bb91b54u, 0xb76bc36cu}
};


static const uint32_t k_dat_f419fff8_pairs[16][2] = {
    {0xeea35a3bu, 0x8cfec539u}, {0xa0791b0au, 0x34a19cbdu},
    {0x524edbd9u, 0xec447441u}, {0xfa5b4303u, 0x9082ce5bu},
    {0xac3103d2u, 0x4825a5dfu}, {0x5e06c4a1u, 0xffc87d63u},
    {0x0fdc8570u, 0xa76b54e7u}, {0xc1b2463fu, 0x5f0e2c6au},
    {0x69bead69u, 0x034c8685u}, {0x1b946e38u, 0xbaef5e09u},
    {0xcd6a2f07u, 0x6292358cu}, {0x7f3fefd6u, 0x1a350d10u},
    {0x3115b0a5u, 0xc1d7e494u}, {0xe2eb7174u, 0x797abc17u},
    {0x8af7d89eu, 0x2db91632u}, {0x3ccd996du, 0xd55bedb6u}
};


static const uint32_t k_unk_f41900b0[8] = {
    0x28969317u, 0x6f349b5du, 0x651d01edu, 0xbad84117u,
    0x2df8d5f7u, 0xcbe4a8d7u, 0xedc59a2fu, 0x4adc2e9bu
};

static const uint32_t k_unk_f41900d0[8] = {
    0xbe321e6bu, 0x60489425u, 0x30d64f2au, 0x1525b06bu,
    0x7ccb2516u, 0x690d7a2au, 0x26d70e01u, 0xdb26e86cu
};

static const uint32_t k_unk_f41900f0[8] = {
    0x7b713b13u, 0x254d7d7fu, 0xca1e0957u, 0x8ae3ff69u,
    0x2c5d0543u, 0xac56ae8du, 0xfa64514fu, 0x7031252bu
};

static const uint32_t k_unk_f4190110[8] = {
    0x0857ed3eu, 0x5172c70bu, 0xb9ac0103u, 0x60622a11u,
    0xdb37e111u, 0x97fc8baeu, 0x725daf3fu, 0x54b6a991u
};

static const uint32_t k_dat_f4190130[8] = {
    0xde0844d7u, 0x9a9d00ebu, 0x989eefc1u, 0x1c41f46fu,
    0xb98297ffu, 0x1d325757u, 0xbbfd27bbu, 0x20d331e9u
};

static const uint32_t k_dat_f4190150[8] = {
    0x6173116fu, 0xa769484du, 0xe3f7f2a7u, 0x81b35d73u,
    0xe084948eu, 0x0705da90u, 0x807e1641u, 0x50fa865fu
};


static const uint32_t k_dat_f41a00f8_pairs[16][2] = {
    {0x46abe3f4u, 0xe90796f6u}, {0x34aa8d0du, 0xf64681e4u},
    {0x22a93626u, 0x03856cd2u}, {0x10a7df3fu, 0x10c457c0u},
    {0x095857d3u, 0x275e5db9u}, {0xf75700ecu, 0x349d48a6u},
    {0xe555aa05u, 0x41dc3394u}, {0xd354531eu, 0x5f1b1e82u},
    {0xc152fc37u, 0x6c5a0970u}, {0xba0374cbu, 0x72f40f69u},
    {0xa8021de4u, 0x8032fa57u}, {0x9600c6fdu, 0x9d71e545u},
    {0x83ff7016u, 0xaab0d033u}, {0x71fe192fu, 0xb7efbb21u},
    {0x6aae91c3u, 0xce89c11au}, {0x58ad3adcu, 0xdbc8ac08u}
};

static const uint32_t k_dat_f41a0178_pairs[16][2] = {
    {0xebb99e0bu, 0xa9ae0d4cu}, {0x0bc4cfb8u, 0x9e21f806u},
    {0x2bd00165u, 0x8295e2bfu}, {0x4bdb3312u, 0x7709cd78u},
    {0x6be664bfu, 0x6b7db831u}, {0xe10b3827u, 0x5d8769b9u},
    {0x011669d4u, 0x41fb5473u}, {0x21219b81u, 0x366f3f2cu},
    {0x412ccd2eu, 0x2ae329e5u}, {0x6137fedbu, 0x1f57149eu},
    {0x81433088u, 0x03caff57u}, {0xf66803f0u, 0xf5d4b0dfu},
    {0x1673359du, 0xea489b99u}, {0x367e674au, 0xdebc8652u},
    {0x568998f7u, 0xc330710bu}, {0x7694caa4u, 0xb7a45bc4u}
};

static const uint32_t k_dat_f41a01f8_pairs[16][2] = {
    {0x7f78b8aau, 0x91ba2908u}, {0x1e7d9aceu, 0x6ceecb5eu},
    {0xbd827cf2u, 0x38236db3u}, {0x5c875f16u, 0x03581009u},
    {0xfb8c413au, 0xde8cb25eu}, {0x9a91235eu, 0xa9c154b4u},
    {0x39960582u, 0x74f5f70au}, {0xd89ae7a6u, 0x402a995fu},
    {0x779fc9cau, 0x1b5f3bb5u}, {0x16a4abeeu, 0xe693de0bu},
    {0xb5a98e12u, 0xb1c88060u}, {0x54ae7036u, 0x8cfd22b6u},
    {0x0365301bu, 0x54e79fb2u}, {0xa26a123fu, 0x201c4207u},
    {0x416ef463u, 0xfb50e45du}, {0xe073d687u, 0xc68586b2u}
};

static const uint32_t k_dat_f41a0078_pairs[16][2] = {
    {0x0910cb8du, 0x0f353b2du}, {0x1c0f4086u, 0xf5047a37u},
    {0x2f0db57fu, 0xead3b941u}, {0x387780f5u, 0xd13efb5eu},
    {0x4b75f5eeu, 0xc70e3a68u}, {0x5e746ae7u, 0xbcdd7972u},
    {0x67de365du, 0xa348bb8fu}, {0x7adcab56u, 0x9917fa99u},
    {0x8ddb204fu, 0x8ee739a3u}, {0x9744ebc5u, 0x75527bc0u},
    {0xaa4360beu, 0x6b21bacau}, {0xbd41d5b7u, 0x50f0f9d4u},
    {0xd0404ab0u, 0x46c038deu}, {0xd9aa1626u, 0x3d2b7afbu},
    {0xeca88b1fu, 0x22faba05u}, {0xffa70018u, 0x18c9f90fu}
};

static uint32_t carry4(uint32_t a, uint32_t b) {
    return (uint32_t)(a + b < a);
}

static uint32_t hi32(uint64_t x) {
    return (uint32_t)(x >> 32);
}

static void nibble_ladder_f41a01f8_step(uint32_t *lo, uint32_t *hi) {
    uint32_t old_lo = *lo;
    uint32_t old_hi = *hi;
    unsigned nib = old_lo & 0x0fu;
    uint32_t shifted = (old_lo >> 4) | (old_hi << 28);
    uint32_t nlo = shifted + k_dat_f41a01f8_pairs[nib][0];
    uint32_t nhi = (old_hi >> 4) + k_dat_f41a01f8_pairs[nib][1] + carry4(shifted, k_dat_f41a01f8_pairs[nib][0]);
    *lo = nlo;
    *hi = nhi;
}

static void nibble_ladder_f41a0078_step(uint32_t *lo, uint32_t *hi) {
    uint32_t old_lo = *lo;
    uint32_t old_hi = *hi;
    unsigned nib = old_lo & 0x0fu;
    uint32_t shifted = (old_lo >> 4) | (old_hi << 28);
    uint32_t nlo = shifted + k_dat_f41a0078_pairs[nib][0];
    uint32_t nhi = (old_hi >> 4) + k_dat_f41a0078_pairs[nib][1] +
                   carry4(shifted, k_dat_f41a0078_pairs[nib][0]);
    *lo = nlo;
    *hi = nhi;
}

static void nibble_ladder_f41a0078_run6_then_peek7(uint32_t *lo, uint32_t *hi,
                                                    uint32_t *peek7_lo,
                                                    uint32_t *peek7_hi) {
    for (unsigned i = 0; i < 6u; ++i) {
        nibble_ladder_f41a0078_step(lo, hi);
    }
    unsigned nib = *lo & 0x0fu;
    uint32_t shifted = (*lo >> 4) | (*hi << 28);
    if (peek7_lo) *peek7_lo = shifted + k_dat_f41a0078_pairs[nib][0];
    if (peek7_hi) {
        *peek7_hi = (*hi >> 4) + k_dat_f41a0078_pairs[nib][1] +
                    carry4(shifted, k_dat_f41a0078_pairs[nib][0]);
    }
}

static void nibble_ladder_pair_step(const uint32_t table[16][2], uint32_t *lo, uint32_t *hi) {
    uint32_t old_lo = *lo;
    uint32_t old_hi = *hi;
    unsigned nib = old_lo & 0x0fu;
    uint32_t shifted = (old_lo >> 4) | (old_hi << 28);
    uint32_t nlo = shifted + table[nib][0];
    uint32_t nhi = (old_hi >> 4) + table[nib][1] + carry4(shifted, table[nib][0]);
    *lo = nlo;
    *hi = nhi;
}

static void nibble_ladder_pair_run8(const uint32_t table[16][2], uint32_t *lo, uint32_t *hi) {
    for (unsigned i = 0; i < 8u; ++i) {
        nibble_ladder_pair_step(table, lo, hi);
    }
}
static void nibble_ladder_pair_run6_then_peek7(const uint32_t table[16][2],
                                                   uint32_t *lo, uint32_t *hi,
                                                   uint32_t *peek7_lo,
                                                   uint32_t *peek7_hi) {
    for (unsigned i = 0; i < 6u; ++i) {
        nibble_ladder_pair_step(table, lo, hi);
    }
    unsigned nib = *lo & 0x0fu;
    uint32_t shifted = (*lo >> 4) | (*hi << 28);
    uint32_t tlo = table[nib][0];
    if (peek7_lo) *peek7_lo = shifted + tlo;
    if (peek7_hi) {
        *peek7_hi = (*hi >> 4) + table[nib][1] + carry4(shifted, tlo);
    }
}

static uint32_t nibble_ladder_pair_run6_step7_step8lo_token(const uint32_t table[16][2],
                                                             uint32_t lo,
                                                             uint32_t hi) {
    for (unsigned i = 0; i < 6u; ++i) {
        nibble_ladder_pair_step(table, &lo, &hi);
    }
    unsigned nib7 = lo & 0x0fu;
    uint32_t shifted7 = (lo >> 4) | (hi << 28);
    uint32_t step7_lo = shifted7 + table[nib7][0];
    uint32_t step7_hi = (hi >> 4) + table[nib7][1] + carry4(shifted7, table[nib7][0]);

    unsigned nib8 = step7_lo & 0x0fu;
    uint32_t step8_lo = ((step7_lo >> 4) | (step7_hi << 28)) + table[nib8][0];
    unsigned nib9 = step8_lo & 0x0fu;
    return (step8_lo >> 4) + table[nib9][0];
}


void l3_p256_private_scalar_seed_stream_a_pair(uint32_t x, uint32_t out_pair2[2]) {
    if (!out_pair2) return;
    uint64_t p0 = (uint64_t)x * 0x25ef3857u;
    uint32_t p0lo = (uint32_t)p0;
    uint32_t lo = p0lo + 0x3c0bca07u;
    uint32_t hi = x * (uint32_t)-0x207f55e3 + hi32(p0) + 0x43d56500u +
                  (uint32_t)(0xc3f435f8u < p0lo);
    nibble_ladder_pair_run8(k_dat_f419fef8_pairs, &lo, &hi);

    uint64_t p1 = (uint64_t)x * 0x8823e2f5u;
    uint32_t p1lo = (uint32_t)p1;
    out_pair2[0] = p1lo + 0x9b4ebb31u;
    out_pair2[1] = x * (uint32_t)-0x32a848c4 + hi32(p1) +
                   lo * (uint32_t)-0x0f2c1f93 + 0x3c19d4d5u +
                   (uint32_t)(0x64b144ceu < p1lo);
}

void l3_p256_private_scalar_seed_stream_b_pair(uint32_t x, uint32_t out_pair2[2]) {
    if (!out_pair2) return;
    uint64_t p0 = (uint64_t)x * 0x59c1c01bu;
    uint32_t p0lo = (uint32_t)p0;
    uint32_t lo = p0lo + 0x8f3e4c58u;
    uint32_t hi = x * 0x3b95b5f4u + hi32(p0) + 0x045f926bu +
                  (uint32_t)(0x70c1b3a7u < p0lo);
    nibble_ladder_pair_run8(k_dat_f419ff78_pairs, &lo, &hi);

    uint64_t p1 = (uint64_t)x * 0x766b9a19u;
    uint32_t p1lo = (uint32_t)p1;
    out_pair2[0] = p1lo + 0x847792d3u;
    out_pair2[1] = x * 0x7b15c2a4u + hi32(p1) + lo * 0x61690625u +
                   0x0638015au + (uint32_t)(0x7b886d2cu < p1lo);
}


void l3_p256_private_scalar_coefficient_stream_pair(uint32_t x, uint32_t out_pair2[2]) {
    if (!out_pair2) return;
    uint64_t p0 = (uint64_t)x * 0x25fdd2d5u;
    uint32_t p0lo = (uint32_t)p0;
    uint32_t lo = p0lo + 0x7c74db95u;
    uint32_t hi = x * 0x56e5c152u + hi32(p0) + 0xbb6eeac9u +
                  (uint32_t)(0x838b246au < p0lo);
    nibble_ladder_pair_run8(k_dat_f419fff8_pairs, &lo, &hi);

    uint64_t p1 = (uint64_t)x * 0x859ed869u;
    uint32_t p1lo = (uint32_t)p1;
    out_pair2[0] = p1lo + 0x6fbe959cu;
    out_pair2[1] = x * 0x15f8d6cau + hi32(p1) +
                   lo * (uint32_t)-0x46117145 + 0xa69fb8e0u +
                   (uint32_t)(0x90416a63u < p1lo);
}

void l3_p256_private_scalar_build_coefficient_schedule(
    const uint32_t words10[L3_AUTH_SCALAR_TAIL_FINAL_INPUT_WORDS],
    uint32_t out_pairs20[L3_AUTH_SCALAR_TAIL_LASTLOOP_INPUT_WORDS]) {
    if (!words10 || !out_pairs20) return;
    for (unsigned i = 0; i < L3_AUTH_SCALAR_TAIL_FINAL_INPUT_WORDS; ++i) {
        uint32_t x = (uint32_t)((words10[i] * k_dat_f4190130[i & 7u] +
                                 k_dat_f4190150[i & 7u]) * 0x5b1530a7u +
                                0x61897338u);
        l3_p256_private_scalar_coefficient_stream_pair(x, out_pairs20 + i * 2u);
    }
}


static void add_pair64(uint32_t acc[2], const uint32_t add[2]) {
    uint32_t old = acc[0];
    acc[0] = old + add[0];
    acc[1] = acc[1] + add[1] + (uint32_t)(acc[0] < old);
}

void l3_p256_private_scalar_build_prefix_stream_a(
    const uint32_t words10[L3_AUTH_SCALAR_TAIL_FINAL_INPUT_WORDS],
    uint32_t out_seed_pairs20[L3_AUTH_SCALAR_TAIL_LASTLOOP_INPUT_WORDS],
    uint32_t out_prefix_pairs20[L3_AUTH_SCALAR_TAIL_LASTLOOP_INPUT_WORDS]) {
    if (!words10) return;
    uint32_t acc[2] = {0u, 0u};
    for (unsigned i = 0; i < L3_AUTH_SCALAR_TAIL_FINAL_INPUT_WORDS; ++i) {
        uint32_t x;
        if (i == 0u) {
            x = (uint32_t)(words10[0] * (uint32_t)-0x4cfcc6a5 + 0xaf7f8b46u);
        } else {
            x = (uint32_t)((words10[i] * k_unk_f41900b0[i & 7u] +
                            k_unk_f41900d0[i & 7u]) * 0x7d54c65du +
                           0x99a7bc67u);
        }
        uint32_t pair[2];
        l3_p256_private_scalar_seed_stream_a_pair(x, pair);
        if (out_seed_pairs20) {
            out_seed_pairs20[i * 2u + 0u] = pair[0];
            out_seed_pairs20[i * 2u + 1u] = pair[1];
        }
        add_pair64(acc, pair);
        if (out_prefix_pairs20) {
            out_prefix_pairs20[i * 2u + 0u] = acc[0];
            out_prefix_pairs20[i * 2u + 1u] = acc[1];
        }
    }
}

void l3_p256_private_scalar_build_prefix_stream_b(
    const uint32_t words10[L3_AUTH_SCALAR_TAIL_FINAL_INPUT_WORDS],
    uint32_t out_seed_pairs20[L3_AUTH_SCALAR_TAIL_LASTLOOP_INPUT_WORDS],
    uint32_t out_prefix_pairs20[L3_AUTH_SCALAR_TAIL_LASTLOOP_INPUT_WORDS]) {
    if (!words10) return;
    uint32_t acc[2] = {0u, 0u};
    for (unsigned i = 0; i < L3_AUTH_SCALAR_TAIL_FINAL_INPUT_WORDS; ++i) {
        uint32_t x;
        if (i == 0u) {
            x = (uint32_t)(words10[0] * (uint32_t)-0x3745cc6b + 0x0a875c52u);
        } else {
            x = (uint32_t)((words10[i] * k_unk_f41900f0[i & 7u] +
                            k_unk_f4190110[i & 7u]) * 0x05c143b7u +
                           0xfed78b00u);
        }
        uint32_t pair[2];
        l3_p256_private_scalar_seed_stream_b_pair(x, pair);
        if (out_seed_pairs20) {
            out_seed_pairs20[i * 2u + 0u] = pair[0];
            out_seed_pairs20[i * 2u + 1u] = pair[1];
        }
        add_pair64(acc, pair);
        if (out_prefix_pairs20) {
            out_prefix_pairs20[i * 2u + 0u] = acc[0];
            out_prefix_pairs20[i * 2u + 1u] = acc[1];
        }
    }
}


static void sub_pair64(uint32_t acc[2], const uint32_t sub[2]) {
    uint32_t old = acc[0];
    acc[0] = old - sub[0];
    acc[1] = acc[1] - sub[1] - (uint32_t)(old < sub[0]);
}

static void range_sum_from_prefix10(const uint32_t prefix20[20], unsigned start, unsigned end, uint32_t out[2]) {
    out[0] = prefix20[end * 2u + 0u];
    out[1] = prefix20[end * 2u + 1u];
    if (start != 0u) {
        uint32_t sub[2] = {prefix20[(start - 1u) * 2u + 0u], prefix20[(start - 1u) * 2u + 1u]};
        sub_pair64(out, sub);
    }
}

void l3_p256_private_scalar_convolution_pairs23(
    const uint32_t fef8_seed_pairs20[L3_AUTH_SCALAR_TAIL_LASTLOOP_INPUT_WORDS],
    const uint32_t ff78_seed_pairs20[L3_AUTH_SCALAR_TAIL_LASTLOOP_INPUT_WORDS],
    const uint32_t fef8_prefix_pairs20[L3_AUTH_SCALAR_TAIL_LASTLOOP_INPUT_WORDS],
    const uint32_t ff78_prefix_pairs20[L3_AUTH_SCALAR_TAIL_LASTLOOP_INPUT_WORDS],
    uint32_t out_pairs46[L3_AUTH_SCALAR_TAIL_CONV_PAIRS * 2u]) {
    if (!fef8_seed_pairs20 || !ff78_seed_pairs20 || !fef8_prefix_pairs20 || !ff78_prefix_pairs20 || !out_pairs46) return;

    for (unsigned k = 0; k < L3_AUTH_SCALAR_TAIL_CONV_PAIRS; ++k) {
        uint32_t lo = 0xbcd7e04bu;
        uint32_t hi = 0x4ca32ac9u;

        unsigned start = (k > 9u) ? (k - 9u) : 0u;
        unsigned end = (k < 9u) ? k : 9u;
        if (start <= end) {
            uint32_t conv_lo = 0u;
            uint32_t conv_hi = 0u;
            for (unsigned i = start; i <= end; ++i) {
                unsigned j = k - i;
                uint32_t a0 = fef8_seed_pairs20[i * 2u + 0u];
                uint32_t a1 = fef8_seed_pairs20[i * 2u + 1u];
                uint32_t b0 = ff78_seed_pairs20[j * 2u + 0u];
                uint32_t b1 = ff78_seed_pairs20[j * 2u + 1u];
                uint64_t p = (uint64_t)a0 * (uint64_t)b0;
                uint32_t p_lo = (uint32_t)p;
                uint32_t c = carry4(p_lo, conv_lo);
                conv_lo += p_lo;
                conv_hi = (uint32_t)(a1 * b0 + a0 * b1 + hi32(p) + conv_hi + c);
            }

            uint32_t fsum[2];
            uint32_t gsum[2];
            range_sum_from_prefix10(fef8_prefix_pairs20, start, end, fsum);
            range_sum_from_prefix10(ff78_prefix_pairs20, k - end, k - start, gsum);

            uint64_t count_mul = (uint64_t)(end - start + 1u) * 0x5e8d9011u;
            uint32_t count_lo = (uint32_t)count_mul;
            uint64_t conv_mul = (uint64_t)conv_lo * 0x96a306fdu;
            uint32_t conv_mul_lo = (uint32_t)conv_mul;
            uint64_t fsum_mul = (uint64_t)fsum[0] * 0xc3c8fc25u;
            uint32_t fsum_mul_lo = (uint32_t)fsum_mul;
            uint64_t gsum_mul = (uint64_t)gsum[0] * 0xb6e94b89u;
            uint32_t gsum_mul_lo = (uint32_t)gsum_mul;

            uint32_t s0 = count_lo + 0xbcd7e04bu;
            uint32_t s1 = s0 + conv_mul_lo;
            uint32_t s2 = s1 + fsum_mul_lo;
            lo = s2 + gsum_mul_lo;

            hi = (uint32_t)((end - start + 1u) * 0x1aae9964u + hi32(count_mul) +
                            0x4ca32ac9u + (uint32_t)(0x43281fb4u < count_lo) +
                            conv_hi * (uint32_t)-0x695cf903 +
                            conv_lo * (uint32_t)-0x73db3b62 + hi32(conv_mul) +
                            carry4(s0, conv_mul_lo) +
                            fsum[1] * (uint32_t)-0x3c3703db +
                            fsum[0] * (uint32_t)-0x18f051d1 + hi32(fsum_mul) +
                            carry4(s1, fsum_mul_lo) +
                            gsum[1] * (uint32_t)-0x4916b477 +
                            gsum[0] * (uint32_t)-0x40151088 + hi32(gsum_mul) +
                            carry4(s2, gsum_mul_lo));
        }

        uint64_t final_mul = (uint64_t)lo * 0x23f39549u;
        uint32_t final_lo = (uint32_t)final_mul;
        out_pairs46[k * 2u + 0u] = final_lo + 0xc9dd803au;
        out_pairs46[k * 2u + 1u] = (uint32_t)(hi * 0x23f39549u +
                                             lo * (uint32_t)-0x06c7ac3d +
                                             hi32(final_mul) + 0x77371ca2u +
                                             (uint32_t)(0x36227fc5u < final_lo));
    }
}


void l3_p256_private_scalar_reduction_seed_core(
    const uint32_t base_pair2[2],
    const uint32_t conv_pair2[2],
    uint32_t out_mul_pair2[2],
    uint32_t out_add_pair2[2]) {
    if (!base_pair2 || !conv_pair2) return;

    const uint32_t base_lo = base_pair2[0];
    const uint32_t base_hi = base_pair2[1];
    const uint32_t conv_lo = conv_pair2[0];
    const uint32_t conv_hi = conv_pair2[1];

    uint64_t base_a_mul = (uint64_t)base_lo * 0x546287b7u;
    uint32_t base_a_mul_lo = (uint32_t)base_a_mul;
    uint32_t base_a_lo = base_a_mul_lo + 0x68d12b0bu;
    uint32_t base_a_hi = (uint32_t)(base_hi * 0x546287b7u +
                                    base_lo * 0x1c10707au + hi32(base_a_mul) +
                                    0x1f174794u +
                                    (uint32_t)(0x972ed4f4u < base_a_mul_lo));

    uint64_t base_b_mul = (uint64_t)base_lo * 0x9310dc9du;
    uint32_t base_b_mul_lo = (uint32_t)base_b_mul;
    uint32_t base_b_lo = base_b_mul_lo + 0xa9150a21u;
    uint32_t base_b_hi = (uint32_t)(base_hi * (uint32_t)-0x6cef2363 +
                                    base_lo * 0x5a3e1fedu + hi32(base_b_mul) +
                                    0x2d07a66eu +
                                    (uint32_t)(0x56eaf5deu < base_b_mul_lo));

    uint64_t z_mul = (uint64_t)base_b_lo * (uint64_t)conv_lo;
    uint32_t z_mul_lo = (uint32_t)z_mul;
    uint32_t z_lo = base_a_lo + z_mul_lo;
    uint32_t z_hi = (uint32_t)(base_a_hi + conv_hi * base_b_lo +
                               conv_lo * base_b_hi + hi32(z_mul) +
                               carry4(base_a_lo, z_mul_lo));

    uint64_t t_mul = (uint64_t)z_lo * 0xc8c41c53u;
    uint32_t t_mul_lo = (uint32_t)t_mul;
    uint32_t lo = t_mul_lo + 0x55e87374u;
    uint32_t hi = (uint32_t)(z_hi * (uint32_t)-0x373be3ad +
                             z_lo * (uint32_t)-0x479a315b + hi32(t_mul) +
                             0x8effcb75u +
                             (uint32_t)(0xaa178c8bu < t_mul_lo));

    uint32_t lo7, hi7;
    nibble_ladder_f41a0078_run6_then_peek7(&lo, &hi, &lo7, &hi7);

    uint64_t lo7_mul = (uint64_t)lo7 * 0x70000000u;
    uint32_t lo7_mul_lo = (uint32_t)lo7_mul;
    uint64_t z_lo_mul = (uint64_t)z_lo * 0x4f56633bu;
    uint32_t z_lo_mul_lo = (uint32_t)z_lo_mul;
    uint32_t s0 = z_lo_mul_lo + lo7_mul_lo;
    uint32_t r_lo = s0 + 0x639d4830u;

    uint32_t r_hi = (uint32_t)(z_hi * 0x4f56633bu +
                               z_lo * (uint32_t)-0x6a593ef6 + hi32(z_lo_mul) +
                               lo7 * (uint32_t)-0x63e27008 + hi32(lo7_mul) +
                               hi7 * 0x70000000u +
                               carry4(z_lo_mul_lo, lo7_mul_lo) +
                               0x70bfb49fu +
                               (uint32_t)(0x9c62b7cfu < s0));

    uint64_t mul1 = (uint64_t)r_lo * 0xa307db97u;
    uint32_t mul1_lo = (uint32_t)mul1;
    uint32_t out1_lo = mul1_lo + 0x670a8908u;
    uint32_t out1_hi = (uint32_t)(r_hi * (uint32_t)-0x5cf82469 +
                                  r_lo * (uint32_t)-0x41f58da7 + hi32(mul1) +
                                  0x24849e6bu +
                                  (uint32_t)(0x98f576f7u < mul1_lo));

    uint64_t mul2 = (uint64_t)r_lo * 0x0a22bec3u;
    uint32_t mul2_lo = (uint32_t)mul2;
    uint32_t out2_lo = mul2_lo + 0x1e6e3aa8u;
    uint32_t out2_hi = (uint32_t)(r_hi * 0x0a22bec3u +
                                  r_lo * (uint32_t)-0x08f795cd + hi32(mul2) +
                                  0x7a47f5b5u +
                                  (uint32_t)(0xe191c557u < mul2_lo));

    if (out_mul_pair2) {
        out_mul_pair2[0] = out1_lo;
        out_mul_pair2[1] = out1_hi;
    }
    if (out_add_pair2) {
        out_add_pair2[0] = out2_lo;
        out_add_pair2[1] = out2_hi;
    }
}

void l3_p256_private_scalar_reduction_feedback_core(
    const uint32_t current_pair2[2],
    const uint32_t add_pair2[2],
    const uint32_t mul_pair2[2],
    const uint32_t coeff_pair2[2],
    const uint32_t feedback_pair2[2],
    uint32_t out_updated_pair2[2],
    uint32_t out_feedback_pair2[2]) {
    if (!current_pair2 || !add_pair2 || !mul_pair2 || !coeff_pair2 || !feedback_pair2) return;

    const uint32_t cur_lo = current_pair2[0];
    const uint32_t cur_hi = current_pair2[1];
    const uint32_t add_lo = add_pair2[0];
    const uint32_t add_hi = add_pair2[1];
    const uint32_t mul_lo = mul_pair2[0];
    const uint32_t mul_hi = mul_pair2[1];
    const uint32_t coeff_lo = coeff_pair2[0];
    const uint32_t coeff_hi = coeff_pair2[1];

    uint64_t coeff_mul = (uint64_t)coeff_lo * (uint64_t)mul_lo;
    uint32_t coeff_mul_lo = (uint32_t)coeff_mul;
    uint32_t sum0 = add_lo + cur_lo;
    uint32_t upd_lo = sum0 + coeff_mul_lo;
    uint32_t upd_hi = (uint32_t)(add_hi + cur_hi + carry4(add_lo, cur_lo) +
                                 mul_hi * coeff_lo + mul_lo * coeff_hi +
                                 hi32(coeff_mul) + carry4(sum0, coeff_mul_lo));

    if (out_updated_pair2) {
        out_updated_pair2[0] = upd_lo;
        out_updated_pair2[1] = upd_hi;
    }

    uint64_t upd_mul = (uint64_t)upd_lo * 0xa1340fefu;
    uint32_t upd_mul_lo = (uint32_t)upd_mul;
    uint32_t lo = upd_mul_lo + 0xe5c026b0u;
    uint32_t hi = (uint32_t)(upd_hi * (uint32_t)-0x5ecbf011 +
                             upd_lo * (uint32_t)-0x1eb8063e + hi32(upd_mul) +
                             0x36ff3d51u + (uint32_t)(0x1a3fd94fu < upd_mul_lo));

    uint32_t lo7, hi7;
    nibble_ladder_pair_run6_then_peek7(k_dat_f41a00f8_pairs, &lo, &hi, &lo7, &hi7);

    uint64_t a_mul = (uint64_t)lo7 * 0x37089badu;
    uint32_t a_mul_lo = (uint32_t)a_mul;
    uint32_t a_lo = a_mul_lo + 0x93e8f606u;
    uint32_t a_hi = (uint32_t)(hi7 * 0x37089badu +
                               lo7 * (uint32_t)-0x56495a0c + hi32(a_mul) +
                               (uint32_t)-0x36bff23e +
                               (uint32_t)(0x6c1709f9u < a_mul_lo));

    uint64_t b_mul = (uint64_t)a_lo * 0x18c2a5e5u;
    uint32_t b_mul_lo = (uint32_t)b_mul;
    uint32_t b_lo = b_mul_lo + 0xf2ee82c1u;
    uint32_t b_hi = (uint32_t)(a_hi * 0x18c2a5e5u +
                               a_lo * 0x18fecef6u + hi32(b_mul) +
                               0x0f00be72u + (uint32_t)(0x0d117d3eu < b_mul_lo));

    uint32_t f4178_token = nibble_ladder_pair_run6_step7_step8lo_token(k_dat_f41a0178_pairs, b_lo, b_hi);

    uint64_t c_mul0 = (uint64_t)a_lo * 0x6bcea6e3u;
    uint32_t c_lo = (uint32_t)c_mul0;
    uint64_t c_mul1 = (uint64_t)c_lo * 0x3f8c7b19u;
    uint32_t c_mul1_lo = (uint32_t)c_mul1;
    uint32_t s = c_mul1_lo + feedback_pair2[0];
    uint32_t fb_lo = s + 0x84ad4519u;
    uint32_t fb_hi = (uint32_t)((a_hi * 0x6bcea6e3u +
                                 a_lo * 0x3604ecf3u + hi32(c_mul0) +
                                 f4178_token * (uint32_t)-0x523cd270) * 0x3f8c7b19u +
                                c_lo * (uint32_t)-0x3a2979e6 + hi32(c_mul1) +
                                feedback_pair2[1] + carry4(c_mul1_lo, feedback_pair2[0]) +
                                (uint32_t)-0x167b3a9e +
                                (uint32_t)(0x7b52bae6u < s));

    if (out_feedback_pair2) {
        out_feedback_pair2[0] = fb_lo;
        out_feedback_pair2[1] = fb_hi;
    }
}




static void add_pair64_inplace(uint32_t acc[2], const uint32_t add[2]) {
    uint32_t old = acc[0];
    acc[0] = old + add[0];
    acc[1] = acc[1] + add[1] + carry4(old, add[0]);
}

static void mul_pair64_low(const uint32_t a[2], const uint32_t b[2], uint32_t out[2]) {
    uint64_t p = (uint64_t)a[0] * (uint64_t)b[0];
    out[0] = (uint32_t)p;
    out[1] = (uint32_t)(a[1] * b[0] + a[0] * b[1] + hi32(p));
}


void l3_p256_private_scalar_reduction_trace13_state(
    const uint32_t base_pair2[2],
    const uint32_t initial_pairs46[L3_AUTH_SCALAR_TAIL_CONV_PAIRS * 2u],
    const uint32_t coeff_pairs20[L3_AUTH_SCALAR_TAIL_LASTLOOP_INPUT_WORDS],
    uint32_t out_feedback_carrier_pairs26[L3_AUTH_SCALAR_TAIL_REDUCTION_ITERS * 2u],
    uint32_t out_updated_pairs26[L3_AUTH_SCALAR_TAIL_REDUCTION_ITERS * 2u],
    uint32_t out_next_pairs26[L3_AUTH_SCALAR_TAIL_REDUCTION_ITERS * 2u],
    uint32_t out_final_pairs46[L3_AUTH_SCALAR_TAIL_CONV_PAIRS * 2u]) {
    if (!base_pair2 || !initial_pairs46 || !coeff_pairs20) return;

    uint32_t slots[L3_AUTH_SCALAR_TAIL_CONV_PAIRS * 2u];
    memcpy(slots, initial_pairs46, sizeof(slots));

    for (unsigned i = 0; i < L3_AUTH_SCALAR_TAIL_REDUCTION_ITERS; ++i) {
        uint32_t current[2] = { slots[i * 2u + 0u], slots[i * 2u + 1u] };
        uint32_t mul_pair[2], add_pair[2];
        l3_p256_private_scalar_reduction_seed_core(base_pair2, current, mul_pair, add_pair);

        /* Native layout for the loop body:
         *   slot i      : scalar current + add + coeff[0] * mul
         *   slots i+1..6: q-vector 64-bit lanes for coeff[1]..coeff[6]
         *   slot i+7    : scalar lane for coeff[7]
         *   slots i+8..9: q-vector 64-bit lanes for coeff[8]..coeff[9]
         * Expressing it as a single coefficient loop is equivalent for the
         * 64-bit lane values, and this function is now the only implementation
         * used by the v201 carrier path and the v202/v203 slots handoff. */
        for (unsigned j = 0; j < L3_AUTH_SCALAR_TAIL_FINAL_INPUT_WORDS; ++j) {
            unsigned slot = i + j;
            if (slot >= L3_AUTH_SCALAR_TAIL_CONV_PAIRS) break;
            uint32_t prod[2];
            uint32_t delta[2] = { add_pair[0], add_pair[1] };
            mul_pair64_low(coeff_pairs20 + j * 2u, mul_pair, prod);
            add_pair64_inplace(delta, prod);
            add_pair64_inplace(slots + slot * 2u, delta);
        }

        uint32_t carrier[2] = {0u, 0u};
        if (i + 1u < L3_AUTH_SCALAR_TAIL_CONV_PAIRS) {
            carrier[0] = slots[(i + 1u) * 2u + 0u];
            carrier[1] = slots[(i + 1u) * 2u + 1u];
        }
        if (out_feedback_carrier_pairs26) {
            out_feedback_carrier_pairs26[i * 2u + 0u] = carrier[0];
            out_feedback_carrier_pairs26[i * 2u + 1u] = carrier[1];
        }

        uint32_t updated[2], next_pair[2];
        l3_p256_private_scalar_reduction_feedback_core(
            current, add_pair, mul_pair, coeff_pairs20, carrier, updated, next_pair);
        if (out_updated_pairs26) {
            out_updated_pairs26[i * 2u + 0u] = updated[0];
            out_updated_pairs26[i * 2u + 1u] = updated[1];
        }
        if (out_next_pairs26) {
            out_next_pairs26[i * 2u + 0u] = next_pair[0];
            out_next_pairs26[i * 2u + 1u] = next_pair[1];
        }

        slots[i * 2u + 0u] = updated[0];
        slots[i * 2u + 1u] = updated[1];
        if (i + 1u < L3_AUTH_SCALAR_TAIL_CONV_PAIRS) {
            slots[(i + 1u) * 2u + 0u] = next_pair[0];
            slots[(i + 1u) * 2u + 1u] = next_pair[1];
        }
    }

    if (out_final_pairs46) {
        memcpy(out_final_pairs46, slots, sizeof(slots));
    }
}

void l3_p256_private_scalar_reduction_slots23_state(
    const uint32_t base_pair2[2],
    const uint32_t initial_pairs46[L3_AUTH_SCALAR_TAIL_CONV_PAIRS * 2u],
    const uint32_t coeff_pairs20[L3_AUTH_SCALAR_TAIL_LASTLOOP_INPUT_WORDS],
    uint32_t out_final_pairs46[L3_AUTH_SCALAR_TAIL_CONV_PAIRS * 2u]) {
    l3_p256_private_scalar_reduction_trace13_state(base_pair2, initial_pairs46, coeff_pairs20,
                                           NULL, NULL, NULL, out_final_pairs46);
}




void l3_p256_private_scalar_lastloop_pairs20_to_words10(
    const uint32_t pairs20[L3_AUTH_SCALAR_TAIL_LASTLOOP_INPUT_WORDS],
    uint32_t words10[L3_AUTH_SCALAR_TAIL_FINAL_INPUT_WORDS]) {
    if (!pairs20 || !words10) return;

    uint32_t hi_acc = 0xd73f6ac2u;
    uint32_t state = 0x1b78033bu;

    for (unsigned i = 0; i < 10u; ++i) {
        const uint32_t a = pairs20[i * 2u + 0u];
        const uint32_t b = pairs20[i * 2u + 1u];

        uint64_t p_a = (uint64_t)a * 0xc60f8e77u;
        uint32_t p_a_lo = (uint32_t)p_a;
        uint64_t p_acc = (uint64_t)hi_acc * 0x8d84aa71u;
        uint32_t p_acc_lo = (uint32_t)p_acc;
        uint32_t s0 = p_acc_lo + p_a_lo;
        uint32_t x = s0 + 0x9da61bf4u;
        uint64_t p_x = (uint64_t)x * 0x92e5994bu;
        uint32_t p_x_lo = (uint32_t)p_x;
        uint32_t lo = p_x_lo + 0xde86e8e0u;

        uint32_t hi = (uint32_t)((state * 0x8d84aa71u) +
                                 (hi_acc * 0x7218b441u) + hi32(p_acc) +
                                 (b * 0xc60f8e77u) +
                                 (a * 0x233c914fu) + hi32(p_a) +
                                 carry4(p_acc_lo, p_a_lo) + 0xc4514a0cu +
                                 (uint32_t)(0x6259e40bu < s0));
        hi = (uint32_t)(hi * 0x92e5994bu +
                        x * 0x39b7209eu + hi32(p_x) + 0xe9c1c710u +
                        (uint32_t)(0x2179171fu < p_x_lo));

        for (unsigned j = 0; j < 7u; ++j) {
            nibble_ladder_f41a01f8_step(&lo, &hi);
        }
        const uint32_t mid_lo = lo;
        const uint32_t mid_hi = hi;

        for (unsigned j = 0; j < 7u; ++j) {
            nibble_ladder_f41a01f8_step(&lo, &hi);
        }
        const unsigned final_nib = lo & 0x0fu;
        const uint32_t final_shift = (lo >> 4) | (hi << 28);
        const uint32_t final_hi_add = (hi >> 4) + k_dat_f41a01f8_pairs[final_nib][1] +
                                      carry4(final_shift, k_dat_f41a01f8_pairs[final_nib][0]);
        const uint32_t final_lo = final_shift + k_dat_f41a01f8_pairs[final_nib][0];
        const uint32_t lookup = k_dat_f41a01f8_pairs[final_lo & 0x0fu][0];

        words10[i] = (uint32_t)((x * 0x2bed775bu +
                                 mid_lo * 0xf0000000u +
                                 0x1d0e4211u) * k_dat_f4190170[i & 7u] +
                                k_dat_f4190190[i & 7u]);

        uint64_t p_mid = (uint64_t)mid_lo * 0x39f2a213u;
        uint32_t p_mid_lo = (uint32_t)p_mid;
        /* Native FUN_f3fbab70 updates the rolling low state from the
         * post-ladder mid word, not from the previous hi_acc multiply.
         * Decompilation around f3fbbdxx:
         *   uVar16 = (uint)((ulonglong)uVar12 * 0x39f2a213);
         *   uVar4  = uVar16 + 0xfc684801;
         *   iVar5  = ... + (uint)(0x0397b7fe < uVar16);
         * where uVar12 is the 7-step mid_ladder low word. */
        hi_acc = p_mid_lo + 0xfc684801u;
        state = (uint32_t)(mid_hi * 0x39f2a213u +
                           mid_lo * 0x72b8a8b1u + hi32(p_mid) +
                           ((final_lo >> 4) + lookup) * 0x60d5ded0u +
                           0xe037132cu +
                           (uint32_t)(0x0397b7feu < p_mid_lo));
        (void)final_hi_add;
    }
}

void l3_p256_private_scalar_lastloop_pairs20_to_tail35(
    const uint32_t pairs20[L3_AUTH_SCALAR_TAIL_LASTLOOP_INPUT_WORDS],
    uint8_t out35[L3_AUTH_SCALAR_TAIL_TAIL_LEN]) {
    if (!pairs20 || !out35) return;
    uint32_t words10[L3_AUTH_SCALAR_TAIL_FINAL_INPUT_WORDS];
    l3_p256_private_scalar_lastloop_pairs20_to_words10(pairs20, words10);
    l3_p256_private_scalar_final_words10_to_tail35(words10, out35);
}

void l3_p256_private_scalar_final_words10_to_lanes10(const uint32_t words10[10],
                                               uint32_t lanes10[10]) {
    if (!words10 || !lanes10) return;
    for (unsigned i = 0; i < 10u; ++i) {
        uint32_t x = words10[i] * k_dat_f41901b0[i & 7u] + k_dat_f41901d0[i & 7u];
        lanes10[i] = x * (uint32_t)-0x529ef199 + 0x3398f037u;
    }
}

static uint32_t shr32(uint32_t v, uint32_t n) {
    n &= 0xffu;
    return n >= 32u ? 0u : (v >> n);
}

static uint32_t shl32(uint32_t v, uint32_t n) {
    n &= 0xffu;
    return n >= 32u ? 0u : (v << n);
}

void l3_p256_private_scalar_final_words10_to_tail35(const uint32_t words10[10],
                                             uint8_t out35[35]) {
    if (!words10 || !out35) return;
    memset(out35, 0, 35u);

    uint32_t lanes[10];
    l3_p256_private_scalar_final_words10_to_lanes10(words10, lanes);

    uint32_t hi = 0;
    uint32_t lo = 0;
    uint32_t bitpos = 0;
    int out_i = 0;
    for (uint32_t i = 0; i < 10u && out_i < 0x23; ++i) {
        uint32_t v = lanes[i];
        uint32_t over = bitpos - 0x20u;
        uint32_t upper = shr32(v, (0x20u - bitpos) & 0xffu);
        if ((int32_t)over >= 0) upper = shl32(v, over & 0xffu);
        uint32_t lower = shl32(v, bitpos & 0xffu);
        if ((int32_t)over >= 0) lower = 0;

        hi ^= upper;
        lo ^= lower;
        bitpos += 0x1cu;
        while (out_i < 0x23 && (int32_t)bitpos > 8) {
            out35[out_i++] = (uint8_t)lo;
            lo = (lo >> 8) | (hi << 24);
            hi >>= 8;
            bitpos -= 8u;
        }
    }
    if ((int32_t)bitpos > 0 && out_i < 0x23) {
        out35[out_i] = (uint8_t)lo;
    }
}


void l3_p256_private_scalar_build_convolution_from_streams(
    const uint32_t fef8_words10[L3_AUTH_SCALAR_TAIL_FINAL_INPUT_WORDS],
    const uint32_t ff78_words10[L3_AUTH_SCALAR_TAIL_FINAL_INPUT_WORDS],
    uint32_t out_pairs46[L3_AUTH_SCALAR_TAIL_CONV_PAIRS * 2u]) {
    if (!fef8_words10 || !ff78_words10 || !out_pairs46) return;
    uint32_t fseed[20], fpref[20], gseed[20], gpref[20];
    l3_p256_private_scalar_build_prefix_stream_a(fef8_words10, fseed, fpref);
    l3_p256_private_scalar_build_prefix_stream_b(ff78_words10, gseed, gpref);
    l3_p256_private_scalar_convolution_pairs23(fseed, gseed, fpref, gpref, out_pairs46);
}

void l3_p256_private_scalar_streams10_to_reduction_pairs20_boundary(
    const uint32_t base_pair2[2],
    const uint32_t fef8_words10[L3_AUTH_SCALAR_TAIL_FINAL_INPUT_WORDS],
    const uint32_t ff78_words10[L3_AUTH_SCALAR_TAIL_FINAL_INPUT_WORDS],
    const uint32_t coeff_words10[L3_AUTH_SCALAR_TAIL_FINAL_INPUT_WORDS],
    uint32_t out_pairs20[L3_AUTH_SCALAR_TAIL_LASTLOOP_INPUT_WORDS]) {
    if (!base_pair2 || !fef8_words10 || !ff78_words10 || !coeff_words10 || !out_pairs20) return;

    uint32_t initial_slots[L3_AUTH_SCALAR_TAIL_CONV_PAIRS * 2u];
    uint32_t final_slots[L3_AUTH_SCALAR_TAIL_CONV_PAIRS * 2u];
    uint32_t coeffs[L3_AUTH_SCALAR_TAIL_LASTLOOP_INPUT_WORDS];
    l3_p256_private_scalar_build_convolution_from_streams(fef8_words10, ff78_words10, initial_slots);
    l3_p256_private_scalar_build_coefficient_schedule(coeff_words10, coeffs);
    l3_p256_private_scalar_reduction_slots23_state(base_pair2, initial_slots, coeffs, final_slots);
    memcpy(out_pairs20, final_slots + 13u * 2u, L3_AUTH_SCALAR_TAIL_LASTLOOP_INPUT_WORDS * sizeof(uint32_t));
}

void l3_p256_private_scalar_streams10_to_tail35_reduction_boundary(
    const uint32_t base_pair2[2],
    const uint32_t fef8_words10[L3_AUTH_SCALAR_TAIL_FINAL_INPUT_WORDS],
    const uint32_t ff78_words10[L3_AUTH_SCALAR_TAIL_FINAL_INPUT_WORDS],
    const uint32_t coeff_words10[L3_AUTH_SCALAR_TAIL_FINAL_INPUT_WORDS],
    uint8_t out35[L3_AUTH_SCALAR_TAIL_TAIL_LEN]) {
    if (!base_pair2 || !fef8_words10 || !ff78_words10 || !coeff_words10 || !out35) return;
    uint32_t pairs20[L3_AUTH_SCALAR_TAIL_LASTLOOP_INPUT_WORDS];
    l3_p256_private_scalar_streams10_to_reduction_pairs20_boundary(base_pair2, fef8_words10, ff78_words10, coeff_words10, pairs20);
    l3_p256_private_scalar_lastloop_pairs20_to_tail35(pairs20, out35);
}




static uint32_t load_le32_local(const uint8_t *p) {
    return ((uint32_t)p[0]) | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

int l3_p256_private_scalar_decode_frame_words(
    const uint8_t param2_frame[L3_AUTH_SCALAR_TAIL_PARAM2_FRAME_LEN],
    uint32_t fef8_words10[L3_AUTH_SCALAR_TAIL_FINAL_INPUT_WORDS]) {
    if (!param2_frame || !fef8_words10) return -1;

    static const uint32_t mul0[4] = {
        0x30e43cdbu, 0xd0e48563u, 0x077c771du, 0x766e4c7fu
    };
    static const uint32_t add0[4] = {
        0xde2fda6cu, 0xd69a17d6u, 0x736f4517u, 0xd6f17fd3u
    };
    static const uint32_t mul1[4] = {
        0xd8733cd9u, 0x7ac6a095u, 0x911fe1abu, 0xdd60a2b3u
    };
    static const uint32_t add1[4] = {
        0x555a1f13u, 0xa0a5c7b9u, 0xd707a932u, 0xe207b16du
    };

    for (unsigned i = 0; i < 4u; ++i) {
        uint32_t w0 = load_le32_local(param2_frame + i * 4u);
        uint32_t w1 = load_le32_local(param2_frame + 0x10u + i * 4u);
        fef8_words10[i] = (uint32_t)(w0 * mul0[i] + add0[i]);
        fef8_words10[4u + i] = (uint32_t)(w1 * mul1[i] + add1[i]);
    }

    uint32_t p20 = load_le32_local(param2_frame + 0x20u);
    uint32_t p24 = load_le32_local(param2_frame + 0x24u);
    fef8_words10[8] = (uint32_t)(p20 * 0x30e43cdbu + 0xde2fda6cu);
    fef8_words10[9] = (uint32_t)(p24 * (uint32_t)-0x2f1b7a9du + 0xd69a17d6u);
    return 0;
}

int l3_p256_private_scalar_extract_parent_streams(
    const uint8_t *parent_ctx,
    uint32_t base_pair2[2],
    uint32_t ff78_words10[L3_AUTH_SCALAR_TAIL_FINAL_INPUT_WORDS],
    uint32_t coeff_words10[L3_AUTH_SCALAR_TAIL_FINAL_INPUT_WORDS]) {
    if (!parent_ctx) return -1;

    if (base_pair2) {
        base_pair2[0] = load_le32_local(parent_ctx + 0x1c70u);
        base_pair2[1] = load_le32_local(parent_ctx + 0x1c74u);
    }
    if (ff78_words10) {
        for (unsigned i = 0; i < L3_AUTH_SCALAR_TAIL_FINAL_INPUT_WORDS; ++i) {
            ff78_words10[i] = load_le32_local(parent_ctx + 0x1c80u + i * 4u);
        }
    }
    if (coeff_words10) {
        for (unsigned i = 0; i < L3_AUTH_SCALAR_TAIL_FINAL_INPUT_WORDS; ++i) {
            coeff_words10[i] = load_le32_local(parent_ctx + 0x1ac8u + i * 4u);
        }
    }
    return 0;
}

int l3_p256_decode_app_private_scalar35(
    const uint8_t *parent_ctx,
    const uint8_t param2_frame[L3_AUTH_SCALAR_TAIL_PARAM2_FRAME_LEN],
    uint8_t out35[L3_AUTH_SCALAR_TAIL_TAIL_LEN]) {
    if (!parent_ctx || !param2_frame || !out35) return -1;
    uint32_t base_pair[2];
    uint32_t fef8_words10[L3_AUTH_SCALAR_TAIL_FINAL_INPUT_WORDS];
    uint32_t ff78_words10[L3_AUTH_SCALAR_TAIL_FINAL_INPUT_WORDS];
    uint32_t coeff_words10[L3_AUTH_SCALAR_TAIL_FINAL_INPUT_WORDS];
    if (l3_p256_private_scalar_decode_frame_words(param2_frame, fef8_words10) != 0) return -1;
    if (l3_p256_private_scalar_extract_parent_streams(parent_ctx, base_pair, ff78_words10, coeff_words10) != 0) return -1;
    l3_p256_private_scalar_streams10_to_tail35_reduction_boundary(base_pair, fef8_words10, ff78_words10, coeff_words10, out35);
    return 0;
}
