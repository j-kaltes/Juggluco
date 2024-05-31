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
/*      Fri Jan 27 15:32:11 CET 2023                                                 */


package tk.glucodata;

//import androidx.annotation.Keep;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;

import androidx.annotation.Keep;

import org.json.JSONArray;
import org.json.JSONObject;


public class EverSense {
private static final String LOG_ID="EverSense";
private static String[] names=null;
public static  void setreceivers() {
	names=Natives.everSenseRecepters();
	}

 private static void sendIntent(Context context,Intent intent) {
	intent.addFlags(Intent.FLAG_INCLUDE_STOPPED_PACKAGES);
	for(var name:names) {
		intent.setPackage(name);
		context.sendBroadcast(intent);
		Log.i(LOG_ID,"send to "+name);
		}
    }

public static final String glucoseaction="com.eveningoutpost.dexdrip.NS_EMULATOR";
@Keep
static void broadcastglucose(int mgdl, float rate, long timmsec) {
    try {
        final JSONArray sgv_array = new JSONArray();
        final JSONObject sgv_object = new JSONObject();
        sgv_object.put("type", "sgv");
        sgv_object.put("date", timmsec);
        sgv_object.put("sgv", mgdl);
        sgv_object.put("direction", tk.glucodata.Natives.getxDripTrendName(rate));
        sgv_array.put(sgv_object);

        final var bundle = new Bundle();

        bundle.putString("data", sgv_array.toString());

        bundle.putString("collection", "entries");
        final Intent intent = new Intent(glucoseaction);
        intent.putExtras(bundle);
        sendIntent(Applic.app, intent);
    } catch(Throwable th) {
            Log.stack(LOG_ID,"broadcastglucose",th);
        }
    }


}
