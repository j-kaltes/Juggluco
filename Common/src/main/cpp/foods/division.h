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


#ifndef DIVISION_H
#define DIVISION_H
#include <regex>
#include "grouptype.h"

struct groups {
const uint32_t nr;
const uint32_t names[1];
};
using namespace std;
class DivisionT {
protected:
const uchar * const base;
const  groups  *  const group;
const uint32_t * const offs;

public:
DivisionT(const uchar *const base,const groups *const gr) :base(base),group(gr),  offs(&(group->names[group->nr+1])) {};


int groupmembnr(const int gr) const {
	return (offs[gr+1]-offs[gr])/sizeof(uint32_t);
	}
Grouptype groupmembers(const int gr) const {
	return Grouptype( reinterpret_cast<const uint32_t *>(base+offs[gr]), (offs[gr+1]-offs[gr])/sizeof(uint32_t));
	};
uint32_t nr() const {
	return group->nr;
	}
const char *grouplabel(int gr) const {
	return reinterpret_cast<const char *>(base+group->names[gr]);
	}

template < class OutputIt, class Reg> void  matches(OutputIt res,Reg &match) const {
   try {
	regex zoek(match, std::regex_constants::icase);

	for(int it=0;it<group->nr;++it) {
		if(std::regex_search(grouplabel(it),grouplabel(it+1),zoek))
			*res++=it;
		}

    	}
    catch (const std::regex_error& e) {
        	LOGGER( "exception std::regex %s\n", e.what());
        }
	} 

Grouptype grouplabels() const {
	return Grouptype(group->names,nr());
	}

};

class HierarchyT: public DivisionT {

using  cuint32p=const uint32_t * const;
cuint32p childrenar,parents,info;

public:
HierarchyT( const uchar *const base,const groups *const gr) : DivisionT( base,  gr) ,
	childrenar( &(group->names[(group->nr+1)*2])),
	parents( &(group->names[(group->nr+1)*3+1])),
	info(&(group->names[(group->nr+1)*4])) {};

uint32_t parent(const int gr) const {
	return parents[gr];
	 }
const char *groupinfo(int gr) const {
	if(info[gr]==info[gr+1]) 	
		return "";
	else
		 return reinterpret_cast<const char *>(base+info[gr]);
	}
	/*
void printhier(ostream &os,uint32_t gr,int d)const {
	os<<string(d,'-')<<grouplabel(gr)<<'\n';
	for(auto el:children(gr))
		printhier(os,el,d+2);
	} */
Grouptype root() const {
	return children(nr());
	}
Grouptype  children(const int gr)const {
	return Grouptype(reinterpret_cast<const uint32_t *>(base+childrenar[gr]), (childrenar[gr+1]- childrenar[gr])/sizeof(uint32_t));
	}
uint32_t  childnr(const int gr)const {
	return (childrenar[gr+1]- childrenar[gr])/sizeof(uint32_t);
	}
};

#endif // DIVISION_H
