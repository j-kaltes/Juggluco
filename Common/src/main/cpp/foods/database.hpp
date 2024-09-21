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
/*      Fri Jan 27 12:37:28 CET 2023                                                 */


#ifndef DATABASE_H
#define DATABASE_H
#include <regex>
#include <vector> 
#include <iterator>
#include <stdint.h>
#include <iostream>
#include  <cmath>
#include <string>
typedef uint8_t uchar;
#ifdef USE_DIVISIONS
#include "division.hpp"
#endif
#include "search.hpp"
//#include "inout.hpp"
using namespace std;
struct  header {
const uint32_t foods_nr /*Number of foods*/
,food_des_ind /*Start indeces in this file */
,food_des /*start food labels in this file */
,food_data /*start food data in this file */
,comps_nr
,comp_ind /*start indeces to component labels ""*/
,comp_labels /*start component labels ""*/
,comp_units /* start units */
,start_groups; } ;

class Nutrients {
#ifdef USE_DIVISIONS
struct subdivisions {
uint16_t nr;
uint16_t withoutsub;
uint32_t offs[1];
}; 
#endif
public:
private:
alignas(8) static constexpr 
#include "fooddata.h"
static constexpr const uint8_t *getbase()  {
	return fooddata;
	}
static inline const header &head=*(reinterpret_cast<const header *>(Nutrients::getbase()+256));
const uint32_t *const foods;
const uint32_t *const compind;
const uchar (*const compunits)[5];
const uint32_t *const components;
#ifdef USE_DIVISIONS
const subdivisions  * const gr;
const DivisionT ** const division;
#endif
public:
bool error() const {
	return false;
	}

unsigned foodnr() const {
	return head.foods_nr;
	}
const char *foodlabel(unsigned ind) const {
	return reinterpret_cast<const char *>(getbase()+foods[ind]);
	}
unsigned compnr() const {
	return head.comps_nr;	
	}
template <typename Ret,typename Mat> 
auto matches(Ret &got,const Mat &reg) const {
//	auto label=[this](const uint32_t ind) {return reinterpret_cast<const char *>(getbase()+foods[ind]);};
	auto label=[this](const uint32_t ind) {return foodlabel(ind);};
	return ::matches(0,foodnr(),back_inserter(got),reg,label); 
}
template <typename cont,typename Ret,typename Mat> 
auto matches(cont &inp,Ret &got,const Mat &reg) const {
	auto label=[this](const uint32_t ind) {return foodlabel(ind);};
	 return ::matches(begin(inp),end(inp),back_inserter(got),reg,  label);
	 }

//s/(\([^()]*\))\([^;]*\);/reinterpret_cast<\1>(\2);
double component(const unsigned ind,const unsigned comp) const {
//	const uint32_t (&comref)[head.foods_nr][head.comps_nr]= *reinterpret_cast<const uint32_t(*)[head.foods_nr][head.comps_nr]>(components); 
//	const uint32_t el=comref[ind][comp];
	const uint32_t *ar=reinterpret_cast<const uint32_t*>(components);
	const uint32_t el= ar[ind*head.comps_nr+comp];
	return (el==std::numeric_limits<uint32_t>::max())?std::numeric_limits<double>::quiet_NaN(): double(el)/1000;
	}
const uint32_t * getcomponents(const unsigned ind) const {
//	const uint32_t (&comref)[head.foods_nr][head.comps_nr]= *reinterpret_cast<const uint32_t(*)[head.foods_nr][head.comps_nr]>(components); 
//	const uint32_t el=comref[ind][comp];
	const uint32_t *ar=reinterpret_cast<const uint32_t*>(components);
	const uint32_t *els= ar+ind*head.comps_nr;
	return els;
	}
const char *compunit(unsigned comp) const {
	return reinterpret_cast<const char *>(compunits[comp]);
	}
const char *complabel(unsigned comp) const {
	return reinterpret_cast<const char *>(getbase()+compind[comp]);
	}

#ifdef USE_DIVISIONS
uint32_t divisions() const {
	return gr->nr;
	}
const HierarchyT *gethierarchy() const {
	return static_cast<const HierarchyT*>(division[gr->withoutsub]);
	}
~Nutrients() {
if(division) {
	for(int i=0;i<gr->nr;i++) {
		delete division[i];
		}
	delete[] division;
	}

	}
#endif
public:
Nutrients():foods(reinterpret_cast<const uint32_t*>(getbase()+head.food_des_ind)),
	compind(reinterpret_cast<const uint32_t*>(getbase()+head.comp_ind)),
	compunits(reinterpret_cast<const uchar(*)[5]>(getbase()+head.comp_units)),
	components(reinterpret_cast<const uint32_t *>(getbase()+head.food_data))
#ifdef USE_DIVISIONS
	,
	gr(reinterpret_cast<const subdivisions*>(getbase()+head.start_groups)),
	division(getbase()?new const DivisionT*[gr->nr]:nullptr)
	 {
	 if(error())
	 	return;
	for(uint32_t i=0;i<gr->withoutsub;i++) { 
		const groups *grou=reinterpret_cast<const groups*>(getbase()+gr->offs[i]);
		division[i]=new DivisionT(getbase(),grou);
		}
	for(uint32_t i=gr->withoutsub;i<gr->nr;i++) { 
		const groups *grou=reinterpret_cast<const groups*>(getbase()+gr->offs[i]);
		division[i]=new HierarchyT(getbase(),grou);
		}
	}
#else
	{}
#endif
};
#endif //DATABASE_H
