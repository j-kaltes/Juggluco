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
/*      Fri Jan 27 12:35:09 CET 2023                                                 */


    jint        (*GetVersion)(JNIEnv *);
    jint         MyGetVersiondeclarefunc() {
   LOGSTRING("GetVersion\n");
   return GetVersioncallfunc();
   }

    jclass      (*DefineClass)(JNIEnv*, const char*, jobject, const jbyte*,jsize);
    jclass       MyDefineClassdeclarefunc( const char*, jobject, const jbyte*,jsize) {
   LOGSTRING("DefineClass\n");
   return DefineClasscallfunc( const char*, jobject, const jbyte*,jsize);
   }

    jmethodID   (*FromReflectedMethod)(JNIEnv*, jobject);
    jmethodID    MyFromReflectedMethoddeclarefunc( jobject) {
   LOGSTRING("FromReflectedMethod\n");
   return FromReflectedMethodcallfunc( jobject);
   }

    jfieldID    (*FromReflectedField)(JNIEnv*, jobject);
    jfieldID     MyFromReflectedFielddeclarefunc( jobject) {
   LOGSTRING("FromReflectedField\n");
   return FromReflectedFieldcallfunc( jobject);
   }

    jobject     (*ToReflectedMethod)(JNIEnv*, jclass, jmethodID, jboolean);
    jobject      MyToReflectedMethoddeclarefunc( jclass, jmethodID, jboolean) {
   LOGSTRING("ToReflectedMethod\n");
   return ToReflectedMethodcallfunc( jclass, jmethodID, jboolean);
   }

    jclass      (*GetSuperclass)(JNIEnv*, jclass);
    jclass       MyGetSuperclassdeclarefunc( jclass) {
   LOGSTRING("GetSuperclass\n");
   return GetSuperclasscallfunc( jclass);
   }

    jboolean    (*IsAssignableFrom)(JNIEnv*, jclass, jclass);
    jboolean     MyIsAssignableFromdeclarefunc( jclass, jclass) {
   LOGSTRING("IsAssignableFrom\n");
   return IsAssignableFromcallfunc( jclass, jclass);
   }

    jobject     (*ToReflectedField)(JNIEnv*, jclass, jfieldID, jboolean);
    jobject      MyToReflectedFielddeclarefunc( jclass, jfieldID, jboolean) {
   LOGSTRING("ToReflectedField\n");
   return ToReflectedFieldcallfunc( jclass, jfieldID, jboolean);
   }

    jint        (*Throw)(JNIEnv*, jthrowable);
    jint         MyThrowdeclarefunc( jthrowable) {
   LOGSTRING("Throw\n");
   return Throwcallfunc( jthrowable);
   }

    jint        (*ThrowNew)(JNIEnv *, jclass, const char *);
    jint         MyThrowNewdeclarefunc( jclass, const char *) {
   LOGSTRING("ThrowNew\n");
   return ThrowNewcallfunc( jclass, const char *);
   }

    jthrowable  (*ExceptionOccurred)(JNIEnv*);
    jthrowable   MyExceptionOccurreddeclarefunc() {
   LOGSTRING("ExceptionOccurred\n");
   return ExceptionOccurredcallfunc();
   }

    void        (*ExceptionDescribe)(JNIEnv*);
    void         MyExceptionDescribedeclarefunc() {
   LOGSTRING("ExceptionDescribe\n");
   return ExceptionDescribecallfunc();
   }

    void        (*ExceptionClear)(JNIEnv*);
    void         MyExceptionCleardeclarefunc() {
   LOGSTRING("ExceptionClear\n");
   return ExceptionClearcallfunc();
   }

    void        (*FatalError)(JNIEnv*, const char*);
    void         MyFatalErrordeclarefunc( const char*) {
   LOGSTRING("FatalError\n");
   return FatalErrorcallfunc( const char*);
   }

    jint        (*PushLocalFrame)(JNIEnv*, jint);
    jint         MyPushLocalFramedeclarefunc( jint) {
   LOGSTRING("PushLocalFrame\n");
   return PushLocalFramecallfunc( jint);
   }

    jobject     (*PopLocalFrame)(JNIEnv*, jobject);
    jobject      MyPopLocalFramedeclarefunc( jobject) {
   LOGSTRING("PopLocalFrame\n");
   return PopLocalFramecallfunc( jobject);
   }

    jobject     (*NewGlobalRef)(JNIEnv*, jobject);
    jobject      MyNewGlobalRefdeclarefunc( jobject) {
   LOGSTRING("NewGlobalRef\n");
   return NewGlobalRefcallfunc( jobject);
   }

    void        (*DeleteGlobalRef)(JNIEnv*, jobject);
    void         MyDeleteGlobalRefdeclarefunc( jobject) {
   LOGSTRING("DeleteGlobalRef\n");
   return DeleteGlobalRefcallfunc( jobject);
   }

    jboolean    (*IsSameObject)(JNIEnv*, jobject, jobject);
    jboolean     MyIsSameObjectdeclarefunc( jobject, jobject) {
   LOGSTRING("IsSameObject\n");
   return IsSameObjectcallfunc( jobject, jobject);
   }

    jobject     (*NewLocalRef)(JNIEnv*, jobject);
    jobject      MyNewLocalRefdeclarefunc( jobject) {
   LOGSTRING("NewLocalRef\n");
   return NewLocalRefcallfunc( jobject);
   }

    jint        (*EnsureLocalCapacity)(JNIEnv*, jint);
    jint         MyEnsureLocalCapacitydeclarefunc( jint) {
   LOGSTRING("EnsureLocalCapacity\n");
   return EnsureLocalCapacitycallfunc( jint);
   }

    jobject     (*AllocObject)(JNIEnv*, jclass);
    jobject      MyAllocObjectdeclarefunc( jclass) {
   LOGSTRING("AllocObject\n");
   return AllocObjectcallfunc( jclass);
   }

    jobject     (*NewObject)(JNIEnv*, jclass, jmethodID, ...);
    jobject      MyNewObjectdeclarefunc( jclass, jmethodID, ...) {
   LOGSTRING("NewObject\n");
   return NewObjectcallfunc( jclass, jmethodID, ...);
   }

    jobject     (*NewObjectV)(JNIEnv*, jclass, jmethodID, va_list);
    jobject      MyNewObjectVdeclarefunc( jclass, jmethodID, va_list) {
   LOGSTRING("NewObjectV\n");
   return NewObjectVcallfunc( jclass, jmethodID, va_list);
   }

    jobject     (*NewObjectA)(JNIEnv*, jclass, jmethodID, const jvalue*);
    jobject      MyNewObjectAdeclarefunc( jclass, jmethodID, const jvalue*) {
   LOGSTRING("NewObjectA\n");
   return NewObjectAcallfunc( jclass, jmethodID, const jvalue*);
   }

    jboolean    (*IsInstanceOf)(JNIEnv*, jobject, jclass);
    jboolean     MyIsInstanceOfdeclarefunc( jobject, jclass) {
   LOGSTRING("IsInstanceOf\n");
   return IsInstanceOfcallfunc( jobject, jclass);
   }

    jmethodID   (*GetMethodID)(JNIEnv*, jclass, const char*, const char*);
    jmethodID    MyGetMethodIDdeclarefunc( jclass, const char*, const char*) {
   LOGSTRING("GetMethodID\n");
   return GetMethodIDcallfunc( jclass, const char*, const char*);
   }

    jobject     (*CallObjectMethod)(JNIEnv*, jobject, jmethodID, ...);
    jobject      MyCallObjectMethoddeclarefunc( jobject, jmethodID, ...) {
   LOGSTRING("CallObjectMethod\n");
   return CallObjectMethodcallfunc( jobject, jmethodID, ...);
   }

    jobject     (*CallObjectMethodV)(JNIEnv*, jobject, jmethodID, va_list);
    jobject      MyCallObjectMethodVdeclarefunc( jobject, jmethodID, va_list) {
   LOGSTRING("CallObjectMethodV\n");
   return CallObjectMethodVcallfunc( jobject, jmethodID, va_list);
   }

    jobject     (*CallObjectMethodA)(JNIEnv*, jobject, jmethodID, const jvalue*);
    jobject      MyCallObjectMethodAdeclarefunc( jobject, jmethodID, const jvalue*) {
   LOGSTRING("CallObjectMethodA\n");
   return CallObjectMethodAcallfunc( jobject, jmethodID, const jvalue*);
   }

    jboolean    (*CallBooleanMethodV)(JNIEnv*, jobject, jmethodID, va_list);
    jboolean     MyCallBooleanMethodVdeclarefunc( jobject, jmethodID, va_list) {
   LOGSTRING("CallBooleanMethodV\n");
   return CallBooleanMethodVcallfunc( jobject, jmethodID, va_list);
   }

    jboolean    (*CallBooleanMethodA)(JNIEnv*, jobject, jmethodID, const jvalue*);
    jboolean     MyCallBooleanMethodAdeclarefunc( jobject, jmethodID, const jvalue*) {
   LOGSTRING("CallBooleanMethodA\n");
   return CallBooleanMethodAcallfunc( jobject, jmethodID, const jvalue*);
   }

    jbyte       (*CallByteMethod)(JNIEnv*, jobject, jmethodID, ...);
    jbyte        MyCallByteMethoddeclarefunc( jobject, jmethodID, ...) {
   LOGSTRING("CallByteMethod\n");
   return CallByteMethodcallfunc( jobject, jmethodID, ...);
   }

    jbyte       (*CallByteMethodV)(JNIEnv*, jobject, jmethodID, va_list);
    jbyte        MyCallByteMethodVdeclarefunc( jobject, jmethodID, va_list) {
   LOGSTRING("CallByteMethodV\n");
   return CallByteMethodVcallfunc( jobject, jmethodID, va_list);
   }

    jbyte       (*CallByteMethodA)(JNIEnv*, jobject, jmethodID, const jvalue*);
    jbyte        MyCallByteMethodAdeclarefunc( jobject, jmethodID, const jvalue*) {
   LOGSTRING("CallByteMethodA\n");
   return CallByteMethodAcallfunc( jobject, jmethodID, const jvalue*);
   }

    jchar       (*CallCharMethod)(JNIEnv*, jobject, jmethodID, ...);
    jchar        MyCallCharMethoddeclarefunc( jobject, jmethodID, ...) {
   LOGSTRING("CallCharMethod\n");
   return CallCharMethodcallfunc( jobject, jmethodID, ...);
   }

    jchar       (*CallCharMethodV)(JNIEnv*, jobject, jmethodID, va_list);
    jchar        MyCallCharMethodVdeclarefunc( jobject, jmethodID, va_list) {
   LOGSTRING("CallCharMethodV\n");
   return CallCharMethodVcallfunc( jobject, jmethodID, va_list);
   }

    jchar       (*CallCharMethodA)(JNIEnv*, jobject, jmethodID, const jvalue*);
    jchar        MyCallCharMethodAdeclarefunc( jobject, jmethodID, const jvalue*) {
   LOGSTRING("CallCharMethodA\n");
   return CallCharMethodAcallfunc( jobject, jmethodID, const jvalue*);
   }

    jshort      (*CallShortMethod)(JNIEnv*, jobject, jmethodID, ...);
    jshort       MyCallShortMethoddeclarefunc( jobject, jmethodID, ...) {
   LOGSTRING("CallShortMethod\n");
   return CallShortMethodcallfunc( jobject, jmethodID, ...);
   }

    jshort      (*CallShortMethodV)(JNIEnv*, jobject, jmethodID, va_list);
    jshort       MyCallShortMethodVdeclarefunc( jobject, jmethodID, va_list) {
   LOGSTRING("CallShortMethodV\n");
   return CallShortMethodVcallfunc( jobject, jmethodID, va_list);
   }

    jshort      (*CallShortMethodA)(JNIEnv*, jobject, jmethodID, const jvalue*);
    jshort       MyCallShortMethodAdeclarefunc( jobject, jmethodID, const jvalue*) {
   LOGSTRING("CallShortMethodA\n");
   return CallShortMethodAcallfunc( jobject, jmethodID, const jvalue*);
   }

    jint        (*CallIntMethod)(JNIEnv*, jobject, jmethodID, ...);
    jint         MyCallIntMethoddeclarefunc( jobject, jmethodID, ...) {
   LOGSTRING("CallIntMethod\n");
   return CallIntMethodcallfunc( jobject, jmethodID, ...);
   }

    jint        (*CallIntMethodV)(JNIEnv*, jobject, jmethodID, va_list);
    jint         MyCallIntMethodVdeclarefunc( jobject, jmethodID, va_list) {
   LOGSTRING("CallIntMethodV\n");
   return CallIntMethodVcallfunc( jobject, jmethodID, va_list);
   }

    jint        (*CallIntMethodA)(JNIEnv*, jobject, jmethodID, const jvalue*);
    jint         MyCallIntMethodAdeclarefunc( jobject, jmethodID, const jvalue*) {
   LOGSTRING("CallIntMethodA\n");
   return CallIntMethodAcallfunc( jobject, jmethodID, const jvalue*);
   }

    jlong       (*CallLongMethod)(JNIEnv*, jobject, jmethodID, ...);
    jlong        MyCallLongMethoddeclarefunc( jobject, jmethodID, ...) {
   LOGSTRING("CallLongMethod\n");
   return CallLongMethodcallfunc( jobject, jmethodID, ...);
   }

    jlong       (*CallLongMethodV)(JNIEnv*, jobject, jmethodID, va_list);
    jlong        MyCallLongMethodVdeclarefunc( jobject, jmethodID, va_list) {
   LOGSTRING("CallLongMethodV\n");
   return CallLongMethodVcallfunc( jobject, jmethodID, va_list);
   }

    jlong       (*CallLongMethodA)(JNIEnv*, jobject, jmethodID, const jvalue*);
    jlong        MyCallLongMethodAdeclarefunc( jobject, jmethodID, const jvalue*) {
   LOGSTRING("CallLongMethodA\n");
   return CallLongMethodAcallfunc( jobject, jmethodID, const jvalue*);
   }

    jfloat      (*CallFloatMethod)(JNIEnv*, jobject, jmethodID, ...);
    jfloat       MyCallFloatMethoddeclarefunc( jobject, jmethodID, ...) {
   LOGSTRING("CallFloatMethod\n");
   return CallFloatMethodcallfunc( jobject, jmethodID, ...);
   }

    jfloat      (*CallFloatMethodV)(JNIEnv*, jobject, jmethodID, va_list);
    jfloat       MyCallFloatMethodVdeclarefunc( jobject, jmethodID, va_list) {
   LOGSTRING("CallFloatMethodV\n");
   return CallFloatMethodVcallfunc( jobject, jmethodID, va_list);
   }

    jfloat      (*CallFloatMethodA)(JNIEnv*, jobject, jmethodID, const jvalue*);
    jfloat       MyCallFloatMethodAdeclarefunc( jobject, jmethodID, const jvalue*) {
   LOGSTRING("CallFloatMethodA\n");
   return CallFloatMethodAcallfunc( jobject, jmethodID, const jvalue*);
   }

    jdouble     (*CallDoubleMethod)(JNIEnv*, jobject, jmethodID, ...);
    jdouble      MyCallDoubleMethoddeclarefunc( jobject, jmethodID, ...) {
   LOGSTRING("CallDoubleMethod\n");
   return CallDoubleMethodcallfunc( jobject, jmethodID, ...);
   }

    jdouble     (*CallDoubleMethodV)(JNIEnv*, jobject, jmethodID, va_list);
    jdouble      MyCallDoubleMethodVdeclarefunc( jobject, jmethodID, va_list) {
   LOGSTRING("CallDoubleMethodV\n");
   return CallDoubleMethodVcallfunc( jobject, jmethodID, va_list);
   }

    jdouble     (*CallDoubleMethodA)(JNIEnv*, jobject, jmethodID, const jvalue*);
    jdouble      MyCallDoubleMethodAdeclarefunc( jobject, jmethodID, const jvalue*) {
   LOGSTRING("CallDoubleMethodA\n");
   return CallDoubleMethodAcallfunc( jobject, jmethodID, const jvalue*);
   }

    void        (*CallVoidMethod)(JNIEnv*, jobject, jmethodID, ...);
    void         MyCallVoidMethoddeclarefunc( jobject, jmethodID, ...) {
   LOGSTRING("CallVoidMethod\n");
   return CallVoidMethodcallfunc( jobject, jmethodID, ...);
   }

    void        (*CallVoidMethodV)(JNIEnv*, jobject, jmethodID, va_list);
    void         MyCallVoidMethodVdeclarefunc( jobject, jmethodID, va_list) {
   LOGSTRING("CallVoidMethodV\n");
   return CallVoidMethodVcallfunc( jobject, jmethodID, va_list);
   }

    void        (*CallVoidMethodA)(JNIEnv*, jobject, jmethodID, const jvalue*);
    void         MyCallVoidMethodAdeclarefunc( jobject, jmethodID, const jvalue*) {
   LOGSTRING("CallVoidMethodA\n");
   return CallVoidMethodAcallfunc( jobject, jmethodID, const jvalue*);
   }

    jobject     (*CallNonvirtualObjectMethod)(JNIEnv*, jobject, jclass,jmethodID, ...);
    jobject      MyCallNonvirtualObjectMethoddeclarefunc( jobject, jclass,jmethodID, ...) {
   LOGSTRING("CallNonvirtualObjectMethod\n");
   return CallNonvirtualObjectMethodcallfunc( jobject, jclass,jmethodID, ...);
   }

    jobject     (*CallNonvirtualObjectMethodV)(JNIEnv*, jobject, jclass,jmethodID, va_list);
    jobject      MyCallNonvirtualObjectMethodVdeclarefunc( jobject, jclass,jmethodID, va_list) {
   LOGSTRING("CallNonvirtualObjectMethodV\n");
   return CallNonvirtualObjectMethodVcallfunc( jobject, jclass,jmethodID, va_list);
   }

    jobject     (*CallNonvirtualObjectMethodA)(JNIEnv*, jobject, jclass,jmethodID, const jvalue*);
    jobject      MyCallNonvirtualObjectMethodAdeclarefunc( jobject, jclass,jmethodID, const jvalue*) {
   LOGSTRING("CallNonvirtualObjectMethodA\n");
   return CallNonvirtualObjectMethodAcallfunc( jobject, jclass,jmethodID, const jvalue*);
   }

    jboolean    (*CallNonvirtualBooleanMethod)(JNIEnv*, jobject, jclass,jmethodID, ...);
    jboolean     MyCallNonvirtualBooleanMethoddeclarefunc( jobject, jclass,jmethodID, ...) {
   LOGSTRING("CallNonvirtualBooleanMethod\n");
   return CallNonvirtualBooleanMethodcallfunc( jobject, jclass,jmethodID, ...);
   }

    jboolean    (*CallNonvirtualBooleanMethodV)(JNIEnv*, jobject, jclass,jmethodID, va_list);
    jboolean     MyCallNonvirtualBooleanMethodVdeclarefunc( jobject, jclass,jmethodID, va_list) {
   LOGSTRING("CallNonvirtualBooleanMethodV\n");
   return CallNonvirtualBooleanMethodVcallfunc( jobject, jclass,jmethodID, va_list);
   }

    jboolean    (*CallNonvirtualBooleanMethodA)(JNIEnv*, jobject, jclass,jmethodID, const jvalue*);
    jboolean     MyCallNonvirtualBooleanMethodAdeclarefunc( jobject, jclass,jmethodID, const jvalue*) {
   LOGSTRING("CallNonvirtualBooleanMethodA\n");
   return CallNonvirtualBooleanMethodAcallfunc( jobject, jclass,jmethodID, const jvalue*);
   }

    jbyte       (*CallNonvirtualByteMethod)(JNIEnv*, jobject, jclass,jmethodID, ...);
    jbyte        MyCallNonvirtualByteMethoddeclarefunc( jobject, jclass,jmethodID, ...) {
   LOGSTRING("CallNonvirtualByteMethod\n");
   return CallNonvirtualByteMethodcallfunc( jobject, jclass,jmethodID, ...);
   }

    jbyte       (*CallNonvirtualByteMethodV)(JNIEnv*, jobject, jclass,jmethodID, va_list);
    jbyte        MyCallNonvirtualByteMethodVdeclarefunc( jobject, jclass,jmethodID, va_list) {
   LOGSTRING("CallNonvirtualByteMethodV\n");
   return CallNonvirtualByteMethodVcallfunc( jobject, jclass,jmethodID, va_list);
   }

    jbyte       (*CallNonvirtualByteMethodA)(JNIEnv*, jobject, jclass,jmethodID, const jvalue*);
    jbyte        MyCallNonvirtualByteMethodAdeclarefunc( jobject, jclass,jmethodID, const jvalue*) {
   LOGSTRING("CallNonvirtualByteMethodA\n");
   return CallNonvirtualByteMethodAcallfunc( jobject, jclass,jmethodID, const jvalue*);
   }

    jchar       (*CallNonvirtualCharMethod)(JNIEnv*, jobject, jclass,jmethodID, ...);
    jchar        MyCallNonvirtualCharMethoddeclarefunc( jobject, jclass,jmethodID, ...) {
   LOGSTRING("CallNonvirtualCharMethod\n");
   return CallNonvirtualCharMethodcallfunc( jobject, jclass,jmethodID, ...);
   }

    jchar       (*CallNonvirtualCharMethodV)(JNIEnv*, jobject, jclass,jmethodID, va_list);
    jchar        MyCallNonvirtualCharMethodVdeclarefunc( jobject, jclass,jmethodID, va_list) {
   LOGSTRING("CallNonvirtualCharMethodV\n");
   return CallNonvirtualCharMethodVcallfunc( jobject, jclass,jmethodID, va_list);
   }

    jchar       (*CallNonvirtualCharMethodA)(JNIEnv*, jobject, jclass,jmethodID, const jvalue*);
    jchar        MyCallNonvirtualCharMethodAdeclarefunc( jobject, jclass,jmethodID, const jvalue*) {
   LOGSTRING("CallNonvirtualCharMethodA\n");
   return CallNonvirtualCharMethodAcallfunc( jobject, jclass,jmethodID, const jvalue*);
   }

    jshort      (*CallNonvirtualShortMethod)(JNIEnv*, jobject, jclass,jmethodID, ...);
    jshort       MyCallNonvirtualShortMethoddeclarefunc( jobject, jclass,jmethodID, ...) {
   LOGSTRING("CallNonvirtualShortMethod\n");
   return CallNonvirtualShortMethodcallfunc( jobject, jclass,jmethodID, ...);
   }

    jshort      (*CallNonvirtualShortMethodV)(JNIEnv*, jobject, jclass,jmethodID, va_list);
    jshort       MyCallNonvirtualShortMethodVdeclarefunc( jobject, jclass,jmethodID, va_list) {
   LOGSTRING("CallNonvirtualShortMethodV\n");
   return CallNonvirtualShortMethodVcallfunc( jobject, jclass,jmethodID, va_list);
   }

    jshort      (*CallNonvirtualShortMethodA)(JNIEnv*, jobject, jclass,jmethodID, const jvalue*);
    jshort       MyCallNonvirtualShortMethodAdeclarefunc( jobject, jclass,jmethodID, const jvalue*) {
   LOGSTRING("CallNonvirtualShortMethodA\n");
   return CallNonvirtualShortMethodAcallfunc( jobject, jclass,jmethodID, const jvalue*);
   }

    jint        (*CallNonvirtualIntMethod)(JNIEnv*, jobject, jclass,jmethodID, ...);
    jint         MyCallNonvirtualIntMethoddeclarefunc( jobject, jclass,jmethodID, ...) {
   LOGSTRING("CallNonvirtualIntMethod\n");
   return CallNonvirtualIntMethodcallfunc( jobject, jclass,jmethodID, ...);
   }

    jint        (*CallNonvirtualIntMethodV)(JNIEnv*, jobject, jclass,jmethodID, va_list);
    jint         MyCallNonvirtualIntMethodVdeclarefunc( jobject, jclass,jmethodID, va_list) {
   LOGSTRING("CallNonvirtualIntMethodV\n");
   return CallNonvirtualIntMethodVcallfunc( jobject, jclass,jmethodID, va_list);
   }

    jint        (*CallNonvirtualIntMethodA)(JNIEnv*, jobject, jclass,jmethodID, const jvalue*);
    jint         MyCallNonvirtualIntMethodAdeclarefunc( jobject, jclass,jmethodID, const jvalue*) {
   LOGSTRING("CallNonvirtualIntMethodA\n");
   return CallNonvirtualIntMethodAcallfunc( jobject, jclass,jmethodID, const jvalue*);
   }

    jlong       (*CallNonvirtualLongMethod)(JNIEnv*, jobject, jclass,jmethodID, ...);
    jlong        MyCallNonvirtualLongMethoddeclarefunc( jobject, jclass,jmethodID, ...) {
   LOGSTRING("CallNonvirtualLongMethod\n");
   return CallNonvirtualLongMethodcallfunc( jobject, jclass,jmethodID, ...);
   }

    jlong       (*CallNonvirtualLongMethodV)(JNIEnv*, jobject, jclass,jmethodID, va_list);
    jlong        MyCallNonvirtualLongMethodVdeclarefunc( jobject, jclass,jmethodID, va_list) {
   LOGSTRING("CallNonvirtualLongMethodV\n");
   return CallNonvirtualLongMethodVcallfunc( jobject, jclass,jmethodID, va_list);
   }

    jlong       (*CallNonvirtualLongMethodA)(JNIEnv*, jobject, jclass,jmethodID, const jvalue*);
    jlong        MyCallNonvirtualLongMethodAdeclarefunc( jobject, jclass,jmethodID, const jvalue*) {
   LOGSTRING("CallNonvirtualLongMethodA\n");
   return CallNonvirtualLongMethodAcallfunc( jobject, jclass,jmethodID, const jvalue*);
   }

    jfloat      (*CallNonvirtualFloatMethod)(JNIEnv*, jobject, jclass,jmethodID, ...);
    jfloat       MyCallNonvirtualFloatMethoddeclarefunc( jobject, jclass,jmethodID, ...) {
   LOGSTRING("CallNonvirtualFloatMethod\n");
   return CallNonvirtualFloatMethodcallfunc( jobject, jclass,jmethodID, ...);
   }

    jfloat      (*CallNonvirtualFloatMethodV)(JNIEnv*, jobject, jclass,jmethodID, va_list);
    jfloat       MyCallNonvirtualFloatMethodVdeclarefunc( jobject, jclass,jmethodID, va_list) {
   LOGSTRING("CallNonvirtualFloatMethodV\n");
   return CallNonvirtualFloatMethodVcallfunc( jobject, jclass,jmethodID, va_list);
   }

    jfloat      (*CallNonvirtualFloatMethodA)(JNIEnv*, jobject, jclass,jmethodID, const jvalue*);
    jfloat       MyCallNonvirtualFloatMethodAdeclarefunc( jobject, jclass,jmethodID, const jvalue*) {
   LOGSTRING("CallNonvirtualFloatMethodA\n");
   return CallNonvirtualFloatMethodAcallfunc( jobject, jclass,jmethodID, const jvalue*);
   }

    jdouble     (*CallNonvirtualDoubleMethod)(JNIEnv*, jobject, jclass,jmethodID, ...);
    jdouble      MyCallNonvirtualDoubleMethoddeclarefunc( jobject, jclass,jmethodID, ...) {
   LOGSTRING("CallNonvirtualDoubleMethod\n");
   return CallNonvirtualDoubleMethodcallfunc( jobject, jclass,jmethodID, ...);
   }

    jdouble     (*CallNonvirtualDoubleMethodV)(JNIEnv*, jobject, jclass,jmethodID, va_list);
    jdouble      MyCallNonvirtualDoubleMethodVdeclarefunc( jobject, jclass,jmethodID, va_list) {
   LOGSTRING("CallNonvirtualDoubleMethodV\n");
   return CallNonvirtualDoubleMethodVcallfunc( jobject, jclass,jmethodID, va_list);
   }

    jdouble     (*CallNonvirtualDoubleMethodA)(JNIEnv*, jobject, jclass,jmethodID, const jvalue*);
    jdouble      MyCallNonvirtualDoubleMethodAdeclarefunc( jobject, jclass,jmethodID, const jvalue*) {
   LOGSTRING("CallNonvirtualDoubleMethodA\n");
   return CallNonvirtualDoubleMethodAcallfunc( jobject, jclass,jmethodID, const jvalue*);
   }

    void        (*CallNonvirtualVoidMethod)(JNIEnv*, jobject, jclass,jmethodID, ...);
    void         MyCallNonvirtualVoidMethoddeclarefunc( jobject, jclass,jmethodID, ...) {
   LOGSTRING("CallNonvirtualVoidMethod\n");
   return CallNonvirtualVoidMethodcallfunc( jobject, jclass,jmethodID, ...);
   }

    void        (*CallNonvirtualVoidMethodV)(JNIEnv*, jobject, jclass,jmethodID, va_list);
    void         MyCallNonvirtualVoidMethodVdeclarefunc( jobject, jclass,jmethodID, va_list) {
   LOGSTRING("CallNonvirtualVoidMethodV\n");
   return CallNonvirtualVoidMethodVcallfunc( jobject, jclass,jmethodID, va_list);
   }

    void        (*CallNonvirtualVoidMethodA)(JNIEnv*, jobject, jclass,jmethodID, const jvalue*);
    void         MyCallNonvirtualVoidMethodAdeclarefunc( jobject, jclass,jmethodID, const jvalue*) {
   LOGSTRING("CallNonvirtualVoidMethodA\n");
   return CallNonvirtualVoidMethodAcallfunc( jobject, jclass,jmethodID, const jvalue*);
   }

    jfieldID    (*GetFieldID)(JNIEnv*, jclass, const char*, const char*);
    jfieldID     MyGetFieldIDdeclarefunc( jclass, const char*, const char*) {
   LOGSTRING("GetFieldID\n");
   return GetFieldIDcallfunc( jclass, const char*, const char*);
   }

    jobject     (*GetObjectField)(JNIEnv*, jobject, jfieldID);
    jobject      MyGetObjectFielddeclarefunc( jobject, jfieldID) {
   LOGSTRING("GetObjectField\n");
   return GetObjectFieldcallfunc( jobject, jfieldID);
   }

    jboolean    (*GetBooleanField)(JNIEnv*, jobject, jfieldID);
    jboolean     MyGetBooleanFielddeclarefunc( jobject, jfieldID) {
   LOGSTRING("GetBooleanField\n");
   return GetBooleanFieldcallfunc( jobject, jfieldID);
   }

    jbyte       (*GetByteField)(JNIEnv*, jobject, jfieldID);
    jbyte        MyGetByteFielddeclarefunc( jobject, jfieldID) {
   LOGSTRING("GetByteField\n");
   return GetByteFieldcallfunc( jobject, jfieldID);
   }

    jchar       (*GetCharField)(JNIEnv*, jobject, jfieldID);
    jchar        MyGetCharFielddeclarefunc( jobject, jfieldID) {
   LOGSTRING("GetCharField\n");
   return GetCharFieldcallfunc( jobject, jfieldID);
   }

    jshort      (*GetShortField)(JNIEnv*, jobject, jfieldID);
    jshort       MyGetShortFielddeclarefunc( jobject, jfieldID) {
   LOGSTRING("GetShortField\n");
   return GetShortFieldcallfunc( jobject, jfieldID);
   }

    jint        (*GetIntField)(JNIEnv*, jobject, jfieldID);
    jint         MyGetIntFielddeclarefunc( jobject, jfieldID) {
   LOGSTRING("GetIntField\n");
   return GetIntFieldcallfunc( jobject, jfieldID);
   }

    jlong       (*GetLongField)(JNIEnv*, jobject, jfieldID);
    jlong        MyGetLongFielddeclarefunc( jobject, jfieldID) {
   LOGSTRING("GetLongField\n");
   return GetLongFieldcallfunc( jobject, jfieldID);
   }

    jfloat      (*GetFloatField)(JNIEnv*, jobject, jfieldID);
    jfloat       MyGetFloatFielddeclarefunc( jobject, jfieldID) {
   LOGSTRING("GetFloatField\n");
   return GetFloatFieldcallfunc( jobject, jfieldID);
   }

    jdouble     (*GetDoubleField)(JNIEnv*, jobject, jfieldID);
    jdouble      MyGetDoubleFielddeclarefunc( jobject, jfieldID) {
   LOGSTRING("GetDoubleField\n");
   return GetDoubleFieldcallfunc( jobject, jfieldID);
   }

    void        (*SetObjectField)(JNIEnv*, jobject, jfieldID, jobject);
    void         MySetObjectFielddeclarefunc( jobject, jfieldID, jobject) {
   LOGSTRING("SetObjectField\n");
   return SetObjectFieldcallfunc( jobject, jfieldID, jobject);
   }

    void        (*SetBooleanField)(JNIEnv*, jobject, jfieldID, jboolean);
    void         MySetBooleanFielddeclarefunc( jobject, jfieldID, jboolean) {
   LOGSTRING("SetBooleanField\n");
   return SetBooleanFieldcallfunc( jobject, jfieldID, jboolean);
   }

    void        (*SetByteField)(JNIEnv*, jobject, jfieldID, jbyte);
    void         MySetByteFielddeclarefunc( jobject, jfieldID, jbyte) {
   LOGSTRING("SetByteField\n");
   return SetByteFieldcallfunc( jobject, jfieldID, jbyte);
   }

    void        (*SetCharField)(JNIEnv*, jobject, jfieldID, jchar);
    void         MySetCharFielddeclarefunc( jobject, jfieldID, jchar) {
   LOGSTRING("SetCharField\n");
   return SetCharFieldcallfunc( jobject, jfieldID, jchar);
   }

    void        (*SetShortField)(JNIEnv*, jobject, jfieldID, jshort);
    void         MySetShortFielddeclarefunc( jobject, jfieldID, jshort) {
   LOGSTRING("SetShortField\n");
   return SetShortFieldcallfunc( jobject, jfieldID, jshort);
   }

    void        (*SetIntField)(JNIEnv*, jobject, jfieldID, jint);
    void         MySetIntFielddeclarefunc( jobject, jfieldID, jint) {
   LOGSTRING("SetIntField\n");
   return SetIntFieldcallfunc( jobject, jfieldID, jint);
   }

    void        (*SetLongField)(JNIEnv*, jobject, jfieldID, jlong);
    void         MySetLongFielddeclarefunc( jobject, jfieldID, jlong) {
   LOGSTRING("SetLongField\n");
   return SetLongFieldcallfunc( jobject, jfieldID, jlong);
   }

    void        (*SetFloatField)(JNIEnv*, jobject, jfieldID, jfloat);
    void         MySetFloatFielddeclarefunc( jobject, jfieldID, jfloat) {
   LOGSTRING("SetFloatField\n");
   return SetFloatFieldcallfunc( jobject, jfieldID, jfloat);
   }

    void        (*SetDoubleField)(JNIEnv*, jobject, jfieldID, jdouble);
    void         MySetDoubleFielddeclarefunc( jobject, jfieldID, jdouble) {
   LOGSTRING("SetDoubleField\n");
   return SetDoubleFieldcallfunc( jobject, jfieldID, jdouble);
   }

    jmethodID   (*GetStaticMethodID)(JNIEnv*, jclass, const char*, const char*);
    jmethodID    MyGetStaticMethodIDdeclarefunc( jclass, const char*, const char*) {
   LOGSTRING("GetStaticMethodID\n");
   return GetStaticMethodIDcallfunc( jclass, const char*, const char*);
   }

    jobject     (*CallStaticObjectMethodV)(JNIEnv*, jclass, jmethodID, va_list);
    jobject      MyCallStaticObjectMethodVdeclarefunc( jclass, jmethodID, va_list) {
   LOGSTRING("CallStaticObjectMethodV\n");
   return CallStaticObjectMethodVcallfunc( jclass, jmethodID, va_list);
   }

    jobject     (*CallStaticObjectMethodA)(JNIEnv*, jclass, jmethodID, const jvalue*);
    jobject      MyCallStaticObjectMethodAdeclarefunc( jclass, jmethodID, const jvalue*) {
   LOGSTRING("CallStaticObjectMethodA\n");
   return CallStaticObjectMethodAcallfunc( jclass, jmethodID, const jvalue*);
   }

    jboolean    (*CallStaticBooleanMethod)(JNIEnv*, jclass, jmethodID, ...);
    jboolean     MyCallStaticBooleanMethoddeclarefunc( jclass, jmethodID, ...) {
   LOGSTRING("CallStaticBooleanMethod\n");
   return CallStaticBooleanMethodcallfunc( jclass, jmethodID, ...);
   }

    jboolean    (*CallStaticBooleanMethodV)(JNIEnv*, jclass, jmethodID,va_list);
    jboolean     MyCallStaticBooleanMethodVdeclarefunc( jclass, jmethodID,va_list) {
   LOGSTRING("CallStaticBooleanMethodV\n");
   return CallStaticBooleanMethodVcallfunc( jclass, jmethodID,va_list);
   }

    jboolean    (*CallStaticBooleanMethodA)(JNIEnv*, jclass, jmethodID, const jvalue*);
    jboolean     MyCallStaticBooleanMethodAdeclarefunc( jclass, jmethodID, const jvalue*) {
   LOGSTRING("CallStaticBooleanMethodA\n");
   return CallStaticBooleanMethodAcallfunc( jclass, jmethodID, const jvalue*);
   }

    jbyte       (*CallStaticByteMethod)(JNIEnv*, jclass, jmethodID, ...);
    jbyte        MyCallStaticByteMethoddeclarefunc( jclass, jmethodID, ...) {
   LOGSTRING("CallStaticByteMethod\n");
   return CallStaticByteMethodcallfunc( jclass, jmethodID, ...);
   }

    jbyte       (*CallStaticByteMethodV)(JNIEnv*, jclass, jmethodID, va_list);
    jbyte        MyCallStaticByteMethodVdeclarefunc( jclass, jmethodID, va_list) {
   LOGSTRING("CallStaticByteMethodV\n");
   return CallStaticByteMethodVcallfunc( jclass, jmethodID, va_list);
   }

    jbyte       (*CallStaticByteMethodA)(JNIEnv*, jclass, jmethodID, const jvalue*);
    jbyte        MyCallStaticByteMethodAdeclarefunc( jclass, jmethodID, const jvalue*) {
   LOGSTRING("CallStaticByteMethodA\n");
   return CallStaticByteMethodAcallfunc( jclass, jmethodID, const jvalue*);
   }

    jchar       (*CallStaticCharMethod)(JNIEnv*, jclass, jmethodID, ...);
    jchar        MyCallStaticCharMethoddeclarefunc( jclass, jmethodID, ...) {
   LOGSTRING("CallStaticCharMethod\n");
   return CallStaticCharMethodcallfunc( jclass, jmethodID, ...);
   }

    jchar       (*CallStaticCharMethodV)(JNIEnv*, jclass, jmethodID, va_list);
    jchar        MyCallStaticCharMethodVdeclarefunc( jclass, jmethodID, va_list) {
   LOGSTRING("CallStaticCharMethodV\n");
   return CallStaticCharMethodVcallfunc( jclass, jmethodID, va_list);
   }

    jchar       (*CallStaticCharMethodA)(JNIEnv*, jclass, jmethodID, const jvalue*);
    jchar        MyCallStaticCharMethodAdeclarefunc( jclass, jmethodID, const jvalue*) {
   LOGSTRING("CallStaticCharMethodA\n");
   return CallStaticCharMethodAcallfunc( jclass, jmethodID, const jvalue*);
   }

    jshort      (*CallStaticShortMethod)(JNIEnv*, jclass, jmethodID, ...);
    jshort       MyCallStaticShortMethoddeclarefunc( jclass, jmethodID, ...) {
   LOGSTRING("CallStaticShortMethod\n");
   return CallStaticShortMethodcallfunc( jclass, jmethodID, ...);
   }

    jshort      (*CallStaticShortMethodV)(JNIEnv*, jclass, jmethodID, va_list);
    jshort       MyCallStaticShortMethodVdeclarefunc( jclass, jmethodID, va_list) {
   LOGSTRING("CallStaticShortMethodV\n");
   return CallStaticShortMethodVcallfunc( jclass, jmethodID, va_list);
   }

    jshort      (*CallStaticShortMethodA)(JNIEnv*, jclass, jmethodID, const jvalue*);
    jshort       MyCallStaticShortMethodAdeclarefunc( jclass, jmethodID, const jvalue*) {
   LOGSTRING("CallStaticShortMethodA\n");
   return CallStaticShortMethodAcallfunc( jclass, jmethodID, const jvalue*);
   }

    jint        (*CallStaticIntMethodV)(JNIEnv*, jclass, jmethodID, va_list);
    jint         MyCallStaticIntMethodVdeclarefunc( jclass, jmethodID, va_list) {
   LOGSTRING("CallStaticIntMethodV\n");
   return CallStaticIntMethodVcallfunc( jclass, jmethodID, va_list);
   }

    jint        (*CallStaticIntMethodA)(JNIEnv*, jclass, jmethodID, const jvalue*);
    jint         MyCallStaticIntMethodAdeclarefunc( jclass, jmethodID, const jvalue*) {
   LOGSTRING("CallStaticIntMethodA\n");
   return CallStaticIntMethodAcallfunc( jclass, jmethodID, const jvalue*);
   }

    jlong       (*CallStaticLongMethod)(JNIEnv*, jclass, jmethodID, ...);
    jlong        MyCallStaticLongMethoddeclarefunc( jclass, jmethodID, ...) {
   LOGSTRING("CallStaticLongMethod\n");
   return CallStaticLongMethodcallfunc( jclass, jmethodID, ...);
   }

    jlong       (*CallStaticLongMethodV)(JNIEnv*, jclass, jmethodID, va_list);
    jlong        MyCallStaticLongMethodVdeclarefunc( jclass, jmethodID, va_list) {
   LOGSTRING("CallStaticLongMethodV\n");
   return CallStaticLongMethodVcallfunc( jclass, jmethodID, va_list);
   }

    jlong       (*CallStaticLongMethodA)(JNIEnv*, jclass, jmethodID, const jvalue*);
    jlong        MyCallStaticLongMethodAdeclarefunc( jclass, jmethodID, const jvalue*) {
   LOGSTRING("CallStaticLongMethodA\n");
   return CallStaticLongMethodAcallfunc( jclass, jmethodID, const jvalue*);
   }

    jfloat      (*CallStaticFloatMethod)(JNIEnv*, jclass, jmethodID, ...);
    jfloat       MyCallStaticFloatMethoddeclarefunc( jclass, jmethodID, ...) {
   LOGSTRING("CallStaticFloatMethod\n");
   return CallStaticFloatMethodcallfunc( jclass, jmethodID, ...);
   }

    jfloat      (*CallStaticFloatMethodV)(JNIEnv*, jclass, jmethodID, va_list);
    jfloat       MyCallStaticFloatMethodVdeclarefunc( jclass, jmethodID, va_list) {
   LOGSTRING("CallStaticFloatMethodV\n");
   return CallStaticFloatMethodVcallfunc( jclass, jmethodID, va_list);
   }

    jfloat      (*CallStaticFloatMethodA)(JNIEnv*, jclass, jmethodID, const jvalue*);
    jfloat       MyCallStaticFloatMethodAdeclarefunc( jclass, jmethodID, const jvalue*) {
   LOGSTRING("CallStaticFloatMethodA\n");
   return CallStaticFloatMethodAcallfunc( jclass, jmethodID, const jvalue*);
   }

    jdouble     (*CallStaticDoubleMethod)(JNIEnv*, jclass, jmethodID, ...);
    jdouble      MyCallStaticDoubleMethoddeclarefunc( jclass, jmethodID, ...) {
   LOGSTRING("CallStaticDoubleMethod\n");
   return CallStaticDoubleMethodcallfunc( jclass, jmethodID, ...);
   }

    jdouble     (*CallStaticDoubleMethodV)(JNIEnv*, jclass, jmethodID, va_list);
    jdouble      MyCallStaticDoubleMethodVdeclarefunc( jclass, jmethodID, va_list) {
   LOGSTRING("CallStaticDoubleMethodV\n");
   return CallStaticDoubleMethodVcallfunc( jclass, jmethodID, va_list);
   }

    jdouble     (*CallStaticDoubleMethodA)(JNIEnv*, jclass, jmethodID, const jvalue*);
    jdouble      MyCallStaticDoubleMethodAdeclarefunc( jclass, jmethodID, const jvalue*) {
   LOGSTRING("CallStaticDoubleMethodA\n");
   return CallStaticDoubleMethodAcallfunc( jclass, jmethodID, const jvalue*);
   }

    void        (*CallStaticVoidMethod)(JNIEnv*, jclass, jmethodID, ...);
    void         MyCallStaticVoidMethoddeclarefunc( jclass, jmethodID, ...) {
   LOGSTRING("CallStaticVoidMethod\n");
   return CallStaticVoidMethodcallfunc( jclass, jmethodID, ...);
   }

    void        (*CallStaticVoidMethodV)(JNIEnv*, jclass, jmethodID, va_list);
    void         MyCallStaticVoidMethodVdeclarefunc( jclass, jmethodID, va_list) {
   LOGSTRING("CallStaticVoidMethodV\n");
   return CallStaticVoidMethodVcallfunc( jclass, jmethodID, va_list);
   }

    void        (*CallStaticVoidMethodA)(JNIEnv*, jclass, jmethodID, const jvalue*);
    void         MyCallStaticVoidMethodAdeclarefunc( jclass, jmethodID, const jvalue*) {
   LOGSTRING("CallStaticVoidMethodA\n");
   return CallStaticVoidMethodAcallfunc( jclass, jmethodID, const jvalue*);
   }

    jfieldID    (*GetStaticFieldID)(JNIEnv*, jclass, const char*,const char*);
    jfieldID     MyGetStaticFieldIDdeclarefunc( jclass, const char*,const char*) {
   LOGSTRING("GetStaticFieldID\n");
   return GetStaticFieldIDcallfunc( jclass, const char*,const char*);
   }

    jobject     (*GetStaticObjectField)(JNIEnv*, jclass, jfieldID);
    jobject      MyGetStaticObjectFielddeclarefunc( jclass, jfieldID) {
   LOGSTRING("GetStaticObjectField\n");
   return GetStaticObjectFieldcallfunc( jclass, jfieldID);
   }

    jboolean    (*GetStaticBooleanField)(JNIEnv*, jclass, jfieldID);
    jboolean     MyGetStaticBooleanFielddeclarefunc( jclass, jfieldID) {
   LOGSTRING("GetStaticBooleanField\n");
   return GetStaticBooleanFieldcallfunc( jclass, jfieldID);
   }

    jbyte       (*GetStaticByteField)(JNIEnv*, jclass, jfieldID);
    jbyte        MyGetStaticByteFielddeclarefunc( jclass, jfieldID) {
   LOGSTRING("GetStaticByteField\n");
   return GetStaticByteFieldcallfunc( jclass, jfieldID);
   }

    jchar       (*GetStaticCharField)(JNIEnv*, jclass, jfieldID);
    jchar        MyGetStaticCharFielddeclarefunc( jclass, jfieldID) {
   LOGSTRING("GetStaticCharField\n");
   return GetStaticCharFieldcallfunc( jclass, jfieldID);
   }

    jshort      (*GetStaticShortField)(JNIEnv*, jclass, jfieldID);
    jshort       MyGetStaticShortFielddeclarefunc( jclass, jfieldID) {
   LOGSTRING("GetStaticShortField\n");
   return GetStaticShortFieldcallfunc( jclass, jfieldID);
   }

    jint        (*GetStaticIntField)(JNIEnv*, jclass, jfieldID);
    jint         MyGetStaticIntFielddeclarefunc( jclass, jfieldID) {
   LOGSTRING("GetStaticIntField\n");
   return GetStaticIntFieldcallfunc( jclass, jfieldID);
   }

    jlong       (*GetStaticLongField)(JNIEnv*, jclass, jfieldID);
    jlong        MyGetStaticLongFielddeclarefunc( jclass, jfieldID) {
   LOGSTRING("GetStaticLongField\n");
   return GetStaticLongFieldcallfunc( jclass, jfieldID);
   }

    jfloat      (*GetStaticFloatField)(JNIEnv*, jclass, jfieldID);
    jfloat       MyGetStaticFloatFielddeclarefunc( jclass, jfieldID) {
   LOGSTRING("GetStaticFloatField\n");
   return GetStaticFloatFieldcallfunc( jclass, jfieldID);
   }

    jdouble     (*GetStaticDoubleField)(JNIEnv*, jclass, jfieldID);
    jdouble      MyGetStaticDoubleFielddeclarefunc( jclass, jfieldID) {
   LOGSTRING("GetStaticDoubleField\n");
   return GetStaticDoubleFieldcallfunc( jclass, jfieldID);
   }

    void        (*SetStaticObjectField)(JNIEnv*, jclass, jfieldID, jobject);
    void         MySetStaticObjectFielddeclarefunc( jclass, jfieldID, jobject) {
   LOGSTRING("SetStaticObjectField\n");
   return SetStaticObjectFieldcallfunc( jclass, jfieldID, jobject);
   }

    void        (*SetStaticBooleanField)(JNIEnv*, jclass, jfieldID, jboolean);
    void         MySetStaticBooleanFielddeclarefunc( jclass, jfieldID, jboolean) {
   LOGSTRING("SetStaticBooleanField\n");
   return SetStaticBooleanFieldcallfunc( jclass, jfieldID, jboolean);
   }

    void        (*SetStaticByteField)(JNIEnv*, jclass, jfieldID, jbyte);
    void         MySetStaticByteFielddeclarefunc( jclass, jfieldID, jbyte) {
   LOGSTRING("SetStaticByteField\n");
   return SetStaticByteFieldcallfunc( jclass, jfieldID, jbyte);
   }

    void        (*SetStaticCharField)(JNIEnv*, jclass, jfieldID, jchar);
    void         MySetStaticCharFielddeclarefunc( jclass, jfieldID, jchar) {
   LOGSTRING("SetStaticCharField\n");
   return SetStaticCharFieldcallfunc( jclass, jfieldID, jchar);
   }

    void        (*SetStaticShortField)(JNIEnv*, jclass, jfieldID, jshort);
    void         MySetStaticShortFielddeclarefunc( jclass, jfieldID, jshort) {
   LOGSTRING("SetStaticShortField\n");
   return SetStaticShortFieldcallfunc( jclass, jfieldID, jshort);
   }

    void        (*SetStaticIntField)(JNIEnv*, jclass, jfieldID, jint);
    void         MySetStaticIntFielddeclarefunc( jclass, jfieldID, jint) {
   LOGSTRING("SetStaticIntField\n");
   return SetStaticIntFieldcallfunc( jclass, jfieldID, jint);
   }

    void        (*SetStaticLongField)(JNIEnv*, jclass, jfieldID, jlong);
    void         MySetStaticLongFielddeclarefunc( jclass, jfieldID, jlong) {
   LOGSTRING("SetStaticLongField\n");
   return SetStaticLongFieldcallfunc( jclass, jfieldID, jlong);
   }

    void        (*SetStaticFloatField)(JNIEnv*, jclass, jfieldID, jfloat);
    void         MySetStaticFloatFielddeclarefunc( jclass, jfieldID, jfloat) {
   LOGSTRING("SetStaticFloatField\n");
   return SetStaticFloatFieldcallfunc( jclass, jfieldID, jfloat);
   }

    void        (*SetStaticDoubleField)(JNIEnv*, jclass, jfieldID, jdouble);
    void         MySetStaticDoubleFielddeclarefunc( jclass, jfieldID, jdouble) {
   LOGSTRING("SetStaticDoubleField\n");
   return SetStaticDoubleFieldcallfunc( jclass, jfieldID, jdouble);
   }

    jstring     (*NewString)(JNIEnv*, const jchar*, jsize);
    jstring      MyNewStringdeclarefunc( const jchar*, jsize) {
   LOGSTRING("NewString\n");
   return NewStringcallfunc( const jchar*, jsize);
   }

    jsize       (*GetStringLength)(JNIEnv*, jstring);
    jsize        MyGetStringLengthdeclarefunc( jstring) {
   LOGSTRING("GetStringLength\n");
   return GetStringLengthcallfunc( jstring);
   }

    const jchar* (*GetStringChars)(JNIEnv*, jstring, jboolean*);
    const jchar*  MyGetStringCharsdeclarefunc( jstring, jboolean*) {
   LOGSTRING("GetStringChars\n");
   return GetStringCharscallfunc( jstring, jboolean*);
   }

    void        (*ReleaseStringChars)(JNIEnv*, jstring, const jchar*);
    void         MyReleaseStringCharsdeclarefunc( jstring, const jchar*) {
   LOGSTRING("ReleaseStringChars\n");
   return ReleaseStringCharscallfunc( jstring, const jchar*);
   }

    jstring     (*NewStringUTF)(JNIEnv*, const char*);
    jstring      MyNewStringUTFdeclarefunc( const char*) {
   LOGSTRING("NewStringUTF\n");
   return NewStringUTFcallfunc( const char*);
   }

    jsize       (*GetStringUTFLength)(JNIEnv*, jstring);
    jsize        MyGetStringUTFLengthdeclarefunc( jstring) {
   LOGSTRING("GetStringUTFLength\n");
   return GetStringUTFLengthcallfunc( jstring);
   }

    const char* (*GetStringUTFChars)(JNIEnv*, jstring, jboolean*);
    const char*  MyGetStringUTFCharsdeclarefunc( jstring, jboolean*) {
   LOGSTRING("GetStringUTFChars\n");
   return GetStringUTFCharscallfunc( jstring, jboolean*);
   }

    void        (*ReleaseStringUTFChars)(JNIEnv*, jstring, const char*);
    void         MyReleaseStringUTFCharsdeclarefunc( jstring, const char*) {
   LOGSTRING("ReleaseStringUTFChars\n");
   return ReleaseStringUTFCharscallfunc( jstring, const char*);
   }

    jobjectArray (*NewObjectArray)(JNIEnv*, jsize, jclass, jobject);
    jobjectArray  MyNewObjectArraydeclarefunc( jsize, jclass, jobject) {
   LOGSTRING("NewObjectArray\n");
   return NewObjectArraycallfunc( jsize, jclass, jobject);
   }

    jobject     (*GetObjectArrayElement)(JNIEnv*, jobjectArray, jsize);
    jobject      MyGetObjectArrayElementdeclarefunc( jobjectArray, jsize) {
   LOGSTRING("GetObjectArrayElement\n");
   return GetObjectArrayElementcallfunc( jobjectArray, jsize);
   }

    void        (*SetObjectArrayElement)(JNIEnv*, jobjectArray, jsize, jobject);
    void         MySetObjectArrayElementdeclarefunc( jobjectArray, jsize, jobject) {
   LOGSTRING("SetObjectArrayElement\n");
   return SetObjectArrayElementcallfunc( jobjectArray, jsize, jobject);
   }

    jbooleanArray (*NewBooleanArray)(JNIEnv*, jsize);
    jbooleanArray  MyNewBooleanArraydeclarefunc( jsize) {
   LOGSTRING("NewBooleanArray\n");
   return NewBooleanArraycallfunc( jsize);
   }

    jcharArray    (*NewCharArray)(JNIEnv*, jsize);
    jcharArray     MyNewCharArraydeclarefunc( jsize) {
   LOGSTRING("NewCharArray\n");
   return NewCharArraycallfunc( jsize);
   }

    jshortArray   (*NewShortArray)(JNIEnv*, jsize);
    jshortArray    MyNewShortArraydeclarefunc( jsize) {
   LOGSTRING("NewShortArray\n");
   return NewShortArraycallfunc( jsize);
   }

    jintArray     (*NewIntArray)(JNIEnv*, jsize);
    jintArray      MyNewIntArraydeclarefunc( jsize) {
   LOGSTRING("NewIntArray\n");
   return NewIntArraycallfunc( jsize);
   }

    jlongArray    (*NewLongArray)(JNIEnv*, jsize);
    jlongArray     MyNewLongArraydeclarefunc( jsize) {
   LOGSTRING("NewLongArray\n");
   return NewLongArraycallfunc( jsize);
   }

    jfloatArray   (*NewFloatArray)(JNIEnv*, jsize);
    jfloatArray    MyNewFloatArraydeclarefunc( jsize) {
   LOGSTRING("NewFloatArray\n");
   return NewFloatArraycallfunc( jsize);
   }

    jdoubleArray  (*NewDoubleArray)(JNIEnv*, jsize);
    jdoubleArray   MyNewDoubleArraydeclarefunc( jsize) {
   LOGSTRING("NewDoubleArray\n");
   return NewDoubleArraycallfunc( jsize);
   }

    jboolean*   (*GetBooleanArrayElements)(JNIEnv*, jbooleanArray, jboolean*);
    jboolean*    MyGetBooleanArrayElementsdeclarefunc( jbooleanArray, jboolean*) {
   LOGSTRING("GetBooleanArrayElements\n");
   return GetBooleanArrayElementscallfunc( jbooleanArray, jboolean*);
   }

    jchar*      (*GetCharArrayElements)(JNIEnv*, jcharArray, jboolean*);
    jchar*       MyGetCharArrayElementsdeclarefunc( jcharArray, jboolean*) {
   LOGSTRING("GetCharArrayElements\n");
   return GetCharArrayElementscallfunc( jcharArray, jboolean*);
   }

    jshort*     (*GetShortArrayElements)(JNIEnv*, jshortArray, jboolean*);
    jshort*      MyGetShortArrayElementsdeclarefunc( jshortArray, jboolean*) {
   LOGSTRING("GetShortArrayElements\n");
   return GetShortArrayElementscallfunc( jshortArray, jboolean*);
   }

    jint*       (*GetIntArrayElements)(JNIEnv*, jintArray, jboolean*);
    jint*        MyGetIntArrayElementsdeclarefunc( jintArray, jboolean*) {
   LOGSTRING("GetIntArrayElements\n");
   return GetIntArrayElementscallfunc( jintArray, jboolean*);
   }

    jlong*      (*GetLongArrayElements)(JNIEnv*, jlongArray, jboolean*);
    jlong*       MyGetLongArrayElementsdeclarefunc( jlongArray, jboolean*) {
   LOGSTRING("GetLongArrayElements\n");
   return GetLongArrayElementscallfunc( jlongArray, jboolean*);
   }

    jfloat*     (*GetFloatArrayElements)(JNIEnv*, jfloatArray, jboolean*);
    jfloat*      MyGetFloatArrayElementsdeclarefunc( jfloatArray, jboolean*) {
   LOGSTRING("GetFloatArrayElements\n");
   return GetFloatArrayElementscallfunc( jfloatArray, jboolean*);
   }

    jdouble*    (*GetDoubleArrayElements)(JNIEnv*, jdoubleArray, jboolean*);
    jdouble*     MyGetDoubleArrayElementsdeclarefunc( jdoubleArray, jboolean*) {
   LOGSTRING("GetDoubleArrayElements\n");
   return GetDoubleArrayElementscallfunc( jdoubleArray, jboolean*);
   }

    void        (*ReleaseBooleanArrayElements)(JNIEnv*, jbooleanArray,jboolean*, jint);
    void         MyReleaseBooleanArrayElementsdeclarefunc( jbooleanArray,jboolean*, jint) {
   LOGSTRING("ReleaseBooleanArrayElements\n");
   return ReleaseBooleanArrayElementscallfunc( jbooleanArray,jboolean*, jint);
   }

    void        (*ReleaseCharArrayElements)(JNIEnv*, jcharArray,jchar*, jint);
    void         MyReleaseCharArrayElementsdeclarefunc( jcharArray,jchar*, jint) {
   LOGSTRING("ReleaseCharArrayElements\n");
   return ReleaseCharArrayElementscallfunc( jcharArray,jchar*, jint);
   }

    void        (*ReleaseShortArrayElements)(JNIEnv*, jshortArray,jshort*, jint);
    void         MyReleaseShortArrayElementsdeclarefunc( jshortArray,jshort*, jint) {
   LOGSTRING("ReleaseShortArrayElements\n");
   return ReleaseShortArrayElementscallfunc( jshortArray,jshort*, jint);
   }

    void        (*ReleaseIntArrayElements)(JNIEnv*, jintArray,jint*, jint);
    void         MyReleaseIntArrayElementsdeclarefunc( jintArray,jint*, jint) {
   LOGSTRING("ReleaseIntArrayElements\n");
   return ReleaseIntArrayElementscallfunc( jintArray,jint*, jint);
   }

    void        (*ReleaseLongArrayElements)(JNIEnv*, jlongArray,jlong*, jint);
    void         MyReleaseLongArrayElementsdeclarefunc( jlongArray,jlong*, jint) {
   LOGSTRING("ReleaseLongArrayElements\n");
   return ReleaseLongArrayElementscallfunc( jlongArray,jlong*, jint);
   }

    void        (*ReleaseFloatArrayElements)(JNIEnv*, jfloatArray,jfloat*, jint);
    void         MyReleaseFloatArrayElementsdeclarefunc( jfloatArray,jfloat*, jint) {
   LOGSTRING("ReleaseFloatArrayElements\n");
   return ReleaseFloatArrayElementscallfunc( jfloatArray,jfloat*, jint);
   }

    void        (*ReleaseDoubleArrayElements)(JNIEnv*, jdoubleArray,jdouble*, jint);
    void         MyReleaseDoubleArrayElementsdeclarefunc( jdoubleArray,jdouble*, jint) {
   LOGSTRING("ReleaseDoubleArrayElements\n");
   return ReleaseDoubleArrayElementscallfunc( jdoubleArray,jdouble*, jint);
   }

    void        (*GetBooleanArrayRegion)(JNIEnv*, jbooleanArray,jsize, jsize, jboolean*);
    void         MyGetBooleanArrayRegiondeclarefunc( jbooleanArray,jsize, jsize, jboolean*) {
   LOGSTRING("GetBooleanArrayRegion\n");
   return GetBooleanArrayRegioncallfunc( jbooleanArray,jsize, jsize, jboolean*);
   }

    void        (*GetByteArrayRegion)(JNIEnv*, jbyteArray,jsize, jsize, jbyte*);
    void         MyGetByteArrayRegiondeclarefunc( jbyteArray,jsize, jsize, jbyte*) {
   LOGSTRING("GetByteArrayRegion\n");
   return GetByteArrayRegioncallfunc( jbyteArray,jsize, jsize, jbyte*);
   }

    void        (*GetCharArrayRegion)(JNIEnv*, jcharArray,jsize, jsize, jchar*);
    void         MyGetCharArrayRegiondeclarefunc( jcharArray,jsize, jsize, jchar*) {
   LOGSTRING("GetCharArrayRegion\n");
   return GetCharArrayRegioncallfunc( jcharArray,jsize, jsize, jchar*);
   }

    void        (*GetShortArrayRegion)(JNIEnv*, jshortArray,jsize, jsize, jshort*);
    void         MyGetShortArrayRegiondeclarefunc( jshortArray,jsize, jsize, jshort*) {
   LOGSTRING("GetShortArrayRegion\n");
   return GetShortArrayRegioncallfunc( jshortArray,jsize, jsize, jshort*);
   }

    void        (*GetIntArrayRegion)(JNIEnv*, jintArray,jsize, jsize, jint*);
    void         MyGetIntArrayRegiondeclarefunc( jintArray,jsize, jsize, jint*) {
   LOGSTRING("GetIntArrayRegion\n");
   return GetIntArrayRegioncallfunc( jintArray,jsize, jsize, jint*);
   }

    void        (*GetLongArrayRegion)(JNIEnv*, jlongArray,jsize, jsize, jlong*);
    void         MyGetLongArrayRegiondeclarefunc( jlongArray,jsize, jsize, jlong*) {
   LOGSTRING("GetLongArrayRegion\n");
   return GetLongArrayRegioncallfunc( jlongArray,jsize, jsize, jlong*);
   }

    void        (*GetFloatArrayRegion)(JNIEnv*, jfloatArray,jsize, jsize, jfloat*);
    void         MyGetFloatArrayRegiondeclarefunc( jfloatArray,jsize, jsize, jfloat*) {
   LOGSTRING("GetFloatArrayRegion\n");
   return GetFloatArrayRegioncallfunc( jfloatArray,jsize, jsize, jfloat*);
   }

    void        (*GetDoubleArrayRegion)(JNIEnv*, jdoubleArray,jsize, jsize, jdouble*);
    void         MyGetDoubleArrayRegiondeclarefunc( jdoubleArray,jsize, jsize, jdouble*) {
   LOGSTRING("GetDoubleArrayRegion\n");
   return GetDoubleArrayRegioncallfunc( jdoubleArray,jsize, jsize, jdouble*);
   }

    void        (*SetBooleanArrayRegion)(JNIEnv*, jbooleanArray,jsize, jsize, const jboolean*);
    void         MySetBooleanArrayRegiondeclarefunc( jbooleanArray,jsize, jsize, const jboolean*) {
   LOGSTRING("SetBooleanArrayRegion\n");
   return SetBooleanArrayRegioncallfunc( jbooleanArray,jsize, jsize, const jboolean*);
   }

    void        (*SetByteArrayRegion)(JNIEnv*, jbyteArray,jsize, jsize, const jbyte*);
    void         MySetByteArrayRegiondeclarefunc( jbyteArray,jsize, jsize, const jbyte*) {
   LOGSTRING("SetByteArrayRegion\n");
   return SetByteArrayRegioncallfunc( jbyteArray,jsize, jsize, const jbyte*);
   }

    void        (*SetCharArrayRegion)(JNIEnv*, jcharArray,jsize, jsize, const jchar*);
    void         MySetCharArrayRegiondeclarefunc( jcharArray,jsize, jsize, const jchar*) {
   LOGSTRING("SetCharArrayRegion\n");
   return SetCharArrayRegioncallfunc( jcharArray,jsize, jsize, const jchar*);
   }

    void        (*SetShortArrayRegion)(JNIEnv*, jshortArray,jsize, jsize, const jshort*);
    void         MySetShortArrayRegiondeclarefunc( jshortArray,jsize, jsize, const jshort*) {
   LOGSTRING("SetShortArrayRegion\n");
   return SetShortArrayRegioncallfunc( jshortArray,jsize, jsize, const jshort*);
   }

    void        (*SetIntArrayRegion)(JNIEnv*, jintArray,jsize, jsize, const jint*);
    void         MySetIntArrayRegiondeclarefunc( jintArray,jsize, jsize, const jint*) {
   LOGSTRING("SetIntArrayRegion\n");
   return SetIntArrayRegioncallfunc( jintArray,jsize, jsize, const jint*);
   }

    void        (*SetLongArrayRegion)(JNIEnv*, jlongArray,jsize, jsize, const jlong*);
    void         MySetLongArrayRegiondeclarefunc( jlongArray,jsize, jsize, const jlong*) {
   LOGSTRING("SetLongArrayRegion\n");
   return SetLongArrayRegioncallfunc( jlongArray,jsize, jsize, const jlong*);
   }

    void        (*SetFloatArrayRegion)(JNIEnv*, jfloatArray,jsize, jsize, const jfloat*);
    void         MySetFloatArrayRegiondeclarefunc( jfloatArray,jsize, jsize, const jfloat*) {
   LOGSTRING("SetFloatArrayRegion\n");
   return SetFloatArrayRegioncallfunc( jfloatArray,jsize, jsize, const jfloat*);
   }

    void        (*SetDoubleArrayRegion)(JNIEnv*, jdoubleArray,jsize, jsize, const jdouble*);
    void         MySetDoubleArrayRegiondeclarefunc( jdoubleArray,jsize, jsize, const jdouble*) {
   LOGSTRING("SetDoubleArrayRegion\n");
   return SetDoubleArrayRegioncallfunc( jdoubleArray,jsize, jsize, const jdouble*);
   }

    jint        (*RegisterNatives)(JNIEnv*, jclass, const JNINativeMethod*,jint);
    jint         MyRegisterNativesdeclarefunc( jclass, const JNINativeMethod*,jint) {
   LOGSTRING("RegisterNatives\n");
   return RegisterNativescallfunc( jclass, const JNINativeMethod*,jint);
   }

    jint        (*UnregisterNatives)(JNIEnv*, jclass);
    jint         MyUnregisterNativesdeclarefunc( jclass) {
   LOGSTRING("UnregisterNatives\n");
   return UnregisterNativescallfunc( jclass);
   }

    jint        (*MonitorEnter)(JNIEnv*, jobject);
    jint         MyMonitorEnterdeclarefunc( jobject) {
   LOGSTRING("MonitorEnter\n");
   return MonitorEntercallfunc( jobject);
   }

    jint        (*MonitorExit)(JNIEnv*, jobject);
    jint         MyMonitorExitdeclarefunc( jobject) {
   LOGSTRING("MonitorExit\n");
   return MonitorExitcallfunc( jobject);
   }

    jint        (*GetJavaVM)(JNIEnv*, JavaVM**);
    jint         MyGetJavaVMdeclarefunc( JavaVM**) {
   LOGSTRING("GetJavaVM\n");
   return GetJavaVMcallfunc( JavaVM**);
   }

    void        (*GetStringRegion)(JNIEnv*, jstring, jsize, jsize, jchar*);
    void         MyGetStringRegiondeclarefunc( jstring, jsize, jsize, jchar*) {
   LOGSTRING("GetStringRegion\n");
   return GetStringRegioncallfunc( jstring, jsize, jsize, jchar*);
   }

    void        (*GetStringUTFRegion)(JNIEnv*, jstring, jsize, jsize, char*);
    void         MyGetStringUTFRegiondeclarefunc( jstring, jsize, jsize, char*) {
   LOGSTRING("GetStringUTFRegion\n");
   return GetStringUTFRegioncallfunc( jstring, jsize, jsize, char*);
   }

    void*       (*GetPrimitiveArrayCritical)(JNIEnv*, jarray, jboolean*);
    void*        MyGetPrimitiveArrayCriticaldeclarefunc( jarray, jboolean*) {
   LOGSTRING("GetPrimitiveArrayCritical\n");
   return GetPrimitiveArrayCriticalcallfunc( jarray, jboolean*);
   }

    void        (*ReleasePrimitiveArrayCritical)(JNIEnv*, jarray, void*, jint);
    void         MyReleasePrimitiveArrayCriticaldeclarefunc( jarray, void*, jint) {
   LOGSTRING("ReleasePrimitiveArrayCritical\n");
   return ReleasePrimitiveArrayCriticalcallfunc( jarray, void*, jint);
   }

    const jchar* (*GetStringCritical)(JNIEnv*, jstring, jboolean*);
    const jchar*  MyGetStringCriticaldeclarefunc( jstring, jboolean*) {
   LOGSTRING("GetStringCritical\n");
   return GetStringCriticalcallfunc( jstring, jboolean*);
   }

    void        (*ReleaseStringCritical)(JNIEnv*, jstring, const jchar*);
    void         MyReleaseStringCriticaldeclarefunc( jstring, const jchar*) {
   LOGSTRING("ReleaseStringCritical\n");
   return ReleaseStringCriticalcallfunc( jstring, const jchar*);
   }

    jweak       (*NewWeakGlobalRef)(JNIEnv*, jobject);
    jweak        MyNewWeakGlobalRefdeclarefunc( jobject) {
   LOGSTRING("NewWeakGlobalRef\n");
   return NewWeakGlobalRefcallfunc( jobject);
   }

    void        (*DeleteWeakGlobalRef)(JNIEnv*, jweak);
    void         MyDeleteWeakGlobalRefdeclarefunc( jweak) {
   LOGSTRING("DeleteWeakGlobalRef\n");
   return DeleteWeakGlobalRefcallfunc( jweak);
   }

    jboolean    (*ExceptionCheck)(JNIEnv*);
    jboolean     MyExceptionCheckdeclarefunc() {
   LOGSTRING("ExceptionCheck\n");
   return ExceptionCheckcallfunc();
   }

    jobject     (*NewDirectByteBuffer)(JNIEnv*, void*, jlong);
    jobject      MyNewDirectByteBufferdeclarefunc( void*, jlong) {
   LOGSTRING("NewDirectByteBuffer\n");
   return NewDirectByteBuffercallfunc( void*, jlong);
   }

    void*       (*GetDirectBufferAddress)(JNIEnv*, jobject);
    void*        MyGetDirectBufferAddressdeclarefunc( jobject) {
   LOGSTRING("GetDirectBufferAddress\n");
   return GetDirectBufferAddresscallfunc( jobject);
   }

    jlong       (*GetDirectBufferCapacity)(JNIEnv*, jobject);
    jlong        MyGetDirectBufferCapacitydeclarefunc( jobject) {
   LOGSTRING("GetDirectBufferCapacity\n");
   return GetDirectBufferCapacitycallfunc( jobject);
   }

    jobjectRefType (*GetObjectRefType)(JNIEnv*, jobject);
    jobjectRefType  MyGetObjectRefTypedeclarefunc( jobject) {
   LOGSTRING("GetObjectRefType\n");
   return GetObjectRefTypecallfunc( jobject);
   }

