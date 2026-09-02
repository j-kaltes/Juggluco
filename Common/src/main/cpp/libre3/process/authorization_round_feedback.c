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

#include "authorization_round_feedback.h"

static uint32_t affine_transform_u32(uint32_t x, uint32_t mul, uint32_t add) {
    return x * mul + add;
}

typedef struct l3_round_output_affine_spec {
    uint32_t mul_a[4];
    uint32_t add_a[4];
    uint32_t mul_b[4];
    uint32_t add_b[4];
} l3_round_output_affine_spec;

static const l3_round_output_affine_spec k_output_affine_specs[3] = {
    {
        {0xbe637357u, 0xb13e3cf1u, 0x2b89db9bu, 0x745a79ebu},
        {0x8d5384dau, 0xb0fc5d5du, 0x5258fe36u, 0x5e88cf7eu},
        {0x1bcf873du, 0xdfdfee13u, 0xd6b5eeb1u, 0xfe48f001u},
        {0x9758affau, 0xf437a370u, 0xb90730b2u, 0x0bcaeb52u}
    },
    {
        {0xc203b9b7u, 0x2ab0861bu, 0xb27b8191u, 0xa11a3661u},
        {0x5d45b576u, 0x1750acccu, 0x6d616e5eu, 0xb4f90343u},
        {0x46b3c5e5u, 0x0f36e889u, 0x5ab50ec3u, 0xdb02388du},
        {0x97cd793fu, 0xbc0dc7e7u, 0x3d316219u, 0x716e7356u}
    },
    {
        {0xa6d644ffu, 0xe98c4d85u, 0x2c79b3e5u, 0xf4fa24abu},
        {0x507cc6efu, 0xbe589156u, 0x60b8041du, 0xb4cb6929u},
        {0x821f2c69u, 0x24b9ee85u, 0xdd4d41c1u, 0xe3a3e88du},
        {0x266d29bfu, 0x6251ff00u, 0x7d8da2e2u, 0xa915becdu}
    }
};

static int materialize_round_output_by_spec(
    const uint32_t vector_feedback_words13[13],
    uint32_t out_words13[13],
    const l3_round_output_affine_spec *spec) {
    if (!vector_feedback_words13 || !out_words13 || !spec) return -1;
    for (unsigned i = 0; i < 4u; ++i) {
        out_words13[i] = affine_transform_u32(vector_feedback_words13[i], spec->mul_a[i], spec->add_a[i]);
        out_words13[4u + i] = affine_transform_u32(vector_feedback_words13[4u + i], spec->mul_b[i], spec->add_b[i]);
        out_words13[8u + i] = affine_transform_u32(vector_feedback_words13[8u + i], spec->mul_a[i], spec->add_a[i]);
    }
    out_words13[12] = affine_transform_u32(vector_feedback_words13[12], spec->mul_b[0], spec->add_b[0]);
    return 0;
}

int l3_authorization_materialize_round_output_a(
    const uint32_t vector_feedback_words13[13],
    uint32_t out_param5_words13[13]) {
    return materialize_round_output_by_spec(vector_feedback_words13, out_param5_words13,
                                            &k_output_affine_specs[0]);
}

int l3_authorization_materialize_round_output_b(
    const uint32_t vector_feedback_words13[13],
    uint32_t out_param6_words13[13]) {
    return materialize_round_output_by_spec(vector_feedback_words13, out_param6_words13,
                                            &k_output_affine_specs[1]);
}

int l3_authorization_materialize_round_output_c(
    const uint32_t vector_feedback_words13[13],
    uint32_t out_param7_words13[13]) {
    return materialize_round_output_by_spec(vector_feedback_words13, out_param7_words13,
                                            &k_output_affine_specs[2]);
}
