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
/*      Fri Jan 27 12:38:28 CET 2023                                                 */


#ifndef WEAROS
#include <charconv>
#include "settings/settings.h"
//#include <arpa/inet.h>
       #include <sys/types.h>
       #include <sys/socket.h>
       #include <netdb.h>
#include <arpa/inet.h>
       #include <sys/socket.h>
       #include <sys/types.h>
       #include <sys/wait.h>
       #include <unistd.h>
       #include <netinet/in.h>
       #include <netinet/tcp.h>
#include <sys/prctl.h>
 
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <errno.h>

#include <netinet/in.h>
#include <sys/wait.h>
       #include <unistd.h>
       #include <sys/syscall.h> 
#include "../netstuff.h"
#include "destruct.h"

#include "logs.h"
#include "watchserver.h"

#include "datestring.h"
#ifndef LOGGER
#define LOGGER(...) fprintf(stderr,__VA_ARGS__)
#endif
typedef std::conditional<sizeof(long long) == sizeof(int64_t), long long, int64_t >::type longlongtype;


static void watchserverloop(int *sockptr,bool secure) ;
static bool startwatchserver(bool secure,int port,int *sockptr) {
	const char *servername=secure?"SSL WATCHSERVER":"WATCHSERVER";
	LOGGER("%s\n",servername);
	constexpr const int maxport=20;
	char watchserverport[maxport];
	snprintf(watchserverport,maxport,"%d",port);

        prctl(PR_SET_NAME, servername, 0, 0, 0);
	struct addrinfo hints{.ai_flags=AI_PASSIVE,.ai_family=AF_UNSPEC,.ai_socktype=SOCK_STREAM};
	int sock;
	{
	struct addrinfo *servinfo=nullptr;
	destruct serv([&servinfo]{ if(servinfo)freeaddrinfo(servinfo);});
	if(int status=getaddrinfo(nullptr,watchserverport,&hints,&servinfo)) {
		LOGGER("getaddrinfo: %s\n",gai_strerror(status));
		return false;
		}
	for(struct addrinfo *ips=servinfo;;ips=ips->ai_next) {
		if(!ips) {
			return false;
			}
		sock=socket(ips->ai_family,ips->ai_socktype,ips->ai_protocol);
		if(sock==-1) {
			lerror("socket");
			continue;
			}
		const int  yes=1;	
		if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			lerror("setsockopt");
			close(sock);
			return false;
			}
		if(bind(sock,ips->ai_addr,ips->ai_addrlen)==-1) {
			lerror("bind");
			close(sock);
			continue;
			}
		break;
		}
	}
	constexpr int const BACKLOG=5;
	if (listen(sock, BACKLOG) == -1) {
		close(sock);
		lerror("listen");
		return false;
		}

	*sockptr=sock;
	watchserverloop(sockptr,secure) ;
	return true;
	}

#include <thread>
#include <algorithm>
#include "sensoren.h"
#define _GNU_SOURCE 1
#include <sched.h>

int xdripserversock=-1;
int xdripserversslsock=-1;

bool stopconnection=false;

#ifdef USE_SSL
extern bool sslstopconnection;

void stopsslwatchthread() ;
std::string startsslwatchthread() ;

const std::string initsslserver(void);

//std::string testkeyfiles() ;
std::string startsslwatchthread() {
extern std::string haskeyfiles() ;
	auto error=haskeyfiles();
	if(error.size())
		return error;
		
	extern	std::string  loadsslfunctions() ; 
	static std::string keepworking=loadsslfunctions() ;
	if(keepworking.size()) {
		LOGGERN(keepworking.data(),keepworking.size());
		return keepworking;
		}
	auto working=initsslserver();
	if(working.size()==0) {
		std::thread watchsec(startwatchserver,true,settings->data()->sslport,&xdripserversslsock);
		watchsec.detach();
		sslstopconnection=false;
		}
	else {
		LOGGERN(working.data(),working.size());
		}
	return working;
	}
void stopsslwatchthread() {
	sslstopconnection=true;
	shutdown(xdripserversslsock,SHUT_RDWR);
	}
#endif

static std::string sha1secret;
extern void makesha1secret();
void makesha1secret() {
	sha1secret=sha1encode(settings->data()->apisecret,settings->data()->apisecretlength);
	}

void startwatchthread() {
	if(xdripserversock==-1)  {
	 	makesha1secret();
		std::thread watch(startwatchserver,false,17580,&xdripserversock);
		watch.detach();
	#ifdef USE_SSL
	if(xdripserversslsock==-1)  {
		if(settings->data()->useSSL) {
			const std::string error=startsslwatchthread();
			if(error.size()) {
				LOGGER("%s\n",error.data());
				}
			}
		}
	#endif
		}
	stopconnection=false;
	}
extern void stopwatchthread() ;
void stopwatchthread() {
	stopconnection=true;
	shutdown(xdripserversock,SHUT_RDWR);
#ifdef USE_SSL
	stopsslwatchthread();
#endif
	}

//&hosts[hostlen-1]

static void watchserverloop(int *sockptr,bool secure)  {
	int serversock=*sockptr;
	while(true) {  // main accept() loop
		 struct sockaddr_in6 their_addr;
//		struct sockaddr_storage their_addr;

		struct sockaddr *addrptr= (struct sockaddr *)&their_addr;
		socklen_t sin_size = sizeof(their_addr) ;
		LOGGER("accept(%d,%p,%d)\n",serversock,addrptr,sin_size);
		int new_fd = accept(serversock, addrptr, &sin_size);
		LOGGER("na accept(serversock)=%d\n",new_fd);
		if (new_fd == -1) {
			int ern=errno;
			flerror("accept %d",ern);
			switch(ern) {
				case EFAULT: 
				case EPROTO:
				case EBADF:
				case EINVAL:
				case ENOTSOCK:
				case EOPNOTSUPP: 
				if(*sockptr==serversock)
					*sockptr=-1;
				close(serversock);
				LOGSTRING("exit\n"); return;
				} 
			continue;
			}
		const namehost name(addrptr);
		const char * namestr=name;
const bool localhostonly=!settings->data()->remotelyxdripserver&&!secure;
		if(localhostonly&&strcmp(namestr,"::ffff:127.0.0.1")&&strcmp(namestr,"127.0.0.1") )  {
			struct sockaddr_in6 own_addr;
			bool sameaddress(const  struct sockaddr *addr, const struct sockaddr_in6  *known);
			bool getownip(struct sockaddr_in6 *outip);
			if(!getownip(&own_addr)) {
				LOGGER("%swatchserver: reject connection from %s getownip failed\n",secure?"secure":"",namestr);
				close(new_fd);
				continue;
				}
			if(!sameaddress(addrptr, &own_addr) ) {
				const namehost myname(&own_addr);
				LOGGER("%swatchserver: reject connection from %s same address %s failed",secure?"secure":"",namestr,(const char *)myname);
				close(new_fd);
				continue;
				}
		  }
	      LOGGER("%swatchserver: got connection from %s sock=%d\n" ,secure?"secure":"",namestr ,new_fd);
		void handlewatch(int sock) ;
#ifdef USE_SSL
		if(secure) {
void handlewatchsecure(int sock) ;
			sslstopconnection=false;
			std::thread  handlecon(handlewatchsecure,new_fd);
			handlecon.detach();
			}
		else 
#endif
		{
			stopconnection=false;
			std::thread  handlecon(handlewatch,new_fd);
			handlecon.detach();
			}
		}
	}
void handlewatch(int sock) {
      const char threadname[]="watchconnect";
      LOGGER("handlewatch %d\n",sock);
      prctl(PR_SET_NAME, threadname, 0, 0, 0);
extern void sendtimeout(int sock,int secs);
extern void receivetimeout(int sock,int secs) ;
 	receivetimeout(sock,60);
 	sendtimeout(sock,5*60);
bool	watchcommands(int sock);
	
	watchcommands(sock);
	close(sock);
	}



#include "getitems.h"

/*
               const char authfailure[]  = "Authentication failed - check api-secret\n"
                        + "\n" + (authNeeded ? "secret is required " : "secret is not required")
                        + "\n" + secretCheckResult.trinary("no secret supplied", "supplied secret matches", "supplied secret doesn't match")
                        + "\n" + "Your address: " + socket.getInetAddress().toString()
                        + "\n\n";
                if (JoH.ratelimit("web-auth-failure", 10)) {
                    UserError.Log.e(TAG, failureMessage);
                }
                response = new WebResponse(failureMessage, 403, "text/plain");

*/
std::string_view servererrorstr="HTTP/1.0 500 Internal Server Error\r\n\r\n";

void servererror(int sock) {
	send(sock,servererrorstr.data(),servererrorstr.size(),0);
	}



static bool	 sgvinterpret(const char *start,int len,bool headonly, recdata *data,bool all=true) ;


static bool givetreatments(const char *args,int argslen, recdata *data) ;


static bool sendall(int sock ,const char *buf,int buflen) {
        int itlen,left=buflen;
        LOGGER("sock=%d sendall len=%d\n",sock,buflen);
        for(const char *it=buf;(itlen=send(sock,it,left,0))<left;) {
		int waser=errno;
                LOGGER("len=%d\n",itlen);
                if(itlen<0) {
			errno=waser;
			flerror("send(%d,%p,%d)",sock,it,left);
			if(waser==EINTR)
				continue;
			return false;
                        }
                it+=itlen;
                left-=itlen;
                }
        LOGSTRING("success sendall\n");
        return true;
        }
bool watchcommands(char *rbuf,int len,recdata *outdata) ;
bool watchcommands(int sock) {
	constexpr const int RBUFSIZE=4096;
	char rbuf[RBUFSIZE];
	int len;
	if((len=recvni(sock,rbuf,RBUFSIZE))==-1) {
		servererror(sock);
		return false;
		}
	if(len==0) {
		LOGSTRING("shutdown\n");
		return false;
		}
	struct recdata outdata;

	if(stopconnection)
		return false;
	bool res=watchcommands(rbuf, len,&outdata); 
	bool res2=sendall( sock ,outdata.data(),outdata.size()) ;
	return res&&res2&&!stopconnection;
	}

 void mkheader(char *outstart,char *outiter,const bool headonly,recdata *outdata) ;
static bool givestatushtml(recdata *outdata) {
//static	constexpr const char status[]="HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: 19\r\n\r\n<h1>STATUS OK</h1>\n";
static	constexpr const char status[]="HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: 19\r\n\r\n<h1>STATUS OK</h1>\n";
	const int statuslen=sizeof(status)-1;
	outdata->allbuf=nullptr;
	outdata->start=status;
	outdata->len=statuslen;
	return true;
}
static bool givesite(recdata *outdata) {
static	constexpr const char webpage[]="HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: 133\r\n\r\n" 
R"(<!DOCTYPE html>
<html>
<head>
   <meta http-equiv="refresh" content="0; url=https://www.juggluco.nl"/>
</head>
<body>
</body>
</html>
)";

	const int statuslen=sizeof(webpage)-1;
	outdata->allbuf=nullptr;
	outdata->start=webpage;
	outdata->len=statuslen;
	return true;
}
static bool givestatus(recdata *outdata) {
	constexpr static
	#include "status.h"
	auto tim=time(nullptr);
        struct tm tmbuf;
        gmtime_r(&tim, &tmbuf);
	constexpr const int len=sizeof(statusformat)+50;
	outdata->allbuf=new(std::nothrow) char[len+512];
        char *start=outdata->allbuf+152;
	auto thigh=gconvert(settings->targethigh());
	auto tlow=gconvert(settings->targetlow());
	auto halarm=gconvert(settings->data()->ahigh);

	auto lowalarm=gconvert(settings->data()->alow);
	const char *unitlabel=settings->getunitlabel().data();
	int alllen=snprintf(start,len,statusformat,tmbuf.tm_year+1900,tmbuf.tm_mon+1,tmbuf.tm_mday, tmbuf.tm_hour, tmbuf.tm_min,tmbuf.tm_sec,0,tim*1000LL,unitlabel,halarm,thigh,tlow,lowalarm);
	mkheader(start,start+alllen,false,outdata); 
	return true;
	}

template <class T, size_t N>
inline static constexpr void addar(char *&uitptr,const T (&array)[N]) {
	constexpr const int len=N-1;
	memcpy(uitptr,array,len);
	uitptr+=len;
	}


extern std::string_view getdeltaname(float rate);
char * writebucket(char *outiter,const int index,const ScanData *val,const char *sensorname) {
	const char * changelabel=getdeltaname(val->ch).data();
	auto mgdl=val->getmgdL();
	longlongtype mmsec=val->gettime()*1000LL;
	longlongtype frommsec=mmsec-1000LL*30;
	longlongtype tomsec=mmsec+1000LL*30;
	return outiter+sprintf(outiter,R"({"mean":%d,"last":%d,"mills":%lld,"index":%d,"fromMills":%lld,"toMills":%lld,"sgvs":[{"_id":"%s#%d","mgdl":%d,"mills":%lld,"device":"Juggluco","direction":"%s","type":"sgv","scaled":%d}]},)",mgdl,mgdl,mmsec,index,frommsec,tomsec,sensorname,val->id,mgdl,mmsec,changelabel,mgdl);
}

//https://api/v1/entries/current

extern int Tdatestring(time_t tim,char *buf) ;

//		return givetreatments(outdata);
bool givenolist(recdata *outdata) {
	LOGSTRING("givenothing\n");
	const std::string_view nothing="[]";
	outdata->allbuf=new(std::nothrow) char[nothing.size()+512];
	char *start=outdata->allbuf+152;
	memcpy(start,nothing.data(),nothing.size());
	mkheader(start,start+nothing.size(),false,outdata);
	return true;
}
bool givenothing(recdata *outdata) {
	LOGSTRING("givenothing\n");
	const std::string_view nothing="{}\n";
	outdata->allbuf=new(std::nothrow) char[nothing.size()+512];
	char *start=outdata->allbuf+152;
	memcpy(start,nothing.data(),nothing.size());
	mkheader(start,start+nothing.size(),false,outdata);
	return true;
}

char *textitem(char *outiter,const ScanData *value,const char sep=9) {
	auto mgdL=value->getmgdL();
	time_t tim=value->gettime();
	const char * changelabel=getdeltaname(value->ch).data();
//	*outiter++='"';
//	outiter+=Tdatestring(tim,outiter);
        struct tm tmbuf;
	gmtime_r(&tim, &tmbuf);
        outiter+=sprintf(outiter,R"("%d-%02d-%02dT%02d:%02d:%02d.000Z")",tmbuf.tm_year+1900,tmbuf.tm_mon+1,tmbuf.tm_mday, tmbuf.tm_hour, tmbuf.tm_min,tmbuf.tm_sec);
	outiter+=sprintf(outiter,R"(%c%lld%c%d%c"%s"%c"Juggluco")" "\r\n",sep,tim*1000LL,sep,mgdL,sep,changelabel,sep);
	return outiter;
	}

//[{"_id":"%s","device":"%s","uploader":{"battery":%d,"type":"PHONE"},"created_at":"2023-03-05T20:05:53.203Z"},
static bool showdevicestatus(recdata *outdata) { //seems to be used for battery level. Unimportant.
	return givenothing(outdata);
	}
static bool givecurrent(recdata *outdata) {
	int sensorid=sensors->last();
	const SensorGlucoseData *sens=getStreamSensor(sensorid);;
	const std::span<const ScanData> gdata=sens->getPolldata();
	const ScanData *first=&gdata.begin()[0];
	const ScanData *iter=&gdata.end()[-1];
	while(!iter->valid()) {
		if(--iter<=first)
			return givenothing(outdata);
		}
	const ScanData *value=iter;;
//static	constexpr const char header[]="HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\nContent-Type: text/plain; charset=utf-8\r\nContent-Length: ";
static	constexpr const char header[]="HTTP/1.1 200 OK\r\nContent-Type: text/plain; charset=utf-8\r\nContent-Length: ";
	constexpr const int headerlen=sizeof(header)-1;
   	outdata->allbuf=new(std::nothrow) char[sizeof(header)+1024];
    char *start=outdata->allbuf+152,*outiter=start;
    	outiter=textitem(outiter,value)-2;
    /*
	addar(outiter,header);
	outiter+=sprintf(outiter,"%d\r\n\r\n", (mgdL>99)?(currentlen+1):currentlen); */
/*
	auto mgdL=value->getmgdL();
	time_t tim=value->gettime();
	const char * changelabel=getdeltaname(value->ch).data();
//	*outiter++='"';
//	outiter+=Tdatestring(tim,outiter);
        struct tm tmbuf;
	gmtime_r(&tim, &tmbuf);
        outiter+=sprintf(outiter,R"("%d-%02d-%02dT%02d:%02d:%02d.000Z")",tmbuf.tm_year+1900,tmbuf.tm_mon+1,tmbuf.tm_mday, tmbuf.tm_hour, tmbuf.tm_min,tmbuf.tm_sec);
	outiter+=sprintf(outiter,R"(	%ld	%d	"%s"	"Juggluco")",tim*1000L,mgdL,changelabel);
	*/
	long long len=outiter-start;
	char lenstr[20];
	int lenlen=snprintf(lenstr,20,"%lld\r\n\r\n",len);
	char *startheader=start-lenlen-headerlen;
	outdata->start=startheader;
	addar(startheader,header);
	memcpy(startheader,lenstr,lenlen);
	startheader+=lenlen;
	outdata->len=outiter-outdata->start;
	LOGGERN(outdata->start,outdata->len);
	return true;
	}
//https:///api/v1/entries/sgv.txt?count=24&find[date][$gte]=1676554219000&find[date][$lt]=1676561418000
/*

HTTP/1.1 200 OK
Server: Cowboy
Connection: keep-alive
X-Dns-Prefetch-Control: off
Expect-Ct: max-age=0
Strict-Transport-Security: max-age=31536000
X-Download-Options: noopen
X-Content-Type-Options: nosniff
X-Permitted-Cross-Domain-Policies: none
Referrer-Policy: no-referrer
X-Xss-Protection: 0
X-Powered-By: Express
Last-Modified: Sat, 18 Feb 2023 21:13:37 GMT   
Vary: Accept, Accept-Encoding
Content-Type: text/plain; charset=utf-8
Etag: W/"25a4-my3ekyu7A50gkeDVfLaUWFRnkhQ"
Content-Encoding: gzip
Date: Sat, 18 Feb 2023 22:08:56 GMT                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    
Transfer-Encoding: chunked
Via: 1.1 vegur
*/
#ifdef NOTAPP
#include "cmdline/jugglucotext.h"
#else
#include "curve/jugglucotext.h"
#endif
extern jugglucotext engtext;
int formattime(char *buf, time_t tim) {
        struct tm tmbuf;
	gmtime_r(&tim, &tmbuf);
	auto daylabel=engtext.daylabel;
	auto monthlabel=engtext.monthlabel;
	return sprintf(buf,"%s, %02d %s %d %02d:%02d:%02d GMT", daylabel[tmbuf.tm_wday], tmbuf.tm_mday,monthlabel[tmbuf.tm_mon] ,tmbuf.tm_year+1900, tmbuf.tm_hour, tmbuf.tm_min,tmbuf.tm_sec);
	}
bool givesgvtxt(const char *input,int inlen,recdata *outdata,char sep=9);
bool givesgvtxt(int nr,int interval,uint32_t lowerend,uint32_t higherend,recdata *outdata,char sep=9) {
//	static	constexpr const char header[]="HTTP/1.1 200 OK\r\nExpect-Ct: max-age=0\r\nAccess-Control-Allow-Origin: *\r\nVary: Accept, Accept-Encoding\r\nContent-Type: text/plain; charset=utf-8\r\nContent-Length: ";
//	static	constexpr const char header[]="HTTP/1.1 200 OK\r\nExpect-Ct: max-age=0\r\nStrict-Transport-Security: max-age=31536000\r\nServer: Cowboy\r\nConnection: keep-alive\r\nX-Dns-Prefetch-Control: off\r\nX-Download-Options: noopen\r\nX-Content-Type-Options: nosniff\r\nX-Permitted-Cross-Domain-Policies: none\r\nReferrer-Policy: no-referrer\r\nX-Xss-Protection: 0\r\nX-Powered-By: Express\r\nContent-Type: text/plain; charset=utf-8\r\nContent-Length: ";
	static	constexpr const char header[]="HTTP/1.1 200 OK\r\nContent-Type: text/plain; charset=utf-8\r\nContent-Length: ";
//	static	constexpr const char header[]="HTTP/1.1 200 OK\r\nContent-Type: text/csv; charset=utf-8\r\nContent-Length: ";
//	text/csv; charset=utf-8
	constexpr const int headerlen=sizeof(header)-1;
	 const int bufsize= headerlen+1+100*nr+100;
   	outdata->allbuf=new(std::nothrow) char[bufsize];
	if(outdata->allbuf==nullptr) {
		LOGGER("givesgvtxt new failed %d\n",bufsize);
		return false;
		}
    char *start=outdata->allbuf+250,*outiter=start;
//	int interval=4*61;
	if(!getitems(outiter,nr,lowerend,higherend,true,interval,[sep](char *outiter,int datit, const ScanData *iter,const char *sensorname,const time_t starttime) {
		return textitem(outiter,iter,sep);
	})) {
		return givenothing(outdata);
	};
	outiter-=2;
	long long len=outiter-start;
	char lenstr[30];
	int lenlen=sprintf(lenstr,"%lld\r\n\r\n",len);

	char *startheader=start-lenlen-headerlen;
	outdata->start=startheader;
	LOGSTRING("Before add header\n");
	addar(startheader,header);
	LOGSTRING("Before add lenstr\n");
	memcpy(startheader,lenstr,lenlen);
	startheader+=lenlen;
	outdata->len=outiter-outdata->start;
//	LOGGERN(outdata->start,outdata->len);
	return true;
}
extern int getdeltaindex(float rate);
extern std::string_view getdeltanamefromindex(int index) ;


//{"sgv":"10.7","trend":4,"direction":"Flat","datetime":1676804318000}
static char *pebbleitem(bool mmolL,char *outiter,const ScanData *item) {
	int trend=getdeltaindex(item->ch);
	float value=mmolL?item->getmmolL():item->getmgdL();
	return outiter+=sprintf(outiter,R"({"sgv":"%.1f","trend":%d,"direction":"%s","datetime":%lld},)",value,trend,getdeltanamefromindex(trend).data(),item->gettime()*1000LL);
	}

bool		 pebbleinterpret(const char *input,int inputlen,recdata *outdata) {
	int count=1;
	bool mmol=true;
   	outdata->allbuf=new(std::nothrow) char[512+80*+count+200];
    	char *start=outdata->allbuf+150,*outiter=start;
	auto nu=time(nullptr);
	constexpr const char startpebble[]=R"({"status":[{"now":%lld}],"bgs":[)";
	constexpr const char endpebble[]=R"(],"cals":[]})";
	outiter+=sprintf(outiter,startpebble,nu*1000LL);
	int interval=4*61;
	if(!getitems(outiter,count,0,UINT32_MAX,true,interval,[mmol](char *outiter,int datit, const ScanData *iter,const char *sensorname,const time_t starttime) {
			
		char *ptr=pebbleitem(mmol,outiter,iter);
		if(!datit) {
	 		//double delta= isnan(iter->ch)?0:iter->ch*deltatimes;
	 		double delta= getdelta(iter->ch);
			ptr-=2;
			ptr+=sprintf(ptr,R"(,"bgdelta":"%.1f"},)",delta);
			}
		return ptr;
		})) {
		return givenothing(outdata);
	};
	addar(--outiter,endpebble);
	mkheader(start, outiter, false, outdata);
	return true;
	}
char * givebuckets(char *start) {
	char *outiter=start;
	const char startbuckets[]=R"("buckets":[)";
	addar(outiter,startbuckets);
	int interval=4*61;
	if(!getitems(outiter,4,0,UINT32_MAX,true,interval,[](char *outiter,int datit, const ScanData *iter,const char *sensorname,const time_t starttime) {
		return writebucket(outiter,datit,iter,sensorname);
	})) {
		return start;
	};
	const char endbuckets[]=R"(],)";
	addar(--outiter,endbuckets);
	return outiter;
}
/*
char * givebuckets(char *start) {
	char *outiter=start;
	const char startbuckets[]=R"("buckets":[)";
	addar(outiter,startbuckets);
	int interval=4*61;
	if(!getitems(outiter,4,true,interval,[](char *outiter,int datit, const ScanData *iter,const char *sensorname,const time_t starttime) {
		return writebucket(outiter,datit,iter,sensorname);
	})) {
		return start;
	};
	const char endbuckets[]=R"(],)";
	addar(--outiter,endbuckets);
	return outiter;
}
*/

char * givecage(char *outiter) {
	const char cage[]= R"("cage":{"found":false,"age":0,"treatmentDate":null,"checkForAlert":false,"level":-3,"display":"n/a"},)";
	addar(outiter,cage);
    return outiter;
    }
    /*
"%s#%d",
*/
			//  {"delta":{"absolute":-2,"elapsedMins":5,"interpolated":false,"mean5MinsAgo":137,"times":{"recent":1676718516000,"previous":1676718216000},"mgdl":-2,"scaled":-2,"display":"-2","previous":{"mean":137,"last":137,"mills":1676718216000,"sgvs":[{"_id":"63f0b09d4d77ce842e333f3d","mgdl":137,"mills":1676718216000,"device":"loop://iPhone","direction":"Flat","type":"sgv","scaled":137}]}}}


char *getdeltastr(char *start) {
	char *outiter=start;
	int sensorid=sensors->last();
	const SensorGlucoseData *sens=getStreamSensor(sensorid);;
	const std::span<const ScanData> gdata=sens->getPolldata();
	const ScanData *first=&gdata.begin()[0];
	const ScanData *iter=&gdata.end()[-1];
	while(!iter->valid()) {
		if(--iter<=first)
			return start;
		}
	const char *sensorname= sens->shortsensorname()->data();
	int timedif=4*62;
	auto nu=iter->gettime();
	auto nuval=(iter--)->getmgdL();
	auto old=nu-timedif;
	while(iter>=first) {
		auto wastime=iter->gettime();
		if(wastime<old) {
			auto prevmgdl=iter->getmgdL();
			auto diff=nuval-prevmgdl;
			longlongtype nowmmsec=nu*1000LL;
			longlongtype prevmmsec=wastime*1000LL;
			int valueid=iter->getid();
			int elapsedMins=(nu-wastime)/60;
			const char * changelabel=getdeltaname(iter->ch).data();
			constexpr const char deltaformat[]=R"("delta":{"absolute":%d,"elapsedMins":%d,"interpolated":false,"mean5MinsAgo":%d,"times":{"recent":%lld,"previous":%lld},"mgdl":%d,"scaled":%d,"display":"%d","previous":{"mean":%d,"last":%d,"mills":%lld,"sgvs":[{"_id":"%s#%d","mgdl":%d,"mills":%lld,"device":"Juggluco","direction":"%s","type":"sgv","scaled":%d}]}},)";
			return outiter+sprintf(outiter,deltaformat,diff,elapsedMins,prevmgdl,nowmmsec,prevmmsec,diff,diff,diff,prevmgdl,prevmgdl,prevmmsec,sensorname,valueid,prevmgdl,prevmmsec,changelabel,prevmgdl);

			}
		--iter;
		}
	return start;
	}


	

//{"bgnow":{"mean":169,"last":169,"mills":1676751516000,"sgvs":[{"_id":"63f132b14d77ce842e5700eb","mgdl":169,"mills":1676751516000,"device":"share2","direction":"FortyFiveUp","type":"sgv","scaled":169}]}}
static char * givebgnow(char *start) {
	int sensorid=sensors->last();
	const SensorGlucoseData *sens=getStreamSensor(sensorid);;
	const std::span<const ScanData> gdata=sens->getPolldata();
	const ScanData *first=&gdata.begin()[0];
	const ScanData *iter=&gdata.end()[-1];
	while(!iter->valid()) {
		if(--iter<=first)
			return start;
		}
	longlongtype mmsectime=iter->gettime()*1000LL;
	auto mgdl=iter->getmgdL();
	int valueid=iter->getid();
	const char * changelabel=getdeltaname(iter->ch).data();
	const char *sensorname= sens->shortsensorname()->data();
	constexpr const char bgformat[]=R"("bgnow":{"mean":%d,"last":%d,"mills":%lld,"sgvs":[{"_id":"%s#%d","mgdl":%d,"mills":%lld,"device":"Juggluco","direction":"%s","type":"sgv","scaled":%d}]},)";
	return start+=sprintf(start,bgformat,mgdl,mgdl,mmsectime,sensorname,valueid,mgdl,mmsectime,changelabel,mgdl);
	}


bool giveproperties(const char *input,int inputlen,recdata *outdata) {
	LOGGER("giveproperties(%s,%d,recdata *outdata) \n",input,inputlen);
//	const char *end=input+inputlen;
 	if(*input++=='/') {
		outdata->allbuf=new(std::nothrow) char[512*inputlen];
		char *start=outdata->allbuf+152,*outiter=start;
		const char *endinput=input+inputlen;
		*outiter++='{';
		while (true) {
			const std::string_view buckets = "buckets";
			if (!memcmp(input, buckets.data(), buckets.size())) {
				LOGSTRING("givebuckets\n");
				outiter = givebuckets(outiter);
				input += buckets.size();
			} else {
				const std::string_view cage = "cage";
				if (!memcmp(input, cage.data(), cage.size())) {
					outiter = givecage(outiter);
					input += cage.size();
					}
				else {
					const std::string_view delta = "delta";
					if (!memcmp(input, delta.data(), delta.size())) {
						outiter = getdeltastr(outiter);
						input += delta.size();
						}
					else  {
						const std::string_view bgnow = "bgnow";
						if (!memcmp(input, bgnow.data(), bgnow.size())) {
							outiter = givebgnow(outiter);
							input += bgnow.size();
							}
						}
					}
				}
			if((input = std::find(input, endinput, ',')) == endinput) {
				if (outiter != (start + 1))
					--outiter;
				*outiter++ = '}';
				mkheader(start, outiter, false, outdata);
				return true;
			}
			++input;
			}
		}
	else {
		outdata->allbuf=new(std::nothrow) char[512*6];
		if(!outdata->allbuf)
    			return givenothing(outdata);
		char *start=outdata->allbuf+152,*outiter=start;
		*outiter++='{';
		outiter = givebgnow(outiter);
		outiter = getdeltastr(outiter);
		outiter = givebuckets(outiter);
//		outiter = givecage(outiter);
		--outiter;
		*outiter++ = '}';
		mkheader(start, outiter, false, outdata);
		return true;
		}
    //return givenothing(outdata);
	}
/*
bool giveproperties(const char *input,int inputlen,recdata *outdata) {
//	constexpr const int len=sizeof(statusformat)+50;
  
   const std::string_view nothing="{}\n";

	outdata->allbuf=new(std::nothrow) char[nothing.size()+512];
    char *start=outdata->allbuf+152;
    memcpy(start,nothing.data(),nothing.size());
    mkheader(start,start+nothing.size(),false,outdata);
    //       char *start=outdata->allbuf+152;

    return true;
	}
bool givestrange(const char *input,int inputlen,recdata *outdata) {
	const std::string_view strange="\n";
	outdata->allbuf=new(std::nothrow) char[strange.size()+512];
    char *start=outdata->allbuf+152;
    memcpy(start,strange.data(),strange.size());
    mkheader(start,start+strange.size(),false,outdata);
    //       char *start=outdata->allbuf+152;
    return true;
    } 
bool givestrange(const char *input,int inputlen,recdata *outdata) {
	const std::string_view strange=R"(96:0{"sid":"mE7M1PSeHBvTxbRlAAAG","upgrades":["websocket"],"pingInterval":25000,"pingTimeout":5000}2:40)";
	outdata->allbuf=new(std::nothrow) char[strange.size()+512];
    char *start=outdata->allbuf+152;
    memcpy(start,strange.data(),strange.size());
    mkheader(start,start+strange.size(),false,outdata);
    //       char *start=outdata->allbuf+152;
    return true;
    } */
static void nosecret(std::string_view secret, recdata *outdata) {
	constexpr const char nosecrettxt[]="HTTP/1.1 403 Forbidden\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\nsecret_api wrong: %.*s\n";
	constexpr const int formatlen=sizeof(nosecrettxt)-1;
	const int buflen=formatlen+secret.size()+20;
	char *nosecret=outdata->allbuf=new char[buflen];
	int textlen=19+secret.size();
	outdata->len= snprintf(nosecret,buflen,nosecrettxt,textlen,(int)secret.size(),secret.data());
	outdata->start=nosecret;
	}
bool watchcommands(char *rbuf,int len,recdata *outdata) {
	LOGGER("watchcommands len=%d %.*s",len,len,rbuf);
	const char *start=rbuf;
	const char *ends=rbuf+len;
	const char *nl;
	std::string_view foundsecret={nullptr,0};
	bool issha1=false;
	std::string_view toget;
	bool behead=false;
	bool json=false;
	const char reget[]= "GET /";
	const int regetlen=sizeof(reget)-1;
	const char rehead[]= "HEAD /";
	const int reheadlen=sizeof(rehead)-1;
	const char api_secret[]= "api_secret: ";
	const int	api_len=sizeof(api_secret)-1;
	while((nl= std::find(start,ends,'\n'))!=ends) {
		if(!memcmp(start,reget,regetlen)) {
			const char *reststart=start+regetlen;
			toget={reststart,(std::string_view::size_type)(nl-reststart)};
			}
		else {
			if(!memcmp(start,rehead,reheadlen)) {
				const char *reststart=start+reheadlen;
				toget={reststart,(std::string_view::size_type)(nl-reststart)};
				behead=true;
				}
			else {
				if(!memcmp(start,api_secret,3)) {
					if(!memcmp(start+4,api_secret+4,api_len-4)) {
						const char *keystart=start+api_len;
						auto end=nl-1;
						if(*end!='\r')
							++end;
						foundsecret={keystart,(size_t )(end-keystart)};
						if(start[3]=='-') {
							issha1=true;
							}
						else  {
							if(start[3]!='_') { 
								foundsecret={nullptr,0};
								}
							}
						}
					}
				else {
					constexpr const char jsonstr[]="Accept: application/json";
					if(!memcmp(start,jsonstr,sizeof(jsonstr)-1)) {
						json=true;
						}
					}
				}
			}
		start=nl+1;
		if(*start==0xD||*start=='\n')
			break;
		}
	
		
	int seclen=settings->data()->apisecretlength;
	if(seclen) {
		if(foundsecret.size()==0) {
			const char *start=toget.data();
			const char *end=start+toget.size();
			static constexpr const char token[]="token=";
			static constexpr const int tokenlen=sizeof(token)-1;
//			const char *hit=std::search(start,end,token,token+tokenlen,[](const char *one,const char *two) { return !memcmp(one,two,tokenlen);});
			const char *hit=std::search(start,end,token,token+tokenlen);
			if(hit!=end) {
				hit+=tokenlen;	
				const auto len=strcspn(hit," &");
				foundsecret={hit,len};
				LOGGER("has token %.*s#%zd\n",(int)len,foundsecret.data(),foundsecret.size());
				}
			}
		const char *realsecret;
		if(issha1) {
			realsecret=sha1secret.data();
			seclen=sha1secret.size();
			}
		else {
			realsecret=settings->data()->apisecret;
			}
		if(seclen!=foundsecret.size()||memcmp(realsecret,foundsecret.data(),seclen)) {
			LOGGER("%s#%d!=%.*s#%zd \n", realsecret,seclen,(int)foundsecret.size(), foundsecret.data(),foundsecret.size());
			nosecret(foundsecret, outdata) ;
			return false;
			}
		else {
			LOGSTRING("secret matched\n");
			}
		}
	if(!toget.size()) {
//		outdata->start= servererrorstr.data();
//		outdata->len=servererrorstr.size();
		givesite(outdata);
		return true;
		}
	LOGGER("toget=%.*s\n",(int)toget.size(),toget.data()); //to set getargs in the beginning and use everywhere
std::string_view sgv="sgv.json";
	if(!memcmp(sgv.data(),toget.data(),sgv.size())) {
		return sgvinterpret(toget.data()+sgv.size(),toget.size()-sgv.size(),behead,outdata,false);
		}

///api/v1/entries.json
//https:///api/v1/entries?count=2
///api/v1/entries/sgv.txt?c
//https://dnarnianbg.herokuapp.com/api/v1/entries/sgv?count=4
	std::string_view api="api/v1/entries";
	if(!memcmp(api.data(),toget.data(),api.size())) {
		const char *ptr=toget.data()+api.size();;
		std::string_view sgvjson="/sgv.json";
		if(!memcmp(sgvjson.data(),ptr,sgvjson.size())) {
			return sgvinterpret(ptr+sgvjson.size(),toget.size()-sgvjson.size()-api.size(),behead,outdata);
			}
		else {
			std::string_view api2=".json";
			if(!memcmp(api2.data(),ptr,api2.size())) {
				return sgvinterpret(ptr+api2.size(),toget.size()-api2.size()-api.size(),behead,outdata);
				}
			else {
				const constexpr std::string_view api2="/sgv";
				if(!memcmp(api2.data(),ptr,api2.size())) {
					ptr+=api2.size();
					{const constexpr std::string_view ext1=".txt";
					const constexpr std::string_view ext2=".tsv";
					 if(!memcmp(ext1.data(),ptr,ext1.size())||!memcmp(ext2.data(),ptr,ext2.size())) 
						return givesgvtxt(ptr+ext1.size(),toget.size()-api2.size()-api.size()-ext1.size(),outdata,9);
					 }
					const constexpr std::string_view ext1=".csv";
					 if(!memcmp(ext1.data(),ptr,ext1.size())) 
						return givesgvtxt(ptr+ext1.size(),toget.size()-api2.size()-api.size()-ext1.size(),outdata,',');

					if(*ptr==' '||*ptr=='?') {
						int sizeleft=toget.size()-api2.size()-api.size();
						if(json)
							return sgvinterpret(ptr,sizeleft,false,outdata);
						else
							return givesgvtxt(ptr,sizeleft,outdata,9);
						}


				//	return givesgvtxt(ptr+api2.size(),toget.size()-api2.size()-api.size(),behead,outdata);
					}
				else  {
					if(*ptr==' '||*ptr=='?') {
				//		return sgvinterpret(ptr+api2.size(),toget.size()-api2.size()-api.size(),behead,outdata);
						if(json)
							return sgvinterpret(ptr,toget.size()-api.size(),false,outdata);
						else
							return givesgvtxt(ptr,toget.size()-api.size(),outdata,9);
						}
					}
				}
			}
		}
std::string_view status="api/v1/status";
	if(!memcmp(status.data(),toget.data(),status.size())) {
		const char *end=toget.data()+status.size();
		if(*end==' '||*end=='/')
			return givestatushtml(outdata);
		else
			return givestatus(outdata);
		}
		/*
std::string_view status="api/v1/status.json";
	if(!memcmp(status.data(),toget.data(),status.size())) {
		return givestatus(outdata);
		} */
std::string_view properties="api/v2/properties";
const auto propsize= properties.size();
	if(!memcmp(properties.data(),toget.data(),propsize)) {
		return giveproperties(toget.data()+propsize,toget.size()-propsize,outdata);
		}

std::string_view current="api/v1/entries/current";
const auto cursize= current.size();
	if(!memcmp(current.data(),toget.data(),cursize)) {
		return givecurrent(outdata);
		}
std::string_view treatments="api/v1/treatments";
const auto treatsize= treatments.size();
	if(!memcmp(treatments.data(),toget.data(),treatsize)) {
		return givetreatments(toget.data()+treatsize,toget.size()-treatsize,outdata);
		}

constexpr const std::string_view devicestatus="api/v1/devicestatus.json";
	if(!memcmp(devicestatus.data(),toget.data(),devicestatus.size())) {
		return showdevicestatus(outdata);
		} 

constexpr const std::string_view pebble="pebble";
	if(!memcmp(pebble.data(),toget.data(),pebble.size())) {
		return pebbleinterpret(toget.data()+pebble.size(),toget.size()-pebble.size(),outdata);
		} 
std::string_view socket="socket.io";
const auto socketsize= socket.size();
	if(!memcmp(socket.data(),toget.data(),socketsize)) {
		return givenothing(outdata);
//		return givestrange(socket.data()+socketsize,toget.size()-propsize,outdata);
		}

		/*
std::string_view pebble="pebble";
	if(!strcmp(pebble.data(),toget.data())) {
		return pebbleinterpret(toget.data()+pebble.size(),toget.size-pebble.size());
		} */
std::string_view index="index.html";
const auto indexsize= index.size();
	if(toget.data()[0]==' '||!memcmp(index.data(),toget.data(),indexsize)) {
		return givesite(outdata);
		}


void wrongpath(std::string_view toget, recdata *outdata);
	wrongpath(toget,outdata);
	return false;
	}



void wrongpath(std::string_view toget, recdata *outdata) {
//	const char notfoundtxt[]="HTTP/1.0 404 Not Found\r\nAccess-Control-Allow-Origin: *\r\nContent-Type: text/plain\r\nContent-Length: ";
	const char notfoundtxt[]="HTTP/1.0 404 Not Found\r\nContent-Type: text/plain\r\nContent-Length: ";
	constexpr const int startlen=sizeof(notfoundtxt)-1;
	constexpr int maxant=4096;
	char *notfound=outdata->allbuf=new char[maxant];
	memcpy(notfound,notfoundtxt,startlen);
	const char notpath[]="Path not found: ";
	constexpr const int notpathlen=sizeof(notpath)-1;
	int pathlen=std::find(toget.begin(),toget.end(),' ')-toget.begin();
	constexpr const int maxpath=(maxant-startlen-30);
	if(pathlen>maxpath)
		pathlen=maxpath;
	int reslen=notpathlen+pathlen;
	char *iter= notfound+startlen;
	iter+=sprintf(iter,"%dr\n\r\n",reslen);
	memcpy(iter,notpath,notpathlen);
	iter+=notpathlen;
	memcpy(iter,toget.data(),pathlen);
	iter[pathlen]='\0';
	outdata->len=iter+pathlen-notfound;
	outdata->start=notfound;
	}

//There is another option `all_data=Y` which you can use to get data additionally from the previous sensor session.




template <typename Num> static const char *readnum(const char *start,const char *ends,Num &num) {
	auto [ptr, ec]=std::from_chars(start, ends, num);
	switch(ec)  {
		 case std::errc():break;
		 case std::errc::invalid_argument:
			LOGSTRING("That isn't a number. ");
		case  std::errc::result_out_of_range:
			LOGSTRING("This number is larger than an int. ");
		default: 
			LOGGER("Error %d\n",ec);
			return nullptr;
		}
	return ptr;
	}

void mkheader(char *outstart,char *outiter,const bool headonly,recdata *outdata)  {
//	const char header1[]="HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\nContent-Type: application/json\r\nContent-Length: " ;
	const char header1[]="HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: " ;
	const int header1len=sizeof(header1)-1;
	int uitlen=outiter-outstart;
	constexpr const int maxlen=20;
	char lenstr[maxlen];
	const int getlen=snprintf(lenstr,maxlen,"%d\r\n\r\n",uitlen);
	const int headerlen=header1len+getlen;
	char * const startheader=outstart-headerlen;
	memcpy(startheader,header1,header1len);
	LOGGER("direct: len=%d\n",header1len);
	logwriter(startheader,80);
	memcpy(startheader+header1len,lenstr,getlen);
	int totlen;
	if(headonly) {
		totlen=headerlen;
		startheader[totlen]='\0';
		}
	else
		 totlen=outiter-startheader;
	LOGSTRING("All:\n");
	logwriter(startheader,totlen);
	outdata->start=startheader;
	outdata->len=totlen;

	}
class Sgvinterpret {
	bool briefmode=false,sensorinfo=false,alldata=false,noempty=false;
  public:
	Sgvinterpret(bool all=true): alldata(all) {}
	int datnr=24;
	int interval=270;
	uint32_t lowerend=0,higherend=INT32_MAX;
	const char *event=nullptr;
	bool carb=false;
	bool insulin=false;
	bool getargs(const char *start,int len) ;
	bool getdata(bool headonly,recdata *outdata) const;

	char *makedata(recdata *outdata ) const;
private:
	static char *dontbrief(char *outiter,const char *name,const ScanData *iter) ;
	char *firstdata(char *outiter,time_t starttime,uint32_t dattime) const ;
	char *writeitem(char *outiter,int datit, const ScanData *iter,const char *sensorname,const time_t starttime) const;
	//void mkheader(char *outstart,char *outiter,const bool headonly,recdata *outdata) const;
	bool getitems(char *&outiter,const int  datnr) const;
}; 

bool givesgvtxt(const char *input,int inlen,recdata *outdata,char sep) {
	Sgvinterpret pret;
	pret.datnr=10;
	if(!pret.getargs(input,inlen))  {
		wrongpath({input,(size_t)inlen},outdata);
		return false;
		}
	LOGGER("givesgvtxt datnr=%d\n",pret.datnr);
	return givesgvtxt(pret.datnr,pret.interval,pret.lowerend,pret.higherend,outdata,sep);
	}
char *Sgvinterpret::makedata(recdata *outdata ) const {
	char *output=outdata->allbuf= new(std::nothrow) char[512+datnr*360];
	if(output==nullptr)
		return nullptr;
	return output+152;
	}
char *Sgvinterpret::dontbrief(char *outiter,const char *name,const ScanData *iter) {
	outiter+=sprintf(outiter,R"("_id":"%s#%d","device":"Juggluco","dateString":")", name,iter->id);
	const char *startdate=outiter;
	int len=Tdatestring(iter->t,outiter);
	outiter+=len;
	std::string_view st= R"(","sysTime":")";
	memcpy(outiter,st.data(),st.size());
	outiter+=st.size();
	memcpy(outiter,startdate,len);
	outiter+=len;
	*outiter++='\"';
	*outiter++=',';
	return outiter;
	}

inline int mktmmin(const struct tm *tmptr) {
	return tmptr->tm_min;
	}
/*	
inline int mktmmin(const struct tm *tmptr) {
	if(tmptr-> tm_sec<30)
		return tmptr->tm_min;
	return tmptr->tm_min+1;
	} */
char *Sgvinterpret::firstdata(char *outiter,time_t starttime,uint32_t dattime) const {
	bool mmol=settings->usemmolL();
	string_view hint=mmol?(R"(,"units_hint":"mmol")"):(R"(,"units_hint":"mgdl")");
	LOGGER("mmol=%d %s\n",mmol,hint.data());
	memcpy(outiter,hint.data(),hint.size());
	outiter+=hint.size();
	if(sensorinfo) {
		int backhour=(dattime-starttime)/(60*60);
		int days=backhour/24;
		int hourleft=backhour%24;
		struct tm tmtim;
		 localtime_r(&starttime, &tmtim);
		 outiter+=sprintf(outiter,R"PRE(,"sensor_status":"%02d-%02d-%02d %02d:%02d (%dd %dh)")PRE",tmtim.tm_mday,tmtim.tm_mon+1,tmtim.tm_year-100,tmtim.tm_hour,mktmmin(&tmtim),days,hourleft);
		 }
	return outiter;
	}
char *Sgvinterpret::writeitem(char *outiter,int datit, const ScanData *iter,const char *sensorname,const time_t starttime) const {
	LOGGER("writeitem %d\n",datit);
	  if(datit>0) {
		*outiter++=',';
		*outiter++='\n';
		}
	  *outiter++='{';
	   if(!briefmode) {
		outiter=dontbrief(outiter,sensorname,iter);
		}
	 double delta= getdelta(iter->ch);
//	 double delta= isnan(iter->ch)?0:iter->ch*deltatimes;
	 std::string_view name=getdeltaname(iter->ch);
	 outiter+=sprintf(outiter,R"("date":%d000,"sgv":%d,"delta":%.3f,"direction":"%s","noise":1)",iter->t,iter->getmgdL(),delta,name.data());
	    if (!briefmode) {
		longlongtype mgdL1000=iter->getmgdL()*1000;
		outiter+=sprintf(outiter,R"(,"filtered":%lld,"unfiltered":%lld,"rssi":100,"type":"sgv")",mgdL1000,mgdL1000);
		}
	   if(datit==0) {
		outiter=firstdata(outiter,starttime,iter->t);
		}
	  *outiter++='}';
	return outiter;
	}





bool Sgvinterpret::getitems(char *&outiter,const int  datnr) const {

	return  ::getitems(outiter,datnr, lowerend,higherend,alldata, interval,[this](char *outiter,int datit, const ScanData *iter,const char *sensorname,const time_t starttime)
				{return writeitem(outiter, datit, iter,sensorname, starttime);});

	}
//{"_id":"6401c40addf76d1473eb7d02","timestamp":1677837282000,"eventType":"<none>","enteredBy":"xdrip pos:6.52","notes":"Swim → Aaps → nog meer","uuid":"6401c40addf76d1473eb7d02","created_at":"2023-03-03T09:54:42.000Z","sysTime":"2023-03-03T10:54:42.000+0100","utcOffset":0,"carbs":null,"insulin":null},
//notes	"AndroidAPS started - realme RMX2202"
#include "nightnumcategories.h"
#include "curve/numiter.h"
	/*
    "_id": "a486879b595f46f7bbc8e20b",
    "timestamp": 1677843182779,
    "eventType": "<none>",
    "enteredBy": "xdrip",
    "uuid": "a486879b-595f-46f7-bbc8-e20b11c1b9d3",
    "carbs": 50,
    "insulinInjections": "[]",
    "created_at": "2023-03-03T11:33:02.000Z",
    "sysTime": "2023-03-03T12:33:02.779+0100",
    "utcOffset": 0,
    "insulin": null*/

static char *writetreatment(char *outiter,const int numbase,const int pos,const Num*num) {
	const int type=num->type;
	if(type>=settings->varcount()||!settings->data()->Nightnums[type].kind)
		return outiter;
	const time_t tim=num->gettime();
        struct tm tmbuf;
        gmtime_r(&tim, &tmbuf);
	outiter+=sprintf(outiter,R"({"_id":"num%d#%d","eventType":"<none>","insulinInjections":"[]","enteredBy":"Juggluco","created_at":"%d-%02d-%02dT%02d:%02d:%02d.000Z",)",numbase,pos,tmbuf.tm_year+1900,tmbuf.tm_mon+1,tmbuf.tm_mday, tmbuf.tm_hour, tmbuf.tm_min,tmbuf.tm_sec);

	float w=0.0f;
	 if((w=longNightWeight(type))!=0.0f) {
	 	addar(outiter,R"("notes":"Long-Acting",)");
	 	}
	else { if((w=rapidNightWeight(type))!=0.0f) {
	 	addar(outiter,R"("notes":"Rapid-Acting",)");
	 	}
		}
	if(w!=0.0f) {
		outiter+=sprintf(outiter,R"("carbs":null,"insulin":%g},)",w*num->value);
		}
	else {
		if((w=carboNightWeight(type) )!=0.0f) {
			outiter+=sprintf(outiter,R"("carbs":%g,"insulin":null},)",w*num->value);
			}
		else {
			std::string_view typestr=settings->getlabel(type);
			outiter+=sprintf(outiter,R"("notes":"%s %g","carbs":null,"insulin":null},)",typestr.data(),num->value);
			}
		}
	return outiter;
	}
	extern vector<Numdata*> numdatas;
/*
template <class T>
static void toend(NumIter<T> *numiters,int maxnum) {
	for(int i=0;i<maxnum;i++) {
		numiters[i].iter=numiters[i].end;
		}
	} */

static bool givetreatments(const char *args,int argslen, recdata *outdata)  {
	Sgvinterpret pret;
	pret.datnr=100;
	std::string_view json{".json"	};
	
	if(!memcmp(args,	json.data(),json.size())) {
		args+=json.size();
		argslen-=json.size();

		}
	else {
		if(args[0]=='/') {
			++args;
			--argslen;
			}
		}
	
	if(!pret.getargs(args,argslen)) {
		return false;
		}
	
	const int basecount=numdatas.size();
	LOGSTRING("givetreatments:\n");
	NumIter<Num> numiters[basecount];
	for(int i=0;i<basecount;i++) {
		auto [low,high]= numdatas[i]->getInRange(pret.lowerend,pret.higherend); 

		numiters[i].begin=low;
		numiters[i].iter=numiters[i].end=high-1;
		numiters[i].bytes=sizeof(Num);
		/*
		if(low!=high) {
			time_t tim=low->time;
			LOGGER("low=%d %s",tim,ctime(&tim));
			tim=(high-1)->time;
			LOGGER("high=%d %s",tim,ctime(&tim));
			} */
		}

	int count= pret.datnr;
	char *outstart=pret.makedata(outdata ); 
	char *outiter=outstart;
	*outiter++='[';
	bool carb=pret.carb;
	bool insulin=pret.insulin;
	if(settings->data()->saytreatments&&!pret.event) {
		for(int i=0;i<count;) {
			auto [ind,num]=findnewestwith(numiters,basecount);
			if(!num) {
				LOGGER("no values %d %p\n",ind,num);
				break;
				}
			if(carb) {
				if(carboNightWeight(num->type) ==0.0f) 
					continue;
				}
			if(insulin) {
				const int type=num->type;
				 if(longNightWeight(type)==0.0f&&rapidNightWeight(type)==0.0f) 
				 	continue;
				}
			int pos=num-numdatas[ind]->begin();
			char *out=writetreatment(outiter,ind,pos,num);
			if(out!=outiter) {
				outiter=out;
				i++;
				}
			}
		if(outiter[-1]==',') --outiter;
		}
	 *outiter++=']';
	 *outiter++='\n';
       mkheader(outstart,outiter, false,outdata);
       return true;
	}

bool Sgvinterpret::getdata(bool headonly,recdata *outdata) const {
	if(!sensors)	
		return false;
	LOGGER("count=%d\n",datnr);

	char *outstart=makedata(outdata);
	if(!outstart)
		return false;
	char *outiter=outstart;
	*outiter++='[';
	if(!getitems(outiter,datnr) )
		return false;
	if(outiter==(outstart+1)&&noempty) {
		*outstart='\0';
		outiter=outstart;
		}
	else  {
	  *outiter++=']';
	  *outiter++='\n';
	  }
       mkheader(outstart,outiter, headonly,outdata);
	return true;
	}


static time_t readtime(const char *input) {
	struct tm tmbuf {.tm_isdst=0,.tm_gmtoff=0};
	if(input[10]=='T') 
	  strptime(input, "%Y-%m-%dT%H:%M:%S", &tmbuf);
	else
	  strptime(input, "%Y-%m-%d", &tmbuf);
	time_t	 tim=timegm(&tmbuf);
	return tim;
	}
  
/*[=%5B
]=%5D
$=%24
=%3D */
#define LOGID "watchserver "

#define toshort(str)  (str[0]|(str[1]<<8))
int rewriteperc(char *start,int len) {
	char *ends=start+len;
	char *iter=start;
	char *uititer=start,*next;
	while(true) {
		if((next=std::find(iter,ends,'%'))==ends) {
			if(iter!=uititer) {
				int left=(ends-iter);
				memmove(uititer,iter,left);
				return uititer+left-start;;
				}
			return len;
			}
		int bijlen=(next-iter);
		memmove(uititer,iter,bijlen);
		uititer+=bijlen;
		++next;
		switch(toshort(next)) {
			case toshort("5B"): *uititer++='[';break;
			case toshort("5D"): *uititer++=']';break;
			case toshort("24"): *uititer++='$';break;
			case toshort("3D"): *uititer++='=';break;
			default: LOGGER(LOGID "strange char %.3s\n",next-1); memmove(uititer,next-1,3);uititer+=3;
			}
		iter=next+2;
		}
	}
bool setitervar(const char *&iter,std::string_view cond,bool &var) {
	if(memcmp(iter,cond.data(),cond.size()))
		return false;
	var=true;
	iter+=cond.size();
	return true;
	}
  bool Sgvinterpret::getargs(const char *start,int lenin) {
	 int 	len=rewriteperc(const_cast<char *>(start),lenin);
	LOGGER("after Sgvinterpret::getargs(%.*s#%d)\n",len,start,len);

	const char *ends=start+len;
	start++;
	for(const char *iter=start;iter<ends;iter=std::find(iter,ends,'&')+1) {
		std::string_view count="count=";
		if(!memcmp(iter,count.data(),count.size())) {
			iter+=count.length();
			if(!(iter=readnum<int>(iter,ends,datnr))||datnr<0) {

				return false;
				}
			}
		else {
			std::string_view brief="brief_mode=";
			if(setitervar(iter,brief,briefmode)) 
				continue;
			std::string_view sensor="sensor=";
			if(setitervar(iter,sensor,sensorinfo))	 
				continue;
			std::string_view all_data="all_data=";
			if(setitervar(iter,all_data,alldata))
				continue;
			std::string_view no_empty="no_empty=";
			if(setitervar(iter,no_empty,noempty)) 
				continue;
			std::string_view intervalstr="interval=";
			if(!memcmp(iter,intervalstr.data(),intervalstr.size())) {
				iter+=intervalstr.length();
				if(!(iter=readnum<int>(iter,ends,interval))) {
					return false;
					}
				continue;
				}
		 std::string_view findstr="find[";
		 if(!memcmp(iter,findstr.data(),findstr.size())) {
			iter+=findstr.size();
			std::string_view datestr="date";
			if(!memcmp(iter,datestr.data(),datestr.size())) {
				iter+=datestr.size();
				if(!memcmp(iter,"][$",3)) {
					iter+=3;
					std::string_view greater="gte]=";
					std::string_view greater3="gt]=";
					if(!memcmp(iter,greater.data(),greater.size())||(greater=greater3, !memcmp(iter,greater.data(),greater.size()))) {
						iter+=greater.length();
						const char *ptr;
						longlongtype tmp;
						if(!(ptr=readnum<longlongtype>(iter,ends,tmp))) {
							LOGGER("%s readnum failed '%s'\n",greater.data(),iter);
							return false;
							}
						lowerend=tmp/1000;
						if(greater==greater3) {
							++lowerend;
							}
						iter=ptr;
						LOGGER("greater than %d\n",lowerend);
						}
					else {
						std::string_view smaller="lte]=";
						std::string_view smaller2="lt]=";
						if((!memcmp(iter,smaller.data(),smaller.size()))||(smaller=smaller2,!memcmp(iter,smaller.data(),smaller.size()))) {
							iter+=smaller.length();
							longlongtype tmp;	
							if(!(iter=readnum<longlongtype>(iter,ends,tmp))) {
								LOGGER("%s= readnum failed\n",smaller.data());
								return false;
								}
							higherend=tmp/1000LL;
							if(smaller!=smaller2) {
								++higherend;
								}
							LOGGER("smaller than %d\n",higherend);
							}
						}
					}
					else {
					std::string_view strdate="String][$";
					if(!memcmp(iter,strdate.data(),strdate.size())) {

					iter+=strdate.size();
					std::string_view greater="gte]="; //TODO greater or equal
					std::string_view greater2="gt]=";
					if(!memcmp(iter,greater.data(),greater.size())||(greater=greater2, !memcmp(iter,greater.data(),greater.size()))) {
						iter+=greater.length();
						lowerend=readtime(iter);
//											if(greater==greater2||greater==greater4)
						if(greater==greater2)
							lowerend++;
						iter+=10;
						LOGGER("greater than %d\n",lowerend);
						}
					else {
						std::string_view smaller="lte]="; //TODO smaller or equal
						std::string_view smaller2="lt]=";
					if((!memcmp(iter,smaller.data(),smaller.size()))||(smaller=smaller2,!memcmp(iter,smaller.data(),smaller.size()))) {
							iter+=smaller.length();
							higherend=readtime(iter);
							if(smaller!=smaller2)
								higherend++;
							iter+=10;
							LOGGER("smaller than %d\n",higherend);
							}
					}
					}
					}
				continue;
				}
			std::string eventtype="eventType]=";
			if(!memcmp(iter,eventtype.data(),eventtype.size())) {
				iter+=eventtype.size();
				event=iter;
				continue;
				}
			std::string_view created="created_at][$";
			if(!memcmp(iter,created.data(),created.size())) {
				iter+=created.size();
				std::string_view greater="gte]=";
				std::string_view greater2="gt]=";
				if(!memcmp(iter,greater.data(),greater.size())||(greater=greater2, !memcmp(iter,greater.data(),greater.size()))) {
						iter+=greater.length();
						lowerend=readtime(iter);
						if(greater==greater2)
							lowerend++;
						iter+=10;
						LOGGER("greater than %d\n",lowerend);
						continue;
						}
				std::string_view smaller="lte]=";
				std::string_view smaller2="lt]=";
				if((!memcmp(iter,smaller.data(),smaller.size()))||(smaller=smaller2,!memcmp(iter,smaller.data(),smaller.size()))) {
						iter+=smaller.length();
						higherend=readtime(iter);
						if(smaller!=smaller2)
							higherend++;
						iter+=10;
						LOGGER("smaller than %d\n",higherend);
						continue;
						}
				continue;
				}
			std::string_view carbstr=R"(carbs][$exists])";
			if(!memcmp(iter,carbstr.data(),carbstr.size())) {
				carb=true;
				continue;
				}
			std::string_view insulinstr=R"(insulin][$exists])";
			if(!memcmp(iter,insulinstr.data(),insulinstr.size())) {
				insulin=true;
				continue;
				}
			}
			}
		
	}
	if(!datnr) {
		if(lowerend|| higherend!=INT32_MAX) //needed for Cockpit
			datnr=100000;

		}
	return true;
	};
static bool	 sgvinterpret(const char *start,int len,bool headonly,recdata *outdata,bool all) {
	Sgvinterpret pret(all);
	if(!pret.getargs(start,len)) {

		return false;
		}
	LOGGER("lowerend=%d higherend=%d\n",pret.lowerend,pret.higherend);
	return pret.getdata(headonly,outdata);
	}


/*
HTTP/1.0 200 OK
Date: Sat, 26 Feb 2022 19:08:27 GMT
Access-Control-Allow-Origin: *
Content-Type: application/json
Content-Length: 6890

*/

#endif
