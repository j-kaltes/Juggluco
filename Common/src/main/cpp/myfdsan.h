#pragma once
#ifdef __ANDROID_API__
#include <dlfcn.h>
#include <android/fdsan.h>
#include "share/logs.hpp"


inline int  close_with_tag(int fd, uint64_t tag) {
   typedef int  (*android_fdsan_close_with_tag_t)(int fd, uint64_t tag);
   static const android_fdsan_close_with_tag_t func=(android_fdsan_close_with_tag_t) dlsym( RTLD_DEFAULT,"android_fdsan_android_fdsan_close_with_tag");
   if(func) {
       int res=func(fd,tag);
      LOGGER("close_with_tag(%d)=%d\n",fd,res);
      return res;
       }
    int res= close(fd);
    LOGGER("no close_with_tag(%d)=%d\n",fd,res);
    return res;
   }



inline uint64_t  get_owner_tag(int fd)  {
   typedef uint64_t  (*get_owner_tag_t)(int fd) ; 
   static const get_owner_tag_t func=(get_owner_tag_t) dlsym( RTLD_DEFAULT,"android_fdsan_get_owner_tag");
   if(func)  {
      auto res=func(fd);
      LOGGER("get_owner_tag(%d)=%lx\n",fd,res);
      return res;
      }
    LOGGER("no get_owner_tag(%d)\n",fd);
    return 0;
   }

inline void  exchange_owner_tag(int fd, uint64_t expected_tag, uint64_t new_tag) {
   typedef void  (*exchange_owner_tag_t)(int fd, uint64_t expected_tag, uint64_t new_tag);
   static const exchange_owner_tag_t func=(exchange_owner_tag_t) dlsym( RTLD_DEFAULT,"android_fdsan_exchange_owner_tag");
   if(func)  {
       LOGGER("exchange_owner_tag(%d,%xl,%xl)\n",fd,expected_tag,new_tag);
       func(fd,expected_tag,new_tag);
       return;
       }
    LOGGER("no exchange_owner_tag(%d,%xl,%xl)\n",fd,expected_tag,new_tag);
   }

inline void sockclose(int sock) {
    if(sock!=-1) {
        if(get_owner_tag(sock))
            return;
        close(sock);
        }
    }

#else
#define close_with_tag( fd, tag) close(fd)
#define get_owner_tag(fd) 0
#define exchange_owner_tag(fd, expected_tag,  new_tag)
#define sockclose(sock) close(sock)
#endif
inline void closesock(int &sock) {
    int tmpsock=sock;
    if(tmpsock!=-1) {
        sock=-1;
        shutdown(tmpsock,SHUT_RDWR);
        sockclose(tmpsock);
        }
    }
