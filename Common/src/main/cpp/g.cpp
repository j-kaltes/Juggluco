
#define setthreadname(buf) prctl(PR_SET_NAME, buf, 0, 0, 0)
/*#ifndef NOLOG
#define TESTGEN2 1
#endif       */
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
extern "C" {
typedef void (*sighandler_t)(int);

sighandler_t bsd_signal(int signum, sighandler_t handler);
};
#define asignal signal
//#include <sys/syscall.h>
//#undef NOLOG
       #include "logs.h"
       #include "inout.h"
#include "libre2.h"
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
extern jbyteArray getjtoken(JNIEnv *env);
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
                asignal(get,SIG_IGN);
                longjmp( jumpenv, tid);
                }
        LOGSTRING("no jump\n");
}

constexpr const int usesig=SIGUSR2;
void alarmhandler(int sig) {
        pid_t tid=syscall(SYS_gettid);
        LOGGER("Alarm %d\n",tid);
        if(nfcdatatid!=0) {
                asignal(SIGALRM,SIG_IGN);
                pid_t grid=syscall(SYS_getpid);
                if(tgkill(grid,nfcdatatid,usesig))
                        lerror("tgkill");
                }
	}
//s/setthreadname(\([^);
extern void	setstreaming(SensorGlucoseData *hist) ;
extern void wakeaftermin(const int waitmin);
extern "C" JNIEXPORT jint JNICALL fromjava(nfcdata)(JNIEnv *env, jclass thiz, jbyteArray uid, jbyteArray info,jbyteArray data) {
	nfcdatatid=syscall(SYS_gettid);
	LOGGER("nfcdata %d\n", nfcdatatid);
      setthreadname( "NFC");
      destruct dest([](){nfcdatatid=0;
                asignal(SIGALRM,SIG_IGN);
                asignal(usesig,SIG_IGN);
      		});
	asignal(usesig,usr2handler);
	asignal(SIGALRM,alarmhandler);
static	 const int waitsig=60;
	alarm(waitsig);
	if(setjmp(jumpenv)==nfcdatatid)  {
		LOGSTRING("after jump");
			return 12<<16;
			}
         jumpenvset=true;

	if(abbottinit()<0) {
		LOGAR("abbottinit failed");
		return 10<<16;
		}
	LOGSTRING("voor Abbott:Abbott\n");
	Abbott ab(env,sensorbasedir,uid, info);
	LOGSTRING("Na Abbott:Abbott\n");
	if(ab.error()) {
		LOGSTRING("Error in Abbott::Abbott\n");
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
	LOGSTRING("after ab.ProcessOne\n");
	if(logscan) {
		logs.lastdata(); 
		}
	LOGSTRING("after lastdata()\n");
	destruct _back([](){ backup->wakebackup(Backup::wakeall);});
	int ret=2<<16;
	if(alg==Initialized) {
		LOGSTRING("Initialized\n");
		const int min=datptr->getSensorAgeInMinutes();
		const bool enablestreaming=setbluetoothon||(ab.hist&&!ab.hist->streamingIsEnabled());
		if(min<60)
			ret=5<<16|(60-min)<<24;
		else
			ret=7<<16;
		if(enablestreaming)
			ret|=(0x80<<16);

		sensor* senso=sensors->getsensor(ab.sensorindex);
		senso->initialized=true;
		senso->halfdays=29;
		}
	else {
	if(alg){
		destruct _desalg([alg=alg]{delete alg;}); //[alg] also works with g++ 13.1.1 , but not with clang++ version 14.0.7 
		LOGSTRING("alg!=null ");
		if(scanda) {
			LOGSTRING("scanda\n");
			logscanresult(alg);
			int gluval=alg->currentglucose().getValue();
			if(gluval) {
				scantoshow={ab.sensorindex,scanda,static_cast<uint32_t>(tim)};
				wakeaftermin(0);
				if(setbluetoothon||!ab.hist->streamingIsEnabled())  {
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
				sensor* senso=sensors->getsensor(ab.sensorindex);
				if(senso->finished) {
					LOGSTRING("was finished\n");
					setstreaming(ab.hist); //NEEDED/
					setusedsensors(); //NEEDED
					senso->finished=0;
					backup->resensordata(ab.sensorindex);
					return 8<<16|gluval;
					}
				return gluval;
				}
			const int min=datptr->getSensorAgeInMinutes();
			if(min<60) {
				sensor* senso=sensors->getsensor(ab.sensorindex);
				if(senso->finished) {
					LOGSTRING("was finished\n");
					setbluetoothon=true;
					senso->finished=0;
					backup->resensordata(ab.sensorindex);
					}
				senso->initialized=true;
				const bool enablestreaming=setbluetoothon||(ab.hist&&!ab.hist->streamingIsEnabled());
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
		LOGAR("alg==null");
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
	if(!sdata) {
		
		return 0LL;
		}
	return reinterpret_cast<jlong>(sdata->hist);
	}
extern "C" JNIEXPORT jlong JNICALL   fromjava(streamfromSensorptr)(JNIEnv *env, jclass cl,jlong sensorptr,int pos) {
	const auto *sens=reinterpret_cast<const SensorGlucoseData*>(sensorptr); 
	const ScanData *start= sens->beginpolls();
	const int len=sens->pollcount();
	for(int i=pos;i<len;i++) {
		const ScanData *item=start+i;
		if(item->valid()) {
			for(++i;i<len&&!start[i].valid();i++)
				;
			return ((jlong)item->gettime())|(((jlong)item->getmgdL())<<32|((jlong)i)<<48);
			}
		}
	return ((jlong)len)<<48;
	}
extern "C" JNIEXPORT void JNICALL   fromjava(healthConnectReset)(JNIEnv *env, jclass cl) {
	sensors->onallsensors([](SensorGlucoseData *sens) {
			auto *info=sens->getinfo();
			info->healthconnectiter=info->pollstart;
			}) ;

	}
extern "C" JNIEXPORT jint JNICALL   fromjava(healthConnectfromSensorptr)(JNIEnv *env, jclass cl,jlong sensorptr) {
	auto *info=reinterpret_cast<SensorGlucoseData*>(sensorptr)->getinfo();
	int start=info->healthconnectiter;
	if(!start) {
		info->healthconnectiter=start=info->pollstart;
		}
	auto res=start|((int)info->pollcount<<16);
	LOGGER("healthConnectfromSensorptr=%x\n",res);
	return res;
	}
extern "C" JNIEXPORT void JNICALL   fromjava(healthConnectWritten)(JNIEnv *env, jclass cl,jlong sensorptr,jint pos) {
	reinterpret_cast<SensorGlucoseData*>(sensorptr)->getinfo()->healthconnectiter=pos;
	}
extern "C" JNIEXPORT jlong JNICALL   fromjava(getSensorStartmsec)(JNIEnv *env, jclass cl,jlong dataptr) {
	if(!dataptr)
		return 0LL;
	const streamdata *sdata=reinterpret_cast<const streamdata *>(dataptr);
	return sdata->hist->getstarttime()*1000LL; 
	}
extern "C" JNIEXPORT void JNICALL   fromjava(finishSensor)(JNIEnv *env, jclass cl,jlong dataptr) {
	streamdata *sdata=reinterpret_cast<streamdata *>(dataptr);
	if(!sdata)
		return;
	LOGGER("finishSensor %s\n",sdata->hist->showsensorname().data());
	sensors->finishsensor(sdata->sensorindex);
	setstreaming(sdata->hist); 
	setusedsensors();
	backup->wakebackup(Backup::wakeall);
	}
extern bool streamHistory() ;
#ifdef SIBIONICS
extern bool siInit();
#endif

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
	SensorGlucoseData *sens= sensors->getSensorData(sensorindex);
	sens->sensorerror=false;
	streamdata *data;
#ifdef SIBIONICS
	if(sens->isSibionics()) {
      LOGGER("getdataptr(%s) isSibinics\n",sensor);
	   if(!siInit())
	      return 0LL;
         data= new sistream(sensorindex,sens);
      }
  else 
#endif
  {
      if(sens->isLibre3()) {
         LOGGER("getdataptr(%s) Libre3\n",sensor);
         data= new libre3stream(sensorindex,sens);
         }
      else {
         LOGGER("getdataptr(%s) Libre2\n",sensor);
         data=new libre2stream(sensorindex,sens);
         if(streamHistory()) {
            if(!sens->getinfo()->startedwithStreamhistory) {
               sens->getinfo()->startedwithStreamhistory=std::max(sens->getinfo()->endhistory,1);
               }
            }
         }
        }
	if(data->good())
		return reinterpret_cast<jlong>(data);
	LOGSTRING("getdataptr(): !data->good()\n");
	delete data;
	return 0LL;
	}
extern "C" JNIEXPORT void JNICALL   fromjava(freedataptr)(JNIEnv *envin, jclass cl,jlong dataptr) {
	streamdata *sdata=reinterpret_cast<streamdata *>(dataptr);
	delete sdata;
	}
extern "C" JNIEXPORT jboolean  JNICALL   fromjava(askstreamingEnabled)(JNIEnv *env, jclass cl,jlong dataptr) {
	return reinterpret_cast<streamdata *>(dataptr)->hist ->streamingIsEnabled()==1; 
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

   /*
extern "C" JNIEXPORT jstring JNICALL   fromjava(getUsedSensorName)(JNIEnv *envin, jclass cl,jlong dataptr) {
	if(!dataptr)
		return nullptr;
	const SensorGlucoseData *usedhist=reinterpret_cast<streamdata *>(dataptr)->hist ; 
	if(!usedhist)
		return nullptr;
	const char *name=usedhist->shortsensorname()->data();
	LOGGER("getSensorName()=%s\n",name);
	return envin->NewStringUTF(name);
	}
extern "C" JNIEXPORT jstring JNICALL   fromjava(getShowSensorName)(JNIEnv *envin, jclass cl,jlong dataptr) {
	if(!dataptr)
		return nullptr;
	const SensorGlucoseData *usedhist=reinterpret_cast<streamdata *>(dataptr)->hist ; 
	if(!usedhist)
		return nullptr;
	const char *name=usedhist->showsensorname()->data();
	LOGGER("getShowSensorName()=%s\n",name);
	return envin->NewStringUTF(name);
	} */

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
extern	strconcat getsensortext(const int sensorindex,const SensorGlucoseData *hist);
extern "C" JNIEXPORT jstring JNICALL   fromjava(getsensortext)(JNIEnv *envin, jclass cl,jlong dataptr) {
   const streamdata *str=reinterpret_cast<const streamdata *>(dataptr);
	const SensorGlucoseData *usedhist= str->hist;
	return envin->NewStringUTF(getsensortext(str->sensorindex,usedhist).data());
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
      setthreadname("sensorUnlockKey"  );
	if(!dataptr) {
		return nullptr;
		}
	if(streamptr->libreversion!=2) {
		LOGGER("libreversion=%d\n",streamptr->libreversion);
		return nullptr;
		}
	scanstate state;
	if(data_t * key=unlockKeySensor(streamptr->hist,&state)) {
		const int uitlen= key->size();
		jbyteArray uit=envin->NewByteArray(uitlen);
		envin->SetByteArrayRegion(uit, 0, uitlen, key->data());
//		data_t::deleteex(key); //IN map don't delete
		return uit;
		}
	LOGAR("unlockKeySensor==null");
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
        setthreadname("getstreamingAuthenticationData"  );
	if(!dataptr)
		return nullptr;
	if(streamptr->libreversion!=2)
		return nullptr;
	const SensorGlucoseData *usedhist=streamptr->hist ; 
	if(!usedhist)
		return nullptr;
	const auto auth=usedhist->getinfo()->getauth();
	const auto authlen=auth.size();
	jbyteArray uit=envin->NewByteArray(authlen);
	envin->SetByteArrayRegion(uit, 0, authlen,(const jbyte *)auth.data());
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


static jlong getalarmonly(const uint32_t glval,const SensorGlucoseData *hist) {
	const uint32_t val=glval*10;
	auto res=(glval<glucoselowest?isLowest:(glval>glucosehighest?isHighest:(settings->highAlarm(val)?isHigh:(settings->lowAlarm(val)?isLow:(settings->availableAlarm()&&hist->waiting?isAgain:0)))));
	LOGGER("val=%u, high=%d, low=%d res=%" PRId64 "\n",val,settings->highAlarm(val),settings->lowAlarm(val),res);
	return res;
	}
int getalarmcode(const uint32_t glval,SensorGlucoseData *hist) {
	int res= getalarmonly(glval,hist)>>48;
	hist->waiting=false;
	return res;
	}
float threshold(float drate)  {
	return glnearnull(drate)?0.0f:drate;
	}
extern "C" JNIEXPORT jfloat JNICALL   fromjava(thresholdchange)(JNIEnv *envin, jclass cl,jfloat drate) {
	return threshold(drate);
	}
jlong glucoselong(uint32_t glval,float drate,const SensorGlucoseData *hist) {
		if(!glval) 
			return 0LL;
		const jlong rate=roundl(((long double)drate)*1000LL);

//		const jlong alarmcode= glval<glucoselowest?isLowest:(glval>glucosehighest?isHighest:(settings->highAlarm(val)?isHigh:(settings->lowAlarm(val)?isLow:(settings->availableAlarm()&&hist->waiting?isAgain:0))));
		const jlong alarmcode= getalarmonly(glval,hist);
		const jlong res= (rate&0xFFFF)<<32|alarmcode|glval;
		LOGGER("glucoselong=%" PRId64 "\n",res);
		return res;
		}
extern const SensorGlucoseData * getlaststream(const uint32_t);
extern "C" JNIEXPORT jlongArray JNICALL   fromjava(getlastGlucose)(JNIEnv *env, jclass cl) {
	auto nu=time(nullptr);
	if(const auto *hist=getlaststream(nu)) {
		const ScanData *poll=hist->lastpoll();
		if(poll&&poll->valid()) {
			jlong uit[2];
			uit[0]=poll->gettime();
			uit[1]=glucoselong(poll->getmgdL(),poll->getchange(),hist);
			jlongArray juit=env->NewLongArray(2);
			env->SetLongArrayRegion(juit, 0, 2,uit );
			return juit;
			}
		}
	return nullptr;
	}
jlong glucoseback(uint32_t glval,float drate,SensorGlucoseData *hist) {
		if(!glval) return 0LL;
		hist->setbluetoothOn(1);
		auto res= glucoselong(glval,drate,hist);
		hist->waiting=false;
		return res;
		}


		extern void wakeuploader();


extern void wakestreamuploader();
extern void wakelibrecurrent() ;
void wakewithcurrent() {
	wakestreamuploader();
#if !defined(WEAROS) && !defined(TESTMENU)
	if(settings->data()->LibreCurrentOnly) {
		LOGAR("wakelibrecurrent()");
		 wakelibrecurrent() ;
		 }
	else {
		LOGAR("!LibreCurrentOnly");
		}
#endif
	}
extern "C" JNIEXPORT jlong JNICALL   fromjava(processTooth)(JNIEnv *envin, jclass cl,jlong dataptr, jbyteArray bluetoothdata) {
//	setbuffer(mystatus);
      setthreadname( "processTooth");
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
	scanstate *newstate=new scanstate(defaultscanstate);
	const AlgorithmResults *algres=sdata->processTooth(bluedata,newstate,nu);

	data_t::deleteex(bluedata);
//	delete newstate;
	if(algres==ALGDUP_VALUE ) {
		delete newstate;
		return 1LL;
		}
	if(algres) {
		destruct _algdes([algres]{delete algres;});
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
			if(senso->finished) {
				LOGGER("processTooth finished=%d\n", senso->finished);
				senso->finished=0;
				backup->resensordata(sdata->sensorindex);
				}

			backup->wakebackup(Backup::wakestream);
			wakewithcurrent();

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
extern "C" JNIEXPORT jboolean  JNICALL   fromjava(hasSibionics)(JNIEnv *env, jclass cl) {
	setusedsensors();
	const int len= usedsensors.size();
	 for(int i=0;i<len;i++) {
	 	const int index=usedsensors[i];
      if(sensors->isSibionics(index))
            return true;
		  }
   return false;
	} 

#ifdef LIBRE3
extern "C" JNIEXPORT jobjectArray  JNICALL   fromjava(activeSensors)(JNIEnv *env, jclass cl) {
	setusedsensors();
	const int len= usedsensors.size();
	jobjectArray  sensjar = env->NewObjectArray(len,env->FindClass("java/lang/String"),nullptr);
	
	LOGGER("activeSensors libre3 len=%d\n",len);
	 for(int i=0;i<len;i++) {
	 	 int index=usedsensors[i];
		 const char *name=sensors->shortsensorname(index)->data();
//		 const char *name=sensors->showsensorname(index); //gives problems in getdataptr
		 LOGGER("%s\n",name);
		 env->SetObjectArrayElement(sensjar,i,env->NewStringUTF(name));
		  }

	return sensjar;
	} 
#else
extern "C" JNIEXPORT jobjectArray  JNICALL   fromjava(activeSensors)(JNIEnv *env, jclass cl) {
	LOGSTRING("activeSensors oldlibre\n");
	setusedsensors();
	const int len= usedsensors.size();
	const char *names[len];
	int uitlen=0;
	 for(int i=0;i<len;i++) {
	 	 int index=usedsensors[i];
		  const SensorGlucoseData *sens=sensors->getSensorData(index );
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
extern "C" JNIEXPORT jstring  JNICALL   fromjava(getCPUarch)(JNIEnv *env, jclass cl) {
	return env->NewStringUTF(archstring);
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
		setthreadname( "Backup");
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
	LOGSTRING("nfcgetresults\n");
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
extern "C"  jint JNICALL   abbottdec(P1)(JNIEnv *envin, jobject obj, jint i, jint i2,jbyteArray  bArr,jbyteArray  bArr2,jbyteArray token72) ;
extern "C"  jbyteArray JNICALL abbottdec(P2)(JNIEnv *, jobject, jint, jint, jbyteArray, jbyteArray,jbyteArray token72);
/*
extern "C"  jint JNICALL   fromjava(V1)(JNIEnv *envin, jobject obj, jint i, jint i2,jbyteArray  bArr,jbyteArray  bArr2)  {
	return  abbottcall(P1)(envin, obj, i, i2, bArr, bArr2)  ;
	}
extern "C"  jbyteArray JNICALL fromjava(V2)(JNIEnv *env, jobject obj, jint i1, jint i2, jbyteArray ar1, jbyteArray ar2) {
	return  abbottcall(P2)(env,obj,i1,i2,ar1,ar2);
	}*/

int  abbottreinit();
bool resetwrong();
bool setgen2() {
	debugclone();
	if(!P1) {
		resetwrong();
		return abbottreinit()>=0;
	}
	else {
		return abbottinit() >=0;
	}
}
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
		data_t::deleteex(ar);
		};
	fprintf(fp,"}");
	}
int p1funcit=0;
FILE *funcfp;
extern "C"   JNIEXPORT jint JNICALL   fromjava(V1)(JNIEnv *env, jobject obj, jint i, jint i2,jbyteArray  bArr,jbyteArray  bArr2) {
	setgen2();
	jint res=abbottcall(P1)(env,obj,i,i2,bArr,bArr2,getjtoken(env));
	fprintf(funcfp,"p1struct p1_%d={%d,%d", p1funcit++,i,i2);
	writearray(env,bArr,funcfp);
	writearray(env,bArr2,funcfp);
	fprintf(funcfp,",%d};\n",res);
	fflush(funcfp);
	return res;
	}
int p2funcit=0;
extern "C"  JNIEXPORT  jbyteArray JNICALL fromjava(V2) (JNIEnv *env, jobject obj, jint i1, jint i2, jbyteArray jar1, jbyteArray jar2) {
	setgen2();
	jbyteArray jres=abbottcall(P2)(env, obj,  i1,  i2, jar1,  jar2,getjtoken(env)) ;
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
	setgen2();
	jint res=abbottcall(P1)(env,obj,i,i2,bArr,bArr2,getjtoken(env));
	 LOGGER("V1(%i,%i,%p,%p)=%i\n",i,i2,bArr,bArr2,res);
	return res;
	}
int p2funcit=0;
extern "C"  JNIEXPORT  jbyteArray JNICALL fromjava(V2) (JNIEnv *env, jclass obj, jint i1, jint i2, jbyteArray jar1, jbyteArray jar2) {
	setgen2();
	jbyteArray jres=abbottcall(P2)(env, obj,  i1,  i2, jar1,  jar2,getjtoken(env)) ;
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
    LOGSTRING("after FindClass \n");
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
	    LOGSTRING("RegisterNatives failed\n");
	    return false;
	    }
	    LOGSTRING("RegisterNatives  OK\n");
   return true;
  }
static bool *registeredptr=nullptr;
*/
extern "C" JNIEXPORT jboolean  JNICALL   fromjava(abbottinit)(JNIEnv *env, jclass _cl) {
	if(abbottinit(false)<0)
		return false;
	return P1!=nullptr;
}


static void reinitabbotter(std::promise<bool> * prom) {
 	resetwrong(); 
	prom->set_value(abbottreinit()>=0);
	}
extern bool reinitabbott() ;
bool reinitabbott() {
	LOGSTRING("abbottreinit\n");
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
		LOGSTRING("setstreaming(SensorGlucoseData)\n");
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
		LOGSTRING("setsendKauth(SensorGlucoseData)\n");
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

static int lastgen=0;
bool hasGen2=false,hasGen1=false;
/*#ifndef NDEBUG
#define TESTGEN2 1
#endif */
#ifdef TESTGEN2 
int getlastGen() {
extern	void setusedsensors() ;
extern std::vector<int> usedsensors;
	 setusedsensors() ;
	 bool has2=true,has1=true;
	hasGen1=has1;
	hasGen2=has2;
//	lastgen=has2?2:1;
	return has2?2:1;
	}
#else
int getlastGen() {
	if(lastgen) return lastgen;
extern	void setusedsensors() ;
extern std::vector<int> usedsensors;
	 setusedsensors() ;
	 bool has2=false,has1=false;
	 for(int index:usedsensors) {
		 auto sens=sensors->getSensorData(index);
		 if(sens->getsensorgen()==2) {
		 	has2=true;
			}
		else {
			has1=true;
			}
		 }
	hasGen1=has1;
	hasGen2=has2;
//	lastgen=has2?2:1;
//	return lastgen;	
	return has2?2:1;
	}
#endif

extern	void closedynlib();
extern bool switchgen2();
extern oldprocessStream_t  oldprocessStream;

extern bool reinitabbott() ;
bool switchgen2() {
	if( !P1 ) {
		lastgen=2;
		if(oldprocessStream) {
			closedynlib();
			return reinitabbott()&&P1;
			/*
			if(abbottreinit()>=0&&P1)
					return true; */
			return false;
			}
		}
	return true;
	}
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
		return P1;
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
extern "C" JNIEXPORT jint JNICALL   fromjava(getinfogen)(JNIEnv *env, jclass _cl, jbyteArray jinfo) {
	jsize lens=env->GetArrayLength(jinfo);
	char info[lens];
	env->GetByteArrayRegion(jinfo, 0, lens, reinterpret_cast<jbyte *>(info));
	int gen= SensorGlucoseData::getgeneration(info);

	#ifndef NOLOG
	const char label[]="getinfogen ";
	auto labellen=sizeof(label)-1;
	int totlen=labellen+lens*3+3;
	char mess[totlen];
	memcpy(mess,label,labellen);
	int pos=labellen;
	for(int i=0;i<lens;i++) {
		pos+=sprintf(mess+pos,"%02X ",info[i]);
		}
	pos+=sprintf(mess+pos,"%d",gen);
	LOGGERN(mess,pos);
	#endif

	return gen;
	}

