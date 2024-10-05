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

//#if defined(__aarch64__) 
#if 0
#define NANOVG_GLES3_IMPLEMENTATION
#if __ANDROID_API__ >= 24
#include <GLES3/gl32.h>
#elif __ANDROID_API__ >= 21
#include <GLES3/gl31.h>
#else
#include <GLES3/gl3.h>
#endif
#else
#include <GLES2/gl2.h>
#define NANOVG_GLES2_IMPLEMENTATION
#endif
#include "nanovg.h"
#include "settings/settings.hpp"
extern float smallfontlineheight;
extern float dleft,dtop,dbottom,dright,dheight,dwidth;
extern float density;
extern float textheight;
extern	int font,menufont,monofont;
extern float smallsize,menusize,headsize,midsize, mediumfont;
extern  float foundPointRadius;
extern  float hitStrokeWidth;
constexpr const NVGcolor nvgRGBAf2(const float r, const float g, const float b, const float a)
{
return  {{{r,g,b,a}}};
}

extern int startincolors;
#define constcol static inline constexpr const
constcol NVGcolor darkgrayin= nvgRGBAf2(0,0,0,0.4);
constcol NVGcolor blue=nvgRGBAf2(0, 0,1,1);
constcol NVGcolor lightblue=nvgRGBAf2(0.2, 0.2,1,1);
constcol NVGcolor green=nvgRGBAf2(0,1,0,1);
constcol NVGcolor greenblue=nvgRGBAf2(0,1,1,1);
constcol NVGcolor pink= nvgRGBAf2(1.0,0,1,1.0);
constcol NVGcolor redinit= nvgRGBAf2(1.0,0,0,1.0);
#define red redinit


#define lowlinecolor red



constcol NVGcolor white=nvgRGBAf2(1.0,1.0,1.0,1.0);

constcol NVGcolor black={{{0,0,0,1.0}}};

constcol NVGcolor yellow=nvgRGBAf2(1,1,0,1);
#define mkcolor(r,g,b) {{{r/255.0f,g/255.0f,b/255.0f,1.0f}}}
 constcol  NVGcolor  blue1={{{0x66/255.0f,0x07/255.0f,0xf5/255.0f,1.0f}}};
constexpr const NVGcolor hexcolor(const uint32_t get) {
	return {{{(((get>>16)&0xFF)/255.0f),(0xFF&(get>>8))/255.0f,(get&0xFF)/255.0f,1}}};
	}
constexpr const NVGcolor hexcoloralpha(const uint32_t get) {
	return {{{(((get>>16)&0xFF)/255.0f),(0xFF&(get>>8))/255.0f,(get&0xFF)/255.0f,(get>>24)/255.0f}}};
	}

constcol NVGcolor  mediumseagreen= hexcolor(0x3CB371);
constcol NVGcolor orange=hexcolor(0xFFA500);
constcol NVGcolor brown=hexcolor(0xA52A2A);
constcol NVGcolor blackbean=hexcolor(0x3D0C02);
constexpr const int fromNVGcolor(const NVGcolor *col) {
	return (((int)(col->r*255))<<16)+((int)(col->g*255)<<8)+((int)(col->b*255))+(((int)(col->a*255))<<24);
	}
 constcol auto red1=hexcolor(0xfc1408);
 constcol auto lightblue2=hexcolor(0x08a3fc);
 constcol auto green1=hexcolor(0x08fce0);
 constcol auto green2=hexcolor(0x08fcf4);
 constcol auto green3=hexcolor(0x14fc08);
 constcol auto green4=hexcolor(0x0c9908);
 constcol auto paars=hexcolor(0xfc08dc);
 constcol auto kleur1=hexcolor(0x732a57);
 constcol auto kleur2=hexcolor(0xd6b085);
 constcol auto kleur3=hexcolor(0xe070a8);
 constcol auto kleur4=hexcolor(0x5f6c87);
 constcol auto green5=hexcolor(0x19429);
 constcol auto brown2=hexcolor(0x8a3119);
extern int lasttouchedcolor;
extern int startincolors;
inline const NVGcolor *getcolor(const int col) {
	return settings->data()->colors+startincolors+col;
	}
inline void setcolor(const int colindex,const NVGcolor col) {
	settings->data()->colors[startincolors+colindex]=col;
	}
constexpr const int lightredoffset=startbackground-1;
constexpr const int grayoffset=startbackground-2;
constexpr const int dooryellowoffset=startbackground-3;
constexpr const int threehouroffset=startbackground-4;
constexpr const int darkgrayoffset=11;

inline constexpr const NVGcolor allcolors[]={green5, blue1,kleur4,green,kleur2,lightblue2,red1,kleur3,paars,green1,kleur4, darkgrayin ,green4, greenblue, blue,kleur1, green2, lightblue,green3,brown};
inline constexpr const int oldnrcolors=std::size(allcolors);
inline constexpr const int nrcolors=threehouroffset;
inline const NVGcolor *getblack() {
	if(startincolors)
		return &white;
	return &black;
	}
constexpr const auto darkmenu=hexcolor(0x07518e);
inline const NVGcolor *getmenucolor() {
	if(startincolors)
		return &darkmenu;
	return &black;
	}
inline const NVGcolor *getmenuforegroundcolor() {
	return &white;
	}


constcol NVGcolor 	foregroundgray=  nvgRGBAf2(0,0,0,0.1);
constcol NVGcolor 	backgroundgray= {{{1.0f,1.0f,1.0f,.4f}}}; 

inline const NVGcolor *getgray() {
	if(startincolors)
		return &backgroundgray;
	return &foregroundgray;
	}
constcol NVGcolor backgrounddarkgray=   nvgRGBAf2(.8,.8,.8,.8);

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

constexpr const  NVGcolor invertcolor(const NVGcolor *colin)  {
   constexpr const auto invertcolor=[](float c) -> float {
	return ((uint8_t)roundf(c*255.0f)^0xFF)/255.0f;
	};
	NVGcolor coluit;
	for(int i=0;i<3;i++) 
		coluit.rgba[i]=invertcolor(colin->rgba[i]);
	coluit.a=colin->a;
	return coluit;
	}
constexpr const  NVGcolor invertcolor(const NVGcolor colin)  {
   constexpr const auto invertcolor=[](float c) -> float {
	return ((uint8_t)roundf(c*255.0f)^0xFF)/255.0f;
	};
	NVGcolor coluit;
	for(int i=0;i<3;i++)
		coluit.rgba[i]=invertcolor(colin.rgba[i]);
	coluit.a=colin.a;
	return coluit;
	}
 const auto yellowinvert=invertcolor(yellow);

 const auto pinkinvert=invertcolor(pink);
inline const NVGcolor *getyellow() {
	if(startincolors)
		return &yellowinvert;
	else
		return &yellow;
	}
inline const NVGcolor getoldcolor() {
		return pink;
	}

constcol NVGcolor  foregroundthreehour=  nvgRGBAf2(1.0,0,1,0.5);
constcol NVGcolor  backgroundthreehour=  nvgRGBAf2(1.0,0,1,1);
inline const NVGcolor *getthreehour() {
	if(startincolors)
		return &backgroundthreehour;
	return &foregroundthreehour;
	}

#include "jugglucotext.hpp"
extern NVGcontext* genVG;

inline int mk12time(int hour,int min,char *buf) {
      int hour12 = hour % 12;
      if(!hour12) hour12 = 12;
      int len=sprintf(buf,"%d:%02d",hour12,min);
      if(hour>=12) {
         memcpy(buf+len,"pm",3);
         }
       else
         memcpy(buf+len,"am",3);
       return len+2;
       }

inline int mk24time(int hour,int min,char *buf) {
	return sprintf(buf,"%02d:%02d", hour,min);
      }
inline bool hour24() {
   extern bool hour24clock;
   return hour24clock;
   }
inline int mktime(int hour,int min, char *buf) {
   if(hour24()) 
      return  mk24time(hour,min,buf);
   return  mk12time(hour,min,buf);
   }
inline int datestr(const time_t tim,char *buf) {
	struct tm tmbuf;
	 struct tm *stm=localtime_r(&tim,&tmbuf);
	int len=sprintf(buf,"%s %02d-%02d-%d ",usedtext->daylabel[stm->tm_wday],stm->tm_mday,stm->tm_mon+1,1900+stm->tm_year);
    len+=mktime(stm->tm_hour,stm->tm_min,buf+len);
   return len;
	}




constcol NVGcolor foregroundlightred=  nvgRGBAf2(1, 0.95, 0.95, 1); 
constcol NVGcolor backgroundlightred=   blackbean; 

//#define lightred  *(settings->data()->colors+startincolors+lightredoffset)
#define unsavecolor (startincolors?backgroundlightred:foregroundlightred)

constcol NVGcolor dooryellow=nvgRGBAf2(0.9,0.9,0.1,0.3); 
//extern int whitefont,blackfont;


