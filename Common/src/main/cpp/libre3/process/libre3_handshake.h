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
#ifndef L3_LIBRE3_HANDSHAKE_H
#define L3_LIBRE3_HANDSHAKE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Libre 3 security-handshake operations used by Juggluco. */
#define L3_SECURITY_OK 0
#define L3_SECURITY_ERR_ARGUMENT       (-1)
#define L3_SECURITY_ERR_STATE          (-2)
#define L3_SECURITY_ERR_ENGINE        (-3)
#define L3_SECURITY_ERR_SHORT_OUTPUT   (-4)
#define L3_SECURITY_ERR_UNSUPPORTED    (-5)
#define L3_SECURITY_ERR_NOMEM          (-6)

/* Sizes of values exchanged by KEYSCrypto and Libre3GattCallback. */
enum {
    L3_LEN_APP_PRIVATE_KEY = 165,
    L3_LEN_SAVED_AUTHORIZATION = 149,
    L3_LEN_PATCH_CERTIFICATE = 140,
    L3_LEN_PATCH_PUBLIC_KEY = 65,
    L3_LEN_EPHEMERAL_PUBLIC_KEY = 64,
    L3_LEN_EPHEMERAL_CERTIFICATE = 65,
    L3_LEN_CHALLENGE_NONCE = 7,
    L3_LEN_CHALLENGE_REPLY_PLAIN = 36,
    L3_LEN_CHALLENGE_REPLY_CRYPT = 40,
    L3_LEN_CHALLENGE_RESPONSE_CRYPT = 60,
    L3_LEN_CHALLENGE_RESPONSE_PLAIN = 56
};

/* Opaque value passed between security-engine stages.  The byte
 * representation is engine-private; the handshake layer only owns/releases it. */
typedef struct l3_security_material {
    uint32_t kind;
    uint32_t origin_tag;
    uint32_t metadata0;
    uint32_t metadata1;
    uint8_t *bytes;
    size_t len;
} l3_security_material;

/*
 * Security-engine operations named by protocol purpose.  There are deliberately no
 * numeric processint/processbar selectors here anymore.
 */
typedef struct l3_handshake_operations {
    void *user;

    int (*begin_session)(void *user);

    int (*decode_app_private_key)(void *user,
                                  const uint8_t *record,
                                  size_t record_len,
                                  l3_security_material **out_private_key);

    int (*restore_authorization_record)(void *user,
                                        const uint8_t *record,
                                        size_t record_len,
                                        l3_security_material **out_root);

    int (*prepare_app_private_key)(void *user,
                                   const l3_security_material *decoded_private_key,
                                   l3_security_material **out_prepared_private_key);

    int (*generate_ephemeral_keypair)(void *user,
                                      l3_security_material **out_keypair);

    int (*derive_shared_point)(void *user,
                               const l3_security_material *private_key,
                               const uint8_t peer_public_xy64[64],
                               l3_security_material **out_shared_point);

    int (*concatenate_shared_points)(void *user,
                                     const l3_security_material *first,
                                     const l3_security_material *second,
                                     l3_security_material **out_joined);

    int (*digest_shared_points)(void *user,
                                const l3_security_material *joined,
                                l3_security_material **out_digest_material);

    int (*extract_authorization_root)(void *user,
                                      const l3_security_material *digest_material,
                                      l3_security_material **out_root);

    int (*copy_ephemeral_public_key)(void *user,
                                     const l3_security_material *keypair,
                                     uint8_t out_public_xy64[L3_LEN_EPHEMERAL_PUBLIC_KEY]);

    int (*encrypt_challenge_reply)(void *user,
                                   const uint8_t nonce7[L3_LEN_CHALLENGE_NONCE],
                                   const uint8_t plain36[L3_LEN_CHALLENGE_REPLY_PLAIN],
                                   const l3_security_material *authorization_root,
                                   uint8_t **out,
                                   size_t *out_len);

    int (*decrypt_challenge_response)(void *user,
                                      const uint8_t nonce7[L3_LEN_CHALLENGE_NONCE],
                                      const uint8_t cipher60[L3_LEN_CHALLENGE_RESPONSE_CRYPT],
                                      const l3_security_material *authorization_root,
                                      uint8_t **out,
                                      size_t *out_len);

    int (*export_authorization_record)(void *user,
                                       const l3_security_material *authorization_root,
                                       uint8_t **out,
                                       size_t *out_len);

    void (*release_material)(void *user, l3_security_material *object);
} l3_handshake_operations;

typedef struct l3_handshake_state {
    const l3_handshake_operations *operations;
    l3_security_material *app_private_key;
    l3_security_material *authorization_root;
    l3_security_material *ephemeral_keypair;

    /* Uncompressed P-256 point (0x04 || X || Y) copied from the patch certificate. */
    uint8_t patch_public_key[L3_LEN_PATCH_PUBLIC_KEY];
    int patch_public_key_ready;
} l3_handshake_state;

void l3_handshake_state_init(l3_handshake_state *state, const l3_handshake_operations *operations);
void l3_handshake_state_clear(l3_handshake_state *state);

int l3_handshake_begin(l3_handshake_state *state);
int l3_handshake_load_app_key_and_saved_authorization(l3_handshake_state *state,
                                     const uint8_t *app_private_key,
                                     size_t app_private_key_len,
                                     const uint8_t *saved_authorization,
                                     size_t saved_authorization_len);
int l3_handshake_set_patch_certificate(l3_handshake_state *state,
                                         const uint8_t *patch_certificate,
                                         size_t patch_certificate_len);
int l3_handshake_generate_ephemeral_public_key(l3_handshake_state *state,
                                              uint8_t **out,
                                              size_t *out_len);
int l3_handshake_derive_authorization_root(l3_handshake_state *state,
                                         const uint8_t *patch_ephemeral_public_key,
                                         size_t patch_ephemeral_public_key_len);
int l3_handshake_encrypt_challenge_reply(l3_handshake_state *state,
                                        const uint8_t *nonce7,
                                        size_t nonce7_len,
                                        const uint8_t *plain36,
                                        size_t plain36_len,
                                        uint8_t **out,
                                        size_t *out_len);
int l3_handshake_decrypt_challenge_response(l3_handshake_state *state,
                                           const uint8_t *nonce7,
                                           size_t nonce7_len,
                                           const uint8_t *cipher60,
                                           size_t cipher60_len,
                                           uint8_t **out,
                                           size_t *out_len);
int l3_handshake_export_saved_authorization(l3_handshake_state *state,
                                         uint8_t **out,
                                         size_t *out_len);

l3_security_material *l3_security_material_new_copy(uint32_t kind,
                                                uint32_t origin_tag,
                                                uint32_t metadata0,
                                                uint32_t metadata1,
                                                const uint8_t *bytes,
                                                size_t len);
void l3_security_material_free(l3_security_material *object);

#ifdef __cplusplus
}
#endif
#endif
