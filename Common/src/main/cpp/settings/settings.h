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
/*      Fri Jan 27 12:36:58 CET 2023                                                 */


#pragma once
#include <array>
#ifdef  JUGGLUCO_APP
#include "appcolor.h"
#else
#include "cmdlinecolor.h"
#endif
#include "config.h"
#include "inout.h"
#include "countryunits.h"
#include "curve/jugglucotext.h"
#include "broadcasts.h"
extern int showui;
struct Settings;
extern Settings *settings;
//s/\([0-9a-zA-Z][0-9a-zA-Z]\)/0x\1,/g
constexpr const	uint8_t defaultid[]= {
#ifdef RELEASEID
0xfd,0x29,0x8a,0xf3,0xf2,0xdf,0x4e,0xf0,0x92,0x59,0x3d,0xb8,0x75,0xbb,0xf5,0xc9

#else
0x72,0x12,0xfb,0xfb,0x1f,0x57,0x4b,0xdc,0x84,0x59,0xc0,0x4e,0xfd,0x90,0xa6,0x04

//0x4e,0x94,0xf2,0x4f,0xad,0x8e,0x44,0x73,0x94,0x0c,0x6d,0x62,0xd0,0x1f,0x67,0xdf

#endif
};
extern const char *gformat;
extern int gludecimal;
struct amountalarm {
	float value;
	uint16_t start,alarm,end;
	uint16_t type;
	};
constexpr int maxnumalarms=19;
constexpr int maxuri=128;
struct ring {
	char uri[maxuri];
	uint16_t duration;
	uint16_t wait:12;
	bool novibration:1;
	bool disturb:1;
	bool nosound:1;
	bool flash:1;
	};
constexpr static const int maxcolors=80;
constexpr static const int startbackground=maxcolors/2;
constexpr static const int maxalarms=5;

struct Tings {
	uint32_t glow,ghigh,tlow,thigh,alow,ahigh;
	int32_t duration;
	uint8_t  camerakey:3;
	bool heartrate:1;
	bool crashed:1;
	bool wakelock:1;
	bool xdripbroadcast:1;
	bool jugglucobroadcast:1;
	char initVersion;
	bool invertcolors:1;
	bool dontshowalways:1;
	bool fixatex:1,fixatey:1,systemUI:1,flash:1,waitwithstreaming:1,nfcsound:1;
//	unsigned char reserved1:2;
	bool havelibrary:1;
	bool xinfuus:1;
	bool levelleft:1;
	bool nolog:1;
	bool setpathworks:1;
	bool usegarmin:1;
	bool usexdripwebserver:1;
	bool useWearos:1;
	uint8_t orientation:7;
	bool kerfstokblack:1;
	bool hasgarmin:1;
	bool askedNotify:1;
	bool balanced_priority:1;
	uint8_t keepWifi:1;
	bool remotelyxdripserver:1;
	bool shownintro:1;
	bool triedasm:1;
	bool asmworks:1;
	bool nobluetooth;
	bool nodebug:7;
	bool floatglucose:1;
	bool lowalarm,highalarm,availablealarm;
	bool lossalarm;
	uint8_t watchid[16];
	struct ToLibre {
		int32_t  kind;
		float weight;
		};
	ToLibre librenums[40];
	uint8_t reserved3[192];
	int8_t unit;
	bool callapp;
	bool nochangenum;
	bool sendlabels;
	bool sendcuts;
	uint32_t oldupdate;
	int32_t  alarmnr;
	amountalarm numalarm[maxnumalarms];
	uint8_t reserved2[4];
	uint32_t unlinkstatestime;
	uint32_t update;
	struct Variables {
		float prec;
		float weight;
		char name[12];
		};//20
	int32_t varcount;
	Variables vars[40];
	struct Shortcut {
		char name[12];
		char value[12];	
		};
	int32_t shortnr;
	std::array<Shortcut,40> shorts;
	uint8_t  mealvar;
	uint8_t  reserved4[3];
	float roundto;
	color_t colors[maxcolors];
	int colorscreated;
	struct ring alarms[maxalarms];
	char librebaseurl[128];
	char libreemail[256];
	int8_t librepasslen;
	char librepass[36];
	bool libre3nums:1;//TODO: use
	bool sendnumbers:1;
	bool haslibre2:1;
	bool haslibre3:1;
	bool libreinit3:1;
	bool sendtolibreview:1;
	bool uselibre:1;
	bool libreinit:1;
	uint16_t startlibreview;
	std::array<char,36> libreviewDeviceID;
	char _nullchar;
	uint8_t empty8;
	uint16_t startlibre3view;
	uint32_t empty32;
	uint32_t lastlibretime;
	std::array<char,36> libreviewAccountID;
	char _nullchar1;
	uint8_t empty1[11];//**
	char newYuApiKey[41];
//	char reserved7[7];//*

	char reserved7;//*
	uint16_t libredeletednr;
	int32_t libre2NUMiter;



	uint16_t tokensize;
	char libreviewUserToken[1024];
	int32_t  DateOfBirth;
	uint8_t FirstName[44];
	uint8_t LastName[84];
	uint8_t GuardianLastName[44];
	uint8_t GuardianFirstName[84];
	char libre3baseurl[128];
	std::array<char,36> libre3viewDeviceID;
	char _nullchar2;
	uint8_t empty2[11];//**
	char newYuApiKey3[41];
	//char reserved9[7];//*


	char reserved9[3];//*
	int32_t libre3NUMiter;

	uint16_t tokensize3;
	char libreviewUserToken3[1024];
	BroadcastListeners<5> librelinkBroadcast;
	BroadcastListeners<10> xdripBroadcast;
	BroadcastListeners<10> glucodataBroadcast;
	void setdefault() {
		memcpy(watchid,defaultid,sizeof(watchid));
		};
	};

struct Settings:Mmap<Tings> {
float convertmult;
/*
    public static final int SCREEN_ORIENTATION_LANDSCAPE = 0;
    public static final int SCREEN_ORIENTATION_PORTRAIT = 1;
  public static final int SCREEN_ORIENTATION_REVERSE_LANDSCAPE = 8;
    public static final int SCREEN_ORIENTATION_REVERSE_PORTRAIT = 9;
    */


#if defined(JUGGLUCO_APP)&& !defined(WEAROS)
#define MAKELABELS 1
#endif
int error=0;
Settings(string_view base, string_view file,const char *country): Settings(pathconcat(base,file).data(),base.data(),country) {
	}



int errnotoerror(int errn) {
	switch(errn) {
	case EACCES: return 5;

	case ELOOP: return 6;

	case ENAMETOOLONG: return 7;

	case ENOENT: return 8;

	case ENOTDIR: return 9;

	case EROFS: return 10;

	case EBADF: return 11;

	case EINVAL: return 12;

	case ETXTBSY: return 13;
	default: return 3;
	}
	}

#ifdef MAKELABELS
void mkshorts() {
//	int len= std::size(shortinit);
	int len=usedtext->shortinit.size();
	for(int i=0;i<len;i++) {
		snprintf(data()->shorts[i].value,12,"%.10g",usedtext->shortinit[i].value);
		strcpy( data()->shorts[i].name,usedtext->shortinit[i].name);
		}
	data()->shortnr=len;
	LOGGER("shorts=%d\n",data()->shortnr);
	}
void mklabels() {
	LOGGER("mklabels\n");
	Tings::Variables *varsptr=data()->vars;
	varsptr[0].prec=.5f;
	varsptr[1].prec=1.0f;
	varsptr[2].prec=1.0f;
	varsptr[3].prec=.5f;
	varsptr[4].prec=1.0f;
	varsptr[5].prec=1.0f;
	varsptr[6].prec=.1f;
//	varsptr[6].weight=tomgperL(1.0f);

//	strcpy( data()->vars[i].name,labels[i].data());
	int nrlab=usedtext->labels.size();
	for(unsigned int i=0;i<nrlab;i++) {
		strcpy( varsptr[i].name,usedtext->labels[i].data());
	}
	data()->mealvar=1;
	data()->varcount=nrlab;
	mkshorts() ;
}
#else
#define mklabels() 
#endif
Settings(const char *settingsname,const char *base,const char *country): Mmap(settingsname,1) {
//Settings(string_view base, string_view file,const char *country): settingsfilename(base,file), Mmap(base,file,1) 
	if(!data())  {
		if(access(base, R_OK|W_OK)!=0) {
			error=errnotoerror(errno);
			}
		else {
			if(access(settingsname, R_OK|W_OK)!=0)
				error=4;
			else
				error=2;
			}
		return;
		}

	if(data()->initVersion<13) {
		if(data()->initVersion<12) {
		if(data()->initVersion<10) {
		if(data()->initVersion<9) {
			data()->sendtolibreview=data()->uselibre;
			if(data()->initVersion<8) {
				LOGGER("initVersion=%d\n",data()->initVersion);
				if(data()->initVersion<7) { 
					if(data()->initVersion<6) {
						if(data()->initVersion<4) {
							if(data()->varcount==0) {
								data()->roundto=1.0f;


								mklabels();
								mkalarms();
								/*
								extern bool iswatch;
								if(iswatch) {
									data()->orientation=1;
									data()->invertcolors=true;
									}
								else {
									data()->orientation=8;
									}*/
			#ifdef WEAROS
								data()->orientation=1;
			#else
								data()->orientation=8;
			#endif

								data()->fixatey=true;
								data()->systemUI=true;
								}
							data()->setdefault();
							};
						data()->flash=false;
						}
					data()->usexdripwebserver=false;
			#ifdef CARRY_LIBS
					data()->havelibrary=true;
			#endif
			#ifdef WEAROS
					data()->invertcolors=true;
					data()->usegarmin=false;
			#else
					data()->usegarmin=true;
			#endif
					}

				setdisturbs();
				data()->balanced_priority=true;
				}
			}
			data()->triedasm=false;
			data()->asmworks=false;
			extern bool getpathworks();
			data()->setpathworks=getpathworks();
			data()->initVersion=10;
			}

			if(data()->libre2NUMiter<1) data()->libre2NUMiter=1;
			if(data()->libre3NUMiter<1) data()->libre3NUMiter=1;
			}
		if(data()->xinfuus) {
			strcpy(data()->librelinkBroadcast.name[0], "com.eveningoutpost.dexdrip");
			data()->librelinkBroadcast.nr=1;
			}
		data()->xdripBroadcast.nr=data()->xdripbroadcast;
		data()->glucodataBroadcast.nr=data()->jugglucobroadcast;
		}
	setconvert(country);

	 showui=getui();
	/*
	setnumalarm(0, .5,19*60+30,21*60);
	setnumalarm(3, 7,21*60+30,23*60);
	setnumalarm(0, 5,12*60,13*60);
	setnumalarm(7, 5,11*60,11*60+30);
	setnumalarm(0, .5,13*60+30,15*60); */
}



void		setdisturbs() {
	struct ring *al=data()->alarms;
	al[0].disturb=true;
	al[1].disturb=true;
	al[3].disturb=true;
	}

 void		mkalarms() {
	struct ring *al=data()->alarms;
	for(int i=0;i<maxalarms;i++) {
		al[i].uri[0]='\0';
		al[i].wait=20;
		}
	al[0].duration=6;

	al[1].duration=6;

	al[2].duration=3;

	al[3].duration=6;

	al[4].duration=4;
	}
void setnodebug(bool val) {
	data()->nodebug=val;
	data()->havelibrary=true;
	}


bool staticnum() const ;
bool getnodebug() const {
	return data()->nodebug;
	}
void setconvert(const char *country) {
	int unit=data()->unit;
	if(unit==0) {
		if(country&&*country)
			unit=getunit(country);
		else
			unit=3;
		setalarms(39*18,13*180,true,true,true,true);
		setranges(3*180,12*180,39*18,10*180);
		}
	setunit(unit);
	}
void setlinuxcountry();
uint32_t graphlow() const {
	return data()->glow;
	}
uint32_t graphhigh() const {
	return data()->ghigh;
	}
uint32_t targetlow() const {
	return data()->tlow;
	}
uint32_t targethigh() const {
	return data()->thigh;
	}
bool usemmolL() const {
	return data()->unit==1;
	}
void setunit(int unit) {
	if((data()->unit=unit)==1) {
		convertmult=1.0f/180.0f;
		gformat="%.1f";
		gludecimal=1;
		}
	else  {
		convertmult=1.0f/10.0f;
		gformat="%3.0f";
		gludecimal=0;
		}

	}
bool availableAlarm() const {
	return data()->availablealarm;
	}
bool highAlarm(int val) const {
	if(data()->highalarm&&val>data()->ahigh)
		return true;
	return false;
	}
bool lowAlarm(int val) const {
	if(data()->lowalarm&&val<data()->alow)
		return true;
	return false;
	}
void setranges(uint32_t glow, uint32_t ghigh, uint32_t tlow, uint32_t thigh) {
	data()->glow=glow;data()->ghigh=ghigh;data()->tlow=tlow;data()->thigh=thigh;
	}
void setalarms(uint32_t alow, uint32_t ahigh, bool lowalarm, bool highalarm, bool availablealarm,bool lossalarm) {
	data()->alow=alow;data()->ahigh=ahigh;
	data()->lowalarm=lowalarm;
	data()->highalarm=highalarm;
	data()->lossalarm=lossalarm;
	data()->availablealarm=availablealarm;
	}


//constexpr static string_view labels[]={"NovoRapid","Carbohydrat","Dextro","Levemir","Fietsen","Lopen","Blood"};
//constexpr static string_view labels[]={"Insulin Rap","Carbohydrat","Dextro","Insulin Slow","Bike","var6","var7","var8"};
/*
constexpr static string_view labels[]={"Aspart","Carbohydrat","Dextro","Levemir","Bike","Walk","Blood","var8"};
constexpr static struct {const char name[12];const float value;}  shortinit[]= { {"Bread",
        .376f},
        {"Currantbun1",
        .56f},
        {"Currantbun2",
        .595f},
        {"Grapes",
        .165f},
        {"FruitYog",
        .058f},
        {"Rice",
        .75f},
        {"Macaroni",
        .65f},
        {"Tomato",
        .03f},
        {"Mexican mix",
        .078f},
        {"OrangeJuice",
        .109f},
        {"SportPowder",
        .873f},
        {"Mix(Carrot)",
        .07f},
        {"Mix mushro",
        .07300000f}};

struct Shortcut_t {const char name[12];const float value;} 

constexpr static string_view itlabels[]= {"Rapida","Carboidrati","Glucosio","Lenta","Bike","Walk","Capillare","var8"};
constexpr static struct {const char name[12];const float value;}  itshortinit[]= { {"Muffin", .54f},

{"Uva",

.165f},

{"YogFrutta",

.058f},

{"Riso", .75f},

{"Pasta", .65f},

{"Pomodoro",

.03f},

{"Messicano",

.078f},

{"SuccoArancia",

.109f},

{"Mix(Carote)",

.07f},

{"Mix funghi",

.07300000f}};
*/

/*
void mklabels() {
for(int i=0;i<8;i++) {
    snprintf( data()->vars[i].name,12,"var%d",i);
    }

data()->varcount=8;
}
*/
int getlabelcount()const {
	return data()->varcount;
	}
int getshortcutcount()const {
	return data()->shortnr;
	}
void setshortcutcount(int nr) {
	data()->shortnr=(nr<data()->shorts.size())?nr:(data()->shorts.size()-1);
	data()->sendcuts=true;
	}


static constexpr const string_view unknownlabel{"Unspecified"};
const string_view getlabel(const int index) const  {
	if(index<getlabelcount())
		return data()->vars[index].name;
	return unknownlabel;
	}
void setlabel(const int index,const char *name)   {
	strncpy(data()->vars[index].name,name,11);
	data()->sendlabels=true;
	}
void setlabel(const int index,const char *name,float prec,float weight)   {
	strncpy(data()->vars[index].name,name,11);
	data()->vars[index].prec=prec;
	data()->vars[index].weight=tomgperL(weight);
	if(index>=varcount())
		varcount()=index+1;
	data()->sendlabels=true;
	}
int &varcount() {
	return data()->varcount;
	}
void setlabel(const char *name)   {
	strncpy(data()->vars[varcount()++].name,name,11);
	data()->sendlabels=true;

	}

const float getlabelweight(const int index) const  {
	return frommgperL(data()->vars[index].weight);
	}
const float getlabelweightmgperL(const int index) const  {
	return data()->vars[index].weight;
	}
const float getlabelprec(const int index) const  {
	return data()->vars[index].prec;
	}
float tomgperL(const float unit) const {
	return unit/convertmult;
	}
float frommgperL(const float mgperL) const {
	return mgperL*convertmult;
	}
string_view getunitlabel() const {
	return data()->unit==1?unitlabels[1]:unitlabels[2];
	}

bool getui()const  {
	return data()->systemUI;
	}
void setui(bool showui) {
	data()->systemUI=showui;
	}

const	uint32_t getupdate()const  {
		return data()->update;
		}
void updated()  {
		 data()->update++;
		}

uint32_t firstAlarm()const  ;
std::vector<int> numAlarmEvents() const;
void setnumalarm(uint16_t type,float value,uint16_t start,uint16_t alarm) ;
void delnumalarm(int pos) ;
};

inline float gconvert(const float mgperL) {
	return settings->frommgperL(mgperL);
	}
inline  const float backconvert(const float unit) {
	return settings->tomgperL(unit);
	}
inline constexpr const char settingsdat[]="settings.dat";
inline bool waitstreaming() {
	return settings->data()->waitwithstreaming;
	}

inline uint16_t &getlibrenumsdeletednr() {
	return settings->data()->libredeletednr;
	}

constexpr const int librekeepsecs=89*24*60*60;
