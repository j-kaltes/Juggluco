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
#include <fcntl.h>
#include <poll.h>
#include <chrono>
#include <atomic>
#include <climits>
#include <cstring>
#include <cstdio>
#include "HttpResponse.hpp"

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#ifndef __ANDROID_API__
// Juggluco's bundled OpenSSL declarations do not include x509v3.h. Including
// it here can mix those declarations with an incompatible system header in
// command-line builds. Declare only the two C functions used for peer-name
// verification; Android resolves these same functions with dlsym below.
extern "C" {
int X509_check_host(X509 *cert, const char *name, size_t length,
                    unsigned int flags, char **peername);
int X509_check_ip_asc(X509 *cert, const char *ip, unsigned int flags);
}
#endif
#include <openssl/err.h>
#include "destruct.hpp"
/*
#define LOGGERHTTPS(...) fprintf(stderr,__VA_ARGS__)
#define LOGARHTTPS(...) fprintf(stderr,"%s\n",__VA_ARGS__)
#define lerrorHTTPS(...) perror(__VA_ARGS__) */
#include "logs.hpp"
#include "inout.hpp"
#include "strsepconcat.hpp"
//#define MAIN 1
//#define LOGHTTPS
#define LOGGERHTTPSERROR(...) LOGGER("HTTPS: " __VA_ARGS__)
#define LOGARHTTPSERROR(...) LOGAR("HTTPS: " __VA_ARGS__)
#define flerrorHTTPS(...) flerror("HTTPS: " __VA_ARGS__)
#define lerrorHTTPS(...) lerror("HTTPS: " __VA_ARGS__)
#ifdef LOGHTTPS
#define LOGGERHTTPS(...) LOGGER("HTTPS: " __VA_ARGS__)
#define LOGARHTTPS(...) LOGAR("HTTPS: " __VA_ARGS__)
//#define flerrorHTTPS(...) flerror("HTTPS: " __VA_ARGS__)
#else
#define LOGGERHTTPS(...) 
#define LOGARHTTPS(...) 
//#define flerrorHTTPS(...) 
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
   #define symtest(name) if(!(getsym(name))) { LOGGERHTTPSERROR("unavailable TLS symbol " #name ": %s\n",dlerror());}
   void *handle=opencrypto();
   if(!handle)  {
        LOGARHTTPSERROR("opencrypto() failed");
        return false;
        }
#include "cryptosyms.h"
   handle=openssl();
   if(!handle)  {
        LOGARHTTPSERROR("openssl() failed");
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
    return TLS_client_methodptr && SSL_CTX_newptr && SSL_CTX_freeptr &&
        SSL_CTX_set_verifyptr && SSL_CTX_load_verify_locationsptr &&
        SSL_newptr && SSL_freeptr && SSL_set_fdptr && SSL_connectptr &&
        SSL_get_errorptr && SSL_writeptr && SSL_readptr &&
        SSL_get_verify_resultptr && SSL_get_peer_certificateptr && X509_freeptr &&
        (SSL_set_tlsext_host_nameptr || SSL_ctrlptr);
   }
#include "cryptodefs.h"
#include "ssldefs.h"
#endif

// ISRG Root X1 (self-signed). Add other explicitly trusted root PEMs here if needed.
// Source: https://letsencrypt.org/certificates/
#ifdef READ_CACERTS
static constexpr char trustedRootPem[] = R"PEM(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)PEM";
#endif

static int logcallback(const char *str, size_t len, void *u) {
    LOGGERHTTPS("TLS error queue: %.*s",static_cast<int>(len),str);
    std::string *uit=(std::string *)u;
    uit->append(str,len);
    return 1;
    }


std::string get_openssl_error_string() {
    std::string uit("");

    LOGGERHTTPS("ERR_print_errors_cbptr=%p\n", ERR_print_errors_cb);
#ifdef DLSYMS_SSL
    if(ERR_print_errors_cbptr)
#endif
    ERR_print_errors_cb(logcallback,&uit);
    return uit;
    }
// Add a PEM-encoded CA from a buffer without opening a file.
#ifdef READ_CACERTS
static bool load_embedded_cacert(SSL_CTX *ctx, const char *pem, int len) {
#ifdef DLSYMS_SSL
    // Android's libcrypto/libssl are loaded with dlopen, not linked directly.
    static auto bioFromMemory = reinterpret_cast<decltype(&BIO_new_mem_buf)>(
        dlsym(opencrypto(), "BIO_new_mem_buf"));
    static auto parsePem = reinterpret_cast<decltype(&PEM_read_bio_X509)>(
        dlsym(opencrypto(), "PEM_read_bio_X509"));
    static auto freeBio = reinterpret_cast<decltype(&BIO_free)>(
        dlsym(opencrypto(), "BIO_free"));
    if (!bioFromMemory || !parsePem || !freeBio || !SSL_CTX_get_cert_storeptr ||
        !X509_STORE_add_certptr || !X509_freeptr) {
        LOGARHTTPSERROR("Functions to load embedded CA unavailable");
        return false;
    }
#else
    auto bioFromMemory = BIO_new_mem_buf;
    auto parsePem = PEM_read_bio_X509;
    auto freeBio = BIO_free;
#endif
    // BIO_new_mem_buf does not modify the supplied buffer. OpenSSL 1.0.1's
    // parameter is void *, whereas recent versions accept const void *.
    BIO *bio = bioFromMemory(const_cast<char *>(pem), len);
    if (!bio) {
        LOGARHTTPSERROR("BIO_new_mem_buf failed");
        return false;
    }
    X509 *cert = parsePem(bio, nullptr, nullptr, nullptr);
    freeBio(bio);
    if (!cert) {
        LOGGERHTTPSERROR("Could not parse embedded CA: %s\n",
                         get_openssl_error_string().c_str());
        return false;
    }
    X509_STORE *store = SSL_CTX_get_cert_store(ctx);
    const bool added = store && X509_STORE_add_cert(store, cert) == 1;
    X509_free(cert);
    if (!added)
        LOGGERHTTPSERROR("Could not add embedded CA: %s\n",
                         get_openssl_error_string().c_str());
    return added;
}
#endif

static int verify_debug(int ok, X509_STORE_CTX *store) {
    if (!ok) {
#ifdef DLSYMS_SSL
        if (!X509_STORE_CTX_get_errorptr || !X509_STORE_CTX_get_error_depthptr ||
            !X509_verify_cert_error_stringptr) {
            LOGARHTTPSERROR("Certificate verification failed (error details unavailable)");
            return ok;
        }
#endif
        int err = X509_STORE_CTX_get_error(store);
        LOGGERHTTPSERROR("Verify depth=%d error=%d: %s\n",
                         X509_STORE_CTX_get_error_depth(store), err,
                         X509_verify_cert_error_string(err));
    }
    return ok;
}

// Load Android system CA certificates (PEM format) into the SSL_CTX trust store.
static bool load_android_cacerts(SSL_CTX* ctx) {
#ifndef READ_CACERTS
     if(SSL_CTX_set_default_verify_paths(ctx)) {
        LOGARHTTPS("SSL_CTX_set_default_verify_paths Succeeded");
        return true;
        }
     else {
        std::string er=get_openssl_error_string();
        LOGGERHTTPSERROR("SSL_CTX_set_default_verify_paths failed: %s\n",er.data());
        return false;
        }
#else
    // Android CA filenames use X509_NAME_hash_old (MD5), but OpenSSL 1.0.1's
    // CApath lookup uses X509_NAME_hash (SHA1). Load each file as a CAfile
    // instead, so that verification does not depend on the filename hash.
    for(const char *dir:{"/apex/com.android.conscrypt/cacerts","/system/etc/security/cacerts"}) {
        if(access(dir,R_OK|X_OK)) {
            // One CA location is normally absent on older/newer Android releases.
            if(errno!=ENOENT) flerrorHTTPS("access CA directory %s",dir);
            continue;
        }
        DIR *d=opendir(dir);
        if(!d) {
            flerrorHTTPS("opendir CA directory %s",dir);
            continue;
        }
        unsigned loaded=0;
        unsigned failed=0;
        while(auto *entry=readdir(d)) {
            std::string_view name(entry->d_name);
            // Only Android's system CA names: 8 hexadecimal digits, '.', number.
            if(name.size()<10 || name[8]!='.' ||
               !std::all_of(name.begin(),name.begin()+8,[](char c) {
                   return (c>='0'&&c<='9') || (c>='a'&&c<='f') || (c>='A'&&c<='F');
               }) ||
               !std::all_of(name.begin()+9,name.end(),[](char c) {
                   return c>='0'&&c<='9';
               }))
                continue;
            std::string path=std::string(dir)+'/'+entry->d_name;
            if(SSL_CTX_load_verify_locations(ctx,path.c_str(),nullptr))
                ++loaded;
            else {
                ++failed;
                LOGGERHTTPSERROR("load CA file %s failed: %s\n",path.c_str(),
                                 get_openssl_error_string().c_str());
            }
        }
        closedir(d);
        LOGGERHTTPS("Android CA directory %s: loaded %u certificates, failed %u\n",
                    dir,loaded,failed);
        if(loaded) return true;
    }
    LOGARHTTPSERROR("No Android system CA directory could be loaded");
    return false;
#endif
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
        if(!initlib) {
            LOGARHTTPSERROR("initLibrary failed");
            error=true;
            return;
            }
        ctx=SSL_CTX_new(TLS_client_method());
        if (!ctx) {
            LOGARHTTPSERROR("Failed to create SSL_CTX");
            error=true;
            return;
            }
        const bool systemCAs = load_android_cacerts(ctx);
#ifdef READ_CACERTS
        const bool embeddedCA = load_embedded_cacert(ctx, trustedRootPem,
                                                      sizeof(trustedRootPem) - 1);
        error = !(systemCAs || embeddedCA);
#else
        error = !systemCAs;
#endif
        SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, verify_debug);
        }
 ContextHTTPS &ContextHTTPS::getContext() {
            static ContextHTTPS contex;
             return contex;
             }
     ContextHTTPS::~ContextHTTPS() {
        LOGARHTTPS("SSL_CTX_free(ctx)");
        if(ctx)
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
#ifndef NOLOG
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
#endif

namespace {
using HttpsClock = std::chrono::steady_clock;
struct HttpsTrace {
    unsigned long long id;
    HttpsClock::time_point start = HttpsClock::now();
    const char *stage = "validation";
    HttpsTrace() {
        static std::atomic<unsigned long long> sequence{0};
        id = ++sequence;
    }
    long long elapsed() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(HttpsClock::now() - start)
            .count();
    }
    std::pair<std::vector<char>, int> fail(const char *reason) const {
        LOGGERHTTPSERROR("request #%llu %s failed after %lld ms: %s\n", id, stage, elapsed(),
                         reason);
        return {{}, -1};
    }
};
static void httpsClose(int fd) {
    if (close(fd))
        flerrorHTTPS("close socket %d", fd);
}
static bool httpsWait(int fd, short event, HttpsClock::time_point deadline,
                      const HttpsTrace &trace) {
    for (;;) {
        auto left =
            std::chrono::duration_cast<std::chrono::milliseconds>(deadline - HttpsClock::now())
                .count();
        if (left <= 0) {
            LOGGERHTTPSERROR("request #%llu %s deadline reached\n", trace.id, trace.stage);
            return false;
        }
        pollfd p{fd, event, 0};
        int n = poll(&p, 1, static_cast<int>(left));
        if (n < 0 && errno == EINTR)
            continue;
        if (n < 0) {
            flerrorHTTPS("request #%llu %s poll", trace.id, trace.stage);
            return false;
        }
        if (!n || (p.revents & (POLLNVAL | POLLERR))) {
            LOGGERHTTPSERROR("request #%llu %s poll %s (events=0x%x)\n", trace.id, trace.stage,
                             !n ? "timed out" : "socket error", static_cast<unsigned>(p.revents));
            return false;
        }
        return true; // HUP is handled by the next TLS call.
    }
}
static int httpsConnect(const std::string &host, int port, HttpsClock::time_point deadline,
                        const HttpsTrace &trace) {
    addrinfo hints{}, *addresses = nullptr;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;
    const auto service = std::to_string(port);
    int lookup = getaddrinfo(host.c_str(), service.c_str(), &hints, &addresses);
    if (lookup) {
        [[maybe_unused]] const int saved = errno;
        LOGGERHTTPSERROR("request #%llu getaddrinfo: %s (%d), errno=%d\n", trace.id,
                         gai_strerror(lookup), lookup, saved);
        return -1;
    }
    destruct release{[addresses] { freeaddrinfo(addresses); }};
    unsigned attempt = 0;
    for (auto *a = addresses; a && HttpsClock::now() < deadline; a = a->ai_next) {
        ++attempt;
        int fd = socket(a->ai_family, SOCK_STREAM | SOCK_CLOEXEC, a->ai_protocol);
        if (fd < 0) {
            flerrorHTTPS("request #%llu socket attempt=%u family=%d", trace.id, attempt,
                         a->ai_family);
            continue;
        }
        if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
            flerrorHTTPS("request #%llu fcntl O_NONBLOCK", trace.id);
            httpsClose(fd);
            continue;
        }
        int r = connect(fd, a->ai_addr, a->ai_addrlen);
        if (r < 0 && errno == EINPROGRESS) {
            if (httpsWait(fd, POLLOUT, deadline, trace)) {
                int error = 0;
                socklen_t len = sizeof(error);
                if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len)) {
                    flerrorHTTPS("request #%llu getsockopt SO_ERROR", trace.id);
                } else if (error) {
                    LOGGERHTTPSERROR("request #%llu connect attempt=%u: %s (%d)\n", trace.id,
                                     attempt, strerror(error), error);
                } else
                    r = 0;
            }
        } else if (r < 0)
            flerrorHTTPS("request #%llu connect attempt=%u", trace.id, attempt);
        if (r == 0) {
            LOGGERHTTPS("request #%llu connected fd=%d attempt=%u\n", trace.id, fd, attempt);
            return fd;
        }
        httpsClose(fd);
    }
    LOGGERHTTPSERROR("request #%llu no TCP connection after %u address attempts\n", trace.id,
                     attempt);
    return -1;
}

#ifndef DLSYMS_SSL
extern  "C" X509 *SSL_get1_peer_certificate(const SSL *ssl);
#endif
// Legacy OpenSSL (Android 5.1) does not provide X509_check_host/ip_asc.
// Read the DER subjectAltName extension through APIs already in OpenSSL 1.0.1.
// Parsing the two relevant GeneralName types directly avoids x509v3.h, which
// is not part of Juggluco's bundled OpenSSL declarations.
#ifdef DLSYMS_SSL
struct LegacyNameApi {
    int (*extByNid)(const X509 *, int, int);
    X509_EXTENSION *(*getExt)(const X509 *, int);
    ASN1_OCTET_STRING *(*extData)(X509_EXTENSION *);
    int (*stringLength)(const ASN1_STRING *);
    const unsigned char *(*stringGet0Data)(const ASN1_STRING *);
    unsigned char *(*stringData)(ASN1_STRING *);
    X509_NAME *(*subjectName)(const X509 *);
    int (*nameIndex)(const X509_NAME *, int, int);
    X509_NAME_ENTRY *(*nameEntry)(const X509_NAME *, int);
    ASN1_STRING *(*entryData)(const X509_NAME_ENTRY *);
};

static const LegacyNameApi &legacyNameApi() {
    static const LegacyNameApi api = [] {
        void *lib = opencrypto();
        LegacyNameApi a{};
        if (!lib) return a;
        a.extByNid = reinterpret_cast<decltype(a.extByNid)>(dlsym(lib,"X509_get_ext_by_NID"));
        a.getExt = reinterpret_cast<decltype(a.getExt)>(dlsym(lib,"X509_get_ext"));
        a.extData = reinterpret_cast<decltype(a.extData)>(dlsym(lib,"X509_EXTENSION_get_data"));
        a.stringLength = reinterpret_cast<decltype(a.stringLength)>(dlsym(lib,"ASN1_STRING_length"));
        a.stringGet0Data = reinterpret_cast<decltype(a.stringGet0Data)>(dlsym(lib,"ASN1_STRING_get0_data"));
        a.stringData = reinterpret_cast<decltype(a.stringData)>(dlsym(lib,"ASN1_STRING_data"));
        a.subjectName = reinterpret_cast<decltype(a.subjectName)>(dlsym(lib,"X509_get_subject_name"));
        a.nameIndex = reinterpret_cast<decltype(a.nameIndex)>(dlsym(lib,"X509_NAME_get_index_by_NID"));
        a.nameEntry = reinterpret_cast<decltype(a.nameEntry)>(dlsym(lib,"X509_NAME_get_entry"));
        a.entryData = reinterpret_cast<decltype(a.entryData)>(dlsym(lib,"X509_NAME_ENTRY_get_data"));
        return a;
    }();
    return api;
}

static const unsigned char *legacyStringData(const LegacyNameApi &api,
                                              ASN1_STRING *str) {
    if (api.stringGet0Data) return api.stringGet0Data(str);
    return api.stringData ? api.stringData(str) : nullptr;
}

// Read one definite-length DER TLV; reject indefinite, non-minimal and
// out-of-bounds lengths. Returned 'value' is bounded by 'length'.
static bool readDer(const unsigned char *&p, const unsigned char *end,
                    unsigned char &tag, const unsigned char *&value, size_t &length) {
    if (end - p < 2) return false;
    tag = *p++;
    const unsigned char first = *p++;
    if (!(first & 0x80)) {
        length = first;
    } else {
        const unsigned n = first & 0x7f;
        if (!n || n > sizeof(size_t) || size_t(end - p) < n || !*p)
            return false;
        length = 0;
        for (unsigned i = 0; i < n; ++i)
            length = (length << 8) | *p++;
        if (length < 128) return false;
    }
    if (length > size_t(end - p)) return false;
    value = p;
    p += length;
    return true;
}

static unsigned char asciiLower(unsigned char c) {
    return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c;
}

// Accept DNS labels only. Permit a wildcard solely as the complete leftmost
// label and only when the suffix contains at least two labels.
static bool validDns(const unsigned char *s, size_t n, bool allowWildcard) {
    if (!s || n == 0 || n > 253) return false;
    size_t i = 0;
    if (allowWildcard && n > 2 && s[0] == '*' && s[1] == '.') {
        i = 2;
        if (!memchr(s + i, '.', n - i)) return false;
    }
    size_t labelStart = i;
    for (; i <= n; ++i) {
        if (i == n || s[i] == '.') {
            if (i == labelStart || i - labelStart > 63 ||
                s[labelStart] == '-' || s[i - 1] == '-') return false;
            labelStart = i + 1;
        } else {
            const unsigned char c = asciiLower(s[i]);
            if (!(c >= 'a' && c <= 'z') && !(c >= '0' && c <= '9') && c != '-')
                return false;
        }
    }
    return true;
}

static bool matchesDns(const unsigned char *pattern, size_t plen,
                       const std::string &host) {
    const auto *h = reinterpret_cast<const unsigned char *>(host.data());
    if (!validDns(pattern, plen, true) || !validDns(h, host.size(), false))
        return false;
    size_t patternOffset = 0, hostOffset = 0;
    if (plen >= 2 && pattern[0] == '*' && pattern[1] == '.') {
        const auto dot = host.find('.');
        if (dot == std::string::npos || dot == 0 ||
            (dot >= 4 && asciiLower(h[0]) == 'x' && asciiLower(h[1]) == 'n' &&
             h[2] == '-' && h[3] == '-')) return false;
        patternOffset = 2;
        hostOffset = dot + 1;
    }
    if (host.size() - hostOffset != plen - patternOffset) return false;
    for (size_t i = patternOffset; i < plen; ++i)
        if (asciiLower(pattern[i]) != asciiLower(h[hostOffset + i - patternOffset]))
            return false;
    return true;
}

static bool legacyPeerName(X509 *cert, const std::string &host,
                           const unsigned char *ip, size_t ipLength) {
    const LegacyNameApi &a = legacyNameApi();
    if (!a.extByNid || !a.getExt || !a.extData || !a.stringLength ||
        (!a.stringGet0Data && !a.stringData)) {
        LOGARHTTPSERROR("OpenSSL 1.0.1 certificate-name functions unavailable");
        return false;
    }
    const int index = a.extByNid(cert, NID_subject_alt_name, -1);
    if (index >= 0) {
        // Multiple SAN extensions are invalid/ambiguous; do not accept one.
        if (a.extByNid(cert, NID_subject_alt_name, index) >= 0) return false;
        X509_EXTENSION *ext = a.getExt(cert, index);
        ASN1_OCTET_STRING *octets = ext ? a.extData(ext) : nullptr;
        int size = octets ? a.stringLength(octets) : -1;
        const unsigned char *der = octets ? legacyStringData(a, octets) : nullptr;
        if (!der || size < 2) return false;
        const unsigned char *p = der, *end = der + size, *value;
        unsigned char tag;
        size_t length;
        if (!readDer(p, end, tag, value, length) || tag != 0x30 || p != end)
            return false;
        p = value;
        end = value + length;
        bool found = false;
        if (p == end) return false;
        while (p < end) {
            if (!readDer(p, end, tag, value, length) ||
                (tag & 0xc0) != 0x80 || (tag & 0x1f) > 8)
                return false;
            if (ipLength) {
                // iPAddress SAN is exactly 4 or 16 raw bytes.
                if (tag == 0x87 && length == ipLength &&
                    memcmp(value, ip, ipLength) == 0) found = true;
            } else if (tag == 0x82 && matchesDns(value, length, host)) {
                found = true;
            }
        }
        return found;
    }
    // IP literals must have an iPAddress SAN, never a DNS CN.
    // For DNS, accept the legacy Common Name only if SAN is wholly absent.
    if (index != -1 || ipLength || !a.subjectName || !a.nameIndex ||
        !a.nameEntry || !a.entryData) return false;
    X509_NAME *subject = a.subjectName(cert);
    if (!subject) return false;
    const int cn = a.nameIndex(subject, NID_commonName, -1);
    if (cn < 0 || a.nameIndex(subject, NID_commonName, cn) >= 0) return false;
    X509_NAME_ENTRY *entry = a.nameEntry(subject, cn);
    ASN1_STRING *name = entry ? a.entryData(entry) : nullptr;
    const int len = name ? a.stringLength(name) : -1;
    const unsigned char *data = name ? legacyStringData(a, name) : nullptr;
    return data && len > 0 && matchesDns(data, size_t(len), host);
}
#endif

static bool httpsPeerName(SSL *ssl, const std::string &host) {
#ifdef DLSYMS_SSL
    using CheckHost = int (*)(X509 *, const char *, size_t, unsigned int, char **);
    using CheckIp = int (*)(X509 *, const char *, unsigned int);
    static auto hostCheck = reinterpret_cast<CheckHost>(dlsym(opencrypto(), "X509_check_host"));
    static auto ipCheck = reinterpret_cast<CheckIp>(dlsym(opencrypto(), "X509_check_ip_asc"));
    if (!SSL_get_peer_certificateptr || !X509_freeptr) {
        LOGARHTTPSERROR("Peer certificate functions unavailable");
        return false;
    }
#else
    auto hostCheck = X509_check_host;
    auto ipCheck = X509_check_ip_asc;
#define SSL_get_peer_certificate SSL_get1_peer_certificate
#endif
    X509 *cert = SSL_get_peer_certificate(ssl);
    if (!cert) {
        LOGARHTTPSERROR("Peer certificate absent");
        return false;
    }
    destruct release{[cert] { X509_free(cert); }};
    unsigned char ip[16];
    const int v4 = inet_pton(AF_INET, host.c_str(), ip);
    const int v6 = v4 == 1 ? 0 : inet_pton(AF_INET6, host.c_str(), ip);
    const size_t ipLength = v4 == 1 ? 4 : (v6 == 1 ? 16 : 0);
    if (ipLength && ipCheck) return ipCheck(cert, host.c_str(), 0) == 1;
    if (!ipLength && hostCheck)
        return hostCheck(cert, host.c_str(), host.size(), 0, nullptr) == 1;
#ifdef DLSYMS_SSL
    // OpenSSL 1.0.1 (Android 5.1.1) has neither X509_check_* function.
    return legacyPeerName(cert, host, ip, ipLength);
#else
    return false;
#endif
}

} // namespace
std::pair<std::vector<char>, int> ContextHTTPS::request(const std::string_view host, int port,
                                                        const std::string_view path,
                                                        const std::string_view TYPE,
                                                        const std::span<const char> input,
                                                        const std::string_view header,
                                                        std::string *location,
                                                        const HttpsClientIdentity *client) {
    HttpsTrace trace;
    if (location)
        location->clear();
    try {
        if (error || !ctx)
            return trace.fail("TLS context unavailable");
        if (host.empty() || host.size() > 253 ||
            host.find_first_not_of(
                "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-_:") != host.npos)
            return trace.fail("Invalid host");
        if (port < 1 || port > 65535)
            return trace.fail("Invalid port");
        if (path.empty() || (path.front() != '/' && !path.starts_with("https://")) ||
            std::any_of(path.begin(), path.end(),
                        [](unsigned char c) { return c <= 32 || c == 127; }))
            return trace.fail("Invalid request path");
        if (TYPE.empty() || TYPE.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ") != TYPE.npos)
            return trace.fail("Invalid HTTP method");
        if (input.size() > 8 * 1024 * 1024)
            return trace.fail("Request body exceeds 8 MiB");
        if (header.find("\r\n\r\n") != header.npos ||
            (!header.empty() && !header.starts_with("\r\n")))
            return trace.fail("Invalid request header framing");
        // One start/end pair per request; never log query strings, headers or bodies.
        [[maybe_unused]] auto logPath = path.substr(0, path.find_first_of("?#"));
        LOGGER("HTTPS: request #%llu %.*s %.*s:%d %.*s (%zu body bytes)\n", trace.id,
               static_cast<int>(std::min<size_t>(TYPE.size(), 20)), TYPE.data(),
               static_cast<int>(host.size()), host.data(), port,
               static_cast<int>(std::min<size_t>(logPath.size(), 180)), logPath.data(),
               input.size());
        const std::string hostname(host);
        const auto deadline = trace.start + std::chrono::seconds(450);
        trace.stage = "TCP connect";
        int sock = httpsConnect(hostname, port, deadline, trace);
        if (sock < 0)
            return trace.fail("Cannot connect");
        // Socket ownership begins before SSL_new (including exception paths).
        destruct socketRelease{[sock] { httpsClose(sock); }};
        trace.stage = "TLS setup";
        SSL *ssl = SSL_new(ctx);
        if (!ssl)
            return trace.fail("SSL_new");
        destruct release{[ssl] { SSL_free(ssl); }};
        if (client) {
            trace.stage = "TLS client identity";
            if (!client->certificate || !client->privateKey || !client->issuer)
                return trace.fail("Incomplete client certificate identity");
#ifdef DLSYMS_SSL
            static auto useCert = reinterpret_cast<int (*)(SSL *, X509 *)>(
                dlsym(openssl(), "SSL_use_certificate"));
            static auto useKey = reinterpret_cast<int (*)(SSL *, EVP_PKEY *)>(
                dlsym(openssl(), "SSL_use_PrivateKey"));
            static auto checkKey = reinterpret_cast<int (*)(const SSL *)>(
                dlsym(openssl(), "SSL_check_private_key"));
            static auto addChain = reinterpret_cast<int (*)(SSL *, X509 *)>(
                dlsym(openssl(), "SSL_add1_chain_cert"));
            if (!useCert || !useKey || !checkKey || (!addChain && !SSL_ctrlptr))
                return trace.fail("Client certificate TLS functions unavailable");
#else
            auto useCert = SSL_use_certificate;
            auto useKey = SSL_use_PrivateKey;
            auto checkKey = SSL_check_private_key;
            auto addChain = [](SSL *s, X509 *cert) { return SSL_add1_chain_cert(s, cert); };
#endif
            if (useCert(ssl, client->certificate) != 1)
                return trace.fail("SSL_use_certificate");
            if (useKey(ssl, client->privateKey) != 1 || checkKey(ssl) != 1)
                return trace.fail("Client certificate/private key mismatch");
#ifdef DLSYMS_SSL
            const auto added = addChain ? addChain(ssl, client->issuer)
                                        : SSL_ctrl(ssl, SSL_CTRL_CHAIN_CERT, 1, client->issuer);
#else
            const auto added = addChain(ssl, client->issuer);
#endif
            if (added != 1)
                return trace.fail("Cannot add client certificate issuer");
            LOGGER("HTTPS: request #%llu client certificate and key installed for this connection\n",
                   trace.id);
        }
        trace.stage = "TLS setup";
        if (SSL_set_fd(ssl, sock) != 1)
            return trace.fail("SSL_set_fd");
        if (SSL_set_tlsext_host_name2(ssl, hostname.c_str()) != 1)
            return trace.fail("SSL_set_tlsext_host_name");
        auto again = [&](int e, int savedErrno) {
            if (e == SSL_ERROR_WANT_READ || e == SSL_ERROR_WANT_WRITE)
                return httpsWait(sock, e == SSL_ERROR_WANT_READ ? POLLIN : POLLOUT, deadline,
                                 trace);
            LOGGERHTTPSERROR("request #%llu %s: %s (%d), errno=%d (%s)\n", trace.id, trace.stage,
                             geterrorstring(e), e, savedErrno, strerror(savedErrno));
            auto detail = get_openssl_error_string();
            if (!detail.empty())
                LOGGERHTTPSERROR("request #%llu TLS: %s\n", trace.id, detail.c_str());
            return false;
        };
        trace.stage = "TLS handshake";
        for (;;) {
            if (HttpsClock::now() >= deadline)
                return trace.fail("Handshake deadline reached");
            errno = 0;
            int r = SSL_connect(ssl), saved = errno;
            if (r == 1)
                break;
            if (!again(SSL_get_error(ssl, r), saved))
                return trace.fail("SSL_connect");
        }
        long verify = SSL_get_verify_result(ssl);
        if (verify != X509_V_OK) {
            LOGGERHTTPSERROR("request #%llu certificate chain verification code=%ld\n", trace.id,
                             verify);
            return trace.fail("Certificate chain rejected");
        }
        if (!httpsPeerName(ssl, hostname))
            return trace.fail("Certificate host name rejected");
        LOGGERHTTPS("request #%llu TLS handshake and peer verification complete\n", trace.id);
        char portSuffix[7]{};  // colon, at most five decimal digits, NUL
        if (port != 443)
            std::snprintf(portSuffix, sizeof(portSuffix), ":%d", port);
        strsepconcat req{""sv, TYPE, " "sv, path, " HTTP/1.1\r\nHost: "sv, host,
                         portSuffix, "\r\nContent-Length: "sv, std::to_string(input.size()),
                         header, "\r\nConnection: close\r\n\r\n"sv};
        // Keep the header and binary body contiguous in the first TLS write.
        // The signalling server's request parser currently assumes this.
        const size_t requestsize = size_t(req.size()) + input.size();
        if (requestsize > size_t(INT_MAX))
            return trace.fail("Request too large for SSL_write");
        Mmap<char> data(std::max(int(requestsize), 16 * 1024));
        char *dataptr = data.data();
        memcpy(dataptr, req.data(), req.size());
        if (!input.empty())
            memcpy(dataptr + req.size(), input.data(), input.size());
        trace.stage = "TLS write";
        for (size_t at = 0; at < requestsize;) {
            if (HttpsClock::now() >= deadline)
                return trace.fail("Write deadline reached");
            errno = 0;
            int r = SSL_write(ssl, dataptr + at, int(requestsize - at)), saved = errno;
            if (r > 0)
                at += r;
            else if (!again(SSL_get_error(ssl, r), saved))
                return trace.fail("SSL_write");
        }
#if defined(OPENSSL_IS_BORINGSSL)
        // Preserve the former BoringSSL zero-length write (flushes pending
        // control data). OpenSSL does not support this operation.
        SSL_write(ssl, "", 0);
#endif
        LOGGERHTTPS("request #%llu wrote %zu bytes\n", trace.id, requestsize);
        std::string raw;
        trace.stage = "TLS read";
        for (;;) {
            if (HttpsClock::now() >= deadline)
                return trace.fail("Read deadline reached");
            char buf[16384];
            errno = 0;
            int n = SSL_read(ssl, buf, sizeof(buf)), saved = errno;
            bool eof = false;
            if (n > 0)
                raw.append(buf, n);
            else {
                int e = SSL_get_error(ssl, n);
                if (e == SSL_ERROR_ZERO_RETURN)
                    eof = true;
                else if (again(e, saved))
                    continue;
                else
                    return trace.fail("SSL_read");
            }
            auto response = juggluco_http::parse(raw, eof);
            if (response.complete) {
                LOGGER("HTTPS: request #%llu complete HTTP %d, %zu body bytes, %lld ms\n", trace.id,
                       response.status, response.body.size(), trace.elapsed());
                if (response.status >= 400)
                    LOGGERHTTPSERROR("request #%llu server returned HTTP %d\n", trace.id,
                                     response.status);
                if (response.status == 307 || response.status == 308) {
                    LOGGER("HTTPS: request #%llu redirect Location %s\n", trace.id,
                           response.location.empty() ? "missing" : "present");
                }
                if (location)
                    *location = std::move(response.location);
                return {std::move(response.body), response.status};
            }
        }
    } catch (const juggluco_http::ParseError &e) {
        return trace.fail(e.what());
    } catch (const std::bad_alloc &) {
        return trace.fail("Caught allocation failure");
    } catch (const std::exception &) {
        LOGGERHTTPSERROR("request #%llu caught std::exception (contents omitted)\n", trace.id);
        return trace.fail("Exception while performing request");
    } catch (...) {
        return trace.fail("Unknown exception while performing request");
    }
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
