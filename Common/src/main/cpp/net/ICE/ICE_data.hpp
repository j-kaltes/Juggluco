#pragma once
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <bitset>
#include <mutex>
#include <semaphore>
#include <condition_variable>
#include "libjuice/include/juice/juice.h"
#include "logs.hpp"
#include "PlaceBuf.hpp"
#include  "udp_header.h"
#define LOGGERICE(...) LOGGER("ICE: " __VA_ARGS__)
#define LOGARICE(...) LOGAR("ICE: " __VA_ARGS__)
struct ICE_data {
    static constexpr const int packetsize=1200;
    static constexpr const int dataunit=packetsize-sizeof(udp_header);
    static constexpr int sendwindow=4096;
    static constexpr int maxrelay=sendwindow*dataunit;
    uint16_t send_trans_id=0, receive_trans_id=0;
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
    int ack_msec=10000;
    PlaceBuf<char> databuf{1024*1024};
    int offset=0;
    int datalen=0;
   // std::binary_semaphore sema{false};
    std::mutex mutex;
    std::condition_variable condi; 
    std::binary_semaphore doSend{false};
    inline void certain_try_acquire(std::binary_semaphore &sema) { //Sometimes doesn't block after one try_acquire
           while(sema.try_acquire()) {
                LOGGERICE("%d doSend.try_acquire()\n",side);
                }
            }
    void handledata(int len) { 
        offset=0;
        LOGGERICE("databuf.size()=%d len=%d\n",databuf.size(),len);
        assert(databuf.size()==len);
     //   sema.release();
        datalen=len;
        std::lock_guard<std::mutex> lck(mutex);
        condi.notify_one();
        }
     void notifyReceive() {
        std::lock_guard<std::mutex> lck(mutex);
        condi.notify_one();
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
    void shutDown(juice_agent_t *agent);
    void end(juice_agent_t *agent);
    void setshutdown() ;
    };
