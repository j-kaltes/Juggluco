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
/*      Thu Mar 23 21:03:49 CET 2023                                                 */


package tk.glucodata;
import static tk.glucodata.Log.doLog;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;

import com.eveningoutpost.dexdrip.services.broadcastservice.models.Settings;

public class watchdrip extends BroadcastReceiver {
private static String  LOG_ID="watchdrip";

private static   String  tostring(Bundle bundle) {
        if(bundle==null)
            return "";
        var builder = new StringBuilder();
        builder.append("bundle content\n");
        var keys = bundle.keySet();
        for(var key: keys) {
            builder.append("[" + key + "]<->[" + bundle.get(key) + "]\n");
        }
        return builder.toString();
    } 
        @Override
        public void onReceive(Context context, Intent intent) {
           	Log.i(LOG_ID,"onReceive ");
		var extras=intent.getExtras();
		if(doLog) Natives.log(tostring(extras));
		var function=extras.getString("FUNCTION","");
		if("update_bg_force".equals(function)) {
			String key=extras.getString("PACKAGE",null);
			if(key==null) {
				Log.e(LOG_ID,"no package");
				return;
				}
//			WearInt.settings  = intent.getParcelableExtra("SETTINGS");
			Settings settings= extras.getParcelable("SETTINGS");
			if(settings==null) {
					Log.e(LOG_ID,"settings==null");
					return;
				}
			WearInt.mapsettings.put(key,settings);
			var gl=Natives.getlastGlucose();
			if(gl==null)
					return;
			long res=gl[1];
			int glumgdl = (int) (res & 0xFFFFFFFFL);
			if (glumgdl != 0) {
				int alarm = (int) ((res >> 48) & 0xFFL);
				short ratein = (short) ((res >> 32) & 0xFFFFL);
				float rate = ratein / 1000.0f;
				var newintent=WearInt.mksendglucoseintent(settings,glumgdl,rate,alarm,  gl[0]*1000L);
				newintent.putExtra( "FUNCTION","update_bg_force");
				newintent.setPackage(key);
				Applic.app.sendBroadcast(newintent);
				}
			}

        	}
static private watchdrip receiver=null;
static void register() {
	if(receiver==null)
		receiver=new watchdrip();
	Applic.app.registerReceiver(receiver, new IntentFilter("com.eveningoutpost.dexdrip.watch.wearintegration.BROADCAST_SERVICE_RECEIVER"));
	}
static void unregister() {
	if(receiver!=null)
		Applic.app.unregisterReceiver(receiver);
	}

public static void set(boolean val) {
	SuperGattCallback.doWearInt=val;
	if(val) {
		register();
		}
	else
		unregister();
	}
}
