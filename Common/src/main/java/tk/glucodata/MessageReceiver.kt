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

import android.content.Intent
import com.google.android.gms.wearable.*
import tk.glucodata.Applic.isWearable
import tk.glucodata.MainActivity.setbluetoothon
//import tk.glucodata.MessageSender.Companion.messagesender
import tk.glucodata.MessageSender.Companion.sendnetinfo
import tk.glucodata.Natives

class MessageReceiver: WearableListenerService() {
    override fun onMessageReceived(messageEvent: MessageEvent) {
        super.onMessageReceived(messageEvent)
	val data= messageEvent.getData();
        val path= messageEvent.path
        Log.i(LOG_ID,"onMessageReceived start $path"  )
        when(path) {
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
		val sender=tk.glucodata.MessageSender.getMessageSender()
		if(sender==null) {
			Log.d(LOG_ID,"messagesender==null")
			return
			}
	    	val sourceId= messageEvent.getSourceNodeId()
		val name:String=(if(isWearable) sender.localnode; else sourceId)?:return
		if(Natives.setmynetinfo(name,data)) {
			sendnetinfo(sourceId)
                  }
		  }
		MessageSender.START_PATH ->  {
		  if(isWearable)
			   UseWifi.usewifi()
		   Natives.ontbytesettings(data)
		   Notify.mkunitstr(Natives.getunit())
		sendnetinfo(messageEvent.getSourceNodeId())
		}
		 MessageSender.SETTINGS_PATH   -> {
			 Natives.ontbytesettings(data)
        		Notify.mkunitstr(Natives.getunit())
			}
		 MessageSender.MESSAGES_PATH -> {
			 val sender=tk.glucodata.MessageSender.getMessageSender()
			 if(sender==null) {
				 Log.d(LOG_ID,"2: messagesender==null")
				 return
			 }
			 val sourceId= messageEvent.getSourceNodeId()
			 val name:String=(if(isWearable) sender.localnode; else sourceId)?:return
			val on=booldata(data)
			Natives.setBlueMessage(name,on)
		 	}
		 MessageSender.BLUETOOTH_PATH -> {
		 	val context=if(MainActivity.thisone==null)Applic.app;else MainActivity.thisone;
			val on=booldata(data)
			Log.i(LOG_ID,"set bluetooth $on  ${data[0]}");
			Applic.setbluetooth(context,on )
		 	}
	    }
        Log.i(LOG_ID,"onMessageReceived end $path"  )
      }

    companion object {
        private const val LOG_ID = "MessageReceiver"
	private const val offbyte:Byte=0
	fun booldata(data:ByteArray):Boolean {
		return data[0]!=offbyte
		}
       }
   }
