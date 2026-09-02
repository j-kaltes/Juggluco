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
/*      Fri Jan 27 15:31:05 CET 2023                                                 */


package tk.glucodata

//import androidx.activity.Context
//import androidx.lifecycle.Lifecycle
//import androidx.lifecycle.lifecycleScope
import android.content.Context
import androidx.annotation.Keep
import com.google.android.gms.tasks.Tasks
import com.google.android.gms.wearable.CapabilityClient
import com.google.android.gms.wearable.CapabilityInfo
import com.google.android.gms.wearable.Node
import com.google.android.gms.wearable.Wearable
import kotlinx.coroutines.*
import kotlinx.coroutines.tasks.await
import tk.glucodata.Applic.JUGGLUCOIDENT;
import tk.glucodata.Applic.isWearable
import tk.glucodata.Log.doLog
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.util.concurrent.CountDownLatch
import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicBoolean
import java.util.concurrent.atomic.AtomicLong

//import tk.glucodata.Applic.messagesender


class MessageSender(val activity: Context):CapabilityClient.OnCapabilityChangedListener {
    private val messageClient by lazy { Wearable.getMessageClient(activity) }
    private val capabilityClient by lazy { Wearable.getCapabilityClient(activity) }
    private val nodeClient by lazy { Wearable.getNodeClient(activity) }
    public val localnodeall by lazy { Tasks.await(nodeClient.localNode) }
    public val localnode by lazy { localnodeall.id }
    public val galaxywatch by lazy {
        isGalaxy(localnodeall) }

    @Volatile var nodes: Set<Node>? = null
    private val nexttimes=ConcurrentHashMap<String,Long>()

    private fun setnodes(ns:Set<Node>) {
        nodes = ns
        val reachable=ns.mapTo(HashSet<String>()) { it.id }
        nexttimes.keys.removeAll { !it.startsWith("ble:")&&it !in reachable }
        associateConfiguredMirrorNodes(ns)
        sendnetinfo();
    }

    private fun associateConfiguredMirrorNodes(ns:Set<Node>) {
        val reachable=ns.mapTo(HashSet<String>()) { it.id }
        mirrorNodes.forEach { (label,nodeId) ->
            if(nodeId !in reachable&&mirrorNodes[label]==nodeId)
                forgetMirrorNode(label,"node $nodeId is not reachable")
        }
        val companionNode=if(isWearable) ns.singleOrNull()?.id else null
        val localMirror=if(isWearable) {
            try {
                localnode
            }
            catch(th:Throwable) {
                Log.stack(LOG_ID,"Cannot resolve the local Wear node while associating mirrors",th)
                null
            }
        }
        else null
        Log.i(LOG_ID,"Associating Wear mirrors: role=${if(isWearable) "watch" else "phone"} local=$localMirror reachable=$reachable")
        val count=Natives.backuphostNr()
        for(index in 0 until count) {
            val label=Natives.getbackuplabel(index)?:continue
            if(Natives.getHostDeactivated(index)) {
                forgetMirrorNode(label,"mirror is deactivated")
                continue
            }
            if(!Natives.isWearOS(index))
                continue

            // A phone's mirror label is the Wear node ID of that particular
            // watch.  Never assign the sole currently reachable watch to all
            // Wear rows: with two watches that aliases their IPs and password.
            // A watch has exactly one valid companion row, named after its own
            // local node; only that row may map to the phone's different ID.
            val target=if(isWearable) {
                if(label==localMirror) companionNode else null
            }
            else {
                label.takeIf { it in reachable }
            }
            if(target!=null)
                rememberMirrorNode(label,target)
            else
                forgetMirrorNode(label,if(isWearable)
                    "not this watch's local mirror ($localMirror)"
                else
                    "no reachable Wear node with the same ID")
        }
    }
    public fun nulltimes() {
        nexttimes.clear()
    }
    private fun reserveNetInfo(nodeId:String,now:Long,force:Boolean=false):Long? = synchronized(nexttimes) {
        if(!force&&(nexttimes[nodeId]?:0L)>now)
            null
        else {
            val deadline=now+NETINFO_IN_FLIGHT_MILLIS
            nexttimes[nodeId]=deadline
            deadline
        }
    }
    private fun confirmNetInfo(nodeId:String,reservation:Long) {
        synchronized(nexttimes) {
            if(nexttimes[nodeId]==reservation)
                nexttimes[nodeId]=System.currentTimeMillis()+netwait
        }
    }
    private fun releaseNetInfo(nodeId:String,reservation:Long) {
        synchronized(nexttimes) {
            if(nexttimes[nodeId]==reservation)
                nexttimes.remove(nodeId)
        }
    }
private val nodesbusy=AtomicBoolean(false)
suspend fun findWearDevicesWithApp() {
    Log.i(LOG_ID,"start findWearDevicesWithApp nodesbusy=${nodesbusy.get()}")
    if(!nodesbusy.compareAndSet(false,true))
        return;
    try {
        val capabilityInfo = capabilityClient.getCapability( JUGGLUCOIDENT, CapabilityClient.FILTER_REACHABLE).await()
        setnodes(capabilityInfo.nodes)
        Log.d(LOG_ID, "Capable Nodes: $nodes")
        Natives.isGalaxyWatch(galaxywatch)
    } catch (cancellationException: CancellationException) {
        throw cancellationException
    } catch (th: Throwable) {
        Thread.currentThread().setName("Devices$findIter")
        ++findIter
        Log.stack(LOG_ID, "findDev",th)
    }
    finally {
        Log.i(LOG_ID,"end findWearDevicesWithApp nodesbusy=false")
        nodesbusy.set(false)
    }
    }

public fun finddevices() {
     val sender=this
     scope.launch {
      findWearDevicesWithApp()
      }
     Wearable.getCapabilityClient(activity).addListener(sender, JUGGLUCOIDENT)
     }

  init {
      finddevices()
      }

/*
    public fun startActivity() {
    val data=Natives.bytesettings()
      sendmessage(START_PATH, data) 
    } */

public fun startWearOSActivity(nodeName:String) {
    val data=Natives.bytesettings()
    nameSendMessage(nodeName,START_PATH,data)
    }
public fun toDefaults(node:Node) {
    toDefaults(node.id)
    }
public fun toDefaults(label:String) {
    val nodata:ByteArray=byteArrayOf(0)
    nameSendMessage(label,DEFAULTS_PATH,nodata)
    }
/*
private fun startnodedetection(context: Context):String? {
    Wearable.getCapabilityClient(context).addListener( this,JUGGLUCOIDENT )
    val capabilityInfo: CapabilityInfo = Tasks.await(capabilityClient.getCapability( JUGGLUCOIDENT, CapabilityClient.FILTER_REACHABLE))
    return pickBestNodeId(capabilityInfo.nodes)
}
*/
//private var transcriptionNodeId: String? = null



override fun onCapabilityChanged(cap: CapabilityInfo) {
        scope.launch {
            setnodes(cap.nodes)
        }
    }
    private fun sendmessage(path:String,data:ByteArray) {
            try {
        val bleTargets=when {
            path==WAKE_PATH||path==WAKESTREAM_PATH -> BleMirror.controlLabels().toSet()
            isWearControlPath(path) -> BleMirror.wearControlLabels().toSet()
            else -> emptySet()
        }
        bleTargets.forEach { nameSendMessage(it,path,data) }
        // A watch's BLE mirror label is its own ID, not the phone's Google ID.
        // Its authenticated Wear connection already reaches that companion.
        if(isWearable&&BleMirror.wearControlLabels().any { it in bleTargets })
            return
        when {
            nodes == null -> {
                Log.d(LOG_ID, "sendmessage nodes=null")
                scope.launch {
                findWearDevicesWithApp()
                }
            }
            nodes?.isEmpty() == true -> {
                Log.d(LOG_ID, "sendmessage nodes.isEmpty")
            }
            else -> {
                    nodes?.forEach { node ->
                        if(node.id !in bleTargets)
                            nameSendMessage(node.id,path,data)
                        }
                    Log.d(LOG_ID, "Starting requests sent successfully")
                }
            }
            } catch (exception: Exception) {
                Log.d(LOG_ID, "Starting activity failed: $exception")
        }
    }
private fun nameSendMessage(name:String, path:String, data:ByteArray) {
    scope.launch {
        Log.i(LOG_ID, "start sendNameMessage($name $path,... )")
        nameSendMessageResult(name,path,data)
        Log.i(LOG_ID,"after sendNameMessage($name $path,... )")
        }
    }
    private fun nameSendMessageResult(name:String, path:String, data:ByteArray):Boolean {
        if(path==NET_PATH) {
            if(BleMirror.canSendWearControl(name))
                return sendBleNetInfoResult(name,data)
            // Callers here supply a Wear node ID. A watch's BLE mirror instead
            // uses its own local node ID; do not reverse-map a stale preference.
            val mirrorName=if(isWearable) localnode else name
            if(BleMirror.canSendWearControl(mirrorName))
                return sendBleNetInfoResult(mirrorName,data)
            // MessageClient still bootstraps a missing or disconnected BLE peer.
            // Keep the original node ID for that path, never another row's map.
            Log.i(LOG_ID,"Sending Wear /netinfo through MessageClient: node=$name")
            return nameSendMessageResultDirect(name,path,data)
        }
        // Settings and commands do not depend on the selected glucose-data
        // carrier. A ready authenticated BLE session can deliver them even
        // while TCP/IP or MessageClient is selected for /data.
        if(isWearControlPath(path)||path==WAKE_PATH||path==WAKESTREAM_PATH) {
            val wearOnly=isWearControlPath(path)
            fun available(label:String)=if(wearOnly) BleMirror.canSendWearControl(label)
                else BleMirror.canSendControl(label)
            val bleLabel=if(available(name)) name
                else mirrorLabelForNode(name)?.takeIf { available(it) }
            if(bleLabel!=null)
                return BleMirror.send(bleLabel,path,data)
        }
        if(BleMirror.isPeer(name)) {
            if(BleMirror.isConfiguredPeer(name)) {
                if(!BleMirror.isReady(name))
                    Log.i(LOG_ID,"Bluetooth mirror carrier for $name is not ready; $path will be retried")
                return BleMirror.isReady(name)&&BleMirror.send(name,path,data)
            }
            if(BleMirror.isAutomaticWearPeer(name)) {
                // A successful Google Task only means Play services accepted the
                // message locally; it does not prove the remote Juggluco received
                // it. Keep an application-level /messagesack liveness check while
                // MessageClient is the selected Automatic carrier.
                val actualNode=mirrorNodes[name]?:name
                val delivered=nameSendMessageResultDirect(actualNode,path,data,
                    WEAR_DATA_TASK_SECONDS)
                if(!delivered&&path!=NET_PATH) {
                    wearMessageSelected.remove(name)
                    requestDirectBleFallback(name,"MessageClient failed for $path")
                }
                else if(delivered&&path==DATA_PATH) {
                    wearMessageSelected.add(name)
                    ensureAutomaticWearMessageLiveness(name,actualNode)
                }
                return delivered
            }
            Log.i(LOG_ID,"Automatic Bluetooth fallback for $name is in TCP/IP standby; "+
                    "not routing $path through MessageClient")
            return false
        }
        return nameSendMessageResultDirect(mirrorNodes[name]?:name,path,data)
    }

    /** Send through Google Play services even while direct Bluetooth is selected. */
    private fun nameSendMessageResultDirect(name:String, path:String, data:ByteArray,
                                             timeoutSeconds:Long=MESSAGE_TASK_SECONDS):Boolean {
        try {
            val res=Tasks.await(messageClient.sendMessage(name,path,data),timeoutSeconds,TimeUnit.SECONDS)
            Log.i(LOG_ID,"nameSendMessageResult "+res)
            return true
        }
        catch (th: Throwable) {
            if(th is InterruptedException) {
                Thread.currentThread().interrupt()
            }
            Log.stack(LOG_ID, th)
            return false
        }

    }

private fun nodeSendmessage(node:Node,path:String,data:ByteArray) {
    nameSendMessage(node.id,path,data);
    }
        
    public fun sendnetinfo(data:ByteArray) {
    sendmessage(NET_PATH,data);
        }
    public fun sendnetinfo( node:Node,data:ByteArray) {
    nodeSendmessage(node,NET_PATH,data);
        }
    public fun sendnetinfo( node:String,data:ByteArray) {
        nameSendMessage(node,NET_PATH,data);
        }

    /** Keep both parts of an explicit phone-to-watch handoff on one carrier. */
    public fun sendBluetoothHandoff(name:String,netinfo:ByteArray,on:Boolean) {
        scope.launch {
            val throughBle=BleMirror.canSendWearControl(name)
            val configured=if(throughBle) sendBleNetInfoResult(name,netinfo)
                else nameSendMessageResultDirect(name,NET_PATH,netinfo)
            if(!configured) {
                Log.e(LOG_ID,"Sensor handoff /netinfo send failed: mirror=$name")
                return@launch
            }
            val onbyte:Byte=if(on) 1 else 0
            val data=byteArrayOf(onbyte)
            val switched=if(throughBle) BleMirror.send(name,BLUETOOTH_PATH,data)
                else nameSendMessageResultDirect(name,BLUETOOTH_PATH,data)
            Log.i(LOG_ID,"Sensor handoff sent: mirror=$name watchBluetooth=$on "+
                    "carrier=${if(throughBle) "BLE" else "MessageClient"} success=$switched")
        }
    }

    private fun sendBleNetInfoResult(label:String,data:ByteArray?=null):Boolean {
        // Replies share a per-mirror throttle. Explicit settings changes always
        // send, even if a routine /netinfo exchange just completed.
        val key="ble:$label"
        val reservation=reserveNetInfo(key,System.currentTimeMillis(),data!=null)
        if(reservation==null) {
            Log.i(LOG_ID,"BLE /netinfo already sent or in flight: mirror=$label")
            return true
        }
        var sent=false
        try {
            if(!BleMirror.canSendWearControl(label))
                return false
            val info=data?:Natives.getmynetinfo(label,false,0,Applic.ALLGALAXY,0,false)
                ?:return false
            Log.i(LOG_ID,"Sending Wear /netinfo through BLE: mirror=$label")
            sent=BleMirror.send(label,NET_PATH,info)
            return sent
        }
        finally {
            if(sent) confirmNetInfo(key,reservation)
            else releaseNetInfo(key,reservation)
        }
    }
    /*
    public fun sendsettings() {
    val data=Natives.bytesettings()
    sendmessage(SETTINGS_PATH,data)
    } */
    public fun sendbluetooth( node:Node,on:Boolean) {
         sendbluetooth( node.id,on);
     }
    public fun sendbluetooth( name:String,on:Boolean) {
        sendbool(BLUETOOTH_PATH,name,on)
     }
    public fun sendunpair( node:Node,on:Boolean) {
         sendunpair(node.id,on);
     }
    public fun sendunpair( name:String,on:Boolean) {
        sendbool(UNPAIR_PATH,name,on)
     }
     /*
    public fun sendbluetooth(on:Boolean) {
    sendbool(BLUETOOTH_PATH,on)
     }
    public fun sendbool(String path,on:Boolean) {
        val onbyte:Byte=if(on) 1;else 0;
        val onar:ByteArray= byteArrayOf(onbyte)
    sendmessage(path,onar)
     } */
    public fun sendbool( path:String,nodeName:String,on:Boolean) {
        val onbyte:Byte=if(on) 1;else 0;
        val onar:ByteArray= byteArrayOf(onbyte)
       nameSendMessage(nodeName,path,onar)
     }

   public fun     findnodeid(id:String):Int {
       val nods=nodes
       if(nods==null)
           return -1
        val num = nods.size
        var it = 0
        while(true) {
            if (it == num) {
                Log.e(LOG_ID, "Can't find $id")
                return -1;
            }
            var othernode = nods.elementAt(it)
            if (id == othernode.getId()) {
               return it;
            }
            it++
        }
    }


companion object {
    private var findIter=0;
    private const val LOG_ID = "MessageSender"
    const val WAKE_PATH = "/wake"
    const val WAKESTREAM_PATH = "/wakestream"
    const val NET_PATH = "/netinfo"
    const val START_PATH = "/start"
    const val ASKFORSTART_PATH = "/askforstart"
    const val DEFAULTS_PATH = "/defaults"
    const val SETTINGS_PATH = "/settings"
    const val BLUETOOTH_PATH = "/bluetooth"
    const val UNPAIR_PATH = "/unpair"
    const val DATA_PATH = "/data"
    const val MESSAGES_PATH = "/messages"
    const val MESSAGES_ACK_PATH = "/messagesack"
    const val MIRROR_TRANSPORT_PATH = "/mirrortransport"
    const val MIRROR_TRANSPORT_ACK_PATH = "/mirrortransportack"
    fun isWearControlPath(path:String):Boolean = when(path) {
        START_PATH, ASKFORSTART_PATH, DEFAULTS_PATH, SETTINGS_PATH,
        BLUETOOTH_PATH, UNPAIR_PATH -> true
        else -> false
    }
    val scope = CoroutineScope(Dispatchers.IO+SupervisorJob()  )
    private const val CONTROL_ACK_SECONDS=8L
    private const val MESSAGE_TASK_SECONDS=30L
    private const val WEAR_CONTROL_TASK_SECONDS=3L
    private const val WEAR_DATA_TASK_SECONDS=5L
    // A sleeping Wear device can take >8 seconds to wake and return /messagesack.
    private const val WEAR_MESSAGE_ACK_SECONDS=15L
    private const val WEAR_MESSAGE_LIVENESS_MILLIS=30L*1000L
    private const val WEAR_MESSAGE_LIVENESS_FAILURES_BEFORE_BLE=2
    private const val TCP_RETRY_MILLIS=5L*60L*1000L
    private val controlSequence=AtomicLong(System.currentTimeMillis())
    private class ControlAck {
        val latch=CountDownLatch(1)
        @Volatile var value=false
        @Volatile var carrierApplied=false
        var label:String?=null
        var requested=false
        var automaticPhone=false
        var fromBle=false
    }
    private class MirrorTransportAck(val label:String) {
        val latch=CountDownLatch(1)
        @Volatile var accepted=false
    }
    private val controlAcks=ConcurrentHashMap<Long,ControlAck>()
    private val mirrorTransportAcks=ConcurrentHashMap<Long,MirrorTransportAck>()
    private val mirrorNodes=ConcurrentHashMap<String,String>()
    private val bleFallbackJobs=ConcurrentHashMap<String,Job>()
    private val wearFallbackJobs=ConcurrentHashMap<String,Job>()
    private val wearLivenessJobs=ConcurrentHashMap<String,Job>()
    private val wearMessageLastAck=ConcurrentHashMap<String,Long>()
    private val wearMessageSelected=ConcurrentHashMap.newKeySet<String>()
    private val wearMessageLivenessFailures=ConcurrentHashMap<String,Int>()
    private const val MIRROR_NODE_PREFS="mirror_wear_nodes"
    private var retryJob:Job?=null
    @Volatile private var messagesender: MessageSender? = null
    @JvmStatic
    public fun getMessageSender(): MessageSender? {
        return messagesender
    }

    @JvmStatic
    fun rememberMirrorNode(label:String,nodeId:String) {
        if(label.isEmpty()||nodeId.isEmpty())
            return
        synchronized(mirrorNodes) {
            val conflicting=mirrorNodes.entries
                .filter { it.key!=label&&it.value==nodeId }
                .map { it.key }
            val editor=mirrorNodePreferences().edit()
            conflicting.forEach { otherLabel ->
                mirrorNodes.remove(otherLabel,nodeId)
                editor.remove(otherLabel)
                Log.e(LOG_ID,"Removed conflicting Wear node mapping: mirror=$otherLabel node=$nodeId; kept mirror=$label")
            }
            val previous=mirrorNodes.put(label,nodeId)
            if(previous!=nodeId||conflicting.isNotEmpty()) {
                editor.putString(label,nodeId).apply()
                Log.i(LOG_ID,"Wear node mapping: mirror=$label node=$nodeId")
            }
        }
    }

    private fun forgetMirrorNode(label:String,reason:String) {
        val removed=mirrorNodes.remove(label)
        val preferences=mirrorNodePreferences()
        val persisted=preferences.contains(label)
        if(persisted)
            preferences.edit().remove(label).apply()
        if(removed!=null||persisted)
            Log.i(LOG_ID,"Removed Wear node mapping: mirror=$label node=$removed reason=$reason")
        // A disappearing Capability node is definitive evidence that a currently
        // selected Wear MessageClient carrier cannot deliver application data.
        // Do not leave an already-authenticated Direct-BLE link in standby.
        if(wearMessageSelected.remove(label)) {
            wearMessageLastAck.remove(label)
            wearMessageLivenessFailures.remove(label)
            requestDirectBleFallback(label,"Wear MessageClient node became unreachable: $reason")
        }
    }

    private fun mirrorNodePreferences() =
        Applic.app.getSharedPreferences(MIRROR_NODE_PREFS,Context.MODE_PRIVATE)

    @JvmStatic
    fun mirrorLabelForNode(nodeId:String):String? {
        if(mirrorNodes[nodeId]==nodeId)
            return nodeId
        val matches=mirrorNodes.entries.filter { it.value==nodeId }.map { it.key }
        if(matches.size>1) {
            Log.e(LOG_ID,"Ambiguous Wear node mapping for node=$nodeId mirrors=$matches")
            return null
        }
        return matches.singleOrNull()
    }

    private var nodenames: Array<String>? = null
    fun getNodeName(ident: Int): String {
        if (nodenames == null)
            throw NullPointerException("getNodeName nodenames==null")
        else
            return nodenames!!.get(ident)
    }
    @JvmStatic
    public fun sendaskforstart() {
        val sender = messagesender ?: return
        val ar = byteArrayOf(0);
        sender.sendmessage(ASKFORSTART_PATH, ar)
      }

    @JvmStatic
    fun sendStartBle(nodeName:String) {
        if(BleMirror.canSendWearControl(nodeName))
            BleMirror.sendAsync(nodeName,START_PATH,Natives.bytesettings())
    }

    @JvmStatic
    fun sendAskForStart(nodeName:String) {
        if(BleMirror.canSendWearControl(nodeName))
            BleMirror.sendAsync(nodeName,ASKFORSTART_PATH,byteArrayOf(0))
        else
            messagesender?.nameSendMessage(nodeName,ASKFORSTART_PATH,byteArrayOf(0))
    }

    @JvmStatic
    public fun sendwake() {
        val sender = messagesender ?: return
        val ar = byteArrayOf(0);
        sender.sendmessage(WAKE_PATH, ar)
    }

    @JvmStatic
    public fun sendwakestream() {
        val sender = messagesender ?: return
        val ar = byteArrayOf(0);
        sender.sendmessage(WAKESTREAM_PATH, ar)
    }

    // Every native mirror /data payload starts with the four-byte
    // wearmessagetype routing header.  BLE is a stream replacement, so a large
    // socket read may safely be delivered as several /data messages as long as
    // each piece repeats that routing header.  Keeping each stream piece small
    // prevents a 47 kB catch-up read from becoming one 95-fragment (or at MTU 23,
    // 5889-fragment) all-or-nothing BLE message.
    private const val MIRROR_DATA_HEADER_BYTES=4
    private const val BLE_DATA_STREAM_CHUNK_BYTES=1024

    private fun sendBleStreamChunks(ident:String,data:ByteArray):Boolean {
        if(data.size<=MIRROR_DATA_HEADER_BYTES+BLE_DATA_STREAM_CHUNK_BYTES)
            return BleMirror.send(ident,DATA_PATH,data)
        var offset=MIRROR_DATA_HEADER_BYTES
        var chunks=0
        while(offset<data.size) {
            val payload=Math.min(BLE_DATA_STREAM_CHUNK_BYTES,data.size-offset)
            val part=ByteArray(MIRROR_DATA_HEADER_BYTES+payload)
            System.arraycopy(data,0,part,0,MIRROR_DATA_HEADER_BYTES)
            System.arraycopy(data,offset,part,MIRROR_DATA_HEADER_BYTES,payload)
            if(!BleMirror.send(ident,DATA_PATH,part)) {
                Log.e(LOG_ID,"BLE stream chunk failed for $ident at offset=$offset total=${data.size}")
                return false
            }
            offset+=payload
            ++chunks
        }
        Log.i(LOG_ID,"BLE stream payload split for $ident: bytes=${data.size} chunks=$chunks")
        return true
    }

    @Keep
    @JvmStatic
    public fun sendDatawithName(ident: String, data: ByteArray): Boolean {
        if(BleMirror.shouldUse(ident))
            return sendBleStreamChunks(ident,data)
        val sender = messagesender ?: return false
    if(doLog) {Log.i(LOG_ID,"start sendDatawithName $ident");}
        val res=sender.nameSendMessageResult(ident, DATA_PATH, data)
    if(doLog) {Log.i(LOG_ID,"end sendDatawithName $ident");}
    return res;
    }

    public fun watchBluetooth(act:MainActivity,sensor:Boolean,nums:Boolean) {
        val sender = messagesender
        if (sender == null) {
            Log.e(LOG_ID, "sendData messagesender==null")
            return
            }
        if(sender.localnode==null) {
             Log.d(LOG_ID,"localnode==null")
             return
             }
        val name=sender.localnode;
        Natives.watchBluetooth(name,sensor,nums);
        }

    @Keep
    @JvmStatic
    public fun sendData(data: ByteArray): Boolean {
        val bleId=BleMirror.getLinkId()
        if(bleId!=null&&BleMirror.isPreferred())
            return BleMirror.send(bleId,DATA_PATH,data)
        val sender = messagesender
        if (sender == null) {
            Log.e(LOG_ID, "sendData messagesender==null")
            return false
        }
        val nodes = sender.nodes
        if (nodes == null) {
            Log.e(LOG_ID, "sendData nodes==null")
                scope.launch {
                sender.findWearDevicesWithApp()
                }
            return false;
        }
        if (nodes.isEmpty()) {
            Log.e(LOG_ID, "sendData nodes.isEmpty()")
            return false
        }
    Log.i(LOG_ID,"start sendData")
        val nodeId=nodes.elementAt(0).id
        val bleLabel=BleMirror.automaticWearLinkId()
        val res=if(bleLabel!=null)
            sender.nameSendMessageResultDirect(nodeId,DATA_PATH,data,WEAR_DATA_TASK_SECONDS)
        else
            sender.nameSendMessageResult(nodeId,DATA_PATH,data)
        if(bleLabel!=null) {
            if(!res) {
                wearMessageSelected.remove(bleLabel)
                requestDirectBleFallback(bleLabel,"Wear MessageClient data send failed")
            }
            else {
                wearMessageSelected.add(bleLabel)
                ensureAutomaticWearMessageLiveness(bleLabel,nodeId)
            }
        }
    Log.i(LOG_ID,"end sendData "+res)
    return res;
    }

    @Keep
    @JvmStatic
    public fun sendNameMessageOn(ident: String, on: Boolean):Boolean {
        return sendMessagesControl(ident,on)
    }

    @Keep
    @JvmStatic
    public fun sendMessageOn(on: Boolean):Boolean {
        val bleId=BleMirror.getLinkId()
        if(bleId!=null&&BleMirror.isPreferred())
            return sendNameMessageOn(bleId,on)
        val sender = messagesender
        if (sender == null) {
            Log.e(LOG_ID, "sendMessageOn messagesender==null")
            return false
        }
        val nodes = sender.nodes
        if (nodes == null) {
            Log.e(LOG_ID, "sendMessageOn nodes==null")
                scope.launch {
                sender.findWearDevicesWithApp()
                }
            return false
        }
        if (nodes.isEmpty()) {
            Log.e(LOG_ID, "sendMessageOn nodes.isEmpty()")
            return false
        }

        return sendNameMessageOn(nodes.elementAt(0).id, on)
    }

    @JvmStatic
    fun receiveMessagesAck(label:String,data:ByteArray,fromBle:Boolean) {
        if(data.size<9)
            return
        val input=ByteBuffer.wrap(data).order(ByteOrder.LITTLE_ENDIAN)
        val value=input.get().toInt()!=0
        val request=input.getLong()
        controlAcks[request]?.let { ack ->
            if(ack.fromBle!=fromBle||(fromBle&&ack.label!=label)) {
                Log.e(LOG_ID,"Ignoring /messagesack on the wrong carrier or mirror: $label")
                return
            }
            ack.value=value
            val accepted=value==ack.requested
            val label=ack.label
            // BLE messages are dispatched through one receive executor. Apply
            // the local carrier while processing /messagesack, before the next
            // queued /data message can reach the native bridge.
            ack.carrierApplied=!ack.automaticPhone||!accepted||label==null||
                BleMirror.setAutomaticCarrier(label,ack.requested)
            ack.latch.countDown()
        }
    }

    /** Send an acknowledged /messages command specifically through authenticated BLE. */
    private fun sendBleControl(label:String,on:Boolean):Boolean {
        if(!BleMirror.canSendControl(label))
            return false
        val request=controlSequence.incrementAndGet()
        val data=ByteBuffer.allocate(9).order(ByteOrder.LITTLE_ENDIAN)
            .put(if(on) 1.toByte() else 0.toByte()).putLong(request).array()
        val acknowledged=ControlAck().also {
            it.label=label
            it.requested=on
            it.automaticPhone=BleMirror.isAutomaticPhonePeer(label)
            it.fromBle=true
        }
        controlAcks[request]=acknowledged
        return try {
            if(!BleMirror.send(label,MESSAGES_PATH,data))
                false
            else if(!acknowledged.latch.await(CONTROL_ACK_SECONDS,TimeUnit.SECONDS)) {
                Log.e(LOG_ID,"No BLE /messagesack from $label")
                false
            }
            else {
                val accepted=acknowledged.value==on
                val applied=accepted&&acknowledged.carrierApplied
                if(applied&&BleMirror.isAutomaticWearPeer(label)) {
                    // This control travelled over Direct BLE; MessageClient is no
                    // longer the active data carrier regardless of on/off.
                    wearMessageSelected.remove(label)
                    wearMessageLastAck.remove(label)
                    wearMessageLivenessFailures.remove(label)
                }
                applied
            }
        }
        catch(th:Throwable) {
            if(th is InterruptedException)
                Thread.currentThread().interrupt()
            Log.stack(LOG_ID,"sendBleControl",th)
            false
        }
        finally {
            controlAcks.remove(request)
        }
    }

    /**
     * Re-select an Automatic BLE carrier after the API-21/22 single-slot
     * scheduler deliberately yielded that peer. The GATT link has already
     * authenticated, so the normal acknowledged /messages=true handshake can
     * be reused; this preserves data synchronization across time-sliced peers.
     */
    @JvmStatic
    fun resumeAutomaticBleCarrier(label:String) {
        requestDirectBleFallback(label,"Android 5 BLE slot resumed")
    }

    /**
     * Promote an Automatic mirror to Direct BLE after another carrier has
     * actually failed. Discovery and authentication may still be in flight;
     * one bounded coroutine waits for readiness and then switches both peers
     * using the already authenticated /messages control packet.
     */
    @JvmStatic
    fun localNetworkUnavailable(label:String) {
        if(!BleMirror.isAutomaticPhonePeer(label))
            return
        Log.i(LOG_ID,"Local side of Automatic BLE peer $label has no current TCP/IP endpoint")
        if(BleMirror.isAutomaticWearPeer(label)) {
            val node=mirrorNodes[label]?:label
            requestAutomaticWearFallback(label,node,"local side has no current TCP/IP endpoint")
        }
        else
            requestDirectBleFallback(label,"local side has no current TCP/IP endpoint")
    }

    @JvmStatic
    fun peerNetworkUnavailable(label:String) {
        if(!BleMirror.isAutomaticPhonePeer(label))
            return
        Log.i(LOG_ID,"Authenticated BLE peer $label reports no current TCP/IP endpoint")
        if(BleMirror.isAutomaticWearPeer(label)) {
            // Wear Automatic prefers Google MessageClient when it is genuinely
            // reachable. Lack of Wi-Fi/TCP must not consume a Direct-BLE GATT
            // slot until MessageClient has also failed.
            val node=mirrorNodes[label]?:label
            requestAutomaticWearFallback(label,node,"peer reported no current TCP/IP endpoint")
        }
        else
            requestDirectBleFallback(label,"peer reported no current TCP/IP endpoint over authenticated BLE")
    }

    private fun requestDirectBleFallback(label:String,reason:String) {
        if(!BleMirror.isPeer(label))
            return
        BleMirror.requestConnection(label)
        synchronized(bleFallbackJobs) {
            if(bleFallbackJobs[label]?.isActive==true)
                return
            val job=scope.launch {
                try {
                    Log.i(LOG_ID,"Direct BLE fallback requested for $label: $reason")
                    var waits=0
                    while(BleMirror.isAutomaticFallbackRequested(label)) {
                        if(BleMirror.canSendControl(label)&&sendBleControl(label,true)) {
                            wearMessageSelected.remove(label)
                            wearMessageLastAck.remove(label)
                            Log.i(LOG_ID,"Direct BLE fallback activated for $label")
                            return@launch
                        }
                        delay(500L)
                        ++waits
                        if(waits%40==0)
                            Log.i(LOG_ID,"Direct BLE fallback still waiting for $label after ${waits/2} seconds")
                    }
                    Log.i(LOG_ID,"Direct BLE fallback cancelled for $label because TCP/IP became usable")
                }
                finally {
                    val self=coroutineContext[Job]
                    if(self!=null)
                        bleFallbackJobs.remove(label,self)
                }
            }
            bleFallbackJobs[label]=job
        }
    }

    /**
     * Automatic Wear fallback must not block sender.cpp's pending TCP poll while
     * Google Play services waits on MessageClient.  Try MessageClient here on
     * an IO coroutine; if it cannot establish the acknowledged carrier, promote
     * the already-discoverable Direct-BLE rendezvous instead.
     */
    private fun requestAutomaticWearFallback(label:String,nodeName:String,reason:String) {
        synchronized(wearFallbackJobs) {
            if(wearFallbackJobs[label]?.isActive==true)
                return
            val job=scope.launch {
                try {
                    Log.i(LOG_ID,"Automatic Wear fallback requested for $label: $reason")
                    val request=controlSequence.incrementAndGet()
                    val data=ByteBuffer.allocate(9).order(ByteOrder.LITTLE_ENDIAN)
                        .put(1.toByte()).putLong(request).array()
                    val acknowledged=ControlAck()
                    controlAcks[request]=acknowledged
                    var messagesReady=false
                    try {
                        val actualNode=mirrorNodes[label]?:mirrorNodes[nodeName]?:nodeName
                        val delivered=messagesender?.nameSendMessageResultDirect(
                            actualNode,MESSAGES_PATH,data,WEAR_CONTROL_TASK_SECONDS)?:false
                        if(delivered) {
                            val gotAck=acknowledged.latch.await(WEAR_MESSAGE_ACK_SECONDS,TimeUnit.SECONDS)
                            messagesReady=gotAck&&acknowledged.value
                            if(messagesReady) {
                                wearMessageLastAck[label]=System.currentTimeMillis()
                                wearMessageLivenessFailures.remove(label)
                            }
                            else
                                Log.e(LOG_ID,"Wear MessageClient /messages control was not acknowledged by $actualNode")
                        }
                    }
                    finally {
                        controlAcks.remove(request)
                    }
                    if(messagesReady) {
                        wearMessageSelected.add(label)
                        wearMessageLivenessFailures.remove(label)
                        // MessageClient is now end-to-end proven. Cancel a pending
                        // Direct-BLE request, or demote an already selected BLE
                        // carrier, so scarce GATT slots are left for phone/tablet
                        // mirrors which have no Wear MessageClient alternative.
                        if(BleMirror.isAutomaticCarrierActive(label))
                            BleMirror.setAutomaticCarrier(label,false)
                        else
                            BleMirror.cancelAutomaticFallback(label)
                        bleFallbackJobs.remove(label)?.cancel()
                        Log.i(LOG_ID,"Automatic Wear MessageClient carrier ready for $label")
                        Natives.setBlueMessage(label,true)
                    }
                    else {
                        wearMessageSelected.remove(label)
                        wearMessageLastAck.remove(label)
                        wearMessageLivenessFailures.remove(label)
                        Log.i(LOG_ID,"Automatic Wear MessageClient failed for $label; trying Direct BLE")
                        requestDirectBleFallback(label,"MessageClient unavailable or unacknowledged after TCP/IP")
                    }
                }
                catch(th:Throwable) {
                    if(th is InterruptedException)
                        Thread.currentThread().interrupt()
                    Log.stack(LOG_ID,"requestAutomaticWearFallback",th)
                    requestDirectBleFallback(label,"MessageClient exception after TCP/IP")
                }
                finally {
                    val self=coroutineContext[Job]
                    if(self!=null)
                        wearFallbackJobs.remove(label,self)
                }
            }
            wearFallbackJobs[label]=job
        }
    }

    /**
     * MessageClient.sendMessage() succeeding is not end-to-end delivery proof.
     * While Automatic Wear is actually using MessageClient, periodically send
     * an acknowledged /messages=true control. A missing app-level ACK promotes
     * the already configured Direct-BLE fallback.
     */
    private fun ensureAutomaticWearMessageLiveness(label:String,nodeName:String) {
        if(!wearMessageSelected.contains(label))
            return
        val now=System.currentTimeMillis()
        if(now-(wearMessageLastAck[label]?:0L)<WEAR_MESSAGE_LIVENESS_MILLIS)
            return
        synchronized(wearLivenessJobs) {
            if(wearLivenessJobs[label]?.isActive==true)
                return
            val job=scope.launch {
                try {
                    if(!wearMessageSelected.contains(label))
                        return@launch
                    val request=controlSequence.incrementAndGet()
                    val data=ByteBuffer.allocate(9).order(ByteOrder.LITTLE_ENDIAN)
                        .put(1.toByte()).putLong(request).array()
                    val acknowledged=ControlAck()
                    controlAcks[request]=acknowledged
                    var alive=false
                    try {
                        val actualNode=mirrorNodes[label]?:nodeName
                        val delivered=messagesender?.nameSendMessageResultDirect(
                            actualNode,MESSAGES_PATH,data,WEAR_CONTROL_TASK_SECONDS)?:false
                        alive=delivered&&acknowledged.latch.await(
                            WEAR_MESSAGE_ACK_SECONDS,TimeUnit.SECONDS)&&acknowledged.value
                    }
                    finally {
                        controlAcks.remove(request)
                    }
                    if(alive) {
                        wearMessageLastAck[label]=System.currentTimeMillis()
                        wearMessageLivenessFailures.remove(label)
                        Log.i(LOG_ID,"Automatic Wear MessageClient liveness acknowledged for $label")
                    }
                    else if(wearMessageSelected.contains(label)) {
                        val failures=(wearMessageLivenessFailures[label]?:0)+1
                        wearMessageLivenessFailures[label]=failures
                        if(failures<WEAR_MESSAGE_LIVENESS_FAILURES_BEFORE_BLE) {
                            Log.w(LOG_ID,"Automatic Wear MessageClient liveness miss $failures/"+
                                "$WEAR_MESSAGE_LIVENESS_FAILURES_BEFORE_BLE for $label; keeping Messages")
                        }
                        else if(wearMessageSelected.remove(label)) {
                            wearMessageLastAck.remove(label)
                            wearMessageLivenessFailures.remove(label)
                            Log.e(LOG_ID,"Automatic Wear MessageClient repeatedly lost application-level liveness for $label")
                            requestDirectBleFallback(label,"Wear MessageClient carrier repeatedly stopped acknowledging")
                        }
                    }
                }
                catch(th:Throwable) {
                    if(th is InterruptedException)
                        Thread.currentThread().interrupt()
                    Log.stack(LOG_ID,"ensureAutomaticWearMessageLiveness",th)
                    if(wearMessageSelected.contains(label)) {
                        val failures=(wearMessageLivenessFailures[label]?:0)+1
                        wearMessageLivenessFailures[label]=failures
                        if(failures>=WEAR_MESSAGE_LIVENESS_FAILURES_BEFORE_BLE&&
                            wearMessageSelected.remove(label)) {
                            wearMessageLastAck.remove(label)
                            wearMessageLivenessFailures.remove(label)
                            requestDirectBleFallback(label,"Wear MessageClient liveness check repeatedly failed")
                        }
                    }
                }
                finally {
                    val self=coroutineContext[Job]
                    if(self!=null)
                        wearLivenessJobs.remove(label,self)
                }
            }
            wearLivenessJobs[label]=job
        }
    }

    private fun sendMessagesControl(nodeName:String,on:Boolean):Boolean {
        val directLabel=when {
            BleMirror.isPeer(nodeName) -> nodeName
            isWearable -> BleMirror.automaticWearLinkId()
            else -> null
        }

        // Once BLE is the selected carrier, control must travel over BLE too.
        if(directLabel!=null&&BleMirror.isConfiguredPeer(directLabel)&&
                BleMirror.canSendControl(directLabel))
            return sendBleControl(directLabel,on)

        val automaticWear=directLabel!=null&&BleMirror.isAutomaticWearPeer(directLabel)
        if(automaticWear&&!on&&directLabel!=null) {
            BleMirror.cancelAutomaticFallback(directLabel)
            bleFallbackJobs.remove(directLabel)?.cancel()
            Log.i(LOG_ID,"Automatic Wear Direct BLE fallback cancelled for $directLabel: TCP/IP ready")
        }
        if(automaticWear&&on&&directLabel!=null) {
            // sender.cpp may still have several TCP sockets pending in poll().
            // Do not block that native thread on MessageClient/ACK timeouts.
            requestAutomaticWearFallback(directLabel,nodeName,"TCP/IP fallback requested")
            return false
        }
        if(!automaticWear&&directLabel!=null) {
            // Phone/phone Automatic has no Google Data Layer peer.  A TCP
            // failure keeps BLE demand armed until TCP actually succeeds; on
            // Android 5 another peer may legitimately delay acquisition beyond
            // the old 20-second fallback job.
            if(on) {
                requestDirectBleFallback(directLabel,"TCP/IP fallback requested")
                return false
            }
            BleMirror.cancelAutomaticFallback(directLabel)
            bleFallbackJobs.remove(directLabel)?.cancel()
            Log.i(LOG_ID,"Automatic Direct BLE fallback cancelled for $directLabel: TCP/IP ready")
            return true
        }

        // Wear OS: MessageClient remains the first non-TCP fallback. The
        // target here is the actual Wear node ID (on a watch it differs from
        // the local mirror label).
        val request=controlSequence.incrementAndGet()
        val data=ByteBuffer.allocate(9).order(ByteOrder.LITTLE_ENDIAN)
            .put(if(on) 1.toByte() else 0.toByte()).putLong(request).array()
        val acknowledged=ControlAck()
        controlAcks[request]=acknowledged
        return try {
            val delivered=messagesender?.nameSendMessageResultDirect(
                mirrorNodes[nodeName]?:nodeName,MESSAGES_PATH,data)?:false
            if(!delivered) {
                if(on&&directLabel!=null)
                    requestDirectBleFallback(directLabel,"Wear MessageClient control failed")
                false
            }
            else {
                val gotAck=acknowledged.latch.await(CONTROL_ACK_SECONDS,TimeUnit.SECONDS)
                if(gotAck) {
                    val accepted=acknowledged.value==on
                    if(accepted&&automaticWear&&directLabel!=null) {
                        if(on) {
                            wearMessageSelected.add(directLabel)
                            wearMessageLastAck[directLabel]=System.currentTimeMillis()
                        }
                        else {
                            wearMessageSelected.remove(directLabel)
                            wearMessageLastAck.remove(directLabel)
                        }
                    }
                    accepted
                }
                else if(automaticWear) {
                    Log.e(LOG_ID,"No /messagesack from $nodeName; refusing Automatic Wear carrier switch")
                    false
                }
                else {
                    // Keep compatibility for explicitly forced legacy MessageClient
                    // peers; Automatic Wear has Direct BLE and must require proof.
                    Log.i(LOG_ID,"No /messagesack from $nodeName; using MessageClient task result for forced legacy carrier")
                    true
                }
            }
        }
        catch(th:Throwable) {
            if(th is InterruptedException)
                Thread.currentThread().interrupt()
            Log.stack(LOG_ID,"sendMessagesControl",th)
            if(on&&directLabel!=null)
                requestDirectBleFallback(directLabel,"Wear MessageClient control exception")
            false
        }
        finally {
            controlAcks.remove(request)
        }
    }

    /**
     * An authenticated BLE /bleips update proves that this peer currently has a
     * network endpoint. Do not open a temporary TCP probe while the BLE bridge is
     * selected: the receiver side uses the same TCPConnect receiver slot for the
     * bridge and accepted TCP sockets, so such a probe can destroy both itself
     * and the working BLE carrier. Release BLE first with the acknowledged
     * /messages=false control; the normal sender then performs the ordinary
     * password-authenticated TCP connection and falls back to BLE again if needed.
     */
    @JvmStatic
    fun preferTcpAfterAddressUpdate(label:String,hostIndex:Int) {
        scope.launch {
            if(!BleMirror.isAutomaticCarrierActive(label))
                return@launch
            Log.i(LOG_ID,"BLE supplied authenticated current IPs for $label; releasing Bluetooth for preferred TCP/IP retry")
            val switched=sendBleControl(label,false)
            Log.i(LOG_ID,"TCP/IP handoff after authenticated BLE endpoint update for $label: $switched")
            if(switched)
                Applic.wakemirrors()
        }
    }

    @JvmStatic
    fun sendMessagesAck(node:String,data:ByteArray) {
        if(data.size<9)
            return
        // This acknowledges a MessageClient probe, so it must prove the return
        // path through MessageClient as well. BLE probes reply on BLE separately.
        scope.launch {
            messagesender?.nameSendMessageResultDirect(node,MESSAGES_ACK_PATH,data.copyOf(9))
        }
    }

    private fun sendBleMirrorTransport(label:String,transport:Int,remoteClient:Boolean):Boolean {
        if(!BleMirror.canSendWearControl(label))
            return false
        val request=controlSequence.incrementAndGet()
        val labelBytes=label.toByteArray(Charsets.UTF_8)
        // Version 2 adds a request ID so a response from another mirror or an
        // earlier change cannot release this change's connection hold.
        val data=ByteBuffer.allocate(11+labelBytes.size).order(ByteOrder.LITTLE_ENDIAN)
            .put(2.toByte()).put(transport.toByte())
            .put(if(remoteClient) 1.toByte() else 0.toByte())
            .putLong(request).put(labelBytes).array()
        val acknowledged=MirrorTransportAck(label)
        mirrorTransportAcks[request]=acknowledged
        return try {
            val delivered=BleMirror.send(label,MIRROR_TRANSPORT_PATH,data)
            // The peer may acknowledge and close GATT before Android reports
            // completion of our final write. Its app-level ACK is stronger.
            if(!delivered&&acknowledged.latch.count!=0L)
                false
            else if(!acknowledged.latch.await(CONTROL_ACK_SECONDS,TimeUnit.SECONDS)) {
                Log.e(LOG_ID,"No BLE /mirrortransportack from $label")
                false
            }
            else acknowledged.accepted
        }
        finally {
            mirrorTransportAcks.remove(request)
        }
    }

    fun receiveMirrorTransportAck(label:String,data:ByteArray) {
        if(data.size!=9||!BleMirror.canSendWearControl(label))
            return
        val input=ByteBuffer.wrap(data).order(ByteOrder.LITTLE_ENDIAN)
        val accepted=input.get().toInt()
        if(accepted !in 0..1)
            return
        val request=input.getLong()
        mirrorTransportAcks[request]?.let { ack ->
            if(ack.label!=label) {
                Log.e(LOG_ID,"Rejected /mirrortransportack from another mirror: $label")
                return
            }
            ack.accepted=accepted==1
            ack.latch.countDown()
        }
    }

    fun receiveMirrorTransport(sourceId:String,connectionName:String,data:ByteArray,fromBle:Boolean) {
        val headerSize=if(fromBle) 11 else 3
        val version=if(fromBle) 2 else 1
        if(data.size<headerSize||data[0].toInt()!=version) {
            Log.e(LOG_ID,"Invalid /mirrortransport payload (${data.size} bytes, BLE=$fromBle)")
            return
        }
        val transport=data[1].toInt() and 0xff
        val role=data[2].toInt()
        val mirrorLabel=if(data.size>headerSize)
            String(data,headerSize,data.size-headerSize,Charsets.UTF_8)
        else connectionName
        if(transport !in BleMirror.TRANSPORT_AUTOMATIC..BleMirror.TRANSPORT_BLUETOOTH||role !in 0..1||
            mirrorLabel.isEmpty()||mirrorLabel!=connectionName) {
            Log.e(LOG_ID,"Rejected mirror transport: source=$sourceId expected=$connectionName payload=$mirrorLabel transport=$transport role=$role")
            return
        }
        // Hold before mutating native configuration: reconciliation may also
        // be requested by a permission callback or a simultaneous local edit.
        if(fromBle&&!BleMirror.holdConfigurationForWearControl(connectionName)) {
            Log.e(LOG_ID,"Rejected transport selection outside an authenticated Wear mirror: $connectionName")
            return
        }
        var applied=false
        try {
            applied=Natives.setMirrorTransport(mirrorLabel,transport,role==1)
            if(applied&&!fromBle)
                rememberMirrorNode(mirrorLabel,sourceId)
            Log.i(LOG_ID,"Mirror $mirrorLabel selected transport=$transport bleclient=${role==1} applied=$applied")
        }
        catch(th:Throwable) {
            Log.stack(LOG_ID,"apply mirror transport",th)
        }
        if(fromBle) {
            val request=ByteBuffer.wrap(data).order(ByteOrder.LITTLE_ENDIAN).getLong(3)
            val ack=ByteBuffer.allocate(9).order(ByteOrder.LITTLE_ENDIAN)
                .put(if(applied) 1.toByte() else 0.toByte()).putLong(request).array()
            // Do not block BLE's receive executor waiting for a GATT write.
            // That executor must remain available for simultaneous peer ACKs.
            scope.launch {
                try {
                    val sent=BleMirror.send(connectionName,MIRROR_TRANSPORT_ACK_PATH,ack)
                    Log.i(LOG_ID,"BLE mirror transport reply for $connectionName: $sent")
                }
                finally {
                    BleMirror.releaseConfigurationForWearControl()
                }
            }
        }
        else if(applied)
            BleMirror.configurationChanged()
    }

    /** Propagate a choice on the existing connection before rebuilding it. */
    @JvmOverloads
    @JvmStatic
    fun sendMirrorTransport(connectionName:String,transport:Int,remoteClient:Boolean,releaseBleConfiguration:Runnable?=null) {
        // A caller holding the old GATT connection must finish or fail on BLE.
        // Do not leave that hold waiting on Google discovery if BLE disconnects
        // before this coroutine runs.
        val throughBle=releaseBleConfiguration!=null||BleMirror.canSendWearControl(connectionName)
        scope.launch {
          try {
            if(throughBle) {
                val sent=sendBleMirrorTransport(connectionName,transport,remoteClient)
                Log.i(LOG_ID,"Mirror transport $transport for $connectionName acknowledged over BLE: $sent")
                return@launch
            }
            val sender=messagesender
            if(sender==null) {
                Log.e(LOG_ID,"Cannot propagate mirror transport: no ready BLE peer or MessageSender")
                return@launch
            }
            if(sender.nodes==null||sender.nodes?.isEmpty()==true)
                sender.findWearDevicesWithApp()
            val reachable=sender.nodes
            val mapped=mirrorNodes[connectionName]?.takeIf { nodeId ->
                reachable?.any { it.id==nodeId }==true
            }
            val target=if(isWearable)
                mapped?:reachable?.singleOrNull()?.id
            else
                mapped?:reachable?.firstOrNull { it.id==connectionName }?.id
            if(target==null) {
                Log.e(LOG_ID,"Cannot propagate mirror transport for $connectionName: no unambiguous reachable Wear node")
                return@launch
            }
            rememberMirrorNode(connectionName,target)
            // Keep version 1 and legacy /messages interoperability on Google.
            val labelBytes=connectionName.toByteArray(Charsets.UTF_8)
            val data=byteArrayOf(1.toByte(),transport.toByte(),if(remoteClient) 1.toByte() else 0.toByte())+labelBytes
            val sent=sender.nameSendMessageResultDirect(target,MIRROR_TRANSPORT_PATH,data)
            Log.i(LOG_ID,"mirror transport $transport for label=$connectionName sent to node=$target: $sent")

            // Current releases ignore /mirrortransport but understand the
            // established /messages command. Keep forced TCP/Messages modes
            // interoperable during a rolling phone/watch update.
            when(transport) {
                BleMirror.TRANSPORT_TCP -> {
                    val legacy=sendMessagesControl(target,false)
                    Log.i(LOG_ID,"legacy TCP-only selection sent to $target: $legacy")
                }
                BleMirror.TRANSPORT_MESSAGES -> {
                    val legacy=sendMessagesControl(target,true)
                    Log.i(LOG_ID,"legacy Messages-only selection sent to $target: $legacy")
                }
            }
          }
          catch(th:Throwable) {
            if(th is InterruptedException)
                Thread.currentThread().interrupt()
            Log.stack(LOG_ID,"send mirror transport",th)
          }
          finally {
            releaseBleConfiguration?.run()
          }
        }
    }
/*
@Keep
@JvmStatic
public fun sendDatawithInt(ident: Int, data: ByteArray) {
        try {
        messagesender?.nameSendMessage(getNodeName(ident), DATA_PATH, data)
        } catch (th: Throwable) {
        Log.stack(LOG_ID, "sendData $ident", th);
        }
    } */

    @JvmStatic
    public fun initwearos(app: Context) {
        if(doLog) {Log.i(LOG_ID, "before new MessageSender");}
        messagesender = MessageSender(app)
        BleMirror.init(app)
        if(retryJob==null) {
            retryJob=scope.launch {
                while(isActive) {
                    delay(TCP_RETRY_MILLIS)
                    try {
                        // Wear mirrors retry when WLAN is known to be back;
                        // phone/phone mirrors periodically leave active BLE fallback
                        // long enough to test their retained TCP/IP settings.
                        Natives.retryMessageConnections(null)
                    }
                    catch(th:Throwable) {
                        Log.stack(LOG_ID,"retryMessageConnections",th)
                    }
                }
            }
        }
//    {if(doLog) {Log.i(LOG_ID,"before sendnetinfo");};};
//    sendnetinfo();
    }

    @JvmStatic
    public fun cansend(): Boolean {
        if(BleMirror.isReady())
            return true
        val sender: MessageSender? = messagesender
        if (sender == null) {
            Log.e(LOG_ID, "messagesender==null");
            return false
        }
        val tmp = sender.nodes
        if (tmp == null || tmp.isEmpty()) {
            Log.e(LOG_ID, "no sender.nodes");
            return false
        }
        return true
    }

    private const val netwait = (1000).toLong()
    private const val NETINFO_RETRY_MILLIS=2000L
    private const val NETINFO_ATTEMPTS=3
    private const val NETINFO_IN_FLIGHT_MILLIS=(MESSAGE_TASK_SECONDS+5L)*1000L

    /** Reply using the authenticated BLE label, without Wear node discovery. */
    @JvmStatic fun sendBleNetInfo(label:String) {
        scope.launch {
            val sender=messagesender?:return@launch
            for(attempt in 1..NETINFO_ATTEMPTS) {
                if(sender.sendBleNetInfoResult(label))
                    return@launch
                delay(NETINFO_RETRY_MILLIS)
            }
            Log.e(LOG_ID,"BLE /netinfo reply failed: mirror=$label")
        }
    }

    private suspend fun inargsendnetinfo(id: String) {
        if(doLog) {Log.i(LOG_ID,"sendnetinfo($id)");}
        for(attempt in 1..NETINFO_ATTEMPTS) {
            val sender=messagesender
            if(sender==null) {
                Log.i(LOG_ID,"Wear /netinfo waits for MessageSender: node=$id attempt=$attempt")
                delay(NETINFO_RETRY_MILLIS)
                continue
            }
            var othernode=sender.nodes?.firstOrNull { it.id==id }
            if(othernode==null) {
                Log.i(LOG_ID,"Wear /netinfo discovers node=$id attempt=$attempt")
                sender.findWearDevicesWithApp()
                othernode=sender.nodes?.firstOrNull { it.id==id }
            }
            if(othernode==null) {
                delay(NETINFO_RETRY_MILLIS)
                continue
            }
            val nu = System.currentTimeMillis()
            val reservation=sender.reserveNetInfo(id,nu)
            if(reservation==null) {
                Log.i(LOG_ID,"Wear /netinfo already sent or in flight for node=$id")
                return
            }
            // The target node identifies the watch on a phone.  On a watch,
            // the mirror is always named after this watch's own local node.
            // Reverse map lookup is unsafe when a stale preference aliases two
            // rows to one reachable watch.
            val mirrorName=if(isWearable) sender.localnode else id
            if(isWearable)
                rememberMirrorNode(mirrorName,id)
            val netinfo=if(isWearable)
                Natives.getmynetinfo(mirrorName,true,0,true,0,false)
            else
                Natives.getmynetinfo(mirrorName,false,0,isGalaxy(othernode),0,false)
            if(netinfo == null) {
                sender.releaseNetInfo(id,reservation)
                Log.e(LOG_ID,"Wear /netinfo data unavailable: mirror=$mirrorName node=$id attempt=$attempt")
                delay(NETINFO_RETRY_MILLIS)
                continue
            }
            if(sender.nameSendMessageResult(id,NET_PATH,netinfo)) {
                sender.confirmNetInfo(id,reservation)
                Log.i(LOG_ID,"Wear /netinfo delivered: mirror=$mirrorName node=$id attempt=$attempt")
                return
            }
            sender.releaseNetInfo(id,reservation)
            Log.e(LOG_ID,"Wear /netinfo delivery failed: mirror=$mirrorName node=$id attempt=$attempt")
            delay(NETINFO_RETRY_MILLIS)
        }
        Log.e(LOG_ID,"Wear /netinfo retry limit reached for node=$id")
    }

        @JvmStatic     public fun sendnetinfo(id: String) {
        scope.launch {    
                inargsendnetinfo(id) 
            }
        }
    private suspend fun insendnetinfo() {
        Log.i(LOG_ID,"sendnetinfo()")

            val nu = System.currentTimeMillis()
            val sender=messagesender
            if(sender==null) {
                Log.i(LOG_ID,"Wear /netinfo broadcast waits for MessageSender")
                return
            }
            // BLE mirrors remain reachable even if the Wear Data Layer does
            // not list the peer. Refresh them before attempting node discovery.
            val bleMirrors=BleMirror.wearControlLabels().toSet()
            for(label in bleMirrors)
                if(!sender.sendBleNetInfoResult(label))
                    sendBleNetInfo(label)
            if(isWearable&&bleMirrors.isNotEmpty())
                return
            var nodes=sender.nodes
            if (nodes == null || nodes.isEmpty())  {
                sender.findWearDevicesWithApp()
                nodes=sender.nodes
                if(nodes==null||nodes.isEmpty()) {
                    Log.i(LOG_ID,"Wear /netinfo broadcast has no reachable nodes")
                    return
                }
            }
            if(isWearable&&nodes.size!=1) {
                Log.e(LOG_ID,"Wear /netinfo broadcast requires one unambiguous companion phone; reachable nodes=${nodes.map { it.id }}")
                return
            }
            val num = nodes.size
            for(i in 0 until num) {
                val node: Node = nodes.elementAt(i)
                if(node.id in bleMirrors)
                    continue
                val reservation=sender.reserveNetInfo(node.id,nu)
                if(reservation!=null) {
                    val name = if(isWearable) sender.localnode else node.id
                    if(isWearable)
                        rememberMirrorNode(name,node.id)
                    val netinfo=Natives.getmynetinfo(name,isWearable,0,isGalaxy(node),0,false)
                    if(netinfo==null) {
                        sender.releaseNetInfo(node.id,reservation)
                        continue
                    }
                    if(sender.nameSendMessageResult(node.id,NET_PATH,netinfo)) {
                        sender.confirmNetInfo(node.id,reservation)
                        Log.i(LOG_ID,"Wear /netinfo broadcast delivered: mirror=$name node=${node.id}")
                    }
                    else {
                        sender.releaseNetInfo(node.id,reservation)
                        Log.e(LOG_ID,"Wear /netinfo broadcast delivery failed: mirror=$name node=${node.id}")
                        sendnetinfo(node.id)
                    }
                } else {
                    Log.i(LOG_ID, "sendnetinfo already done " + node.id)
                  }
              }
        }
      @JvmStatic    public fun sendnetinfo() {
        scope.launch {    
                insendnetinfo()
            }
        }

      private fun retryTcpAfterNetworkUpdate(reason:String,delayMillis:Long) {
        scope.launch {
            try {
                // Native code only releases a working alternate carrier when
                // both the local network and an authenticated *current* peer
                // endpoint are known. The normal sender then performs the actual
                // TCP/password authentication.
                delay(delayMillis)
                Log.i(LOG_ID,"$reason; re-evaluating preferred TCP/IP for active Automatic non-TCP carriers")
                Natives.retryMessageConnections(null)
            }
            catch(th:Throwable) {
                if(th is InterruptedException)
                    Thread.currentThread().interrupt()
                Log.stack(LOG_ID,"network update TCP retry",th)
            }
        }
      }

      /**
       * A real link-address change must bypass the normal /netinfo resend throttle.
       * It must also wake Automatic mirrors that are still using Messages/Direct BLE:
       * the ordinary TCP retry loop is five minutes, which otherwise leaves a newly
       * restored Wi-Fi connection unused for minutes even though remembered peer IPs
       * are already available.
       */
      @JvmStatic public fun networkChanged() {
        getMessageSender()?.nulltimes()
        sendnetinfo()
        // Let ConnectivityManager finish publishing the new local route/address.
        retryTcpAfterNetworkUpdate("Local network changed",500L)
        }

      /**
       * A peer /netinfo update can be the only event visible on this device when,
       * for example, Wi-Fi is enabled on a watch.  Retry immediately after storing
       * the peer's new/remembered addresses instead of waiting for the five-minute
       * periodic Automatic TCP retry.
       */
      @JvmStatic public fun peerNetworkInfoChanged() {
        retryTcpAfterNetworkUpdate("Peer network information changed",250L)
        }
     @JvmStatic     
     public fun isGalaxy(node:Node): Boolean {
         val name=node.getDisplayName()
         val res= name.startsWith("Galaxy Watch") 
         Log.i(LOG_ID,"isGalaxy($name)=$res")
         if(Applic.ALLGALAXY)
                return true;
         return res;
       }

     @JvmStatic     
     public fun reinit() {
        Log.i(LOG_ID,"reinit")
        Natives.resetnetwork()
        getMessageSender()?.nulltimes()
        sendnetinfo()
        }
    }

}
