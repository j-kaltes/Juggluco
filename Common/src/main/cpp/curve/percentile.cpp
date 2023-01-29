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
#include "SensorGlucoseData.h"
#include "sensoren.h"
#include "curve.h"
#include "bino.h"
#include "jugglucotext.h"
extern Sensoren *sensors;
static	constexpr const int seconds_in_day=24*60*60;
extern uint32_t starttime;
extern int duration;
extern bool showpers;
bool showpers=false;
	extern uint32_t setend;

extern float timelen;

extern void showlines(int gm,int gmax) ;

uint32_t pollstart,polllast;


void datainterval(NVGcontext* vg,float x, float y,uint32_t start,uint32_t end) {
		constexpr const int  maxbuf=100;
		char buf[maxbuf];
		int len=datestr(start,buf);
		const char tus[]=" - ";
		constexpr const int tuslen=sizeof(tus)-1;
		memcpy(buf+len,tus,tuslen);
		len+=tuslen;
		len+=datestr(end,buf+len);
		nvgText(vg, x,y,buf,buf+len);
		}

static constexpr const int measuresperday=24*60;
uint32_t startday,endday;
static const ScanData *firstvalid(const ScanData *start,const ScanData *last,uint32_t nexttime) {
	while(!start->valid()||start->gettime()<nexttime) {
		++start;
		if(start>=last)
			return nullptr;
		}
	return start;
	}
static const ScanData *lastvalid(const ScanData *last)  {
	while(!last->valid())
		--last;
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
public:
persgegs & operator= (const persgegs &) = delete;
persgegs & operator= ( persgegs &&) = delete;
persgegs( persgegs & ) = delete;	
persgegs( persgegs && ) = delete;	
// persgegs() {}
 persgegs(uint16_t *dat,int *l,int days,int nr):data(dat),lens(l),days(days),idnr(nr) {}
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
	logprint("compare:\n");
	for(int i=0;i<idnr;i++) 
		if(ar1[i]>ar2[i]) {
			logprint("%d: %d>%d\n",i,ar1[i],ar2[i]);
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
//		return (id+minutes)%idnr;
		}

uint16_t	*mkpercentile(const float frac) const { 
		uint16_t*ar	=new uint16_t[idnr];
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
	void	mkmax()  {
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
	int isplace(const int start,const int end,const float move) const {	
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
//	 savegegs(); saverounds();

//	compare(f10.second,f0.second);
	}
template <typename  T>
void showpercentile(NVGcontext* vg,T frac,const int startid,const int endid) {
	if(showforwardpercentile(vg,frac,startid,endid))
		nvgStroke(vg);
	};
const float getidstep(const int showids) const {
	return ((float)dwidth)/(showids-1);
	}
inline constexpr const static int maxids=60*24;
int reindex(const int index) const {
//	return (index+maxids)%maxids;
	return index%maxids;
	}
bool showforwardpercentile(NVGcontext* vg,const uint16_t *perar,const int startid,const int endid) const {
	int showids=endid-startid;
	const float move=getidstep(showids);
	int ma=getmax();
	int mi=getmin();
	int hi=ma-mi;
	float unit=((float)dheight)/hi;
//	LOGGER("max=%d, min=%d, ver=%d,unit=%.1f\n",ma,mi,hi,unit);
	auto y=[unit,mi](int gl) {
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
//	LOGGER("(%d,%d) ",x1,y1);
	nvgMoveTo(vg, x1,y1);
	for(;i<endid;i++) {
		if(int glu= perar[reindex(i)]) {
			int  x1= x(i);
			int y1= y(glu);
	//		logprint("%d\t%d (%.1f)\n",x1,y1,glu/180.0);
			nvgLineTo( vg,x1,y1);
			}
		}
	return true;
	}
void showbackpercentile(NVGcontext* vg,const uint16_t *ar,int startid,int endid) const {
	int showids=endid-startid;
	const float move=getidstep(showids);
	int ma=getmax();
	int mi=getmin();
	int hi=ma-mi;
	float unit=((float)dheight)/hi;
	auto y=[unit,mi](int gl) {
		return dheight-(gl-mi)*unit;
		};
	auto x=[move,startid](int pos) {
		return (pos-startid)*move;
		};
	for(int i=endid-1;i>=startid;i--) {
		if(int glu=ar[reindex(i)]) {
			int  x1= x(i);
			int y1= y(glu);
		//	logprint("%d\t%d (%.1f)\n",x1,y1,glu/180.f);
			nvgLineTo( vg,x1,y1);
			}
		}
	}
	
void showround(NVGcontext* vg,roundtype dat,const int startid,const int endid) const {
		if(showforwardpercentile(vg,dat.first, startid, endid)) {
			showbackpercentile(vg,dat.second,startid,endid);
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
//static		constexpr const NVGcolor lightblue4={{{0.2f, 0.2f,1.0f,0.25f}}};
//static constexpr auto midlight=hexcolor(0x1E90FF);
static constexpr auto midlight=hexcolor(0x00BFFF);
//static constexpr auto midblue=hexcolor(0x0000FF);
static constexpr auto midblue=hexcolor(0x7B68EE);
//auto darkest=hexcolor(0x00008B);
void showpercentiles(NVGcontext* vg) {

	extern uint32_t settime;
		extern int gmin;
		extern  int grange;
	if((setend<starttime||settime>=(starttime+duration))) {
		defaultextreme();
		gmin=min;
		grange=max-min;
		}
	else  {
		setextreme(gmin,gmin+grange);
		}

		if(duration>seconds_in_day)
		   duration=seconds_in_day;
		if(starttime<startday) {
			starttime=startday;
			}
		else {
			const uint32_t laststart=endday-duration+30*60;
			if(starttime>laststart)
				starttime=laststart;
			}
		//int startid=globalperc.gettimemin(starttijd);
		uint32_t starter=starttime-startday;
		auto &globalperc=*this;
		const int startid=globalperc.effectmin(starter/60);
		const int showids=globalperc.effectmin(duration/60);
		const int endid=startid+showids;
		LOGGER("startid=%d, endid=%d, showids=%d\n",startid,endid,showids);
	//	const float move=((float)dwidth)/(showids-1);
//		const float move=((float)dwidth)/showids;
		const float move=getidstep(showids);
		int pos=isplace(startid,std::min(endid,maxids),move);

//static		constexpr const NVGcolor lighterblue={{{0.2f, 0.2f,1.0f,0.07f}}};
		nvgFillColor(vg, lighttest);
		showround(vg,f0,startid,endid);
		float startpos=move*pos;;	
		float ystart=smallfontlineheight*3,afm=10*density;;
		float ypos=ystart+2* smallfontlineheight;
		nvgBeginPath(vg);
		nvgRect(vg, startpos, ypos,afm , afm);
		nvgFill(vg);

//static		constexpr const NVGcolor lightblue4={{{0.2f, 0.2f,1.0f,0.25f}}};
		nvgFillColor(vg, midlight);
		showround(vg,f10,startid,endid);
		ypos+=smallfontlineheight;
		nvgBeginPath(vg);
		nvgRect(vg, startpos, ypos,afm , afm);
		nvgFill(vg);
		nvgFillColor(vg, midblue);
		showround(vg,f25,startid,endid);
		ypos+=smallfontlineheight;
		nvgBeginPath(vg);
		nvgRect(vg, startpos, ypos,afm , afm);
		nvgFill(vg);
		nvgStrokeColor(vg, *getblack());
		extern float pollCurveStrokeWidth;
		nvgStrokeWidth(vg, pollCurveStrokeWidth); 
		globalperc.showpercentile(vg,m50,startid,endid);
		float linepos=ystart;
		nvgBeginPath(vg);
		nvgMoveTo(vg, startpos,linepos);
		nvgLineTo( vg,startpos+afm,linepos);
		nvgStroke(vg);

//		const int gmin=globalperc.getmin();
		const int gmax=globalperc.getmax();
		nvgFillColor(vg, *getblack());
		nvgFontSize(vg, smallsize);
		showlines(gmin,gmax);
		nvgTextAlign(vg,NVG_ALIGN_LEFT|NVG_ALIGN_TOP);
		const float datehigh=smallfontlineheight*1.2;
		datainterval(vg,density, datehigh,pollstart,polllast);
		nvgTextAlign(vg,NVG_ALIGN_RIGHT|NVG_ALIGN_TOP);
		ypos=ystart;
//		const char median[]="Median";
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
extern void showpercentiles(NVGcontext* vg) ;



#include "stats.h"
std::unique_ptr<struct stats> statptr;
extern void showstats(NVGcontext* vg,stats *stat) ;

bool showsummarygraph=false;
void showpercentiles(NVGcontext* vg) {
	struct stats *st=statptr.get();
	if(st) {
		if(showsummarygraph&&globalpercptr) 
			globalpercptr->showpercentiles(vg); 
		else
			showstats(vg,st);
		}
	}
//constexpr const int maxdays=50;
//uint16_t alldata[measuresperday*maxdays];

static struct persgegs *matchedminutes( std::vector<pair<const ScanData*,const ScanData*>> *polldataptr,uint32_t starttime,uint32_t endtime) {
	std::vector<pair<const ScanData*,const ScanData*>> &polldata=*polldataptr;
	const int days=ceilf(((float)(endtime-starttime))/seconds_in_day)+10;
//	uint16_t  *uitdata=alldata;
	uint16_t  *uitdata=new uint16_t[measuresperday*days]();
	const constexpr int bucketsize=60;
	const constexpr uint32_t maxidents= seconds_in_day/bucketsize;
	int  *lens=new int[measuresperday]();
	int prevsaidid=-1;
	int prevglucose=0;
	uint32_t prevtime=starttime,nexttime=0;
	pollstart=UINT_MAX;
	polllast=0;
        const auto timetoid=[](time_t tim) {
	    return  getminutes(tim)%maxidents; 
            };
	int maxid=0;
	int previd=-1;
	for(auto [firstin,lastin]:polldata) {
		const ScanData *start=firstvalid(firstin,lastin,nexttime);
		if(!start)
			continue;
		const ScanData *last=lastvalid(lastin-1);
		const ScanData *it=start;
		if(start->gettime()<pollstart)
			pollstart=start->gettime();
		if(last->gettime()>polllast)
			polllast=last->gettime();

		prevsaidid=-1;
		for(;it<=last;it++) {
			if(!it->valid()) {
				LOGGER("invalid\n");
				continue;
				}
			const int saidid=it->getid();
			if(saidid==prevsaidid) {
				logprint("dub %d==%d\n",saidid,prevsaidid);
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
						logprint("tusglu=%d\n",tusglu);
						uitdata[tusid*days+lens[tusid]++]=tusglu;
						}
					}
				} 
	//		previd=id;
			prevtime=ittime;
			prevglucose=glu;
			uitdata[id*days+lens[id]++]=glu;
			if(id>maxid)
				maxid=id;
			}
		nexttime=prevtime+30;
		}
	int count= std::reduce( lens, lens+ ++maxid);
	if(count<11000)
		return nullptr;
	LOGGER("end matchedminutes\n");
	return new persgegs(uitdata,lens,days,maxid);
	}

static struct persgegs * sortedmatched( std::vector<pair<const ScanData*,const ScanData*>> *polldataptr,const uint32_t start,const uint32_t endt) {
	persgegs *datastructptr=matchedminutes(polldataptr,start,endt) ;
	if(!datastructptr)	
		return nullptr;
	persgegs &datastruct=*datastructptr;
	int lastid=-1;
	for(int i=0;i<datastruct.idnr;i++) {
		logprint("%d ",datastruct.lens[i]);
		if(datastruct.lens[i]>1) {
			uint16_t*ar=datastruct.data+i*datastruct.days;
			std::sort(ar,ar+datastruct.lens[i]);
			lastid=i;
			}
		};
	logprint("\n");
	datastruct.idnr=lastid+1;
	datastruct.mkfracs();
	datastruct.mkextreme();

	LOGGER("end sortedmatched\n");
	return datastructptr;
	}


static int daystoanalyse=20;

extern std::vector<pair<const ScanData*,const ScanData*>> getsensorranges(uint32_t start,uint32_t endt) ;

extern void visiblebutton();
void makesummarygraph(std::vector<pair<const ScanData*,const ScanData*>> *polldataptr,uint32_t start,uint32_t endt) {
	if(globalpercptr!=nullptr)
		delete globalpercptr;
	globalpercptr=nullptr;
	if(!(globalpercptr=sortedmatched(polldataptr,start,endt)))
		return;
	startday=::starttime-getminutes(::starttime)*60;
	endday=startday+seconds_in_day;
	LOGGER("end makesummarygraph\n");
	setend=0;
	visiblebutton() ;
	return;
	}
std::vector<pair<const ScanData*,const ScanData*>> polldata;
static uint32_t statisticsendtime;
bool mkpercentiles(int days) {
	uint32_t endt=statisticsendtime;
	 showsummarygraph=false;
	auto firsttime=sensors->firstpolltime();
	if(!firsttime)
		return false;
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
	uint32_t start=	endt-days*seconds_in_day;
	polldata.clear();
      	polldata=getsensorranges(start,endt);
	if(polldata.size()<=0)
		return false;
     	std::thread graph(makesummarygraph,&polldata, start, endt);
	graph.detach();
	statptr=std::make_unique<struct stats>(polldata);
	showpers=true;
	return true;
	}


extern bool makepercetages() ;
bool makepercetages() {
       statisticsendtime=starttime+duration;
	return mkpercentiles(daystoanalyse);
	}
#include "settings/settings.h"
void leginterval(NVGcontext* vg,const float x,const float y, const int *between) {
	const constexpr int maxbuf=10;
	char buf[maxbuf];
	int buflen=snprintf(buf,maxbuf,"%.*f-%.*f",gludecimal,gconvert((between[1]+1)*10),gludecimal,gconvert((between[0])*10));
	nvgText(vg, x,y,buf,buf+buflen);
	}
void stats::showbar(NVGcontext* vg) {
	auto stat=this;
	float rowheight=smallfontlineheight*1.5;
	float useh=dheight*.8f,starty=(dheight-useh)/2.0f,startx=dwidth*.01f,usewidth=dwidth*.04f;
	float xleg=startx*2+usewidth+timelen+smallsize;
	 
	const char perform[]="%.1f%%";
	constexpr int maxbuf=40;
	 char perbuf[maxbuf];
	float pery=starty,yleg=starty+rowheight;
	float afm=10*density;
//hexcolor(0x469e3e),
//constexpr const NVGcolor cols[]={orange,yellow,hexcolor(0x469e3e),redinit,brown2};
constexpr const NVGcolor cols[]={orange,yellow,mediumseagreen,redinit,brown};
	for(int i=0;i<5;i++)	 {
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
	nvgFontSize(vg, smallsize);
	nvgTextAlign(vg,NVG_ALIGN_RIGHT|NVG_ALIGN_TOP);
	nvgFillColor(vg, *getblack());
	yleg=starty+rowheight;
	xleg-=afm*.5;
#ifndef NDEBUG
	float xnumpos=dwidth/3+dleft-smallsize;
#endif
	for(int i=0;i<5;i++)	 { 
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
	int buflen=snprintf(perbuf,maxbuf,">%.*f",gludecimal,gconvert(*levels*10));
	nvgText(vg, xleg,yleg,perbuf,perbuf+buflen);
	yleg+=rowheight;
	for(int i=0;i<3;i++)	 { 
		leginterval(vg,xleg,yleg,levels+i);
		yleg+=rowheight;
		}
	buflen=snprintf(perbuf,maxbuf,"<%.*f",gludecimal,gconvert((levels[3]+1)*10));
	nvgText(vg, xleg,yleg,perbuf,perbuf+buflen);
	yleg+=2*rowheight;
	if(memcmp(levels+1,border,2*sizeof(border[0]))) {
		leginterval(vg,xleg, yleg, stat->border); 
		const int perlen=snprintf(perbuf,maxbuf,perform,stat->pertarget*100);
		xleg-=afm*2;
		nvgTextAlign(vg,NVG_ALIGN_RIGHT|NVG_ALIGN_TOP);
		nvgText(vg, xleg,yleg,perbuf,perbuf+perlen);
		}

	}


void stats::otherstats(NVGcontext* vg) {
	nvgTextAlign(vg,NVG_ALIGN_CENTER|NVG_ALIGN_TOP);
	datainterval(vg,dwidth/2, 0,starttime,endtime);
	float rowheight=smallfontlineheight*1.5;
	float xpos=dwidth/3+dleft;
	float ypos=0.1*dheight+dtop;
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

//	#define averageglucose "Average glucose: " constexpr const int meantext=sizeof(averageglucose) -1;
	const int avlen= usedtext->averageglucose.size();
	char meanstr[avlen+5];
	memcpy(meanstr, usedtext->averageglucose.data(),avlen);
	int len=avlen+snprintf(meanstr+avlen,5,gformat,gconvert(mean*10));
	nvgText(vg, xpos,ypos,meanstr,meanstr+len);
	ypos+=rowheight;
	len=snprintf(buf,maxbuf,usedtext->EstimatedA1C, EA1Cper, EA1Cmmol);
	nvgText(vg, xpos,ypos,buf,buf+len);
	ypos+=rowheight;
	len=snprintf(buf,maxbuf,usedtext->GMI, GMIper, GMImmol);
	nvgText(vg, xpos,ypos,buf,buf+len);
	ypos+=rowheight;
	len=snprintf(buf,maxbuf,usedtext->SD,gconvert(sd*10));
	nvgText(vg, xpos,ypos,buf,buf+len);
	ypos+=rowheight;
	len=snprintf(buf,maxbuf,usedtext->glucose_variability,vc*100);
	nvgText(vg, xpos,ypos,buf,buf+len);
	}
extern void showstats(NVGcontext* vg,stats *stat) ;
void showstats(NVGcontext* vg,stats *stat) {
	stat->showbar(vg);
	stat->otherstats(vg) ;
}

#include "fromjava.h"
extern "C" JNIEXPORT void JNICALL fromjava(summarygraph) (JNIEnv *env, jclass clazz,jboolean val) {
	 showsummarygraph=val;
	}
extern "C" JNIEXPORT void JNICALL fromjava(analysedays) (JNIEnv *env, jclass clazz,jint days) {
	daystoanalyse=days;
	mkpercentiles(days);
	}

extern void begrenstijd() ;
extern "C" JNIEXPORT void JNICALL fromjava(endstats) (JNIEnv *env, jclass clazz) {
	showpers=0;
	begrenstijd() ;
	}


#endif
