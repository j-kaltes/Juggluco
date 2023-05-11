#if 1
//#ifndef WEAROS
#include <jni.h>
#include <ctime>
#include <memory>
#include "settings/settings.h"
#include "share/logs.h"
#include "sensoren.h"
extern Settings *settings;
extern Sensoren *sensors;
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
jstring jnightuploadurl=nullptr;
jstring jnightuploadsecret= nullptr;
void makeuploadurl(JNIEnv *env) {
		const int namelen=settings->data()->nightuploadnamelen;
		const char *name=settings->data()->nightuploadname;
		static constexpr const char lasturl[]=R"(/api/v1/entries)";
		char fullname[namelen+sizeof(lasturl)];
		memcpy(fullname,name,namelen);
		memcpy(fullname+namelen,lasturl,sizeof(lasturl));
		if(jnightuploadurl)
			env->DeleteGlobalRef(jnightuploadurl);
		auto local=env->NewStringUTF(fullname);
		jnightuploadurl=  (jstring)env->NewGlobalRef(local);
		env->DeleteLocalRef(local);
		}

extern std::string sha1encode(const char *secret, int len);
void makeuploadsecret(JNIEnv *env) {
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
	makeuploadurl(env);
extern void startuploaderthread();
	startuploaderthread();
	return true;
       }

//static boolean upload(String httpurl,byte[] postdata,String secret) ;
static void reset() {
	const int last=sensors->last();
	settings->data()->nightsensor=0;
	for(int sensorid=0;sensorid<=last;sensorid++) {
		if(SensorGlucoseData *sens=sensors->gethist(sensorid)) {
			sens->getinfo()->nightiter=0;
			}
		}
	}
bool nightupload(const char *data,int len) {
	const static jmethodID  upload=getenv()->GetStaticMethodID(nightpostclass,"upload","(Ljava/lang/String;[BLjava/lang/String;)Z");
	auto env=getenv();
	jbyteArray uit=env->NewByteArray(len);
        env->SetByteArrayRegion(uit, 0, len,(const jbyte *)data);
	bool res=env->CallStaticBooleanMethod(nightpostclass,upload,jnightuploadurl,uit,jnightuploadsecret);
	LOGGER("nightupload=%d\n",res);
	env->DeleteLocalRef(uit);
	return res;
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

//	return sprintf(buf,R"({"type":"sgv","device":"Juggluco","dateString":"%s","date":%lld,"sgv":%d,"delta":%.3f,"direction":"%s","noise":1,"filtered":%d,"unfiltered":%d,"rssi":100},)",tim*1000LL,timestr,mgdL,delta,directionlabel,mgdL*1000,mgdL*1000);
//	return sprintf(buf,R"({"type":"sgv","device":"Juggluco","dateString":"%s","date":%lld,"sgv":%d,"delta":%.3f,"direction":"%s","noise":1,"filtered":%d,"unfiltered":%d,"rssi":100,"sysTime":"%s"},)",timestr,tim*1000LL,mgdL,delta,directionlabel,mgdL*1000,mgdL*1000,timestr);
	return sprintf(buf,R"({"type":"sgv","device":"%s","dateString":"%s","date":%lld,"sgv":%d,"delta":%.3f,"direction":"%s","noise":1,"filtered":%d,"unfiltered":%d,"rssi":100},)",sensorname,timestr,tim*1000LL,mgdL,delta,directionlabel,mgdL*1000,mgdL*1000);
//	return sprintf(buf,R"({"type":"sgv","device":"Juggluco","date":%lld,"sgv":%d,"delta":%.3f,"direction":"%s"},)",tim*1000LL,mgdL,delta,directionlabel);

	}

bool upload() {
	LOGSTRING("upload\n");
	int last=sensors->last();
	if(last<0)
		return false;
	constexpr const int timeback=60*60*24*30;
	time_t nu=time(nullptr);
	uint32_t mintime=nu-timeback;
	if(!settings->data()->nightsensor)
		settings->data()->nightsensor=sensors->firstafter(mintime);
	int startsensor= settings->data()->nightsensor;
	constexpr const int itemsize=350;

	constexpr const auto twoweeks=15*24*60*60;
	time_t old=nu-twoweeks;

	int newstartsensor=startsensor;
	for(int sensorid=last;sensorid>=startsensor;--sensorid) {
		if(SensorGlucoseData *sens=sensors->gethist(sensorid)) {
			std::span<const ScanData> gdata=sens->getPolldata();
			const char *sensorname=sens->showsensorname().data();
			int len=gdata.size();
			int positer=sens->getinfo()->nightiter;
			LOGGER("%d: positer=%d\n",sensorid,positer);
			int left=len-positer;
			if(left>=0) {
				const int arraysize=3+left*itemsize;
				LOGGER("arraysize=%d\n",arraysize);
				char  *start=new(std::nothrow)  char[arraysize];
				if(!start)
					return false;
				unique_ptr<char[]> destruct(start);
				char *ptr=start;
				*ptr++='[';
				for(;positer<len;positer++) { //Geen overlappende data?
					const ScanData &el= gdata[positer];
					if(el.valid(positer)&&el.gettime()>mintime)
						ptr+=mkuploaditem(ptr,sensorname,el);
					}
				LOGGER("%d new positer=%d\n",sensorid,len);
				if(ptr>(start+1)) {
					ptr[-1]=']';
					*ptr='\0';
					int datalen=ptr-start;
					LOGGER("%d: UPLOADER #%d\n",sensorid,datalen);
					LOGGERN(start,datalen);
					if(nightupload(start,datalen)) {
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
		uploadercondition.dobackup=0;
		if(upload()) {
			waitmin=5*60;
			}
		else {
			waitmin=15;
			continue;
			}
		}
	}

void wakeuploader() {
	if(uploaderrunning) 
		uploadercondition.wakebackup(Backup::wakeall);
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
