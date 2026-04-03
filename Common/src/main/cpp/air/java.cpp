#ifdef SIBIONICS
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
/*      Tue Jan 06 12:49:00 CET 2026                                                 */
#include <dlfcn.h>
#include <array>
#include <stdint.h>
#include <jni.h>
#include "logs.hpp"
#include "streamdata.hpp"
#include "fromjava.h"
#include "air.hpp"
#include "jniclass.hpp"
#include "glucose.hpp"
#include "datbackup.hpp"


extern void *openlib(std::string_view libname);

extern jlong mkres(SensorGlucoseData *sens,uint32_t timsec,uint32_t eventTime, int min,int mgdL, int abbotttrend, float change);
extern int rate2changeindex(float rate);
typedef unsigned char (*air1_opcal4_algorithm_t)(air1_opcal4_device_info_t *, air1_opcal4_cgm_input_t *, air1_opcal4_cal_list_t *, air1_opcal4_arguments_t *, air1_opcal4_output_t *, air1_opcal4_debug_t *);
static air1_opcal4_algorithm_t air1_opcal4_algorithm;
static bool getlibfuncs() {
    std::string_view libcal{"/libCALCULATION.so"};
    void *handle=openlib(libcal);
    if(!handle) {
        LOGGER("dlopen %s failed: %s\n",libcal.data(),dlerror());
        return false;
        }
   constexpr const char str[]="air1_opcal4_algorithm";
   air1_opcal4_algorithm= (air1_opcal4_algorithm_t) dlsym(handle,str);
   if(!air1_opcal4_algorithm) {
        LOGGER("dlsym %s failed: %s\n",str,dlerror());
        return false;
        }
    return true;
    };
bool airstream::setNumberNew(int nr) {
//    if(tmptot) return false;
   //  data_t::deleteex (tmpinput);
//     const int totsize=nr*74;
     LOGGER("airstream::setNumberNew(%s,%d)\n",hist->showsensorname().data(),nr);
     tmpiter=0;
     tmptot=nr;
     ininfo=0;
   //  tmpinput=data_t::newex(totsize);
     return true;
    }
extern "C" JNIEXPORT jint JNICALL   fromjava(airGetLast)(JNIEnv *env, jclass cl,jlong dataptr) {
     const airstream *sdata=reinterpret_cast<const airstream *>(dataptr);
     const SensorGlucoseData *sens=sdata->hist;
      if(!sens) {
          LOGAR("airGetLast SensorGlucoseData==null");
          return -1;
         }
    int res=sens->getLastAir();
    if(res>0)
            res-=sens->getinfo()->askEarlier;
    LOGGER("airGetLast()=%d askEarlier=%d\n",res,sens->getinfo()->askEarlier);
    return res;
    }

    /*
extern "C" JNIEXPORT void JNICALL   fromjava(airSetNumberNew)(JNIEnv *env, jclass cl,jlong dataptr,jint nr) {
     auto *sdata=reinterpret_cast<airstream *>(dataptr);
     data_t::deleteex (sdata->tmpinput);
     const int totsize=nr*74;
     LOGGER("airSetNumberNew(%p,%d)\n",dataptr,nr);
     sdata->tmpiter=0;
     sdata->tmptot=nr;
     sdata->errors=0;
     sdata->tmpinput=data_t::newex(totsize);
    } */


void       airsavehistory( SensorGlucoseData *sens, air1_opcal4_output_t &output) {
     int lastid=output.seq_number_final;
     uint32_t lasttime=output.measurement_time_standard;

     int firstid=0,endhistory=0;
     for(int i=0;i<6;++i) {
        int id=output.smooth_seq[i];
        double mgdL=output.smooth_result_glucose[i];
        if(id&&mgdL>0.0) {
             if(!firstid)
                firstid=id;
             uint32_t time=(id-lastid)*5*60+lasttime;
             Glucose *item=sens->getglucose(id);
             item->time=time;
             auto lifeCount=id*5;
             item->id=lifeCount;
             auto mgL=std::round(mgdL*10.0);
             item->glu[1]=mgL;
             #ifndef NOLOG
             time_t wastime=time;
             LOGGER("airsavehistory(%d,%d,%.1f) %s",id,lifeCount,mgL/convfactor,ctime(&wastime));
             #endif
             endhistory=id;
             }
        }
     {
     double mgdLdouble=output.result_glucose;
     if(mgdLdouble>0.0) {
         int mgL= std::round(mgdLdouble*10.0);
         Glucose *item=sens->getglucose(lastid);
         item->time=lasttime;
         auto lifeCount=lastid*5;
         item->id=lifeCount;
         item->glu[1]=mgL;
         #ifndef NOLOG
         time_t wastime=lasttime;
         LOGGER("airsavehistory(%d,%d,%.1f) %s",lastid,lifeCount,mgL/convfactor,ctime(&wastime));
         #endif
         endhistory=lastid;
         if(!firstid)
                firstid=lastid;
         }
     }
    if(firstid) {
        if(sens->getstarthistory()<=0) {
            int i=firstid-1;
            for(;i>0&&sens->getglucose(i)->id;--i) 
                ;
            sens->setstarthistory(i+1);
            }
        ++endhistory;
        if(endhistory>sens->getScanendhistory())
            sens->setendhistory(endhistory);
        sens->backhistory(firstid);
        };

    }

template <typename T>
     char *   printarray(char *ptr,char *end,T *numbers,const int len) {
        for(int i=0;i<(len-1);++i) {
                auto res=std::to_chars(ptr,end,numbers[i]); 
                ptr=res.ptr;
                *ptr++=',';
                *ptr++=' ';
                };
        auto res=std::to_chars(ptr,end,numbers[len-1]); 
        *res.ptr='\0';
        return res.ptr;
        }
static void showm(const  m *mptr) {
    time_t time=mptr->measurement_time;
    constexpr int maxbuf=400;
    char buf[maxbuf];
    char *endbuf=buf+maxbuf;
    int endpos=snprintf(buf,maxbuf,"m seq_num %d temperature %.1f unixtime=%lu: ", mptr->sequence_number,mptr->temperature,time);
    char *end=printarray(buf+endpos,endbuf,mptr->glucose_array.data(),30);
    *end++=' ';
    ctime_r(&time,end);
    LOGGERN(buf,end-buf+24);
    }
extern jlong airProcessData(airstream *sdata,const jbyte *indata,int datalen,jlong *timeres);
extern "C" JNIEXPORT jlong JNICALL   fromjava(airProcessData)(JNIEnv *env, jclass cl,jlong dataptr,jbyteArray value,jlongArray jtimeres) {

    if(!value) {
        LOGAR("airProcessData value==null");
        return 1LL;
        }
         //TODO: other versions?
   const auto arlen=env->GetArrayLength(value);
   if(arlen<4)  {
            LOGGER("airProcessData size  value %d < 4\n",arlen);
            return 1LL;
            }

    airstream *sdata=reinterpret_cast<airstream *>(dataptr);
    CritArSave<jlong> timeres(env,jtimeres);
    const CritAr  bluedata(env,value);
    return airProcessData(sdata,bluedata.data(),arlen,timeres.data());
    }
jlong airProcessData(airstream *sdata,const jbyte *indata,int arlen,jlong *timeres) {
     SensorGlucoseData *sens=sdata->hist;
      if(!sens) {
          LOGAR("airProcessData SensorGlucoseData==null");
          return 1LL;
         }
    const AirData *air=reinterpret_cast<const AirData *>(indata);
    if(air->reg1!=1) {
        LOGGER("airProcessData second byte %d not 1\n",air->reg1);
        return 1LL;
        }
    jlong msec=timeres[0];
    uint32_t nowsec= msec/1000LL;
    if(air->reg0==0xC4) {
        const int newrecords=air->numRecords;
        sdata->setNumberNew(newrecords);
        if(!newrecords) {
            if(const ScanData *last=sens->lastpoll()) {
                int lastid=sens->getLastAir();
                int dataid=last->getid()/5;
                int errortime=(lastid-dataid)*5*60;
                int waiting=nowsec-last->gettime()-errortime;
                LOGGER("getLastAir()=%d dataid=%d now=%u lastime=%u errortime=%u waiting=%d\n",lastid,dataid,nowsec,last->gettime(),errortime,waiting);
                if(waiting>62*5) {
                     ++sens->getinfo()->askEarlier;
                     LOGGER("newrecords=0 set askEarlier=%d\n",sens->getinfo()->askEarlier);
                     return 2LL;
                     }
                }
            }
         return 3LL;
        }
    if(air->reg0!=0xC5) {
        LOGGER("airProcessData first byte %d not 197\n",air->reg0);
        return 1LL;
        }
     if(arlen<sizeof(AirData))  {
        LOGGER("airProcessData size  value %d < AirData %d\n",arlen,sizeof(AirData));
//        sens->sensorerror=true;
 //       sens->sensorErrorTime=timsec;
        return 1LL;
        }
     ++sdata->tmpiter;
    if(!air->deviceErrorCode) {
        static bool haslibs=getlibfuncs();
        auto mtime=air->time;
        if(mtime<31532400) {
            if(sdata->tmptot>1)
                mtime=nowsec - (sdata->tmptot - sdata->tmpiter ) * 300;
             else
                mtime=nowsec;
            }
       const int idnow=air->sequenceNumber;
#ifdef AIR_STACK
       air_input input{.data={.sequence_number=static_cast<uint16_t>(idnow),.measurement_time=mtime,.glucose_array=air->glucose_array,.temperature=air->temperature/100.0}};
       air1_opcal4_output_t output{};
       air1_opcal4_debug_t debug{};
#else
     air_input &input=sdata->input;
        input={.data={.sequence_number=static_cast<uint16_t>(idnow),.measurement_time=mtime,.glucose_array=air->glucose_array,.temperature=air->temperature/100.0}};
       air1_opcal4_output_t &output=sdata->output; sdata->output={};
       air1_opcal4_debug_t &debug=sdata->debug; sdata->debug={};
#endif
       showm(&input.data);

       DeviceInfo3Obj *deviceInfo=sdata->sensorInfo.data();

       unsigned char res=air1_opcal4_algorithm(reinterpret_cast<air1_opcal4_device_info_t *>(deviceInfo), &input.cgm_input, &input.empty, sdata->generated.data(), &output, &debug);
       double mgdLdouble=output.result_glucose;

       if(res&&!output.errcode) {
            double mmolL=mgdLdouble/18.0;
            int mgdL= std::round(mgdLdouble);
            double trendrate=output.trendrate;
            if(trendrate>99.0)
                trendrate=NAN;
            int abbotttrend=rate2changeindex(trendrate);
            int id=output.seq_number_final*5;
            uint32_t time=output.measurement_time_standard;
            time_t tim=time;
           const char *label=abbotttrend<6?GlucoseNow::trendString[abbotttrend]:"Error";
            LOGGER("airProcessData: nr=%d id=%d glucose=%.1f mg/dL %.1f mmol/L trendrate=%.2f %s (%d) %u  %s", output.seq_number_final, id,mgdLdouble,mmolL,trendrate,label,abbotttrend,time,ctime(&tim));
            if(mgdLdouble>35.0&&mgdLdouble<505.0) {
                auto res=mkres(sens,nowsec,time,id, mgdL,  abbotttrend, trendrate);
                if(!res) {
                    if(mgdL) {
                        if(sens->getinfo()->askEarlier)
                            --sens->getinfo()->askEarlier;

                        LOGGER("set askEarlier=%d\n",sens->getinfo()->askEarlier);
                        res=2LL;
                        }
                    }
                airsavehistory(sens,output);
                timeres[0]=time*1000LL;
                sens->setLastAir(output.seq_number_final);
                return res;
                }
            }
        sens->setLastAir(idnow);
        LOGGER("airProcessData: air1_opcal4_algorithm id=%d res=%d output.errcode=%d\n",idnow,(int)res,output.errcode);
       }
     else {
            LOGGER("airProcessData: air->deviceErrorCode=%d\n",air->deviceErrorCode);
        }
    sens->sensorerror=true;
    sens->sensorErrorTime=nowsec;
    return 0LL;
    }
//s/Digit(1[^"]*""[^"]*,"\([a-zA-Z0-9_]*\)",.*$/uint8_t \1;/g
//s/Ascii(\([0-9]*\),[^"]*"."[^"]*,"\([a-zA-Z0-9_]*\)",.*$/char \2[\1];/g
struct SensorInfo {
  uint8_t reg[2]; 
  uint8_t sensorVersion;
  float ycept;
  float slope100;
  float slope;
  float r2;
  float t90;
  float slope_ratio;
  char lot[10];
  char sensor_id[12];
  char expiration[6];
  uint16_t stabilizationInterval;
  uint16_t cgmDataInterval;
  uint16_t bleAdvInterval;
  uint8_t bleAdvDuration;
  uint8_t age;
  uint16_t allowedList;
  float maxGlucose;
  float minGlucose;
  uint8_t mCLibraryVersion;
  } __attribute__ ((packed));

extern "C" JNIEXPORT jboolean JNICALL   fromjava(airSaveSensorInfo)(JNIEnv *env, jclass cl,jlong dataptr,jbyteArray value) {
    if(!value) {
        LOGAR("airSaveSensorInfo value==null");
        return false;
        }
     airstream *sdata=reinterpret_cast<airstream *>(dataptr);
     SensorGlucoseData *sens=sdata->hist;
      if(!sens) {
          LOGAR("airSaveSensorInfo SensorGlucoseData==null");
          return false;
         }
    const auto arlen=env->GetArrayLength(value);
     if(arlen<sizeof(SensorInfo))  {
            LOGGER("airSaveSensorInfo size  value %d < %zd\n",arlen,sizeof(SensorInfo));
            return false;
            }
    const CritAr  bluedata(env,value);
    const SensorInfo *air=reinterpret_cast<const SensorInfo *>(bluedata.data());
    if(air->reg[0]!=0xC2) {
        LOGGER("airSaveSensorInfo firsts byte not 0xC2, but %d\n",air->reg[0]);
        return false;
        }
    if(air->reg[1]!=1) {
        LOGGER("airSaveSensorInfo first byte %d not 0\n",air->reg[1]);
        return false;
        }
    if(air->mCLibraryVersion<2) {
        LOGAR("airSaveSensorInfo cLibraryVersion<2");
        sdata->ininfo=0;
         return true;
         }
     LOGGER("airSaveSensorInfo lot %.10s minGlucose=%d maxGlucose=%d\n",air->lot,air->minGlucose,air->maxGlucose);
     auto *sensorinfo=sdata->sensorInfo.data();
     memcpy(sensorinfo,bluedata.data()+2,sizeof(SensorInfo)-2);
     sensorinfo->stabilizationInterval=1800;
     sdata->ininfo=72;
     return true;
    } 
extern "C" JNIEXPORT jboolean JNICALL   fromjava(airSaveSensorInfo2)(JNIEnv *env, jclass cl,jlong dataptr,jbyteArray value) {
    if(!value) {
        LOGAR("airSaveSensorInfo2 value==null");
        return false;
        }
     airstream *sdata=reinterpret_cast<airstream *>(dataptr);
     SensorGlucoseData *sens=sdata->hist;
      if(!sens) {
          LOGAR("airSaveSensorInfo2 SensorGlucoseData==null");
          return false;
         }
    const CritAr  bluedata(env,value);
    const AirData *air=reinterpret_cast<const AirData *>(bluedata.data());
    const uint8_t *data=reinterpret_cast<const uint8_t *>(bluedata.data());
    if(data[0]!=0xC2) {
        LOGAR("airSaveSensorInfo2 firsts byte not 0xC2");
        return false;
        }
    if(data[1]!=2) {
        LOGGER("airSaveSensorInfo2 first byte %d not 2\n",air->reg0);
        return false;
        }
    auto *sensorinfo=sdata->sensorInfo.data();
    const int ininfo=sdata->ininfo;
    int cplen;
    if(sensorinfo->cLibraryVersion>=2) {
          if(ininfo==72) {
                cplen=157;
                }
          else {
                cplen=205;
                }
           }
      else  {
        if(!ininfo) {
            cplen=202;
            }
        else {
            cplen=125;
            }
         }
    LOGGER("airSaveSensorInfo2 ininfo=%d len=%d\n",ininfo,cplen);
    memcpy(reinterpret_cast<uint8_t*>(sensorinfo)+ininfo,bluedata.data()+2,cplen);
    sdata->ininfo+=cplen;
    return true;
    } 


extern "C" JNIEXPORT void JNICALL   fromjava(airSaveStartSensor)(JNIEnv *env, jclass cl,jlong dataptr, jfloat eapp,jfloat vref,int elapsedSecs){
     airstream *sdata=reinterpret_cast<airstream *>(dataptr);
     SensorGlucoseData *sens=sdata->hist;
      if(!sens) {
          LOGAR("airSaveStartSensor SensorGlucoseData==null");
          return;
         }
    DeviceInfo3Obj *sensorinfo=sdata->sensorInfo.data();
    uint32_t sensorstart=time(nullptr)-elapsedSecs;
    sensorinfo->sensor_start_time=sensorstart;
    uint32_t wasstart=sens->getinfo()->starttime;
    sens->getinfo()->starttime=sensorstart;
    sensorinfo->eapp=eapp;
    sensorinfo->vref=vref;
    #ifndef NOLOG
    time_t time=sensorstart;
    LOGGER("airSaveStartSensor eapp=%f vref=%f elapsedSeconds=%d starttime=%lu %s",eapp,vref,elapsedSecs,sensorstart,ctime(&time));
    #endif
    if(abs((int)(wasstart-sensorstart))>30) {
           const int sensorindex=sens->sensorIndex;
           sensors->getsensor(sensorindex)->starttime=sensorstart;
           sensors->setindices();
           backup->resendResetDevices(&updateone::sendstream);
        }
    } 
extern "C" JNIEXPORT jbyteArray JNICALL  fromjava(airGetPin)(JNIEnv *env, jclass cl,jlong dataptr){
     airstream *sdata=reinterpret_cast<airstream *>(dataptr);
     SensorGlucoseData *sens=sdata->hist;
      if(!sens) {
          LOGAR("airSaveStartSensor SensorGlucoseData==null");
          return nullptr;
         }
   const int pinlen=6;
   jbyteArray uit=env->NewByteArray(pinlen);
   env->SetByteArrayRegion(uit, 0, pinlen,reinterpret_cast<jbyte*>(sens->getinfo()->airData.pinCode));
   LOGGER("airGetPin(%p)=%.6s \n",dataptr,sens->getinfo()->airData.pinCode);
   return uit;
   }


#endif
