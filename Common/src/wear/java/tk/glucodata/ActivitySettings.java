// SPDX-License-Identifier: GPL-3.0-or-later
package tk.glucodata;
import android.view.View;
/** Activities are presented only in the mobile flavor. */
public final class ActivitySettings {
    public static void show(MainActivity act,View parent){}
    public static void details(MainActivity act,View parent,int group,Runnable changed,Runnable removed){}
}
