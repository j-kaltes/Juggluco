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


#pragma once
#include <string_view>
#include <vector>
#include <memory>
 #include <sys/types.h> 
 #include "comtypes.h"
 #include "passhost.h"
 #include "crypt.h"
extern bool senddata(crypt_t *pass,const int sock,const int offset,const senddata_t *data,const int datalen,const std::string_view naar,uint16_t dowith=0,const uint8_t *extra=nullptr,int extralen=0) ;

template <typename T> bool senddata(crypt_t *pass,const int sock,const int offset,const T *startin,const T* endin,const std::string_view naar,uint16_t dowith=0,const uint8_t *extra=nullptr, int extralen=0) {
	const senddata_t *start=reinterpret_cast<const senddata_t*>(startin);	
	const senddata_t *end=reinterpret_cast<const senddata_t*>(endin);	
	int len=end-start;
	return senddata(pass,sock,offset*sizeof(T),start,len,naar,dowith);
	}


void receivetimeout(int sock,int secs) ;
void sendtimeout(int sock,int secs) ;
//extern int makeconnection(passhost_t *pass,int &sock,crypt_t*ctx) ;

extern int makeconnection(passhost_t *pass,int &sock,crypt_t*ctx,char stype) ;
//extern int makeconnection(passhost_t *pass,int &sock,std::vector<int> &tmpsocks,crypt_t*ctx) ;


struct subdata {
	 const senddata_t *data;
	 int offset;
	 int datalen;
	};

extern bool sendcommand(crypt_t *pass,int sock ,const unsigned char *buf,int buflen) ;
extern bool senddata(crypt_t *pass,const int sock,const std::vector<subdata>&data,const std::string_view naar,uint16_t dowith=0,const uint8_t *extra=nullptr, int extralen=0) ;

bool noacksendcommand(crypt_t *pass,int sock ,const unsigned char *buf,int buflen) ;
extern bool sendrender(crypt_t *pass,const int sock) ;
bool sendrender(crypt_t *pass,const int sock,const uint16_t type);

bool sendone(crypt_t *pass,const int sock, const uint32_t com) ;
bool noacksendone(crypt_t *pass,const int sock, const uint32_t com) ;
extern bool sendbackup(crypt_t *pass,const int sock) ;

extern bool sendbackupstop(crypt_t *pass,const int sock) ;
	/*
inline bool sendnewnums(crypt_t *pass,const int sock) {
	return sendone(pass,sock,snewnums);
	} */

template <int nr,typename  T>
struct arcastdeleter { // deleter
    void operator() ( T *ptr) {
	operator delete[] (reinterpret_cast<uint8_t *>(ptr), std::align_val_t(nr));
	}
    };
typedef std::unique_ptr<dataonly,arcastdeleter<4,dataonly>>  dataonlyptr;
//dataonlyptr receivedatanopass(int sock,const int len); 
#include "aligner.h"
template <int nr>
using unique_al= std::unique_ptr<uint8_t[],ardeleter<nr,uint8_t>> ;
unique_al<4> receivedata(int sock, crypt_t *ctx,const int len) ;

dataonlyptr receivedataonly(int sock, crypt_t *ctx,const int len) ;
bool receiveractive();
inline bool askresetdevices(crypt_t *pass,const int sock) {
	const bool ret= noacksendone(pass, sock, sresetdevices) ;
	LOGGER("askresetdevices(pass,%d)=%d\n",sock,ret);
	return ret;
	}
constexpr const int streamupdatebit=1<<15;
constexpr const int starthistoryupdate=1<<14;
