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

/* This file is part of Juggluco, licensed under the GNU GPL, version 3 or later. */
package tk.glucodata;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.os.SystemClock;

import java.util.UUID;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;

import static tk.glucodata.Applic.isWearable;

/**
 * One cancellable recovery deadline for one live sensor callback.
 *
 * When enabled, positive recovery delays use setAlarmClock() on phones and
 * Wear OS alike. No app-executor timer is used to wait for a delayed retry.
 * Zero-delay work is dispatched immediately: Android may clamp an alarm-clock
 * request into the future. Alarms target the existing ConnectReceiver.
 * After process death its normal Application/startup path recreates Bluetooth
 * state; a delivered obsolete alarm is consumed without acting on a new session.
 *
 * The action runs outside this object's monitor. Its owner must also validate a
 * session/deadline generation under its own lock, since cancel() may race an
 * action that has already been extracted for delivery.
 */
final class GattRecoveryAlarm {
    private static final String LOG_ID = "GattRecoveryAlarm";
    private static final String ACTION_PREFIX = "tk.glucodata.LIBRE3_RECOVERY.";
    private static final ConcurrentHashMap<String, Runnable> deliveries = new ConcurrentHashMap<>();
    private final String serial;
    private final String identity = UUID.randomUUID().toString();
    private long generation;
    private long deadlineElapsed;
    private Runnable action;
    private AlarmManager alarmManager;
    private PendingIntent pendingIntent;
    private String deliveryAction;
    private ScheduledFuture<?> future;

    GattRecoveryAlarm(String serial) {
        this.serial = serial;
    }

    synchronized boolean schedule(long delayMillis, Runnable nextAction) {
        cancelLocked();
        if (nextAction == null) {
            Log.e(LOG_ID, serial + " no recovery action");
            return false;
        }
        final long token = generation;
        final long delay = Math.max(0L, delayMillis);
        final long triggerRtc = System.currentTimeMillis() + delay;
        deadlineElapsed = SystemClock.elapsedRealtime() + delay;
        action = nextAction;
        try {
            if (delay == 0L) {
                future = Applic.scheduler.schedule(() -> fire(token), delay,
                        TimeUnit.MILLISECONDS);
            } else {
                alarmManager = (AlarmManager) Applic.app.getSystemService(Context.ALARM_SERVICE);
                if (alarmManager == null || Build.VERSION.SDK_INT < 21) {
                    Log.e(LOG_ID, serial + " alarm-clock scheduling unavailable");
                    cancelLocked();
                    return false;
                }
                if (Build.VERSION.SDK_INT >= 31 && !alarmManager.canScheduleExactAlarms()) {
                    Log.e(LOG_ID, serial + " exact-alarm access unavailable; recovery wakeup not scheduled");
                    cancelLocked();
                    return false;
                }
                // Intent extras do not determine PendingIntent identity. Put the
                // generation in the action so an old delivery cannot become a
                // delivery of a newly scheduled deadline (even after a restart).
                final String intentAction = ACTION_PREFIX + identity + "." + token;
                deliveryAction = intentAction;
                deliveries.put(intentAction, () -> fire(token));
                final Intent intent = new Intent(Applic.app, ConnectReceiver.class)
                        .setAction(intentAction);
                int flags = PendingIntent.FLAG_CANCEL_CURRENT;
                if (Build.VERSION.SDK_INT >= 23) {
                    flags |= PendingIntent.FLAG_IMMUTABLE;
                }
                pendingIntent = PendingIntent.getBroadcast(Applic.app, 0, intent, flags);
                // No show intent: exposing the operation there would allow other
                // apps to trigger this internal deadline via getNextAlarmClock().
                alarmManager.setAlarmClock(new AlarmManager.AlarmClockInfo(triggerRtc, null),
                        pendingIntent);
            }
            Log.i(LOG_ID, serial + " schedule generation=" + token + " delay=" + delay
                    + " rtc=" + triggerRtc + " wear=" + isWearable
                    + " dispatch=" + (delay == 0L ? "executor" : "alarmClock"));
            return true;
        } catch (Throwable error) {
            Log.stack(LOG_ID, serial + " schedule recovery", error);
            cancelLocked();
            return false;
        }
    }

    synchronized void cancel() {
        cancelLocked();
    }

    /** Consume recovery actions, including alarms from a dead process/session. */
    static boolean handle(Intent intent) {
        final String name = intent == null ? null : intent.getAction();
        if (name == null || !name.startsWith(ACTION_PREFIX)) {
            return false;
        }
        // ConcurrentHashMap releases its internal locks before returning; never
        // call into a sensor/helper while holding a registry lock.
        final Runnable delivery = deliveries.get(name);
        if (delivery != null) {
            delivery.run();
        } else {
            Log.i(LOG_ID, "ignore obsolete recovery alarm");
        }
        return true;
    }

    private void fire(long token) {
        final Runnable run;
        final long late;
        synchronized (this) {
            if (generation != token || action == null) {
                return;
            }
            run = action;
            late = SystemClock.elapsedRealtime() - deadlineElapsed;
            // Retire this delivery and invalidate duplicate deliveries before the
            // owner can schedule another deadline from inside its action.
            cancelLocked();
        }
        Log.i(LOG_ID, serial + " fire generation=" + token + " late=" + late);
        try {
            run.run();
        } catch (Throwable error) {
            Log.stack(LOG_ID, serial + " recovery action", error);
        }
    }

    // Caller holds this object's monitor; no owner callback may run in here.
    private void cancelLocked() {
        ++generation;
        action = null;
        deadlineElapsed = 0L;
        if (deliveryAction != null) {
            deliveries.remove(deliveryAction);
            deliveryAction = null;
        }
        if (future != null) {
            future.cancel(false);
            future = null;
        }
        if (pendingIntent != null) {
            final PendingIntent old = pendingIntent;
            pendingIntent = null;
            try {
                if (alarmManager != null) {
                    alarmManager.cancel(old);
                }
            } catch (Throwable error) {
                Log.stack(LOG_ID, serial + " cancel recovery alarm", error);
            }
            try {
                old.cancel();
            } catch (Throwable error) {
                Log.stack(LOG_ID, serial + " cancel recovery intent", error);
            }
        }
        alarmManager = null;
    }
}
