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
void ICE_data::on_recv(juice_agent_t *agent, const char *data, size_t size,int allindex) {
   udp_header  *head=const_cast<udp_header *>(reinterpret_cast<const udp_header *>(data));
   switch(head->com) {
        case ASK: {
            if(shutdown) {
                LOGGERICE("on_rev: side=%d ASK, but shutdown\n",side);
                return;
                }
            if(!head->ack) {
                if(head->trans_id==send_trans_id) {
                    LOGGERICE("on_recv side=%d ASK trans_id=%d same as send_trans_id\n",side,head->trans_id);
                    head->ack=true;
                    juice_send(agent,data,sizeof(udp_header));
                    doSend.release();
                    LOGGERICE("side=%d doSend(%p).release()\n",side,&doSend);

                    }
                 else
                    LOGGERICE("on_recv side=%d ASK trans_id=%d != send_trans_id=%d\n",side,head->trans_id,send_trans_id);
                }
         else {
            if(head->trans_id==receive_trans_id) {
                LOGGERICE("on_recv side=%d ASK acknowledged trans_id=%d same as receive_trans_id\n",side,head->trans_id);
                askedData=true;
                }
          else
                LOGGERICE("on_recv side=%d ASK acknowledged trans_id=%d != receive_trans_id=%d\n",side,head->trans_id,receive_trans_id);
            }
         };break;
       case SHUTDOWN: {
            if(!head->ack) {
                head->ack=true;
                juice_send(agent,data,sizeof(udp_header));
                setshutdown();
                LOGGERICE("%d received shutdown\n",side);
                }
            else {
                LOGGERICE("%d  shutdown acknowledged\n",side);
                sendShutdown=true;
                }
            };break;
       case END: {
            if(!head->ack) {
                head->ack=true;
                juice_send(agent,data,sizeof(udp_header));
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
                return ;
                }
           if(!head->ack) {
              if(head->trans_id<=receive_trans_id) {
                  head->ack=true;
                  juice_send(agent,data,sizeof(udp_header));
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
                }
           else {
              if(head->trans_id==send_trans_id) {
                  if(index<0||index>=acknowledged.size()) {
                    LOGGERICE("%d ERROR received ack for index %d\n",side,index);
                     return;
                     }
                  if(head->fin) {
                    certain_try_acquire(doSend);
                    LOGGERICE("recv side=%d  fin doSend(%p).try_acquire()\n",side,&doSend);
                    ++send_trans_id;
                    LOGGERICE("last ack ++send_trans_id=%d\n",send_trans_id);
                    }
                  acknowledged[index]=true;
                  uint32_t rel_msec=getRelMsec(starttime) ;
                  int ack_msec_tmp=rel_msec-head->rel_msec;
                  ack_msec=(ack_msec_tmp+1+head->fin);
                  LOGGERICE("%d trans_id=%d received acknowledgement for %d new ack_msec=%d\n",side,head->trans_id,index,ack_msec);
                  }
             else {
                  LOGGERICE("%d received acknowledgement for %d for different trans_id %d != %d\n",side,index,head->trans_id,send_trans_id);
                  }
              }
        };break;
    };
  }

void ICE_data::askdata(juice_agent_t *agent) {
        LOGGERICE("askdata receive_trans_id=%d side=%d\n",receive_trans_id,side);
        uint32_t rel_msec=getRelMsec(starttime);
        udp_header  head{.rel_msec=rel_msec,.com=ASK,.side=side,.ack=false,.trans_id=receive_trans_id};
        juice_send(agent, reinterpret_cast<const char *>(&head),sizeof(udp_header));
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
            juice_send(agent, buffer,sizeof(udp_header)+sendsize);
            return rel_msec;
            }
int ICE_data::senddata(juice_agent_t *agent, const char *data,int len) {
        if(shutdown) {
           LOGGERICE("side=%d: senddata  shutdown 1\n",side);
           return -1;
           }
        if(!agent) {
           LOGGERICE("side=%d: senddata  agent==null 1\n",side);
           return -1;
           }
        LOGGERICE("side=%d senddata doSend(%p).acquire();\n",side,&doSend);
        doSend.acquire();
        certain_try_acquire(doSend);
        if(shutdown) {
           LOGGERICE("side=%d: senddata  shutdown 2\n",side);
           return -1;
           }
        if(!agent) {
           LOGGERICE("side=%d: senddata  agent==null 2\n",side);
           return -1;
           }
        ICEConnect *con=static_cast<ICEConnect *>(connections[allindex]);
        if(!con) {
                LOGGERICE("side=%d senddata ICEConnect==null\n",side);
                return -1;
                }
        if(!con->isConnected) {
                LOGGERICE("side=%d Not connected, wait\n",side);
                doSend.acquire();
                }
        // Agent 1: on connected, send a message
        LOGGERICE("side=%d start senddata %.*s %d\n",side,40,data,len);
        const int trans_id=send_trans_id;
        int index=0;
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        
        uint32_t starttime2=tv.tv_sec,rel_msec; 
        starttime=starttime2;
        int totalminuslastunits=(len-1)/dataunit;
        bzero( &acknowledged,sizeof( acknowledged)); //only relevant once?
        for(;index<totalminuslastunits; index++) {
            rel_msec= sendpacket(agent, trans_id,data,len, index, starttime2);
            if(shutdown) {
                LOGGERICE("%d senddata: shutdown 3\n",side);
                return -1;
                }
            usleep(1);
            }
        LOGGERICE("side=%d senddata: send %d packets\n",side,totalminuslastunits);
       const int sendnr=index+1;
        uint32_t unit=std::max(uint32_t(1),rel_msec/sendnr);
        int notack;
        int first=0;
        int left=totalminuslastunits;
        do {
            notack=0;
            for(int i=first;i<totalminuslastunits;) {
                if(!first) {
                    int waited= rel_msec-i*unit;
                    if(waited<ack_msec) {
                        int wait=ack_msec-std::max(waited,0);
                        usleep(wait*10000);
                        LOGGERICE("usleep(%d)\n",wait*10000);
                        rel_msec+=wait;
                        }
                     }
                if(!acknowledged[i]) {
                    if(!notack)
                        first=i;
                    LOGGERICE("side=%d senddata packet %d not acknowledged\n",side,i);
                    rel_msec=sendpacket(agent, trans_id,data,len, i, starttime2);
                    if(shutdown) {
                        LOGGERICE("%d senddata: shutdown 3\n",side);
                        return -1;
                        }
                    ++notack;
                    }
                else {
//                        LOGGERICE("senddata packet %d acknowledged\n",i);
                        }
                ++i;
                }
             if(!notack)
                break;
             if(notack==left) {
                if(rel_msec>20000) {
                    LOGGERICE("%d senddata endConnection a\n",side);
                    con->endConnection();
                    return -1;
                    }
                }
             left=notack;
             usleep(ack_msec*10000);
             } while(true);
        if(shutdown) {
            LOGGERICE("%d senddata: shutdown 4\n",side);
            return -1;
            }
        LOGGERICE("senddata all first %d acknowledged\n",totalminuslastunits);
        rel_msec=sendpacket(agent, trans_id,data,len, totalminuslastunits, starttime2);
        uint32_t start= rel_msec;
        usleep(ack_msec*100*2);
        while(!acknowledged[totalminuslastunits]) {
            if(shutdown) {
                LOGGERICE("%d senddata: shutdown 4\n",side);
                return -1;
                }
           if((rel_msec-start)>20000) {
                LOGGERICE("%d senddata endConnection b\n",side);
                con->endConnection();
                return -1;
                }
            LOGGERICE("side=%d senddata packet %d not acknowledged\n",side,totalminuslastunits);
            rel_msec=sendpacket(agent, trans_id,data,len, totalminuslastunits, starttime2);
            usleep(ack_msec*100*2);
            }
//        ++send_trans_id;
        LOGGERICE("side=%d senddata last acknowledged send_trans_id =%d\n",side,send_trans_id);
        if(shutdown) {
            LOGGERICE("%d senddata: shutdown 5\n",side);
            return -1;
            }
        return len;

        }


void ICE_data::setshutdown() {
    shutdown=true;
    doSend.release();
    LOGGERICE("side=%d setshutdown() doSend(%p).release()\n",side,&doSend);
    condi.notify_all();
    }
void ICE_data::shutDown(juice_agent_t *agent) {
           setshutdown();
           LOGGERICE("shutdown send_trans_id=%d side=%d\n",send_trans_id,side);
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
           for(int i=0;!sendShutdown&&i<5;++i) {
                uint32_t rel_msec=getRelMsec(starttime);
                udp_header  head{.rel_msec=rel_msec,.com=SHUTDOWN,.side=side,.ack=false,.trans_id=send_trans_id};
                juice_send(agent, reinterpret_cast<const char *>(&head),sizeof(udp_header));
                usleep(ack_msec*100*2);
                }
            }
void ICE_data::end(juice_agent_t *agent) {
       setshutdown();
       LOGGERICE("ICE_data::end send_trans_id=%d side=%d\n",send_trans_id,side);
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
            juice_send(agent, reinterpret_cast<const char *>(&head),sizeof(udp_header));
            usleep(ack_msec*100*2);
            }
         LOGGERICE("end ICE_data::end  side=%d\n",side);
        }

int ICE_data::receive(juice_agent_t *agent,char *buf, const int maxbuf) {
        if(shutdown) {
           LOGGERICE("ICE_data::receive side=%d, but shutdown\n",side);
           resetReceive();
           return -1;
           }
        if(!agent) {
           LOGGERICE("ICE_data::receive side=%d, but agent=null\n",side);
           resetReceive();
           return -1;
           }
        std::unique_lock<std::mutex> lck(mutex);
        intmax_t waittime=ack_msec*10*4;
        while(true) {
            LOGGERICE("side=%d ICE_data::receive before wait_for %d msec\n",side,waittime);
            condi.wait_for(lck,std::chrono::milliseconds(waittime), [this,agent,&waittime]{
                if(shutdown) {
                    LOGGERICE("%d receive: wait_for shutdown\n",side);
                    return true;
                    }
                if(datalen>offset)
                    return true;
                if(!askedData) {
                    ICEConnect *con=static_cast<ICEConnect *>(connections[allindex]);
                    if(con->isConnected)  {
                                askdata(agent);
                                }
                    else {
                        waittime=5*60*1000;
                        }
                    }
                 else {
                      waittime=5*60*1000;
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
                return -1;
                }
       // sema.acquire(); 
//            destruct destr{[this]{sema.release();}};
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
