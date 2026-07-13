
//#define DOTESTS
//#define SYSGOOD 1
//#define FIXED_URANDOM 1
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

//WARNING: don't use snprintf in the debugclone, it hangs under Android 8. Use std::to_chars instead. slog for logging.


/*
arch    syscallNR    return    arg0    arg1    arg2    arg3    arg4    arg5
arm    r7        r0    r0    r1    r2    r3    r4    r5
arm64    x8        x0    x0    x1    x2    x3    x4    x5
x86    eax        eax    ebx    ecx    edx    esi    edi    ebp
x86_64    rax        rax    rdi    rsi    rdx    r10    r8    r9
*/
//#define CHANGESTATUS
#include <sys/mman.h>

#include "config.h"

#ifdef DOTESTS
#define L3_ORACLE_DETERMINISTIC_GETRANDOM 1
#define L3_ORACLE_NORMALIZE_PROC_TASK 1
#define FIXED_URANDOM 1
#endif
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
#include <stdio.h>
#include  <sys/user.h>
#include <string_view>
#include <string>
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
#ifndef HAVE_NOPRCTL
#include <sys/prctl.h>
#endif

#include <sys/time.h>
#include <sys/resource.h>
#include <sys/reg.h>
//#include <linux/futex.h>      /* Definition of FUTEX_* constants */
#include "debugclone.hpp"
#include "maximum.hpp"
#include "destruct.hpp"
#ifdef FIXED_URANDOM
static int openedurandom=-1;
const char myurandom[]{"/etc/hosts"};
#endif

/* Deterministic oracle controls for Libre3 replay. */

#ifndef NOLOG
static void copyhexbytes(char *out,int outlen,const char *in,int inlen);
#endif
#ifndef TEST
extern std::string_view globalbasedir;
#endif

static std::mutex oracle_ctl_mutex;
static size_t oracle_getrandom_cursor=0;
static std::string oracle_task_dir;
static std::string oracle_task_comm_path;
static bool oracle_proc_ready=false;

static unsigned char oracle_byte_at(size_t pos) {
    return static_cast<unsigned char>((0xA5u + 0x6Du * static_cast<unsigned>(pos) + (static_cast<unsigned>(pos >> 3) * 0x11u)) & 0xFFu);
}

static bool oracle_tracee_shares_vm(pid_t tid) {
#define task "/proc/self/task/"
    constexpr const int maxpath=64;
    char path[maxpath]=task;
    std::to_chars_result res=std::to_chars(path+sizeof(task)-1,path+maxpath,tid);
    return faccessat(AT_FDCWD, path, F_OK, 0) == 0;
    }

static bool ptrace_write_bytes_fallback(pid_t pid, uintptr_t addr, const unsigned char *data, size_t len) {
    if(!addr || !data || !len)
        return true;

    /*
     * Fallback only: use this when the tracee is a different process without
     * CLONE_VM.  It handles unaligned tracee addresses by preserving untouched
     * bytes at the start/end of each ptrace word.
     */
    constexpr size_t word = sizeof(long);
    const uintptr_t mask = static_cast<uintptr_t>(word - 1);
    uintptr_t wordaddr = addr & ~mask;
    size_t inpos = 0;
    size_t byteoff = static_cast<size_t>(addr - wordaddr);

    while(inpos < len) {
        union { long val; unsigned char bytes[word]; } u{};
        const size_t chunk = std::min(word - byteoff, len - inpos);

        if(byteoff != 0 || chunk != word) {
            errno=0;
            u.val = ptrace(PTRACE_PEEKDATA, pid, wordaddr, nullptr);
            if(errno)
                return false;
        }

        memcpy(u.bytes + byteoff, data + inpos, chunk);
        if(ptrace(PTRACE_POKEDATA, pid, wordaddr, u.val)==-1)
            return false;

        inpos += chunk;
        wordaddr += word;
        byteoff = 0;
    }
    return true;
}

static bool write_tracee_bytes(pid_t pid, uintptr_t addr, const unsigned char *data, size_t len) {
    if(!addr || !data || !len)
        return true;
    if(oracle_tracee_shares_vm(pid)) {
        memcpy(reinterpret_cast<void *>(addr), data, len);
        return true;
    }
    return ptrace_write_bytes_fallback(pid, addr, data, len);
}

static bool fill_tracee_with_oracle_bytes(pid_t pid, uintptr_t addr, size_t len, char *hexout, int hexoutlen) {
    if(!addr || !len)
        return true;
    std::vector<unsigned char> data(len);
    {
        std::lock_guard<std::mutex> lk(oracle_ctl_mutex);
        for(size_t i=0;i<len;i++)
            data[i]=oracle_byte_at(oracle_getrandom_cursor++);
    }
#ifndef NOLOG
    if(hexout && hexoutlen>0)
        copyhexbytes(hexout, hexoutlen, reinterpret_cast<const char *>(data.data()), static_cast<int>(std::min<size_t>(len, 32)));
#else
    (void)hexout;
    (void)hexoutlen;
#endif
    return write_tracee_bytes(pid, addr, data.data(), len);
}

#ifndef TEST
static bool ensure_oracle_proc_tree() {
#if L3_ORACLE_NORMALIZE_PROC_TASK
    std::lock_guard<std::mutex> lk(oracle_ctl_mutex);
    if(oracle_proc_ready)
        return true;
    oracle_task_dir.assign(globalbasedir.data(), globalbasedir.size());
    oracle_task_dir += "/oracle_proc_self_task";
    std::string one = oracle_task_dir + "/1";
    oracle_task_comm_path = one + "/comm";
    mkdir(oracle_task_dir.c_str(), 0777);
    mkdir(one.c_str(), 0777);
    int fd = open(oracle_task_comm_path.c_str(), O_CREAT|O_TRUNC|O_WRONLY|O_CLOEXEC, 0666);
    if(fd >= 0) {
        static constexpr char comm[]="testlib\n";
        write(fd, comm, sizeof(comm)-1);
        close(fd);
    }
    oracle_proc_ready=true;
#endif
    return oracle_proc_ready;
}

static bool endswith_cstr(const char *str,const char *suffix) {
    if(!str || !suffix)
        return false;
    const size_t slen=strlen(str), tlen=strlen(suffix);
    return slen>=tlen && !memcmp(str+slen-tlen, suffix, tlen);
}
#endif
extern bool libre3initialized;
std::string_view syscallstr[callmax];
#ifdef __ARM_NR_BASE
#include "armmax.hpp"
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
#include "inout.hpp"


#include "strsepconcat.hpp"
extern std::string_view libdirname;
#ifdef LIBRE3
static strsepconcat libre3lib;
#endif
//strsepconcat libre3lib("",R"(package:)",libdirname,"/libinit.so\n");
using namespace std;
#include "logs.hpp"
#include "openat.hpp"
#include "access.hpp"
#include "stat.hpp"
#include "slog.hpp"

#ifndef NOLOG
#define LOG_DO(statement) do { statement; } while(false)
#define LOG_ARGPTR(value) (&(value))
#define LOG_BUF(value) (value)
#define LOG_BUFLEN(value) (value)
#else
#define LOG_DO(statement) do {} while(false)
#define LOG_ARGPTR(value) nullptr
#define LOG_BUF(value) nullptr
#define LOG_BUFLEN(value) 0
#endif

static  char *newapkname;
static bool seesnetunix=false;
static constexpr const char netunix[]= R"(/proc/net/unix)";

void uit(string_view el) {
    LOG_DO(LOGGER("%s#%d\n",el.data(),el.size()));
//    const char nl[]="\n"; write(STDOUT_FILENO,el.data(),el.size()); write(STDOUT_FILENO,nl,size(nl)-1);
    }
template <int nr>
void uit(const char (&name)[nr]) {
    LOG_DO(LOGGER("%s\n",name));
//    write(STDOUT_FILENO,name,nr-1);
    }

//#undef LOGGER
//#define LOGGER(...) fprintf(stderr,__VA_ARGS__)
template <typename F>
static span<const string_view> accessable(const string_view *names,const int *used,int usedlen,const F &func) {
    vector<int> ind;
    for(int i=0;i<usedlen;i++) {
        int pos=used[i];
        if(!func(names[pos])) {
            LOG_DO(LOGGER("have %s\n", names[pos].data()));
            ind.push_back(pos);
            }
        else {
            LOG_DO(LOGGER("don't have %s\n", names[pos].data()));
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
            LOG_DO(LOGGER("mymounts=%s already present\n",mymounts));
            return true;
            }
        }
    }
    writeall(mymounts,mounts,sizeof(mounts));
    LOG_DO(LOGGER("mymounts=%s writeall\n",mymounts));
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
    pid_t *has_debugger;
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
arch    syscallNR    return    arg0    arg1    arg2    arg3    arg4    arg5
x86    eax        eax    ebx    ecx    edx    esi    edi    ebp
x86_64    rax        rax    rdi    rsi    rdx    r10    r8    r9
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

#ifndef TEST
static bool redirect_oracle_proc_path(char *name, Register &regi, int regnr, const char **action, const char **replacement) {
#if L3_ORACLE_NORMALIZE_PROC_TASK
    if(!name)
        return false;
    static constexpr char taskdir[]="/proc/self/task";
    static constexpr char taskprefix[]="/proc/self/task/";
    if(!strcmp(name, taskdir)) {
        ensure_oracle_proc_tree();
        regi.set(regnr,(Register::type)oracle_task_dir.c_str());
        regi.setall();
        if(action) *action="redirect-proc-task";
        if(replacement) *replacement=oracle_task_dir.c_str();
        return true;
    }
    if(!strncmp(name, taskprefix, sizeof(taskprefix)-1) && endswith_cstr(name,"/comm")) {
        ensure_oracle_proc_tree();
        regi.set(regnr,(Register::type)oracle_task_comm_path.c_str());
        regi.setall();
        if(action) *action="redirect-proc-task-comm";
        if(replacement) *replacement=oracle_task_comm_path.c_str();
        return true;
    }
#endif
    return false;
}
#endif

struct redir {
    int (*func)(void *arg) ;
    void *arg;
    bool thread;
    };

thread_local pid_t has_debugger=0;

extern pid_t getdebugclone(const int version) ;
int cloner(void *arg) {
//     doesnt=getdoesnt();
    redir *re=(redir *)arg;
    int (*func)(void *arg) =re->func;
    void *argu= re->arg;
    if(re->thread) {
        delete re;
        }
//    debugclone(false,3);
    pid_t pid=0;
    if(wrongfiles())
        pid=getdebugclone(3);
    int res=func(argu);
        if(pid) {
        getsid(pid);
        } 
    return res;
    }


static void    changeonly(const char *becomes,Register &regi,Register::type reg,const char **action=nullptr,const char **replacement=nullptr) {
    regi.set(reg,(Register::type)becomes);
    regi.setall();
    if(action)
        *action="redirect";
    if(replacement)
        *replacement=becomes;
    }
bool    change(const char *ptr,const char *was,const char *becomes,Register &regi,Register::type reg,const char **action=nullptr,const char **replacement=nullptr) {
    if(strcmp(ptr,was)) 
          return false;
    changeonly(becomes,regi,reg,action,replacement);
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
bool rewrongval(span<const string_view> files,bool *useds,pid_t pid,Register &regi,char *name ,int regnr=1,const char **action=nullptr) {
    const string_view *names=files.data(); 
    int len=files.size();
    string_view zoek{name,0};
    if(int pos=binary_find(names,len, zoek, verg);pos>=0) {
        useds[pos]=true;
        if(action)
            *action="blocked";
        *name='\0';
        return true;
        }
    if(action)
        *action="unblocked";
    return false;
    }
bool rewrong(span<const string_view> files,bool *useds,pid_t pid,Register &regi,int regnr=1,const char **action=nullptr) {
      const Register::type reg= regi.get(regnr);
    if(!reg) 
        return false;
    return  rewrongval(files,useds,pid,regi,(char *) reg,regnr,action); 
    }
#undef waitpid
#define waitpid(pid,wstatus,options)   wait4(pid, wstatus, options, nullptr)

#ifndef NOLOG
static constexpr int syscall_path_loglen=192;
static constexpr int syscall_bytes_loglen=200;

static const char *validcallstr(const char *callstr) {
    return callstr&&*callstr?callstr:"Unknown";
    }

static void copylogstr(char *out,int outlen,const char *in) {
    if(outlen<=0)
        return;
    if(!in) {
        out[0]='\0';
        return;
        }
    int pos=0;
    for(;pos+1<outlen&&in[pos];pos++) {
        const unsigned char ch=in[pos];
        out[pos]=(ch>=32&&ch<127)?static_cast<char>(ch):'?';
        }
    if(in[pos]&&outlen>4) {
        out[outlen-4]='.';
        out[outlen-3]='.';
        out[outlen-2]='.';
        out[outlen-1]='\0';
        }
    else
        out[pos]='\0';
    }

static char hexdigit(unsigned val) {
    val&=15;
    return val<10?static_cast<char>('0'+val):static_cast<char>('a'+val-10);
    }

static void copyhexbytes(char *out,int outlen,const char *in,int inlen) {
    if(outlen<=0)
        return;
    out[0]='\0';
    if(!in||inlen<=0)
        return;
    const int maxbytes=(outlen-1)/2;
    const int len=inlen<maxbytes?inlen:maxbytes;
    for(int pos=0;pos<len;pos++) {
        const unsigned char ch=static_cast<unsigned char>(in[pos]);
        out[pos*2]=hexdigit(ch>>4);
        out[pos*2+1]=hexdigit(ch);
        }
    out[len*2]='\0';
    }

static void logptr(slog &log,Register::type val) {
    log<<"0x"<<slbase(16)<<(uintptr_t)val<<slbase(10);
    }

static void logsize(slog &log,Register::type val) {
    log<<(uintptr_t)val;
    }


static bool syscallretiserror(Register::type ret) {
    constexpr uintptr_t maxerrno=4095;
    return (uintptr_t)ret >= ((uintptr_t)0 - maxerrno);
    }

static bool syscallretisptr(int syscallnr) {
    switch(syscallnr) {
#ifdef __NR_brk
        case __NR_brk:
            return true;
#endif
#if defined(__aarch64__) && defined(__NR_mmap)
        case __NR_mmap:
            return true;
#endif
#ifdef __NR_mmap2
        case __NR_mmap2:
            return true;
#endif
#ifdef __NR_mremap
        case __NR_mremap:
            return true;
#endif
#ifdef __NR_shmat
        case __NR_shmat:
            return true;
#endif
        default:
            return false;
        }
    }

static void logret(slog &log,int syscallnr,Register::type ret) {
    if(syscallretisptr(syscallnr) && !syscallretiserror(ret))
        logptr(log,ret);
    else
        log<<(intptr_t)ret;
    }

static bool ptrace_read_bytes(pid_t pid, uintptr_t addr, void *out, size_t len) {
    if(!out)
        return false;
    if(!len)
        return true;
    if(!addr)
        return false;

    constexpr size_t word = sizeof(long);
    const uintptr_t mask = static_cast<uintptr_t>(word - 1);
    uintptr_t wordaddr = addr & ~mask;
    size_t outpos = 0;
    size_t byteoff = static_cast<size_t>(addr - wordaddr);
    auto *dst = static_cast<unsigned char *>(out);

    while(outpos < len) {
        union { long val; unsigned char bytes[word]; } u{};
        errno=0;
        u.val = ptrace(PTRACE_PEEKDATA, pid, wordaddr, nullptr);
        if(errno)
            return false;

        const size_t chunk = std::min(word - byteoff, len - outpos);
        memcpy(dst + outpos, u.bytes + byteoff, chunk);
        outpos += chunk;
        wordaddr += word;
        byteoff = 0;
    }
    return true;
    }

static void logiovecarray(slog &log,pid_t pid,const char *ptrname,const char *cntname,const char *listname,Register::type iovaddr,Register::type iovcnt) {
    log<<ptrname<<"=";
    logptr(log,iovaddr);
    log<<", "<<cntname<<"=";
    logsize(log,iovcnt);
    log<<", "<<listname<<"=[";

    if(!iovaddr||!iovcnt) {
        log<<"]";
        return;
        }

    constexpr uintptr_t maxlogged=4;
    const uintptr_t count=std::min<uintptr_t>((uintptr_t)iovcnt,maxlogged);
    for(uintptr_t pos=0;pos<count;pos++) {
        if(pos)
            log<<", ";
        struct iovec vec{};
        const uintptr_t addr=(uintptr_t)iovaddr + pos*sizeof(struct iovec);
        if(!ptrace_read_bytes(pid,addr,&vec,sizeof(vec))) {
            log<<"?";
            break;
            }
        log<<"{base=";
        logptr(log,(Register::type)(uintptr_t)vec.iov_base);
        log<<", len=";
        logsize(log,(Register::type)vec.iov_len);
        log<<"}";
        }
    if((uintptr_t)iovcnt>maxlogged)
        log<<", ...";
    log<<"]";
    }

static void logloffptr(slog &log,pid_t pid,const char *name,Register::type addr) {
    log<<name<<"=";
    logptr(log,addr);
    if(!addr)
        return;
    long long value=0;
    if(ptrace_read_bytes(pid,(uintptr_t)addr,&value,sizeof(value)))
        log<<"->"<<value;
    }

static void lograwargs(slog &log,const Register::type *args) {
    for(int pos=0;pos<6;pos++) {
        if(pos)
            log<<", ";
        log<<"arg"<<pos<<"=";
        logptr(log,args[pos]);
        }
    }

static bool lognamedargs(slog &log,pid_t pid,int syscallnr,const Register::type *args,const char *patharg,const char *path) {
    switch(syscallnr) {
        case __NR_read:
            log<<"fd="<<(int)(intptr_t)args[0]<<", buf=";
            logptr(log,args[1]);
            log<<", len=";
            logsize(log,args[2]);
            return true;
#ifdef __NR_write
        case __NR_write:
            log<<"fd="<<(int)(intptr_t)args[0]<<", buf=";
            logptr(log,args[1]);
            log<<", len=";
            logsize(log,args[2]);
            return true;
#endif
#ifdef __NR_pread64
        case __NR_pread64:
            log<<"fd="<<(int)(intptr_t)args[0]<<", buf=";
            logptr(log,args[1]);
            log<<", len=";
            logsize(log,args[2]);
            log<<", off="<<(uint64_t)args[3];
            return true;
#endif
#ifdef __NR_pwrite64
        case __NR_pwrite64:
            log<<"fd="<<(int)(intptr_t)args[0]<<", buf=";
            logptr(log,args[1]);
            log<<", len=";
            logsize(log,args[2]);
            log<<", off="<<(uint64_t)args[3];
            return true;
#endif
#ifdef __NR_readv
        case __NR_readv:
            log<<"fd="<<(int)(intptr_t)args[0]<<", ";
            logiovecarray(log,pid,"iov","iovcnt","iov",args[1],args[2]);
            return true;
#endif
#ifdef __NR_writev
        case __NR_writev:
            log<<"fd="<<(int)(intptr_t)args[0]<<", ";
            logiovecarray(log,pid,"iov","iovcnt","iov",args[1],args[2]);
            return true;
#endif
#ifdef __NR_preadv
        case __NR_preadv:
            log<<"fd="<<(int)(intptr_t)args[0]<<", ";
            logiovecarray(log,pid,"iov","iovcnt","iov",args[1],args[2]);
            log<<", pos_l="<<(uint64_t)args[3]<<", pos_h="<<(uint64_t)args[4];
            return true;
#endif
#ifdef __NR_pwritev
        case __NR_pwritev:
            log<<"fd="<<(int)(intptr_t)args[0]<<", ";
            logiovecarray(log,pid,"iov","iovcnt","iov",args[1],args[2]);
            log<<", pos_l="<<(uint64_t)args[3]<<", pos_h="<<(uint64_t)args[4];
            return true;
#endif
#ifdef __NR_preadv2
        case __NR_preadv2:
            log<<"fd="<<(int)(intptr_t)args[0]<<", ";
            logiovecarray(log,pid,"iov","iovcnt","iov",args[1],args[2]);
            log<<", pos_l="<<(uint64_t)args[3]<<", pos_h="<<(uint64_t)args[4]<<", flags=";
            logptr(log,args[5]);
            return true;
#endif
#ifdef __NR_pwritev2
        case __NR_pwritev2:
            log<<"fd="<<(int)(intptr_t)args[0]<<", ";
            logiovecarray(log,pid,"iov","iovcnt","iov",args[1],args[2]);
            log<<", pos_l="<<(uint64_t)args[3]<<", pos_h="<<(uint64_t)args[4]<<", flags=";
            logptr(log,args[5]);
            return true;
#endif
#ifdef __NR_process_vm_readv
        case __NR_process_vm_readv:
            log<<"pid="<<(int)(intptr_t)args[0]<<", ";
            logiovecarray(log,pid,"local_iov","liovcnt","local",args[1],args[2]);
            log<<", ";
            logiovecarray(log,pid,"remote_iov","riovcnt","remote",args[3],args[4]);
            log<<", flags=";
            logptr(log,args[5]);
            return true;
#endif
#ifdef __NR_process_vm_writev
        case __NR_process_vm_writev:
            log<<"pid="<<(int)(intptr_t)args[0]<<", ";
            logiovecarray(log,pid,"local_iov","liovcnt","local",args[1],args[2]);
            log<<", ";
            logiovecarray(log,pid,"remote_iov","riovcnt","remote",args[3],args[4]);
            log<<", flags=";
            logptr(log,args[5]);
            return true;
#endif
        case __NR_close:
            log<<"fd="<<(int)(intptr_t)args[0];
            return true;
        case __NR_dup:
            log<<"oldfd="<<(int)(intptr_t)args[0];
            return true;
#ifdef __NR_dup2
        case __NR_dup2:
            log<<"oldfd="<<(int)(intptr_t)args[0]<<", newfd="<<(int)(intptr_t)args[1];
            return true;
#endif
#ifdef __NR_dup3
        case __NR_dup3:
            log<<"oldfd="<<(int)(intptr_t)args[0]<<", newfd="<<(int)(intptr_t)args[1]<<", flags=";
            logptr(log,args[2]);
            return true;
#endif
#ifdef __NR_lseek
        case __NR_lseek:
            log<<"fd="<<(int)(intptr_t)args[0]<<", off="<<(int64_t)args[1]<<", whence="<<(int)(intptr_t)args[2];
            return true;
#endif
#ifdef __NR_copy_file_range
        case __NR_copy_file_range:
            log<<"fd_in="<<(int)(intptr_t)args[0]<<", ";
            logloffptr(log,pid,"off_in",args[1]);
            log<<", fd_out="<<(int)(intptr_t)args[2]<<", ";
            logloffptr(log,pid,"off_out",args[3]);
            log<<", len=";
            logsize(log,args[4]);
            log<<", flags=";
            logptr(log,args[5]);
            return true;
#endif
        case __NR_getsid:
            log<<"pid="<<(int)(intptr_t)args[0];
            return true;
        case __NR_exit:
            log<<"status="<<(int)(intptr_t)args[0];
            return true;
        case __NR_prctl:
            log<<"option="<<(int)(intptr_t)args[0]<<", arg2=";
            logptr(log,args[1]);
            log<<", arg3=";
            logptr(log,args[2]);
            log<<", arg4=";
            logptr(log,args[3]);
            log<<", arg5=";
            logptr(log,args[4]);
            return true;
#ifdef __NR_open
        case __NR_open:
            log<<"path=\""<<(patharg?path:"")<<"\", flags=";
            logptr(log,args[1]);
            log<<", mode=";
            logptr(log,args[2]);
            return true;
#endif
        case __NR_openat:
            log<<"dirfd="<<(int)(intptr_t)args[0]<<", path=\""<<(patharg?path:"")<<"\", flags=";
            logptr(log,args[2]);
            log<<", mode=";
            logptr(log,args[3]);
            return true;
        case __NR_faccessat:
            log<<"dirfd="<<(int)(intptr_t)args[0]<<", path=\""<<(patharg?path:"")<<"\", mode=";
            logptr(log,args[2]);
            log<<", flags=";
            logptr(log,args[3]);
            return true;
#ifdef __NR_access
        case __NR_access:
            log<<"path=\""<<(patharg?path:"")<<"\", mode=";
            logptr(log,args[1]);
            return true;
#endif
#ifdef __NR_statx
        case __NR_statx:
            log<<"dirfd="<<(int)(intptr_t)args[0]<<", path=\""<<(patharg?path:"")<<"\", flags=";
            logptr(log,args[2]);
            log<<", mask=";
            logptr(log,args[3]);
            log<<", statxbuf=";
            logptr(log,args[4]);
            return true;
#endif
#ifdef __NR_newfstatat
        case __NR_newfstatat:
#else
        case __NR_fstatat64:
#endif
            log<<"dirfd="<<(int)(intptr_t)args[0]<<", path=\""<<(patharg?path:"")<<"\", statbuf=";
            logptr(log,args[2]);
            log<<", flags=";
            logptr(log,args[3]);
            return true;
#ifdef __NR_stat
        case __NR_stat:
#endif
#ifdef __NR_stat64
        case __NR_stat64:
#endif
#if defined(__NR_stat) || defined(__NR_stat64)
            log<<"path=\""<<(patharg?path:"")<<"\", statbuf=";
            logptr(log,args[1]);
            return true;
#endif
#ifdef __NR_ioprio_get
        case __NR_ioprio_get:
            log<<"which="<<(int)(intptr_t)args[0]<<", who="<<(int)(intptr_t)args[1];
            return true;
#endif
#ifdef __NR_pipe2
        case __NR_pipe2:
            log<<"pipefd=";
            logptr(log,args[0]);
            log<<", flags=";
            logptr(log,args[1]);
            return true;
#endif
#ifdef __NR_getrandom
        case __NR_getrandom:
            log<<"buf=";
            logptr(log,args[0]);
            log<<", len=";
            logsize(log,args[1]);
            log<<", flags=";
            logptr(log,args[2]);
            return true;
#endif
        case __NR_munmap:
            log<<"addr=";
            logptr(log,args[0]);
            log<<", len=";
            logsize(log,args[1]);
            return true;
#ifdef __NR_mremap
        case __NR_mremap:
            log<<"old_addr=";
            logptr(log,args[0]);
            log<<", old_len=";
            logsize(log,args[1]);
            log<<", new_len=";
            logsize(log,args[2]);
            log<<", flags=";
            logptr(log,args[3]);
            log<<", new_addr=";
            logptr(log,args[4]);
            return true;
#endif
#if defined(__aarch64__)
        case __NR_mmap:
            log<<"addr=";
            logptr(log,args[0]);
            log<<", len=";
            logsize(log,args[1]);
            log<<", prot=";
            logptr(log,args[2]);
            log<<", flags=";
            logptr(log,args[3]);
            log<<", fd="<<(int)(intptr_t)args[4]<<", off="<<(uintptr_t)args[5];
            return true;
#endif
#ifdef __NR_mmap2
        case __NR_mmap2:
            log<<"addr=";
            logptr(log,args[0]);
            log<<", len=";
            logsize(log,args[1]);
            log<<", prot=";
            logptr(log,args[2]);
            log<<", flags=";
            logptr(log,args[3]);
            log<<", fd="<<(int)(intptr_t)args[4]<<", pgoff="<<(uintptr_t)args[5];
            return true;
#endif
#ifdef __NR_clone
        case __NR_clone:
            log<<"flags=";
            logptr(log,args[0]);
            log<<", stack=";
            logptr(log,args[1]);
            log<<", parent_tid=";
            logptr(log,args[2]);
            log<<", tls=";
            logptr(log,args[3]);
            log<<", child_tid=";
            logptr(log,args[4]);
            return true;
#endif
        default:
            return false;
        }
    }

static void logsyscall(pid_t pid,int syscallnr,const char *callstr,const Register::type *args,Register::type ret,bool hasret,const char *patharg,const char *path,const char *action,const char *replacement,const char *bytes) {
    slog log;
    log<<pid<<" syscallnr "<<syscallnr<<" "<<validcallstr(callstr)<<"(";
    const bool namedargs=lognamedargs(log,pid,syscallnr,args,patharg,path);
    if(!namedargs) {
        lograwargs(log,args);
        if(patharg)
            log<<", "<<patharg<<"=\""<<path<<"\"";
        }
    if(replacement)
        log<<", replacement=\""<<replacement<<"\"";
    log<<")";
    if(hasret) {
        log<<"=";
        logret(log,syscallnr,ret);
        }
    if(action)
        log<<" "<<action;
    if(bytes&&bytes[0])
        log<<" data="<<bytes;
    log<<endl;
    }
#endif

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
    com->condVar.notify_one();               //don't use com after this (local variable in invocator);
}

int syscallwait(pid_t pid,pid_t *has_debugger) {
    int contsig=0;
    while(true) {
        if(ptrace(PTRACE_SYSCALL, pid, 0, contsig) == -1) {
#ifndef NOLOG
            {int ern=errno;
            slog log;
            log<<"PTRACE_SYSCALL "<<pid<<" failed: "<<strerror(ern)<<endl;
            };
#endif



        ptrace(PTRACE_DETACH, pid, 0, 0);
        return 5;
        }
        contsig=0;
        int status;
        if(waitpid(pid, &status, 0) == -1) {
            int waserrno=errno;
#ifndef NOLOG
        {
        slog log;
        log<<"2 waitpid "<<pid<<": "<<strerror(waserrno)<<endl;;
        };
#endif
        if (waserrno != ECHILD) {
            ptrace(PTRACE_DETACH, pid, 0, 0);
            return 2;
        }
        return 2;
        }
#ifdef SYSGOOD
        if(WIFEXITED(status)) {
         LOG_DO(LOGAR("WIFEXITED(status))"));
            return 1;
        }
        if(WIFSIGNALED(status)) {
#ifndef NOLOG
            slog log;
            log<<"WIFSIGNALED "<<pid<<" "<<WTERMSIG(status)<<endl;
#endif
            return 1;
        }
        if(!WIFSTOPPED(status)) {
#ifndef NOLOG
            slog log;
            log<<"unexpected wait status "<<pid<<" "<<status<<endl;
#endif
            return 1;
        }
        const auto sign=WSTOPSIG(status);
        if(sign & 0x80)
            return 0;
#ifndef NOLOG
    {
    slog log;
        log<<"non-syscall stop status "<<status<<" signal "<<sign<< " debugs="<<*has_debugger<<endl;;
    }
#endif
   if(sign==SIGSEGV) {

     *has_debugger=0;
#ifndef NOLOG
    {
    slog log;
        log<<"set has_debugger=0"<<endl;;
    }
#endif
      return 11;
      }
    /*
     * With PTRACE_O_TRACESYSGOOD set, syscall-stops are reported as
     * SIGTRAP|0x80.  A plain signal-delivery stop (for example the
     * SIGCHLD visible in strange.txt as status 4479 / signal 17) is not
     * a syscall entry or exit.  Treating it as one desynchronizes the
     * entry/exit pairing and makes later return values look like syscall
     * arguments (read fd=4020, fd=379, close fd=0, ...).  Resume the
     * tracee and wait for the next real syscall-stop instead.
     */
    contsig=sign;
    continue;
#else
    return 0;
#endif
    }
}

int debugger(void *arg) {
commu *com=reinterpret_cast<commu *>(arg);
pid_t pid=com->tid;
pid_t *has_debugger=com->has_debugger;
//debugtid=pid;
const pid_t ownpid= syscall(SYS_getpid);
#ifndef NOLOG
{slog log;
log<<"pid="<<ownpid<<" tid="<<syscall(SYS_gettid)<<" debugs "<<pid<<endl;
}
#endif

//    signal(SIGUSR1,sighandler);
//ptrace(PTRACE_SEIZE, pid, 0, 0);
if(ptrace(PTRACE_ATTACH, pid, 0, 0)==-1) {
#ifndef NOLOG
    {
    int waserrno=errno;
    slog log;
    log<<"PTRACE_ATTACH "<<pid<<" failed: "<<strerror(waserrno)<<endl;
    }
#endif
    notify(com); //don't block, crash
    return 3;
    }
//tgkill(com->pid, pid, SIGCONT);
//if(wait(NULL)==-1) {
//sleep(1);

if(waitpid(pid, 0, 0)==-1) {
    int waserrno=errno;
#ifndef NOLOG
    { slog log;
        log<<"1 waitpid "<<pid<< strerror(waserrno)<<endl;
    }
#endif
    if(waserrno!= ECHILD ) {
        ptrace(PTRACE_DETACH, pid, 0, 0);
        pid_t thepid=com->pid;
        notify(com);
        tgkill(thepid, pid, SIGCONT);
        return 1;
        }
        }
#ifdef SYSGOOD 
ptrace(PTRACE_SETOPTIONS, pid, 0, PTRACE_O_TRACESYSGOOD);
#endif

#ifdef LIBRE3
int version=com->version;
#endif
LOG_DO(LOGAR("voor notify"));
notify(com);
LOG_DO(LOGAR("na notify"));
#ifdef LIBRE3
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
   if((syserr=syscallwait(pid,has_debugger))) {
       return syserr;
       }
  }

//    struct user_regs_struct regs;
    if (!regi.getall()) {
#ifndef NOLOG
    {int waserrno=errno;
        slog log;
        log<<"PTRACE_GETREGSET "<<pid<<" to get registers: "<<strerror(waserrno)<<endl;
    };
#endif
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return 4;
    }
    int syscallnr = regi.callnr();
    Register::type args[6];
    for(int argpos=0;argpos<6;argpos++)
        args[argpos]=regi.get(argpos);
#ifndef NOLOG
    char logpath[syscall_path_loglen];
    logpath[0]='\0';
    char logbytes[syscall_bytes_loglen];
    logbytes[0]='\0';
    const char *patharg=nullptr;
    const char *action=nullptr;
    const char *replacement=nullptr;
    const char *callstr = syscallnr < std::size(syscallstr) ? syscallstr[syscallnr].data() : (
#ifdef __ARM_NR_BASE
            (syscallnr >= __ARM_NR_BASE && syscallnr < (__ARM_NR_BASE + std::size(armcallstr)))
            ? armcallstr[syscallnr - __ARM_NR_BASE].data() :
            #endif
            "Unknown");
#endif
    bool askdump = false;
    const auto reg0 = args[0];
#ifdef FIXED_URANDOM
    bool openurandom=false;
#endif
    switch (syscallnr) {
    #ifdef  __NR_mmap2
    case __NR_mmap2: {
//        int len=regi.get(1);
//        LOGGER("mmap2(0x%lx %d %ld %ld %ld %ld)\n",regi.get(0),len,regi.get(2),regi.get(3),regi.get(4),regi.get(5));
        break;
        }
#endif


#ifdef LIBRE3
        case __NR_clone: {
            unsigned long flags = args[0];
            if (followthreads) {
                if (flags & CLONE_THREAD) {
                    unsigned long **stack = (unsigned long **) args[1];
                    redir *re = new redir{(int (*)(void *)) *stack, (void *) stack[1],
                                          static_cast<bool>(flags & CLONE_THREAD)};
                    *stack = (unsigned long *) cloner;
                    stack[1] = (unsigned long *) re;
                    LOG_DO(action="clone-hooked");
                };
            }
        }
            break;
#endif
#ifdef __NR_getrandom
        case __NR_getrandom:
#if L3_ORACLE_DETERMINISTIC_GETRANDOM
            LOG_DO(action="getrandom-oracle-entry");
#endif
            break;
#endif
        case __NR_prctl:
            if (args[0] == PR_GET_DUMPABLE) {
                LOG_DO(action="PR_GET_DUMPABLE");
                askdump = true;
            }
            break;
        case __NR_getsid: {
            pid_t thepid = reg0;
//            LOGGER("getsid(%d) ownpid=%d\n", thepid, ownpid);
            if(thepid == ownpid) {
                LOG_DO(action="STOPSIGNAL");
                LOG_DO(logsyscall(pid,syscallnr,callstr,args,0,false,patharg,logpath,action,replacement,logbytes));
                if(istest)
                    saveused();
                if (ptrace(PTRACE_DETACH, pid, 0, 0) == -1) {
#ifndef NOLOG
                    slog log;
                    log<<"PTRACE_DETACH "<<pid<<" failed"<<endl;
#endif
                }
                return 0;
            }
        };break;


        case __NR_exit: {
            LOG_DO(action="exit");
            LOG_DO(logsyscall(pid,syscallnr,callstr,args,0,false,patharg,logpath,action,replacement,logbytes));
            if (ptrace(PTRACE_DETACH, pid, 0, 0) == -1) {
#ifndef NOLOG
        slog log; log<<"PTRACE_DETACH "<<pid<<" failed\n";
#endif
            }

            if (waitpid(pid, 0, 0) == -1) {
                int waserrno = errno;
#ifndef NOLOG
        {slog log;
        log <<"5 waitpid "<<pid<<endl;;
        }
#endif
                if (waserrno != ECHILD) {
                    return 12;
                }
            }
            return 14;
        };
            break;
        case __NR_close:
            break;
#ifdef LIBRE3
        case __NR_ioprio_get: { 
            if (args[1] == ownpid) {
                version = args[0];
                followthreads = version == 3 && wrongfiles();
                LOG_DO(action="libre3-version");
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
            LOG_DO(patharg="path");
            LOG_DO(copylogstr(logpath,syscall_path_loglen,(const char *)args[1]));
            rewrong(hierstat, statused, pid, regi, 1,LOG_ARGPTR(action));
            break;
#ifdef __NR_stat
        case __NR_stat:
#endif
#ifdef __NR_stat64
        case __NR_stat64:
#endif
#if defined( __NR_stat64) || defined( __NR_stat)
            LOG_DO(patharg="path");
            LOG_DO(copylogstr(logpath,syscall_path_loglen,(const char *)args[0]));
            rewrong(hierstat, statused, pid, regi, 0,LOG_ARGPTR(action));
            break;
#endif
        case __NR_faccessat:
            LOG_DO(patharg="path");
            LOG_DO(copylogstr(logpath,syscall_path_loglen,(const char *)args[1]));
            rewrong(hieraccess, accessused, pid, regi, 1,LOG_ARGPTR(action));
            break;


#ifdef __NR_access
        case __NR_access:
            LOG_DO(patharg="path");
            LOG_DO(copylogstr(logpath,syscall_path_loglen,(const char *)args[0]));
            rewrong(hieraccess, accessused, pid, regi, 0,LOG_ARGPTR(action));
            break;
#endif
#ifdef __NR_open
        case __NR_open:
            openreg = 0;
#endif
        case __NR_openat: {
            const Register::type reg1 = args[openreg];
            if (reg1) {
                static Register::type oldreg{};
                char *name = (char *) reg1;
                LOG_DO(patharg="path");
                LOG_DO(copylogstr(logpath,syscall_path_loglen,name));
                bool oracle_proc_redirected=false;
#ifndef TEST
                oracle_proc_redirected=redirect_oracle_proc_path(name, regi, openreg, LOG_ARGPTR(action), LOG_ARGPTR(replacement));
#endif
                if(oracle_proc_redirected) {
                }
                else if (reg1 != oldreg) {
#if LIBRE3 == 2
                int len=strlen(name);
                static  constexpr char apk[] = ".apk";
                static constexpr const int apklen=sizeof(apk)-1;
            if(version==3&&waspipe&&wasclone&&args[openreg+1]==0&&args[openreg+2]==0&&!memcmp(name+len-apklen,apk,apklen)) {
                    wasclone=waspipe=false;
                    changeonly(newapkname,regi, openreg);
                    LOG_DO(action="make base.apk");
                    libre3initialized=true;
                    
              }
else 
#endif
{

#ifdef FIXED_URANDOM
                 static  constexpr char urandom[] = "/dev/urandom";
                 if(change(name, urandom, myurandom, regi, openreg)) {
//                 if(!memcmp(name, urandom,sizeof(urandom))) {
                     openurandom=true;
                     LOG_DO(action="urandom");
                    } 
                else 
#endif
                {
                      static  constexpr char mounts[] = "/proc/self/mounts";
                        if(change(name, mounts, mymounts, regi, openreg,LOG_ARGPTR(action),LOG_ARGPTR(replacement))) {
                        } else {
    #ifdef CHANGESTATUS
                       static     constexpr char status[]="/proc/self/status";
                            if(change(name,status,mystatus,regi,openreg,LOG_ARGPTR(action),LOG_ARGPTR(replacement))) {
                              }
                          else
    #endif

                            {
                  if(seesnetunix&&!strcmp(name,netunix))  {
                    LOG_DO(action="blocked");
                    *name = '\0';
                    }
                else {
                        if(rewrongval(hieropen, openused, pid, regi, name, openreg,LOG_ARGPTR(action))) {
                        oldreg = (uintptr_t) name;
                        }
                        }
                            }
                        }
                    } 
                   }
                }
                    else {
                        LOG_DO(action="blocked-old");
                        *name = '\0';
                    }
            } else  {
            LOG_DO(action="null-path");
            }

        };

#ifdef __NR_open
            openreg = 1;
#endif
            break;
        case __NR_dup:
            break;
#if defined(__aarch64__) && defined(LIBRE3)
    case __NR_mmap: {
        int len=args[1];
        if(len==1089536) { //Otherwise crashes in munmap, but only when it is debugged. As you probably will understand later.
            LOG_DO(action="detach-on-mmap");
            LOG_DO(logsyscall(pid,syscallnr,callstr,args,0,false,patharg,logpath,action,replacement,logbytes));
            if(version==3) {
                if (ptrace(PTRACE_DETACH, pid, 0, 0) == -1) {
#ifndef NOLOG
                    int waserrno=errno;
                    slog log;
                    log<<"PTRACE_DETACH "<<pid<<" failed "<<strerror(waserrno)<<endl;
#endif
                    }
                else {
                    *has_debugger=0;
                    }
                return 0;
                }
            }
        break; 
        };
#endif

        case __NR_munmap:
            break;
    default:;
    };

   {int syserr;
       if((syserr=syscallwait(pid,has_debugger))) {
        return syserr;
        }
    }
    if(!regi.getall()) {
#ifndef NOLOG
        int waserrno=errno;
        {
        slog log;
        log<<"2 PTRACE_GETREGSET "<<pid<<" failed "<<strerror(waserrno)<<endl;
        };
#endif
        ptrace(PTRACE_DETACH, pid, 0, 0);
        return 6;
       }
    switch(syscallnr) {
#ifdef __NR_getrandom
        case __NR_getrandom:
#if L3_ORACLE_DETERMINISTIC_GETRANDOM
        {
            int siz=(int)(intptr_t)regi.ret();
            if(siz>0) {
                const uintptr_t outaddr=(uintptr_t)args[0];
                if(fill_tracee_with_oracle_bytes(pid, outaddr, (size_t)siz, LOG_BUF(logbytes), LOG_BUFLEN(syscall_bytes_loglen))) {
                    LOG_DO(action="getrandom-oracle");
                } else {
                    LOG_DO(action="getrandom-oracle-write-failed");
                }
            }
        }
#endif
            break;
#endif
        case  __NR_read:  
        {
        char *data=(char *)args[1];
        int siz=(int)(intptr_t)regi.ret();
        int fp=(int)args[0];
        constexpr const int readlen=80;
        if(siz>0)
            LOG_DO(copyhexbytes(logbytes,syscall_bytes_loglen,data,siz<readlen?siz:readlen));
        
#if LIBRE3 == 1
            if(version==3) {
                if(waspipe&&wasclone) {
                    int bufsize=args[2];
                    char *package=libre3lib.data();
                    int packagelen=libre3lib.size();
                    if(bufsize>=packagelen&&!memcmp(data,"package",7)) {
                        memcpy(data,package,packagelen);
                        waspipe=false;
                        wasclone=false;
                        LOG_DO(action="changed-package");
                        regi.setret(packagelen);
                        regi.setall();
                        libre3initialized=true;
                        }
                      }
#ifdef FIXED_URANDOM
                 else {
                    if(openedurandom==fp) {
                        openedurandom=-1;
                        LOG_DO(action="read-urandom");
                        }
                    }
#endif
            }
#endif
    }
            break;    
#ifdef LIBRE3
        case  __NR_clone: 
            if(version==3) {
                if(!(args[0]&CLONE_VM))  {
                wasclone=true;
                LOG_DO(action="clone-returned");
                }
                }
            break;
        case  __NR_pipe2:  {
            if(version==3) {
                waspipe=true;
                LOG_DO(action="pipe2-returned");
                }
            };break;
#endif
        case __NR_prctl:
            if(askdump) {
                LOG_DO(action="PR_GET_DUMPABLE forced-return-0");
                regi.setret(0);
                regi.setall();
                }
            break;

    //    case  __NR_clone: LOGAR("clone returned");

#ifndef NOLOG
#ifdef __NR_open
        case __NR_open:
#endif
#ifdef FIXED_URANDOM
        case __NR_openat: 
            if(openurandom) {
                const intptr_t fp=(intptr_t)regi.ret();
                if(fp>=0)
                    openedurandom=(int)fp;
                }
            break;
#endif
#endif
        };
#ifndef NOLOG
    LOG_DO(logsyscall(pid,syscallnr,callstr,args,regi.ret(),true,patharg,logpath,action,replacement,logbytes));
#endif
   }

}

static bool beforedebug() {
#if LIBRE3 == 1
 libre3lib=strsepconcat("",R"(package:)",libdirname,"/libinit.so\n");
 LOG_DO(LOGGER("libre3lib=%s",libre3lib.data()));
#elif LIBRE3 == 2
int liblen=libdirname.size();
const char libend[]="/libinit.so";
newapkname=new char[liblen+sizeof(libend)];
memcpy(newapkname,libdirname.data(),liblen);
memcpy(newapkname+liblen,libend,sizeof(libend));
#endif

       if(prctl(PR_SET_DUMPABLE, 1, 0, 0, 0) < 0)  {
            LOG_DO(LOGGER("could not set dumpable bit flag for pid %d: %s\n", getpid(), strerror(errno)));
        return false;
            }
        else {
           LOG_DO(LOGGER("dumpable bit flag set for pid %d\n", getpid()));
           struct rlimit rl;
#ifdef NDEBUG
           rl.rlim_cur = 0;
#else
           rl.rlim_cur = RLIM_INFINITY;
#endif
           rl.rlim_max = RLIM_INFINITY;
           if (setrlimit(RLIMIT_CORE, &rl) < 0) {
           LOG_DO(LOGGER("could not disable core file generation for pid %d: %s\n", getpid(),
                  strerror(errno)));
               }
            else {
                LOG_DO(LOGGER("Turned off core file generation for pid %d\n", getpid()));
                }
            return true;
            }
    }


static bool getstatus();
pid_t getdebugclone(const int version) {
LOG_DO(LOGAR("getdebugclone"));
#ifdef CHANGESTATUS
           __attribute__((used))  static bool hasstatus=getstatus();     
#endif
           __attribute__((used))  static bool hasmounts=mkmounts();     

    constexpr int STACK_SIZE (1024 * 1024);
    char *vstack=getmem(STACK_SIZE);
    if(!vstack) {
        LOG_DO(LOGAR("getmem(STACK_SIZE)==null"));
        return 0;
        }
    pid_t debugpid;
    pid_t mytid=syscall(SYS_gettid);
    commu com{.tid=mytid,.pid=static_cast<pid_t>(syscall(SYS_getpid)),.version=version,.has_debugger=&has_debugger};
    [[maybe_unused]] static bool isbeforedebug=beforedebug();
    LOG_DO(LOGAR("before clone"));
    if((debugpid=clone(debugger, vstack+STACK_SIZE, CLONE_VM|  CLONE_FILES | CLONE_FS | CLONE_IO|CLONE_UNTRACED , reinterpret_cast<void*>(&com))) == -1) { 
#ifndef NOLOG
        int older=errno;
        slog log;
        log<<"failed to spawn child task "<<strerror(older)<<endl;
#endif
        return 0;
        }
    std::unique_lock<std::mutex> lck(com.Mymutex);
    com.condVar.wait(lck, [&com]{ return com.debugReady; });
    LOG_DO(LOGGER(" got debugclone=%d\n",debugpid));
    return debugpid;
    }

class thedebugger {

public:
thedebugger(){
        }
~thedebugger() {
    const pid_t pid=has_debugger;
#ifndef NOLOG
    {
    slog log;
    log<<"~thedebugger pid="<<pid<<endl;
    }
#endif
     if(pid) {
        syscall(SYS_getsid,pid);
        }
    has_debugger=0;
    }

};
/*
void logfiles(decltype(hieraccess) fil) {
    for(auto el:fil) 
        LOG_DO(LOGGER("%s\n",el.data()));
    }
    */
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
extern        std::vector<std::string_view> usedfiles;
std::vector<std::string_view> usedfiles;

extern const std::string_view selfmounts;
const std::string_view selfmounts{"/proc/self/mounts"};
static bool mountsMagisk() {
    int handle=open(selfmounts.data(),O_RDONLY);
    if(handle<0) {
        LOG_DO(LOGGER("open %s failed\n",selfmounts.data()));
        return true;
        }
    constexpr const int maxbuf=4096;
    constexpr const char zoek[]="magisk";
    constexpr const int zoeklen=sizeof(zoek)-1;
    constexpr const int overlap=zoeklen-1;
    char buf[maxbuf+overlap];
    int start=0;
    int blnr=0;
    while(true) {
        int len=read(handle,buf+start,maxbuf);
        if(len<=0)  {
            LOG_DO(LOGGER("mountsMagisk read up to block %d\n",blnr));
            return false;
            }
        auto end=buf+start+len;
        if(std::search(buf,end,zoek,zoek+zoeklen)!=end) {
            return true;
            }
        memmove(buf,end-overlap,overlap);
        start=overlap;
        ++blnr;
        } 
    }
extern int dodebug(); 
static bool initialized=false;
bool magicmounts=false;
static bool needsdebug() {
    const int debug=dodebug();
    LOG_DO(LOGGER("dodebug()=%d\n",debug));
    if(debug>0) {
        istest=true;
        hieraccess={accessnames,std::size(accessnames)};
        hierstat={statnames,std::size(statnames)};
        hieropen={opennames,std::size(opennames)};
    }
    else {
    LOG_DO(LOGAR("Access:"));
  hieraccess=accessable(accessnames,
#ifndef USEDIN
      reuseaccess.fromfile(pathconcat(globalbasedir,"accessused")),reuseaccess.size()
#else
    reuseaccess->data(),reuseaccess->size()

#endif
        ,[](const  string_view view){
        const char *name=view.data();
        if(!access(name,F_OK)) {
            LOG_DO(LOGGER("access: %s\n",name));
            return 0;
            }
            /*
        int ern=errno;
        if(ern==EACCES)  {
            LOG_DO(LOGGER("access: %s: EACCES\n",name));
            return 0;
            } */
        const char ends[]="/.trdpx";
        const int len=view.size();
        char buf[sizeof(ends)+ len];
        memcpy(buf,name,len);
        memcpy(buf+len,ends,sizeof(ends));
        if(!access(buf,F_OK)) {
            LOG_DO(LOGGER("access: %s\n",buf));
            return 0;
            }
//        if(errno==EACCES||errno==ENOTDIR)
        if(errno==ENOTDIR) {
            LOG_DO(LOGGER("access: %s: ENOTDIR\n",buf));
            return 0;
            }
        return -1;
        });
        ;
        LOG_DO(LOGAR("Stat: "));
     hierstat=accessable(statnames,

#ifndef USEDIN
     reusestat.fromfile(pathconcat(globalbasedir,"statused")),reusestat.size()

#else
    reusestat->data(),reusestat->size()

#endif
     ,[](const string_view view){struct stat st;
        const char *name=view.data();
        if(!stat(name,&st)) {
            LOG_DO(LOGGER("stat %s\n",name));
            return 0;
            };
        if(errno==EACCES) {
            if(!memcmp(name,"/data/",6)) {
                LOG_DO(LOGGER("stat %s EACCES\n",name));
                return 0;
                }
            } 
        else {
            if(errno==ENOTDIR) {
                LOG_DO(LOGGER("stat %s ENOTDIR\n",name));
                return 0;
                }
            }
        const char ends[]="/.trdpx";
        const int len=view.size();
        char buf[sizeof(ends)+ len];
        memcpy(buf,name,len);
        memcpy(buf+len,ends,sizeof(ends));
        if(!stat(buf,&st)) {
            LOG_DO(LOGGER("stat %s\n",buf));
            return 0;
            };
        if(errno==ENOTDIR) {
            LOG_DO(LOGGER("stat %s ENOTDIR\n",buf));
            return 0;
            }
        return -1;
        });
    LOG_DO(LOGAR("Open:"));    
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
            LOG_DO(LOGGER("open %s\n",name));
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
                LOG_DO(LOGGER("open %s\n",buf));
                close(han);
                return 0;
                }
            if(errno==ENOTDIR) {
                LOG_DO(LOGGER("open %s ENOTDIR\n",buf));
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
        LOG_DO(LOGGER("debug %d files\n",totaal));
        int han= open(netunix,O_RDONLY);
        if(han>=0) {
            LOG_DO(LOGGER("can open %s\n",    netunix));
            close(han);
            seesnetunix=true;
            }
        else  {
            LOG_DO(LOGGER("can not open %s\n",    netunix));
            }
        if(!istest) {
            int firstlen=hieraccess.size()+hieropen.size();
            std::string_view tmp[firstlen];
            auto *endtmp=std::set_union(hieraccess.begin(),hieraccess.end(),hieropen.begin(),hieropen.end(),tmp);
            LOG_DO(LOGGER("tmp size=%zu\n",endtmp-tmp));
            usedfiles.clear();
            std::set_union(hierstat.begin(),hierstat.end(),tmp,endtmp,std::back_inserter(usedfiles));
            LOG_DO(LOGGER("usedfiles.size=%zu\n",usedfiles.size()));
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
    if((magicmounts=mountsMagisk())) {
        LOG_DO(LOGAR("mounts Magisk"));
        return true;
        }
    LOG_DO(LOGAR("nothing to debug"));
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
    LOG_DO(LOGGER("mystatus=%s\n",mystatus));
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

//    static bool wrong=((wrong=needsdebug()),wrongptr=&wrong,wrong);
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

//    if(!realyneeds&&debug<0) return false;
   if(version==2&&(dodebug()<0)) {
    LOG_DO(LOGGER("debugclone(%d,%d)=1\n",doalways,version));
       return 1;
    }
LOG_DO(LOGGER("debugclone(%d,%d)\n",doalways,version));
     if(wrongfiles()||doalways) {
#ifdef LIBRE3
    static thread_local    int wasversion ;
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
        #ifndef NOLOG
            int res=
        #endif
                      syscall( __NR_ioprio_get,3,has_debugger);
            LOG_DO(LOGGER("ioprio_get=%d\n",res));
            }
        }
#endif
    static thread_local    thedebugger debugger;
    LOG_DO(LOGAR("end debugclone"));
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

