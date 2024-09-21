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
/*      Fri Jan 27 12:35:09 CET 2023                                                 */

#pragma once
#include <jni.h>
#include <vector>
struct jniHistory;
struct GlucoseValue;
#include "glucose.hpp"
struct scandata;
#include "scanstate.hpp"
struct jniHistory:std::vector<GlucoseValue*> {
	const GlucoseValue &get(int i) const {return *operator[](i);}
	}; /*How to use a forward declaration in libre2.h without mentioning vector?*/
struct AlgorithmResults {
    const jniHistory *hist;
    const GlucoseNow *glu;
/*    const int trendArrow;
    const float rateOfChange;
    */
     const int alarm;
    const jboolean isActionable;
    const int esaMinutesToWait;
    const jboolean lsaDetected;
    bool inserted,removed;
    void setremoved(int rem) {
    	removed=rem;
    	}
    void setinserted(int ins) {
    	inserted=ins;
    	}

void showresults(FILE *stream,scandata *dat) const;
    const jniHistory &history() const {return *hist;}
   const GlucoseNow &currentglucose() const {
	return *glu;
	}
   const jboolean getlsaDetected() const {
   	return lsaDetected;
   	}
   int	trend() const {
	return glu->trendArrow;
	}
  float	RateOfChange() const {
    	return  glu->rateOfChange;
	}
~AlgorithmResults();
};

