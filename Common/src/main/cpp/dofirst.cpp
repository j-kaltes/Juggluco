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

extern bool globalsetpathworks;
bool globalsetpathworks=false;


#include "config.h"
#include <string_view>

#include "inout.hpp"

#if ! defined(NEEDSPATH)
pathconcat mkbindir(std::string_view subdir,std::string_view libname ) {
	return "";
	}

void usepath() {
	}
#else
#include <sys/stat.h>
#include <string.h>
#define _GNU_SOURCE 1
#include <dlfcn.h>
#include "settings/settings.hpp"

/*
static const char *owerjslkdfjlsdQQ(void) {
    Dl_info dl_info;
    dladdr((void*)owerjslkdfjlsdQQ, &dl_info);
    return(dl_info.dli_fname);
} */
extern int setenv(const char *name, const char *value, int overwrite);

extern std::string_view globalbasedir;
static bool	getpathworks() {
	pathconcat testprog(globalbasedir,"testprog");
	const char commando[]{"#!/system/bin/sh\ntrue\n"};
	writeall(testprog,commando,sizeof(commando)-1);
	chmod(testprog,0700);
	constexpr const char path[]="PATH";
	const char *oldpath=getenv(path);
	setenv(path,globalbasedir.data(),1);
	int err=system("testprog");
	LOGGER("testprog gives %d\n",err);
	setenv(path,oldpath,1);
	unlink(testprog.data());
	return !err;
	}


/*
	if(setenv("PATH", bindirstr, 1)) {
		flerror("setenv(PATH,%s,1)\n",bindirstr);
		} */

extern std::string_view libdirname;
pathconcat mkbindir(std::string_view subdir,std::string_view libname ) {

	pathconcat bindir(globalbasedir, subdir);
	const char *bindirstr=bindir.data();
	LOGGER("mkbindir(%s) use\n",libdirname.data());

	mkdir(bindirstr,0700);
	LOGGER("FILESDIR=%s, bindir=%s\n",globalbasedir.data(),bindirstr);

const char pmcomstart[]=R"(export PATH=/system/bin
echo package:)";

	constexpr const int pmstartlen=sizeof(pmcomstart)-1;




	const int pathlen=libdirname.size();
//	constexpr const char endname[]="/libinit.so";
	 const char *endname=libname.data();
	 const int endnamelen=libname.size();
	char pmcom[pmstartlen+pathlen+endnamelen+3];
	memcpy(pmcom,pmcomstart,pmstartlen);
	int pos=pmstartlen;
	memcpy(pmcom+pos,libdirname.data(),pathlen);
	pos+=pathlen;
	pmcom[pos++]='/';
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
	return bindir;
	}


pathconcat libre3path;
void usepath() {
	globalsetpathworks=getpathworks();
#if defined(__aarch64__) 
   if(settings->data()->triedasm&&!settings->data()->asmworks&& globalsetpathworks) 
#else 
   if(globalsetpathworks) 
#endif 
	{
	LOGAR("usepath use");
	}
else  {
	LOGAR("usepath do not use");
	}
	}
#endif
