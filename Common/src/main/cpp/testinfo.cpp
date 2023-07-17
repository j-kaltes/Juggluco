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
#include <stdint.h>
#include "maxsendtohost.h"
static inline constexpr const int deviceaddresslen=18;

struct Info {
uint32_t starttime;
uint32_t lastscantime;
int32_t starthistory;
int32_t endhistory;  //In reality one past last pos. Previously said wrong.
uint32_t scancount;
uint16_t startid;
uint16_t interval;
uint16_t pollstart;
uint8_t dupl:7;
bool uselater:1;
uint8_t days:7;
bool auth12removed:1;
//uint32_t reserved2[2];
uint32_t pin;
uint16_t lastLifeCountReceived;
uint16_t lastHistoricLifeCountReceivedPos;
struct { int len;
	signed char data[8];
	} ident;
struct { 
	int len;
	signed char data[6];
	} info;
uint32_t bluestart;	
union {
	struct { int len;
		signed char data[6];
		uint8_t sensorgenDONTUSE;
		bool reserved2;
		} blueinfo;
	uint8_t streamingAuthenticationData[12];
	};
uint32_t pollcount;
double pollinterval; 
uint32_t lockcount;
int8_t streamingIsEnabled;
int8_t patchState;
uint16_t reserved4:15;
bool auth12:1;
char deviceaddress[deviceaddresslen];
uint16_t libreviewscan;
uint8_t authlendontuse;

uint8_t reserved:6;

bool sendsensorstart:1;
bool libreviewsendall:1;
uint16_t libreviewnotsend:14;
bool prunedstream:1;
bool putsensor:1;
} ;


using std::cout;
using std::endl;
int main() {
cout<<"streamingAuthenticationData: "<<offsetof(Info,streamingAuthenticationData)<<endl;
cout<<"blueinfo::len: "<<offsetof(Info,blueinfo.len)<<endl;
cout<<"blueinfo::data: "<<offsetof(Info,blueinfo.data)<<endl;
cout<<"pollcount: "<<offsetof(Info,pollcount)<<endl;
cout<<"streamingIsEnabled: "<<offsetof(Info,streamingIsEnabled)<<endl;
cout<<"deviceaddress: "<<offsetof(Info,deviceaddress)<<endl;
cout<<"libreviewscan: "<<offsetof(Info,libreviewscan)<<endl;
//cout<<"libreviewid: "<<offsetof(Info,libreviewid)<<endl;
}
