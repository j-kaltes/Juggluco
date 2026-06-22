/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2, Libre 3, Dexcom G7/ONE+,              */
/*      Sibionics GS1Sb and Accu-Chek SmartGuide sensors.                            */
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
/*      Sun Sep 21 10:44:11 CEST 2025                                                */

package tk.glucodata;


import static android.view.View.INVISIBLE;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static tk.glucodata.Log.doLog;
import static tk.glucodata.NumberView.avoidSpinnerDropdownFocus;
import static tk.glucodata.RingTones.EnableControls;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getlabel;

import android.bluetooth.BluetoothDevice;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.Spinner;
import android.widget.Toast;

import java.text.DateFormat;
import java.util.Calendar;
import java.util.Locale;

class MeterConfig {

    static private final String LOG_ID="MeterConfig";
static void config(MainActivity context, int meterIndex, View parent, BluetoothDevice device, MeterList.MeterListViewAdapter adapter) {
    if(parent!=null)
       EnableControls(parent,false);
    if(doLog)
       Log.i(LOG_ID,"MeterConfig "+meterIndex);
    String deviceName=Natives.GlucoseMeterDeviceName(meterIndex);
    if(deviceName==null) {
        deviceName="Error: no device name";
        }
    var namelabel=getlabel(context,deviceName);
   final var bloodafter=getlabel(context,context.getString(R.string.bloodafter));
   var lasttime=Natives.GlucoseMeterGetLastTime(meterIndex);
   long[] newtime={lasttime};
   final var helpbutton=getbutton(context,R.string.helpname);
   helpbutton.setOnClickListener(v-> help.help(R.string.GlucoseMeter,context));
   final   var datebutton=getbutton(context, DateFormat.getDateInstance(DateFormat.DEFAULT).format(lasttime));
   var cal = Calendar.getInstance();
   var cancel=getbutton(context,R.string.cancel);
   var ok=getbutton(context, R.string.save);
   datebutton.setOnClickListener(
                v -> { 
         context.getnumberview().getdateviewal(context,newtime[0], (year,month,day)-> {
           cal.set(Calendar.YEAR,year);
           cal.set(Calendar.MONTH,month);
           cal.set(Calendar.DAY_OF_MONTH,day);
           long newmsec= cal.getTimeInMillis();
           newtime[0]=newmsec;
         datebutton.setText(DateFormat.getDateInstance(DateFormat.DEFAULT).format(newmsec));
         });

      });   
   cal.setTimeInMillis(newtime[0]);
   int[] hour={cal.get(Calendar.HOUR_OF_DAY)};
   int[]  min={cal.get(Calendar.MINUTE)};
   var timebutton=getbutton(context,  String.format(Locale.US,"%02d:%02d",hour[0],min[0] ));
   Spinner spinner=new Spinner(context);
   avoidSpinnerDropdownFocus(spinner);
   final var labels=Natives.getLabels();
   LabelAdapter<String> numspinadapt=new LabelAdapter<String>(context,labels,0);
   spinner.setAdapter(numspinadapt);
   int bloodpos=Natives.getbloodvar();
   final int[] bloodposptr={bloodpos};
   spinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
        @Override
        public  void onItemSelected (AdapterView<?> parent, View view, int position, long id) {
            bloodposptr[0]=position;
            Natives.setbloodvar((byte)position);
            }
        @Override
        public  void onNothingSelected (AdapterView<?> parent) {

        } });
   spinner.setSelection(bloodpos);
   var bloodvar=getlabel(context,R.string.bloodvar);
   float density=GlucoseCurve.metrics.density;
   bloodvar.setPadding((int)(density*10),0,(int)(4*density),0);
   var layout=new Layout(context,(x,w,h)->{
         var width=GlucoseCurve.getwidth();
         x.setX((width-w)/2);
         x.setY(MainActivity.systembarTop);
         return new int[] {w,h};
           },new View[]{namelabel},new View[]{bloodafter},new View[]{datebutton,timebutton},new View[]{bloodvar,spinner},new View[]{cancel,helpbutton,ok});
   timebutton.setOnClickListener(v-> {
     layout.setVisibility(INVISIBLE);
     context.getnumberview().gettimepicker(context,hour[0], min[0], (h,m) -> {
            hour[0]=h;
            min[0]=m;
            cal.set(Calendar.HOUR_OF_DAY,h);
            cal.set(Calendar.MINUTE,m);
            newtime[0]= cal.getTimeInMillis();
            timebutton.setText(String.format(Locale.US,"%02d:%02d",h,m));
            },()-> layout.setVisibility(View.VISIBLE));});
      layout.setBackgroundResource(R.drawable.dialogbackground);

   int pad=(int)(10.0*density);
   layout.setPadding(pad,pad,pad,(int)(density*14.0));
   Runnable closeall= () -> { 
      removeContentView(layout);
       if(parent!=null)
           EnableControls(parent,true);
      {if(doLog) {Log.i(LOG_ID,"MeterConfig.config back");};};
      };
   MainActivity.setonback(closeall);
   ok.setOnClickListener(v -> {
      if(bloodposptr[0]>=(labels.size()-1)) {
         Applic.argToaster(context,context.getString(R.string.specifyblood), Toast.LENGTH_LONG);
         return;
         }
      MainActivity.doonback();
      MainActivity.doonback();
      Log.i(LOG_ID,"save");
      context.finepermission();
      Natives.GlucoseMeterSetLastTime(meterIndex,newtime[0]);
      if(device!=null)
              BluetoothGlucoseMeter.addDevice(meterIndex,device);
      });
   cancel.setOnClickListener(v -> {
           MainActivity.doonback();
           Natives.GlucoseMeterSetActive(meterIndex,false); //TODO: only when new meter? Remove altogether?
           Applic.argToaster(context,"Glucose meter will not be used", Toast.LENGTH_LONG);
           if(adapter!=null)
               adapter.notifyDataSetChanged();
           });
   context.addMyContentView(layout, new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));
   }
}
