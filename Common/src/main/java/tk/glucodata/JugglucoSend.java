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


package tk.glucodata;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;

import static tk.glucodata.Natives.getxDripTrendName;

public class JugglucoSend   {
    public static final String ACTION = "glucodata.Minute";
    private static final String SERIAL = "glucodata.Minute.SerialNumber";
    private static final String MGDL = "glucodata.Minute.mgdl";
	private static final String GLUCOSECUSTOM = "glucodata.Minute.glucose";
	private static final String RATE = "glucodata.Minute.Rate";
    private static final String ALARM = "glucodata.Minute.Alarm";
    private static final String TIME = "glucodata.Minute.Time";
private static final String LOG_ID="JugglucoSend";

private static Bundle mkGlucosebundle(String SerialNumber, int mgdl, float gl, float rate, int alarm, long timmsec) {
      Bundle extras = new Bundle();
        extras.putString(SERIAL,SerialNumber);
	extras.putInt(MGDL,mgdl);
	extras.putFloat(GLUCOSECUSTOM,gl);
        extras.putFloat(RATE,rate);
        extras.putInt(ALARM,alarm);
        extras.putLong(TIME,timmsec);
	return extras;
	  }

private static String[] names=null;
public static  void setreceivers() {
	names=Natives.glucodataRecepters();
	}
static void broadcastglucose(String SerialNumber, int mgdl, float gl, float rate, int alarm, long timmsec) {
	if(names==null)
		return;
	Log.i(LOG_ID,"broadcastglucose "+gl+" rate="+rate);
        final Context context=Applic.app;
        Intent intent = new Intent(ACTION);
	intent.putExtras(mkGlucosebundle(SerialNumber, mgdl, gl, rate, alarm,timmsec));
	intent.addFlags(Intent.FLAG_INCLUDE_STOPPED_PACKAGES);
	for(var name:names) {
		if(name!=null) {
		      Log.i(LOG_ID,name);
	      	      intent.setPackage(name);
		      context.sendBroadcast(intent);
		      }
	   	}
	}
	/*
static void broadcastglucose(String SerialNumber, int mgdl, float gl, float rate, int alarm, long timmsec) {
	Log.i(LOG_ID,"broadcastglucose "+gl+" rate="+rate);
        final Context context=Applic.app;
        Intent intent = new Intent(ACTION);
	intent.putExtras(mkGlucosebundle(SerialNumber, mgdl, gl, rate, alarm,timmsec));
	intent.addFlags(Intent.FLAG_INCLUDE_STOPPED_PACKAGES);
   	var receivers = context.getPackageManager().queryBroadcastReceivers(intent, 0);
	   for(var resolveInfo : receivers) {
	   	String name=resolveInfo.activityInfo.packageName;
		if(name!=null) {
			Log.i(LOG_ID,name);
	      		intent.setPackage(name);
		      context.sendBroadcast(intent);
		      }
	   	}
	}*/
	
}
