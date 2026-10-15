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


#pragma once
#include <vector>
#include "nums/numdata.hpp"
#include "curve.hpp"
extern int shownlabels;

extern bool selshown;
extern float tapx,tapy;
#include "meal/Meal.hpp"
#include "searchgegs.hpp"
#include "../notes/Notes.hpp"
extern Meal *meals;
extern Notes *notes;
extern int carbotype;
extern int *numheights;
extern int shownumbers;
extern NVGcolor *lastcolor;
extern int lasttouchedcolor;
//s/^\(.*\)=.*/extern \1;/g
//extern vector<mealposition> mealpos;
#include "JCurve.hpp"
//extern  NVGcolor *colors[];
#include "misc.hpp"
#include "numdisplay.hpp"
inline void NumDisplay::standout(JCurve &jcurve,float x,float y)  {
	float r=jcurve.foundPointRadius/2;
	nvgRect(jcurve.thevg, x-r,  y-r, 2*r, 2*r);

	}

pair<int,int> NumDisplay::extremenums(JCurve &j,const pair<const int,const int> extr) const {
	const Numdata *numdata=this;
	int gmin=extr.first;
	int gmax=extr.second;
//	const float * const labelweight=numdata->getweights();
        auto [first,second]=j.extrums[numdatasPos];
	for(const Num *it=first;it!=second;it++) {
		if(numdata->valid(it)) {
			if(const float w=settings->getlabelweightmgperL(it->type)) {
				const int val=w*it->value;
				if(val>gmax) {
					LOGGER("%d>%d\n",val,gmax);
					gmax=val;
					}
				if(val<gmin) {
					LOGGER("%d<%d\n",val,gmin);
					gmin=val;
					}
				}
		}
	   }
	 LOGGER("extreamenums(%d,%d)->(%d,%d)\n",extr.first,extr.second,gmin,gmax);
	  return pair<int,int>(gmin,gmax);
	  }
void	NumDisplay::mealdisplay(JCurve &jcurve,float x,float y,const Num *num) const {
    

	mealdata   *mdata=meals->datameal();
	mealdata::mealel   *m=mdata->themeals;
  	const uint32_t mealptr=num->mealptr;

	int nr=mdata->itemsinmeal(mealptr);
	if(!nr)
		return;
	int start=mealptr-nr;
	auto smallfontlineheight=jcurve.smallfontlineheight;
	if(jcurve.portraitReadable()) {
		/*
		 * In readable portrait, drawText() keeps the glyphs upright while the
		 * graph itself is quarter-turned. Logical -X is therefore physical down.
		 * Advancing logical Y, as landscape does, puts meal elements beside and
		 * over the meal amount. Advance -X instead so they form a vertical list.
		 */
		const float mealstartx=x-(nr+1)*smallfontlineheight;
		const float mealendx=x+3*smallfontlineheight;
		float textx=x-smallfontlineheight;
		for(int i=start;i<mealptr;i++) {
			jcurve.drawText(jcurve.thevg,textx,y,
				meals->datameal()->ingredients[m[i].ingr].name.data(),NULL);
			textx-=smallfontlineheight;
			}

		/* Rotated equivalent of the landscape meal hit strip. */
		jcurve.mealpos.emplace_back(mealposition{y,mealstartx,mealendx,
			(int)(num-startdata()),getindex()});
		}
	else {
		float mealstarty=y-3*smallfontlineheight;
		y+=smallfontlineheight;
		for(int i=start;i<mealptr;i++) {
			jcurve.drawText(jcurve.thevg,x,y,
				meals->datameal()->ingredients[m[i].ingr].name.data(),NULL);
			y+=smallfontlineheight;
			}
		jcurve.mealpos.emplace_back(mealposition{x,mealstarty,y,
			(int)(num-startdata()),getindex()});
		}
	}
/*
static inline int mktmmin(const struct tm *tmptr) {
	return tmptr->tm_min;
	} */

#ifdef DONTTALK
const
#endif
extern bool speakout;
extern void speak(const char *message) ;
template <class TX,class TY> void NumDisplay::showNums(JCurve&jcurve, const TX &transx,  const TY &transy,bool *was) const {
        NVGcontext* vg=jcurve.thevg;
	auto [low,high]=jcurve.extrums[numdatasPos];
	const Numdata *numdata=this;
        LOGGER("showNums number= %d\n",high-low);
	for(const Num *it=low;it!=high;it++) {
		if((jcurve.shownumbers||(jcurve.showmeals&&it->type==carbotype))&&numdata->valid(it)) {
			int colorindex= it->type;
			const NVGcolor *colo= jcurve.getcolor(colorindex);
			nvgFillColor(vg,*colo );
			nvgTextAlign(vg,NVG_ALIGN_CENTER|NVG_ALIGN_MIDDLE);
			float xpos= transx(it->time);
			float ypos;
			const bool weighted=settings->getlabelweightmgperL(it->type);
			if(weighted) {
				ypos= transy(it->value*settings->getlabelweightmgperL(it->type));
				nvgBeginPath(vg);
				standout(jcurve, xpos,ypos);
				nvgFill(vg);
				}
			else {
				ypos=jcurve.numtypeheight(it->type);
				constexpr int maxbuf=20;
				char buf[maxbuf];	
				if(isNote(it->type) && notes) {
					const char* text = notes->gettext(it->mealptr);
					if(*text) {
						char notebuf[noteshortbuf];
						shortnotetext(text, notebuf);
						nvgText(vg, xpos, ypos, notebuf, notebuf+strlen(notebuf));
					} else {
						nvgText(vg, xpos,ypos, buf, buf+ snprintf(buf,maxbuf,"%g",it->value));
					}
				}
				else
					nvgText(vg, xpos,ypos, buf, buf+ snprintf(buf,maxbuf,"%g",it->value));
				if(jcurve.showmeals&&it->type==carbotype) {
					mealdisplay(jcurve,xpos,ypos,it);	
					}
				}

			int buflen=0;
#ifdef DOESSEARCH
			const bool hit=searchdata(it);
			if(hit) {
				nvgBeginPath(vg);
				nvgStrokeWidth(vg,jcurve.numcircleStrokeWidth);
				nvgStrokeColor(vg, *colo);
				nvgCircle(vg, xpos,ypos,jcurve.smallsize);
				nvgStroke(vg);

			}
#else
        constexpr const bool hit=false;
#endif
			if(nearby(xpos-tapx,ypos-tapy,jcurve.density)) {
				if(selshown)
					continue;
				constexpr int maxbuf=50;
				char buf[maxbuf];
				const time_t tim= it->time;
				struct tm *tms=localtime(&tim);
				jcurve.lasttouchedcolor=colorindex;
				buflen=mktime(tms->tm_hour, mktmmin(tms),buf);
				char *buf2=buf+buflen;

				if(jcurve.portraitReadable()) {
					/*
					 * In portrait, keep the complete touched-number annotation in
					 * normal reading orientation and stack it like the landscape
					 * presentation reads visually:
					 *
					 *       time
					 *       label        (drawn below, after this block)
					 *       value
					 *
					 * Logical +X is physical upward after the portrait graph turn,
					 * hence the positive X offsets for label/time.
					 */
					const float linegap=jcurve.smallfontlineheight*.95f;
					nvgTextAlign(vg,NVG_ALIGN_CENTER|NVG_ALIGN_MIDDLE);
					jcurve.drawText(vg,xpos+2.0f*linegap,ypos,buf,buf+buflen);
					if(weighted) {
						constexpr const int maxbuf2=10;
						const int len=snprintf(buf2,maxbuf2,"%.1f", it->value);
						jcurve.drawText(vg,xpos,ypos,buf2,buf2+len);
						}
					}
				else if(weighted)  {
					constexpr const int maxbuf2=10;
					jcurve.drawText(vg, xpos,ypos+(((ypos-jcurve.dtop)<(jcurve.dheight/2))?1:-1)*(hit?2.4:2)*jcurve.smallsize, buf, buf+buflen);
					const int len=snprintf(buf2,maxbuf2,"%.1f", it->value);
					jcurve.sidenum(vg,xpos,ypos,buf2,len,hit);
					nvgTextAlign(vg,NVG_ALIGN_CENTER|NVG_ALIGN_MIDDLE);
					}
				else
					jcurve.drawText(vg, xpos,ypos-(hit?2.4:2)*jcurve.smallsize, buf, buf+buflen);

#ifndef DONTTALK
				if(speakout) {
                        std::string_view label=settings->getlabel(it->type);
                        const int maxbuf= label.size()+5;
                        char rtllabel[maxbuf];
                        rtl_to_logical_utf8(label.data(), rtllabel,maxbuf) ;
 					char valbuf[noteshortbuf];
 					if(isNote(it->type)&&notes) {
 						const char *text=notes->gettext(it->mealptr);
 						if(*text)
 							shortnotetext(text,valbuf);
 						else
 							snprintf(valbuf,sizeof(valbuf),"%g",it->value);
 						}
 					else
 						snprintf(valbuf,sizeof(valbuf),"%g",it->value);
 					sprintf(buf2,"\n%s\n%s",rtllabel,valbuf);
					speak(buf);
					}
#endif

				selshown=true;
				}

			/*
			 * Landscape keeps the old behaviour: show one label per type even
			 * when nothing is selected.  In portrait labels are useful only as
			 * part of a selected value's annotation, where the narrow screen has
			 * enough structure to keep time/label/value from colliding.
			 */
			if((jcurve.portraitReadable()&&buflen)||
			   (!jcurve.portraitReadable()&&((!was[it->type] &&(!jcurve.showmeals||it->type!=carbotype)) ||buflen))) {
				was[it->type]=true;
				string_view label=settings->getlabel(it->type);
				//TODO remove:
				if(const char *name=label.data()) {
					if(jcurve.portraitReadable()) {
						const float linegap=jcurve.smallfontlineheight*.95f;
						nvgTextAlign(vg,NVG_ALIGN_CENTER|NVG_ALIGN_MIDDLE);
						jcurve.drawText(vg,xpos+linegap,ypos,name,name+label.size());
						}
					else
						jcurve.drawText(vg, xpos,ypos+((settings->getlabelweightmgperL(it->type)&&((ypos-jcurve.dtop)<(jcurve.dheight/2)))?1:-1)*(hit?jcurve.smallsize*1.4:jcurve.smallsize),name,name+label.size());
					}
				else {
                #ifndef NOLOG
					time_t tim=it->time;
					LOGGER("NULL label: %.1f %d %s",it->value,it->type,ctime(&tim));
                #endif
					}
				}

			}
		}
	}





 NumDisplay*  NumDisplay::getnumdisplay(int index,string_view base,identtype ident,size_t len)   {
	if(mknumdata(base,ident))
		return new NumDisplay(index,base,ident,len);
	return nullptr;
	}


