#pragma once
#include "logs.hpp"
#include "Connect.hpp"
#include <sys/socket.h>

class TCPConnect: public Connect {
int sock;
public:
int connectone( const struct sockaddr_in6  *sin, int &sock,char stype,passhost_t *pass,struct pollfd    *cons,int&use
#if defined(WEAROS_MESSAGES)
      ,bool &activate
#endif
            );
virtual int makeconnection2(passhost_t *pass,char stype) override;
ssize_t  sendni(int sockfd,const void *buf, size_t len) {
 	int waslen;
	while((waslen=send(sockfd,buf,len,0))==-1) {
		if(errno!=EINTR)
			return -1;
		LOGGER("sendni retry %zd\n",len);
		}
	return waslen;
	}

ssize_t  recvni(int sockfd,void *buf, size_t len)  {
    int waslen;
    int inter=0;
    while((waslen=recv(sockfd,buf,len,0))==-1) {
            int erwas=errno;
            flerror("recv(%d,buf,%zd)",sockfd,len);
            if(erwas!=EINTR||inter>20)
                    return -1;
            ++inter;
            }
    return waslen;
    }

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
int &getReceiverSock() {
        return receiverSock;
        }

void setSenderSock(int newsock) {
        setSock(getSenderSock(),newsock);
        }
virtual  void  closeSenderConnection() override {
        ::closesock(getSenderSock());
      }
int &getSenderSock() {
        return senderSock;
        }
virtual  int  getIdent() const override {
    return getSock();
    };


 virtual  bool  isConnected() const override {
        return getSock()>=0;
        };
 void setTimeouts(int sock)  {
        receivetimeout(sock,60) ;
        sendtimeout(sock,60*5);
        }
virtual void setTimeouts() override {
 }

