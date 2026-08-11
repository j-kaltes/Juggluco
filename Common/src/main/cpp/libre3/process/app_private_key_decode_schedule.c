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
#include "app_private_key_decode_schedule.h"

#include <string.h>

static uint32_t u32(uint32_t x) { return x; }

void l3_private_key_rng_seed(l3_private_key_rng *rng, const uint8_t *seed, size_t seed_len) {
    if (!rng) return;
    uint32_t s38 = 0xb3f793d0u;
    uint32_t s3c = 0x58c0ab95u;
    uint32_t s40 = 0xf9259d36u;
    uint32_t s44 = 0xd50ffae4u;
    if (seed) {
        uint32_t bitpos = 0;
        for (size_t i = 0; i < seed_len; ++i) {
            uint32_t b = seed[i];
            uint32_t sh = bitpos & 0x18u;
            bitpos += 8u;
            s3c ^= b << sh; s3c = u32(s3c ^ (s3c << 3) ^ (s3c >> 5));
            s38 ^= b << sh; s38 = u32(s38 ^ (s38 << 13) ^ (s38 >> 7));
            s40 ^= b << sh; s40 = u32(s40 ^ (s40 << 1) ^ (s40 >> 9));
            s44 ^= b << sh; s44 = u32(s44 ^ (s44 << 5) ^ (s44 >> 13));
        }
    }
    rng->s38 = s38;
    rng->s3c = s3c;
    rng->s40 = s40;
    rng->s44 = s44;
    rng->s48 = s38 ^ s3c ^ s40 ^ s44;
}

uint32_t l3_private_key_rng_next(l3_private_key_rng *rng) {
    if (!rng) return 0;
    uint32_t x = rng->s38;
    uint32_t s40 = rng->s40;
    uint32_t y = x ^ (x << 11);
    rng->s38 = rng->s3c;
    rng->s3c = s40;
    rng->s40 = rng->s44;
    rng->s44 = u32(y ^ (y >> 8) ^ rng->s44 ^ (rng->s44 >> 19));
    rng->s48 = rng->s38 ^ rng->s3c ^ rng->s40 ^ rng->s44;
    return rng->s44;
}

int l3_private_key_rng_permutation(l3_private_key_rng *rng, uint32_t n, uint32_t *out_perm) {
    if (!rng || !out_perm) return L3_OUTER_ERR_ARGUMENT;
    for (uint32_t i = 0; i < n; ++i) out_perm[i] = i;
    if (n <= 1) return L3_OUTER_OK;
    for (uint32_t u = n - 1; u != 0; --u) {
        uint32_t r = l3_private_key_rng_next(rng) % (u + 1u);
        uint32_t tmp = out_perm[r];
        out_perm[r] = out_perm[u];
        out_perm[u] = tmp;
    }
    return L3_OUTER_OK;
}

typedef struct {
    uint8_t input_index;
    uint8_t d0;
    uint8_t d1;
    uint16_t k1;
    uint16_t k2;
    uint16_t k3;
    uint16_t k4;
    uint16_t dst_suffix;
} init_case;

static const init_case init_cases[16] = {
    {0x02,7,2,0xb7a1,0x3908,0x56de,0x72c2,0x1f8},
    {0x0c,4,3,0x26d7,0xd27b,0xcc13,0x112a,0x1bc},
    {0x01,1,6,0xcc6d,0xa457,0x79b1,0x3ab6,0x1fe},
    {0x0e,4,2,0xb77b,0x8b9e,0x5c70,0x6731,0x1b0},
    {0x08,6,6,0x8782,0x983b,0x445f,0x2da8,0x1d4},
    {0x06,4,6,0x301c,0x1964,0xd28d,0xb49a,0x1e0},
    {0x05,7,5,0xc732,0x6123,0x62e5,0x758b,0x1e6},
    {0x09,2,4,0xbc2b,0x8f4e,0x45df,0xc1b3,0x1ce},
    {0x0b,5,2,0xc958,0xb6c5,0x70bf,0xb6ce,0x1c2},
    {0x07,3,6,0xc23c,0x42ab,0x8cc2,0xbc52,0x1da},
    {0x00,3,2,0xd1c5,0xd08d,0x478e,0xce29,0x204},
    {0x0f,3,6,0x29a4,0x81f5,0x2416,0x1d79,0x1aa},
    {0x03,5,2,0x35f5,0x7bfa,0xd169,0x9071,0x1f2},
    {0x0d,0,7,0xce41,0x43d6,0xb824,0x83b9,0x1b6},
    {0x04,1,6,0xb36e,0x15d8,0x706c,0xab1f,0x1ec},
    {0x0a,2,3,0xbef9,0x046c,0xb39b,0x1142,0x1c8},
};

static void split_input_byte(uint8_t b, uint8_t v[6], uint8_t d0, uint8_t d1) {
    v[0] = d0;
    v[1] = d1;
    v[2] = b & 3u;
    v[3] = (b >> 2) & 3u;
    v[4] = (b >> 4) & 3u;
    v[5] = (b >> 6) & 3u;
}


int l3_private_key_decode_stage0_expand_full_state(const l3_private_key_tables *tables,
                                      const uint8_t *program_f80040,
                                      size_t program_f80040_len,
                                      const uint8_t input16[16],
                                      const uint8_t *object_state,
                                      size_t object_state_len,
                                      uint8_t out_stage0[0x80]) {
    if (!tables || !program_f80040 || !input16 || !object_state || !out_stage0) {
        return L3_OUTER_ERR_ARGUMENT;
    }

    l3_private_key_rng rng;
    uint32_t perm[16];
    l3_private_key_rng_seed(&rng, input16, 0x10);
    int rc = l3_private_key_rng_permutation(&rng, 16, perm);
    if (rc) return rc;

    uint8_t work[0x80];
    memset(work, 0, sizeof(work));

    for (uint32_t i = 0; i < 16; ++i) {
        const init_case *cs = &init_cases[perm[i] & 15u];
        uint8_t local190[6], tmp196[6], tmp19c[6], tmp1a2[6];
        split_input_byte(input16[cs->input_index], local190, cs->d0, cs->d1);
        rc = l3_private_key_table_mix6_a(tables, cs->k1, local190, local190, tmp196);
        if (rc) return L3_OUTER_ERR_STAGE;
        rc = l3_private_key_table_map6(tables, cs->k2, tmp196, tmp19c);
        if (rc) return L3_OUTER_ERR_STAGE;
        rc = l3_private_key_table_mix6_c(tables, cs->k3, tmp19c, tmp19c, tmp1a2);
        if (rc) return L3_OUTER_ERR_STAGE;
        size_t dst = (size_t)(0x204u - cs->dst_suffix);
        if (dst + 6u > sizeof(work)) return L3_OUTER_ERR_STAGE;
        rc = l3_private_key_table_mix6_a(tables, cs->k4, tmp1a2, tmp196, work + dst);
        if (rc) return L3_OUTER_ERR_STAGE;
    }

    /* Same physical caller-frame overlap as the fixed-size wrapper above. */
    memset(out_stage0, 0, 0x60u);
    memcpy(out_stage0 + 0x60u, work, 0x20u);
    rc = l3_private_key_execute_stage1_instruction_group(tables, program_f80040, program_f80040_len,
                         work, sizeof(work), object_state, object_state_len,
                         out_stage0, 0x80u);
    if (rc) return L3_OUTER_ERR_STAGE;
    return L3_OUTER_OK;
}



typedef struct { uint16_t k, src, dst; } round_d3c;
typedef struct { uint16_t k, src, state_off, dst; } round_state_c1c;
typedef struct { uint8_t op; uint16_t k, a, b, dst; } round_mix_op;

#define FRAME_SIZE 0x400u
#define OFF_ABSTACK_384 0x000u
#define OFF_ABSTACK_204 0x180u
#define OFF_LOCAL_148    0x23cu

static void write_perm_u32(uint8_t *frame, const uint32_t *perm, uint32_t n) {
    if (!frame || !perm) return;
    for (uint32_t i = 0; i < n; ++i) {
        uint32_t v = perm[i];
        frame[OFF_LOCAL_148 + i*4u + 0] = (uint8_t)v;
        frame[OFF_LOCAL_148 + i*4u + 1] = (uint8_t)(v >> 8);
        frame[OFF_LOCAL_148 + i*4u + 2] = (uint8_t)(v >> 16);
        frame[OFF_LOCAL_148 + i*4u + 3] = (uint8_t)(v >> 24);
    }
}

static const round_d3c round_first_d3c[16] = {
    {0x1d8b, 0x132, 0x11a},
    {0x814f, 0x144, 0x114},
    {0x61c2, 0x15c, 0x0cc},
    {0x8b7d, 0x156, 0x10e},
    {0x6025, 0x174, 0x0e4},
    {0xa6ac, 0x138, 0x0d8},
    {0xb8dd, 0x150, 0x0f0},
    {0xaf5e, 0x16e, 0x0c6},
    {0x2923, 0x13e, 0x0f6},
    {0x54ff, 0x162, 0x0ea},
    {0x191f, 0x17a, 0x102},
    {0x6a81, 0x12c, 0x0fc},
    {0xbe4a, 0x126, 0x0de},
    {0x49fb, 0x14a, 0x0d2},
    {0x0057, 0x120, 0x0c0},
    {0x9441, 0x168, 0x108},
};

static const round_d3c round_later_d3c[16] = {
    {0x995c, 0x036, 0x10e},
    {0x957f, 0x018, 0x0d8},
    {0x6051, 0x024, 0x114},
    {0x2701, 0x048, 0x108},
    {0x4a79, 0x054, 0x0e4},
    {0x7fe9, 0x042, 0x0ea},
    {0xba93, 0x006, 0x0de},
    {0x010b, 0x030, 0x0f0},
    {0xd3d4, 0x012, 0x11a},
    {0x2f4e, 0x04e, 0x0c6},
    {0x9d89, 0x05a, 0x102},
    {0x031c, 0x03c, 0x0cc},
    {0xb1f4, 0x02a, 0x0d2},
    {0x80d4, 0x01e, 0x0f6},
    {0x9f33, 0x00c, 0x0fc},
    {0xd50a, 0x000, 0x0c0},
};

static const round_state_c1c round_state_c1c_tbl[16] = {
    {0x12fc, 0x11a, 0x54a, 0x0ba},
    {0x0d3e, 0x0fc, 0x388, 0x09c},
    {0x6342, 0x0cc, 0x0b8, 0x06c},
    {0xc1ad, 0x10e, 0x496, 0x0ae},
    {0xc092, 0x0e4, 0x220, 0x084},
    {0xb6da, 0x108, 0x43c, 0x0a8},
    {0x6096, 0x0de, 0x1c6, 0x07e},
    {0x73b5, 0x0f6, 0x32e, 0x096},
    {0x89e6, 0x114, 0x4f0, 0x0b4},
    {0x1b54, 0x0c6, 0x05e, 0x066},
    {0x1842, 0x0d8, 0x16c, 0x078},
    {0xb408, 0x102, 0x3e2, 0x0a2},
    {0x1af3, 0x0f0, 0x2d4, 0x090},
    {0x159f, 0x0c0, 0x004, 0x060},
    {0x83ad, 0x0d2, 0x112, 0x072},
    {0x18c7, 0x0ea, 0x27a, 0x08a},
};

static const round_mix_op round_mix_case2[] = {
    {1, 0x4459, 0x084, 0x078, 0x1f4},
    {1, 0x917d, 0x08a, 0x07e, 0x1ee},
    {1, 0xacb7, 0x078, 0x08a, 0x1e8},
    {1, 0xa0a0, 0x07e, 0x084, 0x1e2},
    {2, 0xb617, 0x1f4, 0, 0x314},
    {2, 0x5416, 0x1ee, 0, 0x2c4},
    {2, 0x65bd, 0x1e8, 0, 0x29c},
    {2, 0x45fb, 0x1e2, 0, 0x2f8},
    {2, 0x7dde, 0x314, 0, 0x2e6},
    {2, 0x0d20, 0x2c4, 0, 0x30a},
    {1, 0x5d3a, 0x2e6, 0x30a, 0x236},
    {2, 0x7818, 0x236, 0, 0x230},
    {1, 0xa886, 0x230, 0x1f4, 0x22a},
    {1, 0x16aa, 0x22a, 0x1ee, 0x224},
    {1, 0x2b1e, 0x224, 0x2e6, 0x21e},
    {1, 0x2a95, 0x21e, 0x314, 0x218},
    {1, 0x42ed, 0x224, 0x30a, 0x212},
    {1, 0x9227, 0x218, 0x2f8, 0x20c},
    {1, 0x24e9, 0x20c, 0x078, 0x018},
    {1, 0x725b, 0x212, 0x2f8, 0x206},
    {1, 0x1e2d, 0x206, 0x07e, 0x01e},
    {1, 0x6d19, 0x218, 0x29c, 0x200},
    {1, 0x6e9d, 0x200, 0x084, 0x024},
    {1, 0x9c3a, 0x212, 0x29c, 0x1fa},
    {1, 0x4821, 0x1fa, 0x08a, 0x02a},
};

static const round_mix_op round_mix_case1[] = {
    {1, 0x89da, 0x09c, 0x090, 0x1f4},
    {1, 0x2338, 0x0a2, 0x096, 0x1ee},
    {1, 0x59d2, 0x090, 0x0a2, 0x1e8},
    {1, 0x2237, 0x096, 0x09c, 0x1e2},
    {2, 0x7ef4, 0x1f4, 0, 0x314},
    {2, 0xa258, 0x1ee, 0, 0x2c4},
    {2, 0x41ca, 0x1e8, 0, 0x29c},
    {2, 0x05cf, 0x1e2, 0, 0x2f8},
    {2, 0x0611, 0x314, 0, 0x2e6},
    {2, 0xca03, 0x2c4, 0, 0x30a},
    {1, 0x42e7, 0x2e6, 0x30a, 0x236},
    {2, 0x3ace, 0x236, 0, 0x230},
    {1, 0x1fe0, 0x230, 0x1f4, 0x22a},
    {1, 0x95c2, 0x22a, 0x1ee, 0x224},
    {1, 0x3a5e, 0x224, 0x2e6, 0x21e},
    {1, 0x10c7, 0x21e, 0x314, 0x218},
    {1, 0x1a3a, 0x224, 0x30a, 0x212},
    {1, 0x654b, 0x218, 0x2f8, 0x20c},
    {1, 0xb008, 0x20c, 0x090, 0x030},
    {1, 0x1508, 0x212, 0x2f8, 0x206},
    {1, 0xad0c, 0x206, 0x096, 0x036},
    {1, 0xd4b9, 0x218, 0x29c, 0x200},
    {1, 0x3f64, 0x200, 0x09c, 0x03c},
    {1, 0x2854, 0x212, 0x29c, 0x1fa},
    {1, 0xd495, 0x1fa, 0x0a2, 0x042},
};

static const round_mix_op round_mix_case0[] = {
    {1, 0x3ec7, 0x06c, 0x060, 0x1f4},
    {1, 0x1e7c, 0x072, 0x066, 0x1ee},
    {1, 0xc1cb, 0x060, 0x072, 0x1e8},
    {1, 0x2c85, 0x066, 0x06c, 0x1e2},
    {2, 0xaf22, 0x1f4, 0, 0x314},
    {2, 0x298c, 0x1ee, 0, 0x2c4},
    {2, 0x4422, 0x1e8, 0, 0x29c},
    {2, 0x9c55, 0x1e2, 0, 0x2f8},
    {2, 0x1257, 0x314, 0, 0x2e6},
    {2, 0x6229, 0x2c4, 0, 0x30a},
    {1, 0xd66a, 0x2e6, 0x30a, 0x236},
    {2, 0x73a6, 0x236, 0, 0x230},
    {1, 0x088f, 0x230, 0x1f4, 0x22a},
    {1, 0x7fdd, 0x22a, 0x1ee, 0x224},
    {1, 0x38cf, 0x224, 0x2e6, 0x21e},
    {1, 0xd7dd, 0x21e, 0x314, 0x218},
    {1, 0x4ce5, 0x224, 0x30a, 0x212},
    {1, 0x5ba6, 0x218, 0x2f8, 0x20c},
    {1, 0x5897, 0x20c, 0x060, 0x000},
    {1, 0x6505, 0x212, 0x2f8, 0x206},
    {1, 0x738d, 0x206, 0x066, 0x006},
    {1, 0xa14f, 0x218, 0x29c, 0x200},
    {1, 0xc397, 0x200, 0x06c, 0x00c},
    {1, 0x75d8, 0x212, 0x29c, 0x1fa},
    {1, 0x8c92, 0x1fa, 0x072, 0x012},
};

static const round_mix_op round_mix_case3[] = {
    {1, 0x3cc7, 0x0b4, 0x0a8, 0x1f4},
    {1, 0xd0c5, 0x0ba, 0x0ae, 0x1ee},
    {1, 0xd7eb, 0x0a8, 0x0ba, 0x1e8},
    {1, 0x227f, 0x0ae, 0x0b4, 0x1e2},
    {2, 0x3232, 0x1f4, 0, 0x314},
    {2, 0x3fe4, 0x1ee, 0, 0x2c4},
    {2, 0x09d7, 0x1e8, 0, 0x29c},
    {2, 0xcb03, 0x1e2, 0, 0x2f8},
    {2, 0x3e8e, 0x314, 0, 0x2e6},
    {2, 0x250a, 0x2c4, 0, 0x30a},
    {1, 0xc7c7, 0x2e6, 0x30a, 0x236},
    {2, 0x3690, 0x236, 0, 0x230},
    {1, 0x6cc7, 0x230, 0x1f4, 0x22a},
    {1, 0x8346, 0x22a, 0x1ee, 0x224},
    {1, 0x212e, 0x224, 0x2e6, 0x21e},
    {1, 0x4951, 0x21e, 0x314, 0x218},
    {1, 0x3fba, 0x224, 0x30a, 0x212},
    {1, 0x77e4, 0x218, 0x2f8, 0x20c},
    {1, 0x0230, 0x20c, 0x0a8, 0x048},
    {1, 0x2dd5, 0x212, 0x2f8, 0x206},
    {1, 0x6e7a, 0x206, 0x0ae, 0x04e},
    {1, 0x7766, 0x218, 0x29c, 0x200},
    {1, 0x6b74, 0x200, 0x0b4, 0x054},
    {1, 0x7eca, 0x212, 0x29c, 0x1fa},
    {1, 0x0273, 0x1fa, 0x0ba, 0x05a},
};

static uint32_t load_le32_outer(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int do_d3c_frame(const l3_private_key_tables *tables, uint8_t *frame, const round_d3c *op) {
    if (op->src + 6u > FRAME_SIZE || op->dst + 6u > FRAME_SIZE) return L3_OUTER_ERR_STAGE;
    return l3_private_key_table_map6(tables, op->k, frame + op->src, frame + op->dst) ? L3_OUTER_ERR_STAGE : L3_OUTER_OK;
}

static int do_c1c_frame(const l3_private_key_tables *tables, uint8_t *frame, const round_mix_op *op) {
    if (op->a + 6u > FRAME_SIZE || op->b + 6u > FRAME_SIZE || op->dst + 6u > FRAME_SIZE) return L3_OUTER_ERR_STAGE;
    return l3_private_key_table_mix6_a(tables, op->k, frame + op->a, frame + op->b, frame + op->dst) ? L3_OUTER_ERR_STAGE : L3_OUTER_OK;
}

static int run_mix_ops(const l3_private_key_tables *tables, uint8_t *frame,
                       const round_mix_op *ops, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        int rc;
        if (ops[i].op == 1) rc = do_c1c_frame(tables, frame, &ops[i]);
        else {
            round_d3c d = {ops[i].k, ops[i].a, ops[i].dst};
            rc = do_d3c_frame(tables, frame, &d);
        }
        if (rc) return rc;
    }
    return L3_OUTER_OK;
}

int l3_private_key_decode_stage0_round_loop(const l3_private_key_tables *tables,
                                   const uint8_t *program_f80040,
                                   size_t program_f80040_len,
                                   const uint8_t input16[16],
                                   const uint8_t *object_state,
                                   size_t object_state_len,
                                   uint8_t out_frame_0x400[0x400]) {
    if (!tables || !program_f80040 || !input16 || !object_state || !out_frame_0x400) return L3_OUTER_ERR_ARGUMENT;
    if (object_state_len < 4u) return L3_OUTER_ERR_NEED_OBJECT_STATE;

    uint32_t total = load_le32_outer(object_state);
    int local_404 = (int)total - 4;
    if (local_404 < 0) return L3_OUTER_ERR_ARGUMENT;

    uint8_t frame[FRAME_SIZE];
    memset(frame, 0, sizeof(frame));

    /*
     * Re-run stage0. Native passes abStack_204 as f80040 param1 but passes
     * r2 = sp+0x24c as the generated terminal copy-back destination. In this
     * synthetic frame that destination is 0x120 bytes after abStack_384; the
     * first round loop consumes exactly that 0x120..0x17f region.
     */
    uint8_t stage0_out[0x80];
    int rc = l3_private_key_decode_stage0_expand_full_state(tables, program_f80040, program_f80040_len,
                                                        input16, object_state, object_state_len,
                                                        stage0_out);
    if (!rc) memcpy(frame + 0x120u, stage0_out, 0x80u);
    if (rc) return rc;

    l3_private_key_rng rng;
    uint32_t perm[16];
    l3_private_key_rng_seed(&rng, input16, 0x10);
    rc = l3_private_key_rng_permutation(&rng, 16, perm); /* consumed by stage0 init */
    if (rc) return rc;
    write_perm_u32(frame, perm, 16);

    /* Native FUN_f3f80040 begins with FUN_f3f02290() and uses the low byte
     * as a randomized local-stack shift.  The deobfuscated dispatcher is
     * normalized to unshifted work offsets, but the RNG state advancement
     * still affects every later FUN_f3f027b0 permutation in FUN_f3f7e1d0.
     */
    (void)l3_private_key_rng_next(&rng);

    int first_round_value = (int)total - 4;
    while (local_404 > 3) {
        rc = l3_private_key_rng_permutation(&rng, 16, perm);
        if (rc) return rc;
        write_perm_u32(frame, perm, 16);
        const round_d3c *a_tbl = (local_404 == first_round_value) ? round_first_d3c : round_later_d3c;
        for (uint32_t i = 0; i < 16; ++i) {
            rc = do_d3c_frame(tables, frame, &a_tbl[perm[i] & 15u]);
            if (rc) return rc;
        }

        rc = l3_private_key_rng_permutation(&rng, 16, perm);
        if (rc) return rc;
        write_perm_u32(frame, perm, 16);
        for (uint32_t i = 0; i < 16; ++i) {
            const round_state_c1c *op = &round_state_c1c_tbl[perm[i] & 15u];
            size_t st = (size_t)local_404 * 6u + op->state_off;
            if (st + 6u > object_state_len) return L3_OUTER_ERR_NEED_OBJECT_STATE;
            round_mix_op m = {1, op->k, op->src, 0, op->dst};
            if (op->src + 6u > FRAME_SIZE || op->dst + 6u > FRAME_SIZE) return L3_OUTER_ERR_STAGE;
            rc = l3_private_key_table_mix6_a(tables, op->k, frame + op->src, object_state + st, frame + op->dst) ? L3_OUTER_ERR_STAGE : L3_OUTER_OK;
            if (rc) return rc;
            (void)m;
        }

        uint32_t perm4[4];
        rc = l3_private_key_rng_permutation(&rng, 4, perm4);
        if (rc) return rc;
        write_perm_u32(frame, perm4, 4);
        for (uint32_t j = 0; j < 4; ++j) {
            switch (perm4[j] & 3u) {
                case 2:
                    rc = run_mix_ops(tables, frame, round_mix_case2, sizeof(round_mix_case2)/sizeof(round_mix_case2[0]));
                    break;
                case 1:
                    rc = run_mix_ops(tables, frame, round_mix_case1, sizeof(round_mix_case1)/sizeof(round_mix_case1[0]));
                    break;
                case 0:
                    rc = run_mix_ops(tables, frame, round_mix_case0, sizeof(round_mix_case0)/sizeof(round_mix_case0[0]));
                    break;
                default:
                    rc = run_mix_ops(tables, frame, round_mix_case3, sizeof(round_mix_case3)/sizeof(round_mix_case3[0]));
                    break;
            }
            if (rc) return rc;
        }

        local_404 -= 1;
    }

    memcpy(out_frame_0x400, frame, FRAME_SIZE);
    return L3_OUTER_OK;
}


int l3_private_key_decode_stage0_round_loop_and_finalize(const l3_private_key_tables *tables,
                                          const uint8_t *program_f80040,
                                          size_t program_f80040_len,
                                          const uint8_t *program_f80f0c,
                                          size_t program_f80f0c_len,
                                          const uint8_t input16[16],
                                          const uint8_t *object_state,
                                          size_t object_state_len,
                                          uint8_t out_frame_0x400[0x400]) {
    if (!tables || !program_f80040 || !program_f80f0c || !input16 || !object_state || !out_frame_0x400) {
        return L3_OUTER_ERR_ARGUMENT;
    }
    int rc = l3_private_key_decode_stage0_round_loop(tables, program_f80040, program_f80040_len,
                                           input16, object_state, object_state_len,
                                           out_frame_0x400);
    if (rc) return rc;
    /* Native passes param1 = abStack_384.  The f80f0c pre-copy reads well
     * beyond the first 0x80 bytes of that frame (for example param1+0x168),
     * so expose the whole synthetic frame as param1.  The terminal copy-back
     * destination remains r2 = &local_148 and is only 0x80 bytes wide.
     */
    rc = l3_private_key_execute_stage2_instruction_group(tables, program_f80f0c, program_f80f0c_len,
                         out_frame_0x400, FRAME_SIZE, object_state, object_state_len,
                         out_frame_0x400 + OFF_LOCAL_148, 0x80u);
    return rc ? L3_OUTER_ERR_STAGE : L3_OUTER_OK;
}


#define OFF_F4   0x290u
#define OFF_F0   0x294u
#define OFF_EE   0x296u
#define OFF_100  0x284u
#define OFF_FC   0x288u
#define OFF_FA   0x28au
#define OFF_108  0x27cu
#define OFF_106  0x27eu
#define OFF_114  0x270u
#define OFF_112  0x272u
#define OFF_120  0x264u
#define OFF_11E  0x266u
#define OFF_12C  0x258u
#define OFF_12A  0x25au
#define OFF_138  0x24cu
#define OFF_136  0x24eu
#define OFF_144  0x240u
#define OFF_142  0x242u
#define OFF_E8   0x29cu
#define OFF_7A   0x30au
#define OFF_9E   0x2e6u
#define OFF_8C   0x2f8u
#define OFF_C0   0x2c4u
#define OFF_70   0x314u
#define OFF_6C   0x318u
#define OFF_68   0x31cu
#define OFF_67   0x31du
#define OFF_66   0x31eu
#define OFF_65   0x31fu
#define OFF_63   0x321u
#define OFF_61   0x323u
#define OFF_60   0x324u
#define OFF_5F   0x325u
#define OFF_5E   0x326u
#define OFF_5C   0x328u
#define OFF_58   0x32cu
#define OFF_57   0x32du
#define OFF_56   0x32eu
#define OFF_52   0x332u
#define OFF_50   0x334u
#define OFF_4E   0x336u
#define OFF_48   0x33cu
#define OFF_40   0x344u
#define OFF_38   0x34cu
#define OFF_30   0x354u

static void wr32le(uint8_t *p, uint32_t v) { p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8); p[2]=(uint8_t)(v>>16); p[3]=(uint8_t)(v>>24); }

static uint8_t g_tail_residue[7] = {1,7,5,2,4,6,4};

static int tail_a7dc_frame(const l3_private_key_tables *tables, uint32_t k, uint32_t p2,
                           uint8_t *frame, size_t src_off, size_t p4_off, size_t dst_off) {
    uint32_t out_len = (((p2 << 14) & 0xffffffffu) >> 18) + (p2 >> 18);
    if (src_off + out_len > FRAME_SIZE || p4_off + out_len > FRAME_SIZE || dst_off + out_len > FRAME_SIZE) {
        return L3_OUTER_ERR_STAGE;
    }
    int rc = l3_private_key_table_transform(tables, k, p2, frame + src_off, frame + p4_off, frame + dst_off, out_len);
    return rc ? L3_OUTER_ERR_STAGE : L3_OUTER_OK;
}

static int tail_a7dc_out42(const l3_private_key_tables *tables, uint32_t k, uint32_t p2,
                           uint8_t *frame, size_t src_off, size_t p4_off, uint8_t out42[0x42]) {
    uint32_t out_len = (((p2 << 14) & 0xffffffffu) >> 18) + (p2 >> 18);
    if (out_len != 0x42u || src_off + out_len > FRAME_SIZE || p4_off + out_len > FRAME_SIZE) return L3_OUTER_ERR_STAGE;
    int rc = l3_private_key_table_transform(tables, k, p2, frame + src_off, frame + p4_off, out42, out_len);
    return rc ? L3_OUTER_ERR_STAGE : L3_OUTER_OK;
}

static void p4_tag10_from_prefix(uint8_t *f, size_t p3_off, uint8_t tag) {
    memset(f + OFF_70, 0, 4);
    f[OFF_70 + 3] = tag;
    /* p3_off-6 is local_xxx word, p3_off-2 is local_xxx halfword in native layout. */
    memcpy(f + OFF_6C, f + p3_off - 6u, 4);
    f[OFF_68] = f[p3_off - 2u];
    f[OFF_67] = f[p3_off - 1u];
}

static void p4_from_local7a(uint8_t *f, uint32_t word) {
    memset(f + OFF_70, 0, 4);
    wr32le(f + OFF_6C, word);
    memcpy(f + OFF_68, f + OFF_7A, 10);
}

static void p4_from_local9e_first(uint8_t *f, const uint8_t residue7[7]) {
    /* Exact ARM reshuffle after the 0x9aa4 local_9e result.
     *
     * The decompiler makes uStack_5e, uStack_57 and local_56 look
     * uninitialised here.  They are not: f3f7fd8c..f3f7fd9c performs
     *
     *   vld1.8  {d16,d17}, [local_9c]
     *   vst1.16 {d16,d17}, [uStack_5e]
     *
     * i.e. one overlapping 16-byte copy from local_9c..local_90 to
     * uStack_5e..uStack_50.  Model the physical stack write directly;
     * it is more faithful than the decompiler's CONCAT expressions.
     */
    (void)residue7;

    f[OFF_70 + 0] = 0; f[OFF_70 + 1] = 0; f[OFF_70 + 2] = 0; f[OFF_70 + 3] = 0;
    wr32le(f + OFF_6C, 0);
    f[OFF_68] = 0;
    f[OFF_67] = 0;
    f[OFF_66] = 0;
    f[OFF_65] = 0; f[OFF_65 + 1] = 0;
    f[OFF_63] = 0; f[OFF_63 + 1] = 0;

    f[OFF_61] = 6;
    f[OFF_60] = f[OFF_9E + 0];
    f[OFF_5F] = f[OFF_9E + 1];

    /* local_9c (OFF_9E+2) through local_90 is exactly 16 bytes. */
    memmove(f + OFF_5E, f + OFF_9E + 2u, 16u);
}

static void p4_from_local9e_second(uint8_t *f) {
    /* Native reshuffle after the 0x0fdc local_9e result.
     * V45 bug: this helper also zeroed the whole 34-byte p4 buffer.
     */
    f[OFF_70 + 0] = 0; f[OFF_70 + 1] = 0; f[OFF_70 + 2] = 0; f[OFF_70 + 3] = 0;
    wr32le(f + OFF_6C, 0);
    f[OFF_68] = 0;
    f[OFF_67] = 0;
    f[OFF_66] = 0;
    f[OFF_65] = 0; f[OFF_65 + 1] = 0;
    f[OFF_63] = 0; f[OFF_63 + 1] = 0;

    f[OFF_61] = 6;
    f[OFF_60] = f[OFF_9E + 0];
    f[OFF_5F] = f[OFF_9E + 1];

    /* uStack_5e = local_9c. */
    memcpy(f + OFF_5E, f + OFF_9E + 2, 2);

    /* uStack_5c = CONCAT31((int3)uStack_99,uStack_9a). */
    f[OFF_5C + 0] = f[OFF_9E + 4]; /* uStack_9a */
    f[OFF_5C + 1] = f[OFF_9E + 5]; /* first 3 bytes of uStack_99 */
    f[OFF_5C + 2] = f[OFF_9E + 6];
    f[OFF_5C + 3] = f[OFF_9E + 7];

    f[OFF_58] = f[OFF_9E + 8];     /* remaining uStack_99 byte */
    f[OFF_57] = f[OFF_9E + 9];     /* uStack_95 */
    memcpy(f + OFF_56, f + OFF_9E + 10, 4); /* local_94 */
    memcpy(f + OFF_52, f + OFF_9E + 14, 4); /* local_90 */
}

static void p4_final_from_locale8(uint8_t *f) {
    /* Final 66-byte p4: 32 zero bytes followed by 34 bytes from local_e8..local_c8. */
    memset(f + OFF_70, 0, 66);
    memcpy(f + OFF_50, f + OFF_E8, 34);
}

int l3_private_key_decode_final_state_prepare(const l3_private_key_tables *tables,
                                   uint8_t frame_0x400[0x400]) {
    if (!tables || !frame_0x400) return L3_OUTER_ERR_ARGUMENT;
    uint8_t *f = frame_0x400;
    int rc;

    p4_tag10_from_prefix(f, OFF_EE, 5);
    rc = tail_a7dc_frame(tables, 0x7cd8, 0x100060, f, OFF_EE, OFF_70, OFF_E8); if (rc) return rc;

    p4_tag10_from_prefix(f, OFF_FA, 6);
    rc = tail_a7dc_frame(tables, 0x9456, 0x100060, f, OFF_FA, OFF_70, OFF_7A); if (rc) return rc;

    p4_from_local7a(f, 0);
    rc = tail_a7dc_frame(tables, 0x3259, 0x2000a0, f, OFF_E8, OFF_70, OFF_8C); if (rc) return rc;

    p4_tag10_from_prefix(f, OFF_106, 3);
    rc = tail_a7dc_frame(tables, 0x661a, 0x100060, f, OFF_106, OFF_70, OFF_E8); if (rc) return rc;

    p4_tag10_from_prefix(f, OFF_112, 0);
    rc = tail_a7dc_frame(tables, 0x5a9d, 0x100060, f, OFF_112, OFF_70, OFF_7A); if (rc) return rc;

    p4_from_local7a(f, 0x07000000u);
    rc = tail_a7dc_frame(tables, 0x9aa4, 0x2000a0, f, OFF_E8, OFF_70, OFF_9E); if (rc) return rc;

    return L3_OUTER_OK;
}

int l3_private_key_decode_final_state_finish_with_residue(
    const l3_private_key_tables *tables,
    uint8_t frame_0x400[0x400],
    const uint8_t residue7[7],
    uint8_t out42[0x42]) {
    if (!tables || !frame_0x400 || !residue7 || !out42) {
        return L3_OUTER_ERR_ARGUMENT;
    }
    uint8_t *f = frame_0x400;
    int rc;

    p4_from_local9e_first(f, residue7);
    rc = tail_a7dc_frame(tables, 0x3173, 0x400120, f, OFF_8C, OFF_70, OFF_C0); if (rc) return rc;

    p4_tag10_from_prefix(f, OFF_11E, 3);
    rc = tail_a7dc_frame(tables, 0x1bef, 0x100060, f, OFF_11E, OFF_70, OFF_E8); if (rc) return rc;

    p4_tag10_from_prefix(f, OFF_12A, 3);
    rc = tail_a7dc_frame(tables, 0x2356, 0x100060, f, OFF_12A, OFF_70, OFF_7A); if (rc) return rc;

    p4_from_local7a(f, 0x01000000u);
    rc = tail_a7dc_frame(tables, 0xc169, 0x2000a0, f, OFF_E8, OFF_70, OFF_8C); if (rc) return rc;

    p4_tag10_from_prefix(f, OFF_136, 0);
    rc = tail_a7dc_frame(tables, 0x0675, 0x100060, f, OFF_136, OFF_70, OFF_E8); if (rc) return rc;

    p4_tag10_from_prefix(f, OFF_142, 1);
    rc = tail_a7dc_frame(tables, 0x7a85, 0x100060, f, OFF_142, OFF_70, OFF_7A); if (rc) return rc;

    p4_from_local7a(f, 0x07000000u);
    rc = tail_a7dc_frame(tables, 0x0fdc, 0x2000a0, f, OFF_E8, OFF_70, OFF_9E); if (rc) return rc;

    p4_from_local9e_second(f);
    rc = tail_a7dc_frame(tables, 0x79e7, 0x400120, f, OFF_8C, OFF_70, OFF_E8); if (rc) return rc;

    p4_final_from_locale8(f);
    rc = tail_a7dc_out42(tables, 0x96f9, 0x800220, f, OFF_C0, OFF_70, out42); if (rc) return rc;

    return L3_OUTER_OK;
}

int l3_private_key_decode_final_state_with_residue(
    const l3_private_key_tables *tables,
    uint8_t frame_0x400[0x400],
    const uint8_t residue7[7],
    uint8_t out42[0x42]) {
    int rc = l3_private_key_decode_final_state_prepare(tables, frame_0x400);
    if (rc) return rc;
    return l3_private_key_decode_final_state_finish_with_residue(
        tables, frame_0x400, residue7, out42);
}

int l3_private_key_decode_final_state(const l3_private_key_tables *tables,
                            uint8_t frame_0x400[0x400],
                            uint8_t out42[0x42]) {
    return l3_private_key_decode_final_state_with_residue(
        tables, frame_0x400, g_tail_residue, out42);
}

int l3_private_key_decode_object_block(const l3_private_key_tables *tables,
                                         const uint8_t *program_f80040,
                                         size_t program_f80040_len,
                                         const uint8_t *program_f80f0c,
                                         size_t program_f80f0c_len,
                                         const uint8_t input16[16],
                                         const uint8_t *object_state,
                                         size_t object_state_len,
                                         uint8_t out42[0x42]) {
    if (!tables || !program_f80040 || !program_f80f0c || !input16 || !object_state || !out42) return L3_OUTER_ERR_ARGUMENT;
    uint8_t frame[0x400];
    int rc = l3_private_key_decode_stage0_round_loop_and_finalize(tables, program_f80040, program_f80040_len,
                                                  program_f80f0c, program_f80f0c_len,
                                                  input16, object_state, object_state_len,
                                                  frame);
    if (rc) return rc;
    return l3_private_key_decode_final_state(tables, frame, out42);
}

int l3_private_key_decode_replay_context_init_with_state(
    l3_private_key_decode_replay_context *ctx,
    const l3_private_key_tables *tables,
    const uint8_t *program_f80040,
    size_t program_f80040_len,
    const uint8_t *program_f80f0c,
    size_t program_f80f0c_len,
    const uint8_t *object_state,
    size_t object_state_len) {
    if (!ctx || !tables || !program_f80040 || !program_f80f0c || !object_state) return L3_OUTER_ERR_ARGUMENT;
    if (object_state_len > sizeof(ctx->object_state)) return L3_OUTER_ERR_ARGUMENT;
    memset(ctx, 0, sizeof(*ctx));
    ctx->tables = tables;
    ctx->program_f80040 = program_f80040;
    ctx->program_f80040_len = program_f80040_len;
    ctx->program_f80f0c = program_f80f0c;
    ctx->program_f80f0c_len = program_f80f0c_len;
    memcpy(ctx->object_state, object_state, object_state_len);
    ctx->object_state_len = object_state_len;
    return L3_OUTER_OK;
}

int l3_private_key_decode_replay_context_init_from_manager(
    l3_private_key_decode_replay_context *ctx,
    const l3_private_key_tables *tables,
    const uint8_t *program_f80040,
    size_t program_f80040_len,
    const uint8_t *program_f80f0c,
    size_t program_f80f0c_len,
    const uint8_t *manager_a8_selected,
    size_t manager_a8_selected_len) {
    if (!ctx) return L3_OUTER_ERR_ARGUMENT;
    uint8_t state[0x5a4];
    int rc = l3_private_key_construct_object_state(manager_a8_selected, manager_a8_selected_len,
                                         state, sizeof(state));
    if (rc) return L3_OUTER_ERR_ARGUMENT;
    return l3_private_key_decode_replay_context_init_with_state(ctx, tables,
                                                      program_f80040, program_f80040_len,
                                                      program_f80f0c, program_f80f0c_len,
                                                      state, sizeof(state));
}

int l3_private_key_decode_object_method_replay_cb(void *user,
                                        const uint8_t input16[16],
                                        uint8_t out42[0x42]) {
    l3_private_key_decode_replay_context *ctx = (l3_private_key_decode_replay_context *)user;
    if (!ctx || !ctx->tables || !ctx->program_f80040 || !ctx->program_f80f0c ||
        !input16 || !out42 || ctx->object_state_len == 0) {
        return L3_OUTER_ERR_ARGUMENT;
    }
    return l3_private_key_decode_object_block(ctx->tables,
                                              ctx->program_f80040, ctx->program_f80040_len,
                                              ctx->program_f80f0c, ctx->program_f80f0c_len,
                                              input16, ctx->object_state,
                                              ctx->object_state_len, out42);
}
