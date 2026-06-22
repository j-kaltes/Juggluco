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
/*      Wed May 06 21:10:29 CEST 2026                                                */


#ifdef SIBIONICS
#include "config.h"
#ifdef NEWSIBIONICS
#include <string_view>
#include <memory>
#include <inttypes.h>
#include "SensorGlucoseData.hpp"
#include "fromjava.h"
#include "logs.hpp"
#include "streamdata.hpp"
#include "datbackup.hpp"
#include "inout.hpp"
#include "EverSense.hpp"
#include <jni.h>

#ifndef NOLOG
extern void logbytes(std::string_view text,const uint8_t *value,int vallen) ;
#else
#define logbytes(text,value, vallen) 
#endif

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

extern "C" JNIEXPORT jbyteArray JNICALL   fromjava(getSIResetBytes)(JNIEnv *env, jclass cl) {
    constexpr const uint8_t data[]{0x24,0xE7,0x6F,0x34};
    constexpr const int  uitlen= sizeof(data);
    jbyteArray uit=env->NewByteArray(uitlen);
    env->SetByteArrayRegion(uit, 0, uitlen,(const jbyte*)data);
    return uit;
    }




extern int v120_isec_update(bool btrue, uint32_t timesec, uint8_t *output, uint16_t maxoutput);

void   getSItimecmd(std::vector<uint8_t> &vect) {
   constexpr const int maxcmd=7;
   vect.resize(maxcmd);
   uint8_t *cmd=vect.data();
   uint32_t timesec=time(nullptr);
   v120_isec_update(true,  timesec, cmd, maxcmd);
   }



extern "C" JNIEXPORT jbyteArray JNICALL   fromjava(getSItimecmd)(JNIEnv *env, jclass cl) {
   constexpr const int maxcmd=7;
   uint8_t cmd[maxcmd];
   uint32_t timesec=time(nullptr);
   int len= v120_isec_update(true,  timesec, cmd, maxcmd);
   jbyte *dat=(jbyte*)cmd;
   jbyteArray uit=env->NewByteArray(len);
   env->SetByteArrayRegion(uit, 0, len,dat);
   return uit;
   }


extern int64_t v120_apply_authentication(int sisubtype, const uint8_t* deviceName, uint8_t* dest_buf, uint16_t dest_size);

std::array<jbyte,6>  deviceArray(const char address[]);
extern "C" JNIEXPORT jbyteArray JNICALL   fromjava(siAuthBytes)(JNIEnv *env, jclass cl,jlong dataptr) {
   if(!dataptr) {
       LOGAR("siAuthBytes dataptr==null");
       return nullptr;
       }
   const SensorGlucoseData *usedhist=reinterpret_cast<streamdata *>(dataptr)->hist ; 
   if(!usedhist) {
       LOGAR("siAuthBytes usedhist==null");
       return nullptr;
       }
   const auto address=usedhist->deviceaddress();
   auto rev=deviceArray(address);
   constexpr const int maxcmd=26;
   uint8_t cmd[maxcmd];
   int len=v120_apply_authentication(usedhist->siSubtype(), (const uint8_t *)rev.data(), cmd, maxcmd);
   logbytes("siAuthBytes",cmd,len);
   jbyteArray uit=env->NewByteArray(len);
   env->SetByteArrayRegion(uit, 0, len,(const jbyte*)cmd);
   return uit;
   }


extern int getActivation(uint32_t timesec,uint8_t *output,int maxoutput);
extern "C" JNIEXPORT jbyteArray JNICALL   fromjava(getSIActivation)(JNIEnv *env, jclass cl) {
   constexpr const int maxcmd=11;
   uint8_t cmd[maxcmd];
   uint32_t timesec=time(nullptr);
   auto len=getActivation(timesec,cmd,maxcmd);
   jbyteArray uit=env->NewByteArray(len);
   env->SetByteArrayRegion(uit, 0, len,(jbyte*)cmd);
   return uit;
   }


jlong SiContext::processData2(SensorGlucoseData *sens,time_t nowsecs,data_t *data,int sensorindex)  {
  const int datasize=data->size();
  logbytes("processData2 input: ",(const uint8_t *)data->data(),datasize);
  jlong res=interpret_data(sens,sensorindex,nowsecs,(const uint8_t *)data->data(), datasize, true); 
  if(res==0LL) {
     res=interpret_data(sens,sensorindex,nowsecs,(const uint8_t *)data->data(), datasize, false); 
     }
  if(res==10LL) {
      if(sens->siSubtype()!=3)  {
            return 6LL; //Never used??
            }
       }
  return res;
  }
#endif
#endif

