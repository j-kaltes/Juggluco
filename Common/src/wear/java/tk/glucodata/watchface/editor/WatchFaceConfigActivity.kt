
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

import android.os.Bundle
import tk.glucodata.Log
import android.view.View
import android.widget.CheckBox
import androidx.activity.ComponentActivity
import androidx.lifecycle.lifecycleScope
import tk.glucodata.watchface.data.watchface.ColorStyleIdAndResourceIds
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.collect
import kotlinx.coroutines.launch
import tk.glucodata.Natives
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
        Log.d(TAG, "onCreate()")

        binding = ActivityWatchFaceConfigBinding.inflate(layoutInflater)
        setContentView(binding.root)

        // Disable widgets until data loads and values are set.

        // Set max and min.


        lifecycleScope.launch(Dispatchers.Main.immediate) {
            stateHolder.uiState
                .collect { uiState: WatchFaceConfigStateHolder.EditWatchFaceUiState ->
                    when (uiState) {
                        is WatchFaceConfigStateHolder.EditWatchFaceUiState.Loading -> {
                            Log.d(TAG, "StateFlow Loading: ${uiState.message}")
                        }
                        is WatchFaceConfigStateHolder.EditWatchFaceUiState.Success -> {
                            Log.d(TAG, "StateFlow Success.")
                            updateWatchFacePreview(uiState.userStylesAndPreview)
                        }
                        is WatchFaceConfigStateHolder.EditWatchFaceUiState.Error -> {
                            Log.e(TAG, "Flow error: ${uiState.exception}")
                        }
                    }
                }
        }
    }

    private fun updateWatchFacePreview(
        userStylesAndPreview: WatchFaceConfigStateHolder.UserStylesAndPreview
    ) {
        Log.d(TAG, "updateWatchFacePreview: $userStylesAndPreview")

        val colorStyleId: String = userStylesAndPreview.colorStyleId
        Log.d(TAG, "\tselected color style: $colorStyleId")

        binding.preview.watchFaceBackground.setImageBitmap(userStylesAndPreview.previewImage)
	binding.heartRate.isChecked = Natives.getheartrate()


        enabledWidgets()
    }

    private fun enabledWidgets() {
    }
/*
    fun onClickColorStylePickerButton(view: View) {
        Log.d(TAG, "onClickColorStylePickerButton() $view")

        val colorStyleIdAndResourceIdsList = enumValues<ColorStyleIdAndResourceIds>()
        val newColorStyle: ColorStyleIdAndResourceIds = colorStyleIdAndResourceIdsList.random()

        stateHolder.setColorStyle(newColorStyle.id)
    } */
    fun onClickTopComplicationButton(view: View) {
        Log.d(TAG, "onClickToptComplicationButton() $view")
        stateHolder.setComplication(TOP_COMPLICATION_ID)
    }

    fun onClickBottomComplicationButton(view: View) {
        Log.d(TAG, "onClickLeftComplicationButton() $view")
        stateHolder.setComplication(BOTTOM_COMPLICATION_ID)
    }
    fun onClickRightComplicationButton(view: View) {
        Log.d(TAG, "onClickLeftComplicationButton() $view")
        stateHolder.setComplication(RIGHT_COMPLICATION_ID)
    }
    fun onClickExtremeRightComplicationButton(view: View) {
        Log.d(TAG, "onClickExtremeRightComplicationButton() $view")
        stateHolder.setComplication(EXTREMERIGHT_COMPLICATION_ID)
    }

    fun onClickHearRate(view: View) {
        val box= view as CheckBox
    	val on=box.isChecked()
	Natives.setheartrate(on)
//	val now=setheartrate(on)
	}



    companion object {
        const val TAG = "WatchFaceConfigActivity"
    }
}
