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
/*      Sun Sep 21 14:02:17 CEST 2025                                                */
package tk.glucodata;

import static android.view.View.GONE;
import static android.view.View.VISIBLE;
import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static tk.glucodata.Applic.backgroundcolor;
import static tk.glucodata.BluetoothGlucoseMeter.getExistingGatt;
import static tk.glucodata.RingTones.EnableControls;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.util.getbutton;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;

import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

public class MeterList {
//  static private final String LOG_ID ="MeterList";
static final class MeterView extends Layout{
   MeterListViewAdapter adapter;
   int meterIndex;
   TextView text;
   CheckDirectionBox active;
   void setdata(int index) {
         meterIndex=index;
         String name=Natives.GlucoseMeterDeviceName(index);
         String addr=Natives.GlucoseMeterDeviceAddress( index);
         boolean a=Natives.GlucoseMeterGetActive(index);
         long lastused=Natives.GlucoseMeterGetLastTime(index);
         String addtext=name+"\n"+addr+(lastused>0L?("\n"+text.getContext().getString(R.string.last)+bluediag.datestr(lastused)):"");
         if(a) {
            Context context=getContext();
            var gatt=getExistingGatt(index);
            if(gatt!=null) {
                gatt.view=this;
                if(gatt.connectedTime>gatt.disconnectedTime) {
                    addtext+="\n"+context.getString(R.string.isconnected)+": "+bluediag.datestr(gatt.connectedTime);
                    if(gatt.receivedTime==0L)
                        addtext+=gatt.isBonded?"\nBonded":"\nNot bonded";
                    }
                else {  
                    if(gatt.disconnectedTime>0L) {
                        addtext+="\n"+context.getString(R.string.isdisconnected)+": "+bluediag.datestr(gatt.disconnectedTime);
                        if(gatt.receivedTime==0L)
                            addtext+=gatt.isBonded?"\nWas bonded":"\nWas not bonded";
                        }

                      }
                if(gatt.receivedTime>0L) {
                    addtext+="\n"+context.getString(gatt.newvalues?R.string.newdata:R.string.nonewdata)+": "+bluediag.datestr(gatt. receivedTime);
                        }
                  }
            }
        text.setText(addtext);
         active.setChecked(a);
        }
    MeterView(MainActivity act,View parent,TextView t,CheckDirectionBox b, MeterListViewAdapter adapter) {
        super(act,(l,w,h)->{
             return new int[] {w,h};
               },new View[]{t},new View[]{b});
        text=t;
        active=b;
        meterIndex=-1;
        text.setOnClickListener( v -> { 
            MeterConfig.config(act,meterIndex,parent,null,adapter);
            });
        active.setOnCheckedChangeListener( (buttonView,  isChecked) -> {
            if(meterIndex>=0) {
                if(Natives.GlucoseMeterSetActive(meterIndex,isChecked)) {
                    if(isChecked) {
                           var gatt=BluetoothGlucoseMeter.addDevice(meterIndex,null);
                           gatt.view=this;
                            }
                     else
                            BluetoothGlucoseMeter.removeDevice(meterIndex);
                    }
                }
                } );
        active.setText(R.string.active);
        float density=GlucoseCurve.metrics.density;
        var marg=getMargins(active);
        marg.setMarginStart(0);
        marg.setMarginEnd((int)(density*10));
        int pad=(int)(5.0*density);
        setPadding(pad,pad,pad,(int)(density*8.0));

        setLayoutParams(new ViewGroup.LayoutParams(WRAP_CONTENT , ViewGroup.LayoutParams.WRAP_CONTENT));

        }

    MeterView(MainActivity act,View parent,MeterListViewAdapter adapter) {
        this(act,parent,new TextView(act),new CheckDirectionBox(act),adapter);
        }
    };
   static  class MeterListViewHolder extends RecyclerView.ViewHolder {
       public MeterListViewHolder(MeterView view) {
         super(view);
        }
   }


public static  class MeterListViewAdapter extends RecyclerView.Adapter<MeterListViewHolder> {
        View layout;
        MeterListViewAdapter(View par) {
             layout=par;
            }

       @NonNull
      @Override
       public MeterListViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
           MeterView view=new MeterView((MainActivity)parent.getContext(),layout,this);
           return new MeterListViewHolder(view);
       }


      @Override
      public void onBindViewHolder(final MeterListViewHolder holder, int pos) {
         MeterView text=(MeterView)holder.itemView;
         text.setdata(pos);
          }
           @Override
       public int getItemCount() {
            return Natives.GlucoseMeterCount();

           }

   }
static public void show(MainActivity act, View parent) {
     if(Natives.staticnum()) {
            if(parent!=null)
                 EnableControls(parent,false);
            help.help(R.string.staticnum,act,l-> {
                if(parent!=null)
                    EnableControls(parent,true);
                  });
            return;
            }
     if(parent!=null) parent.setVisibility(GONE);
      RecyclerView recycle = new RecyclerView(act);
      recycle.setHasFixedSize(true);
      recycle.setLayoutParams(new ViewGroup.LayoutParams(   ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
      var lin=new GridLayoutManager(act,2);
      recycle.setLayoutManager(lin);
      var close=getbutton(act,R.string.closename);
      close.setOnClickListener( v -> { 
            MainActivity.doonback();
            });
      var devices=getbutton(act,R.string.finddevices);
      var help=getbutton(act,R.string.helpname);
      View[] firstrow=new View[]{help,devices,close};
      Layout layout=new Layout(act,(x,w,h)->{
             return new int[] {w,h};
             },new View[]{recycle},firstrow); 
      var meteradapt = new MeterListViewAdapter(layout);
      recycle.setAdapter(meteradapt);
      devices.setOnClickListener( v -> { 
            DeviceList.show(act,meteradapt);
            });
        help.setOnClickListener(v-> tk.glucodata.help.help(R.string.GlucoseMeterList,act));
       layout.setBackgroundColor(backgroundcolor);
        float density=GlucoseCurve.metrics.density;
        float addleft,addright;
        if(MainActivity.rtl&&Applic.supportsRtl) {
                addleft=8.0f;
                addright=5.0f;
                }
         else {
                addleft=5.0f;
                addright=8.0f;
                }
    layout.setPadding((int)(density*addleft)+MainActivity.systembarLeft,MainActivity.systembarTop,MainActivity.systembarRight+(int)(density*addright),MainActivity.systembarBottom);

     act.addMyContentView(layout, new ViewGroup.LayoutParams(MATCH_PARENT, MATCH_PARENT));
     MainActivity.setonback(()-> {
           BluetoothGlucoseMeter.zeroViews();
           removeContentView(layout);
           parent.setVisibility(VISIBLE);
        });
      }

}
