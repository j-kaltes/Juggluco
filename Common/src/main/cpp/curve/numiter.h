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


#pragma once
#include "nums/numdata.h"

#include "settings/settings.h"

template <class T> struct NumIter {
	const T *startall;
	const T *iter;
	const T *begin;
	const T *end;
//	const T *pagenewest;
//	const T *pageoldest;
	int bytes;
	int index;
	void inc() {
		iter=next();
		}
	void dec()  {
		iter=prev();
		}
	const T*prev(const T* ptr) const {
		return reinterpret_cast<const T *>(reinterpret_cast<const char *>(ptr)-bytes);
		}
	const T*prev() const {
		return prev(iter);
		}
	const T*next(const T*ptr) const {
		return  reinterpret_cast<const T *>(reinterpret_cast<const char *>(ptr)+bytes);
		}

	const T*next() const {
		return next(iter);
		}
	};
#include "SensorGlucoseData.h"
inline bool notvalid(const ScanData *scan) {
	return !scan||!scan->valid();
	}
inline bool notvalid(const Glucose *glu) {
	return !glu||!glu->valid();
	}
inline bool notvalid(const Num *num) {
	if(!num)
		return true;
	if(!num->gettime())
		return true;
	if(num->type>=settings->getlabelcount())
		return true;
	return false;
	}
/*(	
template <class T> 
bool notva(const T *el) {
	return notvalid(el);
	}
	*/
//template <class T,typename F=decltype(notva<T>)> const T *findnewest(NumIter<T> *nums,int count,const F &notval=notva<T>) ;
//template  <class T,typename F=decltype(notva<T>)> const T *findoldest(NumIter<T> *nums,int count,const F &notval=notva<T>) ;

template <class T,typename F=bool (*)(const T *)>
int ifindnewest(NumIter<T> *nums,int count,const F&notval=notvalid) {
	int i=0;
	for(;i<count;i++)  {
		while(nums[i].iter>=nums[i].begin) {
			if(!notval(nums[i].iter))
				goto foundfirst;
//			nums[i].iter--;
			nums[i].dec();
			}
				
		}
	return -1;

	foundfirst:
	int newest=i++;
	for(;i<count;i++) {
		while(nums[i].iter>=nums[i].begin&&notval(nums[i].iter))
			nums[i].dec();
		//	nums[i].iter--;
		if(nums[i].iter>=nums[i].begin) {
			if(nums[i].iter->gettime()>nums[newest].iter->gettime())  {
				newest=i;
				}
			}
			
		}
	
	//nums[newest].iter--;
	nums[newest].dec();
	return newest;
	}
//template <class T,typename F=decltype(notva<T>)>

template <class T,typename F=bool (*)(const T *)>
const T *findnewest(NumIter<T> *nums,int count,const F &notval=notvalid) {
	int newest = ifindnewest(nums,count,notval);
	if(newest>=0)
		return nums[newest].next();
	return nullptr;
	}

template <class T,typename F=bool (*)(const T *)>
const std::pair<int,const T*> findnewestwith(NumIter<T> *nums,int count,const F &notval=notvalid) {
	int newest = ifindnewest(nums,count,notval);
	if(newest>=0)
		return {newest,nums[newest].next()};
	return {-1,nullptr};
	}
/*
template <class T>
const T*	nextptr(const T *&ptr) {
		return ++ptr;
		}
template <class T>
const T*	minusone(const T *&ptr) {
		return ptr-1;
		}
		*/
//template <class T,class F=decltype(notva<T>)>

template <class T,typename F=bool (*)(const T *)>
int ifindoldest(NumIter<T> *nums,int start,int count,const F& notval=notvalid) {
//LOGGER("ifindoldest %d(#%d) ",start,count);
	int i=start;
	for(;i<count;i++)  {
		if(nums[i].bytes>0) {
//			LOGGER("nums[i].bytes=%d\n",nums[i].bytes);
//			LOGGER("nums[i].iter=%p nums[i].end=%p\n",nums[i].iter, nums[i].end);
			while(nums[i].iter&&nums[i].iter<=nums[i].end) {
				if(!notval(nums[i].iter))
					goto foundfirst;
				nums[i].inc();
				}
			}
				
		}
	return -1;

	foundfirst:
//LOGGER("found %d",i);
	int oldest=i++;
	for(;i<count;i++) {
		if(nums[i].iter) {
			while(nums[i].iter<=nums[i].end&&notval(nums[i].iter))
				nums[i].inc();
			if(nums[i].iter<=nums[i].end) {
				if(nums[i].iter->gettime()<nums[oldest].iter->gettime())  {
					oldest=i;
					}
				}
			}
			
		}
	
	nums[oldest].inc();
 //   LOGGER("oldest %d\n",oldest);
	return oldest;
	}
//template  <class T,typename F=decltype(notva<T>)>
template <class T,typename F=bool (*)(const T *)> const T *findoldest(NumIter<T> *nums,int count,const F& notval=notvalid) {
	int oldest = ifindoldest(nums,0,count,notval);
	if(oldest>=0)
		return nums[oldest].prev();
	return nullptr;
	}
template <class T,typename F=bool (*)(const T *)> 
const std::pair<int,const T*> findoldestwith(NumIter<T> *nums,int count,const F& notval=notvalid) {
	int oldest = ifindoldest(nums,0,count,notval);
	if(oldest>=0)
		return {oldest,nums[oldest].prev()};
	return {-1,nullptr};
	}
//template  <class T> const T *findoldest(NumIter<T> *nums,int count) ;
//template  <class T> const T *findnewest(NumIter<T> *nums,int count) ;
//extern template<typename F> const Num *findoldest(NumIter<Num> *nums,int count,const F& notval=notva<Num>) ;
//extern template<typename F> const Num *findnewest(NumIter<Num> *nums,int count,const F& notval=notva<Num>) ;
struct NVGcontext;
extern void shownums(NVGcontext* vg, NumIter<Num> *numiters, const int nr) ;

extern void shownumsback(NVGcontext* vg, NumIter<Num> *numiters, const int nr) ;
extern void shownumsbacknewest(NVGcontext* vg, NumIter<Num> *numiters, const int nr,const int numnr) ;
inline bool notvali(const Num *num) {
	if(!num->gettime())
		return true;
	return false;
	}


