
#define SYSGOOD 1
/*#ifndef NDEBUG
#define TESTDEBUG 1
#endif */
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
/*      Fri Jan 27 12:35:35 CET 2023                                                 */



/*
arch	syscallNR	return	arg0	arg1	arg2	arg3	arg4	arg5
arm	r7		r0	r0	r1	r2	r3	r4	r5
arm64	x8		x0	x0	x1	x2	x3	x4	x5
x86	eax		eax	ebx	ecx	edx	esi	edi	ebp
x86_64	rax		rax	rdi	rsi	rdx	r10	r8	r9
*/
//#define CHANGESTATUS
#include <sys/mman.h>

#include "config.h"
#ifdef CARRY_LIBS 
#define USEDIN 1
#endif

#define VISIBLE __attribute__((__visibility__("default")))
#if defined(__aarch64__) || defined(__arm__) || defined(__x86_64__) ||defined(__i386__)
#undef _GNU_SOURCE
#define _GNU_SOURCE
#include <iostream>

#include <sys/uio.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include  <sys/user.h>
#include <string_view>
#include <string.h>
#include <linux/elf.h>
#ifndef INCLUDE_NR
#define INCLUDE_NR
#include <asm-generic/unistd.h> /*Headers in this order*/
#include <sys/syscall.h>
#endif
//#include <asm/unistd.h>
#include <thread>   
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <thread>   
#include <array>
#include <vector>
#include <condition_variable>
#include <algorithm>
#include <sys/mman.h>
#include <string.h>
#include <span>
#include <sys/types.h>
#include <signal.h>
#include <sys/prctl.h>

#include <sys/time.h>
#include <sys/resource.h>
#include <sys/reg.h>
//#include <linux/futex.h>      /* Definition of FUTEX_* constants */
#include "debugclone.h"
#include "maximum.h"
extern bool libre3initialized;
std::string_view syscallstr[callmax];
#ifdef __ARM_NR_BASE
#include "armmax.h"
std::string_view  armcallstr[armmax];
#endif
using namespace std;
extern bool wrongfiles() ;
bool initstr() {
#include "syscallstr.h"
#ifdef __ARM_NR_BASE
#include "armcallstr.h"
#endif
	return true;
	};
bool inited=initstr();
#ifdef TEST
#pragma message "TEST"
#endif
#include "inout.h"


#include "strconcat.h"
extern std::string_view libdirname;
#ifdef LIBRE3
static strconcat libre3lib;
#endif
//strconcat libre3lib("",R"(package:)",libdirname,"/libinit.so\n");
using namespace std;
#include "logs.h"
#include "openat.h"
#include "access.h"
#include "stat.h"
#include "slog.h"

static bool seesnetunix=false;
static constexpr const char netunix[]= R"(/proc/net/unix)";

void uit(string_view el) {
	LOGGER("%s#%d\n",el.data(),el.size());
//	const char nl[]="\n"; write(STDOUT_FILENO,el.data(),el.size()); write(STDOUT_FILENO,nl,size(nl)-1);
	}
template <int nr>
void uit(const char (&name)[nr]) {
	LOGGER("%s\n",name);
//	write(STDOUT_FILENO,name,nr-1);
	}

//#undef LOGGER
//#define LOGGER(...) fprintf(stderr,__VA_ARGS__)
template <typename F>
static span<const string_view> accessable(const string_view *names,const int *used,int usedlen,const F &func) {
	vector<int> ind;
	for(int i=0;i<usedlen;i++) {
		int pos=used[i];
		if(!func(names[pos])) {
			LOGGER("have %s\n", names[pos].data());
			ind.push_back(pos);
			}
		else {
			LOGGER("don't have %s\n", names[pos].data());
			}
		}
	auto len=ind.size();
	if(!len)
		return {(string_view *)nullptr,len};
	string_view *hier=new string_view[len];
	for(int i=0;i<len;i++)
		hier[i]=names[ind[i]];
	return {hier,len};
	}
span<const string_view>  hieraccess, hierstat, hieropen;
bool *openused,*statused,*accessused;
#ifndef TEST
extern string_view globalbasedir;
#endif

#define saveone(label) pathconcat label##name(globalbasedir,#label "used");\
{Create uit(label##name);\
const int len=hier##label.size();\
for(int i=0;i<len;i++) {\
	if(label##used[i]) {\
		askswrongfiles=true;\
		write(uit,&i,sizeof(int));\
		}\
	}\
}

bool istest=false;
extern bool askswrongfiles;
bool askswrongfiles=false;
void saveused() {

saveone(open)
saveone(access)
saveone(stat)
}
const char *mymounts=nullptr;


static bool mkmounts() {
#include "mounts.h"
	constexpr const char endmounts[]="/mounts";
	constexpr const int endmountslen=sizeof(endmounts);
	int fileslen=globalbasedir.size();
	char *str=new char[fileslen+endmountslen];
	memcpy(str,globalbasedir.data(),fileslen);
	memcpy(str+fileslen,endmounts,endmountslen);
	mymounts=str;
	{struct stat stbuf;
	if(!stat(mymounts, &stbuf)) {
		if(stbuf.st_size==sizeof(mounts)) {
			LOGGER("mymounts=%s already present\n",mymounts);
			return true;
			}
		}
	}
	writeall(mymounts,mounts,sizeof(mounts));
	LOGGER("mymounts=%s writeall\n",mymounts);
	return true;
}

#ifdef CHANGESTATUS
extern "C" {
VISIBLE extern  const char *mystatus;
};
#ifndef TEST
alignas(sizeof(char *))  const char *mystatus;
#else
alignas(sizeof(char *))  const char *mystatus="/sdcard/mystatus";
#endif
#endif
/*
template <int len> constexpr int maxarray(const std::string_view (&names)[len]) {
	int max=names[0].size();
	for(int i=1;i<std::size(names);i++)
		if(max<names[i].size())
			max= names[i].size();
	return max;
	}
constexpr int opennamesmax=maxarray(opennames);
constexpr int accessnamesmax=maxarray(accessnames);
constexpr int statnamesmax=maxarray(statnames);
*/
//extern "C" char doesnt[]="/sec_storage/bestaatniet";
/*
extern "C" {
VISIBLE extern   char *doesnt;
};
//const char doesnt[]="/xyz";
const char doesntbuf[]="/sbin/su";
char *getdoesnt() {
	char *ptr=new char[sizeof(doesntbuf)];
	strcpy(ptr,doesntbuf);
	return ptr;
	}
// char *doesnt=doesntbuf;
 char *doesnt=getdoesnt();
*/
bool filecopy(const char *in, int outh) ;
//alignas(sizeof(char *)) char mystatus[]="/sdcard/mystatus";
//typedef unsigned long long addr_t;
/*
template <int len,typename addr_t>
std::array<char,len> getdata(pid_t child, const addr_t addr) {   
	constexpr const int addr_size = sizeof(addr_t);
	constexpr const int count=(len+addr_size-1)/addr_size;
	union {
		std::array<addr_t,count> data;
		std::array<char,len> ret;
		} data;
	static_assert(offsetof(decltype(data),data)==offsetof(decltype(data),ret));
	addr_t *laddr=data.data.data();
	const addr_t *end=laddr+count;
	for(addr_t iter=addr;laddr<end;laddr++,iter+=addr_size) 
		*laddr = ptrace(PTRACE_PEEKTEXT, child, iter, NULL);
	return data.ret;
}
template <typename addr_t>
void putdata(pid_t child, addr_t addr, char *str, int len)
{   
	constexpr const int addr_size = sizeof(addr_t);
	char *laddr;
    int i, j;
    union u {
            addr_t val;
            char chars[addr_size];
    }data;
    i = 0;
    j = len / addr_size;
    laddr = str;
    while(i < j) {
        memcpy(data.chars, laddr, addr_size);
        ptrace(PTRACE_POKEDATA, child,
               addr + i * addr_size, data.val);
        ++i;
        laddr += addr_size;
    }
    j = len % addr_size;
    if(j != 0) {
        memcpy(data.chars, laddr, j);
        ptrace(PTRACE_POKEDATA, child,
               addr + i * addr_size, data.val);
    }
}
*/
struct commu {
	std::mutex Mymutex;
	std::condition_variable condVar;
	bool debugReady =false;
	pid_t tid;
	pid_t pid;
	int version;
	};

/*
bool wrongkind(const string_view *names,int len,char *name) {
	  for(int i=0;i<len;i++) 
		if(!strncmp(name,names[i].data(),names[i].size())) {
			LOGSTRING("hit ");
			return true;
		  }
	return false;
	}
	*/
#if defined(__aarch64__)
class Register {

 struct user_pt_regs regs;
 struct iovec io={.iov_base = &regs, .iov_len = sizeof(regs)};
pid_t pid;
public:
typedef std::decay<decltype(regs.regs[0])>::type  type;
Register(pid_t pid): pid(pid) {}
bool getall() {
    return ptrace(PTRACE_GETREGSET, pid, (void*)NT_PRSTATUS, &io) != -1;
    }
    /*
type &operator()(int of) {
	return regs.regs[of];
	}
	*/
void set(int of,const type val) {
	regs.regs[of]=val;
	}
type get(int of) const {
	return regs.regs[of];
	}

bool setall() {
	return ptrace(PTRACE_SETREGSET, pid, (void*)NT_PRSTATUS, &io)!=-1;
	}
type  callnr() const {
	return regs.regs[8];
	}
void  setcallnr(type val)  {
	regs.regs[8]=val;
	}
type ret() const {
	return regs.regs[0];
	}
void setret(type val)  {
	regs.regs[0]=val;
	}
/*
type & callnr() {
	return regs.regs[8];
	}
	*/
//#define callreg regs.regs[8]

};
#elif defined(__arm__)

class Register {
	struct pt_regs regs;
	pid_t pid;
public:
typedef std::decay<decltype(regs.uregs[0])>::type  type;
Register(pid_t pid): pid(pid) {}
bool getall() {
   return  ptrace(PTRACE_GETREGS, pid, 0, &regs) != -1||errno==ESRCH;
    }
void set(int of,const type val) {
	regs.uregs[of]=val;
	}
type get(int of) const {
	return regs.uregs[of];
	}
type lr() const {
	return regs.ARM_lr;
	}
bool setall() {
	bool success=ptrace(PTRACE_SETREGS, pid, 0,&regs)!=-1||errno==ESRCH;
	return success;
	}
type  callnr() const {
	return regs.uregs[7];
	}
void  setcallnr(type val)  {
	regs.uregs[7]=val;
	}
type ret() const {
	return regs.uregs[0];
	}
void setret(type val)  {
	regs.uregs[0]=val;
	}
//#define callreg regs.regs[8]

};
#else
/*
arch	syscallNR	return	arg0	arg1	arg2	arg3	arg4	arg5
x86	eax		eax	ebx	ecx	edx	esi	edi	ebp
x86_64	rax		rax	rdi	rsi	rdx	r10	r8	r9
*/
class Register {

const pid_t pid;
public:
//typedef unsigned long  type;
typedef uintptr_t   type;

#if defined(__x86_64__) //__amd64
const type regs[7]={RDI,RSI,RDX,R10,R8,R9};
const type callreg= ORIG_RAX;
const type retreg= RAX;
#else
const type regs[7]={EBX,ECX,EDX,ESI,EDI,EBP};
const type callreg= ORIG_EAX;		
const type retreg=EAX;
#endif

Register(const pid_t pid): pid(pid) {}
bool getall() {
	return true;
	}
bool setall() {
	return true;
	}

void set(int of,const type val) {
	 ptrace(PTRACE_POKEUSER,pid,sizeof(type)*regs[of],val);
	}
type get(int of) const {
	 return ptrace(PTRACE_PEEKUSER,pid,sizeof(type)*regs[of],NULL);
	}

type  callnr() const {
 return ptrace(PTRACE_PEEKUSER,pid,sizeof(type)*callreg,NULL);
 }
void  setcallnr(type val)  {
	 ptrace(PTRACE_POKEUSER,pid,sizeof(type)*callreg,val);
 	}
type ret() const {
 return ptrace(PTRACE_PEEKUSER,pid,sizeof(type)*retreg,NULL);
 }
void setret(type val) {
	 ptrace(PTRACE_POKEUSER,pid,sizeof(type)*retreg,val);
	}
 };
#endif

struct redir {
	int (*func)(void *arg) ;
	void *arg;
	bool thread;
	};

thread_local pid_t has_debugger=0;

extern pid_t getdebugclone(const int version) ;
int cloner(void *arg) {
// 	doesnt=getdoesnt();
	redir *re=(redir *)arg;
	int (*func)(void *arg) =re->func;
	void *argu= re->arg;
	if(re->thread) {
		delete re;
		}
//	debugclone(false,3);
	pid_t pid=0;
	if(wrongfiles())
		pid=getdebugclone(3);
	int res=func(argu);
    	if(pid) {
		getsid(pid);
		} 
	return res;
	}


bool	change(const char *ptr,const char *was,const char *becomes,Register &regi,Register::type reg) {
	  if(strcmp(ptr,was)) 
	  	return false;
	regi.set(reg,(Register::type)becomes);
	regi.setall();
	LOGAR(" * ");
	return true;
	}
	/*
bool verg(const string_view one,const string_view two) {
	if(one.size()) {
		return  strncmp(one.data(),two.data(),one.size())<0;
		}
	else
		return  strncmp(one.data(),two.data(),two.size())<0;

	}
	*/
int verg(const string_view one,const string_view two) {
        if(one.size()) {
                return  strncmp(one.data(),two.data(),one.size());
                }
        else
                return  strncmp(one.data(),two.data(),two.size());

        }
template< class T,class Compare >
constexpr int binary_find(const T *start,size_t len, const T value, Compare comp ) {
        auto end=start+len;
        const T* res=lower_bound(start,end,value,[comp](const T one, const T two){return comp(one,two)<0;});
        if(res==end||comp(value,*res)!=0)
                return  -1;
        return res-start;
        }


//bool rewrong(span<string_view> files,char *name,pid_t pid,struct iovec *ioptr,struct user_pt_regs *regsptr) {
bool rewrongval(span<const string_view> files,bool *useds,pid_t pid,Register &regi,char *name ,int regnr=1) {
	const string_view *names=files.data(); 
	int len=files.size();
	string_view zoek{name,0};
	if(int pos=binary_find(names,len, zoek, verg);pos>=0) {
		useds[pos]=true;
		{
		slog log;

		log <<names[pos].data()<<" blocked "<<endl;
		}
		*name='\0';
		return true;
		}
	slog log;
	log<<name<<" unblocked "<<endl;;
	return false;
	}
bool rewrong(span<const string_view> files,bool *useds,pid_t pid,Register &regi,int regnr=1) {
  	const Register::type reg= regi.get(regnr);
	if(!reg) 
		return false;
	return  rewrongval(files,useds,pid,regi,(char *) reg,regnr); 
	}
#undef waitpid
#define waitpid(pid,wstatus,options)   wait4(pid, wstatus, options, nullptr)

char *getmem(int size) {
   void *stack = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
   if (stack == MAP_FAILED) {
               flerror("mmap");
	       	return nullptr;
	       }
	return (char *)stack;
	}
void notify(commu *com) {
    {
        std::lock_guard<std::mutex> lck(com->Mymutex);
        com->debugReady = true;
    }
    com->condVar.notify_one();              
}

int syscallwait(pid_t pid) {
    while(true) {
	    if(ptrace(PTRACE_SYSCALL, pid, 0, 0) == -1) {
			{int ern=errno;
			slog log;
			log<<"PTRACE_SYSCALL "<<pid<<" failed: "<<strerror(ern)<<endl;
			};



		ptrace(PTRACE_DETACH, pid, 0, 0);
		return 5;
	    }
	    int status;
	    if(waitpid(pid, &status, 0) == -1) {
	    	int waserrno=errno;
		{
		slog log;
		log<<"2 waitpid "<<pid<<": "<<strerror(waserrno)<<endl;;
		};
		if (waserrno != ECHILD) {
		    ptrace(PTRACE_DETACH, pid, 0, 0);
		    return 2;
		}
		}
	const auto sign=WSTOPSIG(status);
	const bool stopped=WIFSTOPPED(status) ;
#ifdef SYSGOOD 
        if(stopped && (sign & 0x80))
            return 0;
        if(WIFEXITED(status)) {
	     LOGAR("WIFEXITED(status))");
            return 1;
	    }
#else
	return 0;
#endif
	{
	slog log;
        log<<"stopped status "<<status<<" "<<stopped<<" "<<sign<<endl;;
	}
    }
}

int debugger(void *arg) {
commu *com=reinterpret_cast<commu *>(arg);
pid_t pid=com->tid;
//debugtid=pid;
const pid_t ownpid= syscall(SYS_getpid);
{slog log;
log<<"pid="<<syscall(SYS_getpid)<<" tid="<<syscall(SYS_gettid)<<" debugs "<<pid<<endl;
}

//    signal(SIGUSR1,sighandler);
//ptrace(PTRACE_SEIZE, pid, 0, 0);
if(ptrace(PTRACE_ATTACH, pid, 0, 0)==-1) {
	{
	int waserrno=errno;
	slog log;
	log<<"PTRACE_ATTACH "<<pid<<" failed: "<<strerror(waserrno)<<endl;
	}
	notify(com); //don't block, crash
	return 3;
	}
//tgkill(com->pid, pid, SIGCONT);
//if(wait(NULL)==-1) {
//sleep(1);

if(waitpid(pid, 0, 0)==-1) {
	int waserrno=errno;
	{ slog log;
    	log<<"1 waitpid "<<pid<< strerror(waserrno)<<endl;
	}
	if(waserrno!= ECHILD ) {
		ptrace(PTRACE_DETACH, pid, 0, 0);
		notify(com);
		tgkill(com->pid, pid, SIGCONT);

		return 1;
		}
    	}
#ifdef SYSGOOD 
ptrace(PTRACE_SETOPTIONS, pid, 0, PTRACE_O_TRACESYSGOOD);
#endif

LOGAR("voor notify");
notify(com);
LOGAR("na notify");
#ifdef LIBRE3
int version=com->version;
bool followthreads=version==3&&wrongfiles();
#endif
//getchar();
#ifndef __NR_open
constexpr const
#endif
int openreg=1;
Register regi(pid);
#ifdef LIBRE3
bool wasclone=false,waspipe=false;
#endif
for (;;) {
   {int syserr;
   if((syserr=syscallwait(pid))) {
   	return syserr;
   	}
  }

//    struct user_regs_struct regs;
    if (!regi.getall()) {
	{int waserrno=errno;
    	slog log;
        log<<"PTRACE_GETREGSET "<<pid<<" to get registers: "<<strerror(waserrno)<<endl;
	};
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return 4;
    }
    int syscallnr = regi.callnr();
    const char *callstr = syscallnr < std::size(syscallstr) ? syscallstr[syscallnr].data() : (
#ifdef __ARM_NR_BASE
            (syscallnr >= __ARM_NR_BASE && syscallnr < (__ARM_NR_BASE + std::size(armcallstr)))
            ? armcallstr[syscallnr - __ARM_NR_BASE].data() :
            #endif
            "Unknown");
   {slog log;
    log<<pid<<" syscallnr "<<syscallnr<<" "<<callstr<<endl;
    }
    bool askdump = false;
    auto reg0 = regi.get(0);

    switch (syscallnr) {
    #ifdef  __NR_mmap2
	case __NR_mmap2: {
//		int len=regi.get(1);
//		LOGGER("mmap2(0x%lx %d %ld %ld %ld %ld)\n",regi.get(0),len,regi.get(2),regi.get(3),regi.get(4),regi.get(5));
		break;
		}
#endif


#ifdef LIBRE3
        case __NR_clone: {
            unsigned long flags = regi.get(0);
	    {
	    	slog log;
            	log<<"clone flags="<<flags<<endl;;
	    }
            if (followthreads) {
                if (flags & CLONE_THREAD) {
                    unsigned long **stack = (unsigned long **) regi.get(1);
                    redir *re = new redir{(int (*)(void *)) *stack, (void *) stack[1],
                                          static_cast<bool>(flags & CLONE_THREAD)};
                    *stack = (unsigned long *) cloner;
                    stack[1] = (unsigned long *) re;
                };
            }
        }
            break;
#endif
        case __NR_prctl:
            if (regi.get(0) == PR_GET_DUMPABLE) {
		slog log;
		log<<"ask PR_GET_DUMPABLE"<<endl;
                askdump = true;
            }
            break;
        case __NR_getsid: {
			pid_t thepid = reg0;
//			LOGGER("getsid(%d) ownpid=%d\n", thepid, ownpid);
			if(thepid == ownpid) {
				{
				slog log;
				log<<"STOPSIGNAL "<<pid<<endl;
				}
				if(istest)
					saveused();
				if (ptrace(PTRACE_DETACH, pid, 0, 0) == -1) {
					slog log;
					log<<"PTRACE_DETACH "<<pid<<" failed"<<endl;
				}
				return 0;
			}
		};break;


        case __NR_exit: {
            if (ptrace(PTRACE_DETACH, pid, 0, 0) == -1) {
		slog log; log<<"PTRACE_DETACH "<<pid<<" failed\n";
            }

            if (waitpid(pid, 0, 0) == -1) {
                int waserrno = errno;
		{slog log;
		log <<"5 waitpid "<<pid<<endl;;
		}
                if (waserrno != ECHILD) {
                    return 12;
                }
            }
            return 14;
        };
            break;
        case __NR_close: {slog log;
		log<<(int)regi.get(0)<<endl;
		};
            break;
#ifdef LIBRE3
        case __NR_ioprio_get: { 
	   {slog log;
            log<<"ioprio_get("<< (int) regi.get(0)<<","<< regi.get(1)<<")"<<endl;
	    }
            if (regi.get(1) == ownpid) {
                version = regi.get(0);
                followthreads = version == 3 && wrongfiles();
            }
		}
            break;
#endif
#ifdef __NR_statx
        case __NR_statx:
#endif
#ifdef __NR_newfstatat
            case __NR_newfstatat:
#else
        case __NR_fstatat64:
#endif

            rewrong(hierstat, statused, pid, regi, 1);
            break;
#ifdef __NR_stat
        case __NR_stat:
#endif
#ifdef __NR_stat64
        case __NR_stat64:
#endif
#if defined( __NR_stat64) || defined( __NR_stat)

            rewrong(hierstat, statused, pid, regi, 0);
            break;
#endif
        case __NR_faccessat:
            rewrong(hieraccess, accessused, pid, regi, 1);
            break;


#ifdef __NR_access
        case __NR_access:
            rewrong(hieraccess, accessused, pid, regi, 0);
            break;
#endif
#ifdef __NR_open
        case __NR_open:
            openreg = 0;
#endif
        case __NR_openat: {
            const Register::type reg1 = regi.get(openreg);
            if (reg1) {
                static Register::type oldreg{};
                char *name = (char *) reg1;
                if (reg1 != oldreg) {
                    constexpr char mounts[] = "/proc/self/mounts";
                    if (change(name, mounts, mymounts, regi, openreg)) {
		    	slog log;
                        log<<name<<"->"<< mymounts<<endl;
                    } else {
#ifdef CHANGESTATUS
                        constexpr char status[]="/proc/self/status";
                        if(change(name,status,mystatus,regi,openreg)) {
			   slog log;
			   log<<name<<"->"<<mystatus<<endl;
                          }
                      else
#endif

                        {
			  if(seesnetunix&&!strcmp(name,netunix))  {
				slog log;
				LOGAR("block /proc/net/unix");
				*name = '\0';
				}
			else {
				    if(rewrongval(hieropen, openused, pid, regi, name, openreg)) {
					oldreg = (uintptr_t) name;
				    }
			    	}
                        }
                    }
                } else {
			slog log;
			log<<name<<" old regi(1)  "<<endl;
                    *name = '\0';
                }

            } else  {
			slog log;
			log<<"zero regi(1)\n";
			}
        };

#ifdef __NR_open
            openreg = 1;
#endif
            break;
        case __NR_dup:
		{
		slprint log;
		log<<regi.get(0);
		}
            break;
#if defined(__aarch64__) 
	case __NR_mmap: {
		int len=regi.get(1);
		if(len==1089536) { //Otherwise crashes in munmap, but only when it is debugged. As you probably will understand later.
		//	LOGGER("mmap %llx %d %lld %lld %lld %lld\n",regi.get(0),len,regi.get(2),regi.get(3),regi.get(4),regi.get(5));
			if (ptrace(PTRACE_DETACH, pid, 0, 0) == -1) {
				int waserrno=errno;
				slog log;
				log<<"PTRACE_DETACH "<<pid<<" failed "<<strerror(waserrno)<<endl;
				}
			return 0;
			}
		break; 
		};
#endif

        case __NR_munmap: {
	    void *addr= (void*)regi.get(0);
            auto len = regi.get(1);
	    slog log;
            log <<"munmap "<<slbase(16)<<(intptr_t)addr<<" "<<slbase(10)<< len<<endl;;
        }; 

        break;
	default:;
	};

   {int syserr;
	   if((syserr=syscallwait(pid))) {
		return syserr;
		}
	}
	if(!regi.getall()) {
		int waserrno=errno;
		{
		slog log;
		log<<"2 PTRACE_GETREGSET "<<pid<<" failed "<<strerror(waserrno)<<endl;
		};
		ptrace(PTRACE_DETACH, pid, 0, 0);
		return 6;
	   }
	switch(syscallnr) {
		case  __NR_read:  
#ifdef LIBRE3
		    if(version==3) {
			if(waspipe&&wasclone) {
			char *data=(char *)regi.get(1);
			LOGGERN(data,25);
			 int siz=regi.ret();
			 int bufsize=regi.get(2);
 char *package=libre3lib.data();
 int packagelen=libre3lib.size();
 		{ 
		slog log;
		log<<"len="<<packagelen<<" siz="<<siz<<" bufsize="<<bufsize<<" package="<<package;
		}
			if(bufsize>=packagelen&&!memcmp(data,"package",7)) {
				memcpy(data,package,packagelen);
				waspipe=false;
				wasclone=false;
				LOGAR("changed package");
				regi.setret(packagelen);
				regi.setall();
				libre3initialized=true;
				}
			  }
			}
#endif
			break;	
#ifdef LIBRE3
		case  __NR_clone: 
			if(version==3) {
			    if(!(reg0&CLONE_VM))  {
				wasclone=true;
				}
				}
			break;
		case  __NR_pipe2:  {
			if(version==3) {
				waspipe=true;
				}
			};break;
#endif
		case __NR_prctl:
			if(askdump) {
				{slog log;
				log<<"give PR_GET_DUMPABLE=0, real "<<(int)regi.ret()<<endl;
				}
				regi.setret(0);
				regi.setall();
				}
			break;

	//	case  __NR_clone: LOGSTRING("clone returned\n");

#ifndef NOLOG
#if defined(__aarch64__)
	case __NR_mmap: 
#endif
    #ifdef  __NR_mmap2
	case __NR_mmap2: 
	#endif
	case __NR_munmap:
		{
		slog log;
		log<<callstr<<"="<<slbase(16)<<regi.ret()<<endl;
		};break;
	#ifdef __NR_newfstatat
		case __NR_newfstatat:
	#else
		case __NR_fstatat64:
	#endif
		case  __NR_faccessat:
		case __NR_dup:
#ifdef __NR_open
		case __NR_open:
#endif
		case __NR_openat: 
			{
			slog log;
			log<<callstr<<"="<<(int)regi.ret()<<endl;
			}
			break;
#endif
		};
   }

}

static bool beforedebug() {
#ifdef LIBRE3
 libre3lib=strconcat("",R"(package:)",libdirname,"/libinit.so\n");
 LOGGER("libre3lib=%s",libre3lib.data());
#endif
	   if(prctl(PR_SET_DUMPABLE, 1, 0, 0, 0) < 0)  {
		    LOGGER("could not set dumpable bit flag for pid %d: %s\n", getpid(), strerror(errno));
		return false;
		    }
	    else {
		LOGGER("dumpable bit flag set for pid %d\n", getpid());
		    struct rlimit rl;
#ifdef NDEBUG
		    rl.rlim_cur = 0;
#else
		    rl.rlim_cur = RLIM_INFINITY;
#endif
		    rl.rlim_max = RLIM_INFINITY;
		if(setrlimit(RLIMIT_CORE, &rl) < 0)
			LOGGER("could not disable core file generation for pid %d: %s\n", getpid(), strerror(errno));
		else {
			LOGGER("Turned off core file generation for pid %d\n", getpid());
			}
		return true;
		}
	}


pid_t getdebugclone(const int version) {
LOGAR("getdebugclone");
#ifdef CHANGESTATUS
     	  __attribute__((used))  static bool hasstatus=getstatus();     
#endif
     	  __attribute__((used))  static bool hasmounts=mkmounts();     

	constexpr int STACK_SIZE (1024 * 1024);
	char *vstack=getmem(STACK_SIZE);
	if(!vstack) {
		LOGAR("getmem(STACK_SIZE)==null");
		return 0;
		}
	pid_t debugpid;
	pid_t mytid=syscall(SYS_gettid);
	commu com{.tid=mytid,.pid=static_cast<pid_t>(syscall(SYS_getpid)),.version=version};
	static bool isbeforedebug=beforedebug();
	LOGAR("before clone");
	if((debugpid=clone(debugger, vstack+STACK_SIZE, CLONE_VM|  CLONE_FILES | CLONE_FS | CLONE_IO|CLONE_UNTRACED , reinterpret_cast<void*>(&com))) == -1) { 
		int older=errno;
		slog log;
		log<<"failed to spawn child task "<<strerror(older)<<endl;
		return 0;
	    }
      std::unique_lock<std::mutex> lck(com.Mymutex);
    com.condVar.wait(lck, [&com]{ return com.debugReady; });
   	slog log; 
	log<<mytid<<" got debugclone="<<debugpid<<endl;
    return debugpid;
	}

class thedebugger {

public:
thedebugger(){
		}
~thedebugger() {
	const pid_t pid=has_debugger;
	{
	slog log;
	log<<"~thedebugger pid="<<pid<<endl;
	}
 	if(pid) {
		syscall(SYS_getsid,pid);
		}
	has_debugger=0;
	}

};
void logfiles(decltype(hieraccess) fil) {
	for(auto el:fil) 
		LOGGER("%s\n",el.data());
	}
#ifndef USEDIN
Readall<int> reusestat;
Readall<int> reuseopen;
Readall<int> reuseaccess;
#else

alignas(sizeof(int))
#include "used/usstat.h"
const std::array<const int,sizeof(usstat)/sizeof(int)> *reusestat=reinterpret_cast<const std::array<const int,sizeof(usstat)/sizeof(int)>*>(usstat) ;
alignas(sizeof(int))
#include "used/usaccess.h"
const std::array<const int,sizeof(usaccess)/sizeof(int)> *reuseaccess=reinterpret_cast<const std::array<const int,sizeof(usaccess)/sizeof(int)>*>(usaccess) ;
alignas(sizeof(int))

#include "used/usopen.h"
const std::array<const int,sizeof(usopen)/sizeof(int)> *reuseopen=reinterpret_cast<const std::array<const int,sizeof(usopen)/sizeof(int)>*>(usopen) ;
#endif
extern		std::vector<std::string_view> usedfiles;
std::vector<std::string_view> usedfiles;

	
extern int dodebug(); 
static bool initialized=false;
static bool needsdebug() {
	const int debug=dodebug();
	LOGGER("dodebug()=%d\n",debug);
	if(debug>0) {
		istest=true;
		hieraccess={accessnames,std::size(accessnames)};
		hierstat={statnames,std::size(statnames)};
		hieropen={opennames,std::size(opennames)};
	}
	else {
	LOGSTRING("Access:\n");
  hieraccess=accessable(accessnames,
#ifndef USEDIN
	  reuseaccess.fromfile(pathconcat(globalbasedir,"accessused")),reuseaccess.size()
#else
	reuseaccess->data(),reuseaccess->size()

#endif
	    ,[](const  string_view view){
		const char *name=view.data();
		if(!access(name,F_OK)) {
			LOGGER("access: %s\n",name);
			return 0;
			}
			/*
		int ern=errno;
		if(ern==EACCES)  {
			LOGGER("access: %s: EACCES\n",name);
			return 0;
			} */
		const char ends[]="/.trdpx";
		const int len=view.size();
		char buf[sizeof(ends)+ len];
		memcpy(buf,name,len);
		memcpy(buf+len,ends,sizeof(ends));
		if(!access(buf,F_OK)) {
			LOGGER("access: %s\n",buf);
			return 0;
			}
//		if(errno==EACCES||errno==ENOTDIR)
		if(errno==ENOTDIR) {
			LOGGER("access: %s: ENOTDIR\n",buf);
			return 0;
			}
		return -1;
		});
		;
		LOGSTRING("Stat: \n");
	 hierstat=accessable(statnames,

#ifndef USEDIN
	 reusestat.fromfile(pathconcat(globalbasedir,"statused")),reusestat.size()

#else
	reusestat->data(),reusestat->size()

#endif
	 ,[](const string_view view){struct stat st;
		const char *name=view.data();
		if(!stat(name,&st)) {
			LOGGER("stat %s\n",name);
			return 0;
			};
		if(errno==EACCES) {
			if(!memcmp(name,"/data/",6)) {
				LOGGER("stat %s EACCES\n",name);
				return 0;
				}
			} 
		else {
			if(errno==ENOTDIR) {
				LOGGER("stat %s ENOTDIR\n",name);
				return 0;
				}
			}
		const char ends[]="/.trdpx";
		const int len=view.size();
		char buf[sizeof(ends)+ len];
		memcpy(buf,name,len);
		memcpy(buf+len,ends,sizeof(ends));
		if(!stat(buf,&st)) {
			LOGGER("stat %s\n",buf);
			return 0;
			};
		if(errno==ENOTDIR) {
			LOGGER("stat %s ENOTDIR\n",buf);
			return 0;
			}
		return -1;
		});
	LOGSTRING("Open:\n");	
	 hieropen=accessable(opennames,

#ifndef USEDIN
	 reuseopen.fromfile(pathconcat(globalbasedir,"openused")),reuseopen.size()
#else
	reuseopen->data(),reuseopen->size()

#endif

	 ,[](const string_view view){
		const char *name=view.data();
		int han= open(name,O_RDONLY);
		if(han>=0) {
			LOGGER("open %s\n",name);
			close(han);
			return 0;
			}

#if defined(__arm__)
		if(name!=opennames[3].data()) 
#endif
		{
			const char ends[]="/.trdpx";
			const int len=view.size();
			char buf[sizeof(ends)+ len];
			memcpy(buf,name,len);
			memcpy(buf+len,ends,sizeof(ends));
			han= open(buf,O_RDONLY);
			if(han>=0) {
				LOGGER("open %s\n",buf);
				close(han);
				return 0;
				}
			if(errno==ENOTDIR) {
				LOGGER("open %s ENOTDIR\n",buf);
				return 0;
				}
			}
		return -1;
	});
	/*{

	} */
	if(hieraccess.size()){
		uit("access:" );
		for(auto el:hieraccess)  
			uit(el);
		}
	if(hierstat.size()) {
		uit("stat:");
		for(auto el:hierstat) 
			uit(el);
		}
	if(hieropen.size())  {
		uit("open:");
		for(auto el:hieropen) 
			uit(el);
		}
	}
	int totaal=hieraccess.size()+hieropen.size()+hierstat.size();


	initialized=true;
	if(totaal) {
		int han= open(netunix,O_RDONLY);
		if(han>=0) {
			LOGGER("can open %s\n",	netunix);
			close(han);
			seesnetunix=true;
			}
		else  {
			LOGGER("can not open %s\n",	netunix);
			}
		if(!istest) {
			int firstlen=hieraccess.size()+hieropen.size();
			std::string_view tmp[firstlen];
			auto *endtmp=std::set_union(hieraccess.begin(),hieraccess.end(),hieropen.begin(),hieropen.end(),tmp);
			LOGGER("tmp size=%zu\n",endtmp-tmp);
			usedfiles.clear();
			std::set_union(hierstat.begin(),hierstat.end(),tmp,endtmp,std::back_inserter(usedfiles));
			LOGGER("usedfiles.size=%zu\n",usedfiles.size());
			constexpr const bool blockall=true;
			if(blockall) { //block all files to be certain
				hieraccess={accessnames,std::size(accessnames)};
				hierstat={statnames,std::size(statnames)};
				hieropen={opennames,std::size(opennames)};
				}
			}
		accessused=new bool[hieraccess.size()]();
		openused=new bool[hieropen.size()]();
		statused=new bool[hierstat.size()]();
		return true;
		  }
	return false;
	}

bool filecopy(const char *in, int outh) {
	Open inh(in,O_RDONLY);
	if(inh<=0)
		return false;
	constexpr const int blsize= 4096;
	char buf[blsize];
	int len;
	do {
		len=read(inh,buf,blsize);
		if(len<0) {
			lerror(in);
			return false;
			}
		write(outh,buf,len);
		} while(len==blsize);
	return true;
	}
#ifdef TEST
#undef CHANGESTATUS
#endif
#ifdef CHANGESTATUS
static bool getstatus() {
	constexpr int statuslen=8;
	constexpr const char status[]="/proc/self/status";

	int fileslen=globalbasedir.size();
	char *str=new char[fileslen+statuslen];
	memcpy(str,globalbasedir.data(),fileslen);
	memcpy(str+fileslen,status+sizeof(status)-statuslen,statuslen);
	mystatus=str;
	LOGGER("mystatus=%s\n",mystatus);
        Create outh(mystatus);
      if(outh<=0)
      		return false;
	if(struct stat st;fstat(outh,&st)==0&&st.st_size>300)
		return true;
	return filecopy(status,outh);
}
#endif 
//bool wrongfiles=false;
extern bool wrongfiles() ;
bool *wrongptr=nullptr;
bool wrongfiles() {

	static bool wrong=(wrong=needsdebug(),wrongptr=&wrong,wrong);
#ifdef TESTDEBUG
	return true;
#else
	return wrong;
#endif
	}
bool resetwrong() {
	if(wrongptr)	
		return (*wrongptr=needsdebug());
	return wrongfiles();	
	}

pid_t debugclone(bool doalways,const int version) {

//	if(!realyneeds&&debug<0) return false;
   if(version==2&&(dodebug()<0)) {
	LOGGER("debugclone(%d,%d)=1\n",doalways,version);
   	return 1;
	}
LOGGER("debugclone(%d,%d)\n",doalways,version);
     if(wrongfiles()||doalways) {
#ifdef LIBRE3
	static thread_local	int wasversion ;
#endif
	if(!has_debugger) {
		has_debugger=getdebugclone(version);
#ifdef LIBRE3
		wasversion=version;
#endif
		}
#ifdef LIBRE3
         else {
		if(version==3&&wasversion!=3) {
			int res=syscall( __NR_ioprio_get,3,has_debugger);
			LOGGER("ioprio_get=%d\n",res);
			}
		}
#endif
	static thread_local	thedebugger debugger;
	LOGAR("end debugclone");
	return has_debugger;
	    }

	return 1;
    }

#else
#ifdef CHANGESTATUS
extern "C" {
VISIBLE extern  const char *mystatus;
};
const char *mystatus ="/sdcard/mystatus";
#endif
#endif
#ifndef CHANGESTATUS
extern "C" {
VISIBLE extern  volatile const char *mystatus;
volatile const char *mystatus =BASEDIR "/mystatus";
};


#endif

