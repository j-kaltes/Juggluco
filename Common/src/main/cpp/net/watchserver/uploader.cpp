#if 1
//#ifndef WEAROS
#include <jni.h>
#include <ctime>
#include <memory>
#include "settings/settings.h"
#include "share/logs.h"
#include "sensoren.h"
#include "nums/numdata.h"
#include "common.h"
extern Settings *settings;
extern Sensoren *sensors;
constexpr int HTTP_OK=200;
/*[
  {
    "type": "string",
    "dateString": "string",
    "date": 0,
    "sgv": 0,
    "direction": "string",
    "noise": 0,
    "filtered": 0,
    "unfiltered": 0,
    "rssi": 0
  }
]

[{"device":"xDrip-LibreReceiver","date":1678294924000,"dateString":"2023-03-08T18:02:04.000+0100","sgv":63,"delta":-1.993,"direction":"Flat","type":"sgv","filtered":63000,"unfiltered":63000,"rssi":100,"noise":1}]1
*/
extern JNIEnv *getenv();

jclass nightpostclass=nullptr;
jstring jnightuploadEntriesurl=nullptr;
jstring jnightuploadEntries3url=nullptr;
jstring jnightuploadTreatmentsurl=nullptr;
jstring jnightuploadTreatments3url=nullptr;
jstring jnightuploadsecret= nullptr;
/*
void makeuploadurl(JNIEnv *env) {
		const int namelen=settings->data()->nightuploadnamelen;
		const char *name=settings->data()->nightuploadname;
		static constexpr const char lasturl[]=R"(/api/v1/entries)";
		char fullname[namelen+sizeof(lasturl)];
		memcpy(fullname,name,namelen);
		memcpy(fullname+namelen,lasturl,sizeof(lasturl));
		if(jnightuploadEntriesurl)
			env->DeleteGlobalRef(jnightuploadEntriesurl);
		auto local=env->NewStringUTF(fullname);
		jnightuploadEntriesurl=  (jstring)env->NewGlobalRef(local);
		env->DeleteLocalRef(local);
		} */
static void makeuploadurl(JNIEnv *env,std::string_view pathstr,jstring &url) {

		const int namelen=settings->data()->nightuploadnamelen;
		const char *name=settings->data()->nightuploadname;
		char fullname[namelen+pathstr.size()+1];
		memcpy(fullname,name,namelen);
		memcpy(fullname+namelen,pathstr.data(),pathstr.size()+1);
		auto local=env->NewStringUTF(fullname);

		if(url)
			env->DeleteGlobalRef(url);
		url=  (jstring)env->NewGlobalRef(local);
		env->DeleteLocalRef(local);
		}
static void makeuploadurls(JNIEnv *env) {
	makeuploadurl(env,R"(/api/v1/entries)",jnightuploadEntriesurl);
	makeuploadurl(env,R"(/api/v3/entries)",jnightuploadEntries3url);
	makeuploadurl(env,R"(/api/v1/treatments)",jnightuploadTreatmentsurl);
	makeuploadurl(env,R"(/api/v3/treatments)",jnightuploadTreatments3url);
	}

extern std::string sha1encode(const char *secret, int len);
static void makeuploadsecret(JNIEnv *env) {
		if(jnightuploadsecret)
			env->DeleteGlobalRef(jnightuploadsecret);

		const char *secret=settings->data()->nightuploadsecret;
		std::string encoded=sha1encode(secret,strlen(secret));
		auto local=env->NewStringUTF(encoded.data());
		jnightuploadsecret=  (jstring)env->NewGlobalRef(local);
		}
bool inituploader(JNIEnv *env) {
	if(!settings->data()->nightuploadon)  
		return false;
	if(nightpostclass==nullptr) {
		constexpr const char nightpostclassstr[]="tk/glucodata/NightPost";
		if(jclass cl=env->FindClass(nightpostclassstr)) {
			LOGGER("found %s\n",nightpostclassstr);
			nightpostclass=(jclass)env->NewGlobalRef(cl);
		       env->DeleteLocalRef(cl);
		       }
	        else  {
			LOGGER("FindClass(%s) failed\n",nightpostclassstr);
			return false;
			}
		}
	makeuploadsecret(env); 
	makeuploadurls(env);
extern void startuploaderthread();
	startuploaderthread();
	LOGAR("end inituploader");
	return true;
       }

//static boolean upload(String httpurl,byte[] postdata,String secret) ;
extern vector<Numdata*> numdatas;
static void reset() {
	const int last=sensors->last();
	settings->data()->nightsensor=0;
	settings->data()->lastuploadtime=0;
	for(int sensorid=0;sensorid<=last;sensorid++) {
		if(SensorGlucoseData *sens=sensors->getSensorData(sensorid)) {
			sens->getinfo()->nightiter=0;
			}
		}
	for(auto *numdata:numdatas)
		numdata->setNightSend(0);
	}
static int nightupload(jstring jnightuploadurl,const char *data,int len,bool put) {
	const static jmethodID  upload=getenv()->GetStaticMethodID(nightpostclass,"upload","(Ljava/lang/String;[BLjava/lang/String;Z)I");
	auto env=getenv();
	jbyteArray uit=env->NewByteArray(len);
        env->SetByteArrayRegion(uit, 0, len,(const jbyte *)data);
	int res=env->CallStaticIntMethod(nightpostclass,upload,jnightuploadurl,uit,jnightuploadsecret,put);
	LOGGER("nightupload=%d\n",res);
	env->DeleteLocalRef(uit);
	return res;
	}
int nightuploadEntries(const char *data,int len) {
	return nightupload(jnightuploadEntriesurl,data,len,false);
	}
int nightuploadEntries3(const char *data,int len) {
	return nightupload(jnightuploadEntries3url,data,len,false);
	}

int nightuploadTreatments(const char *data,int len) {
	return nightupload(jnightuploadTreatmentsurl,data,len,true);
	}
int nightuploadTreatments3(const char *data,int len) {
	return nightupload(jnightuploadTreatments3url,data,len,false);
	}


//extern int Tdatestring(time_t tim,char *buf) ;
#include "datestring.h"
extern double getdelta(float change);
extern std::string_view getdeltaname(float change);
template <class T> int mkuploaditem(char *buf,const char *sensorname,const T &item) {
	const time_t tim=item.gettime();
	int mgdL=item.getmgdL();
	float change= item.getchange();
	const char * directionlabel=getdeltaname(change).data();
	double delta=getdelta(change);
	char timestr[50];
	Tdatestring(tim,timestr);

	return sprintf(buf,R"({"type":"sgv","device":"%s","dateString":"%s","date":%lld,"sgv":%d,"delta":%.3f,"direction":"%s","noise":1,"filtered":%d,"unfiltered":%d,"rssi":100},)",sensorname,timestr,tim*1000LL,mgdL,delta,directionlabel,mgdL*1000,mgdL*1000);

	}

template <class T> int mkuploaditemv3(char *buf,const char *sensorname,const T &item) {
	const time_t tim=item.gettime();
	int mgdL=item.getmgdL();
	float change= item.getchange();
	const char * directionlabel=getdeltaname(change).data();
	double delta=getdelta(change);
	char timestr[50];
	Tdatestring(tim,timestr);

	return sprintf(buf,R"({"type":"sgv","device":"%s","dateString":"%s","date":%lld,"sgv":%d,"delta":%.3f,"direction":"%s","noise":1,"filtered":%d,"unfiltered":%d,"rssi":100},)",sensorname,timestr,tim*1000LL,mgdL,delta,directionlabel,mgdL*1000,mgdL*1000);

	}

extern  const int nighttimeback;
const int nighttimeback=60*60*24*30;

static bool uploadCGM3() {
	LOGSTRING("upload\n");
	int last=sensors->last();
	if(last<0) {
		LOGAR("No sensors");
		return true;
		}
	time_t nu=time(nullptr);
	uint32_t mintime=nu-nighttimeback;
	if(!settings->data()->nightsensor)
		settings->data()->nightsensor=sensors->firstafter(mintime);
	int startsensor= settings->data()->nightsensor;

	constexpr const auto twoweeks=15*24*60*60;
	time_t old=nu-twoweeks;

	int newstartsensor=startsensor;
	for(int sensorid=last;sensorid>=startsensor;--sensorid) {
		if(SensorGlucoseData *sens=sensors->getSensorData(sensorid)) {
			std::span<const ScanData> gdata=sens->getPolldata();
			const sensorname_t *sensorname=sens->shortsensorname();
			int len=gdata.size();
			int positer=sens->getinfo()->nightiter;
			LOGGER("%d: positer=%d\n",sensorid,positer);
			int left=len-positer;
			bool send=false;
			if(left>=0) {
				for(;positer<len;positer++) { //Geen overlappende data?
					const ScanData *el= &gdata[positer];
					if(el->valid(positer)&&el->gettime()>mintime) {
						constexpr const int max3entry=300;
						char buf[max3entry];
extern char * writev3entry(char *outin,const ScanData *val, const sensorname_t *sensorname,bool server=true);
						const char *ptr=writev3entry(buf,el, sensorname,false);
						const int buflen=ptr-buf;
						logwriter(buf,buflen);
						auto res=nightuploadEntries3(buf,buflen);
						if(res==200||res==201) {
							sens->getinfo()->nightiter=positer+1;
							send=true;
							}
						else {
							LOGSTRING("nightupload failure\n");
							if(send)
								settings->data()->nightsensor=sensorid;
							else
								settings->data()->nightsensor=newstartsensor;
							return false;
							}
						}
					}
				if(send)
					newstartsensor=sensorid;
				continue;
				}
			if(sens->getstarttime()>old) 
				newstartsensor=sensorid;

			}
		}
	settings->data()->nightsensor=newstartsensor;
	return true;
	}
static bool uploadCGM() {
	LOGSTRING("upload\n");
	int last=sensors->last();
	if(last<0) {
		LOGAR("No sensors");
		return true;
		}
	time_t nu=time(nullptr);
	uint32_t mintime=nu-nighttimeback;
	if(!settings->data()->nightsensor)
		settings->data()->nightsensor=sensors->firstafter(mintime);
	int startsensor= settings->data()->nightsensor;
	constexpr const int itemsize=350;

	constexpr const auto twoweeks=15*24*60*60;
	time_t old=nu-twoweeks;

	int newstartsensor=startsensor;
	for(int sensorid=last;sensorid>=startsensor;--sensorid) {
		if(SensorGlucoseData *sens=sensors->getSensorData(sensorid)) {
			std::span<const ScanData> gdata=sens->getPolldata();
			const sensorname_t *sensorname=sens->shortsensorname();
			const char *sensornamestr=sensorname->data();
			int len=gdata.size();
			int positer=sens->getinfo()->nightiter;
			LOGGER("%d: positer=%d\n",sensorid,positer);
			int left=len-positer;
			if(left>=0) {
constexpr const int			maxitems=10440;
				if(left>maxitems) {
					left=maxitems;
					len=positer+left;
					}
				const int arraysize=3+left*itemsize;
				LOGGER("arraysize=%d\n",arraysize);
				char  *start=new(std::nothrow)  char[arraysize];
				if(!start) {
					LOGGER("new char[%d] failed\n",arraysize);
					return false;
					}
				unique_ptr<char[]> destruct(start);
				char *ptr=start;
				*ptr++='[';
				for(;positer<len;positer++) { //Geen overlappende data?
					const ScanData &el= gdata[positer];
					if(el.valid(positer)&&el.gettime()>mintime) {
						ptr+=mkuploaditem(ptr,sensornamestr,el);
						}
					}
				LOGGER("%d new positer=%d\n",sensorid,len);
				if(ptr>(start+1)) {
					ptr[-1]=']';
					*ptr='\0';
					int datalen=ptr-start;
					LOGGER("%d: UPLOADER #%d\n",sensorid,datalen);
					LOGGERN(start,datalen);
					if(nightuploadEntries(start,datalen)==HTTP_OK) {
						sens->getinfo()->nightiter=len;
						LOGGER("%d nightupload Success\n",sensorid);
						LOGGER("%d saved nightiter=%d\n", sensorid,len);
						newstartsensor=sensorid;
						continue;
						}
					else {
						LOGSTRING("nightupload failure\n");
						return false;
						}
					}
				}
			if(sens->getstarttime()>old) 
				newstartsensor=sensorid;

			}
		}
	settings->data()->nightsensor=newstartsensor;
	return true;
	}

#include "datbackup.h"
Backup::condvar_t  uploadercondition;
bool uploaderrunning=false;

extern bool networkpresent;

extern bool uploadtreatments(bool);
static void uploaderthread() {
	int waitmin=0;
	uploaderrunning=true;
	const char view[]{"UPLOADER"};
	LOGGERN(view,sizeof(view)-1);
       prctl(PR_SET_NAME, view, 0, 0, 0);

	while(true) {

		  if(!networkpresent||!uploadercondition.dobackup) {
			if(!networkpresent) {
				waitmin=60;
				}
		        std::unique_lock<std::mutex> lck(uploadercondition.backupmutex);
			LOGGER("UPLOADER before lock waitmin=%d\n",waitmin);
 			auto now = std::chrono::system_clock::now();
			auto status=uploadercondition.backupcond.wait_until(lck, now + std::chrono::minutes(waitmin));
			LOGGER("UPLOADER after lock %stimeout\n",(status==std::cv_status::no_timeout)?"no-":"");
			}
		if(uploadercondition.dobackup&Backup::wakeend) {
			uploadercondition.dobackup=0;
			uploaderrunning=false;
			LOGSTRING("end uploaderthread\n");
			return;
			}
		const auto current=uploadercondition.dobackup;
		uploadercondition.dobackup=0;
		bool useV3=settings->data()->nightscoutV3;
		if(current&(Backup::wakestream|Backup::wakeall)) {
			if(!(useV3?uploadCGM3():uploadCGM())) {
				waitmin=15;
				continue;
				}
			}
#ifndef WEAROS
		if(current&(Backup::wakenums|Backup::wakeall)) {
			if(!uploadtreatments(useV3)) {
				waitmin=15;
				continue;
				}
			}
#endif
		waitmin=5*60;
		}
	}

void wakeuploader() {
	if(uploaderrunning) 
		uploadercondition.wakebackup(Backup::wakeall);
	}
void wakestreamuploader() {
	if(uploaderrunning) 
		uploadercondition.wakebackup(Backup::wakestream);
	}
#include "fromjava.h"	
extern "C" JNIEXPORT void JNICALL fromjava(wakeuploader) (JNIEnv *env, jclass clazz) {
	wakeuploader();
	} 
extern "C" JNIEXPORT void JNICALL fromjava(resetuploader) (JNIEnv *env, jclass clazz) {
	reset();
	} 

void enduploaderthread() {
	if(uploaderrunning) {
		uploadercondition.wakebackup(Backup::wakeend);
		}
	}
void startuploaderthread() {
	if(!uploaderrunning) {
		std::thread libre(uploaderthread);
		libre.detach();
		} 
	}
#endif
