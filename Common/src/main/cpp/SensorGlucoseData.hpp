/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2 and 3 sensors.                         */
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
/*      Fri Jan 27 12:35:09 CET 2023                                                 */


//#ifndef GLUCOSEHISTORY_H
//#define GLUCOSEHISTORY_H
#pragma once
//#include <filesystem>

#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <fstream>
#include <stdlib.h>
#include <span>
#include <math.h>
#include <time.h>

#include <stdint.h>
#include <assert.h>
#include <limits>
#include <mutex>
#include <algorithm>
#include <unistd.h>
#include <climits>

//#include "myArray.hpp"
/*
inline int getpagesize(void) {
  return sysconf(_SC_PAGESIZE);
} */
#include "config.h"

#include "inout.hpp"

#include "net/backup.hpp"
#include "net/passhost.hpp"
#include "nfcdata.hpp"
//namespace fs = std::filesystem;
#include "gltype.hpp"
#include "destruct.hpp"
//#include "datbackup.hpp"
#include "maxsendtohost.h"
#include "settings/settings.hpp"
#include "timevalues.h"
#include "calibrate/CaliPara.hpp"
inline  constexpr const int maxdexcount=3025 ;
inline constexpr const int youngsensorsecs=2*60*60;
inline    constexpr const char rawstream[]="rawstream.dat";
#include <string_view>
extern std::string_view globalbasedir;
//string basedir(FILEDIR);

extern int writeStartime(crypt_t *pass, const int sock, const int sensorindex); 
    extern int getgetsendnr();

constexpr const int maxcaliNr=50;
constexpr int maxdays=46;

constexpr const int maxdaysDex=12;

constexpr const int maxdaysAccu=15;

constexpr const int stdMaxDaysSI=24;
constexpr const int maxdaysSI=

#ifdef TESTLONGSI
maxdays;
#else
stdMaxDaysSI;
#endif
constexpr int maxminutes=maxdays*24*60;
constexpr const int maxSIhours=
#ifdef TESTLONGSI
maxdaysSI*60*60*24;
#else
572;
#endif
/*constexpr const int maxSIhours=
#ifndef NOLOG
maxdaysSI*24
#else
572
#endif
; */

#ifdef SKIPTRIEDOFTEN
typedef  std::array<char,18> address_t;
#endif
struct ScanData {uint32_t t;int32_t id;int32_t g;int32_t tr;float ch;
 uint16_t getmgdL() const { return g;};
 float getmmolL() const { return g/convfactordL;};
 uint16_t getsputnik() const { return g*10;};
float getchange() const {
    return ch;
    }
uint32_t gettime() const {
    return t;
    };
 uint32_t getid() const {return id;};
bool valid(int pos=1) const {
    if(pos&&!t) {
        ScanData *ht=const_cast<ScanData*>(this);
        const ScanData *prev=ht-1;
        ht->t=prev->t;
        return false;
        }
   // return g&&g>38&&g<502&&id>=0&&id<maxminutes&&t>1598911200u&&t<2145909600u;
    return g&&g<552&&id>=0&&id<maxminutes&&t>1598911200u&&t<2145909600u;
    }
bool current(int pos=1) const {
    return valid(pos)&&!isnan(ch);
    }
float inappunit() const {
       return ::gconvert(g*10);
       }

} ;

struct CurData {
    const ScanData *data;
    const int startpos,endpos;
    CurData(const ScanData *data,const ScanData *startdat,const ScanData *enddat): data(data),startpos(startdat-data),endpos(enddat-data) {        };
    const ScanData *startall() const {
        return data;
    }
    const ScanData *begin() const {
        return data+startpos;
    }
    const ScanData *end() const {
        return data+endpos;
    }

};
struct Glucose {
    uint32_t time;
    uint16_t id;
    uint16_t glu[];
    uint16_t getraw() const { return glu[0];};
    uint16_t getsputnik() const { return glu[1];};
    uint16_t getmgdL() const { return glu[1]/10;};
    uint32_t gettime() const {return time;};
    uint32_t getid() const {return id;};
    float inappunit() const {
       return ::gconvert(getsputnik());
        }
    bool isStreamed() const {
        return glu[2]&0x4000;
        }
    void setStreamed()  {
        glu[2]|=0x4000;
        }
        /*
    bool isLibreSend() const {
        return glu[2]&0x8000;
        }
    void setLibreSend()  {
        glu[2]|=0x8000;
        }
    void unSetLibreSend() {
        glu[2]&=~0x8000;
        }
        */
    bool valid() const {
        return glu[1]&&glu[1]>380&&glu[1]<5020&&id>=0&&time>1598911200u&&time<2145909600u;
        }
};

class SensorGlucoseData {
bool haserror=false;
string sensordir;
//inline static  const string basedir{FILEDIR};
public:
inline static const int blocksize=sysconf(_SC_PAGESIZE);
private:
static constexpr uint16_t defaultinterval=15*60;

struct updatestate {
    uint32_t scanstart;uint32_t histstart;uint32_t streamstart; uint32_t rawstreamstart;
    bool sendbluetoothOn:1;
    bool sendstreaming:1;
    bool changedstreamstart:1;
    bool changedhistorystart:1;
    bool sendKAuth:1;
    bool sendhiststart:1;
   bool siScan:1;
   bool siStream:1;
    };
static inline constexpr const int deviceaddresslen=18;
struct Info {
uint32_t starttime;
uint32_t lastscantime;
int32_t starthistory;
int32_t endhistory;  //In one past last pos.
uint32_t scancount;
uint16_t startid;
uint16_t interval;
uint16_t pollstart;
uint8_t dupl:7;
bool dexcom:1;
uint8_t days:7;
bool sibionics:1;
union {
  uint32_t pin;
  struct {
      uint16_t warmup;
      uint16_t wearduration;
    
  };};
uint16_t lastLifeCountReceived;
uint16_t lastHistoricLifeCountReceivedPos;
union {
struct { 
   int len;
   signed char data[8];
    } ident;
struct {
      uint16_t wearduration2;
      uint8_t warmup2;
   };
   };
union {
   struct { //Libre 2
      struct { 
         int len;
         signed char data[6];
         int16_t extra;
         } info;
      struct {
         uint32_t bluestart;     
         union {
            struct { int len; //Libre 2 EU
               signed char data[6];
               uint8_t sensorgenDONTUSE;
               bool reserved2;
               } blueinfo;
            uint8_t streamingAuthenticationData[12]; //Libre 2 US
            };
         };
         };
   struct { //Sibionics
       char siBlueToothNum[9];
       bool notchinese:1;
       uint8_t siType:2;  //0: EU, 1:HemaToxic
       char siBetween:4;
       bool reset:1;
       uint8_t siDeviceNamelen;
       char8_t siToken;
       char8_t siDeviceName[16];
    };
  struct { //Dexcom g7
      std::array<uint8_t,16> sharedKey;  
      char DexDeviceName[12];
      };
  };
uint32_t pollcount;
double pollinterval; 
uint32_t lockcount;
int8_t streamingIsEnabled;
int8_t patchState;
uint16_t reserved4:14;
uint16_t accuChek:1;
bool auth12:1;
char deviceaddress[deviceaddresslen];
uint16_t libreviewScan;
uint8_t authlendontuse;

uint8_t reserved:2;
bool redoAll:1;

bool streamHistoryDONtuse:1;

bool realHistory:1;
bool newscan:1;

bool sendsensorstart:1;
bool libreviewsendall:1;
uint16_t libreviewnotsend:14;
bool prunedstream:1;
bool putsensor:1;
updatestate update[std::max(maxsendtohost,8)];
union {
   uint8_t kAuth[149];
   struct {
      uint32_t siIdlen;
      char8_t siId[68];
      char8_t reservedS[77];
      } __attribute__ ((packed)); //sizeof=4 element and non multiple of 4
   }; 
bool haskAuth;
uint16_t nightiter;
uint32_t libreCurrentIter;
int16_t scanoff;
uint16_t endStreamhistory; 
uint16_t startedwithStreamhistory; 
uint16_t libreviewnotsendHistory;
union {
    struct {
       bool oldsendLibre[15*24*4];
       uint16_t oldhealthconnectiter;
       };
    struct {
       bool sendLibre[20*24*4];
       uint16_t healthconnectiter;
       uint16_t broadcastfrom;
       };
//    uint16_t oldbroadcastfrom;
 //   uint32_t oldlibreStarttime;
    }; //end union
uint32_t libreStarttime;
uint32_t reserved3;
CaliPara caliPara[maxcaliNr];
uint32_t caliNr;
uint32_t caliUpdated[std::max(maxsendtohost,8)];

uint32_t lastCalibrated() const {
        if(!caliNr)
                return 0;
        return caliPara[caliNr-1].time;
        }
int     caliPosAfter(const uint32_t time) {
    const uint32_t nr=caliNr;
    if(!nr)  {
        LOGGER("caliPosAfter(%u) no calibrators\n",time);
        return 0;
        }
    const CaliPara *first = caliPara;
    extern const CaliPara *getCaliBefore(const CaliPara *first,const CaliPara *end,uint32_t time);
    if(const CaliPara *cali=getCaliBefore( first,first+nr,time)) {
        int pos=cali-first+1;
        LOGGER("caliPosAfter(%u)=%d\n",time,pos);
        return pos;
        }
    LOGGER("caliPosAfter(%u) no calibrator before time\n",time);
    return 0;
    }
void updateCaliTime(int ind, const uint32_t time) {
        caliUpdated[ind]=caliPosAfter(time);
        }

void updateCaliUpdated(uint32_t val) {
    const int maxind=getgetsendnr();
    if(maxind>0) {
        for(int i=0;i<maxind;++i) {
            auto &el=caliUpdated[i];
            if(val<el)
                el=val;
            }
         }
    }



int addCali(uint32_t tim, float weight,double a, double b) {
        if(caliNr<maxcaliNr) {
            LOGGER("%d addCali(%u,%f,%f)\n",caliNr,tim,a,b);
            int it=caliNr-1;
            for(;it>=0&&caliPara[it].time>=tim;--it) 
                ;
            ++it; 
            if(it<caliNr) {
               if(caliPara[it].time!=tim) {
                    memmove(caliPara+it+1,caliPara+it,(caliNr-it)*sizeof(caliPara[0]));
                    ++caliNr;
                    }
                else {
                   int it2=it+1;
                   for(;it2<caliNr&&caliPara[it2].time==tim;++it2)
                        ;
                   --it2;
                   if(it2>it) { //Remove duplicates
                        int len=it2-it;
                        memmove(caliPara+it+1,caliPara+it2+1,len*sizeof(caliPara[0]));
                        caliNr-=len;
                        }
                  }
                updateCaliUpdated(it);
                }
             else {
                ++caliNr;
                }

            caliPara[it]={tim,weight,a,b};
            return it;
            }
        else {
            LOGGER("Can't add %d==maxcaliNr addCali(%u,%f,%f)\n",caliNr,tim,a,b);
            return -1;
            }
        }

bool removeCaliPos(int pos) {
        if(pos>=caliNr) {
                LOGGER("removeCaliPos(%d) >= caliNr(%d)\n",pos,caliNr);
                return false;
                }
        --caliNr;
        if(pos<caliNr) {
                LOGGER("removeCaliPos(%d) move new CaliNr=%d\n",pos,caliNr);
                memmove(caliPara+pos,caliPara+pos+1,sizeof(caliPara[0])*(caliNr-pos));
                updateCaliUpdated(pos);
                }
          else {
                LOGGER("removeCaliPos(%d) last\n",pos);
                }
        return true;
        }
int  removeCali(uint32_t tim) {
     CaliPara *last=caliPara+caliNr-1;
     if(last->time==tim) {
        LOGGER("removeCali(%u) last\n",tim);
        do {
            --caliNr;
            } while(caliNr>0&&caliPara[caliNr-1].time==tim);
        return caliNr;
        }
     for(CaliPara *iter=last-1;iter>=caliPara;--iter) {
        if(iter->time>tim)
                continue;
        if(iter->time==tim) {
                CaliPara *first=iter-1;
                while(first>=caliPara&&first->time==tim)
                        --first;
                const int rmlen=iter-first;
                ++first;
                const int removedpos= first-caliPara;
                LOGGER("removeCali(%u) pos %d len=%d\n",tim,removedpos,rmlen);
                memmove(first,first+rmlen,reinterpret_cast<uint8_t*>(last)-reinterpret_cast<uint8_t*>(iter));
                caliNr-=rmlen;
                updateCaliUpdated(removedpos);
                return removedpos;
                }
          LOGGER("removeCali(%u) not found\n",tim);
          return iter-caliPara+1;
        }
      return 0;
      }


void clearLibreSendEnd(int start) {
    const int len=(int)sizeof(sendLibre)-start*sizeof(sendLibre[0]);
    LOGGER("clearLibreSendEnd(%d) sizeof(sendLibre)=%d len=%d\n",start,sizeof(sendLibre),len);
    if(len>0)
        bzero(sendLibre+start,len);
    }
void clearLibreSendAll() {
    bzero(sendLibre,sizeof(sendLibre));
    }
bool isLibreSend(int pos ) const {
    return sendLibre[pos];
    }
void setLibreSend(int pos)  {
    sendLibre[pos]=true;
    }
void unSetLibreSend(int pos) {
    sendLibre[pos]=false;
    }

bool infowrong() const {
    if(days<10||days>maxdays)
        return true;    
    if(starttime<1583013600)
        return true;
    if(!dupl)
        return true;
    return false;
    }


void setauth(const uint8_t *authin,int len) {
    int newlen=std::min(len,12);
    memcpy(streamingAuthenticationData,authin,newlen);
    if(newlen==12)
        auth12=true;
    else
        auth12=false;
    }
std::span<const uint8_t> getauth() const {
    return {streamingAuthenticationData,static_cast<size_t>((auth12?12:10))};
    }
} ;
//pathconcat sensordir;
//pathconcat scanfile;
Mmap<unsigned char>  meminfo;
int getelsize() const {
    return sizeof(uint32_t)+(getinfo()->dupl+1)*sizeof(uint16_t);
    }
Mmap<uint8_t> historydata;

const size_t scansize;
Mmap<ScanData> scans,polls;
Mmap<std::array<uint16_t,16>> trends;
//static constexpr int getinfo()->dupl=3,days=15;
const int historybytes(int perhour=4)  {
    if(!getinfo()) {
        LOGGER("%s historybytes no getinfo\n",shortsensorname()->data());
        haserror=true;
        return 0;
        }
    const auto elsize=getelsize();
    LOGAR("historybytes");
   if(isSibionics()) return 4*elsize;
//   if(isDexcom()) return 4*elsize;
    const auto days=getinfo()->days;
    if(elsize<10||elsize>20||days<10||days>maxdays) {
        LOGGER("%s: historybytes error elsize=%d days=%d\n",shortsensorname()->data(),elsize,days);
        haserror=true;
        return 0;
        }
    int res= elsize*days*24*perhour;
    LOGGER("historybytes %d\n",res);
    return res;
    }
    /*
inline int getstartpos() {
    return getinfo()->startpos;
    }
inline void setstartpos(int pos) {
    getinfo()->startpos=pos;
    }
    */
public:
bool resetdevice=false;

std::mutex mutex;
int getsensorgen() const {
//    return getinfo()->blueinfo.sensorgen;
    const data_t *info=getpatchinfo();
   if(!info->length())
      return 0;
    return getgeneration((const char *)info->data());
    }
bool useLibre2rootcheck() const {
        const int gen=getsensorgen();
        if(gen>2)
                return false;
        return gen==2||getinfo()->wearduration>20160;
        }
void setsensorgen() {
/*    extern void setlastGen(int gen);
    const data_t *info=getpatchinfo();
    getinfo()->blueinfo.sensorgen=getgeneration((const char *)info->data()); */

    }
int getSensorgen2() const {
        if(isDexcom())
                return 0x40;
        if(isSibionics())
                return 0x10;
        if(isAccuChek())
                return 0x20;
        if(getinfo()->interval==interval5)
                return 3;
         return 2;
        }
char *deviceaddress() {
    return getinfo()->deviceaddress;
    }
#ifdef JUGGLUCO_APP
#ifdef SKIPTRIEDOFTEN
std::vector<address_t> usedAddresses;
uint32_t usedAddressesTime;
uint32_t lastNewMatch=0;
#endif
#endif
const char *deviceaddress() const {
    return getinfo()->deviceaddress;
    }
const int glucosebytes()const {
    return getelsize();
    }
bool waiting=true;
const int32_t maxpos() const {
  if(isSibionics()) {
       return maxSIhours*perhour();
       }
  return getinfo()->days*24*perhour();
  }

 int streamperhour() const {
      if(isAccuChek()||isDexcom())
        return 12;
    else return 60;
     }
int32_t maxstreampos() const {
  if(haserror)
        return 24*24*60;
  if(isSibionics1()) {
       return maxSIhours*streamperhour();
      }
   auto days=getinfo()->days;
   if(days<15&&!isDexcom())
      days=15;
   return days*24*streamperhour();
   }
int streamingIsEnabled() const{
    return getinfo()->streamingIsEnabled;
    }
void setbluetoothOn(int val) {
    getinfo()->streamingIsEnabled=val;
    }
uint32_t getfirsttime() const {
    if(isLibre()) {
       uint32_t locfirstpos=getstarthistory()+1;
       for(int pos=locfirstpos,end=std::min(getAllendhistory(),maxpos());pos<end;pos++) {
           int16_t id =getid(pos);
           uint32_t tim=timeatpos(pos);
           if(id&&tim)
               return tim;
           }
       LOGGER("%s: no history\n",shortsensorname()->data());
       }

    return  firstpolltime();
    }
/*void checkhistory(std::ostream &os) {
    int interval=getinterval();
    int minhistinterval=interval/60;
    uint32_t locfirstpos=getstarthistory()+1;
    int som=0;
    for(int pos=locfirstpos,end=std::min(getAllendhistory(),maxpos()),loclastpos=locfirstpos;pos<end;pos++) {
        int16_t id =getid(pos);
        uint32_t nu=timeatpos(pos);
        if(id&&nu) {
            auto prevtime=timeatpos(loclastpos);
            if(prevtime) {
                if(int64_t times=pos-loclastpos;times!=0) {
                    uint32_t totdif=nu-(int64_t)prevtime- times*interval;
                    int dif= totdif/times;
                    som+=totdif;
                    if(abs(dif)>60) {
                        os<<dif<<" difference at pos "<<pos;
                        if(times>1)
                            os<<times<<" times"<<std::endl;
                        else
                            os<<std::endl;
                        }
                    }
                }
            if(id!=pos*minhistinterval)
                os<<"pos="<<pos<<"id="<<id<<"!="<<pos*minhistinterval<<std::endl;
            loclastpos    =pos;
            }
        }

    os<<"total difference: "<<som<<" seconds "<<som/60.0<<" minutes"<<std::endl;

    }
   */
int getinterval() const {
    if(getinfo()->interval)
        return getinfo()->interval;
    return defaultinterval;
    }
int getmininterval() const {
    return  getinterval()/60;
    }

const int perhour() const {
    return 60/getmininterval();
    }
int getweardurationMIN() const {
   const int wear=(isLibre2()||isDexcom()||isAccuChek())?getinfo()->wearduration:getinfo()->wearduration2;
   if(wear)
         return wear;
   return 14*24*60;
   } 
int getweardurationSEC() const {
   return getweardurationMIN() *60;
   }

int getWarmupMIN() const {
   const int warmup=(isLibre2()||isAccuChek()||isDexcom())?getinfo()->warmup:getinfo()->warmup2;
   if(warmup)
         return warmup;
   return 60;
   } 
int getWarmupSEC() const {
    return getWarmupMIN()*60;
   } 
uint32_t officialendtime() const {
    return getstarttime()+getweardurationSEC();
    }
int expectedWearDuration() const {
    if(isSibionics()) {
        if(getinfo()->notchinese) {
            switch(siSubtype()) {
                case 1: return 1966080;
                default: return 1972800;
                }

            };
        return (maxSIhours*60-19)*60;
        }
    if(isLibre3()||isAccuChek())
        return getweardurationSEC();
    return getweardurationSEC()+12*60*60;
    }
uint32_t expectedEndTime() const {
     return getstarttime()+ expectedWearDuration();
     }
    
uint32_t getmaxtime() const {
#if  1
   const int hours= isSibionics1()?maxSIhours:((isAccuChek()?maxdaysAccu:getinfo()->days)*24);
#else
   const int hours= isSibionics1()?maxSIhours:((isAccuChek()?maxdaysAccu:getinfo()->days+1)*24);
#endif
    return hours*60*60+getstarttime();
    }
uint32_t getstarttime() const {
    return getinfo()->starttime;
    }
int32_t getstarthistory() const {
    return getinfo()->starthistory;
    }
inline void setstarthistory(int pos)  {
    LOGGER("setstarthistory(%d)\n",pos);
    if(pos!=getinfo()->starthistory) {
        getinfo()->starthistory=pos;
        setsendhiststart();
        }
    }
    /*
int32_t getendhistory() const {
    return getinfo()->endhistory;
    } */
int32_t getAllendhistory() const {
    return std::max(getStreamendhistory(), getScanendhistory());
    }
int getStreamendhistory() const { 
    return getinfo()->endStreamhistory; 
    }
int getScanendhistory() const { 
    return getinfo()->endhistory; 
    }
uint32_t getlasttime() const {
    return timeatpos(getAllendhistory()-1);
    }
inline int getlastpos(int pos) {
    if(pos<getinfo()->starthistory)
        getinfo()->starthistory=pos;
    return getinfo()->endhistory;
    }
inline void setendhistory(int pos)  {
    getinfo()->endhistory=pos;
    }
inline void setendStreamhistory(int pos)  {
    getinfo()->endStreamhistory=pos;
    }
inline void setendScanhistory(int pos)  {
    getinfo()->endhistory=pos;
    }

inline const uint8_t *elstart(int pos) const {
    return &historydata[pos*getelsize()];
    }
inline uint8_t *elstart(int pos)  {
    return &historydata[pos*getelsize()];
    }

Glucose * getglucose(int pos)  {
    return reinterpret_cast<Glucose *>( &historydata[pos*getelsize()]);
    };

const Glucose * getglucose(int pos) const {
    return reinterpret_cast<const Glucose *>( &historydata[pos*getelsize()]);
    };

template<int N>
void saveel(int pos,time_t tin,uint16_t id, uint16_t const (&glu)[N]) {
    int idpos=int(round(id/(double)getmininterval()));
    if(pos!=idpos) {
        LOGGER("GLU: saveel %d!=%d\n",pos,idpos);
        return;
        }
    Glucose *item=getglucose(pos);
    uint32_t newtime=static_cast<uint32_t>(tin);
    *item={.time=newtime,.id=id};
    memcpy(item->glu,glu,N*sizeof(*glu));
    }



bool savenewhistory(int pos, int lifeCount, uint16_t mgL) {
/*
    if(!timelastcurrent) {
         return false;
         } */
    Glucose *item=getglucose(pos);
    if(item->id==lifeCount&&item->glu[1]==mgL) {

#ifndef NOLOG
        const auto wastime=lifeCount2time(lifeCount);
        LOGGER("savenewhistory(%d,%d,%.1f) known %s",pos,lifeCount,mgL/convfactor,ctime(&wastime));
    #endif
        return false;
        }
    const auto wastime=lifeCount2time(lifeCount);
    item->time=wastime;
    item->id=lifeCount;
    item->glu[1]=mgL;
    LOGGER("savenewhistory(%d,%d,%.1f) %s",pos,lifeCount,mgL/convfactor,ctime(&wastime));
//        saveel(pos,wastime,lifeCount, {0,mgL});
    return true;
    }
#ifdef DEXCOM
void saveDexFuture(int frompos,uint32_t fromtime, int mgdL) { //20 minutes later
    if(mgdL>401||mgdL<39) {
        LOGGER("saveDexFuture %d invalid %d mg/dL\n",frompos,mgdL);
        return;
        }
   constexpr const int minfuture=10;
    const int pos=frompos+minfuture/5;
    #ifdef NOLOG
    const uint32_t
    #else
    const time_t
    #endif
    wastime=fromtime+minfuture*60;

    Glucose *item=getglucose(pos);
    item->time=wastime;
    item->id=pos;
    item->glu[1]=mgdL*10;
    setendhistory(pos+1);

    LOGGER("saveDexFuture %d %.1f  %s",pos,(mgdL*10)/convfactor,ctime(&wastime));
    }
#endif

uint32_t timeatpos(int pos)  const {
    const Glucose* gl= getglucose(pos);
    if(!gl->valid())
        return 0;
    return gl->gettime();;
//    return *((const uint32_t *)elstart(pos));
    }
uint32_t rawglucose(int pos) const {
    return *((const uint16_t *)(elstart(pos)+sizeof(uint32_t)+sizeof(uint16_t)));
    }
uint32_t sputnikglucose(int pos)const  {
    return *((const uint16_t *)(elstart(pos)+sizeof(uint32_t)+2*sizeof(uint16_t)));
    }
//    uint8_t *memstart=&historydata[pos*getelsize()];
    
int gettimepos(uint32_t time)const {
    return ( int)round(((double)time-getinfo()->starttime)/getinfo()->interval);
    }
int getfirstnotbeforetime(uint32_t time)const {
    const int pos=gettimepos(time);
    int newpos=pos;
    const int start=getstarthistory(); 
    const int end=getAllendhistory() ;
    if(newpos<start) {
        newpos=start;
        }
    else  {
        if(newpos>end)  {
            if(end<=0) 
                newpos=0;
            else
                newpos=end-1;
            }
        }
    LOGGER("getfirstnotbeforetime start=%d end=%d newpos=%d\n",start,end,newpos);
    for(;newpos>start;--newpos) {
        const Glucose* gl= getglucose(newpos);
        if(!gl->valid())
            continue;
        if(gl->gettime()<time)
            break;
        }
    for(;newpos<end;++newpos) {
        const Glucose* gl= getglucose(newpos);
        if(!gl->valid())
            continue;
        if(gl->gettime()>=time)
            break;
        }
    #ifndef NOLOG
    LOGGER("getfirstnotbeforetime newpos=%d\n",newpos);
    time_t tim=time;
    time_t gltime;
    const Glucose*gl;
    if(newpos<end&&(gl= getglucose(newpos))&&gl->valid())
        gltime=gl->gettime();
    else
        gltime=0;
    char glbuf[27],buf[27];
    LOGGER("getfirstnotbeforetime pos=%d newpos=%d time=%.24s lastbefore %s",pos,newpos,ctime_r(&gltime,glbuf),ctime_r(&tim,buf));
    #endif
    return newpos;
    }

uint16_t getatpos(int pos,int field) const {
    const uint16_t *d=(const uint16_t *)(historydata+pos*getelsize());
    return d[field];
    }
uint16_t getid(int pos) const {
    if(pos<=0) {
        return 0;
        }
    return getatpos(pos,2);    
    }
static constexpr int glinel=6;
uint16_t &glucose(int pos,int kind) {
    uint16_t *glus=(uint16_t *)(historydata+pos*getelsize()+glinel);
    return glus[kind];
    }

static float tommolL(uint16_t raw) {
    return (float)raw/convfactor;
    }
void dumpdata(std::ostream &os,uint32_t locfirstpos,uint32_t loclastpos) {
       os<< std::fixed<< std::setprecision(1);
    for(auto pos=locfirstpos;pos<loclastpos;pos++) {
        os<<timeatpos(pos)<<"\t"<<getid(pos)<<"\t"<<sputnikglucose(pos)<<"\t"<<rawglucose(pos)<<std::endl;
        }
    }
void exporttsv(const char * file) {
    std::ofstream uit(file);
    dumpdata(uit,getstarthistory(),getAllendhistory());
    uit.close();
    }

    bool infowrong() const {
            return getinfo()->infowrong();
        }

public:
static inline constexpr uint32_t bluestartunknown= 97812u;
void setnobluetooth() {
    getinfo()->bluestart=bluestartunknown;
    }
bool hasbluetooth() const {
    return getinfo()->bluestart!=bluestartunknown;
    }
bool canusestreaming() const {
     return  isAccuChek()||isSibionics()||isLibre3()||hasbluetooth()||isDexcom();
 //    return  hasbluetooth();
    }
const std::string_view othershortsensorname() const {
    if(isSibionics()) {
       if(siSubtype()!=3) {
           const char *name=(char *)&getinfo()->siToken;
           if(*name)
                 return {name,11};
            }
      }
   else {
      if(isDexcom()) {
         return {sensordir.data()+sensordir.length()-12,12};
         }
       }
    return {sensordir.data()+sensordir.length()-11,11};
    }
[[nodiscard]]  const std::array<uint8_t,4> getDexPin() const {
    return *reinterpret_cast<const std::array<uint8_t,4>*>(sensordir.data()+sensordir.length()-4);
   }
//typedef array<char,11>  sensorname_t;
[[nodiscard]] const sensorname_t * shortsensorname() const {
    return reinterpret_cast<const sensorname_t *>(sensordir.data()+sensordir.length()-11);
    }
typedef std::array<char,16>  longsensorname_t;
[[nodiscard]] const longsensorname_t * sensorname() const {
    return reinterpret_cast<const longsensorname_t *>(sensordir.data()+sensordir.length()-16);
    }
[[nodiscard]] std::string_view showsensorname() const {
    if(isSibionics()) {
          if(siSubtype()!=3&&getinfo()->siDeviceName[0]) 
              return std::string_view((char *)getinfo()->siDeviceName,getinfo()->siDeviceNamelen);
          }
     else  {
         if(isLibre3()) 
             return std::string_view(sensordir.data()+sensordir.length()-9,9);
         }
     return std::string_view(shortsensorname()->data(),11);
    }
    /*
static int getgeneration(const char *info) {
    return 2;
    } */
static int getgeneration(const char *info) {
    const int i = info[2] >> 4;
    const int i2 = info[2] & 0xF;
    if(i == 3) {
        if(i2 < 9) {
            return 1;
            }
        return 2;
       } 
    else if(i == 7) {
        if(i2 < 4) {
            return 1;
            }
        return 2;
        }
    return 0;
    } 




static bool mkdatabase(string_view sensordir,time_t start,const  char *uid,const  char *infodat,const timevalues *timesptr,uint8_t dupl=3,uint32_t bluestart=0,const char *blueinfo=nullptr,uint16_t secinterval=defaultinterval) {
     const uint16_t wear=timesptr->wear;
     const uint8_t days=wear/(60*24)+1;

     LOGGER("mkdatabase %s,%s",sensordir.data(),ctime(&start));
       int gen=getgeneration(infodat);
//    pathconcat  sensordir{basedir,sensorid};
    mkdir(sensordir.data(),0700);
//    string_view prob{sensordir};
    pathconcat infoname(sensordir,infopdat);
    if(access(infoname,F_OK)!=-1)  {
        Readall<uint8_t> inf(infoname);
        if(inf.data()&&inf.size()>=sizeof(Info)) {
            const Info *in=reinterpret_cast<const Info*>(inf.data());
            if(in->starttime>1590000000&&in->dupl>0&&in->info.len==6)
                return false;
            }
        }
       Info inf{.starttime=(uint32_t)start,.lastscantime=inf.starttime,.starthistory=0,.endhistory=0,.scancount=0,.startid=0,.interval=secinterval,.dupl=dupl,.days=days ,.warmup=timesptr->warmup,.wearduration=wear, .pollcount=0, .lockcount=0};
    inf.ident.len=8;
//        memcpy(&inf.ident.data,uid,8);
    memcpy(inf.ident.data,uid,8);
    inf.info.len=6;
    memcpy(inf.info.data,infodat,6);


    inf.bluestart=bluestart?bluestart:start;    
//    inf.blueinfo.sensorgen=gen;
    if(gen!=2) {
        inf.blueinfo.len=6;
        if(blueinfo) 
            memcpy(inf.blueinfo.data,blueinfo,6);
        else {
            memcpy(inf.blueinfo.data,infodat,4);
            inf.blueinfo.data[4]=0;
            inf.blueinfo.data[5]=0;
            }
        }
    writeall(infoname,&inf,sizeof(inf));
    settings->data()->haslibre2=true;
    return true;
    }
/*
E007-0M0063KNUJ0
E07A-XX068ZMRF18              
E07A-000T3YL1R50
*/
 bool isSibionics() const {
    return getinfo()->sibionics;
    }
 bool isSibionics1() const {
    return isSibionics()&&!(getinfo()->notchinese&&siSubtype()==3);
    }
 bool isSibionics2() const {
    return isSibionics()&&getinfo()->notchinese&&siSubtype()==3;
    }
 bool isDexcom() const {
    return getinfo()->dexcom;
    }
 bool isLibre3() const {
    return !isAccuChek()&&!isSibionics()&&!isDexcom()&&(getinfo()->interval==interval5);
    }
 bool isLibre2() const {
   return !(isAccuChek()||isSibionics()||isDexcom()||getinfo()->interval==interval5);
   }
bool isLibre() const {
    return !(isSibionics()||isDexcom()||isAccuChek());
    }
bool isAccuChek() const {
    return getinfo()->accuChek;
    }
int streaminterval() const {
    const int res=(isDexcom()||isAccuChek())?5:1;
    LOGGER("streaminterval()=%d\n",res);
    return res;
    }
    /*
 bool libreviewable() const {
    return !isLibre3()&&pollcount();
    } */

static    constexpr uint16_t interval5=5*60;
#ifdef LIBRE3
static bool mkdatabase3(string_view sensordir,time_t start,uint32_t pin,const char *address,uint16_t warmup, uint16_t wearduration) {
     LOGGER("mkdatabase3 warmup=%d wearduration=%d %s,%s",warmup,wearduration,sensordir.data(),ctime(&start));
    mkdir(sensordir.data(),0700);
    pathconcat infoname(sensordir,infopdat);
    if(access(infoname,F_OK)!=-1)  {
        Readall<uint8_t> inf(infoname);
        if(inf.data()&&inf.size()>=sizeof(Info)) {
            const Info *in=reinterpret_cast<const Info*>(inf.data());
            if(in->starttime>1590000000&&in->dupl>0&&in->interval==interval5)
                return false;
            }
        }
       constexpr const int minperday=60*24;
       const uint8_t  days=(wearduration+minperday-1)/minperday+1;
       LOGGER("mkdatabase days=%d\n",days);
       Info inf{.starttime=(uint32_t)start,.lastscantime=(uint32_t)time(nullptr),.starthistory=0,.endhistory=0,.scancount=0,.startid=0,.interval=interval5,.dupl=3,.days=days,.pin=pin,.lastLifeCountReceived=1,.wearduration2=wearduration,.warmup2=static_cast<uint8_t>(warmup),.pollcount=0, .lockcount=0};
    if(address)
        strcpy(inf.deviceaddress,address);
    else
        inf.deviceaddress[0]='\0';
        
    writeall(infoname,&inf,sizeof(inf));

    settings->data()->haslibre3=true;

    return true;
    }
#endif
#ifdef SIBIONICS 
static bool mkdatabaseSI(string_view sensordir,string_view sensorgegs,uint32_t now,bool hasnum) {
     LOGGER("mkdatabaseSI %s,%s\n",sensordir.data(),sensorgegs.data());
    mkdir(sensordir.data(),0700);
    pathconcat infoname(sensordir,infopdat);
    if(access(infoname,F_OK)!=-1)  {
        Readall<uint8_t> inf(infoname);
        if(inf.data()&&inf.size()>=sizeof(Info)) {
            const Info *in=reinterpret_cast<const Info*>(inf.data());
            if(in->pollcount&&in->starttime>1700000000&&in->dupl>0&&in->sibionics)
                return false;
            }
        }
    uint32_t start=now;
    //TODO other days
  //  const bool sib2=sensorgegs.size()==59;
        

       Info inf{.starttime=(uint32_t)start,.lastscantime=(uint32_t)start,.starthistory=0,.endhistory=0,.scancount=0,.startid=0,.interval=interval5,.dupl=3,.days=maxdaysSI ,.sibionics=true,.lastLifeCountReceived=0,.siType=0,.pollcount=0,.pollinterval=88.0, .lockcount=1};
       inf.siIdlen=sensorgegs.size();
       memcpy(inf.siId,sensorgegs.data(),inf.siIdlen);
       if(hasnum) {
           memcpy(inf.siBlueToothNum,sensordir.end()-11,8);
        }
    else {
           memcpy(inf.siBlueToothNum,&sensorgegs[28],8);
        }
    inf.siBlueToothNum[8]='\0';
    LOGGER("siBlueToothNum=%s\n", inf.siBlueToothNum);
        
    writeall(infoname,&inf,sizeof(inf));

    return true;
    }
#endif
static bool mkdatabaseAccu(string_view sensordir,string_view sensorgegs,uint32_t now) {
   LOGGER("mkdatabaseAccu %s,%s\n",sensordir.data(),sensorgegs.data());
    mkdir(sensordir.data(),0700);
    pathconcat infoname(sensordir,infopdat);
    if(access(infoname,F_OK)!=-1)  {
        Readall<uint8_t> inf(infoname);
        if(inf.data()&&inf.size()>=sizeof(Info)) {
            const Info *in=reinterpret_cast<const Info*>(inf.data());
            if(in->pollcount&&in->starttime>1700000000&&in->dupl>0&&in->accuChek)
                return false;
            }
        }
    uint32_t start=now;
//    Info inf{.starttime=(uint32_t)0,.lastscantime=(uint32_t)start,.starthistory=0,.endhistory=0,.scancount=0,.startid=0,.interval=interval5,.dupl=3,.days=maxdaysAccu ,.warmup=60,.wearduration=20160,.lastLifeCountReceived=1,.accuChek=true,.pollcount=0};
    Info inf{.starttime=(uint32_t)start,.lastscantime=(uint32_t)start,.starthistory=0,.endhistory=0,.scancount=0,.startid=0,.interval=interval5,.dupl=3,.days=maxdaysAccu ,.warmup=60,.wearduration=20160,.lastLifeCountReceived=1,.pollcount=0,.accuChek=true};
    inf.siIdlen=sensorgegs.size();
    memcpy(inf.siId,sensorgegs.data(),inf.siIdlen);
    writeall(infoname,&inf,sizeof(inf));

    return true;
    };

#ifdef DEXCOM
static bool mkdatabaseDex(string_view sensordir,string_view sensorgegs,uint32_t now) {
   LOGGER("mkdatabaseDex %s,%s\n",sensordir.data(),sensorgegs.data());
    mkdir(sensordir.data(),0700);
    pathconcat infoname(sensordir,infopdat);
    if(access(infoname,F_OK)!=-1)  {
        Readall<uint8_t> inf(infoname);
        if(inf.data()&&inf.size()>=sizeof(Info)) {
            const Info *in=reinterpret_cast<const Info*>(inf.data());
            if(in->pollcount&&in->starttime>1700000000&&in->dupl>0&&in->dexcom)
                return false;
            }
        }
    uint32_t start=now;
    Info inf{.starttime=(uint32_t)now,.lastscantime=(uint32_t)start,.starthistory=0,.endhistory=0,.scancount=0,.startid=0,.interval=interval5,.dupl=3,.dexcom=true,.days=maxdaysDex ,.warmup=30,.wearduration=14400,.lastLifeCountReceived=1,.pollcount=0};
    inf.siIdlen=sensorgegs.size();
    memcpy(inf.siId,sensorgegs.data(),inf.siIdlen);
    writeall(infoname,&inf,sizeof(inf));

//        settings->data()->balanced_priority=false;
 //       settings->data()->android13=true;
    return true;
    }

#endif
    /*
bool bluetoothfirst() const {
    return !getinfo()->blueinfo.data[4]&&!getinfo()->blueinfo.data[5];
    }*/
const std::pair<uint32_t, const data_t *> getbluedata() const {
    if(getinfo()->bluestart==0)
        return {getinfo()->starttime, reinterpret_cast<const data_t *>(&getinfo()->info)};

    return {getinfo()->bluestart, reinterpret_cast<const data_t *>(&getinfo()->blueinfo)};
    }
static constexpr char blueback[]= "bluetooth.bak";
bool setbluetooth(uint32_t start,const signed char *infoin) {
    pathconcat backup(sensordir,blueback);
          Create file(backup.data());
    if(file<0)
        return false;
    if(write(file,&getinfo()->bluestart,4)!=4|| write(file,getinfo()->blueinfo.data,6)!=6)
        return false;
    getinfo()->bluestart=start;
    getinfo()->blueinfo.len=6;
    memcpy(getinfo()->blueinfo.data,infoin,6);
    return true;
    }
bool bluetoothback() {
    Readall all(blueback);
    const char *data=all.data();    
    if(!data)
        return false;
    unlink(blueback);
    memcpy(&getinfo()->bluestart,data,4);
    memcpy(getinfo()->blueinfo.data,data+4,6);
    return true;
    }


bool unused() const {
    const auto *info=getinfo();
    if(info)  {
        const int un=(info->pollcount==0&&info->scancount==0&&info->endhistory==0);
        LOGGER("unused()=%d\n",un);
        return un;
        }
    LOGGER("unused %p->getinfo()==null\n",this);
    return false;
    }
bool canscan() const {
    return isLibre2();
    }
private:
size_t maxscansize()  {
    if(!getinfo())      {
        LOGGER("%s: maxscansize()  getinfo()==null",shortsensorname()->data());
        haserror=true;
        return 0;
        }
    if(!canscan()) return 4;
    const int days=    std::max((int)getinfo()->days,15);
    const int scanblocks=ceil((40*days*sizeof(ScanData))/blocksize);
    int used=getinfo()->scancount*sizeof(ScanData);
    int past= getAllendhistory()-getinfo()->starthistory;
    int take;
    int maxp=maxpos();
    if(maxp<past)
        maxp+=24*perhour();
    if(used<=0||past<=0||(take =ceil((maxp+(maxp-past)*.2)*used/((double)past*blocksize)))<scanblocks)
        take=scanblocks;
    LOGGER("blocksize=%d take=%d used=%d maxp=%d\n",blocksize,take,used,maxp);
    return take*blocksize/sizeof(ScanData);
     }





public:
Info *getinfo() {
    return reinterpret_cast<Info *>(meminfo.data());
    }
const Info *getinfo() const {
    return reinterpret_cast<const Info *>(meminfo.data());
    }
private:
 int specstart;
static constexpr const char infopdat[]="info.dat";
pathconcat polluit;
 pathconcat infopath;
 pathconcat updateinfopath;
 pathconcat histpath;
 pathconcat scanpath;
 pathconcat trendspath;



 static constexpr const char trendsdat[]="trends.dat";
SensorGlucoseData(string_view sensordir,int spec,string_view baseuit,int sensorindex): sensordir(sensordir),meminfo(sensordir,infopdat,sizeof(struct Info)),historydata(sensordir,"data.dat",getinfo()?historybytes(perhour()):(haserror=true,0)),
scansize(maxscansize()),
scans(sensordir,"current.dat",scansize),
polls(sensordir,"polls.dat",maxstreampos()),
trends(sensordir,trendsdat,scansize),
specstart(spec),
 polluit(baseuit, "polls.dat"),
 infopath(baseuit, infopdat),
 updateinfopath(baseuit, "updateinfo.dat"),
 histpath(baseuit, "data.dat"),
 scanpath(baseuit, "current.dat"),
 trendspath(baseuit, trendsdat),
 statefile(sensordir,"state.json"),
 binstatefile(sensordir,"state.bin")

#ifdef SIHISTORY
 ,statefile3(sensordir,"state3.json")
#endif
,sensorIndex(sensorindex)
{
LOGGER("%p=SensorGlucoseData(%s)\n",this, baseuit.data());
if(error()) {
    LOGGER("SensorGlucoseData %s %s Error\n",sensordir.data(),baseuit.data());
    return;
    }
if(isSibionics2())
    getinfo()->days=45;
if(const ScanData *last=lastpoll()) {
    if(last->g) {
        lastlifecount=last->id;;
        timelastcurrent=last->t;
        LOGGER("lastlifecount=%d %s",lastlifecount,ctime(&timelastcurrent));
        }
    int start=getinfo()->pollstart;
    if((start+1)<pollcount()) {
        if(!polls[start].t)
            polls[start].t=polls[start+1].t-60;

        }
    else {
        if(!polls[start].t) {
            getinfo()->pollcount=std::max(start,0);
            }
        }
    }
   if(!(isAccuChek()||isSibionics()||isDexcom())) {
      LOGGER("getinfo()->lastHistoricLifeCountReceivedPos=%d\n", getinfo()->lastHistoricLifeCountReceivedPos);
      if(!getinfo()->lastHistoricLifeCountReceivedPos) getinfo()->lastHistoricLifeCountReceivedPos=12;
      LOGGER("SensorGlucoseData %s %s scansize=%zu\n",sensordir.data(),scanpath.data(),scansize);
      if(getinfo()->pollinterval<58.6||getinfo()->pollinterval>62.6) getinfo()->pollinterval=60.5752;
      if(!getinfo()->prunedstream) {
            prunedata() ;
            getinfo()->prunedstream=true;
            }
       }
   }

    public:
template <typename T>
std::string_view absToRel(T& absname) {
    const auto start=globalbasedir.size()+1;
    return std::string_view{absname.data()+start,absname.size()-start};
    } 
   /*
std::string_view relstatefile() {
    return absToRel(statefile);
    }
*/
    /*
template <typename T>
const char * absToRel(T absname) {
    const int start=globalbasedir.size()+1;
    return absname.data()+start;
    }
const char * relstatefile() {
    return absToRel(statefile);
    } */
pathconcat statefile;
pathconcat binstatefile;
const int sensorIndex;

#ifdef SIHISTORY
pathconcat statefile3;
#endif



SensorGlucoseData(string_view sensin,int specin,int sensorindex): SensorGlucoseData(sensin, specin, std::string_view(sensin.data()+specin,sensin.size()-specin),sensorindex) {
    }
    public:
SensorGlucoseData(string_view sensin,int sensorindex): SensorGlucoseData(sensin,globalbasedir.size()+1,sensorindex) {
    }
//bool haserror=false;
bool error() const {
    if(!haserror&&meminfo.data()&& historydata.data()&&polls.data()&& (!canscan()||(scans.data()&& trends.data())))
        return false;    
    return true;
    }

void    prunestream() ;
void    prunescans() ;

void prunedata() ;
void prunescansonly();
void setlastscantime(uint32_t tim) {
    getinfo()->lastscantime=tim;
    }
uint32_t getlastscantime() const {
    return getinfo()->lastscantime;
    }
    /*
uint32_t getlastpolltime() const {
    uint32_t last=(pollcount()>0)?lastpoll()->t:0;
    return last;
    }
uint32_t getlastpolltime() const {
    const auto stream=lastpoll();
    if(!stream)
        return 0;
    return stream->t;
    }
    */

uint32_t getlastpolltime() const {
    const ScanData* start= polls.data();
    if(!start)
        return 0;
    for(int i=pollcount()-1;i>=getinfo()->pollstart;--i) {
        if(start[i].valid(i))
            return start[i].t;    
        }
    return 0;
    }
uint32_t firstpolltime() const {
    const ScanData* start= polls.data();
    for(int i=getinfo()->pollstart;i<pollcount();i++)
        if(start[i].valid(i))
            return start[i].t;    
    return UINT32_MAX;
    }
int scancount() const {
    return getinfo()->scancount;
    }
int pollcount() const {
    return getinfo()->pollcount;
    }
std::array<uint16_t,16> &gettrendsbuf(int index) {
    return trends[index];
    }
const    std::array<uint16_t,16> &gettrendsbuf(int index) const {
    return trends[index];
    }
void savetrend(const nfcdata *scan,std::array<uint16_t,16> &trendbuf){
    for(int i=0;i<trend::num;i++) 
        trendbuf[i]=scan->getglucose<trend>(i);
    
    }

void saveglucose(const nfcdata*nfc,time_t tim,int id,int glu,int trend,float change) {
    savetrend(nfc,gettrendsbuf(scancount()));
    saveglucosedata(scans,getinfo()->scancount,tim, id, glu, trend, change);
    setlastscantime(tim);
    }

bool savepoll(time_t tim,int id,int glu,int trend,float change) {
    if(getinfo()->pollcount) {
        int count=getinfo()->pollcount-1;
        int previd=polls[count].id;
        if(previd>=id)  {
            LOGGER("GLU: duplicate id: previd=%d id=%d\n",previd,id);
            return false;
            }
        uint32_t prevt=polls[count].t;        
        uint32_t predict =prevt+(id-previd)*getinfo()->pollinterval;
        const int verschil=tim-predict;
        if(verschil>3*60)   {
            LOGGER("GLU: oldvalue: pollinterval=%f %d prev=%d savepoll %d nu=%lu, %d  versch=%d %s",getinfo()->pollinterval,previd,prevt,id,tim,glu,verschil,ctime(&tim));
            }
        else {
            constexpr const double weight=0.9;
            double af=(double)(tim-prevt)/(id-previd);
            getinfo()->pollinterval= weight*getinfo()->pollinterval+(1.0-weight)*af;
            }
        }
    saveglucosedata(polls,getinfo()->pollcount,tim, id, glu, trend, change);
    return true;
    }

void savestreamonly(time_t tim,int id,int glu,int trend,float change) {
    saveglucosedata(polls,getinfo()->pollcount,tim, id, glu, trend, change);
    }

void savestream(time_t tim,int id,int glu,int trend,float change) {
    if(getinfo()->redoAll) {
        getinfo()->redoAll=saveStreamAgain(tim,id,glu,trend,change);
        return;
        }
     else {
        saveglucosedata(polls,getinfo()->pollcount,tim, id, glu, trend, change);
        }
    }

bool saveStreamAgain(time_t tim,int id,int glu,int trend,float change) {
     int index=id-5;
     if(index<0) {
        return true;
        }
     while(index>0&&polls[index].id>id)
        --index;
     while(index<getinfo()->pollcount&&polls[index].id<id) {
        ++index;
        }
     polls[index]={static_cast<uint32_t>(tim),id,glu,trend,change};
     const int count=index+1;
     if(count<getinfo()->pollcount) {
        return true;
        }
    getinfo()->pollcount=count;
    return false;
    }
void saveglucosedata(Mmap<ScanData> &streamscans,uint32_t &count,time_t tim,int id,int glu,int trend,float change) {
     streamscans[count++]={static_cast<uint32_t>(tim),id,glu,trend,change};
    }
bool hasStreamID(const int id) const {
    return polls[id].id==id&&polls[id].g;
    }
template <int secs> int savepollallIDsonly(time_t tim,const int id,int glu,int trend,float change) {
    int count=getinfo()->pollcount;
    if(count<id) {
       LOGGER("savepollallIDsonly count=%d<id=%d\n",count,id);
       const uint32_t startiter=tim-(id-count)*secs;
       if constexpr (secs==60) {
           if(!count) {
          LOGAR("savepollallIDsonly !count");
          const auto starttime=getinfo()->starttime;
          if(starttime>startiter||(startiter-starttime)>60*60) {
             const uint32_t newstart=startiter-80;
             getinfo()->starttime=newstart;
             LOGGER("new start=%d\n",newstart);
             }
           }
          }
      for(uint32_t timiter=startiter;count<id;++count,timiter+=secs)  {
          if(!polls[count].t||polls[count].id!=count)
              polls[count]={timiter,count,0,0,0.0};
          }
      if(!count) {
          getinfo()->pollstart=id;    
          }
      }
    LOGGER("count=%d savepollallIDsonly(%lu,%d,%.1f,%d,%.1f) %s",count,tim,id,glu/convfactordL,trend,change,ctime(&tim));
    polls[id]={static_cast<uint32_t>(tim),id,glu,trend,change};
    return count;
    }
template <int secs> bool savepollallIDs(time_t tim,const int id,int glu,int trend,float change) {
    int count=savepollallIDsonly<secs>(tim,id,glu,trend,change);
    if(id==count)
        getinfo()->pollcount=id+1;
    else {
        if(id<getinfo()->pollstart)
            getinfo()->pollstart=id;
        }
    return true;
    }




void consecutivelifecount() {
    const int pos=getinfo()->lastLifeCountReceived;
    const int count=getinfo()->pollcount;
    for(int i=pos+1;i<count;i++) {
         if(!polls[i].g&&!isnan(polls[i].ch)) {
          getinfo()->lastLifeCountReceived=i-1;
          LOGGER("consecutivelifecount1 getinfo()->lastLifeCountReceived=%d\n",i-1);
          return;
          }
         }
    const int newrec=count-1;
    if(newrec>0)
        getinfo()->lastLifeCountReceived=newrec;
    LOGGER("consecutivelifecount2 getinfo()->lastLifeCountReceived=%d (was %d)\n",newrec,pos);
    }
void fastupdatelifecount(int fastcount) {
    if(getinfo()->lastLifeCountReceived<fastcount) {
        getinfo()->lastLifeCountReceived=fastcount;
        LOGGER("fastupdatelifecount getinfo()->lastLifeCountReceived=%d\n",fastcount);
        }
    }
void consecutivehistorylifecount() {
    const int pos=getinfo()->lastHistoricLifeCountReceivedPos;
    const int count=getScanendhistory();
#ifndef NOLOG
    destruct _des([pos,count,this]{
    LOGGER("consecutivehistorylifecount() lastHistoricLifeCountReceivedPos=%d getendhistory=%d  new lastHistoricLifeCountReceivedPos=%d\n",pos,count,getinfo()->lastHistoricLifeCountReceivedPos); });
#endif
    for(int i=pos+1;i<count;i++) {
        Glucose *item=getglucose(i);
        if(!item->id) {
            getinfo()->lastHistoricLifeCountReceivedPos=i-1;
            return;
            }
        }
    int newrec=count-1;
    if(newrec>0)
        getinfo()->lastHistoricLifeCountReceivedPos=newrec;
    }
void updateHistsorylifecount(int newpos) {
    if(getinfo()->lastHistoricLifeCountReceivedPos<newpos) {
        getinfo()->lastHistoricLifeCountReceivedPos=newpos;
        }
    LOGGER(" updateHistsorylifecount(%d) getinfo()->lastHistoricLifeCountReceivedPos=%d\n",newpos,
        getinfo()->lastHistoricLifeCountReceivedPos);
    }


const std::span<const ScanData> getScandata() const {
    return std::span<const ScanData>(scans.data(),scancount());
    }
const ScanData* beginscans() const {
    return scans.data();
    }
const ScanData*beginpolls() const { 
    return polls.data();
    }
std::span<const ScanData> getPolldata() const {
    const int start=getinfo()->pollstart;
    return std::span<const ScanData>(polls.data()+start,pollcount()-start);
    }

const ScanData *lastscan() const {
    const ScanData *start=scans.data();
    for(int i=scancount()-1;i>=0;i--) {
        const ScanData *sc=start+i;
        if(sc->g)
            return sc;
        }
    return nullptr;
    }
const ScanData *lastpoll() const {
    const auto polc=pollcount();
    if(polc>0)
        return polls.data()+polc-1;
    return nullptr;
    }
const ScanData *lastValidStream() const {
    for(int i=pollcount()-1;i>=0;--i) {
        const ScanData *el= polls.data()+i;
        if(el->valid())
                return el;
        }
    return nullptr;
    }
const ScanData *getscan(int ind) const {
    return scans.data()+ind;
    }
const ScanData *getstream(int ind) const {
    return polls.data()+ind;
    }
static void exportscans(const char *file,int count,const ScanData *scans)  {
    std::ofstream uit(file);
    for(int i=0;i<count;i++) {
        const ScanData &scan=scans[i];    
        uit<<scan.t<<"\t"<<scan.id<<'\t'<<scan.g<<'\t'<<scan.tr<<'\t'<<scan.ch<<std::endl;
        }
    uit.close();
    }

void exportscans(const char *file) const {
    exportscans(file,getinfo()->scancount,scans.data());
    }
void exportpolls(const char *file) const {
    exportscans(file,getinfo()->pollcount,polls.data());
    }
int nextlock()  {
    return getinfo()->lockcount++;
    }
void setlock(uint32_t lock) {
    getinfo()->lockcount=lock;
    }
const string &getsensordir() const {
    return sensordir;
    }
void removeoldstates()  {
extern void removeoldstates(const std::string_view dirin) ;
    removeoldstates(sensordir);
    }
const data_t *getpatchinfo() const {
    return reinterpret_cast<const data_t *>(&getinfo()->info);
    }
const data_t *getsensorident() const {
    return reinterpret_cast<const data_t *>(&getinfo()->ident);
    }
uint32_t lastused() const {
    uint32_t last=getlastpolltime();
    if(last<getlastscantime())
        return getlastscantime();
    return last;
    }

void updateinit(const int ind) {
    getinfo()->update[ind]={};    
    getinfo()->caliUpdated[ind]={};
    }



static  const ScanData *firstnotless(std::span<const ScanData> dat,const uint32_t start) {
    const ScanData *scan=&dat[0];
        const ScanData *endscan= &dat.end()[0];
        const ScanData scanst{.t=start};
        auto comp=[](const ScanData &el,const ScanData &se ){return el.t<se.t;};
        return std::lower_bound(scan,endscan, scanst,comp);
    }
static CurData curInperiod(std::span<const ScanData> dat,const uint32_t starttime,const uint32_t endtime) {
    const ScanData *scan=&dat[0];
    const ScanData *endscan= &dat.end()[0];
    auto comp=[](const ScanData &el,const ScanData &se ){return el.t<se.t;};
    ScanData scanst{.t=starttime};
    auto first=std::lower_bound(scan,endscan, scanst,comp);
    if(first==endscan) 
        return {scan,endscan,endscan};
    scanst.t=endtime;
    return {scan,first,std::lower_bound(first,endscan, scanst,comp)};
    }
CurData  streamInperiod(const uint32_t starttime,const uint32_t endtime) const {
    return curInperiod(getPolldata() ,starttime,endtime);
    }
CurData  scanInperiod(const uint32_t starttime,const uint32_t endtime) const {
    return curInperiod(getScandata(),starttime,endtime);
    }



dataonlyptr  getfromfile(crypt_t *pass,int sock, std::string_view filename,int offset,int asklen) {
    const int pathlen=filename.size()+1;
    constexpr const int ali=alignof(struct askfile);
    const int comlen=((sizeof(askfile)+(pathlen+ali-1))/ali)*ali;
    alignas(ali) uint8_t buf[comlen];
    struct askfile *ask=reinterpret_cast<askfile *>(buf); 
    ask->com=saskfile;
    ask->off=offset;
    ask->len= asklen;

    ask->namelen=pathlen;

    LOGGER("GLU: getfromfile(sock=%d %s offset=%d asklen=%d comlen=%d)\n",sock,filename.data(),offset,asklen,comlen);
    memcpy(ask->name,filename.data(),pathlen);
    if(!noacksendcommand(pass,sock,buf,comlen) ) {
        LOGAR("GLU:  !noacksendcommand");
        return dataonlyptr(nullptr); 
        }
    return receivedataonly(sock,pass, asklen);

    };

int posearlier(int pos,uint32_t starttime) {
#ifndef NOLOG
    time_t timt=starttime;
    LOGGER("postearlier %d %s",pos,ctime(&timt));
#endif
    for(int    i=pos-1;i>=0;i--) { //CHANGED
        if(uint32_t tim=timeatpos(i)) {
            if(tim<=starttime) {
                LOGGER("pos=%d\n",i+1);
                return i+1;     
                }
            else  {
                LOGAR("pos=-1?");
                return -1;
                }
            }
        }
    LOGAR("pos=0");
    return 0;
    }

int getbackuptimestream(uint32_t starttime)  {
    decltype(auto) poldat=getPolldata();
    int pos=firstnotless(poldat,starttime)-beginpolls();
    LOGGER("GLU: getbackuptimestream pos=%d\n",pos);
    return pos;
    }
int  getbackuptimescan(uint32_t starttime)  {
    decltype(auto) scandat=getScandata();
    int pos=firstnotless(scandat,starttime)-&scandat[0];
    LOGGER("GLU: getbackuptimescan pos=%d\n",pos);
    return pos;
    }
    /*
bool setbackuptimes(crypt_t *pass,int sock,int ind,uint32_t starttime) {
    const int pathlen=infopath.size()+1;
    constexpr const int ali=alignof(struct askfile);
    const int comlen=((sizeof(askfile)+(pathlen+ali-1))/ali)*ali;
    const int totlen=sizeof(askfile)+(pathlen);
    const int comlen2=(ali-totlen%ali)+totlen;
    assert(comlen==comlen2);
    alignas(ali) uint8_t buf[comlen];
    struct askfile *ask=reinterpret_cast<askfile *>(buf); 
    const int asklen= offsetof(Info,pollinterval);
    ask->com=saskfile;ask->off=0;ask->len=asklen;
    memcpy(ask->name,infopath.data(),pathlen);
    if(!noacksendcommand(pass,sock,buf,comlen) ) {
        return false;
        }
       dataonlyptr res=receivedata(sock,pass, asklen) ;

    } */
uint32_t getbackuptimehistory(uint32_t starttime)  {
    int pos=gettimepos(starttime);
    const int endpos=getAllendhistory();
    const int first=getstarthistory();
    if(pos<first)
        pos=first;
    else {
        if(pos>endpos)
            pos=endpos;
        else {
            while(pos>first&&timeatpos(pos)>starttime) {
                LOGSTRING("GLU:  > \n");
                pos--;
                }
            while(pos<endpos&&timeatpos(pos)<starttime) {
                LOGSTRING("GLU:  < \n");
                pos++;
                }
            }
        }
#ifndef NOLOG
    time_t tim=starttime;
    LOGGER("GLU: getbackuptimehistory pos=%d %s",pos,ctime(&tim));
#endif
    return pos;
    }
    /*
void setbackuptime(int ind,uint32_t starttime) {
     setbackuptimestream(ind,starttime)  ;
     setbackuptimescan(ind,starttime)  ;
     setbackuptimehistory(ind,starttime)  ;
     }
     */
void backhistory(int pos) ;
void backcalibrated(int pos);
void backstream(int pos) ;

bool setbackuptime(crypt_t *pass,int sock,int ind,uint32_t starttime) {
    
    constexpr const int asklen= offsetof(Info,pollinterval);
    constexpr const int minlen= offsetof(Info,pollcount);
    LOGGER("GLU: %s setbackuptime %u asklen=%d\n",shortsensorname()->data(),starttime,asklen);
    auto dontdestroy=getfromfile(pass,sock,infopath, 0,asklen);
   dataonly *dat=dontdestroy.get();
    if(dat==nullptr) {
        LOGSTRING("GLU: ==nullptr\n");
        return false;
        }

    LOGGER("GLU: len=%d\n",dat->len);

    if(dat->len>=minlen) {
        const Info *infothere=reinterpret_cast<const Info*>(dat->data);
        if(getinfo()->update[ind].scanstart==0) {
            uint32_t scantime= infothere->lastscantime;
            uint32_t scanend=infothere->scancount;
#ifndef NOLOG
            uint32_t scanstart=
#endif
            getinfo()->update[ind].scanstart=(scantime<starttime)?scanend:getbackuptimescan(starttime);
            LOGGER("GLU: scanstart=%d scancount=%d\n",scanstart,scanend);
            }
        else {
            LOGGER("GLU scanstart=%d\n",getinfo()->update[ind].scanstart);
            }
        if(getinfo()->update[ind].histstart==0) { //TODO why only when it is zero?
        //Why not just the from the position with that time here?
        //Why get it when it is not used?
            uint32_t histend=infothere->endhistory;
            int histpos;

#ifndef NOLOG
            uint32_t histstart=
#endif
            getinfo()->update[ind].histstart= (histpos=posearlier(histend,starttime)>=0)?histpos:getbackuptimehistory(starttime);

            LOGGER("GLU: hist start=%d endhistory=%d\n",histstart,histend);
            }
        else {
            LOGGER("GLU histstart=%d\n",getinfo()->update[ind].histstart);
            }
        if(getinfo()->update[ind].streamstart==0) {
#ifndef NOLOG
            uint32_t streamend=(dat->len<asklen)?0:infothere->pollcount;
#endif
#ifndef NOLOG
            uint32_t streamstart=
#endif
            getinfo()->update[ind].streamstart=getbackuptimestream(starttime);
            LOGGER("GLU: %s streamstart=%d streamend=%d\n", showsensorname().data(),streamstart,streamend);
            }
        else {
            LOGGER("GLU: %s streamstart=%d\n", showsensorname().data(),getinfo()->update[ind].streamstart);
            }
        }
    return true;
    }
    
//constexpr const int streamupdatebit=1<<15;
static uint16_t streamupdatecmd(int sensindex) {
    return sensindex<0?0:(streamupdatebit|sensindex);
    }

void setupdatechange(int maxind,uint32_t updatestate::*member,uint32_t value ) {
    updatestate  *up= getinfo()->update;
    for(int i=0;i<maxind;i++) {
        if(up[i].*member>value)
            up[i].*member=value;
            
        }
    }
void setstarthistback(int maxint,uint32_t histchange) {
    setupdatechange(maxint,&updatestate::histstart,histchange);
    }
void setrawstreamstart(int maxint,uint32_t newvalue) {
    setupdatechange(maxint,&updatestate::rawstreamstart,newvalue);
    }
/*
DOESN"T work with bitfield members
template <typename VALUE_T>
void setupdatevalue(int maxind,uint32_t updatestate::*member,VALUE_T value ) {
    updatestate  *up= getinfo()->update;
    for(int i=0;i<maxind;i++) {
                    up[i].*member=value;
        }
    } */


void setsendstreaming(int maxind) {
    updatestate  *up= getinfo()->update;
    for(int i=0;i<maxind;i++) {
        up[i].sendstreaming=true;
        }
    }  
void setsendKAuth(int maxind) {
    updatestate  *up= getinfo()->update;
    for(int i=0;i<maxind;i++) {
        up[i].sendKAuth=true;
        }
    }

void sendbluetoothOn(int maxind) {
    updatestate  *up= getinfo()->update;
    for(int i=0;i<maxind;i++) {
        up[i].sendbluetoothOn=true;
        }
    }


void setsiScan(int maxind) {
    updatestate  *up= getinfo()->update;
    for(int i=0;i<maxind;i++) {
        up[i].siScan=false;
        }
    }

int updateKAuth(crypt_t *pass,int sock,int ind);
void setsendhiststart() {
    const int maxind=getgetsendnr();
    updatestate  *up= getinfo()->update;
    for(int i=0;i<maxind;i++) {
        up[i].sendhiststart=true;
        }
    }
/*
sendscan:    
0: don't send history
1: also via stream
2: also via scan
*/
private:
int sendSistate(const pathconcat *sfile, crypt_t *pass,int sock)  {
    const char *statename=sfile->data();
    Readall<unsigned char> stateBytes(statename);
    if(!stateBytes.data()||!stateBytes.size()) {
        LOGGER("GLU: %s doesn't exist\n",statename);
        return 2;
        }
    const auto relstate=absToRel(*sfile);
    LOGGER("statefile=%s\n",statename);
    if(!senddata(pass,sock,0,stateBytes.data(),stateBytes.size(),relstate)) {
        LOGGER("GLU: senddata %s failed\n",relstate.data());
        return 0;
        }
    return 1;
    }
public:
int sendSibionicsState(crypt_t *pass,int sock,int ind)  {
    int waslock=getinfo()->lockcount;
    if(getinfo()->update[ind].rawstreamstart<waslock) {
        int res=sendSistate(&binstatefile,pass,sock);
        /*
        if(res==2) {
            res=sendSistate(&statefile,pass,sock);
           } */
        if(res==1) {
            getinfo()->update[ind].rawstreamstart=waslock;
            }
        return res;    
        }
    return 2;
    }

int updatestream(crypt_t *pass,int sock,int ind,int sensindex,int sendscan)  {
    getinfo()->update[ind].changedstreamstart=false;
    int streamstart=getinfo()->update[ind].streamstart;
    LOGGER("updatestream sock=%d ind=%d sensindex=%d streamstart=%d sendscan=%d\n",sock,ind,sensindex,streamstart,sendscan);
    struct {
        uint32_t pollcount;
        double pollinterval; 
        uint32_t lockcount;
        uint8_t streamingIsEnabled;
        } pollinfo;
    constexpr const int off=offsetof(Info,pollcount); 
    constexpr const int len=offsetof(Info,streamingIsEnabled)+sizeof(Info::streamingIsEnabled)-off;
    memcpy(&pollinfo,meminfo.data()+off,len);
    const int streamend=pollinfo.pollcount;     //TODO test earlier?
    if(streamstart<streamend) {
        const struct ScanData *startstreambuf= polls.data();
        const uint16_t cmd=streamupdatecmd(sensindex);
#ifndef NOLOG
        const struct ScanData *fn=startstreambuf+streamstart;
        time_t tim=fn->t;
        LOGGER("GLU: streamstart=%d streamend=%d %s %.1f (%d) dowith=%d %s",streamstart,streamend,polluit.data(),fn->g/convfactordL,fn->g,cmd,ctime(&tim));
#endif

        if(!senddata(pass,sock,streamstart,startstreambuf+streamstart,startstreambuf+streamend,polluit)) {
            LOGSTRING("GLU: senddata polls.dat failed\n");
            return 0;
            }

      struct {
          uint16_t endStreamhistory; 
          uint16_t startedwithStreamhistory; 
        } endinfo;
        std::vector<subdata> vect;
        bool wrotehistory=false;
        const bool sendhiststart=getinfo()->update[ind].sendhiststart;
        switch(sendscan) {
            case 1: {
                memcpy(&endinfo,&getinfo()->endStreamhistory,sizeof(endinfo));
                int res=newsendhistory(pass,sock,ind,sensindex, true,endinfo.endStreamhistory) ;
                switch(res) {
                    case 0: return 0;
                    case 1: {
                        wrotehistory=true;
                        vect.reserve(3+sendhiststart);
                        break;
                        };
                    default:
                        vect.reserve(2+sendhiststart);

                    };
                };break;
            case 2: {
                memcpy(&endinfo,&getinfo()->endStreamhistory,sizeof(endinfo));
                int res=oldsendhistory(pass,sock,ind, sensindex,false,endinfo.endStreamhistory) ;
                switch(res) {
                    case 0: return 0;
                    case 1: {
                        wrotehistory=true;
                        vect.reserve(3+sendhiststart);
                        break;
                        };
                    default:
                    vect.reserve(2+sendhiststart);
                    };
                break;
                }
            default: 
                vect.reserve(2);
                break;
            };
        vect.push_back({reinterpret_cast<uint8_t*>(&pollinfo),off,len});
      bool updateStarttime=false;
      if(isSibionics()) {
        if(!getinfo()->update[ind].siStream&&pollcount()>0&&getinfo()->siDeviceName[0]&&
                                 getinfo()->deviceaddress[0]&&
                                 getinfo()->siDeviceNamelen>3) {
            updateStarttime=true;
            LOGAR("updateStream send starttime, deviceName and deviceAddress");
            vect.push_back({reinterpret_cast<const senddata_t *>(&getinfo()->starttime),offsetof(Info,starttime),4});
            vect.push_back({reinterpret_cast<const senddata_t *>(&getinfo()->siDeviceNamelen-1),offsetof(Info,siDeviceNamelen)-1,19});
            vect.push_back({reinterpret_cast<const senddata_t *>(getinfo()->deviceaddress),offsetof(Info,deviceaddress),deviceaddresslen});
            }
         }
        else {
         if(isAccuChek()) {
             if(!getinfo()->update[ind].siStream&&pollcount()) {
                 updateStarttime=true;
                 LOGAR("updateStream send starttime");
                 vect.push_back({reinterpret_cast<const senddata_t *>(&getinfo()->starttime),offsetof(Info,starttime),4});
                 }
            }
         else {
             if(isDexcom()&&!getinfo()->update[ind].siStream&&pollcount()) {
                   updateStarttime=true;
               LOGAR("updateStream send starttime");
                   vect.push_back({reinterpret_cast<const senddata_t *>(&getinfo()->starttime),offsetof(Info,starttime),4});
                   vect.push_back({reinterpret_cast<const senddata_t *>(getinfo()->DexDeviceName),offsetof(Info,DexDeviceName),12});
                   vect.push_back({reinterpret_cast<const senddata_t *>(getinfo()->deviceaddress),offsetof(Info,deviceaddress),deviceaddresslen});
                }
              }
         if(wrotehistory) {
               vect.push_back({reinterpret_cast<const senddata_t *>(&endinfo),offsetof(Info,endStreamhistory),sizeof(endinfo)});
               }
           }
         if(sendhiststart) {
            vect.push_back({reinterpret_cast<const senddata_t *>(&getinfo()->starthistory),offsetof(Info,starthistory),sizeof(getinfo()->starthistory)});
            }

            vect.push_back({reinterpret_cast<const senddata_t *>(&getinfo()->broadcastfrom),offsetof(Info,broadcastfrom),sizeof(getinfo()->broadcastfrom)});
         if(!senddata(pass,sock,vect, infopath,cmd,reinterpret_cast<const uint8_t *>(&streamstart),sizeof(streamstart))) {
            LOGSTRING("GLU: senddata info.data failed\n");
            return 0;
            }
        if(updateStarttime) {
            if(!writeStartime(pass,sock,sensindex))  {
                return 0;
                }
            getinfo()->update[ind].siStream=true;
            }

        if(!getinfo()->update[ind].changedstreamstart) {
            LOGGER("streamstart=%d\n",streamend);
            getinfo()->update[ind].streamstart=streamend;

            }
        else  {
            LOGSTRING("startchanged\n");
            }
        if(sendhiststart) getinfo()->update[ind].sendhiststart=false;
        if(isLibre3()||isDexcom()) {
            int endhistory=getScanendhistory();    
            if(oldsendhistory(pass,sock,ind,sensindex,true,endhistory))
                return 1;
            return 0;
            }

        return 1;
        }
    else {
        if(getinfo()->update[ind].sendbluetoothOn) {
              constexpr    const int offset=offsetof(Info,streamingIsEnabled);
              if(!senddata(pass,sock,offset,meminfo.data()+offset,1, infopath)) {
                LOGSTRING("GLU: senddata bluetoothON info.data failed\n");
                      return 0;
                   }
            getinfo()->update[ind].sendbluetoothOn=false;
            return 1;
            }
        }
    return 2;
    }
    
private:


int sendhistoryinfo(crypt_t *pass,int sock,int sensorindex,uint32_t histstart,uint32_t endhistory) ;
//int sendhistory(crypt_t *pass,int sock,int ind,int sendindex,bool) ;
int oldsendhistory(crypt_t *pass,int sock,int ind,int sensorindex,bool sendinfo,int histend) ;
int newsendhistory(crypt_t *pass,int sock,int ind,int sensorindex,bool sendStream,int histend) ;

public:
int updatescan(crypt_t *pass,int sock,int  ind,int sensorindex,bool,int sendstream); 
int updatescanalg(crypt_t *pass,int sock,int  ind,int sensorindex,int alsostream) { 
    return updatescan(pass,sock,ind,sensorindex,false,alsostream)&3; 
    }
int lastlifecount=0;
time_t timelastcurrent=0;
time_t lifeCount2time(uint32_t lifecount) {
    time_t uit;
    if(lastlifecount) {
        uit=timelastcurrent-60LL*(lastlifecount-(int64_t)lifecount);
        }
    else {
        uit=getstarttime()+60LL*(lifecount+1);
        }
    LOGGER("timelastcurrent=%ld lastlifecount=%d lifeCount2time(lifecount=%d)=%lld\n", timelastcurrent,lastlifecount,lifecount,uit);
    return uit;
    }

bool sensorerror=false;
uint32_t sensorErrorTime;
bool hasSensorError(uint32_t nu) const {
    return sensorerror&&((nu-sensorErrorTime)<(60*(isDexcom()?15:10)));
    }
bool replacesensor=false;
std::vector<int>viewed;
int getSiIndex() const {
    return getinfo()->lockcount;
    }
void setSiIndex(int index)  {
    getinfo()->lockcount=index;
    }

uint32_t receivehistory=0;
int retried=0;
bool scannedAddress=false;
void setbroadcastfrom(int br) {
   LOGGER("setbroadcastfrom(%d)\n",br);
    getinfo()->broadcastfrom=br;
    }
int getbroadcastfrom() const {
    return getinfo()->broadcastfrom;
    }


void setNotchinese() {
   if(isSibionics()) {
        getinfo()->notchinese=true;
        LOGAR("setNotchinese()");
        }
    }
bool notchinese() const {
   return isSibionics()&&getinfo()->notchinese;
   }
uint8_t siSubtype() const {
   return getinfo()->siType;
   }
int previousstream=-1;

public:
int getmaxmgdL() const {
        if(isDexcom()||isAccuChek())
                return 400;
        if(isSibionics())
                return 450;
         return 500;
        }
int getminmgdL() const {
        return 40;
        }
void setSiAdd2Index(int32_t add) {
        setstarthistory(add );
//        getinfo()->starthistory=add;
        }
int siAddedIndex(int index) const {
        return index+getinfo()->starthistory;
        }

void resetSiIndex();

int updateCali(crypt_t *pass,int sock,int ind,int sensorindex)  {
     uint32_t wascaliNr=getinfo()->caliNr;
     uint32_t updatefrom=getinfo()->caliUpdated[ind];
     if(updatefrom==wascaliNr)
        return 2;
     std::vector<subdata> vect;
     if(updatefrom<wascaliNr) {
        vect.reserve(2);
        int caliStart=offsetof(Info,caliPara)+sizeof(Info::caliPara[0])*updatefrom;
        int caliEnd=offsetof(Info,caliPara) +sizeof(Info::caliPara[0])*wascaliNr;

        vect.push_back({meminfo.data()+caliStart,caliStart,caliEnd-caliStart});
        }
     else {
        vect.reserve(1);
        }
     vect.push_back({reinterpret_cast<const senddata_t *>(&wascaliNr),offsetof(Info,caliNr),4});
    const uint16_t calibratedstartcmd=startcalibratedupdate|sensorindex;
    if(!senddata(pass,sock,vect, infopath,calibratedstartcmd, reinterpret_cast<const uint8_t *>(&updatefrom),sizeof(updatefrom))) {
      LOGAR("updateCali: senddata info.data failed");
      return 0;
     }
     LOGGER("updateCali sock=%d ind=%d sensorindex=%d caliNR=%u updatefrom=%u\n",sock,ind,sensorindex,wascaliNr,updatefrom);
     getinfo()->caliUpdated[ind]=wascaliNr;
     return 1;
 }
int addCali(uint32_t tim,float weight,double a, double b) {
        return getinfo()->addCali(tim,weight,a,b);
        }
int removeCali(uint32_t tim) {
        return getinfo()->removeCali(tim);
        }
bool hide=false;

int getLastIndex() const {
        if(const ScanData *last=lastValidStream()) {
                return last->getid();
                }
        return -1;
        }


bool hasData(uint32_t nu) const {
        if(isAccuChek()) {
            if(pollcount()>=4000)
                    return false;
            }
       else {
        if(isDexcom()) {
            if(pollcount()>=maxdexcount)
                    return false;
            }
          else {
          /*
             if(isLibre3()) {
                    if(pollcount()>=getinfo()->wearduration2)
                        return false;
                    }
              else
              */
              {
                    return false;
                    }
              }
           }
    const uint32_t diff=nu-lastused();
    const bool res= diff < youngsensorsecs;
    LOGGER("hasData nu=%d lastused=%d diff=%d return=%d\n",nu,lastused(),diff,res);
     return res;
    }
//         return ((isAccuChek()&&pollcount()<4000)||(isDexcom()&&pollcount()<maxdexcount))&& (nu-lastused())< youngsensorsecs
};

struct lastscan_t {
    int sensorindex;
    const ScanData *scan;
    uint32_t showtime;
    };
//#endif

