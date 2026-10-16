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
/*      Thu Oct 15 12:44:39 CEST 2026                                                */

// SPDX-License-Identifier: GPL-3.0-or-later
package tk.glucodata;

/** JNI boundary only. Persistence, matching and HR statistics live in C++. */
public final class ActivityNative {
    private ActivityNative() {}
    public static final int CREATE_AMOUNTS=1,USE_MILES=2;
    public static native long[] config();
    public static native void configure(long from, int interval, int defaultLabel);
    public static native void setOptions(int options);
    public static native String token(int type);
    public static native String pendingToken(int type);
    public static native void setToken(int type, String token);
    public static native void checkpoint(int type, String token, boolean dirty);
    public static native int markDetails(int type,long fromMillis,long toMillis,String origin);
    public static native int markAllDetails();
    public static native void resetTokens();
    public static native void beginSessionSnapshot();
    public static native void permissions(int mask);
    public static native void succeeded(long now);
    public static native int source(String origin, String device);
    public static native String sourceName(int source);
    public static native int upsert(String id, String origin, String title, String notes,
                                   long start, long end, int type, long modified, int source);
    public static native void deleted(String id);
    public static native void remove(int group);
    public static native void removeSource(int group,int source);
    public static native int[] sourceIDs(int group);
    public static native int[] indices(boolean pending);
    public static native int representative(int group);
    public static native double[] record(int index);
    /** Maximum speed (m/s), its time (milliseconds), and selected maximum HR time (milliseconds). */
    public static native double[] peaks(int index);
    public static native String[] texts(int index);
    public static native int[] sources(int group);
    public static native void metrics(int index, double[] values);
    public static native void samples(int index, int kind, long[] times, float[] values, int[] sources);
    /** Ordered rows: source ID, mean, SD, coverage seconds, resting HR, max HR, max HR time (milliseconds). */
    public static final int HEART_SOURCE_FIELDS=7;
    public static native double[] heartSources(int index);
    public static native int heartSource(int index);
    public static native void complete(int index);
    public static native void linkAmounts();
    public static native int[] rule(int type);
    public static native float roundTo(int type);
    public static native void setRule(int type, int label, int metric, float roundTo, boolean existing);
    public static native int newLabel(String name);
    public static native boolean labelAllowed(int label);
    public static native int reference(long hit);
    public static native String[] candidates(int group);
    public static native void link(int group, int base, int pos, long time, int label, float value);
}
