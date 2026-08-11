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
#ifndef L3_AUTHORIZATION_MATERIAL_PRIMARY_MIX_H
#define L3_AUTHORIZATION_MATERIAL_PRIMARY_MIX_H

#include <stdint.h>

#include "authorization_vector_reduction.h"

#ifdef __cplusplus
extern "C" {
#endif

#define L3_AUTH_ROUND_PRIMARY_SEED_WORDS 13u
#define L3_AUTH_ROUND_PRIMARY_CONV_PAIRS 26u



typedef struct l3_authorization_primary_first_vector_reduce_trace {
    int32_t local5e0_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS];
    uint32_t param6_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
    uint32_t param6_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
    uint32_t local5e0_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
    uint32_t local5e0_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
    uint32_t pre_vector_reduce_state26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2];
    l3_authorization_vector_reduce_full_export_trace f3fdb57c;
} l3_authorization_primary_first_vector_reduce_trace;

typedef struct l3_authorization_primary_post_first_seed_trace {
    uint32_t affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS];
    int32_t local614_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS];
    uint32_t affine_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
    uint32_t affine_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
    uint32_t local614_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
    uint32_t local614_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
} l3_authorization_primary_post_first_seed_trace;

/* v222: first seed/prefix front-end inside FUN_f3fc3c70.
 *
 * Native FUN_f3fc3c70 starts by deriving local_5e0[0..12] from param_6,
 * then builds two 13-pair little-endian 64-bit streams:
 *   - a param_6-derived raw pair stream plus cumulative prefix stream;
 *   - a local_5e0-derived raw pair stream plus cumulative prefix stream.
 *
 * This helper stops before the large convolution/reduction body.  Pair arrays
 * are represented as {lo32, hi32} words in native little-endian order.
 */
int l3_authorization_primary_param6_seed_prefixes13(
    const int32_t param6_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    int32_t out_local5e0_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_param6_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_param6_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_local5e0_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_local5e0_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2]);

/* v223: first convolution/reduction body after the v222 param_6/local_5e0
 * seed-prefix front-end.  This emits the native 26-pair local_578 image
 * immediately before the first FUN_f3fdb57c call in FUN_f3fc3c70.
 */
int l3_authorization_primary_param6_local5e0_convolution26(
    const uint32_t param6_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t param6_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t local5e0_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t local5e0_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2]);


/* v230: compose the first FUN_f3fc3c70 local_578 build with the immediately
 * following FUN_f3fdb57c update+export call.  The first argument is the
 * thirteen-word stream used to derive local_5e0 and the local_578 convolution
 * image.  vector_reduce_param1_13 models the table at native param_4+0x90;
 * vector_reduce_param3/param4 model the two scalar words loaded from native
 * param_4+0x268 and param_4+0x26c.  out_words13 is the local_4a4 result.
 */
int l3_authorization_primary_first_vector_reduce_update_export(
    const int32_t param6_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t vector_reduce_param1_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t vector_reduce_param3,
    uint32_t vector_reduce_param4,
    uint32_t out_state26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    uint32_t out_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    l3_authorization_primary_first_vector_reduce_trace *out);

/* v231: post-first-FUN_f3fdb57c setup inside FUN_f3fc3c70.  Native code
 * takes the first 13 exported words in local_4a4..local_474, forms a
 * 13-word affine stream at local_5ac..local_57c, independently derives
 * local_614[0..12] from param_1, then builds raw and cumulative-prefix
 * pair streams for the next 26-pair convolution before the second
 * FUN_f3fdb57c call.
 */
int l3_authorization_primary_post_first_seed_prefixes13(
    const uint32_t first_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t param1_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    int32_t out_local614_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_affine_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_affine_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_local614_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_local614_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2]);

/* v231: second 26-pair convolution preimage in FUN_f3fc3c70, immediately
 * before the second FUN_f3fdb57c call.  Inputs are the two streams produced
 * by l3_authorization_primary_post_first_seed_prefixes13().
 */
int l3_authorization_primary_post_first_convolution26(
    const uint32_t affine_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t affine_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t local614_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t local614_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2]);

int l3_authorization_primary_post_first_pre_second_preimage(
    const uint32_t first_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t param1_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    l3_authorization_primary_post_first_seed_trace *out);


/* v232: run the second FUN_f3fdb57c call-site in FUN_f3fc3c70.
 *
 * This composes the v231 post-first preimage with the completed v229
 * FUN_f3fdb57c update+export helper.  first_words13 is the local_4a4
 * output from the first call, param1_13 is the native param_1 stream used to
 * build local_614, and vector_reduce_param1_13/param3/param4 model the native
 * second FUN_f3fdb57c arguments.
 */
typedef struct l3_authorization_primary_second_vector_reduce_trace {
    l3_authorization_primary_post_first_seed_trace seed;
    uint32_t pre_vector_reduce_state26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2];
    l3_authorization_vector_reduce_full_export_trace f3fdb57c;
} l3_authorization_primary_second_vector_reduce_trace;

int l3_authorization_primary_second_vector_reduce_update_export(
    const uint32_t first_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t param1_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t vector_reduce_param1_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t vector_reduce_param3,
    uint32_t vector_reduce_param4,
    uint32_t out_state26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    uint32_t out_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    l3_authorization_primary_second_vector_reduce_trace *out);

/* Derive the callee-saved q6/q5/q4 values retained by native
 * FUN_f3fc3c70 from the second 13-word export.
 */
int l3_authorization_primary_live_q654_from_second_words13(
    const uint32_t second_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_q6_words4[4],
    uint32_t out_q5_words4[4],
    uint32_t out_q4_words4[4]);

/* v233: post-second-FUN_f3fdb57c preimage inside FUN_f3fc3c70.
 * The second call's 13 exported words are converted into the next affine
 * stream; param_3 is expanded through the next scalar table ladders; both
 * streams are convolved into the 26-pair image before the third FUN_f3fdb57c
 * call.
 */
typedef struct l3_authorization_primary_post_second_seed_trace {
    uint32_t affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS];
    uint32_t affine_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
    uint32_t affine_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
    uint32_t param3_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
    uint32_t param3_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
} l3_authorization_primary_post_second_seed_trace;

int l3_authorization_primary_post_second_seed_prefixes13(
    const uint32_t second_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t param3_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_affine_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_affine_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_param3_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_param3_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2]);

int l3_authorization_primary_post_second_convolution26(
    const uint32_t affine_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t affine_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t param3_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t param3_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2]);

int l3_authorization_primary_post_second_pre_third_preimage(
    const uint32_t second_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t param3_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    l3_authorization_primary_post_second_seed_trace *out);


/* v234: run the third FUN_f3fdb57c call-site in FUN_f3fc3c70.
 * This composes the v233 post-second preimage with the completed v229
 * FUN_f3fdb57c update+export helper.  second_words13 is the output stream
 * from the second call, param3_13 is the native param_3 expansion stream,
 * and vector_reduce_param1_13/param3/param4 model the native third
 * FUN_f3fdb57c arguments.
 */
typedef struct l3_authorization_primary_third_vector_reduce_trace {
    l3_authorization_primary_post_second_seed_trace seed;
    uint32_t pre_vector_reduce_state26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2];
    l3_authorization_vector_reduce_full_export_trace f3fdb57c;
} l3_authorization_primary_third_vector_reduce_trace;

int l3_authorization_primary_third_vector_reduce_update_export(
    const uint32_t second_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t param3_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t vector_reduce_param1_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t vector_reduce_param3,
    uint32_t vector_reduce_param4,
    uint32_t out_state26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    uint32_t out_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    l3_authorization_primary_third_vector_reduce_trace *out);

/* v235: post-third-FUN_f3fdb57c preimage inside FUN_f3fc3c70.
 * The third call's 13 exported words are transformed into two native
 * affine pair streams backed by the f41a0c30 and f41a0cb0 ladders.  These
 * streams feed the 26-pair preimage immediately before the fourth
 * FUN_f3fdb57c call.
 */
typedef struct l3_authorization_primary_post_third_seed_trace {
    uint32_t affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS];
    uint32_t c30_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
    uint32_t c30_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
    uint32_t cb0_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
    uint32_t cb0_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
} l3_authorization_primary_post_third_seed_trace;

int l3_authorization_primary_post_third_seed_prefixes13(
    const uint32_t third_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_c30_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_c30_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_cb0_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_cb0_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2]);

int l3_authorization_primary_post_third_convolution26(
    const uint32_t c30_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t c30_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t cb0_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t cb0_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2]);

int l3_authorization_primary_post_third_pre_fourth_preimage(
    const uint32_t third_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    l3_authorization_primary_post_third_seed_trace *out);

/* v236: run the fourth FUN_f3fdb57c call-site in FUN_f3fc3c70.
 * This composes the v235 post-third preimage with the completed v229
 * FUN_f3fdb57c update+export helper.  third_words13 is the output stream
 * from the third call; vector_reduce_param1_13/param3/param4 model the native
 * fourth FUN_f3fdb57c arguments.
 */
typedef struct l3_authorization_primary_fourth_vector_reduce_trace {
    l3_authorization_primary_post_third_seed_trace seed;
    uint32_t pre_vector_reduce_state26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2];
    l3_authorization_vector_reduce_full_export_trace f3fdb57c;
} l3_authorization_primary_fourth_vector_reduce_trace;

int l3_authorization_primary_fourth_vector_reduce_update_export(
    const uint32_t third_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t vector_reduce_param1_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t vector_reduce_param3,
    uint32_t vector_reduce_param4,
    uint32_t out_state26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    uint32_t out_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    l3_authorization_primary_fourth_vector_reduce_trace *out);


/* v237: post-fourth-FUN_f3fdb57c front-end inside FUN_f3fc3c70.
 * Native code first transforms the fourth call's 13 exported words into the
 * local_6b0..local_680 affine stream, then builds the first fresh
 * param_1-derived 13-pair stream via the f41a0d30 ladder.  This is the
 * bounded front-end immediately before the later local_718 self-convolution
 * and fifth FUN_f3fdb57c preimage construction.
 */
typedef struct l3_authorization_primary_post_fourth_frontend_trace {
    uint32_t affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS];
    uint32_t param1_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
} l3_authorization_primary_post_fourth_frontend_trace;

int l3_authorization_primary_post_fourth_affine_param1_frontend13(
    const uint32_t fourth_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t param1_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_param1_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    l3_authorization_primary_post_fourth_frontend_trace *out);

/* v238: local_718 self-convolution after the v237 post-fourth front-end.
 * Native FUN_f3fc3c70 consumes the fresh param_1 raw 13-pair stream, folds it
 * through a symmetric 26-lane scalar self-convolution, and emits the 26
 * local_718 words used by the following pre-fifth-call staging.
 */
typedef struct l3_authorization_primary_post_fourth_selfconv_trace {
    l3_authorization_primary_post_fourth_frontend_trace frontend;
    uint32_t local718_26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS];
    uint32_t rolling_tail[2];
} l3_authorization_primary_post_fourth_selfconv_trace;

int l3_authorization_primary_post_fourth_self_convolution26(
    const uint32_t param1_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_local718_26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    uint32_t out_rolling_tail[2]);

int l3_authorization_primary_post_fourth_frontend_self_convolution26(
    const uint32_t fourth_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t param1_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_param1_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_local718_26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    l3_authorization_primary_post_fourth_selfconv_trace *out);


/* v239: post-fourth DAT_f41a0e30/DAT_f41a0eb0 staging and fifth-call
 * preimage.  Native FUN_f3fc3c70 next derives two fresh 13-pair streams:
 * one from the post-fourth affine words through f41a0eb0, and one from the
 * following seed words through f41a0e30.  Their raw and cumulative-prefix
 * streams feed the 26-pair preimage used by the next staged call path.
 */
typedef struct l3_authorization_primary_post_fourth_staging_trace {
    l3_authorization_primary_post_fourth_selfconv_trace selfconv;
    uint32_t e30_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
    uint32_t e30_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
    uint32_t eb0_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
    uint32_t eb0_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
    uint32_t pre_fifth_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2];
} l3_authorization_primary_post_fourth_staging_trace;

int l3_authorization_primary_post_fourth_staging_pair_streams13(
    const uint32_t affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t seed13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_e30_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_e30_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_eb0_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_eb0_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2]);

int l3_authorization_primary_post_fourth_staging_convolution26(
    const uint32_t e30_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t e30_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t eb0_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t eb0_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2]);

int l3_authorization_primary_post_fourth_pre_fifth_preimage(
    const uint32_t fourth_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t post_fourth_param1_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t seed13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_param1_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_local718_26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    uint32_t out_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    l3_authorization_primary_post_fourth_staging_trace *out);



/* v240: post-fourth local_718/DAT_f41a0f30 staging.
 * After the v239 pre-fifth image is built, native FUN_f3fc3c70 derives a
 * 26-pair rolling stream from local_718 via DAT_f41a0f30 and a second
 * 13-pair fixed stream via UNK_f41a0fb0.  These raw/prefix streams are the
 * direct inputs to the following convolution before the fifth FUN_f3fdb57c
 * call-site.
 */
typedef struct l3_authorization_primary_post_fourth_local718_staging_trace {
    uint32_t local718_raw26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2];
    uint32_t local718_prefix26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2];
    uint32_t fixed_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
    uint32_t fixed_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
} l3_authorization_primary_post_fourth_local718_staging_trace;

int l3_authorization_primary_post_fourth_local718_staging_pair_streams(
    const uint32_t local718_26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    uint32_t out_local718_raw26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    uint32_t out_local718_prefix26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    uint32_t out_fixed_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_fixed_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    l3_authorization_primary_post_fourth_local718_staging_trace *out);


/* v241: convolution over the post-fourth local_718/DAT_f41a0f30 stream and
 * the companion fixed UNK_f41a0fb0 stream.  This consumes the raw/prefix
 * streams produced by v240 and emits the next 26-pair image that native code
 * materializes in local_3a0 before the subsequent vector affine transform.
 */
typedef struct l3_authorization_primary_post_fourth_local718_convolution_trace {
    l3_authorization_primary_post_fourth_local718_staging_trace streams;
    uint32_t conv_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2];
} l3_authorization_primary_post_fourth_local718_convolution_trace;

int l3_authorization_primary_post_fourth_local718_fixed_convolution26(
    const uint32_t local718_raw26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    const uint32_t local718_prefix26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    const uint32_t fixed_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t fixed_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2]);

int l3_authorization_primary_post_fourth_local718_staging_convolution26(
    const uint32_t local718_26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    uint32_t out_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    l3_authorization_primary_post_fourth_local718_convolution_trace *out);


/* v242: vector-affine fold after the v241 local_3a0 convolution.
 * Native FUN_f3fc3c70 keeps the v239 pre-fifth 26-pair image in the
 * overlapping stack window immediately after local_3a0, then combines that
 * image with the v241 local_3a0 26-pair image via masked low-16 vector
 * products.  The result is the 26-pair state image staged at local_578 for
 * the fifth FUN_f3fdb57c(...) call-site.
 */
typedef struct l3_authorization_primary_post_fourth_vector_affine_trace {
    uint32_t pre_fifth_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2];
    uint32_t local3a0_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2];
    uint32_t state26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2];
} l3_authorization_primary_post_fourth_vector_affine_trace;

int l3_authorization_primary_post_fourth_vector_affine_state26(
    const uint32_t pre_fifth_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    const uint32_t local3a0_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    uint32_t out_state26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    l3_authorization_primary_post_fourth_vector_affine_trace *out);

int l3_authorization_primary_post_fourth_pre_fifth_state26(
    const uint32_t fourth_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t post_fourth_param1_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t seed13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_state26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    l3_authorization_primary_post_fourth_vector_affine_trace *out);


/* v243: run the fifth FUN_f3fdb57c call-site after the v242 post-fourth
 * vector-affine state image.  This composes the completed pre-fifth state
 * builder with the completed FUN_f3fdb57c update+export helper, preserving
 * the 26-pair state image immediately before mutation for trace checks.
 */
typedef struct l3_authorization_primary_fifth_vector_reduce_trace {
    l3_authorization_primary_post_fourth_vector_affine_trace pre_state;
    uint32_t pre_vector_reduce_state26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2];
    l3_authorization_vector_reduce_full_export_trace f3fdb57c;
} l3_authorization_primary_fifth_vector_reduce_trace;

int l3_authorization_primary_fifth_vector_reduce_update_export(
    const uint32_t fourth_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t post_fourth_param1_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t seed13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t vector_reduce_param1_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t vector_reduce_param3,
    uint32_t vector_reduce_param4,
    uint32_t out_state26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    uint32_t out_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    l3_authorization_primary_fifth_vector_reduce_trace *out);


/* v244: post-fifth-FUN_f3fdb57c affine word transform.
 * Immediately after the fifth FUN_f3fdb57c(...) call, native FUN_f3fc3c70
 * multiplies the 13 exported local_4a4..local_474 words by three 4-lane
 * vector coefficient blocks plus one scalar tail.  The resulting
 * local_648..local_618 13-word stream is the seed for the following
 * DAT_f41a1030/DAT_f41a10b0 pair-stream staging.
 */
typedef struct l3_authorization_primary_post_fifth_affine_trace {
    uint32_t fifth_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS];
    uint32_t affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS];
} l3_authorization_primary_post_fifth_affine_trace;

int l3_authorization_primary_post_fifth_affine_words13(
    const uint32_t fifth_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    l3_authorization_primary_post_fifth_affine_trace *out);


/* v245: post-fifth pair-stream staging after the affine stream.
 * Native FUN_f3fc3c70 expands the v244 local_648..local_618 affine words
 * through two 64-bit nibble-ladder schedules backed by DAT_f41a1030 and
 * DAT_f41a10b0.  The raw 13-pair streams and cumulative prefixes feed the
 * next 26-pair convolution image.
 */
typedef struct l3_authorization_primary_post_fifth_staging_trace {
    uint32_t affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS];
    uint32_t f1030_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
    uint32_t f1030_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
    uint32_t f10b0_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
    uint32_t f10b0_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
} l3_authorization_primary_post_fifth_staging_trace;

int l3_authorization_primary_post_fifth_staging_pair_streams13(
    const uint32_t affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_f1030_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_f1030_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_f10b0_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_f10b0_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    l3_authorization_primary_post_fifth_staging_trace *out);

int l3_authorization_primary_post_fifth_affine_staging_pair_streams13(
    const uint32_t fifth_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_f1030_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_f1030_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_f10b0_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_f10b0_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    l3_authorization_primary_post_fifth_staging_trace *out);


/* v246: post-fifth convolution over the staged DAT_f41a1030/DAT_f41a10b0
 * pair streams.  This emits the next 26-pair local_578 image immediately
 * before the following FUN_f3fdb57c(...) call-site.
 */
typedef struct l3_authorization_primary_post_fifth_convolution_trace {
    l3_authorization_primary_post_fifth_staging_trace staging;
    uint32_t conv_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2];
} l3_authorization_primary_post_fifth_convolution_trace;

int l3_authorization_primary_post_fifth_staging_convolution26(
    const uint32_t f1030_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t f1030_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t f10b0_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t f10b0_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2]);

int l3_authorization_primary_post_fifth_affine_staging_convolution26(
    const uint32_t fifth_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    l3_authorization_primary_post_fifth_convolution_trace *out);



/* v247: next FUN_f3fdb57c call-site after the post-fifth staged-stream
 * convolution.  This composes the v246 26-pair local_578 image with the
 * completed v229 FUN_f3fdb57c update/export helper.
 */
typedef struct l3_authorization_primary_sixth_vector_reduce_trace {
    l3_authorization_primary_post_fifth_convolution_trace pre_state;
    uint32_t pre_vector_reduce_state26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2];
    l3_authorization_vector_reduce_full_export_trace f3fdb57c;
} l3_authorization_primary_sixth_vector_reduce_trace;

int l3_authorization_primary_sixth_vector_reduce_update_export(
    const uint32_t fifth_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t vector_reduce_param1_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t vector_reduce_param3,
    uint32_t vector_reduce_param4,
    uint32_t out_affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_state26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    uint32_t out_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    l3_authorization_primary_sixth_vector_reduce_trace *out);


/* v261: post-sixth q-register affine source for the v248 param_5 ladder.
 * Native FUN_f3fc3c70 stores three NEON q-register affine blocks plus one
 * scalar tail into local_130..local_100 immediately before the DAT_f41a1130
 * ladder loop.  This helper captures that source-materialization boundary;
 * q6/q5/q4 are explicit caller-context inputs because they are live NEON
 * registers spanning the sixth FUN_f3fdb57c(...) call-site.
 */
typedef struct l3_authorization_primary_post_sixth_param5_source_trace {
    uint32_t live_q6_words4[4];
    uint32_t live_q5_words4[4];
    uint32_t live_q4_words4[4];
    uint32_t sixth_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS];
    uint32_t param4_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS];
    uint32_t vector_reduce_param4;
    uint32_t source_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS];
} l3_authorization_primary_post_sixth_param5_source_trace;

int l3_authorization_primary_post_sixth_param5_source_words13(
    const uint32_t live_q6_words4[4],
    const uint32_t live_q5_words4[4],
    const uint32_t live_q4_words4[4],
    const uint32_t sixth_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const uint32_t param4_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t vector_reduce_param4,
    uint32_t out_source_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    l3_authorization_primary_post_sixth_param5_source_trace *out);

/* v248: post-sixth-call DAT_f41a1130 ladder stream.
 * Immediately after the sixth FUN_f3fdb57c(...) call, native FUN_f3fc3c70
 * derives the 13-word stream written through param_5 using the DAT_f41a1130
 * seven-step nibble ladder and DAT_f4190890/DAT_f41908b0 lane tables.
 */
typedef struct l3_authorization_primary_post_sixth_param5_ladder_trace {
    uint32_t source_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS];
    uint32_t param5_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS];
    uint32_t final_ladder_word;
    uint32_t final_rolling_word;
} l3_authorization_primary_post_sixth_param5_ladder_trace;

int l3_authorization_primary_post_sixth_param5_ladder_words13(
    const uint32_t source_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_param5_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    l3_authorization_primary_post_sixth_param5_ladder_trace *out);

typedef struct l3_authorization_primary_post_sixth_param5_source_ladder_trace {
    l3_authorization_primary_post_sixth_param5_source_trace source;
    l3_authorization_primary_post_sixth_param5_ladder_trace ladder;
} l3_authorization_primary_post_sixth_param5_source_ladder_trace;

int l3_authorization_primary_post_sixth_param5_source_ladder_words13(
    const uint32_t live_q6_words4[4],
    const uint32_t live_q5_words4[4],
    const uint32_t live_q4_words4[4],
    const uint32_t sixth_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const uint32_t param4_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t vector_reduce_param4,
    uint32_t out_source_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_param5_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    l3_authorization_primary_post_sixth_param5_source_ladder_trace *out);


/* v249: post-sixth DAT_f41a1170 / DAT_f41a11b0 staging.
 * After the v248 param_5 ladder, native FUN_f3fc3c70 builds another
 * 13-word scalar ladder stream via DAT_f41a1170 and a separate 13-pair
 * raw/prefix stream via DAT_f41a11b0.  The q-register preambles that provide
 * these scalar source words remain caller-context boundaries; this helper
 * captures the bounded ladder/pair-stream staging exactly.
 */
typedef struct l3_authorization_primary_post_sixth_staging_trace {
    uint32_t source_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS];
    uint32_t f1170_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS];
    uint32_t affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS];
    uint32_t f11b0_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
    uint32_t f11b0_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
    uint32_t final_ladder_word;
    uint32_t final_rolling_word;
} l3_authorization_primary_post_sixth_staging_trace;

int l3_authorization_primary_post_sixth_f1170_ladder_words13(
    const uint32_t source_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_f1170_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    l3_authorization_primary_post_sixth_staging_trace *out);

int l3_authorization_primary_post_sixth_f11b0_pair_streams13(
    const uint32_t affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_f11b0_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_f11b0_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2]);

int l3_authorization_primary_post_sixth_staging_pair_streams13(
    const uint32_t source_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const uint32_t affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_f1170_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_f11b0_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_f11b0_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    l3_authorization_primary_post_sixth_staging_trace *out);


/* v262: q-register affine source for the v249 DAT_f41a1170 staging stream.
 * This is the sibling native source-materialization block after the v248
 * param_5 ladder.  It builds local_130..local_100 from live q6/q5/q4,
 * the newly emitted param_5 words, and the next 13-word slice of the native
 * param_4 object before the DAT_f41a1170/DAT_f41a11b0 staging loops.
 */
typedef struct l3_authorization_primary_post_sixth_staging_source_trace {
    uint32_t live_q6_words4[4];
    uint32_t live_q5_words4[4];
    uint32_t live_q4_words4[4];
    uint32_t param5_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS];
    uint32_t param4_next_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS];
    uint32_t vector_reduce_param4;
    uint32_t source_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS];
} l3_authorization_primary_post_sixth_staging_source_trace;

int l3_authorization_primary_post_sixth_staging_source_words13(
    const uint32_t live_q6_words4[4],
    const uint32_t live_q5_words4[4],
    const uint32_t live_q4_words4[4],
    const uint32_t param5_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const uint32_t param4_next_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t vector_reduce_param4,
    uint32_t out_source_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    l3_authorization_primary_post_sixth_staging_source_trace *out);

typedef struct l3_authorization_primary_post_sixth_staging_source_pair_streams_trace {
    l3_authorization_primary_post_sixth_staging_source_trace source;
    l3_authorization_primary_post_sixth_staging_trace staging;
} l3_authorization_primary_post_sixth_staging_source_pair_streams_trace;

int l3_authorization_primary_post_sixth_staging_source_pair_streams13(
    const uint32_t live_q6_words4[4],
    const uint32_t live_q5_words4[4],
    const uint32_t live_q4_words4[4],
    const uint32_t param5_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const uint32_t param4_next_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t vector_reduce_param4,
    const uint32_t affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_source_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_f1170_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_f11b0_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_f11b0_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    l3_authorization_primary_post_sixth_staging_source_pair_streams_trace *out);


/* v250: companion DAT_f41a1230 stream and pair-level convolution after the
 * post-sixth DAT_f41a1170/DAT_f41a11b0 staging.  This stops before the
 * following DAT_f41a12b0 scalar output transform.
 */
typedef struct l3_authorization_primary_post_sixth_companion_trace {
    l3_authorization_primary_post_sixth_staging_trace staging;
    uint32_t f1230_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
    uint32_t f1230_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
    uint32_t conv_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2];
} l3_authorization_primary_post_sixth_companion_trace;

int l3_authorization_primary_post_sixth_f1230_pair_streams13(
    const uint32_t f1170_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_f1230_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_f1230_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2]);

int l3_authorization_primary_post_sixth_staging_convolution26(
    const uint32_t f11b0_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t f11b0_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t f1230_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t f1230_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2]);

int l3_authorization_primary_post_sixth_companion_convolution26(
    const uint32_t source_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const uint32_t affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_f1170_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_f11b0_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_f11b0_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_f1230_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_f1230_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    l3_authorization_primary_post_sixth_companion_trace *out);


/* v251: DAT_f41a12b0 scalar output transform after the v250 post-sixth
 * pair-level convolution.  This emits the 26 scalar words stored by native
 * FUN_f3fc3c70 while preserving the mid-ladder and rolling-pair trace.
 */
typedef struct l3_authorization_primary_post_sixth_f12b0_output_trace {
    uint32_t input_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2];
    uint32_t mid_ladder_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2];
    uint32_t rolling_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2];
    uint32_t tail_ladder_low26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS];
    uint32_t output_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS];
    uint32_t final_rolling_pair[2];
    uint32_t final_state_ladder_low;
} l3_authorization_primary_post_sixth_f12b0_output_trace;

typedef struct l3_authorization_primary_post_sixth_output_trace {
    l3_authorization_primary_post_sixth_companion_trace companion;
    l3_authorization_primary_post_sixth_f12b0_output_trace f12b0;
} l3_authorization_primary_post_sixth_output_trace;

int l3_authorization_primary_post_sixth_f12b0_output_words26(
    const uint32_t pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    uint32_t out_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    l3_authorization_primary_post_sixth_f12b0_output_trace *out);

int l3_authorization_primary_post_sixth_companion_output_words26(
    const uint32_t source_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const uint32_t affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_f1170_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_f11b0_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_f11b0_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_f1230_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_f1230_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    uint32_t out_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    l3_authorization_primary_post_sixth_output_trace *out);

/*
 * Native f3fc4a5c..f3fcb1bc carry chain.  The first FUN_f3fdb57c export is
 * retained as local_5ac, transformed into 13 pairs, and reduced into the
 * local_3a0 scalar image consumed after the sixth call.
 */
int l3_authorization_primary_post_sixth_local3a0_words26(
    const uint32_t first_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_local5ac_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_pairs13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_local3a0_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS]);



/* v252: post-sixth local_130 mixer and DAT_f41a1430 scalar output.
 * Native FUN_f3fc3c70 next overlays selected local_130 slots, mixes the
 * v251 26 output words, a caller param_4+0x1fc slice, and the first 26
 * scalar words from the preceding local_3a0 image, then runs the DAT_f41a1430
 * seven-step scalar ladder to emit the next 26-word local_718 stream.
 */
typedef struct l3_authorization_primary_post_sixth_local130_mix_trace {
    uint32_t initial_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS];
    uint32_t f12b0_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS];
    uint32_t param4_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS];
    uint32_t local3a0_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS];
    uint32_t mixed_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS];
} l3_authorization_primary_post_sixth_local130_mix_trace;

typedef struct l3_authorization_primary_post_sixth_f1430_output_trace {
    uint32_t input_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS];
    uint32_t ladder_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS];
    uint32_t rolling_before26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS];
    uint32_t rolling_after26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS];
    uint32_t output_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS];
    uint32_t final_rolling;
} l3_authorization_primary_post_sixth_f1430_output_trace;

typedef struct l3_authorization_primary_post_sixth_local130_f1430_trace {
    l3_authorization_primary_post_sixth_local130_mix_trace mix;
    l3_authorization_primary_post_sixth_f1430_output_trace f1430;
} l3_authorization_primary_post_sixth_local130_f1430_trace;

int l3_authorization_primary_post_sixth_local130_mix_words26(
    const uint32_t initial_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    const uint32_t f12b0_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    const uint32_t param4_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    const uint32_t local3a0_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    uint32_t out_mixed_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    l3_authorization_primary_post_sixth_local130_mix_trace *out);

int l3_authorization_primary_post_sixth_f1430_output_words26(
    const uint32_t mixed_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    uint32_t out_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    l3_authorization_primary_post_sixth_f1430_output_trace *out);

int l3_authorization_primary_post_sixth_local130_f1430_words26(
    const uint32_t initial_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    const uint32_t f12b0_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    const uint32_t param4_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    const uint32_t local3a0_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    uint32_t out_mixed_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    uint32_t out_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    l3_authorization_primary_post_sixth_local130_f1430_trace *out);


/* v253: DAT_f41a1470 pair-image transform after the v252 local_718 words.
 * Native FUN_f3fc3c70 maps the 26 scalar local_718 words through the
 * DAT_f4190af0/DAT_f4190b10 affine lane tables and the DAT_f41a1470
 * 64-bit nibble ladder to build the next 26-pair local_578 image. That
 * image is the state passed to the following FUN_f3fdb57c call-site.
 */
typedef struct l3_authorization_primary_post_sixth_f1470_state_trace {
    uint32_t input_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS];
    uint32_t affine_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS];
    uint32_t ladder_lows26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS];
    uint32_t state26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2];
} l3_authorization_primary_post_sixth_f1470_state_trace;

typedef struct l3_authorization_primary_post_sixth_f1430_f1470_trace {
    l3_authorization_primary_post_sixth_local130_f1430_trace f1430;
    l3_authorization_primary_post_sixth_f1470_state_trace f1470;
} l3_authorization_primary_post_sixth_f1430_f1470_trace;

int l3_authorization_primary_post_sixth_f1470_state26(
    const uint32_t local718_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    uint32_t out_state26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    l3_authorization_primary_post_sixth_f1470_state_trace *out);

int l3_authorization_primary_post_sixth_f1430_f1470_state26(
    const uint32_t initial_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    const uint32_t f12b0_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    const uint32_t param4_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    const uint32_t local3a0_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    uint32_t out_mixed_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    uint32_t out_local718_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    uint32_t out_state26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    l3_authorization_primary_post_sixth_f1430_f1470_trace *out);


/* v254: next FUN_f3fdb57c call-site after the v253 DAT_f41a1470
 * pair-image transform.  This composes the v252 local_130/f1430 output
 * builder, the v253 f1470 pair-state transform, and the completed
 * FUN_f3fdb57c update/export helper.
 */
typedef struct l3_authorization_primary_seventh_vector_reduce_trace {
    l3_authorization_primary_post_sixth_f1430_f1470_trace pre_state;
    uint32_t pre_vector_reduce_state26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2];
    l3_authorization_vector_reduce_full_export_trace f3fdb57c;
} l3_authorization_primary_seventh_vector_reduce_trace;

int l3_authorization_primary_seventh_vector_reduce_update_export(
    const uint32_t initial_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    const uint32_t f12b0_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    const uint32_t param4_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    const uint32_t local3a0_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    const int32_t vector_reduce_param1_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t vector_reduce_param3,
    uint32_t vector_reduce_param4,
    uint32_t out_mixed_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    uint32_t out_local718_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    uint32_t out_state26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    uint32_t out_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    l3_authorization_primary_seventh_vector_reduce_trace *out);

/* v263: composed post-sixth source-to-seventh segment.  This bridges the
 * v261 param_5 source/ladder, the v262 staging source/pair streams, the
 * v250/v251 companion output, and the v254 seventh FUN_f3fdb57c call-site.
 * The v252 local_130 initial image is formed from the 13 materialized v262
 * source words; native subsequently overwrites the tail constants inside the
 * local_130 mixer, so the remaining 13 bridge words are zero-filled here.
 */
typedef struct l3_authorization_primary_post_sixth_composed_seventh_trace {
    l3_authorization_primary_post_sixth_param5_source_ladder_trace param5;
    l3_authorization_primary_post_sixth_staging_source_pair_streams_trace staging;
    l3_authorization_primary_post_sixth_output_trace companion_output;
    uint32_t local130_initial26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS];
    l3_authorization_primary_seventh_vector_reduce_trace seventh;
} l3_authorization_primary_post_sixth_composed_seventh_trace;

int l3_authorization_primary_post_sixth_composed_seventh_update_export(
    const uint32_t live_q6_words4[4],
    const uint32_t live_q5_words4[4],
    const uint32_t live_q4_words4[4],
    const uint32_t sixth_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const uint32_t param4_param5_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const uint32_t param4_staging_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const uint32_t affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const uint32_t param4_mix_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    const uint32_t local3a0_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    const int32_t vector_reduce_param1_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t vector_reduce_param3,
    uint32_t vector_reduce_param4,
    uint32_t out_param5_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_staging_source_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_f12b0_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    uint32_t out_mixed_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    uint32_t out_local718_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    uint32_t out_state26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    uint32_t out_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    l3_authorization_primary_post_sixth_composed_seventh_trace *out);


/* v255: post-seventh FUN_f3fdb57c export transform.  Native FUN_f3fc3c70
 * writes the seventh call's 13 exported words into param_6 through three
 * four-lane vector affine/multiply blocks plus a scalar tail word.
 */
typedef struct l3_authorization_primary_post_seventh_param6_trace {
    uint32_t seventh_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS];
    uint32_t param6_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS];
} l3_authorization_primary_post_seventh_param6_trace;

int l3_authorization_primary_post_seventh_param6_words13(
    const uint32_t seventh_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_param6_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    l3_authorization_primary_post_seventh_param6_trace *out);


/* v256: post-seventh staging streams before the next FUN_f3fdb57c call.
 * Native FUN_f3fc3c70 derives one raw/prefix pair stream from local_5e0
 * through DAT_f41a14f0 and one from param_3 through DAT_f41a1570.
 */
typedef struct l3_authorization_primary_post_seventh_staging_trace {
    int32_t local5e0_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS];
    int32_t param3_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS];
    uint32_t local5e0_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
    uint32_t local5e0_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
    uint32_t param3_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
    uint32_t param3_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
} l3_authorization_primary_post_seventh_staging_trace;

int l3_authorization_primary_post_seventh_staging_pair_streams13(
    const int32_t local5e0_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t param3_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_local5e0_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_local5e0_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_param3_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_param3_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    l3_authorization_primary_post_seventh_staging_trace *out);


/* v257: post-seventh convolution/reduction body over the two v256 streams.
 * Emits the 26-pair local_578 image used by the following FUN_f3fdb57c
 * call-site.
 */
int l3_authorization_primary_post_seventh_convolution26(
    const uint32_t local5e0_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t local5e0_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t param3_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t param3_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2]);


/* v258: final/eighth FUN_f3fdb57c call-site after the post-seventh
 * DAT_f41a14f0/DAT_f41a1570 convolution image.  This composes the v256
 * staging streams, the v257 local_578 preimage, and the completed
 * FUN_f3fdb57c update/export helper.
 */
typedef struct l3_authorization_primary_eighth_vector_reduce_trace {
    l3_authorization_primary_post_seventh_staging_trace seed;
    uint32_t pre_vector_reduce_state26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2];
    l3_authorization_vector_reduce_full_export_trace f3fdb57c;
} l3_authorization_primary_eighth_vector_reduce_trace;

int l3_authorization_primary_eighth_vector_reduce_update_export(
    const int32_t local5e0_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t param3_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t vector_reduce_param1_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t vector_reduce_param3,
    uint32_t vector_reduce_param4,
    uint32_t out_pre_state26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    uint32_t out_state26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    uint32_t out_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    l3_authorization_primary_eighth_vector_reduce_trace *out);

/* v259: final native param_7 affine export after the eighth FUN_f3fdb57c
 * call-site.  Native FUN_f3fc3c70 writes the eighth call's 13 exported words
 * through three four-lane vector multiply blocks plus a scalar tail word.
 */
typedef struct l3_authorization_primary_post_eighth_param7_trace {
    uint32_t eighth_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS];
    uint32_t param7_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS];
} l3_authorization_primary_post_eighth_param7_trace;

int l3_authorization_primary_post_eighth_param7_words13(
    const uint32_t eighth_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_param7_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    l3_authorization_primary_post_eighth_param7_trace *out);

/* v260: composed terminal FUN_f3fc3c70 caller-visible tail.  This stitches
 * together the post-seventh param_6 export, the eighth FUN_f3fdb57c call-site,
 * and the terminal param_7 export so callers no longer need to wire the final
 * two native output streams by hand.
 */
typedef struct l3_authorization_primary_terminal_param6_param7_trace {
    l3_authorization_primary_post_seventh_param6_trace param6;
    l3_authorization_primary_eighth_vector_reduce_trace eighth;
    l3_authorization_primary_post_eighth_param7_trace param7;
} l3_authorization_primary_terminal_param6_param7_trace;

int l3_authorization_primary_terminal_param6_param7_words13(
    const uint32_t seventh_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t local5e0_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t param3_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t vector_reduce_param1_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t vector_reduce_param3,
    uint32_t vector_reduce_param4,
    uint32_t out_param6_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_eighth_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_param7_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    l3_authorization_primary_terminal_param6_param7_trace *out);


/* v264: composed post-sixth-to-terminal FUN_f3fc3c70 tail.  This joins the
 * v263 post-sixth source-to-seventh segment with the v260 caller-visible
 * terminal param_6 / param_7 export.  It lets adapter-level code pass one
 * native-order bundle after the sixth FUN_f3fdb57c(...) export and receive
 * param_5, param_6, eighth words, and param_7 without manually stitching the
 * seventh/export/eighth/export seam.
 */
typedef struct l3_authorization_primary_post_sixth_terminal_trace {
    l3_authorization_primary_post_sixth_composed_seventh_trace post_sixth;
    l3_authorization_primary_terminal_param6_param7_trace terminal;
} l3_authorization_primary_post_sixth_terminal_trace;

int l3_authorization_primary_post_sixth_terminal_words13(
    const uint32_t live_q6_words4[4],
    const uint32_t live_q5_words4[4],
    const uint32_t live_q4_words4[4],
    const uint32_t sixth_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const uint32_t param4_param5_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const uint32_t param4_staging_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const uint32_t affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const uint32_t param4_mix_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    const uint32_t local3a0_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    const int32_t seventh_vector_reduce_param1_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t seventh_vector_reduce_param3,
    uint32_t seventh_vector_reduce_param4,
    const int32_t eighth_local5e0_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t eighth_param3_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t eighth_vector_reduce_param1_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t eighth_vector_reduce_param3,
    uint32_t eighth_vector_reduce_param4,
    uint32_t out_param5_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_param6_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_eighth_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_param7_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    l3_authorization_primary_post_sixth_terminal_trace *out);

/* Complete portable FUN_f3fc3c70 implementation.
 *
 * The three input arrays and three output arrays are the native param_1,
 * param_2, param_3, param_5, param_6, and param_7 arrays respectively.
 * Only the first 0x270 bytes of the parent context are consumed.  Outputs are
 * committed after the full calculation, so corresponding input/output arrays
 * may alias.
 */
int l3_authorization_primary_run(
    const uint32_t input_param5_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const uint32_t input_param6_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const uint32_t input_param7_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const void *parent_context,
    uint32_t out_param5_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_param6_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_param7_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS]);

#ifdef __cplusplus
}
#endif

#endif
