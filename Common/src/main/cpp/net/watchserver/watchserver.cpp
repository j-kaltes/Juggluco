#ifndef NOLOG
#define SETVALUE 1
#endif
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

#define LOGGERWEB(...) LOGGER("webserver: " __VA_ARGS__)
#define LOGARWEB(...) LOGAR("webserver: " __VA_ARGS__)
typedef std::conditional<sizeof(long long) == sizeof(int64_t), long long, int64_t >::type longlongtype;

#ifdef NOTAPP
#include "cmdline/jugglucotext.h"
#else
#include "curve/jugglucotext.h"
#endif
extern jugglucotext engtext;

static void watchserverloop(int *sockptr,bool secure) ;
static bool startwatchserver(bool secure,int port,int *sockptr) {
	const char *servername=secure?"SSL WATCHSERVER":"WATCHSERVER";
	LOGGERWEB("%s\n",servername);
	constexpr const int maxport=20;
	char watchserverport[maxport];
	snprintf(watchserverport,maxport,"%d",port);

        prctl(PR_SET_NAME, servername, 0, 0, 0);
	struct addrinfo hints{.ai_flags=AI_PASSIVE,.ai_family=AF_UNSPEC,.ai_socktype=SOCK_STREAM};
	int sock;
	{
	struct addrinfo *servinfo=nullptr;
	destruct serv([&servinfo]{ if(servinfo)freeaddrinfo(servinfo);});
	if(
#ifndef NOLOG
	int status=
#endif
	getaddrinfo(nullptr,watchserverport,&hints,&servinfo)) {
		LOGGERWEB("getaddrinfo: %s\n",gai_strerror(status));
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

#include "nightnumcategories.h"
#include "curve/numiter.h"
extern vector<Numdata*> numdatas;
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
				LOGGERWEB("%s\n",error.data());
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
		LOGGERWEB("accept(%d,%p,%d)\n",serversock,addrptr,sin_size);
		int new_fd = accept(serversock, addrptr, &sin_size);
		LOGGERWEB("na accept(serversock)=%d\n",new_fd);
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
				LOGARWEB("exit"); return;
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
				LOGGERWEB("%swatchserver: reject connection from %s getownip failed\n",secure?"secure":"",namestr);
				close(new_fd);
				continue;
				}
			if(!sameaddress(addrptr, &own_addr) ) {
				const namehost myname(&own_addr);
				LOGGERWEB("%swatchserver: reject connection from %s same address %s failed",secure?"secure":"",namestr,(const char *)myname);
				close(new_fd);
				continue;
				}
		  }
	      LOGGERWEB("%swatchserver: got connection from %s sock=%d\n" ,secure?"secure":"",namestr ,new_fd);
		void handlewatch(int sock) ;
		try {
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
		catch (const std::exception& e)     {
			LOGGERWEB("exception %s\n",e.what() );
		}
		catch (...)     {
			LOGARWEB("exception");
		}
		}
	}
static bool	plainwatchcommands(int sock);
void handlewatch(int sock) {
      const char threadname[]="watchconnect";
      LOGGERWEB("handlewatch %d\n",sock);
      prctl(PR_SET_NAME, threadname, 0, 0, 0);
extern void sendtimeout(int sock,int secs);
extern void receivetimeout(int sock,int secs) ;
 	receivetimeout(sock,60);
 	sendtimeout(sock,5*60);
	
	plainwatchcommands(sock);
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



static bool	 sgvinterpret(const char *start,int len,bool headonly, bool gmt,recdata *data,bool all=true) ;


static bool givetreatments(const char *args,int argslen, recdata *data) ;


static bool sendall(int sock ,const char *buf,int buflen) {
        int itlen,left=buflen;
        LOGGERWEB("sock=%d sendall len=%d\n",sock,buflen);
        for(const char *it=buf;(itlen=send(sock,it,left,0))<left;) {
		int waser=errno;
                LOGGERWEB("len=%d\n",itlen);
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
        LOGARWEB("success sendall");
        return true;
        }
bool watchcommands(char *rbuf,int len,recdata *outdata,bool secure) ;
static bool plainwatchcommands(int sock) {
	constexpr const int RBUFSIZE=4096;
	char rbuf[RBUFSIZE];
	int len;
	if((len=recvni(sock,rbuf,RBUFSIZE))==-1) {
		servererror(sock);
		return false;
		}
	if(len==0) {
		LOGARWEB("shutdown");
		return false;
		}
	struct recdata outdata;

	if(stopconnection)
		return false;
	bool res=watchcommands(rbuf, len,&outdata,false); 
	bool res2=sendall( sock ,outdata.data(),outdata.size()) ;
	return res&&res2&&!stopconnection;
	}

void mktypeheader(char *outstart,char *outiter,const bool headonly,recdata *outdata,std::string_view type) ;
 void mkjsonheader(char *outstart,char *outiter,const bool headonly,recdata *outdata) ;
static bool givestatushtml(recdata *outdata) {
//static	constexpr const char status[]="HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: 19\r\n\r\n<h1>STATUS OK</h1>\n";
//static	constexpr const char status[]="HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: 19\r\n\r\n<h1>STATUS OK</h1>";
static	constexpr const char statusstart[]="HTTP/1.1 200 OK\r\nX-Powered-By: Express\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Methods: GET,PUT,POST,DELETE,OPTIONS\r\nAccess-Control-Allow-Headers: Content-Type, Authorization, Content-Length, X-Requested-With\r\nVary: Accept, Accept-Encoding\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: 18\r\nDate: ";

static	constexpr const char statusend[]=" GMT\r\nConnection: keep-alive\r\nKeep-Alive: timeout=5\r\n\r\n<h1>STATUS OK</h1>";

	constexpr const int totlen=sizeof(statusstart)+sizeof(statusend)+60;
	char *status=outdata->allbuf=new(std::nothrow) char[totlen];
	constexpr int startlen=sizeof(statusstart)-1;
	memcpy(status,statusstart,startlen);
	char *ptr=status+startlen;
	auto nu=time(nullptr);
	struct tm stmbuf;
	gmtime_r(&nu,&stmbuf);
	ptr+=sprintf(ptr,R"(%s, %02d %s %04d %02d:%02d:%02d)",
engtext.daylabel[stmbuf.tm_wday],stmbuf.tm_mday, engtext.monthlabel[stmbuf.tm_mon],stmbuf.tm_year+1900,
stmbuf.tm_hour,stmbuf.tm_mday,stmbuf.tm_sec);
	const int statusendlen=sizeof(statusend)-1;
	memcpy(ptr,statusend,statusendlen);
	
	outdata->start=status;
	outdata->len=ptr-status+statusendlen;
	return true;
}
static bool outofmemory(recdata *outdata) {

//	const char notfoundtxt[]="HTTP/1.0 404 Not Found\r\nContent-Type: text/plain\r\nContent-Length: ";
static	constexpr const char status[]="HTTP/1.1 413 Content Too Large\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: 30\r\n\r\n<h1>Server out of memory</h1>\n";
	const int statuslen=sizeof(status)-1;
	outdata->allbuf=nullptr;
	outdata->start=status;
	outdata->len=statuslen;
	return true;
}

/*
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
)"; */
static bool givesite(recdata *outdata,std::string_view hostname,bool secure) {
if(hostname.data()==nullptr) {
	hostname="localhost";
	}
static	constexpr const char webpage1[]="HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: ";
static	constexpr const char webpage2[]="\r\n\r\n"; 
static	constexpr const char webpage3[]=R"(<!DOCTYPE html>
<html>
<head>
   <meta http-equiv="refresh" content="0; url=https://www.juggluco.nl/Juggluco/webserver.html?urlstart=http)";
static	constexpr const char webpageend[]=R"("/>
</head>
<body>
</body>
</html>
)";
static	constexpr const char slashes[]=R"(://)";
int weblen=sizeof(webpage3)+sizeof(webpageend)-3+hostname.size()+sizeof(slashes)+secure;
int totlen=weblen+sizeof(webpage1)+sizeof(webpage2)+5;

	char *start=outdata->allbuf=new(std::nothrow) char[totlen];
	if(!start)
		return false;
	char *endptr=start;
	memcpy(endptr,webpage1,sizeof(webpage1)-1); endptr+=sizeof(webpage1)-1;
	endptr+=sprintf(endptr,"%d",weblen);
	memcpy(endptr,webpage2,sizeof(webpage2)-1); endptr+=sizeof(webpage2)-1;
	#ifndef NOLOG
	const char *startpage=endptr;
	#endif
	memcpy(endptr,webpage3,sizeof(webpage3)-1); endptr+=sizeof(webpage3)-1;
	if(secure)
		*endptr++='s';
	memcpy(endptr,slashes,sizeof(slashes)-1); endptr+=sizeof(slashes)-1;
	memcpy(endptr,hostname.data(),hostname.size());
	endptr+=hostname.size();
	memcpy(endptr,webpageend,sizeof(webpageend)); endptr+=sizeof(webpageend)-1;
	outdata->start=start;
	outdata->len=endptr-start;
	LOGGER("givesite predict=%d pagelen=%d totlen=%d reallen=%d\n",weblen,endptr-startpage,totlen,outdata->len);
	return true;
}

//{"settings":{"units":"mmol","thresholds":{"bgHigh":169,"bgLow":70}}}
static bool givedripstatus(recdata *outdata) {
	constexpr const char format[]= R"({"settings":{"units":"%s","thresholds":{"bgHigh":%.0f,"bgLow":%.0f}}})";
	constexpr const int len=sizeof(format)+6+2*12;
	outdata->allbuf=new(std::nothrow) char[len+512];
	if(!outdata->allbuf) {
		return outofmemory(outdata);
		}
        char *start=outdata->allbuf+152;
	auto halarm=gconvert(settings->data()->ahigh,2);
	auto lowalarm=gconvert(settings->data()->alow,2);
	int alllen=snprintf(start,len,format, settings->getunitlabel().data(),halarm,lowalarm);
	mkjsonheader(start,start+alllen,false,outdata); 
	return true;
	}

static bool givestatusv3right(recdata *outdata) {
	#include "status3.h"
	constexpr const int len=sizeof(statusv3)+20;
	outdata->allbuf=new(std::nothrow) char[len+512];
	if(!outdata->allbuf) {
		return outofmemory(outdata);
		}
        char *start=outdata->allbuf+152;
	int alllen=snprintf(start,len,statusv3, time(nullptr));
	mkjsonheader(start,start+alllen,false,outdata); 
	return true;

	}
static bool givestatusv3(recdata *outdata) {
		return givestatusv3right(outdata);
		}
	/*
static bool givestatusv3wrong(recdata *outdata) {
static	constexpr const char status[]="HTTP/1.1 401 Unauthorized\r\nContent-Type: application/json; charset=utf-8\r\nContent-Length: 61\r\n\r\n" R"({"status":401,"message":"Missing or bad access token or JWT"})";
	const int statuslen=sizeof(status)-1;
	outdata->allbuf=nullptr;
	outdata->start=status;
	outdata->len=statuslen;
	return true;
}
static bool givestatusv3(recdata *outdata) {
	static bool given=false;
	if(given)
		return givestatusv3right(outdata);
	given=true;
	return givestatusv3wrong(outdata);
	}
	*/
//api/v3/entries?sort%24desc=date&limit=6&fields=sgv%2Cdirection%2CsrvCreated&type%24eq=sgv
//api/v3/entries?sort%24desc=date&limit=1&fields=sgv%2Cdirection%2CsrvCreated&type%24eq=sgv
/*
{"status":200,"result":[{"sgv":123,"direction":"Flat","srvCreated":1701290313000},{"sgv":120,"direction":"Flat","srvCreated":1701290308000},{"sgv":125,"direction":"Flat","srvCreated":1701290253000},{"sgv":121,"direction":"Flat","srvCreated":1701290246000},{"sgv":127,"direction":"Flat","srvCreated":1701290193000},{"sgv":121,"direction":"Flat","srvCreated":1701290186000}]}
*/

static bool givenightstatus(recdata *outdata) {
	constexpr static
	#include "status.h"
	auto tim=time(nullptr);
        struct tm tmbuf;
        gmtime_r(&tim, &tmbuf);
	constexpr const int len=sizeof(statusformat)+50;
	outdata->allbuf=new(std::nothrow) char[len+512];
	if(!outdata->allbuf) {
		return outofmemory(outdata);
		}
        char *start=outdata->allbuf+152;
	auto thigh=gconvert(settings->targethigh(),2);
	auto tlow=gconvert(settings->targetlow(),2);
	auto halarm=gconvert(settings->data()->ahigh,2);

	auto lowalarm=gconvert(settings->data()->alow,2);
	const char *unitlabel=settings->getunitlabel().data();
	int alllen=snprintf(start,len,statusformat,tmbuf.tm_year+1900,tmbuf.tm_mon+1,tmbuf.tm_mday, tmbuf.tm_hour, tmbuf.tm_min,tmbuf.tm_sec,0,tim*1000LL,unitlabel,halarm,thigh,tlow,lowalarm);
	mkjsonheader(start,start+alllen,false,outdata); 
	return true;
	}
/*
static bool gzipnightstatus(recdata *outdata) {
	constexpr static
	#include "statusgz.h"
	outdata->allbuf=nullptr;
	outdata->len=sizeof(statusgz);
	outdata->start=(const char *)statusgz;
	return true;
	} */




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
extern int TdatestringGMT(time_t tim,char *buf) ;

static int oneTdatestringGMT(time_t tim,char *buf) {
        struct tm tmbuf;
	gmtime_r(&tim, &tmbuf);
        return sprintf(buf,R"(%04d-%02d-%02dT%02d:%02d:%02d.000Z)",tmbuf.tm_year+1900,tmbuf.tm_mon+1,tmbuf.tm_mday, tmbuf.tm_hour, tmbuf.tm_min,tmbuf.tm_sec);
        }

//		return givetreatments(outdata);
bool givenolist(recdata *outdata) {
	LOGARWEB("givenothing");
	const std::string_view nothing="[]";
	outdata->allbuf=new(std::nothrow) char[nothing.size()+512];
	if(!outdata->allbuf) {
		return outofmemory(outdata);
		}
	char *start=outdata->allbuf+152;
	memcpy(start,nothing.data(),nothing.size());
	mkjsonheader(start,start+nothing.size(),false,outdata);
	return true;
}
bool givenothing(recdata *outdata) {
	LOGARWEB("givenothing");
	const std::string_view nothing="{}\n";
	outdata->allbuf=new(std::nothrow) char[nothing.size()+512];
	if(!outdata->allbuf) {
		return outofmemory(outdata);
		}
	char *start=outdata->allbuf+152;
	memcpy(start,nothing.data(),nothing.size());
	mkjsonheader(start,start+nothing.size(),false,outdata);
	return true;
}

static bool giveservererror(recdata *outdata) {
	outdata->allbuf=nullptr;
	outdata->len=servererrorstr.size();
	outdata->start=servererrorstr.data();
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
/*
static bool showdevicestatus(recdata *outdata) { //seems to be used for battery level. Unimportant.
	return givenothing(outdata);
	} */

static bool	 currentjson(recdata *outdata);
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
	if(!outdata->allbuf)
		return outofmemory(outdata);
    char *start=outdata->allbuf+152,*outiter=start;
    	outiter=textitem(outiter,value)-2;
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
		LOGGERWEB("givesgvtxt new failed %d\n",bufsize);
		return outofmemory(outdata);
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
	LOGARWEB("Before add header");
	addar(startheader,header);
	LOGARWEB("Before add lenstr");
	memcpy(startheader,lenstr,lenlen);
	startheader+=lenlen;
	outdata->len=outiter-outdata->start;
//	LOGGERN(outdata->start,outdata->len);
	return true;
}
extern int getdeltaindex(float rate);
extern std::string_view getdeltanamefromindex(int index) ;

//{"status":[{"now":1699975836641}],"bgs":[{"sgv":"107","trend":4,"direction":"Flat","datetime":1699975773940,"bgdelta":9,"battery":"80","iob":"0.06","bwp":"0.13","bwpo":102,"cob":0}],"cals":[]}
//{"sgv":"10.7","trend":4,"direction":"Flat","datetime":1676804318000}
//{"sgv":"6.9","trend":4,"direction":"Flat","datetime":1703072118000}
static char *pebbleitem(bool mmolL,char *outiter,const ScanData *item) {
	int trend=getdeltaindex(item->ch);
	const char start[]=R"({"sgv":")";
	addar(outiter,start);
	if(mmolL) 
		outiter+=sprintf(outiter,"%.1f", item->getmmolL());
	else
		outiter+=sprintf(outiter,"%d", item->getmgdL());
	return outiter+=sprintf(outiter,R"(","trend":%d,"direction":"%s","datetime":%lld},)",trend,getdeltanamefromindex(trend).data(),item->gettime()*1000LL);
	}

char * givebuckets(char *start) {
	char *outiter=start;
	const char startbuckets[]=R"("buckets":[)";
	addar(outiter,startbuckets);
	int interval=settings->data()->nightinterval;

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

extern uint32_t getnumlasttime() ;
static bool getv2auth(recdata *outdata) {
	static constexpr const char auth[]=R"({"token":"eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhY2Nlc3NUb2tlbiI6Im93bGV0LTIyYjliMjE3YTIwOGJhNzUiLCJpYXQiOjE3MDQ0NTMzOTUsImV4cCI6MTcwNDQ4MjE5NX0.HQzcZ8N_fFcXiGv7qRuiiUXkZFBsdGTvV1O1nvpRWpQ","sub":"owlet","permissionGroups":[["*:*:read"],["*:*:read"]],"iat":%u,"exp":%u})";
   	outdata->allbuf=new(std::nothrow) char[512+sizeof(auth)+2*10];
	if(!outdata->allbuf) {
		return outofmemory(outdata);
		}
    	char *start=outdata->allbuf+150;
	uint32_t now=time(nullptr);
	uint32_t expire=now*60*60*8;
	auto alllen=sprintf(start,auth,now,expire);
	mkjsonheader(start,start+alllen,false,outdata); 
	return true;
	}
static bool getv3modified(recdata *outdata) {
	uint32_t now=time(nullptr);
	auto lastentries= sensors->timelastdata();
	auto lastnum=getnumlasttime();
	#include "lastModified.h"
   	outdata->allbuf=new(std::nothrow) char[512+sizeof(lastModified)+3*10+1];
	if(!outdata->allbuf) {
		return outofmemory(outdata);
		}
    	char *start=outdata->allbuf+150;
	uint32_t lastprofile=99;
	auto alllen=sprintf(start,lastModified,now,lastentries,lastprofile,lastnum);
	mkjsonheader(start,start+alllen,false,outdata); 
	return true;
	}
static bool nothingV3(recdata *outdata) {
static	constexpr const char status[]="HTTP/1.1 200\r\nContent-Type: application/json; charset=utf-8\r\nContent-Length: 26\r\n\r\n" R"({"status":200,"result":[]})";

	const int statuslen=sizeof(status)-1;
	outdata->allbuf=nullptr;
	outdata->start=status;
	outdata->len=statuslen;
	return true;
	}
static bool getv3food(recdata *outdata) {
	return nothingV3(outdata);
	}
static bool getv3entries(recdata *outdata) {
	int count=6;
   	outdata->allbuf=new(std::nothrow) char[512+75*count+200];
	if(!outdata->allbuf) {
		return outofmemory(outdata);
		}
    	char *start=outdata->allbuf+150,*outiter=start;
	constexpr const char begin[]=R"({"status":200,"result":[)";
	addar(outiter,begin);
	uint32_t lowerend=0, higherend=UINT32_MAX;
	 if(!getitems(outiter,count, lowerend,higherend,false, 55,[](char *outiter,int datit, const ScanData *iter,const char *sensorname,const time_t starttime)

				{
				int len=sprintf(outiter,R"({"sgv":%d,"direction":"%s","srvCreated":%u000},)",iter->getmgdL(),getdeltaname(iter->ch).data(),iter->gettime());
				return outiter+len;
				}
				)) {
				return givenothing(outdata);
					}
	
	*--outiter=']';
	++outiter;
	*outiter++='}';	
//	mkjsonheader(start, outiter, false, outdata);

      mktypeheader(start,outiter,false,outdata, "application/json");

	return true;

	}

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
	LOGGERWEB("giveproperties(%s,%d,recdata *outdata) \n",input,inputlen);
//	const char *end=input+inputlen;
 	if(*input++=='/') {
		outdata->allbuf=new(std::nothrow) char[512*inputlen];
		if(!outdata->allbuf)
			return outofmemory(outdata);


		char *start=outdata->allbuf+152,*outiter=start;
		const char *endinput=input+inputlen;
		*outiter++='{';
		while (true) {
			const std::string_view buckets = "buckets";
			if (!memcmp(input, buckets.data(), buckets.size())) {
				LOGARWEB("givebuckets");
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
				mkjsonheader(start, outiter, false, outdata);
				return true;
			}
			++input;
			}
		}
	else {
		outdata->allbuf=new(std::nothrow) char[512*6];
		if(!outdata->allbuf)
    			return outofmemory(outdata);
		char *start=outdata->allbuf+152,*outiter=start;
		*outiter++='{';
		outiter = givebgnow(outiter);
		outiter = getdeltastr(outiter);
		outiter = givebuckets(outiter);
//		outiter = givecage(outiter);
		--outiter;
		*outiter++ = '}';
		mkjsonheader(start, outiter, false, outdata);
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
    mkjsonheader(start,start+nothing.size(),false,outdata);
    //       char *start=outdata->allbuf+152;

    return true;
	}
bool givestrange(const char *input,int inputlen,recdata *outdata) {
	const std::string_view strange="\n";
	outdata->allbuf=new(std::nothrow) char[strange.size()+512];
    char *start=outdata->allbuf+152;
    memcpy(start,strange.data(),strange.size());
    mkjsonheader(start,start+strange.size(),false,outdata);
    //       char *start=outdata->allbuf+152;
    return true;
    } 
bool givestrange(const char *input,int inputlen,recdata *outdata) {
	const std::string_view strange=R"(96:0{"sid":"mE7M1PSeHBvTxbRlAAAG","upgrades":["websocket"],"pingInterval":25000,"pingTimeout":5000}2:40)";
	outdata->allbuf=new(std::nothrow) char[strange.size()+512];
    char *start=outdata->allbuf+152;
    memcpy(start,strange.data(),strange.size());
    mkjsonheader(start,start+strange.size(),false,outdata);
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

void wrongpath(std::string_view toget, recdata *outdata);
static bool apiv1(const char *input,int leftlen,bool behead,bool json,recdata *outdata) {
		const char *posptr=input;
		std::string_view api="entries";
		if(!memcmp(api.data(),posptr,api.size())) {
			posptr+=api.size();
			leftlen-=api.size();
			std::string_view sgvjson="/sgv.json";
			if(!memcmp(sgvjson.data(),posptr,sgvjson.size())) {
				return sgvinterpret(posptr+sgvjson.size(),leftlen-sgvjson.size(),behead,true,outdata);
				}
			else {
				std::string_view api2=".json";
				if(!memcmp(api2.data(),posptr,api2.size())) {
					return sgvinterpret(posptr+api2.size(),leftlen-api2.size(),behead,true,outdata);
					}
				else {
					const constexpr std::string_view api2="/sgv";
					if(!memcmp(api2.data(),posptr,api2.size())) {
						posptr+=api2.size();
						leftlen-=api2.size();

						{const constexpr std::string_view ext1=".txt";
						const constexpr std::string_view ext2=".tsv";
						 if(!memcmp(ext1.data(),posptr,ext1.size())||!memcmp(ext2.data(),posptr,ext2.size())) 
							return givesgvtxt(posptr+ext1.size(),leftlen-ext1.size(),outdata,9);
						 }
						const constexpr std::string_view ext1=".csv";
						 if(!memcmp(ext1.data(),posptr,ext1.size())) 
							return givesgvtxt(posptr+ext1.size(),leftlen-ext1.size(),outdata,',');

						if(*posptr==' '||*posptr=='?') {
							if(json)
								return sgvinterpret(posptr,leftlen,false,true,outdata);
							else
								return givesgvtxt(posptr,leftlen,outdata,9);
							}


						}
					else  {
						if(*posptr==' '||*posptr=='?') {
							if(json)
								return sgvinterpret(posptr,leftlen,false,true,outdata);
							else
								return givesgvtxt(posptr,leftlen,outdata,9);
							}
						else {
							std::string_view current="/current";
							const auto cursize= current.size();
							if(!memcmp(current.data(),posptr,cursize)) {
								posptr+=cursize;
								std::string_view json2=".json";
								if(!memcmp(json2.data(),posptr,json2.size())) 
									return currentjson(outdata);
								else 
									return givecurrent(outdata);
								}
							else {

 								wrongpath({input-7,static_cast<size_t>(leftlen+7)}, outdata);
								return true;
								}
							}
						}
					}
				}
			}
	std::string_view status="status";
	if(!memcmp(status.data(),posptr,status.size())) {
		const char *end=posptr+status.size();
		if(!json&&(*end==' '||*end=='/'))
			return givestatushtml(outdata);
		else {
//			if(gzip) return gzipnightstatus(outdata);
			return givenightstatus(outdata);
			}
		}
	std::string_view treatments="treatments";
	const auto treatsize= treatments.size();
		if(!memcmp(treatments.data(),posptr,treatsize)) {
			return givetreatments(posptr+treatsize,leftlen-treatsize,outdata);
			}
/*
	constexpr const std::string_view devicestatus="devicestatus.json";
		if(!memcmp(devicestatus.data(),posptr,devicestatus.size())) {
			return showdevicestatus(outdata);
			}  */




	wrongpath({input-7,static_cast<size_t>(leftlen+7)}, outdata);
	return true;
 	}
static bool setitertrue(const char *&iter,std::string_view cond) {
	if(strncasecmp(iter,cond.data(),cond.size()))
		return false;
	iter+=cond.size();
	return true;
	}

static bool setitervar(const char *&iter,std::string_view cond,bool &var) {
	if(setitertrue(iter,cond)) {
		var=true;
		return true;
		}
	return false;
	}
extern	std::span<char> gethistory(int startpos, int len, uint32_t starttime, uint32_t endtime,bool,int);
extern	std::span<char> getstream(int startpos, int len, uint32_t starttime, uint32_t endtime,bool,int);
extern	std::span<char> getscans(int startpos, int len, uint32_t starttime, uint32_t endtime,bool,int);
extern	std::span<char> getamounts(int startpos, int len, uint32_t starttime, uint32_t endtime,bool,int);
extern std::span<char> getmeals(int startpos, int len, uint32_t starttime, uint32_t endtime,bool header,int);

static time_t readtime(const char *&input) {
	struct tm tmbuf {.tm_isdst=0,.tm_gmtoff=0};
	if(const char *ptr= (input[10]=='T')?strptime(input, "%Y-%m-%dT%H:%M:%S", &tmbuf):strptime(input, "%Y-%m-%d", &tmbuf))
		input=ptr;
	time_t	 tim=timegm(&tmbuf);
	return tim;
	}
static time_t readlocaltime(const char *&input) {
	struct tm tmbuf {.tm_isdst=-1,.tm_gmtoff=0};
	if(const char *ptr= (input[10]=='T')?strptime(input, "%Y-%m-%dT%H:%M:%S", &tmbuf):strptime(input, "%Y-%m-%d", &tmbuf))
		input=ptr;
	time_t	 tim=mktime(&tmbuf);
	return tim;
	}

template <typename Num> static const char *readnum(const char *start,const char *ends,Num &num) {
	auto [ptr, ec]=std::from_chars(start, ends, num);
	if(ec==std::errc())
		return ptr;
	switch(ec)  {
		case std::errc::invalid_argument:
			LOGARWEB("That isn't a number. ");
		case  std::errc::result_out_of_range:
			LOGARWEB("This number is larger than an int. ");
		default:
			LOGGERWEB("Error %d\n",ec);
			return nullptr;
		}
	}
struct Getopts {
uint32_t start=0,end=0;
bool headermode=false;
int unit=settings->data()->unit;
Getopts(const char *posptr,int size) {
	int duration=0;
	LOGGER("getopts(%s#%d)\n", posptr, size);
	if(*posptr == '?') {
		const char *ends = posptr + size;
		for (const char *iter = posptr + 1; iter < ends; iter = std::find(iter, ends, '&') + 1) {
			std::string_view header = "header";
			if (setitervar(iter, header, headermode))
				continue;
			std::string_view mmol = "mmol/L";
			if (setitertrue(iter, mmol)) {
				unit=1;
				continue;
				}
			std::string_view mgdl = "mg/dL";
			if(setitertrue(iter, mgdl))  {
				unit=2;
				continue;
				}
			std::string_view startsecstr = "starttime=";
			if (!memcmp(iter, startsecstr.data(), startsecstr.size())) {
				iter += startsecstr.length();
				uint32_t tmpstart;
				if(auto tmp=readnum<uint32_t>(iter, ends, tmpstart)) {
					iter = tmp;
					start=tmpstart;
					}
				continue;
			    }
			std::string_view startsstr = "start=";
			if (!memcmp(iter, startsstr.data(), startsstr.size())) {
				iter += startsstr.length();
				if((iter+10)>ends)
					continue;
				start=readlocaltime(iter);
				if(!iter)
					break;
				continue;
			}
			std::string_view endsecstr = "endtime=";
			if (!memcmp(iter, endsecstr.data(), endsecstr.size())) {
				iter += endsecstr.length();
//				iter = readnum<uint32_t>(iter, ends, end);

				uint32_t tmpend;
				if(auto tmp=readnum<uint32_t>(iter, ends, tmpend)) {
					iter = tmp;
					end=tmpend;
					}
				continue;
				}
			std::string_view endsstr = "end=";
			if (!memcmp(iter, endsstr.data(), endsstr.size())) {
				iter += endsstr.length();
				if((iter+10)>ends)
					continue;
				end=readlocaltime(iter);
				if(!iter)
					break;
				continue;
				}
			std::string_view durationsecstr = "duration=";
			if (!memcmp(iter, durationsecstr.data(), durationsecstr.size())) {
				iter += durationsecstr.length();
				int secs;
				if(auto tmpiter = readnum<int>(iter, ends, secs)) {
					iter=tmpiter;
					duration+=secs;
					}
				
				continue;
				}
			std::string_view durationdaystr = "days=";
			if (!memcmp(iter, durationdaystr.data(), durationdaystr.size())) {
				iter += durationdaystr.length();
				double days;
				int res=sscanf(iter,"%lf",&days);
				if(res<=0) {
					flerror("sscan %s failed",iter);
					continue;
					}
				duration+=(int)round(days*60*60*24);
				iter+=res;
				continue;
				}
			}
		}
	if(duration<=0)
		duration=5*60*60;
	if (!end) {
		if(start) {
			end=start+ duration;
			return;
			}
		else
			end = time(nullptr);
		}
	if (!start)  {
		start = end -duration; 
		}
	};
	};

typedef		std::span<char> (*getdata_t)(int startpos, int len, uint32_t starttime, uint32_t endtime,bool,int);

std::string_view jugglucocommand="x/";
static bool jugglucos(const char * const input,int size, recdata *outdata) {
	const char *posptr=input;
	constexpr const std::string_view types[]={"history","stream","scans","amounts","meals"};
	 constexpr const getdata_t procs[]={gethistory,getstream,getscans,getamounts,getmeals};
	constexpr const int perhour[]={12*62,60*81,2*81,5*85,200};
	for(int i=0;i<std::size(types);i++) {
		auto type=types[i];
		if(!memcmp(type.data(),posptr,type.size())) {
			posptr+=type.size();
			Getopts opts(posptr,size-type.size());

			int startlen=((opts.end-opts.start)/(60*60))*perhour[i];
			constexpr const int startpos=152;
			
			std::span<char> res=procs[i](startpos,startlen,opts.start,opts.end,opts.headermode,opts.unit);
			if(!res.data()) {
				return outofmemory(outdata);
				}
			if(res.size()!=std::numeric_limits<size_t>::max()) {
				const std::string_view plain="text/plain";
				const std::string_view html="text/html";
				outdata->allbuf=res.data();
				mktypeheader(res.data()+startpos,res.data()+res.size(),false,outdata,i==4?html:plain);
				return true;
				}
			else {
				delete[] res.data();
				return giveservererror(outdata);
				}
			break;
			}
		}

	wrongpath({input-jugglucocommand.size(),size+jugglucocommand.size()},outdata);
	return true;
	}

static bool		 getv3treatments(const char *input,int inputlen,recdata *outdata); 
static bool		 pebbleinterpret(const char *input,int inputlen,recdata *outdata);
bool watchcommands(char *rbuf,int len,recdata *outdata,bool secure) {
	LOGGERWEB("watchcommands len=%d %.*s",len,len,rbuf);
	const char *start=rbuf;
	const char *ends=rbuf+len;
	const char *nl;
	std::string_view foundsecret={nullptr,0};
	bool issha1=false;
	std::string_view toget;
	bool behead=false;
	bool json=false;
//	bool gzip=false;
	const char reget[]= "GET /";
	const int regetlen=sizeof(reget)-1;
	const char rehead[]= "HEAD /";
	const int reheadlen=sizeof(rehead)-1;
	const char api_secret[]= "api_secret: ";
	const int	api_len=sizeof(api_secret)-1;
	std::string_view hostname;
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
					constexpr const char jsonstr[]=R"(Accept: application/json)";
					if(!memcmp(start,jsonstr,sizeof(jsonstr)-1)) {
						json=true;
						LOGAR("Accepts json");
						}
					else {
/*					    constexpr const char encoding[]=R"(Accept-Encoding:)";
					    constexpr const int encodelen= sizeof(encoding)-1;
						if(json&&!memcmp(start,encoding,encodelen)) {
							const char *type=start+encodelen;
							constexpr const std::string_view gzipstr="gzip";
							const char *hit=std::search(type,nl,gzipstr.begin(),gzipstr.end());
							if(hit!=nl) {
								gzip=true;
								LOGAR("Accepts gzip");
								}
							}
						else  */{
							constexpr const char hostnamestr[]="Host: ";
							constexpr const int hostnamelen= sizeof(hostnamestr)-1;
							if(!memcmp(start,hostnamestr,hostnamelen)) {
								const char *name=start+hostnamelen;
								hostname={name,static_cast<size_t>(nl-name)};
								}
							}
						}
					}
				}
			}
		start=nl+1;
		if(*start==0xD||*start=='\n')
			break;
		}
	

	if(!toget.data()) {
		return givenothing(outdata);
		return false;
		}
		
	int seclen=settings->data()->apisecretlength;
	if(seclen) {
		const char *starttoget=toget.data();
		if(memcmp(starttoget,settings->data()->apisecret,seclen)||(starttoget[seclen]!='/'&&starttoget[seclen]!=' ')) {
			if(foundsecret.size()==0) {
				const char *end=starttoget+toget.size();
				static constexpr const char token[]="token=";
				static constexpr const int tokenlen=sizeof(token)-1;
				const char *hit=std::search(starttoget,end,token,token+tokenlen);
				if(hit!=end) {
					hit+=tokenlen;	
					const auto len=strcspn(hit," &");
					foundsecret={hit,len};
					LOGGERWEB("has token %.*s#%zd\n",(int)len,foundsecret.data(),foundsecret.size());
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
				LOGGERWEB("%s#%d!=%.*s#%zd \n", realsecret,seclen,(int)foundsecret.size(), foundsecret.data(),foundsecret.size());
				nosecret(foundsecret, outdata) ;
				return false;
				}
			else {
				LOGARWEB("secret matched");
				}
			}
		else {
			int newstart=seclen+1;
			if(newstart>=toget.size()||toget.data()[seclen]==' ') {
				givesite(outdata,hostname,secure);
				return true;
				}
			toget=toget.substr(newstart);
			}
		}
	if(!toget.size()||*toget.data()==' '||*toget.data()=='?') {
		givesite(outdata,hostname,secure);
		return true;
		}
	LOGGERWEB("toget=%.*s\n",(int)toget.size(),toget.data()); //to set getargs in the beginning and use everywhere
std::string_view sgv="sgv.json";
	if(!memcmp(sgv.data(),toget.data(),sgv.size())) {
		return sgvinterpret(toget.data()+sgv.size(),toget.size()-sgv.size(),behead,false,outdata,false);
		}

///api/v1/entries.json
//https:///api/v1/entries?count=2
///api/v1/entries/sgv.txt?c
//https://dnarnianbg.herokuapp.com/api/v1/entries/sgv?count=4
	std::string_view v1="api/v1/";
	const char *posptr= toget.data();
	if(!memcmp(v1.data(),posptr,v1.size())) {
		return apiv1(posptr+v1.size(),toget.size()-v1.size(),behead,json,outdata);
		}
std::string_view properties="api/v2/properties";
const auto propsize= properties.size();
	if(!memcmp(properties.data(),toget.data(),propsize)) {
		return giveproperties(toget.data()+propsize,toget.size()-propsize,outdata);
		}

std::string_view authv2=R"(api/v2/authorization/request/)";
const auto authv2size= authv2.size();
	if(!memcmp(authv2.data(),toget.data(),authv2size)) {
		return  getv2auth(outdata);
		}

if(!memcmp(jugglucocommand.data(),posptr,jugglucocommand.size())) {
	return jugglucos(posptr+jugglucocommand.size(),toget.size()-jugglucocommand.size(),outdata);
	}

constexpr const std::string_view pebble="pebble";
	if(!memcmp(pebble.data(),toget.data(),pebble.size())) {
		return pebbleinterpret(toget.data()+pebble.size(),toget.size()-pebble.size(),outdata);
		} 
std::string_view statusv3="api/v3/status";
const auto stav3size= statusv3.size();
	if(!memcmp(statusv3.data(),toget.data(),stav3size)) {
		return givestatusv3(outdata);
		}
std::string_view entriesv3="api/v3/entries";
const auto entries3size= entriesv3.size();
	if(!memcmp(entriesv3.data(),toget.data(),entries3size)) {
		return getv3entries(outdata);
		}
std::string_view treatmentsv3="api/v3/treatments";
const auto treatments3size= treatmentsv3.size();

	if(!memcmp(treatmentsv3.data(),toget.data(),treatments3size)) {
		const char *start=toget.data()+treatmentsv3.size();
		std::string_view history="/history";
		if(!memcmp(start,history.data(),history.size()))  {
			return nothingV3(outdata);
			}
		std::string_view jsonstr=".json";
		if(!memcmp(start,jsonstr.data(),jsonstr.size())) 
			start+=jsonstr.size();
		return getv3treatments( start,toget.end()-start, outdata);
		}

std::string_view foodv3=R"(api/v3/food)";
const auto food3size= foodv3.size();
	if(!memcmp(foodv3.data(),toget.data(),food3size)) {
		return getv3food(outdata);
		}

std::string_view devicestatusv3=R"(api/v3/devicestatus/history)";
const auto devicestatus3size= devicestatusv3.size();
	if(!memcmp(devicestatusv3.data(),toget.data(),devicestatus3size)) {
		return nothingV3(outdata);
		}
std::string_view modifiedv3="api/v3/lastModified";
const auto modified3size= modifiedv3.size();
	if(!memcmp(modifiedv3.data(),toget.data(),modified3size)) {
		return getv3modified(outdata);
		}

constexpr const std::string_view status="status.json";
	if(!memcmp(status.data(),toget.data(),status.size())) {
		return givedripstatus(outdata);
		} 
		/*
std::string_view socket="socket.io";
const auto socketsize= socket.size();
	if(!memcmp(socket.data(),toget.data(),socketsize)) {
		return givenothing(outdata);
		}
		*/
std::string_view index="index.html";
const auto indexsize= index.size();
	if(toget.data()[0]==' '||!memcmp(index.data(),toget.data(),indexsize)) {
		return givesite(outdata,hostname,secure);
		}


	wrongpath(toget,outdata);
	return true;
	}



void wrongpath(std::string_view toget, recdata *outdata) {
//	const char notfoundtxt[]="HTTP/1.0 404 Not Found\r\nAccess-Control-Allow-Origin: *\r\nContent-Type: text/plain\r\nContent-Length: ";
//	const char notfoundtxt[]="HTTP/1.0 404 Not Found\r\nContent-Type: text/plain\r\nContent-Length: ";
	const char notfoundtxt[]="HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: ";
	constexpr const int startlen=sizeof(notfoundtxt)-1;
	constexpr int maxant=4096;
	char *notfound=outdata->allbuf=new char[maxant];
	memcpy(notfound,notfoundtxt,startlen);
	const char notpath[]="Bad Request: ";
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




/*
void mkjsonheader(char *outstart,char *outiter,const bool headonly,recdata *outdata)  {
	const char header1[]="HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: " ;
	const int header1len=sizeof(header1)-1;
	int uitlen=outiter-outstart;
	constexpr const int maxlen=20;
	char lenstr[maxlen];
	const int getlen=snprintf(lenstr,maxlen,"%d\r\n\r\n",uitlen);
	const int headerlen=header1len+getlen;
	char * const startheader=outstart-headerlen;
	memcpy(startheader,header1,header1len);
	LOGGERWEB("direct: len=%d\n",header1len);
	logwriter(startheader,80);
	memcpy(startheader+header1len,lenstr,getlen);
	int totlen;
	if(headonly) {
		totlen=headerlen;
		startheader[totlen]='\0';
		}
	else
		 totlen=outiter-startheader;
	LOGARWEB("All:");
	logwriter(startheader,totlen);
	outdata->start=startheader;
	outdata->len=totlen;

	}
 */
void mktypeheader(char *outstart,char *outiter,const bool headonly,recdata *outdata,std::string_view type)  {
//	constexpr const char header1[]="HTTP/1.1 200 OK\r\nVary: Accept, Accept-Encoding\r\nContent-Type: ";
	constexpr const char header1[]="HTTP/1.1 200 OK\r\nContent-Type: ";
	constexpr const int header1len=sizeof(header1)-1;
	constexpr const char content[]="; charset=utf-8\r\nContent-Length: " ;
	constexpr const int contentlen=sizeof(content)-1;
	const int headerstartlen=header1len+contentlen+type.size();
	int uitlen=outiter-outstart;
	constexpr const int maxlen=20;
	char lenstr[maxlen];
	const int getlen=snprintf(lenstr,maxlen,"%d\r\n\r\n",uitlen);
	const int headerlen=headerstartlen+getlen;
	char * const startheader=outstart-headerlen;
	memcpy(startheader,header1,header1len);
	char *ptr=startheader+header1len;
	memcpy(ptr,type.data(),type.size());
	ptr+=type.size();
	memcpy(ptr,content,contentlen);
	logwriter(startheader,80);
	memcpy(startheader+headerstartlen,lenstr,getlen);
	int totlen;
	if(headonly) {
		totlen=headerlen;
		startheader[totlen]='\0';
		}
	else
		 totlen=outiter-startheader;
	LOGARWEB("START:");
	logwriter(startheader,400);
	outdata->start=startheader;
	outdata->len=totlen;

	}

void mkjsonheader(char *outstart,char *outiter,const bool headonly,recdata *outdata)  {
	mktypeheader(outstart,outiter,headonly,outdata, "application/json");
	}
class Sgvinterpret {
	bool briefmode=false,sensorinfo=false,alldata=false,noempty=false;
  public:
	Sgvinterpret(bool all=true,bool gmt=true,int datnr=24): alldata(all),gmt(gmt),datnr(datnr) {}

	int interval=settings->data()->nightinterval;
	bool gmt;
	int datnr;
	uint32_t lowerend=0,higherend=INT32_MAX;
	const char *event=nullptr;
	bool carb=false;
	bool insulin=false;
	bool mmol=settings->usemmolL();
	bool getargs(const char *start,int len) ;
	bool getdata(bool headonly,recdata *outdata) const;

	char *makedata(recdata *outdata ) const;
private:
	char *dontbrief(char *outiter,const char *name,const ScanData *iter) const ;
	char *firstdata(char *outiter,time_t starttime,uint32_t dattime) const ;
	char *writeitem(char *outiter,int datit, const ScanData *iter,const char *sensorname,const time_t starttime) const;
	//void mkjsonheader(char *outstart,char *outiter,const bool headonly,recdata *outdata) const;
	bool getv1entries(char *&outiter,const int  datnr) const;
}; 

static bool		 pebbleinterpret(const char *input,int inputlen,recdata *outdata) {
	Sgvinterpret pret;
	pret.datnr=1;
	if(!pret.getargs(input,inputlen))  {
		wrongpath({input,(size_t)inputlen},outdata);
		return false;
		}
	int count=pret.datnr>0?pret.datnr:1;
	bool mmol=pret.mmol;
   	outdata->allbuf=new(std::nothrow) char[512+80*+count+200];
	if(!outdata->allbuf) {
		return outofmemory(outdata);
		}
    	char *start=outdata->allbuf+150,*outiter=start;
	auto nu=time(nullptr);
	constexpr const char startpebble[]=R"({"status":[{"now":%lld}],"bgs":[)";
	constexpr const char endpebble[]=R"(],"cals":[]})";
	outiter+=sprintf(outiter,startpebble,nu*1000LL);

	if(!getitems(outiter,count,pret.lowerend,pret.higherend,true,pret.interval,[mmol,nu](char *outiter,int datit, const ScanData *iter,const char *sensorname,const time_t starttime) {
			
		char *ptr=pebbleitem(mmol,outiter,iter);
		if(!datit) {
	 		//double delta= isnan(iter->ch)?0:iter->ch*deltatimes;
	 		double delta= getdelta(iter->ch);

			extern double getiob(uint32_t now);
			double iob=getiob(nu);
			ptr-=2;
			ptr+=sprintf(ptr,R"(,"bgdelta":"%.1f","iob":"%.2f"},)",delta,iob); //TODO remove ""? xDrip has "", Nightscout hasn't, who is right?
			}
		return ptr;
		})) {
		return givenothing(outdata);
	};
	addar(--outiter,endpebble);
	mkjsonheader(start, outiter, false, outdata);
	return true;
	}
bool givesgvtxt(const char *input,int inlen,recdata *outdata,char sep) {
	Sgvinterpret pret;
	pret.datnr=10;
	if(!pret.getargs(input,inlen))  {
		wrongpath({input,(size_t)inlen},outdata);
		return false;
		}
	LOGGERWEB("givesgvtxt datnr=%d\n",pret.datnr);
	return givesgvtxt(pret.datnr,pret.interval,pret.lowerend,pret.higherend,outdata,sep);
	}
char *Sgvinterpret::makedata(recdata *outdata ) const {
	char *output=outdata->allbuf= new(std::nothrow) char[512+datnr*360];
	if(output==nullptr)
		return nullptr;
	return output+152;
	}
char *Sgvinterpret::dontbrief(char *outiter,const char *name,const ScanData *iter) const {
	outiter+=sprintf(outiter,R"("_id":"%s#%d","device":"Juggluco","dateString":")", name,iter->id);
	const char *startdate=outiter;
	int len=(gmt?oneTdatestringGMT:Tdatestring)(iter->t,outiter);
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
	LOGGERWEB("mmol=%d %s\n",mmol,hint.data());
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
	LOGGERWEB("writeitem %d\n",datit);
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





bool Sgvinterpret::getv1entries(char *&outiter,const int  datnr) const {

	return  ::getitems(outiter,datnr, lowerend,higherend,alldata, interval,[this](char *outiter,int datit, const ScanData *iter,const char *sensorname,const time_t starttime)
				{return writeitem(outiter, datit, iter,sensorname, starttime);});

	}
//{"_id":"6401c40addf76d1473eb7d02","timestamp":1677837282000,"eventType":"<none>","enteredBy":"xdrip pos:6.52","notes":"Swim → Aaps → nog meer","uuid":"6401c40addf76d1473eb7d02","created_at":"2023-03-03T09:54:42.000Z","sysTime":"2023-03-03T10:54:42.000+0100","utcOffset":0,"carbs":null,"insulin":null},
//notes	"AndroidAPS started - realme RMX2202"
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
extern int mkid(char *outiter,int base,int pos) ;
/*
template <class T>
static void toend(NumIter<T> *numiters,int maxnum) {
	for(int i=0;i<maxnum;i++) {
		numiters[i].iter=numiters[i].end;
		}
	} */

extern char *writetreatment(char *outiter,const int numbase,const int pos,const Num*num,int border);

int mkid(char *outiter,int base,int pos) {
	int len=sprintf(outiter,"ba%de%d",base,pos);
	int over=24-len;
	memset(outiter+len,'b',over);
	return 24;
	}
char *writetreatment(char *outiter,const int numbase,const int pos,const Num*num,int border) {
	const int type=num->type;
	if(type>=settings->varcount()||!settings->data()->Nightnums[type].kind) {
		return outiter;
		}
	const time_t tim=num->gettime();
        struct tm tmbuf;
        gmtime_r(&tim, &tmbuf);
		addar(outiter,R"({"_id":")");

	if(pos>=border) 
		outiter+=mkid(outiter,numbase,pos);
	else
		outiter+=sprintf(outiter,"num%d#%d",numbase,pos);


	addar(outiter,R"(","eventType":"<none>","enteredBy":"Juggluco","created_at":")");
	outiter+=sprintf(outiter,R"(%04d-%02d-%02dT%02d:%02d:%02d.000Z",)",tmbuf.tm_year+1900,tmbuf.tm_mon+1,tmbuf.tm_mday, tmbuf.tm_hour, tmbuf.tm_min,tmbuf.tm_sec);

	float w=0.0f;
	 if((w=longNightWeight(type))!=0.0f) {
	 	
	 	addar(outiter,R"("notes":"Long-Acting",)");
	 	}
	else { if((w=rapidNightWeight(type))!=0.0f) {
	 	addar(outiter,R"("notes":"Rapid-Acting",)");
	 	}
		}
	if(w!=0.0f) {
		const char * typestr=settings->getlabel(type).data();;
		auto units=w*num->value;
		//outiter+=sprintf(outiter,R"("carbs":null,"insulin":%g,"insulinType":"%s","insulinInjections":"[{\"insulin\":\"%s\",\"units\":%.1f}]"},)",units,typestr,typestr,units); //WARNING: treatments are not displayed in xDrip when insulinInjections is included. Problem?
		outiter+=sprintf(outiter,R"("carbs":null,"insulin":%g,"insulinType":"%s"},)",units,typestr);

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

		wrongpath({args,(size_t)argslen},outdata);
		return false;
		}
	
	const int basecount=numdatas.size();
	NumIter<Num> numiters[basecount];
	for(int i=0;i<basecount;i++) {
		auto [low,high]= numdatas[i]->getInRange(pret.lowerend,pret.higherend); 
		numiters[i].begin=low;
		numiters[i].iter=numiters[i].end=high-1;
		numiters[i].bytes=sizeof(Num);
		}

	int count= pret.datnr;
	char *outstart=pret.makedata(outdata ); 
	if(!outstart) {
		LOGARWEB("givetreatments:");
		return outofmemory(outdata);
		}

	char *outiter=outstart;
	*outiter++='[';
	bool carb=pret.carb;
	bool insulin=pret.insulin;
	int i=0;
	if(settings->data()->saytreatments&&!pret.event) {
		for(;i<count;) {
			auto [ind,num]=findnewestwith(numiters,basecount);
			if(!num) {
				LOGGERWEB("no values %d %p\n",ind,num);
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
			const int pos=num-numdatas[ind]->begin();
			const int border=numdatas[ind]->getnightSwitch();
			char *out=writetreatment(outiter,ind,pos,num,border);
			if(out!=outiter) {
				outiter=out;
				i++;
				}
			}
		if(outiter[-1]==',') --outiter;
		}
	 *outiter++=']';
	 *outiter++='\n';
       mkjsonheader(outstart,outiter, false,outdata);
       LOGGER("givetreatments nr=%d len=%d %.50s\n",i,outiter-outstart,outstart);
       return true;
	}

bool Sgvinterpret::getdata(bool headonly,recdata *outdata) const {
	if(!sensors)	
		return false;
	LOGGERWEB("count=%d\n",datnr);

	char *outstart=makedata(outdata);
	if(!outstart) {
		return outofmemory(outdata);
		}
	char *outiter=outstart;
	*outiter++='[';
	if(!getv1entries(outiter,datnr) )
		return false;
	if(outiter==(outstart+1)&&noempty) {
		*outstart='\0';
		outiter=outstart;
		}
	else  {
	  *outiter++=']';
	  *outiter++='\n';
	  }
       mkjsonheader(outstart,outiter, headonly,outdata);
	return true;
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
			case toshort("20"): *uititer++=' ';break;
			default: LOGGERWEB(LOGID "strange char %.3s\n",next-1); memmove(uititer,next-1,3);uititer+=3;
			}
		iter=next+2;
		}
	}
  bool Sgvinterpret::getargs(const char *start,int lenin) {
	 int 	len=rewriteperc(const_cast<char *>(start),lenin);
	LOGGERWEB("after Sgvinterpret::getargs(%.*s#%d)\n",len,start,len);

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
							LOGGERWEB("%s readnum failed '%s'\n",greater.data(),iter);
							return false;
							}
						lowerend=tmp/1000;
						if(greater==greater3) {
							++lowerend;
							}
						iter=ptr;
						LOGGERWEB("greater than %d\n",lowerend);
						}
					else {
						std::string_view smaller="lte]=";
						std::string_view smaller2="lt]=";
						if((!memcmp(iter,smaller.data(),smaller.size()))||(smaller=smaller2,!memcmp(iter,smaller.data(),smaller.size()))) {
							iter+=smaller.length();
							longlongtype tmp;	
							if(!(iter=readnum<longlongtype>(iter,ends,tmp))) {
								LOGGERWEB("%s= readnum failed\n",smaller.data());
								return false;
								}
							higherend=tmp/1000LL;
							if(smaller!=smaller2) {
								++higherend;
								}
							LOGGERWEB("smaller than %d\n",higherend);
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
						LOGGERWEB("greater than %d\n",lowerend);
						}
					else {
						std::string_view smaller="lte]="; //TODO smaller or equal
						std::string_view smaller2="lt]=";
					if((!memcmp(iter,smaller.data(),smaller.size()))||(smaller=smaller2,!memcmp(iter,smaller.data(),smaller.size()))) {
							iter+=smaller.length();
							higherend=readtime(iter);
							if(smaller!=smaller2)
								higherend++;
							LOGGERWEB("smaller than %d\n",higherend);
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
						LOGGERWEB("greater than %d\n",lowerend);
						continue;
						}
				std::string_view smaller="lte]=";
				std::string_view smaller2="lt]=";
				if((!memcmp(iter,smaller.data(),smaller.size()))||(smaller=smaller2,!memcmp(iter,smaller.data(),smaller.size()))) {
						iter+=smaller.length();
						higherend=readtime(iter);
						if(smaller!=smaller2)
							higherend++;
						LOGGERWEB("smaller than %d\n",higherend);
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
			else {
				std::string_view unitstr="units=";
				if(!memcmp(iter,unitstr.data(),unitstr.size())) {
					iter+=unitstr.length();
					std::string_view mmolstr="mmol";
					if(!memcmp(iter,mmolstr.data(),mmolstr.size())) {
						mmol=true;
						iter+=mmolstr.size();
						}
					else {
						mmol=false;
						}
					continue;
					}
				}
			}
		
	}
	if(!datnr) {
		if(lowerend|| higherend!=INT32_MAX) //needed for Cockpit
			datnr=100000;

		}
	if(higherend<=lowerend)
		return false;
	return true;
	};

static bool	 currentjson(recdata *outdata) {
	Sgvinterpret pret(true,true,1);
	return pret.getdata(false,outdata);
	}
static bool	 sgvinterpret(const char *start,int len,bool headonly,bool gmt,recdata *outdata,bool all) {
	Sgvinterpret pret(all,gmt);
	if(!pret.getargs(start,len)) {

		wrongpath({start,(size_t)len},outdata);

		return false;
		}
	LOGGERWEB("lowerend=%d higherend=%d\n",pret.lowerend,pret.higherend);
	return pret.getdata(headonly,outdata);
	}


/*
HTTP/1.0 200 OK
Date: Sat, 26 Feb 2022 19:08:27 GMT
Access-Control-Allow-Origin: *
Content-Type: application/json
Content-Length: 6890

*/

class V3Args {
public:
	int datnr=0;
	uint32_t lowerend=0,higherend=INT32_MAX;
	bool getargs(const char *start,int len) ;
	}; 
  bool V3Args::getargs(const char *start,int lenin) {
	 int 	len=rewriteperc(const_cast<char *>(start),lenin);
	LOGGERWEB("after V3Args::getargs(%.*s#%d)\n",len,start,len);

	const char *ends=start+len;
	start++;
	for(const char *iter=start;iter<ends;iter=std::find(iter,ends,'&')+1) {
		std::string_view count="limit=";
		if(!memcmp(iter,count.data(),count.size())) {
			iter+=count.length();
			if(!(iter=readnum<int>(iter,ends,datnr))||datnr<0) {

				return false;
				}
			}
		else {
//		created_at$gt=2023-10-02T13:45:26.653Z
		 std::string_view findstr="created_at$";
		 if(!memcmp(iter,findstr.data(),findstr.size())) {
			iter+=findstr.size();
			std::string_view gtstr="gt";
			if(!memcmp(iter,gtstr.data(),gtstr.size())) {
				iter+=gtstr.size();
				bool equal;
				if(*iter=='=') {
					equal=false;
					++iter;	
					}
				else {
					std::string_view eisstr="e=";
					if(!memcmp(iter,eisstr.data(),eisstr.size())) {
						equal=true;
						iter+=eisstr.size();
						}
					else {
						continue;
						}
					}
				   if(iter[4]=='-') {
					lowerend=readtime(iter);
					}
				else  {
					const char *ptr;
					longlongtype tmp;
					if(!(ptr=readnum<longlongtype>(iter,ends,tmp))) {
						LOGGERWEB("gt%s readnum failed '%s'\n",equal?"e":"",iter);
						return false;
						}
					if(lowerend>1602042233000LL)
						lowerend=tmp/1000LL;
					iter=ptr;
					LOGGERWEB("greater than %d\n",lowerend);
					}
				if(!equal) {
					++lowerend;
					}
				}
			else {
			std::string_view ltstr="lt";
			if(!memcmp(iter,ltstr.data(),ltstr.size())) {
				iter+=ltstr.size();
				bool equal;
				if(*iter=='=') {
					equal=false;
					++iter;	
					}
				else {
					std::string_view eisstr="e=";
					if(!memcmp(iter,eisstr.data(),eisstr.size())) {
						equal=true;
						iter+=eisstr.size();
						}
					else {
						continue;
						}
					}
				  if(iter[4]=='-') {
					higherend=readtime(iter);
					}
				else  {
					const char *ptr;
					longlongtype tmp;
					if(!(ptr=readnum<longlongtype>(iter,ends,tmp))) {
						LOGGERWEB("lt%s readnum failed '%s'\n",equal?"e":"",iter);
						return false;
						}
					if(higherend>1602042233000LL)
						higherend=tmp/1000LL;
					iter=ptr;
					LOGGERWEB("smaller than %d\n",higherend);
					}
				if(equal) {
						++higherend;
						}
				  }
				}
			}
			}	
		}
	if(!datnr) {
		return false;
		}
	if(higherend<=lowerend)
		return false;
	return true;
	};

char *writetreatmentv3(char *outiter,const int numbase,const int pos,const Num*num) {
	const int type=num->type;
	if(type>=settings->varcount()||!settings->data()->Nightnums[type].kind) {
		return outiter;
		}
	const time_t tim=num->gettime();
        struct tm tmbuf;
        gmtime_r(&tim, &tmbuf);

	addar(outiter,R"({"eventType":"<none>","enteredBy":"Juggluco","created_at":")");
	outiter+=sprintf(outiter,R"(%04d-%02d-%02dT%02d:%02d:%02d.000Z",)",tmbuf.tm_year+1900,tmbuf.tm_mon+1,tmbuf.tm_mday, tmbuf.tm_hour, tmbuf.tm_min,tmbuf.tm_sec);

	float w=0.0f;
	 if((w=longNightWeight(type))!=0.0f) {
	 	
	 	addar(outiter,R"("notes":"Long-Acting",)");
	 	}
	else { if((w=rapidNightWeight(type))!=0.0f) {
	 	addar(outiter,R"("notes":"Rapid-Acting",)");
	 	}
		}
	if(w!=0.0f) {
		const char * typestr=settings->getlabel(type).data();;
		auto units=w*num->value;
		outiter+=sprintf(outiter,R"("insulin":%g,"insulinType":"%s")",units,typestr);

		}
	else {
		if((w=carboNightWeight(type) )!=0.0f) {
			outiter+=sprintf(outiter,R"("carbs":%g)",w*num->value);
			}
		else {
			std::string_view typestr=settings->getlabel(type);
			outiter+=sprintf(outiter,R"("notes":"%s %g")",typestr.data(),num->value);
			}
		}
	addar(outiter,R"(,"utcOffset":0,"identifier":")");
	outiter+=mkid(outiter,numbase,pos);
	outiter+=sprintf(outiter,R"(","srvModified":%lu000,"srvCreated":%lu000},)",tim,tim);
	return outiter;
	}

void wrongpath(std::string_view toget, recdata *outdata);


 void mkjsonheader(char *outstart,char *outiter,const bool headonly,recdata *outdata) ;
bool		 getv3treatments(const char *input,int inputlen,recdata *outdata) {
	V3Args args; 
	if(!args.getargs(input,inputlen)) {
		wrongpath({input,(size_t)inputlen},outdata);
		return false;
		}
	
	const int basecount=numdatas.size();
	NumIter<Num> numiters[basecount];
	for(int i=0;i<basecount;i++) {
		auto [low,high]= numdatas[i]->getInRange(args.lowerend,args.higherend); 
		numiters[i].iter=numiters[i].begin=low;
		numiters[i].end=high-1;
		numiters[i].bytes=sizeof(Num);
		}
	auto count=args.datnr;

	char *buffer=outdata->allbuf= new(std::nothrow) char[512+count*270];
	if(!buffer) {
		LOGARWEB("getv3treatments:");
		return outofmemory(outdata);
		}
	 char *outiter=buffer+152;
	 char *outstart=outiter;
	addar(outiter,R"({"status":200,"result":[)");
	int i=0;
	
	for(;i<count;) {
		auto [ind,num]=findoldestwith(numiters,basecount);
			if(!num) {
				LOGGERWEB("no values %d %p\n",ind,num);
				break;
				}
			const int pos=num-numdatas[ind]->begin();
			char *out=writetreatmentv3(outiter,ind,pos,num);
			if(out!=outiter) {
				outiter=out;
				i++;
				}
			}
	if(outiter[-1]==',') --outiter;
	 addar(outiter,"]}");
       mkjsonheader(outstart,outiter, false,outdata);
       LOGGERWEB("givetreatments nr=%d len=%d %.50s\n",i,outiter-outstart,outstart);
       return true;
	}

#endif

