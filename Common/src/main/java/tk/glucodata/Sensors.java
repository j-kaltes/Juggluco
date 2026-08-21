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
/*      Fri Apr 03 10:10:15 CEST 2026                                                */
package tk.glucodata;

import static android.text.Html.TO_HTML_PARAGRAPH_LINES_CONSECUTIVE;
import static android.text.Html.fromHtml;
import static android.view.View.GONE;
import static android.view.View.INVISIBLE;
import static android.view.View.VISIBLE;
import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;

import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.Layout.getMargins;
import static tk.glucodata.Log.doLog;
import static tk.glucodata.NumberView.avoidSpinnerDropdownFocus;
import static tk.glucodata.help.helplight;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getcheckbox;

import android.graphics.Color;
import android.os.Build;
import android.text.method.LinkMovementMethod;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.Button;

import android.widget.FrameLayout;
import android.widget.ScrollView;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.SeekBar;

import java.text.MessageFormat;
import java.util.ArrayList;

class Sensors {
    private static final String LOG_ID="sensors";
ViewGroup viewgroup;
CheckDirectionBox hide;
Button calview;
TextView textview;
SeekBar warmupview;
TextView minutes;
long sensorptr;
static private final int MAX_VALUE = 255;
static private final int MAX_PROGRESS = 1000;

private static final double  X_MIN = 0.15;   // try 0.10 .. 0.20



static private int progressToValue(int progress,int minValue) {
    var t = (double)progress / MAX_PROGRESS;
    var x = X_MIN + (1.0 - X_MIN) * t;
    var X_MIN3 = X_MIN * X_MIN * X_MIN;
    var normalized = (x * x * x - X_MIN3) / (1.0 - X_MIN3);
    return (int)Math.round(minValue + (MAX_VALUE - minValue) * normalized);
    }

static private int valueToProgress(double value,int minValue) {
    var normalized = (value - minValue) / (MAX_VALUE - minValue);
    var X_MIN3 = X_MIN * X_MIN * X_MIN;
    var x = Math.cbrt(normalized * (1.0 - X_MIN3) + X_MIN3);
    var t = (x - X_MIN) / (1.0 - X_MIN);
    return (int)Math.round(t * MAX_PROGRESS);
    }



private int minvalue;

void setSensorptr(long sensorptr) {
    this.sensorptr=sensorptr;
     boolean hasCali=Natives.calibrateNR( sensorptr,0)>0||Natives.calibrateNR( sensorptr,1)>0;

     if(hasCali) {
          calview.setVisibility(VISIBLE);
          }
     else {
           calview.setVisibility(GONE);
        }
    hide.setChecked(Natives.getHidefromSensorptr(sensorptr));
    minvalue=Natives.getMinimalWarmup(sensorptr);
    if(true) {
        warmupview.setVisibility(VISIBLE);
        minutes.setVisibility(VISIBLE);
        int currentwarmup= Natives.getManualWarmupMinutes(sensorptr);
         warmupview.setProgress(valueToProgress(currentwarmup,minvalue));
         }
    else {
        warmupview.setVisibility(GONE);
        minutes.setVisibility(GONE);
        }
    if(!Natives.activeSensor(sensorptr)) {
        use.setVisibility(VISIBLE);
        }
     else {
        use.setVisibility(GONE);
        }
    }

void setSensorptrText(long sensorptr) {
   setSensorptr( sensorptr);
    settext(Natives.sensortextfromSensorptr(sensorptr));
    }
void settext(String text) {
        if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            textview.setText(fromHtml(text,TO_HTML_PARAGRAPH_LINES_CONSECUTIVE));
            }
        else {
            textview.setText(fromHtml(text));
            }
         }
Button use;
Sensors(MainActivity act,boolean givehelp,boolean select) {
       textview=new TextView(act);
       textview.setTextColor(util.getColorFromTheme(act, android.R.attr.textColorPrimary));

       textview.setTextIsSelectable(true);
       var close=getbutton(act,R.string.closename);
       calview=getbutton(act,R.string.calibrations);
       hide=getcheckbox(act,R.string.hide,false);
       hide.setOnCheckedChangeListener( (buttonView,  isChecked)-> {
                Natives.setHidefromSensorptr( sensorptr,isChecked);
                act.requestRender();
                });
       use=getbutton(act,R.string.use);
       use.setOnClickListener(v -> {
                useAgain(act,sensorptr);
                use.setVisibility(GONE);
                });
      String prefix=act.getString(R.string.warmup);
      minutes=new TextView(act);
      warmupview=new SeekBar(act);
     Applic.ifRTLseekbar(warmupview);


      var marg=Layout.getMargins(warmupview);
     if(isWearable)
          warmupview.setPadding((int)(GlucoseCurve.metrics.density*10.0f),0,(int)(GlucoseCurve.metrics.density*10.0f),0);
      else {
          warmupview.setPadding((int)(GlucoseCurve.metrics.density*5.5f),0,(int)(GlucoseCurve.metrics.density*5.8f),0);
          Layout.getMargins(minutes).topMargin=(int)( GlucoseCurve.metrics.density*12.0);
          }
      marg.topMargin=(int)( GlucoseCurve.metrics.density*12.0);
      marg.width=ViewGroup.LayoutParams.MATCH_PARENT;
      marg.height= ViewGroup.LayoutParams.WRAP_CONTENT;

      warmupview.setLayoutParams(marg);
      warmupview.setMax(MAX_PROGRESS);
      warmupview.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
        @Override
        public  void onProgressChanged (SeekBar seekBar, int progress, boolean fromUser) {
            var warmup=progressToValue(progress,minvalue);
            minutes.setText(MessageFormat.format(prefix, warmup));
            Natives.setManualWarmupMinutes(sensorptr,warmup);
            act.requestRender();
            }
        @Override
        public void onStartTrackingTouch(SeekBar seekBar) {
            {if(doLog) {Log.i(LOG_ID,"onStartTrackingTouch");};};
            }
        @Override
        public void onStopTrackingTouch(SeekBar seekBar) {
            }
        });

       warmupview.setProgress(valueToProgress(60,0));
       View[][] views;
        if(!isWearable) {
            if(givehelp) {
                var help=getbutton(act,R.string.helpname);
                help.setOnClickListener(v-> helplight(R.string.sensorinfo,act));
                views=new View[][]{new View[]{ textview},new View[]{minutes,warmupview},new View[]{hide,help,close},new View[]{use,calview}};
                }
            else
                views= new View[][]{new View[]{ textview},new View[]{minutes,warmupview},new View[]{hide,use,calview,close}};
             }
        else  {
           if(select) {
                long[] ptrs=Natives.activeSensorPtrs();
                if(ptrs.length==0) {
                    bluediag.nosensors(act);
                    return;
                    }

                var list=new ArrayList<Long>(ptrs.length);
                for(var p : ptrs) {
                    list.add(p);
                    }
                var spin=new Spinner(act);
                avoidSpinnerDropdownFocus(spin);    
                var adap = new RangeAdapter<>(list, act, ptr -> {
                    if(ptr != 0L) {
                        var name=Natives.namefromSensorptr(ptr);
                        if(name!=null)
                            return name;
                        }
                    return "Error";
                    });
                spin.setAdapter(adap);
               int[] waspos={0};

                spin.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
                    @Override
                    public  void onItemSelected (AdapterView<?> parent, View view, int position, long id) {
                        if(doLog) {Log.i(LOG_ID,"onItemSelected "+position);};
                        if(position!=waspos[0]) {
                            waspos[0]=position;
                            setSensorptrText(ptrs[position]);
                            }

                        }
                    @Override
                    public  void onNothingSelected (AdapterView<?> parent) { }
                    });
                views= new View[][]{new View[]{ textview},new View[]{spin},new View[]{warmupview},new View[]{minutes}, new View[]{use}, new View[]{hide},new View[]{calview},new View[]{close}};
                }
             else {
                views= new View[][]{new View[]{ textview},new View[]{warmupview},new View[]{minutes}, new View[]{use}, new View[]{hide},new View[]{calview},new View[]{close}};
                }
             }
       var layout=new Layout(act, (l, w, h) -> { final int[] ret={w,h}; return ret; }, views); 
       var scroll=new ScrollView(act);
       scroll.addView(layout);
       scroll.setFillViewport(true);
       scroll.setSmoothScrollingEnabled(false);
       scroll.setScrollbarFadingEnabled(true);
       scroll.setVerticalScrollBarEnabled(Applic.scrollbar);
        if(!isWearable) {
          layout.setPadding((int)(GlucoseCurve.metrics.density*10),0,(int)(GlucoseCurve.metrics.density*5),(int)(GlucoseCurve.metrics.density*5.0));
         
            }
         else {

            var height=GlucoseCurve.getheight();
                if(false) {
                //if(hasCali) 
                        int padside=(int)(GlucoseCurve.metrics.density*4);
                        layout.setPadding(padside,(int)(height*.1f),padside,(int)(height*.4f));
                        }
                  else {
                        layout.setPadding(0,(int)(height*.1f),0,0);
                        }
                }
        close.setOnClickListener(v -> {
            MainActivity.doonback();
            });
        calview.setOnClickListener(v -> { CalibrateList.show(act,sensorptr,scroll); });
        viewgroup=scroll; 
        }

private static void useAgain(MainActivity act,long sensorptr) {
    if(Natives.useAgain(sensorptr)) {
           var res=SensorBluetooth.updateDevices();
           SuperGattCallback.glucosealarms.setLossAlarm();
           if(res) {
                act.finepermission(); 
                }
              else
                act.systemlocation();
            }
       Applic.wakemirrors();
       act.requestRender();
    }
static    void show(MainActivity act,String text, long sensorptr) {
    if(act.sensorsVisible)
        return;
    act.sensorsVisible=true;
    final var sensors=new Sensors(act,true, isWearable && sensorptr == 0L);
    sensors.settext(text);
    if(isWearable&&sensorptr==0L) {
         long[] ptrs=Natives.activeSensorPtrs();
         if(ptrs.length>0) {
                sensorptr=ptrs[0];
                sensors.setSensorptrText(sensorptr);
                }
         else
                return;
         }
     else
            sensors.setSensorptr(sensorptr);
    var scroll=sensors.viewgroup;
   ViewGroup.LayoutParams params;
    if(!isWearable) {
        scroll.setBackgroundResource(R.drawable.dialogbackground);
  //      scroll.measure(WRAP_CONTENT, WRAP_CONTENT);
 //       var width=GlucoseCurve.getwidth();
//        scroll.setX((width-scroll.getMeasuredWidth()+MainActivity.systembarLeft-MainActivity.systembarRight)*.5f);
        params =
            new FrameLayout.LayoutParams(
                    WRAP_CONTENT,
                    WRAP_CONTENT,
                    Gravity.CENTER|Gravity.CENTER_HORIZONTAL);
        }
           // Layout.getMargins(scroll).topMargin=MainActivity.systembarTop;
    else {
         int param=isWearable?MATCH_PARENT:WRAP_CONTENT;
         params=new ViewGroup.LayoutParams(param,param);
        scroll.setBackgroundColor(Applic.backgroundcolor);
        }

   act.addMyContentView(scroll, params);
//    act.addMyContentView(sensors.viewgroup, new ViewGroup.LayoutParams(param,param));
    MainActivity.setonback(() -> {
            removeContentView(sensors.viewgroup);
            act.sensorsVisible=false;
            });
     }

}


