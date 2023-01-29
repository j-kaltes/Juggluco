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


#include <iostream>
#include <string_view>
#include <fstream>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <thread>
#include <future>

#include "jnisub.h"
#ifdef TESTPS
#define abbottdec(x) (*x)
#define abbottcall(x) x
#define WAS_JNIEXPORT 
extern "C" WAS_JNIEXPORT jint JNICALL   abbottdec(P1)(JNIEnv *envin, jobject obj, jint i, jint i2,jbyteArray  bArr,jbyteArray  bArr2) ;
extern "C" WAS_JNIEXPORT jbyteArray JNICALL abbottdec(P2)(JNIEnv *, jobject, jint, jint, jbyteArray, jbyteArray);
#include "pstructs.h"
#endif
using namespace std;
//extern bool abbotttest(std::string_view dir) ;

#include "inout.h"
//extern bool abbotttest(string_view dir,data_t *infoptr,data_t *rawptr,data_t *uidptr,data_t *initinfo,data_t *bluetooth,uint32_t starttijd,uint32_t bluetime,uint32_t tijd) ;

extern  int abbotttest(string_view dir,scandata &data,scandata &data2,data_t *uidptr,data_t *initinfo,data_t *bluetooth,uint32_t starttijd,uint32_t bluetime,const unsigned char *decr,bool nodebug

#ifdef TESTPS
,std::vector<pstruct*> &ptests
#endif
) ;
//extern int abbotttest(string_view dir,scandata &data,scandata &data2,data_t *uidptr,data_t *initinfo,data_t *bluetooth,uint32_t starttijd,uint32_t bluetime,const unsigned char *decr,bool) ;
template <int nr>
struct initdata {
int len=nr;
unsigned char data[nr];
};
initdata<6> initinfo {6,
#include "initinfo.h"
};
initdata<8> uid {8, 
#include "ident.h"
};
//Sun Apr  4 20:03:36  2021 CF 04 70 F6 8A 71 BA 9A A4 CA FE 6B 1A 78 A8 3D D3 90 E1 63 3D AB CF 45 48 10 78 EE 47 4C CB 54 6E 37 84 38 E2 D5 05 CE E8 B1 E7 31 4B 7D
uint32_t bluetime=1617559416,starttijd=1617028422;
initdata<46> bluetooth {46,{
0xCF,0x04,0x70,0xF6,0x8A,0x71,0xBA,0x9A,0xA4,0xCA,0xFE,0x6B,0x1A,0x78,0xA8,0x3D,0xD3,0x90,0xE1,0x63,0x3D,0xAB,0xCF,0x45,0x48,0x10,0x78,0xEE,0x47,0x4C,0xCB,0x54,0x6E,0x37,0x84,0x38,0xE2,0xD5,0x05,0xCE,0xE8,0xB1,0xE7,0x31,0x4B,0x7D }};
data_t *initinfoptr=reinterpret_cast<data_t*>(&initinfo);
data_t *uidptr=reinterpret_cast<data_t*>(&uid);
data_t *bluetoothptr=reinterpret_cast<data_t*>(&bluetooth);
time_t tijd=1617377114;
uint32_t tijd2=1617381122;

void test(const char *filesdir,std::promise<int> * prom,bool nodebug) {
scandata   data(tijd,
#include "rawin.h"
,
#include  "info.h"
);
scandata   data2(tijd2,
#include "rawin2.h"
,
#include  "info2.h"
);
#include "decrypt.h"

#ifdef TESTPS
#include "funcs2.h"

std::vector<pstruct*> ptests{&p1_0,&p2_0,&p1_1,&p2_1,&p1_2};
#endif
int ret=abbotttest(filesdir,data,data2,uidptr,initinfoptr,bluetoothptr,starttijd,bluetime,decrypt,nodebug

#ifdef TESTPS
,ptests
#endif
);

prom->set_value(ret);
}
int main(int argc,char **argv) {
	std::promise<int> prom;
	std::future<int> fut = prom.get_future();
	std::thread tester(test,argv[1],&prom,argc>2);
	tester.join();
	return fut.get();
	}
/*
exit code:
127 can't find program
1 can't find shared library
139: segmentation violation
135: Bus error (core dumped)
100	Not serialized (no valid serialization found or keycode expired)
159	Bad system call (e.g. a system call that is not supported by the kernel)
*/
/*E007-3MH003ZM0QH
Sat Mar 27 18:02:05  2021 0000017346 0000000090 0000000000005 STABLE          0000000000.43
*/


