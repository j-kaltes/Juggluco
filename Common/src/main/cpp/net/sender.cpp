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
#include <chrono>
#include <algorithm>
#include <array>
//#include <latch>
#include <poll.h>
#include <alloca.h>
#ifndef HAVE_NOPRCTL
       #include <sys/prctl.h>
#endif
#include "destruct.hpp"

#include "logs.hpp"
#include "netstuff.hpp"
#include "passhost.hpp"
#include "crypt.h"
#include "makerandom.hpp"
#include "myfdsan.h"
#include "TCPConnect.hpp"
#include "datbackup.hpp"
#include "comtypes.hpp"
#ifdef WEAROS_MESSAGES
extern std::atomic_bool wearmessages[];
extern bool sendMessagesON(passhost_t *pass,bool val);
#endif
#define lerrortag(...) lerror("sender: " __VA_ARGS__)
#define LOGGERTAG(...) LOGGER("sender: " __VA_ARGS__)
#define LOGARTAG(...) LOGAR("sender: " __VA_ARGS__)
#define LOGSTRINGTAG(...) LOGSTRING("sender: " __VA_ARGS__)
#define flerrortag(...) flerror("sender: " __VA_ARGS__)

using namespace std;
#include "mirrorerror.h"

bool Connect::sendpassinit(passhost_t *host,crypt_t *ctx) {
   if(!host||!ctx)
      return false;
   constexpr int makelen=8;
   uint8_t nonce[ASCON_AEAD_NONCE_LEN];
   constexpr int takelen=ASCON_AEAD_NONCE_LEN-makelen;
   uint8_t *takestart=nonce+makelen;
   makerandom(nonce,makelen);
   if(int didsend=s_sendni(nonce,makelen);didsend!=makelen) {
      flerrortag("sendpassinit send getSenderIdent()=%d ret=%d\n",getSenderIdent(),didsend);
      return false;
      }
   const int len=s_recvni(takestart,takelen);
   if(len!=takelen) {
      flerrortag("sendpassinit getSenderIdent()=%d recv len=%d\n",getSenderIdent(),len);
      return false;
      }
   ascon_aead128a_init(ctx,host->pass.data(),nonce);
   LOGARTAG("end sendpassinit");
   return true;
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


#include "sendmagic.hpp"
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


 int Connect::testsendmagic(passhost_t *pass) {
   #include "receivemagic.h"
   decltype(sendmagicspec) *magicptr;
   LOGARTAG("testsendmagic");
   if(pass->receivedatafrom()&&pass->newconnection) {
      magicptr=(decltype(sendmagicspec) *)alloca(sizeof(sendmagicspec));
      *magicptr=sendmagicspec;   
      magicptr->back()=0;
      LOGGERTAG("testsendmagic newconnection %s\n",pass->getnameif());
      }
   else
      magicptr=&sendmagicspec;
   if(s_sendni(magicptr->data(),magicptr->size())!=magicptr->size()) {
      char *buf=getmirrorerrorsettime(pass);
      int waser=errno;
      constexpr const char mess[]="testsendmagic: send magic failed: ";
      constexpr const int len=sizeof(mess)-1;
      memcpy(buf,mess,len);
      strerror_r(waser, buf+len, maxmirrortext-len);
      LOGGERTAG("%s\n",buf);
      return 1;
      }
   constexpr const int recsize=sizeof(receivemagic);
   char buf[recsize];
   LOGARTAG("testsendmagic before recv magic");
   int gotlen;
   if((gotlen=s_recvni(buf,recsize))!=recsize) {
      char *ptr=getmirrorerrorsettime(pass);
      int waser=errno;
      int len=snprintf(ptr,maxmirrortext,"testsendmagic: magic recv()=%d!=%d: ",gotlen,(int)recsize);
      strerror_r(waser, ptr+len, maxmirrortext-len);
      LOGGERTAG("%s\n",ptr);
      return 2;
      }
   LOGARTAG("testsendmagic: after recv magic");
   if(memcmp(buf,receivemagic,recsize-4)) {//4 less for version info
      char wrong[]="testsendmagic: Wrong magic";
      char *buf=getmirrorerrorsettime(pass);
      memcpy(buf,wrong,sizeof(wrong));
      LOGGERN(wrong,sizeof(wrong)-1);
      return 3;
      }
   LOGGERTAG("testsendmagic %d success\n",getSenderIdent());
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
     LOGGERTAG("USER_TIMEOUT=%d\n",retalive);
     }


   }
//extern void getmyname(int sock) ;


bool Connect::sendtype(char type) {
   LOGGERTAG("sendtype(%d,%d)\n",getSenderIdent(),type);
   char ant=type;
   if(s_sendni(&ant,1)!=1) {
      return false;
      }
   return true;
   }


extern char *getmirrorerror(const passhost_t *pass);
extern char *getmirrorerrorsettime(const passhost_t *pass);
void settimeouts(int sock,int seconds=60*3) {
   struct timeval tv;
   tv.tv_usec = 0;
   tv.tv_sec = seconds;
   setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
   setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO , (const char*)&tv, sizeof tv);
   }

 int Connect::shakehands(passhost_t *pass,char stype) {
   LOGGERTAG("shakehands connection %d\n",getSenderIdent());
   if(pass->hasname) {
      const char *name= pass->getname();
      LOGGERTAG("sendni(%d,%s,)\n",getSenderIdent(),name);
      if(s_sendni(name,pass->maxnamelen)!=pass->maxnamelen) {
         char err[]="s_sendni name error";
         saveerror(pass,err);
         LOGGERTAG("%s\n",getmirrorerror(pass));
         return -1;
         }
      
      }
   int magret;
   if((magret=testsendmagic(pass)))  {
      if(magret==2)  {
         if(pass->hasname) {
            savemessage(pass,"Sender with label %s rejected by receiver",pass->getname());
            }
         return -2;
         }
      return -1;
      }
   if(stype) {
      sendtype(stype);
      }
      
   LOGGERTAG("getSenderIdent()=%d\n",getSenderIdent());
   return 1;
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

int TCPConnect::connectone( const struct sockaddr_in6  *sin, int &sock,char stype,passhost_t *pass,struct pollfd    *cons,int&use,int ioTimeoutSeconds
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
      sockclose(so);
      return -1;
      }
   LOGGERTAG("try  %s sock=%d\n", name.data(),so);
   if(connect(so,(const struct sockaddr* )sin,sizeof(*sin))==-1) {
      if(errno == EINPROGRESS) {
         LOGGERTAG("%d progress\n",so);
         cons[use++]={so,POLLOUT,0};
         }
      else {
         LOGARTAG("close");
         sockclose(so);
         return -1;
         }
      }
   else {
#ifdef WEAROS_MESSAGES
      activate=false;
#endif
      block(so);
      sock=so;
      settimeouts(sock,ioTimeoutSeconds);
      if(shakehands(pass,stype)>=0) {
         LOGGERTAG("before poll %d\n",sock);
         for(int w=0;w<use;w++) {
            sockclose(cons[w].fd);
            }
         return sock;
         }
      sockclose(so);
      sock=-1;
      return -1;;       //close in shakehands
      }
   return -1;
   }

extern int getMirrorCurrentPeerAddresses(int index,sockaddr_in6 *out,int max);
int TCPConnect::makeconnection2withoptions(passhost_t *pass,char stype,bool allowAlternate,int timeoutMillis) {
#ifdef WEAROS_MESSAGES
extern bool mirrorLocalTcpAvailable();
if(allowAlternate&&pass->automatictransport()&&!mirrorLocalTcpAvailable()) {
   closeSenderConnection();
   LOGGER("Automatic mirror %s: no local TCP/IP endpoint; requesting %s fallback without TCP attempt\n",
         pass->getnameif(),pass->wearos?"Google Messages":"Direct Bluetooth");
   sendMessagesON(pass,true);
   return -1;
   }
destruct dest([pass,allowAlternate]() {
   if(allowAlternate&&pass->automatictransport()) {
      LOGGER("Automatic mirror %s: TCP/IP failed, requesting %s fallback\n",pass->getnameif(),
            pass->wearos?"Google Messages":"Direct Bluetooth");
      sendMessagesON(pass,true);
      }
   else {   
    if(pass->automatictransport()) {
          LOGGER("TCP/IP probe failed for Automatic mirror %s; current carrier unchanged\n",pass->getnameif());
          }
       else
          LOGGER("TCP connection failed for %s; forced transport remains TCP/IP\n",pass->getnameif());
      }
   });
bool activate=true;
// Keep fallback armed for early failures too (no saved IP, DNS failure, or no
// route). connectone clears activate after a synchronous TCP connection.
dest.active=activate;
#endif
   int use=0;
   const int ioTimeoutSeconds=allowAlternate?60*3:std::max(1,(timeoutMillis+999)/1000);
    closeSenderConnection(); 
    int   &sock=getSenderSock();
   struct pollfd    cons[16];
#ifdef JUGGLUCO_APP
   // Authenticated BLE /bleips data is kept in a transient cache so current
   // LAN addresses can be tried without replacing deliberately configured
   // Internet/global passhost_t::ips[] entries.  Automatic transport tries
   // these current endpoints first, then falls back to its persisted/DNS
   // bootstrap candidates in the same nonblocking poll.
   std::array<sockaddr_in6,passhost_t::maxip-1> currentPeerIps{};
   const int currentPeerNr=pass->automatictransport()?
         getMirrorCurrentPeerAddresses(allindex,currentPeerIps.data(),
               static_cast<int>(currentPeerIps.size())):0;
   if(currentPeerNr>0)  {
      LOGGERTAG("Automatic mirror %s: trying %d authenticated current peer endpoint(s) before configured endpoints\n", pass->getnameif(),currentPeerNr);
       for(int i=0;i<currentPeerNr;++i) {
          if(int ret=connectone(&currentPeerIps[i],sock,stype,pass,cons,use,ioTimeoutSeconds
    #if defined(WEAROS_MESSAGES)
                                ,activate
    #endif
                  );ret>=0) {
    #ifdef WEAROS_MESSAGES
             dest.active=activate;
    #endif
             LOGGERTAG("Automatic mirror %s: connected through current peer endpoint %d\n",
                   pass->getnameif(),i);
             return ret;
             }
          }
       }
   auto isCurrentPeer=[&](const sockaddr_in6 &candidate) {
      for(int i=0;i<currentPeerNr;++i)
         if(candidate.sin6_family==currentPeerIps[i].sin6_family&&
               candidate.sin6_port==currentPeerIps[i].sin6_port&&
               candidate.sin6_scope_id==currentPeerIps[i].sin6_scope_id&&
               !memcmp(&candidate.sin6_addr,&currentPeerIps[i].sin6_addr,sizeof(candidate.sin6_addr)))
            return true;
      return false;
      };
#else
   int currentPeerNr=0;
   auto isCurrentPeer=[](const sockaddr_in6 &candidate) {return false;};
#endif

   if(pass->hashostname()) { 
             struct addrinfo hints{.ai_flags=AI_ADDRCONFIG,.ai_family=AF_UNSPEC,.ai_socktype=SOCK_STREAM};
             struct addrinfo *servinfo=nullptr;
             destruct serv([&servinfo]{ if(servinfo)freeaddrinfo(servinfo);});
             const char *host= pass->gethostname(); 
             char port[10];
             sprintf(port,"%d",pass->getport());
             LOGGERTAG("connect to %s %s\n",host,port);
             if(int error=getaddrinfo(host,port,&hints,&servinfo)) {
                  char *buf=getmirrorerrorsettime(pass);
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

                  if(int ret=connectone(sin,sock, stype,pass,cons,use,ioTimeoutSeconds
  #if defined(WEAROS_MESSAGES)
                                                            ,activate
  #endif
          );ret>=0) {

                          LOGGERTAG("found %s:%s sock=%d\n",host,port,ret);
#ifdef WEAROS_MESSAGES
                          dest.active=activate;
#endif
                          return ret;
                          }

                  LOGGERTAG("wait %s\n",host);
                  }

                  }
     } else {
             const int nr=pass->nr;
             LOGGERTAG("makeconnection configured=%d current=%d\n",nr,currentPeerNr);
             if(nr<=0&&currentPeerNr<=0) {
                     savemessage(pass,"connection has no IPs\n");
                     return -1;
                     }
             const int validnr=(nr>=0&&nr<passhost_t::maxip)?nr:0;
             if(nr!=validnr) {
                     pass->nr=0;
                     savemessage(pass,"connection has invalid configured IP count %d\n",nr);
                     }
             for(int i=0;i<validnr;i++) {
                     const struct sockaddr_in6  *sin=&pass->ips[i];
                     if(isCurrentPeer(*sin)) {
                             LOGGERTAG("configured endpoint %d duplicates authenticated current endpoint; skipping duplicate connect\n",i);
                             continue;
                             }
                     if(int ret=connectone(sin,sock, stype,pass,cons,use,ioTimeoutSeconds
     #if defined(WEAROS_MESSAGES)
                                                               ,activate
     #endif



                     ) ;ret>=0)  {
                             LOGGERTAG("%d: found %d\n",i,ret);
#ifdef WEAROS_MESSAGES
                             dest.active=activate;
#endif
                             return ret;
                             }

                     LOGGERTAG("wait %d\n",i);
                     }
         }
#ifdef WEAROS_MESSAGES
   dest.active=activate;
#endif
   LOGGERTAG("use=%d\n",use);
#ifdef WEAROS_MESSAGES
   // Keep the existing parallel nonblocking TCP attempts, but do not make an
   // Automatic mirror wait for the full 60-second poll before even starting
   // its alternate carrier.  After three seconds request Messages/Direct BLE
   // while these same sockets continue connecting in the kernel.  Non-Bluetooth
   // peers simply ignore/fail that optional request and retain the old TCP path.
   const int automaticIndex=allindex;
   const auto tcpPollStarted=std::chrono::steady_clock::now();
   const auto tcpPollDeadline=tcpPollStarted+std::chrono::milliseconds(timeoutMillis);
   const auto automaticFallbackAt=tcpPollStarted+std::chrono::seconds(3);
   bool automaticFallbackRequested=false;
   auto closePending=[&]() {
       for(int i=0;i<use;++i)
           sockclose(cons[i].fd);
       use=0;
       };
#endif
   while(use) {
      int timeout=timeoutMillis;
#ifdef WEAROS_MESSAGES
      if(allowAlternate&&pass->automatictransport()) {
          if(automaticFallbackRequested&&automaticIndex>=0&&
                  wearmessages[automaticIndex].load()) {
              LOGGERTAG("Automatic mirror %s: alternate carrier became ready during TCP poll\n",pass->getnameif());
              closePending();
              dest.active=false;
              return -1;
              }
          const auto now=std::chrono::steady_clock::now();
          const auto nextWake=automaticFallbackRequested?
                  std::min(tcpPollDeadline,now+std::chrono::seconds(1)):
                  std::min(tcpPollDeadline,automaticFallbackAt);
          timeout=static_cast<int>(std::max<int64_t>(1,
                  std::chrono::duration_cast<std::chrono::milliseconds>(nextWake-now).count()));
          }
#endif
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
#ifdef WEAROS_MESSAGES
            if(allowAlternate&&pass->automatictransport()) {
                const auto now=std::chrono::steady_clock::now();
                if(!automaticFallbackRequested&&now>=automaticFallbackAt) {
                    automaticFallbackRequested=true;
                    LOGGER("Automatic mirror %s: TCP/IP not connected after 3 seconds; starting alternate carrier in parallel\n",pass->getnameif());
                    if(sendMessagesON(pass,true)) {
                        LOGGERTAG("Automatic mirror %s: alternate carrier selected during TCP poll\n",pass->getnameif());
                        closePending();
                        dest.active=false;
                        return -1;
                        }
                    // Direct BLE may still be discovering/authenticating. Wake
                    // every second so a successful asynchronous switch is seen
                    // without disturbing the pending TCP sockets.
                    continue;
                    }
                if(now<tcpPollDeadline)
                    continue;
                }
#endif
            savemessage(pass,"poll timeout");
            #ifndef NOLOG
            char *ptr=getmirrorerror(pass);
            LOGGERTAG("%s\n",ptr);
            #endif
            return -1;
            }
         };
      int newuse=0;
      for(int i=0;i<use;i++) {
         if(cons[i].revents & POLLRDHUP){
            savemessage(pass," %d: POLLRDHUP",cons[i].fd);
            LOGGERTAG("%s\n",getmirrorerror(pass));
            sockclose(cons[i].fd); 
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
            sockclose(cons[i].fd); 
            continue;
            }
         if(cons[i].revents &POLLHUP){
            savemessage(pass,"socket %d: POLLHUP",cons[i].fd);
            LOGGERTAG("%s\n",getmirrorerror(pass));
            sockclose(cons[i].fd);
            continue;
            }
         if(cons[i].revents &POLLNVAL){
            savemessage(pass,"socket %d: POLLNVAL\n",cons[i].fd);
            LOGGERTAG("%s\n",getmirrorerror(pass));
            sockclose(cons[i].fd);
            continue;
            }
         if(cons[i].revents & POLLOUT){
            sock=cons[i].fd;
            block(sock);
            settimeouts(sock,ioTimeoutSeconds);
            int ret;
            if((ret=shakehands(pass,stype))>=0) {
               for(int w=0;w<newuse;w++) {
                  sockclose(cons[w].fd);
                  }
               for(int w=i+1;w<use;w++) {
                  sockclose(cons[w].fd);
                  }
               LOGGERTAG("via poll %d\n",sock);
#ifdef WEAROS_MESSAGES
               if(pass->automatictransport()&&automaticFallbackRequested) {
                  LOGGERTAG("Automatic mirror %s: TCP/IP connected; cancelling pending alternate carrier\n",
                        pass->getnameif());
                  sendMessagesON(pass,false);
                  }
               dest.active=false;
#endif
               return sock;   
               }
            int so=sock;
            sock=-1;
            sockclose(so);
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
   LOGARTAG("no one");
   return -1;
   }

// Test the currently cached TCP endpoints without changing an Automatic mirror's
// active Messages/Direct-BLE carrier. makeconnection2withoptions() proves only
// that a Juggluco TCP listener answered the label/magic preamble; private IPv4
// collisions can therefore reach a different configured device. Complete the
// password-derived ASCON setup and send an encrypted sack command which requires
// a valid acknowledgement. Only that proves this endpoint owns this mirror row's
// password before Java is allowed to disable the working alternate carrier.
bool probeMirrorTcp(passhost_t *pass,int timeoutMillis) {
   if(!pass||timeoutMillis<=0||!pass->haspass())
      return false;
   const auto hosts=getBackupHosts();
   const int index=pass-hosts.data();
#ifdef WEAROS_MESSAGES
   // Never probe through the normal receiver path while an alternate
   // Messages/Direct-BLE bridge is selected. The peer's serverloop() uses the
   // same TCPConnect receiver slot and such a probe can tear down that bridge.
   if(index>=0&&static_cast<size_t>(index)<hosts.size()&&index<maxallhosts&&
         wearmessages[index].load()) {
      LOGGERTAG("refusing destructive TCP probe for %s while alternate carrier is active\n",
            pass->getnameif());
      return false;
      }
#endif
   TCPConnect probe(index);
   const int sock=probe.makeconnection2withoptions(pass,0,false,timeoutMillis);
   bool ok=false;
   if(sock>=0) {
      crypt_t ctx{};
      const sendack command;
      ok=probe.sendpassinit(pass,&ctx)&&
            probe.s_noacksendcommand(&ctx,reinterpret_cast<const uint8_t*>(&command),sizeof(command))&&
            probe.getack();
      }
   probe.closeSenderConnection();
   LOGGERTAG("TCP password-identity probe for %s: %s\n",pass->getnameif(),
         ok?"authenticated":"not reachable or wrong mirror password");
   return ok;
   }

int Connect::makeconnection(passhost_t *pass,crypt_t*ctx,char stype) {
   int res=makeconnection2(pass,stype);
   if(res>=0) {
      const auto tag=get_owner_tag(res);
      *getmirrorerrorsettime(pass)='\0';
      if(ctx)
         (void)sendpassinit(pass,ctx);
      }
   return res;
   }

#ifdef MAIN

int main(int argc, char **argv) {
//   int sock=makeconnection("192.168.1.69","12345");
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
