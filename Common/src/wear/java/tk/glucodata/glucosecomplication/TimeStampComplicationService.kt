package tk.glucodata.glucosecomplication

import android.content.ComponentName
import android.support.wearable.complications.ComplicationData
import androidx.wear.watchface.complications.data.ComplicationType
import androidx.wear.watchface.complications.data.PlainComplicationText
import androidx.wear.watchface.complications.data.RangedValueComplicationData
import androidx.wear.watchface.complications.datasource.ComplicationDataSourceUpdateRequester
import androidx.wear.watchface.complications.datasource.ComplicationRequest
import androidx.wear.watchface.complications.datasource.SuspendingComplicationDataSourceService
import tk.glucodata.Applic
import tk.glucodata.Log
import tk.glucodata.Natives

class TimeStampComplicationService : SuspendingComplicationDataSourceService() {


    override suspend fun onComplicationRequest(request: ComplicationRequest): androidx.wear.watchface.complications.data.ComplicationData? {
        if (request.complicationType != ComplicationType.RANGED_VALUE) {
            return null
        }
      return getlasttime(false)
    }

    override fun getPreviewData(type: ComplicationType): androidx.wear.watchface.complications.data.ComplicationData? {
      if (type != ComplicationType.RANGED_VALUE) return null
      return getlasttime(true)
    }
    companion object {
        private const val LOG_ID = "TimeStampComplicationService"
        private val max = Math.nextDown(Float.MAX_VALUE)

       private val complicationDataSourceUpdateRequester = ComplicationDataSourceUpdateRequester.create( context=tk.glucodata.Applic.app, complicationDataSourceComponent = ComponentName(
               Applic.app,
               TimeStampComplicationService::class.java
           )
           )

       public fun update() {
            complicationDataSourceUpdateRequester.requestUpdateAll()
        }
     private   fun encodeEpochSeconds(sec: Long): Float {
            val two23 = 8388608.0            // 2^23

            if (sec == 0L) return 0f

            return if (sec < two23.toLong()) {
                (sec * Math.pow(2.0, -149.0)).toFloat()
            } else {
                val eField = Math.floor(sec / two23).toInt()   // 1..254 for current timestamps
                val mant = sec - (eField * two23).toLong()
                (Math.pow(2.0, eField - 127.0) * (1.0 + mant / two23)).toFloat()
            }
        }

      private  fun getlasttime(ex:Boolean):  androidx.wear.watchface.complications.data.ComplicationData? {
            val glucose = Natives.lastglucose()
            var sensorReadingTimeSec:Long;
            if(glucose==null) {
                Log.i(LOG_ID,"lastglucose=null")
                if(!ex)
                    return null;
                sensorReadingTimeSec=((System.currentTimeMillis()/1000L)+60*60);
            }
            else
                sensorReadingTimeSec=glucose.time
//    val relTimeSec: Float = (sensorReadingTimeSec % 86400L).toFloat()
            val relTimeSec: Float = encodeEpochSeconds(sensorReadingTimeSec)


            return RangedValueComplicationData.Builder(
                value = relTimeSec,
                min = 0f,
                max = max,
                contentDescription = PlainComplicationText.Builder("Reading Time").build()
            )
                .setText(PlainComplicationText.Builder(" ").build())
                .build()


        }
        }
}
