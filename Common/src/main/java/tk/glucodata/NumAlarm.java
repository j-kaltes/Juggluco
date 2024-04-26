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

import static android.app.AlarmManager.RTC_WAKEUP;
import static android.app.PendingIntent.getBroadcast;
import static android.content.Context.ALARM_SERVICE;

public class NumAlarm extends BroadcastReceiver {
static final private String LOG_ID="NumAlarm";
        @Override
        public void onReceive(Context context, Intent intent) {
	   String action=intent.getAction();
         Applic app=(Applic) context.getApplicationContext();
        app.initproc();
	   Log.i(LOG_ID,"onReceive "+((action!=null)?action:" null"));
	   handlealarm(app);
	   if(action!=null) {
	   	if(Notify.closename.equals(action)) {
				if( ((KeyguardManager) context.getSystemService(Context.KEYGUARD_SERVICE)).isKeyguardLocked()) {
					Log.i(LOG_ID,"Don't kill myself in locked state");
				}else {
					killprogram(0);
					}
			}
		}
//		if(!keeprunning.started&&(Intent.ACTION_BOOT_COMPLETED.equals(action))) 
	if(!keeprunning.started) {
		Applic.possiblybluetooth(context) ;
		}
        }

static void killprogram(int ret) {
		Notify.cancelmessages();
		Activity act=Applic.app.getActivity();
		if(act!=null)
			act.finish();
		keeprunning.stop();
		android.util.Log.i("NumAlarm","closed");
		System.exit(ret);
		}
static void numalarm(Application mApp) {
    	int[] nums=Natives.numAlarmEvents();
    	if(nums==null)
    		return;
	final int len= nums.length;
	if(len==0)
		return;
	ArrayList<String> labels=Natives.getLabels();

	int last=len-1;
	while(nums[last]>=labels.size()) {
		if(--last<0)
			return;
		}

	StringBuilder builder=new StringBuilder();
	int i=0;
	for(;i<last;i++) {
		int index= nums[i];
		if(index<labels.size()) {
			builder.append(labels.get(index));
			builder.append(mApp.getString(R.string.spacedand));
			}
		}
	builder.append(labels.get(nums[i]));
	builder.append(mApp.getString(R.string.notentered));
	Notify.init(mApp);
	Notify.onenot.amountalarm(builder.toString());
	}
static void	   handlealarm(Application context) {
	numalarm(context);
	long first=Natives.firstAlarm();
	if(first>0)
		setalarm(context,first);
	}
static    void setalarm(Context context, long alarmtime) {
        final int alarmrequest = 0;
        Intent alarmintent = new Intent(context, NumAlarm.class);

	final int alarmflags;
	if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.M) {
	   alarmflags = PendingIntent.FLAG_IMMUTABLE;
	}
	else
	   alarmflags = 0;

        PendingIntent onalarm = getBroadcast(context, alarmrequest, alarmintent, alarmflags);
        AlarmManager manager= (AlarmManager) context.getSystemService(ALARM_SERVICE);
//	manager.cancel(onalarm);//is done automatically
        int type=RTC_WAKEUP;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            manager.setExactAndAllowWhileIdle(type,alarmtime,onalarm);
        }
        else
            manager.setExact(type,alarmtime,onalarm);
    }


    }

