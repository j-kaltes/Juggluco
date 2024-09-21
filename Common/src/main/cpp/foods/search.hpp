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


#ifndef SEARCH_H
#define SEARCH_H
#include <regex>
#include <algorithm>
#include "logs.hpp"
template <class InputIt, class OutputIt, class Reg,typename LABEL> OutputIt matches(InputIt first,InputIt last,OutputIt res,const Reg &match, LABEL label) {
	try {
	std::regex zoek(match, std::regex_constants::icase);
	return std::copy_if(first,last, res, [zoek,label](int el){return std::regex_search(label(el),label(el+1),zoek);} );
	}
	    catch (const std::regex_error& e) {
			LOGGER( "exception std::regex %s\n", e.what());
			return res;	
		}
	}
/*
template <class Input, class Output, class Reg,typename LABEL>  auto matches(Input &inp,Output &res,const Reg &match,LABEL label ) {
	return  matches(begin(inp),end(inp),back_inserter(res),match,label);
	}
*/

template <typename Ret,typename Mat,typename LABEL> 
auto matches(int mini,int maxi,Ret got,const Mat &reg,LABEL label) {
	class nummer {
		int i;
		public:
		nummer(int b): i(b) {};
		size_t operator*() const {return  i;};
		nummer &operator++() {i++;return *this;};
		bool operator!=(const nummer &other) const { return i!=other.i; }
		};
	return matches(nummer(mini),nummer(maxi),got,reg ,label);
	}
#endif
