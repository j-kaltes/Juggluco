/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2, Libre 3, Dexcom G7/ONE+ and           */
/*      Sibionics GS1Sb sensors.                                                     */
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
/*      Fri Sep 12 17:59:51 CEST 2025                                                */

#ifdef DEXCOM
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <jni.h>
#include "jniclass.hpp"
#include "fromjava.h"
#include "logs.hpp"
#include "streamdata.hpp"
#include "SensorGlucoseData.hpp"
#include "datbackup.hpp"
#include "hexstr.hpp"
#include "glucose.hpp"
#include "../calibrate/calculate.hpp"
#include "shortfloat.hpp"
extern int rate2changeindex(float rate);

struct indexCmd {
    uint8_t start[3] {(uint8_t)0x01,(uint8_t)0x03,(uint8_t)0x01};
    uint16_t startIndex;

    indexCmd (uint16_t index):startIndex(index) {};
} __attribute__ ((packed)) ;

struct AccuData {
    uint8_t start[2];//= {0x0D,0x43};
    shortfloat mgdLvalue;
    uint16_t min;
    uint8_t two;
    shortfloat trend;
    uint8_t CGMQuality;
    uint8_t rest[3];

public:
    float getTrend() const {
        return trend;
        }
    float mgdL() const {
        return mgdLvalue;
        }
    uint32_t getTime(uint32_t starttime) const {
        return starttime+min*60;
        }
    }  __attribute__ ((packed));
 extern void wakewithcurrent();
/*
float makearrow(const SensorGlucoseData *sens,float mgdL,uint32_t was)  {
    std::vector<double> w {1.0,1.0,0.5};
    std::vector<double> x,y;
    x.push_back(was);
    y.push_back(mgdL);

    const auto stream=sens->getPolldata();
    int el=1;
    for(const ScanData *iter=&stream.back();el<3&&iter>=&stream.begin()[0];--iter,++el) {
            y.push_back(iter->getmgdL());
            x.push_back(iter->gettime());
            }
    if(el>1) {
          return getA(w,x,y,x.size())*50;
        }
    return NAN;
    } */
extern jlong glucoseback(uint32_t nu,uint32_t glval,float drate,SensorGlucoseData *hist);

jlong mkres(SensorGlucoseData *sens,uint32_t timsec,uint32_t eventTime, int min,int mgdL, int abbotttrend, float change){
    if(sens->savestreamonly(eventTime,min,mgdL,abbotttrend, change)) {
            jlong res;
            if((timsec-eventTime)<maxbluetoothage) {
                 sens->sensorerror=false;
                 const int sensorindex=sens->sensorIndex;
                 sensor *sensor=sensors->getsensor(sensorindex);
                 if(sensor->finished) {
                        LOGGER("mkres %s finished was %d becomes 0\n", sens->showsensorname().data(),sensor->finished);
                        sensor->finished=0;
                        backup->resensordata(sensorindex);
                        }
                 res=glucoseback(eventTime,mgdL,change,sens);
                 wakewithcurrent();
                 }
              else {
                sens->receivehistory=timsec;
                res=1LL;
                }
             backup->wakebackup(wakestream);
             return res;
             }
     return 0LL;
    }
extern "C" JNIEXPORT jlong JNICALL   fromjava(accuProcessData)(JNIEnv *env, jclass cl,jlong dataptr,jbyteArray value,jlong mmsec) {
    if(!value) {
        LOGAR("accuProcessData value==null");
          return 1LL;
        }
     accustream *sdata=reinterpret_cast<accustream *>(dataptr);
     SensorGlucoseData *sens=sdata->hist;
      if(!sens) {
          LOGAR("accuProcessData SensorGlucoseData==null");
          return 1LL;
         }
     const auto arlen=env->GetArrayLength(value);
    const uint32_t timsec=mmsec/1000L;
     if(arlen<sizeof(AccuData))  {
        LOGGER("accuProcessData size  value %d < AccuData %d\n",arlen,sizeof(AccuData));
        sens->sensorerror=true;
        sens->sensorErrorTime=timsec;
        return 0LL;
        }
    const CritAr  bluedata(env,value);
    const uint32_t starttime=sens->getinfo()->starttime;
    const AccuData *accu=reinterpret_cast<const AccuData *>(bluedata.data());
    if(accu->start[0]!=0x0D||accu->start[1]!=0x43) {
//        0E C3 FF 07  AF 46  08 02  FF 07 FF 07  EA 5C
//        0E C3 FF 07  FE 01  08 03  FF 07 FF 07 86 E8
        const uint8_t *start=accu->start;
        if(start[0]==0x0E&&start[1]==0xC3) {
            constexpr const auto isFF07{[](const uint8_t *data){
                return  data[0]==0xFF&&data[1]==0x07;
                }};
            if(isFF07(start+2)&&isFF07(start+8)&&isFF07(start+10)) {
                LOGGER("accuProcessData sensor error id=%d\n",accu->min);
                sens->sensorerror=true;
                sens->sensorErrorTime=timsec;
                return 0LL;
               }
            }
        else {
            if (arlen == 15) {
                struct Lowtype {
                    uint8_t reg1[4];
                    uint16_t min;
                    uint8_t reg2[3];
                    uint8_t last[6];

                    uint32_t getTime(uint32_t starttime) const {
                        return starttime + min * 60;
                    }
                } __attribute__ ((packed));
;
                const uint8_t REG1[]{0x0F, 0xE3, 0x02, 0x08};
                const uint8_t REG2[]{0x08, 0x02, 0x40};
                const Lowtype *low = reinterpret_cast<const Lowtype *>(bluedata.data());
                if (!memcmp(low->reg1, REG1, sizeof(REG1)) &&
                    !memcmp(low->reg2, REG2, sizeof(REG2))) {
                    if (const ScanData *last = sens->lastpoll()) {
                        auto min = low->min;
                        int diff = min - last->id;
                        if (diff >= 5 && diff < 30 && last->getmgdL() < 120) {
                            const uint32_t eventTime = low->getTime(starttime);
                            constexpr const uint32_t mgdL = 39;
                            /*
                            if((timsec-eventTime)<maxbluetoothage) {
                                sens->sensorerror=false;
                                } */
                            return mkres(sens, timsec, eventTime, min, mgdL, 0, NAN);
                            }
                        }
                    }
                }
            }
//0F E3 02 08 E5 15 08 02 40 F0 FF 38 00 64 27
//0F E3 02 08 EA 15 08 02 40 BF EF 0E 00 E5 72
       sens->sensorErrorTime=timsec;
        sens->sensorerror=true;
        return 0LL;
        }
    uint32_t eventTime=accu->getTime(starttime);
    if(eventTime>timsec) {
        LOGGER("accuProcessData: ERROR eventtime %u > now %u\n",eventTime,timsec);
        eventTime=timsec;
        sens->getinfo()->starttime=timsec-60*accu->min;
        }
    float mgdLf=accu->mgdL();
    uint32_t mgdL= std::round(mgdLf);
    /*
    if(mgdL<39||mgdL>401) { //Ever used?
        LOGGER("accuProcessData: ERROR min=%d value %d mg/dL %.1f mmol/L\n",accu->min,mgdL,mgdLf/18.0);
        if((timsec-eventTime)<maxbluetoothage) {
            sens->sensorErrorTime=timsec;
            sens->sensorerror=true;
            return 0LL;
            }
        return 1LL;
        } */
   // float change=makearrow(sens, mgdLf,eventTime) ;
    float change=accu->getTrend() ;
    int abbotttrend=rate2changeindex(change);
    #ifndef NOLOG
    time_t tim=eventTime;
    const char *label=abbotttrend<6?GlucoseNow::trendString[abbotttrend]:"Error";
    LOGGER("accuProcessData glucose id=%d %.1f mg/dL %.1f mmol/L rate=%.2f label=%s CGMQuality=%d %s",accu->min, mgdLf, mgdLf/18.0f,change,label,accu->CGMQuality,ctime(&tim));
    #endif
    return mkres(sens,timsec,eventTime,accu-> min, mgdL, abbotttrend, change);
    }
extern "C" JNIEXPORT jbyteArray JNICALL   fromjava(accuAskValues)(JNIEnv *env, jclass cl,jlong dataptr) {
   if(!dataptr) {
       LOGAR("getAccuAskValues dataptr==0");
       return nullptr;
       }
   const SensorGlucoseData *usedhist=reinterpret_cast<streamdata *>(dataptr)->hist ; 
   if(!usedhist) {
       LOGAR("getAccuAskValues usedhist==null");
       return nullptr;
       }
   int last=usedhist->getLastIndex();

   int askindex=last>=0?(last+5):60;
#ifdef ONLY8HOURS
    uint32_t now=time(nullptr);
   uint32_t minpassed=(now-usedhist->getinfo()->starttime)/60;
   uint32_t last8hours=minpassed-8*60;
   if(askindex<last8hours)
        askindex=last8hours;
#endif
   indexCmd cmd(askindex);
   #ifndef NOLOG
   {
   const hexstr cmdstr(reinterpret_cast<const uint8_t*>(&cmd),sizeof(indexCmd));
   LOGGER("accuAskValues index=%d {%s}\n",askindex,cmdstr.str());
   }
   #endif
   constexpr const int len=sizeof(indexCmd);
   jbyteArray uit=env->NewByteArray(len);
   env->SetByteArrayRegion(uit, 0, len,reinterpret_cast<jbyte*>(&cmd));
   return uit;
   }

struct StartBytes {
     uint16_t minback;
     uint8_t same020[3];
     uint16_t unknown;
     } __attribute__ ((packed));

extern "C" JNIEXPORT void JNICALL   fromjava(accuSetStartTime)(JNIEnv *env, jclass cl,jlong dataptr,jbyteArray value) {
    if(!value) {
        LOGAR("accuSetStartTime value==null");
        return;
        }
     accustream *sdata=reinterpret_cast<accustream *>(dataptr);
     SensorGlucoseData *sens=sdata->hist;
     if(!sens) {
          LOGAR("accuSetStartTime SensorGlucoseData==null");
          return;
         }
    if(sens->pollcount()>0) {
        LOGAR("accuSetStartTime pollcount>0");
        return; 
        }
     const auto arlen=env->GetArrayLength(value);
     if(arlen<sizeof(StartBytes))  {
        LOGGER("accuSetStartTime size  value %d < StartBytes %d\n",arlen,sizeof(StartBytes));
        return;
        }
    const CritAr  bluedata(env,value);
    const StartBytes *start=reinterpret_cast<const StartBytes *>(bluedata.data());


    uint32_t now=time(nullptr);
    sens->getinfo()->starttime=now-start->minback*60;
    #ifndef NOLOG
    const time_t starttime=sens->getinfo()->starttime;
    LOGGER("accuSetStartTime minback=%d starttime=%u %s",start->minback,starttime,ctime(&starttime));
    #endif
    }
//mgdL/init.txt:2025-09-03-18:25:32 10386 start onCharacteristicRead 00002aa9-0000-1000-8000-00805f9b34fb 9A 12 00 02 00 52 8B
              //  Natives.saveStarttime(dataptr,value);
#ifdef MAIN
int main(int argc,char **argv) {
    int index=atoi(argv[1]);
    indexCmd cmd(index);
    uint8_t *ptr=(uint8_t*)&cmd;
    for(int i=0;i<sizeof(cmd);++i) {
        printf("%02x ",ptr[i]);
        }
    puts("");
    }
#endif
#endif
