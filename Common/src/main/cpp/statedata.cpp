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


#include "libre2.h"
#include <iostream>
#include <string_view>
#include <memory>
#include <string.h>
#include <assert.h>
#include "inout.h"
#include "timestr.h"

#define VISIBLE __attribute__((__visibility__("default")))
//template<>  data_t *data_t::newex(int len);
//template<> gegs<T> * gegs<T>::newex(int len) {
extern time_t totime_t(std::string_view inp) ;

scandata::scandata(string_view target)   {
	settime(totime_t(target));
	constexpr int maxpath=512;
	char buf[maxpath];
	int dirlen=target.length();
	memcpy(buf,target.data(),dirlen);
	if(buf[dirlen-1]!='/')
		buf[dirlen++]='/';
	strcpy(buf+dirlen,"rawin.dat"); data=data_t::readfile(buf);
	auto len=data->length();
	if(!data||len<300||len>350) {
		LOGGER("%s mis\n",buf);
		error=true;
		}
	setdata(data->data());
	strcpy(buf+dirlen,"info.dat"); info=data_t::readfile(buf);
	if(!info||(info->length()!=6&&(data_t::deleteex(info),true))) {
		info=data_t::newex(6);
//		LOGGER("%s mis\n",buf);
//		error=true;
		}
}

//extern data_t *fromjbyteArray(JNIEnv *env,jbyteArray jar,jint=-1) ;
scandata::scandata( JNIEnv *env,jbyteArray info,jbyteArray dat,time_t tim):data(fromjbyteArray(env,dat,344)), info(fromjbyteArray(env,info)) {
	settime(tim);
	setdata(data->data());
	}
scandata::~scandata() {
		data_t::deleteex(data);data_t::deleteex(info);
		}

#ifdef MAIN
int main() {
//scanstate prob("/tmp/");
//scanstate prob(4,4*4096);
scanstate prob("/sdcard/libre2/E007-0M000DUM8GH/2020-12-09-12:28:03/",true);
data_t *mess=prob.get(MESS);
writeall("/tmp/mess.dat",mess->data(),mess->length());
data_t *comp=prob.get(COMP);
writeall("/tmp/comp.dat",comp->data(),comp->length());
data_t *atte=prob.get(ATTE);
writeall("/tmp/atte.dat",atte->data(),atte->length());

//scanstate prob("/tmp/",time(nullptr));
}
#if 0
#define READ
#ifdef EMPTYMAIN
int main() {
multimmap  map(4,4*sizeof(int));
for(int i=0;i<4;i++) {
	data_t *one=map.get(i) ;
	if(one) 
		cout<<"data!\n";
	else
		cout<<"null!\n";
	}
}
#else
int main() {
#ifdef READ
cout<<"READ\n";
multimmap  map(4,"/tmp/test.dat");
data_t *one=map.get(1) ;
cout<<one->data();
data_t *two=map.get(2) ;
cout<<two->data();
data_t *three=map.get(0) ;
cout<<three->data();
#else
cout<<"WRITE\n";
multimmap  map(4,"/tmp/test.dat",4*4096);
data_t *one=map.alloc(13664);
data_t *two=map.alloc(32);
data_t *three=map.alloc(320);
map.setpos(0,three);
map.setpos(1,one);
map.setpos(2,two);
strcpy((char *)one->data(),"Hallo this me in one\n");
strcpy((char *)two->data(),"Here in two\n");
strcpy((char *)three->data(),"This in three\n");
#endif
}
#endif
#endif
#endif
