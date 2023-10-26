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
/*      Fri Jan 27 15:33:27 CET 2023                                                 */


package tk.glucodata.watchface

import android.content.Context
import android.content.Context.*
import android.graphics.Canvas
import android.graphics.Color.*
import android.graphics.Paint
import android.graphics.Paint.Align
import android.graphics.Rect
import android.hardware.Sensor
import android.hardware.SensorEvent
import android.hardware.SensorEventListener
import android.hardware.SensorManager
import android.hardware.SensorManager.*
import android.view.SurfaceHolder
import androidx.core.content.ContextCompat.getSystemService
import androidx.wear.watchface.*
import androidx.wear.watchface.Renderer.CanvasRenderer2
import androidx.wear.watchface.complications.data.ComplicationType
import androidx.wear.watchface.complications.rendering.CanvasComplicationDrawable
import androidx.wear.watchface.complications.rendering.ComplicationDrawable
import androidx.wear.watchface.style.CurrentUserStyleRepository
import androidx.wear.watchface.style.UserStyle
import androidx.wear.watchface.style.UserStyleSetting
import androidx.wear.watchface.style.WatchFaceLayer
import kotlinx.coroutines.*
import tk.glucodata.*
import tk.glucodata.Applic.removescreenupdater
import tk.glucodata.Applic.setscreenupdater
import tk.glucodata.watchface.data.watchface.ColorStyleIdAndResourceIds
import tk.glucodata.watchface.data.watchface.WatchFaceColorPalette.Companion.convertToWatchFaceColorPalette
import tk.glucodata.watchface.data.watchface.WatchFaceData
import tk.glucodata.watchface.utils.COLOR_STYLE_SETTING
import tk.glucodata.watchface.utils.ratTimebaseY
import java.lang.Float.isNaN
import java.time.LocalTime
import java.time.ZonedDateTime
import kotlin.math.pow

// Default for how Long each frame is displayed at expected frame rate.
//private const val FRAME_PERIOD_MS_DEFAULT: Long = 16L
private const val FRAME_PERIOD_MS_DEFAULT: Long = 60000L

/**
 * Renders watch face via data in Room database. Also, updates watch face state based on setting
 * changes by user via [userStyleRepository.addUserStyleListener()].
 */
class WatchRenderer(
    private val context: Context,
    surfaceHolder: SurfaceHolder,
    watchState: WatchState,
    private val complicationSlotsManager: ComplicationSlotsManager,
    currentUserStyleRepository: CurrentUserStyleRepository,
    canvasType: Int
) : Renderer.CanvasRenderer2<WatchRenderer.InnerAssets>(
    surfaceHolder,
    currentUserStyleRepository,
    watchState,
    canvasType,
    FRAME_PERIOD_MS_DEFAULT,
    clearWithBackgroundTintBeforeRenderingHighlightLayer = false
) {
    class  InnerAssets: SharedAssets {
        override fun onDestroy() {
        }
    }
    override suspend fun createSharedAssets(): InnerAssets {
        return InnerAssets()
    }

    private val scope: CoroutineScope =
        CoroutineScope(SupervisorJob() + Dispatchers.Main.immediate)

    // Represents all data needed to render the watch face. All value defaults are constants. Only
    // three values are changeable by the user (color scheme, ticks being rendered, and length of
    // the minute arm). Those dynamic values are saved in the watch face APIs and we update those
    // here (in the renderer) through a Kotlin Flow.
    private var watchFaceData: WatchFaceData = WatchFaceData()

    // Converts resource ids into Colors and ComplicationDrawable.
    private var watchFaceColors = convertToWatchFaceColorPalette(
        context,
        watchFaceData.activeColorStyle,
        watchFaceData.ambientColorStyle
    )

    // Initializes paint object for painting the clock hands with default values.


    // Used to paint the main hour hand text with the hour pips, i.e., 3, 6, 9, and 12 o'clock.
    


    // Default size of watch face drawing area, that is, a no size rectangle. Will be replaced with
    // valid dimensions from the system.
private    val manage:SensorManager
private    var sensor:Sensor? = null
   private var heartrate= Float.NaN
   private var registered=false
val updater:Runnable= object: Runnable{
        override  fun run()
        {
            Log.i(LOG_ID, "postInvalidate")
            postInvalidate()
        }
    }
    val sensorlist: SensorEventListener 

    init {
        Log.i(LOG_ID,"init")
     sensorlist=    object:SensorEventListener {
        override fun onAccuracyChanged( sensor: Sensor, accuracy: Int ): Unit {

        }
        override fun onSensorChanged(event: SensorEvent) {
            if(event.accuracy>=SENSOR_STATUS_ACCURACY_LOW){
//            if(event.accuracy==SENSOR_STATUS_ACCURACY_MEDIUM||event.accuracy==SENSOR_STATUS_ACCURACY_HIGH) 
                heartrate= event.values[0]
              //  Log.i( LOG_ID, "onSensorChanged accuracy=" + event.accuracy + " HR=" + heartrate);
            }
            else {
               // Log.i(LOG_ID, "onSensorChanged low accuracy=${event.accuracy} HR=${event.values[0]}");
                heartrate= Float.NaN
            }
        }

    }

        manage = context.getSystemService(SENSOR_SERVICE) as SensorManager
        if(measureheart) {
            if(manage != null) {
                sensor = manage.getDefaultSensor(Sensor.TYPE_HEART_RATE)
                registersensor(Natives.getheartrate())
            }
        }

        scope.launch {
            currentUserStyleRepository.userStyle.collect { userStyle -> updateWatchFaceData(userStyle) }
        }

	setscreenupdater(updater)

//	thisone=this
    }


fun registersensor(on:Boolean):Boolean {
	if(on==registered)
		return on
	heartrate= Float.NaN
	if(sensor!=null) {
		Log.i(LOG_ID,"registersensor($on) sensor!=null")
		if(on) {
			manage.registerListener(sensorlist, sensor, SENSOR_DELAY_NORMAL)
			}
		else  {
		       manage.unregisterListener(sensorlist);
			}
	     registered=on;
	     }
    else
		Log.i(LOG_ID,"registersensor($on) sensor==null")
	    return  registered
	 }
	 /*
override fun shouldAnimate(): Boolean {
	return true;
	}
	*/

    /*
     * Triggered when the user makes changes to the watch face through the settings activity. The
     * function is called by a flow.
     */
    private fun updateWatchFaceData(userStyle: UserStyle) {
        Log.d(LOG_ID, "updateWatchFace(): $userStyle")
	registersensor(Natives.getheartrate())

        var newWatchFaceData: WatchFaceData=watchFaceData 
        // Loops through user style and applies new values to watchFaceData.
        for(options in userStyle) {
            when(options.key.id.toString()) {
                COLOR_STYLE_SETTING -> {
                    val listOption = options.value as UserStyleSetting.ListUserStyleSetting.ListOption
                    newWatchFaceData = newWatchFaceData.copy( activeColorStyle = ColorStyleIdAndResourceIds.getColorStyleConfig( listOption.id.toString()))
                    }
            }
        }
	if( watchFaceData != newWatchFaceData) {
            watchFaceData = newWatchFaceData

            // Recreates Color and ComplicationDrawable from resource ids.
            watchFaceColors = convertToWatchFaceColorPalette(
                context,
                watchFaceData.activeColorStyle,
                watchFaceData.ambientColorStyle
            )

            // Applies the user chosen complication color scheme changes. ComplicationDrawables for
            // each of the styles are defined in XML so we need to replace the complication's
            // drawables.
            for ((_, complication) in complicationSlotsManager.complicationSlots) {
                ComplicationDrawable.getDrawable( context, watchFaceColors.complicationStyleDrawableId)?.let { 
		        it.activeStyle.borderColor= TRANSPARENT
                        it.ambientStyle.borderColor= TRANSPARENT
		        (complication.renderer as CanvasComplicationDrawable).drawable = it
                }
            }
        }
    }

    override fun onDestroy() {
        Log.d(LOG_ID, "onDestroy()")
   
        //thisone=null;
        if(measureheart) {
            registersensor(false)
        }

	removescreenupdater(updater)
        scope.cancel("WatchRenderer scope clear() request")
        super.onDestroy()
    }

    override fun renderHighlightLayer(canvas: Canvas, bounds: Rect, zonedDateTime: ZonedDateTime,sharedAssets: InnerAssets
                                      ) {
        Log.d(LOG_ID,"renderHighlightLayer")

        canvas.drawColor(BLACK)

        for((_, complication) in complicationSlotsManager.complicationSlots) {
            if(complication.enabled) {
                complication.renderHighlightLayer(canvas, zonedDateTime, renderParameters)
            }
        }
    }
private fun watchtime(canvas:Canvas,localtime: LocalTime) {
    with(canvas) {
        val timey = height * ratTimebaseY
        if (Applic.hour24) {
            val timex = width * 0.49f
            timePaint.textSize = height * .25f
            timePaint.setTextAlign(Align.CENTER)
            drawText(
                String.format("%02d:%02d", localtime.getHour(), localtime.getMinute()),
                timex,
                timey,
                timePaint
            )
        } else {
            val timex = width * 0.71f
            val hour = localtime.getHour();
            val daypart = if (hour >= 12) "pm" else "am"
            var hour12 = hour % 12
            if (hour12 == 0)
                hour12 = 12;
            timePaint.textSize = height * .22f
            timePaint.setTextAlign(Align.RIGHT)
            drawText(
                String.format("%d:%02d", hour12, localtime.getMinute()),
                timex,
                timey,
                timePaint
            )
            timePaint.textSize *= 0.4f
            timePaint.setTextAlign(Align.LEFT)
            drawText(daypart, timex, timey, timePaint)
        }
        }
	}

var rendertime:Long=0L

    override fun render(canvas: Canvas, bounds: Rect, zonedDateTime: ZonedDateTime,sharedAssets:InnerAssets) {
	registersensor(Natives.getheartrate())
    Log.i(LOG_ID, "render");
        canvas.drawColor(BLACK)
        drawComplications(canvas, zonedDateTime)
        if(renderParameters.watchFaceLayers.contains(WatchFaceLayer.COMPLICATIONS_OVERLAY)) {
            val unixtime = zonedDateTime.toEpochSecond()
            rendertime=unixtime
            val glucose: strGlucose? = Natives.lastglucose()
                val drawAmbient = renderParameters.drawMode == DrawMode.AMBIENT
                if (drawAmbient) {
                    timePaint.color = watchFaceColors.ambientPrimaryColor
                    glucosePaint.color = watchFaceColors.ambientPrimaryColor
                    agePaint.color = GRAY
                }
            else {
                    timePaint.color = watchFaceColors.activePrimaryColor
                    glucosePaint.color = watchFaceColors.activePrimaryColor
                    agePaint.setARGB(0xFF, 0xFF, 0, 0xFF)
                }

              glucosePaint.textSize =canvas.height*.25f
		val localtime= zonedDateTime.toLocalTime()
		watchtime(canvas,localtime)
                if(glucose!=null)  {
   			val density=canvas.height/250.0f
			val getx= canvas.width*0.5f
			val gety= canvas.height*0.66f
			showglucose(canvas,glucosePaint,agePaint,getx,gety,density,unixtime,glucose)
			}
		else
		   	Log.i(LOG_ID,"glucose==null")
	 if(!heartrate.isNaN()) {
         with(canvas) {
             timePaint.textSize= height * .08f
             timePaint.setTextAlign(Align.RIGHT)
             val hearty = height * 0.38f
             val heartx=0.95f*width
             drawText(
                 String.format("%.0f", heartrate),
                 heartx,
                 hearty,
                 timePaint
             )
         }
	 	}
	}
    }


    private fun isActive(slot:ComplicationSlot ): Boolean {
        return when (slot.complicationData.value.type) {
            ComplicationType.NO_DATA -> false
            ComplicationType.NO_PERMISSION -> false
            ComplicationType.EMPTY -> false
            else -> true
        }
    }

    // ----- All drawing functions -----
    private fun drawComplications(canvas: Canvas, zonedDateTime: ZonedDateTime) {
        for ((_, complication) in complicationSlotsManager.complicationSlots) {
            if(isActive(complication)) {
                complication.render(canvas, zonedDateTime, renderParameters)
            }
        }
    }


    companion object {
        private const val LOG_ID = "WatchRenderer"

       private  val    measureheart=true;
//	private  var thisone:WatchRenderer?=null
/*
  public fun setheartrate(on:Boolean):Boolean {
      val wasone=thisone
      if(wasone==null) {
      		Log.i(LOG_ID,"sethearrate($on) wasone==null")
            return false;
	    }
	return wasone.registersensor(on)
	} */
    private val timePaint = Paint().apply {
        isAntiAlias = true
        textAlign = Align.CENTER
        color = WHITE
    	}
    private val glucosePaint = Paint().apply {
        isAntiAlias = true
        textAlign = Align.CENTER
        color = WHITE
    }
    private val agePaint = Paint().apply {
        setARGB(0xFF,0xFF,0,0xFF)
       }
private fun	showglucose(canvas:Canvas,glucosePaint:Paint,agePaint:Paint,getxin:Float,gety:Float,density:Float,unixtime:Long,glucose:strGlucose)  {
		var getx=getxin
		    Log.i(LOG_ID,"glucose=${glucose.value} time=${glucose.time}")
		     var age:Int=(unixtime-glucose.time).toInt()
		     val oldage=(60.0f*3.0f)
		   if(age<oldage) {
		   	with(canvas) {
			    if(age<0) age=0
			   val rate=glucose.rate
				if(rate.isNaN()) {
//					getx=width*0.45f
					getx*=0.82f
					}
				else  {
				    CommonCanvas.drawarrow(this,glucosePaint,density,rate,width*.25f,height*.55f)
				    }
			    drawText(glucose.value,getx,gety, glucosePaint)
			    val idbounds=Rect()
			    glucosePaint.textSize/=5.0f
			    val sensorid=glucose.sensorid
			    glucosePaint.getTextBounds(sensorid, 0,sensorid.length, idbounds)
			    val yid= gety+idbounds.height()*1.5f
			    val wid= idbounds.width()
			    val relage=age*wid/oldage
			    val xage= getx-wid*.5f
			    val hid= idbounds.height()
			    drawRect( xage,yid-hid*1.05f,xage +relage, yid+0.07f*hid,agePaint)
			    drawText(sensorid,getx,yid, glucosePaint)
			    }
		 	}
		else {
			Log.i(LOG_ID,"age ($age) >= oldage ($oldage)")
			}

	}
    }



}
