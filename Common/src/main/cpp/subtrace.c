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
   LOGSTRING("GetVersion\n");

   }


    jclass       subDefineClass(JNIEnv*env,const char * const1,jobject  jobject2,const jbyte * const3,jsize  jsize4) {
   LOGSTRING("DefineClass\n");

   }


    jmethodID    subFromReflectedMethod(JNIEnv*env,jobject  jobject1) {
   LOGSTRING("FromReflectedMethod\n");

   }


    jfieldID     subFromReflectedField(JNIEnv*env,jobject  jobject1) {
   LOGSTRING("FromReflectedField\n");

   }


    jobject      subToReflectedMethod(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,jboolean  jboolean3) {
   LOGSTRING("ToReflectedMethod\n");

   }


    jclass       subGetSuperclass(JNIEnv*env,jclass  jclass1) {
   LOGSTRING("GetSuperclass\n");

   }


    jboolean     subIsAssignableFrom(JNIEnv*env,jclass  jclass1,jclass  jclass2) {
   LOGSTRING("IsAssignableFrom\n");

   }


    jobject      subToReflectedField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2,jboolean  jboolean3) {
   LOGSTRING("ToReflectedField\n");

   }




    jint         subThrowNew(JNIEnv*env,jclass  jclass1,const char * const2) {
   LOGSTRING("ThrowNew\n");

   }




    void         subExceptionDescribe(JNIEnv*env) {
   LOGSTRING("ExceptionDescribe\n");

   }




    void         subFatalError(JNIEnv*env,const char * const1) {
   LOGSTRING("FatalError\n");

   }


    jint         subPushLocalFrame(JNIEnv*env,jint  jint1) {
   LOGSTRING("PushLocalFrame\n");

   }


    jobject      subPopLocalFrame(JNIEnv*env,jobject  jobject1) {
   LOGSTRING("PopLocalFrame\n");

   }


    jobject      subNewGlobalRef(JNIEnv*env,jobject  jobject1) {
      LOGGER("NewGlobalRef %p\n",jobject1);
      return jobject1;
      }


    void         subDeleteGlobalRef(JNIEnv*env,jobject  jobject1) {
   LOGSTRING("DeleteGlobalRef\n");

   }


    jboolean     subIsSameObject(JNIEnv*env,jobject  jobject1,jobject  jobject2) {
	jboolean res= jobject1==jobject2;	
   LOGGER("IsSameObject(%p,%p)=%d\n",jobject1,jobject2,res);
   return res;
   }


    jobject      subNewLocalRef(JNIEnv*env,jobject  jobject1) {
   LOGSTRING("NewLocalRef\n");
	return jobject1;
   }


    jint         subEnsureLocalCapacity(JNIEnv*env,jint  jint1) {
   LOGSTRING("EnsureLocalCapacity\n");

   }


    jobject      subAllocObject(JNIEnv*env,jclass  jclass1) {
   LOGSTRING("AllocObject\n");

   }


    jobject      subNewObject(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2, ...) {
   LOGSTRING("NewObject\n");

   }



    jobject      subNewObjectA(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGSTRING("NewObjectA\n");

   }


    jboolean     subIsInstanceOf(JNIEnv*env,jobject  jobject1,jclass  jclass2) {
   LOGSTRING("IsInstanceOf\n");

   }




    jobject      subCallObjectMethod(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2, ...) {
   LOGSTRING("CallObjectMethod\n");

   }


    jobject      subCallObjectMethodV(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGSTRING("CallObjectMethodV\n");

   }


    jobject      subCallObjectMethodA(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGSTRING("CallObjectMethodA\n");

   }


    jboolean     subCallBooleanMethodV(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGSTRING("CallBooleanMethodV\n");

   }


    jboolean     subCallBooleanMethodA(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGSTRING("CallBooleanMethodA\n");

   }


    jbyte        subCallByteMethod(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2, ...) {
   LOGSTRING("CallByteMethod\n");

   }


    jbyte        subCallByteMethodV(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGSTRING("CallByteMethodV\n");

   }


    jbyte        subCallByteMethodA(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGSTRING("CallByteMethodA\n");

   }


    jchar        subCallCharMethod(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2, ...) {
   LOGSTRING("CallCharMethod\n");

   }


    jchar        subCallCharMethodV(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGSTRING("CallCharMethodV\n");

   }


    jchar        subCallCharMethodA(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGSTRING("CallCharMethodA\n");

   }


    jshort       subCallShortMethod(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2, ...) {
   LOGSTRING("CallShortMethod\n");

   }


    jshort       subCallShortMethodV(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGSTRING("CallShortMethodV\n");

   }


    jshort       subCallShortMethodA(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGSTRING("CallShortMethodA\n");

   }




    jint         subCallIntMethodV(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGSTRING("CallIntMethodV\n");

   }


    jint         subCallIntMethodA(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGSTRING("CallIntMethodA\n");

   }


    jlong        subCallLongMethod(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2, ...) {
   LOGSTRING("CallLongMethod\n");

   }


    jlong        subCallLongMethodV(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGSTRING("CallLongMethodV\n");

   }


    jlong        subCallLongMethodA(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGSTRING("CallLongMethodA\n");

   }


    jfloat       subCallFloatMethod(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2, ...) {
   LOGSTRING("CallFloatMethod\n");

   }


    jfloat       subCallFloatMethodV(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGSTRING("CallFloatMethodV\n");

   }


    jfloat       subCallFloatMethodA(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGSTRING("CallFloatMethodA\n");

   }




    jdouble      subCallDoubleMethodV(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGSTRING("CallDoubleMethodV\n");

   }


    jdouble      subCallDoubleMethodA(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGSTRING("CallDoubleMethodA\n");

   }




    void         subCallVoidMethodV(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGSTRING("CallVoidMethodV\n");

   }


    void         subCallVoidMethodA(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGSTRING("CallVoidMethodA\n");

   }


    jobject      subCallNonvirtualObjectMethod(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3, ...) {
   LOGSTRING("CallNonvirtualObjectMethod\n");

   }


    jobject      subCallNonvirtualObjectMethodV(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,va_list  va_list4) {
   LOGSTRING("CallNonvirtualObjectMethodV\n");

   }


    jobject      subCallNonvirtualObjectMethodA(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,const jvalue * const4) {
   LOGSTRING("CallNonvirtualObjectMethodA\n");

   }


    jboolean     subCallNonvirtualBooleanMethod(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3, ...) {
   LOGSTRING("CallNonvirtualBooleanMethod\n");

   }


    jboolean     subCallNonvirtualBooleanMethodV(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,va_list  va_list4) {
   LOGSTRING("CallNonvirtualBooleanMethodV\n");

   }


    jboolean     subCallNonvirtualBooleanMethodA(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,const jvalue * const4) {
   LOGSTRING("CallNonvirtualBooleanMethodA\n");

   }


    jbyte        subCallNonvirtualByteMethod(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3, ...) {
   LOGSTRING("CallNonvirtualByteMethod\n");

   }


    jbyte        subCallNonvirtualByteMethodV(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,va_list  va_list4) {
   LOGSTRING("CallNonvirtualByteMethodV\n");

   }


    jbyte        subCallNonvirtualByteMethodA(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,const jvalue * const4) {
   LOGSTRING("CallNonvirtualByteMethodA\n");

   }


    jchar        subCallNonvirtualCharMethod(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3, ...) {
   LOGSTRING("CallNonvirtualCharMethod\n");

   }


    jchar        subCallNonvirtualCharMethodV(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,va_list  va_list4) {
   LOGSTRING("CallNonvirtualCharMethodV\n");

   }


    jchar        subCallNonvirtualCharMethodA(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,const jvalue * const4) {
   LOGSTRING("CallNonvirtualCharMethodA\n");

   }


    jshort       subCallNonvirtualShortMethod(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3, ...) {
   LOGSTRING("CallNonvirtualShortMethod\n");

   }


    jshort       subCallNonvirtualShortMethodV(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,va_list  va_list4) {
   LOGSTRING("CallNonvirtualShortMethodV\n");

   }


    jshort       subCallNonvirtualShortMethodA(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,const jvalue * const4) {
   LOGSTRING("CallNonvirtualShortMethodA\n");

   }


    jint         subCallNonvirtualIntMethod(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3, ...) {
   LOGSTRING("CallNonvirtualIntMethod\n");

   }


    jint         subCallNonvirtualIntMethodV(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,va_list  va_list4) {
   LOGSTRING("CallNonvirtualIntMethodV\n");

   }


    jint         subCallNonvirtualIntMethodA(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,const jvalue * const4) {
   LOGSTRING("CallNonvirtualIntMethodA\n");

   }


    jlong        subCallNonvirtualLongMethod(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3, ...) {
   LOGSTRING("CallNonvirtualLongMethod\n");

   }


    jlong        subCallNonvirtualLongMethodV(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,va_list  va_list4) {
   LOGSTRING("CallNonvirtualLongMethodV\n");

   }


    jlong        subCallNonvirtualLongMethodA(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,const jvalue * const4) {
   LOGSTRING("CallNonvirtualLongMethodA\n");

   }


    jfloat       subCallNonvirtualFloatMethod(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3, ...) {
   LOGSTRING("CallNonvirtualFloatMethod\n");

   }


    jfloat       subCallNonvirtualFloatMethodV(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,va_list  va_list4) {
   LOGSTRING("CallNonvirtualFloatMethodV\n");

   }


    jfloat       subCallNonvirtualFloatMethodA(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,const jvalue * const4) {
   LOGSTRING("CallNonvirtualFloatMethodA\n");

   }


    jdouble      subCallNonvirtualDoubleMethod(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3, ...) {
   LOGSTRING("CallNonvirtualDoubleMethod\n");

   }


    jdouble      subCallNonvirtualDoubleMethodV(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,va_list  va_list4) {
   LOGSTRING("CallNonvirtualDoubleMethodV\n");

   }


    jdouble      subCallNonvirtualDoubleMethodA(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,const jvalue * const4) {
   LOGSTRING("CallNonvirtualDoubleMethodA\n");

   }


    void         subCallNonvirtualVoidMethod(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3, ...) {
   LOGSTRING("CallNonvirtualVoidMethod\n");

   }


    void         subCallNonvirtualVoidMethodV(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,va_list  va_list4) {
   LOGSTRING("CallNonvirtualVoidMethodV\n");

   }


    void         subCallNonvirtualVoidMethodA(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,const jvalue * const4) {
   LOGSTRING("CallNonvirtualVoidMethodA\n");

   }




    jobject      subGetObjectField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2) {
   LOGSTRING("GetObjectField\n");

   }


    jboolean     subGetBooleanField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2) {
   LOGSTRING("GetBooleanField\n");

   }


    jbyte        subGetByteField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2) {
   LOGSTRING("GetByteField\n");

   }


    jchar        subGetCharField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2) {
   LOGSTRING("GetCharField\n");

   }


    jshort       subGetShortField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2) {
   LOGSTRING("GetShortField\n");

   }


    jint         subGetIntField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2) {
   LOGSTRING("GetIntField\n");

   }




    jfloat       subGetFloatField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2) {
   LOGSTRING("GetFloatField\n");

   }


    jdouble      subGetDoubleField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2) {
   LOGSTRING("GetDoubleField\n");

   }


    void         subSetObjectField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2,jobject  jobject3) {
   LOGSTRING("SetObjectField\n");

   }


    void         subSetBooleanField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2,jboolean  jboolean3) {
   LOGSTRING("SetBooleanField\n");

   }


    void         subSetByteField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2,jbyte  jbyte3) {
   LOGSTRING("SetByteField\n");

   }


    void         subSetCharField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2,jchar  jchar3) {
   LOGSTRING("SetCharField\n");

   }


    void         subSetShortField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2,jshort  jshort3) {
   LOGSTRING("SetShortField\n");

   }




    void         subSetFloatField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2,jfloat  jfloat3) {
   LOGSTRING("SetFloatField\n");

   }






    jobject      subCallStaticObjectMethodV(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGSTRING("CallStaticObjectMethodV\n");

   }


    jobject      subCallStaticObjectMethodA(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGSTRING("CallStaticObjectMethodA\n");

   }


/*    jboolean     subCallStaticBooleanMethod(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2, ...) {
   LOGSTRING("CallStaticBooleanMethod\n");

   }*/


    jboolean     subCallStaticBooleanMethodV(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGSTRING("CallStaticBooleanMethodV\n");

   }


    jboolean     subCallStaticBooleanMethodA(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGSTRING("CallStaticBooleanMethodA\n");

   }


    jbyte        subCallStaticByteMethod(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2, ...) {
   LOGSTRING("CallStaticByteMethod\n");

   }


    jbyte        subCallStaticByteMethodV(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGSTRING("CallStaticByteMethodV\n");

   }


    jbyte        subCallStaticByteMethodA(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGSTRING("CallStaticByteMethodA\n");

   }


    jchar        subCallStaticCharMethod(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2, ...) {
   LOGSTRING("CallStaticCharMethod\n");

   }


    jchar        subCallStaticCharMethodV(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGSTRING("CallStaticCharMethodV\n");

   }


    jchar        subCallStaticCharMethodA(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGSTRING("CallStaticCharMethodA\n");

   }


    jshort       subCallStaticShortMethod(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2, ...) {
   LOGSTRING("CallStaticShortMethod\n");

   }


    jshort       subCallStaticShortMethodV(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGSTRING("CallStaticShortMethodV\n");

   }


    jshort       subCallStaticShortMethodA(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGSTRING("CallStaticShortMethodA\n");

   }


    jint         subCallStaticIntMethodV(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGSTRING("CallStaticIntMethodV\n");

   }


    jint         subCallStaticIntMethodA(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGSTRING("CallStaticIntMethodA\n");

   }


    jlong        subCallStaticLongMethod(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2, ...) {
   LOGSTRING("CallStaticLongMethod\n");

   }


    jlong        subCallStaticLongMethodV(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGSTRING("CallStaticLongMethodV\n");

   }


    jlong        subCallStaticLongMethodA(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGSTRING("CallStaticLongMethodA\n");

   }


    jfloat       subCallStaticFloatMethod(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2, ...) {
   LOGSTRING("CallStaticFloatMethod\n");

   }


    jfloat       subCallStaticFloatMethodV(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGSTRING("CallStaticFloatMethodV\n");

   }


    jfloat       subCallStaticFloatMethodA(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGSTRING("CallStaticFloatMethodA\n");

   }


    jdouble      subCallStaticDoubleMethod(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2, ...) {
   LOGSTRING("CallStaticDoubleMethod\n");

   }


    jdouble      subCallStaticDoubleMethodV(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGSTRING("CallStaticDoubleMethodV\n");

   }


    jdouble      subCallStaticDoubleMethodA(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGSTRING("CallStaticDoubleMethodA\n");

   }


    void         subCallStaticVoidMethod(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2, ...) {
   LOGSTRING("CallStaticVoidMethod\n");

   }


    void         subCallStaticVoidMethodV(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGSTRING("CallStaticVoidMethodV\n");

   }


    void         subCallStaticVoidMethodA(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGSTRING("CallStaticVoidMethodA\n");

   }


    jfieldID     subGetStaticFieldID(JNIEnv*env,jclass  jclass1,const char * const2,const char * const3) {
   LOGSTRING("GetStaticFieldID\n");

   }


    jobject      subGetStaticObjectField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2) {
   LOGSTRING("GetStaticObjectField\n");

   }


    jboolean     subGetStaticBooleanField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2) {
   LOGSTRING("GetStaticBooleanField\n");

   }


    jbyte        subGetStaticByteField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2) {
   LOGSTRING("GetStaticByteField\n");

   }


    jchar        subGetStaticCharField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2) {
   LOGSTRING("GetStaticCharField\n");

   }


    jshort       subGetStaticShortField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2) {
   LOGSTRING("GetStaticShortField\n");

   }


    jint         subGetStaticIntField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2) {
   LOGSTRING("GetStaticIntField\n");

   }


    jlong        subGetStaticLongField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2) {
   LOGSTRING("GetStaticLongField\n");

   }


    jfloat       subGetStaticFloatField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2) {
   LOGSTRING("GetStaticFloatField\n");

   }


    jdouble      subGetStaticDoubleField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2) {
   LOGSTRING("GetStaticDoubleField\n");

   }


    void         subSetStaticObjectField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2,jobject  jobject3) {
   LOGSTRING("SetStaticObjectField\n");

   }


    void         subSetStaticBooleanField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2,jboolean  jboolean3) {
   LOGSTRING("SetStaticBooleanField\n");

   }


    void         subSetStaticByteField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2,jbyte  jbyte3) {
   LOGSTRING("SetStaticByteField\n");

   }


    void         subSetStaticCharField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2,jchar  jchar3) {
   LOGSTRING("SetStaticCharField\n");

   }


    void         subSetStaticShortField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2,jshort  jshort3) {
   LOGSTRING("SetStaticShortField\n");

   }


    void         subSetStaticIntField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2,jint  jint3) {
   LOGSTRING("SetStaticIntField\n");

   }


    void         subSetStaticLongField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2,jlong  jlong3) {
   LOGSTRING("SetStaticLongField\n");

   }


    void         subSetStaticFloatField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2,jfloat  jfloat3) {
   LOGSTRING("SetStaticFloatField\n");

   }


    void         subSetStaticDoubleField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2,jdouble  jdouble3) {
   LOGSTRING("SetStaticDoubleField\n");

   }


    jstring      subNewString(JNIEnv*env,const jchar * const1,jsize  jsize2) {
   LOGSTRING("NewString\n");
   }


    jsize        subGetStringLength(JNIEnv*env,jstring  jstring1) {
   LOGSTRING("GetStringLength\n");

   }


    const jchar*  subGetStringChars(JNIEnv*env,jstring  jstring1,jboolean * jboolean2) {
   LOGSTRING("GetStringChars\n");

   }


    void         subReleaseStringChars(JNIEnv*env,jstring  jstring1,const jchar * const2) {
   LOGSTRING("ReleaseStringChars\n");

   }


    jstring      subNewStringUTF(JNIEnv*env,const char * const1) {
   LOGGER("NewStringUTF(%s)\n",const1);
   return (jstring)const1;
   }


    jsize        subGetStringUTFLength(JNIEnv*env,jstring  jstring1) {
   LOGSTRING("GetStringUTFLength\n");
   const char *str=(const char *)jstring1;
   return strlen(str);

   }





    jobjectArray  subNewObjectArray(JNIEnv*env,jsize  jsize1,jclass  jclass2,jobject  jobject3) {
   LOGSTRING("NewObjectArray\n");

   }


    jobject      subGetObjectArrayElement(JNIEnv*env,jobjectArray  jobjectArray1,jsize  jsize2) {
   LOGSTRING("GetObjectArrayElement\n");

   }


    void         subSetObjectArrayElement(JNIEnv*env,jobjectArray  jobjectArray1,jsize  jsize2,jobject  jobject3) {
   LOGSTRING("SetObjectArrayElement\n");

   }


    jbooleanArray  subNewBooleanArray(JNIEnv*env,jsize  jsize1) {
   LOGSTRING("NewBooleanArray\n");

   }


    jcharArray     subNewCharArray(JNIEnv*env,jsize  jsize1) {
   LOGSTRING("NewCharArray\n");

   }


    jshortArray    subNewShortArray(JNIEnv*env,jsize  jsize1) {
   LOGSTRING("NewShortArray\n");

   }


    jintArray      subNewIntArray(JNIEnv*env,jsize  jsize1) {
   LOGSTRING("NewIntArray\n");

   }


    jlongArray     subNewLongArray(JNIEnv*env,jsize  jsize1) {
   LOGSTRING("NewLongArray\n");

   }


    jfloatArray    subNewFloatArray(JNIEnv*env,jsize  jsize1) {
   LOGSTRING("NewFloatArray\n");

   }


    jdoubleArray   subNewDoubleArray(JNIEnv*env,jsize  jsize1) {
   LOGSTRING("NewDoubleArray\n");

   }


    jboolean*    subGetBooleanArrayElements(JNIEnv*env,jbooleanArray  jbooleanArray1,jboolean * jboolean2) {
   LOGSTRING("GetBooleanArrayElements\n");

   }


    jchar*       subGetCharArrayElements(JNIEnv*env,jcharArray  jcharArray1,jboolean * jboolean2) {
   LOGSTRING("GetCharArrayElements\n");

   }


    jshort*      subGetShortArrayElements(JNIEnv*env,jshortArray  jshortArray1,jboolean * jboolean2) {
   LOGSTRING("GetShortArrayElements\n");

   }


    jint*        subGetIntArrayElements(JNIEnv*env,jintArray  jintArray1,jboolean * jboolean2) {
   LOGSTRING("GetIntArrayElements\n");

   }


    jlong*       subGetLongArrayElements(JNIEnv*env,jlongArray  jlongArray1,jboolean * jboolean2) {
   LOGSTRING("GetLongArrayElements\n");

   }


    jfloat*      subGetFloatArrayElements(JNIEnv*env,jfloatArray  jfloatArray1,jboolean * jboolean2) {
   LOGSTRING("GetFloatArrayElements\n");

   }


    jdouble*     subGetDoubleArrayElements(JNIEnv*env,jdoubleArray  jdoubleArray1,jboolean * jboolean2) {
   LOGSTRING("GetDoubleArrayElements\n");

   }


    void         subReleaseBooleanArrayElements(JNIEnv*env,jbooleanArray  jbooleanArray1,jboolean * jboolean2,jint  jint3) {
   LOGSTRING("ReleaseBooleanArrayElements\n");

   }


    void         subReleaseCharArrayElements(JNIEnv*env,jcharArray  jcharArray1,jchar * jchar2,jint  jint3) {
   LOGSTRING("ReleaseCharArrayElements\n");

   }


    void         subReleaseShortArrayElements(JNIEnv*env,jshortArray  jshortArray1,jshort * jshort2,jint  jint3) {
   LOGSTRING("ReleaseShortArrayElements\n");

   }


    void         subReleaseIntArrayElements(JNIEnv*env,jintArray  jintArray1,jint * jint2,jint  jint3) {
   LOGSTRING("ReleaseIntArrayElements\n");

   }


    void         subReleaseLongArrayElements(JNIEnv*env,jlongArray  jlongArray1,jlong * jlong2,jint  jint3) {
   LOGSTRING("ReleaseLongArrayElements\n");

   }


    void         subReleaseFloatArrayElements(JNIEnv*env,jfloatArray  jfloatArray1,jfloat * jfloat2,jint  jint3) {
   LOGSTRING("ReleaseFloatArrayElements\n");

   }


    void         subReleaseDoubleArrayElements(JNIEnv*env,jdoubleArray  jdoubleArray1,jdouble * jdouble2,jint  jint3) {
   LOGSTRING("ReleaseDoubleArrayElements\n");

   }


    void         subGetBooleanArrayRegion(JNIEnv*env,jbooleanArray  jbooleanArray1,jsize  jsize2,jsize  jsize3,jboolean * jboolean4) {
   LOGSTRING("GetBooleanArrayRegion\n");

   }




    void         subGetCharArrayRegion(JNIEnv*env,jcharArray  jcharArray1,jsize  jsize2,jsize  jsize3,jchar * jchar4) {
   LOGSTRING("GetCharArrayRegion\n");

   }


    void         subGetShortArrayRegion(JNIEnv*env,jshortArray  jshortArray1,jsize  jsize2,jsize  jsize3,jshort * jshort4) {
   LOGSTRING("GetShortArrayRegion\n");

   }


    void         subGetIntArrayRegion(JNIEnv*env,jintArray  jintArray1,jsize  jsize2,jsize  jsize3,jint * jint4) {
   LOGSTRING("GetIntArrayRegion\n");

   }


    void         subGetLongArrayRegion(JNIEnv*env,jlongArray  jlongArray1,jsize  jsize2,jsize  jsize3,jlong * jlong4) {
   LOGSTRING("GetLongArrayRegion\n");

   }


    void         subGetFloatArrayRegion(JNIEnv*env,jfloatArray  jfloatArray1,jsize  jsize2,jsize  jsize3,jfloat * jfloat4) {
   LOGSTRING("GetFloatArrayRegion\n");

   }


    void         subGetDoubleArrayRegion(JNIEnv*env,jdoubleArray  jdoubleArray1,jsize  jsize2,jsize  jsize3,jdouble * jdouble4) {
   LOGSTRING("GetDoubleArrayRegion\n");

   }


    void         subSetBooleanArrayRegion(JNIEnv*env,jbooleanArray  jbooleanArray1,jsize  jsize2,jsize  jsize3,const jboolean * const4) {
   LOGSTRING("SetBooleanArrayRegion\n");

   }




    void         subSetCharArrayRegion(JNIEnv*env,jcharArray  jcharArray1,jsize  jsize2,jsize  jsize3,const jchar * const4) {
   LOGSTRING("SetCharArrayRegion\n");

   }


    void         subSetShortArrayRegion(JNIEnv*env,jshortArray  jshortArray1,jsize  jsize2,jsize  jsize3,const jshort * const4) {
   LOGSTRING("SetShortArrayRegion\n");

   }


    void         subSetIntArrayRegion(JNIEnv*env,jintArray  jintArray1,jsize  jsize2,jsize  jsize3,const jint * const4) {
   LOGSTRING("SetIntArrayRegion\n");

   }


    void         subSetLongArrayRegion(JNIEnv*env,jlongArray  jlongArray1,jsize  jsize2,jsize  jsize3,const jlong * const4) {
   LOGSTRING("SetLongArrayRegion\n");

   }


    void         subSetFloatArrayRegion(JNIEnv*env,jfloatArray  jfloatArray1,jsize  jsize2,jsize  jsize3,const jfloat * const4) {
   LOGSTRING("SetFloatArrayRegion\n");

   }


    void         subSetDoubleArrayRegion(JNIEnv*env,jdoubleArray  jdoubleArray1,jsize  jsize2,jsize  jsize3,const jdouble * const4) {
   LOGSTRING("SetDoubleArrayRegion\n");

   }



    jint         subUnregisterNatives(JNIEnv*env,jclass  jclass1) {
   LOGSTRING("UnregisterNatives\n");

   }


    jint         subMonitorEnter(JNIEnv*env,jobject  jobject1) {
   LOGSTRING("MonitorEnter\n");

   }


    jint         subMonitorExit(JNIEnv*env,jobject  jobject1) {
   LOGSTRING("MonitorExit\n");

   }


    jint         subGetJavaVM(JNIEnv*env,JavaVM ** JavaVM1) {
   LOGSTRING("GetJavaVM\n");

   }


    void         subGetStringRegion(JNIEnv*env,jstring  jstring1,jsize  jsize2,jsize  jsize3,jchar * jchar4) {
   LOGSTRING("GetStringRegion\n");

   }


    void         subGetStringUTFRegion(JNIEnv*env,jstring  jstring1,jsize  jsize2,jsize  jsize3,char * char4) {
   LOGSTRING("GetStringUTFRegion\n");

   }


    void*        subGetPrimitiveArrayCritical(JNIEnv*env,jarray  jarray1,jboolean * jboolean2) {
   LOGSTRING("GetPrimitiveArrayCritical\n");

   }


    void         subReleasePrimitiveArrayCritical(JNIEnv*env,jarray  jarray1,void * void2,jint  jint3) {
   LOGSTRING("ReleasePrimitiveArrayCritical\n");

   }


    const jchar*  subGetStringCritical(JNIEnv*env,jstring  jstring1,jboolean * jboolean2) {
   LOGSTRING("GetStringCritical\n");

   }


    void         subReleaseStringCritical(JNIEnv*env,jstring  jstring1,const jchar * const2) {
   LOGSTRING("ReleaseStringCritical\n");

   }


    jweak        subNewWeakGlobalRef(JNIEnv*env,jobject  jobject1) {
   LOGSTRING("NewWeakGlobalRef\n");

   }


    void         subDeleteWeakGlobalRef(JNIEnv*env,jweak  jweak1) {
   LOGSTRING("DeleteWeakGlobalRef\n");

   }




    jobject      subNewDirectByteBuffer(JNIEnv*env,void * void1,jlong  jlong2) {
   LOGSTRING("NewDirectByteBuffer\n");

   }


    void*        subGetDirectBufferAddress(JNIEnv*env,jobject  jobject1) {
   LOGSTRING("GetDirectBufferAddress\n");

   }


    jlong        subGetDirectBufferCapacity(JNIEnv*env,jobject  jobject1) {
   LOGSTRING("GetDirectBufferCapacity\n");

   }


    jobjectRefType  subGetObjectRefType(JNIEnv*env,jobject  jobject1) {
   LOGSTRING("GetObjectRefType\n");

   }

