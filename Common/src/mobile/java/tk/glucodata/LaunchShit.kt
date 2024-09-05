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
/*      Sun Mar 10 11:37:56 CET 2024                                                 */



package tk.glucodata

import androidx.activity.result.ActivityResultCaller
import androidx.annotation.Keep
import androidx.health.connect.client.PermissionController

class LaunchShit(val activity: ActivityResultCaller) {

    private val requestPermissionActivityContract =
        PermissionController.createRequestPermissionResultContract()

    val permissionsLauncher =
        activity.registerForActivityResult(requestPermissionActivityContract) { granted ->
            if (granted.containsAll(HealthConnection.PERMISSIONS)) {
                HealthConnection.hasPermission = true
                Log.i(LOG_ID, "requestPermissions granted")
            } else {
                HealthConnection.hasPermission = false
                Log.i(LOG_ID, "requestPermissions not granted")
            }
        }

companion object {
   val LOG_ID="LaunchShit"
	}
        }
