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

int l3_sensor_security_select_app_key_and_saved_authorization(l3_sensor_security_context *context,
                                unsigned security_version,
                                const uint8_t *saved_authorization,
                                size_t saved_authorization_len) {
    int rc = context_ready(context);
    if (rc != L3_SENSOR_SECURITY_OK) return rc;
    return l3_handshake_select_app_key_and_saved_authorization(&context->handshake,
                                            security_version,
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

int l3_sensor_security_create_ephemeral_public_key_into(l3_sensor_security_context *context,
                                       uint8_t out64[L3_LEN_EPHEMERAL_PUBLIC_KEY]) {
    int rc = context_ready(context);
    if (rc != L3_SENSOR_SECURITY_OK) return rc;
    return l3_handshake_generate_ephemeral_public_key_into(&context->handshake, out64);
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

int l3_sensor_security_encrypt_challenge_reply_into(l3_sensor_security_context *context,
                                   const uint8_t *nonce7,
                                   size_t nonce7_len,
                                   const uint8_t *plain36,
                                   size_t plain36_len,
                                   uint8_t out40[L3_LEN_CHALLENGE_REPLY_CRYPT]) {
    int rc = context_ready(context);
    if (rc != L3_SENSOR_SECURITY_OK) return rc;
    return l3_handshake_encrypt_challenge_reply_into(&context->handshake,
                                                    nonce7,
                                                    nonce7_len,
                                                    plain36,
                                                    plain36_len,
                                                    out40);
}

int l3_sensor_security_decrypt_challenge_response_into(l3_sensor_security_context *context,
                                      const uint8_t *nonce7,
                                      size_t nonce7_len,
                                      const uint8_t *cipher60,
                                      size_t cipher60_len,
                                      uint8_t out56[L3_LEN_CHALLENGE_RESPONSE_PLAIN]) {
    int rc = context_ready(context);
    if (rc != L3_SENSOR_SECURITY_OK) return rc;
    return l3_handshake_decrypt_challenge_response_into(&context->handshake,
                                                       nonce7,
                                                       nonce7_len,
                                                       cipher60,
                                                       cipher60_len,
                                                       out56);
}

int l3_sensor_security_export_saved_authorization_into(l3_sensor_security_context *context,
                                    uint8_t out149[L3_LEN_SAVED_AUTHORIZATION]) {
    int rc = context_ready(context);
    if (rc != L3_SENSOR_SECURITY_OK) return rc;
    return l3_handshake_export_saved_authorization_into(&context->handshake, out149);
}

int l3_sensor_security_debug_copy_authorization_root(
    const l3_sensor_security_context *context,
    uint32_t meta4[4],
    uint8_t *out,
    size_t out_cap,
    size_t *out_len) {
    if (!context || !context->ready || !out_len) return L3_SENSOR_SECURITY_ERR_ARGUMENT;
    const l3_security_material *root = context->handshake.authorization_root;
    *out_len = 0;
    if (meta4) {
        meta4[0] = meta4[1] = meta4[2] = meta4[3] = 0u;
    }
    if (!root) return 0;
    if (meta4) {
        meta4[0] = root->kind;
        meta4[1] = root->origin_tag;
        meta4[2] = root->metadata0;
        meta4[3] = root->metadata1;
    }
    *out_len = root->len;
    if (root->len && (!out || out_cap < root->len)) {
        return L3_SENSOR_SECURITY_ERR_ARGUMENT;
    }
    if (root->len) {
        memcpy(out, root->bytes, root->len);
    }
    return 1;
}
