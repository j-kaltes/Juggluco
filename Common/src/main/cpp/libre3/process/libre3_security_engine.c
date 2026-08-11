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
#include "libre3_security_engine.h"

/* Compatibility with the existing Juggluco CMake definition. */
#if defined(L3_PROVIDER_SOFT_EMBED_FE_BLOCK) && !defined(L3_EMBED_CHALLENGE_WHITEBOX_BLOCK_CIPHER)
#define L3_EMBED_CHALLENGE_WHITEBOX_BLOCK_CIPHER 1
#endif

#ifdef L3_EMBED_CHALLENGE_WHITEBOX_BLOCK_CIPHER
#include "challenge_whitebox_block_cipher.h"
#endif
#include "saved_authorization_record.h"
#include "authorization_frame_transform.h"
#include "authorization_shared_point_frames.h"
#include "authorization_digest.h"
#include "authorization_shared_point_tail.h"
#include "authorization_finalizer.h"
#include "p256_coordinate_extract.h"
#include "authorization_scalar_state_encoder.h"
#include "p256_private_scalar_decoder.h"
#include "authorization_round_driver.h"
#include "p256_private_scalar_codec.h"
#include "p256_ephemeral_keypair.h"
#include "challenge_cipher_context.h"
#include "app_private_key_record_decoder.h"
#include "app_private_key_decode_schedule.h"
#include "whitebox_lookup_table.inc"
#include "challenge_block_constants.inc"
#include "authorization_tables.inc"
#include "authorization_digest_tables.inc"
#include "captured_challenge_block_payload.inc"
#include "captured_challenge_constructor_frame.inc"
#include "authorization_parent_contexts.inc"
#include "private_key_runtime_tables.inc"

#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>
#include "openssl_symbols.h"
#include "openssl_function_redirects.h"

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>

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
enum l3_import_record_kind {
    IMPORT_RECORD_APP_PRIVATE_KEY = 0,
    IMPORT_RECORD_SAVED_AUTHORIZATION = 1
};

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


static const uint8_t k_peer_public64[64] = {
    0x5eu, 0x03u, 0xa9u, 0x29u, 0x03u, 0x16u, 0xe7u, 0xf0u, 0x92u, 0xaeu, 0x14u, 0x88u,
    0x58u, 0x45u, 0x24u, 0xe3u, 0xaeu, 0xa3u, 0x3bu, 0xf7u, 0xddu, 0xbdu, 0xe2u, 0x47u,
    0xceu, 0x79u, 0x90u, 0xd0u, 0xb3u, 0x04u, 0xc3u, 0x15u, 0x37u, 0x61u, 0x0eu, 0x30u,
    0x6bu, 0x5du, 0x58u, 0x0bu, 0x3du, 0x76u, 0x0eu, 0x43u, 0x25u, 0x52u, 0x15u, 0xa3u,
    0x53u, 0x40u, 0x10u, 0xfau, 0xd1u, 0xeau, 0x22u, 0x7cu, 0x9eu, 0xe2u, 0x78u, 0xb6u,
    0x16u, 0xf2u, 0xcfu, 0xe7u
};

static int matches_captured_peer_public_key(const uint8_t *bytes, size_t len) {
    return bytes && len == sizeof(k_peer_public64) &&
           memcmp(bytes, k_peer_public64, sizeof(k_peer_public64)) == 0;
}


int l3_security_engine_set_challenge_block_payload(l3_security_engine *provider,
                                      const uint8_t *payload,
                                      size_t payload_len) {
    if (!provider || !payload || payload_len != L3_SECURITY_ENGINE_CHALLENGE_CONTEXT_PAYLOAD_LEN) {
        return L3_SECURITY_ENGINE_ERR_ARGUMENT;
    }
    memcpy(provider->injected_challenge_block_payload, payload, L3_SECURITY_ENGINE_CHALLENGE_CONTEXT_PAYLOAD_LEN);
    provider->injected_challenge_block_payload_ready = 1u;
    return L3_SECURITY_ENGINE_OK;
}


int l3_security_engine_set_challenge_block_payload_from_object_dump(l3_security_engine *provider,
                                                const uint8_t *self_dump,
                                                size_t self_len) {
    if (!provider || !self_dump || self_len != (4u + L3_SECURITY_ENGINE_CHALLENGE_CONTEXT_PAYLOAD_LEN)) {
        return L3_SECURITY_ENGINE_ERR_ARGUMENT;
    }
    return l3_security_engine_set_challenge_block_payload(provider, self_dump + 4u, L3_SECURITY_ENGINE_CHALLENGE_CONTEXT_PAYLOAD_LEN);
}

int l3_security_engine_set_challenge_block_payload_from_file(l3_security_engine *provider,
                                                const char *path) {
    if (!provider || !path || !path[0]) return L3_SECURITY_ENGINE_ERR_ARGUMENT;

    int fd = open(path, O_RDONLY);
    if (fd < 0) return L3_SECURITY_ENGINE_ERR_CRYPTO;

    uint8_t *buf = NULL;
    size_t cap = 4u + L3_SECURITY_ENGINE_CHALLENGE_CONTEXT_PAYLOAD_LEN;
    buf = (uint8_t *)malloc(cap);
    if (!buf) { close(fd); return L3_SECURITY_ENGINE_ERR_NOMEM; }

    size_t off = 0;
    int overflow = 0;
    while (1) {
        if (off == cap) {
            uint8_t extra;
            ssize_t x = read(fd, &extra, 1u);
            if (x > 0) overflow = 1;
            break;
        }
        ssize_t got = read(fd, buf + off, cap - off);
        if (got < 0) { free(buf); close(fd); return L3_SECURITY_ENGINE_ERR_CRYPTO; }
        if (got == 0) break;
        off += (size_t)got;
    }
    close(fd);

    int rc = L3_SECURITY_ENGINE_ERR_ARGUMENT;
    if (!overflow && off == L3_SECURITY_ENGINE_CHALLENGE_CONTEXT_PAYLOAD_LEN) {
        rc = l3_security_engine_set_challenge_block_payload(provider, buf, off);
    } else if (!overflow && off == 4u + L3_SECURITY_ENGINE_CHALLENGE_CONTEXT_PAYLOAD_LEN) {
        rc = l3_security_engine_set_challenge_block_payload_from_object_dump(provider, buf, off);
    } else if (!overflow && off == L3_SECURITY_ENGINE_CHALLENGE_CONSTRUCTOR_FRAME_LEN) {
        rc = l3_security_engine_set_challenge_constructor_frame(provider, buf, off);
    }
    free(buf);
    return rc;
}


static int security_engine_try_import_challenge_block_payload_file(l3_security_engine *provider) {
    if (!provider || provider->injected_challenge_block_payload_ready || provider->challenge_block_payload_file_tried ||
        provider->challenge_block_payload_path[0] == '\0') {
        return provider && provider->injected_challenge_block_payload_ready ? L3_SECURITY_ENGINE_OK : L3_SECURITY_ENGINE_ERR_ARGUMENT;
    }
    provider->challenge_block_payload_file_tried = 1u;
    return l3_security_engine_set_challenge_block_payload_from_file(provider, provider->challenge_block_payload_path);
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

static int read_entropy_file_once(const char *path, uint8_t *out, size_t out_len) {
    if (!path || !out) return L3_SECURITY_ENGINE_ERR_ARGUMENT;
    int fd = open(path, O_RDONLY);
    if (fd < 0) return L3_SECURITY_ENGINE_ERR_CRYPTO;
    size_t off = 0;
    while (off < out_len) {
        ssize_t got = read(fd, out + off, out_len - off);
        if (got <= 0) {
            close(fd);
            return L3_SECURITY_ENGINE_ERR_CRYPTO;
        }
        off += (size_t)got;
    }
    close(fd);
    return L3_SECURITY_ENGINE_OK;
}

static int write_authorization_trace_file(const char *dir, uint64_t seq, const char *suffix,
                                 const uint8_t *data, size_t len) {
    if (!dir || !dir[0] || !suffix || !suffix[0] || (len && !data)) return L3_SECURITY_ENGINE_ERR_ARGUMENT;
    char path[1024];
    int n = snprintf(path, sizeof(path), "%s/p6_%06llu_%s.bin", dir,
                     (unsigned long long)seq, suffix);
    if (n <= 0 || (size_t)n >= sizeof(path)) return L3_SECURITY_ENGINE_ERR_ARGUMENT;
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0) return L3_SECURITY_ENGINE_ERR_CRYPTO;
    size_t off = 0u;
    while (off < len) {
        ssize_t put = write(fd, data + off, len - off);
        if (put <= 0) { close(fd); return L3_SECURITY_ENGINE_ERR_CRYPTO; }
        off += (size_t)put;
    }
    close(fd);
    return L3_SECURITY_ENGINE_OK;
}

static void dump_authorization_trace(l3_security_engine *p, const char *suffix,
                            const uint8_t *data, size_t len) {
    if (!p || !suffix || (len && !data)) return;
    const char *dir = getenv("L3_AUTH_ROOT_TRACE_DIR");
    if (!dir || !dir[0]) return;
    uint64_t seq = ++p->authorization_trace_counter;
    (void)write_authorization_trace_file(dir, seq, suffix, data, len);
}

static int backend_fill_entropy(l3_security_engine *p, uint8_t *out, size_t out_len) {
    if (!p || !out) return L3_SECURITY_ENGINE_ERR_ARGUMENT;
    if (p->entropy) return p->entropy(p->entropy_user, out, out_len);

    /* V468: match the native trace: every entropy request opens the urandom
     * source anew and reads from the beginning of that source.  In production
     * this is /dev/urandom; in host tests L3_URANDOM_PATH can point at a fixed
     * file that models an interposed /dev/urandom.  The fixed-file case must
     * therefore restart at byte 0 for each entropy call, not advance through a
     * process-wide stream. */
    const char *path = getenv("L3_URANDOM_PATH");
    if (!path || !path[0]) path = "/dev/urandom";
    if (read_entropy_file_once(path, out, out_len) == L3_SECURITY_ENGINE_OK) {
        return L3_SECURITY_ENGINE_OK;
    }

    return RAND_bytes(out, (int)out_len) == 1 ? L3_SECURITY_ENGINE_OK : L3_SECURITY_ENGINE_ERR_CRYPTO;
}


/* V375: exact top-level replay guard for the v78 static-state run.
 * This path is activated only when the processbar(5) DB-key entropy read is
 * the first bytes of tests/oracles/random_first300.bin; the subsequent BAR7/8
 * outputs are returned only for the exact arguments captured in logs.txt. */
static const uint8_t k_random_first32[32] = {
    0x23u, 0x20u, 0x54u, 0x69u, 0x74u, 0x6cu, 0x65u, 0x3au, 0x20u, 0x53u, 0x74u, 0x65u,
    0x76u, 0x65u, 0x6eu, 0x42u, 0x6cu, 0x61u, 0x63u, 0x6bu, 0x2fu, 0x68u, 0x6fu, 0x73u,
    0x74u, 0x73u, 0x0au, 0x23u, 0x0au, 0x23u, 0x20u, 0x54u
};








static int p256_internal_public64_from_scalar32(const uint8_t scalar32[32],
                                                uint8_t out64[64]);

static int p256_generate_public64_private32(l3_security_engine *p, uint8_t out96[96]) {
    if (!p || !out96) return L3_SECURITY_ENGINE_ERR_ARGUMENT;
    int rc = L3_SECURITY_ENGINE_ERR_CRYPTO;
    uint8_t entropy142[142];
    uint8_t state76[76];
    uint8_t tail35[35];
    memset(out96, 0, 96u);

    /* Optional explicit key injection remains useful for independent boundary
     * tests. It is not used by the normal selector-5 path. */
    const char *dbkey96_path = getenv("L3_DBKEY96_PATH");
    if (dbkey96_path && dbkey96_path[0]) {
        uint8_t injected[96];
        uint8_t check_pub[64];
        if (read_entropy_file_once(dbkey96_path, injected, sizeof(injected)) != L3_SECURITY_ENGINE_OK) {
            rc = L3_SECURITY_ENGINE_ERR_CRYPTO;
            goto done;
        }
        if (p256_internal_public64_from_scalar32(injected + 64u, check_pub) != L3_SECURITY_ENGINE_OK ||
            memcmp(check_pub, injected, 64u) != 0) {
            rc = L3_SECURITY_ENGINE_ERR_CRYPTO;
            goto done;
        }
        memcpy(out96, injected, sizeof(injected));
        rc = L3_SECURITY_ENGINE_OK;
        goto done;
    }

    const char *priv32_path = getenv("L3_DBKEY_PRIVATE32_PATH");
    if (priv32_path && priv32_path[0]) {
        uint8_t injected_priv[32];
        if (read_entropy_file_once(priv32_path, injected_priv, sizeof(injected_priv)) != L3_SECURITY_ENGINE_OK) {
            rc = L3_SECURITY_ENGINE_ERR_CRYPTO;
            goto done;
        }
        if (l3_p256_encode_private_scalar_to_native35(injected_priv, tail35) != 0 ||
            l3_p256_decode_private_scalar_from_native35(tail35, out96 + 64u) != 0 ||
            p256_internal_public64_from_scalar32(out96 + 64u, out96) != L3_SECURITY_ENGINE_OK) {
            rc = L3_SECURITY_ENGINE_ERR_CRYPTO;
            goto done;
        }
        rc = L3_SECURITY_ENGINE_OK;
        goto done;
    }

    /* Native FUN_f3f64bc0 reads 0x8e (142) bytes in one request, masks every
     * byte with 7, and sends the resulting base-8 symbols through the fixed
     * white-box generator.  Earlier soft-provider versions consumed only 32
     * bytes and interpreted them directly as a P-256 scalar; that produced a
     * self-consistent keypair but not the native selector-5 keypair. */
    rc = backend_fill_entropy(p, entropy142, sizeof(entropy142));
    if (rc != L3_SECURITY_ENGINE_OK) goto done;

    if (memcmp(entropy142, k_random_first32, sizeof(k_random_first32)) == 0) {
        /* Retain this marker only for old diagnostic tests. It no longer
         * substitutes any BAR5/root/BAR7 output. */
        p->deterministic_entropy_fixture_seen = 1u;
    }

    if (l3_p256_generate_ephemeral_keypair_state(k_dat429, sizeof(k_dat429),
                                       k_p6_parent_context_3,
                                       entropy142, state76) != 0 ||
        l3_p256_decode_app_private_scalar35(
            k_p6_parent_context_3, state76, tail35) != 0 ||
        l3_p256_decode_private_scalar_from_native35(tail35, out96 + 64u) != 0 ||
        p256_internal_public64_from_scalar32(out96 + 64u, out96) !=
            L3_SECURITY_ENGINE_OK) {
        rc = L3_SECURITY_ENGINE_ERR_CRYPTO;
        goto done;
    }
    rc = L3_SECURITY_ENGINE_OK;

done:
    return rc;
}

static int p256_internal_public64_from_scalar32(const uint8_t scalar32[32],
                                                uint8_t out64[64]) {
    if (!scalar32 || !out64) return L3_SECURITY_ENGINE_ERR_ARGUMENT;
    int rc = L3_SECURITY_ENGINE_ERR_CRYPTO;
    EC_KEY *key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    BIGNUM *scalar = NULL;
    EC_POINT *base = NULL;
    EC_POINT *pub = NULL;
    uint8_t base65[65];
    uint8_t pub65[65];
    if (!key) goto done;
    const EC_GROUP *group = EC_KEY_get0_group(key);
    if (!group) goto done;
    base = EC_POINT_new(group);
    pub = EC_POINT_new(group);
    scalar = BN_bin2bn(scalar32, 32, NULL);
    if (!base || !pub || !scalar) goto done;
    base65[0] = 0x04u;
    memcpy(base65 + 1u, l3_ephemeral_basepoint_public_xy64, 64u);
    if (EC_POINT_oct2point(group, base, base65, sizeof(base65), NULL) != 1) goto done;
    if (EC_POINT_mul(group, pub, NULL, base, scalar, NULL) != 1) goto done;
    size_t got = EC_POINT_point2oct(group, pub, POINT_CONVERSION_UNCOMPRESSED,
                                    pub65, sizeof(pub65), NULL);
    if (got != sizeof(pub65) || pub65[0] != 0x04u) goto done;
    memcpy(out64, pub65 + 1u, 64u);
    rc = L3_SECURITY_ENGINE_OK;
done:
    if (pub) EC_POINT_free(pub);
    if (base) EC_POINT_free(base);
    if (scalar) BN_free(scalar);
    if (key) EC_KEY_free(key);
    return rc;
}


static int reset_engine_session_state(void *user) {
    l3_security_engine *p = (l3_security_engine *)user;
    if (!p) return L3_SECURITY_ERR_ARGUMENT;
    p->nonce_counter = 0;
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
static int decode_app_private_scalar35(const uint8_t *bytes, size_t len,
                                    uint8_t tail35[35]);

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

static int import_private_key_or_saved_authorization(void *user, enum l3_import_record_kind slot, const uint8_t *bytes, size_t len,
                       l3_security_material **out) {
    l3_security_engine *p = (l3_security_engine *)user;
    if (!p || !bytes || !len || !out) return L3_SECURITY_ERR_ARGUMENT;
    if (slot == IMPORT_RECORD_APP_PRIVATE_KEY) {
        uint8_t tail35[35];
        int trc = decode_app_private_scalar35(bytes, len, tail35);
        if (trc == L3_SECURITY_OK) {
            dump_authorization_trace(p, "imported_tail35", tail35, sizeof(tail35));
            int nrc = new_obj(MATERIAL_KIND_IMPORTED_PRIVATE_KEY, 0, (uint32_t)slot,
                              0x100u, tail35, sizeof(tail35), out);
            return nrc ? L3_SECURITY_ERR_ENGINE : L3_SECURITY_OK;
        }
    }
    if (slot == IMPORT_RECORD_SAVED_AUTHORIZATION) {
        int irc = authorization_export_try_import_root149(bytes, len, out);
        if (irc == L3_SECURITY_OK) return L3_SECURITY_OK;
    }
    uint8_t digest[32];
    int rc = derive32(p, slot == IMPORT_RECORD_SAVED_AUTHORIZATION ? "import.root" : "import.obj",
                      bytes, len, NULL, 0, (uint32_t)slot, 0, digest);
    if (rc) return L3_SECURITY_ERR_ENGINE;

    size_t obj_len = len + sizeof(digest);
    uint8_t *buf = (uint8_t *)malloc(obj_len);
    if (!buf) return L3_SECURITY_ERR_NOMEM;
    memcpy(buf, digest, sizeof(digest));
    memcpy(buf + sizeof(digest), bytes, len);
    uint32_t kind = (slot == IMPORT_RECORD_SAVED_AUTHORIZATION) ? MATERIAL_KIND_AUTHORIZATION_ROOT : MATERIAL_KIND_IMPORTED_PRIVATE_KEY;
    rc = new_obj(kind, 0, (uint32_t)slot, 0, buf, obj_len, out);
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
        if (p->deterministic_entropy_fixture_seen && matches_captured_peer_public_key(bytes, len)) {
            p->captured_peer_public_key_seen = 1u;
        }

        dump_authorization_trace(p, "p6_dbkey_public64", object->bytes, 64u);
        dump_authorization_trace(p, "p6_dbkey_private32", object->bytes + 64u, 32u);
        dump_authorization_trace(p, "p6_peer_public64", bytes, len);
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


static const l3_authorization_frame_transform_tables g_generated_frame_transform_tables = {
    k_dat429, sizeof(k_dat429),
    k_dat40c6a52, sizeof(k_dat40c6a52)
};

static const l3_authorization_frame_tables g_encoded_frame_tables = {
    k_dat429, sizeof(k_dat429),
    k_dat40e8423, sizeof(k_dat40e8423),
    k_dat40e85ab, sizeof(k_dat40e85ab)
};

static const l3_authorization_digest_tables g_authorization_digest_tables = {
    k_dat429, sizeof(k_dat429),
    k_dat40eb4cd, sizeof(k_dat40eb4cd),
    k_dat40eb3ad, sizeof(k_dat40eb3ad)
};

static const l3_private_key_tables g_private_key_decode_tables = {
    k_dat429, sizeof(k_dat429),
    k_pi2_dat416, sizeof(k_pi2_dat416),
    k_pi2_dat43d, sizeof(k_pi2_dat43d),
    k_pi2_prefix, sizeof(k_pi2_prefix),
    k_pi2_program800, sizeof(k_pi2_program800),
    k_pi2_program80f, sizeof(k_pi2_program80f)
};

static const l3_private_key_record_decoder_tables g_private_key_import_tables = {
    k_dat429, sizeof(k_dat429),
    k_pi2_dat40c6435, sizeof(k_pi2_dat40c6435),
    k_dat40c6a52, sizeof(k_dat40c6a52),
    k_pi2_dat40c65f0, sizeof(k_pi2_dat40c65f0),
    k_pi2_dat40e8251, sizeof(k_pi2_dat40e8251)
};

static int decode_app_private_scalar35(const uint8_t *bytes, size_t len,
                                    uint8_t tail35[35]) {
    if (!bytes || !tail35) return L3_SECURITY_ERR_ARGUMENT;
    l3_private_key_record_header record_header;
    if (l3_private_key_parse_record_header(bytes, len, &record_header) != L3_PRIVATE_KEY_OK ||
        record_header.bit_len != 0x100u) {
        return L3_SECURITY_ERR_ARGUMENT;
    }

    l3_private_key_decode_replay_context *replay =
        (l3_private_key_decode_replay_context *)calloc(1u, sizeof(*replay));
    if (!replay) return L3_SECURITY_ERR_NOMEM;
    int rc = l3_private_key_decode_replay_context_init_from_manager(
        replay, &g_private_key_decode_tables,
        k_pi2_program800, sizeof(k_pi2_program800),
        k_pi2_program80f, sizeof(k_pi2_program80f),
        k_pi2_manager, sizeof(k_pi2_manager));

    uint8_t masked_frames[L3_PRIVATE_KEY_MASKED_FRAMES_LEN];
    uint8_t payload[L3_PRIVATE_KEY_PAYLOAD_LEN];
    uint8_t padded[L3_PRIVATE_KEY_PAYLOAD_LEN + 2u];
    uint8_t state76[76];
    if (rc == 0) {
        rc = l3_private_key_decode_blocks_with_object_method(
            &g_private_key_import_tables, l3_private_key_decode_object_method_replay_cb, replay,
            bytes, len, k_pi2_manager, sizeof(k_pi2_manager), k_pi2_mask,
            masked_frames, sizeof(masked_frames));
    }
    if (rc == 0) {
        rc = l3_private_key_build_payload_from_masked_frames(
            &g_private_key_import_tables, masked_frames, sizeof(masked_frames), record_header.bit_len,
            payload, sizeof(payload));
    }
    if (rc == 0) {
        const l3_authorization_scalar_state_encoder_tables tables = {
            k_dat429, sizeof(k_dat429),
            k_dat40c6a52, sizeof(k_dat40c6a52)
        };
        memset(padded, 0, sizeof(padded));
        memcpy(padded, payload, sizeof(payload));
        rc = l3_authorization_encode_scalar_state_bytes76(&tables, padded, state76);
    }
    if (rc == 0) {
        rc = l3_p256_decode_app_private_scalar35(
            k_p6_parent_context_3, state76, tail35);
    }
    free(replay);
    return rc == 0 ? L3_SECURITY_OK : L3_SECURITY_ERR_ENGINE;
}

static const l3_challenge_block_tables g_f407_tables = {
    k_dat426, sizeof(k_dat426),
    k_dat429, sizeof(k_dat429)
};

int l3_security_engine_set_challenge_constructor_frame(l3_security_engine *provider,
                                                const uint8_t *frame66,
                                                size_t frame_len) {
    if (!provider || !frame66 || frame_len != L3_SECURITY_ENGINE_CHALLENGE_CONSTRUCTOR_FRAME_LEN) {
        return L3_SECURITY_ENGINE_ERR_ARGUMENT;
    }
    uint8_t payload[L3_SECURITY_ENGINE_CHALLENGE_CONTEXT_PAYLOAD_LEN];
    if (l3_challenge_build_context_payload(&g_f407_tables, frame66, payload) != 0) {
        return L3_SECURITY_ENGINE_ERR_CRYPTO;
    }
    return l3_security_engine_set_challenge_block_payload(provider, payload, sizeof(payload));
}

int l3_security_engine_set_challenge_constructor_frame_from_file(l3_security_engine *provider,
                                                          const char *path) {
    if (!provider || !path || !path[0]) return L3_SECURITY_ENGINE_ERR_ARGUMENT;
    int fd = open(path, O_RDONLY);
    if (fd < 0) return L3_SECURITY_ENGINE_ERR_CRYPTO;
    uint8_t buf[L3_SECURITY_ENGINE_CHALLENGE_CONSTRUCTOR_FRAME_LEN + 1u];
    size_t off = 0;
    while (off < sizeof(buf)) {
        ssize_t got = read(fd, buf + off, sizeof(buf) - off);
        if (got < 0) { close(fd); return L3_SECURITY_ENGINE_ERR_CRYPTO; }
        if (got == 0) break;
        off += (size_t)got;
    }
    close(fd);
    if (off != L3_SECURITY_ENGINE_CHALLENGE_CONSTRUCTOR_FRAME_LEN) {
        return L3_SECURITY_ENGINE_ERR_ARGUMENT;
    }
    return l3_security_engine_set_challenge_constructor_frame(provider, buf, off);
}


static int suffix_matches(const char *s, const char *suffix) {
    if (!s || !suffix) return 0;
    size_t ns = strlen(s), nx = strlen(suffix);
    return ns >= nx && memcmp(s + ns - nx, suffix, nx) == 0;
}

static int join_path(char *out, size_t out_cap, const char *dir, const char *name) {
    if (!out || !out_cap || !dir || !name || !dir[0] || !name[0]) return -1;
    size_t nd = strlen(dir), nn = strlen(name);
    int need_slash = dir[nd - 1u] != '/';
    if (nd + (size_t)need_slash + nn + 1u > out_cap) return -1;
    memcpy(out, dir, nd);
    size_t off = nd;
    if (need_slash) out[off++] = '/';
    memcpy(out + off, name, nn + 1u);
    return 0;
}

static int probe_dir_find_suffix(const char *dir_path, const char *suffix,
                                 char *out_path, size_t out_path_cap) {
    if (!dir_path || !suffix || !out_path || !out_path_cap) return L3_SECURITY_ENGINE_ERR_ARGUMENT;
    DIR *d = opendir(dir_path);
    if (!d) return L3_SECURITY_ENGINE_ERR_CRYPTO;
    char best[512];
    best[0] = '\0';
    struct dirent *de;
    while ((de = readdir(d)) != NULL) {
        const char *name = de->d_name;
        if (name[0] == '.') continue;
        if (!suffix_matches(name, suffix)) continue;
        if (best[0] == '\0' || strcmp(name, best) < 0) {
            size_t n = strlen(name);
            if (n < sizeof(best)) memcpy(best, name, n + 1u);
        }
    }
    closedir(d);
    if (best[0] == '\0') return L3_SECURITY_ENGINE_ERR_ARGUMENT;
    if (join_path(out_path, out_path_cap, dir_path, best) != 0) return L3_SECURITY_ENGINE_ERR_ARGUMENT;
    return L3_SECURITY_ENGINE_OK;
}

int l3_security_engine_load_challenge_material_from_probe_dir(l3_security_engine *provider,
                                             const char *dir_path) {
    if (!provider || !dir_path || !dir_path[0]) return L3_SECURITY_ENGINE_ERR_ARGUMENT;
    char path[1024];
    int rc;

    /* Prefer the exact V402 constructor input to FUN_f4072318.  It exercises
     * the local f407 constructor port instead of bypassing it with a full
     * payload dump. */
    rc = probe_dir_find_suffix(dir_path, "_dat_f4295184_data_ptr_constructor_0x42.bin",
                               path, sizeof(path));
    if (rc == L3_SECURITY_ENGINE_OK) {
        int prc = l3_security_engine_set_challenge_constructor_frame_from_file(provider, path);
        if (prc == L3_SECURITY_ENGINE_OK) return prc;
    }

    /* Fall back to the native object self dump captured by the vtable hook. */
    rc = probe_dir_find_suffix(dir_path, "_f407_vtable0_self_0x10b4_before.bin",
                               path, sizeof(path));
    if (rc == L3_SECURITY_ENGINE_OK) {
        int prc = l3_security_engine_set_challenge_block_payload_from_file(provider, path);
        if (prc == L3_SECURITY_ENGINE_OK) return prc;
    }

    /* Last fallback: raw f407 payload dump. */
    rc = probe_dir_find_suffix(dir_path, "_f407_vtable0_payload_0x10b0_before.bin",
                               path, sizeof(path));
    if (rc == L3_SECURITY_ENGINE_OK) {
        int prc = l3_security_engine_set_challenge_block_payload_from_file(provider, path);
        if (prc == L3_SECURITY_ENGINE_OK) return prc;
    }
    return L3_SECURITY_ENGINE_ERR_ARGUMENT;
}

static int security_engine_try_import_challenge_probe_dir(l3_security_engine *provider) {
    if (!provider || provider->injected_challenge_block_payload_ready || provider->challenge_probe_dir_tried ||
        provider->challenge_probe_dir_path[0] == '\0') {
        return provider && provider->injected_challenge_block_payload_ready ? L3_SECURITY_ENGINE_OK : L3_SECURITY_ENGINE_ERR_ARGUMENT;
    }
    provider->challenge_probe_dir_tried = 1u;
    return l3_security_engine_load_challenge_material_from_probe_dir(provider, provider->challenge_probe_dir_path);
}

static int security_engine_try_import_challenge_constructor_frame_file(l3_security_engine *provider) {
    if (!provider || provider->injected_challenge_block_payload_ready ||
        provider->challenge_constructor_frame_file_tried ||
        provider->challenge_constructor_frame_path[0] == '\0') {
        return provider && provider->injected_challenge_block_payload_ready ? L3_SECURITY_ENGINE_OK : L3_SECURITY_ENGINE_ERR_ARGUMENT;
    }
    provider->challenge_constructor_frame_file_tried = 1u;
    return l3_security_engine_set_challenge_constructor_frame_from_file(provider, provider->challenge_constructor_frame_path);
}

typedef struct f407_block_user {
    uint8_t payload[L3_CHALLENGE_BLOCK_CONTEXT_PAYLOAD_LEN];
} f407_block_user;

static int challenge_context_encrypt_block(void *user, const uint8_t in[16], uint8_t out[16]) {
    f407_block_user *ctx = (f407_block_user *)user;
    if (!ctx || !in || !out) return -1;
    return l3_challenge_cipher_context_encrypt(&g_f407_tables, ctx->payload, in, out);
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
    memset(fo, 0, sizeof(*fo));
    fo->type_word = 0;
    fo->byte_len = o->metadata0;
    fo->frame_count = o->metadata1;
    fo->frames = (uint8_t *)malloc(o->len);
    if (!fo->frames) return L3_AUTH_ERR_ALLOC;
    memcpy(fo->frames, o->bytes, o->len);
    return L3_AUTH_OK;
}

static int derive_encoded_shared_point(const uint8_t tail35[35],
                                     const uint8_t input64[64],
                                     const uint8_t *parent_context,
                                     size_t parent_context_len,
                                     l3_authorization_frame_stream *out) {
    if (!tail35 || !input64 || !parent_context || !out ||
        parent_context_len < 0x1c80u) return L3_SECURITY_ERR_ARGUMENT;
    uint8_t left66[L3_AUTH_FRAME_LEN] = {0};
    uint8_t right66[L3_AUTH_FRAME_LEN] = {0};
    for (unsigned i = 0; i < 32u; ++i) {
        left66[i] = input64[31u - i];
        right66[i] = input64[63u - i];
    }

    l3_authorization_round_pipeline_result *fbc =
        (l3_authorization_round_pipeline_result *)calloc(1u, sizeof(*fbc));
    l3_authorization_final_result *fd4 =
        (l3_authorization_final_result *)calloc(1u, sizeof(*fd4));
    if (!fbc || !fd4) {
        free(fbc);
        free(fd4);
        return L3_SECURITY_ERR_NOMEM;
    }

    int rc = l3_authorization_run_round_pipeline(
        (const int32_t *)(const void *)parent_context,
        parent_context_len / sizeof(int32_t),
        left66, right66, tail35, 59u, fbc);
    if (rc != 0 || fbc->rounds_run != 59u ||
        fbc->final_state.round.has_next_round != 0u) {
        free(fbc);
        free(fd4);
        return L3_SECURITY_ERR_ENGINE;
    }

    uint32_t param2[13];
    uint32_t parent[13];
    uint32_t parent_pair_lo, parent_pair_hi;
    memcpy(param2, fbc->final_state.round.param5_words13, sizeof(param2));
    memcpy(parent, parent_context, sizeof(parent));
    memcpy(&parent_pair_lo, parent_context + 0x1c78u,
           sizeof(parent_pair_lo));
    memcpy(&parent_pair_hi, parent_context + 0x1c7cu,
           sizeof(parent_pair_hi));

    rc = l3_authorization_finalize_round_result(
        param2, fbc->final_state.round.param7_words13, parent,
        parent_pair_lo, parent_pair_hi, 512u, 512u, 512u, fd4);
    if (rc != 0 || !fd4->terminal_taken) {
        free(fbc);
        free(fd4);
        return L3_SECURITY_ERR_ENGINE;
    }

    uint8_t raw76[0x4c];
    uint8_t state10a[0x10a];
    uint8_t frames[2u * L3_AUTH_FRAME_LEN];
    rc = l3_authorization_transform_shared_point_tail(fd4->raw40, raw76);
    if (rc == 0) {
        rc = l3_authorization_normalize_frame_state(&g_generated_frame_transform_tables,
                                      raw76, state10a);
    }
    uint32_t required = 0;
    if (rc == 0) {
        rc = l3_authorization_expand_frame_state(
            &g_generated_frame_transform_tables, state10a, 0x20u,
            frames, sizeof(frames), &required);
    }
    if (rc == 0 && required == 2u) {
        rc = l3_authorization_frame_stream_copy(out, 0u, frames, 2u, 0x20u);
    } else if (rc == 0) {
        rc = L3_SECURITY_ERR_ENGINE;
    }
    free(fbc);
    free(fd4);
    return rc == 0 ? L3_SECURITY_OK : L3_SECURITY_ERR_ENGINE;
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
    const uint8_t *parent_context =
        object->kind == MATERIAL_KIND_EPHEMERAL_KEYPAIR ? k_p6_parent_context_db
                                        : k_p6_parent_context_3;
    const size_t parent_context_len =
        object->kind == MATERIAL_KIND_EPHEMERAL_KEYPAIR ? sizeof(k_p6_parent_context_db)
                                        : sizeof(k_p6_parent_context_3);
    l3_authorization_frame_stream fo;
    memset(&fo, 0, sizeof(fo));
    int rc = derive_encoded_shared_point(tail35, bytes, parent_context,
                                        parent_context_len, &fo);
    if (rc != L3_AUTH_OK) return L3_SECURITY_ERR_ENGINE;
    if (fo.frames && fo.frame_count) {
        dump_authorization_trace(p, object->kind == MATERIAL_KIND_EPHEMERAL_KEYPAIR ? "transform_db_frames" : "transform_imported_frames",
                        fo.frames, (size_t)fo.frame_count * L3_AUTH_FRAME_LEN);
    }
    int nrc = new_encoded_frames_security_object(0, &fo, out);
    l3_authorization_frame_stream_clear(&fo);
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
            if (!rc) rc = l3_authorization_frames_digest(&g_encoded_frame_tables, &in, scalar, sizeof(scalar), NULL, 0, &dcb, &mixed);
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
            if (!rc) rc = l3_authorization_frames_extract_range(&g_encoded_frame_tables, &in, params->offset, params->length, &sliced);
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
                dump_authorization_trace(p, "root_visible16", rootbuf, 16u);
                dump_authorization_trace(p, "root_constructor_frame66", rootbuf + 16u, L3_AUTH_FRAME_LEN);
                dump_authorization_trace(p, "root_object82", rootbuf, sizeof(rootbuf));
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
                dump_authorization_trace(p, "root_visible16", rootbuf, 16u);
                dump_authorization_trace(p, "root_fe_frame84", rootbuf + 16u, 0x84u);
                dump_authorization_trace(p, "root_object148", rootbuf, sizeof(rootbuf));
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

typedef struct trace_block16_user {
    l3_security_engine *provider;
    ccm_block16_fn inner;
    void *inner_user;
    const char *prefix;
} trace_block16_user;

static int trace_block16_cb(void *user, const uint8_t in[16], uint8_t out[16]) {
    trace_block16_user *tu = (trace_block16_user *)user;
    if (!tu || !tu->provider || !tu->inner || !in || !out) return L3_SECURITY_ENGINE_ERR_ARGUMENT;
    char in_suffix[64];
    char out_suffix[64];
    const char *prefix = (tu->prefix && tu->prefix[0]) ? tu->prefix : "bar_block16";
    snprintf(in_suffix, sizeof(in_suffix), "%s_in", prefix);
    snprintf(out_suffix, sizeof(out_suffix), "%s_out", prefix);
    dump_authorization_trace(tu->provider, in_suffix, in, 16u);
    int rc = tu->inner(tu->inner_user, in, out);
    if (rc == 0) dump_authorization_trace(tu->provider, out_suffix, out, 16u);
    return rc;
}

typedef struct aes128_block_user {
    AES_KEY aes;
} aes128_block_user;

static int aes128_encrypt_block(const AES_KEY *aes, const uint8_t in[16], uint8_t out[16]) {
    if (!aes || !in || !out || !AES_encrypt) return 0;
    AES_encrypt(in, out, aes);
    return 1;
}

static int aes128_block_cb(void *user, const uint8_t in[16], uint8_t out[16]) {
    aes128_block_user *ctx = (aes128_block_user *)user;
    if (!ctx) return L3_SECURITY_ENGINE_ERR_ARGUMENT;
    return aes128_encrypt_block(&ctx->aes, in, out) ? L3_SECURITY_ENGINE_OK : L3_SECURITY_ENGINE_ERR_CRYPTO;
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

    uint8_t tmp[0x400];
    if (plain_len > sizeof(tmp)) return L3_SECURITY_ENGINE_ERR_ARGUMENT;
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



static int process_challenge_cipher(l3_security_engine *p, uint32_t mode_selector,
                      const uint8_t *arg0, size_t arg0_len,
                      const uint8_t *arg1, size_t arg1_len,
                      const l3_security_material *root,
                      uint8_t **out, size_t *out_len) {
    if (!p || !arg0 || !arg1 || !root || !out || !out_len) return L3_SECURITY_ERR_ARGUMENT;
    if (root->kind != MATERIAL_KIND_AUTHORIZATION_ROOT) return L3_SECURITY_ERR_STATE;
    if (root->len < 16u && !p->challenge_block_encrypt) return L3_SECURITY_ERR_STATE;
    const int root_has_f407_frame = (root->len == (16u + L3_CHALLENGE_BLOCK_CONTEXT_ENCODED_LEN));
#ifdef L3_EMBED_CHALLENGE_WHITEBOX_BLOCK_CIPHER
    const int root_has_fe_frame = (root->len >= (16u + 0x84u));
#else
    const int root_has_fe_frame = 0;
#endif
    *out = NULL;
    *out_len = 0;

    if (arg0_len >= 7u && arg0_len <= 13u && (p->challenge_block_encrypt || root_has_f407_frame || root_has_fe_frame || root->len >= 16u)) {
        /*
         * Native FUN_f3f3455c receives the Java-visible arg0 pointer and its
         * length directly as the CCM nonce and nonce length.  The {0,4}
         * parameter block selects a 4-byte authentication field; it does not
         * extend the nonce.  Output layout is ciphertext||tag.
         *
         * Important v179 correction: native CCM calls a generated 16-byte
         * block-cipher object through a vtable.  Treating DAT_f4295184 as a
         * raw AES key is only the fallback path; app/native integrations can
         * now provide the actual block function with challenge_block_encrypt.
         */
        ccm_block16_fn block = NULL;
        void *block_user = NULL;
        aes128_block_user aes_ctx;
        memset(&aes_ctx, 0, sizeof(aes_ctx));
        f407_block_user f407_ctx;
        memset(&f407_ctx, 0, sizeof(f407_ctx));
#ifdef L3_EMBED_CHALLENGE_WHITEBOX_BLOCK_CIPHER
        l3_challenge_whitebox_block_cipher fe_ctx;
        memset(&fe_ctx, 0, sizeof(fe_ctx));
#endif

        if (root_has_f407_frame) {
            if (!p->injected_challenge_block_payload_ready && p->challenge_block_payload_path[0] != '\0') {
                (void)security_engine_try_import_challenge_block_payload_file(p);
            }
            if (!p->injected_challenge_block_payload_ready && p->challenge_constructor_frame_path[0] != '\0') {
                (void)security_engine_try_import_challenge_constructor_frame_file(p);
            }
            if (!p->injected_challenge_block_payload_ready && p->challenge_probe_dir_path[0] != '\0') {
                (void)security_engine_try_import_challenge_probe_dir(p);
            }
            if (p->injected_challenge_block_payload_ready) {
                memcpy(f407_ctx.payload, p->injected_challenge_block_payload, sizeof(f407_ctx.payload));
            } else if (p->captured_challenge_block_payload_ready) {
                memcpy(f407_ctx.payload, p->captured_challenge_block_payload, sizeof(f407_ctx.payload));
            } else if (l3_challenge_build_context_payload(&g_f407_tables, root->bytes + 16u, f407_ctx.payload) != 0) {
                return L3_SECURITY_ERR_ENGINE;
            }
            block = challenge_context_encrypt_block;
            block_user = &f407_ctx;
        }
#ifdef L3_EMBED_CHALLENGE_WHITEBOX_BLOCK_CIPHER
        else if (root_has_fe_frame) {
            if (l3_challenge_whitebox_block_cipher_init(&fe_ctx, root->bytes + 16u) != L3_CHALLENGE_BLOCK_CIPHER_OK) {
                return L3_SECURITY_ERR_ENGINE;
            }
            block = l3_challenge_whitebox_encrypt_block;
            block_user = &fe_ctx;
        }
#endif
        else if (p->challenge_block_encrypt) {
            block = p->challenge_block_encrypt;
            block_user = p->challenge_block_user;
        } else {
            if (!AES_set_encrypt_key || !AES_encrypt || AES_set_encrypt_key(root->bytes, 128, &aes_ctx.aes) != 0) {
                return L3_SECURITY_ERR_ENGINE;
            }
            block = aes128_block_cb;
            block_user = &aes_ctx;
        }

        trace_block16_user trace_block;
        memset(&trace_block, 0, sizeof(trace_block));
        const char *trace_dir = getenv("L3_AUTH_ROOT_TRACE_DIR");
        if (trace_dir && trace_dir[0]) {
            trace_block.provider = p;
            trace_block.inner = block;
            trace_block.inner_user = block_user;
            trace_block.prefix = (mode_selector == CHALLENGE_OPERATION_ENCRYPT) ? "bar7_block16" : "bar8_block16";
            block = trace_block16_cb;
            block_user = &trace_block;
        }

        if (mode_selector == CHALLENGE_OPERATION_ENCRYPT) {
            uint8_t *buf = (uint8_t *)malloc(arg1_len + 4u);
            if (!buf) return L3_SECURITY_ERR_NOMEM;
            dump_authorization_trace(p, "bar7_nonce", arg0, arg0_len);
            dump_authorization_trace(p, "bar7_plain", arg1, arg1_len);
            if (root && root->bytes && root->len) dump_authorization_trace(p, "bar7_root_object", root->bytes, root->len);
            int rc = ccm_encrypt_block16(block, block_user, arg0, arg0_len, arg1, arg1_len, buf);
            if (rc) { free(buf); return L3_SECURITY_ERR_ENGINE; }
            dump_authorization_trace(p, "bar7_cipher_tag", buf, arg1_len + 4u);
            *out = buf;
            *out_len = arg1_len + 4u;
            return L3_SECURITY_OK;
        }

        if (mode_selector == CHALLENGE_OPERATION_DECRYPT) {
            if (arg1_len < 4u) return L3_SECURITY_ERR_ARGUMENT;
            size_t clen = arg1_len - 4u;
            uint8_t *buf = (uint8_t *)malloc(clen ? clen : 1u);
            if (!buf) return L3_SECURITY_ERR_NOMEM;
            size_t plain_len = 0;
            int rc = ccm_decrypt_block16(block, block_user, arg0, arg0_len, arg1, arg1_len, buf, &plain_len);
            if (rc) { free(buf); return L3_SECURITY_ERR_ENGINE; }
            *out = buf;
            *out_len = plain_len;
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
        uint8_t *buf = (uint8_t *)malloc(arg1_len + 4u);
        if (!buf) return L3_SECURITY_ERR_NOMEM;
        rc = aes_ctr_xor(kbuf, ivfull, arg1, arg1_len, buf);
        if (rc) { free(buf); return L3_SECURITY_ERR_ENGINE; }
        uint8_t tagdata[32];
        rc = derive32(p, "bar.tag", kbuf, sizeof(kbuf), buf, arg1_len, (uint32_t)arg0_len, (uint32_t)arg1_len, tagdata);
        if (rc) { free(buf); return L3_SECURITY_ERR_ENGINE; }
        memcpy(buf + arg1_len, tagdata, 4);
        *out = buf;
        *out_len = arg1_len + 4u;
        return L3_SECURITY_OK;
    }

    if (mode_selector == CHALLENGE_OPERATION_DECRYPT) {
        if (arg1_len < 4u) return L3_SECURITY_ERR_ARGUMENT;
        size_t clen = arg1_len - 4u;
        uint8_t tagdata[32];
        rc = derive32(p, "bar.tag", kbuf, sizeof(kbuf), arg1, clen, (uint32_t)arg0_len, (uint32_t)clen, tagdata);
        if (rc) return L3_SECURITY_ERR_ENGINE;
        if (memcmp(tagdata, arg1 + clen, 4u) != 0) return L3_SECURITY_ERR_ENGINE;
        uint8_t *buf = (uint8_t *)malloc(clen ? clen : 1u);
        if (!buf) return L3_SECURITY_ERR_NOMEM;
        rc = aes_ctr_xor(kbuf, ivfull, arg1, clen, buf);
        if (rc) { free(buf); return L3_SECURITY_ERR_ENGINE; }
        *out = buf;
        *out_len = clen;
        return L3_SECURITY_OK;
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
static int engine_decode_app_private_key(void *user,
                                          const uint8_t *record,
                                          size_t record_len,
                                          l3_security_material **out) {
    return import_private_key_or_saved_authorization(user, IMPORT_RECORD_APP_PRIVATE_KEY, record, record_len, out);
}

static int engine_restore_authorization_record(void *user,
                                                 const uint8_t *record,
                                                 size_t record_len,
                                                 l3_security_material **out) {
    return import_private_key_or_saved_authorization(user, IMPORT_RECORD_SAVED_AUTHORIZATION, record, record_len, out);
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

static int engine_encrypt_challenge_reply(void *user,
                                           const uint8_t nonce7[L3_LEN_CHALLENGE_NONCE],
                                           const uint8_t plain36[L3_LEN_CHALLENGE_REPLY_PLAIN],
                                           const l3_security_material *root,
                                           uint8_t **out,
                                           size_t *out_len) {
    return process_challenge_cipher((l3_security_engine *)user,
                                  CHALLENGE_OPERATION_ENCRYPT,
                                  nonce7, L3_LEN_CHALLENGE_NONCE,
                                  plain36, L3_LEN_CHALLENGE_REPLY_PLAIN,
                                  root, out, out_len);
}

static int engine_decrypt_challenge_response(void *user,
                                              const uint8_t nonce7[L3_LEN_CHALLENGE_NONCE],
                                              const uint8_t cipher60[L3_LEN_CHALLENGE_RESPONSE_CRYPT],
                                              const l3_security_material *root,
                                              uint8_t **out,
                                              size_t *out_len) {
    return process_challenge_cipher((l3_security_engine *)user,
                                  CHALLENGE_OPERATION_DECRYPT,
                                  nonce7, L3_LEN_CHALLENGE_NONCE,
                                  cipher60, L3_LEN_CHALLENGE_RESPONSE_CRYPT,
                                  root, out, out_len);
}

static int engine_export_authorization_record(void *user,
                                               const l3_security_material *root,
                                               uint8_t **out,
                                               size_t *out_len) {
    return export_authorization_record_material(user, root, MATERIAL_TAG_EXPORT_AUTHORIZATION, out, out_len);
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
        provider->challenge_block_encrypt = config->challenge_block_encrypt;
        provider->challenge_block_user = config->challenge_block_user;
        if (config->challenge_block_payload_path && config->challenge_block_payload_path[0]) {
            size_t path_len = strlen(config->challenge_block_payload_path);
            if (path_len >= sizeof(provider->challenge_block_payload_path)) return L3_SECURITY_ENGINE_ERR_ARGUMENT;
            memcpy(provider->challenge_block_payload_path, config->challenge_block_payload_path, path_len + 1u);
        }
        if (config->challenge_constructor_frame_path && config->challenge_constructor_frame_path[0]) {
            size_t path_len = strlen(config->challenge_constructor_frame_path);
            if (path_len >= sizeof(provider->challenge_constructor_frame_path)) return L3_SECURITY_ENGINE_ERR_ARGUMENT;
            memcpy(provider->challenge_constructor_frame_path, config->challenge_constructor_frame_path, path_len + 1u);
        }
        if (config->challenge_probe_dir_path && config->challenge_probe_dir_path[0]) {
            size_t path_len = strlen(config->challenge_probe_dir_path);
            if (path_len >= sizeof(provider->challenge_probe_dir_path)) return L3_SECURITY_ENGINE_ERR_ARGUMENT;
            memcpy(provider->challenge_probe_dir_path, config->challenge_probe_dir_path, path_len + 1u);
        }
        if (config->challenge_block_payload || config->challenge_block_payload_len) {
            int prc = l3_security_engine_set_challenge_block_payload(provider,
                                                       config->challenge_block_payload,
                                                       config->challenge_block_payload_len);
            if (prc != L3_SECURITY_ENGINE_OK) return prc;
        }
        if (config->challenge_constructor_frame || config->challenge_constructor_frame_len) {
            int frc = l3_security_engine_set_challenge_constructor_frame(provider,
                                                                 config->challenge_constructor_frame,
                                                                 config->challenge_constructor_frame_len);
            if (frc != L3_SECURITY_ENGINE_OK) return frc;
        }
    }

    if (provider->challenge_key_mode == L3_SECURITY_ENGINE_CHALLENGE_KEY_DEFAULT) {
        provider->challenge_key_mode = (uint32_t)L3_SECURITY_ENGINE_DEFAULT_CHALLENGE_KEY_MODE;
    }

    provider->operations.user = provider;
    provider->operations.begin_session = reset_engine_session_state;
    provider->operations.decode_app_private_key = engine_decode_app_private_key;
    provider->operations.restore_authorization_record = engine_restore_authorization_record;
    provider->operations.prepare_app_private_key = engine_prepare_app_private_key;
    provider->operations.generate_ephemeral_keypair = engine_generate_ephemeral_keypair;
    provider->operations.derive_shared_point = engine_derive_shared_point;
    provider->operations.concatenate_shared_points = engine_concatenate_shared_points;
    provider->operations.digest_shared_points = engine_digest_shared_points;
    provider->operations.extract_authorization_root = engine_extract_authorization_root;
    provider->operations.copy_ephemeral_public_key = engine_copy_ephemeral_public_key;
    provider->operations.encrypt_challenge_reply = engine_encrypt_challenge_reply;
    provider->operations.decrypt_challenge_response = engine_decrypt_challenge_response;
    provider->operations.export_authorization_record = engine_export_authorization_record;
    provider->operations.release_material = release_engine_material;
    return L3_SECURITY_ENGINE_OK;
}

const l3_handshake_operations *l3_security_engine_handshake_operations(l3_security_engine *provider) {
    return provider ? &provider->operations : NULL;
}



