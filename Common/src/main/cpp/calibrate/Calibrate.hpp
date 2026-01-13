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
    Calibrate(const SensorGlucoseData *sens,bool past, int which): cali(sens->getinfo()->calis[which].caliPara),past(past),nr(sens->getinfo()->calis[which].caliNr)
        {
        }
    public:
    int size() const {
        return nr;
        }
    double bothvalue(uint32_t time,int mgdL) { 
        if(past) {
            iter=nr-1;
           } 
        else {
            if(cali[iter].time>=time) {
                while(cali[iter].time>=time) {
                    --iter;
                    if(iter<0)
                        return NAN;
                    }
                }
            else {
                int it=iter+1;
                for(;it<nr&&cali[it].time<time;++it) {
                    }
               iter=it-1;
                }
            }
        return calibrateValue(cali[iter],time,mgdL);
        }
    double bothvalue(const ScanData &el) { 
        return bothvalue(el.gettime(),el.getmgdL());
        }
    double bothvalue(const Glucose &el) { 
        return bothvalue(el.gettime(),el.getmgdL());
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
    double backvalue(const Glucose &el) { 
        return backvalue(el.gettime(),el.getmgdL());
        }
    double forwardvalue(uint32_t time,int mgdL) { 
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
    double forwardvalue(const ScanData &el) { 
        return forwardvalue(el.gettime(),el.getmgdL());
        }
    double forwardvalue(const Glucose &el) { 
        return forwardvalue(el.gettime(),el.getmgdL());
        }

    };
template <typename DT>
class CalibrateForward: public Calibrate {
CalibrateForward(const SensorGlucoseData *sens,bool past,int which):Calibrate(sens,past,which) {
    if(past)
        iter=nr-1;
     else
        iter=0;
    };
public:
        CalibrateForward(const SensorGlucoseData *sens,bool past):CalibrateForward(sens,past,getindex<DT>()) { }
 };
template <typename DT>
class CalibrateBackward: public Calibrate {
CalibrateBackward(const SensorGlucoseData *sens,bool past,int which):Calibrate(sens,past,which) {
    iter=nr-1;
    }
public:
    CalibrateBackward(const SensorGlucoseData *sens,bool past):CalibrateBackward(sens,past,getindex<DT>()) { }
   };
