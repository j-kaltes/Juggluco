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
import static tk.glucodata.Log.doLog;
import static tk.glucodata.Natives.hasalarmloss;

public class GlucoseAlarms extends SuperGlucoseAlarms {
    final private static String LOG_ID="GlucoseAlarms";
public GlucoseAlarms(Application context) {
    super(context);
    }

public    void handlealarm() {
        SensorBluetooth.reconnectall();
        final long nu = System.currentTimeMillis();
        final var view=Floating.floatview;
        if(view!=null) {
            view.postInvalidate();
            }
        tk.glucodata.glucosecomplication.GlucoseValue.updateall();

        long wastime = SuperGattCallback.lastfoundL;
        if(wastime==0L) {
            wastime=Natives.lastglucosetime();
            }
        final long tryagain = nu + Notify.glucosetimeout;
        long nexttime=tryagain;

        if(hasalarmloss())  {
            final long afterwait = waitmmsec() + wastime;
            if(afterwait > nu) {
                if(doLog) {Log.i(LOG_ID, "handlealarm notify");};
                nexttime = (afterwait < tryagain)  ? afterwait : tryagain;
            } else {
                if(!Natives.lossSignalAlreadyHeard()) {
                    if(doLog) {Log.i(LOG_ID, "handlealarm alarm");};
                    long lasttime=Natives.lastglucosetime( );
                    if(lasttime!=0L)
                        wastime=lasttime;
                    Notify.onenot.lossalarm(wastime);
//                    saidloss = true;
                    }
               else {
                    if(doLog) {Log.i(LOG_ID, "handlealarm saidloss==true");};
                  }
               }
            }
        LossOfSensorAlarm.setalarm(Applic.app, nexttime);
        MessageSender.sendwakestream();
        Natives.wakestreamsender();
    }


}
