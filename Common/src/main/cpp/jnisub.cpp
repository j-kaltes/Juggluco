
#define setthreadname(buf) prctl(PR_SET_NAME, buf, 0, 0, 0)
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
#include <string_view>
#include <algorithm>
#include <unordered_map>
#undef _GNU_SOURCE 
#define _GNU_SOURCE  
#include <dlfcn.h>
#include <link.h>
#include <assert.h>
#include <filesystem>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "config.h"
#include "inout.h"
#include "jnisub.h"
#include "logs.h"
#include "SensorGlucoseData.h"
#include "debugclone.h"
#include "settings/settings.h"
#include "hexstr.h"
#define VISIBLE __attribute__((__visibility__("default")))
using namespace std;
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

//#define fromjava(x) Java_tk_glucodata_Natives_ ##x
//FILE *errorlogfile=stderr;
//#define LOGGER(...) {fprintf(stderr,__VA_ARGS__) ;fflush(stderr);}
//#define LOGGER(...) 
/*
extern "C" int execv(const char *pathname, char *const argv[]) {

	LOGGER("no execv(%s, {",pathname);
	for(char *const *ptr=argv;*ptr;ptr++ ) {
		LOGGER("%s,",*ptr);
		}
	LOGSTRING("})\n");
	puts("package:/o/home/jka/src/android/Librefree/app/src/main/cpp/base.apk");
	fflush(stdout);
	exit(0);
//        return execl(pathname, argv[0],argv[1],argv[2],NULL); //werkt
//	char prog[]="/system/bin/echo";
//        return execl(prog, prog, "package:/data/app/com.freestylelibre.app.nl-728uRZlgjtOPbkd003cvEw==/base.apk",NULL); //werkt
	}
	*/
//extern "C" int __android_log_print(int prio, const char* tag, const char* fmt, ...) __attribute__((__format__(printf, 3, 4)));


//extern "C" int __android_log_vprint(int prio, const char* tag, const char* fmt, va_list ap) __attribute__((__format__(printf, 3, 0)));

/*extern "C" int __android_log_print(int prio, const char* tag, const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
	LOGGER("#%s: ",tag);
int res=	vfprintf(stderr, fmt, args);
	fputc('\n',stderr);
        va_end(args);
	return res;
	}
	*/
/*
typedef const struct JNINativeInterface * JNIEnvC;

ByteArray toByteArray(JNIEnvC *env,const jbyteArray jar) {
//JNIEnvC *env= (JNIEnvC *)envin;
if(! (*env)->IsSameObject(env, jar, NULL)) {
	jsize len=       (*env)->GetArrayLength(env, jar);
	ByteArray byt(len);	
   	(*env)->GetByteArrayRegion(env, jar, 0, len, byt);
	return byt;

	    }

    return ByteArray(0);
    }
*/

struct Method {
string name;
string pat;
void *func;
};


struct jclasser
{
std::vector<Method> funcs;

static std::unordered_map<string,jclasser*> classid; 
jclasser(const char name[], std::vector<Method> &&funcs): funcs(std::move(funcs)) {
	classid.emplace( std::make_pair(name, this));
	}
};
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

struct jnidata_t {
	const struct JNINativeInterface* functions;
	scanstate *map;
	};
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
/*
jboolean    subCallBooleanMethod(JNIEnv* env, jobject obj, jmethodID meth, ...) {
subLOGSTRING("subCallBooleanMethod\n");
	va_list args;
	va_start(args, meth);
	jboolean out= (( jboolean (*)(jobject,  va_list) )meth)(obj,args);
	va_end(args);
	return out;
	}
*/
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
   	subLOGGER("subExceptionOccurred %d\n", syscall(SYS_gettid));


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
struct algobj {
public:
 jclasser *cl;
void *ptr=nullptr;
algobj(jclasser *cl):cl(cl) {}
operator jobject() {
        return (jobject)this;
        }
	/*
operator intptr_t() {
	return reinterpret_cast<intptr_t>(ptr);
	}
	*/
intptr_t toint() {
	return reinterpret_cast<intptr_t>(ptr);
	}
};
extern "C" jclass      subGetObjectClass(JNIEnv*, jobject ob) {
	subLOGSTRING("subGetObjectClass\n");
	algobj *co=(algobj *)ob;
	return  (jclass)co->cl;
	}
/*
class MemoryRegion {
    const int startAddress;
    const int numberOfBytes;
public:
    MemoryRegion(int start, int len): startAddress(start),numberOfBytes(len) { }

int getStartAddress() const {
        return startAddress;
    }

int getNumberOfBytes() const {
        return numberOfBytes;
    }
};
 */


class PatchEvent {
private:
    const int id;
    const int errorCode;
public:
    PatchEvent(int id, int code) :id(id),errorCode(code) { }

   int getId()const { return id; }

   int getErrorCode() const { return errorCode; }
};
          
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

void AlgorithmResults::showresults(FILE *stream,scandata *dat) const {
    time_t nutime=dat->gettime();
    int nuid=glu->id;
    fprintf(stream,"History:\n");
	constexpr const int maxtim=17;
	char buf[maxtim];
 jint len=hist->size();
int uselen=std::min(history::num,len);
for(int i=0;i<uselen;i++) {
        const GlucoseValue *g=(*hist)[i];
	int gv=g->value;
	int id=g->id;
	time_t was=nutime-(nuid-id)*60;
	uint16_t raw=dat->gethistoryglucose(i);

	showtime(&was,buf);
	fprintf(stream,"Glucose %d\t%.1f\t(%d\t%.1f)\t%d\t%d\t%s\n",gv,gv/18.0, (int)roundf(raw/10.0),raw/180.0, id, g->dataQuality,buf);
	}
	showtime(&nutime,buf);
	fputc('\n',stream);
	for(int i=0;i<trend::num;i++) {
		uint16_t raw=dat->gettrendglucose(i);
		fprintf(stream, "%.1f ",raw/180.0);
		}
    fprintf(stream,"\n%s Nu Glucose %.1f %s %f %d %d\n",buf,(float)glu->value/18.0,glu->trendstr(),glu->rate(),nuid,glu->dataQuality);
    fflush(stream);
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

/* renamed from: com.abbottdiabetescare.flashglucose.sensorabstractionservice.dataprocessing.MemoryRegion */
jclasser AlgorithR{"AlgorithmResults", {{"<init>","(Ljava/util/List;Lcom/abbottdiabetescare/flashglucose/sensorabstractionservice/dataprocessing/GlucoseValue;Lcom/abbottdiabetescare/flashglucose/sensorabstractionservice/TrendArrow;Lcom/abbottdiabetescare/flashglucose/sensorabstractionservice/Alarm;ZIZ)V",function(newalgorithmresults2_4<0>)},
{"<init>","(Ljava/util/List;Lcom/abbottdiabetescare/flashglucose/sensorabstractionservice/dataprocessing/GlucoseValue;FLcom/abbottdiabetescare/flashglucose/sensorabstractionservice/TrendArrow;Lcom/abbottdiabetescare/flashglucose/sensorabstractionservice/Alarm;ZIZ)V",function(newalgorithmresults2_4<3>)}}};
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

struct outobj :public algobj{
outobj(): algobj(&out) {}
// jclasser *algobj::cl=&out;
};
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

//WAS_JNIEXPORT jboolean JNICALL   abbottdec(getNeedsReaderInfoForActivation)(JNIEnv *env, jobject obj, jint parsertype) ;

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
/*
typedef WAS_JNIEXPORT jobject JNICALL   (*processStream230_t)(JNIEnv *envin, jobject obj, 
jint parsertype, jobject alarmconf, jobject nonaction, jobject attenuatinconfig, jbyteArray sensorident, 
jbyte person, jbyteArray bluetoothdata, 
jint startsincebase,jint nusincebase,jint warmup,jint wear,
jbyteArray compostate, jbyteArray attenustate, jbyteArray measurestate,
jobject outstarttime, jobject endtime, jobject confinsert, jobject removed, 
jobject compo, jobject attenu, jobject messstate, 
 jobject algorithresults) ;
processStream230_t  processStream230=nullptr;
 
JNIEXPORT jobject JNICALL Java_com_abbottdiabetescare_flashglucose_sensorabstractionservice_dataprocessing_DataProcessingNative_processStream (JNIEnv *, jobject, 
jint,           jobject,           jobject,            jobject,          jobject,                  jbyteArray, jbyteArray, 
jbyte,        jbyteArray, 
jint,               jint,            jint,        jint, 
jbyteArray,           jbyteArray,               jbyteArray, 
jobject,               jobject,        jobject,             jobject, o
jobject,       jobject,           jobject, 
jobject);
*/
//WAS_JNIEXPORT jobject JNICALL abbottdec(getNextRegionToRead)(JNIEnv *, jobject, jint, jbyteArray, jbyteArray, jbyteArray, jint);

//return encodeStatusCode(this.dataProcessing.getStatusCode(serialNumber, elapsedSeconds, elapsedSeconds2, timeOfDayAdapter2.toElapsedSeconds(timeOfDayAdapter2.fromDate(this.timeOsFunctions.getCurrentTime(), timeZoneUTC), DataProcessing.BASE_YEAR), sensor.getEndedEarly()));

//dpSetReaderCodeSimMode
}


/*
extern "C" {
JNIEXPORT jint JNICALL Java_com_abbottdiabetescare_flashglucose_sensorabstractionservice_dataprocessing_DataProcessingNative_getTotalMemorySize(JNIEnv *env, jobject thiz, jint get) ;

JNIEXPORT void JNICALL Java_com_abbottdiabetescare_flashglucose_sensorabstractionservice_dataprocessing_DataProcessingNative_initialize(JNIEnv *, jobject, jobject);
JNIEXPORT void JNICALL Java_com_abbottdiabetescare_flashglucose_sensorabstractionservice_dataprocessing_DataProcessingNative_dpSetMaxAlgorithmVersion(JNIEnv *, jobject, jint);



JNIEXPORT jobject JNICALL Java_com_abbottdiabetescare_flashglucose_sensorabstractionservice_dataprocessing_DataProcessingNative_processScan
  (JNIEnv *, jobject, jint, jobject, jobject, jobject, jobject, jbyteArray, jbyteArray, jbyteArray, jint, jint, jint, jint, jint, jbyteArray, jbyteArray, jbyteArray, jobject, jobject, jobject, jobject, jobject, jobject, jobject, jobject, jobject, jobject);
}
*/
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
/*
    jint        (*origDestroyJavaVM)(JavaVM*);
    jint        (*origAttachCurrentThread)(JavaVM*, JNIEnv**, void*);
    jint        (*origDetachCurrentThread)(JavaVM*);
    jint        (*origGetEnv)(JavaVM*, void**, jint)=nullptr;
    jint        (*origAttachCurrentThreadAsDaemon)(JavaVM*, JNIEnv**, void*);
    jclass      (*origFindClass)(JNIEnv*, const char*);
jint        (*origRegisterNatives)(JNIEnv*, jclass, const JNINativeMethod*, jint);
JNIEnv *origenv;
*/


/*
#include "libre3/unprotect.h"
static jint        libre2getenv(JavaVM* vm, void**envptr, jint arg) {
        JNIEnv **env=reinterpret_cast<JNIEnv**>(envptr);
	LOGGER("libre2getenv(%p,%d) origGetEnv=%p\n",env,arg,origGetEnv);
    	auto res=origGetEnv(vm, envptr, arg);
	LOGAR("libre2getenv afgter origGetEnv");
	origenv=*env;
	origRegisterNatives=origenv->functions->RegisterNatives;
	origFindClass=origenv->functions->FindClass;
	 Unprotect _prot((struct JNINativeInterface*)origenv->functions,sizeof(*origenv->functions));
	((struct JNINativeInterface*)(origenv->functions))->RegisterNatives= subRegisterNatives;
	((struct JNINativeInterface*)(origenv->functions))->FindClass= subFindClass;
	LOGAR("end libre2getenv");
//        *env=subenv;
        return res;
        }*/
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
/*
    origDestroyJavaVM=vmptr->functions->DestroyJavaVM;
    origAttachCurrentThread=vmptr->functions->AttachCurrentThread;
    origDetachCurrentThread=vmptr->functions->DetachCurrentThread;
    origGetEnv=vmptr->functions->GetEnv;
    origAttachCurrentThreadAsDaemon=vmptr->functions->AttachCurrentThreadAsDaemon;
   *(( (void **)(&hiervmptr.functions->reserved0)))=vmptr->functions->reserved0;
	*(( (void **)(&hiervmptr.functions->reserved1)))=vmptr->functions->reserved1;
	*(( (void **)(&hiervmptr.functions->reserved2)))=vmptr->functions->reserved2; */
	return &hiervmptr;
	
	}

typedef struct JNINativeInterface * JNIEnvC;
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
	//		 ds(getNeedsReaderInfoForActivation)
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
//Abbott(string_view basedir,data_t *uidin,int fam=0): sensordir(basedir,getserial(fam,reinterpret_cast<unsigned char *>(uidin->data()))), uid(uidin) {}
static jint getFamily(JNIEnv *env,jbyteArray info) {
	jint fam=abbottcall(getProductFamily)(env,nullptr, parsertype, info,getjtoken(env));
	LOGGER("getProductFamily()=%d\n", fam);
	if(!fam) {
		fam=abbottcall(getProductFamily)(env,nullptr, parsertype, info,getjtoken(env));
		LOGGER("getProductFamily()=%d\n", fam);
		}
	return fam;
	}

Abbott::Abbott(JNIEnv *env,string_view basedir,jbyteArray juid, jbyteArray info): Abbott(basedir,fromjbyteArray(env,juid),getFamily(env,info)) {
	}
AlgorithmResults *callAbbottAlg(data_t *uid,int startsincebase,scanstate *oldstate,scandata *data,scanstate *newstate,outobj *starttime,outobj *endtime,jobject person) {
	LOGSTRING("start callAbbottAlg/3\n");
	const time_t nutime=data->gettime();
	const int lastsincebase=nutime-basesecs;
	const int timeoffset=1000*gettimezoneoffset(&nutime);
	algobj alarm{&AlarmC},nonAction{&NonActiona};
	algobj range{&GluRange}, attenconf{&attconf};
	int warmup=60, wear=14*24*60;
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
	return ::callAbbottAlg(uid,startsincebase,oldstate,data,newstate,starttime,endtime,person) ;
	}
//extern bool savehistory(AlgorithmResults *res,time_t nutime,nfcdata &nfc,SensorGlucoseData &save) ;
//nfcdata  *lastnfcdata=nullptr;
//extern bool savehistory(const AlgorithmResults *res,time_t nutime,const nfcdata *nfcptr,SensorGlucoseData &save) ;

std::vector<int> usedsensors;
void setusedsensors(uint32_t nu) {
	uint32_t recent=nu-daysecs;	
	usedsensors=sensors->bluetoothactive(recent,nu);
	}

void setusedsensors() {
	uint32_t nu=time(nullptr);	
	setusedsensors(nu);
	}



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
	const int timelast=nutime-startDate;
	//constexpr const int endsensorsecs=  1195800;
	LOGGER("timelast=%d endsensorsecs=%d\n",timelast,endsensorsecs);
	if(timelast>=endsensorsecs&&timelast<days15) {
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
	LOGSTRING("initnewsensor ");
	jbyteArray juid =(jbyteArray) uid; 
	scanstate inp,newstate(4);
	outobj person,endtime,starttime;
	AlgorithmResults * alg=callAbbottAlg(0,&inp,data,&newstate,&starttime,&endtime,person);
	if(!alg) {
		return nullptr;
		}
	jint startminbase=starttime.toint();
	time_t start=basesecs+startminbase;
#ifndef NOLOG
	{
	char buf[50];
	LOGGER("init starttime %s",ctime_r(&start,buf));

	}
#endif
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

		SensorGlucoseData::mkdatabase(sensordir,start,(const char *)uid->data(),infodata,15,3,bluestart,nullptr) ;
		int ind=sensors->addsensor(serial.data());
		hist=sensors->getSensorData(ind);
		sensorindex=ind;
extern bool streamHistory() ;

		void setstartedwithStreamhistory();
		if(streamHistory()) 
			setstartedwithStreamhistory();
		}
	constexpr const int	threeyear=94694400 ;//2013
	constexpr const int sec_per_day=60*60*24;
scanstate *newstateptr=new scanstate(sensordir,data->gettime());
jnidata_t  hierjnidata={&envbuf,newstateptr};
JNIEnv *hiersubenv=(JNIEnv *) &hierjnidata;

	data_t *messuit;

const int timelast=data->gettime()-start;
//constexpr const int endsensorsecs= 14*24*60*60;
//constexpr const int endsensorsecs=  1195800;
if(timelast>=endsensorsecs&&timelast<days15) {
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
	if(timelast>=endsensorsecs)
	    newstateptr->datpos(FAKE)=1;
		
	delete alg;
	newstateptr->makelink();
	newstateptr->setpos(MESS,messuit);
	return newstateptr;
	}

#include "environ.h"
 char *inenv(char **env,string_view str)  {
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
	bool gen2= settings->data()->streamHistory||userver2();

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
//				packager.addend();
			/*	abbottcall(initialize)(env,thiz, thiz);
				abbottcall(dpSetMaxAlgorithmVersion)(env,thiz, 3,casttoken);  */
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
		settings->setnodebug(false);
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
	#endif
		return 0;
}

static AlgorithmResults * processBlue(data_t * bluedata, jbyte person,

const data_t *ident,
#ifdef HASP1
const data_t *info,
#endif
scanstate *state,
scanstate *newstate,uint32_t startsincebase,uint32_t nutime) ;

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
	if(warm==-1) {
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
&newstate,startsincebase,bluetime) ;
		if(!res) {
			LOGSTRING("processStream failed\n");
			return 12;
			}
		else {
			const GlucoseNow & gl=res->currentglucose();
			const int gluc=gl.getValue();
			LOGGER("processStream %d %.1f (%d) rate=%.1f",gl.getQuality(),gluc/18.0,gluc,gl.rate());
			if(!nearbymgdl(gluc,96))
				return 12;;
			scanstate statescan(4);
			outobj person,endtime,starttime;
			if(AlgorithmResults *alg2=callAbbottAlg(uidptr,startsincebase,&newstate,&data2,&statescan,&starttime,&endtime,person)) {
				if(!nodebug) {
					__attribute__((used)) static	pid_t stop=getsid(debugpid);
					}
				const GlucoseNow & gl=alg2->currentglucose();
				const int gluc=gl.getValue();
				if(memcmp(data2.data->data(),decr,data2.data->size())) return 12;
			        LOGGER("processScan %d %.1f (%d) rate=%.1f",gl.getQuality(),gluc/18.0,gluc,gl.rate());
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
        if (abbottcall(getPatchTimeValues)(subenv,nullptr,parsertype, (jbyteArray) info, warmup, wear,casttoken)) {
		return  {(int)warmup.toint(), (int)wear.toint()};
        	}
        return {-1,-1}; 
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
	LOGSTRING("start getNeedsReaderInfoForActivation ");
	bool res=   abbottcall(getNeedsReaderInfoForActivation)(subenv, nullptr, parsertype) ;
	LOGSTRING(" end getNeedsReaderInfoForActivation\n");
	return res;
	}
	*/
extern "C" JNIEXPORT jboolean JNICALL fromjava(hasRootcheck)(JNIEnv* env, jclass obj) {
	return rootcheck;
	}
extern "C" JNIEXPORT jbyte JNICALL fromjava(activationcommand)(JNIEnv* env, jclass obj,jbyteArray info) {
	abbottinit();
	debugclone(); //Nodig?
	LOGSTRING("start getActivationCommand ");
       	jbyte res=  abbottcall(getActivationCommand)(env,obj, parsertype, info,getjtoken(env));
	LOGSTRING(" end getActivationCommand\n");
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
scanstate *newstate,uint32_t startsincebase,uint32_t nutime) {
	jint nusincebase=nutime-basesecs;
	algobj alarm{&AlarmC},nonAction{&NonActiona};
	algobj range{&GluRange}, attenconf{&attconf};
	int warmup=60, wear=14*24*60;//TODO set with function
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
const AlgorithmResults * alg= processBlue(bluedata, person,

ident,
#ifdef HASP1
info,
#endif
state,
newstate, startsincebase,nutime);
if(alg) {
		/*int insert=reinterpret_cast<intptr_t>(confinsert.ptr);
		int isremoved=reinterpret_cast<intptr_t>(removed.ptr);
		LOGGER("Is %sinserted and %sremoved\n",insert?"":"not ",isremoved?"":"not ");*/

//		AlgorithmResults *alg=(AlgorithmResults *)outalgres.ptr;
		const GlucoseNow & gl=alg->currentglucose();
		const int gluc=gl.getValue();
		const int qual=gl.getQuality();
		int gid=gl.getId();
		LOGGER("processStream %d %d %.1f\n",gid,qual,gluc/18.0);
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
		SensorGlucoseData::mkdatabase(pathconcat(sensorbasedir,serial),starttime,(const char *)uid->data(),(const char *)info->data(),15,3,starttime,(const char *)info->data());
		sensors->addsensor(serial.data());
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
	AlgorithmResults * alg=callAbbottAlg(uid,0,&inp,data,&newstate,&starttime,&endtime,person);
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


