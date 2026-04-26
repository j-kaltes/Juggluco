#pragma once
#include <string_view>
#include <array>
#include <vector>
#include <time.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>
#include "config.h"
#include "settings/settings.hpp"
#include "curve.hpp"
#include "gluconfig.hpp"
extern int *numheights;
extern int shownlabels;
inline constexpr const int maxnumsources=2;
struct mealposition {
	float mealx;
	float mealstarty;
	float mealendy;
	int mealpos;
	int mealbase;
	};
typedef struct NVGcontext NVGcontext;
//typedef struct NVGcolor NVGcolor;
struct displaytime; 
struct ScanData;
struct NumHit;
struct Num;
template <class T> struct NumIter;
class SensorGlucoseData; 
union bounds_t ;
typedef std::pair<std::string_view,std::string_view> errortype;
struct geo_t;
struct jugglucotext;
extern const jugglucotext engtext;
extern jugglucotext artext;

struct JCurve {
protected:
JCurve()=default;
public:
JCurve(int unit) {
        setunit(unit);
        CalibratePast=settings->data()->CalibratePast;
        }
float glow=0.0f,ghigh=0.0f;
std::pair<const Num*,const Num*> extrums[maxnumsources];
const char *gformat;
int glunit;
int gludecimal;
double convertmult;
void setunit(int unit) {
    glunit=unit;
    if(unit==1) {
        convertmult= convertmultmmol;
        gformat="%.1f";
        gludecimal=1;
        }
    else  {
        convertmult= convertmultmg;
        gformat="%.0f";
        gludecimal=0;
        }

    };
inline float gconvert(const float mgperL) const {
    return ::gconvert(mgperL,glunit);
    }
inline int userunit2mgL(const float unit) const {
        return (int)round(unit/convertmult);
        }
const jugglucotext *usedtext=&engtext;

bool isRTL() const {
    return usedtext==&artext;
    }


    NVGcontext *thevg;

std::pair<const ScanData *,const ScanData*> *scanranges=nullptr;
std::pair<const ScanData *,const ScanData*> *pollranges=nullptr;
std::pair<int32_t,int32_t> *histpositions=nullptr;
int histlen=0;
std::vector<int> hists;
std::vector<int> hidden;
~JCurve() {
        delete[] scanranges;
        delete[] pollranges;
        delete[] histpositions;
        };

uint32_t settime=0;
uint32_t setend=0;
uint32_t starttime;
int duration=8*60*60;
int gmin=2*180;
int grange=8*180;

int statusbarheight=0;
int statusbarleft=0,statusbarright=0;
int startincolors=0;
int lasttouchedcolor=-1;
int showstream=1;
int showcalibratedstream=0;
int showcalibratedhistories=0;
int showcalibratedscans=0;

int showscans=0;
int showhistories=0;
int shownumbers=1;
int showmeals=0;
int invertcolors=0;
float smallerlen;

float valuesize=0;

float facetimefontsize,facetimey;
float listitemlen;
#ifdef JUGGLUCO_APP
int menufont=-1;
#endif
int font=0,monofont=0,whitefont=-1,blackfont=0;
float headheight;
int width=-1,height=-1;

float dleft=0,dtop=0,dbottom{0},dright=0,dheight,dwidth;
float smallsize=300,menusize=smallsize,headsize=900,midsize, mediumfont,timefontsize=smallsize;
float density;
float textheight,menutextheight;
float smallfontlineheight;

struct {
    float left,top;
    union {
        float right;
        float width;
    };
    union {
        float bottom;
        float height;
    };} sensorbounds;
float timelen=300,timeheight;
 float historyStrokeWidth;
 float numcircleStrokeWidth;
 float lowGlucoseStrokeWidth;
float pollCurveStrokeWidth;
float hitStrokeWidth;
 float TrendStrokeWidth;
 float glucoseLinesStrokeWidth;
 float timeLinesStrokeWidth;
 float dayEndStrokeWidth;
 float nowLineStrokeWidth;
 float pointRadius;
float foundPointRadius,arrowstrokewidth;

bool doclamp=false;
bool nowclamp=false;
int diffcurrent=0;

std::vector<mealposition> mealpos;
void setdiffcurrent(bool val) {
   LOGGER("setdiffcurrent(nowclamp=%d)  doclamp=false\n",val);
   doclamp=false;
   nowclamp=val;
   }
    void    initfont(NVGcontext* avg);
     void   sidenum(NVGcontext* avg,const float posx,const float posy,const char *buf,const int len,const bool hit);
     bool    glucosepointinfo(NVGcontext* avg,time_t tim,uint32_t value,   float posx, float posy);
     bool    glucosepoint(NVGcontext* avg,time_t tim,uint32_t value,   float posx, float posy);
    template <class TX,class TY> void    showScan(NVGcontext* avg,const ScanData *low,const ScanData *high,  const TX &transx,  const TY &transy,const int colorindex);
     void    makecircle(NVGcontext* avg,float posx,float posy);
    template <class TX,class TY> void    histcurve(NVGcontext* avg,const SensorGlucoseData  * hist, const int32_t firstpos, const int32_t lastpos,const TX &xtrans,const TY &ytrans,const int colorindex);

template <class TX,class TY> void    calihistcurve(NVGcontext* avg,const SensorGlucoseData  * hist, const int32_t firstpos, const int32_t lastpos,const TX &xtrans,const TY &ytrans,const int colorindex) ;
    std::pair<float,float>    drawtrender(NVGcontext* avg,const std::array<uint16_t,16> &trend,const float x,const float y,const float w,const float h);
    void    showok(NVGcontext* avg,bool good,bool up);
     bool           showerror(NVGcontext* avg,const std::string_view str1,const std::string_view str2);
     void           scanwait(NVGcontext* avg);

 void       showscanner(NVGcontext* avg,const SensorGlucoseData *hist,int scanident,time_t nu,bool calibrate);
    template <class LT> void    glucoselines(NVGcontext* avg,const float last,const float smallfontlineheight,const int gmax,const LT &transy,bool);
    template <class LT>
    void    timelines(NVGcontext* avg,const displaytime *disp, const LT &transx ,uint32_t nu);
    template <class LT> void    epochlines(NVGcontext* avg,uint32_t first,uint32_t last, const LT &transx);
    void    drawarrow(NVGcontext* avg, float rate,float getx,float gety);

 int    showLargevalue(NVGcontext* avg, int index,float getx,float gety,float convglucose,const ScanData *poll) ;
     void    showvalue(NVGcontext* avg, const ScanData *poll,const SensorGlucoseData *hist, float getx,float gety,int index,uint32_t nu);
     int    showerrorvalue(NVGcontext* avg,const SensorGlucoseData *sens,const time_t nu,float getx,float gety,int index);
     void    showlastsstream(NVGcontext* avg,const time_t nu,const float getx,std::vector<int> &used );
    void       showbluevalue(NVGcontext* avg,const time_t nu,const int xpos,std::vector<int> &used);
     void       showsavedomain(NVGcontext* avg,const float last, const float dlow,const float dhigh);
     void    showunsaveredline(NVGcontext* avg,const float last,const float dlow);
     void       showsaverange(NVGcontext* avg,const float last, const float dlow,const float dhigh);
    void        showdates(NVGcontext* avg,time_t nu,uint32_t starttime,time_t endtime);
    void    showlines(NVGcontext* avg,int gm,int gmax);
    int    displaycurve(NVGcontext* avg,time_t nu);
    void    startstepNVG(NVGcontext* avg,int width, int height);
    void    startstep(NVGcontext* avg,const NVGcolor &col);
    void    endstep(NVGcontext* avg);
     void    defaulterror(NVGcontext* avg,int scerror);
     bool    errorpair(NVGcontext* avg,const errortype &error);
    int    badscanMessage(NVGcontext* avg,int kind);
     int    showoldscan(NVGcontext* avg,uint32_t nu);
     void    withredisplay(NVGcontext* avg,uint32_t nu);
    int    onestep(NVGcontext* avg);
    void     setlocale(NVGcontext* avg,const char *localestrbuf,const size_t len);

    void showButton(NVGcontext* avg,float xpos,float ypos,std::string_view str);
    void    showOK(NVGcontext* avg,float xpos,float ypos);
    template <typename  TI,typename TE> void    textbox(NVGcontext* avg,const TI &title,const TE &text);
     const float     getsetlen(NVGcontext* avg,float x, float  y,const char * set,const char *setend,bounds_t &bounds);
     void    showtext(NVGcontext* avg ,time_t nu,int menu);
    void    showfromend(NVGcontext* avg);
    void    showfromstart(NVGcontext* avg);
     void    shownumlist(NVGcontext* avg);

    template <class TX,class TY> void showlineScan(NVGcontext* avg,const ScanData *low,const ScanData *high,  const TX &transx,  const TY &transy,const int colorindex,bool search); 
void setfontsize(float small,float menu,float density,float headin);
void resizescreen(int widthin, int heightin,int initscreenwidth);
void withbottom();
void setextremes(std::pair<int,int> extr);
//auto gettrans(uint32_t starttime,uint32_t endtime);
auto gettrans(uint32_t starttime,uint32_t endtime) {

    const double usedtop=pointRadius;
    const double usedheight=dheight-2*usedtop;
    const int gmax=gmin+grange;
    const double yscale= -usedheight/grange,ymove= usedtop+usedheight*gmax/grange;
   const auto transy=[yscale,ymove](uint32_t y) {return y*yscale + ymove;};


    double interval=endtime-starttime;
    const double xscale=dwidth/interval;
    const double doublestart=starttime;
    const auto transx=[xscale,doublestart](uint32_t x) {return (x-doublestart)*xscale;};

    return std::make_pair(transx,transy);
    }
int64_t longpress(float x,float y);
int64_t screentap(float x,float y);
uint32_t timeend() {
    return starttime+duration;    
    }
uint32_t starttimefromtime(uint32_t pos) {
    return starttime+floor(((double)pos-starttime)/duration)*duration;
    }
void setstarttime(uint32_t newstart) ;
uint32_t maxstarttime() ;
void setdiffcurrent() ;
void begrenstijd() ;
void flingX(float vol) ;
void  calccurvegegs();
void prevscr() ;
void  nextscr() ;

int mouseScale(float dx,float xold,float x);
int translate(float dx,float dy,float yold,float y) ;
void xscaleGesture(float scalex,float midx) ;
std::pair<int,int> getextremes(const std::vector<int> &hists, const std::pair<const ScanData *,const ScanData*> **scanranges, int scannr,const std::pair<int32_t,int32_t> *histpositions) ;
template <class TX,class TY> bool nearbyhistory( const float tapx,const float tapy,  const TX &transx,  const TY &transy) ;
template <class TX,class TY> bool nearbycalibratedhistory( const float tapx,const float tapy,  const TX &transx,  const TY &transy);
void highlightnum(const Num *num) ;
int nextpast() ;
int nextforward() ;
void glucosesel(uint32_t tim);
uint32_t glucosesearch(uint32_t starttime,uint32_t endtime) ;
int searchcommando(int type, float under,float above,int frommin,int tomin,bool forward,const char *regingr,float amount) ;
uint32_t glucoseforwardsearch(uint32_t starttime,uint32_t endtime) ;
void prevdays(int nr);
void nextdays(int nr) ;
int64_t doehier(int menu,int item,bool) ;
void endnumlist();
void	shower(NVGcontext* vg,const Num *num,const float xpos,const float xend,const float ypos) ;
template <typename F> void numscreen(NVGcontext* vg, const F & col);
template <typename F> void numscreenback(NVGcontext* vg, const F & col);
void initcolumns( NVGcontext* vg);
template <typename F> void columnfrombelow(NVGcontext* vg,int nr,const F &show);
template <typename F> void columnfromabove(NVGcontext* vg,const F &show) ;

 float second(geo_t&geo) const;
float colwidth(geo_t&geo) const;
void shownums(NVGcontext* vg, NumIter<Num> *numiters, const int nr) ;
void shownumsback(NVGcontext* vg, NumIter<Num> *numiters, const int nr);
bool percurvegegs(uint32_t setend) ;
inline const NVGcolor *getcolor(const int col) {
	return settings->data()->colors+startincolors+col;
	}
inline void setcolor(const int colindex,const NVGcolor col) {
	settings->data()->colors[startincolors+colindex]=col;
	}
inline const NVGcolor *getblack() {
	if(startincolors)
		return &white;
	return &black;
	}
inline const NVGcolor *getmenucolor() {
	if(startincolors)
		return &darkmenu;
	return &black;
	}
inline const NVGcolor *getgray() {
	if(startincolors)
		return &backgroundgray;
	return &foregroundgray;
	}
inline const NVGcolor *getdarkgray() {
	if(startincolors)
		return &backgrounddarkgray;
	return &darkgrayin;
	}
inline const NVGcolor *getwhite() {
	if(startincolors)
		return &black;
	return &white;
	}
inline const NVGcolor *getyellow() {
	if(startincolors)
		return &yellowinvert;
	else
		return &yellow;
	}
inline const NVGcolor *getthreehour() {
	if(startincolors)
		return &backgroundthreehour;
	return &foregroundthreehour;
	}

void invertcolorsset(bool val) {
	invertcolors=val;
	 startincolors=val*startbackground;
         }
void setinvertcolors(bool val) {
        invertcolorsset(val);
	settings->data()->invertcolors=val;
	 }
int numfrompos(const float x,const float y) ;
template <class TX,class TY> NumHit *nearbynum(const float tapx,const float tapy,const TX &transx,  const TY &transy) ;
int typeatheight(const float h) ;
float getfreey() ;
void numpagenum(const uint32_t tim) ;
void numiterinit() ;
float                getboxwidth(const float x) ;
template <class LT> const displaytime getdisplaytime(const uint32_t nu,const uint32_t starttime,const uint32_t endtime, const LT &transx) ;
uint32_t minstarttime() ;

inline float	numtypeheight(const int type)  {
		float schuif=statusbarheight+smallfontlineheight*
#ifdef WEAROS
   3.8
#else
      3.25
#endif

      ;
		return dtop+schuif+(dheight-schuif-smallfontlineheight/2)*(numheights[type])/(shownlabels-1);
		}
int largedaystr(const time_t tim,char *buf);

inline int datestr(const time_t tim,char *buf) {
	struct tm tmbuf;
	 struct tm *stm=localtime_r(&tim,&tmbuf);
	int len=sprintf(buf,"%s %02d-%02d-%d ",usedtext->daylabel[stm->tm_wday],stm->tm_mday,stm->tm_mon+1,1900+stm->tm_year);
    len+=mktime(stm->tm_hour,stm->tm_min,buf+len);
   return len;
	}

void leginterval(NVGcontext* vg,const float x,const float y, const int *between);

inline int mkshowlow(char *buf, const int maxbuf,int lowest) {
    return snprintf(buf,maxbuf,"%.*f>",gludecimal,gconvert(lowest*10));
    }
inline int mkshowhigh(char *buf, const int maxbuf,int glucosehighest) {
    return snprintf(buf,maxbuf,"%.*f<",gludecimal,gconvert(glucosehighest*10));
    }

template <int N> const Num *findpast();
template <int N=1> const Num *findforward();

bool hasHidden=false;
void    showHideButton(NVGcontext* avg);
void unhide();
bool allvalues=false;
bool CalibratePast=false;

void setsearchshow(int type);
};
struct AppCurve:JCurve {
    AppCurve() { }
    };
extern AppCurve appcurve;
extern int numlist;

struct shownglucose_t {
    const char *errortext=nullptr;
    int glucosetrend;
    float glucosevalue=0;
    float glucosevaluex=-1,glucosevaluey=-1;
    } ;


