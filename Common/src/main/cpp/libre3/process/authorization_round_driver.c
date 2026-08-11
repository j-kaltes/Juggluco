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
#include "authorization_round_driver.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "authorization_round_internal.h"

typedef l3_authorization_round_finish_trace l3_authorization_round_workspace;

typedef struct {
    const int32_t *param1_words;
    uint32_t round_index;
    const uint32_t *live_q6_words4;
    const uint32_t *live_q5_words4;
    const uint32_t *live_q4_words4;
    const uint32_t *sixth_words13;
    const uint32_t *param4_param5_words13;
    const uint32_t *param4_staging_words13;
    const uint32_t *affine_words13;
    const uint32_t *param4_mix_words26;
    const uint32_t *local3a0_words26;
    const int32_t *vector_reduce_param1_words13;
    uint32_t scalar0;
    uint32_t scalar1;
    const int32_t *eighth_local5e0_words13;
    const int32_t *eighth_param3_words13;
    const int32_t *secondary_state_a_words13;
    const int32_t *secondary_context_mix_words26;
    const uint32_t *secondary_carry_seed_words26;
    const int32_t *secondary_state_b_words13;
    const uint32_t *secondary_mix_a_words13;
} l3_authorization_round_terminal_args;

static uint32_t inverse_odd_u32(uint32_t x) {
    uint32_t inverse = 1u;
    for (unsigned i = 0; i < 5u; ++i) inverse *= 2u - x * inverse;
    return inverse;
}

static uint64_t words2_u64(const uint32_t words[2]) {
    return (uint64_t)words[0] | (uint64_t)words[1] << 32;
}

static void derive_following_seed(
    const uint32_t affine_words13[13],
    const uint32_t middle_mul4[4],
    const uint32_t middle_add4[4],
    l3_authorization_vector_seed *out) {
    uint32_t raw_middle4[4];
    for (unsigned i = 0; i < 4u; ++i) {
        raw_middle4[i] = (affine_words13[4u + i] - middle_add4[i]) *
                         inverse_odd_u32(middle_mul4[i]);
    }
    out->high_halves2[0] = words2_u64(affine_words13 + 8u);
    out->high_halves2[1] = words2_u64(affine_words13 + 10u);
    out->low_halves2[0] = words2_u64(raw_middle4);
    out->low_halves2[1] = words2_u64(raw_middle4 + 2u);
    out->middle_low_halves2[0] = words2_u64(affine_words13 + 4u);
    out->middle_low_halves2[1] = words2_u64(affine_words13 + 6u);
}

typedef int (*l3_authorization_round_post_vector_feedback_fn)(
    const uint32_t vector_feedback_words13[13],
    uint32_t out_words13[13]);

static int run_vector_feedback_export(
    const int32_t seed_words13[13],
    uint32_t native_param2_unused,
    uint32_t scalar0,
    uint32_t scalar1,
    const uint64_t local478_halves13[13][2],
    l3_authorization_round_post_vector_feedback_fn post_export,
    uint32_t out_words13[13],
    l3_authorization_feedback_observation *observation) {
    if (!seed_words13 || !local478_halves13 || !post_export ||
        !out_words13 || !observation) {
        return -1;
    }

    observation->arguments3[0] = scalar0;
    observation->arguments3[1] = native_param2_unused;
    observation->arguments3[2] = scalar1;
    memcpy(observation->seed_words13, seed_words13,
           sizeof(observation->seed_words13));

    l3_authorization_vector_feedback_pair32 state26[26];
    for (unsigned lane = 0; lane < 13u; ++lane) {
        state26[2u * lane].lo = (uint32_t)local478_halves13[lane][0];
        state26[2u * lane].hi =
            (uint32_t)(local478_halves13[lane][0] >> 32);
        state26[2u * lane + 1u].lo =
            (uint32_t)local478_halves13[lane][1];
        state26[2u * lane + 1u].hi =
            (uint32_t)(local478_halves13[lane][1] >> 32);
    }
    memcpy(observation->state_before26, state26,
           sizeof(observation->state_before26));

    int rc = l3_authorization_vector_feedback_run13_update_export(
        seed_words13, native_param2_unused, scalar1, state26,
        observation->output_words13, NULL);
    if (rc != 0) return rc;
    memcpy(observation->state_after26, state26,
           sizeof(observation->state_after26));
    return post_export(observation->output_words13, out_words13);
}

/* Native FUN_f3fbc5c0 does not receive nine independent seed vectors.  It
 * materializes param_5 first, derives param_6's vector seed from that export,
 * then derives param_7's seed from param_6.  This helper follows that order
 * directly and uses the post-local_618 low image for the final param_7 path.
 * It therefore evaluates each convolution/materializer exactly once. */
static int call_native_terminal_dynamic(
    const int32_t seed_words13[13],
    uint32_t native_param2_unused,
    uint32_t scalar1_unused,
    const int32_t round_2cc_words13[13],
    const int32_t round_ec8_words13[13],
    const uint32_t local514_words13[13],
    const uint32_t local5b0_words13[13],
    const uint32_t local548_words13[13],
    const uint32_t local57c_words13[13],
    const uint32_t local5e4_words13[13],
    const uint32_t local618_words13[13],
    const l3_authorization_vector_seed *param5_seed,
    uint32_t out_param5_words13[13],
    uint32_t out_param6_words13[13],
    uint32_t out_param7_words13[13],
    l3_authorization_vector_seed *out_next_param5_seed,
    l3_authorization_feedback_observation observations3[3]) {
    static const uint32_t p5_middle_mul[4] = {
        0x1bcf873du, 0xdfdfee13u, 0xd6b5eeb1u, 0xfe48f001u
    };
    static const uint32_t p5_middle_add[4] = {
        0x9758affau, 0xf437a370u, 0xb90730b2u, 0x0bcaeb52u
    };
    static const uint32_t p6_middle_mul[4] = {
        0x46b3c5e5u, 0x0f36e889u, 0x5ab50ec3u, 0xdb02388du
    };
    static const uint32_t p6_middle_add[4] = {
        0x97cd793fu, 0xbc0dc7e7u, 0x3d316219u, 0x716e7356u
    };
    if (!seed_words13 || !round_2cc_words13 || !round_ec8_words13 ||
        !local514_words13 || !local5b0_words13 || !local548_words13 ||
        !local57c_words13 || !local5e4_words13 || !local618_words13 ||
        !param5_seed ||
        !out_param5_words13 || !out_param6_words13 || !out_param7_words13 ||
        !out_next_param5_seed || !observations3) {
        return -1;
    }

    int32_t local514[13], local5b0[13], local548[13], local57c[13];
    int32_t local5e4[13], local618[13];
    memcpy(local514, local514_words13, sizeof(local514));
    memcpy(local5b0, local5b0_words13, sizeof(local5b0));
    memcpy(local548, local548_words13, sizeof(local548));
    memcpy(local57c, local57c_words13, sizeof(local57c));
    memcpy(local5e4, local5e4_words13, sizeof(local5e4));
    memcpy(local618, local618_words13, sizeof(local618));

    l3_authorization_word_pair high_pairs26[26];
    l3_authorization_word_pair low_pairs26[26];
    l3_authorization_word_pair param6_low_pairs26[26];
    uint64_t local478_halves13[13][2];
    uint32_t vector_scalar0;
    l3_authorization_vector_seed param6_seed, param7_seed;

    int rc = l3_authorization_core_post_secondary_mix_accum_convolution26(
        local514, round_2cc_words13, high_pairs26, NULL);
    if (rc != 0) return rc;
    rc = l3_authorization_stage1_stage_accumulate_and_convolve26(
        local5b0, round_ec8_words13, low_pairs26, NULL);
    if (rc != 0) return rc;
    memcpy(observations3[0].vector_input_a13, local5b0,
           sizeof(observations3[0].vector_input_a13));
    memcpy(observations3[0].vector_input_b13, round_ec8_words13,
           sizeof(observations3[0].vector_input_b13));
    memcpy(observations3[0].vector_high_pairs26, high_pairs26,
           sizeof(observations3[0].vector_high_pairs26));
    memcpy(observations3[0].vector_low_pairs26, low_pairs26,
           sizeof(observations3[0].vector_low_pairs26));
    rc = l3_authorization_stage1_stage_vector_reduction13(
        low_pairs26, high_pairs26,
        param5_seed->high_halves2, param5_seed->low_halves2,
        param5_seed->middle_low_halves2, local478_halves13,
        &vector_scalar0, NULL);
    if (rc != 0) return rc;
    rc = run_vector_feedback_export(
        seed_words13, native_param2_unused, vector_scalar0, scalar1_unused,
        local478_halves13, l3_authorization_materialize_round_output_a,
        out_param5_words13, &observations3[0]);
    if (rc != 0) return rc;

    derive_following_seed(out_param5_words13, p5_middle_mul, p5_middle_add,
                          &param6_seed);
    rc = l3_authorization_stage2_stage_accumulate_and_convolve26(
        local548, round_2cc_words13, high_pairs26, NULL);
    if (rc != 0) return rc;
    rc = l3_authorization_stage3_stage_accumulate_and_convolve26(
        local5e4, round_ec8_words13, param6_low_pairs26, NULL);
    if (rc != 0) return rc;
    memcpy(observations3[1].vector_input_a13, local548,
           sizeof(observations3[1].vector_input_a13));
    memcpy(observations3[1].vector_input_b13, round_2cc_words13,
           sizeof(observations3[1].vector_input_b13));
    memcpy(observations3[1].vector_high_pairs26, high_pairs26,
           sizeof(observations3[1].vector_high_pairs26));
    memcpy(observations3[1].vector_low_pairs26, param6_low_pairs26,
           sizeof(observations3[1].vector_low_pairs26));
    rc = l3_authorization_stage2_stage_vector_reduction13(
        param6_low_pairs26, high_pairs26,
        param6_seed.high_halves2, param6_seed.low_halves2,
        param6_seed.middle_low_halves2, local478_halves13,
        &vector_scalar0, NULL);
    if (rc != 0) return rc;
    rc = run_vector_feedback_export(
        seed_words13, native_param2_unused, vector_scalar0, scalar1_unused,
        local478_halves13, l3_authorization_materialize_round_output_b,
        out_param6_words13, &observations3[1]);
    if (rc != 0) return rc;

    derive_following_seed(out_param6_words13, p6_middle_mul, p6_middle_add,
                          &param7_seed);
    rc = l3_authorization_core_param7_high_accum_convolution26(
        local57c, round_2cc_words13, high_pairs26);
    if (rc != 0) return rc;
    rc = l3_authorization_stage4_stage_accumulate_and_convolve26(
        local618, round_ec8_words13, low_pairs26, NULL);
    if (rc != 0) return rc;
    memcpy(observations3[2].vector_input_a13, local618,
           sizeof(observations3[2].vector_input_a13));
    memcpy(observations3[2].vector_input_b13, round_ec8_words13,
           sizeof(observations3[2].vector_input_b13));
    memcpy(observations3[2].vector_high_pairs26, high_pairs26,
           sizeof(observations3[2].vector_high_pairs26));
    memcpy(observations3[2].vector_low_pairs26, low_pairs26,
           sizeof(observations3[2].vector_low_pairs26));
    rc = l3_authorization_stage3_stage_vector_reduction13(
        low_pairs26, high_pairs26,
        param7_seed.high_halves2, param7_seed.low_halves2,
        param7_seed.middle_low_halves2, local478_halves13,
        &vector_scalar0, NULL);
    if (rc != 0) return rc;
    rc = run_vector_feedback_export(
        seed_words13, native_param2_unused, vector_scalar0, scalar1_unused,
        local478_halves13, l3_authorization_materialize_round_output_c,
        out_param7_words13, &observations3[2]);
    if (rc != 0) return rc;

    derive_following_seed(out_param7_words13, p5_middle_mul, p5_middle_add,
                          out_next_param5_seed);
    return 0;
}

static int call_terminal(
    const l3_authorization_round_terminal_args *a,
    const l3_authorization_vector_seed *param5_seed,
    const l3_authorization_vector_seed *param6_seed,
    const l3_authorization_vector_seed *param7_seed,
    uint32_t out_param5_words13[13],
    uint32_t out_param6_words13[13],
    uint32_t out_param7_words13[13],
    l3_authorization_round_workspace *workspace) {
    memset(workspace, 0, sizeof(*workspace));
    return l3_authorization_round_finish(
        a->param1_words, a->round_index,
        a->live_q6_words4, a->live_q5_words4, a->live_q4_words4,
        a->sixth_words13, a->param4_param5_words13,
        a->param4_staging_words13, a->affine_words13,
        a->param4_mix_words26, a->local3a0_words26,
        a->vector_reduce_param1_words13, a->scalar0, a->scalar1,
        a->eighth_local5e0_words13, a->eighth_param3_words13,
        a->vector_reduce_param1_words13, a->scalar0, a->scalar1,
        a->secondary_state_a_words13, a->secondary_context_mix_words26,
        a->secondary_carry_seed_words26, a->secondary_state_b_words13,
        a->secondary_mix_a_words13,
        param5_seed->high_halves2, param5_seed->low_halves2,
        param5_seed->middle_low_halves2,
        param6_seed->high_halves2, param6_seed->low_halves2,
        param6_seed->middle_low_halves2,
        param7_seed->high_halves2, param7_seed->low_halves2,
        param7_seed->middle_low_halves2,
        out_param5_words13, out_param6_words13, out_param7_words13,
        workspace);
}

static void copy_i32_words(const int32_t *ctx, size_t byte_offset,
                           size_t count, int32_t *out) {
    memcpy(out, (const uint8_t *)ctx + byte_offset, count * sizeof(*out));
}

static void copy_u32_words(const int32_t *ctx, size_t byte_offset,
                           size_t count, uint32_t *out) {
    memcpy(out, (const uint8_t *)ctx + byte_offset, count * sizeof(*out));
}

static uint32_t load_u32(const int32_t *ctx, size_t byte_offset) {
    uint32_t value;
    memcpy(&value, (const uint8_t *)ctx + byte_offset, sizeof(value));
    return value;
}

static int context_covers_round(size_t word_count, uint32_t round_index) {
    if (word_count > SIZE_MAX / sizeof(int32_t)) return 0;
    const size_t context_bytes = word_count * sizeof(int32_t);
    const uint64_t last_window_end64 = UINT64_C(0xec8) +
        (uint64_t)round_index * UINT64_C(0x34) +
        UINT64_C(13) * sizeof(int32_t);
    if (word_count < L3_AUTH_ROUND_DRIVER_MIN_CONTEXT_WORDS) return 0;
    if (last_window_end64 > (uint64_t)SIZE_MAX) return 0;
    return (size_t)last_window_end64 <= context_bytes;
}

static int validate_handoff_windows(const int32_t *ctx,
                                    const l3_authorization_round_handoff *tail) {
    int32_t expected_2cc[13];
    int32_t expected_ec8[13];
    const size_t cursor = (size_t)tail->next_round_index * 0x34u;
    copy_i32_words(ctx, 0x2ccu + cursor, 13u, expected_2cc);
    copy_i32_words(ctx, 0xec8u + cursor, 13u, expected_ec8);
    return memcmp(expected_2cc, tail->next_round_2cc_words13, sizeof(expected_2cc)) == 0 &&
           memcmp(expected_ec8, tail->next_round_ec8_words13, sizeof(expected_ec8)) == 0;
}


static int round_step_with_workspace(
    const int32_t *param1_words,
    size_t param1_word_count,
    const l3_authorization_round_handoff *previous_tail,
    const l3_authorization_round_seed *legacy_seeds,
    const l3_authorization_vector_seed *native_param5_seed,
    const int32_t native_secondary_caller_state_a_words13[13],
    const int32_t native_secondary_caller_state_b_words13[13],
    l3_authorization_vector_seed *out_next_param5_seed,
    l3_authorization_round_observation *out,
    l3_authorization_round_workspace *workspace) {
    if (!param1_words || !previous_tail || !out ||
        (legacy_seeds && !workspace) ||
        ((!legacy_seeds) == (!native_param5_seed)) ||
        ((native_secondary_caller_state_a_words13 == NULL) !=
         (native_secondary_caller_state_b_words13 == NULL)) ||
        (native_param5_seed && !out_next_param5_seed)) {
        return L3_AUTH_ROUND_DRIVER_INVALID;
    }
    memset(out, 0, sizeof(*out));
    if (!previous_tail->has_next_round) return L3_AUTH_ROUND_DRIVER_DONE;
    if (!context_covers_round(param1_word_count, previous_tail->next_round_index)) {
        return L3_AUTH_ROUND_DRIVER_BOUNDS;
    }
    if (!validate_handoff_windows(param1_words, previous_tail)) {
        return L3_AUTH_ROUND_DRIVER_STATE_MISMATCH;
    }

    out->round_index = previous_tail->next_round_index;
    for (unsigned i = 0; i < 4u; ++i) {
        out->live_q6_words4[i] = previous_tail->param5_words13[i];
        out->live_q5_words4[i] = previous_tail->param6_words13[i];
        out->live_q4_words4[i] = previous_tail->param7_words13[i];
    }
    for (unsigned i = 0; i < 13u; ++i) {
        out->fifth_words13[i] = (uint32_t)previous_tail->next_round_2cc_words13[i];
        out->sixth_seed_words13[i] = previous_tail->next_round_ec8_words13[i];
    }

    const uint32_t scalar0 = load_u32(param1_words, 0x268u);
    const uint32_t scalar1 = load_u32(param1_words, 0x26cu);
    uint32_t affine_words13[13];
    uint32_t sixth_state26[26][2];
    int rc = l3_authorization_primary_sixth_vector_reduce_update_export(
        out->fifth_words13,
        out->sixth_seed_words13,
        scalar0,
        scalar1,
        affine_words13,
        sixth_state26,
        out->sixth_words13,
        NULL);
    if (rc != 0) return L3_AUTH_ROUND_DRIVER_NATIVE_ERROR;

    uint32_t param4_param5_words13[13];
    uint32_t param4_staging_words13[13];
    uint32_t param4_mix_words26[26];
    uint32_t local3a0_words26[26];
    int32_t vector_reduce_param1_words13[13];
    copy_u32_words(param1_words, 0x0f8u, 13u, param4_param5_words13);
    copy_u32_words(param1_words, 0x12cu, 13u, param4_staging_words13);
    copy_u32_words(param1_words, 0x1fcu, 26u, param4_mix_words26);
    copy_u32_words(param1_words, 0x194u, 26u, local3a0_words26);
    copy_i32_words(param1_words, 0x090u, 13u, vector_reduce_param1_words13);

    int32_t previous_param6_words13[13];
    int32_t eighth_local5e0_words13[13];
    int32_t eighth_param3_words13[13];
    if (native_param5_seed) {
        rc = l3_authorization_primary_run(
            previous_tail->param5_words13,
            previous_tail->param6_words13,
            previous_tail->param7_words13,
            param1_words,
            out->primary_state_a_words13,
            out->primary_state_b_words13,
            out->primary_state_c_words13);
    } else {
        uint32_t seed_raw1[13][2], seed_prefix1[13][2];
        uint32_t seed_raw2[13][2], seed_prefix2[13][2];
        memcpy(previous_param6_words13, previous_tail->param6_words13,
               sizeof(previous_param6_words13));
        memcpy(eighth_param3_words13, previous_tail->param7_words13,
               sizeof(eighth_param3_words13));
        rc = l3_authorization_primary_param6_seed_prefixes13(
            previous_param6_words13,
            eighth_local5e0_words13,
            seed_raw1,
            seed_prefix1,
            seed_raw2,
            seed_prefix2);
        if (rc == 0) {
            rc = l3_authorization_primary_post_sixth_terminal_words13(
                out->live_q6_words4,
                out->live_q5_words4,
                out->live_q4_words4,
                out->sixth_words13,
                param4_param5_words13,
                param4_staging_words13,
                affine_words13,
                param4_mix_words26,
                local3a0_words26,
                vector_reduce_param1_words13,
                scalar0,
                scalar1,
                eighth_local5e0_words13,
                eighth_param3_words13,
                vector_reduce_param1_words13,
                scalar0,
                scalar1,
                out->primary_state_a_words13,
                out->primary_state_b_words13,
                out->primary_intermediate_words13,
                out->primary_state_c_words13,
                NULL);
        }
    }
    if (rc != 0) return L3_AUTH_ROUND_DRIVER_NATIVE_ERROR;

    int32_t secondary_input_words13[13];
    int32_t secondary_context_words13[13];
    int32_t caller_mix_words13[13];
    int32_t caller_seed_words13[13];
    int32_t param4_c4_words13[13];
    uint32_t secondary_affine_words13[13];
    uint32_t local5b0_words13[13];
    uint32_t sixth_secondary_vector_reduce_words13[13];
    memcpy(secondary_input_words13, out->primary_state_a_words13,
           sizeof(secondary_input_words13));
    memcpy(out->secondary_state_a_words13, out->primary_state_b_words13,
           sizeof(out->secondary_state_a_words13));
    memcpy(out->secondary_state_b_words13, out->primary_state_c_words13,
           sizeof(out->secondary_state_b_words13));
    memcpy(secondary_context_words13, param4_param5_words13,
           sizeof(secondary_context_words13));
    memcpy(caller_mix_words13, param4_staging_words13,
           sizeof(caller_mix_words13));
    copy_i32_words(param1_words, 0x090u, 13u, caller_seed_words13);
    copy_i32_words(param1_words, 0x0c4u, 13u, param4_c4_words13);

    rc = l3_authorization_secondary_param3_bound_secondary_vector_reduce_affine_words13(
        out->secondary_state_b_words13,
        caller_seed_words13,
        scalar0,
        scalar1,
        secondary_affine_words13,
        NULL);
    if (rc != 0) return L3_AUTH_ROUND_DRIVER_NATIVE_ERROR;
    rc = l3_authorization_secondary_derive_state_stage1(
        secondary_context_words13,
        secondary_affine_words13,
        caller_seed_words13,
        scalar0,
        scalar1,
        secondary_input_words13,
        caller_mix_words13,
        out->secondary_mix_a_words13,
        NULL);
    if (rc != 0) return L3_AUTH_ROUND_DRIVER_NATIVE_ERROR;
    rc = l3_authorization_secondary_derive_state_stage2(
        out->secondary_state_b_words13,
        secondary_affine_words13,
        caller_seed_words13,
        scalar0,
        scalar1,
        out->secondary_mix_a_words13,
        out->secondary_state_a_words13,
        param4_c4_words13,
        out->secondary_mix_b_words13,
        NULL);
    if (rc != 0) return L3_AUTH_ROUND_DRIVER_NATIVE_ERROR;
    rc = l3_authorization_secondary_derive_state_stage3(
        out->secondary_mix_a_words13,
        caller_seed_words13,
        scalar0,
        scalar1,
        out->secondary_mix_c_words13,
        NULL);
    if (rc != 0) return L3_AUTH_ROUND_DRIVER_NATIVE_ERROR;
    rc = l3_authorization_secondary_local3f8_bound_sixth_local42c_preimage_param7_local460_words13(
        out->secondary_mix_c_words13,
        out->secondary_mix_a_words13,
        param1_words,
        secondary_input_words13,
        caller_seed_words13,
        scalar0,
        scalar1,
        out->secondary_mix_b_words13,
        out->secondary_mix_d_words13,
        sixth_secondary_vector_reduce_words13,
        local5b0_words13,
        out->secondary_mix_e_words13,
        NULL);
    if (rc != 0) return L3_AUTH_ROUND_DRIVER_NATIVE_ERROR;

    l3_authorization_secondary_pair32 raw22f0[13], prefix22f0[13];
    l3_authorization_secondary_pair32 raw2370[13], prefix2370[13];
    rc = l3_authorization_secondary_param2_local42c_accum22f0_2370_pairs13(
        out->secondary_state_a_words13,
        out->secondary_mix_d_words13,
        raw22f0,
        prefix22f0,
        raw2370,
        prefix2370,
        NULL);
    if (rc != 0) return L3_AUTH_ROUND_DRIVER_NATIVE_ERROR;
    memset(out->secondary_carry_seed_words26, 0,
           sizeof(out->secondary_carry_seed_words26));
    memcpy(out->secondary_carry_seed_words26 + 2u, prefix22f0,
           24u * sizeof(uint32_t));

    int32_t secondary_context_mix_words26[26];
    copy_i32_words(param1_words, 0x194u, 26u, secondary_context_mix_words26);
    if (legacy_seeds) {
        const l3_authorization_round_terminal_args terminal = {
            param1_words, out->round_index,
            out->live_q6_words4, out->live_q5_words4, out->live_q4_words4,
            out->sixth_words13, param4_param5_words13, param4_staging_words13,
            affine_words13, param4_mix_words26, local3a0_words26,
            vector_reduce_param1_words13, scalar0, scalar1,
            eighth_local5e0_words13, eighth_param3_words13,
            out->secondary_state_a_words13, secondary_context_mix_words26,
            out->secondary_carry_seed_words26, out->secondary_state_b_words13,
            out->secondary_mix_a_words13
        };
        const l3_authorization_vector_seed p5 = {
            {legacy_seeds->param5_local4c0_halves2[0], legacy_seeds->param5_local4c0_halves2[1]},
            {legacy_seeds->param5_local4d0_halves2[0], legacy_seeds->param5_local4d0_halves2[1]},
            {legacy_seeds->param5_auvar98_halves2[0], legacy_seeds->param5_auvar98_halves2[1]}
        };
        const l3_authorization_vector_seed p6 = {
            {legacy_seeds->param6_high_seed_halves2[0], legacy_seeds->param6_high_seed_halves2[1]},
            {legacy_seeds->param6_low_seed_halves2[0], legacy_seeds->param6_low_seed_halves2[1]},
            {legacy_seeds->param6_middle_low_seed_halves2[0], legacy_seeds->param6_middle_low_seed_halves2[1]}
        };
        const l3_authorization_vector_seed p7 = {
            {legacy_seeds->param7_high_seed_halves2[0], legacy_seeds->param7_high_seed_halves2[1]},
            {legacy_seeds->param7_low_seed_halves2[0], legacy_seeds->param7_low_seed_halves2[1]},
            {legacy_seeds->param7_middle_low_seed_halves2[0], legacy_seeds->param7_middle_low_seed_halves2[1]}
        };
        rc = call_terminal(
            &terminal, &p5, &p6, &p7,
            out->tail.param5_words13, out->tail.param6_words13,
            out->tail.param7_words13, workspace);
        if (rc == 0) {
            out->tail.has_next_round = workspace->has_next_round;
            out->tail.next_round_index = workspace->next_round_index;
            memcpy(out->tail.next_round_2cc_words13,
                   workspace->next_round_2cc_words13,
                   sizeof(out->tail.next_round_2cc_words13));
            memcpy(out->tail.next_round_ec8_words13,
                   workspace->next_round_ec8_words13,
                   sizeof(out->tail.next_round_ec8_words13));
        }
    } else {
        uint32_t local4c8_words26[26], local530_words26[26];
        uint32_t local5e4_words13[13], local618_words13[13];
        rc = l3_authorization_secondary_local3c4_local460_accum_convolution26_materialize4c8(
            out->secondary_mix_b_words13, out->secondary_mix_e_words13,
            local4c8_words26, NULL);
        if (rc == 0) {
            rc = l3_authorization_secondary_param2_local42c_accum_convolution26_materialize530(
                out->secondary_state_a_words13, out->secondary_mix_d_words13,
                local530_words26, NULL);
        }
        if (rc == 0 && native_secondary_caller_state_a_words13) {
            rc = l3_authorization_secondary_run(
                (const int32_t *)out->primary_state_a_words13,
                (const int32_t *)out->primary_state_b_words13,
                (const int32_t *)out->primary_state_c_words13,
                param1_words,
                native_secondary_caller_state_a_words13,
                native_secondary_caller_state_b_words13,
                local5b0_words13, local5e4_words13, local618_words13);
        } else if (rc == 0) {
            rc = l3_authorization_secondary_build_terminal_outputs(
                local4c8_words26, secondary_context_mix_words26,
                local530_words26, out->secondary_carry_seed_words26,
                out->secondary_state_b_words13, out->secondary_mix_a_words13,
                vector_reduce_param1_words13, scalar0, scalar1,
                local5e4_words13, local618_words13, NULL);
        }
        if (rc == 0) {
            rc = call_native_terminal_dynamic(
                vector_reduce_param1_words13, scalar0, scalar1,
                previous_tail->next_round_2cc_words13,
                previous_tail->next_round_ec8_words13,
                out->primary_state_a_words13, local5b0_words13,
                out->primary_state_b_words13,
                out->primary_state_c_words13, local5e4_words13,
                local618_words13, native_param5_seed,
                out->tail.param5_words13, out->tail.param6_words13,
                out->tail.param7_words13, out_next_param5_seed,
                out->vector_feedback_calls3);
        }
        if (rc == 0 && out->round_index != 0u) {
            out->tail.has_next_round = 1u;
            out->tail.next_round_index = out->round_index - 1u;
            const size_t next_cursor = (size_t)out->tail.next_round_index * 0x34u;
            copy_i32_words(param1_words, 0x2ccu + next_cursor, 13u,
                           out->tail.next_round_2cc_words13);
            copy_i32_words(param1_words, 0xec8u + next_cursor, 13u,
                           out->tail.next_round_ec8_words13);
        }
    }
    if (rc != 0) return L3_AUTH_ROUND_DRIVER_NATIVE_ERROR;
    return L3_AUTH_ROUND_DRIVER_OK;
}



int l3_authorization_round_state_init(
    const int32_t *param1_words,
    size_t param1_word_count,
    uint32_t pending_round_index,
    const uint32_t param5_words13[13],
    const uint32_t param6_words13[13],
    const uint32_t param7_words13[13],
    const l3_authorization_vector_seed *param5_seed,
    l3_authorization_round_state *out) {
    if (!param1_words || !param5_words13 || !param6_words13 ||
        !param7_words13 || !param5_seed || !out) {
        return L3_AUTH_ROUND_DRIVER_INVALID;
    }
    memset(out, 0, sizeof(*out));
    if (!context_covers_round(param1_word_count, pending_round_index)) {
        return L3_AUTH_ROUND_DRIVER_BOUNDS;
    }
    out->round.has_next_round = 1u;
    out->round.next_round_index = pending_round_index;
    memcpy(out->round.param5_words13, param5_words13,
           sizeof(out->round.param5_words13));
    memcpy(out->round.param6_words13, param6_words13,
           sizeof(out->round.param6_words13));
    memcpy(out->round.param7_words13, param7_words13,
           sizeof(out->round.param7_words13));
    const size_t cursor = (size_t)pending_round_index * 0x34u;
    copy_i32_words(param1_words, 0x2ccu + cursor, 13u,
                   out->round.next_round_2cc_words13);
    copy_i32_words(param1_words, 0xec8u + cursor, 13u,
                   out->round.next_round_ec8_words13);
    out->param5_seed = *param5_seed;
    return L3_AUTH_ROUND_DRIVER_OK;
}


int l3_authorization_run_rounds(
    const int32_t *param1_words,
    size_t param1_word_count,
    l3_authorization_round_state *inout_state,
    uint32_t max_rounds,
    uint32_t *rounds_run,
    l3_authorization_round_step_result *last_result) {
    if (!param1_words || !inout_state) return L3_AUTH_ROUND_DRIVER_INVALID;
    if (rounds_run) *rounds_run = 0u;
    if (last_result) memset(last_result, 0, sizeof(*last_result));
    if (!inout_state->round.has_next_round) return L3_AUTH_ROUND_DRIVER_OK;
    if (max_rounds == 0u) return L3_AUTH_ROUND_DRIVER_MORE;

    l3_authorization_round_state cursor = *inout_state;
    l3_authorization_round_step_result current;
    uint32_t count = 0u;
    int rc = L3_AUTH_ROUND_DRIVER_OK;
    while (cursor.round.has_next_round && count < max_rounds) {
        memset(&current, 0, sizeof(current));
        rc = round_step_with_workspace(
            param1_words, param1_word_count, &cursor.round,
            NULL, &cursor.param5_seed,
            cursor.has_secondary_caller_words
                ? cursor.secondary_caller_state_a_words13 : NULL,
            cursor.has_secondary_caller_words
                ? cursor.secondary_caller_state_b_words13 : NULL,
            &current.next_param5_seed,
            &current.round, NULL);
        if (rc != L3_AUTH_ROUND_DRIVER_OK) break;
        cursor.round = current.round.tail;
        cursor.param5_seed = current.next_param5_seed;
        *inout_state = cursor;
        if (last_result) *last_result = current;
        ++count;
    }
    if (rounds_run) *rounds_run = count;
    if (rc < 0) return rc;
    return cursor.round.has_next_round ? L3_AUTH_ROUND_DRIVER_MORE
                                      : L3_AUTH_ROUND_DRIVER_OK;
}

int l3_authorization_run_round_pipeline(
    const int32_t *parent_context_words,
    size_t parent_context_word_count,
    const uint8_t left66[0x42],
    const uint8_t right66[0x42],
    const uint8_t scalar35[35],
    uint32_t max_rounds,
    l3_authorization_round_pipeline_result *out) {
    if (!parent_context_words || !left66 || !right66 || !scalar35 || !out) {
        return L3_AUTH_ROUND_DRIVER_INVALID;
    }
    memset(out, 0, sizeof(*out));
    if (!context_covers_round(parent_context_word_count, 58u)) {
        return L3_AUTH_ROUND_DRIVER_BOUNDS;
    }

    int rc = l3_authorization_prepare_round_pipeline(
        (const uint8_t *)parent_context_words, left66, right66, scalar35,
        out->preamble_first_words13, out->preamble_second_words13,
        out->initial_param5_words13, out->initial_param6_words13,
        out->initial_param7_words13, &out->initial_param5_seed);
    if (rc != 0) return L3_AUTH_ROUND_DRIVER_NATIVE_ERROR;

    l3_authorization_round_state cursor;
    rc = l3_authorization_round_state_init(
        parent_context_words, parent_context_word_count, 58u,
        out->initial_param5_words13, out->initial_param6_words13,
        out->initial_param7_words13, &out->initial_param5_seed, &cursor);
    if (rc != L3_AUTH_ROUND_DRIVER_OK) return rc;
    cursor.has_secondary_caller_words = 1u;
    memcpy(cursor.secondary_caller_state_a_words13, out->preamble_first_words13,
           sizeof(cursor.secondary_caller_state_a_words13));
    memcpy(cursor.secondary_caller_state_b_words13, out->preamble_second_words13,
           sizeof(cursor.secondary_caller_state_b_words13));

    rc = l3_authorization_run_rounds(
        parent_context_words, parent_context_word_count, &cursor, max_rounds,
        &out->rounds_run, &out->last_round);
    out->final_state = cursor;
    return rc;
}
