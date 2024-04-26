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
/*      Sun Mar 10 11:42:56 CET 2024                                                 */


package tk.glucodata;

import static tk.glucodata.Log.stack;
import static tk.glucodata.Notify.penmutable;

import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public class DeleteReceiver extends BroadcastReceiver {
    static private final String clearnotification = "clearnotification";
    static private final String LOG_ID = "DeleteReceiver";

    @Override
    public void onReceive(Context context, Intent intent) {
        Applic app=(Applic) context.getApplicationContext();
        app.initproc();
        final String action = intent.getAction();
        try {
            if (action != null && action.equals(clearnotification)) {
                Log.i(LOG_ID, clearnotification + " Stop Alarm");
                Notify.stopalarm();
            } else
                Log.i(LOG_ID, clearnotification);
        } catch(Throwable th) {
                stack(LOG_ID,"onReceive",th);
        }
    }

    static PendingIntent getDeleteIntent() {
        Intent intent = new Intent(Applic.app, DeleteReceiver.class);
        intent.setAction(clearnotification);
	final int deleterequest = 124;
        return PendingIntent.getBroadcast(Applic.app, deleterequest, intent, PendingIntent.FLAG_UPDATE_CURRENT|penmutable);
    }

}
