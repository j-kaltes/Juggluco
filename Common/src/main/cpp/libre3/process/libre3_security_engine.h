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


typedef struct l3_security_engine_config {
    const uint8_t *master_secret;
    size_t master_secret_len;
    l3_entropy_callback entropy;
    void *entropy_user;
    uint32_t export_format; /* L3_SECURITY_ENGINE_EXPORT_*; zero selects native 149-byte export. */
    uint32_t challenge_key_mode; /* internal fallback mode for nonstandard legacy objects. */
} l3_security_engine_config;

typedef struct l3_security_engine {
    l3_handshake_operations operations;
    uint8_t master[32];
    l3_entropy_callback entropy;
    void *entropy_user;
    void *authorization_scratch;
    size_t authorization_scratch_size;
    uint32_t export_format;
    uint32_t challenge_key_mode;
    uint32_t authorization_root_stage; /* Last completed direct-derive stage. */
} l3_security_engine;

int l3_security_engine_init(l3_security_engine *provider,
                          const l3_security_engine_config *config);

const l3_handshake_operations *l3_security_engine_handshake_operations(l3_security_engine *provider);

/* Optional reusable scratch removes every allocator call from the two heavy
   shared-point derivations.  Without it, the Android-compatible path leases
   one temporary block and reuses it for both points. */
size_t l3_security_engine_authorization_scratch_size(void);
size_t l3_security_engine_authorization_scratch_alignment(void);
int l3_security_engine_set_authorization_scratch(
    l3_security_engine *provider,
    void *workspace,
    size_t workspace_size);

/* Challenge records use a 7-byte nonce and a 4-byte authentication field.
 * Encryption maps 36 plaintext bytes to 40 bytes; decryption maps a 60-byte
 * sensor response to 56 plaintext bytes.  The generated block transform is
 * selected from the authorization-root state before the CCM-like wrapper runs. */

#ifdef __cplusplus
}
#endif

#endif
