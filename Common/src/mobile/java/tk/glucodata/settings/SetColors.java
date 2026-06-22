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
/*      Fri Jan 27 15:31:32 CET 2023                                                 */


package tk.glucodata.settings;

import androidx.appcompat.app.AlertDialog;
import android.content.DialogInterface;

import tk.glucodata.Applic;
import tk.glucodata.GlucoseCurve;
import tk.glucodata.Log;

import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.FrameLayout;

import tk.glucodata.MainActivity;
import tk.glucodata.Natives;
import tk.glucodata.R;
import yuku.ambilwarna.AmbilWarnaDialog;

import static android.view.View.GONE;
import static android.view.View.VISIBLE;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.Applic.usedlocale;
import static tk.glucodata.Log.doLog;
import static tk.glucodata.settings.Settings.removeContentView;

public class SetColors {
private static final String LOG_ID="SetColors";
static void show(MainActivity act) {

   act.lightBars(!Natives.getInvertColors( ));
    int initialColor= 0xfff7f022;
    int width=GlucoseCurve.getwidth();
    int height=GlucoseCurve.getheight();
    AmbilWarnaDialog dialog = new AmbilWarnaDialog(act, initialColor,c-> {
        {if(doLog) {Log.i(LOG_ID,String.format(usedlocale,"col=%x",c));};};
            Natives.setlastcolor(c);
            tk.glucodata.Applic.app.redraw();
        }, v-> { });
    View view=dialog.getview();
    view.setBackgroundColor(Applic.backgroundcolor);

    if(isWearable) {
        var params= new FrameLayout.LayoutParams((int)(width*0.65), (int)(height*0.65), Gravity.RIGHT| Gravity.CENTER);
        params.rightMargin=(int)(width*.03f);
        act.addMyContentView(view, params);
        }
    else {
        var params= new FrameLayout.LayoutParams( WRAP_CONTENT, WRAP_CONTENT, Gravity.LEFT|Gravity.TOP);
        params.topMargin=MainActivity.systembarTop;
        params.leftMargin=MainActivity.systembarLeft;
        act.addMyContentView(view, params);
        Button close= act.findViewById(R.id.closeambi);
        Button help= act.findViewById(R.id.helpambi);
        if(close!=null) {
                close.setOnClickListener(v->{
                    removeContentView(view);
                    act.poponback();
                    if(tk.glucodata.Menus.on)
                        tk.glucodata.Menus.show(act);
                            });
                help.setOnClickListener(v->{
                    tk.glucodata.help.helplight(R.string.colorhelp,act);
                    });
            }
        else Log.e(LOG_ID,"findViewbByid failed");
        }

    act.setonback(()-> {
        removeContentView(view);
        if(tk.glucodata.Menus.on)
            tk.glucodata.Menus.show(act);
            
    });
}

}
