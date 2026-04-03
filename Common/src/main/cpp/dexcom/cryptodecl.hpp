/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2 and 3 sensors.                         */
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
/*      Fri Nov 22 12:20:49 CET 2024                                                 */


#pragma once
BN_CTX *(*BN_CTX_newptr)(void) = nullptr;
int (*BN_addptr)(BIGNUM *r, const BIGNUM *a, const BIGNUM *b) = nullptr;
BIGNUM *(*BN_bin2bnptr)(const unsigned char *s, int len, BIGNUM *ret) = nullptr;
int (*BN_bn2binptr)(const BIGNUM *a, unsigned char *to) = nullptr;
int (*BN_bn2binpadptr)(const BIGNUM *a, unsigned char *to, int tolen) = nullptr;
char *(*BN_bn2hexptr)(const BIGNUM *a) = nullptr;
int (*BN_divptr)(BIGNUM *dv, BIGNUM *rem, const BIGNUM *a, const BIGNUM *d, BN_CTX *ctx) = nullptr;

BIGNUM *(*BN_dupptr)(const BIGNUM *a) = nullptr;
void (*BN_freeptr)(BIGNUM *a) = nullptr;
int (*BN_hex2bnptr)(BIGNUM **a, const char *str) = nullptr;

int (*BN_mod_mulptr)(BIGNUM *r, const BIGNUM *a, const BIGNUM *b, const BIGNUM *m, BN_CTX *ctx) = nullptr;

int (*BN_mulptr)(BIGNUM *r, const BIGNUM *a, const BIGNUM *b, BN_CTX *ctx) = nullptr;

BIGNUM *(*BN_newptr)(void) = nullptr;
int (*BN_nnmodptr)(BIGNUM *r, const BIGNUM *m, const BIGNUM *d, BN_CTX *ctx) = nullptr;
int (*BN_rand_rangeptr)(BIGNUM *rnd, const BIGNUM *range) = nullptr;
int (*BN_subptr)(BIGNUM *r, const BIGNUM *a, const BIGNUM *b) = nullptr;

int (*EC_POINT_addptr)(const EC_GROUP *group, EC_POINT *r, const EC_POINT *a,
                         const EC_POINT *b, BN_CTX *ctx) = nullptr;

int (*EC_POINT_cmpptr)(const EC_GROUP *group, const EC_POINT *a, const EC_POINT *b, BN_CTX *ctx) = nullptr;


EC_POINT *(*EC_POINT_dupptr)(const EC_POINT *src, const EC_GROUP *group) = nullptr;
void (*EC_POINT_freeptr)(EC_POINT *point) = nullptr;


int (*EC_POINT_set_affine_coordinatesptr)(const EC_GROUP *group, EC_POINT *p, const BIGNUM *x, const BIGNUM *y, BN_CTX *ctx) = nullptr;
int (*EC_POINT_get_affine_coordinatesptr)(const EC_GROUP *group, const EC_POINT *p, BIGNUM *x, BIGNUM *y, BN_CTX *ctx) = nullptr;



int (*EC_POINT_mulptr)(const EC_GROUP *group, EC_POINT *r, const BIGNUM *n, const EC_POINT *q, const BIGNUM *m, BN_CTX *ctx) = nullptr;


EC_POINT *(*EC_POINT_newptr)(const EC_GROUP *group) = nullptr;

int (*EC_POINT_oct2pointptr)(const EC_GROUP *group, EC_POINT *p, const unsigned char *buf, size_t len, BN_CTX *ctx) = nullptr;


size_t (*EC_POINT_point2octptr)(const EC_GROUP *group, const EC_POINT *p, point_conversion_form_t form, unsigned char *buf, size_t len, BN_CTX *ctx) = nullptr;


int (*EVP_DigestFinalptr)(EVP_MD_CTX *ctx, unsigned char *md, unsigned int *s) = nullptr;


int (*EVP_DigestInitptr)(EVP_MD_CTX *ctx, const EVP_MD *type) = nullptr;
int (*EVP_DigestUpdateptr)(EVP_MD_CTX *ctx, const void *d, size_t cnt) = nullptr;
void (*EVP_MD_CTX_freeptr)(EVP_MD_CTX *ctx) = nullptr;
EVP_MD_CTX *(*EVP_MD_CTX_newptr)(void) = nullptr;
// BIGNUM *(*BN_native2bnptr)(const unsigned char *s, int len, BIGNUM *ret) = nullptr;


 const EVP_MD *(*EVP_sha256ptr)(void) = nullptr;


//void (*CRYPTO_freeptr)(void *str, const char *, int) = nullptr;
       void (*OPENSSL_freeptr)(void *addr) = nullptr;


// const BIGNUM *(*EC_GROUP_get0_orderptr)(const EC_GROUP *group) = nullptr;
   int (*EC_GROUP_get_orderptr)(const EC_GROUP *group, BIGNUM *order, BN_CTX *ctx) = nullptr;


extern void (*ERR_print_errors_cbptr)(int (*cb)(const char *str, size_t len, void *u), void *u) ;
#ifdef TEST
void (*ERR_print_errors_cbptr)(int (*cb)(const char *str, size_t len, void *u), void *u) = nullptr;
#endif

void (*ERR_clear_errorptr)(void) = nullptr;

const EC_POINT *(*EC_GROUP_get0_generatorptr)(const EC_GROUP *group) = nullptr;

 int (*EC_GROUP_get_curveptr)(const EC_GROUP *group, BIGNUM *p, BIGNUM *a, BIGNUM *b, BN_CTX *ctx) = nullptr;

   int (*EC_GROUP_get_cofactorptr)(const EC_GROUP *group, BIGNUM *cofactor, BN_CTX *ctx) = nullptr;


const BIGNUM *(*EC_KEY_get0_private_keyptr)(const EC_KEY *key) = nullptr;

const EC_POINT *(*EC_KEY_get0_public_keyptr)(const EC_KEY *key) = nullptr;
       void (*EC_KEY_freeptr)(EC_KEY *key) = nullptr;

EC_KEY *(*EC_KEY_new_by_curve_nameptr)(int nid) = nullptr;

        int (*EC_KEY_generate_keyptr)(EC_KEY *key) = nullptr;

     EC_GROUP *(*EC_GROUP_new_by_curve_nameptr)(int nid) = nullptr;

typedef struct ec_key_st EC_KEY;
     int (*EC_KEY_set_private_keyptr)(EC_KEY *key, const BIGNUM *priv_key) = nullptr;
     int (*EC_KEY_set_public_keyptr)(EC_KEY *key, const EC_POINT *pub) = nullptr;

   int (*ECDSA_sizeptr)(const EC_KEY *eckey) = nullptr;
   int (*ECDSA_signptr)(int type, const unsigned char *dgst, int dgstlen, unsigned char *sig, unsigned int *siglen, EC_KEY *eckey) = nullptr;


typedef struct aes_key_st AES_KEY;
int (*AES_set_encrypt_keyptr)(const unsigned char *userKey, const int bits, AES_KEY *key) = nullptr;
void (*AES_encryptptr)(const unsigned char *in, unsigned char *out, const AES_KEY *key) = nullptr;
