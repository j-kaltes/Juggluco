/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2, Libre 3, Dexcom G7/ONE+ and           */
/*      Sibionics GS1Sb sensors.                                                     */
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
/*      Thu Aug 21 15:22:14 CEST 2025                                                */


package tk.glucodata;

import static android.text.Html.TO_HTML_PARAGRAPH_LINES_CONSECUTIVE;
import static android.text.Html.fromHtml;
import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static tk.glucodata.Applic.backgroundcolor;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getlabel;
import static tk.glucodata.Log.doLog;
import static android.view.View.GONE;

import android.content.Context;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothManager;

import android.annotation.SuppressLint;
import android.text.method.ScrollingMovementMethod;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.widget.TextView;

import androidx.annotation.NonNull;

import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getcheckbox;
import static tk.glucodata.util.getlabel;

import static tk.glucodata.Applic.isWearable;

import android.view.ViewGroup;

public class Switch {
private static final String LOG_ID="Switch";
static private void logmsec(String message,long start) {
    if(doLog) {
        long after= System.currentTimeMillis()-start;
        Log.i(LOG_ID,message+" "+after);
        }
    }
static void choice(MainActivity act) {
    var stream=getcheckbox(act, R.string.streamname,true);
    var amounts=getcheckbox(act, R.string.amountsname,true);
    var ok=getbutton(act,R.string.ok);
    var cancel=getbutton(act,R.string.cancel);
   
    long laststream=Natives.lastglucosetime();
    var streamtime=getlabel(act,act.getString(R.string.last)+util.timestring(laststream));
    long lastnums=Natives.getnumlasttime( );
    var numstime=getlabel(act,act.getString(R.string.last)+util.timestring(lastnums));
    Layout layout = new Layout(act, (l, w, h) -> {
        int[] ret={w,h};
        return ret;
        },new View[]{ok},new View[]{stream},new View[]{streamtime},new View[]{amounts},new View[]{numstime},new View[]{cancel});
        

   layout.setBackgroundColor(Applic.backgroundcolor);
   act.addContentView(layout, new ViewGroup.LayoutParams(MATCH_PARENT,MATCH_PARENT));
   act.setonback(() -> {
        removeContentView(layout);
        });
   cancel.setOnClickListener(v -> {
        MainActivity.doonback();
        });
   ok.setOnClickListener(v -> {
           MainActivity.doonback();
           boolean takestream=stream.isChecked();
           boolean takeamounts=amounts.isChecked();
           if(takestream||takeamounts) {
               MainActivity.doonback();
               MessageSender.Companion.watchBluetooth(act,takestream,takeamounts);
               if(takestream)
                   act.setbluetoothmain( true); //takes long
               act.requestRender();
               }
        });
    }
static void wearnosensors(MainActivity act) {
    BluetoothManager mBluetoothManager = (BluetoothManager) act.getSystemService(Context.BLUETOOTH_SERVICE);
    BluetoothAdapter mBluetoothAdapter=null;
    if(mBluetoothManager  == null) {
            var mess="mBluetoothManager)==null";
            Log.e(LOG_ID,mess);
            bluediag.showsensormessage(mess,act);
       }
    else {
        mBluetoothAdapter = mBluetoothManager.getAdapter();
        if(mBluetoothAdapter ==null) {
            var mess="mBluetoothManager.getAdapter()==null";
            Log.e(LOG_ID,mess);

            bluediag.showsensormessage(mess,act);
            return;
        }
    }
 var blueenabled=mBluetoothAdapter.isEnabled();
 var bluestate= getlabel(act,blueenabled?act.getString(R.string.bluetoothenabled): act.getString(R.string.bluetoothdisabled));
 final boolean wasused= Natives.getusebluetooth();
 var usebluetooth=getcheckbox(act, R.string.use_bluetooth,wasused);
    usebluetooth.setOnCheckedChangeListener(
         (buttonView,  isChecked) -> {
             {if(doLog) {Log.i(LOG_ID,"usebluetooth "+isChecked);};};
             if(isChecked!=wasused) {
                 act.doonback();
                 act.setbluetoothmain( isChecked);
                 act.requestRender();
                 bluediag.start(act);
             }
         });
    var switcher=getbutton(act,R.string.switchtowatch);
    var close=getbutton(act,R.string.closename);
   if(!Specific.useclose)
      close.setVisibility(GONE);
   if(wasused||!blueenabled) {
       switcher.setVisibility(GONE);
       }
   else {
       switcher.setOnClickListener(v -> {
           choice(act);

        });
    }

    var sensors=getbutton(act,R.string.sensors);
    sensors.setOnClickListener(v -> {
            Sensors.show(act,"",0L);
        });
  Layout layout = new Layout(act, (l, w, h) -> {
        int[] ret={w,h};
        return ret;
        },new View[]{switcher},new View[]{bluestate},new View[]{usebluetooth},new View[]{sensors},new View[]{close});
    act.setonback(() -> {
            removeContentView(layout);
            });

    close.setOnClickListener(v -> {
         act.doonback();
         });

   layout.setBackgroundColor(Applic.backgroundcolor);
   act.addContentView(layout, new ViewGroup.LayoutParams(MATCH_PARENT,MATCH_PARENT));
   }
}
