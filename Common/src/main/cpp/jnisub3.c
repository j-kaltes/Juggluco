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


    jint        (*GetVersion)(JNIEnv *);
    jint         MyGetVersion(JNIEnv*env) {
   LOGGER("GetVersion\n");
   return GetVersion(env);
   }

    jclass      (*DefineClass)(JNIEnv*, const char*, jobject, const jbyte*,jsize);
    jclass       MyDefineClass(JNIEnv*env,const char * const1,jobject  jobject2,const jbyte * const3,jsize  jsize4) {
   LOGGER("DefineClass\n");
   return DefineClass(env,const1,jobject2,const3,jsize4);
   }

    jmethodID   (*FromReflectedMethod)(JNIEnv*, jobject);
    jmethodID    MyFromReflectedMethod(JNIEnv*env,jobject  jobject1) {
   LOGGER("FromReflectedMethod\n");
   return FromReflectedMethod(env,jobject1);
   }

    jfieldID    (*FromReflectedField)(JNIEnv*, jobject);
    jfieldID     MyFromReflectedField(JNIEnv*env,jobject  jobject1) {
   LOGGER("FromReflectedField\n");
   return FromReflectedField(env,jobject1);
   }

    jobject     (*ToReflectedMethod)(JNIEnv*, jclass, jmethodID, jboolean);
    jobject      MyToReflectedMethod(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,jboolean  jboolean3) {
   LOGGER("ToReflectedMethod\n");
   return ToReflectedMethod(env,jclass1,jmethodID2,jboolean3);
   }

    jclass      (*GetSuperclass)(JNIEnv*, jclass);
    jclass       MyGetSuperclass(JNIEnv*env,jclass  jclass1) {
   LOGGER("GetSuperclass\n");
   return GetSuperclass(env,jclass1);
   }

    jboolean    (*IsAssignableFrom)(JNIEnv*, jclass, jclass);
    jboolean     MyIsAssignableFrom(JNIEnv*env,jclass  jclass1,jclass  jclass2) {
   LOGGER("IsAssignableFrom\n");
   return IsAssignableFrom(env,jclass1,jclass2);
   }

    jobject     (*ToReflectedField)(JNIEnv*, jclass, jfieldID, jboolean);
    jobject      MyToReflectedField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2,jboolean  jboolean3) {
   LOGGER("ToReflectedField\n");
   return ToReflectedField(env,jclass1,jfieldID2,jboolean3);
   }

    jint        (*Throw)(JNIEnv*, jthrowable);
    jint         MyThrow(JNIEnv*env,jthrowable  jthrowable1) {
   LOGGER("Throw\n");
   return Throw(env,jthrowable1);
   }

    jint        (*ThrowNew)(JNIEnv *, jclass, const char *);
    jint         MyThrowNew(JNIEnv*env,jclass  jclass1,const char * const2) {
   LOGGER("ThrowNew\n");
   return ThrowNew(env,jclass1,const2);
   }

    jthrowable  (*ExceptionOccurred)(JNIEnv*);
    jthrowable   MyExceptionOccurred(JNIEnv*env) {
   LOGGER("ExceptionOccurred\n");
   return ExceptionOccurred(env);
   }

    void        (*ExceptionDescribe)(JNIEnv*);
    void         MyExceptionDescribe(JNIEnv*env) {
   LOGGER("ExceptionDescribe\n");
   return ExceptionDescribe(env);
   }

    void        (*ExceptionClear)(JNIEnv*);
    void         MyExceptionClear(JNIEnv*env) {
   LOGGER("ExceptionClear\n");
   return ExceptionClear(env);
   }

    void        (*FatalError)(JNIEnv*, const char*);
    void         MyFatalError(JNIEnv*env,const char * const1) {
   LOGGER("FatalError\n");
   return FatalError(env,const1);
   }

    jint        (*PushLocalFrame)(JNIEnv*, jint);
    jint         MyPushLocalFrame(JNIEnv*env,jint  jint1) {
   LOGGER("PushLocalFrame\n");
   return PushLocalFrame(env,jint1);
   }

    jobject     (*PopLocalFrame)(JNIEnv*, jobject);
    jobject      MyPopLocalFrame(JNIEnv*env,jobject  jobject1) {
   LOGGER("PopLocalFrame\n");
   return PopLocalFrame(env,jobject1);
   }

    jobject     (*NewGlobalRef)(JNIEnv*, jobject);
    jobject      MyNewGlobalRef(JNIEnv*env,jobject  jobject1) {
   LOGGER("NewGlobalRef\n");
   return NewGlobalRef(env,jobject1);
   }

    void        (*DeleteGlobalRef)(JNIEnv*, jobject);
    void         MyDeleteGlobalRef(JNIEnv*env,jobject  jobject1) {
   LOGGER("DeleteGlobalRef\n");
   return DeleteGlobalRef(env,jobject1);
   }

    jboolean    (*IsSameObject)(JNIEnv*, jobject, jobject);
    jboolean     MyIsSameObject(JNIEnv*env,jobject  jobject1,jobject  jobject2) {
   LOGGER("IsSameObject\n");
   return IsSameObject(env,jobject1,jobject2);
   }

    jobject     (*NewLocalRef)(JNIEnv*, jobject);
    jobject      MyNewLocalRef(JNIEnv*env,jobject  jobject1) {
   LOGGER("NewLocalRef\n");
   return NewLocalRef(env,jobject1);
   }

    jint        (*EnsureLocalCapacity)(JNIEnv*, jint);
    jint         MyEnsureLocalCapacity(JNIEnv*env,jint  jint1) {
   LOGGER("EnsureLocalCapacity\n");
   return EnsureLocalCapacity(env,jint1);
   }

    jobject     (*AllocObject)(JNIEnv*, jclass);
    jobject      MyAllocObject(JNIEnv*env,jclass  jclass1) {
   LOGGER("AllocObject\n");
   return AllocObject(env,jclass1);
   }

    jobject     (*NewObject)(JNIEnv*, jclass, jmethodID, ...);
    jobject      MyNewObject(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2, ...) {
   LOGGER("NewObject\n");
   return NewObject(env,jclass1,jmethodID2, ...);
   }

    jobject     (*NewObjectV)(JNIEnv*, jclass, jmethodID, va_list);
    jobject      MyNewObjectV(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGGER("NewObjectV\n");
   return NewObjectV(env,jclass1,jmethodID2,va_list3);
   }

    jobject     (*NewObjectA)(JNIEnv*, jclass, jmethodID, const jvalue*);
    jobject      MyNewObjectA(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGGER("NewObjectA\n");
   return NewObjectA(env,jclass1,jmethodID2,const3);
   }

    jboolean    (*IsInstanceOf)(JNIEnv*, jobject, jclass);
    jboolean     MyIsInstanceOf(JNIEnv*env,jobject  jobject1,jclass  jclass2) {
   LOGGER("IsInstanceOf\n");
   return IsInstanceOf(env,jobject1,jclass2);
   }

    jmethodID   (*GetMethodID)(JNIEnv*, jclass, const char*, const char*);
    jmethodID    MyGetMethodID(JNIEnv*env,jclass  jclass1,const char * const2,const char * const3) {
   LOGGER("GetMethodID\n");
   return GetMethodID(env,jclass1,const2,const3);
   }

    jobject     (*CallObjectMethod)(JNIEnv*, jobject, jmethodID, ...);
    jobject      MyCallObjectMethod(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2, ...) {
   LOGGER("CallObjectMethod\n");
   return CallObjectMethod(env,jobject1,jmethodID2, ...);
   }

    jobject     (*CallObjectMethodV)(JNIEnv*, jobject, jmethodID, va_list);
    jobject      MyCallObjectMethodV(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGGER("CallObjectMethodV\n");
   return CallObjectMethodV(env,jobject1,jmethodID2,va_list3);
   }

    jobject     (*CallObjectMethodA)(JNIEnv*, jobject, jmethodID, const jvalue*);
    jobject      MyCallObjectMethodA(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGGER("CallObjectMethodA\n");
   return CallObjectMethodA(env,jobject1,jmethodID2,const3);
   }

    jboolean    (*CallBooleanMethodV)(JNIEnv*, jobject, jmethodID, va_list);
    jboolean     MyCallBooleanMethodV(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGGER("CallBooleanMethodV\n");
   return CallBooleanMethodV(env,jobject1,jmethodID2,va_list3);
   }

    jboolean    (*CallBooleanMethodA)(JNIEnv*, jobject, jmethodID, const jvalue*);
    jboolean     MyCallBooleanMethodA(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGGER("CallBooleanMethodA\n");
   return CallBooleanMethodA(env,jobject1,jmethodID2,const3);
   }

    jbyte       (*CallByteMethod)(JNIEnv*, jobject, jmethodID, ...);
    jbyte        MyCallByteMethod(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2, ...) {
   LOGGER("CallByteMethod\n");
   return CallByteMethod(env,jobject1,jmethodID2, ...);
   }

    jbyte       (*CallByteMethodV)(JNIEnv*, jobject, jmethodID, va_list);
    jbyte        MyCallByteMethodV(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGGER("CallByteMethodV\n");
   return CallByteMethodV(env,jobject1,jmethodID2,va_list3);
   }

    jbyte       (*CallByteMethodA)(JNIEnv*, jobject, jmethodID, const jvalue*);
    jbyte        MyCallByteMethodA(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGGER("CallByteMethodA\n");
   return CallByteMethodA(env,jobject1,jmethodID2,const3);
   }

    jchar       (*CallCharMethod)(JNIEnv*, jobject, jmethodID, ...);
    jchar        MyCallCharMethod(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2, ...) {
   LOGGER("CallCharMethod\n");
   return CallCharMethod(env,jobject1,jmethodID2, ...);
   }

    jchar       (*CallCharMethodV)(JNIEnv*, jobject, jmethodID, va_list);
    jchar        MyCallCharMethodV(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGGER("CallCharMethodV\n");
   return CallCharMethodV(env,jobject1,jmethodID2,va_list3);
   }

    jchar       (*CallCharMethodA)(JNIEnv*, jobject, jmethodID, const jvalue*);
    jchar        MyCallCharMethodA(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGGER("CallCharMethodA\n");
   return CallCharMethodA(env,jobject1,jmethodID2,const3);
   }

    jshort      (*CallShortMethod)(JNIEnv*, jobject, jmethodID, ...);
    jshort       MyCallShortMethod(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2, ...) {
   LOGGER("CallShortMethod\n");
   return CallShortMethod(env,jobject1,jmethodID2, ...);
   }

    jshort      (*CallShortMethodV)(JNIEnv*, jobject, jmethodID, va_list);
    jshort       MyCallShortMethodV(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGGER("CallShortMethodV\n");
   return CallShortMethodV(env,jobject1,jmethodID2,va_list3);
   }

    jshort      (*CallShortMethodA)(JNIEnv*, jobject, jmethodID, const jvalue*);
    jshort       MyCallShortMethodA(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGGER("CallShortMethodA\n");
   return CallShortMethodA(env,jobject1,jmethodID2,const3);
   }

    jint        (*CallIntMethod)(JNIEnv*, jobject, jmethodID, ...);
    jint         MyCallIntMethod(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2, ...) {
   LOGGER("CallIntMethod\n");
   return CallIntMethod(env,jobject1,jmethodID2, ...);
   }

    jint        (*CallIntMethodV)(JNIEnv*, jobject, jmethodID, va_list);
    jint         MyCallIntMethodV(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGGER("CallIntMethodV\n");
   return CallIntMethodV(env,jobject1,jmethodID2,va_list3);
   }

    jint        (*CallIntMethodA)(JNIEnv*, jobject, jmethodID, const jvalue*);
    jint         MyCallIntMethodA(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGGER("CallIntMethodA\n");
   return CallIntMethodA(env,jobject1,jmethodID2,const3);
   }

    jlong       (*CallLongMethod)(JNIEnv*, jobject, jmethodID, ...);
    jlong        MyCallLongMethod(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2, ...) {
   LOGGER("CallLongMethod\n");
   return CallLongMethod(env,jobject1,jmethodID2, ...);
   }

    jlong       (*CallLongMethodV)(JNIEnv*, jobject, jmethodID, va_list);
    jlong        MyCallLongMethodV(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGGER("CallLongMethodV\n");
   return CallLongMethodV(env,jobject1,jmethodID2,va_list3);
   }

    jlong       (*CallLongMethodA)(JNIEnv*, jobject, jmethodID, const jvalue*);
    jlong        MyCallLongMethodA(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGGER("CallLongMethodA\n");
   return CallLongMethodA(env,jobject1,jmethodID2,const3);
   }

    jfloat      (*CallFloatMethod)(JNIEnv*, jobject, jmethodID, ...);
    jfloat       MyCallFloatMethod(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2, ...) {
   LOGGER("CallFloatMethod\n");
   return CallFloatMethod(env,jobject1,jmethodID2, ...);
   }

    jfloat      (*CallFloatMethodV)(JNIEnv*, jobject, jmethodID, va_list);
    jfloat       MyCallFloatMethodV(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGGER("CallFloatMethodV\n");
   return CallFloatMethodV(env,jobject1,jmethodID2,va_list3);
   }

    jfloat      (*CallFloatMethodA)(JNIEnv*, jobject, jmethodID, const jvalue*);
    jfloat       MyCallFloatMethodA(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGGER("CallFloatMethodA\n");
   return CallFloatMethodA(env,jobject1,jmethodID2,const3);
   }

    jdouble     (*CallDoubleMethod)(JNIEnv*, jobject, jmethodID, ...);
    jdouble      MyCallDoubleMethod(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2, ...) {
   LOGGER("CallDoubleMethod\n");
   return CallDoubleMethod(env,jobject1,jmethodID2, ...);
   }

    jdouble     (*CallDoubleMethodV)(JNIEnv*, jobject, jmethodID, va_list);
    jdouble      MyCallDoubleMethodV(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGGER("CallDoubleMethodV\n");
   return CallDoubleMethodV(env,jobject1,jmethodID2,va_list3);
   }

    jdouble     (*CallDoubleMethodA)(JNIEnv*, jobject, jmethodID, const jvalue*);
    jdouble      MyCallDoubleMethodA(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGGER("CallDoubleMethodA\n");
   return CallDoubleMethodA(env,jobject1,jmethodID2,const3);
   }

    void        (*CallVoidMethod)(JNIEnv*, jobject, jmethodID, ...);
    void         MyCallVoidMethod(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2, ...) {
   LOGGER("CallVoidMethod\n");
   return CallVoidMethod(env,jobject1,jmethodID2, ...);
   }

    void        (*CallVoidMethodV)(JNIEnv*, jobject, jmethodID, va_list);
    void         MyCallVoidMethodV(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGGER("CallVoidMethodV\n");
   return CallVoidMethodV(env,jobject1,jmethodID2,va_list3);
   }

    void        (*CallVoidMethodA)(JNIEnv*, jobject, jmethodID, const jvalue*);
    void         MyCallVoidMethodA(JNIEnv*env,jobject  jobject1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGGER("CallVoidMethodA\n");
   return CallVoidMethodA(env,jobject1,jmethodID2,const3);
   }

    jobject     (*CallNonvirtualObjectMethod)(JNIEnv*, jobject, jclass,jmethodID, ...);
    jobject      MyCallNonvirtualObjectMethod(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3, ...) {
   LOGGER("CallNonvirtualObjectMethod\n");
   return CallNonvirtualObjectMethod(env,jobject1,jclass2,jmethodID3, ...);
   }

    jobject     (*CallNonvirtualObjectMethodV)(JNIEnv*, jobject, jclass,jmethodID, va_list);
    jobject      MyCallNonvirtualObjectMethodV(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,va_list  va_list4) {
   LOGGER("CallNonvirtualObjectMethodV\n");
   return CallNonvirtualObjectMethodV(env,jobject1,jclass2,jmethodID3,va_list4);
   }

    jobject     (*CallNonvirtualObjectMethodA)(JNIEnv*, jobject, jclass,jmethodID, const jvalue*);
    jobject      MyCallNonvirtualObjectMethodA(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,const jvalue * const4) {
   LOGGER("CallNonvirtualObjectMethodA\n");
   return CallNonvirtualObjectMethodA(env,jobject1,jclass2,jmethodID3,const4);
   }

    jboolean    (*CallNonvirtualBooleanMethod)(JNIEnv*, jobject, jclass,jmethodID, ...);
    jboolean     MyCallNonvirtualBooleanMethod(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3, ...) {
   LOGGER("CallNonvirtualBooleanMethod\n");
   return CallNonvirtualBooleanMethod(env,jobject1,jclass2,jmethodID3, ...);
   }

    jboolean    (*CallNonvirtualBooleanMethodV)(JNIEnv*, jobject, jclass,jmethodID, va_list);
    jboolean     MyCallNonvirtualBooleanMethodV(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,va_list  va_list4) {
   LOGGER("CallNonvirtualBooleanMethodV\n");
   return CallNonvirtualBooleanMethodV(env,jobject1,jclass2,jmethodID3,va_list4);
   }

    jboolean    (*CallNonvirtualBooleanMethodA)(JNIEnv*, jobject, jclass,jmethodID, const jvalue*);
    jboolean     MyCallNonvirtualBooleanMethodA(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,const jvalue * const4) {
   LOGGER("CallNonvirtualBooleanMethodA\n");
   return CallNonvirtualBooleanMethodA(env,jobject1,jclass2,jmethodID3,const4);
   }

    jbyte       (*CallNonvirtualByteMethod)(JNIEnv*, jobject, jclass,jmethodID, ...);
    jbyte        MyCallNonvirtualByteMethod(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3, ...) {
   LOGGER("CallNonvirtualByteMethod\n");
   return CallNonvirtualByteMethod(env,jobject1,jclass2,jmethodID3, ...);
   }

    jbyte       (*CallNonvirtualByteMethodV)(JNIEnv*, jobject, jclass,jmethodID, va_list);
    jbyte        MyCallNonvirtualByteMethodV(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,va_list  va_list4) {
   LOGGER("CallNonvirtualByteMethodV\n");
   return CallNonvirtualByteMethodV(env,jobject1,jclass2,jmethodID3,va_list4);
   }

    jbyte       (*CallNonvirtualByteMethodA)(JNIEnv*, jobject, jclass,jmethodID, const jvalue*);
    jbyte        MyCallNonvirtualByteMethodA(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,const jvalue * const4) {
   LOGGER("CallNonvirtualByteMethodA\n");
   return CallNonvirtualByteMethodA(env,jobject1,jclass2,jmethodID3,const4);
   }

    jchar       (*CallNonvirtualCharMethod)(JNIEnv*, jobject, jclass,jmethodID, ...);
    jchar        MyCallNonvirtualCharMethod(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3, ...) {
   LOGGER("CallNonvirtualCharMethod\n");
   return CallNonvirtualCharMethod(env,jobject1,jclass2,jmethodID3, ...);
   }

    jchar       (*CallNonvirtualCharMethodV)(JNIEnv*, jobject, jclass,jmethodID, va_list);
    jchar        MyCallNonvirtualCharMethodV(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,va_list  va_list4) {
   LOGGER("CallNonvirtualCharMethodV\n");
   return CallNonvirtualCharMethodV(env,jobject1,jclass2,jmethodID3,va_list4);
   }

    jchar       (*CallNonvirtualCharMethodA)(JNIEnv*, jobject, jclass,jmethodID, const jvalue*);
    jchar        MyCallNonvirtualCharMethodA(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,const jvalue * const4) {
   LOGGER("CallNonvirtualCharMethodA\n");
   return CallNonvirtualCharMethodA(env,jobject1,jclass2,jmethodID3,const4);
   }

    jshort      (*CallNonvirtualShortMethod)(JNIEnv*, jobject, jclass,jmethodID, ...);
    jshort       MyCallNonvirtualShortMethod(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3, ...) {
   LOGGER("CallNonvirtualShortMethod\n");
   return CallNonvirtualShortMethod(env,jobject1,jclass2,jmethodID3, ...);
   }

    jshort      (*CallNonvirtualShortMethodV)(JNIEnv*, jobject, jclass,jmethodID, va_list);
    jshort       MyCallNonvirtualShortMethodV(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,va_list  va_list4) {
   LOGGER("CallNonvirtualShortMethodV\n");
   return CallNonvirtualShortMethodV(env,jobject1,jclass2,jmethodID3,va_list4);
   }

    jshort      (*CallNonvirtualShortMethodA)(JNIEnv*, jobject, jclass,jmethodID, const jvalue*);
    jshort       MyCallNonvirtualShortMethodA(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,const jvalue * const4) {
   LOGGER("CallNonvirtualShortMethodA\n");
   return CallNonvirtualShortMethodA(env,jobject1,jclass2,jmethodID3,const4);
   }

    jint        (*CallNonvirtualIntMethod)(JNIEnv*, jobject, jclass,jmethodID, ...);
    jint         MyCallNonvirtualIntMethod(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3, ...) {
   LOGGER("CallNonvirtualIntMethod\n");
   return CallNonvirtualIntMethod(env,jobject1,jclass2,jmethodID3, ...);
   }

    jint        (*CallNonvirtualIntMethodV)(JNIEnv*, jobject, jclass,jmethodID, va_list);
    jint         MyCallNonvirtualIntMethodV(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,va_list  va_list4) {
   LOGGER("CallNonvirtualIntMethodV\n");
   return CallNonvirtualIntMethodV(env,jobject1,jclass2,jmethodID3,va_list4);
   }

    jint        (*CallNonvirtualIntMethodA)(JNIEnv*, jobject, jclass,jmethodID, const jvalue*);
    jint         MyCallNonvirtualIntMethodA(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,const jvalue * const4) {
   LOGGER("CallNonvirtualIntMethodA\n");
   return CallNonvirtualIntMethodA(env,jobject1,jclass2,jmethodID3,const4);
   }

    jlong       (*CallNonvirtualLongMethod)(JNIEnv*, jobject, jclass,jmethodID, ...);
    jlong        MyCallNonvirtualLongMethod(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3, ...) {
   LOGGER("CallNonvirtualLongMethod\n");
   return CallNonvirtualLongMethod(env,jobject1,jclass2,jmethodID3, ...);
   }

    jlong       (*CallNonvirtualLongMethodV)(JNIEnv*, jobject, jclass,jmethodID, va_list);
    jlong        MyCallNonvirtualLongMethodV(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,va_list  va_list4) {
   LOGGER("CallNonvirtualLongMethodV\n");
   return CallNonvirtualLongMethodV(env,jobject1,jclass2,jmethodID3,va_list4);
   }

    jlong       (*CallNonvirtualLongMethodA)(JNIEnv*, jobject, jclass,jmethodID, const jvalue*);
    jlong        MyCallNonvirtualLongMethodA(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,const jvalue * const4) {
   LOGGER("CallNonvirtualLongMethodA\n");
   return CallNonvirtualLongMethodA(env,jobject1,jclass2,jmethodID3,const4);
   }

    jfloat      (*CallNonvirtualFloatMethod)(JNIEnv*, jobject, jclass,jmethodID, ...);
    jfloat       MyCallNonvirtualFloatMethod(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3, ...) {
   LOGGER("CallNonvirtualFloatMethod\n");
   return CallNonvirtualFloatMethod(env,jobject1,jclass2,jmethodID3, ...);
   }

    jfloat      (*CallNonvirtualFloatMethodV)(JNIEnv*, jobject, jclass,jmethodID, va_list);
    jfloat       MyCallNonvirtualFloatMethodV(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,va_list  va_list4) {
   LOGGER("CallNonvirtualFloatMethodV\n");
   return CallNonvirtualFloatMethodV(env,jobject1,jclass2,jmethodID3,va_list4);
   }

    jfloat      (*CallNonvirtualFloatMethodA)(JNIEnv*, jobject, jclass,jmethodID, const jvalue*);
    jfloat       MyCallNonvirtualFloatMethodA(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,const jvalue * const4) {
   LOGGER("CallNonvirtualFloatMethodA\n");
   return CallNonvirtualFloatMethodA(env,jobject1,jclass2,jmethodID3,const4);
   }

    jdouble     (*CallNonvirtualDoubleMethod)(JNIEnv*, jobject, jclass,jmethodID, ...);
    jdouble      MyCallNonvirtualDoubleMethod(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3, ...) {
   LOGGER("CallNonvirtualDoubleMethod\n");
   return CallNonvirtualDoubleMethod(env,jobject1,jclass2,jmethodID3, ...);
   }

    jdouble     (*CallNonvirtualDoubleMethodV)(JNIEnv*, jobject, jclass,jmethodID, va_list);
    jdouble      MyCallNonvirtualDoubleMethodV(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,va_list  va_list4) {
   LOGGER("CallNonvirtualDoubleMethodV\n");
   return CallNonvirtualDoubleMethodV(env,jobject1,jclass2,jmethodID3,va_list4);
   }

    jdouble     (*CallNonvirtualDoubleMethodA)(JNIEnv*, jobject, jclass,jmethodID, const jvalue*);
    jdouble      MyCallNonvirtualDoubleMethodA(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,const jvalue * const4) {
   LOGGER("CallNonvirtualDoubleMethodA\n");
   return CallNonvirtualDoubleMethodA(env,jobject1,jclass2,jmethodID3,const4);
   }

    void        (*CallNonvirtualVoidMethod)(JNIEnv*, jobject, jclass,jmethodID, ...);
    void         MyCallNonvirtualVoidMethod(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3, ...) {
   LOGGER("CallNonvirtualVoidMethod\n");
   return CallNonvirtualVoidMethod(env,jobject1,jclass2,jmethodID3, ...);
   }

    void        (*CallNonvirtualVoidMethodV)(JNIEnv*, jobject, jclass,jmethodID, va_list);
    void         MyCallNonvirtualVoidMethodV(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,va_list  va_list4) {
   LOGGER("CallNonvirtualVoidMethodV\n");
   return CallNonvirtualVoidMethodV(env,jobject1,jclass2,jmethodID3,va_list4);
   }

    void        (*CallNonvirtualVoidMethodA)(JNIEnv*, jobject, jclass,jmethodID, const jvalue*);
    void         MyCallNonvirtualVoidMethodA(JNIEnv*env,jobject  jobject1,jclass  jclass2,jmethodID  jmethodID3,const jvalue * const4) {
   LOGGER("CallNonvirtualVoidMethodA\n");
   return CallNonvirtualVoidMethodA(env,jobject1,jclass2,jmethodID3,const4);
   }

    jfieldID    (*GetFieldID)(JNIEnv*, jclass, const char*, const char*);
    jfieldID     MyGetFieldID(JNIEnv*env,jclass  jclass1,const char * const2,const char * const3) {
   LOGGER("GetFieldID\n");
   return GetFieldID(env,jclass1,const2,const3);
   }

    jobject     (*GetObjectField)(JNIEnv*, jobject, jfieldID);
    jobject      MyGetObjectField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2) {
   LOGGER("GetObjectField\n");
   return GetObjectField(env,jobject1,jfieldID2);
   }

    jboolean    (*GetBooleanField)(JNIEnv*, jobject, jfieldID);
    jboolean     MyGetBooleanField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2) {
   LOGGER("GetBooleanField\n");
   return GetBooleanField(env,jobject1,jfieldID2);
   }

    jbyte       (*GetByteField)(JNIEnv*, jobject, jfieldID);
    jbyte        MyGetByteField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2) {
   LOGGER("GetByteField\n");
   return GetByteField(env,jobject1,jfieldID2);
   }

    jchar       (*GetCharField)(JNIEnv*, jobject, jfieldID);
    jchar        MyGetCharField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2) {
   LOGGER("GetCharField\n");
   return GetCharField(env,jobject1,jfieldID2);
   }

    jshort      (*GetShortField)(JNIEnv*, jobject, jfieldID);
    jshort       MyGetShortField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2) {
   LOGGER("GetShortField\n");
   return GetShortField(env,jobject1,jfieldID2);
   }

    jint        (*GetIntField)(JNIEnv*, jobject, jfieldID);
    jint         MyGetIntField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2) {
   LOGGER("GetIntField\n");
   return GetIntField(env,jobject1,jfieldID2);
   }

    jlong       (*GetLongField)(JNIEnv*, jobject, jfieldID);
    jlong        MyGetLongField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2) {
   LOGGER("GetLongField\n");
   return GetLongField(env,jobject1,jfieldID2);
   }

    jfloat      (*GetFloatField)(JNIEnv*, jobject, jfieldID);
    jfloat       MyGetFloatField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2) {
   LOGGER("GetFloatField\n");
   return GetFloatField(env,jobject1,jfieldID2);
   }

    jdouble     (*GetDoubleField)(JNIEnv*, jobject, jfieldID);
    jdouble      MyGetDoubleField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2) {
   LOGGER("GetDoubleField\n");
   return GetDoubleField(env,jobject1,jfieldID2);
   }

    void        (*SetObjectField)(JNIEnv*, jobject, jfieldID, jobject);
    void         MySetObjectField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2,jobject  jobject3) {
   LOGGER("SetObjectField\n");
   return SetObjectField(env,jobject1,jfieldID2,jobject3);
   }

    void        (*SetBooleanField)(JNIEnv*, jobject, jfieldID, jboolean);
    void         MySetBooleanField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2,jboolean  jboolean3) {
   LOGGER("SetBooleanField\n");
   return SetBooleanField(env,jobject1,jfieldID2,jboolean3);
   }

    void        (*SetByteField)(JNIEnv*, jobject, jfieldID, jbyte);
    void         MySetByteField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2,jbyte  jbyte3) {
   LOGGER("SetByteField\n");
   return SetByteField(env,jobject1,jfieldID2,jbyte3);
   }

    void        (*SetCharField)(JNIEnv*, jobject, jfieldID, jchar);
    void         MySetCharField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2,jchar  jchar3) {
   LOGGER("SetCharField\n");
   return SetCharField(env,jobject1,jfieldID2,jchar3);
   }

    void        (*SetShortField)(JNIEnv*, jobject, jfieldID, jshort);
    void         MySetShortField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2,jshort  jshort3) {
   LOGGER("SetShortField\n");
   return SetShortField(env,jobject1,jfieldID2,jshort3);
   }

    void        (*SetIntField)(JNIEnv*, jobject, jfieldID, jint);
    void         MySetIntField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2,jint  jint3) {
   LOGGER("SetIntField\n");
   return SetIntField(env,jobject1,jfieldID2,jint3);
   }

    void        (*SetLongField)(JNIEnv*, jobject, jfieldID, jlong);
    void         MySetLongField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2,jlong  jlong3) {
   LOGGER("SetLongField\n");
   return SetLongField(env,jobject1,jfieldID2,jlong3);
   }

    void        (*SetFloatField)(JNIEnv*, jobject, jfieldID, jfloat);
    void         MySetFloatField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2,jfloat  jfloat3) {
   LOGGER("SetFloatField\n");
   return SetFloatField(env,jobject1,jfieldID2,jfloat3);
   }

    void        (*SetDoubleField)(JNIEnv*, jobject, jfieldID, jdouble);
    void         MySetDoubleField(JNIEnv*env,jobject  jobject1,jfieldID  jfieldID2,jdouble  jdouble3) {
   LOGGER("SetDoubleField\n");
   return SetDoubleField(env,jobject1,jfieldID2,jdouble3);
   }

    jmethodID   (*GetStaticMethodID)(JNIEnv*, jclass, const char*, const char*);
    jmethodID    MyGetStaticMethodID(JNIEnv*env,jclass  jclass1,const char * const2,const char * const3) {
   LOGGER("GetStaticMethodID\n");
   return GetStaticMethodID(env,jclass1,const2,const3);
   }

    jobject     (*CallStaticObjectMethodV)(JNIEnv*, jclass, jmethodID, va_list);
    jobject      MyCallStaticObjectMethodV(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGGER("CallStaticObjectMethodV\n");
   return CallStaticObjectMethodV(env,jclass1,jmethodID2,va_list3);
   }

    jobject     (*CallStaticObjectMethodA)(JNIEnv*, jclass, jmethodID, const jvalue*);
    jobject      MyCallStaticObjectMethodA(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGGER("CallStaticObjectMethodA\n");
   return CallStaticObjectMethodA(env,jclass1,jmethodID2,const3);
   }

    jboolean    (*CallStaticBooleanMethod)(JNIEnv*, jclass, jmethodID, ...);
    jboolean     MyCallStaticBooleanMethod(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2, ...) {
   LOGGER("CallStaticBooleanMethod\n");
   return CallStaticBooleanMethod(env,jclass1,jmethodID2, ...);
   }

    jboolean    (*CallStaticBooleanMethodV)(JNIEnv*, jclass, jmethodID,va_list);
    jboolean     MyCallStaticBooleanMethodV(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGGER("CallStaticBooleanMethodV\n");
   return CallStaticBooleanMethodV(env,jclass1,jmethodID2,va_list3);
   }

    jboolean    (*CallStaticBooleanMethodA)(JNIEnv*, jclass, jmethodID, const jvalue*);
    jboolean     MyCallStaticBooleanMethodA(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGGER("CallStaticBooleanMethodA\n");
   return CallStaticBooleanMethodA(env,jclass1,jmethodID2,const3);
   }

    jbyte       (*CallStaticByteMethod)(JNIEnv*, jclass, jmethodID, ...);
    jbyte        MyCallStaticByteMethod(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2, ...) {
   LOGGER("CallStaticByteMethod\n");
   return CallStaticByteMethod(env,jclass1,jmethodID2, ...);
   }

    jbyte       (*CallStaticByteMethodV)(JNIEnv*, jclass, jmethodID, va_list);
    jbyte        MyCallStaticByteMethodV(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGGER("CallStaticByteMethodV\n");
   return CallStaticByteMethodV(env,jclass1,jmethodID2,va_list3);
   }

    jbyte       (*CallStaticByteMethodA)(JNIEnv*, jclass, jmethodID, const jvalue*);
    jbyte        MyCallStaticByteMethodA(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGGER("CallStaticByteMethodA\n");
   return CallStaticByteMethodA(env,jclass1,jmethodID2,const3);
   }

    jchar       (*CallStaticCharMethod)(JNIEnv*, jclass, jmethodID, ...);
    jchar        MyCallStaticCharMethod(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2, ...) {
   LOGGER("CallStaticCharMethod\n");
   return CallStaticCharMethod(env,jclass1,jmethodID2, ...);
   }

    jchar       (*CallStaticCharMethodV)(JNIEnv*, jclass, jmethodID, va_list);
    jchar        MyCallStaticCharMethodV(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGGER("CallStaticCharMethodV\n");
   return CallStaticCharMethodV(env,jclass1,jmethodID2,va_list3);
   }

    jchar       (*CallStaticCharMethodA)(JNIEnv*, jclass, jmethodID, const jvalue*);
    jchar        MyCallStaticCharMethodA(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGGER("CallStaticCharMethodA\n");
   return CallStaticCharMethodA(env,jclass1,jmethodID2,const3);
   }

    jshort      (*CallStaticShortMethod)(JNIEnv*, jclass, jmethodID, ...);
    jshort       MyCallStaticShortMethod(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2, ...) {
   LOGGER("CallStaticShortMethod\n");
   return CallStaticShortMethod(env,jclass1,jmethodID2, ...);
   }

    jshort      (*CallStaticShortMethodV)(JNIEnv*, jclass, jmethodID, va_list);
    jshort       MyCallStaticShortMethodV(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGGER("CallStaticShortMethodV\n");
   return CallStaticShortMethodV(env,jclass1,jmethodID2,va_list3);
   }

    jshort      (*CallStaticShortMethodA)(JNIEnv*, jclass, jmethodID, const jvalue*);
    jshort       MyCallStaticShortMethodA(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGGER("CallStaticShortMethodA\n");
   return CallStaticShortMethodA(env,jclass1,jmethodID2,const3);
   }

    jint        (*CallStaticIntMethodV)(JNIEnv*, jclass, jmethodID, va_list);
    jint         MyCallStaticIntMethodV(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGGER("CallStaticIntMethodV\n");
   return CallStaticIntMethodV(env,jclass1,jmethodID2,va_list3);
   }

    jint        (*CallStaticIntMethodA)(JNIEnv*, jclass, jmethodID, const jvalue*);
    jint         MyCallStaticIntMethodA(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGGER("CallStaticIntMethodA\n");
   return CallStaticIntMethodA(env,jclass1,jmethodID2,const3);
   }

    jlong       (*CallStaticLongMethod)(JNIEnv*, jclass, jmethodID, ...);
    jlong        MyCallStaticLongMethod(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2, ...) {
   LOGGER("CallStaticLongMethod\n");
   return CallStaticLongMethod(env,jclass1,jmethodID2, ...);
   }

    jlong       (*CallStaticLongMethodV)(JNIEnv*, jclass, jmethodID, va_list);
    jlong        MyCallStaticLongMethodV(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGGER("CallStaticLongMethodV\n");
   return CallStaticLongMethodV(env,jclass1,jmethodID2,va_list3);
   }

    jlong       (*CallStaticLongMethodA)(JNIEnv*, jclass, jmethodID, const jvalue*);
    jlong        MyCallStaticLongMethodA(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGGER("CallStaticLongMethodA\n");
   return CallStaticLongMethodA(env,jclass1,jmethodID2,const3);
   }

    jfloat      (*CallStaticFloatMethod)(JNIEnv*, jclass, jmethodID, ...);
    jfloat       MyCallStaticFloatMethod(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2, ...) {
   LOGGER("CallStaticFloatMethod\n");
   return CallStaticFloatMethod(env,jclass1,jmethodID2, ...);
   }

    jfloat      (*CallStaticFloatMethodV)(JNIEnv*, jclass, jmethodID, va_list);
    jfloat       MyCallStaticFloatMethodV(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGGER("CallStaticFloatMethodV\n");
   return CallStaticFloatMethodV(env,jclass1,jmethodID2,va_list3);
   }

    jfloat      (*CallStaticFloatMethodA)(JNIEnv*, jclass, jmethodID, const jvalue*);
    jfloat       MyCallStaticFloatMethodA(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGGER("CallStaticFloatMethodA\n");
   return CallStaticFloatMethodA(env,jclass1,jmethodID2,const3);
   }

    jdouble     (*CallStaticDoubleMethod)(JNIEnv*, jclass, jmethodID, ...);
    jdouble      MyCallStaticDoubleMethod(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2, ...) {
   LOGGER("CallStaticDoubleMethod\n");
   return CallStaticDoubleMethod(env,jclass1,jmethodID2, ...);
   }

    jdouble     (*CallStaticDoubleMethodV)(JNIEnv*, jclass, jmethodID, va_list);
    jdouble      MyCallStaticDoubleMethodV(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGGER("CallStaticDoubleMethodV\n");
   return CallStaticDoubleMethodV(env,jclass1,jmethodID2,va_list3);
   }

    jdouble     (*CallStaticDoubleMethodA)(JNIEnv*, jclass, jmethodID, const jvalue*);
    jdouble      MyCallStaticDoubleMethodA(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGGER("CallStaticDoubleMethodA\n");
   return CallStaticDoubleMethodA(env,jclass1,jmethodID2,const3);
   }

    void        (*CallStaticVoidMethod)(JNIEnv*, jclass, jmethodID, ...);
    void         MyCallStaticVoidMethod(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2, ...) {
   LOGGER("CallStaticVoidMethod\n");
   return CallStaticVoidMethod(env,jclass1,jmethodID2, ...);
   }

    void        (*CallStaticVoidMethodV)(JNIEnv*, jclass, jmethodID, va_list);
    void         MyCallStaticVoidMethodV(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,va_list  va_list3) {
   LOGGER("CallStaticVoidMethodV\n");
   return CallStaticVoidMethodV(env,jclass1,jmethodID2,va_list3);
   }

    void        (*CallStaticVoidMethodA)(JNIEnv*, jclass, jmethodID, const jvalue*);
    void         MyCallStaticVoidMethodA(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,const jvalue * const3) {
   LOGGER("CallStaticVoidMethodA\n");
   return CallStaticVoidMethodA(env,jclass1,jmethodID2,const3);
   }

    jfieldID    (*GetStaticFieldID)(JNIEnv*, jclass, const char*,const char*);
    jfieldID     MyGetStaticFieldID(JNIEnv*env,jclass  jclass1,const char * const2,const char * const3) {
   LOGGER("GetStaticFieldID\n");
   return GetStaticFieldID(env,jclass1,const2,const3);
   }

    jobject     (*GetStaticObjectField)(JNIEnv*, jclass, jfieldID);
    jobject      MyGetStaticObjectField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2) {
   LOGGER("GetStaticObjectField\n");
   return GetStaticObjectField(env,jclass1,jfieldID2);
   }

    jboolean    (*GetStaticBooleanField)(JNIEnv*, jclass, jfieldID);
    jboolean     MyGetStaticBooleanField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2) {
   LOGGER("GetStaticBooleanField\n");
   return GetStaticBooleanField(env,jclass1,jfieldID2);
   }

    jbyte       (*GetStaticByteField)(JNIEnv*, jclass, jfieldID);
    jbyte        MyGetStaticByteField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2) {
   LOGGER("GetStaticByteField\n");
   return GetStaticByteField(env,jclass1,jfieldID2);
   }

    jchar       (*GetStaticCharField)(JNIEnv*, jclass, jfieldID);
    jchar        MyGetStaticCharField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2) {
   LOGGER("GetStaticCharField\n");
   return GetStaticCharField(env,jclass1,jfieldID2);
   }

    jshort      (*GetStaticShortField)(JNIEnv*, jclass, jfieldID);
    jshort       MyGetStaticShortField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2) {
   LOGGER("GetStaticShortField\n");
   return GetStaticShortField(env,jclass1,jfieldID2);
   }

    jint        (*GetStaticIntField)(JNIEnv*, jclass, jfieldID);
    jint         MyGetStaticIntField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2) {
   LOGGER("GetStaticIntField\n");
   return GetStaticIntField(env,jclass1,jfieldID2);
   }

    jlong       (*GetStaticLongField)(JNIEnv*, jclass, jfieldID);
    jlong        MyGetStaticLongField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2) {
   LOGGER("GetStaticLongField\n");
   return GetStaticLongField(env,jclass1,jfieldID2);
   }

    jfloat      (*GetStaticFloatField)(JNIEnv*, jclass, jfieldID);
    jfloat       MyGetStaticFloatField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2) {
   LOGGER("GetStaticFloatField\n");
   return GetStaticFloatField(env,jclass1,jfieldID2);
   }

    jdouble     (*GetStaticDoubleField)(JNIEnv*, jclass, jfieldID);
    jdouble      MyGetStaticDoubleField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2) {
   LOGGER("GetStaticDoubleField\n");
   return GetStaticDoubleField(env,jclass1,jfieldID2);
   }

    void        (*SetStaticObjectField)(JNIEnv*, jclass, jfieldID, jobject);
    void         MySetStaticObjectField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2,jobject  jobject3) {
   LOGGER("SetStaticObjectField\n");
   return SetStaticObjectField(env,jclass1,jfieldID2,jobject3);
   }

    void        (*SetStaticBooleanField)(JNIEnv*, jclass, jfieldID, jboolean);
    void         MySetStaticBooleanField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2,jboolean  jboolean3) {
   LOGGER("SetStaticBooleanField\n");
   return SetStaticBooleanField(env,jclass1,jfieldID2,jboolean3);
   }

    void        (*SetStaticByteField)(JNIEnv*, jclass, jfieldID, jbyte);
    void         MySetStaticByteField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2,jbyte  jbyte3) {
   LOGGER("SetStaticByteField\n");
   return SetStaticByteField(env,jclass1,jfieldID2,jbyte3);
   }

    void        (*SetStaticCharField)(JNIEnv*, jclass, jfieldID, jchar);
    void         MySetStaticCharField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2,jchar  jchar3) {
   LOGGER("SetStaticCharField\n");
   return SetStaticCharField(env,jclass1,jfieldID2,jchar3);
   }

    void        (*SetStaticShortField)(JNIEnv*, jclass, jfieldID, jshort);
    void         MySetStaticShortField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2,jshort  jshort3) {
   LOGGER("SetStaticShortField\n");
   return SetStaticShortField(env,jclass1,jfieldID2,jshort3);
   }

    void        (*SetStaticIntField)(JNIEnv*, jclass, jfieldID, jint);
    void         MySetStaticIntField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2,jint  jint3) {
   LOGGER("SetStaticIntField\n");
   return SetStaticIntField(env,jclass1,jfieldID2,jint3);
   }

    void        (*SetStaticLongField)(JNIEnv*, jclass, jfieldID, jlong);
    void         MySetStaticLongField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2,jlong  jlong3) {
   LOGGER("SetStaticLongField\n");
   return SetStaticLongField(env,jclass1,jfieldID2,jlong3);
   }

    void        (*SetStaticFloatField)(JNIEnv*, jclass, jfieldID, jfloat);
    void         MySetStaticFloatField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2,jfloat  jfloat3) {
   LOGGER("SetStaticFloatField\n");
   return SetStaticFloatField(env,jclass1,jfieldID2,jfloat3);
   }

    void        (*SetStaticDoubleField)(JNIEnv*, jclass, jfieldID, jdouble);
    void         MySetStaticDoubleField(JNIEnv*env,jclass  jclass1,jfieldID  jfieldID2,jdouble  jdouble3) {
   LOGGER("SetStaticDoubleField\n");
   return SetStaticDoubleField(env,jclass1,jfieldID2,jdouble3);
   }

    jstring     (*NewString)(JNIEnv*, const jchar*, jsize);
    jstring      MyNewString(JNIEnv*env,const jchar * const1,jsize  jsize2) {
   LOGGER("NewString\n");
   return NewString(env,const1,jsize2);
   }

    jsize       (*GetStringLength)(JNIEnv*, jstring);
    jsize        MyGetStringLength(JNIEnv*env,jstring  jstring1) {
   LOGGER("GetStringLength\n");
   return GetStringLength(env,jstring1);
   }

    const jchar* (*GetStringChars)(JNIEnv*, jstring, jboolean*);
    const jchar*  MyGetStringChars(JNIEnv*env,jstring  jstring1,jboolean * jboolean2) {
   LOGGER("GetStringChars\n");
   return GetStringChars(env,jstring1,jboolean2);
   }

    void        (*ReleaseStringChars)(JNIEnv*, jstring, const jchar*);
    void         MyReleaseStringChars(JNIEnv*env,jstring  jstring1,const jchar * const2) {
   LOGGER("ReleaseStringChars\n");
   return ReleaseStringChars(env,jstring1,const2);
   }

    jstring     (*NewStringUTF)(JNIEnv*, const char*);
    jstring      MyNewStringUTF(JNIEnv*env,const char * const1) {
   LOGGER("NewStringUTF\n");
   return NewStringUTF(env,const1);
   }

    jsize       (*GetStringUTFLength)(JNIEnv*, jstring);
    jsize        MyGetStringUTFLength(JNIEnv*env,jstring  jstring1) {
   LOGGER("GetStringUTFLength\n");
   return GetStringUTFLength(env,jstring1);
   }

    const char* (*GetStringUTFChars)(JNIEnv*, jstring, jboolean*);
    const char*  MyGetStringUTFChars(JNIEnv*env,jstring  jstring1,jboolean * jboolean2) {
   LOGGER("GetStringUTFChars\n");
   return GetStringUTFChars(env,jstring1,jboolean2);
   }

    void        (*ReleaseStringUTFChars)(JNIEnv*, jstring, const char*);
    void         MyReleaseStringUTFChars(JNIEnv*env,jstring  jstring1,const char * const2) {
   LOGGER("ReleaseStringUTFChars\n");
   return ReleaseStringUTFChars(env,jstring1,const2);
   }

    jobjectArray (*NewObjectArray)(JNIEnv*, jsize, jclass, jobject);
    jobjectArray  MyNewObjectArray(JNIEnv*env,jsize  jsize1,jclass  jclass2,jobject  jobject3) {
   LOGGER("NewObjectArray\n");
   return NewObjectArray(env,jsize1,jclass2,jobject3);
   }

    jobject     (*GetObjectArrayElement)(JNIEnv*, jobjectArray, jsize);
    jobject      MyGetObjectArrayElement(JNIEnv*env,jobjectArray  jobjectArray1,jsize  jsize2) {
   LOGGER("GetObjectArrayElement\n");
   return GetObjectArrayElement(env,jobjectArray1,jsize2);
   }

    void        (*SetObjectArrayElement)(JNIEnv*, jobjectArray, jsize, jobject);
    void         MySetObjectArrayElement(JNIEnv*env,jobjectArray  jobjectArray1,jsize  jsize2,jobject  jobject3) {
   LOGGER("SetObjectArrayElement\n");
   return SetObjectArrayElement(env,jobjectArray1,jsize2,jobject3);
   }

    jbooleanArray (*NewBooleanArray)(JNIEnv*, jsize);
    jbooleanArray  MyNewBooleanArray(JNIEnv*env,jsize  jsize1) {
   LOGGER("NewBooleanArray\n");
   return NewBooleanArray(env,jsize1);
   }

    jcharArray    (*NewCharArray)(JNIEnv*, jsize);
    jcharArray     MyNewCharArray(JNIEnv*env,jsize  jsize1) {
   LOGGER("NewCharArray\n");
   return NewCharArray(env,jsize1);
   }

    jshortArray   (*NewShortArray)(JNIEnv*, jsize);
    jshortArray    MyNewShortArray(JNIEnv*env,jsize  jsize1) {
   LOGGER("NewShortArray\n");
   return NewShortArray(env,jsize1);
   }

    jintArray     (*NewIntArray)(JNIEnv*, jsize);
    jintArray      MyNewIntArray(JNIEnv*env,jsize  jsize1) {
   LOGGER("NewIntArray\n");
   return NewIntArray(env,jsize1);
   }

    jlongArray    (*NewLongArray)(JNIEnv*, jsize);
    jlongArray     MyNewLongArray(JNIEnv*env,jsize  jsize1) {
   LOGGER("NewLongArray\n");
   return NewLongArray(env,jsize1);
   }

    jfloatArray   (*NewFloatArray)(JNIEnv*, jsize);
    jfloatArray    MyNewFloatArray(JNIEnv*env,jsize  jsize1) {
   LOGGER("NewFloatArray\n");
   return NewFloatArray(env,jsize1);
   }

    jdoubleArray  (*NewDoubleArray)(JNIEnv*, jsize);
    jdoubleArray   MyNewDoubleArray(JNIEnv*env,jsize  jsize1) {
   LOGGER("NewDoubleArray\n");
   return NewDoubleArray(env,jsize1);
   }

    jboolean*   (*GetBooleanArrayElements)(JNIEnv*, jbooleanArray, jboolean*);
    jboolean*    MyGetBooleanArrayElements(JNIEnv*env,jbooleanArray  jbooleanArray1,jboolean * jboolean2) {
   LOGGER("GetBooleanArrayElements\n");
   return GetBooleanArrayElements(env,jbooleanArray1,jboolean2);
   }

    jchar*      (*GetCharArrayElements)(JNIEnv*, jcharArray, jboolean*);
    jchar*       MyGetCharArrayElements(JNIEnv*env,jcharArray  jcharArray1,jboolean * jboolean2) {
   LOGGER("GetCharArrayElements\n");
   return GetCharArrayElements(env,jcharArray1,jboolean2);
   }

    jshort*     (*GetShortArrayElements)(JNIEnv*, jshortArray, jboolean*);
    jshort*      MyGetShortArrayElements(JNIEnv*env,jshortArray  jshortArray1,jboolean * jboolean2) {
   LOGGER("GetShortArrayElements\n");
   return GetShortArrayElements(env,jshortArray1,jboolean2);
   }

    jint*       (*GetIntArrayElements)(JNIEnv*, jintArray, jboolean*);
    jint*        MyGetIntArrayElements(JNIEnv*env,jintArray  jintArray1,jboolean * jboolean2) {
   LOGGER("GetIntArrayElements\n");
   return GetIntArrayElements(env,jintArray1,jboolean2);
   }

    jlong*      (*GetLongArrayElements)(JNIEnv*, jlongArray, jboolean*);
    jlong*       MyGetLongArrayElements(JNIEnv*env,jlongArray  jlongArray1,jboolean * jboolean2) {
   LOGGER("GetLongArrayElements\n");
   return GetLongArrayElements(env,jlongArray1,jboolean2);
   }

    jfloat*     (*GetFloatArrayElements)(JNIEnv*, jfloatArray, jboolean*);
    jfloat*      MyGetFloatArrayElements(JNIEnv*env,jfloatArray  jfloatArray1,jboolean * jboolean2) {
   LOGGER("GetFloatArrayElements\n");
   return GetFloatArrayElements(env,jfloatArray1,jboolean2);
   }

    jdouble*    (*GetDoubleArrayElements)(JNIEnv*, jdoubleArray, jboolean*);
    jdouble*     MyGetDoubleArrayElements(JNIEnv*env,jdoubleArray  jdoubleArray1,jboolean * jboolean2) {
   LOGGER("GetDoubleArrayElements\n");
   return GetDoubleArrayElements(env,jdoubleArray1,jboolean2);
   }

    void        (*ReleaseBooleanArrayElements)(JNIEnv*, jbooleanArray,jboolean*, jint);
    void         MyReleaseBooleanArrayElements(JNIEnv*env,jbooleanArray  jbooleanArray1,jboolean * jboolean2,jint  jint3) {
   LOGGER("ReleaseBooleanArrayElements\n");
   return ReleaseBooleanArrayElements(env,jbooleanArray1,jboolean2,jint3);
   }

    void        (*ReleaseCharArrayElements)(JNIEnv*, jcharArray,jchar*, jint);
    void         MyReleaseCharArrayElements(JNIEnv*env,jcharArray  jcharArray1,jchar * jchar2,jint  jint3) {
   LOGGER("ReleaseCharArrayElements\n");
   return ReleaseCharArrayElements(env,jcharArray1,jchar2,jint3);
   }

    void        (*ReleaseShortArrayElements)(JNIEnv*, jshortArray,jshort*, jint);
    void         MyReleaseShortArrayElements(JNIEnv*env,jshortArray  jshortArray1,jshort * jshort2,jint  jint3) {
   LOGGER("ReleaseShortArrayElements\n");
   return ReleaseShortArrayElements(env,jshortArray1,jshort2,jint3);
   }

    void        (*ReleaseIntArrayElements)(JNIEnv*, jintArray,jint*, jint);
    void         MyReleaseIntArrayElements(JNIEnv*env,jintArray  jintArray1,jint * jint2,jint  jint3) {
   LOGGER("ReleaseIntArrayElements\n");
   return ReleaseIntArrayElements(env,jintArray1,jint2,jint3);
   }

    void        (*ReleaseLongArrayElements)(JNIEnv*, jlongArray,jlong*, jint);
    void         MyReleaseLongArrayElements(JNIEnv*env,jlongArray  jlongArray1,jlong * jlong2,jint  jint3) {
   LOGGER("ReleaseLongArrayElements\n");
   return ReleaseLongArrayElements(env,jlongArray1,jlong2,jint3);
   }

    void        (*ReleaseFloatArrayElements)(JNIEnv*, jfloatArray,jfloat*, jint);
    void         MyReleaseFloatArrayElements(JNIEnv*env,jfloatArray  jfloatArray1,jfloat * jfloat2,jint  jint3) {
   LOGGER("ReleaseFloatArrayElements\n");
   return ReleaseFloatArrayElements(env,jfloatArray1,jfloat2,jint3);
   }

    void        (*ReleaseDoubleArrayElements)(JNIEnv*, jdoubleArray,jdouble*, jint);
    void         MyReleaseDoubleArrayElements(JNIEnv*env,jdoubleArray  jdoubleArray1,jdouble * jdouble2,jint  jint3) {
   LOGGER("ReleaseDoubleArrayElements\n");
   return ReleaseDoubleArrayElements(env,jdoubleArray1,jdouble2,jint3);
   }

    void        (*GetBooleanArrayRegion)(JNIEnv*, jbooleanArray,jsize, jsize, jboolean*);
    void         MyGetBooleanArrayRegion(JNIEnv*env,jbooleanArray  jbooleanArray1,jsize  jsize2,jsize  jsize3,jboolean * jboolean4) {
   LOGGER("GetBooleanArrayRegion\n");
   return GetBooleanArrayRegion(env,jbooleanArray1,jsize2,jsize3,jboolean4);
   }

    void        (*GetByteArrayRegion)(JNIEnv*, jbyteArray,jsize, jsize, jbyte*);
    void         MyGetByteArrayRegion(JNIEnv*env,jbyteArray  jbyteArray1,jsize  jsize2,jsize  jsize3,jbyte * jbyte4) {
   LOGGER("GetByteArrayRegion\n");
   return GetByteArrayRegion(env,jbyteArray1,jsize2,jsize3,jbyte4);
   }

    void        (*GetCharArrayRegion)(JNIEnv*, jcharArray,jsize, jsize, jchar*);
    void         MyGetCharArrayRegion(JNIEnv*env,jcharArray  jcharArray1,jsize  jsize2,jsize  jsize3,jchar * jchar4) {
   LOGGER("GetCharArrayRegion\n");
   return GetCharArrayRegion(env,jcharArray1,jsize2,jsize3,jchar4);
   }

    void        (*GetShortArrayRegion)(JNIEnv*, jshortArray,jsize, jsize, jshort*);
    void         MyGetShortArrayRegion(JNIEnv*env,jshortArray  jshortArray1,jsize  jsize2,jsize  jsize3,jshort * jshort4) {
   LOGGER("GetShortArrayRegion\n");
   return GetShortArrayRegion(env,jshortArray1,jsize2,jsize3,jshort4);
   }

    void        (*GetIntArrayRegion)(JNIEnv*, jintArray,jsize, jsize, jint*);
    void         MyGetIntArrayRegion(JNIEnv*env,jintArray  jintArray1,jsize  jsize2,jsize  jsize3,jint * jint4) {
   LOGGER("GetIntArrayRegion\n");
   return GetIntArrayRegion(env,jintArray1,jsize2,jsize3,jint4);
   }

    void        (*GetLongArrayRegion)(JNIEnv*, jlongArray,jsize, jsize, jlong*);
    void         MyGetLongArrayRegion(JNIEnv*env,jlongArray  jlongArray1,jsize  jsize2,jsize  jsize3,jlong * jlong4) {
   LOGGER("GetLongArrayRegion\n");
   return GetLongArrayRegion(env,jlongArray1,jsize2,jsize3,jlong4);
   }

    void        (*GetFloatArrayRegion)(JNIEnv*, jfloatArray,jsize, jsize, jfloat*);
    void         MyGetFloatArrayRegion(JNIEnv*env,jfloatArray  jfloatArray1,jsize  jsize2,jsize  jsize3,jfloat * jfloat4) {
   LOGGER("GetFloatArrayRegion\n");
   return GetFloatArrayRegion(env,jfloatArray1,jsize2,jsize3,jfloat4);
   }

    void        (*GetDoubleArrayRegion)(JNIEnv*, jdoubleArray,jsize, jsize, jdouble*);
    void         MyGetDoubleArrayRegion(JNIEnv*env,jdoubleArray  jdoubleArray1,jsize  jsize2,jsize  jsize3,jdouble * jdouble4) {
   LOGGER("GetDoubleArrayRegion\n");
   return GetDoubleArrayRegion(env,jdoubleArray1,jsize2,jsize3,jdouble4);
   }

    void        (*SetBooleanArrayRegion)(JNIEnv*, jbooleanArray,jsize, jsize, const jboolean*);
    void         MySetBooleanArrayRegion(JNIEnv*env,jbooleanArray  jbooleanArray1,jsize  jsize2,jsize  jsize3,const jboolean * const4) {
   LOGGER("SetBooleanArrayRegion\n");
   return SetBooleanArrayRegion(env,jbooleanArray1,jsize2,jsize3,const4);
   }

    void        (*SetByteArrayRegion)(JNIEnv*, jbyteArray,jsize, jsize, const jbyte*);
    void         MySetByteArrayRegion(JNIEnv*env,jbyteArray  jbyteArray1,jsize  jsize2,jsize  jsize3,const jbyte * const4) {
   LOGGER("SetByteArrayRegion\n");
   return SetByteArrayRegion(env,jbyteArray1,jsize2,jsize3,const4);
   }

    void        (*SetCharArrayRegion)(JNIEnv*, jcharArray,jsize, jsize, const jchar*);
    void         MySetCharArrayRegion(JNIEnv*env,jcharArray  jcharArray1,jsize  jsize2,jsize  jsize3,const jchar * const4) {
   LOGGER("SetCharArrayRegion\n");
   return SetCharArrayRegion(env,jcharArray1,jsize2,jsize3,const4);
   }

    void        (*SetShortArrayRegion)(JNIEnv*, jshortArray,jsize, jsize, const jshort*);
    void         MySetShortArrayRegion(JNIEnv*env,jshortArray  jshortArray1,jsize  jsize2,jsize  jsize3,const jshort * const4) {
   LOGGER("SetShortArrayRegion\n");
   return SetShortArrayRegion(env,jshortArray1,jsize2,jsize3,const4);
   }

    void        (*SetIntArrayRegion)(JNIEnv*, jintArray,jsize, jsize, const jint*);
    void         MySetIntArrayRegion(JNIEnv*env,jintArray  jintArray1,jsize  jsize2,jsize  jsize3,const jint * const4) {
   LOGGER("SetIntArrayRegion\n");
   return SetIntArrayRegion(env,jintArray1,jsize2,jsize3,const4);
   }

    void        (*SetLongArrayRegion)(JNIEnv*, jlongArray,jsize, jsize, const jlong*);
    void         MySetLongArrayRegion(JNIEnv*env,jlongArray  jlongArray1,jsize  jsize2,jsize  jsize3,const jlong * const4) {
   LOGGER("SetLongArrayRegion\n");
   return SetLongArrayRegion(env,jlongArray1,jsize2,jsize3,const4);
   }

    void        (*SetFloatArrayRegion)(JNIEnv*, jfloatArray,jsize, jsize, const jfloat*);
    void         MySetFloatArrayRegion(JNIEnv*env,jfloatArray  jfloatArray1,jsize  jsize2,jsize  jsize3,const jfloat * const4) {
   LOGGER("SetFloatArrayRegion\n");
   return SetFloatArrayRegion(env,jfloatArray1,jsize2,jsize3,const4);
   }

    void        (*SetDoubleArrayRegion)(JNIEnv*, jdoubleArray,jsize, jsize, const jdouble*);
    void         MySetDoubleArrayRegion(JNIEnv*env,jdoubleArray  jdoubleArray1,jsize  jsize2,jsize  jsize3,const jdouble * const4) {
   LOGGER("SetDoubleArrayRegion\n");
   return SetDoubleArrayRegion(env,jdoubleArray1,jsize2,jsize3,const4);
   }

    jint        (*RegisterNatives)(JNIEnv*, jclass, const JNINativeMethod*,jint);
    jint         MyRegisterNatives(JNIEnv*env,jclass  jclass1,const JNINativeMethod * const2,jint  jint3) {
   LOGGER("RegisterNatives\n");
   return RegisterNatives(env,jclass1,const2,jint3);
   }

    jint        (*UnregisterNatives)(JNIEnv*, jclass);
    jint         MyUnregisterNatives(JNIEnv*env,jclass  jclass1) {
   LOGGER("UnregisterNatives\n");
   return UnregisterNatives(env,jclass1);
   }

    jint        (*MonitorEnter)(JNIEnv*, jobject);
    jint         MyMonitorEnter(JNIEnv*env,jobject  jobject1) {
   LOGGER("MonitorEnter\n");
   return MonitorEnter(env,jobject1);
   }

    jint        (*MonitorExit)(JNIEnv*, jobject);
    jint         MyMonitorExit(JNIEnv*env,jobject  jobject1) {
   LOGGER("MonitorExit\n");
   return MonitorExit(env,jobject1);
   }

    jint        (*GetJavaVM)(JNIEnv*, JavaVM**);
    jint         MyGetJavaVM(JNIEnv*env,JavaVM ** JavaVM1) {
   LOGGER("GetJavaVM\n");
   return GetJavaVM(env,JavaVM1);
   }

    void        (*GetStringRegion)(JNIEnv*, jstring, jsize, jsize, jchar*);
    void         MyGetStringRegion(JNIEnv*env,jstring  jstring1,jsize  jsize2,jsize  jsize3,jchar * jchar4) {
   LOGGER("GetStringRegion\n");
   return GetStringRegion(env,jstring1,jsize2,jsize3,jchar4);
   }

    void        (*GetStringUTFRegion)(JNIEnv*, jstring, jsize, jsize, char*);
    void         MyGetStringUTFRegion(JNIEnv*env,jstring  jstring1,jsize  jsize2,jsize  jsize3,char * char4) {
   LOGGER("GetStringUTFRegion\n");
   return GetStringUTFRegion(env,jstring1,jsize2,jsize3,char4);
   }

    void*       (*GetPrimitiveArrayCritical)(JNIEnv*, jarray, jboolean*);
    void*        MyGetPrimitiveArrayCritical(JNIEnv*env,jarray  jarray1,jboolean * jboolean2) {
   LOGGER("GetPrimitiveArrayCritical\n");
   return GetPrimitiveArrayCritical(env,jarray1,jboolean2);
   }

    void        (*ReleasePrimitiveArrayCritical)(JNIEnv*, jarray, void*, jint);
    void         MyReleasePrimitiveArrayCritical(JNIEnv*env,jarray  jarray1,void * void2,jint  jint3) {
   LOGGER("ReleasePrimitiveArrayCritical\n");
   return ReleasePrimitiveArrayCritical(env,jarray1,void2,jint3);
   }

    const jchar* (*GetStringCritical)(JNIEnv*, jstring, jboolean*);
    const jchar*  MyGetStringCritical(JNIEnv*env,jstring  jstring1,jboolean * jboolean2) {
   LOGGER("GetStringCritical\n");
   return GetStringCritical(env,jstring1,jboolean2);
   }

    void        (*ReleaseStringCritical)(JNIEnv*, jstring, const jchar*);
    void         MyReleaseStringCritical(JNIEnv*env,jstring  jstring1,const jchar * const2) {
   LOGGER("ReleaseStringCritical\n");
   return ReleaseStringCritical(env,jstring1,const2);
   }

    jweak       (*NewWeakGlobalRef)(JNIEnv*, jobject);
    jweak        MyNewWeakGlobalRef(JNIEnv*env,jobject  jobject1) {
   LOGGER("NewWeakGlobalRef\n");
   return NewWeakGlobalRef(env,jobject1);
   }

    void        (*DeleteWeakGlobalRef)(JNIEnv*, jweak);
    void         MyDeleteWeakGlobalRef(JNIEnv*env,jweak  jweak1) {
   LOGGER("DeleteWeakGlobalRef\n");
   return DeleteWeakGlobalRef(env,jweak1);
   }

    jboolean    (*ExceptionCheck)(JNIEnv*);
    jboolean     MyExceptionCheck(JNIEnv*env) {
   LOGGER("ExceptionCheck\n");
   return ExceptionCheck(env);
   }

    jobject     (*NewDirectByteBuffer)(JNIEnv*, void*, jlong);
    jobject      MyNewDirectByteBuffer(JNIEnv*env,void * void1,jlong  jlong2) {
   LOGGER("NewDirectByteBuffer\n");
   return NewDirectByteBuffer(env,void1,jlong2);
   }

    void*       (*GetDirectBufferAddress)(JNIEnv*, jobject);
    void*        MyGetDirectBufferAddress(JNIEnv*env,jobject  jobject1) {
   LOGGER("GetDirectBufferAddress\n");
   return GetDirectBufferAddress(env,jobject1);
   }

    jlong       (*GetDirectBufferCapacity)(JNIEnv*, jobject);
    jlong        MyGetDirectBufferCapacity(JNIEnv*env,jobject  jobject1) {
   LOGGER("GetDirectBufferCapacity\n");
   return GetDirectBufferCapacity(env,jobject1);
   }

    jobjectRefType (*GetObjectRefType)(JNIEnv*, jobject);
    jobjectRefType  MyGetObjectRefType(JNIEnv*env,jobject  jobject1) {
   LOGGER("GetObjectRefType\n");
   return GetObjectRefType(env,jobject1);
   }

