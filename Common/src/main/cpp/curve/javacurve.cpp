#include <sys/prctl.h>
#include <jni.h>
#include <string_view>
#include <string>
#include "share/logs.h"
#include "fromjava.h"
//#include "curve.h"
//#include "nanovg_gl.h"
//#include "nanovg_gl_utils.h"
#define NANOVG_GLES2_IMPLEMENTATION

extern "C" JNIEXPORT jint JNICALL fromjava(openglversion)(JNIEnv* env, jclass obj) {
#ifdef NANOVG_GLES2_IMPLEMENTATION
	return 2;
#else
	return 3;
#endif
}

void	initopengl(int started);
extern "C" JNIEXPORT void JNICALL fromjava(initopengl)(JNIEnv* env, jclass obj,jboolean started) {
	initopengl(started);
    }


extern bool alarmongoing;
extern "C" JNIEXPORT jboolean JNICALL fromjava(getisalarm)(JNIEnv* env, jclass obj) {
	return alarmongoing;
	}
extern "C" JNIEXPORT void JNICALL fromjava(setisalarm)(JNIEnv* env, jclass obj,jboolean val) {
	alarmongoing=val;
	}
extern "C" JNIEXPORT jboolean JNICALL fromjava(turnoffalarm)(JNIEnv* env, jclass obj) {
	return alarmongoing;
	}

extern void resizescreen(int widthin, int heightin,int initscreenwidth);
extern "C" JNIEXPORT void JNICALL fromjava(resize)(JNIEnv* env, jclass obj, jint widthin, jint heightin,jint initscreenwidth) {
	resizescreen(widthin,heightin,initscreenwidth);
	}


extern void setfontsize(float small,float menu,float density,float headin) ;
extern "C" JNIEXPORT void JNICALL fromjava(setfontsize)(JNIEnv* env, jclass obj, jfloat small,jfloat menu,jfloat density,jfloat headin) {
	setfontsize(small,menu,density,headin);
	}

extern float getfreey();
extern "C" JNIEXPORT jfloat JNICALL fromjava(freey) (JNIEnv *env, jclass clazz) {
	return getfreey();
	}

extern void setusedsensors() ;
extern "C" JNIEXPORT void JNICALL fromjava(setmaxsensors) (JNIEnv *env, jclass clazz,jint nr) {
	setusedsensors();
	}

extern uint32_t lastsensorends();
extern "C" JNIEXPORT jlong JNICALL fromjava(sensorends)(JNIEnv* env, jclass obj) {
	return  lastsensorends() ;
	}

extern int badscanMessage(int kind);
extern "C" JNIEXPORT jint JNICALL fromjava(badscan)(JNIEnv* env, jclass obj,jint kind) {
	return badscanMessage( kind) ;
	}

jobject glucosecurve=0;

extern "C" JNIEXPORT void  JNICALL   fromjava(setpaused)(JNIEnv *env, jclass cl,jobject val) {
	if(glucosecurve)
		env->DeleteGlobalRef(glucosecurve);
	if(val)
		glucosecurve=env->NewGlobalRef(val);
	else
		glucosecurve=nullptr;
	}
#ifndef NOJAVA
JavaVM *vmptr;
static jmethodID summaryready=nullptr;

	#ifdef  WEAROS
static jmethodID showsensorinfo=nullptr;
#endif
jmethodID  jdoglucose=nullptr, jupdateDevices=nullptr, jbluetoothEnabled=nullptr,jspeak=nullptr;
jclass JNIApplic;
#ifdef OLDXDRIP
#ifndef  WEAROS
jclass XInfuus;
jmethodID  sendGlucoseBroadcast=nullptr;
#endif
#endif

extern void initlibreviewjni(JNIEnv *env);

extern bool jinitmessages(JNIEnv* env);

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
	vmptr=vm;
	   JNIEnv* env=nullptr;
	LOGAR("JNI_OnLoad");
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
      }

{
const jclass cl=env->FindClass("tk/glucodata/GlucoseCurve");
if(!cl) {
	summaryready=nullptr;
	LOGSTRING("Can't find GlucoseCurve\n");
	}
else {
	LOGSTRING("found GlucoseCurve\n");
	summaryready=env->GetMethodID(cl,"summaryready","()V");
	#ifdef  WEAROS
	showsensorinfo=env->GetMethodID(cl,"showsensorinfo","(Ljava/lang/String;)V");
	#endif
	env->DeleteLocalRef(cl);
	}

}

{
const static jclass cl=env->FindClass("tk/glucodata/Applic");
if(cl) {
	JNIApplic = (jclass)env->NewGlobalRef(cl);
	env->DeleteLocalRef(cl);
	if(!(jdoglucose=env->GetStaticMethodID(JNIApplic,"doglucose","(Ljava/lang/String;IFFIJZ)V"))) {
		LOGSTRING(R"(GetStaticMethodID(JNIApplic,"doglucose","(Ljava/lang/String;IFFIJZ)V"))) failed)" "\n");
		}
	if(!(jupdateDevices=env->GetStaticMethodID(JNIApplic,"updateDevices","()Z"))) {
		LOGSTRING(R"(jupdateDevices=env->GetStaticMethodID(JNIApplic,"updateDevices","()Z") failed)" "\n");
		}
	if(!(jbluetoothEnabled=env->GetStaticMethodID(JNIApplic,"bluetoothEnabled","()Z"))) {
		LOGSTRING(R"(jbluetoothEnabled=env->GetStaticMethodID(JNIApplic,"bluetoothEnabled","()Z") failed)" "\n");
		}
	if(!(jspeak=env->GetStaticMethodID(JNIApplic,"speak","(Ljava/lang/String;)V"))) {
		LOGSTRING(R"(jspeak=env->GetStaticMethodID(JNIApplic,"speak","(Ljava/lang/String;)V") failed)" "\n");
		}
	}
else {
	LOGSTRING(R"(FindClass("tk/glucodata/Applic") failed)" "\n");
	}
}


#ifdef OLDXDRIP
#ifndef  WEAROS
{
const static jclass cl=env->FindClass("tk/glucodata/XInfuus");
if(cl) {

	XInfuus = (jclass)env->NewGlobalRef(cl);
	env->DeleteLocalRef(cl);
	if(!(sendGlucoseBroadcast=env->GetStaticMethodID(XInfuus,"sendGlucoseBroadcast","(Ljava/lang/String;DFJ)V"))) {
		LOGSTRING(R"(GetStaticMethodID(XInfuus,"sendGlucoseBroadcast","(Ljava/lang/String;DFJ)V()) failed)" "\n");
		}
	}
else {
	LOGSTRING(R"(FindClass("tk/glucodata/XInfuus") failed)" "\n");
	}
	}
#endif
#endif
	/*
const static jclass clappl=env->FindClass("tk/glucodata/Applic");
jclass Applic=nullptr;
jmethodID jupdatescreen=nullptr;
if(clappl) {
	Applic= (jclass)env->NewGlobalRef(clappl);
	env->DeleteLocalRef(clappl);
	jupdatescreen=env->GetStaticMethodID(Applic,"updatescreen","()V");
	}
bool loadglucoseclass(JNIEnv *env);
if(loadglucoseclass(env)) {
	LOGSTRING("end JNI_OnLoad\n");
	 return JNI_VERSION_1_6;
	 }
return JNI_ERR;
*/
#ifndef WEAROS
initlibreviewjni(env);
#endif


#ifdef WEAROS_MESSAGES
jinitmessages(env) ;
#endif

	LOGSTRING("end JNI_OnLoad\n");
	 return JNI_VERSION_1_6;
}
class attach {
JNIEnv *env;
public:
attach() {
      char buf[17];
      prctl(PR_GET_NAME, buf, 0, 0, 0);
	vmptr->AttachCurrentThreadAsDaemon(&env, nullptr);
        prctl(PR_SET_NAME, buf, 0, 0, 0);
	}
~attach() {
	vmptr->DetachCurrentThread();
	}
[[nodiscard]]   JNIEnv *get()  const {
	return env;
	}
};


JNIEnv *getenv() {
	const thread_local static attach  env;
	return env.get();
	}
bool bluetoothEnabled() {
    return   getenv()->CallStaticBooleanMethod(JNIApplic,jbluetoothEnabled);
    }
void telldoglucose(const char *name,int32_t mgdl,float glu,float rate,int alarm,int64_t mmsec,bool wasnoblue) {
	jstring sname= getenv()->NewStringUTF(name);
	getenv()->CallStaticVoidMethod(JNIApplic,jdoglucose,sname,mgdl,glu,rate,alarm,mmsec,wasnoblue);
	getenv()->DeleteLocalRef(sname);
	}

bool updateDevices() {
    if(!jupdateDevices)  {
    	LOGSTRING("jupdateDevices==null\n");

    		return false;
		}
    return   getenv()->CallStaticBooleanMethod(JNIApplic,jupdateDevices);
    }
void visiblebutton() {
	if(glucosecurve) {
		if(summaryready)  {
			JNIEnv *env =getenv(); 
			LOGSTRING("call summaryready\n");
			env->CallVoidMethod(glucosecurve,summaryready);
			}
		else
			LOGSTRING("didn't find GlucoseCurve\n");
		}
	}

	#ifdef  WEAROS
void callshowsensorinfo(const char *text) {
	if(glucosecurve) {
		if(showsensorinfo)  {
			JNIEnv *env =getenv(); 
			LOGSTRING("call showsensorinfo\n");
			env->CallVoidMethod(glucosecurve,showsensorinfo,env->NewStringUTF(text));
			}
		else
			LOGSTRING("didn't find GlucoseCurve\n");
		}
	}
#endif

void render() {
	LOGSTRING("Render\n");
	if(glucosecurve) {
		struct method {
		   jmethodID requestRendermeth;
		   method(JNIEnv *env) {	
		        jclass	cl=env->FindClass("android/opengl/GLSurfaceView");
		        requestRendermeth=env->GetMethodID(cl,"requestRender","()V");
			env->DeleteLocalRef(cl);
		   	};
		};
		static method meth(getenv());
		getenv()->CallVoidMethod(glucosecurve,meth.requestRendermeth);
		}
//	onestep();
	}
#endif
jint onestep();
extern "C" JNIEXPORT jint JNICALL fromjava(step)(JNIEnv* env, jclass obj) {
	return onestep();
	}

extern bool hour24clock;
extern char localestrbuf[15];



void  setlocale(const char *localestrbuf,const size_t len) ;
extern "C" JNIEXPORT void JNICALL fromjava(setlocale)(JNIEnv *env, jclass clazz,jstring jlocalestr,jboolean hour24) {
	hour24clock=hour24;
	if(jlocalestr) {
		size_t len=env->GetStringLength(jlocalestr);
		env->GetStringUTFRegion( jlocalestr, 0,len, localestrbuf);
		localestrbuf[2]='_';
		setlocale(localestrbuf,5);

		}
	}

extern std::string_view libdirname;
extern int setfilesdir(const std::string_view filesdir,const char *country) ;
extern "C" JNIEXPORT int JNICALL fromjava(setfilesdir)(JNIEnv *env, jclass clazz, jstring dir,jstring jcountry,jstring nativedir) {
	{
	size_t nativedirlen= env->GetStringUTFLength( nativedir);
	char *nativebuf=new char[nativedirlen+1];
	env->GetStringUTFRegion( nativedir, 0,nativedirlen,nativebuf);
	nativebuf[nativedirlen]='\0';
	libdirname={nativebuf,nativedirlen};
	}
	size_t filesdirlen= env->GetStringUTFLength( dir);
	jint jdirlen = env->GetStringLength( dir);
	char *filesdirbuf=new char[filesdirlen+1];
	env->GetStringUTFRegion( dir, 0,jdirlen, filesdirbuf);
	filesdirbuf[filesdirlen]='\0';
	char *country= localestrbuf+3;
	if(jcountry&& env->GetStringLength(jcountry)>=2) {
		env->GetStringUTFRegion( jcountry, 0,2,country );
		country[2]='\0';
		LOGGER("country=%s\n",country);
		}
	else {
		LOGAR("country=null\n");
		strcpy( country,"GB");
		country=(char *)"\0";
		}
	return setfilesdir({filesdirbuf,filesdirlen},country);
	}
void calccurvegegs();

extern "C" JNIEXPORT void JNICALL fromjava(calccurvegegs)(JNIEnv *env, jclass clazz) {
	calccurvegegs();

	}

extern void flingX(float vol);
extern "C" JNIEXPORT void JNICALL fromjava(flingX) (JNIEnv *env, jclass clazz,jfloat vol) {
	flingX(vol);
	}

extern int translate(float dx,float dy,float yold,float y);
extern "C" JNIEXPORT jint JNICALL fromjava(translate) (JNIEnv *env, jclass clazz,jfloat dx,jfloat dy,jfloat yold,jfloat y) {
	return translate(dx,dy,yold,y);
	}

void xscaleGesture(float scalex,float midx);
extern "C" JNIEXPORT void JNICALL fromjava(xscale) (JNIEnv *env, jclass clazz,jfloat scalex,jfloat midx) {
	xscaleGesture(scalex,midx);
	}

void prevscr() ;
extern "C" JNIEXPORT void JNICALL fromjava(prevscr)(JNIEnv* env, jclass obj) {
	prevscr();
	}
void  nextscr() ;
extern "C" JNIEXPORT void JNICALL fromjava(nextscr)(JNIEnv* env, jclass obj) {
	 nextscr() ;
	 }

void pressedback() ;
extern "C" JNIEXPORT void JNICALL fromjava(pressedback) (JNIEnv *env, jclass clazz) {
	pressedback() ;
	}
bool	isbutton(float x,float y) ;
extern "C" JNIEXPORT jboolean JNICALL fromjava(isbutton) (JNIEnv *env, jclass clazz,jfloat x,jfloat y) {
	return isbutton( x, y);
	}


int64_t screentap(float x,float y);
extern "C" JNIEXPORT jlong JNICALL fromjava(tap) (JNIEnv *env, jclass clazz,jfloat x,jfloat y) {
	return screentap(x,y);
	}

int64_t longpress(float x,float y);
extern "C" JNIEXPORT jlong JNICALL fromjava(longpress) (JNIEnv *env, jclass clazz,jfloat x,jfloat y) {
	return longpress(x,y);
	}

#include "numhit.h"

extern NumHit newhit;
#include "numdisplay.h"
#include "nums/num.h"
extern "C" JNIEXPORT jlong JNICALL fromjava(newhit) (JNIEnv *env, jclass clazz) {
		return reinterpret_cast<jlong>(&newhit);
		}
extern "C" JNIEXPORT jlong JNICALL fromjava(hittime) (JNIEnv *env, jclass clazz,jlong ptr) {
	NumHit *num=reinterpret_cast<NumHit *>(ptr);
	return num->hit->time;
	}

extern "C" JNIEXPORT jfloat JNICALL fromjava(hitvalue) (JNIEnv *env, jclass clazz,jlong ptr) {
	NumHit *num=reinterpret_cast<NumHit *>(ptr);
	return num->hit->value;
	}
extern "C" JNIEXPORT jint JNICALL fromjava(hittype) (JNIEnv *env, jclass clazz,jlong ptr) {
	NumHit *num=reinterpret_cast<NumHit *>(ptr);
	return num->hit->type;
	}
extern "C" JNIEXPORT jint JNICALL fromjava(hitmeal) (JNIEnv *env, jclass clazz,jlong ptr) {
	NumHit *num=reinterpret_cast<NumHit *>(ptr);
	return num->hit->mealptr;
	}

int  hitremove(int64_t ptr);
extern "C" JNIEXPORT jint JNICALL fromjava(hitremove) (JNIEnv *env, jclass clazz,jlong ptr) {
	return hitremove(ptr);
	}
extern "C" JNIEXPORT jint JNICALL fromjava(gethitindex) (JNIEnv *env, jclass clazz,jlong ptr) {
	NumHit *num=reinterpret_cast<NumHit *>(ptr);
	const NumDisplay *dis=num->numdisplay;
	if(!dis)
		return 1;
	return dis->getindex();
}

extern "C" JNIEXPORT void JNICALL fromjava(hitchange)(JNIEnv *env, jclass thiz,jlong ptr,jlong time,jfloat value,int type,int mealptr) {
	NumHit *num=reinterpret_cast<NumHit *>(ptr);
	num->numdisplay->numchange(num->hit,time,value,type,mealptr);
	}
extern "C" JNIEXPORT void JNICALL fromjava(hitsetmealptr)(JNIEnv *env, jclass thiz,jlong ptr,jint mealptr) {
	NumHit *num=reinterpret_cast<NumHit *>(ptr);
	num->numdisplay->setmealptr(const_cast<Num *>(num->hit),mealptr);
	}

extern "C" JNIEXPORT jlong JNICALL fromjava(mkhitptr) (JNIEnv *env, jclass clazz,jlong ptr,jint pos) {
	NumDisplay *dis=reinterpret_cast<NumDisplay*>(ptr);
	NumHit *num=new NumHit({dis,dis->startdata()+pos});
	return reinterpret_cast<jlong>(num);
	}
extern "C" JNIEXPORT void JNICALL fromjava(freehitptr)(JNIEnv *env, jclass thiz,jlong ptr) {
	delete reinterpret_cast<NumHit *>(ptr);
	}
extern uint32_t starttime;
extern int duration;
extern "C" JNIEXPORT jlong JNICALL fromjava(getstarttime) (JNIEnv *env, jclass clazz) {
	return static_cast<jlong>(starttime)*1000l;
	};

extern "C" JNIEXPORT jlong JNICALL fromjava(getendtime) (JNIEnv *env, jclass clazz) {
	auto end=	std::min(starttime+duration,(uint32_t)time(nullptr));
	return static_cast<jlong>(end)*1000l;
	};

extern uint32_t mintime();
extern "C" JNIEXPORT jlong JNICALL fromjava(oldestdatatime)(JNIEnv *env, jclass thiz) {
	return static_cast<jlong>(mintime())*1000l;
		}

void stopsearch() ;
extern "C" JNIEXPORT void JNICALL fromjava(stopsearch) (JNIEnv *env, jclass clazz) {
	stopsearch();
	}
int nextforward() ;
extern "C" JNIEXPORT jint JNICALL fromjava(latersearch) (JNIEnv *env, jclass clazz) {
	return nextforward();
	}
int nextpast();
extern "C" JNIEXPORT jint JNICALL fromjava(earliersearch) (JNIEnv *env, jclass clazz) {
	return nextpast();
	}

	extern int carbotype;
int searchcommando(int type, float under,float above,int frommin,int tomin,bool forward,const char *regingr,float amount) ;
extern "C" JNIEXPORT jint JNICALL fromjava(search) (JNIEnv *env, jclass clazz,jint type, jfloat under,jfloat above,jint frommin,jint tomin,jboolean forward,jstring jregingr,jfloat amount) {
        const char *regingr=nullptr;
	if(jregingr!=nullptr&&type==carbotype)  {
        	regingr=env->GetStringUTFChars( jregingr, nullptr);
		if(regingr == nullptr) {

			return 3;
			}
		}
	jint res= searchcommando(type, under,above,frommin,tomin,forward,regingr, amount);
	if(regingr)
		env->ReleaseStringUTFChars(jregingr, regingr);
	return res;
	}

void begrenstijd();
extern "C" JNIEXPORT void JNICALL fromjava(movedate) (JNIEnv *env, jclass clazz,jlong milli,jint year,jint month,jint day) {
	time_t tim=milli/1000l;
	struct tm		stm{};
	localtime_r(&tim,&stm);
	stm.tm_year=year-1900;
	stm.tm_mon=month;
	stm.tm_mday=day;
	time_t timto=mktime(&stm);
	starttime+=uint32_t((int64_t)timto-(int64_t)tim);
	begrenstijd() ;
	};
void prevdays(int nr);
extern "C" JNIEXPORT void JNICALL fromjava(prevday)(JNIEnv* env, jclass obj,jint val) {
	prevdays(val);
	}

void nextdays(int nr) ;
extern "C" JNIEXPORT void JNICALL fromjava(nextday)(JNIEnv* env, jclass obj,jint val) {
	nextdays(val);
	}
/*
extern "C" JNIEXPORT jlong JNICALL fromjava(lastpoll)(JNIEnv *env, jclass thiz) {
	const SensorGlucoseData *hist=sensors->getSensorData(); 
	const ScanData *glu=hist->lastpoll() ;
	if(glu)
		return (((jlong)glu->g)<<32)|(jlong)glu->t;
	else
		return 0LL;
	}*/
//#include "oldest.h"
#ifndef WEAROS
void numfirstpage() ;
extern "C" JNIEXPORT void JNICALL fromjava(firstpage)(JNIEnv *env, jclass thiz) {
	numfirstpage();
	}

void numpagenum(const uint32_t tim) ;
uint32_t maxtime();
extern "C" JNIEXPORT void JNICALL fromjava(lastpage)(JNIEnv *env, jclass thiz) {
	numpagenum(maxtime());
	}

void endnumlist() ;
extern "C" JNIEXPORT void JNICALL fromjava(endnumlist)(JNIEnv *env, jclass thiz) {
	 endnumlist();
	}
#endif
extern int showui;
extern "C" JNIEXPORT jboolean JNICALL fromjava(getsystemUI)(JNIEnv *env, jclass thiz) {
	return showui;
	}
int64_t openNums(std::string_view numpath,int64_t ident);

extern pathconcat numbasedir;
extern "C" JNIEXPORT jlong JNICALL fromjava(openNums)(JNIEnv *env, jclass thiz,jstring jbase,jlong ident) {
	LOGAR("start openNums");
	jint len = env->GetStringUTFLength( jbase);
	int blen=numbasedir.length();
	int alllen=len+blen+1;
	char base[alllen+1];
	memcpy(base,numbasedir.data(),blen);
	base[blen++]='/';
	jint jlen = env->GetStringLength( jbase);
	env->GetStringUTFRegion(jbase, 0,jlen, base+blen);
	base[alllen]='\0';
	const auto res= openNums( string_view(base,alllen),ident);
	LOGAR("end openNums");
	return res;
	}
#include "curve.h"
extern "C" JNIEXPORT void JNICALL fromjava(setlastcolor)(JNIEnv *env, jclass thiz,jint color) {
	LOGGER("lasttouchedcolor=%d color=%x\n",lasttouchedcolor,color);
	if(lasttouchedcolor<0)
		return;
	setcolor(lasttouchedcolor, hexcoloralpha((uint32_t)color));
	}
extern "C" JNIEXPORT jint JNICALL fromjava(getlastcolor)(JNIEnv *env, jclass thiz) {
	if(lasttouchedcolor<0)
		return 0xFFFFFFFF;
	return  fromNVGcolor(getcolor(lasttouchedcolor));
	}
#ifndef WEAROS
extern bool makepercetages() ;
extern "C" JNIEXPORT jboolean JNICALL fromjava(makepercentages)(JNIEnv *env, jclass thiz) {
	return makepercetages();
	}
extern int numlist;
				extern void numiterinit();
extern "C" JNIEXPORT void JNICALL fromjava(makenumbers)(JNIEnv *env, jclass thiz) {
			if(!numlist) {
				numiterinit();
				numlist=1;
				}
			}
#endif
/*
static float screenwidthcm=0;
//extern bool iswatch;
extern "C" JNIEXPORT void JNICALL fromjava(setscreenwidthcm)(JNIEnv *env, jclass thiz,jfloat wcm) {
	screenwidthcm=wcm;
//	iswatch=(wcm<5.8f);
	}
*/
extern int showstream;
extern int showscans;
extern int showhistories;
extern int shownumbers;
extern int showmeals;
#define defdisplay(kind)\
extern "C" JNIEXPORT jboolean JNICALL fromjava(getshow##kind)(JNIEnv *env, jclass thiz) {\
	return show##kind;\
	}\
extern "C" JNIEXPORT void JNICALL fromjava(setshow##kind)(JNIEnv *env, jclass thiz,jboolean val) {\
	show##kind=val;\
	}

defdisplay(scans)
defdisplay(meals)
defdisplay(histories)
defdisplay(stream)
defdisplay(numbers)

void speak(const char *message) {
	if(message)
		getenv()->CallStaticVoidMethod(JNIApplic,jspeak,getenv()->NewStringUTF(message));
	else {
		LOGAR("speak(null)");
		}
	}

#ifndef WEAROS
extern bool speakout;
extern "C" JNIEXPORT void JNICALL fromjava(settouchtalk)(JNIEnv *env, jclass thiz,jboolean val) {

	settings->data()->talktouch=val;
	speakout=val;
	}

extern "C" JNIEXPORT jboolean JNICALL fromjava(gettouchtalk)(JNIEnv *env, jclass thiz) {
	return speakout;
	}
extern "C" JNIEXPORT void JNICALL fromjava(setspeakmessages)(JNIEnv *env, jclass thiz,jboolean val) {

	settings->data()->speakmessages=val;
	}

extern "C" JNIEXPORT jboolean JNICALL fromjava(speakmessages)(JNIEnv *env, jclass thiz) {
	return settings->data()->speakmessages;
	}
extern "C" JNIEXPORT void JNICALL fromjava(setspeakalarms)(JNIEnv *env, jclass thiz,jboolean val) {

	settings->data()->speakalarms=val;
	}

extern "C" JNIEXPORT jboolean JNICALL fromjava(speakalarms)(JNIEnv *env, jclass thiz) {
	return settings->data()->speakalarms;
	}
extern	const SensorGlucoseData *getlaststream(const uint32_t nu);
extern "C" JNIEXPORT jlong JNICALL fromjava(saylastglucose)(JNIEnv *env, jclass thiz) {
	if(speakout) {
		const uint32_t nu=time(nullptr);        
		const auto *hist=getlaststream(nu);
		if(!hist)  {
			LOGAR("getlaststream(nu)=null");
			return 0;
			}
		const ScanData *poll=hist->lastpoll();
		if(!poll||!poll->valid()) {
			return 0L;
			}
		if(poll->gettime()<(nu-60*3)) {
			return poll->gettime()*1000L;
			}
		
		const auto nonconvert= poll->g;
		if(!nonconvert)  {
			LOGAR("glucose = 0");
			return 0L;
			}
		constexpr const int maxvalue=120;
		char value[maxvalue];
		auto trend=usedtext->trends[poll->tr];
		memcpy(value,trend.data(),trend.size());
		char *ptr=value+trend.size();
		*ptr++='\n';
		const float glucosevalue= gconvert(nonconvert*10);
		snprintf(ptr,maxvalue,gformat,glucosevalue);
		LOGGER("saylastglucose %s\n",value);
		speak(value);
		}
	return -1L;
	}

extern "C" JNIEXPORT jboolean JNICALL fromjava(getsystemui)(JNIEnv *env, jclass thiz) {
	return showui;
	}
extern "C" JNIEXPORT void JNICALL fromjava(setsystemui)(JNIEnv *env, jclass thiz,jboolean val) {
	showui=val;
	settings->setui(showui);
	}

extern "C" JNIEXPORT void JNICALL fromjava(settonow)(JNIEnv *env, jclass thiz) {
	auto max=time(nullptr);
	starttime=max-duration*3/5;
	}

#include "sensoren.h"

extern struct lastscan_t scantoshow;
extern "C" JNIEXPORT jboolean JNICALL fromjava(showlastscan)(JNIEnv *env, jclass thiz) {
	int lastsensor=sensors->lastscanned();
	if(lastsensor>=0) {
		const SensorGlucoseData *hist=sensors->getSensorData(lastsensor);
		if(hist) {
			const ScanData *scan= hist->lastscan();
			const uint32_t nu=time(nullptr);
			if(scan&&scan->valid()&&((nu-scan->t)<(60*60*5))) {
				scantoshow={lastsensor,scan,nu};
				return true;
				}
			}
		}
	return false;
	}

#endif

