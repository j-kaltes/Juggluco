
//#define DONTTALK WEAROS

#ifdef WEAROS
#define NOLEFT
#define NOCUTOFF 1
#endif

#ifndef NOLOG
//#define TEST 1
#endif
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
/*      Fri Jan 27 15:20:04 CET 2023                                                 */



//#include <jni.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <algorithm>
//#include <filesystem>
#include <math.h>
#include <cstdint>
#include <cinttypes>
#include <charconv>
#include <string>
#include <new>
using namespace std::literals;
//#include "glucose.hpp"
//ScanData   *glucosenow=nullptr;
#ifdef JUGGLUCO_APP
#define FATAL(...)  LOGAR(__VA_ARGS__)
#else
#define FATAL(...)  fprintf(stderr,__VA_ARGS__)
#endif
#define CURVELOGGER(...) LOGGER("curve: " __VA_ARGS__)
#define CURVELOGAR(...) LOGAR("curve: " __VA_ARGS__)


#include "curve.hpp"
#include "scriptFonts.hpp"
#include "config.h"
//#define FILEDIR "/sdcard/libre2/"
//#include "Glucograph.h"
#include "logs.hpp"
//#define CURVELOGGER(...)  __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#include "settings/settings.hpp"

#include "SensorGlucoseData.hpp"
#include "sensoren.hpp"
#include "nums/numdata.hpp"
#include "nfcdata.hpp"

#include "error_codes.h"
#include "jugglucotext.hpp"
#include "JCurve.hpp"
#include "misc.hpp"
#include "calibrate/Calibrator.hpp"
#include "calibrate/Calibrate.hpp"
static bool getLevelLeft() {
#ifdef NOLEFT
    return false;
#else
    return settings->data()->levelleft;
#endif
    }
#ifdef DONTTALK
extern const bool speakout;
const bool speakout=false;
#else
extern std::vector<shownglucose_t> shownglucose;
std::vector<shownglucose_t> shownglucose;
extern bool speakout;
bool speakout=false;
#endif
        extern bool hasnetwork();

extern int bluePermission();
extern bool bluetoothEnabled();
//extern NVGcolor invertcolor(const NVGcolor *colin) ;

    
void cpcolors(NVGcolor *foreground) {
    int wholes=nrcolors/oldnrcolors;
    for(int i=1;i<wholes;i++) 
        memcpy(foreground+i*oldnrcolors,foreground,oldnrcolors*sizeof(foreground[0]));
    if(int left=nrcolors%oldnrcolors) {
        memcpy(foreground+wholes*oldnrcolors,foreground,left*sizeof(foreground[0]));
        }
    }


void createcolors() {
    NVGcolor *foreground=settings->data()->colors;
    NVGcolor *background=settings->data()->colors+startbackground;
    if(!settings->data()->colorscreated) {
        memcpy(foreground,allcolors,sizeof(allcolors));
    //    foreground[darkgrayoffset]=  nvgRGBAf(0,0,0,0.4);
        foreground[dooryellowoffset]=  nvgRGBAf2(0.9,0.9,0.1,0.3); 
        foreground[lightredoffset]=  nvgRGBAf2(1, 0.95, 0.95, 1); 
        foreground[grayoffset]=  nvgRGBAf2(0,0,0,0.1);

//        for(int i=0;i<std::size(allcolors);i++)  
        for(int i=0;i<oldnrcolors;i++)  {
            background[i]=invertcolor(foreground+i);
            }
        background[darkgrayoffset]=   nvgRGBAf2(.8,.8,.8,.8);
        background[dooryellowoffset]=  nvgRGBAf2(0.9,0.9,0.1,0.3);
//        background[lightredoffset]=   nvgRGBA(65, 65, 65, 255); 
        background[grayoffset]= {{{1.0f,1.0f,1.0f,.4f}}}; 
//        background[redoffset]= nvgRGBAf2(1.0,0,0,1.0);
        }
    if(settings->data()->colorscreated<3) {
        foreground[threehouroffset]=  nvgRGBAf2(1.0,0,1,0.5);
        background[threehouroffset]=  nvgRGBAf2(1.0,0,1,1);
        }
    if(settings->data()->colorscreated<5) {
        cpcolors(foreground);
        cpcolors(background);
        }
    if(settings->data()->colorscreated<15) {
        background[lightredoffset]=   blackbean; 
        settings->data()->colorscreated=15;
        }
    }


#ifdef MENUARROWS
// pyftsubset <font-file> --unicodes=  --output-file=<path>
#include "fonts.h"
#endif
NVGcontext* genVG=nullptr;


extern bool fixatex,fixatey;
int showui=false;

static enum FontType {
    CHINESE,
    HEBREW,
    ARABIC,
    REST
    } chfontset=REST;
//static bool chfontset=false;

bool hebrew() ;
#ifdef JUGGLUCO_APP
#define fontpath "/system/fonts/"
#else
#define fontpath "/home/jka/Android/Sdk/platforms/android-29/data/fonts/"
#endif
extern jugglucotext zhtext; 
extern jugglucotext artext; 


#ifdef JUGGLUCO_APP
static int getWhiteFont(NVGcontext* avg) {
    return nvgCreateFont(avg, "regular", 
#ifdef JUGGLUCO_APP
    fontpath "Roboto-Regular.ttf"
#else
"/usr/share/fonts/truetype/roboto-fontface/roboto/Roboto-Regular.ttf"
//"/usr/share/fonts/truetype/noto/NotoSerif-Regular.ttf"
//"/usr/local/Wolfram/Wolfram/14.2/SystemFiles/Fonts/TrueType/Roboto-Regular.ttf"
#endif
    );
    }
static int getMenuFont(NVGcontext* avg) {
        constexpr const char menufonts[][sizeof(fontpath "SourceSansPro-SemiBold.ttf")]={
        fontpath "Roboto-Medium.ttf",
        fontpath "SourceSansPro-SemiBold.ttf",
        fontpath "NotoSerif.ttf",
        fontpath "SourceSansPro-Regular.ttf",
        fontpath "Roboto-Regular.ttf",
        fontpath "DroidSans.ttf"
        };
            int onefont;
            for(const char *name:menufonts)  {
                if((onefont = nvgCreateFont(avg, "regular", name))!=-1) {
                    CURVELOGGER("menufont %s succeeded\n",name);
                    break;
                    }
                CURVELOGGER("menufont %s failed\n",name);
                }
        return onefont;
        }
#endif
static int getBlackFont(NVGcontext* avg) {
        int blackfont=-1;
        constexpr const char standardfonts[][sizeof(
        #ifdef JUGGLUCO_APP
        fontpath "SourceSansPro-SemiBold.ttf"
        #else
        "/usr/share/fonts/truetype/roboto-fontface/roboto/Roboto-Regular.ttf"

        #endif
        )]= {
        #ifndef JUGGLUCO_APP
        "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
        "/usr/share/fonts/truetype/roboto-fontface/roboto/Roboto-Regular.ttf",
        "/usr/share/fonts/truetype/noto/NotoSerif-Regular.ttf",
        #else
        fontpath "Roboto-Black.ttf",
        fontpath "SourceSansPro-Bold.ttf",
        fontpath "NotoSerif-Bold.ttf",
        fontpath "DroidSans-Bold.ttf",
        fontpath "SourceSansPro-SemiBold.ttf",
        fontpath "Roboto-Regular.ttf",
        #endif
        };


            for(const char *name:standardfonts)  {
                if((blackfont = nvgCreateFont(avg, "dance-bold", name))!=-1) {
                    CURVELOGGER("blackfont %s succeeded\n",name);
                    break;
                    }
                CURVELOGGER("blackfont %s failed\n",name);
                }
        if(blackfont==-1) {
            FATAL("all fonts failed: tried: ");
        #ifndef  JUGGLUCO_APP
            for(const char *name:standardfonts)  {
                    FATAL("%s\n",name);
                    }
        #endif
            }

        return blackfont;
        }

/*
static int getArabicRegular(NVGcontext* avg) {
        constexpr const char fonts[][sizeof(fontpath "NotoNaskhArabic-Regular.ttf")]
        {
        fontpath "NotoNaskhArabic-Regular.ttf",
        fontpath "NotoNaskh-Regular.ttf",
        fontpath  "DroidSansArabic.ttf"
        };
    for(const char *name:fonts)  {
        if(int font = nvgCreateFont(avg, "regular", name);font!=-1) 
                return font;
        }
    return  -1;
    }
static int getArabicBold(NVGcontext* avg) {
        constexpr const char fonts[][sizeof(fontpath "NotoNaskhArabic-Bold.ttf")]
        {
        fontpath "NotoNaskhArabic-Bold.ttf",
        fontpath "NotoNaskh-Bold.ttf",
        fontpath  "DroidSansArabic.ttf"
        };
    for(const char *name:fonts)  {
        if(int font = nvgCreateFont(avg, "dance-bold", name);font!=-1) 
                return font;
        }
    return  -1;
    }
    */

#ifdef JUGGLUCO_APP
static int getArabicMenu(NVGcontext* avg) {
        constexpr const char fonts[][sizeof(
        #ifdef JUGGLUCO_APP
        fontpath "NotoNaskhArabicUI-Regular.ttf"
#else
        "/usr/share/fonts/truetype/noto/NotoNaskhArabicUI-Regular.ttf"
#endif
        )]
        {
        #ifdef JUGGLUCO_APP
        fontpath "NotoNaskhArabicUI-Regular.ttf",
        fontpath "NotoNaskhUI-Regular.ttf",
        fontpath  "DroidSansArabic.ttf"
#else
"/usr/share/fonts/truetype/noto/NotoNaskhArabicUI-Regular.ttf"
#endif
        };
    for(const char *name:fonts)  {
        if(int font = nvgCreateFont(avg, "regular", name);font!=-1) 
                return font;
        }
    return  -1;
    }

#endif 
bool usedScript[SCR_COUNT]{};


bool initScriptFonts(const FontUse &fontuse) {
   bool ret=false;
   auto doinit{[](const FontUse &fontuse,bool&ret){
            LOGAR("initScriptFonts");
            int len=settings->getlabelcount();
            for(int i=0;i<len;i++) {
                   useFontsForName(fontuse, settings->getlabel(i).data());
                   }
            ret=true;
            return true;
            }};
   const static bool init=doinit(fontuse,ret);;
   return ret;
   }
void    JCurve::initfont(NVGcontext* avg) { 
CURVELOGAR("initfont");
if(!avg) {
    CURVELOGAR("avg==null");
    return;
    }
thevg=avg;

    font=blackfont = getBlackFont(avg);
#ifdef JUGGLUCO_APP
    if((whitefont=getWhiteFont(avg))==-1) {
        CURVELOGAR("white font failed");
        whitefont=blackfont;
        }
#else
        whitefont=blackfont;
#endif
const FontUse fontuse{.vg=avg,.whitefont=whitefont,.blackfont=blackfont,.scriptEnabled=usedScript};
if(usedtext==&artext) {

//    nvgAddFallbackFontId(avg, blackfont,getArabicBold(avg)); 


 //   nvgAddFallbackFontId(avg, whitefont,getArabicRegular(avg));
     enableScript(fontuse, SCR_ARABIC);

#ifdef JUGGLUCO_APP
    menufont=nvgCreateFontMem(avg, "regular", (unsigned char *)fontfile, sizeof(fontfile), 0);
    int fallback2 =getArabicMenu(avg);
    nvgAddFallbackFontId(avg,menufont, getMenuFont(avg));
    nvgAddFallbackFontId(avg, menufont,fallback2);
#endif

   chfontset=ARABIC;


}
else
if(usedtext==&zhtext) {
     enableScript(fontuse, SCR_CJK);
#ifdef JUGGLUCO_APP
    if(-1==(menufont = nvgCreateFont(avg, "regular",

#ifdef JUGGLUCO_APP
    fontpath "NotoSerifCJK-Regular.ttc"
#else
"/usr/share/fonts/opentype/noto/NotoSerifCJK-Regular.ttc"
#endif


    ))) {
      CURVELOGAR("menufont NotoSerifCJK-Regular failed");
       if(-1==(menufont = nvgCreateFont(avg, "regular",

#ifdef JUGGLUCO_APP
       fontpath "NotoSansCJK-Regular.ttc"
#else
"/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc"
#endif
       ))) {
         CURVELOGAR("menufont NotoSansCJK-Regular failed");
         if(-1==(menufont = nvgCreateFont(avg, "regular",
#ifdef JUGGLUCO_APP
         fontpath "DroidSansFallback.ttf"
#else
        "/usr/share/fonts-droid-fallback/truetype/DroidSansFallback.ttf"
#endif
         )))  {
               CURVELOGAR("font DroidSansFallback failed");
               }
         }

      }
#endif
//TODO free font ???
    chfontset=CHINESE;
    }
else  {

#ifdef USE_HEBREW
     enableScript(fontuse, SCR_HEBREW);
#ifdef JUGGLUCO_APP
    menufont=nvgCreateFontMem(avg, "regular", (unsigned char *)fontfile, sizeof(fontfile), 0);
    int fallback2 = nvgCreateFont(avg, "regular", fontpath "NotoSerifHebrew-Regular.ttf");
    nvgAddFallbackFontId(avg,menufont, fallback);
    nvgAddFallbackFontId(avg, menufont,fallback2);

#endif

    chfontset=HEBREW;
}

else  
#endif
{
    chfontset=REST;

#ifdef JUGGLUCO_APP
int fallback=getMenuFont(avg);
#ifdef MENUARROWS
    menufont=nvgCreateFontMem(avg, "regular", (unsigned char *)fontfile, sizeof(fontfile), 0);
    nvgAddFallbackFontId(avg,menufont, fallback);
#endif
#endif //JUGGLUCO_APP

    if(invertcolors)
        font=whitefont;
    else
        font=blackfont;
        }
        }
    if(!initScriptFonts(fontuse)) {
        applyfonts(fontuse);
        }

    nvgFontFaceId(avg,font);

    nvgFontSize(avg, headsize);
    constexpr const char smaller[]="<";
    bounds_t bounds;
    nvgTextBounds(avg, 0,  0, smaller,smaller+sizeof(smaller)-1, bounds.array);
    smallerlen=bounds.xmax-bounds.xmin;

    nvgTextMetrics(avg, nullptr,nullptr, &headheight);
    headheight*=0.7;
    nvgFontSize(avg, smallsize);
    nvgTextMetrics(avg, nullptr,nullptr, &smallfontlineheight);
    constexpr const char timestring[]="29:59";
    nvgTextBounds(avg, 0,  0, timestring,timestring+sizeof(timestring)-1, bounds.array);
    timelen=bounds.xmax-bounds.xmin;
    timeheight=bounds.ymax-bounds.ymin;
   CURVELOGGER("timeheight=%f timelen=%f\n",timeheight,timelen);

    const char listitem[]="39-08-2028 09-59 RRRRRRRRRRR 999.9";     
    nvgTextBounds(avg, 0,  0, listitem,listitem+sizeof(listitem)-1, bounds.array);
    listitemlen=bounds.xmax-bounds.xmin+smallsize;

    constexpr const char exampl[]="0M0063KNUJ0";
    float xhalf=dwidth/2;
    float yhalf=dheight/2;
    nvgFontSize(avg, mediumfont);
    nvgTextAlign(avg,NVG_ALIGN_LEFT|NVG_ALIGN_TOP);
     nvgTextBounds(avg, xhalf,  yhalf,exampl, exampl+sizeof(exampl)-1,(float *)&sensorbounds);
     sensorbounds.right-=sensorbounds.left;
     sensorbounds.bottom-=sensorbounds.top;
     sensorbounds.left-=xhalf;
     sensorbounds.top-=yhalf;
     CURVELOGGER("sensorbounds.left=%.1f\n",sensorbounds.left);
    valuesize=sensorbounds.right*2;
#ifdef JUGGLUCO_APP
     fixatex=settings->data()->fixatex;
     fixatey=settings->data()->fixatey;
     if(fixatex)
         duration=settings->data()->duration;
#endif
    createcolors();
    //invertcolors=settings->data()->invertcolors;
    //startincolors=startbackground*invertcolors;
     }





//s/^\([^=]*\)=.*;/static float \1;/g

void JCurve::setfontsize(float small,float menu,float density,float headin) {

const float head=headin
#ifdef WEAROS
*0.7
#endif
;
CURVELOGGER("setfontsize density=%.1f, head=%.1f, small=%.1f menu=%.1f\n",(double)density,(double)head,(double)small,menu); 
smallsize=small;
menusize=menu;
this->density=density;
headsize=head;
midsize=head/3;
mediumfont= headsize/6;

timefontsize=smallsize;
historyStrokeWidth=3*density;
numcircleStrokeWidth=5/2*density;
lowGlucoseStrokeWidth=2.5*density;
pollCurveStrokeWidth=3*density;
hitStrokeWidth=10*density;
TrendStrokeWidth=15/2*density;
glucoseLinesStrokeWidth=1.5*density;
timeLinesStrokeWidth=glucoseLinesStrokeWidth;
dayEndStrokeWidth=2*density;
nowLineStrokeWidth=density*2;
pointRadius=4*density;
foundPointRadius=8*density;
arrowstrokewidth=5*density;
}
void calccurvegegs();

#include "searchgegs.hpp"
extern Searchgegs searchdata;




pair<const ScanData*,const ScanData*> getScanRange(const ScanData *scan,const int len,const uint32_t start,const uint32_t end) {
    ScanData scanst{.t=start};
    const ScanData *endscan= scan+len;
    auto comp=[](const ScanData &el,const ScanData &se ){return el.t<se.t;};
      const ScanData *low=lower_bound(scan,endscan, scanst,comp);
    if(low==endscan) {
        return {endscan,endscan};
        }
    scanst.t=end;
      const ScanData *high=lower_bound(low,endscan, scanst,comp);

    return {low,high};
    }

extern Sensoren *sensors;
/*
extern std::vector<pair<const ScanData*,const ScanData*>> getsensorranges(uint32_t start,uint32_t endt) ;

std::vector<pair<const ScanData*,const ScanData*>> getsensorranges(uint32_t start,uint32_t endt) {
    auto hists= sensors->sensorsInPeriod(start,endt) ;
    vector<pair<const ScanData*,const ScanData*>> polldata;
    polldata.reserve(hists.size());
    uint32_t timeiter=start;
    CURVELOGAR("start getsensorranges: ");
    for(int i=hists.size()-1;i>=0&&timeiter<endt;i--)  {
        auto his=sensors->getSensorData(hists[i]);
        CURVELOGGER("sensor %s\n",his->showsensorname().data());
        std::span<const ScanData>     poll=his->getPolldata();
#if !defined(NDEBUG)&&defined(JUGGLUCO_APP)
        auto wastimeiter=timeiter;
#endif
        auto ran=getScanRange(poll.data(),poll.size(),timeiter,endt);
        if(ran.first==ran.second)
            continue;

        for(const ScanData *striter=ran.second-1;striter>=ran.first;striter--) {
            if(striter->valid())    {
                timeiter=striter->t;
                ran.second=striter+1;
                break;
                }
            }

#if !defined(NDEBUG)&&defined(JUGGLUCO_APP)
        constexpr const int  maxbuf=150;
        char buf[maxbuf];
        int len=appcurve.datestr(wastimeiter,buf);

        const char tus1[]=" : ";
        constexpr const int tus1len=sizeof(tus1)-1;
        memcpy(buf+len,tus1,tus1len);

        len+=tus1len;
        len+=appcurve.datestr(poll.data()->t,buf+len);

        memcpy(buf+len,tus1,tus1len);
        len+=tus1len;
        len+=appcurve.datestr(ran.first->t,buf+len);
        const char tus[]=" - ";
        constexpr const int tuslen=sizeof(tus)-1;
        memcpy(buf+len,tus,tuslen);
        len+=tuslen;
        len+=appcurve.datestr((ran.second-1)->t,buf+len);
        buf[len++]='\n';
        logwriter(buf,len);
#endif
        polldata.push_back(ran);
        }

    CURVELOGAR("end getsensorranges: ");
    return polldata;
    }
    */

#include "GlucoseDataType.hpp" 
/*
std::vector<GlucoseDataType<const ScanData*>> getsensorranges(uint32_t start,uint32_t endt,bool calibrated,bool allvalues,bool calibratePast,std::vector<std::unique_ptr<ScanData []>> &calibrates ) {
    auto hists= sensors->sensorsInPeriod(start,endt) ;
    std::vector<GlucoseDataType<const ScanData*>>  polldata;
    polldata.reserve(hists.size());
    uint32_t timeiter=start;
    CURVELOGAR("start getsensorranges: ");
    typedef decltype(make_calibrator<ScanData>( sensors->getSensorData(0))) CaliType;
    auto califunc=calibratePast?(&CaliType::makecalibratedback):(&CaliType::makecalibrated);
    for(int i=hists.size()-1;i>=0&&timeiter<endt;i--)  {
        auto his=sensors->getSensorData(hists[i]);
        CURVELOGGER("sensor %s\n",his->showsensorname().data());
        std::span<const ScanData>     poll=his->getPolldata();
        auto ran=getScanRange(poll.data(),poll.size(),timeiter,endt);
        if(ran.first==ran.second)
            continue;
        for(const ScanData *striter=ran.second-1;striter>=ran.first;striter--) {
            if(striter->valid())    {
                timeiter=striter->t;
                ran.second=striter+1;
                break;
                }
            }
        if(calibrated) {
            int len=ran.second-ran.first;
            ScanData *calibuf=new ScanData[len];
            calibrates.emplace_back(calibuf);
            auto cali=make_calibrator<ScanData>(his);
            auto res=(cali.*califunc)(ran.first,calibuf,len,allvalues);

            if(res.first&&res.first!=res.second)
                polldata.push_back({res.first,res.second,his->getStreamIdDistance(),false,{his,calibratePast}});
            }
        else
            polldata.push_back({ran.first,ran.second,his->getStreamIdDistance(),false,{his,calibratePast} });
        }

    CURVELOGAR("end getsensorranges: ");
    return polldata;
    }
*/

template <typename IterType>
void getsensorranges(uint32_t start,uint32_t endt,bool calibrated,bool calibratePast,std::vector<GlucoseDataType<IterType>> *polldataptr);
template<> void getsensorranges<const ScanData*>(uint32_t start,uint32_t endt,bool calibrated,bool calibratePast,std::vector<GlucoseDataType<const ScanData*>> *polldataptr) {
    auto &polldata=*polldataptr;
    auto hists= sensors->sensorsInPeriod(start,endt) ;
    polldata.reserve(hists.size());
    uint32_t timeiter=start;
    CURVELOGAR("start getsensorranges: ");
    for(int i=hists.size()-1;i>=0&&timeiter<endt;i--)  {
        auto his=sensors->getSensorData(hists[i]);
        CURVELOGGER("sensor %s\n",his->showsensorname().data());
        std::span<const ScanData>     poll=his->getPolldata();
        auto ran=getScanRange(poll.data(),poll.size(),timeiter,endt);
        if(ran.first==ran.second)
            continue;
        for(const ScanData *striter=ran.second-1;striter>=ran.first;striter--) {
            if(striter->valid())    {
                timeiter=striter->t;
                ran.second=striter+1;
                break;
                }
            }
            polldata.push_back({ran.first,ran.second,his->getStreamIdDistance(),calibrated,{his,calibratePast} });
        }

    CURVELOGAR("end getsensorranges: ");
    }
pair<int32_t,int32_t> histPositions(const SensorGlucoseData  * hist, const uint32_t starttime, const uint32_t endtime) ;
template<> void getsensorranges<HistoryIterator>(uint32_t start,uint32_t endt,bool calibrated,bool calibratePast,std::vector<GlucoseDataType<HistoryIterator>> *historydataptr ) {
    auto &historydata=*historydataptr;
    auto hists= sensors->sensorsInPeriod(start,endt) ;
    historydata.reserve(hists.size());
    uint32_t timeiter=start;
    CURVELOGAR("start getsensorHistoryranges: ");
    for(int i=hists.size()-1;i>=0&&timeiter<endt;i--)  {
        auto his=sensors->getSensorData(hists[i]);
        if(!his->hasRealHistory())
            continue;
        auto ran= histPositions(his, timeiter,  endt); 
        CURVELOGGER("getsensorHistoryranges %s %d-%d\n",his->showsensorname().data(),ran.first,ran.second);
        if(ran.first==ran.second)
            continue;
        for( int striter=ran.second-1;striter>=ran.first;striter--) {
            Glucose *gl=his->getglucose(striter);
            if(gl->valid())    {
                timeiter=gl->gettime();
                ran.second=striter+1;
                break;
                }
            }
        historydata.push_back({{his,ran.first},{his,ran.second},his->getmininterval() ,calibrated,{his,calibratePast}});
        }

    CURVELOGAR("end getsensorHistoryranges");
    }


//static uint32_t pollgapdist=5*60;
static uint32_t pollgapdist=330;
pair<const ScanData*,const ScanData*> getScanRangeRuim(const ScanData *scan,const int len,const uint32_t start,const uint32_t end) {
    return getScanRange(scan,len,start-pollgapdist,end+pollgapdist);
   }    
/*
pair<const ScanData*,const ScanData*> getScanRangeRuim(const ScanData *scan,const int len,const uint32_t start,const uint32_t end) {
    auto [low,high]= getScanRange(scan,len,start,end);
    const ScanData *endscan= scan+len;
    if(low>scan&&(low->t-(low-1)->t)<=pollgapdist)
        low--;
    if(high<endscan&&((high+1)->t-high->t)<=pollgapdist)
        high++;
    return {low,high};
    } */

 void           JCurve::sidenum(NVGcontext* avg,const float posx,const float posy,const char *buf,const int len,const bool hit) {
        int align= NVG_ALIGN_MIDDLE;
        float valx=posx;
        const float afw=hit?1.14:0.64;;
         if((posx-dleft)>(dwidth/2)) {
            align|=NVG_ALIGN_RIGHT;
            valx-=smallsize*afw;
            }
        else {
            align|=NVG_ALIGN_LEFT;
            valx+=smallsize*afw;
            }
        nvgTextAlign(avg,align);
        nvgText(avg, valx,posy, buf, buf+len);
        }



int shownlabels;

float tapx=-700,tapy;
bool selshown=false;

#include "numdisplayfuncs.hpp"
extern vector<NumDisplay*> numdatas;






 bool    JCurve::glucosepointinfo(NVGcontext* avg,time_t tim,uint32_t value,   float posx, float posy) {
    if((!selshown&&nearby(posx-tapx,posy-tapy,density))) {
        constexpr int maxbuf=60;
        char buf[maxbuf];
        struct tm tmbuf;
         struct tm *tms=localtime_r(&tim,&tmbuf);

//        int len=snprintf(buf,maxbuf,"%02d:%02d", tms->tm_hour,mktmmin(tms));
      int len=mktime(tms->tm_hour,mktmmin(tms),buf);
        nvgFontSize(avg, smallsize);
        nvgTextAlign(avg,NVG_ALIGN_CENTER|NVG_ALIGN_MIDDLE);
        float cor=((posy-dtop)<(dheight/2))?smallsize:-smallsize;
        nvgText(avg, posx,posy+cor*.92, buf, buf+len);
        char *buf2=buf+len;
        *buf2++='\n';
         len=snprintf(buf2,maxbuf-len-1,gformat, ::gconvert(value,glunit));
        sidenum(avg,posx,posy,buf2,len,false);
        
    //    nvgText(avg, posx,posy+cor*.92*2, buf, buf+len);
#ifndef DONTTALK
        if(speakout) {
            speak(buf);
            }
#endif

        selshown=true;
        CURVELOGGER("glucosepointinfo %s %ud %f\n",buf,tim,posx);
        return true;
        }
    return false;
    }
 bool    JCurve::glucosepoint(NVGcontext* avg,time_t tim,uint32_t value,   float posx, float posy) {
    nvgCircle(avg, posx,posy,pointRadius);
    return glucosepointinfo(avg,tim,value,posx,posy);
    }



void endstep(NVGcontext* avg) ;
template <class TX,class TY>   void  JCurve::showScan(NVGcontext* avg,const ScanData *low,const ScanData *high,  const TX &transx,  const TY &transy,const int colorindex) {

    nvgFillColor(avg,*getcolor(colorindex));
    nvgBeginPath(avg);
#ifdef DOESSEARCH
    bool search=scansearchtype==(scansearchtype&searchdata.type);
#endif
    for(const ScanData *it=low;it!=high;it++) {
        if(it->valid()) {
            const uint32_t tim= it->t;
            const auto glu=it->g*10;
            const auto posx= transx(tim),posy=transy(glu);
#ifdef DOESSEARCH
            if(search&&searchdata(it)) 
                nvgCircle(avg, posx,posy,foundPointRadius);
            else 
#endif
            {
                if(glucosepoint(avg,tim,glu,posx,posy))
                    lasttouchedcolor=colorindex;
                }
            }
        }
    nvgFill(avg);
    }
    //            nvgCircle(avg, posx,posy,;
    
 void    JCurve::makecircle(NVGcontext* avg,float posx,float posy) {
    nvgBeginPath(avg);
    nvgCircle(avg, posx,posy,pointRadius);
    nvgFill(avg);

    }

template <class TX,class TY> void JCurve::showlineScan(NVGcontext* avg,const ScanData *low,const ScanData *high,  const TX &transx,  const TY &transy,const int colorindex,bool search
) { 
#ifdef SI5MIN
   uint32_t dif=isSibionics?8*60:pollgapdist;
#else
   uint32_t dif=pollgapdist;
#endif


#ifdef DOESSEARCH
    if(search) {
        nvgBeginPath(avg);
        nvgStrokeColor(avg, *getyellow()); nvgFillColor(avg, *getyellow());
        nvgStrokeWidth(avg, hitStrokeWidth);
        bool restart=true,first;
        uint32_t late=0;
        bool washit=false;
        float prevx=-1.0f,prevy;
        for(const ScanData *it=low;it!=high;it++) {
            if(it->valid()&&searchdata(it)) {
                    const uint32_t tim= it->t;
                    const auto glu=it->g*10;
                    const auto posx= transx(tim),posy=transy(glu);
                    if(washit) {
                        if(!restart&&tim>late) {
                            nvgStroke(avg);
                            if(first)
                                makecircle(avg,prevx,prevy);
                            restart=true;
                            }
                        }
                    else {
                        washit=true;    
                        restart=true;
                        }
                    if(restart) {
                        nvgBeginPath(avg);
                         nvgMoveTo(avg, posx,posy);
                         restart=false;
                         first=true;
                         }
                    else {
                        first=false;
                        nvgLineTo( avg,posx,posy);
                        }
                    late=tim+dif;
                    prevx=posx;
                    prevy=posy;
                    }
            else {
                if(washit&&!restart) {
                    nvgStroke(avg);
                    if(first)
                        makecircle(avg,prevx,prevy);
                    }
                else
                    washit=false;
                restart=true;
                }
            }
        if(washit) {    
            if(!restart)
                nvgStroke(avg);
            if(first)
                makecircle(avg,prevx,prevy);
            }
        }
#endif
    bool restart=true;
    nvgBeginPath(avg);
    const NVGcolor *col=getcolor(colorindex);
    nvgStrokeColor(avg, *col);
    nvgFillColor(avg,*col);
    nvgStrokeWidth(avg, pollCurveStrokeWidth);
    uint32_t late=0;
    float startx=-1000,starty=-1000;
    for(const ScanData *it=low;it!=high;it++) {
        if(it->valid()) {
            const uint32_t tim= it->t;
            const auto glu=it->g*10;
            const auto posx= transx(tim),posy=transy(glu);
/*#ifndef NOLOG
time_t ttim=tim;
            CURVELOGGER("showlineScan posx=%f tim=%ud %s",posx,tim,ctime(&ttim));
#endif */

            if(!restart&&tim>late) {
                nvgStroke(avg);
                if(startx>=0) {
                    nvgBeginPath(avg);
                    nvgCircle(avg, startx,starty,pollCurveStrokeWidth);
                    nvgFill(avg);
                     }
                restart=true;
                }
            if(restart) {
                nvgBeginPath(avg);
                 nvgMoveTo(avg, posx,posy);
                 startx=posx,starty=posy;
                 restart=false;
                 }
            else {
                 startx=starty=-1000.0f;
                nvgLineTo( avg,posx,posy);
                }

            late=tim+dif;

            if(glucosepointinfo(avg,tim,glu, posx, posy) ) {
                nvgLineTo( avg,posx,posy);
                nvgStroke(avg);
                nvgBeginPath(avg);
                nvgCircle(avg, posx,posy,pointRadius*1.3);
                nvgFill(avg);
                nvgBeginPath(avg);
                nvgMoveTo(avg, posx,posy);
                lasttouchedcolor=colorindex;
                }
            }
        }

        nvgStroke(avg);
        if(startx>=0) {
            nvgBeginPath(avg);
            nvgCircle(avg, startx,starty,pollCurveStrokeWidth);
            nvgFill(avg);
             }
        }

pair<int32_t,int32_t> histPositions(const SensorGlucoseData  * hist, const uint32_t starttime, const uint32_t endtime) {
    int32_t firstmog=hist->getstarthistory();
    int32_t lastmog= hist->getAllendhistory()-1;
    CURVELOGGER("histPositions first=%d last=%d\n",firstmog,lastmog);
    if(firstmog>=lastmog)
        return {firstmog,lastmog};
    uint32_t begin=hist->getstarttime();
    int sdisp=starttime-begin;
    int period=hist->getinterval();
    int off=sdisp/period;    
    int32_t    firstpos=firstmog+(uint32_t)((off>0)?off:0);
    if(firstpos>lastmog)
        firstpos=lastmog;
    for(;firstpos>firstmog;--firstpos) {
        auto tim=hist->timeatpos(firstpos);
        if(tim&&tim<=starttime)
            break;
        }
    for(;firstpos<lastmog&&!hist->timeatpos(firstpos);++firstpos) {
        }
    uint32_t firsttime=hist->timeatpos(firstpos);

    int lastscreen=firstpos+(endtime-firsttime)/period;
    int32_t lastpos=(lastscreen>lastmog)?lastmog:lastscreen;
    while(lastpos<lastmog&&hist->timeatpos(lastpos)<endtime)
        lastpos++;

    return {firstpos,lastpos};
    }

template <class TX,class TY> void    JCurve::calihistcurve(NVGcontext* avg,const SensorGlucoseData  * hist, const int32_t firstpos, const int32_t lastpos,const TX &xtrans,const TY &ytrans,const int colorindex) {
    if(hist->isDexcom()&&!settings->data()->dexcomPredict)
        return;

    const NVGcolor *col=getcolor(colorindex);
    nvgStrokeColor(avg, *col);
    nvgFillColor(avg,*col);
     bool restart=true;
     float startx=-3000.0f,starty=-3000.0f;
    CalibrateBackward<Glucose>  cali(hist,CalibratePast);
    for(auto pos=lastpos;pos>=firstpos;--pos) {
        const Glucose *histglu=hist->getglucose(pos);
        if(histglu->valid()) {
            const uint32_t tim=histglu->gettime();
            auto mgdL= histglu->getmgdL();
            double calibrated=cali.backvalue(tim,mgdL);
            if(isnan(calibrated)) {
                if(!allvalues) {
                        break;
                        }
                calibrated=mgdL;
                }
            const uint32_t glu=std::round(calibrated*10.0);
            auto posx=xtrans(tim),posy=ytrans( glu);
            bool oncurve=glucosepointinfo(avg,tim,glu, posx, posy);
            if(restart) {
                if(oncurve) {
                    nvgBeginPath(avg);
                    nvgCircle(avg, posx,posy,pointRadius*1.3);
                    nvgFill(avg);
                    lasttouchedcolor=colorindex;
                    }
                nvgBeginPath(avg);
                 nvgMoveTo(avg, posx,posy);
                 startx=posx,starty=posy;
                 restart=false;
                 }
            else {
                nvgLineTo( avg, posx,posy);
                 startx=-3000.0f,starty=-3000.0f;
                if(oncurve) {
                    nvgStroke(avg);
                    nvgBeginPath(avg);
                    nvgCircle(avg, posx,posy,pointRadius*1.3);
                    nvgFill(avg);
                    nvgBeginPath(avg);
                    nvgMoveTo(avg, posx,posy);
                    lasttouchedcolor=colorindex;
                    }
                }

            }
        else {
            if(!restart) {
                nvgStroke(avg);
                if(startx>=0.0f) {
                    nvgBeginPath(avg);
                    nvgCircle(avg, startx,starty,historyStrokeWidth);
                    nvgFill(avg);
                    }
                restart=true;
                }
            }
        }
    if(!restart) {
        nvgStroke(avg);
        if(startx>=0.0f) {
            nvgBeginPath(avg);
            nvgCircle(avg, startx,starty,historyStrokeWidth);
            nvgFill(avg);
            }
        }
#ifdef DOESSEARCH
    if((searchdata.type&calibratedHistorysearchtype)==calibratedHistorysearchtype) {
        CalibrateBackward<Glucose>  calibrate(hist,CalibratePast);
        nvgBeginPath(avg);
        for(auto pos=lastpos;pos>=firstpos;--pos) {
            const Glucose *glu=hist->getglucose(pos);
            const auto tim=glu->gettime();
            const auto mgdL=glu->getmgdL();
            double calibrated=calibrate.backvalue(tim,mgdL);
            if(isnan(calibrated)) {
                if(!allvalues) {
                        break;
                        }
                calibrated=mgdL;
                }
            const int mgL=std::round(calibrated*10.0);
            if(searchdata(tim,mgL)) {
                if(tim) {
                    auto xc=xtrans(tim);
                    auto yc= ytrans(mgL);
                    nvgCircle(avg,xc,yc,foundPointRadius);
                    }
                }
            }
        nvgFill(avg);
        }
#endif
    }
template <class TX,class TY> void    JCurve::histcurve(NVGcontext* avg,const SensorGlucoseData  * hist, const int32_t firstpos, const int32_t lastpos,const TX &xtrans,const TY &ytrans,const int colorindex) {
    if(hist->isDexcom()&&!settings->data()->dexcomPredict)
        return;

    const NVGcolor *col=getcolor(colorindex);
    nvgStrokeColor(avg, *col);
    nvgFillColor(avg,*col);
     bool restart=true;
     float startx=-3000.0f,starty=-3000.0f;
    for(auto pos=firstpos;pos<=lastpos;pos++) {
        const Glucose *histglu=hist->getglucose(pos);
        if(histglu->valid()) {
            const uint32_t tim=histglu->gettime(),glu=histglu->getsputnik();
            auto posx=xtrans(tim),posy=ytrans( glu);
            bool oncurve=glucosepointinfo(avg,tim,glu, posx, posy);
            if(restart) {
                if(oncurve) {
                    nvgBeginPath(avg);
                    nvgCircle(avg, posx,posy,pointRadius*1.3);
                    nvgFill(avg);
                    lasttouchedcolor=colorindex;
                    }
                nvgBeginPath(avg);
                 nvgMoveTo(avg, posx,posy);
                 startx=posx,starty=posy;
                 restart=false;
                 }
            else {
                nvgLineTo( avg, posx,posy);
                 startx=-3000.0f,starty=-3000.0f;
                if(oncurve) {
                    nvgStroke(avg);
                    nvgBeginPath(avg);
                    nvgCircle(avg, posx,posy,pointRadius*1.3);
                    nvgFill(avg);
                    nvgBeginPath(avg);
                    nvgMoveTo(avg, posx,posy);
                    lasttouchedcolor=colorindex;
                    }
                }

            }
        else {
            if(!restart) {
                nvgStroke(avg);
                if(startx>=0.0f) {
                    nvgBeginPath(avg);
                    nvgCircle(avg, startx,starty,historyStrokeWidth);
                    nvgFill(avg);
                    }
                restart=true;
                }
            }
        }
    if(!restart) {
        nvgStroke(avg);
        if(startx>=0.0f) {
            nvgBeginPath(avg);
            nvgCircle(avg, startx,starty,historyStrokeWidth);
            nvgFill(avg);
            }
        }
#ifdef DOESSEARCH
    if((searchdata.type&historysearchtype)==historysearchtype) {
        nvgBeginPath(avg);
        for(auto pos=firstpos;pos<=lastpos;pos++) {
            const Glucose *glu=hist->getglucose(pos);
            if(searchdata(glu)) {
                const auto tim=glu->gettime();
                if(tim) {
                    const auto sput=glu->getsputnik();
                    auto xc=xtrans(tim);
                    auto yc= ytrans(sput);
                    nvgCircle(avg,xc,yc,foundPointRadius);
                    }
                }
            }
        nvgFill(avg);
        }
#endif
    }


extern uint32_t getnumlasttime();
//uint32_t maxstarttime() ;
uint32_t maxtime() {
    const uint32_t numt=getnumlasttime();
    const uint32_t sent= sensors->timelastdata(); 
    #ifndef NOLOG
    time_t tim=sent;
    CURVELOGGER("sensors->timelastdata()=%u %s",sent,ctime(&tim));
    #endif
    return max(numt,sent);
    }
    
static uint32_t getnumfirsttime() {
    uint32_t first=UINT32_MAX;

    for(auto el:numdatas)  {
        auto mog=el->getfirsttime();
        if(mog<first)
            first=mog;    
        }
    return first;
    }
    
uint32_t mintime() {
    uint32_t sent= sensors?sensors->timefirstdata():UINT32_MAX;
    uint32_t numt=getnumfirsttime();
   uint32_t tim= min(numt,sent);
   #ifndef NOLOG
   time_t t=tim;
   CURVELOGGER("mintime=%d %s",tim,ctime(&t));
   #endif

   if(tim==UINT32_MAX)
        tim=time(nullptr);
   return tim;
    }
uint32_t JCurve::minstarttime() {
    uint32_t mini=mintime();
    return mini-duration;
    }

//uint32_t starttime;
//int duration=8*60*60;

//extern void setstarttime(uint32_t);
void JCurve::setdiffcurrent() {
    //diffcurrent=(uint64_t)time(nullptr)-starttime;
    auto now=time(nullptr);
    diffcurrent=now-starttime;
    if(diffcurrent>(duration*5/6)) {
        doclamp=false;
        }
     else
        doclamp=true;
    CURVELOGGER("now=%u starttime=%u diffcurrent=%d doclamp=%d\n",now,starttime,diffcurrent,doclamp);
     return;
    }
void JCurve::setstarttime(uint32_t newstart) {
    CURVELOGGER("setstarttime(%u) nowclamp=%d\n",newstart,nowclamp);
    starttime=newstart;
    if(nowclamp) {
        setdiffcurrent();
        }
    }
uint32_t JCurve::maxstarttime() {
    float duraf=((float)valuesize/dwidth);
    CURVELOGGER("dwidth=%f valuesize=%f duraf=%f\n",(double)dwidth,(double)valuesize,(double)duraf);
    float subtr=0.91 - duraf*1.2f;
    return time(nullptr)-subtr*duration;
    }
void JCurve::begrenstijd() {
    auto maxstart= maxstarttime();
    if(starttime>maxstart)
        setstarttime(maxstart);
    else {
        auto minstart= minstarttime();
        if(starttime<minstart)
            setstarttime(minstart);
        }
    }

#include <memory>

pair<float,float>    JCurve::drawtrender(NVGcontext* avg,const std::array<uint16_t,16> &trend,const float x,const float y,const float w,const float h) {
    auto minel=std::min_element(trend.begin(),trend.end());
    auto maxel=std::max_element(trend.begin(),trend.end());
     const int low=minel-trend.begin();
     const int high=maxel-trend.begin();
     if(low<0||high<0)
         return {0,dtop+dheight/2};
    const float lowval=*minel;
    const float highval=*maxel;
    const float mid=(lowval+highval)/2.0;
    CURVELOGGER("width=%.0f, height=%.0f\n",w,h);
    CURVELOGGER("low=%.0f,high=%.0f,mid=%.0f\n",lowval,highval,mid);
    constexpr float hglurange=2*convfactor;
    const auto gety=[y,h,mid](const short val)->float  { return y+h/2.0-(((val-mid)/hglurange)*h);};
    const int step=w/(trend::num-1);
    nvgBeginPath(avg);
     nvgStrokeWidth(avg, TrendStrokeWidth);
//    nvgStrokeColor(avg, white);
    nvgStrokeColor(avg, *getblack());
    int i=0;
    unsigned short glu0;
    for(;!(glu0=trend[i]);i++)
        if(i>=(trend.size()-3))
            return {0,dtop+dheight/2};
    float pos0=gety(glu0);
    float posx= x+i*step;
     nvgMoveTo(avg,posx ,pos0);
    CURVELOGGER("%.1f (%hi) (%.0f,%.0f)\n",glu0/convfactor,glu0,posx,pos0);
    posx+=step;
    float posy=0.0f;
    i++;
    for(;i<trend.size();i++,posx+=step) {
        short glu=trend[i];
        if(glu) {
            posy=gety(glu);
            CURVELOGGER("%.1f (%hi) (%.0f,%.0f)\n",glu/convfactor,glu,posx,posy);
            nvgLineTo( avg,posx ,posy);
            }
        }
    nvgStroke(avg);
    return std::pair<float,float>({pos0,posy});
    }
void startstep(NVGcontext* avg,const NVGcolor &col);



void JCurve::setextremes(pair<int,int> extr) {
    auto [gminin,gmaxin]=extr;
    setend=0;
    const uint32_t gmaxmax=ghigh>0.0f?ghigh:settings->graphhigh();
    const uint32_t gminmin=glow>0.0f?glow:settings->graphlow();
    if(gmaxin<gmaxmax)
        gmaxin=gmaxmax;
    if(gminin>gminmin)
        gminin=gminmin;
    grange=gmaxin-gminin;
    gmin=gminin;
    }

pair<int,int> JCurve::getextremes(const vector<int> &hists, const pair<const ScanData *,const ScanData*> **scanranges, int scannr,const pair<int32_t,int32_t> *histpositions) {
    int gmax=0;
    int gmin=6000;
    const int histlen=hists.size();
    for(int i=0;i<histlen;i++) {
     const auto his=sensors->getSensorData(hists[i]);
      if(!his->isDexcom()||(showhistories&&settings->data()->dexcomPredict)) {
            for(auto pos=histpositions[i].first,last=histpositions[i].second;pos<=last;pos++) {
                int glu=his->sputnikglucose(pos);
                if(glu) {
                    if(glu>gmax)
                         gmax=glu;
                    if(glu<gmin)
                         gmin=glu;
                    }
                }
            }
        for(int j=0;j<scannr;j++) {
            const pair<const ScanData *,const ScanData*> *srange=scanranges[j];
            for(const ScanData *it=srange[i].first,*last=srange[i].second;it<last;it++) {
                if(it->valid()) {
                    auto mgdL=it->g;
                    int glu=mgdL*10;
                    if(glu>gmax)
                        gmax=glu;
                     if(glu<gmin)
                         gmin=glu;
                    }
                }
            }
        }
    return {gmin,gmax};
    }
template <class LT> void    JCurve::glucoselines(NVGcontext* avg,const float last,const float smallfontlineheight,const int gmax,const LT &transy,bool showlevels) {
    nvgStrokeWidth(avg, glucoseLinesStrokeWidth);
    nvgStrokeColor(avg, *getgray());
    const double yscale=transy(1)-transy(0);
    const float mindisunit=smallsize*1.5;
    const float minst=abs(mindisunit/yscale);
    const bool ismmolL=glunit==1;//settings->usemmolL();
    const double unit=ismmolL?0.5*convfactor:100;
    const double unit2=unit*2;

    uint32_t step=minst<=unit?unit:ceilf(minst/unit2)*unit2;
    float startld;
    nvgTextAlign(avg,NVG_ALIGN_CENTER|NVG_ALIGN_MIDDLE);

    if(getLevelLeft()) {
        startld = timelen*.4;
        }
    else  {
        startld =  dwidth/2+dleft;
        }

  const    uint32_t startl=0;
  
    const float endline=last;
//    CURVELOGGER("glucoselines: unit=%f unit2=%f step=%d (%g) startl=%d (%g)\n",unit,unit2,step,::gconvert(step,glunit),startl,::gconvert(startl,glunit));
#ifdef WEAROS
   const auto endlevel=dheight-smallfontlineheight;
   const auto startlevel=2.5*smallfontlineheight;
#endif
    
    for(auto y=startl+step;y<gmax;y+=step) {
        float dy=transy(y);
        if(dy<=0)
            continue;
        nvgBeginPath(avg);
         nvgMoveTo(avg,dleft ,dy) ;
        nvgLineTo( avg, endline,dy);
        nvgStroke(avg);
#ifdef WEAROS
        if(showlevels&&dy>startlevel&&dy<endlevel) 
#else
        if(dy>smallfontlineheight) 
#endif
        {
            constexpr const int  bufsize=50;
            char buf[bufsize];
#ifdef CONV18
            int len=snprintf(buf,bufsize,"%g",::gconvert(y,glunit));
#else
            int len=snprintf(buf,bufsize,gformat,::gconvert(y,glunit));
            if(ismmolL)  {
                if(buf[len-1]=='0') 
                    len-=2;
                }
#endif
            if(len>bufsize)
                len=bufsize;
            nvgText(avg, startld,dy, buf, buf+len);
            }

        }
    }
struct displaytime {
    const uint32_t tstep;
    const uint32_t first;
    const uint32_t last;
    };
template <class LT> const displaytime JCurve::getdisplaytime(const uint32_t nu,const uint32_t starttime,const uint32_t endtime, const LT &transx) {
    const float xscale=transx(1)-transx(0);
    const float mindisunit=smallsize*(hour24()?3:4.5);
    const  float minst=abs(mindisunit/xscale);
    const uint32_t tstep=(minst<=60*15)?60*15:((minst<=60*30)?60*30:ceilf((minst/(60.0*60)))*(60*60));
    const uint32_t first=uint32_t(ceilf(starttime/(double)tstep))*tstep;    
    const uint32_t endhier=(nu<endtime)?(nu+tstep-59):(endtime-1);
    uint32_t last=uint32_t(floorf(endhier/double(tstep)))*tstep;    
    if((last>nu)&&(2*(last-nu))>tstep)
        last=nu;

    CURVELOGGER("getdisplaytime xscale=%f %u %u %u\n",xscale,tstep,first,last);
    return {tstep,first,last};
}

static bool timemiddle() {
   return false;
   }
#ifdef NOCUTOFF
static bool nocutoff=true;
#endif
template <class LT>
void    JCurve::timelines(NVGcontext* avg,const displaytime *disp, const LT &transx ,uint32_t nu) {

    const uint32_t tstep=disp->tstep;
    const uint32_t first=disp->first;
    const uint32_t last= disp->last;
    #ifdef WEAROS
    const uint32_t numlast= (disp->last>nu)?(disp->last-tstep):disp->last;
    #endif
    nvgFillColor(avg, *getblack());
    nvgFontSize(avg, timefontsize);
    float timeY
#ifdef NOCUTOFF
   ,lower,upper
#endif
   ;
   if(timemiddle()) {
       nvgTextAlign(avg,NVG_ALIGN_CENTER|NVG_ALIGN_MIDDLE);
       timeY=(dheight-statusbarheight-dbottom)*.5f+statusbarheight;
#ifdef NOCUTOFF
if(nocutoff) {
      lower=timelen*.5f;
      upper=dwidth-lower;
      }
#endif
      }
   else {
       nvgTextAlign(avg,NVG_ALIGN_CENTER|NVG_ALIGN_TOP);
    timeY=
    #ifdef WEAROS
        smallfontlineheight*1.45f + //MODIFIED
//        smallfontlineheight*1.7f +
    #endif
   statusbarheight
      ;
#ifdef NOCUTOFF
   if(nocutoff) {
         float straal=dwidth*.5f;
         float over=straal-timeY;
         lower=straal-sqrt(pow(straal,2)-pow(over,2))+timelen*.4f;
         upper=dwidth-lower;
         CURVELOGGER("lower=%f upper=%f over=%f\n",lower,upper,over);
      }
#endif
      }
    const float lowY=dheight+dtop+dbottom;
    for(auto tim=first;tim<=last;tim+=tstep) {
        float dtim=transx(tim);
        char buf[20];
        struct tm tmbuf;
        time_t tmptime=tim;
         struct tm *stm=localtime_r(&tmptime,&tmbuf);

         if(stm->tm_hour||stm->tm_min) {
            if(stm->tm_min||stm->tm_hour%3) {
                nvgStrokeWidth(avg, timeLinesStrokeWidth);
                nvgStrokeColor(avg, *getgray());
                }
            else {
                nvgStrokeWidth(avg, timeLinesStrokeWidth);
                nvgStrokeColor(avg, *getthreehour());
                }
            }
        else {
            nvgStrokeWidth(avg, dayEndStrokeWidth);
            nvgStrokeColor(avg, *getblack());
            }
    #ifdef WEAROS
         if(tim<=numlast
#ifdef NOCUTOFF
      &&(!nocutoff||(dtim>lower&&dtim<upper))
#endif
       )  
    #endif
         {
        int len=mktime(stm->tm_hour,mktmmin(stm),buf);
            nvgText(avg, dtim,timeY, buf, buf+len);
            }
        nvgBeginPath(avg);
        nvgMoveTo(avg,dtim ,0) ;
        nvgLineTo( avg, dtim,lowY);
        nvgStroke(avg);
        }
    nvgFontSize(avg, smallsize);
    }

template <class LT> void    JCurve::epochlines(NVGcontext* avg,uint32_t first,uint32_t last, const LT &transx) {
        time_t startin=first;

        struct tm tmbuf;
         struct tm *stm=localtime_r(&startin,&tmbuf);
        auto hour=stm->tm_hour;
        if(stm->tm_min) {
            startin+=(60-stm->tm_min)*60;
            hour++;
            }
        
        time_t start=startin+(24-hour)*60*60;
        nvgStrokeWidth(avg, dayEndStrokeWidth);
        nvgStrokeColor(avg, *getblack());
        for(time_t t=start;t<last;t+=(24*60*60)) {
            float dtim=transx(t);
        //    CURVELOGGER("%ld\n",t);
            nvgBeginPath(avg);
            nvgMoveTo(avg,dtim ,0) ;
            nvgLineTo( avg, dtim,dheight);
            nvgStroke(avg);
            }
        nvgStrokeWidth(avg, timeLinesStrokeWidth);
        nvgStrokeColor(avg, *getthreehour());
        const int inthree=hour%3;
        start=startin+(inthree?((3-inthree)*60*60):0);
        CURVELOGGER("startin=%ld start=%ld last=%d inthree=%d\n",startin,start,last, inthree);
        for(time_t t=start;t<last;t+=(3*60*60)) {
            float dtim=transx(t);
            nvgBeginPath(avg);
            nvgMoveTo(avg,dtim ,0) ;
            nvgLineTo( avg, dtim,dheight);
            nvgStroke(avg);
            }
    }
extern std::vector<int> usedsensors;
extern void setusedsensors() ;
extern void setusedsensors(uint32_t nu) ;
void setmaxsensors(size_t sensornr) {
    setusedsensors();
    }


uint32_t lastsensorends() {
    if(const SensorGlucoseData *hist = sensors->getSensorData()) {
               return hist->expectedEndTime();
               }
          return 0u;
          }
#include "gluconfig.hpp"
void    JCurve::drawarrow(NVGcontext* avg, float rate,float getx,float gety) {
        if(!isnan(rate)) {
            if(glnearnull(rate))
                rate=.0f;
            if(rate<=0.0f)
                gety-=headheight/12.5f;
            float x1=getx-density*40;
            float y1=gety+rate*density*30;

            long double rx=getx-x1;
            long double ry=gety-y1;
            double rlen= sqrt(pow(rx,2) + pow(ry,2));
             rx/=rlen;
             ry/=rlen;

            long double l=density*12;

            double addx= l* rx;
            double addy= l* ry;
            double tx1=getx-2*addx;
            double ty1=gety-2*addy;
            double xtus=getx-1.5*addx;
            double ytus=gety-1.5*addy;
            double hx=ry;
            double hy=-rx;
            double sx1=tx1+l*hx;
            double sy1=ty1+l*hy;
            double sx2=tx1-l*hx;
            double sy2=ty1-l*hy;
            nvgBeginPath(avg);
            nvgStrokeColor(avg, *getblack());
            nvgStrokeWidth(avg, arrowstrokewidth);
            nvgMoveTo(avg,x1,y1) ;
            nvgLineTo( avg, xtus,ytus);
            nvgStroke(avg);
            nvgBeginPath(avg);
            nvgFillColor(avg, *getblack());
            nvgMoveTo(avg,sx1,sy1) ;
            nvgLineTo( avg, getx,gety);
            nvgLineTo( avg, sx2,sy2);
            nvgLineTo( avg, xtus,ytus);
            nvgClosePath(avg);
            nvgFill(avg);

            }
    }
#ifndef NOLOG
//#define TESTVALUE
#endif

    
//static bool    streamvalueshown=false;

//#define DOTEST 1




int JCurve::largedaystr(const time_t tim,char *buf) {
        CURVELOGAR("largedaystr");
    struct tm stmbuf;
    localtime_r(&tim,&stmbuf);
   int len=mkhourminstr(stmbuf.tm_hour,mktmmin(&stmbuf),buf);
#ifdef WEAROS
     len+=sprintf(buf+len," %s %02d %s",usedtext->daylabel[stmbuf.tm_wday],stmbuf.tm_mday,usedtext->monthlabel[stmbuf.tm_mon]);
#else
     len+=sprintf(buf+len," %s %02d %s %d",usedtext->daylabel[stmbuf.tm_wday],stmbuf.tm_mday,usedtext->monthlabel[stmbuf.tm_mon],1900+stmbuf.tm_year);
#endif
   return len;
    }



void       JCurve::showbluevalue(NVGcontext* avg,const time_t nu,const int xpos,std::vector<int> &used) {
CURVELOGGER("showbluevalue %zd\n",used.size());
        nvgFontSize(avg, smallsize);
        nvgFillColor(avg, *getblack());

        nvgBeginPath(avg);
        nvgStrokeColor(avg, dooryellow);
        nvgStrokeWidth(avg, nowLineStrokeWidth);
        nvgMoveTo(avg,xpos ,dtop) ;
        nvgLineTo( avg, xpos,dheight+dtop+dbottom);
        nvgStroke(avg);
#ifndef WEAROS
        if(const auto *sens=sensors->getSensorData()) {
            if(!(sens->isDexcom()||sens->isSibionics())||!sens->unused()) {
                if(time_t enddate=sens->expectedEndTime()) {
                    float down=0;

                    const float timex=xpos+nowLineStrokeWidth;
                    constexpr int maxhead=80;
                    char head[maxhead];
                    int tstart,end;

                    if(isRTL()) {
                            end= datestr(enddate,head); 
                            memcpy(head+end,usedtext->sensorexpectedend.data(),usedtext->sensorexpectedend.size());
                            tstart=usedtext->sensorexpectedend.size();
                        }
                    else {
                            memcpy(head,usedtext->sensorexpectedend.data(),usedtext->sensorexpectedend.size());
                            tstart=usedtext->sensorexpectedend.size();
                            char *endstr=head+tstart;
                            end= datestr(enddate,endstr); 
                            }
                    nvgTranslate(avg, timex,down);
                    nvgRotate(avg,-NVG_PI/2.0);
                    nvgTextAlign(avg,NVG_ALIGN_CENTER|NVG_ALIGN_BOTTOM);
                    nvgText(avg, -dheight/2+down-smallfontlineheight,dwidth-timex, std::begin(head), head+end+tstart);
                    nvgResetTransform(avg);
                    }
                }
            }
#else
    if( settings->data()->IOB) {
        float down=0;
        const float timex=xpos+nowLineStrokeWidth;
        nvgTranslate(avg, timex,down);
        nvgRotate(avg,-NVG_PI/2.0);
        double getiob(uint32_t);
        int maxbuf=20;
        char tbuf[maxbuf];
        int len=snprintf(tbuf,maxbuf,"IOB: %.1f",getiob(nu));
        nvgTextAlign(avg,NVG_ALIGN_CENTER|NVG_ALIGN_BOTTOM);
        nvgText(avg, -dheight*.40f+down-smallfontlineheight,dwidth*.984f-timex, tbuf,tbuf+len);
        nvgResetTransform(avg);
        }
#endif
        const float getx= xpos+headsize*.9f+8*dwidth/headsize;

constexpr const bool showcurrentdate=true;

if(showcurrentdate) {
        const float datehigh=smallfontlineheight*.72;
        
        nvgTextAlign(avg,NVG_ALIGN_LEFT|NVG_ALIGN_TOP);
        {
        constexpr int maxbuf=120;
        char tbuf[maxbuf];
         const int datlen=largedaystr(nu,tbuf) ;
        const float timex =
            getx
        #ifdef WEAROS
            -timelen*.85f
        #endif
        ;
        const float timey = (datehigh+statusbarheight)
        #ifdef WEAROS
        *(used.size()<2?2.5f:1.0f);
        #endif
        ;

        nvgText(avg, timex,timey, tbuf, tbuf+datlen);

#ifndef WEAROS    
    if( settings->data()->IOB) {
        double getiob(uint32_t);
        int len=snprintf(tbuf,maxbuf,"IOB: %.2f",getiob(nu));
        nvgText(avg, timex,2*smallfontlineheight+statusbarheight, tbuf,tbuf+len);
        }
#endif

        CURVELOGGER("xpos=%d dwidth=%.1f headsize=%.1f density=%.1f getx=%.1f timex=%.1f\n",xpos,dwidth,headsize, density,getx,timex);
        }
      }
    showlastsstream(avg,nu, getx,used) ;
    }

 void       JCurve::showsavedomain(NVGcontext* avg,const float last, const float dlow,const float dhigh) {
    nvgBeginPath(avg);
    nvgFillColor(avg, unsavecolor);
    nvgRect(avg, dleft, dtop, last-dleft, dhigh);
    nvgFill(avg);

    nvgBeginPath(avg);
    nvgFillColor(avg, unsavecolor);
    nvgRect(avg, dleft, dlow, last-dleft, dheight+dtop);
    nvgFill(avg);
    }
 void    JCurve::showunsaveredline(NVGcontext* avg,const float last,const float dlow) {
    nvgBeginPath(avg);
    nvgStrokeWidth(avg, lowGlucoseStrokeWidth);

    nvgStrokeColor(avg, lowlinecolor);
    nvgMoveTo(avg, dleft,dlow) ;
    nvgLineTo( avg,last ,dlow);
    nvgStroke(avg);
    }


 void       JCurve::showsaverange(NVGcontext* avg,const float last, const float dlow,const float dhigh) {
    showsavedomain(avg,last,dlow,dhigh) ;
    showunsaveredline(avg,last,dlow) ;
    }
        

void        JCurve::showdates(NVGcontext* avg,time_t nu,uint32_t starttime,time_t endtime) {
   CURVELOGGER("duration=%d\n",duration);
    int32_t timdis=nu-starttime;
constexpr const int grens=
#ifdef WEAROS
1
#else
3
#endif
;
CURVELOGGER("timdis=%d duration=%d grens=%d\n",timdis,duration,grens);
if(timdis>0&&((duration/timdis)<grens)) {
       CURVELOGGER("timdis=%d larger than zero\n",timdis);
        const float datehigh=smallfontlineheight*
#ifdef WEAROS
        //.71;
        .68;
#else
        1.5;
        #endif

        char tbuf[70];

        nvgFillColor(avg, *
        #ifdef WEAROS
        getdarkgray()
        #else
        getblack()
        #endif
        );
    float xpos;
   int timelen;
#ifdef WEAROS
//        xpos= dwidth/2+dleft;
        xpos= dwidth*.495f+dleft;

     time_t showtime= (endtime+starttime)/2;
    struct tm tmbuf;
     struct tm *stm=localtime_r(&showtime,&tmbuf);
        nvgTextAlign(avg,NVG_ALIGN_LEFT|NVG_ALIGN_TOP);
      timelen=strlen(usedtext->daylabel[stm->tm_wday]);
      memcpy(tbuf,usedtext->daylabel[stm->tm_wday],timelen);
        nvgTextAlign(avg,NVG_ALIGN_CENTER|NVG_ALIGN_BOTTOM);
        nvgText(avg,xpos ,dheight+datehigh*.15f, tbuf, tbuf+timelen);

       timelen=sprintf(tbuf,"%02d-%02d-%d",stm->tm_mday,stm->tm_mon+1,1900+stm->tm_year);
   
        nvgTextAlign(avg,NVG_ALIGN_CENTER|NVG_ALIGN_TOP);
        nvgText(avg,xpos ,datehigh*.80f+statusbarheight, tbuf, tbuf+timelen);
#else
       const time_t showstarttime=starttime+2*60;
        struct tm tmbufstart;
        localtime_r(&showstarttime,&tmbufstart);
        timelen=sprintf(tbuf,"%s %02d-%02d-%d",usedtext->daylabel[tmbufstart.tm_wday],tmbufstart.tm_mday,tmbufstart.tm_mon+1,1900+tmbufstart.tm_year);
//        timelen=daystr(showstarttime,tbuf);
        xpos= getLevelLeft()?timelen*.75:0;
        nvgTextAlign(avg,NVG_ALIGN_LEFT|NVG_ALIGN_TOP);
        nvgText(avg,xpos ,datehigh+statusbarheight, tbuf, tbuf+timelen);
#endif

        CURVELOGGER("displaytime %s\n",tbuf);
#ifndef WEAROS
       const auto showendtime=endtime-2*60;
        if(nu>=endtime) {
            struct tm tmbufend;
            localtime_r(&showendtime,&tmbufend);
 #define equalday(x) (tmbufend.x==tmbufstart.x)
            if(!(equalday(tm_wday)&& equalday(tm_mday)&& equalday(tm_mon)&& equalday(tm_year))) {
                timelen=sprintf(tbuf,"%s %02d-%02d-%d",usedtext->daylabel[tmbufend.tm_wday],tmbufend.tm_mday,tmbufend.tm_mon+1,1900+tmbufend.tm_year);
                nvgTextAlign(avg,NVG_ALIGN_RIGHT|NVG_ALIGN_TOP);
                nvgText(avg, dwidth+dleft,datehigh+statusbarheight, tbuf, NULL);
                }
#undef equalday
            }
#endif
        }
    }



void    JCurve::showlines(NVGcontext* avg,int gm,int gmax) {
    const uint32_t endtime=starttime+duration;
    gmin=gm;
    grange=gmax-gmin;
    const auto [transx,transy]= gettrans(starttime, endtime);
    displaytime disp=getdisplaytime(UINT_MAX,starttime,endtime, transx);
    const float dlast=dleft+dwidth;
    timelines(avg,&disp,  transx,UINT32_MAX);
    if(disp.tstep>(60*60))
        epochlines(avg,starttime,disp.last,transx);
    glucoselines(avg,dlast,smallfontlineheight,gmax,transy,true) ;
    showunsaveredline(avg,dlast,transy(settings->targetlow()));
    int yhigh=transy(settings->targethigh());
    nvgBeginPath(avg);
    nvgStrokeWidth(avg, lowGlucoseStrokeWidth);
    nvgStrokeColor(avg, dooryellow);
    nvgMoveTo(avg, dleft,yhigh) ;
    nvgLineTo( avg,dwidth,yhigh);
    nvgStroke(avg);
    }
        


extern bool hascalibrations;
bool hascalibrations=false;
int    JCurve::displaycurve(NVGcontext* avg,time_t nu) {
    starttime=(doclamp)?(nu-diffcurrent):(starttime);
    const uint32_t starttime2=starttime;
    const uint32_t endtime=starttime2+duration;
    mealpos.clear();
    hidden.clear(); 
    hists.clear(); 
    sensors->sensorsInPeriod(hists,starttime2,endtime,[this,starttime2](const SensorGlucoseData *s){
        if(s->hide) {
            if(s->lastused()>=starttime2) {
                hidden.push_back(s->sensorIndex);
                }
            return true; 
            }
        return false;
        }
            );
            
    histlen=hists.size();
    CURVELOGGER("displaycurve histlen=%d doclamp=%d starttime=%u\n",histlen,doclamp,starttime2);
    delete[] scanranges;
    scanranges=new pair<const ScanData *,const ScanData*> [histlen];
    delete[] pollranges;
    pollranges=new pair<const ScanData *,const ScanData*> [histlen];
    delete[] histpositions;
    histpositions=new std::remove_reference_t<decltype(histpositions[0])>[histlen];
#ifdef SI5MIN
   bool sibionics[histlen];
#endif
    CURVELOGAR("before getranges");
#ifdef NOCUTOFF
   if(histlen)
      nocutoff=false;
#endif
//   int  maxStreamels=0;
        hascalibrations=false;
    for(int i=histlen-1;i>=0;--i) {
        auto his=sensors->getSensorData(hists[i]);
        if(!his)  {
            CURVELOGAR("getSensorData==null");
            sleep(1);
            return 0;
            }
        if(allvalues||his->getinfo()->calis[0].caliNr||his->getinfo()->calis[1].caliNr)
            hascalibrations=true;
        CURVELOGGER("sensor %s\n",his->showsensorname().data());
            //CURVELOGGER("%s\n",his->othershortsensorname()->data());
        std::span<const ScanData>     scan;
        //if(showscans) 
        {
            scan=his->getScandata();
            scanranges[i] =getScanRange(scan.data(),scan.size(),starttime2,endtime) ;
            }
        //if(showstream) 
        {
            scan=his->getPolldata();
            pollranges[i] =getScanRangeRuim(scan.data(),scan.size(),starttime2,endtime) ;

            }

      const auto senso=his;
      if((showcalibratedhistories||showhistories)&&senso->hasHistory())
            histpositions[i]= histPositions(his, starttime2,  endtime); 
       else
            histpositions[i]= {0,0}; 
         }
    std::unique_ptr<ScanData []> calibratedStream[histlen];
    std::pair<const ScanData*,const ScanData*> caliStreamSpans[histlen];
    if(showcalibratedstream)   {
        typedef decltype(make_calibrator<ScanData>( (const SensorGlucoseData*)nullptr)) CaliType;
        auto califunc=settings->data()->CalibratePast?(&CaliType::makecalibratedback):(&CaliType::makecalibrated);
        for(int i=histlen-1;i>=0;i--) {
            const int index= hists[i];
            const auto *sens=sensors->getSensorData(index);
            if(const int nr=pollranges[i].second-pollranges[i].first;nr>0) {
                LOGGER("pollranges nr=%d\n",nr);
                ScanData*calidata=new ScanData[nr];
                calibratedStream[i].reset(calidata);
                auto cali=make_calibrator<ScanData>(sens);
                caliStreamSpans[i]=(cali.*califunc)(pollranges[i].first,calidata,nr,allvalues);
                }
             else {
                caliStreamSpans[i]={};
                }
            }
         }
     else {
        for(auto &el:caliStreamSpans) {
                el={};
                }
        }
    std::unique_ptr<ScanData []> calibratedScans[histlen];
    std::pair<const ScanData*,const ScanData*> caliScansSpans[histlen];
    if(showcalibratedscans)   {
        typedef decltype(make_calibrator<ScanData>( (const SensorGlucoseData*)nullptr)) CaliType;
        auto califunc=settings->data()->CalibratePast?(&CaliType::makecalibratedback):(&CaliType::makecalibrated);
        for(int i=histlen-1;i>=0;i--) {
            const int index= hists[i];
            const auto *sens=sensors->getSensorData(index);
            if(const int nr=scanranges[i].second-scanranges[i].first;nr>0) {
                ScanData*calidata=new ScanData[nr];
                calibratedScans[i].reset(calidata);
                auto cali=make_calibrator<ScanData>(sens);
                caliScansSpans[i]=(cali.*califunc)(scanranges[i].first,calidata,nr,allvalues);
                }
             else {
                caliScansSpans[i]={};
                }
            }
         }
     else {
        for(auto &el:caliScansSpans) {
                el={};
                }
        }
    CURVELOGGER("Before numdatas[i]->getInRange(%u,%u)\n",starttime2,endtime);
    for(int i=0;i< numdatas.size();i++) 
        extrums[i]=numdatas[i]->getInRange(starttime2, endtime) ;

    const pair<const ScanData *,const ScanData*> *scanpoll[]= {scanranges,pollranges,caliStreamSpans,caliScansSpans};
    CURVELOGAR("Before getextremes");
    if((setend<starttime2||settime>=endtime)) {
       auto extr=getextremes(hists,scanpoll,std::size(scanpoll),histpositions);
       for(int i=0;i<numdatas.size();i++)  {
            CURVELOGGER("%d before extremenums \n",i);
            extr  = numdatas[i]->extremenums(*this,extr);
            }
       setextremes(extr) ;
       }
    CURVELOGAR("before gettrans");
    int  gmax = gmin+grange;
    const auto [transx,transy]= gettrans(starttime2, endtime);
displaytime disp=getdisplaytime(nu,starttime2,endtime, transx);
    const float dlast=nu<endtime?transx(disp.last):dleft+dwidth;
    CURVELOGAR("before showsaverange");
    showsaverange(avg,dlast,transy(settings->targetlow()),transy(settings->targethigh()));

    nvgFontSize(avg, smallsize);
    CURVELOGAR("before showNums");
    const int catnr=settings->getlabelcount();

    showdates(avg,nu,starttime2,endtime) ;

    int nupos=transx(nu); 
    timelines(avg,&disp,  transx,nu);
    if(disp.tstep>(60*60))
        epochlines(avg,starttime2,endtime<nu?endtime:disp.last,transx);
    glucoselines(avg,dlast,smallfontlineheight,gmax,transy,(starttime2+duration/3)<nu) ;

//        nvgCircle(avg, posx,posy,foundPointRadius);

    CURVELOGAR("before showhistories");
    const int colorsleft=nrcolors-catnr;
    const auto segcolor=[catnr,colorsleft,colorseg=colorsleft/4](int index,int seg) {
         return catnr+(index+colorseg*seg)%colorsleft;
         };
    if(showhistories) {
        nvgStrokeWidth(avg, historyStrokeWidth);
        for(int i=histlen-1;i>=0;i--) {
            int index= hists[i];
            int colorindex=segcolor(index,2);
             histcurve(avg,sensors->getSensorData(index), histpositions[i].first, histpositions[i].second,transx,transy,colorindex); 
             }
        }

    if(showcalibratedhistories) {
        nvgStrokeWidth(avg, historyStrokeWidth);
        for(int i=histlen-1;i>=0;i--) {
            int index= hists[i];
            int colorindex=segcolor(index,4);
             calihistcurve(avg,sensors->getSensorData(index), histpositions[i].first, histpositions[i].second,transx,transy,colorindex); 
             }
        }
    CURVELOGGER("before showcalibratedstream %d\n",showcalibratedstream);
    if(showcalibratedstream)   {
        nvgStrokeWidth(avg, pollCurveStrokeWidth);
        for(int i=histlen-1;i>=0;i--) {
            const auto cali=caliStreamSpans[i];
            if(cali.second>cali.first) {
                 const int index= hists[i];
                 const int colorindex=segcolor(index,3);

#ifdef DOESSEARCH
                bool search=calibratedStreamsearchtype==(calibratedStreamsearchtype&searchdata.type);
            #else 
                bool search=false;
            #endif
                showlineScan(avg,cali.first,cali.second,transx,transy,colorindex,search);
                }
            }
        }
    CURVELOGGER("before showstream %d\n",showstream);
    if(showstream)   {
        nvgStrokeWidth(avg, pollCurveStrokeWidth);
        for(int i=histlen-1;i>=0;i--) {
            const int index= hists[i];
            int colorindex=segcolor(index,0);
#ifdef DOESSEARCH
                bool search=streamsearchtype==(streamsearchtype&searchdata.type);
            #else 
                bool search=false;
            #endif
            showlineScan(avg,pollranges[i].first,pollranges[i].second,transx,transy,colorindex,search);
             }
        }
    CURVELOGGER("before showcalibratedscans %d\n",showcalibratedscans);
    if(showcalibratedscans)   {
        nvgStrokeWidth(avg, pollCurveStrokeWidth);
        for(int i=histlen-1;i>=0;i--) {
            const auto cali=caliScansSpans[i];
            if(cali.second>cali.first) {
                const int index= hists[i];
                const int colorindex=segcolor(index,5);
                showScan(avg,cali.first,cali.second,transx,transy,colorindex);
                }
            }
        }
    CURVELOGGER("before showscans %d\n",showscans);
    if(showscans) {
        for(int i=histlen-1;i>=0;i--) {
            const int index=hists[i];
            int colorindex=segcolor(index,1);
            showScan(avg,scanranges[i].first,scanranges[i].second,transx,transy,colorindex);
            }
         }

    CURVELOGGER("before showsnums catnr=%d\n",catnr);
    if(catnr>0&&(shownumbers||showmeals))  {
        bool was[catnr];
        memset(was,'\0',sizeof(was));
        for(auto el:numdatas) 
            el->showNums(*this, transx,  transy,was) ;
        }

    if(nu<endtime&&(dwidth-smallfontlineheight)>nupos) {
        showbluevalue(avg,nu, nupos,usedsensors);
        CURVELOGAR("end display curve value");
        }
    else  {
        CURVELOGAR("end display no value");
#ifndef DONTTALK
        shownglucose.resize(0);
#endif
        }

#ifdef JUGGLUCO_APP
    if(hidden.size()) {
        hasHidden=true;
        showHideButton(avg);
        }
    else
       hasHidden=false;
#endif
 return 0;
}


extern void mkheights() ;

//__attribute__((__visibility__("default"))) extern bool skipdisplay;
//bool skipdisplay=true;

//#define WEAROS


//static void shownumlist(NVGcontext* avg);




       #include <unistd.h>
          #include <sys/types.h>
       #include <sys/stat.h>
       #include <fcntl.h>

#include <string.h>



extern int *numheights;
int *numheights=nullptr;
void mkheights() {
    if(!settings)
        return;
    CURVELOGAR("mkheights() ");
    const int maxl= settings->getlabelcount();
    delete[] numheights;
    numheights=new int[maxl];
    int nr=0;
    for(int i=0;i<maxl;i++) {
        if(settings->getlabelweightmgperL(i)==0.0f) {
            numheights[i]=nr++;
            }
        else
            numheights[i]=-1;
        }
    shownlabels=nr;
    }
#include "net/backup.hpp"
#include "datbackup.hpp"
/*
extern void setuseit();
extern void setusenl();
extern void setusesv();
extern void setuseru() ;
extern void setusees();

extern void setusepl();
extern void setusede();

extern void setusezh() ;
extern void setuseuk() ;
extern void setusebe();
extern void setusefr();

extern void setusept() ;
extern void setuseiw() ;
extern void setuseeng() ;
extern void setusetr();
*/
extern std::string_view localestr;
extern bool hour24clock;
char localestrbuf[10]="en";
std::string_view localestr;
bool hour24clock=true;

#define mklanguagenum2(a,b) a|b<<8
#define mklanguagenum(lang) mklanguagenum2(lang[0],lang[1])
/*
bool chinese() {
    const int16_t lannum=mklanguagenum(localestrbuf);
    switch(lannum) {
        case mklanguagenum("ZH"):
        case mklanguagenum("zh"):
        return true;
        }
    return false;
    }
*/
#ifdef USE_HEBREW
bool hebrew() {
    const int16_t lannum=mklanguagenum(localestrbuf);
    switch(lannum) {
        case mklanguagenum("IW"):
        case mklanguagenum("iw"):
        return true;
        }
    return false;
    }
#endif

#include "destruct.hpp"
void     JCurve::setlocale(NVGcontext* avg,const char *localestrbuf,const size_t len) {
    CURVELOGGER("locale=%s\n",localestrbuf);
    localestr={localestrbuf,len};
    uint16_t langid=mklanguagenumlow(localestrbuf);
    const auto *text=language::gettext(langid);
    ::usedtext=usedtext=text;
    switch(langid) {
#ifdef USE_HEBREW
        case mklanguagenum("IW"):
        case mklanguagenum("iw"):
            if(chfontset!=HEBREW) {
                initfont(avg);
                }
            return;
#endif
        case mklanguagenum("AR"):
        case mklanguagenum("ar"):
            if(chfontset!=ARABIC) {
                initfont(avg);
                }
            return; 
        case mklanguagenum("ZH"):
        case mklanguagenum("zh"):
            if(chfontset!=CHINESE) {
                initfont(avg);
                }
            return;
        };
    if(chfontset!=REST) {
        initfont(avg);
         } 
    }


void  JCurve::calccurvegegs() {
    CURVELOGAR("start calccurvegegs");
    mkheights(); 
    starttime=maxtime()-4*duration/5;
    setusedsensors();
    CURVELOGAR("end calccurvegegs");
    }













#include "numhit.hpp"
extern NumHit newhit;
extern Num newnum;

extern Numdata *getherenums();
Numdata *getherenums() {
    return newhit.numdisplay;
    }
Num newnum;
#include "numhit.hpp"
NumHit newhit={nullptr,&newnum};
#include <assert.h>
int64_t openNums(std::string_view numpath,int64_t ident) {
     const int index=numdatas.size();
     assert(index<maxnumsources);
     NumDisplay* numdata=NumDisplay::getnumdisplay(index, numpath,ident,nummmaplen);
     if(numdata) {
        numdatas.push_back(numdata);
        if(ident==0LL)
            newhit.numdisplay=numdata;
        
        }
    
    CURVELOGGER("index=%d numdir=%s ptr=%p\n",index,numpath.data(),numdata);
    return reinterpret_cast<int64_t>(numdata);
    }

#ifdef WEAROS
#define hourtext "00:00                 "
#else
#define hourtext "00:00                 "
#endif
char hourminstr[hourminstrlen]=hourtext;

void    JCurve::startstepNVG(NVGcontext* avg,int width, int height) {
        nvgBeginFrame(avg, width, height, 1.0);
        const int font=invertcolors?whitefont:blackfont;
        this->font=font;
        nvgFontFaceId(avg,font);
        nvgLineCap(avg, NVG_ROUND);
         nvgLineJoin(avg, NVG_ROUND);
         }


 void    JCurve::showlastsstream(NVGcontext* avg,const time_t nu,const float getx,std::vector<int> &used ) {
//CURVELOGGER("showlaststream %d\n",used.size());
    const auto usedsize=used.size();
#ifdef JUGGLUCO_APP
    int success=false;
    bool neterror=false,usebluetoothoff=false,bluetoothoff=false,otherproblem=false;
    int blueperm=2;
    static int failures=0;
    ++failures;

#ifndef DONTTALK
    shownglucose.resize(usedsize);
    #endif
#endif

    for(int i=0;i<usedsize;i++) {
#ifdef JUGGLUCO_APP
#ifndef DONTTALK
        shownglucose[i].glucosevaluex=-1;
#endif
#endif

        const int sensorindex=used[i];
        SensorGlucoseData *hist=sensors->getSensorData(sensorindex);
        int yh=i*2+1;
#ifdef WEAROS
        float gety=smallsize*.5f+dtop+dheight*yh/(usedsize*2.0f);
#else
        float gety=smallsize*1.4f+dtop+(dheight-smallsize*.8f)*yh/(usedsize*2.0f);
#endif

        uint8_t manualwarmup=hist->getinfo()->manualwarmup;
        if(manualwarmup) {
            time_t starttime=hist->getstarttime();
            int waited=nu-starttime;

            if(waited<(manualwarmup*60)) {
                float usegetx=getx-headsize/3;
                static char buf[256];
                const char *bufptr=buf;
                int waitedminutes=waited/60;
                int ends=sprintf(buf,usedtext->readysec.data(),manualwarmup-waitedminutes);
                 nvgTextBox(avg,  usegetx, gety, getboxwidth(usegetx), bufptr,bufptr+ends);
    #ifndef DONTTALK
                 shownglucose[i].errortext=bufptr;
                 shownglucose[i].glucosevalue=0;
                 shownglucose[i].glucosevaluex=usegetx;
                 shownglucose[i].glucosevaluey=gety+headsize*.5;
    #endif
                continue; 
                 }
                 
            }

        const ScanData *poll=hist->lastValidStream();
        if(poll) {
            CURVELOGAR("poll!=null");
            int age=nu-poll->t;
            if(age<maxbluetoothage) {
                CURVELOGAR("age<maxbluetoothage");
#ifdef JUGGLUCO_APP
                failures=0;
#endif
                nvgBeginPath(avg);
                 nvgFillColor(avg,getoldcolor());
                float relage=(float)age/(float)maxbluetoothage;
                float sensory= gety+headsize/3.1f;
                nvgRect(avg, getx+sensorbounds.left, sensorbounds.top+sensory, relage*sensorbounds.width, sensorbounds.height);
                nvgFill(avg);
                showvalue(avg,poll,hist,getx,gety,i,nu);
#ifdef JUGGLUCO_APP
                success=true;
                if(hist->isLibre2()) {
                     if(settings->data()->libreIsViewed&&!hist->getinfo()->libreviewsendall) {
#ifdef NOTALLVIES
                        if(poll->t>nexttimeviewed) 
#endif
                        {

                            const int addnum= hist->pollcount()-1;
                            if(hist->viewed.empty()||hist->viewed.back()!=addnum) {
                                hist->viewed.push_back(addnum);

#ifdef NOTALLVIES
                                nexttimeviewed=poll->t+betweenviews;
                                CURVELOGGER("add %d nextime=%s",addnum,ctime(&nexttimeviewed));
#endif
                                }
                            }
                        }
                    }
#endif
                }
#ifdef JUGGLUCO_APP
            else {
                CURVELOGAR("age>=maxbluetoothage");
                switch(showerrorvalue(avg,hist,nu,getx,gety,i)) {
                    case 1: neterror=true;break;
                    case 2: usebluetoothoff=true;break;
                    case 3: 
                    blueperm=bluePermission();
                    bluetoothoff=true;
                    break;
                    default:  {
                        blueperm=bluePermission();
                        if(blueperm>1)
                            otherproblem=true;
                        }
                    };
                CURVELOGAR("AFgter showerrorvalue(hist,nu,getx,gety)) ");
                }
            }
        else {
            CURVELOGAR("poll==null");

#ifndef NOTCHINESE
         if(hist->notchinese()) {
             const auto eusibinics=usedtext->unsupportedSibionics;
             nvgText(avg,getx ,gety, eusibinics.data(), eusibinics.data()+eusibinics.size());
             otherproblem=true;
            }
       else 
#endif
       {
           time_t starttime=hist->getstarttime();
           auto wait= nu-starttime;
           const int warmup=hist->getWarmupMIN(); 
           blueperm=bluePermission();
           CURVELOGGER("waited=%lu warmup=%d starttime=%lu %s blueperm=%d\n",wait,warmup,starttime,ctime(&starttime),blueperm);
           bool bluescanner=hist->isSibionics()||hist->isDexcom();
           if(bluescanner&&blueperm<2&&!hasnetwork()) { 
                 float usegetx=getx-headsize/3;
                 nvgTextAlign(avg,NVG_ALIGN_LEFT|NVG_ALIGN_MIDDLE);
                 nvgFontSize(avg,headsize/6 );
                 getboxwidth(usegetx);
                  const std::string_view perm=blueperm==1?usedtext->nolocationpermission:usedtext->nonearbydevicespermission;
                 const auto *bufptr=perm.data();
                 const auto ends= perm.size();
                  otherproblem=true;
                 nvgTextBox(avg,  usegetx, gety, getboxwidth(usegetx), bufptr,bufptr+ends);
#ifndef DONTTALK
                 shownglucose[i].errortext=bufptr;
                 shownglucose[i].glucosevalue=0;
                 shownglucose[i].glucosevaluex=usegetx;
                 shownglucose[i].glucosevaluey=gety+headsize*.5;
#endif
                 }
        else {
          if(wait<(warmup*60)&&((blueperm>0&&!settings->data()->nobluetooth&&bluetoothEnabled())||hasnetwork())) {
             float usegetx=getx-headsize/3;
             nvgTextAlign(avg,NVG_ALIGN_LEFT|NVG_ALIGN_MIDDLE);
             nvgFontSize(avg,headsize/6 );
             getboxwidth(usegetx);
             const char *bufptr;
             int ends;
             if((hist->isAccuChek()||hist->isAir()||hist->isSibionics()||hist->isDexcom())&&!hist->sensorerror){
                  const auto siwait=usedtext->waitingforconnection;
                  bufptr=siwait.data();
                  ends=siwait.size();
                }
             else {
                const bool isInitialised=(!hist->isLibre2())||sensors->getsensor(sensorindex)->initialized;
                CURVELOGGER("wait<(%d*60) isInitialised=%d\n",warmup,isInitialised);
                static char buf[256];
                int minutes=warmup-(wait/60);
                ends=sprintf(buf,isInitialised?usedtext->readysec.data():usedtext->readysecEnable.data(),minutes);
                bufptr=buf;
                }
             nvgTextBox(avg,  usegetx, gety, getboxwidth(usegetx), bufptr,bufptr+ends);
#ifndef DONTTALK
             shownglucose[i].errortext=bufptr;
             shownglucose[i].glucosevalue=0;
             shownglucose[i].glucosevaluex=usegetx;
             shownglucose[i].glucosevaluey=gety+headsize*.5;
#endif
             }
           else   {
               CURVELOGAR("age>=maxbluetoothage");
               switch(showerrorvalue(avg,hist,nu,getx,gety,i)) { //TODO: integrate with same above
                   case 1: neterror=true;break;
                   case 2: usebluetoothoff=true;break;
                   case 3: bluetoothoff=true;break;
                   default: otherproblem=true;
                   };
               CURVELOGAR("Afgter showerrorvalue(hist,nu,getx,gety)) ");
               }
             }
            }
            }

        }

    if(!success&&!otherproblem) {
        int i=0;
#ifndef DONTTALK
        shownglucose.resize(1);
#endif
        CURVELOGAR("showlastsstream: !success&&!otherproblem");
        int newgetx=getx-headsize/3;
        nvgTextAlign(avg,NVG_ALIGN_LEFT|NVG_ALIGN_MIDDLE);
        nvgFontSize(avg,headsize/4 );
        float gety=smallsize*.5f+dtop+dheight/2.0f;
        if(neterror) {
//            nvgText(avg,newgetx ,gety, usedtext->networkproblem.begin(), usedtext->networkproblem.end());
             nvgTextBox(avg,  newgetx, gety, getboxwidth(newgetx), usedtext->networkproblem.begin(), usedtext->networkproblem.end());
#ifndef DONTTALK
             shownglucose[i].glucosevalue=0;
             shownglucose[i].glucosevaluex=newgetx;
             shownglucose[i].glucosevaluey=gety+headsize*.5;
             shownglucose[i].errortext=usedtext->networkproblem.data();
#endif
            }
        else { 
             if(usebluetoothoff) {
                CURVELOGAR("showlastsstream: usebluetoothoff");
                nvgTextBox(avg,newgetx ,gety, getboxwidth(newgetx),usedtext->useBluetoothOff.begin(), usedtext->useBluetoothOff.end());
#ifndef DONTTALK
                shownglucose[i].glucosevalue=0;
                shownglucose[i].glucosevaluex=newgetx;
                shownglucose[i].glucosevaluey=gety+headsize*.5;
                shownglucose[i].errortext=usedtext->useBluetoothOff.data();
#endif
           }
           else {
             CURVELOGGER("blueperm=%d\n",blueperm);
                if(blueperm<1) { 
                    const std::string_view perm=blueperm==1?usedtext->nolocationpermission:usedtext->nonearbydevicespermission;
                    nvgTextBox(avg,newgetx ,gety, getboxwidth(newgetx),perm.begin(), perm.end());
#ifndef DONTTALK
                    shownglucose[i].glucosevalue=0;
                    shownglucose[i].glucosevaluex=newgetx;
                    shownglucose[i].glucosevaluey=gety+headsize*.5;
                    shownglucose[i].errortext=perm.data();
        #endif
                    }
                else {
                   if(bluetoothoff) {
                        nvgTextBox(avg,newgetx ,gety, getboxwidth(newgetx),usedtext->enablebluetooth.begin(), usedtext->enablebluetooth.end());
#ifndef DONTTALK
                        shownglucose[i].glucosevalue=0;
                        shownglucose[i].glucosevaluex=newgetx;
                        shownglucose[i].glucosevaluey=gety+headsize*.5;
                        shownglucose[i].errortext=usedtext->enablebluetooth.data();
#endif
                        }
                    }
                }
                }
        }
    if(failures>2) {
        CURVELOGAR("failures>3" );
        for(int i=0;i<used.size();i++) {
            if(SensorGlucoseData *hist=sensors->getSensorData(used[i])) {
                CURVELOGAR("set waiting=true");
            hist->waiting=true;
                }
            }
        }
#else
        }
    }
#endif

    CURVELOGAR(" end showlastsstream");
    }
/*
int    JCurve::showLargevalue(NVGcontext* avg, int index,float getx,float gety,float convglucose,const ScanData *poll) {
        constexpr const int maxhead=11;
        char head[maxhead];
#ifdef JUGGLUCO_APP
#ifndef DONTTALK
        shownglucose[index].glucosevalue=convglucose;
        shownglucose[index].glucosetrend=poll->tr;
#endif
#endif
         float valuex=getx-(convglucose>=10.0f?density*20.0f:0.0f);
         char *value=head+1;
         int gllen=snprintf(value,maxhead-1,gformat,convglucose);
         if(gllen<3) {
            value=head;
            *value=' ';
            ++gllen;
            }
        nvgText(avg,valuex ,gety, value, value+gllen);
        const float rate=poll->ch;
        drawarrow(avg,rate,valuex-10*density,gety);
        return valuex;
        }
*/
 void    JCurve::showvalue(NVGcontext* avg, const ScanData *poll,const SensorGlucoseData *hist, float getx,float gety,int index,uint32_t nu) {
    const auto sensorname=hist->othershortsensorname();
    CURVELOGGER("showvalue %s\n",sensorname.data());
    float sensory= gety+headsize/3.1;
    nvgFillColor(avg, *getblack());
    nvgFontSize(avg,mediumfont );
    nvgTextAlign(avg,NVG_ALIGN_LEFT|NVG_ALIGN_TOP);
    nvgText(avg, getx,sensory, sensorname.begin(), sensorname.end());
    constexpr const int maxhead=11;
    char head[maxhead];

    nvgTextAlign(avg,NVG_ALIGN_LEFT|NVG_ALIGN_MIDDLE);
    const int nonconvert=poll->getmgdL();
  //  Calibrator<ScanData> cali(hist);
   auto cali=make_calibrator<ScanData>(hist);
    double calibrated=cali.calibrateNow(*poll);
    nvgFontSize(avg, headsize*.8);
#ifdef JUGGLUCO_APP
#ifndef DONTTALK
    shownglucose[index].glucosevaluex=getx;
    shownglucose[index].glucosevaluey=sensory;
#endif
#endif
    if(isnan(calibrated)) {
        if(nonconvert<glucoselowest) {
            const  float valuex=getx;
            int gllen=mkshowlow(head, maxhead) ;
            nvgText(avg,valuex,gety, head, head+gllen);
            return;
            }
        else {
            int glucosehighest=hist->getmaxmgdL();
            if(nonconvert>glucosehighest) {
                float valuex=getx-density*14.0f;
                int gllen=mkshowhigh(head, maxhead,glucosehighest) ;
                nvgText(avg,valuex ,gety, head, head+gllen);
                return;
                }
              }
           calibrated=nonconvert;
           }
        const float convglucose= gconvert(calibrated*10.0);
    #ifdef JUGGLUCO_APP
    #ifndef DONTTALK
        shownglucose[index].glucosevalue=convglucose;
        shownglucose[index].glucosetrend=poll->tr;
#endif
#endif
         float valuex=getx-(convglucose>=10.0f?density*20.0f:0.0f);
         char *value=head+1;
         int gllen=snprintf(value,maxhead-1,gformat,convglucose);
         if(gllen<3) {
            value=head;
            *value=' ';
            ++gllen;
            }
         nvgText(avg,valuex ,gety, value, value+gllen);
        const float rate=poll->ch;
        drawarrow(avg,rate,valuex-10*density,gety);
        if(calibrated!=nonconvert) {
            bounds_t bounds;
            nvgTextBounds(avg, valuex,  gety,value,value+gllen, bounds.array);
              nvgFontSize(avg,mediumfont );
                float nextx=valuex+bounds.xmax-bounds.xmin+density*6;
                if(nonconvert<glucoselowest) {
                    int gllen=mkshowlow(head, maxhead) ;
                    nvgText(avg,nextx,gety, head, head+gllen);
                    }
                else {
                    int glucosehighest=hist->getmaxmgdL();
                    if(nonconvert>glucosehighest) {
                        int gllen=mkshowhigh(head, maxhead,glucosehighest) ;
                        nvgText(avg,nextx ,gety, head, head+gllen);
                        }
                    else {
                        const float rawconv=gconvert(nonconvert*10.0);
                        int gllen=snprintf(head,maxhead,gformat,rawconv);
                        nvgText(avg,nextx,gety, head, head+gllen);
                        }
                    }
         }

    }



float                JCurve::getboxwidth(const float x) {
                    return std::max((float)(dwidth-x-smallsize),dwidth*.25f);
                    }
