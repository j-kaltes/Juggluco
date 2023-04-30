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


#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <string_view>
#include <memory>
       #include <unistd.h>

       #include <sys/types.h>
       #include <sys/socket.h>
       #include <new>
       #include <assert.h>

#include "backup.h"
#include "inout.h"
#include "logs.h"
#include "netstuff.h"
//#define LOGGERTAG(...) fprintf(stderr,__VA_ARGS__)
#define lerrortag(...) lerror("sendcommands: " __VA_ARGS__)
#define LOGGERTAG(...) LOGGER("sendcommands: " __VA_ARGS__)
#define flerrortag(...) flerror("sendcommands: " __VA_ARGS__)

#include "aligner.h"

bool receivecrypt(int sock,crypt_t *ctx,uint8_t *uit) {
	constexpr int taglen=16;
	const int alllen=taglen+4;
	uint8_t buf[alllen];
	uint8_t *start=buf+taglen;
	if(int len=recvni(sock,buf,alllen);len!=alllen) {
		LOGGERTAG("receivecrypt %d, shutdown %d\n",len,sock);
		::shutdown(sock,SHUT_RDWR);
		return false;
		}
	int res=ascon_aead128a_decrypt_update(ctx, uit, start, 4);
	bool is_tag_valid;
	res += ascon_aead128a_decrypt_final( ctx,uit+res, &is_tag_valid, buf, taglen);
	return is_tag_valid;
	}
int16_t sendopen(crypt_t *pass,int sock,std::string_view name) {
	const int namelen=name.size();
	const int buflen=aligner<4>(sizeof(fileopen)+namelen+1);
	alignas(alignof(fileopen)) senddata_t buf[buflen];
	struct fileopen *command=new(buf) fileopen; 
	command->com=sopen;
	command->len=namelen;
	memcpy(command->name,name.data(),namelen);
	command->name[namelen]='\0';
//	puts(command->name);
	LOGGERTAG("sendopen %s ",name.data());
	bool noacksendcommand(crypt_t*,int sock ,const unsigned char *buf,int buflen) ;
	if(!noacksendcommand(pass,sock ,buf,buflen)) {
		LOGGERTAG("open %s failed\n",name.data());	
		return -1;
		}
	LOGGERTAG("sendopen after sendcommand\n");
	if(pass) {	
		 if(!receivecrypt(sock,pass,buf))  {
		 	LOGGERTAG("invalid tag\n");
		 	return -1;
			}
		}
	else  {
		if(int len=recvni(sock,buf,4);len!=4) {
			if(len==-1) {
				flerrortag(" recv(,,%d,)==-1,shutdown %d",buflen,sock);
				::shutdown(sock,SHUT_RDWR);
				}
			else
				LOGGERTAG(" wrong size %d\n",len);
			return -1;
			}
		}
	int16_t *fps=reinterpret_cast<int16_t *>(buf);
	int16_t fp=*fps;
	LOGGERTAG("fp=%d, %hx\n",fp,fps[1]); 
	if(((~fp)&0xFFFF)!=(0xFFFF&fps[1])) {
		LOGGERTAG("Transform wrong \n");
		return -1;	
		}
	return fp;
	}
int mklensize() {
	return sizeof(struct mklen);
	}
senddata_t *mklencom(unsigned char *bufin,int16_t han,uint32_t len) {
	unsigned char *buf=aligner<4>(bufin);
	*reinterpret_cast<struct mklen*>(buf)={smklen,han,len};
	return buf+ sizeof(struct mklen);
	}

constexpr std::align_val_t offwritealign= std::align_val_t(alignof(offwrite));
struct offwritealign_deleter { // deleter
    void operator() ( unsigned char ptr[]) {
	operator delete[] (ptr, offwritealign);
    }
};
auto datasize(const uint32_t len) {
	return sizeof(struct offwrite)+aligner<4>(len);
	}

senddata_t *datacom(unsigned char *bufin,int16_t han,uint32_t off,uint32_t len,const unsigned  char *data) {
	unsigned char *buf=aligner<4>(bufin);
	struct offwrite *offw=reinterpret_cast<offwrite *>(buf);
	*offw={swrite,han,off,len};
	memcpy(offw->data,data,len);
	return offw->data+len;
	}


bool noacksendcommand(int sock ,const unsigned char *buf,int buflen) {
	int itlen,left=buflen;
	LOGGERTAG("sock=%d noacksendcommand len=%d\n",sock,buflen);
	for(const unsigned char *it=buf;(itlen=sendni(sock,it,left))<left;) {
		LOGGERTAG("len=%d\n",itlen);
		if(itlen<0) {
			flerrortag("noacksendcommand send %d\n",sock);
			::shutdown(sock,SHUT_RDWR);
			return false;
			}
		it+=itlen;
		left-=itlen;
		}
	LOGGERTAG("success noacksendcommand\n");
	return true;
	}
bool getack(int sock) {
	uint32_t ans=5;
	LOGGERTAG("getack\n");
	if(int len=recvni(sock,&ans,sizeof(ans));len!=sizeof(ans)) {
		flerrortag("%d ans %d\n",sock,len);
		::shutdown(sock,SHUT_RDWR);
		return false;
		}
	if(ans!=ackres) {
		LOGGERTAG("ackres %u!=%u\n",ans,ackres);
		return false;
		}
	LOGGERTAG("getack success\n");
	return true;
	}
bool sendcommand(int sock ,const unsigned char *buf,int buflen) {
	if(!noacksendcommand(sock ,buf, buflen) )
		return false;
	const int alin=aligner<4>(buflen);
	const sendack ack;
	if(alin>buflen) {
		const int over=alin-buflen;
	    uint8_t ackbuf[over+sizeof(sendack)];
	    *reinterpret_cast<sendack*>(ackbuf+over)=ack;
		if(sendni(sock,&ackbuf,sizeof(ackbuf))!=sizeof(ackbuf)) {
			lerrortag("sendcommand send(ackbuf...) failed");
			return false;
			}

	}
	else
		if(sendni(sock,&ack,sizeof(ack))!=sizeof(ack)) {
			lerrortag("sendcommand send(ack..) failed");
			return false;
			}
	return getack(sock);
	}
struct com_t {
	uint16_t com;
	int16_t han;
	}; 
#include "receive.h"
bool sendfile(int sock,crypt_t *pass,const char *filename,uint32_t off,uint32_t len) {
	LOGGERTAG("sendfile %s %u %u ",filename,off,len);
 	int totlen=aligner<alignof(dataonly)>(sizeof(dataonly)+len);
	std::unique_ptr<senddata_t[],ardeleter<4,senddata_t>> destructptr(new(std::align_val_t(4),std::nothrow) senddata_t[totlen],ardeleter<4,senddata_t>());
	struct dataonly* data=reinterpret_cast<struct dataonly*>(destructptr.get());
	if(!data) {
		sleep(1);
		return false;
		}
extern getdata filedata;
	int fp=filedata.openread(filename),got=-4;

	if(fp>=0&&(off==0||(got=lseek(fp,off,SEEK_SET))==off)) {
		errno=0;
		data->len=read(fp,data->data,len);
		if(len!=data->len)
			flerrortag("read(%d)=%d",len,data->len);
		}
	else {
		flerrortag("fp=%d got=%d\n",fp,got);
		data->len=-1;
		}
	filedata.close(fp);
	return noacksendcommand(pass,sock,destructptr.get(),totlen);
	}
	
	

bool sendcommandpass(ascon_aead_ctx_t *ctx,int sock ,const unsigned char *buf,int buflen,bool askack) {
	LOGGERTAG("sendcommandpass %d %d\n",sock,buflen);
	constexpr int taglen=16;
	sendack ack;
	int havelen=sizeof(int)+buflen;
	int tussen=0;
	int comlen;
	if(askack) {
		tussen=aligner<4>(havelen)-havelen;
		havelen+=tussen+sizeof(ack);
		comlen=buflen+tussen+sizeof(ack);
		}
	else
		comlen=buflen;
	const int takelen= (havelen<16)?16:havelen;
	int totlen=taglen+takelen;
	std::unique_ptr<senddata_t[],ardeleter<4,senddata_t>> destructptr(new(std::align_val_t(4),std::nothrow) senddata_t[totlen],ardeleter<4,senddata_t>());
	senddata_t *allbuf=destructptr.get();		
	if(!allbuf) {
		sleep(1);
		return false;
		}
//	*reinterpret_cast<int *>(allbuf+taglen)=buflen;
	senddata_t *startdata=allbuf+taglen;
	size_t new_ct_bytes = ascon_aead128a_encrypt_update(ctx, startdata,reinterpret_cast<uint8_t*>(&comlen) ,sizeof(int));
	new_ct_bytes += ascon_aead128a_encrypt_update(ctx, startdata+new_ct_bytes, buf, buflen);
	if(askack) {
		if(tussen)
			new_ct_bytes += ascon_aead128a_encrypt_update(ctx, startdata+new_ct_bytes, reinterpret_cast<const uint8_t *>(zeros), tussen);
		new_ct_bytes += ascon_aead128a_encrypt_update(ctx, startdata+new_ct_bytes,reinterpret_cast<const uint8_t *>( &ack), sizeof(ack));
		}
	const int erbij=takelen-havelen;
	if(erbij>0)
		new_ct_bytes += ascon_aead128a_encrypt_update(ctx, startdata+new_ct_bytes, reinterpret_cast<const uint8_t *>(zeros), erbij);
	ascon_aead128a_encrypt_final(ctx, startdata + new_ct_bytes, allbuf, taglen);
	if(!noacksendcommand(sock,allbuf,totlen))
		return false;
	if(askack)
		return getack(sock);
	return true;
	}
bool sendcommand(crypt_t *pass,int sock ,const unsigned char *buf,int buflen) {
	if(!pass)
		return sendcommand(sock,buf,buflen);
	else
		return sendcommandpass(pass,sock,buf,buflen,true);
	}
bool noacksendcommand(crypt_t *pass,int sock ,const unsigned char *buf,int buflen) {
	if(!pass)
		return noacksendcommand(sock,buf,buflen);
	else
		return sendcommandpass(pass,sock,buf,buflen,false);
	}


int closesize() {
	return sizeof(com_t);
	}
senddata_t* closecom(unsigned char *bufin,int16_t han) {
	unsigned char *buf=aligner<4>(bufin);
	*reinterpret_cast<struct com_t*>(buf)= {sclose,han};
	return buf+ sizeof(struct com_t);	
	}
bool sendone(crypt_t *pass,const int sock, const uint32_t com) {
	LOGGERTAG("sendone %d\n",com);
	return sendcommand(pass,sock,reinterpret_cast<const senddata_t *>(&com),4);
	}
bool noacksendone(crypt_t *pass,const int sock, const uint32_t com) {
	LOGGERTAG("noacksendone %d\n",com);
	return  noacksendcommand(pass,sock ,reinterpret_cast<const senddata_t *>(&com),4);
	}
	
bool sendbackupstop(crypt_t *pass,const int sock) {
	return  noacksendone(pass,sock, sbackupstop) ;
//	return sendone(pass,sock,sbackupstop);
/*
	constexpr const	uint32_t com= sbackupstop;
	return  noacksendcommand(pass,sock ,reinterpret_cast<const senddata_t *>(&com),4);*/
	}
bool sendbackup(crypt_t *pass,const int sock) {
	return noacksendone(pass,sock,sbackup);
	}
bool sendwakeupstream(crypt_t *pass,const int sock) {
	return noacksendone(pass,sock,swakeupstream);
	}
	/*
bool sendrenum(crypt_t *pass,const int sock) {
	return sendone(pass,sock,srenum);
	} */
bool sendrender(crypt_t *pass,const int sock) {
	return sendone(pass,sock,srender);
/*
	const uint32_t com=srender;
	return sendcommand(sock,reinterpret_cast<const senddata_t *>(&com),4);*/
	}
bool sendshowglucose(crypt_t *pass,const int sock,const uint16_t sensorindex) {
	struct renderstruct rend{sglucose,sensorindex};
	LOGGERTAG("sendshowglucose(pass,%d,%d)\n",sock,sensorindex);
	return sendcommand(pass,sock,reinterpret_cast<const senddata_t *>(&rend),sizeof(struct renderstruct));
	}
bool sendrender(crypt_t *pass,const int sock,const uint16_t type) {
	struct renderstruct rend{srender,type};
	LOGGERTAG("sendrender(pass,%d,%d)\n",sock,type);
	return sendcommand(pass,sock,reinterpret_cast<const senddata_t *>(&rend),sizeof(struct renderstruct));
/*
	const uint32_t com=srender;
	return sendcommand(sock,reinterpret_cast<const senddata_t *>(&com),4);*/
	}
	
/*	
bool oldsenddata(crypt_t *pass,const int sock,const std::vector<subdata>&data,const std::string_view naar) {
	LOGGERTAG("oldsenddata vect\n");
	if(data.size()==0)
		return true;
 	if(int16_t han=sendopen(pass,sock,naar);han>0) {
		int buflen=closesize();
		for(auto &el:data) {
			buflen+=datasize(el.datalen);	
			}
	        std::unique_ptr<senddata_t[],ardeleter<4,senddata_t>> destructptr(new(std::align_val_t(4),std::nothrow) senddata_t[buflen],ardeleter<4,senddata_t>());
		senddata_t *buf=destructptr.get(),*ptr=buf;
		if(!buf) {
			sleep(1);
			return false;
			}
		for(auto &el:data) {
			ptr=datacom(ptr,han,el.offset,el.datalen,el.data); 
			}
#ifndef NDEBUG
		auto endcom=
#endif
		closecom(ptr,han);
		assert(endcom==(buflen+buf));
		return sendcommand(pass,sock,buf,buflen);
		}
	return false;
	}
bool oldsenddata(crypt_t *pass,const int sock,const int offset,const senddata_t *data,const int datalen,const string_view naar) {
	LOGGERTAG("oldsenddata\n");
 	if(int16_t han=sendopen(pass,sock,naar);han>0) {
		int buflen=datasize(datalen)+closesize();
	        std::unique_ptr<senddata_t[],ardeleter<4,senddata_t>> ptr(new(std::align_val_t(4),std::nothrow) senddata_t[buflen],ardeleter<4,senddata_t>());
		senddata_t *buf=ptr.get();
		if(!buf) {
			sleep(1);
			return false;
			}
		auto tus=datacom(buf,han,offset,datalen,data); 
#ifndef NDEBUG
		auto endcom=
#endif
		closecom(tus,han);
		assert(endcom==(buflen+buf));
		return sendcommand(pass,sock,buf,buflen);
		}
	return false;	
	}
*/

bool newsenddata(crypt_t *pass,const int sock,const std::vector<subdata>&data,const std::string_view naar,uint16_t dowith,const uint8_t *extra,int extralen) {
	if(data.size()==0)
		return true;
	const int elnr= data.size();
	const int namelen=naar.size()+1;
	int buflen=sizeof(datel)*elnr+namelen+sizeof(fileonce_t);
	for(auto &el:data) {
		LOGGERTAG("ellen=%d\n",el.datalen);
		buflen+=el.datalen;	
		}
	buflen=aligner<4>(buflen)+extralen;
	std::unique_ptr<senddata_t[],ardeleter<4,senddata_t>> destructptr(new(std::align_val_t(4),std::nothrow) senddata_t[buflen],ardeleter<4,senddata_t>());
	senddata_t *buf=destructptr.get();
	if(!buf) {
		sleep(1);
		return false;
		}
	fileonce_t *stru=reinterpret_cast<fileonce_t*>(buf);
	stru->com=sfileonce;
	stru->namelen=namelen;
	stru->nr=elnr;
	stru->dowith=dowith;
	senddata_t *ptr=reinterpret_cast<senddata_t  *>(stru->gegs+elnr);
	memcpy(ptr,naar.data(),namelen-1);
	ptr[namelen-1]='\0';
	ptr+=namelen;
	datel *datar=stru->gegs;
	for(int i=0;i<elnr;i++) {
		const subdata &el=data[i];
		datar[i]={el.offset,el.datalen};
		memcpy(ptr,el.data,el.datalen);
		ptr+=el.datalen;
		}
	if(extralen>0) {
		memcpy(buf+buflen-extralen,extra,extralen);
		}
	stru->totlen=buflen;	
	LOGGERTAG("senddata vect %s elnr=%d namelen=%d buflen=%d ptr-buf=%d extralen=%d dowith=%d\n",naar.data(),elnr,namelen,buflen,(int) (ptr-buf),extralen,dowith);
	return sendcommand(pass,sock,buf,buflen);
	}

bool newsenddata(crypt_t *pass,const int sock,const int offset,const senddata_t *data,const int datalen,const string_view naar,uint16_t dowith,const uint8_t *extra,int extralen) {
	std::vector<subdata> vect;
	vect.reserve(1);
	vect.push_back({data,offset,datalen});
	return  senddata(pass,sock,vect,naar,dowith,extra,extralen);
	}

bool senddata(crypt_t *pass,const int sock,const std::vector<subdata>&data,const std::string_view naar,uint16_t dowith,const uint8_t *extra,int extralen) {
	return newsenddata(pass,sock,data,naar,dowith,extra,extralen) ;
	}
//static thread_local uint8_t receiverversion=0;
bool senddata(crypt_t *pass,const int sock,const int offset,const senddata_t *data,const int datalen,const string_view naar,uint16_t dowith,const uint8_t *extra,int extralen) {
		return newsenddata(pass,sock,offset,data,datalen,naar,dowith,extra,extralen) ;
	}
	/*
bool senddata(crypt_t *pass,const int sock,const int offset,const senddata_t *data,const int datalen,const string_view naar) {
 	if(int16_t han=sendopen(pass,sock,naar);han>0) {
		int buflen=datasize(datalen)+closesize();
	        std::unique_ptr<senddata_t[],ardeleter<4,senddata_t>> ptr(new(std::align_val_t(4)) senddata_t[buflen],ardeleter<4,senddata_t>());
		senddata_t *buf=ptr.get();
		auto tus=datacom(buf,han,offset,datalen,data); 
		auto endcom=closecom(tus,han);
		assert(endcom==(buflen+buf));
		return sendcommand(pass,sock,buf,buflen);
		}
	return false;	
	} */

	#ifdef MAIN
bool sendcommands(int sock) {
	int len=462;
	int off=12278;
	senddata_t buf[12278+len];
	for(int i=0;i<len;i++) {
		buf[i+off]=i%256;
		}
	if(!senddata(sock,off,buf+off,len, "testfile.dat")) {
		fprintf(stderr,"senddata failed\n");
		return false;
		}
	return true;
	}
	#endif
