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
/*      Wed May 06 21:13:17 CEST 2026                                                */
package tk.glucodata;


import static tk.glucodata.Applic.Toaster;
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.Applic.scheduler;
import static tk.glucodata.Applic.useZXing;
import static tk.glucodata.Backup.getedit;
import static tk.glucodata.GS3ID.GS3IDstatus;
import static tk.glucodata.Log.doLog;
import static tk.glucodata.MainActivity.REQUEST_BARCODE;
import static tk.glucodata.MainActivity.REQUEST_BARCODE_SIB2;
import static tk.glucodata.PhotoScan.deviceAdded;
import static tk.glucodata.ZXing.scanZXingAlg;
import static tk.glucodata.settings.Settings.editoptions;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.util.getbutton;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static tk.glucodata.util.getlabel;
import static tk.glucodata.util.getcheckbox;
import static tk.glucodata.util.getradiobuttonId;
import static tk.glucodata.util.sethtml;


import android.text.InputType;
import android.text.method.PasswordTransformationMethod;
import android.view.Gravity;
import android.view.View;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.RadioGroup;
import android.widget.TextView;
import android.widget.Toast;


class GetGS3ID {
private static final String LOG_ID="GetGS3ID";
private static void getUserid(String name,String oldid,MainActivity context) {
   var editid = new EditText(context);
    if(!"0".equals(oldid))
       editid.setText(oldid);
   editid.setImeOptions(editoptions);
   editid.setInputType(InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD);
    editid.setMinEms(10);
   var save=getbutton(context,R.string.save);
   var close=getbutton(context,R.string.closename);
   String[] prevID={editid.getText().toString()};
  final Layout layout=new Layout(context, (lay, w, h) -> {
      return new int[] {w,h};}, new View[]{editid,},new View[]{close,save});

      layout.setBackgroundResource(R.drawable.dialogbackground);
      int pad= (int)tk.glucodata.GlucoseCurve.metrics.density*7;
    layout.setPadding(pad,pad,pad,pad);
    var  params =    new FrameLayout.LayoutParams( WRAP_CONTENT, WRAP_CONTENT, Gravity.CENTER_HORIZONTAL);
    params.topMargin= MainActivity.systembarTop;
    context.addMyContentView(layout, params);

    Runnable closer= ()->  
      {removeContentView(layout);
      gs3Number(name, context);
      };
   Runnable[] onback={null}; //Idioot, maar gaat niet anders
  onback[0]= ()-> {
             String idstr = editid.getText().toString();
             if(idstr.equals(prevID[0])) {
                  closer.run();
                  }
             else  {
             Confirm.ask2(context,"getUserID "+context.getString(R.string.withoutsaving),"",
                    closer ,
                    ()-> {
                        MainActivity.setonback( onback[0]);
                        });
                  }
          };


   MainActivity.setonback( onback[0]);

   save.setOnClickListener(v->  {
         String idstr = editid.getText().toString();
         try {
            if(!idstr.isEmpty()) {
               long id = Long.parseLong(idstr);
               Natives.saveGS3id(id);
               Applic.argToaster(context, context.getString(R.string.saved)+ " "+ id, Toast.LENGTH_SHORT);
               prevID[0]=idstr;
            } else {
               Applic.argToaster(context, context.getString(R.string.noaccountidspecified), Toast.LENGTH_SHORT);
               return;
            }
         } catch (Throwable th) {
            Applic.argToaster(context, context.getString(R.string.wrongformat) + idstr, Toast.LENGTH_SHORT);
            Log.stack(LOG_ID, "parse account id", th);
            return;
         }
      MainActivity.poponback();
      closer.run();
      });
   close.setOnClickListener(v->  {
         MainActivity.doonback();
         });
   }

static private boolean validate(MainActivity act,String emailstr,String passstr) {
            if(emailstr.length()<3) {
               Applic.argToaster(act, act.getString( R.string.emailaddresstooshort)+emailstr, Toast.LENGTH_SHORT);
               return false;
               }
            if(emailstr.length()>255) {
               Applic.argToaster(act,  act.getString( R.string.emailaddresstoolong)+emailstr, Toast.LENGTH_SHORT);
               return false;
               }
            if(passstr.length()<3) {
               Applic.argToaster(act, act.getString(R.string.password8)+passstr, Toast.LENGTH_SHORT);
               return false;
               }
            if(passstr.length()>36) {
               Applic.argToaster(act,  act.getString(R.string.password36)+passstr, Toast.LENGTH_SHORT);
               return false;
               }
        return true;
        }


public static void  gs3fromserver(String name,MainActivity act) {
   var emaillabel=getlabel(act,R.string.email);
   var email=getedit(act, "");
   email.setMinEms(16);

   var passlabel=getlabel(act,act.getString(R.string.password)+":");
   var      editpass= new EditText(act);
   editpass.setImeOptions(editoptions);
   editpass.setInputType(InputType.TYPE_TEXT_VARIATION_PASSWORD);
   editpass.setTransformationMethod(new PasswordTransformationMethod());
   editpass.setMinEms(12);

        var visible = new CheckBox(act);
        visible.setButtonDrawable(R.drawable.password_visible);
        visible.setOnCheckedChangeListener( (buttonView,  isChecked)-> {
                        var sel=editpass.getSelectionStart();
                        editpass.setInputType(isChecked?InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD:InputType.TYPE_TEXT_VARIATION_PASSWORD);
                        if(isChecked)
                                        editpass.setTransformationMethod(null);
                        else
                                        editpass.setTransformationMethod(new PasswordTransformationMethod());
                        editpass.setSelection(sel);
                        });



   var accountid=getlabel(act, "");
   var getaccountid=getbutton(act,R.string.getaccountid);
   var close=getbutton(act,R.string.closename);
   final var helpbutton=getbutton(act,R.string.helpname);

   helpbutton.setOnClickListener(v-> help.help(R.string.SibionicsServer,act));
   var statusview=getlabel(act,GS3IDstatus);
   String[] waspass={""},wasmail={""};
   final Layout layout=new Layout(act, new View[]{emaillabel,email},new View[]{passlabel,editpass,visible},new View[]{getaccountid,accountid},new View[]{statusview},new View[]{helpbutton,close});
   Runnable closer=() -> {
            removeContentView(layout);
            gs3Number(name, act);
     };
   Runnable[] onback={null};
   onback[0]=
           ()->  {
       var  success=wasmail[0].equals(email.getText().toString())&&waspass[0].equals(editpass.getText().toString());
        if(!success) {
            Confirm.ask2(act,act.getString(R.string.leave),act.getString(R.string.emailnotused),
                closer ,
                ()-> {
                    MainActivity.setonback( onback[0]);
                    });
            }
        else {
            closer.run();
        }};
   MainActivity.setonback( onback[0]);

   close.setOnClickListener(v->  {
         MainActivity.doonback();
         });


    getaccountid.setOnClickListener(v -> {
       String emailstr = email.getText().toString();
       String passstr  = editpass.getText().toString();
       if (!validate(act, emailstr, passstr)) return;        

       var md5pass = Natives.md5sum(passstr);
       statusview.setText(R.string.connecttoserver);
       scheduler.execute(() -> {                                       // background thread
          boolean ok = GS3ID.postgetauth(emailstr, md5pass);
          long id    = Natives.getGS3id();
          String st  = GS3IDstatus;

          Applic.getHandler().post(() -> {                                       // back on the UI thread
             statusview.setText(st);
             if(ok) {
                accountid.setText(Long.toString(id));
                waspass[0]=passstr;
                wasmail[0]=emailstr;
                }
             else {
                 Toast.makeText(act,R.string.retrievefailed, Toast.LENGTH_SHORT).show();
                 }


          });
       });
    });

    layout.setBackgroundResource(R.drawable.dialogbackground);
    int pad= (int)tk.glucodata.GlucoseCurve.metrics.density*17;
    layout.setPadding(pad,0,pad,(int)(tk.glucodata.GlucoseCurve.metrics.density*7));
    int width = GlucoseCurve.getwidth();
    var  params =    new FrameLayout.LayoutParams( (int)(width*0.7f), WRAP_CONTENT, Gravity.CENTER_HORIZONTAL);
    params.topMargin=MainActivity.systembarTop;
   act.addMyContentView(layout, params);
   }

static void gs3Number(String name, MainActivity act) {
    long id=Natives.getGS3id();
    var help=new TextView(act);
    sethtml(help, R.string.gs3number);

    String idstring=Long.toString(id);
    var idtext=getlabel(act,idstring);
   final int rand=(int)tk.glucodata.GlucoseCurve.metrics.density*10;
   final int siderand=(int)tk.glucodata.GlucoseCurve.metrics.density*20;
    idtext.setPadding(0,rand,0,rand);
    var manual=getbutton(act,R.string.manual);
    var retrieve=getbutton(act,R.string.retrieve);
    var ok=getbutton(act, R.string.ok);
    var layout=new Layout(act,new View[]{help},new View[]{idtext},new View[]{manual,retrieve,ok});
    layout.setBackgroundResource(R.drawable.dialogbackground);
   layout.setPadding(siderand,rand,siderand,rand);

    int width = GlucoseCurve.getwidth();
   var  params =    new FrameLayout.LayoutParams((int)(width*.55f), WRAP_CONTENT, Gravity.CENTER_HORIZONTAL| Gravity.CENTER);
   params.topMargin= MainActivity.systembarTop;
   act.addMyContentView(layout, params);
   Runnable closer=() ->
      removeContentView(layout);
  Runnable[] back={null};
  back[0]=
() -> {
       var idstr=idtext.getText().toString();
       if(idstr.isEmpty() || idstr.equals("0")) {
              Confirm.ask2(act,act.getString(R.string.leave),
                        act.getString(R.string.id0message),
                          ()-> {
                              deviceAdded(act);
                              closer.run();} ,
                        ()-> {
                            MainActivity.setonback( back[0]);
                            });
              }
       else {
          deviceAdded(act);
          closer.run();
          }
      };

   MainActivity.setonback(back[0]);

   ok.setOnClickListener(v-> {
        MainActivity.doonback();
        });
   manual.setOnClickListener(v-> {
        MainActivity.poponback();
        closer.run();
        getUserid(name,idstring,act);
        });
   retrieve.setOnClickListener(v-> {
        MainActivity.poponback();
        closer.run();
        gs3fromserver(name,act);
        });

    }
    }
