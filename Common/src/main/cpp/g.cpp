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


//
// Created by jka on 27-11-20.
//
#include <cinttypes>
#include <jni.h>
#include <stdio.h>
#include <unistd.h>
    #include <sys/types.h>
       #include <sys/wait.h>
#include <sys/prctl.h>
#include <signal.h>
#include <stdlib.h>
#include <thread>
#include <future>
//#include <sys/syscall.h>
//#undef NOLOG
       #include "logs.h"
       #include "inout.h"
#include "jnisub.h"
#include "serial.h"
#include "fromjava.h"

#include "settings/settings.h"
#include "datbackup.h"
#include "error_codes.h"
#ifdef DYNLINK
#define abbottdec(x) (*x)
#define abbottcall(x) x
#else
#define abbottdec(x) Java_com_abbottdiabetescare_flashglucose_sensorabstractionservice_dataprocessing_DataProcessingNative_##x
#define abbottcall(x) Java_com_abbottdiabetescare_flashglucose_sensorabstractionservice_dataprocessing_DataProcessingNative_##x
#endif
extern "C" JNIEXPORT jlong JNICALL   abbottdec(getStatusCode)(JNIEnv *env, jobject obj, jstring serial, jint elapsed1, jint elapsed2, jint jint4, jboolean endedearly) ;

//extern string getstatus(JNIEnv *env, jobject thiz, const char *serial,time_t start,time_t lastread,time_t nu,bool endedearly) ;
void testserial(JNIEnv *env, jobject thiz) ;
#ifdef NDEBUG
#define EXTRA 50
#else
#define EXTRA 20
#endif
bool setbluetoothon=false;
extern					void setusedsensors() ;
#include "debugclone.h"
extern int    timestr(char *buf,time_t tim) ;
extern lastscan_t  scantoshow;
//JNIEXPORT void JNICALL Java_tk_glucodata_Glucose_nfcdata(JNIEnv *env, jobject thiz, jbyteArray uid, jbyteArray info,jbyteArray data) {
//JNIEXPORT int JNICALL Java_tk_glucodata_Natives_nfcdata(JNIEnv *env, jclass thiz, jbyteArray uid, jbyteArray info,jbyteArray data) {
//#define SCANLOG
#if defined(SCANLOG) //|| defined(__arm__) 

class scanlogger {
	int senslen;
	char buf[1024];
	const scandata *datptr;
	unsigned long issame;
static		constexpr const char rawin[]{"rawin.dat"};
	public:	

	void firstdata(const std::string_view sensordir,const scandata *datptr) {
		this->datptr=datptr;
		senslen=sensordir.length();
//		buf=new char[senslen+30+EXTRA];
		time_t tim=datptr->gettime();
		memcpy(buf,sensordir.data(),senslen);
		buf[senslen++]='/';
		senslen+=timestr(buf+senslen,tim); 
		mkdir(buf,0700);
		buf[senslen++]='/';
		memcpy(buf+senslen,rawin,sizeof(rawin));
		issame=*reinterpret_cast<unsigned long *>(datptr->data->data());
		writeall(buf,datptr->data->data(),datptr->data->size());
		const char infostr[]{"info.dat"};
		memcpy(buf+senslen,infostr,sizeof(infostr));
		writeall(buf,datptr->info->data(),datptr->info->size());
		}
	void	lastdata() {
		const char raw[]{"raw.dat"};
		if(issame!=*reinterpret_cast<unsigned long *>(datptr->data->data())) {
			memcpy(buf+senslen,raw,sizeof(raw));
			writeall(buf,datptr->data->data(),datptr->data->size());
			}
		else {
			buf[senslen]='\0';
			int dir=open(buf,O_DIRECTORY|O_PATH);
			if(dir>0) {
			      if(renameat(dir, rawin, dir,raw)!=0) { 
					lerror("renameat rawin.dat raw.dat");
				   }
			      close(dir);
			      }
			else
			  lerror(buf);

			}
	 //	   delete[] buf;
		   }
	};
#if defined(SCANLOG) 
inline constexpr const static bool logscan=true;
#else
bool logscan=false;
#endif
#else


class scanlogger {
	public:	
	void firstdata(const std::string_view sensordir,const scandata *datptr) { }
	void	lastdata() { }
	}; 
#define logscan false 
#endif
#ifdef LOGSCANRESULT
static void			logscanresult( const AlgorithmResults *alg) {
	extern			pathconcat logbasedir;
				static pathconcat logfile(logbasedir,"uit.txt");
				if(mkdir(logbasedir.data(),0700)&&errno!=EEXIST) {
					LOGGER("mkdir(%s) failed\n",logbasedir.data());
					}
				else {
					FILE *fp=fopen(logfile.data(),"a");
					alg->showresults(fp,datptr);
					fclose(fp);
					}

				}
#else
#define			logscanresult( alg)  
#endif
pid_t nfcdatatid=0;
thread_local jmp_buf jumpenv;
thread_local bool  jumpenvset=false;
void usr2handler(int get) {
        int tid=syscall(SYS_gettid);
        LOGGER("handler: %d\n",tid);
        if(jumpenvset) {
                signal(get,SIG_IGN);
                longjmp( jumpenv, tid);
                }
        LOGGER("no jump\n");
}

constexpr const int usesig=SIGUSR2;
void alarmhandler(int sig) {
        pid_t tid=syscall(SYS_gettid);
        LOGGER("Alarm %d",tid);
        if(nfcdatatid!=0) {
                signal(SIGALRM,SIG_IGN);
                pid_t grid=syscall(SYS_getpid);
                if(tgkill(grid,nfcdatatid,usesig))
                        lerror("tgkill");
                }
	}

extern void	setstreaming(SensorGlucoseData *hist) ;
extern "C" JNIEXPORT jint JNICALL fromjava(nfcdata)(JNIEnv *env, jclass thiz, jbyteArray uid, jbyteArray info,jbyteArray data) {
	nfcdatatid=syscall(SYS_gettid);
	LOGGER("nfcdata %d\n", nfcdatatid);
      prctl(PR_SET_NAME, "NFC", 0, 0, 0);
      destruct dest([](){nfcdatatid=0;
                signal(SIGALRM,SIG_IGN);
                signal(usesig,SIG_IGN);
      		});
	signal(usesig,usr2handler);
	signal(SIGALRM,alarmhandler);
static	 const int waitsig=60;
	alarm(waitsig);
	if(setjmp(jumpenv)==nfcdatatid)  {
		LOGGER("after jump");
			return 12<<16;
			}
         jumpenvset=true;

	if(abbottinit())
		return 10<<16;
	LOGGER("voor Abbott:Abbott\n");
	Abbott ab(env,sensorbasedir,uid, info);
	LOGGER("Na Abbott:Abbott\n");
	if(ab.error()) {
		LOGGER("Error in Abbott::Abbott\n");
		return 11<<16;
		}
	time_t tim=time(nullptr);
	std::unique_ptr<scandata> unidatptr(new scandata(env,info,data,tim));

	scandata *datptr=unidatptr.get();
/*
	timevalues times= patchtimevalues(datptr->info) ;
	if(times.wear>0) {
		LOGGER("warmup=%d,wear=%d\n",times.warmup,times.wear);
		}
		*/
	scanlogger	logs;
#ifdef SCANLOG
	if(logscan)  {
		logs.firstdata(ab.getsensordir(),datptr);
		}
#endif
	auto [alg,scanda]=ab.ProcessOne(datptr); 
//Abbott::scanresult_t 
//	const AlgorithmResults *alg; const ScanData *scanda;
	LOGGER("after ab.ProcessOne\n");
	if(logscan) {
		logs.lastdata(); 
		}
	LOGGER("after lastdata()\n");
	destruct _back([](){ backup->wakebackup(Backup::wakeall);});
	int ret=2<<16;
	if(alg==Initialized) {
		LOGGER("Initialized\n");
		const int min=datptr->getSensorAgeInMinutes();
		const bool enablestreaming=setbluetoothon||(ab.hist&&!ab.hist->bluetoothOn());
		if(min<60)
			ret=5<<16|(60-min)<<24;
		else
			ret=7<<16;
		if(enablestreaming)
			ret|=(0x80<<16);

		sensor* senso=sensors->getsensor(ab.sensorindex);
		senso->initialized=true;
		}
	else {
	if(alg){
		LOGGER("alg!=null ");
		if(scanda) {
			LOGGER("scanda\n");
			logscanresult(alg);
			int gluval=alg->currentglucose().getValue();
			if(gluval) {
				scantoshow={ab.sensorindex,scanda};
				if(setbluetoothon||!ab.hist->bluetoothOn())  {
					return 8<<16|gluval;
						/*
					if(!waitstreaming()|| ((tim-ab.hist->beginscans()->gettime())>120)) {
						return 8<<16|gluval;
						}
					else   {
						if(!ab.hist->resetdevice) {
							ab.hist->resetdevice=true;
							return 9<<16|gluval;
							}
						} */
					}
				return gluval;
				}
			const int min=datptr->getSensorAgeInMinutes();
			if(min<60) {
				const bool enablestreaming=setbluetoothon||(ab.hist&&!ab.hist->bluetoothOn());
				ret=5<<16|(60-min)<<24;
				if(enablestreaming)
					ret|=(0x80<<16);
				return ret;
				}
			else {
				if(alg->removed) { 	
					sensor* senso=sensors->getsensor(ab.sensorindex);

				       LOGGER("%s was %d,set senso->finished=1;\n",senso->shortsensorname()->data(),senso->finished);

					senso->finished=1;
					setstreaming(ab.hist); 
					setusedsensors();
					if(alg->getlsaDetected()) {
						return SAS_SENSOR_TERMINATED<<16;
						}
					else
						return SAS_SENSOR_REMOVED<<16;
				
					}
				return 6<<16;
				}
			
			}
		ret=4<<16;
		}
	else {
		LOGGER("alg==null ");
		if(datptr->getSensorAgeInMinutes()==0)
			ret=3<<16;
		}
		}
	return ret;
}

//extern SensorGlucoseData  *usedhist;
//SensorGlucoseData  *usedhist;

extern Sensoren *sensors;

//#define fromjava(x) Java_tk_glucodata_Natives_ ##x
//extern data_t * unlockKeySensor(const SensorGlucoseData *usedhist) ;
extern data_t * unlockKeySensor(SensorGlucoseData *usedhist,scanstate *stateptr) ;
 //public static native long getdataptr(String sensorname);
//int  nusensornr=0,maxsens=4;; SensorGlucoseData ** nusensors=new SensorGlucoseData *[maxsens];
extern "C" JNIEXPORT jlong JNICALL   fromjava(getsensorptr)(JNIEnv *env, jclass cl,jlong dataptr) {
	streamdata *sdata=reinterpret_cast<streamdata *>(dataptr);
	if(!sdata)
		return 0LL;
	return reinterpret_cast<jlong>(sdata->hist);
	}
extern "C" JNIEXPORT jlong JNICALL   fromjava(getdataptr)(JNIEnv *env, jclass cl,jstring jsensor) {
	if(!sensors)
		return 0LL;
	constexpr const  int shortsensorlen=11;
	jint getlen= env->GetStringUTFLength( jsensor);
	if(getlen!=shortsensorlen) {
		LOGGER("sensorlen=%d\n",getlen);
		}	
	char sensor[shortsensorlen+1];
	env->GetStringUTFRegion( jsensor, 0,shortsensorlen, sensor);
	sensor[sizeof(sensor)-1]='\0';

	int sensorindex=sensors->sensorindexshort(sensor);
	if(sensorindex<0)
		return 0LL;
	SensorGlucoseData *sens= sensors->gethist(sensorindex);
	sens->sensorerror=false;
	streamdata *data;
	if(sens->isLibre3()) {
		data= new libre3stream(sensorindex,sens);
		}
	else {
		data=new libre2stream(sensorindex,sens);
		}
	if(data->good())
		return reinterpret_cast<jlong>(data);
	LOGGER("getdataptr(): !data->good()\n");
	delete data;
	return 0LL;
	}
extern "C" JNIEXPORT void JNICALL   fromjava(freedataptr)(JNIEnv *envin, jclass cl,jlong dataptr) {
	streamdata *sdata=reinterpret_cast<streamdata *>(dataptr);
	delete sdata;
	}
extern "C" JNIEXPORT jboolean  JNICALL   fromjava(askstreamingEnabled)(JNIEnv *env, jclass cl,jlong dataptr) {
	return reinterpret_cast<streamdata *>(dataptr)->hist ->bluetoothOn()==1; 
	}
extern "C" JNIEXPORT void JNICALL  fromjava(setDeviceAddress)(JNIEnv *env, jclass cl,jlong dataptr,jstring jdeviceAddress ) {
	if(!dataptr)
		return;
	SensorGlucoseData *usedhist=reinterpret_cast<streamdata *>(dataptr)->hist ; 
	if(!usedhist)
		return;
	if(!jdeviceAddress)
		usedhist->deviceaddress()[0]='\0';
	else {
		const jint getlen= std::min(env->GetStringUTFLength( jdeviceAddress),17);
		env->GetStringUTFRegion(jdeviceAddress, 0,getlen,usedhist->deviceaddress());
		usedhist->deviceaddress()[getlen]='\0';
		}
	LOGGER("setDeviceAddress(%s)\n", usedhist->deviceaddress());
        }
extern "C" JNIEXPORT int JNICALL   fromjava(getLibreVersion)(JNIEnv *envin, jclass cl,jlong dataptr) {
	if(!dataptr)
		return 0;
	streamdata *sdata=reinterpret_cast<streamdata *>(dataptr);
	return sdata->libreversion;
	}
extern "C" JNIEXPORT jstring JNICALL   fromjava(getSensorName)(JNIEnv *envin, jclass cl,jlong dataptr) {
	if(!dataptr)
		return nullptr;
	const SensorGlucoseData *usedhist=reinterpret_cast<streamdata *>(dataptr)->hist ; 
	if(!usedhist)
		return nullptr;
	const char *name=usedhist->shortsensorname()->data();
	LOGGER("getSensorName()=%s\n",name);
	return envin->NewStringUTF(name);
	}

extern "C" JNIEXPORT jstring JNICALL   fromjava(getDeviceAddress)(JNIEnv *envin, jclass cl,jlong dataptr) {
	if(!dataptr)
		return nullptr;
	const SensorGlucoseData *usedhist=reinterpret_cast<streamdata *>(dataptr)->hist ; 
	if(!usedhist)
		return nullptr;
	const char *address=usedhist->deviceaddress();
	if(!*address)
		return nullptr;
	LOGGER("getDeviceAddress()=%s\n",address);
	return envin->NewStringUTF(address);
	}
#include "strconcat.h"
extern	strconcat getsensortext(const SensorGlucoseData *hist);
extern "C" JNIEXPORT jstring JNICALL   fromjava(getsensortext)(JNIEnv *envin, jclass cl,jlong dataptr) {
	SensorGlucoseData *usedhist= reinterpret_cast<streamdata *>(dataptr)->hist;
	return envin->NewStringUTF(getsensortext(usedhist).data());
	}




extern "C" JNIEXPORT void JNICALL   fromjava(resetbluetooth)(JNIEnv *envin, jclass cl,jlong dataptr) {
	if(!dataptr)
		return;
	SensorGlucoseData *usedhist= reinterpret_cast<streamdata *>(dataptr)->hist;
	if(!usedhist)
		return;
//	if(!usedhist->bluetoothfirst()||!waitstreaming())
	usedhist->setbluetoothOn(0);
	if(!backup)
		return;
	int maxint=backup->getupdatedata()->sendnr;
	LOGGER("resetbluetooth %d\n",maxint);
	usedhist->sendbluetoothOn(maxint);
	backup->wakebackup(Backup::wakestream);
	}

extern "C" JNIEXPORT jbyteArray JNICALL   fromjava(sensorUnlockKey)(JNIEnv *envin, jclass cl,jlong dataptr) {
	streamdata * const streamptr=reinterpret_cast<streamdata *>(dataptr);
	LOGGER("sensorUnlockKey %p\n",streamptr);
      prctl(PR_SET_NAME,"sensorUnlockKey"  , 0, 0, 0);
	if(!dataptr)
		return nullptr;
	if(streamptr->libreversion!=2)
		return nullptr;
	scanstate state;
	if(data_t * key=unlockKeySensor(streamptr->hist,&state)) {
		const int uitlen= key->size();
		jbyteArray uit=envin->NewByteArray(uitlen);
		envin->SetByteArrayRegion(uit, 0, uitlen, key->data());
//		data_t::deleteex(key);
		return uit;
		}
	return nullptr;
	}
extern "C" JNIEXPORT jbyteArray JNICALL   fromjava(getsensorident)(JNIEnv *envin, jclass cl,jlong dataptr) { 
	streamdata *  streamptr=reinterpret_cast<streamdata *>(dataptr);
	if(!dataptr)
		return nullptr;
	if(streamptr->libreversion!=2)
		return nullptr;
	SensorGlucoseData *usedhist=streamptr->hist ; 
	if(!usedhist)
		return nullptr;
	jbyteArray uit=envin->NewByteArray(8);
	envin->SetByteArrayRegion(uit, 0, 8, usedhist->getsensorident()->data());
	return uit;
	}
extern "C" JNIEXPORT jint JNICALL   fromjava(getsensorgen)(JNIEnv *envin, jclass cl,jlong dataptr) { 
	streamdata * const streamptr=reinterpret_cast<streamdata *>(dataptr);
	if(!dataptr)
		return -1;
	const SensorGlucoseData *usedhist=streamptr->hist ; 
	if(!usedhist)
		return -1;
	return usedhist->getsensorgen();
	}

extern "C" JNIEXPORT jbyteArray JNICALL   fromjava(getstreamingAuthenticationData)(JNIEnv *envin, jclass cl,jlong dataptr) { 
	streamdata * const streamptr=reinterpret_cast<streamdata *>(dataptr);
	LOGGER("getstreamingAuthenticationData %p\n",streamptr);
        prctl(PR_SET_NAME,"getstreamingAuthenticationData"  , 0, 0, 0);
	if(!dataptr)
		return nullptr;
	if(streamptr->libreversion!=2)
		return nullptr;
	const SensorGlucoseData *usedhist=streamptr->hist ; 
	if(!usedhist)
		return nullptr;
	jbyteArray uit=envin->NewByteArray(10);
	envin->SetByteArrayRegion(uit, 0, 10,(const jbyte *)usedhist->getinfo()->getauth() );
	return uit;
	}
/*
extern time_t *glucosetime;
extern float *glucoserate;
//extern  float *glucose;
extern  uint32_t *glucose;
//typedef const char **sensorname;
typedef const sensorname_t *constcharptr_t;
extern constcharptr_t *sensorname;
*/

//extern streams_t laststream;
//extern data_t *fromjbyteArray(JNIEnv *env,jbyteArray jar,jint len=-1) ;
constexpr const jlong isHighest=4LL<<48;
constexpr const jlong isHigh=6LL<<48;
constexpr const jlong isLow=7LL<<48;
constexpr const jlong isAgain=3LL<<48;
constexpr const jlong isLowest=5LL<<48;

#include "gluconfig.h"
extern void setbuffer(char *);

static void savestate(libre2stream *sdata) {
	if(sdata->state->map.size()<4*4096)
		return;
	string_view sensordir=sdata->hist->getsensordir();
	const auto *lastpo=sdata->hist->lastpoll();
	auto name=scanstate::makefilename(sensordir, lastpo->gettime());
	auto *state= sdata->state->map.data();
	writeall(name.data(),state,sdata->state->map.size()*sizeof(state[0]));

	const char *old;
		{
		std::lock_guard<std::mutex> lock(sdata->hist->mutex);

		old=getpreviousstate(sensordir).data();
		scanstate::makelink(name);
		}
	delete[] name.data();
	unlink(old);
	delete[] old;
	}
int getalarmcode(const uint32_t glval,SensorGlucoseData *hist) {
	const uint32_t val=glval*10;
	int res= (glval<glucoselowest?isLowest:(glval>glucosehighest?isHighest:(settings->highAlarm(val)?isHigh:(settings->lowAlarm(val)?isLow:(settings->availableAlarm()&&hist->waiting?isAgain:0)))))>>48;
	hist->waiting=false;
	return res;
	}
float threshold(float drate)  {
	return glnearnull(drate)?0.0f:drate;
	}
extern "C" JNIEXPORT jfloat JNICALL   fromjava(thresholdchange)(JNIEnv *envin, jclass cl,jfloat drate) {
	return threshold(drate);
	}
jlong glucoseback(uint32_t glval,float drate,SensorGlucoseData *hist) {
		hist->setbluetoothOn(1);
		const uint32_t val=glval*10;
		if(!val) 
			return 0LL;
		const jlong rate=roundl(((long double)drate)*1000LL);
		const jlong alarmcode= glval<glucoselowest?isLowest:(glval>glucosehighest?isHighest:(settings->highAlarm(val)?isHigh:(settings->lowAlarm(val)?isLow:(settings->availableAlarm()&&hist->waiting?isAgain:0))));
		const jlong res= (rate&0xFFFF)<<32|alarmcode|glval;
		LOGGER("val=%u, high=%d, low=%d res=%" PRId64 " rate=%" PRId64 "\n",val,settings->highAlarm(val),settings->lowAlarm(val),res,rate);
		hist->waiting=false;
		return res;
		}

extern "C" JNIEXPORT jlong JNICALL   fromjava(processTooth)(JNIEnv *envin, jclass cl,jlong dataptr, jbyteArray bluetoothdata) {
//	setbuffer(mystatus);
      prctl(PR_SET_NAME, "processTooth", 0, 0, 0);
	LOGGER("processTooth %ld\n",syscall(SYS_gettid));
	libre2stream *sdata=reinterpret_cast<libre2stream *>(dataptr);

//	jint index=sdata->index;
	data_t *bluedata=fromjbyteArray(envin,bluetoothdata);
	uint32_t nu=time(NULL);
#ifndef NORAWSTREAM
	write(sdata->blueuit,reinterpret_cast<const char *>(&nu),sizeof(nu));
	write(sdata->blueuit,bluedata->data(),bluedata->size());
#endif
//	scanstate *newstate=new scanstate(sdata->hist->getsensordir(),nu);
	scanstate *newstate=new scanstate(4);
	const AlgorithmResults *algres=sdata->processTooth(bluedata,newstate,nu);
//	delete newstate;
	if(algres==ALGDUP_VALUE ) {
		delete newstate;
		return 1LL;
		}
	if(algres) {
		constexpr unsigned waittime=60*60;
		static time_t savetime=nu+waittime;
	 	sdata->setstate(newstate);
		if(nu>savetime) {
			savestate(sdata);
			savetime=nu+waittime;
			}
		decltype(auto) gluc=algres->currentglucose();
		const uint32_t glval= gluc.getValue();
		const float drate=gluc.rate();
		if(jlong res=glucoseback(glval,drate,sdata->hist) ) {
			sensor *senso=sensors->getsensor(sdata->sensorindex);
			sdata->hist->sensorerror=false;
			senso->finished=0;
			backup->wakebackup(Backup::wakestream);
			return res;
			}
		}
	else
		delete newstate;
	sdata->hist->sensorerror=true;
	return 0LL;
	}


	/*
extern "C" JNIEXPORT void JNICALL   fromjava(backupstream)(JNIEnv *envin, jclass cl,jlong dataptr) {
	streamdata *sdata=reinterpret_cast<streamdata *>(dataptr);
	backup->updatestream(sdata->hist);
	}
	*/
extern "C" JNIEXPORT jstring JNICALL   fromjava(lastsensorname)(JNIEnv *envin, jclass cl) {
	if(const auto *name=sensors->shortsensorname()->data() ) {
		return envin->NewStringUTF(name);
		}
	return nullptr;
	}
extern "C" JNIEXPORT jlong JNICALL   fromjava(laststarttime)(JNIEnv *envin, jclass cl) {
	return sensors->laststarttime();
	}

extern std::vector<int> usedsensors;
extern void setusedsensors() ;
#ifdef LIBRE3
extern "C" JNIEXPORT jobjectArray  JNICALL   fromjava(activeSensors)(JNIEnv *env, jclass cl) {
	setusedsensors();
	const int len= usedsensors.size();
	jobjectArray  sensjar = env->NewObjectArray(len,env->FindClass("java/lang/String"),nullptr);
	
	LOGGER("activeSensors libre3 len=%d\n",len);
	 for(int i=0;i<len;i++) {
	 	 int index=usedsensors[i];
		 const char *name=sensors->shortsensorname(index)->data();
		 LOGGER("%s\n",name);
		 env->SetObjectArrayElement(sensjar,i,env->NewStringUTF(name));
		  }

	return sensjar;
	} 
#else
extern "C" JNIEXPORT jobjectArray  JNICALL   fromjava(activeSensors)(JNIEnv *env, jclass cl) {
	LOGGER("activeSensors oldlibre\n");
	setusedsensors();
	const int len= usedsensors.size();
	const char *names[len];
	int uitlen=0;
	 for(int i=0;i<len;i++) {
	 	 int index=usedsensors[i];
		  const SensorGlucoseData *sens=sensors->gethist(index );
		  if(sens&&!sens->isLibre3())
		  	names[uitlen++]=sensors->shortsensorname(index)->data();
		  }
	jobjectArray  sensjar = env->NewObjectArray(uitlen,env->FindClass("java/lang/String"),nullptr);
	for(int i=0;i<uitlen;i++)
	     env->SetObjectArrayElement(sensjar,i,env->NewStringUTF(names[i]));

	return sensjar;
	} 
#endif

//extern "C" JNIEXPORT void JNICALL   fromjava(saveState)(JNIEnv *envin, jclass cl,jlong dataptr) {

/*
extern "C" JNIEXPORT void JNICALL   fromjava(setwaiting)(JNIEnv *envin, jclass cl,jlong dataptr,jboolean val) {
	streamdata *sdata=reinterpret_cast<streamdata *>(dataptr);
	SensorGlucoseData *usedhist=sdata->hist ;
	usedhist->waiting=val;
	}
extern "C" JNIEXPORT jboolean JNICALL   fromjava(iswaiting)(JNIEnv *envin, jclass cl,jlong dataptr) {
	if(!settings->availableAlarm())
		return false;
	else {
		streamdata *sdata=reinterpret_cast<streamdata *>(dataptr);
		const SensorGlucoseData *usedhist=sdata->hist ; 
		return usedhist->waiting;
		}
	}
	*/

#if defined(__aarch64__)
#define archstring "arm64-v8a"
#elif defined(__x86_64__)
#define archstring "x86_64"
#elif defined(__i386__)
#define archstring "x86"
#else //defined(__arm__)
#define archstring "armeabi-v7a"
#endif


extern "C" JNIEXPORT jstring  JNICALL   fromjava(getLibraryName)(JNIEnv *env, jclass cl) {
	return env->NewStringUTF("lib/" archstring "/libDataProcessing.so");
	}

/*
#include "destruct.h"

extern "C" JNIEXPORT jstring  JNICALL   fromjava(chmod)(JNIEnv *env, jclass cl,jstring filename,jint mode) {
	const char *str = env->GetStringUTFChars( filename, NULL);
	if (str == nullptr) return nullptr; 
	destruct   dest([filename,str,env]() {env->ReleaseStringUTFChars(filename, str);});
	if(chmod(str,mode)!=0) {
		const int errn=errno;
		return env->NewStringUTF(strerror(errn));
		}
	return nullptr;	
	}

extern "C" JNIEXPORT void  JNICALL   fromjava(startbackup)(JNIEnv *env, jclass cl) {
	if(backup ) {
		prctl(PR_SET_NAME, "Backup", 0, 0, 0);
		backup->backupthread();
		}
	}*/

extern "C" JNIEXPORT jboolean  JNICALL   fromjava(sameSensor)(JNIEnv *env, jclass _cl,jlong one,jlong two) {
	if(one==0LL||two==0LL)
		return false;
	streamdata *sone=reinterpret_cast<streamdata *>(one);
	streamdata *stwo=reinterpret_cast<streamdata *>(two);
	if(sone->sensorindex==stwo->sensorindex)
		return true;
	return false;
	}
#ifdef NFCMEM
extern "C" JNIEXPORT jlong  JNICALL   fromjava(nfcptr)(JNIEnv *env, jclass _cl,jbyteArray juid,jbyteArray jinfo) {
	abbottinit();
	return reinterpret_cast<jlong>(new NfcMemory(env,juid,jinfo));
	}
extern "C" JNIEXPORT jint  JNICALL   fromjava(nfcstart)(JNIEnv *_env, jclass _cl,jlong ptr) {
	NfcMemory *mem=reinterpret_cast<NfcMemory *>(ptr);
	return mem->nextspan();
	}
extern "C" JNIEXPORT jint  JNICALL   fromjava(nfcadd)(JNIEnv *env, jclass _cl,jlong ptr,jbyteArray jscanned) {
	NfcMemory *mem=reinterpret_cast<NfcMemory *>(ptr);
	data_t *scanned=fromjbyteArray(env,jscanned);
	LOGGER("nfcadd %p #%d\n",ptr,scanned->size());
        mem->add(scanned);
 	data_t::deleteex(scanned);
	return mem->nextspan();
	};
extern "C" JNIEXPORT jbyteArray  JNICALL   fromjava(nfcgetresults)(JNIEnv *env, jclass _cl,jlong ptr) {
	LOGGER("nfcgetresults\n");
	NfcMemory *mem=reinterpret_cast<NfcMemory *>(ptr);
	int len= mem->len;
	jbyteArray uit=env->NewByteArray(len);
	env->SetByteArrayRegion(uit, 0, len, mem->results->data());
	delete mem;
	return uit;
	}
#endif		
#define abbottdec(x) (*x)
#define abbottcall(x) x
extern "C"  jint JNICALL   abbottdec(P1)(JNIEnv *envin, jobject obj, jint i, jint i2,jbyteArray  bArr,jbyteArray  bArr2) ;
extern "C"  jbyteArray JNICALL abbottdec(P2)(JNIEnv *, jobject, jint, jint, jbyteArray, jbyteArray);
/*
extern "C"  jint JNICALL   fromjava(V1)(JNIEnv *envin, jobject obj, jint i, jint i2,jbyteArray  bArr,jbyteArray  bArr2)  {
	return  abbottcall(P1)(envin, obj, i, i2, bArr, bArr2)  ;
	}
extern "C"  jbyteArray JNICALL fromjava(V2)(JNIEnv *env, jobject obj, jint i1, jint i2, jbyteArray ar1, jbyteArray ar2) {
	return  abbottcall(P2)(env,obj,i1,i2,ar1,ar2);
	}*/
#ifdef  SAVEPS
/*
struct p1struct {
	jint i1; 
	jint i2;
	unsigned char  *ar1;
	unsigned char  *ar2;
	jint res;
	}; */
	/*
static void writearray(JNIEnv *env,jbyteArray  jar,FILE *fp,const char *name) {
	const data_t *ar=fromjbyteArray(env,jar);
	fprintf(fp,"const unsigned char %s[]={",name);
	const unsigned char *dat=reinterpret_cast<const unsigned char*>(ar->data());
	const int len= ar->size();
	for(int i=0;i<len;i++)
		fprintf(fp,"0x%x,",dat[i]);
	fputs("};\n",fp);
	}
	*/
static void writearray(JNIEnv *env,jbyteArray  jar,FILE *fp) {
	fprintf(fp,",{");
	if(jar) {	
		const data_t *ar=fromjbyteArray(env,jar);
		const unsigned char *dat=reinterpret_cast<const unsigned char*>(ar->data());
		const int len= ar->size();
		for(int i=0;i<len;i++)
			fprintf(fp,"0x%x,",dat[i]);
		};
	fprintf(fp,"}");
	}
int p1funcit=0;
FILE *funcfp;
extern "C"   JNIEXPORT jint JNICALL   fromjava(V1)(JNIEnv *env, jobject obj, jint i, jint i2,jbyteArray  bArr,jbyteArray  bArr2) {
	debugclone();
	 abbottinit() ;
	jint res=abbottcall(P1)(env,obj,i,i2,bArr,bArr2);
	fprintf(funcfp,"p1struct p1_%d={%d,%d", p1funcit++,i,i2);
	writearray(env,bArr,funcfp);
	writearray(env,bArr2,funcfp);
	fprintf(funcfp,",%d};\n",res);
	fflush(funcfp);
	return res;
	}
int p2funcit=0;
extern "C"  JNIEXPORT  jbyteArray JNICALL fromjava(V2) (JNIEnv *env, jobject obj, jint i1, jint i2, jbyteArray jar1, jbyteArray jar2) {
	debugclone();
	 abbottinit() ;
	jbyteArray jres=abbottcall(P2)(env, obj,  i1,  i2, jar1,  jar2) ;
	fprintf(funcfp,"p2struct p2_%d={%d,%d",p2funcit++,i1,i2);
	writearray(env,jar1,funcfp);
	writearray(env,jar2,funcfp);
	writearray(env,jres,funcfp);
	fputs("};\n",funcfp);
	fflush(funcfp);
	return jres;
	}
#else

extern "C"  JNIEXPORT  jint JNICALL   fromjava(V1)(JNIEnv *env, jclass obj, jint i, jint i2,jbyteArray  bArr,jbyteArray  bArr2) {
	debugclone();
	 abbottinit() ;
	jint res=abbottcall(P1)(env,obj,i,i2,bArr,bArr2);
	 LOGGER("V1(%i,%i,%p,%p)=%i\n",i,i2,bArr,bArr2,res);
	return res;
	}
int p2funcit=0;
extern "C"  JNIEXPORT  jbyteArray JNICALL fromjava(V2) (JNIEnv *env, jclass obj, jint i1, jint i2, jbyteArray jar1, jbyteArray jar2) {
	debugclone();
	abbottinit() ;
	jbyteArray jres=abbottcall(P2)(env, obj,  i1,  i2, jar1,  jar2) ;
	LOGGER("V2(%d,%d,%p,%p)=%p\n",i1,i2,jar1,jar2,jres);
	return jres;
	}
#endif
/*
bool registernatives(JNIEnv* env) {
	if(!(abbottcall(P2)&&abbottcall(P2)))
		return false;

const char nativesclass[]= "tk/glucodata/Natives";
    jclass c = env->FindClass(nativesclass);
    LOGGER("after FindClass \n");
#ifndef NDEBUG
extern string_view filesdir;
    pathconcat funcfile(filesdir,"funcs.h");
    funcfp=fopen(funcfile.data(),"w");
    if(funcfp==nullptr) {
    	flerror("open %s failed: ",funcfile.data());
    	}
#endif
    if (c == nullptr) {
    	LOGGER("FindClass(%s) failed\n",nativesclass);
    	return false;
	}
         static const JNINativeMethod methods[] = {
            {"P1", "(II[B[B)I", reinterpret_cast<void*>(P1wrap)},
            {"P2", "(II[B[B)[B", reinterpret_cast<void*>(P2wrap)}
    	};
    int rc = env->RegisterNatives(c, methods, sizeof(methods)/sizeof(methods[0]));

   env->DeleteLocalRef(c);
    if (rc != JNI_OK)  {
	    LOGGER("RegisterNatives failed\n");
	    return false;
	    }
	    LOGGER("RegisterNatives  OK\n");
   return true;
  }
static bool *registeredptr=nullptr;
*/
extern "C" JNIEXPORT jboolean  JNICALL   fromjava(abbottinit)(JNIEnv *env, jclass _cl) {
	if(abbottinit()<0)
		return false;
	return P1!=nullptr;
}

int  abbottreinit();
bool resetwrong(); 

static void reinitabbotter(std::promise<bool> * prom) {
 	resetwrong(); 
	prom->set_value(abbottreinit()>=0);
	}
extern bool reinitabbott() ;
bool reinitabbott() {
	LOGGER("abbottreinit\n");
	pid_t pid= syscall(SYS_getpid);
	pid_t tid = syscall(SYS_gettid);
	if(tid==pid) {
		LOGGER("tid=%d mainthread\n",tid);
		std::promise<bool> prom;
		std::future<bool> fut = prom.get_future();
		std::thread thr(reinitabbotter,&prom);
		thr.join();
		return fut.get();
		 }
	else {
		LOGGER("already on thread pid=%d!=tid=%d\n",pid,tid);
 		resetwrong(); 
		return abbottreinit()>=0;
		}
	}
extern "C" JNIEXPORT jboolean  JNICALL   fromjava(abbottreinit)(JNIEnv *env, jclass _cl) {
	return reinitabbott();
	}

void	setstreaming(SensorGlucoseData *hist) {
		LOGGER("setstreaming(SensorGlucoseData)\n");
		if(!backup)
			return;
		int maxint=backup->getupdatedata()->sendnr;
		if(maxint>maxsendtohost) {
			maxint=maxsendtohost;
		 	backup->getupdatedata()->sendnr=maxint;
			}
		if(maxint>0)
			hist->setsendstreaming( maxint) ;
		}
void	sendKAuth(SensorGlucoseData *hist) {
		LOGGER("setsendKauth(SensorGlucoseData)\n");
		if(!backup)
			return;
		int maxint=backup->getupdatedata()->sendnr;
		hist->setsendKAuth( maxint) ;
		}


void	sendstreaming(SensorGlucoseData *hist) {
		setstreaming(hist);
		backup->wakebackup(Backup::wakeall);
		}



extern "C" JNIEXPORT void  JNICALL   fromjava(reenableStreaming)(JNIEnv *env, jclass _cl) {
	setbluetoothon=true;
	}

int lastgen=0;
int getlastGen() {
#ifdef TESTGEN2
		 	lastgen=2;
		 	return 2;
#else
	if(lastgen)
		return lastgen;
extern	void setusedsensors() ;
extern std::vector<int> usedsensors;
	 setusedsensors() ;
	 for(int index:usedsensors) {
		 auto sens=sensors->gethist(index);
		 if(sens->getsensorgen()==2) {
		 	lastgen=2;
		 	return 2;
			}
		 }
	return 0;
#endif
	}


extern	void closedynlib();
extern "C" JNIEXPORT jboolean  JNICALL   fromjava(switchgen2)(JNIEnv *env, jclass cl) {

	if( P1==nullptr) {
		lastgen=2;
		if(abbottinit()<0)
			return false;
		#ifdef CARRY_LIBS
		if( !P1 ) {
			closedynlib();
			if(abbottreinit()>=0&&P1)
				return true;
			return false;
			}
		#else
		return !P1;
		#endif
		}
	return true;
	}

extern "C" JNIEXPORT jboolean  JNICALL   fromjava(hasstreamed)(JNIEnv *env, jclass _cl) {
	if(sensors)
		return sensors->hasstream();
	return false;
	}

extern std::string_view getdeltaname(float rate);
extern "C" JNIEXPORT jstring JNICALL   fromjava(getxDripTrendName)(JNIEnv *envin, jclass cl,jfloat rate) {
	return envin->NewStringUTF(getdeltaname(rate).data());
	}
