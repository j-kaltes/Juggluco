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
/*      Thu Mar 23 21:03:27 CET 2023                                                 */


#include <jni.h>

#include "fromjava.h"
#include "settings/settings.h"
#include "net/watchserver/getitems.h"

constexpr const int FUZZER=30;

extern "C" JNIEXPORT jboolean JNICALL fromjava(mkWearIntgraph)(JNIEnv *env, jclass thiz, jlong startin,jlong endin,jobject low,jobject within, jobject high) {
char *notused;
uint32_t start=startin/1000;
uint32_t end=endin/1000;
int duration=end-start;
int datnr=120;
int interval=duration/datnr;
LOGGER("start=%ud end=%ud duration=%d interval=%d\n",start,end,duration,interval);
	auto targetlow=settings->data()->tlow;
	auto targethigh=settings->data()->thigh;
 return getitems(notused,datnr,start,end,true, interval,[env,targetlow,targethigh,low,within,high](char *outiter,int datit, const ScanData *iter,const char *sensorname,const time_t starttime) {
 	const static       jmethodID jadd=env->GetMethodID(env->FindClass("com/eveningoutpost/dexdrip/services/broadcastservice/models/GraphLine"),"add","(FF)V");
	auto valuemgL=iter->getsputnik();
	jobject jlist=(valuemgL<targetlow)?low:(valuemgL>targethigh?high:within); 
	float time= (float)((double)iter->gettime()/FUZZER);
	float value= gconvert(valuemgL);
	LOGGER("add(%f,%f)\n",time,value);
	 env->CallVoidMethod(jlist,jadd,time,value);
	 return outiter;
	});
   }
