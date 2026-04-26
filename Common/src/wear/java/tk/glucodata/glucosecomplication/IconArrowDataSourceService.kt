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
import androidx.wear.watchface.complications.data.ComplicationType
import androidx.wear.watchface.complications.data.ComplicationType.*
import androidx.wear.watchface.complications.data.MonochromaticImage
import androidx.wear.watchface.complications.data.MonochromaticImageComplicationData
import androidx.wear.watchface.complications.data.PlainComplicationText
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

class IconArrowDataSourceService: SuspendingComplicationDataSourceService()  {
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
      val glucose=Natives.lastglucose()
      val time:Long
      val rate:Float
      if(glucose==null) {
          rate=1.0f
          time= System.currentTimeMillis();

          }
      else  {
                time=glucose.time*1000L
                rate= glucose.rate
                }
      return MonochromaticImageComplicationData.Builder(
//          MonochromaticImage.Builder( Icon.createWithBitmap(getview(type).getArrowBitmap(rate))).build(),
          MonochromaticImage.Builder( Icon.createWithBitmap(getview(type).getArrowTimeBitmap(time,rate))).build(),

            contentDescription = PlainComplicationText.Builder(text = "Glucose Arrow").build() )
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
                     Log.i(LOG_ID,"MonochromaticImage rate: ${glucose.rate}")
//                    getview(type).getArrowBitmap(glucose.rate)
                      getview(type).getArrowTimeBitmap(glucose.time*1000L,glucose.rate);
                     }
             val image=Icon.createWithBitmap(bitmap)

            val complicationPendingIntent = Notify.mkpendingall(this,1002);
            return MonochromaticImageComplicationData.Builder(
                    MonochromaticImage.Builder(image).build(), contentDescription = PlainComplicationText.Builder("Glucose Arrow").build()).setTapAction(complicationPendingIntent).build()
            } 
    else {
                Log.w(LOG_ID, "Unexpected complication type ${request.complicationType}")
                return null
           }
    }

    companion object {
        private const val LOG_ID = "IconArrowDataSourceService"
   private val complicationDataSourceUpdateRequester = ComplicationDataSourceUpdateRequester.create( context=tk.glucodata.Applic.app, complicationDataSourceComponent = ComponentName(tk.glucodata.Applic.app, IconArrowDataSourceService::class.java))

        public fun update() {
            complicationDataSourceUpdateRequester.requestUpdateAll()
        }
    }
}
