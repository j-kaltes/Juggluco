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
/*      Fri Jan 27 12:36:20 CET 2023                                                 */


#include <stdint.h>
/*
struct ingredient_t {
	int unit;
	int used;
	float carb;
	std::array<char,40> name;
	};
typedef std::array<ingredient_t,410> ingredients_t;
typedef std::array<char,20> ingredientunit_t;
typedef std::array<ingredientunit_t,40>  units_ingredient_t;
struct mealdata {
        uint32_t unitnr,ingredientnr;
        uint32_t mealnr;
        uint32_t mealindex;
	units_ingredient_t units;
	ingredients_t ingredients;
	struct meal {
		uint32_t ingr;
		union {
			float amount;
			uint32_t numindex;
			};
		};
	meal   meals[];





};
	*/
	#include "Meal.h"
#include <iostream>
using namespace std;
int main() {
cout<<"units:"<<offsetof(mealdata,units)<<endl;
cout<<"ingredients:"<<offsetof(mealdata,ingredients)<<endl;
cout<<"meals:"<<offsetof(mealdata,meals)<<endl;
cout<<"size="<<sizeof(mealdata)<<endl;
}
