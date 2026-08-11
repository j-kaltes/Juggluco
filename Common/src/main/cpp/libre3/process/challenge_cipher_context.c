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
#include "challenge_cipher_context.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define F426_BASE 0xf4263bc0u
#define F426_LEN  0x00009600u
#define DAT_BASE  0xf4295564u
#define DAT_LEN   0x00020000u

static uint8_t challenge_table_byte(const l3_challenge_block_tables *t, uint32_t addr) {
    if (addr >= F426_BASE && addr < F426_BASE + t->program_len) return t->program[addr - F426_BASE];
    if (addr >= DAT_BASE && addr < DAT_BASE + t->state_transition_table_len) return t->state_transition_table[addr - DAT_BASE];
    fprintf(stderr, "missing table byte 0x%08x\n", addr);
    abort();
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


static uint32_t load_payload_word_le32(const uint8_t *payload, uint32_t word_index) {
    const uint8_t *p = payload + word_index * 4u;
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

int l3_challenge_cipher_context_encrypt(const l3_challenge_block_tables *t,
                            const uint8_t payload[L3_CHALLENGE_BLOCK_CONTEXT_PAYLOAD_LEN],
                            const uint8_t in[16],
                            uint8_t out[16]) {
    if (!t || !t->program || !t->state_transition_table || t->program_len < F426_LEN ||
        t->state_transition_table_len < DAT_LEN || !payload || !in || !out) return -1;

    uint32_t local_48=0, local_44=0, local_40=0, local_3c=0, local_38=0, local_34=0;
    uint32_t local_2c=0, local_28=0;
    uint32_t uVar1,uVar2=0,uVar3=0,uVar4=0,uVar5=0,uVar6=0,uVar7,uVar8=0,uVar9,uVar10,uVar11,uVar12=0,uVar13=0;

    local_28 = (((uint32_t)challenge_table_byte(t,0xf42649c2u + in[0x0eu]) << 8) |
                ((uint32_t)challenge_table_byte(t,0xf42648c2u + in[0x0du]) << 16)) ^
               load_payload_word_le32(payload,3) ^
               (uint32_t)challenge_table_byte(t,0xf4264ac2u + in[0x0fu]) ^
               ((uint32_t)challenge_table_byte(t,0xf42647c2u + in[0x0cu]) << 24);
    uVar11 = (((uint32_t)challenge_table_byte(t,0xf42646c2u + in[0x0bu])) |
              ((uint32_t)challenge_table_byte(t,0xf42644c2u + in[0x09u]) << 16)) ^
             load_payload_word_le32(payload,2) ^
             ((uint32_t)challenge_table_byte(t,0xf42643c2u + in[0x08u]) << 24) ^
             ((uint32_t)challenge_table_byte(t,0xf42645c2u + in[0x0au]) << 8);
    uVar7 = (((uint32_t)challenge_table_byte(t,0xf4263fc2u + in[0x04u]) << 24) |
             ((uint32_t)challenge_table_byte(t,0xf42641c2u + in[0x06u]) << 8) |
             ((uint32_t)challenge_table_byte(t,0xf42640c2u + in[0x05u]) << 16) |
             ((uint32_t)challenge_table_byte(t,0xf42642c2u + in[0x07u]))) ^ load_payload_word_le32(payload,1);
    uVar1 = (((uint32_t)challenge_table_byte(t,0xf4263cc2u + in[0x01u]) << 16) |
             ((uint32_t)challenge_table_byte(t,0xf4263bc2u + in[0x00u]) << 24) |
             ((uint32_t)challenge_table_byte(t,0xf4263ec2u + in[0x03u])) |
             ((uint32_t)challenge_table_byte(t,0xf4263dc2u + in[0x02u]) << 8)) ^ load_payload_word_le32(payload,0);

    uVar9 = 0;
    do {
        local_38 = load_table_word_le32(t,0xf42657c4u + (uVar1 & 0xffu) * 4u) ^ load_payload_word_le32(payload,uVar9 * 4u + 5u) ^
                   load_table_word_le32(t,0xf42683c4u + (((local_28 << 16) >> 24) & 0xffu) * 4u) ^
                   load_table_word_le32(t,0xf4265bc4u + (uVar7 >> 24) * 4u) ^
                   load_table_word_le32(t,0xf4266fc4u + (((uVar11 << 8) >> 24) & 0xffu) * 4u);
        uVar8 = load_table_word_le32(t,0xf42663c4u + (((uVar7 << 16) >> 24) & 0xffu) * 4u) ^
                load_table_word_le32(t,0xf4264fc4u + (((uVar1 << 8) >> 24) & 0xffu) * 4u) ^
                load_table_word_le32(t,0xf42677c4u + (uVar11 & 0xffu) * 4u) ^ load_payload_word_le32(payload,uVar9 * 4u + 7u) ^
                load_table_word_le32(t,0xf4267bc4u + (local_28 >> 24) * 4u);
        uVar5 = load_table_word_le32(t,0xf42667c4u + (uVar7 & 0xffu) * 4u) ^
                load_table_word_le32(t,0xf4266bc4u + (uVar11 >> 24) * 4u) ^
                load_table_word_le32(t,0xf4267fc4u + (((local_28 << 8) >> 24) & 0xffu) * 4u) ^ load_payload_word_le32(payload,uVar9 * 4u + 6u) ^
                load_table_word_le32(t,0xf42653c4u + (((uVar1 << 16) >> 24) & 0xffu) * 4u);
        uVar2 = load_table_word_le32(t,0xf42673c4u + (((uVar11 << 16) >> 24) & 0xffu) * 4u) ^
                load_table_word_le32(t,0xf4265fc4u + (((uVar7 << 8) >> 24) & 0xffu) * 4u) ^ load_payload_word_le32(payload,uVar9 * 4u + 4u) ^
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
        local_28 = load_payload_word_le32(payload,uVar9 * 4u + 0x0bu) ^
                   load_table_word_le32(t,0xf42677c4u + (uVar5 & 0xffu) * 4u) ^
                   load_table_word_le32(t,0xf4267bc4u + (uVar8 >> 24) * 4u) ^
                   load_table_word_le32(t,0xf42663c4u + uVar3 * 4u) ^
                   load_table_word_le32(t,0xf4264fc4u + uVar4 * 4u);
        uVar6 = (uVar8 << 8) >> 24;
        uVar12 = (uVar2 << 16) >> 24;
        local_3c = (uVar8 << 16) >> 24;
        uVar11 = load_table_word_le32(t,0xf42653c4u + uVar12 * 4u) ^ load_table_word_le32(t,0xf4267fc4u + uVar6 * 4u) ^
                 load_table_word_le32(t,0xf4266bc4u + (uVar5 >> 24) * 4u) ^
                 load_table_word_le32(t,0xf42667c4u + (local_38 & 0xffu) * 4u) ^ load_payload_word_le32(payload,uVar9 * 4u + 10u);
        local_44 = (uVar5 << 8) >> 24;
        uVar13 = (local_38 << 8) >> 24;
        uVar7 = load_table_word_le32(t,0xf42683c4u + local_3c * 4u) ^ load_table_word_le32(t,0xf42657c4u + (uVar2 & 0xffu) * 4u) ^
                load_table_word_le32(t,0xf4266fc4u + local_44 * 4u) ^ load_payload_word_le32(payload,uVar9 * 4u + 9u) ^
                load_table_word_le32(t,0xf4265bc4u + (local_38 >> 24) * 4u);
        local_48 = (uVar5 << 16) >> 24;
        uVar1 = load_payload_word_le32(payload,uVar9 * 4u + 8u) ^ load_table_word_le32(t,0xf42673c4u + local_48 * 4u) ^
                load_table_word_le32(t,0xf4265fc4u + uVar13 * 4u) ^ load_table_word_le32(t,0xf42687c4u + (uVar8 & 0xffu) * 4u) ^
                load_table_word_le32(t,0xf4264bc4u + (uVar2 >> 24) * 4u);
        uVar9 = uVar10;
    } while (uVar10 < 10u);

    local_2c = local_38 >> 24;
    local_34 = uVar5 >> 24;
    local_38 &= 0xffu;
    local_40 = uVar2 & 0xffu;
    out[0x0] = payload[(uVar2 >> 24) + 0x0b0u];
    out[0x1] = payload[uVar13 + 0x1b0u];
    out[0x2] = payload[local_48 + 0x2b0u];
    out[0x3] = payload[(uVar8 & 0xffu) + 0x3b0u];
    out[0x4] = payload[local_2c + 0x4b0u];
    out[0x5] = payload[local_44 + 0x5b0u];
    out[0x6] = payload[local_3c + 0x6b0u];
    out[0x7] = payload[local_40 + 0x7b0u];
    out[0x8] = payload[local_34 + 0x8b0u];
    out[0x9] = payload[uVar6 + 0x9b0u];
    out[0xa] = payload[uVar12 + 0xab0u];
    out[0xb] = payload[local_38 + 0xbb0u];
    out[0xc] = payload[(uVar8 >> 24) + 0xcb0u];
    out[0xd] = payload[uVar4 + 0xdb0u];
    out[0xe] = payload[uVar3 + 0xeb0u];
    out[0xf] = payload[(uVar5 & 0xffu) + 0xfb0u];
    return 0;
}

int l3_challenge_build_context_payload(const l3_challenge_block_tables *t, const uint8_t encoded[L3_CHALLENGE_BLOCK_CONTEXT_ENCODED_LEN], uint8_t out[L3_CHALLENGE_BLOCK_CONTEXT_PAYLOAD_LEN]) {
    if (!t || !t->program || !t->state_transition_table || t->program_len < F426_LEN || t->state_transition_table_len < DAT_LEN) return -1;
    uint8_t ab86[128], aba8[128], ab52[128], ab64[128], au36[128], au40[128];
    uint8_t ab108[6], au102[128], abfc[6], auf6[128], abf0[6], auea[128], abe4[6], aude[128];
    uint8_t abd8[6], aud2[128], abcc[6], auc6[128], abc0[6], auba[128], abb4[6], auae[128];
    uint8_t tmp6[6];
    uint8_t u6,u7,u8,u9,u10,u11,u12,u13,u14,u16,u19,u20,local84,local83,local82,local81;
    uint32_t words[0x2c];
    memset(words, 0, sizeof(words));

    challenge_table_transition_emit(t,0x1bcu,0x220u,encoded,encoded,ab86);
    challenge_table_transition_skip_then_emit(t,0x80000a6u,0x220u,encoded,encoded,aba8);
    challenge_table_transition_emit(t,0x1aau,0x120u,ab86,ab86,ab52);
    challenge_table_transition_skip_then_emit(t,0x4000290u,0x120u,ab86,ab86,ab64);
    challenge_table_transition_emit(t,0x92u,0xa0u,ab52,ab52,au36);
    challenge_table_transition_skip_then_emit(t,0x2000140u,0xa0u,ab52,ab52,au40);
    challenge_table_mix_six_states(t,0x80u,au36,au36,ab108);
    challenge_table_transition_skip_then_emit(t,0x10001f4u,0x60u,au36,au36,au102);
    challenge_table_mix_six_states(t,0x272u,au40,au40,abfc);
    challenge_table_transition_skip_then_emit(t,0x10000fau,0x60u,au40,au40,auf6);
    challenge_table_transition_emit(t,0x1feu,0xa0u,ab64,ab64,au36);
    challenge_table_transition_skip_then_emit(t,0x20002f2u,0xa0u,ab64,ab64,au40);
    challenge_table_mix_six_states(t,0x5cu,au36,au36,abf0);
    challenge_table_transition_skip_then_emit(t,0x1000006u,0x60u,au36,au36,auea);
    challenge_table_mix_six_states(t,0x26cu,au40,au40,abe4);
    challenge_table_transition_skip_then_emit(t,0x10001deu,0x60u,au40,au40,aude);
    challenge_table_transition_emit(t,0x4au,0x120u,aba8,aba8,ab52);
    challenge_table_transition_skip_then_emit(t,0x4000176u,0x120u,aba8,aba8,ab64);
    challenge_table_transition_emit(t,0x40u,0xa0u,ab52,ab52,au36);
    challenge_table_transition_skip_then_emit(t,0x200006eu,0xa0u,ab52,ab52,au40);
    challenge_table_mix_six_states(t,0x242u,au36,au36,abd8);
    challenge_table_transition_skip_then_emit(t,0x100020eu,0x60u,au36,au36,aud2);
    challenge_table_mix_six_states(t,0x304u,au40,au40,abcc);
    challenge_table_transition_skip_then_emit(t,0x10002b8u,0x60u,au40,au40,auc6);
    challenge_table_transition_emit(t,0x328u,0xa0u,ab64,ab64,au36);
    challenge_table_transition_skip_then_emit(t,0x2000310u,0xa0u,ab64,ab64,au40);
    challenge_table_mix_six_states(t,0x25au,au36,au36,abc0);
    challenge_table_transition_skip_then_emit(t,0x100009cu,0x60u,au36,au36,auba);
    challenge_table_mix_six_states(t,0x2c2u,au40,au40,abb4);
    challenge_table_transition_skip_then_emit(t,0x1000332u,0x60u,au40,au40,auae);

    load_six_table_bytes(t,0xf4268f00u,tmp6); challenge_table_mix_six_states(t,0x22au,abc0,tmp6,aba8);
    load_six_table_bytes(t,0xf4268f06u,tmp6); challenge_table_mix_six_states(t,0x24eu,aba8,tmp6,ab52);
    load_six_table_bytes(t,0xf4268f0cu,tmp6); challenge_table_mix_six_states(t,0x68u,ab52,tmp6,ab64);
    challenge_table_mix_six_states(t,0x254u,ab64,ab64,ab86);
    u6=ab86[2]; u7=ab86[3]; u8=ab86[4]; u9=ab86[5];
    load_six_table_bytes(t,0xf4269012u,tmp6); challenge_table_mix_six_states(t,0x10au,auae,tmp6,aba8);
    load_six_table_bytes(t,0xf4269018u,tmp6); challenge_table_mix_six_states(t,0x15eu,aba8,tmp6,ab52);
    load_six_table_bytes(t,0xf426901eu,tmp6); challenge_table_mix_six_states(t,0x8cu,ab52,tmp6,ab64);
    challenge_table_mix_six_states(t,0x248u,ab64,ab64,ab86);
    u10=ab86[2]; u11=ab86[3]; u12=ab86[4]; u13=ab86[5];
    load_six_table_bytes(t,0xf4269124u,tmp6); challenge_table_mix_six_states(t,0x152u,abb4,tmp6,aba8);
    load_six_table_bytes(t,0xf426912au,tmp6); challenge_table_mix_six_states(t,0x16u,aba8,tmp6,ab52);
    load_six_table_bytes(t,0xf4269130u,tmp6); challenge_table_mix_six_states(t,0x122u,ab52,tmp6,ab64);
    challenge_table_mix_six_states(t,0x23cu,ab64,ab64,ab86);
    u14=ab86[4]; u19=ab86[2]; u16=ab86[3]; u20=ab86[5];
    load_six_table_bytes(t,0xf4269236u,tmp6); challenge_table_mix_six_states(t,0x2eu,auba,tmp6,aba8);
    load_six_table_bytes(t,0xf426923cu,tmp6); challenge_table_mix_six_states(t,0x198u,aba8,tmp6,ab52);
    load_six_table_bytes(t,0xf4269242u,tmp6); challenge_table_mix_six_states(t,0x27eu,ab52,tmp6,ab64);
    challenge_table_mix_six_states(t,0x2dau,ab64,ab64,ab86);
    local84=ab86[2]; local83=ab86[3]; local82=ab86[4]; local81=ab86[5];
    words[0] = (uint32_t)lookup_packed_2bit_state(t,0xf4268f12u,u6,u7,u8,u9) | ((uint32_t)lookup_packed_2bit_state(t,0xf4269024u,u10,u11,u12,u13)<<24) | ((uint32_t)lookup_packed_2bit_state(t,0xf4269136u,u19,u16,u14,u20)<<16) | ((uint32_t)lookup_packed_2bit_state(t,0xf4269248u,local84,local83,local82,local81)<<8);

    load_six_table_bytes(t,0xf4269348u,tmp6); challenge_table_mix_six_states(t,0x2e6u,auc6,tmp6,aba8);
    load_six_table_bytes(t,0xf426934eu,tmp6); challenge_table_mix_six_states(t,0x1cu,aba8,tmp6,ab52);
    load_six_table_bytes(t,0xf4269354u,tmp6); challenge_table_mix_six_states(t,0x2ecu,ab52,tmp6,ab64);
    challenge_table_mix_six_states(t,0x224u,ab64,ab64,ab86); u6=ab86[2]; u7=ab86[3]; u8=ab86[4]; u9=ab86[5];
    load_six_table_bytes(t,0xf426945au,tmp6); challenge_table_mix_six_states(t,0x2c8u,abd8,tmp6,aba8);
    load_six_table_bytes(t,0xf4269460u,tmp6); challenge_table_mix_six_states(t,0x34u,aba8,tmp6,ab52);
    load_six_table_bytes(t,0xf4269466u,tmp6); challenge_table_mix_six_states(t,0x16au,ab52,tmp6,ab64);
    challenge_table_mix_six_states(t,0x134u,ab64,ab64,ab86); u10=ab86[2]; u11=ab86[3]; u12=ab86[4]; u13=ab86[5];
    load_six_table_bytes(t,0xf426956cu,tmp6); challenge_table_mix_six_states(t,0x0u,abcc,tmp6,aba8);
    load_six_table_bytes(t,0xf4269572u,tmp6); challenge_table_mix_six_states(t,0x13au,aba8,tmp6,ab52);
    load_six_table_bytes(t,0xf4269578u,tmp6); challenge_table_mix_six_states(t,0x128u,ab52,tmp6,ab64);
    challenge_table_mix_six_states(t,0x116u,ab64,ab64,ab86); u14=ab86[4]; u19=ab86[2]; u16=ab86[3]; u20=ab86[5];
    load_six_table_bytes(t,0xf426967eu,tmp6); challenge_table_mix_six_states(t,0x158u,aud2,tmp6,aba8);
    load_six_table_bytes(t,0xf4269684u,tmp6); challenge_table_mix_six_states(t,0x1eeu,aba8,tmp6,ab52);
    load_six_table_bytes(t,0xf426968au,tmp6); challenge_table_mix_six_states(t,0x2b2u,ab52,tmp6,ab64);
    challenge_table_mix_six_states(t,0x2d4u,ab64,ab64,ab86); local84=ab86[2]; local83=ab86[3]; local82=ab86[4]; local81=ab86[5];
    words[1] = (uint32_t)lookup_packed_2bit_state(t,0xf426946cu,u10,u11,u12,u13) | ((uint32_t)lookup_packed_2bit_state(t,0xf426935au,u6,u7,u8,u9)<<24) | ((uint32_t)lookup_packed_2bit_state(t,0xf426957eu,u19,u16,u14,u20)<<16) | ((uint32_t)lookup_packed_2bit_state(t,0xf4269690u,local84,local83,local82,local81)<<8);

    load_six_table_bytes(t,0xf4269790u,tmp6); challenge_table_mix_six_states(t,0x278u,auea,tmp6,aba8);
    load_six_table_bytes(t,0xf4269796u,tmp6); challenge_table_mix_six_states(t,0x86u,aba8,tmp6,ab52);
    load_six_table_bytes(t,0xf426979cu,tmp6); challenge_table_mix_six_states(t,0x12eu,ab52,tmp6,ab64);
    challenge_table_mix_six_states(t,0x21eu,ab64,ab64,ab86); u6=ab86[2]; u7=ab86[3]; u8=ab86[4]; u9=ab86[5];
    load_six_table_bytes(t,0xf42698a2u,tmp6); challenge_table_mix_six_states(t,0x236u,abe4,tmp6,aba8);
    load_six_table_bytes(t,0xf42698a8u,tmp6); challenge_table_mix_six_states(t,0xe8u,aba8,tmp6,ab52);
    load_six_table_bytes(t,0xf42698aeu,tmp6); challenge_table_mix_six_states(t,0x260u,ab52,tmp6,ab64);
    challenge_table_mix_six_states(t,0x28au,ab64,ab64,ab86); u10=ab86[2]; u11=ab86[3]; u12=ab86[4]; u13=ab86[5];
    load_six_table_bytes(t,0xf42699b4u,tmp6); challenge_table_mix_six_states(t,0x2ceu,aude,tmp6,aba8);
    load_six_table_bytes(t,0xf42699bau,tmp6); challenge_table_mix_six_states(t,0x1e8u,aba8,tmp6,ab52);
    load_six_table_bytes(t,0xf42699c0u,tmp6); challenge_table_mix_six_states(t,0x284u,ab52,tmp6,ab64);
    challenge_table_mix_six_states(t,0x110u,ab64,ab64,ab86); u14=ab86[3]; u16=ab86[4]; u20=ab86[2]; u19=ab86[5];
    load_six_table_bytes(t,0xf4269ac6u,tmp6); challenge_table_mix_six_states(t,0x208u,abf0,tmp6,aba8);
    load_six_table_bytes(t,0xf4269accu,tmp6); challenge_table_mix_six_states(t,0x22u,aba8,tmp6,ab52);
    load_six_table_bytes(t,0xf4269ad2u,tmp6); challenge_table_mix_six_states(t,0x164u,ab52,tmp6,ab64);
    challenge_table_mix_six_states(t,0x10u,ab64,ab64,ab86); local84=ab86[2]; local83=ab86[3]; local82=ab86[4]; local81=ab86[5];
    words[2] = ((uint32_t)lookup_packed_2bit_state(t,0xf42698b4u,u10,u11,u12,u13)<<16) | ((uint32_t)lookup_packed_2bit_state(t,0xf42697a2u,u6,u7,u8,u9)<<8) | ((uint32_t)lookup_packed_2bit_state(t,0xf42699c6u,u20,u14,u16,u19)<<24) | (uint32_t)lookup_packed_2bit_state(t,0xf4269ad8u,local84,local83,local82,local81);

    load_six_table_bytes(t,0xf4269bd8u,tmp6); challenge_table_mix_six_states(t,0x19eu,auf6,tmp6,aba8);
    load_six_table_bytes(t,0xf4269bdeu,tmp6); challenge_table_mix_six_states(t,0x11cu,aba8,tmp6,ab52);
    load_six_table_bytes(t,0xf4269be4u,tmp6); challenge_table_mix_six_states(t,0x1a4u,ab52,tmp6,ab64);
    challenge_table_mix_six_states(t,0x170u,ab64,ab64,ab86); u6=ab86[2]; u7=ab86[3]; u8=ab86[4]; u9=ab86[5];
    load_six_table_bytes(t,0xf4269ceau,tmp6); challenge_table_mix_six_states(t,0x30au,au102,tmp6,aba8);
    load_six_table_bytes(t,0xf4269cf0u,tmp6); challenge_table_mix_six_states(t,0x322u,aba8,tmp6,ab52);
    load_six_table_bytes(t,0xf4269cf6u,tmp6); challenge_table_mix_six_states(t,0x28u,ab52,tmp6,ab64);
    challenge_table_mix_six_states(t,0xf4u,ab64,ab64,ab86); u10=ab86[2]; u11=ab86[3]; u12=ab86[4]; u13=ab86[5];
    load_six_table_bytes(t,0xf4269dfcu,tmp6); challenge_table_mix_six_states(t,0x62u,abfc,tmp6,aba8);
    load_six_table_bytes(t,0xf4269e02u,tmp6); challenge_table_mix_six_states(t,0x218u,aba8,tmp6,ab52);
    load_six_table_bytes(t,0xf4269e08u,tmp6); challenge_table_mix_six_states(t,0x266u,ab52,tmp6,ab64);
    challenge_table_mix_six_states(t,0x3au,ab64,ab64,ab86); u14=ab86[4]; u19=ab86[2]; u20=ab86[3]; u16=ab86[5];
    load_six_table_bytes(t,0xf4269f0eu,tmp6); challenge_table_mix_six_states(t,0x104u,ab108,tmp6,aba8);
    load_six_table_bytes(t,0xf4269f14u,tmp6); challenge_table_mix_six_states(t,0x230u,aba8,tmp6,ab52);
    load_six_table_bytes(t,0xf4269f1au,tmp6); challenge_table_mix_six_states(t,0x2e0u,ab52,tmp6,ab64);
    challenge_table_mix_six_states(t,0xeeu,ab64,ab64,ab86); local84=ab86[2]; local83=ab86[3]; local82=ab86[4]; local81=ab86[5];
    words[3] = ((uint32_t)lookup_packed_2bit_state(t,0xf4269cfcu,u10,u11,u12,u13)<<8) | ((uint32_t)lookup_packed_2bit_state(t,0xf4269beau,u6,u7,u8,u9)<<24) | ((uint32_t)lookup_packed_2bit_state(t,0xf4269e0eu,u19,u20,u14,u16)<<16) | (uint32_t)lookup_packed_2bit_state(t,0xf4269f20u,local84,local83,local82,local81);

    uint32_t uVar6 = words[3];
    for (uint32_t iVar15 = 0; iVar15 < 0xa0u; iVar15 += 0x10u) {
        uint32_t idx = iVar15 / 4u;
        uVar6 = load_table_word_le32(t,0xf426b020u+iVar15) ^ (((uVar6 >> 24) | (uVar6 << 8)) & 0xffffffffu);
        uVar6 = words[idx] ^ load_table_word_le32(t,0xf426b0c0u+iVar15) ^ (((uint32_t)challenge_table_byte(t,0xf426a120u+((uVar6>>16)&0xffu))<<16) | ((uint32_t)challenge_table_byte(t,0xf426a220u+((uVar6>>8)&0xffu))<<8) | challenge_table_byte(t,0xf426a320u+(uVar6&0xffu)) | ((uint32_t)challenge_table_byte(t,0xf426a020u+(uVar6>>24))<<24));
        words[idx+4] = uVar6;
        uint32_t uVar7_ = load_table_word_le32(t,0xf426b024u+iVar15) ^ uVar6;
        uint32_t uVar10_ = load_table_word_le32(t,0xf426b0c8u+iVar15), uVar11_ = load_table_word_le32(t,0xf426b0ccu+iVar15);
        uVar6 = words[idx+1] ^ load_table_word_le32(t,0xf426b0c4u+iVar15) ^ (((uint32_t)challenge_table_byte(t,0xf426a520u+((uVar7_>>16)&0xffu))<<16) | ((uint32_t)challenge_table_byte(t,0xf426a620u+((uVar7_>>8)&0xffu))<<8) | challenge_table_byte(t,0xf426a720u+(uVar7_&0xffu)) | ((uint32_t)challenge_table_byte(t,0xf426a420u+(uVar7_>>24))<<24));
        words[idx+5] = uVar6;
        uint32_t uVar8_ = load_table_word_le32(t,0xf426b028u+iVar15) ^ uVar6;
        uVar6 = words[idx+2] ^ uVar10_ ^ (((uint32_t)challenge_table_byte(t,0xf426a920u+((uVar8_>>16)&0xffu))<<16) | ((uint32_t)challenge_table_byte(t,0xf426aa20u+((uVar8_>>8)&0xffu))<<8) | challenge_table_byte(t,0xf426ab20u+(uVar8_&0xffu)) | ((uint32_t)challenge_table_byte(t,0xf426a820u+(uVar8_>>24))<<24));
        words[idx+6] = uVar6;
        uint32_t uVar9_ = load_table_word_le32(t,0xf426b02cu+iVar15) ^ uVar6;
        uVar6 = words[idx+3] ^ uVar11_ ^ (((uint32_t)challenge_table_byte(t,0xf426ad20u+((uVar9_>>16)&0xffu))<<16) | ((uint32_t)challenge_table_byte(t,0xf426ae20u+((uVar9_>>8)&0xffu))<<8) | challenge_table_byte(t,0xf426af20u+(uVar9_&0xffu)) | ((uint32_t)challenge_table_byte(t,0xf426ac20u+(uVar9_>>24))<<24));
        words[idx+7] = uVar6;
    }

    uint32_t uVar7w=words[0x28], uVar9w=words[0x29], uVar6w=words[0x2a], uVar8w=words[0x2b];
    words[0x28] = ((uint32_t)challenge_table_byte(t,0xf426b260u+((uVar7w>>16)&0xffu))<<16)|((uint32_t)challenge_table_byte(t,0xf426b360u+((uVar7w>>8)&0xffu))<<8)|challenge_table_byte(t,0xf426b460u+(uVar7w&0xffu))|((uint32_t)challenge_table_byte(t,0xf426b160u+(uVar7w>>24))<<24);
    uint8_t bVar2=challenge_table_byte(t,0xf426be60u+((uVar8w>>16)&0xffu));
    uint8_t bVar3=challenge_table_byte(t,0xf426c060u+(uVar8w&0xffu));
    uint8_t bVar4=challenge_table_byte(t,0xf426bd60u+(uVar8w>>24));
    uint8_t bVar5=challenge_table_byte(t,0xf426bf60u+((uVar8w>>8)&0xffu));
    uint8_t bVar1=challenge_table_byte(t,0xf426b960u+(uVar6w>>24));
    words[0x29] = ((uint32_t)challenge_table_byte(t,0xf426b660u+((uVar9w>>16)&0xffu))<<16)|((uint32_t)challenge_table_byte(t,0xf426b760u+((uVar9w>>8)&0xffu))<<8)|challenge_table_byte(t,0xf426b860u+(uVar9w&0xffu))|((uint32_t)challenge_table_byte(t,0xf426b560u+(uVar9w>>24))<<24);
    words[0x2a] = ((uint32_t)challenge_table_byte(t,0xf426ba60u+((uVar6w>>16)&0xffu))<<16)|((uint32_t)challenge_table_byte(t,0xf426bb60u+((uVar6w>>8)&0xffu))<<8)|challenge_table_byte(t,0xf426bc60u+(uVar6w&0xffu))|((uint32_t)bVar1<<24);
    words[0x2b] = ((uint32_t)bVar2<<16)|((uint32_t)bVar5<<8)|bVar3|((uint32_t)bVar4<<24);

    memset(out, 0, L3_CHALLENGE_BLOCK_CONTEXT_PAYLOAD_LEN);
    for (int i = 0; i < 0x2c; ++i) {
        out[i*4+0]=(uint8_t)(words[i]&0xffu); out[i*4+1]=(uint8_t)((words[i]>>8)&0xffu); out[i*4+2]=(uint8_t)((words[i]>>16)&0xffu); out[i*4+3]=(uint8_t)((words[i]>>24)&0xffu);
    }
    const uint32_t pos = 0x2cu * 4u;
    for (uint32_t uVar7i = 0; uVar7i < 0x10u; ++uVar7i) {
        uint32_t iVar15 = load_table_word_le32(t,0xf426c160u+uVar7i*4u);
        for (uint32_t iVar18 = 0; iVar18 < 0x100u; ++iVar18) {
            uint32_t x = load_table_word_le32(t,0xf4264bc4u + iVar18*4u + iVar15*0x400u) ^ words[(uVar7i>>2)+0x28];
            uint32_t table_idx = ((x >> ((~(uVar7i<<3)) & 0x18u)) & 0xffu) + uVar7i*0x100u;
            out[pos + uVar7i*0x100u + iVar18] = challenge_table_byte(t,0xf426c1a0u + table_idx);
        }
    }
    return 0;
}

#ifdef L3_CHALLENGE_BLOCK_CONTEXT_REF_MAIN
static uint8_t *read_file(const char *p, size_t *n) {
    FILE *f=fopen(p,"rb"); if(!f){perror(p); return NULL;}
    fseek(f,0,SEEK_END); long sz=ftell(f); fseek(f,0,SEEK_SET);
    if(sz<0){fclose(f); return NULL;}
    uint8_t *b=(uint8_t*)malloc((size_t)sz); if(!b){fclose(f); return NULL;}
    if(fread(b,1,(size_t)sz,f)!=(size_t)sz){perror("fread"); free(b); fclose(f); return NULL;}
    fclose(f); *n=(size_t)sz; return b;
}
static int write_file(const char *p, const uint8_t *b, size_t n) {
    FILE *f=fopen(p,"wb"); if(!f){perror(p); return -1;}
    int ok=fwrite(b,1,n,f)==n?0:-1; fclose(f); return ok;
}
int main(int argc, char **argv) {
    if(argc != 6) {
        fprintf(stderr,"usage: %s f426_static.bin DAT_f4295564.bin encoded_r0_0x42.bin returned_object_0x1100.bin out_payload.bin\n", argv[0]);
        return 2;
    }
    size_t nf,nd,ne,no; uint8_t *f=read_file(argv[1],&nf), *d=read_file(argv[2],&nd), *e=read_file(argv[3],&ne), *o=read_file(argv[4],&no);
    if(!f||!d||!e||!o) return 2;
    if(ne!=L3_CHALLENGE_BLOCK_CONTEXT_ENCODED_LEN || no < 4+L3_CHALLENGE_BLOCK_CONTEXT_PAYLOAD_LEN) { fprintf(stderr,"bad input sizes\n"); return 2; }
    uint8_t out[L3_CHALLENGE_BLOCK_CONTEXT_PAYLOAD_LEN];
    l3_challenge_block_tables t={f,nf,d,nd};
    int rc=l3_challenge_build_context_payload(&t,e,out);
    if(rc) return rc;
    int match = memcmp(out,o+4,L3_CHALLENGE_BLOCK_CONTEXT_PAYLOAD_LEN)==0;
    printf("match_returned_object_payload=%d\n", match);
    if(write_file(argv[5],out,L3_CHALLENGE_BLOCK_CONTEXT_PAYLOAD_LEN)) return 2;
    return match?0:1;
}
#endif
