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

import com.google.android.gms.wearable.*
import kotlinx.coroutines.launch
import tk.glucodata.Applic.isWearable
import tk.glucodata.Log.doLog
import tk.glucodata.MessageSender.Companion.isGalaxy
//import tk.glucodata.MessageSender.Companion.messagesender
import tk.glucodata.MessageSender.Companion.sendnetinfo
import tk.glucodata.Natives
import tk.glucodata.Natives.setWearosdefaults
import tk.glucodata.SensorBluetooth.unpairWatch

class MessageReceiver: WearableListenerService() {
    override fun onMessageReceived(messageEvent: MessageEvent) {
        super.onMessageReceived(messageEvent)
        val data= messageEvent.data
        val path= messageEvent.path
        val sourceId=messageEvent.sourceNodeId
        val sender=MessageSender.getMessageSender()
        val sourceNode=sender?.nodes?.firstOrNull { it.id==sourceId }
        val galaxy=if(isWearable) true else sourceNode?.let(::isGalaxy)?:Applic.ALLGALAXY
        val connectionName=if(isWearable) {
            val localMirror=try {
                sender?.localnode
            }
            catch(th:Throwable) {
                Log.stack(LOG_ID,"Cannot resolve the local Wear node for $path from $sourceId",th)
                null
            }
            if(localMirror==null&&path==MessageSender.NET_PATH) {
                // The phone's node ID is not a valid mirror label on a watch.
                // Waiting for local-node discovery is safer than creating a
                // row named after the phone and mutating it with this payload.
                Log.e(LOG_ID,"Ignoring Wear /netinfo from $sourceId until the local watch node is available")
                MessageSender.sendnetinfo()
                return
            }
            localMirror?:sourceId
        }
        else sourceId
        receiveMessage(sourceId,connectionName,path,data,galaxy,false)
      }

 companion object {
   private const val LOG_ID = "MessageReceiver"
   private const val offbyte:Byte=0

   /** Entry point used by the Google-independent BLE GATT transport. */
   @JvmStatic
   fun receiveBle(linkId:String,path:String,data:ByteArray,remoteIsWearable:Boolean) {
       receiveMessage(linkId,linkId,path,data,remoteIsWearable,true)
       }

   private fun receiveMessage(sourceId:String,connectionName:String,path:String,data:ByteArray,galaxy:Boolean,fromBle:Boolean) {
        Log.i(LOG_ID,"receiveMessage start $path via ${if(fromBle) "BLE" else "MessageClient"}")
        if(fromBle&&MessageSender.isWearControlPath(path)&&!BleMirror.canSendWearControl(connectionName)) {
            Log.e(LOG_ID,"Rejected Wear control $path outside an authenticated Wear mirror: $connectionName")
            return
        }
        when(path) {
            MessageSender.DEFAULTS_PATH ->  {
                val sender = tk.glucodata.MessageSender.getMessageSender()
                 if(doLog) {Log.i(LOG_ID,"path==MessageSender.DEFAULTS_PATH $connectionName" );}
                  setWearosdefaults(connectionName,true);
                   val context=if(MainActivity.thisone==null)Applic.app;else MainActivity.thisone;
                   if(Natives.hasAidexX()) {
                        if(sender==null) {
                            Log.d(LOG_ID,"messagesender==null")
                            return
                        }
                        unpairWatch(context,sourceId,sender);
                        }
                     else
                       Applic.setbluetooth(context,false)
                 }
            MessageSender.WAKE_PATH -> {
                Natives.wakehereonly()
                }
            MessageSender.WAKESTREAM_PATH -> {
                Natives.wakestreamhereonly()
            }
            MessageSender.DATA_PATH   -> {
                if(fromBle) {
                    val localIndex=BleMirror.hostIndex(connectionName)
                    if(localIndex<0)
                        Log.e(LOG_ID,"No local mirror row for authenticated BLE link $connectionName")
                    else if(!Natives.messageForMirror(localIndex,data))
                        Log.e(LOG_ID,"Native mirror bridge rejected BLE data for $connectionName ($localIndex)")
                }
                else if(!Natives.message(data)) {
                    Log.e(LOG_ID,"Native MessageClient bridge rejected /data from $connectionName")
                    if(isWearable) {
                        // A BLE->MessageClient fallback can leave the native
                        // peer-index mapping stale. /netinfo is the authoritative
                        // Wear mapping exchange, so repair it immediately instead
                        // of waiting for a manual Reinit.
                        Log.i(LOG_ID,"Requesting /netinfo after rejected Wear MessageClient data")
                        sendnetinfo(sourceId)
                    }
                }
            }
            MessageSender.NET_PATH   -> {
                if(fromBle&&!BleMirror.canSendWearControl(connectionName)) {
                    // Phone-to-phone mirrors keep their explicitly configured
                    // direction. BLE may only update an existing Wear row.
                    Log.e(LOG_ID,"Rejected BLE /netinfo outside an authenticated Wear mirror: $connectionName")
                }
                else {
                    Log.i(LOG_ID,"Accepting Wear /netinfo through ${if(fromBle) "BLE" else "MessageClient"}: source=$sourceId mirror=$connectionName")
                    // On a phone the source Wear node ID is the mirror label;
                    // on a watch the mirror label is that watch's local node
                    // ID. For BLE, connectionName is the authenticated label.
                    // Native code verifies the embedded label before mutation.
                    val mirrorLabel=Natives.setmynetinfo(connectionName,data,galaxy,false)
                    if(mirrorLabel!=null) {
                        if(mirrorLabel!=connectionName) {
                            Log.e(LOG_ID,"Native /netinfo result mismatch: expected=$connectionName returned=$mirrorLabel source=$sourceId")
                        }
                        else {
                            Log.i(LOG_ID,"Wear mirror created or updated from /netinfo: label=$mirrorLabel source=$sourceId")
                            if(fromBle) {
                                // A BLE label is not a MessageClient node ID.
                                // Never replace the watch -> phone node mapping.
                                MessageSender.sendBleNetInfo(connectionName)
                            }
                            else {
                                MessageSender.rememberMirrorNode(connectionName,sourceId)
                                sendnetinfo(sourceId)
                            }
                            MessageSender.peerNetworkInfoChanged()
                        }
                    }
                    else
                        Log.e(LOG_ID,"Wear /netinfo was rejected: source=$sourceId bytes=${data.size}")
                }
            }
            MessageSender.START_PATH ->  {
               if(isWearable)
                  UseWifi.usewifi()
               val context=Applic.getContext()
               Applic.setinittext(context.getString(R.string.connected));
               Applic.initStarted=Natives.ontbytesettings(data)
               Notify.mkunitstr(context,Natives.getunit())
               if(fromBle)
                   MessageSender.sendBleNetInfo(connectionName)
               else
                   sendnetinfo(sourceId)
            }
             MessageSender.SETTINGS_PATH   -> { //Never used
                 Natives.ontbytesettings(data)
                    Notify.mkunitstr(Applic.app,Natives.getunit())
                }
             MessageSender.MESSAGES_PATH -> {
                val on=booldata(data)
                val automaticBle=fromBle&&BleMirror.isAutomaticPhonePeer(connectionName)
                val accepted=if(automaticBle)
                    BleMirror.canSetAutomaticCarrier(connectionName,on)
                else {
                    Natives.setBlueMessage(connectionName,on)
                    true
                }
                if(fromBle) {
                    val ack=data.copyOf(9.coerceAtMost(data.size))
                    if(ack.isNotEmpty()&&!accepted)
                        ack[0]=if(on) 0.toByte() else 1.toByte()
                    // Queue /messagesack before enabling the native BLE bridge.
                    // The old order could let /data overtake the ACK and get
                    // rejected by the requesting peer before it selected BLE.
                    val ackQueued=BleMirror.sendAsync(connectionName,MessageSender.MESSAGES_ACK_PATH,ack)
                    val activated=if(automaticBle&&accepted&&ackQueued)
                        BleMirror.setAutomaticCarrier(connectionName,on)
                    else
                        !automaticBle||!accepted
                    Log.i(LOG_ID,"BLE carrier request for $connectionName: requested="+
                            "${if(on) "Bluetooth" else "TCP/IP"} accepted=$accepted ackQueued=$ackQueued activated=$activated")
                }
                else
                    MessageSender.sendMessagesAck(sourceId,data)
                }
             MessageSender.MIRROR_TRANSPORT_PATH -> {
                MessageSender.receiveMirrorTransport(sourceId,connectionName,data,fromBle)
             }
             MessageSender.MIRROR_TRANSPORT_ACK_PATH -> {
                if(fromBle)
                    MessageSender.receiveMirrorTransportAck(connectionName,data)
             }
             MessageSender.MESSAGES_ACK_PATH -> {
                MessageSender.receiveMessagesAck(connectionName,data,fromBle)
                }
             MessageSender.BLUETOOTH_PATH -> {
               if(isWearable) {
                    val context=if(MainActivity.thisone==null)Applic.app;else MainActivity.thisone;
                    val on=booldata(data)
                    if(tk.glucodata.Log.doLog) {Log.i(LOG_ID,"set bluetooth $on");}
                    if(!on&&Natives.hasAidexX()) {
                            val sender = tk.glucodata.MessageSender.getMessageSender()
                            if (sender == null) {
                                Log.d(LOG_ID, "5: messagesender==null")
                                return
                            }
                            unpairWatch(context,sourceId,sender);
                            }
                         else
                           Applic.setbluetooth(context,on)
                        }

                }
             MessageSender.UNPAIR_PATH -> {
             if(!isWearable) {
                        val context=MainActivity.thisone;
                        if(context==null) {
                            Log.i(LOG_ID,"unpair no MainActivity")
                            return
                            }
                        val on=booldata(data)
                        if(tk.glucodata.Log.doLog) {Log.i(LOG_ID,"set unpair $on");}
                        val unpair=context.unpairer;
                        if(unpair!=null) {
                            val mess=context.getString(R.string.unpairingwatch) +context.getString(if(on) R.string.successful else R.string.failed)
                            Log.i(LOG_ID,mess)
                            unpair.postFinished(mess, on);
                            unpair.postCloser();
                            context.unpairer=null;
                            var doswitch=context.doswitch;
                            if(doswitch!=null) {
                                doswitch.run();
                                context.doswitch=null;
                                }
                            }
                        }
                }
             MessageSender.ASKFORSTART_PATH -> {
                 if(!isWearable) {
                     if(fromBle) {
                         MessageSender.sendStartBle(sourceId)
                     }
                     else {
                         val sender = tk.glucodata.MessageSender.getMessageSender()
                         if(sender==null) {
                             Log.d(LOG_ID,"3: messagesender==null")
                             return
                         }
                         val node=sender.nodes?.firstOrNull { it.id==sourceId }
                         if(node==null) {
                             Log.e(LOG_ID,"Can't find source node $sourceId")
                             MessageSender.scope.launch { sender.findWearDevicesWithApp() }
                             return
                         }
                         Wearos.sendinitwatchapp(node)
                     }
                 }
               }
        }
        Log.i(LOG_ID,"receiveMessage end $path")
       }

    fun booldata(data:ByteArray):Boolean {
        return data.isNotEmpty()&&data[0]!=offbyte
        }
       }
   }
