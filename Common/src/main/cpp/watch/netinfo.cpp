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


#include <algorithm>
#ifdef WEAROS_MESSAGES
#include <zlib.h>
#endif
#include "net/netstuff.h"
#include "share/fromjava.h"
#include "datbackup.h"
#ifdef WEAROS
#define iswatchapp() 1
#else
#define iswatchapp() 0
#endif
#include <array>
std::array<int,maxallhosts>		peers2us,us2peers;
//void setall(std::array<int<maxallhosts>&ar,const int ini) {	
#define LOGGERTAG(...) LOGGER("netinfo: " __VA_ARGS__)
#define LOGSTRINGTAG(...) LOGSTRING("netinfo: " __VA_ARGS__)


extern void setBlueMessage(int,bool val);
extern bool getpassive(int pos) ;
extern bool getactive(int pos) ;
extern bool getownip(struct sockaddr_in6 *outip);
static constexpr const uint8_t thisversion=2;
static uint8_t usedversion=thisversion;
struct netinfo {
	struct sockaddr_in6 ip;
	bool watchsensor:1;
	uint8_t version:7;
	char label[17];
	};

struct netinfo1 {
	union {
		struct sockaddr_in6 ip;
		char newlabel[17];
		};
	bool watchsensor:1;
	uint8_t version:5;
	bool sendnums:1;
	bool sendscans:1;
	struct sockaddr_in6 ips[3];
	int nr;
	int index;
	bool blue;
	};

bool isGalaxyWatch=true;
extern updateone &getsendto(int index);
// bool mkwearos=false;
#include <mutex>
std::mutex change_host_mutex;
passhost_t * getwearoshost(const bool create,const char *label,bool galaxy) {
  const std::lock_guard<std::mutex> lock(change_host_mutex);

    struct updatedata *update=backup->getupdatedata();
     int nrhost=update->hostnr;
    passhost_t *hosts=update->allhosts;
    passhost_t *endhosts=update->allhosts+nrhost;
    passhost_t *found= std::find_if(hosts,endhosts,[label](const passhost_t &host){
    		 const bool same=host.hasname&&!strncmp(label,host.getname(),passhost_t::maxnamelen);
		 LOGGERTAG("%s %d (as %s)\n",host.getname(),same,label);
		 return same;
		});
    if(found==endhosts) {
    	if(!create) {
		LOGSTRINGTAG("!create\n");
		return nullptr;
		}
	
    	if(nrhost==maxallhosts) {
		LOGGERTAG("nrhost==maxallhosts==%d\n",nrhost);
		--nrhost;
		}
	bool sendstream, sendscans, receive,sendnums,activeonly,passiveonly;

	if constexpr( iswatchapp()) {
		LOGSTRINGTAG("watch app\n");
		sendstream=false;
		sendscans=false;
		sendnums=false;
		receive=true;

		if(isGalaxyWatch) {  
			LOGSTRINGTAG("I am Galaxy Watch\n");
			activeonly=true;
			passiveonly=false;
			}
		else {
			LOGSTRINGTAG("I am No Galaxy Watch\n");
			activeonly=false;
			passiveonly=true;
			}
		}
	else {
		LOGSTRINGTAG("no watch app\n");
		sendstream=true;
		sendscans=true;
		sendnums=true;
		receive=false;
		if(galaxy) {
			LOGSTRINGTAG("connected to galaxy\n");
			activeonly=false;
			passiveonly=true;
			}
		else {
			LOGSTRINGTAG("not connected to galaxy\n");
			activeonly=true;
			passiveonly=false;
			}
		}

        int ret=backup->changehost(nrhost,nullptr,nullptr,0,false,defaultport,sendnums, sendstream, sendscans,false, receive,activeonly ,string_view(nullptr,0),0,passiveonly,label,false,true);
	if(ret<0&&ret!=-2) { 
		LOGSTRINGTAG("changehost<0\n");
		return nullptr;
		}

	found=backup->getupdatedata()->allhosts+nrhost; //extend?
	found->wearos=true;
	LOGGERTAG("getwearoshost new(%d)\n",nrhost);
    	}
    return found;
    }
static void setdefaults(const char *infolabel,bool galaxy) {
   passhost_t *host=getwearoshost(false,infolabel,galaxy);
   if(host) {
   	 LOGGERTAG("setdefaults(%s,%d)\n",infolabel,galaxy);
        	struct updatedata *update=backup->getupdatedata();
		int index=host-update->allhosts;
    		const uint16_t port=host->getport();
		char portstr[7];
		snprintf(portstr,6,"%d",port); 
		const int ipsnr=host->nr;
		const char *names[ipsnr];
		namehost hostnames[ipsnr];
		for(int i=0;i<ipsnr;i++) {
			hostnames[i]=namehost(host->ips+i);
			names[i]=hostnames[i].data();
			LOGGERTAG("host: %s\n",names[i]);
			}
		auto [_id,lasttime]=sensors->lastpolltime();
		bool activeonly;
		bool passiveonly;
		bool sendnums;
		bool sendstream;
		bool sendscans;
		bool receive;
	    if constexpr(iswatchapp()) {
			LOGSTRINGTAG("is watch\n");
			sendnums=false;
			sendstream=false;
			sendscans=false;
			receive=true;
			if(isGalaxyWatch) {  
				LOGSTRINGTAG("I am Galaxy Watch\n");
				activeonly=true;
				passiveonly=false;
				}
			else {
				LOGSTRINGTAG("I am No Galaxy Watch\n");
				activeonly=false;
				passiveonly=true;
				}
			}
		else  {
			LOGSTRINGTAG("no watch app\n");
			sendstream=true;
			sendscans=true;
			sendnums=true;
			receive=false;
			if(galaxy) {
				LOGSTRINGTAG("connected to galaxy\n");
				activeonly=false;
				passiveonly=true;
				}
			else {
				LOGSTRINGTAG("not connected to galaxy\n");
				activeonly=true;
				passiveonly=false;
				}
			}

		backup->changehost(index,nullptr,(jobjectArray)names,ipsnr,true,portstr,sendnums, sendstream, sendscans,false, receive,activeonly ,string_view(nullptr,0),lasttime,passiveonly,infolabel,false,true);
		}
	}


updateone &getsendto(const passhost_t *host);
static bool watchsensor(const passhost_t *wearhost) {
	if(!wearhost)
		return false;
       if constexpr(iswatchapp()) {
       	     LOGGERTAG("is watch isSender=%d\n",wearhost->isSender());
       	      if(wearhost->isSender()&&getsendto(wearhost).sendstream)  {
	      		LOGSTRINGTAG("watch sender\n");
			return true;
			}
		else  {
	      		LOGSTRINGTAG("watch no sender\n");
			return false;
			}
       		}
	else {
       	     LOGGERTAG("is no watch isSender=%d\n",wearhost->isSender());
		if(wearhost->isSender()&&getsendto(wearhost).sendstream) {
	      		LOGSTRINGTAG("watch no sender\n");
			return false;
			}
		else  {
	      		LOGSTRINGTAG("watch sender\n");
	      		return true;
			}
		}
	}
/*watchsensor
-1: not
1: yes
0: don't change
*/

extern		int getownips(struct sockaddr_in6 *outips,int max,bool &) ;



static int getreceivefrom(int index,bool receive) {
	bool sendto;
	if(index<0) {
		sendto=false;
		}
	else {
		updateone &updat= getsendto(index);
		sendto=updat.sendnums||updat.sendstream||updat.sendscans;
		}
	const bool		activeonly=getactive(index);
	const bool		passiveonly=getpassive(index);
	const bool reconnect=(receive&&!passiveonly)||(sendto&&!activeonly);
	int res=receive?(reconnect?3:2):((sendto&&reconnect)?1:0);
	LOGGER("getreceivefrom(%d,%d)=%d\n",index,receive,res);
	return res;
	}


void		setsendinfo(struct netinfo1 &info,passhost_t *wearhost) {
			if(usedversion) {
				if(wearhost->index>=0) {
					updateone &updat= getsendto(wearhost);
					info.sendnums=updat.sendnums;
					info.sendscans=updat.sendscans;
					}
				else {
					info.sendscans=info.sendnums=false;
					}
				}
			}

#ifdef WEAROS_MESSAGES
static uLong crcs[maxallhosts]={};
bool wearmessages[maxallhosts]={};
#endif
extern "C" JNIEXPORT  jbyteArray  JNICALL   fromjava(getmynetinfo)(JNIEnv *env, jclass cl,jstring jident,jboolean create,jint watchHasSensor,jboolean galaxy) {

	if(!backup) {
		LOGSTRINGTAG("getmynetinfo backup=null\n");
		return nullptr;
		}
	if(!jident) {
		LOGSTRINGTAG("jident=null\n");
		return nullptr;
		}
      const char *id = env->GetStringUTFChars( jident, NULL);
        if (id == nullptr) {
		LOGSTRINGTAG("id=null\n");
		return nullptr;
		}
        destruct   dest([jident,id,env]() {env->ReleaseStringUTFChars(jident, id);});
	struct netinfo1 info;
	auto myport=atoi(backup->getmyport());
	LOGGERTAG("getmynetinfo(%s,%d,%d) port=%d\n", id,create,watchHasSensor,myport);
	passhost_t *wearhost=getwearoshost(create,id,galaxy);
	if(!wearhost)  {
		LOGSTRINGTAG("wearhost==null\n");
		return nullptr;
		}
        struct updatedata *update=backup->getupdatedata();
	int index=wearhost-update->allhosts;
	info.index=index;




	if(usedversion) {
		bool haswlan;
		info.nr=getownips(info.ips,maxip-1,haswlan);

		LOGGERTAG("send %d ips:\n",info.nr);
		for(int i=0;i<info.nr;i++) {
			info.ips[i].sin6_port= htons(myport);
			#ifndef NOLOG
			namehost name(info.ips+i);
			LOGGERTAG("%s\n",name.data());
			#endif
			}

#ifdef WEAROS_MESSAGES
	    auto newcrc=crc32(0,reinterpret_cast<const Bytef*>(info.ips),info.nr*sizeof(info.ips[0]));
	    if(newcrc!=crcs[index]) {
	    	LOGSTRINGTAG("crc different\n");
		const bool setmess=!haswlan;
		#ifndef WEAROS
			if(setmess)
		#endif
				setBlueMessage(index,setmess);
		crcs[index]=newcrc;
		}
	else  {
	    	LOGSTRINGTAG("crc the same\n");
		}
	info.blue=wearmessages[index];
#endif
		}
	else  {
		if(!getownip(&info.ip)) {
			LOGSTRINGTAG("!getownip\n");
			return nullptr;
			}
		info.ip.sin6_port= htons(myport);
		}
	if constexpr(!iswatchapp()) {
		if(watchHasSensor) { 
		//Als phone helemaal niets zend gaat het mis.
		//Receive onbekend. Nums kunnen helemaal niet overgezonden worden.
			info.watchsensor=watchHasSensor>0;
			bool receive;
			if(watchHasSensor>0) {
				if(!wearhost->activereceive) {
						if(!galaxy) 
							backup->setactivereceive(index,wearhost,true);
						}
				if(wearhost->index>=0) {
					updateone &updat= getsendto(index);
					updat.sendstream=false;
					if(usedversion) {
						info.sendnums=updat.sendnums;
						info.sendscans=updat.sendscans;
						}
					}
				else {
					if(usedversion) {
						info.sendscans=info.sendnums=false;
						}
					}
				receive=true;
				}
			else {
				updateone &updat= getsendto(index);
				updat.sendstream=true;
				if(usedversion) {
					info.sendnums=updat.sendnums;
					info.sendscans=updat.sendscans;
					}
				if(updat.sendnums)
					receive=false;
				else
					receive=true;
				}
			wearhost->receivefrom=getreceivefrom(index,receive);
			}
		else {
			info.watchsensor=watchsensor(wearhost);
			setsendinfo(info,wearhost);
		   }
		}
	else {
		info.watchsensor=watchsensor(wearhost);
		setsendinfo(info,wearhost);
		}

	info.version=info.watchsensor?thisversion:usedversion;
	char *infolabel=usedversion?info.newlabel:reinterpret_cast<netinfo *>(&info)->label;
	strcpy(infolabel, wearhost->getname()); 
	const int len=usedversion?sizeof(netinfo1):sizeof(netinfo);
	jbyteArray uit = env->NewByteArray(len);
	env->SetByteArrayRegion(uit, 0, len, reinterpret_cast<const jbyte *>(&info));
	return uit;
	}


extern "C" JNIEXPORT jboolean  JNICALL   fromjava(setmynetinfo)(JNIEnv *env, jclass cl,  jstring jident, jbyteArray jar,jboolean galaxy) { 
   if(!jar) return false;
   if(!backup) return false;
	if(!jident) return false;
   const char *id = env->GetStringUTFChars( jident, NULL);
   if (id == nullptr) return false;
	destruct   dest([jident,id,env]() {env->ReleaseStringUTFChars(jident, id);});

    const jsize lens=env->GetArrayLength(jar);

    usedversion=(lens==sizeof(netinfo))?0:(lens==sizeof(netinfo1)?3:1);
    jbyte data[lens];
    env->GetByteArrayRegion(jar, 0, lens,data);
    const netinfo1 *info=reinterpret_cast<const netinfo1*>(data);
    passhost_t *host=getwearoshost(true,id,galaxy);
    if(!host) return false;
   networkpresent=false;
   backup->closeallsocks();
   struct updatedata *update=backup->getupdatedata();
    passhost_t *allhosts=update->allhosts;
   int index=host-allhosts;
   const char *infolabel=usedversion?info->newlabel:reinterpret_cast<const netinfo *>(info)->label;
   LOGGERTAG("setmynetinfo %s usedversion=%d infolabel=%s\n",id,usedversion,infolabel);
    host->setname(infolabel);
   if(!usedversion) {
	   namehost hostnamer(&info->ip);
	   namehost oldname(host->ips);
	   LOGGERTAG("hostname %s->%s\n",oldname.data(),hostnamer.data());
	    if(!host->putip(&info->ip)) {
		LOGSTRINGTAG("putip failed\n");
		}
	    }
   else  {
	if(usedversion>=3) {
		int otherindex= info->index;
		peers2us[otherindex]=index;
		us2peers[index]=otherindex;
		}

	   namehost oldname(host->ips);
	   LOGGERTAG("hostname %s->new names:\n",oldname.data());
	   if(info->nr) {
	   	#ifndef NOLOG
		for(int i=0;i<info->nr;i++) {
			namehost name(info->ips+i);
			LOGGERTAG("%s port=%d\n",name.data(), ntohs( info->ips[i].sin6_port));
			}
		#endif
		   host->putips(info->ips,info->nr);
		   }

#ifdef WEAROS_MESSAGES
	if(usedversion>=3) {
		#ifndef WEAROS
			setBlueMessage(index,info->blue);
		//if(info->blue)
		#endif
		}
#endif
   	}
    const uint16_t port=host->getport();
    LOGGERTAG("setmynetinfo port=%d nr=%d\n",port,host->nr);
    if constexpr(iswatchapp()) {
    	LOGSTRINGTAG("is watch\n");
    	if(info->watchsensor) {
		settings->data()->nobluetooth=false;
		bool sendnums=false;
		if(!host->isSender()||(sendnums=getsendto(index).sendnums,!getsendto(index).sendstream)) {
			bool sendstream=true;
			bool sendscans=false;
			bool receive=info->version>1?(info->sendscans||info->sendnums):true;
			char portstr[7];
			snprintf(portstr,6,"%d",port); 
			const int len=host->nr;
			const char *names[len];
			namehost hostnames[len];
			for(int i=0;i<len;i++) {
				hostnames[i]=namehost(host->ips+i);
				names[i]=hostnames[i].data();
				LOGGERTAG("host: %s\n",names[i]);
				}

			auto [_id,lasttime]=sensors->lastpolltime();
			bool activeonly=getactive(index);
			bool passiveonly=getpassive(index);
        		backup->changehost(index,nullptr,(jobjectArray)names,len,true,portstr,sendnums, sendstream, sendscans,false, receive,activeonly ,backup->getpass(index).data(),lasttime,passiveonly,infolabel,false,true);
			}
		}
	else {
		settings->data()->nobluetooth=true;
		if(host->isSender()) {
			char portstr[7];
			snprintf(portstr,6,"%d",port); 
			const int len=host->nr;
			const char *names[len];
			namehost hostnames[len];
			for(int i=0;i<len;i++) {
				hostnames[i]=namehost(host->ips+i);
				names[i]=hostnames[i].data();
				LOGGERTAG("host: %s\n",names[i]);
				}
			bool sendnums=getsendto(index).sendnums;
			bool activeonly=getactive(index);
			bool passiveonly=getpassive(index);
			uint32_t starttime=0; //continues where left if sendnums=true
			bool receive=true;
        		backup->changehost(index,nullptr,(jobjectArray)names,len,true,portstr,sendnums, false, false,false, receive,activeonly ,backup->getpass(index).data(),starttime,passiveonly,infolabel,false,true);


			}
		}
	}
    else { 
    	LOGSTRINGTAG("is no watch\n");
    	if(info->watchsensor) {
		settings->data()->nobluetooth=true;
		getsendto(index).sendstream=false;
		}
	else {
//		getsendto(index).sendstream=true;
		LOGSTRINGTAG("set stream on?\n");
		}
    	}
    networkpresent=true;
    return true;
    }


struct ringnouri {
	uint16_t duration;
	uint16_t wait:14;
	bool nosound:1;
	bool flash:1;
	};
struct sendsettings {
	uint32_t alow,ahigh;
	struct ringnouri alarms[maxalarms];
	int8_t unit;
	bool lowalarm,highalarm,availablealarm;
	bool lossalarm;
	int32_t  alarmnr;
	amountalarm numalarm[maxnumalarms];
	};



extern "C" JNIEXPORT  jbyteArray  JNICALL   fromjava(bytesettings)(JNIEnv *env, jclass cl) {
	const Tings *set=settings->data();
	sendsettings ss;
	int start=offsetof(Tings,alow);
	int len=offsetof(Tings,duration)-start;
	memcpy(&ss,&set->alow,len);
	for(int i=0;i<maxalarms;i++) {
		ss.alarms[i]=*reinterpret_cast<const ringnouri*>(&set->alarms[i].duration);
		}
	ss.unit=set->unit;
	const int allen=offsetof(sendsettings,alarmnr)-offsetof(sendsettings,lowalarm);
	memcpy(&ss.lowalarm,&set->lowalarm,allen);
	ss.alarmnr=set->alarmnr;
	memcpy(ss.numalarm,set->numalarm,sizeof(amountalarm)*ss.alarmnr);
	int totlen=offsetof(sendsettings,numalarm[ss.alarmnr]);
	jbyteArray uit = env->NewByteArray(totlen);
	env->SetByteArrayRegion(uit, 0, totlen, reinterpret_cast<const jbyte *>(&ss));
	LOGGERTAG("bytesettings success unit=%d highalarm=%d\n",ss.unit,ss.highalarm);
	return uit;
	}

extern "C" JNIEXPORT  jboolean  JNICALL   fromjava(ontbytesettings)(JNIEnv *env, jclass cl,jbyteArray  jar) {
	Tings *set=settings->data();
	sendsettings ssbuf;
	const int minlen=offsetof(sendsettings,numalarm);
    	const jsize lens=env->GetArrayLength(jar);
	if(lens<minlen) {
		LOGGERTAG("ontbytesettings %d<%d\n",lens,minlen);
		return false;
		}
        env->GetByteArrayRegion(jar, 0, lens,reinterpret_cast<jbyte *>(&ssbuf));
	const sendsettings &ss=ssbuf;
	LOGGERTAG("ontbytesettings unit=%d highalarm=%d\n",ss.unit,ss.highalarm);
	if(ss.alarmnr<0) {
		LOGGERTAG("alarmnr=%d\n",ss.alarmnr);
		return false;
		}
	const int larmin=(ss.alarmnr*sizeof(amountalarm)+minlen);
	if(lens<larmin) {
		LOGGERTAG("ontbytesettings %d<%d larmnr=%d\n",lens,larmin,ss.alarmnr);
		return false;
		}
	int start=offsetof(Tings,alow);
	int len=offsetof(Tings,duration)-start;
	memcpy(&set->alow,&ss,len);
	for(int i=0;i<maxalarms;i++) {
		*reinterpret_cast<ringnouri*>(&set->alarms[i].duration)=ss.alarms[i];
		}
	settings->setunit(ss.unit);
	const int allen=offsetof(sendsettings,alarmnr)-offsetof(sendsettings,lowalarm);
	memcpy(&set->lowalarm,&ss.lowalarm,allen);
	set->alarmnr=ss.alarmnr;
	memcpy(set->numalarm,ss.numalarm,sizeof(amountalarm)*ss.alarmnr);
	LOGGERTAG("ontbytesettings success unit=%d highalarm=%d\n",set->unit,set->highalarm);
	return true;
	}

int hostindex(const passhost_t *host) {
    struct updatedata *update=backup->getupdatedata();
    return host-update->allhosts;
    }
extern "C" JNIEXPORT jint  JNICALL   fromjava(directsensorwatch)(JNIEnv *env, jclass cl,jstring jident) {

	if(!jident) return -1;

      const char *id = env->GetStringUTFChars( jident, NULL);
        if (id == nullptr) return false;
        destruct   dest([jident,id,env]() {env->ReleaseStringUTFChars(jident, id);});

	if(passhost_t *host=getwearoshost(false,id,true)) {
		int index=hostindex(host);
		uint32_t nu=time(nullptr);
		long last=lastuptodate[index];
		if((nu-last)>3*60)
			return -1;
		return watchsensor(host);
		}
	return -1;
       }

bool getwearindex(JNIEnv *env, jstring jident) {
    if(!jident) return -1;
   const char *id = env->GetStringUTFChars( jident, NULL);
   if (id == nullptr) return -1;
   destruct   dest([jident,id,env]() {env->ReleaseStringUTFChars(jident, id);});
    passhost_t *host=getwearoshost(false,id,true);
    if(!host) return -1;
   int index=host- backup->getupdatedata()->allhosts;
   LOGGERTAG("%s index=%d\n",id,index);
   return index;
   }
extern "C" JNIEXPORT  void  JNICALL   fromjava(isGalaxyWatch)(JNIEnv *env, jclass cl,jboolean val) {
	 isGalaxyWatch=val;  
	}

extern "C" JNIEXPORT  void  JNICALL   fromjava(setWearosdefaults)(JNIEnv *env, jclass cl,jstring jident,jboolean galaxy) {
    if(!jident) {
    	LOGGERTAG("setWearosdefaults(null,%d)\n",galaxy);
    	return;
	}
   const char *id = env->GetStringUTFChars( jident, NULL);
   if (id == nullptr) {
    	LOGGERTAG("setWearosdefaults(null=env->GetStringUTFChars() ,%d)\n",galaxy);
   	return;
	}
   setdefaults(id,galaxy);
   env->ReleaseStringUTFChars(jident, id);
   }
