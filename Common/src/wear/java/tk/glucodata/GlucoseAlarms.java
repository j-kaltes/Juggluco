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

import static tk.glucodata.Natives.hasalarmloss;

public class GlucoseAlarms extends SuperGlucoseAlarms {
    final private static String LOG_ID="GlucoseAlarms";
public GlucoseAlarms(Application context) {
	super(context);
	}


public	void handlealarm() {
	final long nu = System.currentTimeMillis();
	SensorBluetooth.reconnectall();
	final long lasttime=Natives.lastglucosetime( );
	final long nexttime=lasttime+waitmmsec();
	var view=Floating.floatview;
	if(view!=null) {
		view.postInvalidate();
		}
	if(nu>nexttime) {
		if(hasalarmloss()) {
			Notify.onenot.lossalarm(lasttime);
			}
		}
	MessageSender.sendwakestream();
	}

}
