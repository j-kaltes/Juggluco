//#define STATE
//#define LIVE
#ifdef LIBRE3


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
#include "config.h"
#include <jni.h>
#include <string.h>
#include <cinttypes>
#include <thread>
#include "showbarray.hpp"
#include "logs.hpp"
#include "settings/settings.hpp"

static jclass      myFindClass(JNIEnv*, const char* name) {
	LOGGER("FindClass %s\n",name);
	return reinterpret_cast<jclass>(const_cast<char *>(name));
	}
//#define cryptfunc(x) Java_tk_glucodata_ECDHCrypto_ ##x
#define cryptfunc(x) x

extern "C"  {
typedef  jint  JNICALL  (*process1Type)(JNIEnv *env, jclass _cl,jint i2, jbyteArray bArr, jbyteArray bArr2);
static process1Type process1=nullptr;
typedef   jbyteArray  JNICALL   (*process2Type)(JNIEnv *env, jclass _cl,jint i2, jbyteArray bArr, jbyteArray bArr2);
static process2Type process2=nullptr;

#ifdef NFCSHARED
typedef jint JNICALL (*DPGetActivationCommandDataType)(JNIEnv *env, jclass _cl, jbyteArray bArr, jlong, jlong);
static DPGetActivationCommandDataType DPGetActivationCommandData=nullptr;
#endif
};

//static  jbyteArray  JNICALL   (*process2)(JNIEnv *env, jclass _cl,jint i2, jbyteArray bArr, jbyteArray bArr2);
static jint        myRegisterNatives(JNIEnv*, jclass name, const JNINativeMethod*methods, jint nr) {
	LOGSTRING("myRegisterNatives\n");
    const char *namestr=reinterpret_cast<const char *>(name);
	if(!strcmp(namestr,"com/adc/trident/app/frameworks/mobileservices/libre3/security/Libre3SKBCryptoLib")) {
       	process1=(process1Type)methods[0].fnPtr;
		process2=(process2Type)methods[1].fnPtr;
        LOGGER("process1=%p process2=%p\n",process1,process2);
		}
#ifdef NFCSHARED
	else {
		LOGGER("class=%s\n",namestr);
		if(!strcmp(namestr,"com/adc/trident/app/frameworks/mobileservices/libre3/libre3DPCRLInterface")) {
			DPGetActivationCommandData=( DPGetActivationCommandDataType)methods[5].fnPtr;
			LOGGER("register %s%s  =%p\n",	methods[5].name,methods[5].signature,DPGetActivationCommandData);
			}
		}
#endif
	LOGSTRING("end myRegisterNatives\n");
	return 0;
	}
#ifdef DOTESTS	
jboolean    IsSameObject(JNIEnv*, jobject one, jobject two) {
	LOGGER("IsSameObject(JNIEnv*, %p, %p)\n",one,two); 
	return one==two;
	}
#include <vector>
typedef std::vector<uint8_t> mybyteArray;
typedef  mybyteArray* mybyteArrayptr;
void        GetByteArrayRegion(JNIEnv*, jbyteArray ar, jsize start, jsize len, jbyte* uit) {
	LOGGER("GetByteArrayRegion(%p,%d,%d,%p)\n",ar,start,len,uit);
	 mybyteArrayptr inar=reinterpret_cast<mybyteArrayptr>(ar);
	 memcpy(uit,inar->data()+start,len);
	}

jsize       GetArrayLength(JNIEnv*, jarray ar) {
	 mybyteArrayptr inar=reinterpret_cast<mybyteArrayptr>(ar);
	auto res=inar->size();
	LOGGER("GetArrayLength(%p)=%d\n",ar,res);
    return res;
	}


jbyteArray    NewByteArray(JNIEnv*, jsize len) {
    try {
        auto *uit=new mybyteArray(len);
        LOGGER("NewByteArray(%d)=%p\n",len,uit);
        return (jbyteArray)uit;
        }
    catch(std::exception &e) {
        LOGGER("NewByteArray(%d) %s\n",len,e.what());
        return nullptr;
        }
    }
void      SetByteArrayRegion(JNIEnv*, jbyteArray jar, jsize start, jsize len, const jbyte* in) {
	 mybyteArrayptr ar=reinterpret_cast<mybyteArrayptr>(jar);
     LOGGER("SetByteArrayRegion(%p,%d,%d,%p)\n",jar,start,len,in);
     memcpy(ar->data()+start,in,len);
    }
static const struct JNINativeInterface envfunctions {
.FindClass=myFindClass,
.IsSameObject=IsSameObject,
.GetArrayLength=GetArrayLength,
.NewByteArray=NewByteArray,
.GetByteArrayRegion=GetByteArrayRegion,
.SetByteArrayRegion=SetByteArrayRegion,
.RegisterNatives =myRegisterNatives 
};
#else
static const struct JNINativeInterface envfunctions {
.FindClass=myFindClass,
.RegisterNatives =myRegisterNatives
};
#endif
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

extern            pathconcat logbasedir;
#define CHANGECODE
#ifdef CHANGECODE
#include "unprotect.hpp"
#endif
#include "hexstr.hpp"
/*
void changelib(uint8_t *from) {
        uint8_t *start= (uint8_t*)from+2432 +7560 -4;
        uint8_t was[]={0xE1,0x00,0x00,0x94};
        hexstr isnow(start,4);
   if(!memcmp(start,was,4)) 
        {
        LOGGER("Sign the same =%s\n",isnow.str());
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
#ifndef NOLOG
        hexstr isnow(start,4);
#endif
   if(!memcmp(start,was,4)) 
        {
        LOGGER("Sign the same =%s\n",isnow.str());
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

extern bool globalsetpathworks;
extern bool rootcheck;
static bool doOnLoad(std::string_view libname,bool change) {
#if defined(__aarch64__) 
   if(settings->data()->triedasm&&!settings->data()->asmworks&& globalsetpathworks) 
#else
	if(globalsetpathworks)  
#endif	
	{
		extern pathconcat mkbindir(std::string_view subdir,std::string_view libname );
		const static pathconcat libre3dir=mkbindir("bin","libinit.so");
		setenv("PATH", libre3dir.data(), 1);
		}
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
	LOGSTRING("found OnLoad\n");

#if defined(__aarch64__) 
	if(change) {
	     changelib((uint8_t*)OnLoad);
		}
#endif
	JavaVM vmptr{.functions=&myvm};



	OnLoad(&vmptr,nullptr);
	LOGSTRING("after OnLoad\n");
  // gumshim_install_FUN_f4072318("liblibre3extension.so");
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

	return res;
	}
//extern "C" JNIEXPORT jboolean JNICALL fromjava(loadNFC)(JNIEnv *env, jclass thiz) {

#include "debugclone.hpp"
#ifdef NFCSHARED
static bool loadNFC() {
	if(DPGetActivationCommandData)
		return true;
	LOGSTRING("loadNFC\n");
	return doOnLoad("/libcrl_dp.so",false);
	/*
	LOGSTRING(	"after OnLoad\n");
	const char classname[]="tk/glucodata/libre3/NFC";
	jclass cl=env->FindClass(classname);
	if(!cl) {
		LOGGER("Can't find %s\n",classname);
		return false;
		}
	LOGGER("found %s\n",classname);
	const JNINativeMethod  funcs[]{{ "DPGetActivationCommandData", "([BJJ)I",(void*) DPGetActivationCommandData}};
	int rc=env->RegisterNatives(cl, funcs, std::size(funcs));
	LOGSTRING("after RegisterNatives\n");
   	env->DeleteLocalRef(cl);
	  if (rc != JNI_OK)  {
		    LOGGER("RegisterNatives %s failed\n",classname);
		    return false;
		    }
	LOGGER("RegisterNatives %s OK\n",classname);
	return true;	 */
	}
extern "C" JNIEXPORT jint JNICALL fromjava(startTimeIDsum)(JNIEnv *env, jclass cl, jbyteArray bArr, jlong time, jlong account) {

	settings->setnodebug(false);
	LOGSTRING("startTimeIDsum\n");
	usedebug use(false,3);
//	debugclone(true);
	loadNFC();
	LOGGER("DPGetActivationCommandData(env,cl,bArr,%" PRId64 ",%" PRId64 ")\n",time,account);

#if defined(__arm__) 
	const jlong other=time;
	jint res=DPGetActivationCommandData(env,cl,bArr,account,other); //TODO: what does time?
	//SOMEHOW the time ended up in the account position and in the time sits something unpredictable. 
	//By putting account in the time, you can still transfer it accross reinstalls; I can scan again with Juggluco. 
	// The Libre3 app start counting down again. Because the activition time was said to be in 2073
#else
	jint res=DPGetActivationCommandData(env,cl,bArr,time,account);
#endif
	LOGGER("DPGetActivationCommandData(env,cl,bArr,,%" PRId64 ",%" PRId64 ")=%d\n",time,account,res);
	return res;
	}
#endif
extern thread_local pid_t has_debugger;
extern bool libre3initialized;
bool libre3initialized=false;
extern bool wrongfiles() ;
extern "C" int libre3_gum_root_encoder_install_vtable_wrapper_current_20260521(void);


#if defined(IO)||defined(LIVE)
static std::vector<uint8_t> copy_jbytearray(JNIEnv *env, jbyteArray a) {
    std::vector<uint8_t> v;
    if (!a) return v;
    const jsize n = env->GetArrayLength(a);
    if (n <= 0) return v;
    v.resize(static_cast<size_t>(n));
    env->GetByteArrayRegion(a, 0, n, reinterpret_cast<jbyte *>(v.data()));
    return v;
   }
#endif



extern "C" JNIEXPORT jint JNICALL fromjava(processint)(JNIEnv *env, jclass cl,jint i2, jbyteArray bArr, jbyteArray bArr2) {
#if defined(__aarch64__) 
	const bool changelib=settings->data()->asmworks||!settings->data()->triedasm;
LOGGER("asmworks=%d settings->data()->triedasm=%d\n", settings->data()->asmworks,settings->data()->triedasm);

#else
	const bool changelib=false;
	LOGSTRING("not __arch64__\n");
#endif
	LOGGER("setpathworks=%d libre3initialized=%d\n",globalsetpathworks,libre3initialized);
static	const bool debug=!changelib&&!globalsetpathworks;

	settings->setnodebug(false);
	usedebug use(debug&&!libre3initialized,3);
#ifndef NOLOG
	int load=
#endif
              loadECDHCrypto(changelib);


jint res=process1(env,cl,i2,bArr,bArr2);



#ifndef NOLOG
      showbarray in1(env,bArr);
      showbarray in2(env,bArr2);
    LOGGER("processint(%d,%s#%d, %s#%d)=%d\n",i2 ,in1.str,in1.len,in2.str,in2.len,res);
#endif

	if(use.pid>=sizeof(long)&&!wrongfiles())  {
		getsid(use.pid);
		has_debugger=0;
		}
	if(changelib)
		settings->data()->asmworks=true;


	return res;
	}




extern "C" JNIEXPORT jbyteArray JNICALL fromjava(processbar)(JNIEnv *env, jclass cl,jint i2, jbyteArray bArr, jbyteArray bArr2) {
#if defined(__aarch64__) 
	const bool changelib=settings->data()->asmworks||!settings->data()->triedasm;
#else
	const bool changelib=false;
#endif
static	const bool debug=!changelib&&!globalsetpathworks;
	settings->setnodebug(false);
	usedebug use(debug&&!libre3initialized,3);
	loadECDHCrypto(changelib);



auto res = process2(env, cl, i2, bArr, bArr2);





	if(use.pid>=sizeof(long)&&!wrongfiles()) {
		getsid(use.pid);
		has_debugger=0;
		}
	if(changelib)
		settings->data()->asmworks=true;

#ifndef NOLOG
      showbarray in1(env,bArr);
      showbarray in2(env,bArr2);
      showbarray uit(env,res);
//    hexstr in1(ba
	LOGGER("processbar(%d,%s#%d,%s#%d)=%s#%d\n",i2,in1.str,in1.len,in2.str,in2.len,uit.str,uit.len);
#endif
	return res;
	}


//debugclone();
/*
void testonce() {
mybyteArray anar{ (jbyte)0x01,(jbyte)0x51,(jbyte)0x41,(jbyte)0x08,(jbyte)0x93,(jbyte)0x4C,(jbyte)0x00,(jbyte)0x7A,(jbyte)0xE0,(jbyte)0xD1,(jbyte)0x4A,(jbyte)0x04,(jbyte)0x88,(jbyte)0x1A,(jbyte)0xCC,(jbyte)0x74,(jbyte)0xEE,(jbyte)0xC1,(jbyte)0xD7,(jbyte)0x79,(jbyte)0x1F,(jbyte)0xB8,(jbyte)0x88,(jbyte)0x05,(jbyte)0x13,(jbyte)0x74,(jbyte)0x10,(jbyte)0xD1,(jbyte)0x05,(jbyte)0x75,(jbyte)0x25,(jbyte)0xAF,(jbyte)0x22,(jbyte)0x93,(jbyte)0x24,(jbyte)0xFC,(jbyte)0x0B,(jbyte)0x8C,(jbyte)0x63,(jbyte)0x47,(jbyte)0x31,(jbyte)0x7B,(jbyte)0x4A,(jbyte)0x03,(jbyte)0x2E,(jbyte)0x0F,(jbyte)0xEA,(jbyte)0xBA,(jbyte)0x87,(jbyte)0xDE,(jbyte)0xA3,(jbyte)0x04,(jbyte)0x52,(jbyte)0x71,(jbyte)0x8E,(jbyte)0x47,(jbyte)0x5E,(jbyte)0x71,(jbyte)0x20,(jbyte)0x8B,(jbyte)0x84,(jbyte)0x6B,(jbyte)0xEA,(jbyte)0xAC,(jbyte)0x15,(jbyte)0xE0,(jbyte)0xB2,(jbyte)0x4C,(jbyte)0x79,(jbyte)0x43,(jbyte)0xB7,(jbyte)0xAA,(jbyte)0xDE,(jbyte)0xB4,(jbyte)0x5C,(jbyte)0x3C,(jbyte)0xA7,(jbyte)0x6D,(jbyte)0xBC,(jbyte)0x26,(jbyte)0xAE,(jbyte)0xC2,(jbyte)0x76,(jbyte)0x43,(jbyte)0xE1,(jbyte)0xF0,(jbyte)0xF9,(jbyte)0xE5,(jbyte)0x1A,(jbyte)0xC3,(jbyte)0x39,(jbyte)0xF0,(jbyte)0x71,(jbyte)0x2D,(jbyte)0x3D,(jbyte)0x11,(jbyte)0x7B,(jbyte)0xB4,(jbyte)0xF2,(jbyte)0x71,(jbyte)0x91,(jbyte)0xFB,(jbyte)0xD7,(jbyte)0x70,(jbyte)0x2F,(jbyte)0x4C,(jbyte)0xD6,(jbyte)0x81,(jbyte)0xB7,(jbyte)0x03,(jbyte)0x58,(jbyte)0x21,(jbyte)0x87,(jbyte)0xFB,(jbyte)0x81,(jbyte)0xA2,(jbyte)0x85,(jbyte)0x36,(jbyte)0x5B,(jbyte)0xE2,(jbyte)0xEC,(jbyte)0x18,(jbyte)0xFD,(jbyte)0x4C,(jbyte)0x2E,(jbyte)0xB5,(jbyte)0x46,(jbyte)0xE6,(jbyte)0x5F,(jbyte)0xEB,(jbyte)0x08,(jbyte)0xB9,(jbyte)0x1A,(jbyte)0xAE,(jbyte)0xFB,(jbyte)0x08,(jbyte)0x06,(jbyte)0x98,(jbyte)0x9B,(jbyte)0xFF};
getchar();
  jint res=fromjava(processint)(& theenv,nullptr,4,(jbyteArray)&anar,nullptr);
  }
*/
#ifdef DOTESTS
int  nj_processint(int i2, const mybyteArray *  one,  const mybyteArray *   two) {
      try {
            hexstr hexone(one);
            hexstr hextwo(two);
            LOGGER("start processint(%d,%s#%d,%s)\n",i2,hexone.str(),one?one->size():0,hextwo.str());
            jint res=process1(&theenv,nullptr,i2,(jbyteArray)one,(jbyteArray)two);
            LOGGER("processint(%d,%s,%s)=%d\n",i2,hexone.str(),hextwo.str(),res);
            return res;
            }
      catch(std::exception &e) {
            LOGGER("processint exception %s\n",e.what());
            return -1;
            }
      }
mybyteArrayptr nj_processbar(int i2, const mybyteArray *  one,  const mybyteArray *   two) {
      try {
        hexstr hexone(one);
        hexstr hextwo(two);
        LOGGER("start processbar(%d,%s,%s)\n",i2,hexone.str(),hextwo.str());
        auto res=(mybyteArrayptr) process2(&theenv,nullptr,i2,(jbyteArray)one,(jbyteArray)two);
        hexstr hexres(res);
        LOGGER("processbar(%d,%s,%s)=%s\n",i2,hexone.str(),hextwo.str(),hexres.str());
        return res;
        }
     catch(std::exception &e) {
        LOGGER("processbar exception %s\n",e.what());
        return nullptr;
        }
    }

static void doall() {
LOGAR("start doall");
   settings->setnodebug(false);
   debugclone(true,3);
   loadECDHCrypto(false);
nj_processint(1,nullptr,nullptr);
sleep(1);
static const std::vector<uint8_t> arg2_1{0x1D,0x85,0x8F,0x06,0x02,0x00,0x00,0x00,0x01,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x96,0x95,0x77,0x4B,0x9A,0x04,0x53,0x51,0xFB,0x16,0x0B,0xEC,0x5F,0x49,0xDB,0xDF,0x0D,0xC0,0xCE,0x52,0xFB,0x56,0x5F,0x84,0xE6,0x13,0xB8,0x19,0xAE,0xD3,0xDF,0x91,0x9C,0xE3,0x0A,0x3D,0xD4,0xC0,0x12,0xEA,0xEA,0x70,0xC8,0xCC,0xE2,0x89,0x58,0x40,0x00,0x00,0x00,0x01,0x9B,0xC7,0x79,0x12,0x3D,0x86,0x60,0xB3,0x7E,0x99,0xB4,0xBF,0x10,0xC1,0xC4,0x2C,0x11,0x35,0xB3,0x02,0x5B,0xC9,0xB2,0xEF,0x00,0x00,0x00,0x20,0xE3,0xA1,0xFB,0x17,0x80,0xA1,0x63,0x80,0x2A,0xA0,0xFE,0xB1,0xF2,0x00,0xAC,0x26,0x9A,0x42,0xB2,0x29,0x03,0x8C,0xA6,0xE1,0x4D,0x40,0xEF,0xBC,0x6B,0x7B,0x6A,0xE8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xCE,0xC6,0x67,0xE6,0xC0,0x9D,0x20,0xF5,0xC0,0x33,0xD0,0x61,0xB5,0xFC,0xA1,0x8B,0x39,0x92,0x06,0x8B};
   nj_processint(2,&arg2_1, nullptr);

  {
     std::jthread binder_thread([&] {
 static const        std::vector<uint8_t> arg4_1{0x01,0x71,0x3A,0x73,0x1C,0xB5,0x00,0x7A,0xE0,0xA4,0x4F,0x04,0x10,0x9F,0x4A,0xD3,0x3D,0x00,0xA8,0x6C,0xC2,0x25,0xC5,0xB0,0x6E,0xD3,0x8D,0x12,0x4F,0x72,0x25,0xCA,0x63,0xAE,0xAA,0x0C,0x82,0xE6,0xD5,0xB8,0x85,0x19,0xF8,0xC8,0xCB,0x3C,0xA1,0x95,0xE3,0x10,0x3C,0x6F,0xE8,0x08,0x1E,0x54,0xED,0x41,0xC5,0xAE,0xF6,0x81,0x7E,0x4C,0x1E,0x08,0x27,0x10,0xD4,0xA7,0x53,0x9B,0x44,0xC2,0x76,0x82,0xD6,0x41,0xDC,0x2B,0x0D,0x3D,0xF4,0x06,0x88,0x7E,0x2D,0x09,0x3F,0x2C,0xB8,0x27,0xDC,0x24,0x5A,0x5A,0xF4,0xA3,0x1D,0xC1,0x9F,0x8D,0x0B,0x0A,0x71,0x6C,0x0A,0x92,0xC6,0x4E,0xB2,0xB2,0xDB,0xB4,0x0A,0xA4,0x5A,0x47,0x87,0x4A,0x17,0x80,0xD8,0xAD,0x1C,0x47,0x84,0xD0,0x5C,0x98,0x7F,0xD5,0x35,0x9C,0xD5,0xB0,0xF1,0x25,0x02,0x8A};
        nj_processint(4,&arg4_1,nullptr);
        auto res5=nj_processbar(5,nullptr,nullptr);
        hexstr hex5(res5);
        LOGGER("processbar(5,null,null)=%s\n",hex5.str());
        });
    }
 static const std::vector<uint8_t> arg6_1{0x04,0x5E,0x03,0xA9,0x29,0x03,0x16,0xE7,0xF0,0x92,0xAE,0x14,0x88,0x58,0x45,0x24,0xE3,0xAE,0xA3,0x3B,0xF7,0xDD,0xBD,0xE2,0x47,0xCE,0x79,0x90,0xD0,0xB3,0x04,0xC3,0x15,0x37,0x61,0x0E,0x30,0x6B,0x5D,0x58,0x0B,0x3D,0x76,0x0E,0x43,0x25,0x52,0x15,0xA3,0x53,0x40,0x10,0xFA,0xD1,0xEA,0x22,0x7C,0x9E,0xE2,0x78,0xB6,0x16,0xF2,0xCF,0xE7};
nj_processint(6,&arg6_1,nullptr);

 static const std::vector<uint8_t> arg7_1{0x3C,0x08,0x00,0x00,0x7C,0x1D,0xF2};
 static const std::vector<uint8_t> arg7_2{0x16,0x61,0xDF,0xFD,0xD9,0xCE,0x37,0xE0,0x3E,0xAF,0x06,0x52,0x9D,0xA6,0xA9,0xE9,0x12,0xAD,0x08,0x84,0x8C,0x8E,0x7D,0xB1,0xF4,0x14,0xB0,0x9C,0xBC,0x4E,0xCF,0x70,0x46,0x25,0x1D,0x8C};
auto res7=nj_processbar(7,&arg7_1,&arg7_2);
    hexstr hex7(res7);
    LOGGER("processbar(7)=%s\n",hex7.str());
 static const std::vector<uint8_t> arg8_1{0x3D,0x08,0x00,0x00,0xBF,0xB3,0x80};
 static const std::vector<uint8_t> arg8_2{0x0B,0x2E,0xC4,0xDA,0xA1,0x2B,0xE1,0x9F,0x74,0x2D,0xC2,0x4F,0xE9,0x5B,0x42,0xF9,0xB1,0x81,0xB0,0x2A,0xE6,0x1E,0x00,0xB8,0xC9,0x4A,0x61,0x45,0xFF,0xBB,0x61,0x24,0x4D,0x14,0x50,0x8E,0x66,0x17,0x09,0x07,0x1A,0xEA,0x3D,0x70,0x16,0xC8,0x08,0xBB,0x39,0x41,0x40,0x35,0xB0,0xF0,0xF6,0xB3,0x1A,0xAF,0xDE,0xE9};

auto res8=nj_processbar(8,&arg8_1,&arg8_2);
    hexstr hex8(res8);
    LOGGER("processbar(8)=%s\n",hex8.str());

   auto res9=nj_processbar(9,nullptr,nullptr);
    hexstr hex9(res9);
    LOGGER("processbar(9,null,null)=%s\n",hex9.str());
 }
extern "C" JNIEXPORT void JNICALL fromjava(testLibre3)(JNIEnv *env, jclass cl) {
     std::thread th(doall);
     th.detach();
     }
#endif
#endif
