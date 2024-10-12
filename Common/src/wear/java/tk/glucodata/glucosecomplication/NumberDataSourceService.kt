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
import androidx.wear.watchface.complications.data.SmallImage
import androidx.wear.watchface.complications.data.PlainComplicationText
import androidx.wear.watchface.complications.data.SmallImageComplicationData
import androidx.wear.watchface.complications.data.SmallImageType
import androidx.wear.watchface.complications.datasource.ComplicationDataSourceUpdateRequester
import androidx.wear.watchface.complications.datasource.ComplicationRequest
import androidx.wear.watchface.complications.datasource.SuspendingComplicationDataSourceService
import tk.glucodata.Applic
import tk.glucodata.Log
import tk.glucodata.Natives
import tk.glucodata.Notify

class NumberDataSourceService: SuspendingComplicationDataSourceService()  {
private val glview= GlucoseValue(100,100)

    override fun onComplicationActivated( complicationInstanceId: Int, type: ComplicationType) {
        Log.d(LOG_ID, "onComplicationActivated(): $complicationInstanceId")
    }
    override fun onComplicationDeactivated(complicationInstanceId: Int) {
        Log.d(LOG_ID, "onComplicationDeactivated(): $complicationInstanceId")
    }

    override fun getPreviewData(type: ComplicationType): ComplicationData {
        val value: String
        val time: Long
        val index: Int
        val glucose = Natives.lastglucose()
   	val now = System.currentTimeMillis()
        if (glucose != null&&(now-glucose.time)<tk.glucodata.Notify.glucosetimeout) {
            value = glucose.value
            time = glucose.time * 1000L
            index = glucose.index
        } else {
            value = if (Applic.unit == 1) "5.6" else "101"
            index = 0
	    time=now
        }
        return SmallImageComplicationData.Builder(
            smallImage =  SmallImage.Builder( Icon.createWithBitmap(glview.getNumberBitmap(value,time,index,now)), SmallImageType.PHOTO).build(),
            contentDescription = PlainComplicationText.Builder(text = "Glucose Value").build() )
            .setTapAction(null)
            .build()
    }

    override suspend fun onComplicationRequest(request: ComplicationRequest): ComplicationData? {
        Log.d(LOG_ID, "onComplicationRequest() id: ${request.complicationInstanceId}")

        val complicationPendingIntent = Notify.mkpending();

        return when (request.complicationType) {
            ComplicationType.SMALL_IMAGE-> {
	      val glucose = Natives.lastglucose()
	      var image=
	      if(glucose==null) {
		 Log.i(LOG_ID,"glucose==null") 
		  glview.getnovalue()
		 }
	      else {
		    Log.i(LOG_ID,"glucose==${glucose.value}") 
   		   val now = System.currentTimeMillis()
		   glview.getNumberBitmap(glucose.value,glucose.time*1000L,glucose.index,now)
		  }
                SmallImageComplicationData.Builder( SmallImage.Builder( Icon.createWithBitmap(image), SmallImageType.PHOTO).build(), contentDescription = PlainComplicationText.Builder("Glucose Number").build()).setTapAction(complicationPendingIntent).build()
		}

            else -> {
                Log.w(LOG_ID, "Unexpected complication type ${request.complicationType}")
                null
            }
        }
    }


    companion object {
        private const val LOG_ID = "NumberDataSourceService"
   private val complicationDataSourceUpdateRequester = ComplicationDataSourceUpdateRequester.create( context=tk.glucodata.Applic.app, complicationDataSourceComponent = ComponentName(tk.glucodata.Applic.app,
       NumberDataSourceService::class.java
   ))

        public fun update() {
            complicationDataSourceUpdateRequester.requestUpdateAll()
        }
    }
}
