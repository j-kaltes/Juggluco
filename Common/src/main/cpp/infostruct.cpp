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
#include "maxsendtohost.h"
using std::cout;
using std::endl;
#include <stdint.h>
struct updatestate {
	uint32_t scanstart;uint32_t histstart;uint32_t streamstart; uint32_t rawstreamstart;
	bool sendbluetoothOn:1;
	bool sendstreaming:1;
	bool changedstreamstart:1;
	bool changedhistorystart:1;
	uint8_t rest:4;
	};
static inline constexpr const int deviceaddresslen=18;
struct Info {
uint32_t starttime;
uint32_t lastscantime;
int32_t starthistory;
int32_t endhistory;  //In reality one past last pos. Previously said wrong.
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
//int streamingIsEnabled;
int8_t streamingIsEnabled;
uint8_t reserved4[3];
char deviceaddress[deviceaddresslen];
uint16_t libreviewscan;
//bool libresendall:1;
uint16_t libreviewid;
uint16_t libreviewnotsend:14;
bool prunedstream:1;
bool putsensor:1;
updatestate update[maxsendtohost];

} ;


int main() {
	cout<<offsetof(Info,libreviewid)<<endl;
	}
