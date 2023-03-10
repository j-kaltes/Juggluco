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


#ifndef WEAROS
#include <jni.h>
#include <span>
#include <thread>
#include <sys/prctl.h>
#include "SensorGlucoseData.h"
#include "fromjava.h"
#include "sensoren.h"
extern Sensoren *sensors;
void todrip(JNIEnv *env,const char *sensorname, const std::span<const ScanData> stream) { 
	if(stream.size()) {
		jstring jsensorname= env->NewStringUTF(sensorname);
		for(const ScanData &el:stream) {
			if(el.valid())	 {
				extern jclass XInfuus;
				extern jmethodID  sendGlucoseBroadcast;
				const long long wastime= el.gettime()*1000LL;
				LOGGER("%s %d %f %lld\n",sensorname,el.getmgdL(),el.ch,wastime);
				env->CallStaticVoidMethod(XInfuus,sendGlucoseBroadcast,jsensorname,el.getmgdL(),el.ch,wastime);
				}

			}
		}
	}
static bool xdriprunning=false;
/*
void sendxdripold() {
	if(!sensors)
		return;
extern JNIEnv *getenv();
	
	  prctl(PR_SET_NAME, "sendtoxdrip", 0, 0, 0);

	 xdriprunning=true;
	JNIEnv *env=getenv();
        auto toxdrip=[env](const SensorGlucoseData *sens){
		todrip(env,sens->shortsensorname()->data(),sens->getPolldata() );
		};
	sensors->onallsensors(toxdrip); 
	 xdriprunning=false;
	}
	*/
void sendxdripold() {
	if(!sensors)
		return;
extern JNIEnv *getenv();
	

	 xdriprunning=true;
	JNIEnv *env=getenv();
        if(const SensorGlucoseData *sens=sensors->gethist()) {
	  		prctl(PR_SET_NAME, "sendtoxdrip", 0, 0, 0);
			todrip(env,sens->shortsensorname()->data(),sens->getPolldata() );
			};
	 xdriprunning=false;
	}

extern "C" JNIEXPORT void JNICALL   fromjava(sendxdripold)(JNIEnv *envin, jclass cl) {
	if(xdriprunning)
		return;
	std::thread xdripthread(sendxdripold);
	xdripthread.detach();
	}
#else
extern "C" JNIEXPORT void JNICALL   fromjava(sendxdripold)(JNIEnv *envin, jclass cl) {
	}
#endif
