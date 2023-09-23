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
/*      Sun Aug 27 20:48:47 CEST 2023                                                 */


 #include <unistd.h>
       #include <sys/types.h>
       #include <dirent.h>
#include <stdlib.h>
  #include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
       #include <sys/stat.h>
       #include <fcntl.h>
       #include <string.h>
        #include <dlfcn.h>

#include "destruct.h"

//#define MAIN
#ifdef MAIN
#define lerror perror
#define LOGGER printf
#else
#include "logs.h"
#endif
static int32_t getadbuid() { 
	const int32_t adbuid=2000;
	int sock=socket(AF_UNIX, SOCK_SEQPACKET|SOCK_CLOEXEC, 0) ;
	if(sock<0) {
		lerror("socket failed");
		return adbuid;
		}
	struct sockaddr_un adb {.sun_family=AF_UNIX,.sun_path{"\0jdwp-control"}};

	 if(connect(sock, (const struct sockaddr *) &adb,15)) {
	 	lerror("connect failed");
		return adbuid;
	 	}
	struct ucred adbgegs;	


	socklen_t adbgegslen=sizeof(adbgegs);
	 if(getsockopt(sock, SOL_SOCKET, SO_PEERCRED,(void *) &adbgegs,&adbgegslen)) {
	 	lerror("getsockopt failed");
		return adbuid;
	 	}

 	close(sock);
	return adbgegs.uid;
	}

int getsockets() {
   LOGAR("getsockets()");
	const auto adbuid=getadbuid();
	
      pid_t pid=getpid();
        const char format[]="/proc/%d/fd";
        char dirname[50]; 
        snprintf(dirname,50,format,pid); 
	int fp=open(dirname, O_DIRECTORY|O_RDONLY);
	if(fp<0) {
		lerror("open failed");
		return 3;
		}
	DIR *dir=fdopendir(fp);
	if(!dir) {
		close(fp);
		lerror("opendir");
		return 4;
		}
	destruct _des([dir,fp]{
			closedir(dir);
			close(fp);});
	struct dirent *ent;
	while((ent=readdir(dir))) {
		if(ent->d_type==DT_LNK)  {
			const char *name= ent->d_name;
			int handle=atoi(name);
			struct ucred gegs;	
			socklen_t slen=sizeof(gegs);
			if(!getsockopt(handle, SOL_SOCKET, SO_PEERCRED,(void *) &gegs,&slen)) {
				if(gegs.uid==adbuid) {
					typedef uint64_t (*android_fdsan_get_owner_tag_t)(int fd); android_fdsan_get_owner_tag_t android_fdsan_get_owner_tag;
					typedef uint64_t (*android_fdsan_close_with_tag_t)(int fd, uint64_t tag); android_fdsan_close_with_tag_t android_fdsan_close_with_tag;

					if(android_get_device_api_level()>=29&& 
					(android_fdsan_get_owner_tag= (android_fdsan_get_owner_tag_t)dlsym(RTLD_DEFAULT,"android_fdsan_get_owner_tag")) && 
					(android_fdsan_close_with_tag= (android_fdsan_close_with_tag_t)dlsym(RTLD_DEFAULT,"android_fdsan_close_with_tag"))) {

						uint64_t tag=android_fdsan_get_owner_tag(handle);
						LOGGER("pid=%d uid=%d gid=%d closed fp=%d tag=%llx\n",gegs.pid,gegs.uid,gegs.gid,handle,tag);
						android_fdsan_close_with_tag(handle,tag);
						}
					else {
						LOGGER("nofdsan: pid=%d uid=%d gid=%d closed fp=%d\n",gegs.pid,gegs.uid,gegs.gid,handle);
						close(handle);

						}
//					::shutdown(handle,SHUT_RDWR);
					struct ucred gegs2;	
					if(!getsockopt(handle, SOL_SOCKET, SO_PEERCRED,(void *) &gegs2,&slen)) {
						LOGGER("after shutdown %s pid=%d uid=%d gid=%d\n",name,gegs2.pid,gegs2.uid,gegs2.gid);
						}
					}
					/*
				else
					LOGGER("%s pid=%d uid=%d gid=%d not closed\n",name,gegs.pid,gegs.uid,gegs.gid); */
				}
				/*
			else {
				flerror("getsockopt %d failed\n",handle);
				} */
			}
		}
	return 1;
	}

#ifdef MAIN
int main(int argc,char **argv) {
	findadb(argv[1]);
     /* pid_t pid=getpid();
        const char format[]="/proc/%d/fd";
        char procpid[50]; 
        snprintf(procpid,50,format,pid); */

	}
	#endif
