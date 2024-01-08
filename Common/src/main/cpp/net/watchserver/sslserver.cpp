
#ifndef WEAROS
char fullchainfileonly[]="fullchain.pem";
char privatekey[]="privkey.pem";
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
/*      Fri Jan 27 12:38:28 CET 2023                                                 */


#ifdef USE_SSL
#include <stdlib.h>
#include <dlfcn.h>
#ifdef __ANDROID_API__
#include <android/dlext.h>
#endif
#include <string>
#include <string_view>
#include <sys/prctl.h>
#include <strconcat.h>
#include "openssl/ssl.h"
#include "openssl/err.h"
#include "logs.h"
#include "inout.h"
#include "watchserver.h"
#include "destruct.h"
const SSL_METHOD *(*TheMethod)(void);
int (*SSL_library_initptr)(void)=NULL;
void (*OPENSSL_add_all_algorithms_noconfptr)(void)=NULL;

void (*SSL_load_error_stringsptr)(void)=NULL;

int logcallback(const char *str, size_t len, void *u) {
#ifndef NOLOG
	const char *format=(const char *)u;
	loggert(format,str);
#endif
	return 0;
	}

SSL_CTX *(*SSL_CTX_newptr)(const SSL_METHOD *method);
void (*ERR_print_errors_cbptr)(int (*cb)(const char *str, size_t len, void *u), void *u);

int (*SSL_CTX_use_certificate_chain_fileptr)(SSL_CTX *ctx, const char *file);

//int (*SSL_CTX_use_certificate_ASN1ptr)(SSL_CTX *ctx, int len, unsigned char *d);
//int (*SSL_CTX_use_PrivateKey_ASN1ptr)(int pk, SSL_CTX *ctx, unsigned char *d, long len);
int (*SSL_CTX_use_PrivateKey_fileptr)(SSL_CTX *ctx, const char *file, int type);
//const char* (*ERR_reason_error_stringptr)(unsigned long e);
//unsigned long (*ERR_get_errorptr)(void);
//unsigned long (*ERR_peek_last_errorptr)(void);


int (*SSL_CTX_check_private_keyptr)(const SSL_CTX *ctx);
int (*SSL_acceptptr)(SSL *ssl);
int (*SSL_readptr)(SSL *ssl, void *buf, int num);
int (*SSL_writeptr)(SSL *ssl, const void *buf, int num);
int (*SSL_get_fdptr)(const SSL *ssl);
void (*SSL_freeptr)(SSL *ssl);
SSL *(*SSL_newptr)(SSL_CTX *ctx);
int (*SSL_set_fdptr)(SSL *ssl, int fd);
void (*SSL_CTX_freeptr)(SSL_CTX *ctx);

void  sslerror(const char *format) {
	ERR_print_errors_cbptr(logcallback,(void*)format);
	}


/*
struct android_namespace_t *ns =
    android_create_namespace(
      "trustme",
      "/system/lib64/",
      "/system/lib64/",
      ANDROID_NAMESPACE_TYPE_SHARED |
      ANDROID_NAMESPACE_TYPE_ISOLATED,
      "/system/:/data/:/vendor/",
      NULL);

*/




typedef struct android_namespace_t* (*android_get_exported_namespace_t)(const char*) ;


typedef struct android_namespace_t * (*android_create_namespace_t)(
    const char* name, const char* ld_library_path, const char* default_library_path, uint64_t type,
    const char* permitted_when_isolated_path, struct android_namespace_t* parent);

/*
      "trustme",
      "/system/lib64/",
      "/system/lib64/",
      ANDROID_NAMESPACE_TYPE_SHARED |
      ANDROID_NAMESPACE_TYPE_ISOLATED,
      "/system/:/data/:/vendor/",
      NULL); */

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
	const char *systembase[]="/usr/lib/i386-linux-gnu","/usr/lib32","/usr/lib"};
	#else
	const char *systembase[]="/usr/lib"};
	#endif
#endif
void * dlopener(std::string_view filename,int flags) {
#ifdef __ANDROID_API__
pathconcat localname(globalbasedir,filename);

if(void *handle=dlopen(localname.data(),flags))
	return handle;
LOGGER("dlopen %s\n",dlerror());
#endif
for(const char *base:systembase) {
	pathconcat sysname(base,filename);
	if(void *handle=dlopen(sysname.data(),flags)) {
		LOGGER("dlopen %s\n", sysname.data());
		return handle;
		}
	LOGGER("%s: %s\n",sysname.data(),dlerror());
	}
#if __ANDROID_API__ >= 21
android_create_namespace_t android_create_namespace= (android_create_namespace_t)dlsym(RTLD_DEFAULT, "android_create_namespace");
if(android_create_namespace) {
	static const android_dlextinfo dlextinfo = {
	  .flags = ANDROID_DLEXT_USE_NAMESPACE,
//	  .library_namespace = android_get_exported_namespace("vndk")
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




//char fullchainfileonly[]="bundle_chained.crt";
//char privatekey[]="private.key";
static pathconcat chainfilename;
static pathconcat private_file;
static bool getkeynames() {
	 chainfilename=pathconcat(globalbasedir,fullchainfileonly);
	 private_file=pathconcat(globalbasedir,privatekey);
	 LOGAR("getkeynames");
	 return true;
	}
std::string haskeyfiles() {
[[maybe_unused]] static auto _hasnames=getkeynames();
 if(access(chainfilename.data(), R_OK)!=0) {
	return std::string(fullchainfileonly)+std::string(" missing");
	}
 if(access(private_file.data(), R_OK)!=0) {
	return std::string(privatekey)+std::string(" missing");

	}
	return "";
	}
std::string loadsslfunctions() {


#ifdef __ANDROID_API__
   setenv("LD_LIBRARY_PATH",globalbasedir.data(), 1);
#endif

const char cryptolib[]="libcrypto.so";
void* cryptohandle;
  if(!(cryptohandle=dlopener(cryptolib, RTLD_NOW))) {
	  return  std::string("dlopen==nullptr: ")+std::string(dlerror());
//	  dlclose(cryptohandle);
	  }
#define hgetsym(handle,name) *((void **)&name##ptr)=dlsym(handle, #name)
#define getsym(name) hgetsym(handle,name)
#define symtest(name) if(!(getsym(name))) { dlclose(handle);dlclose(cryptohandle);return std::string(dlerror());;}
if(!(hgetsym(cryptohandle,ERR_print_errors_cb))) {
	  dlclose(cryptohandle);
	  return std::string("hgetsym ERR_print_errors_cb fails");
	}

const char libssl[]="libssl.so";
const char *libname=libssl;
  void *handle;
  if(!(handle=dlopener(libname, RTLD_NOW))) {
	  return std::string("dlopen==nullptr: ")+std::string(dlerror());
	  }
  *((void **)&TheMethod)=dlsym(handle, "TLSv1_2_server_method");
 // *((void **)&TheMethod)=dlsym(handle, "TLS_method");
  if(!TheMethod) {
	  LOGGER("dlsym(TLSv1_2_server_method): %s\n",dlerror());
  	*((void **)&TheMethod)=dlsym(handle, "SSLv23_method");
  		if(!TheMethod) {
			dlclose(handle);
	  		return std::string("dlsym(SSLv23_method): ")+std::string(dlerror());
			}
  	}

//  *((void **)&SSL_library_init_ptr)=dlsym(handle, "SSL_library_init_ptr");
   getsym(SSL_library_init);
   getsym(OPENSSL_add_all_algorithms_noconf);
   getsym(SSL_load_error_strings);
//s/^.*([^a-zA-Z]*\([a-zA-Z_]*\)ptr.*$/symtest(\1);/g
symtest(SSL_CTX_new);
symtest(SSL_CTX_use_certificate_chain_file);
//symtest(SSL_CTX_use_certificate_ASN1);
//symtest(SSL_CTX_use_PrivateKey_ASN1);
symtest(SSL_CTX_use_PrivateKey_file);
symtest(SSL_CTX_check_private_key);
symtest(SSL_accept);
symtest(SSL_read);
symtest(SSL_write);
symtest(SSL_get_fd);
symtest(SSL_free);
symtest(SSL_new);
symtest(SSL_set_fd);
symtest(SSL_CTX_free);
/*
if(!(getsym(ERR_reason_error_string))) {
	LOGGER("%s ",dlerror());
	}

if(!(getsym(ERR_peek_last_error))) {
	LOGGER("%s ",dlerror());
	}
*/
return "";
}



extern std::string_view servererrorstr;

void	sslservererror(SSL *ssl) {
	SSL_writeptr(ssl,servererrorstr.data(),servererrorstr.size()) ;
	}
extern bool sslstopconnection;
bool sslstopconnection=false;
bool securewatchcommands(SSL *ssl) {
	constexpr const int RBUFSIZE=4096;
	char rbuf[RBUFSIZE];
	int len;
	if((len= SSL_readptr(ssl,rbuf,RBUFSIZE))<=0) {
		sslservererror(ssl);
		return false;
		}
	LOGGER("securewatchcommands %d\n",len);
	struct recdata outdata;
	if(sslstopconnection)
		return false;
bool watchcommands(char *rbuf,int len,recdata *outdata,bool secure) ;
/*
	if(!watchcommands(rbuf, len,&outdata,true) ) {
		sslservererror(ssl);
		return false;
		}
	return SSL_writeptr(ssl,outdata.data(),outdata.size())&&!sslstopconnection;
; */
	bool res=watchcommands(rbuf, len,&outdata,true);
	bool res2= SSL_writeptr(ssl,outdata.data(),outdata.size());
	return res&&res2&&!sslstopconnection;
	} 

static SSL_CTX *globalctx=nullptr;

void handlewatchsecure(int sock) {
	static SSL_CTX *ctx=globalctx;
	if(!ctx)
		return;
      const char threadname[17]="ssl watchconnect";
      prctl(PR_SET_NAME, threadname, 0, 0, 0);
      LOGGER("handlewatchsecure %d\n",sock);

        SSL *ssl=SSL_newptr(ctx);  
	if(!ssl)
		return;

extern void sendtimeout(int sock,int secs);
extern void receivetimeout(int sock,int secs) ;
 	receivetimeout(sock,60);
 	sendtimeout(sock,5*60);
        SSL_set_fdptr(ssl, sock); 
    if ( SSL_acceptptr(ssl)<0)   { 
        sslerror("SSL_accept: %s");
    	SSL_freeptr(ssl);  
	return;
	}
bool	securewatchcommands(SSL *ssl);
	securewatchcommands(ssl);
    	SSL_freeptr(ssl);  
	close(sock);
	}

 #include <openssl/err.h>


struct call_data_t{
	std::string_view start;
	std::string back;
	} ;
int geterrorcallback(const char *str, size_t len, void *u) {
	call_data_t *dptr=(call_data_t *)u;
	int startlen =dptr->start.size();
	int totlen=startlen+len+3;
	dptr->back=std::string(totlen,0);
	char *uit=dptr->back.data();
	memcpy(uit,dptr->start.data(),startlen);
	uit+=startlen;
	*uit++=':';
	*uit++=' ';
	memcpy(uit,str,len);
	uit[len]='\0';
	return 0;
	}
std::string geterror(std::string_view start) {
	call_data_t data;
	data.start=start;
	ERR_print_errors_cbptr(geterrorcallback,(void*)&data);
	return data.back;
	}
//static SSL_CTX* 
/*
const char *geterror() {
	if(ERR_reason_error_stringptr)
		 return ERR_reason_error_stringptr(ERR_peek_last_errorptr());
	else return "openSSL error";
	} */

const std::string initsslserver(void) {
    LOGSTRING("initsslserver\n");
	if(globalctx!=nullptr)
		SSL_CTX_freeptr(globalctx);
	globalctx=nullptr;
 class init{
	 public:
		init() {
		    if(SSL_library_initptr) SSL_library_initptr();
		    if(OPENSSL_add_all_algorithms_noconfptr) OPENSSL_add_all_algorithms_noconfptr();
		    if(SSL_load_error_stringsptr) SSL_load_error_stringsptr();  
		    } 
	    };
 static init _init;

    SSL_CTX *ctx = SSL_CTX_newptr(TheMethod());
    if( ctx == NULL ) {
		return geterror("SSL_CTX_newptr");
		}

    destruct des{[ctx] {SSL_CTX_freeptr(ctx);  }};
    
if(SSL_CTX_use_certificate_chain_fileptr(ctx,chainfilename.data())<=0)  {
	return geterror(chainfilename);
	}
if(SSL_CTX_use_PrivateKey_fileptr(ctx, private_file.data(), SSL_FILETYPE_PEM) <= 0) {
	return geterror(private_file.data());
    }
    if( !SSL_CTX_check_private_keyptr(ctx) ) {
	return geterror("Private key does not match the public certificate");
    }
    des.active=false;
    globalctx=ctx;
	return "";
   }

#endif
#endif
