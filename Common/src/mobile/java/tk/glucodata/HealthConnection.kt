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
/*      Sun Mar 10 11:37:11 CET 2024                                                 */


package tk.glucodata

import android.content.Intent
import android.health.connect.datatypes.Metadata
import android.net.Uri
import android.os.Build
import androidx.activity.ComponentActivity
import androidx.health.connect.client.HealthConnectClient
//import androidx.health.connect.client.permission.HealthPermission.Companion.getReadPermission
import androidx.health.connect.client.permission.HealthPermission.Companion.getWritePermission
import androidx.health.connect.client.records.BloodGlucoseRecord
import androidx.health.connect.client.records.metadata.Device
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.GlobalScope
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.launch
import java.time.Instant
import java.util.concurrent.atomic.AtomicBoolean
import kotlin.math.min

class HealthConnection(private val client: HealthConnectClient) {
	var active= AtomicBoolean(false)


  private var scope = CoroutineScope(Dispatchers.IO+SupervisorJob())
//TODO: test not already active
private  fun writeAllIns(sensorptr:Long,sensorName:String) {
    if (active.getAndSet(true)) {
        Log.i(LOG_ID, "writeAll already active");
        return
    }
    scope.launch {
        try {
            Log.i(LOG_ID, "writeAll");
            if (!hasPermission) {
                checkPermissionsAndRun(MainActivity.thisone)
                if (!hasPermission) {
                    Log.i(LOG_ID, "No permission");
                    return@launch
                }
            }
            val endstart = Natives.healthConnectfromSensorptr(sensorptr)

            val end = endstart ushr 16
            var start = endstart and 0xFFFF
           if(start==end)
               return@launch
            Log.i(LOG_ID,"endstart=$endstart start=$start end=$end len=${end-start}")
                val meta = androidx.health.connect.client.records.metadata.Metadata(
                    device=Device("Libre", sensorName)
                )
            while (start < end) {
                val take = min(end - start, 500)
                Log.i(LOG_ID,"start=$start take=$take")
                val siz = client.insertRecords(GlucoseList(meta,sensorptr, start, take)).recordIdsList.size
                if (siz == 0) {
                    Log.e(LOG_ID, "insertRecors $siz==0")
                    return@launch
                }
                start += take;
                Natives.healthConnectWritten(sensorptr, start)
            }
        } catch (th: Throwable) {
            Log.stack(LOG_ID, "writeAll", th);
        } finally {
            active.set(false)
        }
    }
}

private suspend fun checkPermissionsAndRun(act:MainActivity?) {
        Log.i(LOG_ID,"Before getGrantedPermissions()")
        val granted = client.permissionController.getGrantedPermissions()
        Log.i(LOG_ID,"checkPermissionsAndRun granted=$granted")
        if (granted.containsAll(PERMISSIONS)) {
            Log.i(LOG_ID,"granted")
            hasPermission = true
        } else {
            hasPermission = false
            if(act?.permHealth!=null) {
                val launch=act.permHealth
                launch.permissionsLauncher.launch(PERMISSIONS)
                Log.i(LOG_ID,"requested")
                }
            else
                Log.i(LOG_ID,"no act?.permHealth, not requested")
        }
    }


companion object {
    val PERMISSIONS =
        if(Build.VERSION.SDK_INT < 28) setOf("") else
            setOf(
                getWritePermission(
                    BloodGlucoseRecord::class
                )
            )
    var hasPermission = false
    private const val LOG_ID = "HealthConnection"
   @Volatile
        private var instance:HealthConnection? = null

    private fun googleplay(context: ComponentActivity) {
        val playstr =
            "market://details?id=com.google.android.apps.healthdata&url=healthconnect://onboarding"
        val intent = Intent(Intent.ACTION_VIEW)
        intent.setPackage("com.android.vending")
        intent.setData(Uri.parse(playstr))
        intent.putExtra("overlay", true)
        intent.putExtra("callerId", context.packageName)
        context.startActivity(intent)
    }
   fun init(context:MainActivity)  {
     if(instance==null) {
	       GlobalScope.launch {
		  susinit(context)
		}
       }

   }
private suspend   fun susinit(context: MainActivity): Int {

           if (Build.VERSION.SDK_INT < 28) {
               return HealthConnectClient.SDK_UNAVAILABLE

           }
           return try {
               var ret = HealthConnectClient.getSdkStatus(context)
               when (ret) {
                   HealthConnectClient.SDK_AVAILABLE -> {
                       Log.i(LOG_ID, "SDK_AVAILABLE")
                       val client = HealthConnectClient.getOrCreate(context)
                       val health = HealthConnection(client)
                       instance = health
                       Log.i(LOG_ID, "after getOrCreate")
                       health.checkPermissionsAndRun(context)
                       Log.i(LOG_ID, "after checkPermissionsAndRun")
		       MainActivity.tryHealth=0;
                   }

                   HealthConnectClient.SDK_UNAVAILABLE_PROVIDER_UPDATE_REQUIRED -> {
                       Log.i(LOG_ID, "SDK_UNAVAILABLE_PROVIDER_UPDATE_REQUIRED")
                       googleplay(context)
                       Log.i(LOG_ID, "After googleplay")
                   }

                   HealthConnectClient.SDK_UNAVAILABLE -> {
                       Log.i(LOG_ID, "SDK_UNAVAILABLE")

                   }

                   else -> Log.e(LOG_ID, "unknown return value from getSdkStatus(context)")
               }
               ret
           } catch (th: Throwable) {
               Log.stack(LOG_ID, "exception ", th)
               HealthConnectClient.SDK_UNAVAILABLE
           }
   }

fun writeAll(sensorptr:Long,sensorname:String) {
	instance?.writeAllIns(sensorptr,sensorname);
    }
    public fun stop() {
        instance?.scope?.cancel()
        instance = null
        }
    }
}
