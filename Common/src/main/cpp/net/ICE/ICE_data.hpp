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
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <bitset>
#include <mutex>
//#include <semaphore>
#include <atomic>
#include <condition_variable>
#include "libjuice/include/juice/juice.h"
#include "logs.hpp"
#include "PlaceBuf.hpp"
#include  "udp_header.h"
#define LOGGERICE(...) LOGGER("ICE: " __VA_ARGS__)
#define LOGARICE(...) LOGAR("ICE: " __VA_ARGS__)

#define JACOBSON
class ICEConnect;
struct ICE_data {
    static constexpr const int packetsize=1200;
    static constexpr const int dataunit=packetsize-sizeof(udp_header);
    static constexpr int sendwindow=4096;
    static constexpr int maxrelay=sendwindow*dataunit;
    uint16_t send_trans_id=0, receive_trans_id=0;
//    const char *sendbuf=nullptr;
 //   int sendsize=0;
    int lastpacket=0; 
    int nextAck=0;
    bool shutdown=false;
    bool sendShutdown=false;
    bool  sendStop=false;
    int allindex;
    bool side;
    ICE_data(int index,int t):allindex(index),side(t) {
        };
    std::bitset<sendwindow>  acknowledged;
    uint32_t starttime=time(nullptr);
    bool askedData=false;
#ifdef JACOBSON
    double SRTT=0.0;
    double RTTVAR;
#elif P99
    static  constexpr const int maxrtt=150;
    int rtts[maxrtt];
    int rttiter=0;
#else
    struct prevRTT {
        int RTT;
        int past;
        };
    prevRTT high{},nexthigh{};
#endif
    int RTO=10000;
    void setRTO(int RTT);
    PlaceBuf<char> databuf{1024*1024};
    int offset=0;
    int datalen=0;
    std::mutex  receiveMutex;
    std::condition_variable receiveCond; 
    bool sentAck=false;
    bool lastAcked=false;
    std::mutex  sendMutex;
    std::condition_variable sendCond; 
    void ackeddata(juice_agent_t *agent,udp_header  *head);

    std::mutex  doSendMutex;
    std::condition_variable doSendCond; 
    bool doSend=false;
//    std::atomic_flag doSend{};
 inline void certain_try_acquire(bool &flag) {
     bool old=flag;
     flag=false;
     LOGGER("certain_try_acquire flag was %d now %d\n",old,flag);
     }
    /*
    inline void certain_try_acquire(std::binary_semaphore &sema) { //Sometimes doesn't block after one try_acquire
           while(sema.try_acquire()) {
                LOGGERICE("%d doSend.try_acquire()\n",side);
                }
            } */
    void handledata(int len) { 
        offset=0;
        LOGGERICE("handledata databuf.size()=%d len=%d\n",databuf.size(),len);
        if(!databuf.size()) {
                LOGAR("handledata return databuf.size()==0");
                return;
                }
        assert(databuf.size()==len);
        datalen=len;
        std::lock_guard<std::mutex> lck( receiveMutex);
        receiveCond.notify_one();
        }
     void notifyReceive() {
        std::lock_guard<std::mutex> lck( receiveMutex);
        receiveCond.notify_one();
        }
    int receive(juice_agent_t *agent,char *buf, const int maxbuf);
    void resetReceive() {
        offset=0;
        datalen=-1;
        askedData=false;
        databuf.clear();
        }
    void reStarted() {
        resetReceive();
        shutdown=false;
        sendShutdown=false;
        sendStop=false;
        sentAck=false;
        lastAcked=false;
        certain_try_acquire(doSend);
        LOGGERICE("reStarted side=%d doSend(%p).try_acquire()\n",side,&doSend);
        };
    void reCreated() {
        send_trans_id=0;
        receive_trans_id=0;
        };
    uint32_t    sendpacket(juice_agent_t *agent,uint16_t trans_id,const char *totaldata,int totallen,uint16_t index,uint32_t starttime);
    int senddata(juice_agent_t *agent, const char *data,int len);
    void on_recv(juice_agent_t *agent, const char *data, size_t size,int allindex) ;
    void askdata(juice_agent_t *agent);
    void sendshutDown(juice_agent_t *agent);
    void shutDown(juice_agent_t *agent);
    void end(juice_agent_t *agent);
    void setshutdown() ;
   bool sendWithError(juice_agent_t *agent,const char *data, int len);
   void sendStart(juice_agent_t *agent,ICEConnect *con);
   bool sendRetry(juice_agent_t *agent,const char *data, int len);
    };
