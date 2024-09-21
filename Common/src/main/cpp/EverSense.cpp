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
#ifndef WEAROS
#include <jni.h>
#include <span>
#include <sys/prctl.h>
#include "SensorGlucoseData.hpp"
#include "fromjava.h"
#include "sensoren.hpp"
extern Sensoren *sensors;
void toEverSense(JNIEnv *env,const std::span<const ScanData> stream) { 
	if(stream.size()) {
		for(const ScanData &el:stream) {
			if(el.valid()&&(el.getid()%5)==0)	 {
				extern jclass EverSense;
				extern jmethodID  sendGlucoseBroadcast;
				const long long wastime= el.gettime()*1000LL;
				LOGGER("to EverSense %d %f %lld\n",el.getmgdL(),el.ch,wastime);
//	broadcastglucose(int mgdl, float rate, long timmsec)
				env->CallStaticVoidMethod(EverSense,sendGlucoseBroadcast,el.getmgdL(),el.ch,wastime);
				}

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
void sendEverSenseoldthread(const SensorGlucoseData *sens,int startpos,int endpos) {
	extern JNIEnv *getenv();
     JNIEnv *env=getenv();
	prctl(PR_SET_NAME, "sendtoEverSense", 0, 0, 0);
	const auto data=sens->getPolldata();
	const auto start=&data[startpos];
	const auto end=&data[endpos];
   uint32_t  starttime=time(nullptr)-4*60*60;
   const auto newstart=sens->firstnotless({start,end},starttime);
	 if(newstart<end) {
	    toEverSense(env,{newstart,end});
	    }
	 else  {
	    LOGGER("sendEverSenseold %p>=%p\n",start,end);
	   }
	}
#endif
#endif
