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

class ShortArrowValueDataSourceService: SuspendingComplicationDataSourceService()  {
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
      val rate:Float
      val value:String
      val now = System.currentTimeMillis()
      val time: Long
     if (glucose != null&&(now-glucose.time*1000L)<tk.glucodata.Notify.glucosetimeout) {
          rate = glucose.rate
          value = glucose.value
           time = glucose.time * 1000L
           }
      else {
           rate =1.0f
            value = if (Applic.unit == 1) "5.6" else "101"
            time=now
            }

       val icon=Icon.createWithBitmap( getview(type).getArrowTimeBitmap(time,rate));
       Log.i(LOG_ID,"getPreviewData OTHER")
         return ShortTextComplicationData.Builder(text= PlainComplicationText.Builder(text = value).build()
         ,contentDescription = PlainComplicationText.Builder(text = "Small Glucose").build())
            .setSmallImage(SmallImage.Builder( icon, SmallImageType.PHOTO).build())
            .setMonochromaticImage(MonochromaticImage.Builder( icon).build())
            .setTapAction(null)
            .build()
        }

    override suspend fun onComplicationRequest(request: ComplicationRequest): ComplicationData? {
        Log.d(LOG_ID, "onComplicationRequest() id: ${request.complicationInstanceId}")
        val complicationPendingIntent = Notify.mkpendingall(this,1002);
        val type=        request.complicationType
      val glucose = Natives.lastglucose()
   	 val now = System.currentTimeMillis()
      if(glucose==null ||(now-glucose.time*1000L)>=tk.glucodata.Notify.glucosetimeout) {
         Log.i(LOG_ID,"no glucose") 
           return ShortTextComplicationData.Builder(text= PlainComplicationText.Builder(text = Applic.app.getString( R.string.novalue)).build()
             ,contentDescription = PlainComplicationText.Builder(text = "Small Glucose").build())
                .setTapAction(complicationPendingIntent)
                .build()
         }
      else {

            val bitmap=getview(type).getArrowTimeBitmap(glucose.time*1000L,glucose.rate);
            Log.i(LOG_ID," glucose==${glucose.value}") 
                val image=Icon.createWithBitmap(bitmap)
             return ShortTextComplicationData.Builder(text= PlainComplicationText.Builder(text = glucose.value).build()
             ,contentDescription = PlainComplicationText.Builder(text = "Small Glucose").build())
                .setSmallImage(SmallImage.Builder( image, SmallImageType.PHOTO).build())
                .setMonochromaticImage(MonochromaticImage.Builder( image).build())
                .setTapAction(complicationPendingIntent)
                .build()
            }
    }

    companion object {
        private const val LOG_ID = "ShortArrowValueDataSourceService"
   private val complicationDataSourceUpdateRequester = ComplicationDataSourceUpdateRequester.create( context=tk.glucodata.Applic.app, complicationDataSourceComponent = ComponentName(tk.glucodata.Applic.app,
    ShortArrowValueDataSourceService::class.java
   ))

        public fun update() {
            complicationDataSourceUpdateRequester.requestUpdateAll()
        }
    }
}
