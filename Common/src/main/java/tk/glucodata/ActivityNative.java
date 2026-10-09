// SPDX-License-Identifier: GPL-3.0-or-later
package tk.glucodata;

/** JNI boundary only. Persistence, matching and HR statistics live in C++. */
public final class ActivityNative {
    private ActivityNative() {}
    public static native long[] config();
    public static native void configure(long from, int interval, int defaultLabel);
    public static native String token(int type);
    public static native void setToken(int type, String token);
    public static native void checkpoint(int type, String token, boolean dirty);
    public static native void resetTokens();
    public static native void beginSessionSnapshot();
    public static native void permissions(int mask);
    public static native void succeeded(long now);
    public static native int source(String origin, String device);
    public static native String sourceName(int source);
    public static native int upsert(String id, String origin, String title, String notes,
                                   long start, long end, int type, long modified, int source);
    public static native void deleted(String id);
    public static native int[] indices(boolean pending);
    public static native double[] record(int index);
    public static native String[] texts(int index);
    public static native int[] sources(int group);
    public static native void metrics(int index, double[] values);
    public static native void samples(int index, int kind, long[] times, float[] values, int[] sources);
    public static native double[] heartSources(int index);
    public static native void complete(int index);
    public static native void linkAmounts();
    public static native int[] rule(int type);
    public static native void setRule(int type, int label, int metric, boolean existing);
    public static native int newLabel(String name);
    public static native boolean labelAllowed(int label);
    public static native int reference(long hit);
    public static native String[] candidates(int group);
    public static native void link(int group, int base, int pos, long time, int label, float value);
}
