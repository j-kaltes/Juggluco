#pragma once

#include <stdint.h>
#include <vector>
#include <string_view>
#include "passhost.hpp"
#include "crypt.h"
#include "backup.hpp"

class Connect {
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
    bool    activegetcommands(asshost_t *host,crypt_t *ctx);
    bool receivecrypt(crypt_t *ctx,uint8_t *uit);
    int16_t sendopen(crypt_t *pass,std::string_view name);
    bool noacksendcommand(const unsigned char *buf,int buflen);
    bool getack();
    bool sendcommand(const unsigned char *buf,int buflen);
    bool sendfile(crypt_t *pass,const char *filename,uint32_t off,uint32_t len);
    bool sendcommandpass(ascon_aead_ctx_t *ctx,const unsigned char *buf,int buflen,bool askack);
    bool sendcommand(crypt_t *pass,const unsigned char *buf,int buflen);
    bool noacksendcommand(crypt_t *pass,const unsigned char *buf,int buflen);
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
    bool newsenddata(crypt_t *pass,const std::vector<subdata>&data,const std::string_view naar,uint16_t dowith,const uint8_t *extra,int extralen);
    bool newsenddata(crypt_t *pass,const int offset,const senddata_t *data,const int datalen,const string_view naar,uint16_t dowith,const uint8_t *extra,int extralen);
    bool senddata(crypt_t *pass,const std::vector<subdata>&data,const std::string_view naar,uint16_t dowith,const uint8_t *extra,int extralen);
    bool senddata(crypt_t *pass,const int offset,const senddata_t *data,const int datalen,const string_view naar,uint16_t dowith,const uint8_t *extra,int extralen);
    void   sendpassinit(passhost_t *host,crypt_t *ctx);
    int testsendmagic(passhost_t *pass);
    int shakehands(passhost_t *pass,char stype);
    int makeconnection(passhost_t *pass,crypt_t*ctx,char stype);
   bool    getcommandsnopass(passhost_t *host) ;
void passivesender(passhost_t *pass)  ;

 virtual int makeconnection2(passhost_t *pass,char stype)=0;
 virtual ssize_t  sendni(const void *buf, size_t len)=0; 
 virtual ssize_t  recvni(void *buf, size_t len)=0;
 virtual void shutdown()=0;
 virtual    void  closeConnection() =0;
 virtual  int  getIdent() const =0;
 virtual  bool  isConnected() const =0;
virtual void setTimeouts() override =0;
};
