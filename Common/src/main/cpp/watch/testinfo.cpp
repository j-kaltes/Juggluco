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
/*      Fri Jan 27 15:22:27 CET 2023                                                 */


#include <sys/types.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stddef.h>
struct netinfo {
	struct sockaddr_in6 ip;
	bool watchsensor;
	char label[17];
	uint8_t extra[2];
	};


struct netinfo1 {
	union {
		struct sockaddr_in6 ip;
		char newlabel[17];
		};
	bool watchsensor;
//	uint8_t version:7;
	union {
		struct sockaddr_in6 ips[3];
		char label[17];
		};
	int nr;
	};

int main() {
printf("sizeof(netinfo)=%ld\n", sizeof(netinfo));
printf("offsetof(netinfo,ip)=%ld\n", offsetof(netinfo,ip));
printf("offsetof(netinfo1,ip)=%ld\n", offsetof(netinfo1,ip));
printf("offsetof(netinfo,label)=%ld\n", offsetof(netinfo,label));
printf("offsetof(netinfo1,label)=%ld\n", offsetof(netinfo1,label));
printf("offsetof(netinfo1,ips)=%ld\n", offsetof(netinfo1,ips));
printf("offsetof(netinfo,watchsensor)=%ld\n", offsetof(netinfo,watchsensor));
printf("offsetof(netinfo1,watchsensor)=%ld\n", offsetof(netinfo1,watchsensor));

}
