/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2, Libre 3, Dexcom G7/ONE+,              */
/*      Sibionics GS1Sb and Accu-Chek SmartGuide sensors.                            */
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
/*      Fri Nov 21 11:08:14 CET 2025                                                 */


#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <algorithm>
#include <dirent.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>

#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#include <openssl/err.h>
#include "destruct.hpp"
/*
#define LOGGERHTTPS(...) fprintf(stderr,__VA_ARGS__)
#define LOGARHTTPS(...) fprintf(stderr,"%s\n",__VA_ARGS__)
#define lerror(...) perror(__VA_ARGS__) */
#include "logs.hpp"
#include "inout.hpp"
#include "strconcat.hpp"
//#define MAIN 1
#define LOGHTTPS
#ifdef LOGHTTPS
#define LOGGERHTTPS(...) LOGGER("HTTPS: " __VA_ARGS__)
#define LOGARHTTPS(...) LOGAR("HTTPS: " __VA_ARGS__)
#define flerrorHTTPS(...) flerror("HTTPS: " __VA_ARGS__)
#else
#define LOGGERHTTPS(...) 
#define LOGARHTTPS(...) 
#define flerrorHTTPS(...) 
#endif
//#define LOGGERHTTPS(...) 
//#define LOGARHTTPS(...) 

using namespace std::literals;

#ifdef __ANDROID_API__
#define READ_CACERTS 1
#define DLSYMS_SSL 1
#endif

#ifdef DLSYMS_SSL 
#undef SSLv23_client_method
#undef SSLv23_method
typedef int (*SSL_verify_cb)(int preverify_ok, X509_STORE_CTX *x509_ctx);
#include <dlfcn.h>
extern void* opencrypto();
extern void* openssl();
#ifdef TEST
void* opencrypto() {
#ifdef __ANDROID_API__
	#if defined(__aarch64__) || defined(__x86_64__) 
	const char *lib="/system/lib64/libcrypto.so";
	#else
	const char *lib="/system/lib/libcrypto.so";
	#endif
#else
	const char *lib="/usr/lib/libcrypto.so";
#endif
  return dlopen(lib,RTLD_NOW);
  }
void* openssl() {
#ifdef __ANDROID_API__
	#if defined(__aarch64__) || defined(__x86_64__) 
	const char *lib="/system/lib64/libssl.so";
	#else
	const char *lib="/system/lib/libssl.so";
	#endif
#else
	const char *lib="/usr/lib/libssl.so";
#endif
  return dlopen(lib,RTLD_NOW);
  }
#endif

#include "cryptodecl.h"
#include "ssldecl.h"
static bool doinitcryptofuncs() {
   LOGARHTTPS("doinitcryptofuncs");
   #define hgetsym(handle,name) *((void **)&name##ptr)=dlsym(handle, #name)
   #define getsym(name) hgetsym(handle,name)
//   #define symtest(name) if(!(getsym(name))) { dlclose(handle);LOGGERHTTPS(#name ": %s\n",dlerror());return false;}
   #define symtest(name) if(!(getsym(name))) { LOGGERHTTPS(#name ": %s\n",dlerror());}
   void *handle=opencrypto();
   if(!handle)  {
        LOGARHTTPS("opencrypto() failed");
        return false;
        }
#include "cryptosyms.h"
   handle=openssl();
   if(!handle)  {
        LOGARHTTPS("openssl() failed");
        return false;
        }
    #include "sslsyms.h"

    if(!TLS_client_methodptr) {
        if(SSLv23_client_methodptr)
            TLS_client_methodptr=SSLv23_client_methodptr;
        else
            TLS_client_methodptr=SSLv23_methodptr;
        }
   LOGGERHTTPS("doinitcryptofuncs end TLS_client_methodptr=%p\n",TLS_client_methodptr);
    return true;
   }
#include "cryptodefs.h"
#include "ssldefs.h"
#endif

static int logcallback(const char *str, size_t len, void *u) {
    LOGGERHTTPS("logcallback(%.*s,%d,%p)\n",len,str,len,u );
    std::string *uit=(std::string *)u;
    uit->append(str,len);
    return 0;
    }


std::string get_openssl_error_string() {
    std::string uit("");

    LOGGERHTTPS("ERR_print_errors_cbptr=%p\n", ERR_print_errors_cb);
    ERR_print_errors_cb(logcallback,&uit);
    return uit;
    }
// Load Android system CA certs (DER format) into the given X509_STORE
static bool load_android_cacerts(SSL_CTX* ctx) {
#ifndef READ_CACERTS
     if(SSL_CTX_set_default_verify_paths(ctx)) {
        LOGARHTTPS("SSL_CTX_set_default_verify_paths Succeeded");
        return true;
        }
     else {
        std::string er=get_openssl_error_string();
        LOGGERHTTPS("SSL_CTX_set_default_verify_paths failed: %s\n",er.data());
        return false;
        }
#else
   constexpr const char ca_dir[] = "/system/etc/security/cacerts";
//    constexpr const char ca_dir[] = "/data/local/tmp/cacerts";
    if(SSL_CTX_load_verify_locations(ctx, NULL, ca_dir)) {
        LOGARHTTPS("SSL_CTX_load_verify_locations Succeeded");
        return true;
        }
     else {
        std::string er=get_openssl_error_string();
        LOGGERHTTPS("SSL_CTX_load_verify_locations failed: %s\n",er.data());
        return false;
        }
#endif
}
/*
       int flag = 1;
   setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
   struct linger l = { .l_onoff = 0, .l_linger = 0 };
   setsockopt(sock, SOL_SOCKET, SO_LINGER, &l, sizeof(l));
*/

static void sockopt(int new_fd) {
//    LOGGER("sockopt(%d)\n",new_fd);
       const int keepalive = 1;
       if(setsockopt(new_fd, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive)) < 0) {
        flerror("setsockopt(%d,SO_KEEPALIVE, ) failed",new_fd);
         }
      int retalive=-4;
    socklen_t retlen=sizeof(retalive);    

       if(getsockopt(new_fd, SOL_SOCKET, SO_KEEPALIVE, &retalive, &retlen) < 0) {
        flerror("getsockopt(%d,SO_KEEPALIVE, ) failed",new_fd);
         }
//    else LOGGER("KEEPALIVE=%d\n",retalive);
       const int keepcnt = 1;
    if(setsockopt(new_fd, IPPROTO_TCP, TCP_KEEPCNT, &keepcnt, sizeof(keepcnt))<0) {
        flerror("setsockopt(%d,TCP_KEEPCNT ) failed",new_fd);
        }
    retlen=sizeof(retalive);    
    if(getsockopt(new_fd, IPPROTO_TCP, TCP_KEEPCNT, &retalive, &retlen)<0) {
        flerror("getsockopt(%d,TCP_KEEPCNT ) failed",new_fd);
        }
//       else LOGGER("KEEPCNT=%d\n",retalive);
//       if(setsockopt(new_fd, IPPROTO_TCP, TCP_SYNCNT, keepcnt)<0)  {
 //       flerror("setsockopt(%d,TCP_SYNCNT) failed",new_fd); }
       const int keepidle = 10;
       if(setsockopt(new_fd, IPPROTO_TCP, TCP_KEEPIDLE, &keepidle, sizeof(keepidle)) < 0) {
        flerror("setsockopt(%d,TCP_KEEPIDLE, ) failed",new_fd);
         }
    retlen=sizeof(retalive);    

       if(getsockopt(new_fd, IPPROTO_TCP, TCP_KEEPIDLE, &retalive, &retlen) < 0) {
        flerror("getsockopt(%d,TCP_KEEPIDLE, ) failed",new_fd);
         }
//    else LOGGER("KEEPIDLE=%d\n",retalive);
       const int keepintvl = 10;
       if(setsockopt(new_fd, IPPROTO_TCP, TCP_KEEPINTVL, &keepintvl, sizeof(keepintvl)) < 0) {
        flerror("setsockopt(%d,TCP_KEEPINTVL, ) failed",new_fd);
         }
    retlen=sizeof(retalive);    
       if(getsockopt(new_fd, IPPROTO_TCP, TCP_KEEPINTVL, &retalive, &retlen) < 0) {
        flerror("getsockopt(%d,TCP_KEEPINTVL, ) failed",new_fd);
         }
//    else LOGGER("KEEPINTVL=%d\n",retalive);
     }

// Create a TCP connection to host:port
static int tcp_connect(const char *host, int port) {
    struct addrinfo hints{ .ai_family = AF_UNSPEC, .ai_socktype = SOCK_STREAM}, *res = nullptr;
    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", port);
    LOGGERHTTPS("tcp_connect(%s,%d)\n",host,port);
    if(getaddrinfo(host, port_str, &hints, &res) != 0) {
        lerror("getaddrinfo");
        return -1;
        }
    destruct _{[res]{ freeaddrinfo(res);}};
    LOGARHTTPS("Before socket");
    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    LOGGERHTTPS("After socket sock=%d\n",sock);
    if(sock < 0) {
        lerror("socket");
        return -1;
       }
    if(connect(sock, res->ai_addr, res->ai_addrlen) != 0) {
        lerror("connect");
        close(sock);
        return -1;
       }
    sockopt(sock);
    int flag = 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
    return sock;
    }

#ifdef DLSYMS_SSL 
#undef SSL_CTRL_SET_TLSEXT_HOSTNAME   

//long SSL_ctrl(SSL *ssl, int cmd, long larg, void *parg);
#define SSL_CTRL_SET_TLSEXT_HOSTNAME            55
static int SSL_set_tlsext_host_name2(const SSL *s, const char *name) {
    if(SSL_set_tlsext_host_nameptr)
        return  SSL_set_tlsext_host_nameptr(s, name) ;
    return  SSL_ctrl((SSL*)s,SSL_CTRL_SET_TLSEXT_HOSTNAME,TLSEXT_NAMETYPE_host_name,(void *)name);

     }
 #else
#define SSL_set_tlsext_host_name2 SSL_set_tlsext_host_name
 #endif


#include "ContextHTTPS.hpp"
    ContextHTTPS::ContextHTTPS(){
        LOGARHTTPS("ContextHTTPS()");
        static bool initlib=initLibrary();
        ctx=SSL_CTX_new(TLS_client_method());
        if (!ctx) {
            LOGARHTTPS("Failed to create SSL_CTX");
            error=true;
            }
        error=!load_android_cacerts(ctx);
        SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, nullptr);
        }
 ContextHTTPS &ContextHTTPS::getContext() {
            static ContextHTTPS contex;
             return contex;
             }
     ContextHTTPS::~ContextHTTPS() {
        LOGARHTTPS("SSL_CTX_free(ctx)");
        SSL_CTX_free(ctx);
        }
bool ContextHTTPS::initLibrary() {

#ifdef DLSYMS_SSL 
       static bool getcrypto=doinitcryptofuncs();
        if(getcrypto) {
            if(SSL_library_initptr)
                SSL_library_init();
            if(SSL_load_error_stringsptr)
                SSL_load_error_strings();
                return true;
              }
        
         return false;
#else
          SSL_library_init();
          SSL_load_error_strings();
          return true;
#endif
        }
//s/^ssl.h:# define \([^	 ]*\)[	 ]*\([0-9]\+\)[^0-9]*$/case \1: return "\1";/g
//s/^ssl.h:# define \([^	 ]*\)[	 ]*\([0-9]\+\)[^0-9]*$/case \2: return "\1";/g

static const char *geterrorstring(int error) {
    switch(error) {
        case 0: return "SSL_ERROR_NONE";
        case 1: return "SSL_ERROR_SSL";
        case 2: return "SSL_ERROR_WANT_READ";
        case 3: return "SSL_ERROR_WANT_WRITE";
        case 4: return "SSL_ERROR_WANT_X509_LOOKUP";
        case 5: return "SSL_ERROR_SYSCALL";
        case 6: return "SSL_ERROR_ZERO_RETURN";
        case 7: return "SSL_ERROR_WANT_CONNECT";
        case 8: return "SSL_ERROR_WANT_ACCEPT";
        case 9: return "SSL_ERROR_WANT_ASYNC";
        case 10: return "SSL_ERROR_WANT_ASYNC_JOB";
        case 11: return "SSL_ERROR_WANT_CLIENT_HELLO_CB";
        default: return "SSL_UNKNOWN_ERROR";
        }
}


static int SSLreadfull(SSL* ssl, char *dataptr,const int buflen) {
    LOGGERHTTPS("start SSLreadfull %d\n", buflen);
    int n=0;
     for(int res;;n+=res) {
        if(n>=buflen) {
            LOGGERHTTPS("SSL_read all %d\n",n);
            break;
            }
        res=SSL_read(ssl, dataptr+n,buflen-n);
        if(res>0) {
            LOGGERHTTPS("SSLread %d\n",res);
            continue;
            }
        int err = SSL_get_error(ssl, res);
        LOGGERHTTPS("SSL_read Error %d %s\n",err,geterrorstring(err));
        if(err==SSL_ERROR_WANT_READ) {
                res=0;
                continue;
                }
        if (err == SSL_ERROR_SSL) {
            unsigned long e = ERR_get_error();
            constexpr const int maxbuf=200;
            char buf[maxbuf];
            ERR_error_string_n(e, buf, maxbuf);
            LOGGERHTTPS("SSL_read SSL_ERROR %s\n",buf);
            }
        else {
            if(err == SSL_ERROR_SYSCALL) {
                lerror("SSL_read syscall error");
                }
            }
         break;
        }
    LOGGERHTTPS("end SSLreadfull %d\n",n);
    return n;
    }
    /*
static int SSLwritefull(SSL* ssl, const char *dataptr,const int buflen) {
    LOGGERHTTPS("start SSLwritefull %d",buflen);
    int n=0;
     for(int res;;n+=res) {
        if(n>=buflen) {
            LOGGERHTTPS("SSL_write all %d\n",buflen);
            break;
            }
        res=SSL_write(ssl, dataptr+n,buflen-n);
        if(res>0) {
            LOGGERHTTPS("SSLwrite %.*s %d\n",res,dataptr+n,res);
            continue;
            }
        int err = SSL_get_error(ssl, res);
        LOGGERHTTPS("SSL_write Error %d %s\n",err,geterrorstring(err));
        //int err = SSL_get_error(ssl, res);
        if (err == SSL_ERROR_SSL) {
            unsigned long e = ERR_get_error();
            constexpr const int maxbuf=200;
            char buf[maxbuf];
            ERR_error_string_n(e, buf, maxbuf);
            LOGGERHTTPS("SSL_read SSL_ERROR %s\n",buf);
            }
        else {
            if(err == SSL_ERROR_SYSCALL) {
                lerror("SSL_write syscall error");
                }
            }
         break;
        }
    LOGGERHTTPS("end SSLwritefull %d\n",n);
    return n;
    }
*/



static void shutdowner(int sock,SSL* ssl) {
   int flag = 1;
   setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
/*   struct linger l = { .l_onoff = 0, .l_linger = 0 };
   setsockopt(sock, SOL_SOCKET, SO_LINGER, &l, sizeof(l)); */
  for(int i=0;i<10;++i) { 
       int outq = 0;
        if(ioctl(sock, TIOCOUTQ, &outq) == 0) {
            LOGGERHTTPS("SIOCOUTQ = %d bytes\n", outq);
            if(outq==0)
                break;
            }
        else {
            flerrorHTTPS("ioctl(sock, TIOCOUTQ, &outq)): ");
            break;
            }
       
        usleep(500000);
        }
    /*
    struct tcp_info ti;
    socklen_t len = sizeof(ti);
    if(getsockopt(sock, IPPROTO_TCP, TCP_INFO, &ti, &len) == 0) {
        LOGGERHTTPS("tcpi_state=%u tcpi_retransmits=%u tcpi_rto=%u tcpi_snd_mss=%u tcpi_rtt=%u\n",ti.tcpi_state, ti.tcpi_retransmits, ti.tcpi_rto, ti.tcpi_snd_mss, ti.tcpi_rtt);
        } */
   for(int i=0;i<5;++i) {
       int res=SSL_shutdown(ssl);
       LOGGERHTTPS("SSL_shutdown=%d\n",res);
       if(res) {
           if(res==-1) {
                unsigned long err;
                while ((err = ERR_get_error()) != 0) {
                    char buf[256];
                    ERR_error_string_n(err, buf, sizeof(buf));
                    LOGGERHTTPS("OpenSSL error: %s (0x%lx)\n", buf, err);
                    }
                }
             break;
             }
         usleep(1000*500);
         }
      }

std::pair<std::vector<char>,int> ContextHTTPS::request(const std::string_view host,int port,const std::string_view path,const std::string_view TYPE,const std::span<const char> input) {
    std::vector<char> uit;   
    int sock = tcp_connect(host.data(), port);
    if (sock < 0) {
        return {uit,-1};
    }
    SSL* ssl = SSL_new(ctx);
    LOGGERHTTPS("after SSL_new(ctx)=%p\n",ssl);

    destruct _{[ssl,sock]{ 
       shutdowner(sock,ssl);
       shutdown(sock,SHUT_RDWR);
       close(sock);
       SSL_free(ssl);
       LOGGERHTTPS("close(%d)\n",sock);
        }};
    SSL_set_fd(ssl, sock);
    SSL_set_tlsext_host_name2(ssl, host.data());  // SNI

    LOGARHTTPS("before SSL_connect");
    auto conres=SSL_connect(ssl);
    LOGGERHTTPS("after SSL_connect conres=%d\n",conres);
    if(conres != 1) {
       const std:: string mess=get_openssl_error_string();
       LOGGERHTTPS("SSL handshake failed: %s\n", mess.c_str());
        return {uit,-1};
       }
    long verify_result = SSL_get_verify_result(ssl);
    if (verify_result != X509_V_OK) {
       LOGGERHTTPS("Certificate verification failed: %s\n", X509_verify_cert_error_string(verify_result)); 
    }; 
    const char closebuf[]{"\r\nConnection: close\r\n\r\n"};
    strconcat req {""sv,TYPE , " "sv,path," HTTP/1.1\r\nHost: "sv , host , "\r\nContent-Length: "sv,std::to_string(input.size()), closebuf};

    LOGGERHTTPS("connect %.*s %.*s\n",TYPE.size(),TYPE.data(),path.size(),path.data());
    const char *request=req.data();
   int requestsize=req.size();
    const int maxdata=  std::max((int)(input.size()+requestsize),16*1024);
    Mmap<char> data(maxdata);
    char *dataptr=data.data();
    memcpy(dataptr,request,requestsize);
    if(input.size()>0) {
        memcpy(dataptr+requestsize, input.data(), input.size());
        requestsize+=input.size();
       }
   SSL_write(ssl, dataptr, requestsize);
   SSL_write(ssl, "", 0);
   LOGGERHTTPS("after SSL_write %d\n", requestsize);
   int n=SSL_read(ssl, dataptr,maxdata);
   LOGGERHTTPS("after SSL_read=%d\n", n);
   char *enddata=dataptr+n;
   int   status_code=-1;
   char *startpos;
   if((startpos=std::find(dataptr,enddata,' '))!=enddata) {
        int end=sscanf(++startpos,"%d",&status_code);
        startpos+=end;
        }
    else {
        error=true;
        LOGGERHTTPS("no space in #%s# len=%d\n",dataptr,n);
        return {uit,-1};
        }
    LOGGERHTTPS("Status Code=%d\n",status_code);
    char zoek[]="\r\n\r\n";
    if(char *hit=std::search(startpos,enddata,zoek,zoek+sizeof(zoek)-1);hit!=enddata) {
        hit+=sizeof(zoek)-1;
        int len= enddata-hit;
        uit.reserve(len);
        #ifdef __cpp_lib_containers_ranges
        uit.append_range(std::span<char>(hit,len));
        #else
            uit.insert(uit.end(), hit, hit+len);
        #endif
        if(n==maxdata) {
            do {
                int n=SSLreadfull(ssl, dataptr,maxdata);
                if(n<=0)
                    break;
                uit.reserve(uit.capacity()+n);
        #ifdef __cpp_lib_containers_ranges
                uit.append_range(std::span<char>(dataptr,n));
                #else
                  uit.insert(uit.end(), dataptr, dataptr+n);
                  #endif
                } while(n==maxdata);
            }
        }
    return {uit,status_code};
    }
#ifdef MAIN
int main() {
  ContextHTTPS context;
  const char inpstr[]{"Hallo this me"};
  /*(
  auto [res3,code]=context.putRequest("echo.free.beeceptor.com",443,"/address",inpstr);
  if(code==200)
      write(STDERR_FILENO,res3.data(),res3.size());
      */
/*  auto res2=context.getRequest("www.juggluco.nl",443,"/Juggluco/download.html");
  write(STDERR_FILENO,res2.data(),res2.size()); */
 // auto [res1,code]=context.getRequest("a.juggluco.nl",7777,"/hallo/x/stream?header&days=20");
  auto [res1,code]=context.getRequest("www.juggluco.nl",443,"/Juggluco/download.html");
  writeall("tmpfile",res1.data(),res1.size()); 
  //write(STDERR_FILENO,res1.data(),res1.size()); 
}
#endif
