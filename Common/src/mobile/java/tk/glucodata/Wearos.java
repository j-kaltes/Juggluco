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
/*      Fri Jan 27 15:32:11 CET 2023                                                 */


package tk.glucodata;

import androidx.appcompat.app.AlertDialog;
import android.app.Application;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.pm.PackageManager;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.RadioButton;
import android.widget.Space;
import android.widget.Spinner;
import android.widget.Toast;

import com.google.android.gms.wearable.Node;

import java.util.ArrayList;

import static android.view.View.INVISIBLE;
import static android.view.View.VISIBLE;
import static android.view.View.GONE;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static tk.glucodata.Log.doLog;
import static tk.glucodata.MessageSender.isGalaxy;
import static tk.glucodata.Natives.setBlueMessage;
import static tk.glucodata.NumberView.avoidSpinnerDropdownFocus;
import static tk.glucodata.RingTones.EnableControls;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getcheckbox;
import static tk.glucodata.util.getlabel;
import static tk.glucodata.util.getradiobutton;


class Wearos {
static private final String LOG_ID="Wearos";


private static    ArrayList<Node> getnodeslist() {
    var send=tk.glucodata.MessageSender.getMessageSender();
    if(send!=null) {
        var nodes=send.getNodes();
        if(nodes!=null) {
            return new ArrayList<>(nodes);
        }
     }
   return null;
   }
static Spinner mkspinner(MainActivity context, ArrayList<Node> nodeslist,IntConsumer setpos) {
    var spin=new Spinner(context);
    var adap = new RangeAdapter<com.google.android.gms.wearable.Node>(nodeslist, context, node -> {
        if (node != null)
            return node.getDisplayName()+" - "+node.getId();
        return "Error";
        });
    spin.setAdapter(adap);

    spin.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
        @Override
        public  void onItemSelected (AdapterView<?> parent, View view, int position, long id) {
            {if(doLog) {Log.i(LOG_ID,"onItemSelected");};};
            setpos.accept(position);
           }
        @Override
        public  void onNothingSelected (AdapterView<?> parent) {
            setpos.accept(-1);
        } });
    avoidSpinnerDropdownFocus(spin);
    return spin;
    }


static void remake( RadioButton[] sensordirect, RadioButton[] nswitch,  Node node) {
    int dirval,numsval;
    if(node==null) {
        dirval=-1;
        numsval=-1;
        }
    else {
        String name=makenodename(node);
        dirval=Natives.directsensorwatch(name);
        numsval=Natives.hasWatchNums(name);
        }
    if(dirval<0)  {
        for(var v:sensordirect) {
            v.setEnabled(false);
            }
        }
    else  {
        for(var v:sensordirect) {
            v.setEnabled(true);
            }
        sensordirect[0].setChecked(dirval==0);
        sensordirect[1].setChecked(dirval!=0);
        }
    if(numsval<0)  {
        for(var n:nswitch)
            n.setEnabled(false);
        }
    else  {
        for(var n:nswitch)
            n.setEnabled(true);
        nswitch[0].setChecked(numsval==0);
        nswitch[1].setChecked(numsval!=0);
        }
        /*
    if(dirval==1) {
        start.setEnabled(false);
        }
    else
        start.setEnabled(true); */
    }


static public void show(MainActivity context,View parent) {
   final int[] nodenumptr={-1};
//    var start=getbutton(context,R.string.initwatchapp);
    var defaults=getbutton(context,context.getString(R.string.defaults));
    var directstring=context.getString(R.string.directsensor);
    var direct=getlabel(context, directstring);
    if(directstring.length()==0)
        direct.setVisibility(GONE);
    var connectionstring=context.getString(R.string.connectionl);
    var connection=getlabel(context, connectionstring);
    if(connectionstring.length()==0)
        connection.setVisibility(GONE);
    var sphone=getradiobutton(context, R.string.phone);
    var swatch=getradiobutton(context, R.string.watch);
    RadioButton[]sswitch={sphone,swatch}; 
    Backup.setradio(sswitch);


    var enternums=getlabel(context, R.string.enternums);

    var nphone=getradiobutton(context, R.string.phone);
    var nwatch=getradiobutton(context, R.string.watch);
    RadioButton[]nswitch={nphone,nwatch}; 
    Backup.setradio(nswitch);
    var Ok=getbutton(context,R.string.closename);
    var Help=getbutton(context,R.string.helpname);
    Help.setOnClickListener(v-> help.helplight(R.string.wearosinfo,context));
   var nodeslistin=getnodeslist();
   if(nodeslistin==null||nodeslistin.size()==0) {
       Applic.argToaster(context, R.string.nowatchesfound , Toast.LENGTH_LONG);
       if(nodeslistin==null) {
          nodeslistin=new ArrayList<>();
          }
       }
   final var nodeslist=nodeslistin;
   IntConsumer setpos= pos-> {
            try {
                nodenumptr[0]=pos;
                if(nodeslist!=null&&nodeslist.size()>pos) {
                    Node node=pos<0?null:nodeslist.get(pos);
                    remake(sswitch, nswitch,   node);
                    }
                }
            catch(Throwable e) {
                Log.stack(LOG_ID,e);
                }
          };
    var spin=mkspinner(context,nodeslist,setpos);
   EnableControls(parent,false);
    float density=GlucoseCurve.metrics.density;
    var off=(int)(density*6.0f);
    direct.setPadding(off,0,0,0);
    connection.setPadding(off,0,off,0);
    enternums.setPadding(off,0,0,0);
    nwatch.setPadding(0,0,off,0);

   setpos.accept(nodenumptr[0]);
   if(nodeslist==null||nodeslist.isEmpty()) {
     // start.setVisibility(GONE);
      defaults.setVisibility(GONE);
      }
    var layout=new Layout(context,(l,w,h)-> {
        var width=GlucoseCurve.getwidth();
        var height=GlucoseCurve.getheight();
        if(width>w)
            l.setX((width-w)/2);
        if(height>h)
            l.setY((height-h)/2);
        return new int[] {w,h};
        }, new View[]{spin},new View[]{enternums,nphone,nwatch},new View[]{direct,sphone,swatch,connection},new View[]{defaults},new View[]{Help,Ok} );
    int laypad=(int)(density*4.0);
    layout.setPadding(laypad*2,laypad*2,laypad*2,laypad);

    layout.setBackgroundResource(R.drawable.dialogbackground);
    context.addContentView(layout, new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));
    Ok.setOnClickListener(v -> {
        final  var watchnums=nwatch.isChecked();
        final boolean watchdirect=swatch.isChecked(); 
        if(nodenumptr[0]>=0) {
                var node=nodeslist.get(nodenumptr[0]);
                var name=makenodename(node);
                {if(doLog) {Log.i(LOG_ID,"watch nums  "+name+" "+watchnums);};};
                byte[] netinfo=Natives.getmynetinfo(name,true,watchdirect?1:-1,isGalaxy(node),watchnums?1:-1);
                Applic.switchbluetooth(name,netinfo,watchdirect);
                }
            else {
                {if(doLog) {Log.i(LOG_ID,"nodenumptr[0]="+nodenumptr[0]);};};
                }
        context.doonback();
        }
        );

    defaults.setOnClickListener(v -> {
        if(nodenumptr[0]>=0) {
            var sender=tk.glucodata.MessageSender.getMessageSender();
            if(sender!=null) {
                var nod=nodeslist.get(nodenumptr[0]);
                String name=makenodename(nod);
                Runnable setdef=()-> {
                    sender.toDefaults(nod);
                    {if(doLog) {Log.i(LOG_ID,"set to default "+name);};};
                    Natives.setWearosdefaults(name,isGalaxy(nod));
                    var main=MainActivity.thisone;
                    Applic.setbluetooth(main==null?Applic.app:main,true);
                    };
                if(Natives.directsensorwatch(name)<0) {
                    confirmunsynced(context,setdef);
                    }
                else
                    setdef.run();
                }
            }
     });
     /*
    start.setOnClickListener(v -> {
            if(nodenumptr[0]>=0) {
                sendinitwatchapp(nodeslist.get(nodenumptr[0])) ;
                }
         });
*/
    context.setonback(()-> { 
        EnableControls(parent,true);
        removeContentView(layout);
        context.hideSystemUI(); }
        );

    }
static String makenodename(Node node) {
    return node.getId();
    }



private static void confirmunsynced(MainActivity act,Runnable save) {
    AlertDialog.Builder builder = new AlertDialog.Builder(act);
     builder.setTitle(R.string.notsynced).
    setMessage(R.string.lossdata).
           setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
               @Override
               public void onClick(DialogInterface dialog, int which) {
                   save.run();
               }
    }).setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {
            }
        }).show();
    }
static void sendinitwatchapp(Node nod) {
      var sender=tk.glucodata.MessageSender.getMessageSender();
        if(sender==null) {
         Log.e(LOG_ID,"sendintwatchapp getMessageSender()==null");
         return;
         }
      {if(doLog) {Log.i(LOG_ID,"Init watch app");};};
      var nodeName= makenodename(nod);
      Natives.resetbylabel(nodeName,isGalaxy(nod));
      sender.startWearOSActivity(nodeName);
      }
}




