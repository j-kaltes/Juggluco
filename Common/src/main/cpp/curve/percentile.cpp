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
#include <new>
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

//typedef GlucoseDataType std::tuple<StreamIterator,StreamIterator,const SensorGlucoseData *>;
#include "GlucoseDataType.hpp" 

#include "getsensorranges.hpp"
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

//static constexpr const int measuresperday=24*60;

template <class GlucoseIterator> static const GlucoseIterator firstvalid(GlucoseIterator start,const GlucoseIterator last,uint32_t nexttime) {
    while(!start->valid()||start->gettime()<nexttime) {
        ++start;
        if(start>=last)
            return last;
        }
    return start;
    }

template <class GlucoseIterator> static const GlucoseIterator lastvalid(const GlucoseIterator first,GlucoseIterator last)  { //TODO: DOESN"T work with HISTORY
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
//filter<4> bino;
//filter<3> bino;
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
bool doSmooth=false;
const int minperstep;
const  int maxids;
public:
persgegs & operator= (const persgegs &) = delete;
persgegs & operator= ( persgegs &&) = delete;
persgegs( persgegs & ) = delete;    
persgegs( persgegs && ) = delete;    
// persgegs() {}
 persgegs(uint16_t *dat,int *l,int days,int nr,uint32_t pollstart,uint32_t polllast,const jugglucotext *text,int minperstep,int maxids):data(dat),lens(l),days(days),idnr(nr),pollstart(pollstart), polllast(polllast),usedtext(text),doSmooth(minperstep<5),minperstep(minperstep),maxids(maxids) {}
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
        if(doSmooth) {
            uint16_t *res=bino.smoothar(ar,idnr);
            delete[] ar;
            return res;
            }
        return ar;
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
    int isplace(const int start,const int end,const float move,float density,float timelen,float placeback) const {    
        LOGGER("isplace(start=%d,end=%d,move=%.2f,density=%.2f,timelen=%.2f)\n", start,end,move, density, timelen) ;  
        const float minsize=timelen*1.4+5*density;
        const int af=std::ceil(minsize/move);
        const int mid=(end+start)/2;
        const int afmidend=std::ceil((10*density+timelen*.22)/move);
        const int afend=std::ceil((10*density)/move);
        const uint16_t *p100=f0.second;
        const uint16_t *beg=p100+start;
        const uint16_t *midar=p100+mid;
       
        const uint16_t *el1start=beg+af;
        const uint16_t *el1end=midar-afmidend;
        LOGGER("inplace: el1=min_element(%d,%d)\n",el1start-p100,el1end-p100);
        const uint16_t *el1;
        if(el1end>el1start) {
            el1=std::min_element(el1start,el1end);
            }
        else
            el1=nullptr;
        const uint16_t *el2start=midar+af;
        const uint16_t *el2end= p100+end-afend;
        LOGGER("inplace: el2=min_element(%d,%d)\n",el2start-p100,el2end-p100);
        const uint16_t *el2;
        if(el2end>el2start) {
            el2=std::min_element(el2start,el2end);
            }
        else
            el2=nullptr;
        const uint16_t *el=!el1?el2:(!el2?el1:(*el1<*el2?el1:el2));
        return el-beg;
        }



    int isplacefromleft(const int start,const int end,const float move,float density,float timelen,float placeback) const {    
        
        const float minsize=timelen*1.4+5*density;
        const int af=minsize/move;
        const int afmidend=(10*density+timelen*.6+placeback)/move;
        const int afend=(10*density)/move;
        const uint16_t *p100=f0.second;
        const uint16_t *beg=p100+start;
        const uint16_t *el=std::min_element(beg+af+afmidend,p100+end-afend);
//        const uint16_t *el=std::min_element(beg+(int)timelen,p100+end-afend);
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
void showpercentile(NVGcontext* vg,JCurve &jcurve, T frac,const int startid,const int endid,float placeback,float move) {
    if(showforwardpercentile(vg,jcurve,frac,startid,endid,placeback,move))
        nvgStroke(vg);
    };
/*
const float getidstep(int dwidth,const int showids) const {
    return ((float)(dwidth))/(showids-1);
    } */
int reindex(const int index) const {
//    return (index+maxids)%maxids;
    return index%maxids;
    }
bool showforwardpercentile(NVGcontext* vg,JCurve &jcurve,const uint16_t *perar,const int startid,const int endid,float placeback,float move) const {
//    int showids=endid-startid;
//    const float move=getidstep(jcurve.dwidth,showids);
    int ma=getmax();
    int mi=getmin();
    int hi=ma-mi;
    const auto dheight=jcurve.dheight;
    float unit=((float)dheight)/hi;
//    LOGGER("max=%d, min=%d, ver=%d,unit=%.1f\n",ma,mi,hi,unit);
    auto y=[dheight,unit,mi](int gl) {
        return dheight-(gl-mi)*unit;
        };
    auto x=[move,startid,placeback](int pos) {
        return (pos-startid)*move-placeback;
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
            float  x1= x(i);
            float y1= y(glu);
            nvgLineTo( vg,x1,y1);
           // LOGGER("%d: x=%.2f\n",i,x1);

            }
        }
    return true;
    }
void showbackpercentile(NVGcontext* vg,JCurve &jcurve,const uint16_t *ar,int startid,int endid,float placeback,float move) const {
//    int showids=endid-startid;
//    const float move=getidstep(jcurve.dwidth,showids);
    int ma=getmax();
    int mi=getmin();
    int hi=ma-mi;
    auto dheight=jcurve.dheight;
    float unit=((float)dheight)/hi;
    auto y=[dheight,unit,mi](int gl) {
        return dheight-(gl-mi)*unit;
        };
    auto x=[move,startid,placeback](int pos) {
        return (pos-startid)*move-placeback;
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
    
void showround(NVGcontext* vg,JCurve &jcurve,roundtype dat,const int startid,const int endid,float placeback,float move) const {
        if(showforwardpercentile(vg,jcurve,dat.first, startid, endid,placeback,move)) {
            showbackpercentile(vg,jcurve,dat.second,startid,endid,placeback,move);
            nvgClosePath(vg);
            nvgFill(vg);
            }
        }
int effectmin(int min) const {
    return (idnr*min)/(60*24);
    }
    /*
int gettimemin(time_t tim) const {
    return effectmin(getminutes(tim));
    } */

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
//                const uint32_t laststart=endday-jcurve.duration+24*60*60;
                if(starttime>laststart)
                        jcurve.setstarttime(laststart);
                }


        uint32_t starter=jcurve.starttime-startday;
        auto &globalperc=*this;
        const int startid=globalperc.effectmin(starter/60);
        const uint32_t startidtime=startid*minperstep*60;
        const int timeback=starter-startidtime;
        const float placeback=jcurve.dwidth*timeback/jcurve.duration;

        const double staridf=(idnr*starter)/(60.0*24.0*60.0);

        int showids=globalperc.effectmin((jcurve.duration+timeback)/60+minperstep-1);
        const float move=(minperstep*60.0f*jcurve.dwidth/jcurve.duration);
//        getidstep(jcurve.dwidth,showids);
        ++showids;

        const int endid=startid+showids;
#ifndef NOLOG
        const float placeback2=(staridf-startid)*move;
        LOGGER("showpercentiles starttime=%zu startidtime=%zu duration=%zu endidtime=%zu startid=%d, endid=%d, showids=%d move=%.2f timeback=%d placeback=%2.f placeback2=%.2f\n",starttime,startidtime,jcurve.duration,endid*minperstep*60,startid,endid,showids,move,timeback,placeback,placeback2);
#endif
        int pos= (this->*((settings->data()->levelleft)?&persgegs::isplacefromleft:&persgegs::isplace))(startid,std::min(endid,maxids),move,jcurve.density,jcurve.timelen,placeback);

        nvgFillColor(vg, lighttest);
        showround(vg,jcurve,f0,startid,endid,placeback,move);
        float startpos=move*pos-placeback;;    
        auto smallfontlineheight=jcurve.smallfontlineheight;
        auto density=jcurve.density;
        auto statusbarheight=jcurve.statusbarheight;
        const float ystart=smallfontlineheight*3+statusbarheight,afm=10*density;;
        float ypos=ystart+2* smallfontlineheight;
        nvgBeginPath(vg);
        nvgRect(vg, startpos, ypos,afm , afm);
        nvgFill(vg);

        nvgFillColor(vg, midlight);
        showround(vg,jcurve,f10,startid,endid,placeback,move);
        ypos+=smallfontlineheight;
        nvgBeginPath(vg);
        nvgRect(vg, startpos, ypos,afm , afm);
        nvgFill(vg);
        nvgFillColor(vg, midblue);
        showround(vg,jcurve,f25,startid,endid,placeback,move);
        ypos+=smallfontlineheight;
        nvgBeginPath(vg);
        nvgRect(vg, startpos, ypos,afm , afm);
        nvgFill(vg);
        nvgStrokeColor(vg, *jcurve.getblack());
        nvgStrokeWidth(vg, jcurve.pollCurveStrokeWidth); 
        globalperc.showpercentile(vg,jcurve,m50,startid,endid,placeback,move);
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

#include "stats.hpp"
#ifdef JUGGLUCO_APP
#endif
//constexpr const int maxdays=50;
//uint16_t alldata[measuresperday*maxdays];
template <typename GlucoseIterator>
static struct persgegs *matchedminutes( std::vector<GlucoseDataType<GlucoseIterator>> *polldataptr,uint32_t starttime,uint32_t endtime,const jugglucotext *text) {
    std::vector<GlucoseDataType<GlucoseIterator>> &polldata=*polldataptr;
    int maxdist=0;
    for(auto [_a,_b,dist,_cal,_]:polldata) { 
        if(dist>maxdist)
            maxdist=dist;
        }
    if(maxdist==0) {
        LOGAR("matchedminutes: maxdist==0");
        return nullptr;
        }
    int measuresperday=24*60/maxdist;
    LOGGER("matchedminutes maxdist=%d measuresperday=%d\n",maxdist, measuresperday);
    const int days=ceilf(((float)(endtime-starttime))/seconds_in_day)+10;
    uint16_t  *uitdata=new uint16_t[measuresperday*days]();
    const int bucketsize=60*maxdist;
    const int nearlytwo=2*bucketsize-10;
    const int mindistance=50*maxdist;
    const uint32_t maxidents= seconds_in_day/bucketsize;
    int  *lens=new int[measuresperday]();
    int prevsaidid=-1;
    int prevglucose=0;
    uint32_t prevtime=starttime,nexttime=0;
    uint32_t pollstart=UINT_MAX, polllast=0;
    const auto timetoid{ [maxidents,maxdist](time_t tim) { return  (getminutes(tim)/maxdist)%maxidents; }};
    int maxid=0;
    int previd=-1;
    for(auto [firstin,lastin,_a,calibrated,cali]:polldata) {
        auto start=firstvalid(firstin,lastin,nexttime);
        if(start==lastin)
            continue;
        auto last=lastvalid(start,lastin-1);
        auto it=start;
        if(start->gettime()<pollstart)
            pollstart=start->gettime();
        if(last->gettime()>polllast)
            polllast=last->gettime();

        prevsaidid=-1;
        for(;it<=last;it++) {
            if(!it->valid()) {
                LOGGER("matchedminutes invalid %d id=%d time=%u\n",it-firstin,it->getid(),it->gettime());
                continue;
                }
            const int saidid=it->getid();
            if(saidid==prevsaidid) {
                PERSlogprint("matchedminutes dub %d==%d\n",saidid,prevsaidid);
                continue;
                }
            uint32_t ittime=it->gettime();
            if(ittime<nexttime) {
                PERSlogprint("matchedminutes too early nexttime=%zd thistime=%zd %.1f id=%d\n",nexttime,ittime,it->getmmolL(),it->getid());
                continue;
                }
            prevsaidid=saidid;
            int id=timetoid(ittime);
            if(id==previd) {
                PERSlogprint("matchedminutes id==previd prevtime %zu time %zu\n",prevtime,ittime);
                id=(previd+1)%maxidents;
                }
            previd=id;
            int glu;
            if(double  calValue;calibrated&&!isnan(calValue=cali.forwardvalue(*it))) {
                glu=(int)std::round(calValue*10.0);
                }
            else {
                glu=it->getmgL();
                }

            if(prevglucose&&ittime>=(prevtime+nearlytwo)) {
                const float afst=(ittime-prevtime);
                if(afst>bucketsize&&afst<15*bucketsize) { 
                    const float d=((float)glu-prevglucose)/afst;
                    for(uint32_t i=bucketsize;i<afst;i+=bucketsize) {
                        const int tusid=timetoid(prevtime+i);
                        if(tusid==id)
                            break;
                        const uint16_t tusglu=round(prevglucose+i*d);
                        PERSlogprint("matchedminutes tusglu=%d\n",tusglu);
                        uitdata[tusid*days+lens[tusid]++]=tusglu;
                        }
                    }
                } 
            prevtime=ittime;
            prevglucose=glu;
            uitdata[id*days+lens[id]++]=glu;
            if(id>maxid)
                maxid=id;
             nexttime=prevtime+mindistance;
            }
        }
    int count= std::reduce( lens, lens+ ++maxid);
    if((count*maxdist)<11000)  {
        LOGGER("matchedminutes count=%d<11000\n",count*maxdist);
        return nullptr;
        }
    LOGAR("end matchedminutes");
    return new persgegs(uitdata,lens,days,maxid,pollstart,polllast,text,maxdist,maxidents);
    }
template <typename GlucoseIterator>
static struct persgegs * sortedmatched( std::vector<GlucoseDataType<GlucoseIterator>> *polldataptr,const uint32_t start,const uint32_t endt,const jugglucotext *text) {
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
typedef const ScanData* StreamIterator;
extern std::vector<GlucoseDataType<const ScanData*>> getsensorStreamranges(uint32_t start,uint32_t endt,bool calibrated,bool calibratePast);
extern  std::vector<GlucoseDataType<HistoryIterator>> getsensorHistoryranges(uint32_t start,uint32_t endt,bool,bool calibratePast) ;
static std::pair<uint32_t,uint32_t> makepercStartEnd(uint32_t firsttime,uint32_t lasttime,uint32_t endt,int days) {
#ifndef NOLOG
    const uint32_t endtin=endt;
    const int daysin=days;
#endif
    if(!firsttime)
        return {0,0};
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
template<typename DT>   std::pair<uint32_t,uint32_t> percStartEnd(uint32_t endt,int days) ;
template<>   std::pair<uint32_t,uint32_t> percStartEnd<ScanData>(uint32_t endt,int days) {
    auto firsttime=sensors->firstpolltime();
    if(!firsttime)
        return {0,0};
    auto [_id,lasttime]=sensors->lastpolltime();
    return makepercStartEnd(firsttime,lasttime,endt,days);
    }
template<>  std::pair<uint32_t,uint32_t> percStartEnd<Glucose>(uint32_t endt,int days) {
    auto firsttime=sensors->firsthistorytime();
    if(!firsttime)
        return {0,0};
    auto [_id,lasttime]=sensors->lasthistorytime();
    return makepercStartEnd(firsttime,lasttime,endt,days);
    }

#include "net/watchserver/Getopts.hpp"
#include "net/watchserver/watchserver.hpp"
#include "secs.h"

constexpr float devidewith=3;
constexpr  int winWidth = 1536*3/devidewith;
constexpr   int winHeight = 1080/devidewith;
template <typename  DT> std::pair<uint32_t,uint32_t> percStartEndopt(Getopts &opts) {
    if(!opts.width)
        opts.width=winWidth;
    if(!opts.height)
        opts.height=winHeight;
 
    uint32_t endsecs=opts.end;
    uint32_t startsecs=opts.start;
    constexpr int showdur=daysecs;
    int days=(endsecs-startsecs+showdur-1)/showdur;
    return percStartEnd<DT>(endsecs,days);
    }

#ifdef JUGGLUCO_APP

struct persgegs *globalpercptr=nullptr;


std::unique_ptr<struct stats> statptr;
extern void showstats(NVGcontext* vg,JCurve&,stats *stat,const jugglucotext *text) ;

bool showsummarygraph=false;
void graphpercentiles(NVGcontext* vg) {
    struct stats *st=statptr.get();
    if(st) {
        if(showsummarygraph&&globalpercptr) 
            globalpercptr->showpercentiles(vg,appcurve); 

        else
            showstats(vg,appcurve,st,usedtext);
        }
    }
extern void visiblebutton();
#include <atomic>
template <typename GlucoseIterator>
void makesummarygraph(uint32_t start,uint32_t endt, std::vector<GlucoseDataType<GlucoseIterator>> *polldata, std::atomic_flag *readyptr) {
    destruct _{[polldata,readyptr] {
            LOGAR("makesummarygraph before wait");
            readyptr->wait(false);
            LOGAR("makesummarygraph after wait");
            delete polldata;
            delete readyptr;
            }
        }; 

    if(globalpercptr!=nullptr)
        delete globalpercptr;
    globalpercptr=nullptr;

    
    if(!(globalpercptr=sortedmatched(polldata,start,endt,usedtext)))
        return;
    uint32_t startday=appcurve.starttime-getminutes(appcurve.starttime)*60;
    globalpercptr->startday=startday;
    globalpercptr->endday=startday+seconds_in_day;
    appcurve.setend=0;

    visiblebutton() ;
    return;
    }
#endif

template<typename Iter>
using IterType=std::remove_cvref_t<decltype(*std::declval<Iter>())>;
#ifdef JUGGLUCO_APP
static uint32_t statisticsendtime;

//std::vector<GlucoseDataType<StreamIterator>> globalpolldata;

template<typename Iterator>
bool mkpercentiles(int days) {
    showsummarygraph=false;
    const auto [start,endt]=percStartEnd<IterType<Iterator>>(statisticsendtime,days);
    if(start>=endt) return false;
    auto *polldata= new std::vector<GlucoseDataType<Iterator>>; //deleted in makesummarygraph
    bool calibrated=settings->data()->DoCalibrate;
    bool calibratePast=settings->data()->CalibratePast;
    getsensorranges<Iterator>(start,endt,calibrated,calibratePast,polldata);
    if(polldata->size()<=0) {
        LOGGER("mkpercentiles(%d)  no data\n",days);
        delete polldata;
        return false;
        }
    std::atomic_flag *readyptr=new std::atomic_flag{};

    std::thread graph(makesummarygraph<Iterator>, start, endt,polldata,readyptr);
    graph.detach();
    statptr=std::make_unique<struct stats>(*polldata);
    readyptr->test_and_set();
    readyptr->notify_all();
    showpers=true;
    LOGGER("end mkpercentiles(%d)\n",days); 
    return true;
    }
static bool mkpercentiles2(int days,bool history) {
    if(history) 
        return mkpercentiles<HistoryIterator>( days);
    else
        return mkpercentiles<StreamIterator>( days);
    }

int daystoanalyse=20;
bool showhistory=false;
bool mkpercentiles(int days,bool showhist) {
    bool ret=mkpercentiles2(daystoanalyse,showhist);
    if(!ret) {
         ret=mkpercentiles2(daystoanalyse,!showhist);
         if(ret) {
                showhistory=!showhist;
                }
        }
    return ret;
   }
extern bool makepercetages() ;
bool makepercetages() {
    statisticsendtime=appcurve.starttime+appcurve.duration;
//    return mkpercentiles<StreamIterator>(daystoanalyse);
    return mkpercentiles(daystoanalyse,showhistory);
    }
#endif
#include "settings/settings.hpp"
void JCurve::leginterval(NVGcontext* vg,const float x,const float y, const int *between) {
    const constexpr int maxbuf=10;
    char buf[maxbuf];
    int buflen=snprintf(buf,maxbuf,"%.*f-%.*f",gludecimal,::gconvert((between[1]+1)*10,glunit),gludecimal,::gconvert((between[0])*10,glunit));
    nvgText(vg, x,y,buf,buf+buflen);
    }
void stats::showbar(NVGcontext* vg,JCurve &jcurve,const jugglucotext *text) {

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
#if 0
    float xnumpos=dwidth/3+jcurve.dleft-jcurve.smallsize;
#endif
    for(int i=0;i<5;i++)     { 
        const int perlen=snprintf(perbuf,maxbuf,perform,pers[i]*100);
        nvgText(vg, xleg,yleg,perbuf,perbuf+perlen);
#if 0
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
    const string_view unitstr=jcurve.glunit==1?text->mmolL:"mg/dL"sv;
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
    int len;
    if(jcurve.isRTL()) {
          int numlen=snprintf(meanstr,5,jcurve.gformat,::gconvert(mean*10,jcurve.glunit));
          memcpy(meanstr+numlen, usedtext->averageglucose.data(),avlen);
          len=avlen+numlen;
        }
    else {
            memcpy(meanstr, usedtext->averageglucose.data(),avlen);
            len=avlen+snprintf(meanstr+avlen,5,jcurve.gformat,::gconvert(mean*10,jcurve.glunit));
            }

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
    stat->showbar(vg,jcurve,text);
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

#ifdef CATCHGRAPH

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
#endif
extern void mktypeheader(char *outstart,char *outiter,const bool headonly,recdata *outdata,std::string_view type,std::string_view origin);

extern bool givesummarygraph(Getopts &opts,std::string_view origin,recdata *outdata);

    /*
static std::vector<Getopts> newopts;
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
constexpr const int vgnRTflags=0;
//constexpr const int vgnRTflags=NVG_ANTIALIAS;
template <typename DT>
static  char * givepercentiles(Getopts &opts,uint32_t start, uint32_t endt,recdata *outdata,std::string_view origin) {
#ifndef NOLOG
    const auto started=clock();
#endif
    constexpr int showdur=daysecs;
    if(start>=endt) {
        LOGAR("givepercentiles: start>=endt");
        return nullptr;
        }

//    bool calibratePast=settings->data()->CalibratePast;
    bool calibratePast=opts.pastvaluesmode;
    std::vector<GlucoseDataType<DT>> stream;
    getsensorranges<DT>(start,endt,opts.calibratedmode,calibratePast,&stream);

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
    auto vg = nvgCreateRT(vgnRTflags, width, height);
    destruct _{[vg]{nvgDeleteRT(vg);}};
    perscurve.initfont(vg);

    backgroundcolor(vg,*perscurve.getwhite());

    perscurve.startstepNVG(vg,width,height);

    LOGAR("givepercentiles: before showpercentiles");
    percptr->showpercentiles(vg,perscurve); 
    LOGAR("givepercentiles: after showpercentiles");
    delete percptr;
    nvgEndFrame(vg);
    LOGAR("givepercentiles: before nvgReadPixelsRT");
    unsigned char *rgba = nvgReadPixelsRT(vg);
    LOGAR("givepercentiles: after nvgReadPixelsRT");
    constexpr const int startpos=152;
    int len;
    char *imagestart = reinterpret_cast<char *>(stbi_write_png_to_mem(startpos,rgba, width*4, width, height, 4, &len));
    if(!imagestart) {
        LOGAR("givepercentiles: no image");
        return nullptr;
        }
    LOGAR("givepercentiles: after stbi_write_png_to_mem");
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

bool givesummarygraph(Getopts &opts,std::string_view origin,recdata *outdata) {
    const bool history=opts.historymode||opts.calibratedhistorymode;
    const auto [startt,endt]=(history?percStartEndopt<Glucose>:percStartEndopt<ScanData>)(opts);
    if(startt>=endt) {
        LOGAR("givesummarygraph no data");
        return false;
        }
//   LOGGER("opts.end=%u endt=%d diff=%d
   opts.start=startt;
   opts.end=endt;

#ifdef CATCHGRAPH
   const bool absolute=(opts.end-endt)>(10*60);
    if(PercentileGraph *hit=persimages.get(opts,absolute)) {
        const auto &rec=hit->image;
        LOGGER("persimages.get %p\n", rec.allbuf);
        mktypeheader(hit->startimage,(char *)rec.start+rec.len,false,outdata,"image/png"sv,origin);
        outdata->allbuf=nullptr;
        LOGGER("givesummarygraph: found result opts.start=%u opts.end=%u\n",opts.start,opts.end);
        return true;
        }
#endif
    if(char *startimage=
(history?givepercentiles<HistoryIterator>:givepercentiles<const ScanData *>)(opts,startt,endt,outdata,origin)) {
#ifdef CATCHGRAPH
          LOGGER("persimages.emplace_back %p\n", outdata->allbuf);
              {
              const std::lock_guard<std::mutex> lock(persimages.mutex);
              persimages.emplace_back(opts,*outdata,startimage);
              }
#endif
          outdata->allbuf=nullptr;
          return true;
          }
    return false;
    }

/*
template <typename DT>
std::vector<GlucoseDataType<DT>> getsensorrangesAlg(uint32_t start,uint32_t endt,bool calibrated,bool allvalues,bool calibratePast,std::vector<std::unique_ptr<ScanData []>> &calibrates );
template<> std::vector<GlucoseDataType<const ScanData *>> getsensorrangesAlg<const ScanData*>(uint32_t start,uint32_t endt,bool calibrated,bool allvalues,bool calibratePast,std::vector<std::unique_ptr<ScanData []>> &calibrates ) {
    return getsensorranges(start,endt,calibrated,allvalues,calibratePast,calibrates);
    }
template<> std::vector<GlucoseDataType<HistoryIterator>> getsensorrangesAlg<HistoryIterator>(uint32_t start,uint32_t endt,bool calibrated,bool allvalues,bool calibratePast,std::vector<std::unique_ptr<ScanData []>> &calibrates ) {
    return getsensorHistoryranges(start, endt,calibrated,calibratePast);
    } */
template <typename DT>
static std::unique_ptr<stats> calculateStatistics(Getopts &opts) {
    if(opts.start>=opts.end)
        return nullptr;
    const int days=(static_cast<uint64_t>(opts.end)-opts.start+daysecs-1)/daysecs;
    const auto [start,endt]=percStartEnd<IterType<DT>>(opts.end,days);
    LOGGER("calculateStatistics %d %d days=%d\n",start,endt,days);
    if(start>=endt)
        return nullptr;
    const bool calibratePast=opts.pastvaluesmode;
    const bool calibrated=opts.calibratedmode||opts.calibratedhistorymode;
    std::vector<GlucoseDataType<DT>> stream;
    getsensorranges<DT>(start,endt,calibrated,calibratePast,&stream);
    if(stream.empty())
        return nullptr;
    return std::make_unique<stats>(stream);
    }

template <typename DT>
std::span<char> getStatImage(int startpos,Getopts &opts) {
    auto statistics=calculateStatistics<DT>(opts);
    if(!statistics)
        return {(char *)nullptr,0};
    JCurve statimage(opts.unit?opts.unit:settings->data()->unit);
    statimage.invertcolorsset(opts.darkmode);
    constexpr int duration=24*60*60;
    statimage.duration=duration;
    statimage.starttime=opts.end-duration;
    const auto *text=language::gettext(opts.lang);
    statimage.usedtext=text;
    const int winHeight=opts.height?opts.height:256;
    const int winWidth=opts.width?opts.width:768;
    const double mult=winHeight/512.0;
    LOGGER("getStatImage width=%d height=%d mult=%f\n",winWidth,winHeight,mult);
    auto vg = nvgCreateRT(vgnRTflags, winWidth, winHeight);
    destruct _{[vg]{nvgDeleteRT(vg);}};
    statimage.dheight=winHeight;
    statimage.dwidth=winWidth;
    statimage.setfontsize(38.5*mult,44.0*mult,2.8*mult,250.0*mult);
 
    statimage.initfont(vg);
    backgroundcolor(vg,*statimage.getwhite());
    statimage.startstepNVG(vg,winWidth,winHeight);
    showstats(vg,statimage,statistics.get(),text);
    nvgEndFrame(vg);
    unsigned char *rgba = nvgReadPixelsRT(vg);
    int len=0;
    unsigned char *png = stbi_write_png_to_mem(startpos,rgba, winWidth*4, winWidth, winHeight, 4, &len);
    return {(char *)png,static_cast<std::size_t>(len)};
    }

std::span<char> getStatImageHistory(int startpos,Getopts &opts) {
        return getStatImage<HistoryIterator>(startpos,opts);
        } 
std::span<char> getStatImageStream(int startpos,Getopts &opts) {
        return getStatImage<const ScanData*>(startpos,opts);
        } 

class StatisticsWriter {
    char *position;
    char * const limit;
public:
    StatisticsWriter(char *start,size_t capacity):position(start),limit(start+capacity) {}

    bool append(std::string_view value) {
        const size_t available=limit-position;
        if(value.size()>available)
            return false;
        memcpy(position,value.data(),value.size());
        position+=value.size();
        return true;
        }

    bool appendJsonString(std::string_view value) {
        if(!append("\""sv))
            return false;
        for(const unsigned char character:value) {
            switch(character) {
                case '\"':
                    if(!append("\\\""sv)) return false;
                    break;
                case '\\':
                    if(!append("\\\\"sv)) return false;
                    break;
                case '\b':
                    if(!append("\\b"sv)) return false;
                    break;
                case '\f':
                    if(!append("\\f"sv)) return false;
                    break;
                case '\n':
                    if(!append("\\n"sv)) return false;
                    break;
                case '\r':
                    if(!append("\\r"sv)) return false;
                    break;
                case '\t':
                    if(!append("\\t"sv)) return false;
                    break;
                case '<':
                    if(!append("\\u003c"sv)) return false;
                    break;
                default:
                    if(character<0x20) {
                        if(!appendformat("\\u%04x",character)) return false;
                        }
                    else {
                        const char byte=static_cast<char>(character);
                        if(!append(std::string_view(&byte,1))) return false;
                        }
                }
            }
        return append("\""sv);
        }

    template <typename... Args>
    bool appendformat(const char *format,Args... args) {
        const size_t available=limit-position;
        if(!available)
            return false;
        const int len=snprintf(position,available,format,args...);
        if(len<0||static_cast<size_t>(len)>=available)
            return false;
        position+=len;
        return true;
        }

    char *end() const {
        return position;
        }
    };

static double statisticsGlucose(double mgdL,int unit) {
    return unit==1?mgdL/18.0:mgdL;
    }

static bool writeStatisticsJson(StatisticsWriter &writer,const stats &stat,int unit,
                                std::string_view source,bool calibrated) {
    const double durationdays=static_cast<double>(stat.endtime-stat.starttime)/daysecs;
    const int targetcount=static_cast<int>(std::lround(stat.pertarget*stat.count));
    const double targetlow=statisticsGlucose(stat.border[1]+1,unit);
    const double targethigh=statisticsGlucose(stat.border[0],unit);
    const char *unitlabel=unit==1?"mmol/L":"mg/dL";
    return writer.appendformat(R"JSON({"schemaVersion":1,"source":"%.*s","calibrated":%s,"startTime":%u,"endTime":%u,"durationDays":%.10g,"measurementCount":%d,"expectedMeasurementCount":%d,"timeActivePercent":%.10g,"glucoseUnit":"%s","meanGlucose":%.10g,"estimatedA1c":{"percent":%.10g,"mmolMol":%d},"gmi":{"percent":%.10g,"mmolMol":%d},"standardDeviation":%.10g,"coefficientOfVariationPercent":%.10g,"targetRange":{"minimum":%.10g,"maximum":%.10g,"measurementCount":%d,"percent":%.10g},"ranges":[{"name":"veryHigh","minimum":%.10g,"maximum":null,"measurementCount":%d,"percent":%.10g},{"name":"high","minimum":%.10g,"maximum":%.10g,"measurementCount":%d,"percent":%.10g},{"name":"standard","minimum":%.10g,"maximum":%.10g,"measurementCount":%d,"percent":%.10g},{"name":"low","minimum":%.10g,"maximum":%.10g,"measurementCount":%d,"percent":%.10g},{"name":"veryLow","minimum":null,"maximum":%.10g,"measurementCount":%d,"percent":%.10g}]})JSON",
        static_cast<int>(source.size()),source.data(),calibrated?"true":"false",
        stat.starttime,stat.endtime,durationdays,stat.count,stat.totid,stat.active*100.0,
        unitlabel,statisticsGlucose(stat.mean,unit),stat.EA1Cper,stat.EA1Cmmol,
        stat.GMIper,stat.GMImmol,statisticsGlucose(stat.sd,unit),stat.vc*100.0,
        targetlow,targethigh,targetcount,stat.pertarget*100.0,
        statisticsGlucose(stats::levels[0]+1,unit),stat.counts[0],stat.pers[0]*100.0,
        statisticsGlucose(stats::levels[1]+1,unit),statisticsGlucose(stats::levels[0],unit),stat.counts[1],stat.pers[1]*100.0,
        statisticsGlucose(stats::levels[2]+1,unit),statisticsGlucose(stats::levels[1],unit),stat.counts[2],stat.pers[2]*100.0,
        statisticsGlucose(stats::levels[3]+1,unit),statisticsGlucose(stats::levels[2],unit),stat.counts[3],stat.pers[3]*100.0,
        statisticsGlucose(stats::levels[3],unit),stat.counts[4],stat.pers[4]*100.0);
    }

static bool appendBrowserStatisticsText(StatisticsWriter &writer,std::string_view value,
                                        bool visualrtl) {
    if(!visualrtl)
        return writer.appendJsonString(value);
    constexpr size_t maxtext=512;
    if(value.size()>=maxtext)
        return false;
    char visual[maxtext];
    memcpy(visual,value.data(),value.size());
    visual[value.size()]='\0';
    char logical[maxtext];
    const int32_t length=rtl_to_logical_utf8(visual,logical,sizeof(logical));
    if(length<0||static_cast<size_t>(length)>=sizeof(logical))
        return false;
    return writer.appendJsonString({logical,static_cast<size_t>(length)});
    }

template <typename... Args>
static bool appendBrowserStatisticsFormat(StatisticsWriter &writer,bool visualrtl,
                                          const char *format,Args... args) {
    char value[512];
    const int length=snprintf(value,sizeof(value),format,args...);
    if(length<0||static_cast<size_t>(length)>=sizeof(value))
        return false;
    return appendBrowserStatisticsText(writer,{value,static_cast<size_t>(length)},visualrtl);
    }

static bool writeStatisticsTextJson(StatisticsWriter &writer,const stats &stat,int unit,
                                    std::string_view source,const jugglucotext *text,
                                    bool visualrtl,std::string_view locale) {
    const double durationdays=static_cast<double>(stat.endtime-stat.starttime)/daysecs;
    const std::string_view sourcetext=text->menustr2[source=="history"sv?3:2];
    if(!writer.append("{\"locale\":"sv)||!writer.appendJsonString(locale)||
       !writer.append(",\"glucoseUnit\":"sv)||
       !appendBrowserStatisticsText(writer,unit==1?text->mmolL:"mg/dL"sv,visualrtl)||
       !writer.append(",\"title\":"sv)||
       !appendBrowserStatisticsText(writer,text->statisticsName(),visualrtl)||
       !writer.append(",\"source\":"sv)||
       !appendBrowserStatisticsText(writer,sourcetext,visualrtl)||
       !writer.append(",\"duration\":"sv)||
       !appendBrowserStatisticsFormat(writer,visualrtl,text->duration,durationdays)||
       !writer.append(",\"timeActive\":"sv)||
       !appendBrowserStatisticsFormat(writer,visualrtl,text->timeactive,stat.active*100.0)||
       !writer.append(",\"measurements\":"sv)||
       !appendBrowserStatisticsFormat(writer,visualrtl,text->nrmeasurement,stat.count)||
       !writer.append(",\"meanLabel\":"sv)||
       !appendBrowserStatisticsText(writer,text->averageglucose,visualrtl)||
       !writer.append(",\"estimatedA1c\":"sv)||
       !appendBrowserStatisticsFormat(writer,visualrtl,text->EstimatedA1C,stat.EA1Cper,stat.EA1Cmmol)||
       !writer.append(",\"gmi\":"sv)||
       !appendBrowserStatisticsFormat(writer,visualrtl,text->GMI,stat.GMIper,stat.GMImmol)||
       !writer.append(",\"standardDeviation\":"sv)||
       !appendBrowserStatisticsFormat(writer,visualrtl,text->SD,statisticsGlucose(stat.sd,unit))||
       !writer.append(",\"variability\":"sv)||
       !appendBrowserStatisticsFormat(writer,visualrtl,text->glucose_variability,stat.vc*100.0))
        return false;
    return writer.append("}"sv);
    }

static bool writeStatisticsHtml(StatisticsWriter &writer,const stats &stat,int unit,
                                std::string_view source,bool calibrated,bool darkmode,
                                uint16_t lang) {
    const auto *text=language::gettext(lang);
    const bool visualrtl=text==&artext;
    const bool righttoleft=visualrtl||lang==(mklanguagenum2('i','w'));
    char localebuffer[3]={'e','n','\0'};
    if(lang) {
        localebuffer[0]=static_cast<char>(lang&0xFF);
        localebuffer[1]=static_cast<char>((lang>>8)&0xFF);
        }
    const std::string_view locale(localebuffer,2);
    if(!writer.append(R"HTML(<!doctype html><html lang=")HTML")||
       !writer.append(locale)||
       (righttoleft&&!writer.append("\" dir=\"rtl"sv))||
       !writer.append(R"HTML("><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Juggluco</title><style>
:root{color-scheme:light;--bg:#f4f7fb;--card:#fff;--text:#17202a;--muted:#667085;--line:#d9e0e8;--accent:#1565c0;--shadow:0 12px 32px rgba(15,23,42,.09)}
body.dark{color-scheme:dark;--bg:#101419;--card:#1b222b;--text:#f5f7fa;--muted:#aeb8c5;--line:#35404d;--accent:#7db7ff;--shadow:none}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--text);font-family:system-ui,-apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif}main{width:min(68rem,calc(100% - 2rem));margin:0 auto;padding:2rem 0 3rem}header{display:flex;align-items:flex-start;justify-content:space-between;gap:1rem;margin-bottom:1.25rem}.eyebrow{margin:0 0 .2rem;color:var(--accent);font-size:.76rem;font-weight:800;letter-spacing:.12em;text-transform:uppercase}h1{margin:0;font-size:clamp(1.8rem,5vw,2.7rem);line-height:1.05}#period{margin:.55rem 0 0;color:var(--muted)}.badge{margin:.2rem 0 0;padding:.42rem .7rem;border:1px solid var(--line);border-radius:999px;color:var(--muted);font-size:.85rem;white-space:nowrap}.overview{display:grid;grid-template-columns:minmax(18rem,.9fr) minmax(20rem,1.1fr);gap:1rem}.card{background:var(--card);border:1px solid var(--line);border-radius:1rem;box-shadow:var(--shadow)}.ranges-card{display:grid;grid-template-columns:5rem 1fr;gap:1.25rem;align-items:center;padding:1.25rem}.stacked{height:25rem;width:4.25rem;display:flex;flex-direction:column;overflow:hidden;border:1px solid var(--line);background:var(--bg)}.segment{min-height:0}.legend{list-style:none;margin:0;padding:0;display:grid;gap:.8rem}.legend li{display:grid;grid-template-columns:.8rem 1fr auto;align-items:center;gap:.55rem}.swatch{width:.72rem;height:.72rem;border-radius:.2rem}.legend-name{font-weight:700}.legend-range{display:block;color:var(--muted);font-size:.82rem;font-weight:400}.legend-percent{font-variant-numeric:tabular-nums}.metrics{margin:0;padding:1rem;display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:.75rem}.metric{padding:1rem;border:1px solid var(--line);border-radius:.75rem}.metric dd{margin:0;font-size:1.05rem;font-weight:700;font-variant-numeric:tabular-nums}.target{margin-top:1rem;padding:1.1rem 1.25rem;display:flex;align-items:center;justify-content:space-between;gap:1rem}.target p{margin:0;color:var(--muted)}.target strong{font-size:1.45rem;font-variant-numeric:tabular-nums}.footer{margin:1rem .2rem 0;text-align:right}.footer a{color:var(--accent)}@media(max-width:760px){main{padding-top:1.25rem}.overview{grid-template-columns:1fr}.stacked{height:21rem}.metrics{grid-template-columns:1fr}header{display:block}.badge{display:inline-block}.target{align-items:flex-start;flex-direction:column}.footer{text-align:left}}
</style></head><body)HTML"))
        return false;
    if(darkmode&&!writer.append(" class=\"dark\""sv))
        return false;
    if(!writer.append(R"HTML(><main><header><div><p class="eyebrow">Juggluco</p><h1 id="page-title"></h1><p id="period"></p></div><p class="badge" id="source"></p></header><section class="overview"><article class="card ranges-card"><div class="stacked" id="stacked"></div><ol class="legend" id="legend"></ol></article><dl class="card metrics" id="metrics"></dl></section><section class="card target"><p id="target-range"></p><strong id="target-percent"></strong></section><p class="footer"><a id="json-link" href="#">JSON</a></p></main><script id="statistics-data" type="application/json">)HTML"))
        return false;
    if(!writeStatisticsJson(writer,stat,unit,source,calibrated))
        return false;
    if(!writer.append(R"HTML(</script><script id="statistics-text" type="application/json">)HTML")||
       !writeStatisticsTextJson(writer,stat,unit,source,text,visualrtl,locale)||
       !writer.append(R"HTML(</script><script>
const data=JSON.parse(document.getElementById('statistics-data').textContent);
const text=JSON.parse(document.getElementById('statistics-text').textContent);
const one=new Intl.NumberFormat(text.locale,{minimumFractionDigits:1,maximumFractionDigits:1});
const whole=new Intl.NumberFormat(text.locale,{maximumFractionDigits:0});
const glucose=value=>data.glucoseUnit==='mmol/L'?one.format(value):whole.format(value);
const percent=value=>`${one.format(value)}%`;
const date=value=>new Date(value*1000).toLocaleString(text.locale);
document.title=`Juggluco · ${text.title}`;
document.getElementById('page-title').textContent=text.title;
document.getElementById('period').textContent=`${date(data.startTime)} – ${date(data.endTime)} · ${text.duration}`;
document.getElementById('source').textContent=`${text.source}${data.calibrated?' ✓':''}`;
const colors={veryHigh:'#f2994a',high:'#f2c94c',standard:'#3cb371',low:'#eb5757',veryLow:'#7b3f00'};
const rangeText=range=>range.minimum===null?`≤ ${glucose(range.maximum)}`:range.maximum===null?`≥ ${glucose(range.minimum)}`:`${glucose(range.minimum)}–${glucose(range.maximum)}`;
const stacked=document.getElementById('stacked');
const legend=document.getElementById('legend');
for(const range of data.ranges){
  const segment=document.createElement('div');
  segment.className='segment';segment.style.height=`${range.percent}%`;segment.style.background=colors[range.name];
  segment.title=`${rangeText(range)} ${text.glucoseUnit}: ${percent(range.percent)}`;stacked.append(segment);
  const item=document.createElement('li');
  const swatch=document.createElement('span');swatch.className='swatch';swatch.style.background=colors[range.name];
  const label=document.createElement('span');label.className='legend-name';label.textContent=`${rangeText(range)} ${text.glucoseUnit}`;
  const limits=document.createElement('small');limits.className='legend-range';limits.textContent=`# ${whole.format(range.measurementCount)}`;label.append(limits);
  const amount=document.createElement('span');amount.className='legend-percent';amount.textContent=percent(range.percent);
  item.append(swatch,label,amount);legend.append(item);
}
const values=[
  text.timeActive,
  text.measurements,
  `${text.meanLabel}${glucose(data.meanGlucose)} ${text.glucoseUnit}`,
  text.estimatedA1c,
  text.gmi,
  text.standardDeviation,
  text.variability
];
const metrics=document.getElementById('metrics');
for(const value of values){const box=document.createElement('div');box.className='metric';const result=document.createElement('dd');result.textContent=value;box.append(result);metrics.append(box)}
document.getElementById('target-range').textContent=`↔ ${glucose(data.targetRange.minimum)}–${glucose(data.targetRange.maximum)} ${text.glucoseUnit} · # ${whole.format(data.targetRange.measurementCount)}`;
document.getElementById('target-percent').textContent=percent(data.targetRange.percent);
const jsonUrl=new URL(location.href);if(!jsonUrl.pathname.endsWith('.json'))jsonUrl.pathname+='.json';document.getElementById('json-link').href=jsonUrl;
</script></body></html>)HTML"))
        return false;
    return true;
    }

template <typename DT>
static bool giveStatistics(Getopts &opts,std::string_view origin,recdata *outdata,
                           std::string_view source) {
    auto statistics=calculateStatistics<DT>(opts);
    if(!statistics||statistics->count<2||statistics->starttime==UINT32_MAX||
       statistics->endtime<statistics->starttime)
        return false;
    constexpr size_t bodycapacity=32*1024;
    const size_t headercapacity=256+origin.size();
    if(headercapacity<origin.size())
        return false;
    const size_t totalcapacity=headercapacity+bodycapacity+1;
    if(totalcapacity<headercapacity)
        return false;
    char *allbuf=new(std::nothrow) char[totalcapacity];
    if(!allbuf)
        return false;
    char *bodystart=allbuf+headercapacity;
    StatisticsWriter writer(bodystart,bodycapacity);
    const int requestedunit=opts.unit?opts.unit:settings->data()->unit;
    const int unit=requestedunit==1?1:2;
    const bool calibrated=opts.calibratedmode||opts.calibratedhistorymode;
    const bool success=opts.jsonmode
        ?writeStatisticsJson(writer,*statistics,unit,source,calibrated)
        :writeStatisticsHtml(writer,*statistics,unit,source,calibrated,opts.darkmode,opts.lang);
    if(!success) {
        delete[] allbuf;
        return false;
        }
    *writer.end()='\0';
    outdata->allbuf=allbuf;
    mktypeheader(bodystart,writer.end(),false,outdata,
        opts.jsonmode?"application/json; charset=utf-8"sv:"text/html; charset=utf-8"sv,origin);
    return true;
    }

bool givestatistics(Getopts &opts,std::string_view origin,recdata *outdata) {
    const bool history=opts.historymode||opts.calibratedhistorymode;
    return history
        ?giveStatistics<HistoryIterator>(opts,origin,outdata,"history"sv)
        :giveStatistics<const ScanData *>(opts,origin,outdata,"stream"sv);
    }


static NVGcontext* getfilevg(JCurve &curveimage,int width,int height) {
    double multiply=height/800.0;
    LOGGER("multiply=%f\n",multiply);
//constexpr   int winHeight = 1080/devidewith;
    auto vg = nvgCreateRT(vgnRTflags, width, height);
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
  
  if(!(opts.calibratedmode|| opts.calibratedhistorymode||opts.streammode||opts.calibratedscansmode||opts.scansmode||opts.historymode||opts.amountsmode||opts.mealsmode)) {
        LOGAR("getCurveImage: no settings");
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
    curveimage.showcalibratedscans=opts.calibratedscansmode;
    

    curveimage.glow=curveimage.userunit2mgL(opts.glow);
    curveimage.ghigh=curveimage.userunit2mgL(opts.ghigh);
    LOGGER("getCurveImage start=%u end=%u width=%d height=%d calibratedmode=%d calibratedhistorymode=%d\n", startsecs, endsecs,width,height,opts.calibratedmode,opts.calibratedhistorymode);
    NVGcontext* vg=getfilevg(curveimage,width,height);

//    curveimage.historyStrokeWidth*=1.2;
 //   curveimage.pollCurveStrokeWidth*=1.2;
    curveimage.pointRadius*=1.2;
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


/*
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

*/
#endif
