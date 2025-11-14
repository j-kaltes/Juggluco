#pragma once
#include "datbackup.hpp"
#include "logs.hpp"
#include <sys/socket.h>
#include "net/netstuff.hpp"
#include "myfdsan.h"
#include "net/Connect.hpp"
#include "ICE_data.hpp"
#define LOGGERICE(...) LOGGER("ICE: " __VA_ARGS__)
#define LOGARICE(...) LOGAR("ICE: " __VA_ARGS__)
extern bool initAgent(juice_agent *agent,int allindex);
extern juice_agent *createAgent(int allindex);
class ICEConnect: public Connect {
    public:
bool side;
bool endConnect=false;
bool isConnected=false;
ICE_data   icedata[2]{{allindex,side},{allindex,!side}};
juice_agent *agent=nullptr;
char sdp[JUICE_MAX_SDP_STRING_LEN];
int sdplen;
ICEConnect(int allindex,bool side):Connect(allindex),side(side) {}
~ICEConnect() {
        endConnectionHere();
        }
void endConnectionHere() {
       LOGGERICE("%d: endConnectionHere",side);
        icedata[1].setshutdown(); 
        icedata[0].setshutdown(); 
        endConnect=true;
       }
virtual void setindex(int allindex) override{
        Connect::setindex(allindex);
        if(agent)
            juice_destroy(agent);
        icedata[1].reCreated(); 
        icedata[0].reCreated(); 
        agent=createAgent( allindex);

        if(!initAgent(agent,allindex)) {
                agent=nullptr;
                }
        else
            endConnect=false;
        LOGGERICE("ICEConnect::setindex(%d) agent=%p\n",allindex,agent);
        }

public:
 void notifyReceive() {
     icedata[0].notifyReceive(); 
     icedata[1].notifyReceive(); 
     }
void sayEndConnection(){
        endConnect=true;
        icedata[1].shutDown(agent);
        icedata[0].end(agent);
        LOGGERICE("%d: ICEConnect::sayEndConnection allindex=%d agent=%p\n",side,allindex,agent);
        }
void endConnection() override{
        sayEndConnection();
        LOGGERICE("%d: ICEConnect::endConnection allindex=%d agent=%p\n",side,allindex,agent);
        if(agent) {
            juice_destroy(agent);
            agent=nullptr;
            }
        LOGGERICE("%d: end ICEConnect::endConnection allindex=%d agent=%p\n",side,allindex,agent);
        }
bool  connect(passhost_t *pass) {
        icedata[0].reStarted();
        icedata[1].reStarted();
        int index=gethostindex(pass);
        if(endConnect||!agent||index!=allindex)   {
            LOGGERICE("%s %d: makeconnection2 no agent\n",pass->getICEname().data(),pass->side);
            setindex(index);
            return true;
            }
        return false;
        }
virtual int makeconnection2(passhost_t *pass,char stype) override {
        connect(pass);
        if(!agent)
            return -1;
        return shakehands(pass,stype);
        }

virtual ssize_t  r_sendni(const void *buf, size_t len) override{
    auto ret= icedata[1].senddata(agent, (const char *)buf,len);
    LOGGERICE("ICEConnect::r_sendni(%p,%d)=%d\n",buf,len,ret);
    return ret;
    }
virtual ssize_t  r_recvni(void *buf, size_t len) override {
        LOGGERICE("ICEConnect::r_recvni(%p,%d)\n",buf,len);
       auto ret=icedata[1].receive(agent,(char *)buf, len);
        return ret;
        }
virtual ssize_t  s_sendni(const void *buf, size_t len) override{
    LOGGERICE("ICEConnect::s_sendni(%p,%d)\n",buf,len);
    auto ret= icedata[0].senddata(agent, (const char *)buf,len);
    return ret;
	}
virtual ssize_t  s_recvni(void *buf, size_t len) override {
        auto ret= icedata[0].receive(agent,(char *)buf, len);
        LOGGERICE("ICEConnect::s_recvni(%p,%d)=%d\n",buf,len,ret);
        return ret;
        }


virtual void shutdownReceiver() override {
        if(!backup)
            return;
        const passhost_t &host= getBackupHosts()[allindex];
        LOGGERICE("%s %d shutdownReceiver()\n",host.getICEname().data(),host.side);
        if(agent)
            icedata[1].shutDown(agent);
        }
virtual void restartReceiver() override {
    if(!backup)
        return;
    const passhost_t &host= getBackupHosts()[allindex];
    LOGGERICE("%s %d restartReceiver()\n",host.getICEname().data(),host.side);
    if(agent)
        icedata[1].shutDown(agent);
    }
virtual void restartSender() override {
    if(!backup)
        return;
    const passhost_t &host= getBackupHosts()[allindex];
    LOGGERICE("%s %d restartSender()\n",host.getICEname().data(),host.side);
    if(agent)
        icedata[0].shutDown(agent);
    }
virtual void shutdownSender() override {
        if(!backup)
            return;
        const passhost_t &host= getBackupHosts()[allindex];
        LOGGERICE("%s %d shutdownSender()\n",host.getICEname().data(),host.side);
        if(agent)
            icedata[0].shutDown(agent);
        }

virtual  void  closeReceiverConnection() override {
        if(!backup)
            return;
        const passhost_t &host= getBackupHosts()[allindex];
        LOGGERICE("%s %d closeReceiverConnection()\n",host.getICEname().data(),host.side);
        if(agent)
            icedata[0].shutDown(agent);
      }


virtual  void  closeSenderConnection() override {
        if(!backup)
            return;
        const passhost_t &host= getBackupHosts()[allindex];
        LOGGERICE("%s %d closeSenderConnection()\n",host.getICEname().data(),host.side);
        if(agent)
            icedata[0].shutDown(agent);
      }


int getIdent()  const {
    if(!agent)
        return -1;
    return (int)(uint64_t)agent;
    }
virtual  int  getReceiverIdent() const override {
    return getIdent(); 
    };
virtual  int  getSenderIdent() const override {
    return getIdent(); 
    };


 virtual  bool  isConnectedReceiver() const override {
        if(icedata[1].shutdown) {
                return false;
                }
        if(!agent)
                return false;
        juice_state_t state = juice_get_state(agent);
        return state == JUICE_STATE_COMPLETED||state == JUICE_STATE_CONNECTED;
        };
 virtual  bool  isConnectedSender() const override {
        if(icedata[1].shutdown) {
                return false;
                }
        if(!agent)
                return false;
        juice_state_t state = juice_get_state(agent);
        return state == JUICE_STATE_COMPLETED||state == JUICE_STATE_CONNECTED;
        };
virtual void setReceiverTimeouts() override {
    }
virtual void setSenderTimeouts() override {
    }

void notConnected() {
    isConnected=false;
    LOGGERICE("%d notConnected, shutdown=true\n",side);
    }
void setConnected() {
    isConnected=true;
    LOGGERICE("%d setConnected, shutdown=false\n",side);
    icedata[0].shutdown=false;
    icedata[1].shutdown=false;
    }
 };

