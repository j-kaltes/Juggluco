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

import android.app.Activity;
import android.content.Context;

/**
 * Stub for non-mobile (Wear OS) build variants.
 * The real implementation lives in the mobile source set.
 * Notify.java references this class only inside an {@code if (!isWearable)} guard,
 * so these methods are never actually called on a wearable.
 */
public class AlarmLockScreenActivity extends Activity {

    public static final String EXTRA_ALARM_KIND = "alarm_kind";

    public static void close() {}

    public static boolean isActive()             { return false; }
    public static boolean isEnabled()            { return false; }
    public static void    setEnabled(boolean on) {}

    /** Always true on non-mobile variants — full-screen intents are not used here. */
    public static boolean hasFullScreenIntentPermission()               { return true; }
    public static void    requestFullScreenIntentPermission(Context ctx) {}
}
