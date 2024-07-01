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

import android.app.AlertDialog;
import android.content.DialogInterface;

import tk.glucodata.Applic;
import tk.glucodata.GlucoseCurve;
import tk.glucodata.Log;

import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;

import tk.glucodata.MainActivity;
import tk.glucodata.Natives;
import tk.glucodata.R;
import yuku.ambilwarna.AmbilWarnaDialog;

import static android.view.View.GONE;
import static android.view.View.VISIBLE;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.Applic.usedlocale;
import static tk.glucodata.settings.Settings.removeContentView;

public class SetColors {
private static final String LOG_ID="SetColors";
//   AmbilWarnaDialog(Context context, int color, boolean supportsAlpha, OnAmbilWarnaListener listener)
/*
static View theview=null;
public static void endcolors(MainActivity act) {
	View view=theview;
	if(view!=null) {
		theview=null;
		removeContentView(view);
		act.poponback();
		}
	} */
static void show(MainActivity act) {

   act.lightBars(!Natives.getInvertColors( ));
	int initialColor= 0xfff7f022;
//	Natives.getlastcolor();
	int width=GlucoseCurve.getwidth();
	int height=GlucoseCurve.getheight();
    AmbilWarnaDialog dialog = new AmbilWarnaDialog(act, initialColor,c-> {
	Log.i(LOG_ID,String.format(usedlocale,"col=%x",c));
		Natives.setlastcolor(c);
		tk.glucodata.Applic.app.redraw();
    }, v-> {
      int afx,afy;
      if(Natives.getsystemUI()) {
         afy= MainActivity.systembarTop;
         afx= MainActivity.systembarLeft;
         }
        else {
        afx=afy=0;
         
         }
      Log.i(LOG_ID, "Color systembarTop="+afy);
		Log.i(LOG_ID, "Color systembarLeft="+afx);
	    if(isWearable) {
         int h=v.getMeasuredHeight();	
         int w=v.getMeasuredWidth();	
         v.setX((int)(width*.97)-w);
         v.setY((height-h)/2);
		}
      else  {
		   v.setY(afy);
		   v.setX(afx);
         }
    	}
	);
	View view=dialog.getview();
        view.setBackgroundColor(Applic.backgroundcolor);

	if(isWearable) {
	       act.addContentView(view, new ViewGroup.LayoutParams((int)(width*0.65), (int)(height*0.65)));
	      // theview=view;
		}
	else
	       act.addContentView(view, new ViewGroup.LayoutParams(WRAP_CONTENT,WRAP_CONTENT));
        Button close= act.findViewById(R.id.closeambi);
        Button help= act.findViewById(R.id.helpambi);
	if(close!=null) {
		if(isWearable) {
			help.setVisibility(GONE);	
			close.setVisibility(GONE);	
				}
		else	
		{
		close.setOnClickListener(v->{
			removeContentView(view);
			act.poponback();
			if(tk.glucodata.Menus.on)
				tk.glucodata.Menus.show(act);
					});
		help.setOnClickListener(v->{
			tk.glucodata.help.help(R.string.colorhelp,act);
					});
					}
		}
		else Log.e(LOG_ID,"findViewbByid failed");
	act.setonback(()-> {
	       //theview=null;
		removeContentView(view);
		if(tk.glucodata.Menus.on)
			tk.glucodata.Menus.show(act);
			
	});
}

/*
static void show(MainActivity act, int initialColor ) {
	Log.i(LOG_ID,"show");
	String mess="Hallo";
        AlertDialog.Builder builder = new AlertDialog.Builder(act);
        builder.setTitle("Niets?").
	 setMessage(mess).
           setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
				}
                }) .setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {
            }
        }).show();
	}
*/
}
