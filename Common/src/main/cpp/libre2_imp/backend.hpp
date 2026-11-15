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
/*      Sun Nov 15 11:43:27 CET 2026                                                 */

#pragma once
#include <jni.h>
#include "inout.hpp"
#include "scanstate.hpp"
struct AlgorithmResults;
struct outobj;
namespace libre2_imp {
bool bluetooth_supported(const data_t *info);
bool needs_activation(const data_t *fram);
bool usable_state(scanstate *state,const data_t *info);
AlgorithmResults *scan(const data_t *uid,const data_t *info,data_t *data,
    scanstate *previous,scanstate *next,unsigned now,outobj *start,outobj *end,int warmup,unsigned known_start=0);
AlgorithmResults *stream(const data_t *uid,const data_t *info,const data_t *packet,
    scanstate *previous,scanstate *next,unsigned now,unsigned known_start=0, bool plaintext=false);
jint family(JNIEnv *,jobject,jint,jbyteArray,jbyteArray);
jboolean times(JNIEnv *,jobject,jint,jbyteArray,jobject,jobject,jbyteArray);
jbyte activation_command(JNIEnv *,jobject,jint,jbyteArray,jbyteArray);
jbyteArray activation_payload(JNIEnv *,jobject,jint,jbyteArray,jbyteArray,jbyte,jbyteArray);
jbyteArray enable_payload(JNIEnv *,jobject,jint,jbyteArray,jbyteArray,jbyte,jint,jbyteArray);
jbyteArray unlock_payload(JNIEnv *,jobject,jint,jbyteArray,jbyteArray,jbyte,jint,jint,jbyteArray);
jbyteArray activate_state(JNIEnv *,jobject,jbyteArray,jint,jint,jbyteArray,jbyteArray);
jint protocol_p1(JNIEnv *,jobject,jint,jint,jbyteArray,jbyteArray,jbyteArray);
jbyteArray protocol_p2(JNIEnv *,jobject,jint,jint,jbyteArray,jbyteArray,jbyteArray);
}
