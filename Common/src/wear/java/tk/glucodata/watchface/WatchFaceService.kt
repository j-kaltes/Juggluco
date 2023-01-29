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
/*      Fri Jan 27 15:33:27 CET 2023                                                 */


package tk.glucodata.watchface

import tk.glucodata.Log
import android.view.SurfaceHolder
import androidx.annotation.UiThread
//import androidx.annotation.Keep
import androidx.wear.watchface.*
import androidx.wear.watchface.WatchFaceService
import androidx.wear.watchface.style.CurrentUserStyleRepository
import androidx.wear.watchface.style.UserStyleSchema
import tk.glucodata.watchface.utils.createComplicationSlotManager
import tk.glucodata.watchface.utils.createUserStyleSchema

/**
 * Handles much of the boilerplate needed to implement a watch face (minus rendering code; see
 * [WatchRenderer]) including the complications and settings (styles user can change on
 * the watch face).
 */
//class JugglucoWatchFaceService : WatchFaceService() {
//@Keep
class WatchFaceService : WatchFaceService() {

    // Used by Watch Face APIs to construct user setting options and repository.
    override fun createUserStyleSchema(): UserStyleSchema = createUserStyleSchema(context = applicationContext)

    // Creates all complication user settings and adds them to the existing user settings
    // repository.
    override fun createComplicationSlotsManager(
        currentUserStyleRepository: CurrentUserStyleRepository
    ): ComplicationSlotsManager = createComplicationSlotManager(
        context = applicationContext,
        currentUserStyleRepository = currentUserStyleRepository
    )
class Mytaplistener: WatchFace.TapListener {
	@UiThread
	override fun onTapEvent(
        @TapType tapType: Int,
        tapEvent: TapEvent,
        complicationSlot: ComplicationSlot?
	) {
	//Log.d(TAG,"Tap ${tapEvent.xPos} ${tapEvent.yPos}")
	if(tk.glucodata.Natives.turnoffalarm())
                tk.glucodata.Notify.stopalarm();

	}

}
    override suspend fun createWatchFace(
        surfaceHolder: SurfaceHolder,
        watchState: WatchState,
        complicationSlotsManager: ComplicationSlotsManager,
        currentUserStyleRepository: CurrentUserStyleRepository
    ): WatchFace {
        Log.d(TAG, "createWatchFace()")

        // Creates class that renders the watch face.
        val renderer = WatchRenderer(
            context = applicationContext,
            surfaceHolder = surfaceHolder,
            watchState = watchState,
            complicationSlotsManager = complicationSlotsManager,
            currentUserStyleRepository = currentUserStyleRepository,
            canvasType = CanvasType.HARDWARE
        )

        // Creates the watch face.
        return WatchFace(
            watchFaceType = WatchFaceType.DIGITAL,
            renderer = renderer
        ).setTapListener(Mytaplistener())
    }

    companion object {
        const val TAG = "WatchFaceService"
    }
}
