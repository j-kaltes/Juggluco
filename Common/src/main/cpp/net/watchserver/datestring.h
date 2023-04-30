#pragma once
#include <time.h>
#include <cstdio>
#include <string>
inline int Tdatestring(time_t tim,char *buf) {
         struct tm tmbuf;
        int seczone=timegm(localtime_r(&tim,&tmbuf)) - tim;
	int m=seczone/60;
	int h=m/60;	
	int minleft=m%60;
	return sprintf(buf,R"(%d-%02d-%02dT%02d:%02d:%02d.000%+03d:%02d)",tmbuf.tm_year+1900,tmbuf.tm_mon+1,tmbuf.tm_mday,tmbuf.tm_hour,tmbuf.tm_min,tmbuf.tm_sec,h,minleft);
	}


inline double getdelta(float change) {
	static constexpr const double deltatimes=5.0;
	 return isnan(change)?0:change*deltatimes;
	 }

#include "sha1.hpp"
inline std::string sha1encode(const char *secret, int len) {
	std::string strsecret(secret,len);
	SHA1 sh1;
	sh1.update(strsecret);
	return sh1.final();
	}
