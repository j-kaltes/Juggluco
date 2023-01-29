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
extern bool saveSputnik_PG2(const jniHistory &hist,time_t nutime,int nuid,const nfcdata  *nfcptr, SensorGlucoseData &save) ;
bool saveSputnik_PG2(const jniHistory &hist,time_t nutime,int nuid,const nfcdata  *nfcptr, SensorGlucoseData &save) { 
    logger("Save");
     jint len=hist.size();
    LOGGER("size=%d\n",len);
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
    for(int i=start;i<uselen;i++,topos++) {
    	GlucoseValue gluv=hist.get(i);
	uint16_t gv=10*gluv.getValue();//Same unit as raw
	int id=gluv.getId();
	time_t was=nutime-(nuid-id)*60;
	uint16_t const 	rawel= nfcptr?nfcptr->gethistoryglucose(i):0;
	save.saveel(topos,was,id, {rawel,gv});
	if(!gluv.getQuality())
		lastgood=topos;
    	}
/*if(save.getid(topos-1)&&uselen>1) {
	if(int prev=save.glucose(topos-2,1))
		save.glucose(topos-1,1)=prev-nfcptr->gethistoryglucose(uselen-2)+nfcptr->gethistoryglucose(uselen-1); //If last glucose value is missing create one.

	}
	*/

save.setendhistory(lastgood+1);
return true;
}

//const ScanData *lastscan=nullptr;
/*
bool savehistory(const AlgorithmResults *res,time_t nutime,const nfcdata *nfcptr,SensorGlucoseData &save) {
	decltype(auto) gl=res->currentglucose();
	int gid=gl.getId();
	save.saveglucose(nutime,gid,gl.getValue(),res->trend(),res->RateOfChange()); 
//	auto last= save.getScandata();
//	lastscan=&last.back();
	decltype(auto) hist=res->history();
	return saveSputnik_PG2(hist,nutime,gid, nfcptr, save); 
}
*/


