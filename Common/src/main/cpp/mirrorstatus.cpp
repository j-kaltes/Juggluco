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
extern std::array<int,maxallhosts>   messagesendersockets;
extern std::array<int,maxallhosts>   messagereceiversockets;
extern std::array<int,maxallhosts>             us2peers;
extern mirrorstatus_t mirrorstatus[maxallhosts];

//constexpr const int maxmirrortext=200;
extern char *getmirrorerror(const passhost_t *pass);
struct deleter {
const char *ptr;
deleter(const char ptr[]): ptr(ptr){};
   void operator()(const char p[]) const {
   	if(p!=ptr)
		delete[] p;
    }
};

std::unique_ptr<const char[],deleter> getnetstatus(int allindex)  {
	constexpr const char boolstr[][6]={"false","true"};
	constexpr const char errormessage[]=R"(<h1>Error</h1>)";

	if(allindex<0||allindex>=backup->getupdatedata()->hostnr) {
		return std::unique_ptr<const char[],deleter>(errormessage,deleter(errormessage));
		}
		passhost_t &host= getBackupHosts()[allindex];
		mirrorstatus_t &status=mirrorstatus[allindex];
constexpr	const char *sendmessagestrbase[]={"not done","failed","success"};
const char *const * const sendptr=sendmessagestrbase+1;
	int sendsock=-1;
	bool sendnums=false;
	bool sendstream=false;
	bool sendscans=false;
   if(host.nr>maxip) 
      host.nr=0;
	const int len=host.nr;
	char ips[maxip*46+1]="";
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
	int receivesock=hostsocks[allindex];
extern bool getpassive(int pos);
extern bool getactive(int pos); 
      const bool ispassive= getpassive(allindex);
	const	bool isactiveonly=getactive(allindex);
	const char *connect= isactiveonly?"Active only":(ispassive?"Passive only":"Both directions");
	const bool receives=host.receivefrom&2;

	const bool receiveactive=isactiveonly&&receives;
	const char *receivethread=receiveactive?(status.receive.activereceivethread?"active receive thread running":"Active receive thread not running"):"";
	
	

	static char format[]=R"(<h1>Connection %d: %s</h1><p>%s <i>%s</i><br>Send to: %s%s%s running=%s socket=%d locked=%s<br>Receive from: %s %d %s socket=%d wait for commands: %s, interpret: %s</p>  <p><b>WearOS</b><br>messages=%s<br>Sender:<br>to bluetooth running=%d received=%s  sendmessage: %s<br>messagesendersocket=%d<br>Receiver:<br>to bluetooth running=%d received=%s  sendmessage: %s<br>messagereceiversocket=%d<br>otherside index=%d</p>)";
	const int maxbuf=sizeof(format)+12*5+2*8+2+passhost_t::maxnamelen+100+20+maxmirrortext;
	char *buf=new(nothrow) char[maxbuf];
		if(!buf)  {
			return std::unique_ptr<const char[],deleter>((char *)errormessage,deleter(errormessage));
			}
	constexpr const  int startwearos=168;
	int buflen;
	if(host.wearos) {
		format[startwearos]=' ';
		buflen=snprintf(buf,maxbuf,format,allindex,host.getnameif(),ips,connect,sendnums?"Amounts ":"",sendscans?"Scans ":"",sendstream?"Stream ":"",boolstr[status.sender.running],sendsock,boolstr[status.sender.locked],

		boolstr[receives],status.receive.tid,receivethread,receivesock,boolstr[status.receive.ingetcom()],boolstr[status.receive.ininterpret],

		boolstr[wearmessages[allindex]],
		status.toblue[true].runs, boolstr[status.toblue[true].recv], sendptr[status.toblue[true].sendmessage], messagesendersockets[allindex],

		status.toblue[false].runs, boolstr[status.toblue[false].recv], sendptr[status.toblue[false].sendmessage],

messagereceiversockets[allindex],
            us2peers[allindex]
		);
		}
	else {
		format[startwearos]='\0';

		buflen=snprintf(buf,maxbuf,format,allindex,host.getnameif(),ips,connect,sendnums?"Amounts ":"",sendscans?"Scans ":"",sendstream?"Stream ":"",boolstr[status.sender.running],sendsock,boolstr[status.sender.locked],

		boolstr[receives],status.receive.tid,receivethread,receivesock,boolstr[status.receive.ingetcom()],boolstr[status.receive.ininterpret]);

		}
	buf[buflen++]='\n';
	  strcpy(buf+buflen,getmirrorerror(&host));
		return std::unique_ptr<const char[],deleter> (buf,deleter(nullptr));

	}
#include "share/fromjava.h"
extern "C" JNIEXPORT jstring JNICALL   fromjava(mirrorStatus)(JNIEnv *envin, jclass cl,jint allindex) {
	auto text=getnetstatus(allindex); 
	return envin->NewStringUTF(text.get());
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
