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
/*      Fri Jan 27 15:22:27 CET 2023                                                 */


#include <vector>
#include <stdio.h>
#include <jni.h>
#include "SensorGlucoseData.hpp"
#include "sensoren.hpp"
#include "settings/settings.hpp"
#include "share/fromjava.h"
#include "datbackup.hpp"
#include "gluconfig.hpp"

#define LOGGERTAG(...) LOGGER("watchvalue: " __VA_ARGS__)
#define LOGSTRINGTAG(...) LOGSTRING("watchvalue: " __VA_ARGS__)
extern Sensoren *sensors;
extern std::vector<int> usedsensors;
const int maxwatchage=maxbluetoothage;
extern bool networkpresent;

#define javapackage "tk/glucodata/"

extern void resetnetwork();
static void keepnet(uint32_t wastime,uint32_t nu) {
	const bool fromother=backup&&networkpresent&&settings->data()->nobluetooth ;
	if(fromother) {
		const uint32_t age=nu-wastime;
		constexpr const int requestage=126;
		if(age>requestage) {
			static uint32_t nextwake=0;
			if(nu>nextwake) {
				static bool wakenext=false;
				if(!wakenext) {
					resetnetwork();
					wakenext=true;
					}
				else  {
					backup->getupdatedata()->wakestreamsender();
					nextwake=nu+60u;
					wakenext=false;
					}
				}
			}
		}
	}

std::pair<const SensorGlucoseData *,int> getlaststream(const uint32_t nu) {
// 	uint32_t mintime=nu-maxwatchage;
 	uint32_t mintime=0;
	const SensorGlucoseData *take=nullptr;
   int pos=-1;
   const int total=usedsensors.size();
	for(int i=0;i<total ;i++) {
		const int index=usedsensors[i];
		const SensorGlucoseData *hist=sensors->getSensorData(index);
		const ScanData *poll=hist->lastpoll();
		if(poll) {
			uint32_t then=poll->t;
			if(then>mintime) {
				mintime=then;
				take=hist;
                pos=i;
				}
			}
		}
	keepnet(mintime,nu);
   if(total>1)
      ++pos;
	return {take,pos};
	}

extern "C" JNIEXPORT jlong  JNICALL   fromjava(lastglucosetime)(JNIEnv *env, jclass cl) {
	const auto [hist,index]=getlaststream(maxwatchage);
	if(hist) 
		return hist->lastpoll()->t*1000LL;
		
	return 0LL;
       }





int getglucosestr(uint32_t nonconvert,char *glucosestr,int maxglucosestr);


extern float threshold(float drate);
extern "C" JNIEXPORT jobject  JNICALL   fromjava(lastglucose)(JNIEnv *env, jclass cl) {
	const uint32_t nu=time(nullptr);	
	const auto [hist,index]=getlaststream(maxwatchage);
	if(!hist)  {
		LOGSTRINGTAG("getlaststream(nu)=null\n");
		return nullptr;
		}
	const ScanData *poll=hist->lastpoll();
	if(!poll||!poll->valid()) {
		return nullptr;
		}

	const auto nonconvert= poll->g;
	if(!nonconvert)  {
		LOGSTRINGTAG("glucose = 0\n");
		return nullptr;
		}
	const int maxbuf=20;
	char buf[maxbuf];
	int len=getglucosestr(nonconvert,buf,maxbuf);
	const char glucoseclass[]= javapackage "strGlucose";
	static  jclass  item=  (jclass) env->NewGlobalRef(env->FindClass(glucoseclass));
	if(!item) {
		LOGGERTAG("FindClass(%s) failed\n",glucoseclass);
		return nullptr;
		}
	const char glsig[]= "(JLjava/lang/String;Ljava/lang/String;FI)V";
	static jmethodID iconstruct = env->GetMethodID(item,"<init>",glsig);
	if(!iconstruct) {
		LOGGERTAG("GetMethodID(item,<init>,%s) failed\n", glsig);
		return nullptr;
		}
	const jlong tim=poll->gettime();
	const char *sensorid=hist->othershortsensorname()->data();
	LOGGERTAG("strGlucose(%lld,%s,%s,%.1f)\n",(long long)tim,buf,sensorid,poll->ch);
	const float rateofchange=( nonconvert<glucoselowest||nonconvert>glucosehighest)?NAN:threshold(poll->ch);
	return env->NewObject(item,iconstruct,tim,env->NewStringUTF(buf),env->NewStringUTF(sensorid),rateofchange,index);
       }



#include <jni.h>
#include <android/log.h>

///#define fromjava(x) Java_com_example_android_wearable_datalayer_Natives_ ##x
    /*
struct glucose  {
    uint32_t time;
    float value;
    float rate;
    };
const char LOG_ID[]="Natives";
const int pri=3;

#define LOGGERTAG(...) __android_log_print(pri,LOG_ID,__VA_ARGS__)
*/

/*
static  jclass  jglucoseclass;
extern "C" JNIEXPORT jobject  JNICALL   fromjava(bytearray2glucose)(JNIEnv *env, jclass cl,jbyteArray jar) {
    jsize lens=env->GetArrayLength(jar);
    jbyte data[lens];
    env->GetByteArrayRegion(jar, 0, lens,data);
    const glucose *gl=reinterpret_cast<const glucose*>(data);
    const char sig[]= "(JFF)V";
    static jmethodID iconstruct = env->GetMethodID(jglucoseclass,"<init>",sig);
    if(!iconstruct) {
        LOGGERTAG("GetMethodID(jglucoseclass,<init>,%s) failed\n",sig);
        return nullptr;
        }
    return env->NewObject(jglucoseclass,iconstruct,(jlong)gl->time,gl->value,gl->rate);
    }
bool loadglucoseclass(JNIEnv *env) {
	const char glucosestr[]= javapackage "Glucose";
    jglucoseclass=  (jclass) env->NewGlobalRef(env->FindClass(glucosestr));
    if(!jglucoseclass) {
        LOGGERTAG(R"(FindClass()" "%s" R"() failed)" "\n",glucosestr);
        return false;
        }
    return true;
}
extern "C" JNIEXPORT jbyteArray JNICALL   fromjava(glucose2bytearray)(JNIEnv *env, jclass cl,jobject jglucose) {
    static jfieldID jtime=env->GetFieldID(jglucoseclass,"time","J");
    jlong time=env->GetLongField(jglucose,jtime);
    static jfieldID jvalue=env->GetFieldID(jglucoseclass,"value","F");
    jfloat value=env->GetFloatField(jglucose,jvalue);
    static jfieldID jrate=env->GetFieldID(jglucoseclass,"rate","F");
    jfloat rate=env->GetFloatField(jglucose,jrate);
    glucose gl{(uint32_t)time,value,rate};
    constexpr const int len=sizeof(gl);
    jbyteArray uit = env->NewByteArray(len);
    env->SetByteArrayRegion(uit, 0, len, reinterpret_cast<const jbyte *>(&gl));
    return uit;
    }
*/
//    jbyteArray uit = envin->NewByteArray(uitlen);
 //   envin->SetByteArrayRegion(uit, 0, uitlen, key->data());



