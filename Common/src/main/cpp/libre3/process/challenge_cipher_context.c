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
/*      Sun Aug 30 10:21:11 CEST 2026                                                */

#include "challenge_cipher_context.h"
#include "whitebox_lookup_table.h"

#include <string.h>

#define F426_BASE 0xf4263bc0u
#define F426_LEN  0x00009600u
#define DAT_BASE  0xf4295564u
#define DAT_LEN   0x00020000u

static uint8_t challenge_table_byte(const l3_challenge_block_tables *t, uint32_t addr) {
    const uint32_t program_offset = addr - F426_BASE;
    if (program_offset < t->program_len) return t->program[program_offset];

    /* The public entry points validate the complete f426 program.  Every other
     * lookup belongs to the fixed white-box table; its accessor returns zero
     * for an invalid offset, avoiding a hosted stdio/abort dependency. */
    return l3_whitebox_lookup_byte((size_t)(addr - DAT_BASE));
}

static uint32_t load_table_word_le32(const l3_challenge_block_tables *t, uint32_t addr) {
    return (uint32_t)challenge_table_byte(t, addr) | ((uint32_t)challenge_table_byte(t, addr+1) << 8) | ((uint32_t)challenge_table_byte(t, addr+2) << 16) | ((uint32_t)challenge_table_byte(t, addr+3) << 24);
}

static size_t challenge_table_transition_emit(const l3_challenge_block_tables *t, uint32_t p1, uint32_t p2, const uint8_t *p3, const uint8_t *p4, uint8_t *out) {
    uint32_t uVar4 = ((p2 << 14) & 0xffffffffu) >> 18;
    uint8_t state = 0;
    size_t n = 0;
    for (uint32_t uVar3 = 0; uVar3 < uVar4; ++uVar3) {
        uint32_t idx = (((uint32_t)(state & 0xf8u) ^ p3[uVar3]) | ((uint32_t)p4[uVar3] << 8)) ^ ((uint32_t)challenge_table_byte(t, 0xf4268bc4u + uVar3 + (p1 & 0x3fffffu)) << 11);
        state = challenge_table_byte(t, DAT_BASE + idx);
        out[n++] = state & 7u;
    }
    uint32_t tail = p2 >> 18;
    for (uint32_t uVar3 = 0; uVar3 < tail; ++uVar3) {
        uint32_t idx = ((uint32_t)(state & 0xf8u) | ((uint32_t)p4[uVar4 + uVar3] << 8)) ^ ((uint32_t)challenge_table_byte(t, 0xf4268bc4u + uVar3 + (p1 & 0x3fffffu) + uVar4) << 11);
        state = challenge_table_byte(t, DAT_BASE + idx);
        out[n++] = state & 7u;
    }
    return n;
}

static size_t challenge_table_transition_skip_then_emit(const l3_challenge_block_tables *t, uint32_t p1, uint32_t p2, const uint8_t *p3, const uint8_t *p4, uint8_t *out) {
    uint32_t uVar7 = ((p2 << 14) & 0xffffffffu) >> 18;
    uint32_t uVar8 = (p1 >> 22) | ((p2 & 0xfu) << 10);
    uint32_t uVar4 = p1 & 0x3fffffu;
    uint8_t state = 0;
    size_t n = 0;
    for (uint32_t uVar6 = 0; uVar6 < uVar8; ++uVar6) {
        uint32_t idx = (((uint32_t)(state & 0xf8u) ^ p3[uVar6]) | ((uint32_t)p4[uVar6] << 8)) ^ ((uint32_t)challenge_table_byte(t, 0xf4268bc4u + uVar6 + uVar4) << 11);
        state = challenge_table_byte(t, DAT_BASE + idx);
    }
    for (uint32_t uVar6 = 0; uVar6 < uVar7; ++uVar6) {
        uint32_t idx = (((uint32_t)(state & 0xf8u) ^ p3[uVar8 + uVar6]) | ((uint32_t)p4[uVar8 + uVar6] << 8)) ^ ((uint32_t)challenge_table_byte(t, 0xf4268bc4u + uVar6 + uVar4 + uVar8) << 11);
        state = challenge_table_byte(t, DAT_BASE + idx);
        out[n++] = state & 7u;
    }
    uint32_t tail = p2 >> 18;
    for (uint32_t uVar6 = 0; uVar6 < tail; ++uVar6) {
        uint32_t idx = ((uint32_t)(state & 0xf8u) | ((uint32_t)challenge_table_byte(t, 0xf4268bc4u + uVar6 + uVar4 + uVar7 + uVar8) << 11));
        state = challenge_table_byte(t, DAT_BASE + idx);
        out[n++] = state & 7u;
    }
    return n;
}

static void challenge_table_mix_six_states(const l3_challenge_block_tables *t, uint32_t p1, const uint8_t p2[6], const uint8_t p3[6], uint8_t out[6]) {
    uint8_t bVar1 = challenge_table_byte(t, 0xf4268bc7u + p1);
    uint8_t bVar2 = challenge_table_byte(t, 0xf4268bc8u + p1);
    uint8_t bVar3 = challenge_table_byte(t, DAT_BASE + (((uint32_t)p3[0] << 8 | p2[0]) ^ ((uint32_t)challenge_table_byte(t, 0xf4268bc4u + p1) << 11)));
    out[0] = bVar3 & 7u;
    uint8_t bVar4 = challenge_table_byte(t, 0xf4268bc6u + p1);
    bVar3 = challenge_table_byte(t, DAT_BASE + (((uint32_t)p3[1] << 8 | ((bVar3 & 0xf8u) ^ p2[1])) ^ ((uint32_t)challenge_table_byte(t, 0xf4268bc5u + p1) << 11)));
    out[1] = bVar3 & 7u;
    bVar3 = challenge_table_byte(t, DAT_BASE + (((uint32_t)p3[2] << 8 | ((bVar3 & 0xf8u) ^ p2[2])) ^ ((uint32_t)bVar4 << 11)));
    out[2] = bVar3 & 7u;
    bVar4 = challenge_table_byte(t, 0xf4268bc9u + p1);
    bVar1 = challenge_table_byte(t, DAT_BASE + (((uint32_t)p3[3] << 8 | ((bVar3 & 0xf8u) ^ p2[3])) ^ ((uint32_t)bVar1 << 11)));
    out[3] = bVar1 & 7u;
    bVar1 = challenge_table_byte(t, DAT_BASE + (((uint32_t)p3[4] << 8 | ((bVar1 & 0xf8u) ^ p2[4])) ^ ((uint32_t)bVar2 << 11)));
    out[4] = bVar1 & 7u;
    out[5] = challenge_table_byte(t, DAT_BASE + (((uint32_t)p3[5] << 8 | ((bVar1 & 0xf8u) ^ p2[5])) ^ ((uint32_t)bVar4 << 11))) & 7u;
}

static uint8_t lookup_packed_2bit_state(const l3_challenge_block_tables *t, uint32_t base, uint8_t u6, uint8_t u7, uint8_t u8, uint8_t u9) {
    uint32_t idx = (u6 & 3u) | ((uint32_t)(u9 & 3u) << 6) | ((uint32_t)(u7 & 3u) << 2) | ((uint32_t)(u8 & 3u) << 4);
    return challenge_table_byte(t, base + idx);
}

static void load_six_table_bytes(const l3_challenge_block_tables *t, uint32_t addr, uint8_t out[6]) {
    for (int i = 0; i < 6; ++i) out[i] = challenge_table_byte(t, addr + (uint32_t)i);
}


enum {
    CHALLENGE_SEED_COUNT = 8,
    CHALLENGE_CHAIN_COUNT = 16
};

enum {
    CHALLENGE_TRANSITION_SKIP = 0x80000000u
};

typedef struct challenge_seed_leaf_plan {
    uint32_t transition;
    uint32_t short_mix;
    uint32_t long_transition;
} challenge_seed_leaf_plan;

typedef struct challenge_seed_middle_plan {
    uint32_t transition;
    challenge_seed_leaf_plan leaves[2];
} challenge_seed_middle_plan;

typedef struct challenge_seed_top_plan {
    uint32_t transition;
    challenge_seed_middle_plan middles[2];
} challenge_seed_top_plan;

#define CHALLENGE_SKIP(p1) (CHALLENGE_TRANSITION_SKIP | (p1))

static const challenge_seed_top_plan k_challenge_seed_plans[2] = {
    {
        0x1bcu,
        {
            {
                0x1aau,
                {
                    {0x092u, 0x080u, CHALLENGE_SKIP(0x10001f4u)},
                    {CHALLENGE_SKIP(0x2000140u), 0x272u, CHALLENGE_SKIP(0x10000fau)}
                }
            },
            {
                CHALLENGE_SKIP(0x4000290u),
                {
                    {0x1feu, 0x05cu, CHALLENGE_SKIP(0x1000006u)},
                    {CHALLENGE_SKIP(0x20002f2u), 0x26cu, CHALLENGE_SKIP(0x10001deu)}
                }
            }
        }
    },
    {
        CHALLENGE_SKIP(0x80000a6u),
        {
            {
                0x04au,
                {
                    {0x040u, 0x242u, CHALLENGE_SKIP(0x100020eu)},
                    {CHALLENGE_SKIP(0x200006eu), 0x304u, CHALLENGE_SKIP(0x10002b8u)}
                }
            },
            {
                CHALLENGE_SKIP(0x4000176u),
                {
                    {0x328u, 0x25au, CHALLENGE_SKIP(0x100009cu)},
                    {CHALLENGE_SKIP(0x2000310u), 0x2c2u, CHALLENGE_SKIP(0x1000332u)}
                }
            }
        }
    }
};

#undef CHALLENGE_SKIP

typedef struct challenge_chain_plan {
    uint16_t mix[4];
    uint8_t source;
    uint8_t output_shift;
} challenge_chain_plan;

#define CHALLENGE_LONG_SEED(index) (CHALLENGE_SEED_COUNT + (index))

static const challenge_chain_plan k_challenge_chain_plans[
    CHALLENGE_CHAIN_COUNT] = {
    {{0x22au, 0x24eu, 0x068u, 0x254u}, 6u,                     0u},
    {{0x10au, 0x15eu, 0x08cu, 0x248u}, CHALLENGE_LONG_SEED(7), 24u},
    {{0x152u, 0x016u, 0x122u, 0x23cu}, 7u,                    16u},
    {{0x02eu, 0x198u, 0x27eu, 0x2dau}, CHALLENGE_LONG_SEED(6),  8u},

    {{0x2e6u, 0x01cu, 0x2ecu, 0x224u}, CHALLENGE_LONG_SEED(5), 24u},
    {{0x2c8u, 0x034u, 0x16au, 0x134u}, 4u,                     0u},
    {{0x000u, 0x13au, 0x128u, 0x116u}, 5u,                    16u},
    {{0x158u, 0x1eeu, 0x2b2u, 0x2d4u}, CHALLENGE_LONG_SEED(4),  8u},

    {{0x278u, 0x086u, 0x12eu, 0x21eu}, CHALLENGE_LONG_SEED(2),  8u},
    {{0x236u, 0x0e8u, 0x260u, 0x28au}, 3u,                    16u},
    {{0x2ceu, 0x1e8u, 0x284u, 0x110u}, CHALLENGE_LONG_SEED(3), 24u},
    {{0x208u, 0x022u, 0x164u, 0x010u}, 2u,                     0u},

    {{0x19eu, 0x11cu, 0x1a4u, 0x170u}, CHALLENGE_LONG_SEED(1), 24u},
    {{0x30au, 0x322u, 0x028u, 0x0f4u}, CHALLENGE_LONG_SEED(0),  8u},
    {{0x062u, 0x218u, 0x266u, 0x03au}, 1u,                    16u},
    {{0x104u, 0x230u, 0x2e0u, 0x0eeu}, 0u,                     0u}
};

#undef CHALLENGE_LONG_SEED

static void challenge_run_transition(
    const l3_challenge_block_tables *t,
    uint32_t transition,
    uint32_t p2,
    const uint8_t *input,
    uint8_t *output) {
    const uint32_t p1 = transition & ~CHALLENGE_TRANSITION_SKIP;
    if ((transition & CHALLENGE_TRANSITION_SKIP) != 0u) {
        challenge_table_transition_skip_then_emit(
            t, p1, p2, input, input, output);
    } else {
        challenge_table_transition_emit(t, p1, p2, input, input, output);
    }
}

static uint8_t challenge_build_chain_byte(
    const l3_challenge_block_tables *t,
    const uint8_t source[6],
    unsigned chain_index) {
    const challenge_chain_plan *plan = &k_challenge_chain_plans[chain_index];
    const uint32_t base = 0xf4268f00u + chain_index * 0x112u;
    uint8_t constants[6];
    uint8_t state_a[6];
    uint8_t state_b[6];

    load_six_table_bytes(t, base, constants);
    challenge_table_mix_six_states(
        t, plan->mix[0], source, constants, state_a);
    load_six_table_bytes(t, base + 6u, constants);
    challenge_table_mix_six_states(
        t, plan->mix[1], state_a, constants, state_b);
    load_six_table_bytes(t, base + 12u, constants);
    challenge_table_mix_six_states(
        t, plan->mix[2], state_b, constants, state_a);
    challenge_table_mix_six_states(
        t, plan->mix[3], state_a, state_a, state_b);

    return lookup_packed_2bit_state(
        t, base + 0x12u,
        state_b[2], state_b[3], state_b[4], state_b[5]);
}

static uint32_t challenge_substitute_word(
    const l3_challenge_block_tables *t,
    uint32_t table_base,
    uint32_t word) {
    return ((uint32_t)challenge_table_byte(
                t, table_base + ((word >> 24) & 0xffu)) << 24) |
           ((uint32_t)challenge_table_byte(
                t, table_base + 0x100u + ((word >> 16) & 0xffu)) << 16) |
           ((uint32_t)challenge_table_byte(
                t, table_base + 0x200u + ((word >> 8) & 0xffu)) << 8) |
           (uint32_t)challenge_table_byte(
                t, table_base + 0x300u + (word & 0xffu));
}

static uint8_t challenge_context_output_byte(
    const l3_challenge_block_tables *t,
    const l3_challenge_block_context *context,
    unsigned lane,
    uint8_t value) {
    /* The sixteen source-table selectors stored at f426c160 are the
     * permutation (lane * 5) mod 16.  Only one byte of the selected 32-bit
     * table word contributes to this lane, so avoid loading the other three. */
    const uint32_t source_table = (lane * 5u) & 0x0fu;
    const uint32_t shift = (~(lane << 3u)) & 0x18u;
    const uint8_t x =
        challenge_table_byte(
            t, 0xf4264bc4u + (uint32_t)value * 4u +
               source_table * 0x400u + (shift >> 3u)) ^
        (uint8_t)(context->words[0x28u + (lane >> 2u)] >> shift);
    const uint32_t table_index =
        (uint32_t)x + lane * 0x100u;
    return challenge_table_byte(t, 0xf426c1a0u + table_index);
}

#ifdef L3_CHALLENGE_CONTEXT_TEST_HOOKS
uint8_t l3_challenge_context_test_output_byte(
    const l3_challenge_block_tables *tables,
    const l3_challenge_block_context *context,
    unsigned lane,
    uint8_t value) {
    if (!tables || !context || lane >= 16u) return 0u;
    return challenge_context_output_byte(tables, context, lane, value);
}
#endif

int l3_challenge_cipher_context_encrypt(const l3_challenge_block_tables *t,
                            const l3_challenge_block_context *context,
                            const uint8_t in[16],
                            uint8_t out[16]) {
    if (!t || !t->program || t->program_len < F426_LEN || !context || !in || !out) return -1;

    uint32_t local_48=0, local_44=0, local_40=0, local_3c=0, local_38=0, local_34=0;
    uint32_t local_2c=0, local_28=0;
    uint32_t uVar1,uVar2=0,uVar3=0,uVar4=0,uVar5=0,uVar6=0,uVar7,uVar8=0,uVar9,uVar10,uVar11,uVar12=0,uVar13=0;

    local_28 = (((uint32_t)challenge_table_byte(t,0xf42649c2u + in[0x0eu]) << 8) |
                ((uint32_t)challenge_table_byte(t,0xf42648c2u + in[0x0du]) << 16)) ^
               context->words[3] ^
               (uint32_t)challenge_table_byte(t,0xf4264ac2u + in[0x0fu]) ^
               ((uint32_t)challenge_table_byte(t,0xf42647c2u + in[0x0cu]) << 24);
    uVar11 = (((uint32_t)challenge_table_byte(t,0xf42646c2u + in[0x0bu])) |
              ((uint32_t)challenge_table_byte(t,0xf42644c2u + in[0x09u]) << 16)) ^
             context->words[2] ^
             ((uint32_t)challenge_table_byte(t,0xf42643c2u + in[0x08u]) << 24) ^
             ((uint32_t)challenge_table_byte(t,0xf42645c2u + in[0x0au]) << 8);
    uVar7 = (((uint32_t)challenge_table_byte(t,0xf4263fc2u + in[0x04u]) << 24) |
             ((uint32_t)challenge_table_byte(t,0xf42641c2u + in[0x06u]) << 8) |
             ((uint32_t)challenge_table_byte(t,0xf42640c2u + in[0x05u]) << 16) |
             ((uint32_t)challenge_table_byte(t,0xf42642c2u + in[0x07u]))) ^ context->words[1];
    uVar1 = (((uint32_t)challenge_table_byte(t,0xf4263cc2u + in[0x01u]) << 16) |
             ((uint32_t)challenge_table_byte(t,0xf4263bc2u + in[0x00u]) << 24) |
             ((uint32_t)challenge_table_byte(t,0xf4263ec2u + in[0x03u])) |
             ((uint32_t)challenge_table_byte(t,0xf4263dc2u + in[0x02u]) << 8)) ^ context->words[0];

    uVar9 = 0;
    do {
        local_38 = load_table_word_le32(t,0xf42657c4u + (uVar1 & 0xffu) * 4u) ^ context->words[uVar9 * 4u + 5u] ^
                   load_table_word_le32(t,0xf42683c4u + (((local_28 << 16) >> 24) & 0xffu) * 4u) ^
                   load_table_word_le32(t,0xf4265bc4u + (uVar7 >> 24) * 4u) ^
                   load_table_word_le32(t,0xf4266fc4u + (((uVar11 << 8) >> 24) & 0xffu) * 4u);
        uVar8 = load_table_word_le32(t,0xf42663c4u + (((uVar7 << 16) >> 24) & 0xffu) * 4u) ^
                load_table_word_le32(t,0xf4264fc4u + (((uVar1 << 8) >> 24) & 0xffu) * 4u) ^
                load_table_word_le32(t,0xf42677c4u + (uVar11 & 0xffu) * 4u) ^ context->words[uVar9 * 4u + 7u] ^
                load_table_word_le32(t,0xf4267bc4u + (local_28 >> 24) * 4u);
        uVar5 = load_table_word_le32(t,0xf42667c4u + (uVar7 & 0xffu) * 4u) ^
                load_table_word_le32(t,0xf4266bc4u + (uVar11 >> 24) * 4u) ^
                load_table_word_le32(t,0xf4267fc4u + (((local_28 << 8) >> 24) & 0xffu) * 4u) ^ context->words[uVar9 * 4u + 6u] ^
                load_table_word_le32(t,0xf42653c4u + (((uVar1 << 16) >> 24) & 0xffu) * 4u);
        uVar2 = load_table_word_le32(t,0xf42673c4u + (((uVar11 << 16) >> 24) & 0xffu) * 4u) ^
                load_table_word_le32(t,0xf4265fc4u + (((uVar7 << 8) >> 24) & 0xffu) * 4u) ^ context->words[uVar9 * 4u + 4u] ^
                load_table_word_le32(t,0xf4264bc4u + (uVar1 >> 24) * 4u) ^
                load_table_word_le32(t,0xf42687c4u + (local_28 & 0xffu) * 4u);
        if (uVar9 == 8u) {
            uVar6 = (uVar8 << 8) >> 24;
            local_3c = (uVar8 << 16) >> 24;
            local_44 = (uVar5 << 8) >> 24;
            local_48 = (uVar5 << 16) >> 24;
            uVar3 = (local_38 << 16) >> 24;
            uVar13 = (local_38 << 8) >> 24;
            uVar4 = (uVar2 << 8) >> 24;
            uVar12 = (uVar2 << 16) >> 24;
            break;
        }
        uVar10 = uVar9 + 2u;
        uVar3 = (local_38 << 16) >> 24;
        uVar4 = (uVar2 << 8) >> 24;
        local_28 = context->words[uVar9 * 4u + 0x0bu] ^
                   load_table_word_le32(t,0xf42677c4u + (uVar5 & 0xffu) * 4u) ^
                   load_table_word_le32(t,0xf4267bc4u + (uVar8 >> 24) * 4u) ^
                   load_table_word_le32(t,0xf42663c4u + uVar3 * 4u) ^
                   load_table_word_le32(t,0xf4264fc4u + uVar4 * 4u);
        uVar6 = (uVar8 << 8) >> 24;
        uVar12 = (uVar2 << 16) >> 24;
        local_3c = (uVar8 << 16) >> 24;
        uVar11 = load_table_word_le32(t,0xf42653c4u + uVar12 * 4u) ^ load_table_word_le32(t,0xf4267fc4u + uVar6 * 4u) ^
                 load_table_word_le32(t,0xf4266bc4u + (uVar5 >> 24) * 4u) ^
                 load_table_word_le32(t,0xf42667c4u + (local_38 & 0xffu) * 4u) ^ context->words[uVar9 * 4u + 10u];
        local_44 = (uVar5 << 8) >> 24;
        uVar13 = (local_38 << 8) >> 24;
        uVar7 = load_table_word_le32(t,0xf42683c4u + local_3c * 4u) ^ load_table_word_le32(t,0xf42657c4u + (uVar2 & 0xffu) * 4u) ^
                load_table_word_le32(t,0xf4266fc4u + local_44 * 4u) ^ context->words[uVar9 * 4u + 9u] ^
                load_table_word_le32(t,0xf4265bc4u + (local_38 >> 24) * 4u);
        local_48 = (uVar5 << 16) >> 24;
        uVar1 = context->words[uVar9 * 4u + 8u] ^ load_table_word_le32(t,0xf42673c4u + local_48 * 4u) ^
                load_table_word_le32(t,0xf4265fc4u + uVar13 * 4u) ^ load_table_word_le32(t,0xf42687c4u + (uVar8 & 0xffu) * 4u) ^
                load_table_word_le32(t,0xf4264bc4u + (uVar2 >> 24) * 4u);
        uVar9 = uVar10;
    } while (uVar10 < 10u);

    local_2c = local_38 >> 24;
    local_34 = uVar5 >> 24;
    local_38 &= 0xffu;
    local_40 = uVar2 & 0xffu;
    out[0x0] = challenge_context_output_byte(t, context, 0u, (uint8_t)(uVar2 >> 24));
    out[0x1] = challenge_context_output_byte(t, context, 1u, (uint8_t)uVar13);
    out[0x2] = challenge_context_output_byte(t, context, 2u, (uint8_t)local_48);
    out[0x3] = challenge_context_output_byte(t, context, 3u, (uint8_t)uVar8);
    out[0x4] = challenge_context_output_byte(t, context, 4u, (uint8_t)local_2c);
    out[0x5] = challenge_context_output_byte(t, context, 5u, (uint8_t)local_44);
    out[0x6] = challenge_context_output_byte(t, context, 6u, (uint8_t)local_3c);
    out[0x7] = challenge_context_output_byte(t, context, 7u, (uint8_t)local_40);
    out[0x8] = challenge_context_output_byte(t, context, 8u, (uint8_t)local_34);
    out[0x9] = challenge_context_output_byte(t, context, 9u, (uint8_t)uVar6);
    out[0xa] = challenge_context_output_byte(t, context, 10u, (uint8_t)uVar12);
    out[0xb] = challenge_context_output_byte(t, context, 11u, (uint8_t)local_38);
    out[0xc] = challenge_context_output_byte(t, context, 12u, (uint8_t)(uVar8 >> 24));
    out[0xd] = challenge_context_output_byte(t, context, 13u, (uint8_t)uVar4);
    out[0xe] = challenge_context_output_byte(t, context, 14u, (uint8_t)uVar3);
    out[0xf] = challenge_context_output_byte(t, context, 15u, (uint8_t)uVar5);
    return 0;
}


typedef struct challenge_seed_workspace {
    /* Output lengths encoded by p2 = 0x220, 0x120, 0xa0 and 0x60. */
    uint8_t top[0x22];
    uint8_t middle[0x12];
    uint8_t leaf[0x0a];
    uint8_t long_seed[0x06];
    uint8_t short_seed[6];
} challenge_seed_workspace;

int l3_challenge_block_context_init(
    const l3_challenge_block_tables *t,
    const uint8_t encoded[L3_CHALLENGE_BLOCK_CONTEXT_ENCODED_LEN],
    l3_challenge_block_context *context) {
    if (!t || !t->program || t->program_len < F426_LEN ||
        !encoded || !context) {
        return -1;
    }

    uint32_t base_words[4];
    challenge_seed_workspace work;
    for (unsigned top_index = 0; top_index < 2u; ++top_index) {
        const challenge_seed_top_plan *top_plan =
            &k_challenge_seed_plans[top_index];
        challenge_run_transition(
            t, top_plan->transition, 0x220u, encoded, work.top);

        for (unsigned middle_index = 0; middle_index < 2u; ++middle_index) {
            const challenge_seed_middle_plan *middle_plan =
                &top_plan->middles[middle_index];
            const unsigned word_index =
                3u - (top_index * 2u + middle_index);
            const unsigned first_seed =
                top_index * 4u + middle_index * 2u;
            uint32_t word = 0u;

            challenge_run_transition(
                t, middle_plan->transition, 0x120u,
                work.top, work.middle);
            for (unsigned leaf_index = 0; leaf_index < 2u; ++leaf_index) {
                const challenge_seed_leaf_plan *leaf_plan =
                    &middle_plan->leaves[leaf_index];
                const unsigned seed = first_seed + leaf_index;
                challenge_run_transition(
                    t, leaf_plan->transition, 0xa0u,
                    work.middle, work.leaf);
                challenge_table_mix_six_states(
                    t, leaf_plan->short_mix,
                    work.leaf, work.leaf, work.short_seed);
                challenge_run_transition(
                    t, leaf_plan->long_transition, 0x60u,
                    work.leaf, work.long_seed);

                for (unsigned chain = word_index * 4u;
                     chain < word_index * 4u + 4u; ++chain) {
                    const challenge_chain_plan *chain_plan =
                        &k_challenge_chain_plans[chain];
                    const uint8_t *source = NULL;
                    if (chain_plan->source == seed) {
                        source = work.short_seed;
                    } else if (chain_plan->source ==
                               CHALLENGE_SEED_COUNT + seed) {
                        source = work.long_seed;
                    }
                    if (source) {
                        word |=
                            (uint32_t)challenge_build_chain_byte(
                                t, source, chain)
                            << chain_plan->output_shift;
                    }
                }
            }
            base_words[word_index] = word;
        }
    }

    /* Delay the first context write until both top-level transitions have
     * consumed the encoded frame, preserving supported in-place operation. */
    memcpy(context->words, base_words, sizeof(base_words));

    uint32_t previous = context->words[3];
    for (unsigned round = 0; round < 10u; ++round) {
        const uint32_t round_offset = round * 0x10u;
        const unsigned word_index = round * 4u;
        uint32_t chain =
            load_table_word_le32(t, 0xf426b020u + round_offset) ^
            ((previous >> 24) | (previous << 8));

        for (unsigned lane = 0; lane < 4u; ++lane) {
            const uint32_t next =
                context->words[word_index + lane] ^
                load_table_word_le32(
                    t, 0xf426b0c0u + round_offset + lane * 4u) ^
                challenge_substitute_word(
                    t, 0xf426a020u + lane * 0x400u, chain);
            context->words[word_index + 4u + lane] = next;
            if (lane != 3u) {
                chain =
                    load_table_word_le32(
                        t, 0xf426b024u + round_offset + lane * 4u) ^
                    next;
            } else {
                previous = next;
            }
        }
    }

    for (unsigned lane = 0; lane < 4u; ++lane) {
        context->words[0x28u + lane] = challenge_substitute_word(
            t, 0xf426b160u + lane * 0x400u,
            context->words[0x28u + lane]);
    }
    return 0;
}
