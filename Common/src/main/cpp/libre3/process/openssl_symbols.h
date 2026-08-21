#ifndef L3_OPENSSL_SYMBOLS_H
#define L3_OPENSSL_SYMBOLS_H

/*
 * Minimal dynamic libcrypto binding declarations for the app-path build.
 *
 * decs.h from the embedding project already owns storage for several OpenSSL
 * <name>ptr variables.  Those symbols are declared here as extern-only so this
 * library does not define them a second time.  This project defines storage only
 * for libcrypto functions that it actually uses and that are not present in the
 * supplied decs.h snapshot.
 */

#include <stddef.h>
#include <openssl/engine.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Already defined by the embedding program's decs.h; declare only here. */
#ifndef L3_CRYPTO_DEFINE_DECS_COMPAT_POINTERS
#define L3_CRYPTO_DECS_EXTERN extern
#define L3_CRYPTO_DECS_INIT
#else
/* Test/standalone builds may define these in exactly one helper TU. */
#define L3_CRYPTO_DECS_EXTERN
#define L3_CRYPTO_DECS_INIT = 0
#endif

L3_CRYPTO_DECS_EXTERN int (*EVP_DigestInit_exptr)(EVP_MD_CTX *ctx, const EVP_MD *type, ENGINE *impl) L3_CRYPTO_DECS_INIT;
L3_CRYPTO_DECS_EXTERN int (*EVP_DigestUpdateptr)(EVP_MD_CTX *ctx, const void *d, size_t cnt) L3_CRYPTO_DECS_INIT;
L3_CRYPTO_DECS_EXTERN int (*EVP_DigestFinal_exptr)(EVP_MD_CTX *ctx, unsigned char *md, unsigned int *s) L3_CRYPTO_DECS_INIT;
L3_CRYPTO_DECS_EXTERN EVP_MD_CTX *(*EVP_MD_CTX_newptr)(void) L3_CRYPTO_DECS_INIT;
L3_CRYPTO_DECS_EXTERN void (*EVP_MD_CTX_freeptr)(EVP_MD_CTX *ctx) L3_CRYPTO_DECS_INIT;
L3_CRYPTO_DECS_EXTERN const EVP_MD *(*EVP_sha256ptr)(void) L3_CRYPTO_DECS_INIT;
L3_CRYPTO_DECS_EXTERN const EVP_MD *(*EVP_sha512ptr)(void) L3_CRYPTO_DECS_INIT;

#undef L3_CRYPTO_DECS_EXTERN
#undef L3_CRYPTO_DECS_INIT

/* Owned by this project because these pointers were not present in decs.h. */
#ifdef L3_CRYPTO_DEFINE_PROJECT_POINTERS
#define L3_CRYPTO_PROJECT_EXTERN
#define L3_CRYPTO_PROJECT_INIT = 0
#else
#define L3_CRYPTO_PROJECT_EXTERN extern
#define L3_CRYPTO_PROJECT_INIT
#endif

L3_CRYPTO_PROJECT_EXTERN const EVP_MD *(*EVP_sha1ptr)(void) L3_CRYPTO_PROJECT_INIT;
L3_CRYPTO_PROJECT_EXTERN unsigned char *(*HMACptr)(const EVP_MD *evp_md, const void *key, int key_len, const unsigned char *d, size_t n, unsigned char *md, unsigned int *md_len) L3_CRYPTO_PROJECT_INIT;
L3_CRYPTO_PROJECT_EXTERN int (*RAND_bytesptr)(unsigned char *buf, int num) L3_CRYPTO_PROJECT_INIT;
L3_CRYPTO_PROJECT_EXTERN EVP_CIPHER_CTX *(*EVP_CIPHER_CTX_newptr)(void) L3_CRYPTO_PROJECT_INIT;
L3_CRYPTO_PROJECT_EXTERN void (*EVP_CIPHER_CTX_freeptr)(EVP_CIPHER_CTX *c) L3_CRYPTO_PROJECT_INIT;
L3_CRYPTO_PROJECT_EXTERN const EVP_CIPHER *(*EVP_aes_256_ctrptr)(void) L3_CRYPTO_PROJECT_INIT;
L3_CRYPTO_PROJECT_EXTERN int (*EVP_EncryptInit_exptr)(EVP_CIPHER_CTX *ctx, const EVP_CIPHER *cipher, ENGINE *impl, const unsigned char *key, const unsigned char *iv) L3_CRYPTO_PROJECT_INIT;
L3_CRYPTO_PROJECT_EXTERN int (*EVP_EncryptUpdateptr)(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl, const unsigned char *in, int inl) L3_CRYPTO_PROJECT_INIT;
L3_CRYPTO_PROJECT_EXTERN int (*EVP_EncryptFinal_exptr)(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl) L3_CRYPTO_PROJECT_INIT;
#undef L3_CRYPTO_PROJECT_EXTERN
#undef L3_CRYPTO_PROJECT_INIT

#ifdef __cplusplus
}
#endif

#endif /* L3_OPENSSL_SYMBOLS_H */
