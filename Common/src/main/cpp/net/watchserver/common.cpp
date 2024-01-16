#include <stdio.h>
#include <time.h>
#include <charconv>
#include <stdint.h>
#include <system_error>

#include "SensorGlucoseData.h"
#include "gltype.h"
#include "common.h"
#include "logs.h"
extern double getdelta(float change);
extern std::string_view getdeltaname(float rate);
int mkv3streamid(char *outiter,const sensorname_t *name,int num) { 
LOGGER("sensorname=%s\n",name->data());
const uint16_t *gets=reinterpret_cast<const uint16_t*>(name->data());
int ch2=name->back();
int len=sprintf(outiter,"%04x%04x-%04x-%04x-%02xee-eeeeeeeeeeee",(int)(gets[4]),(int)(gets[3]),(int)(gets[2]),(int)(gets[1]),ch2);

outiter+=24;
if(auto [ptr,ec]=std::to_chars(outiter,outiter+10,num);ec != std::errc()) {
	LOGGER("tochar failed: %s\n",std::make_error_code(ec).message().c_str());
	} 
//puts(buf);
return len;
//0fffffff-ffff-ffff-ffff-17500fffffff"
//c149bacfb000007f
}
int mkv1streamid(char *outiter,const sensorname_t *name,int num) { 
LOGGER("sensorname=%s\n",name->data());
const uint16_t *gets=reinterpret_cast<const uint16_t*>(name->data());
int ch2=name->back();
int len=sprintf(outiter,"%04x%04x%04x%04x%02xeeeeee",(int)(gets[4]),(int)(gets[3]),(int)(gets[2]),(int)(gets[1]),ch2);

outiter+=18;
if(auto [ptr,ec]=std::to_chars(outiter,outiter+10,num);ec != std::errc()) {
	LOGGER("tochar failed: %s\n",std::make_error_code(ec).message().c_str());
	} 
//puts(buf);

return len;
//0fffffff-ffff-ffff-ffff-17500fffffff"
//c149bacfb000007f
}
char * writev3entry(char *outin,const ScanData *val, const sensorname_t *sensorname,bool server) {
	char *outptr=outin;
	addar(outptr,R"({"app":"Juggluco","device":")");
	memcpy(outptr,sensorname->data(),sensorname->size());
	outptr+=sensorname->size();
	addar(outptr,R"(","date":)");
	const time_t tim=val->gettime();
	if(auto [ptr,ec]=std::to_chars(outptr,outptr+12,tim);ec == std::errc()) {
		outptr=ptr;
	 	}
	else {
		LOGGER("tochar failed: %s\n",std::make_error_code(ec).message().c_str());
		}
	addar(outptr,R"(000,"sgv":)");
	if(auto [ptr,ec]=std::to_chars(outptr,outptr+12,val->getmgdL());ec == std::errc()) {
		outptr=ptr;
	 	}
	else {
		LOGGER("tochar failed: %s\n",std::make_error_code(ec).message().c_str());
		}
	addar(outptr,R"(,"delta":)");
	 double delta= getdelta(val->ch);

#if __NDK_MAJOR__ >= 26
	if(auto [ptr,ec]=std::to_chars(outptr,outptr+20,delta,std::chars_format::fixed,3);ec == std::errc()) {
		outptr=ptr;
	 	}
	else {
		LOGGER("tochar failed: %s\n",std::make_error_code(ec).message().c_str());
		}
#else
	outptr+=sprintf(outptr,"%.3f",delta);
#endif
	addar(outptr,R"(,"direction":")");
	 std::string_view name=getdeltaname(val->ch);
	 addstrview(outptr,name);
	addar(outptr,R"(","type":"sgv","utcOffset":0,"identifier":")");
	outptr+=mkv3streamid(outptr,sensorname,val->id);
	if(server) {
		addar(outptr,R"(","created_at":")");
		struct tm tmbuf;
		gmtime_r(&tim, &tmbuf);
		outptr+=sprintf(outptr,R"(%04d-%02d-%02dT%02d:%02d:%02d)",tmbuf.tm_year+1900,tmbuf.tm_mon+1,tmbuf.tm_mday, tmbuf.tm_hour, tmbuf.tm_min,tmbuf.tm_sec);

		addar(outptr,R"(.000Z","srvModified":)");
		if(auto [ptr,ec]=std::to_chars(outptr,outptr+12,tim);ec == std::errc()) {
			outptr=ptr;
			}
		else {
			LOGGER("tochar failed: %s\n",std::make_error_code(ec).message().c_str());
			}

		addar(outptr,R"(000,"srvCreated":)");
		if(auto [ptr,ec]=std::to_chars(outptr,outptr+12,tim);ec == std::errc()) {
			outptr=ptr;
			}
		else {
			LOGGER("tochar failed: %s\n",std::make_error_code(ec).message().c_str());
			}
		addar(outptr,R"(000})");
		}
	else {
		addar(outptr,R"(","_id":")");
		outptr+=mkv1streamid(outptr,sensorname,val->id);
		addar(outptr,R"("})");
		}
	return outptr;
	}
