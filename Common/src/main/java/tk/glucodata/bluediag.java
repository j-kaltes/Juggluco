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
import androidx.appcompat.app.AlertDialog;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothManager;
import android.content.Context;
import android.content.DialogInterface;
import android.graphics.Paint;
import android.os.Build;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.Button;

import android.widget.FrameLayout;
import android.widget.GridLayout;
import android.widget.HorizontalScrollView;
import android.widget.Spinner;
import android.widget.TextView;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.TimeUnit;


import static android.bluetooth.BluetoothGatt.CONNECTION_PRIORITY_BALANCED;
import static android.bluetooth.BluetoothGatt.CONNECTION_PRIORITY_HIGH;
import static android.graphics.Color.BLACK;
import static android.graphics.Color.BLUE;
import static android.graphics.Color.GREEN;
import static android.graphics.Color.MAGENTA;
import static android.graphics.Color.CYAN;
import static android.graphics.Color.RED;
import static android.graphics.Color.YELLOW;
import static android.view.View.GONE;
import static android.view.View.INVISIBLE;
import static android.view.View.VISIBLE;
import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static java.util.Objects.isNull;
import static java.util.Objects.nonNull;
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.Log.doLog;
import static tk.glucodata.NumberView.avoidSpinnerDropdownFocus;
import static tk.glucodata.Specific.useclose;
import static tk.glucodata.help.help;
import static tk.glucodata.help.helplight;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getcheckbox;
import static tk.glucodata.util.getlabel;

class bluediag {

static  boolean returntoblue=false;
final static private String LOG_ID="bluediag";
private static  DateFormat fname;
public static void mktimeformat() {
        fname= new SimpleDateFormat("MM-dd HH:mm:ss", Locale.US );
        }
static {
        mktimeformat();
        }
public static String datestr(long tim) {
    return fname.format(tim);
    }
//View view ;
//int selected=0;

RangeAdapter<SuperGattCallback> adap;
Spinner spin=null;
TextView[] contimes;
TextView constatus;
TextView streaming;
TextView address;
TextView starttimeV;
TextView rssiview;
Button forget;
//Button reenable;
Button info;
void setrow(long[] times, TextView[]  timeviews, TextView info) {
    for(int i=0;i<2;i++) {
        long tim= times[i];
        TextView text= timeviews[i];
        if(tim!=0L) {
            text.setText(datestr(tim));
            if(tim<times[(~i)&1]) {
                text.setPaintFlags(text.getPaintFlags() | Paint.STRIKE_THRU_TEXT_FLAG);
                if(i==1) 
                    info.setPaintFlags(text.getPaintFlags() | Paint.STRIKE_THRU_TEXT_FLAG);
                }
                
            else {
                text.setPaintFlags(text.getPaintFlags() & (~Paint.STRIKE_THRU_TEXT_FLAG));
                if(i==1)
                    info.setPaintFlags(text.getPaintFlags() & (~Paint.STRIKE_THRU_TEXT_FLAG));
                }
            }
        else {
            text.setText("");
            }
        }
    }

static void showsensormessage(String text,MainActivity act) {
    var width=GlucoseCurve.getwidth();
    int height = GlucoseCurve.getheight();
    var close=getbutton(act, R.string.closename);
    final boolean wasused= Natives.getusebluetooth();
    final var messview=getlabel(act,text);
    final var usebluetooth=getcheckbox(act, R.string.use_bluetooth,wasused);
    var layout=new Layout(act,(l,w,h)->{
    /*
         l.setX((width-w)*.5f);
         l.setY((height-h)*.3f);
         */
         return new int[] {w,h};
           },new View[]{messview},new View[]{usebluetooth,close});
    final int rand=(int)tk.glucodata.GlucoseCurve.metrics.density*15;
     Layout.getMargins(messview).bottomMargin=rand;
    layout.setPadding(rand,rand,rand,rand);
    layout.setBackgroundColor(Applic.backgroundcolor);
    MainActivity.setonback(() -> {
       removeContentView(layout);
       });
    close.setOnClickListener(v-> {
        MainActivity.doonback();
        });
    usebluetooth.setOnCheckedChangeListener(
         (buttonView,  isChecked) -> {
             {if(doLog) {Log.i(LOG_ID,"usebluetooth "+isChecked);};};
             if(isChecked!=wasused) {
                 act.setbluetoothmain( isChecked);
                 act.requestRender();
                 MainActivity.doonback();
                 bluediag.start(act);
             }
         });

    var  params = new FrameLayout.LayoutParams( WRAP_CONTENT, WRAP_CONTENT, Gravity.CENTER| Gravity.CENTER_HORIZONTAL);
//    params.topMargin=MainActivity.systembarTop;
    act.addMyContentView(layout, params);
    }
/*
public static void showsensorinfo(String text,MainActivity act) {
       var width=GlucoseCurve.getwidth();
        help.basehelp(text,act,xzy->{
        }, (l,w,h)-> {
                var height=GlucoseCurve.getheight();
                if(height>h)
                    l.setY((height-h)/2);
                if(width>w)
                    l.setX((width-w)/2);
            return new int[] {w,h};
            }, new ViewGroup.MarginLayoutParams(WRAP_CONTENT, WRAP_CONTENT));
        }
*/
void showinfo(final SuperGattCallback gatt,MainActivity act) {
    if(isWearable)  {
        disconnectsensor.setVisibility(gatt.sensorgen==3?VISIBLE:GONE);
        }
    if(Natives.optionStreamHistory(gatt.dataptr)) {
       streamhistory.setVisibility(VISIBLE);
      alarmclock.setVisibility(GONE);
      resetbutton.setVisibility(GONE);
      divorcebutton.setVisibility(GONE);
        }
    else  {
      streamhistory.setVisibility(GONE);
      alarmclock.setVisibility(gatt.sensorgen==0x30||gatt.sensorgen==0x40?VISIBLE:GONE);
      final boolean resetvis=gatt.sensorgen==0x10&&Natives.getSiSubtype(gatt.dataptr)==3;
      resetbutton.setVisibility(resetvis?VISIBLE:GONE);
      }

    starttimeV.setText(datestr(gatt.starttime));
   if(gatt.sensorgen==0x50) {
         clear.setVisibility(VISIBLE);
         divorcebutton.setVisibility(VISIBLE);
         } 
    else {
        clear.setVisibility(GONE);
        divorcebutton.setVisibility(GONE);
        }
    if(gatt.sensorgen<=2) {
        if(gatt.streamingEnabled() ) {
            streaming.setText(R.string.streamingenabled);
            }
        else
            streaming.setText(R.string.streamingnotenabled);
        }
    else {
        if(gatt.streamingEnabled() ) {
                streaming.setText(R.string.sensorstreamed);
            }
        else
            streaming.setText(R.string.sensornotstreamed);
        }
    
//    var visi=gatt.sensorgen==3?INVISIBLE:VISIBLE;
    final int rssi=gatt.readrssi;
    if (rssi < 0) {
        rssiview.setText("Rssi = " + rssi);
        rssiview.setVisibility(VISIBLE);
    } else {
        rssiview.setText("");
        rssiview.setVisibility(GONE);
       }
         
    if(forget!=null)  {
//        forget.setVisibility(visi);
//        if(gatt.sensorgen!=3) 
      {
            forget.setOnClickListener(v-> {
                gatt.searchforDeviceAddress();
                gatt.close();
                SensorBluetooth.startscan();
                 act.doonback();
                 bluediag.start(act);

                });
            }
        }

    address.setText(gatt.mActiveDeviceAddress == null?"Address unknown":gatt.mActiveDeviceAddress);
    if(gatt.sensorgen == 2) {
//        address.setBackgroundColor(RED); address.setTextColor(BLACK);
        address.setTextColor(RED);
        }
    else {
//        address.setBackgroundColor(BLUE); address.setTextColor(WHITE);
        if(gatt.sensorgen == 0x10)  {
            long dataptr=gatt.dataptr;
            if(Natives.siNewSI(dataptr)) {
                 switch(Natives.getSiSubtype(dataptr)) {
                    case 0:address.setTextColor(RED);break;
                    case 1: address.setTextColor(CYAN);break;
                    case 2: address.setTextColor(MAGENTA);break;
                    };
                  }
             else {
                   address.setTextColor(YELLOW);;
                }
             
            }
        else
            address.setTextColor(YELLOW);
           //address.setTextColor(GREEN);
           //address.setTextColor(CYAN);
           // address.setTextColor(YELLOW);
        }
    //address.setBackgroundColor(BLACK);
    constatus.setText(gatt.constatstatusstr);
    setrow(gatt.constatchange,contimes,constatus);
    keyinfo.setText(gatt.handshake);
    setrow(gatt.wrotepass,keytimes,keyinfo);
    setrow(gatt.charcha,glucosetimes,glucoseinfo);

    {if(doLog) {Log.i(LOG_ID,"info.setVisibility(VISIBLE);");};};
    info.setVisibility(VISIBLE);
    info.setOnClickListener(v->   {
        Sensors.show(act,gatt.getinfo(),Natives.getsensorptr(gatt.dataptr));
        });

    }
TextView[] keytimes; TextView keyinfo;
TextView[] glucosetimes; TextView glucoseinfo;
TextView bluestate;
private BluetoothAdapter mBluetoothAdapter=null;
//boolean setwakelock=false;
CheckDirectionBox usebluetooth;
boolean wasuse;
CheckDirectionBox priority,streamhistory, alarmclock,disconnectsensor;
Button resetbutton;
Button divorcebutton;
Button clear;

Button locationpermission;
TextView scanview;
MainActivity activity;
static private int gattselected=0;

void confirmFinish(SuperGattCallback gat) {
    AlertDialog.Builder builder = new AlertDialog.Builder(activity);
    String serial= gat.SerialNumber;
    builder.setTitle(serial).setMessage(R.string.finishsensormessage).
      setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
         public void onClick(DialogInterface dialog, int id) {
                gat.finishSensor();
                SensorBluetooth.sensorEnded(serial);
                activity.requestRender();
                activity.doonback();
                bluediag.start(activity);
                }
                }).
        setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {
            }
        }).show().setCanceledOnTouchOutside(false);
    }

void    setadapter(Activity act,    final ArrayList<SuperGattCallback> gatts) {
    adap = new RangeAdapter<>(gatts, act, gatt -> {
        if (gatt != null && gatt.SerialNumber != null)
            return gatt.SerialNumber;
        return "Error";
    });
    spin.setAdapter(adap);
}

static void nosensors(MainActivity act) {
    BluetoothManager mBluetoothManager = (BluetoothManager) act.getSystemService(Context.BLUETOOTH_SERVICE);
    BluetoothAdapter mBluetoothAdapter=null;
    if(mBluetoothManager  != null) {
        mBluetoothAdapter = mBluetoothManager.getAdapter();
        if(mBluetoothAdapter ==null) {
            var mess="mBluetoothManager.getAdapter()==null";
            Log.e(LOG_ID,mess);

            showsensormessage(mess,act);
            return;
        }
    }
 var bluestate= getlabel(act, mBluetoothAdapter==null?act.getString(R.string.nobluetooth):(mBluetoothAdapter.isEnabled()?act.getString(R.string.bluetoothenabled): act.getString(R.string.bluetoothdisabled)));
 final boolean wasused= Natives.getusebluetooth();
 var usebluetooth=getcheckbox(act, R.string.use_bluetooth,wasused);
    usebluetooth.setOnCheckedChangeListener(
         (buttonView,  isChecked) -> {
             {if(doLog) {Log.i(LOG_ID,"usebluetooth "+isChecked);};};
             if(isChecked!=wasused) {
                 act.setbluetoothmain( isChecked);
                 act.requestRender();
                 act.doonback();
                 bluediag.start(act);
             }
         });
    var close=getbutton(act,R.string.closename);
   var height=GlucoseCurve.getheight();
   var width=GlucoseCurve.getwidth();
   if(!useclose)
      close.setVisibility(GONE);

    var help=getbutton(act,R.string.helpname);

    var streamhistory=getcheckbox(act,R.string.streamhistory,Natives.getStreamHistory());
    streamhistory.setOnCheckedChangeListener( (buttonView,  isChecked) -> Natives.setStreamHistory(isChecked) );
    if(!Natives.optionStreamHistory(0L)||!wasused)
        streamhistory.setVisibility(GONE);
help.setOnClickListener(v-> helplight(R.string.sensorhelp,act));
  Layout layout = new Layout(act, (l, w, h) -> {
  /*
      l.setX((width-w)/2);
      l.setY((height-h)/2);
      */
        int[] ret={w,h};
        return ret;
        },new View[]{bluestate},new View[]{usebluetooth},new View[]{streamhistory},new View[]{help,close});
    act.setonback(() -> {
            removeContentView(layout);
            });

        close.setOnClickListener(v -> {
         act.doonback();
         });
   layout.setBackgroundResource(R.drawable.dialogbackground);
   int pads=(int)(GlucoseCurve.metrics.density*(isWearable?2:10));
   {if(doLog) {Log.i(LOG_ID,"density="+GlucoseCurve.metrics.density);};};

    if(!isWearable) bluestate.setPaddingRelative(pads,0,0,0);
   layout.setPadding(pads,pads,pads,pads);

    var  params = new FrameLayout.LayoutParams( WRAP_CONTENT, WRAP_CONTENT, Gravity.CENTER|Gravity.CENTER_HORIZONTAL);
//    params.topMargin=MainActivity.systembarTop;
    act.addMyContentView(layout, params);

   }
   /*
void nogatts(MainActivity act) {
 var bluestate= getlabel(act, mBluetoothAdapter==null?activity.getString(R.string.nobluetooth):(mBluetoothAdapter.isEnabled()?activity.getString(R.string.bluetoothenabled): activity.getString(R.string.bluetoothdisabled)));
 final boolean wasused= Natives.getusebluetooth();
 var usebluetooth=getcheckbox(act, R.string.use_bluetooth,wasused);
    usebluetooth.setOnCheckedChangeListener(
         (buttonView,  isChecked) -> {
             {if(doLog) {Log.i(LOG_ID,"usebluetooth "+isChecked);};};
             if(isChecked!=wasused) {
                 act.setbluetoothmain( isChecked);
                 act.requestRender();
                 act.doonback();
                 start(act);
             }
         });
    var close=getbutton(act,R.string.closename);
   var height=GlucoseCurve.getheight();
   var width=GlucoseCurve.getwidth();
   if(!useclose)
      close.setVisibility(GONE);
  Layout layout = new Layout(act, (l, w, h) -> {
      l.setX((width-w)/2);
      l.setY((height-h)/2);
        int[] ret={w,h};
        return ret;
        },new View[]{bluestate},new View[]{usebluetooth},new View[]{close});
    act.setonback(() -> {
            removeContentView(layout);
            });

        close.setOnClickListener(v -> {
         act.doonback();
         });
      layout.setBackgroundResource(R.drawable.dialogbackground);
    int pads=(int)(GlucoseCurve.metrics.density*(isWearable?2:10));
   {if(doLog) {Log.i(LOG_ID,"density="+GlucoseCurve.metrics.density);};};

      if(!isWearable)
          bluestate.setPadding(pads,0,0,0);
      layout.setPadding(pads,pads,pads*3,pads);
   act.addMyContentView(layout, new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));

   }
   */

bluediag(MainActivity act,final ArrayList<SuperGattCallback> gatts) {
    activity=act;
    BluetoothManager mBluetoothManager = (BluetoothManager) act.getSystemService(Context.BLUETOOTH_SERVICE);
        if(mBluetoothManager  != null) {
            mBluetoothAdapter = mBluetoothManager.getAdapter();
          if(mBluetoothAdapter ==null) {
            var mess="mBluetoothManager.getAdapter()==null";
            Log.e(LOG_ID,mess);

             showsensormessage(mess,act);
            return;
            }
        }
   else {
           var mess="act.getSystemService(Context.BLUETOOTH_SERVICE)==null";
          Log.e(LOG_ID,mess);
           showsensormessage(mess,act);
         return;

           }

    LayoutInflater flater= LayoutInflater.from(act);
    View view = flater.inflate(R.layout.bluesensor, null, false);

    forget=view.findViewById(R.id.forget);
    scanview=view.findViewById(R.id.scan);
    starttimeV=view.findViewById(R.id.stage);
    rssiview=view.findViewById(R.id.rssi);
    rssiview.setPadding(0,0,0,0);
      CheckDirectionBox android13=view.findViewById(R.id.android13);
      if(android13!=null) {
         android13.setChecked( SuperGattCallback.autoconnect);
         android13.setOnCheckedChangeListener( (buttonView,  isChecked) -> { SensorBluetooth.setAutoconnect(isChecked); });
         }
    info=view.findViewById(R.id.info);
    {if(doLog) {Log.i(LOG_ID,"info.setVisibility(INVISIBLE);");};};
    info.setVisibility(INVISIBLE);
    int width2=GlucoseCurve.getwidth();
    HorizontalScrollView addscroll2=null;
    if(isWearable) {
        HorizontalScrollView scroll= view.findViewById(R.id.hori);
        scroll.setSmoothScrollingEnabled(false);
      scroll.setVerticalScrollBarEnabled(false);
      scroll.setHorizontalScrollBarEnabled(Applic.horiScrollbar);
      scroll.setScrollBarFadeDuration(0);
        int height=GlucoseCurve.getheight();
        scroll.setMinimumHeight(height);
        {if(doLog) {Log.i(LOG_ID,"height="+height);};};
        }
    else {
        HorizontalScrollView addscroll= view.findViewById(R.id.hori);
        /*
       GridLayout grid=view.findViewById(R.id.grid);
        final var addscroll= new HorizontalScrollView(act);
//        var  horparams = new FrameLayout.LayoutParams( WRAP_CONTENT, WRAP_CONTENT, Gravity.CENTER|Gravity.CENTER_HORIZONTAL);
 //       addscroll.addView(grid,horparams);
        addscroll.addView(grid);
*/
        addscroll.setSmoothScrollingEnabled(false);
       addscroll.setVerticalScrollBarEnabled(false);
        addscroll.setHorizontalScrollBarEnabled(Applic.horiScrollbar);
 //       int heightU=GlucoseCurve.getheight();
//        addscroll.setMinimumHeight(heightU);
        addscroll2=addscroll;

         }

    View showview=addscroll2!=null?addscroll2:view;

    priority=view.findViewById(R.id.priority);
    streamhistory=view.findViewById(R.id.streamhistory);
    alarmclock=view.findViewById(R.id.alarmclock);
    resetbutton=view.findViewById(R.id.resetbutton);
    divorcebutton=view.findViewById(R.id.divorcebutton);
    clear=view.findViewById(R.id.clear);
   alarmclock.setChecked(Natives.getalarmclock());
if(!isWearable) {
    Button finish = view.findViewById(R.id.finish);
    if (gatts != null && gatts.size() > 0) {
        finish.setOnClickListener(v -> {
            if (gatts != null && gatts.size() > 0) {
                if (gattselected >= gatts.size()) {
                    {if(doLog) {Log.i(LOG_ID, "show: gattselected=" + gattselected);};};
                    gattselected = 0;
                    return;
                }
                var gat = gatts.get(gattselected);
                confirmFinish(gat);
            }
        });
    }
    else {

        {if(doLog) {Log.i(LOG_ID,"finish.setVisibility(GONE);");};};
        finish.setVisibility(GONE);
        }
    } 
else {
   disconnectsensor=view.findViewById(R.id.disconnectsensor);
   disconnectsensor.setChecked(Natives.getDisconnectSensor());
   disconnectsensor.setOnCheckedChangeListener( (buttonView,  isChecked) -> Natives.setDisconnectSensor(isChecked) );
   }
    if (gatts == null || gatts.size()== 0) {
        //priority.setVisibility(GONE);
        forget.setVisibility(GONE);
        }
    contimes=new TextView[]{view.findViewById(R.id.consuccess) , view.findViewById(R.id.confail)};
    constatus=view.findViewById(R.id.constatus);
    constatus.setTextIsSelectable(true);
    streaming=view.findViewById(R.id.streaming);
    address=view.findViewById(R.id.deviceaddress);
    final int addressrand=(int)tk.glucodata.GlucoseCurve.metrics.density*5;
    address.setPaddingRelative(0,0,addressrand,0);
    keytimes=new TextView[]{view.findViewById(R.id.keysuccess) , view.findViewById(R.id.keyfailure)}; keyinfo=view.findViewById(R.id.keyinfo);
    keyinfo.setTextIsSelectable(true);
    glucosetimes=new TextView[]{view.findViewById(R.id.glucosesuccess) , view.findViewById(R.id.glucosefailure)}; glucoseinfo=view.findViewById(R.id.glucoseinfo);
    bluestate=view.findViewById(R.id.bluestate);
    usebluetooth=view.findViewById(R.id.usebluetooth);
   
    usebluetooth.setOnCheckedChangeListener(
         (buttonView,  isChecked) -> {
             {if(doLog) {Log.i(LOG_ID,"usebluetooth "+isChecked);};};
             final boolean blueused = Natives.getusebluetooth();
             if (blueused != usebluetooth.isChecked()) {
                 act.setbluetoothmain( !blueused);
                 act.requestRender();
                 act.doonback();
                 bluediag.start(act);
             }
             else {
                 if (isChecked != wasuse)
                     bluediag.start(act);
             }
         }
         );


    streamhistory.setOnCheckedChangeListener( (buttonView,  isChecked) -> Natives.setStreamHistory(isChecked) );
    alarmclock.setOnCheckedChangeListener( (buttonView,  isChecked) -> Natives.setalarmclock(isChecked) );
    divorcebutton.setOnClickListener( v -> {
            if(gatts!=null&&gattselected<gatts.size()) {
                final SuperGattCallback gatt = gatts.get(gattselected);
                if(gatt.sensorgen!=0x50) {
                    final String message="ERROR: divorcebutton on sensorgen="+gatt.sensorgen;
                    Log.i(LOG_ID,message);
                    Applic.Toaster(message);
                    }
                Confirm.ask(act,gatt.SerialNumber,act.getString(R.string.divorcemessage),()-> {
                    var aid=(AidexXGattCallback)gatt;
                    aid.startUnpair( new UnpairOverlayHost(act,R.string.releasingsensor),res -> {
                        aid.finishSensor();
                        SensorBluetooth.sensorEnded(aid.SerialNumber);
                        act.requestRender();
                        return true;
                        });
                    act.doonback();
                 });
              }
      });
    clear.setOnClickListener( v -> {
            if(gatts!=null&&gattselected<gatts.size()) {
                final SuperGattCallback gatt = gatts.get(gattselected);
                if(gatt.sensorgen!=0x50) {
                    final String message="ERROR: clear on sensorgen="+gatt.sensorgen;
                    Log.i(LOG_ID,message);
                    Applic.Toaster(message);
                    }
                Confirm.ask(act,gatt.SerialNumber,act.getString(R.string.clearmessage),()-> {
                    var aid=(AidexXGattCallback)gatt;
                    aid.startClear( new UnpairOverlayHost(act,R.string.clearingmemory),res -> {
                        act.requestRender();
                        return true;
                        });
                    act.doonback();
                    }
                  );
             }
      });
    resetbutton.setOnClickListener( v -> {
            Confirm.ask(act,act.getString(R.string.resettitle),act.getString(R.string.resetmessage),()-> {
                if(gatts!=null&&gattselected<gatts.size()) {
                    final SuperGattCallback gatt = gatts.get(gattselected);
                    if(gatt.sensorgen!=0x10) {
                        final String message="ERROR: resetbutton on sensorgen="+gatt.sensorgen;
                        Log.i(LOG_ID,message);
                        Applic.Toaster(message);
                        return;
                        }
                     final int subtype=Natives.getSiSubtype(gatt.dataptr);
                     if(subtype!=3) {
                        final String message="ERROR: resetbutton on "+subtype;
                        Log.i(LOG_ID,message);
                        Applic.Toaster(message);
                        return;
                        }
                    Log.i(LOG_ID,"resetbutton");
                    Natives.setResetSibionics2(gatt.dataptr,true);
    //                Log.showbytes("Reset Bytes",Natives.getSIResetBytes());
                    Applic.Toaster("Resetted ");
                    }
                });
            });

     if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
    priority.setOnCheckedChangeListener(
                 (buttonView,  isChecked) -> {
             final boolean priorityused = Natives.getpriority();
             if(priorityused != isChecked) {
                setpriorities(isChecked,gatts);
                }
         }
         );
        }
    else  {
        {if(doLog) {Log.i(LOG_ID,"priority.setVisibility(INVISIBLE);");};};
       priority.setVisibility(INVISIBLE);
       }
    boolean hasperm=Build.VERSION.SDK_INT < 23||Applic.noPermissions(act).length==0;
    if(!isWearable)  {
        locationpermission=view.findViewById(R.id.locationpermission);
        if(hasperm)   {
            {if(doLog) {Log.i(LOG_ID,"locationpermission.setVisibility(GONE);");};};
            locationpermission.setVisibility(GONE);
            }
        else {
            locationpermission.setOnClickListener(v-> {
                var noperm=Applic.noPermissions(act);
                if(noperm.length==0) {
                    {if(doLog) {Log.i(LOG_ID,"locationpermission.setVisibility(GONE);");};};
                    locationpermission.setVisibility(GONE);
                    }

                else  {
                    returntoblue=true;
                    act.doonback();
                    act.requestPermissions(noperm, act.BLUETOOTH_PERMISSION_REQUEST_CODE);
                    }
                });
            }
        }
    spin=view.findViewById(R.id.sensors);
   if(isWearable)
      spin.setPopupBackgroundResource(R.drawable.helpbackground);
    boolean[] first={true};
    spin.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
        @Override
        public  void onItemSelected (AdapterView<?> parent, View view, int position, long id) {
            {if(doLog) {Log.i(LOG_ID,"onItemSelected");};};
            try {
                if (first[0]) {
                    first[0] = false;
                    spin.setSelection(gattselected);
                    }
            else {        
                    
                if (gatts != null && gatts.size() > position) {
                    gattselected = position;
                    {if(doLog) {Log.i(LOG_ID, "onItemSelected: gattselected=" + gattselected);};};
                    SuperGattCallback gatt = gatts.get(gattselected);
                    showinfo(gatt, act);
                }
                }
            }
                catch(Throwable e) {
                    Log.stack(LOG_ID,e);
                    }

        }
        @Override
        public  void onNothingSelected (AdapterView<?> parent) {

        } });
    avoidSpinnerDropdownFocus(spin);
    if(gatts!=null) {
          setadapter(act,gatts);
        }
    if(!isWearable) {
         Button help=!isWearable?view.findViewById(R.id.help):null;
        help.setOnClickListener(v-> helplight(R.string.sensorhelp,act));
         Button background=view.findViewById(R.id.background);
        if(android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.M) {
            background.setOnClickListener(v-> Battery.batteryscreen(act,showview));
            }
        else  {
        {if(doLog) {Log.i(LOG_ID,"background.setVisibility(GONE);");};};
            background.setVisibility(GONE);
            }
        }


     Button close=view.findViewById(R.id.close);
     close.setOnClickListener(v-> act.doonback());
   if(!useclose)
      close.setVisibility(GONE);

      view.setBackgroundColor( Applic.backgroundcolor);
    show(act,showview);
   // act.addMyContentView(showview, new ViewGroup.LayoutParams( WRAP_CONTENT, WRAP_CONTENT));


//    var  params = new FrameLayout.LayoutParams( MATCH_PARENT, MATCH_PARENT, Gravity.CENTER|Gravity.CENTER_HORIZONTAL);
    var  params = new FrameLayout.LayoutParams( WRAP_CONTENT, WRAP_CONTENT, Gravity.CENTER|Gravity.CENTER_HORIZONTAL);
   params.topMargin=(int)(MainActivity.systembarTop*.3f);
   params.bottomMargin=(int)(MainActivity.systembarBottom*.3f);
   params.leftMargin=(int)(MainActivity.systembarLeft*.3f);
   params.rightMargin=(int)(MainActivity.systembarRight*.3f);
    act.addMyContentView(showview, params);

    var scheduled=Applic.scheduler.scheduleAtFixedRate( ()-> {
       {if(doLog) {Log.i(LOG_ID,"scheduled");};};
        act.runOnUiThread( ()-> { 
         if(gatts!=null&&gatts.size()>0) {
            if(gattselected>= gatts.size()) {
               {if(doLog) {Log.i(LOG_ID,"show: gattselected="+ gattselected);};};
               gattselected=0;
               }
            showinfo(gatts.get(gattselected),act);
            }

        });},29,29, TimeUnit.SECONDS);
    act.setonback(() -> {
           {if(doLog) {Log.i(LOG_ID,"onback");};};
            scheduled.cancel(false);
            act.setfineres(null);
            removeContentView(showview);
             if(Menus.on) {
                Menus.show(act);
                }


            });


    }

final static class Pair{
 public    long key;
public    String value;
        public Pair(long key,  String value){
            this.key = key;
            this.value = value;
        }
public    long getKey() {
        return key;
        }
    };
static void put(List<Pair> l,long key,String val) {
    l.add(new Pair(key,val));    
    }
static class onkey implements Comparator<Pair> {
    public int compare(Pair a, Pair b)
    {
        return (int)(a.key - b.key);
    }
}
private void showall() {
{if(doLog) {Log.i(LOG_ID,"showall");};};
//    test();
    SensorBluetooth  blue=SensorBluetooth.blueone;
    if(blue!=null&&blue.scantime!=0L) {
         long lasttime=0;
         final List<Pair> messages = new ArrayList<>();
         final ArrayList<SuperGattCallback> gatts=SensorBluetooth.mygatts();
         boolean found=false;
         if(gatts==null) {
             {if(doLog) {Log.i(LOG_ID,"showall gatts==null");};};
             }
         else {
         for(SuperGattCallback gatt:gatts) {
            if(gatt.foundtime>=blue.scantime) {
                if(gatt.foundtime>lasttime)
                   lasttime=gatt.foundtime;
                final String name=gatt.mygetDeviceName();
                found=true;
                put(messages,gatt.foundtime,": Found "+name +"\n");
                }
              }
          }
        if(!found)
             put(messages,blue.scantime,": Start search for sensors\n");
      if(lasttime==0L||lasttime>(System.currentTimeMillis()-5*60*1000)) {
         if(blue.scantimeouttime>blue.scantime)
            put(messages,blue.scantimeouttime, ": timeout\n");
//         if(blue.stopscantime>=blue.scantime) put(messages,blue.stopscantime, ": Stop searching\n");
         Collections.sort(messages, new onkey());
         
         StringBuilder builder= new StringBuilder();
             for (Pair entry : messages) {
               builder.append(datestr(entry.key));
               builder.append(entry.value);
            }
         
         builder.deleteCharAt(builder.length()-1);
         scanview.setText(builder);
         {if(doLog) {Log.i(LOG_ID,"scanview.setVisibility(VISIBLE);");};};
         scanview.setVisibility(VISIBLE);
         }
      else
         scanview.setVisibility(GONE);
        }
    else  {
        {if(doLog) {Log.i(LOG_ID,"scanview.setVisibility(GONE);");};};
        scanview.setVisibility(GONE);
        }
    if(!isWearable) {
        activity.setfineres(()-> {
        if( Build.VERSION.SDK_INT < 23|| Applic.noPermissions(activity).length==0) {
            {if(doLog) {Log.i(LOG_ID,"locationpermissin.setVisibility(GONE);");};};
            locationpermission.setVisibility(GONE);
            }
        });
        }
    bluestate.setText( mBluetoothAdapter==null?activity.getString(R.string.nobluetooth):(mBluetoothAdapter.isEnabled()?activity.getString(R.string.bluetoothenabled): activity.getString(R.string.bluetoothdisabled)));
        usebluetooth.setChecked(wasuse = Natives.getusebluetooth());
        priority.setChecked(Natives.getpriority());
    streamhistory.setChecked(Natives.getStreamHistory( ));
    if(!isWearable) {
        if( Build.VERSION.SDK_INT < 23|| Applic.noPermissions(activity).length==0) {
            {if(doLog) {Log.i(LOG_ID,"locationpermissin.setVisibility(GONE);");};};
            locationpermission.setVisibility(GONE);
            }
        }
    {if(doLog) {Log.i(LOG_ID,"showall end");};};
    }

    
private void show(MainActivity act,View view) {
    if(spin!=null) {
        SensorBluetooth.updateDevices() ;
        final ArrayList<SuperGattCallback> gatts=SensorBluetooth.mygatts();
        setadapter(activity,gatts);
        if(gatts!=null&&gatts.size()>0) {
            if(gattselected>= gatts.size()) {
                {if(doLog) {Log.i(LOG_ID,"show: gattselected="+ gattselected);};};
                gattselected=0;
                }
            avoidSpinnerDropdownFocus(spin);
            showinfo(gatts.get(gattselected),act);
            }
        }
    {if(doLog) {Log.i(LOG_ID,"view.setVisibility(VISIBLE);");};};
    view.setVisibility(VISIBLE);

    showall();    
    }

  private static  void setpriorities(boolean isChecked,ArrayList<SuperGattCallback> gatts) {
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
          Natives.setpriority(isChecked);
          if(gatts!=null) {
              final int use_priority = isChecked ? CONNECTION_PRIORITY_HIGH : CONNECTION_PRIORITY_BALANCED;
              for (SuperGattCallback g : gatts) {
                  try {
                      var ga = g.mBluetoothGatt;
                      if (ga != null)
                          ga.requestConnectionPriority(use_priority);
                  } catch (Throwable th) {
                      Log.stack(LOG_ID, "setpriorities", th);
                  }
              }
          }
      }

  }
static void start(MainActivity act) {
       boolean use=Natives.getusebluetooth();
       ArrayList<SuperGattCallback> gatts=SensorBluetooth.mygatts();
       if(isNull(gatts) || gatts.isEmpty()) {
           if(doLog) {
                 Log.i(LOG_ID,"start no gatts "+( use?"":"no ")+"use Bluetooth");
                 }
            do {
                  if(use)  {
                        Applic.app.initbluetooth(true,act,true);
                        gatts=SensorBluetooth.mygatts();
                        if(nonNull(gatts) && !gatts.isEmpty()) {
                            break;
                            }

                        }
                  if(isWearable)
                      Specific.wearnosensors(act);
                  else
                      MirrorSensors.show(act);
                  return;
                }  while(false);
           } 

         if(doLog) {
             Log.i(LOG_ID,"start has gatts "+( use?"":"no ")+"use Bluetooth");
             }
        if(!use) {
            Applic.dontusebluetooth();
            start(act);
            }
        new bluediag(act,gatts);
        }
};



