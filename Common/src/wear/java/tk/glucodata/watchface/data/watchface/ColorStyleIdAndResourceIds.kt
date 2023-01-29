
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
package tk.glucodata.watchface.data.watchface

import android.content.Context
import android.graphics.Color.blue
import android.graphics.drawable.Icon
import androidx.annotation.ColorRes
import androidx.annotation.DrawableRes
import androidx.annotation.StringRes
import androidx.wear.watchface.style.UserStyleSetting
import androidx.wear.watchface.style.UserStyleSetting.ListUserStyleSetting
import tk.glucodata.R

// Defaults for all styles.
// X_COLOR_STYLE_ID - id in watch face database for each style id.
// X_COLOR_STYLE_NAME_RESOURCE_ID - String name to display in the user settings UI for the style.
// X_COLOR_STYLE_ICON_ID - Icon to display in the user settings UI for the style.
const val AMBIENT_COLOR_STYLE_ID = "ambient_style_id"
private const val AMBIENT_COLOR_STYLE_NAME_RESOURCE_ID = R.string.ambient_style_name
private const val AMBIENT_COLOR_STYLE_ICON_ID = R.drawable.white

const val RED_COLOR_STYLE_ID = "red_style_id"
private const val RED_COLOR_STYLE_NAME_RESOURCE_ID = R.string.red_style_name
private const val RED_COLOR_STYLE_ICON_ID = R.drawable.red

const val GREEN_COLOR_STYLE_ID = "green_style_id"
private const val GREEN_COLOR_STYLE_NAME_RESOURCE_ID = R.string.green_style_name
private const val GREEN_COLOR_STYLE_ICON_ID = R.drawable.green

const val BLUE_COLOR_STYLE_ID = "blue_style_id"
private const val BLUE_COLOR_STYLE_NAME_RESOURCE_ID = R.string.blue_style_name
private const val BLUE_COLOR_STYLE_ICON_ID = R.drawable.blue
const val YELLOW_COLOR_STYLE_ID = "yellow_style_id"
private const val YELLOW_COLOR_STYLE_NAME_RESOURCE_ID = R.string.yellow_style_name
private const val YELLOW_COLOR_STYLE_ICON_ID = R.drawable.yellow

const val GRAY_COLOR_STYLE_ID = "gray_style_id"
private const val GRAY_COLOR_STYLE_NAME_RESOURCE_ID = R.string.gray_style_name
private const val GRAY_COLOR_STYLE_ICON_ID = R.drawable.gray

const val WHITE_COLOR_STYLE_ID = "white_style_id"
private const val WHITE_COLOR_STYLE_NAME_RESOURCE_ID = R.string.white_style_name
private const val WHITE_COLOR_STYLE_ICON_ID = R.drawable.white

/**
 * Represents watch face color style options the user can select (includes the unique id, the
 * complication style resource id, and general watch face color style resource ids).
 *
 * The companion object offers helper functions to translate a unique string id to the correct enum
 * and convert all the resource ids to their correct resources (with the Context passed in). The
 * renderer will use these resources to render the actual colors and ComplicationDrawables of the
 * watch face.
 */
enum class ColorStyleIdAndResourceIds(
    val id: String,
    @StringRes val nameResourceId: Int,
    @DrawableRes val iconResourceId: Int,
    @DrawableRes val complicationStyleDrawableId: Int,
    @ColorRes val primaryColorId: Int,
) {
    AMBIENT(
        id = AMBIENT_COLOR_STYLE_ID,
        nameResourceId = AMBIENT_COLOR_STYLE_NAME_RESOURCE_ID,
        iconResourceId = AMBIENT_COLOR_STYLE_ICON_ID,
        complicationStyleDrawableId = R.drawable.complication_white_style,
        primaryColorId = R.color.ambient_primary_color,
    ),

    RED(
        id = RED_COLOR_STYLE_ID,
        nameResourceId = RED_COLOR_STYLE_NAME_RESOURCE_ID,
        iconResourceId = RED_COLOR_STYLE_ICON_ID,
        complicationStyleDrawableId = R.drawable.complication_red_style,
        primaryColorId = R.color.red_primary_color,
    ),

    GREEN(
        id = GREEN_COLOR_STYLE_ID,
        nameResourceId = GREEN_COLOR_STYLE_NAME_RESOURCE_ID,
        iconResourceId = GREEN_COLOR_STYLE_ICON_ID,
        complicationStyleDrawableId = R.drawable.complication_green_style,
        primaryColorId = R.color.green_primary_color,
    ),

    BLUE(
        id = BLUE_COLOR_STYLE_ID,
        nameResourceId = BLUE_COLOR_STYLE_NAME_RESOURCE_ID,
        iconResourceId = BLUE_COLOR_STYLE_ICON_ID,
        complicationStyleDrawableId = R.drawable.complication_blue_style,
        primaryColorId = R.color.blue_primary_color,
    ),

    YELLOW(
        id = YELLOW_COLOR_STYLE_ID,
        nameResourceId = YELLOW_COLOR_STYLE_NAME_RESOURCE_ID,
        iconResourceId = YELLOW_COLOR_STYLE_ICON_ID,
        complicationStyleDrawableId = R.drawable.complication_yellow_style,
        primaryColorId = R.color.yellow_primary_color,
    ), 
    GRAY(
        id = GRAY_COLOR_STYLE_ID,
        nameResourceId = GRAY_COLOR_STYLE_NAME_RESOURCE_ID,
        iconResourceId = GRAY_COLOR_STYLE_ICON_ID,
        complicationStyleDrawableId = R.drawable.complication_gray_style,
        primaryColorId = R.color.light_gray_primary_color,
    ), 
    WHITE(
        id = WHITE_COLOR_STYLE_ID,
        nameResourceId = WHITE_COLOR_STYLE_NAME_RESOURCE_ID,
        iconResourceId = WHITE_COLOR_STYLE_ICON_ID,
        complicationStyleDrawableId = R.drawable.complication_white_style,
        primaryColorId = R.color.white_primary_color,
    );

    companion object {
        /**
         * Translates the string id to the correct ColorStyleIdAndResourceIds object.
         */
        fun getColorStyleConfig(id: String): ColorStyleIdAndResourceIds {
            return when (id) {
                AMBIENT.id -> AMBIENT
                RED.id -> RED
                GREEN.id -> GREEN
                BLUE.id -> BLUE
                WHITE.id -> WHITE
                YELLOW.id -> YELLOW
                GRAY.id -> GRAY
                else -> WHITE
            }
        }

        /**
         * Returns a list of [UserStyleSetting.ListUserStyleSetting.ListOption] for all
         * ColorStyleIdAndResourceIds enums. The watch face settings APIs use this to set up
         * options for the user to select a style.
         */
        fun toOptionList(context: Context): List<ListUserStyleSetting.ListOption> {
            val colorStyleIdAndResourceIdsList=kotlin.enumValues<ColorStyleIdAndResourceIds>()

            return colorStyleIdAndResourceIdsList.map { colorStyleIdAndResourceIds ->
                ListUserStyleSetting.ListOption(
                    UserStyleSetting.Option.Id(colorStyleIdAndResourceIds.id),
                    context.resources,
                    colorStyleIdAndResourceIds.nameResourceId,
                    Icon.createWithResource(
                        context,
                        colorStyleIdAndResourceIds.iconResourceId
                    )
                )
            }
        }
    }
}
