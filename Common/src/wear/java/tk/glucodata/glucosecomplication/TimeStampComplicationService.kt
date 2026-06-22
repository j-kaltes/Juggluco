package tk.glucodata.glucosecomplication

import android.annotation.SuppressLint
import android.content.ComponentName
import android.os.Build
import androidx.wear.protolayout.expression.DynamicBuilders.DynamicInstant
import androidx.wear.watchface.complications.data.ComplicationType
import androidx.wear.watchface.complications.data.PlainComplicationText
import androidx.wear.watchface.complications.data.RangedValueComplicationData
import androidx.wear.watchface.complications.datasource.ComplicationDataSourceUpdateRequester
import androidx.wear.watchface.complications.datasource.ComplicationRequest
import androidx.wear.watchface.complications.datasource.SuspendingComplicationDataSourceService
import java.time.Instant
import tk.glucodata.Applic
import tk.glucodata.Log
import tk.glucodata.Natives
import tk.glucodata.R

class TimeStampComplicationService : SuspendingComplicationDataSourceService() {

    override suspend fun onComplicationRequest(
        request: ComplicationRequest
    ): androidx.wear.watchface.complications.data.ComplicationData? {
        if (request.complicationType != ComplicationType.RANGED_VALUE) {
            return null
        }
        return getlasttime(false)
    }

    override fun getPreviewData(
        type: ComplicationType
    ): androidx.wear.watchface.complications.data.ComplicationData? {
        if (type != ComplicationType.RANGED_VALUE) return null
        return getlasttime(true)
    }

    companion object {
        private const val LOG_ID = "TimeStampComplicationService"
        private const val MAX_AGE_SECONDS = 330L

        private val complicationDataSourceUpdateRequester =
            ComplicationDataSourceUpdateRequester.create(
                context = tk.glucodata.Applic.app,
                complicationDataSourceComponent = ComponentName(
                    Applic.app,
                    TimeStampComplicationService::class.java
                )
            )

        fun update() {
            // Call this when a new glucose reading arrives. Do not call it every second.
            complicationDataSourceUpdateRequester.requestUpdateAll()
        }

        @SuppressLint("NewApi")
        private fun getlasttime(
            ex: Boolean
        ): androidx.wear.watchface.complications.data.ComplicationData? {
            val nowSec = System.currentTimeMillis() / 1000L
            val glucose = Natives.lastglucose()

            val sensorReadingTimeSec: Long =
                if (glucose == null) {
                    Log.i(LOG_ID, "lastglucose=null")
                    if (!ex) return null
                    nowSec - 60
                } else {
                    glucose.time
                }

            val ageFallbackSec =
                (nowSec - sensorReadingTimeSec)
                    .coerceIn(0L, MAX_AGE_SECONDS)
                    .toFloat()

            Log.i(
                LOG_ID,
                "request time=$nowSec, glucose.time=$sensorReadingTimeSec, ageFallback=$ageFallbackSec"
            )

            val contentDescription = PlainComplicationText.Builder(R.string.jugglucoupdate.toString()).build()

            val builder =
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                    val ageDynamic =
                        DynamicInstant
                            .withSecondsPrecision(Instant.ofEpochSecond(sensorReadingTimeSec))
                            .durationUntil(DynamicInstant.platformTimeWithSecondsPrecision())
                            .toIntSeconds()
                            .asFloat()

                    RangedValueComplicationData.Builder(
                        dynamicValue = ageDynamic,
                        fallbackValue = ageFallbackSec,
                        min = 0f,
                        max = MAX_AGE_SECONDS.toFloat(),
                        contentDescription = contentDescription
                    )
                } else {
                    RangedValueComplicationData.Builder(
                        value = ageFallbackSec,
                        min = 0f,
                        max = MAX_AGE_SECONDS.toFloat(),
                        contentDescription = contentDescription
                    )
                }

            return builder
                .setText(PlainComplicationText.Builder(" ").build())
                .build()
        }
    }
}
