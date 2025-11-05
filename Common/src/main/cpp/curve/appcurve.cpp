
#include <GLES2/gl2.h>
#include "config.h"
#include "SensorGlucoseData.hpp"
#include "sensoren.hpp"
#define NANOVG_GLES2_IMPLEMENTATION
#include "curve.hpp"
#include "nanovg_gl.h"
#include "nanovg_gl_utils.h"
#include "nums/numdata.hpp"
#include "settings/settings.hpp"

#include "misc.hpp"
#include "JCurve.hpp"
#include "gluconfig.hpp"
extern double     calibrateNow(const SensorGlucoseData *sens,const ScanData &value);

extern Sensoren *sensors;
extern int *numheights;
extern float tapx,tapy;
extern void speak(const char *message) ;
static int nrmenu=0,selmenu=0;
struct lastscan_t scantoshow={-1,nullptr}; 
extern bool makepercetages() ;

extern bool hasnetwork();

bool bluetoothEnabled();
float                JCurve::getboxwidth(const float x) {
                    return std::max((float)(dwidth-x-smallsize),dwidth*.25f);
                    }
#include "numdisplay.hpp"
extern vector<NumDisplay*> numdatas;
#include "numiter.hpp"
NumIter<Num> *numiters=nullptr;
int basecount;

#include "numhit.hpp"
extern NumHit newhit;
extern Num newnum;
#ifdef NOTALLVIES
int betweenviews=60*30;
time_t nexttimeviewed=0;
#endif

extern bool selshown;
extern bool speakout;


bool emptytap=false;
extern bool fixatex,fixatey;
extern std::vector<shownglucose_t> shownglucose;
extern void setusedsensors(uint32_t nu) ;

extern void mkheights() ;
void    updateusedsensors(uint32_t nu) {
    static int wait=0;
    static int32_t waslast=-1;
    int newlast=sensors->last();
    if(waslast!=newlast||!wait) {
        LOGAR("updateusedsensors");
        waslast=newlast;
        setusedsensors(nu);
        wait=100;
        mkheights(); 
        }
    else
        wait--;
    
    }


static void printGlString(const char* name, GLenum s) {
#ifndef NDEBUG
    const char* v = (const char*)glGetString(s);
    if(v)
        LOGGER("GL %s: %s\n", name, v);
#endif
}

int numlist=0;
bool alarmongoing=false;
 int    JCurve::showoldscan(NVGcontext* avg,uint32_t nu) {
    if(!scantoshow.scan) 
        return 0;
    if((nu-scantoshow.showtime)>=60) {
          scantoshow={-1,nullptr}; 
          return -1;
          }
    numlist=0;
    const SensorGlucoseData *hist=sensors->getSensorData(scantoshow.sensorindex);
    showscanner(avg,hist,scantoshow.scan-hist->beginscans(),nu) ;
    return 1;
    }

struct {
    float x=-300.0f,y=-300.0f;
    std::chrono::time_point<std::chrono::steady_clock>  time;
} prevtouch;
#include "strconcat.hpp"
class histgegs {
   //const int sensorindex;
    const SensorGlucoseData *hist;
    time_t nu;
public:
#ifndef DONTTALK
strconcat text;
#endif
    histgegs(const SensorGlucoseData *hist): hist(hist)/*,glu(glu),tim(tim)*/,nu(time(nullptr))
#ifndef DONTTALK
    ,text(getsensorhelp(usedtext->menustr0[3],": ","\n","\n"," "))
#endif
    {


    prevtouch.time = chrono::steady_clock::now();
    LOGGER("histgegs %s",ctime(&nu));
    } 
strconcat  getsensorhelp(string_view starttext,string_view name1,string_view name2,string_view sep1,string_view sep2,string_view endstr="") {
    char starts[50],ends[50],pends[50];
//   const sensor *sensor=sensors->getsensor(sensorindex);
    time_t stime=hist->getstarttime(),etime= hist->officialendtime();
    //time_t reallends=hist->isLibre3()?etime:sensor->maxtime();
    time_t reallends=hist->expectedEndTime();
    char lastscanbuf[50],lastpollbuf[50];
    time_t lastscan=hist->getlastscantime();
    time_t lastpolltime=hist->getlastpolltime();
    return strconcat(string_view(""),starttext ,name1,hist->showsensorname(),name2,usedtext->sensorstarted,sep2,string_view(starts, appcurve.datestr(stime,starts)),!hist->isLibre2()?"":sep1,!hist->isLibre2()?"":usedtext->lastscanned,!hist->isLibre2()?"":sep2,!hist->isLibre2()?"":string_view(lastscanbuf,appcurve.datestr(lastscan,lastscanbuf)),lastpolltime>0?strconcat(string_view(""),sep1,usedtext->laststream,sep2):"",lastpolltime>0?string_view(lastpollbuf,appcurve.datestr(lastpolltime,lastpollbuf)):"",nu<etime?strconcat(string_view(""),sep1,usedtext->sensorends,sep2):"",
nu<etime?string_view(ends, appcurve.datestr(etime,ends)):string_view("",0),sep1,usedtext->sensorexpectedend,sep2,string_view(pends, appcurve.datestr(reallends,pends)),endstr);;
    }
#ifndef DONTTALK
void speak() {
    LOGGER("speak %s\n",text.data());
    ::speak(text.data());
   }
#endif

};



float JCurve::getfreey() {
    const int nrlabs=settings->getlabelcount(); 

    static const int mid=nrlabs/2-1;
    static const float midh=numtypeheight(mid);
    static  float boven=numtypeheight(mid+1);
    return (boven+midh)/2.0f;
    }
int JCurve::typeatheight(const float h) {
//    const float gr= density*24;
    const float gr= density*24;
    const int maxl= settings->getlabelcount();
    for(int i=0;i<maxl;i++) {
        if(numheights[i]>=0) {
            float th= numtypeheight(i);
            if(fabsf(h-th)<gr)
                return i;
            }
        }
    return -1;    
    }


#define makeshows(x) \
  void setshow##x(int val) {\
        settings->data()->show##x=val;\
        }\
  int getshow##x() {\
        return settings->data()->show##x;\
        }
makeshows(calibrated)
makeshows(scans)
makeshows(stream);
makeshows(meals)
makeshows(numbers)
makeshows(histories)
#ifndef WEAROS
extern bool showpers;
extern void showpercentiles(NVGcontext* avg) ;
#endif
int getalarmcode(const uint32_t glval,float drate,SensorGlucoseData *hist) ;
extern void     processglucosevalue(int sendindex,int newstart) ;
struct {
float left,top,right,bottom;
} menupos,hidepos;
#ifndef NOLOG
void logmenupos() {
    LOGGER("left=%.1f top=%.1f right=%.1f bottom=%.1f\n",menupos.left,menupos.top,menupos.right,menupos.bottom);
    }
#else
#define logmenupos() 
#endif
extern std::vector<int> usedsensors;
static bool  inmenu(float x,float y) ;

 const float     JCurve::getsetlen(NVGcontext* avg,float x, float  y,const char * set,const char *setend,bounds_t &bounds) {
         nvgTextBounds(avg, x,  y, set,setend, bounds.array);
        return bounds.xmax-bounds.xmin;
        }
inline int64_t menuel(int menu,int item) {
    return menu+item*0x10LL;
    }
#ifdef WEAROS
int64_t JCurve::doehier(int menu,int item) {
    switch(menu) {
        case 0: 
            switch(item) {
                case 0 :  nrmenu=0;return 1LL*0x10+1;                
                case 1 : nrmenu=0;return 3LL*0x10;
                case 2: //invertcolors=!invertcolors; setinvertcolors(invertcolors) ; return -1LL; break;

                    nrmenu=0;
                    return menuel(0,2);
                case 3: nrmenu=0; return 4LL*0x10;
                case 4: nrmenu=0; return menuel(0,7);

                };break;
        case 1: 
            nrmenu=0;
            switch(item) {
                case 0: return 2LL*0x10+3;
//                case 1: return 1LL*0x10+3;
                case 1: {
                    auto max=time(nullptr);
                    setstarttime(max-duration*3/5);
                    return -1LL;
                    }
                case 2: prevdays(1); return -1LL;
                case 3: 
//                    if(settings->staticnum()) return -1LL;
                    return  menuel(1,2);
                };
        }
    return -1LL;
    }
#else
int64_t JCurve::doehier(int menu,int item) {
    switch(menu) {
        case 0: 
            switch(item) {
                case 0: 
                    showui=!showui;
                    settings->setui(showui);
                    break;
                default:
                    nrmenu=0;
                    break;
                };break;
        case 1: switch(item) {
//                case 0: notify=!notify;return menu|item*0x10|(notify<<8);
                case 2: nrmenu=0;
//                    if(settings->staticnum()) return -1LL;
                    break;
                case 3:    nrmenu=0; 
                   if(!numlist) {
                      numiterinit();
                      numlist=1;
                      }
                    break;

#ifdef PERCENTILES
                case 4:  nrmenu=0; if(!makepercetages())
                        return -1LL;
                    ;break;
#endif
                case 6:break;
                default:
                    nrmenu=0;
                    break;
                };break;
        case 2:
            switch(item)     {
            /*
                case 0: {
                    nrmenu=0;
                    int lastsensor=sensors->lastscanned();
                    if(lastsensor>=0) {
                        const SensorGlucoseData *hist=sensors->getSensorData(lastsensor);
                        if(hist) {
                            const ScanData *scan= hist->lastscan();
                            const uint32_t nu= time(nullptr);
                            if(scan&&scan->valid()&&((nu-scan->t)<(60*60*5)))
                                scantoshow={lastsensor,scan,nu};
                            }
                        }
                    }; return -1ll; */
                case 0:showcalibrated=!showcalibrated; setshowcalibrated(showcalibrated);return -1ll;
                case 1:showscans=!showscans; setshowscans(showscans);return -1ll;
                case 2:showstream=!showstream; setshowstream(showstream);return -1ll;
                case 3:showhistories=!showhistories; setshowhistories(showhistories);return -1ll;
                case 4: shownumbers=!shownumbers; setshownumbers(shownumbers);return -1ll;
                case 5: showmeals=!showmeals; setshowmeals(showmeals);return -1ll;

                case 6: invertcolors=!invertcolors; setinvertcolors(invertcolors) ; return menu+invertcolors*0x10;
            break;//return -1ll;
                };break;
        case 3: {
        nrmenu=0;
        switch(item) {
            case 0: {
            auto max=time(nullptr);
        //    starttime=starttimefromtime(max);
//            if((starttime+duration)<max) 
            setstarttime(max-duration*3/5);
            return -1;
                };
            case 3: prevdays(1); return -1;
            case 4: nextdays(1);return -1;
            case 5: prevdays(7); return -1;
            case 6: nextdays(7);return -1;        
            default: break;
            };
            };break;

        default: nrmenu=0;
        }
    return menu+item*0x10;
    }
#endif






static bool dohealth(int sensorindex) {
    static int thesensor=sensorindex;
    if(!settings->data()->healthConnect)
        return false;
    if(usedsensors.size()==1)  {
        return true;
    }
    if(sensorindex==thesensor)
        return true;
    auto en=usedsensors.end();
    if(std::find(usedsensors.begin(),en,thesensor)==en) {
        thesensor=sensorindex;
        return true;
    }
    return false;
}
#ifndef NDEBUG
#define lognum(x)
#else
void lognum(const Num *num) {
        constexpr int maxitem=80;
        char item[maxitem];
        time_t tim=num->time;
        int itemlen=appcurve.datestr(tim,item);
        if(num->type< settings->getlabelcount()) {
            item[itemlen++]=' ';
            decltype(auto) lab=settings->getlabel(num->type);
            memcpy(item+itemlen,lab.data(),lab.size());
            itemlen+=lab.size();
            item[itemlen]='\0';
            }
        LOGGER("%s %.1f\n",item,num->value);
        }
#endif    
#ifndef DONTTALK
static int verbosedate(time_t tim,char *buf,int maxbuf=256) {
    struct tm tmbuf;
    struct tm *stm=localtime_r(&tim,&tmbuf);
    const auto wdaynr= stm->tm_wday;
    const char *dayname=usedtext->speakdaylabel[wdaynr];
    return snprintf(buf,maxbuf,"%s %d %s %d",dayname,stm->tm_mday,usedtext->monthlabel[stm->tm_mon],1900+stm->tm_year);
    }
static void speakdate(time_t tim) {
    constexpr const int maxbuf=256;
    char buf[maxbuf];
    verbosedate(tim,buf,maxbuf);
    LOGGER("speakdate %s\n",buf);
    speak(buf);
    }
#endif

static int64_t menutap(float x,float y) {
    if(x<menupos.left||x>=menupos.right) {
        nrmenu=0;
        return -1LL;
        }
    float dist=(menupos.bottom-menupos.top)/nrmenu;
    int item=(y-menupos.top)/dist;
    if(item>=0&&item<nrmenu) {

        LOGGER("menuitem %d\n",item);
    //    return doehier(getmenu(x),item);
        return appcurve.doehier(selmenu,item);
        }
    nrmenu=0;
    return -1LL;    
    }

void JCurve::unhide() {
    LOGGER("unhide %d\n",hidden.size());
    for(int index:hidden) {
        SensorGlucoseData *sens=sensors->getSensorData(index);
        sens->hide=false;
        } 
     }
int64_t JCurve::screentap(float x,float y) {
    if(hasHidden) {
       if(!(x<hidepos.left||x>=hidepos.right|| y<hidepos.top||y>=hidepos.bottom)) {
            unhide();
            hasHidden=false;
            return -3LL;
            }
        }
#ifndef WEAROS
    if(!showpers ) 
    {

        if(numlist)  {

            if(int index=numfrompos(x,y);index>=0) {
                const Num *num=numiters[index].prev();
                if(!numdatas[index]->valid(num))
                    return -2LL;

                int pos=num-numdatas[index]->startdata();
                int base=numdatas[index]->getindex();
                return (static_cast<int64_t>(pos)<<16)|((static_cast<int64_t>(base)&0xf)<<8)|0xe;
                }
            else
                return -2LL;
            }
#else
    {
#endif

        if(emptytap) {
            return -1LL;
            }
        if(nrmenu)  {
            return menutap(x,y);
            }
        if(showmeals) {
            const float crit= (density*10);
            for(mealposition &p:mealpos) {
                if((y>=p.mealstarty&&y<p.mealendy)&&abs(x-p.mealx)<crit)
                    return (static_cast<int64_t>(p.mealpos)<<16)|((static_cast<int64_t> (p.mealbase)&0xf)<<8)|0xe;
                }

            }
#ifndef DONTTALK
        if(speakout) {
            for(auto &el:shownglucose) {
                LOGGER("x=%f [%f,%f] y=%f [%f,%f] trend=%d\n", x,el.glucosevaluex,
                       (el.glucosevaluex + headsize), y, (el.glucosevaluey - headsize),el.glucosevaluey,
                      el.glucosetrend);
                if (el.glucosevaluex > 0 && x > el.glucosevaluex && x < (el.glucosevaluex + headsize*1.2f) &&
                    y < el.glucosevaluey && y > (el.glucosevaluey - headsize*.8f)) {
                    if(el.glucosevalue > 0) {
                        constexpr const int maxvalue = 80;
                        char value[maxvalue];
//                        auto trend = usedtext->trends[el.glucosetrend];
                        const auto trend = usedtext->getTrendName(el.glucosetrend);

                        memcpy(value, trend.data(), trend.size());
                        char *ptr = value + trend.size();
                        *ptr++ = '\n';
                        *ptr++ = '\n';
                        snprintf(ptr, maxvalue, gformat, el.glucosevalue);
                        speak(value);
                        return -1LL;
                    } else {
                        const char *error=el.errortext;
                        if(error)  {
                            speak(error);
                            return -1LL;
                            }
                    }
                }
            }
        }
#endif
        }
#ifndef DONTTALK
    if(speakout) {
         const float hgrens=menutextheight+statusbarheight;
        if(y<hgrens) {
             const float wgrens=menutextheight*1.5f+statusbarleft;
            if(x<wgrens) {
                speakdate(starttime);
                return -1LL;
                }
            else {
                 if(x>(dwidth-statusbarright-wgrens)){
                    const time_t endtime=starttime+duration;
                    const time_t nu=time(nullptr); 
                    if(endtime>nu)
                        speakdate(nu);
                    else
                        speakdate(endtime);
                    return -1LL;
                    }
                }
            }
        }
#endif
#ifndef WEAROS 
    const float wgrens=density*10+statusbarleft;
    const float rgrens=dwidth-statusbarright-wgrens;

    if(x<wgrens)  {
            prevscr();
            return -1LL;
            }
        
    else 
        if (x > rgrens)  {
        nextscr();
        return -1LL;
        }
        
    if(showpers)  {

extern bool showsummarygraph;
        if(showsummarygraph) {
            showsummarygraph=false;
                  fixatey=settings->data()->fixatey;
            return 1+4*0x10;
            }
        }
    else
#endif
    {
        tapx=x;tapy=y;
        }
    return -1LL;
    }
 void  JCurve::scanwait(NVGcontext* avg) {
    startstep(avg,*getwhite());
    nvgFontSize(avg, headsize);
    nvgFillColor(avg, *getblack());
    nvgTextAlign(avg,NVG_ALIGN_CENTER|NVG_ALIGN_MIDDLE);
    const std::string_view str1=usedtext->scanned;
    nvgText(avg, dleft+dwidth/2,dtop+dheight/2, str1.begin(), str1.end());
    endstep(avg);
    }
bool restart=false;
//static int showoldscan(NVGcontext* avg,uint32_t ) ;

 void    JCurve::defaulterror(NVGcontext* avg,int scerror)   {
        char buf[50];
        const errortype *error=usedtext->scanerrors;
        size_t len=snprintf(buf,50,error->first.data(),scerror);
        showerror(avg,error->second,{buf,len});
        }


 bool    JCurve::errorpair(NVGcontext* avg,const errortype &error) {
    return showerror(avg,error.first,error.second);
    }

int    JCurve::badscanMessage(NVGcontext* avg,int kind) {
    const uint32_t nu=time(nullptr);
    int res=1;
    switch(showoldscan(avg,nu)) {
        case 0: {
            LOGGER("javabadscan    %d: \n",kind);
            const int scerror= kind&0xff;
            switch(scerror) {
                case 0xF8: {
                        errorpair(avg,usedtext->libre3zeroID);
                        };break;
                case 0xF9: {
                    showerror(avg,usedtext->nolibre3.first,usedtext->needsandroid8);
                    };break;
                case 0xFA: {
    //                showerror(avg,"FreeStyle Libre 3, Scan error", "Try again");
                    errorpair(avg,usedtext->libre3scanerror);
                    };
                    break;
                case 0xFB:
                    errorpair(avg,usedtext->libre3wrongID);
                    break;
    //                showerror(avg,"Error, wrong account ID?","Specify in Settings->Libreview the same account used to activate the sensor");break;
                 case 0xFC: {
                    errorpair(avg,usedtext->libre3scansuccess);
    //                showerror(avg,"FreeStyle Libre 3 sensor", "Glucose values will now be received by Juggluco");
                    };break;
                case 0xFD: {
                    errorpair(avg,usedtext->unknownNFC);
    //                showerror(avg,"Unrecognized NFC scan Error", "Try again");
                    };break;
                case 0xFE: {
                    errorpair(avg,usedtext->nolibre3);
    //                showerror(avg,"FreeStyle Libre 3 sensor","Not supported by this version of Juggluco"  );
                    };break;
                case 0xFF: {
                    scanwait(avg);     
                    return 2;
                    };
                case 5: {
                    const errortype *error=usedtext->scanerrors+scerror;
                    const int bufsize=error->second.size()+5;
                    char buf[bufsize];
                    size_t len=snprintf(buf,bufsize,error->second.data(),kind>>8);
                    showerror(avg,error->first,{buf,len});
                    LOGGER("%s\n",buf);
                    };break;
    /*                        case 7: {
                    auto [first,second]=usedtext->scanerrors[scerror];
                                    showerror(avg,first,error,second);
                                    };break; */
                case 0:
                case 15:
                case 9: defaulterror(avg,scerror);
                    break;
                case 12: restart=true;
                default: 
                 if(scerror>0x10) {
                    defaulterror(avg,scerror);
                    }
                else {
                    errorpair(avg,usedtext->scanerrors[scerror]);
    //                errortype *error=usedtext->scanerrors+scerror; showerror(avg,error->first,error->second);
                    }
                };break;
            }
        case -1: res=2;break;
            };
    endstep(avg);    
    return res;
    }

#ifdef USE_DISPLAYER
#include "displayer.hpp"
std::unique_ptr<histgegs> displayer;
#endif
strconcat getsensortext(const SensorGlucoseData *hist) {
        if((hist->isDexcom()||hist->isSibionics())&&hist->unused()) {
            return {"",R"(<h1>)",hist->showsensorname(),R"(</h1><p>)",usedtext->waitingforconnection,"</p>"};
            }
        else {
            histgegs gegs(hist);
            return gegs.getsensorhelp("","<h1>","</h1>","<br><br>",
            #ifdef WEAROS
            "<br>"
            #else
            "\t\t"
//            "&emsp;&emsp;&emsp;"
            #endif
            ,"");
            }
        }

static void showhistory(SensorGlucoseData *hist,const float tapx, const float tapy) {
//#ifdef WEAROS
#if 1 
                        histgegs gegs(hist);

extern void callshowsensorinfo(const char *text,SensorGlucoseData *hist);
                        callshowsensorinfo(gegs.getsensorhelp("","<h1>","</h1>","<br><br>",
            #ifdef WEAROS
            "<br>"
            #else
            "\t\t"
           // "&emsp;&emsp;&emsp;"
            #endif

                        ,"").data(),hist);

#ifndef DONTTALK
                        if(speakout) gegs.speak();
#endif
#else
                        ::prevtouch.x=tapx;
                        ::prevtouch.y=tapy;
                        LOGGER("x=%.1f, y=%.1f\n",tapx,tapy);
                        histgegs *gegs=new histgegs(hist);
                        if(speakout) gegs->speak();
                        displayer.reset(gegs);
#endif
      }

template <class TX,class TY> 
bool JCurve::nearbyhistory( const float tapx,const float tapy,  const TX &transx,  const TY &transy) {
    for(int i=histlen-1;i>=0;i--) {
      const int sensorindex= hists[i];
        SensorGlucoseData *hist=sensors->getSensorData(sensorindex);
        const auto [firstpos,lastpos]=histpositions[i];
            for(auto pos=firstpos;pos<=lastpos;pos++) {
                uint32_t tim,glu;
                if((tim=hist->timeatpos(pos))&&( glu=hist->sputnikglucose(pos))) {
                    auto posx=transx(tim),posy=transy( glu);
                    if(nearby(posx-tapx,posy-tapy,density)) {
                        showhistory(hist,tapx,tapy);
                        return true;
                        }
                    }
                }
        }
    return false;
    }

#ifndef WEAROS
static int largepausedaystr(const time_t tim,char *buf) {
        LOGAR("largepausedaystr");
    struct tm stmbuf;
    localtime_r(&tim,&stmbuf);
     //return sprintf(buf,"%s %02d %s %d\n%02d:%02d",usedtext->speakdaylabel[stmbuf.tm_wday],stmbuf.tm_mday,usedtext->monthlabel[stmbuf.tm_mon],1900+stmbuf.tm_year,stmbuf.tm_hour,mktmmin(&stmbuf));
     int len=sprintf(buf,"%s %02d %s %d\n",usedtext->speakdaylabel[stmbuf.tm_wday],stmbuf.tm_mday,usedtext->monthlabel[stmbuf.tm_mon],1900+stmbuf.tm_year);
   len+=mktime(stmbuf.tm_hour,mktmmin(&stmbuf),buf+len);
   return len;
    }
static void speaknum(const Num *num) {
    char buf[256];
    char *ptr=buf;
    auto label=settings->getlabel(num->type);
    memcpy(ptr,label.data(),label.size());
    ptr+= label.size();
    *ptr++='\n';
    ptr+=sprintf(ptr,"%g",num->value);
    *ptr++='\n';
    *ptr++='\n';
#ifndef NOLOG
    int len=
#endif
             largepausedaystr(num->gettime(),ptr);
    LOGGERN(buf,ptr-buf+len);
    speak(buf);
    }
#endif

#ifndef DONTTALK
    static bool speakmenutap(float x,float y) ;
#endif


template <class TX,class TY> const ScanData * nearbyscan(const float tapx,const float tapy,const ScanData *low,const ScanData *high,  const TX &transx,  const TY &transy,float density) {
    for(const ScanData *it=low;it!=high;it++) {
        if(it->valid()) {
            const uint32_t tim= it->t;
            const auto glu=it->g*10;
            const auto posx= transx(tim),posy=transy(glu);
            if(nearby(posx-tapx,posy-tapy,density))  {
                return it;
                }
            }
        }
    return nullptr;
    }
int64_t JCurve::longpress(float x,float y) {
    LOGGER("longpress x=%.1f y=%.1f\n",x,y);
#ifndef WEAROS
    if(showpers)
        return 0LL;
    if(numlist)  {
        if(speakout) {
            if(int index=numfrompos(x,y);index>=0) {
                const Num *num=numiters[index].prev();
                if(numdatas[index]->valid(num)) {
                    speaknum(num);
                    }
                }
            }
        return 0LL;
        }
#endif
#ifndef DONTTALK
    if(speakout) {
        if(speakmenutap(x,y))
            return 0LL;
        }
    else
#endif
    {
        if(inmenu(x,y)) {
            return 0LL;
            }
          }
    const uint32_t endtime=starttime+duration;
    const auto [transx,transy]= gettrans(starttime, endtime);
    if(shownumbers)
        if(NumHit *hit= nearbynum(x,y,transx,transy))
            return reinterpret_cast<int64_t>(hit);
    if(showhistories&& nearbyhistory( x,y,  transx,  transy) ) {
        return 0LL;
        }
    if(showscans) {
        for(int i=histlen-1;i>=0;i--) {
            if(const ScanData *scan=nearbyscan(x,y,scanranges[i].first,scanranges[i].second,transx,transy,density)) {
                LOGGER("longpress scan %.1f\n",scan->g/convfactordL);
                int index=hists[i];
                scantoshow={index,scan,static_cast<uint32_t>(time(nullptr))};
                return 0LL;
                }
             }
         }
    if(showstream) {
        for(int i=histlen-1;i>=0;i--) {
            if(const ScanData *poll=nearbyscan(x,y,pollranges[i].first,pollranges[i].second,transx,transy,density)) {
                LOGGER("longpress poll %.1f\n",poll->g/convfactordL);
                const int sensorindex= hists[i];
                SensorGlucoseData *hist=sensors->getSensorData(sensorindex);
                showhistory(hist,x,y);
                return 0LL;
                }
             }
         }
    if(showcalibrated) {
extern std::pair<const ScanData*,const ScanData*>      makecalibrated(const SensorGlucoseData *sens,const ScanData *input,ScanData *calibrated,int nr,bool allvalues);
extern std::pair<const ScanData*,const ScanData*>      makecalibratedback(const SensorGlucoseData *sens,const ScanData *input,ScanData *calibrated,int nr,bool allvalues);
        auto califunc=settings->data()->CalibratePast?makecalibratedback:makecalibrated;
        for(int i=histlen-1;i>=0;i--) {
                const int index= hists[i];
                auto *sens=sensors->getSensorData(index);
                const int nr=pollranges[i].second-pollranges[i].first;
                ScanData calibrated[nr];
                const auto span=califunc(sens,pollranges[i].first,calibrated,nr,allvalues);
                if(const ScanData *poll=nearbyscan(x,y,span.first,span.second,transx,transy,density)) {
                    LOGGER("longpress poll %.1f\n",poll->g/convfactordL);
                    showhistory(sens,x,y);
                    return 0LL;
                    }
                 }
         }
    int type=typeatheight(y); 
    if(type>=0)  {
        newnum.time=starttime+duration*(x-dleft)/dwidth;
        newnum.type=type;
        newnum.value=NAN;
        return reinterpret_cast<int64_t>(&newhit);
        }

    return 0LL;
    }
#ifndef WEAROS
void settoend() ;
void shownumiters() ;
#include "oldest.hpp"
bool numpagepast() {
    if(!numiters)
        return false;
    if(numdatas.size()<basecount)
        return false;
    bool onstart=true;
    for(int i=0;i<basecount;i++) {
        if(getpageoldest(i)>numiters[i].begin) {
            onstart=false;
            }
        }
    if(!onstart) {
        for(int i=0;i<basecount;i++) {
            setpagenewest(i,getpageoldest(i));
            setpageoldest(i,nullptr);
            }
        }
    return onstart;
//    shownumfromtop();
}

extern int nrcolumns;
int nrcolumns=1;
int JCurve::numfrompos(const float x,const float y) {
    int rows=((dheight-statusbarheight)/(double)textheight);

    int ind= ((nrcolumns!=1&&x>(dleft+dwidth/2))?rows:0)+ std::min(rows-1,(int)((y-statusbarheight-dtop)/textheight));
    LOGGER("rows=%d, ind=%d\n",rows,ind);
    int i=0,index;
    for(int i=0;i<basecount;i++) {
        numiters[i].iter=getpageoldest(i);
        }
    do {
        index=ifindoldest(numiters,0,basecount,notvali);
        } while(i++<ind);
    return index;
    }
bool numpageforward() {
    if(!numiters)
        return false;
    if(numdatas.size()<basecount)
        return false;
    LOGSTRING("Page forward\n");
    bool noend=false;
    for(int i=0;i<basecount;i++) {
        const Num*newst=getpagenewest(i);
        if(newst<=numiters[i].end) {
            noend=true;
        }
       }
    if(noend) {
        for(int i=0;i<basecount;i++) {
            const Num*newst=getpagenewest(i);
            setpageoldest(i,newst);
            }
          }
    return !noend;
    }
//s/numiters.i..pageoldest.\([^;]*\);/setpageoldest(i,\1);/g
void  setitertobottom(NumIter<Num> *numiters, const int nr) {
    for(int i=0;i<nr;i++) {
        setpageoldest(i,numiters[i].next());
        }
    }

int numsize() {
    int basecount=numdatas.size();
    int tot=0;
    for(int i=0;i<basecount;i++)
        tot+=numdatas[i]->size();
    return tot;
}
void    JCurve::showfromend(NVGcontext* avg) {
    settoend() ;
    shownumsback(avg, numiters,basecount);
    setitertobottom(numiters,basecount);
    }


void    JCurve::showfromstart(NVGcontext* avg) {
    if(numlist==4) {
        numlist=1;
        }
    else {
        for(int i=0;i<basecount;i++) {
            numiters[i].iter=getpageoldest(i);
            }
        }
     shownums(avg, numiters, basecount);
    for(int i=0;i<basecount;i++) {
        setpagenewest(i,numiters[i].iter);
        }
    }

void JCurve::numpagenum(const uint32_t tim) {
    int tot=0;
    for(int i=0;i<basecount;i++)  {
        const Num *ptr=numdatas[i]->firstnotless(tim) ;
        if(ptr==numdatas[i]->end()||ptr->gettime()>tim)
            ptr--;
        int pos=ptr-numdatas[i]->begin();
        LOGGER("pos=%d\n",pos);
        if(pos>0) {
//            LOGGER("num %.1f type=%d\n",ptr->value, ptr->type);
            LOGGER("num %.1f %.11s\n",ptr->value, settings->getlabel(ptr->type).data());
            tot+=pos;
            }
        numiters[i].iter=ptr;
        }
    const int percol=(dheight-statusbarheight)/textheight;
    const int onpage=nrcolumns*percol;
    #ifndef NOLOG
    time_t tims=tim;
    LOGGER("nrcolumns=%d percol=%d onpage=%d %s",nrcolumns,percol,onpage,ctime(&tims));
    #endif
    for(int tever=tot%onpage
#ifndef NOLOG
    , niets=LOGGER("tever=%d\n",tever)
#endif
    ;tever>0;--tever) {
        ifindnewest(numiters,basecount,notvali);
        };
    int newest;
    for(newest=0;newest<basecount&& numiters[newest].iter>numiters[newest].end;newest++)
            ;
    for(int i=newest+1;i<basecount;i++) {
        if(numiters[i].iter<=numiters[i].end&&numiters[i].iter->gettime()>numiters[newest].iter->gettime())
            newest=i;
        }
    for(int i=0;i<basecount;i++)
        if(i!=newest&&numiters[i].iter<=numiters[i].end)
            numiters[i].inc();
    for(int i=0;i<basecount;i++) 
        setpageoldest(i,numiters[i].iter);
    numlist=4;
    }

 void    JCurve::shownumlist(NVGcontext* avg) {
    startstep(avg,*getwhite());
    if(getpageoldest(0)!=nullptr) {
        showfromstart(avg);
         }
    else {    
        showfromend(avg);
        }
    }
NumIter<Num> *mknumiters() ;


extern int getcolumns(int width);
void JCurve::numiterinit() {
    nrcolumns=getcolumns(round(3.4*smallsize));
    LOGAR("numiterinit");
    basecount=numdatas.size();
    delete[] numiters;
    numiters=mknumiters() ;
    for(int i=0;i<basecount;i++) {
        setpagenewest(i, numdatas[i]->end());
        setpageoldest(i, nullptr);
        }
    numpagenum(starttime+duration/2);
    LOGAR("end numiterinit");
    }

void numendbegin() {
    for(int i=0;i<basecount;i++) {
        setpagenewest(i, numdatas[i]->end());
    //    setpagenewest(i,numiters[i].next(numiters[i].end));
        }
    }
void settoend() {
//static bool init=true; if(init) numiterinit() ; init=false;
    for(int i=0;i<basecount;i++) {
        numiters[i].iter=getpagenewest(i)-1;
        }
    }
int onlast(int onscreen) {
    return numsize()%onscreen;
    }

void shownumiters() {
    LOGSTRING("Iters:\n");
    for(int i=0;i<basecount;i++) {
        lognum(numiters[i].iter);
        }
    }

void numfirstpage() {
    for(int i=0;i<basecount;i++)
        setpageoldest(i,numiters[i].begin);
    }


void JCurve::endnumlist() {
    numlist=0;

    uint32_t first=UINT32_MAX,second=0;
    for(const NumDisplay *num:numdatas) {
        const Num *one=std::max(num->begin(),extrums[num->numdatasPos].first);        
        const Num *two=std::min(num->end(),extrums[num->numdatasPos].second);        //NODIG?
        two--;
        while(one<=two) {
            if(!num->valid(one))  {
                one++;
                continue;
                }
            if(!num->valid(two))  {
                two--;
                continue;
                }
            if(one->gettime()<first)
                first=one->gettime();
            if(two->gettime()>second)
                second=two->gettime();
            break;
            }
        }
#ifndef NDEBUG
time_t newstart=first,start=starttime;
char buf[80];
// char *ctime_r(const time_t *timep, char *buf);

    LOGGER("endnumlist %ud %ud start=%s starttime=%ud %s",first,second,ctime(&newstart),starttime,ctime_r(&start,buf));

#endif
    if(first==UINT32_MAX)
        return;
    if((starttime+duration)>=first&&starttime<second)
        return;
    setstarttime(starttimefromtime((first+second)/2));
    return;
    }
#endif //WEAROS

bool inbutton(float x,float y) {
    return !(x<menupos.left||x>=menupos.right|| y<menupos.top||y>=menupos.bottom) ;
    }
bool isbutton(float x,float y) {
    if(!inbutton(x,y)) {
        LOGAR("isbutton false");
        return false;
        }
    if(restart) {
        LOGAR("isbutton restart");
        exit(1);    
        }
    if(inbutton(prevtouch.x,prevtouch.y)) {
             auto nutime = chrono::steady_clock::now();
             if(chrono::duration_cast<chrono::milliseconds>(nutime - prevtouch.time).count()<450) // don't close immediately if OK sits on pressed point
                return true;
            }
    scantoshow={-1,nullptr}; 
    LOGAR("isbutton true");
#ifdef USE_DISPLAYER
    displayer.reset();
#endif
    return true;
    }

void        numendbegin() ;
void JCurve::flingX(float vol) {
#ifndef WEAROS
    if(numlist)  {
        LOGSTRING("flingX\n");
        if(vol<0) {
            numlist=1;
             numendbegin() ;
            }
        return;
        }
#endif

//    starttime-=(duration*1.2*vol/dwidth);
    setstarttime(starttime-(duration*1.2*vol/dwidth));
#ifndef WEAROS
    if(!showpers)
#endif
        begrenstijd() ;
    }

bool                numpageforward() ;
            void    scrollnum() ;


bool numpagepast() ;

int JCurve::translate(float dx,float dy,float yold,float y) {
static bool ybezig=false;
    auto absdy=fabsf(dy);
    if(fabsf(dx)>absdy) {
#ifndef     WEAROS
        if(numlist) {
            auto tim = std::chrono::system_clock::now();
            static decltype(tim) oldtim{};
                std::chrono::duration<double> dif=tim-oldtim;
            const double grens=.8;
            
            if(dif.count()>grens) {
                if(dx<-10) 
                    numpagepast();
                else
                    if(!numpageforward())
                        return 0;
                oldtim=tim;
                return 1;
                }
            else
                return 0;

            }
        else
#endif
        {
            ybezig=false;
            setstarttime(starttime+1.2*(dx/dwidth)*duration);
            #ifndef WEAROS
            if(!showpers)
            #endif
                begrenstijd() ;
            return 1;
            }
        }

    else {    
        {
        if(fixatey)
            return 0;
        if(ybezig||(dheight/absdy)<convfactor) {
            ybezig=true;
            dy*=-1;
            float grens=dheight/2.0;

            if(y<grens&&yold<grens) {
                grange*=dheight/(dheight-dy*1.4);
                settime=starttime;
                setend=starttime+duration;
                }
            else if(y>grens&&yold>grens) {
                int gmax=gmin+grange;
                grange*=dheight/(dheight+dy*1.4);
                gmin=gmax-grange;
                if(gmin<0)
                    gmin=0;
                settime=starttime;
                setend=starttime+duration;
                }
            if(grange<200)
                grange=200;
           else {
                if(grange>6000)
                    grange=6000;
              }
            return 1;
            }
            }
        }
    return 0;
    }
void JCurve::xscaleGesture(float scalex,float midx) {
    if(fixatex)
        return;

    double rat=((midx-dleft)/dwidth);
    double oldduration=duration;
    uint32_t focustime=rat*oldduration+starttime;
    duration=(int)round(oldduration/pow(scalex,5.0));
constexpr const int maxduration=30*24*60*60;
constexpr const int minduration=10*60;
    if(duration>maxduration)
        duration=maxduration;
   else {
       if(duration<minduration)
                duration=minduration;
        }
    LOGGER("xscale scale=%f mid=%f oldduration=%f newduration=%d\n",scalex,midx,oldduration,duration);
    setstarttime(focustime-rat*duration);
    auto maxstart= maxstarttime();
    if(
#ifndef WEAROS
    !showpers&&
#endif

    starttime>maxstart)
        setstarttime(maxstart);
    setend=0;

    
    }


void JCurve::prevscr() {
    setstarttime(starttime-duration);
    auto minstart= minstarttime();
    if(starttime<minstart)
        setstarttime(minstart);
    }
void  JCurve::nextscr() {
    setstarttime(starttime+duration);
#ifndef WEAROS
    if(!showpers) 
#endif

    {
        auto maxstart= maxstarttime(); 
        if(starttime>maxstart) 
            setstarttime(maxstart);
        }
    }
static int64_t menutap(float x,float y) ;





void pressedback() {
    scantoshow={-1,nullptr}; 
    LOGAR("pressedback");

#ifdef USE_DISPLAYER
    displayer.reset();
#endif
    }


int  hitremove(int64_t ptr) {
    NumHit *num=reinterpret_cast<NumHit *>(ptr);
     int res=num->numdisplay->numremove(const_cast<Num*>(num->hit));
     if(numlist) {
         for(int i=0;i<basecount;i++) {
            numiters[i].end=numdatas[i]->end()-1;
            }
         }

     return res;
    }

template <class TX,class TY>  const Num * NumDisplay::getnearby(JCurve &jcurve,const TX &transx,  const TY &transy,const float tapx,const float tapy) const {
    auto [low,high]=jcurve.extrums[numdatasPos]; 
    for( const Num *it=low;it!=high;it++) {
        if(valid(it)) {
            float xpos= transx(it->time);
            float ypos=settings->getlabelweightmgperL(it->type)?transy(it->value*settings->getlabelweightmgperL(it->type)):jcurve.numtypeheight(it->type);
            if(nearby(xpos-tapx,ypos-tapy,jcurve.density))
                return it;

            }
        }
    return nullptr;
    }
template <class TX,class TY> NumHit *JCurve::nearbynum(const float tapx,const float tapy,const TX &transx,  const TY &transy) {
     for(auto el:numdatas) 
        if(const Num *hit=el->getnearby(*this,transx,transy,tapx,tapy)) {
            return new  NumHit({el,hit});
            }
     return nullptr;
    }

 
void    JCurve::showok(NVGcontext* avg,bool good,bool up) {
    nvgFontSize(avg,headsize/4 );
    nvgTextAlign(avg,NVG_ALIGN_RIGHT|(up?NVG_ALIGN_TOP:NVG_ALIGN_BOTTOM));
    const float fromtop= mediumfont*2.0f;
    float ypos=dtop+(up?fromtop:(dheight-fromtop));
    float xpos=dwidth-statusbarright+dleft-mediumfont*3.0f;

    const char *ok=good?"OK":"ESC";
    const int oklen=good?2:3;
    nvgTextBounds(avg, xpos,ypos ,ok , ok+oklen, (float *)&menupos);
    nvgText(avg, xpos,ypos,ok,ok+oklen);
    menupos.left-=mediumfont;
    menupos.right+=mediumfont;
    menupos.bottom+=mediumfont;
    menupos.top-=mediumfont;
    }

void    JCurve::showOK(NVGcontext* avg,float xpos,float ypos) {
    showButton(avg, xpos, ypos,"OK"sv);
    }
void    JCurve::showButton(NVGcontext* avg,float xpos,float ypos,std::string_view str) {
    nvgFontSize(avg,headsize/4 );

    const char *start=&str[0];
    const char *ends=str.end();
    nvgTextBounds(avg, xpos,ypos ,start ,ends, (float *)&menupos);

    nvgText(avg, xpos,ypos,start,ends);
    menupos.left-=mediumfont;
    menupos.right+=mediumfont;
    menupos.bottom+=mediumfont;
    menupos.top-=mediumfont;
    }

void    JCurve::showHideButton(NVGcontext* avg) {
    const std::string_view str=usedtext->unhide;  
    nvgFontSize(avg,headsize/4 );
    nvgFillColor(avg, *getblack());
    float ypos=dtop+dheight;
    const char *start=&str[0];
    const char *ends=str.end();
#ifdef WEAROS
    float xpos=dwidth*.5;
    nvgTextAlign(avg,NVG_ALIGN_CENTER|NVG_ALIGN_BOTTOM);
    nvgTextBounds(avg, xpos,ypos ,start ,ends, (float *)&hidepos);
    nvgText(avg, xpos,ypos,start,ends);
    hidepos.left-=mediumfont;
    hidepos.right+=mediumfont;
    hidepos.bottom+=mediumfont;
    hidepos.top-=mediumfont;
#else
    const float xaf=statusbarright?statusbarright:mediumfont;
    float xpos=dleft+dwidth-xaf;

    nvgTextAlign(avg,NVG_ALIGN_LEFT|NVG_ALIGN_BOTTOM);
    nvgTextBounds(avg, xpos,ypos ,start ,ends, (float *)&hidepos);
    float w=hidepos.right-hidepos.left;
    float xoff=-w;
    float yoff=0;
    nvgText(avg, xpos+xoff,ypos+yoff,start,ends);
    hidepos.left+=xoff-mediumfont;
    hidepos.right+=xoff+mediumfont;
    hidepos.bottom+=yoff+mediumfont;
    hidepos.top+=yoff-mediumfont;
    #endif
    }
template <typename  TI,typename TE> void    JCurve::textbox(NVGcontext* avg,const TI &title,const TE &text) {
    float w=dwidth*0.6;
//     nvgRoundedRect(avg,  x,  y,  w,  h,  r);
//    x+=smallsize;
    nvgFontFaceId(avg,font);
    bounds_t bounds;
    nvgTextAlign(avg,NVG_ALIGN_LEFT|NVG_ALIGN_TOP);
    nvgFontSize(avg, smallsize);
    nvgTextLineHeight(avg, 1.7);
     nvgTextBoxBounds(avg, 0,  0, w,begin(text), end(text), bounds.array);
    nvgBeginPath(avg);
    float width= bounds.xmax-bounds.xmin+ smallsize;
    float height= bounds.ymax-bounds.ymin+sensorbounds.height*2;
    float x=(dwidth-width)/2;
    float y=(dheight-height)/2;
    nvgFillColor(avg, red);
    nvgRoundedRect(avg,  x-smallsize, y-smallsize,  width+2*smallsize, height+2*smallsize, dwidth/60 );
    nvgFill(avg);
    nvgFillColor(avg, *getblack());
    nvgTextBox(avg,  x,  y+sensorbounds.height+smallsize, width, begin(text),end(text));
    nvgFontSize(avg, mediumfont);

    nvgTextAlign(avg,NVG_ALIGN_LEFT|NVG_ALIGN_TOP);
   auto *beg=begin(title);
   if(*beg) {
      int siz=sizear(title);
      nvgText(avg, x,y, beg,beg+siz);
      }
    nvgTextAlign(avg,NVG_ALIGN_RIGHT|NVG_ALIGN_TOP);
    showOK(avg,x+width,y);
    }


#include "EverSense.hpp"
extern    bool hasnotiset();
void     processglucosevalue(int sendindex,int newstart) {
    if(settings) {
         if(!sensors)
             return;
         if(SensorGlucoseData *hist=sensors->getSensorData(sendindex)) {
             if(newstart>=0) {
               LOGGER("newstart=%d previous=%d\n",newstart,hist->previousstream);
               hist->backstream(newstart);
               if(newstart<=hist->previousstream) {
                   hist->previousstream=newstart;
                   sendEverSenseold(hist,5/hist->streaminterval());
                   return;
                   }
               if(hist->previousstream==-1) {
                   sendEverSenseold(hist,5/hist->streaminterval());
                   }
                hist->previousstream=newstart;
                }
             else
                  hist->previousstream=hist->pollcount()-1;
             if(const ScanData *poll=hist->lastpoll()) {
                 const time_t tim=poll->t;
                 if(!poll->valid()) {
                     LOGGER("invalid value %s ",ctime(&tim));
                     return;
                     }
                 const time_t nutime=time(nullptr);
                 const int dif=nutime-tim;
                 if(dif<maxbluetoothage) {
                     if(!usedsensors.size())
                         setusedsensors(nutime);
                    int32_t mgdL;
                    uint32_t mgL;
                    float glu;
                    if(double cali=calibrateNow(hist,*poll);isnan(cali)) {
                        mgdL=poll->getmgdL();
                        mgL=mgdL*10;
                        glu= gconvert(mgL);
                        }
                    else {
                        double mgLf= cali*10.0;
                        mgL=(uint32_t)round(mgLf);
                        mgdL=(int32_t)round(cali);
                        glu= gconvert(mgLf);
                        }

                     const int alarm=getalarmcode(mgL,poll->getchange(),hist);
                     
                     sensor *senso=sensors->getsensor(sendindex);
                     bool wasnoblue=settings->data()->nobluetooth;
                     int64_t startsensor=hist->getstarttime()*1000LL;
                     const intptr_t  sensorptr=dohealth(sendindex)?reinterpret_cast<intptr_t>(hist):0LL;
                     const int sensorgen=hist->getSensorgen2();
                     LOGGER("processglucosevalue finished=%d,doglucose(%s,%d,%f,%f,%d,%lld,%d,%lld,%p,%i)\n", senso->finished,hist->shortsensorname()->data(),mgdL,glu,poll->ch,alarm,tim*1000LL,wasnoblue,startsensor,sensorptr,sensorgen);
                     if(senso->finished) {
                         senso->finished=0;
                         backup->resensordata(sendindex);
                         }
                     settings->data()->nobluetooth=true;
                     float rate=poll->ch;
extern void telldoglucose(const char *name,int32_t mgdl,float glu,float rate,int alarm,int64_t mmsec,bool wasnoblue,int64_t startmsec,intptr_t sensorptr,int sensorgen);

                     telldoglucose(hist->shortsensorname()->data(),mgdL,glu,rate,alarm,tim*1000LL,wasnoblue,startsensor,sensorptr,sensorgen);

                 //    wakeuploader();
extern                void wakewithcurrent();
                     wakewithcurrent();

                     }
                 else {
                     LOGGER("processglucosevalue too old %s ",ctime(&tim));
                     LOGGER("dist=%d, dif=%d nu %s",maxbluetoothage,dif,ctime(&nutime));
                     }
                 }
             }
         else {
             LOGGER("processglucosevalue no sensor %d\n",sendindex);
#ifdef WEAROS
//              static int wassensor=(settings->setranges(3*180,12*180,39*18,10*180),-1);
              static int wassensor=-1;
             if(sendindex>wassensor) {
                 wassensor=sendindex;
                extern void setInitText(const char *message);
                constexpr const int maxbuf=50;
                char buf[maxbuf];
                const std::string_view sensor= usedtext->menustr0[1];
               memcpy(buf,sensor.data(),sensor.size());
                snprintf(buf+sensor.size(),maxbuf-sensor.size()," %d", wassensor+1);
                setInitText(buf);
                }
#endif
         }
        }

    }

static bool  inmenu(float x,float y) {
    if(!nrmenu)
        return false;
    if(x<menupos.left||x>=menupos.right) {
        return false;
        }
    float dist=(menupos.bottom-menupos.top)/nrmenu;
    if(dist<=0)
        return false;
    int item=(y-menupos.top)/dist;
    if(item>=0&&item<nrmenu) 
        return true;
    return false;
    }

 bool           JCurve::showerror(NVGcontext* avg,const string_view str1,const string_view str2) {
    startstep(avg,*getyellow());
    nvgFontSize(avg, midsize);
    nvgFillColor(avg, *getblack());
    nvgTextAlign(avg,NVG_ALIGN_LEFT|NVG_ALIGN_BOTTOM);
    nvgText(avg, dleft+dwidth/10,dtop+dheight/3, str1.begin(), str1.end());
    nvgFontSize(avg, midsize*.8);
    nvgTextAlign(avg,NVG_ALIGN_LEFT|NVG_ALIGN_MIDDLE);
    nvgTextBox( avg, dleft+dwidth/10, dtop+dheight/2, dwidth*8/10, str2.begin(), str2.end());

    if(settings->data()->speakmessagesget()) {
        char buf[str1.size()+str2.size()+2+10];
        memcpy(buf,str1.data(),str1.size());
        char *ptr=buf+str1.size();
        *ptr++='\n';
        memcpy(ptr,str2.data(),str2.size());
        ptr[str2.size()]='\0';
        LOGGER("speak %s\n",buf);
        speak(buf);

        }

    showok(avg,true,false);
    return true;
    }


extern char hourminstr[hourminstrlen];

int hourmin(const time_t tim,char buf[8]) {
   struct tm tmbuf;
   struct tm *stm=localtime_r(&tim,&tmbuf);
   return  mktime(stm->tm_hour,mktmmin(stm),buf);
   }
void setnowmenu(time_t nu) {
    int timelen=hourmin(nu,hourminstr);
    const int ulen=usedsensors.size();

    if(ulen>0) {
        
        for(int i=0;i<ulen;) {
            const auto *sens=sensors->getSensorData(usedsensors[i++]);
            if(const auto *lastin=sens->lastValidStream()) {
                for(;i<ulen;i++) {
                    const auto *nextsens=sensors->getSensorData(usedsensors[i]);
                    if(const auto *lastsen=nextsens->lastValidStream();lastsen&&(lastsen->t>lastin->t)) {
                        lastin=lastsen;
                        sens=nextsens;
                        }
                    }
                if(lastin->t>(nu-maxbluetoothage)) {
                    double nonconvert;
                    if(double cali=calibrateNow(sens,*lastin);!isnan(cali)) {
                        nonconvert=cali;
                        }
                    else {
                        nonconvert= lastin->g;
                    }
const int  trend=lastin->tr;
//const int  trend=5;

constexpr const char arrows[][sizeof("→")]{"",
"↓",
"↘",
"→",
"↗",
"↑"}; 

char *aftertime=hourminstr+timelen;
#if __NDK_MAJOR__ >= 26

constexpr const int trendoff=
#ifdef WEAROS
0
#else
1
#endif
;
constexpr const int between=
#ifdef WEAROS
    1;
#else
    2;
#endif
    char *ptr=aftertime+between;
   memset(aftertime,' ',between);
    const int trendlen=sizeof(arrows[trend])-1;
    memcpy(ptr,arrows[trend],trendlen);
    static std::to_chars_result res={.ptr=nullptr};
    char *oldres=res.ptr;
    auto value=gconvert(nonconvert*10);
   if constexpr (trendoff) memset(ptr+trendlen,' ',trendoff);
    res=std::to_chars(ptr+trendlen+trendoff,hourminstr+hourminstrlen,value,std::chars_format::fixed,gludecimal);
    for(auto it=res.ptr;it<oldres;++it)
        *it=' ';
                    LOGGER("new hourminstr=%s\n", hourminstr);
#else
    static        int oldend=0;
//    auto aftertime=hourminstr+;
#ifdef WEAROS
                int endpos=snprintf(aftertime,hourminstrlen-5," %s%.*f",arrows[trend],gludecimal,gconvert(nonconvert*10));
#else
            int endpos=snprintf(aftertime,hourminstrlen-5,"  %s %.*f",arrows[trend],gludecimal,gconvert(nonconvert*10));
#endif
    aftertime[endpos]=' ';
    for(int i=endpos+1;i<oldend;i++)
        aftertime[i]=' ';
    
    oldend=endpos;

                    LOGGER("old hourminstr=%s\n", hourminstr);
#endif
                    return ;

                    }
                }
            }
        }
    memset(hourminstr+timelen,' ',hourminstrlen-timelen); 
    }


void  calccurvegegs();
void resetcurvestate() {
    LOGAR("resetcurvestate()");
#if defined(JUGGLUCO_APP) && defined(USE_DISPLAYER)
    displayer.reset();
#endif
    scantoshow={-1,nullptr}; 
    numlist=0;
    #ifndef WEAROS
    showpers=false;
    #endif
    selshown=false;
    selmenu=0;
    emptytap=false;
    nrmenu=0;
    selmenu=0;
    appcurve.calccurvegegs();
    appcurve.setdiffcurrent(settings->data()->currentRelative);
    }

void    initopengl(int started)  {
    if(!started) {
        resetcurvestate();
        }
    if(::genVG) { //Why should it be recreated?
#ifdef NANOVG_GLES2_IMPLEMENTATION
 nvgDeleteGLES2
#else 
 nvgDeleteGLES3
#endif
    (::genVG); 
    ::genVG=nullptr;
    }


    printGlString("Version", GL_VERSION);
    printGlString("Vendor", GL_VENDOR);
    printGlString("Renderer", GL_RENDERER);
    printGlString("Extensions", GL_EXTENSIONS);

   decltype(::genVG)    avg = 

#ifdef NANOVG_GLES2_IMPLEMENTATION
    nvgCreateGLES2
#else
    nvgCreateGLES3
#endif


    (NVG_ANTIALIAS | NVG_STENCIL_STROKES
#ifndef NDEBUG
    | NVG_DEBUG
#endif

    );

    if (avg == nullptr) {
        LOGSTRING("Could not init nanovg.");
        return ;
        }
    ::genVG=avg;
     }

bool openglstarted=false;
/*extern "C" int nvgRecreateGLES2(NVGcontext* ctx) ;
void initopengl(float small,float menu,float density,float headin) {
    LOGAR("initopengl");
    if(!openglstarted||genVG==nullptr||!nvgRecreateGLES2(genVG)) {
        initopengl(openglstarted);
        if(!openglstarted||appcurve.headsize!=headin||appcurve.smallsize!=small||appcurve.menusize!=menu||appcurve.density!=density) {
            appcurve.setfontsize(small, menu, density, headin);
            }
        appcurve.invertcolorsset(settings->data()->invertcolors);
        openglstarted=true;
        appcurve.initfont(::genVG);    
        }
     else {
        if(!openglstarted||appcurve.headsize!=headin||appcurve.smallsize!=small||appcurve.menusize!=menu||appcurve.density!=density) {
            appcurve.setfontsize(small, menu, density, headin);
            }
//        nvgFontFaceId(genVG,appcurve.font);
        LOGAR("nvgRecreateGLES2 succeeded!!");
        }
   } */

void initopengl(float small,float menu,float density,float headin) {
    LOGAR("initopengl");
    initopengl(openglstarted);
    if(!openglstarted||appcurve.headsize!=headin||appcurve.smallsize!=small||appcurve.menusize!=menu||appcurve.density!=density) {
            appcurve.setfontsize(small, menu, density, headin);
            }
    appcurve.invertcolorsset(settings->data()->invertcolors);
//    s/makeshows(\(.*\))/appcurve.show\1=settings->data()->show\1;/g
    appcurve.showcalibrated=settings->data()->showcalibrated;
    appcurve.showscans=settings->data()->showscans;
    appcurve.showstream=settings->data()->showstream;;
    appcurve.showmeals=settings->data()->showmeals;
    appcurve.shownumbers=settings->data()->shownumbers;
    appcurve.showhistories=settings->data()->showhistories;
    openglstarted=true;
    appcurve.initfont(::genVG);    
   }
#define arsizer(x) sizeof(x)/sizeof(x[0])

#ifdef WEAROS

const int *menuopt0[]={nullptr,nullptr,nullptr, nullptr,nullptr};
const int **optionsmenu[]={menuopt0,nullptr};
constexpr const int menulen[]={arsizer(jugglucotext::menustr0),arsizer(jugglucotext::menustr2)};
int getmenulen(const int menu) {
    int len=menulen[menu];
//    if(menu==1&&settings->staticnum()) return len-1;
    if(!menu&&!alarmongoing)
        return len-1;
        
    return len;    
    }

void setfloatptr() {
    }
#else
int menus=0;
const int *menuopt0[]={&showui,&menus,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr};


const int *menuopt0b[]={nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr};
const int *menuopt1[]={&appcurve.showcalibrated,&appcurve.showscans,&appcurve.showstream,&appcurve.showhistories,&appcurve.shownumbers,&appcurve.showmeals,&appcurve.invertcolors};
const int **optionsmenu[]={menuopt0,menuopt0b,menuopt1,nullptr};
#define arsizer(x) sizeof(x)/sizeof(x[0])
constexpr const int menulen[]={arsizer(jugglucotext::menustr0),arsizer(jugglucotext::menustr1),arsizer(jugglucotext::menustr2),arsizer(jugglucotext::menustr3)};
int getmenulen(const int menu) {
    int len=menulen[menu];
    if(!menu&&!alarmongoing)
        return len-1;
    return len;    
    }
void setfloatptr() {
    menuopt0b[6]=&settings->data()->floatglucose;
    }

//void setnewamount() { }
#endif


constexpr const int maxmenulen= *std::max_element(std::begin(menulen),std::end(menulen));
constexpr int maxmenu=arsizer(jugglucotext::menustr);
#ifdef WEAROS
static_assert(maxmenu==2);
#else
static_assert(maxmenu==4);
#endif


int getmenu(int tapx,float dwidth) {
    return tapx*maxmenu/dwidth;
    }
 void    JCurve::withredisplay(NVGcontext* avg,uint32_t nu)  {
    startstep(avg,*getwhite());
#ifndef WEAROS
    if(showpers) {
        showpercentiles(avg);
        }
    else 
#endif
    {

#ifdef WEAROSx
int oldtapx=tapx;
tapx=-8000;
#endif

        if( !displaycurve(avg,nu)&&( ((tapx
#ifdef WEAROSx

        =oldtapx
#endif
                )>=0&&!selshown&&(selmenu=getmenu(tapx,dwidth),true))||nrmenu)) {
          showtext( avg ,nu,selmenu) ;
           }
        }
    tapx=-8000;


LOGAR("end withredisplay");
}



#include "secs.h"
void JCurve::prevdays(int nr) {
    //starttime=starttimefromtime(starttime-nr*daysecs);
    setstarttime(starttime-nr*daysecs);
    auto minstart= minstarttime();
    if(starttime<minstart)
        setstarttime(minstart);
    }
void JCurve::nextdays(int nr) {
    //starttime=starttimefromtime(starttime+daysecs*nr);
    setstarttime(starttime+daysecs*nr);
#ifndef WEAROS
    if(!showpers) 
#endif
    {
        auto maxstart= maxstarttime(); 
        if(starttime>maxstart) 
            setstarttime(maxstart);
        }
    }

//int notify=false;

#ifdef USE_DISPLAYER
bool  showtextbox(JCurve *j,NVGcontext* avg) {
    if(displayer) {
        j->textbox(avg,"",displayer->text);
        return true;
        }
     return false;
     }
#endif
#ifndef DONTTALK
static bool speakmenutap(float x,float y) {
    if(x<menupos.left||x>=menupos.right) {
        return false;
        }
    float dist=(menupos.bottom-menupos.top)/nrmenu;
    int item=(y-menupos.top)/dist;
    if(item>=0&&item<nrmenu) {
        LOGGER("menuitem selmenu=%d item=%d\n",selmenu,item);
        auto options=optionsmenu[selmenu];
        const auto label=usedtext->menustr[selmenu][item];
        if(!options||!options[item]) 
            speak(label.data());
        else {
            constexpr const int maxbuf=256;
            char buf[maxbuf];
            memcpy(buf,label.data(),label.size());
            char *ptr=buf+label.size();
            *ptr++= '\n';
            if(*options[item])
                strcpy(ptr,usedtext->checked);
            else
                strcpy(ptr,usedtext->unchecked);
            speak(buf);
            }
        return true;
        }
    return false;    
    }
#endif

 void    JCurve::showtext(NVGcontext* avg ,time_t nu,int menu) {
LOGAR("showtext");
#ifdef WEAROS
    if(menu==1) {
        setnowmenu(nu);
//        setnewamount();
        }
#else
    if(menu==3)
        setnowmenu(nu);
//    if(menu==1) setnewamount();
#endif
    const string_view *menuitem=usedtext->menustr[menu];
    nrmenu=getmenulen(menu);
    constexpr const float randsize=
    #ifdef WEAROS
    10
    #else
    16
    #endif
    ;
     float xrand=randsize*density;
     float yrand=randsize*density;
//    float menutextheight=density*48;
    float menuplace= dwidth/ maxmenu;
    float x=xrand+menu*menuplace,starty=yrand+statusbarheight,y=starty;

    nvgTextAlign(avg,NVG_ALIGN_LEFT|NVG_ALIGN_TOP);

    bounds_t bounds;

    nvgFontFaceId(avg,menufont);
    nvgFontSize(avg, menusize);
     nvgTextBounds(avg, x,  y, menuitem[0].data(),menuitem[0].data()+menuitem[0].size(), bounds.array);
//    nvgText(avg, x,y, menuitem[0].data(), menuitem[0].data()+menuitem[0].size());
     float maxx=bounds.xmax;
     float maxwidth=bounds.xmax-bounds.xmin;
     for(int i=1;i<nrmenu;i++) {
        y+=menutextheight;
         nvgTextBounds(avg, x,  y, menuitem[i].data(),menuitem[i].data()+menuitem[i].size(), bounds.array);
         if(maxx<bounds.xmax)
            maxx=bounds.xmax;
         float maxwidthone=bounds.xmax-bounds.xmin;
         if(maxwidthone>maxwidth)
             maxwidth=maxwidthone;
        }
    float height=y+bounds.ymax-bounds.ymin;
    nvgBeginPath(avg);
     nvgFillColor(avg, *getmenucolor());
//     nvgFillColor(avg, white);
     float mwidth=maxx-x+2*xrand;
//     float minmenu=128*density;
     float minmenu=
#ifdef WEAROS
     80
#else
    128
#endif

             *density;
     float maxmenu=280*density;
     if(mwidth<minmenu)
         mwidth=minmenu;
    else
        if(mwidth>maxmenu)
            mwidth=maxmenu;
     x+=(menuplace-mwidth)/2;
     #ifdef WEAROS
     if(menu==0)
         x+=xrand;
     #endif
     menupos={ x-xrand, starty-yrand,x-xrand+ mwidth, height+yrand};
     logmenupos();
     nvgRect(avg, x-xrand, starty-yrand, mwidth, height-starty+2*yrand);
    nvgFill(avg);
#ifdef WEAROS
if(menu==0) {
    nvgTextAlign(avg,NVG_ALIGN_RIGHT|NVG_ALIGN_TOP);
    x+=maxwidth;
    }
#endif
    y=starty;
//     nvgFillColor(avg, *getwhite());
     nvgFillColor(avg, *getmenuforegroundcolor());
//     nvgFillColor(avg, black);
     for(int i=0;i<nrmenu;i++) {
        nvgText(avg, x,y, menuitem[i].data(), menuitem[i].data()+menuitem[i].size());
        y+=menutextheight;
        }

    if(const int **options=optionsmenu[menu]) {
        y=starty;
        const char set[]="[x]";
        const char unset[]="[ ]";
        constexpr int len=3;
        float xpos;
#ifdef WEAROS
        if(menu==0) {
            xpos=x;
            }
        else 
#endif
        {
        static const float  dlen=getsetlen(avg, x,  y, set,set+len, bounds);
         xpos=x-2*xrand+mwidth-dlen;
         }
         for(int i=0;i<nrmenu;i++) {
             if(const int *optr=options[i]) {
                const char *op=*optr?set:unset;
                nvgText(avg, xpos ,y,op ,op+len );
                }
            y+=menutextheight;
            }
        }

    LOGAR("end showtext");
    }

void JCurve::withbottom() {
    extern const int maxmenulen;
    dheight=height-dtop-dbottom;
    const float maxmenu= (float)dheight/maxmenulen;
    if(menutextheight>maxmenu)
        menutextheight=maxmenu;
    LOGGER("menutextheight=%f\n", menutextheight);
    }
void JCurve::resizescreen(int widthin, int heightin,int initscreenwidth) {
    width=widthin;
    height=heightin;
    LOGGER("resize(%d,%d)\n",width,height);
    dwidth=width-dleft-dright; //Display area for graph in pixels

    textheight=density*48;
    int times=ceil(height/textheight);
    textheight=height/times;
    menutextheight=density*48;

    withbottom();


    float facetimelen=2.0f*dwidth/3.0f;
    LOGGER("facetimelen=%.1f\n",facetimelen);
    facetimefontsize=smallsize*facetimelen/timelen;
    LOGGER("facetimefontsize=%.1f\n",facetimefontsize);
    float straal=dwidth*0.5f;
    facetimey=(straal-sqrt(pow(straal,2.0)-pow(facetimelen*.5,2.0)))*.70;

    LOGGER("facetimey=%.1f\n",facetimey);
}

int getglucosestr(double nonconvert,char *glucosestr,int maxglucosestr,int glucosehighest) {
    if(nonconvert<glucoselowest) {
        return appcurve.mkshowlow(glucosestr, maxglucosestr) ;
        }
    else {
        if(nonconvert> glucosehighest) {
            return appcurve.mkshowhigh(glucosestr, maxglucosestr,glucosehighest) ;
            }
        else {
            const float convglucose= gconvert(nonconvert*10);
            return snprintf(glucosestr,maxglucosestr,gformat,convglucose);
            }
        }
    }


 void       JCurve::showscanner(NVGcontext* avg,const SensorGlucoseData *hist,int scanident,time_t nu) {
    const ScanData &last=*hist->getscan(scanident);
    const bool isold=(nu-last.t)>=maxbluetoothage;
    startstep(avg,isold?getoldcolor():*getwhite());
   float right=dleft+dwidth-statusbarright;
    float x=right; 
    constexpr int maxbuf=50;
    char buf[maxbuf*2];
    time_t tim=last.t;
    struct tm tmbuf;
     struct tm *tms=localtime_r(&tim,&tmbuf);
//    int len=snprintf(buf,maxbuf,"%02d:%02d ", tms->tm_hour,mktmmin(tms));
   int len=mktime(tms->tm_hour,mktmmin(tms),buf);
   buf[len++]=' ';
    char *buf1=buf+len;
    --len;
    const int32_t gluval=last.g;
    int len1;
    float endtime=x,sensleft=0.0f;

    if(gluval<glucoselowest) {
        len1=mkshowlow(buf1,maxbuf);
        endtime-=smallerlen;
        sensleft=smallerlen;
        }
    else {
        int glucosehighest=hist->getmaxmgdL();
        if(gluval>glucosehighest) {
            len1=mkshowhigh(buf1,maxbuf,glucosehighest);
            endtime-=smallerlen;
            }
        else {
            len1=snprintf(buf1,maxbuf,gformat, gconvert(gluval*10.0f));
            }
        }

    float bounds[4];
    nvgTextAlign(avg,NVG_ALIGN_RIGHT|NVG_ALIGN_MIDDLE);
    nvgFontSize(avg, headsize);
    nvgTextBounds(avg, x,dtop+dheight/2 , buf1, buf1+len1, bounds);
    nvgFillColor(avg, *getblack());
    auto [first,y]=drawtrender( avg,hist->gettrendsbuf(scanident),dleft,dtop,bounds[0]-dleft,dheight);
    float th=(bounds[3]-bounds[1])/2.0;
    if(y<th) 
        y=th;
    else
        if((dheight-(y-dtop))<th)
            y=dheight-th;

    nvgText(avg, x,y, buf1, buf1+len1);
    const bool showabove=y>(dheight/2);
    const float yunder=y+(showabove?-1:1)*headsize/2.0;
//    nvgFontSize(avg,smallsize );
    nvgFontSize(avg,headsize*.134f );
    nvgText(avg, endtime,yunder, buf, buf+len);
    const auto sensorname=hist->othershortsensorname();
    nvgTextAlign(avg,NVG_ALIGN_LEFT|NVG_ALIGN_MIDDLE);
    const auto sensorx=bounds[0] -sensleft;
    nvgText(avg,sensorx,yunder, sensorname.begin(), sensorname.end());
    const bool showdate=(nu-last.t)>=60*60*12;
    constexpr const int maxdatebuf=30;
    int datelen;
    char datebuf[maxdatebuf];
    if(isold) {
        if(showdate) {
            float datey=yunder+(showabove?-1:1)*sensorbounds.height;
            nvgTextAlign(avg,NVG_ALIGN_RIGHT|NVG_ALIGN_MIDDLE);
            datelen=snprintf(datebuf,maxdatebuf,"%s %d %s %04d",usedtext->speakdaylabel[tmbuf.tm_wday],tmbuf.tm_mday,usedtext->monthlabel[tmbuf.tm_mon],1900+tmbuf.tm_year);
            nvgText(avg,right,datey, datebuf, datebuf+datelen);
            }
        nvgStrokeWidth(avg, TrendStrokeWidth);
        nvgStrokeColor(avg, *getwhite());
        nvgBeginPath(avg);
         nvgMoveTo(avg,dleft ,dtop) ;
        nvgLineTo( avg, right,dheight);
        nvgStroke(avg);
        nvgBeginPath(avg);
         nvgMoveTo(avg,right ,dtop) ;
        nvgLineTo( avg, dleft,dheight);
        nvgStroke(avg);
        }
#ifndef DONTTALK

    if(settings->data()->speakmessagesget()) {
        char value[300];
        char *ptr=value;;
        if(isold) {
            if(showdate) {
                memcpy(ptr,datebuf,datelen);
                ptr+=datelen;
                *ptr++='\n';
                *ptr++='\n';
                }
            memcpy(ptr,buf,len);
            ptr+=len;
            *ptr++='\n';
            }
//        auto trend=usedtext->trends[last.tr];
        const auto trend=usedtext->getTrendName(last.tr);
        memcpy(ptr,trend.data(),trend.size());
        ptr+=trend.size();
        *ptr++='\n';
        *ptr++='\n';
        memcpy(ptr,buf1,len1+1);
        speak(value);
        }
#endif

    showok( avg,(last.g>70&&last.g<=140), ((y-dtop)<(dheight/2))?false:true);
    }


 int    JCurve::showerrorvalue(NVGcontext* avg,const SensorGlucoseData *sens,const time_t nu,float getx,float gety,int index) {
 
    shownglucose[index].glucosevalue=0;
    getx-=headsize/3;
    shownglucose[index].glucosevaluex=getx;
    shownglucose[index].glucosevaluey=gety+headsize*.5;
    nvgTextAlign(avg,NVG_ALIGN_LEFT|NVG_ALIGN_MIDDLE);
    nvgFontSize(avg,headsize/6 );
    if(settings->data()->nobluetooth) {
        LOGAR("nobluetooth");
        if(hasnetwork()) {
            return 1;
            }
        else {
            return 2;
            }
        }
    else {
        LOGAR("!nobluetooth");
        {
            if(!bluetoothEnabled()) {
                LOGAR("bluetooth not Enabled");
                return 3;
                } 
            else {
               if((nu-sens->receivehistory)<60) {
                        static char buf[256];
                        const auto past=usedtext->receivingpastvalues;
                        memcpy(buf,past.data(),past.size());
                        char *ptr=buf+past.size();
                        ptr+=sprintf(ptr,": %d",sens->pollcount());
                        nvgTextBox(avg,  getx, gety, getboxwidth(getx),buf, ptr);
                        shownglucose[index].errortext=buf;
                }
            else {
 
                if(sens->hasSensorError(nu)) {
                    const std::string_view sensorerror= sens->replacesensor?usedtext->streplacesensor: usedtext->stsensorerror;
                    char buf[sensorerror.size()+17];
                    int senslen= sens->showsensorname().size();
                    memcpy(buf,sens->showsensorname().data(),senslen);
                    memcpy(buf+senslen,sensorerror.data(), sensorerror.size());
                    auto boxwidth= getboxwidth(getx);
                    nvgTextBox(avg,  getx, gety, boxwidth, buf, buf+sensorerror.size()+senslen);
                    shownglucose[index].errortext=sensorerror.data();
                    }
                else {
                   int state=sens->getinfo()->patchState;
                    if(state&&state!=4){
                        const auto format= state>4?usedtext->endedformat:usedtext->notreadyformat;
                        static char buf[256];
                        int len=snprintf(buf,sizeof(buf)-1,format.data(),sens->showsensorname().data(),state);
                        nvgTextBox(avg,  getx, gety, getboxwidth(getx),buf, buf+len);
                        shownglucose[index].errortext=buf;
                        } 
                    else {
                        char buf[usedtext->noconnectionerror.size()+17];
                        int senslen= sens->showsensorname().size();
                        memcpy(buf,sens->showsensorname().data(),senslen);
                        memcpy(buf+senslen,usedtext->noconnectionerror.data(), usedtext->noconnectionerror.size());
                        nvgTextBox(avg,  getx, gety, getboxwidth(getx),buf, buf+usedtext->noconnectionerror.size()+senslen);
                        shownglucose[index].errortext=usedtext->noconnectionerror.data();
                        }
                    }
               }
                return 0;
                }
            }
    
        }
    }


void    JCurve::startstep(NVGcontext* avg,const NVGcolor &col) {
        glViewport(0, 0, width, height);
        glClearColor(col.r,col.g, col.b, col.a);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        startstepNVG(avg,width,height);
        }

void    JCurve::endstep(NVGcontext* avg) {
    nvgEndFrame(avg);
    glEnable(GL_DEPTH_TEST);
}

time_t lastviewtime=0;
int    JCurve::onestep(NVGcontext* avg) {
    LOGAR("onestep");
    time_t nu=time(nullptr);
    lastviewtime=nu;
    updateusedsensors(nu);

    selshown=false;
    int ret=0;
    emptytap=false;
#ifdef JUGGLUCO_APP
    if(showoldscan(avg,nu)>0) {
        ret=1;
        }
    else
#endif
    {

#if defined(JUGGLUCO_APP)&&!defined(WEAROS)
        if(numlist) {
            shownumlist(avg);
            }
        else
#endif

        {
            withredisplay(avg,nu);
        }
    }
#if defined(JUGGLUCO_APP)&&defined(USE_DISPLAYER)
extern bool  showtextbox(JCurve *,NVGcontext* avg) ;
    if(showtextbox(this,avg))
        ret=1;
#endif
    endstep(avg);    
    return ret;
    }

AppCurve appcurve;
