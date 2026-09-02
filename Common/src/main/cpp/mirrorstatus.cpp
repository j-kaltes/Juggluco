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
/*      Tue Apr 11 15:42:30 CEST 2023                                                 */

#include <memory>
#include <array>
#include "datbackup.hpp"
#include "mirrorstatus.hpp"
#include "net/netstuff.hpp"
#include "mirrorerror.h"
#include "net/Connect.hpp"
extern std::array<std::atomic_int,maxallhosts> messagesendersockets;
extern std::array<std::atomic_int,maxallhosts> messagereceiversockets;
extern std::array<std::atomic_int,maxallhosts> us2peers;
extern std::array<std::atomic_bool,maxallhosts> mirrorfallbackrequested;
extern std::array<std::atomic_bool,maxallhosts> mirrordirectbluetoothactive;
extern mirrorstatus_t mirrorstatus[maxallhosts];
#include "deleter.hpp"
//constexpr const int maxmirrortext=200;
#ifdef NOLOG
static constexpr const int RELEASEEXTRA=50;
#else
static constexpr const int RELEASEEXTRA=0;
#endif
extern char *getmirrorerror(const passhost_t *pass);
extern char *getmirrorerrorsettime(const passhost_t *pass);
extern bool mirrorLocalTcpAvailable();
extern std::pair<std::unique_ptr<const char[],deleter>,int> ICEstatus(int allindex) ;
//extern std::unique_ptr<const char[],deleter> ICEstatus(int allindex);
static std::pair<std::unique_ptr<const char[],deleter>,int> getnetstatus(int allindex)  {

	if(allindex<0||allindex>=backup->getupdatedata()->hostnr) {
		return {std::unique_ptr<const char[],deleter>(errormessage,deleter(errormessage)),(int)(sizeof(errormessage)-1)};
		}
        passhost_t &host= getBackupHosts()[allindex];
        if(host.ICE) {
                return ICEstatus(allindex);
                }
        mirrorstatus_t &status=mirrorstatus[allindex];
constexpr	const char *sendmessagestrbase[]={"not done","failed","success"};
const char *const * const sendptr=sendmessagestrbase+1;
	int sendsock=-1;
	bool sendnums=false;
	bool sendstream=false;
	bool sendscans=false;
   if(host.nr>passhost_t::maxip) 
      host.nr=0;
	const int len=host.nr;
	char ips[passhost_t::maxip*46+1]="";
	char *ipsptr=ips;
	if(host.hashostname() ) {
		const char *name=host.gethostname();
		const int namelen=strlen(name);
		memcpy(ipsptr,name,namelen);
		ipsptr+=namelen;
		*ipsptr++=' ';
		}
	else {
		for(int i=0;i<len;i++) {
			namehost name(host.ips+i);
	#if __ANDROID_API__ >= 26
			ipsptr=stpcpy(ipsptr,name.data());
	#else
			const int nlen=strlen(name.data());
			memcpy(ipsptr,name.data(),nlen);
			ipsptr+=nlen;
	#endif
			*ipsptr++=' ';
			}
		}
	ipsptr+=sprintf(ipsptr,"%d",host.getport());
	if(host.index>=0) {
		updateone &send=backup->getupdatedata()->tosend[host.index];
		sendsock=send.getsock();
		sendnums=send.sendnums;
		sendstream=send.sendstream;
		sendscans=send.sendscans;

		}
         auto con=getconnection(allindex);
	int receivesock=con?con->getReceiverIdent():-1;
extern bool getpassive(int pos);
extern bool getactive(int pos); 
      const bool ispassive= getpassive(allindex);
	const	bool isactiveonly=getactive(allindex);
	const char *connect= isactiveonly?"Active only":(ispassive?"Passive only":"Both directions");
	const bool receives=host.receivefrom&2;

	const bool receiveactive=isactiveonly&&receives;
	const char *receivethread=receiveactive?(status.receive.activereceivethread?"active receive thread running":"Active receive thread not running"):"";
	
	

		const bool messagebridge=wearmessages[allindex].load();
		const bool directbluetooth=mirrordirectbluetoothactive[allindex].load();
		const bool fallbackpending=mirrorfallbackrequested[allindex].load();
		const bool tcpsender=sendsock>=0;
		const bool tcpreceiver=receivesock>=0;
		const bool tcpconnected=tcpsender||tcpreceiver;
		const bool localtcpavailable=mirrorLocalTcpAvailable();
		const int transport=host.gettransport();
		const bool messagescarrier=messagebridge&&!directbluetooth&&
			transport!=passhost_t::transport_bluetooth;
		const char *configuredtransport=
			host.automatictransport()?"Automatic":
			transport==passhost_t::transport_tcp?"TCP/IP":
			transport==passhost_t::transport_messages?"Messages":
			transport==passhost_t::transport_bluetooth?"Direct Bluetooth":"Unknown";
		const char *runtimecarrier;
		if(directbluetooth)
			runtimecarrier="Direct Bluetooth (BLE GATT)";
		else if(messagescarrier)
			runtimecarrier="Messages (Wear OS MessageClient)";
		else if(tcpconnected)
			runtimecarrier="TCP/IP";
		else if(host.automatictransport()&&fallbackpending)
			runtimecarrier=host.wearos?
				"No active carrier; Messages/Direct Bluetooth fallback pending":
				"No active carrier; Direct Bluetooth fallback pending";
		else
			runtimecarrier="No active carrier";


		static constexpr char baseformat[]=R"(<h1>Connection %d: %s</h1><p>%s <i>%s</i><br>Send to: %s%s%s running=%s socket=%d locked=%s<br>Receive from: %s %d %s socket=%d wait for commands: %s, interpret: %s<br><b>Transport</b>: configured=%s, selected=%s<br><b>Alternate carrier requested</b>: %s<br><b>Local TCP/IP endpoint</b>: %s<br><b>TCP/IP live socket</b>: %s (send=%s fd=%d, receive=%s fd=%d)</p>)";
		static constexpr char carrierformat[]=R"(<p><b>Carrier details</b><br>Direct Bluetooth (BLE GATT)=%s<br>Messages (Wear OS MessageClient)=%s<br>message bridge=%s<br>Sender:<br>to bluetooth running=%d received=%s  sendmessage: %s<br>messagesendersocket=%d<br>Receiver:<br>to bluetooth running=%d received=%s  sendmessage: %s<br>messagereceiversocket=%d<br>otherside index=%d</p>)";
		constexpr const int maxbuf=2048+passhost_t::maxip*46+
			passhost_t::maxnamelen+maxmirrortext+RELEASEEXTRA;
		char *buf=new(nothrow) char[maxbuf];
		if(!buf) {
		    return {std::unique_ptr<const char[],deleter>(errormessage,deleter(errormessage)),(int)(sizeof(errormessage)-1)};
        }
	//		return std::unique_ptr<const char[],deleter>((char *)errormessage,deleter(errormessage));


		int buflen=snprintf(buf,maxbuf,baseformat,
			allindex,host.getnameif(),ips,connect,
			sendnums?"Amounts ":"",sendscans?"Scans ":"",sendstream?"Stream ":"",
			boolstr[status.sender.running],sendsock,boolstr[status.sender.locked],
			boolstr[receives],status.receive.tid,receivethread,receivesock,
			boolstr[status.receive.ingetcom()],boolstr[status.receive.ininterpret],
			configuredtransport,runtimecarrier,
			boolstr[fallbackpending],
			boolstr[localtcpavailable],
			boolstr[tcpconnected],
			boolstr[tcpsender],sendsock,
			boolstr[tcpreceiver],receivesock);
		if(buflen<0)
			buflen=0;
		else if(buflen>=maxbuf)
			buflen=maxbuf-1;

		if((host.wearos||!host.usesnetworktransport())&&buflen<maxbuf-1) {
			const int added=snprintf(buf+buflen,maxbuf-buflen,carrierformat,
				boolstr[directbluetooth],boolstr[messagescarrier],boolstr[messagebridge],
				status.toblue[true].runs,boolstr[status.toblue[true].recv],
				sendptr[status.toblue[true].sendmessage],messagesendersockets[allindex].load(),
				status.toblue[false].runs,boolstr[status.toblue[false].recv],
				sendptr[status.toblue[false].sendmessage],messagereceiversockets[allindex].load(),
				us2peers[allindex].load());
			if(added>0)
				buflen=added>=maxbuf-buflen?maxbuf-1:buflen+added;
			}
	buf[buflen++]='\n';
extern int getindex(const  passhost_t *host);
    int hostindex=getindex(&host);
   const char *errorstr=mirrorerrors[hostindex];
   int errlen=strlen(errorstr);
   if(errlen) {
        ctime_r(mirrorerrorstimes+hostindex,buf+buflen);
        buflen+=24;
        buf[buflen++]=':';
        buf[buflen++]=' ';
        memcpy(buf+buflen,errorstr,errlen+1);
        }
    else {
        buf[buflen]='\0';
        }
   return {std::unique_ptr<const char[],deleter> (buf,deleter(nullptr)),buflen};

	}
#include "share/fromjava.h"

extern jstring myNewStringUTF(JNIEnv *env,const std::string_view str);
extern "C" JNIEXPORT jstring JNICALL   fromjava(mirrorStatus)(JNIEnv *envin, jclass cl,jint allindex) {
	auto text=getnetstatus(allindex); 
//	return envin->NewStringUTF(text.get());

        return myNewStringUTF(envin,std::string_view(text.first.get(),text.second));
	}

extern char servererrorbuf[];
extern "C" JNIEXPORT jstring JNICALL   fromjava(serverError)(JNIEnv *envin, jclass cl) {
	return envin->NewStringUTF(servererrorbuf);
	}
#ifndef WEAROS
extern char nighterrorbuf[];
extern "C" JNIEXPORT jstring JNICALL   fromjava(nightError)(JNIEnv *envin, jclass cl) {
	return envin->NewStringUTF(nighterrorbuf);
	}
#endif
