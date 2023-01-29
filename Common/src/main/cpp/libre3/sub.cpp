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
/*      Fri Jan 27 15:22:01 CET 2023                                                 */


#include <stdint.h>
#include <stdio.h>
struct RequestData {
        int8_t kind[2];
        int8_t arg;
        int32_t from;
        } __attribute__ ((packed));
struct ControlHistory:RequestData {
	ControlHistory(int8_t arg,int32_t from): RequestData({{1,0},arg,from}) {}
        };
struct ClinicalControl:RequestData {
	ClinicalControl(int8_t arg,int32_t from): RequestData({{1,1},arg,from}) {}
        };

extern withres(int get);
void test(int arg,int from) {
        ClinicalControl con(arg,from);
	return kind[0
	}
/*
int main() {
	test(4,5);
	}*/
