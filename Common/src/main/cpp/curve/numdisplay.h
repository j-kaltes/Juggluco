#pragma once
#include "nums/numdata.h"
class NumDisplay: public Numdata {
	public:
	using Numdata::Numdata;
	pair<const Num*,const Num*> extrum;
	static void standout(NVGcontext *vg,float x,float y) ; 
	pair<int,int> extremenums(const pair<const int,const int> extr) const; 
	void	mealdisplay(float x,float y,const Num *num) const ;
	template <class TX,class TY> void showNums(NVGcontext* vg, const TX &transx,  const TY &transy,bool *was) const ;
	const Num *findpast(const Num *high) const ;
	template <int N> const Num *findpast() const ;
	template <int N=1> const Num *findforward() const ;
	static NumDisplay* getnumdisplay(string_view base,identtype ident,size_t len) ; 
	template <class TX,class TY>  const Num *getnearby(const TX &transx,  const TY &transy,const float tapx,const float tapy) const; 
	};
