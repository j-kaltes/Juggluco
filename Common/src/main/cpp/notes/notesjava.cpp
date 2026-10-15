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

#include <string.h>
#include "Notes.hpp"
#include "fromjava.h"

extern Notes *notes;

// Truncate raw UTF-8 to at most notemaxlen bytes without splitting a
// character sequence.
static uint16_t safecopy(char *dst, const char *src) {
    size_t rawlen = strlen(src);
    if(rawlen > notemaxlen) {
        size_t cut = notemaxlen;
        while(cut > 0 && (reinterpret_cast<const unsigned char*>(src)[cut] & 0xC0) == 0x80)
            --cut;
        rawlen = cut;
        }
    memcpy(dst, src, rawlen);
    dst[rawlen] = '\0';
    return (uint16_t)rawlen;
    }

// Copies jtext into dst (UTF-8, at most notemaxlen bytes). Returns the
// number of bytes written, or -1 when there is nothing to store.
static int getnotetext(JNIEnv *env, jstring jtext, char *dst) {
    if(!jtext)
        return -1;
    const char *raw = env->GetStringUTFChars(jtext, nullptr);
    if(!raw)
        return -1;
    const uint16_t len = safecopy(dst, raw);
    env->ReleaseStringUTFChars(jtext, raw);
    return len == 0 ? -1 : len;
    }

extern "C" JNIEXPORT jint JNICALL fromjava(addNote)(JNIEnv *env, jclass thiz, jint time, jstring jtext) {
    if(!notes)
        return -1;
    char tmpbuf[notemaxlen + 1];
    const int len = getnotetext(env, jtext, tmpbuf);
    if(len < 0)
        return -1;
    return notes->addnote((uint32_t)time, tmpbuf, (uint16_t)len);
    }

extern "C" JNIEXPORT jstring JNICALL fromjava(getNoteText)(JNIEnv *env, jclass thiz, jint offset) {
    if(!notes || offset < 0)
        return nullptr;
    const char* text = notes->gettext(offset);
    return env->NewStringUTF(text);
    }

extern "C" JNIEXPORT void JNICALL fromjava(deleteNote)(JNIEnv *env, jclass thiz, jint offset) {
    if(notes && offset >= 0)
        notes->removeat(offset);
    }

extern "C" JNIEXPORT jint JNICALL fromjava(updateNote)(JNIEnv *env, jclass thiz, jint offset, jint time, jstring jtext) {
    if(!notes || offset < 0)
        return -1;
    char tmpbuf[notemaxlen + 1];
    const int len = getnotetext(env, jtext, tmpbuf);
    if(len < 0)
        return -1;
    const uint32_t newoff = notes->updatenote((uint32_t)offset, (uint32_t)time, tmpbuf, (uint16_t)len);
    return (jint)newoff;
    }
