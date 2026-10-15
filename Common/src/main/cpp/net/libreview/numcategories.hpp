#include "settings/settings.hpp"
#pragma once
inline float longWeight(uint32_t type) { 
	if(type>=settings->varcount())
		return 0.0f;
	if(settings->data()->librenums[type].kind==2)
		return 1.0f;
	return 0.0f;
	}	
inline float rapidWeight(uint32_t type) { 
	if(type>=settings->varcount())
		return 0.0f;
	if(settings->data()->librenums[type].kind==1)
		return 1.0f;
	return 0.0f;
	}	
inline float carboWeight(uint32_t type) { 
	if(type>=settings->varcount())
		return 0.0f;
	if(settings->data()->librenums[type].kind==3)
		return settings->data()->librenums[type].weight;
	return 0.0f;
	}

extern int notetype;
// notetype < 0 means "no note label configured": never match.
inline bool isNote(uint32_t type) {
	return notetype>=0&&(int)type==notetype;
	}
// Labels configured as Comments (kind 4) in the LibreView settings are
// uploaded as label-comment entries ("Label value"). This excludes the
// note label itself, which is sent with its full text instead.
inline bool isComment(uint32_t type) {
	if((int)type==notetype)
		return false;
	if(type>=settings->varcount())
		return false;
	return settings->data()->librenums[type].kind==4;
	}
