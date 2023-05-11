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



    #undef  _GNU_SOURCE
    #define _GNU_SOURCE
       #include <link.h>
#include <dlfcn.h>
  #include <math.h>
  #include <stdio.h>
  #include <stdlib.h>
         #include <unistd.h>
	   #include <string.h>
       #include <sys/types.h>
       #include <sys/stat.h>
       #include <fcntl.h>
       #ifdef __ANDROID__
#include <android/log.h>
#endif
#include <stdarg.h>
//#include <type_traits>

#include "logs.h"
#define logM(...)

#if __ANDROID_API__ >= 24
#define useddlvsym(handle,symbol,x) dlvsym(handle,symbol,x)
#define usedlsym(handle,name) useddlvsym(handle,name,nullptr)
#else
#define useddlvsym(handle,symbol,x) dlsym(handle,symbol)
#define usedlsym(handle,name) dlsym(handle,name)
#endif

//#define loggert printf
#define VISIBLE __attribute__((__visibility__("default")))
extern "C" int  VISIBLE dl_iterate_phdr( int (*callback) (struct dl_phdr_info *info, size_t size, void *data), void *data) ;
struct dl_data { 
	int (*callback) (struct dl_phdr_info *info, size_t size, void *data);
	void *data;
	};
/*
static int callbackhook(struct dl_phdr_info *info, size_t size, void *data) {
	struct dl_data *gegs= (struct dl_data *)data;
	if(info->dlpi_name)
		loggert("lib: %s\n",info->dlpi_name);
	else
		loggert("lib: null\n");

	if(!info->dlpi_name||!strstr(info->dlpi_name,"jnisub")) {
		return gegs->callback(info,size,data);
		}
	return 0;
	}
*/
/*79  newfstatat              man/ cs/  0x4f  int dfd           const char          struct stat         int flag            -                 -          
                                                              *filename           *statbuf            
                                                              struct                                                                                   */
/*int stat(const char *pathname, struct stat *statbuf) {
//	typedef   size_t (*stattype)(const char *pathname, struct stat *statbuf) ;
//	static stattype realstat=(stattype)useddlvsym(RTLD_NEXT, "stat",nullptr);
	int ret= sys_stat(pathname,statbuf);
//	int ret= realstat(pathname,statbuf);
	loggert("stat(%s)=%d\n",pathname,ret);
	return ret;
	}
extern "C" int VISIBLE dl_iterate_phdr( int (*callback) (struct dl_phdr_info *info, size_t size, void *data), void *data) {
	typedef int (*dl_iterate_phdrtype)( int (*callback) (struct dl_phdr_info *info, size_t size, void *data), void *data);
	static dl_iterate_phdrtype realdl_iterate_phdr=NULL;
	if(!realdl_iterate_phdr)
		realdl_iterate_phdr=(dl_iterate_phdrtype)useddlvsym(RTLD_NEXT, "dl_iterate_phdr",nullptr);
	
	if(!realdl_iterate_phdr) {
		loggert("dlsym(RTLD_NEXT, dl_iterate_phdr) failed\n");
		return -1;
		}
	struct dl_data *gegs=(struct dl_data *)malloc(sizeof(struct dl_data));
	gegs->callback=callback;
	gegs->data=data;
	return realdl_iterate_phdr(callbackhook,gegs);
	}
	
*/
	
extern const char *package;
extern int packagelen;
/*
VISIBLE extern "C" int execv(const char *pathname, char *const argv[])  {

	LOGGER("%d %d no execv(%s, {",getpid(),gettid(),pathname);
	for(char *const *ptr=argv;*ptr;ptr++ ) {
		LOGGER("%s,",*ptr);
		}
	LOGSTRING("})\n");
	#ifndef	USE_ECHO
//	const char package[]="package:/sdcard/libre/base.apk\n";
	sys_write(STDOUT_FILENO,package,packagelen);
//	puts("package:/sdcard/libre/base.apk");
//	fflush(stdout);
	sys_exit(0);
	return 0;
	#else
  	const char prog[]="/system/bin/echo";
        return execl(prog, prog, "package:/sdcard/libre/base.apk",NULL); //werkt
	#endif
	}
	
VISIBLE extern "C" double Pow(double x, double y) {
       double res= pow(x,y);
       logM("pow(%lf,%lf)=%lf\n",x,y,res);
       return res;
       }

VISIBLE extern "C" double Ldexp(double x, int exp) {
       double res= ldexp(x,exp);
       logM("ldexp(%lf,%d)=%lf\n",x,exp,res);
       return res;
       }
VISIBLE extern "C" long int Lround(double x) {
	long int res=lround(x);
	logM("lround(%lf)=%ld\n",x,res);
	return res;
	}
VISIBLE extern "C" char *Strdup(const char *s) {

#ifdef SAVE_MEMCPY
	filenr=0;
	#endif
	LOGGER("strdup(%s)\n",s);
	return strdup(s);
	}

VISIBLE extern "C" double Log(double x) {
	double res=log(x);
	logM("log(%lf)=%lf\n",x,res);
	return res;
	}
VISIBLE extern "C" int Memcmp(const void *s1, const void *s2, size_t n) {
#ifdef SHOW_MEMCMP
	static int iter=0;
	static size_t nwas=-1;
	static int grens=20;
	if(n==nwas) {
	  
	   if(

#ifdef SAVE_MEMCPY
	   filenr<10||
	   #endif
	   iter<2||(iter%grens)==0) {
		unsigned short *els=(unsigned short *)s1;
//		LOGGER("memcmp(%hx%hx%hx,%zd)\n",els[0],els[1],els[2],n);
		LOGGER("memcmp(%.*s,%zd)\n",(int)n,(const char *)s1,n);
		grens*=2;
		}
		iter++;
		}
	else {
		nwas=n;
		iter=1;
		grens=20;
		LOGGER("memcmp(%.*s,%zd)\n",(int)n,(const char *)s1,n);
		}
#endif
	return memcmp(s1,s2,n);
	}
*/
/*
extern "C"  FILE VISIBLE * fopen(const char *pathname, const char *mode) {
	typedef   FILE *(*fopentype)(const char *pathname, const char *mode) ;
	static  fopentype realfopen= (fopentype)useddlvsym(RTLD_NEXT, "fopen",nullptr);
	 auto ret=realfopen(pathname,mode);	
	LOGGER("fopen(%s,%s)=%p\n",pathname,mode,ret);
	return ret;
	}
	
extern "C" void VISIBLE syslog(int priority, const char *format, ...) {
        va_list args;
        va_start(args, format);
//	vsyslog(priority, format, args);
	loggert("syslog: ");
	vloggert(format,args);
        va_end(args);
	}


extern "C"     size_t VISIBLE fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
	typedef   size_t (*fwritetype)(const void *ptr, size_t size, size_t nmemb, FILE *stream) ;
	static fwritetype realfwrite= (fwritetype)useddlvsym(RTLD_NEXT, "fwrite",nullptr);
	size_t ret=realfwrite(ptr, size, nmemb, stream) ;
	LOGGER("fwrite(%zd,%zd,%p)=%zd\n",size, nmemb, stream,ret) ;
	return ret;
	}
	
VISIBLE extern "C" int open(const char* __path, int __flags, ...) {
#define realopen sys_opener
	if(O_RDONLY&& __flags) {
		int ret=realopen(__path,__flags);
		LOGGER("open(%s,%x)=%d\n", __path,  __flags, ret);
		return ret;
		}
        va_list args;
        va_start(args, __flags);
	mode_t mode=va_arg(args,int);
        va_end(args);
	int ret=realopen(__path,__flags,mode);
	LOGGER("open(%s,%x,%x)=%d\n", __path,  __flags, mode,ret);
	return ret;
	}
extern "C"     size_t VISIBLE fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
	typedef   size_t (*freadtype)(void *ptr, size_t size, size_t nmemb, FILE *stream) ;
	static freadtype realfread= (freadtype)useddlvsym(RTLD_NEXT, "fread",nullptr);
	size_t ret=realfread(ptr, size, nmemb, stream) ;
	constexpr const int pos=20;
	char *chp=((char *)ptr)+pos;
	char tmp=*chp;
	*chp='\0';
	LOGGER("fread(%s,%zd,%zd,%p)=%zd\n",(char *)ptr, size, nmemb, stream,ret) ;
	*chp=tmp;
	return ret;
	}

extern "C"       ssize_t VISIBLE read(int fd, void *buf, size_t count) {
	auto ret=sys_read(fd,buf,count);
	constexpr const int pos=20;
	char *chp=((char *)buf)+pos;
	char tmp=*chp;
	*chp='\0';
	LOGGER("read(%d,%s,%zd)=%zd\n",fd,(char *)buf,count,ret);
	*chp=tmp;
	return ret;
	}

void syslogger( const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
    	vsyslog(LOG_USER, fmt, args);
        va_end(args);
	}
*/
#if __ANDROID_API__ >= 24
 extern "C" void  VISIBLE *dlsym(void *handle, const char *symbol) {
  	void *ret=useddlvsym(handle,symbol,nullptr);
	loggert( "dlsym(%p,%s)=%p\n",handle,symbol,ret );
	return ret;
  	}
#endif
	
	/*
  VISIBLE extern "C" void *dlsym(void *handle, const char *symbol) {
  static int fp=-1;
  static int iter=0;
  if(fp==-1) {
  	  const char start[]="start\n";
	  fp=sys_opener( "/sdcard/newlog.txt",O_APPEND|O_CREAT|O_WRONLY, S_IRUSR |S_IWUSR);
		sys_write(fp,start,sizeof(start)-1);
		}
	iter++;
  	void *ret=useddlvsym(handle,symbol,nullptr);
	loggert( "dlsym(%p,%s)=%p\n",handle,symbol,ret );
	
	if(fp!=-1) {
		const char nl[]="\n";
		sys_write(fp,symbol,strlen(symbol));
		sys_write(fp,nl,sizeof(nl)-1);
		}
	return ret;
  	}
VISIBLE extern "C" void *Memcpy(void *dest, const void *src, size_t n) {
#ifdef SAVE_MEMCPY
  	if(filenr<10) { 
//	if(filenr<10) {
		#define BUGSTART FILEDIR "data"
		char base[256]=BUGSTART;
		sprintf(base+sizeof(BUGSTART)-1,"%d",filenr++);
		int file=creat(base,0644);

//		int file=    open(filename,O_WRONLY|O_CREAT, S_IRUSR | S_IWUSR );
		LOGGER("save to %s: ",base);
		if(file<0) {
			LOGGER("Can't create %s\n",base);
				}
		else {
			write(file,src,n);
			close(file);	
			}
		}
	LOGGER("memcpy(,%.*s,%zd)\n",(int)n,src,n);
#endif
	return memcpy(dest,src,n);
	}
*/
//TODO uncomment

extern "C" int VISIBLE __android_log_print(int prio, const char* tag, const char* fmt, ...) __attribute__((__format__(printf, 3, 4)));
extern "C" int VISIBLE __android_log_write(int prio, const char* tag, const char* text) ;

extern "C" int VISIBLE __android_log_write(int prio, const char* tag, const char* text) {
	return loggert("#%s: %s\n",tag,text);
	}
//int get= __android_log_print(ANDROID_LOG_INFO,"Glucose","Hooks startup");
extern "C" int VISIBLE __android_log_print(int prio, const char* tag, const char* fmt, ...) {
        va_list args;
	int res=loggert("#%s: ",tag);
        va_start(args, fmt);
	res+=vloggert(fmt,args);
	loggert("\n");
//	int res=  __android_log_vprint(prio, tag, fmt, args);
        va_end(args);
	return res;
	}


/*
VISIBLE extern "C" pid_t waitpid(pid_t pid, int *wstatus, int options) {
  	static waitpid_t wp=(waitpid_t)useddlvsym(RTLD_NEXT,"waitpid",nullptr);
	pid_t ret=wp(pid,wstatus,options);
	LOGGER("waitpid(%d,%d,%d)=%d\n",pid,wstatus?*wstatus:-1,options,ret);
	return ret;
	}
*/
#define NOFORK
#ifdef NOFORK
static constexpr const pid_t childpid=9815123;
#endif
extern "C" pid_t VISIBLE Waitpid(pid_t pid, int *wstatus, int options) {
#ifdef NOFORK
	if(pid==childpid)  { 
		
		LOGSTRING("waitpid\n");
		if(wstatus)
			*wstatus=0;
		return childpid;
	}
	else 
#endif
	{
	
	//	typedef pid_t (*waitpid_t)(pid_t pid, int *wstatus, int options);
	//	static waitpid_t wp=(waitpid_t)useddlvsym(RTLD_NEXT,"waitpid",nullptr);
		pid_t ret=sys_wait4(pid,wstatus,options,nullptr);
		LOGGER("waitpid(%d,%d,%d)=%d\n",pid,wstatus?*wstatus:-1,options,ret);
		return ret;
		}
	}

extern "C" pid_t VISIBLE waitpid(pid_t pid, int *wstatus, int options) {
	return Waitpid(pid,wstatus,options);
	}
/*(
VISIBLE extern "C" pid_t  fork(void) {
	typedef pid_t (*fork_t)(void);
  	static fork_t fo=(fork_t)useddlvsym(RTLD_NEXT,"fork",nullptr);
	pid_t pid=fo();
	LOGGER("fork()=%d\n",pid);
	return pid;
	}
*/

//static int *checkpipe=nullptr;
#ifdef NOFORK
extern "C" pid_t  VISIBLE fork(void) {
	LOGSTRING("nepfork()\n");
	return childpid;
	}
extern "C" pid_t  VISIBLE Fork(void) {
	LOGSTRING("nepfork()\n");
	return childpid;
	}

extern "C" int  VISIBLE Pipe(int pipefd[2]) {
//	typedef  int (*pipe_t)(int [2]); static pipe_t pi=(pipe_t)useddlvsym(RTLD_NEXT,"pipe",nullptr); int  ret=pi(pipefd);
	int ret=sys_pipe2(pipefd,0);
	LOGGER("pipe({%d,%d})=%d\n",pipefd[0],pipefd[1],ret);
        sys_write(pipefd[1],package,packagelen);
	return ret;	
	}
extern "C" int  VISIBLE pipe(int pipefd[2]) {
	return  Pipe(pipefd);
	}


/*
VISIBLE extern "C" int dup2(int oldfd, int newfd) {
	typedef	 int (*dup2_t)(int oldfd, int newfd);
  	static dup2_t du=(dup2_t)useddlvsym(RTLD_NEXT,"dup2",nullptr);
	int ret=du(oldfd,newfd);
	LOGGER("dup2(%d,%d)=%d\n",oldfd,newfd,ret);
	return ret;
	}
	*/
#endif
