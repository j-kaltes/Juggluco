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
/*      Sun Aug 30 10:21:11 CEST 2026                                                */

#ifndef L3_OPENSSL_SYMBOLS_H
#define L3_OPENSSL_SYMBOLS_H

/* Flat-core Android builds dynamically resolve just the secure entropy entry
 * point.  All saved-record hashing and challenge cryptography is portable C. */
#include <openssl/rand.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef L3_CRYPTO_DEFINE_PROJECT_POINTERS
#define L3_CRYPTO_PROJECT_EXTERN
#define L3_CRYPTO_PROJECT_INIT = 0
#else
#define L3_CRYPTO_PROJECT_EXTERN extern
#define L3_CRYPTO_PROJECT_INIT
#endif

L3_CRYPTO_PROJECT_EXTERN int (*RAND_bytesptr)(unsigned char *buf, int num) L3_CRYPTO_PROJECT_INIT;
#undef L3_CRYPTO_PROJECT_EXTERN
#undef L3_CRYPTO_PROJECT_INIT

#ifdef __cplusplus
}
#endif

#endif /* L3_OPENSSL_SYMBOLS_H */
