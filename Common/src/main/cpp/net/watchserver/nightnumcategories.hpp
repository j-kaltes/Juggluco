#include "settings/settings.hpp"
#pragma once
inline float longNightWeight(uint32_t type) { 
	if(type>=settings->varcount())
		return 0.0f;
	if(settings->data()->Nightnums[type].kind==2)
		return 1.0f;
	return 0.0f;
	}	
inline float rapidNightWeight(uint32_t type) { 
	if(type>=settings->varcount())
		return 0.0f;
	if(settings->data()->Nightnums[type].kind==1)
		return 1.0f;
	return 0.0f;
	}	
inline float carboNightWeight(int type) { 
	if(type>=settings->varcount())
		return 0.0f;
	if(settings->data()->Nightnums[type].kind==3)
		return settings->data()->Nightnums[type].weight;
	return 0.0f;
	}
inline bool isNightNote(int type) { 
	if(type>=settings->varcount())
		return false;
	return settings->data()->Nightnums[type].kind==4;
	}

