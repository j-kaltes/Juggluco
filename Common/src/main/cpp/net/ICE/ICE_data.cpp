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
#include <unistd.h>
#include "ICEConnect.hpp"
#include "ICE_data.hpp"
#define LOGGERICE(...) LOGGER("ICE: " __VA_ARGS__)
#define LOGARICE(...) LOGAR("ICE: " __VA_ARGS__)
static uint32_t getRelMsec(uint32_t starttime) {
            struct timeval tv;
            gettimeofday(&tv, nullptr);
            return ((tv.tv_sec-starttime)*10000+tv.tv_usec/100);
            }

bool idSmallerOrEqual(uint16_t prev, uint16_t next) {
    if(prev<=next) {
        return (next-prev)<500;
        }
    else {
        return (prev-next)> (UINT16_MAX-500);
        }
    }
bool isNext(uint16_t prev, uint16_t next) {
    return (next==(prev+1))||(prev==UINT16_MAX&&next==0);
    }
bool setNext(uint16_t &prev, uint16_t next) {
    if(isNext(prev,next)) {
        LOGGERICE("setNext prev=%d set to next=%d\n",prev,next);
        prev=next;
        return true;
        }
    LOGGERICE("setNext prev=%d not followed by next=%d\n",prev,next);
     return false;
     }
#ifdef JACOBSON
static constexpr const int timesRTO=1;
void ICE_data::setRTO(int RTT) {
    static constexpr const  double α = 0.125;
    static constexpr const  double  β = 0.25;
    static constexpr const  double  K = 4;
    int preRTO;
    if(SRTT==0.0) {
        SRTT = RTT;
        RTTVAR = RTT *.5;
        preRTO = std::round(RTT + K * RTTVAR);
        }
   else {
        RTTVAR = (1 - β) * RTTVAR + β * fabs(SRTT - RTT);
        SRTT   = (1 - α) * SRTT   + α * RTT;
        preRTO    = std::round(SRTT +  K * RTTVAR);
        }
//    RTO=preRTO*30+150;
//    RTO=preRTO+3054;
//    RTO=preRTO+1950;
     RTO=preRTO+1000;
//     if(RTO>100000) RTO=100000;
    }
#else
static constexpr const int timesRTO=1;
#ifdef P99
void ICE_data::setRTO(int RTT) {
    struct FUNC {
        static int getPercentile(int *data, int len,double perc) {
            int sorted[len];
            std::copy_n(data,len,sorted);
            std::sort(sorted,sorted+len);
            return sorted[(int)(perc*len)];
            }
          };
    rtts[rttiter++%maxrtt]=RTT;
    int rttlen=std::min(rttiter,maxrtt);
    RTO=FUNC::getPercentile(rtts,rttlen,.99)+100;
    }
#else
void ICE_data::setRTO(int rtt) {
    if(high.past>150) {
        high=nexthigh;
        nexthigh.RTT=rtt;
        nexthigh.past=0;
        ++high.past;
        }
     else  {
           if(rtt>high.RTT) {
                high.RTT=rtt;
                high.past=0;
                ++nexthigh.past;
                }
            else {
                ++high.past;
                if(rtt>nexthigh.RTT) {
                    nexthigh.RTT=rtt;
                    nexthigh.past=0;
                    }
                 else {
                    ++nexthigh.past;
                    }

                }
            }
    RTO=high.RTT+150;
    }
#endif
#endif
void ICE_data::ackeddata(juice_agent_t *agent,udp_header  *head) {
      int index=head->index;
      if(head->trans_id==send_trans_id) {
          if(index<0||index>=acknowledged.size()) {
            LOGGERICE("ackeddata (%d ERROR received ack for index %d\n",side,index);
             return;
             }
          uint32_t rel_msec=getRelMsec(starttime) ;
          int RTT=rel_msec-head->rel_msec;
          acknowledged[index]=true;
          auto oldRTO=RTO;
          setRTO(RTT);
          if(head->fin) {
            ++nextAck;
            ++send_trans_id;
            doSend=false;
            LOGGERICE("ackeddata: last ack ++send_trans_id=%d\n",send_trans_id);
            {std::lock_guard<std::mutex> lck(sendMutex);
            lastAcked=true;
            sendCond.notify_one();                        
            }
            }
          else {
              while(acknowledged[nextAck])
                ++nextAck;
              if(nextAck==lastpacket) {
                     std::lock_guard<std::mutex> lck(sendMutex);
                     sentAck=true;
                     sendCond.notify_one();                        
               }
               /*
              else {
                  int previndex=index-1;
                  if(previndex>=0&&!acknowledged[previndex]) {
                        if(sendbuf&&sendsize) {
                            LOGGERICE("resend previous index=%d\n",previndex);
                            sendpacket(agent,head->trans_id,sendbuf,sendsize,previndex, starttime);
                            }
                      }
                  } */
               }
          LOGGERICE("ackeddata: %d trans_id=%d received acknowledgement for %d old RTO=%d  RTT=%d new RTO=%d\n",side,head->trans_id,index,oldRTO,RTT,RTO);
          }
     else {
          LOGGERICE("ackeddata: %d received acknowledgement for %d for different trans_id %d != %d\n",side,index,head->trans_id,send_trans_id);
          }
    }
void ICE_data::on_recv(juice_agent_t *agent, const char *data, size_t size,int allindex) {
   udp_header  *head=const_cast<udp_header *>(reinterpret_cast<const udp_header *>(data));
   switch(head->com) {
        case START: {
            ICEConnect *con=static_cast<ICEConnect *>(connections[allindex]);
            if(!con) {
                LOGGERICE("allindex=%d on_recv START, but con=null\n",allindex);
                return;
                }
          if(head->ack) {
                con->start_ack=true;
                LOGGERICE("on_recv START acknowledged allindex=%d\n",allindex);
                }
           else {
                 head->ack=true;
                 if(!sendRetry(agent, data,sizeof(udp_header))) {
                       LOGGERICE("on_recv START allindex=%d, but error\n",allindex);
                       }
                 else
                    LOGGERICE("on_recv START allindex=%d\n",allindex);
                if(!con->other_started) {
                    con->other_started=true;
                    if(!con->start_ack) {
                            std::thread th{&ICE_data::sendStart,this,agent,con};
                            th.detach();
                            }
                     }
                }
           if(con->other_started&&con->start_ack) {
                   LOGGERICE("startSending.clear()  allindex=%d\n",allindex);
                    con->startSending.clear();
                    con->startSending.notify_all();
                 }
                };break;
        case ASK: {
            if(shutdown) {
                LOGGERICE("on_rev: side=%d ASK, but shutdown\n",side);
                if(!sendShutdown) 
                       sendshutDown(agent);
                return;
                }
            if(!head->ack) {
                if(head->trans_id==send_trans_id||setNext(send_trans_id,head->trans_id)) {
                    LOGGERICE("on_recv allindex=%d side=%d ASK trans_id=%d same as send_trans_id\n",allindex,side,head->trans_id);
                    head->ack=true;
                    if(!sendWithError(agent, data,sizeof(udp_header))) {
                            return;
                            }
                   // doSend.release();
                    //doSend.clear();
                    //doSend.notify_all();
                    {
                        std::lock_guard<std::mutex> lck(doSendMutex);
                        doSend=true;
                        doSendCond.notify_one();                        
                    
                    }
                    LOGGERICE("on_recv ASK allindex=%d side=%d doSend(%p).release()\n",allindex,side,&doSend);

                    }
                 else
                    LOGGERICE("on_recv side=%d ASK trans_id=%d != send_trans_id=%d\n",side,head->trans_id,send_trans_id);
                return;
                }
         else {
            if(head->trans_id==receive_trans_id) {
                LOGGERICE("on_recv allindex=%d side=%d ASK acknowledged trans_id=%d same as receive_trans_id\n",allindex,side,head->trans_id);
                askedData=true;
                }
          else
                LOGGERICE("on_recv side=%d ASK acknowledged trans_id=%d != receive_trans_id=%d\n",side,head->trans_id,receive_trans_id);
            }
         };break;
       case SHUTDOWN: {
            sendShutdown=true;
            if(!head->ack) {
                head->ack=true;
                if(!sendWithError(agent, data,sizeof(udp_header))) {
                       return;
                       }
                setshutdown();
                LOGGERICE("allindex= %d side=%d received shutdown\n",allindex,side);
                }
            else {
                LOGGERICE("allindex=%d side=%d  shutdown acknowledged\n",allindex,side);
                }
            };break;
       case END: {
            if(!head->ack) {
                head->ack=true;
                if(!sendWithError(agent, data,sizeof(udp_header))) {
                       return;
                       }
                LOGGERICE("%d received END\n",side);
                if(ICEConnect *con=static_cast<ICEConnect *>(connections[allindex])) {
                        con->endConnectionHere();
                        }
                
                }
            else {
                LOGGERICE("%d  END acknowledged\n",side);
                sendStop=true;
                }
            };break;
       case DATA: {
           int index=head->index;
            if(shutdown) {
                LOGGERICE("on_recv side=%d: DATA index %d,but shutdown\n",side,index);
                if(!sendShutdown) 
                    sendshutDown(agent);
                return ;
                }
           if(!head->ack) {
              if(idSmallerOrEqual( head->trans_id,receive_trans_id)) {
                  head->ack=true;
                  if(!sendWithError(agent, data,sizeof(udp_header))) {
                           return;
                           }
                  if(head->trans_id==receive_trans_id) {
                    askedData=true;
                    LOGGERICE("trans_id=%d side=%d received packet %d\n",head->trans_id,side,index);
                    if(index>=0||index<acknowledged.size()) {
                        const char *input=data+sizeof(udp_header);
                        int  inputlen=size-sizeof(udp_header);
                        int pos= index*dataunit;
                        LOGGERICE("%d index %d wrote %d-%d\n",side,index,pos,pos+inputlen);
                        databuf.place(pos,input,inputlen);
                        if(head->fin) {
                            handledata(pos+inputlen);
                            ++receive_trans_id;
                            LOGGERICE("%d on_recv: final packet received, handle data ++receive_trans_id =%d\n",side,receive_trans_id); 
                            
                            }
                        }
                     return;
                      }
                   }
                LOGGERICE("%d received packet %d with different id=%d\n",side,index,head->trans_id);
                return;
                }
         else {
                ackeddata(agent,head);
                return;
              }
        };break;
    };
  }

bool ICE_data::sendRetry(juice_agent_t *agent,const char *data, int len) {
        switch(juice_send(agent, data,len)) {
            case JUICE_ERR_SUCCESS: return true;
            case JUICE_ERR_FAILED: 
                    break;
            case JUICE_ERR_AGAIN:
                    usleep(1000);
                    return juice_send(agent, data,len)==JUICE_ERR_SUCCESS;
            default: break;
            };
        return false;
        }
bool ICE_data::sendWithError(juice_agent_t *agent,const char *data, int len) {
        if(ICE_data::sendRetry(agent,data,  len))  {
            LOGGER("allindex=%d sendWithError success\n",allindex);
            return true;
            }
        ICEConnect *con=static_cast<ICEConnect *>(connections[allindex]);
        con->endConnectionHere(); 
        LOGGER("allindex=%d sendWithError failed\n",allindex);
        return false;
        }
void ICE_data::askdata(juice_agent_t *agent) {
        LOGGERICE("askdata receive_trans_id=%d side=%d RTO=%d\n",receive_trans_id,side,RTO);
        uint32_t rel_msec=getRelMsec(starttime);
        udp_header  head{.rel_msec=rel_msec,.com=ASK,.side=side,.ack=false,.trans_id=receive_trans_id};
        sendWithError( agent, reinterpret_cast<const char *>(&head),sizeof(udp_header));
        }

void ICE_data::sendStart(juice_agent_t *agent,ICEConnect *con) {
       LOGGERICE("sendStart allindex=%d side=%d\n",con->allindex,side);
       for(int i=0;!con->start_ack&&i<5;++i) {
            uint32_t rel_msec=getRelMsec(starttime);
            udp_header  head{.rel_msec=rel_msec,.com=START,.side=side,.ack=false,.trans_id=send_trans_id};
            if(!sendWithError(agent, reinterpret_cast<const char *>(&head),sizeof(udp_header)))
                    return;
            usleep(RTO*100);
            };
       LOGGERICE("end sendStart allindex=%d side=%d start_ack=%d\n",con->allindex,side,con->start_ack);
        }

uint32_t ICE_data::sendpacket(juice_agent_t *agent,uint16_t trans_id,const char *totaldata,int totallen,uint16_t index,uint32_t starttime) {
            const int start=index*dataunit;
            int left= totallen-start;
            bool fin;
            int sendsize;
            if(left>dataunit) {
                LOGGERICE("side=%d trans_id=%d sendpacket send non-final %d\n",side,trans_id,index);
                fin=false;
                sendsize=dataunit;
                }
            else {
                LOGGERICE("side=%d trans_id=%d sendpacket send final %d\n",side,trans_id,index);
                fin=true;
                sendsize=left;
                }
            alignas(16) char buffer[packetsize];
            uint32_t rel_msec=getRelMsec(starttime);
            *reinterpret_cast<udp_header*>(buffer)={.rel_msec=rel_msec,.com=DATA,.side=side,.fin=fin,.ack=false,.trans_id=trans_id,.index=index};
            memcpy(buffer+sizeof(udp_header), (const char *)totaldata+start,sendsize );
            sendWithError(agent, buffer,sizeof(udp_header)+sendsize);
            return rel_msec;
            }
int ICE_data::senddata(juice_agent_t *agent, const char *data,int len) {
        if(shutdown) {
           LOGGERICE("allindex=%d side=%d: senddata  shutdown 1\n",allindex,side);
           return -1;
           }
        if(!agent) {
           LOGGERICE("allindex=%d side=%d: senddata  agent==null 1\n",allindex,side);
           return -1;
           }
        LOGGERICE("allindex=%d side=%d senddata doSend(%p).acquire();\n",allindex,side,&doSend);
        time_t startWait=time(nullptr);
        static constexpr const int waitsec=4*60;
        {
        std::unique_lock<std::mutex> lck(doSendMutex);
        doSendCond.wait_for(lck,std::chrono::seconds(waitsec), [this] {return doSend; });   
        }
//        certain_try_acquire(doSend);
        if(shutdown) {
           LOGGERICE("allindex=%d side=%d: senddata  shutdown 2\n",allindex,side);
           return -1;
           }
        if(!agent) {
           LOGGERICE("allindex=%d side=%d: senddata  agent==null 2\n",allindex,side);
           return -1;
           }
        ICEConnect *con=static_cast<ICEConnect *>(connections[allindex]);
        if(!con) {
                LOGGERICE("side=%d senddata ICEConnect==null\n",side);
                return -1;
                }
        if(!con->isConnected||(!doSend&&(time(nullptr)-startWait)<waitsec)) {
                LOGGERICE("allindex=%d side=%d connect=%d doSend=%d wait\n",allindex,side,con->isConnected,doSend);
                {
                std::unique_lock<std::mutex> lck(doSendMutex);
                doSendCond.wait_for(lck,std::chrono::seconds(waitsec), [this] {return doSend; });   
                }
                if(!doSend) {
                    LOGGERICE("allindex=%dside=%d senddata !doSend\n",allindex,side);
                    return -1;
                    }
                }
        nextAck=0;
        lastAcked=false;
        sentAck=false;
        destruct _{[this]{
                nextAck=0;
                lastAcked=false;
                sentAck=false;
                lastpacket=-1;
                }};
        LOGGERICE("side=%d start senddata %.*s %d\n",side,40,data,len);
        const int trans_id=send_trans_id;
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        
        uint32_t starttime2=tv.tv_sec,rel_msec; 
        starttime=starttime2;
   //     uint32_t startmsec=tv.tv_usec/100;
        int totalminuslastunits=(len-1)/dataunit;
        lastpacket=totalminuslastunits; 
        bzero( &acknowledged,sizeof( acknowledged)); //only relevant once?
        if(totalminuslastunits>0) {
            for(int index=0;index<totalminuslastunits; index++) {
                rel_msec= sendpacket(agent, trans_id,data,len, index, starttime2);
                if(shutdown) {
                    LOGGERICE("%d senddata: shutdown 3\n",side);
                    return -1;
                    }
                usleep(10);
                }
             auto now = std::chrono::system_clock::now();
             auto   endwait=now+std::chrono::microseconds(RTO*timesRTO*100);
             while(true) {
                 std::unique_lock<std::mutex> lck( sendMutex);
                 sendCond.wait_until(lck,endwait, [this]{
                           return shutdown|| sentAck;
                           });
                 if(shutdown) {
                        LOGGERICE("%d senddata: shutdown 4\n",side);
                        return -1;
                        }
                  if(sentAck) {
                        break;
                        }
                    else {
                        auto waittime= std::chrono::microseconds(RTO*timesRTO*100);
                        auto newnow = std::chrono::system_clock::now();
                        auto waited=newnow-now;
                        LOGGER("waited=%u msec waittime=%u msec\n", 
std::chrono::duration_cast<std::chrono::milliseconds>(waited).count(),
std::chrono::duration_cast<std::chrono::milliseconds>(waittime).count());

                        if(waited>waittime) {
                             if(waited>std::chrono::seconds(10)) {
                                    LOGGERICE("%d senddata endConnection b\n",side);
                                    con->endConnection();
                                    return -1;
                                    }
                             int notack=0;
                             for(int index=nextAck;; ++index) {
                                 if(shutdown) {
                                        LOGGERICE("%d senddata: shutdown 8\n",side);
                                        return -1;
                                        }
                                if(nextAck>index)
                                    index=nextAck;
                                if(index>=totalminuslastunits)
                                    break;
                                if(!acknowledged[index]) {
                                        ++notack;
                                        LOGGERICE("side=%d trans_id %d senddata packet %d not acknowledged\n",side,trans_id,index);
                                        rel_msec= sendpacket(agent, trans_id,data,len, index, starttime2);
                                        if(shutdown) {
                                            LOGGERICE("%d senddata: shutdown 3\n",side);
                                            return -1;
                                            }
                                        }

                                 auto newwait= std::chrono::microseconds(RTO*timesRTO*100);
                                 auto newnow = std::chrono::system_clock::now();
                                 endwait=now+newwait;
                                 if(newnow<endwait) {
                                    LOGGER("wait longer waittime=%u waited=%u\n",std::chrono::duration_cast<std::chrono::milliseconds>(newwait).count(), std::chrono::duration_cast<std::chrono::milliseconds>(newnow-now).count());
                                    sendCond.wait_until(lck,endwait, [this]{
                                               return shutdown|| sentAck;
                                               });
                                    }
                                  if(sentAck) {
                                        break;
                                        }
                                 }
                              
                              if(!sentAck&&notack) {
                                    auto waittime= std::chrono::microseconds(RTO*timesRTO*100);
                                    now = std::chrono::system_clock::now();
                                    endwait= now + waittime;
                                    continue;
                                    }
                               else {
                                   break;
                                    }
                                }
                         else {
                               endwait=now+waittime;
                               continue;
                               }
                         }
                  //firstround=false;
                 }
              }
        rel_msec=sendpacket(agent, trans_id,data,len, totalminuslastunits, starttime2);
        auto now = std::chrono::system_clock::now();
        auto waittime= std::chrono::microseconds(RTO*timesRTO*100);
        auto endwait= now + waittime;
        while(true) {
            std::unique_lock<std::mutex> lck( sendMutex);
            sendCond.wait_until(lck,endwait, [this]{
                   return shutdown|| lastAcked;
                   });
           if(shutdown) {
                    LOGGERICE("%d senddata: shutdown 5\n",side);
                    return -1;
                    }
            if(lastAcked) {
                    break;
                    }
              else {
                auto newnow = std::chrono::system_clock::now();
                if(newnow>=endwait) {
                       LOGGER("trans_id=%d RTO=%d waittime=%d final took too long\n",trans_id,RTO,waittime);
                       if((newnow-now)>std::chrono::seconds(10)) {
                            LOGGERICE("%d senddata endConnection c\n",side);
                            con->endConnection();
                            return -1;
                            }
                        if(!acknowledged[totalminuslastunits]) {
                                LOGGERICE("trans_id=%d side=%d senddata final packet %d not acknowledged\n",trans_id,side,totalminuslastunits);
                                rel_msec= sendpacket(agent, trans_id,data,len, totalminuslastunits, starttime2);
                                if(shutdown) {
                                    LOGGERICE("%d senddata: shutdown 6\n",side);
                                    return -1;
                                    }
                                now=newnow;
                                endwait= newnow + waittime;
                                continue;
                                }
                          else {
                                break;
                            }
                        }
                     else {
                            continue;
                            }
                     }
                  }
        LOGGERICE("side=%d senddata last acknowledged send_trans_id =%d\n",side,send_trans_id);
        if(shutdown) {
            LOGGERICE("%d senddata: shutdown 5\n",side);
            return -1;
            }
        return len;

        }

void ICE_data::setshutdown() {
    shutdown=true;
    {
    std::lock_guard<std::mutex> lck(doSendMutex);
    doSend=true;
    doSendCond.notify_one();                        
    }
    LOGGERICE("allindex=%d side=%d setshutdown() doSend(%p).release()\n",allindex,side,&doSend);
    receiveCond.notify_all();
    sendCond.notify_all();
    }

void ICE_data::sendshutDown(juice_agent_t *agent) {
       ICEConnect *con=static_cast<ICEConnect *>(connections[allindex]);
       if(!con) {
          LOGGERICE( "ICE_data::end connections[%d]==null\n",allindex);
          return;
          }
       if(!con->isConnected)  {
          LOGGERICE( "ICE_data::end side=%d allindex=%d !isConnected\n",side,allindex);
          return;
          }
       if(!agent) {
          LOGGERICE( "ICE_data::end side=%d allindex=%d agent==null\n",side,allindex);
          return;
          }
       LOGGERICE("sendshutDown(%p) allindex=%d\n",agent,allindex);
       for(int i=0;!sendShutdown&&i<5;++i) {
            uint32_t rel_msec=getRelMsec(starttime);
            udp_header  head{.rel_msec=rel_msec,.com=SHUTDOWN,.side=side,.ack=false,.trans_id=send_trans_id};
            if(!sendWithError(agent, reinterpret_cast<const char *>(&head),sizeof(udp_header))) {
                return;
                }
            usleep(RTO*100);
            }
       LOGGERICE("end sendshutDown(%p) allindex=%d sendShutDown=%d\n",agent,allindex,sendShutdown);
    }
void ICE_data::shutDown(juice_agent_t *agent) {
          setshutdown();
          if(!sendShutdown) {
               LOGGERICE("shutDown send_trans_id=%d side=%d\n",send_trans_id,side);
               sendshutDown(agent);
               }
           else
               LOGGERICE("shutDown send_trans_id=%d side=%d already done\n",send_trans_id,side);
            }
void ICE_data::end(juice_agent_t *agent) {
       setshutdown();
       LOGGERICE("ICE_data::end allindex=%d send_trans_id=%d side=%d\n",allindex,send_trans_id,side);
       ICEConnect *con=static_cast<ICEConnect *>(connections[allindex]);
       if(!con) {
          LOGGERICE( "ICE_data::end connections[%d]==null\n",allindex);
          return;
          }
       if(!con->isConnected)  {
          LOGGERICE( "ICE_data::end side=%d allindex=%d !isConnected\n",side,allindex);
          return;
          }
       if(!agent) {
          LOGGERICE( "ICE_data::end side=%d allindex=%d agent==null\n",side,allindex);
          return;
          }
       for(int i=0;!sendStop&&i<5;++i) {
            uint32_t rel_msec=getRelMsec(starttime);
            udp_header  head{.rel_msec=rel_msec,.com=END,.side=side,.ack=false,.trans_id=send_trans_id};
            if(!sendWithError(agent, reinterpret_cast<const char *>(&head),sizeof(udp_header)))
                    return;
            usleep(RTO*100);
            }
         LOGGERICE("end ICE_data::end  side=%d\n",side);
        }

int ICE_data::receive(juice_agent_t *agent,char *buf, const int maxbuf) {
        if(shutdown) {
           LOGGERICE("ICE_data::receive side=%d, but shutdown\n",side);
           resetReceive();
          if(!sendShutdown) 
               sendshutDown(agent);
           return -1;
           }
        if(!agent) {
           LOGGERICE("ICE_data::receive side=%d, but agent=null\n",side);
           resetReceive();
           return -1;
           }
        std::unique_lock<std::mutex> lck( receiveMutex);
        intmax_t waittime=RTO*100*timesRTO;
        int asked=0;
        uint32_t timebegin=time(nullptr);
        while(true) {
            LOGGERICE("side=%d ICE_data::receive before wait_for %d microsecs\n",side,waittime);
            receiveCond.wait_for(lck,std::chrono::microseconds(waittime), [this,agent,&waittime,&asked]{
                if(shutdown) {
                    LOGGERICE("allindex=%d side=%d receive: wait_for shutdown\n",allindex,side);
                    return true;
                    }
                if(datalen>offset)
                    return true;
                if(!askedData) {
                    ICEConnect *con=static_cast<ICEConnect *>(connections[allindex]);
                    if(con->isConnected)  {
                           if(asked>10) {
                                if((time(nullptr)-timebegin)>5*60)
                                    con->endConnection();
                                    return true;
                                    }
                                }
                           waittime=RTO*100*10;
                           askdata(agent);
                           ++asked;
                           if(shutdown) {
                                LOGGERICE("%d receive: askdata shutdown\n",side);
                                return true;
                                }
                           }
                    else {
                        LOGGER("allindex=%d side=%d receive:~isConnected\n",allindex,side);
                        waittime=5*60*1000*1000;
                        }
                    }
                 else {
                      LOGGER("allindex=%d side=%d !askdata\n",allindex,side);
                      waittime=5*60*1000*1000;
                      }
                 return datalen>offset;
                 });
               if(shutdown) {
                    return -1;
                    }

               if(datalen>offset)
                  break;
              };
        if(shutdown) {
                LOGGERICE("%d receive: return shutdown\n",side);
               if(!sendShutdown) 
                       sendshutDown(agent);
                return -1;
                }
        int left=databuf.size()-offset;
        int use=std::min(left,maxbuf);
        memcpy(buf,&databuf[offset],use);
        LOGGERICE("receivedata offset=%d len=%d\n",offset,use);
        offset+=use;
        if(offset==datalen) {
            resetReceive();
            }
        return use;
        }
