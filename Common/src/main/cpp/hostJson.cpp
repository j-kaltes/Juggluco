/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2, Libre 3, Dexcom G7/ONE+ and           */
/*      Sibionics GS1Sb sensors.                                                     */
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
/*      Fri Jun 20 17:38:03 CEST 2025                                                 */


#include "config.h"
#if !defined(WEAROS)
#include <string>
#include <algorithm>
#include "datbackup.hpp"
using namespace std::literals;
template <typename OUTTYPE> int getownips(OUTTYPE *outips,int max,bool &haswlan);

template <typename OutputIt>
OutputIt insertbool( OutputIt inserter, std::string_view name,bool value) {
        return std::format_to( inserter,R"(,"{}":{})",name,value);
        }



static std::string escape(std::string_view input) {
    std::string out;
    out.reserve(input.size()+2);
    auto inserter=std::back_inserter(out);
    const char *end=input.end();
    const constexpr std::string_view zoek=R"("\/)";
    for(auto *iter=input.data();;) {
        auto *hit=std::find_first_of(iter,end,std::begin(zoek),std::end(zoek));
        std::copy(iter,hit,inserter);
        if(hit==end)
            return out;
       *inserter++='\\';
       *inserter++=*hit++;
       iter=hit;
       } 
    }


std::string mkbackjson(int pos) {
    struct updatedata &back=*backup->getupdatedata();
    if(pos<0||pos>= back.hostnr) {
        return ""s;
        }
    std::string uit;
    uit.reserve(1024);
    auto inserter=std::back_inserter(uit);
    const passhost_t &host=back.allhosts[pos];
    if(host.ICE) {
          inserter=std::format_to(inserter,R"({{"ICElabel":"{}")",escape(host.getICEname()));
//          inserter=std::format_to( inserter,R"(,"label":"{}")",escape(host.getname()));
//          inserter=std::format_to(inserter,R"({"ICElabel":"{}")",escape(host.getname()));
          inserter=insertbool(inserter, "side",!host.side);
        }
     else {
        if(!host.hashostname()) {
            const int maxi=passhost_t::maxip-host.hasname;
            alignas (namehost) uint8_t buf[maxi*sizeof(namehost)];
            namehost *ips=reinterpret_cast<namehost*>(buf); //skip constructor
            bool haswlan;
            int ipnr=getownips(ips,maxi,haswlan);
            inserter=std::format_to( inserter,R"({{"nr":{},"names":[)",ipnr);
            if(ipnr>0) {
                for(int i=0;;) {
                    const char *start=ips[i].data();
                    inserter=std::format_to( inserter,R"("{}")",start);
                    if(++i>=ipnr) 
                        break;
                    *inserter++=',';
                    }
                }
            if(ipnr||host.getActive()) {
                char detect[]=R"(],"detect":false)";
                inserter=std::copy(std::begin(detect),std::end(detect)-1,inserter);
                }
            else {
                char detect[]=R"(],"detect":true)";
                inserter=std::copy(std::begin(detect),std::end(detect)-1,inserter);
                }
            }
         else {
              char empty[]=R"({"nr":0,"names":[],"detect":true)";
              inserter=std::copy(std::begin(empty),std::end(empty)-1,inserter);
              inserter=insertbool(inserter, "hasname",true);
            }
        inserter=std::format_to( inserter,R"(,"port":{})",(char *)back.port);
         }
    if(host.receivedatafrom()) {
        int sendindex=host.index;
        if(sendindex>=0) {
            updateone &send=back.tosend[sendindex];
            inserter=insertbool(inserter, "nums",!send.sendnums);
            inserter=insertbool(inserter, "scans",!send.sendscans);
            inserter=insertbool(inserter, "stream",!send.sendstream);
            inserter=insertbool(inserter, "notes",!send.sendnotes);
            const bool receives=!(send.sendnums&& send.sendscans&&send.sendstream);
            inserter=insertbool(inserter, "receive",receives);
            }
        else {
            inserter=insertbool(inserter, "nums",true);
            inserter=insertbool(inserter, "scans",true);
            inserter=insertbool(inserter, "stream",true);
            inserter=insertbool(inserter, "notes",true);
             }
         }
     else {
            inserter=insertbool(inserter, "receive",true);
            } 
      if(!host.ICE) { 
        inserter=std::format_to(inserter,R"(,"transport":{})",host.gettransport());
        // side is permanent pair identity; the peer must always receive the
        // opposite side. For ordinary mirrors blereverse is pair-wide and is
        // copied unchanged to the peer. bleclient remains for compatibility
        // with older/Wear-aware readers and therefore stays inverted.
        inserter=insertbool(inserter,"side",!host.side);
        inserter=insertbool(inserter,"bleclient",!host.bleclient);
        if(!host.wearos)
            inserter=insertbool(inserter,"blereverse",host.blereverse);
        LOGGER("getActive=%d getPassive=%d\n",host.getActive(),host.getPassive());
        inserter=insertbool(inserter,"activeonly",!host.getActive()&&host.getPassive());
        inserter=insertbool(inserter,"passiveonly",host.getActive());
        }
    if(host.haspass()) 
        inserter=std::format_to( inserter,R"(,"pass":"{}")", escape(Backup::passback(host.pass).data()));
    if(host.hasname) {
        inserter=std::format_to( inserter,R"(,"label":"{}")",escape(host.getname()));
        }
    else {
        inserter=insertbool(inserter, "testip",true);
        }
   const char mirrorJuggluco[]=R"(} MirrorJuggluco)";
   inserter=std::copy(std::begin(mirrorJuggluco),std::end(mirrorJuggluco)-1,inserter);
    LOGGERN(uit.data(),uit.size());
    LOGGER("size=%d\n",uit.size());
    return uit;
    }


extern void makepass(char *pass,int len);
int makeHomeBackupSender() {
    int pos=-1;
    bool nums=true,scans=true,stream=true;
    bool receiver=false,activeonly=false,passiveonly=true;
    const int passlen=16;
    char passstr[passlen+1];
    makepass(passstr,passlen);
    passstr[passlen]='\0';
    uint32_t  starttime=0L;
    bool testip=false,hashostname=false;
    constexpr const int lastchar=9;
    char label[lastchar+1];
    makepass(label,lastchar);
    label[lastchar]='\0';
    constexpr const bool detect=true;
     jint res=backup->changehost(pos,nullptr,nullptr,0,detect,std::string_view(nullptr,0),nums,stream,scans,false,receiver,activeonly,std::string_view(passstr,passlen),starttime,passiveonly,label,testip,true,hashostname,passhost_t::transport_automatic,false);
     if(res>=0) {
         auto &host=backup->getupdatedata()->allhosts[res];
         host.side=true;       // r2: sends Scans, offers
         host.bleclient=false;
         host.bleunproven=true; // new pair may learn the opposite role once
         }
     return res;
     }
int makeHomeBackupReceiver() {
    int pos=-1;
    bool nums=false,scans=false,stream=false;
    bool receiver=true,activeonly=false,passiveonly=true;
    const int passlen=16;
    char passstr[passlen+1];
    makepass(passstr,passlen);
    passstr[passlen]='\0';
    uint32_t  starttime=0L;
    bool testip=false,hashostname=false;
    constexpr const int lastchar=9;
    char label[lastchar+1];
    makepass(label,lastchar);
    label[lastchar]='\0';
    constexpr const bool detect=true;
     // One-way phone mirrors use a deterministic GATT role: the receive-only
     // side scans/connects, and the send-only side advertises/accepts. mkbackjson
     // reverses this bit for the QR peer together with the data direction.
     jint res=backup->changehost(pos,nullptr,nullptr,0,detect,std::string_view(nullptr,0),nums,stream,scans,false,receiver,activeonly,std::string_view(passstr,passlen),starttime,passiveonly,label,testip,true,hashostname,passhost_t::transport_automatic,true);
     if(res>=0) {
         auto &host=backup->getupdatedata()->allhosts[res];
         host.side=false;      // r1: no Scans, connects
         host.bleclient=true;
         host.bleunproven=true; // new pair may learn the opposite role once
         }
     return res;
     }


int makeICEsender() {
    const int passlen=16;
    char passstr[passlen+1];
    makepass(passstr,passlen);
    char label[17]="ICE";
    makepass(label+3,13);
    label[16]='\0';
    char ICElabel[33];
    makepass(ICElabel,32);
    ICElabel[32]='\0';
    int pos=-1;
    bool nums=true,scans=true,stream=true;
    bool receive=false;
    uint32_t  starttime=0L;
    bool side=0;
    return backup->changeICEhost(ICElabel,pos,nums,stream,scans,receive,string_view(passstr,passlen), starttime,label,side,true);
    }
int makeICEreceiver() {
    const int passlen=16;
    char passstr[passlen+1];
    makepass(passstr,passlen);
    char label[17]="ICE";
    makepass(label+3,13);
    label[16]='\0';
    char ICElabel[33];
    makepass(ICElabel,32);
    ICElabel[32]='\0';
    int pos=-1;
    bool nums=false,scans=false,stream=false;
    bool receive=true;
    uint32_t  starttime=0L;
    bool side=1;
    return backup->changeICEhost(ICElabel,pos,nums,stream,scans,receive,string_view(passstr,passlen), starttime,label,side,true);
    }
#endif

#include "net/makerandom.hpp"
void makepass(char *pass,int len) {
             constexpr const auto mkchar=[](uint8_t get) { return get%95+32; };
             uint8_t ran[len];
             makerandom(ran,len);
             for(int i=0;i<len;i++) {
                pass[i]=mkchar(ran[i]);
                }
          }
