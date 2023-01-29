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
#ifndef LOGGER
#define LOGGER(...) fprintf(stderr,__VA_ARGS__)
#endif



static void watchserverloop(int *sockptr,bool secure) ;
static bool startwatchserver(bool secure,int port,int *sockptr) {
	const char *servername=secure?"SECUREWATCHSERVER":"WATCHSERVER";
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
#define _GNU_SOURCE
#include <sched.h>
int xdripserversock=-1;
int xdripserversslsock=-1;

bool stopconnection=false;
void startwatchthread() {
	if(xdripserversock==-1)  {
		std::thread watch(startwatchserver,false,17580,&xdripserversock);
		watch.detach();
	#ifdef USE_SSL
	if(xdripserversslsock==-1)  {
extern	bool  loadsslfunctions() ;
		static bool working= loadsslfunctions() ;
		if(working) {
			std::thread watchsec(startwatchserver,true,17581,&xdripserversslsock);
			watchsec.detach();
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
	shutdown(xdripserversslsock,SHUT_RDWR);
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
				LOGGER("exit\n"); return;
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
		stopconnection=false;
#ifdef USE_SSL
		if(secure) {
void handlewatchsecure(int sock) ;
			std::thread  handlecon(handlewatchsecure,new_fd);
			handlecon.detach();
			}
		else 
#endif
		{
			std::thread  handlecon(handlewatch,new_fd);
			handlecon.detach();
			}
		}
	}
void handlewatch(int sock) {
      const char threadname[]="watchconnect";
      prctl(PR_SET_NAME, threadname, 0, 0, 0);
      LOGGER("handlewatch %d\n",sock);
bool	watchcommands(int sock);
	
	while(watchcommands(sock))
		;
	close(sock);
	}




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



static bool	 sgvinterpret(const char *start,int len,bool headonly, recdata *data) ;

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
        LOGGER("success sendall\n");
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
		LOGGER("shutdown\n");
		return false;
		}
	struct recdata outdata;

	if(stopconnection)
		return false;
	bool res=watchcommands(rbuf, len,&outdata); 
	bool res2=sendall( sock ,outdata.data(),outdata.size()) ;
	return res&&res2&&!stopconnection;
	}
bool watchcommands(char *rbuf,int len,recdata *outdata) {
	LOGGER("watchcommands len=%d %s",len,rbuf);
	const char *start=rbuf;
	const char *ends=rbuf+len;
	const char *nl;
	const char *keyword=nullptr;
	std::string_view toget;
	bool behead=false;
	const char reget[]= "GET /";
	const int regetlen=sizeof(reget)-1;
	const char rehead[]= "HEAD /";
	const int reheadlen=sizeof(rehead)-1;
	const char apisecret[]= "api-secret: ";
	const int	apilen=sizeof(apisecret)-1;
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
				if(!memcmp(start,apisecret,apilen)) {
					keyword=start+apilen;
					}
				}
			}
		start=nl+1;
		if(*start==0xD||*start=='\n')
			break;
		}
	if(!toget.size()) {
		outdata->start= servererrorstr.data();
		outdata->len=servererrorstr.size();
		return false;
		}
	LOGGER("toget=%s\n",toget.data());
std::string_view sgv="sgv.json";
	if(!memcmp(sgv.data(),toget.data(),sgv.size())) {
		return sgvinterpret(toget.data()+sgv.size(),toget.size()-sgv.size(),behead,outdata);
		}
std::string_view api="api/v1/entries/sgv.json";
	if(!memcmp(api.data(),toget.data(),api.size())) {
		return sgvinterpret(toget.data()+api.size(),toget.size()-api.size(),behead,outdata);
		}
		/*
std::string_view pebble="pebble";
	if(!strcmp(pebble.data(),toget.data())) {
		return pebbleinterpret(toget.data()+pebble.size(),toget.size-pebble.size());
		} */
void wrongpath(std::string_view toget, recdata *outdata);
	wrongpath(toget,outdata);
	return false;
	}


void wrongpath(std::string_view toget, recdata *outdata) {
	const char notfoundtxt[]="HTTP/1.0 404 Not Found\r\nAccess-Control-Allow-Origin: *\r\nContent-Type: text/plain\r\nContent-Length: ";
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




extern int Tdatestring(time_t tim,char *buf) ;
int Tdatestring(time_t tim,char *buf) {
         struct tm tmbuf;
        int seczone=timegm(localtime_r(&tim,&tmbuf)) - tim;
	int m=seczone/60;
	int h=m/60;	
	int minleft=m%60;
	return sprintf(buf,R"(%d-%02d-%02dT%02d:%02d:%02d.000%+03d:%02d)",tmbuf.tm_year+1900,tmbuf.tm_mon+1,tmbuf.tm_mday,tmbuf.tm_hour,tmbuf.tm_min,tmbuf.tm_sec,h,minleft);
	}

extern std::string_view getdeltaname(float rate);
double deltatimes=5.0;
static const char *readnum(const char *start,const char *ends,int &num) {
	auto [ptr, ec]=std::from_chars(start, ends, num);
	switch(ec)  {
		 case std::errc():break;
		 case std::errc::invalid_argument:
			LOGGER("That isn't a number. ");
		case  std::errc::result_out_of_range:
			LOGGER( "This number is larger than an int. ");
		default: 
			LOGGER("Error %d\n",ec);
			return nullptr;
		}
	if(num<1)
		return nullptr;
	return ptr;
	}
class Sgvinterpret {
   private:
	bool briefmode=false,sensorinfo=false,alldata=false,noempty=false;
	int datnr=24;
	int interval=270;
  public:
	bool getargs(const char *start,int len) ;
	bool getdata(bool headonly,recdata *outdata) const;

private:
	char *makedata(recdata *outdata ) const;
	static char *dontbrief(char *outiter,const char *name,const ScanData *iter) ;
	char *firstdata(char *outiter,time_t starttime,uint32_t dattime) const ;
	char *writeitem(char *outiter,int datit, const ScanData *iter,const char *sensorname,const time_t starttime) const;
	void mkheader(char *outstart,char *outiter,const bool headonly,recdata *outdata) const;
	bool getitems(char *&outiter,const int  datnr) const;
}; 

char *Sgvinterpret::makedata(recdata *outdata ) const {
	char *output=outdata->allbuf= new(std::nothrow) char[512+datnr*360];
	if(output==nullptr)
		return nullptr;
	return output+152;
	}
char *Sgvinterpret::dontbrief(char *outiter,const char *name,const ScanData *iter) {
	outiter+=sprintf(outiter,R"("_id":"%s#%d","device":"Other App","dateString":")", name,iter->id);
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
	 double delta= isnan(iter->ch)?0:iter->ch*deltatimes;
	 std::string_view name=getdeltaname(iter->ch);
	 outiter+=sprintf(outiter,R"("date":%d000,"sgv":%d,"delta":%.3f,"direction":"%s","noise":1)",iter->t,iter->getmgdL(),delta,name.data());
	    if (!briefmode) {
		long mgdL1000=iter->getmgdL()*1000;
		outiter+=sprintf(outiter,R"(,"filtered":%ld,"unfiltered":%ld,"rssi":100,"type":"sgv")",mgdL1000,mgdL1000);
		}
	   if(datit==0) {
		outiter=firstdata(outiter,starttime,iter->t);
		}
	  *outiter++='}';
	return outiter;
	}
void Sgvinterpret::mkheader(char *outstart,char *outiter,const bool headonly,recdata *outdata) const {
	const char header1[]="HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\nContent-Type: application/json\r\nContent-Length: " ;
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
	LOGGER("All:\n");
	logwriter(startheader,totlen);
	outdata->start=startheader;
	outdata->len=totlen;

	}


extern Sensoren *sensors;


const SensorGlucoseData *getPollsensor(int &sensorid) {
	for(;;sensorid--) {
		if(sensorid<0)  {
			return nullptr;
			}
		if(const SensorGlucoseData *sens=sensors->gethist(sensorid)) {
			if(sens->pollcount()>0)
				return sens;
			}
		}
	}
bool Sgvinterpret::getitems(char *&outiter,const int  datnr) const {
	LOGGER("getitems %d\n",datnr);
	int sensorid=sensors->last();
	uint32_t timenext=UINT32_MAX;
	int datit=0; 
	{
		STARTDATA:
		const SensorGlucoseData *sens=getPollsensor(sensorid);;
		if(!sens) {
			if(datit)
				return true;
			return false;
			}

		LOGGER("getPollsensor(%d)\n",sensorid);
		const std::span<const ScanData> gdata=sens->getPolldata();
		const ScanData *first=&gdata.begin()[0];
		const ScanData *iter=&gdata.end()[-1];
		const char *sensorname= sens->shortsensorname()->data();
		const time_t starttime= sens->getstarttime();
		for(;datit<datnr;datit++,iter--) {
			 while(true) {
				if(iter<first) {
					if(alldata) {
						--sensorid;
						goto STARTDATA;
						}
					if(datit)
						return true;
					return false;
					}
					
			 	if(iter->valid()) {
					if(iter->t<timenext)
						break;
					}
				--iter;
					
				}
			timenext=iter->t-interval;
			outiter=writeitem(outiter,datit,iter,sensorname,starttime);
			}

		}
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



/*
	int sensorid=sensors->last();



	uint32_t timenext=UINT32_MAX;

	int datit=0; 
	{
		STARTDATA:
		const SensorGlucoseData *sens;
		for(;;sensorid--) {
			if(sensorid<0)  {
				if(datit)
					goto ENDDATA;
				return false;
				}
			if((sens=sensors->gethist(sensorid))) {
				if(sens->pollcount()>0)
					break;
				}
			}
		const std::span<const ScanData> gdata=sens->getPolldata();
		const ScanData *first=&gdata.begin()[0];
		const ScanData *iter=&gdata.end()[-1];
		const char *sensorname= sens->shortsensorname()->data();
		const time_t starttime= sens->getstarttime();
		for(;datit<datnr;datit++,iter--) {
			 while(true) {
			 	if(iter->valid()) {
					if(iter->t<timenext)
						break;
					}

				if(iter--<=first) {
					if(alldata) {
						--sensorid;
						goto STARTDATA;
						}
					goto ENDDATA;
					}
					
				}
			timenext=iter->t-interval;
			outiter=writeitem(outiter,datit,iter,sensorname,starttime);
			}
		}
	ENDDATA:
*/
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
  bool Sgvinterpret::getargs(const char *start,int len) {
	LOGGER("sgvinterpret(%s#%d)\n",start,len);
	const char *ends=start+len;
	start++;
	for(const char *iter=start;iter<ends;iter=std::find(iter,ends,'&')+1) {
		std::string_view count="count=";
		if(!memcmp(iter,count.data(),count.size())) {
			iter+=count.length();
			if(!(iter=readnum(iter,ends,datnr))) {
				return false;
				}
			}
		else {
			std::string_view brief="brief_mode=";
			if(!memcmp(iter,brief.data(),brief.size())) {
				briefmode=true;
				iter+=brief.length();
				}
			else {
				std::string_view sensor="sensor=";
				if(!memcmp(iter,sensor.data(),sensor.size())) {
					sensorinfo=true;
					iter+=sensor.length();
					}
				else {
					std::string_view all_data="all_data=";
					if(!memcmp(iter,all_data.data(),all_data.size())) {
						alldata=true;
						iter+=all_data.length();
						}
					else {
						std::string_view no_empty="no_empty=";
						if(!memcmp(iter,no_empty.data(),no_empty.size())) {
							noempty=true;
							iter+=no_empty.length();
							}

						else {
							std::string_view intervalstr="interval=";
							if(!memcmp(iter,intervalstr.data(),intervalstr.size())) {
								iter+=intervalstr.length();
								if(!(iter=readnum(iter,ends,interval))) {
									return false;
									}
								}
							}
						}
					}
				}
			}
		
	}
	return true;
	};
static bool	 sgvinterpret(const char *start,int len,bool headonly,recdata *outdata) {
	Sgvinterpret pret;
	if(!pret.getargs(start,len))
		return false;
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
