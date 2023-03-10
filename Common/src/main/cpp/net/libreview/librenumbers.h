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
/*      Fri Jan 27 12:39:13 CET 2023                                                 */



#include <cstdio>
#include <cstring>
#include <cstdint>
#include <ctime>
#include <string_view>
#include <span>
#include <algorithm>
#include "../../nums/num.h"
//#define NOLOGS_H 1
//#define OWNLOGGER 1
#include "../../inout.h"
#include "../../nums/numdata.h"
/*
inline  int loggert( const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        int res=        vfprintf(stderr, fmt, args);
        va_end(args);
        return res;
        } */
extern Mmap<Num> librenumsdeleted;

extern std::vector<Numdata*> numdatas;
inline  static  std::string_view getlabel(int type) { //TODO settings
/*constexpr static std::string_view labels[]={"Aspart","Carbohydrat","Dextro","Levemir","Bike","Walk","Blood","var8"};
	if(type<8)
		return labels[type];
	return "Unknown"; */
   	return settings->getlabel(type);
	}
    /*public enum RecordType {
        GLUCOSE(0),
        RAPID_ACTING_INSULIN(1),
        LONG_ACTING_INSULIN(3),
        FOOD(4),
        EXERCISE(5),
        ALARM(7),
        SMART_TAG(16),
        SENSOR_START(32),
        ERROR(33);
    } */
enum RecordType
{
        irapid=1,
        ilong=3,
        food=4,
        note=16,
};
template <RecordType type>
inline static int64_t mkid(int in) {
	const int64_t iter=in;
	return iter<<8|type;	
	}


	/*
inline void	addstrview(char *&uitptr,const std::string_view indata) {
	memcpy(uitptr,indata.data(),indata.size());
	uitptr+=indata.size();
	} */

#include "../../nums/libreids.h"

class Libregeg {
public:
const Num*start;
Libreids *ids;
int lastpos;
int32_t libresendnr;
const Num &getnum(int pos) const {
	return start[pos];
	}
int addnum(const Num &n,int librenr)  {
	int index=&n-start;
	ids[index].nr=librenr;
	ids[index].modified=false;
	return librenr;
	}
};
template <class LibreType>
class Numbers: public LibreType {
	using LibreType::next;
	using LibreType::space;
	using LibreType::spacenr;
	using LibreType::startentry;
	using LibreType::factorytime;
	using LibreType::timestamp;
	using LibreType::newline;
	using LibreType::librenr;
	using LibreType::dontSendNumbers;
	using  LibreType::maxitemsize;
	using  LibreType::delextra;
	public:


	static int writeentry(char *buf,const std::string_view category,const std::string_view instance,const std::string_view unitname,float units,long long recordnum,uint32_t tim,bool del) {
		char *ptr=buf;
		startentry(ptr);
		addar(ptr,R"(")");
		addstrview(ptr,category);
		addar(ptr,R"(":)");
		space(ptr);
		addar(ptr,R"(")");
		addstrview(ptr,instance);
		addar(ptr,R"(",)");
		if(units!=0.0f) {
			next(ptr);
			addar(ptr,R"(")");
			addstrview(ptr,unitname);
			addar(ptr,R"(":)");
			space(ptr);
			ptr+=sprintf(ptr,"%.1f,",units);
			}
		next(ptr);
		addar(ptr, R"("extendedProperties":)");
		space(ptr);
		*ptr++='{';
		next(ptr);
		spacenr(ptr,2);
		addar(ptr,R"("factoryTimestamp":)");
		space(ptr);
		ptr+=factorytime(ptr,tim);
		if(units==0.0) {
			*ptr++=',';
			newline(ptr);
			spacenr(ptr,12);
			addar(ptr, R"("text":)");
			space(ptr);
			addstrview(ptr,unitname);
			}
		if(del) {
			*ptr++=',';
			newline(ptr);
			spacenr(ptr,12);
			addar(ptr,R"("action":)");
			space(ptr);
			addar(ptr,R"("deleted")");
	    		}
		next(ptr);
		addar(ptr,R"(},)");
		next(ptr);
		addar(ptr,R"("recordNumber":)");
		space(ptr);
		ptr+=sprintf(ptr,"%lld,",recordnum);
		next(ptr);
		addar(ptr,R"("timestamp":)");
		space(ptr);
		ptr+=timestamp(ptr,tim);
		newline(ptr);
		spacenr(ptr,8);
		*ptr++='}';
		*ptr++=',';
		return ptr-buf;
		}
	static int writeinsulin(char *buf,const std::string_view type,float units,long long recordnum,uint32_t tim,bool del=false) {
		return writeentry(buf,"insulinType",type,"units",units,recordnum,tim,del);
		}

	static int writefood(char *buf,const std::string_view type,float units,long long recordnum,uint32_t tim,bool del=false) {
		return writeentry(buf,"foodType",type,"gramsCarbs",units,recordnum,tim,del);
		}
	static int writenote(char *buf,std::string_view label,long long recordnum,uint32_t tim,bool del=false) {
		return writeentry(buf, "type", "com.abbottdiabetescare.informatics.customnote",label,0.0f,recordnum,tim,del);
		}
#include "numcategories.h"
/*
static float longWeight(uint32_t type) { 
	if(type>=settings->varcount())
		return 0.0f;
	if(settings->data()->librenums[type].kind==2)
		return 1.0f;
	return 0.0f;
	}	
static float rapidWeight(uint32_t type) { 
	if(type>=settings->varcount())
		return 0.0f;
	if(settings->data()->librenums[type].kind==1)
		return 1.0f;
	return 0.0f;
	}	
static float carboWeight(int type) { 
	if(type>=settings->varcount())
		return 0.0f;
	if(settings->data()->librenums[type].kind==3)
		return settings->data()->librenums[type].weight;
	return 0.0f;
	}
static bool isNote(int type) { 
	if(type>=settings->varcount())
		return false;
	return settings->data()->librenums[type].kind==4;
	}
	*/
static std::string_view getmealtype(const Num &num) {
	if(num.value>50.0f) {
		time_t tim=num.time;
		 struct tm tmbuf;
		localtime_r(&tim,&tmbuf);
		const auto h=tmbuf.tm_hour;
		if(h>3) {
			if(h<11) {
				return "Breakfast";
				}
			if(h<16) {
				return "Lunch";
				}
			if(h<22) {
				return "Dinner";
				}
			}	
		}
	return "Snack";
	}
//TODO: sort to be deleted items on librenr
public:
 void foodel(char *&ptr,const Num &n,Libregeg *ids,bool del=false) {
	if(n.value!=0.0f) {
		auto carbo=carboWeight(n.type);
		if(carbo!=0.0f) {
			auto recordnum=mkid<food>(del?n.librenr:ids->addnum(n,nextlibrenr()));
			int len=writefood(ptr,getmealtype(n),n.value*carbo,recordnum,n.time,del);
			ptr+=len;
			}
		}
	}



 void restel(char *&ptr,const Num &n,Libregeg *ids,bool del=false) {
	if(isNote(n.type)) {
#ifdef NDEBUG
#define EXTRALABEL 10
#else
#define EXTRALABEL 0
#endif
		auto recordnum=mkid<note>(del?n.librenr:ids->addnum(n,nextlibrenr()));
		constexpr const int buflen=12+13+EXTRALABEL;
		char label[buflen],*labelptr=label;
		std::string_view typestr=getlabel(n.type);
		*labelptr++='"';
		addstrview(labelptr,typestr);
		int startlen=labelptr-label;
		int over=buflen-startlen;
		int getlen=snprintf(labelptr,over,R"( %g")",n.value);
		const int totlen= getlen+startlen;
		int len= writenote(ptr,std::string_view(label,totlen),recordnum,n.time, del);
		LOGGERN(label,totlen);
		ptr+=len;
		}
	}

//inline static bool logall=false;
 void insulinel(char *&ptr,const Num &n,Libregeg *ids,bool del=false) {
 /*	if(logall) {
		time_t tim=n.time;
		LOGGER("insulinel type=%d %.1f %d %s",n.type,n.value,tim,ctime(&tim));
		} */
	if(n.value!=0.0f) {
		auto rapid=rapidWeight(n.type);
		if(rapid!=0.0f) {
			static constexpr const std::string_view	rapidname{"RapidActing"};
			int librenr;			
			if(del) {
				librenr=n.librenr;
				}
			else {
				librenr=ids->addnum(n,nextlibrenr());
				}
			auto recordnum=mkid<irapid>(librenr);

			int len=writeinsulin(ptr,rapidname,n.value*rapid,recordnum,n.time,del);
			ptr+=len;
			}
		auto ilongw=longWeight(n.type);
		if(ilongw!=0.0f) {
			static constexpr const std::string_view	longname{"LongActing"};
			int librenr;			
			if(del) {
				librenr=n.librenr;
				}
			else {
				librenr=ids->addnum(n,nextlibrenr());
				}

			auto recordnum=mkid<ilong>(librenr);

			int len=writeinsulin(ptr,longname,n.value*ilongw,recordnum,n.time,del);
			ptr+=len;
			}
		}
	}
//template <void Numbers::fun(char *&,const Num &,Libregeg *,bool)>

template <void (Numbers::*fun)(char *&,const Num &,Libregeg *,bool)>
 int writespan(char *buf,std::span<const Num> nums,Libregeg *ids,bool del=false) {
 	LOGGER("writespan(del=%d\n",del);
	char *ptr=buf;
	for(const auto &n:nums) {
		(this->*fun)(ptr,n,ids,del);
		}
	return ptr-buf;
	}

template <void (Numbers::*fun)(char *&,const Num &,Libregeg *,bool)>
//template <void Numbers::fun(char *&,const Num &,Libregeg *,bool)>
 int writeindex(char *buf,std::span<int32_t> indices,Libregeg *ids) {
 	LOGGER("writeindex\n");
	char *ptr=buf;
	for(auto index:indices) {
		if(index>=0) {
			LOGGER("getnum(%d)\n",index);
			const Num &n=ids->getnum(index);
			(this->*fun)(ptr,n,ids,false);
			}
		}
	return ptr-buf;
	}
/*	
static constexpr auto writeallInsulin=writespan<insulinel>;
static constexpr auto writeallrest=writespan<restel>;
static constexpr auto writeallfood =writespan<foodel>;
*/

//Mmap<Libreids> libreids;
//Mmap<int32_t> librechanged;
//int32_t &getlibresend()  
//int32_t &getlibrechangednr()  
/*
class Libregeg {
public:
const Num*start;
Libreids *ids;
.int=int32_t NRlibre;
const Num &getnum(int pos) const {
	return start[pos];
	}
void addnum(const Num &n,int librenr)  {
	int index=&n-start;
	ids[index].nr=librenr;
	ids[index].modified=false;
	}
int addnum(const Num &n,nextlibrenr())  {
	int librenr=++NRlibre;
	addnum(n,librenr);
	return librenr;
	}
}; */
private:
Libregeg *ids;
int libreNR;
bool didwrite=false;

int nextlibrenr() {
	const int ret= libreNR++;
	LOGGER("nextlibrenr()=%d\n",ret);
	return ret;
	}
public:
~Numbers() {
	delete[] ids;
	}
static int spaceneeded() {
	if(dontSendNumbers())
		return 0;
	int deleted=settings->data()->libredeletednr;
	int total=deleted;
	const uint32_t from =time(nullptr)-librekeepsecs;
	for(auto *numdat:numdatas) {
		total+=(numdat->getlastpos()-numdat->nextlibresend(from));
		total+=numdat->getlibrechangednr();
		}
//	return total*315+deleted*34;

	 return total*maxitemsize +deleted*delextra;
	}
	
bool didsend() { 
	return didwrite;
	}
	
void onSuccess() { 
	if(dontSendNumbers())
		return;
	LOGGER("Numbers::onSuccess\n");	
	settings->data()->libredeletednr=0;
	const int nnr=numdatas.size();
	for(int i=0;i<nnr;i++) {
		Numdata*numdat=numdatas[i];
		numdat->getlibrechangednr()=0;
		numdat->getlibresend()=ids[i].lastpos;
		LOGGER("numdat->getlibresend()=%d should be %d\n",numdat->getlibresend(),ids[i].lastpos);

		}
 	librenr()=libreNR;
	LOGGER("next libreNR=%d\n",libreNR);
	}
//	int lnr=librenr();
//	Libregeg ids[nnr];
//	auto lnr=librenr();

Numbers():ids(new Libregeg[numdatas.size()]),libreNR(librenr()) {
	if(dontSendNumbers())
		return;
	LOGGER("libreNR=%d\n",libreNR);
	const int nnr= numdatas.size();
	const uint32_t from =time(nullptr)-librekeepsecs;
	for(int i=0;i<nnr;i++) {
		Numdata*numdat=numdatas[i];
		const auto lastpos=numdat->getlastpos(); 
		if(lastpos>=numdat->getfirstpos()) {
			const Num *nums= numdat->startdata();
			const int32_t sendnr=numdat->nextlibresend(from);
			ids[i]={nums,numdat->libreids.data(),lastpos,sendnr};
			}
		else
			ids[i]={.lastpos=0};
		}
	}
template <void (Numbers::*fun)(char *&,const Num &,Libregeg *,bool)>
 int writeallnolast(char *buf) { 
	if(dontSendNumbers())
		return 0;
//	settings->data()->libredeletednr=1; //TODO remove!!
	char *ptr=buf;
	const size_t nrdeleted=settings->data()->libredeletednr;
	if(nrdeleted>0) {
		Num *deleted=librenumsdeleted.data();
		std::sort(deleted,deleted+nrdeleted,[](const Num &one, const Num &two) {return one.time<two.time;});
		ptr+=writespan<fun>(ptr,{deleted,nrdeleted},nullptr,true);
		}
	const int nnr=numdatas.size();
	LOGGER("write all\n");
	for(int i=0;i<nnr;i++) {
		const auto lastpos=ids[i].lastpos;
		if(lastpos>0) {
			Numdata*numdat=numdatas[i];
			size_t changenr= numdat->getlibrechangednr();
			if(changenr>0) {
				int32_t *changes=numdat->librechanged.data();
//				changes[2]=3;
				ptr+=writeindex<fun>(ptr,{changes,changenr},ids+i);
				}
			int32_t sendnr=ids[i].libresendnr;;
			LOGGER("%d: changenr=%lu lastpos=%d numdat->nextlibresend(from)=%d\n",i,changenr,lastpos,sendnr);
			const size_t nownr=lastpos-sendnr;
			LOGGER("nownr=%lu\n",nownr);
			if(nownr>0)  { 
				const Num *nums= ids[i].start;
				ptr+=writespan<fun>(ptr,{nums+sendnr,nownr},ids+i,false);
				}
			}
		}
	auto ret= ptr-buf;
	if(ret)
		didwrite=true;
	return ret;
	}
template <void (Numbers::*fun)(char *&,const Num &,Libregeg *,bool)>
 int writeall(char *buf) { 
 	int ind=writeallnolast<fun>(buf);
	if(ind) {
		return ind-1;
		}
	return 0;
	}

int writeallinsulin(char *buf) {
	return writeall<&Numbers::insulinel>(buf);
	}
int writeallfood(char *buf) {
	return writeall<&Numbers::foodel>(buf);
	}
int writeallnotes(char *buf) {
	return writeallnolast<&Numbers::restel>(buf);
	}
//const decltype(&Numbers<LibreType>::writeall<&Numbers<LibreType>::insulinel>) writeallInsulin=Numbers<LibreType>::writeall<&Numbers<LibreType>::insulinel>;
//const auto writeallrest=writeall<restel>;
//const auto writeallfood =writeall<foodel>;
};
//{"insulinType":"RapidActing","units":20.0,"extendedProperties":{"factoryTimestamp":"2022-09-30T09:10:00.000Z"},"recordNumber":1281,"timestamp":"2022-09-30T11:10:00.000+02:00"},



class libre3 {
protected:
static inline int factorytime(char *buf,time_t tim) {
        struct tm tmbuf;
	gmtime_r(&tim, &tmbuf);
        return sprintf(buf,R"("%d-%02d-%02dT%02d:%02d:00Z")",tmbuf.tm_year+1900,tmbuf.tm_mon+1,tmbuf.tm_mday, tmbuf.tm_hour, tmbuf.tm_min);
        }
static inline int timestamp(char *buf,time_t tim) {
        struct tm tmbuf;
        int seczone=timegm(localtime_r(&tim,&tmbuf)) - tim;
        int m=seczone/60;
        int h=m/60;     
        int minleft=m%60;
        return sprintf(buf,R"("%d-%02d-%02dT%02d:%02d:00%+03d:%02d")",tmbuf.tm_year+1900,tmbuf.tm_mon+1,tmbuf.tm_mday,tmbuf.tm_hour,tmbuf.tm_min,h,minleft);
        }
static void newline(char *&ptr) {
	*ptr++='\n';
	}
static void next(char *&ptr) {
	addar(ptr,R"(
          )");
	}
 static void spacenr(char *&ptr,int nr) {
 	memset(ptr,' ',nr);
	ptr+=nr;
 	}
 static void space(char *&ptr) {
 	*ptr++=' ';
 	}
 static void startentry(char *&ptr) {
	constexpr const char startin[]=R"(
        {
          )";
	  constexpr const int len=sizeof(startin)-1;
	memcpy(ptr,startin,len);
	ptr+=len;
	}
static bool dontSendNumbers() {
	return !settings->data()->sendnumbers||!settings->data()->libre3nums;
	}
 static int32_t & librenr() {
	return settings->data()->libre3NUMiter;
	}

inline static constexpr const int maxitemsize=400;
inline static constexpr const int delextra=50;

};
class libre2 {
protected:
static inline int factorytime(char *buf,time_t tim) {
        struct tm tmbuf;
	gmtime_r(&tim, &tmbuf);
        return sprintf(buf,R"("%d-%02d-%02dT%02d:%02d:00.000Z")",tmbuf.tm_year+1900,tmbuf.tm_mon+1,tmbuf.tm_mday, tmbuf.tm_hour, tmbuf.tm_min);
        }
static inline int timestamp(char *buf,time_t tim) {
        struct tm tmbuf;
        int seczone=timegm(localtime_r(&tim,&tmbuf)) - tim;
        int m=seczone/60;
        int h=m/60;     
        int minleft=m%60;
        return sprintf(buf,R"("%d-%02d-%02dT%02d:%02d:00.000%+03d:%02d")",tmbuf.tm_year+1900,tmbuf.tm_mon+1,tmbuf.tm_mday,tmbuf.tm_hour,tmbuf.tm_min,h,minleft);
        }
static void newline(char *&ptr) {
	}
static void next(char *&ptr) {
	}
 static void space(char *&ptr) {
 	}
 static void spacenr(char *&ptr,int nr) {
 	}

static void startentry(char *&ptr) {
	*ptr++='{';
	}
static bool dontSendNumbers() {
	return !settings->data()->sendnumbers||settings->data()->libre3nums;
	}
		
 static int32_t & librenr() {
	return settings->data()->libre2NUMiter;
	}
inline static constexpr const int maxitemsize=315;
inline static constexpr const  int delextra=34;
	};


extern pathconcat numbasedir;



#ifdef MAIN	
static void testlibre2() {
	using libre=Numbers<libre2>;
	char buf[1024];
	time_t tim=1664529000;
	int len=libre::writeinsulin(buf,"RapidActing",20,1281,tim) ;
	buf[len]='\0';
	puts(buf);
	}
static void testlibre3() {
	using libre=Numbers<libre3>;
	char buf[1024];
	time_t tim=1672135380;
	char *ptr=buf;
	//int len=libre::writeinsulin(buf,"RapidActing",13,3585,tim) ;

	//	int len=libre::writefood(ptr,"Snack",60,1028,1670595720,true);
	int len=libre::writefood(ptr, "Breakfast", 32.0,1284, 1663151100);
	buf[len]='\0';
	puts(buf);
	}
int main(int argc, char **argv) {
	using libre=Numbers<libre3>;
	size_t  totlen=500;
	char buf[totlen*1024];
	char *ptr=buf;
	Mmap<Num> maps(argv[1]);
	//printf("length=%ld\n",maps.length());
//	auto start= &maps[16+1000];
	auto start= &maps[16];
	auto orstart=start;
	Libregeg ids{start,new  Libreids[2500],1024};
	start+=1000;
	std::span<Num> one {start,totlen};
	int len1= libre::writespan<libre::insulinel>(ptr,one,nullptr,true);
	std::span<Num> two= {start+totlen,totlen};
	int len= len1+libre::writespan<libre::insulinel>(ptr+len1,two,&ids,false);
	for(int i=1500;i<2000;i++) {
		printf("%s %d\n",getlabel(ids.start[i].type).data(),ids.ids[i].nr);
		}

	buf[len]='\0';
	puts(buf);
	//testlibre3();
	}
#endif
/*
void notelibre3() {
	using libre=Numbers<libre3>;
	char buf[1024];
	time_t tim= 1668690600;
	char *ptr=buf;
	//int len=libre::writeinsulin(buf,"RapidActing",13,3585,tim) ;

	//	int len=libre::writefood(ptr,"Snack",60,1028,1670595720,true);
	int len=libre::writenote(ptr,R"("Bike")",1808,tim,false);
	buf[len]='\0';
	puts(buf);
	}
int main() {
 notelibre3();

	}
	*/

