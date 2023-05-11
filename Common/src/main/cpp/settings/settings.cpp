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


#include "logs.h"
#include "datbackup.h"
#include "sensoren.h"
#include "settings.h"
#include "config.h"
#ifdef USE_MEAL
#include "meal/Meal.h"
#endif
#include <string.h>

//string_view filesdir;
//int filesdirlen;
//char *filesdir;
std::string_view globalbasedir;
pathconcat numbasedir;
pathconcat sensorbasedir;
pathconcat logbasedir;
Sensoren *sensors=nullptr;
unique_ptr<Sensoren> destructsensors;
extern int carbotype;
int carbotype;

extern bool iswatch;
bool iswatch=false;

#ifdef USE_MEAL
Meal *meals=nullptr;
#endif

  #include <signal.h>
#ifndef NOLOG 
extern bool dolog;
void handlepipe(int sig) {
	const int waserrno=errno;
	LOGSTRING("SIGPIPE happened\n");
	errno=waserrno;
	}
#else
#define handlepipe SIG_IGN
#endif
void	generalsettings() {
	signal(SIGPIPE,handlepipe);
	}
int setfilesdir(const string_view filesdir,const char *country) {
	LOGGER("setfilesdir %s %s\n",filesdir.data(),country?country:"null");
	globalbasedir=filesdir;
	numbasedir.set(globalbasedir,"nums");
	sensorbasedir.set(globalbasedir,"sensors");
	logbasedir.set(globalbasedir,"logs");
 	mkdir(logbasedir.data(),0700);
 	mkdir(numbasedir.data(),0700);
 	mkdir(sensorbasedir.data(),0700);
	settings= new(std::nothrow) Settings(globalbasedir,settingsdat,country);
	if(!settings) {
		return 1;
		}
	if(!settings->data()) {
		return settings->error;
		}
#ifdef LIBRE3
extern	void usepath(std::string_view,std::string_view );
extern std::string_view libdirname;
	usepath(libdirname,filesdir);
#endif
	LOGGER("PATH=%s\n",getenv("PATH"));
#ifdef LIBRENUMBERS
	 extern void initlibreview() ;
	 initlibreview() ;
#endif
	if(settings->data()->crashed)
		settings->setnodebug(false);
	generalsettings();
	return 0;
	}
int startmeals() {
	if(!settings) {
		return 1;
		}
	if(!settings->data()) {
		return 2;
		}
	carbotype=settings->data()->mealvar;
#ifndef NOLOG 
	dolog=!settings->data()->nolog;
#endif
#ifdef USE_MEAL
	if(meals==nullptr)
		meals= new(std::nothrow) Meal();
#endif
	return 0;
	}
void startsensors() {
	if(!sensors) {
		LOGSTRING("voor sensors\n");
		Sensoren::create(sensorbasedir);
		LOGSTRING("na creat\n");
		sensors= new(std::nothrow) Sensoren(sensorbasedir);
		if(!sensors)
			return ;
		destructsensors.reset(sensors);
		LOGSTRING("Na sensors\n");
		/*
		if(settings->data()->initVersion<11) {
			sensors->setversions();
			settings->data()->initVersion=11;
			} */
		}
	}


extern void			startlibrethread();
extern void startthreads();
void startthreads() {
	Backup::startbackup(globalbasedir); 
#ifndef WEAROS
#ifdef ANDROID__APP

	       if(settings->data()->usexdripwebserver) {
			void startwatchthread() ;
			startwatchthread();
			}
#ifndef DONT_USE_LIBREVIEW
		if(settings->data()->uselibre) {
			startlibrethread();
			}
#endif

#endif
#endif
	}


	

Settings *settings=nullptr;
const char *gformat;
int gludecimal=1;



uint32_t Settings::firstAlarm() const {
	const int nr=data()->alarmnr;
	if(!nr)
		return 0;
	const time_t tim=time(nullptr);
	const amountalarm *alarms=data()->numalarm;
//	const time_t tim=time(nullptr);
	struct tm nutm;
    	localtime_r(&tim, &nutm);
	uint16_t nu=nutm.tm_hour*60+nutm.tm_min;
	nutm.tm_hour=0;
	nutm.tm_min=0;
	nutm.tm_sec=0;
	time_t startday=mktime(&nutm);
	for(int i=0;i<nr;i++) {
		if(alarms[i].alarm>nu) {
			return (alarms[i].alarm*60LL+startday);
			}
		}
	return ((alarms[0].alarm+24LL*60LL)*60LL+startday);
	}

std::vector<int> Settings::numAlarmEvents()const  {
	const time_t nutimet=time(nullptr);
	static uint32_t prevtime{};
	const amountalarm *alarms=data()->numalarm;
	const int nr=data()->alarmnr;
	struct tm nutm;
    	localtime_r(&nutimet, &nutm);
	uint16_t nu=nutm.tm_hour*60+nutm.tm_min;
	nutm.tm_hour=0;
	nutm.tm_min=0;
	nutm.tm_sec=0;
	time_t startday=mktime(&nutm);
	std::vector<int> failures;
	constexpr const int daymin=24*60;
	for(int i=0;i<nr;i++) {
		int alarmin= alarms[i].alarm-2;
		if(alarmin<0)
			alarmin+=daymin;
		int alarmend= alarms[i].end;
		const int type= alarms[i].type;
		LOGGER("%d %s %02d:%02d %02d:%02d ",type,data()->vars[type].name,alarmin/60,alarmin%60,alarmend/60,alarmend%60);
		int startmin=alarms[i].start;
		time_t starttime;
		if(nu>=alarmin&&(nu<alarmend||alarmend<alarmin)) {
			time_t alarmtime= (alarmin*60+startday);
			if(alarmtime <prevtime) {
				LOGGER("already alarmed %lu<%u\n",alarmtime,prevtime);
				continue;
				}
			starttime= ((startmin<=alarmin?0:-daymin)+startmin)*60+startday;
			}
		else if(alarmend<alarmin&&nu<alarmend) {
			auto alarmtime=startday+(alarmin-daymin)*60;
			if(alarmtime<prevtime) {
				LOGGER("already alarmed %lu<%u\n",alarmtime,prevtime);
				continue;
				}
			starttime= (startmin-daymin)*60+startday;

			}
		else  {
			LOGSTRING("not in interval\n");
			continue;
			}
extern bool happened(uint32_t stime,int type,float value)  ;
		//char buf[26];
#ifdef NDEBUG
#define EXTRABUF 10
#else
#define EXTRABUF 0
#endif
		char buf[26+EXTRABUF];

		ctime_r(&starttime,buf);
		if(!happened(starttime,type,alarms[i].value)) {
			LOGGER("didn't happen since %s",buf);
			failures.push_back(type);
			}
		else
			LOGGER("happened since %s",buf);
		
		}
	prevtime=nutimet;
	return failures;
	}
void Settings::setnumalarm(uint16_t type,float value,uint16_t start,uint16_t alarm) {
	const int nr=data()->alarmnr;
	if(nr>=maxnumalarms)
		return;
	amountalarm *alarms=data()->numalarm;
	int i=0;
	for(;i<nr;i++) {
		if(alarms[i].alarm>alarm) {
			memmove( alarms + i+1,alarms+i,sizeof(alarms[0])*(nr-i));
			break;
			}
		}
	constexpr uint16_t meldinterval=2*60;
	constexpr uint16_t day=24*60;
	 uint16_t endal=alarm+meldinterval;
	 if(endal>day)
	 	endal-=day;
	alarms[i]={value,(uint16_t)start,alarm,endal,type};
	data()->alarmnr=nr+1;
	}
void Settings::delnumalarm(int pos) {
	if(pos>=data()->alarmnr)
		return;

	if(--data()->alarmnr==pos)  {
		return;
		}
	amountalarm *alarms=data()->numalarm;
	memmove(alarms+pos,alarms+pos+1,sizeof(alarms[0])*(data()->alarmnr-pos));
	}

extern int dontdebug;
int dontdebug =0;
extern int dodebug() {
	if(dontdebug)
		return dontdebug;
	if(!settings)
		return 1;
	if(settings->getnodebug())
		return -1;
	return 0;
	}

void setupnetwork() {
        int hostnr=backup->gethostnr();
        if(hostnr==0) {
                std::string_view libreport="7117";
                if(!strcmp(backup->getupdatedata()->port,"8795")) {
                        memcpy(backup->getupdatedata()->port,libreport.data(),libreport.size());
                        }
//                backup->changehost(hostnr,nullptr,nullptr,0, true,destport,false,true,true,false,false,false,"",0,false,"libre3juggluco",true,true);
                }

        backup->getupdatedata()->wakesender();
        backup->wakebackup(Backup::wakeall);
//        backup->wakebackup();

       }

void startjuggluco(std::string_view dirfiles,const char *country) {
 	mkdir(dirfiles.data(),0700);
        setfilesdir(dirfiles,country);
        extern int startmeals() ;
        startmeals();
        extern void startsensors() ;
        startsensors( );
        extern void startthreads() ;
        startthreads();
	settings->data()->initVersion=16;
        }

static void initinjuggluco(std::string_view dirfiles,const char *country) {
        extern bool networkpresent;
        networkpresent=true;
        mkdir(dirfiles.data(),0700);
	pathconcat &jugdir=*(new pathconcat(dirfiles,"juggluco"));
	LOGGER("initjuggluco(%s %s)\n",jugdir.data(),__DATE__);
        mkdir(jugdir.data(),0700);
        extern int setfilesdir(const string_view filesdir,const char *country) ;
        setfilesdir(jugdir,country);
        extern int startmeals() ;
        startmeals();
        extern void startsensors() ;
        startsensors( );
        extern void startthreads() ;
        startthreads();
	settings->data()->initVersion=16;
        }

void initjuggluco(std::string_view dirfiles) {
 	initinjuggluco(dirfiles,nullptr);
        extern void setupnetwork();
        setupnetwork();
	}

