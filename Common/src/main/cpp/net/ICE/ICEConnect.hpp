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
/*      Fri Nov 21 11:08:14 CET 2025                                                 */
#pragma once
#include <condition_variable>
#include "datbackup.hpp"
#include "logs.hpp"
#include <sys/socket.h>
#include <atomic>
#include "net/netstuff.hpp"
#include "myfdsan.h"
#include "net/Connect.hpp"
#include "ICE_data.hpp"
#define LOGGERICE(...) LOGGER("ICE: " __VA_ARGS__)
#define LOGARICE(...) LOGAR("ICE: " __VA_ARGS__)
extern bool initAgent(juice_agent *agent,int allindex);
extern juice_agent *createAgent(int allindex);

extern juice_log_level_t juice_log_level;

extern int hostselect(std::string_view name);
class ICEConnect: public Connect {
    public:
bool wakeReceiver=false;
std::mutex receiveThreadMutex;
std::condition_variable receiveThreadCon; 
std::atomic_flag startSending{};
std::atomic_flag startDone{};
bool start_ack;
bool other_started;
void resetStart() {
 start_ack=false;
 other_started=false;
 bool old=startSending.test_and_set();
 startDone.test_and_set();
 LOGGER("resetStart flag was %d now %d\n",old,startSending.test());
 };
bool side;
bool endConnect=false;
bool isConnected=false;
ICE_data   icedata[2]{{allindex,side},{allindex,!side}};
std::atomic<juice_agent*> agent;
char sdp[JUICE_MAX_SDP_STRING_LEN];
int sdplen;
int hostindex;

ICEConnect(int allindex,const passhost_t &host):Connect(allindex),side(host.side),hostindex(hostselect(host.getICEname())) {
        agent.store(nullptr);
        }
~ICEConnect() {
        endConnectionHere();
        }
void endConnectionHere() {
       LOGGERICE("%d: endConnectionHere\n",side);
       isConnected=false;
       endConnect=true;
        icedata[1].setshutdown(); 
        icedata[0].setshutdown();
        startSending.clear();
        startSending.notify_all();
        startDone.clear();
        startDone.notify_all();
       }
private:
//uint32_t initrunning=0;
std::atomic_flag initrunning{};
public:
virtual int setindex(int in) override{
        LOGGER("setindex(%d)\n",in);
        icedata[1].allindex=in;
        icedata[0].allindex=in;
        return Connect::setindex(in);
        }
 int newConnection(int allindex) {
        setindex(allindex);
        if(initrunning.test_and_set()) {
            LOGGERICE("newConnection(%d) already running\n",allindex);
            return 0;
            } 

        LOGGER("start newConnection(%d)\n",allindex);
        destruct _{[this]{initrunning.clear();}};
        auto wasagent=agent.exchange(nullptr);
        if(wasagent)  {
            LOGGER("1: juice_destroy(%p)\n",wasagent);
            juice_set_log_level(JUICE_LOG_LEVEL_VERBOSE);
            juice_destroy(wasagent);
            juice_set_log_level(juice_log_level);
            }
        icedata[1].reCreated(); 
        icedata[0].reCreated(); 
        resetStart();
        wakeReceiver=false;
        auto *theagent=createAgent( allindex);
        if(!initAgent(theagent,allindex)) {
//                auto wasagent=agent; agent=nullptr;
                LOGGER("end ICEConnect::newConnection failed allindex=%d, juice_destroy(%p)\n",allindex,wasagent);
                if(theagent) {
                    juice_set_log_level(JUICE_LOG_LEVEL_VERBOSE);
                    juice_destroy(theagent);
                    juice_set_log_level(juice_log_level);
                    }
                return -1;
                }
        else
               endConnect=false;
        agent.store(theagent);
        LOGGERICE("end ICEConnect::newConnection(%d) agent=%p\n",allindex,agent.load());
        return 1;
        }

public:
 void notifyReceive() {
     icedata[0].notifyReceive(); 
     icedata[1].notifyReceive(); 
     }
void sayEndConnection(){
        endConnectionHere();
        icedata[1].shutDown(agent.load());
        icedata[0].end(agent.load());
        LOGGERICE("%d: ICEConnect::sayEndConnection allindex=%d agent=%p\n",side,allindex,agent.load());
        }
void endConnection() override{
        sayEndConnection();
//        auto wasagent=agent; agent=nullptr;
        if(initrunning.test_and_set()) {
                LOGGERICE("%d: ICEConnect::endConnection allindex=%d agent=%p, but initrunning\n",side,allindex,agent.load());
                return;
                }
        destruct _{[this]{initrunning.clear();}};
        LOGGERICE("%d: ICEConnect::endConnection allindex=%d agent=%p\n",side,allindex,agent.load());
        auto wasagent=agent.exchange(nullptr);
        if(wasagent) {
            LOGGER("endConnection: juice_destroy(%p)\n",wasagent);
            juice_set_log_level(JUICE_LOG_LEVEL_DEBUG);
            juice_destroy(wasagent);
            juice_set_log_level(JUICE_LOG_LEVEL_WARN);
            }
        LOGGERICE("%d: end ICEConnect::endConnection allindex=%d set agent=%p\n",side,allindex,wasagent);
        }
int  connect(const passhost_t *pass) {
        icedata[0].reStarted();
        icedata[1].reStarted();
        int index=gethostindex(pass);
        if(endConnect||!agent.load()||index!=allindex||!isConnected)   {
            LOGGERICE("%s %d: ICE::Connect::connect no agent\n",pass->getICEname().data(),pass->side);
            return newConnection(index);
            }
        else {
               std::lock_guard<std::mutex> lck(receiveThreadMutex);
               wakeReceiver=true;
               receiveThreadCon.notify_one();
              }
        return -2;
        }
virtual int makeconnection2(passhost_t *pass,char stype) override {
        LOGGER("makeconnection2 %s\n",pass->getICEname().data());
        sleep(1);
        connect(pass);
        if(!agent.load())
            return -1;
        return shakehands(pass,stype);
        }

virtual ssize_t  r_sendni(const void *buf, size_t len) override{
    if(!isConnected)  {
          LOGGERICE("ICEConnect::r_sendni(%p,%d)= not connected\n",buf,len);
          return -1;
          }
    auto ret= icedata[1].senddata(agent.load(), (const char *)buf,len);
    LOGGERICE("ICEConnect::r_sendni(%p,%d)=%d\n",buf,len,ret);
    return ret;
    }
virtual ssize_t  r_recvni(void *buf, size_t len) override {
        if(!isConnected)  {
            LOGGERICE("ICEConnect::r_recvni(%p,%d) not connected\n",buf,len);
            return -1;
        }
       auto ret=icedata[1].receive(agent.load(),(char *)buf, len);
        LOGGERICE("ICEConnect::r_recvni(%p,%d)=%d\n",buf,len,ret);
        return ret;
        }
virtual ssize_t  s_sendni(const void *buf, size_t len) override{
     if(!isConnected) {
        LOGGERICE("ICEConnect::s_sendni(%p,%d) not connected\n",buf,len);
        return -1;
        }
    auto ret= icedata[0].senddata(agent.load(), (const char *)buf,len);
    LOGGERICE("ICEConnect::s_sendni(%p,%d)=%d\n",buf,len,ret);
    return ret;
	}
virtual ssize_t  s_recvni(void *buf, size_t len) override {
       if(!isConnected) {
           LOGGERICE("ICEConnect::s_recvni(%p,%d) not connected\n",buf,len);
           return -1;
           }
        auto ret= icedata[0].receive(agent.load(),(char *)buf, len);
        LOGGERICE("ICEConnect::s_recvni(%p,%d)=%d\n",buf,len,ret);
        return ret;
        }


virtual void shutdownReceiver() override {
        if(!backup)
            return;
        const passhost_t &host= getBackupHosts()[allindex];
        LOGGERICE("%s %d shutdownReceiver()\n",host.getICEname().data(),host.side);
        icedata[1].shutDown(agent.load());
        }
virtual void restartReceiver() override {
    if(!backup)
        return;
    const passhost_t &host= getBackupHosts()[allindex];
    LOGGERICE("%s %d restartReceiver()\n",host.getICEname().data(),host.side);
    icedata[1].shutDown(agent.load());
    }
virtual void restartSender() override {
    if(!backup)
        return;
    const passhost_t &host= getBackupHosts()[allindex];
    LOGGERICE("%s %d restartSender()\n",host.getICEname().data(),host.side);
    icedata[0].shutDown(agent.load());
    }
virtual void shutdownSender() override {
        if(!backup)
            return;
        const passhost_t &host= getBackupHosts()[allindex];
        LOGGERICE("%s %d shutdownSender()\n",host.getICEname().data(),host.side);
        icedata[0].shutDown(agent.load());
        }

virtual  void  closeReceiverConnection() override {
        if(!backup)
            return;
        const passhost_t &host= getBackupHosts()[allindex];
        LOGGERICE("%s %d closeReceiverConnection()\n",host.getICEname().data(),host.side);
        icedata[1].shutDown(agent.load());
      }


virtual  void  closeSenderConnection() override {
        if(!backup)
            return;
        const passhost_t &host= getBackupHosts()[allindex];
        LOGGERICE("%s %d closeSenderConnection()\n",host.getICEname().data(),host.side);
        icedata[0].shutDown(agent.load());
      }


int getIdent()  const {
    if(!agent.load())
        return -1;
    return (int)(uint64_t)agent.load();
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
        if(!agent.load())
                return false;
        juice_state_t state = juice_get_state(agent.load());
        return state == JUICE_STATE_COMPLETED||state == JUICE_STATE_CONNECTED;
        };
 virtual  bool  isConnectedSender() const override {
        if(icedata[0].shutdown) {
                return false;
                }
        if(!agent.load())
                return false;
        juice_state_t state = juice_get_state(agent.load());
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
    void receiverThread(int allindex);
 };

