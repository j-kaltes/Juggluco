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
struct GlucoseValue {
    const int id;
    const int dataQuality;
    const int value;
    int getId() const {return id;}
    int getQuality() const {return dataQuality;}
    int getValue() const {return value;}
    };

struct GlucoseNow:GlucoseValue {
static constexpr const char *const trendString[]{"NOT_DETERMINED",
"FALLING_QUICKLY",
"FALLING",
"STABLE",
"RISING",
"RISING_QUICKLY" };
const char *trendstr() const {
	return trendString[trendArrow];
	};
    const float rate() const {
    	return  rateOfChange;
	};
	const int trend() const {
	return trendArrow;
	}
    const int trendArrow;
    const float rateOfChange;
    GlucoseNow(GlucoseValue *val,const int trend,const float rate):GlucoseValue(*val),trendArrow(trend),rateOfChange(rate) {}
	};
