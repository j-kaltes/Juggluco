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
#include "libre3_handshake.h"

#include <stdlib.h>
#include <string.h>

static void release_security_object(const l3_handshake_operations *operations,
                                    l3_security_material **slot) {
    if (!slot || !*slot) return;
    if (operations && operations->release_material) {
        operations->release_material(operations->user, *slot);
    } else {
        l3_security_material_free(*slot);
    }
    *slot = NULL;
}

l3_security_material *l3_security_material_new_copy(uint32_t kind,
                                                uint32_t origin_tag,
                                                uint32_t metadata0,
                                                uint32_t metadata1,
                                                const uint8_t *bytes,
                                                size_t len) {
    l3_security_material *object = (l3_security_material *)calloc(1, sizeof(*object));
    if (!object) return NULL;
    object->kind = kind;
    object->origin_tag = origin_tag;
    object->metadata0 = metadata0;
    object->metadata1 = metadata1;
    object->len = len;
    if (len) {
        if (!bytes) {
            free(object);
            return NULL;
        }
        object->bytes = (uint8_t *)malloc(len);
        if (!object->bytes) {
            free(object);
            return NULL;
        }
        memcpy(object->bytes, bytes, len);
    }
    return object;
}

void l3_security_material_free(l3_security_material *object) {
    if (!object) return;
    free(object->bytes);
    free(object);
}

void l3_handshake_state_init(l3_handshake_state *state,
                            const l3_handshake_operations *operations) {
    if (!state) return;
    memset(state, 0, sizeof(*state));
    state->operations = operations;
}

void l3_handshake_state_clear(l3_handshake_state *state) {
    if (!state) return;
    const l3_handshake_operations *operations = state->operations;
    release_security_object(operations, &state->app_private_key);
    release_security_object(operations, &state->authorization_root);
    release_security_object(operations, &state->ephemeral_keypair);
    memset(state->patch_public_key, 0, sizeof(state->patch_public_key));
    state->patch_public_key_ready = 0;
}

static int validate_operations(const l3_handshake_state *state) {
    return (!state || !state->operations) ? L3_SECURITY_ERR_ARGUMENT : L3_SECURITY_OK;
}

static int engine_operation_succeeded(int rc) {
    return rc == L3_SECURITY_OK;
}

static int validate_exact_length(size_t actual, size_t expected) {
    return actual == expected ? L3_SECURITY_OK : L3_SECURITY_ERR_ARGUMENT;
}

int l3_handshake_begin(l3_handshake_state *state) {
    int rc = validate_operations(state);
    if (rc) return rc;
    l3_handshake_state_clear(state);
    if (state->operations->begin_session) {
        rc = state->operations->begin_session(state->operations->user);
        if (!engine_operation_succeeded(rc)) return L3_SECURITY_ERR_ENGINE;
    }
    return 1;
}

int l3_handshake_load_app_key_and_saved_authorization(l3_handshake_state *state,
                                     const uint8_t *app_private_key_record,
                                     size_t app_private_key_record_len,
                                     const uint8_t *saved_authorization_record,
                                     size_t saved_authorization_record_len) {
    int rc = validate_operations(state);
    if (rc) return rc;
    if (!state->operations->decode_app_private_key ||
        !state->operations->restore_authorization_record) {
        return L3_SECURITY_ERR_UNSUPPORTED;
    }
    if (!app_private_key_record ||
        validate_exact_length(app_private_key_record_len, L3_LEN_APP_PRIVATE_KEY)) {
        return L3_SECURITY_ERR_ARGUMENT;
    }
    if ((saved_authorization_record == NULL) != (saved_authorization_record_len == 0)) {
        return L3_SECURITY_ERR_ARGUMENT;
    }
    if (saved_authorization_record &&
        validate_exact_length(saved_authorization_record_len, L3_LEN_SAVED_AUTHORIZATION)) {
        return L3_SECURITY_ERR_ARGUMENT;
    }

    release_security_object(state->operations, &state->authorization_root);
    release_security_object(state->operations, &state->app_private_key);

    rc = state->operations->decode_app_private_key(state->operations->user,
                                                app_private_key_record,
                                                app_private_key_record_len,
                                                &state->app_private_key);
    if (!engine_operation_succeeded(rc) || !state->app_private_key) return L3_SECURITY_ERR_ENGINE;

    if (saved_authorization_record) {
        rc = state->operations->restore_authorization_record(state->operations->user,
                                                          saved_authorization_record,
                                                          saved_authorization_record_len,
                                                          &state->authorization_root);
        if (!engine_operation_succeeded(rc) || !state->authorization_root) {
            return L3_SECURITY_ERR_ENGINE;
        }
    }
    return 1;
}

int l3_handshake_set_patch_certificate(l3_handshake_state *state,
                                         const uint8_t *patch_certificate,
                                         size_t patch_certificate_len) {
    if (!state || !patch_certificate) return L3_SECURITY_ERR_ARGUMENT;
    if (validate_exact_length(patch_certificate_len, L3_LEN_PATCH_CERTIFICATE)) {
        return L3_SECURITY_ERR_ARGUMENT;
    }
    /* Bytes 11..75 are the patch's uncompressed P-256 public key. */
    memcpy(state->patch_public_key, patch_certificate + 0x0b, sizeof(state->patch_public_key));
    state->patch_public_key_ready = 1;
    return 1;
}

int l3_handshake_generate_ephemeral_public_key(l3_handshake_state *state,
                                              uint8_t **out,
                                              size_t *out_len) {
    int rc = validate_operations(state);
    if (rc) return rc;
    if (!out || !out_len) return L3_SECURITY_ERR_ARGUMENT;
    *out = NULL;
    *out_len = 0;
    if (!state->operations->generate_ephemeral_keypair ||
        !state->operations->copy_ephemeral_public_key) {
        return L3_SECURITY_ERR_UNSUPPORTED;
    }

    release_security_object(state->operations, &state->ephemeral_keypair);
    rc = state->operations->generate_ephemeral_keypair(state->operations->user,
                                                     &state->ephemeral_keypair);
    if (!engine_operation_succeeded(rc) || !state->ephemeral_keypair) {
        return L3_SECURITY_ERR_ENGINE;
    }

    uint8_t *public_xy = (uint8_t *)malloc(L3_LEN_EPHEMERAL_PUBLIC_KEY);
    if (!public_xy) return L3_SECURITY_ERR_NOMEM;
    rc = state->operations->copy_ephemeral_public_key(state->operations->user,
                                                   state->ephemeral_keypair,
                                                   public_xy);
    if (!engine_operation_succeeded(rc)) {
        free(public_xy);
        return L3_SECURITY_ERR_ENGINE;
    }
    *out = public_xy;
    *out_len = L3_LEN_EPHEMERAL_PUBLIC_KEY;
    return 1;
}

int l3_handshake_derive_authorization_root(l3_handshake_state *state,
                                         const uint8_t *patch_ephemeral_public_key,
                                         size_t patch_ephemeral_public_key_len) {
    int rc = validate_operations(state);
    if (rc) return rc;
    if (!patch_ephemeral_public_key ||
        validate_exact_length(patch_ephemeral_public_key_len, L3_LEN_PATCH_PUBLIC_KEY)) {
        return L3_SECURITY_ERR_ARGUMENT;
    }
    if (!state->app_private_key || !state->ephemeral_keypair || !state->patch_public_key_ready) {
        return L3_SECURITY_ERR_STATE;
    }
    const l3_handshake_operations *operations = state->operations;
    if (!operations->prepare_app_private_key ||
        !operations->derive_shared_point ||
        !operations->concatenate_shared_points ||
        !operations->digest_shared_points ||
        !operations->extract_authorization_root) {
        return L3_SECURITY_ERR_UNSUPPORTED;
    }

    l3_security_material *prepared_app_private_key = NULL;
    l3_security_material *ephemeral_shared_point = NULL;
    l3_security_material *app_key_shared_point = NULL;
    l3_security_material *joined_shared_points = NULL;
    l3_security_material *digest_material = NULL;

    rc = operations->prepare_app_private_key(operations->user,
                                          state->app_private_key,
                                          &prepared_app_private_key);
    if (!engine_operation_succeeded(rc) || !prepared_app_private_key) goto fail;

    /* Public-key arrays are 0x04 || X || Y.  The P-256 scalar-multiply operation takes X || Y. */
    rc = operations->derive_shared_point(operations->user,
                                      state->ephemeral_keypair,
                                      patch_ephemeral_public_key + 1,
                                      &ephemeral_shared_point);
    if (!engine_operation_succeeded(rc) || !ephemeral_shared_point) goto fail;

    rc = operations->derive_shared_point(operations->user,
                                      prepared_app_private_key,
                                      state->patch_public_key + 1,
                                      &app_key_shared_point);
    if (!engine_operation_succeeded(rc) || !app_key_shared_point) goto fail;

    /* The private key objects are no longer needed after both shared points exist. */
    release_security_object(operations, &state->ephemeral_keypair);
    release_security_object(operations, &prepared_app_private_key);

    rc = operations->concatenate_shared_points(operations->user,
                                            ephemeral_shared_point,
                                            app_key_shared_point,
                                            &joined_shared_points);
    release_security_object(operations, &ephemeral_shared_point);
    release_security_object(operations, &app_key_shared_point);
    if (!engine_operation_succeeded(rc) || !joined_shared_points) goto fail;

    rc = operations->digest_shared_points(operations->user,
                                       joined_shared_points,
                                       &digest_material);
    release_security_object(operations, &joined_shared_points);
    if (!engine_operation_succeeded(rc) || !digest_material) goto fail;

    release_security_object(operations, &state->authorization_root);
    rc = operations->extract_authorization_root(operations->user,
                                             digest_material,
                                             &state->authorization_root);
    release_security_object(operations, &digest_material);
    if (!engine_operation_succeeded(rc) || !state->authorization_root) goto fail;

    return 1;

fail:
    release_security_object(operations, &prepared_app_private_key);
    release_security_object(operations, &ephemeral_shared_point);
    release_security_object(operations, &app_key_shared_point);
    release_security_object(operations, &joined_shared_points);
    release_security_object(operations, &digest_material);
    return L3_SECURITY_ERR_ENGINE;
}

int l3_handshake_encrypt_challenge_reply(l3_handshake_state *state,
                                        const uint8_t *nonce7,
                                        size_t nonce7_len,
                                        const uint8_t *plain36,
                                        size_t plain36_len,
                                        uint8_t **out,
                                        size_t *out_len) {
    int rc = validate_operations(state);
    if (rc) return rc;
    if (!nonce7 || !plain36 || !out || !out_len) return L3_SECURITY_ERR_ARGUMENT;
    if (validate_exact_length(nonce7_len, L3_LEN_CHALLENGE_NONCE) ||
        validate_exact_length(plain36_len, L3_LEN_CHALLENGE_REPLY_PLAIN)) {
        return L3_SECURITY_ERR_ARGUMENT;
    }
    if (!state->authorization_root) return L3_SECURITY_ERR_STATE;
    if (!state->operations->encrypt_challenge_reply) return L3_SECURITY_ERR_UNSUPPORTED;
    *out = NULL;
    *out_len = 0;
    rc = state->operations->encrypt_challenge_reply(state->operations->user,
                                                  nonce7,
                                                  plain36,
                                                  state->authorization_root,
                                                  out,
                                                  out_len);
    return engine_operation_succeeded(rc) ? 1 : L3_SECURITY_ERR_ENGINE;
}

int l3_handshake_decrypt_challenge_response(l3_handshake_state *state,
                                           const uint8_t *nonce7,
                                           size_t nonce7_len,
                                           const uint8_t *cipher60,
                                           size_t cipher60_len,
                                           uint8_t **out,
                                           size_t *out_len) {
    int rc = validate_operations(state);
    if (rc) return rc;
    if (!nonce7 || !cipher60 || !out || !out_len) return L3_SECURITY_ERR_ARGUMENT;
    if (validate_exact_length(nonce7_len, L3_LEN_CHALLENGE_NONCE) ||
        validate_exact_length(cipher60_len, L3_LEN_CHALLENGE_RESPONSE_CRYPT)) {
        return L3_SECURITY_ERR_ARGUMENT;
    }
    if (!state->authorization_root) return L3_SECURITY_ERR_STATE;
    if (!state->operations->decrypt_challenge_response) return L3_SECURITY_ERR_UNSUPPORTED;
    *out = NULL;
    *out_len = 0;
    rc = state->operations->decrypt_challenge_response(state->operations->user,
                                                     nonce7,
                                                     cipher60,
                                                     state->authorization_root,
                                                     out,
                                                     out_len);
    return engine_operation_succeeded(rc) ? 1 : L3_SECURITY_ERR_ENGINE;
}

int l3_handshake_export_saved_authorization(l3_handshake_state *state,
                                         uint8_t **out,
                                         size_t *out_len) {
    int rc = validate_operations(state);
    if (rc) return rc;
    if (!out || !out_len) return L3_SECURITY_ERR_ARGUMENT;
    if (!state->authorization_root) return L3_SECURITY_ERR_STATE;
    if (!state->operations->export_authorization_record) return L3_SECURITY_ERR_UNSUPPORTED;
    *out = NULL;
    *out_len = 0;
    rc = state->operations->export_authorization_record(state->operations->user,
                                                      state->authorization_root,
                                                      out,
                                                      out_len);
    return engine_operation_succeeded(rc) ? 1 : L3_SECURITY_ERR_ENGINE;
}
