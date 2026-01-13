/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2, Libre 3, Dexcom G7/ONE+,              */
/*      Sibionics GS1Sb and Accu-Chek SmartGuide sensors.                            */
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
/*      Tue Jan 06 12:52:10 CET 2026                                                 */
#pragma once
#include <utility>
#include <type_traits>

#include "SensorGlucoseData.hpp"
#include "CaliPara.hpp"
#include "calibrateValue.hpp"
//#include "nums/num.h"
class Numdata;
struct Num;
struct CalcPara {
    float64_t  a;
    float64_t  b;
    float32_t weight;
    };
template <typename DT> CalcPara calculate(const SensorGlucoseData *sens, const uint32_t newtime);
extern template  CalcPara calculate<ScanData>(const SensorGlucoseData *sens, const uint32_t newtime);
extern template  CalcPara calculate<Glucose>(const SensorGlucoseData *sens, const uint32_t newtime);
template <typename DT,typename Sens,typename Cali>
struct Calibrator {
    Cali &cali;
    Sens *sens;
    Calibrator(Sens *sens):cali(sens->getinfo()->calis[getindex<DT>()]),sens(sens) {
        }
//    Calibrator(Sens *sens,Cali &calin):cali(calin), sens(sens) { }
    void reCalcCali( int fromnr);
    CalcPara calculate(const uint32_t newtime) {
            return ::calculate<DT>(sens, newtime);
            }
    bool changeCali(const uint32_t oldtime,const uint32_t newtime,const Num *num, const Numdata *numdata);
    int  removeCali(uint32_t tim) {
        return cali.removeCali(tim);
        };

   double     calibrateNowNoCheck(const uint32_t time, const double value) {
        const uint32_t nr=cali.caliNr;
        if(nr==0) {
            return NAN;
            }
        const CaliPara &calip = cali.caliPara[nr-1];
         return calibrateValue(calip,time,value);
        }

    double     calibrateNow(const uint32_t time, const double value) {
        if(!settings->data()->DoCalibrate)
            return NAN;
        return     calibrateNowNoCheck(time,  value) ;
        }
    double     calibrateNowNoCheck(const DT &value) {
        return calibrateNowNoCheck(value.gettime(),value.getmgdL());
    }
    double     calibrateNow(const DT &value) {
        return calibrateNow(value.gettime(),value.getmgdL());
        }

double     calibrateONE(const uint32_t time, const double value);
double     calibrateONE(const DT &value) {
    return calibrateONE(value.gettime(),value.getmgdL());
    }

double     calibrateONEtest(const DT &value) {
    if(!settings->data()->DoCalibrate)
        return NAN;
     return calibrateONE(value);
     }

    std::pair<const DT*,const DT*> makecalibrated(const DT *input,DT *calibrated,int nr,bool allvalues) ;
    std::pair<const DT*,const DT*> makecalibratedback(const DT *input,DT *calibrated,int nr,bool allvalues);

    int     caliPosAfter(const uint32_t time);

    };

template <typename DT, typename Sens> Calibrator<DT, Sens,typename std::remove_reference_t<decltype(std::declval<Sens>().getinfo()->calis[0])>> make_calibrator(Sens* sens) {
        return {sens};
        }

extern template std::pair<const ScanData*,const ScanData* > Calibrator<ScanData, SensorGlucoseData const, Calibraties const>::makecalibrated(ScanData const*, ScanData*, int, bool);
extern template std::pair<const ScanData*,const ScanData* > Calibrator<ScanData, SensorGlucoseData, Calibraties>::makecalibrated(ScanData const*, ScanData*, int, bool);
extern template std::pair<const ScanData*,const ScanData*> Calibrator<ScanData, SensorGlucoseData, Calibraties>::makecalibratedback(ScanData const*, ScanData*, int, bool);

extern template std::pair<const ScanData*,const ScanData*> Calibrator<ScanData, const SensorGlucoseData, const Calibraties>::makecalibratedback(ScanData const*, ScanData*, int, bool);

extern template double    Calibrator<ScanData, SensorGlucoseData, Calibraties>::calibrateONE(const uint32_t time, double);
