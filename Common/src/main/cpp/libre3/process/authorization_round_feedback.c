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
#include "authorization_round_internal.h"

#include <string.h>

static uint32_t affine_transform_u32(uint32_t x, uint32_t mul, uint32_t add) {
    return x * mul + add;
}

static l3_authorization_vector_feedback_pair32 pair_from_half(uint64_t h) {
    l3_authorization_vector_feedback_pair32 p;
    p.lo = (uint32_t)h;
    p.hi = (uint32_t)(h >> 32);
    return p;
}

int l3_authorization_materialize_round_output_a(
    const uint32_t vector_feedback_words13[13],
    uint32_t out_param5_words13[13]) {
    if (!vector_feedback_words13 || !out_param5_words13) return -1;

    /* Native param_5 block after the FUN_f3fdd33c call at f3fbefc8.
     * The assembly at f3fbefcc..f3fbf060 loads multiplier/add vectors from
     * DAT_f3fbf3b0/DAT_f3fbf3e0 for lanes 0..3 and 8..11, and from
     * DAT_f3fbf3f0/DAT_f3fbf400 for lanes 4..7.  Lane 12 is scalar.
     */
    static const uint32_t mul_a[4] = {
        0xbe637357u, 0xb13e3cf1u, 0x2b89db9bu, 0x745a79ebu
    };
    static const uint32_t add_a[4] = {
        0x8d5384dau, 0xb0fc5d5du, 0x5258fe36u, 0x5e88cf7eu
    };
    static const uint32_t mul_b[4] = {
        0x1bcf873du, 0xdfdfee13u, 0xd6b5eeb1u, 0xfe48f001u
    };
    static const uint32_t add_b[4] = {
        0x9758affau, 0xf437a370u, 0xb90730b2u, 0x0bcaeb52u
    };

    for (unsigned i = 0; i < 4u; ++i) {
        out_param5_words13[i] = affine_transform_u32(vector_feedback_words13[i], mul_a[i], add_a[i]);
        out_param5_words13[4u + i] = affine_transform_u32(vector_feedback_words13[4u + i], mul_b[i], add_b[i]);
        out_param5_words13[8u + i] = affine_transform_u32(vector_feedback_words13[8u + i], mul_a[i], add_a[i]);
    }
    out_param5_words13[12] =
        affine_transform_u32(vector_feedback_words13[12], 0x1bcf873du, 0x9758affau);
    return 0;
}

int l3_authorization_apply_feedback_output_a(
    const int32_t seed_words13[13],
    uint32_t native_param2_unused,
    uint32_t scalar0,
    uint32_t scalar1,
    const uint64_t local478_halves13[13][2],
    uint32_t out_param5_words13[13],
    l3_authorization_feedback_output_a_trace *trace) {
    if (!seed_words13 || !local478_halves13 || !out_param5_words13) return -1;

    l3_authorization_feedback_output_a_trace tmp;
    l3_authorization_feedback_output_a_trace *dst = trace ? trace : &tmp;
    memset(dst, 0, sizeof(*dst));
    dst->native_param2_unused = native_param2_unused;

    l3_authorization_vector_feedback_pair32 state26[26];
    for (unsigned lane = 0; lane < 13u; ++lane) {
        dst->input_local478_halves13[lane][0] = local478_halves13[lane][0];
        dst->input_local478_halves13[lane][1] = local478_halves13[lane][1];
        state26[2u * lane] = pair_from_half(local478_halves13[lane][0]);
        state26[2u * lane + 1u] = pair_from_half(local478_halves13[lane][1]);
    }
    memcpy(dst->state_before26, state26, sizeof(state26));

    int rc = l3_authorization_vector_feedback_run13_update_export(
        seed_words13, scalar0, scalar1, state26, dst->vector_feedback_words13,
        &dst->vector_feedback_trace);
    if (rc != 0) return rc;
    memcpy(dst->state_after26, state26, sizeof(state26));

    rc = l3_authorization_materialize_round_output_a(dst->vector_feedback_words13,
                                                       out_param5_words13);
    if (rc != 0) return rc;
    memcpy(dst->param5_words13, out_param5_words13, sizeof(dst->param5_words13));

    (void)native_param2_unused;
    return 0;
}


/* ---- v310: second FUN_f3fdd33c binding / immediate param_6 export ---- */

int l3_authorization_materialize_round_output_b(
    const uint32_t vector_feedback_words13[13],
    uint32_t out_param6_words13[13]) {
    if (!vector_feedback_words13 || !out_param6_words13) return -1;

    /* Native block at f3fc0bf8..f3fc0c8c.  It mirrors the first export
     * materialization, but uses the second constant family and writes to
     * caller param_6.
     */
    static const uint32_t mul_a[4] = {
        0xc203b9b7u, 0x2ab0861bu, 0xb27b8191u, 0xa11a3661u
    };
    static const uint32_t add_a[4] = {
        0x5d45b576u, 0x1750acccu, 0x6d616e5eu, 0xb4f90343u
    };
    static const uint32_t mul_b[4] = {
        0x46b3c5e5u, 0x0f36e889u, 0x5ab50ec3u, 0xdb02388du
    };
    static const uint32_t add_b[4] = {
        0x97cd793fu, 0xbc0dc7e7u, 0x3d316219u, 0x716e7356u
    };

    for (unsigned i = 0; i < 4u; ++i) {
        out_param6_words13[i] = affine_transform_u32(vector_feedback_words13[i], mul_a[i], add_a[i]);
        out_param6_words13[4u + i] = affine_transform_u32(vector_feedback_words13[4u + i], mul_b[i], add_b[i]);
        out_param6_words13[8u + i] = affine_transform_u32(vector_feedback_words13[8u + i], mul_a[i], add_a[i]);
    }
    out_param6_words13[12] = affine_transform_u32(vector_feedback_words13[12], 0x46b3c5e5u, 0x97cd793fu);
    return 0;
}

int l3_authorization_apply_feedback_output_b(
    const int32_t seed_words13[13],
    uint32_t native_param2_unused,
    uint32_t scalar0,
    uint32_t scalar1,
    const uint64_t local478_halves13[13][2],
    uint32_t out_param6_words13[13],
    l3_authorization_feedback_output_b_trace *trace) {
    if (!seed_words13 || !local478_halves13 || !out_param6_words13) return -1;

    l3_authorization_feedback_output_b_trace tmp;
    l3_authorization_feedback_output_b_trace *dst = trace ? trace : &tmp;
    memset(dst, 0, sizeof(*dst));
    dst->native_param2_unused = native_param2_unused;

    l3_authorization_vector_feedback_pair32 state26[26];
    for (unsigned lane = 0; lane < 13u; ++lane) {
        dst->input_local478_halves13[lane][0] = local478_halves13[lane][0];
        dst->input_local478_halves13[lane][1] = local478_halves13[lane][1];
        state26[2u * lane] = pair_from_half(local478_halves13[lane][0]);
        state26[2u * lane + 1u] = pair_from_half(local478_halves13[lane][1]);
    }
    memcpy(dst->state_before26, state26, sizeof(state26));

    int rc = l3_authorization_vector_feedback_run13_update_export(
        seed_words13, scalar0, scalar1, state26, dst->vector_feedback_words13,
        &dst->vector_feedback_trace);
    if (rc != 0) return rc;
    memcpy(dst->state_after26, state26, sizeof(state26));

    rc = l3_authorization_materialize_round_output_b(dst->vector_feedback_words13,
                                                       out_param6_words13);
    if (rc != 0) return rc;
    memcpy(dst->param6_words13, out_param6_words13, sizeof(dst->param6_words13));

    (void)native_param2_unused;
    return 0;
}


/* ---- v313: third FUN_f3fdd33c binding / immediate param_7 export ---- */

int l3_authorization_materialize_round_output_c(
    const uint32_t vector_feedback_words13[13],
    uint32_t out_param7_words13[13]) {
    if (!vector_feedback_words13 || !out_param7_words13) return -1;

    /* Native block after the third FUN_f3fdd33c call.  It uses the same
     * multiplier/add vector family as the project-extracted v307 param_5
     * materializer, but writes to caller param_7.
     */
    static const uint32_t mul_a[4] = {
        0xa6d644ffu, 0xe98c4d85u, 0x2c79b3e5u, 0xf4fa24abu
    };
    static const uint32_t add_a[4] = {
        0x507cc6efu, 0xbe589156u, 0x60b8041du, 0xb4cb6929u
    };
    static const uint32_t mul_b[4] = {
        0x821f2c69u, 0x24b9ee85u, 0xdd4d41c1u, 0xe3a3e88du
    };
    static const uint32_t add_b[4] = {
        0x266d29bfu, 0x6251ff00u, 0x7d8da2e2u, 0xa915becdu
    };

    for (unsigned i = 0; i < 4u; ++i) {
        out_param7_words13[i] = affine_transform_u32(vector_feedback_words13[i], mul_a[i], add_a[i]);
        out_param7_words13[4u + i] = affine_transform_u32(vector_feedback_words13[4u + i], mul_b[i], add_b[i]);
        out_param7_words13[8u + i] = affine_transform_u32(vector_feedback_words13[8u + i], mul_a[i], add_a[i]);
    }
    out_param7_words13[12] = affine_transform_u32(vector_feedback_words13[12], 0x821f2c69u, 0x266d29bfu);
    return 0;
}

int l3_authorization_apply_feedback_output_c(
    const int32_t seed_words13[13],
    uint32_t native_param2_unused,
    uint32_t scalar0,
    uint32_t scalar1,
    const uint64_t local478_halves13[13][2],
    uint32_t out_param7_words13[13],
    l3_authorization_feedback_output_c_trace *trace) {
    if (!seed_words13 || !local478_halves13 || !out_param7_words13) return -1;

    l3_authorization_feedback_output_c_trace tmp;
    l3_authorization_feedback_output_c_trace *dst = trace ? trace : &tmp;
    memset(dst, 0, sizeof(*dst));
    dst->native_param2_unused = native_param2_unused;

    l3_authorization_vector_feedback_pair32 state26[26];
    for (unsigned lane = 0; lane < 13u; ++lane) {
        dst->input_local478_halves13[lane][0] = local478_halves13[lane][0];
        dst->input_local478_halves13[lane][1] = local478_halves13[lane][1];
        state26[2u * lane] = pair_from_half(local478_halves13[lane][0]);
        state26[2u * lane + 1u] = pair_from_half(local478_halves13[lane][1]);
    }
    memcpy(dst->state_before26, state26, sizeof(state26));

    int rc = l3_authorization_vector_feedback_run13_update_export(
        seed_words13, scalar0, scalar1, state26, dst->vector_feedback_words13,
        &dst->vector_feedback_trace);
    if (rc != 0) return rc;
    memcpy(dst->state_after26, state26, sizeof(state26));

    rc = l3_authorization_materialize_round_output_c(dst->vector_feedback_words13,
                                                       out_param7_words13);
    if (rc != 0) return rc;
    memcpy(dst->param7_words13, out_param7_words13, sizeof(dst->param7_words13));

    (void)native_param2_unused;
    return 0;
}

/* ---- v314: terminal param_5/param_6/param_7 export composition ---- */

int l3_authorization_apply_feedback_outputs(
    const int32_t seed_words13[13],
    uint32_t native_param2_unused,
    uint32_t scalar1_unused,
    const uint64_t param5_local478_halves13[13][2],
    uint32_t param5_scalar0,
    const uint64_t param6_local478_halves13[13][2],
    uint32_t param6_scalar0,
    const uint64_t param7_local478_halves13[13][2],
    uint32_t param7_scalar0,
    uint32_t out_param5_words13[13],
    uint32_t out_param6_words13[13],
    uint32_t out_param7_words13[13],
    l3_authorization_feedback_outputs_trace *trace) {
    if (!seed_words13 || !param5_local478_halves13 || !param6_local478_halves13 ||
        !param7_local478_halves13 || !out_param5_words13 || !out_param6_words13 ||
        !out_param7_words13) {
        return -1;
    }

    l3_authorization_feedback_outputs_trace tmp;
    l3_authorization_feedback_outputs_trace *dst = trace ? trace : &tmp;
    memset(dst, 0, sizeof(*dst));
    dst->native_param2_unused = native_param2_unused;
    dst->scalar1_unused = scalar1_unused;
    dst->scalar0_param5 = param5_scalar0;
    dst->scalar0_param6 = param6_scalar0;
    dst->scalar0_param7 = param7_scalar0;

    int rc = l3_authorization_apply_feedback_output_a(
        seed_words13, native_param2_unused, param5_scalar0, scalar1_unused,
        param5_local478_halves13, out_param5_words13, &dst->param5_trace);
    if (rc != 0) return rc;

    rc = l3_authorization_apply_feedback_output_b(
        seed_words13, native_param2_unused, param6_scalar0, scalar1_unused,
        param6_local478_halves13, out_param6_words13, &dst->param6_trace);
    if (rc != 0) return rc;

    rc = l3_authorization_apply_feedback_output_c(
        seed_words13, native_param2_unused, param7_scalar0, scalar1_unused,
        param7_local478_halves13, out_param7_words13, &dst->param7_trace);
    if (rc != 0) return rc;

    memcpy(dst->param5_words13, out_param5_words13, sizeof(dst->param5_words13));
    memcpy(dst->param6_words13, out_param6_words13, sizeof(dst->param6_words13));
    memcpy(dst->param7_words13, out_param7_words13, sizeof(dst->param7_words13));
    return 0;
}
