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
/*      Fri Jan 27 12:35:35 CET 2023                                                 */



#include "config.h"
#include <string_view>
#if defined(LIBRE3)
#include <sys/stat.h>
#include <string.h>
#include "inout.h"
#define _GNU_SOURCE 1
#include <dlfcn.h>
#include "settings/settings.h"

/*
static const char *owerjslkdfjlsdQQ(void) {
    Dl_info dl_info;
    dladdr((void*)owerjslkdfjlsdQQ, &dl_info);
    return(dl_info.dli_fname);
} */
extern int setenv(const char *name, const char *value, int overwrite);



void usepath(std::string_view libdirname,std::string_view filesdir) {
#if defined(__aarch64__) 
   if(settings->data()->triedasm&&!settings->data()->asmworks&& settings->data()->setpathworks) 
#else
   if(settings->data()->setpathworks) 
#endif
	{
	pathconcat bindir(filesdir, "bin");
	const char *bindirstr=bindir.data();
	LOGGER("usepath(%s) use\n",libdirname.data());
	mkdir(filesdir.data(),0700);

	mkdir(bindirstr,0700);
	LOGGER("FILESDIR=%s, bindir=%s\n",filesdir.data(),bindirstr);
	if(setenv("PATH", bindirstr, 1)) {
		flerror("setenv(PATH,%s,1)\n",bindirstr);
		}


const char pmcomstart[]=R"(export PATH=/system/bin
echo package:)";

	constexpr const int pmstartlen=sizeof(pmcomstart)-1;




	int pathlen=libdirname.size();
	constexpr const char endname[]="/libinit.so";
	constexpr const int endnamelen=sizeof(endname)-1;
extern std::string_view libdirname;
	char pmcom[pmstartlen+pathlen+endnamelen+2];
	memcpy(pmcom,pmcomstart,pmstartlen);
	int pos=pmstartlen;
	memcpy(pmcom+pos,libdirname.data(),pathlen);
	pos+=pathlen;
	memcpy(pmcom+pos,endname,endnamelen);
	pos+=endnamelen;
	strcpy(pmcom+pos,"\n");
	pos++;

	pathconcat cmd(bindir,"cmd");	
 	writeall(cmd,pmcom,pos);
	chmod(cmd, 0755);
	pathconcat app(bindir,"app_process");

 	writeall(app,pmcom,pos);
	chmod(app, 0755);
	}
else  {
	LOGGER("usepath(%s) dont use \n",libdirname.data());
	}
	}
#else
void usepath(std::string_view) {
	}
#endif
