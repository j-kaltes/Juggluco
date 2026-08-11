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
#ifndef L3_OPENSSL_FUNCTION_REDIRECTS_H
#define L3_OPENSSL_FUNCTION_REDIRECTS_H

/* Remap only libcrypto calls actually used by this project. */
#ifdef L3_USE_CRYPTO_DLSYM
#include "openssl_symbols.h"

#define EVP_DigestInit_ex EVP_DigestInit_exptr
#define EVP_DigestUpdate EVP_DigestUpdateptr
#define EVP_DigestFinal_ex EVP_DigestFinal_exptr
#define EVP_MD_CTX_new EVP_MD_CTX_newptr
#define EVP_MD_CTX_free EVP_MD_CTX_freeptr
#define EVP_sha1 EVP_sha1ptr
#define EVP_sha256 EVP_sha256ptr
#define EVP_sha512 EVP_sha512ptr
#define HMAC HMACptr
#define HMAC_CTX_new HMAC_CTX_newptr
#define HMAC_CTX_free HMAC_CTX_freeptr
#define HMAC_Init_ex HMAC_Init_exptr
#define HMAC_Update HMAC_Updateptr
#define HMAC_Final HMAC_Finalptr
#define RAND_bytes RAND_bytesptr
#define EVP_CIPHER_CTX_new EVP_CIPHER_CTX_newptr
#define EVP_CIPHER_CTX_free EVP_CIPHER_CTX_freeptr
#define EVP_CIPHER_CTX_set_padding EVP_CIPHER_CTX_set_paddingptr
#define EVP_aes_128_ecb EVP_aes_128_ecbptr
#define EVP_aes_256_ctr EVP_aes_256_ctrptr
#define EVP_EncryptInit_ex EVP_EncryptInit_exptr
#define EVP_EncryptUpdate EVP_EncryptUpdateptr
#define EVP_EncryptFinal_ex EVP_EncryptFinal_exptr
#define EC_KEY_new_by_curve_name EC_KEY_new_by_curve_nameptr
#define EC_KEY_generate_key EC_KEY_generate_keyptr
#define EC_KEY_get0_private_key EC_KEY_get0_private_keyptr
#define EC_KEY_get0_public_key EC_KEY_get0_public_keyptr
#define EC_KEY_get0_group EC_KEY_get0_groupptr
#define EC_KEY_free EC_KEY_freeptr
#define EC_POINT_point2oct EC_POINT_point2octptr
#define BN_bn2binpad BN_bn2binpadptr
#define BN_bin2bn BN_bin2bnptr
#define BN_free BN_freeptr
#define EC_POINT_new EC_POINT_newptr
#define EC_POINT_free EC_POINT_freeptr
#define EC_POINT_oct2point EC_POINT_oct2pointptr
#define EC_POINT_mul EC_POINT_mulptr
#define AES_set_encrypt_key AES_set_encrypt_keyptr
#define AES_encrypt AES_encryptptr
#endif

#endif /* L3_OPENSSL_FUNCTION_REDIRECTS_H */
