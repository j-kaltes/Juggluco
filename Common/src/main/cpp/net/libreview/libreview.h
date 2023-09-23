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
/*      Fri Jan 27 12:39:13 CET 2023                                                 */


#pragma once
#include <ctime>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <string_view>
#include <jni.h>
#include <array>
#include "SensorGlucoseData.h"
/*#ifndef NDEBUG
//#define  NOREALSEND 1
#define LIBRE3ONLY
#endif */
#ifndef NDEBUG
//#define LIBRE3ONLY 1
//#define  NOREALSEND 1
#endif
constexpr const char *const trendName[]{"Undetermined", "FallingQuickly", "Falling", "Stable", "Rising", "RisingQuickly"};
constexpr inline void	addstrview(char *&uitptr,const std::string_view indata) {
	memcpy(uitptr,indata.data(),indata.size());
	uitptr+=indata.size();
	}
template <class T, size_t N>
inline static constexpr void addar(char *&uitptr,const T (&array)[N]) {
        constexpr const int len=N-1;
        memcpy(uitptr,array,len);
        uitptr+=len;
        }

inline  void submsec(time_t *tim,int mil) {
	if(mil>=500) {
		--*tim;
		}
	}
		
inline int TdatestringGMT(time_t tim,int mil,char *buf) {
	submsec(&tim,mil);
        struct tm tmbuf;
	gmtime_r(&tim, &tmbuf);
        return sprintf(buf,R"(%d-%02d-%02dT%02d:%02d:%02d.%03dZ)",tmbuf.tm_year+1900,tmbuf.tm_mon+1,tmbuf.tm_mday, tmbuf.tm_hour, tmbuf.tm_min,tmbuf.tm_sec,mil);
        }
inline int getmmsec() {
	return rand()%1000;
	}

inline int TdatestringGMT(time_t tim,char *buf) {
	int mil=rand()%1000;
	return TdatestringGMT(tim,mil,buf) ;
        }

//extern int Tdatestringlocal(time_t tim,char *buf) ;

 template<class T>  inline void	addstrcont(char *&uitptr,const T &indata) {
	memcpy(uitptr,indata.data(),indata.size());
	uitptr+=indata.size();
	}
inline int64_t mkhistrecord(const int64_t histor,const uint16_t id) {
	const int64_t uit=histor|id;
	return uit;
	}

inline int Tdatestringlocal(time_t tim,int mil,char *buf) {
         struct tm tmbuf;
	submsec(&tim,mil);
        int seczone=timegm(localtime_r(&tim,&tmbuf)) - tim;
        int m=seczone/60;
        int h=m/60;     
        int minleft=m%60;
        return sprintf(buf,R"(%d-%02d-%02dT%02d:%02d:%02d.%03d%+03d:%02d)",tmbuf.tm_year+1900,tmbuf.tm_mon+1,tmbuf.tm_mday,tmbuf.tm_hour,tmbuf.tm_min,tmbuf.tm_sec,mil,h,minleft);
        }

inline int Tdatestringlocal(time_t tim,char *buf) {
	int mil=rand()%1000;
	return  Tdatestringlocal(tim, mil,buf);
        }

extern bool hour24clock;

extern std::string_view dRELEASE;
extern std::string_view dMANUFACTURER;
extern std::string_view dMODEL;

extern jbyteArray  getlibre3puttext(JNIEnv *env, const char *sensorname) ;
extern jbyteArray getlibre2puttext(JNIEnv *env, const char *sensorname) ;
extern bool putwhenneeded(bool libre3,SensorGlucoseData *sensdata) ;
extern bool	libresendmeasurements(bool libre3,const char *measurements,const int len);

extern bool sendlibre3viewdata(bool,uint32_t);
#include "settings/mixpass.h"

