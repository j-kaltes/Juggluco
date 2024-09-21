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


 #pragma once
 #include <sys/types.h> 
        #include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include "logs.hpp"       

constexpr const uint64_t zeros[]={0,0,0,0,0,0,0,0,0,0,0,0};
class namehost {
char name[INET6_ADDRSTRLEN];
public:
namehost(const struct sockaddr_in6  *in) {
#ifdef __ANDROID__
	if(!memcmp(in->sin6_addr.in6_u.u6_addr8,zeros,10)) {
		inet_ntop(AF_INET ,in->sin6_addr.in6_u.u6_addr32+3,name, sizeof name);
		}
#else
	if(!memcmp(in->sin6_addr.__in6_u.__u6_addr8,zeros,10)) {
		inet_ntop(AF_INET ,in->sin6_addr.__in6_u.__u6_addr32+3,name, sizeof name);
		}
#endif
  	else  {
		if(getnameinfo(( struct sockaddr *)in,sizeof(*in),name,sizeof(name),nullptr,0,NI_NUMERICHOST))
			strcpy(name,"?");
		}
	}
namehost(const struct sockaddr *sa) {
	const void *addr=(sa->sa_family == AF_INET)?reinterpret_cast<const void *>( &(((const struct sockaddr_in*)sa)->sin_addr)):reinterpret_cast<const void *>( &(((const struct sockaddr_in6*)sa)->sin6_addr));
	inet_ntop(sa->sa_family ,addr ,name, sizeof name);
	}
namehost(const struct in6_addr  *addr) {
	inet_ntop(AF_INET6 ,addr ,name, sizeof name);
	}
namehost() {
	name[0]='\0';
	}
operator const char*() const {
	return name;
	}
const char *data() const {
	return name;
	}
 char *data()  {
		return name;
	}
int size() const {
	return strlen(name);
	}

};
#ifdef __ANDROID__
constexpr const struct in6_addr  noaddress {.in6_u{.u6_addr16{ 0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff }}};
#else
constexpr const struct in6_addr  noaddress {.__in6_u{.__u6_addr16{ 0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff }}};
#endif

extern bool  getaddr(const struct sockaddr *sa, struct sockaddr_in6  *uit) ;
extern bool   getaddr(const char *host, const char *port, struct sockaddr_in6  *uit) ;
inline bool invalid( struct in6_addr &addr) {
	return memcmp(&addr,&noaddress,16)==0;
	}
	/*
inline int compare(const struct sockaddr &one,const struct sockaddr &two) {
	return memcmp(&one,&two,sizeof(one));
	}*/
/*
struct sockaddr_in {
   short int            sin_family;
   unsigned short int   sin_port;
   struct in_addr       sin_addr;
   unsigned char        sin_zero[8];
};
*/
constexpr const struct in6_addr v426(const struct sockaddr *sa) {
return
#define SPECV4 0xffff0000
#ifdef __ANDROID__
			{.in6_u{.u6_addr32{0,0,SPECV4, (((const struct sockaddr_in*)sa)->sin_addr).s_addr}}}
#else
			 {.__in6_u{.__u6_addr32{0,0,SPECV4, (((const struct sockaddr_in*)sa)->sin_addr).s_addr}}}
#endif
;
	}
#include "misc.h"

namehost myip();

inline ssize_t  sendni(int sockfd, const void *buf, size_t len) {
 	int waslen;
	while((waslen=send(sockfd,buf,len,0))==-1) {
		if(errno!=EINTR)
			return -1;
		LOGGER("sendni retry %zd\n",len);
		}
	return waslen;
	}

inline ssize_t  recvni(int sockfd, void *buf, size_t len) {
 	int waslen;
	int inter=0;
	while((waslen=recv(sockfd,buf,len,0))==-1) {
		int erwas=errno;
		flerror("recv(%d,buf,%zd)",sockfd,len);
		if(erwas!=EINTR||inter>20)
			return -1;
		++inter;
		}
	return waslen;
	}
#if 0
       struct sockaddr_in6 {
               sa_family_t     sin6_family;   /* AF_INET6 */
               in_port_t       sin6_port;     /* port number */
               uint32_t        sin6_flowinfo; /* IPv6 flow information */
               struct in6_addr sin6_addr;     /* IPv6 address */
               uint32_t        sin6_scope_id; /* Scope ID (new in 2.4) */
           };

           struct in6_addr {
               unsigned char   s6_addr[16];   /* IPv6 address */
           }
#endif
inline bool putiphasfamport(const struct sockaddr *addrptr,struct sockaddr_in6 *outip) {
        LOGGER("family=%d AF_INET=%d AF_INET6=%d\n",addrptr->sa_family,AF_INET,AF_INET6);
	switch(addrptr->sa_family) {
		case AF_INET: outip->sin6_addr=v426(addrptr); break;
		case AF_INET6:  {
		 	const struct sockaddr_in6 *ad6=reinterpret_cast<const struct sockaddr_in6 *>(addrptr);
			outip->sin6_addr=ad6->sin6_addr;
			outip->sin6_scope_id=ad6->sin6_scope_id;
			outip->sin6_flowinfo=ad6->sin6_flowinfo;
		//	outip->sin6_family=AF_INET6;
			}
			break;
		default: return false;
		};
	LOGSTRING("putip succes\n");
	return true;
	}
inline bool putip(const struct sockaddr *addrptr,struct sockaddr_in6 *outip) {
        LOGGER("family=%d AF_INET=%d AF_INET6=%d\n",addrptr->sa_family,AF_INET,AF_INET6);
	switch(addrptr->sa_family) {
		case AF_INET: 
			       *outip={.sin6_family=AF_INET6,.sin6_port=  ((const struct sockaddr_in*)addrptr)->sin_port,.sin6_addr=v426(addrptr)};
				break;
		case AF_INET6:  {
			*outip=*reinterpret_cast<const struct sockaddr_in6 *>(addrptr);
			}
			break;
		default: return false;
		};
	LOGSTRING("putip succes\n");
	return true;
	}

extern sockaddr myname;
extern bool nameset;
