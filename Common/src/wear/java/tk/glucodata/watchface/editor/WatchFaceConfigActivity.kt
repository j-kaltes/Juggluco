
/*
 * Copyright 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package tk.glucodata.watchface.editor

import android.Manifest
import android.app.Activity
import android.app.AlertDialog
import android.content.DialogInterface
import android.content.pm.PackageManager
import android.os.Bundle
import android.view.View
import android.widget.CheckBox
import androidx.activity.ComponentActivity
import androidx.activity.result.contract.ActivityResultContracts
import androidx.core.app.ActivityCompat
import androidx.lifecycle.lifecycleScope
import tk.glucodata.watchface.data.watchface.ColorStyleIdAndResourceIds
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.collect
import kotlinx.coroutines.launch
import tk.glucodata.*
import tk.glucodata.MainActivity.SENSOR_PERMISSION_REQUEST_CODE
import tk.glucodata.databinding.ActivityWatchFaceConfigBinding
import tk.glucodata.watchface.utils.*
import tk.glucodata.watchface.utils.EXTREMERIGHT_COMPLICATION_ID
import tk.glucodata.watchface.utils.TOP_COMPLICATION_ID
//import androidx.annotation.Keep

/**
 * Allows user to edit certain parts of the watch face (color style, ticks displayed, minute arm
 * length) by using the [WatchFaceConfigStateHolder]. (All widgets are disabled until data is
 * loaded.)
 */
//@Keep
class WatchFaceConfigActivity : ComponentActivity() {
    private val stateHolder: WatchFaceConfigStateHolder by lazy {
        WatchFaceConfigStateHolder(
            lifecycleScope,
            this@WatchFaceConfigActivity,
        )
    }

    private lateinit var binding: ActivityWatchFaceConfigBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        Log.d(LOG_ID, "onCreate()")

        binding = ActivityWatchFaceConfigBinding.inflate(layoutInflater)
        setContentView(binding.root)

        // Disable widgets until data loads and values are set.

        // Set max and min.


        lifecycleScope.launch(Dispatchers.Main.immediate) {
            stateHolder.uiState
                .collect { uiState: WatchFaceConfigStateHolder.EditWatchFaceUiState ->
                    when (uiState) {
                        is WatchFaceConfigStateHolder.EditWatchFaceUiState.Loading -> {
                            Log.d(LOG_ID, "StateFlow Loading: ${uiState.message}")
                        }
                        is WatchFaceConfigStateHolder.EditWatchFaceUiState.Success -> {
                            Log.d(LOG_ID, "StateFlow Success.")
                            updateWatchFacePreview(uiState.userStylesAndPreview)
                        }
                        is WatchFaceConfigStateHolder.EditWatchFaceUiState.Error -> {
                            Log.e(LOG_ID, "Flow error: ${uiState.exception}")
                        }
                    }
                }
        }
    }

    private fun updateWatchFacePreview(
        userStylesAndPreview: WatchFaceConfigStateHolder.UserStylesAndPreview
    ) {
        Log.d(LOG_ID, "updateWatchFacePreview: $userStylesAndPreview")

        val colorStyleId: String = userStylesAndPreview.colorStyleId
        Log.d(LOG_ID, "\tselected color style: $colorStyleId")

        binding.preview.watchFaceBackground.setImageBitmap(userStylesAndPreview.previewImage)
	binding.heartRate.isChecked = Applic.getHeartRate()


        enabledWidgets()
    }

    private fun enabledWidgets() {
    }
/*
    fun onClickColorStylePickerButton(view: View) {
        Log.d(LOG_ID, "onClickColorStylePickerButton() $view")

        val colorStyleIdAndResourceIdsList = enumValues<ColorStyleIdAndResourceIds>()
        val newColorStyle: ColorStyleIdAndResourceIds = colorStyleIdAndResourceIdsList.random()

        stateHolder.setColorStyle(newColorStyle.id)
    } */
    fun onClickTopComplicationButton(view: View) {
        Log.d(LOG_ID, "onClickToptComplicationButton() $view")
        stateHolder.setComplication(TOP_COMPLICATION_ID)
    }

    fun onClickBottomComplicationButton(view: View) {
        Log.d(LOG_ID, "onClickLeftComplicationButton() $view")
        stateHolder.setComplication(BOTTOM_COMPLICATION_ID)
    }
    fun onClickRightComplicationButton(view: View) {
        Log.d(LOG_ID, "onClickLeftComplicationButton() $view")
        stateHolder.setComplication(RIGHT_COMPLICATION_ID)
    }
    fun onClickExtremeRightComplicationButton(view: View) {
        Log.d(LOG_ID, "onClickExtremeRightComplicationButton() $view")
        stateHolder.setComplication(EXTREMERIGHT_COMPLICATION_ID)
    }

private var  heartratebox:CheckBox?=null

fun askpermission(perm:String) {
 requestPermissions(arrayOf(perm), SENSOR_REQUEST_CODE)
 }
override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<String>, grantResults: IntArray) {
    super.onRequestPermissionsResult(requestCode, permissions, grantResults)
    when (requestCode) {
        SENSOR_REQUEST_CODE -> {
  	    val granted=(grantResults.isNotEmpty() && grantResults[0] == PackageManager.PERMISSION_GRANTED)
            if(granted) {
			heartratebox?.setChecked(true)
			Log.i(LOG_ID,"Sensor granted")
            	} else {
			Log.i(LOG_ID,"Sensor not granted")
            	}
	    Natives.setheartrate(granted)
	    heartratebox=null
            return
        }

        else -> {
		Log.i(LOG_ID,"On known permission "+requestCode);
        }
    }
}
/*
private val requestPermissionLauncher = registerForActivityResult(ActivityResultContracts.RequestPermission() ){
	isGranted: Boolean -> {
		Natives.setheartrate(isGranted) 
		Log.i(LOG_ID,"RequestPermission "+isGranted)
		if(isGranted) {
			heartratebox?.setChecked(true)
			}
		heartratebox=null;
		}
	}*/
				//	requestPermissionLauncher.launch(bodies) 


    fun onClickHearRate(view: View) {
	Log.i(LOG_ID,"onClickHearRate")
      if(heartratebox==null) {
		val box= view as CheckBox
		val on=box.isChecked()
		if(on) {
			val bodies= Manifest.permission.BODY_SENSORS
			if(ActivityCompat.checkSelfPermission(Applic.app, bodies) != PackageManager.PERMISSION_GRANTED) {
				heartratebox=box
			       box.setChecked(false)
				Log.i(LOG_ID,"No sensor permission")
				if(shouldShowRequestPermissionRationale(bodies)) {
					    heartratebox=null
					    val builder = AlertDialog.Builder(this)
					    builder.setTitle(R.string.sensorpermission)
					    builder.setMessage(R.string.sensorpermissionmessage).
                        setPositiveButton(R.string.ok, { _, _ -> {
                               Log.i(LOG_ID, "now ask permission")
//							    heartratebox = box askpermission(bodies)

                           }
                       }) .show().setCanceledOnTouchOutside(false)
                        }
				else {
				     askpermission(bodies)
					}

				return;

				}

			}
		Natives.setheartrate(on)
		}
	}



    companion object {
        const val LOG_ID = "WatchFaceConfigActivity"
        const val SENSOR_REQUEST_CODE =787
    }
}
