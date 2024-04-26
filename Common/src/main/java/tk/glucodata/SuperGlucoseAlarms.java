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

import android.app.Application;

import static tk.glucodata.Natives.hasalarmloss;

public class SuperGlucoseAlarms {
    final private static String LOG_ID="SuperGlucoseAlarms";
public SuperGlucoseAlarms(Application context) {
	Notify.init(context);
	}

static final long showtime = Notify.glucosetimeout;


public	static long waitmmsec() {
	final short minutes = Natives.readalarmsuspension(4);
	final long mmsec = (minutes * 60 - 20) * 1000L;
	return mmsec;
	}


 boolean saidloss = false;
/*
public	void handlealarm() {
		final long nu = System.currentTimeMillis();
		long wastime = MyGattCallback.oldtime - showtime;
		final long afterwait = waitmmsec() + wastime;
		boolean shouldwake = Natives.shouldwakesender();
		final long tryagain = nu + showtime;
		if(afterwait > nu) {
			Log.i(LOG_ID, "handlealarm notify");
			Notify.onenot.oldnotification(wastime);
			long nexttime = (!shouldwake || (afterwait < tryagain && hasalarmloss())) ? afterwait : tryagain;
			LossOfSensorAlarm.setalarm(Applic.app, nexttime);
		} else {
			if (shouldwake)
				LossOfSensorAlarm.setalarm(Applic.app, tryagain);
			if (!saidloss) {
				Log.i(LOG_ID, "handlealarm alarm");
				long lasttime=Natives.lastglucosetime( );
				if(lasttime!=0L)
					wastime=lasttime;
				if (hasalarmloss()) {
					Notify.onenot.lossalarm(wastime);
				}
				saidloss = true;
			}
		}
		if (shouldwake)
			Natives.wakestreamsender();
	}
*/
	public  void sensorinit() {
		if (hasalarmloss()) {
			Notify.shownovalue();
			saidloss = false;
			final long nu = System.currentTimeMillis();
			MyGattCallback.oldtime = nu + showtime;
			LossOfSensorAlarm.setalarm(Applic.app, nu + waitmmsec());
		}
	}

public void setagealarm(final long numsec) {
		saidloss = false;
		MyGattCallback.oldtime = numsec + showtime;
		LossOfSensorAlarm.setalarm(Applic.app, MyGattCallback.oldtime);
	}



		}
