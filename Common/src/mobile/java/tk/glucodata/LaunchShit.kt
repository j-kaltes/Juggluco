
package tk.glucodata

import androidx.activity.result.ActivityResultCaller
import androidx.health.connect.client.PermissionController

class LaunchShit(val activity: ActivityResultCaller) {

    private val requestPermissionActivityContract= PermissionController.createRequestPermissionResultContract()

    val permissionsLauncher =                activity.registerForActivityResult(requestPermissionActivityContract) { granted ->
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
