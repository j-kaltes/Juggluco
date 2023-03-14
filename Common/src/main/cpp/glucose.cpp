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


#include "settings/settings.h"
#include "datbackup.h"

void SensorGlucoseData::backhistory(int pos) {
	const int maxind=backup->getupdatedata()->sendnr;
	auto *updateptr=getinfo()->update;
	for(int i=0;i<maxind;i++) {
		updateptr[i].changedhistorystart=true;
		if(pos<updateptr[i].histstart) {
			updateptr[i].histstart=pos;
			}
		}

	}
void SensorGlucoseData::backstream(int pos) {
	const int maxind=backup->getupdatedata()->sendnr;
	auto *updateptr=getinfo()->update;
	for(int i=0;i<maxind;i++) {
		updateptr[i].changedstreamstart=true;
		if(pos<updateptr[i].streamstart) {
			LOGGER("%d: streamstart=%d\n",i,pos);
			updateptr[i].streamstart=pos;
			}
		}

	}


	
string_view getpreviousstate(string_view sbasedir ) ; // delete[] should be called on result



int SensorGlucoseData::sendhistoryinfo(crypt_t *pass,int sock,int sensorindex,uint32_t histstart,uint32_t endhistory) {
	constexpr const int endhistoryoff=offsetof(Info,endhistory); 
	constexpr const int lastLifeCountReceivedoff=offsetof(Info, lastLifeCountReceived); 
	constexpr const int u32len= sizeof(uint32_t);
	std::vector<subdata> vect;
	vect.reserve(2);
	vect.push_back({reinterpret_cast<const senddata_t*>( &endhistory),endhistoryoff,u32len});
	vect.push_back({reinterpret_cast<const senddata_t*>(&getinfo()->lastLifeCountReceived),lastLifeCountReceivedoff,u32len});
	const uint16_t historystartcmd=starthistoryupdate|sensorindex;
	 if(!senddata(pass,sock,vect, infopath,historystartcmd, reinterpret_cast<const uint8_t *>(&histstart),sizeof(histstart))) {
		LOGGER("GLU: senddata info.data failed\n");
		return 0;
		 }
	return 1;
	}
int SensorGlucoseData::sendhistory(crypt_t *pass,int sock,int ind,int sensorindex,bool sendinfo) {
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
		LOGGER("sendhistory %d-%d\n",histstart,histend);
		if(!getinfo()->update[ind].changedhistorystart) {
			getinfo()->update[ind].histstart=histend;
			if(sendinfo) {
				return sendhistoryinfo(pass,sock,sensorindex,histstart,histend);
				}
			return 1;
			}
		}
	return 2;
	}
int SensorGlucoseData::updateKAuth(crypt_t *pass,int sock,int ind)  {
	if(getinfo()->update[ind].sendKAuth) {
		const int off=getinfo()->haskAuth?offsetof(Info,kAuth):offsetof(Info,haskAuth);
		constexpr const int  endoff=offsetof(Info,haskAuth)+1;
		const int len=endoff-off;
		if(!senddata(pass,sock,off,reinterpret_cast<const senddata_t *>(getinfo())+off,len, infopath)) {
			LOGGER("GLU: updateKAuth failed\n");
			return 0;
			}
		LOGGER("GLU: updateKAuth send\n");
		getinfo()->update[ind].sendKAuth=false;
		return 1;
		}
	return 2;
	}
int SensorGlucoseData::updatescan(crypt_t *pass,int sock,int ind,int sensorindex,bool dotoch)  {
	bool did=false;
	constexpr const int startinfolen=offsetof(Info, pollcount);
	alignas(alignof(Info)) uint8_t infoptr[startinfolen];
	if(!isLibre3()) {
		switch(sendhistory(pass,sock,ind,sensorindex,false)) {
			case 0:return 0;
			case 1: memcpy(infoptr,meminfo.data(),startinfolen);did=true;
			};
		}
	else {
		if(!updateKAuth(pass,sock,ind))
			return 0;
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
		LOGGER("GLU updatescan %s  scan: %d-%d\n", shortsensorname()->data(),scanstart,scanend);
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
template <typename It,typename T>
It	find_last(It beg, It en,T el) {
		for(It it=en-1;it>=beg;it--)	
			if(*it==el)
				return it;
	return en;
	}
string_view getpreviousstate(string_view sbasedir ) { // delete[] should be called on result
	LOGGER("get previous state %s\n",sbasedir.data());
	constexpr const char start[]="/state.lnk";
	int baselen= sbasedir.length();
	if(sbasedir[baselen-1]=='/')
		baselen--;
	const int buflen=baselen+sizeof(start);
	char filename[buflen]; 
	memcpy(filename,&sbasedir[0],baselen);
	char *ptr=filename+baselen;
	memcpy(ptr,start,sizeof(start));
	struct stat st;
	if(stat(filename,&st)==0&&st.st_size!=0) {
		const string_view::size_type filesize=st.st_size;
		auto alllen=baselen+filesize+1;
		char *all=new char[alllen+1]; //*****
		char *startfile=all+baselen+1;
		if(readfile(filename,startfile,filesize)==filesize) {
		/*
			if(*startfile=='/') { //TODO remove after first use
				char *end=startfile+filesize;
				char *name=find_last(startfile+1,end,'/');
				if(name==end) {
					LOGGER("getprevious NO more / in %s\n",startfile);
					name=startfile;
					}
				name++;
				int endlen= end-name;
				memmove(startfile,name,endlen);
				alllen=baselen+endlen+1;
				} */
			memcpy(all,sbasedir.data(),baselen);
			all[baselen]='/';
			all[alllen]='\0';
			return {all,alllen};
			}
		else
			delete[] all;
		}
	return {nullptr,0};
	}

void Sensoren::removeoldstates()  {
/*
	uint32_t nu=time(nullptr);
	if(nu<settings->data()->unlinkstatestime) {
		return;
		} */
	for(int i=0;i<=last();i++) {
		if(SensorGlucoseData *hist=gethist(i))
			hist->removeoldstates(); 
		}
//	constexpr const uint32_t period=60*60*24;
//	settings->data()->unlinkstatestime=nu+period;
	}



void     sethistorystart(int sendindex,int newstart) {
extern	bool hasnotiset();
	if(!settings)
		return;
	if(!sensors)
		return;
	if(newstart<0) 
		return;
	if(SensorGlucoseData *hist=sensors->gethist(sendindex)) {
		LOGGER("sethistorystart(%d,%d)\n",sendindex,newstart);
		hist->backhistory(newstart);
		}
	else {
		LOGGER("sethistorystart no sensor %d\n",sendindex);
		}

	}

extern int getdeltaindex(float rate);
#define NOT_DETERMINED ""
int getdeltaindex(float rate) {
	if(rate>=3.5f)
                return 1;
	if(rate>=2.0f)
                return 2;
	if(rate>=1.0f)
                return 3;
	if(rate>-1.0f)
                return 4;
	if(rate>-2.0f)
                return 5;
	if(rate>-3.5f)
                return 6;
	if(isnan(rate))
		return 0;
	return 7;
	   }
#define NOT_DETERMINED ""
extern std::string_view getdeltanamefromindex(int index) ;
std::string_view getdeltanamefromindex(int index) {
	static constexpr const std::string_view deltanames[]={NOT_DETERMINED, "DoubleUp", "SingleUp","FortyFiveUp","Flat", "FortyFiveDown", "SingleDown", "DoubleDown",NOT_DETERMINED};
	return deltanames[index];
	}

extern std::string_view getdeltaname(float rate);
std::string_view getdeltaname(float rate) {
	return getdeltanamefromindex(getdeltaindex(rate));
	}

/*
std::string_view getdeltaname(float rate) {
	if(rate>=3.5f)
                return "DoubleUp";
	if(rate>=2.0f)
                return "SingleUp";
	if(rate>=1.0f)
                return "FortyFiveUp";
	if(rate>-1.0f)
                return "Flat";
	if(rate>-2.0f)
                return "FortyFiveDown";
	if(rate>-3.5f)
                return "SingleDown";
	if(isnan(rate))
		return NOT_DETERMINED;
	return "DoubleDown";
	   }

*/
