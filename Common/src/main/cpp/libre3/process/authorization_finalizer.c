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
#include "authorization_finalizer.h"

#include <stdlib.h>
#include <string.h>

static void solver_store_le32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

void l3_authorization_finalizer_final_raw40(const l3_authorization_finalizer_final_scalars *in,
                              uint8_t raw40[L3_AUTH_FINAL_STATE_RAW40_LEN]) {
    if (!in || !raw40) return;
    memset(raw40, 0, L3_AUTH_FINAL_STATE_RAW40_LEN);

    /* FUN_f3fd4a60 final successful branch:
       auVar112 = VectorMultiplyAccumulate({local_188[0],local_188[1],local_180.lo,local_180.hi},
                                           {c3dde99b,2d1b3835,1475f09f,2bbff92f},4,0)
       auVar117 = VectorMultiplyAccumulate({local_178,iStack_174,iStack_170,uStack_16c},
                                           {6cc78157,ce880991,393df98b,1a5c7903},4,0)
       param_4 receives auVar112, auVar117, and two scalar affine words. */
    static const uint32_t k_a[4] = {
        0xc3dde99bu, 0x2d1b3835u, 0x1475f09fu, 0x2bbff92fu
    };
    static const uint32_t add_a[4] = {
        0xd1a583c5u, 0xbab55e94u, 0xda4f05fcu, 0xb2b25a22u
    };
    static const uint32_t k_b[4] = {
        0x6cc78157u, 0xce880991u, 0x393df98bu, 0x1a5c7903u
    };
    static const uint32_t add_b[4] = {
        0xef2c9817u, 0x6bafb53au, 0x7cddcf4bu, 0x582378deu
    };

    for (unsigned i = 0; i < 4; ++i) {
        solver_store_le32(
            raw40 + i * 4u, in->a_lanes[i] * k_a[i] + add_a[i]);
    }
    for (unsigned i = 0; i < 4; ++i) {
        solver_store_le32(
            raw40 + 0x10u + i * 4u,
            in->b_lanes[i] * k_b[i] + add_b[i]);
    }
    solver_store_le32(raw40 + 0x20u, in->scalar0 * 0xc3dde99bu + 0xd1a583c5u);
    solver_store_le32(raw40 + 0x24u, in->scalar1 * 0x2d1b3835u + 0xbab55e94u);
}

static uint32_t l3_auth_u32(uint32_t x) { return x; }

static const uint32_t k_final_state_predicate_left_mul[8] = {
    0xe96b1115u, 0x9d1b57ebu, 0x2abd0cf7u, 0xb8a96bc1u,
    0xba888cefu, 0xe0612963u, 0x755de927u, 0x01b7e731u
};

static const uint32_t k_final_state_predicate_right_mul[8] = {
    0xaf48075du, 0x8f8fb667u, 0x4fcbf83du, 0x42abc8ffu,
    0xa63d9d57u, 0xc8836c33u, 0x0b419475u, 0x6acb5ba7u
};

static const uint32_t k_final_state_predicate_add[8] = {
    0x74c1edcfu, 0xad1c1386u, 0xc7a37262u, 0x3b5626a4u,
    0x1dd9f6b2u, 0x6703c7f2u, 0x5b7e3794u, 0x4c00659du
};

static const uint32_t k_final_state_predicate_fold[16] = {
    0xe8f89099u, 0x5840bc79u, 0xc788e859u, 0x36d11439u,
    0xaf4300c4u, 0x1e8b2ca4u, 0x8dd35884u, 0xfd1b8464u,
    0x6c63b044u, 0xd4d59ccfu, 0x441dc8afu, 0xb365f48fu,
    0x22ae206fu, 0x91f64c4fu, 0x013e782fu, 0x79b064bau
};

uint32_t l3_authorization_finalizer_predicate_reduce7(uint32_t value) {
    for (unsigned i = 0; i < 7; ++i) {
        value = l3_auth_u32(k_final_state_predicate_fold[value & 0x0fu] + (value >> 4));
    }
    return value & 0x0fu;
}

int l3_authorization_finalizer_predicate_exact(const uint32_t left[L3_AUTH_FINAL_STATE_PREDICATE_LANES],
                       const uint32_t right[L3_AUTH_FINAL_STATE_PREDICATE_LANES]) {
    if (!left || !right) return 0;
    const uint32_t sentinel = 0xf4170da9u;
    for (int lane = 12; lane >= 0; --lane) {
        const unsigned j = (unsigned)lane & 7u;
        uint32_t v = l3_auth_u32(right[lane] * k_final_state_predicate_right_mul[j] +
                               left[lane]  * k_final_state_predicate_left_mul[j]  +
                               k_final_state_predicate_add[j]);
        if (v != sentinel) {
            return l3_authorization_finalizer_predicate_reduce7(v) != 9u;
        }
    }
    return 0;
}

/* ---- v97: FUN_f3fd4a60 seed-pair loops --------------------------------- */

static uint32_t l3_auth_carry32(uint32_t a, uint32_t b) {
    return (uint32_t)(((uint64_t)a + (uint64_t)b) >> 32);
}

static uint32_t l3_auth_mulhi_u32(uint32_t a, uint32_t b) {
    return (uint32_t)(((uint64_t)a * (uint64_t)b) >> 32);
}

static uint32_t l3_auth_mullo_u32(uint32_t a, uint32_t b) {
    return (uint32_t)((uint64_t)a * (uint64_t)b);
}

typedef struct { uint32_t lo; uint32_t hi; } l3_auth_pair64;

static const uint32_t k_final_state_param2_mul[8] = {
    0x180f2107u, 0x980be129u, 0x8bad4a09u, 0x64c30a11u,
    0x3dcde78bu, 0x356bf11du, 0x3936c62bu, 0xe3b17e03u
};

static const uint32_t k_final_state_param2_add[8] = {
    0xfd5af87cu, 0x19c06d4du, 0xd0e9b61au, 0x957a73b6u,
    0x97fac23cu, 0xbfbe0782u, 0xf106582cu, 0x81357c22u
};

static const uint32_t k_final_state_param1_mul[8] = {
    0x67740285u, 0x531a11e7u, 0x1af066dbu, 0x6ac52e23u,
    0x151b15d7u, 0xda8b7883u, 0x336b4c2bu, 0x1f1b93afu
};

static const uint32_t k_final_state_param1_add[8] = {
    0xc65be6fdu, 0x5a7274bcu, 0x61972239u, 0x4c365495u,
    0x357bcec6u, 0x05a282f6u, 0xcbec840cu, 0x68c6fb48u
};

static const l3_auth_pair64 k_final_state_fold_2eb0[16] = {
    {0xa36e8a6au, 0x64e53a63u}, {0x3368383du, 0x7ddbbc4cu},
    {0xf6a9536du, 0x8a23d1b2u}, {0x86a30140u, 0x931a539bu},
    {0x169caf13u, 0xac10d584u}, {0xa6965ce6u, 0xb507576cu},
    {0x69d77816u, 0xc14f6cd3u}, {0xf9d125e9u, 0xda45eebbu},
    {0x89cad3bcu, 0xe33c70a4u}, {0x4d0beeecu, 0xff84860bu},
    {0xdd059cbfu, 0x087b07f3u}, {0x6cff4a92u, 0x117189dcu},
    {0x304065c2u, 0x2db99f43u}, {0xc03a1395u, 0x36b0212bu},
    {0x5033c168u, 0x4fa6a314u}, {0x1374dc98u, 0x5beeb87bu}
};

static const l3_auth_pair64 k_final_state_fold_2f30[16] = {
    {0x3d626448u, 0xfe8b3dfdu}, {0xc17d52fau, 0xc62a773eu},
    {0x387545d9u, 0x9898f948u}, {0xbc90348bu, 0x60383289u},
    {0x40ab233du, 0x37d76bcbu}, {0xb7a3161cu, 0x0a45edd4u},
    {0x3bbe04ceu, 0xd1e52716u}, {0xbfd8f380u, 0xa9846057u},
    {0x43f3e232u, 0x71239999u}, {0xbaebd511u, 0x43921ba2u},
    {0x3f06c3c3u, 0x1b3154e4u}, {0xc321b275u, 0xe2d08e25u},
    {0x3a19a554u, 0xb53f102fu}, {0xbe349406u, 0x8cde4970u},
    {0x424f82b8u, 0x547d82b2u}, {0xb9477597u, 0x26ec04bbu}
};


static const l3_auth_pair64 k_final_state_fold_2fb0[16] = {
    {0x66784bb3u, 0xe9b5d72du}, {0xc418e2dau, 0x1f87e193u},
    {0x97b86d4au, 0x4cc81260u}, {0x6b57f7bau, 0x7a08432du},
    {0xc8f88ee1u, 0xafda4d93u}, {0x9c981951u, 0xdd1a7e60u},
    {0xfa38b078u, 0x02ec88c6u}, {0xcdd83ae8u, 0x302cb993u},
    {0x2b78d20fu, 0x65fec3fau}, {0xff185c7fu, 0x933ef4c6u},
    {0x5cb8f3a6u, 0xc910ff2du}, {0x30587e16u, 0xf6512ffau},
    {0x03f80886u, 0x239160c7u}, {0x61989fadu, 0x59636b2du},
    {0x35382a1du, 0x86a39bfau}, {0x92d8c144u, 0xbc75a660u}
};

static const l3_auth_pair64 k_final_state_fold_3030[16] = {
    {0xc574fa37u, 0x063789bcu}, {0x25e979bcu, 0x3a76e0e9u},
    {0x84e92dfeu, 0x6deb8787u}, {0xe55dad83u, 0x922adeb3u},
    {0x45d22d08u, 0xc66a35e0u}, {0xa4d1e14au, 0xf9dedc7eu},
    {0x054660cfu, 0x2e1e33abu}, {0x65bae054u, 0x525d8ad7u},
    {0xc4ba9496u, 0x85d23175u}, {0x252f141bu, 0xba1188a2u},
    {0x85a393a0u, 0xee50dfceu}, {0xe6181325u, 0x129036fau},
    {0x4517c767u, 0x4604dd99u}, {0xa58c46ecu, 0x7a4434c5u},
    {0x0600c671u, 0xae838bf2u}, {0x65007ab3u, 0xd1f83290u}
};

static const l3_auth_pair64 k_final_state_fold_30b0[16] = {
    {0x34875457u, 0xd4e8d9b7u}, {0x3e77f535u, 0xe1975e38u},
    {0x48689613u, 0xfe45e2b9u}, {0x525936f1u, 0x0af4673au},
    {0x5c49d7cfu, 0x17a2ebbbu}, {0x663a78adu, 0x2451703cu},
    {0x702b198bu, 0x30fff4bdu}, {0x7a1bba69u, 0x4dae793eu},
    {0x840c5b47u, 0x5a5cfdbfu}, {0x8dfcfc25u, 0x670b8240u},
    {0x97ed9d03u, 0x73ba06c1u}, {0xa1de3de1u, 0x80688b42u},
    {0xabcedebfu, 0x9d170fc3u}, {0x16b571beu, 0xaedd4c34u},
    {0x20a6129cu, 0xbb8bd0b5u}, {0x2a96b37au, 0xc83a5536u}
};

static const l3_auth_pair64 k_final_state_fold_3130[16] = {
    {0x1de6708eu, 0x711c9d3bu}, {0xfd75a20bu, 0x282fdc11u},
    {0xdd04d388u, 0xdf431ae8u}, {0xbc940505u, 0x865659bfu},
    {0x9d8c31bfu, 0x39f8cf80u}, {0x7d1b633cu, 0xe10c0e57u},
    {0x5caa94b9u, 0x981f4d2eu}, {0x3da2c173u, 0x4bc1c2efu},
    {0x1d31f2f0u, 0xf2d501c6u}, {0xfcc1246du, 0xa9e8409cu},
    {0xddb95127u, 0x5d8ab65du}, {0xbd4882a4u, 0x049df534u},
    {0x9cd7b421u, 0xbbb1340bu}, {0x7dcfe0dbu, 0x6f53a9ccu},
    {0x5d5f1258u, 0x1666e8a3u}, {0x3cee43d5u, 0xcd7a277au}
};

static const l3_auth_pair64 k_final_state_fold_31b0[16] = {
    {0x36f1bc27u, 0x039825c4u}, {0x9da5b1eeu, 0xfa44ff8cu},
    {0xbf8b80feu, 0xe34cc7f0u}, {0xe171500eu, 0xdc549054u},
    {0x482545d5u, 0xc3016a1du}, {0x6a0b14e5u, 0xbc093281u},
    {0xd0bf0aacu, 0xa2b60c49u}, {0xf2a4d9bcu, 0x9bbdd4adu},
    {0x5958cf83u, 0x826aae76u}, {0x7b3e9e93u, 0x7b7276dau},
    {0xe1f2945au, 0x621f50a2u}, {0x03d8636au, 0x5b271907u},
    {0x25be327au, 0x442ee16bu}, {0x8c722841u, 0x3adbbb33u},
    {0xae57f751u, 0x23e38397u}, {0x150bed18u, 0x1a905d60u}
};


static const l3_auth_pair64 k_final_state_fold_3230[16] = {
    {0x1de65a4au, 0x25d24684u}, {0xab2d5aa8u, 0x9324910au},
    {0xcb845a5fu, 0x0c565713u}, {0xebdb5a16u, 0x75881d1cu},
    {0x79225a74u, 0xe2da67a3u}, {0x99795a2bu, 0x5c0c2dacu},
    {0x26c05a89u, 0xc95e7833u}, {0x47175a40u, 0x32903e3cu},
    {0xd45e5a9eu, 0xafe288c2u}, {0xf4b55a55u, 0x19144ecbu},
    {0x150c5a0cu, 0x824614d5u}, {0xa2535a6au, 0xff985f5bu},
    {0xc2aa5a21u, 0x68ca2564u}, {0x4ff15a7fu, 0xd61c6febu},
    {0x70485a36u, 0x4f4e35f4u}, {0xfd8f5a94u, 0xbca0807au}
};


static const l3_auth_pair64 k_final_state_fold_32b0[16] = {
    {0xdf26f873u, 0x028325dcu}, {0xe8cce11au, 0x1c7c857fu},
    {0xf272c9c1u, 0x2675e522u}, {0xfc18b268u, 0x306f44c5u},
    {0x05be9b0fu, 0x4a68a469u}, {0x0f6483b6u, 0x5462040cu},
    {0x190a6c5du, 0x6e5b63afu}, {0x22b05504u, 0x7854c352u},
    {0x2c563dabu, 0x824e22f5u}, {0x35fc2652u, 0x9c478298u},
    {0x3fa20ef9u, 0xa640e23bu}, {0x4947f7a0u, 0xb03a41deu},
    {0x52ede047u, 0xca33a181u}, {0xc2353e7fu, 0xd49706f3u},
    {0xcbdb2726u, 0xee906696u}, {0x0d5810fcdu, 0xf889c639u}
};

static const l3_auth_pair64 k_final_state_fold_3330[16] = {
    {0x8cd4e3d0u, 0x720b2ec2u}, {0x58730f99u, 0x89a87cfau},
    {0xc05e8e29u, 0x9fde3cfbu}, {0x8bfcb9f2u, 0xa77b8b33u},
    {0xf3e83882u, 0xbdb14b34u}, {0xbf86644bu, 0xc54e996cu},
    {0x8b249014u, 0xdcebe7a4u}, {0xf3100ea4u, 0xe321a7a5u},
    {0xbeae3a6du, 0xfabef5ddu}, {0x2699b8fdu, 0x00f4b5dfu},
    {0xf237e4c6u, 0x18920416u}, {0x5a236356u, 0x2ec7c418u},
    {0x25c18f1fu, 0x36651250u}, {0xf15fbae8u, 0x4e026087u},
    {0x594b3978u, 0x54382089u}, {0x24e96541u, 0x6bd56ec1u}
};

static const l3_auth_pair64 k_final_state_fold_33b0[16] = {
    {0x38513622u, 0xd6577530u}, {0x972ce226u, 0x045d43efu},
    {0xf6088e2au, 0x326312aeu}, {0x54e43a2eu, 0x6068e16eu},
    {0xb3bfe632u, 0x9e6eb02du}, {0x129b9236u, 0xcc747eedu},
    {0x71773e3au, 0xfa7a4dacu}, {0xe29829ffu, 0x28233075u},
    {0x4173d603u, 0x5628ff35u}, {0xa04f8207u, 0x842ecdf4u},
    {0xff2b2e0bu, 0xb2349cb3u}, {0x5e06da0fu, 0xe03a6b73u},
    {0xbce28613u, 0x1e403a32u}, {0x1bbe3217u, 0x4c4608f2u},
    {0x7a99de1bu, 0x7a4bd7b1u}, {0xd9758a1fu, 0xa851a670u}
};


static const l3_auth_pair64 k_final_state_fold_3430[16] = {
    {0xbdf4f8deu, 0xd45617eau}, {0x09b6c469u, 0xc413a413u},
    {0x55788ff4u, 0xb3d1303bu}, {0xa13a5b7fu, 0xa38ebc63u},
    {0x58f29425u, 0x9f595d0au}, {0xa4b45fb0u, 0x8f16e932u},
    {0xf0762b3bu, 0x7ed4755au}, {0x3c37f6c6u, 0x6e920183u},
    {0x87f9c251u, 0x5e4f8dabu}, {0xd3bb8ddcu, 0x4e0d19d3u},
    {0x8b73c682u, 0x39d7ba7au}, {0xd735920du, 0x299546a2u},
    {0x22f75d98u, 0x1952d2cbu}, {0x6eb92923u, 0x09105ef3u},
    {0xba7af4aeu, 0xf8cdeb1bu}, {0x72332d54u, 0xe4988bc2u}
};



static const l3_auth_pair64 k_final_state_fold_34b0[16] = {
    {0xd2b2344cu, 0x3e315555u}, {0x57393b03u, 0xc1f45028u},
    {0xd3b3c403u, 0x52958d12u}, {0x502e4d03u, 0xe336c9fdu},
    {0xd4b553bau, 0x76f9c4cfu}, {0x512fdcbau, 0x079b01bau},
    {0xd5b6e371u, 0x9b5dfc8cu}, {0x52316c71u, 0x2bff3977u},
    {0xd6b87328u, 0xbfc23449u}, {0x5332fc28u, 0x40637134u},
    {0xcfad8528u, 0xd104ae1eu}, {0x54348bdfu, 0x64c7a8f1u},
    {0xd0af14dfu, 0xf568e5dbu}, {0x55361b96u, 0x892be0aeu},
    {0xd1b0a496u, 0x19cd1d98u}, {0x5637ab4du, 0xad90186bu}
};

static const l3_auth_pair64 k_final_state_fold_3530[16] = {
    {0x2fbfb592u, 0x5bf9b629u}, {0xb48e1606u, 0x077223c2u},
    {0x4d42e773u, 0xbe86404bu}, {0xd21147e7u, 0x69feade4u},
    {0x6ac61954u, 0x1112ca6du}, {0xef9479c8u, 0xcc8b3806u},
    {0x88494b35u, 0x739f548fu}, {0x20fe1ca2u, 0x2ab37118u},
    {0xa5cc7d16u, 0xd62bdeb1u}, {0x3e814e83u, 0x8d3ffb3au},
    {0xc34faef7u, 0x38b868d3u}, {0x5c048064u, 0xefcc855cu},
    {0xe0d2e0d8u, 0x9b44f2f5u}, {0x7987b245u, 0x42590f7eu},
    {0xfe5612b9u, 0xfdd17d17u}, {0x970ae426u, 0xa4e599a0u}
};

static const l3_auth_pair64 k_final_state_fold_35b0[16] = {
    {0xac311fe0u, 0xb4f8ba54u}, {0x7ab4aa16u, 0x404109e6u},
    {0x84c02225u, 0xd941af91u}, {0x5343ac5bu, 0x6489ff23u},
    {0x5d4f246au, 0xfd8aa4ceu}, {0x675a9c79u, 0x868b4a79u},
    {0x35de26afu, 0x11d39a0bu}, {0x3fe99ebeu, 0xaad43fb6u},
    {0x0e6d28f4u, 0x361c8f48u}, {0x1878a103u, 0xcf1d34f3u},
    {0xe6fc2b39u, 0x5a658484u}, {0xf107a348u, 0xe3662a2fu},
    {0xfb131b57u, 0x7c66cfdau}, {0xc996a58du, 0x07af1f6cu},
    {0xd3a21d9cu, 0x90afc517u}, {0xa225a7d2u, 0x2bf814a9u}
};

static const l3_auth_pair64 k_final_state_fold_3630[16] = {
    {0xcc444d60u, 0x55e77b80u}, {0x64e6067bu, 0x6e05581du},
    {0x7a2939b1u, 0x702e9c21u}, {0x12caf2ccu, 0x884c78beu},
    {0xab6cabe7u, 0x906a555au}, {0x440e6502u, 0xa88831f7u},
    {0xdcb01e1du, 0xb0a60e93u}, {0x7551d738u, 0xc8c3eb30u},
    {0x8a950a6eu, 0xdaed2f34u}, {0x2336c389u, 0xe30b0bd1u},
    {0xbbd87ca4u, 0xfb28e86du}, {0x547a35bfu, 0x0346c50au},
    {0xed1beedau, 0x1b64a1a6u}, {0x025f2210u, 0x2d8de5abu},
    {0x9b00db2bu, 0x35abc247u}, {0x33a29446u, 0x4dc99ee4u}
};

static const l3_auth_pair64 k_final_state_fold_36b0[16] = {
    {0x1757740fu, 0x04e214b9u}, {0x63897dd8u, 0x31f432f5u},
    {0x0bd459cau, 0x67c9a9aau}, {0x58066393u, 0x94dbc7e6u},
    {0x00513f85u, 0xcab13e9bu}, {0x4c83494eu, 0xf7c35cd7u},
    {0xf4ce2540u, 0x2d98d38bu}, {0x9d190132u, 0x536e4a40u},
    {0xe94b0afbu, 0x8080687cu}, {0x9195e6edu, 0xb655df31u},
    {0xddc7f0b6u, 0xe367fd6du}, {0x8612cca8u, 0x193d7422u},
    {0xd244d671u, 0x464f925eu}, {0x7a8fb263u, 0x7c250913u},
    {0xc6c1bc2cu, 0xa937274fu}, {0x6f0c981eu, 0xdf0c9e04u}
};

static const l3_auth_pair64 k_final_state_fold_3730[16] = {
    {0x6d842149u, 0x5633e2dfu}, {0x6a244967u, 0x85aefad4u},
    {0x49e8a81cu, 0xbf7e6e50u}, {0x4688d03au, 0xeef98645u},
    {0x264d2eefu, 0x18c8f9c1u}, {0x22ed570du, 0x484411b6u},
    {0x02b1b5c2u, 0x72138532u}, {0xff51dde0u, 0xa18e9d26u},
    {0xfbf205feu, 0xd109b51bu}, {0xdbb664b3u, 0x0ad92897u},
    {0xd8568cd1u, 0x3a54408cu}, {0xb81aeb86u, 0x6423b408u},
    {0xb4bb13a4u, 0x939ecbfdu}, {0x947f7259u, 0xcd6e3f79u},
    {0x911f9a77u, 0xfce9576eu}, {0x8dbfc295u, 0x2c646f63u}
};

static const l3_auth_pair64 k_final_state_fold_37b0[16] = {
    {0x36be9dc3u, 0x33024957u}, {0x6f6d5aeeu, 0xadb3fd50u},
    {0xd1901d22u, 0x18f0e31cu}, {0x0a3eda4du, 0x83a29716u},
    {0x6c619c81u, 0xfedf7ce2u}, {0xa51059acu, 0x699130dbu},
    {0x07331be0u, 0xd4ce16a8u}, {0x6955de14u, 0x400afc74u},
    {0xa2049b3fu, 0xbabcb06du}, {0x04275d73u, 0x25f9963au},
    {0x3cd61a9eu, 0x90ab4a33u}, {0x9ef8dcd2u, 0x0be82fffu},
    {0xd7a799fdu, 0x7699e3f8u}, {0x39ca5c31u, 0xe1d6c9c5u},
    {0x7279195cu, 0x5c887dbeu}, {0xd49bdb90u, 0xc7c5638au}
};

static const l3_auth_pair64 *l3_auth_fold_table(l3_auth_fold64_table_id table_id) {
    switch (table_id) {
    case L3_AUTH_FOLD64_TABLE_2F30: return k_final_state_fold_2f30;
    case L3_AUTH_FOLD64_TABLE_2FB0: return k_final_state_fold_2fb0;
    case L3_AUTH_FOLD64_TABLE_3030: return k_final_state_fold_3030;
    case L3_AUTH_FOLD64_TABLE_30B0: return k_final_state_fold_30b0;
    case L3_AUTH_FOLD64_TABLE_3130: return k_final_state_fold_3130;
    case L3_AUTH_FOLD64_TABLE_31B0: return k_final_state_fold_31b0;
    case L3_AUTH_FOLD64_TABLE_3230: return k_final_state_fold_3230;
    case L3_AUTH_FOLD64_TABLE_32B0: return k_final_state_fold_32b0;
    case L3_AUTH_FOLD64_TABLE_3330: return k_final_state_fold_3330;
    case L3_AUTH_FOLD64_TABLE_33B0: return k_final_state_fold_33b0;
    case L3_AUTH_FOLD64_TABLE_3430: return k_final_state_fold_3430;
    case L3_AUTH_FOLD64_TABLE_34B0: return k_final_state_fold_34b0;
    case L3_AUTH_FOLD64_TABLE_3530: return k_final_state_fold_3530;
    case L3_AUTH_FOLD64_TABLE_35B0: return k_final_state_fold_35b0;
    case L3_AUTH_FOLD64_TABLE_3630: return k_final_state_fold_3630;
    case L3_AUTH_FOLD64_TABLE_36B0: return k_final_state_fold_36b0;
    case L3_AUTH_FOLD64_TABLE_3730: return k_final_state_fold_3730;
    case L3_AUTH_FOLD64_TABLE_37B0: return k_final_state_fold_37b0;
    case L3_AUTH_FOLD64_TABLE_2EB0:
    default: return k_final_state_fold_2eb0;
    }
}

uint64_t l3_authorization_finalizer_fold64(l3_auth_fold64_table_id table_id,
                             uint64_t value,
                             unsigned rounds) {
    const l3_auth_pair64 *tab = l3_auth_fold_table(table_id);
    uint32_t lo = (uint32_t)value;
    uint32_t hi = (uint32_t)(value >> 32);
    for (unsigned r = 0; r < rounds; ++r) {
        const unsigned idx = lo & 0x0fu;
        const uint32_t shifted_lo = (lo >> 4) | (hi << 28);
        const uint32_t shifted_hi = hi >> 4;
        const uint32_t new_lo = shifted_lo + tab[idx].lo;
        const uint32_t new_hi = shifted_hi + tab[idx].hi + l3_auth_carry32(shifted_lo, tab[idx].lo);
        lo = new_lo;
        hi = new_hi;
    }
    return ((uint64_t)hi << 32) | lo;
}

void l3_authorization_finalizer_seed_param2_pairs(
    const uint32_t param2_words[L3_AUTH_FINAL_STATE_SEED_PAIR_COUNT],
    uint32_t out_words[L3_AUTH_FINAL_STATE_SEED_WORD_COUNT]) {
    if (!param2_words || !out_words) return;
    for (unsigned i = 0; i < L3_AUTH_FINAL_STATE_SEED_PAIR_COUNT; ++i) {
        const unsigned j = i & 7u;
        const uint32_t mixed = (uint32_t)((param2_words[i] * k_final_state_param2_mul[j] +
                                          k_final_state_param2_add[j]) *
                                         0x5f521c3du + 0xd82b3440u);

        uint32_t lo0 = l3_auth_mullo_u32(mixed, 0x3111c6adu);
        uint32_t hi0 = (uint32_t)(mixed * 0xac909a2cu +
                                  l3_auth_mulhi_u32(mixed, 0x3111c6adu) +
                                  0x78cc5a85u +
                                  (uint32_t)(0xca81d2a1u < lo0));
        lo0 += 0x357e2d5eu;
        const uint32_t folded_lo = (uint32_t)l3_authorization_finalizer_fold64(
            L3_AUTH_FOLD64_TABLE_2EB0, ((uint64_t)hi0 << 32) | lo0, 8u);

        const uint32_t lo_mul = l3_auth_mullo_u32(mixed, 0x07c703b5u);
        out_words[i * 2u + 0u] = lo_mul + 0xa655d1bbu;
        out_words[i * 2u + 1u] = (uint32_t)(mixed * 0x1c36f147u +
                                            l3_auth_mulhi_u32(mixed, 0x07c703b5u) +
                                            folded_lo * 0x3b08c5d7u +
                                            0x1209c8e5u +
                                            (uint32_t)(0x59aa2e44u < lo_mul));
    }
}

void l3_authorization_finalizer_seed_param1_pairs(
    const uint32_t parent_words[L3_AUTH_FINAL_STATE_SEED_PAIR_COUNT],
    uint32_t out_words[L3_AUTH_FINAL_STATE_SEED_WORD_COUNT]) {
    if (!parent_words || !out_words) return;
    for (unsigned i = 0; i < L3_AUTH_FINAL_STATE_SEED_PAIR_COUNT; ++i) {
        const unsigned j = i & 7u;
        const uint32_t mixed = (uint32_t)((parent_words[i] * k_final_state_param1_mul[j] +
                                          k_final_state_param1_add[j]) *
                                         0x43082e1bu + 0x59022f45u);

        uint32_t lo0 = l3_auth_mullo_u32(mixed, 0x95921ccdu);
        uint32_t hi0 = (uint32_t)(mixed * 0x28e783ddu +
                                  l3_auth_mulhi_u32(mixed, 0x95921ccdu) +
                                  0xfe79e548u +
                                  (uint32_t)(0x1d2ffb5eu < lo0));
        lo0 += 0xe2d004a1u;
        const uint32_t folded_lo = (uint32_t)l3_authorization_finalizer_fold64(
            L3_AUTH_FOLD64_TABLE_2F30, ((uint64_t)hi0 << 32) | lo0, 8u);

        const uint32_t lo_mul = l3_auth_mullo_u32(mixed, 0xfa20e5c1u);
        out_words[i * 2u + 0u] = lo_mul + 0x5136ac88u;
        out_words[i * 2u + 1u] = (uint32_t)(mixed * 0x5c14b26fu +
                                            l3_auth_mulhi_u32(mixed, 0xfa20e5c1u) +
                                            folded_lo * 0x254c533bu +
                                            0x4d0c96bau +
                                            (uint32_t)(0xaec95377u < lo_mul));
    }
}

/* ---- v99: FUN_f3fd4a60 first post-seed loop scalar head ------------- */

void l3_authorization_finalizer_first_loop_coeffs_from_head(
    uint32_t parent_lo, uint32_t parent_hi,
    uint32_t current_lo, uint32_t current_hi,
    l3_authorization_finalizer_first_loop_coeffs *out) {
    if (!out) return;

    /* Native names from the first do/while after the two seed loops:
       parent_lo/parent_hi = uVar51/iVar47 from param_1[0x1c7]+8/+0xc
       current_lo/current_hi = uVar61/iVar50 from local_328 pair[i]. */
    const uint32_t p61_lo = l3_auth_mullo_u32(parent_lo, 0x061f4326u);
    const uint32_t p61_hi = l3_auth_mulhi_u32(parent_lo, 0x061f4326u);
    const uint32_t pd8_lo = l3_auth_mullo_u32(parent_lo, 0xd89d6e19u);
    const uint32_t pd8_hi = l3_auth_mulhi_u32(parent_lo, 0xd89d6e19u);

    const uint32_t base_lo = p61_lo + 0x6e0ef3e4u; /* uVar57 */
    const uint32_t slope_lo = pd8_lo + 0x92096cc2u; /* uVar58 */

    const uint64_t prod0 = (uint64_t)slope_lo * (uint64_t)current_lo;
    const uint32_t prod0_lo = (uint32_t)prod0;
    const uint32_t x_lo = base_lo + prod0_lo; /* uVar76 */

    const uint32_t inner_hi = (uint32_t)(
        parent_hi * 0xd89d6e19u +
        parent_lo * 0xe226a48fu +
        pd8_hi +
        0xdae96a73u +
        (uint32_t)(0x6df6933du < pd8_lo));

    const uint32_t x_hi = (uint32_t)(
        parent_hi * 0x061f4326u +
        parent_lo * 0xd72e73fcu +
        p61_hi +
        0x190a1f5du +
        (uint32_t)(0x91f10c1bu < p61_lo) +
        current_hi * slope_lo +
        current_lo * inner_hi +
        (uint32_t)(prod0 >> 32) +
        l3_auth_carry32(base_lo, prod0_lo)); /* iVar77 */

    const uint32_t m0_lo = l3_auth_mullo_u32(x_lo, 0xb4e9eedbu);
    const uint32_t m0_hi = l3_auth_mulhi_u32(x_lo, 0xb4e9eedbu);
    const uint32_t f0_lo = m0_lo + 0x98da32abu;
    const uint32_t f0_hi = (uint32_t)(
        x_hi * 0xb4e9eedbu +
        x_lo * 0xd5667c5cu +
        m0_hi +
        0xf95b5847u +
        (uint32_t)(0x6725cd54u < m0_lo));

    const uint64_t folded = l3_authorization_finalizer_fold64(
        L3_AUTH_FOLD64_TABLE_2FB0, ((uint64_t)f0_hi << 32) | f0_lo, 7u);
    const uint32_t fold_lo = (uint32_t)folded;
    const uint32_t fold_hi = (uint32_t)(folded >> 32);

    const uint64_t fold_scaled = (uint64_t)fold_lo * 0xb0000000u;
    const uint32_t fold_scaled_lo = (uint32_t)fold_scaled;
    const uint32_t x2_lo_mul = l3_auth_mullo_u32(x_lo, 0x6aa19a97u);
    const uint32_t x2_hi_mul = l3_auth_mulhi_u32(x_lo, 0x6aa19a97u);
    const uint32_t sum1 = x2_lo_mul + fold_scaled_lo;
    const uint32_t y_lo = sum1 + 0xeaa4119bu; /* uVar71 */
    const uint32_t y_mul_lo = l3_auth_mullo_u32(y_lo, 0x85c973f7u);
    const uint32_t y_mul_hi = l3_auth_mulhi_u32(y_lo, 0x85c973f7u);

    const uint32_t y_hi = (uint32_t)(
        x_hi * 0x6aa19a97u +
        x_lo * 0xda4a21d9u +
        x2_hi_mul +
        fold_lo * 0xbfca8060u +
        (uint32_t)(fold_scaled >> 32) +
        fold_hi * 0xb0000000u +
        l3_auth_carry32(x2_lo_mul, fold_scaled_lo) +
        0xc551b0cau +
        (uint32_t)(0x155bee64u < sum1)); /* iVar49 */

    out->mul_lo = y_mul_lo + 0xa46a3abbu; /* uVar65 */
    out->mul_hi = (uint32_t)(
        y_hi * 0x85c973f7u +
        y_lo * 0x4af7fe38u +
        y_mul_hi +
        0x31d0e21au +
        (uint32_t)(0x5b95c544u < y_mul_lo)); /* iVar78 */

    const uint32_t add_mul_lo = l3_auth_mullo_u32(y_lo, 0xf71054acu);
    const uint32_t add_mul_hi = l3_auth_mulhi_u32(y_lo, 0xf71054acu);
    out->add_lo = add_mul_lo + 0x3a4f0a7cu; /* uVar66 */
    out->add_hi = (uint32_t)(
        y_hi * 0xf71054acu +
        y_lo * 0xc42eba8au +
        add_mul_hi +
        0xcee5f2e6u +
        (uint32_t)(0xc5b0f583u < add_mul_lo)); /* iVar80 */
}

void l3_authorization_finalizer_first_loop_update_head_pair(
    const uint32_t current_pair[2],
    const uint32_t param1_pair0[2],
    const l3_authorization_finalizer_first_loop_coeffs *coeffs,
    uint32_t out_pair[2]) {
    if (!current_pair || !param1_pair0 || !coeffs || !out_pair) return;

    /* Exact write to *(uint32_t *)(&local_328 + local_3c8) and its paired
       high word in the first generated loop. */
    const uint64_t prod = (uint64_t)param1_pair0[0] * (uint64_t)coeffs->mul_lo;
    const uint32_t prod_lo = (uint32_t)prod;
    const uint32_t tmp = coeffs->add_lo + current_pair[0];
    out_pair[0] = tmp + prod_lo;
    out_pair[1] = (uint32_t)(
        coeffs->add_hi +
        current_pair[1] +
        l3_auth_carry32(coeffs->add_lo, current_pair[0]) +
        coeffs->mul_hi * param1_pair0[0] +
        coeffs->mul_lo * param1_pair0[1] +
        (uint32_t)(prod >> 32) +
        l3_auth_carry32(tmp, prod_lo));
}


/* ---- v100: first post-seed loop 3030/30b0 tail ----------------------- */

void l3_authorization_finalizer_first_loop_tail_from_updated_head(
    const uint32_t updated_head_pair[2],
    const uint32_t window_pair_after_vector_add[2],
    l3_authorization_finalizer_first_loop_tail_state *out) {
    if (!updated_head_pair || !window_pair_after_vector_add || !out) return;
    memset(out, 0, sizeof(*out));

    const uint32_t head_lo = updated_head_pair[0];   /* native uVar71 */
    const uint32_t head_hi = updated_head_pair[1];   /* native iVar50 */

    /* Native block immediately after writing local_328:
       x = head * 0xd69cca07 + {lo:0xa73f78c2, hi:-0x4ffc5ffa plus affine high terms}
       fold x seven times through DAT_f41a3030, then mix by 0x207cb425. */
    const uint32_t m3030_lo = l3_auth_mullo_u32(head_lo, 0xd69cca07u);
    const uint32_t m3030_hi = l3_auth_mulhi_u32(head_lo, 0xd69cca07u);
    const uint32_t x3030_lo = m3030_lo + 0xa73f78c2u;
    const uint32_t x3030_hi = (uint32_t)(
        head_hi * 0xd69cca07u +
        head_lo * 0x355f41cbu +
        m3030_hi +
        0xb003a006u +
        l3_auth_carry32(m3030_lo, 0xa73f78c2u));

    const uint64_t folded3030 = l3_authorization_finalizer_fold64(
        L3_AUTH_FOLD64_TABLE_3030, ((uint64_t)x3030_hi << 32) | x3030_lo, 7u);
    const uint32_t f3030_lo = (uint32_t)folded3030;
    const uint32_t f3030_hi = (uint32_t)(folded3030 >> 32);

    const uint32_t mix3030_lo_mul = l3_auth_mullo_u32(f3030_lo, 0x207cb425u);
    const uint32_t mix3030_hi_mul = l3_auth_mulhi_u32(f3030_lo, 0x207cb425u);
    const uint32_t mid_lo = mix3030_lo_mul + 0x945ab955u; /* native uVar73 */
    const uint32_t mid_hi = (uint32_t)(
        f3030_hi * 0x207cb425u +
        f3030_lo * 0x249c71d0u +
        mix3030_hi_mul +
        0xb0f9d5e5u +
        l3_auth_carry32(mix3030_lo_mul, 0x945ab955u)); /* native iVar49 */

    out->fold3030_lo = mid_lo;
    out->fold3030_hi = mid_hi;

    /* Native second fold chain:
       y = mid * 0xb17eadd1 + {lo:0xbcd4e58a, hi:-0x0b0e0dba plus affine high terms}
       fold seven times through DAT_f41a30b0, then take one low-only extra step. */
    const uint32_t m30b0_lo = l3_auth_mullo_u32(mid_lo, 0xb17eadd1u);
    const uint32_t m30b0_hi = l3_auth_mulhi_u32(mid_lo, 0xb17eadd1u);
    const uint32_t x30b0_lo = m30b0_lo + 0xbcd4e58au;
    const uint32_t x30b0_hi = (uint32_t)(
        mid_hi * 0xb17eadd1u +
        mid_lo * 0xbbaf071bu +
        m30b0_hi +
        0xf4f1f246u +
        l3_auth_carry32(m30b0_lo, 0xbcd4e58au));

    const uint64_t folded30b0 = l3_authorization_finalizer_fold64(
        L3_AUTH_FOLD64_TABLE_30B0, ((uint64_t)x30b0_hi << 32) | x30b0_lo, 7u);
    const uint32_t f30b0_lo = (uint32_t)folded30b0;
    const uint32_t f30b0_hi = (uint32_t)(folded30b0 >> 32);
    const l3_auth_pair64 *tab30b0 = l3_auth_fold_table(L3_AUTH_FOLD64_TABLE_30B0);
    const uint32_t tail_shift_lo = (f30b0_lo >> 4) | (f30b0_hi << 28);
    const uint32_t tail_lo = tail_shift_lo + tab30b0[f30b0_lo & 0x0fu].lo;
    const uint32_t tail_mix = (tail_lo >> 4) + tab30b0[tail_lo & 0x0fu].lo;
    out->fold30b0_tail_lo = tail_lo;
    out->fold30b0_tail_mix = tail_mix;

    /* Final scalar write to uStack_320 at this loop offset.  The input
       window_pair_after_vector_add is the native auVar138._0_4_/._4_4_ value
       produced by the vector-add side of the same loop just before this tail. */
    const uint32_t p61_lo = l3_auth_mullo_u32(mid_lo, 0x61c29663u);
    const uint32_t p61_hi = l3_auth_mulhi_u32(mid_lo, 0x61c29663u);
    const uint32_t l4_lo = l3_auth_mullo_u32(p61_lo, 0x155fdc19u);
    const uint32_t l4_hi = l3_auth_mulhi_u32(p61_lo, 0x155fdc19u);
    const uint32_t sum0 = l4_lo + window_pair_after_vector_add[0];
    out->out_lo = sum0 + 0xb61efdffu;
    out->out_hi = (uint32_t)(
        (mid_hi * 0x61c29663u +
         mid_lo * 0x2e3dc5f8u +
         p61_hi +
         tail_mix * 0xa7cb60d0u) * 0x155fdc19u +
        p61_lo * 0x743cce7cu +
        l4_hi +
        window_pair_after_vector_add[1] +
        l3_auth_carry32(l4_lo, window_pair_after_vector_add[0]) +
        0xbb95505au +
        l3_auth_carry32(sum0, 0xb61efdffu));
}


/* ---- v101: first-loop vadd.i64 vector/window helper ------------------ */

static l3_auth_pair32 l3_auth_pair_add64(l3_auth_pair32 a, l3_auth_pair32 b) {
    l3_auth_pair32 r;
    r.lo = a.lo + b.lo;
    r.hi = a.hi + b.hi + l3_auth_carry32(a.lo, b.lo);
    return r;
}

static l3_auth_pair32 l3_auth_pair_mul_affine64(l3_auth_pair32 x, l3_auth_pair32 k) {
    const uint64_t p = (uint64_t)x.lo * (uint64_t)k.lo;
    l3_auth_pair32 r;
    r.lo = (uint32_t)p;
    r.hi = (uint32_t)(x.hi * k.lo + x.lo * k.hi + (uint32_t)(p >> 32));
    return r;
}

void l3_authorization_finalizer_vadd_i64_product_window2(
    const l3_auth_qpair64 *window_in,
    const l3_auth_qpair64 *source_pairs,
    const l3_authorization_finalizer_first_loop_coeffs *coeffs,
    l3_auth_qpair64 *window_out) {
    if (!window_in || !source_pairs || !coeffs || !window_out) return;

    const l3_auth_pair32 base = { coeffs->add_lo, coeffs->add_hi };
    const l3_auth_pair32 mul = { coeffs->mul_lo, coeffs->mul_hi };
    for (unsigned lane = 0; lane < 2; ++lane) {
        const l3_auth_pair32 with_base = l3_auth_pair_add64(window_in->lane[lane], base);
        const l3_auth_pair32 prod = l3_auth_pair_mul_affine64(source_pairs->lane[lane], mul);
        window_out->lane[lane] = l3_auth_pair_add64(with_base, prod);
    }
}

/* ---- v102: first-loop concrete source/window instantiation ------------ */


/* ---- v103: stitched first-loop front+tail step ----------------------- */



/* ---- v104: late first-loop scalar/vector stores ---------------------- */



/* ---- v105: native-source array runner for the first post-seed loop ----- */

static l3_auth_qpair64 l3_auth_q_from_pair_window(const l3_auth_pair32 *p) {
    l3_auth_qpair64 q;
    q.lane[0] = p[0];
    q.lane[1] = p[1];
    return q;
}

static void l3_auth_q_to_pair_window(const l3_auth_qpair64 *q, l3_auth_pair32 *p) {
    p[0] = q->lane[0];
    p[1] = q->lane[1];
}

static void l3_authorization_finalizer_arrays_to_physical(
    const l3_authorization_finalizer_first_loop_arrays *arrays,
    l3_auth_pair32 physical[L3_AUTH_FINAL_STATE_SEED_FULL_PAIR_COUNT]) {
    for (unsigned i = 0; i < L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS; ++i) {
        physical[i] = arrays->local_328[i];
    }
    for (unsigned i = L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS;
         i < L3_AUTH_FINAL_STATE_SEED_FULL_PAIR_COUNT; ++i) {
        physical[i] = arrays->auStack_2c8[i - 12u];
    }
}

static void l3_authorization_finalizer_arrays_from_physical(
    const l3_auth_pair32 physical[L3_AUTH_FINAL_STATE_SEED_FULL_PAIR_COUNT],
    l3_authorization_finalizer_first_loop_arrays *arrays) {
    for (unsigned i = 0; i < L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS; ++i) {
        arrays->local_328[i] = physical[i];
        arrays->auStack_2f0[i] = physical[i + 7u];
    }
    for (unsigned i = 0; i < L3_AUTH_FINAL_STATE_FIRST_LOOP_WINDOW_PAIRS; ++i) {
        arrays->uStack_320[i] = physical[i + 1u];
        arrays->local_310[i] = physical[i + 3u];
        arrays->uStack_300[i] = physical[i + 5u];
        arrays->auStack_2e8[i] = physical[i + 8u];
        arrays->auStack_2d8[i] = physical[i + 10u];
        arrays->auStack_2c8[i] = physical[i + 12u];
    }
}

void l3_authorization_finalizer_first_loop_run13_exact_sources(
    uint32_t parent_lo,
    uint32_t parent_hi,
    const l3_authorization_finalizer_pair_array13 *sources,
    const l3_authorization_finalizer_first_loop_arrays *state_in,
    l3_authorization_finalizer_first_loop_run13_state *state_out) {
    if (!sources || !state_in || !state_out) return;
    memset(state_out, 0, sizeof(*state_out));
    l3_auth_pair32 physical[L3_AUTH_FINAL_STATE_SEED_FULL_PAIR_COUNT];
    l3_authorization_finalizer_arrays_to_physical(state_in, physical);

    /* Every named window aliases the same native 26-pair stack footprint.
       Keeping one physical array is required because a store through
       local_310 at iteration N is read through uStack_320 at iteration N+1. */
    l3_auth_pair32 loop_current = physical[0];

    const l3_auth_pair32 src_local328 = sources->pair[0];
    const l3_auth_pair32 src_2f0      = sources->pair[7];
    const l3_auth_pair32 src_2c8      = sources->pair[12];

    const l3_auth_qpair64 src_320 = {{ sources->pair[1], sources->pair[2] }};
    const l3_auth_qpair64 src_310 = {{ sources->pair[3], sources->pair[4] }};
    const l3_auth_qpair64 src_300 = {{ sources->pair[5], sources->pair[6] }};
    const l3_auth_qpair64 src_2e8 = {{ sources->pair[8], sources->pair[9] }};
    const l3_auth_qpair64 src_2d8 = {{ sources->pair[10], sources->pair[11] }};

    for (unsigned i = 0; i < L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS; ++i) {
        l3_authorization_finalizer_first_loop_coeffs coeffs;
        l3_authorization_finalizer_first_loop_coeffs_from_head(
            parent_lo, parent_hi, loop_current.lo, loop_current.hi, &coeffs);

        uint32_t cur_pair[2];
        uint32_t src_pair[2];
        uint32_t out_pair[2];

        cur_pair[0] = loop_current.lo;
        cur_pair[1] = loop_current.hi;
        src_pair[0] = src_local328.lo;
        src_pair[1] = src_local328.hi;
        l3_authorization_finalizer_first_loop_update_head_pair(cur_pair, src_pair, &coeffs, out_pair);
        physical[i].lo = out_pair[0];
        physical[i].hi = out_pair[1];

        cur_pair[0] = physical[i + 7u].lo;
        cur_pair[1] = physical[i + 7u].hi;
        src_pair[0] = src_2f0.lo;
        src_pair[1] = src_2f0.hi;
        l3_authorization_finalizer_first_loop_update_head_pair(cur_pair, src_pair, &coeffs, out_pair);
        physical[i + 7u].lo = out_pair[0];
        physical[i + 7u].hi = out_pair[1];

        l3_auth_qpair64 qin, qout;
        qin = l3_auth_q_from_pair_window(&physical[i + 1u]);
        l3_authorization_finalizer_vadd_i64_product_window2(&qin, &src_320, &coeffs, &qout);
        l3_auth_q_to_pair_window(&qout, &physical[i + 1u]);

        qin = l3_auth_q_from_pair_window(&physical[i + 3u]);
        l3_authorization_finalizer_vadd_i64_product_window2(&qin, &src_310, &coeffs, &qout);
        l3_auth_q_to_pair_window(&qout, &physical[i + 3u]);

        qin = l3_auth_q_from_pair_window(&physical[i + 5u]);
        l3_authorization_finalizer_vadd_i64_product_window2(&qin, &src_300, &coeffs, &qout);
        l3_auth_q_to_pair_window(&qout, &physical[i + 5u]);

        cur_pair[0] = physical[i + 12u].lo;
        cur_pair[1] = physical[i + 12u].hi;
        src_pair[0] = src_2c8.lo;
        src_pair[1] = src_2c8.hi;
        l3_authorization_finalizer_first_loop_update_head_pair(cur_pair, src_pair, &coeffs, out_pair);
        physical[i + 12u].lo = out_pair[0];
        physical[i + 12u].hi = out_pair[1];

        qin = l3_auth_q_from_pair_window(&physical[i + 8u]);
        l3_authorization_finalizer_vadd_i64_product_window2(&qin, &src_2e8, &coeffs, &qout);
        l3_auth_q_to_pair_window(&qout, &physical[i + 8u]);

        qin = l3_auth_q_from_pair_window(&physical[i + 10u]);
        l3_authorization_finalizer_vadd_i64_product_window2(&qin, &src_2d8, &coeffs, &qout);
        l3_auth_q_to_pair_window(&qout, &physical[i + 10u]);

        const uint32_t updated_head[2] = {
            physical[i].lo,
            physical[i].hi
        };
        const uint32_t window_after_vector[2] = {
            physical[i + 1u].lo,
            physical[i + 1u].hi
        };
        l3_authorization_finalizer_first_loop_tail_from_updated_head(
            updated_head, window_after_vector, &state_out->last_tail);
        physical[i + 1u].lo = state_out->last_tail.out_lo;
        physical[i + 1u].hi = state_out->last_tail.out_hi;

        loop_current.lo = state_out->last_tail.out_lo;
        loop_current.hi = state_out->last_tail.out_hi;
    }

    l3_authorization_finalizer_arrays_from_physical(physical, &state_out->arrays);
    state_out->final_loop_carry = loop_current;
}

/* ---- v106: DAT_f41a3130 reduction over auStack_2c8 -------------------- */

static const uint32_t k_final_state_reduce3130_mul[8] = {
    0xe5f5c5f9u, 0xc1ad1d6fu, 0xa8cb6761u, 0xc049c9e3u,
    0x73d14075u, 0x060c2c59u, 0x403bff1fu, 0xd3160ad5u
};

static const uint32_t k_final_state_reduce3130_add[8] = {
    0x74a3fdf2u, 0xd95838adu, 0xa32287a1u, 0x201a53cfu,
    0xbb1550a1u, 0x9b715da2u, 0x4dd5518cu, 0x718bb512u
};

static void l3_authorization_finalizer_fold_first_loop_step(uint32_t lo,
                                       uint32_t hi,
                                       uint32_t *out_lo,
                                       uint32_t *out_hi) {
    const unsigned idx = lo & 0x0fu;
    const uint32_t shifted_lo = (lo >> 4) | (hi << 28);
    const uint32_t shifted_hi = hi >> 4;
    const uint32_t new_lo = shifted_lo + k_final_state_fold_3130[idx].lo;
    const uint32_t new_hi = shifted_hi + k_final_state_fold_3130[idx].hi +
                            l3_auth_carry32(shifted_lo, k_final_state_fold_3130[idx].lo);
    *out_lo = new_lo;
    *out_hi = new_hi;
}

static uint32_t l3_authorization_finalizer_reduce_first_loop_low_word(uint32_t lo,
                                                          uint32_t hi) {
    const unsigned idx = lo & 0x0fu;
    const uint32_t shifted_lo = (lo >> 4) | (hi << 28);
    return shifted_lo + k_final_state_fold_3130[idx].lo;
}

static void l3_authorization_finalizer_reduce_workspace_first_impl(
    const l3_auth_pair32 auStack_2c8_window[L3_AUTH_FINAL_STATE_FIRST_LOOP_WINDOW_PAIRS],
    l3_authorization_finalizer_reduce3130_state *out,
    l3_authorization_finalizer_reduce3130_trace *trace) {
    if (!auStack_2c8_window || !out) return;
    memset(out, 0, sizeof(*out));
    if (trace) memset(trace, 0, sizeof(*trace));

    /* Native initial carry state:
         iVar55 = 0x132b7c4a;
         uVar61 = 0x36fdf557;
       We store it as hi/lo because the subsequent fold step treats
       (uVar61, iVar55) as a generated 64-bit accumulator. */
    uint32_t state_hi = 0x132b7c4au;
    uint32_t state_lo = 0x36fdf557u;

    for (unsigned i = 0; i < L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS; ++i) {
        const uint32_t in_lo = auStack_2c8_window[i + 1u].lo;
        const uint32_t in_hi = auStack_2c8_window[i + 1u].hi;

        const uint32_t prod_in_lo = l3_auth_mullo_u32(in_lo, 0xd9dd7a9bu);
        const uint32_t prod_in_hi = l3_auth_mulhi_u32(in_lo, 0xd9dd7a9bu);
        const uint32_t prod_state_lo = l3_auth_mullo_u32(state_lo, 0xf3566173u);
        const uint32_t prod_state_hi = l3_auth_mulhi_u32(state_lo, 0xf3566173u);
        const uint32_t sum_lo = prod_state_lo + prod_in_lo;
        const uint32_t pre_lo = sum_lo + 0x865d9c9bu; /* uVar56 */
        const uint32_t pre_mul_lo = l3_auth_mullo_u32(pre_lo, 0xb75d2985u);
        const uint32_t pre_mul_hi = l3_auth_mulhi_u32(pre_lo, 0xb75d2985u);
        const uint32_t fold_seed_lo = pre_mul_lo + 0x0c555d4au;

        uint32_t fold_seed_hi =
            state_hi * 0xf3566173u +
            state_lo * 0xaff9dc9fu +
            prod_state_hi +
            in_hi * 0xd9dd7a9bu +
            in_lo * 0x1ee0d6c3u +
            prod_in_hi +
            l3_auth_carry32(prod_state_lo, prod_in_lo) +
            0x71e72d11u +
            (uint32_t)(0x79a26364u < sum_lo);
        fold_seed_hi =
            fold_seed_hi * 0xb75d2985u +
            pre_lo * 0x8b2105c5u +
            pre_mul_hi +
            0x645cd7d8u +
            (uint32_t)(0xf3aaa2b5u < pre_mul_lo);

        uint32_t lo = fold_seed_lo;
        uint32_t hi = fold_seed_hi;
        for (unsigned r = 0; r < 7u; ++r) {
            l3_authorization_finalizer_fold_first_loop_step(lo, hi, &lo, &hi);
        }

        const uint32_t fold7_lo = lo; /* uVar68 */
        const uint32_t fold7_hi = hi; /* uVar79 */
        const uint32_t fold7_mul_lo = l3_auth_mullo_u32(fold7_lo, 0x9476e43fu);
        const uint32_t fold7_mul_hi = l3_auth_mulhi_u32(fold7_lo, 0x9476e43fu);

        for (unsigned r = 0; r < 8u; ++r) {
            l3_authorization_finalizer_fold_first_loop_step(lo, hi, &lo, &hi);
        }

        /* Native performs eight complete folds after fold7, then one
           low-word-only follow-on fold for the carried high-state term. */
        const uint32_t partial_low = l3_authorization_finalizer_reduce_first_loop_low_word(lo, hi);
        const uint32_t partial_product =
            l3_auth_mullo_u32(partial_low, 0xb891bc10u);
        const uint32_t state_hi_base =
            fold7_hi * 0x9476e43fu +
            fold7_lo * 0x789cfcddu +
            fold7_mul_hi;
        out->local_35c[i] =
            (pre_lo * 0x84372fd5u + fold7_lo * 0xf0000000u + 0xf0f7737cu) *
            k_final_state_reduce3130_mul[i & 7u] +
            k_final_state_reduce3130_add[i & 7u];

        state_lo = fold7_mul_lo + 0x246cb677u;
        state_hi =
            state_hi_base +
            partial_product +
            0xe0a690a2u +
            (uint32_t)(0xdb934988u < fold7_mul_lo);
        if (trace) {
            trace->input_lo[i] = in_lo;
            trace->input_hi[i] = in_hi;
            trace->fold_seed_lo[i] = fold_seed_lo;
            trace->fold_seed_hi[i] = fold_seed_hi;
            trace->fold7_lo[i] = fold7_lo;
            trace->fold7_hi[i] = fold7_hi;
            trace->partial_low[i] = partial_low;
            trace->partial_product[i] = partial_product;
            trace->state_hi_base[i] = state_hi_base;
            trace->state_lo[i] = state_lo;
            trace->state_hi[i] = state_hi;
        }
    }

    out->final_state.lo = state_lo;
    out->final_state.hi = state_hi;
}

void l3_authorization_finalizer_reduce_workspace_first(
    const l3_auth_pair32 auStack_2c8_window[L3_AUTH_FINAL_STATE_FIRST_LOOP_WINDOW_PAIRS],
    l3_authorization_finalizer_reduce3130_state *out) {
    l3_authorization_finalizer_reduce_workspace_first_impl(
        auStack_2c8_window, out, NULL);
}


/* ---- v107: LAB_f3fd5c50 param_3 seed loop ----------------------------- */

static const uint32_t k_final_state_param3_mul[8] = {
    0xb3e21fddu, 0x0949de0fu, 0xddd3cc65u, 0xb7d70773u,
    0xbdc9d2d5u, 0xf2637431u, 0xc7594571u, 0xffbdc86du
};

static const uint32_t k_final_state_param3_add[8] = {
    0x46154d44u, 0xe9479fcdu, 0x56a41bd0u, 0x31eee212u,
    0x1803d244u, 0x3d437fd7u, 0xdb6663b5u, 0x5c558a40u
};

void l3_authorization_finalizer_seed_param3_pairs(
    const uint32_t param3_words[L3_AUTH_FINAL_STATE_SEED_PAIR_COUNT],
    uint32_t out_words[L3_AUTH_FINAL_STATE_SEED_WORD_COUNT]) {
    if (!param3_words || !out_words) return;
    for (unsigned i = 0; i < L3_AUTH_FINAL_STATE_SEED_PAIR_COUNT; ++i) {
        const unsigned j = i & 7u;
        const uint32_t mixed = (uint32_t)((param3_words[i] * k_final_state_param3_mul[j] +
                                          k_final_state_param3_add[j]) *
                                         0x91410071u + 0x6618314fu);

        uint32_t lo0 = l3_auth_mullo_u32(mixed, 0x79c276a9u);
        uint32_t hi0 = (uint32_t)(mixed * 0x06ff35ecu +
                                  l3_auth_mulhi_u32(mixed, 0x79c276a9u) +
                                  0xc8c5ae36u +
                                  (uint32_t)(0x78ba1391u < lo0));
        lo0 += 0x8745ec6eu;

        const uint32_t folded_lo = (uint32_t)l3_authorization_finalizer_fold64(
            L3_AUTH_FOLD64_TABLE_31B0, ((uint64_t)hi0 << 32) | lo0, 8u);

        const uint32_t lo_mul = l3_auth_mullo_u32(mixed, 0x60d4f22fu);
        out_words[i * 2u + 0u] = lo_mul + 0xb9eb3fa3u;
        out_words[i * 2u + 1u] = (uint32_t)(mixed * 0x69f2b198u +
                                            l3_auth_mulhi_u32(mixed, 0x60d4f22fu) +
                                            folded_lo * 0x18a35ee9u +
                                            0x8aa39435u +
                                            (uint32_t)(0x4614c05cu < lo_mul));
    }
}

void l3_authorization_finalizer_seed_param3_pairs26(
    const uint32_t param3_words[L3_AUTH_FINAL_STATE_SEED_PAIR_COUNT],
    uint32_t out_words[L3_AUTH_FINAL_STATE_SEED_FULL_WORD_COUNT]) {
    if (!param3_words || !out_words) return;
    l3_authorization_finalizer_seed_param3_pairs(param3_words, out_words);
    for (unsigned i = L3_AUTH_FINAL_STATE_SEED_PAIR_COUNT;
         i < L3_AUTH_FINAL_STATE_SEED_FULL_PAIR_COUNT; ++i) {
        out_words[i * 2u + 0u] = 0x0a452a36u;
        out_words[i * 2u + 1u] = 0xec81fecbu;
    }
}


/* ---- v108: second parent-context seed loop after param_3 overwrite ----- */

static const uint32_t k_final_state_param1_second_mul[8] = {
    0x7bb0b75fu, 0x1b166ea5u, 0xdbc92ac1u, 0x0abce119u,
    0x5113a175u, 0xe50bf639u, 0x990c27b1u, 0x9ed3bc7du
};

static const uint32_t k_final_state_param1_second_add[8] = {
    0x2cda618eu, 0x10e1c53bu, 0xe6a1a102u, 0xf43ee7d6u,
    0x9d7c02f9u, 0xef3daa89u, 0xc153e02bu, 0x91c6939fu
};

void l3_authorization_finalizer_seed_param1_second_pairs(
    const uint32_t parent_words[L3_AUTH_FINAL_STATE_SEED_PAIR_COUNT],
    uint32_t out_words[L3_AUTH_FINAL_STATE_SEED_WORD_COUNT]) {
    if (!parent_words || !out_words) return;
    for (unsigned i = 0; i < L3_AUTH_FINAL_STATE_SEED_PAIR_COUNT; ++i) {
        const unsigned j = i & 7u;
        const uint32_t mixed = (uint32_t)((parent_words[i] * k_final_state_param1_second_mul[j] +
                                          k_final_state_param1_second_add[j]) *
                                         0x8d818bcfu + 0x94e6003eu);

        uint32_t lo0 = l3_auth_mullo_u32(mixed, 0x4984851fu);
        uint32_t hi0 = (uint32_t)(mixed * 0xf262d1e9u +
                                  l3_auth_mulhi_u32(mixed, 0x4984851fu) +
                                  0x0a0fd017u +
                                  (uint32_t)(0xb6850655u < lo0));
        lo0 += 0x497af9aau;

        const uint32_t folded_lo = (uint32_t)l3_authorization_finalizer_fold64(
            L3_AUTH_FOLD64_TABLE_3230, ((uint64_t)hi0 << 32) | lo0, 8u);

        const uint32_t lo_mul = l3_auth_mullo_u32(mixed, 0x7330cd75u);
        out_words[i * 2u + 0u] = lo_mul + 0x8c3c3f89u;
        out_words[i * 2u + 1u] = (uint32_t)(mixed * 0x3a09fc9du +
                                            l3_auth_mulhi_u32(mixed, 0x7330cd75u) +
                                            folded_lo * 0xabc7d915u +
                                            0x08f22e13u +
                                            (uint32_t)(0x73c3c076u < lo_mul));
    }
}

/* ---- v109: DAT_f41a32b0 coefficient head after second parent reseed ----- */

static void l3_authorization_finalizer_fold_second_loop_step(uint32_t lo,
                                       uint32_t hi,
                                       uint32_t *out_lo,
                                       uint32_t *out_hi) {
    const unsigned idx = lo & 0x0fu;
    const uint32_t shifted_lo = (lo >> 4) | (hi << 28);
    const uint32_t shifted_hi = hi >> 4;
    const uint32_t new_lo = shifted_lo + k_final_state_fold_32b0[idx].lo;
    const uint32_t new_hi = shifted_hi + k_final_state_fold_32b0[idx].hi +
                            l3_auth_carry32(shifted_lo, k_final_state_fold_32b0[idx].lo);
    *out_lo = new_lo;
    *out_hi = new_hi;
}

void l3_authorization_finalizer_second_loop_coeffs_from_head_trace(
    uint32_t parent_lo, uint32_t parent_hi,
    uint32_t current_lo, uint32_t current_hi,
    l3_authorization_finalizer_first_loop_coeffs *out,
    l3_authorization_finalizer_second_loop_coeff_trace *trace) {
    if (!out) return;
    memset(out, 0, sizeof(*out));
    if (trace) memset(trace, 0, sizeof(*trace));

    /* Native setup before the do-loop:
         uVar63 = lo(parent_lo * 0xa46ca182);
         uVar57 = lo(parent_lo * 0x4f2b84ad);
         uVar79 = uVar63 + 0x42d7541d;
         uVar56 = uVar57 + 0x4fe90f2a. */
    const uint32_t p_mul_a_lo = l3_auth_mullo_u32(parent_lo, 0xa46ca182u);
    const uint32_t p_mul_a_hi = l3_auth_mulhi_u32(parent_lo, 0xa46ca182u);
    const uint32_t p_mul_b_lo = l3_auth_mullo_u32(parent_lo, 0x4f2b84adu);
    const uint32_t p_mul_b_hi = l3_auth_mulhi_u32(parent_lo, 0x4f2b84adu);

    const uint32_t base_a_lo = p_mul_a_lo + 0x42d7541du; /* uVar79 */
    const uint32_t base_b_lo = p_mul_b_lo + 0x4fe90f2au; /* uVar56 */

    const uint64_t cur_mul_b = (uint64_t)base_b_lo * (uint64_t)current_lo;
    const uint32_t cur_mul_b_lo = (uint32_t)cur_mul_b;
    const uint32_t cur_mul_b_hi = (uint32_t)(cur_mul_b >> 32);
    const uint32_t affine_lo = base_a_lo + cur_mul_b_lo; /* uVar66 */

    const uint32_t affine_hi =
        parent_hi * 0xa46ca182u +
        parent_lo * 0xb8f179bfu +
        p_mul_a_hi +
        0x0b4210feu +
        (uint32_t)(0xbd28abe2u < p_mul_a_lo) +
        current_hi * base_b_lo +
        current_lo * (parent_hi * 0x4f2b84adu +
                      parent_lo * 0xf84c17f7u +
                      p_mul_b_hi +
                      0xe2e3520cu +
                      (uint32_t)(0xb016f0d5u < p_mul_b_lo)) +
        cur_mul_b_hi +
        l3_auth_carry32(base_a_lo, cur_mul_b_lo);

    const uint32_t fold_seed_mul_lo = l3_auth_mullo_u32(affine_lo, 0xa75c61b3u);
    const uint32_t fold_seed_mul_hi = l3_auth_mulhi_u32(affine_lo, 0xa75c61b3u);
    uint32_t fold_lo = fold_seed_mul_lo + 0x66e861d2u;
    uint32_t fold_hi =
        affine_hi * 0xa75c61b3u +
        affine_lo * 0x87cfdf27u +
        fold_seed_mul_hi +
        0x9b34b08au +
        (uint32_t)(0x99179e2du < fold_seed_mul_lo);

    if (trace) {
        trace->affine_lo = affine_lo;
        trace->affine_hi = affine_hi;
        trace->fold_seed_lo = fold_lo;
        trace->fold_seed_hi = fold_hi;
    }

    for (unsigned r = 0; r < 7u; ++r) {
        l3_authorization_finalizer_fold_second_loop_step(fold_lo, fold_hi, &fold_lo, &fold_hi);
    }

    if (trace) {
        trace->fold32b0_lo = fold_lo;
        trace->fold32b0_hi = fold_hi;
    }

    const uint64_t fold_b_mul = (uint64_t)fold_lo * 0xb0000000ull;
    const uint32_t fold_b_lo = (uint32_t)fold_b_mul;
    const uint32_t fold_b_hi = (uint32_t)(fold_b_mul >> 32);
    const uint32_t affine_b_mul_lo = l3_auth_mullo_u32(affine_lo, 0xa5d4943fu);
    const uint32_t affine_b_mul_hi = l3_auth_mulhi_u32(affine_lo, 0xa5d4943fu);
    const uint32_t mix_sum_lo = affine_b_mul_lo + fold_b_lo;
    const uint32_t mix_a_lo = mix_sum_lo + 0xe2ed5378u; /* uVar58 */

    const uint32_t mix_a_hi =
        affine_hi * 0xa5d4943fu +
        affine_lo * 0xa9777f47u +
        affine_b_mul_hi +
        fold_lo * 0x0b87cbabu +
        fold_b_hi +
        fold_hi * 0xb0000000u +
        l3_auth_carry32(affine_b_mul_lo, fold_b_lo) +
        0x8674021cu +
        (uint32_t)(0x1d12ac87u < mix_sum_lo);

    const uint32_t coeff_mul_mul_lo = l3_auth_mullo_u32(mix_a_lo, 0x7ed81121u);
    const uint32_t coeff_mul_mul_hi = l3_auth_mulhi_u32(mix_a_lo, 0x7ed81121u);
    const uint32_t coeff_mul_lo = coeff_mul_mul_lo + 0x5ef95e61u;
    const uint32_t coeff_mul_hi =
        mix_a_hi * 0x7ed81121u +
        mix_a_lo * 0x69483e9au +
        coeff_mul_mul_hi +
        0x46873403u +
        (uint32_t)(0xa106a19eu < coeff_mul_mul_lo);

    const uint32_t coeff_add_mul_lo = l3_auth_mullo_u32(mix_a_lo, 0xffb2f797u);
    const uint32_t coeff_add_mul_hi = l3_auth_mulhi_u32(mix_a_lo, 0xffb2f797u);
    const uint32_t coeff_add_lo = coeff_add_mul_lo + 0x292c7057u;
    const uint32_t coeff_add_hi =
        mix_a_hi * 0xffb2f797u +
        mix_a_lo * 0xe06494eeu +
        coeff_add_mul_hi +
        0x9193c7f3u +
        (uint32_t)(0xd6d38fa8u < coeff_add_mul_lo);

    out->mul_lo = coeff_mul_lo;
    out->mul_hi = coeff_mul_hi;
    out->add_lo = coeff_add_lo;
    out->add_hi = coeff_add_hi;

    if (trace) {
        trace->mix_a_lo = mix_a_lo;
        trace->mix_a_hi = mix_a_hi;
        trace->coeff_mul_lo = coeff_mul_lo;
        trace->coeff_mul_hi = coeff_mul_hi;
        trace->coeff_add_lo = coeff_add_lo;
        trace->coeff_add_hi = coeff_add_hi;
    }
}

void l3_authorization_finalizer_second_loop_coeffs_from_head(
    uint32_t parent_lo, uint32_t parent_hi,
    uint32_t current_lo, uint32_t current_hi,
    l3_authorization_finalizer_first_loop_coeffs *out) {
    l3_authorization_finalizer_second_loop_coeffs_from_head_trace(
        parent_lo, parent_hi, current_lo, current_hi, out, 0);
}

/* ---- v110: second carry/window loop downstream store mapping ---------- */

void l3_authorization_finalizer_second_loop_front_update(
    const l3_authorization_finalizer_first_loop_front_state *state_in,
    const l3_authorization_finalizer_second_loop_front_sources *sources,
    const l3_authorization_finalizer_first_loop_coeffs *coeffs,
    l3_authorization_finalizer_first_loop_front_state *state_out) {
    if (!state_in || !sources || !coeffs || !state_out) return;

    *state_out = *state_in;

    /* Native stores at lines 959..998 of the decompiled fd4a60 slice:
       local_328 uses local_258[0..1]; auStack_2f0 uses the separately loaded
       local_220/iStack_21c pair (source pair 7 in the 13-pair array). */
    uint32_t cur328[2] = { state_in->local_328.lo, state_in->local_328.hi };
    uint32_t src01[2] = { sources->local258_0_1.lo, sources->local258_0_1.hi };
    uint32_t outpair[2];
    l3_authorization_finalizer_first_loop_update_head_pair(cur328, src01, coeffs, outpair);
    state_out->local_328.lo = outpair[0];
    state_out->local_328.hi = outpair[1];

    uint32_t cur2f0[2] = { state_in->auStack_2f0.lo, state_in->auStack_2f0.hi };
    uint32_t src220[2] = { sources->local220_21c.lo, sources->local220_21c.hi };
    l3_authorization_finalizer_first_loop_update_head_pair(cur2f0, src220, coeffs, outpair);
    state_out->auStack_2f0.lo = outpair[0];
    state_out->auStack_2f0.hi = outpair[1];

    /* Native vector stores:
       local_258[2..5]   -> uStack_320/local_318
       local_258[6..9]   -> local_310/local_308
       local_258[10..13] -> uStack_300/local_2f8 */
    l3_authorization_finalizer_vadd_i64_product_window2(&state_in->uStack_320,
                                           &sources->local258_2_5,
                                           coeffs,
                                           &state_out->uStack_320);
    l3_authorization_finalizer_vadd_i64_product_window2(&state_in->local_310,
                                           &sources->local258_6_9,
                                           coeffs,
                                           &state_out->local_310);
    l3_authorization_finalizer_vadd_i64_product_window2(&state_in->uStack_300,
                                           &sources->local258_10_13,
                                           coeffs,
                                           &state_out->uStack_300);
}


/* ---- v111: second-loop DAT_f41a3330 / DAT_f41a33b0 scalar tail -------- */

void l3_authorization_finalizer_second_loop_scalar_tail_from_updated_head(
    const uint32_t updated_head_pair[2],
    const uint32_t window_pair_after_vector_add[2],
    l3_authorization_finalizer_second_loop_scalar_tail_state *out) {
    if (!updated_head_pair || !window_pair_after_vector_add || !out) return;
    memset(out, 0, sizeof(*out));

    const uint32_t head_lo = updated_head_pair[0];   /* native uVar48 */
    const uint32_t head_hi = updated_head_pair[1];   /* native iVar50 */

    /* Native seed before DAT_f41a3330:
       uVar58/uVar61 = head * {0x03707449, 0x3ce403fa} + {0x68e498be, -0x257c24ef}. */
    const uint32_t m3330_lo = l3_auth_mullo_u32(head_lo, 0x03707449u);
    const uint32_t m3330_hi = l3_auth_mulhi_u32(head_lo, 0x03707449u);
    const uint32_t x3330_lo = m3330_lo + 0x68e498beu;
    const uint32_t x3330_hi = (uint32_t)(
        head_hi * 0x03707449u +
        head_lo * 0x3ce403fau +
        m3330_hi +
        0xda83db11u +
        l3_auth_carry32(m3330_lo, 0x68e498beu));

    const uint64_t folded3330 = l3_authorization_finalizer_fold64(
        L3_AUTH_FOLD64_TABLE_3330, ((uint64_t)x3330_hi << 32) | x3330_lo, 7u);
    const uint32_t f3330_lo = (uint32_t)folded3330;
    const uint32_t f3330_hi = (uint32_t)(folded3330 >> 32);
    out->fold3330_lo = f3330_lo;
    out->fold3330_hi = f3330_hi;

    /* Affine mix between the two table families. */
    const uint32_t mid0_mul_lo = l3_auth_mullo_u32(f3330_lo, 0xf6665e41u);
    const uint32_t mid0_mul_hi = l3_auth_mulhi_u32(f3330_lo, 0xf6665e41u);
    const uint32_t mid0_lo = mid0_mul_lo + 0x28c753d1u;
    const uint32_t mid0_hi = (uint32_t)(
        f3330_hi * 0xf6665e41u +
        f3330_lo * 0xaf4f5f22u +
        mid0_mul_hi +
        0xd5cd95adu +
        l3_auth_carry32(mid0_mul_lo, 0x28c753d1u));

    const uint32_t m33b0_lo = l3_auth_mullo_u32(mid0_lo, 0xea573e89u);
    const uint32_t m33b0_hi = l3_auth_mulhi_u32(mid0_lo, 0xea573e89u);
    const uint32_t x33b0_lo = m33b0_lo + 0xa42d82a8u;
    const uint32_t x33b0_hi = (uint32_t)(
        mid0_hi * 0xea573e89u +
        mid0_lo * 0xab292b4eu +
        m33b0_hi +
        0xba05e116u +
        l3_auth_carry32(m33b0_lo, 0xa42d82a8u));

    /* Native low-tail: seven full folds plus one low-only step, then another
       low-only table lookup after the resulting low word has been shifted. */
    const uint64_t folded33b0 = l3_authorization_finalizer_fold64(
        L3_AUTH_FOLD64_TABLE_33B0, ((uint64_t)x33b0_hi << 32) | x33b0_lo, 7u);
    const uint32_t f33b0_lo = (uint32_t)folded33b0;
    const uint32_t f33b0_hi = (uint32_t)(folded33b0 >> 32);
    const l3_auth_pair64 *tab33b0 = l3_auth_fold_table(L3_AUTH_FOLD64_TABLE_33B0);
    const uint32_t tail_shift_lo = (f33b0_lo >> 4) | (f33b0_hi << 28);
    const uint32_t tail_lo = tail_shift_lo + tab33b0[f33b0_lo & 0x0fu].lo;
    const uint32_t tail_mix = (tail_lo >> 4) + tab33b0[tail_lo & 0x0fu].lo;
    out->fold33b0_tail_lo = tail_lo;
    out->fold33b0_tail_mix = tail_mix;

    out->mid_lo = mid0_lo;
    out->mid_hi = mid0_hi;

    /* Final native overwrite of low uStack_320 pair at the same loop offset. */
    const uint32_t p579_lo = l3_auth_mullo_u32(mid0_lo, 0x579ccc4fu);
    const uint32_t p579_hi = l3_auth_mulhi_u32(mid0_lo, 0x579ccc4fu);
    const uint32_t l4_lo = l3_auth_mullo_u32(p579_lo, 0xb337c777u);
    const uint32_t l4_hi = l3_auth_mulhi_u32(p579_lo, 0xb337c777u);
    const uint32_t sum0 = l4_lo + window_pair_after_vector_add[0];
    out->out_lo = sum0 + 0xde287f42u;
    out->out_hi = (uint32_t)(
        (mid0_hi * 0x579ccc4fu +
         mid0_lo * 0xa1f14017u +
         p579_hi +
         tail_mix * 0xca6c1e90u) * 0xb337c777u +
        p579_lo * 0x8d001eedu +
        l4_hi +
        window_pair_after_vector_add[1] +
        l3_auth_carry32(l4_lo, window_pair_after_vector_add[0]) +
        0x1581b2a0u +
        l3_auth_carry32(sum0, 0xde287f42u));
}


/* ---- v112: second-loop late scalar/vector stores and full step -------- */

void l3_authorization_finalizer_second_loop_late_update(
    const l3_authorization_finalizer_second_loop_late_state *state_in,
    const l3_authorization_finalizer_second_loop_late_sources *sources,
    const l3_authorization_finalizer_first_loop_coeffs *coeffs,
    l3_authorization_finalizer_second_loop_late_state *state_out) {
    if (!state_in || !sources || !coeffs || !state_out) return;

    *state_out = *state_in;

    /* Native f3fd5b00..f3fd5b14 scalar update of auStack_2c8.
       It is the same 64-bit affine pair update used for local_328 and
       auStack_2f0, now with source {local_1f8,local_1f4}. */
    uint32_t cur2c8[2] = { state_in->auStack_2c8.lo, state_in->auStack_2c8.hi };
    uint32_t src1f8[2] = { sources->local_1f8_1f4.lo, sources->local_1f8_1f4.hi };
    uint32_t outpair[2];
    l3_authorization_finalizer_first_loop_update_head_pair(cur2c8, src1f8, coeffs, outpair);
    state_out->auStack_2c8.lo = outpair[0];
    state_out->auStack_2c8.hi = outpair[1];

    /* Native f3fd5af0..f3fd5b0c: local_218 source vector -> auStack_2e8. */
    l3_authorization_finalizer_vadd_i64_product_window2(&state_in->auStack_2e8,
                                           &sources->local_218,
                                           coeffs,
                                           &state_out->auStack_2e8);

    /* Native f3fd5b20..f3fd5b2c: local_208 source vector -> auStack_2d8. */
    l3_authorization_finalizer_vadd_i64_product_window2(&state_in->auStack_2d8,
                                           &sources->local_208,
                                           coeffs,
                                           &state_out->auStack_2d8);
}

void l3_authorization_finalizer_second_loop_full_step(
    const l3_authorization_finalizer_first_loop_front_state *front_in,
    const l3_authorization_finalizer_second_loop_front_sources *front_sources,
    const l3_authorization_finalizer_second_loop_late_state *late_in,
    const l3_authorization_finalizer_second_loop_late_sources *late_sources,
    const l3_authorization_finalizer_first_loop_coeffs *coeffs,
    l3_authorization_finalizer_second_loop_full_step_state *state_out) {
    if (!front_in || !front_sources || !late_in || !late_sources || !coeffs || !state_out) return;
    memset(state_out, 0, sizeof(*state_out));

    l3_authorization_finalizer_second_loop_front_update(front_in, front_sources, coeffs, &state_out->front);

    const uint32_t updated_head[2] = {
        state_out->front.local_328.lo,
        state_out->front.local_328.hi
    };
    const uint32_t window_after_vector[2] = {
        state_out->front.uStack_320.lane[0].lo,
        state_out->front.uStack_320.lane[0].hi
    };
    l3_authorization_finalizer_second_loop_scalar_tail_from_updated_head(updated_head,
                                                           window_after_vector,
                                                           &state_out->tail);

    l3_authorization_finalizer_second_loop_late_update(late_in, late_sources, coeffs, &state_out->late);

    /* Native writes the scalar-tail result to the low uStack_320 pair after
       the late auStack_2c8/2e8/2d8 stores.  It does not affect those late
       store inputs, but it is part of the completed same-offset step. */
    state_out->front.uStack_320.lane[0].lo = state_out->tail.out_lo;
    state_out->front.uStack_320.lane[0].hi = state_out->tail.out_hi;
}


/* ---- v113: second carry/window loop 13-offset runner ------------------ */

void l3_authorization_finalizer_second_loop_run13_exact_sources(
    uint32_t parent_lo,
    uint32_t parent_hi,
    const l3_authorization_finalizer_pair_array13 *sources,
    const l3_authorization_finalizer_first_loop_arrays *state_in,
    l3_authorization_finalizer_second_loop_run13_state *state_out) {
    if (!sources || !state_in || !state_out) return;
    memset(state_out, 0, sizeof(*state_out));
    l3_auth_pair32 physical[L3_AUTH_FINAL_STATE_SEED_FULL_PAIR_COUNT];
    l3_authorization_finalizer_arrays_to_physical(state_in, physical);

    /* Native pre-loads the local_258 source windows before entering the loop.
       The same source windows are reused for all local_3c8 offsets. */
    const l3_authorization_finalizer_second_loop_front_sources front_sources = {
        sources->pair[0],
        {{ sources->pair[1], sources->pair[2] }},
        {{ sources->pair[3], sources->pair[4] }},
        {{ sources->pair[5], sources->pair[6] }},
        sources->pair[7]
    };
    const l3_authorization_finalizer_second_loop_late_sources late_sources = {
        sources->pair[12],
        {{ sources->pair[8], sources->pair[9] }},
        {{ sources->pair[10], sources->pair[11] }}
    };

    l3_auth_pair32 loop_current = physical[0];

    for (unsigned i = 0; i < L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS; ++i) {
        l3_authorization_finalizer_first_loop_coeffs coeffs;
        l3_authorization_finalizer_second_loop_coeffs_from_head(
            parent_lo, parent_hi, loop_current.lo, loop_current.hi, &coeffs);

        l3_authorization_finalizer_first_loop_front_state front_in;
        memset(&front_in, 0, sizeof(front_in));
        front_in.local_328 = loop_current;
        front_in.auStack_2f0 = physical[i + 7u];
        front_in.uStack_320 = l3_auth_q_from_pair_window(&physical[i + 1u]);
        front_in.local_310 = l3_auth_q_from_pair_window(&physical[i + 3u]);
        front_in.uStack_300 = l3_auth_q_from_pair_window(&physical[i + 5u]);

        l3_authorization_finalizer_second_loop_late_state late_in;
        memset(&late_in, 0, sizeof(late_in));
        late_in.auStack_2c8 = physical[i + 12u];
        late_in.auStack_2e8 = l3_auth_q_from_pair_window(&physical[i + 8u]);
        late_in.auStack_2d8 = l3_auth_q_from_pair_window(&physical[i + 10u]);

        l3_authorization_finalizer_second_loop_full_step_state step;
        l3_authorization_finalizer_second_loop_full_step(&front_in, &front_sources,
                                            &late_in, &late_sources,
                                            &coeffs, &step);

        physical[i] = step.front.local_328;
        physical[i + 7u] = step.front.auStack_2f0;
        l3_auth_q_to_pair_window(&step.front.uStack_320, &physical[i + 1u]);
        l3_auth_q_to_pair_window(&step.front.local_310, &physical[i + 3u]);
        l3_auth_q_to_pair_window(&step.front.uStack_300, &physical[i + 5u]);
        physical[i + 12u] = step.late.auStack_2c8;
        l3_auth_q_to_pair_window(&step.late.auStack_2e8, &physical[i + 8u]);
        l3_auth_q_to_pair_window(&step.late.auStack_2d8, &physical[i + 10u]);

        if (i == 0u) {
            memcpy(state_out->iteration0_physical, physical,
                   sizeof(state_out->iteration0_physical));
        }
        state_out->last_tail = step.tail;
        loop_current.lo = step.tail.out_lo;
        loop_current.hi = step.tail.out_hi;
    }

    l3_authorization_finalizer_arrays_from_physical(physical, &state_out->arrays);
    state_out->final_loop_carry = loop_current;
}


/* ---- v114: DAT_f41a3430 reduction over post-second-loop auStack_2c8 ---- */

static const uint32_t k_final_state_reduce3430_mul[8] = {
    0x32f27f1bu, 0xd33ac507u, 0x7c95d2b1u, 0x893c4be3u,
    0x9d7c34b3u, 0x6d2e298du, 0x3e23f6bdu, 0x82ae7f61u
};

static const uint32_t k_final_state_reduce3430_add[8] = {
    0x471754b9u, 0x5bd9e13bu, 0xa9ea3318u, 0xcc9cef0bu,
    0x5f90b173u, 0x3e9ee9d1u, 0x79d01fc4u, 0x82ffd7ccu
};

static void l3_authorization_finalizer_fold_second_loop_tail_step(uint32_t lo,
                                       uint32_t hi,
                                       uint32_t *out_lo,
                                       uint32_t *out_hi) {
    const unsigned idx = lo & 0x0fu;
    const uint32_t shifted_lo = (lo >> 4) | (hi << 28);
    const uint32_t shifted_hi = hi >> 4;
    const uint32_t new_lo = shifted_lo + k_final_state_fold_3430[idx].lo;
    const uint32_t new_hi = shifted_hi + k_final_state_fold_3430[idx].hi +
                            l3_auth_carry32(shifted_lo, k_final_state_fold_3430[idx].lo);
    *out_lo = new_lo;
    *out_hi = new_hi;
}

static uint32_t l3_authorization_finalizer_fold_second_loop_tail_low_word(uint32_t lo, uint32_t hi) {
    const unsigned idx = lo & 0x0fu;
    const uint32_t shifted_lo = (lo >> 4) | (hi << 28);
    return shifted_lo + k_final_state_fold_3430[idx].lo;
}

void l3_authorization_finalizer_reduce_workspace_second(
    const l3_auth_pair32 auStack_2c8_window[L3_AUTH_FINAL_STATE_FIRST_LOOP_WINDOW_PAIRS],
    l3_authorization_finalizer_reduce3430_state *out) {
    if (!auStack_2c8_window || !out) return;
    memset(out, 0, sizeof(*out));

    uint32_t state_hi = 0x15e0e14eu; /* native iVar55 */
    uint32_t state_lo = 0x59fd28ceu; /* native uVar61 */

    for (unsigned i = 0; i < L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS; ++i) {
        const uint32_t in_lo = auStack_2c8_window[i + 1u].lo;
        const uint32_t in_hi = auStack_2c8_window[i + 1u].hi;

        const uint32_t prod_in_lo = l3_auth_mullo_u32(in_lo, 0x969b595du);
        const uint32_t prod_in_hi = l3_auth_mulhi_u32(in_lo, 0x969b595du);
        const uint32_t prod_state_lo = l3_auth_mullo_u32(state_lo, 0x6f09dfedu);
        const uint32_t prod_state_hi = l3_auth_mulhi_u32(state_lo, 0x6f09dfedu);

        const uint32_t sum_lo = prod_state_lo + prod_in_lo;
        const uint32_t pre_lo = sum_lo + 0xb0f791adu; /* native uVar79 */
        const uint32_t pre_mul_lo = l3_auth_mullo_u32(pre_lo, 0xc636f2c7u);
        const uint32_t fold_seed_lo = pre_mul_lo + 0x5fb16312u; /* native uVar68 */

        uint32_t fold_seed_hi =
            state_hi * 0x6f09dfedu +
            state_lo * 0x4d220497u +
            prod_state_hi +
            in_hi * 0x969b595du +
            in_lo * 0x39535d96u +
            prod_in_hi +
            l3_auth_carry32(prod_state_lo, prod_in_lo) +
            0x02265481u +
            (uint32_t)(0x4f086e52u < sum_lo);
        fold_seed_hi =
            fold_seed_hi * 0xc636f2c7u +
            pre_lo * 0x11287ca4u +
            l3_auth_mulhi_u32(pre_lo, 0xc636f2c7u) +
            0x0122e7fdu +
            (uint32_t)(0xa04e9cedu < pre_mul_lo);

        uint32_t lo = fold_seed_lo;
        uint32_t hi = fold_seed_hi;
        for (unsigned r = 0; r < 7u; ++r) {
            l3_authorization_finalizer_fold_second_loop_tail_step(lo, hi, &lo, &hi);
        }

        const uint32_t fold7_lo = lo; /* native uVar28 */
        const uint32_t fold7_hi = hi; /* native uVar19 */
        const uint32_t fold7_mul_lo = l3_auth_mullo_u32(fold7_lo, 0x8d7905f3u);
        const uint32_t fold7_mul_hi = l3_auth_mulhi_u32(fold7_lo, 0x8d7905f3u);

        /* Native then runs a long same-table low-tail.  It materializes six
           full folds, one extra full fold used only through its low word and
           high carry, and one additional low-only table lookup. */
        for (unsigned r = 0; r < 6u; ++r) {
            l3_authorization_finalizer_fold_second_loop_tail_step(lo, hi, &lo, &hi);
        }
        uint32_t tail_lo, tail_hi;
        l3_authorization_finalizer_fold_second_loop_tail_step(lo, hi, &tail_lo, &tail_hi);
        const uint32_t partial_low = l3_authorization_finalizer_fold_second_loop_tail_low_word(tail_lo, tail_hi);
        const uint32_t partial_low2 = (partial_low >> 4) +
            k_final_state_fold_3430[partial_low & 0x0fu].lo;

        out->local_390[i] =
            (pre_lo * 0x3b236333u + fold7_lo * 0xb0000000u + 0xe05894dcu) *
            k_final_state_reduce3430_mul[i & 7u] +
            k_final_state_reduce3430_add[i & 7u];

        state_lo = fold7_mul_lo + 0x28b3b0d3u;
        state_hi =
            fold7_hi * 0x8d7905f3u +
            fold7_lo * 0x22d81068u +
            fold7_mul_hi +
            partial_low2 * 0x286fa0d0u +
            0xeab882fcu +
            (uint32_t)(0xd74c4f2cu < fold7_mul_lo);
    }

    out->final_state.lo = state_lo;
    out->final_state.hi = state_hi;
}


/* ---- v115: post-local_390 seed staging ---------------------------------- */

static const uint32_t k_final_state_post_convolution_param3_mul[8] = {
    0xe39718d7u, 0xb48b13edu, 0xd469906fu, 0x85f95119u,
    0x3e139fbfu, 0x926ff2d3u, 0x9bba3693u, 0xf003d687u
};

static const uint32_t k_final_state_post_convolution_param3_add[8] = {
    0xe7f0ca04u, 0xfe2b5ddfu, 0xf979d0a8u, 0x4c3387eeu,
    0x7daf9904u, 0x0c5da47du, 0x3c50b397u, 0xb64017f8u
};

static const uint32_t k_final_state_post_convolution_l390_mul[8] = {
    0x230ecc9bu, 0xb43c969fu, 0x64742b29u, 0x408e6293u,
    0xc468d7c3u, 0x8d584dfdu, 0x16d1b4cdu, 0x128914f9u
};

static const uint32_t k_final_state_post_convolution_l390_add[8] = {
    0x5cf5b1bdu, 0xf097f41bu, 0xd8c032e8u, 0x02fdf06fu,
    0x79c0aa27u, 0xc07e7933u, 0x43b429ccu, 0xb0d69454u
};

static l3_auth_pair32 l3_authorization_finalizer_post_convolution_param3_pair(unsigned lane, uint32_t word) {
    const unsigned j = lane & 7u;
    const uint32_t mixed = l3_auth_u32(
        l3_auth_u32(word * k_final_state_post_convolution_param3_mul[j] +
                  k_final_state_post_convolution_param3_add[j]) * 0xedbca817u +
        0xd0fd11bfu);

    const uint32_t lo_seed_mul = l3_auth_mullo_u32(mixed, 0x83526785u);
    const uint32_t lo_seed = lo_seed_mul + 0x592e0009u;
    const uint32_t hi_seed = l3_auth_u32(
        mixed * 0x162e1bc1u + l3_auth_mulhi_u32(mixed, 0x83526785u) +
        0x19393c77u + l3_auth_carry32(lo_seed_mul, 0x592e0009u));
    const uint64_t folded = l3_authorization_finalizer_fold64(
        L3_AUTH_FOLD64_TABLE_34B0, ((uint64_t)hi_seed << 32) | lo_seed, 8u);
    const uint32_t f_lo = (uint32_t)folded;

    const uint32_t out_mul = l3_auth_mullo_u32(mixed, 0x8c19f3b1u);
    l3_auth_pair32 out;
    out.lo = out_mul + 0x1bb6ae96u;
    out.hi = l3_auth_u32(
        mixed * 0x162b7854u + l3_auth_mulhi_u32(mixed, 0x8c19f3b1u) +
        f_lo * 0x109e0ac3u - 0x12c6bccfu +
        l3_auth_carry32(out_mul, 0x1bb6ae96u));
    return out;
}

static l3_auth_pair32 l3_authorization_finalizer_post_convolution_local390_pair(unsigned lane, uint32_t word) {
    const unsigned j = lane & 7u;
    const uint32_t mixed = l3_auth_u32(
        l3_auth_u32(word * k_final_state_post_convolution_l390_mul[j] +
                  k_final_state_post_convolution_l390_add[j]) * 0xe182881du +
        0xf5564d79u);

    const uint32_t lo_seed_mul = l3_auth_mullo_u32(mixed, 0x2b4d190du);
    const uint32_t lo_seed = lo_seed_mul + 0xe2181d73u;
    const uint32_t hi_seed = l3_auth_u32(
        mixed * 0xe217169au + l3_auth_mulhi_u32(mixed, 0x2b4d190du) -
        0x691f0860u + l3_auth_carry32(lo_seed_mul, 0xe2181d73u));
    const uint64_t folded = l3_authorization_finalizer_fold64(
        L3_AUTH_FOLD64_TABLE_3530, ((uint64_t)hi_seed << 32) | lo_seed, 8u);
    const uint32_t f_lo = (uint32_t)folded;

    const uint32_t out_mul = l3_auth_mullo_u32(mixed, 0xbdc627a9u);
    l3_auth_pair32 out;
    out.lo = out_mul + 0x34359b4cu;
    out.hi = l3_auth_u32(
        mixed * 0xe3aeee10u + l3_auth_mulhi_u32(mixed, 0xbdc627a9u) +
        f_lo * 0xcfb115f3u - 0x246a3b7eu +
        l3_auth_carry32(out_mul, 0x34359b4cu));
    return out;
}

void l3_authorization_finalizer_post_convolution_seed_staging(
    const uint32_t param3_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_390[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_post_convolution_seed_state *out) {
    if (!param3_words || !local_390 || !out) return;
    memset(out, 0, sizeof(*out));

    l3_auth_pair32 acc_param3 = {0, 0};
    l3_auth_pair32 acc_l390 = {0, 0};
    for (unsigned i = 0; i < L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS; ++i) {
        const l3_auth_pair32 p = l3_authorization_finalizer_post_convolution_param3_pair(i, param3_words[i]);
        out->local_258[i] = p;
        const uint32_t c0 = l3_auth_carry32(acc_param3.lo, p.lo);
        acc_param3.lo += p.lo;
        acc_param3.hi = l3_auth_u32(acc_param3.hi + p.hi + c0);
        out->auStack_128_prefix[i] = acc_param3;

        const l3_auth_pair32 q = l3_authorization_finalizer_post_convolution_local390_pair(i, local_390[i]);
        out->local_b8[i] = q;
        const uint32_t c1 = l3_auth_carry32(acc_l390.lo, q.lo);
        acc_l390.lo += q.lo;
        acc_l390.hi = l3_auth_u32(acc_l390.hi + q.hi + c1);
        out->local_188_prefix[i] = acc_l390;
    }
}

/* ---- v116: post-local_390 convolution/range-correction block ----------- */

static l3_auth_pair32 l3_auth_pair32_sub(l3_auth_pair32 a, l3_auth_pair32 b) {
    l3_auth_pair32 out;
    const uint32_t borrow = (a.lo < b.lo);
    out.lo = a.lo - b.lo;
    out.hi = a.hi - b.hi - borrow;
    return out;
}

static l3_auth_pair32 l3_auth_pair32_zero(void) {
    l3_auth_pair32 z = {0u, 0u};
    return z;
}

static void l3_auth_pair32_mul_accum(l3_auth_pair32 a,
                                   l3_auth_pair32 b,
                                   l3_auth_pair32 *acc) {
    const uint64_t p = (uint64_t)a.lo * (uint64_t)b.lo;
    const uint32_t lo = (uint32_t)p;
    const uint32_t hi = (uint32_t)(p >> 32);
    const uint32_t carry = l3_auth_carry32(acc->lo, lo);
    acc->lo += lo;
    acc->hi = l3_auth_u32(acc->hi + a.hi * b.lo + a.lo * b.hi + hi + carry);
}

static l3_auth_pair32 l3_auth_prefix_range(const l3_auth_pair32 prefix[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
                                        unsigned start,
                                        unsigned end) {
    if (start > end || end >= L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS) {
        return l3_auth_pair32_zero();
    }
    l3_auth_pair32 out = prefix[end];
    if (start != 0u) {
        out = l3_auth_pair32_sub(out, prefix[start - 1u]);
    }
    return out;
}

void l3_authorization_finalizer_post_convolution_convolution_range_correct(
    const l3_authorization_finalizer_post_convolution_seed_state *seed,
    l3_authorization_finalizer_post_convolution_convolution_state *out) {
    if (!seed || !out) return;
    memset(out, 0, sizeof(*out));

    for (unsigned n = 0; n < L3_AUTH_FINAL_STATE_CONV_OUTPUT_PAIRS; ++n) {
        unsigned start;
        unsigned end;
        if (n < 12u || n == 12u) {
            start = 0u;
            end = n;
        } else {
            start = n - 12u;
            end = 12u;
        }

        l3_auth_pair32 product = l3_auth_pair32_zero();
        l3_auth_pair32 param_range = l3_auth_pair32_zero();
        l3_auth_pair32 b8_range = l3_auth_pair32_zero();
        uint32_t acc_lo = 0x05b63a49u;
        uint32_t acc_hi = 0x2d9cd77du;

        if (start <= end) {
            for (unsigned j = start; j <= end; ++j) {
                const unsigned bidx = n - j;
                l3_auth_pair32_mul_accum(seed->local_b8[bidx], seed->local_258[j], &product);
            }

            param_range = l3_auth_prefix_range(seed->auStack_128_prefix, start, end);
            b8_range = l3_auth_prefix_range(seed->local_188_prefix, n - end, n - start);

            const unsigned count = (end - start) + 1u;
            const uint32_t count_mul_lo = l3_auth_mullo_u32(count, 0xc6baf920u);
            const uint32_t count_mul_hi = l3_auth_mulhi_u32(count, 0xc6baf920u);
            const uint32_t prod_corr_lo = l3_auth_mullo_u32(product.lo, 0xe5039fedu);
            const uint32_t param_corr_lo = l3_auth_mullo_u32(param_range.lo, 0xf866e5a0u);
            const uint32_t b8_corr_lo = l3_auth_mullo_u32(b8_range.lo, 0x0e628989u);

            const uint32_t after_count = count_mul_lo + 0x05b63a49u;
            const uint32_t after_prod = after_count + prod_corr_lo;
            const uint32_t after_param = after_prod + param_corr_lo;
            acc_lo = after_param + b8_corr_lo;

            acc_hi = l3_auth_u32(
                count * 0x293d7978u + count_mul_hi + 0x2d9cd77du +
                l3_auth_carry32(count_mul_lo, 0x05b63a49u) +
                product.hi * 0xe5039fedu + product.lo * 0x16f12899u +
                l3_auth_mulhi_u32(product.lo, 0xe5039fedu) +
                l3_auth_carry32(after_count, prod_corr_lo) +
                param_range.hi * 0xf866e5a0u + param_range.lo * 0x061a2af2u +
                l3_auth_mulhi_u32(param_range.lo, 0xf866e5a0u) +
                l3_auth_carry32(after_prod, param_corr_lo) +
                b8_range.hi * 0x0e628989u + b8_range.lo * 0x877a0104u +
                l3_auth_mulhi_u32(b8_range.lo, 0x0e628989u) +
                l3_auth_carry32(after_param, b8_corr_lo));
        }

        const uint32_t out_mul = l3_auth_mullo_u32(acc_lo, 0x182da803u);
        out->local_328[n].lo = out_mul + 0x758fa3fcu;
        out->local_328[n].hi = l3_auth_u32(
            acc_hi * 0x182da803u + acc_lo * 0x77c21592u +
            l3_auth_mulhi_u32(acc_lo, 0x182da803u) + 0x2da4e762u +
            l3_auth_carry32(out_mul, 0x758fa3fcu));
        out->product_accum[n] = product;
        out->param3_range[n] = param_range;
        out->localb8_range[n] = b8_range;
    }
}

/* ---- v117: parent-context reseed after v116 convolution ----------------- */

static const uint32_t k_final_state_postconv_param1_mul[8] = {
    0xae384ddfu, 0xdf19be25u, 0x9213c441u, 0x38123e99u,
    0xe8b4a8f5u, 0xaae803b9u, 0x5262a931u, 0x47490ffdu
};

static const uint32_t k_final_state_postconv_param1_add[8] = {
    0xd5f06aedu, 0x3e466a1au, 0x0a553861u, 0xd7101d35u,
    0x262e54d8u, 0x68f6d468u, 0xce816d0au, 0x791fae7eu
};

void l3_authorization_finalizer_postconv_seed_param1_pairs(
    const uint32_t parent_words[L3_AUTH_FINAL_STATE_SEED_PAIR_COUNT],
    uint32_t out_words[L3_AUTH_FINAL_STATE_SEED_WORD_COUNT]) {
    if (!parent_words || !out_words) return;

    for (unsigned i = 0; i < L3_AUTH_FINAL_STATE_SEED_PAIR_COUNT; ++i) {
        const unsigned j = i & 7u;
        const uint32_t mixed = l3_auth_u32(
            l3_auth_u32(parent_words[i] * k_final_state_postconv_param1_mul[j] +
                      k_final_state_postconv_param1_add[j]) * 0x1f56adc3u +
            0x11e7efb3u);

        const uint32_t lo_seed_mul = l3_auth_mullo_u32(mixed, 0xb22b7065u);
        const uint32_t lo_seed = lo_seed_mul + 0x18394af1u;
        const uint32_t hi_seed = l3_auth_u32(
            mixed * 0xe8718513u + l3_auth_mulhi_u32(mixed, 0xb22b7065u) -
            0x333ef2b4u + l3_auth_carry32(lo_seed_mul, 0x18394af1u));

        const uint64_t folded = l3_authorization_finalizer_fold64(
            L3_AUTH_FOLD64_TABLE_35B0, ((uint64_t)hi_seed << 32) | lo_seed, 8u);
        const uint32_t f_lo = (uint32_t)folded;

        const uint32_t out_mul = l3_auth_mullo_u32(mixed, 0x928f7ed3u);
        out_words[i * 2u + 0u] = out_mul + 0x440b49f2u;
        out_words[i * 2u + 1u] = l3_auth_u32(
            mixed * 0xdc173f28u + l3_auth_mulhi_u32(mixed, 0x928f7ed3u) +
            f_lo * 0xc5d6ed29u + 0x7422a415u +
            l3_auth_carry32(out_mul, 0x440b49f2u));
    }
}



/* ---- v118: post-convolution carry/window loop coefficient head ---------- */

void l3_authorization_finalizer_postconv_loop_coeffs_from_head_trace(
    uint32_t parent_lo, uint32_t parent_hi,
    uint32_t current_lo, uint32_t current_hi,
    l3_authorization_finalizer_first_loop_coeffs *out,
    l3_authorization_finalizer_postconv_loop_coeff_trace *trace) {
    if (!out) return;
    memset(out, 0, sizeof(*out));
    if (trace) memset(trace, 0, sizeof(*trace));

    /* Native setup before the loop after v117:
         uVar52 = lo(parent_lo * 0xe7f0fb5d) + 0x23e743da;
         uVar53 = lo(parent_lo * 0x86253715) + 0x0c20727a;
         affine = uVar52 + uVar53 * current. */
    const uint32_t p_a_lo = l3_auth_mullo_u32(parent_lo, 0xe7f0fb5du);
    const uint32_t p_a_hi = l3_auth_mulhi_u32(parent_lo, 0xe7f0fb5du);
    const uint32_t p_b_lo = l3_auth_mullo_u32(parent_lo, 0x86253715u);
    const uint32_t p_b_hi = l3_auth_mulhi_u32(parent_lo, 0x86253715u);

    const uint32_t base_a_lo = p_a_lo + 0x23e743dau;
    const uint32_t base_b_lo = p_b_lo + 0x0c20727au;
    const uint32_t base_b_hi =
        parent_hi * 0x86253715u +
        parent_lo * 0x50427bd3u +
        p_b_hi +
        0xfd81d9d9u +
        l3_auth_carry32(p_b_lo, 0x0c20727au);

    const uint64_t cur_mul_b = (uint64_t)base_b_lo * (uint64_t)current_lo;
    const uint32_t cur_mul_b_lo = (uint32_t)cur_mul_b;
    const uint32_t cur_mul_b_hi = (uint32_t)(cur_mul_b >> 32);
    const uint32_t affine_lo = base_a_lo + cur_mul_b_lo;
    const uint32_t affine_hi =
        parent_hi * 0xe7f0fb5du +
        parent_lo * 0xf059ccccu +
        p_a_hi +
        0xc5ca701bu +
        l3_auth_carry32(p_a_lo, 0x23e743dau) +
        current_hi * base_b_lo +
        current_lo * base_b_hi +
        cur_mul_b_hi +
        l3_auth_carry32(base_a_lo, cur_mul_b_lo);

    const uint32_t fold_seed_mul_lo = l3_auth_mullo_u32(affine_lo, 0xd9954217u);
    const uint32_t fold_seed_mul_hi = l3_auth_mulhi_u32(affine_lo, 0xd9954217u);
    const uint32_t fold_seed_lo = fold_seed_mul_lo + 0x324e90c7u;
    const uint32_t fold_seed_hi =
        affine_hi * 0xd9954217u +
        affine_lo * 0x84545d77u +
        fold_seed_mul_hi +
        0xe659ceedu +
        l3_auth_carry32(fold_seed_mul_lo, 0x324e90c7u);

    const uint64_t folded = l3_authorization_finalizer_fold64(
        L3_AUTH_FOLD64_TABLE_3630, ((uint64_t)fold_seed_hi << 32) | fold_seed_lo, 7u);
    const uint32_t fold_lo = (uint32_t)folded;
    const uint32_t fold_hi = (uint32_t)(folded >> 32);

    const uint64_t fold_b_mul = (uint64_t)fold_lo * 0xf0000000ull;
    const uint32_t fold_b_lo = (uint32_t)fold_b_mul;
    const uint32_t fold_b_hi = (uint32_t)(fold_b_mul >> 32);
    const uint32_t affine_b_mul_lo = l3_auth_mullo_u32(affine_lo, 0x92686f77u);
    const uint32_t affine_b_mul_hi = l3_auth_mulhi_u32(affine_lo, 0x92686f77u);
    const uint32_t mix_sum_lo = affine_b_mul_lo + fold_b_lo;
    const uint32_t mix_lo = mix_sum_lo + 0x09e3e759u;
    const uint32_t mix_hi =
        affine_hi * 0x92686f77u +
        affine_lo * 0xa86e1782u +
        affine_b_mul_hi +
        fold_lo * 0x0d3c4865u +
        fold_b_hi +
        fold_hi * 0xf0000000u +
        l3_auth_carry32(affine_b_mul_lo, fold_b_lo) +
        0xc485335au +
        l3_auth_carry32(mix_sum_lo, 0x09e3e759u);

    const uint32_t coeff_mul_mul_lo = l3_auth_mullo_u32(mix_lo, 0x1a6147a3u);
    const uint32_t coeff_mul_mul_hi = l3_auth_mulhi_u32(mix_lo, 0x1a6147a3u);
    const uint32_t coeff_mul_lo = coeff_mul_mul_lo + 0xf5ecb9c5u;
    const uint32_t coeff_mul_hi =
        mix_hi * 0x1a6147a3u +
        mix_lo * 0x0699a6fau +
        coeff_mul_mul_hi +
        0x148631d6u +
        l3_auth_carry32(coeff_mul_mul_lo, 0xf5ecb9c5u);

    const uint32_t coeff_add_mul_lo = l3_auth_mullo_u32(mix_lo, 0x0e3b0950u);
    const uint32_t coeff_add_mul_hi = l3_auth_mulhi_u32(mix_lo, 0x0e3b0950u);
    const uint32_t coeff_add_lo = coeff_add_mul_lo + 0x130f0430u;
    const uint32_t coeff_add_hi =
        mix_hi * 0x0e3b0950u +
        mix_lo * 0xab4c990du +
        coeff_add_mul_hi +
        0xa8a14868u +
        l3_auth_carry32(coeff_add_mul_lo, 0x130f0430u);

    out->mul_lo = coeff_mul_lo;
    out->mul_hi = coeff_mul_hi;
    out->add_lo = coeff_add_lo;
    out->add_hi = coeff_add_hi;

    if (trace) {
        trace->fold3630_lo = fold_lo;
        trace->fold3630_hi = fold_hi;
        trace->mix_lo = mix_lo;
        trace->mix_hi = mix_hi;
        trace->coeff_mul_lo = coeff_mul_lo;
        trace->coeff_mul_hi = coeff_mul_hi;
        trace->coeff_add_lo = coeff_add_lo;
        trace->coeff_add_hi = coeff_add_hi;
    }
}

void l3_authorization_finalizer_postconv_loop_coeffs_from_head(
    uint32_t parent_lo, uint32_t parent_hi,
    uint32_t current_lo, uint32_t current_hi,
    l3_authorization_finalizer_first_loop_coeffs *out) {
    l3_authorization_finalizer_postconv_loop_coeffs_from_head_trace(
        parent_lo, parent_hi, current_lo, current_hi, out, 0);
}


/* ---- v119: post-convolution DAT_f41a36b0 / DAT_f41a3730 scalar tail ---- */

void l3_authorization_finalizer_postconv_loop_scalar_tail_from_updated_head(
    const uint32_t updated_head_pair[2],
    const uint32_t window_pair_after_vector_add[2],
    l3_authorization_finalizer_postconv_loop_scalar_tail_state *out) {
    if (!updated_head_pair || !window_pair_after_vector_add || !out) return;
    memset(out, 0, sizeof(*out));

    const uint32_t head_lo = updated_head_pair[0];
    const uint32_t head_hi = updated_head_pair[1];

    const uint32_t m36_lo = l3_auth_mullo_u32(head_lo, 0x66e00819u);
    const uint32_t m36_hi = l3_auth_mulhi_u32(head_lo, 0x66e00819u);
    const uint32_t x36_lo = m36_lo + 0xdbfcc92fu;
    const uint32_t x36_hi =
        head_hi * 0x66e00819u +
        head_lo * 0xccbd0d5bu +
        m36_hi +
        0x2f806e2bu +
        l3_auth_carry32(m36_lo, 0xdbfcc92fu);

    const uint64_t folded36 = l3_authorization_finalizer_fold64(
        L3_AUTH_FOLD64_TABLE_36B0, ((uint64_t)x36_hi << 32) | x36_lo, 7u);
    const uint32_t f36_lo = (uint32_t)folded36;
    const uint32_t f36_hi = (uint32_t)(folded36 >> 32);
    out->fold36b0_lo = f36_lo;
    out->fold36b0_hi = f36_hi;

    const uint32_t mid_mul_lo = l3_auth_mullo_u32(f36_lo, 0x4403aac3u);
    const uint32_t mid_mul_hi = l3_auth_mulhi_u32(f36_lo, 0x4403aac3u);
    const uint32_t mid_lo = mid_mul_lo + 0xde1f863au;
    const uint32_t mid_hi =
        f36_hi * 0x4403aac3u +
        f36_lo * 0x87a72f7fu +
        mid_mul_hi +
        0x61342c3eu +
        l3_auth_carry32(mid_mul_lo, 0xde1f863au);
    out->mid_lo = mid_lo;
    out->mid_hi = mid_hi;

    const uint32_t m3730_lo = l3_auth_mullo_u32(mid_lo, 0xa0613855u);
    const uint32_t m3730_hi = l3_auth_mulhi_u32(mid_lo, 0xa0613855u);
    const uint32_t x3730_lo = m3730_lo + 0x743a429cu;
    const uint32_t x3730_hi =
        mid_hi * 0xa0613855u +
        mid_lo * 0x3e19fa75u +
        m3730_hi +
        0x2a99813du +
        l3_auth_carry32(m3730_lo, 0x743a429cu);

    const uint64_t folded3730 = l3_authorization_finalizer_fold64(
        L3_AUTH_FOLD64_TABLE_3730, ((uint64_t)x3730_hi << 32) | x3730_lo, 7u);
    const uint32_t f3730_lo = (uint32_t)folded3730;
    const uint32_t f3730_hi = (uint32_t)(folded3730 >> 32);
    const l3_auth_pair64 *tab3730 = l3_auth_fold_table(L3_AUTH_FOLD64_TABLE_3730);
    const uint32_t tail_shift_lo = (f3730_lo >> 4) | (f3730_hi << 28);
    const uint32_t tail_lo = tail_shift_lo + tab3730[f3730_lo & 0x0fu].lo;
    const uint32_t tail_mix = (tail_lo >> 4) + tab3730[tail_lo & 0x0fu].lo;
    out->fold3730_tail_lo = tail_lo;
    out->fold3730_tail_mix = tail_mix;

    const uint64_t p860 = (uint64_t)mid_lo * 0x860e0a43ull;
    const uint32_t p860_lo = (uint32_t)p860;
    const uint32_t p860_hi = (uint32_t)(p860 >> 32);
    const uint64_t p879 = (uint64_t)p860_lo * 0x879d8421ull;
    const uint32_t p879_lo = (uint32_t)p879;
    const uint32_t p879_hi = (uint32_t)(p879 >> 32);
    const uint32_t sum0 = p879_lo + window_pair_after_vector_add[0];
    out->out_lo = sum0 + 0xfe5c5c4bu;
    out->out_hi =
        (mid_hi * 0x860e0a43u +
         mid_lo * 0x0e3f6b1cu +
         p860_hi +
         tail_mix * 0xcd0cfc90u) * 0x879d8421u +
        p860_lo * 0x5acdbbbbu +
        p879_hi +
        window_pair_after_vector_add[1] +
        l3_auth_carry32(p879_lo, window_pair_after_vector_add[0]) +
        0x03427a98u +
        l3_auth_carry32(sum0, 0xfe5c5c4bu);
}


/* ---- v120: post-convolution carry/window loop runner ------------------- */

void l3_authorization_finalizer_postconv_loop_full_step(
    const l3_authorization_finalizer_first_loop_front_state *front_in,
    const l3_authorization_finalizer_second_loop_front_sources *front_sources,
    const l3_authorization_finalizer_second_loop_late_state *late_in,
    const l3_authorization_finalizer_second_loop_late_sources *late_sources,
    const l3_authorization_finalizer_first_loop_coeffs *coeffs,
    l3_authorization_finalizer_postconv_loop_full_step_state *state_out) {
    if (!front_in || !front_sources || !late_in || !late_sources || !coeffs || !state_out) return;
    memset(state_out, 0, sizeof(*state_out));

    /* The post-convolution loop uses the same front-store topology as the
       v109/v110 second loop: local_328 gets source pair 0, uStack_320 gets
       pairs 1/2, local_310 gets pairs 3/4, uStack_300 gets pairs 5/6, and
       auStack_2f0 gets the scalar pair 7. */
    l3_authorization_finalizer_second_loop_front_update(front_in, front_sources, coeffs, &state_out->front);

    const uint32_t updated_head[2] = {
        state_out->front.local_328.lo,
        state_out->front.local_328.hi
    };
    const uint32_t window_after_vector[2] = {
        state_out->front.uStack_320.lane[0].lo,
        state_out->front.uStack_320.lane[0].hi
    };
    l3_authorization_finalizer_postconv_loop_scalar_tail_from_updated_head(updated_head,
                                                             window_after_vector,
                                                             &state_out->tail);

    /* Native updates auStack_2c8/2e8/2d8 before writing the scalar-tail result
       back over the low uStack_320 pair.  The late stores do not consume the
       tail result, so preserving that order documents the Ghidra sequence. */
    l3_authorization_finalizer_second_loop_late_update(late_in, late_sources, coeffs, &state_out->late);

    state_out->front.uStack_320.lane[0].lo = state_out->tail.out_lo;
    state_out->front.uStack_320.lane[0].hi = state_out->tail.out_hi;
}

void l3_authorization_finalizer_postconv_loop_run13_exact_sources(
    uint32_t parent_lo,
    uint32_t parent_hi,
    const l3_authorization_finalizer_pair_array13 *sources,
    const l3_authorization_finalizer_first_loop_arrays *state_in,
    l3_authorization_finalizer_postconv_loop_run13_state *state_out) {
    if (!sources || !state_in || !state_out) return;
    memset(state_out, 0, sizeof(*state_out));
    l3_auth_pair32 physical[L3_AUTH_FINAL_STATE_SEED_FULL_PAIR_COUNT];
    l3_authorization_finalizer_arrays_to_physical(state_in, physical);

    const l3_authorization_finalizer_second_loop_front_sources front_sources = {
        sources->pair[0],
        {{ sources->pair[1], sources->pair[2] }},
        {{ sources->pair[3], sources->pair[4] }},
        {{ sources->pair[5], sources->pair[6] }},
        sources->pair[7]
    };
    const l3_authorization_finalizer_second_loop_late_sources late_sources = {
        sources->pair[12],
        {{ sources->pair[8], sources->pair[9] }},
        {{ sources->pair[10], sources->pair[11] }}
    };

    l3_auth_pair32 loop_current = physical[0];

    for (unsigned i = 0; i < L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS; ++i) {
        l3_authorization_finalizer_first_loop_coeffs coeffs;
        l3_authorization_finalizer_postconv_loop_coeffs_from_head(
            parent_lo, parent_hi, loop_current.lo, loop_current.hi, &coeffs);

        l3_authorization_finalizer_first_loop_front_state front_in;
        memset(&front_in, 0, sizeof(front_in));
        front_in.local_328 = loop_current;
        front_in.auStack_2f0 = physical[i + 7u];
        front_in.uStack_320 = l3_auth_q_from_pair_window(&physical[i + 1u]);
        front_in.local_310 = l3_auth_q_from_pair_window(&physical[i + 3u]);
        front_in.uStack_300 = l3_auth_q_from_pair_window(&physical[i + 5u]);

        l3_authorization_finalizer_second_loop_late_state late_in;
        memset(&late_in, 0, sizeof(late_in));
        late_in.auStack_2c8 = physical[i + 12u];
        late_in.auStack_2e8 = l3_auth_q_from_pair_window(&physical[i + 8u]);
        late_in.auStack_2d8 = l3_auth_q_from_pair_window(&physical[i + 10u]);

        l3_authorization_finalizer_postconv_loop_full_step_state step;
        l3_authorization_finalizer_postconv_loop_full_step(&front_in, &front_sources,
                                              &late_in, &late_sources,
                                              &coeffs, &step);

        physical[i] = step.front.local_328;
        physical[i + 7u] = step.front.auStack_2f0;
        l3_auth_q_to_pair_window(&step.front.uStack_320, &physical[i + 1u]);
        l3_auth_q_to_pair_window(&step.front.local_310, &physical[i + 3u]);
        l3_auth_q_to_pair_window(&step.front.uStack_300, &physical[i + 5u]);
        physical[i + 12u] = step.late.auStack_2c8;
        l3_auth_q_to_pair_window(&step.late.auStack_2e8, &physical[i + 8u]);
        l3_auth_q_to_pair_window(&step.late.auStack_2d8, &physical[i + 10u]);

        state_out->last_tail = step.tail;
        loop_current.lo = step.tail.out_lo;
        loop_current.hi = step.tail.out_hi;
    }

    l3_authorization_finalizer_arrays_from_physical(physical, &state_out->arrays);
    state_out->final_loop_carry = loop_current;
}


/* ---- v121: DAT_f41a37b0 reduction after post-convolution loop ---------- */

static const uint32_t k_final_state_reduce37b0_mul[8] = {
    0xeb839037u, 0x73b04d9bu, 0x71c11191u, 0x2e018a13u,
    0x8d34bb7bu, 0x4df6ab39u, 0x2a3e07f9u, 0x1594087du
};

static const uint32_t k_final_state_reduce37b0_add[8] = {
    0x1baf578au, 0x2832a596u, 0xf6fb1c92u, 0xe09427b5u,
    0x458cfa6bu, 0x6542b407u, 0xf1edc2fau, 0xf490414fu
};

static void l3_authorization_finalizer_fold_post_convolution_step(uint32_t lo,
                                       uint32_t hi,
                                       uint32_t *out_lo,
                                       uint32_t *out_hi) {
    const unsigned idx = lo & 0x0fu;
    const uint32_t shifted_lo = (lo >> 4) | (hi << 28);
    const uint32_t shifted_hi = hi >> 4;
    const uint32_t new_lo = shifted_lo + k_final_state_fold_37b0[idx].lo;
    const uint32_t new_hi = shifted_hi + k_final_state_fold_37b0[idx].hi +
                            l3_auth_carry32(shifted_lo, k_final_state_fold_37b0[idx].lo);
    *out_lo = new_lo;
    *out_hi = new_hi;
}

static uint32_t l3_authorization_finalizer_fold_post_convolution_low_word(uint32_t lo, uint32_t hi) {
    const unsigned idx = lo & 0x0fu;
    const uint32_t shifted_lo = (lo >> 4) | (hi << 28);
    return shifted_lo + k_final_state_fold_37b0[idx].lo;
}

void l3_authorization_finalizer_reduce_workspace_post_convolution(
    const l3_auth_pair32 auStack_2c8_window[L3_AUTH_FINAL_STATE_FIRST_LOOP_WINDOW_PAIRS],
    l3_authorization_finalizer_reduce37b0_state *out) {
    if (!auStack_2c8_window || !out) return;
    memset(out, 0, sizeof(*out));

    uint32_t state_hi = 0x470edd75u;
    uint32_t state_lo = 0xa82b506bu;

    for (unsigned i = 0; i < L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS; ++i) {
        const uint32_t in_lo = auStack_2c8_window[i + 1u].lo;
        const uint32_t in_hi = auStack_2c8_window[i + 1u].hi;

        const uint32_t prod_in_lo = l3_auth_mullo_u32(in_lo, 0x213e4925u);
        const uint32_t prod_in_hi = l3_auth_mulhi_u32(in_lo, 0x213e4925u);
        const uint32_t prod_state_lo = l3_auth_mullo_u32(state_lo, 0x20cd730fu);
        const uint32_t prod_state_hi = l3_auth_mulhi_u32(state_lo, 0x20cd730fu);

        const uint32_t sum_lo = prod_state_lo + prod_in_lo;
        const uint32_t pre_lo = sum_lo + 0x780704b9u;
        const uint32_t pre_mul_lo = l3_auth_mullo_u32(pre_lo, 0x5e71b1bbu);
        const uint32_t fold_seed_lo = pre_mul_lo + 0xd63f80ccu;

        uint32_t fold_seed_hi =
            state_hi * 0x20cd730fu +
            state_lo * 0x6d985cfcu +
            prod_state_hi +
            in_hi * 0x213e4925u +
            in_lo * 0x869d927au +
            prod_in_hi +
            l3_auth_carry32(prod_state_lo, prod_in_lo) +
            0x10e0b50cu +
            l3_auth_carry32(sum_lo, 0x780704b9u);
        fold_seed_hi =
            fold_seed_hi * 0x5e71b1bbu +
            pre_lo * 0x8462938cu +
            l3_auth_mulhi_u32(pre_lo, 0x5e71b1bbu) +
            0xb106017au +
            l3_auth_carry32(pre_mul_lo, 0xd63f80ccu);

        uint32_t lo = fold_seed_lo;
        uint32_t hi = fold_seed_hi;
        for (unsigned r = 0; r < 7u; ++r) {
            l3_authorization_finalizer_fold_post_convolution_step(lo, hi, &lo, &hi);
        }

        const uint32_t fold7_lo = lo;
        const uint32_t fold7_hi = hi;
        const uint32_t fold7_mul_lo = l3_auth_mullo_u32(fold7_lo, 0xc9c0715du);
        const uint32_t fold7_mul_hi = l3_auth_mulhi_u32(fold7_lo, 0xc9c0715du);

        for (unsigned r = 0; r < 7u; ++r) {
            l3_authorization_finalizer_fold_post_convolution_step(lo, hi, &lo, &hi);
        }
        const uint32_t partial_low = l3_authorization_finalizer_fold_post_convolution_low_word(lo, hi);
        const uint32_t partial_low2 = (partial_low >> 4) +
            k_final_state_fold_37b0[partial_low & 0x0fu].lo;

        out->local_3c4[i] =
            (pre_lo * 0xf0b9db91u + fold7_lo * 0xd0000000u + 0xfea03ab2u) *
            k_final_state_reduce37b0_mul[i & 7u] +
            k_final_state_reduce37b0_add[i & 7u];

        state_lo = fold7_mul_lo + 0x3b6afb70u;
        state_hi =
            fold7_hi * 0xc9c0715du +
            fold7_lo * 0x9de86c8eu +
            fold7_mul_hi +
            partial_low2 * 0x63f8ea30u +
            0xb9b18f1eu +
            l3_auth_carry32(fold7_mul_lo, 0x3b6afb70u);
    }

    out->final_state.lo = state_lo;
    out->final_state.hi = state_hi;
}



/* ---- v122: post-reduction vector-affine setup ------------------------- */

static void l3_auth_vmla_u32x4(const uint32_t in[4],
                             const uint32_t k[4],
                             const uint32_t add[4],
                             uint32_t out[4]) {
    for (unsigned i = 0; i < 4u; ++i) out[i] = add[i] + in[i] * k[i];
}

void l3_authorization_finalizer_vector_affine_setup(
    const uint32_t local_35c[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_3c4[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_vector_affine_setup_state *out) {
    if (!local_35c || !local_3c4 || !out) return;
    memset(out, 0, sizeof(*out));

    /* Native vldr pairs at DAT_f41a3830/3838 and DAT_f41a3840/3848. */
    static const uint32_t k_local_258_seed[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS] = {
        0x2e2a2f3du, 0x12cb5fcdu, 0x029b2656u, 0x410a049bu,
        0x773e7ea7u, 0x8a590b6au, 0x0e3828bbu, 0x7b1ce3feu,
        0x2e2a2f3du, 0x12cb5fcdu, 0x029b2656u, 0x410a049bu,
        0x773e7ea7u
    };
    memcpy(out->local_258_seed, k_local_258_seed, sizeof(k_local_258_seed));

    static const uint32_t k35c_or_33c[4] = {
        0x93b14e93u, 0x8d542acbu, 0xd6ed8401u, 0xf53e61fdu
    };
    static const uint32_t k34c[4] = {
        0x94080255u, 0x8d234485u, 0x7c6b0dcfu, 0xfb8c1cabu
    };
    static const uint32_t k3c4_or_3a4[4] = {
        0xb0c58f8bu, 0x76134171u, 0x0b423481u, 0x40fabc03u
    };
    static const uint32_t k3b4[4] = {
        0x6bcfb1d1u, 0x2a19a9f7u, 0x539ff9d3u, 0x80f21f55u
    };
    static const uint32_t add35c_or_33c[4] = {
        0x0452bcb1u, 0x57012f4fu, 0x186dd32cu, 0x9714918fu
    };
    static const uint32_t add34c[4] = {
        0xee4e9fd3u, 0xdf91063fu, 0x4540332cu, 0xddc46a16u
    };
    static const uint32_t add3c4_or_3a4[4] = {
        0xf86464a5u, 0xcce9781cu, 0x777bd2f4u, 0x9c15a6deu
    };
    static const uint32_t add3b4[4] = {
        0x89731600u, 0xd1a6a82eu, 0x10a7eae1u, 0x1bf5c637u
    };

    l3_auth_vmla_u32x4(&local_35c[0], k35c_or_33c,
                     add35c_or_33c, out->local_188_head);
    l3_auth_vmla_u32x4(&local_35c[4], k34c,
                     add34c, out->local_34c_affine);
    l3_auth_vmla_u32x4(&local_35c[8], k35c_or_33c,
                     add35c_or_33c, out->local_33c_affine);

    l3_auth_vmla_u32x4(&local_3c4[0], k3c4_or_3a4,
                     add3c4_or_3a4, out->local_3c4_affine);
    l3_auth_vmla_u32x4(&local_3c4[4], k3b4,
                     add3b4, out->local_3b4_affine);
    l3_auth_vmla_u32x4(&local_3c4[8], k3c4_or_3a4,
                     add3c4_or_3a4, out->local_3a4_affine);

    out->local_158 = local_35c[12] * 0x94080255u + 0xee4e9fd3u;
    out->local_18c = local_3c4[12] * 0x6bcfb1d1u + 0x89731600u;
}


/* ---- v123: parent-context DAT_f41a3898 auStack_128 seeding ------------ */

static const uint32_t k_final_state_fold_3898[16] = {
    0xe9d25408u, 0xd11533eeu, 0xc0e3caebu, 0xb826aad1u,
    0xa7f541ceu, 0x9f3821b4u, 0x867b019au, 0x76499897u,
    0x6d8c787du, 0x5d5b0f7au, 0x449def60u, 0x346c865du,
    0x2baf6643u, 0x1b7dfd40u, 0x02c0dd26u, 0xfa03bd0cu
};

static const uint32_t k_final_state_parent_parity_75c[2] = {
    0xf16b491du, 0x69f8cdffu
};

static const uint32_t k_final_state_parent_1a10[8] = {
    0xae9b34fcu, 0x509ea3abu, 0xf2a21258u, 0x94a58107u,
    0x36a8efb4u, 0xd8ac5e63u, 0x6a9457a1u, 0x0c97c64eu
};

static const uint32_t k_final_state_parent_1a30[8] = {
    0x792effe5u, 0xf1be4f07u, 0x26fa7d7bu, 0x73e18bc3u,
    0xcccf74f7u, 0x9ce6ca23u, 0x88eb98cbu, 0x0c31c7cfu
};

static const uint32_t k_final_state_parent_1a50[8] = {
    0xd90c4f2fu, 0xb4bd2f0eu, 0xb105caebu, 0xd30d19c7u,
    0xbe3fe3d8u, 0x3c7e3208u, 0x5124345eu, 0x347d6c1au
};

static const uint32_t k_final_state_parent_1a70[8] = {
    0xd2fd4103u, 0x317c3837u, 0xb32417abu, 0x08f7ca71u,
    0x2899e9c9u, 0x76d549bbu, 0xc891de13u, 0xacb8addfu
};

static const uint32_t k_final_state_parent_1a90[8] = {
    0xef0759c8u, 0x7fc9b706u, 0xabdce3c3u, 0xa76cd4b3u,
    0x08ac257cu, 0xb091da35u, 0xbaeaa0efu, 0xb6c8b46cu
};

uint32_t l3_authorization_finalizer_reduce_vector_stage01(uint32_t value, unsigned rounds) {
    for (unsigned i = 0; i < rounds; ++i) {
        value = k_final_state_fold_3898[value & 0x0fu] + (value >> 4);
    }
    return value;
}

void l3_authorization_finalizer_parent_context_stage01_seed(
    const uint32_t parent_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_parent_context_stage01_state *out) {
    if (!parent_words || !out) return;
    memset(out, 0, sizeof(*out));

    /* Native first-word prologue, using SUB164(*param_1,0). */
    const uint32_t first_mix = parent_words[0] * 0x792effe5u + 0xd90c4f2fu;
    uint32_t driver = (first_mix >> 1) * 0xcc4a0aedu +
                      k_final_state_parent_parity_75c[first_mix & 1u];
    uint32_t red = l3_authorization_finalizer_reduce_vector_stage01(driver * 0x961141c1u + 0x22bdd73bu, 7u);
    driver = driver * 0xe0c15715u +
             (k_final_state_parent_1a10[red & 7u] - (red >> 3)) * 0x80000000u +
             0x8516d849u;

    out->first_affine = first_mix;
    out->first_reduced = red;
    out->initial_driver = driver;

    for (unsigned lane = 0; lane < L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS; ++lane) {
        if (lane == L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS - 1u) {
            driver = driver * 0x76779e83u + 0x2af67365u;
            red = 0;
        } else {
            const unsigned j = (lane + 1u) & 7u;
            const uint32_t lane_mix = parent_words[lane + 1u] * k_final_state_parent_1a30[j] +
                                      k_final_state_parent_1a50[j];
            const uint32_t lane_affine = (lane_mix >> 1) * 0xcc4a0aedu +
                                         k_final_state_parent_parity_75c[lane_mix & 1u];
            red = l3_authorization_finalizer_reduce_vector_stage01(lane_affine * 0x961141c1u + 0x22bdd73bu, 7u);
            const uint32_t next_driver_base = lane_affine * 0xe0c15715u + 0x8516d849u;
            const uint32_t next_driver = next_driver_base +
                ((red >> 3) * 0x4499u + k_final_state_parent_1a10[red & 7u]) * 0x80000000u;
            driver = driver * 0x76779e83u +
                     (next_driver_base * 0x12e6u + lane_mix * 0x043du) * 0x78000000u +
                     0x52f67365u;
            out->lane_input_affine[lane] = lane_affine;
            out->lane_reduced[lane] = red;
            if (lane + 1u < L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS) {
                /* Native carries this value into the next loop body unless the
                   next lane is the special terminal lane 12. */
                out->final_driver = next_driver;
            }
            /* Use a local temporary so the terminal branch below can reuse the
               exact native state-transition order. */
            if (lane != L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS - 2u) {
                out->auStack_128_words[lane] =
                    k_final_state_parent_1a70[lane & 7u] * driver +
                    k_final_state_parent_1a90[lane & 7u];
                driver = next_driver;
                continue;
            }
            out->auStack_128_words[lane] =
                k_final_state_parent_1a70[lane & 7u] * driver +
                k_final_state_parent_1a90[lane & 7u];
            driver = next_driver;
            continue;
        }

        out->lane_input_affine[lane] = 0;
        out->lane_reduced[lane] = red;
        out->auStack_128_words[lane] =
            k_final_state_parent_1a70[lane & 7u] * driver +
            k_final_state_parent_1a90[lane & 7u];
    }

    out->final_driver = driver;
}


/* ---- v124: DAT_f41a38d8 local_b8 seeding ------------------------------ */

static const uint32_t k_final_state_fold_38d8[16] = {
    0xa65b4f29u, 0xf25ffb5eu, 0x47ef4d50u, 0x93f3f985u,
    0xeff8a5bau, 0x3587f7acu, 0x818ca3e1u, 0xdd915016u,
    0x2320a208u, 0x7f254e3du, 0xcb29fa72u, 0x10b94c64u,
    0x6cbdf899u, 0xb8c2a4ceu, 0x0e51f6c0u, 0x5a56a2f5u
};

static const uint32_t k_final_state_local_b8_mul_1ab0[8] = {
    0x60e5139bu, 0x8e87f4b7u, 0xee8642e9u, 0xcd887867u,
    0x6a5b9ecdu, 0x1309345du, 0xa7fc65d3u, 0x194553b5u
};

static const uint32_t k_final_state_local_b8_add_1ad0[8] = {
    0xa9571f38u, 0xcc0a1424u, 0x568fad72u, 0x2a3eee34u,
    0xe0f87c9fu, 0x69b94f67u, 0xe450c14eu, 0xf323070au
};

uint32_t l3_authorization_finalizer_reduce_vector_stage02(uint32_t value, unsigned rounds) {
    for (unsigned i = 0; i < rounds; ++i) {
        value = k_final_state_fold_38d8[value & 0x0fu] + (value >> 4);
    }
    return value;
}

static void l3_authorization_finalizer_make_reduction_state_from_reduced_words(
    const uint32_t au[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    uint32_t out[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS]) {
    static const uint32_t mul_a[4] = {
        0xeb02a53du, 0x48fd2f81u, 0xc8550325u, 0x98f2ada7u
    };
    static const uint32_t add_a0[4] = {
        0xb95802fdu, 0x94f52190u, 0x2dcb4167u, 0x7630bdd1u
    };
    static const uint32_t add_a2[4] = {
        0x95f428eeu, 0x94f52190u, 0x2dcb4167u, 0x7630bdd1u
    };
    static const uint32_t mul_b[4] = {
        0x06cfe27fu, 0x92246b35u, 0x56d3fb4du, 0x2298ec29u
    };
    static const uint32_t add_b[4] = {
        0x15606512u, 0x2548219du, 0x5fc8b8b3u, 0xce8a004au
    };

    for (unsigned i = 0; i < 4u; ++i) {
        out[i] = au[i] * mul_a[i] + add_a0[i];
        out[4u + i] = au[4u + i] * mul_b[i] + add_b[i];
        out[8u + i] = au[8u + i] * mul_a[i] + add_a2[i];
    }
    out[12] = au[12] * 0x06cfe27fu + 0x15606512u;
}

void l3_authorization_finalizer_seed_state_stage02_seed(
    const uint32_t auStack_128_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_seed_state_stage02_state *out) {
    if (!auStack_128_words || !out) return;
    memset(out, 0, sizeof(*out));

    l3_authorization_finalizer_make_reduction_state_from_reduced_words(
        auStack_128_words, out->local_328_stream);

    uint32_t next_stream_word = out->local_328_stream[0];
    uint32_t driver = 0x087e0c41u;
    for (unsigned lane = 0; lane < L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS; ++lane) {
        const uint32_t cur = driver * 0x4b5e8c61u + next_stream_word;
        const uint32_t reduced = l3_authorization_finalizer_reduce_vector_stage02(
            cur * 0x8c9a3f73u + 0x23183003u, 7u);

        out->driver38d8[lane] = cur;
        out->reduced38d8[lane] = reduced;
        out->local_b8_words[lane] =
            (cur * 0x60721597u + reduced * 0x30000000u + 0xd36623c2u) *
            k_final_state_local_b8_mul_1ab0[lane & 7u] +
            k_final_state_local_b8_add_1ad0[lane & 7u];

        if (lane == L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS - 1u) {
            driver = cur;
            break;
        }

        const uint32_t reduced_next = l3_authorization_finalizer_reduce_vector_stage02(reduced, 1u);
        next_stream_word = out->local_328_stream[lane + 1u];
        driver = reduced * 0xb72f5f9bu +
                 reduced_next * 0x8d0a0650u +
                 0xa1cd524fu;
    }
    out->final_driver = driver;
}

void l3_authorization_finalizer_live_context_derive(
    const uint32_t parent_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_b8_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t initial_local_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_live_context *out) {
    static const uint32_t k39_q13_mul[4] = {
        0x5069712fu, 0x4cdeacffu, 0xc80273f1u, 0x341afdf7u
    };
    static const uint32_t k39_q11_mul[4] = {
        0x7c3c1799u, 0x72de9b75u, 0xa9f37babu, 0x9b903c65u
    };
    static const uint32_t k39_const8[4] = {
        0x0de239f5u, 0x1f382270u, 0xf35f05afu, 0x25e1d6b8u
    };
    static const uint32_t k39_q9_const[4] = {
        0x5e83f1f9u, 0xf3ea8608u, 0x168b6d45u, 0xcd95a78bu
    };
    static const uint32_t k3a_q13_mul[4] = {
        0xeb296373u, 0x1ec70003u, 0xbe8f0d2du, 0x55cb451bu
    };
    static const uint32_t k3a_q11_mul[4] = {
        0x90b88a35u, 0x8f6dfaa1u, 0x65b9dfffu, 0x7dd287d1u
    };
    static const uint32_t k3a_const8[4] = {
        0x74abc977u, 0xe47b5d79u, 0x3149e1fau, 0xf703d3cau
    };
    static const uint32_t k3a_q9_const[4] = {
        0xf2957564u, 0x9b04b105u, 0xd5e0f767u, 0xe5d3909bu
    };
    static const uint32_t k3b_q_mul[4] = {
        0x482b8e33u, 0x15549e31u, 0xdf8e6d8bu, 0xda58edafu
    };
    static const uint32_t k3b_q_add[4] = {
        0xe5ac9da5u, 0xef578883u, 0x99b9e3a3u, 0x2de46fd7u
    };
    static const uint32_t k3c_q5_mul[4] = {
        0x6cb79987u, 0x1aa9d2cdu, 0xd7f1847fu, 0xa4dc9d93u
    };
    static const uint32_t k3c_q5_add[4] = {
        0xcd700927u, 0x40416a9bu, 0xfcc58104u, 0xe4cd530cu
    };

    if (!parent_words || !local_b8_words || !initial_local_188_words || !out) return;
    memset(out, 0, sizeof(*out));

    for (unsigned i = 0; i < 4u; ++i) {
        /* Register mapping at the 39d8/3ad8 stores is q9->lanes 0..3,
           q8->lanes 4..7, q10->lanes 8..11. */
        out->add_39d8_a[i] =
            local_b8_words[i] * k39_q11_mul[i] + k39_q9_const[i];
        out->add_39d8_b[i] =
            local_b8_words[4u + i] * k39_q13_mul[i] + k39_const8[i];
        out->add_39d8_c[i] =
            local_b8_words[8u + i] * k39_q11_mul[i] + k39_q9_const[i];

        out->add_3ad8_a[i] =
            local_b8_words[i] * k3a_q11_mul[i] + k3a_q9_const[i];
        out->add_3ad8_b[i] =
            local_b8_words[4u + i] * k3a_q13_mul[i] + k3a_const8[i];
        out->add_3ad8_c[i] =
            local_b8_words[8u + i] * k3a_q11_mul[i] + k3a_q9_const[i];

        out->late_parent_q_3b58_words[i] =
            parent_words[7u + i] * k3b_q_mul[i] + k3b_q_add[i];
        out->late_parent_q5_3c58_words[i] =
            parent_words[7u + i] * k3c_q5_mul[i] + k3c_q5_add[i];
    }

    out->tail_accum_39d8 = local_b8_words[12] * 0x5069712fu;
    out->tail_accum_3ad8 = local_b8_words[12] * 0xeb296373u;
    out->preserved_tail12_3b58 =
        initial_local_188_words[12] * 0x4882153bu +
        parent_words[12] * 0x9ff0b5bbu + 0xcebed81fu;
}

/* ---- v125: first predicate gate and DAT_f41a3918 local_1bc refresh ----- */

static const uint32_t k_final_state_pred_gate_3864[13] = {
    0x2ec21464u, 0x38f0b36cu, 0x748d7bf8u, 0xd8734b04u,
    0x907a0ed7u, 0xe3ac34c8u, 0x701b6a8cu, 0xc5354f1du,
    0x2ec21464u, 0x38f0b36cu, 0x748d7bf8u, 0xd8734b04u,
    0x907a0ed7u
};

static const uint32_t k_final_state_pred_gate_mul_1af0[8] = {
    0xbc84433du, 0xb817ef43u, 0x4d0a6f55u, 0x021a886bu,
    0xb549c957u, 0xb1884cadu, 0x17b3980bu, 0x332bb099u
};

static const uint32_t k_final_state_pred_gate_mul_1b10[8] = {
    0x7c7dc0b7u, 0x2b4f05d3u, 0xdda398b5u, 0xabbf0d93u,
    0xe5702607u, 0x7898c69bu, 0x8c62267fu, 0xa58c836du
};

static const uint32_t k_final_state_pred_gate_add_1b30[8] = {
    0x5dda2b4eu, 0x25543099u, 0xb4d8cdf9u, 0x34c60d46u,
    0x3dee0364u, 0x56e21367u, 0x4d5d75a6u, 0xfaef13cfu
};

static const uint32_t k_final_state_pred_parity_764[2] = {
    0xe8c287a4u, 0x26837fdfu
};

static const uint32_t k_final_state_fold_3918[16] = {
    0xdeb9421bu, 0xada39630u, 0x7f0897ccu, 0x4df2ebe1u,
    0x1f57ed7du, 0xee424192u, 0xbfa7432eu, 0x8e919743u,
    0x5ff698dfu, 0x2ee0ecf4u, 0xfdcb4109u, 0xcf3042a5u,
    0x9e1a96bau, 0x6f7f9856u, 0x3e69ec6bu, 0x0fceee07u
};

static const uint32_t k_final_state_pred_1b50[8] = {
    0x5f066456u, 0x0959bd8cu, 0x1596f9f3u, 0x21d43658u,
    0x2e1172bfu, 0x3a4eaf24u, 0x468beb8bu, 0x52c927f0u
};

static const uint32_t k_final_state_pred_1b70[8] = {
    0x1010db97u, 0x6a2438d1u, 0x5c3996abu, 0x3334ca33u,
    0x9c1e1ed1u, 0xab782da5u, 0xc8724661u, 0xb5cf71dbu
};

static const uint32_t k_final_state_pred_1b90[8] = {
    0xe391bf25u, 0x79e41ce8u, 0xd3918758u, 0xaa6f6573u,
    0xbe8eee8fu, 0xfc78eacfu, 0x3068884bu, 0xd8cc2f54u
};

static const uint32_t k_final_state_pred_1bb0[8] = {
    0xb3d48187u, 0xb5fcb1d1u, 0x219adee3u, 0x01446edbu,
    0x6990cbd1u, 0x78a6354du, 0xa7c72541u, 0xcad42f33u
};

static const uint32_t k_final_state_pred_1bd0[8] = {
    0x955df177u, 0xcfa0f10eu, 0x8013edfau, 0x220ebf91u,
    0xfca16617u, 0xeea32bbbu, 0x327096ebu, 0x8b6cc2a6u
};

uint32_t l3_authorization_finalizer_reduce_vector_stage03(uint32_t value, unsigned rounds) {
    for (unsigned i = 0; i < rounds; ++i) {
        value = k_final_state_fold_3918[value & 0x0fu] + (value >> 4);
    }
    return value;
}

void l3_authorization_finalizer_scan_reduction_predicate(
    const uint32_t local_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_predicate_terminal_gate_state *out) {
    if (!local_1f0_words || !out) return;
    memset(out, 0, sizeof(*out));
    out->first_mismatch_lane = L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS;
    out->all_match = 1u;
    for (unsigned lane = 0; lane < L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS; ++lane) {
        const unsigned j = lane & 7u;
        const uint32_t value = local_1f0_words[lane] * k_final_state_pred_gate_mul_1b10[j] +
                               k_final_state_pred_gate_3864[lane] * k_final_state_pred_gate_mul_1af0[j] +
                               k_final_state_pred_gate_add_1b30[j];
        out->lane_value[lane] = value;
        if (value != 0x72e328f5u && out->all_match) {
            out->all_match = 0u;
            out->first_mismatch_lane = lane;
            out->first_mismatch_value = value;
            break;
        }
    }
}

static uint32_t l3_authorization_finalizer_predicate_stage03_lane_driver(uint32_t word,
                                                   uint32_t *affine_out,
                                                   uint32_t *reduced_out) {
    const uint32_t affine = (word >> 1) * 0xa7236b17u +
                            k_final_state_pred_parity_764[word & 1u];
    const uint32_t reduced = l3_authorization_finalizer_reduce_vector_stage03(
        affine * 0x45579f27u + 0x742df06bu, 7u);
    if (affine_out) *affine_out = affine;
    if (reduced_out) *reduced_out = reduced;
    return affine * 0x71b07581u + 0x15c53736u +
           ((reduced >> 3) * 0xa6dbu + k_final_state_pred_1b50[reduced & 7u]) * 0x80000000u;
}

void l3_authorization_finalizer_lane_state_a_even_stage03_refresh(
    const uint32_t local_1bc_in[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_lane_state_a_stage03_state *out) {
    if (!local_1bc_in || !out) return;
    memset(out, 0, sizeof(*out));
    memcpy(out->input_words, local_1bc_in, sizeof(out->input_words));
    out->branch_applicable = ((local_1bc_in[0] & 1u) == 0u) ? 1u : 0u;
    if (!out->branch_applicable) {
        memcpy(out->output_words, local_1bc_in, sizeof(out->output_words));
        return;
    }

    uint32_t affine = 0;
    uint32_t reduced = 0;
    uint32_t driver = l3_authorization_finalizer_predicate_stage03_lane_driver(
        local_1bc_in[0] * 0x1010db97u + 0xe391bf25u,
        &affine, &reduced);
    out->lane_affine[0] = affine;
    out->lane_reduced[0] = reduced;
    out->initial_driver = driver;

    for (unsigned lane = 0; lane < L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS - 1u; ++lane) {
        const unsigned next_lane = lane + 1u;
        const unsigned j = next_lane & 7u;
        const uint32_t next_mix = local_1bc_in[next_lane] * k_final_state_pred_1b70[j] +
                                  k_final_state_pred_1b90[j];
        uint32_t next_affine = 0;
        uint32_t next_reduced = 0;
        const uint32_t next_driver = l3_authorization_finalizer_predicate_stage03_lane_driver(
            next_mix, &next_affine, &next_reduced);
        const uint32_t next_base = next_affine * 0x71b07581u + 0x15c53736u;
        const uint32_t write_driver = driver * 0xc72224c7u +
            (next_base * 0x2cceu - next_mix) * 0xf8000000u + 0x28000000u;

        out->lane_mix[next_lane] = next_mix;
        out->lane_affine[next_lane] = next_affine;
        out->lane_reduced[next_lane] = next_reduced;
        out->write_driver[lane] = write_driver;
        out->output_words[lane] = k_final_state_pred_1bb0[lane & 7u] *
                                  (write_driver + 0x6ba84f05u) +
                                  k_final_state_pred_1bd0[lane & 7u];

        if (lane == L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS - 2u) {
            const uint32_t terminal_driver = next_driver * 0xc72224c7u;
            const unsigned term = L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS - 1u;
            out->write_driver[term] = terminal_driver;
            out->output_words[term] = k_final_state_pred_1bb0[term & 7u] *
                                      (terminal_driver + 0x6ba84f05u) +
                                      k_final_state_pred_1bd0[term & 7u];
            out->final_driver = terminal_driver;
        } else {
            driver = next_driver;
        }
    }
}

/* ---- v126: local_188 odd/even branch reducers, DAT_f41a3958/3998 ---- */

static const uint32_t k_final_state_fold_3958[16] = {
    0x7eee0848u, 0xccdc8aebu, 0x139eaca1u, 0x6a60ce57u,
    0xb84f50fau, 0x0f1172b0u, 0x55d39466u, 0xa3c21709u,
    0xfa8438bfu, 0x41465a75u, 0x9f34dd18u, 0xe5f6feceu,
    0x3cb92084u, 0x8aa7a327u, 0xd169c4ddu, 0x282be693u
};

static const uint32_t k_final_state_fold_3998[16] = {
    0xaa7fd773u, 0xb66c528du, 0xcdf321feu, 0xd9df9d18u,
    0xe1666c89u, 0xfd52e7a3u, 0x04d9b714u, 0x10c6322eu,
    0x2cb2ad48u, 0x34397cb9u, 0x4025f7d3u, 0x57acc744u,
    0x6399425eu, 0x7b2011cfu, 0x870c8ce9u, 0x92f90803u
};

static const uint32_t k_final_state_pred_parity_76c[2] = {
    0xd8076166u, 0x030c3697u
};

static const uint32_t k_final_state_pred_parity_774[2] = {
    0x192f8cefu, 0xe3516de2u
};

static const uint32_t k_final_state_pred_1bf0[8] = {
    0x696ef0d4u, 0x4c4fc2ffu, 0x2f309528u, 0xb50f36e2u,
    0x97f0090du, 0x7ad0db36u, 0x00af7cf0u, 0xe3904f1bu
};

static const uint32_t k_final_state_pred_1c10[8] = {
    0xa135b619u, 0xae3dc6e7u, 0x17b6f763u, 0x5a6008b5u,
    0x292727bbu, 0xf1cb76bfu, 0xf5cd6153u, 0x2a611ae5u
};

static const uint32_t k_final_state_pred_1c30[8] = {
    0x477e3fcbu, 0x2eef1068u, 0xc343cfd3u, 0xa79f3810u,
    0x23cb1a82u, 0x6d8202c3u, 0x4c0b57fau, 0x19094a0eu
};

static const uint32_t k_final_state_pred_1c50[8] = {
    0xa1a3ff4du, 0x7dd17fb3u, 0x3865d4f7u, 0x60ba8691u,
    0x6f7898bfu, 0x101cd1bbu, 0x61cb47c7u, 0xc8081121u
};

static const uint32_t k_final_state_pred_1c70[8] = {
    0x607247a3u, 0x7cea9982u, 0xbc403881u, 0x8b49abceu,
    0x8892cd5cu, 0x05ecb9cdu, 0xb38510d4u, 0x7b108988u
};

static const uint32_t k_final_state_pred_1c90[8] = {
    0xb6f4cf26u, 0xab003155u, 0x9f0b9382u, 0x9316f5b1u,
    0x872257deu, 0x7b2dba0du, 0x6f391c3au, 0xc2e96cf8u
};

static const uint32_t k_final_state_pred_1cb0[8] = {
    0x8754c22bu, 0x738514d5u, 0x5e58aac9u, 0xeca8da7fu,
    0xbf4678d1u, 0x55adf15du, 0xdebefa99u, 0x4228db0fu
};

static const uint32_t k_final_state_pred_1cd0[8] = {
    0x0eccf5aeu, 0xe90aa2e5u, 0x147a7cc6u, 0xd5badcddu,
    0x377fbbf3u, 0xb0520796u, 0xc0766d5bu, 0x69e51d17u
};

static const uint32_t k_final_state_pred_1cf0[8] = {
    0xe0487ae5u, 0xe8fcc5edu, 0x318f0da9u, 0x4261e6a3u,
    0x8a53b501u, 0x780285b3u, 0xd7ac530bu, 0x68b387b7u
};

static const uint32_t k_final_state_pred_1d10[8] = {
    0x5b82ffdeu, 0xe1ec77e9u, 0xdcd41348u, 0x34b3a0f4u,
    0xa56fa05fu, 0x0fb05c9au, 0x71e9b7f6u, 0xd7b2f391u
};

uint32_t l3_authorization_finalizer_reduce_vector_stage04(uint32_t value, unsigned rounds) {
    for (unsigned i = 0; i < rounds; ++i) {
        value = k_final_state_fold_3958[value & 0x0fu] + (value >> 4);
    }
    return value;
}

uint32_t l3_authorization_finalizer_reduce_vector_stage05(uint32_t value, unsigned rounds) {
    for (unsigned i = 0; i < rounds; ++i) {
        value = k_final_state_fold_3998[value & 0x0fu] + (value >> 4);
    }
    return value;
}

static uint32_t l3_authorization_finalizer_predicate_stage04_lane_driver(uint32_t word,
                                                   uint32_t *affine_out,
                                                   uint32_t *reduced_out,
                                                   uint32_t *base_out) {
    const uint32_t affine = (word >> 1) * 0x6c7c4715u +
                            k_final_state_pred_parity_76c[word & 1u];
    const uint32_t reduced = l3_authorization_finalizer_reduce_vector_stage04(
        affine * 0xc88ae421u + 0x521e55e2u, 7u);
    const uint32_t base = affine * 0x7b4bfb3fu + 0x1ee4924fu;
    if (affine_out) *affine_out = affine;
    if (reduced_out) *reduced_out = reduced;
    if (base_out) *base_out = base;
    return base + ((reduced >> 3) * 0x2e2du +
                   k_final_state_pred_1bf0[reduced & 7u]) * 0x80000000u;
}

static uint32_t l3_authorization_finalizer_predicate_stage05_lane_driver(uint32_t word,
                                                   uint32_t *affine_out,
                                                   uint32_t *reduced_out,
                                                   uint32_t *base_out) {
    const uint32_t affine = (word >> 1) * 0x048c31b9u +
                            k_final_state_pred_parity_774[word & 1u];
    const uint32_t reduced = l3_authorization_finalizer_reduce_vector_stage05(
        affine * 0xc828c513u + 0xf498d176u, 7u);
    const uint32_t base = affine * 0xcb1d32d5u - 0x0856a877u;
    if (affine_out) *affine_out = affine;
    if (reduced_out) *reduced_out = reduced;
    if (base_out) *base_out = base;
    return base + ((reduced >> 3) * 0xf71du +
                   k_final_state_pred_1c90[reduced & 7u]) * 0x80000000u;
}

void l3_authorization_finalizer_feedback_state_a_odd_stage04_refresh(
    const uint32_t local_188_in[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_feedback_state_a_stage04_state *out) {
    if (!local_188_in || !out) return;
    memset(out, 0, sizeof(*out));
    memcpy(out->input_words, local_188_in, sizeof(out->input_words));
    memcpy(out->output_words, local_188_in, sizeof(out->output_words));
    out->branch_applicable = (local_188_in[0] & 1u) ? 1u : 0u;
    if (!out->branch_applicable) return;

    uint32_t affine = 0, reduced = 0, base = 0;
    uint32_t driver = l3_authorization_finalizer_predicate_stage04_lane_driver(
        local_188_in[0] * k_final_state_pred_1c10[0] + k_final_state_pred_1c30[0],
        &affine, &reduced, &base);
    out->lane_mix[0] = local_188_in[0] * k_final_state_pred_1c10[0] + k_final_state_pred_1c30[0];
    out->lane_affine[0] = affine;
    out->lane_reduced[0] = reduced;
    out->initial_driver = driver;

    for (unsigned lane = 0; lane < L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS - 1u; ++lane) {
        const unsigned next_lane = lane + 1u;
        const unsigned j = next_lane & 7u;
        const uint32_t next_mix = local_188_in[next_lane] * k_final_state_pred_1c10[j] +
                                  k_final_state_pred_1c30[j];
        uint32_t next_affine = 0, next_reduced = 0, next_base = 0;
        const uint32_t next_driver = l3_authorization_finalizer_predicate_stage04_lane_driver(
            next_mix, &next_affine, &next_reduced, &next_base);
        uint32_t write_driver = driver * 0xd3e2b867u +
            (next_base * 0x5ce6u + next_mix * 0x43afu) * 0x18000000u +
            0x3c446cbbu;

        out->lane_mix[next_lane] = next_mix;
        out->lane_affine[next_lane] = next_affine;
        out->lane_reduced[next_lane] = next_reduced;
        out->write_driver[lane] = write_driver;
        out->output_words[lane] = k_final_state_pred_1c50[lane & 7u] * write_driver +
                                  k_final_state_pred_1c70[lane & 7u];

        if (lane == L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS - 2u) {
            const unsigned term = L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS - 1u;
            write_driver = next_driver * 0xd3e2b867u + 0x4c446cbbu;
            out->write_driver[term] = write_driver;
            out->output_words[term] = k_final_state_pred_1c50[term & 7u] * write_driver +
                                      k_final_state_pred_1c70[term & 7u];
            out->final_driver = write_driver;
        } else {
            driver = next_driver;
        }
    }
}

void l3_authorization_finalizer_feedback_state_a_even_stage05_refresh_reduced_words(
    const uint32_t local_188_in[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_feedback_state_a_stage05_state *out) {
    if (!local_188_in || !out) return;
    memset(out, 0, sizeof(*out));
    memcpy(out->input_words, local_188_in, sizeof(out->input_words));
    out->branch_applicable = ((local_188_in[0] & 1u) == 0u) ? 1u : 0u;
    if (!out->branch_applicable) return;

    uint32_t affine = 0, reduced = 0, base = 0;
    uint32_t driver = l3_authorization_finalizer_predicate_stage05_lane_driver(
        local_188_in[0] * k_final_state_pred_1cb0[0] + k_final_state_pred_1cd0[0],
        &affine, &reduced, &base);
    out->lane_mix[0] = local_188_in[0] * k_final_state_pred_1cb0[0] + k_final_state_pred_1cd0[0];
    out->lane_affine[0] = affine;
    out->lane_reduced[0] = reduced;
    out->initial_driver = driver;

    for (unsigned lane = 0; lane < L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS - 1u; ++lane) {
        const unsigned next_lane = lane + 1u;
        const unsigned j = next_lane & 7u;
        const uint32_t next_mix = local_188_in[next_lane] * k_final_state_pred_1cb0[j] +
                                  k_final_state_pred_1cd0[j];
        uint32_t next_affine = 0, next_reduced = 0, next_base = 0;
        const uint32_t next_driver = l3_authorization_finalizer_predicate_stage05_lane_driver(
            next_mix, &next_affine, &next_reduced, &next_base);
        uint32_t write_driver = driver * 0xfe21e389u +
            (next_base * 0x423a2ddau + next_mix * 0x0f8b4697u) * 0x98000000u +
            0x0c9300c6u;

        out->lane_mix[next_lane] = next_mix;
        out->lane_affine[next_lane] = next_affine;
        out->lane_reduced[next_lane] = next_reduced;
        out->write_driver[lane] = write_driver;
        out->auStack_128_words[lane] = k_final_state_pred_1cf0[lane & 7u] * write_driver +
                                       k_final_state_pred_1d10[lane & 7u];

        if (lane == L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS - 2u) {
            const unsigned term = L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS - 1u;
            write_driver = next_driver * 0xfe21e389u + 0x049300c6u;
            out->write_driver[term] = write_driver;
            out->auStack_128_words[term] = k_final_state_pred_1cf0[term & 7u] * write_driver +
                                           k_final_state_pred_1d10[term & 7u];
            out->final_driver = write_driver;
        } else {
            driver = next_driver;
        }
    }
}


/* ---- v127: post-DAT_f41a3998 DAT_f41a39d8 local_188 regeneration ------ */

static const uint32_t k_final_state_fold_39d8[16] = {
    0x22135407u, 0x94053bbeu, 0x05f72375u, 0x77e90b2cu,
    0xe4d0c9beu, 0x56c2b175u, 0xc8b4992cu, 0x3aa680e3u,
    0xac98689au, 0x1980272cu, 0x8b720ee3u, 0xfd63f69au,
    0x6f55de51u, 0xd147c608u, 0x4339adbfu, 0xb0216c51u
};

static const uint32_t k_final_state_pred_1d30[8] = {
    0x71e53701u, 0xf641c3ffu, 0x0bd6a053u, 0xeda60d55u,
    0x465c773bu, 0x5f1a8227u, 0xc2dc8663u, 0xb6274a25u
};

static const uint32_t k_final_state_pred_1d50[8] = {
    0x4599ff6eu, 0x1929eab7u, 0xf7d6c852u, 0xb20be035u,
    0xc1b65725u, 0x83b2e4bau, 0xe116af55u, 0x5cab16dfu
};

static const uint32_t k_final_state_add_39d8_a_default[4] = {
    0xb541ca64u, 0xaaadae6fu, 0xb2ce365eu, 0xa4055202u
};
static const uint32_t k_final_state_add_39d8_b_default[4] = {
    0xb4787219u, 0x3c3795fcu, 0x0f153210u, 0x4e64ab51u
};
static const uint32_t k_final_state_add_39d8_c_default[4] = {
    0xf4df1139u, 0xbd60be9cu, 0xb2ce365eu, 0x8e3bfa02u
};

uint32_t l3_authorization_finalizer_reduce_vector_stage06(uint32_t value, unsigned rounds) {
    for (unsigned i = 0; i < rounds; ++i) {
        value = k_final_state_fold_39d8[value & 0x0fu] + (value >> 4);
    }
    return value;
}

static void l3_authorization_finalizer_make_reduction_state_from_stage05_reduced_words(
    const uint32_t au[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    uint32_t tail_accum,
    const uint32_t add_a[4],
    const uint32_t add_b[4],
    const uint32_t add_c[4],
    uint32_t out[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS]) {
    static const uint32_t mul_a[4] = {
        0x53149187u, 0xc2833a6fu, 0x14b0f18bu, 0xfd816061u
    };
    static const uint32_t mul_b[4] = {
        0x7a01a4c3u, 0xde4a21b1u, 0x9d800529u, 0x967f2c55u
    };
    for (unsigned i = 0; i < 4u; ++i) {
        out[i] = add_a[i] + au[i] * mul_a[i];
        out[4u + i] = add_b[i] + au[4u + i] * mul_b[i];
        out[8u + i] = add_c[i] + au[8u + i] * mul_a[i];
    }
    out[12] = au[12] * 0x7a01a4c3u + tail_accum + 0x0de239f5u;
}

void l3_authorization_finalizer_feedback_state_a_stage06_regenerate_with_adds(
    const uint32_t auStack_128_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    uint32_t tail_accum,
    const uint32_t add_a[4],
    const uint32_t add_b[4],
    const uint32_t add_c[4],
    l3_authorization_finalizer_feedback_state_a_stage06_state *out) {
    if (!auStack_128_words || !add_a || !add_b || !add_c || !out) return;
    memset(out, 0, sizeof(*out));
    memcpy(out->auStack_128_words, auStack_128_words, sizeof(out->auStack_128_words));
    out->tail_accum = tail_accum;

    l3_authorization_finalizer_make_reduction_state_from_stage05_reduced_words(
        auStack_128_words, tail_accum, add_a, add_b, add_c,
        out->local_328_stream);

    uint32_t driver = 0x33ae498fu;
    for (unsigned lane = 0; lane < L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS; ++lane) {
        const uint32_t cur = driver * 0x7fbc57a9u + out->local_328_stream[lane];
        const uint32_t reduced = l3_authorization_finalizer_reduce_vector_stage06(
            cur * 0x391b2111u + 0xe0f03a67u, 7u);

        out->driver39d8[lane] = cur;
        out->reduced39d8[lane] = reduced;
        out->local_188_words[lane] =
            (cur * 0xda9bb7bdu + reduced * 0x30000000u + 0x27ffc735u) *
            k_final_state_pred_1d30[lane & 7u] +
            k_final_state_pred_1d50[lane & 7u];

        if (lane == L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS - 1u) {
            driver = cur;
            break;
        }

        const uint32_t reduced_next = l3_authorization_finalizer_reduce_vector_stage06(reduced, 1u);
        driver = reduced * 0x231ee309u +
                 reduced_next * 0xce11cf70u +
                 0x3758a166u;
    }
    out->final_driver = driver;
}

void l3_authorization_finalizer_feedback_state_a_stage06_regenerate_from_reduced_words(
    const uint32_t auStack_128_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    uint32_t tail_accum,
    l3_authorization_finalizer_feedback_state_a_stage06_state *out) {
    l3_authorization_finalizer_feedback_state_a_stage06_regenerate_with_adds(
        auStack_128_words, tail_accum,
        k_final_state_add_39d8_a_default,
        k_final_state_add_39d8_b_default,
        k_final_state_add_39d8_c_default,
        out);
}


/* ---- v128: local_1f0 odd branch reducer, DAT_f41a3a18 --------------- */

static const uint32_t k_final_state_fold_3a18[16] = {
    0xba50e31eu, 0x4a06a941u, 0xd9bc6f64u, 0x6c6502eeu,
    0xfc1ac911u, 0x8ec35c9bu, 0x1e7922beu, 0xa121b648u,
    0x30d77c6bu, 0xc3800ff5u, 0x5335d618u, 0xe2eb9c3bu,
    0x75942fc5u, 0x0549f5e8u, 0x97f28972u, 0x27a84f95u
};

static const uint32_t k_final_state_pred_parity_77c[2] = {
    0xc7cfd71au, 0x7a633c51u
};

static const uint32_t k_final_state_pred_1d70[8] = {
    0x9191d182u, 0x74cf7341u, 0x3e20070fu, 0x215da8ccu,
    0x049b4a8bu, 0xe7d8ec48u, 0xcb168e07u, 0xae542fc4u
};

static const uint32_t k_final_state_pred_1d90[8] = {
    0x4dbcab25u, 0xc1f3de79u, 0x24dc751fu, 0x4b0435b9u,
    0x65948b15u, 0xc39f58d1u, 0x78f3147du, 0xf82d7d47u
};

static const uint32_t k_final_state_pred_1db0[8] = {
    0xa8e27381u, 0xedf0acb2u, 0xa690490eu, 0xa92fa1f1u,
    0x3553bf7au, 0xd38c28c8u, 0x4b9bcf19u, 0xf9507c87u
};

static const uint32_t k_final_state_pred_1dd0[8] = {
    0x71a418efu, 0x21913d23u, 0x6af9dc15u, 0x53d2c163u,
    0x10c6c81fu, 0x3dff299bu, 0x8bd40aa7u, 0x3980bd9du
};

static const uint32_t k_final_state_pred_1df0[8] = {
    0x1d7fe358u, 0xa20150ffu, 0x379fd975u, 0xba92c388u,
    0x3ed17c83u, 0x7f6aa3a1u, 0x6fb282a0u, 0x4bd460beu
};

uint32_t l3_authorization_finalizer_reduce_vector_stage07(uint32_t value, unsigned rounds) {
    for (unsigned i = 0; i < rounds; ++i) {
        value = k_final_state_fold_3a18[value & 0x0fu] + (value >> 4);
    }
    return value;
}

static uint32_t l3_authorization_finalizer_predicate_stage07_lane_driver(uint32_t word,
                                                   uint32_t *affine_out,
                                                   uint32_t *reduced_out,
                                                   uint32_t *base_out) {
    const uint32_t affine = (word >> 1) * 0x989c2a47u +
                            k_final_state_pred_parity_77c[word & 1u];
    const uint32_t reduced = l3_authorization_finalizer_reduce_vector_stage07(
        affine * 0x6ef68f3fu + 0x2536884eu, 7u);
    const uint32_t base = affine * 0xcd5651dbu + 0x66d8bd2fu;
    if (affine_out) *affine_out = affine;
    if (reduced_out) *reduced_out = reduced;
    if (base_out) *base_out = base;
    return base + ((reduced >> 3) * 0x2cc1u +
                   k_final_state_pred_1d70[reduced & 7u]) * 0x80000000u;
}

void l3_authorization_finalizer_lane_state_b_odd_stage07_refresh(
    const uint32_t local_1f0_in[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_lane_state_b_stage07_state *out) {
    if (!local_1f0_in || !out) return;
    memset(out, 0, sizeof(*out));
    memcpy(out->input_words, local_1f0_in, sizeof(out->input_words));
    memcpy(out->output_words, local_1f0_in, sizeof(out->output_words));
    out->branch_applicable = (local_1f0_in[0] & 1u) ? 1u : 0u;
    if (!out->branch_applicable) return;

    uint32_t affine = 0, reduced = 0, base = 0;
    uint32_t driver = l3_authorization_finalizer_predicate_stage07_lane_driver(
        local_1f0_in[0] * k_final_state_pred_1d90[0] + k_final_state_pred_1db0[0],
        &affine, &reduced, &base);
    out->lane_mix[0] = local_1f0_in[0] * k_final_state_pred_1d90[0] +
                       k_final_state_pred_1db0[0];
    out->lane_affine[0] = affine;
    out->lane_reduced[0] = reduced;
    out->initial_driver = driver;

    for (unsigned lane = 0; lane < L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS - 1u; ++lane) {
        const unsigned next_lane = lane + 1u;
        const unsigned j = next_lane & 7u;
        const uint32_t next_mix = local_1f0_in[next_lane] * k_final_state_pred_1d90[j] +
                                  k_final_state_pred_1db0[j];
        uint32_t next_affine = 0, next_reduced = 0, next_base = 0;
        const uint32_t next_driver = l3_authorization_finalizer_predicate_stage07_lane_driver(
            next_mix, &next_affine, &next_reduced, &next_base);
        uint32_t write_driver = driver * 0x6705c75fu +
            (next_base * 0x2942u + next_mix * 0x0423u) * 0x08000000u -
            0x204843a7u;

        out->lane_mix[next_lane] = next_mix;
        out->lane_affine[next_lane] = next_affine;
        out->lane_reduced[next_lane] = next_reduced;
        out->write_driver[lane] = write_driver;
        out->output_words[lane] = k_final_state_pred_1dd0[lane & 7u] * write_driver +
                                  k_final_state_pred_1df0[lane & 7u];

        if (lane == L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS - 2u) {
            const unsigned term = L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS - 1u;
            write_driver = next_driver * 0x6705c75fu - 0x504843a7u;
            out->write_driver[term] = write_driver;
            out->output_words[term] = k_final_state_pred_1dd0[term & 7u] * write_driver +
                                      k_final_state_pred_1df0[term & 7u];
            out->final_driver = write_driver;
        } else {
            driver = next_driver;
        }
    }
}

/* ---- v129: local_258 odd/even branch reducers, UNK_f41a3a58/3a98 ---- */

static const uint32_t k_final_state_fold_3a58[16] = {
    0x152cf610u, 0xabc4c067u, 0x3cb56a53u, 0xcda6143fu,
    0x5e96be2bu, 0xef876817u, 0x70781203u, 0x070fdc5au,
    0x98008646u, 0x28f13032u, 0xb9e1da1eu, 0x4ad2840au,
    0xd16a4e61u, 0x625af84du, 0xf34ba239u, 0x843c4c25u
};

static const uint32_t k_final_state_fold_3a98[16] = {
    0x8796c464u, 0x77e6e9d1u, 0x67e191fdu, 0x57dc3a29u,
    0x47d6e255u, 0x37d18a81u, 0x27cc32adu, 0x17c6dad9u,
    0x07c18305u, 0xf7bc2b31u, 0xe7b6d35du, 0xd7b17b89u,
    0xc7ac23b5u, 0xb7a6cbe1u, 0xa7a1740du, 0x979c1c39u
};

static const uint32_t k_final_state_pred_parity_784[2] = {
    0x1f4c6573u, 0xef8482dbu
};

static const uint32_t k_final_state_pred_parity_78c[2] = {
    0x3e50dcc6u, 0xc5c44a0cu
};

static const uint32_t k_final_state_pred_1e10[8] = {
    0x46a4e12du, 0x70c30622u, 0xbde42328u, 0x0b05402eu,
    0x35236525u, 0x8244822bu, 0xcf659f31u, 0xf983c426u
};

static const uint32_t k_final_state_pred_1e30[8] = {
    0xa53e8d39u, 0x0c9a63bbu, 0x9ad3d899u, 0xf7112973u,
    0xf4d6e5ebu, 0xacff4917u, 0x90b54b71u, 0xa5ee27fbu
};

static const uint32_t k_final_state_pred_1e50[8] = {
    0x424cafd3u, 0xe98e8ba9u, 0xbb875402u, 0xd9f3e8c7u,
    0x1e38271bu, 0x1ea48ce2u, 0xd08c09ddu, 0xbf48915eu
};

static const uint32_t k_final_state_pred_1e70[8] = {
    0xce8eff33u, 0xae79a6e1u, 0xd8554613u, 0x5582e679u,
    0xe666fe51u, 0x55d2045du, 0x2737a38bu, 0x7a60a621u
};

static const uint32_t k_final_state_pred_1e90[8] = {
    0xa1977309u, 0x52fef851u, 0x36f8e5a2u, 0xbcdad37fu,
    0x69f724ebu, 0xda6f25deu, 0x6849f5e7u, 0xa5b6b982u
};

static const uint32_t k_final_state_pred_1eb0[8] = {
    0x3a8ad7b8u, 0xfe4b172fu, 0x06e67bd5u, 0x0f81e07bu,
    0x181d4521u, 0x20b8a9c7u, 0x29540e6du, 0x31ef7313u
};

static const uint32_t k_final_state_pred_1ed0[8] = {
    0x49a5ed9bu, 0x49029401u, 0xd053a9bbu, 0xa336fda9u,
    0x4cfdff91u, 0x72bee855u, 0x6a6178c3u, 0x0950fcc1u
};

static const uint32_t k_final_state_pred_1ef0[8] = {
    0x8683ffacu, 0x84532eceu, 0xbd954bc9u, 0xdf3ed948u,
    0xd581fd04u, 0x10863869u, 0x5a61642au, 0xf085281du
};

static const uint32_t k_final_state_pred_1f10[8] = {
    0xdb123f1bu, 0x034e1659u, 0x6b6dd679u, 0xcdf39d55u,
    0xc47f6bfbu, 0x0b0434f1u, 0x1ebec50fu, 0xdf6d694bu
};

static const uint32_t k_final_state_pred_1f30[8] = {
    0x8a1fdce3u, 0x45c5159au, 0x6a5436b6u, 0xb8f23fe8u,
    0xb8cd659bu, 0x377c3a83u, 0x38e0a7b3u, 0x25cc3e97u
};

uint32_t l3_authorization_finalizer_reduce_vector_stage08(uint32_t value, unsigned rounds) {
    for (unsigned i = 0; i < rounds; ++i) {
        value = k_final_state_fold_3a58[value & 0x0fu] + (value >> 4);
    }
    return value;
}

uint32_t l3_authorization_finalizer_reduce_vector_stage09(uint32_t value, unsigned rounds) {
    for (unsigned i = 0; i < rounds; ++i) {
        value = k_final_state_fold_3a98[value & 0x0fu] + (value >> 4);
    }
    return value;
}

static uint32_t l3_authorization_finalizer_predicate_stage08_lane_driver(uint32_t word,
                                                   uint32_t *affine_out,
                                                   uint32_t *reduced_out,
                                                   uint32_t *base_out) {
    const uint32_t affine = (word >> 1) * 0x388f985fu +
                            k_final_state_pred_parity_784[word & 1u];
    const uint32_t reduced = l3_authorization_finalizer_reduce_vector_stage08(
        affine * 0x5af7d965u + 0x3aa689c6u, 7u);
    const uint32_t base = affine * 0x09bcae2du - 0x63a058bdu;
    if (affine_out) *affine_out = affine;
    if (reduced_out) *reduced_out = reduced;
    if (base_out) *base_out = base;
    return base + ((reduced >> 3) * 0xe0b3u +
                   k_final_state_pred_1e10[reduced & 7u]) * 0x80000000u;
}

static uint32_t l3_authorization_finalizer_predicate_stage09_lane_driver(uint32_t word,
                                                   uint32_t *affine_out,
                                                   uint32_t *reduced_out,
                                                   uint32_t *base_out) {
    const uint32_t affine = (word >> 1) * 0xa3defe3bu +
                            k_final_state_pred_parity_78c[word & 1u];
    const uint32_t reduced = l3_authorization_finalizer_reduce_vector_stage09(
        affine * 0x07a985f1u + 0x24a748e6u, 7u);
    const uint32_t base = affine * 0x5f0f34b1u + 0x4eed977du;
    if (affine_out) *affine_out = affine;
    if (reduced_out) *reduced_out = reduced;
    if (base_out) *base_out = base;
    return base + ((reduced >> 3) * 0x9967u +
                   k_final_state_pred_1eb0[reduced & 7u]) * 0x80000000u;
}

void l3_authorization_finalizer_feedback_state_b_odd_stage08_refresh(
    const uint32_t local_258_in[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_feedback_state_b_stage08_state *out) {
    if (!local_258_in || !out) return;
    memset(out, 0, sizeof(*out));
    memcpy(out->input_words, local_258_in, sizeof(out->input_words));
    memcpy(out->output_words, local_258_in, sizeof(out->output_words));
    out->branch_applicable = (local_258_in[0] & 1u) ? 1u : 0u;
    if (!out->branch_applicable) return;

    uint32_t affine = 0, reduced = 0, base = 0;
    uint32_t driver = l3_authorization_finalizer_predicate_stage08_lane_driver(
        local_258_in[0] * k_final_state_pred_1e30[0] + k_final_state_pred_1e50[0],
        &affine, &reduced, &base);
    out->lane_mix[0] = local_258_in[0] * k_final_state_pred_1e30[0] +
                       k_final_state_pred_1e50[0];
    out->lane_affine[0] = affine;
    out->lane_reduced[0] = reduced;
    out->initial_driver = driver;

    for (unsigned lane = 0; lane < L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS - 1u; ++lane) {
        const unsigned next_lane = lane + 1u;
        const unsigned j = next_lane & 7u;
        const uint32_t next_mix = local_258_in[next_lane] * k_final_state_pred_1e30[j] +
                                  k_final_state_pred_1e50[j];
        uint32_t next_affine = 0, next_reduced = 0, next_base = 0;
        const uint32_t next_driver = l3_authorization_finalizer_predicate_stage08_lane_driver(
            next_mix, &next_affine, &next_reduced, &next_base);
        uint32_t write_driver = driver * 0xcc43bb61u +
            (next_base * 0x7632u + next_mix * 0x6385u) * 0xb8000000u +
            0x1726f266u;

        out->lane_mix[next_lane] = next_mix;
        out->lane_affine[next_lane] = next_affine;
        out->lane_reduced[next_lane] = next_reduced;
        out->write_driver[lane] = write_driver;
        out->output_words[lane] = k_final_state_pred_1e70[lane & 7u] * write_driver +
                                  k_final_state_pred_1e90[lane & 7u];

        if (lane == L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS - 2u) {
            const unsigned term = L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS - 1u;
            write_driver = next_driver * 0xcc43bb61u + 0x7726f266u;
            out->write_driver[term] = write_driver;
            out->output_words[term] = k_final_state_pred_1e70[term & 7u] * write_driver +
                                      k_final_state_pred_1e90[term & 7u];
            out->final_driver = write_driver;
        } else {
            driver = next_driver;
        }
    }
}

void l3_authorization_finalizer_feedback_state_b_even_stage09_refresh_reduced_words(
    const uint32_t local_258_in[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_feedback_state_b_stage09_state *out) {
    if (!local_258_in || !out) return;
    memset(out, 0, sizeof(*out));
    memcpy(out->input_words, local_258_in, sizeof(out->input_words));
    out->branch_applicable = ((local_258_in[0] & 1u) == 0u) ? 1u : 0u;
    if (!out->branch_applicable) return;

    uint32_t affine = 0, reduced = 0, base = 0;
    uint32_t driver = l3_authorization_finalizer_predicate_stage09_lane_driver(
        local_258_in[0] * k_final_state_pred_1ed0[0] + k_final_state_pred_1ef0[0],
        &affine, &reduced, &base);
    out->lane_mix[0] = local_258_in[0] * k_final_state_pred_1ed0[0] +
                       k_final_state_pred_1ef0[0];
    out->lane_affine[0] = affine;
    out->lane_reduced[0] = reduced;
    out->initial_driver = driver;

    for (unsigned lane = 0; lane < L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS - 1u; ++lane) {
        const unsigned next_lane = lane + 1u;
        const unsigned j = next_lane & 7u;
        const uint32_t next_mix = local_258_in[next_lane] * k_final_state_pred_1ed0[j] +
                                  k_final_state_pred_1ef0[j];
        uint32_t next_affine = 0, next_reduced = 0, next_base = 0;
        const uint32_t next_driver = l3_authorization_finalizer_predicate_stage09_lane_driver(
            next_mix, &next_affine, &next_reduced, &next_base);
        uint32_t write_driver = driver * 0xcafb5335u +
            (next_base * 0x5f66u + next_mix * 0x070fu) * 0x48000000u +
            0x0cc19d7eu;

        out->lane_mix[next_lane] = next_mix;
        out->lane_affine[next_lane] = next_affine;
        out->lane_reduced[next_lane] = next_reduced;
        out->write_driver[lane] = write_driver;
        out->auStack_128_words[lane] = k_final_state_pred_1f10[lane & 7u] * write_driver +
                                       k_final_state_pred_1f30[lane & 7u];

        if (lane == L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS - 2u) {
            const unsigned term = L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS - 1u;
            write_driver = next_driver * 0xcafb5335u + 0x74c19d7eu;
            out->write_driver[term] = write_driver;
            out->auStack_128_words[term] = k_final_state_pred_1f10[term & 7u] * write_driver +
                                           k_final_state_pred_1f30[term & 7u];
            out->final_driver = write_driver;
        } else {
            driver = next_driver;
        }
    }
}


/* ---- v130: post-UNK_f41a3a98 DAT_f41a3ad8 local_258 regeneration ------ */

static const uint32_t k_final_state_fold_3ad8[16] = {
    0xb9451fc3u, 0x0740b4b1u, 0x554a6d0cu, 0xa34601fau,
    0xf14196e8u, 0x4f3d2bd6u, 0x9d46e431u, 0xeb42791fu,
    0x393e0e0du, 0x8747c668u, 0xd5435b56u, 0x233ef044u,
    0x7148a89fu, 0xcf443d8du, 0x1d3fd27bu, 0x6b498ad6u
};

static const uint32_t k_final_state_pred_1f50[8] = {
    0x9f31ec13u, 0x1fd1f281u, 0xb4619ef3u, 0x1eb85919u,
    0x4f207ff1u, 0x460bc17du, 0x9999ef6bu, 0xaef139c1u
};

static const uint32_t k_final_state_pred_1f70[8] = {
    0x87ef99d1u, 0x8e33bb69u, 0x189df16au, 0x67235cd7u,
    0x3cdf3a83u, 0x08a63496u, 0x9685d4efu, 0xef6cea9au
};

static const uint32_t k_final_state_add_3ad8_a_default[4] = {
    0x6ae6ff23u, 0xf68e66d0u, 0x0aaaed1cu, 0xcce95036u
};
static const uint32_t k_final_state_add_3ad8_b_default[4] = {
    0x0928250bu, 0x6eec2ed5u, 0x8d92b5d7u, 0x6dbd26ffu
};
static const uint32_t k_final_state_add_3ad8_c_default[4] = {
    0x882957a4u, 0xb9bdfb49u, 0x0aaaed1cu, 0xdfad5836u
};

uint32_t l3_authorization_finalizer_reduce_vector_stage10(uint32_t value, unsigned rounds) {
    for (unsigned i = 0; i < rounds; ++i) {
        value = k_final_state_fold_3ad8[value & 0x0fu] + (value >> 4);
    }
    return value;
}

static void l3_authorization_finalizer_make_reduction_state_from_stage09_reduced_words(
    const uint32_t au[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    uint32_t tail_accum,
    const uint32_t add_a[4],
    const uint32_t add_b[4],
    const uint32_t add_c[4],
    uint32_t out[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS]) {
    static const uint32_t mul_a[4] = {
        0x56283e59u, 0xed67371bu, 0xf18f50bbu, 0x3781f8d7u
    };
    static const uint32_t mul_b[4] = {
        0x1ccdb6b9u, 0x32ff5493u, 0x42b63d6du, 0x39dbdc49u
    };
    for (unsigned i = 0; i < 4u; ++i) {
        out[i] = add_a[i] + au[i] * mul_a[i];
        out[4u + i] = add_b[i] + au[4u + i] * mul_b[i];
        out[8u + i] = add_c[i] + au[8u + i] * mul_a[i];
    }
    out[12] = au[12] * 0x1ccdb6b9u + tail_accum + 0x74abc977u;
}

void l3_authorization_finalizer_feedback_state_b_stage10_regenerate_with_adds(
    const uint32_t auStack_128_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    uint32_t tail_accum,
    const uint32_t add_a[4],
    const uint32_t add_b[4],
    const uint32_t add_c[4],
    l3_authorization_finalizer_feedback_state_b_stage10_state *out) {
    if (!auStack_128_words || !add_a || !add_b || !add_c || !out) return;
    memset(out, 0, sizeof(*out));
    memcpy(out->auStack_128_words, auStack_128_words, sizeof(out->auStack_128_words));
    out->tail_accum = tail_accum;

    l3_authorization_finalizer_make_reduction_state_from_stage09_reduced_words(
        auStack_128_words, tail_accum, add_a, add_b, add_c,
        out->local_328_stream);

    uint32_t driver = 0x9259fe39u; /* native -0x6da601c7 */
    for (unsigned lane = 0; lane < L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS; ++lane) {
        const uint32_t cur = driver * 0x6ff16b3du + out->local_328_stream[lane];
        const uint32_t reduced = l3_authorization_finalizer_reduce_vector_stage10(
            cur * 0x5457cb13u + 0x56616a83u, 7u);

        out->driver3ad8[lane] = cur;
        out->reduced3ad8[lane] = reduced;
        out->local_258_words[lane] =
            (cur * 0xa38e121du + reduced * 0x10000000u + 0x912e7b53u) *
            k_final_state_pred_1f50[lane & 7u] +
            k_final_state_pred_1f70[lane & 7u];

        if (lane == L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS - 1u) {
            driver = cur;
            break;
        }

        const uint32_t reduced_next = l3_authorization_finalizer_reduce_vector_stage10(reduced, 1u);
        driver = reduced * 0x28613137u +
                 reduced_next * 0x79ecec90u +
                 0x172f69b3u;
    }
    out->final_driver = driver;
}

void l3_authorization_finalizer_feedback_state_b_stage10_regenerate_from_reduced_words(
    const uint32_t auStack_128_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    uint32_t tail_accum,
    l3_authorization_finalizer_feedback_state_b_stage10_state *out) {
    l3_authorization_finalizer_feedback_state_b_stage10_regenerate_with_adds(
        auStack_128_words, tail_accum,
        k_final_state_add_3ad8_a_default,
        k_final_state_add_3ad8_b_default,
        k_final_state_add_3ad8_c_default,
        out);
}

/* ---- v131: local_1f0 even DAT_f41a3b18 scan and DAT_f41a3b58 local_1bc rewrite ---- */

static const uint32_t k_final_state_fold_3b18[16] = {
    0xc875c824u, 0x1c3ea7f9u, 0x600787ceu, 0xb14d9b60u,
    0x05167b35u, 0x58df5b0au, 0xaa256e9cu, 0xfdee4e71u,
    0x41b72e46u, 0x92fd41d8u, 0xe6c621adu, 0x3a8f0182u,
    0x8bd51514u, 0xdf9df4e9u, 0x2366d4beu, 0x74ace850u
};

static const uint32_t k_final_state_fold_3b58[16] = {
    0x3a2156b5u, 0xe5a7c8a6u, 0x912e3a97u, 0x4f3cf783u,
    0xfac36974u, 0xa649db65u, 0x51d04d56u, 0x0d56bf47u,
    0xbb657c33u, 0x66ebee24u, 0x12726015u, 0xcdf8d206u,
    0x797f43f7u, 0x2505b5e8u, 0xd31472d4u, 0x8e9ae4c5u
};

static const uint32_t k_final_state_pred_1f90[8] = {
    0x128e47cfu, 0x9e49634bu, 0xb32f0c7du, 0x452c410bu,
    0x58f0f99fu, 0xba93af53u, 0x119c80d7u, 0xed2f77f5u
};

static const uint32_t k_final_state_pred_1fb0[8] = {
    0x693242edu, 0x2bc2d40bu, 0x4a233609u, 0xace65561u,
    0xa422160bu, 0x6bc15e67u, 0xc086a23bu, 0xbe117419u
};

static const uint32_t k_final_state_pred_1fd0[8] = {
    0xd999ef02u, 0x41c51906u, 0x0a89a06au, 0x0e8bba8cu,
    0x618c46fbu, 0xbbc605e5u, 0x872631ccu, 0xf4644249u
};

static const uint32_t k_final_state_pred_1ff0[8] = {
    0x8804c185u, 0xb82aa203u, 0x96aca159u, 0x502394c1u,
    0xdc0b9003u, 0x19864637u, 0x17bdfb53u, 0x6528ef49u
};

static const uint32_t k_final_state_pred_2010[8] = {
    0x1863a4c4u, 0x42f93689u, 0x2c650e3bu, 0xc255c37au,
    0x720dc992u, 0xafdfb18au, 0x773e7a36u, 0xa75d4e57u
};

uint32_t l3_authorization_finalizer_reduce_vector_stage11(uint32_t value, unsigned rounds) {
    for (unsigned i = 0; i < rounds; ++i) {
        value = k_final_state_fold_3b18[value & 0x0fu] + (value >> 4);
    }
    return value;
}

uint32_t l3_authorization_finalizer_reduce_vector_stage12(uint32_t value, unsigned rounds) {
    for (unsigned i = 0; i < rounds; ++i) {
        value = k_final_state_fold_3b58[value & 0x0fu] + (value >> 4);
    }
    return value;
}

void l3_authorization_finalizer_predicate_stage11_scan(
    const uint32_t local_1bc_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_predicate_stage11_state *out) {
    if (!local_1bc_words || !local_1f0_words || !out) return;
    memset(out, 0, sizeof(*out));
    memcpy(out->local_1bc_words, local_1bc_words, sizeof(out->local_1bc_words));
    memcpy(out->local_1f0_words, local_1f0_words, sizeof(out->local_1f0_words));
    out->selected_index = 0xffffffffu;
    out->branch_applicable = ((local_1f0_words[0] * 0x0000c665u) & 1u) == 0u;
    if (!out->branch_applicable) return;

    for (int lane = (int)L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS - 1; lane >= 0; --lane) {
        const unsigned j = (unsigned)lane & 7u;
        const uint32_t mix = local_1bc_words[lane] * k_final_state_pred_1fb0[j] +
                             local_1f0_words[lane] * k_final_state_pred_1f90[j] +
                             k_final_state_pred_1fd0[j];
        out->lane_mix[lane] = mix;
        out->visited_mask |= 1u << (unsigned)lane;
        if (mix == 0x7c742affu) {
            out->sentinel_mask |= 1u << (unsigned)lane;
            continue;
        }
        const uint32_t reduced = l3_authorization_finalizer_reduce_vector_stage11(mix, 7u);
        out->lane_reduced[lane] = reduced;
        out->selected_index = (uint32_t)lane;
        out->selected_mix = mix;
        out->selected_reduced = reduced;
        out->goes_to_c58_final_path = ((reduced & 0x0fu) == 0x0fu);
        out->goes_to_3b58_rewrite = !out->goes_to_c58_final_path;
        return;
    }
    out->goes_to_c58_final_path = 1u;
}

static void l3_authorization_finalizer_make_reduction_state_from_1bc_1f0_stage12(
    const uint32_t local_1bc[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_1f0[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    uint32_t out[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS]) {
    static const uint32_t m_1bc_a[4] = {
        0x0347f757u, 0xb736068du, 0x42b6dc45u, 0xf9b70157u
    };
    static const uint32_t m_1f0_a[4] = {
        0xe4b4bb97u, 0x55768651u, 0x54a8f857u, 0xd4fe4ebbu
    };
    static const uint32_t add_a[4] = {
        0x1ccafa20u, 0x48917894u, 0x99f401feu, 0xfb8fccc9u
    };
    static const uint32_t m_1bc_b[4] = {
        0x288e39a3u, 0x8d3cc047u, 0x876f52ddu, 0xaa0ab081u
    };
    static const uint32_t m_1f0_b[4] = {
        0xf12b22bfu, 0x5b4e7fd3u, 0x612c04a9u, 0xd44297abu
    };
    static const uint32_t add_b[4] = {
        0x867e811bu, 0x06034e3eu, 0x6b9a688fu, 0x9efc328cu
    };

    out[0] = local_1bc[0] * 0xaa0ab081u +
             local_1f0[0] * 0xd44297abu +
             0x9efc328cu;
    for (unsigned i = 0; i < 4u; ++i) {
        out[1u + i] = local_1bc[1u + i] * m_1bc_a[i] +
                      local_1f0[1u + i] * m_1f0_a[i] + add_a[i];
        out[5u + i] = local_1bc[5u + i] * m_1bc_b[i] +
                      local_1f0[5u + i] * m_1f0_b[i] + add_b[i];
        out[9u + i] = local_1bc[9u + i] * m_1bc_a[i] +
                      local_1f0[9u + i] * m_1f0_a[i] + add_a[i];
    }
}

void l3_authorization_finalizer_lane_state_a_stage12_rewrite(
    const uint32_t local_1bc_in[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_lane_state_a_stage12_state *out) {
    if (!local_1bc_in || !local_1f0_words || !out) return;
    memset(out, 0, sizeof(*out));
    memcpy(out->input_1bc_words, local_1bc_in, sizeof(out->input_1bc_words));
    memcpy(out->input_1f0_words, local_1f0_words, sizeof(out->input_1f0_words));

    l3_authorization_finalizer_predicate_stage11_scan(local_1bc_in, local_1f0_words, &out->scan);
    out->branch_applicable = out->scan.branch_applicable && out->scan.goes_to_3b58_rewrite;
    memcpy(out->output_1bc_words, local_1bc_in, sizeof(out->output_1bc_words));
    if (!out->branch_applicable) return;

    l3_authorization_finalizer_make_reduction_state_from_1bc_1f0_stage12(
        local_1bc_in, local_1f0_words, out->local_328_stream);

    uint32_t driver = out->local_328_stream[0];
    uint32_t scalar = 0x125e3effu;
    for (unsigned lane = 0; lane < L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS; ++lane) {
        const uint32_t cur = scalar * 0xab827e93u + driver;
        const uint32_t reduced = l3_authorization_finalizer_reduce_vector_stage12(
            cur * 0x94259d35u + 0x77ab6286u, 7u);
        out->driver3b58[lane] = cur;
        out->reduced3b58[lane] = reduced;
        out->output_1bc_words[lane] =
            (cur * 0x76d3b8cdu + reduced * 0x70000000u + 0x4ba4a5bcu) *
            k_final_state_pred_1ff0[lane & 7u] +
            k_final_state_pred_2010[lane & 7u];

        if (lane == L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS - 1u) {
            driver = cur;
            break;
        }
        const uint32_t reduced_next = l3_authorization_finalizer_reduce_vector_stage12(reduced, 1u);
        driver = out->local_328_stream[lane + 1u];
        scalar = reduced * 0xa39c638fu +
                 reduced_next * 0xc639c710u +
                 0x399520afu;
    }
    out->final_driver = driver;
}

/* ---- v132: alternate DAT_f41a3b18 path and DAT_f41a3c58 local_1f0 rewrite ---- */

static const uint32_t k_final_state_fold_3c58[16] = {
    0x25bab7f6u, 0x93c064b6u, 0x01c61176u, 0x79532f03u,
    0xe758dbc3u, 0x555e8883u, 0xcceba610u, 0x3af152d0u,
    0xa8f6ff90u, 0x10841d1du, 0x8e89c9ddu, 0xfc8f769du,
    0x641c942au, 0xd22240eau, 0x4027edaau, 0xb7b50b37u
};

static const uint32_t k_final_state_pred_2110[8] = {
    0x52860d7fu, 0x6b5b9873u, 0xe1fde945u, 0x8783f8b3u,
    0x653321afu, 0x839d596bu, 0x965d1fb7u, 0xaea3164du
};

static const uint32_t k_final_state_pred_2130[8] = {
    0xb3c188c2u, 0xc9623d61u, 0x337016e3u, 0xc76cd56au,
    0xb03d860du, 0x68bf8953u, 0x24c97cdau, 0x484a18dcu
};

uint32_t l3_authorization_finalizer_reduce_vector_stage15(uint32_t value, unsigned rounds) {
    for (unsigned i = 0; i < rounds; ++i) {
        value = k_final_state_fold_3c58[value & 0x0fu] + (value >> 4);
    }
    return value;
}

static void l3_authorization_finalizer_make_reduction_state_from_1bc_1f0_stage15(
    const uint32_t local_1bc[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_1f0[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    uint32_t out[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS]) {
    static const uint32_t m_1f0_a[4] = {
        0xd5ead583u, 0xfbea0d85u, 0xc64a7943u, 0xbeda6ef7u
    };
    static const uint32_t m_1bc_a[4] = {
        0xd9928443u, 0xb7c4bef1u, 0x597db809u, 0xc8ab1643u
    };
    static const uint32_t add_a[4] = {
        0x74cd9e53u, 0x8c870b57u, 0xac3421c9u, 0x4bcc6c10u
    };
    static const uint32_t m_1f0_b[4] = {
        0xe28cdecbu, 0xd6b09aefu, 0x0de5a7bdu, 0x15181ca7u
    };
    static const uint32_t m_1bc_b[4] = {
        0x63370effu, 0xbbbab1f3u, 0xf4796781u, 0x7e74cd75u
    };
    static const uint32_t add_b[4] = {
        0xdabf628au, 0x12289b09u, 0x5fe84c8eu, 0x1597f5afu
    };

    out[0] = local_1bc[0] * 0x7e74cd75u +
             local_1f0[0] * 0x15181ca7u +
             0x1597f5afu;
    for (unsigned i = 0; i < 4u; ++i) {
        out[1u + i] = local_1f0[1u + i] * m_1f0_a[i] +
                      local_1bc[1u + i] * m_1bc_a[i] + add_a[i];
        out[5u + i] = local_1f0[5u + i] * m_1f0_b[i] +
                      local_1bc[5u + i] * m_1bc_b[i] + add_b[i];
        out[9u + i] = local_1f0[9u + i] * m_1f0_a[i] +
                      local_1bc[9u + i] * m_1bc_a[i] + add_a[i];
    }
}

void l3_authorization_finalizer_lane_state_b_stage15_rewrite(
    const uint32_t local_1bc_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_1f0_in[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_lane_state_b_stage15_state *out) {
    if (!local_1bc_words || !local_1f0_in || !out) return;
    memset(out, 0, sizeof(*out));
    memcpy(out->input_1bc_words, local_1bc_words, sizeof(out->input_1bc_words));
    memcpy(out->input_1f0_words, local_1f0_in, sizeof(out->input_1f0_words));
    memcpy(out->output_1f0_words, local_1f0_in, sizeof(out->output_1f0_words));

    l3_authorization_finalizer_predicate_stage11_scan(local_1bc_words, local_1f0_in, &out->scan);
    out->branch_applicable = out->scan.branch_applicable && out->scan.goes_to_c58_final_path;
    if (!out->branch_applicable) return;

    l3_authorization_finalizer_make_reduction_state_from_1bc_1f0_stage15(
        local_1bc_words, local_1f0_in, out->local_328_stream);

    uint32_t driver = out->local_328_stream[0];
    uint32_t scalar = 0x1d09933du;
    for (unsigned lane = 0; lane < L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS; ++lane) {
        const uint32_t cur = scalar * 0xfb7da4d9u + driver;
        const uint32_t reduced = l3_authorization_finalizer_reduce_vector_stage15(
            cur * 0xbe4ecd97u + 0x0b7a10ad3u, 7u);

        out->driver3c58[lane] = cur;
        out->reduced3c58[lane] = reduced;
        out->output_1f0_words[lane] =
            (cur * 0x7764be69u + reduced * 0x10000000u + 0x37eb3e6cu) *
            k_final_state_pred_2110[lane & 7u] +
            k_final_state_pred_2130[lane & 7u];

        if (lane == L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS - 1u) {
            driver = cur;
            break;
        }
        const uint32_t reduced_next = l3_authorization_finalizer_reduce_vector_stage15(reduced, 1u);
        driver = out->local_328_stream[lane + 1u];
        scalar = reduced * 0x4ac2feffu +
                 reduced_next * 0x53d01010u +
                 0x01a12f77u;
    }
    out->final_driver = driver;
}


/* ---- v133: post-DAT_f41a3c58 FDE21C split, DAT_f41a3c98/DAT_f41a3cd8 local_258 rewrites ---- */

static const uint32_t k_final_state_fold_3c98[16] = {
    0x559f9597u, 0x24334bb4u, 0xf754bb0eu, 0xc5e8712bu,
    0x947c2748u, 0x679d96a2u, 0x36314cbfu, 0x04c502dcu,
    0xd7e67236u, 0xa67a2853u, 0x750dde70u, 0x482f4dcau,
    0x16c303e7u, 0xe556ba04u, 0xb3ea7021u, 0x870bdf7bu
};

static const uint32_t k_final_state_fold_3cd8[16] = {
    0x889a15edu, 0x3cb29614u, 0xef4313ccu, 0x935b93f3u,
    0x4774141au, 0xfb8c9441u, 0xafa51468u, 0x53bd948fu,
    0x07d614b6u, 0xbbee94ddu, 0x60071504u, 0x141f952bu,
    0xc8381552u, 0x7c509579u, 0x206915a0u, 0xd48195c7u
};

static const uint32_t k_final_state_pred_2150[8] = {
    0xe595906fu, 0xb4df9235u, 0xcc4268cfu, 0x4ac153adu,
    0x21c74265u, 0x26d77661u, 0x3bbce5a7u, 0xbc2f3275u
};

static const uint32_t k_final_state_pred_2170[8] = {
    0x01c631b8u, 0xa6bfcb46u, 0x664ea4b1u, 0x00cb59ecu,
    0xa763bc10u, 0xe23599ffu, 0xe52b8bceu, 0x6eb49cb7u
};

static const uint32_t k_final_state_pred_2190[8] = {
    0x9b732c55u, 0x49acbc77u, 0xa9468275u, 0x42c8e91fu,
    0x29461387u, 0x6ef6699bu, 0xdfe7163du, 0x4458fb37u
};

static const uint32_t k_final_state_pred_21b0[8] = {
    0xc90ffd52u, 0xff8dcb84u, 0x80cc228bu, 0xe6992afau,
    0x068f2d6eu, 0x3feba745u, 0xc278deb8u, 0x00ea1e75u
};

uint32_t l3_authorization_finalizer_reduce_vector_stage16(uint32_t value, unsigned rounds) {
    for (unsigned i = 0; i < rounds; ++i) {
        value = k_final_state_fold_3c98[value & 0x0fu] + (value >> 4);
    }
    return value;
}

uint32_t l3_authorization_finalizer_reduce_vector_stage17(uint32_t value, unsigned rounds) {
    for (unsigned i = 0; i < rounds; ++i) {
        value = k_final_state_fold_3cd8[value & 0x0fu] + (value >> 4);
    }
    return value;
}

void l3_authorization_finalizer_feedback_state_b_stage16_optional_rewrite_from_stream(
    const uint32_t local_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_258_in[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_328_stream[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_feedback_state_b_stage16_state *out) {
    if (!local_188_words || !local_258_in || !local_328_stream || !out) return;
    memset(out, 0, sizeof(*out));
    memcpy(out->input_188_words, local_188_words, sizeof(out->input_188_words));
    memcpy(out->input_258_words, local_258_in, sizeof(out->input_258_words));
    memcpy(out->local_328_stream, local_328_stream, sizeof(out->local_328_stream));
    memcpy(out->output_258_words, local_258_in, sizeof(out->output_258_words));

    out->fde21c_result = (uint32_t)l3_authorization_finalizer_predicate_exact(local_188_words, local_258_in);
    out->branch_applicable = (out->fde21c_result == 0u);
    if (!out->branch_applicable) return;

    uint32_t driver = local_328_stream[0];
    uint32_t scalar = 0x4d27f55bu;
    for (unsigned lane = 0; lane < L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS; ++lane) {
        const uint32_t cur = scalar * 0xfc566671u + driver;
        const uint32_t reduced = l3_authorization_finalizer_reduce_vector_stage16(
            cur * 0xe9e17f37u + 0xb50a7050u, 7u);

        out->driver3c98[lane] = cur;
        out->reduced3c98[lane] = reduced;
        out->output_258_words[lane] =
            (cur * 0x1c1419a5u + reduced * 0xd0000000u + 0x5105a8eau) *
            k_final_state_pred_2150[lane & 7u] +
            k_final_state_pred_2170[lane & 7u];

        if (lane == L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS - 1u) {
            driver = cur;
            break;
        }
        const uint32_t reduced_next = l3_authorization_finalizer_reduce_vector_stage16(reduced, 1u);
        driver = local_328_stream[lane + 1u];
        scalar = reduced * 0x97614877u +
                 reduced_next * 0x89eb7890u +
                 0x59bf4bc6u;
    }
    out->final_driver = driver;
}

void l3_authorization_finalizer_feedback_state_b_stage17_rewrite_from_stream(
    const uint32_t local_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_258_in[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_328_stream[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_feedback_state_b_stage17_state *out) {
    if (!local_188_words || !local_258_in || !local_328_stream || !out) return;
    memset(out, 0, sizeof(*out));
    memcpy(out->input_188_words, local_188_words, sizeof(out->input_188_words));
    memcpy(out->input_258_words, local_258_in, sizeof(out->input_258_words));
    memcpy(out->local_328_stream, local_328_stream, sizeof(out->local_328_stream));

    uint32_t driver = local_328_stream[0];
    uint32_t scalar = 0x4edfac3cu;
    for (unsigned lane = 0; lane < L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS; ++lane) {
        const uint32_t cur = scalar * 0x10b16cb3u + driver;
        const uint32_t reduced = l3_authorization_finalizer_reduce_vector_stage17(
            cur * 0xba6394e5u + 0x9535053du, 7u);

        out->driver3cd8[lane] = cur;
        out->reduced3cd8[lane] = reduced;
        out->output_258_words[lane] =
            (cur * 0x7df7c0e7u + reduced * 0x50000000u + 0x9d0690a3u) *
            k_final_state_pred_2190[lane & 7u] +
            k_final_state_pred_21b0[lane & 7u];

        if (lane == L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS - 1u) {
            driver = cur;
            break;
        }
        const uint32_t reduced_next = l3_authorization_finalizer_reduce_vector_stage17(reduced, 1u);
        driver = local_328_stream[lane + 1u];
        scalar = reduced * 0xf69ac3dfu +
                 reduced_next * 0x9653c210u +
                 0x560f0195u;
    }
    out->final_driver = driver;
}


/* ---- v134: materialized local_328 streams for the post-3c58 local_258 rewrites ---- */

void l3_authorization_finalizer_make_reduction_state_stage16_from_parent_feedback_b_tail(
    const uint32_t local_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t parent_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t late_parent_q5_words[4],
    uint32_t local_328_stream[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS]) {
    if (!local_258_words || !parent_words || !late_parent_q5_words || !local_328_stream) return;

    static const uint32_t m258_first[4] = {
        0x12f98923u, 0xf3ac4319u, 0x10eec843u, 0x78916c81u
    };
    static const uint32_t mparent_first[4] = {
        0x1aa9d2cdu, 0xd7f1847fu, 0xa4dc9d93u, 0x772f141bu
    };
    static const uint32_t add_first[4] = {
        0x40416a9bu, 0xfcc58104u, 0xe4cd530cu, 0xcfffa02fu
    };
    static const uint32_t m258_late[4] = {
        0x4947bdd9u, 0x12f98923u, 0xf3ac4319u, 0x10eec843u
    };

    for (unsigned i = 0; i < 4u; ++i) {
        local_328_stream[i] = local_258_words[i] * m258_first[i] +
                              parent_words[i] * mparent_first[i] +
                              add_first[i];
        local_328_stream[7u + i] = late_parent_q5_words[i] +
                                   local_258_words[7u + i] * m258_late[i];
    }

    local_328_stream[4] = local_258_words[4] * 0xe3a33429u +
                          parent_words[4] * 0x4e8bcdefu +
                          0xe3ed10e4u;
    local_328_stream[5] = local_258_words[5] * 0x79ec6b4du +
                          parent_words[5] * 0x52080d7bu +
                          0x8e396c71u;
    local_328_stream[6] = local_258_words[6] * 0x3da7780bu +
                          parent_words[6] * 0xce018f63u +
                          0xcf7bc6d0u;
    local_328_stream[11] = local_258_words[11] * 0x78916c81u +
                           parent_words[11] * 0x0772f141bu +
                           0xcfffa02fu;
    /* The scalar tail reuses the lane-4 parent affine term prepared before
       the predicate loop; it is not a plain copy of local_258[12]. */
    local_328_stream[12] = local_258_words[12] * 0xe3a33429u +
                           parent_words[4] * 0x4e8bcdefu +
                           0xe3ed10e4u;
}

void l3_authorization_finalizer_make_reduction_state_stage17_from_188_258(
    const uint32_t local_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    uint32_t local_328_stream[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS]) {
    if (!local_188_words || !local_258_words || !local_328_stream) return;

    static const uint32_t m258_a[4] = {
        0xeb78ee7bu, 0x682294a1u, 0xa12c3e9bu, 0x80e50749u
    };
    static const uint32_t m188_a[4] = {
        0xa02be903u, 0xae6825fdu, 0xd0636591u, 0x7aecedf7u
    };
    static const uint32_t add_a[4] = {
        0x0ab80e3au, 0x260d650bu, 0x2cdbb84fu, 0x0abda17du
    };
    static const uint32_t m258_b[4] = {
        0x99b93a31u, 0x04b99d75u, 0x147772a3u, 0x68127561u
    };
    static const uint32_t m188_b[4] = {
        0xeb26a7d9u, 0xc9c62ac5u, 0xe2241be1u, 0x2b6d3b07u
    };
    static const uint32_t add_b[4] = {
        0xd8047d7fu, 0xd3e9d23fu, 0xb00e4d0du, 0xfb5b147cu
    };

    for (unsigned i = 0; i < 4u; ++i) {
        local_328_stream[i] = local_258_words[i] * m258_a[i] +
                              local_188_words[i] * m188_a[i] +
                              add_a[i];
        local_328_stream[4u + i] = local_258_words[4u + i] * m258_b[i] +
                                   local_188_words[4u + i] * m188_b[i] +
                                   add_b[i];
        local_328_stream[8u + i] = local_258_words[8u + i] * m258_a[i] +
                                   local_188_words[8u + i] * m188_a[i] +
                                   add_a[i];
    }
    local_328_stream[12] = local_258_words[12] * 0x99b93a31u +
                           local_188_words[12] * 0xeb26a7d9u +
                           0xd8047d7fu;
}

void l3_authorization_finalizer_feedback_state_b_stage16_stage17_rewrite_materialized(
    const uint32_t local_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t parent_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t late_parent_q5_words[4],
    l3_authorization_finalizer_feedback_state_b_stage16_stage17_affine_state *out) {
    if (!local_188_words || !local_258_words || !parent_words || !late_parent_q5_words || !out) return;
    memset(out, 0, sizeof(*out));
    memcpy(out->input_188_words, local_188_words, sizeof(out->input_188_words));
    memcpy(out->input_258_words, local_258_words, sizeof(out->input_258_words));
    memcpy(out->parent_words, parent_words, sizeof(out->parent_words));
    memcpy(out->late_parent_q5_words, late_parent_q5_words, sizeof(out->late_parent_q5_words));

    l3_authorization_finalizer_make_reduction_state_stage16_from_parent_feedback_b_tail(
        local_258_words, parent_words, late_parent_q5_words, out->local328_3c98);
    l3_authorization_finalizer_feedback_state_b_stage16_optional_rewrite_from_stream(
        local_188_words, local_258_words, out->local328_3c98, &out->after_3c98);

    l3_authorization_finalizer_make_reduction_state_stage17_from_188_258(
        local_188_words, out->after_3c98.output_258_words, out->local328_3cd8);
    l3_authorization_finalizer_feedback_state_b_stage17_rewrite_from_stream(
        local_188_words, out->after_3c98.output_258_words, out->local328_3cd8, &out->after_3cd8);
}

/* ---- v135: terminal DAT_f41a3864 gate to final raw40 bridge ----------- */


void l3_authorization_finalizer_terminal_gate_raw40(
    const uint32_t local_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_terminal_gate_raw40_state *out) {
    if (!local_1f0_words || !local_188_words || !out) return;
    memset(out, 0, sizeof(*out));
    memcpy(out->input_1f0_words, local_1f0_words, sizeof(out->input_1f0_words));
    memcpy(out->input_188_words, local_188_words, sizeof(out->input_188_words));

    l3_authorization_finalizer_scan_reduction_predicate(local_1f0_words, &out->gate);
    out->terminal_taken = out->gate.all_match ? 1u : 0u;
    if (!out->terminal_taken) return;

    for (unsigned i = 0; i < 4u; ++i) {
        out->scalars.a_lanes[i] = local_188_words[i];
        out->scalars.b_lanes[i] = local_188_words[4u + i];
    }
    out->scalars.scalar0 = local_188_words[8];
    out->scalars.scalar1 = local_188_words[9];
    l3_authorization_finalizer_final_raw40(&out->scalars, out->raw40);
}


/* ---- v136: post-3b58 local_188 FUN_f3fde21c split, DAT_f41a3bd8/3c18 ---- */

static const uint32_t k_final_state_fold_3bd8[16] = {
    0xd1421094u, 0x8333fcd4u, 0x37ad63d9u, 0xec26cadeu,
    0x90a031e3u, 0x451998e8u, 0xf992ffedu, 0xab84ec2du,
    0x5ffe5332u, 0x0477ba37u, 0xb8f1213cu, 0x6d6a8841u,
    0x1f5c7481u, 0xc3d5db86u, 0x784f428bu, 0x2cc8a990u
};

static const uint32_t k_final_state_fold_3c18[16] = {
    0x038e2b77u, 0x13f99e0fu, 0x246510a7u, 0x34d0833fu,
    0x453bf5d7u, 0x55a7686fu, 0x6f5bb188u, 0x7fc72420u,
    0x803296b8u, 0x909e0950u, 0xa1097be8u, 0xb174ee80u,
    0xc1e06118u, 0xd24bd3b0u, 0xe2b74648u, 0xf322b8e0u
};

static const uint32_t k_final_state_pred_2090[8] = {
    0x9117ef35u, 0x853e07cbu, 0xec79952fu, 0xd2263e99u,
    0x6f4df337u, 0x62734613u, 0x1d72f67fu, 0x4a5995a9u
};

static const uint32_t k_final_state_pred_20b0[8] = {
    0x745aad2bu, 0x3fd68bfau, 0x145bfb99u, 0x66b0b1f6u,
    0x534f83b4u, 0x902ab985u, 0x17b3dc6cu, 0x44506e30u
};

static const uint32_t k_final_state_pred_20d0[8] = {
    0xfd50918bu, 0x0d55b775u, 0x0b806911u, 0x47240127u,
    0x31df3909u, 0x9a11172du, 0xee6793c1u, 0xce424617u
};

static const uint32_t k_final_state_pred_20f0[8] = {
    0x82af2f4du, 0x5eab5fd8u, 0x11c3229fu, 0x8a36bb40u,
    0xadfc558au, 0x9900eeb3u, 0xdc0ed192u, 0x520acb1au
};

uint32_t l3_authorization_finalizer_reduce_vector_stage13(uint32_t value, unsigned rounds) {
    for (unsigned i = 0; i < rounds; ++i) {
        value = k_final_state_fold_3bd8[value & 0x0fu] + (value >> 4);
    }
    return value;
}

uint32_t l3_authorization_finalizer_reduce_vector_stage14(uint32_t value, unsigned rounds) {
    for (unsigned i = 0; i < rounds; ++i) {
        value = k_final_state_fold_3c18[value & 0x0fu] + (value >> 4);
    }
    return value;
}

void l3_authorization_finalizer_feedback_state_a_stage13_optional_rewrite_from_stream(
    const uint32_t local_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_328_stream[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_feedback_state_a_stage13_state *out) {
    if (!local_188_words || !local_258_words || !local_328_stream || !out) return;
    memset(out, 0, sizeof(*out));
    memcpy(out->input_188_words, local_188_words, sizeof(out->input_188_words));
    memcpy(out->input_258_words, local_258_words, sizeof(out->input_258_words));
    memcpy(out->local_328_stream, local_328_stream, sizeof(out->local_328_stream));
    memcpy(out->output_188_words, local_188_words, sizeof(out->output_188_words));

    out->fde21c_result = (uint32_t)l3_authorization_finalizer_predicate_exact(local_188_words, local_258_words);
    out->branch_applicable = out->fde21c_result ? 1u : 0u;
    if (!out->branch_applicable) return;

    uint32_t driver = local_328_stream[0];
    uint32_t scalar = 0u - 0x0e075f76u;
    for (unsigned lane = 0; lane < L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS; ++lane) {
        const uint32_t cur = scalar * (0u - 0x051c4877u) + driver;
        const uint32_t reduced = l3_authorization_finalizer_reduce_vector_stage13(
            cur * (0u - 0x72c02333u) + 0x0c004d07u, 7u);

        out->driver3bd8[lane] = cur;
        out->reduced3bd8[lane] = reduced;
        out->output_188_words[lane] =
            (cur * (0u - 0x61a8acdbu) + reduced * 0x70000000u + 0x7408c640u) *
            k_final_state_pred_2090[lane & 7u] +
            k_final_state_pred_20b0[lane & 7u];

        if (lane == L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS - 1u) {
            driver = cur;
            break;
        }
        const uint32_t reduced_next = l3_authorization_finalizer_reduce_vector_stage13(reduced, 1u);
        driver = local_328_stream[lane + 1u];
        scalar = reduced * (0u - 0x35ac6663u) +
                 reduced_next * 0x5ac66630u +
                 (0u - 0x12269a4bu);
    }
    out->final_driver = driver;
}

void l3_authorization_finalizer_feedback_state_a_stage14_rewrite_from_stream(
    const uint32_t local_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_328_stream[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_feedback_state_a_stage14_state *out) {
    if (!local_188_words || !local_258_words || !local_328_stream || !out) return;
    memset(out, 0, sizeof(*out));
    memcpy(out->input_188_words, local_188_words, sizeof(out->input_188_words));
    memcpy(out->input_258_words, local_258_words, sizeof(out->input_258_words));
    memcpy(out->local_328_stream, local_328_stream, sizeof(out->local_328_stream));

    uint32_t driver = local_328_stream[0];
    uint32_t scalar = 0x0910f816u;
    for (unsigned lane = 0; lane < L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS; ++lane) {
        const uint32_t cur = scalar * 0x65dee01bu + driver;
        const uint32_t reduced = l3_authorization_finalizer_reduce_vector_stage14(
            cur * (0u - 0x461801d5u) + 0x54e7d96fu, 7u);

        out->driver3c18[lane] = cur;
        out->reduced3c18[lane] = reduced;
        out->output_188_words[lane] =
            (cur * 0x6b4110a1u + reduced * (0u - 0x30000000u) + (0u - 0x505d7e82u)) *
            k_final_state_pred_20d0[lane & 7u] +
            k_final_state_pred_20f0[lane & 7u];

        if (lane == L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS - 1u) {
            driver = cur;
            break;
        }
        const uint32_t reduced_next = l3_authorization_finalizer_reduce_vector_stage14(reduced, 1u);
        driver = local_328_stream[lane + 1u];
        scalar = reduced * (0u - 0x46379847u) +
                 reduced_next * 0x63798470u +
                 0x616056b1u;
    }
    out->final_driver = driver;
}
/* ---- v137: materialized local_328 streams for DAT_f41a3bd8/DAT_f41a3c18 ---- */

void l3_authorization_finalizer_make_reduction_state_stage13_from_parent_feedback_a_tail(
    const uint32_t local_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t parent_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t late_parent_q_words[4],
    uint32_t preserved_tail12,
    uint32_t local_328_stream[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS]) {
    if (!local_188_words || !parent_words || !late_parent_q_words || !local_328_stream) return;

    static const uint32_t m188_first[4] = {
        0xa5917299u, 0xd7108a67u, 0x3d4938e3u, 0x19f25335u
    };
    static const uint32_t mparent_first[4] = {
        0x15549e31u, 0xdf8e6d8bu, 0xda58edafu, 0xd65f5dd7u
    };
    static const uint32_t add_first[4] = {
        0xef578883u, 0x99b9e3a3u, 0x2de46fd7u, 0xb997ec80u
    };
    static const uint32_t m188_late[4] = {
        0x69257d65u, 0xa5917299u, 0xd7108a67u, 0x3d4938e3u
    };

    for (unsigned i = 0; i < 4u; ++i) {
        local_328_stream[i] = local_188_words[i] * m188_first[i] +
                              parent_words[i] * mparent_first[i] +
                              add_first[i];
        local_328_stream[7u + i] = late_parent_q_words[i] +
                                   local_188_words[7u + i] * m188_late[i];
    }

    local_328_stream[4] = parent_words[4] * (0u - 0x600f4a45u) +
                          local_188_words[4] * 0x4882153bu +
                          (0u - 0x314127e1u);
    local_328_stream[5] = local_188_words[5] * (0u - 0x032f159c1u) +
                          parent_words[5] * 0x4c6a06b7u +
                          (0u - 0x02a207b0u);
    local_328_stream[6] = local_188_words[6] * (0u - 0x294e652du) +
                          parent_words[6] * (0u - 0x358da641u) +
                          (0u - 0x53233dbbu);
    local_328_stream[11] = local_188_words[11] * 0x19f25335u +
                           parent_words[11] * (0u - 0x29a0a229u) +
                           (0u - 0x46681380u);
    local_328_stream[12] = preserved_tail12;
}

void l3_authorization_finalizer_make_reduction_state_stage14_from_188_258(
    const uint32_t local_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    uint32_t local_328_stream[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS]) {
    if (!local_188_words || !local_258_words || !local_328_stream) return;

    static const uint32_t m188_a[4] = {
        0xed4bb443u, 0x5a809abdu, 0x82124551u, 0x1c074c37u
    };
    static const uint32_t m258_a[4] = {
        0xb5dd3bbbu, 0x81e1f061u, 0x063d03dbu, 0x061a25909u
    };
    static const uint32_t add_a[4] = {
        0x4801953au, 0x06ce7bcbu, 0x911d0e0fu, 0xade563bdu
    };
    static const uint32_t m188_b[4] = {
        0xb81e1599u, 0x62808d85u, 0x741ea7a1u, 0x0a4c89547u
    };
    static const uint32_t m258_b[4] = {
        0x388571f1u, 0x02c21435u, 0xe21b55e3u, 0xa2dca121u
    };
    static const uint32_t add_b[4] = {
        0x1689473fu, 0x33ec6bffu, 0xbdd96b4du, 0x162012fcu
    };

    for (unsigned i = 0; i < 4u; ++i) {
        local_328_stream[i] = local_188_words[i] * m188_a[i] +
                              local_258_words[i] * m258_a[i] +
                              add_a[i];
        local_328_stream[4u + i] = local_188_words[4u + i] * m188_b[i] +
                                   local_258_words[4u + i] * m258_b[i] +
                                   add_b[i];
        local_328_stream[8u + i] = local_188_words[8u + i] * m188_a[i] +
                                   local_258_words[8u + i] * m258_a[i] +
                                   add_a[i];
    }
    local_328_stream[12] = local_188_words[12] * (0u - 0x47e1ea67u) +
                           local_258_words[12] * 0x388571f1u +
                           0x1689473fu;
}

void l3_authorization_finalizer_feedback_state_a_stage13_stage14_rewrite_materialized(
    const uint32_t local_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t parent_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t late_parent_q_words[4],
    uint32_t preserved_tail12,
    l3_authorization_finalizer_feedback_state_a_stage13_stage14_affine_state *out) {
    if (!local_188_words || !local_258_words || !parent_words || !late_parent_q_words || !out) return;
    memset(out, 0, sizeof(*out));
    memcpy(out->input_188_words, local_188_words, sizeof(out->input_188_words));
    memcpy(out->input_258_words, local_258_words, sizeof(out->input_258_words));
    memcpy(out->parent_words, parent_words, sizeof(out->parent_words));
    memcpy(out->late_parent_q_words, late_parent_q_words, sizeof(out->late_parent_q_words));
    out->preserved_tail12 = preserved_tail12;

    l3_authorization_finalizer_make_reduction_state_stage13_from_parent_feedback_a_tail(
        local_188_words, parent_words, late_parent_q_words, preserved_tail12, out->local328_3bd8);
    l3_authorization_finalizer_feedback_state_a_stage13_optional_rewrite_from_stream(
        local_188_words, local_258_words, out->local328_3bd8, &out->after_3bd8);

    l3_authorization_finalizer_make_reduction_state_stage14_from_188_258(
        out->after_3bd8.output_188_words, local_258_words, out->local328_3c18);
    l3_authorization_finalizer_feedback_state_a_stage14_rewrite_from_stream(
        out->after_3bd8.output_188_words, local_258_words, out->local328_3c18, &out->after_3c18);
}


/* ---- v138: composed predicate-loop step for DAT_f41a3b18 -> 3b58 path ---- */

void l3_authorization_finalizer_predicate_loop_stage12_step_materialized(
    const uint32_t local_1bc_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t parent_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t late_parent_q_words[4],
    uint32_t preserved_tail12,
    l3_authorization_finalizer_predicate_loop_stage12_step_state *out) {
    if (!local_1bc_words || !local_1f0_words || !local_188_words ||
        !local_258_words || !parent_words || !late_parent_q_words || !out) return;

    memset(out, 0, sizeof(*out));
    memcpy(out->input_1bc_words, local_1bc_words, sizeof(out->input_1bc_words));
    memcpy(out->input_1f0_words, local_1f0_words, sizeof(out->input_1f0_words));
    memcpy(out->input_188_words, local_188_words, sizeof(out->input_188_words));
    memcpy(out->input_258_words, local_258_words, sizeof(out->input_258_words));
    memcpy(out->parent_words, parent_words, sizeof(out->parent_words));
    memcpy(out->late_parent_q_words, late_parent_q_words, sizeof(out->late_parent_q_words));
    out->preserved_tail12 = preserved_tail12;

    memcpy(out->output_1bc_words, local_1bc_words, sizeof(out->output_1bc_words));
    memcpy(out->output_188_words, local_188_words, sizeof(out->output_188_words));
    memcpy(out->output_258_words, local_258_words, sizeof(out->output_258_words));
    out->path = L3_AUTH_FINAL_STATE_STEP_3B58_NOT_APPLICABLE;

    l3_authorization_finalizer_lane_state_a_stage12_rewrite(
        local_1bc_words, local_1f0_words, &out->after_3b58);

    if (!out->after_3b58.scan.branch_applicable) {
        out->path = L3_AUTH_FINAL_STATE_STEP_3B58_NOT_APPLICABLE;
        return;
    }
    if (out->after_3b58.scan.goes_to_c58_final_path) {
        out->path = L3_AUTH_FINAL_STATE_STEP_3C58_ALTERNATE;
        return;
    }
    if (!out->after_3b58.branch_applicable) {
        out->path = L3_AUTH_FINAL_STATE_STEP_3B58_NOT_APPLICABLE;
        return;
    }

    memcpy(out->output_1bc_words,
           out->after_3b58.output_1bc_words,
           sizeof(out->output_1bc_words));

    l3_authorization_finalizer_feedback_state_a_stage13_stage14_rewrite_materialized(
        local_188_words,
        local_258_words,
        parent_words,
        late_parent_q_words,
        preserved_tail12,
        &out->after_3bd8_3c18);

    memcpy(out->output_188_words,
           out->after_3bd8_3c18.after_3c18.output_188_words,
           sizeof(out->output_188_words));
    memcpy(out->output_258_words, local_258_words, sizeof(out->output_258_words));
    out->path = L3_AUTH_FINAL_STATE_STEP_3B58_TAKEN;
    out->branch_completed = 1u;
}


/* ---- v139: composed alternate DAT_f41a3c58 -> DAT_f41a3c98/3cd8 predicate-loop step ---- */

void l3_authorization_finalizer_predicate_loop_stage15_step_materialized(
    const uint32_t local_1bc_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t parent_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t late_parent_q5_words[4],
    l3_authorization_finalizer_predicate_loop_stage15_step_state *out) {
    if (!local_1bc_words || !local_1f0_words || !local_188_words ||
        !local_258_words || !parent_words || !late_parent_q5_words || !out) return;

    memset(out, 0, sizeof(*out));
    memcpy(out->input_1bc_words, local_1bc_words, sizeof(out->input_1bc_words));
    memcpy(out->input_1f0_words, local_1f0_words, sizeof(out->input_1f0_words));
    memcpy(out->input_188_words, local_188_words, sizeof(out->input_188_words));
    memcpy(out->input_258_words, local_258_words, sizeof(out->input_258_words));
    memcpy(out->parent_words, parent_words, sizeof(out->parent_words));
    memcpy(out->late_parent_q5_words, late_parent_q5_words, sizeof(out->late_parent_q5_words));

    memcpy(out->output_1bc_words, local_1bc_words, sizeof(out->output_1bc_words));
    memcpy(out->output_1f0_words, local_1f0_words, sizeof(out->output_1f0_words));
    memcpy(out->output_188_words, local_188_words, sizeof(out->output_188_words));
    memcpy(out->output_258_words, local_258_words, sizeof(out->output_258_words));
    out->path = L3_AUTH_FINAL_STATE_STEP_3C58_NOT_APPLICABLE;

    l3_authorization_finalizer_lane_state_b_stage15_rewrite(
        local_1bc_words, local_1f0_words, &out->after_3c58);

    if (!out->after_3c58.scan.branch_applicable) {
        out->path = L3_AUTH_FINAL_STATE_STEP_3C58_NOT_APPLICABLE;
        return;
    }
    if (out->after_3c58.scan.goes_to_3b58_rewrite) {
        out->path = L3_AUTH_FINAL_STATE_STEP_3B58_SIBLING;
        return;
    }
    if (!out->after_3c58.branch_applicable) {
        out->path = L3_AUTH_FINAL_STATE_STEP_3C58_NOT_APPLICABLE;
        return;
    }

    memcpy(out->output_1f0_words,
           out->after_3c58.output_1f0_words,
           sizeof(out->output_1f0_words));

    l3_authorization_finalizer_feedback_state_b_stage16_stage17_rewrite_materialized(
        local_188_words,
        local_258_words,
        parent_words,
        late_parent_q5_words,
        &out->after_3c98_3cd8);

    memcpy(out->output_258_words,
           out->after_3c98_3cd8.after_3cd8.output_258_words,
           sizeof(out->output_258_words));
    memcpy(out->output_188_words, local_188_words, sizeof(out->output_188_words));
    out->path = L3_AUTH_FINAL_STATE_STEP_3C58_TAKEN;
    out->branch_completed = 1u;
}


/* ---- v140: one-cycle materialized predicate-network state machine ---- */

void l3_authorization_finalizer_predicate_cycle_materialized(
    const uint32_t local_1bc_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t parent_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t late_parent_q_3b58_words[4],
    uint32_t preserved_tail12_3b58,
    const uint32_t late_parent_q5_3c58_words[4],
    l3_authorization_finalizer_predicate_cycle_state *out) {
    if (!local_1bc_words || !local_1f0_words || !local_188_words ||
        !local_258_words || !parent_words || !late_parent_q_3b58_words ||
        !late_parent_q5_3c58_words || !out) return;

    memset(out, 0, sizeof(*out));
    memcpy(out->input_1bc_words, local_1bc_words, sizeof(out->input_1bc_words));
    memcpy(out->input_1f0_words, local_1f0_words, sizeof(out->input_1f0_words));
    memcpy(out->input_188_words, local_188_words, sizeof(out->input_188_words));
    memcpy(out->input_258_words, local_258_words, sizeof(out->input_258_words));
    memcpy(out->parent_words, parent_words, sizeof(out->parent_words));
    memcpy(out->late_parent_q_3b58_words, late_parent_q_3b58_words,
           sizeof(out->late_parent_q_3b58_words));
    memcpy(out->late_parent_q5_3c58_words, late_parent_q5_3c58_words,
           sizeof(out->late_parent_q5_3c58_words));
    out->preserved_tail12_3b58 = preserved_tail12_3b58;

    memcpy(out->output_1bc_words, local_1bc_words, sizeof(out->output_1bc_words));
    memcpy(out->output_1f0_words, local_1f0_words, sizeof(out->output_1f0_words));
    memcpy(out->output_188_words, local_188_words, sizeof(out->output_188_words));
    memcpy(out->output_258_words, local_258_words, sizeof(out->output_258_words));
    out->path = L3_AUTH_FINAL_STATE_CYCLE_NO_PROGRESS;

    l3_authorization_finalizer_terminal_gate_raw40(local_1f0_words, local_188_words, &out->terminal);
    if (out->terminal.terminal_taken) {
        out->path = L3_AUTH_FINAL_STATE_CYCLE_TERMINAL_RAW40;
        out->terminal_taken = 1u;
        out->branch_completed = 1u;
        memcpy(out->raw40, out->terminal.raw40, sizeof(out->raw40));
        return;
    }

    l3_authorization_finalizer_predicate_loop_stage12_step_materialized(
        local_1bc_words, local_1f0_words, local_188_words, local_258_words,
        parent_words, late_parent_q_3b58_words, preserved_tail12_3b58,
        &out->step_3b58);

    if (out->step_3b58.path == L3_AUTH_FINAL_STATE_STEP_3B58_TAKEN) {
        out->path = L3_AUTH_FINAL_STATE_CYCLE_3B58_STEP;
        out->branch_completed = 1u;
        memcpy(out->output_1bc_words, out->step_3b58.output_1bc_words,
               sizeof(out->output_1bc_words));
        memcpy(out->output_1f0_words, local_1f0_words, sizeof(out->output_1f0_words));
        memcpy(out->output_188_words, out->step_3b58.output_188_words,
               sizeof(out->output_188_words));
        memcpy(out->output_258_words, out->step_3b58.output_258_words,
               sizeof(out->output_258_words));
        return;
    }

    if (out->step_3b58.path == L3_AUTH_FINAL_STATE_STEP_3C58_ALTERNATE ||
        out->step_3b58.path == L3_AUTH_FINAL_STATE_STEP_3B58_NOT_APPLICABLE) {
        l3_authorization_finalizer_predicate_loop_stage15_step_materialized(
            local_1bc_words, local_1f0_words, local_188_words, local_258_words,
            parent_words, late_parent_q5_3c58_words, &out->step_3c58);
        if (out->step_3c58.path == L3_AUTH_FINAL_STATE_STEP_3C58_TAKEN) {
            out->path = L3_AUTH_FINAL_STATE_CYCLE_3C58_STEP;
            out->branch_completed = 1u;
            memcpy(out->output_1bc_words, out->step_3c58.output_1bc_words,
                   sizeof(out->output_1bc_words));
            memcpy(out->output_1f0_words, out->step_3c58.output_1f0_words,
                   sizeof(out->output_1f0_words));
            memcpy(out->output_188_words, out->step_3c58.output_188_words,
                   sizeof(out->output_188_words));
            memcpy(out->output_258_words, out->step_3c58.output_258_words,
                   sizeof(out->output_258_words));
        }
    }
}


/* ---- v141: bounded materialized predicate-network loop driver ---- */


/* ---- v142: upstream predicate preloop and bounded driver bridge ---- */

static void l3_authorization_finalizer_preloop_record_trace(
    l3_authorization_finalizer_predicate_preloop_state *out, uint32_t path) {
    if (out->trace_len < L3_AUTH_FINAL_STATE_PREDICATE_PRELOOP_MAX_TRACE) {
        out->path_trace[out->trace_len++] = path;
    }
}

static void l3_authorization_finalizer_predicate_preloop_impl(
    const uint32_t local_1bc_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    uint32_t tail_accum_39d8,
    uint32_t tail_accum_3ad8,
    const l3_authorization_finalizer_live_context *live,
    uint32_t max_1bc188_cycles,
    uint32_t max_1f0258_cycles,
    l3_authorization_finalizer_predicate_preloop_state *out) {
    if (!local_1bc_words || !local_1f0_words || !local_188_words ||
        !local_258_words || !out) return;

    memset(out, 0, sizeof(*out));
    memcpy(out->input_1bc_words, local_1bc_words, sizeof(out->input_1bc_words));
    memcpy(out->input_1f0_words, local_1f0_words, sizeof(out->input_1f0_words));
    memcpy(out->input_188_words, local_188_words, sizeof(out->input_188_words));
    memcpy(out->input_258_words, local_258_words, sizeof(out->input_258_words));
    out->tail_accum_39d8 = tail_accum_39d8;
    out->tail_accum_3ad8 = tail_accum_3ad8;
    out->max_1bc188_cycles = max_1bc188_cycles;
    out->max_1f0258_cycles = max_1f0258_cycles;
    out->stop_reason = L3_AUTH_FINAL_STATE_PRELOOP_READY_FOR_DRIVER;

    uint32_t cur_1bc[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t cur_1f0[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t cur_188[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t cur_258[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    memcpy(cur_1bc, local_1bc_words, sizeof(cur_1bc));
    memcpy(cur_1f0, local_1f0_words, sizeof(cur_1f0));
    memcpy(cur_188, local_188_words, sizeof(cur_188));
    memcpy(cur_258, local_258_words, sizeof(cur_258));

    memcpy(out->output_1bc_words, cur_1bc, sizeof(out->output_1bc_words));
    memcpy(out->output_1f0_words, cur_1f0, sizeof(out->output_1f0_words));
    memcpy(out->output_188_words, cur_188, sizeof(out->output_188_words));
    memcpy(out->output_258_words, cur_258, sizeof(out->output_258_words));

    l3_authorization_finalizer_terminal_gate_raw40(cur_1f0, cur_188, &out->terminal);
    if (out->terminal.terminal_taken) {
        out->stop_reason = L3_AUTH_FINAL_STATE_PRELOOP_TERMINAL_RAW40;
        out->terminal_taken = 1u;
        memcpy(out->raw40, out->terminal.raw40, sizeof(out->raw40));
        l3_authorization_finalizer_preloop_record_trace(out, L3_AUTH_FINAL_STATE_PRELOOP_TRACE_TERMINAL);
        return;
    }

    while (((cur_1bc[0] * 0x00005785u) & 1u) == 0u) {
        if (out->cycles_1bc188 >= max_1bc188_cycles) {
            out->stop_reason = L3_AUTH_FINAL_STATE_PRELOOP_CYCLE_LIMIT;
            goto finish;
        }

        l3_authorization_finalizer_lane_state_a_even_stage03_refresh(cur_1bc, &out->last_3918);
        if (!out->last_3918.branch_applicable) {
            break;
        }
        memcpy(cur_1bc, out->last_3918.output_words, sizeof(cur_1bc));

        if (cur_188[0] & 1u) {
            l3_authorization_finalizer_feedback_state_a_odd_stage04_refresh(cur_188, &out->last_3958);
            if (out->last_3958.branch_applicable) {
                memcpy(cur_188, out->last_3958.output_words, sizeof(cur_188));
            }
        } else {
            l3_authorization_finalizer_feedback_state_a_even_stage05_refresh_reduced_words(cur_188, &out->last_3998);
            if (out->last_3998.branch_applicable) {
                if (live) {
                    l3_authorization_finalizer_feedback_state_a_stage06_regenerate_with_adds(
                        out->last_3998.auStack_128_words, tail_accum_39d8,
                        live->add_39d8_a, live->add_39d8_b, live->add_39d8_c,
                        &out->last_39d8);
                } else {
                    l3_authorization_finalizer_feedback_state_a_stage06_regenerate_from_reduced_words(
                        out->last_3998.auStack_128_words, tail_accum_39d8,
                        &out->last_39d8);
                }
                memcpy(cur_188, out->last_39d8.local_188_words, sizeof(cur_188));
            }
        }
        out->cycles_1bc188++;
        l3_authorization_finalizer_preloop_record_trace(out, L3_AUTH_FINAL_STATE_PRELOOP_TRACE_1BC188);
    }

    while (((cur_1f0[0] * 0x0000c665u) & 1u) != 0u) {
        if (out->cycles_1f0258 >= max_1f0258_cycles) {
            out->stop_reason = L3_AUTH_FINAL_STATE_PRELOOP_CYCLE_LIMIT;
            goto finish;
        }

        l3_authorization_finalizer_lane_state_b_odd_stage07_refresh(cur_1f0, &out->last_3a18);
        if (out->last_3a18.branch_applicable) {
            memcpy(cur_1f0, out->last_3a18.output_words, sizeof(cur_1f0));
        }

        if (cur_258[0] & 1u) {
            l3_authorization_finalizer_feedback_state_b_odd_stage08_refresh(cur_258, &out->last_3a58);
            if (out->last_3a58.branch_applicable) {
                memcpy(cur_258, out->last_3a58.output_words, sizeof(cur_258));
            }
        } else {
            l3_authorization_finalizer_feedback_state_b_even_stage09_refresh_reduced_words(cur_258, &out->last_3a98);
            if (out->last_3a98.branch_applicable) {
                if (live) {
                    l3_authorization_finalizer_feedback_state_b_stage10_regenerate_with_adds(
                        out->last_3a98.auStack_128_words, tail_accum_3ad8,
                        live->add_3ad8_a, live->add_3ad8_b, live->add_3ad8_c,
                        &out->last_3ad8);
                } else {
                    l3_authorization_finalizer_feedback_state_b_stage10_regenerate_from_reduced_words(
                        out->last_3a98.auStack_128_words, tail_accum_3ad8,
                        &out->last_3ad8);
                }
                memcpy(cur_258, out->last_3ad8.local_258_words, sizeof(cur_258));
            }
        }
        out->cycles_1f0258++;
        l3_authorization_finalizer_preloop_record_trace(out, L3_AUTH_FINAL_STATE_PRELOOP_TRACE_1F0258);
    }

finish:
    memcpy(out->output_1bc_words, cur_1bc, sizeof(out->output_1bc_words));
    memcpy(out->output_1f0_words, cur_1f0, sizeof(out->output_1f0_words));
    memcpy(out->output_188_words, cur_188, sizeof(out->output_188_words));
    memcpy(out->output_258_words, cur_258, sizeof(out->output_258_words));
}



static void l3_authorization_finalizer_predicate_run_from_preloop_impl(
    const uint32_t local_1bc_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    uint32_t tail_accum_39d8,
    uint32_t tail_accum_3ad8,
    const l3_authorization_finalizer_live_context *live,
    uint32_t max_1bc188_cycles,
    uint32_t max_1f0258_cycles,
    const uint32_t parent_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t late_parent_q_3b58_words[4],
    uint32_t preserved_tail12_3b58,
    const uint32_t late_parent_q5_3c58_words[4],
    uint32_t max_predicate_cycles,
    l3_authorization_finalizer_predicate_run_from_preloop_state *out) {
    if (!local_1bc_words || !local_1f0_words || !local_188_words ||
        !local_258_words || !parent_words || !late_parent_q_3b58_words ||
        !late_parent_q5_3c58_words || !out) return;

    memset(out, 0, sizeof(*out));

    uint32_t cur_1bc[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t cur_1f0[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t cur_188[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t cur_258[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    memcpy(cur_1bc, local_1bc_words, sizeof(cur_1bc));
    memcpy(cur_1f0, local_1f0_words, sizeof(cur_1f0));
    memcpy(cur_188, local_188_words, sizeof(cur_188));
    memcpy(cur_258, local_258_words, sizeof(cur_258));

    memcpy(out->loop.input_1bc_words, local_1bc_words,
           sizeof(out->loop.input_1bc_words));
    memcpy(out->loop.input_1f0_words, local_1f0_words,
           sizeof(out->loop.input_1f0_words));
    memcpy(out->loop.input_188_words, local_188_words,
           sizeof(out->loop.input_188_words));
    memcpy(out->loop.input_258_words, local_258_words,
           sizeof(out->loop.input_258_words));
    memcpy(out->loop.parent_words, parent_words,
           sizeof(out->loop.parent_words));
    memcpy(out->loop.late_parent_q_3b58_words, late_parent_q_3b58_words,
           sizeof(out->loop.late_parent_q_3b58_words));
    memcpy(out->loop.late_parent_q5_3c58_words, late_parent_q5_3c58_words,
           sizeof(out->loop.late_parent_q5_3c58_words));
    out->loop.preserved_tail12_3b58 = preserved_tail12_3b58;
    out->loop.max_cycles = max_predicate_cycles;
    out->loop.stop_reason = L3_AUTH_FINAL_STATE_LOOP_CYCLE_LIMIT;

    for (uint32_t cycle_idx = 0;; ++cycle_idx) {
        /* Native returns to the terminal gate and then reruns both parity
           normalization loops before every 3b58/3c58 driver choice. */
        l3_authorization_finalizer_predicate_preloop_impl(
            cur_1bc, cur_1f0, cur_188, cur_258,
            tail_accum_39d8, tail_accum_3ad8,
            live,
            max_1bc188_cycles, max_1f0258_cycles, &out->preloop);

        memcpy(cur_1bc, out->preloop.output_1bc_words, sizeof(cur_1bc));
        memcpy(cur_1f0, out->preloop.output_1f0_words, sizeof(cur_1f0));
        memcpy(cur_188, out->preloop.output_188_words, sizeof(cur_188));
        memcpy(cur_258, out->preloop.output_258_words, sizeof(cur_258));
        memcpy(out->output_1bc_words, cur_1bc, sizeof(out->output_1bc_words));
        memcpy(out->output_1f0_words, cur_1f0, sizeof(out->output_1f0_words));
        memcpy(out->output_188_words, cur_188, sizeof(out->output_188_words));
        memcpy(out->output_258_words, cur_258, sizeof(out->output_258_words));
        memcpy(out->loop.output_1bc_words, cur_1bc,
               sizeof(out->loop.output_1bc_words));
        memcpy(out->loop.output_1f0_words, cur_1f0,
               sizeof(out->loop.output_1f0_words));
        memcpy(out->loop.output_188_words, cur_188,
               sizeof(out->loop.output_188_words));
        memcpy(out->loop.output_258_words, cur_258,
               sizeof(out->loop.output_258_words));

        if (out->preloop.stop_reason == L3_AUTH_FINAL_STATE_PRELOOP_TERMINAL_RAW40) {
            out->final_stop_reason = L3_AUTH_FINAL_STATE_PRELOOP_TERMINAL_RAW40;
            out->terminal_taken = 1u;
            out->loop.stop_reason = L3_AUTH_FINAL_STATE_LOOP_TERMINAL_RAW40;
            out->loop.terminal_taken = 1u;
            memcpy(out->raw40, out->preloop.raw40, sizeof(out->raw40));
            memcpy(out->loop.raw40, out->preloop.raw40,
                   sizeof(out->loop.raw40));
            return;
        }
        if (out->preloop.stop_reason != L3_AUTH_FINAL_STATE_PRELOOP_READY_FOR_DRIVER) {
            out->final_stop_reason = out->preloop.stop_reason;
            return;
        }
        if (cycle_idx >= max_predicate_cycles) {
            out->final_stop_reason =
                0x100u + L3_AUTH_FINAL_STATE_LOOP_CYCLE_LIMIT;
            return;
        }

        l3_authorization_finalizer_predicate_cycle_materialized(
            cur_1bc, cur_1f0, cur_188, cur_258,
            parent_words, late_parent_q_3b58_words,
            preserved_tail12_3b58, late_parent_q5_3c58_words,
            &out->loop.last_cycle);
        out->loop.cycles_executed = cycle_idx + 1u;
        if (out->loop.trace_len < L3_AUTH_FINAL_STATE_PREDICATE_LOOP_MAX_TRACE) {
            out->loop.path_trace[out->loop.trace_len++] =
                out->loop.last_cycle.path;
        }
        if (out->loop.last_cycle.path == L3_AUTH_FINAL_STATE_CYCLE_3B58_STEP) {
            out->loop.seen_3b58_cycle = 1u;
            out->loop.last_3b58_cycle = out->loop.last_cycle;
        } else if (out->loop.last_cycle.path == L3_AUTH_FINAL_STATE_CYCLE_3C58_STEP) {
            out->loop.seen_3c58_cycle = 1u;
            out->loop.last_3c58_cycle = out->loop.last_cycle;
        }

        if (out->loop.last_cycle.terminal_taken) {
            out->final_stop_reason =
                0x100u + L3_AUTH_FINAL_STATE_LOOP_TERMINAL_RAW40;
            out->terminal_taken = 1u;
            out->loop.stop_reason = L3_AUTH_FINAL_STATE_LOOP_TERMINAL_RAW40;
            out->loop.terminal_taken = 1u;
            memcpy(out->raw40, out->loop.last_cycle.raw40,
                   sizeof(out->raw40));
            memcpy(out->loop.raw40, out->loop.last_cycle.raw40,
                   sizeof(out->loop.raw40));
            return;
        }
        if (!out->loop.last_cycle.branch_completed ||
            out->loop.last_cycle.path == L3_AUTH_FINAL_STATE_CYCLE_NO_PROGRESS) {
            out->final_stop_reason =
                0x100u + L3_AUTH_FINAL_STATE_LOOP_NO_PROGRESS;
            out->loop.stop_reason = L3_AUTH_FINAL_STATE_LOOP_NO_PROGRESS;
            return;
        }

        memcpy(cur_1bc, out->loop.last_cycle.output_1bc_words,
               sizeof(cur_1bc));
        memcpy(cur_1f0, out->loop.last_cycle.output_1f0_words,
               sizeof(cur_1f0));
        memcpy(cur_188, out->loop.last_cycle.output_188_words,
               sizeof(cur_188));
        memcpy(cur_258, out->loop.last_cycle.output_258_words,
               sizeof(cur_258));
    }
}



/* ---- v143: constructor bridge from post-v122/v124 state into predicate network ---- */

void l3_authorization_finalizer_predicate_initial_state_construct(
    const uint32_t local_35c[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_3c4[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t parent_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_predicate_initial_state *out) {
    if (!local_35c || !local_3c4 || !parent_words || !out) return;
    memset(out, 0, sizeof(*out));
    memcpy(out->input_local_35c, local_35c, sizeof(out->input_local_35c));
    memcpy(out->input_local_3c4, local_3c4, sizeof(out->input_local_3c4));
    memcpy(out->input_parent_words, parent_words, sizeof(out->input_parent_words));

    l3_authorization_finalizer_vector_affine_setup(local_35c, local_3c4, &out->vector_setup);
    l3_authorization_finalizer_parent_context_stage01_seed(parent_words, &out->parent3898);
    l3_authorization_finalizer_seed_state_stage02_seed(out->parent3898.auStack_128_words, &out->local_b8_38d8);

    memcpy(&out->local_188_words[0], out->vector_setup.local_188_head, 4u * sizeof(uint32_t));
    memcpy(&out->local_188_words[4], out->vector_setup.local_34c_affine, 4u * sizeof(uint32_t));
    memcpy(&out->local_188_words[8], out->vector_setup.local_33c_affine, 4u * sizeof(uint32_t));
    out->local_188_words[12] = out->vector_setup.local_158;

    memcpy(&out->local_1bc_words[0], out->vector_setup.local_3c4_affine, 4u * sizeof(uint32_t));
    memcpy(&out->local_1bc_words[4], out->vector_setup.local_3b4_affine, 4u * sizeof(uint32_t));
    memcpy(&out->local_1bc_words[8], out->vector_setup.local_3a4_affine, 4u * sizeof(uint32_t));
    out->local_1bc_words[12] = out->vector_setup.local_18c;

    memcpy(out->local_258_words, out->vector_setup.local_258_seed, sizeof(out->local_258_words));
    memcpy(out->local_b8_words, out->local_b8_38d8.local_b8_words, sizeof(out->local_b8_words));

    static const uint32_t parent_first_mul[4] = {
        0x8301c885u, 0x491ee3bbu, 0xe570f379u, 0xe7021607u
    };
    static const uint32_t parent_first_add[4] = {
        0xbe3d9c55u, 0xa313168bu, 0x17249598u, 0x6ead46d9u
    };
    for (unsigned i = 0; i < 4u; ++i) {
        out->local_1f0_words[i] =
            parent_words[i] * parent_first_mul[i] + parent_first_add[i];
    }
    out->local_1f0_words[4] = parent_words[4] * 0x0c49db87u + 0xf67c2e29u;
    out->local_1f0_words[5] = parent_words[5] * 0x182a1abfu + 0xb10a7dcfu;
    out->local_1f0_words[6] = parent_words[6] * 0x6844cac3u + 0x59de200cu;

    static const uint32_t parent_late_mul[4] = {
        0xe1b647ddu, 0x8301c885u, 0x491ee3bbu, 0xe570f379u
    };
    static const uint32_t parent_late_add[4] = {
        0x66304416u, 0xbe3d9c55u, 0xa313168bu, 0x17249598u
    };
    for (unsigned i = 0; i < 4u; ++i) {
        out->local_1f0_words[7u + i] =
            parent_words[7u + i] * parent_late_mul[i] + parent_late_add[i];
    }
    out->local_1f0_words[11] = parent_words[11] * 0xe7021607u + 0x6ead46d9u;
    out->local_1f0_words[12] = parent_words[12] * 0x0c49db87u + 0xf67c2e29u;
}


void l3_authorization_finalizer_predicate_run_from_constructor_derived(
    const uint32_t local_35c[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_3c4[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t parent_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    uint32_t max_1bc188_cycles,
    uint32_t max_1f0258_cycles,
    uint32_t max_predicate_cycles,
    l3_authorization_finalizer_live_context *live_out,
    l3_authorization_finalizer_predicate_run_from_constructor_state *out) {
    l3_authorization_finalizer_live_context live;
    if (!local_35c || !local_3c4 || !parent_words || !out) return;
    memset(out, 0, sizeof(*out));
    memset(&live, 0, sizeof(live));

    l3_authorization_finalizer_predicate_initial_state_construct(
        local_35c, local_3c4, parent_words, &out->initial);
    l3_authorization_finalizer_live_context_derive(
        parent_words, out->initial.local_b8_words,
        out->initial.local_188_words, &live);

    l3_authorization_finalizer_predicate_run_from_preloop_impl(
        out->initial.local_1bc_words,
        out->initial.local_1f0_words,
        out->initial.local_188_words,
        out->initial.local_258_words,
        live.tail_accum_39d8,
        live.tail_accum_3ad8,
        &live,
        max_1bc188_cycles,
        max_1f0258_cycles,
        parent_words,
        live.late_parent_q_3b58_words,
        live.preserved_tail12_3b58,
        live.late_parent_q5_3c58_words,
        max_predicate_cycles,
        &out->run);

    if (live_out) *live_out = live;
    out->final_stop_reason = out->run.final_stop_reason;
    out->terminal_taken = out->run.terminal_taken;
    memcpy(out->raw40, out->run.raw40, sizeof(out->raw40));
    memcpy(out->output_1bc_words, out->run.output_1bc_words,
           sizeof(out->output_1bc_words));
    memcpy(out->output_1f0_words, out->run.output_1f0_words,
           sizeof(out->output_1f0_words));
    memcpy(out->output_188_words, out->run.output_188_words,
           sizeof(out->output_188_words));
    memcpy(out->output_258_words, out->run.output_258_words,
           sizeof(out->output_258_words));
}

/* ---- v144: post-convolution reduction bridge into predicate constructor ---- */




/* ---- v145: post-local_390 convolution -> postconv loop -> predicate bridge ---- */


void l3_authorization_finalizer_post_convolution_convolution_to_windows(
    const l3_authorization_finalizer_post_convolution_convolution_state *conv,
    l3_authorization_finalizer_post_convolution_convolution_windows_state *out) {
    if (!conv || !out) return;
    memset(out, 0, sizeof(*out));

    /* The native stack layout is a single 26-pair footprint beginning at
       &local_328.  The named windows used by the following loop are aliases at
       fixed pair offsets inside that same footprint. */
    for (unsigned i = 0; i < L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS; ++i) {
        out->arrays.local_328[i] = conv->local_328[i + 0u];
        out->arrays.auStack_2f0[i] = conv->local_328[i + 7u];
    }
    for (unsigned i = 0; i < L3_AUTH_FINAL_STATE_FIRST_LOOP_WINDOW_PAIRS; ++i) {
        out->arrays.uStack_320[i] = conv->local_328[i + 1u];
        out->arrays.local_310[i] = conv->local_328[i + 3u];
        out->arrays.uStack_300[i] = conv->local_328[i + 5u];
        out->arrays.auStack_2e8[i] = conv->local_328[i + 8u];
        out->arrays.auStack_2d8[i] = conv->local_328[i + 10u];
        out->arrays.auStack_2c8[i] = conv->local_328[i + 12u];
    }
}

static void l3_authorization_finalizer_make_postconv_sources_from_parent(
    const uint32_t parent_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_pair_array13 *sources) {
    uint32_t words[L3_AUTH_FINAL_STATE_SEED_WORD_COUNT];
    memset(words, 0, sizeof(words));
    l3_authorization_finalizer_postconv_seed_param1_pairs(parent_words, words);
    for (unsigned i = 0; i < L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS; ++i) {
        sources->pair[i].lo = words[i * 2u + 0u];
        sources->pair[i].hi = words[i * 2u + 1u];
    }
}



/* ---- v146: second-loop reduction -> post_convolution bridge --------------------- */





/* ---- v147: first-loop reduction -> second-loop bridge ------------------ */

void l3_authorization_finalizer_param3_seed26_to_second_loop_windows(
    const uint32_t seed26_words[L3_AUTH_FINAL_STATE_SEED_FULL_WORD_COUNT],
    l3_authorization_finalizer_first_loop_arrays *out) {
    if (!seed26_words || !out) return;
    memset(out, 0, sizeof(*out));

    l3_auth_pair32 footprint[L3_AUTH_FINAL_STATE_SEED_FULL_PAIR_COUNT];
    for (unsigned i = 0; i < L3_AUTH_FINAL_STATE_SEED_FULL_PAIR_COUNT; ++i) {
        footprint[i].lo = seed26_words[i * 2u + 0u];
        footprint[i].hi = seed26_words[i * 2u + 1u];
    }

    /* v107 writes the same contiguous &local_328 26-pair stack footprint that
       later loops address through overlapping local_328/uStack_320/... roots. */
    for (unsigned i = 0; i < L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS; ++i) {
        out->local_328[i] = footprint[i + 0u];
        out->auStack_2f0[i] = footprint[i + 7u];
    }
    for (unsigned i = 0; i < L3_AUTH_FINAL_STATE_FIRST_LOOP_WINDOW_PAIRS; ++i) {
        out->uStack_320[i] = footprint[i + 1u];
        out->local_310[i] = footprint[i + 3u];
        out->uStack_300[i] = footprint[i + 5u];
        out->auStack_2e8[i] = footprint[i + 8u];
        out->auStack_2d8[i] = footprint[i + 10u];
        out->auStack_2c8[i] = footprint[i + 12u];
    }
}

void l3_authorization_finalizer_second_loop_sources_from_parent(
    const uint32_t parent_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_pair_array13 *out) {
    if (!parent_words || !out) return;
    memset(out, 0, sizeof(*out));
    uint32_t words[L3_AUTH_FINAL_STATE_SEED_WORD_COUNT];
    l3_authorization_finalizer_seed_param1_second_pairs(parent_words, words);
    for (unsigned i = 0; i < L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS; ++i) {
        out->pair[i].lo = words[i * 2u + 0u];
        out->pair[i].hi = words[i * 2u + 1u];
    }
}



/* ---- v148: initial seed loops -> first-loop bridge --------------------- */

void l3_authorization_finalizer_seed_param2_pairs26(
    const uint32_t param2_words[L3_AUTH_FINAL_STATE_SEED_PAIR_COUNT],
    uint32_t out_words[L3_AUTH_FINAL_STATE_SEED_FULL_WORD_COUNT]) {
    if (!param2_words || !out_words) return;
    l3_authorization_finalizer_seed_param2_pairs(param2_words, out_words);
    for (unsigned i = L3_AUTH_FINAL_STATE_SEED_PAIR_COUNT;
         i < L3_AUTH_FINAL_STATE_SEED_FULL_PAIR_COUNT; ++i) {
        out_words[i * 2u + 0u] = 0xde7a2aeau;
        out_words[i * 2u + 1u] = 0x3433ae2eu;
    }
}

void l3_authorization_finalizer_param2_seed26_to_first_loop_windows(
    const uint32_t seed26_words[L3_AUTH_FINAL_STATE_SEED_FULL_WORD_COUNT],
    l3_authorization_finalizer_first_loop_arrays *out) {
    if (!seed26_words || !out) return;
    memset(out, 0, sizeof(*out));

    l3_auth_pair32 footprint[L3_AUTH_FINAL_STATE_SEED_FULL_PAIR_COUNT];
    for (unsigned i = 0; i < L3_AUTH_FINAL_STATE_SEED_FULL_PAIR_COUNT; ++i) {
        footprint[i].lo = seed26_words[i * 2u + 0u];
        footprint[i].hi = seed26_words[i * 2u + 1u];
    }

    for (unsigned i = 0; i < L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS; ++i) {
        out->local_328[i] = footprint[i + 0u];
        out->auStack_2f0[i] = footprint[i + 7u];
    }
    for (unsigned i = 0; i < L3_AUTH_FINAL_STATE_FIRST_LOOP_WINDOW_PAIRS; ++i) {
        out->uStack_320[i] = footprint[i + 1u];
        out->local_310[i] = footprint[i + 3u];
        out->uStack_300[i] = footprint[i + 5u];
        out->auStack_2e8[i] = footprint[i + 8u];
        out->auStack_2d8[i] = footprint[i + 10u];
        out->auStack_2c8[i] = footprint[i + 12u];
    }
}

void l3_authorization_finalizer_first_loop_sources_from_parent(
    const uint32_t parent_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_pair_array13 *out) {
    if (!parent_words || !out) return;
    memset(out, 0, sizeof(*out));
    uint32_t words[L3_AUTH_FINAL_STATE_SEED_WORD_COUNT];
    l3_authorization_finalizer_seed_param1_pairs(parent_words, words);
    for (unsigned i = 0; i < L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS; ++i) {
        out->pair[i].lo = words[i * 2u + 0u];
        out->pair[i].hi = words[i * 2u + 1u];
    }
}



/* ---- v149: single materialized FUN_f3fd4a60 core-state wrapper -------- */


/* ---- v150: native caller/context adapter ------------------------------ */






/* ---- v151: native carry-input resolver from local_328 streams --------- */









/* ---- v152: native-context run with resolved local_328 stream carries ---- */



/* ---- v153: native stream-provider adapter ----------------------------- */





/* ---- v154: stream-provider extraction from concrete producer states ---- */






/* ---- v155: top-level native-run producer-provider bridge -------------- */





/* ---- v156: sibling-provider probe bridge ----------------------------- */






/* ---- v157: native 3c58-provider witness bridge ---------------------- */


/* ---- v158: complete native-provider replay comparison ---------------- */







/* ---- v159: exact derived FUN_f3fd4a60 entry --------------------------- */

typedef struct {
    uint32_t param2_seed26[L3_AUTH_FINAL_STATE_SEED_FULL_WORD_COUNT];
    l3_authorization_finalizer_first_loop_arrays first_initial;
    l3_authorization_finalizer_pair_array13 first_sources;
    l3_authorization_finalizer_first_loop_run13_state first_loop;
    l3_authorization_finalizer_reduce3130_state reduce3130;

    uint32_t param3_seed26[L3_AUTH_FINAL_STATE_SEED_FULL_WORD_COUNT];
    l3_authorization_finalizer_first_loop_arrays second_initial;
    l3_authorization_finalizer_pair_array13 second_sources;
    l3_authorization_finalizer_second_loop_run13_state second_loop;
    l3_authorization_finalizer_reduce3430_state reduce3430;

    l3_authorization_finalizer_post_convolution_seed_state post_convolution_seed;
    l3_authorization_finalizer_post_convolution_convolution_state post_convolution_conv;
    l3_authorization_finalizer_post_convolution_convolution_windows_state postconv_windows;
    l3_authorization_finalizer_pair_array13 postconv_sources;
    l3_authorization_finalizer_postconv_loop_run13_state postconv_loop;
    l3_authorization_finalizer_reduce37b0_state reduce37b0;

    l3_authorization_finalizer_predicate_run_from_constructor_state predicate;
} l3_authorization_finalizer_derived_workspace;

int l3_authorization_finalize_round_result(
    const uint32_t param2_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t param3_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t parent_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    uint32_t parent_pair_lo,
    uint32_t parent_pair_hi,
    uint32_t max_1bc188_cycles,
    uint32_t max_1f0258_cycles,
    uint32_t max_predicate_cycles,
    l3_authorization_final_result *out) {
    if (!param2_words || !param3_words || !parent_words || !out) return -1;
    memset(out, 0, sizeof(*out));

    l3_authorization_finalizer_derived_workspace *ws =
        (l3_authorization_finalizer_derived_workspace *)calloc(1u, sizeof(*ws));
    if (!ws) return -2;

    l3_authorization_finalizer_seed_param2_pairs26(param2_words, ws->param2_seed26);
    l3_authorization_finalizer_param2_seed26_to_first_loop_windows(
        ws->param2_seed26, &ws->first_initial);
    l3_authorization_finalizer_first_loop_sources_from_parent(
        parent_words, &ws->first_sources);
    l3_authorization_finalizer_first_loop_run13_exact_sources(
        parent_pair_lo, parent_pair_hi,
        &ws->first_sources, &ws->first_initial, &ws->first_loop);
    l3_authorization_finalizer_reduce_workspace_first(
        ws->first_loop.arrays.auStack_2c8, &ws->reduce3130);

    l3_authorization_finalizer_seed_param3_pairs26(param3_words, ws->param3_seed26);
    l3_authorization_finalizer_param3_seed26_to_second_loop_windows(
        ws->param3_seed26, &ws->second_initial);
    l3_authorization_finalizer_second_loop_sources_from_parent(
        parent_words, &ws->second_sources);
    l3_authorization_finalizer_second_loop_run13_exact_sources(
        parent_pair_lo, parent_pair_hi,
        &ws->second_sources, &ws->second_initial, &ws->second_loop);
    l3_authorization_finalizer_reduce_workspace_second(
        ws->second_loop.arrays.auStack_2c8, &ws->reduce3430);

    l3_authorization_finalizer_post_convolution_seed_staging(
        param3_words, ws->reduce3430.local_390, &ws->post_convolution_seed);
    l3_authorization_finalizer_post_convolution_convolution_range_correct(
        &ws->post_convolution_seed, &ws->post_convolution_conv);
    l3_authorization_finalizer_post_convolution_convolution_to_windows(
        &ws->post_convolution_conv, &ws->postconv_windows);
    l3_authorization_finalizer_make_postconv_sources_from_parent(
        parent_words, &ws->postconv_sources);
    l3_authorization_finalizer_postconv_loop_run13_exact_sources(
        parent_pair_lo, parent_pair_hi,
        &ws->postconv_sources, &ws->postconv_windows.arrays,
        &ws->postconv_loop);
    l3_authorization_finalizer_reduce_workspace_post_convolution(
        ws->postconv_loop.arrays.auStack_2c8, &ws->reduce37b0);

    l3_authorization_finalizer_predicate_run_from_constructor_derived(
        ws->reduce3130.local_35c,
        ws->reduce37b0.local_3c4,
        parent_words,
        max_1bc188_cycles,
        max_1f0258_cycles,
        max_predicate_cycles,
        &out->live,
        &ws->predicate);

    out->final_stop_reason = ws->predicate.final_stop_reason;
    out->terminal_taken = ws->predicate.terminal_taken;
    out->predicate_cycles = ws->predicate.run.loop.cycles_executed;
    memcpy(out->raw40, ws->predicate.raw40, sizeof(out->raw40));
    free(ws);
    return out->terminal_taken ? 0 : -3;
}
