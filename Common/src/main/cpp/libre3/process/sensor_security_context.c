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
#include "sensor_security_context.h"
#ifdef L3_USE_CRYPTO_DLSYM
#include "openssl_loader.h"
#endif

#include <stdlib.h>
#include <string.h>

static void make_default_config(l3_security_engine_config *dst,
                                const l3_security_engine_config *src) {
    memset(dst, 0, sizeof(*dst));
    if (src) {
        *dst = *src;
    }
    /* Juggluco stores the native-shaped 149-byte authorization export. */
    if (!src || src->export_format == 0u) {
        dst->export_format = L3_SECURITY_ENGINE_EXPORT_NATIVE149;
    }

    /* Optional diagnostic/replay inputs. Explicit configuration wins; these
     * environment variables are only fallbacks for host/debug builds. */
    if (!dst->challenge_probe_dir_path) {
        const char *v = getenv("L3_BAR7_F407_PROBE_DIR");
        if (v && v[0]) dst->challenge_probe_dir_path = v;
    }
    if (!dst->challenge_constructor_frame_path) {
        const char *v = getenv("L3_BAR7_F407_CONSTRUCTOR_FRAME");
        if (v && v[0]) dst->challenge_constructor_frame_path = v;
    }
    if (!dst->challenge_block_payload_path) {
        const char *v = getenv("L3_BAR7_F407_PAYLOAD");
        if (v && v[0]) dst->challenge_block_payload_path = v;
    }
}

l3_sensor_security_context *l3_sensor_security_context_create(const l3_security_engine_config *config) {
    l3_sensor_security_context *context = (l3_sensor_security_context *)malloc(sizeof(*context));
    if (!context) return NULL;
    if (l3_sensor_security_context_init(context, config) != L3_SENSOR_SECURITY_OK) {
        free(context);
        return NULL;
    }
    return context;
}

void l3_sensor_security_context_destroy(l3_sensor_security_context *context) {
    if (!context) return;
    l3_sensor_security_context_clear(context);
    free(context);
}

int l3_sensor_security_context_init(l3_sensor_security_context *context,
                        const l3_security_engine_config *config) {
    if (!context) return L3_SENSOR_SECURITY_ERR_ARGUMENT;
#ifdef L3_USE_CRYPTO_DLSYM
    if (!l3_openssl_symbols_init()) return L3_SENSOR_SECURITY_ERR_INIT;
#endif
    memset(context, 0, sizeof(*context));

    l3_security_engine_config effective;
    make_default_config(&effective, config);
    int rc = l3_security_engine_init(&context->engine, &effective);
    if (rc != L3_SECURITY_ENGINE_OK) {
        memset(context, 0, sizeof(*context));
        return L3_SENSOR_SECURITY_ERR_INIT;
    }
    l3_handshake_state_init(&context->handshake, l3_security_engine_handshake_operations(&context->engine));
    context->ready = 1;
    return L3_SENSOR_SECURITY_OK;
}

void l3_sensor_security_context_clear(l3_sensor_security_context *context) {
    if (!context) return;
    if (context->ready) {
        l3_handshake_state_clear(&context->handshake);
    }
    memset(context, 0, sizeof(*context));
}

static int context_ready(const l3_sensor_security_context *context) {
    return (context && context->ready) ? L3_SENSOR_SECURITY_OK : L3_SENSOR_SECURITY_ERR_ARGUMENT;
}

int l3_sensor_security_begin_handshake(l3_sensor_security_context *context) {
    int rc = context_ready(context);
    if (rc != L3_SENSOR_SECURITY_OK) return rc;
    return l3_handshake_begin(&context->handshake);
}

int l3_sensor_security_load_app_key_and_saved_authorization(l3_sensor_security_context *context,
                                const uint8_t *app_private_key,
                                size_t app_private_key_len,
                                const uint8_t *saved_authorization,
                                size_t saved_authorization_len) {
    int rc = context_ready(context);
    if (rc != L3_SENSOR_SECURITY_OK) return rc;
    return l3_handshake_load_app_key_and_saved_authorization(&context->handshake,
                                            app_private_key,
                                            app_private_key_len,
                                            saved_authorization,
                                            saved_authorization_len);
}

int l3_sensor_security_set_patch_certificate(l3_sensor_security_context *context,
                                    const uint8_t *patch_certificate,
                                    size_t patch_certificate_len) {
    int rc = context_ready(context);
    if (rc != L3_SENSOR_SECURITY_OK) return rc;
    return l3_handshake_set_patch_certificate(&context->handshake,
                                                patch_certificate,
                                                patch_certificate_len);
}

int l3_sensor_security_create_ephemeral_public_key(l3_sensor_security_context *context,
                                       uint8_t **out,
                                       size_t *out_len) {
    int rc = context_ready(context);
    if (rc != L3_SENSOR_SECURITY_OK) return rc;
    return l3_handshake_generate_ephemeral_public_key(&context->handshake, out, out_len);
}

int l3_sensor_security_derive_authorization_root(l3_sensor_security_context *context,
                                     const uint8_t *patch_ephemeral_public_key,
                                     size_t patch_ephemeral_public_key_len) {
    int rc = context_ready(context);
    if (rc != L3_SENSOR_SECURITY_OK) return rc;
    return l3_handshake_derive_authorization_root(&context->handshake,
                                                 patch_ephemeral_public_key,
                                                 patch_ephemeral_public_key_len);
}

int l3_sensor_security_encrypt_challenge_reply(l3_sensor_security_context *context,
                                   const uint8_t *nonce7,
                                   size_t nonce7_len,
                                   const uint8_t *plain36,
                                   size_t plain36_len,
                                   uint8_t **out,
                                   size_t *out_len) {
    int rc = context_ready(context);
    if (rc != L3_SENSOR_SECURITY_OK) return rc;
    return l3_handshake_encrypt_challenge_reply(&context->handshake,
                                               nonce7,
                                               nonce7_len,
                                               plain36,
                                               plain36_len,
                                               out,
                                               out_len);
}

int l3_sensor_security_decrypt_challenge_response(l3_sensor_security_context *context,
                                      const uint8_t *nonce7,
                                      size_t nonce7_len,
                                      const uint8_t *cipher60,
                                      size_t cipher60_len,
                                      uint8_t **out,
                                      size_t *out_len) {
    int rc = context_ready(context);
    if (rc != L3_SENSOR_SECURITY_OK) return rc;
    return l3_handshake_decrypt_challenge_response(&context->handshake,
                                                  nonce7,
                                                  nonce7_len,
                                                  cipher60,
                                                  cipher60_len,
                                                  out,
                                                  out_len);
}

int l3_sensor_security_export_saved_authorization(l3_sensor_security_context *context,
                                    uint8_t **out,
                                    size_t *out_len) {
    int rc = context_ready(context);
    if (rc != L3_SENSOR_SECURITY_OK) return rc;
    return l3_handshake_export_saved_authorization(&context->handshake, out, out_len);
}
