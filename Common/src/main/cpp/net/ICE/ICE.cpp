

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
const std::string_view hostname{"a.juggluco.nl"};
int port{9999};
#ifndef LOGGER
#define LOGGER(...) fprintf(stderr,__VA_ARGS__)
#endif
#define BUFFER_SIZE 4096

#define JUICE_ERR_SUCCESS 0
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

//constexpr const char commonLabel[]{ "IetsGemeenschap2"};

//constexpr const int labelsize=sizeof(Agent_data::label); 
/*
class CreateAgentData {
public:
   CreateAgentData(std::string_view commonLabel,bool side,const char *sdp,int sdplen=-1):buflen(sizeof(Agent_data)+commonLabel.size()+1(sdplen==-1?(sdplen=strlen(sdp)):sdplen)), agentbuf(new(std::align_val_t(alignof(Agent_data))) char[buflen]) {
        agentdata()->side='0'+side;
        memcpy(agentdata()->label,commonLabel.data(),commonLabel.size());
        if(commonLabel.size()<labelsize) {
            memset(agentdata()->label+commonLabel.size(),'X',labelsize-commonLabel.size());
            } 
        memcpy(agentdata()->description,sdp,sdplen);
        };
    ~CreateAgentData() {
        ::operator delete[](agentbuf,std::align_val_t(alignof(Agent_data)));
        }
    Agent_data *agentdata() {
        return reinterpret_cast<Agent_data*>(agentbuf);
        }
    const char *data() const {
        return agentbuf;
        }
     int size() const {
        return buflen;
        }
private:
    int buflen;
    char   *agentbuf;
    };
    */
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

void receivethread(juice_agent *agent,std::string_view commonLabel,bool side) {
   static std::string_view address{"/address"};
   CreateAgentData addressdata(commonLabel,side,"") ;
    LOGGERICE("address %s\n",addressdata.data());
        while(true) {
                LOGARICE("Address: before GET address");
                auto [resbody,code]=ContextHTTPS::getContext().getRequest(hostname,port,address,addressdata.getSpan());
                if(code== 200) {
                    if(resbody.size()>= (sizeof(BackDescription )+20)) {
                        const BackDescription *other=reinterpret_cast<const BackDescription *>(resbody.data());
                        int res=juice_add_remote_candidate(agent, other->description);
                        LOGGERICE("%s %d: Address %s res=%d\n",commonLabel.data(),side,other->description,res);
                        }
                     else {
                        juice_set_remote_gathering_done(agent);
                        LOGGERICE("%s %d: juice_set_remote_gathering_done\n",commonLabel.data(),side);
                        return;
                        }
                    }
                 else {
                    LOGGERICE("%s %d: Address Http error\n",commonLabel.data(),side);
                    sleep(1);
                    }
                }
      LOGGERICE("%s %d: end thread\n",commonLabel.data(),side);
     } 



static void on_candidate1(juice_agent_t *agent, const char *sdp, void *user_ptr) {
   const int allindex=(int)(long)user_ptr;
   const passhost_t &host= getBackupHosts()[allindex];
   static std::string_view address{"/address"};
   CreateAgentData sdpdata(host.getICEname(),host.side,sdp) ;
   /*
   if(gathering_done)  {
        LOGGERICE("%s %d: Candidate: %s, but gathering_done\n",host.getICEname().data(),host.side, sdp);
        return;
        }  */
   auto [resbody,code]=ContextHTTPS::getContext().putRequest(hostname,port,address,std::span((const char *)sdpdata.data(),sdpdata.size()));
   if(code==200) {
            LOGGERICE( "%s %d: OK send Candidate: %s\n",host.getICEname().data(),host.side, sdp);
        }
    else {
          LOGGERICE("%s %d: ERROR Candidate: %s\n",host.getICEname().data(),host.side,  sdp);
        }
   }


// Agent 1: on local candidates gathering done
static void on_gathering_done1(juice_agent_t *agent, void *user_ptr) {
   const int allindex=(int)(long)user_ptr;
   const passhost_t &host= getBackupHosts()[allindex];
    //gathering_done=true;
    LOGGERICE("Gathering done %s %d\n",host.getICEname().data(),host.side);
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

extern void receiverthread(passhost_t *host,const int allindex);
static void on_state_changed1(juice_agent_t *agent, juice_state_t state, void *user_ptr) {
   const int allindex=(int)(long)user_ptr;
    const passhost_t &host= getBackupHosts()[allindex];
    LOGGERICE("%s %d State: %s\n", host.getICEname().data(),host.side,juice_state_to_string(state));
    ICEConnect *con=static_cast<ICEConnect*>(connections[allindex]);
    switch(state) {
        case	JUICE_STATE_GATHERING:
        case	JUICE_STATE_CONNECTING:
            con->notConnected();
            break;
        case JUICE_STATE_CONNECTED: {
            con->setConnected();
            struct DONE {
                static void thread( juice_agent_t *agent, int allindex) {
                    passhost_t &host= getBackupHosts()[allindex];
                    ICEConnect *con=static_cast<ICEConnect*>(connections[allindex]);
                    if(!con) {
                        LOGGERICE("connection[%d]==NULL\n",allindex);
                        return;
                        }
                    CreateAgentData body(host.getICEname(),host.side,con->sdp,con->sdplen);
                    std::string_view done{"/done"sv};
                    auto [resbody,code]=ContextHTTPS::getContext().putRequest(hostname,port,done,std::span((const char *)body.data(),body.size()));
                    if(code==200) {
                        LOGGERICE("%s %d: OK DONE\n",host.getICEname().data(),host.side);
                       }
                     else {
                       LOGGERICE("%s %d: ERROR DONE code=%d\n",host.getICEname().data(),host.side,code);
                         }
                    con->notifyReceive();
                    if(!con->receiving) {
                        LOGARICE("running receiverthread");
                        while(true) {
                            if(!con->isConnected)
                                return;
                            if(con->receiveConnect(&host)) {
                                receiverthread(&host,allindex);
                                con->icedata[1].reStarted();
                                }
                             if(!host.isSender()) {
                                    LOGGERICE("%d DONE thread, make connect\n",host.side);
                                    while(true) {
                                            if(con->connect(&host)) {
                                                if(agent)
                                                        return;
                                                LOGGERICE("side=%d DONE::thread con->connect failed, sleep\n",host.side);
                                                sleep(1);
                                                }
                                             else
                                                 break;
                                            }
                                    }
                               else {
                                 
                                if(con->agent==nullptr)
                                     return;
                                if(!con->isConnected)
                                    return;
                                if(!con->endConnect)
                                    return;

                                 }
                             }
                        }
                    else {
                        LOGARICE("running receiverthread already running");
                        }
                    };
                   };
            std::thread th(DONE::thread,agent,allindex);
            th.detach();
            diagnostics(agent,host.getICEname().data(),host.side);
            }; break;
        case JUICE_STATE_FAILED: {
            passhost_t &host= getBackupHosts()[allindex];
            ICEConnect *con=static_cast<ICEConnect*>(connections[allindex]);
            if(!con) {
                LOGGERICE("connection[%d]==NULL\n",allindex);
                return;
                }
            CreateAgentData body(host.getICEname(),host.side,con->sdp,con->sdplen);
            std::string_view failure{"/failure"sv};
            auto [resbody,code]=ContextHTTPS::getContext().putRequest(hostname,port,failure,std::span((const char *)body.data(),body.size()));
            if(code==200) {
                LOGGERICE("%s %d: OK FAILURE\n",host.getICEname().data(),host.side);
               }
             else {
               LOGGERICE("%s %d: ERROR FAILURE code=%d\n",host.getICEname().data(),host.side,code);
                 }
             };
        case JUICE_STATE_DISCONNECTED: {//Ever used?
            ICEConnect *con= static_cast<ICEConnect *>( connections[allindex]);
            con->notConnected();
            con->sayEndConnection();
            };break;

        default:break;

        };
    LOGARICE("end on_state_changed1");
    }


juice_agent *createAgent(int allindex) {
//    juice_set_log_level(JUICE_LOG_LEVEL_DEBUG);

    juice_turn_server_t turn_server{
        .host="a.juggluco.nl",
        .username="jka",
        .password="Een1Password2",
        .port=3478};
    juice_config_t config1{
        .concurrency_mode=JUICE_CONCURRENCY_MODE_THREAD,
        .stun_server_host = "stun.l.google.com",
        .stun_server_port = 19302,
        .turn_servers=&turn_server,
        .turn_servers_count=1,  
        .cb_state_changed = on_state_changed1,
        .cb_candidate = on_candidate1,
        .cb_gathering_done = on_gathering_done1,
        .cb_recv = on_recv1,
        .user_ptr=(void*)(long)allindex
        };
    return juice_create(&config1);
  };
static std::string_view description="/description";

static bool waitonDescription(juice_agent *agent,std::string_view commonLabel) {
    CreateAgentData sdpdata(commonLabel,1,"");
    LOGGERICE("getdescription %s\n",sdpdata.data());
    while(true) {
        auto [resbody,code]=ContextHTTPS::getContext().getRequest(hostname,port,description,sdpdata.getSpan());
        if(code== 200) {
            if(resbody.size()>= (sizeof(BackDescription )+20)) {
                const BackDescription *other=reinterpret_cast<const BackDescription *>(resbody.data());
                LOGGERICE("getdescription SUCCESS: %s 1: Remote description in:\n%s\n",commonLabel.data(),other->description);
                juice_set_remote_description(agent, other->description);
                return true;
                }
             else {
                LOGGERICE("getdescription failure %s size=%d: getdescription Remote small body in :\n%.*s\n",commonLabel.data(),(int)resbody.size(),(const char *)resbody.data());
                return false;
                }
            }
        LOGGERICE("getdescription failure %s 1: %s returns code=%d\n",commonLabel.data(),sdpdata.data(),code); 
        sleep(0);
        }
//    return false;
    }
static  bool putDescription(int allindex,juice_agent *agent,std::string_view commonLabel,bool side)  {
    ICEConnect *con=static_cast<ICEConnect *>(connections[allindex]);
    if(const int error=juice_get_local_description(agent, con->sdp, JUICE_MAX_SDP_STRING_LEN);JUICE_ERR_SUCCESS!=error) {
        LOGGERICE("%s %d: juice_get_local_description failed: %s (%d)\n",commonLabel.data(),side,juiceErrorString(error),error);
        return  false;
        }
     con->sdplen=strlen(con->sdp);
    LOGGERICE("%s %d: Local description:\n%s\n",commonLabel.data(),side, con->sdp);
    CreateAgentData sdpdata(commonLabel,side,con->sdp,con->sdplen);
    while(true) {
            auto [resbody,code]=ContextHTTPS::getContext().putRequest(hostname,port,description,std::span((const char *)sdpdata.data(),sdpdata.size()));
            if(code==200) {
                if(resbody.size()>= (sizeof(BackDescription )+20)) {
                    const BackDescription *other=reinterpret_cast<const BackDescription *>(resbody.data());
                    LOGGERICE("%s %d: Remote description in:\n%s\n",commonLabel.data(),side,other->description);
                    if(!side)
                        juice_set_remote_description(agent, other->description);
                    return true;
                    }
                 else {
                    LOGGERICE("%s %d: Remote small body in :\n%s\n",commonLabel.data(),side,(const char *)resbody.data());
                    }
                }
             else {
                LOGGERICE("%s %d: Http error\n",commonLabel.data(),side);
                }
          sleep(20);
        }
    }
bool initAgent(juice_agent *agent,int allindex) {
    const passhost_t &host= getBackupHosts()[allindex];
    // Agent 1: Generate local description
    std::string_view commonLabel=host.getICEname();
    bool side=host.side;
    if(side) {
        if(!waitonDescription(agent,commonLabel)) {
            juice_destroy(agent);
            return false;
        }
      }
    if(!putDescription(allindex,agent, commonLabel, side)) { 
        juice_destroy(agent);
        return false;
         }

    std::jthread receive{receivethread,agent,commonLabel,side};
    LOGGERICE("%s %d: Before juice_gather_candidates\n",commonLabel.data(),side);
    juice_gather_candidates(agent);
    return true;
  }
