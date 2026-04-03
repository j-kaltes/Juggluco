
#ifdef SIBIONICS
#include "config.h"
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
/*      Thu Apr 04 17:28:08 CEST 2024                                                 */


#include <array>
#include <string_view>
#include <iostream>
#include <stdio.h>
#include <stdint.h>
#include <type_traits>
#include "contconcat.hpp"

#include "SensorGlucoseData.hpp"
#include "streamdata.hpp" 
#include "fromjava.h"
#include "share/hexstr.hpp"

/*
static int8_t oneel(const char *start) {
	return strtol(start,nullptr,16);
	}
auto deviceArray(std::string_view address) {
	std::array<int8_t,6>  ar{};
	int pos=address.find_last_of(':',-1)+1;
	const char *start=address.data();
	for(auto &el:ar) {
		el=oneel(start+pos);
		if(pos==0)
			break;
		pos=std::string_view(start,pos-1).find_last_of(':',-1)+1;
		}
	return ar;
	} */

std::array<int8_t,6>  deviceArray(const char address[]) {
   std::array<int8_t,6> uitar;
   auto *uit=uitar.data();
   sscanf(address,"%hhX:%hhX:%hhX:%hhX:%hhX:%hhX",uit+5,uit+4,uit+3,uit+2,uit+1,uit);
   return uitar;
   }


template <typename T1,typename T2,std::size_t N> T1 sum(T2 (&vect)[N],T1 start) {
     for(auto el:vect) start+=el;
     return start;
     }
template <typename T1,typename T2> T1 sum(T2 vect,T1 start) {
     for(auto el:vect) start+=el;
     return start;
     }

static auto makeWriteCharacter(int index, std::string_view address) {
        std::array<int8_t,2> &indexAr = *reinterpret_cast<array<int8_t,2>*>(&index);
        auto AddArr=deviceArray(address.data());
         int8_t startAr[] = {-86, 85, 7};
	int8_t zeros[] = {0, 0, 0, 0,0, 0, 0, 0};
        const auto su = sum(AddArr, sum(indexAr, sum(startAr,0))) ;
        std::array<int8_t,1> checksum= {(int8_t) (((int8_t) (~(su & 0xFF))) + 1)};
        return concat(startAr,indexAr,AddArr,zeros,checksum);
    }
#ifdef MAIN
int  main(int argc,char **argv) {
   int get=34304;
   if(argc>1)
      get=atoi(argv[1]);

   auto res=makeWriteCharacter(get,"E1:54:53:09:27:43");
   for(auto el:res) {
      printf("%02X ",(uint8_t)el);
      }
     puts("");
   } 
#else


extern Data_t askindexdata(jlong index) ;
extern "C" JNIEXPORT jboolean JNICALL   fromjava(siNotchinese)(JNIEnv *env, jclass cl,jlong dataptr) {
	const SensorGlucoseData *usedhist=reinterpret_cast<streamdata *>(dataptr)->hist ; 
	if(!usedhist)
		return false;
   if(usedhist==Sensoren::isdeleted)  {
        LOGAR("siNotchinese isdeleted");
        return false;
        }
	return usedhist->notchinese();
   }
   
extern "C" JNIEXPORT jbyteArray JNICALL   fromjava(siAsknewdata)(JNIEnv *env, jclass cl,jlong dataptr) {
	if(!dataptr)
		return nullptr;
	const SensorGlucoseData *usedhist=reinterpret_cast<streamdata *>(dataptr)->hist ; 
	if(!usedhist)
		return nullptr;
	const int index=usedhist->getSiIndex();
#ifdef NOTCHINESE
	if(usedhist->notchinese()) {
		auto dat=askindexdata(index);
		int len=dat.used;
		jbyte *data=(jbyte*)dat.data->data();
		jbyteArray uit=env->NewByteArray(len);
		env->SetByteArrayRegion(uit, 0, len,data);
		#ifndef NOLOG
		hexstr hex((const uint8_t *)data,len);
		LOGGER("siAsknewdata(index=%d)=%s\n",index,hex.str());
		#endif
		return uit;
		}
	else 
#endif
   {
	   const auto address=usedhist->deviceaddress();
	   const auto codes=makeWriteCharacter(index,address);
	   const auto *data=codes.data();
	   const int len=codes.size();
	   #ifndef NOLOG
	   hexstr hex((const uint8_t *)data,len);
	   LOGGER("siAsknewdata(index=%d)=%s\n",index,hex.str());
	   #endif
		jbyteArray uit=env->NewByteArray(len);
		env->SetByteArrayRegion(uit, 0, len,data);
		return uit;
	    }
	}
#endif
#endif
