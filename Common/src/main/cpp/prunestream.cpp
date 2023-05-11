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
/*      Fri Jan 27 12:35:35 CET 2023                                                 */


#include "SensorGlucoseData.h"


void SensorGlucoseData::prunestream() { 
	struct ScanData *pollsdata=polls.data();
	struct ScanData *scansdata=scans.data();
	int len=pollcount();
	int scount=scancount();
	LOGGER("prunestream %d\n",len);
	for(int i=len-2;i>=0;i--) {
		if(pollsdata[i].t>pollsdata[i+1].t) {
			time_t tim=pollsdata[i].t;
			LOGGER("%d's wrong stream wrong: id=%d %.1f %s",i,pollsdata[i].id,pollsdata[i].g/18.0f,ctime(&tim));
			if(i<scount&&scansdata[i].t==0&&scansdata[i].id==0) {
				if((i==(scount-1)||pollsdata[i].t<scansdata[i+1].t)&&(i==0||pollsdata[i].t>scansdata[i-1].t)) {
					LOGSTRING("move to scans\n");
					scansdata[i]=pollsdata[i];
					}
				}
		      pollsdata[i]={.t=pollsdata[i+1].t-45,.id=pollsdata[i+1].id-1,.g=0};
		       LOGGER("changed to id=%d %.1f %s",pollsdata[i].id,pollsdata[i].g/18.0f,ctime(&tim));
			}
		}
	}
void SensorGlucoseData::prunescans() { 
	struct ScanData *scansdata=scans.data();
	int iter=scancount()-1;
	for(;iter>=0&&!scansdata[iter].t;--iter) 
		;
	getinfo()->scancount=iter+1;
	for(int i=iter-1;i>=0;i--) {
		if(!scansdata[i].t) {
			scansdata[i].t=scansdata[i+1].t-1;
			scansdata[i].id=scansdata[i+1].id-1;
			scansdata[i].g=0;
			}
		}
	}

void SensorGlucoseData::prunescansonly() { 
	struct ScanData *scansdata=scans.data();
	struct ScanData *pollsdata=polls.data();
	int scountmin=scancount()-1;
	for(int i=scountmin;i>=0;i--) {
		if(!scansdata[i].t) {
			if(pollsdata[i].valid()&&(i==scountmin||pollsdata[i].t<scansdata[i+1].t)&&(i==0||pollsdata[i].t>scansdata[i-1].t)) {
				LOGSTRING("move to scans\n");
				scansdata[i]=pollsdata[i];
				}
			else {
				if(i==scountmin) {
					--scountmin;
					}
				else {
					scansdata[i].t=scansdata[i+1].t-1;
					scansdata[i].id=scansdata[i+1].id-1;
					scansdata[i].g=0;
					}
				}
		}
	}
   getinfo()->scancount=scountmin+1;
	}
void SensorGlucoseData::prunedata() {
	if(scancount()>0) {
		if(pollcount()) {
			prunestream();
			prunescans();
			}
		else {
				if(polls.size()>=scans.size()) 
					prunescansonly();
				else
					prunescans();
			}
		}
	}
