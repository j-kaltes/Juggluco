#pragma once

#include <stdint.h>
#include <vector>
#include <string_view>
#include "passhost.hpp"
#include "crypt.h"
#include "backup.hpp"
class Connect {
protected:
    int allindex;
public:
    bool receiving=false;
    Connect(int index):allindex(index) {}

virtual void setindex(int index) {
        allindex=index; 
        }
    bool testreceivemagic(passhost_t *pass);
    bool sendcrypt(crypt_t *ctx,uint8_t *data,int datalen);
    bool openfile(crypt_t *ctx,const char *name);
    std::pair<int,int> interpret(passhost_t *host,crypt_t *ctx,senddata_t *datain,int len);
    int interpretcommands(passhost_t *host,crypt_t *ctx,senddata_t *com,int totlen);
    bool getcom( passhost_t *host,ascon_aead_ctx_t *ctx);
    unique_al<4> receivedatanopass(const int totlen);
    dataonlyptr receivedataonly(crypt_t *ctx,const int len);
    unique_al<4>  receivedatapass(crypt_t *ctx,int messlen);
    unique_al<4> receivedata( crypt_t *ctx,const int len);
    bool    receivepassinit(passhost_t *host,ascon_aead_ctx_t *ctx);
    bool    getcommandspassinit(passhost_t *host);
    bool    getcommands(passhost_t *host);
    bool    activegetcommands(passhost_t *host,crypt_t *ctx);
    bool receivecrypt(crypt_t *ctx,uint8_t *uit);
    int16_t sendopen(crypt_t *pass,std::string_view name);
    bool getack();
    bool sendcommand(const unsigned char *buf,int buflen);
    bool sendfile(crypt_t *pass,const char *filename,uint32_t off,uint32_t len);
    bool sendcommandpass(ascon_aead_ctx_t *ctx,const unsigned char *buf,int buflen,bool askack);
    bool sendcommand(crypt_t *pass,const unsigned char *buf,int buflen);
//    bool noacksendcommand(crypt_t *pass,const unsigned char *buf,int buflen);
    bool sendone(crypt_t *pass, const uint32_t com);
    bool noacksendone(crypt_t *pass, const uint32_t com);
    bool sendbackupstop(crypt_t *pass);
    bool sendResetDevices(crypt_t *pass);
    bool sendbackup(crypt_t *pass);
    bool sendwakeupstream(crypt_t *pass);
    bool sendrender(crypt_t *pass);
    bool    senduint16(crypt_t*pass,uint16_t com,uint16_t arg);
    bool sendStartSendCalibrate(crypt_t *pass,const uint16_t sensorindex);
    bool sendBlueWatch(crypt_t *pass,int8_t stream,int8_t nums);
    bool sendshowglucose(crypt_t *pass,const uint16_t sensorindex);
    bool sendrender(crypt_t *pass,const uint16_t type);
   // bool newsenddata(crypt_t *pass,const std::vector<subdata>&data,const std::string_view naar,uint16_t dowith,const uint8_t *extra,int extralen);
   // bool newsenddata(crypt_t *pass,const int offset,const senddata_t *data,const int datalen,const string_view naar,uint16_t dowith,const uint8_t *extra,int extralen);

 bool senddata(crypt_t *pass,const std::vector<subdata>&data,const std::string_view naar,uint16_t dowith=0,const uint8_t *extra=nullptr, int extralen=0) ;
    bool senddata(crypt_t *pass,const int offset,const senddata_t *data,const int datalen,const std::string_view naar,uint16_t dowith=0,const uint8_t *extra=nullptr,int extralen=0) ;
    void   sendpassinit(passhost_t *host,crypt_t *ctx);
    int testsendmagic(passhost_t *pass);
    int shakehands(passhost_t *pass,char stype);
    int makeconnection(passhost_t *pass,crypt_t*ctx,char stype);
   bool    getcommandsnopass(passhost_t *host) ;
bool sendtype(char type);
bool receiveConnect(passhost_t *hostptr);

 virtual int makeconnection2(passhost_t *pass,char stype)=0;
 virtual ssize_t  r_sendni(const void *buf, size_t len)=0; 
 virtual ssize_t  r_recvni(void *buf, size_t len)=0;
 virtual ssize_t  s_sendni(const void *buf, size_t len)=0; 
 virtual ssize_t  s_recvni(void *buf, size_t len)=0;
 virtual void shutdownReceiver()=0;
 virtual void shutdownSender()=0;
 virtual void restartReceiver()=0;
 virtual void restartSender()=0;

 virtual    void  closeReceiverConnection() =0;
 virtual    void  closeSenderConnection() =0;
 virtual  int  getSenderIdent() const =0;
 virtual  int  getReceiverIdent() const =0;
 virtual  bool  isConnectedReceiver() const =0;
 virtual  bool  isConnectedSender() const =0;
virtual void setSenderTimeouts()  =0;
virtual void setReceiverTimeouts()  =0;
virtual void endConnection()  =0;

template <typename T> bool senddata(crypt_t *pass,const int offset,const T *startin,const T* endin,const std::string_view naar,uint16_t dowith=0,const uint8_t *extra=nullptr, int extralen=0) {
	const senddata_t *start=reinterpret_cast<const senddata_t*>(startin);	
	const senddata_t *end=reinterpret_cast<const senddata_t*>(endin);	
	int len=end-start;
	return senddata(pass,offset*sizeof(T),start,len,naar,dowith);
	}


typedef	bool (Connect::*noacksendcommand_type)(const unsigned char *,int );
template <noacksendcommand_type noacksendcommand>
bool gensendcommandpass(ascon_aead_ctx_t *ctx,const unsigned char *buf,int buflen,bool (Connect::*getack)()) ;
bool s_sendcommandpass(ascon_aead_ctx_t *ctx,const unsigned char *buf,int buflen,bool askack);
bool s_noacksendcommand(crypt_t *pass,const unsigned char *buf,int buflen) ;
bool s_noacksendcommandonly(const unsigned char *buf,int buflen);
bool r_noacksendcommand(crypt_t *pass,const unsigned char *buf,int buflen);
typedef ssize_t  (Connect::*sendni_type)(const void *, size_t );
typedef int (Connect::*getIdent_type)() const;
template<sendni_type sendni,getIdent_type getIdent>
bool noacksendcommand(const unsigned char *buf,int buflen) ;
virtual ~Connect() = default;
};
