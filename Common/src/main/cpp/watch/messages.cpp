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
#include <thread>
#include <semaphore>
#include "destruct.h"
#include "datbackup.h"
#include "net/netstuff.h"
/*
struct wearmessage {
   int32_t len;
   int16_t phonehostnr;
   bool phonesender;
   uint8_t reserved;
   uint8_t data[];
};
*/
struct wearmessagetype{
   int16_t phonehostnr;
   bool phonesender;
   uint8_t reserved;
};
struct wearmessage {
   int32_t len;
   wearmessagetype type;
   uint8_t data[];
};
extern std::array<int,maxallhosts>             peers2us,us2peers;

std::array<int,maxallhosts>   messagesendersockets;
std::array<int,maxallhosts>   messagereceiversockets;
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

int toreceiversocket(const wearmessage *mess) { //TODO test socket==-1
#ifdef WEAROS
const    int host=peers2us[mess->type.phonehostnr];
if(host<0) {
	LOGGER("toreceiversocket host=%d\n",host);
	return -1;
	}
int sock;
   if(mess->type.phonesender) {
	sock= messagereceiversockets[host];
	LOGGER("toreceiversocket host=%d receiver sock=%d\n",host,sock);
   	}
else {
	sock= messagesendersockets[host];
	LOGGER("toreceiversocket host=%d sender sock=%d\n",host,sock);
	}
#else
 const   int host=mess->type.phonehostnr;
 int sock;
   if(mess->type.phonesender) {
   	sock= messagesendersockets[host];
	LOGGER("toreceiversocket host=%d sender sock=%d\n",host,sock);
	}
else {
   sock= messagereceiversockets[host];
   LOGGER("toreceiversocket host=%d receiver sock=%d\n",host,sock);
   }
#endif

return sock;
    }
bool receivemessage(wearmessage *pWearmessage) {
       LOGGER("type.phonehostnr=%d type.phonesender=%d %p#%d\n",pWearmessage->type.phonehostnr,pWearmessage->type.phonesender,(char *)pWearmessage->data,pWearmessage->len);
       return(sendni(toreceiversocket(pWearmessage),pWearmessage->data,pWearmessage->len)>=0);
    }
    /*
bool initmessages(const int hostnr) {
      passhost_t &host= getBackupHosts()[hostnr];
      if(!host.wearos) {
      	LOGGER("initmessage hostnr=%d %s no wearos\n",hostnr,host.getnameif());
	return false;
	}
     if(host.receivefrom) {
      	LOGGER("initmessage %s receivefrom\n",host.getnameif());
     	}
   const int sendindex=host.index;
    if(sendindex>=0) {
    	LOGGER("initmessage %s\n",host.getnameif());
    	}
return true;
    }
*/
extern "C"
JNIEXPORT jboolean JNICALL fromjava(message)(JNIEnv *env, jclass thiz, jbyteArray data) {
        if(!data)
                return false;
        int arlen=env->GetArrayLength(data);
        int datlen=arlen- (int)sizeof(wearmessage::type);
	if(datlen<=0) { //TODO:0?
		LOGGER("message: arlen=%d\n",arlen);
		return false;
		}
        alignas(wearmessage) uint8_t weardata[datlen+sizeof(wearmessage)];
        wearmessage *mess=reinterpret_cast<wearmessage*>(&weardata);
        env->GetByteArrayRegion(data,0,arlen,reinterpret_cast<jbyte*>(&mess->type));
        mess->len=datlen;
        return receivemessage(mess);
	}
	/*
#include <cerrno>
#include <cstdarg>
int  flerror(const char *format,...) {
    char buf[1024];
    const char *errstr=strerror(errno);
    va_list ap;
    va_start(ap,format);
    int end=vsprintf(buf,format,ap);
    va_end(ap);
    buf[end++]=':';
    buf[end++]=' ';
    strcpy(buf+end,errstr);
    return __android_log_write(1,"Message",buf);
    }

#define lerror(x) flerror("%s\n",x);
#include <unistd.h>
       #include <sys/mman.h>
inline ssize_t  recvni(int sockfd, void *buf, size_t len) {
        int waslen;
        int inter=0;    
        while((waslen=recv(sockfd,buf,len,0))==-1) {
                int erwas=errno;
                flerror("recv(%d,buf,%zd)",sockfd,len);
                if(erwas!=EINTR||inter>20)
                        return -1;
                ++inter;
                }
        return waslen;
        }
*/
jclass jMessageSender =nullptr;
jmethodID jsendDatawithName=nullptr;
jmethodID jsendData=nullptr;
jmethodID jsendNameMessageOn=nullptr;
jmethodID jsendMessageOn=nullptr;
constexpr const char classname[]="tk/glucodata/MessageSender";
extern JavaVM* vmptr;
bool jinitmessages(JNIEnv* env) {
    jclass cl=env->FindClass(classname);
if(!cl) {
        LOGGER("Can't find %s\n",classname);
        return false;
    }
else {
        jMessageSender = (jclass)env->NewGlobalRef(cl);
        env->DeleteLocalRef(cl);
#ifdef WEAROS
        if(!(jsendData=env->GetStaticMethodID(jMessageSender,"sendData","([B)Z"))) {
                LOGGER("GetStaticMethodID(jMessageSender,\"sendData\",\"([B)Z\" failed\n");
                return false;
                }
        if(!(jsendMessageOn=env->GetStaticMethodID(jMessageSender,"sendMessageOn","(Z)V"))) {
                LOGGER("GetStaticMethodID(jMessageSender,\"sendMessageOn\",\"(Z)V\" failed\n");
                return false;
                }
#else	
        if(!(jsendDatawithName=env->GetStaticMethodID(jMessageSender,"sendDatawithName","(Ljava/lang/String;[B)Z"))) {
                LOGGER("GetStaticMethodID(jMessageSender,\"sendDatawithName\",\"(Ljava/lang/String;[B)Z\" failed\n");
                return false;
                }

       /* if(!(jsendNameMessageOn=env->GetStaticMethodID(jMessageSender,"sendNameMessageOn","(Ljava/lang/String;Z)V"))) {
                LOGGER("GetStaticMethodID(jMessageSender,\"sendNameMessageOn\",\"(Ljava/lang/String;Z)V\" failed\n");
                return false;
                }  */
#endif

        }
    return true;
}
extern JNIEnv *getenv();
#ifdef WEAROS
extern void sendMessagesON(passhost_t *pass,bool val);
extern void setBlueMessage(int,bool val);
void sendMessagesON(passhost_t *pass, bool val) {
	 const int index=pass-getBackupHosts().data();
	auto env=getenv();
        env->CallStaticVoidMethod(jMessageSender,jsendMessageOn,val);
	setBlueMessage(index,val);
	}
#endif

std::array<jstring,maxallhosts>   connectionnames;
bool	sendmessage(const int phonehostnr,bool phonesender,const uint8_t *buf,const int inlen) {
	LOGGER("sendmessage(%d,%d,%p#%d)\n",phonehostnr,phonesender,buf,inlen);
	auto env=getenv();
   	int totlen=inlen+sizeof(wearmessage)-sizeof(wearmessage::len);
	jbyteArray uit=env->NewByteArray(totlen);
	int start=offsetof(wearmessage,type);
	int offdata=offsetof(wearmessage,data);

	wearmessagetype type{.phonehostnr=static_cast<int16_t>(phonehostnr),.phonesender=phonesender};
        env->SetByteArrayRegion(uit, 0, sizeof(wearmessagetype),(const jbyte *)&type);
        env->SetByteArrayRegion(uit, offdata-start, inlen,(const jbyte *)buf);
#ifdef WEAROS
        bool res=env->CallStaticBooleanMethod(jMessageSender,jsendData,uit);
#else
	if(!connectionnames[phonehostnr]) {
		const char *name=getBackupHosts()[phonehostnr].getname(); 
		if(!name) {
			LOGGER("sendmessage %d noname\n",phonehostnr);
			return false;
			}
		jstring tmpstr=	env->NewStringUTF(name);
		connectionnames[phonehostnr]=(jstring) env->NewGlobalRef(tmpstr);
        	env->DeleteLocalRef(tmpstr);
		}
        bool res=env->CallStaticBooleanMethod(jMessageSender,jsendDatawithName,connectionnames[phonehostnr],uit);
#endif
        env->DeleteLocalRef(uit);
	return res;
	}
void clearnetworkcache() {
	connectionnames={};
	}
void tobluetooth(int hostnr,bool sender,int sock, std::binary_semaphore *waitstarted) {
   	LOGGER("tobluetooth(%d,%d,%d)\n", hostnr, sender, sock);

	#ifdef WEAROS
	int phonehost= us2peers[hostnr];
	while(phonehost<0) {
		LOGGER("sleep(5) phonehost=%d\n",phonehost);
		sleep(5);
		phonehost=us2peers[hostnr];
		LOGGER(" after sleep(5) phonehost=%d\n",phonehost);
		}
	#else
	const int phonehost= hostnr;
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
	    return;
    	}
   destruct _dest([maxbuf,buf]() { munmap(buf,maxbuf); });
// receiversockopt(sock);
	{
	char buf[30]; 
	int len=sprintf(buf, "tobluetooth %d %s",hostnr,sender?"S":"R");
	prctl(PR_SET_NAME, buf, 0, 0, 0);
	}
	LOGGER("tobluetooth before release\n");
	waitstarted->release();
   while(true) { 
        int inlen=recvni(sock,buf,maxbuf);
       LOGGER("recvni(%d,...)=%d\n",sock,inlen);
        if(inlen<=0)  {
            if(messagesendersockets[hostnr]==sock) {
		  messagesendersockets[hostnr]=-1;
		  shutdown(sock,SHUT_RDWR);
		  close(sock);
	    	}
            return;
	    }
        while(!sendmessage(phonehost,phonesender,buf,inlen)) {
		LOGGER("sendmessage failed %d %d #%d\n",phonehost,phonesender,inlen);
		sleep(30);
		}
        }
    }
void messagereceivecommands(passhost_t *pass) {
	const int index=pass-getBackupHosts().data();

	 for(int i=0;wearmessages[index];i++) {
		int sockpair[2];
		if(socketpair(AF_LOCAL,SOCK_STREAM,0,sockpair)!=0) {
			lerror("socketpair");
			return ; 
			}
		{
		char buf[24];
		int len=sprintf(buf,"%d message %d",i,index);
		prctl(PR_SET_NAME, buf, 0, 0, 0);
		LOGGERN(buf,len);
		}
		messagereceiversockets[index]=sockpair[0];

		std::binary_semaphore waitstarted(0);
		std::thread th(tobluetooth,index,false, sockpair[0],&waitstarted); //TODO handshake?
	//	th.detach();
		waitstarted.acquire();
		bool    getcommandsnopass(int sock,passhost_t *host); //TODO password?
	extern	void receiversockopt(int new_fd);
		receiversockopt(sockpair[1]);
		getcommandsnopass(sockpair[1],pass);
		 int receivesock=messagereceiversockets[index];
		  shutdown(receivesock,SHUT_RDWR);
		 LOGGER("%d message join\n",index);
		th.join();
		  close(receivesock);
		  /*
		if(receivesock!=sockpair[0])  {
			LOGGER("messagereceivecommands %d!=%d\n",receivesock,sockpair[0]);
			return;
			} */
		LOGGER("try again\n");
		 }
	LOGGER("wearmessages[%d]==false\n",index);
	return;
	}
void startmessagereceiver(passhost_t &host) {
	LOGGER("startmessagereceiver %s\n",host.getname());
	std::thread th(messagereceivecommands,&host);
	th.detach();
	}
void startmessagereceivers(Backup *backup) {
	LOGGER("startmessagereceivers\n");
	auto hspan=backup->getHosts();
	for(passhost_t &host:hspan) {
		if(host.wearos) {
			startmessagereceiver(host);
			}
		}
	}
int messagemakeconnection(passhost_t *pass,bool sender,int &sock,crypt_t*ctx,char stype) {
	int sockpair[2];
	if(socketpair(AF_LOCAL,SOCK_STREAM,0,sockpair)!=0) {
		lerror("messagemakeconnection socketpair");
		sock=-1;
		return -1;
		}
	const int index=pass-getBackupHosts().data();
	messagesendersockets[index]=sockpair[1];
	sock=sockpair[0];
	std::binary_semaphore waitstarted(0);
	std::thread th(tobluetooth,index,sender, sockpair[1],&waitstarted); //TODO handshake?
	th.detach();
	waitstarted.acquire();
	LOGGER("messagemakeconnection %s sock=%d (other end=%d)\n",pass->getname(),sock,sockpair[1]);
	return sock;
	}


#endif
//bool wearmessages=false;


extern bool getwearindex(JNIEnv *env, jstring jident) ;
extern "C" JNIEXPORT jboolean  JNICALL   fromjava(getBlueMessage)(JNIEnv *env, jclass cl,int index) {
	if(index<0)
		return false;
	LOGGER("getBluemessage(%d)=%d\n", index,wearmessages[index]);
	return wearmessages[index];
	}
/*
extern "C" JNIEXPORT jboolean  JNICALL   fromjava(getBlueMessage)(JNIEnv *env, jclass cl,jstring jident) {
	int index=getwearindex(env,jident);
	if(index<0)
		return false;
	LOGGER("getBluemessage(%d)=%d\n", index,wearmessages[index]);
	return wearmessages[index];
	}
	*/
void closesock(int &sock) {
	shutdown(sock,SHUT_RDWR);
	close(sock);
	sock=-1;
	}
void closesocks(std::array<int,maxallhosts>   &socks) {
	for(auto &el:socks) {
		if(el!=-1) {
			closesock(el);
			}
		}
	}
extern void startmessagereceivers(Backup*);

extern void startmessagereceiver(passhost_t &host);
extern void startactivereceivers();
void setBlueMessage(int ident,bool val) {
      passhost_t &host= getBackupHosts()[ident];
	LOGGER("setBlueMessage(%s(%d),%d)\n",host.getname(),ident,val);
	if(wearmessages[ident]!=val ) {
		wearmessages[ident]=val;
		if(backup) {
			backup->closeallsocks();
			if(!val) {
				closesock(messagesendersockets[ident]);
				closesock(messagereceiversockets[ident]);
				}
			else {
				startmessagereceiver(host);
				}
			}
		}
	}

extern "C" JNIEXPORT void  JNICALL   fromjava(setBlueMessage)(JNIEnv *env, jclass cl,jstring jident, jboolean val) {
    int index=getwearindex(env,jident);
	if(index<0) return;
	setBlueMessage(index,val);
//	LOGGER("setBluemessage(%d)\n", wearmessages[index]);
	}
