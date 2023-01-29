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
/*      Fri Jan 27 12:35:33 CET 2023                                                 */



bool savehistory(const AlgorithmResults *res,time_t nutime,const nfcdata *nfcptr,SensorGlucoseData &save) {
		decltype(auto) gl=res->currentglucose();
		int gid=gl.getId();
		decltype(auto) reshist=res->history();
extern bool saveSputnik_PG2(const jniHistory &hist,time_t nutime,int nuid,const nfcdata  *nfcptr, SensorGlucoseData &save) ;
		saveSputnik_PG2(reshist,nutime,gid, nfcptr, save); 
		if(newnu<(nutime-60*3)) {
//			lastscan=nullptr;
			LOGGER("%s: newnu=%zd nutime=%zd\n",serial.data(),newnu,nutime);
			senso->finished=1;
			void setusedsensors() ;
			setusedsensors();
			}
		else {
			senso->finished=0;
			hist->saveglucose(data,newnu,gid,gl.getValue(),res->trend(),res->RateOfChange()); 
			auto last= hist->getScandata();
//			lastscan=&last.back();

			newstate->makelink();
			state->removefile();
			delete state;
			state=newstate;
			return {res,&last.back()};
			}

		}
