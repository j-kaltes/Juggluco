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


#define PERCENTILES 1

#include <jni.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <algorithm>
//#include <filesystem>
#include <math.h>
#include <cstdint>
#include <cinttypes>
//#include "glucose.h"
//ScanData   *glucosenow=nullptr;



#include "fromjava.h"
#include "curve.h"
#include "nanovg_gl.h"
#include "nanovg_gl_utils.h"

//#include "gles3jni.h"
#include "config.h"
//#define FILEDIR "/sdcard/libre2/"
//#include "Glucograph.h"
#include "logs.h"
//#define LOGGER(...)  __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#include "settings/settings.h"

#include "SensorGlucoseData.h"
#include "sensoren.h"
#include "nums/numdata.h"
#include "nfcdata.h"

#include "error_codes.h"
#include "jugglucotext.h"
int startincolors=0;
int lasttouchedcolor=-1;
static void printGlString(const char* name, GLenum s) {
#ifndef NDEBUG
    const char* v = (const char*)glGetString(s);
    if(v)
	    LOGGER("GL %s: %s\n", name, v);
#endif
}
/*
NVGcolor *allcolors[]
{ &pink,&brown,& blue,& green,& lightblue,&greenblue,&red,
&transred,
&nowcolor,
&dooryellow,
&white,
&black,
&gray,
&yellow};*/

//extern NVGcolor invertcolor(const NVGcolor *colin) ;

	
/*
extern bool doinvertcolors(bool val);
bool doinvertcolors(bool val) {
	struct init {
		NVGcolor col[std::size(allcolors)]; 
		init() {
			for(int i=0;i<std::size(col);i++)
				col[i]=*allcolors[i];
			}
		};
	static init cols;
	if(val) {
		lightred= nvgRGBA(65, 65, 65, 255); 
		gray= {{{1.0f,1.0f,1.0f,.4f}}}; dooryellow=nvgRGBAf(0.9,0.9,0.1,0.3);
// white={{{0,0,0,1.0}}}; black=nvgRGBAf(1.0,1.0,1.0,1.0);
	 darkgray= nvgRGBAf(.8,.8,.8,.8);

		for(int i=0;i<std::size(cols.col);i++)  {
			*allcolors[i]=invertcolor(cols.col+i);;
			}
		}
	else {
// white=nvgRGBAf(1.0,1.0,1.0,1.0); black={{{0,0,0,1.0}}};
	      darkgray= nvgRGBAf(0,0,0,0.4);
		dooryellow=nvgRGBAf(0.9,0.9,0.1,0.3); 
lightred= nvgRGBAf(1, 0.95, 0.95, 1); 
gray= nvgRGBAf2(0,0,0,0.1);
		for(int i=0;i<std::size(cols.col);i++)
			*allcolors[i]=cols.col[i];
		}
	return true;
	}
*/	
void cpcolors(NVGcolor *foreground) {
	int wholes=nrcolors/oldnrcolors;
	for(int i=1;i<wholes;i++) 
		memcpy(foreground+i*oldnrcolors,foreground,oldnrcolors*sizeof(foreground[0]));
	if(int left=nrcolors%oldnrcolors) {
		memcpy(foreground+wholes*oldnrcolors,foreground,left*sizeof(foreground[0]));
		}
	}

static float screenwidthcm=0;

void createcolors() {
	NVGcolor *foreground=settings->data()->colors;
	NVGcolor *background=settings->data()->colors+startbackground;
	if(!settings->data()->colorscreated) {
		memcpy(foreground,allcolors,sizeof(allcolors));
	//	foreground[darkgrayoffset]=  nvgRGBAf(0,0,0,0.4);
		foreground[dooryellowoffset]=  nvgRGBAf2(0.9,0.9,0.1,0.3); 
		foreground[lightredoffset]=  nvgRGBAf2(1, 0.95, 0.95, 1); 
		foreground[grayoffset]=  nvgRGBAf2(0,0,0,0.1);

//		for(int i=0;i<std::size(allcolors);i++)  
		for(int i=0;i<oldnrcolors;i++)  {
			background[i]=invertcolor(foreground+i);
			}
		background[darkgrayoffset]=   nvgRGBAf2(.8,.8,.8,.8);
		background[dooryellowoffset]=  nvgRGBAf2(0.9,0.9,0.1,0.3);
//		background[lightredoffset]=   nvgRGBA(65, 65, 65, 255); 
		background[grayoffset]= {{{1.0f,1.0f,1.0f,.4f}}}; 
//		background[redoffset]= nvgRGBAf2(1.0,0,0,1.0);
		}
	if(settings->data()->colorscreated<3) {
		foreground[threehouroffset]=  nvgRGBAf2(1.0,0,1,0.5);
		background[threehouroffset]=  nvgRGBAf2(1.0,0,1,1);
		}
		/*
	if(settings->data()->colorscreated<4) {
		for(int i=redoffset+1;i<std::size(allcolors);i++)  {
			background[i]=invertcolor(foreground+i);
			}
		settings->data()->colorscreated=4;
		}
		*/
	if(settings->data()->colorscreated<5) {
		cpcolors(foreground);
		cpcolors(background);
		}
	if(settings->data()->colorscreated<15) {
		background[lightredoffset]=   blackbean; 
//		background[lightredoffset]=   hexcolor( 0x191C20);
//		background[lightredoffset]=   hexcolor( 0x0E0C01);
		settings->data()->colorscreated=15;
		}
	}

// white={{{0,0,0,1.0}}}; black=nvgRGBAf(1.0,1.0,1.0,1.0);
// ----------------------------------------------------------------------------

//#define fromjava(x) Java_none_libre_glucosecurve_GLES3JNILib_  ##x

//JNIEXPORT void JNICALL Java_tk_glucodata_Glucose_nfcdata(JNIEnv *env, jobject thiz, jbyteArray uid, jbyteArray info,jbyteArray data) {
//s/Java_com_android_gles3jni_GLES3JNILib_\([a-zA-Z]*\)(/fromjava(\1)(/g


NVGcontext* vg=nullptr;
	int font=0,menufont=0,monofont=0,whitefont=-1,blackfont=0;
float headheight;
//Numdata *numdata=nullptr;
jint width=-1,height=-1;

float dleft=0,dtop=0,dbottom{0},dright=0,dheight,dwidth;
float smallsize=300,menusize=smallsize,headsize=900,midsize, mediumfont;
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
float timelen=300;
union bounds_t{
	float array[4];
	struct {float xmin,ymin, xmax,ymax;};
	} ;
float listitemlen;

int numlist=0;
float smallerlen;

int duration=8*60*60;
extern bool fixatex,fixatey;

extern "C" JNIEXPORT jint JNICALL fromjava(openglversion)(JNIEnv* env, jclass obj) {
#ifdef NANOVG_GLES2_IMPLEMENTATION
	return 2;
#else
	return 3;
#endif
}

int showstream=1;
int showscans=1;
int showhistories=1;
int shownumbers=1;
int showmeals=0;
int invertcolors=0;
int showui=false;
float valuesize=0;

float facetimefontsize,facetimey;
void resetcurvestate();
extern "C" JNIEXPORT void JNICALL fromjava(initopengl)(JNIEnv* env, jclass obj,jboolean started) {
	if(!started) {
	    resetcurvestate();
	    }


    printGlString("Version", GL_VERSION);
    printGlString("Vendor", GL_VENDOR);
    printGlString("Renderer", GL_RENDERER);
    printGlString("Extensions", GL_EXTENSIONS);

//    const char* versionStr = (const char*)glGetString(GL_VERSION); if (strstr(versionStr, "OpenGL ES 3.")) 
//	vg = (version==2?nvgCreateGLES2:nvgCreateGLES3)(NVG_ANTIALIAS | NVG_STENCIL_STROKES
	vg = 

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

	if (vg == nullptr) {
		LOGGER("Could not init nanovg.");
		return ;
		}

const char standardfonts[][41]= {
"/system/fonts/Roboto-Black.ttf",
"/system/fonts/SourceSansPro-Bold.ttf",
"/system/fonts/NotoSerif-Bold.ttf",
"/system/fonts/DroidSans-Bold.ttf",
"/system/fonts/SourceSansPro-SemiBold.ttf",
"/system/fonts/Roboto-Regular.ttf",
};


const char menufonts[][41]={
"/system/fonts/Roboto-Medium.ttf",
"/system/fonts/SourceSansPro-SemiBold.ttf",
"/system/fonts/NotoSerif.ttf",
"/system/fonts/SourceSansPro-Regular.ttf",
"/system/fonts/Roboto-Regular.ttf",
"/system/fonts/DroidSans.ttf"};

	for(const char *name:standardfonts)  {
		if((blackfont = nvgCreateFont(vg, "dance-bold", name))!=-1)
			break;
		}
	if((whitefont= nvgCreateFont(vg, "dance-bold", "/system/fonts/Roboto-Regular.ttf"))==-1)
		whitefont=blackfont;

	for(const char *name:menufonts)  {
		if((menufont = nvgCreateFont(vg, "regular", name))!=-1)
			break;
		}
	if(invertcolors)
		font=whitefont;
	else
		font=blackfont;

	nvgFontFaceId(vg,font);
	nvgFontSize(vg, headsize);
	constexpr const char smaller[]="<";
	bounds_t bounds;
	nvgTextBounds(vg, 0,  0, smaller,smaller+sizeof(smaller)-1, bounds.array);
	smallerlen=bounds.xmax-bounds.xmin;

	nvgTextMetrics(vg, nullptr,nullptr, &headheight);
	headheight*=0.7;
	nvgFontSize(vg, smallsize);
	nvgTextMetrics(vg, nullptr,nullptr, &smallfontlineheight);
	constexpr const char timestring[]="29:59";
	nvgTextBounds(vg, 0,  0, timestring,timestring+sizeof(timestring)-1, bounds.array);
	timelen=bounds.xmax-bounds.xmin;




	const char listitem[]="39-08-2028 09-59 RRRRRRRRRRR 999.9";     
	nvgTextBounds(vg, 0,  0, listitem,listitem+sizeof(listitem)-1, bounds.array);
	listitemlen=bounds.xmax-bounds.xmin+smallsize;

	constexpr const char exampl[]="0M0063KNUJ0";
	float xhalf=dwidth/2;
	float yhalf=dheight/2;
	nvgFontSize(vg, mediumfont);
	nvgTextAlign(vg,NVG_ALIGN_LEFT|NVG_ALIGN_TOP);
	 nvgTextBounds(vg, xhalf,  yhalf,exampl, exampl+sizeof(exampl)-1,(float *)&sensorbounds);
	 sensorbounds.right-=sensorbounds.left;
	 sensorbounds.bottom-=sensorbounds.top;
	 sensorbounds.left-=xhalf;
	 sensorbounds.top-=yhalf;
	 LOGGER("sensorbounds.left=%.1f\n",sensorbounds.left);
	valuesize=sensorbounds.right*2;
	 fixatex=settings->data()->fixatex;
	 fixatey=settings->data()->fixatey;
	 if(fixatex)
	 	duration=settings->data()->duration;
	createcolors();
	invertcolors=settings->data()->invertcolors;
         startincolors=startbackground*invertcolors;
    }


bool alarmongoing=false;
extern "C" JNIEXPORT jboolean JNICALL fromjava(getisalarm)(JNIEnv* env, jclass obj) {
	return alarmongoing;
	}
extern "C" JNIEXPORT void JNICALL fromjava(setisalarm)(JNIEnv* env, jclass obj,jboolean val) {
	alarmongoing=val;
	}
	/*
extern "C" JNIEXPORT jboolean JNICALL fromjava(gettoucheverywhere)(JNIEnv* env, jclass obj) {
	return! settings->data()->specificstopalarm;
	}
extern "C" JNIEXPORT void JNICALL fromjava(settoucheverywhere)(JNIEnv* env, jclass obj,jboolean val) {
	settings->data()->specificstopalarm=!val;
	}*/
extern "C" JNIEXPORT jboolean JNICALL fromjava(turnoffalarm)(JNIEnv* env, jclass obj) {
//	return alarmongoing&&!settings->data()->specificstopalarm;
	return alarmongoing;
	}


//float rotation=	-NVG_PI/2.0;
extern "C" JNIEXPORT void JNICALL fromjava(resize)(JNIEnv* env, jclass obj, jint widthin, jint heightin,jint initscreenwidth) {
/*
LOGGER("resizein(%d,%d)\n",widthin,heightin);
if(widthin<heightin)  {
	width=heightin;
	height=widthin;
 	rotation=-NVG_PI/2.0;
	}
else {
	width=widthin;
	height=heightin;
 	rotation=0.0;
	} */
width=widthin;
height=heightin;
LOGGER("resize(%d,%d)\n",width,height);
//::width=width;
//::height=height;
dheight=height-dtop-dbottom,dwidth=width-dleft-dright; //Display area for graph in pixels

textheight=density*48;
int times=ceil(height/textheight);
textheight=height/times;
/*
struct init {
init(int initscreenwidth, int width) {
	if(initscreenwidth>=width) {
		menustr0[0]={"",0};
		menuopt0[0]=nullptr;
		}
	 showui=settings->getui();
	  }

	 };
static struct init once(initscreenwidth,width);	
*/

menutextheight=density*48;

extern const int maxmenulen;
const float maxmenu= (float)dheight/maxmenulen;
if(menutextheight>maxmenu)
	menutextheight=maxmenu;
	
LOGGER("menutextheight=%f\n", menutextheight);

	float facetimelen=2.0f*dwidth/3.0f;
	LOGGER("facetimelen=%.1f\n",facetimelen);
	facetimefontsize=smallsize*facetimelen/timelen;
	LOGGER("facetimefontsize=%.1f\n",facetimefontsize);
	float straal=dwidth*0.5f;
	facetimey=(straal-sqrt(pow(straal,2.0)-pow(facetimelen*.5,2.0)))*.70;

	LOGGER("facetimey=%.1f\n",facetimey);


}
//s/^\([^=]*\)=.*;/static float \1;/g
static float historyStrokeWidth;
static float numcircleStrokeWidth;
static float lowGlucoseStrokeWidth;
extern float pollCurveStrokeWidth;
float pollCurveStrokeWidth;
float hitStrokeWidth;
static float TrendStrokeWidth;
static float glucoseLinesStrokeWidth;
static float timeLinesStrokeWidth;
static float dayEndStrokeWidth;
static float nowLineStrokeWidth;
static float pointRadius;
float foundPointRadius,arrowstrokewidth;
extern "C" JNIEXPORT void JNICALL fromjava(setfontsize)(JNIEnv* env, jclass obj, jfloat small,jfloat menu,jfloat density,jfloat headin) {
float head=headin
#ifdef WEAROS
*0.7
#endif
;
LOGGER("density=%.1f, head=%.1f, small=%.1f\n",(double)density,(double)head,(double)small); 
::smallsize=small;
::menusize=menu;
::density=density;
::headsize=head;
::midsize=head/3;
::mediumfont= headsize/6;
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
//dbottom=dtop=dright=dleft=foundPointRadius;
}
/*
#include <EGL/egl.h>
void sizechanged() {
EGLDisplay display= eglGetCurrentDisplay ();
decltype(auto) surface=eglGetCurrentSurface(EGL_DRAW);
    EGLint width;
    EGLint height;
    eglQuerySurface(display,surface,EGL_WIDTH,&width);
    eglQuerySurface(display,surface,EGL_HEIGHT,&height);
	LOGGER("EGL: width=%d, height=%d\n",width,height);
}

*/
// Adds line segment from the last point in the path to the specified point.

int daystr(const time_t tim,char *buf) {
	struct tm tmbuf;
	 struct tm *stm=localtime_r(&tim,&tmbuf);
	return sprintf(buf,"%s %02d-%02d-%d",usedtext->daylabel[stm->tm_wday],stm->tm_mday,stm->tm_mon+1,1900+stm->tm_year);
	}
inline int mktmmin(const struct tm *tmptr) {
	return tmptr->tm_min;
	}
/*
inline int mktmmin(const struct tm *tmptr) {
	if(tmptr-> tm_sec<30)
		return tmptr->tm_min;
	return tmptr->tm_min+1;
	} */
bool hourmin(const time_t tim,char buf[6]) {
	struct tm tmbuf;
	 struct tm *stm=localtime_r(&tim,&tmbuf);
	 snprintf(buf,6,"%02d:%02d",stm->tm_hour,mktmmin(stm));
	 if(stm->tm_hour||stm->tm_min)
	 	return false;
	return true;
	}
int largedaystr(const time_t tim,char *buf) {
	struct tm stmbuf;
	localtime_r(&tim,&stmbuf);
 	return sprintf(buf,"%02d:%02d %s %02d %s %d",stmbuf.tm_hour,mktmmin(&stmbuf),usedtext->daylabel[stmbuf.tm_wday],stmbuf.tm_mday,usedtext->monthlabel[stmbuf.tm_mon],1900+stmbuf.tm_year);
	}
//		strftime(tbuf, maxbuf,"%H:%M %a %e %b %Y", &tmbuf);

//static constexpr const int glucosetype=0xffffffff;//std::numeric_limits<int>::max();
  static constexpr const int glucosetype=0x40000000;//std::numeric_limits<int>::max();
  static constexpr const int nosearchtype=0x20000000;//std::numeric_limits<int>::max();
	static constexpr const int historysearchtype=2|glucosetype;
	static constexpr const int scansearchtype=1|glucosetype;
	static constexpr const int streamsearchtype=4|glucosetype;
struct  {
int type;float under;float above;int frommin; int tomin;
uint32_t count;
float amount;
std::vector<int> ingredients;
bool operator()  (const Num *num) const {
	if(num->type>count)
		return false;
	if((type<0||num->type==type)) {
		float val=num->value;
		if(val>=under&&val<=above&&righttime(num->time)) {
			extern int carbotype;
			if(type==carbotype&&ingredients.size()>0) {
				return (num->mealptr>0)&& meals->datameal()->matchmeals(ingredients,amount,num->mealptr);
				}
			return  true;
			}
		}
	return false;
	}



bool operator()  (const Num &num) const {
	return operator()(&num);
	}

bool righttime(time_t tim)const {
	if(frommin<0&&tomin<0)
		return true;
	struct tm stm;
	localtime_r(&tim,&stm);
	int minutes=60*stm.tm_hour+ mktmmin(&stm);
	if(frommin<=tomin||tomin<0) {	
		if(minutes>=frommin&&(tomin<0||tomin>=minutes) ){
			return  true;
			}

		return false;
		}
	if(minutes<tomin|minutes>frommin)
		return true;
	return false;
	}
bool operator() (const ScanData *g) const {
//	if((type&scansearchtype)!=scansearchtype) return false;
	if(!g)
		return false;
	uint32_t glu=g->g*10.0;
	if(g->t&&glu&&glu>=under&&glu<=above&&righttime(g->t)) {
		return  true;
		}
	return false;
	}
bool operator() (const Glucose *g) const {
//	if((type&historysearchtype)!=historysearchtype) return false;
	if(!g||!g->valid())
		return false;
	uint32_t glu=g->getsputnik();

	if(glu>=under&&glu<=above&&righttime(g->gettime())) {
		return  true;
		}
	return false;
	}
} searchdata={.type=nosearchtype};
template <typename T>
bool searchhit(const T *ptr) {
	return searchdata(ptr);
	}
//template bool searchhit(const Num *ptr);

template   bool searchhit<Num>(const Num *ptr); 
/*
findminenmax:
getminmax(
	uint32_t  gmin=6000, gmax=0;
	for(auto pos=firstpos;pos<=lastpos;pos++) {
		auto glu=hist->sputnikglucose(pos);
		gmin=std::min(glu,gmin);
		gmax=std::max(glu,gmax);
		}
daylabel:
	char buf[128];
	auto endstr=daystr(hist.timeatpos((1+lastpos+firstpos)/2),buf);
	nvgFillColor(vg, datecolor);
	nvgTextAlign(vg,NVG_ALIGN_CENTER|NVG_ALIGN_MIDDLE);
	nvgFontSize(vg, headsize);
	nvgText(vg, dwidth/2+dleft,dtop+dheight/2, buf, buf+endstr);

		*/



pair<const ScanData*,const ScanData*> getScanRange(const ScanData *scan,const int len,const uint32_t start,const uint32_t end) {

	ScanData scanst{.t=start};
	const ScanData *endscan= scan+len;
	auto comp=[](const ScanData &el,const ScanData &se ){return el.t<se.t;};
  	const ScanData *low=lower_bound(scan,endscan, scanst,comp);
	if(low==endscan) {
		return {endscan,endscan};
		}
	scanst.t=end;
  	const ScanData *high=upper_bound(low,endscan, scanst,comp);

	return {low,high};
	}

extern std::vector<pair<const ScanData*,const ScanData*>> getsensorranges(uint32_t start,uint32_t endt) ;
std::vector<pair<const ScanData*,const ScanData*>> getsensorranges(uint32_t start,uint32_t endt) {
	auto hists= sensors->inperiod(start,endt) ;
	vector<pair<const ScanData*,const ScanData*>> polldata;
	polldata.reserve(hists.size());
	uint32_t timeiter=start;
	LOGGER("getsensorranges: \n");
	for(int i=hists.size()-1;i>=0&&timeiter<endt;i--)  {
		auto his=sensors->gethist(hists[i]);
		std::span<const ScanData> 	poll=his->getPolldata();
#ifndef NDEBUG
		auto wastimeiter=timeiter;
#endif
		auto ran=getScanRange(poll.data(),poll.size(),timeiter,endt);
		if(ran.first==ran.second)
			continue;

		for(const ScanData *striter=ran.second-1;striter>=ran.first;striter--) {
			if(striter->valid())	{
				timeiter=striter->t;
				ran.second=striter+1;
				break;
				}
			}
#ifndef NDEBUG
		constexpr const int  maxbuf=150;
		char buf[maxbuf];
		int len=datestr(wastimeiter,buf);

		const char tus1[]=" : ";
		constexpr const int tus1len=sizeof(tus1)-1;
		memcpy(buf+len,tus1,tus1len);

		len+=tus1len;
		len+=datestr(poll.data()->t,buf+len);

		memcpy(buf+len,tus1,tus1len);
		len+=tus1len;
		len+=datestr(ran.first->t,buf+len);
		const char tus[]=" - ";
		constexpr const int tuslen=sizeof(tus)-1;
		memcpy(buf+len,tus,tuslen);
		len+=tuslen;
		len+=datestr((ran.second-1)->t,buf+len);
		buf[len++]='\n';
		logwriter(buf,len);
#endif
		polldata.push_back(ran);
		}
	return polldata;
	}
static uint32_t pollgapdist=5*60;
pair<const ScanData*,const ScanData*> getScanRangeRuim(const ScanData *scan,const int len,const uint32_t start,const uint32_t end) {
	auto [low,high]= getScanRange(scan,len,start,end);
	const ScanData *endscan= scan+len;
	/*
	ScanData scanst{.t=start};
	const ScanData *endscan= scan+len;
	auto comp=[](const ScanData &el,const ScanData &se ){return el.t<se.t;};
  	const ScanData *low=lower_bound(scan,endscan, scanst,comp);
	if(low==endscan) {
		return {endscan,endscan};
		}
	scanst.t=end;
  	const ScanData *high=upper_bound(low,endscan, scanst,comp);*/

	if(low>scan&&(low->t-(low-1)->t)<pollgapdist)
		low--;
	if(high<endscan&&((high+1)->t-high->t)<pollgapdist)
		high++;
	return {low,high};
	}

static void		sidenum(const float posx,const float posy,const char *buf,const int len,const bool hit) {
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
		nvgTextAlign(vg,align);
		nvgText(vg, valx,posy, buf, buf+len);
		}

static uint32_t getmaxlabel() { return settings->getlabelcount(); }


int *numheights;
int shownlabels;

//y=A+x*D
//x=(y-A)/D;
jfloat tapx=-700,tapy;
bool selshown=false;
#include "numdisplay.h"
//vector<NumDisplay*> numdatas;
extern vector<NumDisplay*> numdatas;
/*
bool updatenums(int sock,struct changednums *nums) {
	for(int i=0;i<numdatas.size();i++) {
		if(!numdatas[i]->update(sock,nums) )
			return false;
		}
	return true;
	}
	*/
int typeatheight(const float h) {
//	const float gr= density*24;
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

/*
int numheighttype(cont float h) {

	float schuif=smallfontlineheight*3;
	int volg=round((shownlabels-1)*(h-(dtop+schuif))/(dheight-schuif-smallfontlineheight/2));
	for(int i=0;i<shownlabels;i++) 
		if(numheight[i]==volg)
			return volg;
	}
	*/

extern "C" JNIEXPORT jfloat JNICALL fromjava(freey) (JNIEnv *env, jclass clazz) {
	const int nrlabs=getmaxlabel();
	static const int mid=nrlabs/2-1;
	static const float midh=numtypeheight(mid);
//	const float dmid=dtop+dheight/2.0f;
//	if(abs(dmid-mid)>(smallsize*2)) return dmid;
	static  float boven=numtypeheight(mid+1);
	return (boven+midh)/2.0f;
	}


//	float labelweight[7]={[6]=180.0f};
//template <class TX,class TY> void showNums(NVGcontext* vg,const Num *low,const Num *high,  TX &&transx,  TY &&transy) {


//time_t showtime=0;
static bool glucosepointinfo(time_t tim,uint32_t value,   float posx, float posy) {
	if((!selshown&&nearby(posx-tapx,posy-tapy))) {
		constexpr int maxbuf=50;
		char buf[maxbuf];
		struct tm tmbuf;
		 struct tm *tms=localtime_r(&tim,&tmbuf);

		int len=snprintf(buf,maxbuf,"%02d:%02d", tms->tm_hour,mktmmin(tms));
		nvgFontSize(vg, smallsize);
		nvgTextAlign(vg,NVG_ALIGN_CENTER|NVG_ALIGN_MIDDLE);
		float cor=((posy-dtop)<(dheight/2))?smallsize:-smallsize;
		nvgText(vg, posx,posy+cor*.92, buf, buf+len);
	 	len=snprintf(buf,maxbuf,gformat, gconvert(value));
		sidenum(posx,posy,buf,len,false);
		
	//	nvgText(vg, posx,posy+cor*.92*2, buf, buf+len);

		selshown=true;
		return true;
		}
	return false;
	}
static bool glucosepoint(time_t tim,uint32_t value,   float posx, float posy) {
	nvgCircle(vg, posx,posy,pointRadius);
	return glucosepointinfo(tim,value,posx,posy);
	}
constexpr const int nfclen=344;
extern int    timestr(char *buf,time_t tim) ;
class nfc: public nfcdata {
public:
nfc(const string &base, const sensor *sens,const time_t d):nfcdata(new uint8_t[nfclen],d) {
	constexpr const int tmplen=20;
	char tmp[tmplen];
	int len=timestr(tmp,d); 
	assert(len<tmplen);
	pathconcat  rawfile(base,string_view(sens->name,sensornamelen),string_view(tmp,len),"raw.dat");
	if(readfile(rawfile,databuf,nfclen)!=nfclen) {
		this->~nfc();
		databuf=nullptr;
		}
	}
~nfc() {
	delete[] reinterpret_cast<uint8_t *>(databuf);
	}
};

static void endstep() ;
static bool emptytap=false;
template <class TX,class TY> bool showScan(NVGcontext* vg,const ScanData *low,const ScanData *high,  const TX &transx,  const TY &transy,const int colorindex) {

	nvgFillColor(vg,*getcolor(colorindex));
	nvgBeginPath(vg);
	bool search=scansearchtype==(scansearchtype&searchdata.type);
	for(const ScanData *it=low;it!=high;it++) {
		if(it->valid()) {
			const uint32_t tim= it->t;
			const auto glu=it->g*10;
			const auto posx= transx(tim),posy=transy(glu);
			if(search&&searchdata(it)) 
				nvgCircle(vg, posx,posy,foundPointRadius);
			else {
				if(glucosepoint(tim,glu,posx,posy))
					lasttouchedcolor=colorindex;
				}
			}
		}
	nvgFill(vg);
	return true;
	}
	//			nvgCircle(vg, posx,posy,;
	

static void makecircle(float posx,float posy) {
	nvgBeginPath(vg);
	nvgCircle(vg, posx,posy,pointRadius);
	nvgFill(vg);

	}

template <class TX,class TY> void showlineScan(NVGcontext* vg,const ScanData *low,const ScanData *high,  const TX &transx,  const TY &transy,const int colorindex) {
	bool search=streamsearchtype==(streamsearchtype&searchdata.type);
	if(search) {
		nvgBeginPath(vg);
//		nvgStrokeColor(vg, yellow); nvgFillColor(vg, yellow);
		nvgStrokeColor(vg, *getyellow()); nvgFillColor(vg, *getyellow());
		nvgStrokeWidth(vg, hitStrokeWidth);
		bool restart=true,first;
		uint32_t late=0,dif=pollgapdist;
		bool washit=false;
		float prevx=-1.0f,prevy;
		for(const ScanData *it=low;it!=high;it++) {
			if(it->valid()&&searchdata(it)) {
					const uint32_t tim= it->t;
					const auto glu=it->g*10;
					const auto posx= transx(tim),posy=transy(glu);
					if(washit) {
						if(!restart&&tim>late) {
							nvgStroke(vg);
							if(first)
								makecircle(prevx,prevy);
							restart=true;
							}
						}
					else {
						washit=true;	
						restart=true;
						}
					if(restart) {
						nvgBeginPath(vg);
						 nvgMoveTo(vg, posx,posy);
						 restart=false;
						 first=true;
						 }
					else {
						first=false;
						nvgLineTo( vg,posx,posy);
						}
					late=tim+dif;
					prevx=posx;
					prevy=posy;
					}
			else {
				if(washit&&!restart) {
					nvgStroke(vg);
					if(first)
						makecircle(prevx,prevy);
					}
				else
					washit=false;
				restart=true;
				}
			}
		if(washit) {	
			if(!restart)
				nvgStroke(vg);
			if(first)
				makecircle(prevx,prevy);
			}
		}
	bool restart=true;
	nvgBeginPath(vg);
	const NVGcolor *col=getcolor(colorindex);
	nvgStrokeColor(vg, *col);
	nvgFillColor(vg,*col);
	nvgStrokeWidth(vg, pollCurveStrokeWidth);
	uint32_t late=0,dif=5*60;
	float startx=-1000,starty=-1000;
	for(const ScanData *it=low;it!=high;it++) {
			if(it->valid()) {
				const uint32_t tim= it->t;
				const auto glu=it->g*10;
				const auto posx= transx(tim),posy=transy(glu);

				if(!restart&&tim>late) {
					nvgStroke(vg);
					if(startx>=0) {
						nvgBeginPath(vg);
						nvgCircle(vg, startx,starty,pollCurveStrokeWidth);
						nvgFill(vg);
						 }
					restart=true;
					}
				if(restart) {
					nvgBeginPath(vg);
					 nvgMoveTo(vg, posx,posy);
					 startx=posx,starty=posy;
					 restart=false;
					 }
				else {
					 startx=starty=-1000.0f;
					nvgLineTo( vg,posx,posy);
					}

				late=tim+dif;

				if(glucosepointinfo(tim,glu, posx, posy) ) {
					nvgLineTo( vg,posx,posy);
					nvgStroke(vg);
					nvgBeginPath(vg);
					nvgCircle(vg, posx,posy,pointRadius*1.3);
					nvgFill(vg);
					nvgBeginPath(vg);
					nvgMoveTo(vg, posx,posy);
					lasttouchedcolor=colorindex;
					}
				}
			else {
				/*
				if(!restart) {
					nvgStroke(vg);
					restart=true;
					} */
				}
		}

		nvgStroke(vg);
		if(startx>=0) {
			nvgBeginPath(vg);
			nvgCircle(vg, startx,starty,pollCurveStrokeWidth);
			nvgFill(vg);
			 }
		}

pair<int32_t,int32_t> histPositions(const SensorGlucoseData  * hist, const uint32_t starttime, const uint32_t endtime) {
	int32_t firstmog=hist->getstarthistory();
	int32_t lastmog= hist->getendhistory()-1;
	LOGGER("histPositions first=%u last=%u\n",firstmog,lastmog);
	if(firstmog>=lastmog)
		return {firstmog,lastmog};
	uint32_t begin=hist->getstarttime();
	int sdisp=starttime-begin;
	int period=hist->getinterval();
	int off=sdisp/period;	
	int32_t	firstpos=firstmog+(uint32_t)((off>0)?off:0);
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

template <class TX,class TY> void histcurve(NVGcontext* vg,const SensorGlucoseData  * hist, const int32_t firstpos, const int32_t lastpos,const TX &xtrans,const TY &ytrans,const int colorindex) {
	const NVGcolor *col=getcolor(colorindex);
	nvgStrokeColor(vg, *col);
	nvgFillColor(vg,*col);
	 bool restart=true;
	for(auto pos=firstpos;pos<=lastpos;pos++) {
		const Glucose *histglu=hist->getglucose(pos);
		if(histglu->valid()) {
			const uint32_t tim=histglu->gettime(),glu=histglu->getsputnik();
			auto posx=xtrans(tim),posy=ytrans( glu);
			bool oncurve=glucosepointinfo(tim,glu, posx, posy);
			if(restart) {
				if(oncurve) {
					nvgBeginPath(vg);
					nvgCircle(vg, posx,posy,pointRadius*1.3);
					nvgFill(vg);
					lasttouchedcolor=colorindex;
					}
				nvgBeginPath(vg);
				 nvgMoveTo(vg, posx,posy);
				 restart=false;
				 }
			else {
				nvgLineTo( vg, posx,posy);
				if(oncurve) {
					nvgStroke(vg);
					nvgBeginPath(vg);
					nvgCircle(vg, posx,posy,pointRadius*1.3);
					nvgFill(vg);
					nvgBeginPath(vg);
					nvgMoveTo(vg, posx,posy);
					lasttouchedcolor=colorindex;
					}
				}

			}
		else {
			if(!restart) {
				nvgStroke(vg);
				restart=true;
				}
			}
		}
	if(!restart)
		nvgStroke(vg);
	if((searchdata.type&historysearchtype)==historysearchtype) {
		nvgBeginPath(vg);
		for(auto pos=firstpos;pos<=lastpos;pos++) {
			const Glucose *glu=hist->getglucose(pos);
			if(searchdata(glu)) {
				const auto tim=glu->gettime();
				if(tim) {
					const auto sput=glu->getsputnik();
					auto xc=xtrans(tim);
					auto yc= ytrans(sput);
					nvgCircle(vg,xc,yc,foundPointRadius);
					}
//				nvgCircle(vg, xtrans(glu->time),ytrans(glu->getsputnik()),foundPointRadius); //Bus Error in release arm32
				}
			}
		nvgFill(vg);
		}
	}


static uint32_t getnumlasttime() {
	uint32_t last=0u;
	for(auto el:numdatas)  {
		auto mog=el->getlasttime();
		if(mog>last)
			last=mog;	
		}
	return last;
	}
uint32_t maxstarttime() ;
uint32_t maxtime() {
	const uint32_t numt=getnumlasttime();
	const uint32_t sent= sensors->timelastdata(); 
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
	return min(numt,sent);
	}
int gmin=2*180;
int grange=8*180;
uint32_t starttime;
uint32_t maxstarttime() {
//	return maxtime()-duration/2;
	float duraf=((float)valuesize/dwidth);
	LOGGER("dwidth=%f valuesize=%f duraf=%f\n",(double)dwidth,(double)valuesize,(double)duraf);
//	return time(nullptr)-((duraf<=0.65)?(duration*0.45):0);
//	float subtr=0.55f - (55.0f*(duraf - 0.33f))/60.0;
//	float subtr=0.8525 - 0.916667*duraf;
	float subtr=0.91 - duraf*1.2f;
	return time(nullptr)-subtr*duration;
//	return time(nullptr)-((duraf<=0.65)?(duration*0.55):0);
	}
uint32_t minstarttime() {
	uint32_t mini=mintime();
	if(mini<duration)
		return mini;

	return mini-duration/2;
	}
/*

	return mini-duration;
	*/
void begrenstijd() {
	auto maxstart= maxstarttime();
	if(starttime>maxstart)
		starttime=maxstart;
	else {
		auto minstart= minstarttime();
		if(starttime<minstart)
			starttime=minstart;
		}
	}

#include <memory>
uint32_t settime=0;
uint32_t setend=0;

pair<float,float> drawtrender(NVGcontext* vg,const std::array<uint16_t,16> &trend,const float x,const float y,const float w,const float h) {
	auto minel=std::min_element(trend.begin(),trend.end());
	auto maxel=std::max_element(trend.begin(),trend.end());
	 const int low=minel-trend.begin();
	 const int high=maxel-trend.begin();
	 if(low<0||high<0)
	 	return {0,dtop+dheight/2};
	const float lowval=*minel;
	const float highval=*maxel;
	const float mid=(lowval+highval)/2.0;
	LOGGER("width=%.0f, height=%.0f\n",w,h);
	LOGGER("low=%.0f,high=%.0f,mid=%.0f\n",lowval,highval,mid);
	constexpr float hglurange=2*180;
	const auto gety=[y,h,mid](const short val)->float  { return y+h/2.0-(((val-mid)/hglurange)*h);};
	const int step=w/(trend::num-1);
	nvgBeginPath(vg);
	 nvgStrokeWidth(vg, TrendStrokeWidth);
//	nvgStrokeColor(vg, white);
	nvgStrokeColor(vg, *getblack());
	int i=0;
	unsigned short glu0;
	for(;!(glu0=trend[i]);i++)
		if(i>=(trend.size()-3))
			return {0,dtop+dheight/2};
	float pos0=gety(glu0);
	float posx= x+i*step;
	 nvgMoveTo(vg,posx ,pos0);
	LOGGER("%.1f (%hi) (%.0f,%.0f)\n",glu0/180.0,glu0,posx,pos0);
	posx+=step;
	float posy;
	i++;
	for(;i<trend.size();i++,posx+=step) {
		short glu=trend[i];
		if(glu) {
			posy=gety(glu);
			LOGGER("%.1f (%hi) (%.0f,%.0f)\n",glu/180.0,glu,posx,posy);
			nvgLineTo( vg,posx ,posy);
			}
		}
	LOGGER("\n");
	nvgStroke(vg);
	return std::pair<float,float>({pos0,posy});
	}

static void startstep(const NVGcolor &col);
struct {
float left,top,right,bottom;
} menupos;
void showok(bool good,bool up) {
	nvgFontSize(vg,headsize/4 );
	nvgTextAlign(vg,NVG_ALIGN_RIGHT|(up?NVG_ALIGN_TOP:NVG_ALIGN_BOTTOM));
	const float fromtop= mediumfont*2.0f;
	float ypos=dtop+(up?fromtop:(dheight-fromtop));
	float xpos=dwidth+dleft-mediumfont*3.0f;

	const char *ok=good?"OK":"ESC";
	const int oklen=good?2:3;
	nvgTextBounds(vg, xpos,ypos ,ok , ok+oklen, (float *)&menupos);
	nvgText(vg, xpos,ypos,ok,ok+oklen);
	menupos.left-=mediumfont;
	menupos.right+=mediumfont;
	menupos.bottom+=mediumfont;
	menupos.top-=mediumfont;
	}
static bool		showerror(NVGcontext* vg,const string_view str1,const string_view str2) {
	startstep(*getyellow());
	nvgFontSize(vg, midsize);
	nvgFillColor(vg, *getblack());
	nvgTextAlign(vg,NVG_ALIGN_LEFT|NVG_ALIGN_BOTTOM);
	nvgText(vg, dleft+dwidth/10,dtop+dheight/3, str1.begin(), str1.end());
	nvgFontSize(vg, midsize*.8);
	nvgTextAlign(vg,NVG_ALIGN_LEFT|NVG_ALIGN_MIDDLE);
//	nvgText(vg, dleft+dwidth/2,dtop+dheight/2, str2.begin(), str2.end());
//	void nvgTextBox(NVGcontext* ctx, float x, float y, float breakRowWidth, const char* string, const char* end);
	nvgTextBox( vg, dleft+dwidth/10, dtop+dheight/2, dwidth*8/10, str2.begin(), str2.end());

	showok(true,false);
	return true;
	}
static void		scanwait(NVGcontext* vg) {
	startstep(*getwhite());
	nvgFontSize(vg, headsize);
	nvgFillColor(vg, *getblack());
	nvgTextAlign(vg,NVG_ALIGN_CENTER|NVG_ALIGN_MIDDLE);
//	std::string_view str1="Scanned";
	const std::string_view str1=usedtext->scanned;
	nvgText(vg, dleft+dwidth/2,dtop+dheight/2, str1.begin(), str1.end());
	endstep();
	}
#include "gluconfig.h"
int mkshowlow(char *buf, const int maxbuf) {
		return snprintf(buf,maxbuf,"%.*f>",gludecimal,gconvert(glucoselowest*10));
	}
int mkshowhigh(char *buf, const int maxbuf) {
	return snprintf(buf,maxbuf,"%.*f<",gludecimal,gconvert(glucosehighest*10));
	}
int getglucosestr(uint32_t nonconvert,char *glucosestr,int maxglucosestr) {
	if(nonconvert<glucoselowest) {
		return mkshowlow(glucosestr, maxglucosestr) ;
		}
	else {
		if(nonconvert>glucosehighest) {
			return mkshowhigh(glucosestr, maxglucosestr) ;
			}
		else {
			const float convglucose= gconvert(nonconvert*10);
			return snprintf(glucosestr,maxglucosestr,gformat,convglucose);
			}
		}
	}

constexpr int maxbluetoothage=3*60;
static void	showscanner(NVGcontext* vg,const SensorGlucoseData *hist,int scanident) {
	time_t nu=time(nullptr);
	const ScanData &last=*hist->getscan(scanident);
	startstep((nu-last.t)< maxbluetoothage?*getwhite():getoldcolor());
	float x= dwidth+dleft;
	constexpr int maxbuf=50;
	char buf1[maxbuf];
	const int32_t gluval=last.g;
//	const int32_t gluval=501;
	int len1;
	float endtime=x,sensleft=0.0f;

	if(gluval<glucoselowest) {
	//	len1=snprintf(buf1,maxbuf,"%.*f>",gludecimal,gconvert(glucoselowest*10));
		len1=mkshowlow(buf1,maxbuf);
		endtime-=smallerlen;
		sensleft=smallerlen;
		}
	else {
		if(gluval>glucosehighest) {
		//	len1=snprintf(buf1,maxbuf,"%.*f<",gludecimal,gconvert(glucosehighest*10));
			len1=mkshowhigh(buf1,maxbuf);
			endtime-=smallerlen;
			}
		else {
			len1=snprintf(buf1,maxbuf,gformat, gconvert(gluval*10.0f));
			}
		}

	float bounds[4];
	nvgTextAlign(vg,NVG_ALIGN_RIGHT|NVG_ALIGN_MIDDLE);
	nvgFontSize(vg, headsize);
	nvgTextBounds(vg, x,dtop+dheight/2 , buf1, buf1+len1, bounds);
	nvgFillColor(vg, *getblack());
	auto [first,y]=drawtrender( vg,hist->gettrendsbuf(scanident),dleft,dtop,bounds[0]-dleft,dheight);
	float th=(bounds[3]-bounds[1])/2.0;
	if(y<th) 
		y=th;
	else
		if((dheight-(y-dtop))<th)
			y=dheight-th;

	nvgText(vg, x,y, buf1, buf1+len1);
	char buf[maxbuf];
	time_t tim=last.t;
	struct tm tmbuf;
	 struct tm *tms=localtime_r(&tim,&tmbuf);
	int len=snprintf(buf,maxbuf,"%02d:%02d", tms->tm_hour,mktmmin(tms));
//	nvgFontSize(vg,smallfont );
	const float yunder=y+((y>(dheight/2))?-1:1)*headsize/2.0;
	nvgFontSize(vg,mediumfont );
	nvgText(vg, endtime,yunder, buf, buf+len);
	const sensorname_t *sensorname=hist->shortsensorname();
	nvgFontSize(vg,smallsize );
	nvgTextAlign(vg,NVG_ALIGN_LEFT|NVG_ALIGN_MIDDLE);
	nvgText(vg,bounds[0] -sensleft,yunder, sensorname->begin(), sensorname->end());

	showok( (last.g>70&&last.g<=140), ((y-dtop)<(dheight/2))?false:true);
	}
	/*
static void	showscanner(NVGcontext* vg,int sensorident,int scanident) {
	const SensorGlucoseData *hist=sensors->gethist(sensorident);
	showscanner(vg,hist, scanident) ;

	}
	*/
	/*
static bool	showlastscan(NVGcontext* vg,const ScanData *last) {
	if(!last)
		return false;
	if(!last->g) {
		return showerror(vg,"Data unclear, wait", "for things to get better");
		}
	showscan(vg,*last,lastnfcdata,nullptr);
	return true;
	}*/
//int gmaxmax=180*7.5;
//int gminmin=180*3;
void setextremes(pair<int,int> extr) {
	auto [gminin,gmaxin]=extr;
	setend=0;
	const uint32_t gmaxmax=settings->graphhigh();
	const uint32_t gminmin=settings->graphlow();
	if(gmaxin<gmaxmax)
		gmaxin=gmaxmax;
	if(gminin>gminmin)
		gminin=gminmin;
	grange=gmaxin-gminin;
	gmin=gminin;
	}

static pair<int,int> getextremes(const vector<int> &hists, const pair<const ScanData *,const ScanData*> **scanranges, int scannr,const pair<int32_t,int32_t> *histpositions) {
	int gmax=0;
	int gmin=6000;
	const int histlen=hists.size();
	for(int i=0;i<histlen;i++) {
		if(1||showhistories) {
			for(auto pos=histpositions[i].first,last=histpositions[i].second;pos<=last;pos++) {
				int glu=sensors->gethist(hists[i])->sputnikglucose(pos);
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
					int glu=it->g*10;
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
template <class LT> void glucoselines(const float last,const float smallfontlineheight,const int gmax,const LT &transy) {
	nvgStrokeWidth(vg, glucoseLinesStrokeWidth);
	nvgStrokeColor(vg, *getgray());
	const double yscale=transy(1)-transy(0);
	const float mindisunit=smallsize*1.5;
	const float minst=abs(mindisunit/yscale);
	const float unit=settings->usemmolL()?0.5*180.0:100;
	const float unit2=unit*2;

	uint32_t step=minst<=unit?unit:ceilf(minst/unit2)*unit2;
	float startld;
	nvgTextAlign(vg,NVG_ALIGN_CENTER|NVG_ALIGN_MIDDLE);

	if(settings->data()->levelleft) {
		startld = timelen*.4;
		}
	else  {
		startld =  dwidth/2+dleft;
		}


	uint32_t keer=floorf(ceil(gmin)/step);
	uint32_t startl=keer*step;
//	const float endline=(dleft+dwidth)>nupos?nupos:(dwidth+dleft);
	const float endline=last;
	for(auto y=startl+step;y<gmax;y+=step) {
		float dy=transy(y);
		nvgBeginPath(vg);
	 	nvgMoveTo(vg,dleft ,dy) ;
		nvgLineTo( vg, endline,dy);
		nvgStroke(vg);
		if(dy>smallfontlineheight) {
			constexpr const int  bufsize=50;
			char buf[bufsize];
//			int len=snprintf(buf,bufsize,"%g",y/180.0);
			int len=snprintf(buf,bufsize,"%g",gconvert(y));
			if(len>bufsize)
				len=bufsize;
			nvgText(vg, startld,dy, buf, buf+len);
			}

		}
	}
struct displaytime {
	const uint32_t tstep;
	const uint32_t first;
	const uint32_t last;
	};
template <class LT>
const displaytime getdisplaytime(const uint32_t nu,const uint32_t starttime,const uint32_t endtime, const LT &transx) {
	const float xscale=transx(1)-transx(0);
	const float mindisunit=smallsize*3;
	const  float minst=abs(mindisunit/xscale);
	const uint32_t tstep=(minst<=60*15)?60*15:((minst<=60*30)?60*30:ceilf((minst/(60.0*60)))*(60*60));
	const uint32_t first=uint32_t(ceilf(starttime/(double)tstep))*tstep;	
	const uint32_t endhier=(nu<endtime)?(nu+tstep-59):(endtime-1);
	const uint32_t last=uint32_t(floorf(endhier/double(tstep)))*tstep;	
	LOGGER("getdisplaytime xscale=%.1f %u %u %u\n",xscale,tstep,first,last);
	return {tstep,first,last};
}
template <class LT>
void timelines(const displaytime *disp, const LT &transx ,uint32_t nu) {

	const uint32_t tstep=disp->tstep;
	const uint32_t first=disp->first;
	const uint32_t last= disp->last;
	#ifdef WEAROS
	const uint32_t numlast= (disp->last>nu)?(disp->last-tstep):disp->last;
	#endif
	nvgFillColor(vg, *getblack());
	nvgTextAlign(vg,NVG_ALIGN_CENTER|NVG_ALIGN_TOP);
	const float timehight=
	#ifdef WEAROS
		smallfontlineheight*1.6
	#else
		0
	#endif
	;
	for(auto tim=first;tim<=last;tim+=tstep) {
		float dtim=transx(tim);
		char buf[6];
		struct tm tmbuf;
		time_t tmptime=tim;
		 struct tm *stm=localtime_r(&tmptime,&tmbuf);

	 	if(stm->tm_hour||stm->tm_min) {
			if(stm->tm_min||stm->tm_hour%3) {
				nvgStrokeWidth(vg, timeLinesStrokeWidth);
				nvgStrokeColor(vg, *getgray());
				}
			else {
				nvgStrokeWidth(vg, timeLinesStrokeWidth);
				nvgStrokeColor(vg, *getthreehour());
				}
			}
		else {
			nvgStrokeWidth(vg, dayEndStrokeWidth);
			nvgStrokeColor(vg, *getblack());
			}
	#ifdef WEAROS
		 if(tim<=numlast)  
	#endif
		 {
		 	snprintf(buf,6,"%02d:%02d",stm->tm_hour,mktmmin(stm));
			nvgText(vg, dtim,timehight, buf, buf+5);
			}
		nvgBeginPath(vg);
		nvgMoveTo(vg,dtim ,0) ;
		nvgLineTo( vg, dtim,dheight);
		nvgStroke(vg);
		}
	}

template <class LT> void epochlines(uint32_t first,uint32_t last, const LT &transx) {
		time_t startin=first;

		struct tm tmbuf;
		 struct tm *stm=localtime_r(&startin,&tmbuf);
		auto hour=stm->tm_hour;
		if(stm->tm_min) {
			startin+=(60-stm->tm_min)*60;
			hour++;
			}
		
		time_t start=startin+(24-hour)*60*60;
		nvgStrokeWidth(vg, dayEndStrokeWidth);
		nvgStrokeColor(vg, *getblack());
		for(time_t t=start;t<last;t+=(24*60*60)) {
			float dtim=transx(t);
			LOGGER("%ld\n",t);
			nvgBeginPath(vg);
			nvgMoveTo(vg,dtim ,0) ;
			nvgLineTo( vg, dtim,dheight);
			nvgStroke(vg);
			}
		nvgStrokeWidth(vg, timeLinesStrokeWidth);
		nvgStrokeColor(vg, *getthreehour());
		const int inthree=hour%3;
		start=startin+(inthree?((3-inthree)*60*60):0);
		LOGGER("startin=%ld start=%ld last=%d inthree=%d\n",startin,start,last, inthree);
		for(time_t t=start;t<last;t+=(3*60*60)) {
			float dtim=transx(t);
			nvgBeginPath(vg);
			nvgMoveTo(vg,dtim ,0) ;
			nvgLineTo( vg, dtim,dheight);
			nvgStroke(vg);
			}
	}
extern std::vector<int> usedsensors;
extern void setusedsensors() ;
extern void setusedsensors(uint32_t nu) ;
//extern std::span<streamdat> laststream;
//std::span<streamdat>  laststream{},oldstream{new streamdat[1](),1};
void setmaxsensors(size_t sensornr) {
	setusedsensors();
//	str={new streamdat[sensornr],sensornr};
	}

/*
void setmaxsensors(struct streams_t *str,int sensornr) {
	delete[] str->glucosetime;
	delete[] str->glucose;
	delete[] str->sensorname;
	delete[] str->glucoserate;
	str->sensornr=sensornr;
	str->glucosetime=new time_t[sensornr]();
	str->glucose=new uint32_t[sensornr];
	str->glucoserate=new float[sensornr];
	str->sensorname=new constcharptr_t[sensornr];
	}

void setmaxsensors(int nr) {
	delete[] glucosetime;
	delete[] glucose;
	delete[] sensorname;
	delete[] glucoserate;
	sensornr=nr;
	glucosetime=new time_t[sensornr]();
	glucose=new uint32_t[sensornr];
	glucoserate=new float[sensornr];
	sensorname=new constcharptr_t[sensornr];
	}
class startnow { 
public:
template <typename T>
startnow(T fun) {
	fun();
	}
	};
startnow oldinit([]{setmaxsensors(oldstream,1);});

	*/
extern "C" JNIEXPORT void JNICALL fromjava(setmaxsensors) (JNIEnv *env, jclass clazz,jint nr) {
	setmaxsensors(nr);
	}

//float highglucose=1350;
//float lowglucose=702;
uint32_t lastsensorends() {
		int lastsen=sensors->last();
		if(lastsen>=0) {
			const sensor *sen=sensors->getsensor(lastsen);
			time_t enddate= (14*24)*60*60+sen->starttime;
			return enddate;
			}
		return 0;
		}
//void	showbluevalue(const float dlast,const time_t nu,const int xpos) {

void drawarrow(NVGcontext* vg, float rate,float getx,float gety) {
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
			nvgBeginPath(vg);
			nvgStrokeColor(vg, *getblack());
			nvgStrokeWidth(vg, arrowstrokewidth);
			nvgMoveTo(vg,x1,y1) ;
			nvgLineTo( vg, xtus,ytus);
			nvgStroke(vg);
			nvgBeginPath(vg);
			nvgFillColor(vg, *getblack());
			nvgMoveTo(vg,sx1,sy1) ;
			nvgLineTo( vg, getx,gety);
			nvgLineTo( vg, sx2,sy2);
			nvgLineTo( vg, xtus,ytus);
			nvgClosePath(vg);
			nvgFill(vg);

			}
	}
void showvalue(const ScanData *poll,const sensorname_t *sensorname, float getx,float gety) {
	LOGGER("showvalue %s\n",sensorname->data());
			float sensory= gety+headsize/3.1;
			nvgFillColor(vg, *getblack());
			nvgFontSize(vg,mediumfont );
			nvgTextAlign(vg,NVG_ALIGN_LEFT|NVG_ALIGN_TOP);
			nvgText(vg, getx,sensory, sensorname->begin(), sensorname->end());
			nvgTextAlign(vg,NVG_ALIGN_LEFT|NVG_ALIGN_MIDDLE);
			constexpr const int maxhead=11;
			char head[maxhead];
//#if defined(NDEBUG) 
#if 1
			const auto nonconvert= poll->g;
#else
			const uint32_t nonconvert= 40;
#endif
			nvgFontSize(vg, headsize*.8);
			if(nonconvert<glucoselowest) {
const				float valuex=getx;
//				int gllen=snprintf(head,maxhead,"%.*f>",gludecimal,gconvert(glucoselowest*10));

				 int gllen=mkshowlow(head, maxhead) ;
				nvgText(vg,valuex,gety, head, head+gllen);
				}
			else {
				if(nonconvert>glucosehighest) {
				float valuex=getx-density*14.0f;
				//	int gllen=snprintf(head,maxhead,"%.*f<",gludecimal,gconvert(glucosehighest*10));
				 int gllen=mkshowhigh(head, maxhead) ;
					nvgText(vg,valuex ,gety, head, head+gllen);
					}
				else {
#if 0
					const float convglucose= 27.8f;
#else
					const float convglucose= gconvert(nonconvert*10);
#endif
					float valuex=getx-(convglucose>=10.0f?density*20.0f:0.0f);
					int gllen=snprintf(head,maxhead,gformat,convglucose);
					nvgText(vg,valuex ,gety, head, head+gllen);
					float rate=poll->ch;
					drawarrow(vg,rate,valuex-10*density,gety);
					}
				}

			}

	
//static bool	streamvalueshown=false;
bool bluetoothEnabled();
float				getboxwidth(const float x) {
					return std::max((float)(dwidth-x-smallsize),dwidth*.25f);
					}

//#define DOTEST 1
static int showerrorvalue(const SensorGlucoseData *sens,const time_t nu,float getx,float gety) {
	//nvgTextAlign(vg,NVG_ALIGN_LEFT|NVG_ALIGN_TOP);
	getx-=headsize/3;
	nvgTextAlign(vg,NVG_ALIGN_LEFT|NVG_ALIGN_MIDDLE);
	//nvgFontSize(vg,headsize/4 );
	nvgFontSize(vg,headsize/6 );
	if(settings->data()->nobluetooth) {
		extern bool hasnetwork();
		if(hasnetwork()) {
			return 1;
		//	static	constexpr const std::string_view network="Network problem?";
		//	nvgText(vg,getx ,gety, network.begin(), network.end());
			}
		else {
//			static	constexpr const std::string_view useblue="'Use Bluetooth' off";
//			nvgText(vg,getx ,gety, useblue.begin(), useblue.end());
			return 2;
			}
		}
	else {
		/*(
		time_t starttime=sens->getstarttime();
		auto wait= nu-starttime;
		LOGGER("wait=%u starttime=%" PRId64 " %s",wait,starttime,ctime(&starttime));
		if(wait<(60*60)) {
			const char format[]="%s: Sensor ready in %d minutes";
			char buf[sizeof(format)+5+16];
			int minutes=wait/60;
			int ends=sprintf(buf,format,sens->showsensorname(),minutes);
			nvgTextBox(vg,  getx, gety, 0.4*dwidth, buf,buf+ends);
			return 0;
			}
		else*/ {
			if(!bluetoothEnabled()) {
				return 3;
			//	static	constexpr const std::string_view enablebluetooth="Enable Bluetooth";
			//	nvgText(vg,getx ,gety, enablebluetooth.begin(), enablebluetooth.end());
				} 
			else {
				if(sens->sensorerror) {
					static	const std::string_view sensorerror= sens->replacesensor?usedtext->streplacesensor: usedtext->stsensorerror;
					char buf[sensorerror.size()+17];
					int senslen= sens->showsensorname().size();
					memcpy(buf,sens->showsensorname().data(),senslen);
					memcpy(buf+senslen,sensorerror.data(), sensorerror.size());
					nvgTextBox(vg,  getx, gety, getboxwidth(getx), buf, buf+sensorerror.size()+senslen);
					}
				else {
				//	nvgText(vg,getx ,gety, connectionerror.begin(), connectionerror.end());
					char buf[usedtext->noconnectionerror.size()+17];
					int senslen= sens->showsensorname().size();
					memcpy(buf,sens->showsensorname().data(),senslen);
					memcpy(buf+senslen,usedtext->noconnectionerror.data(), usedtext->noconnectionerror.size());
					nvgTextBox(vg,  getx, gety, getboxwidth(getx),buf, buf+usedtext->noconnectionerror.size()+senslen);
					}
				return 0;
				}
			}
	
		}
	}
static void showlastsstream(const time_t nu,const float getx,std::vector<int> &used ) {
//LOGGER("showlaststream %d\n",used.size());
	int success=false;
	bool neterror=false,usebluetoothoff=false,bluetoothoff=false,otherproblem=false;
	static int failures=0;
	++failures;
	for(int i=0;i<used.size();i++) {
		const int sensorindex=used[i];
		const SensorGlucoseData *hist=sensors->gethist(sensorindex);
		int yh=i*2+1;
		float gety=smallsize*.5f+dtop+dheight*yh/(used.size()*2.0f);
		const ScanData *poll=hist->lastpoll();
		if(poll) {
			LOGGER("poll!=null\n");
			int age=nu-poll->t;
			if(age<maxbluetoothage) {
				LOGGER("age<maxbluetoothage\n");
				if(!poll->valid())
					return;
				failures=0;
				nvgBeginPath(vg);
				 nvgFillColor(vg,getoldcolor());
				float relage=(float)age/(float)maxbluetoothage;
				float sensory= gety+headsize/3.1f;
				nvgRect(vg, getx+sensorbounds.left, sensorbounds.top+sensory, relage*sensorbounds.width, sensorbounds.height);
				nvgFill(vg);
				showvalue(poll,hist->shortsensorname(),getx,gety);
				success=true;
				}
			else {
				LOGGER("age>=maxbluetoothage\n");
				switch(showerrorvalue(hist,nu,getx,gety)) {
					case 1: neterror=true;break;
					case 2: usebluetoothoff=true;break;
					case 3: bluetoothoff=true;break;
					default: otherproblem=true;
					};
				}
			}
		else {
			LOGGER("poll==null\n");
			time_t starttime=hist->getstarttime();
			auto wait= nu-starttime;
			LOGGER("wait=%lu starttime=%lu %s\n",wait,starttime,ctime(&starttime));
			if(wait<(60*60)) {
			//	const bool streaming=hist->deviceaddress()[0];
				const bool isInitialised=hist->isLibre3()||sensors->getsensor(sensorindex)->initialized;
				LOGGER("wait<(60*60) isInitialised=%d\n",isInitialised);

				float usegetx=getx-headsize/3;
				nvgTextAlign(vg,NVG_ALIGN_LEFT|NVG_ALIGN_MIDDLE);
				nvgFontSize(vg,headsize/6 );
				char buf[usedtext->readysecEnable.size()+6];
				int minutes=60-(wait/60);
				int ends=sprintf(buf,isInitialised?usedtext->readysec.data():usedtext->readysecEnable.data(),minutes);
				getboxwidth(usegetx);
				nvgTextBox(vg,  usegetx, gety, getboxwidth(usegetx), buf,buf+ends);
				}
			else
				LOGGER("wait>=(60*60)\n");
			}

		}
	if(!success&&!otherproblem) {
		int newgetx=getx-headsize/3;
		nvgTextAlign(vg,NVG_ALIGN_LEFT|NVG_ALIGN_MIDDLE);
		nvgFontSize(vg,headsize/4 );
		float gety=smallsize*.5f+dtop+dheight/2.0f;
		if(neterror) {
			nvgText(vg,newgetx ,gety, usedtext->networkproblem.begin(), usedtext->networkproblem.end());
			}
		else { if(usebluetoothoff) {
		   nvgText(vg,newgetx ,gety, usedtext->useBluetoothOff.begin(), usedtext->useBluetoothOff.end());
		   }
		   else {
		   	if(bluetoothoff) {
				nvgText(vg,newgetx ,gety, usedtext->enablebluetooth.begin(), usedtext->enablebluetooth.end());
				}
				}
				}
		}
	if(failures>3) {
		for(int i=0;i<used.size();i++) {
			if(SensorGlucoseData *hist=sensors->gethist(used[i])) {
				LOGGER("set waiting=true\n");
                		hist->waiting=true;
				}
			}
		}

	}



void	showbluevalue(const time_t nu,const int xpos,std::vector<int> &used) {
LOGGER("showbluevalue %zd\n",used.size());
		nvgFontSize(vg, smallsize);
		nvgFillColor(vg, *getblack());

		nvgBeginPath(vg);
		nvgStrokeColor(vg, dooryellow);
		nvgStrokeWidth(vg, nowLineStrokeWidth);
		nvgMoveTo(vg,xpos ,dtop) ;
		nvgLineTo( vg, xpos,dheight+dtop);
		nvgStroke(vg);
		#ifndef WEAROS
		{
		float down=0;

		const float timex=xpos+nowLineStrokeWidth;
		nvgTranslate(vg, timex,down);
		nvgRotate(vg,-NVG_PI/2.0);
		constexpr int maxhead=54;
		char head[maxhead];
		memcpy(head,usedtext->sensorends.data(),usedtext->sensorends.size());

		if(time_t enddate=lastsensorends()) {
		const int tstart=usedtext->sensorends.size();
			char *endstr=head+tstart;
			int end= datestr(enddate,endstr); 
			nvgTextAlign(vg,NVG_ALIGN_CENTER|NVG_ALIGN_BOTTOM);
			nvgText(vg, -dheight/2+down-smallfontlineheight,dwidth-timex, std::begin(head), head+end+tstart);
			}
		nvgResetTransform(vg);
		}
		#endif
		const float getx= xpos+headsize*.9f+8*dwidth/headsize;

		const float datehigh=smallfontlineheight*.72;
		
		nvgTextAlign(vg,NVG_ALIGN_LEFT|NVG_ALIGN_TOP);
		{
		constexpr int maxbuf=80;
		char tbuf[maxbuf];
           largedaystr(nu,tbuf) ;
		const float timex =
			getx
		#ifdef WEAROS
			-timelen
		#endif
		;
		nvgText(vg, timex,datehigh, tbuf, NULL);
		LOGGER("xpos=%d dwidth=%.1f headsize=%.1f density=%.1f getx=%.1f timex=%.1f\n",xpos,dwidth,headsize, density,getx,timex);
		}
	showlastsstream(nu, getx,used) ;
		}

void	showsavedomain(const float last, const float dlow,const float dhigh) {
	nvgBeginPath(vg);
	nvgFillColor(vg, unsavecolor);
	nvgRect(vg, dleft, dtop, last-dleft, dhigh);
	nvgFill(vg);

	nvgBeginPath(vg);
	nvgFillColor(vg, unsavecolor);
	nvgRect(vg, dleft, dlow, last-dleft, dheight+dtop);
	nvgFill(vg);
	}
void showunsaveredline(const float last,const float dlow) {
	nvgBeginPath(vg);
	nvgStrokeWidth(vg, lowGlucoseStrokeWidth);

	nvgStrokeColor(vg, lowlinecolor);
	nvgMoveTo(vg, dleft,dlow) ;
	nvgLineTo( vg,last ,dlow);
	nvgStroke(vg);
	}
void	showsaverange(const float last, const float dlow,const float dhigh) {
	showsavedomain(last,dlow,dhigh) ;
	showunsaveredline(last,dlow) ;
	}
		
void 	showdates(time_t nu,uint32_t starttime,time_t endtime) {
   LOGGER("duration=%d\n",duration);
	int32_t timdis=nu-starttime;
constexpr const int grens=
#ifdef WEAROS
1
#else
3
#endif
;
LOGGER("timdis=%d duration=%d grens=%d\n",timdis,duration,grens);
if(timdis>0&&((duration/timdis)<grens)) {
	   LOGGER("timdis=%d larger than zero\n",timdis);
		const float datehigh=smallfontlineheight*
#ifdef WEAROS
		.71;
#else
		1.5;
		#endif

		char tbuf[50];
uint32_t showtime=
	#ifdef WEAROS
	(endtime+starttime)/2
	#else
	starttime
	#endif
	;

		daystr(showtime,tbuf);
		nvgFillColor(vg, *
		#ifdef WEAROS
		getdarkgray()
		#else
		getblack()
		#endif
		);
	float xpos;
	#ifdef WEAROS
		xpos= dwidth/2+dleft;
		nvgTextAlign(vg,NVG_ALIGN_CENTER|NVG_ALIGN_TOP);
	#else
		xpos= settings->data()->levelleft?timelen*.75:0;
		nvgTextAlign(vg,NVG_ALIGN_LEFT|NVG_ALIGN_TOP);
	#endif

		LOGGER("displaytime %s\n",tbuf);
		nvgText(vg,xpos ,datehigh, tbuf, NULL);
#ifndef WEAROS
		if(nu>=endtime) {
			daystr(endtime,tbuf);
			nvgTextAlign(vg,NVG_ALIGN_RIGHT|NVG_ALIGN_TOP);
			nvgText(vg, dwidth+dleft,datehigh, tbuf, NULL);
			}
#endif
		}
	}
auto gettrans(uint32_t starttime,uint32_t endtime) {
	double interval=endtime-starttime;
	const double usedleft=0.0;
	const double usedwidth=dwidth-2*usedleft;
	const double usedtop=pointRadius;
	const double usedheight=dheight-2*usedtop;
	const int gmax=gmin+grange;
	const double xscale=usedwidth/interval,xmove=usedleft-((double)starttime)*usedwidth/interval;
	const double yscale= -usedheight/grange,ymove= usedtop+usedheight*gmax/grange;

	const auto transx=[xscale,xmove](uint32_t x) {return x*xscale + xmove;};
       	const auto transy=[yscale,ymove](uint32_t y) {return y*yscale + ymove;};
//	return pair<decltype(transx),decltype
	return make_pair(transx,transy);
	}
void showlines(int gm,int gmax) {
		uint32_t endtime=starttime+duration;
		gmin=gm;
		grange=gmax-gmin;
		const auto [transx,transy]= gettrans(starttime, endtime);
	       displaytime disp=getdisplaytime(UINT_MAX,starttime,endtime, transx);
		const float dlast=dleft+dwidth;
		timelines(&disp,  transx,UINT32_MAX);
		if(disp.tstep>(60*60))
			epochlines(starttime,disp.last,transx);
		glucoselines(dlast,smallfontlineheight,gmax,transy) ;
		showunsaveredline(dlast,transy(settings->targetlow()));
		int yhigh=transy(settings->targethigh());
		nvgBeginPath(vg);
		nvgStrokeWidth(vg, lowGlucoseStrokeWidth);
		nvgStrokeColor(vg, dooryellow);
		nvgMoveTo(vg, dleft,yhigh) ;
		nvgLineTo( vg,dwidth,yhigh);
		nvgStroke(vg);
		}
		

pair<const ScanData *,const ScanData*> *scanranges=nullptr;
pair<const ScanData *,const ScanData*> *pollranges=nullptr;
pair<int32_t,int32_t> *histpositions=nullptr;
int histlen=0;
vector<int> hists;

extern bool iswatch;
int displaycurve(NVGcontext* vg,time_t nu,uint32_t starttime,uint32_t endtime) {
//	if(iswatch) { return showwatchface(nu); }

	mealpos.clear();
	LOGGER("display\n");
	hists= sensors->inperiod(starttime,endtime) ;
	histlen=hists.size();
	delete[] scanranges;
	scanranges=new pair<const ScanData *,const ScanData*> [histlen];
	delete[] pollranges;
	pollranges=new pair<const ScanData *,const ScanData*> [histlen];
//	pair<const ScanData *,const ScanData*> scanranges[histlen];
//	pair<const ScanData *,const ScanData*> pollranges[histlen];
	delete[] histpositions;
	histpositions=new std::remove_reference_t<decltype(histpositions[0])>[histlen];
	LOGGER("before getranges\n");
	for(int i=histlen-1;i>=0;--i) {
		auto his=sensors->gethist(hists[i]);
		if(!his)  {
			LOGGER("gethist==null\n");
			sleep(1);
			return 0;
			}
        	LOGGER("%s\n",his->shortsensorname()->data());
		std::span<const ScanData> 	scan;
		//if(showscans) 
		{
			scan=his->getScandata();
			scanranges[i] =getScanRange(scan.data(),scan.size(),starttime,endtime) ;
			}
		//if(showstream) 
		{
			scan=his->getPolldata();
			pollranges[i] =getScanRangeRuim(scan.data(),scan.size(),starttime,endtime) ;
			}
//		if(showhistories)
			histpositions[i]= histPositions(his, starttime,  endtime); 
		 }
	LOGGER("Before numdatas[i]->getInRange(%u,%u)\n",starttime,endtime);
	for(int i=0;i< numdatas.size();i++) 
		numdatas[i]->extrum=numdatas[i]->getInRange(starttime, endtime) ;
	const pair<const ScanData *,const ScanData*> *scanpoll[]= {scanranges,pollranges};
	LOGGER("Before getextremes\n");
	if((setend<starttime||settime>=endtime)) {
	   auto extr=getextremes(hists,scanpoll,2,histpositions);
	   for(int i=0;i<numdatas.size();i++)  {
	   	       LOGGER("%d before extremenums \n",i);
			extr  = numdatas[i]->extremenums(extr);
			}
	   setextremes(extr) ;
	   }
	LOGGER("before gettrans\n");
	int  gmax = gmin+grange;
	const auto [transx,transy]= gettrans(starttime, endtime);
displaytime disp=getdisplaytime(nu,starttime,endtime, transx);
	const float dlast=nu<endtime?transx(disp.last):dleft+dwidth;
	LOGGER("before showsaverange\n");
	showsaverange(dlast,transy(settings->targetlow()),transy(settings->targethigh()));

	nvgFontSize(vg, smallsize);
	LOGGER("before showNums\n");
	if(shownumbers||showmeals)  {
		const int catnr=settings->getlabelcount()+1;
		bool was[catnr-1];
		memset(was,0,sizeof(was));
		for(auto el:numdatas) 
			el->showNums(vg, transx,  transy,was) ;
		}

	showdates(nu,starttime,endtime) ;

	int nupos=transx(nu); 
	timelines(&disp,  transx,nu);
	if(disp.tstep>(60*60))
		epochlines(starttime,endtime<nu?endtime:disp.last,transx);
	glucoselines(dlast,smallfontlineheight,gmax,transy) ;

//		nvgCircle(vg, posx,posy,foundPointRadius);

	LOGGER("before showhistories\n");
	if(showhistories) {
		nvgStrokeWidth(vg, historyStrokeWidth);
		for(int i=histlen-1;i>=0;i--) {
			int index= hists[i];
			int colorindex= (index+nrcolors*3/4)%nrcolors;
			 histcurve(vg,sensors->gethist(index), histpositions[i].first, histpositions[i].second,transx,transy,colorindex); 
			 }
		}
	LOGGER("before showstream\n");
	if(showstream)   {
		nvgStrokeWidth(vg, pollCurveStrokeWidth);
		for(int i=histlen-1;i>=0;i--) {
			const int index= hists[i];
		//	decltype(auto) col=*colors[(index+nrcolors/4)%nrcolors];
			int  colorindex=(index+nrcolors/4)%nrcolors;
			showlineScan(vg,pollranges[i].first,pollranges[i].second,transx,transy,colorindex);
			 }
		}
	LOGGER("before showscans\n");
	if(showscans) {
		for(int i=histlen-1;i>=0;i--) {
			const int index=hists[i];
			const int colorindex=(index+nrcolors*2/4)%nrcolors;
			 if(!showScan(vg,scanranges[i].first,scanranges[i].second,transx,transy,colorindex))
				return 1;
			 }
		 }

	if(nu<endtime) {
		if((dwidth-smallfontlineheight)>nupos) {
			showbluevalue(nu, nupos,usedsensors);
			}
		}
 return 0;
}
/*
extern "C" JNIEXPORT void JNICALL fromjava(setglucose) (JNIEnv *env, jclass clazz,jlong time,jfloat glu,jstring jsensor,jint index) {
	if(index<sensornr) {
		env->GetStringUTFRegion( jsensor, 0, shortsensorlen,sensorname[index]);
		sensorname[index][shortsensorlen]='\0';
		glucosetime[index]=time;	
		glucose[index]=glu;
		}
	}
	*/

static void startstep(const NVGcolor &col) {
		glViewport(0, 0, width, height);
		glClearColor(col.r,col.g, col.b, col.a);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		nvgBeginFrame(vg, width, height, 1.0);
		if(invertcolors)
			font=whitefont;
		else
			font=blackfont;
		nvgFontFaceId(vg,font);
		nvgLineCap(vg, NVG_ROUND);
 		nvgLineJoin(vg, NVG_ROUND);
		}
static void endstep() {
    nvgEndFrame(vg);
    glEnable(GL_DEPTH_TEST);
}
/*
static bool doclear=false;
extern "C" JNIEXPORT void JNICALL fromjava(clear)(JNIEnv* env, jclass obj) {
	doclear=true;
	}
	*/

extern "C" JNIEXPORT jlong JNICALL fromjava(sensorends)(JNIEnv* env, jclass obj) {
	return  lastsensorends() ;
	}
bool restart=false;
static bool showoldscan(NVGcontext* vg) ;

static void defaulterror(NVGcontext* vg,int scerror)   {
		char buf[50];
		errortype *error=usedtext->scanerrors;
		snprintf(buf,50,error->first,scerror);
		showerror(vg,error->second,buf);
		}
extern "C" JNIEXPORT jint JNICALL fromjava(badscan)(JNIEnv* env, jclass obj,jint kind) {
	if(!showoldscan(vg)) {
		LOGGER("javabadscan	%d: \n",kind);
		const int scerror= kind&0xff;
		switch(scerror) {
			case 0xFA: {
				showerror(vg,"FreeStyle Libre 3, Scan error", "Try again");
				};
				break;
			case 0xFB:
				showerror(vg,"Error, wrong account ID?","Specify in Settings->Libreview the same account used to activate the sensor");break;
		 	case 0xFC: {
				showerror(vg,"FreeStyle Libre 3 sensor", "Glucose values will now be received by Juggluco");
				};break;
			case 0xFD: {
				showerror(vg,"Unrecognized NFC scan Error", "Try again");
				};break;
			case 0xFE: {
				showerror(vg,"FreeStyle Libre 3 sensor","Not supported by this version of Juggluco"  );
				};break;
			case 0xff: {
				scanwait(vg); 	
				return 2;
				};
			case 5: {
				errortype *error=usedtext->scanerrors+scerror;
				char buf[15];
				snprintf(buf,15,error->second,kind>>8);
				showerror(vg,error->first,buf);
				LOGGER("%s\n",buf);
				};break;
			case 0:
			case 15:
			case 9: defaulterror(vg,scerror);
				break;
			case 12: restart=true;
			default: 
			 if(scerror>0x10) {
				defaulterror(vg,scerror);
				}
			else {
				errortype *error=usedtext->scanerrors+scerror;
				showerror(vg,error->first,error->second);
				}
			}
		}
	endstep();	
	return 1;
	}
	/*
extern "C" JNIEXPORT void JNICALL fromjava(showlast)(JNIEnv* env, jclass obj) {
	if(showoldscan(vg))
		endstep();	
}
*/
static int nrmenu=0,selmenu=0;

static void showtext(NVGcontext* vg ,time_t nu,int tapx) ;


struct lastscan_t scantoshow={-1,nullptr}; 

static bool showoldscan(NVGcontext* vg) {
	if(scantoshow.scan) {
		numlist=0;
	      const SensorGlucoseData *hist=sensors->gethist(scantoshow.sensorindex);
	      showscanner(vg,hist,scantoshow.scan-hist->beginscans()) ;
	      return true;
		}
	return false;
	}
int getmenu(int tapx) ;
#include "displayer.h"
std::unique_ptr<Displayer> displayer;

extern void mkheights() ;
void	updateusedsensors(uint32_t nu) {
	static int wait=0;
	static int32_t waslast=-1;
	int newlast=sensors->last();
	if(waslast!=newlast||!wait) {
		LOGGER("updateusedsensors\n");
		waslast=newlast;
		setusedsensors(nu);
		wait=100;
		mkheights(); 
		}
	else
		wait--;
	
	}

//__attribute__((__visibility__("default"))) extern bool skipdisplay;
//bool skipdisplay=true;

//#define WEAROS
#ifndef WEAROS
extern bool showpers;
extern void showpercentiles(NVGcontext* vg) ;
#endif
extern "C" JNIEXPORT void JNICALL fromjava(calccurvegegs)(JNIEnv *env, jclass clazz);
void resetcurvestate() {
   displayer.reset();
  scantoshow={-1,nullptr}; 
    numlist=0;
#ifndef WEAROS
    showpers=false;
#endif
 selshown=false;
nrmenu=0;
 selmenu=0;
 emptytap=false;
 nrmenu=0,selmenu=0;
  fromjava(calccurvegegs)(nullptr, nullptr);
    }
void withredisplay(NVGcontext* vg,uint32_t nu,uint32_t endtime)  {
/*
if(rotation!=0.0) {
	LOGGER("rotate %f\n",rotation);
	nvgTranslate(vg, dheight/2.0f,dwidth/2.0f);
	nvgRotate(vg,rotation);
	} */
    startstep(*getwhite());
#ifndef WEAROS
    if(showpers) {
		showpercentiles(vg);
		}
	else 
#endif
	{

#ifdef WEAROSx
int oldtapx=tapx;
tapx=-8000;
#endif
	    if( !displaycurve(vg,nu,starttime, endtime)&&( ((tapx
#ifdef WEAROSx

	    =oldtapx
#endif
	    		)>=0&&!selshown&&(selmenu=getmenu(tapx),true))||nrmenu)) {
		  showtext( vg ,nu,selmenu) ;
		   }
		}
	tapx=-8000;

}
/*
void withoutredisplay(NVGcontext* vg,uint32_t nu,uint32_t endtime)  {

	    if( (tapx>=0&&!selshown&&(selmenu=getmenu(tapx),true))||nrmenu) 
			  showtext( vg ,nu,selmenu) ;
	 else  {
		    startstep(*getwhite());
		    if(showpers) {
				showpercentiles(vg);
				}
		else
			displaycurve(vg,nu,starttime, endtime);
		}
	tapx=-8000;
} */
static jint onestep() {
	LOGGER("onestop\n");
//	if(skipdisplay) return 0;
	time_t nu=time(nullptr);
	updateusedsensors(nu);
	uint32_t endtime=starttime+duration;
	selshown=false;
	int ret=0;
	emptytap=false;

	void		shownumlist();
	if(showoldscan(vg)) {
		ret=1;
		}
	else {
		if(numlist) {
			shownumlist();
			}
		else {
			withredisplay(vg,nu,endtime);
		}
	}
	if(displayer)
		ret=displayer->display();
	endstep();	
	return ret;
	}

extern void render() ;

jobject glucosecurve=0;
/*
void requestRender() {
	if(glucosecurve!=0) {
		env->CallVoidMethod(glucosecurve,requestRendermeth);
		}
	}

	jmethodID requestRendermeth=env->GetMethodId(env->FindClass("android/opengl/GLSurfaceView"),"requestRender" "()V");
	env->CallVoidMethod(glsurface,requestRender);
	}
NewGlobalRef
*/
extern "C" JNIEXPORT void  JNICALL   fromjava(setpaused)(JNIEnv *env, jclass cl,jobject val) {
	if(glucosecurve)
		env->DeleteGlobalRef(glucosecurve);
	if(val)
		glucosecurve=env->NewGlobalRef(val);
	else
		glucosecurve=nullptr;
	}
#ifndef NOJAVA
JavaVM *vmptr;
static jmethodID summaryready=nullptr;

	#ifdef  WEAROS
static jmethodID showsensorinfo=nullptr;
#endif
jmethodID  jdoglucose=nullptr, jupdateDevices=nullptr, jbluetoothEnabled=nullptr;
jclass JNIApplic;
#ifdef OLDXDRIP
#ifndef  WEAROS
jclass XInfuus;
jmethodID  sendGlucoseBroadcast=nullptr;
#endif
#endif

extern void initlibreviewjni(JNIEnv *env);

extern bool jinitmessages(JNIEnv* env);

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
	LOGGER("JNI_OnLoad\n");
	vmptr=vm;
	   JNIEnv* env;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
      }

{
const jclass cl=env->FindClass("tk/glucodata/GlucoseCurve");
if(!cl) {
	summaryready=nullptr;
	LOGGER("Can't find GlucoseCurve\n");
	}
else {
	LOGGER("found GlucoseCurve\n");
	summaryready=env->GetMethodID(cl,"summaryready","()V");
	#ifdef  WEAROS
	showsensorinfo=env->GetMethodID(cl,"showsensorinfo","(Ljava/lang/String;)V");
	#endif
	env->DeleteLocalRef(cl);
	}

}

{
const static jclass cl=env->FindClass("tk/glucodata/Applic");
if(cl) {
	JNIApplic = (jclass)env->NewGlobalRef(cl);
	env->DeleteLocalRef(cl);
	if(!(jdoglucose=env->GetStaticMethodID(JNIApplic,"doglucose","(Ljava/lang/String;IFFIJZ)V"))) {
		LOGGER(R"(GetStaticMethodID(JNIApplic,"doglucose","(Ljava/lang/String;IFFIJZ)V"))) failed)" "\n");
		}
	if(!(jupdateDevices=env->GetStaticMethodID(JNIApplic,"updateDevices","()Z"))) {
		LOGGER(R"(jupdateDevices=env->GetStaticMethodID(JNIApplic,"updateDevices","()Z") failed)" "\n");
		}
	if(!(jbluetoothEnabled=env->GetStaticMethodID(JNIApplic,"bluetoothEnabled","()Z"))) {
		LOGGER(R"(jbluetoothEnabled=env->GetStaticMethodID(JNIApplic,"bluetoothEnabled","()Z") failed)" "\n");
		}
	}
else {
	LOGGER(R"(FindClass("tk/glucodata/Applic") failed)" "\n");
	}
}


#ifdef OLDXDRIP
#ifndef  WEAROS
{
const static jclass cl=env->FindClass("tk/glucodata/XInfuus");
if(cl) {

	XInfuus = (jclass)env->NewGlobalRef(cl);
	env->DeleteLocalRef(cl);
	if(!(sendGlucoseBroadcast=env->GetStaticMethodID(XInfuus,"sendGlucoseBroadcast","(Ljava/lang/String;DFJ)V"))) {
		LOGGER(R"(GetStaticMethodID(XInfuus,"sendGlucoseBroadcast","(Ljava/lang/String;DFJ)V()) failed)" "\n");
		}
	}
else {
	LOGGER(R"(FindClass("tk/glucodata/XInfuus") failed)" "\n");
	}
	}
#endif
#endif
	/*
const static jclass clappl=env->FindClass("tk/glucodata/Applic");
jclass Applic=nullptr;
jmethodID jupdatescreen=nullptr;
if(clappl) {
	Applic= (jclass)env->NewGlobalRef(clappl);
	env->DeleteLocalRef(clappl);
	jupdatescreen=env->GetStaticMethodID(Applic,"updatescreen","()V");
	}
bool loadglucoseclass(JNIEnv *env);
if(loadglucoseclass(env)) {
	LOGGER("end JNI_OnLoad\n");
	 return JNI_VERSION_1_6;
	 }
return JNI_ERR;
*/
#ifndef WEAROS
initlibreviewjni(env);
#endif


#ifdef WEAROS_MESSAGES
jinitmessages(env) ;
#endif

	LOGGER("end JNI_OnLoad\n");
	 return JNI_VERSION_1_6;
}
class attach {
JNIEnv *env;
public:
attach() {
//	vmptr->AttachCurrentThread(&env, nullptr);
	vmptr->AttachCurrentThreadAsDaemon(&env, nullptr);
	}
~attach() {
	vmptr->DetachCurrentThread();
	}
[[nodiscard]]   JNIEnv *get() const {
	return env;
	}
};



JNIEnv *getenv() {
	const thread_local static attach  env;
	return env.get();
	}
bool bluetoothEnabled() {
    return   getenv()->CallStaticBooleanMethod(JNIApplic,jbluetoothEnabled);
    }
//static void     doglucose(String SerialNumber,float gl,float rate,int alarm,long timmsec) {

//struct ScanData {uint32_t t;int32_t id;int32_t g;int32_t tr;float ch;

	extern void	wakeuploader();
int getalarmcode(const uint32_t glval,SensorGlucoseData *hist) ;
extern void     processglucosevalue(int sendindex,int newstart) ;
void     processglucosevalue(int sendindex,int newstart) {
//	if(!streamvalueshown) return;
extern	bool hasnotiset();
	if(settings) {
		if(!sensors)
			return;
		if(SensorGlucoseData *hist=sensors->gethist(sendindex)) {
			if(newstart>=0) {
				LOGGER("newstart=%d\n",newstart);
				hist->backstream(newstart);
				}
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


					const float glu= gconvert(poll->g*10);
					const int alarm=getalarmcode(poll->g,hist);
					
					sensor *senso=sensors->getsensor(sendindex);
					LOGGER("processglucosevalue finished=%d,doglucose(%s,%d,%f,%f,%d,%lld)\n", senso->finished,hist->shortsensorname()->data(),poll->g,glu,poll->ch,alarm,tim*1000LL);
					senso->finished=0;
					jstring sname= getenv()->NewStringUTF(hist->shortsensorname()->data());
				        bool wasnoblue=settings->data()->nobluetooth;
					settings->data()->nobluetooth=true;
					float rate=poll->ch;
//					if(glnearnull(rate)) rate=0.0f;
					getenv()->CallStaticVoidMethod(JNIApplic,jdoglucose,sname,poll->g,glu,rate,alarm,tim*1000LL,wasnoblue);
					getenv()->DeleteLocalRef(sname);
			#ifndef WEAROS
			wakeuploader();
			#endif

					}
				else {
					LOGGER("processglucosevalue too old %s ",ctime(&tim));
					LOGGER("dist=%d, dif=%d nu %s",maxbluetoothage,dif,ctime(&nutime));
					}
				}
			}
		else {
			LOGGER("processglucosevalue no sensor %d\n",sendindex);
			}
		}

	}

bool updateDevices() {
    if(!jupdateDevices)  {
    	LOGGER("jupdateDevices==null\n");

    		return false;
		}
    return   getenv()->CallStaticBooleanMethod(JNIApplic,jupdateDevices);
    }
void visiblebutton() {
	if(glucosecurve) {
		if(summaryready)  {
			JNIEnv *env =getenv(); 
			LOGGER("call summaryready\n");
			env->CallVoidMethod(glucosecurve,summaryready);
			}
		else
			LOGGER("didn't find GlucoseCurve\n");
		}
	}
	#ifdef  WEAROS
void callshowsensorinfo(const char *text) {
	if(glucosecurve) {
		if(showsensorinfo)  {
			JNIEnv *env =getenv(); 
			LOGGER("call showsensorinfo\n");
			env->CallVoidMethod(glucosecurve,showsensorinfo,env->NewStringUTF(text));
			}
		else
			LOGGER("didn't find GlucoseCurve\n");
		}
	}
#endif

void render() {
	LOGGER("Render\n");
	if(glucosecurve) {
		struct method {
		   jmethodID requestRendermeth;
		   method(JNIEnv *env) {	
		        jclass	cl=env->FindClass("android/opengl/GLSurfaceView");
		        requestRendermeth=env->GetMethodID(cl,"requestRender","()V");
			env->DeleteLocalRef(cl);
		   	};
		};
		static method meth(getenv());
//		static jmethodID requestRendermeth=getenv()->GetMethodID(getenv()->FindClass("android/opengl/GLSurfaceView"),"requestRender","()V");
		getenv()->CallVoidMethod(glucosecurve,meth.requestRendermeth);
		}
//	onestep();
	}
#endif
extern "C" JNIEXPORT jint JNICALL fromjava(step)(JNIEnv* env, jclass obj) {
	return onestep();
	}

       #include <unistd.h>
          #include <sys/types.h>
       #include <sys/stat.h>
       #include <fcntl.h>

#include <string.h>
//extern string_view filesdir;

extern std::string_view globalbasedir;
extern pathconcat numbasedir;
extern pathconcat sensorbasedir;
extern pathconcat logbasedir;


//int  abbottinit(std::string_view filesdir,JNIEnv *env=nullptr,jobject thiz=nullptr);
//int  abbottinit(std::string_view filesdir,JNIEnv *env=nullptr,jobject thiz=nullptr);
extern int  abbottinit();
void mkheights() {
	if(!settings)
		return;
	LOGGER("mkheights() \n");
	const int maxl= settings->getlabelcount();
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

#include "net/backup.h"
#include "datbackup.h"
extern void setuseit();
extern void setusenl();

extern void setusepl();
extern void setusede();
extern void setusept() ;
extern void setuseeng() ;
extern int setfilesdir(const string_view filesdir,const char *country) ;
extern std::string_view localestr;
extern bool hour24clock;
char localestrbuf[10];
std::string_view localestr;
bool hour24clock=true;

#define mklanguagenum2(a,b) a|b<<8
//#define mklanguagenum(lang) *reinterpret_cast<const int16_t *>(lang)
#define mklanguagenum(lang) mklanguagenum2(lang[0],lang[1])
extern "C" JNIEXPORT void JNICALL fromjava(setlocale)(JNIEnv *env, jclass clazz,jstring jlocalestr,jboolean hour24) {
	hour24clock=hour24;
	if(jlocalestr) {
		size_t len=env->GetStringLength(jlocalestr);
		env->GetStringUTFRegion( jlocalestr, 0,len, localestrbuf);
		localestrbuf[len]='\0';
		LOGGER("locale=%s\n",localestrbuf);
		localestr={localestrbuf,len};
		const int16_t lannum=mklanguagenum(localestrbuf);
//		const int16_t lannum=*reinterpret_cast<const int16_t *>(localestrbuf);
		switch(lannum) {
			case mklanguagenum("DE"):
			case mklanguagenum("de"):
				setusede();
				break;
			case mklanguagenum("IT"):
			case mklanguagenum("it"):
				setuseit();
				break;
			case mklanguagenum("NL"):
			case mklanguagenum("nl"):
				setusenl();
				break;
			case mklanguagenum("PT"):
			case mklanguagenum("pt"):
				setusept();
				break;
			case mklanguagenum("PL"):
			case mklanguagenum("pl"):
				setusepl();
				break;
			default: setuseeng();
			};
		}

	}

extern std::string_view libdirname;
extern "C" JNIEXPORT int JNICALL fromjava(setfilesdir)(JNIEnv *env, jclass clazz, jstring dir,jstring jcountry,jstring nativedir) {
	{
	size_t nativedirlen= env->GetStringUTFLength( nativedir);
	char *nativebuf=new char[nativedirlen+1];
	env->GetStringUTFRegion( nativedir, 0,nativedirlen,nativebuf);
	nativebuf[nativedirlen]='\0';
	libdirname={nativebuf,nativedirlen};
	}
	size_t filesdirlen= env->GetStringUTFLength( dir);
	jint jdirlen = env->GetStringLength( dir);
	char *filesdirbuf=new char[filesdirlen+1];
	env->GetStringUTFRegion( dir, 0,jdirlen, filesdirbuf);
	filesdirbuf[filesdirlen]='\0';
	char country[3];
	if(jcountry&& env->GetStringLength(jcountry)>=2) {
		env->GetStringUTFRegion( jcountry, 0,2, country);
		country[2]='\0';
		}
	else
		country[0]='\0';
	return setfilesdir({filesdirbuf,filesdirlen},country);
	}
extern "C" JNIEXPORT void JNICALL fromjava(calccurvegegs)(JNIEnv *env, jclass clazz) {
	LOGGER("start calccurvegegs\n");
	mkheights(); 
	starttime=maxtime()-4*duration/5;
	setusedsensors();
	LOGGER("end calccurvegegs\n");
	}

void		numendbegin() ;
extern "C" JNIEXPORT void JNICALL fromjava(flingX) (JNIEnv *env, jclass clazz,jfloat vol) {
	if(numlist)  {
		LOGGER("flingX\n");
		if(vol<0) {
			numlist=1;
 			numendbegin() ;
			}
		return;
		}
	starttime-=(duration*1.2*vol/dwidth);
#ifndef WEAROS
	if(!showpers)
#endif
		begrenstijd() ;
	}

bool				numpageforward() ;
			void	scrollnum() ;


bool numpagepast() ;
extern "C" JNIEXPORT jint JNICALL fromjava(translate) (JNIEnv *env, jclass clazz,jfloat dx,jfloat dy,jfloat yold,jfloat y) {
static bool ybezig=false;
	auto absdy=fabsf(dy);
	if(fabsf(dx)>absdy) {	
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
		else  {
			ybezig=false;
//			starttime+=(dx/(10.0*24.0*4.0))*duration;
			starttime+=1.2*(dx/dwidth)*duration;
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
		if(ybezig||(dheight/absdy)<180) {
			ybezig=true;
			dy*=-1;
			float grens=dheight/2.0;

			if(y<grens&&yold<grens) {
//				grange*=(1+dy*3/dheight);
				grange*=dheight/(dheight-dy*1.4);
				settime=starttime;
				setend=starttime+duration;
				}
			else if(y>grens&&yold>grens) {
				int gmax=gmin+grange;
				grange*=dheight/(dheight+dy*1.4);
				gmin=gmax-grange;
				/*
					float wasgmin=gmin;
					gmin+=(dy*1.5/dheight)*grange;
					if(gmin<0)
						gmin=0;
					grange+=(wasgmin-gmin); */
					settime=starttime;
					setend=starttime+duration;
					}
			if(grange<180)
				grange=180;
			return 1;
			}
			}
		}
	return 0;
	}
	/*
void xscale(jfloat scalex, jfloat midx) {
	double rat=((midx-dleft)/dwidth);
	uint32_t focustime=rat*wasduration+wasstarttime;
	duration=wasduration/scalex;
	starttime=focustime-rat*duration;
	setend=0;
	}*/
extern "C" JNIEXPORT void JNICALL fromjava(xscale) (JNIEnv *env, jclass clazz,jfloat scalex,jfloat midx) {
	//xscale(scalex,midx);
	if(fixatex)
		return;

	double rat=((midx-dleft)/dwidth);
	uint32_t focustime=rat*duration+starttime;
	duration/=scalex;
	starttime=focustime-rat*duration;
	auto maxstart= maxstarttime();
	if(
#ifndef WEAROS
	!showpers&&
#endif

	starttime>maxstart)
		starttime=maxstart;
	setend=0;

	
	}


extern "C" JNIEXPORT void JNICALL fromjava(prevscr)(JNIEnv* env, jclass obj) {
	starttime-=duration;
	auto minstart= minstarttime();
	if(starttime<minstart)
		starttime=minstart;
	}
extern "C" JNIEXPORT void JNICALL fromjava(nextscr)(JNIEnv* env, jclass obj) {
	starttime+=duration;
#ifndef WEAROS
	if(!showpers) 
#endif

	{
		auto maxstart= maxstarttime(); if(starttime>maxstart) starttime=maxstart;
		}
	}
static int64_t menutap(float x,float y) ;

bool inbutton(float x,float y) {
	return !(x<menupos.left||x>=menupos.right|| y<menupos.top||y>=menupos.bottom) ;
	}

struct {
	float x=-300.0f,y=-300.0f;
	std::chrono::time_point<std::chrono::steady_clock>  time;
	} prevtouch;



extern "C" JNIEXPORT void JNICALL fromjava(pressedback) (JNIEnv *env, jclass clazz) {
	scantoshow={-1,nullptr}; 
	LOGGER("true\n");
	displayer.reset();
	}
extern "C" JNIEXPORT jboolean JNICALL fromjava(isbutton) (JNIEnv *env, jclass clazz,jfloat x,jfloat y) {
	LOGGER("isbutton ");
	if(!inbutton(x,y)) {
		LOGGER("false\n");
		return false;
		}
	if(restart) {
		LOGGER("restart\n");
		exit(1);	
		}
	if(inbutton(prevtouch.x,prevtouch.y)) {
			 auto nutime = chrono::steady_clock::now();
			 if(chrono::duration_cast<chrono::milliseconds>(nutime - prevtouch.time).count()<450) // don't close immediately if OK sits on pressed point
				return true;
			}
	scantoshow={-1,nullptr}; 
	LOGGER("true\n");
	displayer.reset();
//	lastscan=nullptr;
	return true;
	}

#include "numiter.h"
NumIter<Num> *numiters=nullptr;
int basecount;

#ifndef NDEBUG
#define lognum(x)
#else
void lognum(const Num *num) {
		constexpr int maxitem=80;
		char item[maxitem];
		time_t tim=num->time;
		int itemlen=datestr(tim,item);
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
int numfrompos(const float x,const float y) ;
vector<mealposition> mealpos;

extern "C" JNIEXPORT jlong JNICALL fromjava(tap) (JNIEnv *env, jclass clazz,jfloat x,jfloat y) {

#ifndef WEAROS
	if(!showpers ) 
#endif
	{
		if(numlist)  {
	//		if(x<(dwidth/10))	{ numpagepast(); return -1l; }

			if(int index=numfrompos(x,y);index>=0) {;
				const Num *num=numiters[index].prev();
				if(!numdatas[index]->valid(num))
					return -2LL;

				int pos=num-numdatas[index]->startdata();
				int base=numdatas[index]->getindex();
				return (static_cast<jlong>(pos)<<16)|((static_cast<jlong>(base)&0xf)<<8)|0xe;
				}
			else
				return -2LL;
			}
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
					return (static_cast<jlong>(p.mealpos)<<16)|((static_cast<jlong> (p.mealbase)&0xf)<<8)|0xe;
				}

			}
		}
#ifndef WEAROS
	const float wgrens=density*10;
	const float rgrens=dwidth-wgrens;

	if(x<wgrens)  {
			fromjava(prevscr)(0,0);
			return -1LL;
			}
		
	else 
	    if (x > rgrens)  {
		fromjava(nextscr)(0,0);
		return -1LL;
		}
	    
	if(showpers)  {

extern bool showsummarygraph;
		if(showsummarygraph) {
			showsummarygraph=false;
			return 1+5*0x10;
			}
		}
	else
#endif
	{
		tapx=x;tapy=y;
		}
//	highlight=nullptr;
//	lastscan=nullptr;
	return -1LL;
	}

struct NumHit{
NumDisplay *numdisplay;
const Num *hit;
};
Num newnum;
NumHit newhit={nullptr,&newnum};
template <class TX,class TY> NumHit *nearbynum(const float tapx,const float tapy,const TX &transx,  const TY &transy) {
	 for(auto el:numdatas) 
		if(const Num *hit=el->getnearby(transx,transy,tapx,tapy)) {
			return new  NumHit({el,hit});
			}
	 return nullptr;
	}

template <class TX,class TY> const ScanData * nearbyscan(const float tapx,const float tapy,const ScanData *low,const ScanData *high,  const TX &transx,  const TY &transy) {
	for(const ScanData *it=low;it!=high;it++) {
		if(it->valid()) {
			const uint32_t tim= it->t;
			const auto glu=it->g*10;
			const auto posx= transx(tim),posy=transy(glu);
			if(nearby(posx-tapx,posy-tapy))  {
				return it;
				}
			}
		}
	return nullptr;
	}
#include "strconcat.h"
void showOK(float xpos,float ypos) {
	nvgFontSize(vg,headsize/4 );

	const char ok[]="OK";
	const int oklen=sizeof(ok)-1;
	nvgTextBounds(vg, xpos,ypos ,ok , ok+oklen, (float *)&menupos);

	nvgText(vg, xpos,ypos,ok,ok+oklen);
	menupos.left-=mediumfont;
	menupos.right+=mediumfont;
	menupos.bottom+=mediumfont;
	menupos.top-=mediumfont;
	}
template <typename  TI,typename TE>
void textbox(const TI &title,const TE &text) {
	float w=dwidth*0.6;
// 	nvgRoundedRect(vg,  x,  y,  w,  h,  r);
//	x+=smallsize;
	nvgFontFaceId(vg,font);
	bounds_t bounds;
	nvgTextAlign(vg,NVG_ALIGN_LEFT|NVG_ALIGN_TOP);
	nvgFontSize(vg, smallsize);
	nvgTextLineHeight(vg, 1.7);
	 nvgTextBoxBounds(vg, 0,  0, w,begin(text), end(text), bounds.array);
	nvgBeginPath(vg);
	float width= bounds.xmax-bounds.xmin+ smallsize;
	float height= bounds.ymax-bounds.ymin+sensorbounds.height*2;
	float x=(dwidth-width)/2;
	float y=(dheight-height)/2;
	nvgFillColor(vg, red);
 	nvgRoundedRect(vg,  x-smallsize, y-smallsize,  width+2*smallsize, height+2*smallsize, dwidth/60 );
	nvgFill(vg);
	nvgFillColor(vg, *getblack());
	nvgTextBox(vg,  x,  y+sensorbounds.height+smallsize, width, begin(text),end(text));
	nvgFontSize(vg, mediumfont);

	nvgTextAlign(vg,NVG_ALIGN_LEFT|NVG_ALIGN_TOP);
	nvgText(vg, x,y, begin(title),end(title));
	nvgTextAlign(vg,NVG_ALIGN_RIGHT|NVG_ALIGN_TOP);
	showOK(x+width,y);
	}
class histgegs:public Displayer {
	const SensorGlucoseData *hist;
//	const uint32_t glu,tim;
	time_t nu;
public:
	histgegs(const SensorGlucoseData *hist): hist(hist)/*,glu(glu),tim(tim)*/,nu(time(nullptr)) {


	 prevtouch.time = chrono::steady_clock::now();
	LOGGER("histgegs %s",ctime(&nu));
	} 
	/*
virtual int display() override {
	const char title[]="History";
	char starts[50],ends[50];
	time_t stime=hist->getstarttime(),etime= stime+(14*24)*60*60;
	char lastscanbuf[50],lastpollbuf[50];
	time_t lastscan=hist->getlastscantime();
	time_t lastpolltime=hist->getlastpolltime();
	strconcat text(string_view(""), "Once per 15 minutes, remembered for 8 hours.\nScanning transfers them to this program.\nSensor: ",hist->shortsensorname(),"\nSensor started:  ",string_view(starts, datestr(stime,starts)),"\nLast scanned:     ",string_view(lastscanbuf,datestr(lastscan,lastscanbuf)),lastpolltime>0?"\nLast stream:        ":"",lastpolltime>0?string_view(lastpollbuf,datestr(lastpolltime,lastpollbuf)):"",nu<etime?"\nSensor ends:      ":"",
nu<etime?string_view(ends, datestr(etime,ends)):string_view("",0));

	textbox(title,text);

	return 1;
	}
*/
strconcat  getsensorhelp(string_view starttext,string_view name1,string_view name2,string_view sep1,string_view sep2) {
	char starts[50],ends[50];
	time_t stime=hist->getstarttime(),etime= stime+(14*24)*60*60;
	char lastscanbuf[50],lastpollbuf[50];
	time_t lastscan=hist->getlastscantime();
	time_t lastpolltime=hist->getlastpolltime();
	return strconcat(string_view(""),starttext ,name1,hist->shortsensorname(),name2,usedtext->sensorstarted,sep2,string_view(starts, datestr(stime,starts)),hist->isLibre3()?"":sep1,hist->isLibre3()?"":usedtext->lastscanned,hist->isLibre3()?"":sep2,hist->isLibre3()?"":string_view(lastscanbuf,datestr(lastscan,lastscanbuf)),lastpolltime>0?strconcat(string_view(""),sep1,usedtext->laststream,sep2):"",lastpolltime>0?string_view(lastpollbuf,datestr(lastpolltime,lastpollbuf)):"",nu<etime?strconcat(string_view(""),sep1,usedtext->sensorends,sep2):"",
nu<etime?string_view(ends, datestr(etime,ends)):string_view("",0));
	}
virtual int display() override {
	textbox(usedtext->history,getsensorhelp(hist->isLibre3()?usedtext->history3info:usedtext->historyinfo,"","\n","\n"," "));
	return 1;
	}


};
//histgegs gegs(
strconcat getsensortext(const SensorGlucoseData *hist) {
		histgegs gegs(hist);
		//return gegs.getsensorhelp("<small><br></small>","<h1>","</h1>","<br><br>","<br>");
		return gegs.getsensorhelp("","<h1>","</h1>","<br><br>","<br>");
		}
template <class TX,class TY> 
bool nearbyhistory( const float tapx,const float tapy,  const TX &transx,  const TY &transy) {
	for(int i=histlen-1;i>=0;i--) {
		const SensorGlucoseData *hist=sensors->gethist(hists[i]);
		const auto [firstpos,lastpos]=histpositions[i];
			for(auto pos=firstpos;pos<=lastpos;pos++) {
				uint32_t tim,glu;
				if((tim=hist->timeatpos(pos))&&( glu=hist->sputnikglucose(pos))) {
					auto posx=transx(tim),posy=transy( glu);
					if(nearby(posx-tapx,posy-tapy)) {
#ifdef WEAROS
						histgegs gegs(hist);
						callshowsensorinfo(gegs.getsensorhelp("","<h1>","</h1>","<br><br>","<br>").data());
#else
						::prevtouch.x=tapx;
						::prevtouch.y=tapy;
						LOGGER("x=%.1f, y=%.1f\n",tapx,tapy);
//						displayer=std::make_unique<histgegs>(hist,tim,glu);
						displayer=std::make_unique<histgegs>(hist);
#endif
						return true;
						}
					}
				}
		}
	return false;
	}

static bool  inmenu(float x,float y) ;

extern "C" JNIEXPORT jlong JNICALL fromjava(longpress) (JNIEnv *env, jclass clazz,jfloat x,jfloat y) {
	LOGGER("longpress\n");
	if(numlist
#ifndef WEAROS
	||showpers
#endif
	)
		return 0LL;
	if(inmenu(x,y))
		return 0LL;
	const uint32_t endtime=starttime+duration;
	const auto [transx,transy]= gettrans(starttime, endtime);
	if(shownumbers)
		if(NumHit *hit= nearbynum(x,y,transx,transy))
			return reinterpret_cast<jlong>(hit);
	if(showhistories&& nearbyhistory( x,y,  transx,  transy) ) {
		return 0LL;
		}
	if(showscans) {
		for(int i=histlen-1;i>=0;i--) {
			if(const ScanData *scan=nearbyscan(x,y,scanranges[i].first,scanranges[i].second,transx,transy)) {
				LOGGER("longpress scan %.1f\n",scan->g/18.0);
				int index=hists[i];
				scantoshow={index,scan};
				return 0LL;
				}
			 }
		 }
	if(showstream) {
		for(int i=histlen-1;i>=0;i--) {
			if(const ScanData *poll=nearbyscan(x,y,pollranges[i].first,pollranges[i].second,transx,transy)) {
				LOGGER("longpress poll %.1f\n",poll->g/18.0);
			//	oldstream
		//		oldstream[0]={poll, sensors->gethist(hists[i])->shortsensorname() };
				return 0LL;
				}
			 }
		 }

	int type=typeatheight(y); 
	if(type>=0)  {
		newnum.time=starttime+duration*(x-dleft)/dwidth;
		newnum.type=type;
		newnum.value=NAN;
		return reinterpret_cast<jlong>(&newhit);
		}
/*	tapx=x;tapy=y;
	selshown=true;
	longpress=true;*/
	return 0LL;
	}

extern "C" JNIEXPORT jlong JNICALL fromjava(newhit) (JNIEnv *env, jclass clazz) {
		return reinterpret_cast<jlong>(&newhit);
		}
extern "C" JNIEXPORT jlong JNICALL fromjava(hittime) (JNIEnv *env, jclass clazz,jlong ptr) {
	NumHit *num=reinterpret_cast<NumHit *>(ptr);
	return num->hit->time;
	}

extern "C" JNIEXPORT jfloat JNICALL fromjava(hitvalue) (JNIEnv *env, jclass clazz,jlong ptr) {
	NumHit *num=reinterpret_cast<NumHit *>(ptr);
	return num->hit->value;
	}
extern "C" JNIEXPORT jint JNICALL fromjava(hittype) (JNIEnv *env, jclass clazz,jlong ptr) {
	NumHit *num=reinterpret_cast<NumHit *>(ptr);
	return num->hit->type;
	}
extern "C" JNIEXPORT jint JNICALL fromjava(hitmeal) (JNIEnv *env, jclass clazz,jlong ptr) {
	NumHit *num=reinterpret_cast<NumHit *>(ptr);
	return num->hit->mealptr;
	}
extern "C" JNIEXPORT jint JNICALL fromjava(hitremove) (JNIEnv *env, jclass clazz,jlong ptr) {
	NumHit *num=reinterpret_cast<NumHit *>(ptr);
	 jint res=num->numdisplay->numremove(const_cast<Num*>(num->hit));
	 if(numlist) {
	 	for(int i=0;i<basecount;i++) {
			numiters[i].end=numdatas[i]->end()-1;
			}
	 	}

	 return res;
	}
	/*
extern "C" JNIEXPORT jboolean JNICALL fromjava(hitneedsync) (JNIEnv *env, jclass clazz,jlong ptr) {
	NumHit *num=reinterpret_cast<NumHit *>(ptr);
	return num->numdisplay->needsync();
}*/
extern "C" JNIEXPORT jint JNICALL fromjava(gethitindex) (JNIEnv *env, jclass clazz,jlong ptr) {
	NumHit *num=reinterpret_cast<NumHit *>(ptr);
	const NumDisplay *dis=num->numdisplay;
	if(!dis)
		return 1;
	return dis->getindex();
}

extern "C" JNIEXPORT void JNICALL fromjava(hitchange)(JNIEnv *env, jclass thiz,jlong ptr,jlong time,jfloat value,int type,int mealptr) {
	NumHit *num=reinterpret_cast<NumHit *>(ptr);
	num->numdisplay->numchange(num->hit,time,value,type,mealptr);
	}
extern "C" JNIEXPORT void JNICALL fromjava(hitsetmealptr)(JNIEnv *env, jclass thiz,jlong ptr,jint mealptr) {
	NumHit *num=reinterpret_cast<NumHit *>(ptr);
	num->numdisplay->setmealptr(const_cast<Num *>(num->hit),mealptr);
	}
/*
struct NumHit{
    NumDisplay *numdisplay;
    const Num *hit;
};
 */
extern "C" JNIEXPORT jlong JNICALL fromjava(mkhitptr) (JNIEnv *env, jclass clazz,jlong ptr,jint pos) {
	NumDisplay *dis=reinterpret_cast<NumDisplay*>(ptr);
	NumHit *num=new NumHit({dis,dis->startdata()+pos});
	return reinterpret_cast<jlong>(num);
	}
extern "C" JNIEXPORT void JNICALL fromjava(freehitptr)(JNIEnv *env, jclass thiz,jlong ptr) {
	delete reinterpret_cast<NumHit *>(ptr);
	}
extern "C" JNIEXPORT jlong JNICALL fromjava(getstarttime) (JNIEnv *env, jclass clazz) {
	return static_cast<jlong>(starttime)*1000l;
	};
extern "C" JNIEXPORT jlong JNICALL fromjava(getendtime) (JNIEnv *env, jclass clazz) {
	return (static_cast<jlong>(starttime)+duration)*1000l;
	};

static uint32_t timeend() {
	return starttime+duration;	
	}

uint32_t starttimefromtime(uint32_t pos) {
	return starttime+floor(((double)pos-starttime)/duration)*duration;
	}


void numpagenum(const uint32_t tim) ;

static void highlightnum(const Num *num) {
	uint32_t tim=num->time;
	if(numlist)
		numpagenum(tim) ;
	else
		starttime=starttimefromtime(tim);
	}

static uint32_t glucosesearch(uint32_t starttime,uint32_t endtime) ;
static uint32_t glucoseforwardsearch(uint32_t starttime,uint32_t endtime) ;

template <int N>
const Num *findpast() {
	const Num  *hit=nullptr;
	const int len=numdatas.size();
	int i=0;
	for(;i<len;i++) {
		if((hit=numdatas[i]->findpast<N>()))
			break;
		}
	if(hit) {
		for(;i<len;i++) {
			if(const Num *mog=numdatas[i]->findpast<N>();mog&&mog->time>hit->time)
				hit=mog;
			}
		}
	return hit;
	}

template <int N=1>
const Num *findforward()  {
	const Num  *hit=nullptr;
	const int len=numdatas.size();
	int i=0;
	for(;i<len;i++) 
		if((hit=numdatas[i]->findforward<N>()  ))
			break;

	if(hit) {
		for(;i<len;i++)  {
			if(const Num *mog=numdatas[i]->findforward<N>();mog&&mog->time<hit->time)
				hit=mog;
			}
		}
	return hit;
	}


int nextpast() {
	if(searchdata.type&glucosetype) {
		 return glucosesearch(0,starttime-1);
		}
	else {
		if(const Num *hit=findpast<0>()) {
			highlightnum(hit);
			return 0;
			}
		}
	return 1;
	}


int nextforward() {
	if(searchdata.type&glucosetype) {
		return glucoseforwardsearch(starttime+duration, std::numeric_limits<uint32_t>::max());
		}
	else {
		if(const Num *hit=findforward()) {
			highlightnum(hit);
			return 0;
			}
		}
	return 1;			
	}
extern "C" JNIEXPORT void JNICALL fromjava(stopsearch) (JNIEnv *env, jclass clazz) {
	searchdata.type=nosearchtype;
	}
extern "C" JNIEXPORT jint JNICALL fromjava(latersearch) (JNIEnv *env, jclass clazz) {
	return nextforward();
	}
extern "C" JNIEXPORT jint JNICALL fromjava(earliersearch) (JNIEnv *env, jclass clazz) {
	return nextpast();
	}
static const ScanData * findScan(const ScanData *start,const ScanData *en) {
	for(const ScanData *it=en-1;it>=start;--it) {
		if(searchdata(it))
			return it;
			/*
		int32_t glu=it->g;
		if(glu&&glu>=low&&glu<=high)
			return it;
			*/
		}
	return nullptr;
 }
static const Glucose * findhistory(const SensorGlucoseData  * hist, const uint32_t firstpos, const uint32_t lastpos) {
	for(auto pos=lastpos;pos>=firstpos;--pos)  {
		const Glucose *g=hist->getglucose(pos);
		if(searchdata(g))
			return g;

		}
	return nullptr;
	}
static void glucosesel(uint32_t tim) {
	if(tim>starttime&&tim<timeend())
		return;
	starttime=starttimefromtime(tim);
	}
#ifndef NDEBUG
void logglucose(const char *str,const Glucose *glu) {
	const time_t tim=glu->gettime();
	LOGGER("%s: %d %.1f %s",str, glu->id,glu->getsputnik()/100.0f,ctime(&tim));
	}
#else
#define	logglucose(x,y)
#endif
static uint32_t glucosesearch(uint32_t starttime,uint32_t endtime) {
	LOGGER("glucosesearch(%u,%u)\n",starttime, endtime);
	uint32_t hittime=starttime;
	const Glucose *histhit=nullptr;
	const ScanData *scanhit=nullptr;
	for(int it=sensors->last();it>=0;it--) {
		const sensor *sen=sensors->getsensor(it);
		LOGGER("Sensor %s\n",sen->name);
		if(sen->starttime>endtime)
			continue;
		if(sen->endtime&&sen->endtime<starttime)
			break;
		if(auto his=sensors->gethist(it)) {
			int32_t lastpos= his->getendhistory()-1;
			uint32_t tim=0;
			int32_t firstpos = his->getstarthistory();
			if(lastpos<firstpos) {
				goto skiphistory;
				}
			for(;!(tim=his->getglucose(lastpos)->gettime());lastpos--) {
				if(lastpos<=firstpos)
					break;
				}
			if(tim<hittime)  {
				//continue;
				goto skiphistory;
				}
			if((searchdata.type&historysearchtype)==historysearchtype) {
				int endpos;
				if(tim<endtime)
					endpos=lastpos;
				else {
					int period=his->getinterval();
					endpos=lastpos-(tim-endtime)/period;
					if(endpos<1)
						endpos=1;	
					}
				while(endpos<lastpos&&	his->timeatpos(endpos)<endtime)
					endpos++;
				uint32_t tmptim;
				while(!(tmptim=his->timeatpos(endpos))||tmptim>=endtime) {
					endpos--;
					if(endpos<=firstpos)
						goto skiphistory;
					}

				int startpos=his->gettimepos(hittime);
				if(startpos<1) 
					startpos=1;
				else {
					
					while(startpos>1&&(!(tmptim=his->timeatpos(startpos))||tmptim>=hittime))
						startpos--;
					while(startpos<endpos&&	his->timeatpos(startpos)<hittime)
						startpos++;
					}
				 const Glucose *mog=findhistory(his,startpos,endpos); 
				 if(mog&&mog->gettime()>hittime) {
					histhit=mog;
					hittime=mog->gettime();
					logglucose("glucosesearch mog ",mog);
					}
				}
			skiphistory:
			if((searchdata.type&scansearchtype)==scansearchtype) {
				std::span<const ScanData> 	scan=his->getScandata();
				auto [under,above] =getScanRange(scan.data(),scan.size(),hittime,endtime) ;
				const ScanData *mogscan=findScan(under,above);
				if(mogscan&&mogscan->t>hittime) {
					scanhit=mogscan;
					hittime=mogscan->t;
					}
					}
			if((searchdata.type&streamsearchtype)==streamsearchtype) {
				std::span<const ScanData> 	scan=his->getPolldata();
				auto [under,above] =getScanRange(scan.data(),scan.size(),hittime,endtime) ;
				const ScanData *mogscan=findScan(under,above);
				if(mogscan&&mogscan->t>hittime) {
					scanhit=mogscan;
					hittime=mogscan->t;
					}
				   }
		}
	   }

	uint32_t res;
	if(!histhit)	{
		if(!scanhit)  {
			LOGGER("no hist and no scanhit\n");
			return 1;
			}
		LOGGER("no scan hist and but scanhit\n");
		res=scanhit->t;
		}
	else {
		LOGGER("hist hit\n");
		if(!scanhit||histhit->time>scanhit->t)
			res=histhit->time;
		else
		 	res=scanhit->t;
		 }
	LOGGER("glucosesearch found %d\n",res);
	glucosesel(res);
	return 0;
	}

static const ScanData * findforwardScan(const ScanData *start,const ScanData *en) {
	for(const ScanData *it=start;it<en;++it) {
		if(searchdata(it))
			return it;
		}
	return nullptr;
 }
static const Glucose * findforwardhistory(const SensorGlucoseData  * hist, const uint32_t firstpos, const uint32_t lastpos) {
	for(auto pos=firstpos;pos<=lastpos;++pos)  {
		const Glucose *g=hist->getglucose(pos);
		if(searchdata(g))
			return g;

		}
	return nullptr;
	}

static uint32_t glucoseforwardsearch(uint32_t starttime,uint32_t endtime) {
	uint32_t hittime=endtime;
	const Glucose *histhit=nullptr;
	const ScanData *scanhit=nullptr;
	for(int it=0;it<=sensors->last();it++) {
		const sensor *sen=sensors->getsensor(it);
		LOGGER("Sensor %s\n",sen->name);
		if(sen->starttime>hittime)
			break;
		if(sen->endtime<starttime)
			continue;
		auto his=sensors->gethist(it);
		int32_t lastpos= his->getendhistory()-1;
		int32_t firstpos= his->getstarthistory();
		uint32_t tim=0;
		if(lastpos<firstpos) {
			goto skiphistory;
			
			}
		for(;!(tim=his->getglucose(lastpos)->gettime());lastpos--) {
			if(lastpos<=firstpos)
				break;
			}
				
		if(tim<starttime) {
			goto skiphistory;
			}

	if((searchdata.type&historysearchtype)==historysearchtype) {
		int endpos;
		if(tim<hittime)
			endpos=lastpos;
		else {
			int period=his->getinterval();
			endpos=lastpos-(tim-hittime)/period;
			if(endpos<1)
				endpos=1;	
			}
		while(endpos<lastpos&&	his->timeatpos(endpos)<hittime)
			endpos++;
		uint32_t tmptim;
		while(!(tmptim=his->timeatpos(endpos))||tmptim>=endtime) {
			endpos--;
			if(endpos<=firstpos)
				goto skiphistory;
			}

		int startpos=his->gettimepos(starttime);
		if(startpos<1) 
			startpos=1;
		else {
		      uint32_t tmptim;
			while(startpos>1&&(!(tmptim=his->timeatpos(startpos))||tmptim>=starttime))
				startpos--;
			while(startpos<endpos&&	his->timeatpos(startpos)<starttime)
				startpos++;
			}
		 const Glucose *mog=findforwardhistory(his,startpos,endpos); 
		 if(mog&&mog->gettime()<hittime) {
			histhit=mog;
			hittime=mog->gettime();
			}
		}
	skiphistory:
	if((searchdata.type&scansearchtype)==scansearchtype) {
		const std::span<const ScanData> 	scan=his->getScandata();
		auto [under,above] =getScanRange(scan.data(),scan.size(),starttime,hittime) ;
		const ScanData *mogscan=findforwardScan(under,above);
		if(mogscan&&mogscan->t<hittime) {
			scanhit=mogscan;
			hittime=mogscan->t;
			}
	   	    }

	if((searchdata.type&streamsearchtype)==streamsearchtype) {
		const std::span<const ScanData> 	scan=his->getPolldata();
		auto [under,above] =getScanRange(scan.data(),scan.size(),starttime,hittime) ;
		const ScanData *mogscan=findforwardScan(under,above);
		if(mogscan&&mogscan->t<hittime) {
			scanhit=mogscan;
			hittime=mogscan->t;
			}
	   	    }





	   }



	uint32_t res;
	if(!histhit)	{
		if(!scanhit) 
			return 1;
		res=scanhit->t;
		}
	else {
		if(!scanhit||histhit->time<scanhit->t)
			res=histhit->time;
		else
		 	res=scanhit->t;
		 }

	glucosesel(res);
	return 0;
	}
extern "C" JNIEXPORT jint JNICALL fromjava(search) (JNIEnv *env, jclass clazz,jint type, jfloat under,jfloat above,jint frommin,jint tomin,jboolean forward,jstring jregingr,jfloat amount) {
if(type&glucosetype) {
//	searchdata={type ,float(under*180.0), float(above*180.0), frommin, tomin,0};
	searchdata={type ,backconvert(under), backconvert(above), frommin, tomin,0};
	return forward?glucoseforwardsearch(starttime, std::numeric_limits<uint32_t>::max()):glucosesearch(0,starttime+duration);
	}
auto maxlab=getmaxlabel();
if(type>=maxlab)
	type=0x80000000;
searchdata={ type, under, above, frommin, tomin,maxlab};
if(jregingr!=nullptr&&type==carbotype) {
        const char *regingr = env->GetStringUTFChars( jregingr, nullptr);
        if(regingr == nullptr) {
		searchdata.type=nosearchtype;
		return 3;
		}
	meals->datameal()->searchingredients(regingr,searchdata.ingredients);
        env->ReleaseStringUTFChars(jregingr, regingr);
	if(searchdata.ingredients.size()==0) {
		searchdata.type=nosearchtype;
		return 4;
		}
	searchdata.amount=amount;
	}
else
	searchdata.ingredients.clear();
if(const Num *hit=forward?findforward<0>():findpast<1>()) {
	highlightnum(hit);
	return 0;
	}
searchdata.type=nosearchtype;
return 1;
}

extern "C" JNIEXPORT void JNICALL fromjava(movedate) (JNIEnv *env, jclass clazz,jlong milli,jint year,jint month,jint day) {
	time_t tim=milli/1000l;
	struct tm		stm{};
	localtime_r(&tim,&stm);
	stm.tm_year=year-1900;
	stm.tm_mon=month;
	stm.tm_mday=day;
	time_t timto=mktime(&stm);
	starttime+=uint32_t((int64_t)timto-(int64_t)tim);
	begrenstijd() ;
	};
static constexpr const int day=60*60*24;
void prevdays(int nr) {
	//starttime=starttimefromtime(starttime-nr*day);
	starttime-=nr*day;
	auto minstart= minstarttime();
	if(starttime<minstart)
		starttime=minstart;
	}
extern "C" JNIEXPORT void JNICALL fromjava(prevday)(JNIEnv* env, jclass obj) {
	prevdays(1);
	}
static void nextdays(int nr) {
	//starttime=starttimefromtime(starttime+day*nr);
	starttime+=day*nr;
#ifndef WEAROS
	if(!showpers) 
#endif
	{
		auto maxstart= maxstarttime(); if(starttime>maxstart) starttime=maxstart;
		}
	}
extern "C" JNIEXPORT void JNICALL fromjava(nextday)(JNIEnv* env, jclass obj) {
	nextdays(1);
/*
	starttime+=day;
	auto maxstart= maxstarttime();
	if(starttime>maxstart)
		starttime=maxstart;
		*/
	}

//constexpr int hourminstrlen=20;
//static char hourminstr[hourminstrlen]="00:00        ";
//static char hourminstr[hourminstrlen]="00:00       ";
#define hourtext "00:00       "
//constexpr const int hourtextlen=sizeof(hourtext)-1;
char hourminstr[hourminstrlen]=hourtext;
void setnowmenu(time_t nu) {
	hourmin(nu,hourminstr);
	const int ulen=usedsensors.size();
	if(ulen>0) {
		for(int i=0;i<ulen;) {
			if(const auto *lastin=sensors->gethist(usedsensors[i++])->lastpoll()) {
				for(;i<ulen;i++) {
					if(const auto *lastsen=sensors->gethist(usedsensors[i])->lastpoll();lastsen&&(lastsen->t>lastin->t)) {
						lastin=lastsen;
						}
					}
				if(lastin->t>(nu-maxbluetoothage)) {
					snprintf(hourminstr+5,hourminstrlen-5,"  %.*f   ",gludecimal,gconvert(lastin->g*10));
					return ;
					}
				}
			}
		}
	memset(hourminstr+5,' ',hourminstrlen-6); 
	}
int notify=false;

#define arsizer(x) sizeof(x)/sizeof(x[0])

#ifdef WEAROS
#if 0
string_view menustr0[]= {
	"Mirror",
	"Sensor",
	"Dark mode     ",
        "Settings",
	"Stop Alarm"
};

const char	nothing[]="      ";
const char    newamount[]="Amount";
char amountstr[]="Amount";
//string_view menustr2[]= {"Date","Search",hourminstr,"Day back","Day later"};
string_view menustr2[]= {"Date",hourminstr,"Day back",amountstr};
#endif

const int *menuopt0[]={nullptr,nullptr,&invertcolors, nullptr,nullptr};
const int **optionsmenu[]={menuopt0,nullptr};
//string_view *menustr[]={menustr0,menustr2};
constexpr const int menulen[]={arsizer(jugglucotext::menustr0),arsizer(jugglucotext::menustr2)};
//constexpr const int maxmenulen= std::max(std::size(menustr0),std::size(menustr2));
int getmenulen(const int menu) {
	int len=menulen[menu];
	if(menu==1&&settings->staticnum()) 
		return len-1;
	if(!menu&&!alarmongoing)
		return len-1;
		
	return len;	
	}

#else
/*
const char newamount[]="New Amount";
char amountstr[]="New Amount";

string_view menustr0[]={
        "System UI     ",
	"Watch",
	"Sensor",
	 "Settings",
        "Close",
	"Stop Alarm"

	}; */
const int *menuopt0[]={&showui, nullptr,nullptr,nullptr,nullptr,nullptr,nullptr};
/*
string_view menustr1[]= {
	"Notify",
        "Export",
	"Mirror",
	 amountstr,
	"List"
#ifdef PERCENTILES
	, "Statistics"
#endif
};
*/


const int *menuopt0b[]={&notify,nullptr,nullptr,nullptr,nullptr,nullptr};
const int *menuopt1[]={nullptr,&showscans, &showstream,&showhistories, &shownumbers,&showmeals,&invertcolors};
//const int *menuopt1[]={nullptr,&showscans, &showstream,&showhistories, &shownumbers,&showmeals, &settings->data()->invertcolors};
const int **optionsmenu[]={menuopt0,menuopt0b,menuopt1,nullptr};
/*
string_view menustr2[]= {"Last Scan","Scans","Stream","History","Amounts","Meals","Dark mode       "};
//int nomenuop;
string_view menustr3[]= {hourminstr,"Search","Date","Day back","Day later","Week back","Week later"}; 
string_view *menustr[]={menustr0,menustr1,menustr2,menustr3}; */
//int menustr[]={menustr0,menustr2,menustr3};
#define arsizer(x) sizeof(x)/sizeof(x[0])
constexpr const int menulen[]={arsizer(jugglucotext::menustr0),arsizer(jugglucotext::menustr1),arsizer(jugglucotext::menustr2),arsizer(jugglucotext::menustr3)};
int getmenulen(const int menu) {
	int len=menulen[menu];
	if(!menu&&!alarmongoing)
		return len-1;
	return len;	
	}

static std::string_view	nothing="          ";
void setnewamount() {
	if(settings->staticnum()) {
		usedtext->menustr1[3]=nothing;
		}
	else
		usedtext->menustr1[3]=usedtext->newamount;
	}
#endif
/*
void setnewamount() {
	if(settings->staticnum()) {
		memcpy(amountstr,nothing,sizeof(amountstr)-1);
		}
	else
		memcpy(amountstr,newamount,sizeof(amountstr)-1);
	}
*/


constexpr const int maxmenulen= *std::max_element(std::begin(menulen),std::end(menulen));
constexpr int maxmenu=arsizer(jugglucotext::menustr);
#ifdef WEAROS
static_assert(maxmenu==2);
#else
static_assert(maxmenu==4);
#endif
int getmenu(int tapx) {
	return tapx*maxmenu/dwidth;
	}

static const float  getsetlen(NVGcontext* vg,float x, float  y,const char * set,const char *setend,bounds_t &bounds) {
	 	nvgTextBounds(vg, x,  y, set,setend, bounds.array);
		return bounds.xmax-bounds.xmin;
		}
static void showtext(NVGcontext* vg ,time_t nu,int menu) {
#ifdef WEAROS
	if(menu==1) {
		setnowmenu(nu);
//		setnewamount();
		}
#else
	if(menu==3)
		setnowmenu(nu);
	if(menu==1)
		setnewamount();
#endif
	string_view *menuitem=usedtext->menustr[menu];
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
//	float menutextheight=density*48;
	float menuplace= dwidth/ maxmenu;
	float x=xrand+menu*menuplace,starty=yrand,y=starty;

	nvgTextAlign(vg,NVG_ALIGN_LEFT|NVG_ALIGN_TOP);

	bounds_t bounds;

	nvgFontFaceId(vg,menufont);
	nvgFontSize(vg, menusize);
	 nvgTextBounds(vg, x,  y, menuitem[0].data(),menuitem[0].data()+menuitem[0].size(), bounds.array);
//	nvgText(vg, x,y, menuitem[0].data(), menuitem[0].data()+menuitem[0].size());
	 float maxx=bounds.xmax;
	 float maxwidth=bounds.xmax-bounds.xmin;
	 for(int i=1;i<nrmenu;i++) {
		y+=menutextheight;
	 	nvgTextBounds(vg, x,  y, menuitem[i].data(),menuitem[i].data()+menuitem[i].size(), bounds.array);
	 	if(maxx<bounds.xmax)
			maxx=bounds.xmax;
		 float maxwidthone=bounds.xmax-bounds.xmin;
		 if(maxwidthone>maxwidth)
		 	maxwidth=maxwidthone;
		}
	float height=y+bounds.ymax-bounds.ymin;
	nvgBeginPath(vg);
	 nvgFillColor(vg, *getmenucolor());
//	 nvgFillColor(vg, white);
	 float mwidth=maxx-x+2*xrand;
//	 float minmenu=128*density;
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
	 nvgRect(vg, x-xrand, starty-yrand, mwidth, height-starty+2*yrand);
	nvgFill(vg);
#ifdef WEAROS
if(menu==0) {
	nvgTextAlign(vg,NVG_ALIGN_RIGHT|NVG_ALIGN_TOP);
	x+=maxwidth;
	}
#endif



	y=starty;
//	 nvgFillColor(vg, *getwhite());
	 nvgFillColor(vg, *getmenuforegroundcolor());
//	 nvgFillColor(vg, black);
	 for(int i=0;i<nrmenu;i++) {
		nvgText(vg, x,y, menuitem[i].data(), menuitem[i].data()+menuitem[i].size());
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
		static const float  dlen=getsetlen(vg, x,  y, set,set+len, bounds);
		 xpos=x-2*xrand+mwidth-dlen;
		 }
		 for(int i=0;i<nrmenu;i++) {
		 	if(const int *optr=options[i]) {
				const char *op=*optr?set:unset;
				nvgText(vg, xpos ,y,op ,op+len );
				}
			y+=menutextheight;
			}
		}
	}


extern bool makepercetages() ;
//alignas(sizeof(char *)) char buffer[1024];
//#include <thread>
/*
extern		void			testbackup();
void testbackup() {
 	int ind=backup->newupdate("192.168.1.71","12345");
	if(ind<0) {
	     LOGGER("no room for other host\n");
	     return;
	     }
//	     int ind=0;
	   backup->update(ind);
	}
	*/
extern bool setbluetoothon;
extern void setbuffer(char *);


extern Backup *backup;
inline jlong menuel(int menu,int item) {
	return menu+item*0x10LL;
	}
#ifdef WEAROS
static int64_t doehier(int menu,int item) {
	switch(menu) {
		case 0: 
			switch(item) {
				case 0 :  nrmenu=0;return 2LL*0x10+1;				
				case 1 : nrmenu=0;return 2LL*0x10;
				extern void setinvertcolors(bool val) ;
				case 2: invertcolors=!invertcolors; setinvertcolors(invertcolors) ; return -1LL;
					break;
				case 3: nrmenu=0; return 3LL*0x10;
				case 4: nrmenu=0; return menuel(0,5);

				};break;
		case 1: 
			nrmenu=0;
			switch(item) {
				case 0: return 2LL*0x10+3;
//				case 1: return 1LL*0x10+3;
				case 1: {
					auto max=time(nullptr);
					starttime=max-duration*3/5;
					return -1LL;
					}
				case 2: prevdays(1); return -1LL;
				case 3: 
					if(settings->staticnum()) 
						return -1LL;
					return  menuel(1,3);
					/*
				case 4:	
					if(!numlist) {
						void numiterinit();
						numiterinit();
						numlist=1;
						}
					return menuel(1,4); */
				};
		}
	return -1LL;
	}
#else
static int64_t doehier(int menu,int item) {
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
				case 0: notify=!notify;return menu|item*0x10|(notify<<8);
				case 3: nrmenu=0;
					if(settings->staticnum()) 
						return -1LL;
					break;
				case 4:	nrmenu=0; 
					if(!numlist) {
						void numiterinit();
						numiterinit();
						numlist=1;
						}
					break;

#ifdef PERCENTILES
				case 5:  nrmenu=0; if(!makepercetages())
						return -1LL;
					;break;
#endif
				default:
					nrmenu=0;
					break;
				};break;
		case 2:
			switch(item) 	{
				case 0: {
					nrmenu=0;
					int lastsensor=sensors->lastscanned();
					if(lastsensor>=0) {
						const SensorGlucoseData *hist=sensors->gethist(lastsensor);
						if(hist) {
							const ScanData *scan= hist->lastscan();
							if(scan&&scan->valid()&&((time(nullptr)-scan->t)<(60*60*5)))
								scantoshow={lastsensor,scan};
							}
						}
					}; return -1ll;
				case 1:showscans=!showscans;return -1ll;
				case 2:showstream=!showstream;return -1ll;
				case 3:showhistories=!showhistories;return -1ll;
				case 4: shownumbers=!shownumbers;return -1ll;
				case 5: showmeals=!showmeals;return -1ll;
				extern void setinvertcolors(bool val) ;

				case 6: invertcolors=!invertcolors; setinvertcolors(invertcolors) ; return -1ll;
				};break;
		case 3: {
		nrmenu=0;
		switch(item) {
			case 0: {
			auto max=time(nullptr);
		//	starttime=starttimefromtime(max);
//			if((starttime+duration)<max) 
			starttime=max-duration*3/5;
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

static int64_t menutap(float x,float y) {
	if(x<menupos.left||x>=menupos.right) {
		nrmenu=0;
		return -1LL;
		}
	float dist=(menupos.bottom-menupos.top)/nrmenu;
	int item=(y-menupos.top)/dist;
	if(item>=0&&item<nrmenu) {

		LOGGER("menuitem %d\n",item);
	//	return doehier(getmenu(x),item);
		return doehier(selmenu,item);
		}
	nrmenu=0;
	return -1LL;	
	}


extern "C" JNIEXPORT jlong JNICALL fromjava(lastpoll)(JNIEnv *env, jclass thiz) {
	const SensorGlucoseData *hist=sensors->gethist(); 
	const ScanData *glu=hist->lastpoll() ;
	if(glu)
		return (((jlong)glu->g)<<32)|(jlong)glu->t;
	else
		return 0LL;
//	return (((jlong)glu->g)<<32);
	}




void settoend() ;
void shownumiters() ;
#include "oldest.h"
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
//	shownumfromtop();
}

extern int nrcolumns;
int nrcolumns=1;
int numfrompos(const float x,const float y) {
	int rows=dheight/textheight;

	int ind= ((nrcolumns!=1&&x>(dleft+dwidth/2))?rows:0)+ (y-dtop)/textheight;
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
	LOGGER("Page forward\n");
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
void showfromend() {
	settoend() ;
	shownumsback(vg, numiters,basecount);
       setitertobottom(numiters,basecount);
	}
void showfromstart() {
	if(numlist==4) {
		numlist=1;
		}
	else {
		for(int i=0;i<basecount;i++) {
			numiters[i].iter=getpageoldest(i);
			}
		}
	 shownums(vg, numiters, basecount);
	for(int i=0;i<basecount;i++) {
		setpagenewest(i,numiters[i].iter);
		}
	}

void numpagenum(const uint32_t tim) {
//	const Num *nums[basecount];
	int tot=0;
	for(int i=0;i<basecount;i++)  {
		const Num *ptr=numdatas[i]->firstnotless(tim) ;
		if(ptr==numdatas[i]->end()||ptr->gettime()>tim)
			ptr--;
		int pos=ptr-numdatas[i]->begin()+1;
		if(pos>0)
			tot+=pos;
		numiters[i].iter=ptr;
		}

	int percol=dheight/textheight;
	int tever=tot%(nrcolumns*percol);
	while(tever--) {
//		findnewest(numiters,basecount,notvali);
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
void shownumlist() {
	startstep(*getwhite());
	if(getpageoldest(0)!=nullptr) {
		showfromstart();
		 }
	else {	
		showfromend();
		}
	}

NumIter<Num> *mknumiters() ;

void numiterinit() {
	basecount=numdatas.size();
	delete[] numiters;
	numiters=mknumiters() ;
	for(int i=0;i<basecount;i++) {
		setpagenewest(i, numdatas[i]->end());
		setpageoldest(i, nullptr);
		}
	numpagenum(starttime+duration/2);
	}

void numendbegin() {
	for(int i=0;i<basecount;i++) {
		setpagenewest(i, numdatas[i]->end());
	//	setpagenewest(i,numiters[i].next(numiters[i].end));
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
//template <class T> int ifindnewest(NumIter<T> *nums,int count) ;
//extern template int ifindnewest<Num>(NumIter<Num> *,int );
/*
int numfrompos(float x) {
	settoend() ;
	float textheight=density*48;
	int ind=(dwidth -(x-dtop))/textheight;
	int i=0,index;
	do {
		index=ifindnewest(numiters,basecount);
		} while(i++<ind);
	return index;
	}
	*/
void shownumiters() {
	LOGGER("Iters:\n");
	for(int i=0;i<basecount;i++) {
		lognum(numiters[i].iter);
		}
	LOGGER("\n");
	}
/*
void scrollnum() {
	settoend() ;
	int res=ifindnewest(numiters,basecount);
	const Num *num=numiters[res].iter+1;
	lognum(num);
	for(int i=0;i<basecount;i++) {
		numiters[i].end=numiters[i].iter;
		}
	}
extern "C" JNIEXPORT void JNICALL fromjava(logtid)(JNIEnv *env, jclass thiz,jstring jmess) {

	jint len = env->GetStringUTFLength( jmess);
	char mess[len+1];
	env->GetStringUTFRegion(jmess, 0,len, mess);
	mess[len]='\0';
	LOGGER("tid=%ld %s\n",syscall(SYS_gettid),mess);
	}
	*/

extern "C" JNIEXPORT void JNICALL fromjava(firstpage)(JNIEnv *env, jclass thiz) {
	for(int i=0;i<basecount;i++)
		setpageoldest(i,numiters[i].begin);
	}

extern "C" JNIEXPORT void JNICALL fromjava(lastpage)(JNIEnv *env, jclass thiz) {
	numpagenum(maxtime());
	}

extern "C" JNIEXPORT void JNICALL fromjava(endnumlist)(JNIEnv *env, jclass thiz) {
	numlist=0;

	uint32_t first=UINT32_MAX,second=0;
	for(const NumDisplay *num:numdatas) {
		const Num *one=std::max(num->begin(),num->extrum.first);		
		const Num *two=std::min(num->end(),num->extrum.second);		//NODIG?
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
	starttime=starttimefromtime((first+second)/2);
	return;
	}
/*
extern "C" JNIEXPORT void JNICALL fromjava(systemUI)(JNIEnv *env, jclass thiz,jboolean val) {
	showui=val;
	} */
extern "C" JNIEXPORT jboolean JNICALL fromjava(getsystemUI)(JNIEnv *env, jclass thiz) {
	return showui;
	}
//#include <stdio.h>
       #include <sys/types.h>
       #include <sys/stat.h>
       #include <fcntl.h>

/*
extern "C" JNIEXPORT void JNICALL fromjava(tryfopen)(JNIEnv *env, jclass thiz) {
	char file[]="/system/fonts/Roboto-Black.ttf";
	LOGGER("tryfopen\n");
	int fp=open(file,O_RDONLY);
	
//	FILE *fp=fopen(file,"rb");
	if(fp) {
		LOGGER("opened %s\n",file);
		close(fp);
		}
	else
		LOGGER("opened %s failed\n",file);
	}*/
extern "C" JNIEXPORT jlong JNICALL fromjava(openNums)(JNIEnv *env, jclass thiz,jstring jbase,jlong ident) {

	jint len = env->GetStringUTFLength( jbase);
	int blen=numbasedir.length();
	int alllen=len+blen+1;
	char base[alllen+1];
	memcpy(base,numbasedir.data(),blen);
	base[blen++]='/';
	jint jlen = env->GetStringLength( jbase);
	env->GetStringUTFRegion(jbase, 0,jlen, base+blen);
	base[alllen]='\0';
	 NumDisplay* numdata=NumDisplay::getnumdisplay( string_view(base,alllen),ident,nummmaplen);
	 if(numdata) {
		numdatas.push_back(numdata);
		if(ident==0LL)
			newhit.numdisplay=numdata;
		}
	
	LOGGER("numdir=%s ptr=%p\n",base,numdata);
	return reinterpret_cast<jlong>(numdata);
	}
/*
extern "C" JNIEXPORT jboolean JNICALL fromjava(usemeal)(JNIEnv *env, jclass thiz) {
	#ifdef USE_MEAL
		return true;
	#else
		return false;
	#endif
	} */
extern "C" JNIEXPORT void JNICALL fromjava(setlastcolor)(JNIEnv *env, jclass thiz,jint color) {
	LOGGER("lasttouchedcolor=%d color=%x\n",lasttouchedcolor,color);
	if(lasttouchedcolor<0)
		return;
	setcolor(lasttouchedcolor, hexcoloralpha((uint32_t)color));
	}
extern "C" JNIEXPORT jint JNICALL fromjava(getlastcolor)(JNIEnv *env, jclass thiz) {
	if(lasttouchedcolor<0)
		return 0xFFFFFFFF;
	return  fromNVGcolor(getcolor(lasttouchedcolor));
	}
extern "C" JNIEXPORT void JNICALL fromjava(setscreenwidthcm)(JNIEnv *env, jclass thiz,jfloat wcm) {
	screenwidthcm=wcm;
	iswatch=(wcm<5.8f);
	}

#define defdisplay(kind)\
extern "C" JNIEXPORT jboolean JNICALL fromjava(getshow##kind)(JNIEnv *env, jclass thiz) {\
	return show##kind;\
	}\
extern "C" JNIEXPORT void JNICALL fromjava(setshow##kind)(JNIEnv *env, jclass thiz,jboolean val) {\
	show##kind=val;\
	}

defdisplay(scans)
defdisplay(histories)
defdisplay(stream)
defdisplay(numbers)

