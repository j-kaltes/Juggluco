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
int l3_sensor_security_load_app_key_and_saved_authorization(
    l3_sensor_security_context *context,
    const uint8_t *app_private_key,
    size_t app_private_key_len,
    const uint8_t *saved_authorization,
    size_t saved_authorization_len);
int l3_sensor_security_set_patch_certificate(
    l3_sensor_security_context *context,
    const uint8_t *patch_certificate,
    size_t patch_certificate_len);
int l3_sensor_security_create_ephemeral_public_key(
    l3_sensor_security_context *context,
    uint8_t **out,
    size_t *out_len);
int l3_sensor_security_derive_authorization_root(
    l3_sensor_security_context *context,
    const uint8_t *patch_ephemeral_public_key,
    size_t patch_ephemeral_public_key_len);
int l3_sensor_security_encrypt_challenge_reply(
    l3_sensor_security_context *context,
    const uint8_t *nonce7,
    size_t nonce7_len,
    const uint8_t *plain36,
    size_t plain36_len,
    uint8_t **out,
    size_t *out_len);
int l3_sensor_security_decrypt_challenge_response(
    l3_sensor_security_context *context,
    const uint8_t *nonce7,
    size_t nonce7_len,
    const uint8_t *cipher60,
    size_t cipher60_len,
    uint8_t **out,
    size_t *out_len);
int l3_sensor_security_export_saved_authorization(
    l3_sensor_security_context *context,
    uint8_t **out,
    size_t *out_len);

#ifdef __cplusplus
}
#endif
#endif
