/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2(+), Libre 3(+), Dexcom G7/ONE+,        */
/*      Sibionics GS1Sb and GS3, Accu-Chek SmartGuide, CareSens Air and              */
/*      Aidex X sensors.                                                             */
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
/*      Thu Oct 15 12:44:39 CEST 2026                                                */

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

/** Android API adapter. Native code owns saved activities and Amounts. */
object ActivityHealth {
    private const val tag="ActivityHealth"
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
    @Volatile private var historyMessage=""
    @Volatile private var visible=false
    @Volatile private var waiting:MainActivity?=null
    @Volatile private var waitingForRetrieval=false
    private var periodic:Runnable?=null

    @JvmStatic fun status():String=if(ready()&&Natives.staticnum())
        Applic.app?.getString(R.string.activity_mirrored)?:"Activities are received with mirrored Amounts. Health Connect retrieval is disabled."
        else listOf(message,historyMessage).filter{it.isNotEmpty()}.joinToString("\n")
    @JvmStatic fun available():Boolean=Build.VERSION.SDK_INT>=28
    private fun ready()=available()&&Applic.Nativesloaded&&Applic.initproccalled&&Applic.stopprogram==0
    @JvmStatic fun typeCodes():IntArray=ActivityTypes.codes().also {Log.i(tag,"Exercise types: ${it.size}")}
    @JvmStatic fun typeName(type:Int):String=ActivityTypes.name(type)

    private fun failed(operation:String,e:Exception) {
        message=e.message?:e.toString()
        Log.stack(tag,operation,e)
    }

    private fun client(context:Context):HealthConnectClient {
        check(available()&&HealthConnectClient.getSdkStatus(context)==HealthConnectClient.SDK_AVAILABLE) {
            "Health Connect is unavailable or needs updating"
        }
        return HealthConnectClient.getOrCreate(context)
    }
    private fun permission(type:KClass<out Record>)=HealthPermission.getReadPermission(type)
    private fun supports(client:HealthConnectClient,feature:Int)=
        client.features.getFeatureStatus(feature)==HealthConnectFeatures.FEATURE_STATUS_AVAILABLE
    private fun historyAccess(context:Context,c:HealthConnectClient,granted:Set<String>):Boolean {
        val supported=supports(c,HealthConnectFeatures.FEATURE_READ_HEALTH_DATA_HISTORY)
        historyMessage=context.getString(when {
            history in granted -> R.string.activity_history_granted
            supported -> R.string.activity_history_missing
            else -> R.string.activity_history_unavailable
        })
        Log.i(tag,"History access: supported=$supported, granted=${history in granted}")
        return supported
    }
    @JvmStatic fun retrieve(activity:MainActivity) {
        request(activity,false)
    }
    @JvmStatic fun permissions(activity:MainActivity) {
        request(activity,true)
    }
    @JvmStatic fun saved(activity:MainActivity) {
        request(activity,false,false)
    }
    private fun request(activity:MainActivity,askOptional:Boolean,retrieve:Boolean=true) {
        if(!ready()||Natives.staticnum())return
        Log.i(tag,"Request activities: permissions=$askOptional")
        scope.launch {
            try {
                if(Natives.staticnum())return@launch
                val c=client(activity);val granted=c.permissionController.getGrantedPermissions()
                val historySupported=historyAccess(activity,c,granted)
                val config=ActivityNative.config();val from=config[0]
                // Health Connect limits history to 30 days before the first
                // permission grant, not 30 days before each retrieval. Its grant
                // date is not exposed here; an older selected date can need history.
                val needsHistory=historySupported&&history !in granted&&from>0&&
                    from<Instant.now().minusSeconds(30*86400).epochSecond
                val wanted=types.map(::permission).toMutableSet()
                if((askOptional||config[1]>0)&&supports(c,HealthConnectFeatures.FEATURE_READ_HEALTH_DATA_IN_BACKGROUND))wanted.add(background)
                if(askOptional&&historySupported||needsHistory)wanted.add(history)
                // Ask on explicit Save/Retrieve when access is missing. Optional
                // measures can still be declined; sync uses the granted subset.
                if(askOptional||!granted.containsAll(wanted)) {
                    Log.i(tag,"Request permissions: wanted=${wanted.size}, granted=${granted.size}, from=$from, needsHistory=$needsHistory")
                    withContext(Dispatchers.Main) {
                        if(Natives.staticnum())return@withContext
                        if(waiting!=null){waitingForRetrieval=waitingForRetrieval||retrieve;return@withContext}
                        waitingForRetrieval=retrieve
                        waiting=activity
                        try {activity.permHealth.permissionsLauncher.launch(wanted+granted)}
                        catch(e:Exception){waiting=null;waitingForRetrieval=false;throw e}
                    }
                } else { if(retrieve)sync(activity,true,false);schedule(activity,c,granted) }
            } catch(e:CancellationException){throw e}
              catch(e:Exception){failed("Request activities: permissions=$askOptional",e)}
        }
    }
    @JvmStatic fun permissionResult():Boolean {
        val activity=waiting?:return false;waiting=null
        val retrieve=waitingForRetrieval;waitingForRetrieval=false
        Log.i(tag,"Activity permission result")
        // sync() resets cursors when the granted permission mask changes.
        // Closing an unchanged permission dialog must preserve import progress.
        scope.launch {
            if(retrieve)sync(activity,true,false)
            settingsChanged(activity)
        }
        return true
    }
    @JvmStatic fun settingsChanged(context:Context) {
        if(!ready())return
        if(Natives.staticnum()) {
            context.getSystemService(JobScheduler::class.java)?.cancel(jobId)
            handler.post {periodic?.let{handler.removeCallbacks(it)};periodic=null}
            return
        }
        scope.launch {
            try {
                if(Natives.staticnum())return@launch
                val c=client(context);val granted=c.permissionController.getGrantedPermissions()
                historyAccess(context,c,granted)
                schedule(context,c,granted)
            }
            catch(e:CancellationException){throw e}
            catch(e:Exception){failed("Schedule activity retrieval",e)}
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
            catch(e:Exception){failed("Link activity Amounts after editor closes",e)}
        }
    }
    private fun armTimer(context:Context) {
        handler.post {
            periodic?.let{handler.removeCallbacks(it)}
            if(!ready()||Natives.staticnum()){periodic=null;return@post}
            val interval=try {ActivityNative.config()[1]}catch(e:Exception){failed("Read activity timer settings",e);return@post}
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
        if(Natives.staticnum()||interval==0L||background !in granted||!supports(c,HealthConnectFeatures.FEATURE_READ_HEALTH_DATA_IN_BACKGROUND)) {
            scheduler.cancel(jobId);return
        }
        val wanted=interval*1000
        if(scheduler.getPendingJob(jobId)?.intervalMillis==wanted)return
        val result=scheduler.schedule(JobInfo.Builder(jobId,ComponentName(context,ActivityImportJob::class.java))
            .setPeriodic(wanted).setPersisted(true).build())
        Log.i(tag,"Schedule activity job: interval=$interval seconds, result=$result")
        if(result==JobScheduler.RESULT_FAILURE)Log.e(tag,"Android rejected the activity retrieval job")
    }
    private class Expired:Exception()
    internal suspend fun sync(context:Context,force:Boolean,backgroundRun:Boolean):Boolean {
        if(!ready()||Natives.staticnum()||!running.compareAndSet(false,true))return true
        try {
            val cfg=ActivityNative.config();val now=Instant.now()
            if(!force&&(cfg[1]==0L||now.epochSecond-cfg[2]<cfg[1]))return true
            Log.i(tag,"Retrieve activities: force=$force, background=$backgroundRun, from=${cfg[0]}, generation=${cfg[4]}")
            val c=client(context);val granted=c.permissionController.getGrantedPermissions()
            historyAccess(context,c,granted)
            val mask=types.withIndex().fold(if(history in granted)1 shl 16 else 0) {bits,(index,type)->
                if(permission(type) in granted)bits or (1 shl index) else bits
            }
            withContext(Dispatchers.Main){ActivityNative.permissions(mask)}
            if(permission(types[0]) !in granted) {
                Log.i(tag,"Activity retrieval skipped: exercise permission missing")
                if(force)message="Grant permission to read exercise sessions"
                return true
            }
            if(backgroundRun&&background !in granted) {
                Log.i(tag,"Activity retrieval skipped: background permission missing")
                return true
            }
            message=context.getString(R.string.activity_retrieving)
            if(cfg[0]==0L)withContext(Dispatchers.Main) {
                val current=ActivityNative.config()
                if(current[0]==0L)ActivityNative.configure(now.minusSeconds(7*86400).epochSecond,current[1].toInt(),current[3].toInt())
            }
            var success=false
            for(attempt in 0..1) {
                try {import(context,c,granted,now);success=true;break}
                catch(e:Expired){Log.i(tag,"Activity changes cursor expired: attempt=${attempt+1}");ActivityNative.resetTokens();if(attempt==1)throw e}
            }
            if(success) {
                message=context.getString(R.string.activity_retrieved)
                Log.i(tag,"Activity retrieval completed")
                withContext(Dispatchers.Main){numberEditorClosed()}
            }
            return success
        } catch(e:CancellationException){throw e}
          catch(e:Exception){failed("Retrieve activities: force=$force, background=$backgroundRun",e);return false}
        finally {running.set(false)}
    }
    private class ReadSession(val client:HealthConnectClient,private val context:Context,generation:Long) {
        private var progress=context.getString(R.string.activity_retrieving)
        val retry=ActivityReadRetry(
            // Use the available quota at full speed; wait only after a rejection.
            isQuota={error->ActivityReadRetry.isQuota(error) {cause->
                if(Build.VERSION.SDK_INT>=34&&cause is android.health.connect.HealthConnectException)
                    cause.errorCode==android.health.connect.HealthConnectException.ERROR_RATE_LIMIT_EXCEEDED
                else null
            }},
            beforeAttempt={
                check(ready()&&!Natives.staticnum()) {"Activity retrieval stopped"}
                check(ActivityNative.config()[4]==generation) {"Retrieval settings changed; retrieve again"}
            },
            waiting={operation,millis->
                message=progress+"\n"+context.getString(R.string.activity_quota_wait)
                Log.i(tag,"Health Connect quota: $operation; retry in ${millis/1000} seconds")
            },
            resumed={message=progress;Log.i(tag,"Health Connect quota retry succeeded")}
        )
        fun progress(done:Int,total:Int) {
            progress=context.getString(R.string.activity_retrieving_details,done,total)
            message=progress
        }
    }
    private suspend fun import(context:Context,c:HealthConnectClient,granted:Set<String>,now:Instant) {
        val config=ActivityNative.config()
        val from=Instant.ofEpochSecond(config[0])
        val reads=ReadSession(c,context,config[4])
        val next=mutableMapOf<Int,String>()
        val intervals=ActivityChangeIndex(context,config[4])
        val updates=ActivityRecordChanges(intervals) {kind,range->
            val marked=ActivityNative.markDetails(kind,range.start,range.end,range.origin)
            Log.i(tag,"Changed ${types[kind].java.simpleName}: ${Instant.ofEpochMilli(range.start)} – ${Instant.ofEpochMilli(range.end)}, origin=${range.origin}, affected=$marked")
        }
        try {
            // Tokens are obtained before the initial read, including metric tokens.
            for((index,type) in types.withIndex())if(permission(type) in granted) {
                val pendingToken=ActivityNative.pendingToken(index)
                if(pendingToken.isNotEmpty()) {
                    // Finish this saved batch first. Fetching newer metric changes
                    // now could mark every completed activity dirty all over again.
                    next[index]=pendingToken
                    Log.i(tag,"Resume ${type.java.simpleName}: saved changes, details pending")
                    continue
                }
                var metricsChanged=false
                val old=ActivityNative.token(index)
                Log.i(tag,"Read ${type.java.simpleName}: ${if(old.isEmpty()) "initial read" else "changes"}")
                if(old.isEmpty()) {
                    next[index]=reads.retry.call("Get ${type.java.simpleName} changes token") {
                        c.getChangesToken(ChangesTokenRequest(setOf(type)))
                    }
                    if(index==0) {
                        val sessions=read<ExerciseSessionRecord>(reads,from,now)
                        ActivityNative.beginSessionSnapshot()
                        for(r in sessions)upsert(r,from,now)
                    } else {
                        indexMetrics(reads,intervals,index,type,from,now)
                        metricsChanged=true
                    }
                } else {
                    if(index!=0&&!intervals.indexed(index)) {
                        // One upgrade/bootstrap read records the IDs needed to
                        // locate later corrections/deletions. Existing cached
                        // details are refreshed once when this lookup is missing.
                        indexMetrics(reads,intervals,index,type,from,now)
                        metricsChanged=true
                    }
                    var token=old
                    do {
                        val changes=reads.retry.call("Get ${type.java.simpleName} changes") {c.getChanges(token)}
                        if(changes.changesTokenExpired) {
                            Log.i(tag,"Expired cursor: ${type.java.simpleName}")
                            throw Expired()
                        }
                        Log.i(tag,"${type.java.simpleName}: ${changes.changes.size} changes, more=${changes.hasMore}")
                        for(change in changes.changes)when(change) {
                            is UpsertionChange -> {
                                val r=change.record
                                if(r is ExerciseSessionRecord)upsert(r,from,Instant.now())
                                else updates.upsert(index,r.metadata.id,metricRange(r))
                            }
                            is DeletionChange -> if(index==0)ActivityNative.deleted(change.recordId)
                                else if(!updates.deleted(index,change.recordId)) {
                                    // Deletion events omit the timestamp and app.
                                    // With a complete lookup, an untracked ID has no
                                    // indexed contribution to the saved activities.
                                    // Initial/bootstrap reads already refresh details.
                                    if(metricsChanged)Log.i(tag,"Deleted ${type.java.simpleName}: unknown ID; refresh saved details")
                                    else Log.i(tag,"Deleted ${type.java.simpleName}: unknown ID; no indexed activity to refresh")
                                }
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
                // Native imports can be suppressed after switching to mirrored Amounts.
                if(ActivityNative.pendingToken(index)==next.getValue(index))updates.checkpoint(index)
            }
            val pending=ActivityNative.indices(true).asIterable().mapNotNull {index->
                val record=ActivityNative.record(index)?:return@mapNotNull null
                val text=ActivityNative.texts(index)?:return@mapNotNull null
                DetailTask(index,record,text)
            }.sortedByDescending{it.start}
            Log.i(tag,"Read activity details: ${pending.size} pending records")
            if(pending.isNotEmpty()) {
                val raw=DetailReads(reads,pending.last().start,pending.map{DataOrigin(it.origin)}.toSet())
                val keepEnds=Array(pending.size){Instant.MIN}
                for(position in pending.indices.reversed())
                    keepEnds[position]=maxOf(pending[position].end,keepEnds.getOrNull(position+1)?:Instant.MIN)
                for((position,task) in pending.withIndex()) {
                    reads.progress(position+1,pending.size)
                    details(reads,raw,granted,task,keepEnds[position])
                }
            }
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
        } finally {intervals.close()}
    }
    private fun metricRange(record:Record):ActivityRecordRange {
        val origin=record.metadata.dataOrigin.packageName
        fun interval(start:Instant,end:Instant):ActivityRecordRange {
            val begin=start.toEpochMilli()
            return ActivityRecordRange(begin,maxOf(begin+1,end.toEpochMilli()),origin)
        }
        return when(record) {
            is HeartRateRecord -> interval(record.startTime,record.endTime)
            is RestingHeartRateRecord -> record.time.toEpochMilli().let {ActivityRecordRange(it,it+1,origin)}
            is DistanceRecord -> interval(record.startTime,record.endTime)
            is ActiveCaloriesBurnedRecord -> interval(record.startTime,record.endTime)
            is SpeedRecord -> interval(record.startTime,record.endTime)
            is StepsRecord -> interval(record.startTime,record.endTime)
            is ElevationGainedRecord -> interval(record.startTime,record.endTime)
            is PowerRecord -> interval(record.startTime,record.endTime)
            is CyclingPedalingCadenceRecord -> interval(record.startTime,record.endTime)
            else -> throw IllegalArgumentException("Unsupported activity metric: ${record.javaClass.simpleName}")
        }
    }
    private suspend fun indexMetrics(reads:ReadSession,intervals:ActivityChangeIndex,kind:Int,
        type:KClass<out Record>,from:Instant,to:Instant) {
        val begin=from.minusSeconds(if(kind==2)7*86400L else 86400L)
        // Publish the refresh before completing the lookup. If the process
        // stops between SQLite completion and the native checkpoint, the old
        // activities still need their bootstrap refresh when retrieval resumes.
        ActivityNative.markAllDetails()
        val scan=intervals.beginSnapshot(kind)
        var page:String?=null
        var count=0
        Log.i(tag,"Index ${type.java.simpleName}: from=$begin, to=$to")
        do {
            val response=reads.retry.call("Index ${type.java.simpleName}") {
                reads.client.readRecords(ReadRecordsRequest(type,TimeRangeFilter.between(begin,to),pageSize=5000,pageToken=page))
            }
            intervals.snapshotPage(kind,scan,response.records.map {it.metadata.id to metricRange(it)})
            count+=response.records.size;page=response.pageToken
        } while(page!=null)
        intervals.finishSnapshot(kind,scan)
        Log.i(tag,"Indexed ${type.java.simpleName}: $count records")
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
    private class DetailTask(val index:Int,val record:DoubleArray,val text:Array<String>) {
        val start=Instant.ofEpochSecond(record[3].toLong())
        val end=Instant.ofEpochSecond(record[4].toLong())
        val origin=text[1]
    }
    private class DetailReads(reads:ReadSession,from:Instant,origins:Set<DataOrigin>) {
        val heart=pages<HeartRateRecord>(reads,from.minusSeconds(86400)){it.startTime}
        val resting=pages<RestingHeartRateRecord>(reads,from.minusSeconds(7*86400)){it.time}
        val speed=pages<SpeedRecord>(reads,from,origins){it.startTime}
        val power=pages<PowerRecord>(reads,from,origins){it.startTime}
        val cadence=pages<CyclingPedalingCadenceRecord>(reads,from,origins){it.startTime}
    }
    private inline fun <reified T:Record> pages(reads:ReadSession,from:Instant,origins:Set<DataOrigin> = emptySet(),
                                              noinline time:(T)->Instant):ActivityReadPages<T> =
        ActivityReadPages(time) {page,to->
            if(page==null)Log.i(tag,"Read ${T::class.java.simpleName} shared pages: from=$from, to=$to, origins=${origins.map{it.packageName}.ifEmpty{listOf("all")}}")
            val request=ReadRecordsRequest(T::class,TimeRangeFilter.between(from,to),
                dataOriginFilter=origins,ascendingOrder=false,pageSize=5000,pageToken=page)
            val response=reads.retry.call("Read ${T::class.java.simpleName} page") {reads.client.readRecords(request)}
            Log.i(tag,"Read ${T::class.java.simpleName} page: ${response.records.size} records, more=${!response.pageToken.isNullOrEmpty()}")
            ActivityReadPages.Page(response.records,response.pageToken)
        }
    private suspend inline fun <reified T:Record> read(reads:ReadSession,from:Instant,to:Instant,origin:String?=null):List<T> {
        val result=ArrayList<T>();var page:String?=null
        Log.i(tag,"Read ${T::class.java.simpleName} samples/records: from=$from, to=$to, origin=${origin?:"all"}")
        do {
            currentCoroutineContext().ensureActive()
            val request=ReadRecordsRequest(T::class,TimeRangeFilter.between(from,to),
                dataOriginFilter=origin?.let{setOf(DataOrigin(it))}?:emptySet(),pageSize=5000,pageToken=page)
            val response=reads.retry.call("Read ${T::class.java.simpleName} page") {reads.client.readRecords(request)}
            result.addAll(response.records);page=response.pageToken
        } while(!page.isNullOrEmpty())
        Log.i(tag,"Read ${T::class.java.simpleName}: ${result.size} records")
        if(T::class==ExerciseSessionRecord::class) {
            val sessions=result.filterIsInstance<ExerciseSessionRecord>()
            Log.i(tag,"Exercise session range: first=${sessions.minOfOrNull{it.startTime}}, last=${sessions.maxOfOrNull{it.endTime}}")
        }
        return result
    }
    private suspend fun details(reads:ReadSession,raw:DetailReads,granted:Set<String>,task:DetailTask,keepTo:Instant) {
        val index=task.index
        Log.i(tag,"Activity details: index=$index")
        val r=task.record
        val start=task.start;val end=task.end;val origin=task.origin
        val values=r.copyOfRange(6,18);values[0]=(end.epochSecond-start.epochSecond).toDouble()
        val metrics=mutableSetOf<AggregateMetric<*>>()
        fun has(type:KClass<out Record>)=permission(type) in granted
        if(has(DistanceRecord::class))metrics.add(DistanceRecord.DISTANCE_TOTAL)
        if(has(ActiveCaloriesBurnedRecord::class))metrics.add(ActiveCaloriesBurnedRecord.ACTIVE_CALORIES_TOTAL)
        if(has(StepsRecord::class))metrics.add(StepsRecord.COUNT_TOTAL)
        if(has(ElevationGainedRecord::class))metrics.add(ElevationGainedRecord.ELEVATION_GAINED_TOTAL)
        if(metrics.isNotEmpty()) {
            Log.i(tag,"Aggregate activity index=$index: ${metrics.size} metrics, origin=$origin")
            val a=reads.retry.call("Aggregate activity $index") {
                reads.client.aggregate(AggregateRequest(metrics,TimeRangeFilter.between(start,end),setOf(DataOrigin(origin))))
            }
            if(has(DistanceRecord::class))values[1]=a[DistanceRecord.DISTANCE_TOTAL]?.inMeters?:Double.NaN
            if(has(ActiveCaloriesBurnedRecord::class))values[6]=a[ActiveCaloriesBurnedRecord.ACTIVE_CALORIES_TOTAL]?.inKilocalories?:Double.NaN
            if(has(StepsRecord::class))values[8]=a[StepsRecord.COUNT_TOTAL]?.toDouble()?:Double.NaN
            if(has(ElevationGainedRecord::class))values[9]=a[ElevationGainedRecord.ELEVATION_GAINED_TOTAL]?.inMeters?:Double.NaN
        }
        ActivityNative.metrics(index,values)
        fun samples(kind:Int,points:List<Point>) {
            val selected=if(kind==4)points else points.filter{it.time>=start&&it.time<end}
            if(kind==0||kind==4)for((src,count) in selected.groupingBy{it.source}.eachCount())
                Log.i(tag,"Activity ${if(kind==0) "heart rate" else "resting heart rate"}: index=$index, source=${ActivityNative.sourceName(src)}, samples=$count")
            ActivityNative.samples(index,kind,selected.map{it.time.toEpochMilli()}.toLongArray(),
                selected.map{it.value}.toFloatArray(),selected.map{it.source}.toIntArray())
        }
        // A session and its watch measurements can be published by different
        // apps (for example, a Strava session with Samsung Health heart rate).
        // Preserve every origin/device; native summaries select a single source.
        // Raw interval queries filter by record start, not overlap. Include a
        // day's lead-in for earlier-starting workout/daily batches, then retain
        // only samples inside the activity in samples() above.
        if(has(HeartRateRecord::class))samples(0,raw.heart.read(start.minusSeconds(86400),end,keepTo)
            .filter{it.endTime>start}.flatMap{record->val src=source(record);record.samples.asSequence()
                .filter{it.time>=start&&it.time<end}.map{Point(it.time,it.beatsPerMinute.toFloat(),src)}.toList()})
        if(has(SpeedRecord::class))samples(1,raw.speed.read(start,end,keepTo).filter{it.metadata.dataOrigin.packageName==origin}
            .flatMap{record->val src=source(record);record.samples.map{Point(it.time,it.speed.inMetersPerSecond.toFloat(),src)}})
        if(has(PowerRecord::class))samples(2,raw.power.read(start,end,keepTo).filter{it.metadata.dataOrigin.packageName==origin}
            .flatMap{record->val src=source(record);record.samples.map{Point(it.time,it.power.inWatts.toFloat(),src)}})
        if(has(CyclingPedalingCadenceRecord::class))samples(3,raw.cadence.read(start,end,keepTo).filter{it.metadata.dataOrigin.packageName==origin}
            .flatMap{record->val src=source(record);record.samples.map{Point(it.time,it.revolutionsPerMinute.toFloat(),src)}})
        if(has(RestingHeartRateRecord::class))samples(4,raw.resting.read(start.minusSeconds(7*86400),start.plusSeconds(1))
            .map{Point(it.time,it.beatsPerMinute.toFloat(),source(it))})
        Log.i(tag,"Activity heart rate used: index=$index, source=${ActivityNative.sourceName(ActivityNative.heartSource(index))}")
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
