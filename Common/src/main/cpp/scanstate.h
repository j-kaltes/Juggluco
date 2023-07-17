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
/*      Fri Jan 27 12:35:09 CET 2023                                                 */


#pragma once
#include <string_view>
#include "inout.h"
using std::string_view;

template <typename T>
constexpr int alignstart(int len) {
	constexpr int size=alignof(T);
	return ((len+size-1)/size)*size;
	}

enum state:int {
MESS=0,
COMP=1,
ATTE=2,
FAKE=3
};
class multimmap {
const int maxdats;
int freepos;
public:
Mmap<unsigned char> map;
int &datpos(int datnr) {
	return reinterpret_cast<int*>(map.data())[datnr];
	}
template <typename... T> multimmap(int maxdats,T... args):maxdats(maxdats),freepos(datastart()),map(args...) { }
virtual ~multimmap() {}

int datastart() const {
	return maxdats*sizeof(int);
	}
data_t *get(int datnr) {
	if(!map.data())
		return nullptr;
	int pos=datpos(datnr);
	if(pos<datastart())
		return nullptr;
	data_t *dat=reinterpret_cast<data_t*>(map.data()+pos);
	if(dat->len>0)
		return dat;	
	return nullptr;
	}
bool good() const {
	return const_cast<multimmap*>(this)->get(MESS);
	}
data_t * alloc(int len) {
	int was=freepos;
	data_t *dat=reinterpret_cast<data_t*>(map.data()+was);
	dat->len=len;
	freepos+=alignstart<int>(sizeof(data_t)+len);
	return dat;
	}
void reset() {
	freepos=datastart();
	memset(map.data(),'\0',freepos);
	}
void makenull(int datnr) {
	datpos(datnr)=0;
	}
	
void setpos(int datnr,data_t *dat) {
	int pos=reinterpret_cast<unsigned char *>(dat)-map.data();
	datpos(datnr)=pos;
	}
	};

	class SavedApart {};
string_view getpreviousstate(string_view sbasedir ) ; // delete[] should be called on result
constexpr const int defaultscanstate=4; //Also at other places
class scanstate: public multimmap {
	class TimeFile {};
	public:
	string_view filename;
	private:

	data_t *fromfile(const char *filename) ;
	scanstate(TimeFile,string_view datafile):multimmap(4,datafile.data(),4*4096),filename(datafile) {
		LOGSTRING("scanstate\n");
		}
	public:
//static	string_view getprevious(string_view basedir ) ;
static	string_view makefilename(const string_view basedir,const time_t tim) ;
	virtual ~scanstate() override {
		delete[] filename.data();;
		}
	bool makelink() {
		return makelink(filename);
		}

	static bool makelink(string_view filename);
	scanstate(string_view basedir,time_t tim):scanstate(TimeFile{},makefilename(basedir,tim)) {
		}
	scanstate(int block=1): multimmap(4,block*4096) {}
//	scanstate(string_view basedir) : multimmap(4,getprevious(basedir),4*4096) { }
	scanstate(string_view basedir) : scanstate(TimeFile{}, getpreviousstate(basedir)) { }
	scanstate(string_view prev,SavedApart);
	void removefile(); 
};
