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



int SensorGlucoseData::sendhistory(crypt_t *pass,int sock,int ind) {
	int histend=getendhistory(); 
	getinfo()->update[ind].changedhistorystart=false;
	int histstart=getinfo()->update[ind].histstart;
	if(histstart<histend) {
		if(histstart>0)
			histstart--;//TODO? Last in history contains only raw value, so should be overwritten
		if(!senddata(pass,sock,histstart*getelsize(),elstart(histstart),(histend+1-histstart)*getelsize(), histpath)) {
			LOGGER("GLU: senddata data.data failed\n");
			return 0;
			}
		if(!getinfo()->update[ind].changedhistorystart)
			getinfo()->update[ind].histstart=histend;
		did=true;
		return 2;
		}
	return 1;
	}
int SensorGlucoseData::updatescan(crypt_t *pass,int sock,int ind,int sensorindex,bool dotoch)  {
	bool did=false;
	constexpr const int startinfolen=offsetof(Info, pollcount);
	alignas(alignof(Info)) uint8_t infoptr[startinfolen];
	if(!isLibre3()) {
		switch(sendhistory(sock,ind)) {
			case 0:return 0;
			case 2: memcpy(infoptr,meminfo.data(),startinfolen);did=true;
			};
		}

	int scanend=getinfo()->scancount;
	int scanstart=getinfo()->update[ind].scanstart;
	if(scanend>scanstart) {
		if(!did)
			memcpy(infoptr,meminfo.data(),startinfolen);
		LOGGER("GLU: updatescan\n");
		if(const struct ScanData *startscans=scans.data()) {
			if(scanpath) {
				if(senddata(pass,sock,scanstart,startscans+scanstart,startscans+scanend,scanpath)) {
					if(const std::array<uint16_t,16> *starttrends=trends.data()) {
						if(trendspath) {
							if(!senddata(pass,sock,scanstart,starttrends+scanstart,starttrends+scanend,trendspath)) {
								LOGGER("GLU: senddata trends.dat failed");
								return 0;
								}
							getinfo()->update[ind].scanstart=scanend;
							}
						}
					}
				else  {
					LOGGER("GLU: senddata current.dat failed\n");
					return 0;
					}
				did=true;
				}
			}
		}
	if(!did) {
		if(getinfo()->update[ind].sendstreaming||dotoch) {
			memcpy(infoptr,meminfo.data(),startinfolen);
			goto dosendinfo;
			}
		}
	else {
		dosendinfo:
		LOGGER("GLU updatescan %s hist: %d-%d scan: %d-%d ", shortsensorname()->data(),histstart,histend,scanstart,scanend);
		 {
			std::vector<subdata> vect;
			vect.reserve(2);
			vect.push_back({infoptr,0,startinfolen});
	//		constexpr const int startdev=offsetof(Info, deviceaddress);
			constexpr const int startdev=offsetof(Info, streamingIsEnabled);
			constexpr const int devlen=offsetof(Info,libreviewscan)-startdev;
			static_assert((4+deviceaddresslen)==devlen);
			vect.push_back({((uint8_t*)meminfo.data())+startdev,startdev,devlen});
			 if(!senddata(pass,sock,vect, infopath)) {
				LOGGER("GLU: senddata info.data failed\n");
				return 0;
				 }
				  	
		         LOGGER("send device address %p %p %s\n", ((char*)meminfo.data())+startdev+4,getinfo()->deviceaddress,getinfo()->deviceaddress);
			}

		if(string_view state=getpreviousstate(sensordir);state.data()) {
			Readall<senddata_t> dat(state.data());
			if(dat) {
				LOGGER("GLU: %s\n",state.data());
				if(!senddata(pass,sock,0,dat.data(),dat.size(),std::string_view(state.data()+specstart,state.size()-specstart))) {
					LOGGER("GLU: senddata %s failed\n",state.data());
					return 0;
					}
				int sensdirlen= sensordir.size();
				pathconcat link(std::string_view(sensordir.data()+specstart,sensdirlen-specstart),"state.lnk");
				LOGGER("GLU: link=%s\n",link.data());
				sensdirlen++;
				if(!senddata(pass,sock,0,reinterpret_cast<const senddata_t *>(state.data())+sensdirlen,state.size()-sensdirlen,link)) {
					LOGGER("GLU: senddata %s failed\n",link.data());
					return 0;
					}
				}
			delete[] state.data();	
			}
		  if(getinfo()->update[ind].sendstreaming) {
			getinfo()->update[ind].sendstreaming=false;
			return 5;
			}
		return 1;
		  }
	return 2;
	}
/*
			if(histstart<histend) {
		        	const uint16_t historystartcmd=starthistoryupdate|sensorindex;
				 if(!senddata(pass,sock,vect, infopath,historystartcmd, reinterpret_cast<const uint8_t *>(&histstart),sizeof(histstart))) {
					LOGGER("GLU: senddata info.data failed\n");
					return 0;
					 }
				  }*/
