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
/*      Fri Jan 27 12:37:28 CET 2023                                                 */



#include <iostream>
#include  <cmath>
#include <string>
#include <vector>
#include "database.h"
int main(int argc,char **argv) {
Nutrients nuts;
/*
	const HierarchyT *hi=nuts.gethierarchy();
	auto ro=hi->root();
	for(auto el:ro) {
		hi->printhier(cout,el,0);
		}*/

	std::vector<uint32_t> vect;
	nuts.matches(vect,argv[1]);
//	std::vector<uint32_t> vect;
//	nuts.matches(vecttus,vect,argv[2]);
//	for(int i=0;i<nuts.compnr();i++) cout<<nuts.complabel(i)<<" "<<nuts.compunit(i)<<std::endl;

	for(auto v:vect) {
		cout<<nuts.foodlabel(v)<<endl;
		const int32_t *comps =reinterpret_cast<const int32_t *>(nuts.getcomponents(v)) ;
		for(int i=0;i<nuts.compnr();i++) {
			int32_t co=comps[i];
			if(co==-1)
				continue;
			cout<<nuts.complabel(i)<<" ";
			switch(co) {
				case -2:cout<<"unknown"<<endl;break;
				case -3:cout<<"trace amounts"<<endl;break;
				default: double val=co/1000.0;
				cout<<val<<" "<<nuts.compunit(i)<<std::endl;break;
				};
			}
		}

}

