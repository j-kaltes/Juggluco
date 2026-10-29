// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

namespace mirrornums {
inline bool source(int base,int javaIndex) {
    return base>=0&&base<=1&&javaIndex==1-base;
}
// Protocol databases are here=0 and watch=1. A failed open must not change
// those identities when the remaining databases occupy different vector slots.
template<typename Databases>
auto database(const Databases &databases,int base)->typename Databases::value_type {
    if(base<0||base>1)return nullptr;
    typename Databases::value_type found=nullptr;
    for(auto *data:databases)if(data&&source(base,data->getindex())) {
        if(found)return nullptr; // An ambiguous identity must never select a file.
        found=data;
    }
    return found;
}
}
