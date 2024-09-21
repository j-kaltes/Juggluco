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
/*      Fri Jan 27 12:38:18 CET 2023                                                 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>
       #include <unistd.h>
#include "netstuff.hpp"
#include "destruct.hpp"
#include "logs.hpp"

namehost myip (){
  if(int socketfd = socket(AF_INET, SOCK_STREAM, 0);socketfd >= 0) {
    destruct dest([socketfd]{close(socketfd);});
    struct    ifreq data[50];
    struct ifconf conf{.ifc_len = sizeof(data),.ifc_ifcu{.ifcu_req = data}};
    if (ioctl(socketfd,SIOCGIFCONF,&conf) < 0) {
       lerror("ioctl(,SIOCGIFCONF,)");
       return namehost();
    	}
    const	int nr= (conf.ifc_len /sizeof(struct ifreq));
    for(int in=0;in<nr;in++) {
    	struct ifreq *ifr=data+in;
	namehost host( &ifr->ifr_addr);
	if (ioctl(socketfd, SIOCGIFFLAGS, (char *) ifr) < 0) {
			lerror("ioctl(SIOCGIFFLAGS )");
			continue;
			}
	if(! (ifr->ifr_flags & IFF_POINTOPOINT) && !(ifr->ifr_flags & IFF_LOOPBACK) &&(ifr->ifr_flags & IFF_UP)&& (ifr->ifr_flags & IFF_RUNNING)  ) {
	//		fprintf(stderr,"myip %s\n",host.data());
			return host;	
			}
      }
    }

  else {
    lerror("socket");
  }
return namehost();
}

#if 0
int main() {
namehost host=getip();
printf("%s\n",host.data());
	}
#endif
