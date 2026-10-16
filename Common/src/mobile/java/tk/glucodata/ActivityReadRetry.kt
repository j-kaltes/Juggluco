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

import kotlinx.coroutines.CancellationException
import kotlinx.coroutines.currentCoroutineContext
import kotlinx.coroutines.delay
import kotlinx.coroutines.ensureActive

/** Retry the request rejected by HC; successful requests have no added delay. */
internal class ActivityReadRetry(
    private val isQuota:(Throwable)->Boolean,
    private val beforeAttempt:()->Unit,
    private val waiting:(String,Long)->Unit,
    private val resumed:()->Unit,
    private val clock:()->Long={System.nanoTime()/1_000_000},
    private val pause:suspend (Long)->Unit={delay(it)}
) {
    private suspend fun waitUntil(deadline:Long) {
        while(true) {
            currentCoroutineContext().ensureActive()
            beforeAttempt()
            val remaining=deadline-clock()
            if(remaining<=0)return
            // Notice changed retrieval settings even during a long quota wait.
            pause(minOf(remaining,5_000L))
        }
    }

    suspend fun <T> call(operation:String,request:suspend ()->T):T {
        var backoff=1_000L
        var nextAttempt=clock()
        var retried=false
        while(true) {
            waitUntil(nextAttempt)
            try {
                val result=request()
                if(retried)resumed()
                return result
            } catch(e:CancellationException) {throw e}
              catch(e:Exception) {
                if(!isQuota(e))throw e
                waiting(operation,backoff)
                nextAttempt=clock()+backoff
                backoff=minOf(backoff*2,300_000L)
                retried=true
            }
        }
    }

    companion object {
        /** AndroidX wraps platform errors; older providers may retain only text. */
        fun isQuota(error:Throwable,platform:(Throwable)->Boolean?={null}):Boolean {
            var current:Throwable?=error
            var matchingMessage=false
            repeat(16) {
                val cause=current?:return matchingMessage
                if(cause is CancellationException||cause is SecurityException||cause is IllegalArgumentException)return false
                platform(cause)?.let{return it}
                val text=cause.message.orEmpty()
                matchingMessage=matchingMessage||text.contains("API call quota exceeded",ignoreCase=true)||
                    text.contains("rate limit",ignoreCase=true)
                current=cause.cause
                if(current===cause)return matchingMessage
            }
            return matchingMessage
        }
    }
}
