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

#pragma once

#include "config.h"
//#define LOGGER(...)  { fprintf(errorlogfile,__VA_ARGS__); fflush(errorlogfile); }

#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <android/log.h>

#ifndef _WIN32
#undef _GNU_SOURCE
#define _GNU_SOURCE 1
#ifndef INCLUDE_NR
	#define INCLUDE_NR
	#include <asm-generic/unistd.h> /*Headers in this order*/
	#include <sys/syscall.h>
#endif // INCLUDE_NR
#include <unistd.h>

#define sys_opener(...) syscall(__NR_openat,AT_FDCWD, __VA_ARGS__)
#define sys_close(...) syscall(__NR_close,__VA_ARGS__)
#define sys_write(handle,data,siz) syscall(__NR_write,handle,data,siz)
#define sys_read(...) syscall(__NR_read,__VA_ARGS__)
#define sys_exit(...) syscall(__NR_exit,__VA_ARGS__)
#define sys_mkdir(...) syscall(__NR_mkdirat,AT_FDCWD, __VA_ARGS__)
#define sys_wait4(...) syscall(__NR_wait4, __VA_ARGS__)
#define sys_pipe2(...) syscall(__NR_pipe2, __VA_ARGS__)
#ifdef __NR_newfstatat
	#define sys_stat(f,st) syscall(__NR_newfstatat, AT_FDCWD,f,st,0)
#else
	#ifdef __NR_stat
	#define sys_stat(f,st) syscall(__NR_stat, f,st) wrong
	#endif
#endif // __NR_newfstatat

#endif //__WIN32

//#define sys_creat(...) syscall(__NR_creat,__VA_ARGS__)
#ifdef __cplusplus
extern "C" {
#else
typedef char bool;
#endif
//extern bool mute;
typedef int (*opent)(const char* __path, int __flags, ...);
#ifndef NOLOG
__attribute__((__visibility__("default"))) extern int loggert(const char *format, ...)  __attribute__((format(printf, 1, 2)));

extern  int vloggert( const char *format, va_list args)   __attribute__ ((__format__ (__printf__, 1, 0))); ;
//extern  void flerror(const char* fmt, ...);
extern  int logprint(const char *format, ...) ;
#endif
#ifdef __cplusplus
}
#endif
extern 
#ifdef __cplusplus
"C" 
#endif
pid_t getTid();
#ifdef LOGCAT

#define donothing do { if (0) ((void)0); } while (0)
#define loggert(...) __android_log_print(ANDROID_LOG_INFO,"cpp",__VA_ARGS__)

#define LOGGER(...) loggert(__VA_ARGS__)
#define LOGSTRING(pformat)  __android_log_write(ANDROID_LOG_INFO,"cpp",pformat)
#define LOGAR(x) LOGGER("%s\n",x)

#define logger(x)   LOGAR(x)

//#define logwriter(buf, len)  __android_log_print(ANDROID_LOG_INFO,"write","%.*s",len>120?120:len,buf)
#define logwriter(buf, len)  donothing

#define logprint( ...)  __android_log_print(ANDROID_LOG_INFO,"print",__VA_ARGS__)


#define LOGGERN(buf, len)  __android_log_print(ANDROID_LOG_INFO,"logn","%.*s",len>120?120:len,buf)

#define LOGGERNO(buf, len,x) __android_log_print(ANDROID_LOG_INFO,"logno","%.*s",len>120?120:len,buf)



inline void lerror(const char *str) {
	int waser=errno;
	__android_log_print(ANDROID_LOG_INFO,"error","%s: %s\n",(char *)str,strerror(waser));
	}
inline void flerror(const char* fmt, ...){
	int waser=errno;
	const int maxbuf=160;
	char buf[maxbuf];
        va_list args;
        va_start(args, fmt);
	vsnprintf(buf,maxbuf, fmt, args);
	va_end(args);
	LOGGER("%s: %s\n",buf,strerror(waser));
	}


#else
#ifdef __cplusplus
#define LOGGER(pformat,...) logprint("%lu %d " pformat ,::time(nullptr), getTid(), __VA_ARGS__ )
#define LOGSTRING(pformat) logprint("%lu %d " pformat ,::time(nullptr), getTid())
#else
#define LOGGER(pformat,...) logprint("%lu %d " pformat ,time(NULL), getTid(), __VA_ARGS__ )
#define LOGSTRING(pformat) logprint("%lu %d " pformat ,time(NULL), getTid())
#endif


#ifdef NOLOG
#define donothing do { if (0) ((void)0); } while (0)
#define logger(x)   donothing
static 
#ifdef __cplusplus
constexpr 
#endif
const inline int returnzero() { return 0;}
#define  loggert( ...)   returnzero()
#define  vloggert(...)  returnzero()

#define logwriter( ...) donothing
#define logprint( ...) donothing

#define LOGGERN(buf, len) donothing
#define LOGGERNO(buf, len,x) donothing
#ifdef NOTAPP
inline void lerror(const char *str) {
	int waser=errno;
	fprintf(stderr,"%s: %s\n",(char *)str,strerror(waser));
	}
inline void flerror(const char* fmt, ...){
	}
#else //NOTAPP
#define lerror( ...) donothing
#define flerror( ...) donothing
#endif //NOTAPP



	#define LOGAR(arg)   


#else //!NOLOG

#ifdef __cplusplus
extern "C" {
#endif
void LOGGERN(const char *buf,int len) ;
void LOGGERNO(const char *buf,int len,bool endl) ;


inline void lerror(const char *str) {
	int waser=errno;
#ifdef NOTAPP
	fprintf(stderr,"%s: %s\n",(char *)str,strerror(waser));
#else
	LOGGER("%s: %s\n",(char *)str,strerror(waser));
#endif
	}
#ifdef NOTAPP
inline void flerror(const char* fmt, ...){
	int waser=errno;
	const int maxbuf=160;
	char buf[maxbuf];
        va_list args;
        va_start(args, fmt);
	vsnprintf(buf,maxbuf, fmt, args);
	va_end(args);
	LOGGER("%s: %s\n",buf,strerror(waser));
	}
#else //!NOTAPP
void flerror(const char* fmt, ...) __attribute__((format(printf, 1, 2)))   ;
#endif
#ifdef __cplusplus
};
#endif

extern void logwriter(const char *buf,const int len);



#ifdef __cplusplus
template <class T, size_t N>
constexpr void LOGAR(const T (&array)[N])  {
	LOGGERN(array,N-1);
	}
#else
#define LOGAR(arg)   LOGGERN(arg,sizeof(arg)-1)
#endif

#define logger(x)   LOGAR(x)
#endif //NOLOG
#endif //LOGCAT
