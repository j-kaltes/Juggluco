/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2(+), Libre 3(+), Dexcom G7/ONE+,        */
/*      Sibionics GS1Sb and GS3, Accu-Chek SmartGuide, CareSens Air and              */
/*      Aidex X sensors.                                                             */
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
/*      Thu Oct 15 12:54:42 CEST 2026                                                */

// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <array>
#include <cstdint>

namespace jgchatdata {
// On-disk prefix of Juggluco's mealdata. The production adapter checks these
// layouts against Meal.hpp at compile time. Read copies with pread, never keep
// a pointer into the mutable/remappable meal store on the chat worker thread.
struct MealIngredient {
    int32_t unit, used;
    float carb;
    std::array<char, 40> name;
};
struct MealHeader {
    uint8_t unitnr, reserved[3];
    uint16_t ingredientnr, gotlastingredient;
    uint32_t gotlastmeal, mealindex;
    std::array<std::array<char, 20>, 40> units;
    std::array<MealIngredient, 410> ingredients;
};
struct MealElement { uint32_t ingr; float amount; };
static_assert(sizeof(MealIngredient) == 52 && sizeof(MealElement) == 8);
static_assert(sizeof(MealHeader) == 22136);
}
