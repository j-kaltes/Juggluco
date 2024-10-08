/*
 * Copyright 2020 The Android Open Source Project
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
package tk.glucodata.watchface.utils

import android.content.Context
import android.graphics.RectF
import androidx.wear.watchface.CanvasComplicationFactory
import androidx.wear.watchface.ComplicationSlot
import androidx.wear.watchface.ComplicationSlotBoundsType
import androidx.wear.watchface.ComplicationSlotsManager
import androidx.wear.watchface.complications.ComplicationSlotBounds
import androidx.wear.watchface.complications.DefaultComplicationDataSourcePolicy
import androidx.wear.watchface.complications.SystemDataSources
import androidx.wear.watchface.complications.data.ComplicationExperimental
import androidx.wear.watchface.complications.data.ComplicationType
import androidx.wear.watchface.complications.data.SmallImageType
import androidx.wear.watchface.complications.rendering.CanvasComplicationDrawable
import androidx.wear.watchface.complications.rendering.ComplicationDrawable
import androidx.wear.watchface.style.CurrentUserStyleRepository
import tk.glucodata.R

// Information needed for complications.
// Creates bounds for the locations of both right and left complications. (This is the
// location from 0.0 - 1.0.)
// Both left and right complications use the same top and bottom bounds.
public const val ratTimebaseY=0.265f
public const val ratupcompheight=0.17f
const val toptopcomps=       ratTimebaseY+0.02f
const val topbottomcomps=      toptopcomps +ratupcompheight
//const val larger=0.02f
const val larger=0.0f

public const val LEFT_AND_RIGHT_COMPLICATIONS_TOP_BOUND = 0.745f
private const val LEFT_AND_RIGHT_COMPLICATIONS_BOTTOM_BOUND = 0.97f


private const val RIGHT_COMPLICATION_LEFT_BOUND = 0.51f
private const val RIGHT_COMPLICATION_RIGHT_BOUND = 0.81f

private const val DEFAULT_COMPLICATION_STYLE_DRAWABLE_ID = R.drawable.complication_red_style

// Unique IDs for each complication. The settings activity that supports allowing users
// to select their complication data provider requires numbers to be >= 0.
internal const val TOP_COMPLICATION_ID = 99
internal const val BOTTOM_COMPLICATION_ID = 100
internal const val EXTREMERIGHT_COMPLICATION_ID = 101
internal const val RIGHT_COMPLICATION_ID = 102
//internal const val BACKGROUND_COMPLICATION_ID = 103

/**
 * Represents the unique id associated with a complication and the complication types it supports.
 */
sealed class ComplicationConfig(val id: Int, val supportedTypes: List<ComplicationType>) {
 /*   object Background : ComplicationConfig(
        BACKGROUND_COMPLICATION_ID,
        listOf(
            ComplicationType.PHOTO_IMAGE,
                    ComplicationType.MONOCHROMATIC_IMAGE,
                            ComplicationType.NO_PERMISSION
        )
    )*/
 @OptIn(ComplicationExperimental::class)
 object Top : ComplicationConfig(
        TOP_COMPLICATION_ID,
        listOf(
            ComplicationType.EMPTY,
            ComplicationType.LONG_TEXT,
            ComplicationType.SHORT_TEXT,
            ComplicationType.PHOTO_IMAGE,
            ComplicationType.RANGED_VALUE,
            ComplicationType.SMALL_IMAGE,
            ComplicationType.MONOCHROMATIC_IMAGE
        )
    )
    @OptIn(ComplicationExperimental::class)
    object ExtremeRight : ComplicationConfig(
        EXTREMERIGHT_COMPLICATION_ID,
        listOf(
            ComplicationType.EMPTY,
            ComplicationType.RANGED_VALUE,
            ComplicationType.SHORT_TEXT,
            ComplicationType.MONOCHROMATIC_IMAGE,
            ComplicationType.SMALL_IMAGE,
	    ComplicationType.LONG_TEXT,
	    ComplicationType.PHOTO_IMAGE,
        )
    )
    @OptIn(ComplicationExperimental::class)
    object Bottom : ComplicationConfig(
        BOTTOM_COMPLICATION_ID,
        listOf(
            ComplicationType.EMPTY,
            ComplicationType.RANGED_VALUE,
            ComplicationType.SHORT_TEXT,
            ComplicationType.PHOTO_IMAGE,
            ComplicationType.SMALL_IMAGE,
            ComplicationType.MONOCHROMATIC_IMAGE,
            ComplicationType.LONG_TEXT
        )
    )

    @OptIn(ComplicationExperimental::class)
    object Right : ComplicationConfig(
        RIGHT_COMPLICATION_ID,
        listOf(
            ComplicationType.EMPTY,
            ComplicationType.RANGED_VALUE,
            ComplicationType.SHORT_TEXT,
            ComplicationType.LONG_TEXT,
            ComplicationType.MONOCHROMATIC_IMAGE,
            ComplicationType.SMALL_IMAGE,
            ComplicationType.PHOTO_IMAGE,
        )
    )

}

// Utility function that initializes default complication slots (left and right).
fun createComplicationSlotManager(
    context: Context,
    currentUserStyleRepository: CurrentUserStyleRepository,
    drawableId: Int = DEFAULT_COMPLICATION_STYLE_DRAWABLE_ID
): ComplicationSlotsManager {

    val defaultCanvasComplicationFactory =
        CanvasComplicationFactory { watchState, listener ->
            CanvasComplicationDrawable(
                ComplicationDrawable.getDrawable(context, drawableId)!!,
                watchState,
                listener
            )
        }
/*    val backgroundComplication =ComplicationSlot.createBackgroundComplicationSlotBuilder(
        id=ComplicationConfig.Background.id,
        canvasComplicationFactory = defaultCanvasComplicationFactory,
        supportedTypes = ComplicationConfig.Background.supportedTypes,
        defaultDataSourcePolicy = DefaultComplicationDataSourcePolicy(
            SystemDataSources.NO_DATA_SOURCE
        ), ).setDefaultDataSourceType(ComplicationType.EMPTY).build() */
   // ).setDefaultDataSourceType(ComplicationType.PHOTO_IMAGE).build()
    val topComplication = ComplicationSlot.createRoundRectComplicationSlotBuilder(
        id = ComplicationConfig.Top.id,
        canvasComplicationFactory = defaultCanvasComplicationFactory,
        supportedTypes = ComplicationConfig.Top.supportedTypes,
        defaultDataSourcePolicy = DefaultComplicationDataSourcePolicy( SystemDataSources.NO_DATA_SOURCE, ComplicationType.EMPTY ),
        bounds = ComplicationSlotBounds( RectF( 0.20f, toptopcomps, 0.65f, topbottomcomps))
    ).build()
    val extremeRightComplication = ComplicationSlot.createRoundRectComplicationSlotBuilder(
        id = ComplicationConfig.ExtremeRight.id,
        canvasComplicationFactory = defaultCanvasComplicationFactory,
        supportedTypes = ComplicationConfig.ExtremeRight.supportedTypes,
        defaultDataSourcePolicy = DefaultComplicationDataSourcePolicy(
            SystemDataSources.DATA_SOURCE_WATCH_BATTERY,
                    ComplicationType.SHORT_TEXT
        ),
        bounds = ComplicationSlotBounds(
            RectF( 0.755f,topbottomcomps-0.04f, .975f, LEFT_AND_RIGHT_COMPLICATIONS_TOP_BOUND-0.04f)
        )
    ).build()

    val bottomComplication = ComplicationSlot.createRoundRectComplicationSlotBuilder(
        id = ComplicationConfig.Bottom.id,
        canvasComplicationFactory = defaultCanvasComplicationFactory,
        supportedTypes = ComplicationConfig.Bottom.supportedTypes,
        defaultDataSourcePolicy = DefaultComplicationDataSourcePolicy(
                SystemDataSources.NO_DATA_SOURCE,
            	ComplicationType.EMPTY
        ),
        bounds = ComplicationSlotBounds(
            RectF( 1.0f-RIGHT_COMPLICATION_RIGHT_BOUND-8*larger , LEFT_AND_RIGHT_COMPLICATIONS_TOP_BOUND-8*larger, 1.0f-RIGHT_COMPLICATION_LEFT_BOUND+8*larger, LEFT_AND_RIGHT_COMPLICATIONS_BOTTOM_BOUND+larger
            )
        )
    ).build()

	
    val rightComplication = ComplicationSlot.createRoundRectComplicationSlotBuilder(
        id = ComplicationConfig.Right.id,
        canvasComplicationFactory = defaultCanvasComplicationFactory,
        supportedTypes = ComplicationConfig.Right.supportedTypes,
        defaultDataSourcePolicy = DefaultComplicationDataSourcePolicy( SystemDataSources.NO_DATA_SOURCE, ComplicationType.EMPTY),
        bounds = ComplicationSlotBounds(
            RectF(
                RIGHT_COMPLICATION_LEFT_BOUND,
                LEFT_AND_RIGHT_COMPLICATIONS_TOP_BOUND,
                RIGHT_COMPLICATION_RIGHT_BOUND,
                LEFT_AND_RIGHT_COMPLICATIONS_BOTTOM_BOUND
            )
        )
    ).build()
    
    return ComplicationSlotsManager(
        listOf(
//           backgroundComplication,
            topComplication,extremeRightComplication, bottomComplication,rightComplication),
        currentUserStyleRepository
    )
}
