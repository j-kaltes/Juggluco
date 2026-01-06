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
/*      Fri Jan 27 12:35:33 CET 2023                                                 */

#define SUBLOGAR LOGAR
#pragma GCC diagnostic ignored "-Wreturn-type"
    jint         subGetVersion(JNIEnv*env) {
   SUBLOGAR("GetVersion");

   }


    jclass       subDefineClass(JNIEnv*env,const char * const1,jobject  jobject2,const jbyte * const3,jsize  jsize4) {
   SUBLOGAR("DefineClass");

   }


    jmethodID    subFromReflectedMethod(JNIEnv*env,jobject  jobject1) {
   SUBLOGAR("FromReflectedMethod");

   }


    jfieldID     subFromReflectedField(JNIEnv*env,jobject  jobject1) {
   SUBLOGAR("FromReflectedField");

   }


    jobject      subToReflectedMethod(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,jboolean  jboolean3) {
   SUBLOGAR("ToReflectedMethod");

   }


    jclass       subGetSuperclass(JNIEnv*env,jclass  jclass1) {
   SUBLOGAR("GetSuperclass");

   }


    jboolean     subIsAssignableFrom(JNIEnv*env,jclass  jclass1,jclass  jclass2) {
   SUBLOGAR("IsAssignableFrom");

   }


    jobject      subToReflectedField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2,jboolean  jboolean3) {
   SUBLOGAR("ToReflectedField");

   }




    jint         subThrowNew(JNIEnv*env,jclass  jclass1,const char * const2) {
   SUBLOGAR("ThrowNew");

   }




    void         subExceptionDescribe(JNIEnv*env) {
   SUBLOGAR("ExceptionDescribe");

   }




    void         subFatalError(JNIEnv*env,const char * const1) {
   SUBLOGAR("FatalError");

   }


    jint         subPushLocalFrame(JNIEnv*env,jint  jint1) {
   SUBLOGAR("PushLocalFrame");

   }


    jobject      subPopLocalFrame(JNIEnv*env,jobject  jobject1) {
   SUBLOGAR("PopLocalFrame");

   }


    jobject      subNewGlobalRef(JNIEnv*env,jobject  jobject1) {
      LOGGER("NewGlobalRef %p\n",jobject1);
      return jobject1;
      }


    void         subDeleteGlobalRef(JNIEnv*env,jobject  jobject1) {
   SUBLOGAR("DeleteGlobalRef");

   }


    jboolean     subIsSameObject(JNIEnv*env,jobject  jobject1,jobject  jobject2) {
	jboolean res= jobject1==jobject2;	
   LOGGER("IsSameObject(%p,%p)=%d\n",jobject1,jobject2,res);
   return res;
   }


    jobject      subNewLocalRef(JNIEnv*env,jobject  jobject1) {
   SUBLOGAR("NewLocalRef");
	return jobject1;
   }


    jint         subEnsureLocalCapacity(JNIEnv*env,jint  jint1) {
   SUBLOGAR("EnsureLocalCapacity");

   }


    jobject      subAllocObject(JNIEnv*env,jclass  jclass1) {
   SUBLOGAR("AllocObject");

   }


    jobject      subNewObject(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2, ...) {
   SUBLOGAR("NewObject");

   }



    jobject      subNewObjectA(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,const jvalue * const3) {
   SUBLOGAR("NewObjectA");

   }


    jboolean     subIsInstanceOf(JNIEnv*env,jobject  jobject1,jclass  jclass2) {
   SUBLOGAR("IsInstanceOf");

   }




    jobject      subCallObjectMethod(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2, ...) {
   SUBLOGAR("CallObjectMethod");

   }


    jobject      subCallObjectMethodV(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,va_list  va_list3) {
   SUBLOGAR("CallObjectMethodV");

   }


    jobject      subCallObjectMethodA(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,const jvalue * const3) {
   SUBLOGAR("CallObjectMethodA");

   }


    jboolean     subCallBooleanMethodV(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,va_list  va_list3) {
   SUBLOGAR("CallBooleanMethodV");

   }


    jboolean     subCallBooleanMethodA(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,const jvalue * const3) {
   SUBLOGAR("CallBooleanMethodA");

   }


    jbyte        subCallByteMethod(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2, ...) {
   SUBLOGAR("CallByteMethod");

   }


    jbyte        subCallByteMethodV(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,va_list  va_list3) {
   SUBLOGAR("CallByteMethodV");

   }


    jbyte        subCallByteMethodA(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,const jvalue * const3) {
   SUBLOGAR("CallByteMethodA");

   }


    jchar        subCallCharMethod(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2, ...) {
   SUBLOGAR("CallCharMethod");

   }


    jchar        subCallCharMethodV(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,va_list  va_list3) {
   SUBLOGAR("CallCharMethodV");

   }


    jchar        subCallCharMethodA(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,const jvalue * const3) {
   SUBLOGAR("CallCharMethodA");

   }


    jshort       subCallShortMethod(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2, ...) {
   SUBLOGAR("CallShortMethod");

   }


    jshort       subCallShortMethodV(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,va_list  va_list3) {
   SUBLOGAR("CallShortMethodV");

   }


    jshort       subCallShortMethodA(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,const jvalue * const3) {
   SUBLOGAR("CallShortMethodA");

   }




    jint         subCallIntMethodV(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,va_list  va_list3) {
   SUBLOGAR("CallIntMethodV");

   }


    jint         subCallIntMethodA(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,const jvalue * const3) {
   SUBLOGAR("CallIntMethodA");

   }


    jlong        subCallLongMethod(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2, ...) {
   SUBLOGAR("CallLongMethod");

   }


    jlong        subCallLongMethodV(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,va_list  va_list3) {
   SUBLOGAR("CallLongMethodV");

   }


    jlong        subCallLongMethodA(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,const jvalue * const3) {
   SUBLOGAR("CallLongMethodA");

   }


    jfloat       subCallFloatMethod(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2, ...) {
   SUBLOGAR("CallFloatMethod");

   }


    jfloat       subCallFloatMethodV(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,va_list  va_list3) {
   SUBLOGAR("CallFloatMethodV");

   }


    jfloat       subCallFloatMethodA(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,const jvalue * const3) {
   SUBLOGAR("CallFloatMethodA");

   }




    jdouble      subCallDoubleMethodV(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,va_list  va_list3) {
   SUBLOGAR("CallDoubleMethodV");

   }


    jdouble      subCallDoubleMethodA(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,const jvalue * const3) {
   SUBLOGAR("CallDoubleMethodA");

   }




    void         subCallVoidMethodV(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,va_list  va_list3) {
   SUBLOGAR("CallVoidMethodV");

   }


    void         subCallVoidMethodA(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,const jvalue * const3) {
   SUBLOGAR("CallVoidMethodA");

   }


    jobject      subCallNonvirtualObjectMethod(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3, ...) {
   SUBLOGAR("CallNonvirtualObjectMethod");

   }


    jobject      subCallNonvirtualObjectMethodV(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,va_list  va_list4) {
   SUBLOGAR("CallNonvirtualObjectMethodV");

   }


    jobject      subCallNonvirtualObjectMethodA(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,const jvalue * const4) {
   SUBLOGAR("CallNonvirtualObjectMethodA");

   }


    jboolean     subCallNonvirtualBooleanMethod(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3, ...) {
   SUBLOGAR("CallNonvirtualBooleanMethod");

   }


    jboolean     subCallNonvirtualBooleanMethodV(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,va_list  va_list4) {
   SUBLOGAR("CallNonvirtualBooleanMethodV");

   }


    jboolean     subCallNonvirtualBooleanMethodA(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,const jvalue * const4) {
   SUBLOGAR("CallNonvirtualBooleanMethodA");

   }


    jbyte        subCallNonvirtualByteMethod(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3, ...) {
   SUBLOGAR("CallNonvirtualByteMethod");

   }


    jbyte        subCallNonvirtualByteMethodV(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,va_list  va_list4) {
   SUBLOGAR("CallNonvirtualByteMethodV");

   }


    jbyte        subCallNonvirtualByteMethodA(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,const jvalue * const4) {
   SUBLOGAR("CallNonvirtualByteMethodA");

   }


    jchar        subCallNonvirtualCharMethod(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3, ...) {
   SUBLOGAR("CallNonvirtualCharMethod");

   }


    jchar        subCallNonvirtualCharMethodV(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,va_list  va_list4) {
   SUBLOGAR("CallNonvirtualCharMethodV");

   }


    jchar        subCallNonvirtualCharMethodA(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,const jvalue * const4) {
   SUBLOGAR("CallNonvirtualCharMethodA");

   }


    jshort       subCallNonvirtualShortMethod(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3, ...) {
   SUBLOGAR("CallNonvirtualShortMethod");

   }


    jshort       subCallNonvirtualShortMethodV(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,va_list  va_list4) {
   SUBLOGAR("CallNonvirtualShortMethodV");

   }


    jshort       subCallNonvirtualShortMethodA(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,const jvalue * const4) {
   SUBLOGAR("CallNonvirtualShortMethodA");

   }


    jint         subCallNonvirtualIntMethod(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3, ...) {
   SUBLOGAR("CallNonvirtualIntMethod");

   }


    jint         subCallNonvirtualIntMethodV(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,va_list  va_list4) {
   SUBLOGAR("CallNonvirtualIntMethodV");

   }


    jint         subCallNonvirtualIntMethodA(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,const jvalue * const4) {
   SUBLOGAR("CallNonvirtualIntMethodA");

   }


    jlong        subCallNonvirtualLongMethod(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3, ...) {
   SUBLOGAR("CallNonvirtualLongMethod");

   }


    jlong        subCallNonvirtualLongMethodV(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,va_list  va_list4) {
   SUBLOGAR("CallNonvirtualLongMethodV");

   }


    jlong        subCallNonvirtualLongMethodA(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,const jvalue * const4) {
   SUBLOGAR("CallNonvirtualLongMethodA");

   }


    jfloat       subCallNonvirtualFloatMethod(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3, ...) {
   SUBLOGAR("CallNonvirtualFloatMethod");

   }


    jfloat       subCallNonvirtualFloatMethodV(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,va_list  va_list4) {
   SUBLOGAR("CallNonvirtualFloatMethodV");

   }


    jfloat       subCallNonvirtualFloatMethodA(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,const jvalue * const4) {
   SUBLOGAR("CallNonvirtualFloatMethodA");

   }


    jdouble      subCallNonvirtualDoubleMethod(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3, ...) {
   SUBLOGAR("CallNonvirtualDoubleMethod");

   }


    jdouble      subCallNonvirtualDoubleMethodV(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,va_list  va_list4) {
   SUBLOGAR("CallNonvirtualDoubleMethodV");

   }


    jdouble      subCallNonvirtualDoubleMethodA(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,const jvalue * const4) {
   SUBLOGAR("CallNonvirtualDoubleMethodA");

   }


    void         subCallNonvirtualVoidMethod(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3, ...) {
   SUBLOGAR("CallNonvirtualVoidMethod");

   }


    void         subCallNonvirtualVoidMethodV(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,va_list  va_list4) {
   SUBLOGAR("CallNonvirtualVoidMethodV");

   }


    void         subCallNonvirtualVoidMethodA(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,const jvalue * const4) {
   SUBLOGAR("CallNonvirtualVoidMethodA");

   }




    jobject      subGetObjectField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2) {
   SUBLOGAR("GetObjectField");

   }


    jboolean     subGetBooleanField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2) {
   SUBLOGAR("GetBooleanField");

   }


    jbyte        subGetByteField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2) {
   SUBLOGAR("GetByteField");

   }


    jchar        subGetCharField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2) {
   SUBLOGAR("GetCharField");

   }


    jshort       subGetShortField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2) {
   SUBLOGAR("GetShortField");

   }


    jint         subGetIntField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2) {
   SUBLOGAR("GetIntField");

   }




    jfloat       subGetFloatField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2) {
   SUBLOGAR("GetFloatField");

   }


    jdouble      subGetDoubleField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2) {
   SUBLOGAR("GetDoubleField");

   }


    void         subSetObjectField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2,jobject  jobject3) {
   SUBLOGAR("SetObjectField");

   }


    void         subSetBooleanField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2,jboolean  jboolean3) {
   SUBLOGAR("SetBooleanField");

   }


    void         subSetByteField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2,jbyte  jbyte3) {
   SUBLOGAR("SetByteField");

   }


    void         subSetCharField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2,jchar  jchar3) {
   SUBLOGAR("SetCharField");

   }


    void         subSetShortField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2,jshort  jshort3) {
   SUBLOGAR("SetShortField");

   }




    void         subSetFloatField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2,jfloat  jfloat3) {
   SUBLOGAR("SetFloatField");

   }






    jobject      subCallStaticObjectMethodV(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,va_list  va_list3) {
   SUBLOGAR("CallStaticObjectMethodV");

   }


    jobject      subCallStaticObjectMethodA(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,const jvalue * const3) {
   SUBLOGAR("CallStaticObjectMethodA");

   }


/*    jboolean     subCallStaticBooleanMethod(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2, ...) {
   SUBLOGAR("CallStaticBooleanMethod");

   }*/


    jboolean     subCallStaticBooleanMethodV(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,va_list  va_list3) {
   SUBLOGAR("CallStaticBooleanMethodV");

   }


    jboolean     subCallStaticBooleanMethodA(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,const jvalue * const3) {
   SUBLOGAR("CallStaticBooleanMethodA");

   }


    jbyte        subCallStaticByteMethod(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2, ...) {
   SUBLOGAR("CallStaticByteMethod");

   }


    jbyte        subCallStaticByteMethodV(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,va_list  va_list3) {
   SUBLOGAR("CallStaticByteMethodV");

   }


    jbyte        subCallStaticByteMethodA(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,const jvalue * const3) {
   SUBLOGAR("CallStaticByteMethodA");

   }


    jchar        subCallStaticCharMethod(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2, ...) {
   SUBLOGAR("CallStaticCharMethod");

   }


    jchar        subCallStaticCharMethodV(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,va_list  va_list3) {
   SUBLOGAR("CallStaticCharMethodV");

   }


    jchar        subCallStaticCharMethodA(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,const jvalue * const3) {
   SUBLOGAR("CallStaticCharMethodA");

   }


    jshort       subCallStaticShortMethod(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2, ...) {
   SUBLOGAR("CallStaticShortMethod");

   }


    jshort       subCallStaticShortMethodV(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,va_list  va_list3) {
   SUBLOGAR("CallStaticShortMethodV");

   }


    jshort       subCallStaticShortMethodA(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,const jvalue * const3) {
   SUBLOGAR("CallStaticShortMethodA");

   }


    jint         subCallStaticIntMethodV(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,va_list  va_list3) {
   SUBLOGAR("CallStaticIntMethodV");

   }


    jint         subCallStaticIntMethodA(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,const jvalue * const3) {
   SUBLOGAR("CallStaticIntMethodA");

   }


    jlong        subCallStaticLongMethod(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2, ...) {
   SUBLOGAR("CallStaticLongMethod");

   }


    jlong        subCallStaticLongMethodV(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,va_list  va_list3) {
   SUBLOGAR("CallStaticLongMethodV");

   }


    jlong        subCallStaticLongMethodA(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,const jvalue * const3) {
   SUBLOGAR("CallStaticLongMethodA");

   }


    jfloat       subCallStaticFloatMethod(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2, ...) {
   SUBLOGAR("CallStaticFloatMethod");

   }


    jfloat       subCallStaticFloatMethodV(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,va_list  va_list3) {
   SUBLOGAR("CallStaticFloatMethodV");

   }


    jfloat       subCallStaticFloatMethodA(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,const jvalue * const3) {
   SUBLOGAR("CallStaticFloatMethodA");

   }


    jdouble      subCallStaticDoubleMethod(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2, ...) {
   SUBLOGAR("CallStaticDoubleMethod");

   }


    jdouble      subCallStaticDoubleMethodV(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,va_list  va_list3) {
   SUBLOGAR("CallStaticDoubleMethodV");

   }


    jdouble      subCallStaticDoubleMethodA(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,const jvalue * const3) {
   SUBLOGAR("CallStaticDoubleMethodA");

   }


    void         subCallStaticVoidMethod(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2, ...) {
   SUBLOGAR("CallStaticVoidMethod");

   }


    void         subCallStaticVoidMethodV(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,va_list  va_list3) {
   SUBLOGAR("CallStaticVoidMethodV");

   }


    void         subCallStaticVoidMethodA(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,const jvalue * const3) {
   SUBLOGAR("CallStaticVoidMethodA");

   }


    jfieldID     subGetStaticFieldID(JNIEnv*env,jclass  jclass1,const char * const2,const char * const3) {
   SUBLOGAR("GetStaticFieldID");

   }


    jobject      subGetStaticObjectField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2) {
   SUBLOGAR("GetStaticObjectField");

   }


    jboolean     subGetStaticBooleanField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2) {
   SUBLOGAR("GetStaticBooleanField");

   }


    jbyte        subGetStaticByteField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2) {
   SUBLOGAR("GetStaticByteField");

   }


    jchar        subGetStaticCharField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2) {
   SUBLOGAR("GetStaticCharField");

   }


    jshort       subGetStaticShortField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2) {
   SUBLOGAR("GetStaticShortField");

   }


    jint         subGetStaticIntField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2) {
   SUBLOGAR("GetStaticIntField");

   }


    jlong        subGetStaticLongField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2) {
   SUBLOGAR("GetStaticLongField");

   }


    jfloat       subGetStaticFloatField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2) {
   SUBLOGAR("GetStaticFloatField");

   }


    jdouble      subGetStaticDoubleField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2) {
   SUBLOGAR("GetStaticDoubleField");

   }


    void         subSetStaticObjectField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2,jobject  jobject3) {
   SUBLOGAR("SetStaticObjectField");

   }


    void         subSetStaticBooleanField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2,jboolean  jboolean3) {
   SUBLOGAR("SetStaticBooleanField");

   }


    void         subSetStaticByteField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2,jbyte  jbyte3) {
   SUBLOGAR("SetStaticByteField");

   }


    void         subSetStaticCharField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2,jchar  jchar3) {
   SUBLOGAR("SetStaticCharField");

   }


    void         subSetStaticShortField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2,jshort  jshort3) {
   SUBLOGAR("SetStaticShortField");

   }


    void         subSetStaticIntField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2,jint  jint3) {
   SUBLOGAR("SetStaticIntField");

   }


    void         subSetStaticLongField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2,jlong  jlong3) {
   SUBLOGAR("SetStaticLongField");

   }


    void         subSetStaticFloatField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2,jfloat  jfloat3) {
   SUBLOGAR("SetStaticFloatField");

   }


    void         subSetStaticDoubleField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2,jdouble  jdouble3) {
   SUBLOGAR("SetStaticDoubleField");

   }


    jstring      subNewString(JNIEnv*env,const jchar * const1,jsize  jsize2) {
   SUBLOGAR("NewString");
   }


    jsize        subGetStringLength(JNIEnv*env,jstring  jstring1) {
   SUBLOGAR("GetStringLength");

   }


    const jchar*  subGetStringChars(JNIEnv*env,jstring  jstring1,jboolean * jboolean2) {
   SUBLOGAR("GetStringChars");

   }


    void         subReleaseStringChars(JNIEnv*env,jstring  jstring1,const jchar * const2) {
   SUBLOGAR("ReleaseStringChars");

   }


    jstring      subNewStringUTF(JNIEnv*env,const char * const1) {
   LOGGER("NewStringUTF(%s)\n",const1);
   return (jstring)const1;
   }


    jsize        subGetStringUTFLength(JNIEnv*env,jstring  jstring1) {
   SUBLOGAR("GetStringUTFLength");
   const char *str=(const char *)jstring1;
   return strlen(str);

   }







    jbooleanArray  subNewBooleanArray(JNIEnv*env,jsize  jsize1) {
   SUBLOGAR("NewBooleanArray");

   }


    jcharArray     subNewCharArray(JNIEnv*env,jsize  jsize1) {
   SUBLOGAR("NewCharArray");

   }


    jshortArray    subNewShortArray(JNIEnv*env,jsize  jsize1) {
   SUBLOGAR("NewShortArray");

   }


    jintArray      subNewIntArray(JNIEnv*env,jsize  jsize1) {
   SUBLOGAR("NewIntArray");

   }


    jlongArray     subNewLongArray(JNIEnv*env,jsize  jsize1) {
   SUBLOGAR("NewLongArray");

   }


    jfloatArray    subNewFloatArray(JNIEnv*env,jsize  jsize1) {
   SUBLOGAR("NewFloatArray");

   }


    jdoubleArray   subNewDoubleArray(JNIEnv*env,jsize  jsize1) {
   SUBLOGAR("NewDoubleArray");

   }


    jboolean*    subGetBooleanArrayElements(JNIEnv*env,jbooleanArray  jbooleanArray1,jboolean * jboolean2) {
   SUBLOGAR("GetBooleanArrayElements");

   }


    jchar*       subGetCharArrayElements(JNIEnv*env,jcharArray  jcharArray1,jboolean * jboolean2) {
   SUBLOGAR("GetCharArrayElements");

   }


    jshort*      subGetShortArrayElements(JNIEnv*env,jshortArray  jshortArray1,jboolean * jboolean2) {
   SUBLOGAR("GetShortArrayElements");

   }

/*
    jint*        subGetIntArrayElements(JNIEnv*env,jintArray  jintArray1,jboolean * jboolean2) {
   SUBLOGAR("GetIntArrayElements");

   } */


    jlong*       subGetLongArrayElements(JNIEnv*env,jlongArray  jlongArray1,jboolean * jboolean2) {
   SUBLOGAR("GetLongArrayElements");

   }


    jfloat*      subGetFloatArrayElements(JNIEnv*env,jfloatArray  jfloatArray1,jboolean * jboolean2) {
   SUBLOGAR("GetFloatArrayElements");

   }


    jdouble*     subGetDoubleArrayElements(JNIEnv*env,jdoubleArray  jdoubleArray1,jboolean * jboolean2) {
   SUBLOGAR("GetDoubleArrayElements");

   }


    void         subReleaseBooleanArrayElements(JNIEnv*env,jbooleanArray  jbooleanArray1,jboolean * jboolean2,jint  jint3) {
   SUBLOGAR("ReleaseBooleanArrayElements");

   }


    void         subReleaseCharArrayElements(JNIEnv*env,jcharArray  jcharArray1,jchar * jchar2,jint  jint3) {
   SUBLOGAR("ReleaseCharArrayElements");

   }


    void         subReleaseShortArrayElements(JNIEnv*env,jshortArray  jshortArray1,jshort * jshort2,jint  jint3) {
   SUBLOGAR("ReleaseShortArrayElements");

   }


/*    void         subReleaseIntArrayElements(JNIEnv*env,jintArray  jintArray1,jint * jint2,jint  jint3) {
   SUBLOGAR("ReleaseIntArrayElements");

   } */


    void         subReleaseLongArrayElements(JNIEnv*env,jlongArray  jlongArray1,jlong * jlong2,jint  jint3) {
   SUBLOGAR("ReleaseLongArrayElements");

   }


    void         subReleaseFloatArrayElements(JNIEnv*env,jfloatArray  jfloatArray1,jfloat * jfloat2,jint  jint3) {
   SUBLOGAR("ReleaseFloatArrayElements");

   }


    void         subReleaseDoubleArrayElements(JNIEnv*env,jdoubleArray  jdoubleArray1,jdouble * jdouble2,jint  jint3) {
   SUBLOGAR("ReleaseDoubleArrayElements");

   }


    void         subGetBooleanArrayRegion(JNIEnv*env,jbooleanArray  jbooleanArray1,jsize  jsize2,jsize  jsize3,jboolean * jboolean4) {
   SUBLOGAR("GetBooleanArrayRegion");

   }




    void         subGetCharArrayRegion(JNIEnv*env,jcharArray  jcharArray1,jsize  jsize2,jsize  jsize3,jchar * jchar4) {
   SUBLOGAR("GetCharArrayRegion");

   }


    void         subGetShortArrayRegion(JNIEnv*env,jshortArray  jshortArray1,jsize  jsize2,jsize  jsize3,jshort * jshort4) {
   SUBLOGAR("GetShortArrayRegion");

   }


    void         subGetIntArrayRegion(JNIEnv*env,jintArray  jintArray1,jsize  jsize2,jsize  jsize3,jint * jint4) {
   SUBLOGAR("GetIntArrayRegion");

   }


    void         subGetLongArrayRegion(JNIEnv*env,jlongArray  jlongArray1,jsize  jsize2,jsize  jsize3,jlong * jlong4) {
   SUBLOGAR("GetLongArrayRegion");

   }


    void         subGetFloatArrayRegion(JNIEnv*env,jfloatArray  jfloatArray1,jsize  jsize2,jsize  jsize3,jfloat * jfloat4) {
   SUBLOGAR("GetFloatArrayRegion");

   }


    void         subGetDoubleArrayRegion(JNIEnv*env,jdoubleArray  jdoubleArray1,jsize  jsize2,jsize  jsize3,jdouble * jdouble4) {
   SUBLOGAR("GetDoubleArrayRegion");

   }


    void         subSetBooleanArrayRegion(JNIEnv*env,jbooleanArray  jbooleanArray1,jsize  jsize2,jsize  jsize3,const jboolean * const4) {
   SUBLOGAR("SetBooleanArrayRegion");

   }




    void         subSetCharArrayRegion(JNIEnv*env,jcharArray  jcharArray1,jsize  jsize2,jsize  jsize3,const jchar * const4) {
   SUBLOGAR("SetCharArrayRegion");

   }


    void         subSetShortArrayRegion(JNIEnv*env,jshortArray  jshortArray1,jsize  jsize2,jsize  jsize3,const jshort * const4) {
   SUBLOGAR("SetShortArrayRegion");

   }


    void         subSetIntArrayRegion(JNIEnv*env,jintArray  jintArray1,jsize  jsize2,jsize  jsize3,const jint * const4) {
   SUBLOGAR("SetIntArrayRegion");

   }


    void         subSetLongArrayRegion(JNIEnv*env,jlongArray  jlongArray1,jsize  jsize2,jsize  jsize3,const jlong * const4) {
   SUBLOGAR("SetLongArrayRegion");

   }


    void         subSetFloatArrayRegion(JNIEnv*env,jfloatArray  jfloatArray1,jsize  jsize2,jsize  jsize3,const jfloat * const4) {
   SUBLOGAR("SetFloatArrayRegion");

   }


    void         subSetDoubleArrayRegion(JNIEnv*env,jdoubleArray  jdoubleArray1,jsize  jsize2,jsize  jsize3,const jdouble * const4) {
   SUBLOGAR("SetDoubleArrayRegion");

   }



    jint         subUnregisterNatives(JNIEnv*env,jclass  jclass1) {
   SUBLOGAR("UnregisterNatives");

   }


    jint         subMonitorEnter(JNIEnv*env,jobject  jobject1) {
   SUBLOGAR("MonitorEnter");

   }


    jint         subMonitorExit(JNIEnv*env,jobject  jobject1) {
   SUBLOGAR("MonitorExit");

   }


    jint         subGetJavaVM(JNIEnv*env,JavaVM ** JavaVM1) {
   SUBLOGAR("GetJavaVM");

   }


    void         subGetStringRegion(JNIEnv*env,jstring  jstring1,jsize  jsize2,jsize  jsize3,jchar * jchar4) {
   SUBLOGAR("GetStringRegion");

   }


    void         subGetStringUTFRegion(JNIEnv*env,jstring  jstring1,jsize  jsize2,jsize  jsize3,char * char4) {
   SUBLOGAR("GetStringUTFRegion");

   }


    void*        subGetPrimitiveArrayCritical(JNIEnv*env,jarray  jarray1,jboolean * jboolean2) {
   SUBLOGAR("GetPrimitiveArrayCritical");

   }


    void         subReleasePrimitiveArrayCritical(JNIEnv*env,jarray  jarray1,void * void2,jint  jint3) {
   SUBLOGAR("ReleasePrimitiveArrayCritical");

   }


    const jchar*  subGetStringCritical(JNIEnv*env,jstring  jstring1,jboolean * jboolean2) {
   SUBLOGAR("GetStringCritical");

   }


    void         subReleaseStringCritical(JNIEnv*env,jstring  jstring1,const jchar * const2) {
   SUBLOGAR("ReleaseStringCritical");

   }


    jweak        subNewWeakGlobalRef(JNIEnv*env,jobject  jobject1) {
   SUBLOGAR("NewWeakGlobalRef");

   }


    void         subDeleteWeakGlobalRef(JNIEnv*env,jweak  jweak1) {
   SUBLOGAR("DeleteWeakGlobalRef");

   }




    jobject      subNewDirectByteBuffer(JNIEnv*env,void * void1,jlong  jlong2) {
   SUBLOGAR("NewDirectByteBuffer");

   }


    void*        subGetDirectBufferAddress(JNIEnv*env,jobject  jobject1) {
   SUBLOGAR("GetDirectBufferAddress");

   }


    jlong        subGetDirectBufferCapacity(JNIEnv*env,jobject  jobject1) {
   SUBLOGAR("GetDirectBufferCapacity");

   }


    jobjectRefType  subGetObjectRefType(JNIEnv*env,jobject  jobject1) {
   SUBLOGAR("GetObjectRefType");

   }

