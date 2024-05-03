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
class SensorGlucoseData;
#include "AlgorithmContext.hpp"
class SiContext {
private:
   AlgorithmContext *algcontext;
#ifdef SIHISTORY
   AlgorithmContext *algcontext3;
#endif
    double process(int index,double value, double temp);
#ifdef SIHISTORY
    double process3(int index,double value, double temp);
void	 saveSi3(SensorGlucoseData *sens,int index,uint32_t eventTime,bool save,int value,float temp,bool savejson) ;
#endif
public:
   SiContext(SensorGlucoseData *sens);
    jlong processData(SensorGlucoseData *sens,time_t nowsecs,int8_t *data,int totlen,int sensorindex) ;
   ~SiContext();

    };
#endif
