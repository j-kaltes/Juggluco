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
import android.app.Activity;
import android.app.AlarmManager;
import android.app.Application;
import android.app.KeyguardManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Build;

import java.util.ArrayList;
import java.util.Calendar;

import static android.app.AlarmManager.RTC_WAKEUP;
import static android.app.PendingIntent.getBroadcast;
import static android.content.Context.ALARM_SERVICE;
import static android.os.SystemClock.elapsedRealtime;

public class Maintenance extends BroadcastReceiver {
static final private String LOG_ID="Maintenance";
        @Override
        public void onReceive(Context context, Intent intent) {
		 Log.i(LOG_ID,"onReceive");
		if(Applic.initproccalled) 
			Natives.maintenance();

        }


static    void setMaintenancealarm(Context context) {
	 Log.i(LOG_ID,"setMaintenancealarm");
        final int alarmrequest = 0;
        Intent alarmintent = new Intent(context, Maintenance.class);

	final int alarmflags;
	if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.M) {
	   alarmflags = PendingIntent.FLAG_IMMUTABLE;
	}
	else
	   alarmflags = 0;

        PendingIntent onalarm = getBroadcast(context, alarmrequest, alarmintent, alarmflags);
        AlarmManager manager= (AlarmManager) context.getSystemService(ALARM_SERVICE);
	Calendar calendar = Calendar.getInstance();
	var nu=System.currentTimeMillis();
	calendar.setTimeInMillis(nu);
	calendar.set(Calendar.HOUR_OF_DAY, 3);
	var gooff=calendar.getTimeInMillis();
	if(gooff<nu)
		gooff+=1000L*60*60*24L;
	manager.setInexactRepeating(AlarmManager.RTC_WAKEUP,gooff, AlarmManager.INTERVAL_DAY, onalarm);
    }


   }
