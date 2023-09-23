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
/*      Fri Jan 27 12:37:55 CET 2023                                                 */


#include "SensorGlucoseData.h"
#include "jnihistory.h"
#include "nfcdata.h"
#include "datbackup.h"
extern bool saveSputnik_PG2(const jniHistory &hist,time_t nutime,int nuid,const nfcdata  *nfcptr, SensorGlucoseData &save) ;
bool saveSputnik_PG2(const jniHistory &hist,time_t nutime,int nuid,const nfcdata  *nfcptr, SensorGlucoseData &save) { 
     jint len=hist.size();
    LOGGER("saveSputnik_PG2 size=%d\n",len);
    if(len<=0)
    	return false;
decltype(auto)  first=hist.get(0);
    int fid=first.getId();
    int pos=int(round(fid/ save.getmininterval()));
    int start,topos;
    int lastpos=save.getlastpos(pos);
    if(pos>=lastpos) {
    	topos=pos;
	start=0;
	if(save.getstarthistory()<=0) {
		save.setstarthistory(topos);
		}
	}
    else {
    	topos=lastpos;
	start=lastpos-pos;
	}
int uselen=std::min(history::num,len);
int lastgood=topos-1;
int firstchanged=-1;
LOGGER("start=%d end=%d\n",start,uselen);
    for(int i=start;i<uselen;i++,topos++) {
	GlucoseValue gluv=hist.get(i);
	Glucose *item=save.getglucose(topos);
	if(!item->isStreamed()) {
		uint16_t gv=10*gluv.getValue();//Same unit as raw
		uint16_t id=gluv.getId();
		uint32_t was=nutime-(nuid-id)*60;

		uint16_t const 	rawel= nfcptr?nfcptr->gethistoryglucose(i):0;

		int idpos=int(round(id/(double)save.getmininterval()));
		if(topos!=idpos) {
			LOGGER("GLU: savehistory %d!=%d\n",topos,idpos);
			continue;
			}
		*item={.time=was,.id=id};
		item->glu[0]=rawel;item->glu[1]=gv;
		if(firstchanged<0)
			firstchanged=topos;
#ifndef NOLOG
		time_t tim=item->gettime();
		LOGGER("add %d %.1f %s",item->getid(),(float) item->getmgdL()/18.0f,ctime(&tim));
#endif
		}
	else {
#ifndef NOLOG
		time_t tim=item->gettime();	
		LOGGER("already streamed %d %.1f %s",item->getid(),(float)item->getmgdL()/18.0f,ctime(&tim));
#endif
		}
	if(!gluv.getQuality())
		lastgood=topos;
    	}
save.setendScanhistory(lastgood+1);
if(save.getStreamendhistory()) {
	if(firstchanged!=-1) {
		int maxint=backup->getupdatedata()->sendnr;
		save.setstarthistback(maxint,firstchanged);
		const auto first=std::max((uint16_t)firstchanged,save.getinfo()->startedwithStreamhistory);
		if(save.getinfo()->libreviewnotsendHistory>first) {
			LOGGER("libreviewnotsendHistory=%d\n",first);
			save.getinfo()->libreviewnotsendHistory=first; 
			}
		}
	}

return true;
}

extern std::vector<int> usedsensors;
extern void setusedsensors();
void setstartedwithStreamhistory() {
	if(SensorGlucoseData *sens=sensors->getSensorData()) {
		if(!sens->isLibre3()&&!sens->getinfo()->startedwithStreamhistory) {
			sens->getinfo()->startedwithStreamhistory=std::max(sens->getinfo()->endhistory,1);
			}
		}
	if(settings->data()->streamHistLib)  {
		return;
		}
	setusedsensors();
	for(int index:usedsensors) {
		  SensorGlucoseData *sens=sensors->getSensorData(index );
		  if(sens&&!sens->isLibre3()) {
			sens->getinfo()->startedwithStreamhistory=std::max(sens->getinfo()->endhistory,1);
			}
		}
	settings->data()->streamHistLib=true;
	}
void setendedwithStreamhistory() {
	if(!settings->data()->streamHistLib) 
		return;
	setusedsensors();
	for(int index:usedsensors) {
		  SensorGlucoseData *sens=sensors->getSensorData(index );
		  if(sens&&!sens->isLibre3()) {
			sens->getinfo()->startedwithStreamhistory=0;
			}
		}
	settings->data()->streamHistLib=false;
	}
extern bool addStreamHistory(const jniHistory &hist,time_t nutime,int nuid, SensorGlucoseData &save) ;
bool addStreamHistory(const jniHistory &hist,time_t nutime,int nuid, SensorGlucoseData &save) { 
	jint len=hist.size();
	LOGGER("addStreamHistory size=%d\n",len);
	if(len<32)  {
		return false;
		}
	int endhist=save.getAllendhistory();
	GlucoseValue   histvalue=hist.get(len-2);
	if(!histvalue.valid()) {
		LOGAR("not valid");
		return false;
		}
	int id=histvalue.getId();
	int pos=int(round(id/ save.getmininterval()));
	if(pos<endhist)  {
		LOGAR("old value");
		return false;
		}
	uint16_t gv=10*histvalue.getValue();//Same unit as raw
	time_t was=nutime-(nuid-id)*60;
	LOGGER("glucose id=%d %.1f (%d) %ld %s",id,gv/180.0f,gv,was,ctime(&was));
	save.saveel(pos,was,id, {0,gv,0x4000});
	save.setendStreamhistory(pos+1);
	return true;
	}
