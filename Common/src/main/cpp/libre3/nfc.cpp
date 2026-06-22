#ifdef LIBRE3
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
/*      Fri Jan 27 15:22:01 CET 2023                                                 */

#include <jni.h>
#include <ctime>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include "fromjava.h"
#include "logs.hpp"
#include "libre2.hpp"
#include "destruct.hpp"
#include "sensoren.hpp"
#include "SensorGlucoseData.hpp"
#include "hexstr.hpp"
//00A500010001000000C04E1E0D0101040C043036385A4D524631382F97
//00A500010001000000C04E1E0D0101040C013036385A4D52463138164A
extern Sensoren *sensors;
struct firstnfc {
//	uint8_t zero; uint8_t start[2];
	uint16_t sec_version;
	uint16_t localization;
	uint16_t puckgen;
	uint16_t  wearduration;
	uint32_t  firmwareversion;
	uint8_t producttype;
	uint8_t warmup;
	uint8_t state;
	char serialnumber[9];
	uint8_t crc16[2];
	}  __attribute__ ((packed));

struct nfc1 {
    uint8_t nfcbuf[50+sizeof(firstnfc)];
	firstnfc *nfcptr;
	bool error;
	const std::string_view getSerialNumber() const {
 		return std::string_view(nfcptr->serialnumber,9);
		}
	nfc1(JNIEnv *env,  jbyteArray jnfcout) {
        if(!jnfcout) {
			LOGAR("NFC: firstnfc==null");
            error=true;
            return;
            }
        jsize lens=env->GetArrayLength(jnfcout);
		if(lens<(sizeof(firstnfc)+3)) {
			LOGGER("NFC: sizeof bytearray=%d sizeof(firstnfc)=%ld\n",lens,sizeof(firstnfc));
			error=true;
			return;
			}
		env->GetByteArrayRegion(jnfcout, 0, lens, reinterpret_cast<jbyte*>(nfcbuf));
#ifndef NOLOG
{
		hexstr hex((uint8_t*)nfcbuf,lens);
		LOGGERN(hex.str(),hex.size());
		}
#endif
       auto *iter=nfcbuf+1;
       while(*iter++==0xa5)
           ;
        nfcptr=reinterpret_cast<firstnfc*>(iter);
		nfcptr->crc16[0]=0;
		LOGGER("serialnumber=%s state=%d warmup=%d*5 wearduration=%d  sec_version=%hx localization=%hx puckgen=%hx firmwareversion=%x producttype=%hhx\n",nfcptr->serialnumber,nfcptr->state,nfcptr->warmup,nfcptr->wearduration,
nfcptr->sec_version,
nfcptr->localization,
nfcptr->puckgen,
nfcptr->firmwareversion,
nfcptr->producttype
        );
		};
	};

static_assert(sizeof(firstnfc)==26);






/*
extern "C" JNIEXPORT  jlong JNICALL fromjava(interpret3NFC1)(JNIEnv *env, jclass thiz, jbyteArray jnfcout) {
	if(!jnfcout)  {
		LOGSTRING("interpret3NFC1(null)\n");
		return nullptr;
		}
         jsize lens=env->GetArrayLength(jnfcout);
	 jbyte nfcout=new jbyte[lens];
        env->GetByteArrayRegion(jnfcout, 0, lens, nfcout);
	hexstr hex((uint8_t*)nfcout,lens);
	LOGGERN(hex.str(),hex.size());
	if(lens!=sizeof(firstnfc)) {
		LOGGER("NFC: sizeof bytearray=%d sizeof(firstnfc)=%d\n",lens,sizeof(firstnfc));
		delete[] nfcout;
		return nullptr;
		}
	firstnfc *nfc=reinterpret_cast<firstnfc*>(nfcout);
    const uint16_t start=0xa5;
    if(*reinterpret_cast<const uint16_t*>(nfc->start)!=start) {
            LOGSTRING("NFC: doesn't start with 0xA5,0x0\n");
	   delete[] nfcout;
           return nullptr;
    	}
	nfc->crc16[0]=0;

	LOGGER("serialnumber=%s state=%d wearduration=%d\n",nfc->serialnumber,nfc->state,nfc->wearduration);
       return nfcout;
	}
*/
struct  nfc2{
	uint8_t zero;
	uint8_t response[2];
	uint8_t deviceAddress[6];
	uint32_t pin;
	uint32_t activationTime;
	uint8_t crc16[2];
	} __attribute__ ((packed));
//A501B0
struct  nfc2error{
	uint8_t zero;
	uint16_t recogn;
	uint8_t error;
	} __attribute__ ((packed));
/*

rx: 00A500111DB71932189A948F6E9F0A7562A2FA                            
rx: 00A500111DB71932189A948F6E9F0A7562A2FA
NFC: Switch receiver response: 00A500
11 1D B7 19 32 18
9A948F6E
9F0A7562 == bigendian A_UTC = 1651837599
A2FA
Activation Response
BD_Addr = 11 1D B7 19 32 18
BLE_Key(BLE_Pin) = 9A948F6E
A_UTC = 1651837599
APP_CRC16 = A2 FA

NFC: Patch activated at: Fri May 06 13:46:39 GMT+02:00 2022, BLE Pin 9A948F6E

 BLE: Using scan identifier: 18:32:19:B7:1D:11, BLE Pin: 9A948F6E
*/
int mkdeviceaddressstr(char *outbuf,const uint8_t *inbuf) {
	return sprintf(outbuf,"%02X:%02X:%02X:%02X:%02X:%02X",inbuf[5], inbuf[4], inbuf[3], inbuf[2], inbuf[1], inbuf[0]);
	}

constexpr const auto dayseconds=24*60*60;                   
extern "C" JNIEXPORT jlong JNICALL fromjava(getLibre3secs)(JNIEnv *env, jclass thiz, jbyteArray  nfc1ar) {
	nfc1 first(env,nfc1ar);
	if(first.error) {
      LOGAR("getLibre3secs error");
		return 0LL;
		}
 	const auto  name=Sensoren::namelibre3(std::string_view(first.nfcptr->serialnumber,9));
	time_t nu=time(nullptr);
	if(const uint32_t start=sensors->sensorStarttime(name.data())) {
		if((nu-start)>	13*dayseconds) {
			nu=start+12*dayseconds;
			}
		}
	LOGGER("getLibre3secs now=%lu %s",nu,ctime(&nu));
	return static_cast<jlong>(nu);
	}

extern void	sendstreaming(SensorGlucoseData *hist);


static void finishsensor(const nfc1 &first) {
	auto namesensor=Sensoren::namelibre3(first.getSerialNumber());

	const char *name= namesensor.data();
	const int sensorindex=sensors->sensorindex(name);
	if(sensorindex>=0) {
		sensors->finishsensor(sensorindex);
		LOGGER("finishsensor %.16s %d\n",name,sensorindex);
		if(SensorGlucoseData *sens=sensors->getSensorData(sensorindex) ) {
extern void	setstreaming(SensorGlucoseData *hist) ;
			setstreaming(sens); 
extern					void setusedsensors() ;
			setusedsensors();
			}
		}
	else {
		LOGGER("can't find %.16s sensorindex=%d\n",name,sensorindex);
		}
	}
//char scannedsensorname[10]{};
extern "C" JNIEXPORT jlong JNICALL fromjava(interpret3NFC2)(JNIEnv *env, jclass thiz, jbyteArray  nfc1ar,jbyteArray jnfcout,jlong now) {
	nfc1 first(env,nfc1ar);
	if(first.error) {
        LOGAR("interpret3NFC2 error");
		return 0LL;
		}
	if(!jnfcout)  {
		LOGSTRING("interpret3NFC2(null)\n");
		return 0LL;
		}
        jsize lens=env->GetArrayLength(jnfcout);
	jbyte nfcout[lens];
        env->GetByteArrayRegion(jnfcout, 0, lens, nfcout);
#ifndef NOLOG 
	hexstr hex((uint8_t*)nfcout,lens);
	LOGGERN(hex.str(),hex.size());
#endif
#if  defined(TEST) && !defined(NOLOG)
	char devaddress[18]="30:53:C1:6C:BE:5F";
	time_t acttime= now;
   uint32_t pin=0xFFFFFFFFF;
//	int sensindex=sensors->makelibre3sensorindex(std::string_view(first.nfc.serialnumber,9),acttime,pin,devaddress,now,first.nfc.warmup*5,first.nfc.wearduration);
#else
	if(lens!=sizeof(nfc2)) {
		if(lens==sizeof(nfc2error)) {
			const nfc2error *nfc=reinterpret_cast<const nfc2error*>(nfcout);
			const uint16_t error=0x01A5;
			if(nfc->recogn==error) {
		//		memcpy(scannedsensorname,first.nfc.serialnumber,9);
				LOGGER("NFC: error %x\n",nfc->error);
				if(nfc->error==0xb1)
					return 1LL;
				else  {
					switch(first.nfcptr->state) {
						case 8: finishsensor(first);return 5LL;
						case 6: finishsensor(first);return 6LL;
						default: return 3LL;
						}
					}
				}
			LOGGER("NFC: error regn %hd instead of %hd\n",nfc->recogn,error);
			return 0LL;	
			}
		LOGGER("NFC: sizeof bytearray=%d sizeof(nfc2)=%ld\n",lens,sizeof(nfc2));
		return 0LL;
		}

	const nfc2 *nfc=reinterpret_cast<const nfc2*>(nfcout);
	char devaddress[18];
	mkdeviceaddressstr(devaddress,nfc->deviceAddress);
	time_t acttime= nfc->activationTime;
	if(acttime<=0) {
		acttime=now;
//		time(&acttime);
		}
#ifndef NOLOG	
	hexstr pinhex((uint8_t*)&nfc->pin,4);
	LOGGER("deviceaddress=%s pin=%s response=%02hhx%02hhx activation: %s",devaddress,pinhex.str(),nfc->response[0],nfc->response[1],ctime(&acttime));
#endif
  const auto pin=nfc->pin;
#endif
	int sensindex=sensors->makelibre3sensorindex(std::string_view(first.nfcptr->serialnumber,9),acttime,pin,devaddress,now,first.nfcptr->warmup*5,first.nfcptr->wearduration);
	SensorGlucoseData *sens=sensors->getSensorData(sensindex);
	sendstreaming(sens); 
	libre3stream *streamd=new libre3stream(sensindex,sens);
	return reinterpret_cast<jlong>(streamd);
	}


#include "dp_activation.hpp"
#include "jniclass.hpp"
extern "C" JNIEXPORT jint JNICALL fromjava(startTimeIDsum)(JNIEnv *env, jclass cl, jbyteArray bArr, jlong time, jlong account) {

	LOGGER("startTimeIDsum(bArr,%" PRId64 ",%" PRId64 ")\n",time,account);

    const auto arlen=env->GetArrayLength(bArr);
    if(arlen!=10) {
        LOGGER("DPGetActivationCommandData called with size=%d\n",arlen);
        }
    CritArSave<std::byte>  bluedata(env,bArr);
    
    dp::DPGetActivationCommandData(std::span<std::byte, 10>(bluedata.data(),10) , time, account);

	return 0;
	}


#endif

