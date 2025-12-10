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
#include "receive.hpp"
#include "passhost.hpp"
#include "makerandom.hpp"
#include "crypt.h"
#include "netstuff.hpp"
#include "datbackup.hpp"
#include "aligner.hpp"
#include "mirrorerror.h"

#define lerrortag(...) lerror("getcommands: " __VA_ARGS__)
#define LOGGERTAG(...) LOGGER("getcommands: " __VA_ARGS__)
#define LOGARTAG(...) LOGAR("getcommands: " __VA_ARGS__)
#define LOGSTRINGTAG(...) LOGSTRING("getcommands: " __VA_ARGS__)
#define flerrortag(...) flerror("getcommands: " __VA_ARGS__)

extern Backup *backup;
extern void        processglucosevalue(int sendindex,int newstart=-1);

//getdata filedata("/data/local/tmp/testdir");
getdata filedata;


 bool Connect::sendcrypt(crypt_t *ctx,uint8_t *data,int datalen) {
    constexpr int taglen=16;
    const int alllen=taglen+datalen;
    uint8_t buf[alllen];
    uint8_t *start=buf+taglen;
    size_t wrote= ascon_aead128a_encrypt_update(ctx, start,data ,datalen);
    ascon_aead128a_encrypt_final(ctx, start + wrote, buf, taglen);
    if(r_sendni(buf,sizeof(buf))!=sizeof(buf)) {
        flerrortag("sendcrypt send restartReceiver %d",getReceiverIdent());
        restartReceiver();
        return false;
        }
    return true;
    }

 bool Connect::openfile(crypt_t *ctx,const char *name) {
    int fp=filedata.open(name);
    LOGGERTAG("open(%s)=%d\n",name,fp);
    int16_t sfp=(int16_t)fp;
    int16_t sendfp[2]={sfp,static_cast<int16_t>(((uint16_t)0xFFFF)&(~sfp))};
    if(ctx) {
        if(!sendcrypt(ctx, reinterpret_cast<uint8_t*>(sendfp),sizeof(sendfp)))  {
            filedata.close(fp); 
            LOGGERTAG("openfile restartReceiver %d\n",getReceiverIdent());
            restartReceiver();
            return false;
            }

        }
    else  {
        if(r_sendni(sendfp,sizeof(sendfp))!=sizeof(sendfp)) {
            lerrortag("openfile send +restartReceiver %d\n");
            filedata.close(fp); 
            restartReceiver();
            return false;
            }
        }
    return true;
    }

void    backupbase(string_view basedir) {
    filedata.setpath(basedir);
    }

extern    bool backupnums(const struct numsend* innums);
//cmd han uint32_t uint32_t unsigned byte
//bool savedata(int fp,uint32_t offset, uint32_t len,const unsigned char *data) {

#include "aligner.hpp"
//int alignadd(int was,int bij) { return was+((bij+3)/4)*4; }
/*
int alignadd(int was,int bij) {
//    return was+aligner<4>((unsigned long)bij);
    return was+aligner<4>(bij);
    }
    */
#define addlen(x,y) x+=y
//s/it+=comlen/it=alignadd(it,comlen)/g
extern bool receivelastpos(const lastpos_t *data) ;

static            bool savefileonce(const struct fileonce_t *gegs);



extern bool javaUpdateDevices();
static bool updateDevices() {
    LOGAR("before deletelast()");
    sensors->deletelast();
    LOGAR("after deletelast()");
    sensors->setindices();
    backup->resendResetDevices();
    return  javaUpdateDevices();
    }

/*
bool Connect::r_noacksendcommandonly(const unsigned char *buf,int buflen) {
        return noacksendcommand<&Connect::r_sendni,&Connect::getReceiverIdent>(buf, buflen);
        }
*/
/*
bool Connect::s_sendcommandpass(ascon_aead_ctx_t *ctx,const unsigned char *buf,int buflen,bool askack) {
        return gensendcommandpass<&Connect::s_noacksendcommandonly>(ctx,buf, buflen, askack?&Connect::getack:nullptr);
        } */
extern bool setBlueWatch(passhost_t *host,int sensor,int nums) ;


 std::pair<int,int> Connect::interpret(passhost_t *host,crypt_t *ctx,senddata_t *datain,int len) {

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
    LOGGERTAG("%d com=%s (%d) %d\n",getReceiverIdent(),command<size(comlabels)?comlabels[command]:"error",command,it);
 
    bool ret=false;
    if(!(host->receivefrom&2)&&command!=sBlueWatch&&command!=sbackupstop&&command!=swakeupstream&&command!=sbackup&&command!=sack)  {

      savemessage(host,"Other side sends, but I am not configured to receive");
        LOGARTAG("interpret: I don't receive from  this host");
        return {-1,0};
        }

        
    switch(command) {
        case sack: {
            comlen=sizeof(sendack);
            addlen(it,comlen);
            if(it>len) {
                return {it,comlen};
                }
            if(r_sendni(&ackres,sizeof(ackres))!=sizeof(ackres)) 
                return {-1,0};
            LOGARTAG("ack send");
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
            ret=openfile(ctx,com->name);
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
            LOGARTAG("swrite");
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
#ifdef            JUGGLUCO_APP
            case sBlueWatch:
                {comlen=sizeof(bluewatchstruct);
                addlen(it,comlen);
                if(it>len) {
                    return {it,comlen};
                    }
                const struct bluewatchstruct *bluewatch=reinterpret_cast<const struct bluewatchstruct*>(data);
                ret=setBlueWatch(host,bluewatch->stream,bluewatch->nums);
                }
                break;
#endif
            case sStartSendCalibrate:
                {comlen=sizeof(uint16_t_arg_struct);
            addlen(it,comlen);
            if(it>len) {
                return {it,comlen};
                }
            const uint16_t_arg_struct *stru=reinterpret_cast<const uint16_t_arg_struct*>(data);
                    extern void setCalibrates(uint16_t sensorindex) ;
                        setCalibrates(stru->arg);
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
extern                bool updateDevices() ;
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
            extern void wakeupall();
            wakeupall();
            break;
        case swakeupstream: 
            ret=true;
            comlen=4;
            addlen(it,comlen);
            if(it>len) {
                return {it,comlen};
                }

            extern void wakeupstream();
            wakeupstream();
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
            ret=sendfile(ctx,ask->name,ask->off,ask->len);
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
            ret=r_noacksendcommand(ctx, reinterpret_cast<uint8_t*>(&last),sizeof(last)) ;
            }; break;

        case sfileonce:
            {
            LOGARTAG("fileonce");
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
 
bool    Connect::getcommandsnopass(passhost_t *host) {
    int allindex=gethostindex(host);
    auto &status=mirrorstatus[allindex].receive;
    status.running(true);
    destruct _dest([&status]{status.running(false);});

    LOGGER("getcommandsnopass sock=%d\n",getReceiverIdent());
    int start=0;
    int maxcom =1024*1024;
    std::unique_ptr<senddata_t[],senddata_deleter> ptr(new (maxalign,std::nothrow) senddata_t [maxcom],senddata_deleter());

    senddata_t *com=ptr.get();
    if(!com) {
        sleep(1);
        return false;
        }

    while(true) {
        LOGARTAG("voor recv");
        int len=r_recvni(com+start,maxcom-start);
        LOGGERTAG("getReceiverIdent()=%d recv len=%d\n",getReceiverIdent(),len);
        switch(len) { 
            case 0: {
                LOGGERTAG("closed getReceiverIdent()=%d\n",getReceiverIdent());
                return false;
                };
            case -1: {
                flerrortag("recv getReceiverIdent()=%d\n",getReceiverIdent());
                return false;
                }
            default:
                int totlen=start+len;
                status.ininterpret=true;
                auto [last,comlen]=interpret(host,nullptr,com,totlen);
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


 int Connect::interpretcommands(passhost_t *host,crypt_t *ctx,senddata_t *com,int totlen) {
    auto [last,comlen]=interpret(host,ctx,com,totlen);
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

 bool Connect::getcom( passhost_t *host,ascon_aead_ctx_t *ctx) {
    int allindex=gethostindex(host);
    auto &status=mirrorstatus[allindex].receive;
    status.running(true);
    destruct _dest([&status]{status.running(false);});



    while(true) {
        LOGAR("getcom");
        constexpr const int taglen=16;
        uint8_t tag[taglen];
        if(int getlen=r_recvni(tag,taglen);getlen!=taglen) {
            LOGGERTAG("recv tag %d\n",getlen);
            return false;
            }
        LOGARTAG("got tag");
        constexpr int testlen=16;
        senddata_t start[testlen];
        const int len1=r_recvni(start,testlen);
        switch(len1) {
            case 0: {
            LOGARTAG("recv==0");
            return false;
            };
            case -1: {
                flerrortag("error recv, restartReceiver %d\n",getReceiverIdent());
                restartReceiver();
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
         savemessage(host,"1: Encrypted with a different password?");
            LOGGERTAG("ascon_aead128a_decrypt_update(...%d)=%d\n",len1,res);
            return false;
            }
        const int datlen=*reinterpret_cast<int *>(uit);
        LOGGERTAG("datlen=%d\n",datlen);
        if(datlen<0||datlen>MAXDATA) {
         savemessage(host,"2: Encrypted with a different password?");
            LOGGERTAG("strange datasize=%d\n",datlen);
            return false;
            }
        const int intdatlen=datlen+sizeof(int);
        const int totlen=(intdatlen<16?16:intdatlen);
        if(totlen>maxcom) {
            senddata_t *newbuf= new (maxalign,std::nothrow) senddata_t [totlen];
            if(!uit) {
            savemessage(host,"Out of Memory?");
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
         savemessage(host,"Out of Memory?");
            sleep(1);
            return false;
            }
        
        std::unique_ptr<uint8_t[] > deltoo(incrypt);
        int len=0;
        LOGARTAG("voor recv");
        for(;len<bijlen;) {
            int bij=r_recvni(incrypt+len,bijlen-len);
            switch(bij) {
                case 0: {
                LOGARTAG("2: recv==0");
                return false;
                };
                case -1: {
                    lerrortag("2: recv");
                    return false;
                    }
                }
            len+=bij;
            }
        LOGARTAG("voor ascon_aead128a_decrypt_update");
        if(totlen>testlen)
            res+= ascon_aead128a_decrypt_update(ctx, uit+res, incrypt, bijlen);
        bool is_tag_valid;
        res += ascon_aead128a_decrypt_final( ctx,uit+res, &is_tag_valid, tag, taglen);
        if(!is_tag_valid) {        
         savemessage(host,"3: Encrypted with a different password");
            LOGARTAG("ascon_aead128a_decrypt_final returns invalid");
            return false;
            }
        else
            LOGARTAG("ascon_aead128a_decrypt_final valid");
        status.ininterpret=true;
        if(int res=interpretcommands(host,ctx,uit+intlen,datlen)) {
            if(res==1)
                return true;
            return false;
            }
        status.ininterpret=false;
        }    
    }

template<Connect::recvni_type recvni,Connect::getIdent_type getIdent>
 unique_al<4> Connect::receivedatanopassalg(const int totlen) {
    LOGGERTAG("receivedatanopass(%d,%d)\n",(this->*getIdent)(),totlen);
    uint8_t *buf=new(std::align_val_t(4),std::nothrow) uint8_t[totlen];
    if(!buf) {
        sleep(1);
        return unique_al<4>(nullptr);
        }
    unique_al<4> destructptr(buf,ardeleter<4,uint8_t>());
    for(int took=0,bij;took<totlen;took+=bij) {
        bij=(this->*recvni)(buf+took,totlen-took);
        switch(bij) {
            case -1:lerrortag("recv");return unique_al<4>(nullptr);
            case 0:LOGARTAG("recv==0"); return unique_al<4>(nullptr);


            };
        }
    return destructptr;
    }
    /*
 unique_al<4> Connect::receivedatanopass_r(const int totlen) {
    return receivedatanopassalg<&Connect::r_recvni,&Connect::getReceiverIdent>( totlen);
    }
 unique_al<4> Connect::receivedatanopass_s(const int totlen) {
    return receivedatanopassalg<&Connect::s_recvni,&Connect::getSenderIdent>( totlen);
    }
*/
dataonlyptr Connect::receivedataonly_s(crypt_t *ctx,const int len) {
     int totlen=aligner<alignof(dataonly)>(sizeof(dataonly)+len);
    auto dat=receivedata_s(ctx, totlen) ;
    dataonlyptr destructptr(reinterpret_cast<dataonly*>(dat.get()),arcastdeleter<4,dataonly>());
    dat.release();
    return destructptr;
    }
template<Connect::recvni_type recvni,Connect::getIdent_type getIdent>
unique_al<4>  Connect::receivedatapass_alg(crypt_t *ctx,int messlen) {
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
    if(int len=(this->*recvni)(buf,totlen);len!=totlen) {
        flerrortag("recv(%d,,%d)!=%d",(this->*getIdent)(),totlen,len);
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
        LOGARTAG("ascon_aead128a_decrypt_final returns invalid tag");
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
    /*(
unique_al<4>  Connect::receivedatapass_r(crypt_t *ctx,int messlen) {
    return Connect::receivedatapass_alg<&Connect::r_recvni,&Connect::getReceiverIdent>(crypt_t *ctx,int messlen) ;
    }
unique_al<4>  Connect::receivedatapass_s(crypt_t *ctx,int messlen) {
    return Connect::receivedatapass_alg<&Connect::s_recvni,&Connect::getSenderIdent>(crypt_t *ctx,int messlen) ;
    } 
unique_al<4> Connect::receivedata_r( crypt_t *ctx,const int len) {
    if(ctx==nullptr) 
        return receivedatanopass_r(len);
    else return receivedatapass_r( ctx, len) ;
    }
unique_al<4> Connect::receivedata_s( crypt_t *ctx,const int len) {
    if(ctx==nullptr) 
        return receivedatanopass_s(len);
    else return receivedatapass_s( ctx, len) ;
    }
inline void show(uint8_t *dat,int len) {
    for(int i=0;i<len;i++)
        fprintf(stderr,"%x ",dat[i]);
    fprintf(stderr,"\n");
    }
    */

bool    Connect::receivepassinit(passhost_t *host,ascon_aead_ctx_t *ctx) {
    LOGGERTAG("getReceiverIdent()=%d receivepassinit %s\n",getReceiverIdent(),host->getnameif());
    constexpr int takelen=8;
    uint8_t nonce[ASCON_AEAD_NONCE_LEN];
    int len=r_recvni(nonce,takelen);
    if(len!=takelen) {
        if(len<0) 
            flerrortag("receivepassinit: recv %d",len);
        else {
            LOGGERTAG("receivepassinit: recv()=%d!=%d\n",len,takelen);
            }
        return false;
        }
    
    constexpr int makelen=ASCON_AEAD_NONCE_LEN-takelen;
    uint8_t *makestart=nonce+takelen;
       makerandom(makestart, makelen);    
       if((len=r_sendni(makestart,makelen))!=makelen) {
         if(len<=0) {
            flerrortag("receivepassinit send=%d",len);
            }
        else
            LOGGERTAG("receivepassinit send=%d != %d",len,makelen);
        return false;
        }
    if(ctx) {
        ascon_aead128a_init(ctx, host->pass.data(),nonce);    
        }
    else {
        LOGAR("ctx==null");
        }
    LOGGERTAG("end getReceiverIdent()=%d receivepassinit\n",getReceiverIdent());
    return true;
    }



 bool    Connect::getcommandspassinit(passhost_t *host) {
    ascon_aead_ctx_t ctx;
    if(!receivepassinit(host,&ctx)) 
        return false;  //NO cleanup, because no init
    const bool ret=getcom(host,&ctx) ;
    ascon_aead_cleanup(&ctx);
    return ret;
    }
bool    Connect::getcommands(passhost_t *host) {
      LOGGERTAG("getcommands(%d)\n",getReceiverIdent());
    if(host->haspass())
        return getcommandspassinit(host);
    else {
        return getcommandsnopass(host);
        }
    }
bool    Connect::activegetcommands(passhost_t *host,crypt_t *ctx) {
    if(ctx) {
        bool res=getcom(host,ctx);
        ascon_aead_cleanup(ctx);
        return res;
        }
    return getcommandsnopass(host);
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
void setcalibratedstart(int,int);
#ifdef WEAROS
#include "curve/jugglucotext.hpp"
 extern void setInitText(const char *message);
static bool startedreceiving() {
       setInitText(usedtext->receivingdata.data());
       return true;
   }
#endif
static bool savefileonce(const struct fileonce_t *gegs) {
    const int nr=gegs->nr;
    const uint8_t *start=reinterpret_cast<const uint8_t*>(&gegs->gegs[nr]);
    const char *name=reinterpret_cast<const char *>(start);
    int fp=filedata.open(name);
    LOGGERTAG("savefileonce %s %d show=%d\n",name,nr, (gegs->dowith&startcalibratedupdate));
    if(fp<0)
        return false;
    destruct des([fp](){filedata.close(fp);});
    start+=(gegs->namelen);
    for(int i=0;i<nr;i++) {
        if(!filedata.savedata(fp,gegs->gegs[i].off,gegs->gegs[i].len,start)) {
            LOGARTAG("savedata failed");
            return false;
            }
        start+=gegs->gegs[i].len;
        }
        if((gegs->dowith&startcalibratedupdate)==startcalibratedupdate) {
            const auto [sendindex,startpos]=getstartinfo(gegs,start);
            setcalibratedstart(sendindex,startpos);
            }
         else {
            if((gegs->dowith&streamupdatebit)==streamupdatebit) {
                    const auto [sendindex,startpos]=getstartinfo(gegs,start);
                    processglucosevalue(sendindex,startpos);

                    }
            else {
                    if((gegs->dowith&starthistoryupdate)==starthistoryupdate) {
                            const auto [sendindex,startpos]=getstartinfo(gegs,start);
                            sethistorystart(sendindex,startpos);
                            }
                 }
            }
    LOGARTAG("savedata success");
#ifdef WEAROS
   static const bool res=startedreceiving();
#endif
    return true;
    }

#include "receive.hpp"
bool Connect::sendfile(crypt_t *pass,const char *filename,uint32_t off,uint32_t len) {
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
    return r_noacksendcommand(pass,destructptr.get(),totlen);
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
