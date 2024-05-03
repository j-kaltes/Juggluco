
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
/*      Thu Apr 04 17:28:08 CEST 2024                                                 */


#include <array>
#include <array>
#include <string_view>
#include <iostream>
#include <stdio.h>
#include <stdint.h>
#include <type_traits>
#include "contconcat.h"


static int8_t oneel(const char *start) {
	return strtol(start,nullptr,16);
	}
static auto deviceArray(std::string_view address) {
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
        auto AddArr=deviceArray(address);
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

#include "SensorGlucoseData.h"
#include "libre2.h" //PUT sistream etc in different header?
#include "fromjava.h"
#include "share/hexstr.h"
extern "C" JNIEXPORT jbyteArray JNICALL   fromjava(getSiWriteCharacter)(JNIEnv *env, jclass cl,jlong dataptr) {
	if(!dataptr)
		return nullptr;
	const SensorGlucoseData *usedhist=reinterpret_cast<streamdata *>(dataptr)->hist ; 
	if(!usedhist)
		return nullptr;
	const int index=usedhist->getSiIndex();
	const auto address=usedhist->deviceaddress();
   const auto codes=makeWriteCharacter(index,address);
   const auto *data=codes.data();
	const int len=codes.size();
   hexstr hex((const uint8_t *)data,len);
   LOGGER("getSiWriteCharacter(index=%d)=%s\n",index,hex.str());
	jbyteArray uit=env->NewByteArray(len);
	env->SetByteArrayRegion(uit, 0, len,data);
	return uit;
	}
#endif
#endif
