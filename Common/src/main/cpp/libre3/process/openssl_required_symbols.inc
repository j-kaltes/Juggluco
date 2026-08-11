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
/* Minimal symbol list for l3_openssl_symbols_init().  Include only where symtest(name) is defined. */
symtest(EVP_DigestInit_ex);
symtest(EVP_DigestUpdate);
symtest(EVP_DigestFinal_ex);
symtest(EVP_MD_CTX_new);
symtest(EVP_MD_CTX_free);
symtest(EVP_sha1);
symtest(EVP_sha256);
symtest(EVP_sha512);
symtest(HMAC);
symtest(HMAC_CTX_new);
symtest(HMAC_CTX_free);
symtest(HMAC_Init_ex);
symtest(HMAC_Update);
symtest(HMAC_Final);
symtest(RAND_bytes);
symtest(EVP_CIPHER_CTX_new);
symtest(EVP_CIPHER_CTX_free);
symtest(EVP_CIPHER_CTX_set_padding);
symtest(EVP_aes_128_ecb);
symtest(EVP_aes_256_ctr);
symtest(EVP_EncryptInit_ex);
symtest(EVP_EncryptUpdate);
symtest(EVP_EncryptFinal_ex);

symtest(EC_KEY_new_by_curve_name);
symtest(EC_KEY_generate_key);
symtest(EC_KEY_get0_private_key);
symtest(EC_KEY_get0_public_key);
symtest(EC_KEY_get0_group);
symtest(EC_KEY_free);
symtest(EC_POINT_point2oct);
symtest(BN_bn2binpad);
symtest(BN_bin2bn);
symtest(BN_free);
symtest(EC_POINT_new);
symtest(EC_POINT_free);
symtest(EC_POINT_oct2point);
symtest(EC_POINT_mul);
symtest(AES_set_encrypt_key);
symtest(AES_encrypt);
