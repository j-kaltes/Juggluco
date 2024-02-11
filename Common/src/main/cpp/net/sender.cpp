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


//#include <arpa/inet.h>
       #include <sys/types.h>
       #include <sys/socket.h>
       #include <netdb.h>
#include <arpa/inet.h>
       #include <sys/socket.h>
              #include <unistd.h>
       #include <netinet/tcp.h>

#include <iostream>
#include <string.h>
#include <atomic>
#include <thread>
#include <algorithm>
//#include <latch>
#include <poll.h>
#include <alloca.h>
       #include <sys/prctl.h>

#include "destruct.h"

#include "logs.h"
#include "netstuff.h"
#include "passhost.h"
#include "crypt.h"
#include "makerandom.h"

#define lerrortag(...) lerror("sender: " __VA_ARGS__)
#define LOGGERTAG(...) LOGGER("sender: " __VA_ARGS__)
#define LOGSTRINGTAG(...) LOGSTRING("sender: " __VA_ARGS__)
#define flerrortag(...) flerror("sender: " __VA_ARGS__)

using namespace std;
#include "mirrorerror.h"

void	sendpassinit(int sock,passhost_t *host,crypt_t *ctx) {
	constexpr int makelen=8;
	uint8_t nonce[ASCON_AEAD_NONCE_LEN];
	constexpr int takelen=ASCON_AEAD_NONCE_LEN-makelen;
	uint8_t *takestart=nonce+makelen;
       makerandom(nonce, makelen);
	if(sendni(sock,nonce,makelen)!=makelen) {
		lerrortag("sendpassinit send");
		return;
		}
	int len=recvni(sock,takestart,takelen);
	if(len!=takelen) {
		lerrortag("recv");
		return;
		}
        ascon_aead128a_init(ctx, host->pass.data(),nonce);	
	LOGSTRINGTAG("end sendpassinit\n");
	}
bool unblock(int sock) {
  if( int val = fcntl(sock, F_GETFL, NULL);val >=0) {
	  val|=O_NONBLOCK;
	  if( fcntl(sock, F_SETFL,val) < 0) {
     		flerrortag("fcntl(%d, F_SETFL,%d)",sock,val);
     		return false;
		} 
	return true;
	} 
  else {
	     flerrortag("fcntl(%d, F_GETFL)",sock);
	     return false;
	     }
     }
bool block(int sock) {
  if( int val = fcntl(sock, F_GETFL, NULL);val >=0) {
	  val&=(~O_NONBLOCK);
	  if( fcntl(sock, F_SETFL,val) < 0) {
     		flerrortag("fcntl(%d, F_SETFL,%d)",sock,val);
     		return false;
		} 
	return true;
	} 
  else {
	     flerrortag("fcntl(%d, F_GETFL)",sock);
	     return false;
	     }
     }


#include "sendmagic.h"
static auto getsendmagic() {
	std::array<unsigned char,sizeof(sendmagic)> back=sendmagicinit;
	uint8_t lastrand;
	do {
		makerandom(end(back)-4,4);
		lastrand=back.back();
		} while(!lastrand);
	return back;
	}
std::array<unsigned char,sizeof(sendmagic)>  sendmagicspec=getsendmagic();


static int testsendmagic(passhost_t *pass,int sock) {
	#include "receivemagic.h"
	decltype(sendmagicspec) *magicptr;

	if(pass->receivedatafrom()&&pass->newconnection) {
		magicptr=(decltype(sendmagicspec) *)alloca(sizeof(sendmagicspec));
		*magicptr=sendmagicspec;	
		magicptr->back()=0;
		LOGGERTAG("testsendmagic newconnection %s\n",pass->getnameif());
		}
	else
		magicptr=&sendmagicspec;
	if(sendni(sock,magicptr->data(),magicptr->size())!=magicptr->size()) {
		char *buf=getmirrorerror(pass);
		int waser=errno;
		constexpr const char mess[]="send magic failed: ";
		constexpr const int len=sizeof(mess)-1;
		memcpy(buf,mess,len);
		strerror_r(waser, buf+len, maxmirrortext-len);
		LOGGERTAG("%s\n",buf);
		return 1;
		}
constexpr const int recsize=sizeof(receivemagic);
	char buf[recsize];
	LOGSTRINGTAG("before recv magic\n");
	int gotlen;
	if((gotlen=recvni(sock,buf,recsize))!=recsize) {
		char *ptr=getmirrorerror(pass);
		int waser=errno;
		int len=snprintf(ptr,maxmirrortext,"magic recv()=%d!=%d: ",gotlen,(int)recsize);
		strerror_r(waser, ptr+len, maxmirrortext-len);
		LOGGERTAG("%s\n",ptr);
		return 2;
		}
	LOGSTRINGTAG("after recv magic\n");
	if(memcmp(buf,receivemagic,recsize-4)) {//4 less for version info
		char wrong[]="Wrong magic";
		char *buf=getmirrorerror(pass);
		memcpy(buf,wrong,sizeof(wrong));
		LOGGERN(wrong,sizeof(wrong)-1);
		return 3;
		}
	LOGGERTAG("testsendmagic %d success\n",sock);
//extern void	setreceiverversion(uint8_t version) ;
//	setreceiverversion(buf[recsize-1]);
	if(!buf[recsize-1]) {
		extern void resethost(passhost_t &host) ;
		resethost(*pass);
		}
	pass->newconnection=false;
	return 0;
	}

void receivetimeout(int sock,int secs) {
	LOGGERTAG("receivetimeout(%d,%d)\n",sock,secs);
	struct timeval tv;
	tv.tv_usec = 0;
	tv.tv_sec = secs;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
	}
void sendtimeout(int sock,int secs) {
	LOGGERTAG("sendtimeout(%d,%d)\n",sock,secs);
	struct timeval tv;
	tv.tv_usec = 0;
	tv.tv_sec = secs;
	setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO , (const char*)&tv, sizeof tv);

 const int  user_timeout = 94000;
  if (setsockopt(sock, IPPROTO_TCP, TCP_USER_TIMEOUT, &user_timeout, sizeof(user_timeout))) {
		flerrortag("setsockopt(%d,TCP_USER_TIMEOUT, ) failed",sock);
  }
  int retalive=-7;
  socklen_t retlen=sizeof(retalive);	
  if(getsockopt(sock, IPPROTO_TCP, TCP_USER_TIMEOUT, &retalive, &retlen)) {
	flerrortag("getsockopt(%d,TCP_USER_TIMEOUT, ) failed",sock);
      }
 else {
	  LOGGER("USER_TIMEOUT=%d\n",retalive);
	  }


	}
//extern void getmyname(int sock) ;


bool sendtype(int sock,char type) {
	LOGGERTAG("sendtype(%d,%d)\n",sock,type);
	char ant=type;
	if(sendni(sock,&ant,1)!=1) {
		return false;
		}
	return true;
        }


extern char *getmirrorerror(const passhost_t *pass);

int shakehands(passhost_t *pass,int &sock,char stype) {
	LOGGERTAG("shakehands connection %d\n",sock);
	
	destruct closer([&sock]()->void{
		int so=sock;
		sock=-1;
		close(so);; });
	
	struct timeval tv;
	tv.tv_usec = 0;
	tv.tv_sec = 60*3;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
	setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO , (const char*)&tv, sizeof tv);
	if(pass->hasname) {
		const char *name= pass->getname();
		LOGGERTAG("sendni(%d,%s,)\n",sock,name);
		if(sendni(sock,name,pass->maxnamelen)!=pass->maxnamelen) {
			char err[]="send name";
			saveerror(pass,err);
			LOGGER("%s",getmirrorerror(pass));
			return -1;
			}
		
		}
	int magret;
	if((magret=testsendmagic(pass,sock)))  {
		if(magret==2)  {
			if(pass->hasname) {
				savemessage(pass,"Wrong label?");
				}
			return -2;
			}
		return -1;
		}
	if(stype) {
		sendtype(sock,stype);
		}
		
	closer.active=false;
	LOGGERTAG("sock=%d\n",sock);
	return sock;
	}
/*
sockaddr myname;
bool nameset=false;

void getmyname(int sock) {
	 socklen_t  namelen= sizeof(myname);
	if(getsockname(sock,  &myname,&namelen )==-1) {
		lerrortag("getsockname");
		}
	else {
		nameset=true;
		namehost mname(&myname);
		LOGGERTAG("getsockname returned %s\n",mname.data());
		}
	} 
	*/

static int connectone( const struct sockaddr_in6  *sin, int &sock,char stype,passhost_t *pass,struct pollfd	 *cons,int&use
#if defined(WEAROS_MESSAGES)
		,bool &activate
#endif
				)  {
	int so;
	namehost name(sin);
	LOGGERTAG("%s family=%d\n", name.data(),sin->sin6_family);
	if((so=socket(sin->sin6_family,SOCK_STREAM,0))==-1) {
		flerrortag("openone  socket");
		return -1;;
		}

	if(!unblock(so)) {
		close(so);
		return -1;
		}
	LOGGERTAG("try  %s sock=%d\n", name.data(),so);
	if(connect(so,(const struct sockaddr* )sin,sizeof(*sin))==-1) {
		if(errno == EINPROGRESS) {
			LOGGERTAG("%d progress\n",so);
			cons[use++]={so,POLLOUT,0};
			}
		else {
			LOGSTRINGTAG("close\n");
			close(so);
			return -1;
			}
		}
	else {
#ifdef WEAROS_MESSAGES
		activate=false;
#endif
		block(so);
		sock=so;
		if(int ret=shakehands(pass,sock,stype);ret>=0) {
			LOGGERTAG("before poll %d\n",sock);
			for(int w=0;w<use;w++) {
				close(cons[w].fd);
				}
			return ret;
			}
		return -1;;		
		}
	return -1;
	}

static int makeconnection2(passhost_t *pass,int &sock,char stype) {
#ifdef WEAROS_MESSAGES
destruct dest([pass]() {
	if(pass->wearos) {

		extern void sendMessagesON(passhost_t *pass, bool val);
		sendMessagesON(pass,true);
		}
	});
dest.active=false;
bool activate=true;
#endif
	int use=0;

	sock=-1;

	struct pollfd	 cons[10];
	if(pass->hashostname()) { 
		struct addrinfo hints{.ai_flags=AI_ADDRCONFIG,.ai_family=AF_UNSPEC,.ai_socktype=SOCK_STREAM};
	//	struct addrinfo hints{.ai_family=AF_UNSPEC,.ai_socktype=SOCK_STREAM};
		struct addrinfo *servinfo=nullptr;
		destruct serv([&servinfo]{ if(servinfo)freeaddrinfo(servinfo);});
		const char *host= pass->gethostname(); 
		char port[10];
		sprintf(port,"%d",pass->getport());
		LOGGERTAG("connect to %s %s\n",host,port);
		if(int error=getaddrinfo(host,port,&hints,&servinfo)) {
			char *buf=getmirrorerror(pass);
			#ifndef NOLOG
			int len=
			#endif
			snprintf(buf, maxmirrortext,"connect %s %s failed: %s\n",host,port,gai_strerror(error));
			LOGGERN(buf,len);
			return -1;
			}
		else {
			for(struct addrinfo *iter=servinfo;iter!=nullptr;iter=iter->ai_next) {
			const struct sockaddr *sa=iter->ai_addr;
			const struct sockaddr_in6  *sin;
			switch(sa->sa_family) {
				case AF_INET6: sin= reinterpret_cast<const struct sockaddr_in6*>(iter->ai_addr);
				 	break;
				case AF_INET: {
					auto *tmp=reinterpret_cast<sockaddr_in6*>(alloca(sizeof(sockaddr_in6 )));
					*tmp={.sin6_family=AF_INET6, .sin6_port=((struct sockaddr_in *)sa)->sin_port, .sin6_addr=v426(sa)};	
					sin=const_cast<const sockaddr_in6*>(tmp);
					break;
					}
				default: {
					LOGGERTAG("unknown family %d\n",sa->sa_family);
					return -1;

					}
					};

			if(int ret=connectone(sin,sock, stype,pass,cons,use
	#if defined(WEAROS_MESSAGES)
								  ,activate
	#endif
		);ret>=0) {

				LOGGERTAG("found %s:%s sock=%d\n",host,port,ret);
				return ret;
				}

			LOGGERTAG("wait %s\n",host);
           		}

			}
	} else {
		const int nr=pass->nr;
		LOGGERTAG("makeconnection nr=%d\n",nr);
		if(nr<=0) {
			savemessage(pass,"connection has on %d ips\n",nr);
			return -1;
			}
		for(int i=0;i<nr;i++) {
			const struct sockaddr_in6  *sin=&pass->ips[i];
			if(int ret=connectone(sin,sock, stype,pass,cons,use
	#if defined(WEAROS_MESSAGES)
								  ,activate
	#endif



			) ;ret>=0)  {
				LOGGERTAG("%d: found %d\n",i,ret);
				return ret;
				}

			LOGGERTAG("wait %d\n",i);
			}
	    }
#ifdef WEAROS_MESSAGES
	dest.active=activate;
#endif
	LOGGERTAG("use=%d\n",use);
	while(use) {	
		constexpr const int	timeout= 60000;
		int errcode=poll(cons, use, timeout);
		switch(errcode) {
			case -1: {
				int er=errno;
				saveerror(pass,"poll");
				#ifndef NOLOG
				char *ptr=getmirrorerror(pass);
				LOGGERTAG("%s",ptr);
				#endif
				if(er== EINTR)
					continue;
				return -1;
				};
			case 0: {
				savemessage(pass,"poll timeout");
				#ifndef NOLOG
				char *ptr=getmirrorerror(pass);
				LOGGERTAG("%s",ptr);
				#endif
				return -1;
				}
			};
		int newuse=0;
		for(int i=0;i<use;i++) {
			if(cons[i].revents & POLLRDHUP){
				savemessage(pass," %d: POLLRDHUP",cons[i].fd);
				LOGGERTAG("%s\n",getmirrorerror(pass));
				close(cons[i].fd); 
				continue;
				}
			if(cons[i].revents &POLLERR){
				int error = 0;
				socklen_t errlen = sizeof(error);
				if(getsockopt(cons[i].fd, SOL_SOCKET, SO_ERROR, (void *)&error, &errlen)==-1)
					lerrortag("getsockopt");
				const char *errstr="";
				switch(error) {
					case EINTR: errstr="The system call was interrupted by a signal that was caught; see signal(7).";break;

					case EISCONN: errstr="The socket is already connected.";break;
					case EHOSTUNREACH: errstr="Host is unreachable.";break;

					case ENETUNREACH: errstr="Network is unreachable.";break;

					case ENOTSOCK: errstr="The file descriptor sockfd does not refer to a socket.";break;

					case EPROTOTYPE: errstr="The  socket type does not support the requested communications protocol.";break;

					case ETIMEDOUT: errstr="Timeout while attempting connection.";break;

					};
				savemessage(pass,"POLLERR: %s",errstr);

				LOGGERTAG("%s (%d) socket=%d\n",getmirrorerror(pass),error,cons[i].fd);
				close(cons[i].fd); 
				continue;
				}
			if(cons[i].revents &POLLHUP){
				savemessage(pass,"socket %d: POLLHUP",cons[i].fd);
				LOGGERTAG("%s\n",getmirrorerror(pass));
				close(cons[i].fd);
				continue;
				}
			if(cons[i].revents &POLLNVAL){
				savemessage(pass,"socket %d: POLLNVAL\n",cons[i].fd);
				LOGGERTAG("%s\n",getmirrorerror(pass));
				close(cons[i].fd);
				continue;
				}
			if(cons[i].revents & POLLOUT){
				sock=cons[i].fd;
				block(sock);
				int ret;
				if((ret=shakehands(pass,sock,stype))>=0) {
					for(int w=0;w<newuse;w++) {
						close(cons[w].fd);
						}
					for(int w=i+1;w<use;w++) {
						close(cons[w].fd);
						}
					LOGGERTAG("via poll %d\n",sock);
#ifdef WEAROS_MESSAGES
					dest.active=false;
#endif
					return ret;	
					}
				if(ret==-2) {
#ifdef WEAROS_MESSAGES
					dest.active=false;
#endif
					
					return -1;
					}
				continue;
				}
			else  {
				LOGGERTAG("poll again %d\n",cons[i].fd);
				cons[newuse++]={cons[i].fd,POLLOUT,0};
				}
			}
		use=newuse;
		}
	LOGSTRINGTAG("no one\n");
	return -1;
	}

int makeconnection(passhost_t *pass,int &sock,crypt_t*ctx,char stype) {
	int res=makeconnection2(pass,sock,stype);
	if(res>=0) {
		*getmirrorerror(pass)='\0';
		if(ctx)
			sendpassinit(sock,pass,ctx);
		}
	return res;
	}

#ifdef MAIN

int main(int argc, char **argv) {
//	int sock=makeconnection("192.168.1.69","12345");
const char *name=argv[1],*port=argv[2];
struct in6_addr  addr= getaddr(name, port) ;
for(int i=0;i<16;i++) 
#ifdef __ANDROID__
	printf("%d ",addr.in6_u.u6_addr8[i]);
#else	
	printf("%d ",addr.__in6_u.__u6_addr8[i]);
	#endif

printf("/n");
int sock;
int res=makeconnection(addr,port,sock);
	}
#endif
