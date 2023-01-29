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
/*      Fri Jan 27 12:38:18 CET 2023                                                 */


#pragma once
#include <new>
template <int nr,typename T>
T aligner(T ptr) {
	static constexpr uintptr_t mis=(nr-1);
	static constexpr uintptr_t alignedon=~0-mis;
//	return reinterpret_cast<T>(reinterpret_cast<uintptr_t>(ptr+mis)&alignedon);
	return (T)(reinterpret_cast<uintptr_t>(ptr+mis)&alignedon);
	}
template <int nr,typename  T>
struct ardeleter { // deleter
    void operator() ( T ptr[]) {
	operator delete[] (ptr, std::align_val_t(nr));
    }
};
 
