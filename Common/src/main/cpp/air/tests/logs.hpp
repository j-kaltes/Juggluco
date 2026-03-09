#pragma once
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdarg.h>

#define LOGGER(...) fprintf(stderr,__VA_ARGS__)
#define LOGAR(x)  fprintf(stderr,"%s\n",x)

#define LOGSTRING(x) fputs(x,stderr)
/*
       struct iovec {
           void   *iov_base; 
           size_t  iov_len;  
           };

 ssize_t writev(int fd, const struct iovec *iov, int iovcnt);
*/
inline void LOGGERN(const char *buf,size_t len) {
        char nl[]="\n";
        iovec data[2]{{(void*)buf,(size_t)len},{(void*)nl,sizeof(nl)-1}};
        writev(STDERR_FILENO,data,2);
//        write(buf,1,len,stderr) 
        }

inline void flerror(const char* fmt, ...){
	int waser=errno;
	const int maxbuf=160;
	char buf[maxbuf];
        va_list args;
        va_start(args, fmt);
	vsnprintf(buf,maxbuf, fmt, args);
	va_end(args);
	LOGGER("%s: %s\n",buf,strerror(waser));
	}

#define lerror(x) flerror("%s",x); 

#ifndef INCLUDE_NR
	#define INCLUDE_NR
	#include <asm-generic/unistd.h> /*Headers in this order*/
	#include <sys/syscall.h>
#endif // INCLUDE_NR
#include <unistd.h>
