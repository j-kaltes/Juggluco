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


#include <jni.h>
#include "config.h"
#include "fromjava.h"
#ifdef WEAROS_MESSAGES
#include <android/log.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <cstring>
#include <cerrno>
#include <vector>
#include <array>
#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>
#include <semaphore>
#include "destruct.hpp"
#include "datbackup.hpp"
#include "net/netstuff.hpp"
#include "net/TCPConnect.hpp"
/*
struct wearmessage {
   int32_t len;
   int16_t phonehostnr;
   bool phonesender;
   uint8_t reserved;
   uint8_t data[];
};
*/

#define LOGGERTAG(...) LOGGER("messages: " __VA_ARGS__)
#define LOGARTAG(...) LOGAR("messages: " __VA_ARGS__)
#define LOGSTRINGTAG(...) LOGSTRING("messages: " __VA_ARGS__)
struct wearmessagetype{
   int16_t phonehostnr;
   bool phonesender;
   uint8_t reserved;
};
static_assert(sizeof(wearmessagetype)==4);
struct wearmessage {
   int32_t len;
   wearmessagetype type;
   uint8_t data[];
};
extern std::array<std::atomic_int,maxallhosts> peers2us,us2peers;
extern std::atomic_bool messagephonepeer[];

std::array<std::atomic_int,maxallhosts> messagesendersockets;
std::array<std::atomic_int,maxallhosts> messagereceiversockets;
static std::array<uint64_t,maxallhosts> messagesendergenerations{};
static std::array<uint64_t,maxallhosts> messagereceivergenerations{};
static std::array<std::atomic_bool,maxallhosts> messagereceiverrunning{};
// Automatic transport intent that has requested Messages/Direct BLE but has not
// yet established that alternate carrier.  Keep this separate from wearmessages[]:
// the latter means the alternate carrier is actually selected and usable.
std::array<std::atomic_bool,maxallhosts> mirrorfallbackrequested{};
// True only while Direct Bluetooth/BLE GATT is the selected data carrier.
// Keep this separate from wearmessages[]: that flag is shared by both the
// BLE-backed message bridge and Wear OS MessageClient.
std::array<std::atomic_bool,maxallhosts> mirrordirectbluetoothactive{};
// Distinguish a BLE-backed message socket from the established Google
// MessageClient fallback. Both use wearmessages[], but changing away from
// Direct Bluetooth must tear down the BLE bridge before Automatic/TCP resumes.
static std::array<std::atomic_bool,maxallhosts> directbluetoothmode{};
static std::mutex messagesocketmutex;

void setall(auto &ar,const auto ini) {    
    for(auto &el:ar)
        el=ini;
    }
static bool initconarray() {
    setall(peers2us,-1);
    setall(us2peers,-1);
    setall(messagesendersockets,-1);
    setall(messagereceiversockets,-1);
    return true;
    }
static bool _coninitted=initconarray();

static uint64_t installmessagesocket(std::array<std::atomic_int,maxallhosts> &sockets,
                                     std::array<uint64_t,maxallhosts> &generations,
                                     int host,int sock) {
    int oldsock=-1;
    uint64_t generation;
    {
        const std::lock_guard<std::mutex> lock(messagesocketmutex);
        oldsock=sockets[host];
        sockets[host]=sock;
        generation=++generations[host];
    }
    if(oldsock>=0&&oldsock!=sock)
        shutdown(oldsock,SHUT_RDWR);
    return generation;
    }

static void stopmessagesocket(std::array<std::atomic_int,maxallhosts> &sockets,
                              std::array<uint64_t,maxallhosts> &generations,
                              int host) {
    int sock=-1;
    {
        const std::lock_guard<std::mutex> lock(messagesocketmutex);
        sock=sockets[host];
        sockets[host]=-1;
        ++generations[host];
    }
    if(sock>=0)
        shutdown(sock,SHUT_RDWR);
    }

static void clearmessagesocket(std::array<std::atomic_int,maxallhosts> &sockets,
                               const std::array<uint64_t,maxallhosts> &generations,
                               int host,int sock,uint64_t generation) {
    const std::lock_guard<std::mutex> lock(messagesocketmutex);
    if(generations[host]==generation&&sockets[host]==sock)
        sockets[host]=-1;
    }

static int duplicatemessagesocket(const std::array<std::atomic_int,maxallhosts> &sockets,int host) {
    const std::lock_guard<std::mutex> lock(messagesocketmutex);
    const int sock=sockets[host];
    return sock<0?-1:dup(sock);
    }

int toreceiversocket(const wearmessage *mess) {
if(mess->type.phonehostnr<0||mess->type.phonehostnr>=maxallhosts) {
    LOGGERTAG("toreceiversocket invalid phonehostnr=%d\n",mess->type.phonehostnr);
    return -1;
    }
#ifdef WEAROS
const    int host=peers2us[mess->type.phonehostnr];
if(host<0||host>=maxallhosts) {
    LOGGERTAG("toreceiversocket host=%d\n",host);
    return -1;
    }
int sock;
   if(mess->type.phonesender) {
    sock= duplicatemessagesocket(messagereceiversockets,host);
    LOGGERTAG("toreceiversocket host=%d receiver sock=%d\n",host,sock);
       }
else {
    sock= duplicatemessagesocket(messagesendersockets,host);
    LOGGERTAG("toreceiversocket host=%d sender sock=%d\n",host,sock);
    }
#else
 int host=mess->type.phonehostnr;
 const bool phonepeer=mess->type.reserved&1;
 if(phonepeer)
    host=peers2us[mess->type.phonehostnr];
 if(host<0||host>=maxallhosts) {
    LOGGERTAG("toreceiversocket invalid mapped host=%d remote=%d\n",host,mess->type.phonehostnr);
    return -1;
    }
 int sock;
   if(mess->type.phonesender) {
       sock= phonepeer?duplicatemessagesocket(messagereceiversockets,host):
                       duplicatemessagesocket(messagesendersockets,host);
    LOGGERTAG("toreceiversocket host=%d %s sock=%d\n",host,phonepeer?"receiver":"sender",sock);
    }
else {
   sock= phonepeer?duplicatemessagesocket(messagesendersockets,host):
                   duplicatemessagesocket(messagereceiversockets,host);
   LOGGERTAG("toreceiversocket host=%d %s sock=%d\n",host,phonepeer?"sender":"receiver",sock);
   }
#endif

return sock;
    }
bool receivemessage(wearmessage *pWearmessage) {
       LOGGERTAG("type.phonehostnr=%d type.phonesender=%d %p#%d\n",pWearmessage->type.phonehostnr,pWearmessage->type.phonesender,(char *)pWearmessage->data,pWearmessage->len);
       const int sock=toreceiversocket(pWearmessage);
       if(sock<0)
           return false;
       const bool success=sendni(sock,pWearmessage->data,pWearmessage->len)>=0;
       sockclose(sock);
       return success;
       }

// A Direct-Bluetooth GATT connection is already authenticated against one
// local mirror row. Route by that row instead of the peer's numeric row. Peer
// row numbers are only local database details and can legitimately collide
// when several phones (for example phone + Boox) both use row zero.
static bool receivemessageformirror(wearmessage *mess,int host) {
    if(!backup||host<0||host>=backup->gethostnr()||host>=maxallhosts) {
        LOGGERTAG("receivemessageformirror invalid host=%d\n",host);
        return false;
        }
    const bool receiverSocket=
#ifdef WEAROS
        mess->type.phonesender;
#else
        mess->type.phonesender==messagephonepeer[host].load();
#endif
    const int sock=receiverSocket?
            duplicatemessagesocket(messagereceiversockets,host):
            duplicatemessagesocket(messagesendersockets,host);
    LOGGERTAG("receivemessageformirror host=%d phonepeer=%d phonesender=%d %s sock=%d\n",
            host,messagephonepeer[host].load(),mess->type.phonesender,
            receiverSocket?"receiver":"sender",sock);
    if(sock<0)
        return false;
    const bool success=sendni(sock,mess->data,mess->len)>=0;
    sockclose(sock);
    return success;
    }

static jboolean receivejavamessage(JNIEnv *env,jbyteArray data,int directHost) {
    if(!data)
        return false;
    const int arlen=env->GetArrayLength(data);
    const int datlen=arlen-static_cast<int>(sizeof(wearmessage::type));
    if(datlen<=0) {
        LOGGERTAG("message: arlen=%d\n",arlen);
        return false;
        }
    std::vector<uint8_t> weardata(datlen+sizeof(wearmessage));
    auto *mess=reinterpret_cast<wearmessage*>(weardata.data());
    env->GetByteArrayRegion(data,0,arlen,reinterpret_cast<jbyte*>(&mess->type));
    if(env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
        }
    mess->len=datlen;
    return directHost<0?receivemessage(mess):receivemessageformirror(mess,directHost);
    }

extern "C"
JNIEXPORT jboolean JNICALL fromjava(message)(JNIEnv *env, jclass thiz, jbyteArray data) {
        return receivejavamessage(env,data,-1);
    }

extern "C"
JNIEXPORT jboolean JNICALL fromjava(messageForMirror)(JNIEnv *env,jclass,jint localIndex,jbyteArray data) {
        return receivejavamessage(env,data,localIndex);
    }
jclass jMessageSender =nullptr;
//jmethodID jsendDatawithName=nullptr;
//jmethodID jsendData=nullptr;
//jmethodID jsendNameMessageOn=nullptr;
//jmethodID jsendMessageOn=nullptr;
extern JavaVM* vmptr;
bool jinitmessages(JNIEnv* env) {
    constexpr const char classname[]="tk/glucodata/MessageSender";
    jclass cl=env->FindClass(classname);
if(!cl) {
        LOGGERTAG("Can't find %s\n",classname);
        return false;
    }
else {
        jMessageSender = (jclass)env->NewGlobalRef(cl);
        env->DeleteLocalRef(cl);
    /*
#ifdef WEAROS
       if(!(jsendData=env->GetStaticMethodID(jMessageSender,"sendData","([B)Z"))) {
                LOGSTRINGTAG("GetStaticMethodID(jMessageSender,\"sendData\",\"([B)Z\" failed\n");
                return false;
                } 
        if(!(jsendMessageOn=env->GetStaticMethodID(jMessageSender,"sendMessageOn","(Z)V"))) {
                LOGSTRINGTAG("GetStaticMethodID(jMessageSender,\"sendMessageOn\",\"(Z)V\" failed\n");
                return false;
                } 
#else    
        if(!(jsendDatawithName=env->GetStaticMethodID(jMessageSender,"sendDatawithName","(Ljava/lang/String;[B)Z"))) {
                LOGSTRINGTAG("GetStaticMethodID(jMessageSender,\"sendDatawithName\",\"(Ljava/lang/String;[B)Z\" failed\n");
                return false;
                }

        if(!(jsendNameMessageOn=env->GetStaticMethodID(jMessageSender,"sendNameMessageOn","(Ljava/lang/String;Z)V"))) {
                LOGSTRINGTAG("GetStaticMethodID(jMessageSender,\"sendNameMessageOn\",\"(Ljava/lang/String;Z)V\" failed\n");
                return false;
                }  
#endif
*/
        }
    return true;
}
extern JNIEnv *getenv();
extern bool sendMessagesON(passhost_t *pass,bool val);
extern void setBlueMessage(int,bool val);
jstring getconnectionname(JNIEnv *env, int phonehostnr) {
    if(phonehostnr<0||phonehostnr>=maxallhosts||!backup)
        return nullptr;
    const auto hosts=getBackupHosts();
    if(static_cast<size_t>(phonehostnr)>=hosts.size())
        return nullptr;
    const auto &host=hosts[phonehostnr];
    if(!host.hasname) {
        LOGGERTAG("sendmessage %d noname\n",phonehostnr);
        return nullptr;
        }
    return env->NewStringUTF(host.getname());
    }
bool sendMessagesON(passhost_t *pass, bool val) {
    if(!backup||!pass||!jMessageSender)
        return false;
    const auto hosts=getBackupHosts();
    const int index=pass-hosts.data();
    if(index<0||static_cast<size_t>(index)>=hosts.size()||index>=maxallhosts)
        return false;
    if(pass->automatictransport())
        mirrorfallbackrequested[index]=val;
    auto env=getenv();
    bool delivered=false;
#ifdef WEAROS

    static auto jsendMessageOn=env->GetStaticMethodID(jMessageSender,"sendMessageOn","(Z)Z");
    if(jsendMessageOn)
        delivered=env->CallStaticBooleanMethod(jMessageSender,jsendMessageOn,val);
#else
    jstring jname=getconnectionname(env, index);
    if(!jname) {
        if(env->ExceptionCheck())
            env->ExceptionClear();
        return false;
        }

    static auto jsendNameMessageOn=env->GetStaticMethodID(jMessageSender,"sendNameMessageOn","(Ljava/lang/String;Z)Z");
    if(jsendNameMessageOn)
        delivered=env->CallStaticBooleanMethod(jMessageSender,jsendNameMessageOn,jname,val);
    env->DeleteLocalRef(jname);
#endif
    if(env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        delivered=false;
        }
    if(delivered)
        setBlueMessage(index,val);
    else
        LOGGERTAG("sendMessagesON(%d,%d) was not delivered\n",index,val);
    return delivered;
    }

bool    sendmessage(const int phonehostnr,bool phonesender,const uint8_t *buf,const int inlen) {
    if(phonehostnr<0||phonehostnr>=maxallhosts||!buf||inlen<=0||!jMessageSender)
        return false;
    auto env=getenv();
       int totlen=inlen+sizeof(wearmessage)-sizeof(wearmessage::len);
    LOGGERTAG("start sendmessage(%d,%d,%p#%d) totlen=%d\n",phonehostnr,phonesender,buf,inlen,totlen);
    jbyteArray uit=env->NewByteArray(totlen);
    if(!uit) {
        if(env->ExceptionCheck())
            env->ExceptionClear();
        return false;
        }
    LOGARTAG("after env->NewByteArray(totlen)");
    int start=offsetof(wearmessage,type);
    int offdata=offsetof(wearmessage,data);

    const bool senderIndex=
#ifdef WEAROS
        false;
#else
        messagephonepeer[phonehostnr].load();
#endif
    wearmessagetype type{.phonehostnr=static_cast<int16_t>(phonehostnr),.phonesender=phonesender,
                         .reserved=static_cast<uint8_t>(senderIndex)};
        env->SetByteArrayRegion(uit, 0, sizeof(wearmessagetype),(const jbyte *)&type);
        env->SetByteArrayRegion(uit, offdata-start, inlen,(const jbyte *)buf);
#ifdef WEAROS
        bool res=false;
        if(directbluetoothmode[phonehostnr].load()) {
            jstring jname=getconnectionname(env,phonehostnr);
            static auto jsendDatawithName=env->GetStaticMethodID(jMessageSender,"sendDatawithName","(Ljava/lang/String;[B)Z");
            res=jname&&jsendDatawithName&&env->CallStaticBooleanMethod(jMessageSender,jsendDatawithName,jname,uit);
            if(jname)
                env->DeleteLocalRef(jname);
            }
        else {
            static auto jsendData=env->GetStaticMethodID(jMessageSender,"sendData","([B)Z");
            res=jsendData&&env->CallStaticBooleanMethod(jMessageSender,jsendData,uit);
            }
#else
    jstring  jname= getconnectionname(env,phonehostnr);

        static auto jsendDatawithName=env->GetStaticMethodID(jMessageSender,"sendDatawithName","(Ljava/lang/String;[B)Z");
        bool res=jname&&jsendDatawithName&&env->CallStaticBooleanMethod(jMessageSender,jsendDatawithName,jname,uit);
        if(jname)
            env->DeleteLocalRef(jname);
#endif
        if(env->ExceptionCheck()) {
            env->ExceptionDescribe();
            env->ExceptionClear();
            res=false;
            }
        env->DeleteLocalRef(uit);
    LOGGERTAG("end sendmessage res=%d\n",res);
    return res;
    }
void clearnetworkcache() {
    // Connection names are now short-lived local JNI references.
    }
struct bridgestart {
    std::binary_semaphore ready{0};
    bool success=false;
    };

void tobluetooth(int hostnr,bool sender,int sock,uint64_t generation,std::shared_ptr<bridgestart> started) {
       LOGGERTAG("tobluetooth(%d,%d,%d)\n", hostnr, sender, sock);

    auto &registeredsockets=sender?messagesendersockets:messagereceiversockets;
    const auto &generations=sender?messagesendergenerations:messagereceivergenerations;
    bool startreported=false;
    destruct reportstart([started,&startreported]() {
        if(!startreported)
            started->ready.release();
        });

    #ifdef WEAROS
    int phonehost=hostnr;
    if(!directbluetoothmode[hostnr].load()) {
        phonehost=us2peers[hostnr];
        constexpr int mappingwaits=100;
        for(int wait=0;phonehost<0&&wait<mappingwaits&&wearmessages[hostnr];++wait) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            phonehost=us2peers[hostnr];
            }
        if(phonehost<0||phonehost>=maxallhosts) {
            LOGGERTAG("tobluetooth no peer mapping for host=%d (peer=%d)\n",hostnr,phonehost);
            clearmessagesocket(registeredsockets,generations,hostnr,sock,generation);
            shutdown(sock,SHUT_RDWR);
            sockclose(sock);
            return;
            }
        }
    #else
    const int phonehost=hostnr;
    #endif
    const bool phonesender=
    #ifdef WEAROS
        !sender
    #else
        sender    
    #endif
    ;
     int maxbuf= sysconf(_SC_PAGESIZE)*20;
    uint8_t *buf =reinterpret_cast<uint8_t *>(mmap(NULL, maxbuf, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS , -1, 0));
    if(buf==MAP_FAILED) {
        lerror("tobluetooth: mmap failed");
        clearmessagesocket(registeredsockets,generations,hostnr,sock,generation);
        shutdown(sock,SHUT_RDWR);
        sockclose(sock);
        return;
        }
   destruct _dest([maxbuf,buf]() { munmap(buf,maxbuf); });
    {
    char buf[30]; 
#ifndef NOLOG
    int len=
#endif
             sprintf(buf, "tobluetooth %d %s",hostnr,sender?"S":"R");
    prctl(PR_SET_NAME, buf, 0, 0, 0);
    LOGGERN(buf,len);
    }
  started->success=true;
  startreported=true;
  started->ready.release();
  auto &status=mirrorstatus[hostnr].toblue[sender];
  status.running(true);
  destruct stopstatus([&status](){status.running(false);});
   while(true) { 
     const int inlen=recvni(sock,buf,maxbuf);
     LOGGERTAG("tobluetooth recvni(%d,...)=%d\n",sock,inlen);
     if(inlen<=0||((status.recv=true)&&!(status.sendmessage=sendmessage(phonehost,phonesender,buf,inlen))))  {
         if(inlen>0)
            LOGGERTAG("sendmessage failed %d %d #%d\n",phonehost,phonesender,inlen);
         clearmessagesocket(registeredsockets,generations,hostnr,sock,generation);
         shutdown(sock,SHUT_RDWR);
         sockclose(sock);
         LOGGERTAG("%d %d Return from thread\n",hostnr,sender);
         return;
       }
        }
    }
    /*
void closesock(int &sock) {
    int tmpsock=sock;
    if(tmpsock!=-1) {
        sock=-1;
        shutdown(tmpsock,SHUT_RDWR);
        sockclose(tmpsock);
        }
    }
    */
//extern    bool    getcommandsnopass(int sock,passhost_t *host); //TODO password?
extern bool    getcommands(int,passhost_t *);
extern    void receiversockopt(int new_fd);

static void messagereceivecommands(passhost_t *pass) {
    const int index=pass-getBackupHosts().data();
    if(messagereceiverrunning[index].exchange(true)) {
        LOGGERTAG("messagereceivecommands %d already running\n",index);
        return;
        }
    destruct runningguard([index](){messagereceiverrunning[index]=false;});
    LOGGERTAG("messagereceivecommands %d start\n",index);
    stopmessagesocket(messagereceiversockets,messagereceivergenerations,index);
    for(int i=0;wearmessages[index];i++) {
        int sockpair[2];
        if(socketpair(AF_LOCAL,SOCK_STREAM,0,sockpair)!=0) {
            lerror("socketpair");
            return ; 
            }
        {
        char buf[24];
#ifndef NOLOG
        int len=
#endif
                 sprintf(buf,"%d message %d",i,index);
        prctl(PR_SET_NAME, buf, 0, 0, 0);
        LOGGERN(buf,len);
        }
        auto connect=getconnectionas<TCPConnect>(index);
        if(!connect) {
            sockclose(sockpair[0]);
            sockclose(sockpair[1]);
            return;
            }

        const uint64_t generation=installmessagesocket(messagereceiversockets,messagereceivergenerations,index,sockpair[0]);
        auto started=std::make_shared<bridgestart>();
        std::thread th(tobluetooth,index,false,sockpair[0],generation,started);
        started->ready.acquire();
        if(!started->success) {
            sockclose(sockpair[1]);
            th.join();
            continue;
            }
/*
        int &recsock=hostsocks[index];
        if(recsock!=-1)
            closesock(recsock);

        recsock= sockpair[1]; */
        int recsock= sockpair[1]; 
        connect->setReceiverSock(recsock);
        receiversockopt(recsock);

#ifdef ENCRYPTMESSAGES 
        LOGARTAG("Encrypt getcommands");
    
        connect->getcommands
#else
        connect->getcommandsnopass
#endif
        (pass);

        LOGGERTAG("%d message join\n",index);
        stopmessagesocket(messagereceiversockets,messagereceivergenerations,index);
        th.join();
        connect=getconnectionas<TCPConnect>(index);
        if(!connect) {
            LOGGER("not connection %d\n",index);
            return;
            }

        connect->closeReceiverConnection();

        LOGARTAG("try again");
         }
    LOGGERTAG("messagereceivecommands wearmessages[%d]==false\n",index);
    return;
    }
void startmessagereceiver(passhost_t &host) {
    LOGGERTAG("startmessagereceiver %s\n",host.getname());
    std::thread th(messagereceivecommands,&host);
    th.detach();
    }
        /*
void startmessagereceivers(Backup *backup) {
    LOGSTRINGTAG("startmessagereceivers\n");
    auto hspan=backup->getHosts();
    for(passhost_t &host:hspan) {
        if(host.wearos) {
            startmessagereceiver(host);
            }
        }
    } */


//extern void   sendpassinit(int sock,passhost_t *host,crypt_t *ctx);
int messagemakeconnection(passhost_t *pass,int &sock,crypt_t*ctx,char stype) {
    const int index=pass-getBackupHosts().data();
    stopmessagesocket(messagesendersockets,messagesendergenerations,index);
    int sockpair[2];
    if(socketpair(AF_LOCAL,SOCK_STREAM,0,sockpair)!=0) {
        lerror("messagemakeconnection socketpair");
        sock=-1;
        return -1;
        }
    sock=sockpair[0];
    const uint64_t generation=installmessagesocket(messagesendersockets,messagesendergenerations,index,sockpair[1]);
    auto started=std::make_shared<bridgestart>();
    std::thread th(tobluetooth,index,true,sockpair[1],generation,started);
    th.detach();
    started->ready.acquire();
    if(!started->success) {
        sockclose(sockpair[0]);
        sock=-1;
        return -1;
        }
    LOGGERTAG("messagemakeconnection %s sock=%d (other end=%d)\n",pass->getname(),sock,sockpair[1]);
/*
    if(!pass->sendpassive)  {
         if(ctx) {
             sendpassinit(sock,pass,ctx);
             }
           } */
    return sock;
    }


#endif
//bool wearmessages=false;


extern int getwearindex(JNIEnv *env, jstring jident) ;
extern "C" JNIEXPORT jboolean  JNICALL   fromjava(getBlueMessage)(JNIEnv *env, jclass cl,int index) {
    if(index<0||index>=maxallhosts)
        return false;
    const bool enabled=wearmessages[index].load();
    LOGGERTAG("getBluemessage(%d)=%d\n",index,enabled);
    return enabled;
    }
/*
extern "C" JNIEXPORT jboolean  JNICALL   fromjava(getBlueMessage)(JNIEnv *env, jclass cl,jstring jident) {
    int index=getwearindex(env,jident);
    if(index<0)
        return false;
    const bool enabled=wearmessages[index].load();
    LOGGERTAG("getBluemessage(%d)=%d\n",index,enabled);
    return enabled;
    }
    *
void closesocks(std::array<int,maxallhosts>   &socks) {
    for(auto &el:socks) {
        if(el!=-1) {
            closesock(el);
            }
        }
    }
    */
extern void startmessagereceivers(Backup*);

extern void startmessagereceiver(passhost_t &host);
extern void startactivereceivers();
extern bool mirrorLocalTcpAvailable();
static void wakemessagehost(passhost_t &host) {
    if(host.index>=0&&host.index<backup->con_vars.size()&&backup->con_vars[host.index])
        backup->con_vars[host.index]->wakebackup(wakereconnect|wakeall);
    if(host.activereceive) {
        const int activeindex=host.activereceive-1;
        if(activeindex>=0&&activeindex<active_receivenr&&active_receive[activeindex])
            active_receive[activeindex]->wakebackup(wakereconnect|wakeall);
        }
    }
void setBlueMessage(int ident,bool val) {
    if(ident<0||ident>=maxallhosts||!backup)
        return;
    mirrorfallbackrequested[ident]=false;
    passhost_t &host= getBackupHosts()[ident];
    LOGGERTAG("setBlueMessage(%s(%d),%d) transport=%d\n",host.getnameif(),ident,val,host.gettransport());
    if(host.deactivated&&val)
        return;
    const bool changed=wearmessages[ident].exchange(val)!=val;
    if(changed) {
        backup->closesocksone(ident);
        if(!val) {
            stopmessagesocket(messagesendersockets,messagesendergenerations,ident);
            stopmessagesocket(messagereceiversockets,messagereceivergenerations,ident);
            }
        else {
            startmessagereceiver(host);
            }
        wakemessagehost(host);
        }
    else if(val&&!messagereceiverrunning[ident].load()) {
        // /messages is also a periodic carrier confirmation.  If the bridge
        // thread died while the carrier flag stayed true, use that confirmation
        // as a self-heal instead of waiting for Reinit to rebuild the sockets.
        LOGGERTAG("MessageClient carrier still selected for %s(%d), but receiver bridge is not running; restarting it\n",
                host.getnameif(),ident);
        startmessagereceiver(host);
        wakemessagehost(host);
        }
    }

void applyConfiguredMirrorTransports() {
    if(!backup)
        return;
    auto hosts=backup->getHosts();
    LOGGERTAG("applyConfiguredMirrorTransports hostnr=%zu\n",hosts.size());
    bool needtcpreceiver=false;
    for(auto &host:hosts) {
        const int index=&host-hosts.data();
        if(!host.deactivated&&host.usesnetworktransport()&&!host.ICE&&host.passive())
            needtcpreceiver=true;
        if(host.deactivated) {
            directbluetoothmode[index]=false;
            mirrordirectbluetoothactive[index]=false;
            us2peers[index]=-1;
            messagephonepeer[index]=false;
            setBlueMessage(index,false);
            continue;
            }
        const auto transport=host.gettransport();
        const bool enteringDirect=transport==passhost_t::transport_bluetooth&&
                !directbluetoothmode[index].exchange(true);
        const bool leavingDirect=transport!=passhost_t::transport_bluetooth&&
                directbluetoothmode[index].exchange(false);
        if(enteringDirect||leavingDirect) {
            LOGGERTAG("mirror %s(%d): %s Direct Bluetooth; resetting carrier bridge\n",
                    host.getnameif(),index,enteringDirect?"entering":"leaving");
            us2peers[index]=-1;
            messagephonepeer[index]=false;
            setBlueMessage(index,false);
            }
        if(transport==passhost_t::transport_tcp||transport==passhost_t::transport_messages)
            mirrordirectbluetoothactive[index]=false;
        if(transport==passhost_t::transport_tcp) {
            us2peers[index]=-1;
            messagephonepeer[index]=false;
            setBlueMessage(index,false);
            }
        else if(transport==passhost_t::transport_automatic) {
            // Runtime state is intentionally retained.  Automatic starts on
            // TCP after process start, falls back through the legacy
            // /messages exchange, and can later retry TCP.
            LOGGERTAG("automatic mirror %s(%d): current carrier=%s\n",host.getnameif(),index,
                    wearmessages[index].load()?"Messages":"TCP/IP");
            }
        else {
            setBlueMessage(index,true);
            }
        if(wearmessages[index]&&transport!=passhost_t::transport_bluetooth) {
            if(!messagereceiverrunning[index])
                startmessagereceiver(host);
            wakemessagehost(host);
            }
        }
    for(int index=static_cast<int>(hosts.size());index<maxallhosts;++index) {
        directbluetoothmode[index]=false;
        mirrordirectbluetoothactive[index]=false;
        wearmessages[index]=false;
        mirrorfallbackrequested[index]=false;
        peers2us[index]=-1;
        us2peers[index]=-1;
        messagephonepeer[index]=false;
        stopmessagesocket(messagesendersockets,messagesendergenerations,index);
        stopmessagesocket(messagereceiversockets,messagereceivergenerations,index);
        }
    if(needtcpreceiver)
        backup->startreceiver(false);
    else
        ::stopreceiver();
    }

extern "C" JNIEXPORT void JNICALL fromjava(applyMirrorTransports)(JNIEnv *,jclass) {
    applyConfiguredMirrorTransports();
    }

extern "C" JNIEXPORT void JNICALL fromjava(mirrorTransportReady)(JNIEnv *,jclass,jint localIndex,jint remoteIndex,jboolean phonePeer) {
    if(!backup||localIndex<0||localIndex>=backup->gethostnr()||remoteIndex<0||remoteIndex>=maxallhosts) {
        LOGGERTAG("mirrorTransportReady invalid local=%d remote=%d\n",localIndex,remoteIndex);
        return;
    }
    auto &host=backup->getHosts()[localIndex];
    const auto transport=host.gettransport();
    if(host.deactivated||(transport!=passhost_t::transport_bluetooth&&
            transport!=passhost_t::transport_automatic)) {
        LOGGERTAG("mirrorTransportReady ignored local=%d transport=%d deactivated=%d\n",localIndex,host.gettransport(),host.deactivated);
        return;
        }
    for(auto &mapped:peers2us) {
        int expected=localIndex;
        mapped.compare_exchange_strong(expected,-1);
        }
    peers2us[remoteIndex]=localIndex;
    us2peers[localIndex]=remoteIndex;
    messagephonepeer[localIndex]=phonePeer;
    LOGGERTAG("mirrorTransportReady local=%d remote=%d phonePeer=%d transport=%d carrier=%s\n",
            localIndex,remoteIndex,phonePeer,transport,
            transport==passhost_t::transport_automatic?"automatic Bluetooth fallback":"Direct Bluetooth");
    mirrordirectbluetoothactive[localIndex]=true;
    setBlueMessage(localIndex,true);
    backup->closesocksone(localIndex);
    if(!messagereceiverrunning[localIndex])
        startmessagereceiver(host);
    wakemessagehost(host);
    }

extern "C" JNIEXPORT void JNICALL fromjava(mirrorTransportDisconnected)(JNIEnv *,jclass,jint localIndex,jint remoteIndex) {
    if(!backup||localIndex<0||localIndex>=maxallhosts)
        return;
    const auto transport=localIndex<backup->gethostnr()?
            getBackupHosts()[localIndex].gettransport():passhost_t::transport_tcp;
    const bool stilldirect=transport==passhost_t::transport_bluetooth;
    const bool wasautomatic=transport==passhost_t::transport_automatic;
    mirrordirectbluetoothactive[localIndex]=false;
    LOGGERTAG("mirrorTransportDisconnected local=%d remote=%d stilldirect=%d automatic=%d\n",
            localIndex,remoteIndex,stilldirect,wasautomatic);
    bool preserveWearMessageMapping=false;
#ifdef WEAROS
    // On the watch the same peer-index mapping is used by the Wear OS
    // MessageClient bridge.  An Automatic BLE disconnect must therefore not
    // erase it: doing so makes every subsequently delivered /data packet fail
    // with "toreceiversocket host=-1" until /netinfo (for example Reinit)
    // happens to recreate the mapping.
    preserveWearMessageMapping=wasautomatic&&localIndex<backup->gethostnr()&&
            getBackupHosts()[localIndex].wearos;
#endif
    if(preserveWearMessageMapping) {
        if(remoteIndex>=0&&remoteIndex<maxallhosts) {
            peers2us[remoteIndex]=localIndex;
            us2peers[localIndex]=remoteIndex;
            }
        LOGGERTAG("automatic Wear BLE disconnect: preserving MessageClient peer mapping local=%d remote=%d\n",
                localIndex,us2peers[localIndex].load());
        }
    else {
        if(remoteIndex>=0&&remoteIndex<maxallhosts) {
            int expected=localIndex;
            peers2us[remoteIndex].compare_exchange_strong(expected,-1);
            }
        else {
            for(auto &mapped:peers2us) {
                int expected=localIndex;
                mapped.compare_exchange_strong(expected,-1);
                }
            }
        us2peers[localIndex]=-1;
        messagephonepeer[localIndex]=false;
        }
    if(wasautomatic) {
        auto &host=getBackupHosts()[localIndex];
        setBlueMessage(localIndex,false);
        if(!mirrorLocalTcpAvailable()) {
            LOGGERTAG("automatic mirror %s(%d): Bluetooth fallback disconnected but local TCP/IP is unavailable; re-requesting Bluetooth\n",
                    host.getnameif(),localIndex);
            sendMessagesON(&host,true);
            }
        else
            LOGGERTAG("automatic mirror %s(%d): Bluetooth fallback disconnected; TCP/IP is locally available for retry\n",
                    host.getnameif(),localIndex);
        return;
        }
    if(!stilldirect)
        return;
    stopmessagesocket(messagesendersockets,messagesendergenerations,localIndex);
    stopmessagesocket(messagereceiversockets,messagereceivergenerations,localIndex);
    backup->closesocksone(localIndex);
    }

extern bool mirrorPeerCurrentTcpAvailable(int index);
extern "C" JNIEXPORT void JNICALL fromjava(retryMessageConnections)(JNIEnv *env,jclass,jstring jskip) {
    if(!backup)
        return;
    const int skip=jskip?getwearindex(env,jskip):-1;
    auto hosts=backup->getHosts();
    const bool localTcpAvailable=mirrorLocalTcpAvailable();
    for(auto &host:hosts) {
        const int index=&host-hosts.data();
        const bool peerTcpCurrent=mirrorPeerCurrentTcpAvailable(index);
        const bool canRetryTcp=localTcpAvailable&&peerTcpCurrent;
        if(index!=skip&&host.automatictransport()&&!host.deactivated&&wearmessages[index]&&canRetryTcp) {
            // Do not open a "probe" TCP connection while the Messages/Direct-BLE
            // socket bridge is active. serverloop() installs every accepted TCP
            // connection into the same TCPConnect::receiverSock; that replaces
            // the bridge socket, after which its self-heal replaces the probe
            // socket in turn. The client then sees EOF during sendpassinit and
            // reports a false password/identity failure.
            //
            // /bleips and Wear /netinfo are authenticated and have just proved
            // that the peer currently has a TCP endpoint. Perform an orderly,
            // acknowledged carrier release first, then wake the ordinary sender.
            // Its normal TCP handshake/password authentication remains the final
            // identity check; if TCP still fails, sender.cpp re-arms the alternate
            // carrier after the existing three-second fallback delay.
            LOGGERTAG("automatic mirror %s(%d): local and authenticated peer TCP endpoints are current; releasing %s carrier for normal TCP/IP retry\n",
                    host.getnameif(),index,host.wearos?"Wear message/Bluetooth":"Bluetooth");
            if(sendMessagesON(&host,false)) {
                LOGGERTAG("automatic mirror %s(%d): alternate carrier released; preferred TCP/IP sender woken\n",
                        host.getnameif(),index);
                wakemessagehost(host);
                }
            else
                LOGGERTAG("automatic mirror %s(%d): alternate-carrier release was not acknowledged; keeping current carrier\n",
                        host.getnameif(),index);
            }
        else {
            if(index!=skip&&host.automatictransport()&&!host.deactivated&&wearmessages[index]&&!localTcpAvailable) {
                LOGGERTAG("automatic mirror %s(%d): local TCP/IP endpoint unavailable; keeping current carrier\n", host.getnameif(),index);
                }
            else {
                if(index!=skip&&host.automatictransport()&&!host.deactivated&&wearmessages[index]&&localTcpAvailable&&!peerTcpCurrent)
                LOGGERTAG("automatic mirror %s(%d): peer has no authenticated current TCP endpoint; keeping current carrier\n", host.getnameif(),index);
                }
            }
        }
    }

extern "C" JNIEXPORT void  JNICALL   fromjava(setBlueMessage)(JNIEnv *env, jclass cl,jstring jident, jboolean val) {
    int index=getwearindex(env,jident);
    if(index<0) return;
    const auto &host=getBackupHosts()[index];
    if(host.deactivated) {
        setBlueMessage(index,false);
        return;
        }
    const auto transport=host.gettransport();
    if(transport==passhost_t::transport_automatic) {
        // This JNI entry point is the Wear MessageClient/TCP carrier switch.
        // Direct BLE selection arrives through mirrorTransportReady() instead.
        mirrordirectbluetoothactive[index]=false;
        LOGGERTAG("automatic mirror %s(%d): accepting remote carrier=%s\n",host.getnameif(),index,
                val?"Messages":"TCP/IP");
        setBlueMessage(index,val);
        }
    else {
        const bool configured=transport==passhost_t::transport_messages||
                transport==passhost_t::transport_bluetooth;
        if(static_cast<bool>(val)!=configured) {
            LOGGERTAG("ignoring remote carrier switch for %s(%d): requested=%d configured=%d transport=%d\n",
                    host.getnameif(),index,val,configured,transport);
            return;
            }
        setBlueMessage(index,configured);
        }
//    LOGGERTAG("setBluemessage(%d)\n", wearmessages[index]);
    }

extern "C" JNIEXPORT void JNICALL fromjava(resetMessageConnection)(JNIEnv *env,jclass,jstring jident) {
    const int index=getwearindex(env,jident);
    if(index<0||index>=maxallhosts||!backup||!wearmessages[index])
        return;
    LOGGERTAG("resetMessageConnection(%d)\n",index);
    stopmessagesocket(messagesendersockets,messagesendergenerations,index);
    stopmessagesocket(messagereceiversockets,messagereceivergenerations,index);
    backup->closesocksone(index);
    auto &host=getBackupHosts()[index];
    startmessagereceiver(host);
    wakemessagehost(host);
    }
