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
/*      Fri Jan 27 12:35:09 CET 2023                                                 */


#pragma once 
#include <string_view>
#include <condition_variable>
#include <thread>
//#include <pthread.h>
#include <vector>
#include <sys/prctl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <setjmp.h>
#include <sys/socket.h>
#include <stdlib.h>

#include <jni.h>
constexpr const char defaultport[]{
#ifdef RELEASEID
"8795"
#else
"9113"
#endif

};


#include "inout.h"
#include "net/backup.h"
#include "destruct.h"
#include "net/passhost.h"
#include "net/netstuff.h"
#include "sensoren.h"

#include "settings/settings.h"

#include "mirrorstatus.hpp"
#include "mirrorerror.h"
#ifdef WEAROS_MESSAGES
extern bool wearmessages[];
#endif
/*
inline void closesock(int &sock) {
	if(sock>=0) {
		LOGGER("%d ",sock);
		::shutdown(sock,SHUT_RDWR);
		sock=-1;
		}
	}
	*/
#ifndef TESTMENU
#include <mutex>
extern std::mutex change_host_mutex;
#endif
extern bool networkpresent;
class Backup;
extern Backup *backup;
extern Sensoren *sensors;
extern Settings *settings;
typedef pid_t tid_t;
struct numspan {
	int start;
	int end;
};
constexpr const int maxchanged=100;
struct changednums {
	int len;
	struct numspan changed[maxchanged];
	uint32_t lastmeal;
	senddata_t reserved[28];
	uint32_t lastlastpos;
	void clear() {
		len=1;
		changed[0]={};
		lastmeal=0;
		lastlastpos=0;
		}
	};
int  updatenums(crypt_t *,int sock,struct changednums *nums);
inline static constexpr const char backupdat[]="backup.dat";
inline static constexpr const char orbackup[] ="orbackup.dat";
extern int hostsocks[];
extern std::vector<int> sendsocks;
extern uint32_t lastuptodate[];

//extern int tmpsocks[maxallhosts][maxip];
extern std::vector<crypt_t *>crypts;
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
	uint16_t starttimeindex;
	bool resetdevices;
	bool dontuseopen;
	bool sendnums;
	bool sendstream;
	bool sendscans;
	bool restore:7;
	bool sendjugglucoid:1;
	uint8_t pass[16];
	struct changednums nums[2];
	void setindex(int index,int allin) {
		LOGGER("setindex(%d,%d)\n",index,allin);
		ind=index;
		allindex=allin;
		}
	void setcrypt(crypt_t *ctx)  {
		if(crypts.size()>ind) {
			crypts[ind]=ctx;
			}
		else {
			LOGGER("setcrypt ind (%d) <=crypts.size() (%d)\n",ind,crypts.size());
			}
		}
	
	crypt_t *getcrypt() const; 
		/*
	int * gettmpsock() const {
		return tmpsocks[allindex];
		}
		*/
	int &getsock() const {
		return sendsocks[ind];
		}
	void setsock(int val) const {
		sendsocks[ind]=val;
		}
void	open() ;
void close() {	
       const int so= getsock();
       LOGGER("updateone close(%d)\n",so);
       if(so>=0) {
//		mirrorstatus[allindex].sensor.hassocket=false;
		setsock(-1);
		::close(so);
		if(crypt_t *ctx=getcrypt()) {
			ascon_aead_cleanup(ctx);
			}
		}
	}

//void 	setbackupstarttime(const uint32_t starttime);
int updatenums() ;
int updatestreamu() ;
int updatescansu() ;
int update(); 

	};

#include "maxsendtohost.h"
struct updatedata {
	int32_t hostnr;
	uint32_t receive;
	//uint32_t update;
	char port[6];
	bool hasrestore;
	uint8_t sendnr;
	passhost_t allhosts[maxallhosts];
	updateone tosend[maxsendtohost];

	void wakesender() ;
	void wakestreamsender();
	};

class  Backup {

Mmap<unsigned char> mapdata;
public:
struct  condvar_t {
	uintptr_t dobackup=0;
	std::mutex backupmutex;
	std::condition_variable backupcond; 
	/*
void setkind(uintptr_t kind){
	   if(dobackup)
		   dobackup=kind;
	  }
	  */
void wakebackuponly(uintptr_t kind){
	      std::lock_guard<std::mutex> lck(backupmutex);
	      dobackup=kind|(dobackup&(wakeend|wakestop));
	    
	    backupcond.notify_one();                        
	  }
void wakebackup(uintptr_t kind){
	    {
	      std::lock_guard<std::mutex> lck(backupmutex);
	      auto wasdo=dobackup;
	      dobackup=dobackup|kind;
	    if(wasdo) {
       			LOGGER("wakebackup %lx  no-notify\n",kind);
			return;
			}

	    LOGGER("wakebackup %lx notify\n",kind);
	    }
	    backupcond.notify_one();                        
	  }
	};
std::vector<condvar_t*> con_vars;
struct updatedata* getupdatedata() {return reinterpret_cast<struct updatedata*>(mapdata.data());}
const struct updatedata* getupdatedata() const {return reinterpret_cast<const struct updatedata*>(mapdata.data());}

bool getshouldaskfordata() {
	const int len=getupdatedata()->hostnr;
	for(int i=0;i<len;i++) {
		if(getupdatedata()->allhosts[i].receivefrom==3)
			return true;
		}
	return false;
	}
 
bool shouldaskfordata;

void startactivereceivers() {
	const int len=getupdatedata()->hostnr;
	for(int i=0;i<len;i++) {
		hostsocks[i]=-1;
		passhost_t *ph= getupdatedata()->allhosts+i;
		if(ph->activereceive&&!ph->deactivated) {
			setactivereceive(i,ph); 
			}
		}
	}

Backup(std::string_view base): mapdata(base,backupdat,sizeof(struct updatedata)), con_vars(getupdatedata()->sendnr) {

const int len=getupdatedata()->hostnr;
sendsocks.reserve(getupdatedata()->sendnr);
crypts.reserve(getupdatedata()->sendnr);
for(int i=0;i<getupdatedata()->sendnr;i++) {
	sendsocks.push_back(-1);
	auto &host=getupdatedata()->tosend[i];
	if(getupdatedata()->allhosts[host.allindex].haspass()) {
		LOGGER("crypts[%d]=new crypt_t()\n",i);
		crypts.push_back(new crypt_t());
		}
	else  {
		LOGGER("crypts[%d]=nullptr\n",i);
		crypts.push_back(nullptr);
		}
	}



 startactivereceivers();
 /*
for(int i=0;i<len;i++) {
	hostsocks[i]=-1;
	passhost_t *ph= getupdatedata()->allhosts+i;
	if(ph->activereceive) {
		setactivereceive(i,ph); 
		}
	} */
if(!getupdatedata()->port[0])
	strcpy(getupdatedata()->port,defaultport );

void	backupbase(string_view basedir);
backupbase(globalbasedir);
for(int i=0;i<len;i++) {
	if(getupdatedata()->allhosts[i].passive()) {
		startreceiver(false);
		break;
		}
	}


shouldaskfordata=getshouldaskfordata();
/*
#ifdef WEAROS_MESSAGES
extern void startmessagereceivers(Backup*);
if(wearmessages)
	startmessagereceivers(this);
#endif */
}

void resensordata(int sensor)  {
     if(sensor<0) {
     	LOGGER("resensordata(%d)\n",sensor);
	return;
     	}
     for(int i=0;i<getupdatedata()->sendnr;i++) {
	auto &host=getupdatedata()->tosend[i];
	if(host.firstsensor>sensor)
		host.firstsensor=sensor;
	}
  };

bool sendScans() const {
     for(int i=0;i<getupdatedata()->sendnr;i++) {
	auto &host=getupdatedata()->tosend[i];
	if(host.sendscans)
		return true;
	}
     return false;
  }
const char *getmyport() const{
	return getupdatedata()->port;
	}
void startreceiver(bool always=false) {
	if(receiveractive()&&!always)
		return;
	::stopreceiver();
	::startreceiver(getupdatedata()->port,getupdatedata()->allhosts,getupdatedata()->hostnr,hostsocks);
	}
void stopreceiver() {
	::stopreceiver() ;
	for(int i=0;i<getupdatedata()->hostnr;i++) {
		if(getupdatedata()->allhosts[i].receivefrom) {
			int sock=hostsocks[i];
			if(sock>=0) {
				LOGGER("shutdown %d\n",sock);
				::shutdown(sock,SHUT_RDWR);
				}
			}
		}

	}
~Backup() {
	wakebackuponly(wakestop|wakeend);
	}
#include <sys/socket.h>
#include <netdb.h>



static constexpr std::array< char,17> zero{};
const std::array<char,17> getpass(int pos) const {
 	if(getupdatedata()->allhosts[pos].haspass())
		return passback(getupdatedata()->allhosts[pos].pass);
	else
		return zero;
	}
void getport(int pos,char *buf) {
	auto &host=getupdatedata()->allhosts[pos];
	snprintf(buf,6, "%d",host.getport());
//	snprintf(buf,6, "%d",ntohs(host.ips[0].sin6_port));
	}

void addsize() {
	if(getupdatedata()->sendnr>=maxsendtohost) {
		const int newsendnr= getupdatedata()->sendnr+1;
		int newlen=sizeof(updatedata)+(newsendnr-maxsendtohost)*sizeof(struct updateone);
		int len=mapdata.size();
		if(newlen>len) {
			mapdata.extend(globalbasedir,backupdat,newlen); //Goes wrong because of existing pointers to old data
			}
		}
	}
void deupdated() {
	for(int i=0;i<getupdatedata()->sendnr;i++)
		if(getupdatedata()->tosend[i].backupupdated==1)
			getupdatedata()->tosend[i].backupupdated=0; 
	}
void deletestart(int sendindex) {
		for(int i=0;i<(getupdatedata()->hostnr);i++) {
			auto &host=getupdatedata()->allhosts[i];
			if(!host.deactivated) {
				int sin=host.index;
				if(sin>=sendindex) {
				       if(con_vars[sin]) {
					       const int sock= getupdatedata()->tosend[sin].getsock();
					       LOGGER("call wakestop %p\n",con_vars[sin]);
						con_vars[sin]->wakebackuponly(Backup::wakestop);
					       LOGGER(" shutdown %d\n",sock);
						::shutdown(sock,SHUT_RDWR);

					       }
				       }
				 }
			}
		if(--getupdatedata()->sendnr!=sendindex)  {
			int fromend=getupdatedata()->sendnr-sendindex;
			memmove( getupdatedata()->tosend+sendindex,getupdatedata()->tosend+sendindex+1,fromend*sizeof(getupdatedata()->tosend[0]));
			delete crypts[sendindex];
			memmove(&crypts[sendindex], &crypts[sendindex+1],fromend*sizeof(crypts[0]));
			crypts[getupdatedata()->sendnr]=nullptr;
			LOGGER("crypts[%d]=nullptr\n", getupdatedata()->sendnr);
			}
		con_vars[getupdatedata()->sendnr]->wakebackuponly(Backup::wakestop|Backup::wakeend);
		}
void deleteend(int sendindex) {
		for(int i=0;i<backup->getupdatedata()->hostnr;i++) {
			const int sin=getupdatedata()->allhosts[i].index;
			if(sin>sendindex) {
				--getupdatedata()->allhosts[i].index;
				LOGGER("allhosts[%d].index=%d\n",i,getupdatedata()->allhosts[i].index);
				}
			}
		}
void setindices(int start) {
	for(int i=start;i< backup->getupdatedata()->hostnr;i++) {
		const int si=getupdatedata()->allhosts[i].index;
		if(si>-1)  {
			getupdatedata()->tosend[si].setindex(si,i);
			}
		}
	}

void deletehost(int index) {
	if(index>=getupdatedata()->hostnr)
		return;
	LOGGER("deletehost(%d)\n",index);
	auto &thehost=getupdatedata()->allhosts[index];
	if(thehost.activereceive) {
		endactivereceive(index); 
		}
	if(getupdatedata()->allhosts[index].receivefrom) {
		int sock=hostsocks[index];
		if(sock>=0) {
			LOGGER("%d: close(%d)\n",index,sock);
			::shutdown(sock,SHUT_RDWR);
			close(sock);
			hostsocks[index]=-1;
			}
		}
	bool wasnet=networkpresent;
	 networkpresent=false;
	const int sendindex=getupdatedata()->allhosts[index].index;
	if(sendindex>=0) {
		deletestart(sendindex);
		}
	if(--getupdatedata()->hostnr!=index)  {
		LOGGER("--hostnr %d\n",getupdatedata()->hostnr);
		int fromend= getupdatedata()->hostnr-index;
		memmove( getupdatedata()->allhosts+index,getupdatedata()->allhosts+index+1,fromend*sizeof(getupdatedata()->allhosts[0]));
		memmove(  hostsocks+index, hostsocks+index+1,fromend*sizeof(hostsocks[0]));
		memmove(  lastuptodate+index, lastuptodate+index+1,fromend*sizeof(lastuptodate[0]));
		}

	if(sendindex>=0) 	 {
		deupdated();
		deleteend(sendindex);
		}

	if(getupdatedata()->hostnr==0) {
		getupdatedata()->sendnr=0;
		stopreceiver();
		}
	setindices(index);
 	if(!networkpresent)
		networkpresent=wasnet;
	}
void clearhost(int index) {
	LOGGER("clearhost(%d)\n",index);
	updateone &host=getupdatedata()->tosend[index];
	if(int so=host.getsock();so!=-1) {
		LOGGER("shutdown(%d)\n",so);
		::shutdown(so,SHUT_RDWR);
	//	host.setsock(-1);
		}

	sensors->updateinit(index);
	host.nums[0].clear();
	host.nums[1].clear();
	host.startsensors=0;
	host.firstsensor=0;
	host.updatesettings=0;
	host.backupupdated=0;
	host.starttime=0;
	host.starttimeindex=UINT16_MAX;
	host.sendjugglucoid=false;
	}
void resetall()  {
	if(getupdatedata()->sendnr>0)
		deletestart(0);
	getupdatedata()->receive=0;
	getupdatedata()->port[0]='\0';
	getupdatedata()->sendnr=0;
	getupdatedata()->hostnr=0;
	int lenc=crypts.size();
	for(int i=0;i<lenc;i++) {
		LOGGER("crypts[%d]=nullptr\n",i);
		delete crypts[i];
		crypts[i]=nullptr;
		}
//	fill(crypts.begin(),crypts.end(),nullptr);
	}
void changereceiver(int allindex,int index,const bool sendnums,const bool sendstream,const bool sendscans,const bool restore,const bool haspass,const uint32_t starttime) {
	LOGGER("changereceiver(allindex=%d,index=%d,sendnums=%d,sendstream=%d,sendscans=%d,haspass=%d,starttime=%ul)\n",allindex,index,sendnums,sendstream,sendscans,haspass,starttime);
	if(index==getupdatedata()->sendnr) {
		addsize();
		sendsocks.resize(getupdatedata()->sendnr+1,-1);
//		tmpsocks.resize(getupdatedata()->sendnr+1);
		crypts.resize(getupdatedata()->sendnr+1,nullptr);
		clearhost(index);
		getupdatedata()->sendnr++;
		con_vars.resize( getupdatedata()->sendnr);
		}
	else  {
		if(con_vars[index])
			con_vars[index]->wakebackuponly(Backup::wakestop);
		}
	updateone &host=getupdatedata()->tosend[index];
	host.setindex(index,allindex);
//	if(starttime) host.setbackupstarttime(starttime); 
	if(!starttime)
		host.starttime=1;
	else  {
		host.starttime=starttime;
		host.starttimeindex=UINT16_MAX;
		}
	host.sendnums=sendnums;
	host.sendstream=sendstream;
	host.sendscans=sendscans;
	host.restore=restore;
	if(int so=host.getsock();so!=-1) {
		LOGGER("shutdown(%d)\n",so);
		::shutdown(so,SHUT_RDWR);
	//	host.setsock(-1);
		}
	crypt_t *oldcrypt=host.getcrypt();
	if(haspass)  {
		if(!oldcrypt)  {
			LOGGER("crypts[%d]=new crypt\n",index);
			host.setcrypt(new crypt_t);
			}
		}
	else  {
		if(oldcrypt)
			delete oldcrypt;
		host.setcrypt(nullptr);
		LOGGER("crypts[%d]=nullptr\n",index);

		}
	}
static constexpr const std::array<uint8_t,16> remix= {0x19,0xED,0xA0,0x4A,0x94,0x9D,0x0C,0xD7,0x82,0x4A,0x74,0xA9,0x0E,0x71,0x84,0x8B};
//,0x87,0x7F,0x0F,0xD7,0xA8,0xEE,0x8C,0xD7,0x80
static void	setpass( std::array<uint8_t,16> &passuit,const string_view passin) {
	const int len=passin.size();
	if(!len) {
		passuit={};
		return;
		}	
	for(int i=0;i<len;i++) {
		passuit[i]=passin[i]^remix[i];
		}
	for(int i=len;i<16;i++) 
		passuit[i]=remix[i];
	}

static const std::array< char,17> passback(const std::array<uint8_t,16> &passin)  {
	std::array<char,17> plain;
	for(int i=0;i<passin.size();i++) {
		plain[i]=passin[i]^remix[i];
		}
	plain[16]='\0';
	return plain;
	}

void resethost(passhost_t &ph) {
	LOGGER("resethost(%s)\n",ph.getnameif());
	if(ph.index>=0) {  
		int tohost=ph.index;
		clearhost(tohost);
		}
	}
void resethost(int index) {
	if(index<getupdatedata()->hostnr) {
		resethost(getupdatedata()->allhosts[index]);
		}
	} 
/*
void resethost(int index) {
	if(index<getupdatedata()->hostnr&&getupdatedata()->allhosts[index].index>=0) {  //Fout??
		int tohost=getupdatedata()->allhosts[index].index;
		clearhost(tohost);
		}
	} */


int jsetips(const char *port,JNIEnv *env, const jobjectArray jar, const int len,struct sockaddr_in6 *connect ,const int lmaxip) {
	int uselen=std::min(lmaxip,len);
	for(int i=0;i<uselen;i++) {
		jstring  jname=(jstring)env->GetObjectArrayElement(jar,i);
		int namelen= env->GetStringUTFLength( jname);
		char name[namelen+1];

		jint jnamelen = env->GetStringLength( jname);
		env->GetStringUTFRegion( jname, 0,jnamelen, name); name[namelen]='\0';
		if(!getaddr(name,port,connect+i))
			return  -1;

		}
	return uselen;
	}
int setips(const char *port, const char **names, const int len,struct sockaddr_in6 *connect ,const int lmaxip) {
	int uselen=std::min(lmaxip,len);
	for(int i=0;i<uselen;i++) {
		LOGGER("setip %s\n",names[i]);
		if(!getaddr(names[i],port,connect+i))  {
			LOGGER("getaddr(%s,%s,...) failed\n",names[i],port);
			return  -1;
			}

		}
	return uselen;
	}
	/*
	   case -1: mess="Port should between 1024 and 65535";break;
                                case -2: mess="Error parsing ip";break;
				*/

void endactivereceive(int allindex) { 
	extern std::vector<condvar_t*> active_receive;
	passhost_t *ph=getupdatedata()->allhosts+allindex;
	if(ph->activereceive) {
		if(!ph->deactivated)
			active_receive[ph->activereceive-1]->wakebackup(Backup::wakeend);
		ph->activereceive=0;
		LOGGER("endactivereceive(%d) shutdown(%d)\n",allindex,hostsocks[allindex]);
		::shutdown(hostsocks[allindex],SHUT_RDWR);
		}
	}
void setactivereceive(int allindex,passhost_t *ph,bool startthread=true) { 

	extern std::vector<condvar_t*> active_receive;
		active_receive.push_back(new condvar_t);
		ph->activereceive=active_receive.size();
		LOGGER("setactivereceive allindex=%d nr=%d\n",allindex,ph->activereceive);

		if(startthread) {
			void activereceivethread(int allindex,passhost_t *pass) ;
			std::thread the(activereceivethread,allindex,ph);
			the.detach();
			}
		}

int changehost(int index,JNIEnv *env,jobjectArray jnames,int nr,bool detect,string_view port,const bool sendnums,const bool sendstream,const bool sendscans,const bool restore,const bool receive,const bool activeonly,string_view pass,uint32_t starttime,bool passiveonly,const char *label=nullptr,const bool testip=true,bool startthreads=true,bool hashostname=false) {
	LOGGER("changehost(%d,sendnums=%d,sendstream=%d,sendscans=%d,receive=%d,activeonly=%d,passiveonly=%d,label=%s port=%s nr=%d hashostname=%d\n",index,sendnums,sendstream,sendscans,receive,activeonly,passiveonly,label,port.data(),nr,hashostname);
	const int hostnr=getupdatedata()->hostnr;
	if(index<0) 
		index=hostnr;
	if(index>=maxallhosts)  {
		LOGSTRING("changehost: index>=maxallhosts\n");
		return -3;
		}
	const bool receiveactive=receive&&activeonly;
	if(port.data()==nullptr||port.size()==0) {
		port={defaultport,sizeof(defaultport)-1};
		}
	else {
		if(port.size()>5) {
			LOGSTRING("changehost: port.size()>5)\n");
			return -1;
			}
		}
	int portint=atoi(port.data());
	if(portint>65535||portint<1024) {
		LOGGER("port out of range %d\n",portint);
		}

 	struct oldnet {
		bool wasnet=networkpresent;
		oldnet() {
			networkpresent=false;
			}
		~oldnet() {
			if(!networkpresent)
				networkpresent=wasnet;
			};
		};
	struct oldnet desnet;
	const bool newhost=(index==hostnr);

	const bool sendto= sendnums|| sendstream|| sendscans;
	const bool reconnect=(receive&&!passiveonly)||(sendto&&!activeonly);
	LOGGER("hostnr=%d\n",hostnr);
	int tohost;
	bool newthread=false;
	const bool dontopen=sendto&&passiveonly;
	int lmaxip=maxip-(label?1:0);
	auto &thehost=getupdatedata()->allhosts[index];

	if(sendto) {
		if(newhost||thehost.index==-1) {  //Fout??
			tohost=getupdatedata()->sendnr;
			if(tohost>=maxsendtohost) {
				LOGSTRING("changehost: tohost>=maxsendtohost\n");
				return -4;
				}
			thehost.index=tohost;
			newthread=true;
			}
		else  {
			tohost=thehost.index;
			}

		changereceiver(index,tohost,sendnums,sendstream,sendscans,restore,pass.size(),starttime);
		thehost.sendpassive=dontopen;
		}
	else {
		tohost=0;
		thehost.sendpassive=false;
		int sendindex=thehost.index;
		if(!newhost) {
			if(sendindex>=0) {
				deletestart(sendindex);
				thehost.index=-1;
				deleteend(sendindex); 
				setindices(index);
				}
			}
		thehost.index=-1;
		}

	if(thehost.activereceive)
			endactivereceive(index) ;
	int res;
	if(hashostname) {
		thehost.hostname=true;
		if(env) {
			jstring  jhostname=(jstring)env->GetObjectArrayElement(jnames,0);
			int namelen= env->GetStringUTFLength( jhostname);
			if(namelen>maxhostname) {
				LOGGER("supplied name too long %d>%d\n",namelen,maxhostname);
				return -5;
				}

			jint jnamelen = env->GetStringLength( jhostname);

			char *hostname=thehost.gethostname();

			env->GetStringUTFRegion( jhostname, 0,jnamelen, hostname); 
			hostname[namelen]='\0';
			}
		else {
			LOGAR("What to do with hashostname with env=null?");
			return -6;
			}
		thehost.setportwithhostname(portint);
		res=thehost.nr=1;
		detect=false;
		}
	else   {

        thehost.hostname=false;
		res=thehost.nr=env?jsetips(port.data(),env, jnames, nr,thehost.ips,lmaxip): setips(port.data(),(const char **)jnames, nr,thehost.ips,lmaxip);
		}
	int ret=index;
	
	if(res<0) {
		thehost.nr=0;
		ret=-2;
		}
	if(detect) {
		if(res<lmaxip)  {
			thehost.detect=true;
			thehost.ips[res]={.sin6_family=AF_INET6,.sin6_port=htons(atoi(port.data())),.sin6_addr=noaddress};
			}
		else
			thehost.detect=false;
		}
	else {
		if(res==0&&!(passiveonly&&!testip))   {
			LOGSTRING("res==0&&!(passiveonly&&!testip))\n");
			ret=-2;
			}
		thehost.detect=false;
		}
	if(label) {
		thehost.setname(label);
		}
	else
		thehost.hasname=false;
	thehost.noip=!testip;
	lastuptodate[index]=0;
/*
Receive	   	reconnect	receivefrom:
true		true 	  	3
true	  	false	  	2
false		true 	 	1
false	  	false	  	0
*/
	thehost.receivefrom=receive?(reconnect?3:2):((sendto&reconnect)?1:0);
	LOGGER("receivefrom=%d\n", thehost.receivefrom);
	setpass( thehost.pass,pass);
	thehost.deactivated=false;

	if(newhost)  {
		++(getupdatedata()->hostnr);
		thehost.wearos=false;
		LOGSTRING("new host ++hostnr\n");
		thehost.newconnection=true;

		}
	else {
		LOGGER("wearos(%d)=%d\n", index,thehost.wearos);
		}

	deupdated(); 
	if(startthreads) {
		if(newthread)
			startthread(index,tohost);
		if(!activeonly)
			startreceiver(false);
		}
	LOGGER("activereceive was=%d nu=%d\n",thehost.activereceive,receiveactive);
	if(receiveactive) {
		setactivereceive(index,getupdatedata()->allhosts+index,startthreads);
		}
	else
		thehost.activereceive=0;
		
	shouldaskfordata=getshouldaskfordata();
#ifdef WEAROS_MESSAGES
extern	void clearnetworkcache();
	clearnetworkcache();
	#endif
	LOGGER("changehost=%d\n",res);
	return ret;
	}
bool isreceiving() const {
	const auto nr=getupdatedata()->hostnr;
	const passhost_t *host=getupdatedata()->allhosts;
	bool receives=false;
	for(int i=0;i<nr;i++) {
		if(host[i].receivefrom&2)  {
			auto ind=host[i].index;
			if(ind<0||!getupdatedata()->tosend[ind].sendstream)
				receives=true;
			}
		}
	return receives;
//	return getupdatedata()->hostnr!= getupdatedata()->sendnr;
	}

void deactivateHost(int index,bool deactive) {
	LOGGER("deactivateHost(%d,%d)\n",index,deactive);
	if(index>=getupdatedata()->hostnr) 
		return ;
	auto &host=getupdatedata()->allhosts[index];

	if(host.deactivated==deactive)
		return;
	host.deactivated=deactive;
	if(deactive) {
		if(host.activereceive) {
			LOGGER("stop active receive 	shutdown(%d)\n",hostsocks[index]);
            extern std::vector<condvar_t*> active_receive;
			active_receive[host.activereceive-1]->wakebackup(Backup::wakeend);
			::shutdown(hostsocks[index],SHUT_RDWR);
			}
		else {
			if(host.receivefrom) {
				int sock=hostsocks[index];
				if(sock>=0) {
					LOGGER("%d: close(%d)\n",index,sock);
					::shutdown(sock,SHUT_RDWR);
					close(sock);
					hostsocks[index]=-1;
					}
				}
			}
		int sin=host.index;
		if(sin>=0) {
		       const int sock= getupdatedata()->tosend[sin].getsock();
		       LOGGER(" shutdown %d\n",sock);
		       if(con_vars[sin])
				con_vars[sin]->wakebackuponly(Backup::wakestop|Backup::wakeend);
			::shutdown(sock,SHUT_RDWR);
			}

		}
	else {
		if(host.index>=0)
			startthread(index,host.index);
		if(host.activereceive) {
			setactivereceive(index,&host);
			}
		}
	}
void notupdatedsettings() {

	wakebackup(wakeall);
	}
struct changednums *getnums(int hostindex,int numindex) { 
	return getupdatedata()->tosend[hostindex].nums+numindex;
	}
int gethostnr() const {
	return getupdatedata()->hostnr;
	}
int getsendhostnr() const {
	return getupdatedata()->sendnr;
	}
//int histnr=0;
//SensorGlucoseData *backuphist=nullptr;
//static constexpr const uintptr_t wakereopen=1;
static constexpr const uintptr_t wakestream=1;
static constexpr const uintptr_t wakescan=2;
static constexpr const uintptr_t wakenums=4;
static constexpr const uintptr_t wakeall=8;
static constexpr const uintptr_t wakestop=16;
static constexpr const uintptr_t wakeother=32;
static constexpr const uintptr_t wakeend=64;
static constexpr const uintptr_t wakesend=128;
static constexpr const uintptr_t wakereconnect=256;
static constexpr const uintptr_t wakestreamsend=512;
void closeallsocks() {
	LOGSTRING("closeallsocks\n");
	for(int i=0;i<getupdatedata()->hostnr;i++) {
		 LOGGER("host %d shutdown(%d)\n",i,hostsocks[i]);
		::shutdown(hostsocks[i],SHUT_RDWR);
		}
	for(int i=0;i<getupdatedata()->sendnr;i++) {
		if(sendsocks[i]>0) {
		      LOGGER("send %d shutdown(%d)\n",i,sendsocks[i]);
			::shutdown(sendsocks[i],SHUT_RDWR);
			}
		else
		   LOGGER("sendsock[%d]==-1\n",i);
		}
	}
bool sendwakesender(int h) {
	updateone &shost=getupdatedata()->tosend[h];
	shost.close();
	shost.open();
	return sendbackup(shost.getcrypt(),shost.getsock());
	}

bool sendwakestreamsender(int h) {
	updateone &shost=getupdatedata()->tosend[h];
	shost.close();
	shost.open();
	extern	bool sendwakeupstream(crypt_t *pass,const int sock);
	return sendwakeupstream(shost.getcrypt(),shost.getsock());
	}
	/*
extern int updateproc(condvar_t *varsptr,uintptr_t cond,updateone &shost,int  (updateone::*proc)( )) ;
 extern void	endbackupthread(int h) ;
extern void		doupdates(const uintptr_t current,const int h) ;
extern void lockwait(uintptr_t &current,int h) ;
extern void		notpassive(uintptr_t current,int sendindex);
extern bool doend(int sendindex); */
void backupthread(int allindex,int sendindex) {
	LOGGER("%d backupthread, wearos=%d\n", allindex,getupdatedata()->allhosts[allindex].wearos);
//	const int sendindex=getupdatedata()->allhosts[allindex].index;
	const bool passive=getupdatedata()->allhosts[allindex].sendpassive;

	auto &status=mirrorstatus[allindex].sender;
	status.start();
	destruct _dest([&status]{ status.stop();});
	{
	constexpr int maxbuf=15;
	char buf[maxbuf];
	 snprintf(buf,maxbuf,"send %d",sendindex);
	   LOGGER("%s\n",buf);
	   prctl(PR_SET_NAME, buf, 0, 0, 0);
	   }
	uintptr_t current=0;
	   while(true) {
	   	if(doend(sendindex))
			return;
		status.locked=true;
		 lockwait(current,sendindex);
		status.locked=false;

	   	if(doend(sendindex))
			return;
		if(!passive) {
			notpassive(current,sendindex);
			}
		static int count=0;
		if(current==wakestream) {
			if(count++%5==4) {
				current=wakeall;
				}
			}
			
		doupdates(current,sendindex); 
		}
	}

void		notpassive(uintptr_t current,int sendindex) {
		if(current&wakereconnect)  {
			getupdatedata()->tosend[sendindex].close();
			getupdatedata()->tosend[sendindex].open();
			} 
		if(current&wakesend) {
			sendwakesender(sendindex);
			}
		if(current&wakestreamsend) {
			sendwakestreamsender(sendindex);
			}
	}
bool doend(int sendindex) {
	LOGGER("doend: con_vars=%p\n",con_vars[sendindex]);
	  if(con_vars[sendindex]->dobackup&wakeend)  {
		endbackupthread(sendindex);
		return true;
		}
	return false;
	}

int updateproc(condvar_t *varsptr,uintptr_t cond,updateone &shost,int  (updateone::*proc)( )) {
	  if(varsptr->dobackup&wakestop)  
	  	return 0;
	if(cond) {
		int res= (shost.*proc)();
		if(!res) {
			auto *pass=backup->getupdatedata()->allhosts+shost.allindex;
			savemessage(pass,"Send failed");
		//	if(!pass->sendpassive)
			shost.close();
			if(
#ifdef WEAROS_MESSAGES
			(!pass->wearos||!wearmessages[shost.allindex])&&
#endif
			pass->sendpassive)
				return 0;
			  if(varsptr->dobackup&wakestop)  
				return 0;
			shost.open();
			  if(varsptr->dobackup&wakestop)   {
				shost.close();
				return 0;
				}
			res= (shost.*proc)();
			if(!res)
				shost.close();
			}
		return res;
	   	}
	  else
		  return 2;
	}


  void	endbackupthread(int h) {
	LOGGER("%d: end backupthread  %p\n",h,con_vars[h]);
#ifndef TESTMENU
  	const std::lock_guard<std::mutex> lock(change_host_mutex);
#endif

	getupdatedata()->tosend[h].close();
	delete con_vars[h];
	con_vars[h]=nullptr;
	LOGSTRING("after delete con_vars[h]\n");
	}

void		doupdates(const uintptr_t current,const int h) { 
		int didnums=0,didstream=0,didscans=0,didupdate=0;
		(didnums=updateproc(con_vars[h],current&wakenums,getupdatedata()->tosend[h],&updateone::updatenums))&&
			(didstream=	updateproc(con_vars[h],current&wakestream,getupdatedata()->tosend[h],&updateone::updatestreamu))&&
			(didscans=		updateproc(con_vars[h],current&wakescan,getupdatedata()->tosend[h],&updateone::updatescansu))&&
			(didupdate=		updateproc(con_vars[h],current&wakeall,getupdatedata()->tosend[h],&updateone::update));
		const uint16_t command=((didnums&1)?wakenums:0)|(didstream&1?wakestream:0)|(didscans&1?wakescan:0)|(didupdate&1?wakeall:0);
		if(command)
			sendrender(getupdatedata()->tosend[h].getcrypt(),getupdatedata()->tosend[h].getsock(),command);
		}
void lockwait(uintptr_t &current,int h) {
	LOGGER("%d before lock\n",h)	;
	    std::unique_lock<std::mutex> lck(con_vars[h]->backupmutex);
	    con_vars[h]->dobackup=con_vars[h]->dobackup&~current;
	    con_vars[h]->backupcond.wait(lck, [h] {return backup->con_vars[h]->dobackup; });   
	    current=con_vars[h]->dobackup;
	  LOGGER("%d after lock sock=%d\n",h,getupdatedata()->tosend[h].getsock())	;
	}

//void streambackup(int index) { wakebackup(wakestream); }
typedef std::conditional<sizeof(unsigned long) == sizeof(uintptr_t), unsigned long, uintptr_t>::type myuintptr_t; 
//TO get rid of stupid clang++ warnings, that I shouldnt specify %lx  even if sizeof(long)==sizeof(int)

void wakebackuponly(myuintptr_t kind=wakeall){
	LOGGER("start wakebackuponly %lx\n",kind);
	int nr;
	if(getupdatedata()->sendnr>con_vars.size()) {
		nr=con_vars.size();
		LOGGER("ERROR getupdatedata()->sendnr>con_vars.size() %d %d\n",getupdatedata()->sendnr,nr);
		}
	else
		nr=getupdatedata()->sendnr;
	  for(int i=0;i<nr;i++) {
		auto *el=con_vars[i];
	  	if(el)
			el->wakebackuponly(kind);
		}
	LOGSTRING(" end wakebackuponly\n");
  }
void wakebackup(myuintptr_t kind=wakeall){
	LOGGER("start wakebackup %lx\n",kind);
	int nr;
	if(getupdatedata()->sendnr>con_vars.size()) {
		nr=con_vars.size();
		LOGGER("ERROR getupdatedata()->sendnr>con_vars.size() %d %d\n",getupdatedata()->sendnr,nr);
		}
	else
		nr=getupdatedata()->sendnr;
	  for(int i=0;i<nr;i++) {
		auto *el=con_vars[i];
	  	if(el)  {
			bool doe; 
			if(networkpresent)  {
				doe=true;
				}
			else {
				const auto &here=getupdatedata()->tosend[i];
				const int index=here.allindex;
				const auto &host=getupdatedata()->allhosts[index];
				if(host.wearos) {
					LOGSTRING("networkabsent wearos->wake\n");
					doe=true;
					}
				else {
					LOGSTRING("networkabsent !wearos\n");
					doe=false;
					}
				}
			if(doe) {
				LOGGER("host %d wake\n",i);
				el->wakebackup(kind);
				}
			}
		}
	LOGSTRING(" end wakebackup\n");
  }
  static void startthread(int allindex,int sendindex) {
         LOGGER("in startthread %d %d\n",allindex,sendindex);
	backup->con_vars[sendindex]=new condvar_t;
	std::thread back(&Backup::backupthread,backup,allindex,sendindex);
	back.detach();
	}
static void startbackup(std::string_view globalbasedir) {
	LOGSTRING("startbackup\n");
	backup=new(std::nothrow) Backup(globalbasedir);
	if(backup) {
		const int maxsend=backup->getupdatedata()->sendnr;
		const int hostnr= backup->getupdatedata()->hostnr;
		for(int i=0;i<hostnr;i++) {
			auto &host=backup->getupdatedata()->allhosts[i];
			int index=host.index;
			LOGGER("index=%d\n",index);
			if(index>=maxsend) {
				LOGGER("allhosts[i].index %d >= %d sendnr\n",index,maxsend);
				host.index=-1;
				}
			else {
				if(index>=0) {
					if(host.deactivated) {
						LOGGER("deactivated %d %d\n",i,index);
					}
					else {
						LOGGER("startthread %d %d\n",i,index);
						startthread(i,index);
						}
					}
				else {
					LOGSTRING(" no start\n");
					}
				}
			}
		}
	
	}
bool hassendnum() {
	auto sendnr=getupdatedata()->sendnr;
	for(int i=0;i< sendnr;i++) {
		if(getupdatedata()->tosend[i].sendnums)
			return true;
		}
	return false;
	}
/*void updatestream(SensorGlucoseData *hist) {
    int nrh= getupdatedata()->hostnr;
    if(!nrh)
    	return;
    for( int h=0;h<nrh;h++) {
		if(!hist->updatestream(getupdatedata()->tosend[h].sock,h)) {
			getupdatedata()->tosend[h].close();
			getupdatedata()->tosend[h].sock=-1;
      			wakebackup(wakereopen);
			}
	      }
   }*/

friend int updateone::update() ;
inline std::span<passhost_t> getHosts() {
	return { getupdatedata()->allhosts,static_cast<std::span<passhost_t>::size_type>( getupdatedata()->hostnr)};
	}
};

//extern void wakebackup(int kind=1);


extern void startbackup(std::string_view globalbasedir) ;


inline std::span<passhost_t> getBackupHosts() {
	return backup-> getHosts();
	}
inline int gethostindex(passhost_t *host) {
	return host-getBackupHosts().data();
	}
