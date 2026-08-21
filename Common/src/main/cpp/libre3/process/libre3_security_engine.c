#include "libre3_security_engine.h"

#include "saved_authorization_record.h"
#include "authorization_shared_point_core.h"
#include "authorization_shared_point_frames.h"
#include "authorization_digest.h"
#include "p256_private_scalar_codec.h"
#include "p256_scalar_multiply.h"
#include "challenge_cipher_context.h"
#include "challenge_block_constants.inc"
#include "fixed_app_private_keys.h"
#include "whitebox_lookup_table.h"
#include "whitebox_lookup_table.inc"
#include "authorization_digest_tables.inc"

#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include "openssl_symbols.h"
#include "openssl_function_redirects.h"

#include <stdlib.h>
#include <string.h>

#define MATERIAL_KIND_IMPORTED_PRIVATE_KEY 0x5188u
#define MATERIAL_KIND_AUTHORIZATION_ROOT     0x5184u
#define MATERIAL_KIND_EPHEMERAL_KEYPAIR    0x51acu
#define MATERIAL_KIND_PREPARED_PRIVATE_KEY     0x0721u
#define MATERIAL_KIND_TRANSFORMED_POINT    0x3000u
#define MATERIAL_KIND_JOINED_POINTS     0x306cu
#define MATERIAL_KIND_DIGESTED_POINTS      0x3080u
#define MATERIAL_KIND_SHARED_POINT     0xec00u
#define MATERIAL_KIND_ENCODED_POINT_FRAMES 0xf600u


/* Internal tags retained only to preserve the recovered object's metadata.
 * They are not part of the application/backend API. */
enum {
    MATERIAL_TAG_PREPARED_PRIVATE_KEY = 0x72111a45u,
    MATERIAL_TAG_EPHEMERAL_KEYPAIR = 0x67a70d0fu,
    MATERIAL_TAG_JOINED_SHARED_POINTS = 0x175b8a89u,
    MATERIAL_TAG_DIGESTED_SHARED_POINTS = 0x5d9235d5u,
    MATERIAL_TAG_AUTHORIZATION_ROOT = 0x79ae1f61u,
    MATERIAL_TAG_CHALLENGE_CIPHER = 0x414b0d3bu,
    MATERIAL_TAG_EXPORT_AUTHORIZATION = 0x71b91326u,
    CHALLENGE_OPERATION_ENCRYPT = 0x4c2c7e7fu,
    CHALLENGE_OPERATION_DECRYPT = 0x12d7a8f7u
};

typedef struct key_material_request {
    uint32_t material_tag;
    const l3_security_material *source_object;
    uint32_t requested_len;
} key_material_request;

typedef struct authorization_material_request {
    uint32_t material_tag;
    const l3_security_material *rhs_object;
    uint32_t digest_domain;
    uint32_t offset;
    uint32_t length;
} authorization_material_request;

#ifndef L3_SECURITY_ENGINE_DEFAULT_CHALLENGE_KEY_MODE
#define L3_SECURITY_ENGINE_DEFAULT_CHALLENGE_KEY_MODE L3_SECURITY_ENGINE_CHALLENGE_KEY_SHA256_SHARED_POINT_THEN_OBJECT_DIGEST
#endif

static void le32(uint8_t out[4], uint32_t v) {
    out[0] = (uint8_t)v;
    out[1] = (uint8_t)(v >> 8);
    out[2] = (uint8_t)(v >> 16);
    out[3] = (uint8_t)(v >> 24);
}


static int hmac_sha256(const uint8_t *key, size_t key_len,
                       const uint8_t *data, size_t data_len,
                       uint8_t out[32]) {
    unsigned int n = 0;
    if (!HMAC(EVP_sha256(), key, (int)key_len, data, data_len, out, &n) || n != 32u) {
        return L3_SECURITY_ENGINE_ERR_CRYPTO;
    }
    return L3_SECURITY_ENGINE_OK;
}

static int hmac_sha512(const uint8_t *key, size_t key_len,
                       const uint8_t *data, size_t data_len,
                       uint8_t out[64]) {
    unsigned int n = 0;
    if (!HMAC(EVP_sha512(), key, (int)key_len, data, data_len, out, &n) || n != 64u) {
        return L3_SECURITY_ENGINE_ERR_CRYPTO;
    }
    return L3_SECURITY_ENGINE_OK;
}

static int sha256_concat(const uint8_t *a, size_t an,
                         const uint8_t *b, size_t bn,
                         uint8_t out[32]) {
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    unsigned int n = 0;
    if (!ctx) return L3_SECURITY_ENGINE_ERR_NOMEM;
    int ok = EVP_DigestInit_ex(ctx, EVP_sha256(), NULL) &&
             EVP_DigestUpdate(ctx, a, an) &&
             EVP_DigestUpdate(ctx, b, bn) &&
             EVP_DigestFinal_ex(ctx, out, &n) && n == 32u;
    EVP_MD_CTX_free(ctx);
    return ok ? L3_SECURITY_ENGINE_OK : L3_SECURITY_ENGINE_ERR_CRYPTO;
}

static int derive32(l3_security_engine *p,
                    const char *label,
                    const uint8_t *a, size_t an,
                    const uint8_t *b, size_t bn,
                    uint32_t u0,
                    uint32_t u1,
                    uint8_t out[32]) {
    uint8_t hdr[32];
    size_t label_len = strlen(label);
    if (label_len > 16u) label_len = 16u;
    memset(hdr, 0, sizeof(hdr));
    memcpy(hdr, label, label_len);
    le32(hdr + 16, u0);
    le32(hdr + 20, u1);
    le32(hdr + 24, (uint32_t)an);
    le32(hdr + 28, (uint32_t)bn);

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    unsigned int n = 0;
    if (!ctx) return L3_SECURITY_ENGINE_ERR_NOMEM;
    int ok = EVP_DigestInit_ex(ctx, EVP_sha256(), NULL) &&
             EVP_DigestUpdate(ctx, p->master, sizeof(p->master)) &&
             EVP_DigestUpdate(ctx, hdr, sizeof(hdr)) &&
             (an == 0 || EVP_DigestUpdate(ctx, a, an)) &&
             (bn == 0 || EVP_DigestUpdate(ctx, b, bn)) &&
             EVP_DigestFinal_ex(ctx, out, &n) && n == 32u;
    EVP_MD_CTX_free(ctx);
    return ok ? L3_SECURITY_ENGINE_OK : L3_SECURITY_ENGINE_ERR_CRYPTO;
}

static int derive64(l3_security_engine *p,
                    const char *label,
                    const uint8_t *a, size_t an,
                    const uint8_t *b, size_t bn,
                    uint32_t u0,
                    uint32_t u1,
                    uint8_t out[64]) {
    uint8_t seed[32];
    int rc = derive32(p, label, a, an, b, bn, u0, u1, seed);
    if (rc) return rc;
    return hmac_sha512(p->master, sizeof(p->master), seed, sizeof(seed), out);
}




static int object_digest(l3_security_engine *p, const l3_security_material *o, uint8_t out[32]) {
    if (!p || !o) return L3_SECURITY_ENGINE_ERR_ARGUMENT;
    uint8_t hdr[16];
    le32(hdr + 0, o->kind);
    le32(hdr + 4, o->origin_tag);
    le32(hdr + 8, o->metadata0);
    le32(hdr + 12, o->metadata1);
    uint8_t tmp[32];
    int rc = sha256_concat(hdr, sizeof(hdr), o->bytes, o->len, tmp);
    if (rc) return rc;
    return hmac_sha256(p->master, sizeof(p->master), tmp, sizeof(tmp), out);
}

static int new_obj(uint32_t kind, uint32_t selector, uint32_t aux0, uint32_t aux1,
                   const uint8_t *bytes, size_t len, l3_security_material **out) {
    if (!out) return L3_SECURITY_ENGINE_ERR_ARGUMENT;
    *out = l3_security_material_new_copy(kind, selector, aux0, aux1, bytes, len);
    return *out ? L3_SECURITY_ENGINE_OK : L3_SECURITY_ENGINE_ERR_NOMEM;
}

static int p256_public64_from_private32_be(const uint8_t private32[32],
                                          uint8_t public64[64]) {
    if (!private32 || !public64) return L3_SECURITY_ENGINE_ERR_ARGUMENT;
    return l3_p256_public64_from_private32_libre_basepoint(private32, public64) == 0
               ? L3_SECURITY_ENGINE_OK
               : L3_SECURITY_ENGINE_ERR_CRYPTO;
}

static int p256_generate_public64_private32(l3_security_engine *p, uint8_t out96[96]) {
    if (!p || !out96) return L3_SECURITY_ENGINE_ERR_ARGUMENT;
#ifdef L3_USE_CRYPTO_DLSYM
    if (!RAND_bytes) return L3_SECURITY_ENGINE_ERR_CRYPTO;
#endif
    memset(out96, 0, 96u);

    /* Use a fresh app ephemeral scalar for each full no-KAuth handshake.  The
     * public point must use Libre's recovered base point, not OpenSSL's default
     * P-256 generator.  Generate it with the portable recovered multiplier so
     * the Android path performs no indirect EC_KEY/EC_POINT calls.  The stored
     * layout remains public64 || private32_be. */
    for (unsigned attempt = 0; attempt < 16u; ++attempt) {
        if (RAND_bytes(out96 + 64u, 32) != 1) {
            memset(out96, 0, 96u);
            return L3_SECURITY_ENGINE_ERR_CRYPTO;
        }
        int rc = p256_public64_from_private32_be(out96 + 64u, out96);
        if (rc == L3_SECURITY_ENGINE_OK) return L3_SECURITY_ENGINE_OK;
    }
    memset(out96, 0, 96u);
    return L3_SECURITY_ENGINE_ERR_CRYPTO;
}



static int reset_engine_session_state(void *user) {
    l3_security_engine *p = (l3_security_engine *)user;
    if (!p) return L3_SECURITY_ERR_ARGUMENT;
    p->authorization_root_stage = L3_AUTH_ROOT_STAGE_NONE;
    return L3_SECURITY_OK;
}

static int derive_transformed_point_native(l3_security_engine *p,
                                    const l3_security_material *object,
                                    const uint8_t *bytes,
                                    size_t len,
                                    uint32_t bit_count,
                                    const uint8_t *extra,
                                    size_t extra_len,
                                    l3_security_material **out);

static const uint8_t k_authorization_export_magic4[4] = {'L','3','S','9'};

static uint16_t authorization_export_frame_checksum16(const uint8_t frame[L3_SECURITY_ENGINE_CHALLENGE_CONSTRUCTOR_FRAME_LEN]) {
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < L3_SECURITY_ENGINE_CHALLENGE_CONSTRUCTOR_FRAME_LEN; ++i) {
        h = (h ^ frame[i]) * 16777619u;
    }
    return (uint16_t)((h >> 16) ^ h);
}

static int authorization_export_try_import_root149(const uint8_t *bytes, size_t len,
                                        l3_security_material **out) {
    if (!bytes || !out || len != L3_SAVED_AUTHORIZATION_RECORD_SIZE) return L3_SECURITY_ERR_ARGUMENT;
    if (memcmp(bytes + L3_SAVED_AUTHORIZATION_MAGIC_OFFSET, k_authorization_export_magic4, 4u) != 0) {
        return L3_SECURITY_ERR_ARGUMENT;
    }
    if (l3_saved_authorization_record_verify_header_mac(bytes, len) != L3_SAVED_AUTHORIZATION_OK) {
        return L3_SECURITY_ERR_ARGUMENT;
    }

    uint8_t frame[L3_SECURITY_ENGINE_CHALLENGE_CONSTRUCTOR_FRAME_LEN];
    memcpy(frame + 0u,  bytes + L3_SAVED_AUTHORIZATION_MANAGER_ID_OFFSET, 16u);
    memcpy(frame + 16u, bytes + L3_SAVED_AUTHORIZATION_PUBLIC_DATA_OFFSET, 16u);
    memcpy(frame + 32u, bytes + L3_SAVED_AUTHORIZATION_NONCE_OFFSET, 16u);
    memcpy(frame + 48u, bytes + L3_SAVED_AUTHORIZATION_PAYLOAD_TAG_OFFSET, 4u);
    memcpy(frame + 52u, bytes + L3_SAVED_AUTHORIZATION_TRAILER_OFFSET, 14u);

    const uint8_t *tr = bytes + L3_SAVED_AUTHORIZATION_TRAILER_OFFSET;
    uint16_t got = (uint16_t)tr[14] | ((uint16_t)tr[15] << 8);
    uint16_t exp = authorization_export_frame_checksum16(frame);
    if (got != exp) return L3_SECURITY_ERR_ARGUMENT;
    if (memcmp(bytes + L3_SAVED_AUTHORIZATION_ROOT_OFFSET, frame, 16u) != 0) {
        return L3_SECURITY_ERR_ARGUMENT;
    }

    uint8_t rootbuf[16u + L3_SECURITY_ENGINE_CHALLENGE_CONSTRUCTOR_FRAME_LEN];
    memcpy(rootbuf, frame, 16u);
    memcpy(rootbuf + 16u, frame, sizeof(frame));
    return new_obj(MATERIAL_KIND_AUTHORIZATION_ROOT, MATERIAL_TAG_AUTHORIZATION_ROOT, 0u, 0x10u,
                   rootbuf, sizeof(rootbuf), out) ? L3_SECURITY_ERR_ENGINE : L3_SECURITY_OK;
}

static int restore_saved_authorization_material(void *user, const uint8_t *bytes, size_t len,
                                                l3_security_material **out) {
    l3_security_engine *p = (l3_security_engine *)user;
    if (!p || !bytes || !len || !out) return L3_SECURITY_ERR_ARGUMENT;

    int irc = authorization_export_try_import_root149(bytes, len, out);
    if (irc == L3_SECURITY_OK) return L3_SECURITY_OK;

    uint8_t digest[32];
    int rc = derive32(p, "import.root", bytes, len, NULL, 0, 1u, 0u, digest);
    if (rc) return L3_SECURITY_ERR_ENGINE;

    const size_t obj_len = len + sizeof(digest);
    uint8_t *buf = (uint8_t *)malloc(obj_len);
    if (!buf) return L3_SECURITY_ERR_NOMEM;
    memcpy(buf, digest, sizeof(digest));
    memcpy(buf + sizeof(digest), bytes, len);
    rc = new_obj(MATERIAL_KIND_AUTHORIZATION_ROOT, 0, 1u, 0, buf, obj_len, out);
    free(buf);
    return rc ? L3_SECURITY_ERR_ENGINE : L3_SECURITY_OK;
}

static int create_private_or_ephemeral_key_material(void *user, const key_material_request *params, l3_security_material **out) {
    l3_security_engine *p = (l3_security_engine *)user;
    if (!p || !params || !out) return L3_SECURITY_ERR_ARGUMENT;
    *out = NULL;

    if (params->material_tag == MATERIAL_TAG_EPHEMERAL_KEYPAIR) {
        /*
         * Native processbar(5) is consumed by Java as an EC public key: Java
         * prefixes 0x04 and sends 0x04 || 64 bytes to the sensor.  Earlier soft
         * provider revisions returned arbitrary RAND_bytes here, which produces
         * an invalid P-256 point and the sensor disconnects after command 0x0e.
         *
         * Store public X||Y first so copy_object_bytes(0x40) returns the exact
         * Java-visible payload, and keep the private scalar after it for the
         * later processint(6) derivation path.
         */
        if (params->requested_len != 0u && params->requested_len != 0x40u) {
            return L3_SECURITY_ERR_ARGUMENT;
        }
        uint8_t keypair[96];
        int rc = p256_generate_public64_private32(p, keypair);
        if (rc) return L3_SECURITY_ERR_ENGINE;
        rc = new_obj(MATERIAL_KIND_EPHEMERAL_KEYPAIR, params->material_tag, 0x40u, 0x20u, keypair, sizeof(keypair), out);
        return rc ? L3_SECURITY_ERR_ENGINE : L3_SECURITY_OK;
    }

    if (params->material_tag == MATERIAL_TAG_PREPARED_PRIVATE_KEY) {
        if (!params->source_object ||
            params->source_object->kind != MATERIAL_KIND_IMPORTED_PRIVATE_KEY ||
            params->source_object->len < 35u) return L3_SECURITY_ERR_ARGUMENT;
        int rc;
        rc = new_obj(MATERIAL_KIND_PREPARED_PRIVATE_KEY, params->material_tag, params->source_object->kind, 0,
                     params->source_object->bytes, 35u, out);
        return rc ? L3_SECURITY_ERR_ENGINE : L3_SECURITY_OK;
    }

    return L3_SECURITY_ERR_UNSUPPORTED;
}

static int derive_shared_point_material(void *user, const l3_security_material *object, const uint8_t *bytes,
                          size_t len, uint32_t bit_count, l3_security_material **out) {
    l3_security_engine *p = (l3_security_engine *)user;
    if (!p || !object || !bytes || !out) return L3_SECURITY_ERR_ARGUMENT;
    if (object->kind == MATERIAL_KIND_EPHEMERAL_KEYPAIR && object->len >= 96u && object->metadata0 == 0x40u &&
        object->metadata1 == 0x20u && len == 0x40u && bit_count == 0x20u) {
        int rc = derive_transformed_point_native(
            p, object, bytes, len, bit_count, NULL, 0, out);
        if (rc == L3_SECURITY_OK) return L3_SECURITY_OK;
        return rc;
    }

    if ((object->kind == MATERIAL_KIND_PREPARED_PRIVATE_KEY || object->kind == MATERIAL_KIND_IMPORTED_PRIVATE_KEY ||
         object->kind == MATERIAL_KIND_AUTHORIZATION_ROOT) && len == 0x40u && bit_count == 0x20u) {
        int rc = derive_transformed_point_native(p, object, bytes, len, bit_count, NULL, 0, out);
        if (rc == L3_SECURITY_OK) return L3_SECURITY_OK;
    }

    uint8_t odig[32];
    int rc = object_digest(p, object, odig);
    if (rc) return L3_SECURITY_ERR_ENGINE;
    uint8_t out64[64];
    rc = derive64(p, "transform", object->bytes, object->len, bytes, len,
                  bit_count, object->origin_tag, out64);
    if (rc) return L3_SECURITY_ERR_ENGINE;
    uint32_t kind = MATERIAL_KIND_TRANSFORMED_POINT | (object->kind & 0xffu);
    rc = new_obj(kind, 0, bit_count, (uint32_t)len, out64, sizeof(out64), out);
    return rc ? L3_SECURITY_ERR_ENGINE : L3_SECURITY_OK;
}


static uint32_t effective_challenge_key_mode(const l3_security_engine *p) {
    uint32_t mode = p ? p->challenge_key_mode : 0u;
    if (mode == L3_SECURITY_ENGINE_CHALLENGE_KEY_DEFAULT) {
        mode = (uint32_t)L3_SECURITY_ENGINE_DEFAULT_CHALLENGE_KEY_MODE;
    }
    if (mode < L3_SECURITY_ENGINE_CHALLENGE_KEY_HMAC_SHARED_POINT_KEY_OBJECT_DIGEST ||
        mode > L3_SECURITY_ENGINE_CHALLENGE_KEY_SHA256_SHARED_POINT_THEN_RAW_OBJECT) {
        mode = L3_SECURITY_ENGINE_CHALLENGE_KEY_HMAC_SHARED_POINT_KEY_OBJECT_DIGEST;
    }
    return mode;
}

static int derive_shared_point_join_fallback(l3_security_engine *p,
                                     const l3_security_material *lhs_ecdh,
                                     const l3_security_material *rhs_object,
                                     uint8_t out32[32]) {
    if (!p || !lhs_ecdh || !rhs_object || !out32 || lhs_ecdh->len < 16u) {
        return L3_SECURITY_ENGINE_ERR_ARGUMENT;
    }
    uint8_t rhs_digest[32];
    if (object_digest(p, rhs_object, rhs_digest)) return L3_SECURITY_ENGINE_ERR_CRYPTO;
    uint32_t mode = effective_challenge_key_mode(p);

    if (mode == L3_SECURITY_ENGINE_CHALLENGE_KEY_HMAC_SHARED_POINT_KEY_OBJECT_DIGEST ||
        mode == L3_SECURITY_ENGINE_CHALLENGE_KEY_HMAC_OBJECT_DIGEST_KEY_SHARED_POINT) {
        const uint8_t *key = (mode == L3_SECURITY_ENGINE_CHALLENGE_KEY_HMAC_OBJECT_DIGEST_KEY_SHARED_POINT) ? rhs_digest : lhs_ecdh->bytes;
        size_t key_len = (mode == L3_SECURITY_ENGINE_CHALLENGE_KEY_HMAC_OBJECT_DIGEST_KEY_SHARED_POINT) ? sizeof(rhs_digest) : 16u;
        const uint8_t *data = (mode == L3_SECURITY_ENGINE_CHALLENGE_KEY_HMAC_OBJECT_DIGEST_KEY_SHARED_POINT) ? lhs_ecdh->bytes : rhs_digest;
        size_t data_len = (mode == L3_SECURITY_ENGINE_CHALLENGE_KEY_HMAC_OBJECT_DIGEST_KEY_SHARED_POINT) ? 16u : sizeof(rhs_digest);
        return hmac_sha256(key, key_len, data, data_len, out32);
    }

    if (mode == L3_SECURITY_ENGINE_CHALLENGE_KEY_SHA256_OBJECT_DIGEST_THEN_SHARED_POINT) {
        return sha256_concat(rhs_digest, sizeof(rhs_digest), lhs_ecdh->bytes, 16u, out32);
    }

    if (mode == L3_SECURITY_ENGINE_CHALLENGE_KEY_SHA256_SHARED_POINT_THEN_RAW_OBJECT) {
        return sha256_concat(lhs_ecdh->bytes, 16u, rhs_object->bytes, rhs_object->len, out32);
    }

    /* Default trial: bind the ECDH half to the private/certificate transform digest. */
    return sha256_concat(lhs_ecdh->bytes, 16u, rhs_digest, sizeof(rhs_digest), out32);
}

static int derive_authorization_digest_fallback(l3_security_engine *p,
                                    const l3_security_material *ecdh_seed,
                                    uint32_t scalar0,
                                    uint8_t out32[32]) {
    if (!p || !ecdh_seed || !out32 || ecdh_seed->len < 16u) return L3_SECURITY_ENGINE_ERR_ARGUMENT;
    uint8_t scalar[16];
    memset(scalar, 0, sizeof(scalar));
    le32(scalar + 0u, scalar0);
    le32(scalar + 4u, ecdh_seed->origin_tag);
    le32(scalar + 8u, ecdh_seed->metadata0);
    le32(scalar + 12u, ecdh_seed->metadata1);

    uint32_t mode = effective_challenge_key_mode(p);
    if (mode == L3_SECURITY_ENGINE_CHALLENGE_KEY_HMAC_SHARED_POINT_KEY_OBJECT_DIGEST ||
        mode == L3_SECURITY_ENGINE_CHALLENGE_KEY_HMAC_OBJECT_DIGEST_KEY_SHARED_POINT) {
        return hmac_sha256(ecdh_seed->bytes, ecdh_seed->len, scalar, sizeof(scalar), out32);
    }
    return sha256_concat(ecdh_seed->bytes, ecdh_seed->len, scalar, sizeof(scalar), out32);
}


static const l3_authorization_digest_tables g_authorization_digest_tables = {
    k_dat40eb4cd, k_dat40eb4cd_logical_len, L3_AUTH_TABLE_PACK6_PREFIX7503_RAW_TAIL,
    k_dat40eb3ad, k_dat40eb3ad_logical_len, L3_AUTH_TABLE_RAW_U8
};


static const l3_challenge_block_tables g_f407_tables = {
    k_dat426, sizeof(k_dat426),
    NULL, 0u
};

static int challenge_context_encrypt_block(void *user, const uint8_t in[16], uint8_t out[16]) {
    const l3_challenge_block_context *ctx =
        (const l3_challenge_block_context *)user;
    if (!ctx || !in || !out) return L3_SECURITY_ENGINE_ERR_ARGUMENT;
    return l3_challenge_cipher_context_encrypt(&g_f407_tables, ctx, in, out) == 0
        ? L3_SECURITY_ENGINE_OK : L3_SECURITY_ENGINE_ERR_CRYPTO;
}


typedef struct p6_state10a_ctx {
    l3_security_engine *provider;
    const l3_security_material *object;
    const uint8_t *extra;
    size_t extra_len;
} p6_state10a_ctx;













typedef struct p6_digest_ctx {
    l3_authorization_digest_context gen;
} p6_digest_ctx;

static int authorization_digest_init_cb(void *user) {
    p6_digest_ctx *d = (p6_digest_ctx *)user;
    if (!d) return -1;
    return l3_authorization_digest_init(&d->gen, &g_authorization_digest_tables, 0x20u) == L3_AUTHORIZATION_DIGEST_OK ? 0 : -1;
}

static int authorization_digest_update_cb(void *user, const uint8_t *bytes, size_t len) {
    p6_digest_ctx *d = (p6_digest_ctx *)user;
    if (!d || (len && !bytes)) return -1;
    return l3_authorization_digest_update(&d->gen, bytes, len) == L3_AUTHORIZATION_DIGEST_OK ? 0 : -1;
}

static int authorization_digest_update_frame_cb(void *user, const uint8_t frame66[L3_AUTH_FRAME_LEN], size_t byte_count) {
    p6_digest_ctx *d = (p6_digest_ctx *)user;
    if (!d || !frame66) return -1;
    return l3_authorization_digest_update_frame(&d->gen, frame66, byte_count) == L3_AUTHORIZATION_DIGEST_OK ? 0 : -1;
}

static int authorization_digest_final_cb(void *user, uint8_t out82[0x82]) {
    p6_digest_ctx *d = (p6_digest_ctx *)user;
    if (!d || !out82) return -1;
    return l3_authorization_digest_finalize_frame82(&d->gen, out82) == L3_AUTHORIZATION_DIGEST_OK ? 0 : -1;
}

static int new_encoded_frames_security_object(uint32_t selector,
                            const l3_authorization_frame_stream *fo, l3_security_material **out) {
    if (!fo || !out || (fo->frame_count && !fo->frames)) return L3_SECURITY_ENGINE_ERR_ARGUMENT;
    return new_obj(MATERIAL_KIND_ENCODED_POINT_FRAMES, selector, fo->byte_len, fo->frame_count,
                   fo->frames, (size_t)fo->frame_count * L3_AUTH_FRAME_LEN, out);
}

static int encoded_frames_from_security_object(const l3_security_material *o, l3_authorization_frame_stream *fo) {
    if (!o || !fo || o->kind != MATERIAL_KIND_ENCODED_POINT_FRAMES) return L3_AUTH_ERR_ARGUMENT;
    if (o->metadata1 == 0 || o->len != (size_t)o->metadata1 * L3_AUTH_FRAME_LEN) return L3_AUTH_ERR_STATE;
    return l3_authorization_frame_stream_copy(fo, 0, o->bytes, o->metadata1, o->metadata0);
}

static int derive_encoded_shared_point(l3_security_engine *p,
                                       const uint8_t tail35[35],
                                       const uint8_t input64[64],
                                       uint8_t out_frames[2u * L3_AUTH_FRAME_LEN]) {
    if (!tail35 || !input64 || !out_frames) return L3_SECURITY_ERR_ARGUMENT;
    const size_t scratch_size = l3_authorization_shared_point_workspace_size();
    void *scratch = p ? p->authorization_scratch : NULL;
    int owns_scratch = 0;
    if (!scratch) {
        scratch = malloc(scratch_size);
        if (!scratch) return L3_SECURITY_ERR_NOMEM;
        owns_scratch = 1;
    }
    const int core_rc = l3_authorization_shared_point_frames_into(
        tail35, input64, scratch, scratch_size, out_frames);
    if (owns_scratch) free(scratch);
    return core_rc == L3_AUTHORIZATION_SHARED_POINT_OK
               ? L3_SECURITY_OK
               : L3_SECURITY_ERR_ENGINE;
}

static int derive_transformed_point_native(l3_security_engine *p,
                                    const l3_security_material *object,
                                    const uint8_t *bytes,
                                    size_t len,
                                    uint32_t bit_count,
                                    const uint8_t *extra,
                                    size_t extra_len,
                                    l3_security_material **out) {
    if (!p || !object || !bytes || !out || len != 0x40u || bit_count != 0x20u) {
        return L3_SECURITY_ERR_ARGUMENT;
    }
    (void)extra;
    (void)extra_len;
    uint8_t tail35[35];
    if (object->kind == MATERIAL_KIND_EPHEMERAL_KEYPAIR && object->len >= 96u) {
        if (l3_p256_encode_private_scalar_to_native35(object->bytes + 64u, tail35) != 0) {
            return L3_SECURITY_ERR_ENGINE;
        }
    } else if ((object->kind == MATERIAL_KIND_PREPARED_PRIVATE_KEY ||
                object->kind == MATERIAL_KIND_IMPORTED_PRIVATE_KEY) && object->len >= 35u) {
        memcpy(tail35, object->bytes, sizeof(tail35));
    } else {
        return L3_SECURITY_ERR_ENGINE;
    }
    /* The old prepared/imported-key path materialized parent context_3 here.
     * Its eleven differing bytes are outside every live pipeline/finalizer
     * read, so both key kinds can use the immutable base without a heap copy. */
    uint8_t frames[2u * L3_AUTH_FRAME_LEN];
    int rc = derive_encoded_shared_point(p, tail35, bytes, frames);
    if (rc != L3_AUTH_OK) return L3_SECURITY_ERR_ENGINE;
    int nrc = new_obj(MATERIAL_KIND_ENCODED_POINT_FRAMES, 0u, 0x20u, 2u,
                      frames, sizeof(frames), out);
    return nrc ? L3_SECURITY_ERR_ENGINE : L3_SECURITY_OK;
}

#ifdef L3_EMBED_CHALLENGE_WHITEBOX_BLOCK_CIPHER
static void encode32_to_frame84_for_fe(const uint8_t in32[32], uint8_t out84[0x84]) {
    memset(out84, 0, 0x84u);
    for (unsigned j = 0; j < 64u; ++j) {
        out84[2u + j] = (uint8_t)((in32[j >> 2] >> ((j * 2u) & 6u)) & 3u);
        out84[0x42u + 2u + j] = (uint8_t)((in32[16u + (j >> 2)] >> ((j * 2u) & 6u)) & 3u);
    }
}
#endif


static int engine_derive_authorization_root_direct(void *user,
                                      const l3_security_material *app_private_key,
                                      const l3_security_material *ephemeral_keypair,
                                      const uint8_t patch_certificate_public65[L3_LEN_PATCH_PUBLIC_KEY],
                                      const uint8_t patch_ephemeral_public65[L3_LEN_PATCH_PUBLIC_KEY],
                                      l3_security_material **out_root) {
    l3_security_engine *p = (l3_security_engine *)user;
    if (p) p->authorization_root_stage = L3_AUTH_ROOT_STAGE_NONE;
    if (!app_private_key || !ephemeral_keypair || !patch_certificate_public65 ||
        !patch_ephemeral_public65 || !out_root) {
        return L3_SECURITY_ERR_ARGUMENT;
    }
    *out_root = NULL;
    if (patch_certificate_public65[0] != 0x04u || patch_ephemeral_public65[0] != 0x04u) {
        return L3_SECURITY_ERR_ARGUMENT;
    }
    if (p) p->authorization_root_stage = L3_AUTH_ROOT_STAGE_INPUTS_VALID;

    uint8_t eph_tail35[35];
    if (ephemeral_keypair->kind != MATERIAL_KIND_EPHEMERAL_KEYPAIR || ephemeral_keypair->len < 96u ||
        l3_p256_encode_private_scalar_to_native35(ephemeral_keypair->bytes + 64u, eph_tail35) != 0) {
        return L3_SECURITY_ERR_ENGINE;
    }
    if (p) p->authorization_root_stage = L3_AUTH_ROOT_STAGE_EPHEMERAL_SCALAR_READY;

    uint8_t app_tail35[35];
    if ((app_private_key->kind != MATERIAL_KIND_IMPORTED_PRIVATE_KEY &&
         app_private_key->kind != MATERIAL_KIND_PREPARED_PRIVATE_KEY) ||
        app_private_key->len < sizeof(app_tail35)) {
        return L3_SECURITY_ERR_ENGINE;
    }
    memcpy(app_tail35, app_private_key->bytes, sizeof(app_tail35));
    if (p) p->authorization_root_stage = L3_AUTH_ROOT_STAGE_APP_SCALAR_READY;

    uint8_t four_frames[4u * L3_AUTH_FRAME_LEN];
    const size_t scratch_size = l3_authorization_shared_point_workspace_size();
    void *scratch = p ? p->authorization_scratch : NULL;
    int owns_scratch = 0;
    if (!scratch) {
        scratch = malloc(scratch_size);
        if (!scratch) return L3_SECURITY_ERR_NOMEM;
        owns_scratch = 1;
    }

    int rc = l3_authorization_shared_point_frames_into(
        eph_tail35, patch_ephemeral_public65 + 1u,
        scratch, scratch_size, four_frames);
    if (rc != L3_AUTHORIZATION_SHARED_POINT_OK) {
        rc = L3_SECURITY_ERR_ENGINE;
        goto fail;
    }
    if (p) p->authorization_root_stage = L3_AUTH_ROOT_STAGE_EPHEMERAL_POINT_READY;

    /* See authorization_parent_contexts.inc and the size5 differential oracle. */
    rc = l3_authorization_shared_point_frames_into(
        app_tail35, patch_certificate_public65 + 1u,
        scratch, scratch_size,
        four_frames + 2u * L3_AUTH_FRAME_LEN);
    if (rc != L3_AUTHORIZATION_SHARED_POINT_OK) {
        rc = L3_SECURITY_ERR_ENGINE;
        goto fail;
    }
    if (owns_scratch) {
        free(scratch);
        scratch = NULL;
        owns_scratch = 0;
    }
    if (p) p->authorization_root_stage = L3_AUTH_ROOT_STAGE_APP_POINT_READY;
    if (p) p->authorization_root_stage = L3_AUTH_ROOT_STAGE_FRAME_SHAPE_VALID;

    uint8_t scalar[4];
    le32(scalar, 0x01000000u);
    uint8_t rootbuf[16u + L3_AUTH_FRAME_LEN];
    rc = l3_authorization_frames_digest_extract_root16(
                                                       &l3_authorization_shared_point_frame_tables,
                                                       &g_authorization_digest_tables,
                                                       four_frames, 4u, 0x40u,
                                                       scalar, sizeof(scalar),
                                                       rootbuf);
    if (rc != L3_AUTH_OK) {
        rc = L3_SECURITY_ERR_ENGINE;
        goto fail;
    }
    if (p) p->authorization_root_stage = L3_AUTH_ROOT_STAGE_DIGEST_READY;

    rc = new_obj(MATERIAL_KIND_AUTHORIZATION_ROOT, MATERIAL_TAG_AUTHORIZATION_ROOT, 0u, 0x10u,
                 rootbuf, sizeof(rootbuf), out_root);
    rc = rc ? L3_SECURITY_ERR_ENGINE : L3_SECURITY_OK;
    if (rc == L3_SECURITY_OK && p) {
        p->authorization_root_stage = L3_AUTH_ROOT_STAGE_ROOT_READY;
    }

fail:
    if (owns_scratch) free(scratch);
    return rc;
}

static int transform_authorization_material(void *user, const l3_security_material *object,
                       const authorization_material_request *params, l3_security_material **out) {
    l3_security_engine *p = (l3_security_engine *)user;
    if (!p || !object || !params || !out) return L3_SECURITY_ERR_ARGUMENT;
    *out = NULL;

    if (params->material_tag == MATERIAL_TAG_JOINED_SHARED_POINTS) {
        if (!params->rhs_object) return L3_SECURITY_ERR_ARGUMENT;
        if (object->kind == MATERIAL_KIND_ENCODED_POINT_FRAMES && params->rhs_object->kind == MATERIAL_KIND_ENCODED_POINT_FRAMES) {
            l3_authorization_frame_stream lhs, rhs, joined;
            memset(&lhs, 0, sizeof(lhs));
            memset(&rhs, 0, sizeof(rhs));
            memset(&joined, 0, sizeof(joined));
            int rc = encoded_frames_from_security_object(object, &lhs);
            if (!rc) rc = encoded_frames_from_security_object(params->rhs_object, &rhs);
            if (!rc) rc = l3_authorization_frames_concatenate(&lhs, &rhs, &joined);
            int nrc = rc ? L3_SECURITY_ERR_ENGINE : (new_encoded_frames_security_object(params->material_tag, &joined, out) ? L3_SECURITY_ERR_ENGINE : L3_SECURITY_OK);
            l3_authorization_frame_stream_clear(&lhs);
            l3_authorization_frame_stream_clear(&rhs);
            l3_authorization_frame_stream_clear(&joined);
            return nrc;
        }
        if (object->kind == MATERIAL_KIND_SHARED_POINT && object->len >= 16u) {
            /*
             * processint(6) joins the DB-key/sensor-ECDH half with the
             * private-key/patch-certificate transform.  The exact native
             * generated-object KDF is still the remaining live-device gap, so
             * keep this stage explicit and configurable rather than hiding it
             * behind packet-layout guesses.  Mode 1 reproduces the v174 path;
             * newer modes are build/app-selectable KDF trials over the same
             * native topology.
             */
            uint8_t joined[32];
            if (derive_shared_point_join_fallback(p, object, params->rhs_object, joined)) {
                return L3_SECURITY_ERR_ENGINE;
            }
            int rc = new_obj(MATERIAL_KIND_SHARED_POINT, params->material_tag, object->kind,
                             params->rhs_object->kind, joined, sizeof(joined), out);
            return rc ? L3_SECURITY_ERR_ENGINE : L3_SECURITY_OK;
        }
        uint8_t lhs[32], rhs[32], buf[64];
        if (object_digest(p, object, lhs) || object_digest(p, params->rhs_object, rhs)) return L3_SECURITY_ERR_ENGINE;
        memcpy(buf, lhs, 32);
        memcpy(buf + 32, rhs, 32);
        int rc = new_obj(MATERIAL_KIND_JOINED_POINTS, params->material_tag, object->kind, params->rhs_object->kind,
                         buf, sizeof(buf), out);
        return rc ? L3_SECURITY_ERR_ENGINE : L3_SECURITY_OK;
    }

    if (params->material_tag == MATERIAL_TAG_DIGESTED_SHARED_POINTS) {
        if (object->kind == MATERIAL_KIND_ENCODED_POINT_FRAMES) {
            l3_authorization_frame_stream in, mixed;
            memset(&in, 0, sizeof(in));
            memset(&mixed, 0, sizeof(mixed));
            uint8_t scalar[4];
            le32(scalar, params->digest_domain);
            p6_digest_ctx du;
            memset(&du, 0, sizeof(du));
            l3_authorization_digest_callbacks dcb;
            memset(&dcb, 0, sizeof(dcb));
            dcb.user = &du;
            dcb.init = authorization_digest_init_cb;
            dcb.update = authorization_digest_update_cb;
            dcb.update_frame = authorization_digest_update_frame_cb;
            dcb.final82 = authorization_digest_final_cb;
            int rc = encoded_frames_from_security_object(object, &in);
            if (!rc) rc = l3_authorization_frames_digest(
                &l3_authorization_shared_point_frame_tables,
                &in, scalar, sizeof(scalar), NULL, 0, &dcb, &mixed);
            int nrc = rc ? L3_SECURITY_ERR_ENGINE : (new_encoded_frames_security_object(params->material_tag, &mixed, out) ? L3_SECURITY_ERR_ENGINE : L3_SECURITY_OK);
            l3_authorization_frame_stream_clear(&in);
            l3_authorization_frame_stream_clear(&mixed);
            return nrc;
        }
        if (object->kind == MATERIAL_KIND_SHARED_POINT && object->len >= 16u) {
            uint8_t mixed[32];
            if (derive_authorization_digest_fallback(p, object, params->digest_domain, mixed)) {
                return L3_SECURITY_ERR_ENGINE;
            }
            int rc = new_obj(MATERIAL_KIND_SHARED_POINT, params->material_tag, params->digest_domain, 0,
                             mixed, sizeof(mixed), out);
            return rc ? L3_SECURITY_ERR_ENGINE : L3_SECURITY_OK;
        }
        uint8_t out64[64];
        int rc = derive64(p, "mix", object->bytes, object->len, NULL, 0,
                          params->digest_domain, object->kind, out64);
        if (rc) return L3_SECURITY_ERR_ENGINE;
        rc = new_obj(MATERIAL_KIND_DIGESTED_POINTS, params->material_tag, params->digest_domain, 0, out64, sizeof(out64), out);
        return rc ? L3_SECURITY_ERR_ENGINE : L3_SECURITY_OK;
    }

    if (params->material_tag == MATERIAL_TAG_AUTHORIZATION_ROOT) {
        if (object->kind == MATERIAL_KIND_ENCODED_POINT_FRAMES) {
            l3_authorization_frame_stream in, sliced;
            memset(&in, 0, sizeof(in));
            memset(&sliced, 0, sizeof(sliced));
            int rc = encoded_frames_from_security_object(object, &in);
            if (!rc) rc = l3_authorization_frames_extract_range(
                &l3_authorization_shared_point_frame_tables,
                &in, params->offset, params->length, &sliced);
            if (rc) {
                l3_authorization_frame_stream_clear(&in);
                l3_authorization_frame_stream_clear(&sliced);
                return L3_SECURITY_ERR_ENGINE;
            }
            if (params->offset == 0u && params->length == 0x10u && sliced.frame_count >= 1u) {
                /*
                 * Native DAT_f4295184 is still a generated 0x10-byte object,
                 * not a raw AES key and not the two-frame 0x20 FE object.  The
                 * BAR7 resolver for 0x10 roots selects FUN_f4072318, whose
                 * constructor consumes the 0x42 encoded frame from the sliced
                 * object and builds the FUN_f4070b78 block context.  Preserve
                 * that exact frame next to the 16-byte digest used only as a
                 * stable fallback/export prefix.
                 */
                uint8_t rootbuf[16u + L3_AUTH_FRAME_LEN];
                memset(rootbuf, 0, sizeof(rootbuf));
                /* V405: native DAT_f4295184 advertises a 0x10-byte visible length,
                 * but the data pointer that FUN_f4072318 receives points at the
                 * 0x42-byte generated constructor frame.  The visible 16 bytes are
                 * the first 16 bytes of that frame, not an independent SHA/HMAC
                 * digest.  Preserve the full frame after the visible prefix for the
                 * host-side f407 builder while making the visible prefix native-shaped. */
                /* The generic selector-6 path is now native-exact.  Never replace
                 * this computed frame with a captured-session constructor frame. */
                const uint8_t *constructor_frame = sliced.frames;
                memcpy(rootbuf, constructor_frame, 16u);
                memcpy(rootbuf + 16u, constructor_frame, L3_AUTH_FRAME_LEN);
                int nrc = new_obj(MATERIAL_KIND_AUTHORIZATION_ROOT, params->material_tag, params->offset, params->length,
                                  rootbuf, sizeof(rootbuf), out);
                l3_authorization_frame_stream_clear(&in);
                l3_authorization_frame_stream_clear(&sliced);
                return nrc ? L3_SECURITY_ERR_ENGINE : L3_SECURITY_OK;
            }
            int nrc = new_encoded_frames_security_object(params->material_tag, &sliced, out);
            l3_authorization_frame_stream_clear(&in);
            l3_authorization_frame_stream_clear(&sliced);
            return nrc ? L3_SECURITY_ERR_ENGINE : L3_SECURITY_OK;
        }
        if (object->kind == MATERIAL_KIND_SHARED_POINT && object->len >= 16u) {
            if (params->length == 0 || params->offset > object->len ||
                params->length > object->len - params->offset) return L3_SECURITY_ERR_ARGUMENT;
#ifdef L3_EMBED_CHALLENGE_WHITEBOX_BLOCK_CIPHER
            if (params->offset == 0u && params->length == 0x10u && object->len >= 32u) {
                uint8_t rootbuf[16u + 0x84u];
                memcpy(rootbuf, object->bytes, 16u);
                encode32_to_frame84_for_fe(object->bytes, rootbuf + 16u);
                int rc = new_obj(MATERIAL_KIND_AUTHORIZATION_ROOT, params->material_tag, params->offset, params->length,
                                 rootbuf, sizeof(rootbuf), out);
                return rc ? L3_SECURITY_ERR_ENGINE : L3_SECURITY_OK;
            }
#endif
            int rc = new_obj(MATERIAL_KIND_AUTHORIZATION_ROOT, params->material_tag, params->offset, params->length,
                             object->bytes + params->offset, params->length, out);
            return rc ? L3_SECURITY_ERR_ENGINE : L3_SECURITY_OK;
        }
        if (params->length == 0 || params->offset > object->len ||
            params->length > object->len - params->offset) return L3_SECURITY_ERR_ARGUMENT;
        uint8_t key[32];
        int rc = derive32(p, "root.slice", object->bytes + params->offset, params->length,
                          object->bytes, object->len, params->offset, params->length, key);
        if (rc) return L3_SECURITY_ERR_ENGINE;
        rc = new_obj(MATERIAL_KIND_AUTHORIZATION_ROOT, params->material_tag, params->offset, params->length,
                     key, 16u, out);
        return rc ? L3_SECURITY_ERR_ENGINE : L3_SECURITY_OK;
    }

    return L3_SECURITY_ERR_UNSUPPORTED;
}

static int copy_ephemeral_public_key_material(void *user, const l3_security_material *object, uint8_t *out, size_t out_len) {
    (void)user;
    if (!object || !out) return L3_SECURITY_ERR_ARGUMENT;
    if (object->kind != MATERIAL_KIND_EPHEMERAL_KEYPAIR) return L3_SECURITY_ERR_ENGINE;
    if (object->len >= 96u && object->metadata0 == 0x40u && object->metadata1 == 0x20u) {
        if (out_len > 0x40u) return L3_SECURITY_ERR_ENGINE;
        memcpy(out, object->bytes, out_len);
        return L3_SECURITY_OK;
    }
    if (out_len > object->len) return L3_SECURITY_ERR_ENGINE;
    memcpy(out, object->bytes, out_len);
    return L3_SECURITY_OK;
}

static int aes_ctr_xor(const uint8_t key32[32], const uint8_t iv16[16],
                       const uint8_t *in, size_t in_len, uint8_t *out) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return L3_SECURITY_ENGINE_ERR_NOMEM;
    int outl = 0, finl = 0;
    int ok = EVP_EncryptInit_ex(ctx, EVP_aes_256_ctr(), NULL, key32, iv16) &&
             EVP_EncryptUpdate(ctx, out, &outl, in, (int)in_len) &&
             EVP_EncryptFinal_ex(ctx, out + outl, &finl) &&
             (size_t)(outl + finl) == in_len;
    EVP_CIPHER_CTX_free(ctx);
    return ok ? L3_SECURITY_ENGINE_OK : L3_SECURITY_ENGINE_ERR_CRYPTO;
}


static void xor_block16(uint8_t dst[16], const uint8_t src[16]) {
    for (size_t i = 0; i < 16u; ++i) dst[i] ^= src[i];
}

static int ccm_encode_len(uint8_t *out, size_t L, size_t value) {
    if (!out || L < 2u || L > 8u) return 0;
    for (size_t i = 0; i < L; ++i) {
        out[L - 1u - i] = (uint8_t)(value >> (8u * i));
    }
    if (L < sizeof(size_t)) {
        size_t max = ((size_t)1u << (8u * L)) - 1u;
        if (value > max) return 0;
    }
    return 1;
}

typedef int (*ccm_block16_fn)(void *user, const uint8_t in[16], uint8_t out[16]);


typedef struct aes128_block_user {
    uint8_t round_key[176];
} aes128_block_user;

static const uint8_t k_aes_sbox[256] = {
    0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
    0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
    0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
    0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
    0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
    0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
    0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
    0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
    0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
    0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
    0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
    0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
    0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
    0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
    0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
    0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
};

static uint8_t aes_xtime(uint8_t x) {
    return (uint8_t)((x << 1u) ^ ((x & 0x80u) ? 0x1bu : 0u));
}

static void aes_key_expand_128(const uint8_t key[16], uint8_t round_key[176]) {
    static const uint8_t rcon[10] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x1b,0x36};
    memcpy(round_key, key, 16u);
    uint8_t temp[4];
    unsigned bytes = 16u;
    unsigned r = 0u;
    while (bytes < 176u) {
        temp[0] = round_key[bytes - 4u];
        temp[1] = round_key[bytes - 3u];
        temp[2] = round_key[bytes - 2u];
        temp[3] = round_key[bytes - 1u];
        if ((bytes & 15u) == 0u) {
            const uint8_t t = temp[0];
            temp[0] = (uint8_t)(k_aes_sbox[temp[1]] ^ rcon[r++]);
            temp[1] = k_aes_sbox[temp[2]];
            temp[2] = k_aes_sbox[temp[3]];
            temp[3] = k_aes_sbox[t];
        }
        for (unsigned i = 0; i < 4u; ++i) {
            round_key[bytes] = (uint8_t)(round_key[bytes - 16u] ^ temp[i]);
            ++bytes;
        }
    }
}

static void aes_add_round_key(uint8_t state[16], const uint8_t *rk) {
    for (unsigned i = 0; i < 16u; ++i) state[i] ^= rk[i];
}

static void aes_sub_bytes(uint8_t state[16]) {
    for (unsigned i = 0; i < 16u; ++i) state[i] = k_aes_sbox[state[i]];
}

static void aes_shift_rows(uint8_t s[16]) {
    uint8_t t;
    t=s[1]; s[1]=s[5]; s[5]=s[9]; s[9]=s[13]; s[13]=t;
    t=s[2]; s[2]=s[10]; s[10]=t; t=s[6]; s[6]=s[14]; s[14]=t;
    t=s[15]; s[15]=s[11]; s[11]=s[7]; s[7]=s[3]; s[3]=t;
}

static void aes_mix_columns(uint8_t s[16]) {
    for (unsigned c = 0; c < 4u; ++c) {
        uint8_t *a = s + 4u * c;
        const uint8_t a0 = a[0], a1 = a[1], a2 = a[2], a3 = a[3];
        const uint8_t t = (uint8_t)(a0 ^ a1 ^ a2 ^ a3);
        const uint8_t u = a0;
        a[0] ^= (uint8_t)(t ^ aes_xtime((uint8_t)(a0 ^ a1)));
        a[1] ^= (uint8_t)(t ^ aes_xtime((uint8_t)(a1 ^ a2)));
        a[2] ^= (uint8_t)(t ^ aes_xtime((uint8_t)(a2 ^ a3)));
        a[3] ^= (uint8_t)(t ^ aes_xtime((uint8_t)(a3 ^ u)));
    }
}

static int aes128_set_encrypt_key_checked(const uint8_t key[16], aes128_block_user *ctx) {
    if (!key || !ctx) return 0;
    aes_key_expand_128(key, ctx->round_key);
    return 1;
}

static int aes128_encrypt_block_builtin(const aes128_block_user *ctx, const uint8_t in[16], uint8_t out[16]) {
    if (!ctx || !in || !out) return 0;
    uint8_t state[16];
    memcpy(state, in, 16u);
    aes_add_round_key(state, ctx->round_key);
    for (unsigned round = 1u; round < 10u; ++round) {
        aes_sub_bytes(state);
        aes_shift_rows(state);
        aes_mix_columns(state);
        aes_add_round_key(state, ctx->round_key + 16u * round);
    }
    aes_sub_bytes(state);
    aes_shift_rows(state);
    aes_add_round_key(state, ctx->round_key + 160u);
    memcpy(out, state, 16u);
    return 1;
}

static int aes128_block_cb(void *user, const uint8_t in[16], uint8_t out[16]) {
    aes128_block_user *ctx = (aes128_block_user *)user;
    if (!ctx) return L3_SECURITY_ENGINE_ERR_ARGUMENT;
    return aes128_encrypt_block_builtin(ctx, in, out) ? L3_SECURITY_ENGINE_OK : L3_SECURITY_ENGINE_ERR_CRYPTO;
}



static int ccm_encrypt_block16(ccm_block16_fn block, void *block_user,
                               const uint8_t *nonce, size_t nonce_len,
                               const uint8_t *plain, size_t plain_len, uint8_t *out) {
    const size_t tag_len = 4u;
    if (!block || !nonce || !plain || !out || nonce_len < 7u || nonce_len > 13u) {
        return L3_SECURITY_ENGINE_ERR_ARGUMENT;
    }
    const size_t L = 15u - nonce_len;
    if (L < 2u || L > 8u) return L3_SECURITY_ENGINE_ERR_ARGUMENT;
    if (L < sizeof(size_t)) {
        size_t max = ((size_t)1u << (8u * L)) - 1u;
        if (plain_len > max) return L3_SECURITY_ENGINE_ERR_ARGUMENT;
    }

    uint8_t b[16], x[16], y[16], ctr[16], sblock[16];
    memset(x, 0, sizeof(x));

    /* B_0: flags | nonce | encoded message length.  Adata=0, M=4. */
    memset(b, 0, sizeof(b));
    b[0] = (uint8_t)((((tag_len - 2u) / 2u) & 0x7u) << 3) | (uint8_t)(L - 1u);
    memcpy(b + 1, nonce, nonce_len);
    if (!ccm_encode_len(b + 1u + nonce_len, L, plain_len)) return L3_SECURITY_ENGINE_ERR_ARGUMENT;
    xor_block16(x, b);
    if (block(block_user, x, x) != L3_SECURITY_ENGINE_OK) return L3_SECURITY_ENGINE_ERR_CRYPTO;

    /* CBC-MAC over padded plaintext blocks; no AAD for the native BAR7 object. */
    for (size_t off = 0; off < plain_len; off += 16u) {
        memset(b, 0, sizeof(b));
        size_t n = plain_len - off;
        if (n > 16u) n = 16u;
        memcpy(b, plain + off, n);
        xor_block16(x, b);
        if (block(block_user, x, x) != L3_SECURITY_ENGINE_OK) return L3_SECURITY_ENGINE_ERR_CRYPTO;
    }

    memset(ctr, 0, sizeof(ctr));
    ctr[0] = (uint8_t)(L - 1u);
    memcpy(ctr + 1, nonce, nonce_len);

    for (size_t off = 0, counter = 1u; off < plain_len; off += 16u, ++counter) {
        if (!ccm_encode_len(ctr + 1u + nonce_len, L, counter)) return L3_SECURITY_ENGINE_ERR_ARGUMENT;
        if (block(block_user, ctr, sblock) != L3_SECURITY_ENGINE_OK) return L3_SECURITY_ENGINE_ERR_CRYPTO;
        size_t n = plain_len - off;
        if (n > 16u) n = 16u;
        for (size_t i = 0; i < n; ++i) out[off + i] = (uint8_t)(plain[off + i] ^ sblock[i]);
    }

    if (!ccm_encode_len(ctr + 1u + nonce_len, L, 0u)) return L3_SECURITY_ENGINE_ERR_ARGUMENT;
    if (block(block_user, ctr, y) != L3_SECURITY_ENGINE_OK) return L3_SECURITY_ENGINE_ERR_CRYPTO;
    for (size_t i = 0; i < tag_len; ++i) out[plain_len + i] = (uint8_t)(x[i] ^ y[i]);
    return L3_SECURITY_ENGINE_OK;
}

static int ccm_decrypt_block16(ccm_block16_fn block, void *block_user,
                               const uint8_t *nonce, size_t nonce_len,
                               const uint8_t *cipher_tag, size_t cipher_tag_len, uint8_t *out,
                               size_t *plain_len_out) {
    const size_t tag_len = 4u;
    if (!block || !nonce || !cipher_tag || !out || !plain_len_out || cipher_tag_len < tag_len ||
        nonce_len < 7u || nonce_len > 13u) {
        return L3_SECURITY_ENGINE_ERR_ARGUMENT;
    }
    size_t plain_len = cipher_tag_len - tag_len;
    const size_t L = 15u - nonce_len;
    if (L < 2u || L > 8u) return L3_SECURITY_ENGINE_ERR_ARGUMENT;
    if (L < sizeof(size_t)) {
        size_t max = ((size_t)1u << (8u * L)) - 1u;
        if (plain_len > max) return L3_SECURITY_ENGINE_ERR_ARGUMENT;
    }

    uint8_t ctr[16], sblock[16], calc[4];
    memset(ctr, 0, sizeof(ctr));
    ctr[0] = (uint8_t)(L - 1u);
    memcpy(ctr + 1, nonce, nonce_len);

    for (size_t off = 0, counter = 1u; off < plain_len; off += 16u, ++counter) {
        if (!ccm_encode_len(ctr + 1u + nonce_len, L, counter)) return L3_SECURITY_ENGINE_ERR_ARGUMENT;
        if (block(block_user, ctr, sblock) != L3_SECURITY_ENGINE_OK) return L3_SECURITY_ENGINE_ERR_CRYPTO;
        size_t n = plain_len - off;
        if (n > 16u) n = 16u;
        for (size_t i = 0; i < n; ++i) out[off + i] = (uint8_t)(cipher_tag[off + i] ^ sblock[i]);
    }

    /* This backend is reached only through the fixed 60-byte Libre 3 response
     * API (56 plaintext bytes plus the four-byte tag).  Do not reserve the
     * reconstruction-era 1 KiB generic scratch buffer on a watch stack. */
    uint8_t tmp[L3_LEN_CHALLENGE_RESPONSE_CRYPT];
    if (plain_len > sizeof(tmp) - tag_len) {
        return L3_SECURITY_ENGINE_ERR_ARGUMENT;
    }
    if (ccm_encrypt_block16(block, block_user, nonce, nonce_len, out, plain_len, tmp) != L3_SECURITY_ENGINE_OK) {
        return L3_SECURITY_ENGINE_ERR_CRYPTO;
    }
    memcpy(calc, tmp + plain_len, tag_len);
    uint8_t diff = 0;
    for (size_t i = 0; i < tag_len; ++i) diff |= (uint8_t)(calc[i] ^ cipher_tag[plain_len + i]);
    if (diff) return L3_SECURITY_ENGINE_ERR_CRYPTO;
    *plain_len_out = plain_len;
    return L3_SECURITY_ENGINE_OK;
}



static int process_challenge_cipher_to_buffer(l3_security_engine *p, uint32_t mode_selector,
                      const uint8_t *arg0, size_t arg0_len,
                      const uint8_t *arg1, size_t arg1_len,
                      const l3_security_material *root,
                      uint8_t *out, size_t expected_out_len) {
    if (!p || !arg0 || !arg1 || !root || !out) return L3_SECURITY_ERR_ARGUMENT;
    if (root->kind != MATERIAL_KIND_AUTHORIZATION_ROOT) return L3_SECURITY_ERR_STATE;
    if (root->len < 16u) return L3_SECURITY_ERR_STATE;
    const int root_has_constructor_frame =
        (root->len == (16u + L3_SECURITY_ENGINE_CHALLENGE_CONSTRUCTOR_FRAME_LEN));

    if (arg0_len >= 7u && arg0_len <= 13u &&
        (root_has_constructor_frame || root->len >= 16u)) {
        ccm_block16_fn block = NULL;
        void *block_user = NULL;
        aes128_block_user aes_ctx;
        memset(&aes_ctx, 0, sizeof(aes_ctx));
        uint8_t challenge_key[16];
        memset(challenge_key, 0, sizeof(challenge_key));

        l3_challenge_block_context f407_ctx;

        if (root_has_constructor_frame) {
            /* Fresh authorization roots and restored saved-KAuth roots both carry
             * the native 66-byte f407 constructor frame after the visible 16-byte
             * prefix.  The compact AES-key reduction works for some restored
             * saved records, but the 2026-08-14 fresh-pairing log shows status 19
             * immediately after BAR7 when that reduction is used live.  Use the
             * recovered f407 block context as the authoritative BAR7/BAR8 block
             * transform again, while keeping the memory-safe fixed-output/JNI path. */
            if (l3_challenge_block_context_init(
                    &g_f407_tables, root->bytes + 16u, &f407_ctx) != 0) {
                return L3_SECURITY_ERR_ENGINE;
            }
            block = challenge_context_encrypt_block;
            block_user = &f407_ctx;
        } else {
            if (!aes128_set_encrypt_key_checked(root->bytes, &aes_ctx)) {
                return L3_SECURITY_ERR_ENGINE;
            }
            memcpy(challenge_key, root->bytes, sizeof(challenge_key));
            block = aes128_block_cb;
            block_user = &aes_ctx;
        }

        if (mode_selector == CHALLENGE_OPERATION_ENCRYPT) {
            if (expected_out_len != arg1_len + 4u) return L3_SECURITY_ERR_ARGUMENT;
            int rc = ccm_encrypt_block16(block, block_user, arg0, arg0_len, arg1, arg1_len, out);
            return rc ? L3_SECURITY_ERR_ENGINE : L3_SECURITY_OK;
        }

        if (mode_selector == CHALLENGE_OPERATION_DECRYPT) {
            if (arg1_len < 4u) return L3_SECURITY_ERR_ARGUMENT;
            size_t clen = arg1_len - 4u;
            if (expected_out_len != clen) return L3_SECURITY_ERR_ARGUMENT;
            size_t plain_len = 0;
            int rc = ccm_decrypt_block16(block, block_user, arg0, arg0_len, arg1, arg1_len,
                                         out, &plain_len);
            if (rc || plain_len != expected_out_len) return L3_SECURITY_ERR_ENGINE;
            return L3_SECURITY_OK;
        }
    }

    uint8_t kbuf[32];
    uint8_t ivfull[32];
    int rc = derive32(p, "bar.key", root->bytes, root->len, arg0, arg0_len, MATERIAL_TAG_CHALLENGE_CIPHER, 0, kbuf);
    if (rc) return L3_SECURITY_ERR_ENGINE;
    rc = derive32(p, "bar.iv", root->bytes, root->len, arg0, arg0_len, MATERIAL_TAG_CHALLENGE_CIPHER, 1, ivfull);
    if (rc) return L3_SECURITY_ERR_ENGINE;

    if (mode_selector == CHALLENGE_OPERATION_ENCRYPT) {
        if (expected_out_len != arg1_len + 4u) return L3_SECURITY_ERR_ARGUMENT;
        rc = aes_ctr_xor(kbuf, ivfull, arg1, arg1_len, out);
        if (rc) return L3_SECURITY_ERR_ENGINE;
        uint8_t tagdata[32];
        rc = derive32(p, "bar.tag", kbuf, sizeof(kbuf), out, arg1_len, (uint32_t)arg0_len, (uint32_t)arg1_len, tagdata);
        if (rc) return L3_SECURITY_ERR_ENGINE;
        memcpy(out + arg1_len, tagdata, 4);
        return L3_SECURITY_OK;
    }

    if (mode_selector == CHALLENGE_OPERATION_DECRYPT) {
        if (arg1_len < 4u) return L3_SECURITY_ERR_ARGUMENT;
        size_t clen = arg1_len - 4u;
        if (expected_out_len != clen) return L3_SECURITY_ERR_ARGUMENT;
        uint8_t tagdata[32];
        rc = derive32(p, "bar.tag", kbuf, sizeof(kbuf), arg1, clen, (uint32_t)arg0_len, (uint32_t)clen, tagdata);
        if (rc) return L3_SECURITY_ERR_ENGINE;
        if (memcmp(tagdata, arg1 + clen, 4u) != 0) return L3_SECURITY_ERR_ENGINE;
        rc = aes_ctr_xor(kbuf, ivfull, arg1, clen, out);
        return rc ? L3_SECURITY_ERR_ENGINE : L3_SECURITY_OK;
    }

    return L3_SECURITY_ERR_UNSUPPORTED;
}

static int export_legacy_authorization_record(l3_security_engine *p, const l3_security_material *object,
                                  uint32_t selector, uint8_t **out, size_t *out_len) {
    const uint8_t magic[8] = {'L','3','S','O','F','T','1',0};
    size_t n = 8u + 4u * 5u + object->len + 32u;
    uint8_t *buf = (uint8_t *)calloc(1, n);
    if (!buf) return L3_SECURITY_ERR_NOMEM;
    memcpy(buf, magic, sizeof(magic));
    le32(buf + 8, selector);
    le32(buf + 12, object->kind);
    le32(buf + 16, object->origin_tag);
    le32(buf + 20, object->metadata0);
    le32(buf + 24, (uint32_t)object->len);
    memcpy(buf + 28, object->bytes, object->len);
    uint8_t mac[32];
    int rc = hmac_sha256(p->master, sizeof(p->master), buf, 28u + object->len, mac);
    if (rc) { free(buf); return L3_SECURITY_ERR_ENGINE; }
    memcpy(buf + 28u + object->len, mac, sizeof(mac));
    *out = buf;
    *out_len = n;
    return L3_SECURITY_OK;
}

static int export_native_saved_authorization_record(l3_security_engine *p, const l3_security_material *object,
                                 uint8_t **out, size_t *out_len) {
    if (object->len < 16u) return L3_SECURITY_ERR_STATE;

    uint8_t tmp[L3_SAVED_AUTHORIZATION_RECORD_SIZE];
    int rc = L3_SAVED_AUTHORIZATION_ERR_ARGUMENT;

    if (object->len >= 16u + L3_SECURITY_ENGINE_CHALLENGE_CONSTRUCTOR_FRAME_LEN) {
        const uint8_t *frame = object->bytes + 16u;
        uint8_t trailer20[20];
        memset(trailer20, 0, sizeof(trailer20));
        memcpy(trailer20, frame + 52u, 14u);
        uint16_t chk = authorization_export_frame_checksum16(frame);
        trailer20[14] = (uint8_t)chk;
        trailer20[15] = (uint8_t)(chk >> 8);
        /*
         * Standalone BAR9 export format: keep the native 149-byte envelope and
         * valid header MAC, but store the complete generated root constructor
         * frame in descriptor fields so the recovered backend can re-import it on a
         * later processint(2/3) call.  This removes the old V397/V398 captured
         * output replay guard and makes the export session-dependent.
         */
        rc = l3_saved_authorization_record_build_with_options(k_authorization_export_magic4,
                                                frame + 48u,
                                                frame + 0u,
                                                frame + 16u,
                                                frame + 32u,
                                                frame + 0u,
                                                trailer20,
                                                tmp);
    }

    if (rc != L3_SAVED_AUTHORIZATION_OK) {
        uint8_t seed[32];
        rc = object_digest(p, object, seed);
        if (rc) return L3_SECURITY_ERR_ENGINE;

        uint8_t magic[32], nonce[32], tag[32];
        rc = derive32(p, "export.magic", object->bytes, object->len, seed, sizeof(seed), 0, 0, magic);
        if (rc) return L3_SECURITY_ERR_ENGINE;
        rc = derive32(p, "export.nonce", object->bytes, object->len, seed, sizeof(seed), 0, 1, nonce);
        if (rc) return L3_SECURITY_ERR_ENGINE;

        memset(tmp, 0, sizeof(tmp));
        memset(tag, 0, sizeof(tag));
        rc = l3_saved_authorization_record_build(magic, NULL, NULL, nonce, object->bytes, tag, tmp);
        if (rc) return L3_SECURITY_ERR_ENGINE;
        rc = hmac_sha256(p->master, sizeof(p->master), tmp, L3_SAVED_AUTHORIZATION_TRAILER_OFFSET, tag);
        if (rc) return L3_SECURITY_ERR_ENGINE;
        rc = l3_saved_authorization_record_build(magic, NULL, NULL, nonce, object->bytes, tag, tmp);
        if (rc) return L3_SECURITY_ERR_ENGINE;
    }

    uint8_t *buf = (uint8_t *)malloc(sizeof(tmp));
    if (!buf) return L3_SECURITY_ERR_NOMEM;
    memcpy(buf, tmp, sizeof(tmp));
    *out = buf;
    *out_len = sizeof(tmp);
    return L3_SECURITY_OK;
}

static int export_authorization_record_material(void *user, const l3_security_material *object, uint32_t selector,
                       uint8_t **out, size_t *out_len) {
    l3_security_engine *p = (l3_security_engine *)user;
    if (!p || !object || !out || !out_len) return L3_SECURITY_ERR_ARGUMENT;
    if (selector != MATERIAL_TAG_EXPORT_AUTHORIZATION || object->kind != MATERIAL_KIND_AUTHORIZATION_ROOT) return L3_SECURITY_ERR_UNSUPPORTED;
    *out = NULL;
    *out_len = 0;

    if (p->export_format == L3_SECURITY_ENGINE_EXPORT_NATIVE149) {
        return export_native_saved_authorization_record(p, object, out, out_len);
    }
    return export_legacy_authorization_record(p, object, selector, out, out_len);
}

static void release_engine_material(void *user, l3_security_material *object) {
    (void)user;
    l3_security_material_free(object);
}


/* Semantic backend entry points used by the handshake layer.  The recovered
 * operation tags stay below this boundary as implementation metadata only. */
static int engine_select_app_private_key(void *user,
                                         unsigned security_version,
                                         l3_security_material **out) {
    l3_security_engine *p = (l3_security_engine *)user;
    if (!p || !out) return L3_SECURITY_ERR_ARGUMENT;
    uint8_t native_scalar35[L3_APP_PRIVATE_NATIVE_SCALAR_LEN];
    if (l3_fixed_app_private_native_scalar35(security_version, native_scalar35) != 0) {
        return L3_SECURITY_ERR_ARGUMENT;
    }
    return new_obj(MATERIAL_KIND_IMPORTED_PRIVATE_KEY, 0, security_version, 0x100u,
                   native_scalar35, sizeof(native_scalar35), out)
               ? L3_SECURITY_ERR_ENGINE : L3_SECURITY_OK;
}

static int engine_restore_authorization_record(void *user,
                                                 const uint8_t *record,
                                                 size_t record_len,
                                                 l3_security_material **out) {
    return restore_saved_authorization_material(user, record, record_len, out);
}

static int engine_prepare_app_private_key(void *user,
                                           const l3_security_material *decoded_private_key,
                                           l3_security_material **out) {
    const key_material_request request = {
        MATERIAL_TAG_PREPARED_PRIVATE_KEY, decoded_private_key, 0u
    };
    return create_private_or_ephemeral_key_material(user, &request, out);
}

static int engine_generate_ephemeral_keypair(void *user, l3_security_material **out) {
    const key_material_request request = {
        MATERIAL_TAG_EPHEMERAL_KEYPAIR, NULL, L3_LEN_EPHEMERAL_PUBLIC_KEY
    };
    return create_private_or_ephemeral_key_material(user, &request, out);
}

static int engine_derive_shared_point(void *user,
                                       const l3_security_material *private_key,
                                       const uint8_t peer_public_xy64[64],
                                       l3_security_material **out) {
    return derive_shared_point_material(user, private_key, peer_public_xy64,
                                   L3_LEN_EPHEMERAL_PUBLIC_KEY, 0x20u, out);
}

static int engine_concatenate_shared_points(void *user,
                                             const l3_security_material *first,
                                             const l3_security_material *second,
                                             l3_security_material **out) {
    const authorization_material_request request = {
        MATERIAL_TAG_JOINED_SHARED_POINTS, second, 0u, 0u, 0u
    };
    return transform_authorization_material(user, first, &request, out);
}

static int engine_digest_shared_points(void *user,
                                        const l3_security_material *joined,
                                        l3_security_material **out) {
    const authorization_material_request request = {
        MATERIAL_TAG_DIGESTED_SHARED_POINTS, NULL, 0x01000000u, 0u, 0u
    };

    return transform_authorization_material(user, joined, &request, out);
}

static int engine_extract_authorization_root(void *user,
                                              const l3_security_material *digest_material,
                                              l3_security_material **out) {
    const authorization_material_request request = {
        MATERIAL_TAG_AUTHORIZATION_ROOT, NULL, 0u, 0u, 0x10u
    };
    return transform_authorization_material(user, digest_material, &request, out);
}

static int engine_copy_ephemeral_public_key(void *user,
                                             const l3_security_material *keypair,
                                             uint8_t out_public_xy64[L3_LEN_EPHEMERAL_PUBLIC_KEY]) {
    return copy_ephemeral_public_key_material(user, keypair, out_public_xy64,
                                     L3_LEN_EPHEMERAL_PUBLIC_KEY);
}

static int engine_encrypt_challenge_reply_into(void *user,
                                           const uint8_t nonce7[L3_LEN_CHALLENGE_NONCE],
                                           const uint8_t plain36[L3_LEN_CHALLENGE_REPLY_PLAIN],
                                           const l3_security_material *root,
                                           uint8_t out40[L3_LEN_CHALLENGE_REPLY_CRYPT]) {
    return process_challenge_cipher_to_buffer((l3_security_engine *)user,
                                  CHALLENGE_OPERATION_ENCRYPT,
                                  nonce7, L3_LEN_CHALLENGE_NONCE,
                                  plain36, L3_LEN_CHALLENGE_REPLY_PLAIN,
                                  root, out40, L3_LEN_CHALLENGE_REPLY_CRYPT);
}

static int engine_decrypt_challenge_response_into(void *user,
                                              const uint8_t nonce7[L3_LEN_CHALLENGE_NONCE],
                                              const uint8_t cipher60[L3_LEN_CHALLENGE_RESPONSE_CRYPT],
                                              const l3_security_material *root,
                                              uint8_t out56[L3_LEN_CHALLENGE_RESPONSE_PLAIN]) {
    return process_challenge_cipher_to_buffer((l3_security_engine *)user,
                                  CHALLENGE_OPERATION_DECRYPT,
                                  nonce7, L3_LEN_CHALLENGE_NONCE,
                                  cipher60, L3_LEN_CHALLENGE_RESPONSE_CRYPT,
                                  root, out56, L3_LEN_CHALLENGE_RESPONSE_PLAIN);
}

static int engine_export_authorization_record_into(void *user,
                                               const l3_security_material *root,
                                               uint8_t out149[L3_LEN_SAVED_AUTHORIZATION]) {
    uint8_t *tmp = NULL;
    size_t tmp_len = 0;
    int rc = export_authorization_record_material(user, root, MATERIAL_TAG_EXPORT_AUTHORIZATION,
                                                  &tmp, &tmp_len);
    if (rc != L3_SECURITY_OK || !tmp || tmp_len != L3_LEN_SAVED_AUTHORIZATION) {
        free(tmp);
        return rc == L3_SECURITY_OK ? L3_SECURITY_ERR_ENGINE : rc;
    }
    memcpy(out149, tmp, L3_LEN_SAVED_AUTHORIZATION);
    free(tmp);
    return L3_SECURITY_OK;
}

size_t l3_security_engine_authorization_scratch_size(void) {
    return l3_authorization_shared_point_workspace_size();
}

size_t l3_security_engine_authorization_scratch_alignment(void) {
    return l3_authorization_shared_point_workspace_alignment();
}

int l3_security_engine_set_authorization_scratch(
    l3_security_engine *provider,
    void *workspace,
    size_t workspace_size) {
    if (!provider) return L3_SECURITY_ENGINE_ERR_ARGUMENT;
    if (!workspace && workspace_size == 0u) {
        provider->authorization_scratch = NULL;
        provider->authorization_scratch_size = 0u;
        return L3_SECURITY_ENGINE_OK;
    }
    const size_t required_size =
        l3_security_engine_authorization_scratch_size();
    const size_t required_alignment =
        l3_security_engine_authorization_scratch_alignment();
    if (!workspace || workspace_size < required_size ||
        (uintptr_t)workspace % required_alignment != 0u) {
        return L3_SECURITY_ENGINE_ERR_ARGUMENT;
    }
    provider->authorization_scratch = workspace;
    provider->authorization_scratch_size = workspace_size;
    return L3_SECURITY_ENGINE_OK;
}

int l3_security_engine_init(l3_security_engine *provider,
                          const l3_security_engine_config *config) {
    if (!provider) return L3_SECURITY_ENGINE_ERR_ARGUMENT;
    memset(provider, 0, sizeof(*provider));

    static const uint8_t default_secret[] = "libre3-soft-provider-v82";
    const uint8_t *secret = default_secret;
    size_t secret_len = sizeof(default_secret) - 1u;
    if (config && config->master_secret && config->master_secret_len) {
        secret = config->master_secret;
        secret_len = config->master_secret_len;
    }
    uint8_t salt[32];
    memset(salt, 0x82, sizeof(salt));
    int rc = hmac_sha256(salt, sizeof(salt), secret, secret_len, provider->master);
    if (rc) return rc;

    if (config) {
        provider->entropy = config->entropy;
        provider->entropy_user = config->entropy_user;
        provider->export_format = config->export_format;
        provider->challenge_key_mode = config->challenge_key_mode;
    }

    if (provider->challenge_key_mode == L3_SECURITY_ENGINE_CHALLENGE_KEY_DEFAULT) {
        provider->challenge_key_mode = (uint32_t)L3_SECURITY_ENGINE_DEFAULT_CHALLENGE_KEY_MODE;
    }

    provider->operations.user = provider;
    provider->operations.begin_session = reset_engine_session_state;
    provider->operations.select_app_private_key = engine_select_app_private_key;
    provider->operations.restore_authorization_record = engine_restore_authorization_record;
    provider->operations.prepare_app_private_key = engine_prepare_app_private_key;
    provider->operations.generate_ephemeral_keypair = engine_generate_ephemeral_keypair;
    provider->operations.derive_shared_point = engine_derive_shared_point;
    provider->operations.concatenate_shared_points = engine_concatenate_shared_points;
    provider->operations.digest_shared_points = engine_digest_shared_points;
    provider->operations.extract_authorization_root = engine_extract_authorization_root;
    provider->operations.derive_authorization_root_direct = engine_derive_authorization_root_direct;
    provider->operations.copy_ephemeral_public_key = engine_copy_ephemeral_public_key;
    provider->operations.encrypt_challenge_reply_into = engine_encrypt_challenge_reply_into;
    provider->operations.decrypt_challenge_response_into = engine_decrypt_challenge_response_into;
    provider->operations.export_authorization_record_into = engine_export_authorization_record_into;
    provider->operations.release_material = release_engine_material;
    return L3_SECURITY_ENGINE_OK;
}

const l3_handshake_operations *l3_security_engine_handshake_operations(l3_security_engine *provider) {
    return provider ? &provider->operations : NULL;
}
