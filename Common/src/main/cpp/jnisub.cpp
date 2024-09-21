
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
/*      Fri Jan 27 12:35:35 CET 2023                                                 */



#include <string.h>
#include <jni.h>
#include <vector>
#include <algorithm>
#undef _GNU_SOURCE 
#define _GNU_SOURCE  
#include "config.h"
#include "inout.hpp"
#include "jnisub.hpp"
#include "logs.hpp"
#include "jnisubin.hpp"
using namespace std;

;
std::unordered_map<string,jclasser*> jclasser::classid(20);
#define subLOGGER(...) LOGGER(__VA_ARGS__)
#define subLOGSTRING(...) LOGSTRING(__VA_ARGS__)

extern "C" jsize       subGetArrayLength(JNIEnv* env, jarray jar) {
	subLOGGER("subGetArrayLength(%p)\n",jar);
	if(!jar)
		return 0;
    	 data_t *ar=	reinterpret_cast<data_t *>(jar);
	 jint len= ar->length();
	subLOGGER("subGetArrayLength=%d\n",len);
	return len;
	 }




extern "C" void         subGetByteArrayRegion(JNIEnv*env,jbyteArray  jar,jsize  start,jsize  len,jbyte * jbyte4) {
    	 data_t *ar=	reinterpret_cast<data_t *>(jar);
   LOGGER("GetByteArrayRegion(%p#%d,%d,%d,%p)\n",jar,ar->size(),start,len,jbyte4);
	memcpy(jbyte4,ar->data()+start,len);

   }
extern "C" void         subSetByteArrayRegion(JNIEnv*env,jbyteArray  jar,jsize  start,jsize  len,const jbyte * const4) {
    	 data_t *ar=	reinterpret_cast<data_t *>(jar);
       LOGGER("SetByteArrayRegion(%p#%d,%d,%d,%p)\n",jar,ar->size(),start,len,const4);
	 memcpy(ar->data()+start,const4,len);

   }
extern "C" jbyte*      subGetByteArrayElements(JNIEnv*env, jbyteArray ar, jboolean* copy) {
	jbyte *buf=reinterpret_cast<data_t *>(ar)->buf;
	subLOGGER("subGetByteArrayElements(%p,%p)=%p\n",ar,copy,buf);
	if(copy) {
		*copy=JNI_FALSE;
		}
	return buf;
	}

extern "C"    void        subReleaseByteArrayElements(JNIEnv*, jbyteArray jar, jbyte* car, jint mode) { 
    subLOGGER("subReleaseByteArrayElements(%p,%p,%d)\n",jar,car,mode);
    }

extern "C"    void        subDeleteLocalRef(JNIEnv*, jobject obj) {
subLOGGER("subDeleteLocalRef %p\n",obj);
}

extern "C"  jbyteArray    subNewByteArray(JNIEnv*subenv, jsize len) {
	jnidata_t *dat=reinterpret_cast<jnidata_t *>(subenv);
	 data_t *res=dat->map->alloc(len);
	 subLOGGER("subNewByteArray(%d)=%p\n",len,res);
	return (jbyteArray)res;
	}

typedef jint (*staticintmethod)(va_list);
extern "C" jint     subCallStaticIntMethod(JNIEnv*env, jclass cl, jmethodID meth, ...) {
 	subLOGSTRING("subCallStaticIntMethod=");
	va_list args;
	va_start(args, meth);
	jint out= ((staticintmethod)meth)(args);
	LOGGER("%d\n",out);
	va_end(args);
	return out;
}
typedef jboolean (*staticbooleanmethod)(va_list);
extern "C" jboolean     subCallStaticBooleanMethod(JNIEnv*env, jclass cl, jmethodID meth, ...) {
 	subLOGSTRING("subCallStaticBooleanMethod=");
	va_list args;
	va_start(args, meth);
	jboolean out= ((staticbooleanmethod)meth)(args);
	LOGGER("%d\n",out);
	va_end(args);
	return out;
}

typedef jobject (*staticObjmethod)(va_list);
extern "C" jobject     subCallStaticObjectMethod(JNIEnv*env, jclass cl, jmethodID meth, ...) {
subLOGSTRING("subCallStaticObjectMethod\n");
	va_list args;
	va_start(args, meth);
	jobject out= ((staticObjmethod)meth)(args);
	va_end(args);
	return out;
}

template <typename T>
 T    subCallMethod(JNIEnv* env, jobject obj, jmethodID meth, ...) {
//	subLOGGER("subCall-%s-Method\n",typeid(T).name());
	subLOGSTRING("subCall--Method\n");
	va_list args;
	va_start(args, meth);
	 T out= (( T (*)(jobject,  va_list) )meth)(obj,args);
	va_end(args);
	return out;
	}
constexpr auto subCallBooleanMethod=subCallMethod<jboolean>;
constexpr auto subCallIntMethod=subCallMethod<jint>;
constexpr auto subCallDoubleMethod=subCallMethod<jdouble>;
//s/\<LOGGER(\"sub/subLOGGER(\"sub/g
extern "C"    void         subCallVoidMethod(JNIEnv* env, jobject obj, jmethodID meth, ...) {
   	subLOGSTRING("subCallVoidMethod\n");
	va_list args;
	va_start(args, meth);
	 (( void (*)(jobject,  va_list) )meth)(obj,args);
	va_end(args);
   }


extern "C" jclass      subFindClass(JNIEnv*env, const char* clname) {
	subLOGGER("start subFindClass %s\n",clname);
	const char *end=strrchr(clname,'/')+1;
	jclass cl= (jclass)jclasser::classid[end];
	subLOGGER("subFindClass %s=%p\n",clname,cl);
	return cl;
    }

class thrower: public _jthrowable {
	string name;
	public:
	thrower(const char *name): name(name) {}
	const string &getname()const {return name;}
	};
    jint         subThrow(JNIEnv*env,jthrowable  jthrowable1) {

	   subLOGGER("subThrow %s\n",static_cast<thrower*>(jthrowable1)->getname().data());
	return 0;
   }

jboolean except=0;
extern "C"    jboolean     subExceptionCheck(JNIEnv*env)  {
   subLOGSTRING("subExceptionCheck\n");
/*	if(except)
		return except;
		*/
	return 0;
   }

   extern "C" void        subExceptionClear(JNIEnv*) {
   subLOGSTRING("subExceptionClear\n");

   	except=0;
   	}

extern "C"    jthrowable subExceptionOccurred(JNIEnv*env) {
	static thrower athrow("Hier");
   	subLOGGER("subExceptionOccurred %ld\n", syscall(SYS_gettid));


//	return new thrower("Hier");
	return &athrow;

    	}	
extern "C" jobject     nothingproc(va_list) {
	LOGSTRING("no function found!!!\n");
	return nullptr;
	}
static jmethodID nothing=(jmethodID) nothingproc;
extern "C"

jmethodID   subGetStaticMethodID(JNIEnv* , jclass cl, const char*name, const char*sig) {
	subLOGGER("subGet...MethodID(JNIEnv*,..,%s,%s)..",name,sig); 
	jclasser* cla=(jclasser *)cl;
	auto funcs=cla->funcs;
	auto it = std::find_if (funcs.begin(), funcs.end(), [name,sig](Method &me){ return !(strcmp(me.name.data(),name)||strcmp(me.pat.data(),sig));});
	if(it!=funcs.end()) {
		subLOGSTRING("found\n");
/*		jmethodID med= (jmethodID)it->func;
		return med;*/
		return (jmethodID)it->func;
		}
	subLOGSTRING("Not found\n");
	return  nothing;
	}
#define mkfieldID(cl,memb) offsetof(cl,memb)+8
template <typename T>
   T *fieldvalue(jobject obj,jfieldID  fieldid)  {
      return reinterpret_cast<T *>(reinterpret_cast<uint8_t *>(obj)+reinterpret_cast<intptr_t>(fieldid)-8);
      }
#ifdef SIBIONICS
#include "sibionics/AlgorithmContext.hpp"
constexpr const int ig_dataField=mkfieldID(AlgorithmContext,ig_data); 
constexpr const int mNativeContextField=mkfieldID(AlgorithmContext,mNativeContext); 
constexpr const int currentWarningField=mkfieldID(AlgorithmContext,currentWarning); 
constexpr const int glucoseWarningField=mkfieldID(AlgorithmContext,glucoseWarning); 
constexpr const int ig_trendField=mkfieldID(AlgorithmContext,ig_trend); 
constexpr const int temperatureWarningField=mkfieldID(AlgorithmContext,temperatureWarning); 

jobject newAlgContext(va_list) {
	LOGAR("newAlgContext(va_list)");
	return reinterpret_cast<jobject>(new AlgorithmContext()); 
	}
#endif


//s/[a-z]\+ \(.*\);/constexpr const int \1Field=offsetof(AlgorithmContext,\1); 
void         subSetLongField(JNIEnv*env,jobject  jobject1,jfieldID  fieldid,jlong  value) {
   LOGGER("SetLongField %llx\n",value);
   *fieldvalue<jlong>(jobject1,fieldid)=value;
   }
void         subSetDoubleField(JNIEnv*env,jobject  jobject1,jfieldID  fieldid,jdouble  value) {
   LOGGER("SetDoubleField %lf\n",value);
   *fieldvalue<jdouble>(jobject1,fieldid)=value;
   }
void         subSetIntField(JNIEnv*env,jobject  jobject1,jfieldID  fieldid,jint  value) {
   LOGGER("SetIntField %i\n",value);
   *fieldvalue<jint>(jobject1,fieldid)=value;
   }


jlong        subGetLongField(JNIEnv*env,jobject  jobject1,jfieldID  fieldid) {
//    uint8_t *alg=reinterpret_cast<uint8_t *>(jobject1);
   jlong value =*fieldvalue<jlong>(jobject1,fieldid);
   LOGGER("GetLongField()=%llx\n",value);
   return value;
   }

const char*  subGetStringUTFChars(JNIEnv*env,jstring  jstring1,jboolean * jboolean2) {
   const char *res=reinterpret_cast<const char *>(jstring1);
   LOGGER("GetStringUTFChars %s\n",res);
   return res;
   }
void         subReleaseStringUTFChars(JNIEnv*env,jstring  jstring1,const char * const2) {
   LOGGER("ReleaseStringUTFChars %s %s\n",const2,((const char *)jstring1)==const2?"equal":"unequal");
   }

jobject      subNewObjectV(JNIEnv*env,jclass  jclass1,jmethodID  jmethodID2,va_list  va_list3) {
	LOGGER("NewObjectV start %p\n",jmethodID2);
	typedef jobject     (*new_t)(va_list args);
	new_t newer= reinterpret_cast<new_t>(jmethodID2);
	auto res=newer(va_list3);
	LOGGER("NewObjectV=%p\n",res);
	return res;
   }



jfieldID     subGetFieldID(JNIEnv*env,jclass  jclass1,const char * name,const char * sig) {
   LOGGER("GetFieldID(%s,%s)\n",name,sig);
   return reinterpret_cast<jfieldID>(subGetStaticMethodID(env , jclass1, name, sig) );
   }
extern "C" jclass      subGetObjectClass(JNIEnv*, jobject ob) {
	subLOGSTRING("subGetObjectClass\n");
	algobj *co=(algobj *)ob;
	return  (jclass)co->cl;
	}

          
jobject     newpatchevent(va_list args) {
	jint id=	 va_arg(args, jint);
	jint code=	 va_arg(args, jint);
	return (jobject) new PatchEvent{id,code};
	}





jobject     newglucosevalue(va_list args) {
	jint id=	 va_arg(args, jint);
	jint qual=	 va_arg(args, jint);
	jint val=	 va_arg(args, jint);
	return (jobject) new GlucoseValue{id,qual,val};

	}

class MemoryRegion {
private:
    const int start;
    const int len;
public:
    MemoryRegion(int start, int len) :start(start),len(len) { }

   int getStart()const { return start; }

   int getLength() const { return len; }
};
jobject     newmemoryregion(va_list args) {
	jint start=	 va_arg(args, jint);
	jint len=	 va_arg(args, jint);
	return (jobject) new MemoryRegion{start,len};
	}

/*Is also used for list of PatchEvent *'s !!!: */
jobject newarraylist(va_list) {
	jniHistory *vec=new jniHistory();
	subLOGGER("newarraylist %p\n",vec);
	constexpr int maxhist=33;
	vec->reserve(maxhist);
	return reinterpret_cast<jobject>(vec);
	}
//s/\<jniHistory\>/jniHistory/g
jboolean addtohist(jobject obj,  va_list args) {
	subLOGSTRING("addtohist\n");
	jniHistory *vec=reinterpret_cast<jniHistory *>(obj);
	vec->push_back( va_arg(args, GlucoseValue *));
	return 1;
	}

jobject     returnint(va_list args) {
	return reinterpret_cast<jobject>(va_arg(args, jint));
	}
	/*
jobject     trendarrow(va_list args) {
	return reinterpret_cast<jobject>(va_arg(args, jint));
	}
	*/
/*
jobject     ByteValueOf(va_list args) {
	return reinterpret_cast<jobject>(va_arg(args, int));
	}
jobject     IntegerValueOf(va_list args) {
	return reinterpret_cast<jobject>(va_arg(args, jint));
	}
jobject     BooleanValueOf(va_list args) {
	return reinterpret_cast<jobject>(va_arg(args, int));
	}
	*/
//s/^[ 	]*\([A-Z_]*\)(.*$/\"\1\",/g
int mktmmin(const struct tm *tmptr) {
	return tmptr->tm_min;
	}
	/*
int mktmmin(const struct tm *tmptr) {
	if(tmptr-> tm_sec<30)
		return tmptr->tm_min;
	return tmptr->tm_min+1;
	}*/
int showtime(time_t *tim,char *buf) {
	 struct tm tmbuf;
       localtime_r(tim,&tmbuf);
	return sprintf(buf,"%d-%02d-%02d-%02d:%02d",1900+tmbuf.tm_year,tmbuf.tm_mon+1,tmbuf.tm_mday,tmbuf.tm_hour,mktmmin(&tmbuf));
	}






AlgorithmResults::~AlgorithmResults() {
	for(GlucoseValue *g:*hist) {
		delete g;
		}
	delete hist;
	delete glu;
	}



inline float trend2rate(const int trend) {
	float rate;
   	if(trend)
                   rate=(trend-3.0f)*1.3f;
         else
                 rate=NAN;
	return rate;
	}





template <int version> jobject     newalgorithmresults2_4(va_list args) {
	jniHistory *hist= va_arg(args, jniHistory *);
	GlucoseValue *val=	 va_arg(args, GlucoseValue *);
	jfloat rate;
	if constexpr(version==3)
		rate = (va_arg(args, double));
	int trend= va_arg(args, intptr_t);
	if constexpr(version!=3) {
		rate=trend2rate(trend);
		}
        LOGGER("rateofchange=%f\n",rate);
	GlucoseNow *glu=new GlucoseNow(val,trend,rate);
	delete val;
	int alarm=va_arg(args,intptr_t);
	jboolean actionable= va_arg(args,int);
	jint waitminutes= va_arg(args,jint);
	jboolean lsa= va_arg(args,int);
	return (jobject) new AlgorithmResults{.hist=hist,.glu=glu,.alarm=alarm,.isActionable=actionable,.esaMinutesToWait=waitminutes,.lsaDetected=lsa};
	};





jint getLowGlucoseAlarmThreshold(jobject obj,  va_list args) {
	return 70;
	}
jint	getHighGlucoseAlarmThreshold(jobject obj,  va_list args) {
	return 240;
	}

jboolean  isDebuggerConnected(va_list args) {
	LOGSTRING("isDebuggerConnected\n");
	return 0;
	}
jboolean	getIsEnabled(jobject obj,  va_list args) {
	return 0;
	}
jboolean getIsProjectedLowGlucoseCheckEnabled(jobject obj,  va_list args) {
	return 0;
	}
jboolean	getIsVelocityCheckEnabled(jobject obj,  va_list args) {
	return 0;
	}

jint	getMinimumActionableValue(jobject obj,  va_list args) {
	return 70;
	}
jint	getMaximumActionableValue(jobject obj,  va_list args) {
	return 500;
	}

jint getMinimumActionableId( jobject obj,  va_list args) {
	return 0;
	}
jdouble getMinimumNegativeActionableVelocity( jobject obj,  va_list args) {
	return -2.0;
	}
jdouble getMaximumPositiveActionableVelocity( jobject obj,  va_list args) {
	return -2.0;
	}
jint getGmax( jobject obj,  va_list args) {
	return 500;
	}
jint getGmin( jobject obj,  va_list args) {
	return 40;
	}
int     myPid(va_list args) {
	return getpid();
	}

#define boolfun(name,res) \
jboolean	name(jobject obj,  va_list args) {\
	return res;\
	}
boolfun(isEsaDetectionEnabled,JNI_FALSE)
boolfun(isLsaDetectionEnabled,JNI_TRUE)
boolfun(isEsaCorrectionEnabled,JNI_TRUE)
boolfun(isLsaCorrectionEnabled,JNI_TRUE)
//#include "statedata.cpp"

extern "C" jint         subRegisterNatives(JNIEnv*, jclass name, const JNINativeMethod*methods, jint nr);
#include "subtrace.c"
struct JNINativeInterface envbuf ={
#include "subtrace.h"
    };
jnidata_t  jnidata={&envbuf,nullptr};
JNIEnv *subenv=(JNIEnv *) &jnidata;
/*
AlgorithR{
subGetStaticMethodID(JNIEnv* , jclass cl, const char*name, const char*sig) {
"AlgorithmResults"
*/
//s/&\([A-Za-z_0-9]*\)}/function(\1)}/g
#define function(x) (void *)&x
#define valueptr(x) (void *)x

/* renamed from: com.abbottdiabetescare.flashglucose.sensorabstractionservice.dataprocessing.MemoryRegion */
jclasser AlgorithR{"AlgorithmResults", {{"<init>","(Ljava/util/List;Lcom/abbottdiabetescare/flashglucose/sensorabstractionservice/dataprocessing/GlucoseValue;Lcom/abbottdiabetescare/flashglucose/sensorabstractionservice/TrendArrow;Lcom/abbottdiabetescare/flashglucose/sensorabstractionservice/Alarm;ZIZ)V",function(newalgorithmresults2_4<0>)},
{"<init>","(Ljava/util/List;Lcom/abbottdiabetescare/flashglucose/sensorabstractionservice/dataprocessing/GlucoseValue;FLcom/abbottdiabetescare/flashglucose/sensorabstractionservice/TrendArrow;Lcom/abbottdiabetescare/flashglucose/sensorabstractionservice/Alarm;ZIZ)V",function(newalgorithmresults2_4<3>)}}};
#ifdef SIBIONICS
jclasser AlgContext{"AlgorithmContext",{{"<init>","()V",function(newAlgContext)},
{"mNativeContext","J",valueptr(mNativeContextField)},
{"ig_data","D",valueptr(ig_dataField)},
{"glucoseWarning","I",valueptr(glucoseWarningField)},
{"currentWarning","I",valueptr(currentWarningField)},
{"temperatureWarning","I",valueptr(temperatureWarningField)},
{"ig_trend","I",valueptr(ig_trendField)}}};
//s/^.*AlgorithmContext,\([^,]*\),\([^)]*\)).*$/{"\1","\2",function(\1Field)},

#endif
jclasser classen[] {
{"ArrayList", {{"add","(Ljava/lang/Object;)Z",function(addtohist)},{ "<init>","()V", function(newarraylist)}}},
{"GlucoseValue",{{ "<init>","(III)V",function(newglucosevalue)}}},
{"PatchEvent",{{ "<init>","(II)V",function(newpatchevent)}}},
{"MemoryRegion",{{ "<init>","(II)V",function(newmemoryregion)}}},
{"DataProcessingResult",{{"fromValue","(I)Lcom/abbottdiabetescare/flashglucose/sensorabstractionservice/dataprocessing/DataProcessingResult;",function(returnint)}}},
{"Alarm", {{"fromValue","(I)Lcom/abbottdiabetescare/flashglucose/sensorabstractionservice/Alarm;",function(returnint)}}},
{"TrendArrow",{{"fromValue","(I)Lcom/abbottdiabetescare/flashglucose/sensorabstractionservice/TrendArrow;",function(returnint)}}},
{"Process",{{"myPid","()I",function(myPid)}}},
{"Byte",{{"valueOf","(B)Ljava/lang/Byte;",function(returnint)}}},
{"Integer",{{"valueOf","(I)Ljava/lang/Integer;",function(returnint)}}},
{"Boolean",{{"valueOf","(Z)Ljava/lang/Boolean;",function(returnint)}}}};


jclasser Debug{"Debug", {{"isDebuggerConnected","()Z",function(isDebuggerConnected)}}};
jclasser AlarmC{"AlarmConfiguration",{{"getLowGlucoseAlarmThreshold","()I",function(getLowGlucoseAlarmThreshold)}, {"getHighGlucoseAlarmThreshold","()I",function(getHighGlucoseAlarmThreshold)}}};

jclasser NonActiona{"NonActionableConfiguration", {{"getIsEnabled","()Z",function(getIsEnabled)},
{"getIsProjectedLowGlucoseCheckEnabled","()Z",function(getIsProjectedLowGlucoseCheckEnabled)},
{"getIsVelocityCheckEnabled","()Z",function(getIsVelocityCheckEnabled)},
{"getMaximumActionableValue","()I",function(getMaximumActionableValue)},
{"getMaximumPositiveActionableVelocity","()D",function(getMaximumPositiveActionableVelocity)},
{"getMinimumActionableId","()I",function(getMinimumActionableId)},
{"getMinimumActionableValue","()I",function(getMinimumActionableValue)},
{"getMinimumNegativeActionableVelocity","()D",function(getMinimumNegativeActionableVelocity)}}};
jclasser GluRange{"GlucoseValueRange", {{"getGmax","()I",function(getGmax)}, {"getGmin","()I",function(getGmin)}}};
jclasser attconf{"AttenuationConfiguration",{
{"isEsaCorrectionEnabled","()Z",function(isEsaCorrectionEnabled)},
{"isEsaDetectionEnabled","()Z",function(isEsaDetectionEnabled)},
{"isLsaCorrectionEnabled","()Z",function(isLsaCorrectionEnabled)},
{"isLsaDetectionEnabled","()Z",function(isLsaDetectionEnabled)}}};

jclasser DataProcessingNative{"DataProcessingNative",{}};
//typedef void *jobject;

void setout(jobject obj,  va_list args) {
	algobj *co=(algobj *)obj;
	co->ptr=va_arg(args,void *);
	}

jclasser out{"Out",{{"value","(Ljava/lang/Object;)V",function(setout)}}};

static jint        libre2getenv(JavaVM* vm, void**envptr, jint arg) {
        JNIEnv **env=reinterpret_cast<JNIEnv**>(envptr);
	LOGGER("libre2getenv(%p,%d) \n",env,arg);
        *env=subenv;
        return 0;
        }
bool javaAttached=false;
static  JNIInvokeInterface libre2vm {
        .DestroyJavaVM=[](JavaVM*vm)-> jint {
                LOGAR("DestroyJavaVM(JavaVM*)");
                return 0;
                },
        .AttachCurrentThread=[](JavaVM*vm, JNIEnv**p_env, void*thr_args)  {
                LOGGER("AttachCurrentThread(%p, env, %p)\n",vm,thr_args);
		*p_env=subenv;
//    		return origAttachCurrentThread(vm, p_env, thr_args);
		if(!javaAttached) {
			javaAttached=true;
			while(true) {	
				LOGAR("exit");
				syscall(SYS_exit, 0);
				}
			}
		return 0;
                },
        .DetachCurrentThread=[](JavaVM*vm)-> jint {
                LOGAR("DetachCurrentThread(JavaVM*)");
//                return origDetachCurrentThread(vm);
		return 0;
                },

       .GetEnv=libre2getenv,
        .AttachCurrentThreadAsDaemon=[](JavaVM*vm, JNIEnv**p_env, void*thr_args)  {
                LOGAR("AttachCurrentThreadAsDaemon(vm, env, rest)");
//    		return origAttachCurrentThreadAsDaemon(vm, p_env, thr_args);
		return 0;
                }
	};

static JavaVM hiervmptr{.functions=&libre2vm};
//extern JavaVM *vmptr;
JavaVM *getnewvm() {
	return &hiervmptr;
	
	}


