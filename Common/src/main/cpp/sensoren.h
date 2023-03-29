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
/*      Fri Jan 27 12:35:09 CET 2023                                                 */


#ifndef SENSOREN_H
#define SENSOREN_H
#include <iostream>
#include <string.h>
#include <algorithm>
#include <vector>
#include <time.h>
#include <limits.h>
#include "inout.h"
#include "SensorGlucoseData.h"
#include "settings/settings.h"
using namespace std;
//char sensorid[]="E007-0M0063KNUJ0";
//#define sensorid "E007-0M0063KNUJ0"xxxxxx
inline constexpr const int sensornamelen=16;
#include "gltype.h"
//typedef array<char,11>  sensorname_t;
struct sensor {
	uint32_t starttime;
	uint32_t endtime;
//	int8_t name[17];
	char name[sensornamelen+1];
	uint8_t present;
	uint8_t finished:7;
	bool initialized:1;
	uint8_t reserved[5];
const sensorname_t *shortsensorname() const { 
	return reinterpret_cast<const sensorname_t *>( name+5);
	}
const char *showsensorname() const {
	const char *name=shortsensorname()->data();
	if(!memcmp(name,"XX",2)) return name+2;
	return name;
	}
	} __attribute__ ((packed)) __attribute__ ((aligned (4))) ; /*always 32 bytes */
class Sensoren {
	string inbasedir;
	pathconcat mapfile;
	typedef Mmap<struct sensor> MapType;
	MapType map;
	struct infoblock {
		int32_t last, current, markold, start,version;
	};
//infoblock *infoblockptr();
//sensor *sensorlist();
	int maxhist;
	SensorGlucoseData **hist;
	int error = 0;
public:
	const string &getbasedir() const { return inbasedir; };

	int geterror() const {
		return error;
	}

	int32_t capacity() const {
		return map.count() - 1;
//	return infoblockptr()->capacity;
	}

	sensor *sensorlist() {
		return map.data() + 1;
	}

	const sensor *sensorlist() const {
		return map.data() + 1;
	}

	int32_t last() const {
		auto l= reinterpret_cast<const infoblock *>(map.data())->last;
		LOGGER("last()=%d\n",l);
		return l;
	}

	infoblock *infoblockptr() {
		return reinterpret_cast<infoblock *>(map.data());
	}

	Sensoren(string_view basedirin) : inbasedir(basedirin), mapfile{inbasedir, "sensors.dat"},
									  map(mapfile), maxhist(map.data()?(last() + 2):100),
									  hist(new SensorGlucoseData *[maxhist]()) {
	}
//static inline constexpr const int librekeepsecs=89*24*60*60;
	void setlibre3nums() {
		const SensorGlucoseData *sens=gethist();
	extern	void setlibrenum3(bool);
		setlibrenum3(sens!=nullptr&&sens->isLibre3());
		}
	void setversions()  {
		bool libre3;
		bool libre2;
		if(settings->data()->sendnumbers) {
			setlibre3nums();
			libre3=settings->data()->libre3nums;
			 libre2=!libre3;
			}
		else {
			 libre2=libre3=false;
			}
		uint32_t sendfrom=time(nullptr)-librekeepsecs;
#ifndef NOLOG
{
		time_t showtime=sendfrom;
		LOGGER("settings->data()->libre3nums=%d %u %s",libre3,sendfrom,ctime(&showtime));
		}
#endif
		int lastsens=last();
		for(int i= settings->data()->startlibreview;i<=lastsens; i++) {
			const SensorGlucoseData *sens=gethist(i);
			if(sens&&!sens->isLibre3()) {
				settings->data()->startlibreview=i;
				if(!sens->getinfo()->libreviewsendall&&sens->getstarttime()>sendfrom) {
					libre2=true;
					break;
					}
				}
			}
		for(int i= settings->data()->startlibre3view;i<=lastsens; i++) {
			const SensorGlucoseData *sens=gethist(i);
			if(sens&&sens->isLibre3()) {
				settings->data()->startlibre3view=i;
				if(!sens->getinfo()->libreviewsendall&&sens->getstarttime()>sendfrom) {
					libre3=true;
					break;
					}
				}
			}
		settings->data()->haslibre2=libre2;
		settings->data()->haslibre3=libre3;
		LOGGER("haslibre2=%d, haslibre3=%d\n",libre2,libre3);
		}
	static bool create(string_view inbasedir) {
		pathconcat file{inbasedir, "sensors.dat"};
		const int pagesize = getpagesize();
		{
			struct stat st;
			if (!stat(file, &st) && ((st.st_mode & S_IFMT) == S_IFREG) && st.st_size >= pagesize)
				return true;
		}
		//LOGGER("create %s pagesize=%d filesize=%d\n",file.data(),getpagesize(),st.st_size);

		mkdir(inbasedir.data(), 0700);
//	constexpr int cap=2;

		const int cap = pagesize / sizeof(struct sensor);
		LOGGER("map(%s,%d) getpagesize=%d\n", file.data(), cap, pagesize);
		MapType map(file, cap);
		auto dat = reinterpret_cast<infoblock *>(map.data());
		if (!dat)
			return false;
		constexpr int starthist = 5;
		if (dat->markold < starthist)
			dat[0] = {-1, -1, starthist, 0};
		LOGGER("Sensoren::create success\n");
		return true;
	}

	uint32_t timefirstdata() {
		static uint32_t first = UINT32_MAX;
		int end = std::min(last(), 3);
		for (int i = 0; i <= end; i++) {
			if (sensorlist()[i].present) {
				if (const SensorGlucoseData *hist = gethist(i)) {
					auto tim = hist->getfirsttime();
					if (tim < first) {
						first = tim;
					}
				}


//			if(sensorlist()[i].starttime<first) first=sensorlist()[i].starttime;
			}
		}
		if (first == UINT32_MAX)
			return time(nullptr) - 24 * 60 * 60;
		return first;
	}

	uint32_t timelastdata() {
		auto nr = last();
		if (nr >= 0) {
			if (const SensorGlucoseData *his = gethist(nr))
				return his->lastused();
//		return his->getlastscantime();
		}
		return time(NULL);
	}

	void extend(int newcap) {
		map.extend(mapfile, 1 + newcap);
//	infoblockptr()=reinterpret_cast<infoblock*>(map.data());
//	sensorlist()=map.data()+1;
	}

	void setmaxhistory(int max) {
		SensorGlucoseData **tmphist = new SensorGlucoseData *[max]();
		memcpy(tmphist, hist, sizeof(SensorGlucoseData *) * maxhist);
		SensorGlucoseData **oldhist = hist;
		hist = tmphist;
		delete[] oldhist;
		maxhist = max;
	}

	int getmaxhistory() const {
//	return infoblockptr()->maxhist;
		return maxhist;
	}

	int addsensor(string_view name) {
		LOGGER("addsensor(%.16s)\n", name.data());
		infoblockptr()->last++;
		if (last() >= capacity())
			extend(capacity() * 2);
		if (last() >= getmaxhistory()) {
			setmaxhistory(last() * 2);
		}
		SensorGlucoseData *histel = new SensorGlucoseData(pathconcat(inbasedir, name));
		hist[infoblockptr()->last] = histel;
		sensorlist()[infoblockptr()->last].starttime = histel->getstarttime();
		memcpy(sensorlist()[infoblockptr()->last].name, name.data(), name.length());
		sensorlist()[infoblockptr()->last].name[sensornamelen] = '\0';
		sensorlist()[infoblockptr()->last].present = 1;
		sensorlist()[infoblockptr()->last].endtime = 0;
		sensorlist()[infoblockptr()->last].finished = 0;
		infoblockptr()->current = infoblockptr()->last;
		LOGGER("add sensor %.16s\n", name.data());
		return infoblockptr()->last;
	}

	const sensor *getsensor(const int ind) const {
		return sensorlist() + ind;
	}

	sensor *getsensor(const int ind) {
		return sensorlist() + ind;
	}

static SensorGlucoseData::longsensorname_t  namelibre3(const std::string_view sensorid) {
	SensorGlucoseData::longsensorname_t  sens;
	constexpr const char start[]="E07A-XXXX";
        const int len=sensorid.length();
	const int startlen=sens.size()-len;
	memcpy(&sens[0],start,startlen);
        memcpy(&sens[0]+startlen,sensorid.data(),len);
	return sens;
        }
SensorGlucoseData *makelibre3sensor(std::string_view shortname,uint32_t starttime,uint32_t now) {
	return makelibre3sensor(shortname, starttime,0,nullptr,now);
	}

int makelibre3sensorindex(std::string_view shortname,uint32_t starttime,const uint32_t pin,const char *deviceaddress,uint32_t now) {
 	const auto  name=namelibre3(shortname);

#ifndef NOLOG
	time_t tim=starttime;
	LOGGER("makelibre3sensor(%s,%u) %s",shortname.data(),starttime,ctime(&tim));
#endif

	if(sensor *sensgegs = findsensorm(name.data()) ) {
		LOGGER("known sensor %s\n",sensgegs->showsensorname());
		const int	sensindex= sensgegs - sensorlist();
		SensorGlucoseData *sens=gethist(sensindex) ;
		if(pin) {
			sens->getinfo()->pin=pin;
			}
		if(deviceaddress) {
			char *address=sens->deviceaddress();
			if(!*address)
				strcpy(address,deviceaddress);
			}
		sens->getinfo()->haskAuth=false;
extern void	sendKAuth(SensorGlucoseData *hist);
		sendKAuth(sens);
		sensgegs->finished=0;
		sens->getinfo()->lastscantime=now;
		return sensindex;
		}
	const pathconcat sensordir(inbasedir,name);
	if(SensorGlucoseData::mkdatabase3(sensordir, starttime,pin,deviceaddress) ) {
		const int ind=addsensor(std::string_view(name.data(),name.size()));
		sensor *sen=getsensor(ind);
		sen->initialized=true;
		return ind ;
		}
	return -1;
	}
SensorGlucoseData *makelibre3sensor(std::string_view shortname,uint32_t starttime,const uint32_t pin,const char *deviceaddress,const uint32_t now) {
	int sensindex=makelibre3sensorindex(shortname,starttime,pin,deviceaddress,now);
	if(sensindex<0)
		return nullptr;
	return gethist(sensindex);
	}
	const sensor *findsensor(const char *name) const {
		const sensor *end = sensorlist() + last() + 1;
		const sensor *sens = find_if(sensorlist(), end, [name](const sensor &sens) -> bool {
			return !memcmp(name, sens.name, sensornamelen);
		});
		if (end == sens)
			return nullptr;
		return sens;
	}

	const sensor *findsensorshort(const char *name) const {
		const sensor *end = sensorlist() + last() + 1;
		const sensor *sens = find_if(sensorlist(), end, [name](const sensor &sens) -> bool {
			return !memcmp(name, sens.name + 5, 11);
		});
		if (end == sens)
			return nullptr;
		return sens;
	}

	sensor *findsensorm(const char *name) {
		const sensor *sens = findsensor(name);
		return const_cast<sensor *>(sens);
	}

	int sensorindex(const char *name) const {
		const sensor *sens = findsensor(name);
		if (!sens)
			return -1;
		return sens - sensorlist();
	}

	int sensorindexshort(const char *name) const {
		const sensor *sens = findsensorshort(name);
		if (!sens)
			return -1;
		return sens - sensorlist();
	}

	SensorGlucoseData *gethist(const char *name) {
		if (int ind = sensorindex(name);ind >= 0)
			return gethist(ind);
		return nullptr;
	}

	SensorGlucoseData *gethistshort(const char *name) {
		if (int ind = sensorindexshort(name);ind >= 0)
			return gethist(ind);
		return nullptr;
	}


//static constexpr const uint32_t twoweeks= 14.5*24*60*60;
	static constexpr const uint32_t sensorageseconds = 15 * 24 * 60 * 60u;

	vector<int> inperiod(uint32_t starttime, uint32_t endtime) {
		vector<int> out;
		const uint32_t startend = starttime - sensorageseconds;
		const uint32_t nothingbefore = startend - sensorageseconds;
		const uint32_t nu = time(nullptr);
		for(int i = last(); i >= 0; i--) {
			const uint32_t startsensor = sensorlist()[i].starttime;
			if(startsensor >= endtime)
				continue;
			if(startsensor < startend)  {
				if(startsensor < nothingbefore) {
					break;
					}
				continue;
				}

			auto oneend = sensorlist()[i].endtime;
			if(sensorlist()[i].finished && oneend && oneend <= starttime) {
				continue;
			}
			checkinfo(i, nu);
			oneend = sensorlist()[i].endtime;
			if(oneend && oneend <= starttime) {
				continue;
			}
			out.push_back(i);
		}
		return out;
	}
	bool inperiod(int i, uint32_t starttime, uint32_t endtime) {
		const uint32_t startsensor = sensorlist()[i].starttime;
		if (startsensor >= endtime)
			return false;
		if (startsensor < (starttime - sensorageseconds))
			return false;

		const auto oneend = sensorlist()[i].endtime;
		if (sensorlist()[i].finished && oneend && oneend <= starttime) {
			return false;;
		}
		const uint32_t nu = time(nullptr);
		checkinfo(i, nu);
		if (sensorlist()[i].endtime && sensorlist()[i].endtime <= starttime)
			return false;

		return true;
	}

int firstafter(uint32_t starttime)  {
	int started=starttime-60*60;
	for(int i=last();i>=0;i--) {
		if(sensorlist()[i].starttime>started) 
			continue;
		SensorGlucoseData *sens=gethist(i);
		auto firsttime=sens->getfirsttime() ;
		if(firsttime==UINT32_MAX)
			continue;
		if(firsttime<=starttime)
			return i;
		}
	return 0;
	}
/*Finds the sensor with a starttime around tim */
	sensor *findwithstarttime(uint32_t tim, int distance = 60 * 60 * 4) {
		sensor *starts = sensorlist();
		sensor *ends = sensorlist() + last() + 1;
		if (sensor *hit = std::find_if(starts, ends, [tim, distance](sensor &sen) {
				return labs((long) sen.starttime - (long) tim) < distance;
			});hit != ends)
			return hit;
		return nullptr;
	}

/*
vector<SensorGlucoseData *> inperiod(uint32_t starttime,uint32_t endtime) {
	vector<SensorGlucoseData *> out;
	for(int i=last();i>=0;i--) {
		if(sensorlist()[i].starttime>=endtime)
			continue;
		if(sensorlist()[i].finished&&sensorlist()[i].endtime<=starttime) {
			continue;
			}
		checkinfo(i);
		if(sensorlist()[i].endtime&&sensorlist()[i].endtime<=starttime) {
			if(sensorlist()[i].finished) 
				break;
			else
				continue;
			}
		out.push_back(hist[i]);
		}
	return out;
	}
	*/
	SensorGlucoseData *gethist(int ind = -1) {
		LOGGER("gethist(%d)\n",ind);
		if (ind < 0) {
			ind = last();
			if (ind < 0) {
				LOGGER("gethist last()<0\n");
				return nullptr;
			}
		}
		if (ind >= getmaxhistory())
			setmaxhistory(ind * 2);
		if (!hist[ind]) {
			if (!sensorlist()[ind].name[0]) {
				LOGGER("gethist sensorlist()[%d].name[0]==null\n", ind);
				return nullptr;
			}
			hist[ind] = new SensorGlucoseData( pathconcat(inbasedir, string_view(sensorlist()[ind].name, sensornamelen)));
			}
		if (hist[ind]) {
			bool error = hist[ind]->error();
			if(!error) {
				if(hist[ind]->infowrong()) {
					infoblockptr()->last = ind - 1;
					LOGGER("infoblock wrong\n");
					goto INFOWRONGERROR;
				}
				}
			else {
				INFOWRONGERROR:
				LOGGER("hist[ind]->error()\n");
				SensorGlucoseData *tmp = hist[ind];
				hist[ind] = nullptr;
				delete tmp;
				return nullptr;
			}
			sensorlist()[ind].present = 1;
			sensorlist()[ind].starttime = hist[ind]->getstarttime();
			return hist[ind];
		}
		LOGGER("gethist new SensorGlucoseData(...)=NULL\n");
		return nullptr;
	}

	bool hasstream() {
		auto sens = gethist();
		if (!sens)
			return false;
		return sens->pollcount() > 0;
	}
void finishsensor(int ind) {
	if(ind>=0&&ind<=last())
		sensorlist()[ind].finished=1;
	}
	void checkinfo(const int ind, uint32_t nu) {
		const SensorGlucoseData *thishist = gethist(ind);
//	thishist=hist[ind];
		if (!thishist)
			sensorlist()[ind].present = 0;
		else {
			sensorlist()[ind].endtime = thishist->lastused();
			uint32_t maxtime = thishist->getmaxtime();
			if (maxtime < nu) {
				LOGGER("%s finished was %d set to 1\n", sensorlist()[ind].name, sensorlist()[ind].finished);
				sensorlist()[ind].finished=1;
			     }
			// "TODO test on presence"
			sensorlist()[ind].present = 1;
		}
	}

	void checkall() {
		const uint32_t nu = time(nullptr);
		for (int i = 0; i <= last(); i++)
			checkinfo(i, nu);
	}


	void deletehist() {
		if (hist) {
			for (int i = 0; i <= last(); i++) {
				delete hist[i];
			}
			delete[] hist;
		}
	}

	~Sensoren() {
		deletehist();
	}

	typedef array<char, 11> sensorname_t;

	const sensorname_t *shortsensorname(int index) const {
		const sensor *sens = getsensor(index);
		return sens->shortsensorname();
//	return reinterpret_cast<const sensorname_t *>( sens->name+5);
	}
	const char *showsensorname(int index) const {
		const sensor *sens = getsensor(index);
		return sens->showsensorname();
	}

	const sensorname_t *shortsensorname() const {
		if (int l = last();l >= 0)
			return shortsensorname(l);
		return nullptr;
	}

	const uint32_t laststarttime() const {
		if (int l = last();l >= 0) {
			return sensorlist()[l].starttime;
		}
		return 0;
	}

	const uint32_t  sensorStarttimei(const int ind) const {
		return sensorlist()[ind].starttime;
		}
	const uint32_t sensorStarttime(const char *name) const {
		if(int ind = sensorindex(name);ind >= 0)
			return sensorlist()[ind].starttime;
		return 0;
		}


	/*
vector<int> usedsince(uint32_t tim,uint32_t nu) {
	vector<int> out;
	uint32_t old=nu-14.6*24*60*60;
	for(int i=last();i>=0;i--) {
		if(sensorlist()[i].starttime<old)
			break;
		const SensorGlucoseData *hist=gethist(i);
		if(hist->lastused()<tim)
			continue;
		out.push_back(i);
		}
	return out;
	} */
	vector<int> bluetoothactive(uint32_t tim, uint32_t nu) {
		LOGGER("bluetoothactive\n");
		vector<int> out;
		uint32_t old = nu - sensorageseconds;
//		uint32_t established = nu - 2*60*60;

		for (int i = last(); i >= 0; i--) {
			if (sensorlist()[i].finished) {
				LOGGER("%s finished\n", showsensorname(i));
				continue;
				}
			if (sensorlist()[i].starttime < old) {
				LOGGER("%s old\n", showsensorname(i));
				break;
				}
			const SensorGlucoseData *hist = gethist(i);
			if (!hist) {
				LOGGER("hist==null\n");
				continue;
				}
			const auto lasttime=hist->lastused() ;
			bool canuse=hist-> canusestreaming() ;
			LOGGER("%s: can %suse streaming, lasttime=%d\n",showsensorname(i),canuse?"":"not ",lasttime);
			if(!canuse|| (lasttime &&  lasttime < tim)) {
				continue;
				}
//		sensorlist()[i].finished=0;
			out.push_back(i);
		}
		LOGGER("end bluetoothactive %zu\n",out.size());
		setlibre3nums();
		return out;
	}
	/*
int lastscanned() {
	int newst=last();
	if(newst<0)
		return -1;
	const SensorGlucoseData *hist=gethist(newst);
	uint32_t lasttime=hist->getlastscantime();
	uint32_t old=lasttime-14.5*24*60*60;
	for(int i=last()-1;i>=0;i--) {
		if(sensorlist()[i].starttime<old)
			break;
		const SensorGlucoseData *hist=gethist(i);
		if(const uint32_t tim=hist->getlastscantime();tim>lasttime) {
			lasttime= tim;
			newst=i;
			}
		}
	return newst;
	}
	*/
//uint32_t getlastpolltime() const 
std::pair<int, uint32_t> lastused(uint32_t (SensorGlucoseData::*proc)(void) const) {
	uint32_t lasttime;
	uint32_t old;
	int newst = last();
	for(;;--newst) {
		if(newst < 0)
			return {-1, 0};
		const SensorGlucoseData *hist = gethist(newst);
		if(!hist) {
			return {-1, 0};
			}
		if((lasttime = (hist->*proc)())>sensorageseconds) {
			old = lasttime - sensorageseconds;
			break;
			}
		}
	for(int i = newst - 1; i >= 0; i--) {
		if(sensorlist()[i].starttime < old)
			break;
		const SensorGlucoseData *hist = gethist(i);
		if(const uint32_t tim = (hist->*proc)();tim > lasttime) {
			lasttime = tim;
			newst = i;
		}
	}
	return {newst, lasttime};
	}
/*
	std::pair<int, uint32_t> lastused(uint32_t (SensorGlucoseData::*proc)(void) const) {
		int newst = last();
		if(newst < 0)
			return {-1, 0};
		const SensorGlucoseData *hist = gethist(newst);
		if(!hist) {
			return {-1, 0};
			}
		uint32_t lasttime = (hist->*proc)();
		uint32_t old = lasttime - sensorageseconds;
		for(int i = last() - 1; i >= 0; i--) {
			if(sensorlist()[i].starttime < old)
				break;
			const SensorGlucoseData *hist = gethist(i);
			if(const uint32_t tim = (hist->*proc)();tim > lasttime) {
				lasttime = tim;
				newst = i;
			}
		}
		return {newst, lasttime};
	}
	*/

	int lastscanned() {
		auto[id, _]= lastused(&SensorGlucoseData::getlastscantime);
		return id;
	}

	auto lastpolltime() {
		return lastused(&SensorGlucoseData::getlastpolltime);
	}

	auto firstpolltime() {
		for (int i = 0; i <= last(); i++) { //MODIFIED!!
			if (sensorlist()[i].present) {
				if(const SensorGlucoseData *hist = gethist(i)) {
					auto tim = hist->firstpolltime();
					if (tim < UINT32_MAX)
						return tim;
					}
			}
		}
		return (decltype(std::declval<SensorGlucoseData>().firstpolltime())) 0;
	}

	const SensorGlucoseData *laststreamsensor() {
		for (int i = last(); i >= 0; i++) {
			if (const SensorGlucoseData *hist = gethist(i)) {
				if (hist->pollcount() > 0)
					return hist;
			}
		}
		return nullptr;
	}

	/*
void converttrends() {
	for(int i=0;i<=last();i++) {
		SensorGlucoseData *hist=gethist(i);
		hist->converttrends();
		}
	}
void convertlast() {
	SensorGlucoseData *hist=gethist(last());
	hist->converttrends();
	}
	*/

	void removeoldstates();

	void updateinit(const int ind) {
		for (int i = last(); i >= 0; i--) {
			if (SensorGlucoseData *hist = gethist(i))
				hist->updateinit(ind);
		}
	}

	bool setbackuptime(crypt_t *pass, const int sock, const int ind, const uint32_t starttime,uint16_t &starttimeindex) {
		int nr=last()+1;
		if(starttimeindex>nr)
			starttimeindex=nr;
		for (;starttimeindex>0;) {
			auto newindex=starttimeindex-1;
			if(SensorGlucoseData *hist = gethist(newindex)) {
				if(!hist->setbackuptime(pass, sock, ind, starttime))
					return false;
				}
			else   {
				LOGGER("gethist(%d) failed\n",newindex);
				return false;
				}
			starttimeindex=newindex;
			}
		return true;
	}

	int update(crypt_t *pass, const int sock, const int ind, int &startupdate, int &firstsensor,
			   const bool upstream, const bool upscan, const bool restoreinfo) {
		LOGGER("Sensoren::update firstsensor=%d sock=%d ind=%d\n", firstsensor, sock, ind);
		int changed = INT_MAX;
		int did = 2;
		int lastlast = -1;
		for(int i = firstsensor; i <= last(); i++) {
			LOGGER("sensor %d\n", i);
			if(SensorGlucoseData *hist = gethist(i)) {
				if(upstream) {
					const int resstream = hist->updatestream(pass, sock, ind, i);
					switch (resstream) {
						case 0: return did&0x4;
							//		case 1: changed=i;
						};
					if(resstream == 1 && i == last())
						lastlast = i;
					did |= resstream;
					}
				if(upscan) {
					const int resscan = hist->updatescan(pass, sock, ind, i,i>=startupdate);
					switch(resscan) {
						case 0: return did&0x4;
						case 5: 
						case 1: {
							if(changed>i)
								changed = i + 1; //MODIFIED
							}
					};
					did |= resscan;
				}
				if(sensorlist()[i].finished) {
					if ((firstsensor + 1) == i)
						firstsensor = i;
				}
			} else
				return did&0x4;

		}
		if((last() >= startupdate && (changed = 0, true)) || changed < INT_MAX) {

			int endsens = last() + 1;
			string_view sensorfile("sensors/sensors.dat");
			const auto *begin = map.data(); //Start with info block, sensor at position 1
			LOGGER("senddata(%d,%p,%d,%s)\n", changed, begin + changed, endsens + 1 - changed,
				   sensorfile.data());
			if (!senddata(pass, sock, changed, begin + changed, begin + endsens + 1, sensorfile))
				return did&0x4;
			startupdate = endsens;

			did = 1;
		}
		if(lastlast >= 0) {
			bool sendshowglucose(crypt_t *pass, const int sock, const uint16_t sensorindex);
			if (!sendshowglucose(pass, sock, lastlast))
				return did&0x4;
		}

		return did;
	}

	int update(crypt_t *pass, const int sock, const int ind, int &firstsensor,
			   int (SensorGlucoseData::*proc)(crypt_t *, int, int, int)) {
		uint32_t start = time(NULL) - sensorageseconds;
		int did = 2;
		for (int i = last(); i >= firstsensor; i--) {
			if (sensorlist()[i].starttime < start)
				break;
			if (!sensorlist()[i].finished) {
				SensorGlucoseData *hist = gethist(i);
				int subdid = (hist->*proc)(pass, sock, ind, i);
				if (!subdid) return 0;
				did |= subdid;
			}
		}
		return did;
	}


	int updatescanss(crypt_t *pass, const int sock, const int ind, int &firstsensor) {
		return update(pass, sock, ind, firstsensor, &SensorGlucoseData::updatescanalg);
	}

	int updatestreams(crypt_t *pass, const int sock, const int ind, int &firstsensor) {
		int res = update(pass, sock, ind, firstsensor, &SensorGlucoseData::updatestream);

		return res;
	}
	template<class FUN> void onallsensors(const FUN &fun)  {
		for (int i = last(); i >= 0; i--) {
			SensorGlucoseData *hist = gethist(i);
			fun(hist);
		}

	};
};
inline std::ostream& operator<<(std::ostream& os,const sensor &sens) {
	os<<sens.name<<endl;
	time_t tim=sens.starttime;
	os<<"starttime:\t"<<ctime(&tim);
	tim=sens.endtime;
	os<<(!sens.finished?"Not ":"")<<"Finished"<<endl;
	if(tim)
		os<<"endtime:\t"<<ctime(&tim);
	os<<(!sens.present?"Not ":"")<<"Present"<<endl;
        return os;
        }

#endif
