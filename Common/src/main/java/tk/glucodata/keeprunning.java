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

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.IBinder;
import android.os.PowerManager;

import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.Log.stack;
//import static tk.glucodata.Natives.getwakelock;

public class keeprunning extends Service {
static boolean started=false;
static keeprunning theservice=null;
static final String LOG_ID="keeprunning";
/*
static void turnonwakelock() {
	PowerManager powerManager = (PowerManager) Applic.app.getSystemService(POWER_SERVICE);
	wakeLock = powerManager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "Juggluco::keeprunning");
	wakeLock.acquire();
	} 
static void	turnoffwakelock() {
	if(wakeLock!=null) {
		wakeLock.release();
		wakeLock=null;
		}
	}


public static void setWakelock(boolean isChecked) {
	 final boolean wakened = Natives.getwakelock();
	 if(isChecked != wakened) {
		 Natives.setwakelock(isChecked);
		 if(isChecked)
			keeprunning.turnonwakelock();
		else
			keeprunning.turnoffwakelock();
		}
	} */

static PowerManager.WakeLock wakeLock =null;
 @Override
  public int onStartCommand(Intent intent, int flags, int startId) {
	if(started) return Service.START_STICKY;//NODIG?
        started=true;
	Applic.app.initproc();
	if(Natives.getfloatglucose()&&!Natives.gethidefloatinJuggluco())
		Floating.makefloat();
  	try {
		if(intent==null) {
			if(!Applic.possiblybluetooth(this)) {
				if(Natives.backuphostNr( )<=0) {
					started=false;
					stopSelf();
					return Service.START_NOT_STICKY;
					}
				}
			}
		theservice=this;
		Notify.foregroundnot(this);
//		if(getwakelock()) turnonwakelock();

	      return Service.START_STICKY;
	     } catch(Throwable e) {
			started=false;
    			stack(LOG_ID,e) ;
			stopSelf();
			return Service.START_NOT_STICKY;
	     	}
  }

  @Override
  public IBinder onBind(Intent intent) {
    return null;
  }
static boolean start(Context context) {
	if(!started) {
		Log.i(LOG_ID,"start keeprunning");
		try {
			Intent i = new Intent(context, keeprunning.class);
			context.startService(i);
			return true;
		} catch (Throwable e) {
			stack(LOG_ID, e);
		}
		}
	return false;
	}
void stopper() {
	stopForeground(true);
	stopSelf();
//	turnoffwakelock();
	Log.i(LOG_ID,"Stopped");
	}
static void stop() {
//	context.stopService(new Intent(context, keeprunning.class));
	LossOfSensorAlarm.cancelalarm();
	if(theservice!=null) {
		theservice.stopper();
		theservice=null;
		started=false;
		}
	}

};
