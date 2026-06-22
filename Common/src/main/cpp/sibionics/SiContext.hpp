#ifdef SIBIONICS
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
/*      Thu Apr 04 20:14:40 CEST 2024                                                 */


#pragma once
#include <cstdint>
#include <ctime>
#include <jni.h>
#include "config.h"
#include "scanstate.hpp"
class SensorGlucoseData;
struct sensor;
#include "AlgorithmContext.hpp"
template <typename T,int NR> struct gegs;
typedef gegs<signed char> data_t;
class SiContext {
private:
   multimmap binState;
   AlgorithmContext *algcontext;
   bool newSI;
   double process2(int index,double value, double temp);
   double process3(int index,double value, double temp);
 jlong     handleOneGlucose(SensorGlucoseData *sens,int sensorindex,uint32_t nowsecs,int index,int tempin,int current,int reindex,time_t eventTime);
//   jlong     handleOneGlucose(SensorGlucoseData *sens,sensor *sensor,int sensorindex,uint32_t nowsecs,int index,int temp,int current,int reindex,int trend,time_t eventTime);
   jlong handleGlucoses(SensorGlucoseData *sens,int sensorindex,uint32_t nowsecs,const std::uint8_t* decoded_records, std::size_t record_count);
//   jlong interpret_data(SensorGlucoseData *sens,int sensorindex,const jbyte* data, jint datalen, jboolean doDecrypt);
jlong interpret_data(SensorGlucoseData *sens,int sensorindex,uint32_t nowsecs,const uint8_t* data, jint datalen, jboolean doDecrypt) ;
   void release();
public:
   void  	setNewSI(SensorGlucoseData *sens);
   SiContext(SensorGlucoseData *sens);
    jlong processData(SensorGlucoseData *sens,time_t nowsecs,int8_t *data,int totlen,int sensorindex) ;
#ifdef NEWSIBIONICS
    jlong processData2(SensorGlucoseData *sens,time_t nowsecs,data_t *data,int sensorindex) ;
#endif
   ~SiContext();

    };
#endif
