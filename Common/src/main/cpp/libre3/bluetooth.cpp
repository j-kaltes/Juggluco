
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


#define UNCAPPED 1
#include <jni.h>
#include <math.h>

#include "sensoren.hpp"
#include "fromjava.h"
#include "logs.hpp"
#include "destruct.hpp"
#include "datbackup.hpp"
#include "hexstr.hpp"
extern Sensoren *sensors;

extern void    sendKAuth(SensorGlucoseData *hist);
extern "C" JNIEXPORT  void JNICALL fromjava(setLibre3kAuth)(JNIEnv *env, jclass thiz, jlong sensorptr,jbyteArray kauthin) {
    SensorGlucoseData *sens=reinterpret_cast<SensorGlucoseData *>(sensorptr);
    if(!sens) {
        LOGAR("setLibre3kAuth sensorptr==null");
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
        LOGAR("getLibre3kAuth sensorptr==null");
        return nullptr;
        }
    if(!sens->getinfo()->haskAuth) {
        LOGAR("getLibre3kAuth !haskAuth");
        return nullptr;
        }
    constexpr const int kauthlen=149;
    jbyteArray uit=env->NewByteArray(kauthlen);
    env->SetByteArrayRegion(uit, 0, kauthlen,(jbyte *)sens->getinfo()->kAuth);
    return uit;
    }
extern "C" JNIEXPORT  jlong JNICALL fromjava(getLibre3SensorptrPD)(JNIEnv *env, jclass thiz, jstring jsensorid,jlong jstarttime,jbyteArray jpin,jstring jaddress) {
    if(!sensors)   {
        LOGAR("getLibre3Sensorptr: sensors=null");
        return 0LL;
        }
    if(!jsensorid) {
        LOGAR("getLibre3Sensorptr: jsensorid=null");
        return 0LL;
        }
    const char *sensorid = env->GetStringUTFChars(jsensorid, NULL);
    if(sensorid == nullptr) {
        LOGAR("getLibre3Sensorptr: env->GetStringUTFChars(jsensorid, NULL)==null");
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
#ifdef UNCAPPED
    auto histval= minptr->uncappedHistoricMgDl;
    if(histval<39) {
        if(histval>0)  
            histval=39;
        }
    else {
        if(histval>501) {
            if(histval<1000)
                histval=501;
            }
        }
#else
    auto histval= minptr->historicalReading;
#endif
    if(!validglucosevalue(histval)) 
        histval=0;
     if(saveLibre3Historyel(sens,minptr->historicalLifeCount, histval))
             sens->consecutivehistorylifecount();

    }


extern jlong glucoseback(uint32_t nu,uint32_t glval,float drate,SensorGlucoseData *hist) ;
static jlong save3current(SensorGlucoseData *sens, const oneminute *minptr,uint32_t now) {
#ifdef UNCAPPED
    auto curval= minptr->uncappedCurrentMgDl;
    if(curval<39) {
        if(curval>0)  
            curval=39;
        }
    else {
        if(curval>501) {
            if(curval<1000)
                curval=501;
            }
        }
#else
    auto curval= minptr->readingMgDl;
#endif
    jlong res=0LL;
    sens->timelastcurrent=now;
    sens->lastlifecount=minptr->lifeCount;

#ifndef UNCAPPED
    if(curval==8231) {
         LOGGER("save3current curval=%d\n",curval);
            if(const ScanData *last=sens->lastValidStream() ) {
                const auto lastmgdL=last->getmgdL();
                if(lastmgdL < 54) {
                    curval=39;
                    }
               else {
                    if(lastmgdL >450) //Probably doesn't happen
                        curval=501;
                   }
                }
            }
#endif
    if(validglucosevalue(curval)) {
        sens->sensorerror=false;
        int16_t rateofchange=minptr->rateOfChange;
        if(rateofchange==-32768) {
            rateofchange=trend2rate(minptr->trend);
            }
        const float rate= rateofchange/100.0f;
        sens->savepollallIDs<60>(now,minptr->lifeCount,curval,minptr->trend,rate);
        res=glucoseback(now,curval,rate,sens);
        sens->consecutivelifecount();
        }
    else {
        sens->savepollallIDsonly<60>(now,minptr->lifeCount,0,0,NAN);
        sens->sensorerror=true;
        sens->sensorErrorTime=now;
        LOGGER("save3current %d invalid value=%d\n",minptr->lifeCount,curval);
        }
    return res;
    }
extern "C" JNIEXPORT jint JNICALL fromjava(getlastLifeCountReceived)(JNIEnv *env, jclass thiz,jlong sensorptr) {
    if(SensorGlucoseData *sens=reinterpret_cast<SensorGlucoseData *>(sensorptr)) {
        LOGGER("getlastLifeCountReceived()=%d\n", sens->getinfo()->lastLifeCountReceived);

        return sens->getinfo()->lastLifeCountReceived+1;
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
        LOGAR("getpin sensorptr==null");
        return nullptr;
        }
    const jbyte *pin=reinterpret_cast<const jbyte *>(&sens->getinfo()->pin);
    constexpr const int pinlen=4; 
    jbyteArray uit=env->NewByteArray(pinlen);
    env->SetByteArrayRegion(uit, 0, pinlen,pin);
    return uit;
    }


extern                void wakewithcurrent();
extern "C" JNIEXPORT  jlong JNICALL fromjava(saveLibre3MinuteL)(JNIEnv *env, jclass thiz, jlong sensorptr,jbyteArray jmindata,jlong msec) {
    SensorGlucoseData *sens=reinterpret_cast<SensorGlucoseData *>(sensorptr);
    if(!sens) {
        LOGAR("saveLibre3Minute sensorptr==null");
        return 0LL;
        }
    
    if(!jmindata)    {
        LOGAR("saveLibre3Minute jmindata==null");
        return 0LL;
        }
   /* NOT needed
    if(sens->processing.test_and_set()) {
        LOGAR("saveLibre3Minute is processing");
        return 0LL;
        }
    destruct _{[sens]{sens->processing.clear();}};
    */
    const jint len = env->GetArrayLength(jmindata);
    if(len!=sizeof(oneminute)) {
        LOGAR("saveLibre3Minute length jmindata =0");
        return 0LL;
        }
    const jbyte *mindata = (jbyte*)env->GetPrimitiveArrayCritical(jmindata, nullptr);
    if(!mindata) {
        LOGAR("saveLibre3Minute mindata =null");
        return 0LL;
        }
    LOGGER("saveLibre3Minute %s\n",hexstr((const uint8_t*)mindata,len).str());
    destruct _dest([env,jmindata,mindata](){env->ReleasePrimitiveArrayCritical(jmindata,const_cast<jbyte*>(mindata), JNI_ABORT);});

    const oneminute *minptr=reinterpret_cast<const oneminute*>(mindata);
    const uint32_t nowsec=msec/1000L;
    jlong res=save3current(sens,minptr,nowsec);
    save3history(sens,minptr);

    backup->wakebackup(wakestream);
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
        LOGAR("saveLibre3fastData sensorptr=null");
        return false;
        }
        
    if(!jfastdata) {
        LOGAR("saveLibre3fastData jfastdata==null");
        return false;
        }
    const jint len = env->GetArrayLength(jfastdata);
    if(len!=sizeof(fastData)) {
        LOGAR("saveLibre3fastData len!=sizeof(fastData)");
        return false;
        }
    const jbyte *fastdata = (jbyte*)env->GetPrimitiveArrayCritical(jfastdata, nullptr);
    if(!fastdata) {
        LOGAR("saveLibre3fastData fastdata=null");
        return false;
        }
    destruct _dest([env,jfastdata,fastdata](){env->ReleasePrimitiveArrayCritical(jfastdata,const_cast<jbyte*>(fastdata), JNI_ABORT);});

    LOGGER("saveLibre3fastData %s\n",hexstr((const uint8_t*)fastdata,len).str());
    const fastData *fastptr=reinterpret_cast<const fastData*>(fastdata);

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
            const auto curval= fastptr->readingMgDl;
            if(validglucosevalue(curval)) {
                const auto wastime=sens->lifeCount2time(lifecount);
                if(wastime<1666476000) {
                    LOGGER("low time %s",ctime(&wastime));
                    return false;
                    }
                if(sens->hasStreamID(lifecount,wastime)) {
                    LOGGER("Have fastdata %d already present\n",lifecount);
                    return false;
                    }

                LOGAR("save fastdata");
                sens->savepollallIDs<60>(wastime,lifecount,curval,0,NAN);
                sens->backstream(lifecount);
                if(lifecount>=(sens->pollcount()-2))
                    backup->wakebackup(wakestream);
                }
            else {
                LOGGER("fastdata invalid lifecount=%d curval=%.1f\n",lifecount,curval/convfactordL);
                }
            sens->fastupdatelifecount(lifecount);
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

    sens->updateHistorylifecount(lastsave);
    if(++lastsave>sens->getScanendhistory())
        sens->setendhistory(lastsave);
//    backup->wakebackup(wakescan);
    return true;
    }

extern "C" JNIEXPORT  jboolean JNICALL fromjava(saveLibre3History)(JNIEnv *env, jclass thiz, jlong sensorptr,jbyteArray jhistory) {
    SensorGlucoseData *sens=reinterpret_cast<SensorGlucoseData *>(sensorptr);
    if(!sens)  {
        LOGAR("saveLibre3History sensorptr=null");
        return false;
        }
    if(!jhistory) {
        LOGAR("saveLibre3History jhistory=null");
        return false;
        }
    const jint len = env->GetArrayLength(jhistory);
    if(len<4) {
        LOGGER("saveLibre3History len<4 %d\n",len);
        return false;
        }
    const jbyte *history = (jbyte*)env->GetPrimitiveArrayCritical(jhistory, nullptr);
    if(!history) {
        LOGAR("saveLibre3History (jbyte*)env->GetPrimitiveArrayCritical(jhistory, nullptr)=null");
        return false;
        }
    destruct _dest([env,jhistory,history](){env->ReleasePrimitiveArrayCritical(jhistory,const_cast<jbyte*>(history), JNI_ABORT);});
    LOGGER("saveLibre3History %s\n",hexstr((const uint8_t*)history,len).str());
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
        LOGAR("libre3processpatchstatus jstatus==null");
        return -1;
        }    
    const jint len = env->GetArrayLength(jstatus);
    if(len!=sizeof(Patchstatus)) {
        LOGGER("libre3processpatchstatus length(jstatus)==%d!=%d\n",len,(int)sizeof(Patchstatus));
        return -1;
        }
        const jbyte *status = (jbyte*)env->GetPrimitiveArrayCritical(jstatus, nullptr);
        if(!status) {
        LOGAR("libre3processpatchstatus status =null");
        return -1;
        }
    destruct _dest([env,jstatus,status](){env->ReleasePrimitiveArrayCritical(jstatus,const_cast<jbyte*>(status), JNI_ABORT);});
    LOGAR("libre3processpatchstatus");
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
        LOGAR("libre3EventLog jlogdata =null");
        return -1;
        }    
        const jbyte *logdata = (jbyte*)env->GetPrimitiveArrayCritical(jlogdata, nullptr);
        if(!logdata) {
        LOGAR("libre3EventLog logdata =null");
        return -1;
        }
    LOGAR("libre3EventLog");
    destruct _dest([env,jlogdata,logdata](){env->ReleasePrimitiveArrayCritical(jlogdata,const_cast<jbyte*>(logdata), JNI_ABORT);});
    
    const jint len = env->GetArrayLength(jlogdata);
    int units=len/7;
    const EventLog *events=reinterpret_cast<const EventLog *>(logdata);
    for(int i=0;i<units;i++) {
        LOGGER("%d: lifecount=%d, errorData=%d,eventData=%d index=%d\n",i,events[i].lifeCount,events[i].errorData,4000+events[i].eventData,events[i].index);
        }
    for(int i=units-1;i>=0;i--) {
        int index=    events[i].index;
            if(index!=0xFF)
                return index;
        }
    return -1;
#else
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
    LOGGER("libre3ControlHistory(%d,%d)\n",arg,from);
    const ControlHistory com(arg,from);    
    return comtojbyteArray(env,com);
    }
extern "C" JNIEXPORT  jbyteArray JNICALL  fromjava(libre3ClinicalControl)(JNIEnv *env, jclass thiz, jint arg,jint from) {
    LOGGER("libre3ClinicalControl(%d,%d)\n",arg,from);
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

#endif
