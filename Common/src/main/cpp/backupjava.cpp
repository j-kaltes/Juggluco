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
    if(!backup) return false;
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
    if(!backup) {
        return 0;
    }
    return backup->    gethostnr();
    }

extern "C" JNIEXPORT jboolean  JNICALL   fromjava(detectIP)(JNIEnv *envin, jclass cl,jint pos) {
    if(!backup || pos<0 || pos>=backup->gethostnr()) return false;
    const passhost_t &host=backup->getupdatedata()->allhosts[pos];
    return host.detect;
    }
extern "C" JNIEXPORT jboolean  JNICALL   fromjava(getbackupHasHostname)(JNIEnv *envin, jclass cl,jint pos) {
    if(!backup || pos<0 || pos>=backup->gethostnr()) return false;
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


static int getposbylabel(const char *label) {
    if(!backup) return -1;
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
static bool removebylabel(const char *label) {
    if(!backup) return false;
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
static bool resetbylabel(const char *label,bool galaxy) {
    if(!backup) return false;
    int pos=getposbylabel(label);
    if(pos<0)
        return false;

    const passhost_t &host=backup->getupdatedata()->allhosts[pos];
    const int nr=host.nr;

    struct sockaddr_in6 ips[passhost_t::maxip];
    if(nr>0)
        memcpy(ips,host.ips,sizeof(ips));

    passhost_t *newhost=getwearoshost(true,label,galaxy,true);
    if(!newhost)
        return false;

    if(nr>0) {
        memcpy(newhost->ips,ips,sizeof(ips));
        newhost->nr=std::min(nr,passhost_t::maxip);
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
static const char *gethostlabel(int pos) {
    if(!backup||pos<0||pos>=backup->gethostnr())
        return nullptr;
    const passhost_t &host=backup->getupdatedata()->allhosts[pos];
    if(!host.hasname)
        return nullptr;
    return host.getname();
    }
static bool gethosttestip(int pos) {
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
extern "C" JNIEXPORT jobjectArray JNICALL fromjava(getMirrorHostEditState)(JNIEnv *env, jclass, jint pos) {
    if(!backup)
        return nullptr;
#ifndef TESTMENU
    const std::lock_guard<std::mutex> lock(change_host_mutex);
#endif
    mirrorhoststate state;
    if(!backup->getmirrorstate(pos,state)) {
        LOGGER("getMirrorHostEditState(%d) invalid\n",pos);
        return nullptr;
        }
    const jclass objectclass=env->FindClass("java/lang/Object");
    if(!objectclass)
        return nullptr;
    jobjectArray jall=env->NewObjectArray(mirrorstate_size,objectclass,nullptr);
    if(!jall)
        return nullptr;
    const jclass stringclass=env->FindClass("java/lang/String");
    const jclass integerclass=env->FindClass("java/lang/Integer");
    const jclass longclass=env->FindClass("java/lang/Long");
    const jclass booleanclass=env->FindClass("java/lang/Boolean");
    if(!stringclass||!integerclass||!longclass||!booleanclass)
        return nullptr;
    const jmethodID intinit=env->GetMethodID(integerclass,"<init>","(I)V");
    const jmethodID longinit=env->GetMethodID(longclass,"<init>","(J)V");
    const jmethodID boolinit=env->GetMethodID(booleanclass,"<init>","(Z)V");
    char portbuf[6];
    if(state.icelabel.empty())
        backup->getport(pos,portbuf);
    else
        strcpy(portbuf,"0");
    jobjectArray ipar=env->NewObjectArray(passhost_t::maxip,stringclass,nullptr);
    if(!ipar)
        return nullptr;
    for(int i=0;i<passhost_t::maxip;i++) {
        env->SetObjectArrayElement(ipar,i,env->NewStringUTF(state.ips[i].c_str()));
        }
    env->SetObjectArrayElement(jall,mirrorstate_label,env->NewStringUTF(state.label.c_str()));
    jboolean haslabel=state.haslabel;
    env->SetObjectArrayElement(jall,mirrorstate_haslabel,env->NewObject(booleanclass,boolinit,haslabel));
    env->SetObjectArrayElement(jall,mirrorstate_ips,ipar);
    env->SetObjectArrayElement(jall,mirrorstate_port,env->NewStringUTF(portbuf));
    jint receivefrom=state.receivefrom;
    env->SetObjectArrayElement(jall,mirrorstate_receivefrom,env->NewObject(integerclass,intinit,receivefrom));
    jint activereceive=state.activereceive;
    env->SetObjectArrayElement(jall,mirrorstate_activereceive,env->NewObject(integerclass,intinit,activereceive));
    jboolean sendnums=state.sendnums;
    env->SetObjectArrayElement(jall,mirrorstate_sendnums,env->NewObject(booleanclass,boolinit,sendnums));
    jboolean sendstream=state.sendstream;
    env->SetObjectArrayElement(jall,mirrorstate_sendstream,env->NewObject(booleanclass,boolinit,sendstream));
    jboolean sendscans=state.sendscans;
    env->SetObjectArrayElement(jall,mirrorstate_sendscans,env->NewObject(booleanclass,boolinit,sendscans));
    jboolean sendpassive=state.sendpassive;
    env->SetObjectArrayElement(jall,mirrorstate_sendpassive,env->NewObject(booleanclass,boolinit,sendpassive));
    jboolean restore=state.restore;
    env->SetObjectArrayElement(jall,mirrorstate_restore,env->NewObject(booleanclass,boolinit,restore));
    jlong starttime=state.starttime;
    env->SetObjectArrayElement(jall,mirrorstate_starttime,env->NewObject(longclass,longinit,starttime));
    jboolean detect=state.detect;
    env->SetObjectArrayElement(jall,mirrorstate_detect,env->NewObject(booleanclass,boolinit,detect));
    jboolean testip=state.testip;
    env->SetObjectArrayElement(jall,mirrorstate_testip,env->NewObject(booleanclass,boolinit,testip));
    jboolean hashostname=state.hashostname;
    env->SetObjectArrayElement(jall,mirrorstate_hostname,env->NewObject(booleanclass,boolinit,hashostname));
    env->SetObjectArrayElement(jall,mirrorstate_ice,env->NewStringUTF(state.icelabel.c_str()));
    jboolean side=state.side;
    env->SetObjectArrayElement(jall,mirrorstate_side,env->NewObject(booleanclass,boolinit,side));
    jint transport=state.transport;
    env->SetObjectArrayElement(jall,mirrorstate_transport,env->NewObject(integerclass,intinit,transport));
    jboolean bleclient=state.bleclient;
    env->SetObjectArrayElement(jall,mirrorstate_bleclient,env->NewObject(booleanclass,boolinit,bleclient));
    jboolean blereverse=state.blereverse;
    env->SetObjectArrayElement(jall,mirrorstate_blereverse,env->NewObject(booleanclass,boolinit,blereverse));
    jboolean bleunproven=state.bleunproven;
    env->SetObjectArrayElement(jall,mirrorstate_bleunproven,env->NewObject(booleanclass,boolinit,bleunproven));
    jboolean wearos=state.wearos;
    env->SetObjectArrayElement(jall,mirrorstate_wearos,env->NewObject(booleanclass,boolinit,wearos));
    jboolean deactivated=state.deactivated;
    env->SetObjectArrayElement(jall,mirrorstate_deactivated,env->NewObject(booleanclass,boolinit,deactivated));
    jboolean haspass=state.haspass;
    env->SetObjectArrayElement(jall,mirrorstate_haspass,env->NewObject(booleanclass,boolinit,haspass));
    return jall;
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
    if(!backup || pos<0 || pos>=backup->getupdatedata()->hostnr) 
        return 0;
    return backup->getupdatedata()->allhosts[pos].receivefrom;
    }
extern "C" JNIEXPORT jboolean JNICALL   fromjava(getbackuphostnums)(JNIEnv *envin, jclass cl,jint pos) {
    if(!backup || pos<0 || pos>=backup->getupdatedata()->hostnr) return false;
    int index=backup->getupdatedata()->allhosts[pos].index;
    if(index>=0)
        return  backup->getupdatedata()->tosend[index].sendnums;
    return false;
    }
extern "C" JNIEXPORT jboolean JNICALL   fromjava(getbackuphoststream)(JNIEnv *envin, jclass cl,jint pos) {
    if(!backup || pos<0 || pos>=backup->getupdatedata()->hostnr) return false;
    int index=backup->getupdatedata()->allhosts[pos].index;
    if(index>=0)
        return  backup->getupdatedata()->tosend[index].sendstream;
    return false;
    }
extern "C" JNIEXPORT jboolean JNICALL   fromjava(getbackuphostscans)(JNIEnv *envin, jclass cl,jint pos) {
    if(!backup || pos<0 || pos>=backup->getupdatedata()->hostnr) return false;
    int index=backup->getupdatedata()->allhosts[pos].index;
    if(index>=0)
        return  backup->getupdatedata()->tosend[index].sendscans;
    return false;
    }
extern "C" JNIEXPORT jint JNICALL   fromjava(setreceiveport)(JNIEnv *env, jclass cl,jstring jport) {
    if(!backup||!jport) return receiveport_status_nodigits;
    jint portlen= env->GetStringUTFLength( jport);
    jint jlen = env->GetStringLength( jport);
    if(portlen<1||portlen>5||jlen!=portlen) {
        LOGGER("setreceiveport invalid length %d/%d\n",portlen,jlen);
        return receiveport_status_nodigits;
        }
    char newport[6];
     env->GetStringUTFRegion( jport, 0,jlen, newport); 
    newport[portlen]='\0';
    int portnum=0;
    for(jint i=0;i<portlen;i++) {
        if(newport[i]<'0'||newport[i]>'9') {
            LOGGER("setreceiveport not a number len=%d\n",portlen);
            return receiveport_status_nodigits;
            }
        portnum=portnum*10+(newport[i]-'0');
        }
    if(portnum<1024||portnum>65535) {
        LOGGER("setreceiveport out of range %d\n",portnum);
        return receiveport_status_range;
        }
    LOGGER("setreceiveport %s\n",newport);
    if(backup->getupdatedata()->port[portlen]||memcmp(newport, backup->getupdatedata()->port,portlen)) {
        memcpy(backup->getupdatedata()->port,newport,portlen);
        backup->getupdatedata()->port[portlen]='\0';
        backup->startreceiver(true);
        }
    return receiveport_status_ok;
    }
extern "C" JNIEXPORT jstring JNICALL   fromjava(getreceiveport)(JNIEnv *env, jclass cl) {
    if(!backup) {
        return env->NewStringUTF(defaultport);
    }
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

extern "C" JNIEXPORT jint JNICALL   fromjava(changebackuphost)(JNIEnv *env, jclass cl,jint pos,jobjectArray jnames,jint nr,jboolean detect,jstring jport,jboolean nums,jboolean stream,jboolean scans,jboolean recover,jboolean receive,jboolean activeonly,jboolean passiveonly,jstring jpass,jlong starttime,jstring jlabel,jboolean testip,jboolean hashostname,jstring jICElabel,jboolean side,jint transport,jboolean bleclient) {
    if(!backup) return changehost_invalidindex;
#ifndef TESTMENU
    LOGAR("changebackuphost const std::lock_guard<std::mutex> lock(change_host_mutex)");
  const std::lock_guard<std::mutex> lock(change_host_mutex);
#endif
    LOGGER("changebackuphost(%d,%p,%d,%d,%p,%d,%d,%d,%d%,%d,%d,%d,%p,%ld,%p,%d,%d)\n", pos, jnames, nr, detect, jport, nums, stream, scans, recover, receive, activeonly, passiveonly, jpass, starttime, jlabel, testip, hashostname);
    char passbuf[passhost_t::maxpasslen+1];
    int passlen=0;
    if(jpass) {
        const jint passutf= env->GetStringUTFLength( jpass);
        const jint jpasslen = env->GetStringLength( jpass);
        if(passutf<0||passutf>passhost_t::maxpasslen||jpasslen!=passutf) {
            LOGGER("changebackuphost invalid password length %d/%d\n",passutf,jpasslen);
            return changehost_passwordlong;
            }
        env->GetStringUTFRegion( jpass, 0,jpasslen, passbuf); passbuf[passutf]='\0';
        passlen=passutf;
        }
    const char *label=jlabel?env->GetStringUTFChars( jlabel, NULL):nullptr;
    if(jlabel&&(!label||env->GetStringLength(jlabel)!=(jint)strlen(label))) {
        if(label)
            env->ReleaseStringUTFChars(jlabel,label);
        return changehost_labellong;
        }
     jint res;
     if(jICElabel) {
        const char *ICElabel=env->GetStringUTFChars( jICElabel, NULL);
        res=backup->changeICEhost(ICElabel,pos,nums,stream,scans,receive,
                std::string_view(passbuf,passlen),starttime,label,side,true,transport,bleclient);
        env->ReleaseStringUTFChars(jICElabel, ICElabel);
        }
     else {
        char port[6];
        int portlen=0;
        if(jport) {
            const jint portutf= env->GetStringUTFLength( jport);
            const jint jlen = env->GetStringLength( jport);
            if(portutf<0||portutf>5||jlen!=portutf) {
                LOGGER("changebackuphost invalid port length %d/%d\n",portutf,jlen);
                if(jlabel)
                    env->ReleaseStringUTFChars(jlabel,label);
                return changehost_invalidport;
                }
            env->GetStringUTFRegion( jport, 0,jlen, port); port[portutf]='\0';
            portlen=portutf;
            }
        const int arlen=jnames?std::min(env->GetArrayLength(jnames),nr):0;
        res=backup->changehost(pos,env,jnames,arlen,detect,std::string_view(port,portlen),nums,stream,scans,recover,receive,activeonly,std::string_view(passbuf,passlen),starttime,passiveonly,label,testip,true,hashostname,transport,bleclient);
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

extern "C" JNIEXPORT jint JNICALL   fromjava(patchbackuphost)(JNIEnv *env, jclass,jint pos,jint mask,jobjectArray jnames,jint nr,jstring jport,jint receivefrom,jboolean sendnums,jboolean sendstream,jboolean sendscans,jstring jlabel,jstring jpassword,jint passaction) {
    if(!backup) return changehost_invalidindex;
#ifndef TESTMENU
    const std::lock_guard<std::mutex> lock(change_host_mutex);
#endif
    LOGGER("patchbackuphost(%d,%d,%p,%d,%p,%d,%d,%d,%d,%p,%p,%d)\n",pos,mask,jnames,nr,jport,receivefrom,
            sendnums,sendstream,sendscans,jlabel,jpassword,passaction);
    char portbuf[6]{};
    int portlen=0;
    if(jport) {
        const jint portutf= env->GetStringUTFLength( jport);
        const jint jlen = env->GetStringLength( jport);
        if(portutf<1||portutf>5||jlen!=portutf) {
            LOGGER("patchbackuphost invalid port length %d/%d\n",portutf,jlen);
            return changehost_invalidport;
            }
        env->GetStringUTFRegion( jport, 0,jlen, portbuf); portbuf[portutf]='\0';
        portlen=portutf;
        }
    char passbuf[passhost_t::maxpasslen+1]{};
    int passlen=0;
    if(jpassword) {
        const jint passutf= env->GetStringUTFLength( jpassword);
        const jint jpasslen = env->GetStringLength( jpassword);
        if(passutf<0||passutf>passhost_t::maxpasslen||jpasslen!=passutf) {
            LOGGER("patchbackuphost invalid password length %d/%d\n",passutf,jpasslen);
            return changehost_passwordlong;
            }
        env->GetStringUTFRegion( jpassword, 0,jpasslen, passbuf); passbuf[passutf]='\0';
        passlen=passutf;
        }
    const char *label=jlabel?env->GetStringUTFChars( jlabel, NULL):nullptr;
    destruct   dest([&]{ if(jlabel&&label) env->ReleaseStringUTFChars(jlabel,label); });
    if(jlabel&&(!label||env->GetStringLength(jlabel)!=(jint)strlen(label))) {
        LOGAR("patchbackuphost invalid label");
        return changehost_labellong;
        }
    const int arlen=jnames?std::min(env->GetArrayLength(jnames),nr):0;
    return backup->patchhost(pos,env,jnames,arlen,mask,std::string_view(portbuf,portlen),receivefrom,
            sendnums,sendstream,sendscans,label?std::string_view(label,strlen(label)):std::string_view(),
            std::string_view(passbuf,passlen),passaction);
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
    if(!backup) return false;
    return backup->isreceiving() ;
    }
extern "C" JNIEXPORT void JNICALL   fromjava(deletebackuphost)(JNIEnv *env, jclass cl,jint pos) {
    if(!backup || pos<0 || pos>=backup->gethostnr()) return;
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
    if(!backup || pos<0 || pos>=backup->gethostnr()) return;
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
     if(backup) {
         backup->wakebackup(wakeUpSwitch|wakeall);
         backup->getupdatedata()->wakesender(wakeUpSwitch|wakeall);
     }
    }

//void wakebackup(myuintptr_t kind=wakeall,bool sendwake=false){
extern void resetnetwork();

static void resetnetworkthread(bool *useresetnetwork) {
    LOGSTRING("resetnetwork\n");
    if(backup) {
        backup->closeallsocks();
        backup->getupdatedata()->wakesender();
        networkpresent=true;
        backup->notupdatedsettings();
        }
    *useresetnetwork=true;
    }
extern void resetnetwork();
void resetnetwork() {
    static bool useresetnetwork=true;
    if(useresetnetwork) {
        useresetnetwork=false;
        std::thread th(resetnetworkthread,&useresetnetwork);
        th.detach();
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
    if(backup) {
        backup->endAllConnections();
    }
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
    if(!backup || pos<0 || pos>=backup->getupdatedata()->hostnr) {
        return true;
        }
    return backup->getupdatedata()->allhosts[pos].deactivated;
    }
extern "C" JNIEXPORT void JNICALL   fromjava(setHostDeactivated)(JNIEnv *envin, jclass cl,jint pos,jboolean val) {
    if(backup) backup->deactivateHost(pos,val);
    }



#if !defined(WEAROS)&& __NDK_MAJOR__ >= 26
extern std::string mkbackjson(int pos);
extern "C" JNIEXPORT jstring JNICALL   fromjava(getbackJson)(JNIEnv *envin, jclass cl,jint pos) {
    if(!backup) // networking not started yet
        return myNewStringUTF(envin,"");
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
