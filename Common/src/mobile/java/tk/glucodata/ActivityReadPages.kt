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

import java.time.Instant
import kotlinx.coroutines.currentCoroutineContext
import kotlinx.coroutines.ensureActive

/** Share descending raw-record pages between activities processed newest first. */
internal class ActivityReadPages<T>(
    private val time:(T)->Instant,
    private val fetch:suspend (String?,Instant)->Page<T>
) {
    data class Page<T>(val records:List<T>,val next:String?)
    private val records=ArrayList<T>()
    private var upper:Instant?=null
    private var oldest:Instant?=null
    private var token:String?=null
    private var finished=false
    private var previousFrom:Instant?=null
    private var previousKeep:Instant?=null

    /** keepTo is the latest end among this and all remaining activity queries. */
    suspend fun read(from:Instant,to:Instant,keepTo:Instant=to):List<T> {
        currentCoroutineContext().ensureActive()
        require(from<to&&to<=keepTo)
        require(previousFrom?.let{from<=it}!=false) {"Activities must be read newest first"}
        require(previousKeep?.let{keepTo<=it}!=false) {"Remaining query ends must not increase"}
        previousFrom=from
        previousKeep=keepTo
        records.removeAll{time(it)>=keepTo}
        if(upper==null)upper=keepTo
        if(!finished&&oldest?.let{keepTo<it}==true) {
            // Seek over a gap instead of paging through unrelated daily data.
            // No remaining activity needs records at or after keepTo.
            upper=keepTo
            oldest=null
            token=null
        }
        while(!finished&&(oldest?.let{it>=from}!=false)) {
            currentCoroutineContext().ensureActive()
            val page=fetch(token,upper!!)
            // A failed page does not advance the cursor or publish coverage.
            // Read through equal timestamps: more records can share a page edge.
            page.records.minOfOrNull(time)?.let{oldest=it}
            records.addAll(page.records.filter{time(it)<keepTo})
            token=page.next?.takeIf{it.isNotEmpty()}
            finished=token==null
        }
        // Keep the unread tail of the last page for the next activity, including
        // sparse records far in the past. At most one page extends below from.
        return records.filter{time(it)>=from&&time(it)<to}
    }
}
