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
#include <math.h>

#include "sensoren.h"
#include "fromjava.h"
#include "logs.h"
#include "destruct.h"
#include "datbackup.h"
#include "hexstr.h"
extern Sensoren *sensors;

extern void	sendKAuth(SensorGlucoseData *hist);
extern "C" JNIEXPORT  void JNICALL fromjava(setLibre3kAuth)(JNIEnv *env, jclass thiz, jlong sensorptr,jbyteArray kauthin) {
	SensorGlucoseData *sens=reinterpret_cast<SensorGlucoseData *>(sensorptr);
	if(!sens) {
		LOGSTRING("setLibre3kAuth sensorptr==null\n");
		return;
		}
	if(kauthin==nullptr) {
		sens->getinfo()->haskAuth=false;
		return;
		}
	constexpr const int kauthlen=149;
	jsize lens=env->GetArrayLength(kauthin);
	if(lens!=kauthlen) {
		LOGGER("setLibre3kAuth length(kauthin)==%d!=149\n",lens);
		return;
		}
	env->GetByteArrayRegion(kauthin, 0, kauthlen,(jbyte *)sens->getinfo()->kAuth);
	sens->getinfo()->haskAuth=true;;
	sendKAuth(sens);
	return;
	}

extern "C" JNIEXPORT  jbyteArray JNICALL fromjava(getLibre3kAuth)(JNIEnv *env, jclass thiz, jlong sensorptr) {
	SensorGlucoseData *sens=reinterpret_cast<SensorGlucoseData *>(sensorptr);
	if(!sens) {
		LOGSTRING("getLibre3kAuth sensorptr==null\n");
		return nullptr;
		}
	if(!sens->getinfo()->haskAuth) {
		LOGSTRING("getLibre3kAuth !haskAuth\n");
		return nullptr;
		}
	int kauthlen=149;
	jbyteArray uit=env->NewByteArray(kauthlen);
	env->SetByteArrayRegion(uit, 0, kauthlen,(jbyte *)sens->getinfo()->kAuth);
	return uit;
	}
extern "C" JNIEXPORT  jlong JNICALL fromjava(getLibre3SensorptrPD)(JNIEnv *env, jclass thiz, jstring jsensorid,jlong jstarttime,jbyteArray jpin,jstring jaddress) {
	if(!sensors)   {
		LOGSTRING("getLibre3Sensorptr: sensors=null\n");
		return 0LL;
		}
	if(!jsensorid) {
		LOGSTRING("getLibre3Sensorptr: jsensorid=null\n");
		return 0LL;
		}
	const char *sensorid = env->GetStringUTFChars(jsensorid, NULL);
	if(sensorid == nullptr) {
		LOGSTRING("getLibre3Sensorptr: env->GetStringUTFChars(jsensorid, NULL)==null\n");
		return 0LL;
		}
	uint32_t pin;
	if(jpin) 
		env->GetByteArrayRegion(jpin, 0, 4,(jbyte*)&pin);
	else pin=0;

	const char *address=jaddress?env->GetStringUTFChars(jaddress, NULL):nullptr;

#ifndef NOLOG
	const time_t tim=jstarttime/1000L;
	LOGGER("getLibre3Sensorptr(%s,%lu,%.24s,%s) %s\n",sensorid,tim,ctime(&tim),pin?hexstr((const uint8_t*)&pin,4).str():"null",address?address:"null");
#endif
	destruct   dest([jsensorid,sensorid,jaddress,address,env]() {
		env->ReleaseStringUTFChars(jsensorid, sensorid);
		if(jaddress) env->ReleaseStringUTFChars(jaddress, address);
			});
	jint len = env->GetStringUTFLength( jsensorid);
	return reinterpret_cast<jlong>(sensors->makelibre3sensor(std::string_view(sensorid,len),jstarttime/1000,pin,address,time(nullptr)));
	}

extern "C" JNIEXPORT  jlong JNICALL fromjava(getLibre3Sensorptr)(JNIEnv *env, jclass thiz, jstring jsensorid,jlong jstarttime) {
	return   fromjava(getLibre3SensorptrPD)(env,thiz,jsensorid,jstarttime,nullptr,nullptr) ;
	}


struct oneminute {
        uint16_t lifeCount;
        uint16_t readingMgDl;
        int16_t rateOfChange;
        uint16_t esaDuration;
        uint16_t projectedGlucose;
        uint16_t historicalLifeCount;
        uint16_t historicalReading;
        uint8_t trend:3;
        uint8_t rest:5;
        uint16_t uncappedCurrentMgDl;
        uint16_t uncappedHistoricMgDl;
        uint16_t temperature;
        uint8_t fastdata[8];
        } __attribute__ ((packed));


static inline float trend2rate(const int trend) {
        float rate;
        if(trend)
                   rate=(trend-3.0f)*1.3f;
         else
                 rate=NAN;
        return rate;
        }

static bool validglucosevalue(uint16_t value) {
	return value>=39&&value<=501;
	}
static bool saveLibre3Historyel(SensorGlucoseData *save,int lifeCount, uint16_t historicMgDl) {
	const int idpos=int(round(lifeCount/(double)save->getmininterval()));
	const uint16_t mgL=10*historicMgDl;
	if(save->savenewhistory(idpos,  lifeCount, mgL)) {
		const int newend=idpos+1;
		if(newend>save->getScanendhistory())
			save->setendhistory(newend);
		save->backhistory(idpos);
		return true;
		}
	return false;
        }
static void save3history(SensorGlucoseData *sens, const oneminute *minptr) {
	auto histval= minptr->historicalReading;
	if(!validglucosevalue(histval)) 
		histval=0;
	 if(saveLibre3Historyel(sens,minptr->historicalLifeCount, histval))
		 	sens->consecutivehistorylifecount();

	}


extern jlong glucoseback(uint32_t glval,float drate,SensorGlucoseData *hist) ;
static jlong save3current(SensorGlucoseData *sens, const oneminute *minptr) {
	auto curval= minptr->readingMgDl;
	jlong res=0LL;
	const	auto now=time(nullptr);
	sens->timelastcurrent=now;
	sens->lastlifecount=minptr->lifeCount;
	if(validglucosevalue(curval)) {
		sens->sensorerror=false;
		int16_t rateofchange=minptr->rateOfChange;
		if(rateofchange==-32768) {
			rateofchange=trend2rate(minptr->trend);
			}
		const float rate= rateofchange/100.0f;
		sens->savepollallIDs(now,minptr->lifeCount,curval,minptr->trend,rate);
		res=glucoseback(curval,rate,sens);
		sens->consecutivelifecount();
		}
	else {
		sens->savepollallIDsonly(now,minptr->lifeCount,0,0,NAN);
/*		if(curval==32800) {
			sens->replacesensor=true;
			}*/
		sens->sensorerror=true;
		LOGGER("save3current %d invalid value=%d\n",minptr->lifeCount,curval);
		}
	return res;
	}
extern "C" JNIEXPORT jint JNICALL fromjava(getlastLifeCountReceived)(JNIEnv *env, jclass thiz,jlong sensorptr) {
	if(SensorGlucoseData *sens=reinterpret_cast<SensorGlucoseData *>(sensorptr)) {
		return sens->getinfo()->lastLifeCountReceived;
		}
	return 0;
	}

extern "C" JNIEXPORT jint JNICALL fromjava(getlastHistoricLifeCountReceived)(JNIEnv *env, jclass thiz,jlong sensorptr) {
	if(SensorGlucoseData *sens=reinterpret_cast<SensorGlucoseData *>(sensorptr)) {
		int lastpos=sens->getinfo()->lastHistoricLifeCountReceivedPos;
		if(!lastpos)
			return 0;
		Glucose *item=sens->getglucose(lastpos);
		return item->id;
		}
	return 0;
	}


extern "C" JNIEXPORT jbyteArray JNICALL fromjava(getpin)(JNIEnv *env, jclass thiz,jlong sensorptr) {
	SensorGlucoseData *sens=reinterpret_cast<SensorGlucoseData *>(sensorptr);
	if(!sens) {
		LOGSTRING("getpin sensorptr==null\n");
		return nullptr;
		}
	const jbyte *pin=reinterpret_cast<const jbyte *>(&sens->getinfo()->pin);
	constexpr const int pinlen=4; 
	jbyteArray uit=env->NewByteArray(pinlen);
	env->SetByteArrayRegion(uit, 0, pinlen,pin);
	return uit;
	}


extern				void wakewithcurrent();
extern "C" JNIEXPORT  jlong JNICALL fromjava(saveLibre3MinuteL)(JNIEnv *env, jclass thiz, jlong sensorptr,jbyteArray jmindata) {
	SensorGlucoseData *sens=reinterpret_cast<SensorGlucoseData *>(sensorptr);
	if(!sens) {
		LOGSTRING("saveLibre3Minute sensorptr==null\n");
		return 0LL;
		}
	if(!jmindata)	{
		LOGSTRING("saveLibre3Minute jmindata==null\n");
		return 0LL;
		}

	const jint len = env->GetArrayLength(jmindata);
	if(len!=sizeof(oneminute)) {
		LOGSTRING("saveLibre3Minute length jmindata =0\n");
		return 0LL;
		}
        jbyte *mindata = (jbyte*)env->GetPrimitiveArrayCritical(jmindata, nullptr);
        if(!mindata) {
		LOGSTRING("saveLibre3Minute mindata =null\n");
		return 0LL;
		}

	LOGSTRING("saveLibre3Minute\n");
	destruct _dest([env,jmindata,mindata](){env->ReleasePrimitiveArrayCritical(jmindata,mindata, 0);});

	const oneminute *minptr=reinterpret_cast<oneminute*>(mindata);
	jlong res=save3current(sens,minptr);
	save3history(sens,minptr);

	backup->wakebackup(Backup::wakestream);
					wakewithcurrent();

	return res;
	}




struct fastData {
	uint16_t lifeCount;
	uint8_t rawData[8];
	uint16_t readingMgDl;
	uint16_t historicMgDl;
	int getHistoricLifeCount() const {
		return round((lifeCount-19.0)/5.0)*5;
		};
	} __attribute__ ((packed));
static_assert(sizeof(fastData)==14);
extern "C" JNIEXPORT  jboolean JNICALL fromjava(saveLibre3fastData)(JNIEnv *env, jclass thiz, jlong sensorptr,jbyteArray jfastdata) {
	SensorGlucoseData *sens=reinterpret_cast<SensorGlucoseData *>(sensorptr);
	if(!sens)  {
		LOGSTRING("saveLibre3fastData sensorptr=null\n");
		return false;
		}
		
	if(!jfastdata) {
		LOGSTRING("saveLibre3fastData jfastdata==null\n");
		return false;
		}
	const jint len = env->GetArrayLength(jfastdata);
	if(len!=sizeof(fastData)) {
		LOGSTRING("saveLibre3fastData len!=sizeof(fastData)\n");
		return false;
		}
        jbyte *fastdata = (jbyte*)env->GetPrimitiveArrayCritical(jfastdata, nullptr);
        if(!fastdata) {
		LOGSTRING("saveLibre3fastData fastdata=null\n");
		return false;
		}
	destruct _dest([env,jfastdata,fastdata](){env->ReleasePrimitiveArrayCritical(jfastdata,fastdata, 0);});

	const fastData *fastptr=reinterpret_cast<fastData*>(fastdata);

	auto histval= fastptr->historicMgDl;
	const auto histcount=fastptr->getHistoricLifeCount();
	const bool validhist=validglucosevalue(histval);

	if(validhist) {
		LOGGER("valid history lifecount=%d value=%d\n",histcount,histval);
		}
	else {
		LOGGER("invalid history lifecount=%d value=%d\n",histcount,histval);
		histval=0;
		}

	 if(histcount>=0&&saveLibre3Historyel(sens,histcount, histval)) {
		sens->consecutivehistorylifecount();
		}
	else {
		LOGGER("histcount=%d\n",histcount);
		}
	const int lifecount=fastptr->lifeCount;
	if(lifecount>=0) {
		if(sens->hasStreamID(lifecount)) {
			LOGGER("Have fastdata %d already present\n",lifecount);
			}
		else {
			const auto curval= fastptr->readingMgDl;
			if(validglucosevalue(curval)) {
				const auto wastime=sens->lifeCount2time(lifecount);
				if(wastime<1666476000) {
					LOGGER("low time %s",ctime(&wastime));
					return false;
					}
				LOGSTRING("save fastdata\n");
				sens->savepollallIDs(wastime,lifecount,curval,0,NAN);
				sens->backstream(lifecount);
				if(lifecount>=(sens->pollcount()-2))
					backup->wakebackup(Backup::wakestream);
				}
			else {
				LOGGER("fastdata invalid lifecount=%d curval=%.1f\n",lifecount,curval/18.0f);
				}
			sens->fastupdatelifecount(lifecount);
			}
		return true;
		}
	LOGGER("lifecount=%d\n",lifecount);
	return false;
	}

struct HistoryData {
	uint16_t lifeCount;
	uint16_t values[];
	};


static bool saveLibre3History(SensorGlucoseData *sens, const jbyte *history,const int len) {
	const HistoryData *historyptr=reinterpret_cast<const HistoryData*>(history);
	const int itemsnr=(len/2)-1;
	const uint16_t *values=historyptr->values;
	const int startlife=historyptr->lifeCount;
	const int mininterval= sens->getmininterval();
	int idpos=int(round(startlife/(double)mininterval));
	int lifecount=startlife;
	sens->backhistory(idpos);
	int lastsave=0;
	LOGGER("saveLibre3History itemsnr=%d\n",itemsnr);
	for(int i=0;i<itemsnr;i++,idpos++,lifecount+=mininterval) {
		const uint16_t val=values[i];
		if(validglucosevalue(val)) {
			const uint16_t mgL=10*val;
			sens->savenewhistory(idpos,  lifecount, mgL);
			lastsave=idpos;
			}
#ifndef NOLOG
		else {
		      LOGGER("invalid pos=%d val=%d\n",idpos,val);
			}
#endif

		
		 }

	sens->updateHistsorylifecount(lastsave);
	if(++lastsave>sens->getScanendhistory())
		sens->setendhistory(lastsave);
//	backup->wakebackup(Backup::wakescan);
	return true;
	}

extern "C" JNIEXPORT  jboolean JNICALL fromjava(saveLibre3History)(JNIEnv *env, jclass thiz, jlong sensorptr,jbyteArray jhistory) {
	SensorGlucoseData *sens=reinterpret_cast<SensorGlucoseData *>(sensorptr);
	if(!sens)  {
		LOGSTRING("saveLibre3History sensorptr=null\n");
		return false;
		}
	if(!jhistory) {
		LOGSTRING("saveLibre3History jhistory=null\n");
		return false;
		}
	const jint len = env->GetArrayLength(jhistory);
	if(len<4) {
		LOGGER("saveLibre3History len<4 %d\n",len);
		return false;
		}
    jbyte *history = (jbyte*)env->GetPrimitiveArrayCritical(jhistory, nullptr);
    if(!history) {
		LOGSTRING("saveLibre3History (jbyte*)env->GetPrimitiveArrayCritical(jhistory, nullptr)=null\n");
		return false;
		}
	destruct _dest([env,jhistory,history](){env->ReleasePrimitiveArrayCritical(jhistory,history, 0);});
	return  saveLibre3History(sens, history,len);
	}

struct Patchstatus  {
	int16_t lifeCount;	
	int16_t errorData;//?
	int16_t eventData;//?
	int8_t index;
	int8_t patchState;
	int16_t currentLifeCount;
	int8_t stackDisconnectReason;
	int8_t appDisconnectReason;
	int totalEvents() const {
		return index+1;
		};
	int getEventData() const{
		return 4000+eventData;
		}

	} __attribute__ ((packed));


extern "C" JNIEXPORT  jint JNICALL  fromjava(libre3processpatchstatus)(JNIEnv *env, jclass thiz, jlong sensorptr,jbyteArray jstatus) {
	if(!jstatus) {
		LOGSTRING("libre3processpatchstatus jstatus==null\n");
		return -1;
		}	
	const jint len = env->GetArrayLength(jstatus);
	if(len!=sizeof(Patchstatus)) {
		LOGGER("libre3processpatchstatus length(jstatus)==%d!=%d\n",len,(int)sizeof(Patchstatus));
		return -1;
		}
        jbyte *status = (jbyte*)env->GetPrimitiveArrayCritical(jstatus, nullptr);
        if(!status) {
		LOGSTRING("libre3processpatchstatus status =null\n");
		return -1;
		}
	destruct _dest([env,jstatus,status](){env->ReleasePrimitiveArrayCritical(jstatus,status, 0);});
	LOGSTRING("libre3processpatchstatus\n");
	const Patchstatus *pstatus=reinterpret_cast<const Patchstatus *>(status);
	LOGGER("patchState=%d, totalEvents=%d, lifeCount=%d, errorData=%d, eventData=%d, index=%d, currentLifeCount=%d, stackDisconnectReason=%d, appDisconnectReason=%d\n", pstatus->patchState, pstatus->totalEvents(), pstatus->lifeCount, pstatus->errorData, pstatus->getEventData(), pstatus->index, pstatus->currentLifeCount, pstatus->stackDisconnectReason, pstatus->appDisconnectReason);
	SensorGlucoseData *sens=reinterpret_cast<SensorGlucoseData *>(sensorptr);
	sens->getinfo()->patchState=pstatus->patchState;
//s/\<\([a-zA-Z]*\)=%d/pstatus->\1/g
	return pstatus->currentLifeCount|pstatus->index<<16;
	}

struct EventLog {
	int16_t lifeCount;	
	int16_t errorData;
	int16_t eventData;
	int8_t index; // index==255 means no data
	} __attribute__ ((packed));

extern "C" JNIEXPORT  int JNICALL  fromjava(libre3EventLog)(JNIEnv *env, jclass thiz, jlong sensorptr,jbyteArray jlogdata) {
#ifndef NOLOG
	if(!jlogdata) {
		LOGSTRING("libre3EventLog jlogdata =null\n");
		return -1;
		}	
        jbyte *logdata = (jbyte*)env->GetPrimitiveArrayCritical(jlogdata, nullptr);
        if(!logdata) {
		LOGSTRING("libre3EventLog logdata =null\n");
		return -1;
		}
	LOGSTRING("libre3EventLog\n");
	destruct _dest([env,jlogdata,logdata](){env->ReleasePrimitiveArrayCritical(jlogdata,logdata, 0);});
	const jint len = env->GetArrayLength(jlogdata);
	int units=len/7;
	const EventLog *events=reinterpret_cast<const EventLog *>(logdata);
	for(int i=0;i<units;i++) {
		LOGGER("%d: lifecount=%d, errorData=%d,eventData=%d index=%d\n",i,events[i].lifeCount,events[i].errorData,4000+events[i].eventData,events[i].index);
		}
	for(int i=units-1;i>=0;i--) {
		int index=	events[i].index;
			if(index!=0xFF)
				return index;
		}
	return -1;

#endif
	}


struct RequestData {
	int8_t kind[2];
	int8_t arg;
	int32_t from;

	} __attribute__ ((packed));

struct ControlHistory:RequestData {
	ControlHistory(int8_t arg,int32_t from): RequestData({{1,0},arg,from}) {}
        };
struct ClinicalControl:RequestData {
	ClinicalControl(int8_t arg,int32_t from): RequestData({{1,1},arg,from}) {}
        };

static jbyteArray comtojbyteArray(JNIEnv *env, const struct RequestData &con) {
	int conlen=sizeof(con);
	jbyteArray uit=env->NewByteArray(conlen);
	env->SetByteArrayRegion(uit, 0, conlen,(jbyte *)&con);
	return uit;
	}
extern "C" JNIEXPORT  jbyteArray JNICALL  fromjava(libre3ControlHistory)(JNIEnv *env, jclass thiz, jint arg,jint from) {
	const ControlHistory com(arg,from);	
	return comtojbyteArray(env,com);
	}
extern "C" JNIEXPORT  jbyteArray JNICALL  fromjava(libre3ClinicalControl)(JNIEnv *env, jclass thiz, jint arg,jint from) {
	const ClinicalControl com(arg,from);	
	return comtojbyteArray(env,com);
	}

extern "C" JNIEXPORT  jbyteArray JNICALL  fromjava(libre3EventLogControl)(JNIEnv *env, jclass thiz, jint arg) {
	constexpr const int comlen=7;
	jbyte command[comlen];
        command[0]=4;
        *reinterpret_cast<int*>(command+1)=arg;
        command[5]=0;
        command[6]=0;
	jbyteArray uit=env->NewByteArray(comlen);
	env->SetByteArrayRegion(uit, 0, comlen,command);
        return uit;
        }
