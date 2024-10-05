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

#include "meal/Meal.hpp"

extern Meal *meals;
extern int carbotype;
extern int showmeals;
extern int *numheights;
extern int shownumbers;
extern NVGcolor *lastcolor;
extern int lasttouchedcolor;
//s/^\(.*\)=.*/extern \1;/g
struct mealposition {
	float mealx;
	float mealstarty;
	float mealendy;
	int mealpos;
	int mealbase;
	};
extern vector<mealposition> mealpos;

extern int statusbarheight;
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
inline bool nearby(float dx,float dy) {
	
	const float gr= density*
#ifdef WEAROS
	22;
#else
	24;
#endif
	const float grens=gr*gr;
	float afst=dx*dx+dy*dy;
	return afst<grens;
	}	
//extern  NVGcolor *colors[];
#include "numdisplay.hpp"
void NumDisplay::standout(NVGcontext *vg,float x,float y)  {
	float r=foundPointRadius/2;
	nvgRect(vg, x-r,  y-r, 2*r, 2*r);

	}

pair<int,int> NumDisplay::extremenums(const pair<const int,const int> extr) const {
	const Numdata *numdata=this;
	int gmin=extr.first;
	int gmax=extr.second;
//	const float * const labelweight=numdata->getweights();
	for(const Num *it=extrum.first;it!=extrum.second;it++) {
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
void	NumDisplay::mealdisplay(float x,float y,const Num *num) const {

	mealdata   *mdata=meals->datameal();
	mealdata::mealel   *m=mdata->themeals;
  	const uint32_t mealptr=num->mealptr;

//	int nr=m[mealptr].ingr; if(nr>mealptr) return;
	int nr=mdata->itemsinmeal(mealptr);
	if(!nr)
		return;
	int start=mealptr-nr;
	float mealstarty=y-3*smallfontlineheight;
	
	y+=smallfontlineheight;
	for(int i=start;i<mealptr;i++) {
		nvgText(genVG, x,y,meals->datameal()->ingredients[m[i].ingr].name.data(),NULL );
		y+=smallfontlineheight;
		}
	mealpos.emplace_back(mealposition{x,
	mealstarty,
	y,
	(int)(num-startdata()),
	getindex()}
	);
	}
/*
static inline int mktmmin(const struct tm *tmptr) {
	return tmptr->tm_min;
	} */

#ifdef WEAROS
const
#endif
bool speakout=false;
extern void speak(const char *message) ;
template <class TX,class TY> void NumDisplay::showNums(NVGcontext* vg, const TX &transx,  const TY &transy,bool *was) const {
	auto [low,high]=extrum; 
	const Numdata *numdata=this;
//	bool was[catnr-1];memset(was,0,sizeof(was));
//	const float * const labelweight=numdata->getweights();
	for(const Num *it=low;it!=high;it++) {
		if((shownumbers||(showmeals&&it->type==carbotype))&&numdata->valid(it)) {
			int colorindex= it->type;
			const NVGcolor *colo= getcolor(colorindex);
			nvgFillColor(vg,*colo );
			nvgTextAlign(vg,NVG_ALIGN_CENTER|NVG_ALIGN_MIDDLE);
			float xpos= transx(it->time);
			float ypos;
			if(settings->getlabelweightmgperL(it->type)) {
				ypos= transy(it->value*settings->getlabelweightmgperL(it->type));
				nvgBeginPath(vg);
				standout(vg, xpos,ypos);
				nvgFill(vg);
				}
			else {
				ypos=numtypeheight(it->type);
				constexpr int maxbuf=20;
				char buf[maxbuf];	
				nvgText(vg, xpos,ypos, buf, buf+ snprintf(buf,maxbuf,"%g",it->value));
				if(showmeals&&it->type==carbotype) {
					mealdisplay(xpos,ypos,it);	
					}
				}

			int buflen=0;
			const bool hit=searchdata(it);
			if(hit) {
				nvgBeginPath(vg);
				nvgStrokeWidth(vg,numcircleStrokeWidth);
				nvgStrokeColor(vg, *colo);
				nvgCircle(vg, xpos,ypos,smallsize);
				nvgStroke(vg);

			}
				if(nearby(xpos-tapx,ypos-tapy)) { 
						{
					if(selshown)
						continue;
					 constexpr int maxbuf=50;
					 char buf[maxbuf];
					 const time_t tim= it->time;
					 struct tm *tms=localtime(&tim);
					lasttouchedcolor=colorindex;
//					buflen=snprintf(buf,maxbuf,"%02d:%02d", tms->tm_hour, mktmmin(tms));
					buflen=mktime(tms->tm_hour, mktmmin(tms),buf);
					char *buf2=buf+buflen;
					if(settings->getlabelweightmgperL(it->type))  {
						constexpr const int maxbuf2=10;
						nvgText(vg, xpos,ypos+(((ypos-dtop)<(dheight/2))?1:-1)*(hit?2.4:2)*smallsize, buf, buf+buflen);
						const int len=snprintf(buf2,maxbuf2,"%.1f", it->value);
						sidenum(xpos,ypos,buf2,len,hit);
						nvgTextAlign(vg,NVG_ALIGN_CENTER|NVG_ALIGN_MIDDLE);
						}
					else
						nvgText(vg, xpos,ypos-(hit?2.4:2)*smallsize, buf, buf+buflen);

#ifndef WEAROS
					if(speakout) {
						sprintf(buf2,"\n%s\n%g",settings->getlabel(it->type).data(),it->value);
						speak(buf);
						}
#endif

					}
				selshown=true;
				}
			if((!was[it->type] &&(!showmeals||it->type!=carbotype)) ||buflen) {
				was[it->type]=true;
				string_view label=settings->getlabel(it->type);
				//TODO remove:
				if(const char *name=label.data()) {
					nvgText(vg, xpos,ypos+((settings->getlabelweightmgperL(it->type)&&((ypos-dtop)<(dheight/2)))?1:-1)*(hit?smallsize*1.4:smallsize),name,name+label.size());
					}
				else {
					time_t tim=it->time;
					LOGGER("NULL label: %.1f %d %s",it->value,it->type,ctime(&tim));
					}
				}

			}
		}
	}




const Num * NumDisplay::findpast(const Num *high) const {
	const Num *low=begin();
	for(const Num *it=high-1;it>=low;it--) 
		if(searchdata(it))
			return it;
	return nullptr;
	}
template <int N>
const Num * NumDisplay::findpast() const {
	return findpast(std::get<N>(extrum));
	}
template <int N>
const Num * NumDisplay::findforward() const {
	const Num *en=end();
	const Num *start=std::get<N>(extrum);
	if(const Num *hit=find_if(start,en,searchdata);hit<en) {
		return hit;
		}
	return nullptr;
	}



 NumDisplay*  NumDisplay::getnumdisplay(string_view base,identtype ident,size_t len)   {
	if(mknumdata(base,ident))
		return new NumDisplay(base,ident,len);
	return nullptr;
	}

template <class TX,class TY>  const Num * NumDisplay::getnearby(const TX &transx,  const TY &transy,const float tapx,const float tapy) const {
	auto [low,high]=extrum; 
	for( const Num *it=low;it!=high;it++) {
		if(valid(it)) {
			float xpos= transx(it->time);
			float ypos=settings->getlabelweightmgperL(it->type)?transy(it->value*settings->getlabelweightmgperL(it->type)):numtypeheight(it->type);
			if(nearby(xpos-tapx,ypos-tapy)) 
				return it;

			}
		}
	return nullptr;
	}

