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

#ifdef NOLOG
30
#else
0
#endif
;
constexpr const int histelUitlen=sizeof(R"({"valueInMgPerDl":159.0,"extendedProperties":{"factoryTimestamp":"2022-06-20T20:24:33.000Z","isFirstAfterTimeChange":false},"recordNumber":-6629292502336270757,"timestamp":"2022-06-20T22:24:33.000+02:00"},)")+EXTRALEN;

constexpr const int startsensorlen=sizeof(R"({"type":"com.abbottdiabetescare.informatics.sensorstart","extendedProperties":{"factoryTimestamp":"2022-06-18T05:57:19.000Z"},"recordNumber":-6629292502336274432,"timestamp":"2022-06-18T07:57:19.000+02:00"})")+EXTRALEN;

//int64_t recordnumbermask=-6629292502336274432L;
//constexpr const  std::string_view datastart=R"({"DeviceData":{"deviceSettings":{"factoryConfig":{"UOM":")"; 
constexpr const  std::string_view datastart=R"({"DeviceData":{"connectedDevices":{"insulinDevices":[]},"deviceSettings":{"factoryConfig":{"UOM":")"; 
//s/char\(.*\)\[\]=R\(.*\)$/constexpr  std::string_view \1=R\2/g
constexpr const  std::string_view afterunit= R"("},"firmwareVersion":"2.10.1","miscellaneous":{"isStreaming":)";
constexpr const  std::string_view afterstreaming= R"(,"selectedLanguage":")";
constexpr const  std::string_view afterlocale=R"(","valueGlucoseTargetRangeLowInMgPerDl":)";
constexpr const  std::string_view afterlow=R"(,"valueGlucoseTargetRangeHighInMgPerDl":)";
constexpr const  std::string_view afterhigh=R"(,"selectedTimeFormat":")";
constexpr const  std::string_view aftertimeformat=R"(hr","selectedCarbType":"grams of carbs"},"timestamp":")";
const constexpr  std::string_view  aftercurrenttime=R"("},"forceUpload":false,"header":{"device":{"hardwareDescriptor":")";
const constexpr std::string_view afterHardDescr=R"(","hardwareName":")";
const constexpr  std::string_view  afterHardwareName=R"(","modelName":")";
const constexpr  std::string_view  aftermodelName=R"(","osType":"Android","osVersion":")";
const constexpr  std::string_view  afterosVersionName=R"(","uniqueIdentifier":")";
constexpr const  std::string_view  afterident= R"("}},"measurementLog":{"bloodGlucoseEntries":[],"capabilities":["scheduledContinuousGlucose","unscheduledContinuousGlucose","currentGlucose","bloodGlucose","insulin","food","generic-com.abbottdiabetescare.informatics.exercise","generic-com.abbottdiabetescare.informatics.customnote","generic-com.abbottdiabetescare.informatics.ondemandalarm.low","generic-com.abbottdiabetescare.informatics.ondemandalarm.high","generic-com.abbottdiabetescare.informatics.ondemandalarm.projectedlow","generic-com.abbottdiabetescare.informatics.ondemandalarm.projectedhigh","generic-com.abbottdiabetescare.informatics.sensorstart","generic-com.abbottdiabetescare.informatics.error","generic-com.abbottdiabetescare.informatics.isfGlucoseAlarm","generic-com.abbottdiabetescare.informatics.alarmSetting","generic-com.abbottdiabetescare.informatics.sensorend","generic-com.abbottdiabetescare.informatics.bleDisconnect","generic-com.abbottdiabetescare.informatics.bleReconnect"],"currentGlucoseEntries":[)";



const constexpr std::string_view aftercurrents=R"(],)";
const constexpr std::string_view afterdevice2=R"(,"foodEntries":[)";
const constexpr  std::string_view  afterfood=R"(],"genericEntries":[)";




//const constexpr  std::string_view  afterident2=R"("},"foodEntries":[)";
const constexpr  std::string_view  afterlocalstartime=R"(],"insulinEntries":[)";
const constexpr  std::string_view  afterinsulin=R"(],"ketoneEntries":[],"scheduledContinuousGlucoseEntries":[)";
constexpr const std::string_view afterhists=R"(],"unscheduledContinuousGlucoseEntries":[)";
//{"valueInMgPerDl":163.0,"extendedProperties":{"factoryTimestamp":"2022-06-20T20:49:33.000Z","trendArrow":"Stable","isActionable":true,"isFirstAfterTimeChange":false},"recordNumber":2304,"timestamp":"2022-06-20T22:49:33.000+02:00"},{"valueInMgPerDl":133.0,"extendedProperties":{"factoryTimestamp":"2022-06-20T21:15:01.000Z","trendArrow":"Stable","isActionable":true,"isFirstAfterTimeChange":false},"recordNumber":2560,"timestamp":"2022-06-20T23:15:01.000+02:00"}
const constexpr  std::string_view  afterscans=R"(]}},"UserToken":")";
const constexpr  std::string_view  aftertoken=R"(","Domain":"Libreview","GatewayType":"FSLibreLink.Android"})";



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
#include "LibreHist.hpp"
extern void switchrealhistory(SensorGlucoseData *sensdata,bool torealhistory) ;
extern LibreHist  libreRealHistory(SensorGlucoseData *sens,uint32_t starttime,uint32_t nu) ;
extern void setlibresend(SensorGlucoseData *sens,const LibreHist &hists) ;

constexpr const char *int64printf() {
	 if constexpr(sizeof(int64_t)==sizeof(long))
		return "%ld";
	else
		return "%lld";
	}
static char *librehistel(const LibreHistEl *el,const int64_t histor,char *buf) {
	char *ptr=buf;
	valuestart(ptr, (float)el->mgdL);
	int mil=getmmsec();
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



           /* {
               "valueInMgPerDl": 117,
               "extendedProperties": {
                  "factoryTimestamp": "2023-09-08T19:57:38.000Z",
                  "lowOutOfRange": "false",
                  "highOutOfRange": "false",
                  "trendArrow": "Stable",
                  "isActionable": "true",
                  "isFirstAfterTimeChange": "false"
               },
               "recordNumber": 256,
               "timestamp": "2023-09-08T21:57:38.000+02:00"
            } */

static char *libreScanel(const ScanData &scanel,const int16_t  nr,char *ptr) {
	const auto mgdL= scanel.getmgdL();
	valuestart(ptr, (float)mgdL);
	int mil=0;
	ptr+=TdatestringGMT(scanel.gettime(),mil,ptr);
	int16_t id=nr>>8|(nr&0xFF)<<8;
	const char lowout[]=R"(","lowOutOfRange":")";
	addar(ptr,lowout);
	if(mgdL<40) {
		addar(ptr,"true");
		}
	else
		addar(ptr,"false");
	const char highout[]=R"(","highOutOfRange":")";
	addar(ptr,highout);
	if(mgdL>500) {
		addar(ptr,"true");
		}
	else
		addar(ptr,"false");

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
#endif
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
			LOGAR("no hist: no data");
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
				LOGAR("no librehist no whole period");
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
		if(found>laststream)  {
			LOGGER("found>laststream .notsend=%d\n",streamlen);
			return {.notsend=streamlen};
			}
		if(found->valid()) 
			break;
		++found;
		}; 
	if(!startid)
		startid=found->id;
	int histnum=(startid + periodright-1)/periodmin;
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
	const uint32_t notsendHistory=datuit?((list[datuit-1].id/15)+1):0;
	LOGGER("send %d data notsend=%d notsendHistory=%d\n",datuit,newnotsend,notsendHistory);

	return {.histor=histor,.list=list,.size=datuit,.starttime=sensdata->getstarttime(),.msec=0,.notsend=newnotsend,.notsendHistory=notsendHistory,.sendstart=sensdata->getinfo()->sendsensorstart };
	}



//{"type":"com.abbottdiabetescare.informatics.sensorstart","extendedProperties":{"factoryTimestamp":"2023-09-09T20:16:00.000Z","gmin":"40","gmax":"500","latejoin":"false","puckGen":"3","wearDuration":"20160","warmupTime":"60","isStreaming":"true","productType":"3"},"recordNumber":-6629287740365012992,"timestamp":"2023-09-09T22:16:00.000+02:00"}
//{"type":"com.abbottdiabetescare.informatics.sensorstart","extendedProperties":{"factoryTimestamp":"2023-09-09T20:16:00.000Z","gmin":"40","gmax":"500","latejoin":"false","puckGen":"3","wearDuration":"20160","warmupTime":"60","isStreaming":"true","productType":"3"},"recordNumber":-6629287740365012992,"timestamp":"2023-09-09T22:16:00.000+02:00"}
static int mksensorstart(int64_t recordnumbermask, time_t starttime,int msec,char *buf) {
	static constexpr const char start[]=R"({"type":"com.abbottdiabetescare.informatics.sensorstart","extendedProperties":{"factoryTimestamp":")";
	static constexpr const int startlen=sizeof(start)-1;
	memcpy(buf,start,startlen);
	char *ptr=buf+startlen;
	ptr+=TdatestringGMT(starttime,msec,ptr);
       static const char afterstartimeGMT[]= R"(","gmin":"40","gmax":"500","latejoin":"false","puckGen":"3","wearDuration":"20160","warmupTime":"60","isStreaming":"true","productType":"3"},"recordNumber":)";
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
	if(el.sendstart)
		return 0;
	return mksensorstart(el.histor,el.starttime,el.msec,buf);
	}
static int writelist(const LibreHist *list,const int len,char *buf,int (*func)(const LibreHist &el,char *buf)) {
	char *bufptr=buf;
	for(int i=len-1;i>=0;--i) {
		if(list[i].size) {
			const int added=func(list[i],bufptr);
			if(added) {
				bufptr+=added;
				*bufptr++=',';
				}
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
extern std::string_view localestr;
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
		LOGAR("putsensor already done");
		return true;
		}
	}
extern time_t lastviewtime;
extern time_t nexttimeviewed;
extern int betweenviews;
bool getisviewed(time_t wastime) {
	bool viewed;
	if(settings->data()->libreIsViewed
#ifdef NOTALLVIES
	&&wastime>nexttimeviewed
#endif
			)
	{
		int diff=((long long)wastime-lastviewtime);
		viewed=abs(diff)<60;
		LOGGER("diff=%d viewed=%d\n",diff,viewed);
		if(viewed) {
//			nexttimeviewed=wastime+betweenviews;
			return true;
			}
		}
	return false;
	}
static char *onecurrent(const ScanData &scanel,const int  nr,char *ptr,bool isviewed) {
	const auto mgdL= scanel.getmgdL();
	valuestart(ptr, (float)mgdL);
	int mil=getmmsec();
	auto wastime= scanel.gettime();
	
	ptr+=TdatestringGMT(wastime,mil,ptr);
	const char currentformat[]=R"(","isViewed":"%s","lowOutOfRange":"%s","highOutOfRange":"%s","trendArrow":"%s","isActionable":"true","isFirstAfterTimeChange":"false"},"recordNumber":%d,"timestamp":")";
	const int id=nr*256;
	ptr+=sprintf(ptr,currentformat,isviewed?"true":"false",mgdL<40?"true":"false", mgdL>500?"true":"false", trendName[scanel.tr],id);
	ptr+=Tdatestringlocal(scanel.gettime(),mil,ptr);
	addar(ptr,R"("})");
	return ptr;
	};

static void addoldcurrents(char *&uitptr,const SensorGlucoseData *sens,int start,int current) { 
	if(!settings->data()->libreIsViewed)
		return;
	const auto &viewed=sens->viewed;
	const ScanData *startstream=sens->beginpolls();
	for(int i:viewed) {
		if(i>=start&&i<current) {
			const ScanData *el=startstream+i;
			uitptr=onecurrent(*el,i,uitptr,true);
			*uitptr++=',';
			}
		}
	}
static int addcurrents(char *&uitptr,time_t nu,const SensorGlucoseData *sens) { 
	if(!sens)  {
		LOGAR("addcurrents: SensorGlucoseData==null");
		return -1;
		}
	 int start=sens->getinfo()->libreCurrentIter;
	int pollstart= sens->getinfo()->pollstart;
	int		ends=sens->pollcount()-1;
	LOGGER("%s pollstart=%d libreCurrentIter=%d ends=%d\n",sens->showsensorname().data(),pollstart,start,ends);
	if(start<pollstart)
		start=pollstart;
	LOGGER("start=%d ends=%d\n",start,ends);
	if(start>ends) {
		LOGAR("addcurrents: start>ends");
		return ends;
		}
	const time_t old=nu-5*60;
	const ScanData *startstream=sens->beginpolls();
	for(int i=ends;i>=start;i--) {
		const ScanData *el=startstream+i;
		if(el->current(i)) {
			auto wastime= el->gettime();
			if(wastime<old)
				return 0;
			const auto &viewed=sens->viewed;
			bool isViewed=false;
			int id=i;
			if(settings->data()->libreIsViewed) {
				if(!viewed.empty()) {
					int previd=viewed.back();
					const ScanData *prev=startstream+previd;
					if(((long long)wastime-prev->gettime())<
#ifdef NOTALLVIES
betweenviews
#else
					60
#endif
					) {
						LOGGER("previd=%d near %d\n",previd,i);
						isViewed=true;
						id=previd;
						}
					else {
						LOGGER("previd=%d %x near %d %x\n",previd,prev->gettime(),i,wastime);
						}
					}
				}
			uitptr=onecurrent(*el,i,uitptr,isViewed);
			*uitptr++=',';
			addoldcurrents(uitptr,sens,start,id);
			return ends;
			}
		}
	addoldcurrents(uitptr,sens,start,INT_MAX);
	return ends;
	}
 extern bool streamHistory();
bool sendlibreviewdata(const int newcurrent,const uint32_t nu) {
const bool hasnewcurrent=newcurrent>=0;
#ifdef  LIBRE3ONLY
return true;
#endif
//const bool streamhist=streamHistory();
	uint32_t starttime=(settings->data()->lastlibretime)?(settings->data()->lastlibretime):(nu-librekeepsecs);
	extern Sensoren *sensors;

	vector<int> sensints=sensors->inperiod(starttime,nu); 
	const int senslen=sensints.size();

#ifndef NOLOG
	time_t tim=starttime;
	LOGGER("start %d sensors sendlibreviewdata librekeepsecs=%d from %s",senslen,librekeepsecs,ctime(&tim));
#endif

	if(senslen<=0) {
		LOGAR("not sensors in period");
		return true;
		}

	LibreHist lists[senslen];
	const int last=senslen-1;
	int histtotal=0;
	int periodsec=15*60;
	int nrscans=0;

int startsensor=0;	

	{
	int usedsensor=0;
	uint32_t starttimeiter=starttime;
	int i=last;
	for(;i>=0;i--) {
		int index=sensints[i];
		SensorGlucoseData *sensdata=sensors->getSensorData(index);
		auto *info=sensdata->getinfo();
		if(!info->libreviewsendall) {
			if(!sensdata->isLibre3()) {
				const bool userealhistory=  (info->startedwithStreamhistory&&( info->libreviewnotsendHistory>= info->startedwithStreamhistory||info->startedwithStreamhistory==1))
						||!sensdata->pollcount();
				switchrealhistory(sensdata,userealhistory);
				const auto el=lists[i]=(userealhistory?libreRealHistory:librehistory)(sensdata,starttimeiter,nu);
				if(const auto elsize=el.size) {
					starttimeiter=el.list[elsize-1].ti+periodsec;
					histtotal+=elsize;
					}
				int scanoff=info->scanoff;
				int startid=info->libreviewScan;
				int lastscan=scanoff+startid;
				if(!info->newscan||lastscan>sensdata->scancount()) {
					auto  scans=sensdata->getScandata();
					const ScanData *startscans=&scans[0];
					const ScanData *endscans=&scans[scans.size()];
					const auto after=settings->data()->lastlibretime+60*20;
					const ScanData *found=sensdata->firstnotless({startscans,endscans},after);
					int index=found-startscans;
					scanoff=info->scanoff=index-info->libreviewScan;
					info->newscan=true;
					lastscan=scanoff+startid;
					}
				LOGGER("startedwithStreamhistory=%d scanoff=%d startid=%d lastscan=%d scancount=%d\n",info->startedwithStreamhistory,scanoff,startid,lastscan,sensdata->scancount());
				nrscans+=(sensdata->scancount() -lastscan);

				if(usedsensor>5) {
					startsensor=i;
					break;
					}
				++usedsensor;
				}
			else {
				if(index>=settings->data()->startlibre3view) {
					settings->data()->haslibre3=true; 
					} 
				lists[i]= {};
				}
			}
		else
			lists[i]= {};
		}
	}
	destruct _dest([&lists,startsensor,senslen]{
		for(int i=startsensor;i<senslen;i++)
			delete lists[i].list;
		});

#ifdef LIBRENUMBERS
Numbers<libre2> numbers;
int bytesnumbers=numbers.spaceneeded();
LOGGER("startsensor=%d numbers.spaceneeded()=%d\n",startsensor,bytesnumbers);

#else
constexpr const int  bytesnumbers=0;
#endif
	if(!nrscans&&!histtotal&&!bytesnumbers&&!hasnewcurrent) {
		LOGAR("libreview not needed");
		return true;
		}

	if(nrscans||histtotal||hasnewcurrent) {
		for(int i=startsensor;i<senslen;i++) {
			if(lists[i].size||(i==newcurrent)) {
				const int index=sensints[i];
				SensorGlucoseData *sensdata=sensors->getSensorData(index);
				if(!sensdata->getinfo()->libreviewsendall) {
					if(!putwhenneeded(false,sensdata))
						return false;
					break;
					}
				}
			}
		}
constexpr const int timeuitlen=sizeof(R"(2022-06-21T00:26:21.098+02:00)")-1;
constexpr const int deviceidlen=sizeof(R"(b76f19a9-d4a0-4d67-8999-56b89b0968ee)")-1;
	const int usertokenlen= settings->data()->tokensize;
     const std::array<char,36> &deviceID=settings->data()->libreviewDeviceID;

constexpr const std::string_view appstart=R"(com.freestylelibre.app.)";
/*
constexpr const std::string_view mmolmodel=R"(com.freestylelibre.app.gb)";
constexpr const std::string_view mgmodel=R"(com.freestylelibre.app.fr)";
constexpr const std::string_view mmolmodel=R"(com.freestylelibre.app.nl)";
constexpr const std::string_view mgmodel=R"(com.freestylelibre.app.pl)";
constexpr const std::string_view rumodel=R"(com.freestylelibre.app.ru)"; */
constexpr const char countries[][3]={"gb","fr","nl","pl","ru"};


constexpr const int unitlen=6;

	int totallen=bytesnumbers+histtotal*histelUitlen+ startsensorlen*senslen+datastart.size()+unitlen+
afterunit.size()+
afterstreaming.size()+
afterlocale.size()+
afterlow.size()+10+
afterhigh.size()+4+
aftertimeformat.size()+6+timeuitlen+
//s/addstrview(uitptr,\(.*\));/\1.size()+/g
(
aftercurrenttime.size()+ 
dMODEL.size()+
	afterHardDescr.size()+
	dMANUFACTURER.size()+
	afterHardwareName.size()+
	aftermodelName.size()+
	dRELEASE.size()+
	afterosVersionName.size()+
    appstart.size()+
sizeof(countries[0])+
deviceidlen+ afterident.size())*2+ 350 + timeuitlen+
	aftercurrents.size()
	+afterdevice2.size() +
afterlocalstartime.size() +afterhists.size() + afterscans.size()+ usertokenlen+ aftertoken.size()+nrscans*scanelsize+ afterfood.size()+ afterinsulin.size();

#ifndef NOLOG

//time_t stim=scantime;
	LOGGER("usertokenlen=%d histtotal=%d histelUitlen=%d senslen=%d nrscans=%u scanelsize=%d totallen=%d nu=%u \n",usertokenlen,histtotal,histelUitlen,senslen,nrscans,scanelsize,totallen,nu);
#endif
	char *uitbuf=new(std::nothrow) char[totallen+EXTRALEN*100+DEBUGLEN];

	if(!uitbuf) {
		LOGGER("libreview: new char[%d] failed\n",totallen+EXTRALEN*100);
		return false;
		}
     std::unique_ptr<char[]> bufdeleter(uitbuf);

static    const bool mmolL=settings->data()->isLibreMmolL();
static    const int country=settings->data()->getLibreCountry();
static  constexpr const char unitlabel[][7]={"mg/dL","mmol/L"};
	char *uitptr=uitbuf;
	addstrview(uitptr,datastart);
	addstrview(uitptr,unitlabel[mmolL]);
	addstrview(uitptr,afterunit);
	SensorGlucoseData *currentsensor=sensors->getSensorData(newcurrent);
	if(!currentsensor||!currentsensor->pollcount()) {
		addar(uitptr,"false");
		}
	else {
		addar(uitptr,"true");
		}
	addstrview(uitptr,afterstreaming);
	addstrview(uitptr,localestr);
	addstrview(uitptr,afterlocale);
	uitptr+=sprintf(uitptr,"%d",(int)round(settings->targetlow()/10.0));
	addstrview(uitptr,afterlow);
	uitptr+=sprintf(uitptr,"%d",(int)round(settings->targethigh()/10.0));
	addstrview(uitptr,afterhigh);
	uitptr+=sprintf(uitptr,"%d",hour24clock?24:12);
	addstrview(uitptr,aftertimeformat);
	uitptr+=Tdatestringlocal(nu,0,uitptr);
	const char *devicestart=uitptr+33;
	addstrview(uitptr,aftercurrenttime);

	addstrview(uitptr,dMODEL);
	addstrview(uitptr,afterHardDescr);
	addstrview(uitptr,dMANUFACTURER);
	addstrview(uitptr,afterHardwareName);
	addstrview(uitptr,appstart);
	addar(uitptr,countries[country]);
	addstrview(uitptr,aftermodelName);
	addstrview(uitptr,dRELEASE);
	addstrview(uitptr,afterosVersionName);



	addstrcont(uitptr,deviceID);
	int devicelen=uitptr-devicestart+2;
	addstrview(uitptr,afterident);
	auto *startuitptr=uitptr;
	addcurrents(uitptr,nu,currentsensor);
	bool currentsend;
	if(uitptr!=startuitptr) {
		--uitptr;
		currentsend=true;
		}
	else
		currentsend=false;
	addstrview(uitptr, aftercurrents);

	memcpy(uitptr,devicestart,devicelen);
	uitptr+=devicelen;

	addstrview(uitptr,afterdevice2);
	uitptr+=numbers.writeallfood(uitptr);
	addstrview(uitptr,afterfood);
	const char *startptr=uitptr;
	uitptr+=numbers.writeallnotes(uitptr);
	uitptr+=makesensorstarts(lists+startsensor,senslen-startsensor,uitptr);
	if(uitptr!=startptr)
		--uitptr;
	addstrview(uitptr,afterlocalstartime);
	LOGAR("before numbers.writeallinsulin(uitptr)");
	uitptr+=numbers.writeallinsulin(uitptr);
	LOGAR("after numbers.writeallinsulin(uitptr)");
	addstrview(uitptr,afterinsulin);

	if(histtotal) {
		uitptr=writelibrehists(lists+startsensor,senslen-startsensor,uitptr);
		}
	addstrview(uitptr, afterhists);

	LOGGER("before getscans len=%ld\n",uitptr-uitbuf);

	int scanids[senslen];
	{
		auto starttimeiter=starttime;
		char *wasuit=uitptr;
		int scansiter=0;
		for(int i=last;i>=startsensor;i--) {
			int index=sensints[i];
			SensorGlucoseData *sensdata=sensors->getSensorData(index);

			if(!sensdata->getinfo()->libreviewsendall) {
				auto  scans=sensdata->getScandata();
				const ScanData *endscans=&scans[scans.size()];
				int scanoff=sensdata->getinfo()->scanoff;
				int startid=sensdata->getinfo()->libreviewScan;
				int id=startid;
				if(startid) {
					starttimeiter=0;
					}
				LOGGER("scanoff=%d startid=%d end=%d\n",scanoff,startid,scans.size());
				for(const ScanData *iter=&scans[scanoff+startid];iter<endscans;++iter) {
					++id;
					if(iter->valid()&&iter->gettime()>starttimeiter) {
						uitptr=libreScanel(*iter, id,uitptr);
						*uitptr++=',';
						++scansiter;
						}
					}
				scanids[i]=id;
				if(lists[i].size) {
					starttimeiter=lists[i].list[lists[i].size-1].ti;
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
	LOGAR("libresendmeasurements not send");
#else
	bool datasend=libresendmeasurements(false,uitbuf,uitlen);
	if(datasend) {
		LOGAR("libresendmeasurements success");
		numbers.onSuccess();
		if(currentsend) {
		   currentsensor->getinfo()->libreCurrentIter=currentsend+1;
			currentsensor->getinfo()->sendsensorstart=true;
		   }
		//if(histtotal) 
		{
			if(last>=startsensor) {
				SensorGlucoseData *previewsens=nullptr;
				int lastlibre2=-1;
				for(int i=last;i>=startsensor;i--) {
					const int index=sensints[i];
					SensorGlucoseData *sensdata=sensors->getSensorData(index);
					if(!sensdata->getinfo()->libreviewsendall) {
						if(!sensdata->isLibre3()) {
							sensdata->viewed.clear();
							if(newcurrent==i||sensdata->getinfo()->libreviewScan!=scanids[i]||lists[i].size) { 
								sensdata->getinfo()->sendsensorstart=true;
								if(previewsens) {
									previewsens->getinfo()->libreviewsendall=true;
									}
								}
							sensdata->getinfo()->libreviewScan=scanids[i];
							if(lists[i].size) {
								if(lists[i].notsend)
									sensdata->getinfo()->libreviewnotsend=lists[i].notsend;
								if(lists[i].notsendHistory)
									sensdata->getinfo()->libreviewnotsendHistory=lists[i].notsendHistory;
								setlibresend(sensdata,lists[i]);
								}
							previewsens=sensdata;
							lastlibre2=i;

							}
						}
					}
					
				for(int i=startsensor;i<=last;i++) {
					if(lists[i].size) {
						int lastitem=lists[i].size-1;
						settings->data()->lastlibretime=lists[i].list[lastitem].ti;

						#ifndef NOLOG
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
		LOGAR("libresendmeasurements failure");
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

int askhasnewcurrent(time_t nu) {
	if(!settings->data()->LibreCurrentOnly) { 
		LOGSTRING("LibreCurrentOnly==false\n");
		return -1;
		}
	const auto *lastsensor=sensors->getSensorData(-1);
	if(!lastsensor)  {
		LOGAR("no sensor");
		return -1;
		}
	auto oldtime=nu-15*24*60*60;
	int last=sensors->last();
	for(int i=last;i>=0;--i) {	
		const auto *sens=sensors->getSensorData(i);
		 int start=sens->isLibre3()?sens->getinfo()->libreviewScan:sens->getinfo()->libreCurrentIter;
		int pollstart= sens->getinfo()->pollstart;
		if(start<pollstart)
			start=pollstart;
		const int		ends=sens->pollcount()-1;
		if(start<=ends)
			return i;
		if(sens->getstarttime()	<oldtime)
			return -1;
		}
	return -1;
	}
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
				librecondition.dobackup=0;
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
			LOGAR("end libreviewthread");
			return;
			}
 		const   time_t nu=time(nullptr);
		const int newcurrent=askhasnewcurrent(nu);
		const bool hasnewcurrent=newcurrent>=0;
		if((librecondition.dobackup&Backup::wakeall)||(hasnewcurrent&&(librecondition.dobackup&Backup::wakestream))) {
			librecondition.dobackup=0;
			if(askforaccount||settings->data()->haslibre3) {
				LOGAR("Libreview: haslibre3");
				if((!settings->data()->libreinit3)){
					if(!(settings->data()->libreinit3=initlibreconfig(true,alwaysnewstatus3))) {
						alwaysnewstatus3=false;
						LOGAR("initlibreconfig failed");
						continue;
						}
					LOGGER("initlibreconfig success %d\n",settings->data()->libreinit3);
					alwaysnewstatus3=false;
					}
				if(!settings->data()->sendtolibreview) {
					askforaccount=false;
					settings->data()->uselibre=false;
					libreviewrunning=false;
					LOGAR("end libreview thread, account id only");
					return;
					}
				if(sendlibre3viewdata(hasnewcurrent,nu)) {
					waitmin=5*60;
					}
				else {
					waitmin=15;
					}
				}
			if(settings->data()->haslibre2) {
				LOGAR("Libreview: haslibre2");
				if(!settings->data()->libreinit){
					if(!(settings->data()->libreinit=initlibreconfig(false,alwaysnewstatus))) {
						alwaysnewstatus=false;
						LOGAR("initlibreconfig failed");
						continue;
						}
					alwaysnewstatus=false;
					}
				if(!settings->data()->sendtolibreview) {
					askforaccount=false;
					settings->data()->uselibre=false;
					libreviewrunning=false;
					LOGAR("end libreview thread, account id only");
					return;
					}

				if(sendlibreviewdata(newcurrent,nu)) {
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
			 if((now-prevwake)<60*waitmin) {
				LOGGER("wakeaftermin(%d) too early\n",waitmin);
			 	return;
				}
			 }
		LOGGER("wakeaftermin(%d)\n",waitmin);
		librecondition.wakebackup(Backup::wakeall);
		}
	else {
		LOGAR("wakeaftermin: no thread running");
		librecondition.dobackup=Backup::wakeall;
		}
 }
void wakelibrecurrent() {
	if(libreviewrunning) {
		librecondition.wakebackup(Backup::wakestream);
		}
	else
		librecondition.dobackup=Backup::wakestream;
	}
void clearlibregegs() { //TODO clear after certain date/time
	settings->data()->libreinit=false;
	settings->data()->libreinit3=false;
	settings->data()->lastlibretime=0;
	settings->data()->startlibreview=0;
	settings->data()->startlibre3view=0;
	sensors->onallsensors([](SensorGlucoseData *sens) {
		auto *info=sens->getinfo();
		const auto end=sens->getStreamendhistory();
		info->clearLibreSend(end);
		info->putsensor=false;
		info->libreviewScan=0;
		info->sendsensorstart=false;
		info->libreviewnotsend=0;
		info->libreviewnotsendHistory=0;
		info->libreviewsendall=false;
	 	info->libreCurrentIter=0;
		info->scanoff=0;
//		info->startedwithStreamhistory=0;
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
	else {
		LOGAR("wakelibreview netwerkpresent==false");
		}
	}
/*
static bool started() {
		srand(time(nullptr));
	}
static bool isstarted=started();
*/
#include <thread>
#define srand(x)
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
std::string_view dRELEASE{};
std::string_view dMANUFACTURER{};
std::string_view dMODEL{};

std::string_view getjstring(JNIEnv *env,jstring jstr)  {
	size_t strlen= env->GetStringUTFLength( jstr);
	jint jstrlen = env->GetStringLength( jstr);
	char *strbuf=new char[strlen+1];
	env->GetStringUTFRegion( jstr, 0,jstrlen, strbuf);
	strbuf[strlen]='\0';
	return {strbuf,strlen};
	}
extern "C" JNIEXPORT void JNICALL fromjava(setDevice) (JNIEnv *env, jclass clazz, jstring jMANUFACTURER, jstring jMODEL, jstring jRELEASE) { 
	LOGAR("setDevice");
	if(jMANUFACTURER) {
		delete[] dMANUFACTURER.data(); //Ever called twice?
		dMANUFACTURER=getjstring(env,jMANUFACTURER);
		}
	if(jMODEL) {
		delete[] dMODEL.data();
		dMODEL=getjstring(env,jMODEL);
		}
	if(jRELEASE) {
		delete[] dRELEASE.data();
		dRELEASE=getjstring(env,jRELEASE);
		}
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


#else
extern void wakeaftermin(const int waitmin) ;
void wakeaftermin(const int waitmin) {
	}
#endif
