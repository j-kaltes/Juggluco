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
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
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
#include <string_view>
//#define loggert printf
#define VISIBLE __attribute__((__visibility__("default")))
extern const char *package;
extern int packagelen;

extern "C" int VISIBLE __android_log_print(int prio, const char* tag, const char* fmt, ...) __attribute__((__format__(printf, 3, 4)));
extern "C" int VISIBLE __android_log_write(int prio, const char* tag, const char* text) ;

#if !defined(NOLOG) && !defined(LOGCAT)
extern "C" int VISIBLE __android_log_write(int prio, const char* tag, const char* text) {
	return loggert("#%s: %s\n",tag,text);
	}
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
#endif 

#ifndef NOFORKHOOK
#define NOFORK
#endif
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

#endif

#ifdef NEWUS
extern "C" VISIBLE int  getTimeofday(struct timeval * tv, struct timezone *  tz) {
	LOGAR("gettimeofday");
//        int ret=gettimeofday(tv, tz);
        tv->tv_sec=0L;
        tv->tv_usec=0;
	if(tz) {
               tz->tz_minuteswest=0;
               tz->tz_dsttime=0;
	       }
        return 0;
        } 
#ifdef FINDHANDLE
struct pthread_arg {
        void *(*start_routine)(void *);
        void * origarg;
        };
#include <destruct.h>
static int copyfd=0;

static char *pmcom;
static int pmcomlen=0;


#include "destruct.h"
static bool endroutine=false;
static int giveuslibre2(const char *dirname) {
	int fp=open(dirname, O_DIRECTORY|O_RDONLY);
	if(fp<0) {
		lerror("open failed");
		return 3;
		}
	DIR *dir=fdopendir(fp);
	if(!dir) {
		close(fp);
		lerror("opendir");
		return 4;
		}
	destruct _des([dir,fp]{
			closedir(dir);
			close(fp);});
	struct dirent *ent;
	do {
	while((ent=readdir(dir))) {
		if(ent->d_type==DT_LNK)  {
			const char *name= ent->d_name;
			if(name[1]&&(name[0]>'3'||name[2])) {
				int maxbuf=256;
				char buf[maxbuf];
				int len;
				if((len=readlinkat(fp,name , buf,maxbuf))>0)  {
					buf[len]='\0';
					constexpr const char pipe[]="pipe:";
					constexpr const int pipelen=sizeof(pipe)-1;
					if(!memcmp(pipe,buf,pipelen)) {
						int get=atoi(name);
						copyfd=dup(get);
						close(get);
						int pipefd[2];
						int ret=sys_pipe2(pipefd,0);
						write(pipefd[1],pmcom,pmcomlen);
						close(pipefd[1]);
						LOGGER("%s: %s new %d %d %s\n",name,buf,pipefd[0],pipefd[1],pmcom); 
//						LOGGER("%s: %s\n",name,buf);
						return 0;

						}
					}
				}

			}
		}
		rewinddir(dir);
		}while(!endroutine);
	return 1;
}
/*
static int giveuslibre2(const char *dirname) {
	int fp=open(dirname, O_DIRECTORY|O_RDONLY);
	if(fp<0) {
		lerror("open failed");
		return 3;
		}
	destruct _des([fp]{
			close(fp);});
	int end=fp+40;
	LOGGER("giveuslibre2 %d\n",fp);
	do {
		for(int handle=fp+1;handle<end;handle++) {
			char name[10];
			sprintf(name,"%d",handle);
			int len;
			int maxbuf=50;
			char buf[maxbuf];
			if((len=readlinkat(fp,name , buf,maxbuf))>0)  {
				buf[len]='\0';
				constexpr const char pipe[]="pipe:";
				constexpr const int pipelen=sizeof(pipe)-1;
				if(!memcmp(pipe,buf,pipelen)) {
					copyfd=dup(handle);
					close(handle);
					int pipefd[2];
					int ret=sys_pipe2(pipefd,0);
					write(pipefd[1],pmcom,pmcomlen);
					close(pipefd[1]);
					LOGGER("%s: %s new %d %d %s\n",name,buf,pipefd[0],pipefd[1],pmcom); 
	//						LOGGER("%s: %s\n",name,buf);
					return 0;

					}
				}
			}

	 	LOGAR("NEXT");
		} while(!endroutine);
	return 1;
	}
 */



static void uslibre2fds() {
	pid_t pid=getpid();
	const char format[]="/proc/%d/fd";
	char procpid[50];
	snprintf(procpid,50,format,pid);
	giveuslibre2(procpid); 
	}
#include <sys/prctl.h>
#include <signal.h>
void ioreadyhandler(int sig) {
	pid_t pid= syscall(SYS_getpid);
	char buf[80];
	prctl(PR_GET_NAME, buf, 0, 0, 0);
	LOGGER("SIGCHLD pid=%d %s\n",pid,buf);
	}
void *pstart_routine( void * arg) {
	signal(SIGCHLD,ioreadyhandler);
        LOGAR("pstart_routine");
        pthread_arg *myarg=reinterpret_cast<pthread_arg *>(arg);
        void *res=myarg->start_routine(myarg->origarg);
	endroutine=true;
        LOGAR("after start_routine");
	if(copyfd)
		close(copyfd);
        delete myarg;
        return res ;
        }
#include <pthread.h>
#include <spawn.h>

  #include <sched.h>
extern "C" VISIBLE   int pthread_Create(pthread_t * thread, const pthread_attr_t * attr, void *(*start_routine)(void *), void * arg) {
extern std::string_view libdirname;
			const char pmcomstart[]=R"(package:)";
		constexpr const int pmstartlen=sizeof(pmcomstart)-1;
	int pathlen=libdirname.size();
	constexpr const char endname[]="/libcalibrate.so";
	constexpr const int endnamelen=sizeof(endname)-1;
	pmcom=new char[pmstartlen+pathlen+endnamelen+2];
	memcpy(pmcom,pmcomstart,pmstartlen);
	int pos=pmstartlen;
	memcpy(pmcom+pos,libdirname.data(),pathlen);
	pos+=pathlen;
	memcpy(pmcom+pos,endname,endnamelen);
	pos+=endnamelen;
	strcpy(pmcom+pos,"\n");
	pos++;
	pmcomlen=pos;
        LOGAR("pthread_create");
        auto *newarg=new pthread_arg{start_routine,arg};
        int res=pthread_create(thread, attr, pstart_routine,  newarg);
        LOGAR("after pthread_create");
        return res;
        }
extern "C" VISIBLE  int pthread_Detach(pthread_t thread) {
	LOGAR("pthread_detach");
	int res=  pthread_detach(thread);
	uslibre2fds();
	LOGAR("after pthread_detach");
	return res;
	}
#endif
#endif
#if 0
extern "C" VISIBLE    FILE *FOpen(const char *pathname, const char *mode) {
	#define urandom "/dev/urandom"
	if(!memcmp(urandom,pathname,sizeof( urandom))) {
	//	pathname="/proc/self/mountinfo";
		pathname="/dev/zero";
		} 
	FILE *res=fopen(pathname,mode);
	LOGGER("fopen(%s,%s)=%p\n",pathname,mode,res);
	return res;
	}
extern "C" VISIBLE 
int myexecve(const char *pathname, char *const argv[], char *const envp[]) {
	constexpr const int maxbuf=1024;
	char buf[maxbuf];
	int start=strlen(pathname);
	memcpy(buf,pathname,start);
	for(char *const *ptr=argv;*ptr;ptr++) {
		buf[start++]=' ';
		int len=strlen(*ptr);
		memcpy(buf+start,*ptr,len);
		start+=len;
		}
	buf[start]='\0';
	LOGGER("myexecve(%s)",buf);		
	return execve(pathname,argv,envp);
	}
extern "C" VISIBLE  int myfstatat(int dirfd, const char *pathname, struct stat *statbuf, int flags) {
	int res=fstatat(dirfd, pathname, statbuf, flags);
	LOGGER("fstatat(%d, %s, statbuf, %d)=%d\n",dirfd,pathname,flags,res);
	return res;
	}
extern "C" VISIBLE    VISIBLE  int Strncasecmp(const char *s1, const char *s2, size_t n) {
	int res=strncasecmp(s1, s2, n) ;
	logwriter(".",1);
//	LOGGER("strncasecmp(%s, %s, %zu)=%d\n",s1,s2,n,res);
	return res;
	}

extern "C" VISIBLE  int (*posix_spawn)(pid_t *pid, const char *path, const posix_spawn_file_actions_t *file_actions, const posix_spawnattr_t *attrp, char *const argv[], char *const envp[]);
int (*posix_spawn)(pid_t *pid, const char *path, const posix_spawn_file_actions_t *file_actions, const posix_spawnattr_t *attrp, char *const argv[], char *const envp[]);
extern "C" VISIBLE  int myposix_spawn(pid_t *pid, const char *path, const posix_spawn_file_actions_t *file_actions, const posix_spawnattr_t *attrp, char *const argv[], char *const envp[]) {
	int res=posix_spawn(pid, path, file_actions, attrp, argv,  envp);
	LOGGER("posix_spawn(%d,%s,,%s)=%d\n",*pid,path,*argv?*argv:"null",res);
	return res;
	}
extern "C" VISIBLE void *d1sym(void *handle, const char *symbol) {
	static const constexpr	std::pair<std::string_view,void *> funcs[]= {
		{"execve",(void*)myexecve},
		{"fopen",(void*)FOpen},
		{"fstatat", (void*) myfstatat},
//		{"open",(void*)Open},
//		{"opendir",(void*)myopendir},
//		{"read",(void*)Read},
//		{"signal",(void*)mysignal},
//		{"strcpy",(void*)mystrcpy},
//		{"strlen",(void*)mystrlen}
	};

	static const constexpr	std::pair<std::string_view,void *> * endfunc=funcs+std::size(funcs);

	const auto symlen=strlen(symbol);
	const std::pair<std::string_view,void *>  value={{symbol,symlen},nullptr};
        auto * res=std::lower_bound(funcs,endfunc,value,
[](const std::pair<std::string_view,void *> &one,const std::pair<std::string_view,void *> &two) {
		return one.first<two.first;
		});
	if(res==endfunc||memcmp(res->first.data(),symbol,symlen)) {
		auto fun=dlsym(handle,symbol);
		if(!fun||strcmp("posix_spawn",symbol)) {
			LOGGER("dlsym(%s)=%p original\n",symbol,fun);
			return fun;
			}
		LOGAR("dlsym(posix_spawn) give mine");
		*((void **)&posix_spawn)=fun;
		return (void *)myposix_spawn;
		}
	LOGGER("dlsym(%s) give mine\n",symbol);
	return res->second;
 	}

extern "C" VISIBLE     int Close(int fd) {
	int res= close(fd);
	LOGGER("close(%d)=%d\n",fd,res);
	return res;
	}
extern "C" VISIBLE  int FClose(FILE *stream) {
	int fn=fileno(stream);
	int res= fclose(stream);
	LOGGER("fclose(%d)=%d\n",fn,res);
	return res;
	}
#endif
