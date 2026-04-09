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
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.Toast;

import static tk.glucodata.Layout.getMargins;
import static android.view.View.INVISIBLE;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static tk.glucodata.Log.doLog;
import static tk.glucodata.Log.stack;
import static tk.glucodata.NumberView.geteditview;
import static tk.glucodata.NumberView.geteditwearos;
import static tk.glucodata.NumberView.smallScreen;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getcheckbox;
import static tk.glucodata.util.getlabel;

class Stats {
private static final String LOG_ID="Stats";
static private void askdays(MainActivity act,boolean history) {
   var label=getlabel(act,act.getString(R.string.days));

   int pad= (int)(tk.glucodata.GlucoseCurve.metrics.density*8);
   label.setPadding(pad,pad,pad,pad);
   Button Ok = getbutton(act, R.string.ok);
   Button Cancel = getbutton(act, R.string.cancel);
   EditText days= smallScreen?geteditwearos(act):geteditview(act,new editfocus()) ;
   days.setMinEms(4);
   days.setText(""+Natives.getAnalysedays());
   Layout layout = new Layout(act, (l, w, h) -> {
      int wid = GlucoseCurve.getwidth();
      if(!smallScreen) {
         int hei = GlucoseCurve.getheight();
         if(hei>h&&wid>w) {
                int half= wid / 2;
                int af=(half-w)/4;
             l.setX(half - w-af);
             l.setY((hei - h) / 2);
             }
            else {
             l.setX(0);
             l.setY(0);
               }
            }
      else {
             l.setX((wid-w)/2);
             l.setY(0);
         }
      return new int[]{w, h};
      },new View[]{label,days},new View[]{Cancel,Ok});
        layout.setPadding(pad,pad,pad,pad);
   act.addContentView(layout,new ViewGroup.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT));
     layout.post(layout::requestLayout);
        layout.setBackgroundResource(R.drawable.dialogbackground);
   days.requestFocus();
   if(!smallScreen) {
      act.curve.numberview.showkeyboard(act);
      }
   else  {
      help.showkeyboard(act,days);
      }
final Runnable closeonback=()-> {
      removeContentView(layout);
      if(smallScreen) {
         help.hidekeyboard(act);
         }
      else
         act.curve.numberview.hidekeyboard() ;
      mkstats(act);
      };
   Cancel.setOnClickListener(v -> {
      act.poponback();   
      closeonback.run();
      });
   Ok.setOnClickListener(v -> {
      int get=0;
      String str=days.getText().toString();
      try {
         get=Integer.parseInt(str);  
         }
      catch(Throwable e) {
         stack(LOG_ID, e);
         };
      if(get<=0) {
              Applic.argToaster(act, "'"+str+act.getString(R.string.invaliddays), Toast.LENGTH_SHORT);
         return;
         }
      act.poponback();   
      act.curve.statspresent=false;
      act.curve.summarybutton=null;
      Natives.analysedays(get,history);
      removeContentView(layout);
      if(smallScreen) {
         help.hidekeyboard(act);
         }
      else
         act.curve.numberview.hidekeyboard() ;
      act.requestRender();
      mkstats(act);
      });
   act.setonback(closeonback);
   }

private static void webPercentiles(Context context, int days,boolean history) {
    final long endtime=Natives.percentileEndtime(days);
	final String key=Natives.getApiSecret();
    final String addkey=(key!=null&&!key.isEmpty())?key+"/":"";
    final String type=(Natives.getDoCalibrate()?"&calibrated":"&")+(history?"history":"stream");
    final String url="http://127.0.0.1:17580/"+addkey+"x/report?amounts&days="+days+"&endtime="+endtime+type;
    var intent = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
    context.startActivity(intent);
    }

static void mkstats(MainActivity act) {
     boolean showhistory=Natives.getAnalysehistory();
      MainActivity.clearonback();

        var stats=getbutton(act,R.string.save);


      Button Help = getbutton(act, R.string.helpname);
      Button Close = getbutton(act, R.string.closename);
      Button Days = getbutton(act, R.string.days);
      var history = getcheckbox(act, R.string.historyname,showhistory);
      getMargins(history).rightMargin=(int)(GlucoseCurve.metrics.density*5.0);
      Button Curve = getbutton(act, R.string.summarygraph);
      Layout layout = new Layout(act, (l, w, h) -> {
      /*
         int height = GlucoseCurve.getheight();
         int width = GlucoseCurve.getwidth();
         if(width>w) l.setX(width - w-MainActivity.systembarRight);

         if(height>h) l.setY((height - h -MainActivity. systembarBottom));
         */
         return new int[]{w, h};
      }, new View[]{history,Days, Help},new View[]{Close,stats, Curve});

      layout.setBackgroundColor(Applic.backgroundcolor);
      if(!act.curve.statspresent)
         Curve.setVisibility(INVISIBLE);
      act.curve.summarybutton=Curve;

    var  params =    new FrameLayout.LayoutParams( WRAP_CONTENT, WRAP_CONTENT, Gravity.BOTTOM| Gravity.RIGHT);
    params.bottomMargin=MainActivity.systembarBottom;
    params.rightMargin=MainActivity.systembarRight;
      act.addContentView(layout, params);
    //  act.addContentView(layout, new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));
    final Runnable closeonback=()-> {
             act.curve.statspresent=false;
             act.curve.summarybutton=null;
             removeContentView(layout);
             Natives.endstats();
             {if(doLog) {Log.i(LOG_ID,"closeonback");};};

             if(Menus.on)  {
                Menus.show(act);
                }
             else
                act.requestRender();
             };
    Close.setOnClickListener(v -> {
       act.poponback();
       closeonback.run();
       });
    act.setonback(closeonback);
      Help.setOnClickListener(v ->  {
                    act.lightBars(false);
                    help.help(R.string.stathelp, act,l->act.lightBars(!Natives.getInvertColors( ))); 
                    });
      Days.setOnClickListener(v -> {
         act.poponback();
         askdays(act,history.isChecked());
         removeContentView(layout);
      });
     boolean[] dontswitch={false};
      history.setOnCheckedChangeListener( (buttonView,  isChecked)->  {
             // act.poponback();
             // removeContentView(layout);
             if(dontswitch[0])
                return;
              act.curve.summarybutton=Curve;
              Curve.setVisibility(INVISIBLE);
              act.curve.statspresent=false;
//              act.curve.summarybutton=null;
              Natives.analysedays(-1,isChecked);
              final boolean usehistory=Natives.getAnalysehistory();
              if(usehistory!=isChecked) {
                    Applic.argToaster(act,usehistory?R.string.nostreamvalues:R.string.nohistoryvalues, Toast.LENGTH_SHORT);
                    dontswitch[0]=true;
                    history.setChecked(usehistory);
                    dontswitch[0]=false;
                    }
              act.requestRender();
             // mkstats(act);
               }
             );
      Curve.setOnClickListener(v -> {
         boolean hist= history.isChecked();
         Log.i(LOG_ID,"history.isChecked()="+hist);

         act.poponback();
         act.setonback(()-> {
            Natives.summarygraph(false);
            Stats.mkstats(act);
            act.requestRender();
            });
         Natives.summarygraph(true);
         removeContentView(layout);
         act.requestRender();
      });

        stats.setOnClickListener(v->  {
            if(Natives.getusexdripwebserver() ) {
                webPercentiles(act,Natives.getAnalysedays(),history.isChecked());
                }
            else {
                    Confirm.message(act,
                        act.getString(R.string.titlewebserverneed),
                        act.getString(R.string.messagewebserverneed)
                        ,()->{}); 
                }
            });
   }
}
