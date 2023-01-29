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


#pragma GCC diagnostic ignored "-Wreturn-type"
    jint         subGetVersion(JNIEnv*env) {
   LOGGER("GetVersion\n");

   }


    jclass       subDefineClass(JNIEnv*env,const char * const1,jobject  jobject2,const jbyte * const3,jsize  jsize4) {
   LOGGER("DefineClass\n");

   }


    jmethodID    subFromReflectedMethod(JNIEnv*env,jobject  jobject1) {
   LOGGER("FromReflectedMethod\n");

   }


    jfieldID     subFromReflectedField(JNIEnv*env,jobject  jobject1) {
   LOGGER("FromReflectedField\n");

   }


    jobject      subToReflectedMethod(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,jboolean  jboolean3) {
   LOGGER("ToReflectedMethod\n");

   }


    jclass       subGetSuperclass(JNIEnv*env,jclass  jclass1) {
   LOGGER("GetSuperclass\n");

   }


    jboolean     subIsAssignableFrom(JNIEnv*env,jclass  jclass1,jclass  jclass2) {
   LOGGER("IsAssignableFrom\n");

   }


    jobject      subToReflectedField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2,jboolean  jboolean3) {
   LOGGER("ToReflectedField\n");

   }




    jint         subThrowNew(JNIEnv*env,jclass  jclass1,const char * const2) {
   LOGGER("ThrowNew\n");

   }




    void         subExceptionDescribe(JNIEnv*env) {
   LOGGER("ExceptionDescribe\n");

   }




    void         subFatalError(JNIEnv*env,const char * const1) {
   LOGGER("FatalError\n");

   }


    jint         subPushLocalFrame(JNIEnv*env,jint  jint1) {
   LOGGER("PushLocalFrame\n");

   }


    jobject      subPopLocalFrame(JNIEnv*env,jobject  jobject1) {
   LOGGER("PopLocalFrame\n");

   }


    jobject      subNewGlobalRef(JNIEnv*env,jobject  jobject1) {
   LOGGER("NewGlobalRef\n");

   }


    void         subDeleteGlobalRef(JNIEnv*env,jobject  jobject1) {
   LOGGER("DeleteGlobalRef\n");

   }


    jboolean     subIsSameObject(JNIEnv*env,jobject  jobject1,jobject  jobject2) {
	jboolean res= jobject1==jobject2;	
   LOGGER("IsSameObject(%p,%p)=%d\n",jobject1,jobject2,res);
   return res;
   }


    jobject      subNewLocalRef(JNIEnv*env,jobject  jobject1) {
   LOGGER("NewLocalRef\n");
	return jobject1;
   }


    jint         subEnsureLocalCapacity(JNIEnv*env,jint  jint1) {
   LOGGER("EnsureLocalCapacity\n");

   }


    jobject      subAllocObject(JNIEnv*env,jclass  jclass1) {
   LOGGER("AllocObject\n");

   }


    jobject      subNewObject(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2, ...) {
   LOGGER("NewObject\n");

   }


    jobject      subNewObjectV(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGGER("NewObjectV\n");

   }


    jobject      subNewObjectA(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGGER("NewObjectA\n");

   }


    jboolean     subIsInstanceOf(JNIEnv*env,jobject  jobject1,jclass  jclass2) {
   LOGGER("IsInstanceOf\n");

   }




    jobject      subCallObjectMethod(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2, ...) {
   LOGGER("CallObjectMethod\n");

   }


    jobject      subCallObjectMethodV(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGGER("CallObjectMethodV\n");

   }


    jobject      subCallObjectMethodA(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGGER("CallObjectMethodA\n");

   }


    jboolean     subCallBooleanMethodV(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGGER("CallBooleanMethodV\n");

   }


    jboolean     subCallBooleanMethodA(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGGER("CallBooleanMethodA\n");

   }


    jbyte        subCallByteMethod(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2, ...) {
   LOGGER("CallByteMethod\n");

   }


    jbyte        subCallByteMethodV(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGGER("CallByteMethodV\n");

   }


    jbyte        subCallByteMethodA(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGGER("CallByteMethodA\n");

   }


    jchar        subCallCharMethod(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2, ...) {
   LOGGER("CallCharMethod\n");

   }


    jchar        subCallCharMethodV(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGGER("CallCharMethodV\n");

   }


    jchar        subCallCharMethodA(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGGER("CallCharMethodA\n");

   }


    jshort       subCallShortMethod(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2, ...) {
   LOGGER("CallShortMethod\n");

   }


    jshort       subCallShortMethodV(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGGER("CallShortMethodV\n");

   }


    jshort       subCallShortMethodA(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGGER("CallShortMethodA\n");

   }




    jint         subCallIntMethodV(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGGER("CallIntMethodV\n");

   }


    jint         subCallIntMethodA(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGGER("CallIntMethodA\n");

   }


    jlong        subCallLongMethod(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2, ...) {
   LOGGER("CallLongMethod\n");

   }


    jlong        subCallLongMethodV(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGGER("CallLongMethodV\n");

   }


    jlong        subCallLongMethodA(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGGER("CallLongMethodA\n");

   }


    jfloat       subCallFloatMethod(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2, ...) {
   LOGGER("CallFloatMethod\n");

   }


    jfloat       subCallFloatMethodV(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGGER("CallFloatMethodV\n");

   }


    jfloat       subCallFloatMethodA(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGGER("CallFloatMethodA\n");

   }




    jdouble      subCallDoubleMethodV(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGGER("CallDoubleMethodV\n");

   }


    jdouble      subCallDoubleMethodA(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGGER("CallDoubleMethodA\n");

   }




    void         subCallVoidMethodV(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGGER("CallVoidMethodV\n");

   }


    void         subCallVoidMethodA(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGGER("CallVoidMethodA\n");

   }


    jobject      subCallNonvirtualObjectMethod(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3, ...) {
   LOGGER("CallNonvirtualObjectMethod\n");

   }


    jobject      subCallNonvirtualObjectMethodV(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,va_list  va_list4) {
   LOGGER("CallNonvirtualObjectMethodV\n");

   }


    jobject      subCallNonvirtualObjectMethodA(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,const jvalue * const4) {
   LOGGER("CallNonvirtualObjectMethodA\n");

   }


    jboolean     subCallNonvirtualBooleanMethod(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3, ...) {
   LOGGER("CallNonvirtualBooleanMethod\n");

   }


    jboolean     subCallNonvirtualBooleanMethodV(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,va_list  va_list4) {
   LOGGER("CallNonvirtualBooleanMethodV\n");

   }


    jboolean     subCallNonvirtualBooleanMethodA(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,const jvalue * const4) {
   LOGGER("CallNonvirtualBooleanMethodA\n");

   }


    jbyte        subCallNonvirtualByteMethod(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3, ...) {
   LOGGER("CallNonvirtualByteMethod\n");

   }


    jbyte        subCallNonvirtualByteMethodV(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,va_list  va_list4) {
   LOGGER("CallNonvirtualByteMethodV\n");

   }


    jbyte        subCallNonvirtualByteMethodA(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,const jvalue * const4) {
   LOGGER("CallNonvirtualByteMethodA\n");

   }


    jchar        subCallNonvirtualCharMethod(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3, ...) {
   LOGGER("CallNonvirtualCharMethod\n");

   }


    jchar        subCallNonvirtualCharMethodV(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,va_list  va_list4) {
   LOGGER("CallNonvirtualCharMethodV\n");

   }


    jchar        subCallNonvirtualCharMethodA(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,const jvalue * const4) {
   LOGGER("CallNonvirtualCharMethodA\n");

   }


    jshort       subCallNonvirtualShortMethod(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3, ...) {
   LOGGER("CallNonvirtualShortMethod\n");

   }


    jshort       subCallNonvirtualShortMethodV(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,va_list  va_list4) {
   LOGGER("CallNonvirtualShortMethodV\n");

   }


    jshort       subCallNonvirtualShortMethodA(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,const jvalue * const4) {
   LOGGER("CallNonvirtualShortMethodA\n");

   }


    jint         subCallNonvirtualIntMethod(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3, ...) {
   LOGGER("CallNonvirtualIntMethod\n");

   }


    jint         subCallNonvirtualIntMethodV(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,va_list  va_list4) {
   LOGGER("CallNonvirtualIntMethodV\n");

   }


    jint         subCallNonvirtualIntMethodA(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,const jvalue * const4) {
   LOGGER("CallNonvirtualIntMethodA\n");

   }


    jlong        subCallNonvirtualLongMethod(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3, ...) {
   LOGGER("CallNonvirtualLongMethod\n");

   }


    jlong        subCallNonvirtualLongMethodV(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,va_list  va_list4) {
   LOGGER("CallNonvirtualLongMethodV\n");

   }


    jlong        subCallNonvirtualLongMethodA(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,const jvalue * const4) {
   LOGGER("CallNonvirtualLongMethodA\n");

   }


    jfloat       subCallNonvirtualFloatMethod(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3, ...) {
   LOGGER("CallNonvirtualFloatMethod\n");

   }


    jfloat       subCallNonvirtualFloatMethodV(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,va_list  va_list4) {
   LOGGER("CallNonvirtualFloatMethodV\n");

   }


    jfloat       subCallNonvirtualFloatMethodA(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,const jvalue * const4) {
   LOGGER("CallNonvirtualFloatMethodA\n");

   }


    jdouble      subCallNonvirtualDoubleMethod(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3, ...) {
   LOGGER("CallNonvirtualDoubleMethod\n");

   }


    jdouble      subCallNonvirtualDoubleMethodV(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,va_list  va_list4) {
   LOGGER("CallNonvirtualDoubleMethodV\n");

   }


    jdouble      subCallNonvirtualDoubleMethodA(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,const jvalue * const4) {
   LOGGER("CallNonvirtualDoubleMethodA\n");

   }


    void         subCallNonvirtualVoidMethod(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3, ...) {
   LOGGER("CallNonvirtualVoidMethod\n");

   }


    void         subCallNonvirtualVoidMethodV(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,va_list  va_list4) {
   LOGGER("CallNonvirtualVoidMethodV\n");

   }


    void         subCallNonvirtualVoidMethodA(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,const jvalue * const4) {
   LOGGER("CallNonvirtualVoidMethodA\n");

   }


    jfieldID     subGetFieldID(JNIEnv*env,jclass  jclass1,const char * const2,const char * const3) {
   LOGGER("GetFieldID\n");

   }


    jobject      subGetObjectField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2) {
   LOGGER("GetObjectField\n");

   }


    jboolean     subGetBooleanField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2) {
   LOGGER("GetBooleanField\n");

   }


    jbyte        subGetByteField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2) {
   LOGGER("GetByteField\n");

   }


    jchar        subGetCharField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2) {
   LOGGER("GetCharField\n");

   }


    jshort       subGetShortField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2) {
   LOGGER("GetShortField\n");

   }


    jint         subGetIntField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2) {
   LOGGER("GetIntField\n");

   }


    jlong        subGetLongField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2) {
   LOGGER("GetLongField\n");

   }


    jfloat       subGetFloatField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2) {
   LOGGER("GetFloatField\n");

   }


    jdouble      subGetDoubleField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2) {
   LOGGER("GetDoubleField\n");

   }


    void         subSetObjectField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2,jobject  jobject3) {
   LOGGER("SetObjectField\n");

   }


    void         subSetBooleanField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2,jboolean  jboolean3) {
   LOGGER("SetBooleanField\n");

   }


    void         subSetByteField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2,jbyte  jbyte3) {
   LOGGER("SetByteField\n");

   }


    void         subSetCharField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2,jchar  jchar3) {
   LOGGER("SetCharField\n");

   }


    void         subSetShortField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2,jshort  jshort3) {
   LOGGER("SetShortField\n");

   }


    void         subSetIntField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2,jint  jint3) {
   LOGGER("SetIntField\n");

   }


    void         subSetLongField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2,jlong  jlong3) {
   LOGGER("SetLongField\n");

   }


    void         subSetFloatField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2,jfloat  jfloat3) {
   LOGGER("SetFloatField\n");

   }


    void         subSetDoubleField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2,jdouble  jdouble3) {
   LOGGER("SetDoubleField\n");

   }




    jobject      subCallStaticObjectMethodV(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGGER("CallStaticObjectMethodV\n");

   }


    jobject      subCallStaticObjectMethodA(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGGER("CallStaticObjectMethodA\n");

   }


/*    jboolean     subCallStaticBooleanMethod(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2, ...) {
   LOGGER("CallStaticBooleanMethod\n");

   }*/


    jboolean     subCallStaticBooleanMethodV(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGGER("CallStaticBooleanMethodV\n");

   }


    jboolean     subCallStaticBooleanMethodA(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGGER("CallStaticBooleanMethodA\n");

   }


    jbyte        subCallStaticByteMethod(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2, ...) {
   LOGGER("CallStaticByteMethod\n");

   }


    jbyte        subCallStaticByteMethodV(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGGER("CallStaticByteMethodV\n");

   }


    jbyte        subCallStaticByteMethodA(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGGER("CallStaticByteMethodA\n");

   }


    jchar        subCallStaticCharMethod(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2, ...) {
   LOGGER("CallStaticCharMethod\n");

   }


    jchar        subCallStaticCharMethodV(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGGER("CallStaticCharMethodV\n");

   }


    jchar        subCallStaticCharMethodA(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGGER("CallStaticCharMethodA\n");

   }


    jshort       subCallStaticShortMethod(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2, ...) {
   LOGGER("CallStaticShortMethod\n");

   }


    jshort       subCallStaticShortMethodV(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGGER("CallStaticShortMethodV\n");

   }


    jshort       subCallStaticShortMethodA(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGGER("CallStaticShortMethodA\n");

   }


    jint         subCallStaticIntMethodV(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGGER("CallStaticIntMethodV\n");

   }


    jint         subCallStaticIntMethodA(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGGER("CallStaticIntMethodA\n");

   }


    jlong        subCallStaticLongMethod(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2, ...) {
   LOGGER("CallStaticLongMethod\n");

   }


    jlong        subCallStaticLongMethodV(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGGER("CallStaticLongMethodV\n");

   }


    jlong        subCallStaticLongMethodA(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGGER("CallStaticLongMethodA\n");

   }


    jfloat       subCallStaticFloatMethod(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2, ...) {
   LOGGER("CallStaticFloatMethod\n");

   }


    jfloat       subCallStaticFloatMethodV(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGGER("CallStaticFloatMethodV\n");

   }


    jfloat       subCallStaticFloatMethodA(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGGER("CallStaticFloatMethodA\n");

   }


    jdouble      subCallStaticDoubleMethod(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2, ...) {
   LOGGER("CallStaticDoubleMethod\n");

   }


    jdouble      subCallStaticDoubleMethodV(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGGER("CallStaticDoubleMethodV\n");

   }


    jdouble      subCallStaticDoubleMethodA(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGGER("CallStaticDoubleMethodA\n");

   }


    void         subCallStaticVoidMethod(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2, ...) {
   LOGGER("CallStaticVoidMethod\n");

   }


    void         subCallStaticVoidMethodV(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGGER("CallStaticVoidMethodV\n");

   }


    void         subCallStaticVoidMethodA(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGGER("CallStaticVoidMethodA\n");

   }


    jfieldID     subGetStaticFieldID(JNIEnv*env,jclass  jclass1,const char * const2,const char * const3) {
   LOGGER("GetStaticFieldID\n");

   }


    jobject      subGetStaticObjectField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2) {
   LOGGER("GetStaticObjectField\n");

   }


    jboolean     subGetStaticBooleanField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2) {
   LOGGER("GetStaticBooleanField\n");

   }


    jbyte        subGetStaticByteField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2) {
   LOGGER("GetStaticByteField\n");

   }


    jchar        subGetStaticCharField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2) {
   LOGGER("GetStaticCharField\n");

   }


    jshort       subGetStaticShortField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2) {
   LOGGER("GetStaticShortField\n");

   }


    jint         subGetStaticIntField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2) {
   LOGGER("GetStaticIntField\n");

   }


    jlong        subGetStaticLongField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2) {
   LOGGER("GetStaticLongField\n");

   }


    jfloat       subGetStaticFloatField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2) {
   LOGGER("GetStaticFloatField\n");

   }


    jdouble      subGetStaticDoubleField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2) {
   LOGGER("GetStaticDoubleField\n");

   }


    void         subSetStaticObjectField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2,jobject  jobject3) {
   LOGGER("SetStaticObjectField\n");

   }


    void         subSetStaticBooleanField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2,jboolean  jboolean3) {
   LOGGER("SetStaticBooleanField\n");

   }


    void         subSetStaticByteField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2,jbyte  jbyte3) {
   LOGGER("SetStaticByteField\n");

   }


    void         subSetStaticCharField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2,jchar  jchar3) {
   LOGGER("SetStaticCharField\n");

   }


    void         subSetStaticShortField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2,jshort  jshort3) {
   LOGGER("SetStaticShortField\n");

   }


    void         subSetStaticIntField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2,jint  jint3) {
   LOGGER("SetStaticIntField\n");

   }


    void         subSetStaticLongField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2,jlong  jlong3) {
   LOGGER("SetStaticLongField\n");

   }


    void         subSetStaticFloatField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2,jfloat  jfloat3) {
   LOGGER("SetStaticFloatField\n");

   }


    void         subSetStaticDoubleField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2,jdouble  jdouble3) {
   LOGGER("SetStaticDoubleField\n");

   }


    jstring      subNewString(JNIEnv*env,const jchar * const1,jsize  jsize2) {
   LOGGER("NewString\n");

   }


    jsize        subGetStringLength(JNIEnv*env,jstring  jstring1) {
   LOGGER("GetStringLength\n");

   }


    const jchar*  subGetStringChars(JNIEnv*env,jstring  jstring1,jboolean * jboolean2) {
   LOGGER("GetStringChars\n");

   }


    void         subReleaseStringChars(JNIEnv*env,jstring  jstring1,const jchar * const2) {
   LOGGER("ReleaseStringChars\n");

   }


    jstring      subNewStringUTF(JNIEnv*env,const char * const1) {
   LOGGER("NewStringUTF\n");

   }


    jsize        subGetStringUTFLength(JNIEnv*env,jstring  jstring1) {
   LOGGER("GetStringUTFLength\n");

   }


    const char*  subGetStringUTFChars(JNIEnv*env,jstring  jstring1,jboolean * jboolean2) {
   LOGGER("GetStringUTFChars\n");

   }


    void         subReleaseStringUTFChars(JNIEnv*env,jstring  jstring1,const char * const2) {
   LOGGER("ReleaseStringUTFChars\n");

   }


    jobjectArray  subNewObjectArray(JNIEnv*env,jsize  jsize1,jclass  jclass2,jobject  jobject3) {
   LOGGER("NewObjectArray\n");

   }


    jobject      subGetObjectArrayElement(JNIEnv*env,jobjectArray  jobjectArray1,jsize  jsize2) {
   LOGGER("GetObjectArrayElement\n");

   }


    void         subSetObjectArrayElement(JNIEnv*env,jobjectArray  jobjectArray1,jsize  jsize2,jobject  jobject3) {
   LOGGER("SetObjectArrayElement\n");

   }


    jbooleanArray  subNewBooleanArray(JNIEnv*env,jsize  jsize1) {
   LOGGER("NewBooleanArray\n");

   }


    jcharArray     subNewCharArray(JNIEnv*env,jsize  jsize1) {
   LOGGER("NewCharArray\n");

   }


    jshortArray    subNewShortArray(JNIEnv*env,jsize  jsize1) {
   LOGGER("NewShortArray\n");

   }


    jintArray      subNewIntArray(JNIEnv*env,jsize  jsize1) {
   LOGGER("NewIntArray\n");

   }


    jlongArray     subNewLongArray(JNIEnv*env,jsize  jsize1) {
   LOGGER("NewLongArray\n");

   }


    jfloatArray    subNewFloatArray(JNIEnv*env,jsize  jsize1) {
   LOGGER("NewFloatArray\n");

   }


    jdoubleArray   subNewDoubleArray(JNIEnv*env,jsize  jsize1) {
   LOGGER("NewDoubleArray\n");

   }


    jboolean*    subGetBooleanArrayElements(JNIEnv*env,jbooleanArray  jbooleanArray1,jboolean * jboolean2) {
   LOGGER("GetBooleanArrayElements\n");

   }


    jchar*       subGetCharArrayElements(JNIEnv*env,jcharArray  jcharArray1,jboolean * jboolean2) {
   LOGGER("GetCharArrayElements\n");

   }


    jshort*      subGetShortArrayElements(JNIEnv*env,jshortArray  jshortArray1,jboolean * jboolean2) {
   LOGGER("GetShortArrayElements\n");

   }


    jint*        subGetIntArrayElements(JNIEnv*env,jintArray  jintArray1,jboolean * jboolean2) {
   LOGGER("GetIntArrayElements\n");

   }


    jlong*       subGetLongArrayElements(JNIEnv*env,jlongArray  jlongArray1,jboolean * jboolean2) {
   LOGGER("GetLongArrayElements\n");

   }


    jfloat*      subGetFloatArrayElements(JNIEnv*env,jfloatArray  jfloatArray1,jboolean * jboolean2) {
   LOGGER("GetFloatArrayElements\n");

   }


    jdouble*     subGetDoubleArrayElements(JNIEnv*env,jdoubleArray  jdoubleArray1,jboolean * jboolean2) {
   LOGGER("GetDoubleArrayElements\n");

   }


    void         subReleaseBooleanArrayElements(JNIEnv*env,jbooleanArray  jbooleanArray1,jboolean * jboolean2,jint  jint3) {
   LOGGER("ReleaseBooleanArrayElements\n");

   }


    void         subReleaseCharArrayElements(JNIEnv*env,jcharArray  jcharArray1,jchar * jchar2,jint  jint3) {
   LOGGER("ReleaseCharArrayElements\n");

   }


    void         subReleaseShortArrayElements(JNIEnv*env,jshortArray  jshortArray1,jshort * jshort2,jint  jint3) {
   LOGGER("ReleaseShortArrayElements\n");

   }


    void         subReleaseIntArrayElements(JNIEnv*env,jintArray  jintArray1,jint * jint2,jint  jint3) {
   LOGGER("ReleaseIntArrayElements\n");

   }


    void         subReleaseLongArrayElements(JNIEnv*env,jlongArray  jlongArray1,jlong * jlong2,jint  jint3) {
   LOGGER("ReleaseLongArrayElements\n");

   }


    void         subReleaseFloatArrayElements(JNIEnv*env,jfloatArray  jfloatArray1,jfloat * jfloat2,jint  jint3) {
   LOGGER("ReleaseFloatArrayElements\n");

   }


    void         subReleaseDoubleArrayElements(JNIEnv*env,jdoubleArray  jdoubleArray1,jdouble * jdouble2,jint  jint3) {
   LOGGER("ReleaseDoubleArrayElements\n");

   }


    void         subGetBooleanArrayRegion(JNIEnv*env,jbooleanArray  jbooleanArray1,jsize  jsize2,jsize  jsize3,jboolean * jboolean4) {
   LOGGER("GetBooleanArrayRegion\n");

   }




    void         subGetCharArrayRegion(JNIEnv*env,jcharArray  jcharArray1,jsize  jsize2,jsize  jsize3,jchar * jchar4) {
   LOGGER("GetCharArrayRegion\n");

   }


    void         subGetShortArrayRegion(JNIEnv*env,jshortArray  jshortArray1,jsize  jsize2,jsize  jsize3,jshort * jshort4) {
   LOGGER("GetShortArrayRegion\n");

   }


    void         subGetIntArrayRegion(JNIEnv*env,jintArray  jintArray1,jsize  jsize2,jsize  jsize3,jint * jint4) {
   LOGGER("GetIntArrayRegion\n");

   }


    void         subGetLongArrayRegion(JNIEnv*env,jlongArray  jlongArray1,jsize  jsize2,jsize  jsize3,jlong * jlong4) {
   LOGGER("GetLongArrayRegion\n");

   }


    void         subGetFloatArrayRegion(JNIEnv*env,jfloatArray  jfloatArray1,jsize  jsize2,jsize  jsize3,jfloat * jfloat4) {
   LOGGER("GetFloatArrayRegion\n");

   }


    void         subGetDoubleArrayRegion(JNIEnv*env,jdoubleArray  jdoubleArray1,jsize  jsize2,jsize  jsize3,jdouble * jdouble4) {
   LOGGER("GetDoubleArrayRegion\n");

   }


    void         subSetBooleanArrayRegion(JNIEnv*env,jbooleanArray  jbooleanArray1,jsize  jsize2,jsize  jsize3,const jboolean * const4) {
   LOGGER("SetBooleanArrayRegion\n");

   }




    void         subSetCharArrayRegion(JNIEnv*env,jcharArray  jcharArray1,jsize  jsize2,jsize  jsize3,const jchar * const4) {
   LOGGER("SetCharArrayRegion\n");

   }


    void         subSetShortArrayRegion(JNIEnv*env,jshortArray  jshortArray1,jsize  jsize2,jsize  jsize3,const jshort * const4) {
   LOGGER("SetShortArrayRegion\n");

   }


    void         subSetIntArrayRegion(JNIEnv*env,jintArray  jintArray1,jsize  jsize2,jsize  jsize3,const jint * const4) {
   LOGGER("SetIntArrayRegion\n");

   }


    void         subSetLongArrayRegion(JNIEnv*env,jlongArray  jlongArray1,jsize  jsize2,jsize  jsize3,const jlong * const4) {
   LOGGER("SetLongArrayRegion\n");

   }


    void         subSetFloatArrayRegion(JNIEnv*env,jfloatArray  jfloatArray1,jsize  jsize2,jsize  jsize3,const jfloat * const4) {
   LOGGER("SetFloatArrayRegion\n");

   }


    void         subSetDoubleArrayRegion(JNIEnv*env,jdoubleArray  jdoubleArray1,jsize  jsize2,jsize  jsize3,const jdouble * const4) {
   LOGGER("SetDoubleArrayRegion\n");

   }


    jint         subRegisterNatives(JNIEnv*env,jclass  jclass1,const JNINativeMethod * const2,jint  jint3) {
   LOGGER("RegisterNatives\n");

   }


    jint         subUnregisterNatives(JNIEnv*env,jclass  jclass1) {
   LOGGER("UnregisterNatives\n");

   }


    jint         subMonitorEnter(JNIEnv*env,jobject  jobject1) {
   LOGGER("MonitorEnter\n");

   }


    jint         subMonitorExit(JNIEnv*env,jobject  jobject1) {
   LOGGER("MonitorExit\n");

   }


    jint         subGetJavaVM(JNIEnv*env,JavaVM ** JavaVM1) {
   LOGGER("GetJavaVM\n");

   }


    void         subGetStringRegion(JNIEnv*env,jstring  jstring1,jsize  jsize2,jsize  jsize3,jchar * jchar4) {
   LOGGER("GetStringRegion\n");

   }


    void         subGetStringUTFRegion(JNIEnv*env,jstring  jstring1,jsize  jsize2,jsize  jsize3,char * char4) {
   LOGGER("GetStringUTFRegion\n");

   }


    void*        subGetPrimitiveArrayCritical(JNIEnv*env,jarray  jarray1,jboolean * jboolean2) {
   LOGGER("GetPrimitiveArrayCritical\n");

   }


    void         subReleasePrimitiveArrayCritical(JNIEnv*env,jarray  jarray1,void * void2,jint  jint3) {
   LOGGER("ReleasePrimitiveArrayCritical\n");

   }


    const jchar*  subGetStringCritical(JNIEnv*env,jstring  jstring1,jboolean * jboolean2) {
   LOGGER("GetStringCritical\n");

   }


    void         subReleaseStringCritical(JNIEnv*env,jstring  jstring1,const jchar * const2) {
   LOGGER("ReleaseStringCritical\n");

   }


    jweak        subNewWeakGlobalRef(JNIEnv*env,jobject  jobject1) {
   LOGGER("NewWeakGlobalRef\n");

   }


    void         subDeleteWeakGlobalRef(JNIEnv*env,jweak  jweak1) {
   LOGGER("DeleteWeakGlobalRef\n");

   }




    jobject      subNewDirectByteBuffer(JNIEnv*env,void * void1,jlong  jlong2) {
   LOGGER("NewDirectByteBuffer\n");

   }


    void*        subGetDirectBufferAddress(JNIEnv*env,jobject  jobject1) {
   LOGGER("GetDirectBufferAddress\n");

   }


    jlong        subGetDirectBufferCapacity(JNIEnv*env,jobject  jobject1) {
   LOGGER("GetDirectBufferCapacity\n");

   }


    jobjectRefType  subGetObjectRefType(JNIEnv*env,jobject  jobject1) {
   LOGGER("GetObjectRefType\n");

   }

