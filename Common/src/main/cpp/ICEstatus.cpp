#include <stdio.h>
#include <string.h>
#include <string_view>
#include <memory>
#include "deleter.hpp"
#include "libjuice/include/juice/juice.h"
#include "datbackup.hpp"
#include "mirrorstatus.hpp"
#include "net/netstuff.hpp"
#include "net/ICE/ICEConnect.hpp"

extern mirrorstatus_t mirrorstatus[maxallhosts];
template <class T, size_t N>
inline static constexpr void addar(char *&uitptr,const T (&array)[N]) {
        constexpr const int len=N-1;
        memcpy(uitptr,array,len);
        uitptr+=len;
        }
inline void    addstrview(char *&uitptr,const std::string_view indata) {
    memcpy(uitptr,indata.data(),indata.size());
    uitptr+=indata.size();
    }
extern std::unique_ptr<const char[],deleter> ICEstatus(int allindex);

#define  phasesel(x,str) case x: {\
        char name[]=str;\
        memcpy(buf,name,sizeof(name)-1);\
        return sizeof(name)-1;\
        };break;
int addphase( enum Phase_t phase,char *buf) {
        switch(phase) {
            phasesel(Start,"Start");
            phasesel(GetDescription,"Get description");
            phasesel(PutDescription,"Put description");
            phasesel(GatherCandidates,"Gather candidates");
            phasesel(FailedInitAgent, "InitAgent failed");
            phasesel(NewConnection,"New Connection");
            phasesel(SameConnection, "Same Connection");
        }
   };
std::unique_ptr<const char[],deleter> ICEstatus(int allindex) {
         ICEConnect *con=static_cast<ICEConnect *>(connections[allindex]);
         if(!con)  {
                constexpr const char errormessage[]=R"(<h1>System ERROR, restart Juggluco</h1>)";
        return std::unique_ptr<const char[],deleter>(errormessage,deleter(errormessage));
                }
        passhost_t &host= getBackupHosts()[allindex];
        mirrorstatus_t &status=mirrorstatus[allindex];
    bool sendnums=false;
        bool sendstream=false;
        bool sendscans=false;
    const bool receives=host.receivefrom&2;
    if(host.index>=0) {
        updateone &send=backup->getupdatedata()->tosend[host.index];
        sendnums=send.sendnums;
        sendstream=send.sendstream;
        sendscans=send.sendscans;
        }
    const int maxbuf=1024;
    char *buf=new(nothrow) char[maxbuf];
        if(!buf)  {
         constexpr const char errormessage[]=R"(<h1>Error</h1>)";
         return std::unique_ptr<const char[],deleter>((char *)errormessage,deleter(errormessage));
             }

    static char format[]=R"(<h1>ICE Connection %d: %s</h1><p><b>ICElabel</b>: "%s" <b>Side</b>=%d<br><b>Send to</b>: %s%s%s <b>Running</b>=%s <b>Locked</b>=%s  %s<br><b>Receive from</b>: %s    %d wait for commands: %s    <b>interpret</b>: %s %s</p><p><b>Phase</b>: )";
        char *bufptr=buf+snprintf(buf,maxbuf,format,allindex,host.getnameif(),host.getICEname().data(),host.side,sendnums?"Amounts ":"",sendscans?"Scans ":"",sendstream?"Stream ":"",boolstr[status.sender.running],boolstr[status.sender.locked],con->icedata[0].shutdown?"shutdown":"",
        boolstr[receives],status.receive.tid,boolstr[status.receive.ingetcom()],boolstr[status.receive.ininterpret],con->icedata[1].shutdown?"shutdown":"");
    juice_agent *agent=con->agent.load();
    char remoteAddr[JUICE_MAX_ADDRESS_STRING_LEN+10];
    bufptr+=addphase(con->phase,bufptr);
    if(agent) {
         addar(bufptr,"<br><b>Local:</b> ");
        if(int res=juice_get_selected_addresses_inc_type(agent, bufptr, JUICE_MAX_ADDRESS_STRING_LEN, remoteAddr, JUICE_MAX_ADDRESS_STRING_LEN);res == 0) {
            bufptr+=strlen(bufptr);
            addar(bufptr,"<br><b>Remote:</b> ");
            addstrview(bufptr,remoteAddr);
          }
      }
   addar(bufptr,"<br><b>ICE State:</b> ");
   auto statename=juice_state_to_string(con->state);
   addstrview(bufptr,statename);
   if(con->endConnect) {
      addar(bufptr,"<br>End connection");
     }
   if(con->isConnected) {
       addar(bufptr,"<br>Connected");
       }
   if(con->connectTime) {
       addar(bufptr,"<br>");
       ctime_r(&con->connectTime,bufptr);
       bufptr+=26;
       }
   else
       *bufptr++='\0';
/*
 ICEConnect:
bool wakeReceiver=false;
std::mutex receiveThreadMutex;
std::condition_variable receiveThreadCon;
std::atomic_flag startSending{};
std::atomic_flag startDone{};
bool start_ack;
bool other_started;
std::atomic_flag initrunning{};
ICE_data
 bool shutdown=false;
    bool sendShutdown=false;
    bool  sendStop=false;
        bool askedData=false;
    std::mutex  receiveMutex;
    std::condition_variable receiveCond;
    bool sentAck=false;
    bool lastAcked=false;
    std::mutex  sendMutex;
    std::condition_variable sendCond;
    std::mutex  doSendMutex;
    std::condition_variable doSendCond;
    bool doSend=false;
    */



   LOGGER("ICEstatus size=%d\n",bufptr-buf);
   return std::unique_ptr<const char[],deleter> (buf,deleter(nullptr));
   }
