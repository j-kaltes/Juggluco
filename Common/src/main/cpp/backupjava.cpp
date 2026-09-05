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


#include <jni.h>
#include <alloca.h>
#include "fromjava.h"
#include "datbackup.hpp"
#include "net/netstuff.hpp"
#include <string_view>
#ifndef TESTMENU
#include <mutex>
extern std::mutex change_host_mutex;
#endif
extern jclass JNIString;
extern jstring myNewStringUTF(JNIEnv *env,const std::string_view str);
extern bool networkpresent;

extern "C" JNIEXPORT jboolean  JNICALL   fromjava(backuphasrestore)(JNIEnv *env, jclass cl) {
    return backup->getupdatedata()->hasrestore;
    }
extern "C" JNIEXPORT jint JNICALL fromjava(getbackuptransport)(JNIEnv *,jclass,jint pos) {
    if(!backup||pos<0||pos>=backup->gethostnr())
        return passhost_t::transport_automatic;
    return backup->getupdatedata()->allhosts[pos].gettransport();
    }
extern "C" JNIEXPORT jboolean JNICALL fromjava(getbackupside)(JNIEnv *,jclass,jint pos) {
    if(!backup||pos<0||pos>=backup->gethostnr())
        return false;
    return backup->getupdatedata()->allhosts[pos].side;
    }
extern "C" JNIEXPORT jboolean JNICALL fromjava(getbackupbleclient)(JNIEnv *,jclass,jint pos) {
    if(!backup||pos<0||pos>=backup->gethostnr())
        return false;
    return backup->getupdatedata()->allhosts[pos].bleclient;
    }
extern "C" JNIEXPORT jboolean JNICALL fromjava(getbackupblereverse)(JNIEnv *,jclass,jint pos) {
    if(!backup||pos<0||pos>=backup->gethostnr())
        return false;
    return backup->getupdatedata()->allhosts[pos].blereverse;
    }
extern "C" JNIEXPORT jboolean JNICALL fromjava(getbackupbleunproven)(JNIEnv *,jclass,jint pos) {
    if(!backup||pos<0||pos>=backup->gethostnr())
        return false;
    return backup->getupdatedata()->allhosts[pos].bleunproven;
    }



/**
 * One-shot migration for mirror rows created before non-ICE side had a
 * persistent meaning. The QR-created direction pairs used by Juggluco have
 * complementary Scans settings, so sendscans is the stable r1/r2 discriminator.
 *
 * IMPORTANT: assigning side must never change a physical BLE role that has
 * already proved itself. bleclient is the pre-migration persisted local role.
 * After assigning side, derive the pair-wide direction bit from that existing
 * role:
 *
 *   client = (!side) XOR blereverse
 *   therefore blereverse = client == side
 *
 * This makes the migration role-preserving. For example, an r1 connection
 * that was already a proven GATT server becomes side=r1, blereverse=true and
 * remains a server instead of being reset to the nominal r1-client direction.
 */
extern "C" JNIEXPORT jint  JNICALL   fromjava(backuphostNr)(JNIEnv *env, jclass cl) {
    return backup->    gethostnr();
    }

extern "C" JNIEXPORT jboolean  JNICALL   fromjava(detectIP)(JNIEnv *envin, jclass cl,jint pos) {
    const passhost_t &host=backup->getupdatedata()->allhosts[pos];
    return host.detect;
    }
extern "C" JNIEXPORT jboolean  JNICALL   fromjava(getbackupHasHostname)(JNIEnv *envin, jclass cl,jint pos) {
    const passhost_t &host=backup->getupdatedata()->allhosts[pos];
    return host.hashostname();
    }
extern "C" JNIEXPORT jobjectArray  JNICALL   fromjava(getbackupIPs)(JNIEnv *env, jclass cl,jint pos) {
    if(!backup)  {
        LOGSTRING("backup==null\n");
        return nullptr;
        }
    const auto hostnr=backup->gethostnr();
    if(pos>=hostnr) {
        LOGGER("pos(%d)>=backup->gethostnr()(%d)\n",pos,hostnr);
        return nullptr;
        }
    passhost_t &host=backup->getupdatedata()->allhosts[pos];
    LOGGER("%s pos=%d nr=%d index=%d\n",host.getnameif(),pos,host.nr,host.index);
    int len=host.nr;
    if(len<0||len>passhost_t::maxip) {
        LOGGER("host.nr==%d\n",len);
        host.nr=len=0;
        }
    jobjectArray  ipar = env->NewObjectArray(len,JNIString,nullptr);
    if(!ipar) {
        LOGGER(R"(NewObjectArray(%d,JNIString==null)""\n",len);
        return nullptr;
        }
    if(len>0) {
        if(host.hashostname()) {
            env->SetObjectArrayElement(ipar,0,env->NewStringUTF(host.gethostname()));
            }
        else  {
            for(int i=0;i<len;i++) {
                namehost name(host.ips+i);
                LOGGER("%s\n",name.data());
                env->SetObjectArrayElement(ipar,i,env->NewStringUTF(name));
                }
            }
        }
    return ipar;
    }

/*
extern "C" JNIEXPORT jstring JNICALL   fromjava(getbackuphostname)(JNIEnv *envin, jclass cl,jint pos) {
    if(address(backup->getupdatedata()->allhosts[pos])) {
        auto host=backup->gethost(pos);
        return envin->NewStringUTF(host);
        }
    return nullptr;
    }
    */


int getposbylabel(const char *label) {
    const int nr=backup->gethostnr();
    for(int pos=0;pos<nr;++pos) {
        const passhost_t &host=backup->getupdatedata()->allhosts[pos];
        if(!host.hasname)
           continue;
        if(!strcmp(host.getname(),label)) {
            LOGGER("getposbylabel(%s)=%d\n",label,pos);
            return pos;
            }
         }
    LOGGER("getposbylabel(%s)=-1\n",label);
    return -1;
    }
bool removebylabel(const char *label) {
    int pos = getposbylabel(label);
    if (pos < 0)
        return false;
    backup->deletehost(pos);
    return true;
    }
/*
extern "C" JNIEXPORT jboolean JNICALL   fromjava(removebylabel)(JNIEnv *env, jclass cl,jstring jlabel) {
      const char *label = env->GetStringUTFChars( jlabel, NULL);
        if(!label) return false;
        destruct   dest([jlabel,label,env]() {env->ReleaseStringUTFChars(jlabel, label);});
    return removebylabel(label);
    } */

#ifndef ABBOTT

passhost_t * getwearoshost(const bool create,const char *label,bool,bool=false,bool=false);
bool resetbylabel(const char *label,bool galaxy) {
    
    int pos=getposbylabel(label);
    if(pos<0)
        return false;
    const passhost_t &host=backup->getupdatedata()->allhosts[pos];
    const int nr=host.nr;
    if(nr>0) {
        struct sockaddr_in6 ips[passhost_t::maxip];
        memcpy(ips,host.ips,sizeof(ips));

        passhost_t *newhost=getwearoshost(true,label,galaxy,true);
        memcpy(newhost->ips,ips,sizeof(ips));
        newhost->nr=std::min(nr,passhost_t::maxip);
        }
    else {
        backup->deletehost(pos);
        }
    return true;
    }
    
extern "C" JNIEXPORT jboolean JNICALL   fromjava(resetbylabel)(JNIEnv *env, jclass cl,jstring jlabel,jboolean galaxy) {
    const char *label = env->GetStringUTFChars( jlabel, NULL);
    if(!label) return false;
    LOGGER("resetbylabel(%s,%d)\n",label,galaxy);
    destruct   dest([jlabel,label,env]() {env->ReleaseStringUTFChars(jlabel, label);});
    return resetbylabel(label,galaxy);
    }
#endif
const char *gethostlabel(int pos) {
    if(!backup||pos<0||pos>=backup->gethostnr())
        return nullptr;
    const passhost_t &host=backup->getupdatedata()->allhosts[pos];
    if(!host.hasname)
        return nullptr;
    return host.getname();
    }
bool gethosttestip(int pos) {
    if(!backup||pos>=backup->gethostnr())
        return true;
    const passhost_t &host=backup->getupdatedata()->allhosts[pos];
    return !host.noip;
    }
extern "C" JNIEXPORT jboolean JNICALL   fromjava(getbackuptestip)(JNIEnv *envin, jclass cl,jint pos) {
    return gethosttestip(pos);
    }
extern "C" JNIEXPORT jstring JNICALL   fromjava(getbackuplabel)(JNIEnv *envin, jclass cl,jint pos) {
    if(const char *label=gethostlabel(pos))
        return myNewStringUTF(envin,label);
    return nullptr;
    }

extern "C" JNIEXPORT jstring JNICALL   fromjava(getICElabel)(JNIEnv *env, jclass cl,jint pos) {
    if(!backup||pos<0||pos>=backup->gethostnr())
        return nullptr;
    const passhost_t &host=backup->getupdatedata()->allhosts[pos];
    if(!host.ICE)
        return nullptr;
    std::string_view label=host.getICEname();
    return myNewStringUTF(env,label);
    }
extern "C" JNIEXPORT jboolean JNICALL   fromjava(getICEside)(JNIEnv *env, jclass cl,jint pos) {
    if(!backup||pos<0||pos>=backup->gethostnr())
        return false;
    const passhost_t &host=backup->getupdatedata()->allhosts[pos];
    return host.side;
    }
extern "C" JNIEXPORT jstring JNICALL   fromjava(getbackuppassword)(JNIEnv *envin, jclass cl,jint pos) {
    if(!backup||pos>=backup->gethostnr())
        return nullptr;
    return myNewStringUTF(envin,backup->getpass(pos).data());
    }
extern "C" JNIEXPORT jstring JNICALL   fromjava(getbackuphostport)(JNIEnv *envin, jclass cl,jint pos) {
    if(!backup||pos>=backup->gethostnr())
        return nullptr;
    
    char port[6];
    backup->getport(pos,port);
    return envin->NewStringUTF(port);
    }
extern "C" JNIEXPORT jboolean JNICALL   fromjava(isWearOS)(JNIEnv *envin, jclass cl,jint pos) {
    if(!backup||pos<0||pos>=backup->gethostnr()) {
        LOGGER("isWearos(%d)=false\n",pos);
        return false;
        }
    auto ret= backup->getupdatedata()->allhosts[pos].wearos;
    LOGGER("isWearos(%d)=%d\n",pos,ret);
    return ret;
    }


bool getpassive(int pos);
bool getactive(int pos); 
extern "C" JNIEXPORT jboolean JNICALL   fromjava(getbackuphostactive)(JNIEnv *envin, jclass cl,jint pos) {
    bool active=getactive(pos);
    LOGGER("getbackuphostactive(%d)=%d\n",pos,active);
    return active;
    }

extern "C" JNIEXPORT jboolean JNICALL   fromjava(getbackuphostpassive)(JNIEnv *envin, jclass cl,jint pos) {
    return getpassive(pos);
    }
extern "C" JNIEXPORT int JNICALL   fromjava(getbackuphostreceive)(JNIEnv *envin, jclass cl,jint pos) {
    if(pos<backup->getupdatedata()->hostnr) 
        return backup->getupdatedata()->allhosts[pos].receivefrom;
    return 0;
    }
extern "C" JNIEXPORT jboolean JNICALL   fromjava(getbackuphostnums)(JNIEnv *envin, jclass cl,jint pos) {
    if(pos<backup->getupdatedata()->hostnr) {
        int index=backup->getupdatedata()->allhosts[pos].index;
        if(index>=0)
            return  backup->getupdatedata()->tosend[index].sendnums;
        }
    return false;
    }
extern "C" JNIEXPORT jboolean JNICALL   fromjava(getbackuphoststream)(JNIEnv *envin, jclass cl,jint pos) {
    if(pos<backup->getupdatedata()->hostnr) {
        int index=backup->getupdatedata()->allhosts[pos].index;
        if(index>=0)
            return  backup->getupdatedata()->tosend[index].sendstream;
        }
    return false;
    }
extern "C" JNIEXPORT jboolean JNICALL   fromjava(getbackuphostscans)(JNIEnv *envin, jclass cl,jint pos) {
    if(pos<backup->getupdatedata()->hostnr) {
        int index=backup->getupdatedata()->allhosts[pos].index;
        if(index>=0)
            return  backup->getupdatedata()->tosend[index].sendscans;
        }
    return false;
    }
extern "C" JNIEXPORT jboolean JNICALL   fromjava(getbackuphostnotes)(JNIEnv *envin, jclass cl,jint pos) {
    if(pos<backup->getupdatedata()->hostnr) {
        int index=backup->getupdatedata()->allhosts[pos].index;
        if(index>=0)
            return  backup->getupdatedata()->tosend[index].sendnotes;
        }
    return true;
    }
extern "C" JNIEXPORT void JNICALL   fromjava(setreceiveport)(JNIEnv *env, jclass cl,jstring jport) {
    jint portlen= env->GetStringUTFLength( jport);
    if(portlen<6) {
        char newport[portlen+1];
        jint jlen = env->GetStringLength( jport);
         env->GetStringUTFRegion( jport, 0,jlen, newport); 
        LOGGER("setreceiveport %s\n",newport);
        if(backup->getupdatedata()->port[portlen]||memcmp(newport, backup->getupdatedata()->port,portlen)) {
            memcpy(backup->getupdatedata()->port,newport,portlen);
            backup->getupdatedata()->port[portlen]='\0';
            backup->startreceiver(true);
            }
        }
    }
extern "C" JNIEXPORT jstring JNICALL   fromjava(getreceiveport)(JNIEnv *env, jclass cl) {

    return env->NewStringUTF(backup->getupdatedata()->port);
    }
/*
extern "C" JNIEXPORT jboolean JNICALL   fromjava(stringarray)(JNIEnv *env, jclass cl,jobjectArray jar ) {
    constexpr const int maxad=4;    
    int len=env->GetArrayLength(jar);
    LOGSTRING("stringarray ");
    const char port[]="8795";
    struct sockaddr_in6     connect[maxad];
    int uselen=std::min(maxad,len);
    for(int i=0;i<uselen;i++) {
        jstring  jname=(jstring)env->GetObjectArrayElement(jar,i);
        int namelen= env->GetStringUTFLength( jname);
        char name[namelen+1];
        env->GetStringUTFRegion( jname, 0,namelen, name); name[namelen]='\0';
        LOGGER("%s ",name);
        if(!getaddr(name,port,connect+i))
            return  false;
        }
    LOGSTRING("\nips: ");
    for(int i=0;i<uselen;i++) {
        LOGGER("%s ",namehost(connect+i));
        }
    LOGSTRING("\n");
    return true;
    }
    */

//extern "C" JNIEXPORT jint JNICALL   fromjava(changebackuphost)(JNIEnv *env, jclass cl,jint pos,jobjectArray jnames,jint nr,jboolean detect,jstring jport,jboolean nums,jboolean stream,jboolean scans,jboolean recover,jboolean receive,jboolean reconnect,jboolean accepts,jstring jpass,jlong starttime) {
//extern bool mkwearos;

extern "C" JNIEXPORT jint JNICALL   fromjava(changebackuphost)(JNIEnv *env, jclass cl,jint pos,jobjectArray jnames,jint nr,jboolean detect,jstring jport,jboolean nums,jboolean stream,jboolean scans,jboolean recover,jboolean receive,jboolean activeonly,jboolean passiveonly,jstring jpass,jlong starttime,jstring jlabel,jboolean testip,jboolean hashostname,jstring jICElabel,jboolean side,jboolean notes,jint transport,jboolean bleclient) {
#ifndef TESTMENU
    LOGAR("changebackuphost const std::lock_guard<std::mutex> lock(change_host_mutex)");
  const std::lock_guard<std::mutex> lock(change_host_mutex);
#endif
LOGGER("changebackuphost(%d,%p,%d,%d,%p,%d,%d,%d,%d%,%d,%d,%d,%p,%ld,%p,%d,%d,%d)\n", pos, jnames, nr, detect, jport, nums, stream, scans, recover, receive, activeonly, passiveonly, jpass, starttime, jlabel, testip, hashostname, notes);
    char *passptr=nullptr;
    jint passlen=0;
    if(jpass) {
        passlen= env->GetStringUTFLength( jpass);
        jint jpasslen = env->GetStringLength( jpass);
        passptr=(char *)alloca(passlen+1); 
        env->GetStringUTFRegion( jpass, 0,jpasslen, passptr); passptr[passlen]='\0';
        }
    const char *label=jlabel?env->GetStringUTFChars( jlabel, NULL):nullptr;
     jint res;
     if(jICElabel) {
        const char *ICElabel=env->GetStringUTFChars( jICElabel, NULL);
        res=backup->changeICEhost(ICElabel,pos,nums,stream,scans,receive,
                std::string_view(passptr,passlen),starttime,label,side,true,notes,transport,bleclient);
        env->ReleaseStringUTFChars(jICElabel, ICElabel);
        }
     else {
        jint portlen= env->GetStringUTFLength( jport);
        jint jlen = env->GetStringLength( jport);
        char port[portlen+1]; 
        env->GetStringUTFRegion( jport, 0,jlen, port); port[portlen]='\0';
        const int arlen=jnames?std::min(env->GetArrayLength(jnames),nr):0;
        res=backup->changehost(pos,env,jnames,arlen,detect,std::string_view(port,portlen),nums,stream,scans,recover,receive,activeonly,std::string_view(passptr,passlen),starttime,passiveonly,label,testip,true,hashostname,notes,transport,bleclient);
        if(res>=0&&res<backup->gethostnr()) {
            // For ordinary mirrors side is immutable pair identity. New QR
            // peers explicitly receive the opposite bit; editing a row passes
            // its existing side unchanged.
            auto &savedhost=backup->getupdatedata()->allhosts[res];
            savedhost.side=side;
            // Rows which predate this bit have zero in the formerly-reserved
            // position and are intentionally treated as proven. Only a newly
            // created nearby mirror is allowed to experiment with physical BLE
            // direction until its first authenticated session succeeds.
            if(pos<0)
                savedhost.bleunproven=true;
            LOGGER("changebackuphost saved side=%d bleunproven=%d for %s(%d)\n",side,
                    savedhost.bleunproven,savedhost.getnameif(),res);
            }
        }
    if(jlabel)
        env->ReleaseStringUTFChars(jlabel, label);
    return res;
    }

extern "C" JNIEXPORT jboolean JNICALL fromjava(setbackupblereverse)(JNIEnv *,jclass,jint pos,jboolean reverse) {
    if(!backup||pos<0||pos>=backup->gethostnr())
        return false;
#ifndef TESTMENU
    const std::lock_guard<std::mutex> lock(change_host_mutex);
#endif
    if(pos>=backup->gethostnr())
        return false;
    auto &host=backup->getupdatedata()->allhosts[pos];
    const bool value=reverse;
    if(host.blereverse!=value) {
        LOGGER("setbackupblereverse pair direction %s(%d): %d -> %d\n",
                host.getnameif(),pos,host.blereverse,value);
        host.blereverse=value;
        }
    return true;
    }

extern "C" JNIEXPORT jboolean JNICALL fromjava(setbackupbleunproven)(JNIEnv *,jclass,jint pos,jboolean unproven) {
    if(!backup||pos<0||pos>=backup->gethostnr())
        return false;
#ifndef TESTMENU
    const std::lock_guard<std::mutex> lock(change_host_mutex);
#endif
    if(pos>=backup->gethostnr())
        return false;
    auto &host=backup->getupdatedata()->allhosts[pos];
    const bool value=unproven;
    if(host.bleunproven!=value) {
        LOGGER("setbackupbleunproven %s(%d): %d -> %d\n",
                host.getnameif(),pos,host.bleunproven,value);
        host.bleunproven=value;
        }
    return true;
    }

extern "C" JNIEXPORT jboolean JNICALL fromjava(setbackupbleclient)(JNIEnv *,jclass,jint pos,jboolean bleclient) {
    if(!backup||pos<0||pos>=backup->gethostnr())
        return false;
#ifndef TESTMENU
    const std::lock_guard<std::mutex> lock(change_host_mutex);
#endif
    if(pos>=backup->gethostnr())
        return false;
    auto &host=backup->getupdatedata()->allhosts[pos];
    const bool client=bleclient;
    if(host.bleclient!=client) {
        LOGGER("setbackupbleclient preferred role %s(%d): %d -> %d\n",
                host.getnameif(),pos,host.bleclient,client);
        host.bleclient=client;
        }
    return true;
    }

extern void applyConfiguredMirrorTransports();
extern "C" JNIEXPORT jboolean JNICALL fromjava(setMirrorTransport)(JNIEnv *env,jclass,jstring jlabel,jint transport,jboolean bleclient) {
    if(!backup||!jlabel||transport<passhost_t::transport_automatic||
            transport>passhost_t::transport_bluetooth)
        return false;
    const char *label=env->GetStringUTFChars(jlabel,nullptr);
    if(!label)
        return false;
    int pos=-1;
    {
#ifndef TESTMENU
        const std::lock_guard<std::mutex> lock(change_host_mutex);
#endif
        pos=getposbylabel(label);
        if(pos>=0) {
            if(transport==passhost_t::transport_bluetooth) {
                for(int i=0;i<backup->gethostnr();++i) {
                    const auto &other=backup->getupdatedata()->allhosts[i];
                    if(i!=pos&&!other.deactivated&&other.gettransport()==passhost_t::transport_bluetooth) {
                        pos=-1;
                        break;
                        }
                    }
                }
            if(pos>=0) {
                auto &host=backup->getupdatedata()->allhosts[pos];
                const int oldtransport=host.gettransport();
                host.settransport(transport);
                host.bleclient=bleclient;
                // /mirrortransport is received through the Wear data layer,
                // so this existing QR-created entry is the Wear peer rather
                // than a second mirror that should be auto-created by netinfo.
                host.wearos=true;
                LOGGER("setMirrorTransport saved %s(%d): transport=%d bleclient=%d\n",
                        host.getnameif(),pos,host.gettransport(),host.bleclient);
                if(oldtransport!=transport) {
                    LOGGER("setMirrorTransport %s(%d): %d -> %d, closing old carrier sockets\n",
                            host.getnameif(),pos,oldtransport,transport);
                    backup->closesocksone(pos);
                    }
                }
            }
    }
    env->ReleaseStringUTFChars(jlabel,label);
    if(pos>=0)
        applyConfiguredMirrorTransports();
    return pos>=0;
    }
extern "C" JNIEXPORT void JNICALL fromjava(setMirrorWearOS)(JNIEnv *,jclass,jint index) {
    if(!backup||index<0||index>=backup->gethostnr())
        return;
#ifndef TESTMENU
    const std::lock_guard<std::mutex> lock(change_host_mutex);
#endif
    auto &host=backup->getupdatedata()->allhosts[index];
    host.wearos=true;
    LOGGER("setMirrorWearOS %s(%d)\n",host.getnameif(),index);
    }
extern "C" JNIEXPORT jboolean JNICALL   fromjava(isreceiving)(JNIEnv *env, jclass cl) {
    return backup->isreceiving() ;
    }
extern "C" JNIEXPORT void JNICALL   fromjava(deletebackuphost)(JNIEnv *env, jclass cl,jint pos) {
    backup->deletehost(pos);
    }
extern "C" JNIEXPORT jlong JNICALL   fromjava(lastuptodate)(JNIEnv *env, jclass cl,jint pos) {
    return lastuptodate[pos]*1000LL;
    }
extern "C" JNIEXPORT void JNICALL   fromjava(setWifi)(JNIEnv *env, jclass cl,jboolean val) {
    settings->data()->keepWifi=val;
    }
extern "C" JNIEXPORT jboolean JNICALL   fromjava(getWifi)(JNIEnv *env, jclass cl) {
    return settings->data()->keepWifi;
    }
extern "C" JNIEXPORT jboolean JNICALL   fromjava(stopWifi)(JNIEnv *env, jclass cl) {
    if(settings->data()->keepWifi)
        return false;
        
    return     (time(nullptr)-((long)lastuptodate[0]))<2*60;
    }

extern "C" JNIEXPORT void JNICALL   fromjava(resetbackuphost)(JNIEnv *env, jclass cl,jint pos) {
    backup->resethost(pos) ;
    }
extern void wakeaftermin(const int waitmin) ;
extern void wakeuploader();

extern "C" JNIEXPORT void JNICALL   fromjava(networkpresent)(JNIEnv *env, jclass cl) {
      LOGAR("networkpresent");
    if(backup) {
        backup->getupdatedata()->wakesender();
        networkpresent=true;
        backup->notupdatedsettings();
    //    backup->wakebackup();
        }
    else
        networkpresent=true;

    wakeuploader();
#if !defined(WEAROS) && !defined(TESTMENU)
     wakeaftermin(0) ;
#endif
    LOGAR("end networkpresend");
    }

extern "C" JNIEXPORT void JNICALL   fromjava(switchSync)(JNIEnv *env, jclass cl) {
     networkpresent=true;
     LOGAR("switchSync");
     backup->wakebackup(wakeUpSwitch|wakeall);
     backup->getupdatedata()->wakesender(wakeUpSwitch|wakeall);
    }

//void wakebackup(myuintptr_t kind=wakeall,bool sendwake=false){
void resetnetwork() {
    LOGSTRING("resetnetwork\n");
    if(backup) {
        backup->closeallsocks();
        backup->getupdatedata()->wakesender();
        networkpresent=true;
        backup->notupdatedsettings();
        }
    }
extern "C" JNIEXPORT void JNICALL   fromjava(resetnetwork)(JNIEnv *env, jclass cl) {
    resetnetwork();
    }

extern bool probeMirrorTcp(passhost_t *pass,int timeoutMillis);
extern "C" JNIEXPORT jboolean JNICALL fromjava(probeMirrorTcp)(JNIEnv *,jclass,jint pos) {
    if(!backup||pos<0||pos>=backup->gethostnr())
        return false;
    return probeMirrorTcp(&getBackupHosts()[pos],3000);
    }

extern "C" JNIEXPORT void JNICALL   fromjava(networkabsent)(JNIEnv *env, jclass cl) {
    LOGSTRING("networkabsent\n");
    backup->endAllConnections();
/*    networkpresent=false;
    if(backup) {
        backup->closeallsocks();
        } */
    }
extern "C" JNIEXPORT void JNICALL   fromjava(wakestreamsender)(JNIEnv *env, jclass cl) {
    if(backup) {
        backup->getupdatedata()->wakestreamsender();
        }
    }
extern "C" JNIEXPORT void JNICALL   fromjava(wakestreamhereonly)(JNIEnv *env, jclass cl) {
    if(backup) {
        backup->wakebackup(wakestream);
        }
    }
    /*
extern "C" JNIEXPORT void JNICALL   fromjava(wakeallsender)(JNIEnv *env, jclass cl) {
    if(backup) {
        backup->getupdatedata()->wakesender();
        }
    } */
extern "C" JNIEXPORT void JNICALL   fromjava(wakebackup)(JNIEnv *env, jclass cl) {
    if(backup) {
        backup->getupdatedata()->wakesender();
        backup->wakebackup();
        }
    }
extern "C" JNIEXPORT void JNICALL   fromjava(wakehereonly)(JNIEnv *env, jclass cl) {
    if(backup) {
        backup->wakebackup();
        }
    }

extern "C" JNIEXPORT jboolean JNICALL   fromjava(getHostDeactivated)(JNIEnv *envin, jclass cl,jint pos) {
    if(pos<backup->getupdatedata()->hostnr) {
        return backup->getupdatedata()->allhosts[pos].deactivated;
        }
    return true;
    }
extern "C" JNIEXPORT void JNICALL   fromjava(setHostDeactivated)(JNIEnv *envin, jclass cl,jint pos,jboolean val) {
    backup->deactivateHost(pos,val);
    }



#if !defined(WEAROS)&& __NDK_MAJOR__ >= 26
extern std::string mkbackjson(int pos);
extern "C" JNIEXPORT jstring JNICALL   fromjava(getbackJson)(JNIEnv *envin, jclass cl,jint pos) {
    auto jsonstr=mkbackjson(pos);
    char *data=jsonstr.data();
    data[jsonstr.size()]='\0';
    return myNewStringUTF(envin,jsonstr.data());
    }
extern int makeHomeBackupSender();
extern "C" JNIEXPORT jint JNICALL   fromjava(makeHomeSender)(JNIEnv *envin, jclass cl) {
    return  makeHomeBackupSender();
     }
extern int makeICEsender();
extern "C" JNIEXPORT jint JNICALL   fromjava(makeICESender)(JNIEnv *envin, jclass cl) {
    return makeICEsender();
     }
extern int makeHomeBackupReceiver();
extern "C" JNIEXPORT jint JNICALL   fromjava(makeHomeReceiver)(JNIEnv *envin, jclass cl) {
    return  makeHomeBackupReceiver();
     }
extern int makeICEreceiver();
extern "C" JNIEXPORT jint JNICALL   fromjava(makeICEReceiver)(JNIEnv *envin, jclass cl) {
    return makeICEreceiver();
     }

#endif

extern "C" JNIEXPORT jstring JNICALL   fromjava(getTurnHost)(JNIEnv *env, jclass cl,jint pos) {
    return env->NewStringUTF(backup->getupdatedata()->turnserver[pos].hostname);
    }
extern "C" JNIEXPORT jstring JNICALL   fromjava(getTurnUser)(JNIEnv *env, jclass cl,jint pos) {
    return env->NewStringUTF(backup->getupdatedata()->turnserver[pos].username);
    }
extern "C" JNIEXPORT jstring JNICALL   fromjava(getTurnPassword)(JNIEnv *env, jclass cl,jint pos) {
    return env->NewStringUTF(backup->getupdatedata()->turnserver[pos].password);
    }
extern "C" JNIEXPORT jint JNICALL   fromjava(getTurnPort)(JNIEnv *env, jclass cl,jint pos) {
    return backup->getupdatedata()->turnserver[pos].port;
    }


extern void   recreateAgents();

extern "C" JNIEXPORT void JNICALL   fromjava(setTurnHost)(JNIEnv *env, jclass cl,jint pos,jstring jhost) {
   jint hostlen= env->GetStringUTFLength( jhost);
   jint jlen = env->GetStringLength( jhost);
   env->GetStringUTFRegion( jhost, 0,jlen,backup->getupdatedata()->turnserver[pos].hostname); 
   backup->getupdatedata()->turnserver[pos].hostname[hostlen]='\0';
  backup->getupdatedata()->NRturnserver=1;
   recreateAgents();

   }

extern "C" JNIEXPORT void JNICALL   fromjava(deleteTurnServer)(JNIEnv *env, jclass cl,jint pos) {
    backup->getupdatedata()->NRturnserver=0;
    recreateAgents();
    }
extern "C" JNIEXPORT jint JNICALL   fromjava(TurnServerNR)(JNIEnv *env, jclass cl) {
    return backup->getupdatedata()->NRturnserver;
    }
extern "C" JNIEXPORT void JNICALL   fromjava(setTurnUser)(JNIEnv *env, jclass cl,jint pos,jstring juser) {
   jint userlen= env->GetStringUTFLength( juser);
   jint jlen = env->GetStringLength( juser);
   env->GetStringUTFRegion( juser, 0,jlen,backup->getupdatedata()->turnserver[pos].username); 
   backup->getupdatedata()->turnserver[pos].username[userlen]='\0';
   }
extern "C" JNIEXPORT void JNICALL   fromjava(setTurnPassword)(JNIEnv *env, jclass cl,jint pos,jstring jpassword) {
   jint passwordlen= env->GetStringUTFLength( jpassword);
   jint jlen = env->GetStringLength( jpassword);
   env->GetStringUTFRegion( jpassword, 0,jlen,backup->getupdatedata()->turnserver[pos].password); 
   backup->getupdatedata()->turnserver[pos].password[passwordlen]='\0';
   }

extern "C" JNIEXPORT void JNICALL   fromjava(setTurnPort)(JNIEnv *env, jclass cl,jint pos,jint port) {
    backup->getupdatedata()->turnserver[pos].port=port;
    }
