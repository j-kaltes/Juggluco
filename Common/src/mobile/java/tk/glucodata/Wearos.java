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

import static android.view.View.GONE;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static tk.glucodata.Applic.sendbluetooth;
import static tk.glucodata.Log.doLog;
import static tk.glucodata.MessageSender.isGalaxy;
import static tk.glucodata.NumberView.avoidSpinnerDropdownFocus;
import static tk.glucodata.RingTones.EnableControls;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getlabel;
import static tk.glucodata.util.getradiobutton;

import android.content.DialogInterface;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.FrameLayout;
import android.widget.Spinner;
import android.widget.Toast;

import androidx.appcompat.app.AlertDialog;

import com.google.android.gms.wearable.Node;

import java.util.ArrayList;


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


static void remake(CheckDirectionRadio[] sensordirect, CheckDirectionRadio[] nswitch,  Node node,boolean[] direct) {
    int dirval,numsval;
    if(node==null) {
        dirval=-1;
        numsval=-1;
        if(doLog)
            Log.i(LOG_ID,"remake node=null");
        }
    else {
        String name=makenodename(node);
        dirval=Natives.directsensorwatch(name);
        numsval=Natives.hasWatchNums(name);
        if(dirval==0&&!Natives.getusebluetooth()) {
            dirval=-1;
            numsval=-1;
            }
        if(doLog)
            Log.i(LOG_ID,"remake node="+name);
        }
    if(dirval<0)  {
        for(var v:sensordirect) {
            v.setEnabled(false);
            v.setChecked(false);
            }
        direct[0]=false;
        }
    else  {
        boolean watchsensor=dirval!=0; 
        sensordirect[0].setChecked(dirval==0);
        sensordirect[1].setChecked(watchsensor);
        direct[0]=watchsensor;
        for(var v:sensordirect) {
            v.setEnabled(true);
            }
        }
    if(numsval<0)  {
        for(var n:nswitch) {
            n.setEnabled(false);
            n.setChecked(false);
            }
        }
    else  {
        nswitch[0].setChecked(numsval==0);
        nswitch[1].setChecked(numsval!=0);
        for(var n:nswitch)
            n.setEnabled(true);
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
    CheckDirectionRadio[] sswitch={sphone,swatch};



    var enternums=getlabel(context, R.string.enternums);

    var nphone=getradiobutton(context, R.string.phone);
    var nwatch=getradiobutton(context, R.string.watch);
    CheckDirectionRadio[]nswitch={nphone,nwatch}; 
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
   boolean[] watchsensor={false};
   IntConsumer setpos= pos-> {
            try {
                nodenumptr[0]=pos;
                if(nodeslist!=null&&nodeslist.size()>pos) {
                    Node node=pos<0?null:nodeslist.get(pos);
                    remake(sswitch, nswitch,   node,watchsensor);
                    defaults.setEnabled(true);
                    if(node!=null) {  
                       Consumer<View> switched= v-> {
                            defaults.setEnabled(false);
                          };
                        Object[] switcher={switched}; 
                        Backup.setradiotest(nswitch,switcher);
                        Backup.setradiotest(sswitch,switcher);
                        }
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
      defaults.setVisibility(GONE);
      }

 Layout.getMargins(spin).bottomMargin= (int)(density*20.0);
 Layout.getMargins(enternums).bottomMargin= Layout.getMargins(nphone).bottomMargin= Layout.getMargins(nwatch).bottomMargin=(int)(density*20.0);
 Layout.getMargins(Ok).topMargin= Layout.getMargins(defaults).topMargin= Layout.getMargins(Help).topMargin=(int)(density*20.0);
    var layout=new Layout(context,(l,w,h)-> {
    /*
        var width=GlucoseCurve.getwidth();
        var height=GlucoseCurve.getheight();
        if(width>w)
            l.setX((width-w)/2);
        if(height>h)
            l.setY((height-h)/2);
            */
        return new int[] {w,h};
        }, new View[]{spin},new View[]{enternums,nphone,nwatch},new View[]{direct,sphone,swatch,connection},new View[]{Help,defaults,Ok} );
    int laypad=(int)(density*4.0);
    layout.setPadding(laypad*2,laypad*2,laypad*2,laypad);

    layout.setBackgroundResource(R.drawable.dialogbackground);

    var  params =    
            new FrameLayout.LayoutParams(
                    WRAP_CONTENT,
                    WRAP_CONTENT,
                    Gravity.CENTER_HORIZONTAL| Gravity.CENTER);

    context.addMyContentView(layout, params);
    //context.addMyContentView(layout, new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));
//    layout.post(layout::requestLayout);
    Ok.setOnClickListener(v -> {
        if(swatch.isEnabled()) {
            if(defaults.isEnabled()) {
                if(doLog) {Log.i(LOG_ID,"default enabled, so do not save");};
                }
             else {
                final  var watchnums=nwatch.isChecked();
                final boolean watchdirect=swatch.isChecked(); 
                if(nodenumptr[0]>=0) {
                        var node=nodeslist.get(nodenumptr[0]);
                        var name=makenodename(node);
                        final boolean wasdirect=watchsensor[0]; 
                        {if(doLog) {Log.i(LOG_ID,"watch "+name+" "+"nums "+watchnums+" direct "+watchdirect+ " was "+wasdirect);};};
                        byte[] netinfo=Natives.getmynetinfo(name,true,watchdirect?1:-1,isGalaxy(node),watchnums?1:-1);
                        Runnable doswitch=()-> {
                            Applic.switchbluetooth(name,netinfo,watchdirect);
                            };
                        if(Natives.hasAidexX()) {
                            if(!wasdirect&&watchdirect) {
                                SensorBluetooth.afterUnpair(context,res->doswitch.run());
                                }
                            else {
                                if(wasdirect&&!watchdirect)  {
                                   if(sendbluetooth( name,netinfo,false)) {
                                        var unpair=new UnpairOverlayHost(context,R.string.releasingsensor);
                                        unpair.postMessage(context.getString(R.string.unpairingwatch));
                                        context.unpairer=unpair;
                                        context.doswitch=()->Applic.setbluetooth(context,true);
                                        }
                                    }
                                else
                                    doswitch.run();
                                }
                            }
                        else {
                            doswitch.run();
                            }
                        }
                    else {
                        {if(doLog) {Log.i(LOG_ID,"nodenumptr[0]="+nodenumptr[0]);};};
                        }
                   }
                 }
        else {
            if(doLog) {Log.i(LOG_ID,"Not Enabled");};
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
                    context.doonback();
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




