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

//#define TESThistor
#ifdef TESThistor
#define NOLOGS_H 1
#endif

#include <iostream>
#include <string.h>
#include <assert.h>
#include "inout.hpp"
#include "serial.hpp"
using std::array;
#include "hex.hpp"
/*
static unsigned char dehex(char ch) { return ch<='9'?ch-'0':(ch-'A'+10);};

	
static unsigned char hexnum(char h,char l) {
	return dehex(h)<<4|dehex(l);
	}
static char showhexel(int deel) {return deel<=9?'0'+deel:deel-10+'A';};

template <int part> static char showhex(unsigned char ch){
	return showhexel(ch>>4);
	}
 template<>  char showhex<0>(unsigned char ch) {
	return showhexel(ch&0xF);
	}
	*/
static constexpr const char selnum[] = "0123456789ACDEFGHJKLMNPQRTUVWXYZ";



string getserial(int fam, const unsigned char *byte) {
	LOGGER("Get serial %d\n",fam);
        return string(
	{
	showhex<1>(byte[7]),
	showhex<0>(byte[7]),
	showhex<1>(byte[6]),
	showhex<0>(byte[6]),
	'-',	
	selnum[fam],
        selnum[(byte[5] >> 3)],
        selnum[((byte[4] >> 6) | ((byte[5] & 7) << 2))],
        selnum[((byte[4] >> 1) & 31)],
        selnum[(((byte[4] & 1) << 4) | (byte[3] >> 4))],
        selnum[(((byte[3] & 15) << 1) | (byte[2] >> 7))],
        selnum[((byte[2] >> 2) & 31)],
        selnum[(((byte[2] & 3) << 3) | (byte[1] >> 5))],
        selnum[(byte[1] & 31)],
        selnum[(byte[0] >> 3)],
        selnum[((byte[0] << 2) & 31)]});
        }

string  encodeStatusCode(int64_t  code) {
        return {selnum[(code >> 0) & 31],
        selnum[ (code >> 5) & 31],
        selnum[ (code >> 10) & 31],
        selnum[ (code >> 15) & 31],
        selnum[ (code >> 20) & 31],
        selnum[ (code >> 25) & 31],
        selnum[ (code >> 30) & 31],
        selnum[ (code >> 35) & 31],
        selnum[ (code >> 40) & 31],
        selnum[ (code >> 45) & 31]};
    }

//array<unsigned char,8> unserial(string_view str) {
array<unsigned char,8> unserial(const char * const str) {
	array<unsigned char,8> bytuit;
	constexpr int len=11;
	unsigned char bytin[len];
	const char *serial=str+5;
	for(int i=0;i<len;i++)
		bytin[i]=unalf(serial[i]);

	bytuit[7]=hexnum(str[0],str[1]);
	bytuit[6]=hexnum(str[2],str[3]);
	bytuit[5]=bytin[1]<<3|((bytin[2]>>2)&7);
	bytuit[4]=bytin[3]<<1|bytin[2]<<6|((bytin[4]>>4)&1);
	bytuit[3]= ((bytin[4]&0xF)<<4)|((bytin[5]>>1)&0xF);
	bytuit[2]=bytin[5]<<7| (bytin[6]&31)<<2| ((bytin[7]>>3)&3);
	bytuit[1]=(bytin[7]&7)<<5| bytin[8];
	bytuit[0]=bytin[9]<<3| bytin[10]>>2;
	return bytuit;
	}

//#define MAIN
#ifdef MAIN
using std::cout;
using std::endl;
#if 1
pathconcat logbasedir("/tmp/logs");
int main(int argc,char **argv) {
mkdir(logbasedir.data(),0777);
for(int i=1;i<argc;i++) {
	Readall<char> inp(argv[i]);
	string ser{getserial(0,(unsigned char *)((char *)inp))};
	cout<<argv[i]<<" " <<ser<<endl;
	}
}
#else
#include <random>
int main() {
 std::random_device rd;  
    std::mt19937 gen(rd()); 
    std::uniform_int_distribution<unsigned long long> distrib(0, std::numeric_limits<unsigned long long>::max());
unsigned long long get;
int maxloop= std::numeric_limits<int>::max();
int diffs=0;
for(int i=0;i<maxloop;i++) {
	get=distrib(gen);
	unsigned char *byt=reinterpret_cast<unsigned char *>(&get);
	string ser{getserial(0,byt)};
	auto back=unserial(ser);
	if(memcmp(byt,back.data(),8)) {
		diffs++;
		cout<<get<<" ";
		}
	}
cout<<"tested "<<maxloop<<" times\n";
cout<<"found "<<diffs<<" differences\n";
}
#endif
#endif
/*
typedef std::array<char,11>  sensorname_t;
//#include "gltype.hpp"
template<class T>
int64_t libreviewHistorAlg(const T *sensorid,int start, int shift) {
        int64_t  uit = 0;
        const char *str=sensorid->data();
        const int len= sensorid->size();
        for(int it = start; it <len ; it++) {
           uit = (uit << 5) | unalf(str[it]);
           }
        return  (uit << shift);
    }
template<class T>
int64_t libreviewHistor3(const T *sensorid) {
        return libreviewHistorAlg(sensorid,0,19) ;
        }
int64_t libreviewHistor(const sensorname_t *sensorid) {
        return libreviewHistorAlg(sensorid,1,14) ;
    }

int64_t libreviewHistor(const sensorname_t *sensorid) {
        int64_t  uit = 0;
	const char *str=sensorid->data();
	const int len= sensorid->size();
        for(int it = 1; it <len ; it++) {
           uit = (uit << 5) | unalf(str[it]);
           }
        return  (uit << 14);
    }
*/
#ifdef TESThistor
#include <cinttypes>
typedef std::array<char,11>  sensorname_t;
template<class T>
int64_t libreviewHistorAlg(const T *sensorid,int start, int shift) {
        int64_t  uit = 0;
        const char *str=sensorid->data();
        const int len= sensorid->size();
        for(int it = start; it <len ; it++) {
           uit = (uit << 5) | unalf(str[it]);
           }
        return  (uit << shift);
    }
template<class T>
int64_t libreviewHistor3(const T *sensorid) {
        return libreviewHistorAlg(sensorid,0,19) ;
        }

int64_t libreviewHistor(const sensorname_t *sensorid) {
        return libreviewHistorAlg(sensorid,1,14) ;
    }

int64_t createUniqueReadingIdentifier(const sensorname_t *sensorid,int id) {
	return  libreviewHistor(sensorid)|id ;
	}
static int getcharpos(const char ch) {
	const char *endpos=std::end(selnum)-1;
	const char *found=std::lower_bound(std::begin(selnum), endpos, ch );
	assert(found!=endpos);
	return found-selnum;	
	} 
int main(int argc,char **argv) {
	printf("%s\n",selnum);
	int len=std::size(selnum)-1;
	for(int i=0;i<len;i++) {
		printf("%c-%d %d\n",selnum[i],getcharpos(selnum[i]),unalf(selnum[i]));
		}
	const sensorname_t *sensorid=reinterpret_cast<const sensorname_t *>(argv[1]);
	printf("%" PRId64 "\n",createUniqueReadingIdentifier(sensorid,atoi(argv[2])));
	}

#endif
//#define UNSERIAL
#ifdef UNSERIAL
int main(int argc,char **argv) {
	auto uid=unserial(argv[1]);
	for(auto el:uid) {
		printf("%02X ",el);
		}
	puts("");
	}
#endif
