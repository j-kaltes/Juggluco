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


#include <string.h>
#include <iostream>
#include "settings/settings.h"
using namespace std;

int getoneunit() {
constexpr const char *vars[]={
	"LC_ALL",
	"LC_ADDRESS",
	"LC_TELEPHONE",
	"LC_NAME",
	"LC_IDENTIFICATION",
	"LC_TIME",
	"LC_MEASUREMENT",
	"LC_MONETARY",
	"LC_NUMERIC",
	"LC_PAPER",
	};
	for(const char *v:vars) {
		const char *val=getenv(v);
		if(val) {
			if(int unit=getunit(val+3) ) 
				return unit;
			}
		}
return 0;
}

void Settings::setlinuxcountry() {
	if(data()->unit==0)
		setunit(getoneunit());
	}
