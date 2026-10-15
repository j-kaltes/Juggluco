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

#include <array>
#include <stdio.h>
#include "numiter.hpp"
#include "settings/settings.hpp"
#include "curve.hpp"
#include "JCurve.hpp"
#include "../notes/Notes.hpp"
#include "../net/libreview/numcategories.hpp"
extern Notes *notes;

 //statusbarleft=left;
 //statusbarright=right;
extern int statusbarleft,statusbarright;
extern  NVGcolor *colors[];
struct geo_t {
 int width;
 int height;
} numcontrol;
 float JCurve::second(geo_t&geo) const {
       return (dwidth-statusbarleft-statusbarright+geo.width)/2 +dleft+statusbarleft;	
 	};
float JCurve::colwidth(geo_t&geo) const {
            return (dwidth-geo.width-statusbarright-statusbarleft)/2;
            };

//extern int statusbarheight;
template <typename F> void JCurve::columnfromabove(NVGcontext* vg,const F &show) {
	float miny=dtop+textheight*.35+statusbarheight;
	int nr=(dheight-statusbarheight)/textheight;
	for(float y=miny;nr--&&show(y);y+=textheight) {
		}

	}
	
template <typename F> void JCurve::columnfrombelow(NVGcontext* vg,int nr,const F &show) {
	float miny=dtop+textheight*.35+statusbarheight;
	float maxy= miny+(nr-1)*textheight;

	for(float y=maxy;nr--&&show(y);y-=textheight) {
		}

	}


void JCurve::initcolumns( NVGcontext* vg) {
	nvgStrokeColor(vg, *getyellow());
	nvgStrokeWidth(vg, hitStrokeWidth);
	nvgFontFaceId(vg,font);
	nvgFontSize(vg, menusize);
	nvgFillColor(vg, *getblack()); 
	}

extern int nrcolumns; 
template <typename F> void JCurve::numscreen(NVGcontext* vg, const F & col)  {
   auto l=dleft+ statusbarleft;
   auto w=dwidth-statusbarright-statusbarleft;
	initcolumns(vg);
	if(nrcolumns==1) {
		col(vg,l,l+w-numcontrol.width-smallsize);
		}
	else {
		float xmid=second(numcontrol);
		float xwidth=colwidth(numcontrol);
		col(vg,l,l+xwidth-smallsize);
		col(vg,xmid+smallsize,xmid+xwidth-smallsize);
		}
	}
template <typename F> void JCurve::numscreenback(NVGcontext* vg, const F & col)  {
	initcolumns(vg);
	int nr=(dheight-statusbarheight)/textheight;

   auto l=dleft+ statusbarleft;
   auto w=dwidth-statusbarright-statusbarleft;
	if(nrcolumns==1) {
		col(vg,nr,l,l+w-numcontrol.width-smallsize);
		}
	else {
		float xmid=second(numcontrol);
		float xwidth=colwidth(numcontrol);
		col(vg,nr,xmid+smallsize,xmid+xwidth-smallsize);
		col(vg,nr,l,l+xwidth-smallsize);
		}
	}
    /*
inline int mktmmin(const struct tm *tmptr) {
	return tmptr->tm_min;
	} */

inline int datestr2(const time_t tim,char *buf) {
	struct tm tmbuf;
	 struct tm *stm=localtime_r(&tim,&tmbuf);
	int len=sprintf(buf,"%02d-%02d-%d ",stm->tm_mday,stm->tm_mon+1,1900+stm->tm_year);
   len+=mktime(stm->tm_hour,mktmmin(stm),buf+len);
   return len;
	}

#ifdef JUGGLUCO_APP
template<typename T>   bool searchhit(const T *ptr); 
extern template   bool searchhit<Num>(const Num *ptr); 
#endif
void	JCurve::shower(NVGcontext* vg,const Num *num,const float xpos,const float xend,const float ypos) {
	if(num->type>=settings->getlabelcount()) {
//		constexpr char const deleted[]="Deleted";
        const char *deleted=usedtext->deleted.data();
        const int dellen=usedtext->deleted.size();
        nvgFillColor(vg, *getgray());
		nvgTextAlign(vg,NVG_ALIGN_CENTER|NVG_ALIGN_TOP); 
		nvgText(vg, (xpos+xend)/2,ypos,deleted,deleted+dellen);
		return;
		}
#ifdef JUGGLUCO_APP
	if(searchhit(num)) {
		float ry=smallfontlineheight/2;
		int xmid=(xpos+xend)/2;
		nvgBeginPath(vg);
		nvgEllipse(vg,xmid , ypos+ry,(xend-xpos)/2+smallsize, textheight/2);
		nvgStroke(vg);
		}
#endif
	constexpr int maxitem=80;
	char item[maxitem];
	time_t tim=num->time;

	int itemlen=datestr2(tim,item);

	item[itemlen++]=' ';
	item[itemlen++]=' ';
	nvgFillColor(vg, *getcolor(num->type));
	decltype(auto) lab=settings->getlabel(num->type);
	memcpy(item+itemlen,lab.data(),lab.size());
	itemlen+=lab.size();
	nvgTextAlign(vg,NVG_ALIGN_LEFT|NVG_ALIGN_TOP);
	item[itemlen]='\0';
	nvgText(vg, xpos,ypos,item,item+itemlen);
	if(isNote(num->type)&&notes) {
		const char *text=notes->gettext(num->mealptr);
		if(*text) {
			char notebuf[noteshortbuf];
			shortnotetext(text,notebuf);
			nvgTextAlign(vg,NVG_ALIGN_RIGHT|NVG_ALIGN_TOP);
			nvgText(vg, xend,ypos,notebuf,notebuf+strlen(notebuf));
			return;
			}
		}
	itemlen=snprintf(item,maxitem, "%.1f",num->value);
	nvgTextAlign(vg,NVG_ALIGN_RIGHT|NVG_ALIGN_TOP);
	nvgText(vg, xend,ypos,item,item+itemlen);
	}

void JCurve::shownums(NVGcontext* vg, NumIter<Num> *numiters, const int nr) {
LOGGER("shownums width=%d height=%d\n",numcontrol.width,numcontrol.height);
	numscreen(vg,[this,numiters,nr](NVGcontext *vg,float xpos,float xend) {
		columnfromabove(vg,[this,vg,numiters,nr,xpos,xend](const float ypos) {
		if(const Num *num=findoldest(numiters,nr,notvali)) {
			shower(vg,num, xpos, xend,ypos);
			return true;
			}
		return false;
		}); });
	}
void JCurve::shownumsback(NVGcontext* vg, NumIter<Num> *numiters, const int nr) {
LOGGER("shownumsback width=%d height=%d\n",numcontrol.width,numcontrol.height);
	numscreenback(vg,[this,numiters,nr](NVGcontext *vg,int rows,float xpos,float xend) {
		columnfrombelow(vg,rows,[this,vg,numiters,nr,xpos,xend](const float ypos) {
		if(const Num *num=findnewest(numiters,nr,notvali)) {
			shower(vg,num, xpos, xend,ypos);
			return true;
			}
		return false;
		}); });
	}

#include "fromjava.h"

extern float listitemlen;

extern void numiterinit() ;
extern int numlist;
int getcolumns(jint width) {
    const float usedwidth=appcurve.portrait
        ? (float)appcurve.surfacewidth-appcurve.dleft-appcurve.dright
        : appcurve.dwidth;
	return ((appcurve.listitemlen*2+width)>usedwidth)?1:2;
	}
extern "C" JNIEXPORT jint JNICALL fromjava(getcolumns)(JNIEnv *env, jclass thiz,jint width) {
	if(!numlist)
		return 2;
      LOGGER("getcolumns %d\n",width);
	return getcolumns(width);
	}
extern "C" JNIEXPORT jint JNICALL fromjava(numcontrol)(JNIEnv *env, jclass thiz,jint width,jint height) {
	if(!numlist)
		return 2;
	numcontrol={width,height};
	nrcolumns= getcolumns(width);
   LOGGER("numcontrol %d nrcolumns=%d\n",width,nrcolumns);
//	numiterinit();
	return nrcolumns;
	}


extern bool	numpageforward();
extern bool	numpagepast();
extern "C" JNIEXPORT void JNICALL fromjava(forwardnumlist)(JNIEnv *env, jclass thiz) {
	numpageforward();
	}

extern "C" JNIEXPORT void JNICALL fromjava(backwardnumlist)(JNIEnv *env, jclass thiz) {
	numpagepast();
	}
#endif
