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
#include "challenge_whitebox_block_cipher.h"

#include <cstdint>
#include <cstring>
#include <cstdlib>

#include "whitebox_lookup_table.inc"
#include "challenge_block_schedule.inc"

static constexpr uint32_t K_DAT41B_BASE = 0xf41b3743u;
static constexpr uint32_t K_DAT41B_SIZE = sizeof(k_dat41b);
static constexpr uint32_t K_DAT429_BASE = 0xf4295564u;
static constexpr uint32_t K_DAT429_SIZE = sizeof(k_dat429);

static inline uint8_t challenge_program_byte(uint32_t addr) {
    if (addr < K_DAT41B_BASE || addr - K_DAT41B_BASE >= K_DAT41B_SIZE) return 0;
    return k_dat41b[addr - K_DAT41B_BASE];
}
static inline uint8_t challenge_transition_byte(uint32_t idx) {
    return idx < K_DAT429_SIZE ? k_dat429[idx] : 0;
}
static inline uint8_t challenge_lookup_byte(uint32_t addr) {
    if (addr >= K_DAT41B_BASE && addr - K_DAT41B_BASE < K_DAT41B_SIZE) return challenge_program_byte(addr);
    if (addr >= K_DAT429_BASE && addr - K_DAT429_BASE < K_DAT429_SIZE) return challenge_transition_byte(addr - K_DAT429_BASE);
    return 0;
}
static inline uint32_t challenge_table_word_le32(uint32_t addr) {
    return (uint32_t)challenge_lookup_byte(addr) | ((uint32_t)challenge_lookup_byte(addr+1u)<<8) | ((uint32_t)challenge_lookup_byte(addr+2u)<<16) | ((uint32_t)challenge_lookup_byte(addr+3u)<<24);
}
static inline uint32_t pack_u8_u8(uint32_t a, uint32_t b) { return ((a & 0xffu) << 8) | (b & 0xffu); }
static inline uint32_t pack_u8_u16(uint32_t a, uint32_t b) { return ((a & 0xffu) << 16) | (b & 0xffffu); }
static inline uint32_t pack_u8_u24(uint32_t a, uint32_t b) { return ((a & 0xffu) << 24) | (b & 0xffffffu); }

static constexpr uint32_t pack_four_2bit_values(
    uint32_t bits_0_1, uint32_t bits_6_7,
    uint32_t bits_2_3, uint32_t bits_4_5) noexcept {
    return (bits_0_1 & 3u) |
           ((bits_6_7 & 3u) << 6) |
           ((bits_2_3 & 3u) << 2) |
           ((bits_4_5 & 3u) << 4);
}

static void challenge_table_transform_emit(uint32_t param_1, uint32_t param_2, const uint8_t *param_3, const uint8_t *param_4, uint8_t *param_5) {
    uint32_t uVar1, uVar2, uVar3, uVar4;
    uVar4 = (param_2 << 0xe) >> 0x12;
    if (uVar4 == 0) { uVar1 = 0; }
    else {
        uVar3 = 0; uVar1 = 0;
        do {
            uVar2 = uVar1 & 0xf8u;
            uint32_t idx = ((uVar2 ^ param_3[uVar3]) | ((uint32_t)param_4[uVar3] << 8)) ^ ((uint32_t)challenge_program_byte(0xf41b8744u + uVar3 + (param_1 & 0x3fffffu)) << 0xb);
            uVar1 = challenge_transition_byte(idx);
            param_5[uVar3] = (uint8_t)(uVar1 & 7u);
            uVar3++;
        } while (uVar3 < uVar4);
    }
    if (param_2 >> 0x12) {
        uVar3 = 0;
        do {
            uVar2 = uVar1 & 0xf8u;
            uint32_t idx = (uVar2 | ((uint32_t)param_4[uVar4 + uVar3] << 8)) ^ ((uint32_t)challenge_program_byte(0xf41b8744u + uVar3 + (param_1 & 0x3fffffu) + uVar4) << 0xb);
            uVar1 = challenge_transition_byte(idx);
            param_5[uVar4 + uVar3] = (uint8_t)(uVar1 & 7u);
            uVar3++;
        } while (uVar3 < (param_2 >> 0x12));
    }
}

static void challenge_table_transform_skip_emit(uint32_t param_1, uint32_t param_2, const uint8_t *param_3, const uint8_t *param_4, uint8_t *param_5) {
    uint32_t uVar4,uVar5,uVar6,uVar7,uVar8,uVar9;
    uVar7 = (param_2 << 0xe) >> 0x12;
    uVar8 = (param_1 >> 0x16) | ((param_2 & 0xfu) << 10);
    uVar4 = param_1 & 0x3fffffu;
    if (uVar8 == 0) { uVar9 = 0; }
    else {
        uVar6 = 0; uVar9 = 0;
        do {
            uint32_t idx = (((uVar9 & 0xf8u) ^ param_3[uVar6]) | ((uint32_t)param_4[uVar6] << 8)) ^ ((uint32_t)challenge_program_byte(0xf41b8744u + uVar6 + uVar4) << 0xb);
            uVar9 = challenge_transition_byte(idx);
            uVar6++;
        } while (uVar6 < uVar8);
    }
    if (uVar7 != 0) {
        uVar6 = 0;
        do {
            uVar5 = uVar9 & 0xf8u;
            uint32_t idx = ((uVar5 ^ param_3[uVar8 + uVar6]) | ((uint32_t)param_4[uVar8 + uVar6] << 8)) ^ ((uint32_t)challenge_program_byte(0xf41b8744u + uVar6 + uVar4 + uVar8) << 0xb);
            uVar9 = challenge_transition_byte(idx);
            param_5[uVar6] = (uint8_t)(uVar9 & 7u);
            uVar6++;
        } while (uVar6 < uVar7);
    }
    if (param_2 >> 0x12) {
        uVar6 = 0;
        do {
            uVar5 = uVar9 & 0xf8u;
            uint32_t idx = uVar5 | ((uint32_t)challenge_program_byte(0xf41b8744u + uVar6 + uVar4 + uVar7 + uVar8) << 0xb);
            uVar9 = challenge_transition_byte(idx);
            param_5[uVar7 + uVar6] = (uint8_t)(uVar9 & 7u);
            uVar6++;
        } while (uVar6 < (param_2 >> 0x12));
    }
}

static void challenge_mix_six_lanes(uint32_t param_1, const uint8_t *param_2, const uint8_t *param_3, uint8_t *param_4) {
    uint8_t bVar1,bVar2,bVar3,bVar4;
    bVar1 = challenge_program_byte(0xf41b8747u + param_1);
    bVar2 = challenge_program_byte(0xf41b8748u + param_1);
    bVar3 = challenge_transition_byte(pack_u8_u8(param_3[0], param_2[0]) ^ ((uint32_t)challenge_program_byte(0xf41b8744u + param_1) << 0xb));
    param_4[0] = (uint8_t)(bVar3 & 7u);
    bVar4 = challenge_program_byte(0xf41b8746u + param_1);
    bVar3 = challenge_transition_byte(pack_u8_u8(param_3[1], (bVar3 & 0xf8u) ^ param_2[1]) ^ ((uint32_t)challenge_program_byte(0xf41b8745u + param_1) << 0xb));
    param_4[1] = (uint8_t)(bVar3 & 7u);
    bVar3 = challenge_transition_byte(pack_u8_u8(param_3[2], (bVar3 & 0xf8u) ^ param_2[2]) ^ ((uint32_t)bVar4 << 0xb));
    param_4[2] = (uint8_t)(bVar3 & 7u);
    bVar4 = challenge_program_byte(0xf41b8749u + param_1);
    bVar1 = challenge_transition_byte(pack_u8_u8(param_3[3], (bVar3 & 0xf8u) ^ param_2[3]) ^ ((uint32_t)bVar1 << 0xb));
    param_4[3] = (uint8_t)(bVar1 & 7u);
    bVar1 = challenge_transition_byte(pack_u8_u8(param_3[4], (bVar1 & 0xf8u) ^ param_2[4]) ^ ((uint32_t)bVar2 << 0xb));
    param_4[4] = (uint8_t)(bVar1 & 7u);
    param_4[5] = (uint8_t)(challenge_transition_byte(pack_u8_u8(param_3[5], (bVar1 & 0xf8u) ^ param_2[5]) ^ ((uint32_t)bVar4 << 0xb)) & 7u);
}

static void challenge_mix_six_lanes(uint32_t param_1, const uint8_t *param_2, uint32_t param_3_addr, uint8_t *param_4) {
    uint8_t bVar1,bVar2,bVar3,bVar4;
    bVar1 = challenge_program_byte(0xf41b8747u + param_1);
    bVar2 = challenge_program_byte(0xf41b8748u + param_1);
    bVar3 = challenge_transition_byte(pack_u8_u8(challenge_lookup_byte(param_3_addr), param_2[0]) ^ ((uint32_t)challenge_program_byte(0xf41b8744u + param_1) << 0xb));
    param_4[0] = (uint8_t)(bVar3 & 7u);
    bVar4 = challenge_program_byte(0xf41b8746u + param_1);
    bVar3 = challenge_transition_byte(pack_u8_u8(challenge_lookup_byte(param_3_addr+1u), (bVar3 & 0xf8u) ^ param_2[1]) ^ ((uint32_t)challenge_program_byte(0xf41b8745u + param_1) << 0xb));
    param_4[1] = (uint8_t)(bVar3 & 7u);
    bVar3 = challenge_transition_byte(pack_u8_u8(challenge_lookup_byte(param_3_addr+2u), (bVar3 & 0xf8u) ^ param_2[2]) ^ ((uint32_t)bVar4 << 0xb));
    param_4[2] = (uint8_t)(bVar3 & 7u);
    bVar4 = challenge_program_byte(0xf41b8749u + param_1);
    bVar1 = challenge_transition_byte(pack_u8_u8(challenge_lookup_byte(param_3_addr+3u), (bVar3 & 0xf8u) ^ param_2[3]) ^ ((uint32_t)bVar1 << 0xb));
    param_4[3] = (uint8_t)(bVar1 & 7u);
    bVar1 = challenge_transition_byte(pack_u8_u8(challenge_lookup_byte(param_3_addr+4u), (bVar1 & 0xf8u) ^ param_2[4]) ^ ((uint32_t)bVar2 << 0xb));
    param_4[4] = (uint8_t)(bVar1 & 7u);
    param_4[5] = (uint8_t)(challenge_transition_byte(pack_u8_u8(challenge_lookup_byte(param_3_addr+5u), (bVar1 & 0xf8u) ^ param_2[5]) ^ ((uint32_t)bVar4 << 0xb)) & 7u);
}

static void challenge_expand_schedule_groups(const uint8_t *param_1, uint8_t *param_2, uint8_t *param_3, uint8_t *param_4, uint8_t *param_5,
                   uint8_t *param_6, uint8_t *param_7, uint8_t *param_8, uint8_t *param_9, uint8_t *param_10,
                   uint8_t *param_11, uint8_t *param_12, uint8_t *param_13, uint8_t *param_14, uint8_t *param_15,
                   uint8_t *param_16, uint8_t *param_17) {
    uint8_t auStack_a4[34], auStack_82[34], auStack_60[18], auStack_4e[18], auStack_3c[10], auStack_32[10];
    challenge_table_transform_emit(0x29c,0x220,param_1,param_1,auStack_82);
    challenge_table_transform_skip_emit(0x8000048,0x220,param_1,param_1,auStack_a4);
    challenge_table_transform_emit(0x250,0x120,auStack_82,auStack_82,auStack_4e);
    challenge_table_transform_skip_emit(0x40001a6,0x120,auStack_82,auStack_82,auStack_60);
    challenge_table_transform_emit(0x1e0,0xa0,auStack_4e,auStack_4e,auStack_32);
    challenge_table_transform_skip_emit(0x20001c8,0xa0,auStack_4e,auStack_4e,auStack_3c);
    challenge_mix_six_lanes(0x388,auStack_32,auStack_32,param_2);
    challenge_table_transform_skip_emit(0x1000318,0x60,auStack_32,auStack_32,param_3);
    challenge_mix_six_lanes(0x486,auStack_3c,auStack_3c,param_4);
    challenge_table_transform_skip_emit(0x1000454,0x60,auStack_3c,auStack_3c,param_5);
    challenge_table_transform_emit(0x44a,0xa0,auStack_60,auStack_60,auStack_32);
    challenge_table_transform_skip_emit(0x20002e8,0xa0,auStack_60,auStack_60,auStack_3c);
    challenge_mix_six_lanes(0x358,auStack_32,auStack_32,param_6);
    challenge_table_transform_skip_emit(0x1000136,0x60,auStack_32,auStack_32,param_7);
    challenge_mix_six_lanes(0x150,auStack_3c,auStack_3c,param_8);
    challenge_table_transform_skip_emit(0x1000422,0x60,auStack_3c,auStack_3c,param_9);
    challenge_table_transform_emit(0x1ea,0x120,auStack_a4,auStack_a4,auStack_4e);
    challenge_table_transform_skip_emit(0x4000184,0x120,auStack_a4,auStack_a4,auStack_60);
    challenge_table_transform_emit(0x3a6,0xa0,auStack_4e,auStack_4e,auStack_32);
    challenge_table_transform_skip_emit(0x2000370,0xa0,auStack_4e,auStack_4e,auStack_3c);
    challenge_mix_six_lanes(0x18,auStack_32,auStack_32,param_10);
    challenge_table_transform_skip_emit(0x100012c,0x60,auStack_32,auStack_32,param_11);
    challenge_mix_six_lanes(0x202,auStack_3c,auStack_3c,param_12);
    challenge_table_transform_skip_emit(0x1000156,0x60,auStack_3c,auStack_3c,param_13);
    challenge_table_transform_emit(0x140,0xa0,auStack_60,auStack_60,auStack_32);
    challenge_table_transform_skip_emit(0x20003fe,0xa0,auStack_60,auStack_60,auStack_3c);
    challenge_mix_six_lanes(0x126,auStack_32,auStack_32,param_14);
    challenge_table_transform_skip_emit(0x1000286,0x60,auStack_32,auStack_32,param_15);
    challenge_mix_six_lanes(0x9c,auStack_3c,auStack_3c,param_16);
    challenge_table_transform_skip_emit(0x100047c,0x60,auStack_3c,auStack_3c,param_17);
}

static void challenge_expand_frame_to_schedule(const uint8_t *param_1, uint32_t *param_3)

{
  uint8_t bVar1;
  uint8_t bVar2;
  uint8_t bVar3;
  uint8_t bVar4;
  uint8_t bVar5;
  uint32_t uVar6;
  uint32_t uVar7;
  uint32_t uVar8;
  uint32_t uVar9;
  uint32_t uVar10;
  uint32_t uVar11;
  uint32_t uVar12;
  uint32_t uVar13;
  uint32_t uVar14;
  uint32_t *puVar15;
  int32_t iVar16;
  int32_t iVar17;
  uint32_t uVar18;
  uint32_t uVar19;
  uint32_t uVar20;
  int32_t iVar21;
  uint32_t local_b4;
  uint32_t local_ac_addr;
  uint32_t local_a8_addr;
  uint8_t abStack_a4 [6];
  uint8_t auStack_9e [6];
  uint8_t abStack_98 [6];
  uint8_t auStack_92 [6];
  uint8_t abStack_8c [6];
  uint8_t auStack_86 [6];
  uint8_t abStack_80 [6];
  uint8_t auStack_7a [6];
  uint8_t abStack_74 [6];
  uint8_t auStack_6e [6];
  uint8_t abStack_68 [6];
  uint8_t auStack_62 [6];
  uint8_t abStack_5c [6];
  uint8_t auStack_56 [6];
  uint8_t abStack_50 [6];
  uint8_t auStack_4a [6];
  uint8_t abStack_44 [6];
  uint8_t abStack_3e [6];
  uint8_t abStack_38 [6];
  uint8_t abStack_32 [6];
#define local_30 abStack_32[2]
#define local_2f abStack_32[3]
#define local_2e abStack_32[4]
#define local_2d abStack_32[5]
challenge_expand_schedule_groups(param_1,abStack_a4,auStack_9e,abStack_98,auStack_92,abStack_8c,
               auStack_86,abStack_80,auStack_7a,abStack_74,auStack_6e,abStack_68,
               auStack_62,abStack_5c,auStack_56,abStack_50,auStack_4a);
  challenge_mix_six_lanes(0x444,abStack_50,0xf41b8c00u,abStack_38);
  challenge_mix_six_lanes(6,abStack_38,0xf41b8c06u,abStack_3e);
  challenge_mix_six_lanes(0x38e,abStack_3e,0xf41b8c0cu,abStack_44);
  challenge_mix_six_lanes(0x17e,abStack_44,abStack_44,abStack_32);
  uVar6 = (uint32_t)local_30;
  uVar7 = (uint32_t)local_2f;
  uVar8 = (uint32_t)local_2e;
  uVar9 = (uint32_t)local_2d;
  challenge_mix_six_lanes(0x172,abStack_5c,0xf41b8d12u,abStack_38);
  challenge_mix_six_lanes(0x364,abStack_38,0xf41b8d18u,abStack_3e);
  challenge_mix_six_lanes(0x322,abStack_3e,0xf41b8d1eu,abStack_44);
  challenge_mix_six_lanes(0x20e,abStack_44,abStack_44,abStack_32);
  uVar10 = (uint32_t)local_30;
  uVar11 = (uint32_t)local_2f;
  uVar12 = (uint32_t)local_2e;
  uVar13 = (uint32_t)local_2d;
  challenge_mix_six_lanes(0x3b6,auStack_56,0xf41b8e24u,abStack_38);
  challenge_mix_six_lanes(0x2a,abStack_38,0xf41b8e2au,abStack_3e);
  challenge_mix_six_lanes(0x438,abStack_3e,0xf41b8e30u,abStack_44);
  challenge_mix_six_lanes(0xf0,abStack_44,abStack_44,abStack_32);
  uVar14 = (uint32_t)local_2e;
  uVar20 = (uint32_t)local_30;
  uVar19 = (uint32_t)local_2f;
  uVar18 = (uint32_t)local_2d;
  challenge_mix_six_lanes(0x352,auStack_4a,0xf41b8f36u,abStack_38);
  challenge_mix_six_lanes(0x3da,abStack_38,0xf41b8f3cu,abStack_3e);
  challenge_mix_six_lanes(0x1e,abStack_3e,0xf41b8f42u,abStack_44);
  challenge_mix_six_lanes(0x46a,abStack_44,abStack_44,abStack_32);
  *param_3 = ((uint32_t)challenge_lookup_byte(0xf41b8d24u + pack_four_2bit_values(uVar10, uVar13, uVar11, uVar12))) |
             ((uint32_t)challenge_lookup_byte(0xf41b8c12u + pack_four_2bit_values(uVar6, uVar9, uVar7, uVar8))) <<
             0x10 | ((uint32_t)challenge_lookup_byte(0xf41b8e36u + pack_four_2bit_values(uVar20, uVar18, uVar19, uVar14))) << 8 |
             ((uint32_t)challenge_lookup_byte(0xf41b8f48u + pack_four_2bit_values(local_30, local_2d, local_2f, local_2e))) << 0x18;
  challenge_mix_six_lanes(0x300,auStack_6e,0xf41b9048u,abStack_38);
  challenge_mix_six_lanes(0x306,abStack_38,0xf41b904eu,abStack_3e);
  challenge_mix_six_lanes(0xa2,abStack_3e,0xf41b9054u,abStack_44);
  challenge_mix_six_lanes(0x30c,abStack_44,abStack_44,abStack_32);
  uVar6 = (uint32_t)local_30;
  uVar7 = (uint32_t)local_2f;
  uVar8 = (uint32_t)local_2e;
  uVar9 = (uint32_t)local_2d;
  challenge_mix_six_lanes(0x334,abStack_74,0xf41b915au,abStack_38);
  challenge_mix_six_lanes(0x36a,abStack_38,0xf41b9160u,abStack_3e);
  challenge_mix_six_lanes(0x160,abStack_3e,0xf41b9166u,abStack_44);
  challenge_mix_six_lanes(0x296,abStack_44,abStack_44,abStack_32);
  uVar10 = (uint32_t)local_30;
  uVar11 = (uint32_t)local_2f;
  uVar12 = (uint32_t)local_2e;
  uVar13 = (uint32_t)local_2d;
  challenge_mix_six_lanes(0x2ca,auStack_62,0xf41b926cu,abStack_38);
  challenge_mix_six_lanes(0x382,abStack_38,0xf41b9272u,abStack_3e);
  challenge_mix_six_lanes(0xfc,abStack_3e,0xf41b9278u,abStack_44);
  challenge_mix_six_lanes(0x14a,abStack_44,abStack_44,abStack_32);
  uVar14 = (uint32_t)local_2e;
  uVar19 = (uint32_t)local_30;
  uVar18 = (uint32_t)local_2f;
  uVar20 = (uint32_t)local_2d;
  challenge_mix_six_lanes(0xe4,abStack_68,0xf41b937eu,abStack_38);
  challenge_mix_six_lanes(0x328,abStack_38,0xf41b9384u,abStack_3e);
  challenge_mix_six_lanes(0x394,abStack_3e,0xf41b938au,abStack_44);
  challenge_mix_six_lanes(0x8a,abStack_44,abStack_44,abStack_32);
  param_3[1] = (uint32_t)pack_u8_u8(challenge_lookup_byte(0xf41b905au + pack_four_2bit_values(uVar6, uVar9, uVar7, uVar8)),
                              challenge_lookup_byte(0xf41b916cu + pack_four_2bit_values(uVar10, uVar13, uVar11, uVar12))) |
               ((uint32_t)challenge_lookup_byte(0xf41b927eu + pack_four_2bit_values(uVar19, uVar20, uVar18, uVar14)))
               << 0x18 | ((uint32_t)challenge_lookup_byte(0xf41b9390u + pack_four_2bit_values(local_30, local_2d, local_2f, local_2e))) << 0x10;
  challenge_mix_six_lanes(0x33a,abStack_8c,0xf41b9490u,abStack_38);
  challenge_mix_six_lanes(0x3e6,abStack_38,0xf41b9496u,abStack_3e);
  challenge_mix_six_lanes(0x48c,abStack_3e,0xf41b949cu,abStack_44);
  challenge_mix_six_lanes(0x3c8,abStack_44,abStack_44,abStack_32);
  uVar6 = (uint32_t)local_30;
  uVar7 = (uint32_t)local_2f;
  uVar8 = (uint32_t)local_2e;
  uVar9 = (uint32_t)local_2d;
  challenge_mix_six_lanes(0x274,auStack_86,0xf41b95a2u,abStack_38);
  challenge_mix_six_lanes(0x3c,abStack_38,0xf41b95a8u,abStack_3e);
  challenge_mix_six_lanes(0x3a0,abStack_3e,0xf41b95aeu,abStack_44);
  challenge_mix_six_lanes(0x3ec,abStack_44,abStack_44,abStack_32);
  uVar10 = (uint32_t)local_30;
  uVar11 = (uint32_t)local_2f;
  uVar12 = (uint32_t)local_2e;
  uVar13 = (uint32_t)local_2d;
  challenge_mix_six_lanes(0x2fa,abStack_80,0xf41b96b4u,abStack_38);
  challenge_mix_six_lanes(0x498,abStack_38,0xf41b96bau,abStack_3e);
  challenge_mix_six_lanes(0x16c,abStack_3e,0xf41b96c0u,abStack_44);
  challenge_mix_six_lanes(0x27a,abStack_44,abStack_44,abStack_32);
  uVar14 = (uint32_t)local_2e;
  uVar20 = (uint32_t)local_30;
  uVar18 = (uint32_t)local_2f;
  uVar19 = (uint32_t)local_2d;
  challenge_mix_six_lanes(0x39a,auStack_7a,0xf41b97c6u,abStack_38);
  challenge_mix_six_lanes(0x102,abStack_38,0xf41b97ccu,abStack_3e);
  challenge_mix_six_lanes(0x238,abStack_3e,0xf41b97d2u,abStack_44);
  challenge_mix_six_lanes(0x43e,abStack_44,abStack_44,abStack_32);
  param_3[2] = pack_u8_u24(challenge_lookup_byte(0xf41b97d8u + pack_four_2bit_values(local_30, local_2d, local_2f, local_2e)),
                        pack_u8_u16(challenge_lookup_byte(0xf41b96c6u + pack_four_2bit_values(uVar20, uVar19, uVar18, uVar14)),
                                 pack_u8_u8(challenge_lookup_byte(0xf41b95b4u + pack_four_2bit_values(uVar10, uVar13, uVar11, uVar12)),
                                          challenge_lookup_byte(0xf41b94a2u + pack_four_2bit_values(uVar6, uVar9, uVar7, uVar8)))));
  challenge_mix_six_lanes(0x22c,auStack_92,0xf41b98d8u,abStack_38);
  challenge_mix_six_lanes(0x3b0,abStack_38,0xf41b98deu,abStack_3e);
  local_b4 = 0;
  challenge_mix_six_lanes(0,abStack_3e,0xf41b98e4u,abStack_44);
  challenge_mix_six_lanes(0x23e,abStack_44,abStack_44,abStack_32);
  uVar6 = (uint32_t)local_30;
  uVar7 = (uint32_t)local_2f;
  uVar8 = (uint32_t)local_2e;
  uVar9 = (uint32_t)local_2d;
  challenge_mix_six_lanes(0x11a,abStack_98,0xf41b99eau,abStack_38);
  challenge_mix_six_lanes(0x416,abStack_38,0xf41b99f0u,abStack_3e);
  challenge_mix_six_lanes(0x4aa,abStack_3e,0xf41b99f6u,abStack_44);
  challenge_mix_six_lanes(0xc,abStack_44,abStack_44,abStack_32);
  uVar10 = (uint32_t)local_30;
  uVar11 = (uint32_t)local_2f;
  uVar12 = (uint32_t)local_2e;
  uVar13 = (uint32_t)local_2d;
  challenge_mix_six_lanes(0x166,auStack_9e,0xf41b9afcu,abStack_38);
  challenge_mix_six_lanes(0x2dc,abStack_38,0xf41b9b02u,abStack_3e);
  challenge_mix_six_lanes(0x4b0,abStack_3e,0xf41b9b08u,abStack_44);
  challenge_mix_six_lanes(0x120,abStack_44,abStack_44,abStack_32);
  uVar14 = (uint32_t)local_2f;
  uVar18 = (uint32_t)local_2e;
  uVar19 = (uint32_t)local_30;
  uVar20 = (uint32_t)local_2d;
  challenge_mix_six_lanes(0x268,abStack_a4,0xf41b9c0eu,abStack_38);
  challenge_mix_six_lanes(0x12,abStack_38,0xf41b9c14u,abStack_3e);
  challenge_mix_six_lanes(0xba,abStack_3e,0xf41b9c1au,abStack_44);
  challenge_mix_six_lanes(0x244,abStack_44,abStack_44,abStack_32);
  param_3[3] = ((uint32_t)challenge_lookup_byte(0xf41b99fcu + pack_four_2bit_values(uVar10, uVar13, uVar11, uVar12)))
               << 0x10 | ((uint32_t)challenge_lookup_byte(0xf41b98eau + pack_four_2bit_values(uVar6, uVar9, uVar7, uVar8))) << 0x18 |
               ((uint32_t)challenge_lookup_byte(0xf41b9b0eu + pack_four_2bit_values(uVar19, uVar20, uVar14, uVar18)))
               << 8 | ((uint32_t)challenge_lookup_byte(0xf41b9c20u + pack_four_2bit_values(local_30, local_2d, local_2f, local_2e)));
  challenge_expand_schedule_groups(param_1 + 0x42,abStack_a4,auStack_9e,abStack_98,auStack_92,abStack_8c,
               auStack_86,abStack_80,auStack_7a,abStack_74,auStack_6e,abStack_68,
               auStack_62,abStack_5c,auStack_56,abStack_50,auStack_4a);
  challenge_mix_six_lanes(0x2be,abStack_5c,0xf41b9d20u,abStack_38);
  challenge_mix_six_lanes(0xd8,abStack_38,0xf41b9d26u,abStack_3e);
  challenge_mix_six_lanes(0x2e2,abStack_3e,0xf41b9d2cu,abStack_44);
  challenge_mix_six_lanes(0x178,abStack_44,abStack_44,abStack_32);
  uVar6 = (uint32_t)local_30;
  uVar7 = (uint32_t)local_2f;
  uVar8 = (uint32_t)local_2e;
  uVar9 = (uint32_t)local_2d;
  challenge_mix_six_lanes(0x96,auStack_4a,0xf41b9e32u,abStack_38);
  challenge_mix_six_lanes(0x2c4,abStack_38,0xf41b9e38u,abStack_3e);
  challenge_mix_six_lanes(0x312,abStack_3e,0xf41b9e3eu,abStack_44);
  challenge_mix_six_lanes(0x108,abStack_44,abStack_44,abStack_32);
  uVar10 = (uint32_t)local_30;
  uVar11 = (uint32_t)local_2f;
  uVar12 = (uint32_t)local_2e;
  uVar13 = (uint32_t)local_2d;
  challenge_mix_six_lanes(0xde,auStack_56,0xf41b9f44u,abStack_38);
  challenge_mix_six_lanes(0x90,abStack_38,0xf41b9f4au,abStack_3e);
  challenge_mix_six_lanes(0x30,abStack_3e,0xf41b9f50u,abStack_44);
  challenge_mix_six_lanes(0x214,abStack_44,abStack_44,abStack_32);
  uVar14 = (uint32_t)local_2e;
  uVar19 = (uint32_t)local_30;
  uVar20 = (uint32_t)local_2f;
  uVar18 = (uint32_t)local_2d;
  challenge_mix_six_lanes(0xf6,abStack_50,0xf41ba056u,abStack_38);
  challenge_mix_six_lanes(0x232,abStack_38,0xf41ba05cu,abStack_3e);
  challenge_mix_six_lanes(0x280,abStack_3e,0xf41ba062u,abStack_44);
  challenge_mix_six_lanes(0xc0,abStack_44,abStack_44,abStack_32);
  param_3[4] = ((uint32_t)challenge_lookup_byte(0xf41b9d32u + pack_four_2bit_values(uVar6, uVar9, uVar7, uVar8))) |
               ((uint32_t)challenge_lookup_byte(0xf41b9e44u + pack_four_2bit_values(uVar10, uVar13, uVar11, uVar12)))
               << 0x18 | ((uint32_t)challenge_lookup_byte(0xf41b9f56u + pack_four_2bit_values(uVar19, uVar18, uVar20, uVar14))) << 8 |
               ((uint32_t)challenge_lookup_byte(0xf41ba068u + pack_four_2bit_values(local_30, local_2d, local_2f, local_2e))) << 0x10;
  challenge_mix_six_lanes(0x262,auStack_6e,0xf41ba168u,abStack_38);
  challenge_mix_six_lanes(0x1da,abStack_38,0xf41ba16eu,abStack_3e);
  challenge_mix_six_lanes(0x26e,abStack_3e,0xf41ba174u,abStack_44);
  challenge_mix_six_lanes(0x208,abStack_44,abStack_44,abStack_32);
  uVar6 = (uint32_t)local_30;
  uVar7 = (uint32_t)local_2f;
  uVar8 = (uint32_t)local_2e;
  uVar9 = (uint32_t)local_2d;
  challenge_mix_six_lanes(0x42c,abStack_74,0xf41ba27au,abStack_38);
  challenge_mix_six_lanes(0xa8,abStack_38,0xf41ba280u,abStack_3e);
  challenge_mix_six_lanes(0x410,abStack_3e,0xf41ba286u,abStack_44);
  challenge_mix_six_lanes(0xea,abStack_44,abStack_44,abStack_32);
  uVar10 = (uint32_t)local_30;
  uVar11 = (uint32_t)local_2f;
  uVar12 = (uint32_t)local_2e;
  uVar13 = (uint32_t)local_2d;
  challenge_mix_six_lanes(0x220,abStack_68,0xf41ba38cu,abStack_38);
  challenge_mix_six_lanes(0xcc,abStack_38,0xf41ba392u,abStack_3e);
  challenge_mix_six_lanes(0x3d4,abStack_3e,0xf41ba398u,abStack_44);
  challenge_mix_six_lanes(0x346,abStack_44,abStack_44,abStack_32);
  uVar14 = (uint32_t)local_2e;
  uVar19 = (uint32_t)local_30;
  uVar18 = (uint32_t)local_2f;
  uVar20 = (uint32_t)local_2d;
  challenge_mix_six_lanes(0xae,auStack_62,0xf41ba49eu,abStack_38);
  challenge_mix_six_lanes(0x114,abStack_38,0xf41ba4a4u,abStack_3e);
  challenge_mix_six_lanes(0x21a,abStack_3e,0xf41ba4aau,abStack_44);
  challenge_mix_six_lanes(0x3ce,abStack_44,abStack_44,abStack_32);
  param_3[5] = pack_u8_u24(challenge_lookup_byte(0xf41ba4b0u + pack_four_2bit_values(local_30, local_2d, local_2f, local_2e)),
                        pack_u8_u16(challenge_lookup_byte(0xf41ba39eu + pack_four_2bit_values(uVar19, uVar20, uVar18, uVar14)),
                                 pack_u8_u8(challenge_lookup_byte(0xf41ba17au + pack_four_2bit_values(uVar6, uVar9, uVar7, uVar8)),
                                          challenge_lookup_byte(0xf41ba28cu + pack_four_2bit_values(uVar10, uVar13, uVar11, uVar12)))));
  challenge_mix_six_lanes(0x3f8,auStack_7a,0xf41ba5b0u,abStack_38);
  challenge_mix_six_lanes(0x3bc,abStack_38,0xf41ba5b6u,abStack_3e);
  challenge_mix_six_lanes(0xc6,abStack_3e,0xf41ba5bcu,abStack_44);
  challenge_mix_six_lanes(0x1fc,abStack_44,abStack_44,abStack_32);
  uVar6 = (uint32_t)local_30;
  uVar7 = (uint32_t)local_2f;
  uVar8 = (uint32_t)local_2e;
  uVar9 = (uint32_t)local_2d;
  challenge_mix_six_lanes(0x470,abStack_8c,0xf41ba6c2u,abStack_38);
  challenge_mix_six_lanes(0x4a4,abStack_38,0xf41ba6c8u,abStack_3e);
  challenge_mix_six_lanes(0x36,abStack_3e,0xf41ba6ceu,abStack_44);
  challenge_mix_six_lanes(0x34c,abStack_44,abStack_44,abStack_32);
  uVar10 = (uint32_t)local_30;
  uVar11 = (uint32_t)local_2f;
  uVar12 = (uint32_t)local_2e;
  uVar13 = (uint32_t)local_2d;
  challenge_mix_six_lanes(0x32e,auStack_86,0xf41ba7d4u,abStack_38);
  challenge_mix_six_lanes(0x4b6,abStack_38,0xf41ba7dau,abStack_3e);
  challenge_mix_six_lanes(0x476,abStack_3e,0xf41ba7e0u,abStack_44);
  challenge_mix_six_lanes(0x226,abStack_44,abStack_44,abStack_32);
  uVar14 = (uint32_t)local_2e;
  uVar19 = (uint32_t)local_30;
  uVar18 = (uint32_t)local_2f;
  uVar20 = (uint32_t)local_2d;
  challenge_mix_six_lanes(0x2d6,abStack_80,0xf41ba8e6u,abStack_38);
  challenge_mix_six_lanes(0x2d0,abStack_38,0xf41ba8ecu,abStack_3e);
  challenge_mix_six_lanes(0x24,abStack_3e,0xf41ba8f2u,abStack_44);
  challenge_mix_six_lanes(0xd2,abStack_44,abStack_44,abStack_32);
  param_3[6] = ((uint32_t)challenge_lookup_byte(0xf41ba6d4u + pack_four_2bit_values(uVar10, uVar13, uVar11, uVar12)))
               | ((uint32_t)challenge_lookup_byte(0xf41ba5c2u + pack_four_2bit_values(uVar6, uVar9, uVar7, uVar8))) <<
                 0x18 | ((uint32_t)challenge_lookup_byte(0xf41ba7e6u + pack_four_2bit_values(uVar19, uVar20, uVar18, uVar14))) << 8 |
               ((uint32_t)challenge_lookup_byte(0xf41ba8f8u + pack_four_2bit_values(local_30, local_2d, local_2f, local_2e))) << 0x10;
  challenge_mix_six_lanes(0x290,abStack_98,0xf41ba9f8u,abStack_38);
  challenge_mix_six_lanes(0x24a,abStack_38,0xf41ba9feu,abStack_3e);
  challenge_mix_six_lanes(0x45e,abStack_3e,0xf41baa04u,abStack_44);
  challenge_mix_six_lanes(0x35e,abStack_44,abStack_44,abStack_32);
  uVar6 = (uint32_t)local_30;
  uVar7 = (uint32_t)local_2f;
  uVar8 = (uint32_t)local_2e;
  uVar9 = (uint32_t)local_2d;
  challenge_mix_six_lanes(0x3c2,abStack_a4,0xf41bab0au,abStack_38);
  challenge_mix_six_lanes(0x41c,abStack_38,0xf41bab10u,abStack_3e);
  challenge_mix_six_lanes(0x10e,abStack_3e,0xf41bab16u,abStack_44);
  challenge_mix_six_lanes(0x492,abStack_44,abStack_44,abStack_32);
  uVar10 = (uint32_t)local_30;
  uVar11 = (uint32_t)local_2f;
  uVar12 = (uint32_t)local_2e;
  uVar13 = (uint32_t)local_2d;
  challenge_mix_six_lanes(0x464,auStack_9e,0xf41bac1cu,abStack_38);
  challenge_mix_six_lanes(0x42,abStack_38,0xf41bac22u,abStack_3e);
  challenge_mix_six_lanes(0x3f2,abStack_3e,0xf41bac28u,abStack_44);
  challenge_mix_six_lanes(0x432,abStack_44,abStack_44,abStack_32);
  uVar14 = (uint32_t)local_2e;
  uVar19 = (uint32_t)local_30;
  uVar18 = (uint32_t)local_2f;
  uVar20 = (uint32_t)local_2d;
  challenge_mix_six_lanes(0xb4,auStack_92,0xf41bad2eu,abStack_38);
  challenge_mix_six_lanes(0x3e0,abStack_38,0xf41bad34u,abStack_3e);
  challenge_mix_six_lanes(0x340,abStack_3e,0xf41bad3au,abStack_44);
  challenge_mix_six_lanes(0x49e,abStack_44,abStack_44,abStack_32);
  uVar6 = ((uint32_t)challenge_lookup_byte(0xf41bab1cu + pack_four_2bit_values(uVar10, uVar13, uVar11, uVar12))) |
          ((uint32_t)challenge_lookup_byte(0xf41baa0au + pack_four_2bit_values(uVar6, uVar9, uVar7, uVar8))) << 0x10 |
          ((uint32_t)challenge_lookup_byte(0xf41bac2eu + pack_four_2bit_values(uVar19, uVar20, uVar18, uVar14))) << 8
          | ((uint32_t)challenge_lookup_byte(0xf41bad40u + pack_four_2bit_values(local_30, local_2d, local_2f, local_2e))) << 0x18;
  iVar17 = 8;
  param_3[7] = uVar6;
  local_a8_addr = 0xf41bcf14u;
  local_ac_addr = 0xf41bce44u;
  do {
    uVar6 = challenge_table_word_le32(0xf41bce40u + (local_b4 * 4)) ^ (uVar6 >> 0x18 | uVar6 << 8);
    iVar16 = 0;
    uVar6 = challenge_table_word_le32(0xf41bcf10u + (local_b4 * 4)) ^ param_3[iVar17 + -8] ^
            ((uint32_t)(uint8_t)challenge_lookup_byte(0xf41bb140u + (uVar6 & 0xff)) |
             (uint32_t)(uint8_t)challenge_lookup_byte(0xf41baf40u + (uVar6 >> 0x10 & 0xff)) << 0x10 |
             (uint32_t)(uint8_t)challenge_lookup_byte(0xf41bb040u + (uVar6 >> 8 & 0xff)) << 8 |
            (uint32_t)(uint8_t)challenge_lookup_byte(0xf41bae40u + (uVar6 >> 0x18)) << 0x18);
    iVar21 = local_b4 - 0x32;
    local_b4 += 8;
    puVar15 = param_3 + 1 + iVar17;
    param_3[iVar17] = uVar6;
    do {
      uVar6 = challenge_table_word_le32(local_ac_addr + (uint32_t)(-iVar16) * 4u) ^ uVar6;
      uVar6 = challenge_table_word_le32(local_a8_addr + (uint32_t)(-iVar16) * 4u) ^ puVar15[-8] ^
              ((uint32_t)(uint8_t)challenge_lookup_byte(0xf41bb540u + (iVar16 * -0x400 + (uVar6 & 0xff))) |
               (uint32_t)(uint8_t)challenge_lookup_byte(0xf41bb340u + (iVar16 * -0x400 + (uVar6 >> 0x10 & 0xff))) << 0x10 |
               (uint32_t)(uint8_t)challenge_lookup_byte(0xf41bb440u + (iVar16 * -0x400 + (uVar6 >> 8 & 0xff))) << 8 |
              (uint32_t)(uint8_t)challenge_lookup_byte(0xf41bb240u + (iVar16 * -0x400 + (uVar6 >> 0x18))) << 0x18);
      *puVar15 = uVar6;
      if (iVar21 == iVar16) goto LAB_f3fe94f8;
      iVar16 += -1;
      puVar15 = puVar15 + 1;
    } while (iVar16 != -7);
    iVar17 += 8;
    local_ac_addr += 8u * 4u;
    local_a8_addr += 8u * 4u;
  } while (local_b4 < 0x34);
LAB_f3fe94f8:
  uVar7 = param_3[0x3b];
  uVar8 = param_3[0x38];
  uVar9 = param_3[0x39];
  uVar10 = param_3[0x3a];
  uVar6 = 0;
  bVar1 = challenge_lookup_byte(0xf41bd7e0u + (uVar10 >> 0x18));
  param_3[0x38] =
       (uint32_t)(uint8_t)challenge_lookup_byte(0xf41bd2e0u + (uVar8 & 0xff)) |
       (uint32_t)(uint8_t)challenge_lookup_byte(0xf41bd0e0u + (uVar8 >> 0x10 & 0xff)) << 0x10 |
       (uint32_t)(uint8_t)challenge_lookup_byte(0xf41bd1e0u + (uVar8 >> 8 & 0xff)) << 8 |
       (uint32_t)(uint8_t)challenge_lookup_byte(0xf41bcfe0u + (uVar8 >> 0x18)) << 0x18;
  bVar2 = challenge_lookup_byte(0xf41bdce0u + (uVar7 >> 0x10 & 0xff));
  param_3[0x39] =
       (uint32_t)(uint8_t)challenge_lookup_byte(0xf41bd6e0u + (uVar9 & 0xff)) |
       (uint32_t)(uint8_t)challenge_lookup_byte(0xf41bd4e0u + (uVar9 >> 0x10 & 0xff)) << 0x10 |
       (uint32_t)(uint8_t)challenge_lookup_byte(0xf41bd5e0u + (uVar9 >> 8 & 0xff)) << 8 |
       (uint32_t)(uint8_t)challenge_lookup_byte(0xf41bd3e0u + (uVar9 >> 0x18)) << 0x18;
  bVar3 = challenge_lookup_byte(0xf41bdde0u + (uVar7 >> 8 & 0xff));
  bVar4 = challenge_lookup_byte(0xf41bdbe0u + (uVar7 >> 0x18));
  bVar5 = challenge_lookup_byte(0xf41bdee0u + (uVar7 & 0xff));
  param_3[0x3a] =
       (uint32_t)(uint8_t)challenge_lookup_byte(0xf41bdae0u + (uVar10 & 0xff)) |
       (uint32_t)(uint8_t)challenge_lookup_byte(0xf41bd8e0u + (uVar10 >> 0x10 & 0xff)) << 0x10 |
       (uint32_t)(uint8_t)challenge_lookup_byte(0xf41bd9e0u + (uVar10 >> 8 & 0xff)) << 8 | (uint32_t)bVar1 << 0x18;
  param_3[0x3b] = (uint32_t)bVar5 | (uint32_t)bVar2 << 0x10 | (uint32_t)bVar3 << 8 | (uint32_t)bVar4 << 0x18;
  puVar15 = param_3 + 0x3c;
  do {
    iVar16 = 0;
    iVar17 = challenge_lookup_byte(0xf41bdfe0u + (uVar6));
    do {
      ((uint8_t*)puVar15)[iVar16] =
           challenge_lookup_byte(0xf41be020u + (((challenge_table_word_le32(0xf41b4744u + (uint32_t)(iVar16 * 4 + iVar17 * 0x400)) ^ param_3[(uVar6 >> 2) + 0x38])
             >> (~(uVar6 << 3) & 0x18)) & 0xffu) + uVar6 * 0x100u);
      iVar16 += 1;
    } while (iVar16 != 0x100);
    uVar6 += 1;
    puVar15 = puVar15 + 0x40;
  } while (uVar6 != 0x10);
return;
#undef local_30
#undef local_2f
#undef local_2e
#undef local_2d
}


static void challenge_encrypt_with_schedule(const uint8_t *param_1, uint8_t *param_2, const uint32_t *param_3)
{
  uint32_t uVar1=0,uVar2=0,uVar3=0,uVar4,uVar5=0,uVar6=0,uVar7=0,uVar8=0,uVar9=0,uVar10=0,uVar11,uVar12;
  uint32_t local_48, local_44, local_40=0, local_3c=0, local_38=0, local_2c, local_28;
  local_28 = param_3[3] ^ ((uint32_t)challenge_lookup_byte(0xf41b4643u + param_1[0xf])) ^ ((uint32_t)challenge_lookup_byte(0xf41b4443u + param_1[0xd]) << 16) ^ ((uint32_t)challenge_lookup_byte(0xf41b4343u + param_1[0xc]) << 24) ^ ((uint32_t)challenge_lookup_byte(0xf41b4543u + param_1[0xe]) << 8);
  local_2c = (((uint32_t)challenge_lookup_byte(0xf41b3f43u + param_1[8]) << 24) | ((uint32_t)challenge_lookup_byte(0xf41b4143u + param_1[10]) << 8) | ((uint32_t)challenge_lookup_byte(0xf41b4043u + param_1[9]) << 16) | ((uint32_t)challenge_lookup_byte(0xf41b4243u + param_1[0xb]))) ^ param_3[2];
  uVar11 = (((uint32_t)challenge_lookup_byte(0xf41b3e43u + param_1[7])) | ((uint32_t)challenge_lookup_byte(0xf41b3b43u + param_1[4]) << 24)) ^ param_3[1] ^ ((uint32_t)challenge_lookup_byte(0xf41b3d43u + param_1[6]) << 8) ^ ((uint32_t)challenge_lookup_byte(0xf41b3c43u + param_1[5]) << 16);
  uVar4 = 0;
  uVar12 = param_3[0] ^ ((uint32_t)challenge_lookup_byte(0xf41b3943u + param_1[2]) << 8) ^ ((uint32_t)challenge_lookup_byte(0xf41b3743u + param_1[0]) << 24) ^ ((uint32_t)challenge_lookup_byte(0xf41b3a43u + param_1[3])) ^ ((uint32_t)challenge_lookup_byte(0xf41b3843u + param_1[1]) << 16);
  do {
    uVar8 = challenge_table_word_le32(0xf41b7744u + ((local_28 >> 24) * 4u)) ^ challenge_table_word_le32(0xf41b7344u + ((local_2c & 0xffu) * 4u)) ^ param_3[uVar4 * 4u + 7u] ^ challenge_table_word_le32(0xf41b4b44u + (((uVar12 << 8) >> 24) * 4u)) ^ challenge_table_word_le32(0xf41b5f44u + (((uVar11 << 16) >> 24) * 4u));
    uVar1 = param_3[uVar4 * 4u + 6u] ^ challenge_table_word_le32(0xf41b6344u + ((uVar11 & 0xffu) * 4u)) ^ challenge_table_word_le32(0xf41b6744u + ((local_2c >> 24) * 4u)) ^ challenge_table_word_le32(0xf41b4f44u + (((uVar12 << 16) >> 24) * 4u)) ^ challenge_table_word_le32(0xf41b7b44u + (((local_28 << 8) >> 24) * 4u));
    uVar6 = param_3[uVar4 * 4u + 5u] ^ challenge_table_word_le32(0xf41b7f44u + (((local_28 << 16) >> 24) * 4u)) ^ challenge_table_word_le32(0xf41b5344u + ((uVar12 & 0xffu) * 4u)) ^ challenge_table_word_le32(0xf41b5744u + ((uVar11 >> 24) * 4u)) ^ challenge_table_word_le32(0xf41b6b44u + (((local_2c << 8) >> 24) * 4u));
    uVar5 = challenge_table_word_le32(0xf41b6f44u + (((local_2c << 16) >> 24) * 4u)) ^ param_3[uVar4 * 4u + 4u] ^ challenge_table_word_le32(0xf41b4744u + ((uVar12 >> 24) * 4u)) ^ challenge_table_word_le32(0xf41b8344u + ((local_28 & 0xffu) * 4u)) ^ challenge_lookup_byte(0xf41b5b44u + ((uVar11 << 8) >> 24));
    if (uVar4 == 0xc) {
      local_40 = (uVar8 << 8) >> 24; uVar10 = (uVar8 << 16) >> 24; uVar3 = (uVar1 << 8) >> 24; uVar7 = (uVar1 << 16) >> 24; local_3c = (uVar6 << 16) >> 24; uVar9 = (uVar6 << 8) >> 24; local_38 = (uVar5 << 8) >> 24; uVar2 = (uVar5 << 16) >> 24; break;
    }
    local_38 = (uVar5 << 8) >> 24;
    local_3c = (uVar6 << 16) >> 24;
    local_28 = challenge_table_word_le32(0xf41b7744u + ((uVar8 >> 24) * 4u)) ^ challenge_table_word_le32(0xf41b7344u + ((uVar1 & 0xffu) * 4u)) ^ challenge_table_word_le32(0xf41b4b44u + (local_38 * 4u)) ^ param_3[uVar4 * 4u + 0xb] ^ challenge_table_word_le32(0xf41b5f44u + (local_3c * 4u));
    local_40 = (uVar8 << 8) >> 24; uVar2 = (uVar5 << 16) >> 24;
    local_2c = challenge_table_word_le32(0xf41b4f44u + (uVar2 * 4u)) ^ challenge_table_word_le32(0xf41b7b44u + (local_40 * 4u)) ^ challenge_table_word_le32(0xf41b6344u + ((uVar6 & 0xffu) * 4u)) ^ challenge_table_word_le32(0xf41b6744u + ((uVar1 >> 24) * 4u)) ^ param_3[uVar4 * 4u + 10u];
    uVar3 = (uVar1 << 8) >> 24; uVar10 = (uVar8 << 16) >> 24;
    uVar11 = challenge_table_word_le32(0xf41b5744u + ((uVar6 >> 24) * 4u)) ^ param_3[uVar4 * 4u + 9u] ^ challenge_table_word_le32(0xf41b6b44u + (uVar3 * 4u)) ^ challenge_table_word_le32(0xf41b5344u + ((uVar5 & 0xffu) * 4u)) ^ challenge_table_word_le32(0xf41b7f44u + (uVar10 * 4u));
    uVar9 = (uVar6 << 8) >> 24; uVar7 = (uVar1 << 16) >> 24;
    uVar12 = challenge_lookup_byte(0xf41b5b44u + uVar9) ^ challenge_table_word_le32(0xf41b8344u + ((uVar8 & 0xffu) * 4u)) ^ challenge_table_word_le32(0xf41b6f44u + (uVar7 * 4u)) ^ challenge_table_word_le32(0xf41b4744u + ((uVar5 >> 24) * 4u)) ^ param_3[uVar4 * 4u + 8u];
    uVar4 += 2;
  } while (uVar4 < 0xe);
  local_44 = uVar6 & 0xffu; local_48 = uVar1 >> 24;
  const uint8_t *sched = reinterpret_cast<const uint8_t *>(param_3);
  param_2[0] = sched[(uVar5 >> 24) + 0xf0];
  param_2[1] = sched[uVar9 + 0x1f0];
  param_2[2] = sched[uVar7 + 0x2f0];
  param_2[3] = sched[(uVar8 & 0xffu) + 0x3f0];
  param_2[4] = sched[(uVar6 >> 24) + 0x4f0];
  param_2[5] = sched[uVar3 + 0x5f0];
  param_2[6] = sched[uVar10 + 0x6f0];
  param_2[7] = sched[(uVar5 & 0xffu) + 0x7f0];
  param_2[8] = sched[local_48 + 0x8f0];
  param_2[9] = sched[local_40 + 0x9f0];
  param_2[10] = sched[uVar2 + 0xaf0];
  param_2[11] = sched[local_44 + 0xbf0];
  param_2[12] = sched[(uVar8 >> 24) + 0xcf0];
  param_2[13] = sched[local_38 + 0xdf0];
  param_2[14] = sched[local_3c + 0xef0];
  param_2[15] = sched[(uVar1 & 0xffu) + 0xff0];
}

extern "C" int l3_challenge_whitebox_block_cipher_init(l3_challenge_whitebox_block_cipher *ctx, const uint8_t frame84[0x84]) {
    if (!ctx || !frame84) return L3_CHALLENGE_BLOCK_CIPHER_ERR_ARGUMENT;
    std::memset(ctx, 0, sizeof(*ctx));
    std::memcpy(ctx->frame84, frame84, 0x84u);
    challenge_expand_frame_to_schedule(ctx->frame84, ctx->schedule);
    ctx->initialized = 1u;
    return L3_CHALLENGE_BLOCK_CIPHER_OK;
}

extern "C" int l3_challenge_whitebox_encrypt_block(void *user, const uint8_t in[16], uint8_t out[16]) {
    l3_challenge_whitebox_block_cipher *ctx = static_cast<l3_challenge_whitebox_block_cipher *>(user);
    if (!ctx || !in || !out || !ctx->initialized) return L3_CHALLENGE_BLOCK_CIPHER_ERR_ARGUMENT;
    challenge_encrypt_with_schedule(in, out, ctx->schedule);
    return L3_CHALLENGE_BLOCK_CIPHER_OK;
}
