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

import android.content.Context;
import android.content.SharedPreferences;

import java.text.DateFormat;
import java.util.Arrays;
import java.util.Date;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.TreeSet;

/**
 * Manages alarm snooze state.  When the user presses a snooze button on the
 * {@link AlarmLockScreenActivity}, alarms are suppressed until
 * {@link #snoozeUntilMs()} has passed.
 *
 * Also persists which snooze-duration buttons are shown on the alarm UI
 * (up to 3, chosen from 30/60/90/120 min — default 60/90/120, matching GDH).
 *
 * State is persisted so it survives a process restart.
 */
public class AlarmSnooze {

    private static final String PREFS          = "juggluco_dev";
    private static final String KEY_SNOOZE_END = "alarm_snooze_end_ms";
    /** Comma-separated list of selected snooze durations, e.g. "60,90,120". */
    private static final String KEY_SNOOZE_BTNS = "alarm_snooze_buttons";

    /** All available snooze durations (minutes). */
    public static final long[] ALL_DURATIONS = {30L, 60L, 90L, 120L};

    /** Default selection when no preference has been saved yet. */
    private static final Set<String> DEFAULT_BTNS =
            new HashSet<>(Arrays.asList("60", "90", "120"));

    /** In-memory cache; initialised from prefs on first access via {@link #init()}. */
    private static volatile long snoozeEndMs = 0L;
    private static boolean       initialised = false;

    // ── init ─────────────────────────────────────────────────────────────────

    /** Call once at startup (e.g. from keeprunning) to restore persisted state. */
    public static void init() {
        snoozeEndMs = prefs().getLong(KEY_SNOOZE_END, 0L);
        initialised = true;
    }

    // ── snooze active state ───────────────────────────────────────────────────

    /** Returns true when a snooze is currently active (not yet expired). */
    public static boolean isActive() {
        if (!initialised) init();
        return snoozeEndMs > System.currentTimeMillis();
    }

    /**
     * Start a snooze for the given number of minutes.
     * Pass 0 to cancel an active snooze immediately.
     */
    public static void set(long minutes) {
        if (!initialised) init();
        snoozeEndMs = minutes > 0
                ? System.currentTimeMillis() + minutes * 60_000L
                : 0L;
        prefs().edit().putLong(KEY_SNOOZE_END, snoozeEndMs).apply();
    }

    /** The absolute end time of the current snooze (0 if not active). */
    public static long snoozeUntilMs() {
        if (!initialised) init();
        return snoozeEndMs;
    }

    /**
     * Human-readable "until HH:mm" string for status display.
     * Returns an empty string if no snooze is active.
     */
    public static String snoozeUntilText() {
        if (!isActive()) return "";
        return DateFormat.getTimeInstance(DateFormat.SHORT).format(new Date(snoozeEndMs));
    }

    // ── snooze button configuration ───────────────────────────────────────────

    /**
     * Returns the currently configured snooze durations (minutes), sorted
     * ascending, capped at 3 entries — matching GDH behaviour.
     * Default is {60, 90, 120}.
     */
    public static List<Long> getSnoozeButtons() {
        final Set<String> raw = prefs().getStringSet(KEY_SNOOZE_BTNS, DEFAULT_BTNS);
        // TreeSet sorts them; map to Long; take at most 3
        final TreeSet<Long> sorted = new TreeSet<>();
        for (String s : raw) {
            try { sorted.add(Long.parseLong(s)); } catch (NumberFormatException ignored) {}
        }
        return sorted.stream().limit(3).collect(java.util.stream.Collectors.toList());
    }

    /**
     * Persist a new set of selected snooze durations.
     * @param selected set of minute-values as strings, e.g. {"60","90","120"}
     */
    public static void setSnoozeButtons(Set<String> selected) {
        prefs().edit().putStringSet(KEY_SNOOZE_BTNS, selected).apply();
    }

    /** Returns the raw persisted set of button values (strings) for UI display. */
    public static Set<String> getSnoozeButtonsRaw() {
        return prefs().getStringSet(KEY_SNOOZE_BTNS, DEFAULT_BTNS);
    }

    // ── helpers ───────────────────────────────────────────────────────────────

    private static SharedPreferences prefs() {
        return Applic.app.getSharedPreferences(PREFS, Context.MODE_PRIVATE);
    }
}
