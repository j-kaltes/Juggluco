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

import tk.glucodata.Applic;
import tk.glucodata.GlucoseCurve;
import tk.glucodata.Log;

import static tk.glucodata.Log.doLog;
import static tk.glucodata.Specific.useclose;

import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;

import tk.glucodata.MainActivity;
import tk.glucodata.Natives;
import yuku.ambilwarna.AmbilWarnaDialog;

import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static tk.glucodata.Applic.usedlocale;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.util.getbutton;

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
        }, v-> {
        /*
    int afx,afy;
    if(Natives.getsystemUI()) {
         afy= MainActivity.systembarTop;
         afx= MainActivity.systembarLeft;
         }
     else {
        afx=afy=0;
         }
     {if(doLog) {Log.i(LOG_ID, "Color systembarTop="+afy);};};
     {if(doLog) {Log.i(LOG_ID, "Color systembarLeft="+afx);};};
         int h=v.getMeasuredHeight();
         int w=v.getMeasuredWidth();    
         v.setX((int)(width*.97)-w);
         v.setY((height-h)*.5f);
        */
        }
    );
    View view=dialog.getview();
    view.setBackgroundColor(Applic.backgroundcolor);
  //  act.addMyContentView(view, new ViewGroup.LayoutParams((int)(width*0.65), (int)(height*0.65)));

        var params= new FrameLayout.LayoutParams((int)(width*0.65), (int)(height*0.65), Gravity.RIGHT| Gravity.CENTER);
        params.rightMargin=(int)(width*.03f);
        act.addMyContentView(view, params);
    var ok=useclose?getbutton(act,"Ok"):null;
    if(ok!=null) {
        ok.setOnClickListener(v->MainActivity.doonback());
        var okparams= new FrameLayout.LayoutParams( WRAP_CONTENT, WRAP_CONTENT, Gravity.TOP|Gravity.CENTER_HORIZONTAL);
        act.addMyContentView(ok, okparams);

//        act.addMyContentView(ok, new ViewGroup.LayoutParams(WRAP_CONTENT,WRAP_CONTENT));

 //       ok.setX((int)(width*.45));

        int padx=(int)(width*.04f);
        int pady=(int)(width*.015f);
        ok.setPadding(padx,pady,padx,pady);
        }


    MainActivity.setonback(()-> {
        removeContentView(view);
        if(ok!=null)
            removeContentView(ok);
        if(tk.glucodata.Menus.on)
            tk.glucodata.Menus.show(act);
            
    });
}

}
