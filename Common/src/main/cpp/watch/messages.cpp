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
extern "C"
JNIEXPORT jboolean JNICALL fromjava(message)(JNIEnv *env, jclass thiz, jbyteArray data) {
        if(!data)
                return false;
        int arlen=env->GetArrayLength(data);
        int datlen=arlen- (int)sizeof(wearmessage::type);
    if(datlen<=0) { //TODO:0?
        LOGGERTAG("message: arlen=%d\n",arlen);
        return false;
        }
        std::vector<uint8_t> weardata(datlen+sizeof(wearmessage));
        wearmessage *mess=reinterpret_cast<wearmessage*>(weardata.data());
        env->GetByteArrayRegion(data,0,arlen,reinterpret_cast<jbyte*>(&mess->type));
        if(env->ExceptionCheck()) {
            env->ExceptionClear();
            return false;
            }
        mess->len=datlen;
        return receivemessage(mess);
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
    static auto jsendData=env->GetStaticMethodID(jMessageSender,"sendData","([B)Z");
        bool res=jsendData&&env->CallStaticBooleanMethod(jMessageSender,jsendData,uit);
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
    int phonehost= us2peers[hostnr];
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
    #else
    const int phonehost= hostnr;
    if(messagephonepeer[hostnr].load()) {
        constexpr int mappingwaits=100;
        int peer=us2peers[hostnr];
        for(int wait=0;peer<0&&wait<mappingwaits&&wearmessages[hostnr];++wait) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            peer=us2peers[hostnr];
            }
        if(peer<0||peer>=maxallhosts) {
            LOGGERTAG("tobluetooth no phone-peer mapping for host=%d\n",hostnr);
            clearmessagesocket(registeredsockets,generations,hostnr,sock,generation);
            shutdown(sock,SHUT_RDWR);
            sockclose(sock);
            return;
            }
        }
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
        TCPConnect *connect=static_cast<TCPConnect *>(connections[index]);
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
        connect=static_cast<TCPConnect *>(connections[index]);
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
extern bool messagehaswlan(int index);
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
    passhost_t &host= getBackupHosts()[ident];
    LOGGERTAG("setBlueMessage(%s(%d),%d)\n",host.getname(),ident,val);
    if(host.deactivated&&val)
        return;
    if(wearmessages[ident].exchange(val)!=val) {
        if(backup) {
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
        }
    }

extern "C" JNIEXPORT void JNICALL fromjava(retryMessageConnections)(JNIEnv *env,jclass,jstring jskip) {
    if(!backup)
        return;
    const int skip=jskip?getwearindex(env,jskip):-1;
    auto hosts=backup->getHosts();
    for(auto &host:hosts) {
        const int index=&host-hosts.data();
        if(index!=skip&&host.wearos&&!host.deactivated&&wearmessages[index]&&messagehaswlan(index))
            sendMessagesON(&host,false);
        }
    }

extern "C" JNIEXPORT void  JNICALL   fromjava(setBlueMessage)(JNIEnv *env, jclass cl,jstring jident, jboolean val) {
    int index=getwearindex(env,jident);
    if(index<0) return;
    setBlueMessage(index,val);
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
