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
/*      Thu Apr 04 20:18:48 CEST 2024                                                 */




#pragma once
#include <functional>
#include <type_traits>
#include <vector>
#include <string.h>
#ifndef LOGGER
#include "share/logs.h"
#endif
using namespace std;

#include "contsize.h"
template <typename eltype>
class contconcat {
eltype *name;
int namelen;
public:
template<typename T,typename ...Ts> eltype  *	ncontconcat(int len, T &&one,  Ts &&... args)  {
   eltype *ptr;
	int en=contsize(one);
	if constexpr(sizeof...(Ts)==0) {
		namelen=len+en;
		ptr= new eltype[namelen];
		}
	else {
		ptr=ncontconcat(len+en,args...);
		}
   static_assert(sizeof(one[0])==sizeof(eltype),"All element need to be of the same size");
	memcpy(ptr+len,&one[0],en*sizeof(one[0]));
	return ptr;
	}
contconcat():name(nullptr),namelen(0) {}
template <typename ...Ts>
contconcat(Ts &&... args) {
	name=ncontconcat(0,args ...);
	}
contconcat(contconcat &&in) noexcept :name(in.name),namelen(in.namelen) { 
	in.name=nullptr;
	in.namelen=0;
}
contconcat(const contconcat &&in) noexcept:contconcat( std::move(const_cast<contconcat &&>(in))) { 
}
contconcat( contconcat &in):name(new eltype[in.namelen]),namelen(in.namelen) { 
	memcpy(name,in.name,in.namelen);
}
contconcat &operator=(contconcat &&in) {
	std::swap(name,in.name);
	std::swap(namelen,in.namelen);
	return *this;
	}
contconcat &operator=(const contconcat &&in) {
	return operator=(std::move(const_cast<contconcat &&>(in)));
	}
contconcat &operator=(contconcat &in) {
	LOGGER("contconcat &operator=(contconcat &in %s) {\n",in.name);
	delete[] name;
	name=new eltype[in.namelen];
	namelen=in.namelen;
	memcpy(name,in.name,namelen);
	return *this;
	}
~contconcat() {
	delete[] name;
	}
eltype *begin() {return data();}
eltype *end() {return begin()+size();}
const eltype *cbegin() const {return data();}
const eltype *cend() const {return begin()+size();}
const eltype *begin() const {return cbegin();}
const eltype *end() const {return cend();}
eltype * data() { return name;}
const eltype * data() const { return name;}
operator eltype *() {
	return name;
	}
operator const eltype *() const {
	return name;
	}
int size() const {return namelen;}
int length() const {return size();}
};

template <typename T,typename ...Ts> requires requires(T t) {typename T::value_type;}
contconcat<typename T::value_type> concat(T a,Ts &&... args) {
      return contconcat<typename T::value_type> (a,args...);
   };
template<typename T, std::size_t N,typename ...Ts>
contconcat<T> concat(T (&a)[N],Ts &&... args) {
      return contconcat<T>(a,args...);
      }      
template<typename T, std::size_t N,typename ...Ts>
contconcat<T> concat(const T (&a)[N],Ts &&... args){
      return contconcat<T>(a,args...);
      }      
