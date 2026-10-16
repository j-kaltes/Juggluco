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

import androidx.health.connect.client.records.ExerciseSessionRecord
import java.lang.reflect.Modifier

/** Only the public integer constants are exercise types; the SDK also exposes
 * maps whose names start with EXERCISE_TYPE_. Keep both lookups on this list. */
internal object ActivityTypes {
    private val names by lazy {
        ExerciseSessionRecord::class.java.fields.asSequence()
            .filter { it.name.startsWith("EXERCISE_TYPE_") &&
                it.type==Int::class.javaPrimitiveType && Modifier.isStatic(it.modifiers) &&
                Modifier.isFinal(it.modifiers) }
            .associate { it.getInt(null) to it.name.removePrefix("EXERCISE_TYPE_")
                .lowercase().replace('_',' ').replaceFirstChar { c -> c.uppercase() } }
            .toSortedMap().also { check(it.isNotEmpty()) { "No Health Connect exercise type constants found" } }
    }
    fun codes():IntArray=names.keys.toIntArray()
    fun name(type:Int):String=names[type]?:"Activity $type"
}
