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
/*      Fri Jan 27 12:36:44 CET 2023                                                 */



#include "nums/numdata.h"
#include "fromjava.h"
extern vector<Numdata*> numdatas;

extern "C" JNIEXPORT void JNICALL fromjava(closeNums)(JNIEnv *env, jclass thiz,jlong ptr) {
	Numdata *numdata=reinterpret_cast<Numdata *>(ptr);
	for (auto it = numdatas.begin(); it != numdatas.end(); ++it) {
		if(*it==numdata) {
			numdatas.erase(it);
			break;
			}
		}
	delete numdata;
	}
extern "C" JNIEXPORT jlong JNICALL fromjava(getident)(JNIEnv *env, jclass thiz,jlong ptr) {
	const Numdata *numdata=reinterpret_cast<const Numdata *>(ptr);
	return numdata->getnewident();
	}
extern "C" JNIEXPORT void JNICALL fromjava(setident)(JNIEnv *env, jclass thiz,jlong ptr,jlong ident) {
	Numdata *numdata=reinterpret_cast<Numdata *>(ptr);
	return numdata->setnewident(ident);
	}
extern "C" JNIEXPORT void JNICALL fromjava(saveNumpos)(JNIEnv *env, jclass thiz,jlong ptr,jint pos,jlong time,jfloat value,int type,int meal) {
	Numdata *numdata=reinterpret_cast<Numdata *>(ptr);
	numdata->numsavepos(pos,time,value,type,meal);

	}
extern "C" JNIEXPORT jint JNICALL fromjava(getfirstNum)(JNIEnv *env, jclass thiz,jlong ptr) {
	Numdata *numdata=reinterpret_cast<Numdata *>(ptr);
	return  numdata->getfirstpos();
	 }
extern "C" JNIEXPORT void JNICALL fromjava(setfirstNum)(JNIEnv *env, jclass thiz,jlong ptr,jint pos) {
	Numdata *numdata=reinterpret_cast<Numdata *>(ptr);
	 numdata->setfirstpos(pos);
	 }
extern "C" JNIEXPORT jint JNICALL fromjava(getlastNum)(JNIEnv *env, jclass thiz,jlong ptr) {
	Numdata *numdata=reinterpret_cast<Numdata *>(ptr);
	return  numdata->getlastpos();
	 }
extern "C" JNIEXPORT jint JNICALL fromjava(getlastpolledNum)(JNIEnv *env, jclass thiz,jlong ptr) {
	Numdata *numdata=reinterpret_cast<Numdata *>(ptr);
	return  numdata->getlastpolledpos();
	 }
extern "C" JNIEXPORT void JNICALL fromjava(setlastpolledNum)(JNIEnv *env, jclass thiz,jlong ptr,jint pos) {
	Numdata *numdata=reinterpret_cast<Numdata *>(ptr);
	 numdata->setlastpolledpos(pos);
	if(!numdata->getindex())
	 	numdata->setlastpos(pos);
	 }
extern "C" JNIEXPORT void JNICALL fromjava(setlastNum)(JNIEnv *env, jclass thiz,jlong ptr,jint pos) {
	Numdata *numdata=reinterpret_cast<Numdata *>(ptr);
	 numdata->setlastpos(pos);
	 }
extern "C" JNIEXPORT void JNICALL fromjava(updatedNum)(JNIEnv *env, jclass thiz,jlong ptr,jint pos) {
	Numdata *numdata=reinterpret_cast<Numdata *>(ptr);
	 numdata->updated(pos);
	 }
extern "C" JNIEXPORT void JNICALL fromjava(updatedNumstartend)(JNIEnv *env, jclass thiz,jlong ptr,jint start,jint end) {
	Numdata *numdata=reinterpret_cast<Numdata *>(ptr);
	LOGGER("updatedNumstartend %d %d\n",start,end);
	 numdata->updated(start,end);
	 }
extern "C" JNIEXPORT jint JNICALL fromjava(getchangedNum)(JNIEnv *env, jclass thiz,jlong ptr) {
	Numdata *numdata=reinterpret_cast<Numdata *>(ptr);
	 return numdata->getchangedpos();
	 }
extern "C" JNIEXPORT jint JNICALL fromjava(getonechangeNum)(JNIEnv *env, jclass thiz,jlong ptr) {
	Numdata *numdata=reinterpret_cast<Numdata *>(ptr);
	 return numdata->getonechange();
	 }

extern "C" JNIEXPORT void JNICALL fromjava(setchangedNum)(JNIEnv *env, jclass thiz,jlong ptr,jint pos) {
	Numdata *numdata=reinterpret_cast<Numdata *>(ptr);
	if(pos>numdata->getchangedpos()) {
		 numdata->getchangedpos()=pos;
		if(pos>=numdata->getlastpos())
			numdata->receivedbackup()=false;
		}
	numdata->getonechange()=0;

	 }
extern "C" JNIEXPORT void JNICALL fromjava(removeNum)(JNIEnv *env, jclass thiz,jlong ptr,jint pos) {
	Numdata *numdata=reinterpret_cast<Numdata *>(ptr);
	 numdata->numremove(pos) ;
	 }
extern "C" JNIEXPORT void JNICALL fromjava(saveNum)(JNIEnv *env, jclass thiz,jlong ptr,jlong time,jfloat value,jint type,jint mealptr) {
	Numdata *numdata=reinterpret_cast<Numdata *>(ptr);
	numdata->numsave(time,value,type,mealptr);

	}
extern "C" JNIEXPORT jobject JNICALL fromjava(getNumitem)(JNIEnv *env, jclass thiz,jlong ptr,jint pos) {
	Numdata *numdata=reinterpret_cast<Numdata *>(ptr);
	const Num &num=numdata->at(pos);
//	LOGGER("getNumitem(%p,%d)={%ld,%f,%d}\n",numdata,pos,num.time,num.value,num.type);
	static jclass  item=(jclass) env->NewGlobalRef(env->FindClass("tk/glucodata/nums/item"));
  	static jmethodID iconstruct = env->GetMethodID(item,"<init>","(JIFI)V");
	const jlong tim= num.time;
	return env->NewObject(item,iconstruct,tim,num.mealptr,num.value,num.type);
	}
extern "C" JNIEXPORT jint JNICALL fromjava(getNumindex)(JNIEnv *env, jclass thiz,jlong ptr) {
	Numdata *numdata=reinterpret_cast<Numdata *>(ptr);
	return numdata->getindex();
	}

extern "C" JNIEXPORT jboolean JNICALL fromjava(receivedbackup)(JNIEnv *env, jclass thiz,jlong ptr) {
	Numdata *numdata=reinterpret_cast<Numdata *>(ptr);
	if(numdata)
		return numdata->receivedbackup();
	return true;
	}
	/*
extern "C" JNIEXPORT void JNICALL fromjava(resendtowatch)(JNIEnv *env, jclass thiz) {
	for(Numdata *num:numdatas) 
		num->resendtowatch();
	}*/
