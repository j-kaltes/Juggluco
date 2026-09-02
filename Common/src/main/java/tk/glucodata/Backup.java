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

import androidx.appcompat.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.res.TypedArray;
import android.graphics.BlendMode;
import android.graphics.BlendModeColorFilter;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.text.Editable;
import android.text.InputType;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.text.method.PasswordTransformationMethod;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;

import android.widget.CompoundButton;
import android.widget.EditText;
//import android.widget.HorizontalScrollView;
import android.widget.FrameLayout;
import android.widget.RadioButton;
import android.widget.ScrollView;
import android.widget.Space;
import android.widget.TextView;
import android.widget.Toast;

import java.net.InetAddress;
import java.net.NetworkInterface;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Enumeration;
import java.util.Locale;

import androidx.annotation.ColorInt;
import androidx.annotation.NonNull;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import static tk.glucodata.Layout.getMargins;
import static android.graphics.Color.YELLOW;
import static android.view.View.GONE;
import static android.view.View.INVISIBLE;
import static android.view.View.VISIBLE;
import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static tk.glucodata.Applic.backgroundcolor;
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.BuildConfig.isReleaseID;
import static tk.glucodata.Log.doLog;
import static tk.glucodata.Natives.getICEside;
import static tk.glucodata.Natives.getInvertColors;
import static tk.glucodata.Natives.getWifi;
import static tk.glucodata.Natives.getbackJson;
import static tk.glucodata.Natives.getbackupHasHostname;
import static tk.glucodata.Natives.isWearOS;
import static tk.glucodata.Natives.mirrorStatus;
import static tk.glucodata.RingTones.EnableControls;
import static tk.glucodata.Specific.useclose;
import static tk.glucodata.UseWifi.usewifi;
import static tk.glucodata.help.help;
import static tk.glucodata.help.hidekeyboard;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getcheckbox;
import static tk.glucodata.util.getlabel;
import static tk.glucodata.settings.Settings.editoptions;
import static tk.glucodata.util.getradiobutton;
import static tk.glucodata.util.sethtml;

import tk.glucodata.nums.numio;

//import org.w3c.dom.Text;

public class Backup {
    static final int hide=isWearable?GONE:INVISIBLE;
   static final private String LOG_ID="Backup";
   static class changer implements TextWatcher {
      View view;
      changer(View v) {
         view=v;
         }
       public void    afterTextChanged(Editable s) {}

       public void    beforeTextChanged(CharSequence s, int start, int count, int after) {}

      public void    onTextChanged(CharSequence s, int start, int before, int count) {
         view.setVisibility(VISIBLE);
         }
      }
   static void hideSystemUI(Context cnt) {}
   static public  EditText getedit(Context act, String text) {
      EditText label=new EditText(act);
           label.setInputType(InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD);
           label.setImeOptions(editoptions);
           label.setMinEms(6);
      label.setText(text);
      return label;
      }

   static public  EditText getnumedit(Context act, String text) {
      EditText label=new EditText(act);
      label.setInputType(InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL);
      label.setImeOptions(editoptions);
      label.setMinEms(3);
      label.setText(text);
      return label;
      }
   static String[] gethostnames() {
        String p2p=null;
        String norm=null;
        String bluepan=null;
        String hasone=null;
       try {
               Enumeration<NetworkInterface> inter = NetworkInterface.getNetworkInterfaces();
          if(inter!=null) {
               while(inter.hasMoreElements()) {
                   NetworkInterface in=inter.nextElement();
                   Enumeration<InetAddress> addrs= in.getInetAddresses();
                   while(addrs.hasMoreElements()) {
                       InetAddress a=addrs.nextElement();
                       String sa = a.getHostAddress();
                       String name=in.getName();
                       if(name.startsWith("p2p")) {
                     if(sa!=null&&sa.startsWith("192.168.")) {
                              p2p=sa;
                     hasone=p2p;
                     }
                     }
                       else {
                           if (!in.isVirtual()) {
                               if(name.startsWith("wlan")) {
                                   norm = sa;
                    hasone=norm;
                               } else {
                                   if(name.startsWith("bt-pan")) {
                                       bluepan = sa;
                    hasone=bluepan;
                                      }
                               }
                           }
                       }
                   }

         }
               }
        }
     catch(Throwable e) {
          String mess=e.getMessage() ;
          if(mess==null)
              mess="Network error";
           Log.stack(LOG_ID,mess,e);
           }
      return new String[]{p2p,norm,bluepan,hasone};
      }



   //String defaultport="7345";
   boolean isasender=false;
   boolean[] sendchecked;

      private static final String defaultport= isReleaseID==1?"8795":"9113";
      private    CheckDirectionBox Amounts =null;
      private CheckDirectionBox Scans =null;
      private CheckDirectionBox Stream =null,receive=null,detect=null,checkhostname;
      private CheckDirectionRadio activeonly=null,passiveonly=null,both=null;
      private CheckDirectionRadio transportAutomatic=null,transportTcp=null,transportBluetooth=null,transportMessages=null;
      private boolean transportMessagesAllowed=isWearable;
      private final EditText[] editIPs={null,null,null,null};
      private EditText editpass=null;
      private EditText portedit=null;
      private ScrollView hostview=null;
      private CheckDirectionBox Password;
      private Button reset=null;
        private CheckDirectionBox testip,haslabel;
      private   EditText label;
   private CheckDirectionRadio[] sendfrom;
   private View[] fromrow;
   private Runnable refreshTransportUI;

    private CheckDirectionBox   visible;
      int hostindex=-1;

   public    static void setradio(RadioButton[] radios) {
         for(var but:radios) {
             but.setOnCheckedChangeListener( (buttonView,  isChecked) -> {
                if(isChecked) {
                    for(var b:radios)
                        if(b!=buttonView)
                       b.setChecked(false);
                       }
                    });
             }
        }
      static public void setradiotest(RadioButton[] radios,Object[] ap) {
         for(var but:radios) {
         but.setOnCheckedChangeListener( (buttonView,  isChecked) -> {
            if(isChecked) {
               for(var o:ap) {
                 var a = (Consumer<View>) o;
                  a.accept(buttonView);
               }
            for(var b:radios)
                if(b!=buttonView)
               b.setChecked(false);
               }
            });
         }
        }
      @SuppressWarnings("deprecation")
      public static int agetColor(Context context, int id) {
         if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            return context.getColor(id);
         } else {
            //noinspection deprecation
            return context.getResources().getColor(id);
         }
      }
      @SuppressWarnings("deprecation")
      public static void setColorFilter(@NonNull Drawable drawable, @ColorInt int color) {
         if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            drawable.setColorFilter(new BlendModeColorFilter(color, BlendMode.SRC_ATOP));
         } else {
            drawable.setColorFilter(color, PorterDuff.Mode.SRC_ATOP);
         }
      }
   private void deleteconfirmation(MainActivity act) {
           AlertDialog.Builder builder = new AlertDialog.Builder(act);
   //     setMessage(mess).
      String title=null;
      try {
         title = label.getText().toString();
         if (title == null || title.isEmpty()) {
            if (editIPs[0] != null) {
               title = editIPs[0].getText().toString();
            }
         }
      } catch(Throwable th) {
         Log.stack(LOG_ID,"title",th)    ;
      }
         if(title==null|| title.isEmpty())
            title="  ";
           builder.setTitle(title).setMessage(R.string.deleteconnection).
   //     setMessage(mess).
              setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
                       public void onClick(DialogInterface dialog, int id) {
            if(hostindex>=0) {
               Natives.deletebackuphost(hostindex);
               hostadapt.notifyItemRemoved(hostindex);
               BleMirror.configurationChanged();
               }
   //            hostview.setVisibility(GONE);
   //            hidekeyboard(act); 
            act.doonback();
            act.doonback();
                       }
                   }) .setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
               public void onClick(DialogInterface dialog, int id) {
               }
           }).show().setCanceledOnTouchOutside(false);
      }


static public String changehostError(MainActivity act,int pos) {
            String mess= switch (pos) {
               case -1 : yield act.getString(R.string.portrange);
               case -2 : yield act.getString(R.string.parseip);
               case -3 : yield act.getString(R.string.toomanyhosts);
               case -4 : yield act.getString(R.string.senthosts);
               case -5 : yield "Hostname too long";
            case -6 : yield "Database busy, try again";
               case -7 : yield "Invalid mirror transport or missing connection label";
               default : yield "Error";
            };
            return mess;
            }

   private void resentconfirmation(MainActivity act,int hostindex) {
           AlertDialog.Builder builder = new AlertDialog.Builder(act);
           builder.setTitle(act.getString(R.string.resenddata)+"?").
       setMessage(R.string.resendwarning).
              setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
                       public void onClick(DialogInterface dialog, int id) {
            Natives.resetbackuphost(hostindex);
            configchanged=true;
                       }
                   }) .setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
               public void onClick(DialogInterface dialog, int id) {
               }
           }).show().setCanceledOnTouchOutside(false);
      }


boolean makeQR(MainActivity act,int pos) {
        if(pos<0) {
                var mess= changehostError(act,pos);
                 Applic.argToaster(act,mess,Toast.LENGTH_SHORT);
                 return false;
                }
          else {
                hostadapt.notifyItemInserted(pos);
                final int transport=Natives.getbackuptransport(pos);
                if(transport==BleMirror.TRANSPORT_BLUETOOTH||
                        (transport==BleMirror.TRANSPORT_AUTOMATIC&&!isWearable&&!Natives.isWearOS(pos)))
                   // Refresh/cache permission state before configurationChanged()
                   // is allowed to start BLE scanning or advertising.
                   act.finepermission();
                BleMirror.configurationChanged(pos,true);
                var jsonstr= getbackJson(pos);
                QRmake.show(act,jsonstr);
                return true;
                }
         }

void makeAutoQR(MainActivity act,View parent) {
      EnableControls(parent,false);
      var cancel=getbutton(act,R.string.cancel);
     // var title=getlabel(act, R.string.autoqr);
      var help=getbutton(act,R.string.helpname);
      var send=getlabel(act,R.string.sendto);
      var homenetS=getbutton(act,R.string.homenet);
      var internetS=getbutton(act,R.string.internet);
      var receive=getlabel(act,R.string.receivefrom);
      var homenetR=getbutton(act,R.string.homenet);
      var internetR=getbutton(act,R.string.internet);
      help.setOnClickListener(v-> {
            help(R.string.autoqrmessage,act);
        });
      var layout=new Layout(act, new View[]{send},new View[]{homenetS,internetS},new View[]{receive},new View[]{homenetR,internetR},new View[] {help,cancel});
      layout.setPadding((int)(GlucoseCurve.metrics.density*4.0),(int)(GlucoseCurve.metrics.density*4.0),(int)(GlucoseCurve.metrics.density*4.0),(int)(GlucoseCurve.metrics.density*4));
      layout.setBackgroundColor(backgroundcolor);
    //  layout.measure(WRAP_CONTENT, WRAP_CONTENT);
     // layout.setX((GlucoseCurve.getwidth()-layout.getMeasuredWidth()+MainActivity.systembarLeft-MainActivity.systembarRight)*.5f);
     // layout.setY( (GlucoseCurve.getheight()-layout.getMeasuredHeight() +MainActivity.systembarTop-MainActivity.systembarBottom)*.5f);
    var  params =    new FrameLayout.LayoutParams( WRAP_CONTENT, WRAP_CONTENT, Gravity.CENTER|Gravity.CENTER_HORIZONTAL);

      act.addMyContentView(layout, params);
      layout.setBackgroundResource(R.drawable.dialogbackground);
      Runnable closerun=()->{
         removeContentView(layout);
         EnableControls(parent,true);
         };

      MainActivity.setonback(()->{
        closerun.run();
         });
      cancel.setOnClickListener(v-> {
        MainActivity.doonback();
        });
      homenetS.setOnClickListener(v-> {
            MainActivity.poponback();
            makeQR(act,Natives.makeHomeSender());
            closerun.run();
            });
      internetS.setOnClickListener(v-> {
            MainActivity.poponback();
            makeQR(act,Natives.makeICESender());
            closerun.run();
            });
      homenetR.setOnClickListener(v-> {
            dowhenasked(act,false,false,false,()-> {
                MainActivity.poponback();
                makeQR(act,Natives.makeHomeReceiver());
                closerun.run();
                });
            });
      internetR.setOnClickListener(v-> {
            dowhenasked(act,false,false,false,()-> {
                MainActivity.poponback();
                makeQR(act,Natives.makeICEReceiver());
                closerun.run();
                });
            });
    };
CheckDirectionRadio one;
EditText ICElabel;
CheckDirectionBox ICE;
   void makehostview(MainActivity act) {
      ICE=getcheckbox(act,R.string.ICE, true);
      TextView transportLabel=getlabel(act,R.string.mirror_transport);
      transportAutomatic=getradiobutton(act,R.string.transport_automatic);
      transportTcp=getradiobutton(act,R.string.transport_tcp);
      transportBluetooth=getradiobutton(act,R.string.transport_bluetooth);
      transportMessages=getradiobutton(act,R.string.transport_messages);
      CheckDirectionRadio[] transportRadios={transportAutomatic,transportTcp,transportBluetooth,transportMessages};
      setradio(transportRadios);
      transportAutomatic.setChecked(true);
      transportMessagesAllowed=isWearable;
      transportMessages.setVisibility(transportMessagesAllowed?VISIBLE:GONE);
      for(int i=0;i<editIPs.length;i++) {
         editIPs[i]=new EditText(act);
         editIPs[i].setMinEms(6);
         editIPs[i].setInputType(InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD);
         editIPs[i].setImeOptions(editoptions);
         setColorFilter(editIPs[i].getBackground().mutate(),agetColor(act,android.R.color.holo_blue_light));
         }
     CheckDirectionRadio zero=getradiobutton(act, R.string.zero);
     one=getradiobutton(act, R.string.one);
     var sides=new CheckDirectionRadio[]{zero,one};
     setradio(sides);
     zero.setChecked(true);
     var ICElabellabel=getlabel(act,R.string.icelabel);
     ICElabel = new EditText(act);
     ICElabel.setInputType(InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD);
     ICElabel.setImeOptions(editoptions);
     ICElabel.setMinEms(16);
      portedit=new EditText(act);
      portedit.setInputType(InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL);
      portedit.setImeOptions(editoptions);
      portedit.setMinEms(3);
      Button save=getbutton(act,R.string.save);
      TextView IPslabel=getlabel(act,R.string.ips);
      detect = new CheckDirectionBox(act);
      detect.setText(R.string.detect);
      detect.setOnCheckedChangeListener( (buttonView,  isChecked)-> {
            final int vis=isChecked?hide:VISIBLE;
            final int lastip=editIPs.length-(haslabel.isChecked()?1:0)-1;
            editIPs[lastip].setVisibility(vis);
            });
      detect.setVisibility(hide);

      testip= new CheckDirectionBox(act); testip.setText(R.string.testip);

      haslabel= new CheckDirectionBox(act); haslabel.setText(R.string.testlabel);
      label = new EditText(act);
           label.setInputType(InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD);

           label.setImeOptions(editoptions);
           label.setMinEms(10);

      checkhostname=getcheckbox(act,act.getString(R.string.hostname),false);
      final Runnable doHasName= ()->{
            IPslabel.setVisibility(hide);
            final int lastip= editIPs.length;
            for(var i=1;i<lastip;++i)
               editIPs[i].setVisibility(hide);
            editIPs[0].setMinEms(20);
            detect.setVisibility(hide);
            };

       checkhostname.setOnCheckedChangeListener( (buttonView,  isChecked)-> {
         if(isChecked) {
            Applic.argToaster(act,"hostname is slow",Toast.LENGTH_LONG);
            doHasName.run();
            }
         else {
            IPslabel.setVisibility(VISIBLE);
            detect.setVisibility(VISIBLE);
            final int nrips=editIPs.length-(detect.isChecked()?1:0)-(haslabel.isChecked()?1:0);
            for(var i=1;i<nrips;++i)
               editIPs[i].setVisibility(VISIBLE);
            editIPs[0].setMinEms(6);
            }
         });


      setColorFilter(label.getBackground().mutate(),agetColor(act,android.R.color.holo_red_light));
      haslabel.setOnCheckedChangeListener( (buttonView,  isChecked)-> {
            final int vis=isChecked?VISIBLE:hide;
            label.setVisibility(vis);
            label.requestFocus();
            if(checkhostname.isChecked()||ICE.isChecked())
               return;
            final int vis2=isChecked?hide:VISIBLE;
            final int lastip=editIPs.length-(detect.isChecked()?1:0)-1;
            editIPs[lastip].setVisibility(vis2);
            });

         
      passiveonly=new CheckDirectionRadio(act);
      passiveonly.setText(R.string.passiveonly);
      TextView Portlabel=getlabel(act,R.string.port);
        activeonly = new CheckDirectionRadio(act);
        activeonly.setText(R.string.activeonly);
        both = new CheckDirectionRadio(act);
        both.setText(R.string.both);
        CheckDirectionRadio[] actives={passiveonly,activeonly,both};
      Consumer<View> test1=
      buttonView-> {
         if(buttonView==activeonly)
            detect.setChecked(false);
         final var vis=buttonView==passiveonly?hide:VISIBLE;
         Portlabel.setVisibility(vis);
         portedit.setVisibility(vis);
         final var vis2=(buttonView==activeonly||(buttonView==passiveonly&&!testip.isChecked()))?hide:VISIBLE;
         detect.setVisibility(vis2);
          final var vis3=buttonView==activeonly?hide:VISIBLE;
          testip.setVisibility(vis3);
          if(checkhostname.isChecked()&&buttonView != passiveonly) {
            editIPs[0].setVisibility(VISIBLE);
            doHasName.run();
          }
          else {
              final var vis4 = (buttonView == passiveonly && !testip.isChecked()) ? hide : VISIBLE;
              final int ipnr = editIPs.length - (haslabel.isChecked() ? 1 : 0) - (detect.isChecked() ? 1 : 0);
              for (int i = 0; i < ipnr; i++)
                  editIPs[i].setVisibility(vis4);
             }
      };
      Object[] tests={test1};
        setradiotest(actives,tests);
      testip.setOnCheckedChangeListener( (buttonView,  isChecked)-> {
         final var vis2=(passiveonly.isChecked()&&!isChecked)?hide:VISIBLE;
         final var vis=(activeonly.isChecked()||(passiveonly.isChecked()&&!testip.isChecked()))?hide:VISIBLE;
         detect.setVisibility(vis);
         final int ipnr=editIPs.length-(haslabel.isChecked()?1:0)-(detect.isChecked()?1:0);
         for(int i=0;i<ipnr;i++)
            editIPs[i].setVisibility(vis2);
         });
      receive = new CheckDirectionBox(act);
      receive.setText(R.string.receivefrom);

      TextView Sendlabel=getlabel(act,R.string.sendto);

         Amounts = new CheckDirectionBox(act); Amounts.setText(R.string.amountsname);
         Scans = new CheckDirectionBox(act); Scans.setText(R.string.scansname);
         Stream = new CheckDirectionBox(act); Stream.setText(R.string.streamname);
      CheckDirectionRadio fromnow=new CheckDirectionRadio(act);
      CheckDirectionRadio alldata=new CheckDirectionRadio(act);
      CheckDirectionRadio screenpos=new CheckDirectionRadio(act);
      TextView startlabel=getlabel(act,act.getString(R.string.datapresentuntil));
         alldata.setText(R.string.start);
         fromnow.setText(R.string.now);
      sendfrom=new CheckDirectionRadio[]{alldata,fromnow,screenpos};
       fromrow=new View[]{startlabel, alldata,fromnow,screenpos};

      setradio(sendfrom);
      CheckDirectionBox restore=new CheckDirectionBox(act);restore.setText("Restore");
      if(!Natives.backuphasrestore( ))
         restore.setVisibility(GONE);

      Button Help=getbutton(act,R.string.helpname);
      Help.setOnClickListener(v-> help(R.string.addconnection,act));

      Button delete=getbutton(act,act.getString(R.string.delete));
      Button Close=getbutton(act,R.string.cancel);
          Password = new CheckDirectionBox(act); Password.setText(R.string.password);
         Password.setChecked(true);
       editpass= new EditText(act);
           editpass.setImeOptions(editoptions);
           editpass.setInputType(InputType.TYPE_TEXT_VARIATION_PASSWORD);
      editpass.setTransformationMethod(new PasswordTransformationMethod());
           editpass.setMinEms(6);
          visible = new CheckDirectionBox(act);// visible.setText(R.string.visible);
          visible.setButtonDrawable(R.drawable.password_visible);
//          visible.setButtonDrawable(R.drawable.visibility_toggle);
   //      visible.setMinimumWidth(0); visible.setMinWidth(0);
      visible.setOnCheckedChangeListener( (buttonView,  isChecked)-> {
               var sel=editpass.getSelectionStart();
               editpass.setInputType(isChecked?InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD:InputType.TYPE_TEXT_VARIATION_PASSWORD);
               if(isChecked)
                  editpass.setTransformationMethod(null);
               else
                  editpass.setTransformationMethod(new PasswordTransformationMethod());
               editpass.setSelection(sel);

            });

      Password.setOnCheckedChangeListener( (buttonView,  isChecked)-> {
            final int vis=isChecked?VISIBLE:hide;
            editpass.setVisibility(vis);
            visible.setVisibility(vis);
            });
       Password.setChecked(false); 
       IntSupplier saver= ()-> { 
         final boolean sender= Amounts.isChecked()|| Stream.isChecked()|| Scans.isChecked();
         final boolean receiver=receive.isChecked();
         if(!sender&&!receiver) {
            Applic.argToaster(act, R.string.specifyreceiveordata,Toast.LENGTH_SHORT);
            return -15;
            }
         if(receiver&& Amounts.isChecked()&& Stream.isChecked()&& Scans.isChecked()) {
            Applic.argToaster(act,R.string.allsentnoreceive ,Toast.LENGTH_LONG);
            return -15;
            }        
         hidekeyboard(act); //USE
         int hostnr=Natives.backuphostNr( );
         final int selectedTransport=transportBluetooth.isChecked()?BleMirror.TRANSPORT_BLUETOOTH:
                 (transportMessages.isChecked()?BleMirror.TRANSPORT_MESSAGES:
                 (transportTcp.isChecked()?BleMirror.TRANSPORT_TCP:BleMirror.TRANSPORT_AUTOMATIC));
         final boolean usesNetwork=selectedTransport==BleMirror.TRANSPORT_AUTOMATIC||
                 selectedTransport==BleMirror.TRANSPORT_TCP;
         boolean ice=usesNetwork&&ICE.isChecked();
         int struse=0;
         String[] names=null;
         final boolean dodetect=usesNetwork&&detect.isChecked()&&!activeonly.isChecked();
         final var ICEstring=ICElabel.getText().toString();
         final String selectedLabel=haslabel.isChecked()?label.getText().toString():null;
         if((selectedTransport==BleMirror.TRANSPORT_MESSAGES||selectedTransport==BleMirror.TRANSPORT_BLUETOOTH)&&
                 (selectedLabel==null||selectedLabel.trim().isEmpty())) {
            Applic.argToaster(act,R.string.transport_needs_label,Toast.LENGTH_LONG);
            return -15;
            }
         if(selectedTransport==BleMirror.TRANSPORT_BLUETOOTH&&
                 (!Password.isChecked()||editpass.getText().length()==0)) {
            Applic.argToaster(act,R.string.transport_needs_password,Toast.LENGTH_LONG);
            return -15;
            }
         if(usesNetwork&&ice) {
            if(ICEstring.length()<16) {
                Applic.argToaster(act,R.string.ICElabeltooshort,Toast.LENGTH_LONG);
                return -16;
                }
            }
         else if(usesNetwork) {
             names=new String[editIPs.length];
             if(testip.isChecked()||!passiveonly.isChecked()) {
                for (EditText editText : editIPs) {
                   String name = editText.getText().toString();
                   if (name.length() != 0) {
                      names[struse++] = name;
                   }
                }
                }
             int ipmax=editIPs.length-(dodetect?1:0)-(haslabel.isChecked()?1:0);
             if(struse>=ipmax)
                struse=ipmax;
             if((testip.isChecked()&&!dodetect)||activeonly.isChecked()) {
                if(struse==0) {
                   Applic.argToaster(act, R.string.specifyip,Toast.LENGTH_SHORT);
                   return -15;
                   }
                }
              }


         long starttime=(alldata.getVisibility()!=VISIBLE||alldata.isChecked())?0L:(fromnow.isChecked()? System.currentTimeMillis():Natives.getstarttime())/1000L;
         final boolean mirrorSide=ice?one.isChecked():
                 (hostindex>=0?Natives.getbackupside(hostindex):Scans.isChecked());
         final boolean ordinaryNearby=!ice&&!isWearable&&!(hostindex>=0&&isWearOS(hostindex));
         final boolean bleReverse=ordinaryNearby&&hostindex>=0?
                 Natives.getbackupblereverse(hostindex):false;
         final boolean bleclient;
         if(isWearable)
            bleclient=true;
         else if(hostindex>=0&&isWearOS(hostindex))
            bleclient=false;
         else if(ordinaryNearby)
            // Legacy compatibility field. The real ordinary-mirror role is
            // derived from immutable side plus pair-wide bleReverse.
            bleclient=(!mirrorSide)^bleReverse;
         else if(hostindex>=0)
            bleclient=Natives.getbackupbleclient(hostindex);
         else
            bleclient=!Scans.isChecked();
         int pos=Natives.changebackuphost(hostindex,names,struse,dodetect,usesNetwork?portedit.getText().toString():"0", Amounts.isChecked(),Stream.isChecked(),Scans.isChecked(),restore.isChecked(),receiver,activeonly.isChecked(),passiveonly.isChecked(),Password.isChecked()?editpass.getText().toString():null,starttime,selectedLabel,usesNetwork&&testip.isChecked(),usesNetwork&&checkhostname.isChecked(), ice?ICEstring:null,mirrorSide,selectedTransport,bleclient);

         if(pos<0) {
            String mess=changehostError(act, pos);
            Applic.argToaster(act,mess,Toast.LENGTH_SHORT);
            return pos;
            }    

         if(pos>=0&&ordinaryNearby)
             Natives.setbackupblereverse(pos,bleReverse);
         final boolean storedClient=pos>=0&&ordinaryNearby?
                 ((!Natives.getbackupside(pos))^Natives.getbackupblereverse(pos)):
                 (pos>=0&&Natives.getbackupbleclient(pos));
         Log.i(LOG_ID,"saved mirror index="+pos+" label="+selectedLabel+
                 " requestedTransport="+BleMirror.transportName(selectedTransport)+"("+selectedTransport+")"+
                 " storedTransport="+BleMirror.transportName(Natives.getbackuptransport(pos))+
                 "("+Natives.getbackuptransport(pos)+")"+
                 " bleRole="+(storedClient?"client":"server")+
                 (ordinaryNearby?" direction="+(Natives.getbackupblereverse(pos)?"reversed":"normal"):""));

         if(!receiver&& !(Amounts.isChecked()&& Stream.isChecked()&& Scans.isChecked())) {
            Applic.argToaster(act,R.string.notalldata ,Toast.LENGTH_LONG);
            }        
         configchanged=true;
         final boolean needsBlePermission=selectedTransport==BleMirror.TRANSPORT_BLUETOOTH||
                 (selectedTransport==BleMirror.TRANSPORT_AUTOMATIC&&!isWearable&&!Natives.isWearOS(pos));
         if(needsBlePermission)
            // Refresh/cache permission state before configurationChanged() can
            // start BLE work for a newly enabled mirror.
            act.finepermission();
         BleMirror.configurationChanged(pos,true);
         if(needsBlePermission) {
            final String blocker=BleMirror.blockingStatusForConnection(pos);
            if(blocker!=null)
               Applic.argToaster(act,blocker,Toast.LENGTH_LONG);
            }
         if(pos==hostnr)  {
            delete.setVisibility(VISIBLE);
            hostadapt.notifyItemInserted(pos);
            }
         else
            hostadapt.notifyItemChanged(pos);
         return pos;
         };
      save.setOnClickListener(v->{
           if(saver.getAsInt()>=0)
              act.doonback();
        }); 
      delete.setOnClickListener(v->{ 
         deleteconfirmation(act) ;
         //alarms.setEnabled( Natives.isreceiving( ));
         });
      reset=getbutton(act,R.string.resenddata);
      reset.setOnClickListener(v->{ 
         if(hostindex>=0) {
            resentconfirmation(act,hostindex);
            }
         });
      CheckDirectionBox[] boxes={Amounts,Scans,Stream,restore};
       CompoundButton.OnCheckedChangeListener needport =(buttonView, isChecked)-> {
         if(sendchecked==null)
            return;
         var vis=INVISIBLE;
         for(int i=0;i<3;i++) {
            if(!sendchecked[i]&&boxes[i].isChecked()) {
               vis=VISIBLE;
               }
            }
         for(View v:fromrow)
            v.setVisibility(vis);
         };
      for(CheckDirectionBox vi:boxes) {
         vi.setOnCheckedChangeListener(needport);
         }
     hostview=new ScrollView(act);
      visible.setPaddingRelative(0,0,(int)(GlucoseCurve.metrics.density*5.0),0);
    var iceviews=new View[]{ICE,ICElabellabel,ICElabel,zero,one};
      Sendlabel.setPaddingRelative((int)(GlucoseCurve.metrics.density*10.0),0,0,0);
   Stream.setPaddingRelative(0,0,(int)(GlucoseCurve.metrics.density*5.0),0);
    var firstrow=new View[]{Portlabel, portedit, checkhostname,IPslabel, detect};
    var directions=new View[]{passiveonly, activeonly, both};
      Layout layout;
      if(isWearable) {
         getMargins(save).topMargin=(int)(GlucoseCurve.metrics.density*5.0);
         layout=new Layout(act, (l, w, h) -> {
            hideSystemUI(act);
            final int[] ret={w,h};
            return ret;

         },new View[]{ICE},new View[]{ Portlabel},new View[] {portedit},new View[]{checkhostname},new View[]{new Space(act),IPslabel,detect,new Space(act)},new View[]{ICElabellabel},new View[]{ICElabel},sides, new View[]{editIPs[0]},new View[]{editIPs[1]},editIPs.length>=3?new View[]{editIPs[2]}:null,editIPs.length>=4?new View[]{editIPs[3]}:null ,new View[] {testip},new View[] {haslabel},new View[]{label},
               new View[]{passiveonly},new View[]{activeonly},new View[]{both},new View[] {receive},new View[] {Sendlabel,Stream},new View[]{Scans,Amounts},new View[]{startlabel},new View[]{alldata,fromnow},new View[]{screenpos} ,new View[]{Password },new View[]{editpass,visible}, new View[]{transportLabel},new View[]{transportAutomatic},new View[]{transportTcp},new View[]{transportBluetooth},new View[]{transportMessages},new View[]{delete,Close},new View[] {reset},new View[]{save});

      layout.setPaddingRelative((int)(GlucoseCurve.metrics.density*4.0),0,(int)(GlucoseCurve.metrics.density*10.0),(int)(GlucoseCurve.metrics.density*4));
         }
      else {
        var hormargin=(int)(GlucoseCurve.metrics.density*20.0f);

         getMargins(delete).setMarginStart(hormargin);
         getMargins(save).setMarginEnd(hormargin);;
        var withqr=BuildConfig.minSDK>=20?new View[]{Password, editpass, visible}:new View[]{Password, editpass, visible};
         var sidepad=(int)(GlucoseCurve.metrics.density*8.0);
         layout = new Layout(act, (l, w, h) -> {
            hideSystemUI(act);
            final int[] ret = {w, h};
            return ret;

         }, firstrow,new View[]{ICE,ICElabellabel,ICElabel,zero,one},editIPs, new View[]{testip, haslabel, label},
               directions, new View[]{receive, Sendlabel, Amounts, Scans, Stream, restore}, fromrow, withqr,new View[]{transportLabel,transportAutomatic,transportTcp,transportBluetooth,transportMessages}, new View[]{delete, Close, reset, Help, save})
            .portraitLayout(
new View[]{Portlabel,portedit,checkhostname},new View[]{IPslabel,detect},
               new View[]{ICE,ICElabellabel},new View[]{ICElabel},new View[]{zero,one},
               new View[]{editIPs[0],editIPs[1]},new View[]{editIPs[2],editIPs[3]},
               new View[]{testip,haslabel},new View[]{label},directions,
               new View[]{receive,Sendlabel},new View[]{Amounts,Scans,Stream},new View[]{restore},
               new View[]{startlabel},new View[]{alldata,fromnow,screenpos},
               withqr,
               new View[]{transportLabel,transportAutomatic},new View[]{transportTcp,transportBluetooth,transportMessages},

               new View[]{Close,Help},new View[]{reset},new View[]{delete,save})
            .systembarPadding((left,top,right,bottom)->
               new int[]{left+sidepad,top/2,sidepad+right,bottom});

         }
      Close.setOnClickListener(v-> act.doonback());
      hostview.addView(layout);
      hostview.setFillViewport(true);
      hostview.setSmoothScrollingEnabled(true);
       hostview.setVerticalScrollBarEnabled(Applic.scrollbar);
       hostview.setScrollbarFadingEnabled(true);
       act.addMyContentView(hostview, new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.MATCH_PARENT));
       hostview.setBackgroundColor(backgroundcolor);
    Consumer<Boolean> setICE=(isChecked) -> {
            // ICE is always a network/TURN connection. Automatic and TCP/IP are
            // therefore equivalent for ICE, while Bluetooth/Messages cannot be
            // used at all. There is no meaningful transport choice: hide the
            // complete transport section while ICE is selected. Keep the checked
            // network radio internally so existing stored connections keep their
            // transport value.
            if(isChecked&&(transportBluetooth.isChecked()||transportMessages.isChecked()))
               transportAutomatic.setChecked(true);
            final int transportVisibility=isChecked?GONE:VISIBLE;
            transportLabel.setVisibility(transportVisibility);
            transportAutomatic.setVisibility(transportVisibility);
            transportTcp.setVisibility(transportVisibility);
            transportBluetooth.setVisibility(transportVisibility);
            transportMessages.setVisibility(!isChecked&&transportMessagesAllowed?VISIBLE:GONE);

            final int vis=isChecked?VISIBLE:hide;
            for(int i=1;i<iceviews.length;++i) {
                iceviews[i].setVisibility(vis);
                }
            final int notvis=!isChecked?VISIBLE:hide;
            for(var el:firstrow) {
                el.setVisibility(notvis);
                }
            final int nrips=editIPs.length-(detect.isChecked()?1:0)-(haslabel.isChecked()?1:0);
            for(var i=0;i<nrips;++i)
               editIPs[i].setVisibility(notvis);
            for(var el:directions) {
                el.setVisibility(notvis);
                }
            testip.setVisibility(notvis);
            };

      refreshTransportUI=()-> {
         final boolean network=transportAutomatic.isChecked()||transportTcp.isChecked();
         if(network) {
            ICE.setEnabled(true);
            haslabel.setEnabled(true);
            ICE.setVisibility(VISIBLE);
            setICE.accept(ICE.isChecked());
            if(!ICE.isChecked()) {
               final View selected=activeonly.isChecked()?activeonly:(passiveonly.isChecked()?passiveonly:both);
               test1.accept(selected);
               if(checkhostname.isChecked())
                  doHasName.run();
               label.setVisibility(haslabel.isChecked()?VISIBLE:hide);
               }
            }
         else {
            ICE.setChecked(false);
            ICE.setEnabled(false);
            for(var view:firstrow)
               view.setVisibility(hide);
            for(var view:iceviews)
               view.setVisibility(hide);
            for(var view:editIPs)
               view.setVisibility(hide);
            for(var view:directions)
               view.setVisibility(hide);
            testip.setVisibility(hide);
            haslabel.setChecked(true);
            haslabel.setEnabled(false);
            label.setVisibility(VISIBLE);
            }
         };
      Consumer<View> transportChange=view->refreshTransportUI.run();
      setradiotest(transportRadios,new Object[]{transportChange});

      ICE.setOnCheckedChangeListener( (buttonView,  isChecked)-> {
            setICE.accept(isChecked);
            });
     ICE.setChecked(false);
     refreshTransportUI.run();
      // setICE.accept(false);
      }
   void changehostview(MainActivity act,final int index,String[] names,boolean dodetect,String port,String pass,View parent) {
         parent.setVisibility(GONE);
      if(hostview==null)
         makehostview(act);
      else {
         hostview.setVisibility(VISIBLE);
         hostview.bringToFront();
         visible.setChecked(false);
         }
      act.setonback(() -> {
            parent.setVisibility(VISIBLE);
            hidekeyboard(act);
            hostview.setVisibility(GONE);
            });
      boolean stream,scans,amounts;
      String ICElabelstr=Natives.getICElabel(index);
      boolean hasICE=ICElabelstr!=null;

      boolean isnew=index<0;
      final int savedTransport=isnew?BleMirror.TRANSPORT_AUTOMATIC:Natives.getbackuptransport(index);
      transportMessagesAllowed=isWearable||(!isnew&&isWearOS(index))||
              savedTransport==BleMirror.TRANSPORT_MESSAGES;
      transportMessages.setVisibility(transportMessagesAllowed?VISIBLE:GONE);
      String labelstr=null;
      if(!isnew) {
         stream=Natives.getbackuphoststream(index);
         scans=Natives.getbackuphostscans(index);
         amounts=Natives.getbackuphostnums(index);
         int recnum=Natives.getbackuphostreceive(index);
         boolean doreceive= (recnum&2)!=0;
         receive.setChecked(doreceive);
         labelstr=Natives.getbackuplabel(index);
         if(labelstr!=null) {
            label.setText(labelstr);
            haslabel.setChecked(true); 
              }
          else {
            label.setText("");
            haslabel.setChecked(false); 
            label.setVisibility(hide);
            }
          if((savedTransport==BleMirror.TRANSPORT_AUTOMATIC||savedTransport==BleMirror.TRANSPORT_TCP)&&!hasICE) {
             final boolean dotestip=Natives.getbackuptestip(index);
             final boolean ispassive=Natives.getbackuphostpassive(index);
             testip.setChecked(dotestip);
             final var vis=(ispassive&&!dotestip)?hide:VISIBLE;
             detect.setChecked(dodetect);
             final boolean hasHostname=getbackupHasHostname(index);
             int maxhosts=hasHostname?1:(editIPs.length-(dodetect?1:0)-(labelstr==null?0:1));
              for(int i=0;i<Math.min(names.length,maxhosts);i++) {
                   editIPs[i].setText(names[i]);
                }
                for(int i=0;i<maxhosts;i++)
                       editIPs[i].setVisibility(vis);
                boolean isactiveonly =Natives.getbackuphostactive(index);
                detect.setVisibility((ispassive&&!dotestip||isactiveonly)?hide:VISIBLE);
             if(isactiveonly)
                activeonly.setChecked(true);
             else {
                if(ispassive) {
                   passiveonly.setChecked(true);
                   }
                else
                   both.setChecked(true);
                }
             boolean iswearos=isWearOS(index);
             {if(doLog) {Log.i(LOG_ID,(labelstr!=null?labelstr:"")+" Iswearos("+index+")="+iswearos);};};

             checkhostname.setChecked(hasHostname);
             }
          else if(!hasICE) {
             testip.setChecked(Natives.getbackuptestip(index));
             if(names!=null) {
                for(int i=0;i<Math.min(names.length,editIPs.length);++i)
                   editIPs[i].setText(names[i]);
                }
             final boolean ispassive=Natives.getbackuphostpassive(index);
             final boolean isactiveonly=Natives.getbackuphostactive(index);
             if(isactiveonly)
                activeonly.setChecked(true);
             else if(ispassive)
                passiveonly.setChecked(true);
             else
                both.setChecked(true);
             checkhostname.setChecked(false);
             detect.setChecked(false);
             }
          }
      else {
         stream=false;scans=false;amounts=false;
         haslabel.setChecked(false);
         receive.setChecked(false);
         label.setVisibility(hide);
         label.setText("");
         }
      if(isnew||hasICE) {
         checkhostname.setChecked(false);
         detect.setChecked(false);
         both.setChecked(true);
         testip.setChecked(true);
          } 

      Stream.setChecked(stream); Scans.setChecked(scans); Amounts.setChecked(amounts);
      isasender=stream||scans||amounts;
      sendchecked=new boolean[]{amounts,scans,stream};
      sendfrom[2].setText( tk.glucodata.util.timestring(Natives.getstarttime()));
      if(!isasender) {
         reset.setVisibility(INVISIBLE);
         }
      else {
         reset.setVisibility(VISIBLE);
         }
      sendfrom[0].setChecked(true);
      for(View v:fromrow) v.setVisibility(GONE);
      if(!hasICE) {
          for(int i=names==null?0:names.length;i<editIPs.length;i++) editIPs[i].setText("");
          portedit.setText(port);
          ICE.setChecked(false);
          ICElabel.setText("");
          one.setChecked(false);
          }
      else {
        ICElabel.setText(ICElabelstr);
        boolean side= getICEside(index);
        one.setChecked(side);
        ICE.setChecked(true);
        }
      if(pass!=null&&pass.length()>0) {
          editpass.setText(pass);
          Password.setChecked(true);
          editpass.setVisibility(VISIBLE);
          }
      else {
         editpass.setText("");
         Password.setChecked(false);
         editpass.setVisibility(hide);
         }

      if(savedTransport==BleMirror.TRANSPORT_BLUETOOTH)
         transportBluetooth.setChecked(true);
      else if(savedTransport==BleMirror.TRANSPORT_MESSAGES)
         transportMessages.setChecked(true);
      else if(savedTransport==BleMirror.TRANSPORT_TCP)
         transportTcp.setChecked(true);
      else
         transportAutomatic.setChecked(true);
      refreshTransportUI.run();

      hostindex=index;
      }
   void changehostview(MainActivity act,int index,View parent) {
      String[] names=Natives.getbackupIPs(index);
      String port=Natives.getbackuphostport(index);
      String pass= Natives.getbackuppassword(index);
      changehostview(act,index,names,Natives.detectIP(index),port,pass, parent) ;
      }

   void        showhostinfo(final MainActivity act,final View parview,int pos) {
   if(!isWearable)
         EnableControls(parview,false);
      var close=getbutton(act,R.string.closename);
      var modify=getbutton(act,R.string.modify);


      var info=new TextView(act);
      final int pad=(int)(GlucoseCurve.metrics.density*7.0);
      if(!isWearable) info.setPadding(pad,0,pad,0);

      var deactive=getcheckbox(act,R.string.off,Natives.getHostDeactivated(pos));
      deactive.setOnCheckedChangeListener( (buttonView,  isChecked)->  {
                Natives.setHostDeactivated(pos,isChecked);
                hostadapt.notifyItemChanged(pos);
                BleMirror.configurationChanged();
               }
                );
      String mirrorInfo=mirrorStatus(pos);
      final String bluetoothStatus=BleMirror.statusForConnection(pos);
      if(bluetoothStatus!=null)
         mirrorInfo+="<p><b>Bluetooth mirror:</b> "+TextUtils.htmlEncode(bluetoothStatus)+"</p>";
      sethtml(info,mirrorInfo);

      ViewGroup layall;
ViewGroup.LayoutParams params;
      if(isWearable) {
          if(!useclose) close.setVisibility(GONE);
         var space1=new Space(act);
         var space2=getlabel(act,"      ");
          Layout layout=new Layout(act,new View[]{space1,deactive,modify,space2}, new View[]{info},new View[]{close});
   //      layout.round=true;
         layout.setBackgroundColor(Applic.backgroundcolor);
         var leftpad=(int)(GlucoseCurve.getwidth()*.1);
         layout.setPaddingRelative(leftpad,leftpad,(int)(GlucoseCurve.getwidth()*0.08), leftpad*2);
         var scroll= new ScrollView(act);
         scroll.setFillViewport(true);
         scroll.setVerticalScrollBarEnabled(true);
         scroll.setScrollbarFadingEnabled(true);
         scroll.setSmoothScrollingEnabled(true);
         scroll.addView(layout);
         layall=scroll;
        params=new ViewGroup.LayoutParams(MATCH_PARENT, MATCH_PARENT);
         }
      else {
           var modmar=Layout.getMargins(modify);
           var hormar= (int)(GlucoseCurve.metrics.density*10);
           modmar.setMarginStart(hormar);
           var closemar=Layout.getMargins(close);
           closemar.setMarginEnd(hormar);
           View[] firstrow;
           if(BuildConfig.minSDK>=20) {
                Button qr=getbutton(act,"QR");
                qr.setOnClickListener(v->  {
                      if(pos>=0) {
                            String jsonstr=getbackJson(pos);
                            QRmake.show(act,jsonstr);
                            }
                     });
                    firstrow=new View[]{modify,deactive,qr,close} ;
                     }
               else {
                     firstrow= new View[]{modify,deactive,close} ;
                    }
                                                                                                
           Layout layout=new Layout(act, (l, w, h) -> {
                final int[] lret={w,h};
                return lret;
                },firstrow , new View[]{info})
                .systembarMargins((left,top,right,bottom)->
                    new int[]{left*7/8,top*7/8,right*7/8,0});
            layout.setBackgroundResource(R.drawable.dialogbackground);
            layall=layout;
           params =    new FrameLayout.LayoutParams( WRAP_CONTENT, WRAP_CONTENT, Gravity.RIGHT);
            }

      modify.setOnClickListener(v->     changehostview(act,pos,layall));
      act.addMyContentView(layall, params);
      if(isWearable) {
         var margs=getMargins(layall);
         margs.topMargin=MainActivity.systembarTop*3/4;
         margs.leftMargin=MainActivity.systembarLeft*3/4;
         margs.rightMargin=MainActivity.systembarRight*3/4;
         }
      Runnable closerun= ()-> {
         removeContentView(layall);

   if(!isWearable)
         EnableControls(parview,true);
         };
      act.setonback(closerun);    
      close.setOnClickListener(v->  {
         act.poponback();    
         closerun.run();
         });
      }
   void addhostview(MainActivity act,View parent) {
      changehostview(act,-1,null,false,defaultport,"",parent) ;
      }


   HostViewAdapter hostadapt;
//   Button alarms;
   public  void mkbackupview(MainActivity act) {
      act.themeLightBars();
      act.showui=true;
      if(!isWearable&&!Natives.getsystemUI()) {
         act.showSystemUI();
         Applic.app.getHandler().postDelayed( ()->{
         realmkbackupview(act,true); },1);
         }
       else
         realmkbackupview(act,true);
   //    Applic.app.getHandler().postDelayed( ()-> realmkbackupview(act),1); //for what was it needed?
      }
   public  void realmkbackupview(MainActivity act,boolean lightback) {
   configchanged=false;
    // activity=act;
    String[] thishost=gethostnames();
    if(thishost[3]!=null)
     Natives.networkpresent();
     TextView ip= isWearable? getlabel(act,thishost[1]==null?"wlan: null":thishost[1]): getlabel(act,"wlan: "+thishost[1]);
//     View p2p= (thishost[0]==null)?new Space(act):getlabel(act,"p2p: "+thishost[0]);
 //    View blpan= (thishost[2]==null)?new Space(act):getlabel(act,"bt-pan: "+thishost[2]);
     View p2p= (thishost[0]==null)?null:getlabel(act,"p2p: "+thishost[0]);
     View blpan= (thishost[2]==null)?null:getlabel(act,"bt-pan: "+thishost[2]);
     String port=Natives.getreceiveport();
     TextView labport=getlabel(act,R.string.port);
     EditText portview=getnumedit(act, port);

     portview.setMinEms(2);

     Button hosts=getbutton(act,R.string.addconnectionbutton);
     Button Help=getbutton(act,R.string.helpname);
      Help.setOnClickListener(v->
         help(R.string.connectionoverview,act) );

     Button Sync=getbutton(act,act.getString(R.string.sync));
      Sync.setOnClickListener(v-> {
          Applic.switchSync();
          });
     Button reinit=getbutton(act,R.string.reinit);
      reinit.setOnClickListener(v-> {
      MessageSender.reinit();
      }
      );

     final Button battery = new Button(act);


     Button Cancel=getbutton(act,R.string.closename);
      Button Save=getbutton(act,R.string.save);
      Save.setVisibility(INVISIBLE);
      changer ch=new changer(Save);
      portview.addTextChangedListener(ch);
      RecyclerView recycle = new RecyclerView(act);
      LinearLayoutManager lin = new LinearLayoutManager(act);
      recycle.setLayoutManager(lin);

      CheckDirectionBox staticnum = new CheckDirectionBox(act);
      staticnum.setOnCheckedChangeListener( (buttonView,  isChecked)-> {
        Natives.setstaticnum(isChecked);
        if(!isWearable) {
            if(isChecked) {
                BluetoothGlucoseMeter.stopDevices();
                }
            else {
                BluetoothGlucoseMeter.startDevices();
                }
            }
        });

      staticnum.setText(R.string.dontchangeamounts);
      staticnum.setChecked(Natives.staticnum());
      if(!isWearable) {
         var lineheight=staticnum.getLineHeight();
         recycle.setMinimumHeight(lineheight*6);
         }
      else {
             recycle.setPadding(0,(int)(GlucoseCurve.metrics.density*7.0),0,(int)(GlucoseCurve.metrics.density*3.0));
         }
      View lay;

      var errstr=Natives.serverError();
      var errorrow=errstr.length()>0?new View[]{getlabel(act,errstr)}:null;
      var turnserver=getbutton(act,R.string.turnserver);
      if(isWearable) {
         CheckDirectionBox wifi=getcheckbox(act,act.getString(R.string.wifi),getWifi());
         wifi.setOnCheckedChangeListener( (buttonView,  isChecked)-> {
            Natives.setWifi(isChecked);
            if(isChecked) {
               usewifi(); 
               }
            else
               UseWifi.stopusewifi();
            });
         if(!useclose) Cancel.setVisibility(INVISIBLE);
         final var width=GlucoseCurve.getwidth();
         getMargins(labport).setMarginStart((int)(width*0.12));
         getMargins(Save).setMarginEnd((int)(width*0.12));
         var margIP=getMargins(ip);
         margIP.setMarginStart((int)(width*0.01));
         final Layout layout=new Layout(act, new View[]{getlabel(act,act.getString(R.string.thishost))},new View[]{labport,portview,Save},new View[]{ip},new View[]{blpan},new View[]{p2p},new View[]{recycle},new View[] {hosts},new View[]{staticnum},new View[]{Sync,reinit},new View[]{wifi},errorrow,new View[]{Cancel});
         var hori=new ScrollView(act);
         hori.setFillViewport(true);
         hori.setVerticalScrollBarEnabled(Applic.scrollbar);
         hori.setScrollbarFadingEnabled(true);
         hori.setSmoothScrollingEnabled(true);
         int height=GlucoseCurve.getheight();
         hori.setMinimumHeight(height);
         hori.addView(layout);
         lay=hori;
         int pad=(int)(GlucoseCurve.metrics.density*5);
         layout.setPaddingRelative((int)(GlucoseCurve.metrics.density*6),pad,(int)(GlucoseCurve.metrics.density*9),pad);
         }
      else {
        Button autoqr;
         if(BuildConfig.minSDK>=20) {
             autoqr=getbutton(act,R.string.autoqr);
            }
         else {
            autoqr=null;
            }
         var hormarg=(int)(GlucoseCurve.metrics.density*20.0f);
         getMargins(Help).setMarginStart(hormarg);
         getMargins(Cancel).setMarginEnd(hormarg);
         var withqr=BuildConfig.minSDK>=20?new View[]{autoqr,hosts,Cancel}:new View[]{hosts,Cancel};
         var layout=new Layout(act, new View[]{ip,blpan,p2p,labport,portview,Save,turnserver},new View[]{recycle},new View[] {battery,Help,Sync,reinit,staticnum},errorrow,withqr)
            .portraitLayout(new View[]{ip},new View[]{blpan},new View[]{p2p},
               new View[]{labport,portview,Save},new View[]{turnserver},new View[]{recycle},errorrow,
               new View[]{battery,staticnum},BuildConfig.minSDK>=20?new View[]{autoqr,Sync,reinit}:new View[]{Sync,reinit},
               new View[]{hosts},new View[]{Help,Cancel});
        if(BuildConfig.minSDK>=20) {
            autoqr.setOnClickListener(v -> {
                makeAutoQR(act, layout);
                });
          };

       var density=GlucoseCurve.metrics.density;
      layout.systembarPadding((left,top,right,bottom)->new int[]{
         left+(int)(density*10),top*5/6,right+(int)(density*10),bottom+(int)(density*3)});

          {if(doLog) {Log.i(LOG_ID,"density="+GlucoseCurve.metrics.density+" systembarTop="+ MainActivity.systembarTop+" systembarLeft="+ MainActivity.systembarLeft);};};
         lay=layout;
         }

      Save.setOnClickListener(v->  {
         Natives.setreceiveport(portview.getText().toString());
         Save.setVisibility(GONE);
         hidekeyboard(act);
      });

      turnserver.setOnClickListener(v->  {
        TurnServer.show(act,lay);
         });
         //alarms.setOnClickListener(v-> tk.glucodata.settings.Settings.alarmsettings(act,lay,issaved));
         hosts.setOnClickListener(v-> addhostview(act,lay));
      hostadapt = new HostViewAdapter(lay); //USE
      recycle.setAdapter(hostadapt);
      recycle.setLayoutParams(new ViewGroup.LayoutParams(  MATCH_PARENT, WRAP_CONTENT));
      Runnable closerun= ()-> {
          if(lightback) act.lightBars(!getInvertColors( ));
         if(hostview!=null)
            removeContentView(hostview);
         hidekeyboard(act);
         removeContentView(lay);
         if(configchanged)  {
            Natives.resetnetwork();
            Applic.wakemirrors();
            }
         Applic.updateservice(act,Natives.getusebluetooth());
         act.showui=false;
         if(!isWearable)
            Applic.app.getHandler().postDelayed(act::hideSystemUI,1);
         if(Menus.on)
            Menus.show(act);

         };
      act.setonback(closerun);    
      Cancel.setOnClickListener(v->  {
         act.poponback();    
         closerun.run();
         });

      if(!isWearable&&android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.M) {
         battery.setText(R.string.dozemode);
         battery.setOnClickListener(v-> {
            Battery.batteryscreen(act,lay);
            });
         }
      else {
         battery.setVisibility(GONE);
      }
      lay.setBackgroundColor(Applic.backgroundcolor);
   //   act.themeLightBars();
      act.addMyContentView(lay, new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.MATCH_PARENT));
      }


     class HostViewHolder extends RecyclerView.ViewHolder {
       public HostViewHolder(View view,View parent) {
         super(view);
         view.setOnClickListener(v -> {
             int pos=getAbsoluteAdapterPosition();
             showhostinfo((MainActivity)(v.getContext()),parent,pos);
             });

       }

   }
    public class HostViewAdapter extends RecyclerView.Adapter<HostViewHolder> {
      View pview;
         HostViewAdapter(View parent) {
         this.pview=parent;
         }


private int getMirrorListColor(MainActivity act) {
    if(act.mirrorlistcolor==-1) {
        int[] attrs = new int[] { R.attr.colorMirrorConnection };

        try(TypedArray typedArray = act.obtainStyledAttributes(attrs)) {
            act.mirrorlistcolor = typedArray.getColor(0, android.graphics.Color.RED);
 //           typedArray.recycle();
            }
        }
    return act.mirrorlistcolor;
    }
       @NonNull
      @Override
       public HostViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
         TextView view=new TextView( parent.getContext());

          view.setAccessibilityDelegate(tk.glucodata.Layout.accessDeli);
   //        view.setTextSize(TypedValue.COMPLEX_UNIT_SP, 24f);
            // view.setTextSize(TypedValue.COMPLEX_UNIT_PX,Applic.largefontsize);
         view.setLayoutParams(new ViewGroup.LayoutParams(  ViewGroup.LayoutParams.MATCH_PARENT, WRAP_CONTENT));
         if(isWearable) {
            final var af=(int)(GlucoseCurve.metrics.density*12.0);
             view.setGravity(Gravity.CENTER);
             view.setPadding(0,0,0,af);
             }
         else {
            final var af=(int)(GlucoseCurve.metrics.density*7.5);
//            view.setTextSize(TypedValue.COMPLEX_UNIT_PX,isWearable?Applic.mediumfontsize:Applic.largefontsize);
            view.setTextSize(TypedValue.COMPLEX_UNIT_PX,Applic.largefontsize);
             view.setGravity(Gravity.LEFT);
             view.setPaddingRelative((int)(GlucoseCurve.metrics.density*10.0),0,0,af);
             }
           view.setTextColor(getMirrorListColor((MainActivity)view.getContext()));
           return new HostViewHolder(view,pview);

       }

   private static final DateFormat hhmm=             new SimpleDateFormat("HH:mm", Locale.US );
      @Override
      public void onBindViewHolder(final HostViewHolder holder, int pos) {
         TextView text=(TextView)holder.itemView;
         String[] names =Natives.getbackupIPs(pos);
          StringBuilder sb = new StringBuilder();
         String port=Natives.getbackuphostport(pos);
         long date=Natives.lastuptodate(pos);
         boolean passive=Natives.getbackuphostpassive(pos);
         String label=Natives.getbackuplabel(pos);
         boolean stream=Natives.getbackuphoststream(pos);
         boolean scans=Natives.getbackuphostscans(pos);
         boolean amounts=Natives.getbackuphostnums(pos);
         int recnum=Natives.getbackuphostreceive(pos);
         boolean off=Natives.getHostDeactivated(pos);
         int transport=Natives.getbackuptransport(pos);
         boolean doreceive= (recnum&2)!=0;
         String ICElabelstr=Natives.getICElabel(pos);
      if(ICElabelstr!=null&&ICElabelstr.length()<16) {
           text.setText(R.string.ICElabeltooshort);
           }
       else  {
         if(off)
             text.setPaintFlags(text.getPaintFlags() | Paint.STRIKE_THRU_TEXT_FLAG);
         else
             text.setPaintFlags(text.getPaintFlags() & ~Paint.STRIKE_THRU_TEXT_FLAG);
         if(label!=null) {
            sb.append(label);
            sb.append(" ");
            }
       if(isWearable&&transport==BleMirror.TRANSPORT_BLUETOOTH)
          sb.append("[Bluetooth] ");
       else if(isWearable&&transport==BleMirror.TRANSPORT_MESSAGES)
          sb.append("[Messages] ");
       else if(isWearable&&transport==BleMirror.TRANSPORT_TCP)
          sb.append("[TCP only] ");
       if(!isWearable) {
          if(transport==BleMirror.TRANSPORT_BLUETOOTH) {
               sb.append("[Bluetooth] ");
               }
          else if(transport==BleMirror.TRANSPORT_MESSAGES) {
               sb.append("[Messages] ");
               }
          else if(transport==BleMirror.TRANSPORT_TCP) {
               sb.append("[TCP only] ");
               }
          if(transport==BleMirror.TRANSPORT_AUTOMATIC||transport==BleMirror.TRANSPORT_TCP) {
             if(ICElabelstr==null) {
                  sb.append((names!=null&&names.length!=0)?names[0]:(Natives.detectIP(pos)?"Detect":"---"));
                  if(!passive) {
                     sb.append(" ");
                     sb.append(port);
                     }
                  sb.append(' ');
                  }
             else
                  sb.append(" ICE ");
             }
           }
          if(amounts) {
              sb.append("n");
              }
          if(scans) {
              sb.append("s");
              }
          if(stream) { 
              sb.append("b");
            }
          if(doreceive) { 
              sb.append("r");
            }
         if(date!=0L) {
            String str=isWearable?hhmm.format(date):bluediag.datestr(date);

            sb.append("   \u21CB ").append(str);
            }
         text.setText(sb);
         }
      }
           @Override
           public int getItemCount() {
         return Natives.backuphostNr( );

           }

   }
   boolean configchanged=false;

static private String mkreceiveString(Context act,boolean nums, boolean scans, boolean stream) {
        String type=""; 
        if(!nums&& numio.hasNumdata()) {
            type=act.getString(R.string.amountsname); 
            }
        if(!scans&&Natives.hasscans()) {
            final var addstr=act.getString(R.string.scansname); 
            if(!type.isEmpty())
                type+=", "+addstr; 
            else type=addstr; 
            }
        if(!stream&&Natives.hasstreamed( )) {
            final var addstr=act.getString(R.string.streamname); 
            if(!type.isEmpty())
                type+=", "+addstr; 
            else type=addstr; 
            }
        return type;
       }
static void dowhenasked(Context act,boolean nums,boolean scans,boolean stream, Runnable save) {
        String type=mkreceiveString(act,nums,scans,stream) ; 
        if(!type.isEmpty()) {
                Confirm.ask(act,act.getString(R.string.datapresent)+type,act.getString(R.string.overwrite),save);
                return;
               } 
         save.run();
         }
}
