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
/*      Fri Jan 27 12:39:13 CET 2023                                                 */


#ifndef WEAROS
#include <memory>
#include <string_view>
#include <array>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include "sensoren.h"
#include "destruct.h"
#include "SensorGlucoseData.h"
#include "settings/settings.h"
#include "libreview.h"
#ifdef LIBRENUMBERS
#include "librenumbers.h"
#endif
constexpr const int DEBUGLEN=0;
constexpr const int EXTRALEN=

#ifdef NDEBUG
30
#else
0
#endif
;
constexpr const int histelUitlen=sizeof(R"({"valueInMgPerDl":159.0,"extendedProperties":{"factoryTimestamp":"2022-06-20T20:24:33.000Z","isFirstAfterTimeChange":false},"recordNumber":-6629292502336270757,"timestamp":"2022-06-20T22:24:33.000+02:00"},)")+EXTRALEN;

constexpr const int startsensorlen=sizeof(R"({"type":"com.abbottdiabetescare.informatics.sensorstart","extendedProperties":{"factoryTimestamp":"2022-06-18T05:57:19.000Z"},"recordNumber":-6629292502336274432,"timestamp":"2022-06-18T07:57:19.000+02:00"})")+EXTRALEN;

//int64_t recordnumbermask=-6629292502336274432L;
constexpr const  std::string_view datastart=R"({"DeviceData":{"deviceSettings":{"factoryConfig":{"UOM":")"; 
//s/char\(.*\)\[\]=R\(.*\)$/constexpr  std::string_view \1=R\2/g
constexpr const  std::string_view afterunit= R"("},"firmwareVersion":"2.3.0","miscellaneous":{"selectedLanguage":")";
constexpr const  std::string_view afterlocale=R"(","valueGlucoseTargetRangeLowInMgPerDl":)";
constexpr const  std::string_view afterlow=R"(,"valueGlucoseTargetRangeHighInMgPerDl":)";
constexpr const  std::string_view afterhigh=R"(,"selectedTimeFormat":")";
constexpr const  std::string_view aftertimeformat=R"(hr","selectedCarbType":"grams of carbs"},"timestamp":")";
const constexpr  std::string_view  aftercurrenttime=R"("},"header":{"device":{"modelName":"com.freestylelibre.app.de","uniqueIdentifier":")";
constexpr const  std::string_view  afterident= R"("}},"measurementLog":{"bloodGlucoseEntries":[],"capabilities":["scheduledContinuousGlucose","unscheduledContinuousGlucose","bloodGlucose","ketone","insulin","food","generic-com.abbottdiabetescare.informatics.exercise","generic-com.abbottdiabetescare.informatics.medication","generic-com.abbottdiabetescare.informatics.customnote","generic-com.abbottdiabetescare.informatics.ondemandalarm.low","generic-com.abbottdiabetescare.informatics.ondemandalarm.high","generic-com.abbottdiabetescare.informatics.ondemandalarm.projectedlow","generic-com.abbottdiabetescare.informatics.ondemandalarm.projectedhigh","generic-com.abbottdiabetescare.informatics.sensorstart","generic-com.abbottdiabetescare.informatics.error"],"device":{"modelName":"com.freestylelibre.app.de","uniqueIdentifier":")";
const constexpr  std::string_view  afterident2=R"("},"foodEntries":[)";
const constexpr  std::string_view  afterfood=R"(],"genericEntries":[)";
const constexpr  std::string_view  afterlocalstartime=R"(],"insulinEntries":[)";
const constexpr  std::string_view  afterinsulin=R"(],"ketoneEntries":[],"scheduledContinuousGlucoseEntries":[)";
constexpr const std::string_view afterhists=R"(],"unscheduledContinuousGlucoseEntries":[)";
//{"valueInMgPerDl":163.0,"extendedProperties":{"factoryTimestamp":"2022-06-20T20:49:33.000Z","trendArrow":"Stable","isActionable":true,"isFirstAfterTimeChange":false},"recordNumber":2304,"timestamp":"2022-06-20T22:49:33.000+02:00"},{"valueInMgPerDl":133.0,"extendedProperties":{"factoryTimestamp":"2022-06-20T21:15:01.000Z","trendArrow":"Stable","isActionable":true,"isFirstAfterTimeChange":false},"recordNumber":2560,"timestamp":"2022-06-20T23:15:01.000+02:00"}
const constexpr  std::string_view  afterscans=R"(]}},"UserToken":")";
const constexpr  std::string_view  aftertoken=R"(","Domain":"Libreview","GatewayType":"FSLibreLink.Android"})";

struct LibreHistEl {
	uint32_t ti;
	uint16_t mgdL;
	uint16_t id;
	};
	

struct LibreHist {
	int64_t histor;
	LibreHistEl *list;
	int32_t size;
	uint32_t starttime;
	int msec;
	uint32_t notsend;
	};


inline static void valuestart(char *&ptr,float glvalue) {
	ptr+=sprintf(ptr, R"({"valueInMgPerDl":%.1f,"extendedProperties":{"factoryTimestamp":")",glvalue);
	}

//se},"recordNumber":-6629292502336270757,"timestamp":"2022-06-20T	
/*
#if sizeof(int64_t)==sizeof(long)
#define int64printf "%ld"
#else
#define int64printf "%lld"
#endif
*/
constexpr const char *int64printf() {
	 if constexpr(sizeof(int64_t)==sizeof(long))
		return "%ld";
	else
		return "%lld";
	}
static char *librehistel(const LibreHistEl *el,const int64_t histor,char *buf) {
	char *ptr=buf;
	valuestart(ptr, (float)el->mgdL);
	int mil=0;
	ptr+=TdatestringGMT(el->ti,mil,ptr);
	static constexpr const std::string_view prerecord=R"(","isFirstAfterTimeChange":false},"recordNumber":)"; 
	addstrview(ptr,prerecord);
	ptr+=sprintf(ptr,int64printf(),mkhistrecord(histor,el->id));
	static constexpr const std::string_view timestamp=R"(,"timestamp":")";
	addstrview(ptr,timestamp);
	ptr+=Tdatestringlocal(el->ti,mil,ptr);
//	ptr+=sprintf(ptr,R"("})");
	addar(ptr,R"("})");
	return ptr;
	}




static char *libreScanel(const ScanData &scanel,const int16_t  nr,char *ptr) {
	valuestart(ptr, (float)scanel.getmgdL());
	int mil=0;
	ptr+=TdatestringGMT(scanel.gettime(),mil,ptr);
	int16_t id=nr>>8|(nr&0xFF)<<8;
	ptr+=sprintf(ptr,R"(","trendArrow":"%s","isActionable":true,"isFirstAfterTimeChange":false},"recordNumber":%hd,"timestamp":")",trendName[scanel.tr],id);
	ptr+=Tdatestringlocal(scanel.gettime(),mil,ptr);
//	ptr+=sprintf(ptr,R"("})");
	addar(ptr,R"("})");
	return ptr;
	}



static char *writelibrehists(LibreHist *hists,int size,char *buf) {
	char *ptr=buf;
	for(int i=size-1;i>=0;--i) {
		if(const int len=hists[i].size) {
			const LibreHistEl *list=hists[i].list;
			const int64_t histor=hists[i].histor;
			for(int j=0;j<len;j++) {
				ptr=librehistel(list+j,histor,ptr);
				*ptr++=',';
				}
			}
		}
	if(buf!=ptr)
	 	--ptr;
	return ptr;
	}

static LibreHist  librehistory(SensorGlucoseData *sensdata,uint32_t starttime,uint32_t nu) {
	const int notsend=sensdata->getinfo()->libreviewnotsend;

#ifndef NOLOG
	const time_t tim=starttime;
	LOGGER("%s librehistory notsend=%d from %s",sensdata->shortsensorname()->data(),notsend,ctime(&tim));
	#else
	#endif
//	int lasthistid=sensdata->getinfo()->libreviewid;
//	int notsend=0;
	const std::span<const ScanData> stream=sensdata->getPolldata() ;
	uint32_t streamlen= stream.size();
	if(streamlen<1||streamlen<=notsend) {
		LOGGER("no hist: streamlen=%d\n",streamlen);
		return {.notsend=streamlen};
		}
	const int lastpos= streamlen-1;
	const ScanData *laststream=&stream[lastpos];
	const ScanData *firststream=&stream[notsend];
	while(!laststream->valid(lastpos)) {
		if(--laststream<firststream) {
			LOGGER("no hist: no data\n");
			return {.notsend=streamlen};
			}
		}
		
	int laststreamid=laststream->id;

	constexpr int periodmin=15;
	constexpr int periodright=15/2;
	int left=laststreamid%periodmin;
	if(left!=periodright) {
		int toid=laststreamid-((left>periodright)?(left-periodright):(periodmin-(periodright-left)));
		while(true) {
			--laststream;
			if(laststream<firststream) {
				LOGGER("no librehist no whole period\n");
				return {.notsend=static_cast<uint32_t>(notsend)};
				}
			if(laststream->valid()) {
				if(laststream->id<=toid) {
					laststreamid=laststream->id;
					break;
					}

				}
			}
		}
	LOGGER("laststreadid=%d\n",laststreamid);
	streamlen=laststream-&stream[0]+1;
	size_t leftstream=streamlen-notsend;
	const ScanData *found=(firststream->t<starttime)?sensdata->firstnotless({firststream,leftstream},starttime):firststream;
	int startid=0;
	if(found>&stream[0])  {
		const ScanData &prev=found[-1];
		if(prev.valid()) {
			startid=prev.id+1;
			}
		}
	while(true) {
		if(found>laststream)
			return {.notsend=streamlen};
		if(found->valid()) 
			break;
		++found;
		}; 
	if(!startid)
		startid=found->id;
	int histnum=(startid + periodright-1)/periodmin;
//	int histnum=(found->id + periodright-1)/periodmin;
	int previd= histnum*periodmin;
	int id=previd+periodmin;
	int firstid=previd+periodright+1;
	LOGGER("id=%d\n",id);
	while(true) {
		if(found->id>=firstid)	
			break;
		while(true) {
			++found;
			if(found>laststream)  {
				LOGGER("found >laststream notsend=%d\n",streamlen);
				return {.notsend=streamlen};
				}
			if(found->valid()) 
				break;
			}
		}


	int datalen=1+(laststreamid-previd)/periodmin;
	if(datalen<=0) {
		LOGGER("datalen=%d\n",datalen);
		return {.notsend=streamlen};
		}
	LibreHistEl *list=new LibreHistEl[datalen];
	if(!list)
		return {.notsend=static_cast<uint32_t>(found-stream.data())};
	int datuit=0;
	const ScanData *useddata=found-1;
	for(const ScanData *iter=found;iter<=laststream;) {
		uint64_t timesum=0LL;
		double glusum=0.0;
		int num=0;
		const int nextid=id+periodright;
		if(nextid>laststreamid)
			break;
		for(;iter->id<=nextid;) {
			LOGGER("id %d\n",iter->id);
			timesum+=iter->t;
			glusum+=iter->g;
			useddata=iter;
			++num;
			do {
			   if(++iter>laststream)
				goto ENDINLOOP;
			    } while(!iter->valid());
			}
		ENDINLOOP:
		if(num) {
			list[datuit].ti=timesum/num;
			list[datuit].mgdL=(int16_t) round(glusum/num);
			list[datuit++].id=id;	
			LOGGER("in id=%d\n",id);
			}
		id+=periodmin;
		}
	extern int64_t libreviewHistor(const sensorname_t *sensorid) ;
	const int64_t histor=libreviewHistor(sensdata->shortsensorname());
	uint32_t newnotsend=useddata-stream.data()+1;
	LOGGER("send %d data notsend=%d\n",datuit,newnotsend);
//	return {.histor=histor,.list=list,.size=datuit,.starttime=sensdata->getstarttime(),.msec=sensdata->getinfo()->msec,.notsend=newnotsend};
	return {.histor=histor,.list=list,.size=datuit,.starttime=sensdata->getstarttime(),.msec=0,.notsend=newnotsend};
	}
/*
struct LibreHist {
	int64_t histor;
	LibreHistEl *list;
	int32_t size;
	uint32_t starttime;
	}; */


static int mksensorstart(int64_t recordnumbermask, time_t starttime,int msec,char *buf) {
//{"type":"com.abbottdiabetescare.informatics.sensorstart","extendedProperties":{"factoryTimestamp":"2022-06-18T05:57:19.000Z"},"recordNumber":-6629292502336274432,"timestamp":"2022-06-18T07:57:19.000+02:00"}
	static constexpr const char start[]=R"({"type":"com.abbottdiabetescare.informatics.sensorstart","extendedProperties":{"factoryTimestamp":")";
	static constexpr const int startlen=sizeof(start)-1;
	memcpy(buf,start,startlen);
	char *ptr=buf+startlen;
	ptr+=TdatestringGMT(starttime,msec,ptr);
       static const char afterstartimeGMT[]= R"("},"recordNumber":)";
       static constexpr const int aftergmtlen=sizeof(afterstartimeGMT)-1;
       memcpy(ptr,afterstartimeGMT, aftergmtlen);
       ptr+= aftergmtlen;
 
       ptr+=sprintf(ptr,int64printf(),recordnumbermask);
	const char afterrecordmask[]=R"(,"timestamp":")";
	static constexpr const int afterrecordmasklen=sizeof(afterrecordmask)-1;
	memcpy(ptr,afterrecordmask,afterrecordmasklen);
	ptr+=afterrecordmasklen;
	ptr+=Tdatestringlocal(starttime,msec,ptr);
	constexpr const char end[]=R"("})";
	constexpr const int endlen=sizeof(end)-1;
	memcpy(ptr,end,endlen);
	ptr+=endlen;
	return ptr-buf;
	}

int mksensorstart(const LibreHist *list,char *buf);
int mksensorstart(int64_t recordnumbermask, time_t starttime,char *buf);
static int mksensorstart(const LibreHist &el,char *buf) {
	return mksensorstart(el.histor,el.starttime,el.msec,buf);
	}
static int writelist(const LibreHist *list,const int len,char *buf,int (*func)(const LibreHist &el,char *buf)) {
	char *bufptr=buf;
	for(int i=len-1;i>=0;--i) {
		if(list[i].size) {
			bufptr+=func(list[i],bufptr);
			*bufptr++=',';
			}
		}
//	if(bufptr!=buf) --bufptr;
	return bufptr-buf;
	}
static int makesensorstarts(const LibreHist *list,const int len,char *buf) {
	return writelist(list,len,buf,mksensorstart);
	}
constexpr const int scanelsize=sizeof(R"({"valueInMgPerDl":500.0,"extendedProperties":{"factoryTimestamp":"2022-06-20T16:26:06.000Z","trendArrow":"FallingQuickly","isActionable":true,"isFirstAfterTimeChange":false},"recordNumber":10256,"timestamp":"2022-06-20T18:26:06.000+02:00"},)");
#include <jni.h>

extern JNIEnv *getenv();

jclass libreviewclass=nullptr;
extern char *localestr;
extern bool hour24clock;

 bool	libresendmeasurements(bool libre3,const char *measurements,const int len) {
	static jmethodID  postmeasurements=getenv()->GetStaticMethodID(libreviewclass,"postmeasurements","(Z[B)Z");
	auto env=getenv();
	jbyteArray uit=env->NewByteArray(len);
        env->SetByteArrayRegion(uit, 0, len,(const jbyte *)measurements);
	bool res=env->CallStaticBooleanMethod(libreviewclass,postmeasurements,libre3,uit);
	env->DeleteLocalRef(uit);
	return res;
	}


static bool putsensor(bool libre3,const char *sensorid);
bool putwhenneeded(bool libre3,SensorGlucoseData *sensdata) {
	if(!sensdata) {
		return true;
		}
	if(!sensdata->getinfo()->putsensor) {
		time_t nu=time(nullptr)	;
        const char *sensorid= sensdata->showsensorname().data();
		if((nu-sensdata->getstarttime())<(14*24*60*60)) {
			if(!(sensdata->getinfo()->putsensor=putsensor(libre3,sensorid))) {
				LOGGER("putsensor %s failed\n",sensorid);
				return false;
				}
			LOGGER("putsensor %s succeeded\n",sensorid);
			return true;
			}
		else  {
			LOGGER("putsensor %s not done, old sensor %s",sensorid,ctime(&nu));
			return true;
			}
		}
	else {
		LOGGER("putsensor already done\n");
		return true;
		}
	}
bool sendlibreviewdata() {
#ifdef  LIBRE3ONLY
return true;
#endif
LOGGER("start sendlibreviewdata()\n");
 const   uint32_t nu=time(nullptr);
//	int period= 30*24*60*60;
//	int period= 90*24*60*60;
//constexpr const	int period= 90*24*60*60;

	//int period= 8*60*60;

	uint32_t starttime=(settings->data()->lastlibretime)?(settings->data()->lastlibretime):(nu-librekeepsecs);
	extern Sensoren *sensors;

	vector<int> sensints=sensors->inperiod(starttime,nu); 
	const int senslen=sensints.size();
	LibreHist lists[senslen];
	int last=senslen-1;
	int histtotal=0;
	int periodsec=15*60;
	{
	uint32_t starttimeiter=starttime;
	for(int i=last;i>=0;i--) {
		int index=sensints[i];
		SensorGlucoseData *sensdata=sensors->gethist(index);
		if(!sensdata->getinfo()->libreviewsendall) {
			if(sensdata->libreviewable()) {
				const auto el=lists[i]=librehistory(sensdata,starttimeiter,nu);
				if(const auto elsize=el.size) {
					starttimeiter=el.list[elsize-1].ti+periodsec;
					histtotal+=elsize;
					}
				}
			else {
				if(sensdata->isLibre3()&&index>=settings->data()->startlibre3view) {
					settings->data()->haslibre3=true; 
					} 
				lists[i]= {};
				}
			}
		else
			lists[i]= {};
		}
	}
	destruct _dest([&lists,senslen]{
		for(int i=0;i<senslen;i++)
			delete lists[i].list;
		});

#ifdef LIBRENUMBERS
Numbers<libre2> numbers;
int bytesnumbers=numbers.spaceneeded();
LOGGER("numbers.spaceneeded()=%d\n",bytesnumbers);

#else
constexpr const int  bytesnumbers=0;
#endif
	if(!histtotal&&!bytesnumbers) {
		LOGGER("libreview not needed\n");
		return true;
		}
	if(histtotal) {
		for(int i=0;i<senslen;i++) {
			if(lists[i].size) {
				const int index=sensints[i];
				SensorGlucoseData *sensdata=sensors->gethist(index);
				if(!putwhenneeded(false,sensdata))
					return false;
				break;
				}
			}
		}
constexpr const int timeuitlen=sizeof(R"(2022-06-21T00:26:21.098+02:00)")-1;
constexpr const int deviceidlen=sizeof(R"(b76f19a9-d4a0-4d67-8999-56b89b0968ee)")-1;
					//  24ab418b-4f48-4841-92af-eec19e51d8cf
	const int usertokenlen= settings->data()->tokensize;
/*      const char deviceIDin[]="b76f19a9-d4a0-4d67-8999-56b89b0968ee";
     const std::array<const char,36> &deviceID=*reinterpret_cast<const std::array<const char,36> *>(deviceIDin);
   const char usertokenIN[]=R"(Ztu2/3qqaim/NmhN/wzMRlLuHCvDAb18E3htHXqB7arJCP2Da8wVkJfqsvQdVSXU89jy4GyVAy2qMWH1K4nRB7qAZpmWJ7nQte0RJll5G2v9BKMtGmEgeqMYPLjQ+5fQ/7a9TiwtU9zPGShSb5iibQ+npquzk96uILPOqxWbt4sBKc6H0YtXDA8JUkniTW5qxmtQT7U5Aik9i7Drdfn9Q3Yo4vQZ9Sz7oJU+B4z0mkNwQHbr9MbUxa2Td5f1HZMWr6vLYZtU9NvjoiOf7i26+tM+RqzRW/j8/2Pe9Xmmdu0gckbspUmfv/8mRtzUpZ/oO0hhuHQMSiTJlR6KJ+W81ELYbh6eRWlxuAJWhx1EOyCiBHfB05qakBKTNHhcZT4j28Cvyv30nrSg77djP+MfbSD9zf7z6OvUNHiScbPSTLmi3bt4DnZ4lCEwHLwaxGDHh8Y1cUdL1aV+F59wvlEwywxU1MYUxu49XkRYnHPQZx1gju1qS4p1ICwLW45obdOI)";
 const  std::array<const char,512> &usertoken=*reinterpret_cast<const std::array<const char,512>*>(usertokenIN); */
     const std::array<char,36> &deviceID=settings->data()->libreviewDeviceID;

	unsigned int interval=60*60*6;
	uint32_t scantime=0;
	for(int i=last;i>=0;i--) {
		if(lists[i].size) {
			scantime=lists[i].list[0].ti+interval;
			break;
			}
		}

constexpr const int unitlen=6;
	int numscans=nu-scantime;
	uint32_t nrscans=(numscans>0?(numscans/interval):0)+(senslen*2)+1;

	int totallen=bytesnumbers+histtotal*histelUitlen+ startsensorlen*senslen+datastart.size()+unitlen+
afterunit.size()+
afterlocale.size()+
afterlow.size()+10+
afterhigh.size()+4+
aftertimeformat.size()+6+timeuitlen+aftercurrenttime.size()+deviceidlen+afterident.size()+deviceidlen+ afterident2.size()+timeuitlen+
afterlocalstartime.size() +afterhists.size() + afterscans.size()+ usertokenlen+ aftertoken.size()+nrscans*scanelsize+ afterfood.size()+ afterinsulin.size();
#ifndef NDEBUG

time_t tim=scantime;
	LOGGER("usertokenlen=%d histtotal=%d histelUitlen=%d senslen=%d nrscans=%u scanelsize=%d totallen=%d nu=%u scantime=%u %s",usertokenlen,histtotal,histelUitlen,senslen,nrscans,scanelsize,totallen,nu,scantime,ctime(&tim));
#endif
	char *uitbuf=new(std::nothrow) char[totallen+EXTRALEN*10+DEBUGLEN];

	if(!uitbuf) {
		LOGGER("libreview: new char[%d] failed\n",totallen+EXTRALEN*10);
		return false;
		}
     std::unique_ptr<char[]> bufdeleter(uitbuf);


	char *uitptr=uitbuf;
//	time_t nu=time(nullptr);
	addstrview(uitptr,datastart);
	addstrview(uitptr,settings->getunitlabel());
	addstrview(uitptr,afterunit);
	addstrview(uitptr,localestr);
	addstrview(uitptr,afterlocale);
	uitptr+=sprintf(uitptr,"%d",(int)round(settings->targetlow()/10.0));
	addstrview(uitptr,afterlow);
	uitptr+=sprintf(uitptr,"%d",(int)round(settings->targethigh()/10.0));
	addstrview(uitptr,afterhigh);
	uitptr+=sprintf(uitptr,"%d",hour24clock?24:12);
	addstrview(uitptr,aftertimeformat);
	uitptr+=Tdatestringlocal(nu,0,uitptr);
	addstrview(uitptr,aftercurrenttime);

	addstrcont(uitptr,deviceID);
	addstrview(uitptr,afterident);
	addstrcont(uitptr,deviceID);
	addstrview(uitptr,afterident2);
	uitptr+=numbers.writeallfood(uitptr);
	addstrview(uitptr,afterfood);
	const char *startptr=uitptr;
	uitptr+=numbers.writeallnotes(uitptr);
	if(histtotal) {
		uitptr+=makesensorstarts(lists,senslen,uitptr);
		}
	if(uitptr!=startptr)
		--uitptr;
	addstrview(uitptr,afterlocalstartime);
	LOGGER("before numbers.writeallinsulin(uitptr)\n");
	uitptr+=numbers.writeallinsulin(uitptr);
	LOGGER("after numbers.writeallinsulin(uitptr)\n");
	addstrview(uitptr,afterinsulin);

	if(histtotal) {
		uitptr=writelibrehists(lists,senslen,uitptr);
		}
	addstrview(uitptr, afterhists);

	LOGGER("before getscans len=%ld\n",uitptr-uitbuf);

	int scanids[senslen];
	if(histtotal) {
		char *wasuit=uitptr;
		int scansiter=0;
		for(int i=last;i>=0;i--) {
			if(lists[i].size)  {
				int index=sensints[i];
				SensorGlucoseData *sensdata=sensors->gethist(index);
				uint16_t id=sensdata->getinfo()->libreviewscan;
				auto  stream=sensdata->getPolldata();
				const ScanData *endstream=&stream[stream.size()];
				for(const ScanData *startstream=&stream[0];;) {
					const ScanData *found=sensdata->firstnotless({startstream,endstream},scantime);
					if(found==endstream) {
						do {
						    --found;
						    if(found<startstream)  {
							goto ENDSENSOR;
							}
						    } while(!found->valid()); 
						uitptr=libreScanel(*found, ++id,uitptr);
						++scansiter;
						*uitptr++=',';
						scantime+=interval;
						ENDSENSOR:
						scanids[i]=id;
						break;
						}
					const ScanData *wasfound=found;
					while(!found->valid()) {
						--found;
						if(found<startstream) {
							goto NEXTDATA;
							}
						}
					uitptr=libreScanel(*found, ++id,uitptr);
					++scansiter;
					*uitptr++=',';
					scantime+=interval;

					NEXTDATA:
					startstream=wasfound+1;
					}
				 }
			}
		if(uitptr>wasuit)
			--uitptr;
		LOGGER("scancount=%d after getscans len=%ld\n",scansiter,uitptr-uitbuf);
		}
	addstrview(uitptr, afterscans);
	LOGGER("usertoken=%d len=%ld\n",usertokenlen,uitptr-uitbuf);
	memcpy(uitptr, settings->data()->libreviewUserToken,usertokenlen);
	uitptr+=usertokenlen;
	LOGGER("after usertoken len=%ld\n",uitptr-uitbuf);
	addstrview(uitptr, aftertoken);
	*uitptr++='\0';
	const int uitlen=uitptr-uitbuf-1;
//	write(STDOUT_FILENO,uitbuf,uitlen);
	LOGGER("uitlen=%d\n",uitlen);
	logwriter(uitbuf,uitlen);
//#define  NOREALSEND 1
#ifdef NOREALSEND
	bool datasend=false;
	LOGGER("libresendmeasurements not send\n");
#else
	bool datasend=libresendmeasurements(false,uitbuf,uitlen);
	if(datasend) {
		LOGGER("libresendmeasurements success\n");
		numbers.onSuccess();
		if(histtotal) {
			if(last>=0) {
				SensorGlucoseData *previewsens=nullptr;
				int lastlibre2=-1;
				for(int i=last;i>=0;i--) {
					const int index=sensints[i];
					SensorGlucoseData *sensdata=sensors->gethist(index);
					if(sensdata->libreviewable()) {
						if(lists[i].size) {
							sensdata->getinfo()->libreviewscan=scanids[i];
							sensdata->getinfo()->libreviewnotsend=lists[i].notsend;
							if(previewsens) {
								previewsens->getinfo()->libreviewsendall=true;
								}
							}
						previewsens=sensdata;
						lastlibre2=i;
						}
					}
					
				for(int i=0;i<=last;i++) {
					if(lists[i].size) {
						int lastitem=lists[i].size-1;
						settings->data()->lastlibretime=lists[i].list[lastitem].ti;
						#ifndef NDEBUG
						time_t tim=settings->data()->lastlibretime;
						LOGGER("last item was %s",ctime(&tim));
						#endif
						break;
						}
					}
				if(lastlibre2>=0) {
					settings->data()->startlibreview=sensints[lastlibre2];
					}
				}
			}
		}
	else
		LOGGER("libresendmeasurements failure\n");
#endif	
	return datasend;
	}
#include <jni.h>
#include <sys/prctl.h>
extern JNIEnv *getenv();

#include "datbackup.h"

Backup::condvar_t  librecondition;
void initlibreviewjni(JNIEnv *env) {
	const char librclassstr[]="tk/glucodata/Libreview";
	if(jclass cl=env->FindClass(librclassstr)) {
      		LOGGER("found %s\n",librclassstr);
		libreviewclass=(jclass)env->NewGlobalRef(cl);
	       env->DeleteLocalRef(cl);
	       }
      else  {
      	LOGGER("FindClass(%s) failed\n",librclassstr);
      	}
       }
static bool initlibreconfig(bool libre3,bool restart) {
	static jmethodID  libreconfig=getenv()->GetStaticMethodID(libreviewclass,"libreconfig","(ZZ)Z");
	return   getenv()->CallStaticBooleanMethod(libreviewclass,libreconfig,libre3,restart);
	}
/*
static bool putsensor(const char *sensorid) {
	JNIEnv *env=getenv();
	static jmethodID  putsensor=env->GetStaticMethodID(libreviewclass,"putsensor","(Ljava/lang/String;)Z");
	const jstring jsensorid= env->NewStringUTF(sensorid);
	bool res=env->CallStaticBooleanMethod(libreviewclass,putsensor,jsensorid);
        env->DeleteLocalRef(jsensorid);
	return res;
	} */
static bool putsensor(bool libre3,const char *sensorid) {
	JNIEnv *env=getenv();
	static jmethodID  putsensor=env->GetStaticMethodID(libreviewclass,"putsensor","(Z[B)Z");
	jbyteArray message=libre3?getlibre3puttext(env, sensorid):getlibre2puttext(env, sensorid);
	bool res=env->CallStaticBooleanMethod(libreviewclass,putsensor,libre3,message); 
	env->DeleteLocalRef(message);
	return res;
	}

static bool libreviewrunning=false;
static bool alwaysnewstatus=false;
static bool alwaysnewstatus3=false;

static bool askforaccount=false;
extern bool networkpresent;

void libreviewthread() {
	sensors->setversions();
//	settings->data()->libreinit3=false;
	LOGGER("libreinit3=%d\n", settings->data()->libreinit3);
	libreviewrunning=true;
	const char view[]{"VIEW"};
	LOGGERN(view,sizeof(view)-1);
       prctl(PR_SET_NAME, view, 0, 0, 0);
	int waitmin=10;
	while(true) {
		  if(!networkpresent||!librecondition.dobackup) {
			if(!networkpresent) {
				waitmin=60;
				}
		        std::unique_lock<std::mutex> lck(librecondition.backupmutex);
			LOGGER("VIEW before lock waitmin=%d\n",waitmin);
 			auto now = std::chrono::system_clock::now();
			auto status=librecondition.backupcond.wait_until(lck, now + std::chrono::minutes(waitmin));
			LOGGER("VIEW after lock %stimeout\n",(status==std::cv_status::no_timeout)?"no-":"");
			}
		if(!librecondition.dobackup)
			continue;
		if(librecondition.dobackup&Backup::wakeend) {
			librecondition.dobackup=0;
			libreviewrunning=false;
			LOGGER("end libreviewthread\n");
			return;
			}
extern bool askhasnewcurrent();
		bool hasnewcurrent=askhasnewcurrent();
		if((librecondition.dobackup&Backup::wakeall)||(hasnewcurrent&&(librecondition.dobackup&Backup::wakestream))) {
			librecondition.dobackup=0;
			if(askforaccount||settings->data()->haslibre3) {
				LOGGER("Libreview: haslibre3\n");
				if((!settings->data()->libreinit3)){
					if(!(settings->data()->libreinit3=initlibreconfig(true,alwaysnewstatus3))) {
						alwaysnewstatus3=false;
						LOGGER("initlibreconfig failed\n");
						continue;
						}
					LOGGER("initlibreconfig success %d\n",settings->data()->libreinit3);
					alwaysnewstatus3=false;
					}
				if(!settings->data()->sendtolibreview) {
					askforaccount=false;
					settings->data()->uselibre=false;
					libreviewrunning=false;
					LOGGER("end libreview thread, account id only\n");
					return;
					}
				if(sendlibre3viewdata(hasnewcurrent)) {
					waitmin=5*60;
					}
				else {
					waitmin=15;
					}
				}
			if(settings->data()->haslibre2) {
				LOGGER("Libreview: haslibre2\n");
				if(!settings->data()->libreinit){
					if(!(settings->data()->libreinit=initlibreconfig(false,alwaysnewstatus))) {
						alwaysnewstatus=false;
						LOGGER("initlibreconfig failed\n");
						continue;
						}
					alwaysnewstatus=false;
					}
				if(!settings->data()->sendtolibreview) {
					askforaccount=false;
					settings->data()->uselibre=false;
					libreviewrunning=false;
					LOGGER("end libreview thread, account id only\n");
					return;
					}
				if(sendlibreviewdata()) {
					waitmin=5*60;
					}
				else {
					waitmin=15;
					continue;
					}
				}
			}
		else
			librecondition.dobackup=0;
		}
	}

extern void wakeaftermin(const int waitmin) ;
void wakeaftermin(const int waitmin) {
	if(libreviewrunning) {
		if(waitmin) {
			 uint32_t now=time(nullptr);
			 uint32_t prevwake=settings->data()->lastlibretime;
			 if((now-prevwake)<60*waitmin)
			 	return;
			 }
		librecondition.wakebackup(Backup::wakeall);
		}
	else
		librecondition.dobackup=Backup::wakeall;
 }
void wakelibrecurrent() {
	if(libreviewrunning) {
		librecondition.wakebackup(Backup::wakestream);
		}
	else
		librecondition.dobackup=Backup::wakestream;
	}
void clearlibregegs() {
	settings->data()->libreinit=false;
	settings->data()->libreinit3=false;
	settings->data()->lastlibretime=0;
	settings->data()->startlibreview=0;
	settings->data()->startlibre3view=0;
	sensors->onallsensors([](SensorGlucoseData *sens) {
		auto *info=sens->getinfo();
		info->putsensor=false;
		info->libreviewscan=0;
		info->sendsensorstart=false;
		info->libreviewnotsend=0;
		info->libreviewsendall=false;
		});
	sensors->setversions();
#ifdef LIBRE3ONLY
	settings->data()->haslibre2=false; 
#endif
#ifdef LIBRENUMBERS
	settings->data()->libredeletednr=0;
	for(Numdata*numdat:numdatas) {
		numdat->getlibrechangednr()=0;
		numdat->getlibresend()=0;
		numdat->getlibreSolid()=0;
		}
	 settings->data()->libre3NUMiter=1;
	 settings->data()->libre2NUMiter=1;
#endif
	}
#include "fromjava.h"

extern "C" JNIEXPORT void JNICALL fromjava(clearlibreview) (JNIEnv *env, jclass clazz) {
	clearlibregegs();
	}
extern "C" JNIEXPORT void JNICALL fromjava(wakelibreview) (JNIEnv *env, jclass clazz,int minutes) {
	if(networkpresent)
		wakeaftermin(minutes) ;
	}
/*
static bool started() {
		srand(time(nullptr));
	}
static bool isstarted=started();
*/
#include <thread>
void startlibrethread() {
	if(!libreviewrunning) {

		srand(time(nullptr));
		std::thread libre(libreviewthread);
		libre.detach();
		}
	}
void endlibrethread() {
	if(libreviewrunning) {
		librecondition.wakebackup(Backup::wakeend);
		}
	}
extern "C" JNIEXPORT jboolean  JNICALL   fromjava(getuselibreview)(JNIEnv *env, jclass cl) {
//	 return settings->data()->uselibre; 
	 return settings->data()->sendtolibreview;
	}

extern "C" JNIEXPORT void  JNICALL   fromjava(setuselibreview)(JNIEnv *env, jclass cl,jboolean val) {
	alwaysnewstatus=true;
	 settings->data()->sendtolibreview=val;
	 if(val) {
	 	settings->data()->uselibre=true; 
		 startlibrethread();
		 }
//	else endlibrethread();
	}

extern "C" JNIEXPORT void  JNICALL   fromjava(askServerforAccountID)(JNIEnv *env, jclass cl) {
	alwaysnewstatus=true;
	askforaccount=true;
	settings->data()->haslibre3=true; 
	settings->data()->uselibre=true; 
	settings->data()->libreinit3=false;
	//settings->data()->libreinit=false;
//	librecondition.dobackup=Backup::wakeall;
	startlibrethread();
	wakeaftermin(0);
	 }
std::string_view dRELEASE;
std::string_view dMANUFACTURER;
std::string_view dMODEL;

std::string_view getjstring(JNIEnv *env,jstring jstr)  {
	size_t strlen= env->GetStringUTFLength( jstr);
	jint jstrlen = env->GetStringLength( jstr);
	char *strbuf=new char[strlen+1];
	env->GetStringUTFRegion( jstr, 0,jstrlen, strbuf);
	strbuf[strlen]='\0';
	return {strbuf,strlen};
	}

extern "C" JNIEXPORT void JNICALL fromjava(setDevice) (JNIEnv *env, jclass clazz, jstring jMANUFACTURER, jstring jMODEL, jstring jRELEASE) {
	if(jMANUFACTURER)
		dMANUFACTURER=getjstring(env,jMANUFACTURER);
	if(jMODEL)
		dMODEL=getjstring(env,jMODEL);
	if(jRELEASE)
		dRELEASE=getjstring(env,jRELEASE);
	}
/*
int makesensorstarts(const LibreHist *list,const int len,char *buf) {
	char *bufptr=buf;
	for(int i=len-1;;) {

		bufptr+=mksensorstart(list[i].histor,list[i].starttime,bufptr);
		if(i) {
			*bufptr++=',';
			--i;
			}
		else {
			break;
			}
		}
	return bufptr-buf;
	} */



#endif
