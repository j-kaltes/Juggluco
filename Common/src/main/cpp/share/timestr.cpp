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
/*      Fri Jan 27 12:37:55 CET 2023                                                 */



#include <time.h>
#include <stdio.h>
#include <string.h>


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
//    #define MAIN
#ifdef MAIN
#include <iostream>
#include <string.h>
using namespace std;
int main(void) {
    char buf[500];
	strcpy(buf+timestr(buf),"-raw.dat");
	cout<<buf<<endl;
    }
#endif
