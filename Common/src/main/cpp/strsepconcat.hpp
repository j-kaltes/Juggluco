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
#include <string.h>
#ifndef LOGGER
#include "share/logs.hpp"
#endif
#include "sizear.hpp"
using namespace std;
class strsepconcat {
char *name;
int namelen;
public:
template<typename T,typename ...Ts> char *	nstrconcat(int len,string_view sep,  T &&one,  Ts &&... args) {

	char *ptr;
	int en=sizear(one);
	const char *asstr=reinterpret_cast<const char *>( &one[0]);
	//LOGGER("len=%d %s size=%d\n",len,asstr,en);
	if constexpr(sizeof...(Ts)==0) {
		namelen=len+en+1;
		ptr= new char[namelen];
		ptr[namelen-1]='\0';
		}
	else {
		do { en--; } while(en>=0&&asstr[en]=='\0'); //NEEDED?
		en++;
		ptr=nstrconcat(len+en+sep.size(),sep,args...);
		memcpy(ptr+len+en,sep.data(),sep.size());
		}
	memcpy(ptr+len,asstr,en);
	return ptr;
	}
strsepconcat():name(nullptr),namelen(0) {}
template <typename ...Ts>
strsepconcat(string_view sep,  Ts &&... args) {
	name=nstrconcat(0,sep,args ...);
	}
strsepconcat(strsepconcat &&in) noexcept :name(in.name),namelen(in.namelen) { 
	in.name=nullptr;
	in.namelen=0;
	LOGGER("strsepconcat( strsepconcat &&in=%s)\n",name);
}
strsepconcat(const strsepconcat &&in) noexcept:strsepconcat( std::move(const_cast<strsepconcat &&>(in))) { 
	LOGGER("strsepconcat( const strsepconcat &&in=%s)\n",name);
}
strsepconcat( strsepconcat &in):name(new char[in.namelen]),namelen(in.namelen) { 
	memcpy(name,in.name,in.namelen);
	LOGGER("strsepconcat( strsepconcat &in=%s)\n",name);
}
strsepconcat &operator=(strsepconcat &&in) {
	LOGGER("strsepconcat &operator=(strsepconcat &&in %s) {\n",in.name);
	std::swap(name,in.name);
	std::swap(namelen,in.namelen);
	return *this;
	}
strsepconcat &operator=(const strsepconcat &&in) {
	return operator=(std::move(const_cast<strsepconcat &&>(in)));
	}
strsepconcat &operator=(strsepconcat &in) {
	LOGGER("strsepconcat &operator=(strsepconcat &in %s) {\n",in.name);
	delete[] name;
	name=new char[in.namelen];
	namelen=in.namelen;
	memcpy(name,in.name,namelen);
	return *this;
	}
~strsepconcat() {
	delete[] name;
	}
operator const string_view() const {
	return string_view(name,namelen-1);
	}
char *begin() {return data();}
char *end() {return begin()+size();}
const char *cbegin() const {return data();}
const char *cend() const {return begin()+size();}
const char *begin() const {return cbegin();}
const char *end() const {return cend();}
char * data() { return name;}
const char * data() const { return name;}
operator char *() {
	return name;
	}
operator const char *() const {
	return name;
	}
int size() const {return namelen-1;}
int length() const {return size();}
};

