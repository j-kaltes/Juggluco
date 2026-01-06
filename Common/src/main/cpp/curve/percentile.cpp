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


#ifndef WEAROS
#include <string.h>
#include <memory>
#include <thread>
#include <time.h>
#include <chrono>

#include "SensorGlucoseData.hpp"
#include "sensoren.hpp"
#include "curve.hpp"
#include "bino.hpp"
#include "jugglucotext.hpp"

#include "JCurve.hpp"
using namespace std::literals;

//typedef GlucoseDataType std::tuple<const ScanData*,const ScanData*,const SensorGlucoseData *>;
#include "GlucoseDataType.hpp" 
extern Sensoren *sensors;
static    constexpr const int seconds_in_day=24*60*60;

extern bool showpers;
bool showpers=false;


#define PERSlogprint(...)


void datainterval(JCurve &jcurve,NVGcontext* vg,float x, float y,uint32_t start,uint32_t end) {
        constexpr const int  maxbuf=100;
        char buf[maxbuf];
        int len=jcurve.datestr(start,buf);
        const char tus[]=" - ";
        constexpr const int tuslen=sizeof(tus)-1;
        memcpy(buf+len,tus,tuslen);
        len+=tuslen;
        len+=jcurve.datestr(end,buf+len);
        nvgText(vg, x,y,buf,buf+len);
        }

static constexpr const int measuresperday=24*60;

template <class GlucoseEl> static const GlucoseEl *firstvalid(const GlucoseEl *start,const GlucoseEl *last,uint32_t nexttime) {
    while(!start->valid()||start->gettime()<nexttime) {
        ++start;
        if(start>=last)
            return nullptr;
        }
    return start;
    }

template <class GlucoseEl> static const GlucoseEl *lastvalid(const GlucoseEl *first,const GlucoseEl *last)  { //TODO: DOESN"T work with HISTORY
    while(!last->valid()) {
        --last;
        if(last==first)
            return first;
        }
    return last;
    }
static int getpercentile(const float frac,const uint16_t *ar,const int len)  {
    float indf=len*frac;
    const int index=indf;
    float over=indf-index;
    if(over>0.001f&&ar[index+1]) {
        if(!ar[index])
            return ar[index+1];
        return ar[index]*(1-over)+ar[index+1]*over;
        }
    else
        return ar[index];
    }

int getseconds(time_t tim) {
    struct tm tmbuf;
    struct tm *stm=localtime_r(&tim,&tmbuf);
    return (stm->tm_hour*60+stm->tm_min)*60+stm->tm_sec;
    }
int getminutes(time_t tim) {
    return round(getseconds(tim)/60.0);
    }
/*
    struct tm tmbuf;
    struct tm *stm=localtime_r(&tim,&tmbuf);
    return stm->tm_hour*60+stm->tm_min;
    */

filter<8> bino;
//filter<4> bino4;
//filter<13> bino10;
//int16_t *m50perc2=nullptr;
//uint16_t *m50perc4=nullptr;
//uint16_t *m50perc10=nullptr;
    extern pathconcat logbasedir;
typedef std::pair<const uint16_t *,const uint16_t*>  roundtype;

struct persgegs {
     uint16_t *  data=nullptr;
    const int *  lens=nullptr;
     int days;
    int idnr;
    int min,max;
roundtype f0,f10,f25;
uint16_t *m50=nullptr;
uint32_t pollstart,polllast;
uint32_t startday,endday;
const jugglucotext *usedtext;
public:
persgegs & operator= (const persgegs &) = delete;
persgegs & operator= ( persgegs &&) = delete;
persgegs( persgegs & ) = delete;    
persgegs( persgegs && ) = delete;    
// persgegs() {}
 persgegs(uint16_t *dat,int *l,int days,int nr,uint32_t pollstart,uint32_t polllast,const jugglucotext *text):data(dat),lens(l),days(days),idnr(nr),pollstart(pollstart), polllast(polllast),usedtext(text) {}
 void remove(roundtype &var) {
     delete var.first;
     delete var.second;
     }
    /*
void savegegs() {
    pathconcat uit(logbasedir,"aggre.dat");
    writeall(uit.data(),data, measuresperday*days*sizeof(data[0]));
    pathconcat lens2(logbasedir,"lens.dat");
    writeall(lens2.data(),lens, measuresperday*sizeof(lens[0]));

    }*/
 ~persgegs() {
    delete[] lens;
     delete[] data; 
    delete[] m50;
    remove(f0);
    remove(f10);
    remove(f25);
     }
    /*
void compare(const uint16_t *ar1,const uint16_t *ar2) {
    PERSlogprint("compare:\n");
    for(int i=0;i<idnr;i++) 
        if(ar1[i]>ar2[i]) {
            PERSlogprint("%d: %d>%d\n",i,ar1[i],ar2[i]);
            }

    }*/
    uint16_t percentile(const int idin,const float frac) const {
        const int id=getid(idin);
        const int len=lens[id];
        if(len<=0)
            return 0;
        return ::getpercentile(frac,data+id*days,len-1);
       }
    int getid(int id) const {
        return id;
//        return (id+minutes)%idnr;
        }

uint16_t    *mkpercentile(const float frac) const { 
        uint16_t*ar    =new uint16_t[idnr];
        for(int i=0;i<idnr;i++)
            ar[i]=percentile(i,frac);
        uint16_t *res=bino.smoothar(ar,idnr);
        delete[] ar;
        return res;
        }

roundtype mkroundpercentile(const float frac) const { 
    return  {mkpercentile(frac),mkpercentile(1.0f-frac)};
    }

    uint16_t getel(int id, int day) {
        return data[getid(id)*days+day];
        }
/*
    void mkmin()  {
        int minh=INT_MAX;
        for(int i=0;i<idnr;i++) {
            if(lens[i]) {
                const int m=data[i*days];
                if(m<minh)
                    minh=m;
                }
            }
        min=minh;    
        }
    void    mkmax()  {
        int maxh=0;
        for(int i=0;i<idnr;i++) {
            const int l=lens[i];
            if(l) {
                const int m=data[i*days+l-1];
                if(m>maxh)
                    maxh=m;
                }
            }
        max= maxh;    
        }
    void mkextreme() {
        mkmin();
        mkmax();
        }
        */
    int getmin() const {
        return min;
        }
    int getmax() const {
        return max;
        }
    int maxall,minall;
    void mkextreme() {
        const uint16_t *p100=f0.second;
        const uint16_t *p0=f0.first;
        maxall=*std::max_element(p100,p100+idnr);
        minall=*std::min_element(p0,p0+idnr);
        }
    void defaultextreme() {
        max=maxall;
        min=minall;
        }
    void setextreme(int minin,int maxin) {
        min=minin;
        max=maxin;
        }
    int isplace(const int start,const int end,const float move,float density,float timelen) const {    
        
        const float minsize=timelen*1.4+5*density;
        const int af=minsize/move;
        const int mid=(end+start)/2;
        const int afmidend=(10*density+timelen*.22)/move;
        const int afend=(10*density)/move;
        const uint16_t *p100=f0.second;
        const uint16_t *beg=p100+start;
        const uint16_t *midar=p100+mid;
        const uint16_t *el1=std::min_element(beg+af,midar-afmidend);
        const uint16_t *el2=std::min_element(midar+af,p100+end-afend);
        const uint16_t *el=*el1<*el2?el1:el2;
        return el-beg;
        }
public:
void mkfracs() {
    f0=mkroundpercentile(0);
    f10=mkroundpercentile(.05);
    f25=mkroundpercentile(.25);
    m50=mkpercentile(.5);
//     savegegs(); saverounds();

//    compare(f10.second,f0.second);
    }
template <typename  T>
void showpercentile(NVGcontext* vg,JCurve &jcurve, T frac,const int startid,const int endid) {
    if(showforwardpercentile(vg,jcurve,frac,startid,endid))
        nvgStroke(vg);
    };

const float getidstep(int dwidth,const int showids) const {
    return ((float)(dwidth))/(showids-1);
    }
inline constexpr const static int maxids=60*24;
int reindex(const int index) const {
//    return (index+maxids)%maxids;
    return index%maxids;
    }
bool showforwardpercentile(NVGcontext* vg,JCurve &jcurve,const uint16_t *perar,const int startid,const int endid) const {
    int showids=endid-startid;
    const float move=getidstep(jcurve.dwidth,showids);
    int ma=getmax();
    int mi=getmin();
    int hi=ma-mi;
    const auto dheight=jcurve.dheight;
    float unit=((float)dheight)/hi;
//    LOGGER("max=%d, min=%d, ver=%d,unit=%.1f\n",ma,mi,hi,unit);
    auto y=[dheight,unit,mi](int gl) {
        return dheight-(gl-mi)*unit;
        };
    auto x=[move,startid](int pos) {
        return (pos-startid)*move;
        };
    int i=startid;
    const int lastid=endid-1;
    for(;!perar[reindex(i)];i++) {
        if(i==lastid)
            return false;
        }
    nvgBeginPath(vg);
    int x1=x(i);
    int y1=y(perar[reindex(i++)]);
    nvgMoveTo(vg, x1,y1);
    for(;i<endid;i++) {
        if(int glu= perar[reindex(i)]) {
            int  x1= x(i);
            int y1= y(glu);
            nvgLineTo( vg,x1,y1);
            }
        }
    return true;
    }
void showbackpercentile(NVGcontext* vg,JCurve &jcurve,const uint16_t *ar,int startid,int endid) const {
    int showids=endid-startid;
    const float move=getidstep(jcurve.dwidth,showids);
    int ma=getmax();
    int mi=getmin();
    int hi=ma-mi;
    auto dheight=jcurve.dheight;
    float unit=((float)dheight)/hi;
    auto y=[dheight,unit,mi](int gl) {
        return dheight-(gl-mi)*unit;
        };
    auto x=[move,startid](int pos) {
        return (pos-startid)*move;
        };
    for(int i=endid-1;i>=startid;i--) {
        if(int glu=ar[reindex(i)]) {
            int  x1= x(i);
            int y1= y(glu);
        //    PERSlogprint("%d\t%d (%.1f)\n",x1,y1,glu/180.f);
            nvgLineTo( vg,x1,y1);
            }
        }
    }
    
void showround(NVGcontext* vg,JCurve &jcurve,roundtype dat,const int startid,const int endid) const {
        if(showforwardpercentile(vg,jcurve,dat.first, startid, endid)) {
            showbackpercentile(vg,jcurve,dat.second,startid,endid);
            nvgClosePath(vg);
            nvgFill(vg);
            }
        }
int effectmin(int min) const {
    return (idnr*min)/(60*24);
    }
int gettimemin(time_t tim) const {
    return effectmin(getminutes(tim));
    }

static constexpr auto lighttest=hexcoloralpha(0x5087CEFA);
//static constexpr auto lighttest=hexcolor(0xF0F8FF);
//static        constexpr const NVGcolor lightblue4={{{0.2f, 0.2f,1.0f,0.25f}}};
//static constexpr auto midlight=hexcolor(0x1E90FF);
static constexpr auto midlight=hexcolor(0x00BFFF);
//static constexpr auto midblue=hexcolor(0x0000FF);
static constexpr auto midblue=hexcolor(0x7B68EE);
//auto darkest=hexcolor(0x00008B);
/*
used:
starttime|
duration)
    extern uint32_t settime;
            extern int gmin;
            extern  int grange;
    smallfontlineheight
smallsize
    statusbarheight
    statusbarleft
    statusbarright;    
    dwidth
    dheight
    density
        pollCurveStrokeWidth;
            statusbarleft
            statusbarheight
            pollstart,polllast
*/
void showpercentiles(NVGcontext* vg,JCurve &jcurve) {
   auto starttime=jcurve.starttime;
    if((jcurve.setend<starttime||jcurve.settime>=(starttime+jcurve.duration))) {
            defaultextreme();
            jcurve.gmin=min;
            jcurve.grange=max-min;
            }
    else  {
            setextreme(jcurve.gmin,jcurve.gmin+jcurve.grange);
            }
      if(jcurve.duration>seconds_in_day)
           jcurve.duration=seconds_in_day;
      if(starttime<startday) {
                jcurve.setstarttime(startday);
                }
        else {
                const uint32_t laststart=endday-jcurve.duration+30*60;
                if(starttime>laststart)
                        jcurve.setstarttime(laststart);
                }


        uint32_t starter=jcurve.starttime-startday;
        auto &globalperc=*this;
        const int startid=globalperc.effectmin(starter/60);
        const int showids=globalperc.effectmin(jcurve.duration/60);
        const int endid=startid+showids;
        LOGGER("startid=%d, endid=%d, showids=%d\n",startid,endid,showids);
        const float move=getidstep(jcurve.dwidth,showids);
        int pos=isplace(startid,std::min(endid,maxids),move,jcurve.density,jcurve.timelen);

        nvgFillColor(vg, lighttest);
        showround(vg,jcurve,f0,startid,endid);
        float startpos=move*pos;;    
        auto smallfontlineheight=jcurve.smallfontlineheight;
        auto density=jcurve.density;
        auto statusbarheight=jcurve.statusbarheight;
        const float ystart=smallfontlineheight*3+statusbarheight,afm=10*density;;
        float ypos=ystart+2* smallfontlineheight;
        nvgBeginPath(vg);
        nvgRect(vg, startpos, ypos,afm , afm);
        nvgFill(vg);

        nvgFillColor(vg, midlight);
        showround(vg,jcurve,f10,startid,endid);
        ypos+=smallfontlineheight;
        nvgBeginPath(vg);
        nvgRect(vg, startpos, ypos,afm , afm);
        nvgFill(vg);
        nvgFillColor(vg, midblue);
        showround(vg,jcurve,f25,startid,endid);
        ypos+=smallfontlineheight;
        nvgBeginPath(vg);
        nvgRect(vg, startpos, ypos,afm , afm);
        nvgFill(vg);
        nvgStrokeColor(vg, *jcurve.getblack());
        nvgStrokeWidth(vg, jcurve.pollCurveStrokeWidth); 
        globalperc.showpercentile(vg,jcurve,m50,startid,endid);
        float linepos=ystart;
        nvgBeginPath(vg);
        nvgMoveTo(vg, startpos,linepos);
        nvgLineTo( vg,startpos+afm,linepos);
        nvgStroke(vg);

        const int gmax=globalperc.getmax();
        nvgFillColor(vg, *jcurve.getblack());
        nvgFontSize(vg, jcurve.smallsize);
        jcurve.showlines(vg,jcurve.gmin,gmax);
        nvgTextAlign(vg,NVG_ALIGN_LEFT|NVG_ALIGN_TOP);
        const float datehigh=smallfontlineheight*1.2;
        datainterval(jcurve,vg,density+jcurve.statusbarleft, datehigh+statusbarheight,pollstart,polllast);
        nvgTextAlign(vg,NVG_ALIGN_RIGHT|NVG_ALIGN_TOP);

        startpos-=5*density;
        ypos=ystart-smallfontlineheight/2;
        nvgText(vg,startpos,ypos,usedtext->median.begin(),usedtext->median.end());
        const string_view  strs[]{usedtext->middle,"100%", "90%","50%"} ;
        ypos=ystart+smallfontlineheight;
        for(auto el:strs) { 
                nvgText(vg,startpos,ypos,el.begin(),el.end());
                ypos+=smallfontlineheight;
                }
        
        }
    };
struct persgegs *globalpercptr=nullptr;



#include "stats.hpp"
#ifdef JUGGLUCO_APP
std::unique_ptr<struct stats> statptr;
extern void showstats(NVGcontext* vg,JCurve&,stats *stat,const jugglucotext *text) ;

bool showsummarygraph=false;
void showpercentiles(NVGcontext* vg) {
    struct stats *st=statptr.get();
    if(st) {
        if(showsummarygraph&&globalpercptr) 
            globalpercptr->showpercentiles(vg,appcurve); 

        else
            showstats(vg,appcurve,st,usedtext);
        }
    }
#endif
//constexpr const int maxdays=50;
//uint16_t alldata[measuresperday*maxdays];

static struct persgegs *matchedminutes( std::vector<GlucoseDataType<const ScanData*>> *polldataptr,uint32_t starttime,uint32_t endtime,const jugglucotext *text) {
    std::vector<GlucoseDataType<const ScanData*>> &polldata=*polldataptr;
    const int days=ceilf(((float)(endtime-starttime))/seconds_in_day)+10;
    uint16_t  *uitdata=new uint16_t[measuresperday*days]();
    const constexpr int bucketsize=60;
    const constexpr uint32_t maxidents= seconds_in_day/bucketsize;
    int  *lens=new int[measuresperday]();
    int prevsaidid=-1;
    int prevglucose=0;
    uint32_t prevtime=starttime,nexttime=0;
    uint32_t pollstart=UINT_MAX, polllast=0;
    constexpr const auto timetoid{ [](time_t tim) { return  getminutes(tim)%maxidents; }};
    int maxid=0;
    int previd=-1;
    for(auto [firstin,lastin,_]:polldata) {
        const ScanData *start=firstvalid(firstin,lastin,nexttime);
        if(!start)
            continue;
        const ScanData *last=lastvalid(start,lastin-1);
        const ScanData *it=start;
        if(start->gettime()<pollstart)
            pollstart=start->gettime();
        if(last->gettime()>polllast)
            polllast=last->gettime();

        prevsaidid=-1;
        for(;it<=last;it++) {
            if(!it->valid()) {
                LOGAR("invalid");
                continue;
                }
            const int saidid=it->getid();
            if(saidid==prevsaidid) {
                PERSlogprint("dub %d==%d\n",saidid,prevsaidid);
                continue;
                }
            uint32_t ittime=it->gettime();
            prevsaidid=saidid;
            int id=timetoid(ittime);
            if(id==previd) {
                id=(previd+1)%maxidents;
                }
            previd=id;
            const int glu=it->getsputnik();

            
            if(prevglucose&&ittime>=(prevtime+110)) {
                const float afst=(ittime-prevtime);
                if(afst>bucketsize&&afst<15*bucketsize) { 
                    const float d=((float)glu-prevglucose)/afst;
                    for(uint32_t i=bucketsize;i<afst;i+=bucketsize) {
                        const int tusid=timetoid(prevtime+i);
                        if(tusid==id)
                            break;
                        const uint16_t tusglu=round(prevglucose+i*d);
                        PERSlogprint("tusglu=%d\n",tusglu);
                        uitdata[tusid*days+lens[tusid]++]=tusglu;
                        }
                    }
                } 
    //        previd=id;
            prevtime=ittime;
            prevglucose=glu;
            uitdata[id*days+lens[id]++]=glu;
            if(id>maxid)
                maxid=id;
            }
        nexttime=prevtime+30;
        }
    int count= std::reduce( lens, lens+ ++maxid);
    if(count<11000)  {
        LOGGER("matchedminutes count=%d<11000\n",count);
        return nullptr;
        }
    LOGAR("end matchedminutes");
    return new persgegs(uitdata,lens,days,maxid,pollstart,polllast,text);
    }

static struct persgegs * sortedmatched( std::vector<GlucoseDataType<const ScanData*>> *polldataptr,const uint32_t start,const uint32_t endt,const jugglucotext *text) {
    persgegs *datastructptr=matchedminutes(polldataptr,start,endt,text) ;
    if(!datastructptr)     {
        return nullptr;
        }
    persgegs &datastruct=*datastructptr;
    int lastid=-1;
    for(int i=0;i<datastruct.idnr;i++) {
        PERSlogprint("%d ",datastruct.lens[i]);
        if(datastruct.lens[i]>1) {
            uint16_t*ar=datastruct.data+i*datastruct.days;
            std::sort(ar,ar+datastruct.lens[i]);
            lastid=i;
            }
        };
    PERSlogprint("\n");
    datastruct.idnr=lastid+1;
    datastruct.mkfracs();
    datastruct.mkextreme();

    LOGAR("end sortedmatched");
    return datastructptr;
    }

extern std::vector<GlucoseDataType<const ScanData*>> getsensorranges(uint32_t start,uint32_t endt,bool calibrated,bool allvalues,bool calibratePast,std::vector<std::unique_ptr<ScanData []>> &calibrates );
//extern std::vector<pair<const ScanData*,const ScanData*>> getsensorranges(uint32_t start,uint32_t endt,bool calibrated,bool allvalues,bool calibratePast,std::vector<std::unique_ptr<ScanData []>> &calibrates ) ;
//extern std::vector<pair<const ScanData*,const ScanData*>> getsensorranges(uint32_t start,uint32_t endt) ;
#ifdef JUGGLUCO_APP
extern void visiblebutton();
void makesummarygraph(std::vector<GlucoseDataType<const ScanData*>> *polldataptr,uint32_t start,uint32_t endt) {
    if(globalpercptr!=nullptr)
        delete globalpercptr;
    globalpercptr=nullptr;

    if(!(globalpercptr=sortedmatched(polldataptr,start,endt,usedtext)))
        return;
    uint32_t startday=appcurve.starttime-getminutes(appcurve.starttime)*60;
    globalpercptr->startday=startday;
    globalpercptr->endday=startday+seconds_in_day;
    appcurve.setend=0;

    visiblebutton() ;
    LOGAR("end makesummarygraph");
    return;
    }
#endif
std::vector<GlucoseDataType<const ScanData*>> polldata;

static std::pair<uint32_t,uint32_t> percStartEnd(uint32_t endt,int days) {
#ifndef NOLOG
    const uint32_t endtin=endt;
    const int daysin=days;
#endif
    auto firsttime=sensors->firstpolltime();
    if(!firsttime)
        return {0,0};
    auto [_id,lasttime]=sensors->lastpolltime();
    int maxdays=1+(lasttime-firsttime)/(60*60*24);
    if(days>maxdays)
        days=maxdays;
    int insecs=days*seconds_in_day;
    if((endt-firsttime)<insecs)
        endt=firsttime+insecs;
    if(endt>lasttime) {
        endt=lasttime;
        }
    uint32_t start=    endt-days*seconds_in_day;
    LOGGER("percStartEnd(%u,%d)=[%u,%u]\n",endtin,daysin,start,endt);
    return {start,endt};
    }

#ifdef JUGGLUCO_APP
static uint32_t statisticsendtime;
static std::vector<std::unique_ptr<ScanData []>> globalcalibrates;
bool mkpercentiles(int days) {
    showsummarygraph=false;
    const auto [start,endt]=percStartEnd(statisticsendtime,days);
    if(start>=endt) return false;
    polldata.clear();
    globalcalibrates.clear();

    bool calibrated=settings->data()->DoCalibrate;
    bool allvalues=true;
    bool calibratePast=settings->data()->CalibratePast;
    polldata=getsensorranges(start,endt,calibrated,allvalues,calibratePast,globalcalibrates);
    if(polldata.size()<=0)
        return false;
    std::thread graph(makesummarygraph,&polldata, start, endt);
    graph.detach();
    statptr=std::make_unique<struct stats>(polldata);
    showpers=true;
    LOGGER("end mkpercentiles(%d)\n",days); 
    return true;
    }

extern bool makepercetages() ;
int daystoanalyse=20;
bool makepercetages() {
    statisticsendtime=appcurve.starttime+appcurve.duration;
    return mkpercentiles(daystoanalyse);
    }
#endif
#include "settings/settings.hpp"
void JCurve::leginterval(NVGcontext* vg,const float x,const float y, const int *between) {
    const constexpr int maxbuf=10;
    char buf[maxbuf];
    int buflen=snprintf(buf,maxbuf,"%.*f-%.*f",gludecimal,::gconvert((between[1]+1)*10,glunit),gludecimal,::gconvert((between[0])*10,glunit));
    nvgText(vg, x,y,buf,buf+buflen);
    }
void stats::showbar(NVGcontext* vg,JCurve &jcurve) {

    int dwidth=jcurve.dwidth-jcurve.statusbarleft-jcurve.statusbarright;    
    int dheight=jcurve.dheight;
    auto stat=this;
    float rowheight=jcurve.smallfontlineheight*1.5;
    float useh=dheight*.8f,starty=(dheight-useh)/2.0f,startx=dwidth*.01f+jcurve.statusbarleft,usewidth=dwidth*.04f;
    float xleg=startx+ dwidth*.01f+usewidth+jcurve.timelen+jcurve.smallsize;
     
    const char perform[]="%.1f%%";
    constexpr int maxbuf=40;
     char perbuf[maxbuf];
    float pery=starty,yleg=starty+rowheight;
    float afm=10*jcurve.density;
constexpr const NVGcolor cols[]={orange,yellow,mediumseagreen,redinit,brown};
    for(int i=0;i<5;i++)     {
        float h=useh*pers[i];
        nvgBeginPath(vg);
        nvgFillColor(vg,cols[i]);
        nvgRect(vg, startx, pery, usewidth,h );
        nvgFill(vg);
        nvgBeginPath(vg);
        nvgRect(vg, xleg, yleg,afm , afm);
        nvgFill(vg);
        yleg+=rowheight;
        pery+=h;
        }
    nvgFontSize(vg, jcurve.smallsize);
    nvgTextAlign(vg,NVG_ALIGN_RIGHT|NVG_ALIGN_TOP);
    nvgFillColor(vg, *jcurve.getblack());
    yleg=starty+rowheight;
    xleg-=afm*.5;
#ifndef NDEBUG
    float xnumpos=dwidth/3+jcurve.dleft-jcurve.smallsize;
#endif
    for(int i=0;i<5;i++)     { 
        const int perlen=snprintf(perbuf,maxbuf,perform,pers[i]*100);
        nvgText(vg, xleg,yleg,perbuf,perbuf+perlen);
#ifndef NDEBUG
        {
        const int perlen=snprintf(perbuf,maxbuf,"%d",counts[i]);
        nvgText(vg, xnumpos,yleg,perbuf,perbuf+perlen);
        }
#endif
        yleg+=rowheight;
        }
        
    xleg+=afm*2;
    nvgTextAlign(vg,NVG_ALIGN_LEFT|NVG_ALIGN_TOP);
    yleg=starty+rowheight;
    string_view unitstr=settings->getunitlabel();
    nvgText(vg, xleg,starty,unitstr.begin(),unitstr.end());
    const int *levels=stat->levels;
    int buflen=snprintf(perbuf,maxbuf,">%.*f",jcurve.gludecimal,::gconvert(*levels*10,jcurve.glunit));
    nvgText(vg, xleg,yleg,perbuf,perbuf+buflen);
    yleg+=rowheight;
    for(int i=0;i<3;i++)     { 
        jcurve.leginterval(vg,xleg,yleg,levels+i);
        yleg+=rowheight;
        }
    buflen=snprintf(perbuf,maxbuf,"<%.*f",jcurve.gludecimal,::gconvert((levels[3]+1)*10,jcurve.glunit));
    nvgText(vg, xleg,yleg,perbuf,perbuf+buflen);
    yleg+=2*rowheight;
    if(memcmp(levels+1,border,2*sizeof(border[0]))) {
        jcurve.leginterval(vg,xleg, yleg, stat->border); 
        const int perlen=snprintf(perbuf,maxbuf,perform,stat->pertarget*100);
        xleg-=afm*2;
        nvgTextAlign(vg,NVG_ALIGN_RIGHT|NVG_ALIGN_TOP);
        nvgText(vg, xleg,yleg,perbuf,perbuf+perlen);
        }

    }


void stats::otherstats(NVGcontext* vg,JCurve &jcurve,const jugglucotext *usedtext) {
     auto  statusbarleft=  jcurve.statusbarleft;
     auto  statusbarright=  jcurve.statusbarright;

    int dwidth=jcurve.dwidth-statusbarleft-statusbarright;    
    int dheight=jcurve.dheight;
//    auto starttime=jcurve.starttime;
    nvgTextAlign(vg,NVG_ALIGN_CENTER|NVG_ALIGN_TOP);
    datainterval(jcurve,vg,dwidth/2, 0,starttime,endtime);
    float rowheight=jcurve.smallfontlineheight*1.5;
    float xpos=dwidth/3+jcurve.dleft+statusbarleft;
    float ypos=0.1*dheight+jcurve.dtop;
    constexpr int maxbuf=70;
    char buf[maxbuf];

    nvgTextAlign(vg,NVG_ALIGN_LEFT|NVG_ALIGN_TOP);
    float days=((double)endtime-starttime)/seconds_in_day;
    int len1=snprintf(buf,maxbuf,usedtext->duration,days);
    nvgText(vg, xpos,ypos,buf,buf+len1);
    ypos+=rowheight;
    len1=snprintf(buf,maxbuf,usedtext->timeactive,active*100);
    nvgText(vg, xpos,ypos,buf,buf+len1);
    ypos+=rowheight;
    len1=snprintf(buf,maxbuf,usedtext->nrmeasurement,count);
    nvgText(vg, xpos,ypos,buf,buf+len1);
    ypos+=rowheight;

    const int avlen= usedtext->averageglucose.size();
    char meanstr[avlen+5];
    memcpy(meanstr, usedtext->averageglucose.data(),avlen);
    int len=avlen+snprintf(meanstr+avlen,5,jcurve.gformat,::gconvert(mean*10,jcurve.glunit));
    nvgText(vg, xpos,ypos,meanstr,meanstr+len);
    ypos+=rowheight;
    len=snprintf(buf,maxbuf,usedtext->EstimatedA1C, EA1Cper, EA1Cmmol);
    nvgText(vg, xpos,ypos,buf,buf+len);
    ypos+=rowheight;
    len=snprintf(buf,maxbuf,usedtext->GMI, GMIper, GMImmol);
    nvgText(vg, xpos,ypos,buf,buf+len);
    ypos+=rowheight;
    len=snprintf(buf,maxbuf,usedtext->SD,jcurve.gconvert(sd*10));
    nvgText(vg, xpos,ypos,buf,buf+len);
    ypos+=rowheight;
    len=snprintf(buf,maxbuf,usedtext->glucose_variability,vc*100);
    nvgText(vg, xpos,ypos,buf,buf+len);
    }
void showstats(NVGcontext* vg,JCurve &jcurve,stats *stat,const jugglucotext *text) {
    stat->showbar(vg,jcurve);
    stat->otherstats(vg,jcurve,text) ;
}



#define NANOVG_RT_IMPLEMENTATION
#define NANORT_IMPLEMENTATION
#include "nanovg_rt.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#ifdef __cplusplus
}
#endif
extern bool savepercentiles(int days);


void backgroundcolor(NVGcontext* avg,const NVGcolor &col) {
    nvgClearBackgroundRT(avg,col.r,col.g, col.b, col.a);
    }

    /*
bool savepercentiles(int days) {
    ::starttime=1746914400;
    ::duration=24*60*60;
    uint32_t statend=starttime+duration;
    const auto [start,endt]=percStartEnd(statend,days);
    LOGGER("savepercentiles %d %d\n",start,endt);
    if(start>=endt)
        return false;
    auto stream=getsensorranges(start,endt);
    if(stream.size()<=0)
        return false;
    LOGGER("savepercentiles stream.size()=%d\n",stream.size());
    auto percptr=sortedmatched(&stream,start,endt);
    startday=::starttime-getminutes(::starttime)*60;
    endday=startday+seconds_in_day;
    setend=0;
    struct stats st(stream);
    int wasdwidth = ::dwidth;
    int wasdheight = ::dheight;
    auto *vg=getfilevg();
//    showstats(vg,&st);
    percptr->showpercentiles(vg,curveimage); 
    delete percptr;
    nvgEndFrame(vg);
     ::dwidth=wasdwidth;
     ::dheight=wasdheight;
    setfontsize(38.5f,44.0f,2.8f,308.0f);  ///get real values
    calccurvegegs();
    appcurve.initfont(::genVG);    
    unsigned char *rgba = nvgReadPixelsRT(vg);
    stbi_write_png("/data/data/tk.glucodata/files/logs/stats.png", winWidth, winHeight, 4, rgba, winWidth * 4);
  //  nvgDeleteRT(vg);
    return true;
    } */
#include "destruct.hpp"


#include <condition_variable>
#include <thread>



/*
static bool runsummaryloop=false;;
static std::condition_variable persloopcond; 
static void makepercentiles();
#include <pthread.h>
#include <limits.h>
static void set_realtime_priority(pthread_t this_thread ) {
     struct sched_param params { .sched_priority = sched_get_priority_max(SCHED_FIFO)};
     if(pthread_setschedparam(this_thread, SCHED_FIFO, &params)) {
        lerror("pthread_setschedparam failed");
         return;     
         }
     int policy = 0;
     if(pthread_getschedparam(this_thread, &policy, &params)) {
        lerror("pthread_getschedparam failed");
        return;
        }
     if(policy != SCHED_FIFO) 
        LOGAR("Scheduling is NOT SCHED_FIFO!");
      else 
         LOGAR("SCHED_FIFO OK");
    LOGGER("Thread priority is %d\n", params.sched_priority); 
     }
static pthread_t persthread=0;
void highpriority() {
    if(persthread)
         set_realtime_priority(persthread);
    }
    */
    /*
#define highpriority() 
#include <sys/time.h>
#include <sys/resource.h>
void showpriority() {
    LOGGER("priority()=%d\n",getpriority(PRIO_PROCESS,0));
    }

static std::mutex persloopmutex;
void mksummaryimageloop() {
//    persthread=pthread_self();
    showpriority();
    setpriority(PRIO_PROCESS,0,-20);
    showpriority();
    while(runsummaryloop) {
        makepercentiles();
        std::unique_lock<std::mutex> lck(persloopmutex);
       // constexpr const int waitsec=60*60;
        //persloopcond.wait_for(lck,std::chrono::seconds(waitsec));   
        persloopcond.wait(lck);   
        }
    }
void wakemksummary(){
     std::lock_guard<std::mutex> lck(persloopmutex);
     persloopcond.notify_all();                        
     }
void startsummarythread() {
   if(!runsummaryloop)  {
        runsummaryloop=true;;
        std::thread th{ mksummaryimageloop};
        th.detach();
        }
    }
void stopsummarythread() {
    // persthread=0;
     runsummaryloop=false;
     wakemksummary();
     }
*/
void startsummarythread() {
    }
void stopsummarythread() {
    }

#include "net/watchserver/Getopts.hpp"
#include "net/watchserver/watchserver.hpp"
#include "CircularArray.hpp"

struct PercentileGraph {
        Getopts opts; 
        recdata image;
        char *startimage; 
        PercentileGraph() {
            LOGGER("PercentileGraph() this=%p\n",this);
            }
//        PercentileGraph(const Getopts &opts,const recdata &image) = delete;
        PercentileGraph(const Getopts &opts,const recdata &im,char *start): opts(opts),image(im),startimage(start) { 
                LOGGER("this=%p PercentileGraph(Getopts &opts,recdata &in) allbuf=%p\n",this,image.allbuf); 
                }
        PercentileGraph(Getopts &&opts,recdata &&in,char *start): opts(opts),image(in),startimage(start) {

                in.allbuf=nullptr;
                LOGGER("this=%p PercentileGraph(Getopts &&opts,recdata &&in) allbuf=%p\n",this,image.allbuf); 
                }
      PercentileGraph(const PercentileGraph & in) =delete;
      PercentileGraph(PercentileGraph & in) =delete;
      PercentileGraph(PercentileGraph && in): opts(in.opts),image(in.image),startimage(in.startimage) {
            in.image.allbuf=nullptr; 
            LOGGER("this=%p PercentileGraph(PercentileGraph && in) allbuf=%p\n",this,image.allbuf);
            }
        ~PercentileGraph() {
            LOGGER("~PercentileGraph() this=%p allbuf=%p\n", this,image.allbuf);
            delete[] image.allbuf;
            };
         PercentileGraph& operator =(const PercentileGraph& b) =delete;
         PercentileGraph& operator =(PercentileGraph&& b) {
            opts=b.opts;
            image=b.image;
            startimage=b.startimage;
            b.image.allbuf=nullptr;
            LOGGER("this=%p PercentileGraph& operator =(PercentileGraph&& b) allbuf=%p\n",this,image.allbuf);
           return *this; 
            }
        };
#include <atomic>

class PersImages:public CircularArray<20,PercentileGraph> { 
public:
std::mutex mutex;
PercentileGraph *get(Getopts &opt,bool absolute) {
        const std::lock_guard<std::mutex> lock(mutex);
        auto beg=begin();
        for(auto iter=end()-1;iter>=beg;--iter) {
                if(opt.aboutequal(iter->opts,absolute)) {
                        return &iter[0];
                        }
                }
        return nullptr;
        }
};
extern PersImages persimages;
PersImages persimages;
extern void mktypeheader(char *outstart,char *outiter,const bool headonly,recdata *outdata,std::string_view type,std::string_view origin);

extern bool givesummarygraph(Getopts &opts,std::string_view origin,recdata *outdata);
static std::vector<Getopts> newopts;

    /*
bool hassummary(Getopts &opts) {
    return false;
    }
static bool  queuedata(Getopts &opts,const char *label) {
       const bool newdata=std::ranges::find_if(newopts,[&opts](Getopts &old) { return opts.aboutequal(old);})==newopts.end();
        LOGGER("%s: NOT start=%u end=%u darkmode=%d%s %.2s\n",label,opts.start,opts.end,opts.darkmode, newdata?"":" already queued",(char *)&opts.lang);
        if(newdata) {
            newopts.push_back(opts);
            }
        wakemksummary();
        return newdata;
        }
bool hassummary(Getopts &opts) {
       if(persimages.get(opts)) {
            LOGGER("hassummary start=%u end=%u darkmode=%d\n",opts.start,opts.end,opts.darkmode);
            return true;
            }
        queuedata(opts,"hassummary");
        return false;
        }
*/
//static bool givepercentiles(Getopts &opts,recdata *outdata);


static  char * givepercentiles(Getopts &opts,uint32_t start, uint32_t endt,recdata *outdata,std::string_view origin) ;
static std::pair<uint32_t,uint32_t> percStartEnd(Getopts &opts);
bool givesummarygraph(Getopts &opts,std::string_view origin,recdata *outdata) {
    const auto [startt,endt]=percStartEnd(opts);
    if(startt>=endt) {
        LOGAR("givesummarygraph no data");
        return false;
        }
//   LOGGER("opts.end=%u endt=%d diff=%d
   opts.start=startt;
   const bool absolute=(opts.end-endt)>(10*60);
   opts.end=endt;
    if(PercentileGraph *hit=persimages.get(opts,absolute)) {
        const auto &rec=hit->image;
        LOGGER("persimages.get %p\n", rec.allbuf);
        mktypeheader(hit->startimage,(char *)rec.start+rec.len,false,outdata,"image/png"sv,origin);
        outdata->allbuf=nullptr;
        LOGGER("givesummarygraph: found result opts.start=%u opts.end=%u\n",opts.start,opts.end);
        return true;
        }
    if(char *startimage=givepercentiles(opts,startt,endt,outdata,origin)) {
          LOGGER("persimages.emplace_back %p\n", outdata->allbuf);
              {
              const std::lock_guard<std::mutex> lock(persimages.mutex);
              persimages.emplace_back(opts,*outdata,startimage);
              }
          outdata->allbuf=nullptr;
          return true;
          }
    return false;
    }

constexpr float devidewith=3;
constexpr  int winWidth = 1536*3/devidewith;
constexpr   int winHeight = 1080/devidewith;



std::span<char> getStatImage(int startpos,Getopts &opts) {
//    const bool has=hassummary(opts);
    JCurve statimage(opts.unit?opts.unit:settings->data()->unit);
    uint32_t startsecs=opts.start;
    uint32_t endsecs=opts.end;
    statimage.invertcolorsset(opts.darkmode);
    int duration=statimage.duration=24*60*60;
    statimage.starttime=endsecs-duration;
    int days=(endsecs-startsecs+duration-1)/duration;
    const auto [start,endt]=percStartEnd(endsecs,days);
    LOGGER("getStatImage %d %d days=%d\n",start,endt,days);
    if(start>=endt)
        return {(char *)nullptr,0};
    std::vector<std::unique_ptr<ScanData []>> calibrates;
    bool calibratePast=settings->data()->CalibratePast;
    auto stream=getsensorranges(start,endt,opts.calibratedmode,opts.allvaluesmode,calibratePast,calibrates);
    if(stream.size()<=0)
        return {(char *)nullptr,0};
    const auto *text=language::gettext(opts.lang);
    statimage.usedtext=text;
//    constexpr int winHeight=640*mult;
    const int winHeight=opts.height?opts.height:256;
    const int winWidth=opts.width?opts.width:768;
    const double mult=winHeight/512.0;
    LOGGER("getStatImage %d %d days=%d width=%f height=%f mult=%f\n",start,endt,days,winWidth,winHeight,mult);
    auto vg = nvgCreateRT(0, winWidth, winHeight);
    destruct _{[vg]{nvgDeleteRT(vg);}};
    statimage.dheight=winHeight;
    statimage.dwidth=winWidth;
    //statimage.setfontsize(38.5*mult,44.0*mult,2.8*mult,308.0*mult); 
    statimage.setfontsize(38.5*mult,44.0*mult,2.8*mult,250.0*mult);
 
    statimage.initfont(vg);
    backgroundcolor(vg,*statimage.getwhite());
    statimage.startstepNVG(vg,winWidth,winHeight);
    struct stats st(stream);
    showstats(vg,statimage,&st,text);
    nvgEndFrame(vg);
    unsigned char *rgba = nvgReadPixelsRT(vg);
    int len=0;
   unsigned char *png = stbi_write_png_to_mem(startpos,rgba, winWidth*4, winWidth, winHeight, 4, &len);
    return {(char *)png,static_cast<std::size_t>(len)};
    }



static NVGcontext* getfilevg(JCurve &curveimage,int width,int height) {
    double multiply=height/800.0;
    LOGGER("multiply=%f\n",multiply);
//constexpr   int winHeight = 1080/devidewith;
    auto vg = nvgCreateRT(0, width, height);
    curveimage.dheight=height;
    curveimage.dwidth=width;
//    curveimage.setfontsize(38.5f*multiply,44.0f*multiply,2.8f*multiply,308.0f*multiply);
    curveimage.setfontsize(38.5*multiply,44.0*multiply,2.8*multiply,250.0*multiply);
    curveimage.initfont(vg);
    backgroundcolor(vg,*curveimage.getwhite());
    curveimage.startstepNVG(vg,width,height);
    return vg;
    }


extern bool isLargeCurve(Getopts &opts);
bool isLargeCurve(Getopts &opts) {
        if(opts.width||opts.height) {
            const int width=opts.width?opts.width:winWidth;
            const int height=opts.height?opts.height:winHeight;
            const double curvetime=settings->data()->loadtime;
            return ((width*height)*curvetime)>20.0 ;
            }
        return false;
        }
std::span<char> getCurveImage(int startpos,Getopts &opts) {
    int width=opts.width?opts.width:winWidth;
    int height=opts.height?opts.height:winHeight;
  
  if(!(opts.calibratedmode||opts.streammode||opts.scansmode||opts.historymode||opts.amountsmode||opts.mealsmode)) {
        if(settings->data()->DoCalibrate) {
                opts.calibratedmode=true;
                opts.allvaluesmode=true;
                }
        else
            opts.streammode=true;
        opts.scansmode=true;
        opts.amountsmode=true;
        }

    const long double started=clock();
    uint32_t startsecs=opts.start;
    uint32_t endsecs=opts.end;
    JCurve curveimage(opts.unit?opts.unit:settings->data()->unit);
    const auto *text=language::gettext(opts.lang);
    curveimage.usedtext=text;
    curveimage.showstream=opts.streammode;
    curveimage.showscans=opts.scansmode;
    curveimage.showmeals=opts.mealsmode;
    curveimage.showhistories=opts.historymode;
    curveimage.shownumbers=opts.amountsmode;
    curveimage.invertcolorsset(opts.darkmode);
    curveimage.allvalues=opts.allvaluesmode;
    curveimage.showcalibratedstream=opts.calibratedmode;
    curveimage.showcalibratedhistories=opts.calibratedhistorymode;


    curveimage.glow=curveimage.userunit2mgL(opts.glow);
    curveimage.ghigh=curveimage.userunit2mgL(opts.ghigh);
    LOGGER("getCurveImage start=%u end=%u width=%d height=%d\n", startsecs, endsecs,width,height);
    NVGcontext* vg=getfilevg(curveimage,width,height);
    destruct _{[vg]{nvgDeleteRT(vg);}};
    curveimage.starttime=startsecs; 
    curveimage.duration=endsecs-startsecs;
    curveimage.displaycurve(vg,time(nullptr));
    nvgEndFrame(vg);
    unsigned char *rgba = nvgReadPixelsRT(vg);
    int len;
    unsigned char *png = stbi_write_png_to_mem(startpos,rgba, width*4, width, height, 4, &len);
    const long double stopped=clock();
    double clocksecs=(stopped-started)/CLOCKS_PER_SEC;
    settings->data()->loadtime=clocksecs/(width*height);
    return {(char *)png,static_cast<std::size_t>(len)};
    }


#include "secs.h"

static std::pair<uint32_t,uint32_t> percStartEnd(Getopts &opts) {
    if(!opts.width)
        opts.width=winWidth;
    if(!opts.height)
        opts.height=winHeight;
 
    uint32_t endsecs=opts.end;
    uint32_t startsecs=opts.start;
    constexpr int showdur=daysecs;
    int days=(endsecs-startsecs+showdur-1)/showdur;
    return percStartEnd(endsecs,days);
    }

static  char * givepercentiles(Getopts &opts,uint32_t start, uint32_t endt,recdata *outdata,std::string_view origin) {
#ifndef NOLOG
    const auto started=clock();
#endif
    constexpr int showdur=daysecs;
    if(start>=endt) {
        LOGAR("givepercentiles: start>=endt");
        return nullptr;
        }

    std::vector<std::unique_ptr<ScanData []>> calibrates;
    bool calibratePast=settings->data()->CalibratePast;
    auto stream=getsensorranges(start,endt,opts.calibratedmode,opts.allvaluesmode,calibratePast,calibrates);
    if(stream.size()<=0) {
        LOGAR("givepercentiles: stream.size()<=0");
        return nullptr;
        }
    const auto *text=language::gettext(opts.lang);
    auto percptr=sortedmatched(&stream,start,endt,text);
    if(!percptr)  {
        LOGAR("sortedmatched==null");
        return nullptr;
        }
    int width=opts.width;
    int height=opts.height;
    uint32_t starttime=endt-showdur;
    uint32_t startday=starttime-getminutes(starttime)*60;
    JCurve perscurve(opts.unit?opts.unit:settings->data()->unit);
    perscurve.duration=showdur;
    perscurve.starttime=startday;
    bool darkmode=opts.darkmode;
    perscurve.dheight=height;
    perscurve.dwidth=width;
    perscurve.invertcolorsset(darkmode);

    perscurve.usedtext=text;
    percptr->startday=startday;
    percptr->endday=startday+seconds_in_day;
    perscurve.setend=0;


    double multiply=height/800.0;
    LOGGER("multiply=%f\n",multiply);
    //perscurve.setfontsize(38.5f*multiply,44.0f*multiply,2.8f*multiply,308.0f*multiply); 
    perscurve.setfontsize(38.5*multiply,44.0*multiply,2.8*multiply,250.0*multiply);
    auto vg = nvgCreateRT(0, width, height);
    destruct _{[vg]{nvgDeleteRT(vg);}};
    perscurve.initfont(vg);

    backgroundcolor(vg,*perscurve.getwhite());

    perscurve.startstepNVG(vg,width,height);

    percptr->showpercentiles(vg,perscurve); 
    LOGAR("givepercentiles: after showpercentiles");
    delete percptr;
    nvgEndFrame(vg);
    unsigned char *rgba = nvgReadPixelsRT(vg);
    constexpr const int startpos=152;
    int len;
    char *imagestart = reinterpret_cast<char *>(stbi_write_png_to_mem(startpos,rgba, width*4, width, height, 4, &len));
    if(!imagestart) {
        LOGAR("givepercentiles: no image");
        return nullptr;
        }
    delete[] outdata->allbuf;
    outdata->allbuf=imagestart-startpos;
    char *imageend=imagestart+len;
    constexpr const std::string_view imagetype{"image/png"sv};
    LOGAR("givepercentiles before mktypeheader");
    mktypeheader(imagestart,imageend,false,outdata,imagetype,origin);
#ifndef NOLOG
    const auto stopped=clock();
    double clockduration=((( long double)stopped-started)*1000)/CLOCKS_PER_SEC;
    LOGGER("end givepercentiles start=%u end=%u darkmode=%d loadtime=%lf lang=%.2s\n", opts.start, opts.end, darkmode,clockduration,(const char *)&opts.lang);
#endif
    return imagestart;
    }
#endif

