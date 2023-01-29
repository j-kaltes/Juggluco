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

public class SendLikexDrip   {
    public static final String ACTION = "com.eveningoutpost.dexdrip.BgEstimate";
    private static final String EXTRA_BG_ESTIMATE = "com.eveningoutpost.dexdrip.Extras.BgEstimate";
    private static final String EXTRA_BG_SLOPE_NAME = "com.eveningoutpost.dexdrip.Extras.BgSlopeName";
   private static final String EXTRA_BG_SLOPE = "com.eveningoutpost.dexdrip.Extras.BgSlope";
    private static final String EXTRA_SENSOR_BATTERY = "com.eveningoutpost.dexdrip.Extras.SensorBattery";
    private static final String EXTRA_TIMESTAMP = "com.eveningoutpost.dexdrip.Extras.Time";
private static final String LOG_ID="SendLikexDrip";

private static Bundle mkGlucosebundle(double glucose,float rate,long timmsec) {
      Bundle extras = new Bundle();
	extras.putDouble(EXTRA_BG_ESTIMATE,glucose);
        extras.putString(EXTRA_BG_SLOPE_NAME,getxDripTrendName(rate));
	extras.putDouble(EXTRA_BG_SLOPE,(double)rate/60000.0);
        extras.putLong(EXTRA_TIMESTAMP,timmsec);
//        extras.putInt(EXTRA_SENSOR_BATTERY,0);
	return extras;
	  }

private static String[] names=null;
public static  void setreceivers() {
	names=Natives.xdripRecepters();
	}
static void broadcastglucose(double glucose,float rate,long timmsec) {
	if(names==null)
		return;
	Log.i(LOG_ID,"broadcastglucose "+glucose);
        final Context context=Applic.app;
        Intent intent = new Intent(ACTION);
	intent.putExtras(mkGlucosebundle(glucose,rate,timmsec));
	intent.addFlags(Intent.FLAG_INCLUDE_STOPPED_PACKAGES);
	for(var name:names) {
		if(name!=null) {
			Log.i(LOG_ID,name);
	      		intent.setPackage(name);
		      context.sendBroadcast(intent);
		      }
	   	}
	}
	
}
