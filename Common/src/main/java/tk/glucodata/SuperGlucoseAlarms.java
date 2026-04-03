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

//static final long showtime = Notify.glucosetimeout;


public	static long waitmmsec() {
	final short minutes = Natives.readalarmsuspension(4);
	final long mmsec = (minutes * 60 - 20) * 1000L;
	return mmsec;
	}


//boolean saidloss = false;

public  void setLossAlarm() {
    if(hasalarmloss()) {
         //saidloss = false;
         final long nu = System.currentTimeMillis();
         SuperGattCallback.lastfoundL=nu;
//         Libre2GattCallback.oldtime = nu + Notify.glucosetimeout;
         LossOfSensorAlarm.setalarm(Applic.app, nu + waitmmsec());
       }
}

public void setagealarm(final long numsec,long showtime) {
    Notify.stoplossalarm();

    //saidloss = false;
    SuperGattCallback.lastfoundL=numsec;
   // Libre2GattCallback.oldtime = numsec + showtime;
    var oldtime = numsec + showtime;
    LossOfSensorAlarm.setalarm(Applic.app, oldtime);
    }



		}
