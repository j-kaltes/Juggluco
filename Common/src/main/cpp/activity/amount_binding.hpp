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
/*      Thu Oct 15 12:52:57 CEST 2026                                                */

// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "store.hpp"
#include "../nums/num.h"

namespace juggluco::activity {
// A copied mealptr is an index in another installation. Remember the exact
// local Amount we linked, independently of Numdata's Garmin synchronization flag.
struct alignas(8) AmountBinding {
    int64_t database=0;
    uint32_t base=0;
    float roundTo=0; // The increment used for this generated Amount, in its unit.
    Num number{};
};
static_assert(sizeof(AmountBinding)==32&&sizeof(Num)==16);
static_assert(offsetof(AmountBinding,roundTo)==12&&offsetof(AmountBinding,number)==16);
inline float amountRounding(Store &store,const Record &owner) {
    const auto bindings=store.view<AmountBinding>(owner.amountBinding);
    return bindings.size()==1?bindings[0].roundTo:0;
}
inline bool ownsAmount(Store &store,uint32_t base,int64_t database,const Num &number) {
    if(!Num::issportref(number.mealptr))return false;
    const auto *owner=store.get(indexOf(number.mealptr));if(!owner)return false;
    const auto bindings=store.view<AmountBinding>(owner->amountBinding);
    return bindings.size()==1&&bindings[0].base==base&&bindings[0].database==database&&
        !memcmp(&bindings[0].number,&number,sizeof(Num));
}
inline void bindAmount(Store &store,uint32_t base,int64_t database,const Num &number,float roundTo=missing) {
    if(!Num::issportref(number.mealptr))throw std::invalid_argument("Invalid activity Amount reference");
    const auto index=indexOf(number.mealptr);
    auto *owner=store.get(index);if(!owner)throw std::invalid_argument("Activity Amount owner is unavailable");
    const float previous=amountRounding(store,*owner);
    if(std::isnan(roundTo))roundTo=previous;
    if(!std::isfinite(roundTo)||roundTo<0)throw std::invalid_argument("Invalid Amount rounding increment");
    if(ownsAmount(store,base,database,number)&&previous==roundTo)return;
    const AmountBinding binding{database,base,roundTo,number};
    const auto saved=store.append<AmountBinding>({&binding,1});
    owner=store.get(index);
    owner->amountBinding=saved;
}
inline void changedAmount(Store &store,uint32_t base,int64_t database,const Num &before,const Num &after) {
    if(!ownsAmount(store,base,database,before))return;
    if(before.mealptr==after.mealptr)bindAmount(store,base,database,after);
    else {
        auto *owner=store.get(indexOf(before.mealptr));
        owner->amountBinding={};
        store.changed();
    }
}
}
