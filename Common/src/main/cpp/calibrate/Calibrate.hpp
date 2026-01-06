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
/*      Sun Aug 03 14:41:48 CEST 2025                                                */
#pragma once
#include "calibrateValue.hpp"
#include "SensorGlucoseData.hpp"
class Calibrate {
    const CaliPara *cali;
   protected: 
    const bool past;
    const int nr;
    int iter;

    Calibrate(const SensorGlucoseData *sens,int which): cali(sens->getinfo()->calis[which].caliPara),past(settings->data()->CalibratePast),nr(sens->getinfo()->calis[which].caliNr)
        {
        }
    public:
    int size() const {
        return nr;
        }
    double backvalue(uint32_t time,int mgdL) { 
        if(!past) {
            while(cali[iter].time>=time) {
                --iter;
                if(iter<0)
                    return NAN;
                }
             }
         else {
                if(iter<0)
                    return NAN;
                }
        return calibrateValue(cali[iter],time,mgdL);
        }
    double backvalue(const ScanData &el) { 
        return backvalue(el.gettime(),el.getmgdL());
        }
    double value(uint32_t time,int mgdL) { 
        if(!past) {
            if(cali[iter].time>=time)
                    return NAN;
            int it=iter+1;
            for(;it<nr&&cali[it].time<time;++it) {
                }
           iter=it-1;
           }
         else {
                if(iter<0)
                    return NAN;
                }
        return calibrateValue(cali[iter],time,mgdL);
        }
    double value(const ScanData &el) { 
        return value(el.gettime(),el.getmgdL());
        }

    };
class CalibrateForward: public Calibrate {
public:
CalibrateForward(const SensorGlucoseData *sens,int which):Calibrate(sens,which) {
    if(past)
        iter=nr-1;
     else
        iter=0;
    };
 };
class CalibrateBackward: public Calibrate {
public:
CalibrateBackward(const SensorGlucoseData *sens,int which):Calibrate(sens,which) {
    iter=nr-1;
    }

};
