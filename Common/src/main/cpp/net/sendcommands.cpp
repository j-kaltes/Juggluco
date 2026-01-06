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

#include "backup.hpp"
#include "inout.hpp"
#include "logs.hpp"
#include "netstuff.hpp"
#include "Connect.hpp"
//#define LOGGERTAG(...) fprintf(stderr,__VA_ARGS__)
#define lerrortag(...) lerror("sendcommands: " __VA_ARGS__)
#define LOGGERTAG(...) LOGGER("sendcommands: " __VA_ARGS__)
#define LOGARTAG(...) LOGAR("sendcommands: " __VA_ARGS__)
#define LOGSTRINGTAG(...) LOGSTRING("sendcommands: " __VA_ARGS__)
#define flerrortag(...) flerror("sendcommands: " __VA_ARGS__)

#include "aligner.hpp"

bool Connect::receivecrypt(crypt_t *ctx,uint8_t *uit) {
	constexpr int taglen=16;
	const int alllen=taglen+4;
	uint8_t buf[alllen];
	uint8_t *start=buf+taglen;
	if(int len=s_recvni(buf,alllen);len!=alllen) {
		LOGGERTAG("receivecrypt %d, restartSender %d\n",len,getSenderIdent());
		restartSender();
		return false;
		}
	int res=ascon_aead128a_decrypt_update(ctx, uit, start, 4);
	bool is_tag_valid;
	res += ascon_aead128a_decrypt_final( ctx,uit+res, &is_tag_valid, buf, taglen);
	return is_tag_valid;
	}
        /*
int16_t Connect::sendopen(crypt_t *pass,std::string_view name) {
	const int namelen=name.size();
	const int buflen=aligner<4>(sizeof(fileopen)+namelen+1);
	alignas(alignof(fileopen)) senddata_t buf[buflen];
	struct fileopen *command=new(buf) fileopen; 
	command->com=sopen;
	command->len=namelen;
	memcpy(command->name,name.data(),namelen);
	command->name[namelen]='\0';
	LOGGERTAG("sendopen %s ",name.data());
	if(!noacksendcommand(pass,buf,buflen)) {
		LOGGERTAG("open %s failed\n",name.data());	
		return -1;
		}
	LOGARTAG("sendopen after sendcommand");
	if(pass) {	
		 if(!receivecrypt(pass,buf))  {
		 	LOGARTAG("invalid tag");
		 	return -1;
			}
		}
	else  {
		if(int len=s_recvni(buf,4);len!=4) {
			if(len==-1) {
				flerrortag(" recv(,,%d,)==-1,restartSender %d",buflen,getSenderIdent());
				restartSender();
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
		LOGARTAG("Transform wrong ");
		return -1;	
		}
	return fp;
	}
        */
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
/*bool Connect::noacksendcommand(const unsigned char *buf,int buflen) {
	int itlen,left=buflen;
	LOGGERTAG("getSenderIdent()=%d s_noacksendcommand len=%d\n",getSenderIdent(),buflen);
	for(const unsigned char *it=buf;(itlen=s_sendni(it,left))<left;) {
		LOGGERTAG("len=%d\n",itlen);
		if(itlen<0) {
			flerrortag("s_noacksendcommand send %d\n",getSenderIdent());
			restartSender();
			return false;
			}
		it+=itlen;
		left-=itlen;
		}
	LOGARTAG("success s_noacksendcommand");
	return true;
	}
    template <int (Ex::*func)(int)>
*/

template<Connect::sendni_type sendni,Connect::getIdent_type getIdent>
bool Connect::noacksendcommand(const unsigned char *buf,int buflen) {
	int itlen,left=buflen;
	LOGGERTAG("getIdent()=%d noacksendcommand len=%d\n",(this->*getIdent)(),buflen);
	for(const unsigned char *it=buf;(itlen=(this->*sendni)(it,left))<left;) {
		LOGGERTAG("noacksendcommand sendni(%p,%d)=%d\n",it,left,itlen);
		if(itlen<0) {
			flerrortag("noacksendcommand send failed %d\n",(this->*getIdent)());
			restartSender();
			return false;
			}
		it+=itlen;
		left-=itlen;
		}
	LOGARTAG("success noacksendcommand");
	return true;
	}
bool Connect::s_noacksendcommandonly(const unsigned char *buf,int buflen) {
        return noacksendcommand<&Connect::s_sendni,&Connect::getSenderIdent>(buf, buflen);
        }
bool Connect::getack() {
	uint32_t ans=5;
	LOGARTAG("getack");
	if(int len=s_recvni(&ans,sizeof(ans));len!=sizeof(ans)) {
		flerrortag("%d ans %d\n",getSenderIdent(),len);
		restartSender();
		return false;
		}
	if(ans!=ackres) {
		LOGGERTAG("ackres %u!=%u\n",ans,ackres);
		return false;
		}
	LOGARTAG("getack success");
	return true;
	}
bool Connect::sendcommand(const unsigned char *buf,int buflen) {
	if(!s_noacksendcommandonly(buf, buflen) )
		return false;
	const int alin=aligner<4>(buflen);
	const sendack ack;
	if(alin>buflen) {
		const int over=alin-buflen;
	    uint8_t ackbuf[over+sizeof(sendack)];
	    *reinterpret_cast<sendack*>(ackbuf+over)=ack;
         LOGGER("sendack buflen=%d acklen=%d\n",buflen,sizeof(ackbuf));
		if(s_sendni(&ackbuf,sizeof(ackbuf))!=sizeof(ackbuf)) {
			lerrortag("sendcommand send(ackbuf...) failed");
			return false;
			}

	}
	else {
        LOGGER("sendack buflen=%d acklen=%d\n",buflen,sizeof(ack));
		if(s_sendni(&ack,sizeof(ack))!=sizeof(ack)) {
			lerrortag("sendcommand send(ack..) failed");
			return false;
			}
        }
	return getack();
	}
struct com_t {
	uint16_t com;
	int16_t han;
	}; 
	
template <Connect::noacksendcommand_type noacksendcommand>
bool Connect::gensendcommandpass(ascon_aead_ctx_t *ctx,const unsigned char *buf,int buflen,bool (Connect::*getack)()) {
	LOGGERTAG("sendcommandpass %d %d\n",getSenderIdent(),buflen);
	constexpr int taglen=16;
	sendack ack;
	int havelen=sizeof(int)+buflen;
	int tussen=0;
	int comlen;
	if(getack) {
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
	if(getack) {
		if(tussen)
			new_ct_bytes += ascon_aead128a_encrypt_update(ctx, startdata+new_ct_bytes, reinterpret_cast<const uint8_t *>(zeros), tussen);
		new_ct_bytes += ascon_aead128a_encrypt_update(ctx, startdata+new_ct_bytes,reinterpret_cast<const uint8_t *>( &ack), sizeof(ack));
		}
	const int erbij=takelen-havelen;
	if(erbij>0)
		new_ct_bytes += ascon_aead128a_encrypt_update(ctx, startdata+new_ct_bytes, reinterpret_cast<const uint8_t *>(zeros), erbij);
	ascon_aead128a_encrypt_final(ctx, startdata + new_ct_bytes, allbuf, taglen);
	if(!(this->*noacksendcommand)(allbuf,totlen))  {
		return false;
		}
	if(getack)
		return (this->*getack)();
	return true;
	}

bool Connect::s_sendcommandpass(ascon_aead_ctx_t *ctx,const unsigned char *buf,int buflen,bool askack) {
        return gensendcommandpass<&Connect::s_noacksendcommandonly>(ctx,buf, buflen, askack?&Connect::getack:nullptr);
        }
bool Connect::sendcommand(crypt_t *pass,const unsigned char *buf,int buflen) {
	if(!pass)
		return sendcommand(buf,buflen);
	else
		return s_sendcommandpass(pass,buf,buflen,true);
	}
bool Connect::s_noacksendcommand(crypt_t *pass,const unsigned char *buf,int buflen) {
	if(!pass)
		return s_noacksendcommandonly(buf,buflen);
	else
		return s_sendcommandpass(pass,buf,buflen,false);
	}


int closesize() {
	return sizeof(com_t);
	}
senddata_t* closecom(unsigned char *bufin,int16_t han) {
	unsigned char *buf=aligner<4>(bufin);
	*reinterpret_cast<struct com_t*>(buf)= {sclose,han};
	return buf+ sizeof(struct com_t);	
	}
bool Connect::sendone(crypt_t *pass, const uint32_t com) {
	LOGGERTAG("sendone %d\n",com);
	return sendcommand(pass,reinterpret_cast<const senddata_t *>(&com),4);
	}
bool Connect::noacksendone(crypt_t *pass, const uint32_t com) {
	LOGGERTAG("noacksendone %d\n",com);
	return  s_noacksendcommand(pass,reinterpret_cast<const senddata_t *>(&com),4);
	}
	
bool Connect::sendbackupstop(crypt_t *pass) {
	return  noacksendone(pass, sbackupstop) ;
	}
bool Connect::sendResetDevices(crypt_t *pass) {
	const bool ret= noacksendone(pass, sresetdevices) ;
	LOGGER("sendResetDevices(pass,%d)=%d\n",getSenderIdent(),ret);
	return ret;
	}
bool Connect::sendbackup(crypt_t *pass) {
	return noacksendone(pass,sbackup);
	}
bool Connect::sendwakeupstream(crypt_t *pass) {
	return noacksendone(pass,swakeupstream);
	}
	/*
bool sendrenum(crypt_t *pass,const int sock) {
	return sendone(pass,sock,srenum);
	} */
bool Connect::sendrender(crypt_t *pass) {
	return sendone(pass,srender);
	}

 bool    Connect::senduint16(crypt_t*pass,uint16_t com,uint16_t arg) {
    uint16_t_arg_struct data{com,arg};
    return sendcommand(pass,reinterpret_cast<uint8_t*>(&data),sizeof(data));
    }
bool Connect::sendStartSendCalibrate(crypt_t *pass,const uint16_t sensorindex) {
    LOGGERTAG("sendStartSendCalibrate getSenderIdent()=%d sensorindex=%hd\n",getSenderIdent(),sensorindex);
    return senduint16(pass,sStartSendCalibrate,sensorindex);
    }
bool Connect::sendBlueWatch(crypt_t *pass,int8_t stream,int8_t nums) {
    LOGGERTAG("sendBlueWatch getSenderIdent()=%d stream=%d nums=%d\n",getSenderIdent(),stream,nums);
    bluewatchstruct data{
         .com=sBlueWatch,
         .stream=stream,
         .nums=nums
         };
    return sendcommand(pass,reinterpret_cast<uint8_t*>(&data),sizeof(data));
    }

bool Connect::sendshowglucose(crypt_t *pass,const uint16_t sensorindex) {
	struct renderstruct rend{sglucose,sensorindex};
	LOGGERTAG("sendshowglucose(pass,%d,%d)\n",getSenderIdent(),sensorindex);
	return sendcommand(pass,reinterpret_cast<const senddata_t *>(&rend),sizeof(struct renderstruct));
	}
bool Connect::sendrender(crypt_t *pass,const uint16_t type) {
	struct renderstruct rend{srender,type};
	LOGGERTAG("sendrender(pass,%d,%x)\n",getSenderIdent(),type);
	return sendcommand(pass,reinterpret_cast<const senddata_t *>(&rend),sizeof(struct renderstruct));
	}
	

bool Connect::senddata(crypt_t *pass,const std::vector<subdata>&data,const std::string_view naar,uint16_t dowith,const uint8_t *extra,int extralen) {
	if(data.size()==0)
		return true;
	const int elnr= data.size();
	const int namelen=naar.size()+1;
	int buflen=sizeof(datel)*elnr+namelen+sizeof(fileonce_t);
	LOGGERTAG("start enddata vect %s elnr=%d\n",naar.data(),elnr);
	for(auto &el:data) {
		LOGGERTAG("offset=%d, ellen=%d\n",el.offset,el.datalen);
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
	return sendcommand(pass,buf,buflen);
	}

bool Connect::senddata(crypt_t *pass,const int offset,const senddata_t *data,const int datalen,const string_view naar,uint16_t dowith,const uint8_t *extra,int extralen) {
	std::vector<subdata> vect;
	vect.reserve(1);
	vect.push_back({data,offset,datalen});
	return  senddata(pass,vect,naar,dowith,extra,extralen);
	}
/*
bool Connect::senddata(crypt_t *pass,const std::vector<subdata>&data,const std::string_view naar,uint16_t dowith,const uint8_t *extra,int extralen) {
	return newsenddata(pass,data,naar,dowith,extra,extralen) ;
	}
bool Connect::senddata(crypt_t *pass,const int offset,const senddata_t *data,const int datalen,const string_view naar,uint16_t dowith,const uint8_t *extra,int extralen) {
		return newsenddata(pass,offset,data,datalen,naar,dowith,extra,extralen) ;
	}
*/

bool Connect::r_noacksendcommand(crypt_t *pass,const unsigned char *buf,int buflen) {
	if(!pass)
        return noacksendcommand<&Connect::r_sendni,&Connect::getReceiverIdent>(buf, buflen);
	else
        return gensendcommandpass<&Connect::noacksendcommand<&Connect::r_sendni,&Connect::getReceiverIdent>>(pass,buf, buflen, nullptr);
	}
