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
/*      Sun Mar 10 11:38:31 CET 2024                                                 */


package tk.glucodata;


import static android.health.connect.datatypes.BloodGlucoseRecord.RelationToMealType.RELATION_TO_MEAL_UNKNOWN;
import static android.health.connect.datatypes.BloodGlucoseRecord.SpecimenSource.SPECIMEN_SOURCE_INTERSTITIAL_FLUID;
import static android.health.connect.datatypes.BloodGlucoseRecord.SpecimenSource.SPECIMEN_SOURCE_UNKNOWN;
import static java.lang.Math.random;

import android.os.Build;

import androidx.health.connect.client.records.BloodGlucoseRecord;
import androidx.health.connect.client.records.MealType;
import androidx.health.connect.client.records.metadata.Metadata;
import androidx.health.connect.client.units.BloodGlucose;

import java.time.Instant;
import java.time.ZoneOffset;
import java.util.Iterator;
import java.util.ListIterator;

public class GlucoseIterator implements ListIterator<BloodGlucoseRecord> {

//public class GlucoseIterator implements Iterator<BloodGlucoseRecord> {
    final private static String LOG_ID="GlucoseIterator";
    GlucoseList base;
    int iter;

    GlucoseIterator(GlucoseList gl,int it) {
            base=gl;
            iter=it;
    }



    @Override
    public boolean hasNext() {
    	Log.i(LOG_ID,"hasnext iter="+iter);	
        return iter < base.len;
    }

private    BloodGlucoseRecord getglucose(long time,double value) {
        final ZoneOffset offset = null;
        return new BloodGlucoseRecord(Instant.ofEpochSecond(time), offset, BloodGlucose.milligramsPerDeciliter(value),
          SPECIMEN_SOURCE_INTERSTITIAL_FLUID,
         MealType.MEAL_TYPE_UNKNOWN,
         RELATION_TO_MEAL_UNKNOWN,
         base.metadata);
    }
private    BloodGlucoseRecord getglucose() {
	int pos=base.start+iter;
 	final long timevalue=Natives.streamfromSensorptr(base.sensorptr,pos);
	final long time=timevalue&0xFFFFFFFFL;
	final int mgdL=(int)((timevalue>>32)&0xFFFF);
    final int nextpos=(int)((timevalue>>48)&0xFFFF);
    iter=nextpos-base.start;
    Log.i(LOG_ID,"getglucose("+pos+") nextpos="+nextpos+" time="+time+" mgdL="+mgdL+" mmol="+mgdL/18.0f);
	return getglucose(time,mgdL);
    }

    @Override
    public BloodGlucoseRecord next() {
        return getglucose();

    }

    @Override
    public boolean hasPrevious() {
    	Log.i(LOG_ID,"hasPrevious() iter="+iter);
        return iter>0;
    }

    @Override
    public BloodGlucoseRecord previous() {
        Log.i(LOG_ID,"previous");
        return null;
    }

    @Override
    public int nextIndex() {
    	Log.i(LOG_ID,"nextIndex()="+iter);
        return iter;
    }

    @Override
    public int previousIndex() {
    	Log.i(LOG_ID,"prevIndex()="+(iter-1));
        return iter-1;
    }

    @Override
    public void remove() {

    }

    @Override
    public void set(BloodGlucoseRecord bloodGlucoseRecord) {

    }

    @Override
    public void add(BloodGlucoseRecord bloodGlucoseRecord) {

    }
}
