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


#ifndef SERIAL_H
#define SERIAL_H
#include <string_view>
#include <string>
#include <array>


extern std::string getserial(int fam, const unsigned char *byte) ;
extern std::array<unsigned char,8> unserial(const char * const str) ;

inline constexpr unsigned char unalf(char ch) {
// 0123456789 A CDEFGH JKLMN PQR TUVWXYZ;
	ch=toupper(ch);
	if(ch=='B')
		ch='8';
	if(ch=='I')
		ch='1';
	if(ch=='O')
		ch='0';
	if(ch=='S')
		ch='5';
	if(ch<='9')	
		return ch-'0';
	if(ch=='A')
		return 10;
	if(ch<='H')
		return ch-'A'+9;
	if(ch<='N')
		return ch-'A'+8;
	if(ch<='R')
		return ch-'A'+7;
	return ch-'A'+6;
	}
#endif
