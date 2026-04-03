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

#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#ifndef HAVE_NOPRCTL
#include <sys/prctl.h>
#endif
#include <alloca.h>
 
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <errno.h>

#include <sys/wait.h>
#include <unistd.h>
#include "destruct.hpp"
#include "logs.hpp"
#include "myfdsan.h"
#include "TCPConnect.hpp"
extern Connect *connections[];
#ifndef LOGGER
#define LOGGER(...) fprintf(stderr,__VA_ARGS__)
#endif


#define lerrortag(...) lerror("backup: " __VA_ARGS__)
#define LOGGERTAG(...) LOGGER("backup: " __VA_ARGS__)
#define LOGARTAG(...) LOGAR("backup: " __VA_ARGS__)
#define LOGSTRINGTAG(...) LOGSTRING("backup: " __VA_ARGS__)
#define flerrortag(...) flerror("backup: " __VA_ARGS__)
using namespace std;

//#include <memory>
//void serverloop(int sock) ;
struct passhost_t ;

bool *shutdownreceiver=nullptr;
#include "mirrorerror.h"
constexpr const int maxservererror=200;
char servererrorbuf[maxservererror]="";
#define serverprint(...) snprintf( servererrorbuf,maxservererror,__VA_ARGS__)
#define servererror(...) savebuferror(servererrorbuf,maxservererror,__VA_ARGS__)
static bool serverloop(int sock, passhost_t *hosts,int &hostlen)  ;
static bool startserver(char *port, passhost_t *hosts,int *hostlen,bool *shutdownreceiver) {
    constexpr const char receiver[]{"RECEIVER"};
#ifndef HAVE_NOPRCTL
    prctl(PR_SET_NAME, receiver, 0, 0, 0);
#endif
   LOGGERN(receiver,sizeof(receiver)-1);
   
    destruct receiv([shutdownreceiver]{
            if(shutdownreceiver==::shutdownreceiver) {
                ::shutdownreceiver=nullptr;
                delete[] shutdownreceiver;
                }

            ;});
    struct addrinfo hints{.ai_flags=AI_PASSIVE,.ai_family=AF_INET6,.ai_socktype=SOCK_STREAM};

    struct addrinfo *servinfo=nullptr;
    destruct serv([&servinfo]{ if(servinfo)freeaddrinfo(servinfo);});
    destruct wweg([port]{delete[] port;});
    LOGARTAG("before getaddrinfo");
    if(int status= getaddrinfo(nullptr,port,&hints,&servinfo)) {
        LOGARTAG("after failed getaddrinfo");
        serverprint("getaddrinfo: %s",gai_strerror(status));
        LOGGERTAG("%s\n",servererrorbuf);
        return false;
        }
    LOGARTAG("after getaddrinfo");
    RESTART: {
    int sock=-1;
    for(struct addrinfo *ips=servinfo;;ips=ips->ai_next) {
        if(!ips) {
            LOGARTAG("no addresses to bind left");
            if(*shutdownreceiver) {
                LOGARTAG("shutdownreceiver return");
                return false;
                }
            sleep(1);
            goto RESTART;
            }
        sock=socket(AF_INET6,ips->ai_socktype,ips->ai_protocol);
        if(sock==-1) {
            servererror("socket");
            LOGGERTAG("%s\n",servererrorbuf);
            continue;
            }
        LOGGERTAG("startserver sock=%d\n",sock);
        const int  yes=1;    
        if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            servererror("setsockopt close(%d)",sock);
            LOGGERTAG("%s\n",servererrorbuf);
            sockclose(sock);
            return false;
            }
        if(bind(sock,ips->ai_addr,ips->ai_addrlen)==-1) {
            servererror("bind port=%s",port);
            LOGGERTAG("%s close(%d)\n",servererrorbuf,sock);
            sockclose(sock);
            continue;
            }
        break;
        }
    const auto tag=get_owner_tag(sock);
    constexpr int const BACKLOG=5;
    if(listen(sock, BACKLOG) == -1) {
        if(*shutdownreceiver) {
            lerrortag("listen");
            return false;
            }
        }
    serverloop(sock,hosts,*hostlen);
    if(*shutdownreceiver) {
        LOGARTAG("stop server");
        return true;
        }
    LOGARTAG("restart serverloop");
    sleep(1);
    goto RESTART;
    }
    }

#include "netstuff.hpp"
#include <thread>
#include "passhost.hpp"
static int globalsocket=-1;
//get_in_addr((struct sockaddr *)&their_addr)
bool receiveractive() {
    return globalsocket!=-1;
    }
void stopreceiver() {
    LOGGERTAG("stopreceiver %d\n",globalsocket);
    if(shutdownreceiver) {
        LOGARTAG("ask for shutdown receiver");
        *shutdownreceiver=true;
        }
    if(globalsocket>=0) {
        LOGGERTAG("shutdown globalsocket %d\n",globalsocket);
        shutdown(globalsocket,SHUT_RDWR);
        }
    }
#include <algorithm>
bool sameaddress(const  struct sockaddr *addr, const struct sockaddr_in6  *known) {
    switch(addr->sa_family) {
        case AF_INET: {
            uint32_t ip4= (((const struct sockaddr_in*)addr)->sin_addr).s_addr;
#ifdef __ANDROID__
            if(ip4==known->sin6_addr.in6_u.u6_addr32[3]&&!memcmp(known->sin6_addr.in6_u.u6_addr8,zeros,10))
#else
            if(ip4==known->sin6_addr.__in6_u.__u6_addr32[3]&&!memcmp(known->sin6_addr.__in6_u.__u6_addr8,zeros,10))
#endif
                return true;
            };
    case AF_INET6: {
         const struct sockaddr_in6 *ina6=(const struct sockaddr_in6*)addr;
         const struct in6_addr  *addr6=&ina6->sin6_addr;
//        if(!memcmp(known->__in6_u.__u6_addr8,zeros,10)) {
        if(!memcmp(known,zeros,10)) {
#ifdef __ANDROID__
            return addr6->in6_u.u6_addr32[3]==known->sin6_addr.in6_u.u6_addr32[3];
#else
            return addr6->__in6_u.__u6_addr32[3]==known->sin6_addr.__in6_u.__u6_addr32[3];
#endif
            }

        return known->sin6_scope_id==ina6->sin6_scope_id&&!memcmp(&known->sin6_addr,addr6,16);
        }
//        return !memcmp(known->__in6_u.__u6_addr8,addr6,16);
        default: return false;
        }
    }
//&hosts[hostlen-1]

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "destruct.hpp"
//#define LOGGER printf
static bool same_address(const struct sockaddr *addr1,const struct sockaddr *addr2 ) {
    switch(addr1->sa_family) {
      case AF_INET: {
         if(addr2->sa_family==AF_INET6) return sameaddress(addr1, (const struct sockaddr_in6*)  addr2);
        const struct sockaddr_in *ip1 = (const struct sockaddr_in *)addr1;
        const struct sockaddr_in *ip2 = (const struct sockaddr_in *)addr2;
        return ip1->sin_addr.s_addr==ip2->sin_addr.s_addr;
         }
      case    AF_INET6: {
        if(addr2->sa_family==AF_INET) return sameaddress(addr2, (const struct sockaddr_in6*)  addr1);
        const struct sockaddr_in6 *ip1 = (const struct sockaddr_in6 *)addr1;
        const struct sockaddr_in6 *ip2 = (const struct sockaddr_in6 *)addr2;
        return !memcmp(ip1->sin6_addr.s6_addr,ip2->sin6_addr.s6_addr,16);
         }
       }
     return false;
       }
#ifndef NOLOG
static void address(char *ipstr,int ipstrlen,const struct sockaddr *addr) {
    void *addr_ptr;

    if (addr->sa_family == AF_INET) {
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)addr;
        addr_ptr = &(ipv4->sin_addr);
    } else if (addr->sa_family == AF_INET6) {
        struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)addr;
        addr_ptr = &(ipv6->sin6_addr);
    } else {
        printf("Unsupported address family\n");
        return;
    }

    inet_ntop(addr->sa_family, addr_ptr, ipstr, ipstrlen);
}
#endif
bool testhostname(const char *hostname,const struct sockaddr *addr) {
    struct addrinfo hints{.ai_family = AF_UNSPEC,.ai_socktype = SOCK_STREAM};

   struct addrinfo  *firstaddr=nullptr;
    if(int errcode = getaddrinfo(hostname, NULL, &hints, &firstaddr)) {
       LOGGER("getaddrinfo: %s\n", gai_strerror(errcode));
       return false;
       }
#ifndef NOLOG
         char ipstr2[INET6_ADDRSTRLEN];
          address(ipstr2,sizeof(ipstr2), addr);
#endif
    destruct _dest( [firstaddr]{freeaddrinfo(firstaddr);});
    for(struct addrinfo  *addri = firstaddr; addri != NULL; addri = addri->ai_next) {
#ifndef NOLOG
         char ipstr1[INET6_ADDRSTRLEN];
          address(ipstr1,sizeof(ipstr1), addri->ai_addr);
#endif
        if(same_address(addri->ai_addr, addr)) {
            LOGGER("%s == %s \n",ipstr1,ipstr2);
            return true;
            }
         else {
             LOGGER("%s != %s \n",ipstr1,ipstr2);
             }
      }

    return false;
   }

 bool Connect::testreceivemagic(passhost_t *pass) {
#include "receivemagic.h"
#include "sendmagic.hpp"
    constexpr int buflen=1024;
    char buf[buflen];
    int res;
    LOGARTAG("testreceivemagic");
    if((res =r_recvni(buf,buflen))==sendmagicspec.size()) {
        const int testlen=sendmagicspec.size()-4;
        if(!memcmp(buf,sendmagicspec.data(),testlen)) { 
            if(!memcmp(buf+testlen,sendmagicspec.end()-4,3)) {
                savemessage(pass,"I don't connect with myself");
                LOGGERTAG("%s\n",getmirrorerror(pass));
                }
            else {
                constexpr int reclen=sizeof(receivemagic);
                unsigned char *magicptr;
                if(pass->receivedatafrom()&&pass->newconnection) {
                    LOGGERTAG("new connection %s\n",pass->getnameif());
                    magicptr=(unsigned char *)alloca(reclen);
                    memcpy(magicptr,receivemagic,reclen-1);
                    magicptr[reclen-1]=0;
                    }
                else {
                    magicptr=receivemagic;
                    }
                if(r_sendni(magicptr,reclen)==reclen) {
                    LOGARTAG("receivemagic success");
                    if(!buf[sendmagicspec.size()-1]) {
                        LOGGERTAG("testreceivemagic zerolast %s\n",pass->getnameif());

                        extern void resethost(passhost_t &host) ;
                        resethost(*pass);
                        }
                    pass->newconnection=false;
                    return true;

                    }
                else {
                    saveerror(pass,"receivemagic ERROR %d",getReceiverIdent());
                    LOGGERTAG("%s\n",getmirrorerror(pass));
                    }
                }
            }
        else  {
            saveerror(pass,"receivemagic wrong  magic %d",getReceiverIdent());
            LOGGERTAG("%s\n",getmirrorerror(pass));
            }
        }
    else    {
        saveerror(pass,"testreceivemagic error recvni(%d..)=%d != %d\n",getReceiverIdent(),res, sendmagicspec.size() );
        LOGGERTAG("%s\n",getmirrorerror(pass));
        }
    return false;
    }
extern bool networkpresent;

void receiversockopt(int new_fd) {
    LOGGERTAG("receiversockopt(%d)\n",new_fd);
       const int keepalive = 1;
       if(setsockopt(new_fd, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive)) < 0) {
        flerrortag("setsockopt(%d,SO_KEEPALIVE, ) failed",new_fd);
         }
      int retalive=-4;
    socklen_t retlen=sizeof(retalive);    

       if(getsockopt(new_fd, SOL_SOCKET, SO_KEEPALIVE, &retalive, &retlen) < 0) {
        flerrortag("getsockopt(%d,SO_KEEPALIVE, ) failed",new_fd);
         }
    else
          LOGGER("KEEPALIVE=%d\n",retalive);
       const int keepcnt = 1;
    if(setsockopt(new_fd, IPPROTO_TCP, TCP_KEEPCNT, &keepcnt, sizeof(keepcnt))<0) {
        flerrortag("setsockopt(%d,TCP_KEEPCNT ) failed",new_fd);
        }
    retlen=sizeof(retalive);    
    if(getsockopt(new_fd, IPPROTO_TCP, TCP_KEEPCNT, &retalive, &retlen)<0) {
        flerrortag("getsockopt(%d,TCP_KEEPCNT ) failed",new_fd);
        }
       else
      LOGGER("KEEPCNT=%d\n",retalive);
      /*
       if(setsockopt(new_fd, IPPROTO_TCP, TCP_SYNCNT, keepcnt)<0)  {
        flerrortag("setsockopt(%d,TCP_SYNCNT) failed",new_fd);
           } */
       const int keepidle = 50;
       if(setsockopt(new_fd, IPPROTO_TCP, TCP_KEEPIDLE, &keepidle, sizeof(keepidle)) < 0) {
        flerrortag("setsockopt(%d,TCP_KEEPIDLE, ) failed",new_fd);
         }
    retlen=sizeof(retalive);    

       if(getsockopt(new_fd, IPPROTO_TCP, TCP_KEEPIDLE, &retalive, &retlen) < 0) {
        flerrortag("getsockopt(%d,TCP_KEEPIDLE, ) failed",new_fd);
         }
    else
      LOGGER("KEEPIDLE=%d\n",retalive);
       const int keepintvl = 45;
       if(setsockopt(new_fd, IPPROTO_TCP, TCP_KEEPINTVL, &keepintvl, sizeof(keepintvl)) < 0) {
        flerrortag("setsockopt(%d,TCP_KEEPINTVL, ) failed",new_fd);
         }
    retlen=sizeof(retalive);    
       if(getsockopt(new_fd, IPPROTO_TCP, TCP_KEEPINTVL, &retalive, &retlen) < 0) {
        flerrortag("getsockopt(%d,TCP_KEEPINTVL, ) failed",new_fd);
         }
    else
      LOGGER("KEEPINTVL=%d\n",retalive);

extern void sendtimeout(int sock,int secs);
     sendtimeout(new_fd,60);
extern void receivetimeout(int sock,int secs) ;
     receivetimeout(new_fd,0);
     }

void receiverthread(passhost_t *host,const int allindex) {
      char buf[17];
      snprintf(buf,17,"receiver %d",allindex);
#ifndef HAVE_NOPRCTL
      prctl(PR_SET_NAME, buf, 0, 0, 0);
#endif


    Connect *con=connections[allindex];
    con->receiving=true;
    destruct _{[con]{ con->receiving=false;}};
    LOGGERTAG("receiverthread %d %s\n",con->getReceiverIdent(),buf);
    if(con->getcommands(host)) {
        LOGGERTAG("open return receiverthread %d\n",con->getReceiverIdent());
        return;
        }
    LOGGERTAG("shutdown(%d)\n",con->getReceiverIdent());
    con->shutdownReceiver();
    }


bool serverloop(int serversock, passhost_t *hosts,int &hostlen)  {
globalsocket=serversock;
    while(true) {  // main accept() loop
         struct sockaddr_in6 their_addr;
//        struct sockaddr_storage their_addr;

        struct sockaddr *addrptr= (struct sockaddr *)&their_addr;
        socklen_t sin_size = sizeof(their_addr) ;
        LOGGERTAG("serverloop: accept(%d,%p,%d)\n",serversock,addrptr,sin_size);
        int new_fd = accept(serversock, addrptr, &sin_size);
        LOGGERTAG("na accept(serversock=%d)=%d\n",serversock,new_fd);
        if(new_fd == -1) {
            int ern=errno;
            servererror("serverloop: accept %d",ern);
            LOGGERTAG("%s\n",servererrorbuf);

            switch(ern) {
                case EFAULT: 
                case EPROTO:
                case EBADF:
                case EINVAL:
                case ENOTSOCK:
                case EOPNOTSUPP: 
                sockclose(serversock);
                if(serversock==globalsocket) {
                    globalsocket=-1;
                    }
                LOGGER("ending server, globalsocket=%d\n",globalsocket);
                return true;
                } 
            continue;
            }
        const auto tag=get_owner_tag(new_fd);
        if(!networkpresent) {
            LOGGERTAG("serverloop !networkpresent close %d\n",new_fd);
            sockclose(new_fd);
            continue;
            }
        {
        struct timeval tv;
        tv.tv_usec = 0;
        tv.tv_sec = 60*3;
        setsockopt(new_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
        setsockopt(new_fd, SOL_SOCKET, SO_SNDTIMEO , (const char*)&tv, sizeof tv);
        }

        passhost_t *endhost= hosts+hostlen;
        bool hasname=false;
        passhost_t *hit=std::find_if(hosts,endhost,[&hasname,addrptr](const passhost_t &host) {
                if(host.hasname) {
                    hasname=true;
                    return false;
                    }
                if(host.noip)
                    return true;
                if(host.passive()) {
                    if(host.hasip(addrptr))
                        return true;
                    }
                return false;
                });
        const namehost name(addrptr);
        LOGGERTAG("server: got connection from %s sock=%d\n" ,(const char *)name ,new_fd);
        if(hit==endhost) {
            for(int h=0;h<hostlen;h++) {
                passhost_t& host=hosts[h];
                if(host.deactivated)
                    continue;
                if(host.passive()&&!host.hasname&&host.detect) {
                    if(!host.addiphasfamport(addrptr)) {
                        continue;
                        }
                    LOGARTAG("detected");        
                    hit=&host;
                    goto RIGHTHOST;
                     }
                }
            if(hasname) {
                char labelstr[passhost_t::maxnamelen];
                int rlen;
                if((rlen=recvni(new_fd,labelstr,passhost_t::maxnamelen))==passhost_t::maxnamelen) {
                    serverprint(R"(host %s tries to connect, label="%s")",(const char *)name,labelstr);
                    LOGGERTAG("%s\n",servererrorbuf);
                    for(int h=0;h<hostlen;h++) {
                        passhost_t& host=hosts[h];
                        if(host.deactivated)
                            continue;
                        if((host.passive())&&host.hasname&&!memcmp(host.getname(),labelstr,passhost_t::maxnamelen)) { 
                             if(!host.noip) {
                                bool nothostreg=!host.hasip(addrptr)&&(!host.detect||!host.addiphasfamport(addrptr));

                                if(nothostreg) {
                                   serverprint("label %s host %d wrong ip",labelstr,h);
                                   LOGGERTAG("%s\n",servererrorbuf);
                                   continue;
                                   }
                                 }
                               else {
                                     if((!host.hasip(addrptr))&&host.detect) {
                                        host.addiphasfamport(addrptr);
                                        }
                                     
                                    }
                                LOGARTAG("take ");
                                hit=&host;
                                goto RIGHTHOST;
                                }
                        }
                    }
                else {
                    const int ind= rlen>0?(rlen>passhost_t::maxnamelen?(passhost_t::maxnamelen-1):rlen):0;
                    labelstr[ind]='\0';
                    serverprint("recvni(%d,%s)==%d!=%d\n",new_fd,labelstr,rlen,passhost_t::maxnamelen);
                    LOGGERTAG("%s\n",servererrorbuf);
                    }

                }
            LOGGER("Wrong host close(%d)\n",new_fd);
            sockclose(new_fd);
            continue;
            }
    RIGHTHOST:
         {
//           const int keepidle = 10*60;
        int allindex=hit-hosts;
        TCPConnect *con=(TCPConnect *)connections[allindex];
        if(!con) {
            sockclose(new_fd);
            continue;
            }
        int &sock=con->getReceiverSock();
        int oldsock=sock;
        sock=-1;
        con->setReceiverSock(new_fd);
        if(!con->testreceivemagic(hit)) {
            con->closeReceiverConnection();
            sock=oldsock;
            continue;
            }
    *servererrorbuf='\0';
    #define SENDPASSIVE 1     
    #define RECEIVEFROM 2     

//extern void getmyname(int sock); getmyname(new_fd);
        if(hit->sendpassive) {
            LOGARTAG("sendpassive");
            char ant=SENDPASSIVE;
            extern bool sendall(const passhost_t *host);
            if(hit->receivedatafrom()&&!sendall(hit)) {
                if(con->r_recvni(&ant, 1)!=1) {
                      LOGARTAG("No send/recv distinction");
                      }
                else {
                    LOGGERTAG("also receivefrom ant=%d\n",ant);
                    }
                }
            if(ant==SENDPASSIVE) {
                
//                con->setSenderSock(new_fd);
 //               sock=oldsock;
                con->passivesender(hit,sock,oldsock);
                continue;
                }
         else {
            constexpr const char mess[]="Other side should be active only";
            LOGAR(mess);
            savemessage(hit,mess);
            }

            }

        shutdown(oldsock,SHUT_RDWR);
        sleep(1);
        close(oldsock);
        receiversockopt(new_fd) ;
        LOGGER("serverloop oldsock=%d newsock=%d\n",oldsock,new_fd);
        std::thread  handlecon(receiverthread,hit,allindex);
        handlecon.detach();
        }
        }
    }
void startreceiver(const char *port,passhost_t *hosts,int &hostlen) {
    if(globalsocket!=-1) {
        sleep(2);
        globalsocket=-1;
        }
    int len=strlen(port)+1;
    char *portcp=new char[len];
    memcpy(portcp,port,len);
    std::thread backup(startserver,portcp,hosts,&hostlen,shutdownreceiver=new bool[1]());
    backup.detach(); 
    }



bool Connect::receiveConnect(passhost_t *hostptr) {
        char labelstr[passhost_t::maxnamelen];
        int rlen;
        if((rlen=r_recvni(labelstr,passhost_t::maxnamelen))==passhost_t::maxnamelen) {
            if(memcmp(hostptr->getname(),labelstr,passhost_t::maxnamelen)) { 
                LOGGER("receiveConnect: label different here %s there %s\n",hostptr->getname(),labelstr);
                closeReceiverConnection();
                return false;
                }
            if(!testreceivemagic(hostptr)) {
                LOGGER("receiveConnect: %s magic failed\n",labelstr);
                closeReceiverConnection();
                return false;
                }
            LOGGER("Receiver %.*s connected\n",rlen,labelstr);
            return true;
            }
        else {
                LOGGER("receiveConnect %s: can't get label r_recvni()=%d\n",hostptr->getname(),rlen);
                closeReceiverConnection();
                return false;
                }
        }


#ifdef MAIN

//s/\([A-Z]\+\)/printf(\"\1=%d\\n\",\1);/g
int main(int argc, char **argv) {
 printf("EBADF=%d\n",EBADF); printf("EINVAL=%d\n",EINVAL); printf("ENOTSOCK=%d\n",ENOTSOCK); printf("EOPNOTSUPP=%d\n",EOPNOTSUPP); 
    if(argc!=3) {
        LOGGERTAG("Usage: %s port basedir\n",argv[0]);
        exit(2);
        }

passhost_t hosts[1]{{.connect={.sin6_family=AF_INET6,.sin6_addr={.__in6_u{.__u6_addr32{0xffffffff,0xffffffff,0xffffffff,0xffffffff}}}},.receivefrom=2}};

int socks[1]={-1};
int len=1;
    startreceiver(argv[1],argv[2],hosts,len,socks);
    sleep(2);

    pause();
    }
#endif
