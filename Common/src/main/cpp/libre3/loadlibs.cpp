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
/*      Fri Jan 27 15:22:01 CET 2023                                                 */


#include <jni.h>
#include <string.h>
#include <cinttypes>
#include "logs.h"
#include "settings/settings.h"


static jclass      myFindClass(JNIEnv*, const char* name) {
	LOGGER("FindClass %s\n",name);
	return reinterpret_cast<jclass>(const_cast<char *>(name));
	}
//#define cryptfunc(x) Java_tk_glucodata_ECDHCrypto_ ##x
#define cryptfunc(x) x

typedef  jint  JNICALL   (*process1Type)(JNIEnv *env, jclass _cl,jint i2, jbyteArray bArr, jbyteArray bArr2);
static process1Type process1=nullptr;
typedef   jbyteArray  JNICALL   (*process2Type)(JNIEnv *env, jclass _cl,jint i2, jbyteArray bArr, jbyteArray bArr2);
static process2Type process2=nullptr;
typedef jint JNICALL (*DPGetActivationCommandDataType)(JNIEnv *env, jclass _cl, jbyteArray bArr, jlong, jlong);
static DPGetActivationCommandDataType DPGetActivationCommandData=nullptr;

//static  jbyteArray  JNICALL   (*process2)(JNIEnv *env, jclass _cl,jint i2, jbyteArray bArr, jbyteArray bArr2);
static jint        myRegisterNatives(JNIEnv*, jclass name, const JNINativeMethod*methods, jint nr) {
	LOGGER("myRegisterNatives\n");
    const char *namestr=reinterpret_cast<const char *>(name);
	if(!strcmp(namestr,"com/adc/trident/app/frameworks/mobileservices/libre3/security/Libre3SKBCryptoLib")) {
        	process1=(process1Type)methods[0].fnPtr;
		process2=(process2Type)methods[1].fnPtr;
		}
	else {
		LOGGER("class=%s\n",namestr);
		if(!strcmp(namestr,"com/adc/trident/app/frameworks/mobileservices/libre3/libre3DPCRLInterface")) {
			LOGGER("register %s\n",	methods[5].name);
			DPGetActivationCommandData=( DPGetActivationCommandDataType)methods[5].fnPtr;
			}
		}
	LOGGER("end myRegisterNatives\n");
	return 0;
	}
	
static const struct JNINativeInterface envfunctions {
.FindClass=myFindClass,
.RegisterNatives =myRegisterNatives
};
 JNIEnv theenv={.functions=&envfunctions};

jint        mygetenv(JavaVM*, void**envptr, jint) {
	JNIEnv **env=reinterpret_cast<JNIEnv**>(envptr);
	*env=&theenv;
	return 0;
	}

static  JNIInvokeInterface myvm{.GetEnv=mygetenv};


#include <dlfcn.h>
#include "fromjava.h"
#include <string_view>
extern std::string_view libdirname;

#define CHANGECODE
#ifdef CHANGECODE
#include "unprotect.h"
#endif
#include "hexstr.h"
/*
void changelib(uint8_t *from) {
        uint8_t *start= (uint8_t*)from+2432 +7560 -4;
        uint8_t was[]={0xE1,0x00,0x00,0x94};
        hexstr isnow(start,4);
   if(!memcmp(start,was,4)) 
        {
        LOGGER("Sign de same =%s\n",isnow.str());
#ifdef CHANGECODE
        Unprotect unpr(start,4);
        uint8_t nop[]={0xE0,0x03,0x00,0xAA};
        memcpy(start,nop,4);
#endif
        }
else {   
        LOGGER("Sign different =%s\n",isnow.str());
        }
        }
*/
#if defined(__aarch64__) 
void changelib(uint8_t *from) {
   	settings->data()->triedasm=true;
	const uint8_t was[]{0x04,0x2E,0x05,0x94};
	uint8_t *start=(uint8_t*)from+0x119c+7560 -4;
        hexstr isnow(start,4);
   if(!memcmp(start,was,4)) 
        {
        LOGGER("Sign de same =%s\n",isnow.str());
#ifdef CHANGECODE
        Unprotect unpr(start,4);
        uint8_t nop[]={0xE0,0x03,0x00,0xAA};
        memcpy(start,nop,4);
#endif
        }
else {
        LOGGER("Sign different =%s\n",isnow.str());
        }
	}
#endif
#define EXTRA 100

extern bool rootcheck;
static bool doOnLoad(std::string_view libname,bool change) {
	rootcheck=true;

	int libnamelen=libname.size()+1;
	int liblen=libdirname.length();
	char fullpath[libnamelen+ liblen+EXTRA];
	memcpy(fullpath,libdirname.data(),liblen);
	memcpy(fullpath+liblen,libname.data(),libnamelen);
	LOGGER("open %s\n",fullpath);
	void *handle=dlopen(fullpath, RTLD_NOW);
	if(!handle) {
		LOGGER("dlopen %s failed: %s\n",fullpath,dlerror());
		return false;
		}
	const char name[]{"JNI_OnLoad"};
       typedef   jint (*OnLoadtype)(JavaVM* vm, void* reserved) ;
	LOGGER("opened .%s.\n",fullpath);
         OnLoadtype OnLoad= (OnLoadtype)dlsym(handle, name);
	 if(!OnLoad) {
	 	LOGGER("dlsym %s failed\n",name);
		return false;
	 	}
	LOGGER("found OnLoad\n");

#if defined(__aarch64__) 
	if(change) {
	     changelib((uint8_t*)OnLoad);
		}
#endif
	JavaVM vmptr{.functions=&myvm};
	OnLoad(&vmptr,nullptr);
	LOGGER("after OnLoad\n");
if(change)
	LOGGER("process1-OnLoad %ld\n",(uint8_t*)process1-(uint8_t*)OnLoad);
	return true;
	}
////////////////extern "C" JNIEXPORT jboolean JNICALL fromjava(loadECDHCrypto)(JNIEnv *env, jclass thiz) {

static bool loadECDHCrypto(const bool changelib) {
	
	
	if(process1) {
		return true;
		}
	auto res= doOnLoad("/liblibre3extension.so",changelib);
/*
        uint8_t *start= (uint8_t*)process1+2432-4;
        uint8_t was[]={0xE1,0x00,0x00,0x94};
        hexstr isnow(start,4);
   if(!memcmp(start,was,4)) 
        {
        LOGGER("Sign de same =%s\n",isnow.str());
#ifdef CHANGECODE
        Unprotect unpr(start,4);
        uint8_t nop[]={0xE0,0x03,0x00,0xAA};
        memcpy(start,nop,4);
#endif
        }
	else {     
		LOGGER("Sign different =%s\n",isnow.str());
		} */
	return res;
/*
	const JNINativeMethod  funcs[]{{"process1","(I[B[B)I",(void*)process1}, {"process2","(I[B[B)[B",(void*)process2}};
	const char classname[]="tk/glucodata/ECDHCrypto";
	jclass cl=env->FindClass(classname);
	if(!cl) {
		LOGGER("Can't find %s\n",classname);
		return false;
		}
	int rc=env->RegisterNatives(cl, funcs, 2);
   	env->DeleteLocalRef(cl);
	    if (rc != JNI_OK)  {
		    LOGGER("RegisterNatives failed\n");
		    return false;
		    }
	    LOGGER("RegisterNatives  OK\n"); 
	return true;	
	*/
	}
//extern "C" JNIEXPORT jboolean JNICALL fromjava(loadNFC)(JNIEnv *env, jclass thiz) {
static bool loadNFC() {
	if(DPGetActivationCommandData)
		return true;
	LOGGER("loadNFC\n");
	return doOnLoad("/libcrl_dp.so",false);
	/*
	LOGGER(	"after OnLoad\n");
	const char classname[]="tk/glucodata/libre3/NFC";
	jclass cl=env->FindClass(classname);
	if(!cl) {
		LOGGER("Can't find %s\n",classname);
		return false;
		}
	LOGGER("found %s\n",classname);
	const JNINativeMethod  funcs[]{{ "DPGetActivationCommandData", "([BJJ)I",(void*) DPGetActivationCommandData}};
	int rc=env->RegisterNatives(cl, funcs, std::size(funcs));
	LOGGER("after RegisterNatives\n");
   	env->DeleteLocalRef(cl);
	  if (rc != JNI_OK)  {
		    LOGGER("RegisterNatives %s failed\n",classname);
		    return false;
		    }
	LOGGER("RegisterNatives %s OK\n",classname);
	return true;	 */
	}
#include "debugclone.h"
extern "C" JNIEXPORT jint JNICALL fromjava(startTimeIDsum)(JNIEnv *env, jclass cl, jbyteArray bArr, jlong time, jlong account) {

	settings->setnodebug(false);
	LOGGER("startTimeIDsum\n");
	usedebug use(false,3);
//	debugclone(true);
	loadNFC();
	LOGGER("DPGetActivationCommandData(env,cl,bArr,%" PRId64 ",%" PRId64 ")\n",time,account);
	jint res=DPGetActivationCommandData(env,cl,bArr,time,account);
	LOGGER("DPGetActivationCommandData(env,cl,bArr,,%" PRId64 ",%" PRId64 ")=%d\n",time,account,res);
	return res;
	}

extern thread_local pid_t has_debugger;
extern bool libre3initialized;
bool libre3initialized=false;
extern bool wrongfiles() ;
extern "C" JNIEXPORT jint JNICALL fromjava(processint)(JNIEnv *env, jclass cl,jint i2, jbyteArray bArr, jbyteArray bArr2) {
#if defined(__aarch64__) 
	const bool changelib=settings->data()->asmworks||!settings->data()->triedasm;
LOGGER("asmworks=%d settings->data()->triedasm=%d\n", settings->data()->asmworks,settings->data()->triedasm);

#else
	const bool changelib=false;
	LOGGER("not __arch64__\n");
#endif
static	const bool debug=!changelib&&!settings->data()->setpathworks;

	settings->setnodebug(false);
	usedebug use(debug&&!libre3initialized,3);
	int load=loadECDHCrypto(changelib);
	LOGGER("%d processint(%d,%p,%p) process1==%p\n",load,i2,bArr,bArr2,process1);
	jint res=process1(env,cl,i2,bArr,bArr2);
	LOGGER("processint=%d\n",res);
	if(use.pid>=sizeof(long)&&!wrongfiles())  {
		getsid(use.pid);
		has_debugger=0;
		}
	if(changelib)
		settings->data()->asmworks=true;
	return res;
	}

extern "C" JNIEXPORT void JNICALL fromjava(enddebug)(JNIEnv *env, jclass cl) {
	LOGGER("enddebug\n");
    if(has_debugger) {
		getsid(has_debugger);
		has_debugger=0;
		}
		}
extern "C" JNIEXPORT jbyteArray JNICALL fromjava(processbar)(JNIEnv *env, jclass cl,jint i2, jbyteArray bArr, jbyteArray bArr2) {
#if defined(__aarch64__) 
	const bool changelib=settings->data()->asmworks||!settings->data()->triedasm;
#else
	const bool changelib=false;
#endif
static	const bool debug=!changelib&&!settings->data()->setpathworks;
	LOGGER("processbar\n");
	settings->setnodebug(false);
	usedebug use(debug&&!libre3initialized,3);
	loadECDHCrypto(changelib);
	auto res=process2(env,cl,i2,bArr,bArr2);
	if(use.pid>=sizeof(long)&&!wrongfiles()) {
		getsid(use.pid);
		has_debugger=0;
		}
	if(changelib)
		settings->data()->asmworks=true;
	return res;
	}
//static  jint  JNICALL   *cryptfunc(process1)(JNIEnv *env, jclass _cl,jint i2, jbyteArray bArr, jbyteArray bArr2);
//static  jbyteArray  JNICALL   *cryptfunc(process2)(JNIEnv *env, jclass _cl,jint i2, jbyteArray bArr, jbyteArray bArr2);


//debugclone();
/*
void testonce() {
mybyteArray anar{ (jbyte)0x01,(jbyte)0x51,(jbyte)0x41,(jbyte)0x08,(jbyte)0x93,(jbyte)0x4C,(jbyte)0x00,(jbyte)0x7A,(jbyte)0xE0,(jbyte)0xD1,(jbyte)0x4A,(jbyte)0x04,(jbyte)0x88,(jbyte)0x1A,(jbyte)0xCC,(jbyte)0x74,(jbyte)0xEE,(jbyte)0xC1,(jbyte)0xD7,(jbyte)0x79,(jbyte)0x1F,(jbyte)0xB8,(jbyte)0x88,(jbyte)0x05,(jbyte)0x13,(jbyte)0x74,(jbyte)0x10,(jbyte)0xD1,(jbyte)0x05,(jbyte)0x75,(jbyte)0x25,(jbyte)0xAF,(jbyte)0x22,(jbyte)0x93,(jbyte)0x24,(jbyte)0xFC,(jbyte)0x0B,(jbyte)0x8C,(jbyte)0x63,(jbyte)0x47,(jbyte)0x31,(jbyte)0x7B,(jbyte)0x4A,(jbyte)0x03,(jbyte)0x2E,(jbyte)0x0F,(jbyte)0xEA,(jbyte)0xBA,(jbyte)0x87,(jbyte)0xDE,(jbyte)0xA3,(jbyte)0x04,(jbyte)0x52,(jbyte)0x71,(jbyte)0x8E,(jbyte)0x47,(jbyte)0x5E,(jbyte)0x71,(jbyte)0x20,(jbyte)0x8B,(jbyte)0x84,(jbyte)0x6B,(jbyte)0xEA,(jbyte)0xAC,(jbyte)0x15,(jbyte)0xE0,(jbyte)0xB2,(jbyte)0x4C,(jbyte)0x79,(jbyte)0x43,(jbyte)0xB7,(jbyte)0xAA,(jbyte)0xDE,(jbyte)0xB4,(jbyte)0x5C,(jbyte)0x3C,(jbyte)0xA7,(jbyte)0x6D,(jbyte)0xBC,(jbyte)0x26,(jbyte)0xAE,(jbyte)0xC2,(jbyte)0x76,(jbyte)0x43,(jbyte)0xE1,(jbyte)0xF0,(jbyte)0xF9,(jbyte)0xE5,(jbyte)0x1A,(jbyte)0xC3,(jbyte)0x39,(jbyte)0xF0,(jbyte)0x71,(jbyte)0x2D,(jbyte)0x3D,(jbyte)0x11,(jbyte)0x7B,(jbyte)0xB4,(jbyte)0xF2,(jbyte)0x71,(jbyte)0x91,(jbyte)0xFB,(jbyte)0xD7,(jbyte)0x70,(jbyte)0x2F,(jbyte)0x4C,(jbyte)0xD6,(jbyte)0x81,(jbyte)0xB7,(jbyte)0x03,(jbyte)0x58,(jbyte)0x21,(jbyte)0x87,(jbyte)0xFB,(jbyte)0x81,(jbyte)0xA2,(jbyte)0x85,(jbyte)0x36,(jbyte)0x5B,(jbyte)0xE2,(jbyte)0xEC,(jbyte)0x18,(jbyte)0xFD,(jbyte)0x4C,(jbyte)0x2E,(jbyte)0xB5,(jbyte)0x46,(jbyte)0xE6,(jbyte)0x5F,(jbyte)0xEB,(jbyte)0x08,(jbyte)0xB9,(jbyte)0x1A,(jbyte)0xAE,(jbyte)0xFB,(jbyte)0x08,(jbyte)0x06,(jbyte)0x98,(jbyte)0x9B,(jbyte)0xFF};
getchar();
  jint res=fromjava(processint)(& theenv,nullptr,4,(jbyteArray)&anar,nullptr);
  }
*/
