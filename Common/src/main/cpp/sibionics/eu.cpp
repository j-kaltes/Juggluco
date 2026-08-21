
#ifdef SIBIONICS
#include "config.h"
#ifdef NEWSIBIONICS
#include <string_view>
#include <jni.h>
#include <memory>
#include <inttypes.h>
#include "SensorGlucoseData.hpp"
#include "fromjava.h"
#include "logs.hpp"
//#include "sibionics/AlgorithmContext.hpp"
#include "streamdata.hpp"
#include "datbackup.hpp"
#include "inout.hpp"
#include "EverSense.hpp"

#ifndef NOLOG
extern void logbytes(std::string_view text,const uint8_t *value,int vallen) ;
#else
#define logbytes(text,value, vallen) 
#endif

extern JNIEnv *subenv;

#include "share/hexstr.hpp"


extern "C" JNIEXPORT jint JNICALL   fromjava(getSensorptrSiSubtype)(JNIEnv *env, jclass cl,jlong sensorptr) {
   if(!sensorptr) {
       LOGAR("getSensorptrSiSubtype sensorptr==0");
       return -1;
       }
   return reinterpret_cast<const SensorGlucoseData*>(sensorptr)->siSubtype();
   }
extern "C" JNIEXPORT void JNICALL   fromjava(setSensorptrSiSubtype)(JNIEnv *env, jclass cl,jlong sensorptr,jint type) {
   if(!sensorptr) {
       LOGAR("setSiSubtype sensorptr==0");
       return;
       }
   LOGGER("setSensorptrSiSubtype %d\n",type);
   auto *usedhist=reinterpret_cast<SensorGlucoseData*>(sensorptr);
   usedhist->getinfo()->siType=type;
   if(type!=3) {
        usedhist->getinfo()->reset=false;
        }
   LOGGER("after usedhist->getinfo()->siType %d\n",type);
   sendsiScan(usedhist);
   LOGAR("sendsiScan(usedhist)");
   if(backup)
       backup->wakebackup(wakeall);
   LOGAR("wakebackup(wakeall)");

   }
extern "C" JNIEXPORT jint JNICALL   fromjava(getSiSubtype)(JNIEnv *env, jclass cl,jlong dataptr) {
   if(!dataptr) {
       LOGAR("getSiSubtype dataptr==0");
       return -1;
       }
   const SensorGlucoseData *usedhist=reinterpret_cast<streamdata *>(dataptr)->hist ; 
   if(!usedhist) {
       LOGAR("getSiSubtype usedhist==null");
       return -1;
       }

   if(usedhist==Sensoren::isdeleted)  {
        LOGAR("getSiSubtype isdeleted");
        return -1;
        }
   return  usedhist->siSubtype();
   }

#include <vector>
extern int sitrend2abbott(int sitrend) ;
extern float sitrend2RateOfChange(int sitrend);

extern uint32_t makestarttime(int index,uint32_t eventTime);

extern jlong glucoseback(uint32_t nu,uint32_t glval,float drate,SensorGlucoseData *hist) ;

jlong     SiContext::handleOneGlucose(SensorGlucoseData *sens,int sensorindex,uint32_t nowsecs,int index,int tempin,int current,int reindex,time_t eventTime) {
        if(!sens || !sensors || sensorindex < 0 || sensorindex > sensors->last()) {
            LOGGER("SIprocess invalid state sens=%p sensors=%p sensorindex=%d\n", sens, sensors, sensorindex);
            return 8LL;
            }
        sensor *sensor=sensors->getsensor(sensorindex);
        auto *info=sens->getinfo();
        if(!info) {
            LOGAR("SIprocess getinfo()==null");
            return 8LL;
            }
        double temp=tempin/10.0;
        double value=current/10.0;
        LOGGER("current=%d %.1f mmol/L\n",current,value);
        if(eventTime>nowsecs)
            eventTime=nowsecs;
        int maxid=sens->getSiIndex();
        if(index!=maxid)   {
                if(index<maxid)   {
                   LOGGER("SIprocess index=%d<maxid=%d\n",index,maxid);
                   uint32_t lasttime=sens->getlastpolltime();
                   if(eventTime>lasttime) {
                      int larger=maxid-index; 
                      int add2index=5*((larger+4)/5);
                      sens->setSiAdd2Index(add2index);
                      maxid=index;
                      sens->setSiIndex(index);
                      LOGGER("index=%d setSiAdd2Index(add2index=%d)\n",index,add2index);
                      }
                   else {
                       LOGAR("SIprocess eventTime<=lasttime, return 3");
                       return 3LL;
                       }
                   }
                else {
                   LOGGER("SIprocess index=%d>maxid=%d\n",index,maxid);
                   int maxretry=(index-maxid)<20?2:((index-maxid)<200?5:10);
                   if(sens->retried++<maxretry) {
                      LOGAR("SIprocess retry, return 3");
                      return 3LL;
                      }
                   }
               }
        if(maxid<10) {
               const auto starttime=makestarttime(index,eventTime);
               info->starttime=starttime;
               sensor->starttime=starttime;
               sensors->setindices();
               if(backup)
                   backup->resendResetDevices(&updateone::sendstream);
               }


       double newvalue=value;
       if(algcontext) {
           if(current>1&&value<3000.0&&(newvalue= process2(index,value,temp))>1.8&&(index%5==0)) {
                info->pollinterval=newvalue-value;
                }
           else {
             if(info->pollinterval<40) 
                    newvalue=value+info->pollinterval;
                 }
             }
       else   {
            LOGAR("algcontext==null");
            }
       const int mgdL=std::round(newvalue*convfactordL);
       const int trend2=algcontext?algcontext->ig_trend:0;
       const float change=sitrend2RateOfChange(trend2);
       const int abbottrend=sitrend2abbott(trend2);

       const int totalIndex=sens->siAddedIndex(index);

       LOGGER("SIprocess totalIndex=%d index=%d temp=%f value=%f newvalue=%f trend=%d %d %1.f itime=%" PRId64 " %s" ,totalIndex,index,temp,value,newvalue,trend2,abbottrend,change,static_cast<int64_t>(eventTime),ctime(&eventTime));
      if(newvalue>1.8&&newvalue<30) {
           sens->savestream(eventTime,totalIndex,mgdL,abbottrend,change);
           sens->setSiIndex(index+1);
           sens->retried=0;
           if(!reindex)  {
                uint32_t starttime=info->starttime;
                uint32_t warminutes=info->manualwarmup;
                uint32_t endwarmup=starttime+warminutes*60;
               if((nowsecs-eventTime)< maxbluetoothage) {
                   if(eventTime>endwarmup) {
                             sens->sensorerror=false;
                             if(sensor->finished) {
                                    sensor->finished=0;
                                    LOGGER("SIprocess finished=%d\n", sensor->finished);
                                    if(backup)
                                        backup->resensordata(sensorindex);
                                    }
                             auto res=glucoseback(eventTime,mgdL,change,sens);
                             if(backup)
                                 backup->wakebackup(wakestream);
                             extern void wakewithcurrent();
                             wakewithcurrent();

            #ifdef OLDEVERSENSE
                              sendEverSenseold(sens,5);
            #endif
                               return res;
                               }
                          else {
                                if(!info->redoAll) {
                                    info->warmupstartpos=std::min((int)info->pollcount-1,0);
                                    }
                               
                              }
                      }
                     }
           else {
                 sens->receivehistory=nowsecs;
                 }
           const int last=sens->pollcount()-1;
           if(last<sens->getbroadcastfrom()) sens->setbroadcastfrom(last);
           }
    else {
            if(index==maxid) sens->setSiIndex(maxid+1);
            LOGGER("SIprocess failed: index=%d temp=%f value=%f reindex=%d\n", index, temp, value,reindex);
            if(!reindex&&!(index%5))  {
                  sens->sensorerror=true;
                  sens->sensorErrorTime=nowsecs;
                  return 0LL;
                  }
                }
      return -1LL;
      }


#else
#include <jni.h>
#include "fromjava.h"
extern "C" JNIEXPORT jbyteArray JNICALL   fromjava(getSIActivation)(JNIEnv *env, jclass cl) {
   return nullptr;
   }
extern "C" JNIEXPORT jbyteArray JNICALL   fromjava(getSItimecmd)(JNIEnv *env, jclass cl) {
   return nullptr;
   }
extern "C" JNIEXPORT jbyteArray JNICALL   fromjava(siAuthBytes)(JNIEnv *env, jclass cl,jlong dataptr) {
   return nullptr;
   }
#endif
#endif
