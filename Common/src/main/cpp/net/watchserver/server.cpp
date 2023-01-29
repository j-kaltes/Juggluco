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


#include <errno.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <resolv.h>
#include <dlfcn.h>
#include "openssl/ssl.h"
#include "openssl/err.h"
#define FAIL    -1
// Create the SSL socket and intialize the socket address structure
int OpenListener(int port)
{
    int sd = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr={
    .sin_family = AF_INET,
    .sin_port = htons(port),
    .sin_addr={.s_addr = INADDR_ANY}};
    if (bind(sd, (struct sockaddr*)&addr, sizeof(addr)) != 0 )
    {
        perror("can't bind port");
        abort();
    }
    if ( listen(sd, 10) != 0 )
    {
        perror("Can't configure listening port");
        abort();
    }
    return sd;
}

#define LOGGER(...) fprintf(stderr,__VA_ARGS__)

const SSL_METHOD *(*TheMethod)(void);
int (*SSL_library_initptr)(void)=NULL;
void (*OPENSSL_add_all_algorithms_noconfptr)(void)=NULL;

void (*SSL_load_error_stringsptr)(void)=NULL;

int logcallback(const char *str, size_t len, void *u) {
	LOGGER("%s\n",str);
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
void  ERR_print_errors_fp(FILE *fp) {
	ERR_print_errors_cbptr(logcallback,nullptr);
	}

#if defined(__aarch64__) 
const char libssl[]="/system/lib64/libssl.so";
#else
const char libssl[]="/system/lib/libssl.so";
#endif
int loadsslfunctions() {
const char *libname=libssl;
  void *handle;
  if(!(handle=dlopen(libname, RTLD_NOW))) {
	  LOGGER("dlopen==nullptr: %s\n",dlerror());
	  return false;
	  }
  *((void **)&TheMethod)=dlsym(handle, "TLS_method");
  if(!TheMethod) {
	  LOGGER("dlsym(TLS_method): %s\n",dlerror());
  	*((void **)&TheMethod)=dlsym(handle, "SSLv23_method");
  		if(!TheMethod) {
	  		LOGGER("dlsym(SSLv23_method): %s\n",dlerror());
			dlclose(handle);
			return false;
			}
  	}
#define getsym(name) *((void **)&name##ptr)=dlsym(handle, #name)
#define symtest(name) if(!( *((void **)&name##ptr)=dlsym(handle, #name))) { LOGGER("%s",dlerror());dlclose(handle);return false;}

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
if(!(getsym(ERR_print_errors_cb))) {
#if defined(__aarch64__) 
const char cryptolib[]="/system/lib64/libcrypto.so";
#else
const char cryptolib[]="/system/lib/libcrypto.so";
#endif
	void *sslhandle=handle;
	  if(!(handle=dlopen(cryptolib, RTLD_NOW))) {
		  LOGGER("dlopen==nullptr: %s\n",dlerror());
		  dlclose(sslhandle);
		  return false;
		  }
	if(!(getsym(ERR_print_errors_cb))) {
		  dlclose(handle);
		  dlclose(sslhandle);
		  return false;
		}
	}
return true;
}
SSL_CTX* InitServerCTX(void)
{
    SSL_CTX *ctx;

    if(OPENSSL_add_all_algorithms_noconfptr)
    	OPENSSL_add_all_algorithms_noconfptr();
    //OpenSSL_add_all_algorithms();  /* load & register all cryptos, etc. */
    //OPENSSL_init_crypto(0x00000004L | 0x00000008L, ((void*)0));

    if(SSL_load_error_stringsptr)
	    SSL_load_error_stringsptr();   /* load all error messages */
    //OPENSSL_init_ssl(0x00200000L | 0x00000002L, ((void*)0));

     ctx = SSL_CTX_newptr(TheMethod());
//     ctx = SSL_CTX_new(SSLv23_method());
     //ctx = SSL_CTX_new(TLS_method());

    if ( ctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;

}


#include "servercrt.h"
#include "serverkey.h"
void LoadCertificates(SSL_CTX* ctx)
{
    if ( SSL_CTX_use_certificate_ASN1ptr(ctx, sizeof(servercrt), servercrt) <= 0 )
    {
	fprintf(stderr,"SSL_CTX_use_certificate_ASN1\n");
        ERR_print_errors_fp(stderr);
        abort();
    }
    if( SSL_CTX_use_PrivateKey_ASN1ptr(EVP_PKEY_RSA, ctx, serverkey, sizeof(serverkey))<=0)
    {
	fprintf(stderr,"SSL_CTX_use_PrivateKey_ASN1 failed\n");
        ERR_print_errors_fp(stderr);
        abort();
    }
    if ( !SSL_CTX_check_private_keyptr(ctx) )
    {
        fprintf(stderr, "Private key does not match the public certificate\n");
        abort();
    }
}



void Servlet(SSL* ssl) /* Serve the connection -- threadable */
{
    if ( SSL_acceptptr(ssl) == FAIL )     /* do SSL-protocol accept */
        ERR_print_errors_fp(stderr);
    else
    {
    	char buf[1024];
        int bytes = SSL_readptr(ssl, buf, sizeof(buf)); /* get request */
        buf[bytes] = '\0';
        printf("Client msg: \"%s\"\n", buf);
	const char ServerResponse[]="Hallo this me";
	SSL_writeptr(ssl, ServerResponse,sizeof(ServerResponse)-1); /* send reply */
    }
    int sd = SSL_get_fdptr(ssl);       /* get socket connection */
    SSL_freeptr(ssl);         /* release SSL state */
    close(sd);          /* close connection */
}

int main(int count, char *Argc[])
{
if(!loadsslfunctions())
	return -1;
    SSL_CTX *ctx;
    int server;
    char *portnum;
    if ( count != 2 )
    {
        printf("Usage: %s <portnum>\n", Argc[0]);
        exit(0);
    }
    if(SSL_library_initptr)
    	SSL_library_initptr();
   // OPENSSL_init_ssl(0, ((void*)0));
    portnum = Argc[1];
    ctx = InitServerCTX();        /* initialize SSL */
    LoadCertificates(ctx); /* load certs */
    server = OpenListener(atoi(portnum));    /* create server socket */
    while (1)
    {
        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);
        SSL *ssl;
        int client = accept(server, (struct sockaddr*)&addr, &len);  /* accept connection as usual */
        printf("Connection: %s:%d\n",inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
        ssl = SSL_newptr(ctx);              /* get new SSL state with context */
        SSL_set_fdptr(ssl, client);      /* set connection socket to SSL state */
        Servlet(ssl);         /* service connection */
    }
    close(server);          /* close server socket */
    SSL_CTX_freeptr(ctx);         /* release context */
}
