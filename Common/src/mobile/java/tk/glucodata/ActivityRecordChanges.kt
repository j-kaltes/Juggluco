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
/*      Thu Oct 15 12:44:39 CEST 2026                                                */

// SPDX-License-Identifier: GPL-3.0-or-later
package tk.glucodata

/** Local Health Connect record intervals; IDs belong to this installation. */
internal data class ActivityRecordRange(val start:Long,val end:Long,val origin:String)
internal interface ActivityRecordIntervals {
    fun find(kind:Int,id:String):ActivityRecordRange?
    fun put(kind:Int,id:String,range:ActivityRecordRange)
    fun remove(kind:Int,id:String)
}
internal class ActivityRecordChanges(private val intervals:ActivityRecordIntervals,
    private val invalidate:(Int,ActivityRecordRange)->Unit) {
    private val deletions=mutableSetOf<Pair<Int,String>>()
    fun upsert(kind:Int,id:String,range:ActivityRecordRange) {
        val previous=intervals.find(kind,id)
        // Corrections can move a record or change its app. Refresh the old
        // activity too, so it does not retain measurements which moved away.
        if(previous!=null&&previous!=range)invalidate(kind,previous)
        invalidate(kind,range)
        intervals.put(kind,id,range)
        deletions.remove(kind to id)
    }
    fun deleted(kind:Int,id:String):Boolean {
        val previous=intervals.find(kind,id)?:return false
        invalidate(kind,previous)
        // Keep the interval until its pending cursor is saved. An interrupted
        // batch must still be able to locate this deletion when it is replayed.
        deletions.add(kind to id)
        return true
    }
    fun checkpoint(kind:Int) {
        val pending=deletions.iterator()
        while(pending.hasNext()) {
            val (type,id)=pending.next()
            if(type==kind) {
                intervals.remove(type,id)
                pending.remove()
            }
        }
    }
}
