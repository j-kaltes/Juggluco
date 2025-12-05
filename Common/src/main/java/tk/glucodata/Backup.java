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

import android.annotation.SuppressLint;
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
import android.text.TextWatcher;
import android.text.method.PasswordTransformationMethod;
import android.text.method.ScrollingMovementMethod;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.EditText;
//import android.widget.HorizontalScrollView;
import android.widget.RadioButton;
import android.widget.ScrollView;
import android.widget.Space;
import android.widget.TextView;
import android.widget.Toast;

import java.lang.reflect.Method;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Arrays;
import java.util.Enumeration;
import java.util.Locale;

import androidx.annotation.ColorInt;
import androidx.annotation.NonNull;
import androidx.core.widget.NestedScrollView;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import static tk.glucodata.Layout.getMargins;
import static android.graphics.Color.WHITE;
import static android.graphics.Color.GREEN;
import static android.graphics.Color.YELLOW;
import static android.view.View.GONE;
import static android.view.View.INVISIBLE;
import static android.view.View.VISIBLE;
import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static tk.glucodata.Applic.backgroundcolor;
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.BuildConfig.isReleaseID;
import static tk.glucodata.GlucoseCurve.width;
import static tk.glucodata.Log.doLog;
import static tk.glucodata.Natives.getBlueMessage;
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
import static tk.glucodata.Applic.isRelease;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getcheckbox;
import static tk.glucodata.util.getlabel;
import static tk.glucodata.settings.Settings.editoptions;
import static tk.glucodata.util.getradiobutton;
import static tk.glucodata.util.sethtml;

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
      private    CheckBox Amounts =null;
      private CheckBox Scans =null;
      private CheckBox Stream =null,receive=null,detect=null,checkhostname;
      private RadioButton activeonly=null,passiveonly=null,both=null;
      private final EditText[] editIPs={null,null,null,null};
      private EditText editpass=null;
      private EditText portedit=null;
      private ScrollView hostview=null;
      private CheckBox Password;
      private Button reset=null;
        private CheckBox testip,haslabel;
      private   EditText label;
   private RadioButton[] sendfrom;
   private View[] fromrow;

    private CheckBox   visible;
      int hostindex=-1;

   public    static void setradio(RadioButton[] radios) {
         for(RadioButton but:radios) {
         but.setOnCheckedChangeListener( (buttonView,  isChecked) -> {
            if(isChecked) {
            for(RadioButton b:radios) 
                if(b!=buttonView)
               b.setChecked(false);
               }
            });
         }
        }
      static void setradiotest(RadioButton[] radios,Object[] ap) {
         for(RadioButton but:radios) {
         but.setOnCheckedChangeListener( (buttonView,  isChecked) -> {
            if(isChecked) {
               for(var o:ap) {
                 var a = (Consumer<View>) o;
                  a.accept(buttonView);
               }
            for(RadioButton b:radios) 
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
                var jsonstr= getbackJson(pos);
                QRmake.show(act,jsonstr);
                return true;
                }
         }

void makeAutoQR(MainActivity act,View parent) {
      EnableControls(parent,false);
      var cancel=getbutton(act,R.string.cancel);
      var title=getlabel(act, R.string.autoqr);
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
      var layout=new Layout(act, new View[]{ title},new View[]{send},new View[]{homenetS,internetS},new View[]{receive},new View[]{homenetR,internetR},new View[] {help,cancel});
      layout.setPadding((int)(GlucoseCurve.metrics.density*4.0),(int)(GlucoseCurve.metrics.density*4.0),(int)(GlucoseCurve.metrics.density*4.0),(int)(GlucoseCurve.metrics.density*4));
      layout.setBackgroundColor(backgroundcolor);
      layout.measure(WRAP_CONTENT, WRAP_CONTENT);
      layout.setX((GlucoseCurve.getwidth()-layout.getMeasuredWidth()+MainActivity.systembarLeft-MainActivity.systembarRight)*.5f);
      layout.setY( (GlucoseCurve.getheight()-layout.getMeasuredHeight() +MainActivity.systembarTop-MainActivity.systembarBottom)*.5f);
      act.addContentView(layout, new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));
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
            MainActivity.poponback();
            makeQR(act,Natives.makeHomeReceiver());
            closerun.run();
            });
      internetR.setOnClickListener(v-> {
            MainActivity.poponback();
            makeQR(act,Natives.makeICEReceiver());
            closerun.run();
            });
    };
RadioButton one;
EditText ICElabel;
CheckBox ICE;
   void makehostview(MainActivity act) {
      ICE=getcheckbox(act,R.string.ICE, true);
      for(int i=0;i<editIPs.length;i++) {
         editIPs[i]=new EditText(act);
         editIPs[i].setMinEms(6);
         editIPs[i].setInputType(InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD);
         editIPs[i].setImeOptions(editoptions);
         setColorFilter(editIPs[i].getBackground().mutate(),agetColor(act,android.R.color.holo_blue_light));
         }
     RadioButton zero=getradiobutton(act, R.string.zero);
     one=getradiobutton(act, R.string.one);
     var sides=new RadioButton[]{zero,one};
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
      detect = new CheckBox(act);
      detect.setText(R.string.detect);
      detect.setOnCheckedChangeListener( (buttonView,  isChecked)-> {
            final int vis=isChecked?hide:VISIBLE;
            final int lastip=editIPs.length-(haslabel.isChecked()?1:0)-1;
            editIPs[lastip].setVisibility(vis);
            });
      detect.setVisibility(hide);

      testip= new CheckBox(act); testip.setText(R.string.testip);

      haslabel= new CheckBox(act); haslabel.setText(R.string.testlabel);
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
            if(checkhostname.isChecked())
               return;
            final int vis2=isChecked?hide:VISIBLE;
            final int lastip=editIPs.length-(detect.isChecked()?1:0)-1;
            editIPs[lastip].setVisibility(vis2);
            });

         
      passiveonly=new RadioButton(act);
      passiveonly.setText(R.string.passiveonly);
      TextView Portlabel=getlabel(act,R.string.port);
        activeonly = new RadioButton(act);
        activeonly.setText(R.string.activeonly);
        both = new RadioButton(act);
        both.setText(R.string.both);
        RadioButton[] actives={passiveonly,activeonly,both};
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
      receive = new CheckBox(act);
      receive.setText(R.string.receivefrom);

      TextView Sendlabel=getlabel(act,R.string.sendto);

         Amounts = new CheckBox(act); Amounts.setText(R.string.amountsname);
         Scans = new CheckBox(act); Scans.setText(R.string.scansname);
         Stream = new CheckBox(act); Stream.setText(R.string.streamname);
      RadioButton fromnow=new RadioButton(act);
      RadioButton alldata=new RadioButton(act);
      RadioButton screenpos=new RadioButton(act);
      TextView startlabel=getlabel(act,act.getString(R.string.datapresentuntil));
         alldata.setText(R.string.start);
         fromnow.setText(R.string.now);
      sendfrom=new RadioButton[]{alldata,fromnow,screenpos};
       fromrow=new View[]{startlabel, alldata,fromnow,screenpos};

      setradio(sendfrom);
      CheckBox restore=new CheckBox(act);restore.setText("Restore");
      if(!Natives.backuphasrestore( ))
         restore.setVisibility(GONE);

      Button Help=getbutton(act,R.string.helpname);
      Help.setOnClickListener(v-> help(R.string.addconnection,act));

      Button delete=getbutton(act,act.getString(R.string.delete));
      Button Close=getbutton(act,R.string.cancel);
          Password = new CheckBox(act); Password.setText(R.string.password);
         Password.setChecked(true);
       editpass= new EditText(act);
           editpass.setImeOptions(editoptions);
           editpass.setInputType(InputType.TYPE_TEXT_VARIATION_PASSWORD);
      editpass.setTransformationMethod(new PasswordTransformationMethod());
           editpass.setMinEms(6);
          visible = new CheckBox(act);// visible.setText(R.string.visible);
          visible.setButtonDrawable(R.drawable.password_visible);
   //      visible.setMinimumWidth(0); visible.setMinWidth(0);
      visible.setOnCheckedChangeListener( (buttonView,  isChecked)-> {

               editpass.setInputType(isChecked?InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD:InputType.TYPE_TEXT_VARIATION_PASSWORD);
               if(isChecked)
                  editpass.setTransformationMethod(null);
               else
                  editpass.setTransformationMethod(new PasswordTransformationMethod());
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
         boolean ice=ICE.isChecked();
         int struse=0;
         String[] names=null;
         final boolean dodetect= detect.isChecked()&&!activeonly.isChecked();
         if(ice) {
            }
         else {
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

         int pos=Natives.changebackuphost(hostindex,names,struse,dodetect,portedit.getText().toString(), Amounts.isChecked(),Stream.isChecked(),Scans.isChecked(),restore.isChecked(),receiver,activeonly.isChecked(),passiveonly.isChecked(),Password.isChecked()?editpass.getText().toString():null,starttime,haslabel.isChecked()?label.getText().toString():null,testip.isChecked(),checkhostname.isChecked(),ICElabel.getText().toString(),one.isChecked());

         if(pos<0) {
            String mess=changehostError(act, pos);
            Applic.argToaster(act,mess,Toast.LENGTH_SHORT);
            return pos;
            }    

         if(!receiver&& !(Amounts.isChecked()&& Stream.isChecked()&& Scans.isChecked())) {
            Applic.argToaster(act,R.string.notalldata ,Toast.LENGTH_LONG);
            }        
         configchanged=true;
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
      CheckBox[] boxes={Amounts,Scans,Stream,restore};
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
      for(CheckBox vi:boxes) {
         vi.setOnCheckedChangeListener(needport);
         }
     hostview=new ScrollView(act);
      visible.setPadding(0,0,(int)(GlucoseCurve.metrics.density*5.0),0);
    var iceviews=new View[]{ICE,ICElabellabel,ICElabel,zero,one};
      Sendlabel.setPadding((int)(GlucoseCurve.metrics.density*10.0),0,0,0);
   Stream.setPadding(0,0,(int)(GlucoseCurve.metrics.density*5.0),0);
    var firstrow=new View[]{Portlabel, portedit, checkhostname,IPslabel, detect};
    var directions=new View[]{passiveonly, activeonly, both};
      Layout layout;
      if(isWearable) {
         getMargins(save).topMargin=(int)(GlucoseCurve.metrics.density*5.0);
         layout=new Layout(act, (l, w, h) -> {
            hideSystemUI(act);
            final int[] ret={w,h};
            return ret;

         }, new View[]{ICE},new View[]{ Portlabel},new View[] {portedit},new View[]{checkhostname},new View[]{new Space(act),IPslabel,detect,new Space(act)},new View[]{ICElabellabel},new View[]{ICElabel},sides, new View[]{editIPs[0]},new View[]{editIPs[1]},editIPs.length>=3?new View[]{editIPs[2]}:null,editIPs.length>=4?new View[]{editIPs[3]}:null ,new View[] {testip},new View[] {haslabel},new View[]{label},
               new View[]{passiveonly},new View[]{activeonly},new View[]{both},new View[] {receive},new View[] {Sendlabel,Stream},new View[]{Scans,Amounts},new View[]{startlabel},new View[]{alldata,fromnow},new View[]{screenpos} ,new View[]{Password },new View[]{editpass,visible},new View[]{delete,Close},new View[] {reset},new View[]{save});

      layout.setPadding((int)(GlucoseCurve.metrics.density*4.0),0,(int)(GlucoseCurve.metrics.density*10.0),(int)(GlucoseCurve.metrics.density*4));
         }
      else {
         getMargins(delete).leftMargin=getMargins(save).rightMargin=(int)(GlucoseCurve.metrics.density*20.0f);
        var withqr=BuildConfig.minSDK>=20?new View[]{Password, editpass, visible}:new View[]{Password, editpass, visible};
         layout = new Layout(act, (l, w, h) -> {
            hideSystemUI(act);
            final int[] ret = {w, h};
            return ret;

         }, firstrow,new View[]{ICE,ICElabellabel,ICElabel,zero,one},editIPs, new View[]{testip, haslabel, label},
               directions, new View[]{receive, Sendlabel, Amounts, Scans, Stream, restore}, fromrow, withqr, new View[]{delete, Close, reset, Help, save});

         var sidepad=(int)(GlucoseCurve.metrics.density*8.0);
         layout.setPadding(MainActivity.systembarLeft+sidepad,MainActivity.systembarTop/2,sidepad+MainActivity.systembarRight,MainActivity.systembarBottom);

         }
      Close.setOnClickListener(v-> act.doonback());
      hostview.addView(layout);
      hostview.setFillViewport(true);
      hostview.setSmoothScrollingEnabled(true);
       hostview.setVerticalScrollBarEnabled(Applic.scrollbar);
       hostview.setScrollbarFadingEnabled(true);
       act.addContentView(hostview, new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.MATCH_PARENT));
       hostview.setBackgroundColor(backgroundcolor);
    Consumer<Boolean> setICE=(isChecked) -> {
            final int vis=isChecked?VISIBLE:hide;
            for(int i=1;i<iceviews.length;++i) {
                iceviews[i].setVisibility(vis);
                }
            final int notvis=!isChecked?VISIBLE:hide;
            for(var el:firstrow) {
                el.setVisibility(notvis);
                }
            for(var el:editIPs) {
                el.setVisibility(notvis);
                }
            for(var el:directions) {
                el.setVisibility(notvis);
                }
            testip.setVisibility(notvis);
            };

      ICE.setOnCheckedChangeListener( (buttonView,  isChecked)-> {
            setICE.accept(isChecked);
            });
     ICE.setChecked(false);
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

      boolean isnew=names==null&&!hasICE;
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
          if(!isnew&&!hasICE) {
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
               }
                );
      sethtml(info, mirrorStatus(pos));

      ViewGroup layall;

      if(isWearable) {
          if(!useclose) close.setVisibility(GONE);
         var space1=new Space(act);
         var space2=getlabel(act,"      ");
          Layout layout=new Layout(act,new View[]{space1,deactive,modify,space2}, new View[]{info},new View[]{close});
   //      layout.round=true;
         layout.setBackgroundColor(Applic.backgroundcolor);
         var leftpad=(int)(GlucoseCurve.getwidth()*.1);
         layout.setPadding(leftpad,leftpad,(int)(GlucoseCurve.getwidth()*0.08), leftpad*2);
         var scroll= new ScrollView(act);
         scroll.setFillViewport(true);
         scroll.setVerticalScrollBarEnabled(true);
         scroll.setScrollbarFadingEnabled(true);
         scroll.setSmoothScrollingEnabled(true);
         scroll.addView(layout);
         layall=scroll;
         }
      else {
           var modmar=Layout.getMargins(modify);
           modmar.leftMargin=(int)(GlucoseCurve.metrics.density*10);
           var closemar=Layout.getMargins(close);
           closemar.rightMargin=(int)(GlucoseCurve.metrics.density*10);;
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
                var x=GlucoseCurve.getwidth()-MainActivity.systembarRight-w;
                if(x<MainActivity.systembarLeft)
                   x=MainActivity.systembarLeft;
                l.setX(GlucoseCurve.getwidth()-MainActivity.systembarRight-w);
                l.setY(MainActivity.systembarTop);
                final int[] lret={w,h};
                return lret;
                },firstrow , new View[]{info});
          // info.setPadding(pad,0,pad,0);
            layout.setBackgroundResource(R.drawable.dialogbackground);
   //          layout.setRotation(90);
         layall=layout;
            }

      modify.setOnClickListener(v->     changehostview(act,pos,layall));
      final var lpar=isWearable?MATCH_PARENT: WRAP_CONTENT;
      act.addContentView(layall, new ViewGroup.LayoutParams(lpar,lpar));
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
      act.lightBars(false);
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
     View p2p= (thishost[0]==null)?new Space(act):getlabel(act,"p2p: "+thishost[0]);
     View blpan= (thishost[2]==null)?new Space(act):getlabel(act,"bt-pan: "+thishost[2]);
     String port=Natives.getreceiveport();
     TextView labport=getlabel(act,R.string.port);
     EditText portview=getnumedit(act, port);

     portview.setMinEms(2);

     Button hosts=getbutton(act,R.string.addconnectionbutton);
     Button Help=getbutton(act,R.string.helpname);
      Help.setOnClickListener(v->
         help(R.string.connectionoverview,act) );

     Button Sync=getbutton(act,act.getString(R.string.sync));
      Sync.setOnClickListener(v-> Applic.wakemirrors());
     Button reinit=getbutton(act,R.string.reinit);
      reinit.setOnClickListener(v-> {
      MessageSender.reinit();
      }
      );
//     boolean[] issaved={false};
      //alarms=getbutton(act,R.string.alarms);
   //      if(!Natives.isreceiving( )) { alarms.setEnabled(false); }

     final Button battery = new Button(act);


     Button Cancel=getbutton(act,R.string.closename);
      Button Save=getbutton(act,R.string.save);
      Save.setVisibility(INVISIBLE);
      changer ch=new changer(Save);
      portview.addTextChangedListener(ch);
      RecyclerView recycle = new RecyclerView(act);
      LinearLayoutManager lin = new LinearLayoutManager(act);
      recycle.setLayoutManager(lin);

      CheckBox staticnum = new CheckBox(act);
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
         CheckBox wifi=getcheckbox(act,act.getString(R.string.wifi),getWifi());
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
         getMargins(labport).leftMargin=(int)(width*0.12);
         getMargins(Save).rightMargin=(int)(width*0.12);
         var margIP=getMargins(ip);
         margIP.leftMargin=(int)(width*0.01);
   //      if(doLog) ip.setText("2a01:59f:a075:b0d1:a4ef:afff:fec4:59f2");
         //final Layout layout=new Layout(act, new View[]{getlabel(act,act.getString(R.string.thishost))},new View[]{blpan},new View[]{p2p},new View[]{ip},new View[]{new Space(act),labport,portview,Save,new Space(act)},new View[]{recycle},new View[] {hosts},new View[]{staticnum},new View[]{Sync,reinit},new View[]{space1,wifi,alarms,space2},errorrow,new View[]{Cancel});
         final Layout layout=new Layout(act, new View[]{getlabel(act,act.getString(R.string.thishost))},new View[]{labport,portview,Save},new View[]{ip},new View[]{blpan},new View[]{p2p},new View[]{recycle},new View[] {hosts},new View[]{staticnum},new View[]{Sync,reinit},new View[]{wifi},errorrow,new View[]{Cancel});
   //        var hori=new NestedScrollView(act);
         var hori=new ScrollView(act);
         hori.setFillViewport(true);
   //        hori.setSmoothScrollingEnabled(false);
          hori.setVerticalScrollBarEnabled(Applic.scrollbar);
   //       hori.setHorizontalScrollBarEnabled(Applic.horiScrollbar);
         hori.setScrollbarFadingEnabled(true);
         hori.setSmoothScrollingEnabled(true);
         int height=GlucoseCurve.getheight();
         hori.setMinimumHeight(height);
         hori.addView(layout);
         lay=hori;
         int pad=(int)(GlucoseCurve.metrics.density*5);
         layout.setPadding((int)(GlucoseCurve.metrics.density*6),pad,(int)(GlucoseCurve.metrics.density*9),pad);
         }
      else {
        Button autoqr;
         if(BuildConfig.minSDK>=20) {
             autoqr=getbutton(act,R.string.autoqr);
            }
         else {
            autoqr=null;
            }
         getMargins(Help).leftMargin=getMargins(Cancel).rightMargin=(int)(GlucoseCurve.metrics.density*20.0f);
         var withqr=BuildConfig.minSDK>=20?new View[]{Help,autoqr,hosts,Cancel}:new View[]{Help,hosts,Cancel};
         var layout=new Layout(act, new View[]{ip,blpan,p2p,labport,portview,Save,turnserver},new View[]{recycle},new View[] {battery,Sync,reinit,staticnum},errorrow,withqr);
        if(BuildConfig.minSDK>=20) {
            autoqr.setOnClickListener(v -> {
                makeAutoQR(act, layout);
                });
          };

       var density=GlucoseCurve.metrics.density;
      layout.setPadding(MainActivity.systembarLeft+(int)(density*10),MainActivity.systembarTop/2,MainActivity.systembarRight+(int)(density*10),MainActivity.systembarBottom+(int)(density*3));
       //  layout.setPadding(MainActivity.systembarLeft,MainActivity.systembarTop/2,MainActivity.systembarRight,MainActivity.systembarBottom);

          {if(doLog) {Log.i(LOG_ID,"density="+GlucoseCurve.metrics.density+" systembarTop="+ MainActivity.systembarTop+" systembarLeft="+ MainActivity.systembarLeft);};};
      //    layout.setPadding(pad,MainActivity.systembarTop,MainActivity.systembarRight,MainActivity.systembarBottom);
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
   //   act.lightBars(false);
      act.addContentView(lay, new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.MATCH_PARENT));
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
             view.setPadding((int)(GlucoseCurve.metrics.density*10.0),0,0,af);
          //   view.setPadding(0,0,0,af);
             //view.setPadding(view.getPaddingLeft(),0,0,af);
             }
            view.setTextColor(YELLOW);
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
         boolean doreceive= (recnum&2)!=0;
         if(off)
             text.setPaintFlags(text.getPaintFlags() | Paint.STRIKE_THRU_TEXT_FLAG);
         else
             text.setPaintFlags(text.getPaintFlags() & ~Paint.STRIKE_THRU_TEXT_FLAG);
         if(label!=null) {
            sb.append(label);
            sb.append(" ");
            }
   if(!isWearable) {
           sb.append((names!=null&&names.length!=0)?names[0]:(Natives.detectIP(pos)?"Detect":"---"));
           if(!passive) {
              sb.append(" ");
              sb.append(port);
              }
           sb.append(' ');
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
           @Override
           public int getItemCount() {
         return Natives.backuphostNr( );

           }

   }
   boolean configchanged=false;

}
