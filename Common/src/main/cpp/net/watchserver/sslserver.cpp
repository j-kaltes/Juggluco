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
#include <sys/prctl.h>
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
	const char *format=(const char *)u;
	LOGGER(format,str);
	return 0;
	}

SSL_CTX *(*SSL_CTX_newptr)(const SSL_METHOD *method);
void (*ERR_print_errors_cbptr)(int (*cb)(const char *str, size_t len, void *u), void *u);

int (*SSL_CTX_use_certificate_ASN1ptr)(SSL_CTX *ctx, int len, unsigned char *d);
int (*SSL_CTX_use_PrivateKey_ASN1ptr)(int pk, SSL_CTX *ctx, unsigned char *d, long len);
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
	const char *systembase[]={"/usr/lib/x86_64-linux-gnu","/usr/lib64"};

	#else
	const char *systembase[]="/usr/lib/i386-linux-gnu","/usr/lib32"};
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
		LOGGER("android_create_namespace failed\n");
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



// Access granted


bool loadsslfunctions() {

#ifdef __ANDROID_API__
   setenv("LD_LIBRARY_PATH",globalbasedir.data(), 1);
#endif

const char cryptolib[]="libcrypto.so";
void* cryptohandle;
  if(!(cryptohandle=dlopener(cryptolib, RTLD_NOW))) {
	  LOGGER("dlopen==nullptr: %s\n",dlerror());
//	  dlclose(cryptohandle);
	  return false;
	  }
#define hgetsym(handle,name) *((void **)&name##ptr)=dlsym(handle, #name)
#define getsym(name) hgetsym(handle,name)
#define symtest(name) if(!(getsym(name))) { LOGGER("%s",dlerror());dlclose(handle);dlclose(cryptohandle);return false;}
if(!(hgetsym(cryptohandle,ERR_print_errors_cb))) {
	  dlclose(cryptohandle);
	  return false;
	}

const char libssl[]="libssl.so";
const char *libname=libssl;
  void *handle;
  if(!(handle=dlopener(libname, RTLD_NOW))) {
	  LOGGER("dlopen==nullptr: %s\n",dlerror());
	  return false;
	  }
  *((void **)&TheMethod)=dlsym(handle, "TLSv1_2_server_method");
 // *((void **)&TheMethod)=dlsym(handle, "TLS_method");
  if(!TheMethod) {
	  LOGGER("dlsym(TLSv1_2_server_method): %s\n",dlerror());
  	*((void **)&TheMethod)=dlsym(handle, "SSLv23_method");
  		if(!TheMethod) {
	  		LOGGER("dlsym(SSLv23_method): %s\n",dlerror());
			dlclose(handle);
			return false;
			}
  	}

//  *((void **)&SSL_library_init_ptr)=dlsym(handle, "SSL_library_init_ptr");
   getsym(SSL_library_init);
   getsym(OPENSSL_add_all_algorithms_noconf);
   getsym(SSL_load_error_strings);
//s/^.*([^a-zA-Z]*\([a-zA-Z_]*\)ptr.*$/symtest(\1);/g
symtest(SSL_CTX_new);
symtest(SSL_CTX_use_certificate_ASN1);
symtest(SSL_CTX_use_PrivateKey_ASN1);
symtest(SSL_CTX_check_private_key);
symtest(SSL_accept);
symtest(SSL_read);
symtest(SSL_write);
symtest(SSL_get_fd);
symtest(SSL_free);
symtest(SSL_new);
symtest(SSL_set_fd);
symtest(SSL_CTX_free);

return true;
}



extern std::string_view servererrorstr;

void	sslservererror(SSL *ssl) {
	SSL_writeptr(ssl,servererrorstr.data(),servererrorstr.size()) ;
	}
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
	bool watchcommands(char *rbuf,int len,recdata *outdata) ;
	if(!watchcommands(rbuf, len,&outdata) ) {
		sslservererror(ssl);
		return false;
		}
	return SSL_writeptr(ssl,outdata.data(),outdata.size()) ;
	}

SSL_CTX* initsslserver(void) ;
void handlewatchsecure(int sock) {
	static SSL_CTX *ctx=initsslserver();
	if(!ctx)
		return;
      const char threadname[]="secure watchconnect";
      prctl(PR_SET_NAME, threadname, 0, 0, 0);
      LOGGER("handlewatchsecure %d\n",sock);

        SSL *ssl=SSL_newptr(ctx);  
	if(!ssl)
		return;
        SSL_set_fdptr(ssl, sock); 
    if ( SSL_acceptptr(ssl)<0)   { 
        sslerror("SSL_accept: %s");
    	SSL_freeptr(ssl);  
	return;
	}
bool	securewatchcommands(SSL *ssl);
	while(securewatchcommands(ssl))
		;
    	SSL_freeptr(ssl);  
	close(sock);
	}


SSL_CTX* initsslserver(void) {
	LOGGER("initsslserver\n");
    if(SSL_library_initptr) SSL_library_initptr();
    SSL_CTX *ctx;
    if(OPENSSL_add_all_algorithms_noconfptr) OPENSSL_add_all_algorithms_noconfptr();
    if(SSL_load_error_stringsptr) SSL_load_error_stringsptr();  

     ctx = SSL_CTX_newptr(TheMethod());
    if ( ctx == NULL ) {
    	sslerror("SSL_CTX_newptr %s\n");
		return nullptr;
		}

    destruct des{[ctx] {SSL_CTX_freeptr(ctx);  }}; 
#include "servercrt.h"
    if ( SSL_CTX_use_certificate_ASN1ptr(ctx, sizeof(servercrt), (unsigned char*)servercrt) <= 0 )
    {
	sslerror("SSL_CTX_use_certificate_ASN1 %s\n");
	return nullptr;
    }
#include "serverkey.h"
    if( SSL_CTX_use_PrivateKey_ASN1ptr(EVP_PKEY_RSA, ctx, (unsigned char *)serverkey, sizeof(serverkey))<=0)
    {
	sslerror("SSL_CTX_use_PrivateKey_ASN1 failed %s\n");
	return nullptr;
    }
    if ( !SSL_CTX_check_private_keyptr(ctx) )
    {
        sslerror("Private key does not match the public certificate %s");
	return nullptr;
    }
   des.active=false;
    return ctx;
   }
#endif
