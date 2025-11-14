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

#include "config.h"
#if !defined(NOLOG )&&!defined(LOGCAT)
//#if 1 
#undef _GNU_SOURCE
#define _GNU_SOURCE
#include <dlfcn.h>
#include <algorithm>
#include <string.h>
#include <charconv>
#include <sys/uio.h>
#include <sys/time.h>

//#undef NOLOG
#include "logs.hpp"
#include "inout.hpp"
//bool mute=true;
/*
void setbuffer(char * buf) {
	syscall(511,buf);
LOGGER("bufaddress: %p: %s",buf,buf);

 } */
//extern pathconcat logbasedir; pathconcat logfile(logbasedir, LASTDIR ".log");


#ifndef NOTAPP
#include <android/log.h>
#define anlog(...)  __android_log_print(ANDROID_LOG_INFO,"logs",__VA_ARGS__)

extern int getlogfile();
int getlogfile() {
#pragma  message "basedir" BASEDIR
	static int handle=-1;
        if(handle!=-1) 
                return handle;
	if(sys_mkdir(BASEDIR,0700)!=0&&errno!=EEXIST) {
extern		pathconcat logbasedir;
extern          pathconcat logfile;
		if(logbasedir.data()) {
			
	//		pathconcat file(logbasedir,LASTDIR ".log");
			if(logfile.data())
				handle=sys_opener( logfile.data(),O_APPEND|O_CREAT|O_WRONLY, S_IRUSR |S_IWUSR);
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
         if(dup2(handle,STDERR_FILENO)<0) {
		LOGAR("dup2(handle,STDERR_FILENO) failed");
		}
         if(dup2(handle,STDOUT_FILENO)<0) {
		LOGAR("dup2(handle,STDOUT_FILENO) failed");
		}
	time_t tim=time(NULL);
	char *timestr=ctime(&tim);

	pid_t pid= syscall(SYS_getpid);
	constexpr const int maxbuf=120;
	char buf[maxbuf];	
	int buflen=std::snprintf(buf,maxbuf,"%.24s %d Start logging %d "
#ifdef NDEBUG
""
#else
"not"
#endif

" NDEBUG "

#ifdef NDK_DEBUG
""
#else
"not"
#endif
" NDK_DEBUG\n" ,timestr, (int)syscall(SYS_gettid),pid);

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

//ssize_t writev(int fd, const struct iovec *iov, int iovcnt);

static void logwritev(const struct iovec *iov, int iovcnt) {
	if(dolog) {
		static int handle=STDERR_FILENO;
	#ifndef NOTAPP
		 if(handle==STDERR_FILENO)
			handle=getlogfile();
	#endif
	       writev(handle,iov,iovcnt);
       	}
	}


int vloggert( const char *format, va_list args) {
	if(dolog) {
		if(logging::log	)
			return -1;
		logging now;
		constexpr const int size=4096;
		char str[size];
        struct timeval tv;
        gettimeofday(&tv,nullptr);
		int start=std::sprintf(str,"%lu.%03d %ld ",tv.tv_sec,(int)(tv.tv_usec/1000), (long)syscall(SYS_gettid));
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
	/*
int timetidlogprint(const char *format, ...) {
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
	} */
int logprint(const char *format, ...) {
	if(dolog) {
		if(logging::log	)
			return -1;
		logging now;
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

void flerror(const char* fmt, ...){
	if(logging::log	)
		return ;
	logging now;
	int waser=errno;
	const int maxbuf=100;
	char buf[maxbuf];
        va_list args;
        va_start(args, fmt);
	vsnprintf(buf,maxbuf, fmt, args);
	va_end(args);
	constexpr const int maxuitbuf=200;
	char uitbuf[maxuitbuf];
    struct timeval tv;
    gettimeofday(&tv,nullptr);
	int len=snprintf(uitbuf,maxuitbuf,"%lu.%03d %ld %s: %s\n",tv.tv_sec,(int)(tv.tv_usec/1000), syscall(SYS_gettid), buf,strerror(waser));
	logwriter(uitbuf,len);
	}

void LOGGERNO(const char *buf,int len,bool endl) {
	if(dolog)	 {
		if(logging::log	)
			return ;
		logging now;
        char timegitbuf[50];
        struct timeval tv;
        gettimeofday(&tv,nullptr);
        const size_t start=snprintf(timegitbuf,50,"%lu.%03d %ld ",tv.tv_sec,(int)(tv.tv_usec/1000), (long)syscall(SYS_gettid));
        static char nl[]{"\n"};
        const int arlen=2+endl; 
         const struct iovec ar[3] {{( void*)timegitbuf,start},{( void*)buf,(size_t)len},{( void*)nl,sizeof(nl)-1}};
        logwritev(ar,arlen);
        }
    }
    /*
void LOGGERNO(const char *buf,int len,bool endl) {
	if(dolog)	 {
		if(logging::log	)
			return ;
		logging now;
		if(len<1024) {
			char allbuf[len+50];
			int start=sprintf(allbuf,"%lu %ld ",time(nullptr), (long)syscall(SYS_gettid));
			memcpy(allbuf+start,buf,len);
			if(endl)
				allbuf[start+len++]='\n';
			logwriter(allbuf,start+len);
			}
		else {
			char allbuf[50];
			int start=sprintf(allbuf,"%lu %ld ",time(nullptr), (long)syscall(SYS_gettid));
			logwriter(allbuf,start);
			logwriter(buf,len);
			if(endl)
				logwriter("\n",1);
			}
		}
	}
*/


void LOGGERN(const char *buf,int len) {
	LOGGERNO(buf,len,true);
	}


#else

#ifndef INCLUDE_NR
#define INCLUDE_NR
#include <asm-generic/unistd.h> /*Headers in this order*/
#include <sys/syscall.h>
#endif

#include <unistd.h>
extern bool dolog;
bool dolog=false;
extern "C" pid_t getTid();
#endif
pid_t getTid() {
	return syscall(SYS_gettid);
	}
