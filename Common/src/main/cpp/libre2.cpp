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
/*      Tue Apr 30 15:11:24 CEST 2024                                                 */

#define setthreadname(buf) prctl(PR_SET_NAME, buf, 0, 0, 0)

#include <string.h>
#include <jni.h>
#include <assert.h>
#include <filesystem>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dlfcn.h>
#include <link.h>
#include <string_view>
#include "SensorGlucoseData.h"
#include "debugclone.h"
#include "settings/settings.h"
#include "libre2.h"
#include "jnisubin.h"
#include "hexstr.h"
#if !defined(__aarch64__) 
#define GEN2ROOTSHECK 1
#endif
//constexpr const int librewearduration=14*24*60;

typedef struct JNINativeInterface * JNIEnvC;
extern struct JNINativeInterface envbuf;
extern JavaVM *getnewvm();

extern "C" jint         subRegisterNatives(JNIEnv*, jclass name, const JNINativeMethod*methods, jint nr);
#define VISIBLE __attribute__((__visibility__("default")))
//#define DYNLINK
//#undef DYNLINK
#ifdef DYNLINK
#define abbottdec(x) (*x)
#define abbottcall(x) x
#else
#define abbottdec(x) Java_com_abbottdiabetescare_flashglucose_sensorabstractionservice_dataprocessing_DataProcessingNative_##x
#define abbottcall(x) Java_com_abbottdiabetescare_flashglucose_sensorabstractionservice_dataprocessing_DataProcessingNative_##x
#endif
#include "fromjava.h"

static constexpr const unsigned char tokenar[]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x27,0x2B,0xCD,0x3B,0xF3,0xCC,0xA0,0xB8,0x56,0xCE,0x96,0x1F,0xDB,0xA5,0x6B,0xB6,0x17,0xB2,0x1B,0xD6,0x57,0x0D,0xF3,0x5F,0x00,0x6D,0x4C,0xD4,0x62,0x66,0x26,0x54,0x1F,0xF9,0x5C,0x87,0xAE,0x63,0xD9,0x7F,0x53,0x6E,0xB6,0x80,0xE7,0x71,0x2E,0x59,0xCF,0xE4,0x50,0xC0,0xC8,0x53,0x1A,0x33,0x61,0xE2,0xDB,0x32,0xF2,0x2A,0xC1,0xB6};

auto token= (const data_t*)gegs<unsigned char>::newex(tokenar);
jbyteArray casttoken=(jbyteArray)token;
static jbyteArray genjtoken(JNIEnv *env) {
	const int len=std::size(tokenar);
	jbyteArray loc=env->NewByteArray(len);
	env->SetByteArrayRegion(loc, 0, len, (jbyte*)tokenar);
    	auto gl=    (jbyteArray) env->NewGlobalRef(loc);
    	env->DeleteLocalRef(loc);
	return gl;
	}
jbyteArray getjtoken(JNIEnv *env) {
	const static jbyteArray tok=genjtoken(env);
	return tok;
	}

extern "C" {
WAS_JNIEXPORT jint JNICALL abbottdec(getTotalMemorySize)(JNIEnv *env, jobject thiz, jint get,  jbyteArray) =nullptr;
WAS_JNIEXPORT void JNICALL abbottdec(initialize)(JNIEnv *, jobject, jobject)=nullptr;
WAS_JNIEXPORT void JNICALL abbottdec(dpSetMaxAlgorithmVersion)(JNIEnv *, jobject, jint, jbyteArray)=nullptr;
WAS_JNIEXPORT void JNICALL abbottdec(dpSetMaxAlgorithmVersiongen2)(JNIEnv *, jobject, jint, jbyteArray)=nullptr;
WAS_JNIEXPORT jint JNICALL   abbottdec(P1)(JNIEnv *envin, jobject obj, jint i, jint i2,jbyteArray  bArr,jbyteArray  bArr2,jbyteArray token72)=nullptr;
WAS_JNIEXPORT jbyteArray JNICALL abbottdec(P2)(JNIEnv *, jobject, jint, jint, jbyteArray, jbyteArray,jbyteArray token72)=nullptr;


WAS_JNIEXPORT void JNICALL abbottdec(initializeNative)(JNIEnv *, jobject, jobject, jbyteArray)=nullptr;


//WAS_JNIEXPORT void JNICALL abbottdec(dpEnableDebugLogging) (JNIEnv *, jobject);
WAS_JNIEXPORT jobject JNICALL abbottdec(processScan2_7) (JNIEnv *, jobject, jint, jobject, jobject, jobject, jobject, jbyteArray, jbyteArray, jbyteArray, jint, jint, jint, jint, jint, jbyteArray, jbyteArray, jbyteArray, jobject, jobject, jobject, jobject, jobject, jobject, jobject, jobject, jobject, jobject,jbyteArray token72)=nullptr;
WAS_JNIEXPORT jobject JNICALL abbottdec(processScan2_10) (JNIEnv *, jobject, jint, jobject, jobject, jobject, jobject, jboolean vitC,jbyteArray, jbyteArray, jbyteArray, jint, jint, jint, jint,jboolean streaming, jint, jbyteArray, jbyteArray, jbyteArray, jobject, jobject, jobject, jobject, jobject, jobject, jobject, jobject, jobject, jobject,jbyteArray token72)=nullptr;
//JNIEXPORT jobject JNICALL Java_com_abbottdiabetescare_flashglucose_sensorabstractionservice_dataprocessing_DataProcessingNative_processScan JNIEnv *, jobject, jint, jobject, jobject, jobject, jobject, jbyteArray, jbyteArray, jbyteArray,  jint, jint, jint, jint, jint, jbyteArray, jbyteArray, jbyteArray, jobject, jobject, jobject, jobject, jobject, jobject, jobject, jobject, jobject, jobject);
WAS_JNIEXPORT jbyteArray JNICALL abbottdec(getEnableStreamingPayload)(JNIEnv *, jobject, jint, jbyteArray, jbyteArray, jbyte, jint,jbyteArray token72)=nullptr;
WAS_JNIEXPORT jbyteArray JNICALL abbottdec(activationComplete)(JNIEnv *, jobject, jbyteArray, jint, jint, jbyteArray,jbyteArray token72)=nullptr;
WAS_JNIEXPORT jint JNICALL abbottdec(getProductFamily)(JNIEnv *, jobject, jint, jbyteArray,jbyteArray token72)=nullptr;

//WAS_JNIEXPORT jlong JNICALL   abbottdec(getStatusCode) (JNIEnv *env, jobject obj, jstring serial, jint startminbase, jint lastestreadingminbase, jint numinbase, jboolean endedearly) ;

//JNIEXPORT jlong JNICALL Java_com_abbottdiabetescare_flashglucose_sensorabstractionservice_dataprocessing_DataProcessingNative_getStatusCode (JNIEnv *, jobject, jstring, 		  jint, jint, jint, jboolean); 
WAS_JNIEXPORT jboolean JNICALL abbottdec(getPatchTimeValues)(JNIEnv *, jobject, jint, jbyteArray, jobject, jobject,jbyteArray token72)=nullptr;
WAS_JNIEXPORT jbyte JNICALL   abbottdec(getActivationCommand)(JNIEnv *env, jobject obj, jint parsertype, jbyteArray info,jbyteArray token72)=nullptr;

WAS_JNIEXPORT jbyteArray JNICALL  abbottdec( getActivationPayload)(JNIEnv *env, jobject obj, jint parsertype, jbyteArray patchid, jbyteArray info, jbyte person,jbyteArray token72)=nullptr;

//WAS_JNIEXPORT jboolean JNICALL   abbottdec(getNeedsReaderInfoForActivation)(JNIEnv *env, jobject obj, jint parsertype,jbyteArray token72) ;
WAS_JNIEXPORT jbyteArray JNICALL  abbottdec(getStreamingUnlockPayload) (JNIEnv *envin, jobject obj, jint parsertype, jbyteArray sensorident, jbyteArray patchinfo, jbyte person, jint timestamp, jint unlockcount,jbyteArray token72)=nullptr;

WAS_JNIEXPORT jobject JNICALL abbottdec(getNextRegionToRead)(JNIEnv *, jobject, jint, jbyteArray, jbyteArray, jbyteArray, jint,jbyteArray token72)=nullptr;

//JNIEXPORT jbyteArray JNICALL Java_com_abbottdiabetescare_flashglucose_sensorabstractionservice_dataprocessing_DataProcessingNative_getStreamingUnlockPayload (JNIEnv *, jobject, 	    jint,              jbyteArray,           jbyteArray, jbyte, jint, jint);
WAS_JNIEXPORT jobject JNICALL   abbottdec(processStream2_7)(JNIEnv *envin, jobject obj, 
jint parsertype, jobject alarmconf, jobject nonaction, jobject glrange, jobject attenuatinconfig, jbyteArray sensorident, 
#ifdef HASP1
jbyteArray info, 
#endif
jbyte person, jbyteArray bluetoothdata, 
jint startsincebase,jint nusincebase,jint warmup,jint wear,
jbyteArray compostate, jbyteArray attenustate, jbyteArray measurestate,
jobject outstarttime, jobject endtime, jobject confinsert, jobject removed, 
jobject compo, jobject attenu, jobject messstate, 
 jobject algorithresults,jbyteArray token72) =nullptr;
WAS_JNIEXPORT jobject JNICALL   abbottdec(processStream2_10)(JNIEnv *envin, jobject obj, 
jint parsertype, jobject alarmconf, jobject nonaction, jobject glrange, jobject attenuatinconfig, jbyteArray sensorident, 
#ifdef HASP1
jbyteArray info, 
#endif
jbyte person, jbyteArray bluetoothdata, 
jint startsincebase,jint nusincebase,jint warmup,jint wear,jboolean isstreaming,
jbyteArray compostate, jbyteArray attenustate, jbyteArray measurestate,
jobject outstarttime, jobject endtime, jobject confinsert, jobject removed, 
jobject compo, jobject attenu, jobject messstate, 
 jobject algorithresults,jbyteArray token72) =nullptr;
oldprocessStream_t  oldprocessStream=nullptr;
};
#ifdef NOLOG 
#define setfuncname(nam,proc)    {*(void **) (&proc)= methods[it++].fnPtr;}
#else
#define setfuncname(nam,proc)   {if(strcmp(#nam,methods[it].name)) {LOGGER("%s!=%s\n",#nam,methods[it].name);} *(void **) (&proc)= methods[it++].fnPtr;}
#endif
#define setfunc(nam)    setfuncname(nam,nam)
#ifdef NOLOG 
#define setfuncneed(nam)    {if(!nam) {*(void **) (&nam)= methods[it++].fnPtr;}}
#else
#define setfuncneed(nam)   {if(strcmp(#nam,methods[it].name)) {LOGGER("%s!=%s\n",#nam,methods[it].name);} {if(!nam) {*(void **) (&nam)= methods[it].fnPtr;};++it;};}
#endif
jint         subRegisterNatives(JNIEnv*, jclass name, const JNINativeMethod*methods, jint nr) {
	if(nr!=20) {
		LOGGER("RegisterNatives %d\n",nr);
		return 0;
		}
	int it=0;
	setfunc(initializeNative);
//	setfunc(isPatchSupported);
	++it;
	setfunc(getPatchTimeValues);
	setfunc(getProductFamily);
	setfunc(getActivationCommand);
	setfunc(getActivationPayload);
	setfunc(getEnableStreamingPayload);
	setfunc(getStreamingUnlockPayload); 
//	setfuncneed(getStreamingUnlockPayload); //Use version without rootcheck instead. This one is so slow that the sensor hangs up (status=19) on slow devices (e.g.  WearOS watch).
 //  LOGGER("%s %s\n",methods[it].name,methods[it].signature);
//	setfunc(getNeedsReaderInfoForActivation);
	++it;
	setfunc(getTotalMemorySize);
#ifdef NFCMEM
	setfunc(getNextRegionToRead);
#else
	++it;
#endif
	if(methods[it].signature[339]=='Z') {
		LOGAR("processScan2_10");
		setfuncname(processScan,processScan2_10) ;
		}
	else {
		LOGAR("processScan2_7");
		setfuncname(processScan,processScan2_7) ;
		}
	if(methods[it].signature[350]=='Z') {
		LOGAR("processStream2_10");
		setfuncname(processStream,processStream2_10);
		}
	else {
		LOGAR("processStream2_7");
		setfuncname(processStream,processStream2_7);
		}
	//setfunc(getStatusCodeNative);
	++it;
	setfunc(activationComplete);
//	setfunc(dpSetReaderCodeSimMode);
	++it;
	setfuncname(dpSetMaxAlgorithmVersion,dpSetMaxAlgorithmVersiongen2);
//	setfunc(dpEnableDebugLogging);
	++it;
	setfunc(P1);
	setfunc(P2);
        LOGGER("end myRegisterNatives %d\n",it);
        return 0;
        }

#if __ANDROID_API__ >= 24
#define usedlsym(han,name) dlvsym(han,name,nullptr)
#else
#define usedlsym(han,name) dlsym(han,name)
#endif
#ifdef DYNLINK
//#define usedlsym dlsym



extern JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) ;

#define getdynsymname(name,proc)  ( *(void **) (&proc)=usedlsym(dynlibhandle, "Java_com_abbottdiabetescare_flashglucose_sensorabstractionservice_dataprocessing_DataProcessingNative_" #name))

#define getdynsym(name) getdynsymname(name,name)
//#define getdynsym(name)  ( *(void **) (&name)=usedlsym(dynlibhandle, "Java_com_abbottdiabetescare_flashglucose_sensorabstractionservice_dataprocessing_DataProcessingNative_" #name))
#define ds(name) && getdynsym(name)
static void *dynlibhandle=nullptr;
bool linklib(const char *filename) {
LOGGER("linklib %s %p\n",filename,dynlibhandle);
debugclone();

if(dynlibhandle) {
	dlclose(dynlibhandle);
	LOGSTRING("closed oldlib\n");;
	} 
oldprocessStream=nullptr;
//P1=nullptr;
//initializeNative=nullptr;
if((dynlibhandle=dlopen(filename,RTLD_LAZY))) {
//if((dynlibhandle=dlopen(filename,RTLD_NOW))) {
	LOGGER("opened %p\n",dynlibhandle);
	LOGGER("PATH=%s\n",getenv("PATH"));
	if(getdynsym(initialize)
			&& getdynsym(getTotalMemorySize)
			&& getdynsymname(processScan,processScan2_7)&&	getdynsym(getEnableStreamingPayload)&& getdynsym(getProductFamily)
			 &&getdynsym(getPatchTimeValues)
			ds(getActivationCommand)
			ds(getActivationPayload)
//			 ds(getNeedsReaderInfoForActivation)
			ds(getStreamingUnlockPayload)
			&&getdynsymname(processStream,processStream2_7)
			 ds(dpSetMaxAlgorithmVersion)
			ds(activationComplete)
	#ifdef NFCMEM
			ds(getNextRegionToRead)
	#endif
			  ){
			if(getdynsym(P1)) {
				if(getdynsym(P2)) {
					LOGSTRING("have P2\n");
					}
				else {
					LOGGER("no P2 %s\n",dlerror());
					}
				} 
			else {
			      oldprocessStream= (oldprocessStream_t)  processStream2_7;
				}
			LOGSTRING("success\n");
			return true;
			}
		else {
		typedef   jint (*OnLoadtype)(JavaVM* vm, void* reserved) ;
		OnLoadtype OnLoad=(OnLoadtype)usedlsym(dynlibhandle,"JNI_OnLoad");
		if(OnLoad!=JNI_OnLoad) {
		     LOGAR("OnLoad exists");
			extern bool globalsetpathworks;
			if(globalsetpathworks) {
				extern pathconcat mkbindir(std::string_view subdir,std::string_view libname );
				const static pathconcat libre2dir=mkbindir("xbin","libus.so");
				setenv("PATH", libre2dir.data(), 1);
				}
		     jint res=OnLoad(getnewvm(),nullptr);
		     LOGGER("OnLoad returns %d\n",res);
			if(P1)
			     return true;
			  LOGAR("no P1");
			}
		}
	LOGGER("linklib: %s\n",dlerror());
	dlclose(dynlibhandle);
	dynlibhandle=nullptr;
	return false;
	}
else {
	LOGGER("Can't open: %s %s\n",filename,dlerror());
	return  false;
	}
}

#endif
jint parsertype=1095774808;

extern string  encodeStatusCode(jlong j);
/*
static string getstatus(JNIEnv *env, jobject thiz, string_view serial,time_t start,time_t lastread,time_t nu,bool endedearly) {
	const char *str=serial.data();
	 int off=memcmp(str,"E007-",5)?0:5;
	 if((serial.length()-off)<11)
	 	return {};
	jstring jserial=env->NewStringUTF(str+off);
	jlong status=abbottcall(getStatusCode)(env,thiz,jserial,start-basesecs,lastread-basesecs,nu-basesecs,endedearly);
	return encodeStatusCode(status);
	}
	
void testserial(JNIEnv *env, jobject thiz) {
	 jstring jserial=env->NewStringUTF("0M000DV8V60");
	jlong status=abbottcall(getStatusCode)(env,thiz,jserial,345417978,346241271,346242574,0);
	string ststat=getstatus(env, thiz,"0M000DV8V60" ,345417978+basesecs, 346241271+basesecs,346242574+basesecs,0);
	LOGGER("status=%ld %s\n",status,ststat.data());
	}



scanstate::scanstate(string_view prev) {
	constexpr int maxpath=512;
	char buf[maxpath];
	int dirlen=prev.length();
	memcpy(buf,prev.data(),dirlen);
	if(buf[dirlen-1]!='/')
		buf[dirlen++]='/';
	strcpy(buf+dirlen,"composite.dat"); composite=ByteArray::readfile(buf);
	strcpy(buf+dirlen,"attenuation.dat"); attenuation=ByteArray::readfile(buf);
	strcpy(buf+dirlen,"mess.dat"); mess=ByteArray::readfile(buf);
}

scanstate::~scanstate() {
	 delete composite;delete attenuation;delete mess;
	 }
	 */
int gettimezoneoffset(const time_t *timp) {
	 struct tm tmbuf;
 	return timegm(localtime_r(timp,&tmbuf)) - *timp;
	}
data_t *fromjbyteArray(JNIEnv *env,jbyteArray jar,jint len) {
	 jsize lens=env->GetArrayLength(jar);
	 if(len>=0&&lens>len)
	 	lens=len;
	 data_t *dat=data_t::newex(lens);
	env->GetByteArrayRegion(jar, 0, lens, dat->data());
	LOGGER("fromjbyteArray %d\n",lens);
	return dat;
	}
/*
static jint getFamily(JNIEnv *env,jbyteArray info) {
	jint fam=abbottcall(getProductFamily)(env,nullptr, parsertype, info,getjtoken(env));
	LOGGER("getProductFamily()=%d\n", fam);
	if(!fam) {
		fam=abbottcall(getProductFamily)(env,nullptr, parsertype, info,getjtoken(env));
		LOGGER("getProductFamily()=%d\n", fam);
		}
	return fam;
	} */

//timevalues     patchtimevalues(const data_t *info) {
static jint getFamily(const data_t *info) {
	abbottinit();
	debugclone();
	jint fam=abbottcall(getProductFamily)(subenv,nullptr, parsertype, (jbyteArray) info,casttoken);
	LOGGER("getProductFamily()=%d\n", fam);
	if(!fam) {
		fam=abbottcall(getProductFamily)(subenv,nullptr, parsertype, (jbyteArray) info,casttoken);
		LOGGER("getProductFamily()=%d\n", fam);
		}
	return fam;
	}


Abbott::Abbott(JNIEnv *env,string_view basedir,jbyteArray juid, const data_t *info): Abbott(basedir,fromjbyteArray(env,juid),getFamily(info)) {
	if(hist&&hist->getinfo()->wearduration>(60*7*24)) {
		warmup=hist->getinfo()->warmup;
		wearduration=hist->getinfo()->wearduration;
		}
	else {
		timevalues times= patchtimevalues(info) ;
		warmup=times.warmup;
		wearduration=times.wear;
      if(hist) {
		   hist->getinfo()->warmup=warmup;
		   hist->getinfo()->wearduration=wearduration;
          }
		}
	}
extern jclasser AlarmC, NonActiona, GluRange, attconf;
AlgorithmResults *callAbbottAlg(data_t *uid,int startsincebase,scanstate *oldstate,scandata *data,scanstate *newstate,outobj *starttime,outobj *endtime,jobject person,int warmup,int wear) {
	LOGSTRING("start callAbbottAlg/3\n");
	const time_t nutime=data->gettime();
	const int lastsincebase=nutime-basesecs;
	const int timeoffset=1000*gettimezoneoffset(&nutime);
	algobj alarm{&AlarmC},nonAction{&NonActiona};
	algobj range{&GluRange}, attenconf{&attconf};
//	int warmup=60, wear=14*24*60;
//	constexpr const int warmup=60, wear=librewearduration;
	outobj  confinsert, removed, compo, attenu, messstate, outalgres,  OutListPatchEvent;

jnidata_t  hierjnidata={&envbuf,newstate};
JNIEnv *hiersubenv=(JNIEnv *) &hierjnidata;
//	 reinterpret_cast<jnidata_t*>(subenv)->map=newstate;
debugclone();
LOGAR("start processScan ");
const bool vitc=false,isstreaming=true;
jobject result;
if(processScan2_10) {
	 result= abbottcall(processScan2_10)(hiersubenv,nullptr, parsertype, alarm, nonAction, range, attenconf, vitc,(jbyteArray)uid, (jbyteArray) data->info,(jbyteArray)data->data ,startsincebase,lastsincebase,warmup,wear, isstreaming,timeoffset , (jbyteArray)oldstate->get(COMP), (jbyteArray)oldstate->get(ATTE), (jbyteArray)oldstate->get(MESS), (jobject)starttime, (jobject)endtime, confinsert, removed, compo, attenu, messstate, outalgres,OutListPatchEvent, person,casttoken);
	 }
else  {
	result= abbottcall(processScan2_7)(hiersubenv,nullptr, parsertype, alarm, nonAction, range, attenconf, (jbyteArray)uid, (jbyteArray) data->info,(jbyteArray)data->data ,startsincebase,lastsincebase,warmup,wear, timeoffset , (jbyteArray)oldstate->get(COMP), (jbyteArray)oldstate->get(ATTE), (jbyteArray)oldstate->get(MESS), (jobject)starttime, (jobject)endtime, confinsert, removed, compo, attenu, messstate, outalgres,OutListPatchEvent, person,casttoken);
	}
	LOGAR(" end processScan");
	intptr_t res=reinterpret_cast<intptr_t>(result);
	LOGGER("person=%ld\n",reinterpret_cast<outobj *>(person)->toint());
	if( vector<PatchEvent *> *patch= (vector<PatchEvent *> *)OutListPatchEvent.ptr) {
		LOGGER("number of PatchEvent's=%ld\n",patch->size());
		for( PatchEvent *p:*patch) {
			LOGGER("id=%d, errorCode=%d\n",p->getId(),p->getErrorCode());

			delete p;
			}
		delete patch;
		}
AlgorithmResults *alg=(AlgorithmResults *)outalgres.ptr;
		const int isinserted=reinterpret_cast<intptr_t>(confinsert.ptr);
		const int isremoved=reinterpret_cast<intptr_t>(removed.ptr);
#ifndef NOLOG 

		LOGGER("Is %sinserted and %sremoved %s\n",isinserted?"":"not ",isremoved?"":"not ", (alg&&alg->getlsaDetected())?" lsaDetected":"");

#endif
	if(alg) {

#ifndef NOLOG
	jint startminbase=starttime->toint();
	time_t start=basesecs+startminbase;
	{
	char buf[50];
	LOGGER("init starttime %s",ctime_r(&start,buf));

	jint endminbase=endtime->toint();
	time_t end=basesecs+endminbase;
	LOGGER("init endtime %s",ctime_r(&end,buf));
	}


#endif




		alg->setremoved(isremoved);
		alg->setinserted(isinserted);
		}

	if(res==0) {
//		intptr_t nstart=starttime->toint();
//		time_t newstart=nstart+basesecs;
//		printf("old %s, new start time: %s",ctime(&startDate),ctime(&newstart));

		newstate->datpos(FAKE)=oldstate->datpos(FAKE);
		newstate->setpos(COMP,static_cast<data_t *>(compo.ptr));
		newstate->setpos(ATTE,static_cast<data_t *>(attenu.ptr));
		newstate->setpos(MESS,static_cast<data_t *>(messstate.ptr));


		return alg;
		}
	else {

#ifndef NOLOG 
		char buf[50];
		LOGGER("processScan returned %zd %s ",res,ctime_r(&nutime,buf));
#endif
		delete alg;
		return nullptr;
		}
	}

//AlgorithmResults *callAbbottAlg(data_t *uid,int startsincebase,scanstate *oldstate,scandata *data,scanstate *newstate,outobj *starttime,outobj *endtime,jobject person) {
AlgorithmResults *Abbott::callAbbottAlg(int startsincebase,scanstate *oldstate,scandata *data,scanstate *newstate,outobj *starttime,outobj *endtime,jobject person) {

	return ::callAbbottAlg(uid,startsincebase,oldstate,data,newstate,starttime,endtime,person,warmup,wearduration) ;
	}
//extern bool savehistory(AlgorithmResults *res,time_t nutime,nfcdata &nfc,SensorGlucoseData &save) ;
//nfcdata  *lastnfcdata=nullptr;
//extern bool savehistory(const AlgorithmResults *res,time_t nutime,const nfcdata *nfcptr,SensorGlucoseData &save) ;
extern void setusedsensors() ;




extern 	bool	setbluetoothon;
Abbott::scanresult_t Abbott::callAbbottAlg(scandata *data) {
//	const int sensorindex=sensors->sensorindex(serial.data());
	if(sensorindex<0)
		return {nullptr,nullptr};
		
	if(!hist&&!(hist=sensors->getSensorData(sensorindex)))  {
		LOGGER("getSensorData(%s) returns null\n", serial.data());
		return {nullptr,nullptr};
		}
	scanstate *newstate=new scanstate(sensordir,data->gettime());

	time_t nutime=data->gettime();

	time_t startDate=hist->getstarttime();

#ifndef NOLOG
	char timebuf[30];

	LOGGER("startdate: %s",ctime_r(&startDate,timebuf)?timebuf:"null");
#endif
	int startsincebase=startDate-basesecs;
//	const int timelast=nutime-startDate; LOGGER("timelast=%d endsensorsecs=%d\n",timelast,endsensorsecs);
const    int timeleft=hist->officialendtime()-nutime;
LOGGER("timeleft=%d\n",timeleft); 
	if(timeleft<(60*60)&&timeleft>-daysecs) {
//	if(timelast>=endsensorsecs&&timelast<days15) 
		if(!state->datpos(FAKE)) {
		    LOGSTRING("new Fake\n");
		   setbluetoothon=true;
		    delete newstate;
		    delete state;
		    state = initnewsensor( data) ;
		    return {nullptr,nullptr};
		    }
		else
			startsincebase+=daysecs;
		}
	outobj person,starttime,endtime;
	AlgorithmResults *res=callAbbottAlg(startsincebase,state,data,newstate,&starttime,&endtime,person);
	if(res) {
		time_t newnu=basesecs+reinterpret_cast<intptr_t>(endtime.ptr);
		if(res->currentglucose().getValue())
			hist->waiting=false;
		else
			hist->waiting=true;
		decltype(auto) gl=res->currentglucose();
		int gid=gl.getId();
		decltype(auto) reshist=res->history();
extern bool saveSputnik_PG2(const jniHistory &hist,time_t nutime,int nuid,const nfcdata  *nfcptr, SensorGlucoseData &save) ;
		saveSputnik_PG2(reshist,newnu,gid, data, *hist); 
		sensor *senso=sensors->getsensor(sensorindex);
		if(newnu<(nutime-60*3)) {
//			lastscan=nullptr;
			LOGGER("senso->finished was %d, set to 1. %s: newnu=%zd nutime=%zd\n",senso->finished,serial.data(),newnu,nutime);
			senso->finished=1;
            extern void	setstreaming(SensorGlucoseData *hist);
            		setstreaming(hist);
			void setusedsensors() ;
			setusedsensors();
			}
		else {
			LOGGER("senso->finished was %d, set to 0\n",senso->finished);
		//	senso->finished=0;
			hist->saveglucose(data,newnu,gid,gl.getValue(),res->trend(),res->RateOfChange()); 
			auto last= hist->getScandata();
//			lastscan=&last.back();

			newstate->makelink();
			state->removefile();
			delete state;
			state=newstate;
			return {res,&last.back()};
			}

		}
	else
		hist->waiting=true;
	delete newstate; 
	return {res,nullptr};
	}
Abbott::scanresult_t Abbott::ProcessOne(scandata *data) {
	if(!state||!state->good()) {
		delete state;
		state=initnewsensor( data) ;
		if(!state)  
			return {nullptr,nullptr};
			
		else 
			return { Initialized,nullptr}; 
		}
	return callAbbottAlg(data);
	}

//const data_t*  getStreamingPayload(const data_t * sensorident, const data_t * patchinfo, uint32_t tim) ;
//scanstate is a bad idea, I should change it to malloc e.d.
const data_t*  getStreamingPayload(scanstate *stateptr,const data_t * sensorident, const data_t * patchinfo, uint32_t tim) ;
scanstate *Abbott::initnewsensor( scandata *data) {
	LOGAR("initnewsensor");
	jbyteArray juid =(jbyteArray) uid; 
	scanstate inp,newstate(4);
	outobj person,endtime,starttime;
	AlgorithmResults * alg=callAbbottAlg(0,&inp,data,&newstate,&starttime,&endtime,person);
	if(!alg) {
		return nullptr;
		}
	jint startminbase=starttime.toint();
	time_t start=basesecs+startminbase;

	timevalues times= patchtimevalues(data->info) ;

	if(int sindex=sensors->sensorindex(serial.data());sindex>=0) {
		sensorindex=sindex;
		 hist=sensors->getSensorData(sindex);

		if(hist->streamingIsEnabled()==1)
			hist->setbluetoothOn(0);
		}
	else   {
		const char *infodata=(const char *)data->info->data();
		uint32_t bluestart;
		scanstate astate;
     if(getStreamingPayload(&astate,uid, data->info, start)) {
			bluestart=0;
			}
		else
			bluestart=SensorGlucoseData:: bluestartunknown;
		SensorGlucoseData::mkdatabase(sensordir,start,(const char *)uid->data(),infodata,&times,3,bluestart,nullptr) ;
		int ind=sensors->addsensor(serial.data());
/*		sensor *sen=sensors->getsensor(ind);
		sen->halfdays=times.wear/(30*24)+1;*/

		hist=sensors->getSensorData(ind);
		sensorindex=ind;
extern bool streamHistory() ;

		void setstartedwithStreamhistory();
		if(streamHistory()) 
			setstartedwithStreamhistory();
		}
   sensor *sen=sensors->getsensor(sensorindex);
   sen->halfdays=times.wear/(30*24)+1;

	constexpr const int	threeyear=94694400 ;//2013
	constexpr const int sec_per_day=60*60*24;
scanstate *newstateptr=new scanstate(sensordir,data->gettime());
jnidata_t  hierjnidata={&envbuf,newstateptr};
JNIEnv *hiersubenv=(JNIEnv *) &hierjnidata;

	data_t *messuit;

//const int timelast=data->gettime()-start;

const int timeleft=hist->officialendtime()-data->gettime();
const bool oldsensor=timeleft<=(60*60);
LOGGER("timeleft=%d oldsensor=%d\n",timeleft,oldsensor); 
if(oldsensor&&timeleft>-daysecs) {
//if(timelast>=endsensorsecs&&timelast<days15) 
	startminbase+=daysecs;
	LOGSTRING("Tijdelijk erbij\n");
	}
	LOGSTRING("voor activationComplete ");
	debugclone();//Al gedaan
	if( (messuit=reinterpret_cast<data_t *>( abbottcall(activationComplete)(hiersubenv,nullptr,juid, (startminbase > threeyear ? startminbase - threeyear : 0) / sec_per_day, startminbase, (jbyteArray)newstate.get(MESS),casttoken)))&&messuit->length()>0) {
			LOGSTRING(" activationComplete>0\n");
		newstateptr->makenull(COMP);	//NODIG?
		newstateptr->makenull(ATTE);	
//		newstateptr->makenull(FAKE);	
		}
	else {
		newstateptr->reset();
		data_t *messwas=newstate.get(MESS);
		messuit=newstateptr->alloc(messwas->length());
		memcpy(messuit->data(),messwas->data(),messwas->length());
		}
	if(oldsensor)
	    newstateptr->datpos(FAKE)=1;
		
	delete alg;
	newstateptr->makelink();
	newstateptr->setpos(MESS,messuit);
	return newstateptr;
	}


#include "environ.h"
static char *inenv(char **env,string_view str)  {
	 const char *name {str.data()};
	 int len=str.length();
	for(char **ptr=env;*ptr;ptr++) {
		if(!memcmp(name,*ptr,len))
			return *ptr;
		}
	return nullptr;
	}
extern "C" char **environ VISIBLE;
char *package=nullptr;
int packagelen;

//extern string_view filesdir;


extern std::string_view libdirname;
static std::string_view thislibname(void) {
    Dl_info dl_info;
    dladdr((void*)thislibname, &dl_info);
    const char *libgname=dl_info.dli_fname;
   const char *libgnameend=strrchr(libgname,'/');
   std::string_view uit;
   if(libgnameend) {
	const int libdirlen=libgnameend-libgname;
   char *newpath=new char[libdirlen+1];
	memcpy(newpath,libgname,libdirlen);
	newpath[libdirlen]='\0';
	uit= std::string_view(newpath,libdirlen);
	}
    else {
       char *newpath=new char[PATH_MAX];
       getcwd(newpath,PATH_MAX);
       uit= std::string_view(newpath);
       }
       LOGGER("libpathname=%s\n",uit.data());
      return uit;
   } 
#ifdef NOTAPP 
std::string_view libdirname=thislibname();
#else
std::string_view libdirname;
#endif
//inline		 const char * const libgname=owerjslkdfjlsdQQ();
//inline		const char *libgnameend=strrchr(libgname,'/');
/*(
static const char *owerjslkdfjlsdQQ(void) {
    Dl_info dl_info;
    dladdr((void*)owerjslkdfjlsdQQ, &dl_info);
    const char *path=dl_info.dli_fname;
    if(*path=='/')
    	return path;
   return realpath(  path, newpath);
   }  */
/*
static const char *owerjslkdfjlsdQQ(void) {
    Dl_info dl_info;
    dladdr((void*)owerjslkdfjlsdQQ, &dl_info);
    return(dl_info.dli_fname);
   }*/
struct packagepath {
static constexpr const char packstart[]= "package:";
static constexpr const char lib[]="/libcalibrate.so";
static constexpr int packstartlen= sizeof(packstart)-1;
int ver2pos;
  char *package;
  int packagelen;
  packagepath() {
  	}
  packagepath(std::string_view base) {
  	LOGGER("packagepath %s\n",base.data());
	const int filesdirlen=base.length();
	packagelen=sizeof(lib)+filesdirlen+packstartlen;
	package= new char[packagelen+1];
	memcpy(package,packstart,packstartlen);
	memcpy(package+packstartlen,base.data(),filesdirlen);
	memcpy(package+filesdirlen+packstartlen,lib,sizeof(lib));
	ver2pos=filesdirlen+packstartlen+sizeof(lib)-5;
	LOGGER("before linklib %s\n",package);
	}
  void move(packagepath &&ander) {
  	package=ander.package;
	packagelen=ander.packagelen;
	ander.package=nullptr;
	ander.packagelen=0;
  	}
  void copy(packagepath &ander) {
  	packagelen=ander.packagelen;
  	package=new char[packagelen+1];
	memcpy(package,ander.package,packagelen);
	package[packagelen]='\0';
  	}
  packagepath(packagepath &&ander) {
  	move(std::move(ander));
  	}
  packagepath(packagepath &ander) {
  	copy(ander);
  	}
   packagepath &operator= (packagepath &&ander) {
    	move(std::move(ander));
	return *this;
   	}
   packagepath &operator= (packagepath &ander) {
    	copy(ander);
	return *this;
   	}
void make2()  {
	LOGSTRING("Use gen 2 lib\n");
	package[ver2pos]='2';
  	}
  const char *getlib() const {
  		return package+packstartlen;
		}
~packagepath() {
	LOGGER("~packagepath() %s\n",package?package:"null");
	delete[] package;
	}
void	addend() {
	package[packagelen-1]='\n';
	package[packagelen]='\0';
	::package=package;
	::packagelen=packagelen;
	}
   };

packagepath packager;
struct nativelibpath:packagepath {
	nativelibpath(std::string_view libdirname):packagepath(libdirname) {
		}
	};

/*
nativelibpath nativelib;
bool nativeexist() {
	static const nativelibpath nativelib;
	if(access(nativelib.getlib(),R_OK|X_OK)) {
		flerror("access %s\n", nativelib.getlib());
		return false;
		}
	return true;
	} */
bool userver2() {
	extern int getlastGen();
	return getlastGen()==2;
	}
//bool	norootchecklib=false;
bool doinitnative=false;;
bool rootcheck=false;

static int doabbottinit(bool dochmod) {
LOGAR("doabbottinit");
      settings->data()->crashed=true;

//	norootchecklib=false;
	if(char *str=inenv(environ,"TZ")) {
		envptrs[std::size(envptrs)-2]=str;
		}
	JNIEnv *env=subenv;
	jobject thiz=nullptr;
	environ=envptrs;
#ifdef DYNLINK
packagepath pak(globalbasedir);
const char *libpath=pak.getlib();
#ifdef DATALIB
	bool haslib=(access(libpath,R_OK)==0);
	if(haslib) {
		LOGAR("haslib");
		if(dochmod) {
			if(chmod(libpath,0400)!=0)
				flerror("chmod(%s,0400)\n",libpath);
			}
		haslib=linklib(libpath);
		}
	if(haslib)  {
		packager=pak;
		}
	else {
		LOGGER("linking failed %s\n",libpath);
#else
unlink(libpath);
 {
#endif
#ifndef CARRY_LIBS
		LOGAR("doesn't carry libs");
		return -1;	
#else
	bool gen2= 
#ifdef GEN2ROOTSHECK
   settings->data()->streamHistory||userver2();
#else
   true;
#endif

	bool firstsuccess=false;
	extern bool hasGen1;
	  LOGGER("libdirname=%s\n",libdirname.data());
	nativelibpath  native(libdirname);
//	if(hasGen1||!gen2) 
	if(!gen2) {
		LOGAR("!gen2");
		extern thread_local pid_t has_debugger;
		if(has_debugger) {
			getsid(has_debugger);
			has_debugger=0;
			} 
		settings->setnodebug(true);
		if(!oldprocessStream) {
			libpath=native.getlib();
			firstsuccess=linklib(libpath) ;
			if(firstsuccess) {
				packager=native;
				void setendedwithStreamhistory();
				setendedwithStreamhistory();
				}
			}
		}

	if(!firstsuccess||gen2) {
		void setstartedwithStreamhistory();
		setstartedwithStreamhistory();
		LOGAR("native.make2()");
		native.make2();
		settings->setnodebug(
#ifdef GEN2ROOTSHECK
      false
#else
     true 
#endif
      );
		libpath=native.getlib();
		if(!linklib(libpath)) {
			LOGGER("linking failed %s\n",libpath);
			return -1;	
			}
		packager=native;
		}
#endif
		}
	#endif
	packager.addend();
	if(initializeNative) {
		LOGGER("Before initializeNative %s\n",package);
		doinitnative=true;	
		javaAttached=false;
		abbottcall(initializeNative)(env,thiz, thiz,nullptr);
		doinitnative=false;	
		LOGAR("after initializeNative 1");
		abbottcall(dpSetMaxAlgorithmVersiongen2)(env,thiz, 3,casttoken);
		LOGSTRING("After dpSetMaxAlgorithmVersiongen2\n");
		}
	else  {
		LOGGER("Before initialize %s\n",package);
		abbottcall(initialize)(env,thiz, thiz);
		LOGSTRING("After initialize\n");
		abbottcall(dpSetMaxAlgorithmVersion)(env,thiz, 3,casttoken);
		LOGSTRING("After dpSetMaxAlgorithmVersion\n");
		}
	settings->data()->crashed=false;
	//	abbottcall(dpEnableDebugLogging)(env,thiz);


//      const int memsize=abbottcall(getTotalMemorySize)(env,thiz,parsertype,casttoken); LOGGER("getTotalMemorySize()=%d\n",memsize);

	#define DYNLINK3
	#ifdef DYNLINK3
		if(P1) {
			rootcheck=true;
			LOGSTRING("Try P1(0,0,nullptr,nullptr)\n");
			int res=abbottcall(P1)(env,thiz,0,0,nullptr,nullptr,casttoken);
			LOGGER("P1(0,0,nullptr,nullptr)=%d\n",res);
			}
		else {
			settings->setnodebug(true);
			rootcheck=false;
			LOGSTRING("No P1\n");
		}

/*    LOGAR("start getNeedsReaderInfoForActivation ");
    bool res=   abbottcall(getNeedsReaderInfoForActivation)(subenv, nullptr, parsertype,casttoken) ;
    LOGGER("end getNeedsReaderInfoForActivation=%d\n",res); */
	#endif
		return 0;
}

static AlgorithmResults * processBlue(data_t * bluedata, jbyte person,

const data_t *ident,
#ifdef HASP1
const data_t *info,
#endif
scanstate *state,
scanstate *newstate,uint32_t startsincebase,uint32_t nutime,int warmup,int wearduration) ;

scanstate* testinitsensor( scandata *data,data_t *uid,scanstate*) ;

bool nearbymgdl(int get,int crit) {
	return abs(get-crit)<27;
	}

#ifdef TESTPS
#include "pstructs.h"
#endif

extern void usepath() ;
VISIBLE  int abbotttest(string_view dir,scandata &data,scandata &data2,data_t *uidptr,data_t *initinfo,data_t *bluetooth,uint32_t starttijd,uint32_t bluetime,const unsigned char *decr,bool nodebug
#ifdef TESTPS
,std::vector<pstruct*> &ptests
#endif
) {
LOGAR("abbotttest");
libdirname=thislibname();

extern int dontdebug;
dontdebug=nodebug?-1:1;
/*
        struct sigaction sa;
	sa.sa_sigaction=childhandler;
//    sa.sa_handler = sigchld_handler; // reap all dead processes
        sigemptyset(&sa.sa_mask);
        sa.sa_flags =SA_SIGINFO|SA_RESTART;

        if (sigaction(SIGCHLD, &sa, NULL) == -1) {
                lerror("sigaction");
        }

*/
   data_t *infoptr=data.info;
//   data_t *rawptr=data.data;
   uint32_t tijd=data.gettime();
	char *name=new char[dir.size()+1];
	memcpy(name,dir.data(),dir.size());
	name[dir.size()]='\0';
	globalbasedir={name,dir.size()};
settings= new(std::nothrow) Settings(globalbasedir,settingsdat,"NL");
	usepath();

	pid_t debugpid=nodebug?0:debugclone();
	if(abbottinit(true)<0)  {
		fprintf(stderr,"Init failed\n");
		return 12;
		}

 	jbyte com=getactivationcommand(infoptr);
	LOGGER("activation command %i\n",com);
	if(com!=-95)
		return 12;
	scanstate astate(4);
	jnidata_t  hierjnidata2={&envbuf,&astate};
	JNIEnv *hiersubenv2=(JNIEnv *) &hierjnidata2;
	if(data_t *payload = reinterpret_cast<data_t *>(  abbottcall(getActivationPayload)(hiersubenv2, nullptr,  parsertype, (jbyteArray) uidptr, (jbyteArray)  infoptr, 0,casttoken) )) {
//		pathconcat uit(dir,"getActivationPayload"); writeall(uit.data(),payload->data(),payload->length());
		#include "getActivationPayload.h"
		if(memcmp(payload->data(),getActivationPayload,payload->size()))
			return 12;
		LOGGER("getActivationPayload=%d\n",payload?payload->length():-1);
		}
	else {
		LOGSTRING("getActivationPayload failed");
		return 12;
		}
	int fam= abbottcall(getProductFamily)(hiersubenv2, nullptr, parsertype,  (jbyteArray)  infoptr,casttoken);
	LOGGER("Family=%d\n",fam);
	auto [warm,wear]= patchtimevalues(infoptr);
	if(wear==0) {
		LOGSTRING("patchtimefailure\n");
		return 12;
		}
	LOGGER("warm=%d wear=%d\n",warm,wear);
	scanstate state(4);
       if(const data_t* pay= ::getStreamingPayload(&state,uidptr, infoptr, tijd) ) {
//		pathconcat struit(dir,"getStreamingPayload"); writeall(struit.data(),pay->data(),pay->length());
		LOGGER("getStreamingPayload len=%d\n",pay->size());
		#include "getStreamingPayload.h"
		if(memcmp(getStreamingPayload,pay->data(),pay->size())) {
			return 12;
			}
		}
	else {
       		LOGSTRING("getStreamingPayload failed\n");
		return 12;
       		}

#ifdef TESTPS
	{scanstate state(4);
jnidata_t  hierjnidata3={&envbuf,&state};
	JNIEnv *hiersubenv=(JNIEnv *) &hierjnidata3;
	LOGSTRING("Before ptests\n");
	bool error=false;
	for(auto el:ptests) {
		el->print(stderr);
		if(!el->test( hiersubenv)) {
			LOGSTRING("test failed\n");
			error=true;
			}
		}
	LOGSTRING("After ptests\n");
	}

#endif
	jbyte person=0;
	int lockcount=5000;
	int startsincebase=starttijd-basesecs;
jnidata_t  hierjnidata={&envbuf,&state};
JNIEnv *hiersubenv=(JNIEnv *) &hierjnidata;
	if(data_t *res= reinterpret_cast<data_t *>(abbottcall(getStreamingUnlockPayload)(hiersubenv,nullptr,parsertype,(jbyteArray)uidptr,(jbyteArray)infoptr,person,startsincebase,lockcount,casttoken))) {
		const int uitlen= res->size();
//		pathconcat unluit(dir,"getStreamingUnlockPayload"); writeall(unluit.data(),res->data(),res->length());
		LOGGER(" getStreamingUnlockPayload len=%d\n",uitlen);
		#include "getStreamingUnlockPayload.h"
		if(memcmp(res->data(),getStreamingUnlockPayload,res->size()))
			return 12;
		}
	else {
		LOGSTRING("getStreamingUnlockPayload==null\n");
		return 12;
		}

	scanstate *newstateptr=new scanstate(defaultscanstate);
	if(scanstate *state=testinitsensor( &data,uidptr,newstateptr)) {
		LOGSTRING("Initsensor succeeded\n");
		scanstate newstate(4);

AlgorithmResults *res=processBlue(bluetooth, 0, uidptr,
#ifdef HASP1
initinfo,
#endif
state,
&newstate,startsincebase,bluetime,60,14*24*60) ;
		if(!res) {
			LOGSTRING("processStream failed\n");
			return 12;
			}
		else {
			const GlucoseNow & gl=res->currentglucose();
			const int gluc=gl.getValue();
			LOGGER("processStream %d %.1f (%d) rate=%.1f",gl.getQuality(),gluc/convfactordL,gluc,gl.rate());
			if(!nearbymgdl(gluc,96))
				return 12;;
			scanstate statescan(4);
			outobj person,endtime,starttime;
			if(AlgorithmResults *alg2=callAbbottAlg(uidptr,startsincebase,&newstate,&data2,&statescan,&starttime,&endtime,person,60,(60*24*14))) {
				if(!nodebug) {
					__attribute__((used)) static	pid_t stop=getsid(debugpid);
					}
				const GlucoseNow & gl=alg2->currentglucose();
				const int gluc=gl.getValue();
				if(memcmp(data2.data->data(),decr,data2.data->size())) return 12;
			        LOGGER("processScan %d %.1f (%d) rate=%.1f",gl.getQuality(),gluc/convfactordL,gluc,gl.rate());
				if(!nearbymgdl(gluc,78)) 
					return 12;;
				}
			else {
				LOGSTRING("processScan failed\n");
				return 12;
				}
			}
		}
	else
		return 12;

extern bool askswrongfiles;
	LOGGER("END abbotttest %d\n",askswrongfiles);

	if(askswrongfiles)
		return 209;
	return 208;
	}

int *initptr=nullptr;
int  abbottinit(bool doch) {


#ifndef CARRY_LIBS
if(settings&&!settings->data()->havelibrary) {//settings==null for abbotttest 
	LOGSTRING("No library\n");
	return -4;
	}
#endif
static int init=(init=doabbottinit(doch),initptr=&init,init);
return init;
}
int  abbottreinit() {
	if(!initptr) return  abbottinit(false);
	return (*initptr=doabbottinit(false));
	}

timevalues     patchtimevalues(const data_t *info) {
	outobj warmup, wear;
	abbottinit();
	debugclone();
  if(abbottcall(getPatchTimeValues)(subenv,nullptr,parsertype, (jbyteArray) info, warmup, wear,casttoken)) {
      const auto warmupget=(uint16_t)warmup.toint();
      const auto wearget=(uint16_t)wear.toint();
      LOGGER("patchtimevalues warmup=%d wear=%d\n",warmupget,wearget);
		return  {warmupget, wearget};
        	}
     LOGAR("patchtimevalues failed");
     return {0,0};
    }
jbyte  getactivationcommand(const data_t *info) {
	LOGSTRING("start getActivationCommand ");
        jbyte res= abbottcall(getActivationCommand)(subenv,nullptr, parsertype, (jbyteArray) info,casttoken);
	LOGSTRING(" end getActivationCommand\n");
        return res;
        }
/*
data_t  * getactivationpayload(scanstate *state,const data_t * patchid, const data_t * info, jbyte person) {
	 reinterpret_cast<jnidata_t*>(subenv)->map=state;
	
	LOGSTRING("start getActivationPayload ");
	debugclone();
	data_t  * res=(data_t *)   abbottcall(getActivationPayload)(subenv, nullptr,  parsertype, (jbyteArray) patchid, (jbyteArray) info, person) ;
	LOGSTRING("end getActivationPayload\n");
         return res;
        }
	*/
/*	
bool getneedsreaderinfoforactivation()   {
	abbottinit();
	debugclone();
	LOGAR("start getNeedsReaderInfoForActivation ");
	bool res=   abbottcall(getNeedsReaderInfoForActivation)(subenv, nullptr, parsertype,casttoken) ;
	LOGGER("end getNeedsReaderInfoForActivation=%d\n",res);
	return res;
	} */
extern "C" JNIEXPORT jboolean JNICALL fromjava(hasRootcheck)(JNIEnv* env, jclass obj) {
	return rootcheck;
	}
extern "C" JNIEXPORT jbyte JNICALL fromjava(activationcommand)(JNIEnv* env, jclass obj,jbyteArray info) {
	abbottinit();
	debugclone(); //Nodig?
	LOGAR("start getActivationCommand");
   jbyte res=  abbottcall(getActivationCommand)(env,obj, parsertype, info,getjtoken(env));
	LOGAR(" end getActivationCommand");
	return res;
	}
extern "C" JNIEXPORT jbyteArray  JNICALL fromjava(activationpayload)(JNIEnv *env, jclass obj,  jbyteArray patchid, jbyteArray info, jbyte person)  {
	abbottinit();
	debugclone(); //Nodig?
	LOGSTRING("start getActivationPayload ");
	jbyteArray res=   abbottcall(getActivationPayload)(env, obj,  parsertype,  patchid,  info, person,getjtoken(env)) ;
	LOGSTRING("end getActivationPayload\n");
	return res;
        }

bool streamHistory() {
	return processStream2_10;
	}

//info and startsincebase should be the same as used by getEnableStreamingPayload
#define MODTIME
data_t * unlockKeySensor( SensorGlucoseData *usedhist,scanstate *stateptr) {
	jbyte person=0;
const auto [bluestart,info]= usedhist->getbluedata();

	jint startsincebase= bluestart-basesecs; 
jnidata_t  hierjnidata={&envbuf,stateptr};
JNIEnv *hiersubenv=(JNIEnv *) &hierjnidata;
	if(abbottinit())  {
		LOGAR("abbottinit()");
		return nullptr;
		}
	debugclone();
	auto lock= usedhist->nextlock();
	LOGGER("start getStreamingUnlockPayload(env,null,%d,%s,%s,%d,%d,%d,token)\n",parsertype,hexstr(usedhist->getsensorident()).str(),hexstr(info).str(),person,startsincebase,lock);
	data_t *res= reinterpret_cast<data_t *>(abbottcall(getStreamingUnlockPayload)(hiersubenv,nullptr, parsertype, (jbyteArray)usedhist->getsensorident(), (jbyteArray)info, person, startsincebase, lock,casttoken));
	LOGAR(" end getStreamingUnlockPayload");
	return res;
	}
static AlgorithmResults * processBlue(data_t * bluedata, jbyte person,

const data_t *ident,
#ifdef HASP1
const data_t *info,
#endif
scanstate *state,
scanstate *newstate,uint32_t startsincebase,uint32_t nutime,int warmup,int wear) {
	jint nusincebase=nutime-basesecs;
	algobj alarm{&AlarmC},nonAction{&NonActiona};
	algobj range{&GluRange}, attenconf{&attconf};
//	int warmup=60, wear=14*24*60;//TODO set with function
//	constexpr const int warmup=60, wear=librewearduration;//TODO set with function
	outobj  outstarttime,  endtime,confinsert, removed, compo, attenu, messstate, outalgres;
 
jnidata_t  hierjnidata={&envbuf,newstate};
JNIEnv *hiersubenv=(JNIEnv *) &hierjnidata;
abbottinit();
LOGAR("start processStream");
constexpr const bool isstreaming=true;
auto _deb=debugclone();
LOGGER("before proc oldprocessStream=%p\n",oldprocessStream);
intptr_t  res=
(oldprocessStream?
reinterpret_cast<intptr_t>(abbottcall(oldprocessStream)(hiersubenv,nullptr, parsertype, alarm, nonAction, range, attenconf, (jbyteArray)ident, 
	person, (jbyteArray)bluedata, 

 startsincebase, nusincebase, warmup, wear,
(jbyteArray)state->get(COMP), (jbyteArray)state->get(ATTE), (jbyteArray)state->get(MESS), 

 outstarttime,  endtime,  confinsert,  removed, 


 compo,  attenu,  messstate, 
outalgres)): (processStream2_10==nullptr?reinterpret_cast<intptr_t>(abbottcall(processStream2_7)(hiersubenv,nullptr, parsertype, alarm, nonAction, range, attenconf, (jbyteArray)ident, 
#ifdef HASP1
 (jbyteArray)info, 
#endif
	person, (jbyteArray)bluedata, 

 startsincebase, nusincebase, warmup, wear,
(jbyteArray)state->get(COMP), (jbyteArray)state->get(ATTE), (jbyteArray)state->get(MESS), 

 outstarttime,  endtime,  confinsert,  removed, 


 compo,  attenu,  messstate, 
outalgres,casttoken)): reinterpret_cast<intptr_t>(abbottcall(processStream2_10)(hiersubenv,nullptr, parsertype, alarm, nonAction, range, attenconf, (jbyteArray)ident, 
#ifdef HASP1
 (jbyteArray)info, 
#endif
	person, (jbyteArray)bluedata, 

 startsincebase, nusincebase, warmup, wear,isstreaming,
(jbyteArray)state->get(COMP), (jbyteArray)state->get(ATTE), (jbyteArray)state->get(MESS), 

 outstarttime,  endtime,  confinsert,  removed, 


 compo,  attenu,  messstate, 
outalgres,casttoken))));

#ifndef NOLOG
int isremoved=reinterpret_cast<intptr_t>(removed.ptr);
int isinserted=reinterpret_cast<intptr_t>(confinsert.ptr);
LOGGER("end processStream %ld, %sinserted and %sremoved\n",res, isinserted?"":"not ", isremoved?"":"not ");
#endif

if(res)
	return nullptr;

newstate->datpos(FAKE)=state->datpos(FAKE);
newstate->setpos(COMP,static_cast<data_t *>(compo.ptr));
newstate->setpos(ATTE,static_cast<data_t *>(attenu.ptr));
newstate->setpos(MESS,static_cast<data_t *>(messstate.ptr));
return (AlgorithmResults *)outalgres.ptr;
};


//extern std::span<streamdat> laststream;

extern bool addStreamHistory(const jniHistory &hist,time_t nutime,int nuid, SensorGlucoseData &save) ;

const AlgorithmResults *  libre2stream::processTooth(data_t * bluedata, scanstate *newstate,uint32_t nutime) {

const data_t *ident=getident();
#ifdef HASP1
const data_t *info=getinfo();
#endif

if(hist->getinfo()->wearduration<(7*24*60)) {
	 auto [warmup,wear]= patchtimevalues(info) ;
   hist->getinfo()->warmup=warmup;
   hist->getinfo()->wearduration=wear;
   }
const AlgorithmResults * alg= processBlue(bluedata, person,

ident,
#ifdef HASP1
info,
#endif
state,
newstate, startsincebase,nutime,hist->getinfo()->warmup,hist->getinfo()->wearduration);
if(alg) {
		/*int insert=reinterpret_cast<intptr_t>(confinsert.ptr);
		int isremoved=reinterpret_cast<intptr_t>(removed.ptr);
		LOGGER("Is %sinserted and %sremoved\n",insert?"":"not ",isremoved?"":"not ");*/

//		AlgorithmResults *alg=(AlgorithmResults *)outalgres.ptr;
		const GlucoseNow & gl=alg->currentglucose();
		const int gluc=gl.getValue();
		const int qual=gl.getQuality();
		int gid=gl.getId();
		LOGGER("processStream %d %d %.1f\n",gid,qual,gluc/convfactordL);
		if(!qual&&gluc) {
			if(hist->savepoll(nutime,gid,gluc,gl.trend(),gl.rate())) {
				if(streamHistory()) {
				         addStreamHistory(alg->history(), nutime,gid, *hist) ;
					}
				return alg;
				}
			else {
				delete alg;
				return ALGDUP_VALUE;

				}
			}
		else {
			delete alg;
			LOGSTRING("bad quality\n");
			}
		}
return nullptr;
}


//WAS_JNIEXPORT jbyteArray JNICALL abbottdec(getEnableStreamingPayload)(JNIEnv *env, jobject obj, jint parsertype, jbyteArray sensorident, jbyteArray info, jbyte person, jint numinbase) { }

const data_t*  getStreamingPayload(scanstate *stateptr,const data_t * sensorident, const data_t * patchinfo, uint32_t tim) {
	jint timeminbase=tim-basesecs;
	jbyte person=0;
jnidata_t  hierjnidata={&envbuf,stateptr};
JNIEnv *hiersubenv=(JNIEnv *) &hierjnidata;
//	basismap.reset(); reinterpret_cast<jnidata_t *>(subenv)->map=&basismap;
	abbottinit();
	debugclone(); //Nodig?
	LOGSTRING("start getStreamingPayload ");
	const data_t *pay= reinterpret_cast<data_t *>(abbottcall(getEnableStreamingPayload)(hiersubenv,nullptr, parsertype, (jbyteArray)sensorident, (jbyteArray)patchinfo, person, timeminbase,casttoken));
	LOGSTRING(" end getStreamingPayload\n");
	return pay;
	}
extern "C" JNIEXPORT jbyteArray  JNICALL fromjava(bluetoothOnKey)(JNIEnv *envin, jclass cl,jbyteArray sensorident, jbyteArray patchinfo) {

      setthreadname( "bluetoothOnKey");
	abbottinit();
	int fam= abbottcall(getProductFamily)(envin, cl, parsertype, patchinfo, getjtoken(envin));

	const data_t *uid=fromjbyteArray(envin,sensorident);
	destruct _destuid([uid]{data_t::deleteex(uid);});
	string serial=getserial(fam,reinterpret_cast<const unsigned char *>(uid->data()));

	time_t starttime=time(NULL);
	const data_t *info=fromjbyteArray(envin,patchinfo);
	destruct _destinfo([info]{data_t::deleteex(info);});

	LOGGER("bluetoothOnKey %s\n",serial.data());
	if(SensorGlucoseData *hist=sensors->getSensorData(serial.data())) {
		LOGSTRING("Old history\n");	
		if(!hist->setbluetooth(starttime,info->data()))
			return nullptr;
		}
	else {
	   timevalues times= patchtimevalues(info) ;
		SensorGlucoseData::mkdatabase(pathconcat(sensorbasedir,serial),starttime,(const char *)uid->data(),(const char *)info->data(),&times,3,starttime,(const char *)info->data());
		int ind=sensors->addsensor(serial.data());
		sensor *sen=sensors->getsensor(ind);
		sen->halfdays=times.wear/(30*24)+1;
		}
	scanstate astate;
	const data_t* pay=getStreamingPayload(&astate,uid, info, starttime); 
	if(pay) {
	 	LOGSTRING("getStreamingPayload returned something\n");
		jint uitlen=pay->size();
		jbyteArray uit=envin->NewByteArray(uitlen);
		envin->SetByteArrayRegion(uit, 0, uitlen, pay->data());
		return uit;
		}
	else
		{
		if(SensorGlucoseData *hist=sensors->getSensorData(serial.data())) {
			hist->setnobluetooth();
//			hist->bluetoothback();
			}
		
	 	LOGSTRING("getStreamingPayload returned null\n");
		}
	return nullptr;
	}
//	return abbottcall(getEnableStreamingPayload)(envin,nullptr, parsertype, sensorident, patchinfo, person, timeminbase);



string getserial(JNIEnv *envin, jclass cl,jbyteArray sensorident,jbyteArray patchinfo) {
	abbottinit();
	LOGAR("getserial");
	int fam= abbottcall(getProductFamily)(envin, cl, parsertype, patchinfo,getjtoken(envin));
	const data_t *uid=fromjbyteArray(envin,sensorident);
	destruct _dest([uid] {data_t::deleteex(uid);});
	return getserial(fam,reinterpret_cast<const unsigned char *>(uid->data()));
	}

extern "C" JNIEXPORT jstring  JNICALL fromjava(getserial)(JNIEnv *envin, jclass cl,jbyteArray sensorident,jbyteArray patchinfo) {
	LOGAR("jgetserial");
	return envin->NewStringUTF(getserial(envin,cl,sensorident,patchinfo).data()+5);
	}
extern "C" JNIEXPORT void  JNICALL fromjava(bluetoothback)(JNIEnv *envin, jclass cl,jbyteArray sensorident,jbyteArray patchinfo) {
	string serial=getserial(envin,cl,sensorident,patchinfo);
	if(SensorGlucoseData *hist=sensors->getSensorData(serial.data())) {
		hist->bluetoothback();
		}
	}

//extern "C" JNIEXPORT jbyteArray  JNICALL fromjava(hasBluetooth)(JNIEnv *envin, jclass cl,jbyteArray sensorident,jbyteArray patchinfo,jlong tim) {
extern "C" JNIEXPORT jboolean  JNICALL fromjava(hasBluetooth)(JNIEnv *envin, jclass cl,jbyteArray sensorident,jbyteArray patchinfo) {
	abbottinit();
	const jint timeminbase= time(NULL)-basesecs;
	LOGAR("start getStreamingPayload");
	debugclone();
	jbyteArray res=abbottcall(getEnableStreamingPayload)(envin,cl, parsertype, sensorident, patchinfo, 0, timeminbase,getjtoken(envin));
	LOGSTRING("end getStreamingPayload\n");
	return res!=nullptr;
	}

#include "hex.h"
bool  setDeviceAddressB(JNIEnv *env,SensorGlucoseData *hist,jbyteArray jaddress ) {
	if(jaddress==nullptr) {
		LOGSTRING("setDeviceAddressB(null)\n");
		return false;
		}
	 jsize len=env->GetArrayLength(jaddress);
	 if(len!=6) {
		LOGGER("setDeviceAddressB(len=%d)\n",len);
	 	return false;
		}
	jbyte address[7];
	env->GetByteArrayRegion(jaddress, 0, len, address);

	char *saddress=hist->deviceaddress();
	for(int i=5;i>=0;i--) {
		*saddress++=showhex<1>(address[i]);
		*saddress++=showhex<0>(address[i]);
		*saddress++=':';
		}
	saddress[-1]='\0';
	LOGGER("setDeviceAddressB(%s)\n",hist->deviceaddress());
	return true;
	}

//extern "C" JNIEXPORT void JNICALL   fromjava(enabledStreaming)(JNIEnv *envin, jclass cl,jbyteArray sensorident,jbyteArray patchinfo,int val) {
extern void	sendstreaming(SensorGlucoseData *hist) ;
extern "C" JNIEXPORT void JNICALL   fromjava(enabledStreaming)(JNIEnv *envin, jclass cl,jbyteArray sensorident,jbyteArray patchinfo,jint val,jbyteArray address) {
	LOGGER("enabledStreaming(%d)\n",val);
      setthreadname( "enabledStreaming");
	if(abbottinit())
		return ;
	int fam= abbottcall(getProductFamily)(envin, cl, parsertype, patchinfo,getjtoken(envin));
	const data_t *uid=fromjbyteArray(envin,sensorident);
	destruct _dest([uid] {data_t::deleteex(uid);});
	string serial=getserial(fam,reinterpret_cast<const unsigned char *>(uid->data()));
	if(SensorGlucoseData *hist=sensors->getSensorData(serial.data())) {
		hist->setbluetoothOn( val); 
		if(val==1) 
			setDeviceAddressB(envin,hist,address);
		setbluetoothon=false;
		sendstreaming(hist);
		}
	}
extern "C" JNIEXPORT void JNICALL   fromjava(USenabledStreaming)(JNIEnv *envin, jclass cl,jbyteArray sensorident,jbyteArray jauth,jbyteArray address) {
	const char uslog[]="USenabledStreaming\n";
        setthreadname(uslog );
	LOGGERN(uslog,sizeof(uslog)-1);
	const int fam=3;
	const data_t *uid=fromjbyteArray(envin,sensorident);
	destruct _dest([uid] {data_t::deleteex(uid);});
	string serial=getserial(fam,reinterpret_cast<const unsigned char *>(uid->data()));
	if(SensorGlucoseData *hist=sensors->getSensorData(serial.data())) {
		hist->setbluetoothOn( 1); 
		setDeviceAddressB(envin,hist,address);
		hist->setsensorgen();
		if(hist->getsensorgen()==2) {
			jsize authlen=envin->GetArrayLength(jauth);
			if(authlen<=12) {
				envin->GetByteArrayRegion(jauth, 0, authlen, (jbyte *)hist->getinfo()->streamingAuthenticationData);
				hist->getinfo()->auth12=authlen==12;
				}
			}
		setbluetoothon=false;
		sendstreaming(hist);
		}
	}
scanstate * testinitsensor( scandata *data,data_t *uid,scanstate *newstateptr) {
	scanstate inp,newstate(4);
	outobj person,endtime,starttime;
	AlgorithmResults * alg=callAbbottAlg(uid,0,&inp,data,&newstate,&starttime,&endtime,person,60,60*24*14);
	if(!alg) {
		LOGSTRING("first callAbbottAlg failed\n");
		return nullptr;
		}
	jbyteArray juid =(jbyteArray) uid; 
	constexpr const int	threeyear=94694400 ;//2013
	constexpr const int sec_per_day=60*60*24;
	jint startminbase=starttime.toint();
//	time_t start=basesecs+startminbase;

jnidata_t  hierjnidata={&envbuf,newstateptr};
JNIEnv *hiersubenv=(JNIEnv *) &hierjnidata;

	data_t *messuit;

	if((messuit=reinterpret_cast<data_t *>( abbottcall(activationComplete)(hiersubenv,nullptr,juid, (startminbase > threeyear ? startminbase - threeyear : 0) / sec_per_day, startminbase, (jbyteArray)newstate.get(MESS),casttoken)))&&messuit->length()>0) {
			LOGSTRING(" activationComplete>0\n");
		newstateptr->makenull(COMP);	//NODIG?
		newstateptr->makenull(ATTE);	
		}
	else {
		newstateptr->reset();
		data_t *messwas=newstate.get(MESS);
		messuit=newstateptr->alloc(messwas->length());
		memcpy(messuit->data(),messwas->data(),messwas->length());
		}
	delete alg;
	newstateptr->setpos(MESS,messuit);
	return newstateptr;
	}




#ifdef NFCMEM
int NfcMemory::nextspan() {
	delete[] prevspan;
	prevspan=nullptr;
	jbyteArray juid =(jbyteArray) uid; 
	jbyteArray jinfo =(jbyteArray) info; 
	jbyteArray jresults =(jbyteArray) results; 
        MemoryRegion *span = (MemoryRegion*)abbottcall(getNextRegionToRead)(subenv,nullptr,parsertype,juid, jinfo, jresults, iter++,casttoken);
	if(span) {
		prevspan=span;
		int start= span->getStart();
		int len=span->getLength();
		int res= start|(len<<16);
		LOGGER("iter=%d nextspan start=%d len=%d %d\n",iter-1,start,len,res);
		return res;
		}
	LOGSTRING("nextspan -1\n");
	return -1;
	}

void NfcMemory::add(data_t *dat) {
	signed char *buf=results->data();	
	memcpy(buf+prevspan->getStart(),dat->data(),prevspan->getLength());
	}
#endif
void closedynlib(void) {
	LOGAR("closedynlib");
	if(dynlibhandle)
		dlclose(dynlibhandle);
	dynlibhandle=nullptr; 
	}

extern "C" JNIEXPORT void  JNICALL   fromjava(closedynlib)(JNIEnv *env, jclass _cl) {
	 closedynlib() ;
	}


