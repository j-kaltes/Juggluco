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
/*      Fri Jan 27 15:32:56 CET 2023                                                 */


package tk.glucodata;

import android.app.Application;

import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.Natives.hasalarmloss;

public class GlucoseAlarms extends SuperGlucoseAlarms {
    final private static String LOG_ID="GlucoseAlarms";
public GlucoseAlarms(Application context) {
	super(context);
	}

/*
public	void handlealarm() {
	SensorBluetooth.reconnectall();
	final long nu = System.currentTimeMillis();
	var view=Floating.floatview;
	if(view!=null) {
		view.postInvalidate();
		}

	final long lasttime=Natives.lastglucosetime( );
	final long nexttime=lasttime+waitmmsec();
	if(nu>nexttime) {
		if(hasalarmloss()) {
			Notify.onenot.lossalarm(lasttime);
			}
		}
	MessageSender.sendwakestream();
	}
	*/

public	void handlealarm() {
		SensorBluetooth.reconnectall();
		final long nu = System.currentTimeMillis();
		final var view=Floating.floatview;
		if(view!=null) {
			view.postInvalidate();
			}
		 tk.glucodata.glucosecomplication.GlucoseValue.updateall();

		long wastime = MyGattCallback.oldtime - showtime;

		final long tryagain = nu + showtime;
		long nexttime=tryagain;

		if(hasalarmloss())  {
			final long afterwait = waitmmsec() + wastime;
			if(afterwait > nu) {
				Log.i(LOG_ID, "handlealarm notify");
				nexttime = (afterwait < tryagain)  ? afterwait : tryagain;
			} else {
				if(!saidloss) {
					Log.i(LOG_ID, "handlealarm alarm");
					long lasttime=Natives.lastglucosetime( );
					if(lasttime!=0L)
						wastime=lasttime;
					Notify.onenot.lossalarm(wastime);
					saidloss = true;
					}
			}
			}
		LossOfSensorAlarm.setalarm(Applic.app, nexttime);
		MessageSender.sendwakestream();
		Natives.wakestreamsender();
	}


}
