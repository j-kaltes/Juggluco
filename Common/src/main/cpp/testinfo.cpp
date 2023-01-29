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
uint32_t firstpos;
uint32_t lastpos; //Really last, not one past the last
uint32_t scancount;
uint16_t startid;
uint16_t interval;
uint16_t reserved;
uint8_t dupl;
uint8_t days;
//uint32_t reserved2[2];
uint32_t pollcountwas;
uint32_t lockcountwas;
struct { int len;
	signed char data[8];
	} ident;
struct { int len;
	signed char data[6];
	} info;
uint32_t bluestart;	
union {
	struct { int len;
		signed char data[6];
		uint8_t sensorgen;
		bool reserved2;
		} blueinfo;
	uint8_t streamingAuthenticationData[10];
	};
uint32_t pollcount;
double pollinterval; 
uint32_t lockcount;
//int bluetoothOn;
int8_t bluetoothOn;
uint8_t reserved4[3];
char deviceaddress[deviceaddresslen];
uint16_t libreviewscan;
uint16_t libreviewid;
uint16_t libreviewnotsend:15;
bool putsensor:1;

} ;

using std::cout;
using std::endl;
int main() {
cout<<"streamingAuthenticationData: "<<offsetof(Info,streamingAuthenticationData)<<endl;
cout<<"blueinfo::len: "<<offsetof(Info,blueinfo.len)<<endl;
cout<<"blueinfo::data: "<<offsetof(Info,blueinfo.data)<<endl;
cout<<"pollcount: "<<offsetof(Info,pollcount)<<endl;
cout<<"bluetoothOn: "<<offsetof(Info,bluetoothOn)<<endl;
cout<<"deviceaddress: "<<offsetof(Info,deviceaddress)<<endl;
cout<<"libreviewscan: "<<offsetof(Info,libreviewscan)<<endl;
cout<<"libreviewid: "<<offsetof(Info,libreviewid)<<endl;
}
