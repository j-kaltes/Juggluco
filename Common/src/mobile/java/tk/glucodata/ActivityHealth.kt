// SPDX-License-Identifier: GPL-3.0-or-later
package tk.glucodata

import android.app.job.JobInfo
import android.app.job.JobParameters
import android.app.job.JobScheduler
import android.app.job.JobService
import android.content.ComponentName
import android.content.Context
import android.os.Build
import android.os.Handler
import android.os.Looper
import androidx.health.connect.client.HealthConnectClient
import androidx.health.connect.client.HealthConnectFeatures
import androidx.health.connect.client.aggregate.AggregateMetric
import androidx.health.connect.client.changes.DeletionChange
import androidx.health.connect.client.changes.UpsertionChange
import androidx.health.connect.client.permission.HealthPermission
import androidx.health.connect.client.records.*
import androidx.health.connect.client.records.metadata.DataOrigin
import androidx.health.connect.client.request.AggregateRequest
import androidx.health.connect.client.request.ChangesTokenRequest
import androidx.health.connect.client.request.ReadRecordsRequest
import androidx.health.connect.client.time.TimeRangeFilter
import java.time.Instant
import java.util.concurrent.atomic.AtomicBoolean
import kotlin.reflect.KClass
import kotlinx.coroutines.*

/** Android API adapter. Native code owns all persistent state and Amounts. */
object ActivityHealth {
    private val scope=CoroutineScope(SupervisorJob()+Dispatchers.IO)
    private val running=AtomicBoolean(false)
    private val handler=Handler(Looper.getMainLooper())
    private const val jobId=0x4a474143
    private const val background="android.permission.health.READ_HEALTH_DATA_IN_BACKGROUND"
    private const val history="android.permission.health.READ_HEALTH_DATA_HISTORY"
    private val types=listOf<KClass<out Record>>(ExerciseSessionRecord::class,HeartRateRecord::class,
        RestingHeartRateRecord::class,DistanceRecord::class,ActiveCaloriesBurnedRecord::class,
        SpeedRecord::class,StepsRecord::class,ElevationGainedRecord::class,PowerRecord::class,
        CyclingPedalingCadenceRecord::class)
    @Volatile private var message=""
    @Volatile private var visible=false
    @Volatile private var waiting:MainActivity?=null
    private var periodic:Runnable?=null

    @JvmStatic fun status():String=message
    @JvmStatic fun available():Boolean=Build.VERSION.SDK_INT>=28
    private fun ready()=available()&&Applic.Nativesloaded&&Applic.initproccalled&&Applic.stopprogram==0
    @JvmStatic fun typeCodes():IntArray=ExerciseSessionRecord::class.java.fields
        .filter { it.name.startsWith("EXERCISE_TYPE_") }.map {it.getInt(null)}.distinct().sorted().toIntArray()
    @JvmStatic fun typeName(type:Int):String=ExerciseSessionRecord::class.java.fields
        .firstOrNull{it.name.startsWith("EXERCISE_TYPE_")&&it.getInt(null)==type}?.name
        ?.removePrefix("EXERCISE_TYPE_")?.lowercase()?.replace('_',' ')
        ?.replaceFirstChar {it.uppercase()}?:"Activity $type"

    private fun client(context:Context):HealthConnectClient {
        check(available()&&HealthConnectClient.getSdkStatus(context)==HealthConnectClient.SDK_AVAILABLE) {
            "Health Connect is unavailable or needs updating"
        }
        return HealthConnectClient.getOrCreate(context)
    }
    private fun permission(type:KClass<out Record>)=HealthPermission.getReadPermission(type)
    private fun supports(client:HealthConnectClient,feature:Int)=
        client.features.getFeatureStatus(feature)==HealthConnectFeatures.FEATURE_STATUS_AVAILABLE
    @JvmStatic fun retrieve(activity:MainActivity) {
        request(activity,false)
    }
    @JvmStatic fun permissions(activity:MainActivity) {
        request(activity,true)
    }
    private fun request(activity:MainActivity,askOptional:Boolean) {
        scope.launch {
            try {
                val c=client(activity);val granted=c.permissionController.getGrantedPermissions()
                val wanted=types.map(::permission).toMutableSet()
                if(supports(c,HealthConnectFeatures.FEATURE_READ_HEALTH_DATA_IN_BACKGROUND))wanted.add(background)
                if(supports(c,HealthConnectFeatures.FEATURE_READ_HEALTH_DATA_HISTORY))wanted.add(history)
                // Exercise access is required. Other access remains optional;
                // the Permissions button lets the user revisit partial grants.
                if(askOptional||permission(types[0]) !in granted) {
                    withContext(Dispatchers.Main) {
                        waiting=activity
                        activity.permHealth.permissionsLauncher.launch(wanted+granted)
                    }
                } else { sync(activity,true,false);schedule(activity,c,granted) }
            } catch(e:CancellationException){throw e}
              catch(e:Exception){message=e.message?:e.toString();Log.stack("ActivityHealth",message,e)}
        }
    }
    @JvmStatic fun permissionResult():Boolean {
        val activity=waiting?:return false;waiting=null
        // Newly granted history access also needs a fresh initial read.
        try {ActivityNative.resetTokens()}catch(e:Exception){message=e.message?:e.toString();return true}
        scope.launch {
            sync(activity,true,false);settingsChanged(activity)
        }
        return true
    }
    @JvmStatic fun settingsChanged(context:Context) {
        if(!ready())return
        scope.launch {
            try {val c=client(context);schedule(context,c,c.permissionController.getGrantedPermissions())}
            catch(e:Exception){message=e.message?:e.toString()}
        }
        armTimer(context.applicationContext)
    }
    @JvmStatic fun onResume(activity:MainActivity) {
        if(!ready())return
        visible=true
        settingsChanged(activity)
        scope.launch {sync(activity,false,false)}
        numberEditorClosed()
    }
    @JvmStatic fun onPause() {visible=false}
    @JvmStatic fun numberEditorClosed() {
        handler.post {
            if(!ready())return@post
            if(Applic.app?.curve?.numberview?.newnumview!=null)return@post
            try {ActivityNative.linkAmounts();Applic.app?.curve?.requestRender()}
            catch(e:Exception){message=e.message?:e.toString()}
        }
    }
    private fun armTimer(context:Context) {
        handler.post {
            periodic?.let{handler.removeCallbacks(it)}
            val interval=try {ActivityNative.config()[1]}catch(e:Exception){message=e.message?:e.toString();return@post}
            if(interval==0L)return@post
            periodic=Runnable {
                scope.launch {sync(context,false,!visible)}
                armTimer(context)
            }.also{handler.postDelayed(it,interval*1000)}
        }
    }
    private fun schedule(context:Context,c:HealthConnectClient,granted:Set<String>) {
        val scheduler=context.getSystemService(JobScheduler::class.java)?:return
        val interval=ActivityNative.config()[1]
        if(interval==0L||background !in granted||!supports(c,HealthConnectFeatures.FEATURE_READ_HEALTH_DATA_IN_BACKGROUND)) {
            scheduler.cancel(jobId);return
        }
        val wanted=interval*1000
        if(scheduler.getPendingJob(jobId)?.intervalMillis==wanted)return
        scheduler.schedule(JobInfo.Builder(jobId,ComponentName(context,ActivityImportJob::class.java))
            .setPeriodic(wanted).setPersisted(true).build())
    }
    private class Expired:Exception()
    internal suspend fun sync(context:Context,force:Boolean,backgroundRun:Boolean):Boolean {
        if(!ready()||!running.compareAndSet(false,true))return true
        try {
            val cfg=ActivityNative.config();val now=Instant.now()
            if(!force&&(cfg[1]==0L||now.epochSecond-cfg[2]<cfg[1]))return true
            val c=client(context);val granted=c.permissionController.getGrantedPermissions()
            val mask=types.withIndex().fold(if(history in granted)1 shl 16 else 0) {bits,(index,type)->
                if(permission(type) in granted)bits or (1 shl index) else bits
            }
            withContext(Dispatchers.Main){ActivityNative.permissions(mask)}
            if(permission(types[0]) !in granted) {
                if(force)message="Grant permission to read exercise sessions"
                return true
            }
            if(backgroundRun&&background !in granted)return true
            message=context.getString(R.string.activity_retrieving)
            if(cfg[0]==0L)withContext(Dispatchers.Main) {
                val current=ActivityNative.config()
                if(current[0]==0L)ActivityNative.configure(now.minusSeconds(7*86400).epochSecond,current[1].toInt(),current[3].toInt())
            }
            var success=false
            for(attempt in 0..1) {
                try {import(c,granted,now);success=true;break}
                catch(e:Expired){ActivityNative.resetTokens();if(attempt==1)throw e}
            }
            if(success) {
                message=context.getString(R.string.activity_retrieved)
                withContext(Dispatchers.Main){numberEditorClosed()}
            }
            return success
        } catch(e:CancellationException){throw e}
          catch(e:Exception){message=e.message?:e.toString();Log.stack("ActivityHealth",message,e);return false}
        finally {running.set(false)}
    }
    private suspend fun import(c:HealthConnectClient,granted:Set<String>,now:Instant) {
        val config=ActivityNative.config()
        val from=Instant.ofEpochSecond(config[0])
        val next=mutableMapOf<Int,String>()
        // Tokens are obtained before the initial read, including metric tokens.
        for((index,type) in types.withIndex())if(permission(type) in granted) {
            var metricsChanged=false
            val old=ActivityNative.token(index)
            if(old.isEmpty()) {
                next[index]=c.getChangesToken(ChangesTokenRequest(setOf(type)))
                if(index==0) {
                    val sessions=read<ExerciseSessionRecord>(c,from,now)
                    ActivityNative.beginSessionSnapshot()
                    for(r in sessions)upsert(r,from,now)
                } else metricsChanged=true
            } else {
                var token=old
                do {
                    val changes=c.getChanges(token)
                    if(changes.changesTokenExpired)throw Expired()
                    for(change in changes.changes)when(change) {
                        is UpsertionChange -> {
                            val r=change.record
                            if(r is ExerciseSessionRecord)upsert(r,from,now) else metricsChanged=true
                        }
                        is DeletionChange -> if(index==0)ActivityNative.deleted(change.recordId)
                            else metricsChanged=true // Deletions contain only the HC ID.
                    }
                    token=changes.nextChangesToken
                } while(changes.hasMore)
                next[index]=token
            }
            withContext(Dispatchers.Main) {
                if(ActivityNative.config()[4]!=config[4])throw IllegalStateException("Retrieval settings changed; retrieve again")
                // Dirty flags precede the pending cursor. On a quota error or
                // process restart, already completed details need not repeat.
                ActivityNative.checkpoint(index,next.getValue(index),metricsChanged)
            }
        }
        for(index in ActivityNative.indices(true))details(c,granted,index)
        withContext(Dispatchers.Main) {
            // An open Amount editor retains a pointer into the number array.
            // Defer insertion while it is open; close/resume completes linking.
            if(Applic.app?.curve?.numberview?.newnumview==null)ActivityNative.linkAmounts()
        }
        // Dirty records and associations are handled before the cursor advances.
        withContext(Dispatchers.Main) {
            // Configuration also runs on Main. A changed start date during the
            // read must not acquire a cursor from the previous date range.
            if(ActivityNative.config()[4]!=config[4])throw IllegalStateException("Retrieval settings changed; retrieve again")
            for((index,token) in next)ActivityNative.setToken(index,token)
            ActivityNative.succeeded(now.epochSecond)
        }
    }
    private fun upsert(r:ExerciseSessionRecord,from:Instant,now:Instant) {
        if(r.startTime<from||r.endTime>now)return
        ActivityNative.upsert(r.metadata.id,r.metadata.dataOrigin.packageName,r.title?:"",r.notes?:"",
            r.startTime.epochSecond,r.endTime.epochSecond,r.exerciseType,r.metadata.lastModifiedTime.toEpochMilli(),source(r))
    }
    private fun source(record:Record):Int {
        val d=record.metadata.device
        val device=if(d==null)"" else listOfNotNull(d.manufacturer,d.model).joinToString(" ")+" [${d.type}]"
        return ActivityNative.source(record.metadata.dataOrigin.packageName,device)
    }
    private data class Point(val time:Instant,val value:Float,val source:Int)
    private suspend inline fun <reified T:Record> read(c:HealthConnectClient,from:Instant,to:Instant,origin:String?=null):List<T> {
        val result=ArrayList<T>();var page:String?=null
        do {
            currentCoroutineContext().ensureActive()
            val response=c.readRecords(ReadRecordsRequest(T::class,TimeRangeFilter.between(from,to),
                dataOriginFilter=origin?.let{setOf(DataOrigin(it))}?:emptySet(),pageToken=page))
            result.addAll(response.records);page=response.pageToken
        } while(!page.isNullOrEmpty())
        return result
    }
    private suspend fun details(c:HealthConnectClient,granted:Set<String>,index:Int) {
        val r=ActivityNative.record(index)?:return
        val text=ActivityNative.texts(index)?:return
        val start=Instant.ofEpochSecond(r[3].toLong());val end=Instant.ofEpochSecond(r[4].toLong());val origin=text[1]
        val values=r.copyOfRange(6,18);values[0]=(end.epochSecond-start.epochSecond).toDouble()
        val metrics=mutableSetOf<AggregateMetric<*>>()
        fun has(type:KClass<out Record>)=permission(type) in granted
        if(has(DistanceRecord::class))metrics.add(DistanceRecord.DISTANCE_TOTAL)
        if(has(ActiveCaloriesBurnedRecord::class))metrics.add(ActiveCaloriesBurnedRecord.ACTIVE_CALORIES_TOTAL)
        if(has(StepsRecord::class))metrics.add(StepsRecord.COUNT_TOTAL)
        if(has(ElevationGainedRecord::class))metrics.add(ElevationGainedRecord.ELEVATION_GAINED_TOTAL)
        if(metrics.isNotEmpty()) {
            val a=c.aggregate(AggregateRequest(metrics,TimeRangeFilter.between(start,end),setOf(DataOrigin(origin))))
            if(has(DistanceRecord::class))values[1]=a[DistanceRecord.DISTANCE_TOTAL]?.inMeters?:Double.NaN
            if(has(ActiveCaloriesBurnedRecord::class))values[6]=a[ActiveCaloriesBurnedRecord.ACTIVE_CALORIES_TOTAL]?.inKilocalories?:Double.NaN
            if(has(StepsRecord::class))values[8]=a[StepsRecord.COUNT_TOTAL]?.toDouble()?:Double.NaN
            if(has(ElevationGainedRecord::class))values[9]=a[ElevationGainedRecord.ELEVATION_GAINED_TOTAL]?.inMeters?:Double.NaN
        }
        ActivityNative.metrics(index,values)
        fun samples(kind:Int,points:List<Point>) {
            val selected=if(kind==4)points else points.filter{it.time>=start&&it.time<end}
            ActivityNative.samples(index,kind,selected.map{it.time.toEpochMilli()}.toLongArray(),
                selected.map{it.value}.toFloatArray(),selected.map{it.source}.toIntArray())
        }
        if(has(HeartRateRecord::class))samples(0,read<HeartRateRecord>(c,start,end,origin).flatMap{record->val src=source(record);record.samples.map{Point(it.time,it.beatsPerMinute.toFloat(),src)}})
        if(has(SpeedRecord::class))samples(1,read<SpeedRecord>(c,start,end,origin).flatMap{record->val src=source(record);record.samples.map{Point(it.time,it.speed.inMetersPerSecond.toFloat(),src)}})
        if(has(PowerRecord::class))samples(2,read<PowerRecord>(c,start,end,origin).flatMap{record->val src=source(record);record.samples.map{Point(it.time,it.power.inWatts.toFloat(),src)}})
        if(has(CyclingPedalingCadenceRecord::class))samples(3,read<CyclingPedalingCadenceRecord>(c,start,end,origin).flatMap{record->val src=source(record);record.samples.map{Point(it.time,it.revolutionsPerMinute.toFloat(),src)}})
        if(has(RestingHeartRateRecord::class))samples(4,read<RestingHeartRateRecord>(c,start.minusSeconds(7*86400),start.plusSeconds(1),origin).map{Point(it.time,it.beatsPerMinute.toFloat(),source(it))})
        ActivityNative.complete(index)
    }
}

class ActivityImportJob:JobService() {
    private var task:Job?=null
    override fun onStartJob(params:JobParameters):Boolean {
        task=CoroutineScope(Dispatchers.Main+SupervisorJob()).launch {
            val ok=withContext(Dispatchers.IO){ActivityHealth.sync(this@ActivityImportJob,false,true)}
            jobFinished(params,!ok)
        }
        return true
    }
    override fun onStopJob(params:JobParameters):Boolean {task?.cancel();return true}
}
