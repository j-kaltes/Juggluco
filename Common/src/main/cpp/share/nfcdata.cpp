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


  #define _XOPEN_SOURCE

//#include "history.h"
#include <algorithm>
#include "alg.h"
#include "nfcdata.h"
#include "settings/settings.h"



//s/\<\([ht][ir][se][a-z]*\)data\>/\1/g

#ifdef MAIN
extern int timestring(time_t tim,char *buf,int max) ;
void nfcdata::showhistory() {
const int maxt=100;
char timestr[maxt];

int    sensorAgeInMinutes = getSensorAgeInMinutes();
   int mostRecentHistoryAgeInMinutes = 3 + (sensorAgeInMinutes - 3) % historyIntervalInMinutes;
        long onmindate=(gettime()+30)/60;
        for (int counter = 0; counter < history::num; counter++) {
            int glucoseLevelRaw  = gethistoryglucose(counter);
            // skip zero values if the sensor has not filled the ring buffer yet completely
            if (glucoseLevelRaw > 0) {
                int dataAgeInMinutes = mostRecentHistoryAgeInMinutes + (history::num - (counter + 1)) * historyIntervalInMinutes;
		time_t wastime=(onmindate-dataAgeInMinutes)*60;
	timestring(wastime,timestr,maxt);
	//		int sum=gethistglucose<uint32_t>(counter);
		outf("%s %.1f %d\n",timestr,glucoseLevelRaw/convfactor,glucoseLevelRaw);
            }
        }
	}

void nfcdata::showgegs() {
const int maxt=100;
char timestr[maxt];
        long onmindate=(gettime()+30)/60;
int    sensorAgeInMinutes = getSensorAgeInMinutes();

        time_t startDate=(onmindate-sensorAgeInMinutes)*60;
	timestring(startDate,timestr,maxt);
	cout<<"Sensor start: "<<timestr<<endl<<endl;
        // read trend values from ring buffer, starting at indexTrend (bytes 28-123)
        for (int counter = 0; counter < trend::num; counter++) {
            int glucoseLevelRaw = gettrendglucose(counter);
            if (glucoseLevelRaw > 0) {
                int dataAgeInMinutes = trend::num - counter-1;
		time_t wastime=(onmindate-dataAgeInMinutes)*60;
//		int  sum=getTrendrest<uint32_t>(counter);
		timestring(wastime,timestr,maxt);
		outf("%s %.1f %d\n",timestr,glucoseLevelRaw/convfactor,glucoseLevelRaw);

            }
        }
	}
#endif
//time_t totime_t(const char *name) {
#include <string_view>
//2020-10-18-01:55             
time_t totime_t(std::string_view inp) {
//const char *name=strrchr(inp.data(),'/');
int len=inp.length();
if(len<16)
	return -1;
auto pos=inp.find_last_of('/',len-15);
const char *name=inp.data()+( (pos== std::string_view::npos)? 0: pos+1);
//const char *end = inp.data()+inp.length();
const char *end = inp.end();
const char *num=std::find_if_not(name,end,[](char val) {return isdigit(val)==0;});
	if(end==num) {
		cerr<<name<<"No date in filename"<<endl;
		return -1;
		}
	struct tm tms{ .tm_isdst=-1};
//	time_t tt=time(NULL);
//	localtime_r(&tt,&tms);
//	const char *tot=strptime(num, "%Y-%m-%d-%H:%M",&tms);
	const char *tot=strptime(num, "%Y-%m-%d-%H:%M:%S",&tms);
	if(tot!=end) {
		cerr<<"characters after time: "<<tot<<endl;;
		}
	return mktime(&tms);
}

#ifdef MAIN
constexpr const int maxsensormin=15*24*60;
bool nfcdata::testdata() {
int    sensorAgeInMinutes = getSensorAgeInMinutes();
if(sensorAgeInMinutes>maxsensormin) {
	outf("Old sensor? %.1f days (%d minutes) old?\n", sensorAgeInMinutes/(24.0*60.0), sensorAgeInMinutes);
	return false;
	}
if(sensorAgeInMinutes<60) {
	outf("New sensor? %d minutes old?\n",  sensorAgeInMinutes);
	return false;
	}

	time_t now=gettime();
   int mostRecentHistoryAgeInMinutes = 3 + (sensorAgeInMinutes - 3) % historyIntervalInMinutes;
        long onmindate=(gettime()+30)/60;
const int maxt=100;
char timestr[maxt];
	//TODO: test aansluiting bestaande waarden
        for (int counter=0; counter < history::num; counter++) {
            int glucoseLevelRaw  = gethistoryglucose(counter);
	    int dataAgeInMinutes = mostRecentHistoryAgeInMinutes + (history::num - (counter + 1)) * historyIntervalInMinutes;
            if (glucoseLevelRaw > 0) {
		if(dataAgeInMinutes >= sensorAgeInMinutes ) {
			time_t wastime=(onmindate-dataAgeInMinutes)*60;
			timestring(wastime,timestr,maxt);
			outf("%s: %.1f, before start sensor\n",timestr,glucoseLevelRaw/convfactor);
			return false;
			}
		  }
	    else {
		   if(dataAgeInMinutes < sensorAgeInMinutes ) {
			time_t wastime=(onmindate-dataAgeInMinutes)*60;
			timestring(wastime,timestr,maxt);
			outf("%s: Glucose 0.0\n",timestr);
//			return false;
			}
		   }

            }
	   return true;
   }

#include <stdio.h>
       #include <time.h>
       #include <string.h>
#include <algorithm>
       #include <ctype.h>


int main(int argc, char **argv) {
if(argc<2) {
	cout<<"Usage: "<<argv[0]<<" filenames"<<endl<<"Displays content of freelibre nfc data"<<endl;
	return 2;
	}
for(int i=1;i<argc;i++) {
const char *name=argv[i];
  int    fp= open(name,O_RDWR|O_CREAT,S_IRUSR |S_IWUSR);
        if(fp==-1) {
		cout<<"open "<<name<<" failed"<<endl;
		return 3;
		}
struct stat statbuf;
    if(fstat(fp, &statbuf)) {
	    cout<<"fstat: "<<strerror(errno)<<endl;
    	}
int mmaplen=statbuf.st_size;
        uint8_t *mmapbuf=(uint8_t *)mmap(NULL, mmaplen, PROT_READ |PROT_WRITE,MAP_SHARED, fp, 0);
       close(fp);
       if(mmapbuf== MAP_FAILED) {
	    cout<<"mmap: "<<strerror(errno)<<endl;
             return 4;
            }

	time_t wastim=totime_t(name);
	outf("%s data made at %s",name,ctime(&wastim));
	   nfcdata dat(mmapbuf, wastim);
	   if(!dat.testdata()) {
	   	cerr<<"Error\n";
		munmap(mmapbuf,mmaplen);
		continue;
	   	}
	   cout<<"History:"<<endl;
	   dat.showhistory();
	   cout<<"Trends:"<<endl;
	   dat.showgegs();
	   History hist;
	   hist.savehistory(&dat);
	   cout<<"All history"<<endl;
	   char *his=	   hist.readhistorys();
	   write(STDOUT_FILENO,his,strlen(his));
	   hist.exporthistory("/tmp/hist.tsv");
//	   hist.readhistory();
		munmap(mmapbuf,mmaplen);
    }
}

#endif
