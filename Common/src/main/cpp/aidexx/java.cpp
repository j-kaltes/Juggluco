/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2, Libre 3, Dexcom G7/ONE+,              */
/*      Sibionics GS1Sb and Accu-Chek SmartGuide sensors.                            */
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
/*      Fri Mar 27 17:12:36 CET 2026                                                 */

#ifdef DEXCOM
#include <stdint.h>
#include <jni.h>
#include "logs.hpp"
#include "streamdata.hpp"
#include "fromjava.h"
#include "glucose.h"
#include "jniclass.hpp"
#include "datbackup.hpp"
#include "Showhex.hpp"

//void setSiAdd2Index(int32_t add) {
//int siAddedIndex(int index) const {


extern jlong glucoseback(uint32_t nu,uint32_t glval,float drate,SensorGlucoseData *hist);
extern int rate2changeindex(float rate);

static constexpr int aidexXlowest =   18;
static constexpr int aidexXhighest =  800 ;
static bool validGlucose(int mgdL) {
     if(mgdL<aidexXlowest) {
        LOGGER("validGlucose TOO LOW %d mg/dLn",mgdL)
        return false;
        }
     if(mgdL>aidexXhighest) {
        LOGGER("validGlucose TOO HIGH %d mg/dL\n",mgdL)
        return false;
        }
    return true;
    }
static uint32_t id2time(const SensorGlucoseData *sens,int id) {
    const uint32_t starttime=sens->getinfo()->starttime+sens->addToStartTime();
    return starttime+id*60;
    }

static bool savepast(SensorGlucoseData *sens,int id,int mgdL ) {
    const uint32_t eventtime=id2time(sens,id);
    if(sens->hasStreamID(id,eventtime)) {
        LOGGER("savepast: %d %d mg/dL already present\n",id,mgdL);
        return false;
        }
    sens->savepollallIDsonly<60,false>(eventtime,id,mgdL,0,NAN);
    return true;
    }

static constexpr float AidexTrend2RateOfChange(int trend) {
    return trend*.1f;
    }

static bool savepast(SensorGlucoseData *sens, int id,const struct HistoryGlucose *glu) {
     if(!glu->valid) {
        LOGGER("savepast %d not valid\n",id);
        return false;
        }
     int mgdL=glu->glucose;
    if(!validGlucose(mgdL)) {
        return false;
        }
    return savepast(sens,id,mgdL);
    }
static bool savepast(SensorGlucoseData *sens, int id,const struct NextGlucose *glu) {
    return savepast(sens,id,reinterpret_cast<const HistoryGlucose*>(glu));
    }

 extern void wakewithcurrent();
static jlong saveGlucose(SensorGlucoseData *sens,bool valid,int id,int mgdL,int trend,jlong &nowmmsec) {
 
    if(!sens->getinfo()->aidexXdat.hasTime) {
        LOGAR("saveGlucose !hasTime");
        return 1LL;
        }
    uint32_t now=nowmmsec/1000LL;
    uint32_t eventtime=id2time(sens,id);
    if(eventtime>now) {
        if(eventtime>(now+30)) {
              LOGGER("saveGlucose %d  %u too late, now %u\n",id,eventtime,now)
              return 2LL;
              }
          eventtime=now;
          }
    sens->timelastcurrent=eventtime;
    sens->lastlifecount=id;
    if(valid&&mgdL<1023) {
        nowmmsec=eventtime*1000LL;
        float rate=AidexTrend2RateOfChange(trend);
        int abbotttrend=rate2changeindex(rate);
        sens->savepollallIDs<60,false>(eventtime,id,mgdL,abbotttrend,rate);
        sens->consecutivelifecount();
        LOGGER("saveGlucose %d  %d mg/dL %.1f mmol/L rate=%.2f valid\n",id,mgdL,mgdL/18.0f,rate);
        jlong res;
        if((now-eventtime)<maxbluetoothage) {
             sens->sensorerror=false;
             const int sensorindex=sens->sensorIndex;
             sensor *sensor=sensors->getsensor(sensorindex);
             if(sensor->finished) {
                    LOGGER("saveGlucose %s finished was %d becomes 0\n", sens->showsensorname().data(),sensor->finished);
                    sensor->finished=0;
                    backup->resensordata(sensorindex);
                    }
            uint32_t warminutes=sens->getinfo()->manualwarmup;
            const uint32_t starttime=sens->getinfo()->starttime;
            const uint32_t endwarmup=starttime+warminutes*60;
            if(eventtime<endwarmup) {
                res=2LL;
                }
             else {
               res=glucoseback(eventtime,mgdL,rate,sens);
               wakewithcurrent();
               }
            }
        else {
            res=2LL;
            }
        backup->wakebackup(wakestream);
        LOGGER("saveGlucose res=%lld\n",(long long)res);
        return res;
        }
    LOGGER("saveGlucose %d not valid\n",id);
    sens->savepollallIDsonly<60,false>(eventtime,id,0,0,NAN);
    sens->sensorerror=true;
    sens->sensorErrorTime=eventtime;
    return 0LL;
   }

extern "C" JNIEXPORT jlong JNICALL   fromjava(aidexXscanBytes)(JNIEnv *env, jclass cl,jlong dataptr,jbyteArray value,jlongArray jtimeres) {
    if(!value) {
        LOGAR("aidexXscanBytes value==null");
        return 0LL;
        }
     auto *sdata=reinterpret_cast< aidexXstream *>(dataptr);
     SensorGlucoseData *sens=sdata->hist;
      if(!sens) {
          LOGAR("aidexXscanBytes SensorGlucoseData==null");
          return 0LL;
         }
    const auto arlen=env->GetArrayLength(value);
     if(arlen!=62)  {
            LOGGER("aidexXscanBytes size  value %d != 62\n",arlen);
            return 0LL;
            }
    const CritAr<>  bluedata(env,value);
    #ifndef NOLOG
    Showhex hex(bluedata.data(),arlen);
    LOGGER("aidexXscanBytes %s\n",hex.data);
    #endif
    if(!sens->getinfo()->aidexXdat.hasTime) {
        LOGAR("aidexXscanBytes !hasTime");
        return 2LL;
        }
    const AdvertiseBytes *adv=reinterpret_cast<const AdvertiseBytes*>(bluedata.data());
    LOGAR("before good crc");
    if(!adv->goodcrc()) {
        LOGGER("aidexXscanBytes  ERROR: crc=%x should %x\n",adv->calculateCRC(),adv->crc32);
        return 0LL;
        }
    LOGAR("after good crc");
    #ifndef NOLOG
    const uint32_t start=sens->getRestartTime();
    adv->log(start);
    #endif
    //TODO
    /*
    if(adv->lastglucose.status) {
        LOGGER("aidexXscanBytes status=%d\n",adv->lastglucose.status)
        return 1LL;
        }
    if(adv->lastglucose.minfromstart<60) {
        LOGGER("aidexXscanBytes minfromstart=%d < 60\n",adv->lastglucose.minfromstart)
        sens->getinfo()->aidexXdat.hasTime=false;
        return 2LL;
        }

        */
    const LastPast *cur=&adv->lastglucose;
    const int preid=cur->minfromstart;
   const int id=sens->siAddedIndex(preid); 

   const int count=sens->pollcount();
   if(id<count) {
       LOGGER("aidexXscanBytes id=%d<count=%d\n",id,count);
       return 2LL;
   /*
      const int last=count-1;
      if(id<last) {
        return 2LL;
        }
      ScanData *el=sens->getstream(last);
      if(el->getmgdL()!=glucose)
        return 2LL;
*/
      }




    for(int i=0,iditer=id;i<2&&iditer>0;++i) {
        savepast(sens,--iditer,adv->prev+i);
        }

    CritArSave<jlong> timeres(env,jtimeres);

    sens->getinfo()->lastHistoricLifeCountReceivedPos=id;
    LOGGER("lastHistoricLifeCountReceivedPos=%d\n",id);
    return saveGlucose(sens,cur->valid,id,cur->glucose,cur->trend,*timeres.data());
    } 

extern int aes_cfb128_encrypt(const unsigned char *plaintext, int plaintext_len, const unsigned char key[16], const unsigned char iv[16], unsigned char *ciphertext);
extern int aes_cfb128_decrypt(const unsigned char *ciphertext, int ciphertext_len, const unsigned char key[16], const unsigned char iv[16],        unsigned char *plaintext)    ;
static int decrypt(int type,const SensorGlucoseData *sens,const uint8_t *input,int len, uint8_t *output) {
    const uint8_t *key=sens->getinfo()->aidexXdat.keys[type].key;
    const uint8_t *iv=sens->getinfo()->aidexXdat.iv;
    #ifndef NOLOG
    Showhex keyhex(key,16);
    LOGGER("%p key=%s\n",key,keyhex.data);
    Showhex ivhex(iv,16);
    LOGGER("iv=%s\n",ivhex.data);
    #endif
    int res=aes_cfb128_decrypt(input, len, key, iv, output);             
    #ifndef NOLOG
    Showhex crypt(input,len);
    Showhex plain(output,len);
    LOGGER("decrypt %s => %s\n",crypt.data,plain.data);
    #endif
    return res;
    }

static int encrypt(int type,const SensorGlucoseData *sens,const uint8_t *input,int len, uint8_t *output) {
    const uint8_t *key=sens->getinfo()->aidexXdat.keys[type].key;
    const uint8_t *iv=sens->getinfo()->aidexXdat.iv;
    #ifndef NOLOG
    Showhex keyhex(key,16);
    LOGGER("%p key=%s\n",key,keyhex.data);
    Showhex ivhex(iv,16);
    LOGGER("iv=%s\n",ivhex.data);
    #endif
    int res=aes_cfb128_encrypt(input, len, key, iv, output);             
    #ifndef NOLOG
    Showhex plain(input,len);
    Showhex enc(output,len);
    LOGGER("encrypt %s => %s\n",plain.data,enc.data);
    #endif
    return res;
    }
    /*
int encrypt2(const SensorGlucoseData *sens,const uint8_t *input,int len, uint8_t *output) {
    const uint8_t *key=sens->getinfo()->aidexXdat.key2;
    const uint8_t *iv=sens->getinfo()->aidexXdat.iv;
    return aes_cfb128_encrypt(input, len, key, iv, output);             
    } */

static jbyteArray getEncrypted(JNIEnv *env, const SensorGlucoseData *sens,const uint8_t *data,int len) {
    uint8_t encmd[len];
    constexpr const int keynr=1;
    if(encrypt(keynr,sens,data, len, encmd)!=len)
        return nullptr;
    jbyteArray out = env->NewByteArray(static_cast<jsize>(len));
    env->SetByteArrayRegion(out, 0, static_cast<jsize>(len), reinterpret_cast<const jbyte*>(encmd));
    return out;
    }

static jbyteArray sizeZero(JNIEnv *env) {
    return env->NewByteArray(static_cast<jsize>(0));
    }
static jbyteArray sizeOne(JNIEnv *env,const jbyte val) {
    auto ar=env->NewByteArray(static_cast<jsize>(1));
    env->SetByteArrayRegion(ar, 0, static_cast<jsize>(1), &val);
    return ar;
    }

template <int N> 
static jbyteArray getEncrypted(JNIEnv *env, const SensorGlucoseData *sens,const uint8_t (&data)[N]) {
    return getEncrypted(env, sens,data,N);
    }
static int jniDecrypter(int keynr,JNIEnv *env, SensorGlucoseData *sens,jbyteArray jar,uint8_t *decrypted) {
     const int len=env->GetArrayLength(jar);
     uint8_t *encrypted=(uint8_t*)env->GetPrimitiveArrayCritical(jar, nullptr);
    int res= decrypt(keynr,sens,encrypted,len,decrypted);
    env->ReleasePrimitiveArrayCritical(jar,(jbyte*)encrypted,JNI_ABORT );
    return res;
    }
template <int keynr=1>
class jniDecrypt {
    int len;
    uint8_t *decrypted;
public:
    jniDecrypt()=delete;
    jniDecrypt(jniDecrypt&&)=delete;
    jniDecrypt(jniDecrypt&)=delete;
   jniDecrypt(JNIEnv *env,const SensorGlucoseData *sens,jarray jar): len(env->GetArrayLength(jar)),decrypted(new uint8_t[len]) {
       uint8_t *encrypted=(uint8_t*)env->GetPrimitiveArrayCritical(jar, nullptr);
       len=decrypt(keynr,sens,encrypted,len,decrypted);
       env->ReleasePrimitiveArrayCritical(jar,(jbyte*)encrypted,JNI_ABORT );
      }
   ~jniDecrypt()  {
    delete[] decrypted;
      };
   const uint8_t *data() const  {	
      return decrypted;
      }
    uint8_t *data()   {
        return decrypted;
    }
    int  size() const {
        return len;
    }

   };



extern void addcrc16(uint8_t *data, size_t len);
/*
static auto makecmd(JNIEnv *env, SensorGlucoseData *sens,const uint8_t *data,int len) {
    uint8_t tmp[len+2];
    memcpy(tmp,data,len);
    addcrc16(tmp, len);
    return  getEncrypted(env, sens,tmp,len+2);
   } 
static auto makecmd(JNIEnv *env, SensorGlucoseData *sens,const uint8_t *data,int len) {
    addcrc16(data, len);
    return  getEncrypted(env, sens,tmp,len+2);
   } 
   */
static auto makeByteShort(JNIEnv *env, SensorGlucoseData *sens,uint8_t cmd,uint16_t arg) {
    uint8_t tmp[5];
    tmp[0]=cmd;
    *(uint16_t*)(tmp+1)=arg;
    addcrc16(tmp, 3);
    return  getEncrypted(env, sens,tmp,5);
  }


static auto askHistory(JNIEnv *env, SensorGlucoseData *sens,int startID) {
    int relID=startID-sens->getinfo()->starthistory;
    if(relID<1)
        relID=1;
    LOGGER("askHistory %d -%d = relID=%d\n",startID,sens->getinfo()->starthistory,relID);
    return makeByteShort(env,sens,0x23,relID);
     }

// 22 d0 e5
static auto  askLastID(JNIEnv *env, SensorGlucoseData *sens) { 
   LOGAR("askLastID");
   uint8_t cmd[]{0x22,0xd0,0xe5}; //0x22 crc16
   return getEncrypted(env,sens,cmd,sizeof(cmd));
 }


static jbyteArray  whenPresent( JNIEnv *env, SensorGlucoseData *sens) {
    auto nextid=sens->getinfo()->lastLifeCountReceived+1;
    if(nextid<sens->getinfo()->lastHistoricLifeCountReceivedPos)
        return askHistory(env, sens,nextid);
    LOGGER("whenPresent next to receiv id= %d present on sensor=%d\n",nextid,sens->getinfo()->lastHistoricLifeCountReceivedPos)
    return sizeZero(env);
}

static jbyteArray onceOld(JNIEnv *env, SensorGlucoseData *sens,const uint8_t *data,int len) { 
    if(len<sizeof(LastPast)) {
        LOGGER("oldOnce %d<%d\n",len,sizeof(LastPast));
         return nullptr;
        }
   const LastPast *cur=reinterpret_cast<const LastPast*>(data);
   const auto preminfromstart=cur->minfromstart;
   const int minfromstart=sens->siAddedIndex(preminfromstart); 

   const int count=sens->pollcount();
   LOGGER("onceOld preminfromstart=%d total=%d\n",preminfromstart,minfromstart);
   if(minfromstart>=count) {
       jlong mmsec=time(nullptr)*1000LL;
       jlong res=saveGlucose(sens,cur->valid,minfromstart,cur->glucose,cur->trend,mmsec);
       #ifndef NOLOG
       const uint32_t start=sens->getRestartTime();
       cur->log("onceOld"sv,start);
       #endif
        sens->getinfo()->lastHistoricLifeCountReceivedPos=minfromstart;
        LOGGER("lastHistoricLifeCountReceivedPos=%d\n",minfromstart);
       }
    else {
        if(minfromstart<(count -1)) {
             LOGGER("minfromstart %d< count %d -1. set hasTime=false\n",minfromstart,count);
            sens->getinfo()->aidexXdat.hasTime=false;
            }
        else {
            LOGGER("minfromstart %d< count %d\n",minfromstart,count);
            sens->getinfo()->lastHistoricLifeCountReceivedPos=minfromstart;
            }
        }

   if(!cur->minfromstart) {
        if(sens->getinfo()->aidexXdat.hasTime) {
                return nullptr;
                }
        }
     if(!sens->getinfo()->aidexXdat.hasTime)
        return getEncrypted(env,sens,{0x21,0xb3,0xd5});

//    auto nextid=sens->getinfo()->lastLifeCountReceived+1; 
//    return askHistory(env, sens,nextid);
    return whenPresent(env,sens);

  // auto nextid=sens->getinfo()->lastLifeCountReceived+1;
   //return askHistory(env, sens,nextid);
 //  return askLastID(env,sens);
/*    auto nextid=sens->getinfo()->lastLifeCountReceived; 

    return askHistory(env, sens,nextid); */
  }
static auto askRawHistory(JNIEnv *env, SensorGlucoseData *sens,int start) {
    LOGGER("askRawHistory %d\n",start);
    return makeByteShort(env,sens,0x24,start);
     }

extern uint16_t crc16_ccitt_false(const uint8_t *data, size_t len);


static jbyteArray lastValue(JNIEnv *env, SensorGlucoseData *sens,const uint8_t *data,int len) {
    uint32_t mid=*(uint32_t*)(data+2);
    if(mid!=0x10001) {
        LOGGER("lastvalue mid: %x != 0x10001\n",mid);
        return sizeZero(env);
        }

   auto lastOnSensor=*(uint16_t*)(data+len-4);
   int totalid=sens->siAddedIndex(lastOnSensor); 
   LOGGER("last on sensor %d totalid=%d\n",lastOnSensor,totalid);

    LOGGER("lastHistoricLifeCountReceivedPos=%d\n",totalid);
    sens->getinfo()->lastHistoricLifeCountReceivedPos=totalid; 
    return whenPresent(env,sens);
    }
//21 01 ea 07 02 0f 0c 21 36 04 00 9e 72



struct LocalStartTime {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t min;
    uint8_t seconds;
    int8_t  timeZoneQuarterHours;
    uint8_t dstOffsetQuarterHours;
    uint32_t unixtime() const {
       if(year<2010)
            return 0;
        struct tm tm{
            .tm_sec  = seconds,
            .tm_min  = min,
            .tm_hour = hour,
            .tm_mday = day,
            .tm_mon  = month - 1,
            .tm_year = year - 1900
            };
        const int tz_minutes = timeZoneQuarterHours * 15;
        const int dst_minutes  = dstOffsetQuarterHours * 15;
        int total_offset = tz_minutes + dst_minutes;
        return timegm(&tm) - total_offset * 60;
        }

    void from_t_time(const time_t tim) {
        struct tm lt;
        localtime_r(&tim, &lt);

        year    = lt.tm_year + 1900;
        month   = lt.tm_mon + 1;
        day     = lt.tm_mday;
        hour    = lt.tm_hour;
        min     = lt.tm_min;
        seconds = lt.tm_sec;

        dstOffsetQuarterHours = lt.tm_isdst ? 4 : 0;
        timeZoneQuarterHours  = lt.tm_gmtoff / (60 * 15) - dstOffsetQuarterHours;
       }


    } __attribute__ ((packed)) ;


static uint8_t clearCMD[]{0xf3,0x8C,0x3E};

static jbyteArray receiveLocalStarttime(JNIEnv *env, SensorGlucoseData *sens,aidexXstream *stream,bool unbonded,bool bondednow,const uint8_t *data,int len) {
    if(len<sizeof(LocalStartTime)) {
        LOGGER("receiveLocalStarttime %d < %d",len,sizeof(LocalStartTime));
        return nullptr;
        };
    const LocalStartTime *start=reinterpret_cast<const LocalStartTime*>(data);
//    const auto wasstarttime=sens->getinfo()->starttime;
    const auto newstarttime=start->unixtime();
     if(!newstarttime) {
        stream->time0=1;
        LOGAR("receiveLocalStarttime newstarttime==0");
        return nullptr;
        }
     stream->time0=0;
    int starttime=sens->getRestartTime();
    bool newtime=false;
    if(starttime!=newstarttime) {
         if(!sens->getinfo()->starttime||!sens->pollcount()) {
               newtime=true;
               const int sensorindex=sens->sensorIndex;
               sensors->getsensor(sensorindex)->starttime=sens->getinfo()->starttime=newstarttime;
               sens->getinfo()->aidexXdat.hasTime=true;
               sensors->setindices();
               backup->resendResetDevices(&updateone::sendstream);
               }
        else {
       //     uint32_t oldstart=sens->getinfo()->starttime;
            if(starttime<newstarttime) {
                const uint32_t oldstart=sens->getinfo()->starttime;
                const int diff=newstarttime-oldstart;
                int min=diff/60;
                int addsec=diff-min*60;
                sens->addToStartTime()=addsec;
                sens->setSiAdd2Index(min);
                #ifndef NOLOG
                time_t newtim=newstarttime;
                LOGGER("receiveLocalStarttime: new starttime=%u oldtime=%u diff=%d min=%d addsec=%d %s",newstarttime,oldstart,diff,min,addsec,ctime(&newtim));
                #endif
                 sens->getinfo()->aidexXdat.hasTime=true;
                 newtime=true;
                }
            else {
                if(starttime>newstarttime) {
                    LOGGER("ERROR: receiveLocalStarttime: newstarttime %u < old starttime=%u\n",newstarttime,starttime);
                    return nullptr;
                    }
                
               }
            }
        }
     sens->getinfo()->aidexXdat.hasTime=true;
    #ifndef NOLOG
    time_t tim=
    newstarttime;
    LOGGER("LocalStarttime %4d-%02d-%02d %02d:%02d:%02d UTC%+02.2f dst=%d %s",start->year,start->month,start->day,start->hour,start->min,start->seconds,start->timeZoneQuarterHours/4.0f,start->dstOffsetQuarterHours,ctime(&tim));
//    return getEncrypted(env, sens, {0x31,0x01,0x8a,0x3b});
    #endif

    if(unbonded&&bondednow)  {
        LOGAR("receiveLocalStarttime unbonded&&bondednow return null");
        return nullptr;
        }
//   if(wasstarttime!=newstarttime) return nullptr;
   if(newtime)
        return getEncrypted(env, sens, { 0x35, 0x01, 0x4e, 0xf7 });



//   if(sens->getinfo()->lastHistoricLifeCountReceivedPos) return whenPresent(env,sens);

    return askLastID(env, sens);
    }

static void timebytes(time_t time,uint8_t *buf) {
        LocalStartTime *start=reinterpret_cast<LocalStartTime *>(buf);
        start->from_t_time(time);
        }
static jbyteArray writeTime(JNIEnv *env, SensorGlucoseData *sens) {
    constexpr const int cmdlen=sizeof(LocalStartTime)+3; 
    alignas(4) uint8_t buf[cmdlen+3];
    auto* cmd=buf+3;
    timebytes(time(nullptr),cmd+1);
    cmd[0]=0x20;
    addcrc16(cmd, cmdlen-2);
    return  getEncrypted(env, sens,cmd,cmdlen);
    }


static jbyteArray getPastValues(JNIEnv *env, SensorGlucoseData *sens,const uint8_t *data,int len) {
   const int startID=*(uint16_t*)(data+2); 
   const struct HistoryGlucose *histglus=reinterpret_cast<const HistoryGlucose*>(data+4);
   const int numhist=(len-6)/2;
   int curID=sens->siAddedIndex(startID);

   const int lastreceived=sens->getinfo()->lastLifeCountReceived; 
   if(curID<=lastreceived) {
#ifndef NOLOG
       LOGGER("getPastValues startID=%d curID=%d <= lastreceived=%d set hasTime=false\n",startID,curID,lastreceived);
       auto newel=histglus[0];
       const ScanData *savedel=sens->getstream(curID);
       auto oldglucose=savedel->getmgdL();
       auto newglucose=newel.glucose;
       if(newglucose!=oldglucose)  {
           LOGGER("getPastValues saved %d mg/dL %.1f mmol/L new %d mg/dL %.1f mmol/L\n", oldglucose, oldglucose/18.0 ,newglucose, newglucose/18.0);
           }
       // sens->getinfo()->aidexXdat.hasTime=false;
#endif
        if(startID) {
           return  sizeZero(env);
           }
        else {
            sens->getinfo()->aidexXdat.hasTime=false;
            return nullptr;
            }
       }

   int firstnew=-1;
   for(int it=0;it<numhist;++it,++curID) {
        auto el=histglus[it];
        if(!el.valid)  {
            LOGGER("getPastValues id=%d invalid\n",curID);
            continue;
            }
         auto glu=el.glucose;
         if(glu<aidexXlowest) {
             LOGGER("getPastValues id=%d too low %d %.1f\n",curID,glu,glu/18.0);
             continue;
            }
         if(glu>aidexXhighest) {
             LOGGER("getPastValues id=%d too high %d %.1f\n",curID,glu,glu/18.0);
             continue;
            }
        bool res=savepast(sens,curID,glu);
        if(firstnew<0&&res)
            firstnew=curID;
         }
  LOGGER("getPastValues startID=%d lastLifeCountReceived=%d\n",startID,curID-1);
  sens->getinfo()->lastLifeCountReceived=curID-1; 
  if(firstnew>=0) {
      sens->receivehistory=time(nullptr);
      sens->backstream(firstnew);
      backup->wakebackup(wakestream);
      }
   if(curID<sens->getinfo()->lastHistoricLifeCountReceivedPos)
       return askHistory(env, sens,curID);
   return  sizeZero(env);
  }
static auto getRawPastValues(JNIEnv *env, SensorGlucoseData *sens,const uint8_t *data,int len) {
   return  sizeZero(env);
  
//    return askRawHistory(env, sens,next);
  }
//10 01 00 00 01 08 02 00 0F 00 47 58 2D 30 31 53 00 00 00 0
/// deviceVersion:1.8.2.0, deviceModel:0, deviceLife:15


extern "C" JNIEXPORT void JNICALL fromjava(aidexXunpair)(JNIEnv* env, jclass,jlong dataptr) {
    auto* sdata = reinterpret_cast<aidexXstream*>(dataptr);
    sdata->unpair=1;
    }
extern "C" JNIEXPORT void JNICALL fromjava(aidexXclear)(JNIEnv* env, jclass,jlong dataptr) {
    auto* sdata = reinterpret_cast<aidexXstream*>(dataptr);
    sdata->clear=1;
    }
/*
extern "C" JNIEXPORT jbyteArray JNICALL fromjava( aidexXunpairCommand)(JNIEnv* env, jclass,jlong dataptr) {
    auto *sdata=reinterpret_cast< aidexXstream *>(dataptr);
    SensorGlucoseData *sens=sdata->hist;
    if(!sens) {
        LOGAR("aidexXprocessHistoryData SensorGlucoseData==null");
        return nullptr;
    }
    LOGAR("unpair command");
    return getEncrypted(env, sens,{0xf2,0xad,0x2e});
    }
*/
 static   jbyteArray  unPairingSuccessful(JNIEnv *env) {
    LOGAR("unpairing Successful");
    return sizeOne(env,1);
    }


void clearKey(SensorGlucoseData* sens) {
      memset(sens->getinfo()->aidexXdat.keys,'\0',16);
    }
 static   jbyteArray  clearSuccessful(JNIEnv *env,SensorGlucoseData *sens) {
    LOGAR("clear successful set hasTime=false");
    clearKey(sens);
    sens->getinfo()->aidexXdat.hasTime=false;
    return sizeOne(env,2);
    }
 static   jbyteArray  clearFailure(JNIEnv *env) {
    LOGAR("clear failed");
    return sizeOne(env,3);
    }
/*
static    jbyteArray unPairingUnSuccessful(JNIEnv* env,jlong dataptr) {
    LOGAR("unpairing failed");
    auto *sdata=reinterpret_cast< aidexXstream *>(dataptr);
    if(sdata->unpairtry<3) {
        ++sdata->unpairtry;
        sleep(1);
        return fromjava( aidexXunpairCommand)( env, nullptr,dataptr);
        }
     sdata->unpairtry=0;
     return sizeOne(env,0);
    }
*/
 static   jbyteArray  unPairingUnSuccessful(JNIEnv *env) {
    LOGAR("unpairing Unsuccessful");
    return sizeOne(env,0);
    }
static jbyteArray deviceInfo(JNIEnv *env, SensorGlucoseData *sens,aidexXstream *stream,const uint8_t *data,int len) {
    const struct  {
        uint8_t reg10;
        uint8_t reg01;
        uint16_t ofDeviceModel1;
        uint8_t version0;
        uint8_t version1;
        uint8_t version2;
        uint8_t version3;
        uint8_t days;
        uint8_t ofDeviceModel2;
        char name[10];
        void log() const {
            LOGGER("deviceVersion %d.%d.%d.%d deviceModel:%d or %d deviceLife:%d %s\n",version0,version1,version2,version3,ofDeviceModel1,ofDeviceModel2,days,name);
            };
    } *info=reinterpret_cast<decltype(info)>(data);
    info->log();
    switch(stream->time0) {
        case 1:
            return getEncrypted(env,sens,{0x31,0x01,0x8a,0x3b});
         case 0: 
            if(sens->getinfo()->aidexXdat.hasTime&&!stream->unbonded)
                return getEncrypted(env, sens, { 0x35, 0x01, 0x4e, 0xf7 });
        };
    return getEncrypted(env,sens,{0x21,0xb3,0xd5});
//        return getEncrypted(env,sens,{0x21,0xb3,0xd5});
        
//    static constexpr uint8_t cmd[] {0x31,0x01,0x8a,0x3b};

//    return getEncrypted(env, sens, { 0x35, 0x01, 0x4e, 0xf7 });
    }
extern "C" JNIEXPORT jbyteArray  JNICALL   fromjava(aidexXprocessHistoryData)(JNIEnv *env, jclass cl,jlong dataptr,jboolean bondednow,jbyteArray value) {

    if(!value) {
        LOGAR("aidexXprocessHistoryData value==null");
        return nullptr;
        }
     auto *sdata=reinterpret_cast< aidexXstream *>(dataptr);
     SensorGlucoseData *sens=sdata->hist;
      if(!sens) {
          LOGAR("aidexXprocessHistoryData SensorGlucoseData==null");
          return nullptr;
         }
    jniDecrypt<1> decrypted(env,sens,value);
    const int arlen=decrypted.size();
     if(arlen<2)  {
            LOGGER("aidexXprocessHistoryData size  value %d < 2\n",arlen);
            return nullptr;
            }
    const uint8_t *deinput=decrypted.data();
    #ifndef NOLOG
    Showhex hex(deinput,arlen);
   LOGGER("aidexXprocessHistoryData ptr=%p %s length=%d\n",sens,hex.data,arlen);
   #endif
    const auto crc=crc16_ccitt_false(deinput,  arlen-2);
    const auto iscrc=*(uint16_t*)(deinput+arlen-2);
    if(crc!=iscrc) {
        LOGGER("aidexXprocessHistoryData wrong %x!=%x\n",crc,iscrc);
        return nullptr;
        }
    uint16_t short1=*((const uint16_t*)deinput);
    switch(short1) {
    //    case 0x101: break;//Glucose value ???
        case 0x110: return deviceInfo(env,sens,sdata,deinput,arlen);
        case 0x111:  {
            return onceOld(env,sens,deinput+2,arlen-2);
            }
        case 0x020: ;
        case 0x120: 
//            return  getEncrypted(env, sens,{0x11,0xe0,0xe3});
              sdata->time0=2;
              return getEncrypted(env,sens,{0x21,0xb3,0xd5});

        //return getEncrypted(env, sens, { 0x35, 0x01, 0x4e, 0xf7 });
        case 0x121:  {
                auto res=receiveLocalStarttime(env,sens,sdata,sdata->unbonded,bondednow,deinput+2,arlen-2);
                 return res;
                }
        case 0x122: {
            return lastValue(env,sens,deinput,arlen);
            }
        case 0x123: return getPastValues(env,sens,deinput,arlen);
        case 0x124: return getRawPastValues(env,sens,deinput,arlen);
        case 0x131: return writeTime(env,sens);
        case 0x134: 
            return  getEncrypted(env, sens,{0x11,0xe0,0xe3});
        case 0x135: 
            return  getEncrypted(env, sens,{0x34,0x01,0x7f,0xc4});
        case 0x1f2: 
            sdata->unpair=0;
            return unPairingSuccessful(env);
        case 0x0f2:  
            if(sdata->unpair==5)
                sdata->unpair=0;
            else
                ++sdata->unpair;

        return unPairingUnSuccessful(env);
        case 0x1f3: 
            sdata->clear=0;
            return clearSuccessful(env,sens);
        case 0x0f3:  
            if(sdata->clear==5)
                sdata->clear=0;
            else
                ++sdata->clear;
            return clearFailure(env);
        default: LOGGER("aidexXprocessHistoryData unknown start=%hx\n",short1)
        } 
      return sizeZero(env);

      }
static  jlong saveCurrent(SensorGlucoseData *sens,const uint8_t *data,int len,jlong &time) {
   if(len!=15) {
        LOGGER("saveCurrent len=%d!=15\n",len);
        return 2LL;
        }
   const CurrentGlucose *cur=reinterpret_cast<const CurrentGlucose*>(data);

   const int totalMinfromStart=sens->siAddedIndex(cur->minfromstart); 
   jlong res=saveGlucose(sens,cur->valid,totalMinfromStart,cur->glucose,cur->trend,time);
   if(data[0]==3) {
        LOGAR("sensor ended");
        return 1LL;
        }
   return res;
 }


extern "C" JNIEXPORT jlong  JNICALL   fromjava(aidexXprocessCurrentData)(JNIEnv *env, jclass cl,jlong dataptr,jbyteArray value,jlongArray jtimeres) {

    if(!value) {
        LOGAR("aidexXprocessCurrentData value==null");
        return 1LL;
        }
     auto *sdata=reinterpret_cast< aidexXstream *>(dataptr);
     SensorGlucoseData *sens=sdata->hist;
      if(!sens) {
          LOGAR("aidexXprocessCurrentData SensorGlucoseData==null");
          return 1LL;
         }
    jniDecrypt<1> decrypted(env,sens,value);
    const int arlen=decrypted.size();
    if(arlen<2) {
            LOGGER("aidexXprocessCurrentData size  value %d < 2\n",arlen);
            return 1LL;
            }
    const uint8_t *data=decrypted.data();
    const auto crc=crc16_ccitt_false(data,  arlen-2);
    const auto iscrc=*(uint16_t*)(data+arlen-2);
    if(crc!=iscrc) {
        LOGGER("aidexXprocessCurrentData wrong %x!=%x\n",crc,iscrc);
        return 1LL;
        }
    #ifndef NOLOG
    Showhex hex(data,arlen);
    LOGGER("aidexXprocessCurrentData %s\n",hex.data);
    #endif
     const int datalen=arlen-2;
    switch(data[0]) {
        case 3: 
        case 1:  {
            CritArSave<jlong> timeres(env,jtimeres);
            return saveCurrent(sens,data,datalen,*timeres.data());
            };
        case 4: ;  
        default: LOGGER("aidexXprocessCurrentData: Unknown type=%d\n",data[0]);

        } 
      return 2LL;
      }


extern "C" JNIEXPORT jboolean JNICALL   fromjava(aidexXSaveKey)(JNIEnv *env, jclass cl,jlong dataptr,jbyteArray value) {
    if(!value) {
        LOGAR("aidexXSaveKey value==null");
        return false;
        }
     auto *sdata=reinterpret_cast< aidexXstream *>(dataptr);
     SensorGlucoseData *sens=sdata->hist;
      if(!sens) {
          LOGAR("aidexXSaveKey SensorGlucoseData==null");
          return false;
         }
    const auto arlen=env->GetArrayLength(value);
     if(arlen!=16)  {
            LOGGER("aidexXSaveKey size  value %d != 16\n",arlen);
            return false;
            }
    const CritAr  bluedata(env,value);
    memcpy(sens->getinfo()->aidexXdat.keys[0].key,bluedata.data(),16);
    #ifndef NOLOG
    Showhex hex(sens->getinfo()->aidexXdat.keys[0].key,16);
    LOGGER("aidexXSaveKey %s success\n",hex.data);
    #endif
    return true; 
    } 
extern "C" JNIEXPORT jboolean JNICALL   fromjava(aidexHasKey)(JNIEnv *env, jclass cl,jlong dataptr) {
     auto *sdata=reinterpret_cast< aidexXstream *>(dataptr);
     SensorGlucoseData *sens=sdata->hist;
     if(!sens) {
          LOGAR("aidexHasKey SensorGlucoseData==null");
          return false;
         }
     if(sens==Sensoren::isdeleted)  {
        LOGAR("aidexHasKey: isdeleted");
        return false;
        }

//   return *reinterpret_cast<unsigned __int128*>(sens->getinfo()->aidexXdat.keys[0].key); //not always available
    const auto longkey=reinterpret_cast<const long long *>(sens->getinfo()->aidexXdat.keys[0].key); 
    return longkey[0]||longkey[1];
    } 


extern uint8_t crc8_maxim(const uint8_t *data, size_t len);
extern "C" JNIEXPORT jbyteArray JNICALL fromjava(aidexXstartCommand)(JNIEnv* env, jclass, jboolean haskey,jlong dataptr, jbyteArray value) {
   constexpr const char logPrefix[]{"aidexXstartCommand"};

    if (!value) {
        LOGGER("%s value==null", logPrefix);
        return nullptr;
    }

    auto* sdata = reinterpret_cast<aidexXstream*>(dataptr);
    sdata->unbonded=!haskey;
    SensorGlucoseData* sens = sdata ? sdata->hist : nullptr;
    if(!sens) {
        LOGGER("%s SensorGlucoseData==null", logPrefix);
        return nullptr;
       }

    const auto arlen = env->GetArrayLength(value);
    if(arlen != 17) {
        LOGGER("%s size value %d != 17\n", logPrefix, arlen);
        clearKey(sens);
        return nullptr;
       }

    auto&       newkey= sens->getinfo()->aidexXdat.keys[1];
    if(jniDecrypter(0,env, sens,value,newkey.key)<17) {
       LOGGER("%s jniDecrypter < 17\n",logPrefix);
        clearKey(sens);
        return nullptr;
        }
    uint8_t crc8=crc8_maxim(newkey.key,16);
    #ifndef NOLOG
    Showhex hex(newkey.key,17);
    LOGGER("%s %s  crc8=%x endcrc=%x\n",logPrefix,hex.data,crc8,newkey.crc8);
    #endif
    if(crc8!=newkey.crc8) {
        LOGGER("%s ERROR crc8 %x !=lastbyte %x\n",logPrefix,crc8,newkey.crc8);
        clearKey(sens);
//        memset(sens->getinfo()->aidexXdat.keys,'\0',16);
        return nullptr;
        }

   static constexpr uint8_t unpaircmd[]{0xf2,0xad,0x2e};
   static constexpr uint8_t cmd0x10[]{ 0x10, 0xc1, 0xf3 };
   static constexpr uint8_t cmd0x35[]{ 0x35, 0x01, 0x4e, 0xf7 };
   const bool late=haskey&&sens->getinfo()->aidexXdat.hasTime;
   const bool unpair=sdata->unpair;
   const bool clear=sdata->clear;
   return getEncrypted(env, sens,clear?clearCMD:(unpair?unpaircmd:(late?cmd0x35:cmd0x10)),(unpair||clear||!late)?sizeof(cmd0x10):sizeof(cmd0x35));
   }

//21 b3 d5


#endif

#include <jni.h>
#include "fromjava.h"
#include "logs.hpp"
#include "streamdata.hpp"

extern "C" JNIEXPORT jint JNICALL   fromjava(getManualWarmupMinutes)(JNIEnv *env, jclass cl,jlong dataptr) {
     if(!dataptr) {
          LOGAR("getManualWarmupMinute dataptr==0");
          return 60;
         }
     const SensorGlucoseData *sens=reinterpret_cast<const SensorGlucoseData *>(dataptr);
    return sens->getinfo()->manualwarmup;
    } 

extern "C" JNIEXPORT void JNICALL   fromjava(setManualWarmupMinutes)(JNIEnv *env, jclass cl,jlong sensorptr,jint min) {
     if(!sensorptr) {
          LOGAR("setManualWarmupMinute sensorptr==0");
          return;
         }
     SensorGlucoseData *sens=reinterpret_cast<SensorGlucoseData *>(sensorptr);
    sens->getinfo()->manualwarmup=min;
    if(sens->isAidexX()||sens->isLibre3()) {
        LOGGER("setManualWarmupMinutes(%d) warmupstartpos=%d\n",min,min);
        sens->getinfo()->warmupstartpos=min;
        }
    else {
        if(min==0) {
            LOGAR("setManualWarmupMinutes(0) warmupstartpos=0");
            sens->getinfo()->warmupstartpos=0;
            }
        else {
            const auto starttime=sens->getstarttime();
            const uint32_t endtime=starttime+min*60;
            const ScanData *polls=sens->beginpolls();
            const int count=sens->pollcount();
            const int nr=std::min(count,min+1);
            for(int it=1;it<nr;++it) {
                const ScanData &glu=polls[it];
                if(glu.valid(it)&&glu.t>endtime) {
                    LOGGER("setManualWarmupMinutes(%d) warmupstartpos=%d\n",min,it-1);
                    sens->getinfo()->warmupstartpos=it-1;
                    return;
                    }
                }
            }
        }
    } 


extern "C" JNIEXPORT jint JNICALL   fromjava(getMinimalWarmup)(JNIEnv *env, jclass cl,jlong dataptr) {
     if(!dataptr) {
          LOGAR("getMinimalWarmup dataptr==0");
          return 60;
         }
     const SensorGlucoseData *sens=reinterpret_cast<const SensorGlucoseData *>(dataptr);
     return sens->getMinimalWarmup();
     }

#ifdef DEXCOM

//AiDEX X-22222A2WC8

extern void    sendstreaming(SensorGlucoseData *hist);



static Sensoren::MicroTech getMicroType(const char *deviceName) {
   if(!memcmp(deviceName,Sensoren::nameLinX,sizeof(Sensoren::nameLinX)-1))
        return Sensoren::LinX;
   if(!memcmp(deviceName,Sensoren::nameLumiFlex,sizeof(Sensoren::nameLumiFlex)-1))
        return Sensoren::LumiFlex;
     return Sensoren::AidexX;
     }
extern "C" JNIEXPORT jstring JNICALL   fromjava(aidexXaddSensorByDeviceName)(JNIEnv *env, jclass cl,jstring jdeviceName) {
   const char *deviceName = env->GetStringUTFChars( jdeviceName, NULL);
   destruct   dest([jdeviceName,deviceName,env]() {env->ReleaseStringUTFChars(jdeviceName, deviceName);});
   const size_t deviceNamelen= env->GetStringUTFLength( jdeviceName);
   std::string_view scandeviceName{deviceName+deviceNamelen-10,10};
   const uint32_t now=time(nullptr);
   auto [sensindex,sens]= sensors->makeAidexXSensorindex(scandeviceName,getMicroType(deviceName),""sv,now);
   if(sens) {
      const char *name=sens->shortsensorname()->data();
      LOGGER("addSensorByDeviceName(%s)=%s\n",deviceName,name);
      sendstreaming(sens);  //TODO??
      backup->resendResetDevices();
      backup->wakebackup(wakeall);
      return env->NewStringUTF(name);
      }
   LOGGER("addSensorByDeviceName(%s)=nullptr\n",deviceName);
   return nullptr;
   }

extern bool aidexXaskKey(std::string_view serial,uint8_t uit[16]) ;
extern "C" JNIEXPORT jbyteArray  JNICALL   fromjava(aidexXaskKey)(JNIEnv *env, jclass cl,jlong dataptr) {
     auto *sdata=reinterpret_cast< aidexXstream *>(dataptr);
     SensorGlucoseData *sens=sdata->hist;
     if(!sens) {
          LOGAR("aidexXaskKey SensorGlucoseData==null");
          return nullptr;
         }
     if(sens==Sensoren::isdeleted)  {
        LOGAR("aidexXaskKey : isdeleted");
        return nullptr;
        }
    auto ar=env->NewByteArray(static_cast<jsize>(16));
    uint8_t md5[16];
    if(!aidexXaskKey( sens->aidexXsensorname(),md5) ) {
        return nullptr;
        }
    env->SetByteArrayRegion(ar, 0, static_cast<jsize>(16), (jbyte*)md5);
    return ar;
     }
#endif
