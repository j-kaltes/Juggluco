#pragma once
#ifndef WEAROS
#ifdef OLDEVERSENSE 
#include <thread>
#include "settings/settings.hpp"
#include "SensorGlucoseData.hpp"
inline void sendEverSenseold(const SensorGlucoseData *sens,int startpos,int endpos) {
	if(settings->data()->everSenseBroadcast.nr) {

extern void sendEverSenseoldthread(const SensorGlucoseData *sens,int startpos,int endpos);
		std::thread EverSensethread(sendEverSenseoldthread,sens,startpos,endpos);
		EverSensethread.detach();
		}
	}
#endif
#else
#define sendEverSenseold(sens, startpos, endpos) 
#endif
