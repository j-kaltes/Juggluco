#ifndef L3_SENSOR_SECURITY_CONTEXT_H
#define L3_SENSOR_SECURITY_CONTEXT_H

#include "libre3_handshake.h"
#include "libre3_security_engine.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * One instance belongs to one Libre3GattCallback / sensor connection.
 * All mutable handshake material lives here; no sensor-dependent state is global.
 */
typedef struct l3_sensor_security_context {
    l3_security_engine engine;
    l3_handshake_state handshake;
    int ready;
} l3_sensor_security_context;

enum {
    L3_SENSOR_SECURITY_OK = 0,
    L3_SENSOR_SECURITY_ERR_ARGUMENT = -1,
    L3_SENSOR_SECURITY_ERR_INIT = -2
};

/* Lifetime. */
l3_sensor_security_context *l3_sensor_security_context_create(
    const l3_security_engine_config *config);
void l3_sensor_security_context_destroy(l3_sensor_security_context *context);

/* Stack/static variants used by tests and non-JNI callers. */
int l3_sensor_security_context_init(
    l3_sensor_security_context *context,
    const l3_security_engine_config *config);
void l3_sensor_security_context_clear(l3_sensor_security_context *context);

/* Libre 3 handshake operations used by KEYSCrypto / Libre3GattCallback. */
int l3_sensor_security_begin_handshake(l3_sensor_security_context *context);
int l3_sensor_security_select_app_key_and_saved_authorization(
    l3_sensor_security_context *context,
    unsigned security_version,
    const uint8_t *saved_authorization,
    size_t saved_authorization_len);
int l3_sensor_security_set_patch_certificate(
    l3_sensor_security_context *context,
    const uint8_t *patch_certificate,
    size_t patch_certificate_len);
int l3_sensor_security_create_ephemeral_public_key_into(
    l3_sensor_security_context *context,
    uint8_t out64[L3_LEN_EPHEMERAL_PUBLIC_KEY]);
int l3_sensor_security_derive_authorization_root(
    l3_sensor_security_context *context,
    const uint8_t *patch_ephemeral_public_key,
    size_t patch_ephemeral_public_key_len);
int l3_sensor_security_encrypt_challenge_reply_into(
    l3_sensor_security_context *context,
    const uint8_t *nonce7,
    size_t nonce7_len,
    const uint8_t *plain36,
    size_t plain36_len,
    uint8_t out40[L3_LEN_CHALLENGE_REPLY_CRYPT]);
int l3_sensor_security_decrypt_challenge_response_into(
    l3_sensor_security_context *context,
    const uint8_t *nonce7,
    size_t nonce7_len,
    const uint8_t *cipher60,
    size_t cipher60_len,
    uint8_t out56[L3_LEN_CHALLENGE_RESPONSE_PLAIN]);
int l3_sensor_security_export_saved_authorization_into(
    l3_sensor_security_context *context,
    uint8_t out149[L3_LEN_SAVED_AUTHORIZATION]);


/* Diagnostic copy of current authorization-root material.  Returns 1 when a
 * root is present and copied, 0 when no root is present, or a negative error.
 * The material is engine-private; this is for live differential logs only. */
int l3_sensor_security_debug_copy_authorization_root(
    const l3_sensor_security_context *context,
    uint32_t meta4[4],
    uint8_t *out,
    size_t out_cap,
    size_t *out_len);

#ifdef __cplusplus
}
#endif
#endif
