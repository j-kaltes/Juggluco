#ifndef WEAROS
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


#include "settings/settings.hpp"
#include "SensorGlucoseData.hpp"
#include "sensoren.hpp"
#include "curve/numiter.hpp"
#include "../notes/Notes.hpp"
#include "../net/libreview/numcategories.hpp"
#include <ctype.h>

extern Notes *notes;

extern Sensoren *sensors;
#define LOGGERTAG(...) LOGGER("export: " __VA_ARGS__)
#define LOGGARTAG(...) LOGGAR("export: " __VA_ARGS__)

#define DATESTRING "UnixTime\tYYYY-mm-ddTHH:MM:SS"
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
static auto    timedata(const time_t tim) {
	struct tm tmbuf;
	struct tm *stm=localtime_r(&tim,&tmbuf);
    constexpr const int maxbuf=21;
    struct { 
     	char buf[maxbuf+veilig];
        float zone;
        } ret;  
      snprintf(ret.buf,maxbuf,"%04u-%02u-%02uT%02u:%02u:%02u", 1900+stm->tm_year,stm->tm_mon+1,stm->tm_mday,stm->tm_hour,stm->tm_min,stm->tm_sec);
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
		numiters[i]={.startall=numdatas[i]->begin(),.begin=numdatas[i]->begin(),.end=numdatas[i]->end()-1,.bytes=sizeof(Num),.index=i };
		}
	return numiters;
	}

NumIter<Num> *mknumPerioditers(uint32_t starttime,uint32_t endtime) {
	const int basecount=numdatas.size();
	NumIter<Num> *numiters=new NumIter<Num>[basecount];
	for(int i=0;i<basecount;i++) {
		auto [beg,en]= numdatas[i]->getInRange(starttime,endtime);
		numiters[i]={.startall=numdatas[i]->begin(),.begin=beg,.end=en-1,.bytes=sizeof(Num),.index=i };
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
bool exportdata(myfilep fp,NumIter<Num>*numiters,int start,int basecount,const F & func ,const int maxcount)  {
	for(int oldest,count=0;count<maxcount&&(oldest=ifindoldest(numiters,start,basecount))>=0;) {
		const Num *num=numiters[oldest].prev();
		const int bytes=numiters[oldest].bytes;
		const Num *startall=numiters[oldest].startall;
		const int index=(reinterpret_cast<const uint8_t*>(num)-reinterpret_cast<const uint8_t*>(startall))/bytes+1;
		const Num *beg=numiters[oldest].begin;
		if(func(fp,index,num,numiters[oldest].index,beg))
			++count;
		}
	return true;
	}

#include "glucose.hpp"	
template <class T,class FG,class FP>
bool sensorexports(myfilep handle, const FG& proc,const FP& print,uint32_t starttime=0,uint32_t endtime=UINT32_MAX,int maxcount=INT_MAX) {	
	auto indices=sensors->sensorsInPeriod(	starttime,endtime);
	int totsen=indices.size();
	LOGGERTAG("sensorexports start time=%u endtime=%u %d indices\n",starttime,endtime,totsen);
	NumIter<T> *iters=new NumIter<T>[totsen];
	destruct _dest([iters]{delete[] iters;});
	int i=0;
	for(int id=totsen-1;id>=0;--id) {
		const int index=indices[id];
		if(SensorGlucoseData *hist=sensors->getSensorData(index)) {
			auto scans=(hist->*(proc))(starttime,endtime);
			const T *beg=&scans.begin()[0];
			const T *en=&scans.end()[0];
			#ifndef NOLOG
			if(beg!=en) {
				LOGGERTAG("h%d %u\n",index,beg->gettime());
                                }
			else {
				LOGGERTAG("h%d\n",index);
                                }
			#endif	
			iters[i++]={.startall=scans.startall(),.iter=beg,.begin=beg,.end=en-1,.bytes=sizeof(T),.index=index};
			}
		}
	
	bool res= exportdata(handle,iters,0,i,print,maxcount);
    LOGAR("end sensorexports");
    return res;
	}

#ifdef NDEBUG
#define EXTRA 5
#else
#define EXTRA 0
#endif

bool currentheader(FILE* handle,int unit,bool calibrated=false) {
	#define scanhead "Sensorid\tnr\t" DATESTRING "\tTZ\tMin\t"
	const char raw[]="\tRaw\tdiff";
	const char headend[]="\tRate\tChangeLabel\n";
	constexpr const int headstart= sizeof(scanhead)-1;
	char header[headstart+6+sizeof(headend)+sizeof(raw)+EXTRA]=scanhead;
	auto units=unitlabels[unit];
	memcpy(header+headstart, units.data(),units.size());
    int pos=headstart+units.size();
    if(calibrated) {
        int rawlen=sizeof(raw)-1;
        memcpy(header+headstart+units.size(),raw,rawlen);
        pos+=rawlen;
        }
	memcpy(header+pos,headend,sizeof(headend));
	int tothead=pos+sizeof(headend)-1;
	if(fwrite(header,tothead,1,handle)!=1) {
		lerror("exportscans");
		return false;
		}
	return true;
	}
#include "calibrate/Calibrator.hpp"


template <bool repeatids>
bool fexportscans(myfilep handle, int unit,CurData   (SensorGlucoseData::*proc)(const uint32_t,const uint32_t) const,uint32_t starttime,uint32_t endtime,int maxcount=INT_MAX,bool isCalibrated=false,bool calibratePast=false) {
	return sensorexports<ScanData>(handle,proc, [unit,isCalibrated,calibratePast](myfilep fp,const int index,const ScanData *scan,const int sensorindex,const ScanData *beg) {
		if(repeatids||scan==beg||scan->id!=scan[-1].id) {
			const char *sensorname=sensors->shortsensorname(sensorindex)->data();
			const uint32_t scantime=scan->gettime();
			const auto [buf,zone]=	timedata(scantime);
            const auto mgdL=scan->getmgdL();
            const float rawconvert=gconvert(mgdL*10,unit); 
            if(isCalibrated) {
               const SensorGlucoseData *sensdata=sensors->getSensorData(sensorindex);
               float calconvert;
               auto cali= make_calibrator<ScanData>(sensdata);
               if(double calibrated=cali.calibrateONE(*scan,calibratePast);!isnan(calibrated)) {
                  calconvert=gconvert(10.0*calibrated,unit);
                  }
              else {
                  calconvert=rawconvert;
                 }
              float diff=calconvert-rawconvert;
              const int dec= getgludecimal(unit);
              fprintf(fp,"%s\t%d\t%u\t%s\t%g\t%d\t%.*f\t%.*f\t%.*f\t%+g\t%s\n",sensorname ,index,scantime,buf,zone,scan->id,dec,calconvert,dec,rawconvert,2,diff,scan->ch,GlucoseNow::trendString[scan->tr]); 
              return true;
               }
			fprintf(fp,"%s\t%d\t%u\t%s\t%g\t%d\t%.*f\t%+g\t%s\n",sensorname ,index,scantime,buf,zone,scan->id,getgludecimal(unit),rawconvert,scan->ch,GlucoseNow::trendString[scan->tr]); 
			return true;
			}
		return false;
			},starttime,endtime,maxcount);
	} 

template <bool repeatids, CurData  (SensorGlucoseData::*proc)(const uint32_t,const uint32_t) const>
bool exportcurrent(myfilep handle,int unit,uint32_t start,uint32_t end,int maxcount=INT_MAX,bool calibrated=false,bool calibratePast=false) {
	return fexportscans<repeatids>(handle,unit,proc,start,end,maxcount,calibrated,calibratePast);
	}
template <bool repeatids>
bool exportscans(int handle,  CurData  (SensorGlucoseData::*proc)(const uint32_t,const uint32_t) const,uint32_t starttime=0,uint32_t endtime=UINT32_MAX,int maxcount=INT_MAX,bool calibrated=false,bool calibratePast=false) {
	myfilep fp=myopen(handle);
	if(!fp) {
		close(handle);
		return false;
		}
	const int unit= settings->data()->unit;
	if(!currentheader(fp,unit,calibrated))  {
      		myclose(fp);
		return false;
		}
      bool ret=fexportscans<repeatids>(fp,unit,proc,starttime,endtime,maxcount,calibrated,calibratePast);
      myclose(fp);
      return ret;
	}
static bool writehistoryheader(FILE *handle,int unit,bool calibrated=false) {
#define histhead "Sensorid\tnr\t" DATESTRING "\tTZ\tMin\t"
	constexpr const int headstart= sizeof(histhead)-1;
	constexpr const char headend[]="\n";
	constexpr const char headcalibratedend[]{"\tRaw\tdiff\n"};
	char header[headstart+6+sizeof(headcalibratedend)+EXTRA]=histhead;
	auto units=unitlabels[unit];
	memcpy(header+headstart, units.data(),units.size());
    const int headsize=(calibrated?sizeof(headcalibratedend):sizeof(headend))-1;

	memcpy(header+headstart+units.size(),calibrated?headcalibratedend:headend,headsize+1);
	int tothead=headstart+units.size()+headsize;
	if(fwrite(header,tothead,1,handle)!=1) {
		lerror("exporthistory");
		return false;
		}
	return true;
	}
bool fexporthistory(myfilep  handle,int unit,uint32_t starttime=0,uint32_t endtime=UINT32_MAX,int maxcount=INT_MAX,bool calibrated=false,bool calibratePast=false) {
	const auto indices=sensors->sensorsInPeriod(starttime,endtime);
	const int totsen=indices.size();
	NumIter<Glucose> *iters=new NumIter<Glucose>[totsen];
	destruct _dest([iters]{delete[] iters;});
	int i=0;
	for(int ind=totsen-1;ind>=0;--ind) {
		const int index=indices[ind];
		SensorGlucoseData *hist=sensors->getSensorData(index);
		if(hist&&hist->hasRealHistory()) {
			const int start=hist->getfirstnotbeforetime(starttime);
			const Glucose *beg= hist->getglucose(start);
			const int allend=hist->getAllendhistory();
			const int end=hist->getfirstnotbeforetime(endtime);
			const Glucose *en= hist->getglucose(end);
			LOGGER("hist->getAllendhistory()=%d end=%d\n",allend,end);
			if(end>=allend||en->gettime()>endtime) {
				LOGAR("--en");
				--en;
				}
			iters[i++]={.startall=hist->getglucose(hist->getstarthistory()),.iter=beg ,.begin=beg,.end=en,.bytes=hist->glucosebytes(),.index=index};
			LOGGERTAG("fexporthistory starttime=%u start=%d endtime=%u end=%d\n",starttime,start,endtime,en);
			}

		}
	LOGGERTAG("exporthistory take=%d totsen=%d\n",i,totsen);
	if(i>0)  {
		return exportdata(handle,iters,0,i,
	[unit,calibrated,calibratePast](myfilep fp,const int index,const Glucose *glu,const int sens,const Glucose *beg) {
			const auto [buf,zone]=	timedata(glu->gettime());
            auto raw=gconvert(glu->getsputnik(),unit);
            if(calibrated) {
                const SensorGlucoseData *sensdata=sensors->getSensorData(sens);
               auto cali= make_calibrator<Glucose>(sensdata);
               float calconvert;
               if(double calibrated=cali.calibrateONE(*glu,calibratePast);!isnan(calibrated)) {
                  calconvert=gconvert(10.0*calibrated,unit);
                  }
              else {
                  calconvert=raw;
                 }
              float diff=raw-calconvert;
              int dec=getgludecimal(unit);
                fprintf(fp,"%s\t%d\t%u\t%s\t%g\t%d\t%.*f\t%.*f\t%.*f\n", sensors->shortsensorname(sens)->data(),index,glu->gettime(),buf,zone,glu->getid(),dec,raw,dec,calconvert,2,diff); 
                }
            else
                fprintf(fp,"%s\t%d\t%u\t%s\t%g\t%d\t%.*f\n", sensors->shortsensorname(sens)->data(),index,glu->gettime(),buf,zone,glu->getid(),getgludecimal(unit),raw); 
			return true;

			},maxcount);
		}
	else
		return false;

	}

bool exporthistory(int handle,uint32_t starttime=0,uint32_t endtime=UINT32_MAX,int maxcount=INT_MAX,bool calibrated=false,bool calibratePast=false) {
	myfilep fp=myopen(handle);
	if(!fp) {
		close(handle);
		return false;
		}
	int unit=settings->data()->unit;
	if(!writehistoryheader(fp,unit,calibrated)) {
		myclose(fp);
		return false;
		}
	bool ret=fexporthistory(fp,unit,starttime,endtime,maxcount,calibrated,calibratePast);

	myclose(fp);
	return ret;
	}
bool fexportnums(myfilep handle,int _unit,uint32_t starttime=0,uint32_t endtime=UINT32_MAX,int maxcount=INT_MAX,bool=false,bool=false) {
	NumIter<Num> *numiters=mknumPerioditers(starttime,endtime);
	destruct _dest([numiters]{delete[] numiters;});
	int basecount=numdatas.size();
	tostart(numiters,basecount);
	return exportdata(handle,numiters,0,basecount, [](myfilep fp,const int index,const Num *num,const int oldest,const Num *beg) {
			const char *label=settings->getlabel(num->type).data();
			const auto [buf,zone]=timedata(num->gettime());
			char valbuf[notemaxlen+1];
			const char *valuetext=nullptr;
			if(isNote(num->type)&&notes) {
				// The value column holds the note text: export is TSV,
				// so all whitespace (tabs, newlines, ...) becomes a
				// space. The full text is exported, not the shortened
				// display form.
				const char *text=notes->gettext(num->mealptr);
				if(text&&*text) {
					size_t i=0;
					for(;text[i]&&i<sizeof(valbuf)-1;++i)
						valbuf[i]=isspace((unsigned char)text[i])?' ':text[i];
					valbuf[i]='\0';
					valuetext=valbuf;
					}
				}
			if(valuetext)
				fprintf(fp,"%d\t%d\t%u\t%s\t%g\t%s\t%s\n",oldest,index+numdatas[oldest]->getfirstpos(),num->gettime(),buf,zone,valuetext,label);
			else
				fprintf(fp,"%d\t%d\t%u\t%s\t%g\t%g\t%s\n",oldest,index+numdatas[oldest]->getfirstpos(),num->gettime(),buf,zone,num->value,label);
			return true;
		},maxcount);

	}

static bool writenumheader(FILE* handle,int,bool=false) {
	constexpr const char header[]="Source\tnr\t" DATESTRING "\tTZ\tValue\tLabel\n";
	constexpr int headlen=sizeof(header)-1;
	if(fwrite(header,headlen,1,handle)!=1)
		return false;
	return true;
	}
bool exportnums(int handle,uint32_t starttime=0,uint32_t endtime=UINT32_MAX,int maxcount=INT_MAX) {

	myfilep fp=myopen(handle);
	if(!fp) {
		close(handle);
		return false;
		}
	if(!writenumheader(fp,0)) {
		myclose(fp);
		return false;
		}
	bool ret=fexportnums( fp,0,starttime,endtime,maxcount);
	myclose(fp);
	return ret;
	}





#ifdef USE_MEAL
//extern bool savemeal(int handle,const Num *num)  ;
#include "meal/MealSave.hpp"
extern bool allsavemeals(int handle,uint32_t starttime,uint32_t endtime,int maxcount=INT_MAX);


extern bool savemeals(FILE* handle,uint32_t starttime,uint32_t endtime);
bool savemeals(FILE * handle,uint32_t starttime,uint32_t endtime) {
    const bool mealvar=settings->data()->mealvar;
	MealSave mealsave;
	extern vector<Numdata*> numdatas;
	//NumIter<Num> *numiters=mknumiters();

	NumIter<Num> *numiters=mknumPerioditers(starttime,endtime);
	destruct _dest([numiters]{delete[] numiters;});
	int basecount=numdatas.size();
	tostart(numiters,basecount);
	for(int oldest;(oldest=ifindoldest(numiters,0,basecount))>=0;) {
		const Num *num=numiters[oldest].prev();
		if(num->type==mealvar&&num->mealptr) {
			if(!mealsave.savemeal(handle,num))
				return false;
			}
		}
	return true;
	}
#endif

#ifdef JUGGLUCO_APP
extern bool libreviewexport(int handle,uint32_t starttime,uint32_t endtime,const bool calibrate,bool calibratePast)  ;
bool  exportdata(uint32_t starttimein, uint32_t duration,int intype,int fd,float days) {
	uint32_t endtime=std::min(starttimein+duration,(uint32_t)time(nullptr));
	uint32_t starttime=endtime-days*24*60*60;
    int type=intype&7;
    bool calibrated=intype&8;
	switch(type) {
		case 0: return exportnums(fd,starttime,endtime);	;
		case 1: return exportscans<true>(fd, &SensorGlucoseData::scanInperiod,starttime,endtime,INT_MAX,calibrated,settings->data()->CalibratePast);
		case 2: return exportscans<false>(fd, &SensorGlucoseData::streamInperiod,starttime,endtime,INT_MAX,calibrated,settings->data()->CalibratePast);
		case 3: return exporthistory(fd,starttime,endtime,INT_MAX,calibrated,settings->data()->CalibratePast);
#ifdef USE_MEAL
		case 4: return allsavemeals(fd,starttime,endtime);
#endif
		case 5: return libreviewexport(fd,starttime,endtime,calibrated,settings->data()->CalibratePast);
		};
	return false;
	}
#else
template bool exportscans<true>(int handle,  CurData  (SensorGlucoseData::*proc)(const uint32_t,const uint32_t) const,uint32_t,uint32_t,int,bool,bool) ;

template bool exportscans<false>(int handle, CurData (SensorGlucoseData::*proc)(const uint32_t,const uint32_t) const,uint32_t,uint32_t,int,bool,bool) ;
#endif


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <span>


std::span<char> getexportdata(int startpos,int len,uint32_t starttime,uint32_t endtime, int unit,bool (*exporter)(FILE *fp,int unit,uint32_t starttime,uint32_t endtime,int maxcount,bool calibrated,bool calibratePast),bool (*header)(FILE* handle,int,bool),int maxcount,bool calibrated=false,bool calibratePast=false) {
	struct Fmemopen {
		char *mem;
		int max;
		int iter;
#ifdef __ANDROID_API__
	static int  writer(void *ptr,const char *gegs,int len) 
#else
	static ssize_t  writer(void *ptr,const char *gegs,size_t len) 
#endif
        {
		Fmemopen *mem=(Fmemopen*)ptr;
		int end=mem->iter+len;
		if(end>=mem->max) {
			const int newmax=end*2;
			LOGGERTAG("increase to %d\n",newmax);
			char *tmp= new(std::nothrow) char [newmax];
			if(!tmp) {
				delete[] mem->mem;
				mem->mem=nullptr;
				return -1;
				}
			mem->max=newmax;
			memcpy(tmp,mem->mem,mem->iter);
			delete[] mem->mem;
			mem->mem=tmp;
			}
		memcpy(mem->mem+mem->iter,gegs,len);
		mem->iter+=len;
		return len;
		}
	};
	Fmemopen mem{.mem= new(std::nothrow) char[len],.max=len,.iter=startpos};
	if(!mem.mem)
		return {(char *)nullptr,(size_t)0};
#ifdef __ANDROID_API__
extern FILE* funopen(const void* __cookie,
              int (*__read_fn)(void*, char*, int),
              int (*__write_fn)(void*, const char*, int),
              fpos_t (*__seek_fn)(void*, fpos_t, int),
              int (*__close_fn)(void*));
#define FMEMOPEN(memptr) funopen(memptr,0,Fmemopen::writer,0,0)
#else
constexpr const cookie_io_functions_t  memfuncs = {
               .read  = nullptr,
               .write =Fmemopen::writer,
               .seek  = nullptr,
               .close =nullptr
           };

#define FMEMOPEN(memptr)  fopencookie(memptr,"w",memfuncs)
#endif
	if(FILE *fp=FMEMOPEN(&mem)) {
		if(header) {
			if(!header(fp,unit,calibrated)) {
				return {mem.mem,std::numeric_limits<size_t>::max()};
				}	
			}
		bool res=exporter(fp,unit,starttime,endtime,maxcount,calibrated,calibratePast);
		if(fclose(fp)==0&&res)
			return {mem.mem,(size_t)mem.iter};
		lerror("fclose ");	
		return {mem.mem,std::numeric_limits<size_t>::max()};
		}
	else
			return {mem.mem,std::numeric_limits<size_t>::max()};
	}
std::span<char> gethistory(int startpos, int len, uint32_t starttime, uint32_t endtime,bool header,int unit,bool overlap,int maxcount=INT_MAX,bool calibrated=false,bool calibratePast=false) {
	return getexportdata(startpos,len,starttime, endtime,unit,fexporthistory,header?writehistoryheader:nullptr,maxcount,calibrated,calibratePast);
	}
std::span<char> getamounts(int startpos, int len, uint32_t starttime, uint32_t endtime,bool header,int unit,bool overlap,int maxcount=INT_MAX,bool=false,bool calibratePast=false) {
	return getexportdata(startpos,len,starttime, endtime,unit,fexportnums,header?writenumheader:nullptr,maxcount);
	}
std::span<char> getstream(int startpos, int len, uint32_t starttime, uint32_t endtime,bool header,int unit,bool overlap,int maxcount=INT_MAX,bool calibrated=false,bool calibratePast=false) {
	return getexportdata(startpos,len,starttime, endtime,unit,exportcurrent<false,&SensorGlucoseData::streamInperiod>,header?currentheader:nullptr,maxcount,calibrated,calibratePast);
       }	
std::span<char> getscans(int startpos, int len, uint32_t starttime, uint32_t endtime,bool header,int unit,bool overlap,int maxcount=INT_MAX,bool calibrated=false,bool calibratePast=false) {
	return getexportdata(startpos,len,starttime, endtime,unit,exportcurrent<true,&SensorGlucoseData::scanInperiod>,header?currentheader:nullptr,maxcount,calibrated,calibratePast);
	}

extern bool fallsavemeals(FILE *handle,int unit,uint32_t starttime,uint32_t endtime,int maxcount,bool=false,bool=false) ;
std::span<char> getmeals(int startpos, int len, uint32_t starttime, uint32_t endtime,bool header,int unit,bool overlap,int maxcount=INT_MAX,bool=false,bool calibratePast=false) {
	return getexportdata(startpos,len,starttime, endtime,unit,fallsavemeals,nullptr,maxcount,false);
	}


extern char * writev3entry(char *outin,const ScanData *val, const sensorname_t *sensorname,bool) ;

char *nightexport(char *buffer,uint32_t starttime,uint32_t endtime,int maxcount,uint32_t &last) {
	
	char *iter=buffer,**ptr=&iter;
	myfilep  handle=0;
	uint32_t lasttime=0;
	sensorexports<ScanData>(handle,&SensorGlucoseData::streamInperiod, [ptr,&lasttime](myfilep fp,const int index,const ScanData *scan,const int sens,const ScanData *beg) {
		if(scan==beg||scan->id!=scan[-1].id) {
			lasttime=scan->gettime();
			char *res=writev3entry(*ptr,scan,sensors->shortsensorname(sens),true);
			*res++=',';
			*ptr=res;
			return true;
			}
		return false;
			},starttime,endtime,maxcount);
	last=lasttime;
	return *ptr;
	} 

#endif
