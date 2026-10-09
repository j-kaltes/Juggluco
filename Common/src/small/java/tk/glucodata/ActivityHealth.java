// SPDX-License-Identifier: GPL-3.0-or-later
package tk.glucodata;
public final class ActivityHealth {
    public static boolean available(){return false;}
    public static String status(){return "Health Connect activity import is unavailable in this build";}
    public static void retrieve(MainActivity a){}
    public static void permissions(MainActivity a){}
    public static void settingsChanged(android.content.Context c){}
    public static void onResume(MainActivity a){}
    public static void onPause(){}
    public static void numberEditorClosed(){}
    public static int[] typeCodes(){return new int[0];}
    public static String typeName(int type){return "Activity "+type;}
}
