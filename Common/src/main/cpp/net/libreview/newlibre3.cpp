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

#include <cctype>
#include <cstdint>
#include <string>
#include <cstdio>
#include <cstring>
#include <ctime>
#include  <inttypes.h>
#include <memory>
#include "sensoren.h"
#include "settings/settings.h"
#include "SensorGlucoseData.h"
#include "share/serial.h"
#include "libreview.h"
#ifdef LIBRENUMBERS
#include "librenumbers.h"
#endif


extern char *localestr;

#define addarray(x) memcpy(uitptr,x,sizeof(x)-1);uitptr+=(sizeof(x)-1);
constexpr const char startheader[]=R"({
  "DeviceData": {
    "deviceSettings": {
      "factoryConfig": {
        "UOM": ")";
constexpr const char afterunit[]=R"("
      },
      "firmwareVersion": "3.3.0",
      "miscellaneous": {
        "selectedLanguage": ")";
constexpr const char afterlang[]=R"(",
        "valueGlucoseTargetRangeLowInMgPerDl": )";
constexpr const char afterlowtarget[]=R"(,
        "valueGlucoseTargetRangeHighInMgPerDl": )";
constexpr const char	afterhightarget[]=R"(,
        "selectedTimeFormat": ")";//24hr 
constexpr const char afterhours[]=R"(hr",
        "selectedCarbType": "grams of carbs"
      },
      "timestamp": ")";//2022-10-18T11:57:31.795+02:00
constexpr const char aftertimestamp[]=R"("
    },
    "header": {
      "device": {
        "hardwareDescriptor": ")";//RMX2202 
constexpr const char afterhardware[]=R"(",
        "hardwareName": ")";//realme
constexpr const char afterhardname[]=R"(",
        "modelName": "com.freestylelibre3.app.de",
        "osType": "Android",
        "osVersion": ")";//31
constexpr const char afterosversion[]=R"(",
        "uniqueIdentifier": ")";//2977dec2-492a-11ea-9702-0242ac110002 
constexpr const char afteridentifier[]=R"("
      }
    },
    "measurementLog": {
      "capabilities": [
        "scheduledContinuousGlucose",
        "unscheduledContinuousGlucose",
        "currentGlucose",
        "insulin",
        "food",
        "generic-com.abbottdiabetescare.informatics.exercise",
        "generic-com.abbottdiabetescare.informatics.ondemandalarm.low",
        "generic-com.abbottdiabetescare.informatics.ondemandalarm.high",
        "generic-com.abbottdiabetescare.informatics.ondemandalarm.projectedlow",
        "generic-com.abbottdiabetescare.informatics.ondemandalarm.projectedhigh",
        "generic-com.abbottdiabetescare.informatics.customnote",
        "generic-com.abbottdiabetescare.informatics.sensorstart",
        "generic-com.abbottdiabetescare.informatics.sensorEnd",
        "generic-com.abbottdiabetescare.informatics.isfGlucoseAlarm",
        "generic-com.abbottdiabetescare.informatics.alarmSetting",
        "generic-com.abbottdiabetescare.informatics.bleDisconnect",
        "generic-com.abbottdiabetescare.informatics.bleReconnect",
        "generic-com.abbottdiabetescare.informatics.error"
      ],
      "currentGlucoseEntries": [)";
static constexpr const char 	afterentries[]=R"(],
      "foodEntries": [)";
static constexpr const char 	afterFoodentries[]=R"(],
      "genericEntries": [)";
static constexpr const char aftergeneric[]=R"(],
      "insulinEntries": [)";
static constexpr const char afterinsulin[]=R"(],
      "scheduledContinuousGlucoseEntries": [)";


int historyformat(char *buf,const char *factime,const int64_t num,const char *stamp,float value) {
constexpr const char entry[]=R"(
	{
          "extendedProperties": {
            "canMerge": true,
            "isFirstAfterTimeChange": false,
            "factoryTimestamp": "%s"
          },
          "recordNumber": %)" PRId64 R"(,
          "timestamp": "%s",
          "valueInMgPerDl": %.1f
        })";
return sprintf(buf,entry, factime,num,stamp, value);
}




#ifdef NDEBUG
#define EXTRATIME 5
#else
#define EXTRATIME 0
#endif
static_assert(sizeof(long long)==sizeof(int64_t), "");
static int histel3_2str(const Glucose *el,const int64_t histor,char *buf) {
	char gmttime[25+EXTRATIME];
	auto tim=el->gettime();
	int mil=getmmsec();
	TdatestringGMT(tim,mil,gmttime);
	char timestr[30+EXTRATIME];
	Tdatestringlocal(tim,mil,timestr);
	return historyformat(buf,gmttime,mkhistrecord(histor,el->getid()),timestr,(float)el->getmgdL());
	}



template<class T>
int64_t libreviewHistorAlg(const T *sensorid,int start, int shift) {
        int64_t  uit = 0;
        const char *str=sensorid->data();
        const int len= sensorid->size();
        for(int it = start; it <len ; it++) {
           uit = (uit << 5) | unalf(str[it]);
           }
        return  (uit << shift);
    }
template<class T>
int64_t libreviewHistor3(const T *sensorid) {
        return libreviewHistorAlg(sensorid,0,19) ;
        }

int64_t libreviewHistor(const sensorname_t *sensorid) {
        return libreviewHistorAlg(sensorid,1,14) ;
    }
int64_t libreviewSensorNameID(SensorGlucoseData *sens) {
	auto name=sens->showsensorname();
	return libreviewHistor3(&name);
	}
static constexpr const char onecurrent[]=R"(
        {
          "extendedProperties": {
            "trendArrow": "%s",
            "isActionable": true,
            "isViewed": %s,
            "factoryTimestamp": "%s",
            "isFirstAfterTimeChange": false
          },
          "recordNumber": %)" PRId64 R"(,
          "timestamp": "%s",
          "valueInMgPerDl": %.1f
        })";

static constexpr const char onesensorstart[]=R"(
        {
          "type": "com.abbottdiabetescare.informatics.sensorstart",
          "extendedProperties": {
            "factoryTimestamp": "%s",
            "puckGen": 0,
            "wearDuration": 21600,
            "warmupTime": 60,
            "productType": 4
          },
          "recordNumber": %)" PRId64 R"(,
          "timestamp": "%s"
        })";
int addsensorstart(char *buf,uint32_t nu,int mil,SensorGlucoseData *sens)  {
	if(!sens)
		return 0;
	if(sens->getinfo()->sendsensorstart)
		return 0;
	int newmil=mil+20;
	if(mil<500&&newmil>=500)
		++nu;
	auto polls=sens->getPolldata();
	time_t start=0;
	for(int i=0;i<polls.size();i++) {
		const ScanData &el=polls[i];
		if(el.current(i)) {
			start=el.gettime();
			break;
			}
		}
	if(!start)
		return 0;
	char startGMT[25+EXTRATIME];
	TdatestringGMT(start,startGMT);
	char nowtimestr[30+EXTRATIME];
	Tdatestringlocal(nu,newmil,nowtimestr);
	int64_t histor=libreviewSensorNameID(sens);
	return sprintf(buf,onesensorstart,startGMT,histor,nowtimestr);
	}


extern bool getisviewed(time_t wastime) ;
static int addcurrent(char *buf,int64_t histor,const ScanData *el,bool &viewed) {
	char gmttime[25+EXTRATIME];
	auto tim=el->gettime();
	int mil=getmmsec();
	TdatestringGMT(tim,mil,gmttime);
	char timestr[30+EXTRATIME];
	Tdatestringlocal(tim,mil,timestr);
	int64_t recordnum=mkhistrecord(histor,el->getid());
	viewed=getisviewed(tim) ;
	const char *isviewed=viewed?"true":"false";
	return sprintf(buf,onecurrent, trendName[el->tr],isviewed,gmttime,recordnum,timestr,(float)el->getmgdL());
	}
//uint16_t lastLifeCountReceived;



static int sendallcurrent(uint32_t nu,SensorGlucoseData *sens,char *buf,int *lastsend,bool &viewed) { 
	if(!sens)
		return 0;
	 int start=sens->getinfo()->libreviewScan;
	int pollstart= sens->getinfo()->pollstart;
//	 int ends=sens->getinfo()->lastLifeCountReceived;
	int		ends=sens->pollcount()-1;
	LOGGER("%s pollstart=%d libreviewScan=%d ends=%d\n",sens->showsensorname().data(),pollstart,start,ends);
//	if(ends==0) {
//		}
	*lastsend=ends;	
	if(start<pollstart)
		start=pollstart;
	LOGGER("start=%d ends=%d\n",start,ends);
	if(start>ends)
		return 0;
	const ScanData *startstream=sens->beginpolls();
	for(int i=ends;i>=start;i--) {
		const ScanData *el=startstream+i;
		if(el->current(i)) {
			int64_t histor=libreviewSensorNameID(sens);
			int wrote=addcurrent(buf,histor,el,viewed);
			return wrote;
			}
		}
	return 0;
	}
int allhistory(SensorGlucoseData *sens,char *buf,uint32_t *lasttime,int *nextnum) { //TODO remember endpos and set notsend=endpos+1
	const int endpos=sens->getinfo()->lastHistoricLifeCountReceivedPos;
	const int notsend=sens->getinfo()->libreviewnotsend;
	LOGGER("lastHistoricLifeCountReceivedPos=%d libreviewnotsend=%d\n",endpos, notsend);
	if(endpos<notsend)
		return 0;
	int pos=0;
	int64_t histor=libreviewSensorNameID(sens);
	int iter=notsend;
	for(;iter<=endpos;iter++) {
		const Glucose *gl=sens->getglucose(iter);
		if(gl->valid()&&gl->gettime()>*lasttime)
			break;
		
		}
	for(;iter<=endpos;iter++) {
		const Glucose *gl=sens->getglucose(iter);
		if(gl->valid()) {
			pos+=histel3_2str(gl,histor,buf+pos);
			*lasttime=gl->gettime();
			buf[pos++]=',';
			}
		}
	if(pos)
		*nextnum=iter;
	return pos;
	}
static constexpr const std::string_view beforeUserToken=R"(],
      "unscheduledContinuousGlucoseEntries": []
    }
  },
  "UserToken": ")";
static constexpr const std::string_view  afterUserToken=R"(",
  "Domain": "Libreview",
  "GatewayType": "FSLibreLink3.Android"
}
)";

bool notsendall2(const int index) {
	return index>=settings->data()->startlibreview;
	}
extern Sensoren *sensors;
bool sendnumbers3() {
	return settings->data()->sendnumbers&&settings->data()->libre3nums;
	}
bool sendlibre3viewdata(bool hasnewcurrent,uint32_t nu) {
	int startsensor=settings->data()->startlibre3view;
	int lastsensor=sensors->last();
	int inhistory=0;
	LOGGER("Start sendlibre3viewdata startlibre3view=%d lastsensor=%d\n",startsensor,lastsensor);
	if(lastsensor<startsensor)	{
		LOGGER("lastsensor(%d)<startsensor(%d)\n",lastsensor,startsensor);
		if(!sendnumbers3())
			return true;
		}
	for(;startsensor<=lastsensor;startsensor++) {
		SensorGlucoseData *sensdata=sensors->getSensorData(startsensor);
		if(sensdata->isLibre3()) 
			break;
		else {
			if(notsendall2(startsensor)) {
				settings->data()->haslibre2=true; 
				} 
			}
		}
	
	int lastlibre3=-1;
	const uint32_t oldtimer=nu-Sensoren::sensorageseconds;
	for(int i=startsensor;i<=lastsensor;i++) {
		SensorGlucoseData *sensdata=sensors->getSensorData(i);
		if(sensdata->isLibre3()) {
			auto add=(sensdata->getScanendhistory()-sensdata->getinfo()->libreviewnotsend);
			lastlibre3=i;
			if(add<=0) {
				if(sensdata->getstarttime()<oldtimer) {
					settings->data()->startlibre3view=i+1;
					sensdata->getinfo()->libreviewsendall=true;
					lastlibre3=-1;
					}
				}
			else  {
				inhistory+=add;
				}
			}
		else {
			if(notsendall2(i)) {
				settings->data()->haslibre2=true; 
				} 
			}
		}
	if(startsensor>lastlibre3) {
		settings->data()->haslibre3= sendnumbers3();
		if(!settings->data()->haslibre3)
			return true;
		}
#ifdef LIBRENUMBERS
Numbers<libre3> numbers; //TODO: test on Libre3nums?
int bytesnumbers=numbers.spaceneeded();
LOGGER("numbers.spaceneeded()=%d\n",bytesnumbers);

#else
constexpr const int  bytesnumbers=0;
#endif
	lastsensor=lastlibre3;
LOGGER("startsensor=%d lastsensor=%d\n",startsensor,lastsensor);
SensorGlucoseData *lastsensdata=(lastsensor<0)?nullptr:sensors->getSensorData(lastsensor); //TODO later?

	if(bytesnumbers==0&&inhistory<=0) {
		if(!hasnewcurrent)
			return true;
		}
if(lastsensor>=0) {
	if(!putwhenneeded(true,lastsensdata)) 
		return false;
	}
int incurrent=1;
constexpr int onecurrentsize=420;
constexpr int onehistorysize=342;
const int usertokenlen= settings->data()->tokensize3;

constexpr int restsize=2300;

int totalsize= bytesnumbers+restsize+usertokenlen+incurrent*onecurrentsize+inhistory*onehistorysize+1;

LOGGER("inhistory=%d incurrent=%d totalsize=%d\n",inhistory,incurrent,totalsize);
char *buf= new(std::nothrow)  char[totalsize];
if(!buf) {
	LOGSTRING("libreview3: new failed\n");
	return false;
	}
std::unique_ptr<char[]> deleter(buf);

char *uitptr=buf;
addarray(startheader);
addstrview(uitptr,settings->getunitlabel());
addarray(afterunit);
addstrview(uitptr,localestr);
addarray(afterlang);
uitptr+=sprintf(uitptr,"%d",(int)round(settings->targetlow()/10.0));
addarray(afterlowtarget);
uitptr+=sprintf(uitptr,"%d",(int)round(settings->targethigh()/10.0));
addarray(afterhightarget);
uitptr+=sprintf(uitptr,"%d",hour24clock?24:12);
addarray(afterhours);
int mil=getmmsec();
uitptr+=Tdatestringlocal(nu,mil,uitptr);
addarray(aftertimestamp);
addstrview(uitptr,dMODEL);
addarray(afterhardware);
addstrview(uitptr,dMANUFACTURER);
addarray(afterhardname);
addstrview(uitptr,dRELEASE);
addarray(afterosversion);
const std::array<char,36> &deviceID=settings->data()->libre3viewDeviceID;
addstrcont(uitptr,deviceID);
addarray(afteridentifier);
constexpr const char newline[]="\n     ";
int laststreamsend;
bool viewed=false;
int wrotecurrent=(hasnewcurrent||inhistory>0)?sendallcurrent(nu,lastsensdata,uitptr,&laststreamsend,viewed):0;
if(wrotecurrent>0) {
	uitptr+=wrotecurrent;
	addarray(newline);
	}
addarray(afterentries);
int wrotefood=numbers.writeallfood(uitptr);
if(wrotefood>0) {
	uitptr+=wrotefood;
	addarray(newline);
	}
addarray(afterFoodentries);

const int notelen=numbers.writeallnotes(uitptr);
uitptr+=notelen;

int addsensorlen;
if(wrotecurrent>0) {
	addsensorlen=addsensorstart(uitptr,nu,mil,lastsensdata);
	uitptr+=addsensorlen;
	}
else
	addsensorlen=0;
if(addsensorlen>0)  {
	addarray(newline);
	}
else {
	if(notelen>0)  {
        --uitptr;
          addarray(newline);
          }
	}
addarray(aftergeneric);
int wroteinsulin=numbers.writeallinsulin(uitptr);
if(wroteinsulin>0) {
	uitptr+=wroteinsulin;
	addarray(newline);
	}
addarray(afterinsulin);
uint32_t lasthisttime=0;
int nextnum=0,lastwrote=-1;
if(inhistory>0) { 
	for(int i=startsensor;i<=lastsensor;i++) {
		SensorGlucoseData *sens=sensors->getSensorData(i);
		if(sens->isLibre3()) {
			int wrote=allhistory(sens,uitptr,&lasthisttime,&nextnum);
			if(wrote) {
				uitptr+=wrote;
				lastwrote=i;
				}
			}
		}
	if(uitptr[-1]==',') {
		--uitptr;
		addarray(newline);
		lasthisttime+=5*60;
		}
	}
addstrview(uitptr,beforeUserToken);
memcpy(uitptr, settings->data()->libreviewUserToken3,usertokenlen);
uitptr+=usertokenlen;
addstrview(uitptr,afterUserToken);
int uitlen=uitptr-buf;
LOGGER("END sendlibre3viewdata uitlen=%d\n",uitlen);
buf[uitlen]='\n';
logwriter(buf,uitlen+1);
int nextsensor=(wrotecurrent>0)?lastsensor:lastwrote;
if(nextsensor<0) {
	LOGSTRING("nextsensor<0\n");
	if(!numbers.didsend())
		return true;
	}
#ifndef NOREALSEND
bool datasend=libresendmeasurements(true,buf,uitlen);
if(datasend) {
	LOGSTRING("libresendmeasurements libre3 success\n");
	numbers.onSuccess();
//	int nextsensor=(wrotecurrent>0)?lastsensor:lastwrote;
	if(nextsensor>=0) {
		settings->data()->startlibre3view=nextsensor;
		if(lastwrote>=0) {
			SensorGlucoseData *sens=sensors->getSensorData(lastwrote);
			sens->getinfo()->libreviewnotsend=nextnum;
			}
		if(wrotecurrent>0) {
			extern time_t nexttimeviewed;
			extern int betweenviews;
			nexttimeviewed=nu+betweenviews;
			lastsensdata->getinfo()->libreviewScan=laststreamsend+1;
			lastsensdata->getinfo()->sendsensorstart=true;
			}
		}
	return true;
	} 
LOGSTRING("libresendmeasurements libre3 failed\n");
#else
LOGSTRING("libresendmeasurements libre3 NOREALSEND\n");
#endif
return false;
	}
#ifdef MAIN
struct LibreHistEl {
	uint32_t ti;
	uint16_t mgdL;
	uint16_t id;
	};

int main() {
//Sensorid        nr      UnixTime        YYYY-mm-dd-HH:MM:SS     TZ      Min     mg/dL

//XX09ECXALLU     28      1666086031      2022-10-18-11:40:31     2       135     90
std::string sens="09ECXALLU";
int64_t histor=libreviewHistor3(&sens);
char buf[1024];
LibreHistEl el{1666086031,90,135};
const  char *endstr=libre3histel(&el,histor,buf);
puts(buf);
}
#endif
#endif
