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


#ifndef LOGS_H
#define LOGS_H
#include "config.h"
//#define LOGGER(...)  { fprintf(errorlogfile,__VA_ARGS__); fflush(errorlogfile); }

#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#ifndef _WIN32
#undef _GNU_SOURCE
#define _GNU_SOURCE
#ifndef INCLUDE_NR
#define INCLUDE_NR
#include <asm-generic/unistd.h> /*Headers in this order*/
#include <sys/syscall.h>
#endif
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
#endif

#endif

//#define sys_creat(...) syscall(__NR_creat,__VA_ARGS__)
#ifdef __cplusplus
//#include <string_view>
//#define BASEDIR "/sdcard/glucodata"
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
#define LOGGER  loggert

#ifdef NOLOG
#define donothing do { if (0) ((void)0); } while (0)
#define logger(x)   donothing
static constexpr const inline int returnzero() { return 0;}
#define  loggert( ...)   returnzero()
#define  vloggert(...)  returnzero()
//constexpr const inline int loggert(...)  { return 0;}

//constexpr const inline int vloggert(...)  { return 0;}

#define logwriter( ...) donothing
#define logprint( ...) donothing

#define LOGGERN(buf, len) donothing
#ifdef NOTAPP
inline void lerror(const char *str) {
	int waser=errno;
	fprintf(stderr,"%s: %s\n",(char *)str,strerror(waser));
	}
inline void flerror(const char* fmt, ...){
#ifndef NOLOG
	int waser=errno;
#endif
	const int maxbuf=160;
	char buf[maxbuf];
        va_list args;
        va_start(args, fmt);
	vsnprintf(buf,maxbuf, fmt, args);
	va_end(args);
	LOGGER("%s: %s\n",buf,strerror(waser));
	}
#else
#define lerror( ...) donothing
#define flerror( ...) donothing
#endif




#else
#define LOGGER  loggert
#define logger(x)   LOGGER("%s\n",x)

void LOGGERN(const char *buf,int len) ;
inline void lerror(const char *str) {
	int waser=errno;
	LOGGER("%s: %s\n",(char *)str,strerror(waser));
	}

#ifdef __cplusplus
extern "C" {
#endif
inline void flerror(const char* fmt, ...) __attribute__((format(printf, 1, 2)))   ;
inline void flerror(const char* fmt, ...){
	int waser=errno;
	const int maxbuf=80;
	char buf[maxbuf];
        va_list args;
        va_start(args, fmt);
	vsnprintf(buf,maxbuf, fmt, args);
	va_end(args);
	LOGGER("%s: %s\n",buf,strerror(waser));
	}
#ifdef __cplusplus
};
#endif

extern void logwriter(const char *buf,const int len);






#endif
#endif
