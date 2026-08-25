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

#ifdef USE_FREETEXT_NOTE

#include <string.h>
#include <string_view>
#include "Notes.hpp"
#include "fromjava.h"

extern jstring myNewStringUTF(JNIEnv *env,const std::string_view str);
extern Notes *notes;

extern "C" JNIEXPORT jint JNICALL fromjava(addNote)(JNIEnv *env, jclass thiz, jint time, jstring jtext) {
    if(!notes)
        return -1;
    jint rawlen = env->GetStringUTFLength(jtext);
    if(rawlen <= 0)
        return -1;
    uint16_t len = rawlen > notemaxlen ? notemaxlen : (uint16_t)rawlen;
    char tmpbuf[notemaxlen + 1];
    env->GetStringUTFRegion(jtext, 0, env->GetStringLength(jtext) < len ? env->GetStringLength(jtext) : len, tmpbuf);
    tmpbuf[len] = '\0';
    return notes->addnote(time, tmpbuf, len);
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

extern "C" JNIEXPORT void JNICALL fromjava(updateNote)(JNIEnv *env, jclass thiz, jint offset, jint time, jstring jtext) {
    if(!notes || offset < 0)
        return;
    jint rawlen = env->GetStringUTFLength(jtext);
    if(rawlen <= 0)
        return;
    uint16_t len = rawlen > notemaxlen ? notemaxlen : (uint16_t)rawlen;
    char tmpbuf[notemaxlen + 1];
    env->GetStringUTFRegion(jtext, 0, env->GetStringLength(jtext) < len ? env->GetStringLength(jtext) : len, tmpbuf);
    tmpbuf[len] = '\0';
    notes->updatenote(offset, time, tmpbuf, len);
}

extern "C" JNIEXPORT void JNICALL fromjava(purgeOldNotes)(JNIEnv *env, jclass thiz, jint cutoffTime) {
    if(notes)
        notes->purgenotes(cutoffTime);
}

extern "C" JNIEXPORT jint JNICALL fromjava(getNoteCount)(JNIEnv *env, jclass thiz) {
    if(!notes)
        return 0;
    return notes->count();
}

#endif
