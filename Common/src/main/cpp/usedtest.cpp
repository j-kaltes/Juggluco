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


#include <array>
#include <iostream>
using std::cout;
using std::endl;
alignas(sizeof(int))
#include "used/usstat.h"
const std::array<const int,sizeof(usstat)/sizeof(int)> *reusestat=reinterpret_cast<const std::array<const int,sizeof(usstat)/sizeof(int)>*>(usstat) ;
alignas(sizeof(int))
#include "used/usaccess.h"
const std::array<const int,sizeof(usaccess)/sizeof(int)> *reuseaccess=reinterpret_cast<const std::array<const int,sizeof(usaccess)/sizeof(int)>*>(usaccess) ;
alignas(sizeof(int))
#include "used/usopen.h"
const std::array<const int,sizeof(usopen)/sizeof(int)> *reuseopen=reinterpret_cast<const std::array<const int,sizeof(usopen)/sizeof(int)>*>(usopen) ;

int main() {
for(auto el:*reusestat) 
	cout<<el<<endl;
/*
	const int *ptr=reinterpret_cast<const int*>(usaccess);
	for(int i=0;i<sizeof(usaccess)/sizeof(int);i++) {
		cout<<ptr[i]<<endl;
		} */
	}
