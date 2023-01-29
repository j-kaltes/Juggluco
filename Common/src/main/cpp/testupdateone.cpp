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


#include <stdint.h>
#include "net/passhost.h"
struct numspan {
	int start;
	int end;
};
constexpr const int maxchanged=100;
constexpr int maxallhosts=8;
typedef uint8_t senddata_t ;
struct changednums {
	int len;
	struct numspan changed[maxchanged];
	senddata_t reserved[32];
	uint32_t lastlastpos;
	void clear() {
		len=1;
		changed[0]={};
		lastlastpos=0;
		}
	};
struct updateone {
//	struct sockaddr host;
	int ind;
	int allindex;
	int startsensors;//vanwaaraf sensors.dat updaten
//	char port[6];
	uint8_t backupupdated;
	uint32_t updatesettings;
	int32_t firstsensor;
	uint32_t starttime;
	uint32_t reserved;
	bool sendnums;
	bool sendstream;
	bool sendscans;
	bool restore;
	uint8_t pass[16];
	struct changednums nums[2];
	};
constexpr const int maxhost=3;
struct updatedata {
int32_t hostnr;
uint32_t receive;
//uint32_t update;
char port[6];
bool hasrestore;
uint8_t sendnr;
passhost_t allhosts[maxallhosts];
updateone tosend[maxhost];

};
#include <stdio.h>
int main() {
printf("%zd\n",offsetof(updatedata,tosend[0]));
printf("%zd\n",offsetof(updatedata,tosend[1]));
printf("%zd\n",offsetof(updatedata,tosend[2]));
printf("%zd\n",offsetof(updatedata,allhosts[0]));
printf("first allhosts %zd\n",offsetof(updatedata,allhosts[0].nr));
printf("%zd\n",offsetof(passhost_t, nr));
printf("%zd\n",offsetof(updatedata,allhosts[1]));
printf("%zd\n",offsetof(updatedata,allhosts[1].nr));
printf("%zd\n",offsetof(updatedata,allhosts[1].index));
printf("start tosend 0: %zd\n",offsetof(updatedata,tosend[0]));
printf("start tosend 1: %zd\n",offsetof(updatedata,tosend[1]));
}
