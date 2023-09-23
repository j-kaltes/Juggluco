#pragma once
#include "sensoren.h"
extern Sensoren *sensors;


inline const SensorGlucoseData *getStreamSensor(int &sensorid) {
	for(;;sensorid--) {
		if(sensorid<0)  {
			return nullptr;
		}
		if(const SensorGlucoseData *sens=sensors->getSensorData(sensorid)) {
			if(sens->pollcount()>0)
				return sens;
		}
	}
}
template <class Funtype>
bool getitems(char *&outiter,const int  datnr,uint32_t newer,uint32_t older,bool alldata, int interval,Funtype writeitem)  {
	LOGGER("getitems %d\n",datnr);
	int sensorid=sensors->last();
	uint32_t timenext=older;
	int datit=0;
	{
		STARTDATA:
		const SensorGlucoseData *sens=getStreamSensor(sensorid);;
		if(!sens) {
			if(datit)
				return true;
			return false;
		}
		std::span<const ScanData> gdata=sens->getPolldata();
		const ScanData *iter=&gdata.end()[-1];
		--sensorid;
		if(const SensorGlucoseData *sens2=getStreamSensor(sensorid)) {
			std::span<const ScanData> gdata2=sens2->getPolldata();
			const ScanData *last=&gdata2.end()[-1];
			if(last->t>iter->t) {
				sens=sens2;
				iter=last;
				gdata=gdata2;
				}
			}
		const char *sensorname= sens->shortsensorname()->data();
		LOGGER("getStreamSensor(%d) %s pollcount=%d\n",sensorid+1,sensorname,sens->pollcount());
		const ScanData *first=&gdata.begin()[0];

		const time_t starttime= sens->getstarttime();

		for(;datit<datnr;datit++,iter--) {
			while(true) {
				if(iter<first) {
					if(alldata) {
						--sensorid;
						goto STARTDATA;
					}
					if(datit)
						return true;
					return false;
				}

				if(iter->valid(iter-first)) {
					if(iter->t<newer) {
						if(datit)
							return true;
						return false;
						}
					if(iter->t<timenext)
						break;
				}
				else
					LOGSTRING("invalid\n");
				--iter;

			}
			timenext=iter->t-interval;
			outiter=writeitem(outiter,datit,iter,sensorname,starttime);
		}

	}
	return true;
}
