/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2, Libre 3, Dexcom G7/ONE+ and           */
/*      Sibionics GS1Sb sensors.                                                     */
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
/*      Sun Aug 03 14:41:38 CEST 2025                                                */
#pragma once
#include <math.h>
#include "CaliPara.hpp"

inline double mkweight(double age) {
    return 2.0L/
        (1.0L + expl(2.3148148148148148L* powl(10,-6) *age));
    }
inline double calibrateValue(const CaliPara &cali ,const uint32_t time,const double value) {
        if(time<=cali.time) {
            return value*cali.a+cali.b;
            }
        const double   w=mkweight(time-cali.time);
        if(w<=0.0) {
            return NAN;
            }
        return w*(value*cali.a+cali.b)+(1.0-w)*value;
        }

template <typename T> 
inline int getindex();
template<>
inline int getindex<ScanData>() { return 0; }
template<>
inline int getindex<Glucose>() { return 1; }
