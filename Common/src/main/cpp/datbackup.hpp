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
#ifndef HAVE_NOPRCTL
#include <sys/prctl.h>
#endif
#include <signal.h>
#include <unistd.h>
//#include <sys/syscall.h>
#include <setjmp.h>
#include <sys/socket.h>
#include <stdlib.h>

#include <jni.h>
#include "net/TCPConnect.hpp"
#include "myfdsan.h"

//#include "logvector.hpp"
inline int mytag() {
   return 0;
   }

extern void    sendstartsensors(int startpos);
extern void  startReceiverThread(int i);
extern void setBlueMessage(int ident,bool val);
constexpr const char defaultport[]{
#ifdef MIRRORPORT
xquotes(MIRRORPORT)
#else
"9996"
#endif

};


#include "inout.hpp"
#include "net/backup.hpp"
#include "destruct.hpp"
#include "net/passhost.hpp"
#include "net/netstuff.hpp"
#include "sensoren.hpp"

#include "settings/settings.hpp"

#include "mirrorstatus.hpp"
#include "mirrorerror.h"
#ifdef WEAROS_MESSAGES
extern bool wearmessages[];
#endif
#ifndef TESTMENU
#include <mutex>
extern std::mutex change_host_mutex;
#endif

extern void setConnectTime(const int allindex,uint32_t tim);
extern Connect *connections[];
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
int  updatenums(crypt_t *,Connect *connect,struct changednums *nums,bool=true);
inline static constexpr const char backupdat[]="backup.dat";
inline static constexpr const char orbackup[] ="orbackup.dat";
//extern int hostsocks[];
//extern std::vector<int> sendsocks;
extern uint32_t lastuptodate[];

//extern int tmpsocks[maxallhosts][passhost_t::maxip];
extern std::vector<crypt_t *>crypts;
struct updateone {
    int ind;
    int allindex;
    int startsensors;//vanwaaraf sensors.dat updaten

    uint8_t backupupdated;
    uint8_t countSync:4;
    uint8_t reserved1:4;
    uint16_t iobupdated;
    uint32_t updatesettings;

    int32_t firstsensor;
    uint32_t starttime;
    uint16_t starttimeindex;
    bool resetdevices;
    bool RESERVED;
    bool sendnums;
    bool sendstream;
    bool sendscans;
    bool restore:4;

    bool blueWatch:1;

    bool sendNight:1;
    bool sendLibre:1;
    bool sendjugglucoid:1;
    bool sendnotes;
    uint16_t startSendCalibrate;
    uint8_t NotUsed[13];
    struct changednums nums[2];
    void setCalibrate(uint16_t sensorindex) {
        if(sensorindex<startSendCalibrate)
                startSendCalibrate=sensorindex;
                
        }
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
            LOGGER("setcrypt ind (%d) <=crypts.size() (%d) delete %p\n",ind,crypts.size(),ctx);
            delete ctx;
            }
        }
    
    crypt_t *getcrypt() const; 

    template <typename Self>
auto *getConnect(this Self&&self)  {
        return connections[self.allindex];
        }
    int &getsock() const {
        auto *con=((TCPConnect *)getConnect());
        if(!con) {
                static int minone=-1;
                return minone;
                }
//        return sendsocks[ind];
        return con->getSenderSock();
        }
    void setsock(int val) const {
        if(auto *con=((TCPConnect *)getConnect())) {
                con->setSenderSock(val);
          } 
      //  sendsocks[ind]=val;
        }
void    open() ;
void close() {    
    auto *con =connections[allindex];
    if(con) {
        con->closeSenderConnection();
//       const int so= getsock();
       LOGGER("updateone close(%d)\n",con->getSenderIdent());
       }
    /*   if(so>=0) {
         setsock(-1);
         ::sockclose(so);
        } */
     if(crypt_t *ctx=getcrypt()) {
        ascon_aead_cleanup(ctx);
        }
    }

//void     setbackupstarttime(const uint32_t starttime);
int updatenums() ;
int updatestreamu() ;
int updatescansu() ;
int update(); 
int updateiob();
int numbertypes();
int sendCalibrate();
int updateBeforeSwitch() ;
    };

#include "maxsendtohost.h"
struct TurnServer {
    uint16_t port;
    char hostname[254];
    char username[256];
    char password[256];
    };
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
static constexpr const uintptr_t wakeUpSwitch=1024;
struct updatedata {
    int32_t hostnr;
    uint32_t receive;
    //uint32_t update;
    char port[6];
    bool hasrestore;
    uint8_t sendnr;
    passhost_t allhosts[maxallhosts];
    updateone tosend[maxsendtohost];
    uint32_t lastused[maxallhosts];
    int32_t NRturnserver;
    TurnServer  turnserver[4];
    void wakesender(uintptr_t kind=wakeall) ;
    void wakestreamsender();
    };

struct  condvar_t {
    uintptr_t dobackup=0;
    std::mutex backupmutex;
    std::condition_variable backupcond; 

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

extern condvar_t* active_receive[maxallhosts];
extern int active_receivenr;
class  Backup {

Mmap<unsigned char> mapdata;
public:

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
#ifndef WEAROS
void sendLibreNumbers() {
    const int len=getupdatedata()->hostnr;
    for(int i=0;i<len;i++) {
        passhost_t *ph= getupdatedata()->allhosts+i;
        if(ph->wearos) {
            int index=ph->index;
            if(index>=0) {
                getupdatedata()->tosend[index].sendLibre=true;
                }
            }
        }
    }
void sendNightNumbers() {
    const int len=getupdatedata()->hostnr;
    for(int i=0;i<len;i++) {
        passhost_t *ph= getupdatedata()->allhosts+i;
        if(ph->wearos) {
            int index=ph->index;
            if(index>=0) {
                getupdatedata()->tosend[index].sendNight=true;
                }
            }
        }
    }

#endif
void startactivereceivers() {
    const int len=getupdatedata()->hostnr;
    for(int i=0;i<len;i++) {
/*        if(backup) {
            if(auto *con=connections[i])
                con->closeReceiverConnection();
            } */
        passhost_t *ph= getupdatedata()->allhosts+i;
        if(ph->activereceive&&!ph->deactivated) {
            setactivereceive(i,ph); 
            }
        }
    }

void resetindices() {
    LOGGER("resetindices hostnr=%d sendnr=%d\n",getupdatedata()->hostnr, getupdatedata()->sendnr);
    int maxsend=-1;
    for(int i=0;i< getupdatedata()->hostnr;i++) {
        const int si=getupdatedata()->allhosts[i].index;
        if(si>-1)  {
            getupdatedata()->tosend[si].setindex(si,i);
            if(si>maxsend)
                maxsend=si;
            }
        }
    getupdatedata()->sendnr=maxsend+1;
    LOGGER("sendnr=%d\n", getupdatedata()->sendnr);
    }

Backup(std::string_view base);

void resensordata(int sensor)  {
     if(sensor<0) {
         LOGGER("resensordata(%d)\n",sensor);
         return;
         }

    sendstartsensors(sensor);
    for(int i=0;i<getupdatedata()->sendnr;i++) {
        auto &host=getupdatedata()->tosend[i];
        if(host.firstsensor>sensor)
            host.firstsensor=sensor;
        }
  };


void resendResetDevices(bool updateone::*datatype=&updateone::sendscans)  {
    const int nr=getupdatedata()->sendnr;
    LOGGER("resendResetDevices sendnr=%d\n",nr);
    for(int i=0;i<nr;i++) {
        auto &host=getupdatedata()->tosend[i];
        if(host.*datatype) {
            LOGGER("%d resetdevices=true\n",i);
            host.resetdevices=true;
            }
        }
  }

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
    ::startreceiver(getupdatedata()->port,getupdatedata()->allhosts,getupdatedata()->hostnr);
    }
void stopreceiver() {
    ::stopreceiver() ;
    for(int i=0;i<getupdatedata()->hostnr;i++) {
        if(getupdatedata()->allhosts[i].receivefrom) {
            if(auto *con=connections[i])
                con->shutdownReceiver();
            /*
            int sock=hostsocks[i];
            if(sock>=0) {
                LOGGER("shutdown %d\n",sock);
                ::shutdown(sock,SHUT_RDWR);
                }
                */
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
    const auto port=host.getport();
    if(port==0&&!host.getActive())
        strcpy(buf,defaultport);
    else
        snprintf(buf,6, "%d",port);
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
//                           const int sock= getupdatedata()->tosend[sin].getsock();
                           LOGGER("call wakestop %p\n",con_vars[sin]);
                           con_vars[sin]->wakebackuponly(wakestop);
                            if(auto *con=connections[i]) {
                               LOGGER(" shutdown %d\n",con->getSenderIdent());
                               con->shutdownSender();
                               }
//                           ::shutdown(sock,SHUT_RDWR);
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
        if(con_vars[getupdatedata()->sendnr])
           con_vars[getupdatedata()->sendnr]->wakebackuponly(wakestop|wakeend);
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
    for(int i=start;i< getupdatedata()->hostnr;i++) {
        const int si=getupdatedata()->allhosts[i].index;
        if(si>-1)  {
            getupdatedata()->tosend[si].setindex(si,i);
            }
        }
    }

void deletehost(int index) {
    if(index>=getupdatedata()->hostnr)
        return;
    bool wasnet=networkpresent;
     networkpresent=false;
    LOGGER("deletehost(%d)\n",index);
    for(int it=index;it<getupdatedata()->hostnr;++it) {
         auto *con=connections[it];
         if(con) {
            con->finish=true;
            con->endConnection();
            }
        }
    auto &thehost=getupdatedata()->allhosts[index];
    if(thehost.activereceive) {
        endactivereceive(index); 
        }
    sleep(1);
    const int sendindex=getupdatedata()->allhosts[index].index;
    if(sendindex>=0) {
        deletestart(sendindex);
        }
    if(--getupdatedata()->hostnr!=index)  {
        LOGGER("--hostnr %d\n",getupdatedata()->hostnr);
        int fromend= getupdatedata()->hostnr-index;
        memmove( getupdatedata()->allhosts+index,getupdatedata()->allhosts+index+1,fromend*sizeof(getupdatedata()->allhosts[0]));
        memmove( getupdatedata()->lastused+index,getupdatedata()->lastused+index+1,fromend*sizeof(getupdatedata()->lastused[0]));
//        memmove(  hostsocks+index, hostsocks+index+1,fromend*sizeof(hostsocks[0]));
        memmove(  lastuptodate+index, lastuptodate+index+1,fromend*sizeof(lastuptodate[0]));
        memmove(  connections+index, connections+index+1,fromend*sizeof(connections[0]));
        }

    if(sendindex>=0)      {
        deupdated();
        deleteend(sendindex);
        }

    if(getupdatedata()->hostnr==0) {
        getupdatedata()->sendnr=0;
        stopreceiver();
        }
    delete connections[getupdatedata()->hostnr];
    connections[getupdatedata()->hostnr]=nullptr;
    setindices(index);
    for(int i=index;i<getupdatedata()->hostnr;++i) {
        connections[i]->finish=false;
        connections[i]->setindex(i);
        }
     if(!networkpresent)
        networkpresent=wasnet;
    }
void clearhost(int index) {
    LOGGER("clearhost(%d)\n",index);
    updateone &host=getupdatedata()->tosend[index];
    /*
    if(int so=host.getsock();so!=-1) {
        LOGGER("shutdown(%d)\n",so);
        ::shutdown(so,SHUT_RDWR);
        } */
    if(auto *con=connections[host.allindex])
        con->restartSender();
    sensors->updateinit(index);

    host.startsensors=0;
    host.firstsensor=0;
    host.updatesettings=0;
    host.startSendCalibrate=0;
    host.backupupdated=0;
    host.iobupdated=0;
    host.starttime=0;
    host.starttimeindex=UINT16_MAX;
    host.resetdevices=false;
    host.sendjugglucoid=false;

    host.blueWatch=false;
    host.sendNight=false;
    host.sendLibre=false;

    host.nums[0].clear();
    host.nums[1].clear();
    LOGGER("end clearhost nums[0].lastlastpos==%d nums[1].lastlastpos==%d\n", host.nums[0].lastlastpos,host.nums[1].lastlastpos);
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
//    fill(crypts.begin(),crypts.end(),nullptr);
    }
void changereceiver(int allindex,int index,const bool sendnums,const bool sendstream,const bool sendscans,const bool restore,const bool haspass,const uint32_t starttime,const bool sendnotes=true) {
    LOGGER("changereceiver(allindex=%d,index=%d,sendnums=%d,sendstream=%d,sendscans=%d,haspass=%d,starttime=%u,sendnotes=%d) sendnr=%d\n",allindex,index,sendnums,sendstream,sendscans,haspass,starttime,sendnotes,getupdatedata()->sendnr);
    if(index==getupdatedata()->sendnr) {
        addsize();
      //  sendsocks.resize(getupdatedata()->sendnr+1,-1);
//        tmpsocks.resize(getupdatedata()->sendnr+1);
        crypts.resize(getupdatedata()->sendnr+1,nullptr);
        clearhost(index);
        getupdatedata()->sendnr++;
        con_vars.resize( getupdatedata()->sendnr);
        }
    else  {
        if(con_vars[index])
            con_vars[index]->wakebackuponly(wakestop);
        if(auto *con=connections[allindex])
                con->shutdownSender();
        }
    updateone &host=getupdatedata()->tosend[index];
    host.setindex(index,allindex);
    if(!starttime)
        host.starttime=1;
    else  {
        host.starttime=starttime;
        host.starttimeindex=UINT16_MAX;
        }
    host.sendnums=sendnums;
    host.sendstream=sendstream;
    host.sendscans=sendscans;
    host.sendnotes=sendnotes;
    host.restore=restore;
/*
    if(int so=host.getsock();so!=-1) {
        LOGGER("shutdown(%d)\n",so);
        ::shutdown(so,SHUT_RDWR);
    //    host.setsock(-1);
        } */
    crypt_t *oldcrypt=host.getcrypt();
    if(haspass)  {
        if(!oldcrypt)  {
            auto cry=new crypt_t;
            LOGGER("crypts[%d]=%p=new crypt\n",index,cry);
            host.setcrypt(cry);
            }
        }
    else  {
        if(oldcrypt)
            delete oldcrypt;
        host.setcrypt(nullptr);
        LOGGER("crypts[%d]=nullptr\n",index);

        }
    }


void setcrypt(passhost_t *host) {
    const int ind=host->index;
    if(crypts.size()>ind&&ind>=0) {
         if(!crypts[ind])  {
            auto cry=new crypt_t;
            LOGGER("crypts[%d]=%p=new crypt\n",ind,cry);
            crypts[ind]=cry;
            }
         }
    }


static constexpr const std::array<uint8_t,16> remix= {0x19,0xED,0xA0,0x4A,0x94,0x9D,0x0C,0xD7,0x82,0x4A,0x74,0xA9,0x0E,0x71,0x84,0x8B};
//,0x87,0x7F,0x0F,0xD7,0xA8,0xEE,0x8C,0xD7,0x80
static void    setpass( std::array<uint8_t,16> &passuit,const string_view passin) {
    int len=passin.size();
    if(!len) {
        passuit={};
        return;
        }    
    if(len>16)
        len=16;
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
    LOGGER("jsetips len=%d\n",len);

    int uselen=std::min(lmaxip,len);
    for(int i=0;i<uselen;i++) {
        jstring  jname=(jstring)env->GetObjectArrayElement(jar,i);
        int namelen= env->GetStringUTFLength( jname);
        char name[namelen+1];

        jint jnamelen = env->GetStringLength( jname);
        env->GetStringUTFRegion( jname, 0,jnamelen, name); 
        name[namelen]='\0';
        int start=0;
        for(;start<namelen&&name[start]==' ';++start) {
            }
        if(start>=namelen) {
            LOGGER("no ip '%s'\n",name);
            return -1;
            }
        if(start) {
            memmove(name,name+start,namelen+1-start);
            namelen-=start;
            }
        for(int last=namelen-1;last>=0&&name[last]==' ';--last) {
            name[last]='\0';
            }
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
    LOGGER("endactivereceive(%d)\n",allindex);
    passhost_t *ph=getupdatedata()->allhosts+allindex;
    if(ph->activereceive) {
        if(!ph->deactivated) {
            int pos=ph->activereceive-1;
            if(auto actrec=active_receive[pos]) {
                LOGGER("wakeend %d\n",pos);
                actrec->wakebackup(wakeend);
                }
            }
        if(ph->activereceive==active_receivenr)
            --active_receivenr;
        
        ph->activereceive=0;
        if(Connect *con=connections[allindex]) {
            LOGGER("endactivereceive(%d) shutdown(%d)\n",allindex,con->getReceiverIdent());

            con->shutdownReceiver();
            }
//        ::shutdown(hostsocks[allindex],SHUT_RDWR);
        }
    }
void setactivereceive(int allindex,passhost_t *ph,bool startthread=true) { 
       if(active_receivenr<maxallhosts) {
           active_receive[active_receivenr++]=new condvar_t;
           ph->activereceive=active_receivenr;

           LOGGER("setactivereceive allindex=%d nr=%d %p\n",allindex,ph->activereceive,active_receive[ph->activereceive-1]);

           if(startthread) {
               void activereceivethread(int allindex,passhost_t *pass) ;
               std::thread the(activereceivethread,allindex,ph);
               the.detach();
               }
            }
        else {
            LOGGER("setactivereceive active_receivenr too large %d\n", active_receivenr);
            kill(getpid(),15);
            }
       }


int changeICEhost(const char *ICElabel,int index,const bool sendnums,const bool sendstream,const bool sendscans,const bool receive,string_view pass,uint32_t starttime,const char *label,bool side,bool startthreads=true,const bool sendnotes=true);
int changehost(int index,JNIEnv *env,jobjectArray jnames,int nr,bool detect,string_view port,const bool sendnums,const bool sendstream,const bool sendscans,const bool restore,const bool receive,const bool activeonly,string_view pass,uint32_t starttime,bool passiveonly,const char *label=nullptr,const bool testip=true,bool startthreads=true,bool hashostname=false,const bool sendnotes=true) {
    const int hostnr=getupdatedata()->hostnr;
    LOGGER("hostnr=%d changehost(%d,sendnums=%d,sendstream=%d,sendscans=%d,receive=%d,activeonly=%d,passiveonly=%d,label=%s port=%s nr=%d hashostname=%d,sendnotes=%d)\n",hostnr,index,sendnums,sendstream,sendscans,receive,activeonly,passiveonly,label,port.data(),nr,hashostname,sendnotes);
    if(index<0) 
        index=hostnr;
    if(index>=maxallhosts)  {
        LOGAR("changehost: index>=maxallhosts");
        return -3;
        }
    const bool receiveactive=receive&&activeonly;
    if(port.data()==nullptr||port.size()==0) {
        port={defaultport,sizeof(defaultport)-1};
        }
    else {
        if(port.size()>5) {
            LOGAR("changehost: port.size()>5)");
            return -1;
            }
        }
    int portint=atoi(port.data());
    if(!passiveonly&&(portint>65535||portint<1024)) {
        LOGGER("port out of range %d\n",portint);
        return -1;
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
    int tohost;
    bool newthread=false;
    const bool dontopen=sendto&&passiveonly;
    int lmaxip=passhost_t::maxip-(label?1:0);
    auto &thehost=getupdatedata()->allhosts[index];
    LOGGER("changehost newhost=%d thehost.index=%d\n",newhost,thehost.index);
    if(sendto) {
        if(newhost||thehost.index==-1) {  //Fout??
            tohost=getupdatedata()->sendnr;
            if(tohost>=maxsendtohost) {
                LOGGER("changehost: tohost(%d)>=maxsendtohost(%d)\n",tohost,maxsendtohost);
                return -4;
                }
            thehost.index=tohost;
            newthread=true;
            }
        else  {
            tohost=thehost.index;
            }

        changereceiver(index,tohost,sendnums,sendstream,sendscans,restore,pass.size(),starttime,sendnotes);
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
    if(!newhost) {  
        if(auto *con=connections[index]) {
            if(!thehost.ICE) {
                con->setindex( index);
                goto keepTCP;
                }
            con->finish=true;
            con->endConnection();
            sleep(1);
            delete con;
            }
        }

    connections[index]=new TCPConnect(index);
    thehost.ICE=false;
    keepTCP:
    if(!newhost&&thehost.activereceive)
        endactivereceive(index) ;
    int res;
    if(hashostname) {
        thehost.hostname=true;
        if(env) {
            jstring  jhostname=(jstring)env->GetObjectArrayElement(jnames,0);
            if(!jhostname) {
                              LOGAR("no hostname");
                              if(newhost&&sendto) {
                                          --getupdatedata()->sendnr;
                              }
                return -8;
                }
            int namelen= env->GetStringUTFLength( jhostname);

            if(namelen>=passhost_t::hostnamedata::maxhostname) { //terminanting zero
                LOGGER("supplied name too long %d>%d\n",namelen,passhost_t::hostnamedata::maxhostname);
                if(newhost&&sendto) {
                 --getupdatedata()->sendnr;
                   }
                return -5;
                }

            jint jnamelen = env->GetStringLength( jhostname);

            char *hostname=thehost.gethostname();

            env->GetStringUTFRegion( jhostname, 0,jnamelen, hostname); 
            hostname[namelen]='\0';
            }
        else {
            LOGAR("What to do with hashostname with env=null?");
            if(newhost&&sendto) {
                  --getupdatedata()->sendnr;
               }
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
               LOGGER("changehost res(%d)<0",res);
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
        if(!hashostname&&res==0&&!(passiveonly&&!testip))   {
            LOGAR("res==0&&!(passiveonly&&!testip))");
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
Receive           reconnect    receivefrom:
true              true           3
true              false          2
false             true           1
false             false          0
*/
    thehost.receivefrom=receive?(reconnect?3:2):((sendto&reconnect)?1:0);
    LOGGER("changehost receivefrom=%d\n", thehost.receivefrom);
    setpass( thehost.pass,pass);
    thehost.deactivated=false;

    if(newhost)  {
        ++(getupdatedata()->hostnr);
        thehost.wearos=false;
        LOGGER("new host %s ++hostnr=%d\n",thehost.getnameif(),getupdatedata()->hostnr);
        thehost.newconnection=true;
        }
    else {
        LOGGER("wearos(%d)=%d\n", index,thehost.wearos);
        }

    setConnectTime(index,0);
    deupdated(); 

    closesocksone(index);
    if(startthreads) {
        if(newthread)
            startthread(index,tohost);
        if(!activeonly)
            startreceiver(false);
        }
    LOGGER("activereceive was=%d nu=%d\n",thehost.activereceive,receiveactive);
    if(receiveactive) {
        if(newhost||!thehost.activereceive)
            setactivereceive(index,getupdatedata()->allhosts+index,startthreads);
        }
    else  {
        if(thehost.activereceive) {
            if(thehost.activereceive==active_receivenr)
                --active_receivenr;
              thehost.activereceive=0;
            }
        }
        
    shouldaskfordata=getshouldaskfordata();
    #ifdef WEAROS_MESSAGES
    extern    void clearnetworkcache();
    clearnetworkcache();
    #endif
    LOGGER("changehost=%d\n",ret);
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
    }

void deactivateHost(int index,bool deactive) {
    LOGGER("deactivateHost(%d,%d)\n",index,deactive);
    if(index>=getupdatedata()->hostnr) 
        return ;
    auto &host=getupdatedata()->allhosts[index];

    if(host.deactivated==deactive)
        return;
#ifdef JUGGLUCO_APP
    if(host.wearos)
        setBlueMessage(index,false);
#endif
    host.deactivated=deactive;
    Connect *con=connections[index];
    if(deactive) {
        if(con) {
                con->finish=true;
                }
        if(host.activereceive) {
            active_receive[host.activereceive-1]->wakebackup(wakeend);
            if(con) {
                LOGGER("stop active receive     shutdown(%d)\n",con->getReceiverIdent());
                con->shutdownReceiver();
                }
            }
        int sin=host.index;
        if(sin>=0) {
               if(con_vars[sin])
                    con_vars[sin]->wakebackuponly(wakestop|wakeend);
               if(con) {
                   LOGGER(" shutdown %d\n",con->getSenderIdent());
                   con->shutdownSender();
                   }
            }
        if(con)
            con->endConnection();
        }
    else {
        if(con) {
                con->finish=false;
                }
      setConnectTime(index,0);
        if(host.index>=0)
            startthread(index,host.index);
        if(host.activereceive) {
            setactivereceive(index,&host);
               }
       if(host.ICE)
            startReceiverThread(index);
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
void closeallsocks() {
    LOGAR("closeallsocks");
    for(int i=0;i<getupdatedata()->hostnr;i++) {
        if(auto *con=connections[i]) {
             LOGGER("closeall receiver %d shutdown(%d)\n",i,con->getReceiverIdent());
            con->closeReceiverConnection();
            LOGGER("closeall sender %d shutdown(%d)\n",i,con->getSenderIdent());
            con->closeSenderConnection();
            }
//        ::shutdown(hostsocks[i],SHUT_RDWR);
        }
        /*
    for(int i=0;i<getupdatedata()->sendnr;i++) {
        if(sendsocks[i]>=0) {
            LOGGER("send %d shutdown(%d)\n",i,sendsocks[i]);
            ::shutdown(sendsocks[i],SHUT_RDWR);
            }
        else
           LOGGER("sendsock[%d]==-1\n",i);
        } */
    }
void endAllConnections() {
    LOGAR("endAllConnections");
    for(int i=0;i<getupdatedata()->hostnr;i++) {
        if(auto *con=connections[i]) {
            con->endConnection();
            }
        }
     }
void closesocksone(int allindex) {
    LOGGER("closesocksone %d\n",allindex);
    if(Connect *connect=connections[allindex]) {
        connect->shutdownReceiver();
        connect->shutdownSender();
        }
    /*
    int sock=hostsocks[allindex];
    if(sock>=0) {
        hostsocks[allindex]=-1;
        ::shutdown(sock,SHUT_RDWR);
        } 
     int ind=host->index;
     if(ind>=0) {
        int ssock=sendsocks[ind];
        if(ssock>=0) {
            sendsocks[ind]=-1;
            LOGGER("send %d shutdown(%d)\n",ind,ssock);
            ::shutdown(ssock,SHUT_RDWR);
            }
        else
           LOGGER("sendsock[%d]==-1\n",ind);
        }
        */
    }
bool sendwakesender(int h) {
    LOGGER("sendwakesender(%d)\n",h);
    updateone &shost=getupdatedata()->tosend[h];
    shost.close();
    shost.open();
    if(auto *con=shost.getConnect()) {
        if(con->finish) {
            LOGGER("sendwakesenser: %d finish\n",h);
            return false;
            }
        if(con->allindex!=shost.allindex) {
                LOGGER("sendwakesender %d allindex different\n",h);
                return false;
                }
        return con->sendbackup(shost.getcrypt());
        }
    LOGAR("sendwakesender shost.getConnect()==null");
     return false;
    }

bool sendwakestreamsender(int h) {
    LOGGER("sendwakestreamsender(%d)\n",h);
    updateone &shost=getupdatedata()->tosend[h];
    shost.close();
    shost.open();
    if(auto *con=shost.getConnect()) {
        if(con->finish) {
            LOGGER("sendwakestreamsender: %d finish\n",h);
            return false;
            }
        if(con->allindex!=shost.allindex) {
                LOGGER("sendwakestreamsender %d allindex different\n",h);
                return false;
                }
        return con->sendwakeupstream(shost.getcrypt());
        }
    LOGAR("sendwakestreamsender shost.getConnect()==null");
     return false;
    }


void backupthread(int allindex,int sendindex) {
    auto &host=getupdatedata()->tosend[sendindex];
#ifndef NOLOG
   auto *con=connections[allindex];
    LOGGER("%d backupthread, wearos=%d con_vars=%p sock=%i %p\n", allindex,getupdatedata()->allhosts[allindex].wearos, con_vars[sendindex],con?con->getSenderIdent():-1,host.getcrypt());
#endif
//    const int sendindex=getupdatedata()->allhosts[allindex].index;
    const bool passive=getupdatedata()->allhosts[allindex].sendpassive;

    auto &status=mirrorstatus[allindex].sender;
    status.start();
    destruct _dest([&status]{ status.stop();});
    {
    constexpr int maxbuf=15;
    char buf[maxbuf];
    snprintf(buf,maxbuf,"send %d",sendindex);
    LOGGER("%s\n",buf);
#ifndef HAVE_NOPRCTL
    prctl(PR_SET_NAME, buf, 0, 0, 0);
#endif
       }
    uintptr_t current=0;
       while(true) {
          if(doend(sendindex)) 
            return;
        if(!con_vars[sendindex]) { 
                LOGGER("con_vars[%d]==null\n",sendindex);
                return;
                }
         if(!con_vars[sendindex]->dobackup) {
            status.locked=true;
            lockwait(current,sendindex);
            status.locked=false;
            }
          if(doend(sendindex))
            return;
         current=con_vars[sendindex]->dobackup;
         con_vars[sendindex]->dobackup=0;
         if(!passive) {
            notpassive(current,sendindex);
            }
         if(current==wakestream) {
            if(host.countSync++%5==4) {
               current=wakeall;
               }
            }
            doupdates(current,sendindex); 
        }
    }

void        notpassive(uintptr_t current,int sendindex) {
        if(current&wakereconnect)  {
            LOGGER("notpassive(%ul,%d) wakereconnect\n",current,sendindex);
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
    const condvar_t* var=con_vars[sendindex];
      if(!var||(var->dobackup&wakeend))  {
         LOGGER("doend: con_vars=%p end\n",var);
        endbackupthread(sendindex);
        return true;
        }
    LOGGER("doend: con_vars=%p continue\n",var);
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
        //    if(!pass->sendpassive)
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
                LOGAR("updateproce wakestop");
                shost.close();
                return 0;
                }
            res= (shost.*proc)();
            if(!res) {
                savemessage(pass,"Send failed2");
                LOGAR("updateproce !res");
                shost.close();
                }
             else {
                 *getmirrorerror(pass)='\0';
                }
            }
        return res;
           }
      else
          return 2;
    }


  void    endbackupthread(int h) {
    const int sendnr=getupdatedata()->sendnr;
    LOGGER("%d:sendnr=%d end backupthread  %p \n",h, sendnr, con_vars[h]);
#ifndef TESTMENU
      const std::lock_guard<std::mutex> lock(change_host_mutex);
#endif

    if(h<sendnr) {
        getupdatedata()->tosend[h].close();
        }
    else {
        LOGGER("endbackupthread h(%d)>sendnr(%d)\n",h,sendnr);
        }
    if(h<con_vars.size()) {
        delete con_vars[h];
        con_vars[h]=nullptr;
        }
    else {
        LOGGER("endbackupthread h(%d)>con_vars.size()(%d)\n",h,con_vars.size());
        }
    LOGAR("after delete con_vars[h]");
    }

void        doupdates(const uintptr_t current,const int h) { 
        LOGGER("doupdates(%x,%d)\n",current,h);
        int didnums=0,didstream=0,didscans=0,didupdate=0,didUpSwitch=0;
        (didnums=updateproc(con_vars[h],current&wakenums,getupdatedata()->tosend[h],&updateone::updatenums))&&
            (didstream=    updateproc(con_vars[h],current&wakestream,getupdatedata()->tosend[h],&updateone::updatestreamu))&&
            (didscans=        updateproc(con_vars[h],current&wakescan,getupdatedata()->tosend[h],&updateone::updatescansu))&&
            (didUpSwitch=        updateproc(con_vars[h],current&wakeUpSwitch,getupdatedata()->tosend[h],&updateone::updateBeforeSwitch))&&
            (didupdate=        updateproc(con_vars[h],current&wakeall,getupdatedata()->tosend[h],&updateone::update));
        const uint16_t command=((didnums&1)?wakenums:0)|(didstream&1?wakestream:0)|(didscans&1?wakescan:0)|(didupdate&1?wakeall:0)|
(didUpSwitch&1?wakeUpSwitch:0);
        if(command) {
            auto &host=getupdatedata()->tosend[h];
            host.getConnect()->sendrender(host.getcrypt(),command);
            }
        }
void lockwait(uintptr_t &current,int h) {
    LOGGER("%d before lock\n",h)    ;
    std::unique_lock<std::mutex> lck(con_vars[h]->backupmutex);
    LOGGER("%d after lock\n",h)    ;
    con_vars[h]->dobackup=con_vars[h]->dobackup&~current;
    LOGGER("%d dobackup=%lu\n",h,con_vars[h]->dobackup)    ;
    con_vars[h]->backupcond.wait(lck, [h] {return backup->con_vars[h]->dobackup; });   
    LOGGER("%d afterwait\n",h)    ;
    current=con_vars[h]->dobackup;
    #ifndef NOLOG
    LOGGER("%d after current=%lu\n",h,current)    ;
    int allindex=getupdatedata()->tosend[h].allindex;
    auto *con=connections[allindex];
   LOGGER("%d after connections[%d]=%p eSenderIdent=%d\n",h,allindex,con,con?con->getSenderIdent():-1)    ;
   #endif
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
    LOGAR(" end wakebackuponly");
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
                    LOGAR("networkabsent wearos->wake");
                    doe=true;
                    }
                else {
                    LOGAR("networkabsent !wearos");
                    doe=false;
                    }
                }
            if(doe) {
                LOGGER("host %d wake\n",i);
                el->wakebackup(kind);
                }
            }
        }
    LOGAR(" end wakebackup");
  }
  static void startthread(int allindex,int sendindex) {
    LOGGER("in startthread %d %d\n",allindex,sendindex);
    backup->con_vars[sendindex]=new condvar_t;
    std::thread back(&Backup::backupthread,backup,allindex,sendindex);
    back.detach();
    }
static void startbackup(std::string_view globalbasedir) {
    LOGAR("startbackup");
    backup=new(std::nothrow) Backup(globalbasedir);
    if(backup) {
        const int maxsend=backup->getupdatedata()->sendnr;
        const int hostnr= backup->getupdatedata()->hostnr;
        for(int i=0;i<hostnr;i++) {
            auto &host=backup->getupdatedata()->allhosts[i];
            int index=host.index;
            LOGGER("index=%d\n",index);
            if(host.ICE&&!host.deactivated) {
                startReceiverThread(i);
                }
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
                    LOGAR(" no start");
                    }
                }
            }

extern void showbackup() ;
      showbackup();
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

inline std::span<passhost_t> getBackupHosts() {
    return backup-> getHosts();
    }
inline int gethostindex(const passhost_t *host) {
    return host-getBackupHosts().data();
    }

//extern void wakebackup(int kind=1);


extern void startbackup(std::string_view globalbasedir) ;



