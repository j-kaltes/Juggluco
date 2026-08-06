#include "logs.hpp"
extern int logcallback(const char *str, size_t len, void *u) ;
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
#include <errno.h>
#include <dlfcn.h>
#ifdef __ANDROID_API__
#include <android/dlext.h>
#define DLSYMS_SSL 1
#endif
#include <unistd.h>

#include <string>
#include <string_view>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#ifndef HAVE_NOPRCTL
#include <sys/prctl.h>
#endif
#include "strsepconcat.hpp"
#include "inout.hpp"
#include "watchserver.hpp"
#include "destruct.hpp"
#ifdef DLSYMS_SSL
#include "openssl/ssl.h"
#include "openssl/err.h"
#undef SSLv23_method
static const SSL_METHOD *(*TheMethod)(void);
static int (*SSL_library_initptr)(void)=NULL;
static void (*OPENSSL_add_all_algorithms_noconfptr)(void)=NULL;
static void (*SSL_load_error_stringsptr)(void)=NULL;
static SSL_CTX *(*SSL_CTX_newptr)(const SSL_METHOD *method);
static int (*SSL_CTX_use_certificate_chain_fileptr)(SSL_CTX *ctx, const char *file);
static int (*SSL_CTX_use_PrivateKey_fileptr)(SSL_CTX *ctx, const char *file, int type);
static int (*SSL_CTX_check_private_keyptr)(const SSL_CTX *ctx);
static int (*SSL_acceptptr)(SSL *ssl);
static int (*SSL_readptr)(SSL *ssl, void *buf, int num);
static int (*SSL_writeptr)(SSL *ssl, const void *buf, int num);
static int (*SSL_get_fdptr)(const SSL *ssl);
static void (*SSL_freeptr)(SSL *ssl);
static SSL *(*SSL_newptr)(SSL_CTX *ctx);
static int (*SSL_set_fdptr)(SSL *ssl, int fd);
static void (*SSL_CTX_freeptr)(SSL_CTX *ctx);
static int (*SSL_shutdownptr)(SSL *ssl);
static    unsigned long (*ERR_get_errorptr)(void);
static void (*ERR_error_string_nptr)(unsigned long e, char *buf, size_t len);

extern void (*ERR_print_errors_cbptr)(int (*cb)(const char *str, size_t len, void *u), void *u);


extern int (*SSL_get_errorptr)(const SSL *ssl, int ret);

#else
#include <openssl/ssl.h>
#include <openssl/err.h>

#define TheMethod SSLv23_method
#define SSL_library_initptr SSL_library_init
#define OPENSSL_add_all_algorithms_noconfptr OPENSSL_add_all_algorithms_noconf
#define SSL_load_error_stringsptr SSL_load_error_strings
#define SSL_CTX_newptr SSL_CTX_new
#define SSL_CTX_use_certificate_chain_fileptr SSL_CTX_use_certificate_chain_file
#define SSL_CTX_use_PrivateKey_fileptr SSL_CTX_use_PrivateKey_file
#define SSL_CTX_check_private_keyptr SSL_CTX_check_private_key
#define SSL_acceptptr SSL_accept
#define SSL_readptr SSL_read
#define SSL_writeptr SSL_write
#define SSL_get_fdptr SSL_get_fd
#define SSL_freeptr SSL_free
#define SSL_newptr SSL_new
#define SSL_set_fdptr SSL_set_fd
#define SSL_CTX_freeptr SSL_CTX_free
#define ERR_print_errors_cbptr ERR_print_errors_cb
#define SSL_shutdownptr  SSL_shutdown
#define ERR_get_errorptr ERR_get_error
#define ERR_error_string_nptr ERR_error_string_n
#define SSL_get_errorptr SSL_get_error
#endif

void  sslerror(const char *format) {
	ERR_print_errors_cbptr(logcallback,(void*)format);
	}


extern std::string_view globalbasedir;




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

extern void *opencrypto();
extern void *openssl();
extern void * dlopener(std::string_view filename,int flags);
std::string loadsslfunctions() {
#ifdef DLSYMS_SSL
   #ifndef  __ANDROID_API__
   char cryptolib[]="libcrypto.so.3";
   void* cryptohandle;
   if(!(cryptohandle=dlopener(cryptolib, RTLD_NOW))&&(cryptolib[12]='\0', !(cryptohandle=dlopener(cryptolib, RTLD_NOW)))) {
         cryptolib[12]='.';
        return  std::string("dlopen==nullptr: ");
        }
   #else
   void* cryptohandle=opencrypto();
   if(!cryptohandle) {
        return  std::string("dlopen==nullptr: ");
        }
   #endif
   #define hgetsym(handle,name) *((void **)&name##ptr)=dlsym(handle, #name)
   #define getsym(name) hgetsym(handle,name)
   #define symtest(name) if(!(getsym(name))) { dlclose(handle);dlclose(cryptohandle);return std::string(dlerror());;}
   if(!(hgetsym(cryptohandle,ERR_print_errors_cb))) {
        dlclose(cryptohandle);
        return std::string("hgetsym ERR_print_errors_cb fails");
      }
   if(!(hgetsym(cryptohandle,ERR_get_error))) {
        dlclose(cryptohandle);
        return std::string("hgetsym ERR_get_error fails");
      }
   if(!(hgetsym(cryptohandle,ERR_error_string_n))) {
        dlclose(cryptohandle);
        return std::string("hgetsym ERR_error_string_n fails");
      }
#ifndef __ANDROID_API__
   char libssl[]="libssl.so.3";
   const char *libname=libssl;
     void *handle;
     if(!(handle=dlopener(libname, RTLD_NOW))&&(libssl[9]='\0',!(handle=dlopener(libname, RTLD_NOW)))) {
         libssl[9]='.';
        return std::string("dlopen==nullptr: ");
        }
#else
     void *handle=openssl();
     if(!handle) {
        return std::string("dlopen==nullptr: ");
        }
#endif
     *((void **)&TheMethod)=dlsym(handle, "TLSv1_2_server_method");
     if(!TheMethod) {
     #ifndef NOLOG
        const char *error=dlerror();
        LOGGER("dlsym(TLSv1_2_server_method): %s\n",error?error:"?");
    #endif
      *((void **)&TheMethod)=dlsym(handle, "SSLv23_method");
         if(!TheMethod) {
            const char *error=dlerror();
            dlclose(handle);
            return std::string("dlsym(SSLv23_method): ")+(error?std::string(error):""s);
            }
      }

   getsym(SSL_library_init);
   getsym(OPENSSL_add_all_algorithms_noconf);
   getsym(SSL_load_error_strings);
   symtest(SSL_CTX_new);
   symtest(SSL_CTX_use_certificate_chain_file);
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
   symtest(SSL_shutdown);
   symtest(SSL_get_error);
#endif
   return "";
 }



extern std::string_view servererrorstr;

void	sslservererror(SSL *ssl) {
	SSL_writeptr(ssl,servererrorstr.data(),servererrorstr.size()) ;
	}
extern bool sslstopconnection;
bool sslstopconnection=false;

static bool sslwriteall(SSL *ssl,const char *data,int len) {
	int left=len;
	const char *iter=data;
	while(left>0) {
		const int wrote=SSL_writeptr(ssl,iter,left);
		if(wrote<=0) {
			const int error=SSL_get_errorptr(ssl,wrote);
			if(error==SSL_ERROR_WANT_READ||error==SSL_ERROR_WANT_WRITE)
				continue;
			if(error==SSL_ERROR_SYSCALL&&errno==EINTR)
				continue;
			LOGGER("SSL_write failed error=%d\n",error);
			return false;
			}
		iter+=wrote;
		left-=wrote;
		}
	return true;
	}

static bool securelivewrite(void *context,const char *data,int len) {
	return sslwriteall(static_cast<SSL *>(context),data,len);
	}

static bool securelivestopped(void *) {
	return sslstopconnection;
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
	if(sslstopconnection)
		return false;
        extern void wakesender();
        wakesender();
        bool watchcommands(char *rbuf,int len,recdata *outdata,bool secure) ;
	bool res=watchcommands(rbuf, len,&outdata,true);
	bool res2=sslwriteall(ssl,outdata.data(),outdata.size());
	const bool livestream=outdata.livestream;
	const livestreamoptions streamoptions=outdata.streamoptions;
   LOGGER("securewatchcommands: delete outdata.allbuf=%p\n",outdata.allbuf);
    delete[] outdata.allbuf;
	if(res&&res2&&livestream)
		res2=streamlive(ssl,securelivewrite,securelivestopped,streamoptions);
	return res&&res2&&!sslstopconnection;
	} 

static SSL_CTX *globalctx=nullptr;

bool	securewatchcommands(SSL *ssl);
extern void sendtimeout(int sock,int secs);
extern void receivetimeout(int sock,int secs) ;

void handlewatchsecure(int sock) {
	static SSL_CTX *ctx=globalctx;
	if(!ctx) {
       shutdown(sock,SHUT_RDWR);
       close(sock);
		return;
        }
   const char threadname[17]="ssl watchconnect";
#ifndef HAVE_NOPRCTL
   prctl(PR_SET_NAME, threadname, 0, 0, 0);
#endif
   LOGGER("handlewatchsecure %d\n",sock);
   SSL *ssl=SSL_newptr(ctx);  
	if(!ssl) {
       shutdown(sock,SHUT_RDWR);
       close(sock);
	   return;
       }
   destruct _des{[sock,ssl]{
        shutdown(sock,SHUT_RDWR);
        close(sock);
        SSL_freeptr(ssl);  
        LOGGER("handlewatchsecure close(%d)\n",sock);
        }};
   receivetimeout(sock,60);
   sendtimeout(sock,5*60);
   SSL_set_fdptr(ssl, sock); 
   if(SSL_acceptptr(ssl)<0)   { 
      sslerror("SSL_accept: %s");
      return;
      }
	securewatchcommands(ssl);
   SSL_writeptr(ssl, "", 0);
  for(int i=0;i<10;++i) { 
       int outq = 0;
        if(ioctl(sock, TIOCOUTQ, &outq) == 0) {
            LOGGER("SIOCOUTQ = %d bytes\n", outq);
            if(outq==0)
                break;
            }
        else {
            flerror("ioctl(sock, TIOCOUTQ, &outq)): ");
            break;
            }
       
        usleep(500000);
        }
   /*for(int i=0;i<5;++i) {
       int res=SSL_shutdownptr(ssl);
       LOGGER("SSL_shutdown=%d\n",res);
       if(res) {
           if(res==-1) {
                unsigned long err;
                while ((err = ERR_get_errorptr()) != 0) {
                    char buf[256];
                    ERR_error_string_nptr(err, buf, sizeof(buf));
                    LOGGER("OpenSSL error: %s (0x%lx)\n", buf, err);
                    }
                }
             break;
             }
         usleep(1000*500);
         } */

    int res = SSL_shutdownptr(ssl);
    LOGGER("SSL_shutdown=%d\n", res);

    if (res < 0) {
    #ifndef NOLOG
        int sslerr = SSL_get_errorptr(ssl, res);
        LOGGER("SSL_shutdown SSL_get_error=%d\n", sslerr);
        unsigned long err;
        while ((err = ERR_get_errorptr()) != 0) {
            char buf[256];
            ERR_error_string_nptr(err, buf, sizeof(buf));
            LOGGER("OpenSSL error: %s (0x%lx)\n", buf, err);
            }
#endif
       }


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
        #ifdef DLSYMS_SSL
		    if(SSL_library_initptr) 
        #endif
                SSL_library_initptr();
        #ifdef DLSYMS_SSL
		    if(OPENSSL_add_all_algorithms_noconfptr) 
        #endif
                OPENSSL_add_all_algorithms_noconfptr();
        #ifdef DLSYMS_SSL
		    if(SSL_load_error_stringsptr) 
        #endif
                SSL_load_error_stringsptr();  
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



/*
extern void linkopenssl();
void linkopenssl() {
//const char *filename="/apex/com.android.conscrypt/lib64/libssl.so";
const char *filename="/system/lib64/libssl.so";
void * handle;
   if((handle=bypass_loader_dlopen(filename,RTLD_LAZY|RTLD_GLOBAL))) {
     LOGGER("bypass_loader_dlopen %s succeeded\n",filename);
  }
else {
      LOGGER("dlopen %s failed\n",filename);
      return;
    }
    const char *symbol;
   SSL_CTX *(*SSL_CTX_newptr)(const SSL_METHOD *method);
    symbol= "SSL_CTX_new";
   if( (*(void **) (&SSL_CTX_newptr)=dlsym(handle, symbol))) {
      LOGGER("dlsym %s success!\n",symbol);
      }
   else {
      LOGGER("dlsym %s failed: %s\n",symbol,dlerror());
      }

//   dlclose(handle);
}
*/
#endif
#endif

int logcallback(const char *str, size_t len, void *u) {
#ifndef NOLOG
	const char *format=(const char *)u;
	loggert(format,str);
#endif
	return 0;
	}
