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
/*      Thu Mar 23 21:03:54 CET 2023                                                 */



package tk.glucodata;

import static android.graphics.Color.WHITE;

import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.os.BadParcelableException;
import android.os.BatteryManager;
import android.os.Bundle;
import android.os.IBinder;
import android.os.Parcelable;
import android.os.PowerManager;
import android.preference.PreferenceManager;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.HashMap;
import java.util.Locale;
import java.util.Map;

import static tk.glucodata.Natives.getxDripTrendName;

import com.eveningoutpost.dexdrip.services.broadcastservice.models.GraphLine;
import com.eveningoutpost.dexdrip.services.broadcastservice.models.Settings;

import tk.glucodata.Applic;

public class  WearInt  {
private static final String LOG_ID="WearInt";
private static final String ACTION_WATCH_COMMUNICATION_SENDER = "com.eveningoutpost.dexdrip.watch.wearintegration.BROADCAST_SERVICE_SENDER";
static private final long oldvalue=1000*60*5L;
static final Map<String, Settings> mapsettings=new HashMap<>();
/*
private static int getBatteryLevel() { //From xDrip

        final Intent batteryIntent = Applic.app.registerReceiver(null, new IntentFilter(Intent.ACTION_BATTERY_CHANGED));
        try {
            int level = batteryIntent.getIntExtra(BatteryManager.EXTRA_LEVEL, -1);
            int scale = batteryIntent.getIntExtra(BatteryManager.EXTRA_SCALE, -1);
            if (level == -1 || scale == -1) {
                Log.e(LOG_ID,"level="+level+" scale="+scale);
                return 50;
            }
            int perc= (int) (((float) level / (float) scale) * 100.0f);
            Log.i(LOG_ID,"BatteryLevel="+perc);
            return perc;
        } catch (Throwable error) {
                        String mess=error!=null?error.getMessage():null;
                        Log.e(LOG_ID,"getBatteryLevel "+mess);

            return 50;
                }
        } 

*/
static void alarm(String value ) {
        Intent intent = new Intent(ACTION_WATCH_COMMUNICATION_SENDER);
	Bundle bundle=new Bundle();
	bundle.putString("FUNCTION","alarm");
	bundle.putString("type","BG_ALERT_TYPE");
	bundle.putString("message",value);
	intent.putExtras(bundle);
        intent.addFlags(Intent.FLAG_INCLUDE_STOPPED_PACKAGES);
	mapsettings.forEach((pack, settings) -> {
		intent.setPackage(pack);
		Applic.app.sendBroadcast(intent);
		});

	}

static void missingalarm(long timmsec)  {
        Intent intent = new Intent(ACTION_WATCH_COMMUNICATION_SENDER);
	Bundle bundle=new Bundle();
	bundle.putString("FUNCTION","alarm");
	bundle.putString("type","bg_missed_alerts");
	SimpleDateFormat df = new SimpleDateFormat("HH:mm", Locale.US);
	String time = df.format(new Date(timmsec));
	bundle.putString("message","BG Readings Missed  (@"+time+")");
	intent.putExtras(bundle);
        intent.addFlags(Intent.FLAG_INCLUDE_STOPPED_PACKAGES);
	mapsettings.forEach((pack, settings) -> {
		intent.setPackage(pack);
		Applic.app.sendBroadcast(intent);
		});

    	}

static Intent mksendglucoseintent(Settings settings,int mgdl,float rate, int alarm, long timmsec)  {
        Intent intent = new Intent(ACTION_WATCH_COMMUNICATION_SENDER);
	Bundle bundle=new Bundle();
//	bundle.putInt("phoneBattery",getBatteryLevel()); //Who needs that?
        bundle.putDouble("bg.valueMgdl", mgdl);
        bundle.putDouble("bg.deltaValueMgdl", (double)rate/60000.0);
        bundle.putString("bg.deltaName",getxDripTrendName(rate));
        bundle.putLong("bg.timeStamp", timmsec);
	final var now=System.currentTimeMillis();
        bundle.putBoolean("bg.isStale", (now-timmsec)>oldvalue);
        bundle.putBoolean("doMgdl", Applic.unit!=1);
	boolean ishigh=false,islow=false;  
	if((alarm&4)==4) {
		if((alarm&1)==1)
			islow=true;	
		else
			ishigh=true;	
		}
        bundle.putBoolean("bg.isHigh", ishigh);
        bundle.putBoolean("bg.isLow", islow);
//	bundle.putString("pumpJSON","{\"reservoir\":-1,\"bolusiob\":-1,\"battery\":-1}");
	bundle.putString("pumpJSON","{}");
//	bundle.putLong("treatment.timeStamp",timmsec);
    	if(settings.isDisplayGraph()) addgraph(settings,bundle,now);
	intent.putExtras(bundle);
        intent.addFlags(Intent.FLAG_INCLUDE_STOPPED_PACKAGES);
	return intent;
	}
private static final int FUZZER = 30000;
static private void addgraph(Settings settings,Bundle bundle,long now) {
	long graphStartOffset = settings.getGraphStart();
	if(graphStartOffset == 0) {
	    graphStartOffset = 60*60*1000 * 2;
		}
//	long end = now +settings.getGraphEnd(); //much too large with small periods
	long end = now + graphStartOffset/15;
	long start = now - graphStartOffset;
	bundle.putInt("fuzzer", FUZZER);
	bundle.putLong("start", start);
	bundle.putLong("end", end);
	float targetlow= Natives.targetlow();
	float targethigh= Natives.targethigh();
	bundle.putDouble("lowMark", targetlow); 
	bundle.putDouble("highMark", targethigh);

	float startfuz=start/FUZZER;
	float endfuz=end/FUZZER;
	final int highcolor=-17613;
	final int lowcolor=-3995383;
	var lowline=new GraphLine(lowcolor);
	lowline.add(startfuz,targetlow);
	lowline.add(endfuz,targetlow);
	bundle.putParcelable("graph.lowLine", lowline);
	var highline=new GraphLine(highcolor);
	highline.add(startfuz,targethigh);
	highline.add(endfuz,targethigh);
	bundle.putParcelable("graph.highLine", highline);
	var low=new GraphLine(lowcolor);
//	int inrangecolor=-13388314;
	int inrangecolor=WHITE;
	var inrange=new GraphLine(inrangecolor);
	var high=new GraphLine(highcolor);
	if(!Natives.mkWearIntgraph(start,end,low,inrange,high)) {
		Log.e(LOG_ID,"Natives.mkWearIntgraph(start,end,low,inrange,high)==false");
		}

	bundle.putParcelable("graph.low", low);
	bundle.putParcelable("graph.inRange", inrange);
	bundle.putParcelable("graph.high", high);
//	bundle.putParcelable("graph.iob", new GraphLine()); //ihhhnsulin on board
//	bundle.putParcelable("graph.treatment", new GraphLine()); //treatmentValues

//	bundle.putParcelable("graph.predictedBg", new GraphLine());
//	bundle.putParcelable("graph.cob", new GraphLine());  //cobValues
//	bundle.putParcelable("graph.polyBg", new GraphLine());  //poly predict ;
            }
static void sendglucose(int mgdl,float rate, int alarm, long timmsec)  {
	mapsettings.forEach((pack, settings) -> {
		var intent=mksendglucoseintent(settings,mgdl,rate,alarm, timmsec);
		intent.putExtra( "FUNCTION","update_bg");
		intent.setPackage(pack);
		Applic.app.sendBroadcast(intent);
		});

    	}
}
