#pragma once

#include <jni.h>
extern "C" {
#define algtype(x) x##_t
typedef  jobject JNICALL (*algtype(getAlgorithmContextFromNative))(JNIEnv *env, jclass thiz);
typedef  jint JNICALL (*algtype(initAlgorithmContext))(JNIEnv *env, jclass thiz,jobject alg,jint i,jstring strarg);
typedef  jdouble JNICALL (*algtype(processAlgorithmContext))(JNIEnv *env, jclass thiz,jobject algContext,jint index,jdouble value, jdouble temp,jdouble dzero,jdouble low,jdouble high);
typedef  jstring JNICALL (*algtype(getAlgorithmVersion))(JNIEnv *env, jclass thiz);

typedef  jint JNICALL (*algtype(releaseAlgorithmContext))(JNIEnv *env, jclass thiz,jobject algContext);

typedef jbyteArray  JNICALL (*algtype(getBinaryStructAlgorithmContext))(JNIEnv *env, jclass thiz,jobject algContext);

typedef jint  JNICALL (*algtype(setBinaryStructAlgorithmContext))(JNIEnv *env, jclass thiz,jobject algContext,jbyteArray bar );


#ifdef JNI_HANDLE
 typedef   jint  JNICALL (*algtype(V120Activation))(JNIEnv *env, jclass thiz,jint i, jboolean z, jbyteArray bArr, jlong j, jint i2, jbyteArray bArr2, jint i3);
 typedef   jint  JNICALL (*algtype(V120Reset))(JNIEnv *env, jclass thiz,jint i2, jboolean z, jbyteArray  bArr, jint i3, jbyteArray bArr2, int i4);
typedef jint JNICALL (*algtype(v120RegisterKey))(JNIEnv *env, jclass thiz,jbyteArray bArr, jint i, jbyteArray bArr2);
typedef jint JNICALL (*algtype( V120ApplyAuthentication))(JNIEnv *env, jclass thiz, jint i, jboolean z, jint i2, jbyteArray bArr, jbyteArray bArr2, jint i3);
 typedef   jint  JNICALL (*algtype(V120IsecUpdate))(JNIEnv *env, jclass thiz,jint i, jboolean z, jbyteArray bArr, jlong j, jbyteArray bArr2, jint i3);
  typedef jint  JNICALL (*algtype(V120RawData))(JNIEnv *env, jclass thiz, jint i, jboolean z, jbyteArray bArr, jlong j, jint i2, jbyteArray bArr2, jint i3);
  typedef jint  JNICALL (*algtype(V120SpiltData))(JNIEnv *env, jclass thiz,jint i, jbyteArray bArr, jintArray iArr, jbyteArray bArr2, jboolean z, jbyteArray bArr3, jint i2);

extern algtype(V120SpiltData) V120SpiltData;
extern algtype(v120RegisterKey) v120RegisterKey;
extern algtype(V120ApplyAuthentication) V120ApplyAuthentication;
extern algtype(V120RawData) V120RawData;
extern algtype(V120Activation) V120Activation;
extern algtype(V120Reset) V120Reset;
extern algtype(V120IsecUpdate) V120IsecUpdate;
#endif
};

