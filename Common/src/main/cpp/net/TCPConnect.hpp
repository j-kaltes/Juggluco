/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2, Libre 3, Dexcom G7/ONE+,              */
/*      Sibionics GS1Sb and Accu-Chek SmartGuide sensors.                            */
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
/*      Fri Nov 21 11:12:24 CET 2025                                                 */
#pragma once
#include "logs.hpp"
#include <sys/socket.h>
#include "netstuff.hpp"
#include "myfdsan.h"
#include "Connect.hpp"
class TCPConnect: public Connect {
using Connect::Connect;
int senderSock=-1;
int receiverSock=-1;

void setTimeouts(int sock)  {
        LOGAR("TCPConnect::setTimeouts");
        if(sock>=0) {
            receivetimeout(sock,60) ;
            sendtimeout(sock,60*5);
            }
        }
public:
template <typename Self>
auto &getReceiverSock( this Self&& self) {
        return self.receiverSock;
        }
template <typename Self>
auto &getSenderSock( this Self&& self) {
        return self.senderSock;
        }
int connectone( const struct sockaddr_in6  *sin, int &sock,char stype,passhost_t *pass,struct pollfd    *cons,int&use
#if defined(WEAROS_MESSAGES)
      ,bool &activate
#endif
            );
virtual int makeconnection2(passhost_t *pass,char stype) override;

virtual ssize_t  r_sendni(const void *buf, size_t len) override{
        return sendni(getReceiverSock(),buf,len);
	}
virtual ssize_t  r_recvni(void *buf, size_t len) override {
        return recvni(getReceiverSock(),buf,len);
        }
virtual ssize_t  s_sendni(const void *buf, size_t len) override{
        return sendni(getSenderSock(),buf,len);
	}
virtual ssize_t  s_recvni(void *buf, size_t len) override {
        return recvni(getSenderSock(),buf,len);
        }
virtual void shutdownReceiver() override {
            shutdown(getReceiverSock(),SHUT_RDWR);
        }
virtual void shutdownSender() override {
            shutdown(getSenderSock(),SHUT_RDWR);
        }
virtual void restartReceiver() override {
            shutdownReceiver();
        }
virtual void restartSender() override {
         shutdownSender();
        }

  void  setSock(int &sock,int newsock) {
        int oldsock=sock;
        sock=-1;
        if(oldsock>=0&&oldsock!=newsock) {
            shutdown(oldsock,SHUT_RDWR);
            sockclose(oldsock);
            }
        sock=newsock;
        }
void setReceiverSock(int newsock) {
        setSock(getReceiverSock(),newsock);
        }
virtual  void  closeReceiverConnection() override {
        ::closesock(getReceiverSock());
      }


void setSenderSock(int newsock) {
        setSock(getSenderSock(),newsock);
        }
virtual  void  closeSenderConnection() override {
        ::closesock(getSenderSock());
      }
virtual  int  getReceiverIdent() const override {
    return getReceiverSock();
    };
virtual  int  getSenderIdent() const override {
    return getSenderSock();
    };


 virtual  bool  isConnectedReceiver() const override {
        return getReceiverSock()>=0;
        };
 virtual  bool  isConnectedSender() const override {
        return getSenderSock()>=0;
        };
virtual void setReceiverTimeouts() override {
    setTimeouts(receiverSock);
    }
virtual void setSenderTimeouts() override {
    setTimeouts(senderSock)  ;
    }

virtual void endConnection() override { 
     closeReceiverConnection(); 
     closeSenderConnection(); 
     }
void passivesender(passhost_t *pass,int &recsock,int oldrecsock) ;
 };

