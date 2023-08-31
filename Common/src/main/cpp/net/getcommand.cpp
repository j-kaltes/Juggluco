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
/*      Fri Jan 27 12:38:18 CET 2023                                                 */


#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
       #include <unistd.h>
#include "comtypes.hpp"
#include "receive.h"
#include "passhost.h"
#include "makerandom.h"
#include "crypt.h"
#include "netstuff.h"
#include "datbackup.h"
#include "aligner.h"

#define lerrortag(...) lerror("getcommands: " __VA_ARGS__)
#define LOGGERTAG(...) LOGGER("getcommands: " __VA_ARGS__)
#define LOGSTRINGTAG(...) LOGSTRING("getcommands: " __VA_ARGS__)
#define flerrortag(...) flerror("getcommands: " __VA_ARGS__)

extern Backup *backup;
extern void		processglucosevalue(int sendindex,int newstart=-1);

//getdata filedata("/data/local/tmp/testdir");
getdata filedata;


static bool sendcrypt(int sock,crypt_t *ctx,uint8_t *data,int datalen) {
	constexpr int taglen=16;
	const int alllen=taglen+datalen;
	uint8_t buf[alllen];
	uint8_t *start=buf+taglen;
	size_t wrote= ascon_aead128a_encrypt_update(ctx, start,data ,datalen);
	ascon_aead128a_encrypt_final(ctx, start + wrote, buf, taglen);
	if(sendni(sock,buf,sizeof(buf))!=sizeof(buf)) {
		flerrortag("sendcrypt send shutdown %d",sock);
		::shutdown(sock,SHUT_RDWR);
		return false;
		}
	return true;
	}

static bool openfile(int sock,crypt_t *ctx,const char *name) {
	int fp=filedata.open(name);
	LOGGERTAG("open(%s)=%d\n",name,fp);
	int16_t sfp=(int16_t)fp;
	int16_t sendfp[2]={sfp,static_cast<int16_t>(((uint16_t)0xFFFF)&(~sfp))};
	if(ctx) {
		if(!sendcrypt(sock,ctx, reinterpret_cast<uint8_t*>(sendfp),sizeof(sendfp)))  {
			filedata.close(fp); 
			LOGGERTAG("openfile shutdown %d\n",sock);
			::shutdown(sock,SHUT_RDWR);
			return false;
			}

		}
	else  {
		if(sendni(sock,sendfp,sizeof(sendfp))!=sizeof(sendfp)) {
			lerrortag("openfile send +shutdown %d\n");
			filedata.close(fp); 
			::shutdown(sock,SHUT_RDWR);
			return false;
			}
		}
	return true;
	}

void	backupbase(string_view basedir) {
	filedata.setpath(basedir);
	}

extern	bool backupnums(const struct numsend* innums);
//cmd han uint32_t uint32_t unsigned byte
//bool savedata(int fp,uint32_t offset, uint32_t len,const unsigned char *data) {

#include "aligner.h"
//int alignadd(int was,int bij) { return was+((bij+3)/4)*4; }
/*
int alignadd(int was,int bij) {
//	return was+aligner<4>((unsigned long)bij);
	return was+aligner<4>(bij);
	}
	*/
#define addlen(x,y) x+=y
//s/it+=comlen/it=alignadd(it,comlen)/g
extern bool receivelastpos(const lastpos_t *data) ;

static			bool savefileonce(const struct fileonce_t *gegs);

static std::pair<int,int> interpret(int sock,passhost_t *host,crypt_t *ctx,senddata_t *datain,int len) {

LOGGERTAG("interpret len=%d \n",len);
for(int it=0;it<len;) {
	int comlen;
	if((len-it)<2) {
		comlen=sizeof(sendack);
		addlen(it,comlen);
		return {it,comlen};
		}
	senddata_t *data=datain+it;
	uint16_t *us=reinterpret_cast<uint16_t*>(data),command=*us;
	LOGGERTAG("%d com=%d %d\n",sock,command,it);
 
	bool ret=false;
	if(!(host->receivefrom&2)&&command!=sbackupstop&&command!=swakeupstream&&command!=sbackup&&command!=sack)  {
		LOGSTRINGTAG("interpret: I don't receive from  this host\n");
		return {-1,0};
		}

		
	switch(command) {
		case sack: {
			comlen=sizeof(sendack);
			addlen(it,comlen);
			if(it>len) {
				return {it,comlen};
				}
			if(sendni(sock,&ackres,sizeof(ackres))!=sizeof(ackres)) 
				return {-1,0};
			LOGSTRINGTAG("ack send\n");
			ret=true;
			};break;
		case sopen: 
			{
			struct fileopen *com=reinterpret_cast<fileopen *>(data);
			comlen=sizeof(struct fileopen)+com->len+1;
			addlen(it,comlen);
			if(it>len) {
				return {it,comlen};
				}
			ret=openfile(sock,ctx,com->name);
			break;};
		case smklen: 
			comlen=sizeof(struct mklen);
			addlen(it,comlen);
			if(it>len) {
				return {it,comlen};
				}
			ret=filedata.mkfile(us[1], reinterpret_cast<uint32_t *>(data)[1]);;break;
		case snumnr: {
			comlen=sizeof(lastpos_t);
			addlen(it,comlen);
			if(it>len) {
				return {it,comlen};
				}
			ret=receivelastpos(reinterpret_cast<const lastpos_t *>(data));
			};break;
		case snuminit: {
			comlen=sizeof(numinit);
			addlen(it,comlen);
			if(it>len) {
				return {it,comlen};
				}
			bool backupnuminit(const numinit *numst) ;

			ret=backupnuminit(reinterpret_cast<const numinit *>(data));
			};break;
		case snums: {
			 numsend *numgeg=reinterpret_cast<numsend *>(data);
			 comlen=numgeg->totlen;
			addlen(it,comlen);
			if(it>len) {
				return {it,comlen};
				}
			ret=backupnums(numgeg);
			};break;
		case swrite: 
			{
			struct offwrite *wcom=reinterpret_cast<struct offwrite*>(data);
			comlen=sizeof(struct offwrite)+wcom->len;
			addlen(it,comlen);
			if(it>len) {
				return {it,comlen};
				}
			LOGSTRINGTAG("swrite\n");
			ret= filedata.savedata(wcom->han,wcom->off,wcom->len,wcom->data);
			};break;
		case sclose: comlen=4;
			addlen(it,comlen);
			if(it>len) {
				return {it,comlen};
				}
			ret= filedata.close(us[1]);
			break;
		case sglucose:
		    {comlen=4;
			addlen(it,comlen);
			if(it>len) {
				return {it,comlen};
				}
			struct renderstruct *rend=reinterpret_cast<struct renderstruct*>(data);
		        processglucosevalue(rend->type);
			ret=true;
			}
			break;
		case srender:
		    {comlen=4;
			addlen(it,comlen);
			if(it>len) {
				return {it,comlen};
				}
			struct renderstruct *rend=reinterpret_cast<struct renderstruct*>(data);
			if(rend->type)
				backup->wakebackup(rend->type);
			else
				backup->wakebackup();
			extern void render(); 
			render();
			ret=true;
			}
			break;
		case suptodate: comlen=4;
			addlen(it,comlen);
			if(it>len) {
				return {it,comlen};
				}
			void uptodate(passhost_t *);
			uptodate(host);
			ret=true;
			break;
		case sresetdevices: {
			comlen=4;
			addlen(it,comlen);
			if(it>len) {
				return {it,comlen};
				}
extern				bool updateDevices() ;
			ret=updateDevices();
			LOGGERTAG("updateDevices=%d\n",ret);
			};break;
		case sbackup: 
			ret=true;
		case sbackupstop: 
			comlen=4;
			addlen(it,comlen);
			if(it>len) {
				return {it,comlen};
				}

			bool netwakeup(int sock,passhost_t *host,crypt_t *ctx);
			if(netwakeup(sock,host,ctx)) {
				return {-1,-1};
				}
			break;
		case swakeupstream: 
			ret=true;
			comlen=4;
			addlen(it,comlen);
			if(it>len) {
				return {it,comlen};
				}

			bool netwakeupstream(int sock,passhost_t *host,crypt_t *ctx);
			if(netwakeupstream(sock,host,ctx)) {
				return {-1,-1};
				}
			break;
		case saskfile:		
			{int minlen=sizeof(askfile);
			addlen(it,minlen);
			if(it>len)
				return {it,minlen};
			struct askfile *ask=reinterpret_cast<struct askfile *>(data);
			addlen(it,ask->namelen);
			if(it>ask->len) {
				return {it,aligner<4>(minlen+ask->namelen)};
				}
			bool sendfile(int sock,crypt_t *pass,const char *filename,uint32_t off,uint32_t len);
			ret=sendfile(sock,ctx,ask->name,ask->off,ask->len);
			};
			break;
		case sasklastnum:{
			comlen=sizeof(asklastnum);
			addlen(it,comlen);
			if(it>len) {
				return {it,comlen};
				}
			const asklastnum *asklast=reinterpret_cast<const asklastnum *>(data);
			int sendlastnum(const int dbase);
			int last=sendlastnum(asklast->dbase);
			if(last<0)
				return {-1,0};
		bool noacksendcommand(crypt_t*,int sock ,const unsigned char *buf,int buflen) ;
			ret=noacksendcommand(ctx, sock ,reinterpret_cast<uint8_t*>(&last),sizeof(last)) ;
			}; break;

		case sfileonce:
			{
			LOGSTRINGTAG("fileonce\n");
			int tolen=offsetof(fileonce_t,nr);
			const fileonce_t *gegs=reinterpret_cast<const fileonce_t *>(data);
			if(len>(it+tolen)) {
				comlen=gegs->totlen;
				}
			else {
				comlen=sizeof(fileonce_t);
				}
			addlen(it,comlen);
			if(it>len) {
				logprint("too small %d\n",len);
				return {it,comlen};
				}

			ret=savefileonce(gegs);
			} break;
		default:;
		}
	if(!ret)
		return {-1,0};
	it=aligner<4>(it);
	}
return {0,0};
}
#include <algorithm>
//constexpr std::align_val_t maxalign=static_cast<std::align_val_t>( std::max({alignof(struct fileopen), alignof(struct mklen ), alignof(struct offwrite )})) ;
//constexpr std::align_val_t maxalign( std::max({alignof(struct fileopen), alignof(struct mklen ), alignof(struct offwrite )})) ;

constexpr std::align_val_t maxalign=std::align_val_t( std::max({alignof(struct fileopen), alignof(struct mklen ), alignof(struct offwrite )})) ;
 
struct senddata_deleter { // deleter
    void operator() (senddata_t *ptr) {
	operator delete[] (ptr, maxalign);
    }
};
 
bool	getcommandsnopass(int sock,passhost_t *host) {
	int allindex=gethostindex(host);
	auto &status=mirrorstatus[allindex].receive;
	status.running(true);
	destruct _dest([&status]{status.running(false);});

	LOGSTRINGTAG("getcommandsnopass\n");
	int start=0;
	int maxcom =1024*1024;
	std::unique_ptr<senddata_t[],senddata_deleter> ptr(new (maxalign,std::nothrow) senddata_t [maxcom],senddata_deleter());

	senddata_t *com=ptr.get();
	if(!com) {
		sleep(1);
		return false;
		}

	while(true) {
		LOGSTRINGTAG("voor recv\n");
		int len=recvni(sock,com+start,maxcom-start);
		LOGGERTAG("%d=recv\n",len);
		switch(len) { 
			case 0: {
				LOGSTRINGTAG("closed\n");
				return false;
				};
			case -1: {
				lerrortag("recv");
				return false;
				}
			default:
				int totlen=start+len;
				status.ininterpret=true;
				auto [last,comlen]=interpret(sock,host,nullptr,com,totlen);
				status.ininterpret=false;
				LOGGERTAG("[%d,%d]=interpret\n",last,comlen);
				if(last) {
					if(last<0) {
						if(comlen==0)
							return false;
						return true;
						}
					int was=last-comlen;	
					start=totlen-was;
					if(comlen>maxcom) {
						senddata_t *oldcom=com;
						maxcom=comlen;
						com=new (maxalign,std::nothrow) senddata_t [maxcom];
						if(!com) {
							sleep(1);
							return false;
							}
						memcpy(com,oldcom+was,start);
						ptr.reset(com);
						}
					else  {
						if(was)  
							memmove(com,com+was,start);
						}
					}
				else   {
					start=0;
					}
			}
		}
	
	}
#include "crypt.h"


static int interpretcommands(int sock,passhost_t *host,crypt_t *ctx,senddata_t *com,int totlen) {
	auto [last,comlen]=interpret(sock,host,ctx,com,totlen);
	if(last<0) {
		if(comlen==0)
			return 2;
		return 1;
		}
	if(last>0) {
		LOGGERTAG("Strange error last>0 %d\n",last);
		return 2;
		}
	return 0;
	}
constexpr int MAXDATA=1024*1024*256;
static bool getcom(int sock, passhost_t *host,ascon_aead_ctx_t *ctx) {
	int allindex=gethostindex(host);
	auto &status=mirrorstatus[allindex].receive;
	status.running(true);
	destruct _dest([&status]{status.running(false);});



	while(true) {
		LOGSTRINGTAG("getcom\n");
		constexpr const int taglen=16;
		uint8_t tag[taglen];
		if(int getlen=recvni(sock,tag,taglen);getlen!=taglen) {
			LOGGERTAG("recv tag %d\n",getlen);
			return false;
			}
		LOGSTRINGTAG("got tag\n");
		constexpr int testlen=16;
		senddata_t start[testlen];
		const int len1=recvni(sock,start,testlen);
		switch(len1) {
			case 0: {
			LOGSTRINGTAG("recv==0\n");
			return false;
			};
			case -1: {
				flerrortag("error recv, shutdown %d\n",sock);
				::shutdown(sock,SHUT_RDWR);
				return false;
				}
			};
		constexpr int maxcom =1024*1024;
		std::unique_ptr<senddata_t[],senddata_deleter> ptruit(new (maxalign,std::nothrow) senddata_t [maxcom],senddata_deleter());
		senddata_t *uit=ptruit.get();
		if(!uit) {
			sleep(1);
			return false;
			}
		int res= ascon_aead128a_decrypt_update(ctx, uit, start, testlen);
		constexpr int intlen=sizeof(int);
		if(res<intlen)  {
			LOGGERTAG("ascon_aead128a_decrypt_update(...%d)=%d\n",len1,res);
			return false;
			}
		const int datlen=*reinterpret_cast<int *>(uit);
		LOGGERTAG("datlen=%d\n",datlen);
		if(datlen<0||datlen>MAXDATA) {
			LOGGERTAG("strange datasize=%d\n",datlen);
			return false;
			}
		const int intdatlen=datlen+sizeof(int);
		const int totlen=(intdatlen<16?16:intdatlen);
		if(totlen>maxcom) {
			senddata_t *newbuf= new (maxalign,std::nothrow) senddata_t [totlen];
			if(!uit) {
				sleep(1);
				return false;
				}
			memcpy(newbuf,uit,res);
			uit=newbuf;
			ptruit.reset(uit);
			}	
		const int bijlen=totlen-testlen;
		uint8_t *incrypt=new(std::nothrow) uint8_t[bijlen];
		if(!incrypt) {
			sleep(1);
			return false;
			}
		
		std::unique_ptr<uint8_t[] > deltoo(incrypt);
		int len=0;
		LOGSTRINGTAG("voor recv\n");
		for(;len<bijlen;) {
			int bij=recvni(sock,incrypt+len,bijlen-len);
			switch(bij) {
				case 0: {
				LOGSTRINGTAG("2: recv==0\n");
				return false;
				};
				case -1: {
					lerrortag("2: recv");
					return false;
					}
				}
			len+=bij;
			}
		LOGSTRINGTAG("voor ascon_aead128a_decrypt_update\n");
		if(totlen>testlen)
			res+= ascon_aead128a_decrypt_update(ctx, uit+res, incrypt, bijlen);
		bool is_tag_valid;
		res += ascon_aead128a_decrypt_final( ctx,uit+res, &is_tag_valid, tag, taglen);
		if(!is_tag_valid) {		
			LOGSTRINGTAG("ascon_aead128a_decrypt_final returns invalid\n");
			return false;
			}
		else
			LOGSTRINGTAG("ascon_aead128a_decrypt_final valid\n");
		status.ininterpret=true;
		if(int res=interpretcommands(sock,host,ctx,uit+intlen,datlen)) {
			if(res==1)
				return true;
			return false;
			}
		status.ininterpret=false;
		}	
	}
template <int nr>
using unique_al= std::unique_ptr<uint8_t[],ardeleter<nr,uint8_t>> ;
//std::unique_ptr<uint8_t[],ardeleter<4,uint8_t>>  receivedatanopass(int sock,const int totlen) {
static unique_al<4> receivedatanopass(int sock,const int totlen) {
	LOGGERTAG("receivedatanopass(%d,%d)\n",sock,totlen);
	uint8_t *buf=new(std::align_val_t(4),std::nothrow) uint8_t[totlen];
	if(!buf) {
		sleep(1);
		return unique_al<4>(nullptr);
		}
	unique_al<4> destructptr(buf,ardeleter<4,uint8_t>());
	for(int took=0,bij;took<totlen;took+=bij) {
		bij=recvni(sock,buf+took,totlen-took);
		switch(bij) {
			case -1:lerrortag("recv");return unique_al<4>(nullptr);
			case 0:LOGSTRINGTAG("recv==0\n"); return unique_al<4>(nullptr);


			};
		}
	return destructptr;
	}

dataonlyptr receivedataonly(int sock, crypt_t *ctx,const int len) {
 	int totlen=aligner<alignof(dataonly)>(sizeof(dataonly)+len);
	auto dat=receivedata(sock,ctx, totlen) ;
	dataonlyptr destructptr(reinterpret_cast<dataonly*>(dat.get()),arcastdeleter<4,dataonly>());
	dat.release();
	return destructptr;
	}
unique_al<4>  receivedatapass(int sock,crypt_t *ctx,int messlen) {
	int havelen=sizeof(int)+messlen;
	const int takelen= (havelen<16)?16:havelen;
	constexpr int taglen=16;
	int totlen=taglen+takelen;

	uint8_t *buf=new(std::align_val_t(4),std::nothrow) uint8_t[totlen];
	if(!buf) {
		sleep(1);
		return unique_al<4>(nullptr);
		}
	unique_al<4> destructptr(buf,ardeleter<4,uint8_t>());
	uint8_t *start=buf+taglen;
	if(int len=recvni(sock,buf,totlen);len!=totlen) {
		flerrortag("recv(%d,,%d)!=%d",sock,totlen,len);
		return unique_al<4>(nullptr);
		}
	uint8_t *tmpbuf=new(std::align_val_t(4),std::nothrow) uint8_t[takelen];
	if(!tmpbuf) {
		sleep(1);
		return unique_al<4>(nullptr);
		}
	unique_al<4> dtmpbuf(tmpbuf,ardeleter<4,uint8_t>());
	int res=ascon_aead128a_decrypt_update(ctx, tmpbuf, start, takelen);
	bool is_tag_valid;
	res += ascon_aead128a_decrypt_final( ctx,tmpbuf+res, &is_tag_valid, buf, taglen);
	if(!is_tag_valid) {		
		LOGSTRINGTAG("ascon_aead128a_decrypt_final returns invalid tag\n");
		return unique_al<4>(nullptr);
		}

	uint8_t *uit=new(std::align_val_t(4),std::nothrow) uint8_t[messlen];
	if(!uit) {
		sleep(1);
		return unique_al<4>(nullptr);
		}
	unique_al<4> uitu(uit,ardeleter<4,uint8_t>());
	memcpy(uit,tmpbuf+sizeof(int),messlen);
	return uitu;
	}
unique_al<4> receivedata(int sock, crypt_t *ctx,const int len) {
	if(ctx==nullptr) 
		return receivedatanopass( sock,len);
	else return receivedatapass( sock,ctx, len) ;
	}
/*
inline void show(uint8_t *dat,int len) {
	for(int i=0;i<len;i++)
		fprintf(stderr,"%x ",dat[i]);
	fprintf(stderr,"\n");
	}
	*/

bool	receivepassinit(int sock,passhost_t *host,ascon_aead_ctx_t *ctx) {
	LOGGERTAG("sock=%d receivepassinit %s\n",sock,host->getnameif());
	constexpr int takelen=8;
	uint8_t nonce[ASCON_AEAD_NONCE_LEN];
	int len=recvni(sock,nonce,takelen);
	if(len!=takelen) {
		lerrortag("recv");
		return false;
		}
	
	constexpr int makelen=ASCON_AEAD_NONCE_LEN-takelen;
	uint8_t *makestart=nonce+takelen;
       makerandom(makestart, makelen);	
       if(sendni(sock,makestart,makelen)!=makelen) {
		lerrortag("receivepassinit send");
		return false;
		}
	if(ctx) {
		ascon_aead128a_init(ctx, host->pass.data(),nonce);	
		}
	else {
		LOGAR("ctx==null");
		}
	LOGGERTAG("end sock=%d receivepassinit\n",sock);
	return true;
	}



static bool	getcommandspassinit(int sock,passhost_t *host) {
	ascon_aead_ctx_t ctx;
	if(!receivepassinit(sock,host,&ctx)) 
		return false;  //NO cleanup, because no init
	const bool ret=getcom(sock,host,&ctx) ;
	ascon_aead_cleanup(&ctx);
	return ret;
	}
bool	getcommands(int sock,passhost_t *host) {
      LOGGERTAG("getcommands(%d)\n",sock);
	if(host->haspass())
		return getcommandspassinit(sock,host);
	else {
		return getcommandsnopass(sock,host);
		}
	}
bool	activegetcommands(int sock,passhost_t *host,crypt_t *ctx) {
	if(ctx) {
		bool res=getcom(sock,host,ctx);
		ascon_aead_cleanup(ctx);
		return res;
		}
	return getcommandsnopass(sock,host);
	}

static std::pair<int,int> getstartinfo(const struct fileonce_t *gegs,const uint8_t *start) {
	const uint8_t *buf= reinterpret_cast<const uint8_t*>(gegs);
	int tothier= aligner<4>(start-buf);
	int over=gegs->totlen-tothier;
	int startpos=over==sizeof(int)?*reinterpret_cast<const int *>(buf+tothier):-1;
	int sendindex=gegs->dowith&0x3FFF;
	LOGGERTAG("tothier=%d totlen=%d startpos=%d sendindex=%d\n",tothier,gegs->totlen,startpos,sendindex);
	return {sendindex,startpos};
	}	
void sethistorystart(int,int);
static bool savefileonce(const struct fileonce_t *gegs) {
	const int nr=gegs->nr;
	const uint8_t *start=reinterpret_cast<const uint8_t*>(&gegs->gegs[nr]);
	const char *name=reinterpret_cast<const char *>(start);
	int fp=filedata.open(name);
	LOGGERTAG("savefileonce %s %d show=%d\n",name,nr, (gegs->dowith&streamupdatebit));
	if(fp<0)
		return false;
	destruct des([fp](){filedata.close(fp);});
	start+=(gegs->namelen);
	for(int i=0;i<nr;i++) {
		if(!filedata.savedata(fp,gegs->gegs[i].off,gegs->gegs[i].len,start)) {
			LOGSTRINGTAG("savedata failed\n");
			return false;
			}
		start+=gegs->gegs[i].len;
		}
	if((gegs->dowith&streamupdatebit)) {
		const auto [sendindex,startpos]=getstartinfo(gegs,start);
		processglucosevalue(sendindex,startpos);
		}
	else {
		if((gegs->dowith&starthistoryupdate)) {
			const auto [sendindex,startpos]=getstartinfo(gegs,start);
			sethistorystart(sendindex,startpos);
			}
		}
	LOGSTRINGTAG("savedata success\n");
	return true;
	}
/*
bool turnreceiver(int sock,passhost_t *hostptr,crypt_t *ctxptr) {
	if(ctxptr) {
		bool ret=getcom(sock, hostptr,ctxptr);
		ascon_aead_cleanup(ctxptr);
		return ret;
		}
	else {
		if(getcommandsnopass(sock,hostptr) )
			return true;
		}
	return false;
	}
	*/
#ifdef MAIN
void render() {}
void remakenums() {}
#endif
