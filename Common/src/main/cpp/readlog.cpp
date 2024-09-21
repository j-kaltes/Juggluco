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


#include <iostream>
       #include <iostream>
       #include <fstream>
#include <omp.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>


#include <string>
using namespace std;
#undef LOGGER
#define LOGGER(...)   fprintf(stderr,__VA_ARGS__)
#include "strconcat.hpp"
using namespace std;
int    timestr(char *buf,time_t tim) {
        struct tm tmbuf;
        if(localtime_r(&tim,&tmbuf))
                return sprintf(buf,"%d-%02d-%02d-%02d:%02d:%02d",1900+tmbuf.tm_year,tmbuf.tm_mon+1,tmbuf.tm_mday,tmbuf.tm_hour,tmbuf.tm_min,tmbuf.tm_sec);
        else {
                const char fout[]="localtime-failed!!!";
                memcpy(buf,fout,sizeof(fout));
                return sizeof(fout)-1;
                }
    }

void retimelog(const char *name) {
	ifstream inp(name);
	strconcat outfile("",name,".out");
	ofstream outp(outfile);
	while(inp) {
		string input;
		std::getline(inp,input);
		constexpr const int endpos=10;
		if(input.size()>13) {
			char *dat=input.data();    
			char was=dat[endpos];
			dat[endpos]='\0';
			char *endptr;
			time_t tim=strtoul(dat,&endptr,10);
			if(endptr==dat+endpos&&tim>1598911200u&&tim<1914444000u) {
				char buf[25];
				int end=timestr(buf,tim);
				outp<<buf<<" "<<dat+endpos+1<<endl;
				}
			else {
				dat[endpos]=was;
				outp<<dat<<endl;
				}
			}
		else
			outp<<input<<endl;
		}
	}

int main(int argc,char **argv) {
if(argc<2) {
	cerr<<"Usage: "<<argv[0]<<" file1 [file2...file-n]\nConverts unixtime to time string at start of line\n";
	exit(0);
	}
int nr=std::min(argc-1,omp_get_num_procs());
#pragma omp parallel for num_threads(nr)
for(int i=1;i<argc;i++) {
	retimelog(argv[i]);
	}

}
