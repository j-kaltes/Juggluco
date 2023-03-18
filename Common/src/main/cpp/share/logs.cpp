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
/*      Fri Jan 27 12:37:55 CET 2023                                                 */


#ifndef NOLOG 
//#if 1 
#undef _GNU_SOURCE
#define _GNU_SOURCE
#include <dlfcn.h>
#include <algorithm>
#include <string.h>
#include <charconv>
//#undef NOLOG
#include "logs.h"
#include "inout.h"
//bool mute=true;
/*
void setbuffer(char * buf) {
	syscall(511,buf);
LOGGER("bufaddress: %p: %s",buf,buf);

 } */
//extern pathconcat logbasedir; pathconcat logfile(logbasedir, LASTDIR ".log");

#ifndef NOTAPP
#ifndef LASTDIR
#define LASTDIR "trace"
#endif

#include <android/log.h>
#define anlog(...)  __android_log_print(ANDROID_LOG_INFO,"logs",__VA_ARGS__)

static int getlogfile() {
#pragma  message "basedir" BASEDIR
	int handle;

	if(sys_mkdir(BASEDIR,0700)!=0&&errno!=EEXIST) {
extern		pathconcat logbasedir;
		if(logbasedir.data()) {
			
			pathconcat file(logbasedir,LASTDIR ".log");
			if(file.data())
				handle=sys_opener( file.data(),O_APPEND|O_CREAT|O_WRONLY, S_IRUSR |S_IWUSR);
			else
				return STDERR_FILENO;
			}
		else {
			return STDERR_FILENO;
			}
		}
	else {
		#define LOGBASE BASEDIR "/logs"
		if(sys_mkdir(LOGBASE,0700)!=0&&errno!=EEXIST)
			return STDERR_FILENO;
		if(sys_mkdir(LOGBASE,0700)!=0&&errno!=EEXIST)
			return STDERR_FILENO;
		  const char logfile[]= LOGBASE "/" LASTDIR ".log";
         	handle=sys_opener( logfile,O_APPEND|O_CREAT|O_WRONLY, S_IRUSR |S_IWUSR);
	 	}
        // int handle=open( logfile,O_APPEND|O_CREAT|O_WRONLY, S_IRUSR |S_IWUSR);
	 if(handle<0) {
		  return STDERR_FILENO;
		  }
	 else

		  LOGGER("open succeeded\n");
/*	time_t tim=time(NULL);
	char *str=ctime(&tim);

*/
	#define startlog "Start logging: "
	constexpr int loglen=sizeof(startlog)-1;
	char buf[loglen+10]=startlog;
	pid_t pid= syscall(SYS_getpid);
#if  __NDK_MAJOR__ > 20 // 20 adds zero's
    std::to_chars_result res=std::to_chars(buf+loglen,buf+sizeof(buf),pid);
    int buflen=res.ptr-buf;
#else
int buflen=snprintf(buf+loglen,9,"%d",pid)+loglen;
#endif
	buf[buflen++]='\n';

       sys_write(handle, buf,buflen);
       return handle;
       }
       #endif

struct logging {   
thread_local	inline static   bool log=false;
	logging()  { log=true; };
	~logging() {
		log=false;
		}
	};
/*
int timestring(time_t tim,char *buf,int max) {
struct tm tms;
if(!localtime_r(&tim,&tms)) {
	lerror("localtime_r");
	return 0;
	}
return strftime(buf, max, "%Y-%m-%d-%H:%M:%S", &tms);
}
*/
extern bool dolog;
bool dolog=true;
void logwriter(const char *buf,const int len) {
	if(dolog) {
		static int handle=STDERR_FILENO;
	#ifndef NOTAPP
		 if(handle==STDERR_FILENO)
			handle=getlogfile();
	#endif
	       sys_write(handle,buf,len);
       	}
	}
int vloggert( const char *format, va_list args) {
	if(dolog) {
		if(logging::log	)
			return -1;
		logging now;
		constexpr const int size=4096;
		char str[size];
		int start=std::sprintf(str,"%lu %ld ",time(nullptr), syscall(SYS_gettid));
	       int ret= std::vsnprintf(str+start, size-start, format, args);
	       if(ret<=0) {
			return ret;
			}
		ret+=start;
		logwriter(str,std::min(ret,size));

		return ret;
		}
	else
		return 0;
        }
int logprint(const char *format, ...) {
	if(dolog)	 {
		constexpr const int size=4096;
		char str[size];
		va_list args;
		va_start(args, format);
	       int ret= vsnprintf(str, size, format, args);
		va_end(args); 
		logwriter(str,std::min(ret,size));
		return ret;
		}
	else
		return 0;
	}

int loggert(const char *format, ...) {
	if(dolog)	 {
		va_list args;
		va_start(args, format);
		int ret= vloggert(format,args);
		va_end(args); 
		return ret;
		}
	else
		return 0;
	}



void LOGGERNO(const char *buf,int len,bool endl) {
	if(dolog)	 {
		if(len<1024) {
			char allbuf[len+50];
			int start=sprintf(allbuf,"%lu %ld ",time(nullptr), syscall(SYS_gettid));
			memcpy(allbuf+start,buf,len);
			if(endl)
				allbuf[start+len++]='\n';
			logwriter(allbuf,start+len);
			}
		else {
			char allbuf[50];
			int start=sprintf(allbuf,"%lu %ld ",time(nullptr), syscall(SYS_gettid));
			logwriter(allbuf,start);
			logwriter(buf,len);
			if(endl)
				logwriter("\n",1);
			}
		}
	}



void LOGGERN(const char *buf,int len) {
	LOGGERNO(buf,len,true);
	}



#endif
