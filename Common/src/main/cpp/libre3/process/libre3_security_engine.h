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
#ifndef L3_LIBRE3_SECURITY_ENGINE_H
#define L3_LIBRE3_SECURITY_ENGINE_H

#include "libre3_handshake.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define L3_SECURITY_ENGINE_OK 0
#define L3_SECURITY_ENGINE_ERR_ARGUMENT (-1)
#define L3_SECURITY_ENGINE_ERR_CRYPTO   (-2)
#define L3_SECURITY_ENGINE_ERR_NOMEM    (-3)

#define L3_SECURITY_ENGINE_EXPORT_LEGACY_SOFT     0u
#define L3_SECURITY_ENGINE_EXPORT_NATIVE149   1u

#define L3_SECURITY_ENGINE_CHALLENGE_CONTEXT_PAYLOAD_LEN 0x10b0u
#define L3_SECURITY_ENGINE_CHALLENGE_CONSTRUCTOR_FRAME_LEN 0x42u

/* Fallback challenge-key derivations retained from reconstruction experiments.
 * The app's exact generated path is preferred; value 0 selects the build default. */
#define L3_SECURITY_ENGINE_CHALLENGE_KEY_DEFAULT              0u
#define L3_SECURITY_ENGINE_CHALLENGE_KEY_HMAC_SHARED_POINT_KEY_OBJECT_DIGEST        1u
#define L3_SECURITY_ENGINE_CHALLENGE_KEY_SHA256_SHARED_POINT_THEN_OBJECT_DIGEST  2u
#define L3_SECURITY_ENGINE_CHALLENGE_KEY_SHA256_OBJECT_DIGEST_THEN_SHARED_POINT  3u
#define L3_SECURITY_ENGINE_CHALLENGE_KEY_HMAC_OBJECT_DIGEST_KEY_SHARED_POINT    4u
#define L3_SECURITY_ENGINE_CHALLENGE_KEY_SHA256_SHARED_POINT_THEN_RAW_OBJECT  5u

typedef int (*l3_entropy_callback)(void *user, uint8_t *out, size_t out_len);

/* The Libre 3 challenge cipher uses a generated 16-byte block transform.
 * This callback exposes that block boundary for testing or injected implementations.
 * It must deterministically encrypt one 16-byte block and return zero on success. */
typedef int (*l3_challenge_block_encrypt_callback)(void *user,
                                          const uint8_t in[16],
                                          uint8_t out[16]);

typedef struct l3_security_engine_config {
    const uint8_t *master_secret;
    size_t master_secret_len;
    l3_entropy_callback entropy;
    void *entropy_user;
    uint32_t export_format; /* L3_SECURITY_ENGINE_EXPORT_*; zero selects legacy test format. */
    uint32_t challenge_key_mode; /* L3_SECURITY_ENGINE_CHALLENGE_KEY_*; zero selects build default */
    l3_challenge_block_encrypt_callback challenge_block_encrypt;
    void *challenge_block_user;

    /* Optional captured challenge block-cipher state.  When supplied, challenge
     * encryption/decryption uses it instead of rebuilding the state from the
     * 66-byte authorization-root constructor frame.  Origin: f407 object payload. */
    const uint8_t *challenge_block_payload;
    size_t challenge_block_payload_len;

    /* Optional file containing captured challenge block-cipher state.  It may
     * contain either the 0x10b0-byte state payload or the 0x10b4-byte original
     * object dump (4-byte vtable word followed by the payload). */
    const char *challenge_block_payload_path;

    /* Optional 66-byte constructor frame for the generated challenge block
     * cipher.  Supplying it lets the engine rebuild the block-cipher state. */
    const uint8_t *challenge_constructor_frame;
    size_t challenge_constructor_frame_len;

    /* Optional file containing the 0x42-byte constructor frame. */
    const char *challenge_constructor_frame_path;

    /* Optional directory containing reconstruction/probe captures of the
     * challenge constructor frame or block-cipher state. */
    const char *challenge_probe_dir_path;
} l3_security_engine_config;

typedef struct l3_security_engine {
    l3_handshake_operations operations;
    uint8_t master[32];
    l3_entropy_callback entropy;
    void *entropy_user;
    uint64_t nonce_counter;
    uint32_t deterministic_entropy_fixture_seen; /* host/replay diagnostic; not sensor identity. */
    uint32_t export_format;
    uint32_t challenge_key_mode;
    l3_challenge_block_encrypt_callback challenge_block_encrypt;
    void *challenge_block_user;

    /* Optional replay capture of the generated challenge block-cipher state.
     * Kept separate from the authorization-root material because the native
     * implementation constructs a larger block-cipher object from that root. */
    uint32_t captured_challenge_block_payload_ready;
    uint32_t captured_peer_public_key_seen;
    uint8_t captured_challenge_block_payload[0x10b0u];

    /* Explicitly injected challenge block-cipher state for probes/tests. */
    uint32_t injected_challenge_block_payload_ready;
    uint8_t injected_challenge_block_payload[L3_SECURITY_ENGINE_CHALLENGE_CONTEXT_PAYLOAD_LEN];

    /* Lazy file import for challenge block-cipher state. */
    uint32_t challenge_block_payload_file_tried;
    char challenge_block_payload_path[512];

    /* Lazy file import for the 66-byte challenge constructor frame. */
    uint32_t challenge_constructor_frame_file_tried;
    char challenge_constructor_frame_path[512];

    /* Lazy probe-directory import. */
    uint32_t challenge_probe_dir_tried;
    char challenge_probe_dir_path[512];

    /* Sequence counter for optional authorization trace dumps. */
    uint64_t authorization_trace_counter;
} l3_security_engine;

int l3_security_engine_init(l3_security_engine *provider,
                          const l3_security_engine_config *config);

const l3_handshake_operations *l3_security_engine_handshake_operations(l3_security_engine *provider);

int l3_security_engine_set_challenge_block_payload(l3_security_engine *provider,
                                      const uint8_t *payload,
                                      size_t payload_len);


int l3_security_engine_set_challenge_block_payload_from_object_dump(l3_security_engine *provider,
                                                const uint8_t *self_dump,
                                                size_t self_len);

int l3_security_engine_set_challenge_block_payload_from_file(l3_security_engine *provider,
                                                const char *path);

int l3_security_engine_set_challenge_constructor_frame(l3_security_engine *provider,
                                                const uint8_t *frame66,
                                                size_t frame_len);

int l3_security_engine_set_challenge_constructor_frame_from_file(l3_security_engine *provider,
                                                          const char *path);

int l3_security_engine_load_challenge_material_from_probe_dir(l3_security_engine *provider,
                                             const char *dir_path);

/* Challenge records use a 7-byte nonce and a 4-byte authentication field.
 * Encryption maps 36 plaintext bytes to 40 bytes; decryption maps a 60-byte
 * sensor response to 56 plaintext bytes.  The generated block transform is
 * selected from the authorization-root state before the CCM-like wrapper runs. */

#ifdef __cplusplus
}
#endif

#endif
