#include <stdint.h>
#include <string_view>
#include <stdlib.h>
#include <dlfcn.h>
#ifdef __ANDROID_API__
#include <android/dlext.h>
#endif
#include <unistd.h>
#include <string>
#include <string_view>
#include "strconcat.hpp"
#include "logs.hpp"
#include "inout.hpp"
typedef struct android_namespace_t* (*android_get_exported_namespace_t)(const char*) ;


typedef struct android_namespace_t * (*android_create_namespace_t)(
    const char* name, const char* ld_library_path, const char* default_library_path, uint64_t type,
    const char* permitted_when_isolated_path, struct android_namespace_t* parent);

void (*ERR_print_errors_cbptr)(int (*cb)(const char *str, size_t len, void *u), void *u);
int logcallback(const char *str, size_t len, void *u) {
#ifndef NOLOG
	const char *format=(const char *)u;
	loggert(format,str);
#endif
	return 0;
	}

extern std::string_view globalbasedir;

#ifdef __ANDROID_API__
	#if defined(__aarch64__) || defined(__x86_64__) 
	const char *systembase[]={"/system/lib64"};
	#else
	const char *systembase[]={"/system/lib"};
	#endif
#else
	#if defined(__x86_64__) 
	const char *systembase[]={"/usr/lib/x86_64-linux-gnu","/usr/lib64","/usr/lib"};

	#elif defined(__i386__)
	const char *systembase[]={"/usr/lib/i386-linux-gnu","/usr/lib32","/usr/lib"};
	#elif defined(__aarch64__) 
    const char *systembase[]={"/usr/lib/aarch64-linux-gnu","/usr/lib64","/usr/lib"};
	#elif defined(__arm__)
	const char *systembase[]={"/usr/lib/arm-linux-gnueabihf","/usr/lib32","/usr/lib"};
    #elif defined(__powerpc)|| defined(__powerpc__)|| defined(__powerpc64__)|| defined(__POWERPC__)|| defined(__ppc__)|| defined(__ppc64__)|| defined(__PPC__)|| defined(__PPC64__)|| defined(_ARCH_PPC)|| defined(_ARCH_PPC64)
    const char *systembase[]={"/usr/lib/powerpc64le-linux-gnu","/usr/lib64","/usr/lib"};
    #elif defined(__hppa__)
    const char *systembase[]={"/usr/lib/riscv64-linux-gnu","/usr/lib64","/usr/lib"};
    #elif defined(__s390x__) || defined(__zarch__) ||defined(__s390__)
    const char *systembase[]={"/usr/lib/s390x-linux-gnu","/usr/lib64","/usr/lib"};
    #else
	const char *systembase[]={"/usr/lib","/usr/lib64","/usr/lib32"};
	#endif
#endif

#ifdef __ANDROID_API__
static constexpr const char cryptolib[]="libcrypto.so"            ;
static constexpr const char libssl[]="libssl.so"      ;
#endif
#ifdef JUGGLUCO_APP 
        static void rmlib(std::string_view filename) {
           pathconcat localname(globalbasedir,filename);
           const char *name=localname.data();
           if(unlink(name)) {
            flerror("unlink %s",name);
            }
           else
              LOGGER("unlink %s success",name);
           }
        void removelibs() {
          rmlib(cryptolib);
          rmlib(libssl);
           }
extern "C" void *bypass_loader_dlopen(const char *filename, int flag);
#define DLOPEN bypass_loader_dlopen
#else
        void removelibs() {
        }
#define DLOPEN dlopen
#endif
void * dlopener(std::string_view filename,int flags) {
#ifdef __ANDROID_API__
[[maybe_unused]]  static int isset=setenv("LD_LIBRARY_PATH",globalbasedir.data(), 1);
pathconcat localname(globalbasedir,filename);
if(void *handle=dlopen(localname.data(),flags)) {
	return handle;
    }
LOGGER("dlopen %s\n",dlerror());
#endif
#if !defined(__ANDROID_API__) || !defined(NOLOG)
constexpr const int maxerr=std::size(systembase)*300;
char errorm[maxerr];
int errpos=0;

#endif
for(const char *base:systembase) {
	pathconcat sysname(base,filename);
	if(void *handle=DLOPEN(sysname.data(),flags)) {
		LOGGER("dlopen %s\n", sysname.data());
		return handle;
		}
#if !defined(__ANDROID_API__) || !defined(NOLOG)
        errpos+=snprintf(errorm+errpos,maxerr-errpos, "%s\n",dlerror());
#endif
	}
#if defined(__ANDROID_API__) 
LOGGERN(errorm, errpos);
#else
write(STDERR_FILENO,errorm,errpos);
return nullptr;
#endif
#if defined(__ANDROID_API__) && __ANDROID_API__ >= 21
android_create_namespace_t android_create_namespace= (android_create_namespace_t)dlsym(RTLD_DEFAULT, "android_create_namespace");
if(android_create_namespace) {
	static const android_dlextinfo dlextinfo = {
	  .flags = ANDROID_DLEXT_USE_NAMESPACE,
	  .library_namespace = android_create_namespace(
	      "thisspace",
	      "/system/lib64/",
	      "/system/lib64/",
	      3,
	      "/system/:/data/:/vendor/",
	      NULL)
		};
	if(dlextinfo.library_namespace) { 
		if(void *handle=android_dlopen_ext(filename.data(), flags, &dlextinfo)) {
			return handle;
			}
		else
			LOGGER("android_dlopen_ext %s\n",dlerror());
		}
	else {
		LOGSTRING("android_create_namespace failed\n");
		}
	}
else {
	LOGGER("dlsym %s\n",dlerror());
	}
#endif	

#ifdef __ANDROID_API__
pathconcat sysname(systembase[0],filename);
Readall file(sysname.data());
if(!file) {
	flerror("%s\n",sysname.data());
	return nullptr;
	}
if(!writeall(localname.data(),file.data(),file.size())) {
	return nullptr;
	}

return dlopen(localname.data(),flags);
#else
return nullptr;
#endif

}


#ifdef __ANDROID_API__
extern void *opencrypto();
void *opencrypto() {
    LOGAR("opencrypto()");
     static void *cryptohandle=dlopener(cryptolib, RTLD_NOW);
     return cryptohandle;
   }

extern void *openssl();
void *openssl() {
     static void *handle=dlopener(libssl, RTLD_NOW);
     return handle;
     }
#endif
