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
private val glview= GlucoseValue()

    override fun onComplicationActivated( complicationInstanceId: Int, type: ComplicationType) {
        Log.d(LOG_ID, "onComplicationActivated(): $complicationInstanceId")
    }
    override fun onComplicationDeactivated(complicationInstanceId: Int) {
        Log.d(LOG_ID, "onComplicationDeactivated(): $complicationInstanceId")
    }

    /*
     * A request for representative preview data for the complication, for use in the editor UI.
     * Preview data is assumed to be static per type. E.g. for a complication that displays the
     * date and time of an event, rather than returning the real time it should return a fixed date
     * and time such as 10:10 Aug 1st.
     *
     * This will be called on a background thread.
     */
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

    /*
     * Called when the complication needs updated data from your data source. There are four
     * scenarios when this will happen:
     *
     *   1. An active watch face complication is changed to use this data source
     *   2. A complication using this data source becomes active
     *   3. The period of time you specified in the manifest has elapsed (UPDATE_PERIOD_SECONDS)
     *   4. You triggered an update from your own class via the
     *       ComplicationDataSourceUpdateRequester.requestUpdate method.
     */
//onComplicationUpdate
    override suspend fun onComplicationRequest(request: ComplicationRequest): ComplicationData? {
        Log.d(LOG_ID, "onComplicationRequest() id: ${request.complicationInstanceId}")

        // Create Tap Action so that the user can trigger an update by tapping the complication.
        //val thisDataSource = ComponentName(this, javaClass)
        // We pass the complication id, so we can only update the specific complication tapped.
        //val complicationPendingIntent = TapBroadcastReceiver.getToggleIntent( this, thisDataSource, request.complicationInstanceId)
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
