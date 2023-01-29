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


  #include <sys/types.h>
       #include <dirent.h>
       #include <stdio.h>
#include <stdlib.h>
#include "inout.h"


void removeoldstates(const std::string_view dirin) {
	    const char *dirname=dirin.data();
	    LOGGER("removeoldstates %s\n",dirname);
//	    const int maxname=256; char statelink[maxname]; snprintf(statelink,maxname,"%s/state.lnk",dirname);
	   pathconcat  statelink(dirin,"state.lnk");
	    Readall ref(statelink.data());
	    const char *curstate=ref.data();
	    if(curstate) {
	    	pathconcat keepstate(dirin,(std::string_view)ref);
	    	struct stat st;
   	    	if(stat(keepstate.data(), &st)) {
			LOGGER("Stat %s failed\n",curstate);
			return;
			}
	       time_t lastchange=st.st_mtime;
	    	LOGGER("curstate=%s %s",curstate,ctime(&lastchange));
		DIR *dir=opendir(dirname);
		if(!dir) {
			flerror("opendir %s",dirname);
			return;
			}

		struct dirent *ent;
		while((ent=readdir(dir))) {
			const char *name=ent->d_name;
			const char state[]="state2";
			const int statelen=sizeof(state)-1;
			if(!memcmp(state,name,statelen)) {
				if(strcmp(curstate,name))   {
					pathconcat onestate(dirin,name);
					const char *lname=onestate.data();
					if(stat(lname, &st)) {
						perror(lname);
						continue;
						}
					if(st.st_mtime>=lastchange) {
						LOGGER("%s newer %s",lname,ctime(&st.st_mtime));
						continue;
						}
					if(st.st_size!=16384) {
						LOGGER("%ld!=16384",st.st_size);
						continue;
						}
					unlink(lname);
					LOGGER("unlink %s\n",lname);
					}
				else
					LOGGER("linked: %s\n",name);
				}
			}
		closedir(dir);
		}
	return ;
	}
#ifdef TESTMAIN
int main(int argc,char **argv) {
	if(argc<2) {
		fprintf(stderr,"Usage %s dir1 dir2 ... dirn\n",argv[0]);
		exit(4);
		}
	for(int i=1;i<argc;i++) {
	    const char *dirname=argv[i];
	    removeoldstates(dirname);
	}

}
#endif
