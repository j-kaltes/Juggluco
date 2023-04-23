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
/*      Fri Jan 27 12:35:35 CET 2023                                                 */


#include "datbackup.h"
#include "nums/numdata.h"
void receivetimeout(int sock,int secs) ;
void sendtimeout(int sock,int secs) ;

extern void stopreceiver() ;
//constexpr const char orsettings[]="orsettings.dat";

extern std::vector<Numdata*> numdatas;
 void uptodate(int ind) {
 	LOGGER("uptodate %d\n",ind);
	lastuptodate[ind]=time(nullptr);
	}

void uptodate(passhost_t *host) {
	if(backup)
		uptodate(host-backup->getupdatedata()->allhosts);
	}
int updateone::update() {
	if(getsock()<0)
		return 0;
	crypt_t *pass=getcrypt();
	bool sendsensors=(sendstream||sendscans) ;
	int ret =0;
	LOGGER("updateone::update starttime=%d\n",starttime);
	if(starttime) {
		if(sendnums) {
			if(nums[0].lastlastpos==0&&nums[1].lastlastpos==0) {
				if(starttime==1)  {
					for(auto el:numdatas)
						if(! el->sendbackupinit(pass,getsock(),nums) )
							return 0;
					}
				else   {
					bool numsbackupsendinit(crypt_t*pass,int sock,struct changednums *nuall,uint32_t starttime) ;
					if(!numsbackupsendinit(pass,getsock(),nums, starttime) )
						return 0;
					}
				}
			}

		if(starttime!=1)  {
			if(sendsensors) {
				if( !sensors->setbackuptime(pass, getsock(),ind,starttime,starttimeindex))  {
					LOGGER("updateone::update failed\n");
					return 0;
					}
				}
			}
		LOGGER("updateone::update set starttime=0\n");
		starttime=0;
		ret=1;
	   }

	int subdid=updatenums();
	if(!subdid)
		return 0;
	ret|=subdid;
	if(sendsensors) {
		subdid=sensors->update(pass,getsock(),ind,startsensors,firstsensor,sendstream,sendscans,restore);
		if(subdid&4) {
			resetdevices=true;
			subdid&=3;
			}
		if(!subdid)
			return 0; 
		ret|=subdid;
		}
	auto update= settings->getupdate();
	static bool init=true;
	if(update>updatesettings||init) {
		init=false;
		if(sendnums) {
			std::vector<subdata> vect;
			vect.reserve(2);
			bool nochangenum =true;
			vect.push_back({reinterpret_cast<const senddata_t *>(&nochangenum),offsetof(Tings,nochangenum),sizeof(nochangenum)});
			constexpr const int  sharedstart=offsetof(Tings, update);
			constexpr const int len=offsetof(Tings,mealvar )+1-sharedstart;
			vect.push_back({reinterpret_cast<const senddata_t *>( settings->data())+sharedstart,sharedstart,len});
			if(!senddata(pass,getsock(),vect,settingsdat) ) 
				return 0;
			}
		ret=1;
		}
	updatesettings=update;

  	uptodate(allindex) ;
	if(!noacksendone(pass,getsock(), suptodate))
		return 0;
	if(resetdevices) {
		if(!askresetdevices(pass,getsock()) ) {
			LOGGER("askresetdevices(%p,%d) failed\n",pass,getsock() );
			return 0;
			}
		ret=1;
		resetdevices=false;
		}
	return ret;
//	return sendrender(getsock());

	}
int 	updateone::updatenums() {
	if(!sendnums)
		return 2;
	int soc=getsock();
	if(soc<0)
		return 0;
	return ::updatenums(getcrypt(),soc,nums);
	}

int  updateone::updatestreamu() {
	if(!sendstream)
		return 2;
	if(getsock()<0)
		return 0;
	return sensors->updatestreams(getcrypt(),getsock(),ind,firstsensor);
 	} 
int updateone::updatescansu() {
	if(!sendscans)
		return 2;
	if(getsock()<0)
		return 0;
	return sensors->updatescanss(getcrypt(),getsock(),ind,firstsensor);
 	} 
bool netwakeup(int sock,passhost_t *pass,crypt_t *ctx){
	if(backup) {
		LOGGER("netwakeup %d\n",sock);
		backup->wakebackup(Backup::wakeall|Backup::wakereconnect);
		}
	return false;
	}
bool netwakeupstream(int sock,passhost_t *pass,crypt_t *ctx){
	if(backup) {
		LOGGER("netwakeupstream %d\n",sock);
		backup->wakebackup(Backup::wakestream|Backup::wakereconnect);
		}
	return false;
	}
bool networkpresent=false;


int hostsocks[maxallhosts];
uint32_t lastuptodate[maxallhosts]={};
//int *sendsocks=nullptr;
//crypt_t **crypts=nullptr;
std::vector<int> sendsocks;
//std::vector<std::vector<int>> tmpsocks;
//int tmpsocks[maxallhosts][maxip];
std::vector<crypt_t *> crypts;
Backup *backup=nullptr;
#define SENDPASSIVE 1
#define RECEIVEFROM 2
/*
bool sendtype(int sock,char type) {
	if(sock!=-1) {
		char ant=type;
		if(sendni(sock,&ant,1)!=1) {
		//	return false;
			
			}
		}
	return true;
	}
	*/
static int saysender(const passhost_t *host) {
	if(host->activereceive) 
		return RECEIVEFROM;
	return 0;
	}
#ifdef WEAROS_MESSAGES
extern bool wearmessages[];
extern int messagemakeconnection(passhost_t *pass,int &sock,crypt_t*ctx,char stype);
#endif
static int sayactivereceive(const passhost_t *host) {
	if(host->index>=0) {
		return SENDPASSIVE;
		}
	return 0;
	}

	crypt_t * updateone::getcrypt() const {
#ifdef WEAROS_MESSAGES
		if(wearmessages[allindex]) {
			if(backup) {
				 const auto &host= backup->getHosts()[allindex];
				if(host.wearos)
					return nullptr;
				}
			}
#endif
		if(crypts.size()>ind&&ind>=0)
			return crypts[ind];
		return nullptr;
		}
void	updateone::open() {
     auto *host=backup->getupdatedata()->allhosts+allindex;
     LOGGER("updateone::open %d %s  receivefrom=%d sendpassive=%d activereceive=%d\n",allindex,host->getnameif(),host->receivefrom,host->sendpassive,host->activereceive);

#ifdef WEAROS_MESSAGES
	if(host->wearos&&wearmessages[allindex]) {
		 messagemakeconnection(host,getsock(),getcrypt(),saysender(host));
		
 	}   else 
#endif
	{
		if(host->sendpassive) 
			return;
		makeconnection(host,getsock(),getcrypt(),saysender(host));

	}
	if(getsock()>=0) {
//		mirrorstatus[allindex].sendor.hassocket=true;
		receivetimeout(getsock(),60);
		sendtimeout(getsock(),60*5);
		}
	}



bool turnreceiver(int sock,passhost_t *hostptr,crypt_t *ctxptr) ;

static void sendup(passhost_t *hostptr) {
	const bool haspas= hostptr->haspass();
	LOGGER("wake sender %spass\n",(haspas?"":"no" ));
	crypt_t ctx;
	crypt_t *ctxptr=haspas?&ctx:nullptr;

        prctl(PR_SET_NAME, "wake sender", 0, 0, 0);
	if(int sock;makeconnection(hostptr,sock,ctxptr,saysender(hostptr))>=0) {
		sendtimeout(sock,60*5);
		receivetimeout(sock,0);
		if(sendbackup(ctxptr,sock)) {

			LOGGER("sendup success %d\n",sock);	
			}
		else
			LOGGER("%d: failure %d\n",agettid(),sock);	

		close(sock);
		}
	}

extern std::vector<Backup::condvar_t*> active_receive;
std::vector<Backup::condvar_t*> active_receive;
#include <chrono>
using namespace std::chrono_literals;
void activereceivethread(int allindex,passhost_t *pass) {
	auto &status=mirrorstatus[allindex].receive;
	status.activereceivethread=true;
	destruct _dest([&status](){ status.activereceivethread=false;});
	const int h = pass->activereceive - 1;
	if(h < 0) {
		LOGGER("activereceivethread h(%d)<0\n", h);
		return;
	}
	if(h >= active_receive.size()) {
		LOGGER("activereceivethread h(%d)>=active_receive.size()(%zd)\n", h, active_receive.size());
		return;
	}
	if(!active_receive[h]) {
		LOGGER("activereceivethread !active_receive[%d]\n",h);
		return;
		}
	const bool haspas = pass->haspass();
	crypt_t ctx, *ctxptr = haspas ? &ctx : nullptr;
	decltype(active_receive[h]->dobackup) current{};
{
	constexpr const int maxbuf = 50;
	char buf[maxbuf];
#ifndef NOLOG
	int slen =
#endif
	snprintf(buf, maxbuf, "Ractive%d", h);
	LOGGERN(buf, slen);
	prctl(PR_SET_NAME, buf, 0, 0, 0);
}
	while(true) {
		  active_receive[h]->dobackup=active_receive[h]->dobackup&(~current);
		  if(!active_receive[h]->dobackup) {
		    std::unique_lock<std::mutex> lck(active_receive[h]->backupmutex);
			LOGGER("R-active before lock\n");
	constexpr const int waitsec=
#if defined(JUGGLUCO_APP) && !defined(WEAROS)
	70
#else
	30
#endif
;
#ifdef WEAROS_MESSAGES
	if(pass->wearos&&wearmessages[allindex]) {
		active_receive[h]->backupcond.wait(lck, [h] {return active_receive[h]->dobackup; });   
		LOGGER("R-active after wait\n");
	    }
   else   
#endif  
		{
			auto status=active_receive[h]->backupcond.wait_for(lck,std::chrono::seconds(waitsec));    //Inreality much longer if phone is in doze mode.
			LOGGER("R-active after lock %stimeout\n",(status==std::cv_status::no_timeout)?"no-":"");
			}
			}
		current=active_receive[h]->dobackup;
		if(current&Backup::wakeend) {
			int sockwas=hostsocks[allindex];
			hostsocks[allindex]=-1;
			close(sockwas);
		       delete active_receive[h];
			active_receive[h]=nullptr;
			LOGGER("end activereceivethread\n");
			return;
			}
		int &sock=hostsocks[allindex];
#ifdef WEAROS_MESSAGES
	if(!pass->wearos||!wearmessages[allindex])  //TODO use it?
#endif  
	{
		if(makeconnection(pass,sock,ctxptr,sayactivereceive(pass))<0) {
			continue;
			}
//		status.hassocket=true;
		void	receiversockopt(int sock) ;
		receiversockopt(sock) ;
		bool	activegetcommands(int sock,passhost_t *host,crypt_t *ctx) ;
		LOGGER("before activegetcommands\n");
		activegetcommands(sock,pass,ctxptr); 
		LOGGER("after activegetcommands\n");
		close(sock);
//		status.hassocket=false;
		sock=-1;
	}
//		if(ctxptr) ascon_aead_cleanup(ctxptr);

		}
	}
bool hasnetwork() {
	return 	backup&&backup->gethostnr()>0;
	}
void updatedata::wakesender() {
    LOGGER("wakesender\n");
    for(int i=0;i<hostnr;i++) {
	passhost_t &host=allhosts[i];
	if(
#ifdef WEAROS_MESSAGES
	!(wearmessages[i]&&host.wearos)&&
#endif
	host.activereceive) {
		auto ind=host.activereceive-1;
		LOGGER("active %d\n",ind);
		if(active_receive[ind])  {
			active_receive[ind]->wakebackup(Backup::wakeall);
			}
		}
	else {
			if(host.receivefrom==3&&host.index<0) {
#ifdef WEAROS_MESSAGES
			if(host.wearos&&wearmessages[i]) { //TODO
			LOGGER("wearos messages\n");
			}  else
#endif
			{	
				std::thread wake(sendup,&host);
				wake.detach();
				}
				}
			else {
				if(host.index>=0&&backup->con_vars[host.index])  {
					LOGGER("con_vars[%d]->wakebackup\n",host.index);
					  backup->con_vars[host.index]->wakebackup(Backup::wakesend);
					  }
					  
				}
		}
	}
	}
void updatedata::wakestreamsender() {
    LOGGER("wakestreamsender\n");
	for(int i=0;i<hostnr;i++) {
		passhost_t &host=allhosts[i];
	if(
#ifdef WEAROS_MESSAGES
	!(wearmessages[i]&&host.wearos)&&
#endif
	host.activereceive) {
			auto ind=host.activereceive-1;
			LOGGER("active %d\n",ind);
			if(active_receive[ind])
				active_receive[ind]->wakebackup(Backup::wakestream);
			}
		else {
			if(host.receivefrom==3&&host.index<0) {
				std::thread wake(sendup,&host);
				wake.detach();
				}
			else {
				if(host.index>=0&&backup->con_vars[host.index]) {
					LOGGER("host.index=%d\n",host.index);
					  backup->con_vars[host.index]->wakebackup(Backup::wakestreamsend);
					  }
					  
				}
			}
		}
	}


void passivesender(int sock,passhost_t *pass)  {
	LOGGER("passivesender %d\n",sock);
	 if(!networkpresent) {
	 	LOGGER("!networkpresent close and return sock=%d\n",sock);
		close(sock);
		return;
		}
	int h=pass->index;
	updateone &host=backup->getupdatedata()->tosend[h];
	LOGGER("passivesender got host\n");
	if(h>=0&&backup->con_vars[h]) {
		int oldsock=host.getsock();
		if(oldsock>=0) {
			LOGGER("passivesender shutdown oldsock %d\n",oldsock);
			host.setsock(-1);
			::shutdown(oldsock,SHUT_RDWR);
			close(oldsock);
			}
		const bool haspas= pass->haspass();
		if(haspas) {
			LOGGER("passivesender  haspas true\n");
			bool	receivepassinit(int ,passhost_t *,crypt_t *);
			if(!receivepassinit(sock,pass,host.getcrypt()))  {
				close(sock);
				return ;
				}
			}
		else
			LOGGER("passivesender  haspas false\n");

		receivetimeout(sock,60) ;
		sendtimeout(sock,60*5);
		host.setsock(sock); 
//		mirrorstatus[host.allindex].sendor.hassocket=true;
		LOGGER("wakebackup con_vars[%d]\n",h);
		 backup->con_vars[h]->wakebackup(Backup::wakeall);
		 }
	}

/*
	int h=pass->index;
	updateone &host=backup->getupdatedata()->tosend[h];
	host.close();
	host.setsock(sock); */
	


bool getpassive(int pos) {
	if(pos<backup->getupdatedata()->hostnr)  {
		const auto &host=backup->getupdatedata()->allhosts[pos];
		if(host.index>=0)
			return host.sendpassive;
		return host.receivefrom==2;
		}
	return false;
	}
bool getactive(int pos) {
	if(pos<backup->getupdatedata()->hostnr)  {
		const auto &host=backup->getupdatedata()->allhosts[pos];
		LOGGER("receivefrom=%d sendpassive=%d activereceive=%d\n",host.receivefrom,host.sendpassive,host.activereceive);
		if(host.index>=0) {
			if(host.sendpassive) 
				return false;
			if(host.activereceive)
				return true;
			return !host.receivefrom;
			}
			
		return host.activereceive;
		}
	return false;
	}
	/*
updateone &getsendto(int index) {
         int tohost=backup->getupdatedata()->allhosts[index].index;
      	return backup->getupdatedata()->tosend[tohost];
	}*/

updateone &getsendto(const passhost_t *host) {
        const int tohost=host->index;
      	return backup->getupdatedata()->tosend[tohost];
	}
updateone &getsendto(int index) {
	return getsendto(backup->getupdatedata()->allhosts + index);
	}

bool sendall(const passhost_t *host) {
 const updateone &sender=getsendto(host);
  return (sender.sendnums&&sender.sendstream&&sender.sendscans);
  }

mirrorstatus_t mirrorstatus[maxallhosts];
