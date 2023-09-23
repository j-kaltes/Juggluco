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
/*      Fri Jan 27 12:38:18 CET 2023                                                 */


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <array>
#include "mkdirs.h"
using namespace std;

#include "inout.h"
class getdata {

string basepath;
public:
getdata() {}
getdata(std::string_view name): basepath(name) {}
void setpath(std::string_view name) {
	basepath=name;
	}
int open(string_view relname) {
    pathconcat abspath(basepath,relname);
	if(!mkdirs(striplastname(abspath),0700)) {
		lerror("mkdirs");
		return -1;
		}
    const char *filename=abspath.data();
    int    fp= ::open(filename,O_RDWR|O_CREAT,S_IRUSR |S_IWUSR);
    if(fp==-1) {
       lerror(filename);
	return -1;
	}
   return fp;
   }
int openread(string_view relname) {
    pathconcat abspath(basepath,relname);
    const char *filename=abspath.data();
    int    fp= ::open(filename,O_RDONLY);
    if(fp==-1) {
       lerror(filename);
	return -1;
	}
   return fp;
   }
bool mkfile(int fp,uint32_t len) {
    struct stat st;
   if(fstat(fp,&st)!=0) {
       lerror("fstat");
	return false;
	}
   if(st.st_size<len) {
	if(ftruncate(fp,len)) {
		lerror("ftruncate");
		return false;
		}
	}
    return true;
    }
char showhex(int get) {
	return (get>10?(get-10)+'A':get+'0');
	}
auto loghex(const unsigned char *data, int len) {
constexpr const int maxbytes=16;
	std::array<char,maxbytes*2+1> back;
	int take=std::min(len,maxbytes);
	for(int i=0;i<take;i++) {
		back[i*2]=showhex(data[i]>>4);
		back[i*2+1]=showhex(data[i]&0xf);
		}
	back[maxbytes*2]='\0';
	return back;
	}

bool savedata(int fp,uint32_t offset, uint32_t len,const unsigned char *data) {
    if(lseek( fp, offset, SEEK_SET )!=offset) {
       lerror("lseek");
	return false;
   	}
    LOGGER("savedata fp=%d off=%u len=%u data=%s\n",fp,offset,len,(char *)loghex(data,len).data());    
    if(write(fp,data,len)!=len) {
       lerror("write");
	return false;
   	}
     return true;
     }
bool close(int fp) {
   LOGGER("data close %d\n",fp);
	return ::close(fp)==0;

	}
};



