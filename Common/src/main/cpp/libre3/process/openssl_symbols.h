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
#include <openssl/ec.h>
#include <openssl/bn.h>
#include <openssl/aes.h>

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
L3_CRYPTO_DECS_EXTERN EC_KEY *(*EC_KEY_new_by_curve_nameptr)(int nid) L3_CRYPTO_DECS_INIT;
L3_CRYPTO_DECS_EXTERN int (*EC_KEY_generate_keyptr)(EC_KEY *key) L3_CRYPTO_DECS_INIT;
L3_CRYPTO_DECS_EXTERN const BIGNUM *(*EC_KEY_get0_private_keyptr)(const EC_KEY *key) L3_CRYPTO_DECS_INIT;
L3_CRYPTO_DECS_EXTERN const EC_POINT *(*EC_KEY_get0_public_keyptr)(const EC_KEY *key) L3_CRYPTO_DECS_INIT;
L3_CRYPTO_DECS_EXTERN void (*EC_KEY_freeptr)(EC_KEY *key) L3_CRYPTO_DECS_INIT;
L3_CRYPTO_DECS_EXTERN size_t (*EC_POINT_point2octptr)(const EC_GROUP *group, const EC_POINT *p, point_conversion_form_t form, unsigned char *buf, size_t len, BN_CTX *ctx) L3_CRYPTO_DECS_INIT;
L3_CRYPTO_DECS_EXTERN int (*BN_bn2binpadptr)(const BIGNUM *a, unsigned char *to, int tolen) L3_CRYPTO_DECS_INIT;
L3_CRYPTO_DECS_EXTERN BIGNUM *(*BN_bin2bnptr)(const unsigned char *s, int len, BIGNUM *ret) L3_CRYPTO_DECS_INIT;
L3_CRYPTO_DECS_EXTERN void (*BN_freeptr)(BIGNUM *a) L3_CRYPTO_DECS_INIT;
L3_CRYPTO_DECS_EXTERN EC_POINT *(*EC_POINT_newptr)(const EC_GROUP *group) L3_CRYPTO_DECS_INIT;
L3_CRYPTO_DECS_EXTERN void (*EC_POINT_freeptr)(EC_POINT *point) L3_CRYPTO_DECS_INIT;
L3_CRYPTO_DECS_EXTERN int (*EC_POINT_oct2pointptr)(const EC_GROUP *group, EC_POINT *p, const unsigned char *buf, size_t len, BN_CTX *ctx) L3_CRYPTO_DECS_INIT;
L3_CRYPTO_DECS_EXTERN int (*EC_POINT_mulptr)(const EC_GROUP *group, EC_POINT *r, const BIGNUM *n, const EC_POINT *q, const BIGNUM *m, BN_CTX *ctx) L3_CRYPTO_DECS_INIT;
L3_CRYPTO_DECS_EXTERN int (*AES_set_encrypt_keyptr)(const unsigned char *userKey, const int bits, AES_KEY *key) L3_CRYPTO_DECS_INIT;
L3_CRYPTO_DECS_EXTERN void (*AES_encryptptr)(const unsigned char *in, unsigned char *out, const AES_KEY *key) L3_CRYPTO_DECS_INIT;

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
L3_CRYPTO_PROJECT_EXTERN HMAC_CTX *(*HMAC_CTX_newptr)(void) L3_CRYPTO_PROJECT_INIT;
L3_CRYPTO_PROJECT_EXTERN void (*HMAC_CTX_freeptr)(HMAC_CTX *ctx) L3_CRYPTO_PROJECT_INIT;
L3_CRYPTO_PROJECT_EXTERN int (*HMAC_Init_exptr)(HMAC_CTX *ctx, const void *key, int len, const EVP_MD *md, ENGINE *impl) L3_CRYPTO_PROJECT_INIT;
L3_CRYPTO_PROJECT_EXTERN int (*HMAC_Updateptr)(HMAC_CTX *ctx, const unsigned char *data, size_t len) L3_CRYPTO_PROJECT_INIT;
L3_CRYPTO_PROJECT_EXTERN int (*HMAC_Finalptr)(HMAC_CTX *ctx, unsigned char *md, unsigned int *len) L3_CRYPTO_PROJECT_INIT;
L3_CRYPTO_PROJECT_EXTERN int (*RAND_bytesptr)(unsigned char *buf, int num) L3_CRYPTO_PROJECT_INIT;
L3_CRYPTO_PROJECT_EXTERN EVP_CIPHER_CTX *(*EVP_CIPHER_CTX_newptr)(void) L3_CRYPTO_PROJECT_INIT;
L3_CRYPTO_PROJECT_EXTERN void (*EVP_CIPHER_CTX_freeptr)(EVP_CIPHER_CTX *c) L3_CRYPTO_PROJECT_INIT;
L3_CRYPTO_PROJECT_EXTERN int (*EVP_CIPHER_CTX_set_paddingptr)(EVP_CIPHER_CTX *c, int pad) L3_CRYPTO_PROJECT_INIT;
L3_CRYPTO_PROJECT_EXTERN const EVP_CIPHER *(*EVP_aes_128_ecbptr)(void) L3_CRYPTO_PROJECT_INIT;
L3_CRYPTO_PROJECT_EXTERN const EVP_CIPHER *(*EVP_aes_256_ctrptr)(void) L3_CRYPTO_PROJECT_INIT;
L3_CRYPTO_PROJECT_EXTERN int (*EVP_EncryptInit_exptr)(EVP_CIPHER_CTX *ctx, const EVP_CIPHER *cipher, ENGINE *impl, const unsigned char *key, const unsigned char *iv) L3_CRYPTO_PROJECT_INIT;
L3_CRYPTO_PROJECT_EXTERN int (*EVP_EncryptUpdateptr)(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl, const unsigned char *in, int inl) L3_CRYPTO_PROJECT_INIT;
L3_CRYPTO_PROJECT_EXTERN int (*EVP_EncryptFinal_exptr)(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl) L3_CRYPTO_PROJECT_INIT;
L3_CRYPTO_PROJECT_EXTERN const EC_GROUP *(*EC_KEY_get0_groupptr)(const EC_KEY *key) L3_CRYPTO_PROJECT_INIT;

#undef L3_CRYPTO_PROJECT_EXTERN
#undef L3_CRYPTO_PROJECT_INIT

#ifdef __cplusplus
}
#endif

#endif /* L3_OPENSSL_SYMBOLS_H */
