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

#include "libre3_app_core.h"

#include "challenge_block_constants.inc"
#include "saved_authorization_record.h"

#ifndef L3_SAVED_AUTH_ONLY
#include "authorization_digest.h"
#include "authorization_digest_tables.inc"
#include "authorization_shared_point_core.h"
#include "authorization_shared_point_frames.h"
#include "fixed_app_private_keys.h"
#include "p256_private_scalar_codec.h"
#include "p256_scalar_multiply.h"
#if !defined(L3_EXTERNAL_ENTROPY_ONLY)
#include <openssl/rand.h>
#include "openssl_symbols.h"
#include "openssl_function_redirects.h"
#endif
#endif

#include <stdlib.h>
#include <string.h>

enum {
    L3_CORE_FLAG_APP_KEY = 1u << 0,
    L3_CORE_FLAG_PATCH_CERTIFICATE = 1u << 1,
    L3_CORE_FLAG_EPHEMERAL_KEY = 1u << 2,
    L3_CORE_FLAG_AUTHORIZED = 1u << 3
};

static const uint8_t k_saved_magic4[4] = {'L', '3', 'S', '9'};

static const l3_challenge_block_tables k_challenge_tables = {
    k_dat426, sizeof(k_dat426), NULL, 0u
};

static void secure_zero(void *memory, size_t len) {
    volatile uint8_t *p = (volatile uint8_t *)memory;
    while (len-- != 0u) *p++ = 0u;
}

static uint16_t frame_checksum16(
    const uint8_t frame[L3_LEN_AUTHORIZATION_FRAME]) {
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < L3_LEN_AUTHORIZATION_FRAME; ++i) {
        h = (h ^ frame[i]) * 16777619u;
    }
    return (uint16_t)((h >> 16) ^ h);
}

static int core_is_authorized(const l3_app_core *core) {
    return core && (core->flags & L3_CORE_FLAG_AUTHORIZED) != 0u;
}

static int install_authorization_frame(
    l3_app_core *core,
    const uint8_t frame[L3_LEN_AUTHORIZATION_FRAME]) {
    if (!core || !frame) return L3_SECURITY_ERR_ARGUMENT;

    secure_zero(&core->session, sizeof(core->session));
    if (l3_challenge_block_context_init(
            &k_challenge_tables, frame,
            &core->session.authorized.challenge) != 0) {
        secure_zero(&core->session, sizeof(core->session));
        core->flags = 0u;
        return L3_SECURITY_ERR_ENGINE;
    }
    memcpy(core->session.authorized.root_frame, frame,
           L3_LEN_AUTHORIZATION_FRAME);
    core->flags = L3_CORE_FLAG_AUTHORIZED;
    return 1;
}

static int import_saved_authorization(
    l3_app_core *core,
    const uint8_t record[L3_LEN_SAVED_AUTHORIZATION]) {
    if (!core || !record) return L3_SECURITY_ERR_ARGUMENT;
    if (memcmp(record + L3_SAVED_AUTHORIZATION_MAGIC_OFFSET,
               k_saved_magic4, sizeof(k_saved_magic4)) != 0 ||
        l3_saved_authorization_record_verify_header_mac(
            record, L3_LEN_SAVED_AUTHORIZATION) !=
            L3_SAVED_AUTHORIZATION_OK) {
        return L3_SECURITY_ERR_ARGUMENT;
    }

    uint8_t frame[L3_LEN_AUTHORIZATION_FRAME];
    memcpy(frame + 0u,
           record + L3_SAVED_AUTHORIZATION_MANAGER_ID_OFFSET, 16u);
    memcpy(frame + 16u,
           record + L3_SAVED_AUTHORIZATION_PUBLIC_DATA_OFFSET, 16u);
    memcpy(frame + 32u,
           record + L3_SAVED_AUTHORIZATION_NONCE_OFFSET, 16u);
    memcpy(frame + 48u,
           record + L3_SAVED_AUTHORIZATION_PAYLOAD_TAG_OFFSET, 4u);
    memcpy(frame + 52u,
           record + L3_SAVED_AUTHORIZATION_TRAILER_OFFSET, 14u);

    const uint8_t *trailer =
        record + L3_SAVED_AUTHORIZATION_TRAILER_OFFSET;
    const uint16_t stored =
        (uint16_t)trailer[14] | ((uint16_t)trailer[15] << 8);
    if (stored != frame_checksum16(frame) ||
        memcmp(record + L3_SAVED_AUTHORIZATION_ROOT_OFFSET,
               frame, 16u) != 0) {
        secure_zero(frame, sizeof(frame));
        return L3_SECURITY_ERR_ARGUMENT;
    }

    const int rc = install_authorization_frame(core, frame);
    secure_zero(frame, sizeof(frame));
    if (rc == 1) {
        core->authorization_root_stage = L3_AUTH_ROOT_STAGE_ROOT_READY;
    }
    return rc;
}

int l3_app_core_init(l3_app_core *core,
                     const l3_security_engine_config *config) {
    if (!core) return L3_SECURITY_ENGINE_ERR_ARGUMENT;
    memset(core, 0, sizeof(*core));
    if (config) {
        core->entropy = config->entropy;
        core->entropy_user = config->entropy_user;
    }
    return L3_SECURITY_ENGINE_OK;
}

void l3_app_core_clear(l3_app_core *core) {
    if (core) secure_zero(core, sizeof(*core));
}

int l3_app_core_begin(l3_app_core *core) {
    if (!core) return L3_SECURITY_ERR_ARGUMENT;
    secure_zero(&core->session, sizeof(core->session));
    core->flags = 0u;
    core->authorization_root_stage = L3_AUTH_ROOT_STAGE_NONE;
    return 1;
}

int l3_app_core_select_app_key_and_saved_authorization(
    l3_app_core *core,
    unsigned security_version,
    const uint8_t *saved_authorization,
    size_t saved_authorization_len) {
    if (!core ||
        ((saved_authorization == NULL) != (saved_authorization_len == 0u)) ||
        (saved_authorization &&
         saved_authorization_len != L3_LEN_SAVED_AUTHORIZATION) ||
        security_version >= 2u) {
        return L3_SECURITY_ERR_ARGUMENT;
    }

    secure_zero(&core->session, sizeof(core->session));
    core->flags = 0u;
    core->authorization_root_stage = L3_AUTH_ROOT_STAGE_NONE;
    if (saved_authorization) {
        return import_saved_authorization(core, saved_authorization);
    }

#ifdef L3_SAVED_AUTH_ONLY
    return L3_SECURITY_ERR_UNSUPPORTED;
#else
    if (l3_fixed_app_private_native_scalar35(
            security_version,
            core->session.pairing.app_private_scalar35) != 0) {
        secure_zero(&core->session, sizeof(core->session));
        return L3_SECURITY_ERR_ENGINE;
    }
    core->flags = L3_CORE_FLAG_APP_KEY;
    return 1;
#endif
}

int l3_app_core_set_patch_certificate(
    l3_app_core *core,
    const uint8_t *patch_certificate,
    size_t patch_certificate_len) {
#ifdef L3_SAVED_AUTH_ONLY
    (void)core;
    (void)patch_certificate;
    (void)patch_certificate_len;
    return L3_SECURITY_ERR_UNSUPPORTED;
#else
    if (!core || !patch_certificate ||
        patch_certificate_len != L3_LEN_PATCH_CERTIFICATE) {
        return L3_SECURITY_ERR_ARGUMENT;
    }
    if (core_is_authorized(core)) return L3_SECURITY_ERR_STATE;
    memcpy(core->session.pairing.patch_public65,
           patch_certificate + 0x0bu, L3_LEN_PATCH_PUBLIC_KEY);
    core->flags |= L3_CORE_FLAG_PATCH_CERTIFICATE;
    return 1;
#endif
}

#ifndef L3_SAVED_AUTH_ONLY
static int fill_ephemeral_private(l3_app_core *core, uint8_t out32[32]) {
    if (core->entropy) {
        return core->entropy(core->entropy_user, out32, 32u) == 1
                   ? L3_SECURITY_OK : L3_SECURITY_ERR_ENGINE;
    }
#if defined(L3_EXTERNAL_ENTROPY_ONLY)
    (void)out32;
    return L3_SECURITY_ERR_UNSUPPORTED;
#else
#ifdef L3_USE_CRYPTO_DLSYM
    if (!RAND_bytes) return L3_SECURITY_ERR_ENGINE;
#endif
    return RAND_bytes(out32, 32) == 1
               ? L3_SECURITY_OK : L3_SECURITY_ERR_ENGINE;
#endif
}
#endif

int l3_app_core_create_ephemeral_public_key_into(
    l3_app_core *core,
    uint8_t out64[L3_LEN_EPHEMERAL_PUBLIC_KEY]) {
#ifdef L3_SAVED_AUTH_ONLY
    (void)core;
    (void)out64;
    return L3_SECURITY_ERR_UNSUPPORTED;
#else
    if (!core || !out64) return L3_SECURITY_ERR_ARGUMENT;
    if (core_is_authorized(core) ||
        (core->flags & L3_CORE_FLAG_APP_KEY) == 0u) {
        return L3_SECURITY_ERR_STATE;
    }

    core->flags &= ~L3_CORE_FLAG_EPHEMERAL_KEY;
    for (unsigned attempt = 0; attempt < 16u; ++attempt) {
        int rc = fill_ephemeral_private(
            core, core->session.pairing.ephemeral_private32);
        if (rc != L3_SECURITY_OK) {
            secure_zero(core->session.pairing.ephemeral_private32, 32u);
            return rc;
        }
        if (l3_p256_public64_from_private32_libre_basepoint(
                core->session.pairing.ephemeral_private32, out64) == 0) {
            core->flags |= L3_CORE_FLAG_EPHEMERAL_KEY;
            return 1;
        }
    }
    secure_zero(core->session.pairing.ephemeral_private32, 32u);
    secure_zero(out64, L3_LEN_EPHEMERAL_PUBLIC_KEY);
    return L3_SECURITY_ERR_ENGINE;
#endif
}

size_t l3_app_core_authorization_scratch_size(void) {
#ifdef L3_SAVED_AUTH_ONLY
    return 0u;
#else
    return l3_authorization_shared_point_workspace_size();
#endif
}

size_t l3_app_core_authorization_scratch_alignment(void) {
#ifdef L3_SAVED_AUTH_ONLY
    return 1u;
#else
    return l3_authorization_shared_point_workspace_alignment();
#endif
}

int l3_app_core_set_authorization_scratch(
    l3_app_core *core,
    void *workspace,
    size_t workspace_size) {
    if (!core) return L3_SECURITY_ENGINE_ERR_ARGUMENT;
    if (!workspace && workspace_size == 0u) {
        core->authorization_scratch = NULL;
        core->authorization_scratch_size = 0u;
        return L3_SECURITY_ENGINE_OK;
    }
#ifdef L3_SAVED_AUTH_ONLY
    return L3_SECURITY_ENGINE_ERR_ARGUMENT;
#else
    const size_t required_size = l3_app_core_authorization_scratch_size();
    const size_t required_alignment =
        l3_app_core_authorization_scratch_alignment();
    if (!workspace || workspace_size < required_size ||
        (uintptr_t)workspace % required_alignment != 0u) {
        return L3_SECURITY_ENGINE_ERR_ARGUMENT;
    }
    core->authorization_scratch = workspace;
    core->authorization_scratch_size = workspace_size;
    return L3_SECURITY_ENGINE_OK;
#endif
}

#ifndef L3_SAVED_AUTH_ONLY
static const l3_authorization_digest_tables k_authorization_digest_tables = {
    k_dat40eb4cd, k_dat40eb4cd_logical_len,
    L3_AUTH_TABLE_PACK6_PREFIX7503_RAW_TAIL,
    k_dat40eb3ad, k_dat40eb3ad_logical_len,
    L3_AUTH_TABLE_RAW_U8
};
#endif

int l3_app_core_derive_root82_into(
    const uint8_t app_scalar35[35],
    const uint8_t ephemeral_private32[32],
    const uint8_t patch_certificate_public65[L3_LEN_PATCH_PUBLIC_KEY],
    const uint8_t patch_ephemeral_public65[L3_LEN_PATCH_PUBLIC_KEY],
    void *scratch,
    size_t scratch_size,
    uint8_t out82[L3_LEN_AUTHORIZATION_ROOT],
    uint32_t *stage_out) {
#ifdef L3_SAVED_AUTH_ONLY
    (void)app_scalar35;
    (void)ephemeral_private32;
    (void)patch_certificate_public65;
    (void)patch_ephemeral_public65;
    (void)scratch;
    (void)scratch_size;
    (void)out82;
    if (stage_out) *stage_out = L3_AUTH_ROOT_STAGE_NONE;
    return L3_SECURITY_ERR_UNSUPPORTED;
#else
    uint32_t stage = L3_AUTH_ROOT_STAGE_NONE;
#define SET_STAGE(value) do { \
        stage = (value); \
        if (stage_out) *stage_out = stage; \
    } while (0)
    if (stage_out) *stage_out = stage;
    if (!app_scalar35 || !ephemeral_private32 ||
        !patch_certificate_public65 || !patch_ephemeral_public65 ||
        !scratch || !out82 ||
        scratch_size < l3_app_core_authorization_scratch_size() ||
        (uintptr_t)scratch % l3_app_core_authorization_scratch_alignment() != 0u) {
        return L3_SECURITY_ERR_ARGUMENT;
    }
    if (patch_certificate_public65[0] != 0x04u ||
        patch_ephemeral_public65[0] != 0x04u) {
        return L3_SECURITY_ERR_ARGUMENT;
    }
    SET_STAGE(L3_AUTH_ROOT_STAGE_INPUTS_VALID);

    uint8_t ephemeral_scalar35[35];
    if (l3_p256_encode_private_scalar_to_native35(
            ephemeral_private32, ephemeral_scalar35) != 0) {
        return L3_SECURITY_ERR_ENGINE;
    }
    SET_STAGE(L3_AUTH_ROOT_STAGE_EPHEMERAL_SCALAR_READY);
    SET_STAGE(L3_AUTH_ROOT_STAGE_APP_SCALAR_READY);

    uint8_t ephemeral_frames[2u * L3_LEN_AUTHORIZATION_FRAME];
    int rc = l3_authorization_shared_point_frames_into(
        ephemeral_scalar35, patch_ephemeral_public65 + 1u,
        scratch, scratch_size, ephemeral_frames);
    secure_zero(ephemeral_scalar35, sizeof(ephemeral_scalar35));
    if (rc != L3_AUTHORIZATION_SHARED_POINT_OK) {
        secure_zero(ephemeral_frames, sizeof(ephemeral_frames));
        return L3_SECURITY_ERR_ENGINE;
    }
    SET_STAGE(L3_AUTH_ROOT_STAGE_EPHEMERAL_POINT_READY);

    const size_t pair_size = 2u * L3_LEN_AUTHORIZATION_FRAME;
    if (l3_authorization_root_digest_workspace_size() >
            scratch_size - pair_size) {
        secure_zero(ephemeral_frames, sizeof(ephemeral_frames));
        return L3_SECURITY_ERR_ENGINE;
    }
    uint8_t *app_frames = (uint8_t *)scratch + scratch_size - pair_size;
    rc = l3_authorization_shared_point_frames_into(
        app_scalar35, patch_certificate_public65 + 1u,
        scratch, scratch_size, app_frames);
    if (rc != L3_AUTHORIZATION_SHARED_POINT_OK) {
        secure_zero(ephemeral_frames, sizeof(ephemeral_frames));
        return L3_SECURITY_ERR_ENGINE;
    }
    SET_STAGE(L3_AUTH_ROOT_STAGE_APP_POINT_READY);
    SET_STAGE(L3_AUTH_ROOT_STAGE_FRAME_SHAPE_VALID);

    static const uint8_t prefix4[4] = {0x00u, 0x00u, 0x00u, 0x01u};
    rc = l3_authorization_frame_pairs_digest_extract_root16_with_workspace(
        &l3_authorization_shared_point_frame_tables,
        &k_authorization_digest_tables,
        ephemeral_frames, app_frames,
        prefix4, sizeof(prefix4),
        scratch, scratch_size - pair_size, out82);
    secure_zero(ephemeral_frames, sizeof(ephemeral_frames));
    if (rc != L3_AUTH_OK) return L3_SECURITY_ERR_ENGINE;
    SET_STAGE(L3_AUTH_ROOT_STAGE_DIGEST_READY);
    SET_STAGE(L3_AUTH_ROOT_STAGE_ROOT_READY);
#undef SET_STAGE
    return 1;
#endif
}

int l3_app_core_derive_authorization_root(
    l3_app_core *core,
    const uint8_t *patch_ephemeral_public_key,
    size_t patch_ephemeral_public_key_len) {
#ifdef L3_SAVED_AUTH_ONLY
    (void)core;
    (void)patch_ephemeral_public_key;
    (void)patch_ephemeral_public_key_len;
    return L3_SECURITY_ERR_UNSUPPORTED;
#else
    if (!core || !patch_ephemeral_public_key ||
        patch_ephemeral_public_key_len != L3_LEN_PATCH_PUBLIC_KEY) {
        return L3_SECURITY_ERR_ARGUMENT;
    }
    const uint32_t required = L3_CORE_FLAG_APP_KEY |
                              L3_CORE_FLAG_PATCH_CERTIFICATE |
                              L3_CORE_FLAG_EPHEMERAL_KEY;
    if (core_is_authorized(core) || (core->flags & required) != required) {
        return L3_SECURITY_ERR_STATE;
    }

    void *scratch = core->authorization_scratch;
    const size_t scratch_size = l3_app_core_authorization_scratch_size();
    int owns_scratch = 0;
    if (!scratch) {
        scratch = malloc(scratch_size);
        if (!scratch) return L3_SECURITY_ERR_NOMEM;
        owns_scratch = 1;
    }

    uint8_t root82[L3_LEN_AUTHORIZATION_ROOT];
    int rc = l3_app_core_derive_root82_into(
        core->session.pairing.app_private_scalar35,
        core->session.pairing.ephemeral_private32,
        core->session.pairing.patch_public65,
        patch_ephemeral_public_key,
        scratch, scratch_size, root82,
        &core->authorization_root_stage);
    if (owns_scratch) {
        secure_zero(scratch, scratch_size);
        free(scratch);
    }
    if (rc != 1) {
        secure_zero(root82, sizeof(root82));
        return rc;
    }

    rc = install_authorization_frame(core, root82 + 16u);
    secure_zero(root82, sizeof(root82));
    if (rc == 1) {
        core->authorization_root_stage = L3_AUTH_ROOT_STAGE_ROOT_READY;
    }
    return rc;
#endif
}

static int encrypt_challenge_block(
    const l3_app_core *core,
    const uint8_t input[16],
    uint8_t output[16]) {
    return l3_challenge_cipher_context_encrypt(
        &k_challenge_tables,
        &core->session.authorized.challenge,
        input, output) == 0;
}

static void xor_block16(uint8_t left[16], const uint8_t right[16]) {
    for (unsigned i = 0; i < 16u; ++i) left[i] ^= right[i];
}

static void make_ccm_block(
    uint8_t block[16],
    uint8_t flags,
    const uint8_t nonce7[L3_LEN_CHALLENGE_NONCE],
    uint8_t final_byte) {
    memset(block, 0, 16u);
    block[0] = flags;
    memcpy(block + 1u, nonce7, L3_LEN_CHALLENGE_NONCE);
    block[15] = final_byte;
}

static int ccm_encrypt36(
    const l3_app_core *core,
    const uint8_t nonce7[L3_LEN_CHALLENGE_NONCE],
    const uint8_t plain36[L3_LEN_CHALLENGE_REPLY_PLAIN],
    uint8_t out40[L3_LEN_CHALLENGE_REPLY_CRYPT]) {
    uint8_t block[16];
    uint8_t mac[16];
    uint8_t stream[16];

    /* M=4, L=8, no associated data. */
    make_ccm_block(block, 0x0fu, nonce7, L3_LEN_CHALLENGE_REPLY_PLAIN);
    if (!encrypt_challenge_block(core, block, mac)) return 0;

    for (unsigned offset = 0u; offset < L3_LEN_CHALLENGE_REPLY_PLAIN;
         offset += 16u) {
        const unsigned remaining = L3_LEN_CHALLENGE_REPLY_PLAIN - offset;
        const unsigned count = remaining < 16u ? remaining : 16u;
        memset(block, 0, sizeof(block));
        memcpy(block, plain36 + offset, count);
        xor_block16(mac, block);
        if (!encrypt_challenge_block(core, mac, mac)) return 0;
    }

    for (unsigned offset = 0u, counter = 1u;
         offset < L3_LEN_CHALLENGE_REPLY_PLAIN;
         offset += 16u, ++counter) {
        const unsigned remaining = L3_LEN_CHALLENGE_REPLY_PLAIN - offset;
        const unsigned count = remaining < 16u ? remaining : 16u;
        make_ccm_block(block, 0x07u, nonce7, (uint8_t)counter);
        if (!encrypt_challenge_block(core, block, stream)) return 0;
        for (unsigned i = 0u; i < count; ++i) {
            out40[offset + i] = (uint8_t)(plain36[offset + i] ^ stream[i]);
        }
    }

    make_ccm_block(block, 0x07u, nonce7, 0u);
    if (!encrypt_challenge_block(core, block, stream)) return 0;
    for (unsigned i = 0u; i < 4u; ++i) {
        out40[L3_LEN_CHALLENGE_REPLY_PLAIN + i] =
            (uint8_t)(mac[i] ^ stream[i]);
    }
    return 1;
}

static int ccm_decrypt56(
    const l3_app_core *core,
    const uint8_t nonce7[L3_LEN_CHALLENGE_NONCE],
    const uint8_t cipher60[L3_LEN_CHALLENGE_RESPONSE_CRYPT],
    uint8_t out56[L3_LEN_CHALLENGE_RESPONSE_PLAIN]) {
    uint8_t block[16];
    uint8_t mac[16];
    uint8_t stream[16];

    make_ccm_block(block, 0x0fu, nonce7,
                   L3_LEN_CHALLENGE_RESPONSE_PLAIN);
    if (!encrypt_challenge_block(core, block, mac)) return 0;

    for (unsigned offset = 0u, counter = 1u;
         offset < L3_LEN_CHALLENGE_RESPONSE_PLAIN;
         offset += 16u, ++counter) {
        const unsigned remaining = L3_LEN_CHALLENGE_RESPONSE_PLAIN - offset;
        const unsigned count = remaining < 16u ? remaining : 16u;
        make_ccm_block(block, 0x07u, nonce7, (uint8_t)counter);
        if (!encrypt_challenge_block(core, block, stream)) return 0;
        memset(block, 0, sizeof(block));
        for (unsigned i = 0u; i < count; ++i) {
            const uint8_t plain =
                (uint8_t)(cipher60[offset + i] ^ stream[i]);
            out56[offset + i] = plain;
            block[i] = plain;
        }
        xor_block16(mac, block);
        if (!encrypt_challenge_block(core, mac, mac)) return 0;
    }

    make_ccm_block(block, 0x07u, nonce7, 0u);
    if (!encrypt_challenge_block(core, block, stream)) return 0;
    uint8_t diff = 0u;
    for (unsigned i = 0u; i < 4u; ++i) {
        diff |= (uint8_t)((mac[i] ^ stream[i]) ^
                          cipher60[L3_LEN_CHALLENGE_RESPONSE_PLAIN + i]);
    }
    if (diff != 0u) {
        secure_zero(out56, L3_LEN_CHALLENGE_RESPONSE_PLAIN);
        return 0;
    }
    return 1;
}

int l3_app_core_encrypt_challenge_reply_into(
    l3_app_core *core,
    const uint8_t *nonce7,
    size_t nonce7_len,
    const uint8_t *plain36,
    size_t plain36_len,
    uint8_t out40[L3_LEN_CHALLENGE_REPLY_CRYPT]) {
    if (!core || !nonce7 || !plain36 || !out40 ||
        nonce7_len != L3_LEN_CHALLENGE_NONCE ||
        plain36_len != L3_LEN_CHALLENGE_REPLY_PLAIN) {
        return L3_SECURITY_ERR_ARGUMENT;
    }
    if (!core_is_authorized(core)) return L3_SECURITY_ERR_STATE;
    return ccm_encrypt36(core, nonce7, plain36, out40)
               ? 1 : L3_SECURITY_ERR_ENGINE;
}

int l3_app_core_decrypt_challenge_response_into(
    l3_app_core *core,
    const uint8_t *nonce7,
    size_t nonce7_len,
    const uint8_t *cipher60,
    size_t cipher60_len,
    uint8_t out56[L3_LEN_CHALLENGE_RESPONSE_PLAIN]) {
    if (!core || !nonce7 || !cipher60 || !out56 ||
        nonce7_len != L3_LEN_CHALLENGE_NONCE ||
        cipher60_len != L3_LEN_CHALLENGE_RESPONSE_CRYPT) {
        return L3_SECURITY_ERR_ARGUMENT;
    }
    if (!core_is_authorized(core)) return L3_SECURITY_ERR_STATE;
    return ccm_decrypt56(core, nonce7, cipher60, out56)
               ? 1 : L3_SECURITY_ERR_ENGINE;
}

int l3_app_core_export_saved_authorization_into(
    const l3_app_core *core,
    uint8_t out149[L3_LEN_SAVED_AUTHORIZATION]) {
    if (!core || !out149) return L3_SECURITY_ERR_ARGUMENT;
    if (!core_is_authorized(core)) return L3_SECURITY_ERR_STATE;

    const uint8_t *frame = core->session.authorized.root_frame;
    uint8_t trailer20[20];
    memset(trailer20, 0, sizeof(trailer20));
    memcpy(trailer20, frame + 52u, 14u);
    const uint16_t checksum = frame_checksum16(frame);
    trailer20[14] = (uint8_t)checksum;
    trailer20[15] = (uint8_t)(checksum >> 8);

    const int rc = l3_saved_authorization_record_build_with_options(
        k_saved_magic4,
        frame + 48u,
        frame + 0u,
        frame + 16u,
        frame + 32u,
        frame + 0u,
        trailer20,
        out149);
    secure_zero(trailer20, sizeof(trailer20));
    return rc == L3_SAVED_AUTHORIZATION_OK
               ? 1 : L3_SECURITY_ERR_ENGINE;
}

int l3_app_core_debug_copy_authorization_root(
    const l3_app_core *core,
    uint32_t meta4[4],
    uint8_t *out,
    size_t out_cap,
    size_t *out_len) {
    if (!core || !out_len) return L3_SECURITY_ERR_ARGUMENT;
    *out_len = 0u;
    if (meta4) memset(meta4, 0, 4u * sizeof(meta4[0]));
    if (!core_is_authorized(core)) return 0;
    if (!out || out_cap < L3_LEN_AUTHORIZATION_ROOT) {
        *out_len = L3_LEN_AUTHORIZATION_ROOT;
        return L3_SECURITY_ERR_ARGUMENT;
    }
    if (meta4) {
        meta4[0] = 0x5184u;
        meta4[1] = 0x79ae1f61u;
        meta4[2] = 0u;
        meta4[3] = 0x10u;
    }
    memcpy(out, core->session.authorized.root_frame, 16u);
    memcpy(out + 16u, core->session.authorized.root_frame,
           L3_LEN_AUTHORIZATION_FRAME);
    *out_len = L3_LEN_AUTHORIZATION_ROOT;
    return 1;
}
