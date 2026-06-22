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

#include <string.h>
#include <algorithm>
#ifdef WEAROS_MESSAGES
#include <zlib.h>
#endif
#include "net/netstuff.hpp"
#include "share/fromjava.h"
#include "datbackup.hpp"
#ifdef WEAROS
#define iswatchapp() 1
#else
#define iswatchapp() 0
#endif
#include <array>
std::array<int,maxallhosts>        peers2us,us2peers;
//void setall(std::array<int<maxallhosts>&ar,const int ini) {    
//      s/LOGSTRINGTAG("\([^"]*\)\\n")/LOGARTAG("\1")
#define LOGGERTAG(...) LOGGER("netinfo: " __VA_ARGS__)
#define LOGARTAG(...) LOGAR("netinfo: " __VA_ARGS__)
#define LOGSTRINGTAG(...) LOGSTRING("netinfo: " __VA_ARGS__)


extern uint32_t sendstreamfrom() ;
extern void setBlueMessage(int,bool val);
extern bool getpassive(int pos) ;
extern bool getactive(int pos) ;
extern bool getownip(struct sockaddr_in6 *outip);
static constexpr const uint8_t thisversion=2;
static uint8_t usedversion=thisversion;
static bool connectionbusy[maxallhosts]{};
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
struct netinfo2:netinfo1 {
    bool setpass;
    std::array<uint8_t,16>  pass;
    };

int isGalaxyWatch=-1;
extern updateone &getsendto(int index);
extern updateone &getsendto(const passhost_t *host);
// bool mkwearos=false;
#include <mutex>
extern std::mutex change_host_mutex;
//static bool remakewearhost=false;
static bool newlycreated=false;
int makeversion=0;
passhost_t * getwearoshost(const bool create,const char *label,bool galaxy,bool remake=false) {
  const std::lock_guard<std::mutex> lock(change_host_mutex);
    struct updatedata *update=backup->getupdatedata();
     int nrhost=update->hostnr;
    LOGGER("getwearoshost(create=%d,%s,galaxy=%d, remake=%d) usedversion=%d nrhost=%d \n",create,label,galaxy,remake,usedversion,nrhost);
    passhost_t *hosts=update->allhosts;
    passhost_t *endhosts=update->allhosts+nrhost;
    passhost_t *found= std::find_if(hosts,endhosts,[label](const passhost_t &host){
        const bool same=host.hasname&&!strncmp(label,host.getname(),passhost_t::maxnamelen);
         LOGGERTAG("%s %s %s)\n",host.getname(),same?"=":"!=",label);
         return same;
        });
      bool newhost;
    if(found==endhosts) {
        if(!create) {
            LOGARTAG("!create");
            return nullptr;
        }
        if(nrhost==maxallhosts) {
            LOGGERTAG("nrhost==maxallhosts==%d\n",nrhost);
            --nrhost;
            }
       newhost=true;
      }
    else {
      if(newlycreated&&usedversion!=makeversion) {
            makeversion=usedversion;
            if(usedversion==4) {
                if(found->getActive()||found->getPassive()) {
                    remake=true;
                    }
                 }
            else {
                if(usedversion==3) {
                    if(!(found->getActive()||found->getPassive())) {
                        remake=true;
                        }
                    }
                }
            }
        if(!remake)
            return found;
        nrhost=found-hosts;
       newhost=false;
        }

   newlycreated=true;

    bool sendstream, sendscans, receive,sendnums,activeonly=false,passiveonly=false;

    const bool onedirection=!usedversion||usedversion==3;
    if constexpr( iswatchapp()) {
        LOGARTAG("watch app");
      if(isGalaxyWatch<0) {
         LOGARTAG("isGalaxyWatch<0");
         return nullptr;
         }
        sendstream=false;
        sendscans=false;
        sendnums=false;
        receive=true;

        if(isGalaxyWatch) {  
            LOGARTAG("I am Galaxy Watch");
            if(onedirection) {
                activeonly=true;
                }
            //passiveonly=false;
            }
        else {
            LOGARTAG("I am No Galaxy Watch");
            if(onedirection) {
             //   activeonly=false;
                passiveonly=true;
                }
            }
        }
    else {
        LOGARTAG("no watch app");
        sendstream=true;
        sendscans=true;
        sendnums=true;
        receive=false;
        if(galaxy) {
            LOGARTAG("connected to galaxy");
            //activeonly=false;
            if(onedirection) {
                passiveonly=true;
                }
            }
        else {
            LOGARTAG("not connected to galaxy");
            if(onedirection) {
                activeonly=true;
                }

            //passiveonly=false;
            }
        }
        
        int ret=backup->changehost(nrhost,nullptr,nullptr,0,false,defaultport,sendnums, sendstream, sendscans,false, receive,activeonly ,newhost?std::string_view(nullptr,0):backup->getpass(nrhost).data(),0,passiveonly,label,false,true);
    if(ret<0&&ret!=-2) { 
        LOGARTAG("changehost<0");
        return nullptr;
        }

    found=backup->getupdatedata()->allhosts+nrhost; //extend?
    found->wearos=true;
    LOGGERTAG("getwearoshost new(%d)\n",nrhost);
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
        //auto [_id,lasttime]=sensors->lastpolltime();

        auto lasttime=sendstreamfrom();
        bool activeonly=false;
        bool passiveonly=false;
        bool sendnums;
        bool sendstream;
        bool sendscans;
        bool receive;
       const bool onedirection=!usedversion||usedversion==3;
        if constexpr(iswatchapp()) {
            LOGARTAG("is watch");
            sendnums=false;
            sendstream=false;
            sendscans=false;
            receive=true;
            if(isGalaxyWatch) {  
                LOGARTAG("I am Galaxy Watch");
                if(onedirection) {
                    activeonly=true;
                    }
                //passiveonly=false;
                }
            else {
                LOGARTAG("I am No Galaxy Watch");
                //activeonly=false;
                if(onedirection) {
                    passiveonly=true;
                    }
                }
            }
        else  {
            LOGARTAG("no watch app");
            sendstream=true;
            sendscans=true;
            sendnums=true;
            receive=false;
            if(galaxy) {
                LOGARTAG("connected to galaxy");
                //activeonly=false;
                if(onedirection) {
                    passiveonly=true;
                    }
                }
            else {
                LOGARTAG("not connected to galaxy");
                if(onedirection) {
                    activeonly=true;
                    }
                //passiveonly=false;
                }
            }

        backup->changehost(index,nullptr,(jobjectArray)names,ipsnr,true,portstr,sendnums, sendstream, sendscans,false, receive,activeonly ,string_view(nullptr,0),lasttime,passiveonly,infolabel,false,true);
        }
    }


updateone &getsendto(const passhost_t *host);
static bool hasDirectWatchConnection(const passhost_t *wearhost) {
    if(!wearhost)
        return false;
    if constexpr(iswatchapp()) {
           LOGGERTAG("is watch isSender=%d\n",wearhost->isSender());
           if(wearhost->isSender()&&getsendto(wearhost).sendstream)  {
                LOGARTAG("watch sender");
                return true;
                }
            else  {
                LOGARTAG("watch no sender");
                return false;
                }
           }
    else {
        LOGGERTAG("is no watch isSender=%d\n",wearhost->isSender());
        if(wearhost->isSender()&&getsendto(wearhost).sendstream) {
            LOGARTAG("watch no sender");
            return false;
            }
        else  {
            LOGARTAG("watch sender");
            return true;
            }
        }
    }
static bool hasWatchNums(const passhost_t *wearhost) {
    if(!wearhost)
        return false;
       if constexpr(iswatchapp()) {
            LOGGERTAG("is watch isSensor=%d\n",wearhost->isSender());
            if(wearhost->isSender()&&getsendto(wearhost).sendnums)  {
                  LOGARTAG("watch nums sender");
            return true;
            }
        else  {
              LOGARTAG("watch no nums sender");
            return false;
            }
               }
    else {
        LOGGERTAG("is no watch isSender=%d\n",wearhost->isSender());
        if(wearhost->isSender()&&getsendto(wearhost).sendnums) {
              LOGARTAG("watch no nums sender");
            return false;
            }
        else  {
                  LOGARTAG("watch nums sender");
                  return true;
            }
        }
    }
/*watchsensor
-1: not
1: yes
0: don't change
*/

template<typename OUTTYPE> int getownips(OUTTYPE *outips,int max,bool &) ;


static int getreceivefrom(int index,bool receive,bool activeonly,bool passiveonly) {
    bool sendto;
    if(index<0) {
        sendto=false;
        }
    else {
        updateone &updat= getsendto(index);
        sendto=updat.sendnums||updat.sendstream||updat.sendscans;
        }
    const bool reconnect=(receive&&!passiveonly)||(sendto&&!activeonly);
    int res=receive?(reconnect?3:2):((sendto&&reconnect)?1:0);
    LOGGER("passiveonly=%d activeonly=%d reconnect=%d getreceivefrom(%d,%d)=%d\n",passiveonly,activeonly,reconnect,index,receive,res);
    return res;
    }
static void        setsendinfo(struct netinfo1 &info,passhost_t *wearhost) {
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


extern void makepass(char *pass,int len);
static bool sendpass=false;


static int getmynetinfo(const char *id,jboolean create,jint watchHasSensor,jboolean galaxy,jint setnums,struct netinfo2 &info) {
    if(!backup) {
        LOGARTAG("getmynetinfo backup=null");
        return 0;
        }

    auto myport=atoi(backup->getmyport());
    LOGGERTAG("getmynetinfo(%s,%d,%d,%d) port=%d\n", id,create,watchHasSensor,galaxy,myport);
    passhost_t *wearhost=getwearoshost(create,id,galaxy);
    if(!wearhost)  {
        LOGARTAG("wearhost==null");
        return 0;
        }
    struct updatedata *update=backup->getupdatedata();
    int index=wearhost-update->allhosts;
   connectionbusy[index]=true;
   destruct _{[index]{ connectionbusy[index]=false;}};
    info.index=index;
    info.setpass=false;;
    if(usedversion) {
        bool haswlan;
        info.nr=getownips(info.ips,passhost_t::maxip-1,haswlan);

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
            LOGARTAG("crc different");
        const bool setmess=!haswlan;
        #ifndef WEAROS
            if(setmess)
        #endif
                setBlueMessage(index,setmess);
        crcs[index]=newcrc;
        }
    else  {
            LOGARTAG("crc the same");
        }
    info.blue=wearmessages[index];
#endif
        }
    else  {
        if(!getownip(&info.ip)) {
            LOGARTAG("!getownip");
            return 0;
            }
        info.ip.sin6_port= htons(myport);
        }
    if constexpr(!iswatchapp()) {
        if(usedversion>=4&&sendpass) {
            LOGARTAG("sendpass");
            memcpy(info.pass.data(),wearhost->pass.data(),info.pass.size());
            info.setpass=true;
            sendpass=false;
           }
        if(usedversion&&setnums) {
            if(wearhost->index>=0) {
                backup->con_vars[wearhost->index]->wakebackuponly(wakestop);
                auto &sendhost= getsendto(index);
                bool sendnums=setnums<0;
                sendhost.sendnums=sendnums;
                if(sendnums) {
                       sendhost.starttime=time(nullptr);
                       sendhost.nums[0].lastlastpos=0;
                       sendhost.nums[1].lastlastpos=0;
//                       sendhost.nums[0].len=1;
 //                      sendhost.nums[1].len=1;
                       }
                bool receive=!(sendnums&&sendhost.sendstream);
                const bool        activeonly=getactive(index);
                const bool        passiveonly=getpassive(index);
                wearhost->receivefrom=getreceivefrom(index,receive,activeonly,passiveonly);
                settings->data()->nochangenum=!sendnums;
                }
            }

        if(watchHasSensor) { 

        const bool        activeonly=getactive(index);
        const bool        passiveonly=getpassive(index);
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
                    updat.sendstream=false; //******
                    LOGGER("watchHasSEnsor %d sendstream=%d\n",watchHasSensor,updat.sendstream);
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
                LOGGER("sendstream=%d\n",updat.sendstream);
                if(usedversion) {
                    info.sendnums=updat.sendnums;
                    info.sendscans=updat.sendscans;
                    }
                if(updat.sendnums) {
                    receive=false;
                    backup->endactivereceive(index);
                    }
                else
                    receive=true;
                }
            wearhost->receivefrom=getreceivefrom(index,receive,activeonly,passiveonly);
            }
        else {
            info.watchsensor=hasDirectWatchConnection(wearhost);
            setsendinfo(info,wearhost);
           }
        }
    else {
        info.watchsensor=hasDirectWatchConnection(wearhost);
        setsendinfo(info,wearhost);
        if(usedversion>=4) {
            if(!wearhost->haspass()) {
                 char pass[17];
                 makepass(pass,16);
                 pass[16]='\0';
                 LOGGER("created pass %.16s\n",pass);
                 backup->setpass(info.pass,std::string_view(pass,16));
                 info.setpass=true;
                 }
              }
        }
    LOGGER("getmynetinfo info.watchsensor=%d\n",info.watchsensor);
    info.version=3;
    char *infolabel=usedversion?info.newlabel:reinterpret_cast<netinfo *>(&info)->label;
    strcpy(infolabel, wearhost->getname()); 
    const int len=usedversion?sizeof(netinfo2):sizeof(netinfo);
    return len;
    }


extern "C" JNIEXPORT  jbyteArray  JNICALL   fromjava(getmynetinfo)(JNIEnv *env, jclass cl,jstring jident,jboolean create,jint watchHasSensor,jboolean galaxy,jint setnums) {

    if(!jident) {
        LOGARTAG("jident=null");
        return nullptr;
        }
      const char *id = env->GetStringUTFChars( jident, NULL);
        if (id == nullptr) {
        LOGARTAG("id=null");
        return nullptr;
        }
    destruct   dest([jident,id,env]() {env->ReleaseStringUTFChars(jident, id);});

    struct netinfo2 info;
    int len;
     if(!(len=getmynetinfo(id, create, watchHasSensor, galaxy, setnums,info)))
        return nullptr;
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

    usedversion=(lens==sizeof(netinfo))?0:(lens==sizeof(netinfo1)?3:(lens>=sizeof(netinfo2)?4:1));
    if(usedversion==1) {
        LOGGER("lens=%d sizeof(netinfo)==%d sizeof(netinfo1)==%d sizeof(netinfo2)==%d\n",lens,sizeof(netinfo),sizeof(netinfo1),sizeof(netinfo2));
        }
    jbyte data[lens];
    env->GetByteArrayRegion(jar, 0, lens,data);
    const netinfo2 *info=reinterpret_cast<const netinfo2*>(data);
    passhost_t *host=getwearoshost(true,id,galaxy);
    if(!host) return false;
   networkpresent=false;
    destruct _niets {[]() { networkpresent=true;}};
   struct updatedata *update=backup->getupdatedata();
    passhost_t *allhosts=update->allhosts;
   int index=host-allhosts;
   connectionbusy[index]=true;
   destruct _{[index]{ connectionbusy[index]=false;}};
//   std::jthread th{Backup::closesocksone,backup,index};
   backup->closesocksone(index);
   const char *infolabel=usedversion?info->newlabel:reinterpret_cast<const netinfo *>(info)->label;
   LOGGERTAG("setmynetinfo %s usedversion=%d infolabel=%s galaxy=%d watchsensor=%d\n",id,usedversion,infolabel,galaxy,info->watchsensor);
    host->setname(infolabel);
   if(!usedversion) {
       namehost hostnamer(&info->ip);
       namehost oldname(host->ips);
       LOGGERTAG("hostname %s->%s\n",oldname.data(),hostnamer.data());
        if(!host->putip(&info->ip)) {
        LOGARTAG("putip failed");
        }
        }
   else  {
        if(usedversion>=3) {
            int otherindex= info->index;
            peers2us[otherindex]=index;
            us2peers[index]=otherindex;
            if(usedversion>=4) {
                if(info->setpass) {
                    if constexpr(!iswatchapp()) { 
                       sendpass=true;
                       }
                    //remakewearhost=true;
                    memcpy(host->pass.data(),info->pass.data(),info->pass.size());
                    LOGARTAG("setpass");
                    backup->setcrypt(host);
                    backup->closesocksone(index);
                    }
                }
//             if(info->version>=3) usedversion=4;
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
    LOGGERTAG("setmynetinfo port=%d nr=%d watchsensor=%d\n",port,host->nr,info->watchsensor);
    if constexpr(iswatchapp()) {
        LOGARTAG("is watch");
    
        if(info->watchsensor) {
           // settings->data()->nobluetooth=false;
              settings->setusebluetooth(true);
//            bool sendnums=false;
            if(!host->isSender()||!getsendto(index).sendstream||getsendto(index).sendnums==info->sendnums) {

                bool sendnums=!info->sendnums;
                settings->data()->nochangenum=!sendnums;
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
                auto lasttime=sendstreamfrom();

                bool activeonly=getactive(index);
                bool passiveonly=getpassive(index);
                    backup->changehost(index,nullptr,(jobjectArray)names,len,true,portstr,sendnums, sendstream, sendscans,false, receive,activeonly ,backup->getpass(index).data(),lasttime,passiveonly,infolabel,false,true);
                }
        }
    else {
//        settings->data()->nobluetooth=true;
        settings->setusebluetooth(false);
        const bool sendnums=!info->sendnums;
        if(host->isSender()) {
             const updateone &updat=getsendto(host);
             if(updat.blueWatch) {
                LOGAR("setmynetinfo  blueWatch=true");
                return true;
                }
             if(!updat.sendstream&&updat.sendnums==sendnums) {
                    LOGAR("setmynetinfo no need");
                    return true;
                    }
            }
        else {
            if(!sendnums)  {
                LOGAR("setmynetinfo no send stream or nums");
                return true;
                }
            }
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
        settings->data()->nochangenum=!sendnums;
        bool activeonly=getactive(index);
        bool passiveonly=getpassive(index);
        uint32_t starttime=time(nullptr);
        //continues where left if sendnums=true
        bool receive=true;
        backup->changehost(index,nullptr,(jobjectArray)names,len,true,portstr,sendnums, false, false,false, receive,activeonly ,backup->getpass(index).data(),starttime,passiveonly,infolabel,false,true);


        }
    }
    else { 
        LOGGER("is no watch watchsensor=%d sendstream=%d\n",info->watchsensor,host->isSender()&&getsendto(host).sendstream);
        if(info->watchsensor) {
          //  settings->data()->nobluetooth=true;
           settings->setusebluetooth(false);
            if(host->isSender()) {
                getsendto(host).sendstream=false;
                getsendto(host).sendnums=!info->sendnums;
                }
            host->receivefrom= host->receivefrom|2;
            LOGGER("sendstream(%d)=false\n",index);
            }
        }
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

extern void setallunit(int unit);
extern "C" JNIEXPORT  jboolean  JNICALL   fromjava(ontbytesettings)(JNIEnv *env, jclass cl,jbyteArray  jar) {
//    Tings *set=settings->data();
    sendsettings ssbuf;
    const int minlen=offsetof(sendsettings,numalarm);
    const jsize lens=env->GetArrayLength(jar);
    if(lens<minlen) {
        LOGGERTAG("ontbytesettings %d<%d\n",lens,minlen);
        return false;
        }
    env->GetByteArrayRegion(jar, 0, lens,reinterpret_cast<jbyte *>(&ssbuf));
    setallunit(ssbuf.unit);

    LOGGERTAG("ontbytesettings unit=%d\n",ssbuf.unit);
    return true;
/*
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
    const int allen=offsetof(sendsettings,alarmnr)-offsetof(sendsettings,lowalarm);
    memcpy(&set->lowalarm,&ss.lowalarm,allen);
    set->alarmnr=ss.alarmnr;
    memcpy(set->numalarm,ss.numalarm,sizeof(amountalarm)*ss.alarmnr);
    LOGGERTAG("ontbytesettings success unit=%d highalarm=%d\n",set->unit,set->highalarm);
    return true;
    */
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
        if(connectionbusy[index]) {
                LOGGER("directsensorwatch connectionbusy[%d]\n",index);
                return -1;
                }
        uint32_t nu=time(nullptr);
        long last=lastuptodate[index];
        if((nu-last)>3*60)
            return -1;

        connectionbusy[index]=true;
        destruct _{[index]{ connectionbusy[index]=false;}};
        bool direct=hasDirectWatchConnection(host);
        /*
         if(!direct&&settings->data()->nobluetooth) {
            LOGGER("directsensorwatch index=%d not direct, but not phone so other watch\n",index);
            return -1;
            } */
        return direct;
        }
    return -1;
       }
extern "C" JNIEXPORT jint  JNICALL   fromjava(hasWatchNums)(JNIEnv *env, jclass cl,jstring jident) {

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
        return hasWatchNums(host);
        }
    return -1;
       }

int getwearindex(JNIEnv *env, jstring jident) {
   if(!jident) 
       return -1;
   const char *id = env->GetStringUTFChars( jident, NULL);
   if (id == nullptr) 
       return -1;
   destruct   dest([jident,id,env]() {env->ReleaseStringUTFChars(jident, id);});
   passhost_t *host=getwearoshost(false,id,true);
   if(!host) 
       return -1;
   int index=host- backup->getupdatedata()->allhosts;
   LOGGERTAG("%s index=%d\n",id,index);
   return index;
   }
extern "C" JNIEXPORT  void  JNICALL   fromjava(isGalaxyWatch)(JNIEnv *env, jclass cl,jboolean val) {
   LOGGER("setGalaxyWatch(%d)\n",val);
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


extern jmethodID jswitchbluetooth;
extern jclass JNIApplic;
extern JNIEnv *getenv();
//meaning sensor,nums:
//1: watch,-1:phone, 0:keep current setting
bool setBlueWatch(passhost_t *host,int sensor,int nums) {
#ifndef WEAROS
        if(!host->wearos) {
            LOGGER("BlueWatch send from not wearos connection %s\n",host->getname());
            return false;
            }
       const char *name= host->getname();
       struct netinfo2 info;
       const int len=getmynetinfo(name,false, sensor,true, nums,info);
       if(len<0) {
            LOGGER("setBlueWatch(%s sensor=%d nums=%d)=false\n",name,sensor,nums);
            return false;
            }
        if(sensor) {
            auto *env=getenv();
            jbyteArray jinfoAr = env->NewByteArray(len);
            env->SetByteArrayRegion(jinfoAr, 0, len, reinterpret_cast<const jbyte *>(&info));
            const bool res=env->CallStaticBooleanMethod(JNIApplic,jswitchbluetooth,env->NewStringUTF(name),jinfoAr,true);
           LOGGER("setBlueWatch( %s sensor=%d nums=%d)=%d\n",name,sensor,nums,res);
            env->DeleteLocalRef(jinfoAr);
            return res;
            }
         else {
             LOGGER("setBlueWatch( %s sensor=%d nums=%d)=true\n",name,sensor,nums);
             return true;
            }
#else
            return true;
#endif
        }
#ifdef WEAROS
#include <thread>
extern uint32_t getnumlasttime();
void watchBluetoothThread(passhost_t *host,jboolean sensor,jboolean amounts) {
    if(amounts)
        settings->data()->nochangenum=false;
    int index=host- backup->getupdatedata()->allhosts;
   connectionbusy[index]=true;
   destruct _{[index]{ connectionbusy[index]=false;}};
    const uint16_t port=host->getport();
    bool sendnums=amounts;
    bool sendstream=sensor;
    bool sendscans=false;
    bool receive=false;
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
    auto lasttime=sensor?sendstreamfrom():getnumlasttime(); 

    bool activeonly=getactive(index);
    bool passiveonly=getpassive(index);
    backup->changehost(index,nullptr,(jobjectArray)names,len,true,portstr,sendnums, sendstream, sendscans,false, receive,activeonly ,backup->getpass(index).data(),lasttime,passiveonly,host->getname(),false,true);

    getsendto(index).blueWatch=true;
    backup->wakebackup(wakestream);
    }
    
extern "C" JNIEXPORT void  JNICALL   fromjava(watchBluetooth)(JNIEnv *env, jclass cl,jstring jident,jboolean sensor,jboolean amounts) {
    if(!jident) { 
        LOGAR("watchBluetooth jident==null");
        return ;
        }
    if(!sensor&&!amounts) {
        LOGAR("watchBluetooth no sensor no amou9nts");
        return;
        }
     LOGAR("watchBluetooth");
      const char *id = env->GetStringUTFChars( jident, NULL);
      if (id == nullptr) return;
      destruct   dest([jident,id,env]() {env->ReleaseStringUTFChars(jident, id);});
      if(passhost_t *host=getwearoshost(false,id,true)) {
//            std::thread th{watchBluetoothThread,host}; th.detach();
            watchBluetoothThread(host,sensor,amounts);
            }
        return ;
       }
#endif
