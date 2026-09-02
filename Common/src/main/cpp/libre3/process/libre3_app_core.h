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

#ifndef L3_LIBRE3_APP_CORE_H
#define L3_LIBRE3_APP_CORE_H

#include "challenge_cipher_context.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Fixed application protocol sizes. */
enum {
    L3_LEN_SAVED_AUTHORIZATION = 149,
    L3_LEN_PATCH_CERTIFICATE = 140,
    L3_LEN_PATCH_PUBLIC_KEY = 65,
    L3_LEN_EPHEMERAL_PUBLIC_KEY = 64,
    L3_LEN_EPHEMERAL_CERTIFICATE = 65,
    L3_LEN_CHALLENGE_NONCE = 7,
    L3_LEN_CHALLENGE_REPLY_PLAIN = 36,
    L3_LEN_CHALLENGE_REPLY_CRYPT = 40,
    L3_LEN_CHALLENGE_RESPONSE_CRYPT = 60,
    L3_LEN_CHALLENGE_RESPONSE_PLAIN = 56,
    L3_LEN_AUTHORIZATION_ROOT = 82,
    L3_LEN_AUTHORIZATION_FRAME = 66
};

/* Operation results retain the values used by the existing JNI boundary. */
#define L3_SECURITY_OK 0
#define L3_SECURITY_ERR_ARGUMENT       (-1)
#define L3_SECURITY_ERR_STATE          (-2)
#define L3_SECURITY_ERR_ENGINE         (-3)
#define L3_SECURITY_ERR_SHORT_OUTPUT   (-4)
#define L3_SECURITY_ERR_UNSUPPORTED    (-5)
#define L3_SECURITY_ERR_NOMEM          (-6)

#define L3_SECURITY_ENGINE_OK 0
#define L3_SECURITY_ENGINE_ERR_ARGUMENT (-1)
#define L3_SECURITY_ENGINE_ERR_CRYPTO   (-2)
#define L3_SECURITY_ENGINE_ERR_NOMEM    (-3)

typedef int (*l3_entropy_callback)(void *user, uint8_t *out, size_t out_len);

/* Kept source-compatible with the Android configuration object.  The flat
 * core needs only entropy and entropy_user; reconstruction-era fields are
 * accepted and ignored so existing callers do not need conditional code. */
typedef struct l3_security_engine_config {
    const uint8_t *master_secret;
    size_t master_secret_len;
    l3_entropy_callback entropy;
    void *entropy_user;
    uint32_t export_format;
    uint32_t challenge_key_mode;
} l3_security_engine_config;

enum l3_authorization_root_stage {
    L3_AUTH_ROOT_STAGE_NONE = 0,
    L3_AUTH_ROOT_STAGE_INPUTS_VALID = 1,
    L3_AUTH_ROOT_STAGE_EPHEMERAL_SCALAR_READY = 2,
    L3_AUTH_ROOT_STAGE_APP_SCALAR_READY = 3,
    L3_AUTH_ROOT_STAGE_EPHEMERAL_POINT_READY = 4,
    L3_AUTH_ROOT_STAGE_APP_POINT_READY = 5,
    L3_AUTH_ROOT_STAGE_FRAME_SHAPE_VALID = 6,
    L3_AUTH_ROOT_STAGE_DIGEST_READY = 7,
    L3_AUTH_ROOT_STAGE_ROOT_READY = 8
};

typedef struct l3_app_pairing_state {
    uint8_t app_private_scalar35[35];
    uint8_t patch_public65[L3_LEN_PATCH_PUBLIC_KEY];
    uint8_t ephemeral_private32[32];
} l3_app_pairing_state;

typedef struct l3_app_authorized_state {
    l3_challenge_block_context challenge;
    /* Native 66-byte constructor frame.  Its first 16 bytes are also the
     * visible authorization-root prefix, so storing a separate 82-byte copy
     * would be redundant. */
    uint8_t root_frame[L3_LEN_AUTHORIZATION_FRAME];
} l3_app_authorized_state;

typedef union l3_app_session_state {
    l3_app_pairing_state pairing;
    l3_app_authorized_state authorized;
} l3_app_session_state;

/* One flat, caller-ownable context.  Pairing state is overwritten by the
 * compact challenge context after authorization; no material objects, vtable,
 * or per-stage heap allocations remain. */
typedef struct l3_app_core {
    l3_entropy_callback entropy;
    void *entropy_user;
    void *authorization_scratch;
    size_t authorization_scratch_size;
    l3_app_session_state session;
    uint32_t authorization_root_stage;
    uint32_t flags;
} l3_app_core;

int l3_app_core_init(l3_app_core *core,
                     const l3_security_engine_config *config);
void l3_app_core_clear(l3_app_core *core);

int l3_app_core_begin(l3_app_core *core);
int l3_app_core_select_app_key_and_saved_authorization(
    l3_app_core *core,
    unsigned security_version,
    const uint8_t *saved_authorization,
    size_t saved_authorization_len);
int l3_app_core_set_patch_certificate(
    l3_app_core *core,
    const uint8_t *patch_certificate,
    size_t patch_certificate_len);
int l3_app_core_create_ephemeral_public_key_into(
    l3_app_core *core,
    uint8_t out64[L3_LEN_EPHEMERAL_PUBLIC_KEY]);
int l3_app_core_derive_authorization_root(
    l3_app_core *core,
    const uint8_t *patch_ephemeral_public_key,
    size_t patch_ephemeral_public_key_len);
int l3_app_core_encrypt_challenge_reply_into(
    l3_app_core *core,
    const uint8_t *nonce7,
    size_t nonce7_len,
    const uint8_t *plain36,
    size_t plain36_len,
    uint8_t out40[L3_LEN_CHALLENGE_REPLY_CRYPT]);
int l3_app_core_decrypt_challenge_response_into(
    l3_app_core *core,
    const uint8_t *nonce7,
    size_t nonce7_len,
    const uint8_t *cipher60,
    size_t cipher60_len,
    uint8_t out56[L3_LEN_CHALLENGE_RESPONSE_PLAIN]);
int l3_app_core_export_saved_authorization_into(
    const l3_app_core *core,
    uint8_t out149[L3_LEN_SAVED_AUTHORIZATION]);

size_t l3_app_core_authorization_scratch_size(void);
size_t l3_app_core_authorization_scratch_alignment(void);
int l3_app_core_set_authorization_scratch(
    l3_app_core *core,
    void *workspace,
    size_t workspace_size);

/* Portable fixed fresh-pairing leaf: 35-byte app scalar, 32-byte ephemeral
 * scalar and two uncompressed P-256 public points produce the native 82-byte
 * root.  It performs no OpenSSL calls and no allocation when scratch is
 * supplied. */
int l3_app_core_derive_root82_into(
    const uint8_t app_scalar35[35],
    const uint8_t ephemeral_private32[32],
    const uint8_t patch_certificate_public65[L3_LEN_PATCH_PUBLIC_KEY],
    const uint8_t patch_ephemeral_public65[L3_LEN_PATCH_PUBLIC_KEY],
    void *scratch,
    size_t scratch_size,
    uint8_t out82[L3_LEN_AUTHORIZATION_ROOT],
    uint32_t *stage_out);

int l3_app_core_debug_copy_authorization_root(
    const l3_app_core *core,
    uint32_t meta4[4],
    uint8_t *out,
    size_t out_cap,
    size_t *out_len);

#ifdef __cplusplus
}
#endif

#endif /* L3_LIBRE3_APP_CORE_H */
