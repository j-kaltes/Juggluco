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


#include <iostream>
#include <array>
#include <string.h>
#include <stdio.h>
typedef std::array<char,16>  longsensorname_t;
longsensorname_t  namelibre3(const std::string_view sensorid) {
	longsensorname_t  sens;
	const char start[]="E07A-XXX";
        const int len=sensorid.length();
	const int startlen=sens.size()-len;
	memcpy(&sens[0],start,startlen);
        memcpy(&sens[0]+startlen,sensorid.data(),len);
	return sens;
        }
#include "strconcat.h"
#include "inout.h"

pathconcat logbasedir("/tmp/logs");
int main(int argc,char **argv) {
	auto res=namelibre3(argv[1]);
	printf("%.16s\n",res.data());
	}

