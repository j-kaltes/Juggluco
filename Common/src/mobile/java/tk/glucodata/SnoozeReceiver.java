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

package tk.glucodata;

import static tk.glucodata.Log.doLog;

import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

/**
 * BroadcastReceiver that handles snooze notification actions on the alarm
 * notification.  Each action carries the snooze duration (minutes) as an
 * intent extra.  On receipt, sets the snooze and stops the active alarm sound.
 *
 * Inspired by GlucoDataHandler's notification action pattern.
 */
public class SnoozeReceiver extends BroadcastReceiver {

    static final String ACTION_SNOOZE  = "tk.glucodata.SNOOZE_ALARM";
    static final String EXTRA_MINUTES  = "snooze_minutes";

    private static final String LOG_ID = "SnoozeReceiver";

    @Override
    public void onReceive(Context context, Intent intent) {
        Applic app = (Applic) context.getApplicationContext();
        app.initproc();
        if (ACTION_SNOOZE.equals(intent.getAction())) {
            final long minutes = intent.getLongExtra(EXTRA_MINUTES, 60L);
            {if(doLog) {Log.i(LOG_ID, "snooze " + minutes + " min from notification action");}}
            AlarmSnooze.set(minutes);
            Notify.stopalarm();
        }
    }

    /** Build a PendingIntent that snoozes for {@code minutes} minutes. */
    static PendingIntent pendingSnooze(long minutes, int requestCode) {
        final Intent intent = new Intent(Applic.app, SnoozeReceiver.class);
        intent.setAction(ACTION_SNOOZE);
        intent.putExtra(EXTRA_MINUTES, minutes);
        return PendingIntent.getBroadcast(
                Applic.app, requestCode, intent,
                PendingIntent.FLAG_UPDATE_CURRENT | Notify.penmutable);
    }
}
