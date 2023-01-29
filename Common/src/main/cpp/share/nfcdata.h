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
/*      Fri Jan 27 12:37:55 CET 2023                                                 */


#pragma once
#include <stdint.h>
 #include <time.h>
 #include <string_view>
class History;
struct Glucose_t {
	const uint16_t value:14;
	const uint16_t two:2;
	const uint16_t rest[2];
	} __attribute__ ((packed));
struct nfcpatch {
	const uint8_t start[26];
	const uint8_t trendindex,historyindex;
	const Glucose_t trend[16];
	const Glucose_t history[32];
	const uint16_t age;
	} __attribute__ ((packed));

static_assert(offsetof(nfcpatch,trendindex)==26);
static_assert(offsetof(nfcpatch,historyindex)==27);
static_assert(offsetof(nfcpatch,trend)==28);
static_assert(offsetof(nfcpatch,history)==124);
static_assert(offsetof(nfcpatch,age)==316);
#define makestruct(typ) struct typ {\
     static constexpr        int num = sizeof(nfcpatch::typ)/sizeof(nfcpatch::typ[0]);\
     static constexpr const uint8_t nfcpatch::*index = &nfcpatch::typ##index;\
     static constexpr const Glucose_t (nfcpatch::*values)[num] = &nfcpatch::typ;\
	};

makestruct(trend)
makestruct(history)
static_assert(trend::num==16);
static_assert(history::num==32);
class nfcdata {
friend	class History;
private:
static const int sensorInitializationInMinutes = 60;
static const int historyIntervalInMinutes = 15;

protected:
   nfcpatch *databuf;
public:
    time_t utime;

template<class D> uint16_t getglucose(const int counter) const  {
	return (databuf->*D::values)[ (databuf->*D::index + counter) % D::num].value;
	   }

#define makefunc(typ) uint16_t get##typ##glucose(const int counter) const { return getglucose<typ>(counter); }

makefunc(trend)
makefunc(history)

#define loghier(...)
    auto *getdatabuf() {
	    return databuf;
    	}

template <class Dat,class T=uint16_t> int lowest() const {
	static_assert(sizeof(T)==2);
	int low=-1;
	auto mini=std::numeric_limits<T>::max();
	for(int i=0;i<Dat::num;i++) {
		auto val=getglucose<Dat>(i);
		if(val&&val<mini) {
			mini=val;
			low=i;
			}
		}
	return low;
	  }
template <class Dat,class T=uint16_t> int highest() const {
	static_assert(sizeof(T)==2);
	int high=-1;
	T maxi=0;
	for(int i=0;i<Dat::num;i++) {
		auto val=getglucose<Dat>(i);
		if(val>maxi)  {
			maxi=val;
			high=i;
			}
		}
	return high;
	  }
template <class Dat,class T=uint16_t> inline float getaverage() const {
	static_assert(sizeof(T)==2);
	float tot=0.0;
	int hasval=0;
	for(int i=0;i<Dat::num;i++) {
		auto val=getglucose<Dat>(i);
		if(val)  {
			tot+=val;
			hasval++;
			}
		}
	return tot/hasval;
	  }


template <class T>
	nfcdata(T *data,time_t tim=time(NULL)):  databuf(reinterpret_cast<nfcpatch*>(data)), utime(tim) { 
	       static_assert(sizeof(T)==1, "sizeof data[0] must by 1");
	}

nfcdata() {}
template <class T>
void setdata(T *data) {
        static_assert(sizeof(T)==1, "sizeof data[0] must by 1");
	databuf=reinterpret_cast<nfcpatch*>(data);
	 }
virtual ~nfcdata() {}


inline      int getSensorAgeInMinutes() const {
        return databuf->age;
    }
time_t getStartDate() const {
	return (utime/60-getSensorAgeInMinutes())*60;
	}
inline int getSensorReadyInMinutes() const {
	int left=sensorInitializationInMinutes - getSensorAgeInMinutes();
        return left<0?0:left;
    }

inline     time_t gettime() const {
        return utime;
    }

   inline  void settime(time_t tim) {
    	utime=tim;
    }

void showhistory() ;
//void savehistory(History *hist) ;
void showgegs() ;
bool testdata() ;
};

extern time_t totime_t(std::string_view inp) ;

extern int timestring(time_t tim,char *buf,int max) ;
