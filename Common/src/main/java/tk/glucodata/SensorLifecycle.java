/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2(+), Libre 3(+), Dexcom G7/ONE+,        */
/*      Sibionics GS1Sb and GS3, Accu-Chek SmartGuide, CareSens Air and              */
/*      Aidex X sensors.                                                             */
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
/*      Thu Oct 15 12:48:42 CEST 2026                                                */

package tk.glucodata;

import android.content.Context;
import android.content.SharedPreferences;

/** Sensor lifecycle notifications shared by the phone and Wear builds. The
 * native active-sensor list owns end/use-again state, including across restart.
 * Remember only the last explicit addition as a candidate preference; a Java
 * notification or an old preference must never override native activity. */
public final class SensorLifecycle {
    public interface Listener {
        void changed();
        /** A UI finish action must update local settings before it returns. */
        void ended();
    }
    private static volatile Listener listener;
    private SensorLifecycle() {}

    private static SharedPreferences prefs() {
        return Applic.app.getSharedPreferences("sensor_lifecycle", Context.MODE_PRIVATE);
    }

    public static void listen(Listener callback) {
        listener=callback;
        changed();
    }

    public static void changed() {
        Listener callback=listener;
        if(callback!=null) callback.changed();
    }

    public static void ended(String serial) {
        if(serial==null) return;
        // Remove the obsolete v40/v41 marker. Native finish happens before
        // this notification; a later native reactivation must remain usable.
        prefs().edit().remove("ended:"+serial).apply();
        Listener callback=listener;
        if(callback!=null) callback.ended();
    }

    public static void added(String serial) {
        if(serial==null) return;
        prefs().edit().remove("ended:"+serial).putString("added",serial).apply();
        changed();
    }

    /** Sensors.useAgain passes SensorGlucoseData*, not the owning dataptr. */
    public static void added(long sensorptr) {
        String[] active=Natives.activeSensors();
        if(active!=null) for(String serial:active) {
            if(serial==null) continue;
            long ptr=Natives.getdataptr(serial);
            if(ptr==0L) continue;
            try {
                if(Natives.getsensorptr(ptr)==sensorptr) {
                    added(serial);
                    return;
                }
            } finally { Natives.freedataptr(ptr); }
        }
        changed();
    }

    public static String lastAdded() { return prefs().getString("added",null); }
}
