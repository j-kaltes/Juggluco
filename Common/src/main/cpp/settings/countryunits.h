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
/*      Fri Jan 27 12:36:58 CET 2023                                                 */


#pragma once
#include <string_view>
#include <algorithm>
#include <string.h>

constexpr const std::string_view unitlabels[]={"undetermined","mmol/L","mg/dL","both"};

struct units_t {
const char code[3];
const int type;
};
constexpr const units_t countryunits[]= {
{"AE",2},
{"AG",2},
{"AM",1},
{"AR",2},
{"AT",2},
{"AU",1},
{"AZ",2},
{"BA",1},
{"BD",2},
{"BE",2},
{"BG",1},
{"BH",3},
{"BR",2},
{"BS",2},
{"BT",2},
{"BW",1},
{"BY",1},
{"BZ",2},
{"CA",1},
{"CH",1},
{"CL",2},
{"CN",1},
{"CO",2},
{"CR",2},
{"CY",2},
{"CZ",1},
{"DE",3},
{"DK",1},
{"DO",2},
{"DZ",2},
{"EC",2},
{"EE",1},
{"EG",2},
{"ES",2},
{"FI",1},
{"FR",2},
{"GB",1},
{"GE",2},
{"GR",2},
{"HK",1},
{"HR",1},
{"HU",1},
{"IE",1},
{"IL",2},
{"IN",2},
{"IQ",2},
{"IS",1},
{"IT",2},
{"JO",2},
{"JP",2},
{"KR",2},
{"KW",1},
{"KZ",1},
{"LB",2},
{"LK",2},
{"LT",1},
{"LU",2},
{"LV",1},
{"MT",1},
{"MV",2},
{"MX",2},
{"MY",1},
{"NA",1},
{"NL",1},
{"NO",1},
{"NP",2},
{"NZ",1},
{"OM",3},
{"PA",2},
{"PE",2},
{"PH",2},
{"PL",2},
{"PK",2},
{"PT",2},
{"QA",2},
{"RO",2},
{"RS",1},
{"RU",1},
{"SA",2},
{"SE",1},
{"SG",3},
{"SI",1},
{"SK",1},
{"SY",2},
{"TH",2},
{"TN",2},
{"TR",2},
{"TT",2},
{"TW",2},
{"UA",1},
{"US",2},
{"UY",2},
{"VE",2},
{"VI",3},
{"YE",2},
{"ZA",1},
{"ZW",3} };



inline int getunit(const char code[2]) {
	const units_t el({{code[0],code[1]},0});
//	const units_t el(code,0);
	const units_t *ends= std::end(countryunits);
	 if(const units_t *found= std::lower_bound( std::begin(countryunits),ends , el, [](const units_t &one,const units_t &two) {return strncasecmp(one.code,two.code,2)<0;} );found!=ends&&!strncasecmp(found->code,code,2))
		 	return found->type;
		
	return 0;	
	}
