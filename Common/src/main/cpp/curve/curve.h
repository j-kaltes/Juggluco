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
#include "settings/settings.h"
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
//constexpr const NVGcolor black={{{0,0,0,1.0}}};
//extern NVGcolor black;

//extern NVGcolor gray;
//extern NVGcolor yellow;

extern int startincolors;
#define constcol static inline constexpr const
//constcol NVGcolor transred= nvgRGBAf(1.0,0,0,0.06);
constcol NVGcolor darkgrayin= nvgRGBAf2(0,0,0,0.4);
constcol NVGcolor blue=nvgRGBAf2(0, 0,1,1);
constcol NVGcolor lightblue=nvgRGBAf2(0.2, 0.2,1,1);
//constcolcol NVGcolor doorblue=nvgRGBAf(0, 0,1,0.04);
constcol NVGcolor green=nvgRGBAf2(0,1,0,1);
constcol NVGcolor greenblue=nvgRGBAf2(0,1,1,1);
////////////////constcol NVGcolor brown= nvgRGBAf2(.4,.6,0,1.0);
constcol NVGcolor pink= nvgRGBAf2(1.0,0,1,1.0);
constcol NVGcolor redinit= nvgRGBAf2(1.0,0,0,1.0);
//#define red *(settings->data()->colors+startincolors+redoffset)
#define red redinit

#define lowlinecolor red
//#define oldcolor   *(settings->data()->colors+startincolors+pinkoffset)
#define lightred  *(settings->data()->colors+startincolors+lightredoffset)

//constcol NVGcolor nowcolor=nvgRGBAf(1,1,0,0.1);
//NVGcolor &unsavecolor=lightred; //{79.0f/256.0f, 4.0f/256.0f, 9.0f/256.0f,1.0f};
//const NVGcolor yellow=nvgRGBAf(0.9,0.9,0.1,1);
#define dooryellow *(settings->data()->colors+startincolors+dooryellowoffset)
#define unsavecolor lightred

constcol NVGcolor white=nvgRGBAf2(1.0,1.0,1.0,1.0);

// constcol NVGcolor white={{{0,0,0,1.0}}};
//NVGcolor datecolor=nvgRGBAf(204.0f/255.0f,102.0f/255.0f,0,0.2);
constcol NVGcolor black={{{0,0,0,1.0}}};

//constcol NVGcolor orange=nvgRGBAf2(1,.5,0,.5);
//constcol  NVGcolor gray= nvgRGBAf2(0,0,0,0.1);
constcol NVGcolor yellow=nvgRGBAf2(1,1,0,1);
#define mkcolor(r,g,b) {{{r/255.0f,g/255.0f,b/255.0f,1.0f}}}
 constcol  NVGcolor  blue1={{{0x66/255.0f,0x07/255.0f,0xf5/255.0f,1.0f}}};
constexpr const NVGcolor hexcolor(const uint32_t get) {
	return {{{(((get>>16)&0xFF)/255.0f),(0xFF&(get>>8))/255.0f,(get&0xFF)/255.0f,1}}};
	}
constexpr const NVGcolor hexcoloralpha(const uint32_t get) {
	return {{{(((get>>16)&0xFF)/255.0f),(0xFF&(get>>8))/255.0f,(get&0xFF)/255.0f,(get>>24)/255.0f}}};
	}
/*
struct NVGcolor {
	union {
		float rgba[4];
		struct {
			float r,g,b,a;
		};
	};*/

constcol NVGcolor  mediumseagreen= hexcolor(0x3CB371);
constcol NVGcolor orange=hexcolor(0xFFA500);
constcol NVGcolor brown=hexcolor(0xA52A2A);
//constcol NVGcolor basaltblack=hexcolor(0x4D423E);
constcol NVGcolor blackbean=hexcolor(0x3D0C02);
//Hex #3D0C02
//RGB 77, 66, 62
//constcol NVGcolor brown= nvgRGBAf2(.4,.6,0,1.0);
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
// auto kleur5=hexcolor(0x7d232f);
// auto kleur7=hexcolor(0x514e5c);
 //auto kleur6=hexcolor(0xeafc5d);
/*
lightblue #08a3fc
red:#fc1408
green: #08fce0
#08fcf4
green: #14fc08
#0c9908
paars: #fc08dc
#732a57
#d6b085
#e070a8
#5f6c87
#7d232f
#eafc5d
#514e5c*/
//extern   NVGcolor *colors[];
//constcol NVGcolor *colors[]={ &pink,&brown,& blue,& green,& lightblue,&black,&greenblue,&red};
//constcol NVGcolor *colors[]={ &pink,&blue1,&darkgray,& blue,& green,& lightblue,&brown,&greenblue,&red};
//const NVGcolor *colors[]={ &red1,&blue1,&darkgray,& blue,& green,& lightblue,&brown,&greenblue,&red};
// NVGcolor *colors[]={&green5, &blue1,&kleur4,&green,&kleur2,&lightblue2,&red,&kleur3,&paars,&green1,&kleur4, &darkgray ,&green4, &greenblue,& blue,&kleur1,& green2,& lightblue,&green3,&brown,&red1};
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
//inline constexpr const NVGcolor allcolors[]={green5, blue1,kleur4,green,kleur2,lightblue2,red1,kleur3,paars,green1,kleur4, darkgrayin ,green4, greenblue, blue,kleur1, green2, lightblue,green3,brown,redinit,pink,yellow,white,black};
inline constexpr const int oldnrcolors=std::size(allcolors);
inline constexpr const int nrcolors=threehouroffset;
/*
constexpr const int blackoffset=std::size(allcolors)-1;
constexpr const int whiteoffset=std::size(allcolors)-2;
constexpr const int yellowoffset=std::size(allcolors)-3;
constexpr const int pinkoffset=std::size(allcolors)-4;
constexpr const int redoffset=std::size(allcolors)-5;
*/
inline const NVGcolor *getblack() {
	if(startincolors)
		return &white;
	return &black;
//	return getcolor(blackoffset);
	}
constexpr const auto darkmenu=hexcolor(0x07518e);
inline const NVGcolor *getmenucolor() {
	if(startincolors)
		return &darkmenu;
	return &black;
//	return getcolor(blackoffset);
	}
inline const NVGcolor *getmenuforegroundcolor() {
	return &white;
//	return getcolor(blackoffset);
	}
inline const NVGcolor *getgray() {
	return getcolor(grayoffset);
	}
inline const NVGcolor *getdarkgray() {
	return getcolor(darkgrayoffset);
	}
inline const NVGcolor *getwhite() {
	if(startincolors)
		return &black;
	return &white;
//	return getcolor(whiteoffset);
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
//	return getcolor(yellowoffset);
	}
inline const NVGcolor getoldcolor() {
/*
	if(startincolors)
		return pinkinvert;
	else */
		return pink;
	}

inline const NVGcolor *getthreehour() {
	return getcolor(threehouroffset);
	}
/*
struct NVGcolor {
        union {
                float rgba[4];
                struct {
                        float r,g,b,a;
                };
        };
};
typedef struct NVGcolor NVGcolor;
NVGcolor nvgRGBAf(float r, float g, float b, float a)
{
        NVGcolor color;
        // Use longer initialization to suppress warning.
        color.r = r;
        color.g = g;
        color.b = b;
        color.a = a;
        return color;
}


*/

//constexpr const char daylabel[][4]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
#include "jugglucotext.h"
inline int datestr(const time_t tim,char *buf) {
	struct tm tmbuf;
	 struct tm *stm=localtime_r(&tim,&tmbuf);
      // struct tm *stm=localtime(&tim);
//	return sprintf(buf,"%s %02d-%02d-%d %02d:%02d",usedtext->daylabel[stm->tm_wday],stm->tm_mday,stm->tm_mon+1,1900+stm->tm_year,stm->tm_hour,stm->tm_min+((stm->tm_sec>=30)?1:0));
	return sprintf(buf,"%s %02d-%02d-%d %02d:%02d",usedtext->daylabel[stm->tm_wday],stm->tm_mday,stm->tm_mon+1,1900+stm->tm_year,stm->tm_hour,stm->tm_min);
	}

