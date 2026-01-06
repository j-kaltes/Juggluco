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

#include "config.h"
#ifdef OLDEVERSENSE 
#include "EverSense.hpp"
#include <jni.h>
#include <span>
#ifndef HAVE_NOPRCTL
#include <sys/prctl.h>
#endif
#include "SensorGlucoseData.hpp"
#include "fromjava.h"
#include "sensoren.hpp"
#include "calibrate/Calibrator.hpp"
extern Sensoren *sensors;
void toEverSense(JNIEnv *env,const SensorGlucoseData *sens,const std::span<const ScanData> stream,const int modulo) { 
   if(stream.size()) {
       for(const ScanData &el:stream) {
         if(!el.valid()) {
            LOGGER("to EverSense %d %d %f %lld invalid\n",el.getid(),el.getmgdL(),el.ch,el.gettime());
            continue;
            }
         if((el.getid()%modulo)!=0)	 {
              LOGGER("to EverSense %d%d!=0 \n",el.getid(),modulo);
              continue;
              }
           extern jclass EverSense;
           extern jmethodID  sendGlucoseBroadcast;
           const long long wastime= el.gettime()*1000LL;
           int mgdL;
           auto cali= make_calibrator<ScanData>(sens);
           if(double calibrated= cali.calibrateONEtest(el);!isnan(calibrated)) {
                mgdL=(int)round(calibrated);
                }
           else {
                mgdL=el.getmgdL();
                }
           LOGGER("to EverSense %d %d %f %lld\n",el.getid(),mgdL,el.ch,el.gettime());
           env->CallStaticVoidMethod(EverSense,sendGlucoseBroadcast,mgdL,el.ch,wastime);
         }
       }
   }
/*
void sendEverSenseold() {
	if(!sensors)
		return;
extern JNIEnv *getenv();
	
	  prctl(PR_SET_NAME, "sendtoEverSense", 0, 0, 0);

	 EverSenserunning=true;
	JNIEnv *env=getenv();
        auto toEverSense=[env](const SensorGlucoseData *sens){
		toEverSense(env,sens->shortsensorname()->data(),sens->getPolldata() );
		};
	sensors->onallsensors(toEverSense); 
	 EverSenserunning=false;
	}
	*/
void sendEverSenseoldthread(const SensorGlucoseData *sens,int startpos,int endpos,int modulo) {
   extern JNIEnv *getenv();
   JNIEnv *env=getenv();
#ifndef HAVE_NOPRCTL
   prctl(PR_SET_NAME, "sendtoEverSense", 0, 0, 0);
#endif
   const auto data=sens->getPolldata();
   const auto start=&data[startpos];
   const auto end=&data[endpos];
   uint32_t  starttime=time(nullptr)-4*60*60;
   const auto newstart=sens->firstnotless({start,end},starttime);
   if(newstart<end) {
	 toEverSense(env,sens,{newstart,end},modulo);
	 }
      else  {
	 LOGGER("sendEverSenseold %p>=%p\n",start,end);
	}
     }
#endif
