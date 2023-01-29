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
/*      Fri Jan 27 12:37:55 CET 2023                                                 */


#pragma once
inline unsigned char dehex(char ch) { return ch<='9'?ch-'0':(ch-'A'+10);};

	
inline unsigned char hexnum(char h,char l) {
	return dehex(h)<<4|dehex(l);
	}
inline char showhexel(int deel) {return deel<=9?'0'+deel:deel-10+'A';};

template <int part> inline char showhex(unsigned char ch){
	return showhexel(ch>>4);
	}
 template<>  inline char showhex<0>(unsigned char ch) {
	return showhexel(ch&0xF);
	}
