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
/*      Fri Jan 27 12:37:28 CET 2023                                                 */


#include <jni.h>
#include "fromjava.h"
#ifdef WEAROS
extern "C" JNIEXPORT jlong  JNICALL   fromjava(foodsearch)(JNIEnv *env, jclass cl, jstring jzoek) {
	return 0LL;
	}
extern "C" JNIEXPORT void  JNICALL   fromjava(freefoodptr)(JNIEnv *env, jclass cl, jlong ptr) {
	}
extern "C" JNIEXPORT jint  JNICALL   fromjava(foodhitnr)(JNIEnv *env, jclass cl, jlong ptr) {
	return 0;
	}
extern "C" JNIEXPORT jstring  JNICALL   fromjava(foodlabel)(JNIEnv *env, jclass cl, jlong ptr,jint pos) {
	return nullptr;
	}


extern "C" JNIEXPORT jint  JNICALL   fromjava(foodnr)(JNIEnv *env, jclass cl) {
	return 0;
	}



extern "C" JNIEXPORT jstring  JNICALL   fromjava(idfoodlabel)(JNIEnv *env, jclass cl, jint ind) {
	return nullptr;
	}
extern "C" JNIEXPORT jint  JNICALL   fromjava(getfoodid)(JNIEnv *env, jclass cl, jlong ptr,jint pos) {
	return -1;
	}
extern "C" JNIEXPORT jintArray  JNICALL   fromjava(getcomponents)(JNIEnv *env, jclass cl,jint ingr) {
	return nullptr;
	}

extern "C" JNIEXPORT jobjectArray  JNICALL   fromjava(getcomponentlabels)(JNIEnv *env, jclass cl) {
	return nullptr;
	}
extern "C" JNIEXPORT jobjectArray  JNICALL   fromjava(getcomponentunits)(JNIEnv *env, jclass cl) {
	return nullptr;
	}



#else
#include "destruct.hpp"
#include "database.hpp"

static Nutrients nutrients;
extern "C" JNIEXPORT jlong  JNICALL   fromjava(foodsearch)(JNIEnv *env, jclass cl, jstring jzoek) {
	const char *zoek = env->GetStringUTFChars( jzoek, NULL);
	if (zoek == nullptr) return 0LL; 
	destruct   dest([jzoek,zoek,env]() {env->ReleaseStringUTFChars(jzoek, zoek);});
	std::vector<uint32_t> *vectptr=new std::vector<uint32_t>;
	nutrients.matches(*vectptr,zoek);
	 return reinterpret_cast<jlong >(vectptr);	
	}
extern "C" JNIEXPORT void  JNICALL   fromjava(freefoodptr)(JNIEnv *env, jclass cl, jlong ptr) {
	std::vector<uint32_t> *vectptr=reinterpret_cast<std::vector<uint32_t>*>(ptr) ;
	delete vectptr;
	}
extern "C" JNIEXPORT jint  JNICALL   fromjava(foodhitnr)(JNIEnv *env, jclass cl, jlong ptr) {
	std::vector<uint32_t> *vectptr=reinterpret_cast<std::vector<uint32_t>*>(ptr) ;
	if(vectptr)
		return vectptr->size();
	return 0;
	}
/*
extern "C" JNIEXPORT jint  JNICALL   fromjava(foodhitel)(JNIEnv *env, jclass cl, jlong ptr,jint pos) {
	std::vector<uint32_t> *vectptr=reinterpret_cast<std::vector<uint32_t>*>(ptr) ;
	return (int)*vectptr[pos];
	} */
extern "C" JNIEXPORT jstring  JNICALL   fromjava(foodlabel)(JNIEnv *env, jclass cl, jlong ptr,jint pos) {
	std::vector<uint32_t> *vectptr=reinterpret_cast<std::vector<uint32_t>*>(ptr) ;
	
	uint32_t ind=(*vectptr)[pos];
//	char buf[50];
//	snprintf(buf,50,"food %d",ind);
//	return env->NewStringUTF(buf);
	return env->NewStringUTF(nutrients.foodlabel(ind));
	}

/*
extern "C" JNIEXPORT jstring  JNICALL   fromjava(getcomponents)(JNIEnv *env, jclass cl,jint ingr) {
	int len=nutrients.compnr():
	jdoubleArray ar=NewDoubleArray(len);
	for(int i=0;i<len;i++) {
		}*/

extern "C" JNIEXPORT jint  JNICALL   fromjava(foodnr)(JNIEnv *env, jclass cl) {
	return nutrients.foodnr();
	}



extern "C" JNIEXPORT jstring  JNICALL   fromjava(idfoodlabel)(JNIEnv *env, jclass cl, jint ind) {
	return env->NewStringUTF(nutrients.foodlabel(ind));
	}
extern "C" JNIEXPORT jint  JNICALL   fromjava(getfoodid)(JNIEnv *env, jclass cl, jlong ptr,jint pos) {
	std::vector<uint32_t> *vectptr=reinterpret_cast<std::vector<uint32_t>*>(ptr) ;
	uint32_t ind=(*vectptr)[pos];
	return ind;
	}
extern "C" JNIEXPORT jintArray  JNICALL   fromjava(getcomponents)(JNIEnv *env, jclass cl,jint ingr) {
	int len=nutrients.compnr();
	jintArray  uit=env->NewIntArray(len) ;
	env->SetIntArrayRegion(uit, 0,len, (const jint *)nutrients.getcomponents(ingr));
	return uit;
	}

extern "C" JNIEXPORT jobjectArray  JNICALL   fromjava(getcomponentlabels)(JNIEnv *env, jclass cl) {
	int len=nutrients.compnr();
	jobjectArray  namejar = env->NewObjectArray(len,env->FindClass("java/lang/String"),nullptr);
	 for(int i=0;i<len;i++) {
		 env->SetObjectArrayElement(namejar,i,env->NewStringUTF(nutrients.complabel(i)));
		  }
	return namejar;
	}
extern "C" JNIEXPORT jobjectArray  JNICALL   fromjava(getcomponentunits)(JNIEnv *env, jclass cl) {
	int len=nutrients.compnr();
	jobjectArray  namejar = env->NewObjectArray(len,env->FindClass("java/lang/String"),nullptr);
	 for(int i=0;i<len;i++) {
		 env->SetObjectArrayElement(namejar,i,env->NewStringUTF(nutrients.compunit(i)));
		  }
	return namejar;
	}



#endif
