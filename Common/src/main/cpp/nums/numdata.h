#define LOCKNUM 1
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
/*      Fri Jan 27 12:36:44 CET 2023                                                 */


#ifndef NUMDATA_H
#define NUMDATA_H

#include <algorithm>
#include <string_view>
#include <type_traits>
#include <stdint.h>
#include <memory>
#include <stdlib.h>
#include <time.h>

#include <string.h>
#include <filesystem>
#include "inout.h"
 #include <assert.h>
#ifdef LOCKNUM
#include  <mutex>
#endif
#include "settings/settings.h"
#include "net/backup.h"
#include "datbackup.h"
#include "meal/Meal.h"
#include "num.h"
//extern void wakeaftermin(const int waitmin) ;
//extern void wakeuploader();

#define LOGGERTAG(...) LOGGER("nums: " __VA_ARGS__)
#define LOGARTAG(...) LOGAR("nums: " __VA_ARGS__)
extern Meal *meals;
extern Backup *backup;
void	setnumchanged();

typedef std::conditional<sizeof(long long) == sizeof(int64_t), long long, int64_t >::type identtype; //to get rid of %lld warning
constexpr size_t nummmaplen=77056;//  ((sizeof(size_t)==4)?64ul*1024ul*1024ul:(1024ul*1024*1024ul))/sizeof(Num);
//constexpr size_t nummmaplen= sizeof(size_t)==4?(1024ul*1024*1024ul):(256*1024ul*1024*1024ul);

namespace fs = std::filesystem;
struct showitem {
  uint32_t time;
  float value;
  const char *type;
  };
struct Daypos {
int start;
int end;
};
#include "libreids.h"
class Numdata {
//static constexpr const int weightstart=4;
static constexpr const int receivedbackuppos=16;
static constexpr const int newidentpos=32;
static constexpr const int lastpolledpos=120;
static constexpr const int lastpos=128;
static constexpr const int changedpos=136;
static constexpr const int firstpos=144;
static constexpr const int onechange=152;
static constexpr const int libresend=160;
static constexpr const int librechangednr=164;
static constexpr const int libreSolid=168;
static constexpr const int nightStart=172;
static constexpr const int nightSend=176;
static constexpr const int nightSwitch=180;
static constexpr const int nightIDstart=184;
static constexpr const int datastart=256;
std::unique_ptr<char[]> newnumsfile;
protected:

identtype ident;
Mmap<Num> nums;
public:
Mmap<Libreids> libreids;
Mmap<int32_t> librechanged;
Mmap<uint32_t> changetimes;

static constexpr const std::string_view libreidsname="libreids.dat";
static constexpr const std::string_view librechangedname="librechanged.dat";
static constexpr const std::string_view changetimesname="changetimes.dat";
uint32_t changed(int pos) const {
	return changetimes[pos];
	}
bool changedsince(uint32_t tim,int pos) const {
	return changed(pos)>=tim;
	}
Numdata(const std::string_view base,identtype id,size_t len=0):newnumsfile(numfilename(base,id==0LL?0LL:-1LL)), ident(renamefile(base,id)),nums(newnumsfile.get(),len),libreids(base,libreidsname,len),librechanged(base,librechangedname,len), changetimes(base,changetimesname,len)
{
	auto lastpo=getlastpos();
	LOGGERTAG("Numdata ident=%lld lastpo=%d\n",ident,lastpo);
	if(ident!=-1LL)
		setnewident(ident);
	if(lastpo*3/2> nums.size()) {
		auto newsize=nums.size()*10;
		nums.extend(newnumsfile.get(),newsize);
		libreids.extend(base,libreidsname,newsize);
		librechanged.extend(base,librechangedname,newsize);
		changetimes.extend(base,changetimesname,newsize);
		}
	else {
		if(len)
			addmagic();
		}
	}

//Numdata(const std::string_view base):Numdata(base,readident(base)) { }
Numdata(const std::string_view base):Numdata(base,-1LL) { }


void renamefromident(std::string_view base,identtype ident) {
	auto from=numfilename(base,ident);
	if(!access(from.get(),F_OK))  {
		rename(from.get(),newnumsfile.get());
		}
	}
identtype renamefile(std::string_view base,identtype id) {
	if(id==0LL)	
		return id;
	identtype ident=readident(base);
	if(ident==-1LL) 
		return id;
	renamefromident(base,ident);
	if(id>0LL)	
		return id;
	return ident;
	}

#define  devicenr "devicenr"
//static constexpr const char devicenr[]="devicenr";
static identtype readident(const std::string_view base) {
	pathconcat ident{base,devicenr};
	Readall	all(ident);
	if(all.data())
		return *reinterpret_cast<identtype*>(all.data());
	return -1LL;
	}
static bool writeident(const std::string_view base,identtype ident) {
	const char *uit=reinterpret_cast<const char *>(&ident);
	pathconcat filename{base,devicenr};
	return writeall(filename,uit,sizeof(identtype));
	}
	

bool valid(const Num *num)const {
	uint32_t start=begin()->time;
	uint32_t endtime=(end()-1)->time;
	if(num->time&&num->time>=start&&num->time<=endtime&&num->type<settings->getlabelcount())
		return true;
	return false;
	}
	/*
uint32_t starttime= UINT32_MAX;
void firstime(void) {
	starttime= UINT32_MAX;
	Num *en=end();
	for(Num *it=begin();it<en;it++) {
		if(valid(it)) {
			if(auto tim=it->time;tim<starttime)  {
				time_t t=tim;
				LOGGERTAG("tim %s",ctime(&t));
				t=starttime;
				LOGGERTAG(" < starttime %s",ctime(&t));
				starttime=tim;
				}
			}
		}
	time_t t=starttime;
	LOGGERTAG("starttime %s",ctime(&t));
	}	
	*/
static constexpr unsigned char magic[]={0xFE,0xD2,0xDE,0xAD};
unsigned char *raw() {
	return reinterpret_cast<unsigned char *>(nums.data());
	}
const unsigned char *raw() const {
	return reinterpret_cast<const unsigned char *>(nums.data());
	}
 void addmagic() {
	 memcpy(raw(),magic,sizeof(magic));
	 }
bool trymagic() const {
	const unsigned char *ra=raw();
	return !memcmp(magic,ra,sizeof(magic));
	}

static bool mknumdata(const string_view base,identtype ident)   {
	 if(struct stat st;stat(base.data(),&st)||!S_ISDIR(st.st_mode)) {
//	if(!fs::is_directory(basepath))  
		fs::path basepath(base.cbegin(),base.cend());
		std::error_code err;
		if(!fs::create_directories( basepath,err)) {
			LOGGERTAG("create_directories(%s) failed: %s\n",basepath.c_str(),err.message().data());
			return false;
			}
		}
	if(ident>=0LL) {
		if(!writeident(base,ident)) {
			LOGARTAG("writeident failed");
			return false;
			}
		}
	return true;
	}
static Numdata* getnumdata(const string_view base,identtype ident,size_t len)   {
	if(mknumdata(base,ident))
		return new Numdata(base,ident,len);
	return nullptr;
	}
Numdata* createnew(const string_view base,identtype ident,size_t len)  const {
	fs::path basepath { base.data()};
	if(!fs::is_directory(basepath))  {
		std::error_code err;
		if(!fs::create_directories( basepath,err)) {
			LOGGERTAG("create_directories(%s) failed: %s\n",basepath.c_str(),err.message().data());
			return nullptr;
			}
		}
		/*
	if(!writelabels(base,ident)) {
		LOGARTAG("Writelabels failed");
		return nullptr;
		}
		*/
	if(!writeident(base,ident)) {
		LOGARTAG("writeident failed");
		}
	Numdata *numdata=new Numdata(base,ident,len);
	numdata->addmagic();
	return numdata;
	}
static std::unique_ptr<char[]> numfilename(const std::string_view path ,identtype ident) {
	return identfilename(path,"/nums%lld",ident);
	}
static std::unique_ptr<char[]> identfilename(const std::string_view path ,const std::string_view format,identtype ident) {

	int pathlen=path.size();
	if(path.data()[pathlen-1]=='/')
		pathlen--;
	const int totlen= pathlen+20+format.size();
//	std::unique_ptr<char[]> name= make_unique_for_overwrite<char[]>(totlen);
	std::unique_ptr<char[]> name( new char[totlen]);

	memcpy(name.get(),path.data(),pathlen);
	snprintf(name.get()+pathlen,totlen-pathlen,format.data(),ident);
	return name;
	}
void setfirstpos(int first)  { 
	auto ra=raw();
	*reinterpret_cast<int *>(ra+firstpos)=first;
	}

int getlastpos() const {
	const char *raw=reinterpret_cast<const char *>(nums.data());
	return *reinterpret_cast<const int *>(raw+lastpos);
	}
int *lastposptr()  {
	return reinterpret_cast<int *>(raw()+lastpos);
	}
void inclastpos() {
	++*lastposptr();
	}
void setlastpos(int last)  { 
	int *was=lastposptr();
	if(getchangedpos()==*was)
		getchangedpos()=last;
		
	*was=last;
	LOGGERTAG("setlastpos=%d\n",last);
	}
void setlastpolledpos(int last)  { 
	auto ra=raw();
	*reinterpret_cast<int *>(ra+lastpolledpos)=last;
	}
int getlastpolledpos() const {
	const char *raw=reinterpret_cast<const char *>(nums.data());
	return *reinterpret_cast<const int *>(raw+lastpolledpos);
	}
void setnewident(identtype newident)  { 
	ident=newident;
	auto ra=raw();
	*reinterpret_cast<identtype *>(ra+newidentpos)=newident;
	}
identtype  getnewident() const {
	auto ra=raw();
	return *reinterpret_cast<const identtype *>(ra+newidentpos);
	}
bool &receivedbackup()  {
	char *raw=reinterpret_cast< char *>(nums.data());
	return *reinterpret_cast< bool *>(raw+receivedbackuppos);
	}

void updated(int pos) {
	setlastpolledpos(pos);
	
	if(pos>getlastpos()||ident!=0LL) 
		setlastpos(pos);
	if(pos>getchangedpos()) {
		getchangedpos()=pos;
		}
	if(pos>=getonechange())
		getonechange()=0;
	}
void updated(int start,int end) {
	updated(end);
	updatepos(start,end);
	}
int getfirstpos() const { /*File functions: from which position in file starting at 256, is data present  each element is 16 bytes long*/
	const char *raw=reinterpret_cast<const char *>(nums.data());
	return *reinterpret_cast<const int *>(raw+firstpos);
	}
int &getchangedpos()  {
	char *raw=reinterpret_cast<char *>(nums.data());
	return *reinterpret_cast<int *>(raw+changedpos);
	}
const int getchangedpos() const  {
	const char *raw=reinterpret_cast<const char *>(nums.data());
	return *reinterpret_cast<const int *>(raw+changedpos);
	}
int &getonechange()  {
	char *raw=reinterpret_cast<char *>(nums.data());
	return *reinterpret_cast<int *>(raw+onechange);
	}
const int getonechange() const  {
	const char *raw=reinterpret_cast<const char *>(nums.data());
	return *reinterpret_cast<const int *>(raw+onechange);
	}


// all before getlibresend()   is send to Libreview 
int32_t &getlibresend()  { 
	char *raw=reinterpret_cast<char *>(nums.data());
	return *reinterpret_cast<int32_t *>(raw+libresend);
	}
const int32_t getlibresend() const  {
	const char *raw=reinterpret_cast<const char *>(nums.data());
	return *reinterpret_cast<const int32_t *>(raw+libresend);
	}

//const int32_t nextlibresend(uint32_t after=time(nullptr)-librekeepsecs) const {
const int32_t nextlibresend(uint32_t after) const {
	const int32_t next=getlibresend();
	const auto lastpo=getlastpos();
	if(next>=lastpo) {
		LOGGERTAG("getlibresend(%u)=%d==getlastpos()=%u\n",after,next,lastpo);
		return next;
		}
	const auto firstpos=getfirstpos();
	const Num* start=startdata();
	if(next>firstpos) {
		const Num* was;
		uint32_t wastime=0;
		for(int i=next;i>=firstpos;--i) {
			was=start+i;
			wastime=was->gettime();
			if(wastime) 
				break;
			else {
				LOGGERTAG("zero time %d\n",i);
				}
			}
		if(wastime>after) {
			LOGGERTAG("getlibresend(%u<%u(wastime))=%d\n",after,wastime,next);
			return next;
			}
		else {
			LOGGERTAG("getlibresend(%u>=%u(wastime)) \n",after,wastime);
			}
		}
	const Num*prev=firstAfter(after);
	int32_t res=prev-start;
	LOGGERTAG("getlibresend take=%d time=%u\n",res,prev->gettime());
	return res;
	}
int32_t &getlibrechangednr()  {
	char *raw=reinterpret_cast<char *>(nums.data());
	return *reinterpret_cast<int32_t *>(raw+librechangednr);
	}
const int32_t getlibrechangednr() const  {
	const char *raw=reinterpret_cast<const char *>(nums.data());
	return *reinterpret_cast<const int32_t *>(raw+librechangednr);
	}
int32_t &getlibreSolid()  {
	char *raw=reinterpret_cast<char *>(nums.data());
	return *reinterpret_cast<int32_t *>(raw+libreSolid);
	}
const int32_t getlibreSolid() const  {
	const char *raw=reinterpret_cast<const char *>(nums.data());
	return *reinterpret_cast<const int32_t *>(raw+libreSolid);
	}

int32_t &getnightStart()  {
	char *raw=reinterpret_cast<char *>(nums.data());
	return *reinterpret_cast<int32_t *>(raw+nightStart);
	}
const int32_t getnightStart() const  {
	const char *raw=reinterpret_cast<const char *>(nums.data());
	return *reinterpret_cast<const int32_t *>(raw+nightStart);
	}
int32_t &getnightSend()  {
	char *raw=reinterpret_cast<char *>(nums.data());
	return *reinterpret_cast<int32_t *>(raw+nightSend);
	}
const int32_t getnightSend() const  {
	const char *raw=reinterpret_cast<const char *>(nums.data());
	return *reinterpret_cast<const int32_t *>(raw+nightSend);
	}

int32_t &getnightSwitch()  {
	char *raw=reinterpret_cast<char *>(nums.data());
	return *reinterpret_cast<int32_t *>(raw+nightSwitch);
	}

int32_t &getnightIDstart()  {
	char *raw=reinterpret_cast<char *>(nums.data());
	return *reinterpret_cast<int32_t *>(raw+nightIDstart);
	}

void nightBack(int pos) {
	auto was=getnightStart();
	if(pos<was)
		getnightStart()=pos;
	}
void setNightSend(int pos) {
	getnightStart()=pos;
	getnightSend()=pos;
	}
	
void setonechange(const Num *num) {
	const int pos=num-startdata();
	setonechange(pos);
}
void setonechange(int pos) {
	if(getonechange()>0) {
		setlastchange(getonechange());
		getonechange()=0;
		setlastchange(pos);
		}
	else {
		if(pos==0)  
			setlastchange(0); //I already set that getonechange()=0 means nothing set. So what can I do? It is only in the very beginning
		else
			getonechange()=pos;
		}
	}


void inclastpolledpos() {
	++*reinterpret_cast<int *>(raw()+lastpolledpos);
	}
private:
template <class Cl,typename T>
static T *begin(Cl *cl) {
	return cl->nums.data()+16+cl->getfirstpos();
	}
template <class Cl,typename T>
static T *end(Cl *cl) {
	return cl->nums.data()+16+cl->getlastpos();
	}
public:
const Num *startdata() const {
	return nums.data()+16;
	}
 Num *startdata()  {
	return nums.data()+16;
	}
Num *begin() { return begin<Numdata,Num>(this); }
const Num *begin() const { return begin<const Numdata,const Num>(this); }

Num *end() { return end<Numdata,Num>(this); }
const Num *end() const { return end<const Numdata,const Num>(this); }
	/*
Num *begin() {
	return nums.data()+16+getfirstpos();
	}
Num *end() {
	return nums.data()+16+getlastpos();
	}
	*/

showitem getitem(const int pos) const { /* the pos position, starting with 0 end ending with size()-1 */
	const Num *item =startdata()+pos;
	return {item->time,item->value,settings->getlabel(item->type).data()};
	}
Num &at(const int pos) {
	Num *item =startdata()+pos;
//	LOGGERTAG("at(%d)=%p (offset=%d)\n",pos,item,reinterpret_cast<intptr_t>(item)-reinterpret_cast<intptr_t>(nums.data()));
	return *item;
	}
Num & operator[](const int pos) {
	return at(pos);
	}
const Num &at(const int pos) const {
	const Num *item =startdata()+pos;
	return *item;
	}
const Num & operator[](const int pos) const {
	return at(pos);
	}
/*
float *getweights() {
	char *raw=reinterpret_cast<char *>(nums.data());
	return reinterpret_cast<float *>(raw+weightstart);
	}
const float *getweights() const {
	const char *raw=reinterpret_cast<const char *>(nums.data());
	return reinterpret_cast<const float *>(raw+weightstart);
	}
	*/
const Num *data() const {
	return begin();
	}
Num *data() {
	return begin();
	}
int size() const {
	return getlastpos()-getfirstpos();
	}
virtual ~Numdata() {
	}
Num* firstAfter(const uint32_t endtime)  {
	 Num zoek {.time=endtime};
	 Num *beg=  begin();
	 Num *en= end();
	auto comp=[](const Num &el,const Num &se ){return el.time<se.time;};
  	return std::upper_bound(beg,en, zoek,comp);
	}
const Num* firstAfter(const uint32_t endtime)  const {
	return const_cast<Numdata*>(this)->firstAfter(endtime);
	}
Num* firstnotless(const uint32_t tim) {
	Num zoek;
	zoek.time=tim;
	Num *beg=  begin();
	Num *en= end();
	auto comp=[](const Num &el,const Num &se ){return el.time<se.time;};
  	return std::lower_bound(beg,en, zoek,comp);
	}
const Num* firstnotless(const uint32_t tim) const {
    Numdata *mthis=const_cast<Numdata*>(this);
    Num *num=mthis->firstnotless(tim);
	return const_cast<const Num *>(num);
	}
int firstnotlessIndex(const uint32_t tim) const {
	return firstnotless(tim)-startdata(); 
	}
/*
struct Num {
  uint32_t time;
  uint32_t reserved;
  float32_t value;
  uint32_t type;
  };
  */

//Libreids libreids;
void receivelastpos(int lastpos)  {
	const auto oldlastpos=getlastpos();
	if(lastpos<oldlastpos) {
		setchangetimes(lastpos,oldlastpos);
		}
	setlastpos(lastpos);
	}

void setonlychangetimes(int ind,int nr,const Num*data) { 
	const int end=ind+nr;
	const Num *start= startdata();
	uint32_t now=time(nullptr);
	for(int i=ind;i<end;i++) {
		if(memcmp(start +i, data+i-ind,sizeof(*start))) { 
			changetimes[i]=now;
			}
		}
	}
void setchangetimes(int pos,int end,const uint32_t tim=time(nullptr)) {
	for(int i=pos;i<end;i++)
		changetimes[i]=tim;
	}
void numsaveonly( const uint32_t time, const float32_t value, const uint32_t type,const uint32_t mealptrin) {
	Num *num=firstAfter(time);
	const int ind=index(num);
	const Num *endnum=end();
	if(num<endnum)  {
		const int movelen=(endnum-num);
		libremovelarger(ind,ind+1,movelen);
		memmove(num+1,num,movelen*sizeof(Num));
		setchangetimes(ind,ind+movelen+1);
		}
	else {
		setchangetimes(ind,ind+1);
		}
	uint32_t mealptr=meals->datameal()->endmeal(mealptrin);
	*num={.time=time,.mealptr=mealptr,.value=value,.type=type};
	inclastpos();
	inclastpolledpos();
	setlastchange(num);
	const auto lastnum=getlastpos();
	nightBack(ind);
	updateposnowake(ind,lastnum);
	addlibrechange(ind);
	LOGGERTAG("pos=%d newlastnum=%d numsave %f %s mealptrin=%d mealptr=%d\n",ind,lastnum,value,settings->getlabel(type).data(),mealptrin,mealptr);
	 }

void numsave( const uint32_t time, const float32_t value, const uint32_t type,const uint32_t mealptrin) {
	numsaveonly(time,  value,  type, mealptrin);
	if(backup)
		backup->wakebackup(Backup::wakenums);
	setnumchanged();
	//wakeuploader();
	//wakeaftermin(1);
	 }


void numsavepos(int pos, uint32_t time, float32_t value, uint32_t type,uint32_t mealptr) {
	if(pos>0&&time<at(pos-1).time)  {
		LOGGERTAG("numsavepos earlier than previous %u<%u\n",time,at(pos-1).time);
//		at(pos).time=at(pos-1).time;
		}
	else {
		LOGGERTAG("numsavepos %d %d %f %s\n",pos,mealptr,value,settings->getlabel(type).data());
		if(mealptr&&!meals->datameal()->goodmeal(mealptr)) {
			mealptr=0;
			}
		Num &num=at(pos);
		addlibrenumsdeleted(&num,pos);
		num={.time=time,.mealptr=mealptr,.value=value,.type=type};
		if(pos>=getlibreSolid()) {
			addlibrechange(pos);
			}
		else {
			LOGGERTAG("%d older than getlibreSolid()=%d\n",pos, getlibreSolid());
			}
		receivedbackup()=false;
		nightBack(pos);
		updatepos(pos,getlastpos()); //ADDED
		setchangetimes(pos,pos+1);
		}
	setnumchanged();
	 }
//static constexpr uint32_t removedtype=0xFFFFFFFF;
static constexpr uint32_t removedbit=1<<31;
static constexpr uint32_t otherbits=~removedbit;

void changeDevice() {
	getlibreSolid()=getlibresend();
	}
void numremove(int pos) {
	Num &num=at(pos);
	addlibrenumsdeleted(&num,pos);
	LOGGERTAG("numremove(%d)\n",pos);
//	num.type=removedtype;
	num.type|=removedbit;
	if(pos>0)
		num.time=at(pos-1).time; //Deleted items to be ordered in time, for binary search
	else
		num.time=at(pos+1).time;
	nightBack(pos);
	updatepos(pos,pos+1);
	setchangetimes(pos,pos+1);
	setnumchanged();
	}
int index(const Num *num)const {
	return num-startdata();
	}
	/*
int numremove(Num *num) { //Ook effect in NumberView
	int pos=index(num);
	numremove(pos);
	return pos;
	}
	*/
int getdeclastpos() {
	Num *last=end()-1;
//	last->type=removedtype;
	last->type|=removedbit;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsequence-point"
	last->time=(--last)->time;
#pragma GCC diagnostic pop
	for(;last>=begin()&&(last->type&removedbit);last--)
		;
	return last-startdata()+1;
	}
	
int numremove(Num *num) {
	const int ver=end()-num;
	int pos=index(num);
	LOGGERTAG("numremove(NUM %d)\n",pos);
	addlibrenumsdeleted(num,pos);
	if(ver>0) {	
		if(ver>1) {
			const int movelen= (ver-1);
			memmove(num,num+1,movelen*sizeof(Num));
			libremovesmaller(pos+1,pos,movelen);
			setchangetimes(pos,pos+1+movelen);
			}
		else {
		   setchangetimes(pos,pos+1);
		   }
		const int newlastpos= getdeclastpos();
		setlastpos(newlastpos);
		setlastchange(pos);
		if(getlibresend() > newlastpos)
			getlibresend()=newlastpos;
		}
	else  {
		   setchangetimes(pos,pos+1);
		   }
	nightBack(pos);
	updatepos(pos,getlastpos());
	setnumchanged();
	return pos;
	}


void setlastchange(int pos) {
	if(pos<getchangedpos())
		getchangedpos()=pos;
	}
void setlastchange(const Num *el) {
	setlastchange(el-startdata());
	}

void setmealptr(Num *hit,uint32_t mealptr) {
	LOGGERTAG("setmealptr(%d,%d)\n",index(hit),mealptr);
	hit->mealptr= meals->datameal()->endmeal(mealptr);
	}

private:
void addlibrechange(int ind) {
	if(ind>=getlibresend()) {
		LOGGERTAG("addlibrechange(%d) larger\n",ind);
		return;
		}
	const int nrchanges=getlibrechangednr();
	if(nrchanges) {
		auto start=	librechanged.data();
		auto after=	start+nrchanges;
		auto hit=std::lower_bound(start,after, ind);
		if(hit<after) {
			if(*hit==ind) {
				LOGGERTAG("addlibrechange(%d) already present\n",ind);
				return;
				}
			memmove(hit+1,hit,reinterpret_cast<uint8_t*>(after)-reinterpret_cast<uint8_t*>(hit));
			LOGGERTAG("addlibrechange(%d) move\n",ind);
			}
		else {
			LOGGERTAG("addlibrechange(%d) add to last position at %d\n",ind,nrchanges);
			}
		*hit=ind;
		++getlibrechangednr();
		}
	else {
		LOGGERTAG("addlibrechange(%d) first change\n",ind);
		librechanged[getlibrechangednr()++]=ind;
		}
	}



void	librechangelastpos(const int newlastpos) {
	const int sendnr= getlibresend();
	LOGGERTAG("librechangelastpos(%d) sendnr=%d\n",newlastpos,sendnr);
	if(newlastpos>=sendnr)
		return;
	const Num *start= startdata();
	for(int i=newlastpos;i<sendnr;i++)
		addlibrenumsdeleted(start+i,i);
	}


void addmorelibrenumschanged(int ind,int nr,const Num*data) { 
	const int sendnr= getlibresend();
	LOGGERTAG("addmorelibrenumschanged(%d,%d,...)   sendnr=%d\n",ind,nr,sendnr);
	if(ind>=sendnr) {
		return;
		}
	int end=ind+nr;
	if(end> sendnr)
		end=sendnr;
	if(end<getlibreSolid())
		return;
		/*
	if(ind<getlibreSolid())
		ind=getlibreSolid(); */
	const Num *start= startdata();
	for(int i=ind;i<end;i++) {
		if(memcmp(start +i, data+i-ind,sizeof(*start))) { //TODO: CHANGE HERE instead of memcpy
			addlibrenumsdeleted(start+i,i);
			addlibrechange(i);
			}
		}
	}
void addlibrenumsdeleted(const Num *num,int ind) { //Moet VOOR  andere libre operaties.
#ifdef LIBRENUMBERS
	if(ind>=getlibresend())  {
		return;
		}
	if(ind<getlibreSolid())
		return;
	const auto nrchanges=getlibrechangednr();
	if(nrchanges>0) {
	    auto start=	librechanged.data();
	    auto after=	start+nrchanges;
		auto hit=std::lower_bound(start,after, ind);
		if(hit!=after) {
			LOGGERTAG("addlibrenumsdeleted(const Num *num,%d) in librechanged\n",ind);
			*hit=-1;
			return;	
			}
		}
	LOGGERTAG("addlibrenumsdeleted(const Num *num,%d)\n",ind);
	extern Mmap<Num> librenumsdeleted;
	librenumsdeleted[getlibrenumsdeletednr()]=*num;
	const auto librenr=libreids[ind].nr;
	librenumsdeleted[getlibrenumsdeletednr()++].librenr=librenr;
#endif
	}
void	libremovesmaller(int from,int to,int movelen) {  //to smaller than from
	int nrsend=getlibresend();
	if(to>=nrsend) {
		LOGGERTAG("libremove(%d,%d) nrsend=%d  from>to>=nrsend\n",from,to,nrsend);
		return;
		}
	auto *beg=libreids.data();
	int len=nrsend-from;
	if(len>0) {
		memmove(beg+to,beg+from,sizeof(*beg)*std::min(len,movelen));
		if(movelen>=len)
			getlibresend()=len+to;
		int nr=getlibrechangednr(); 
		int af=from-to;
		int smallerthan=from+movelen;
		for(int i=0;i<nr;i++) {
			auto &id=librechanged[i];
			if(id>to) {
				if(id>=from) {
					if(id<smallerthan)
						id-=af;
					}
				else {
					id=-1;
					}
				}
			}
		LOGGERTAG("libremove(%d,%d) nrsend=%d  moved new libresend=%d\n",from,to,nrsend,getlibresend());
		return;
		}
	getlibresend()=to;
	LOGGERTAG("libremove(%d,%d) nrsend=%d no move new libersend=%d\n",from,to,nrsend,getlibresend());
	return;
	}

void	libremovelarger(int from,int to,int movelen) { //to>from
	int nrsend=getlibresend();
	if(from>=nrsend) {
		LOGGERTAG("libremove(%d,%d) nrsend=%d  to>=from>=nrsend\n",from,to,nrsend);
		return;
		}
	auto *beg=libreids.data();
	int len=nrsend-from;
	memmove(beg+to,beg+from,sizeof(*beg)*std::min(len,movelen));
	if(movelen>=len)
		getlibresend()=len+to;
	const int nr=getlibrechangednr(); 
	const int bij=to-from;
	const int smallerthan=from+movelen;
	for(int i=0;i<nr;i++) {
		auto &id=librechanged[i];
		if(id>=from) {
			if(id<smallerthan) {
				id+=bij;
				}
			else {
				if(id==smallerthan)
					id=-1;
				}

			}
		}
	LOGGERTAG("libremove(%d,%d) nrsend=%d  to>=from<nrsend new libresend=%d\n",from,to,nrsend,getlibresend());
	return;
	}

public:
void numchange(const Num *hit, uint32_t time, float32_t value, uint32_t type,uint32_t mealptr) {
	const int oldpos=hit-startdata();
	LOGGERTAG("Start numchange oldpos=%d\n",oldpos);
	addlibrenumsdeleted(hit,oldpos);
	if(mealptr==0)
		mealptr=hit->mealptr;

	Num *num;
	int pos,lastupdate;
	if(hit->time!=time) {
		num=firstAfter(time);
		if(num<hit) {
			LOGARTAG("numchange num<hit");
			int movelen= (hit-num);
			memmove(num+1,num,movelen*sizeof(Num));
			setlastchange(num);
			pos=num-startdata();
			lastupdate=getlastpos();
			libremovelarger(pos,pos+1,movelen);
		   	setchangetimes(pos,pos+1+movelen);
			}
		else {
			if(num>(hit+1)) {
				LOGARTAG("if(num>(hit+1))");
				num--;
				Num *modhit=const_cast<Num *>(hit);
				int movelen= (num-modhit);
				memmove(modhit,modhit+1,movelen*sizeof(Num));
				setlastchange(modhit);
				pos=modhit-startdata();
				lastupdate=getlastpos();
				libremovesmaller(pos+1,pos,movelen);
		   		setchangetimes(pos,pos+1+movelen);
				}
			else  {
				LOGARTAG("if(num<=(hit+1))");
				num=const_cast<Num *>(hit);
				setonechange(hit);
				pos=num-startdata();
				lastupdate=pos+1;
			//	addlibrenumsdeleted(num,pos);
		   		setchangetimes(pos,pos+1);
				}
			}
		}
	else {
		LOGARTAG("hit->time==time"); 
		num=const_cast<Num *>(hit);
		setonechange(hit);
		pos=num-startdata();
		//addlibrenumsdeleted(num,pos);
		lastupdate=pos+1;
		setchangetimes(pos,pos+1);
		}
	LOGGERTAG("numchange %d %d\n",pos,mealptr);
	mealptr=meals->datameal()->endmeal(mealptr);
	const int newpos=num-startdata();
	*num={.time=time,.mealptr=mealptr,.value=value,.type=type};

	if(newpos>=getlibreSolid()) {
		addlibrechange(newpos);
		}
	else  {
		LOGGERTAG("%d older than getlibreSolid()=%d\n",newpos, getlibreSolid());
		}
	nightBack(pos);
	updatepos(pos,lastupdate);
	setnumchanged();
	}
std::pair<const Num*,const Num*> getInRange(const uint32_t start,const uint32_t endtime) const {
	Num zoek;
	zoek.time=start;

	const Num *beg=  begin();
	const Num *en= end();
	auto comp=[](const Num &el,const Num &se ){return el.time<se.time;};
  	const Num *low=std::lower_bound(beg,en, zoek,comp);
	if(low==en) {
		return {en,en};
		}
	zoek.time=endtime;
//  	const Num *high=std::upper_bound(low,en, zoek,comp);
  	const Num *high=std::lower_bound(low,en, zoek,comp);
	return {low,high};
	}
uint32_t getlasttime() const {
	const Num *en=end();
	if(en>begin()) {
		return (--en)->time;	
		}
	return 0;
	}
uint32_t getfirsttime() const {
	const Num *first=begin();
	const Num *endnum=end();
	Num zoek;
	zoek.time=0;
	auto comp=[](const Num &el,const Num &se ){return el.time<se.time;};
  	const Num *nonnull=std::upper_bound(first,endnum, zoek,comp);
	time_t tim=(nonnull==endnum)?UINT32_MAX:nonnull->time;
	LOGGERTAG("getfirsttime()=%ld %s",tim,ctime(&tim));
	return tim;
	}
	/*if(first<end()&&first->time)

		return first->time;
	return  UINT32_MAX; */

	/*
const identtype getident() const {
	return ident;
	} */
const bool needsync() const {
	return ident!=0LL;
	}
const int getindex() const { //index in Java niet numdatas
	LOGGERTAG("%p, ident=%lld\n",this,ident);
	if(ident==0LL)
		return 1;
	return 0;
	}
/*
struct changednums {
	int len;
	struct numspan changed[100];
	};
void updatesizeon() {
	const int ind=getindex();
	const int hnr=backup->getsendhostnr();
	for(int i=0;i<hnr;i++) 
		backup->getnums(i,ind)->updatedsize=false;	
	}
	*/
	
void updatesize() {
//	updatesizeon();
//	backup->wakebackup(Backup::wakenums);
	}
private:

#ifdef LOCKNUM
std::mutex nummutex;
#define NUMLOCKGUARD 	std::lock_guard<std::mutex> lock(nummutex);
#else
#define NUMLOCKGUARD
#endif


bool updatebusy[maxsendtohost]{};
void updateposnowake(int pos,int end) {
 	NUMLOCKGUARD
	int ind=getindex();
	const int hnr=backup->getsendhostnr();
	LOGGERTAG("updateposnowake pos=%d ind=%d hnr=%d\n",pos, ind,hnr);
	for(int hostindex=0;hostindex<hnr;hostindex++) {
		updatebusy[hostindex]=true;
		struct changednums *nu=backup->getnums(hostindex,ind); 
		int st=nu->changed[0].start;
		LOGGERTAG("st=%d nu->len=%d\n",st,nu->len);
		if(pos<st) {
			if(pos==(st-1)) {
				nu->changed[0].start=pos;	
				LOGGERTAG("under start=%d\n",pos);
				}
			else {
				if(nu->len>=maxchanged) {
					int mini=nu->changed[0].start;
					for(int i=1;i<maxchanged;i++) {
						if(nu->changed[i].start<mini)
							mini=nu->changed[i].start;
						}
					nu->changed[0].start=mini;
					nu->len=1;
					LOGGERTAG("toomuch start=%d\n",mini);
					}
				else {
					nu->changed[nu->len++]={pos,end};
					LOGGERTAG("changed %d %d\n",pos,end);
					}
				}
			}

		}
	LOGARTAG("end updateposnowake"); 
	}
void updatepos(int pos,int end) {
	if(backup) {
		updateposnowake(pos,end);
		backup->wakebackup(Backup::wakenums);

	//wakeuploader();
	//wakeaftermin(1);

		}
	}


#include "net/passhost.h"
//bool update(int sock,int &len, struct numspan *ch) 
static constexpr int sizeslen=onechange+4-lastpolledpos;
static_assert(sizeslen==36);

public:
template <typename... Ts>
void remake(Ts... args) {
	nums.~Mmap();
	new(this) Numdata(args...);
	}

bool happened(uint32_t stime,int type,float value) const {
	const Num *sta=begin();
	for(const Num *ptr=end()-1;ptr>=sta;ptr--) {
		const auto ti=ptr->gettime();
		if(ti<stime)
			return false;
		if(ptr->type==type&&ptr->value>value)
			return true;
		}
	return false;
	}



template <int nr,typename  T>
struct ardeleter { // deleter
    void operator() ( T ptr[]) {
        operator delete[] (ptr, std::align_val_t(nr));
    }
};

static bool	sendlastpos(crypt_t*pass,int sock,uint16_t dbase,uint32_t lastpo) {
	LOGGERTAG("sendlastpos %hd %u\n",dbase,lastpo);
	lastpos_t data{snumnr,dbase,lastpo}; 
	return sendcommand(pass,sock,reinterpret_cast<uint8_t*>(&data),sizeof(data));
	}

//std::unique_ptr<char[]> newnumsfile;
//static void renamefromident(std::string_view base,identtype ident) 
bool numbackupinit(const numinit *nums) {
	LOGARTAG("numbackupinit");
	const identtype  newident=nums->ident;
	if(ident!=newident) {
		extern pathconcat numbasedir;
		pathconcat base(numbasedir,"watch");
		if(!writeident(base,newident)) //TODO needed?
			return false;
		}
	const uint32_t sendfirst=nums->first;
	setfirstpos(sendfirst);
	/*(
	int endpos=getlastpos();
	int begpos=getfirstpos();	
	if(endpos>begpos) {
		const uint32_t tim=at(endpos-1).time;
		for(int pos=endpos;pos<sendfirst;pos++)  {
			at(pos).type=removedtype;
			at(pos).time=tim;
			}
		if(sendfirst<begpos)
			setfirstpos(sendfirst);
		else
			setfirstpos(begpos);
		}
	else {
		setfirstpos(sendfirst);
		}
		*/
	return true;
	}

bool sendbackupinit(crypt_t*pass,int sock,struct changednums *nuall) {

 { 	NUMLOCKGUARD
	LOGARTAG("sendbackupinit start NUMLOCKGUARD");

	struct changednums *nu=nuall+getindex();	
	struct numspan *ch=nu->changed;
	ch[0].start=nu->lastlastpos=getfirstpos();
	nu->len=1;
	LOGARTAG("end NUMLOCKGUARD");
	}

	numinit gegs{.first=static_cast<uint32_t>(getfirstpos()),.ident=ident};
	 if(!sendcommand(pass, sock ,reinterpret_cast<uint8_t*>(&gegs),sizeof(gegs))) {
		LOGARTAG("sendbackupinit failure");
		return false;
		}
	LOGARTAG("sendbackupinit success");
//	nuall->init=false;
	return true;
	}
bool backupsendinit(crypt_t*pass,int sock,struct changednums *nuall,uint32_t starttime) {
	LOGGERTAG("NUM: backupsendinit %u ",starttime);
	if(starttime&&(getfirstpos()!=getlastpos()))  {
		asklastnum ask{.dbase=(bool)ident};
		 if(!noacksendcommand(pass,sock,reinterpret_cast<uint8_t*>(&ask),sizeof(ask))) {
		 	LOGARTAG("NUM: noacksendcommand asklastnum failed");
			return false;
			}
		auto ret=receivedata(sock, pass,sizeof(int));
		int *posptr=reinterpret_cast<int*>(ret.get());
		if(!posptr) {
			LOGARTAG("NUM: receivedata==null");
			return false;
			}
		int pos=*posptr;
	       LOGGERTAG("NUM: get pos=%d ",pos);
		if(pos<=getfirstpos())
			goto STARTOVER;
		if(pos>getlastpos()) {
			pos=getlastpos();
			}
		const Num &num=at(pos-1);
		if(num.time>starttime) {
			pos=firstnotless(starttime)-startdata();
			LOGGERTAG("NUM: startime pos=%d\n",pos);
			}
 		{ 
		NUMLOCKGUARD
		struct changednums *nu=nuall+getindex();	
		struct numspan *ch=nu->changed;
		ch[0].start=nu->lastlastpos=pos;
		nu->len=1;
		}
		LOGAR("end backupsendinit");
		return true;
		}
	STARTOVER:
	LOGARTAG("NUM: zero start");
	return sendbackupinit(pass,sock,nuall);
	}

static inline constexpr const int intinnum=(sizeof(Num)/sizeof(uint32_t));
int update(crypt_t*pass,int sock,struct changednums *nuall,int ind) {
// 	NUMLOCKGUARD
	nummutex.lock();
	updatebusy[ind]=false; //Otherwise it has lock in network operation and which can lead to an ANR kill off app.
	struct changednums *nu=nuall+getindex();	
	LOGARTAG("nums: update");
	struct numspan *ch=nu->changed;
	int endpos=getlastpos();

	uint16_t dbase=(bool)ident;

	int offoutnr=0;
	int totlen=0;
	for(int i=nu->len-1;i>=0;i--) {
		int chend=ch[i].end==0?endpos:ch[i].end;
		if(ch[i].start<chend) {	
			int len=sizeof(Num)*(chend-ch[i].start);
			totlen+=(len+8);
			offoutnr++;
			}
		}
	int ret;
	if(!offoutnr) {
		nummutex.unlock();
		if(nu->lastlastpos==endpos)
			ret=2;
		else {
			if(!sendlastpos(pass,sock,dbase,endpos))  {
				return 0;
				}
			ret=1;
			}
		}
	else {
		totlen+=sizeof(numsend);
		std::unique_ptr<uint8_t[],ardeleter<alignof(numsend),uint8_t>> destructptr(new(std::align_val_t(alignof(numsend))) uint8_t[totlen],ardeleter<alignof(numsend),uint8_t>());
		numsend *uitnums= reinterpret_cast<numsend *>(destructptr.get());
		uitnums->type=snums;
		uitnums->dbase=dbase;
		uitnums->nr=offoutnr;
		uitnums->totlen=totlen;
		uitnums->last=endpos;
		LOGGERTAG("update numsend dbase=%d nr=%d totlen=%d last=%d\n",dbase,offoutnr,totlen,endpos);
		uint32_t *numsar=uitnums->nums;
		const Num *start=startdata();
		for(int i=nu->len-1;i>=0;i--) {
			const int chend=ch[i].end==0?endpos:ch[i].end;
			const int chstart=ch[i].start;
			if(chstart<chend) {	
				*numsar++=chstart;
				const int nr=chend-chstart;
				LOGGERTAG("NUM SN%d: %d (%d)\n",dbase,chstart,nr);
				*numsar++=nr;
				const int datlen=nr*sizeof(Num);

				memcpy(numsar,start+chstart,datlen);

				numsar+=nr*intinnum;
				}
			}
		nummutex.unlock();
		 if(!sendcommand(pass, sock ,destructptr.get(),totlen))
		 	return 0;
		ret=1;

		}

 	 {NUMLOCKGUARD
		if(updatebusy[ind])
			return 2;
		nu->lastlastpos=endpos;
		ch[0].start=endpos;
		nu->len=1;
		LOGAR("end update");
	}
	return ret;
	}
	
//	int minpos=INT_MAX; if(off<minpos) minpos=off;
void resendtowatch() {
	receivedbackup()=true;
	setlastpolledpos(getlastpos());
	getchangedpos()=0;
	}
bool backupnums(const struct numsend* innums) { 
	const int nr=innums->nr;
	const uint32_t *numsar=innums->nums;
	Num *start=startdata();

	LOGGERTAG("backupnums numsend dbase=%d nr=%d totlen=%d last=%d\n", innums->dbase,
		nr,
		innums->totlen,
		innums->last);
	for(int i=0;i<nr;i++) {
		uint32_t off=*numsar++;
		uint32_t nr=*numsar++;
		LOGGERTAG("NUM RN%d: off=%d %d items\n",(bool)ident,off,nr);
		const Num *data=reinterpret_cast<const Num *>(numsar);
		setonlychangetimes(off, nr,data);
		addmorelibrenumschanged(off,nr,data);

		const int datlen=nr*sizeof(Num);
		memcpy(start+off,data,datlen);
		numsar+=nr*intinnum;
		uint32_t eind=off+nr;
		if(off<getchangedpos()) 
			getchangedpos()=off;
		nightBack(off);
		updateposnowake(off,eind);
		receivedbackup()=true;
		}
	auto newlastpos=innums->last;
	if(newlastpos<getlastpos()) {
		setchangetimes(newlastpos,getlastpos());
		}
	librechangelastpos(newlastpos);
	setlastpos(newlastpos);
	setlastpolledpos(newlastpos); 
	
	if(backup)
		backup->wakebackup(Backup::wakenums);
	//wakeuploader();
	//wakeaftermin(1);
	return true;
	}

//TODO send ident and send magic
//update sizes? startpos?
};
#undef  devicenr 
#undef LOGGERTAG
#undef LOGARTAG

#endif
