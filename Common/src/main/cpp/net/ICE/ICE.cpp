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


constexpr const int givefirst=0;
//#define TEST
/*
#define xquotes(s) quotes(s)
#define quotes(s) #s

#define SIDE xquotes(NSIDE) */
#define LOGGERICE(...) LOGGER("ICE: " __VA_ARGS__)
#define LOGARICE(...) LOGAR("ICE: " __VA_ARGS__)

#include "datbackup.hpp"
#include "libjuice/include/juice/juice.h"
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <thread>
#include <bitset>
#include <assert.h>
#include <semaphore>
#include <condition_variable>

#include "logs.hpp"
#include "ContextHTTPS.hpp"
using namespace std::literals;
#include "Agent_data.hpp"
#include "BackDescription.hpp"
#include "inout.hpp"
#include  "udp_header.h"
#include "destruct.hpp"
#include "PlaceBuf.hpp"
#include "ICEConnect.hpp"

constexpr const int maxconnectionunused=24*60*60;
extern uint32_t getConnectTime(const int allindex);
extern void setConnectTime(const int allindex,uint32_t tim);
constexpr const std::string_view hostnames[]{
#include "jugglucoconnect.h"
};
constexpr int nrhostnames=std::size(hostnames);
#include <zlib.h>
constexpr const uLong hashfunc(const char *d, int len) {
    return crc32(0,(const unsigned char*)d,len);
    }
int hostselect(std::string_view name) {
    int hash=hashfunc(name.data(),name.size())%nrhostnames;
    LOGGERICE("hostselect(%.*s)=%d %.*s\n",name.data(),name.data(),hash,hostnames[hash].size(),hostnames[hash].data());
    return hash;
    }
int port{6789};
#ifndef LOGGER
#define LOGGER(...) fprintf(stderr,__VA_ARGS__)
#endif
#define BUFFER_SIZE 4096

#define JUICE_ERR_SUCCESS 0

static bool stillworking(int allindex)  {
    ICEConnect *con=static_cast<ICEConnect *>(connections[allindex]);
    bool res=con&&!con->finish&&con->allindex==allindex;
    if(!res)  {
        LOGGERICE("stillworking(%d)=%d\n",allindex,res);
        }
    return res;
    }
const char *juiceErrorString(int error) {
    switch(error) {
        case JUICE_ERR_SUCCESS : return "success";
        case JUICE_ERR_INVALID: return "invalid argument";
        case JUICE_ERR_FAILED: return "runtime error";
        case JUICE_ERR_NOT_AVAIL: return "element not available";
        case JUICE_ERR_IGNORED: return "ignored";
        case JUICE_ERR_AGAIN: return "buffer full";
        case JUICE_ERR_TOO_LARGE: return "datagram too large";
        default: return "Unknown error";
        };
 }


static void on_state_changed1(juice_agent_t *agent, juice_state_t state, void *user_ptr);

static void on_candidate1(juice_agent_t *agent, const char *sdp, void *user_ptr);

static void on_gathering_done1(juice_agent_t *agent, void *user_ptr);

static void on_recv1(juice_agent_t *agent, const char *data, size_t size, void *user_ptr);

class CreateAgentData {
public:
   CreateAgentData(std::string_view commonLabel,bool side,const char *sdp,int sdplen=-1):
    agent(Agent_data::newAgent('0'+side,commonLabel,{sdp,(sdplen==-1?strlen(sdp):sdplen)})) {
            };
    ~CreateAgentData() {
        Agent_data::deleteAgent(agent);
        }
    Agent_data *agentdata() {
        return agent;
        }
    const char *data() const {
        return reinterpret_cast<const char*>(agent);
        }
     int size() const {
        return agent->datalen();
        }
      std::span<const char> getSpan() const {
        return {data(),size_t(size())};
        }
     operator std::span<const char>() const {
        return getSpan();
        }
private:
    Agent_data *agent;
    };
//static bool gathering_done=false;

static void getAddressesThread(juice_agent *agent,std::string_view commonLabel,bool side,std::string_view hostname) {
   static std::string_view address{"/address"};
   CreateAgentData addressdata(commonLabel,side,"") ;
   int errors=0;
   while(errors<5) {
            LOGGERICE("getaddress %s %d\n",commonLabel.data(),side);
            auto [resbody,code]=ContextHTTPS::getContext().getRequest(hostname,port,address,addressdata.getSpan());
            switch(code) {
                case 200: {
                if(resbody.size()>= (sizeof(BackDescription )+20)) {
                    errors=0;
                    const BackDescription *other=reinterpret_cast<const BackDescription *>(resbody.data());
                    int res=juice_add_remote_candidate(agent, other->description);
                    LOGGERICE("%s %d: getaddress %s res=%d\n",commonLabel.data(),side,other->description,res);
                    }
                 else {
                    juice_set_remote_gathering_done(agent);
                    LOGGERICE("getaddress %s %d: juice_set_remote_gathering_done\n",commonLabel.data(),side);
                    return;
                    }
                };break;
              case 400: {
                LOGGERICE("getaddress %s %d: ERROR try again\n",commonLabel.data(),side);
                sleep(2);
                ++errors;
                  };break;
              default: {
                LOGGERICE("getaddress %s %d: Http error\n",commonLabel.data(),side);
                sleep(10);
                ++errors;
                };break;
                };
          }
      LOGGERICE("getaddress %s %d: end thread\n",commonLabel.data(),side);
     } 



static void on_candidate1(juice_agent_t *agent, const char *sdp, void *user_ptr) {
   const int allindex=(int)(long)user_ptr;
    ICEConnect *con=static_cast<ICEConnect *>(connections[allindex]);

   const passhost_t &host= getBackupHosts()[allindex];
   static std::string_view address{"/address"};
   CreateAgentData sdpdata(host.getICEname(),host.side,sdp) ;
   for(int i=0;i<20;++i) {
       auto [resbody,code]=ContextHTTPS::getContext().putRequest(hostnames[con->hostindex],port,address,std::span((const char *)sdpdata.data(),sdpdata.size()));
       if(code==200) {
             LOGGERICE( "putaddress %s %d: success: %s\n",host.getICEname().data(),host.side, sdp);
            break;
            }
       if(code==400) {
            LOGGERICE( "putaddress %s %d: failed: %s\n",host.getICEname().data(),host.side, sdp);
            break;
            }
      LOGGERICE("putaddress %s %d: ERROR: %s\n",host.getICEname().data(),host.side,  sdp);
      sleep(20);
      }
   }


// Agent 1: on local candidates gathering done
static void on_gathering_done1(juice_agent_t *agent, void *user_ptr) {
   const int allindex=(int)(long)user_ptr;
    //gathering_done=true;
    std::thread th{[allindex] {
        const passhost_t &host= getBackupHosts()[allindex];
        LOGGERICE("Gathering done %s %d\n",host.getICEname().data(),host.side);
        ICEConnect *con=static_cast<ICEConnect*>(connections[allindex]);
        if(!con) {
            LOGGERICE("connection[%d]==NULL\n",allindex);
            return;
            }
        CreateAgentData body(host.getICEname(),host.side,con->sdp,con->sdplen);
        std::string_view done{"/done"sv};
        con->startDone.wait(true);
        while(true) {
            auto [resbody,code]=ContextHTTPS::getContext().putRequest(hostnames[con->hostindex],port,done,std::span((const char *)body.data(),body.size()));
            if(code==200) {
                LOGGERICE("%s %d: OK DONE\n",host.getICEname().data(),host.side);
                break;
               }
            if(code==400) {
                LOGGERICE("%s %d: WRONG DONE\n",host.getICEname().data(),host.side);
                break;
                }
            LOGGERICE("%s %d: ERROR DONE code=%d\n",host.getICEname().data(),host.side,code);
            sleep(10);
            if(con->finish) {
                LOGARICE("Finish  thread");
                return;
                }
              };
          }};
       th.detach();
    }

// Agent 2: on local candidates gathering done

// Agent 1: on message received
#include "ICE_data.hpp"
static void on_recv1(juice_agent_t *agent, const char *data, size_t size, void *user_ptr) {
    const int allindex=(int)(long)user_ptr;
    const passhost_t &host= getBackupHosts()[allindex];
    if(!host.ICE) {
            LOGGERICE("ERROR: on_recv1 called on non-ICE host allindex=%d name=%s\n",allindex, host.getICEname());
            return;
            }
    ICEConnect *con=static_cast<ICEConnect *>(connections[allindex]);
    ICE_data *userdata=con->icedata;
    udp_header  *head=const_cast<udp_header *>(reinterpret_cast<const udp_header *>(data));
    userdata[head->side!=host.side].on_recv(agent,data,size,allindex);
    }

static bool diagnostics(juice_agent *agent,const char *name,bool side) {
    bool success=true;
    // Retrieve candidates
    char local[JUICE_MAX_CANDIDATE_SDP_STRING_LEN];
    char remote[JUICE_MAX_CANDIDATE_SDP_STRING_LEN];
    if (int  res=juice_get_selected_candidates(agent, local, JUICE_MAX_CANDIDATE_SDP_STRING_LEN, remote,
                                       JUICE_MAX_CANDIDATE_SDP_STRING_LEN);res==0) {
        LOGGERICE("%s %d: Local candidate: %s\n",name,side, local);
        LOGGERICE("%s %d: Remote candidate: %s\n",name,side, remote);
        }
    else {
        success=false;
        }
    // Retrieve addresses
    char localAddr[JUICE_MAX_ADDRESS_STRING_LEN];
    char remoteAddr[JUICE_MAX_ADDRESS_STRING_LEN];
    if (int res=juice_get_selected_addresses(agent, localAddr, JUICE_MAX_ADDRESS_STRING_LEN, remoteAddr, JUICE_MAX_ADDRESS_STRING_LEN);res == 0) {
        LOGGERICE("%s %d: Local address: %s\n", name,side,localAddr);
        LOGGERICE("%s %d: Remote address: %s\n", name,side,remoteAddr);
    }
   else {
        LOGGERICE("%s %d: juice_get_selected_addresses failed: %s (%d)\n",name,side,juiceErrorString(res),res);
        success=false;
      }
      return success;
    }

/*
void startICEReceiver(passhost_t *host,ICEConnect *con) {
    LOGGERICE("start startICEReceiver %s\n",host->getname());
    struct FUNC {
        static void connect( passhost_t *host,ICEConnect *con) {
            LOGGERICE("startICEReceiver %s\n",host->getname());
            sleep(5);
            con->connect(host);
            }
        };
    std::thread th(FUNC::connect,host,con);
    th.detach();
    } */
extern void receiverthread(passhost_t *host,const int allindex);
static void on_state_changed1(juice_agent_t *agent, juice_state_t state, void *user_ptr) {
   const int allindex=(int)(long)user_ptr;
    LOGGERICE("on_state_changed1 allindex=%d\n",allindex);
    if(!stillworking(allindex))
        return;
    const passhost_t &host= getBackupHosts()[allindex];
    LOGGERICE("%s %d State: %s\n", host.getICEname().data(),host.side,juice_state_to_string(state));
    ICEConnect *con=static_cast<ICEConnect*>(connections[allindex]);
    switch(state) {
        case	JUICE_STATE_GATHERING:
        case	JUICE_STATE_CONNECTING:
            con->notConnected();
            break;
        case JUICE_STATE_CONNECTED: {
            setConnectTime(allindex,0);
            con->setConnected();
            con->agent.store(agent);
            struct CONNECTED {
                static void thread( juice_agent_t *agent, int allindex) {
                   LOGGERICE("start CONNECT::thread allindex=%d\n",allindex);
                    passhost_t &host= getBackupHosts()[allindex];
                    ICEConnect *con=static_cast<ICEConnect*>(connections[allindex]);
                    if(!con) {
                        LOGGERICE("connection[%d]==NULL\n",allindex);
                        return;
                        }
                    if(con->finish) {
                        LOGGERICE("allindex=%d Finish  thread\n",allindex);
                        return;
                        }

                   con->icedata[host.side].sendStart(agent,con);
                   con->startSending.wait(true);
                   LOGGERICE("allindex=%d After con->startSending.wait(true)\n",allindex);
                   {
                   std::lock_guard<std::mutex> lck(con->receiveThreadMutex);
                   con->wakeReceiver=true;
                   con->receiveThreadCon.notify_one();
                   }
                   con->startDone.clear();
                   con->startDone.notify_all();
                   };
                   };
            std::thread th(CONNECTED::thread,agent,allindex);
            th.detach();
            diagnostics(agent,host.getICEname().data(),host.side);
            }; break;
        case JUICE_STATE_FAILED: {
            con->notConnected();
            struct Failure {
                static void thread( juice_agent_t *agent, int allindex) {
                    const passhost_t &host= getBackupHosts()[allindex];
                    ICEConnect *con=static_cast<ICEConnect*>(connections[allindex]);
                    if(!con) {
                        LOGGERICE("connections[%d]==NULL\n",allindex);
                        return;
                        }
                    CreateAgentData body(host.getICEname(),host.side,con->sdp,con->sdplen);
                    std::string_view failure{"/failure"sv};
                    for(int i=0;i<20;++i) {
                        auto [resbody,code]=ContextHTTPS::getContext().putRequest(hostnames[con->hostindex],port,failure,std::span((const char *)body.data(),body.size()));
                        if(code==200) {
                            LOGGERICE("%s %d: OK FAILURE\n",host.getICEname().data(),host.side);
                            break;
                            }
                       LOGGERICE("%s %d: ERROR FAILURE code=%d\n",host.getICEname().data(),host.side,code);
                       sleep(20);
                       }
                    con->notConnected();
                    con->endConnectionHere();
                    if(con->finish) {
                        LOGARICE("Finish Failure::thread");
                        return;
                        }
                   {
                   std::lock_guard<std::mutex> lck(con->receiveThreadMutex);
                   con->wakeReceiver=true;
                   con->receiveThreadCon.notify_one();
                   }
                    };
                    };
            std::thread th(Failure::thread,agent,allindex);
            th.detach();
             }
            break;

        default:break;

        };
    LOGARICE("end on_state_changed1");
    }

void ICEConnect::receiverThread(int argindex) {
    #ifndef HAVE_NOPRCTL
      constexpr const int maxbuf=14;
      char name[14];
      snprintf(name,maxbuf,"ICE Receive %d",argindex);
       prctl(PR_SET_NAME, name, 0, 0, 0);
    #endif
    passhost_t &host= getBackupHosts()[argindex];
    LOGGERICE("start receiverThread argindex=%d\n",argindex);
    int waitsec=host.isSender()?120:1;
    while(true) {
        if(argindex!=allindex) {
            LOGARICE("receiverThread: allindex changed, return");
            return;
            }
        if(finish) {
            LOGARICE("Finish receiverThread");
            return;
            }

        LOGGER("receiverThread  before wait_for %d seconds\n",waitsec);
        {
        std::unique_lock<std::mutex> lck(receiveThreadMutex);
        receiveThreadCon.wait_for(lck,std::chrono::seconds(waitsec), [this] {return wakeReceiver; });   
        }
        wakeReceiver=false;
        if(isConnected) {
            notifyReceive();
            if(receiveConnect(&host)) {
                LOGARICE("running receiverthread");
                receiverthread(&host,argindex);
                sleep(1);
//                icedata[1].reStarted();
                waitsec=70;
                }
             else {
                waitsec=60;
                }
             }
         if(!host.isSender()) {
                LOGGERICE("allindex=%d %d receiverThead, make connect\n",allindex,host.side);
                if(finish) {
                    LOGARICE("2: Finish done thread");
                    return;
                    }
                switch(connect(&host)) {
                    case 1: {
                           LOGGERICE("side=%d receiverThread: connected\n",host.side);
                           waitsec=1;
                           continue;
                           };
                    case -2:
                        LOGGERICE("side=%d receiverThread: connect continue old agent\n",host.side);
                        waitsec=1;
                        continue;
                    case 0:
                        LOGGERICE("side=%d receiverThread: already connecting\n",host.side);
                        waitsec=2*60;
                        continue;
                    case -1: 
                        LOGGERICE("side=%d receiverThread: error retry\n",host.side);
                        waitsec=5*60;
                        continue;
                    };
                     break;
             }
         else {
            waitsec=5*60;
            continue;
            }
        waitsec=70;
        }
    };

void startReceiverThread(int allindex) {
    if(ICEConnect *con=static_cast<ICEConnect *>(connections[allindex])) {
        LOGGER("startReceiverThread(%d)\n",allindex);
        std::thread th{&ICEConnect::receiverThread,con,allindex};
        th.detach();
        }
    else {
        LOGGER("startReceiverThread() connections[%d]=NULL\n",allindex);
        }
    }
static  void juice_logger(juice_log_level_t level, const char *message) {
        if(message) {
            LOGGER("libjuice%d: %s\n",level,message);
            }
        else
            LOGGER("libjuice%d: NO MESSAGE!!!\n",level);
        }
//juice_log_level_t juice_log_level=JUICE_LOG_LEVEL_DEBUG;
//juice_log_level_t juice_log_level=JUICE_LOG_LEVEL_VERBOSE;
juice_log_level_t juice_log_level=JUICE_LOG_LEVEL_WARN;
//   juice_set_log_level(JUICE_LOG_LEVEL_WARN);
juice_agent *createAgent(int allindex) {
   juice_set_log_level(juice_log_level);
   juice_set_log_handler(&juice_logger);

   LOGGER("createAgent(%d)\n",allindex);
                        
          
static constexpr const  juice_turn_server_t default_turn_servers[]{
    #include "turnservers.hpp"
         } ;
static constexpr const  int defaultservercount=std::size(default_turn_servers);
const juice_turn_server_t *turn_servers;
int servercount;

juice_turn_server_t conf_server;
if(backup->getupdatedata()->NRturnserver) {
    conf_server.host=backup->getupdatedata()->turnserver[0].hostname;
    conf_server.username=backup->getupdatedata()->turnserver[0].username;
    conf_server.password=backup->getupdatedata()->turnserver[0].password;
    conf_server.port=backup->getupdatedata()->turnserver[0].port;
    servercount=1;
    turn_servers=&conf_server;
    }
else {
    servercount=defaultservercount;
    turn_servers=default_turn_servers;
    }
    juice_config_t config1{
        .concurrency_mode=JUICE_CONCURRENCY_MODE_THREAD,
        .stun_server_host = "stun.l.google.com",
        .stun_server_port = 19302,
        .turn_servers=(juice_turn_server_t*)  turn_servers,
        .turn_servers_count=servercount,  
        .cb_state_changed = on_state_changed1,
        .cb_candidate = on_candidate1,
        .cb_gathering_done = on_gathering_done1,
        .cb_recv = on_recv1,
        .user_ptr=(void*)(long)allindex
        };
   auto*ret= juice_create(&config1);
   LOGGER("end createAgent(%d)=%p\n",allindex,ret);
   return ret;
  };
static std::string_view description="/description";

static bool waitonDescription(juice_agent *agent,std::string_view commonLabel,int side,std::string_view hostname) {
    CreateAgentData sdpdata(commonLabel,side,"");
    LOGGERICE("getdescription %s\n",sdpdata.data());
    while(true) {
        auto [resbody,code]=ContextHTTPS::getContext().getRequest(hostname,port,description,sdpdata.getSpan());
        if(code== 200) {
            if(resbody.size()>= (sizeof(BackDescription )+20)) {
                const BackDescription *other=reinterpret_cast<const BackDescription *>(resbody.data());
                LOGGERICE("getdescription SUCCESS: %s %d: Remote description in:\n%s\n",commonLabel.data(),side,other->description);
                juice_set_remote_description(agent, other->description);
                return true;
                }
             else {
                LOGGERICE("getdescription failure %s size=%d: getdescription Remote small body in :\n%.*s\n",commonLabel.data(),(int)resbody.size(),(int)resbody.size(),(const char *)resbody.data());
                sleep(1);
                }
            }
         else {
            LOGGERICE("getdescription failure %s %d: %s returns code=%d\n",commonLabel.data(),side,sdpdata.data(),code); 
            sleep(20);
            }
        }
//    return false;
    }
static  bool putDescription(int allindex,juice_agent *agent,std::string_view commonLabel,bool side,std::string_view hostname)  {
    ICEConnect *con=static_cast<ICEConnect *>(connections[allindex]);
    if(const int error=juice_get_local_description(agent, con->sdp, JUICE_MAX_SDP_STRING_LEN);JUICE_ERR_SUCCESS!=error) {
        LOGGERICE("%s %d: juice_get_local_description failed: %s (%d)\n",commonLabel.data(),side,juiceErrorString(error),error);
        return  false;
        }
     con->sdplen=strlen(con->sdp);
    CreateAgentData sdpdata(commonLabel,side,con->sdp,con->sdplen);
    while(true) {
            LOGGERICE("putdescription: %s %d: Local description:\n%s\n",commonLabel.data(),side, con->sdp);
            auto [resbody,code]=ContextHTTPS::getContext().putRequest(hostname,port,description,std::span((const char *)sdpdata.data(),sdpdata.size()));
            if(code==200) {
                if(resbody.size()>= (sizeof(BackDescription )+20)) {
                    const BackDescription *other=reinterpret_cast<const BackDescription *>(resbody.data());
                    LOGGERICE("putdescription %s %d: received Remote in:\n%s\n",commonLabel.data(),side,other->description);
                    if(side==givefirst)
                        juice_set_remote_description(agent, other->description);
                    return true;
                    }
                 else {
                    LOGGERICE("putdescription: %s %d: Remote small body in :\n%s\n",commonLabel.data(),side,(const char *)resbody.data());
                    sleep(1);
                    }
                }
             else {
                LOGGERICE("putdescription: %s %d: Http error\n",commonLabel.data(),side);
                sleep(20);
                }
        }
    }


bool initAgent(juice_agent *agent,int allindex) {
    const passhost_t &host= getBackupHosts()[allindex];
    std::string_view commonLabel=host.getICEname();
    ICEConnect *con=static_cast<ICEConnect *>(connections[allindex]);
    std::string_view hostname=hostnames[con->hostindex];
    LOGGER("initAgent %s allindex=%d side=%d\n",commonLabel.data(),allindex,host.side);
    int32_t firstfailed=getConnectTime(allindex);
    int32_t now=time(nullptr);
    if(!firstfailed)
        setConnectTime(allindex,now);
    else  {
        if((now-firstfailed)>maxconnectionunused) {
            backup->deactivateHost(allindex,true);
            return -1;
            }
        }
    bool side=host.side;
    if(side!=givefirst) {
        if(!waitonDescription(agent,commonLabel,side,hostname)) {
           LOGGERICE("initAgent %s %d: waitonDescription failed\n",commonLabel.data(),side);
            return false;
        }
      if(!stillworking(allindex))
        return false;
      }
    if(!putDescription(allindex,agent, commonLabel, side,hostname)) { 
        LOGGERICE("initAgent %s %d: putDescription failed\n",commonLabel.data(),side);
        return false;
         }
    if(!stillworking(allindex))
        return false;

    std::jthread receive{getAddressesThread,agent,commonLabel,side,hostname};
    LOGGERICE("initAgent %s %d: Before juice_gather_candidates\n",commonLabel.data(),side);
    int ret=juice_gather_candidates(agent);
    if(!stillworking(allindex))
        return false;
    LOGGERICE("initAgent %s %d: After juice_gather_candidates(%p)=%d\n",commonLabel.data(),side,agent,ret);
    return ret==JUICE_ERR_SUCCESS;
  }
