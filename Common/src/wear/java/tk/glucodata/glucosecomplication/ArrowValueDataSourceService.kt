package tk.glucodata.glucosecomplication


import android.content.ComponentName
import android.graphics.drawable.Icon
import androidx.wear.watchface.complications.data.ComplicationData
import androidx.wear.watchface.complications.data.ComplicationType
import androidx.wear.watchface.complications.data.ComplicationType.*
import androidx.wear.watchface.complications.data.MonochromaticImage
import androidx.wear.watchface.complications.data.MonochromaticImageComplicationData
import androidx.wear.watchface.complications.data.PhotoImageComplicationData
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

class ArrowValueDataSourceService: SuspendingComplicationDataSourceService()  {
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
	val icon=Icon.createWithBitmap( glview.previewbitmap())
        return when (type) {
        /*
         MONOCHROMATIC_IMAGE -> {
            Log.i(LOG_ID,"MonochromaticImage")
            MonochromaticImageComplicationData.Builder(
                MonochromaticImage.Builder (Icon.createWithBitmap(
                        glview.getArrowValueBitmap(
                            value,
                            time,
                            index,
                            rate
                        )
                    )).build(), contentDescription = PlainComplicationText.Builder("Glucose+arrow").build()
                ).setTapAction(null).build()
			    } */
            PHOTO_IMAGE -> {
                Log.i(LOG_ID,"getPreviewData PHOTO_IMAGE")
                PhotoImageComplicationData.Builder(photoImage = icon, contentDescription = PlainComplicationText.Builder("Glucose+arrow").build()
                ).setTapAction(null).build()
            } 
         else -> 
            //ComplicationType.SMALL_IMAGE -> 
            {
               Log.i(LOG_ID,"getPreviewData OTHER")
                 SmallImageComplicationData.Builder(
                    smallImage = SmallImage.Builder( icon, SmallImageType.PHOTO).build(),
                    contentDescription = PlainComplicationText.Builder(text = "Glucose+arrow")
                        .build()
                )
                    .setTapAction(null)
                    .build()
            }

        }
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
      //  val thisDataSource = ComponentName(this, javaClass)
        // We pass the complication id, so we can only update the specific complication tapped.
//        val complicationPendingIntent = TapBroadcastReceiver.getToggleIntent( this, thisDataSource, request.complicationInstanceId)
        val complicationPendingIntent = Notify.mkpending();

      val glucose = Natives.lastglucose()
      val bitmap=
      if(glucose==null) {
         Log.i(LOG_ID,"glucose==null") 
	      glview.getnovalue()
         }
	else {
         Log.i(LOG_ID,"glucose==${glucose.value}") 
	glview.getArrowValueBitmap(glucose.value,glucose.time*1000L,glucose.index,glucose.rate)
	}

	val image=Icon.createWithBitmap(bitmap)
    return when (request.complicationType) {
        /* MONOCHROMATIC_IMAGE -> {
            Log.i(LOG_ID,"MonochromaticImage")
            MonochromaticImageComplicationData.Builder(
                    MonochromaticImage.Builder(Icon.createWithBitmap(glview.getArrowValueBitmap(glucose.value,glucose.time*1000L,glucose.index,glucose.rate))).build(), contentDescription = PlainComplicationText.Builder("Glucose Number").build()).setTapAction(complicationPendingIntent).build()
			    } */
         PHOTO_IMAGE -> {
         Log.i(LOG_ID,"PHOTO_IMAGE")
                PhotoImageComplicationData.Builder( image , contentDescription = PlainComplicationText.Builder("Glucose Arrow+Value").build()).setTapAction(complicationPendingIntent).build()
			    }
           SMALL_IMAGE -> {
         Log.i(LOG_ID,"SMALL_IMAGE")
                SmallImageComplicationData.Builder( SmallImage.Builder( image, SmallImageType.PHOTO).build(), contentDescription = PlainComplicationText.Builder("Glucose Arrow+Value").build()).setTapAction(complicationPendingIntent).build()
//		setAmbientImage(Icon ambientImage) ??
			    }
            else -> {
                Log.w(LOG_ID, "Unexpected complication type ${request.complicationType}")
                null
            }
            }

    }


    companion object {
        private const val LOG_ID = "ArrowValueDataSourceService"
   private val complicationDataSourceUpdateRequester = ComplicationDataSourceUpdateRequester.create( context=tk.glucodata.Applic.app, complicationDataSourceComponent = ComponentName(tk.glucodata.Applic.app,
       ArrowValueDataSourceService::class.java
   ))

        public fun update() {
            complicationDataSourceUpdateRequester.requestUpdateAll()
        }
    }
}
