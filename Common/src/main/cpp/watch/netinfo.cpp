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
#include <string>
#include <unordered_map>
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
#include <atomic>
std::array<std::atomic_int,maxallhosts> peers2us,us2peers;
std::atomic_bool messagephonepeer[maxallhosts]={};
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
// usedversion/newlycreated are legacy protocol-negotiation state. JNI may
// dispatch /netinfo for different watches concurrently, so serialize netinfo
// operations and retain the negotiated wire version by mirror label.
static std::mutex netinfomutex;
static std::unordered_map<std::string,uint8_t> peerversions;
static uint8_t peerversion(const char *label) {
    const auto found=peerversions.find(label);
    return found==peerversions.end()?thisversion:found->second;
    }
passhost_t * getwearoshost(const bool create,const char *label,bool galaxy,bool remake=false,bool phonepeer=false) {
  const std::lock_guard<std::mutex> lock(change_host_mutex);
    struct updatedata *update=backup->getupdatedata();
     int nrhost=update->hostnr;
    LOGGER("getwearoshost(create=%d,%s,galaxy=%d, remake=%d, phonepeer=%d) usedversion=%d nrhost=%d \n",create,label,galaxy,remake,phonepeer,usedversion,nrhost);
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
        if(phonepeer) {
            LOGARTAG("phone peer: keep mirror data one-way; carrier is configured separately");
            }
        else if(galaxy) {
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
   const std::lock_guard<std::mutex> netinfolock(netinfomutex);
   usedversion=peerversion(infolabel);
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
            LOGGERTAG("is watch isSender=%d\n",wearhost->isSender());
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
static bool previoushaswlan[maxallhosts]={};
static bool networkstateknown[maxallhosts]={};
static std::mutex networkstatemutex;
std::atomic_bool wearmessages[maxallhosts]={};

bool messagehaswlan(int index) {
    if(index<0||index>=maxallhosts)
        return false;
    const std::lock_guard<std::mutex> lock(networkstatemutex);
    return networkstateknown[index]&&previoushaswlan[index];
    }
#endif


extern void makepass(char *pass,int len);
// Password acknowledgements belong to one mirror row. A single global flag
// allowed watch A's acknowledgement to be consumed while constructing the
// /netinfo reply for watch B.
static std::array<std::atomic_bool,maxallhosts> sendpass{};

// Runtime current-peer endpoint cache is defined below together with the BLE
// /bleips helpers. Wear /netinfo feeds the same cache so Automatic transport
// has one authoritative "peer is reachable here now" source independent of
// persistent bootstrap addresses.
static bool setMirrorCurrentPeerAddresses(int index,const sockaddr_in6 *ips,int nr);
bool mirrorPeerCurrentTcpAvailable(int index);
static void updateMirrorLocalTcpAvailability(bool present);


static int getmynetinfo(const char *id,jboolean create,jint watchHasSensor,jboolean galaxy,jint setnums,struct netinfo2 &info,bool phonepeer) {
    const std::lock_guard<std::mutex> netinfolock(netinfomutex);
    usedversion=peerversion(id);
    if(!backup) {
        LOGARTAG("getmynetinfo backup=null");
        return 0;
        }

    auto myport=atoi(backup->getmyport());
    LOGGERTAG("getmynetinfo(%s,%d,%d,%d,phonepeer=%d) port=%d\n", id,create,watchHasSensor,galaxy,phonepeer,myport);
    passhost_t *wearhost=getwearoshost(create,id,galaxy,false,phonepeer);
    if(!wearhost)  {
        LOGARTAG("wearhost==null");
        return 0;
        }
    struct updatedata *update=backup->getupdatedata();
    int index=wearhost-update->allhosts;
   messagephonepeer[index]=phonepeer;
   connectionbusy[index]=true;
   destruct _{[index]{ connectionbusy[index]=false;}};
    info.index=index;
    info.setpass=false;;
    if(usedversion) {
        bool haswlan;
        info.nr=getownips(info.ips,passhost_t::maxip-1,haswlan);
        // Wear /netinfo is often the first network enumeration after install.
        // Feed the same cache used by Automatic transport so the initial
        // TCP-vs-Message/BLE decision is based on an actual local address
        // snapshot rather than the conservative "unknown means available".
        updateMirrorLocalTcpAvailability(info.nr>0);

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
        bool networkchanged;
        uLong oldcrc;
        bool oldhaswlan;
        {
            const std::lock_guard<std::mutex> lock(networkstatemutex);
            oldcrc=crcs[index];
            oldhaswlan=previoushaswlan[index];
            networkchanged=!networkstateknown[index]||newcrc!=oldcrc||haswlan!=oldhaswlan;
            crcs[index]=newcrc;
            previoushaswlan[index]=haswlan;
            networkstateknown[index]=true;
        }
        if(networkchanged) {
            LOGGERTAG("network changed: crc=%lu->%lu wlan=%d->%d\n",oldcrc,newcrc,oldhaswlan,haswlan);
            if(wearhost->automatictransport()) {
                const bool setmess=!haswlan;
                LOGGERTAG("automatic mirror %s(%d): network selects %s\n",wearhost->getnameif(),index,
                        setmess?"Messages":"TCP/IP");
                // Retain the established asymmetry: a phone changes to
                // Messages immediately when WLAN disappears and uses the
                // retry timer to return; a watch can switch in either
                // direction as its network state changes.
                if(!phonepeer) {
                #ifndef WEAROS
                    if(setmess)
                #endif
                        setBlueMessage(index,setmess);
                    }
                }
            else
                LOGGERTAG("forced mirror %s(%d): ignoring network carrier change\n",wearhost->getnameif(),index);
            }
    else  {
            LOGARTAG("crc the same");
        }
    info.blue=wearhost->gettransport()==passhost_t::transport_messages||
            (wearhost->automatictransport()&&wearmessages[index].load());
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
        if(phonepeer) {
            if(wearhost->haspass()) {
                memcpy(info.pass.data(),wearhost->pass.data(),info.pass.size());
                }
            else {
                char pass[17];
                makepass(pass,16);
                pass[16]='\0';
                backup->setpass(info.pass,std::string_view(pass,16));
                }
            info.setpass=true;
            sendpass[index].store(false);
            }
        else if(usedversion>=4&&sendpass[index].exchange(false)) {
            LOGARTAG("sendpass");
            memcpy(info.pass.data(),wearhost->pass.data(),info.pass.size());
            info.setpass=true;
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
                 }
            else
                memcpy(info.pass.data(),wearhost->pass.data(),info.pass.size());
            // The watch owns the password for its Wear mirror. Sending it on
            // every version-4 Data Layer bootstrap also repairs a phone
            // row that an older multi-watch build accidentally overwrote.
            info.setpass=true;
            LOGARTAG("watch sends authoritative mirror password");
              }
        }
    LOGGER("getmynetinfo info.watchsensor=%d\n",info.watchsensor);
    info.version=3;
    char *infolabel=usedversion?info.newlabel:reinterpret_cast<netinfo *>(&info)->label;
    strcpy(infolabel, wearhost->getname()); 
    const int len=usedversion?sizeof(netinfo2):sizeof(netinfo);
    return len;
    }


extern "C" JNIEXPORT  jbyteArray  JNICALL   fromjava(getmynetinfo)(JNIEnv *env, jclass cl,jstring jident,jboolean create,jint watchHasSensor,jboolean galaxy,jint setnums,jboolean phonepeer) {
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

    struct netinfo2 info{};
    int len;
     if(!(len=getmynetinfo(id,create,watchHasSensor,galaxy,setnums,info,phonepeer)))
        return nullptr;
    jbyteArray uit = env->NewByteArray(len);
    if(!uit) {
        if(env->ExceptionCheck())
            env->ExceptionClear();
        return nullptr;
        }
    env->SetByteArrayRegion(uit, 0, len, reinterpret_cast<const jbyte *>(&info));
    if(env->ExceptionCheck()) {
        env->ExceptionClear();
        env->DeleteLocalRef(uit);
        return nullptr;
        }
    return uit;
    }

extern "C" JNIEXPORT jstring JNICALL fromjava(setmynetinfo)(JNIEnv *env,jclass cl,jstring jident,jbyteArray jar,jboolean galaxy,jboolean phonepeer) {
    const std::lock_guard<std::mutex> netinfolock(netinfomutex);
   if(!jar) return nullptr;
   if(!backup) return nullptr;
    if(!jident) return nullptr;
   const char *id = env->GetStringUTFChars( jident, NULL);
   if (id == nullptr) return nullptr;
    destruct   dest([jident,id,env]() {env->ReleaseStringUTFChars(jident, id);});

    const jsize lens=env->GetArrayLength(jar);
    if(lens<17||lens>4096)
        return nullptr;

    const uint8_t incomingversion=(lens==sizeof(netinfo))?0:(lens==sizeof(netinfo1)?3:(lens>=sizeof(netinfo2)?4:1));
    if(incomingversion==1) {
        LOGGER("lens=%d sizeof(netinfo)==%d sizeof(netinfo1)==%d sizeof(netinfo2)==%d\n",lens,sizeof(netinfo),sizeof(netinfo1),sizeof(netinfo2));
        }
    struct netinfo2 received{};
    const jsize copylen=std::min<jsize>(lens,sizeof(received));
    env->GetByteArrayRegion(jar,0,copylen,reinterpret_cast<jbyte*>(&received));
    if(env->ExceptionCheck()) {
        env->ExceptionClear();
        return nullptr;
        }
    const netinfo2 *info=&received;
    constexpr int maxreceivedips=sizeof(info->ips)/sizeof(info->ips[0]);
    if(incomingversion&&(info->nr<0||info->nr>maxreceivedips))
        return nullptr;
    const char *receivedlabel=incomingversion?info->newlabel:reinterpret_cast<const netinfo *>(info)->label;
    constexpr size_t receivedlabelsize=sizeof(info->newlabel);
    if(!memchr(receivedlabel,'\0',receivedlabelsize))
        return nullptr;
    if(incomingversion>=3&&(info->index<0||info->index>=maxallhosts))
        return nullptr;
    // The Java boundary supplies the label expected for this exact Wear node:
    // the source node ID on a phone and the local watch node ID on a watch.
    // Never trust a different embedded label. Otherwise a stale Data Layer
    // mapping can overwrite another watch's IP addresses and password.
    if(!*receivedlabel||strcmp(receivedlabel,id)) {
        LOGGERTAG("reject /netinfo label mismatch expected=%s received=%s; no mirror data changed\n",id,receivedlabel);
        return nullptr;
        }
    usedversion=incomingversion;
    peerversions[std::string(id)]=incomingversion;
    passhost_t *host=getwearoshost(true,id,galaxy,false,phonepeer);
    if(!host) return nullptr;
    host->wearos=true;
   networkpresent=false;
    destruct _niets {[]() { networkpresent=true;}};
   struct updatedata *update=backup->getupdatedata();
    passhost_t *allhosts=update->allhosts;
   int index=host-allhosts;
   messagephonepeer[index]=phonepeer;
   connectionbusy[index]=true;
   destruct _{[index]{ connectionbusy[index]=false;}};
//   std::jthread th{Backup::closesocksone,backup,index};
   backup->closesocksone(index);
    const char *infolabel=id;
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
                    bool acceptpass=true;
                    if constexpr(iswatchapp()) {
                        // A phone only echoes the password originated by this
                        // watch. Do not let an unsolicited/stale phone payload
                        // replace an already established watch password.
                        if(host->haspass()&&memcmp(host->pass.data(),info->pass.data(),info->pass.size())) {
                            LOGARTAG("ignored conflicting password received by watch");
                            acceptpass=false;
                            }
                        }
                    else {
                        sendpass[index].store(true);
                       }
                    if(acceptpass) {
                        //remakewearhost=true;
                        memcpy(host->pass.data(),info->pass.data(),info->pass.size());
                        LOGARTAG("setpass");
                        backup->setcrypt(host);
                        backup->closesocksone(index);
                        }
                    }
                }
//             if(info->version>=3) usedversion=4;
            }

       namehost oldname(host->ips);
       LOGGERTAG("hostname %s->new names:\n",oldname.data());
       #ifndef NOLOG
        for(int i=0;i<info->nr;i++) {
            namehost name(info->ips+i);
            LOGGERTAG("%s port=%d\n",name.data(), ntohs( info->ips[i].sin6_port));
            }
        #endif
       // A non-empty /netinfo update refreshes the remembered TCP candidates.
       // A zero-address update only says that the peer has no usable network
       // endpoint *right now*.  Keep the last learned addresses so they remain
       // available as TCP bootstrap candidates when Wi-Fi comes back.  The
       // normal password-authenticated mirror handshake verifies the peer before
       // any remembered address can become the active TCP carrier.
       // /netinfo is authenticated by the Wear transport relationship and
       // describes the peer's current network endpoints. Feed the same runtime
       // cache used by /bleips so carrier promotion never has to infer current
       // reachability from stale persisted addresses.
       setMirrorCurrentPeerAddresses(index,info->nr>0?info->ips:nullptr,info->nr);
       if(info->nr>0)
           host->putips(info->ips,info->nr);
       else
           LOGGERTAG("setmynetinfo %s(%d): peer has no current TCP endpoint; cleared transient endpoints and keeping %d remembered IP(s)\n",
                   host->getnameif(),index,host->nr);

#ifdef WEAROS_MESSAGES
    if(usedversion>=3) {
        if(host->automatictransport()) {
        #ifndef WEAROS
            // Remote /netinfo may request a non-TCP carrier immediately when
            // its network is gone, but it is not proof that TCP is safe when
            // info->blue is false. The Java/native password-authenticated TCP
            // probe performs that promotion separately.
            if(info->blue)
                setBlueMessage(index,true);
        #endif
            }
        else
            setBlueMessage(index,host->forcedmessagecarrier());
        }
#endif
       }
    const uint16_t port=host->getport();
    LOGGERTAG("setmynetinfo port=%d nr=%d watchsensor=%d\n",port,host->nr,info->watchsensor);
    if(phonepeer)
        LOGARTAG("phone peer keeps the explicitly configured one-way mirror settings");
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
                return env->NewStringUTF(host->getname());
                }
             if(!updat.sendstream&&updat.sendnums==sendnums) {
                    LOGAR("setmynetinfo no need");
                    return env->NewStringUTF(host->getname());
                    }
            }
        else {
            if(!sendnums)  {
                LOGAR("setmynetinfo no send stream or nums");
                return env->NewStringUTF(host->getname());
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
    return env->NewStringUTF(host->getname());
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
   if(!backup||!jident)
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
       struct netinfo2 info{};
       const int len=getmynetinfo(name,false,sensor,true,nums,info,false);
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


/*
 * Authenticated BLE endpoint exchange.
 *
 * QR-provided addresses remain the universal bootstrap and are stored in the
 * same passhost_t::ips[] array.  BLE is only an optional authenticated update
 * channel: it sends the current addresses after the existing BLE handshake and
 * when Android reports a network change.  Non-Bluetooth peers (including the
 * command-line server) never use these functions and continue to work solely
 * from the QR/cached addresses.
 *
 * Wire format v1:
 *   byte 0      version (=1)
 *   byte 1      number of sockaddr_in6 records
 *   remaining   exactly nr * sizeof(sockaddr_in6) bytes
 *
 * getownips() already canonicalises IPv4 into IPv4-mapped sockaddr_in6 values,
 * and the port is stored in every record, exactly as /netinfo has done for
 * Wear OS.  sockaddr_in6 is a fixed Linux/Bionic socket ABI structure and this
 * packet is only exchanged between Android Juggluco BLE peers.
 */
static constexpr uint8_t bleaddressversion=1;

// Current local network reachability, deliberately independent of remembered
// peer addresses.  Probe it only at explicit configuration/network-address
// boundaries (getMirrorAddresses()); sender hot paths read this cached state.
// Unknown is treated as potentially available so startup cannot suppress TCP
// before the first authoritative address snapshot has been taken.
static std::atomic_bool mirrorLocalTcpKnown{false};
static std::atomic_bool mirrorLocalTcpPresent{false};
bool mirrorLocalTcpAvailable() {
    return !mirrorLocalTcpKnown.load()||mirrorLocalTcpPresent.load();
    }
static void updateMirrorLocalTcpAvailability(bool present) {
    mirrorLocalTcpPresent.store(present);
    mirrorLocalTcpKnown.store(true);
    }

// Authenticated /bleips data describes where this peer is reachable *now*.
// Keep that information separately from passhost_t::ips[].  The latter also
// contains deliberately configured Internet/global bootstrap addresses and
// must not be destroyed merely because the phones temporarily share a LAN.
//
// This transient cache solves the "all saved slots are non-local" case: an
// Automatic mirror can try the freshly authenticated LAN address first while
// still retaining every configured address unchanged on disk.
static std::mutex mirrorCurrentPeerMutex;
static std::array<std::array<sockaddr_in6,passhost_t::maxip-1>,maxallhosts> mirrorCurrentPeerIps{};
static std::array<int,maxallhosts> mirrorCurrentPeerNr{};

static bool mirrorCurrentSame(const sockaddr_in6 &one,const sockaddr_in6 &other) {
    return one.sin6_family==other.sin6_family&&
            one.sin6_port==other.sin6_port&&
            one.sin6_scope_id==other.sin6_scope_id&&
            !memcmp(&one.sin6_addr,&other.sin6_addr,sizeof(one.sin6_addr));
    }

static bool setMirrorCurrentPeerAddresses(int index,const sockaddr_in6 *ips,int nr) {
    if(index<0||index>=maxallhosts)
        return false;
    nr=std::max(0,std::min(nr,passhost_t::maxip-1));
    const std::lock_guard<std::mutex> lock(mirrorCurrentPeerMutex);
    bool changed=mirrorCurrentPeerNr[index]!=nr;
    if(!changed)
        for(int i=0;i<nr;++i)
            if(!mirrorCurrentSame(mirrorCurrentPeerIps[index][i],ips[i])) {
                changed=true;
                break;
                }
    if(changed) {
        mirrorCurrentPeerNr[index]=nr;
        for(int i=0;i<nr;++i)
            mirrorCurrentPeerIps[index][i]=ips[i];
        for(int i=nr;i<passhost_t::maxip-1;++i)
            mirrorCurrentPeerIps[index][i]={};
        }
    return changed;
    }

// Used by the TCP sender.  These addresses are runtime hints only; callers must
// still perform Juggluco's normal label/password authentication.
int getMirrorCurrentPeerAddresses(int index,sockaddr_in6 *out,int max) {
    if(!out||max<=0||index<0||index>=maxallhosts)
        return 0;
    const std::lock_guard<std::mutex> lock(mirrorCurrentPeerMutex);
    const int nr=std::min(max,mirrorCurrentPeerNr[index]);
    for(int i=0;i<nr;++i)
        out[i]=mirrorCurrentPeerIps[index][i];
    return nr;
    }

bool mirrorPeerCurrentTcpAvailable(int index) {
    if(index<0||index>=maxallhosts)
        return false;
    const std::lock_guard<std::mutex> lock(mirrorCurrentPeerMutex);
    return mirrorCurrentPeerNr[index]>0;
    }

extern "C" JNIEXPORT jbyteArray JNICALL fromjava(getMirrorAddresses)(JNIEnv *env,jclass,jint localIndex) {
    if(!backup||localIndex<0||localIndex>=backup->gethostnr()||localIndex>=maxallhosts)
        return nullptr;
    const auto &host=getBackupHosts()[localIndex];
    if(host.deactivated||host.ICE)
        return nullptr;

    std::array<sockaddr_in6,passhost_t::maxip-1> ips{};
    bool haswlan=false;
    int nr=getownips(ips.data(),static_cast<int>(ips.size()),haswlan);
    updateMirrorLocalTcpAvailability(nr>0);
    if(nr<=0) {
        // Send an authenticated zero-record status update.  The receiver uses
        // this to select the non-TCP carrier while the network is unavailable,
        // but deliberately retains its last learned IPs for later TCP recovery.
        const jbyte header[2]={static_cast<jbyte>(bleaddressversion),0};
        jbyteArray out=env->NewByteArray(2);
        if(!out)
            return nullptr;
        env->SetByteArrayRegion(out,0,2,header);
#ifndef NOLOG
        LOGGERTAG("BLE endpoint update for %s(%d): no current IPs; peer should keep remembered endpoints\n",
                host.getnameif(),localIndex);
#endif
        return out;
        }
    nr=std::min<int>(nr,static_cast<int>(ips.size()));
    const uint16_t port=htons(static_cast<uint16_t>(atoi(backup->getmyport())));
    for(int i=0;i<nr;++i)
        ips[i].sin6_port=port;

    const jsize len=2+nr*static_cast<int>(sizeof(sockaddr_in6));
    jbyteArray out=env->NewByteArray(len);
    if(!out)
        return nullptr;
    const jbyte header[2]={static_cast<jbyte>(bleaddressversion),static_cast<jbyte>(nr)};
    env->SetByteArrayRegion(out,0,2,header);
    env->SetByteArrayRegion(out,2,nr*sizeof(sockaddr_in6),reinterpret_cast<const jbyte*>(ips.data()));
    if(env->ExceptionCheck()) {
        env->ExceptionClear();
        env->DeleteLocalRef(out);
        return nullptr;
        }
#ifndef NOLOG
    LOGGERTAG("BLE endpoint update for %s(%d): %d current IP(s), wlan=%d\n",
            host.getnameif(),localIndex,nr,haswlan);
    for(int i=0;i<nr;++i) {
        namehost name(ips.data()+i);
        LOGGERTAG("BLE endpoint %d: %s port=%d\n",i,name.data(),ntohs(ips[i].sin6_port));
        }
#endif
    return out;
    }

static bool bleClearlyLocalAddress(const sockaddr_in6 &address) {
    if(address.sin6_family!=AF_INET6)
        return false;
    const uint8_t *bytes=address.sin6_addr.s6_addr;

    // getownips() represents IPv4 as IPv4-mapped IPv6 (::ffff:a.b.c.d).
    bool mapped=true;
    for(int i=0;i<10;++i)
        if(bytes[i]!=0) {
            mapped=false;
            break;
            }
    mapped=mapped&&bytes[10]==0xff&&bytes[11]==0xff;
    if(mapped) {
        const uint8_t first=bytes[12];
        const uint8_t second=bytes[13];
        return first==10||                                      // RFC1918 10/8
                (first==172&&second>=16&&second<=31)||          // RFC1918 172.16/12
                (first==192&&second==168)||                     // RFC1918 192.168/16
                (first==169&&second==254)||                     // IPv4 link-local
                first==127;                                    // loopback
        }

    // For IPv6, replace only addresses that are unambiguously local.  In
    // particular, a global IPv6 address is retained even if it was learned on
    // some earlier network: it may have been deliberately configured.
    if((bytes[0]&0xfe)==0xfc)                                  // ULA fc00::/7
        return true;
    if(bytes[0]==0xfe&&(bytes[1]&0xc0)==0x80)                  // link-local fe80::/10
        return true;
    bool loopback=true;
    for(int i=0;i<15;++i)
        if(bytes[i]!=0) {
            loopback=false;
            break;
            }
    return loopback&&bytes[15]==1;                             // ::1
    }

static bool bleSameAddress(const sockaddr_in6 &one,const sockaddr_in6 &other) {
    return one.sin6_family==other.sin6_family&&
            !memcmp(&one.sin6_addr,&other.sin6_addr,sizeof(one.sin6_addr));
    }

extern "C" JNIEXPORT jboolean JNICALL fromjava(setMirrorAddresses)(JNIEnv *env,jclass,jint localIndex,jbyteArray jar) {
    if(!backup||!jar||localIndex<0||localIndex>=backup->gethostnr()||localIndex>=maxallhosts)
        return false;
    const jsize len=env->GetArrayLength(jar);
    if(len<2)
        return false;
    jbyte header[2]{};
    env->GetByteArrayRegion(jar,0,2,header);
    if(env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
        }
    const int version=static_cast<uint8_t>(header[0]);
    const int nr=static_cast<uint8_t>(header[1]);
    if(version!=bleaddressversion||nr>=passhost_t::maxip||
            len!=2+nr*static_cast<int>(sizeof(sockaddr_in6)))
        return false;

    std::array<sockaddr_in6,passhost_t::maxip> incoming{};
    if(nr>0)
        env->GetByteArrayRegion(jar,2,nr*sizeof(sockaddr_in6),reinterpret_cast<jbyte*>(incoming.data()));
    if(env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
        }
    for(int i=0;i<nr;++i) {
        if(incoming[i].sin6_family!=AF_INET6||incoming[i].sin6_port==0)
            return false;
        }

    bool changed=false;
    {
        const std::lock_guard<std::mutex> hostlock(change_host_mutex);
        if(!backup||localIndex<0||localIndex>=backup->gethostnr())
            return false;
        auto &host=backup->getupdatedata()->allhosts[localIndex];
        if(host.deactivated||host.ICE) {
            setMirrorCurrentPeerAddresses(localIndex,nullptr,0);
            return false;
            }

        // Bluetooth endpoint discovery is meaningful only for Automatic rows.
        // A TCP/IP-only row is explicitly user-managed, and a Direct-Bluetooth
        // row has no reason to maintain TCP reachability hints.
        if(!host.automatictransport()) {
            setMirrorCurrentPeerAddresses(localIndex,nullptr,0);
#ifndef NOLOG
            LOGGERTAG("BLE endpoint update ignored for non-Automatic %s(%d); keeping %d configured IP(s)\n",
                    host.getnameif(),localIndex,host.nr);
#endif
            return false;
            }

        // Always remember the authenticated *current* peer endpoints in a
        // transient cache, even when every persistent slot is occupied by
        // deliberately configured Internet/global addresses.  sender.cpp tries
        // these runtime endpoints first but never writes them to passhost_t.
        const bool currentChanged=setMirrorCurrentPeerAddresses(localIndex,
                nr>0?incoming.data():nullptr,nr);
        changed=currentChanged;
        if(nr==0) {
#ifndef NOLOG
            LOGGERTAG("BLE peer %s(%d) has no current TCP endpoint; cleared transient endpoints and keeping %d remembered IP(s)\n",
                    host.getnameif(),localIndex,host.nr);
#endif
            return currentChanged;
            }

        if(host.hashostname()) {
#ifndef NOLOG
            LOGGERTAG("BLE endpoint update cached %d current endpoint(s) for hostname-backed %s(%d); keeping configured hostname\n",
                    nr,host.getnameif(),localIndex);
#endif
            return currentChanged;
            }

        // Mirror rows reserve the final sockaddr_in6 storage slot for their
        // label.  /bleips already has the same maxip-1 wire limit.
        const int capacity=host.hasname?passhost_t::maxip-1:passhost_t::maxip;
        const int oldnr=std::max(0,std::min(host.nr,capacity));
        std::array<sockaddr_in6,passhost_t::maxip> merged{};
        std::array<bool,passhost_t::maxip> receivedNow{};
        if(oldnr>0)
            std::copy_n(host.ips,oldnr,merged.begin());
        int mergednr=oldnr;
        bool persistentChanged=false;

#ifndef NOLOG
        for(int i=0;i<nr;++i) {
            namehost name(incoming.data()+i);
            LOGGERTAG("received BLE endpoint %d: %s port=%d local=%d\n",i,name.data(),
                    ntohs(incoming[i].sin6_port),bleClearlyLocalAddress(incoming[i]));
            }
#endif

        for(int incomingIndex=0;incomingIndex<nr;++incomingIndex) {
            const auto &candidate=incoming[incomingIndex];
            // BLE may expose globally scoped IPv6 addresses too.  Do not let
            // automatic discovery manage them: only clearly local addresses
            // are added/replaced.
            if(!bleClearlyLocalAddress(candidate))
                continue;

            int duplicate=-1;
            for(int i=0;i<mergednr;++i)
                if(bleSameAddress(merged[i],candidate)) {
                    duplicate=i;
                    break;
                    }
            if(duplicate>=0) {
                // Refresh port/scope information for an already-known local IP.
                if(memcmp(&merged[duplicate],&candidate,sizeof(candidate))) {
                    merged[duplicate]=candidate;
                    persistentChanged=true;
                    }
                receivedNow[duplicate]=true;
                continue;
                }

            // Preserve everything already configured while there is unused room.
            if(mergednr<capacity) {
                merged[mergednr]=candidate;
                receivedNow[mergednr]=true;
                ++mergednr;
                persistentChanged=true;
                continue;
                }

            // Once full, only an old, clearly-local address may be replaced.
            // Never replace a non-local IPv4 or an IPv6 address that is not
            // unambiguously local.  Also do not replace an address already
            // supplied in this same update merely to fit another one.
            int replace=-1;
            for(int i=0;i<mergednr;++i)
                if(!receivedNow[i]&&bleClearlyLocalAddress(merged[i])) {
                    replace=i;
                    break;
                    }
            if(replace>=0) {
                merged[replace]=candidate;
                receivedNow[replace]=true;
                persistentChanged=true;
                }
#ifndef NOLOG
            else {
                namehost name(&candidate);
                LOGGERTAG("no replaceable local slot for BLE endpoint %s on %s(%d); preserving configured endpoints\n",
                        name.data(),host.getnameif(),localIndex);
                }
#endif
            }

        if(!persistentChanged)
            return changed;
#ifndef NOLOG
        LOGGERTAG("BLE merged local endpoints for %s(%d): %d -> %d IP(s)\n",
                host.getnameif(),localIndex,oldnr,mergednr);
        for(int i=0;i<mergednr;++i) {
            namehost name(merged.data()+i);
            LOGGERTAG("remembered endpoint %d: %s port=%d local=%d\n",i,name.data(),
                    ntohs(merged[i].sin6_port),bleClearlyLocalAddress(merged[i]));
            }
#endif
        host.putips(merged.data(),mergednr);
        changed=true;
        }

    // Persistent address changes remain conservative, while the transient
    // current-peer cache may also have changed even when no saved slot could be
    // replaced. Java uses the true return value to request an orderly
    // BLE -> TCP handoff; sender.cpp then tries the transient endpoints first.
    return changed;
    }
