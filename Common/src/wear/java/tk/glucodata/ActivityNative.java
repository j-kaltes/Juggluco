// SPDX-License-Identifier: GPL-3.0-or-later
package tk.glucodata;
/** Shared Amount editor hooks; no activity UI or JNI in this flavor. */
public final class ActivityNative {
    public static int reference(long hit){return 0;}
    public static boolean labelAllowed(int label){return true;}
}
