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
import tk.glucodata.MainActivity
import tk.glucodata.Natives
import tk.glucodata.Notify
import java.lang.Math.min

class ArrowValueDataSourceService: SuspendingComplicationDataSourceService()  {
private var glview: GlucoseValue? =null

    override fun onComplicationActivated( complicationInstanceId: Int, type: ComplicationType) {
        Log.d(LOG_ID, "onComplicationActivated(): $complicationInstanceId")
    }
    override fun onComplicationDeactivated(complicationInstanceId: Int) {
        Log.d(LOG_ID, "onComplicationDeactivated(): $complicationInstanceId")
    }
fun getview(type: ComplicationType):GlucoseValue {
     if(glview==null) {
        val width:Int
        val height:Int
         if(type==PHOTO_IMAGE ) {
             val size= kotlin.math.min(MainActivity.screenheight, MainActivity.screenwidth)
               height=size
                width=size
 //            height= MainActivity.screenheight
  //           width=MainActivity.screenwidth
             }
           else {
             width = 100
             height = 100
            }
         glview= GlucoseValue(width,height)
         }
      return glview as GlucoseValue;
      }
    override fun getPreviewData(type: ComplicationType): ComplicationData {

	val icon=Icon.createWithBitmap( getview(type).previewbitmap())
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

    override suspend fun onComplicationRequest(request: ComplicationRequest): ComplicationData? {
        Log.d(LOG_ID, "onComplicationRequest() id: ${request.complicationInstanceId}")

        val complicationPendingIntent = Notify.mkpending();
    val type=        request.complicationType
      val glucose = Natives.lastglucose()
      val bitmap=
      if(glucose==null) {
         Log.i(LOG_ID,"glucose==null") 
	      getview(type).getnovalue()
         }
	else {
         Log.i(LOG_ID,"glucose==${glucose.value}") 
      getview(type).getArrowValueBitmap(glucose.value,glucose.time*1000L,glucose.index,glucose.rate)
	}

	val image=Icon.createWithBitmap(bitmap)
    return when (type) {
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
                Log.w(LOG_ID, "Unexpected complication type $type")
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
