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

extern vector<NumDisplay*> numdatas;
//extern NumIter<Num> *numiters;
inline void setpageoldest(const int i,const Num * const el) {
	numdatas[i]->extrum.first=el;
	}
inline const Num *getpageoldest(const int i) {
	return numdatas[i]->extrum.first;
	}
inline void setpagenewest(const int i,const Num *const el) {
	numdatas[i]->extrum.second=el;
	}
inline const Num *getpagenewest(const int i) {
	return numdatas[i]->extrum.second;
	}

