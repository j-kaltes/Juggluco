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

import androidx.annotation.Keep;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;

/* loaded from: /tmp/jadx-14638048635823846283.dex */
public class XInfuus {

//@Keep
public static void sendGlucoseBroadcast(String serial, double currentGlucose,float rate,long mmsec) {
    }

public static final String glucoseaction="";

    public static void sendSensorActivateBroadcast(Context context,String serial,long startsec) {
    } 

static public void setlibrenames() {
        }


}
