#include "LibreHist.hpp"
#include "libreview.h"
/*
bool startedwithStreamhistory(const SensorGlucoseData *sensdata) {
	const bool userealhistory= (sensdata->getinfo()->libreviewnotsendHistory>= sensdata->getinfo()->startedwithStreamhistory)||!sensdata->pollcount();
	} */
void switchrealhistory(SensorGlucoseData *sensdata,bool torealhistory) {
	if(torealhistory) {
		if(!sensdata->getinfo()->realHistory) {
			LOGAR("switchrealhistory ON");
			const std::span<const ScanData> stream=sensdata->getPolldata() ;
			int histnum=4;
			int notsend=sensdata->getinfo()->libreviewnotsend;
			if(notsend>0) {
				auto *first=&stream.front();
				for(auto *last=&stream[notsend-1];last>=first;--last) {
					if(last->valid()) {
						constexpr int periodmin=15;
						constexpr int periodright=15/2;
						histnum=1+(last->id + periodright-1)/periodmin;
						LOGGER("last->valid id=%d\n",last->id);
						break;
						}
					}
				}

			sensdata->getinfo()->libreviewnotsendHistory=histnum;
			LOGGER("libreviewnotsend=%d\n",sensdata->getinfo()->libreviewnotsendHistory);
			sensdata->getinfo()->realHistory=true;
			}
		}
	else {
		if(sensdata->getinfo()->realHistory) {
			LOGAR("switchrealhistory OFF");
			int lastid=0;
			int notsend=sensdata->getinfo()->libreviewnotsendHistory;
			if(notsend>0) {
				for(int iter=notsend-1;iter>=0;iter--) {
					const Glucose *gl=sensdata->getglucose(iter);
					if(gl->valid()) {
						lastid=gl->getid()+8;
						break;
						}
					}
				}
			const std::span<const ScanData> stream=sensdata->getPolldata() ;
			auto *first=&stream.front();
			int nownotsend=0;
			for(auto *iter=&stream.back();iter>=first;--iter) {
				if(iter->valid()) {
					if(iter->getid()<lastid) {
						nownotsend=iter-first+1;
						}
					}
				}
			LOGGER("lastid=%d libreviewnotsend old=%d new=%d\n",lastid,notsend,nownotsend);
			sensdata->getinfo()->libreviewnotsend=nownotsend;
			sensdata->getinfo()->realHistory=false;
			}
		}
	}


LibreHist  libreRealHistory(SensorGlucoseData *sens,uint32_t starttime,uint32_t nu) { //test on start time;

	const uint32_t notsend=sens->getinfo()->libreviewnotsendHistory;
	/*
	if(sens->getinfo()->libreviewnotsendHistory==1104)
		sens->getinfo()->libreviewnotsendHistory=1036; */

		

	const uint32_t endpos=sens->getAllendhistory();
#ifndef NOLOG
	time_t tim=starttime;
	LOGGER("libreRealHistory %s getAllendhistory=%d libreviewnotsendHistory=%d from %s", sens->shortsensorname()->data(), endpos, notsend,ctime(&tim));
#endif
	auto iter=notsend;
	auto *info=sens->getinfo();
	if(!iter) {
		while(true) {
			if(iter>=endpos) {
				return {.notsendHistory=endpos};
				}
			const Glucose *gl=sens->getglucose(iter);

			if(gl->valid()&&!info->isLibreSend(iter)&&gl->gettime()>starttime)
				break;
			++iter;
			}
		}
	else {
		if(iter>=endpos) {
			return {.notsendHistory=endpos};
			}
		}
	int datalen=(int)endpos-iter;
	LibreHistEl *list=new LibreHistEl[datalen];

	int startit=iter;	
	int uitit=0;
	for(;iter<endpos;iter++) {
		const Glucose *gl=sens->getglucose(iter);
		if(gl->valid()&&!info->isLibreSend(iter)) {
			list[uitit++]={.ti=gl->gettime(),.mgdL=gl->getmgdL(),.id=(uint16_t)(gl->getid())};
			}
		}
	LOGGER("startit=%d nr=%d\n",startit,uitit);
	extern int64_t libreviewHistor(const sensorname_t *sensorid) ;
	const int64_t histor=libreviewHistor(sens->shortsensorname());
	return {.histor=histor,.list=list,.size=uitit,.starttime=sens->getstarttime(),.msec=0,.notsendHistory=endpos,.sendstart=sens->getinfo()->sendsensorstart };
	}

void setlibresend(SensorGlucoseData *sens,const LibreHist &hists) {
	const int len=hists.size;
	const LibreHistEl *lists=hists.list;;
	for(int i=0;i<len;i++) {
		int index=lists[i].id/15;
		sens->getinfo()->setLibreSend(index);
		}

	}
