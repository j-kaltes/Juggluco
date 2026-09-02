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

#include "sensor_security_context.h"
#if defined(L3_USE_CRYPTO_DLSYM) && \
    !defined(L3_SAVED_AUTH_ONLY) && !defined(L3_EXTERNAL_ENTROPY_ONLY)
#include "openssl_loader.h"
#endif

#include <stdlib.h>
#include <string.h>

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
#if defined(L3_USE_CRYPTO_DLSYM) && \
    !defined(L3_SAVED_AUTH_ONLY) && !defined(L3_EXTERNAL_ENTROPY_ONLY)
    if (!l3_openssl_symbols_init()) return L3_SENSOR_SECURITY_ERR_INIT;
#endif
    memset(context, 0, sizeof(*context));

    int rc = l3_app_core_init(&context->core, config);
    if (rc != L3_SECURITY_ENGINE_OK) {
        memset(context, 0, sizeof(*context));
        return L3_SENSOR_SECURITY_ERR_INIT;
    }
    context->ready = 1;
    return L3_SENSOR_SECURITY_OK;
}

void l3_sensor_security_context_clear(l3_sensor_security_context *context) {
    if (!context) return;
    l3_app_core_clear(&context->core);
    context->ready = 0;
}

static int context_ready(const l3_sensor_security_context *context) {
    return (context && context->ready) ? L3_SENSOR_SECURITY_OK : L3_SENSOR_SECURITY_ERR_ARGUMENT;
}

int l3_sensor_security_begin_handshake(l3_sensor_security_context *context) {
    int rc = context_ready(context);
    if (rc != L3_SENSOR_SECURITY_OK) return rc;
    return l3_app_core_begin(&context->core);
}

size_t l3_sensor_security_authorization_scratch_size(void) {
    return l3_app_core_authorization_scratch_size();
}

size_t l3_sensor_security_authorization_scratch_alignment(void) {
    return l3_app_core_authorization_scratch_alignment();
}

int l3_sensor_security_set_authorization_scratch(
    l3_sensor_security_context *context,
    void *workspace,
    size_t workspace_size) {
    int rc = context_ready(context);
    if (rc != L3_SENSOR_SECURITY_OK) return rc;
    return l3_app_core_set_authorization_scratch(
        &context->core, workspace, workspace_size);
}

int l3_sensor_security_select_app_key_and_saved_authorization(l3_sensor_security_context *context,
                                unsigned security_version,
                                const uint8_t *saved_authorization,
                                size_t saved_authorization_len) {
    int rc = context_ready(context);
    if (rc != L3_SENSOR_SECURITY_OK) return rc;
    return l3_app_core_select_app_key_and_saved_authorization(
        &context->core, security_version,
        saved_authorization, saved_authorization_len);
}

int l3_sensor_security_set_patch_certificate(l3_sensor_security_context *context,
                                    const uint8_t *patch_certificate,
                                    size_t patch_certificate_len) {
    int rc = context_ready(context);
    if (rc != L3_SENSOR_SECURITY_OK) return rc;
    return l3_app_core_set_patch_certificate(
        &context->core, patch_certificate, patch_certificate_len);
}

int l3_sensor_security_create_ephemeral_public_key_into(l3_sensor_security_context *context,
                                       uint8_t out64[L3_LEN_EPHEMERAL_PUBLIC_KEY]) {
    int rc = context_ready(context);
    if (rc != L3_SENSOR_SECURITY_OK) return rc;
    return l3_app_core_create_ephemeral_public_key_into(&context->core, out64);
}

int l3_sensor_security_derive_authorization_root(l3_sensor_security_context *context,
                                     const uint8_t *patch_ephemeral_public_key,
                                     size_t patch_ephemeral_public_key_len) {
    int rc = context_ready(context);
    if (rc != L3_SENSOR_SECURITY_OK) return rc;
    return l3_app_core_derive_authorization_root(
        &context->core, patch_ephemeral_public_key,
        patch_ephemeral_public_key_len);
}

int l3_sensor_security_encrypt_challenge_reply_into(l3_sensor_security_context *context,
                                   const uint8_t *nonce7,
                                   size_t nonce7_len,
                                   const uint8_t *plain36,
                                   size_t plain36_len,
                                   uint8_t out40[L3_LEN_CHALLENGE_REPLY_CRYPT]) {
    int rc = context_ready(context);
    if (rc != L3_SENSOR_SECURITY_OK) return rc;
    return l3_app_core_encrypt_challenge_reply_into(
        &context->core, nonce7, nonce7_len,
        plain36, plain36_len, out40);
}

int l3_sensor_security_decrypt_challenge_response_into(l3_sensor_security_context *context,
                                      const uint8_t *nonce7,
                                      size_t nonce7_len,
                                      const uint8_t *cipher60,
                                      size_t cipher60_len,
                                      uint8_t out56[L3_LEN_CHALLENGE_RESPONSE_PLAIN]) {
    int rc = context_ready(context);
    if (rc != L3_SENSOR_SECURITY_OK) return rc;
    return l3_app_core_decrypt_challenge_response_into(
        &context->core, nonce7, nonce7_len,
        cipher60, cipher60_len, out56);
}

int l3_sensor_security_export_saved_authorization_into(l3_sensor_security_context *context,
                                    uint8_t out149[L3_LEN_SAVED_AUTHORIZATION]) {
    int rc = context_ready(context);
    if (rc != L3_SENSOR_SECURITY_OK) return rc;
    return l3_app_core_export_saved_authorization_into(&context->core, out149);
}

int l3_sensor_security_debug_copy_authorization_root(
    const l3_sensor_security_context *context,
    uint32_t meta4[4],
    uint8_t *out,
    size_t out_cap,
    size_t *out_len) {
    if (!context || !context->ready || !out_len) return L3_SENSOR_SECURITY_ERR_ARGUMENT;
    return l3_app_core_debug_copy_authorization_root(
        &context->core, meta4, out, out_cap, out_len);
}
