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
/*      Fri Jan 27 12:36:58 CET 2023                                                 */

#pragma once
inline constexpr const int defaulthttpport=17580;
#include <stdint.h>
enum class Insulin: uint8_t {
    Not=0,
    Human,
    Aspart,
    Lispro,
    Glulisine,
    Fiasp,
    URli,
    Afrezza
    };
constexpr auto insulinsNR() {
    return static_cast<uint8_t>(Insulin::Afrezza)+1;
    }

constexpr int maxbluetoothage=11*30;
#define CONV18 1 //Minimally different and 18 fits better
#ifdef CONV18
static constexpr const double convfactor=180.0;
#else
static constexpr const double convfactor=180.182;
#endif
static constexpr const double convertmultmmol=1.0/convfactor;
static constexpr const double convertmultmg=1.0/10.0;
//static constexpr const float convfactor=180.0f;
static constexpr const double convfactordL=convfactor*0.1;
#include <array>
#include "float_t.hpp"

//#include <stdfloat>

#include "curve/nanovg/src/nanovg.h"
typedef  NVGcolor  color_t;

#include "config.h"
#include "inout.hpp"
#include "countryunits.hpp"
#if defined(JUGGLUCO_APP)&& !defined(WEAROS)
#define MAKELABELS 1
#endif

#ifdef MAKELABELS
#include "curve/jugglucotext.hpp"
#endif
#include "broadcasts.hpp"
#include "novopens.hpp"
#include "GlucoseMeter.hpp"
extern int showui;
struct Settings;
extern Settings *settings;
//s/\([0-9a-zA-Z][0-9a-zA-Z]\)/0x\1,/g
constexpr const    uint8_t defaultid[]= {
#ifdef RELEASEID
0xfd,0x29,0x8a,0xf3,0xf2,0xdf,0x4e,0xf0,0x92,0x59,0x3d,0xb8,0x75,0xbb,0xf5,0xc9

#else
0x72,0x12,0xfb,0xfb,0x1f,0x57,0x4b,0xdc,0x84,0x59,0xc0,0x4e,0xfd,0x90,0xa6,0x04

//0x4e,0x94,0xf2,0x4f,0xad,0x8e,0x44,0x73,0x94,0x0c,0x6d,0x62,0xd0,0x1f,0x67,0xdf

#endif
};
inline constexpr const int maxvarnr=40;

extern const char *gformat;
extern int gludecimal;
struct amountalarm {
    float value;
    uint16_t start,alarm,end;
    uint16_t type;
    };
constexpr int maxnumalarms=19;
constexpr int maxuri=128;
struct ring {
    char uri[maxuri];
    uint16_t duration;
    uint16_t wait:12;
    bool novibration:1;
    bool disturb:1;
    bool nosound:1;
    bool flash:1;
    };
constexpr static const int maxcolors=80;
constexpr static const int startbackground=maxcolors/2;
constexpr static const int maxalarms=5;
constexpr static const int maxextraalarms=5;

typedef std::array<char,179> auth_t;
constexpr static const int AUTHMAX=512;
struct authpair {
    auth_t auth;
    uint32_t expires;
    };

constexpr static const int maxprofiles=5;
constexpr static const int maxprofileMins=10;

struct AlarmProfile {
    uint32_t alow,ahigh,averylow,averyhigh,aprelow,aprehigh;
    struct ring alarms[maxalarms+maxextraalarms];
    uint32_t empty[3];

    uint8_t reserved8;
    uint8_t alarmSoundType;
    uint8_t  radius;
    uint8_t  Theme;

    int32_t soundtype;
    float voicespeed;
    float voicepitch;
    uint16_t voicesep:15;
    bool voiceactive:1;
    uint8_t voicespeaker;
    uint8_t restbits:2;
    bool isOval:1;
    bool invertcolors:1;
    bool speakmessages:1;
    bool speakalarms:1;
    bool talktouch:1;
    bool USE_ALARMoff:1;
    bool verylowalarm,prelowalarm,lowalarm,highalarm,prehighalarm,veryhighalarm,availablealarm,lossalarm;
    };
struct  ProfileMin {
    int16_t min;
    int16_t profile;
    };

inline std::pair<uint32_t,uint16_t> startdaynowmin() {
        const time_t tim=time(nullptr);
        struct tm nutm;
        localtime_r(&tim, &nutm);
        uint16_t nu=nutm.tm_hour*60+nutm.tm_min;
        nutm.tm_hour=0;
        nutm.tm_min=0;
        nutm.tm_sec=0;
        time_t startday=mktime(&nutm);
        return {startday,nu};
        }
template <typename Tfunc>
    uint32_t nextalarmtime(int nr, Tfunc atime) {
        const auto [startday,nu]=startdaynowmin();
        /*
        const time_t tim=time(nullptr);
        struct tm nutm;
        localtime_r(&tim, &nutm);
        uint16_t nu=nutm.tm_hour*60+nutm.tm_min;
        nutm.tm_hour=0;
        nutm.tm_min=0;
        nutm.tm_sec=0;
        time_t startday=mktime(&nutm); */
        for(int i=0;i<nr;i++) {
            const auto mins=atime(i);
            if(mins>nu) {
                return (mins*60LL+startday);
                }
            }
        return ((atime(0)+24LL*60LL)*60LL+startday);
        }
struct Tings {
    uint32_t glow,ghigh,tlow,thigh,alow,ahigh;
    int32_t duration;
    uint8_t  camerakey:2;
    bool separatenotify:1;
    bool heartrate:1;
    bool crashed:1;
    bool wakelock:1;
    bool xdripbroadcast:1;
    bool jugglucobroadcast:1;
    char initVersion;
    bool invertcolors:1;
    bool dontshowalways:1;
    bool fixatex:1,fixatey:1,systemUI:1,flash:1,waitwithstreaming:1,nfcsound:1;
    bool havelibrary:1;
    bool xinfuus:1;
    bool levelleft:1;
    bool nolog:1;
    bool postTreatments:1; 
    bool usegarmin:1;
    bool usexdripwebserver:1;
    bool useWearos:1;
    uint8_t orientation:7;
    bool kerfstokblack:1;
    bool hasgarmin:1;
    bool askedNotify:1;
    bool balanced_priority:1;
    uint8_t keepWifi:1;
    bool remotelyxdripserver:1;
    bool shownintro:1;
    bool triedasm:1;
    bool asmworks:1;
    bool nobluetooth;
    bool nodebug:1;
    bool USE_ALARMoff:1;
    bool watchdrip:1;
    bool android13:1;
    bool saytreatments:1;
    bool useSSL:1;
    bool floatingNotTouchable:1;
    bool hour24:1;
    bool lowalarm,highalarm,availablealarm;
    bool lossalarm;
    uint8_t watchid[16];
    struct ToLibre {
        int32_t  kind;
        float weight;
        };
    ToLibre librenums[maxvarnr];
    int64_t libreaccountIDnum;
    uint8_t apisecretlength;
    char apisecret[183];
    int8_t unit;
    bool gadgetbridge;
    bool nochangenum;
    bool sendlabels;
    bool sendcuts;
    int32_t floatingFontsize;
    int32_t  alarmnr;
    amountalarm numalarm[maxnumalarms];
    int32_t floatingforeground;
    int32_t floatingbackground;
    uint32_t update;
    struct Variables {
        float prec;
        float weight;
        char name[12];
        };//20
    int32_t varcount;
    Variables vars[maxvarnr];
    struct Shortcut {
        char name[12];
        char value[12];    
        };
    int32_t shortnr;
    std::array<Shortcut,maxvarnr> shorts;
    int8_t  mealvar;
    uint8_t  dexcomPredict;
    uint16_t nightsensor;
    float roundto;
    color_t colors[maxcolors];
    int colorscreated;
    struct ring alarms[maxalarms];
    char librebaseurl[128];
    char libreemail[256];
    int8_t librepasslen;
    char librepass[36];
    bool libre3nums:1;
    bool sendnumbers:1;
    bool haslibre2:1;
    bool haslibre3:1;
    bool libreinit3:1;
    bool sendtolibreview:1;
    bool uselibre:1;
    bool libreinit:1;
    uint16_t startlibreview;
    std::array<char,36> libreviewDeviceID;
    char _nullchar;
    bool LibreCurrentOnly:1;
    bool nightscoutV3:1;

    bool RTL:1;

    bool libreIsViewed:1;
    bool streamHistory:1;
    bool streamHistLib:1;

    uint8_t  libreunit:2;
    uint16_t startlibre3view;
    uint32_t floatingPos;
    uint32_t lastlibretime;
    std::array<char,36> libreviewAccountID;
    char _nullchar1;
    uint8_t librecountry;
    
    bool showcalibratedhistories:1;
    bool DisconnectSensor:1;

    bool showcalibratedstream:1;
    bool showscans:1;
    bool showstream:1;

    bool showhistories:1;
    bool shownumbers:1;
    bool showmeals:1;

    uint8_t bloodvar;

    float32_t threshold;
    int32_t floatglucose;

    char newYuApiKey[41];
    bool AllValues:1;
    bool DoNotCalibrateA:1;
    bool CalibratePast:1;
    bool DoCalibrate:1;
    bool GoogleScan:1;
    bool logcat:1;
    bool noalarmclock:1;
    bool dontuseclose:1;
    uint16_t libredeletednr;
    int32_t libre2NUMiter;



    uint16_t tokensize;
    char libreviewUserToken[1024];
    int32_t  DateOfBirth;
    uint8_t FirstName[44];
    uint8_t LastName[84];
    uint8_t GuardianLastName[44];
    uint8_t GuardianFirstName[84];
    char libre3baseurl[128];
    std::array<char,36> libre3viewDeviceID;
    char _nullchar2;

    uint8_t voicespeaker;
    uint16_t voicesep:15;
    bool voiceactive:1;
    float voicespeed;
    float voicepitch;

    char newYuApiKey3[41];

    bool hidefloatinJuggluco:1;//*
    bool floattime:1;
    bool currentRelative:1;//*
    bool IOB:1;
    bool healthConnect:1;

    bool speakmessages:1;
    bool speakalarms:1;
    bool talktouch:1;

    uint16_t sslport;
    int32_t libre3NUMiter;

    uint16_t tokensize3;
    char libreviewUserToken3[1024];
   union {
      struct {
         BroadcastListeners<2> librelinkBroadcast;
         BroadcastListeners<3> everSenseBroadcast;
         BroadcastListeners<10> xdripBroadcast;
         BroadcastListeners<10> glucodataBroadcast;
         };
   struct {
         BroadcastListeners<5> librelinkBroadcastOld;
         BroadcastListeners<10> xdripBroadcastOld;
         BroadcastListeners<10> glucodataBroadcastOld;
         };
      };
    int nightuploadnamelen;
    char nightuploadname[256+8];
    char nightuploadsecret[80];
    bool nightuploadon;
    int pensnr;
    std::array<NovoPen,maxpennr>  pens;
    ToLibre Nightnums[maxvarnr];
    int32_t nightinterval;
    uint32_t timenumchanged;
    uint32_t lastuploadtime;
    uint32_t authstart;
    uint32_t authend;
    authpair authdata[AUTHMAX];
    int32_t WasTheme;
    uint64_t jugglucoID;
    uint32_t startlibretime;

   int32_t ComplicationArrowColor;
    int32_t ComplicationTextColor;
    int32_t ComplicationTextBorderColor;
    int32_t ComplicationBackgroundColor;



    Insulin insulintypes[maxvarnr];
    int32_t iobupdate;

    uint32_t averylow,averyhigh,aprelow,aprehigh;

    int16_t currentProfile,nrProfile,nrProfileMins;

    bool verylowalarm,veryhighalarm,prelowalarm,prehighalarm;
    uint16_t httpport;
    AlarmProfile  profiles[maxprofiles];

    ProfileMin profileMins[maxprofileMins];

    struct ring extraAlarms[maxextraalarms];
    int32_t soundtype;
    uint8_t alarmSoundType;
    uint8_t radius;
    uint8_t Theme;
    uint8_t reserved5:1;
    bool rotateText:1;
    bool Rotate:1;
    bool isOval:1;
    bool timeOnComplication:1;
    bool askedWatchFace:1;
    bool   lossSignalOff:1;
    bool  showcalibratedscans:1;
    float64_t loadtime;
    uint32_t glucoseMeterNR;
    int8_t notevar;
    int8_t reserved4[3];
    GlucoseMeter  glucosemeters[maxglucosemeters];
    uint8_t gs3id[12];
    uint32_t reserved32;

static bool meterMatch(const struct GlucoseMeter &meter,const std::string_view deviceName,uint8_t *address)  {
    if(address) {
        static constexpr const uint8_t zero[6]{};
        if(memcmp(zero,meter.deviceAddress,6))
            return !memcmp(address, meter.deviceAddress,6);
        }
    const int len=std::min(static_cast<int>(deviceName.size())+1, maxDeviceName);
    return !memcmp(deviceName.data(),meter.deviceName,len) ;
    }


void removeGlucoseMeterIntern(const int i)  {
    const int next=i+1;
    const int left=glucoseMeterNR-next;
    if(left>0) {
            memmove(glucosemeters+i,glucosemeters+next,left*sizeof(GlucoseMeter));
            }
     --glucoseMeterNR;
     glucosemeters[glucoseMeterNR]={};
     }

bool removeGlucoseMeter(const int i)  {
    if(i>=glucoseMeterNR)
        return false;
     removeGlucoseMeterIntern(i); 
     return true;
     }


bool removeGlucoseMeter(const std::string_view deviceName,uint8_t *address)  {
    const int tot=glucoseMeterNR; 
    for(int i=0;i<tot;++i) {
        if(meterMatch(glucosemeters[i],deviceName,address)) {
            removeGlucoseMeterIntern(i);
            return true;
            }
        }
     return false;
     }




template <typename Self>
auto *getGlucoseMeter(this Self&& self,int index)  {
    decltype(&self.glucosemeters[0]) meter=nullptr;
    if(index>=0&&index< self.glucoseMeterNR) {
        meter=&self.glucosemeters[index];
        }
    return meter;
    }
template <typename Self>
auto *getGlucoseMeter(this Self&& self,const std::string_view deviceName,uint8_t *address)  {
    const int tot=self.glucoseMeterNR; 
    for(int i=0;i<tot;++i) {
        if(meterMatch(self.glucosemeters[i],deviceName,address)) {
            return &self.glucosemeters[i];
            }
        }
    return static_cast<decltype(&self.glucosemeters[0])>(nullptr);
     }
template <typename Self>
auto *newGlucoseMeter(this Self&& self,std::string_view deviceName,uint8_t *address)  {
    const int len=std::min(static_cast<int>(deviceName.size())+1, maxDeviceName);
    const int nr=self.glucoseMeterNR;
    if(nr<maxglucosemeters) {
        memcpy(self.glucosemeters[nr].deviceName,deviceName.data(),len);
        if(address)
               memcpy(self.glucosemeters[nr].deviceAddress,address,6);
        self.glucoseMeterNR=nr+1;
        return &self.glucosemeters[nr];
        }
    return static_cast<decltype(&self.glucosemeters[0])>(nullptr);
    }
template <typename Self>
auto *giveGlucoseMeter(this Self&& self,std::string_view deviceName,uint8_t *address)  {
     auto *meter=self.getGlucoseMeter(deviceName,address);
     if(!meter)
        meter=self.newGlucoseMeter(deviceName,address);
     return meter;
     }
void defaultshows() {
    showcalibratedstream=false;
    showcalibratedhistories=false;
    showscans=true;
    showstream=true;
    showhistories=true;
    shownumbers=true;
    showmeals=false;
    bloodvar=maxvarnr;
    httpport=defaulthttpport;
    }
 void        mkadvancedalarms() {
    struct ring *al=extraAlarms;
    for(int i=0;i<maxextraalarms;i++) {
        al[i].uri[0]='\0';
        al[i].wait=20;
        al[i].duration=0xFFFF;
        }
    averylow=550;
    averyhigh=14*180;
    aprelow=39*18;
    aprehigh=13*180;
    soundtype=
#ifdef WEAROS
    13
#else
    5  
#endif
    ;
    }

    void setdefault() {
        memcpy(watchid,defaultid,sizeof(watchid));
        };
        /*
    bool isLibreMmolL() {
        if(!libreunit) libreunit=unit==1?1:2;
        return unit==1;
        } */
    bool isLibreMmolL() {
        return !(getLibreCountry()&1);
        }
    int  getLibreCountry() {
        if(!librecountry||librecountry>5) 
            librecountry=unit==1?1:2;
        return librecountry-1;
        }



struct ring &getalarmringtone(int type) {
    if(type==3||currentProfile<1) {
        if(type<maxalarms)
            return alarms[type];
        else
            return extraAlarms[type-maxalarms];
        }
    return profiles[currentProfile-1].alarms[type];
    }

template <typename T>
  T &alarmget(T &at,T AlarmProfile::*ap) {  
        if(currentProfile>0) {
            const int profindex=currentProfile-1;
            return profiles[profindex].*ap;
            }
        else
            return at;
        }
   

#define maketalk(type)\
  auto &type##Get() {  \
    return alarmget(type,&AlarmProfile::type);\
    }
maketalk(Theme)
maketalk(radius)
maketalk(soundtype)
maketalk(voicespeed)
maketalk(voicepitch)
maketalk(voicespeaker)
#define makebitvoice(type)\
    auto type##get() const {\
        if(currentProfile>0) {\
            const int profindex=currentProfile-1;\
            return profiles[profindex].type;\
            }\
        else\
            return type;\
        }\
    void type##set(decltype(type) val) {\
        if(currentProfile>0) {\
            const int profindex=currentProfile-1;\
            profiles[profindex].type=val;\
            }\
        else\
            type=val;\
        }

makebitvoice(voicesep)
makebitvoice(voiceactive)
makebitvoice(speakmessages)
makebitvoice(speakalarms)
makebitvoice(talktouch)
makebitvoice(USE_ALARMoff)

makebitvoice(alarmSoundType)

makebitvoice(invertcolors)
makebitvoice(isOval)

#define mkhasalarm(type)\
bool &has##type##alarm() {\
    return alarmget(type##alarm,&AlarmProfile::type##alarm);\
    } \
const  bool has##type##alarm() const {  \
       return const_cast<Tings*>(this)->has##type##alarm();\
        } 

#define mkalarmget(type)\
  uint32_t &a##type##get() {  \
    return alarmget(a##type,&AlarmProfile::a##type);\
    }\
 mkhasalarm(type)\
const  uint32_t a##type##get() const {  \
       return const_cast<Tings*>(this)->a##type##get();\
        }

mkalarmget(low)
mkalarmget(verylow)
mkalarmget(prelow)
mkalarmget(high)
mkalarmget(veryhigh)
mkalarmget(prehigh)
mkhasalarm(loss)
mkhasalarm(available)

#define setproel(x)  prof.x=x;
void newprofile(int nr) {
   for(int prnr=nrProfile;prnr<nr;++prnr) {
        struct AlarmProfile  &prof=profiles[prnr];
        memcpy(prof.alarms,alarms,maxalarms*sizeof(alarms[0]));
        memcpy(prof.alarms+maxalarms,extraAlarms,maxextraalarms*sizeof(extraAlarms[0]));

        prof.alow=alow;
        prof.averylow=averylow;
        prof.aprelow=aprelow;
        prof.ahigh=ahigh;
        prof.averyhigh=averyhigh;
        prof.aprehigh=aprehigh;
        setproel(verylowalarm) setproel(prelowalarm) setproel(lowalarm) setproel(highalarm) setproel(prehighalarm) setproel(veryhighalarm) setproel(availablealarm) setproel(lossalarm) 

        setproel( voicespeed)
        setproel( voicepitch)
        setproel( voicesep)
        setproel( voiceactive)
        setproel( voicespeaker)
        setproel( speakmessages)
        setproel( speakalarms)
        setproel( talktouch)
        setproel( USE_ALARMoff)
        setproel( invertcolors)
        setproel( Theme)
        setproel( isOval)
        setproel( radius)
        }
     nrProfile=nr;
    }
static auto randomTheme() {
    return arc4random_uniform(55); 
    }
static auto randomRadius() {
    return arc4random_uniform(18); 
    }
void darkmode2profile() {
   Theme=randomTheme();
   radius=randomRadius();
   
   for(int prnr=0;prnr<nrProfile;++prnr) {
        struct AlarmProfile  &prof=profiles[prnr];
        setproel(invertcolors)
        prof.isOval=false;
        prof.Theme=randomTheme();
        prof.radius=randomRadius();
        }
    }
void toAlarmType() {
   alarmSoundType=USE_ALARMoff;
   for(int prnr=0;prnr<nrProfile;++prnr) {
        struct AlarmProfile  &prof=profiles[prnr];
        prof.alarmSoundType=prof.USE_ALARMoff;
        }
    }
bool setprofile() {
       const int nr=nrProfileMins;
       if(nr<=0)  {
           LOGAR("setprofile nrProfileMins=0");
            return false;
            }
        const time_t tim=time(nullptr);
        struct tm nutm;
        localtime_r(&tim, &nutm);
        const uint16_t nu=nutm.tm_hour*60+nutm.tm_min;
        for(int i=nr-1;i>=0;--i) { //std::upper_bound?
            if(profileMins[i].min<=nu) {
                currentProfile=profileMins[i].profile;
                LOGGER("setprofile: min=%d profile=%d\n",profileMins[i].min,currentProfile);
#ifndef DONTTALK
                extern bool speakout;
                speakout=talktouchget();
#endif
                return true;
                }
           } 
        LOGAR("setprofile: no profile activated");
        return false;
        } 

    uint32_t nextprofiletime() const {
        const int nr=nrProfileMins;
        if(nr<=0)
            return 0;
        return nextalarmtime(nr,[mins=profileMins](const int index) {return mins[index].min;});
        }

void setAdvancedAlarms(uint32_t verylow1, uint32_t veryhigh1, bool verylowalarm1, bool veryhighalarm1, bool prelowalarm1,bool prehighalarm1,
                       uint32_t prelow1, uint32_t prehigh1) {
        averylowget()=verylow1;
        averyhighget()=veryhigh1;
        hasverylowalarm()=verylowalarm1;
        hasveryhighalarm()=veryhighalarm1;
        hasprelowalarm()=prelowalarm1;
        hasprehighalarm()=prehighalarm1;
        aprelowget()=prelow1;
        aprehighget()=prehigh1;
        }
void removeProfileMin(int index) {
    if(index>=0&&index<nrProfileMins) {
        int len=--nrProfileMins-index;
        if(len<=0)
            return;
        memmove(profileMins+index,profileMins+index+1,len*sizeof(profileMins[0]));
        }
    }
int addProfileMin(int min,int profile) {
    if(nrProfileMins>=maxprofileMins)
        return -1;
    struct  ProfileMin zoek{
        .min=(int16_t)min
        };
    auto *beg=profileMins;
    auto *end=profileMins+nrProfileMins++;
    auto *hit=std::lower_bound(beg,end, zoek,[](const ProfileMin one, const ProfileMin two) {return one.min<two.min;});
    if(hit<end) {
        memmove(hit+1,hit,(uint8_t*)end-(uint8_t*)hit);
        }
    hit->min=min;
    hit->profile=profile;
    return hit-beg;
    }
int changeProfileMin(int index,int min,int profile) {
    removeProfileMin(index);
    return addProfileMin(min,profile);
    }
    };

struct Settings:Mmap<Tings> {
double convertmult;
/*
    public static final int SCREEN_ORIENTATION_LANDSCAPE = 0;
    public static final int SCREEN_ORIENTATION_PORTRAIT = 1;
  public static final int SCREEN_ORIENTATION_REVERSE_LANDSCAPE = 8;
    public static final int SCREEN_ORIENTATION_REVERSE_PORTRAIT = 9;
    */


int error=0;
Settings(string_view base, string_view file,const char *country): Settings(pathconcat(base,file).data(),base.data(),country) {
    }



int errnotoerror(int errn) {
    switch(errn) {
    case EACCES: return 5;

    case ELOOP: return 6;

    case ENAMETOOLONG: return 7;

    case ENOENT: return 8;

    case ENOTDIR: return 9;

    case EROFS: return 10;

    case EBADF: return 11;

    case EINVAL: return 12;

    case ETXTBSY: return 13;
    default: return 3;
    }
    }

#ifdef MAKELABELS
void mkshorts() {
//    int len= std::size(shortinit);
    int len=usedtext->shortinit.size();
    for(int i=0;i<len;i++) {
        snprintf(data()->shorts[i].value,12,"%.10g",usedtext->shortinit[i].value);
        strcpy( data()->shorts[i].name,usedtext->shortinit[i].name);
        }
    data()->shortnr=len;
    LOGGER("shorts=%d\n",data()->shortnr);
    }
void mklabels() {
    LOGSTRING("mklabels\n");
    Tings::Variables *varsptr=data()->vars;
    varsptr[0].prec=1.0f;
    varsptr[1].prec=1.0f;
    varsptr[2].prec=.5f;
    varsptr[3].prec=.5f;
    varsptr[4].prec=1.0f;
    varsptr[5].prec=1.0f;
    varsptr[6].prec=.1f;
//    varsptr[6].weight=tomgperL(1.0f);

//    strcpy( data()->vars[i].name,labels[i].data());
    int nrlab=usedtext->labels.size();
    for(unsigned int i=0;i<nrlab;i++) {
        strcpy( varsptr[i].name,usedtext->labels[i].data());
    }
    data()->mealvar=0;
    data()->varcount=nrlab;
    mkshorts() ;
}
#else
#define mklabels() 
#endif
private:
void movecast(BroadcastListeners<10> &in,BroadcastListeners<10> &out) {
   const int len=in.nr;
   LOGGER("movecast len=%d\n",len);
   for(int i=len-1;i>=0;--i) {
      LOGGER("movecast IN %s\n",in.name[i]);
      const auto slen=strlen(in.name[i]);
      memmove(out.name[i],in.name[i],slen);
      LOGGER("movecast %s\n",out.name[i]);
      }
   out.nr=len;
   };
void  movebroadcast() {
    LOGAR("movebroadcast");
   if(data()->librelinkBroadcast.nr>data()->librelinkBroadcast.getmax()) data()->librelinkBroadcast.nr=data()->librelinkBroadcast.getmax();
   movecast(data()->glucodataBroadcastOld,data()->glucodataBroadcast);
   movecast(data()->xdripBroadcastOld,data()->xdripBroadcast);
   };
public:
Settings(const char *settingsname,const char *base,const char *country): Mmap(settingsname,1) {
//Settings(string_view base, string_view file,const char *country): settingsfilename(base,file), Mmap(base,file,1) 
    if(!data())  {
        if(access(base, R_OK|W_OK)!=0) {
            error=errnotoerror(errno);
            }
        else {
            if(access(settingsname, R_OK|W_OK)!=0)
                error=4;
            else
                error=2;
            }
        return;
        }

//    if(data()->initVersion<30) 
   { 
    LOGGER("initVersion=%d\n",data()->initVersion);
    if(data()->initVersion<38) {
    if(data()->initVersion<37) {
       if(data()->initVersion<35) { 
       if(data()->initVersion<34) { 
       if(data()->initVersion<33) { 
        if(data()->initVersion<31) { 
            if(data()->initVersion<26) { 
              if(data()->initVersion<22) { 
              if(data()->initVersion<20) {
              if(data()->initVersion<18) { // set in Applic.initbroadcasts, startjuggluco and initinjuggluco 
              if(data()->initVersion<17) { 
                   memcpy(data()->Nightnums,data()->librenums, sizeof(Tings::ToLibre)*data()->varcount);
                 if(data()->initVersion<16) { 
                 if(data()->initVersion<15) {
                    if(data()->initVersion<13) {
                       if(data()->initVersion<12) {
                       if(data()->initVersion<10) {
                       if(data()->initVersion<9) {
                          data()->sendtolibreview=data()->uselibre;
                          if(data()->initVersion<8) {
                             if(data()->initVersion<7) { 
                                if(data()->initVersion<6) {
                                   if(data()->initVersion<4) {
                                      if(data()->varcount==0) {
                                         data()->roundto=1.0f;
                                         data()->update=1;
                                         mklabels();
                                         mkalarms();
                                         data()->logcat=true;
                          #ifdef WEAROS
                                         data()->orientation=1;
                          #else
                                         data()->orientation=8;
                          #endif

                                         data()->fixatey=true;
                                         data()->systemUI=true;
                                         }
                                      data()->setdefault();
                                      data()->streamHistory=true;
                                      };
                                   data()->flash=false;
                                   }
                                data()->usexdripwebserver=false;
                          #ifdef CARRY_LIBS
                                data()->havelibrary=true;
                          #endif
                          #ifdef WEAROS
                                data()->invertcolors=true;
                                data()->usegarmin=false;
                          #else
                                data()->usegarmin=true;
                          #endif
                                }

                             data()->balanced_priority=true;
                             }
                          }
                          data()->triedasm=false;
                          data()->asmworks=false;
                          }

                          if(data()->libre2NUMiter<1) data()->libre2NUMiter=1;
                          if(data()->libre3NUMiter<1) data()->libre3NUMiter=1;
                          }
                       if(data()->xinfuus) {
                          strcpy(data()->librelinkBroadcast.name[0], "com.eveningoutpost.dexdrip");
                          data()->librelinkBroadcast.nr=1;
                          }
                       data()->xdripBroadcast.nr=data()->xdripbroadcast;
                       data()->glucodataBroadcast.nr=data()->jugglucobroadcast;
                       }

                    data()->libreaccountIDnum=-1LL;
                      }
                    data()->sslport=17581;
                      }
                    }

                  data()->libreinit=0; //reinit during switch to 2.10.1
                    }
                  if(country&&!strcasecmp(country,"RU")) {
                    data()->librecountry=5;
                    }
                 else
                     data()->librecountry = data()->libreunit;
                 }
                 data()->nightinterval=270;
                 }
                 movebroadcast();
           
                 }
    #if defined(JUGGLUCO_APP)&& !defined(WEAROS)
             setIOBtype();
    #endif
    #if !defined(__x86_64__) && defined(__i386__)
            uint32_t  *startptr=&data()->startlibretime;
            int32_t  *endptr=&data()->ComplicationBackgroundColor;
            auto start=reinterpret_cast<uint8_t*>(startptr);
            memmove(start,start-4,reinterpret_cast<uint8_t*>(endptr)-start+4);
    #endif
            }
          else {
    #if defined(__arm__) && !defined(WEAROS)
    const time_t now=time(nullptr);
    if(data()->startlibretime>now) {
            uint32_t  *startptr=&data()->startlibretime;
            auto start=reinterpret_cast<uint8_t*>(startptr);
            memmove(start,start+4,4);
            if(data()->startlibretime>now||data()->startlibretime<1741958880)
                data()->startlibretime=1742218080;
            }

    #endif
        }
        data()->mkadvancedalarms();
        }

        data()->loadtime=10.0/(3000.0*3000.0);
          }
         data()->defaultshows();
         }
      data()->darkmode2profile();
      }
      data()->toAlarmType();
      }
   }
   
#if 0&&defined(WEAROS)&&!defined(NOLOG)
        data()->logcat=true;
#endif
    setconvert(country);

     showui=getui();
    if(!data()->httpport)
        data()->httpport=defaulthttpport;
}




 void        mkalarms() {
    struct ring *al=data()->alarms;
    for(int i=0;i<maxalarms;i++) {
        al[i].uri[0]='\0';
        al[i].wait=20;
        }
    al[0].duration=0xFFFF; //Low glucose alarm

    al[1].duration=0xFFFF; //High glucose alarm

    al[2].duration=3; //Availability notification

    al[3].duration=0xFFFF; //amount alarm

    al[4].duration=0xFFFF; //Loss of sensor alarm
    }

void setnodebug(bool val) {
    data()->nodebug=val;
    data()->havelibrary=true;
    }


bool staticnum() const ;
bool getnodebug() const {
    return data()->nodebug;
    }
void setconvert(const char *country) {
    int unit=data()->unit;

    if(unit==0) {
        if(country&&*country)
            unit=getunit(country);
        else
            unit=3;
        setalarms(39*18,13*180,true,true,true,true);
        setranges(3*180,12*180,39*18,10*180);
        }
    else {
        LOGGER("setconvert was unit=%d\n",unit);
        }
    setunit(unit);
    }
void setlinuxcountry();
uint32_t graphlow() const {
    return data()->glow;
    }
uint32_t graphhigh() const {
    return data()->ghigh;
    }
uint32_t targetlow() const {
    return data()->tlow;
    }
uint32_t targethigh() const {
    return data()->thigh;
    }
bool usemmolL() const {
    return data()->unit==1;
    }
void setunit(int unit) {
    LOGGER("setunit(%d)\n",unit);
    if((data()->unit=unit)==1) {
        convertmult= convertmultmmol;
        gformat="%.1f";
        gludecimal=1;
        }
    else  {
        convertmult= convertmultmg;
        gformat="%.0f";
        gludecimal=0;
        }

    }
bool availableAlarm() const {
    return data()->hasavailablealarm();
    }
bool highAlarm(int val) const {
    if(data()->hashighalarm()&&val>data()->ahighget())
        return true;
    return false;
    }
bool lowAlarm(int val) const {
    if(data()->haslowalarm()&&val<data()->alowget())
        return true;
    return false;
    }
bool veryhighAlarm(int val) const {
    LOGGER("veryhighalarm(%d) has=%d threshold=%d\n",val,data()->hasveryhighalarm(),data()->averyhighget());
    if(data()->hasveryhighalarm()&&val>data()->averyhighget())
        return true;
    return false;
    }
bool verylowAlarm(int val) const {
    if(data()->hasverylowalarm()&&val<data()->averylowget())
        return true;
    return false;
    }
static float preval(int val,float rate)  {
        return val+rate*.4f*180.0f;
        }
bool prehighAlarm(int val,float rate) const { 
    if(data()->hasprehighalarm()&&preval(val,rate)>data()->aprehighget()) 
        return true;
    return false;
    }
bool prelowAlarm(int val,float rate) const { 
    if(data()->hasprelowalarm()&&preval(val,rate)< data()->aprelowget())
        return true;
    return false;
    }
void setranges(uint32_t glow, uint32_t ghigh, uint32_t tlow, uint32_t thigh) {
    data()->glow=glow;data()->ghigh=ghigh;data()->tlow=tlow;data()->thigh=thigh;
    }
void setalarms(uint32_t alow, uint32_t ahigh, bool lowalarm, bool highalarm, bool availablealarm,bool lossalarm) {
    data()->alowget()=alow;data()->ahighget()=ahigh;
    data()->haslowalarm()=lowalarm;
    data()->hashighalarm()=highalarm;
    data()->haslossalarm()=lossalarm;
    data()->hasavailablealarm()=availablealarm;
    }


//constexpr static string_view labels[]={"NovoRapid","Carbohydrat","Dextro","Levemir","Fietsen","Lopen","Blood"};
//constexpr static string_view labels[]={"Insulin Rap","Carbohydrat","Dextro","Insulin Slow","Bike","var6","var7","var8"};
/*
constexpr static string_view labels[]={"Aspart","Carbohydrat","Dextro","Levemir","Bike","Walk","Blood","var8"};
constexpr static struct {const char name[12];const float value;}  shortinit[]= { {"Bread",
        .376f},
        {"Currantbun1",
        .56f},
        {"Currantbun2",
        .595f},
        {"Grapes",
        .165f},
        {"FruitYog",
        .058f},
        {"Rice",
        .75f},
        {"Macaroni",
        .65f},
        {"Tomato",
        .03f},
        {"Mexican mix",
        .078f},
        {"OrangeJuice",
        .109f},
        {"SportPowder",
        .873f},
        {"Mix(Carrot)",
        .07f},
        {"Mix mushro",
        .07300000f}};

struct Shortcut_t {const char name[12];const float value;} 

constexpr static string_view itlabels[]= {"Rapida","Carboidrati","Glucosio","Lenta","Bike","Walk","Capillare","var8"};
constexpr static struct {const char name[12];const float value;}  itshortinit[]= { {"Muffin", .54f},

{"Uva",

.165f},

{"YogFrutta",

.058f},

{"Riso", .75f},

{"Pasta", .65f},

{"Pomodoro",

.03f},

{"Messicano",

.078f},

{"SuccoArancia",

.109f},

{"Mix(Carote)",

.07f},

{"Mix funghi",

.07300000f}};
*/

/*
void mklabels() {
for(int i=0;i<8;i++) {
    snprintf( data()->vars[i].name,12,"var%d",i);
    }

data()->varcount=8;
}
*/
int getlabelcount()const {
    return data()->varcount;
    }
int getshortcutcount()const {
    return data()->shortnr;
    }
void setshortcutcount(int nr) {
    data()->shortnr=(nr<data()->shorts.size())?nr:(data()->shorts.size()-1);
    data()->sendcuts=true;
    }


static constexpr const string_view unknownlabel{"Unspecified"};
const string_view getlabel(const int index) const  {
    if(index>=0&&index<getlabelcount())
        return data()->vars[index].name;
    return unknownlabel;
    }
void setlabel(const int index,const char *name)   {
    strncpy(data()->vars[index].name,name,11);
    data()->sendlabels=true;
    }
void setlabel(const int index,const char *name,float prec,float weight)   {
    strncpy(data()->vars[index].name,name,11);
    data()->vars[index].prec=prec;
    data()->vars[index].weight=tomgperL(weight);
    if(index>=varcount())
        varcount()=index+1;
    data()->sendlabels=true;
    }
int &varcount() {
    return data()->varcount;
    }
void setlabel(const char *name)   {
    strncpy(data()->vars[varcount()++].name,name,11);
    data()->sendlabels=true;

    }

const float getlabelweight(const int index) const  {
    return frommgperL(data()->vars[index].weight);
    }
const float getlabelweightmgperL(const int index) const  {
    return data()->vars[index].weight;
    }
const float getlabelprec(const int index) const  {
    return data()->vars[index].prec;
    }
float tomgperL(const float unit) const {
    return unit/convertmult;
    }
float frommgperL(const float mgperL) const {
    return mgperL*convertmult;
    }
string_view getunitlabel() const {
    return data()->unit==1?unitlabels[1]:unitlabels[2];
    }

bool getui()const  {
    return data()->systemUI;
    }
void setui(bool showui) {
    data()->systemUI=showui;
    }

const    uint32_t getupdate()const  {
        return data()->update;
        }
void updated()  {
         data()->update++;
        }

uint32_t firstAlarm()const  ;
std::vector<int> numAlarmEvents() const;
void setnumalarm(uint16_t type,float value,uint16_t start,uint16_t alarm) ;
void delnumalarm(int pos) ;

Insulin getIOBtype(uint32_t type) const {
    if(type>=getlabelcount())
        return Insulin::Not;
    return data()->insulintypes[type];
    }

void  setIOBtype();
void setusebluetooth(bool on) {
        LOGGER("settings->setusebluetooth(%d)\n",on);
        data()->nobluetooth=!on;
        }
};

inline float gconvert(const float mgperL) {
    return settings->frommgperL(mgperL);
    }
/*
float frommgperL(const float mgperL) const {
    return mgperL*convertmult;
    }
    */
inline float gconvert(const float mgperL,int unit) {
    if(unit==1)
        return convertmultmmol*mgperL;
    return mgperL*convertmultmg;
    }
inline int getgludecimal(int init) {
    return init==1;
    }
inline  const float backconvert(const float unit) {
    return settings->tomgperL(unit);
    }
inline constexpr const char settingsdat[]="settings.dat";
inline bool waitstreaming() {
    return settings->data()->waitwithstreaming;
    }

inline uint16_t &getlibrenumsdeletednr() {
    return settings->data()->libredeletednr;
    }

inline constexpr const char *getunitstring(int unit) {
        return unit==1?"mmol/L":"mg/dL";
        }
inline constexpr const char *getunitformat(int unit) {
        return unit==1?"%.1f":"%.0f";
        }

#ifdef NDK_DEBUG
constexpr const int librekeepsecs=3000*24*60*60;
#else
constexpr const int librekeepsecs=89*24*60*60;
#endif
