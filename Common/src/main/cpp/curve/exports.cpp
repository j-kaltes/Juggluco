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
/*      Fri Jan 27 15:20:04 CET 2023                                                 */


#include "settings/settings.h"
#include "SensorGlucoseData.h"
#include "sensoren.h"
#include "numiter.h"

extern Sensoren *sensors;


#define DATESTRING "UnixTime\tYYYY-mm-dd-HH:MM:SS"
constexpr int veilig=
#ifdef NDEBUG
		5;
#else
	0;
#endif
/*
auto    timedata(const time_t tim) {
        struct tm tmbuf;
         struct tm *stm=localtime_r(&tim,&tmbuf);
     struct {
        char buf[28+veilig];
      float zone;
        } ret;
//      sprintf(ret.buf,"%d-%02d-%02d-%02d:%02d:%02d",1900+stm->tm_year,stm->tm_mon+1,stm->tm_mday,stm->tm_hour,stm->tm_min,stm->tm_sec);
      int zone=timegm(stm) - tim;
        cerr<<"zone="<<zone<<endl;
      int min=zone/60;
        cerr<<"min="<<min<<endl;
      int hour=min/60;
        cerr<<"hour="<<hour<<endl;
      int minleft=abs(min)-abs(hour*60);
        cerr<<"minleft="<<minleft<<endl;;
      if(minleft==0) {
              sprintf(ret.buf,"%d-%02d-%02d %02d:%02d:%02d GMT%+d",1900+stm->tm_year,stm->tm_mon+1,stm->tm_mday,stm->tm_hour,stm->tm_min,stm->tm_sec,hour);
              }
      else {
              sprintf(ret.buf,"%d-%02d-%02d %02d:%02d:%02d GMT%+d:%d",1900+stm->tm_year,stm->tm_mon+1,stm->tm_mday,stm->tm_hour,stm->tm_min,stm->tm_sec,hour,minleft);
              }
      return {ret,zone/(60.0f*60.0f)};
    }
*/
auto    timedata(const time_t tim) {
	struct tm tmbuf;
	 struct tm *stm=localtime_r(&tim,&tmbuf);
//       struct tm *stm=localtime(&tim);
     struct { 
     	char buf[21+veilig];
	float zone;
	} ret;  
      sprintf(ret.buf,"%d-%02d-%02d-%02d:%02d:%02d",1900+stm->tm_year,stm->tm_mon+1,stm->tm_mday,stm->tm_hour,stm->tm_min,stm->tm_sec);
      int zone=timegm(stm) - tim;
      ret.zone=zone/(60.0f*60.0f);
      return ret;
    }

NumIter<Num> *mknumiters();
extern vector<Numdata*> numdatas;
NumIter<Num> *mknumiters() {
	int basecount=numdatas.size();
	NumIter<Num> *numiters=new NumIter<Num>[basecount];
	for(int i=0;i<basecount;i++) {
		numiters[i]={.begin=numdatas[i]->begin(),.end=numdatas[i]->end()-1,.bytes=sizeof(Num) };
		}
	return numiters;
	}



template <class T>
void tostart(NumIter<T> *numiters,int maxnum) {
	for(int i=0;i<maxnum;i++) {
		numiters[i].iter=numiters[i].begin;
		}
	}
template <class T>
void toend(NumIter<T> *numiters,int maxnum) {
	for(int i=0;i<maxnum;i++) {
		numiters[i].iter=numiters[i].end;
		}
	}
#ifdef NOFDOPEN
int myprintf(int handle,const char *format, ...) {
        va_list args;
        va_start(args, format);
	constexpr const int maxbuf=1024;
	char buffer[maxbuf];
	int tot=vsnprintf(buffer,maxbuf,format ,args);
        va_end(args); 
	return write(handle,buffer,tot);
	}

typedef int myfilep;
#define myclose(fp) close(fp)
#define myopen(handle) handle
#else
typedef FILE*  myfilep;
#define myopen(handle) fdopen(handle,"w")
#define myclose(fp) fclose(fp)
#define myprintf fprintf
#endif
template <class Num, class F>
bool exportdata(int handle,NumIter<Num>*numiters,int start,int basecount,const F & func )  {
myfilep fp=myopen(handle);
	
/*	FILE *fp=fdopen(handle,"w");
	if(fp==NULL) {
		LOGSTRING("exportnums fdopen failed");
		close(handle);
		return false;
		}
*/
	for(int oldest;(oldest=ifindoldest(numiters,start,basecount))>=0;) {
		const Num *num=numiters[oldest].prev();
		const int bytes=numiters[oldest].bytes;
		const Num *beg=numiters[oldest].begin;
		const int previndex=num-beg+1;
		const int index=(reinterpret_cast<const uint8_t*>(num)-reinterpret_cast<const uint8_t*>(beg))/bytes+1;
		LOGGER("previndex=%d index=%d bytes=%d sizeof(T)=%lu\n",previndex,index,bytes,sizeof(Num));
		func(fp,index,num,oldest);
		}
	myclose(fp);
	return true;
	}

#include "glucose.h"	
/*(
bool exportscans(int handle, const std::span<const ScanData>  (SensorGlucoseData::*proc)(void) const) {	
	int totsen=sensors->last()+1;
	NumIter<ScanData> *iters=new NumIter<ScanData>[totsen];
	for(int i=0;i<totsen;i++) {
		SensorGlucoseData *hist=sensors->gethist(i);
		if(hist) {
			auto scans=(hist->*(proc))();
			const ScanData *beg=&scans.begin()[0];
			const ScanData *en=&scans.end()[0];
			iters[i]={.iter=beg,.begin=beg,.end=en-1};
			}
		else
			iters[i]={};
		}
	return exportdata(handle,iters,totsen,[](FILE *fp,const ScanData *scan,const int sens) {
		constexpr int maxbuf=20;
		char buf[maxbuf];
			timestr(buf,scan->gettime());
		fprintf(fp,"%s\t%s\t%d\t%.1f\t%.1f\t%s\n", sensors->shortsensorname(sens)->data(),buf,scan->id,scan->g/18.0,scan->ch,GlucoseNow::trendString[scan->tr]); });
	}
	*/
template <class T,class FG,class FP>
bool exports(int handle, const FG& proc,const FP& print) {	
	int totsen=sensors->last()+1;
	NumIter<T> *iters=new NumIter<T>[totsen];
	destruct _dest([iters]{delete[] iters;});
	LOGSTRING("exports: ");
	for(int i=0;i<totsen;i++) {
		SensorGlucoseData *hist=sensors->gethist(i);
		if(hist) {
			LOGGER("h%d\n",i);
			auto scans=(hist->*(proc))();
			const T *beg=&scans.begin()[0];
			const T *en=&scans.end()[0];
			iters[i]={.iter=beg,.begin=beg,.end=en-1,.bytes=sizeof(T)};
			}
		else
			iters[i]={};
		}
	
	return exportdata(handle,iters,0,totsen,print);
	}

#ifdef NDEBUG
#define EXTRA 5
#else
#define EXTRA 0
#endif
template <bool repeatids>
bool exportscans(int handle, const std::span<const ScanData>  (SensorGlucoseData::*proc)(void) const) {	
#define scanhead "Sensorid\tnr\t" DATESTRING "\tTZ\tMin\t"
	const char headend[]="\tRate\tChangeLabel\n";
	constexpr const int headstart= sizeof(scanhead)-1;
	char header[headstart+6+sizeof(headend)+EXTRA]=scanhead;
	auto units=unitlabels[settings->data()->unit];
	memcpy(header+headstart, units.data(),units.size());
	memcpy(header+headstart+units.size(),headend,sizeof(headend));
	int tothead=headstart+units.size()+sizeof(headend)-1;
	if(write(handle,header,tothead)!=tothead) {
		lerror("exportscans");
		return false;
		}

	return exports<ScanData>(handle,proc, [](myfilep fp,const int index,const ScanData *scan,const int sens) {
		static int oldid=-1;
		static const char *oldname=nullptr;
		const char *sensorname=sensors->shortsensorname(sens)->data();
		const uint32_t scantime=scan->gettime();
		static  uint32_t  oldtimes=0;
		if((repeatids&&oldtimes!=scantime)||oldid!=scan->id||oldname!=sensorname) {
			oldtimes=scantime;
			oldid=scan->id;
			oldname=sensorname;
			const auto [buf,zone]=	timedata(scantime);
			fprintf(fp,"%s\t%d\t%u\t%s\t%g\t%d\t%.*f\t%+g\t%s\n",sensorname ,index,scan->gettime(),buf,zone,scan->id,gludecimal,gconvert(10*scan->g),scan->ch,GlucoseNow::trendString[scan->tr]); 
			}	
		});
	} 
/*
	
		const auto [buf]=	timedata(scan->gettime());
		myprintf(fp,"%s\t%d\t%u\t%s\t%d\t%.*f\t%+g\t%s\n", sensors->shortsensorname(sens)->data(),index,scan->gettime(),buf,scan->id,gludecimal,gconvert(10*scan->g),scan->ch,GlucoseNow::trendString[scan->tr]); }		); */

bool exporthistory(int handle) {
	int totsen=sensors->last()+1;
	NumIter<Glucose> *iters=new NumIter<Glucose>[totsen];
	destruct _dest([iters]{delete[] iters;});
#define histhead "Sensorid\tnr\t" DATESTRING "\tTZ\tMin\t"
	const char headend[]="\n";
	constexpr const int headstart= sizeof(histhead)-1;
	char header[headstart+6+sizeof(headend)+EXTRA]=histhead;
	auto units=unitlabels[settings->data()->unit];
	memcpy(header+headstart, units.data(),units.size());
	memcpy(header+headstart+units.size(),headend,sizeof(headend));
	int tothead=headstart+units.size()+sizeof(headend)-1;

//	write(handle,header,tothead);
	if(write(handle,header,tothead)!=tothead) {
		lerror("exporthistory");
		return false;
		}
	int start=0;
	for(int i=start;i<totsen;i++) {
		SensorGlucoseData *hist=sensors->gethist(i);
		if(hist) {
			const int begp=hist->getstarthistory();
			const int endp=hist->getendhistory()-1;
			const Glucose *beg= hist->getglucose(begp);
			const Glucose *en= hist->getglucose(endp);
			
			iters[i]={.iter=beg ,.begin=beg,.end=en,.bytes=hist->glucosebytes()};
			}
		else  {
			if(start==i)
				++start;
			iters[i]={};
			}

		}
	LOGGER("exporthistory start=%d totsen=%d\n",start,totsen);
	if(start<totsen)
		return exportdata(handle,iters,start,totsen,
	[](myfilep fp,const int index,const Glucose *glu,const int sens) {
			const auto [buf,zone]=	timedata(glu->gettime());
			fprintf(fp,"%s\t%d\t%u\t%s\t%g\t%d\t%.*f\n", sensors->shortsensorname(sens)->data(),index,glu->gettime(),buf,zone,glu->getid(),gludecimal,gconvert(glu->getsputnik())); 

			});

	else
		return false;

	}
bool exportnums(int handle) {
	NumIter<Num> *numiters=mknumiters();
	destruct _dest([numiters]{delete[] numiters;});
	int basecount=numdatas.size();
	tostart(numiters,basecount);
	const char header[]="Source\tnr\t" DATESTRING "\tTZ\tValue\tLabel\n";
	constexpr int headlen=sizeof(header)-1;
	if(write(handle,header,headlen)!=headlen)
		return false;
	return exportdata(handle,numiters,0,basecount, [](myfilep fp,const int index,const Num *num,const int oldest) {
			const char *label=settings->getlabel(num->type).data();
			const auto [buf,zone]=timedata(num->gettime());
			fprintf(fp,"%d\t%d\t%u\t%s\t%g\t%g\t%s\n",oldest,index+numdatas[oldest]->getfirstpos(),num->gettime(),buf,zone,num->value,label);
		});



	}
//template int ifindnewest<Num>(NumIter<Num> *,int);
//template const Num *findnewest(NumIter<Num> *nums,int count) ;
//template const Num *findoldest(NumIter<Num> *nums,int count) ;



#ifdef USE_MEAL
//extern bool savemeal(int handle,const Num *num)  ;
#include "meal/MealSave.h"
extern bool allsavemeals(int handle) ;

bool savemeals(int handle) {
	MealSave mealsave;
	extern vector<Numdata*> numdatas;
	NumIter<Num> *numiters=mknumiters();
	destruct _dest([numiters]{delete[] numiters;});
	int basecount=numdatas.size();
	tostart(numiters,basecount);
	for(int oldest;(oldest=ifindoldest(numiters,0,basecount))>=0;) {
		const Num *num=numiters[oldest].prev();
		if(num->mealptr) {
			if(!mealsave.savemeal(handle,num))
				return false;
			}
		}
	return true;
	}
#endif

#ifdef JUGGLUCO_APP
#include <jni.h>
#include "fromjava.h"
extern "C" JNIEXPORT jboolean  JNICALL   fromjava(exportdata)(JNIEnv *env, jclass cl,jint type,jint fd) {
	switch(type) {
		case 0: return exportnums(fd);	;
		case 1: return exportscans<true>(fd, &SensorGlucoseData::getScandata);
		case 2: return exportscans<false>(fd, &SensorGlucoseData::getPolldata);
		case 3: return exporthistory(fd);
#ifdef USE_MEAL
		case 4: return allsavemeals(fd);
#endif
		};
	return false;
	}
#else
template bool exportscans<true>(int handle, const std::span<const ScanData>  (SensorGlucoseData::*proc)(void) const) ;
template bool exportscans<false>(int handle, const std::span<const ScanData>  (SensorGlucoseData::*proc)(void) const) ;	
#endif
