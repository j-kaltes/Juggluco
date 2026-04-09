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
/*      Fri Jan 27 15:31:05 CET 2023                                                 */


package tk.glucodata;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.graphics.text.LineBreakConfig;
import android.graphics.text.LineBreaker;
import android.net.Uri;
import android.os.Build;
import android.text.InputType;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

import static android.graphics.text.LineBreaker.BREAK_STRATEGY_SIMPLE;
import static android.text.Layout.BREAK_STRATEGY_HIGH_QUALITY;
import static android.view.View.GONE;
import static android.view.View.INVISIBLE;
import static android.view.View.VISIBLE;
import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;

import static java.lang.System.currentTimeMillis;

import static tk.glucodata.Log.doLog;
import static tk.glucodata.NumberView.geteditview;
import static tk.glucodata.NumberView.geteditwearos;
import static tk.glucodata.NumberView.smallScreen;
import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getcheckbox;
import static tk.glucodata.util.getlabel;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Locale;

import tk.glucodata.settings.Settings;

//import org.w3c.dom.Text;

public class Dialogs {
private    final static String LOG_ID="Dialogs";
private float density;
private ViewGroup exportscreen=null;
 TextView exportlabel=null;
 private boolean isCalibrated=Natives.getDoCalibrate();
Dialogs(float density) {
    this.density=density;
    }
private Button exportbutton(MainActivity activity,String label, int type) {
    Button but=new Button(activity);
    but.setText(label);
    but.setOnClickListener(
        v-> {
            float daynr;
            try {
                daynr=Float.parseFloat(String.valueOf(days.getText()));
                } catch(Throwable th) {

                    exportlabel.setText("I don't understand \'"+days.getText()+"\'");
                    return;
                };
            switch(type) {
                case 4: algexporter(activity,   type,label,".html",daynr);break;
                case 5: algexporter(activity,type|(isCalibrated?8:0),label,".csv",daynr);break;
                default: exporter( activity,  type|(isCalibrated?8:0),label,daynr);
                };
            });
    return but;
    }
         




EditText days;


public void showexport(MainActivity activity,int width,int height,View parent) {
   if(parent!=null) {
       activity.lightBars(!Natives.getInvertColors());
       parent.setVisibility(GONE);
       {if(doLog) {Log.i(LOG_ID, "parent.setVisibility(GONE)");};};
       }
    if(exportscreen==null) {
        Button num=exportbutton(activity,activity.getString(R.string.amountsname),0);
        Button scan=exportbutton(activity,activity.getString(R.string.scansname),1);
        Button stream=exportbutton(activity,activity.getString(R.string.streamname),2);
        Button hist=exportbutton(activity,activity.getString(R.string.historyname),3);
        Button meals=exportbutton(activity,activity.getString(R.string.mealsname),4);
        Button libreview=exportbutton(activity,activity.getString(R.string.libreviewname),5);
        days= smallScreen?geteditwearos(activity):geteditview(activity,new editfocus()) ;
        days.setMinEms(3);
        long hour24=1000*60*60*24;
        long endtime=Natives.getendtime();
        long daysnr= (endtime- Natives.oldestdatatime()+hour24-1)/hour24;
        days.setText(Long.toString(daysnr));
        var daylabel=getlabel(activity, R.string.days);


        var help=getbutton(activity,R.string.helpname);
        help.setOnClickListener(v->  {
                tk.glucodata.help.helplight(R.string.helpexport,activity); 
                }
                );

        exportlabel=new TextView(activity);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            exportlabel.setElegantTextHeight(true);
        }
        exportlabel.setInputType(InputType.TYPE_TEXT_FLAG_MULTI_LINE);

        
   //     exportlabel.setLayoutParams(new ViewGroup.LayoutParams(  (int)(width*(smallScreen?0.6f:.33f)), WRAP_CONTENT));
        exportlabel.setLayoutParams(new ViewGroup.LayoutParams(  MATCH_PARENT, WRAP_CONTENT));
        exportlabel.setSingleLine(false);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            exportlabel.setBreakStrategy(BREAK_STRATEGY_SIMPLE);
        }
        final int rand=Math.round(5*density);
        final int leftright=Math.round(2*density);
        exportlabel.setPadding(leftright,rand,0,rand);
        Button close=new Button(activity);
        close.setText(R.string.closename);
        close.setOnClickListener(v-> activity.doonback());
        var calibrated=getcheckbox(activity,R.string.calibrated, isCalibrated);
        calibrated.setOnCheckedChangeListener( (buttonView,  isChecked) ->  { 
            isCalibrated=isChecked; 
            });
        exportscreen=new Layout(activity, (l, w, h) -> {
            int wid = GlucoseCurve.getwidth();
            if(!smallScreen) {
                int hei = GlucoseCurve.getheight();
                if(hei>h&&wid>w) {
                    int half= wid / 2;
                    int af=(half-w)/4;
                    l.setX(half - w-af +MainActivity.systembarLeft);
                    l.setY((hei - h) / 2);
                    }
                   else {
                    l.setX(MainActivity.systembarLeft);
                    l.setY(MainActivity.systembarTop*3/4);
                    }
                   }
            else {
                 l.setX((int)(MainActivity.systembarLeft+(wid-w-MainActivity.systembarLeft-MainActivity.systembarRight)*.5f));
                l.setY(MainActivity.systembarTop*3/4);
                 }
               return new int[] {w,h};
            }, 
            new View[] {calibrated,daylabel,days},
             new View[] {scan,hist,stream},
             new View[] {num,meals,libreview},
            new View[]{exportlabel},
             new View[] {help,close}
            );
        ((Layout)exportscreen).useMatch=true;
        exportscreen.setPadding(rand,rand,rand,rand);
        exportscreen.setBackgroundColor( Applic.backgroundcolor);
        activity.addContentView(exportscreen, new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));
//        exportlabel.requestFocus();

         exportscreen.post(exportscreen::requestLayout);
        }
    else {
            exportscreen.setVisibility(VISIBLE);
         exportscreen.bringToFront();
        }

    exportlabel.setText(R.string.exporthelp);
    days.requestFocus();
    if(!smallScreen) {
        activity.curve.numberview.showkeyboard(activity);
        }
    else  {
        help.showkeyboard(activity,days);
        }
    {if(doLog) {Log.i(LOG_ID, "parent==null");};};
     activity.setonback(() -> {
       if(smallScreen) {
          help.hidekeyboard(activity);
          }
       else
          activity.curve.numberview.hidekeyboard() ;
       exportscreen.setVisibility(GONE);
       if(parent!=null) {
            activity.lightBars(false);
            {if(doLog) {Log.i(LOG_ID, "parent.setVisibility(VISIBLE)");};};
            parent.setVisibility(VISIBLE);
          }
       else {
          if(Menus.on)
             Menus.show(activity);
          } });
    }
static    public final DateFormat fdatename=             new SimpleDateFormat("yyyy-MM-dd-HH:mm:ss", Locale.US);
static void algexporter(MainActivity context,int type,String prefix,String ext,float days) {
    final long time=Natives.getendtime();
    final String datestr=fdatename.format(time)      ;
        final String filename = prefix+datestr+ext;
        exportdata(context,type,filename,days);
    }
static void exporter(MainActivity context,int type,String prefix,float days) {
        algexporter(context,type,prefix,".tsv",days);
    }


static float showdays=0;
static private void exportdata(MainActivity     context,int type,String name,float days) {
        Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        intent.putExtra(Intent.EXTRA_TITLE, name);
    intent.putExtra(Intent.EXTRA_LOCAL_ONLY, true);
          int request= MainActivity.REQUEST_EXPORT|type;
    try {
        showdays=days;
        context.startActivityForResult(intent, request);
        } catch(Throwable th) {
            Log.stack(LOG_ID,"ACTION_CREATE_DOCUMENT",th);
            }
    }

}
