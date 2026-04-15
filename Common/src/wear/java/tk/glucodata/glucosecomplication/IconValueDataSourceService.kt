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
/*      Fri Oct 11 12:22:15 CEST 2024                                                 */


package tk.glucodata.glucosecomplication


import android.content.ComponentName
import android.graphics.drawable.Icon
import androidx.wear.watchface.complications.data.ComplicationData
import androidx.wear.watchface.complications.data.ComplicationText
import androidx.wear.watchface.complications.data.ComplicationType
import androidx.wear.watchface.complications.data.ComplicationType.*
import androidx.wear.watchface.complications.data.MonochromaticImage
import androidx.wear.watchface.complications.data.MonochromaticImageComplicationData
import androidx.wear.watchface.complications.data.PhotoImageComplicationData
import androidx.wear.watchface.complications.data.SmallImage
import androidx.wear.watchface.complications.data.PlainComplicationText
import androidx.wear.watchface.complications.data.ShortTextComplicationData
import androidx.wear.watchface.complications.data.SmallImageComplicationData
import androidx.wear.watchface.complications.data.SmallImageType
import androidx.wear.watchface.complications.datasource.ComplicationDataSourceUpdateRequester
import androidx.wear.watchface.complications.datasource.ComplicationRequest
import androidx.wear.watchface.complications.datasource.SuspendingComplicationDataSourceService
import tk.glucodata.Applic
import tk.glucodata.Log
import tk.glucodata.MainActivity
import tk.glucodata.Natives
import tk.glucodata.Notify
import tk.glucodata.R
import java.lang.Math.min

class IconValueDataSourceService: SuspendingComplicationDataSourceService()  {
private var glview: GlucoseValue? =null

    override fun onComplicationActivated( complicationInstanceId: Int, type: ComplicationType) {
        Log.d(LOG_ID, "onComplicationActivated(): $complicationInstanceId")
    }
    override fun onComplicationDeactivated(complicationInstanceId: Int) {
        Log.d(LOG_ID, "onComplicationDeactivated(): $complicationInstanceId")
    }
fun getview(type: ComplicationType):GlucoseValue {
     if(glview==null) {
        val width:Int = 150
        val height:Int = 150
         glview= GlucoseValue(width,height)
         }
      return glview as GlucoseValue;
      }
    override fun getPreviewData(type: ComplicationType): ComplicationData {
        val value: String
        val time: Long
        val index: Int
        val glucose = Natives.lastglucose()
        val now = System.currentTimeMillis()
        if (glucose != null&&(now-glucose.time*1000L)<tk.glucodata.Notify.glucosetimeout) {
            time = glucose.time * 1000L
            value = glucose.value
        } else {
            value = if (Applic.unit == 1) "5.6" else "101"
	        time=now
        }
        val showtime=Natives.gettimeOnComplication()
        return MonochromaticImageComplicationData.Builder(
             MonochromaticImage.Builder( Icon.createWithBitmap(getview(type).getNumberBitmap(value,time,-1,now,showtime))).build(),
            contentDescription = PlainComplicationText.Builder(text = "Glucose Value").build() )
            .setTapAction(null)
            .build()
    }

    override suspend fun onComplicationRequest(request: ComplicationRequest): ComplicationData? {
        Log.d(LOG_ID, "onComplicationRequest() id: ${request.complicationInstanceId}")
        val type=        request.complicationType
    if(type== MONOCHROMATIC_IMAGE) {
            val glucose = Natives.lastglucose()
            val now = System.currentTimeMillis()
            val bitmap= if(glucose==null ||(now-glucose.time*1000L)>=tk.glucodata.Notify.glucosetimeout){
                  Log.i(LOG_ID,"MonochromaticImage novalue")
                 getview(type).getnovalue()
                 } else {
                     Log.i(LOG_ID,"MonochromaticImage ${glucose.value}")
                    val showtime=Natives.gettimeOnComplication()
                    getview(type).getNumberBitmap(glucose.value,glucose.time*1000L,-1,now,showtime)
                     }
             val image=Icon.createWithBitmap(bitmap)
             val complicationPendingIntent = Notify.mkpending()
            return MonochromaticImageComplicationData.Builder(
                    MonochromaticImage.Builder(image).build(), contentDescription = PlainComplicationText.Builder("Glucose Value").build()).setTapAction(complicationPendingIntent).build()
            } 
    else {
                Log.w(LOG_ID, "Unexpected complication type ${request.complicationType}")
                return null
           }
    }

    companion object {
        private const val LOG_ID = "IconValueDataSourceService"
   private val complicationDataSourceUpdateRequester = ComplicationDataSourceUpdateRequester.create( context=tk.glucodata.Applic.app, complicationDataSourceComponent = ComponentName(tk.glucodata.Applic.app, IconValueDataSourceService::class.java))

        public fun update() {
            complicationDataSourceUpdateRequester.requestUpdateAll()
        }
    }
}
