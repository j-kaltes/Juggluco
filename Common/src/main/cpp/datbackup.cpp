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


#include "datbackup.hpp"
#include "nums/numdata.hpp"
#include "net/Connect.hpp"
void receivetimeout(int sock,int secs) ;
void sendtimeout(int sock,int secs) ;
extern bool sendResetDevices(crypt_t *pass,const int sock) ;

extern void stopreceiver() ;
//constexpr const char orsettings[]="orsettings.dat";

extern std::vector<Numdata*> numdatas;
 void uptodate(int ind) {
     LOGGER("uptodate %d\n",ind);
    lastuptodate[ind]=time(nullptr);
    #ifdef WEAROS
    bool rmInitLayout();
        static bool did=rmInitLayout();
    #endif
    }

void uptodate(passhost_t *host) {
    if(backup)
        uptodate(host-backup->getupdatedata()->allhosts);
    }
int updateone::updateiob() {
    const auto iobupdate=settings->data()->iobupdate;
    if(iobupdate>iobupdated ) {
          crypt_t *pass=getcrypt();
           std::vector<subdata> vect;
           vect.reserve(1);
           const auto startinsulin=offsetof(Tings,insulintypes);
           const auto endinsulin=offsetof(Tings,iobupdate)+sizeof(Tings::iobupdate);
           LOGGER("updateiob start=%zd end=%zd\n",startinsulin,endinsulin);
           vect.push_back({reinterpret_cast<const senddata_t *>(settings->data()->insulintypes),startinsulin,endinsulin-startinsulin});
           if(!getConnect()->senddata(pass,vect,settingsdat) )
                    return 0;
            iobupdated=iobupdate;
            return 1;
            }
     return 2;
    }


int updateone::sendCalibrate() {
    return sensors->sendCalibrates(getcrypt(), getConnect(),ind,startSendCalibrate);
    }


#ifndef WEAROS
int updateone::numbertypes() {
    if(!sendNight&&!sendLibre) {
        return 2;
        }
    const auto &host= backup->getHosts()[allindex]; 
    if(!host.wearos)  {
        sendLibre=false;
        sendNight=false;
        return 2;
        }
  std::vector<subdata> vect;
  vect.reserve(2);
  if(sendNight) {
       const auto startnight=offsetof(Tings,Nightnums);
       const auto endnight=offsetof(Tings,nightinterval);
       LOGGER("Nightnums start=%zd end=%zd\n",startnight,endnight);
       vect.push_back({reinterpret_cast<const senddata_t *>(settings->data()->Nightnums),startnight,endnight-startnight});
       }
  if(sendLibre) {
       const auto startnight=offsetof(Tings,librenums);
       const auto endnight=offsetof(Tings,libreaccountIDnum);
       LOGGER("librenums start=%zd end=%zd\n",startnight,endnight);
       vect.push_back({reinterpret_cast<const senddata_t *>(settings->data()->librenums),startnight,endnight-startnight});
       }
   if(!getConnect()->senddata(getcrypt(),vect,settingsdat) )
        return 0;
    sendLibre=false;
    sendNight=false;
    return 1;
  }
#endif

int updateone::update() {
    Connect *connect=getConnect();    
    if(!connect->isConnectedSender())
        return 0;
    crypt_t *pass=getcrypt();

    bool sendsensors=(sendstream||sendscans) ;
    int ret =0;
    LOGGER("updateone::update starttime=%d\n",starttime);
#ifdef WEAROS
    if(blueWatch) {
        if(!connect->sendBlueWatch(pass,sendstream,sendnums)) 
            return 0;
         backup->getupdatedata()->allhosts[allindex].receivefrom=3;
         blueWatch=false;
        }
#endif
    if(starttime) {
        if(sendnums) {
            if(nums[0].lastlastpos==0&&nums[1].lastlastpos==0) {
                LOGAR("sendnums==true 2*lastlastpos==0");
                if(starttime==1)  {
                    for(auto el:numdatas)
                        if(! el->sendbackupinit(pass,connect,nums) )
                            return 0;
                    }
                else   {
                    bool numsbackupsendinit(crypt_t*pass,Connect *,struct changednums *nuall,uint32_t starttime) ;
                    if(!numsbackupsendinit(pass,connect,nums, starttime) )
                        return 0;
                    startSendCalibrate=sensors->firstafter(starttime);
                    const int last=sensors->last();
                    for(int it=startSendCalibrate;it<=last;++it) {
                        SensorGlucoseData *sens=sensors->getSensorData(it);
                        sens->getinfo()->updateCaliTime(ind,starttime);
                        }
                    }
                }
             else {
                LOGGER("sendnums==true nums[0].lastlastpos==%d nums[1].lastlastpos==%d\n", nums[0].lastlastpos,nums[1].lastlastpos);
                }
            }
        else {
            LOGAR("sendnums==false");
            }

        if(starttime!=1)  {
            if(sendsensors) {
                if( !sensors->setbackuptime(pass, connect,ind,starttime,starttimeindex))  {
                    LOGAR("updateone::update failed");
                    return 0;
                    }
                }
            else {
                LOGAR("sendsensor=false");
                }
            }
        LOGAR("updateone::update set starttime=0");
        starttime=0;
        ret=1;
       }

    int subdid=updatenums();
    if(!subdid)
        return 0;
    ret|=subdid;
    if(sendsensors) {
        subdid=sensors->update(pass,connect,ind,startsensors,firstsensor,sendstream,sendscans,restore,resetdevices);
        if(subdid&4) {
            resetdevices=true;
            subdid&=3;
            }
        if(!subdid)
            return 0; 
        ret|=subdid;
        }
    const auto update= settings->getupdate();
    static bool init=true;
    if(update>updatesettings||init) {
        init=false;
        if(sendnums) {
            std::vector<subdata> vect;
            vect.reserve(3);
            bool nochangenum =true;
            vect.push_back({reinterpret_cast<const senddata_t *>(&nochangenum),offsetof(Tings,nochangenum),sizeof(nochangenum)});
            constexpr const int  bloodvaroff=offsetof(Tings, bloodvar);
            vect.push_back({reinterpret_cast<const senddata_t *>( settings->data())+bloodvaroff,bloodvaroff,1});
            constexpr const int  sharedstart=offsetof(Tings, update);
            constexpr const int len=offsetof(Tings,mealvar )+1-sharedstart;
            vect.push_back({reinterpret_cast<const senddata_t *>( settings->data())+sharedstart,sharedstart,len});
            if(!connect->senddata(pass,vect,settingsdat) ) 
                return 0;
            }
        ret=1;
        }
    updatesettings=update;
    if(int iobret=updateiob()) {
        ret|=iobret;
        }
    else
        return 0;
#ifndef WEAROS
    if(int numret=numbertypes())
        ret|=numret;
    else
        return 0;
#endif
    uptodate(allindex) ;
    if(!connect->noacksendone(pass, suptodate))
        return 0;
    if(resetdevices) {
        if(!connect->sendResetDevices(pass) ) {
            LOGGER("sendResetDevices(%p,%d) failed\n",pass,connect->getSenderIdent() );
            return 0;
            }
        ret=1;
        resetdevices=false;
        }
    return ret;
    }
    
extern int  updatenums(crypt_t *,Connect *,struct changednums *nums,int);

int     updateone::updatenums() {
    if(!sendnums)
        return 2;
    if(starttime) {
        return update();
        }
    Connect *connect=getConnect();
    if(!connect->isConnectedSender())
        return 0;
    if(!sendjugglucoid) {
        LOGAR("updatenums sendjugglucoid");
        const int offset=offsetof(Tings,jugglucoID);
        const auto *data=reinterpret_cast<const senddata_t*>(&settings->data()->jugglucoID);
        const int len=sizeof(Tings::jugglucoID);
        if(!connect->senddata(getcrypt(),offset,data,len,settingsdat) )  {
            LOGAR("updatenums sendjugglucoid error");
            return 0;
            }
        sendjugglucoid=true;
        }
    if(int did= ::updatenums(getcrypt(),connect,nums,ind))  {
        return sendCalibrate()|did;
        }
    return 0; 
    }

int  updateone::updatestreamu() {
    if(!sendstream)
        return 2;
    if(starttime) {
        return update();
        }
    Connect *connect=getConnect();
    if(!connect->isConnectedSender())
        return 0;
    return sensors->updatestreams(getcrypt(),connect,ind,firstsensor,sendscans?2:1);
     } 
int updateone::updatescansu() {
    if(!sendscans)
        return 2;
    if(starttime) {
        return update();
        }
    Connect *connect=getConnect();
    if(!connect->isConnectedSender())
        return 0;
    return sensors->updatescanss(getcrypt(),connect,ind,firstsensor,sendstream);
     } 
void wakeupall(){
    if(backup) {
        LOGAR("wakeupall");
        backup->wakebackup(wakeall|wakereconnect);
        }
    }
void wakeupstream(){
    if(backup) {
        LOGAR("wakeupstream");
        backup->wakebackup(wakestream|wakereconnect);
        }
    }
bool networkpresent=false;


//int hostsocks[maxallhosts]{-1,-1,-1,-1,-1,-1,-1,-1};
uint32_t lastuptodate[maxallhosts]={};
//std::vector<int> sendsocks;
std::vector<crypt_t *> crypts;
Backup *backup=nullptr;
#define SENDPASSIVE 1
#define RECEIVEFROM 2
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
#if defined(WEAROS_MESSAGES)&&!defined(ENCRYPTMESSAGES)
        if(wearmessages[allindex]) {
            if(backup) {
                 const auto &host= backup->getHosts()[allindex]; //WHY???
                if(host.wearos)  {
                    LOGGER("getcrypt allindex=%d wearos\n",allindex);
                    return nullptr; //WHY
                    }
                }
            }
#endif 
        if(crypts.size()>ind&&ind>=0)
            return crypts[ind];
        return nullptr;
        }
void    updateone::open() {
     auto *host=backup->getupdatedata()->allhosts+allindex;
     LOGGER("updateone::open %d %s  receivefrom=%d sendpassive=%d activereceive=%d\n",allindex,host->getnameif(),host->receivefrom,host->sendpassive,host->activereceive);
     if(host->deactivated) {
         return;
          }

#ifdef WEAROS_MESSAGES
    if(host->wearos&&wearmessages[allindex]) {
         messagemakeconnection(host,getsock(),getcrypt(),saysender(host));
        
     }   else 
#endif
    {
        if(host->sendpassive) 
            return;
       auto *con=connections[allindex];
       if(con)
           con->makeconnection(host,getcrypt(),saysender(host));

    }
   auto *con=connections[allindex];
   if(con)
        con->setSenderTimeouts();
    LOGGER("updateone::open()=%d\n",connections[allindex]->getSenderIdent());
    }




static void sendup(passhost_t *hostptr) {
    const bool haspas= hostptr->haspass();
    LOGGER("wake sender %spass\n",(haspas?"":"no" ));
    crypt_t ctx;
    crypt_t *ctxptr=haspas?&ctx:nullptr;

#ifndef HAVE_NOPRCTL
        prctl(PR_SET_NAME, "wake sender", 0, 0, 0);
#endif
     int allindex=hostptr-backup->getupdatedata()->allhosts;

     Connect *connect=connections[allindex];
    if(connect&&connect->makeconnection(hostptr,ctxptr,saysender(hostptr))>=0) {
        if(!hostptr->ICE) {
            int sock=static_cast<TCPConnect*>(connect)->getSenderSock();
            sendtimeout(sock,60*5);
            receivetimeout(sock,0);
            }
        if(connect->sendbackup(ctxptr)) {
            connect=connections[allindex];
            LOGGER("sendup success %d\n",connect?connect->getSenderIdent():-1);    
            }
        else {
            connect=connections[allindex];
            LOGGER("%d: failure %d\n",agettid(),connect?connect->getSenderIdent():-1);    
            }
        if(!hostptr->ICE&&(connect=connections[allindex]))
            connect->closeSenderConnection();
///        sockclose(sock);
        }
    }

condvar_t* active_receive[maxallhosts];
int active_receivenr=0;

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
    if(h >= active_receivenr) {
        LOGGER("activereceivethread h(%d)>=active_receivenr(%zd)\n", h, active_receivenr);
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
    snprintf(buf, maxbuf, "Ractive%d_%d", allindex,h);
    LOGGERN(buf, slen);
#ifndef HAVE_NOPRCTL
    prctl(PR_SET_NAME, buf, 0, 0, 0);
#endif
}
    while(true) {
          active_receive[h]->dobackup=active_receive[h]->dobackup&(~current);
          if(!active_receive[h]->dobackup) {
            std::unique_lock<std::mutex> lck(active_receive[h]->backupmutex);
            LOGAR("R-active before lock");
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
        LOGAR("R-active after wait");
        }
   else   
#endif  
        {
           LOGGER("activereceivethread before wait_for %d %p\n",h ,active_receive[h]);

#ifndef NOLOG
            auto status=
#endif
                active_receive[h]->backupcond.wait_for(lck,std::chrono::seconds(waitsec));    //In reality much longer if phone is in doze mode.
            LOGGER("R-active after lock %stimeout\n",(status==std::cv_status::no_timeout)?"no-":"");
            }
            }
       LOGGER("before if(!active_receive[%d]) %p \n",h,active_receive[h]);
       if(h>=active_receivenr||!active_receive[h]) {
            LOGGER("active_receive[%d]==0, return\n",h);
            return;
          }
       current=active_receive[h]->dobackup;
       LOGAR("after current=active_receive[h]->dobackup;");
       TCPConnect *con=static_cast<TCPConnect*>(connections[allindex]);
       LOGAR("after TCPConnect *con=static_cast<TCPConnect*>(connections[allindex]);");
        if(!con||current&wakeend) {
            if(con) {
                int recsock=con->getReceiverSock(); 
                con->closeReceiverConnection();
                LOGGER("end activereceivethread %d close(%d)\n",h,recsock);
                }
            else
                LOGGER("end activereceivethread %d \n",h);
            LOGAR("before delete");
            delete active_receive[h];
            LOGAR("after delete");
            active_receive[h]=nullptr;
            LOGAR("return");
            return;
            }
        
        int &sock=con->getSenderSock();;
        int wassock=sock;
        sock=-1;
#ifdef WEAROS_MESSAGES
    if(!pass->wearos||!wearmessages[allindex])  //TODO use it?
#endif  
    {
        auto *con=static_cast<TCPConnect*>(connections[allindex]);
        if(!con) {
            return;
            }
        if(con->makeconnection(pass,ctxptr,sayactivereceive(pass))<0) {
            continue;
            }
//        status.hassocket=true;
     
        con->setReceiverSock(sock);;
        sock=wassock;
        void    receiversockopt(int sock) ;
        receiversockopt(sock) ;
        LOGAR("before activegetcommands");
        auto *con2=connections[allindex];
        if(!con2) {
            return;
            }
        con2->activegetcommands(pass,ctxptr);
        if(connections[allindex]) {
            LOGGER("after activegetcommands close(%d)\n",sock);
            sockclose(sock);
            }
         else {
            LOGAR("after activegetcommands");
            }
        }

        }
    }
bool hasnetwork() {
    return     backup&&backup->gethostnr()>0;
    }
void updatedata::wakesender() {
    LOGAR("wakesender");
    for(int i=0;i<hostnr;i++) {
        passhost_t &host=allhosts[i];
        if(host.deactivated) {
            LOGGER("%d deactivated\n", i);
            }
        else {
        if(
    #ifdef WEAROS_MESSAGES
        !(wearmessages[i]&&host.wearos)&&
    #endif
        host.activereceive) {
            auto ind=host.activereceive-1;
            LOGGER("active %d\n",ind);
            if(active_receive[ind])  {
                active_receive[ind]->wakebackup(wakeall);
                }
            }
        else {
                if(host.receivefrom==3&&host.index<0) {
    #ifdef WEAROS_MESSAGES
                if(host.wearos&&wearmessages[i]) { //TODO
                LOGAR("wearos messages");
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
                          backup->con_vars[host.index]->wakebackup(wakesend);
                          }
                          
                    }
            }
            }
       }
    }
void updatedata::wakestreamsender() {
    LOGAR("wakestreamsender");
    for(int i=0;i<hostnr;i++) {
        passhost_t &host=allhosts[i];
        if(host.deactivated) {
        LOGGER("deactivated %d\n",i);
        }
    else {
    if(
#ifdef WEAROS_MESSAGES
    !(wearmessages[i]&&host.wearos)&&
#endif
    host.activereceive) {
            auto ind=host.activereceive-1;
            LOGGER("active %d\n",ind);
            if(active_receive[ind])
                active_receive[ind]->wakebackup(wakestream);
            }
        else {
            if(host.receivefrom==3&&host.index<0) {
                std::thread wake(sendup,&host);
                wake.detach();
                }
            else {
                if(host.index>=0&&backup->con_vars[host.index]) {
                    LOGGER("host.index=%d\n",host.index);
                      backup->con_vars[host.index]->wakebackup(wakestreamsend);
                      }
                      
                }
            }
        }
        }
    }


void TCPConnect::passivesender(passhost_t *pass,int &recsock,int oldrecsock)  {
    LOGGER("passivesender %d\n",getReceiverIdent());
     if(!networkpresent) {
        LOGGER("!networkpresent close and return sock=%d\n",getSenderIdent());
        closeSenderConnection();
        closeReceiverConnection();
        return;
        }
    int h=pass->index;
    updateone &host=backup->getupdatedata()->tosend[h];
    LOGAR("passivesender got host");
    if(h>=0&&backup->con_vars[h]) {
        const bool haspas= pass->haspass();
        if(haspas) {
            LOGAR("passivesender  haspas true");
            if(!receivepassinit(pass,host.getcrypt()))  {
                LOGGER("close(%d)\n",getReceiverIdent());
                closeReceiverConnection();
                closeSenderConnection();
                return ;
                }
            }
        else
            LOGAR("passivesender  haspas false");
         
        setSenderSock(recsock);
        recsock=oldrecsock;
        setSenderTimeouts();
//        host.setsock(sock); 
        LOGGER("wakebackup con_vars[%d]\n",h);
         backup->con_vars[h]->wakebackup(wakeall);
         }
     else {
         LOGGER("passivesender h>=0&&backup->con_vars[h] failed h=%d\n",h);
       }
    }

    


bool getpassive(int pos) {
    if(pos<backup->getupdatedata()->hostnr)  {
        const auto &host=backup->getupdatedata()->allhosts[pos];
        return host.getPassive();
        }
    return false;
    }
bool getactive(int pos) {
    if(pos<backup->getupdatedata()->hostnr)  {
        const auto &host=backup->getupdatedata()->allhosts[pos];
        LOGGER("receivefrom=%d sendpassive=%d activereceive=%d\n",host.receivefrom,host.sendpassive,host.activereceive);
        return host.getActive();
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
extern void resethost(passhost_t &host) ;
void resethost(passhost_t &host) {
    backup->resethost(host);
    }

#include <mutex>
std::mutex change_host_mutex;
void resensordata(int sensorindex) {
    backup->resensordata(sensorindex);    
    }

int getgetsendnr() {
    if(backup)
       return backup->getupdatedata()->sendnr;
    return 0;
    }
void wakesender() {
     backup->getupdatedata()->wakesender();    
     }
#include "mirrorerror.h"
char mirrorerrors[maxallhosts][maxmirrortext];
int getindex(const  passhost_t *host) {
    return host-backup->getupdatedata()->allhosts;
    }
char *getmirrorerror(const passhost_t *pass) {
    int index=getindex(pass);
    return mirrorerrors[index];
    }
int savemessage(const passhost_t *pass,const char* fmt, ...){
    va_list args;
    va_start(args, fmt);
    char *buf=getmirrorerror(pass);
    int len=vsnprintf(buf,maxmirrortext, fmt, args);
    va_end(args);
    return len;
    }
    /*
void saveerror(const passhost_t *pass,const char* fmt, ...){
    int waser=errno;
        va_list args;
        va_start(args, fmt);
    char *buf=getmirrorerror(pass);
    int len=vsnprintf(buf,maxmirrortext, fmt, args);
    va_end(args);
    strcpy(buf+len,": ");
    len+=2;
    if(len<maxmirrortext)
        strerror_r(waser, buf+len, maxmirrortext-len);
    } */

void savebuferror(char *buf,int maxbuf,const char* fmt, ...){
    int waser=errno;
    va_list args;
    va_start(args, fmt);
    int len=vsnprintf(buf,maxbuf, fmt, args);
    va_end(args);
    strcpy(buf+len,": ");
    len+=2;
    if(len<maxbuf)
        strerror_r(waser, buf+len, maxbuf-len);
    }


void    sendstartsensors(int startpos) {
    LOGGER("sendstartsensors(%d)\n",startpos);
    const int maxint=backup->getupdatedata()->sendnr;
    for(int i=0;i<maxint;i++) {
        auto &host=backup->getupdatedata()->tosend[i];
        LOGGER("%i %d\n",i,host.startsensors);
        if(host.startsensors>startpos)
            host.startsensors=startpos;
        }
    }


extern void setCalibrates(uint16_t sensorindex) ;

void setCalibrates(uint16_t sensorindex) {
    LOGGER("setCalibrates(%hd)\n",sensorindex);
    const int maxint=getgetsendnr();
    for(int i=0;i<maxint;++i) {
        auto &host=backup->getupdatedata()->tosend[i];
        if(host.sendnums)
            host.setCalibrate(sensorindex);
        }
    }

Connect *connections[maxallhosts];


#include "net/ICE/ICEConnect.hpp"
Backup::Backup(std::string_view base): mapdata(base,backupdat,sizeof(struct updatedata)), con_vars((resetindices(),getupdatedata()->sendnr)) {
   const int len=getupdatedata()->hostnr;
   auto *allhosts=getupdatedata()->allhosts;
   for(int i=0;i<len;++i) {
     auto *host= allhosts+i;
      if(host->ICE)
          connections[i]=new ICEConnect(i,host->side);
      else
          connections[i]=new TCPConnect(i);
      }
  // sendsocks.reserve(getupdatedata()->sendnr);
   crypts.reserve(getupdatedata()->sendnr);
   for(int i=0;i<getupdatedata()->sendnr;i++) {
       //sendsocks.push_back(-1);
       auto &host=getupdatedata()->tosend[i];
       if(settings->data()->initVersion<31) { 
          LOGGER("%d set sendjugglucoid=false\n",i);
          host.sendjugglucoid=false;
#ifndef WEAROS
          if(getupdatedata()->allhosts[host.allindex].wearos) {
              if(settings->data()->sendnumbers)
                    host.sendLibre=true;
              if(settings->data()->saytreatments ||settings->data()->postTreatments)
                    host.sendNight=true;
            }
#endif
          }
       if(getupdatedata()->allhosts[host.allindex].haspass()) {
           auto cry=new crypt_t();
           LOGGER("crypts[%d]=%p=new crypt_t()\n",i,cry);
           crypts.push_back(cry);
           }
       else  {
           LOGGER("crypts[%d]=nullptr\n",i);
           crypts.push_back(nullptr);
           }
       }



   startactivereceivers();
   if(!getupdatedata()->port[0])
       strcpy(getupdatedata()->port,defaultport );

   void    backupbase(string_view basedir);
   backupbase(globalbasedir);
   for(int i=0;i<len;i++) {
       if(getupdatedata()->allhosts[i].passive()) {
           startreceiver(false);
           break;
           }
       }


   shouldaskfordata=getshouldaskfordata();
   }

int Backup::changeICEhost(const char *ICElabel,int index,const bool sendnums,const bool sendstream,const bool sendscans,const bool receive,string_view pass,uint32_t starttime,const char *label,bool side,bool startthreads) {
    const int hostnr=getupdatedata()->hostnr;
    LOGGER("hostnr=%d changeICEhost(%d,sendnums=%d,sendstream=%d,sendscans=%d,receive=%d,label=%s \n",hostnr,index,sendnums,sendstream,sendscans,receive,label);
    if(index<0) 
        index=hostnr;
    if(index>=maxallhosts)  {
        LOGAR("changeICEhost: index>=maxallhosts");
        return -3;
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
    int tohost;
    bool newthread=false;
    auto &thehost=getupdatedata()->allhosts[index];
    LOGGER("changeICEhost newhost=%d thehost.index=%d\n",newhost,thehost.index);
    thehost.sendpassive=false;
    thehost.hostname=false;
    if(sendto) {
        if(newhost||thehost.index==-1) {  //Fout??
            tohost=getupdatedata()->sendnr;
            if(tohost>=maxsendtohost) {
                LOGGER("changeICEhost: tohost(%d)>=maxsendtohost(%d)\n",tohost,maxsendtohost);
                return -4;
                }
            thehost.index=tohost;
            newthread=true;
            }
        else  {
            tohost=thehost.index;
            }

        changereceiver(index,tohost,sendnums,sendstream,sendscans,false,pass.size(),starttime);
        }
    else {
        tohost=0;
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
    thehost.side=side;
    if(!newhost) {  
        if(thehost.ICE) {
            goto keepICE;
            }
        delete connections[index];
        }
    thehost.ICE=true;
    connections[index]=new ICEConnect(index,side);
    keepICE:
    thehost.setname(label);
    thehost.setICEname(ICElabel);
    thehost.noip=true;
    lastuptodate[index]=0;
    thehost.receivefrom=receive?3:1;
    LOGGER("changeICEhost receivefrom=%d\n", thehost.receivefrom);
    setpass( thehost.pass,pass);
    thehost.deactivated=false;
    thehost.wearos=false;
    if(newhost)  {
        ++(getupdatedata()->hostnr);
        LOGGER("new host %s ++hostnr=%d\n",thehost.getnameif(),getupdatedata()->hostnr);
        thehost.newconnection=true;
        }

    deupdated(); 
    closesocksone(index,getupdatedata()->allhosts+index);
    if(startthreads) {
        if(newthread)
            startthread(index,tohost);
        }
    thehost.activereceive=0;
    shouldaskfordata=getshouldaskfordata();
    #ifdef WEAROS_MESSAGES
    extern    void clearnetworkcache();
    clearnetworkcache();
    #endif
    LOGGER("changeICEhost=%d\n",index);
    return index;
    }
