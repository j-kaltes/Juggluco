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
        val connectionName=if(isWearable) sender?.localnode?:sourceId else sourceId
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
        when(path) {
            MessageSender.DEFAULTS_PATH ->  {
                val sender = tk.glucodata.MessageSender.getMessageSender()
                 if(doLog) {Log.i(LOG_ID,"path==MessageSender.DEFAULTS_PATH $connectionName" );}
                  setWearosdefaults(connectionName,true);
                   val context=if(MainActivity.thisone==null)Applic.app;else MainActivity.thisone;
                   if(!fromBle&&Natives.hasAidexX()) {
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
                Natives.message(data);
            }
            MessageSender.NET_PATH   -> {
                if(!fromBle&&tk.glucodata.MessageSender.getMessageSender()==null) {
                    Log.d(LOG_ID, "messagesender==null")
                    return
                }
                val phonePeer=fromBle&&!isWearable&&!galaxy
                if(Natives.setmynetinfo(connectionName,data,galaxy,phonePeer)) {
                    sendnetinfo(sourceId)
                    if(fromBle) {
                        Natives.resetMessageConnection(connectionName)
                        BleMirror.restorePreferredTransport(connectionName)
                        }
                }
            }
            MessageSender.START_PATH ->  {
               if(isWearable)
                  UseWifi.usewifi()
               val context=Applic.getContext()
               Applic.setinittext(context.getString(R.string.connected));
               Applic.initStarted=Natives.ontbytesettings(data)
               Notify.mkunitstr(context,Natives.getunit())
               sendnetinfo(sourceId)
            }
             MessageSender.SETTINGS_PATH   -> { //Never used
                 Natives.ontbytesettings(data)
                    Notify.mkunitstr(Applic.app,Natives.getunit())
                }
             MessageSender.MESSAGES_PATH -> {
                val on=booldata(data)
                if(fromBle)
                    BleMirror.setPreferred(on)
                Natives.setBlueMessage(connectionName,on)
                if(fromBle) {
                    BleMirror.sendAsync(MessageSender.MESSAGES_ACK_PATH,data.copyOf(9.coerceAtMost(data.size)))
                    if(on)
                        Natives.resetMessageConnection(connectionName)
                    }
                else
                    MessageSender.sendMessagesAck(sourceId,data)
                }
             MessageSender.MESSAGES_ACK_PATH -> {
                MessageSender.receiveMessagesAck(data)
                }
             MessageSender.BLUETOOTH_PATH -> {
               if(isWearable) {
                    val context=if(MainActivity.thisone==null)Applic.app;else MainActivity.thisone;
                    val on=booldata(data)
                    if(tk.glucodata.Log.doLog) {Log.i(LOG_ID,"set bluetooth $on");}
                    if(!on&&!fromBle&&Natives.hasAidexX()) {
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
