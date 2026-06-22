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
/*      Fri Jan 27 15:31:32 CET 2023                                                 */


package tk.glucodata;

import static android.view.View.GONE;
import static android.view.View.INVISIBLE;
import static android.view.View.VISIBLE;
import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.Applic.usedlocale;
import static tk.glucodata.Floating.rewritefloating;
import static tk.glucodata.Log.doLog;
import static tk.glucodata.MainActivity.screenheight;
import static tk.glucodata.Specific.useclose;
import static tk.glucodata.settings.Settings.editoptions;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getcheckbox;
import static tk.glucodata.util.getlabel;
import static tk.glucodata.Layout.getMargins;

import android.graphics.Color;
import android.text.InputType;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.Space;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;

import yuku.ambilwarna.AmbilWarnaDialog;

public class FloatingConfig {
private static final String LOG_ID="FloatingConfig";
//   AmbilWarnaDialog(Context context, int color, boolean supportsAlpha, OnAmbilWarnaListener listener)
static private boolean background=true;


static public void    setcolor(int c) {
        {if(doLog) {Log.i(LOG_ID,"setcolor("+(c&0xFFFFFFFF)+")");};};
        if(background) { 
            Floating.setbackgroundcolor(c);
            }
        else  {Floating.setforegroundcolor(c);            }
        }
static public int    getcolor() {
        return background?Natives.getfloatingbackground( ):Natives.getfloatingforeground( );
        }


static public void show(MainActivity act,View view) {
  final boolean  wasfloating=Natives.getfloatglucose();
  view.setVisibility(INVISIBLE);
  int height=GlucoseCurve.getheight();
  int width=GlucoseCurve.getwidth();
  final String fontstring=act.getString(R.string.fontsizeshort)+ " ";
  var  sizelabel=getlabel(act,fontstring);
  int maxfont=Math.min(height*7/10,width*4/10);
  int pad=height/14;
  sizelabel.setPaddingRelative(pad,0,0,0);
  int currentfont=Natives.getfloatingFontsize();
   if(currentfont<5||currentfont>(int)(screenheight*.8)) {
        currentfont=(int)Notify.glucosesize; 
        }
  SeekBar fontsizeview=new SeekBar(act);
  Applic.ifRTLseekbar(fontsizeview);
  fontsizeview.setMax((int)((maxfont-5)*100.0));
  fontsizeview.setProgress((int)((currentfont-5)*100.0));
//      var fwidth=(int)(width*0.8f);
  fontsizeview.setMinimumWidth(width);
//  final int minimumvalue=500;
//  fontsizeview.setMin(minimumvalue);
              
  fontsizeview.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
      @Override
      public  void onProgressChanged (SeekBar seekBar, int progress, boolean fromUser) {
//         int newprogress=progress+minimumvalue; 
         var siz=(int)Math.round(progress/100.0)+5;
//        if(doLog) sizelabel.setText(fontstring+siz);
        Natives.setfloatingFontsize(siz);
        rewritefloating(act);
          }
        @Override
        public void onStartTrackingTouch(SeekBar seekBar) {
            {if(doLog) {Log.i(LOG_ID,"onStartTrackingTouch");};};
            }
        @Override
        public void onStopTrackingTouch(SeekBar seekBar) {
            }
        });
  /* 
    var  sizeview= new EditText(act);
  sizeview.setImeOptions(editoptions);
    sizeview.setMinEms(2);
    sizeview.setInputType(InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL);

    sizeview.setText(fontsize+"");
//    sizeview.setPaddingRelative(pad,0,0,0);
  TextView.OnEditorActionListener  actlist= new TextView.OnEditorActionListener() {
                    @Override
                    public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                    if (event != null && event.getKeyCode() == KeyEvent.KEYCODE_ENTER || actionId == EditorInfo.IME_ACTION_DONE) {
                                 {if(doLog) {Log.i(LOG_ID,"onEditorAction");};};
                 try {
                    var siz=Integer.parseInt(String.valueOf(v.getText()));
                    if(siz>maxfont) {
                        Toast.makeText(act, act.getString(R.string.fonttoolarge)+maxfont, Toast.LENGTH_SHORT).show();
                        return true;
                        }
                    else  {
                        Natives.setfloatingFontsize(siz);
                         rewritefloating(act);
                    //    Floating.invalidatefloat();
                         }
                    }
                catch(Throwable th) {
                    Log.stack(LOG_ID,"parseInt",th);
                    }
//                                return true;
                           }
                    return false;
                    }};
    sizeview.setOnEditorActionListener(actlist);
*/


    var touch=Natives.getfloatingTouchable();
    var touchable=getcheckbox(act,R.string.touchable,touch);
    var close=getbutton(act,R.string.closename);
       close.setIncludeFontPadding(false);

//    close.setPaddingRelative(0,0,0,0);
    var color=Natives.getfloatingbackground();
    boolean transp= Color.alpha(color)!=0xFF;
    var transparentview=getcheckbox(act,R.string.transparent,transp);
    var backgroundview=getbutton(act,R.string.backgroundshort);

    if (transp) backgroundview.setVisibility(INVISIBLE);

    var foreground=getbutton(act,R.string.foreground);

    final   int pad2=(int)(tk.glucodata.GlucoseCurve.metrics.density*5.0);


   foreground.setIncludeFontPadding(false);
   backgroundview.setIncludeFontPadding(false);


    var timeshow=getcheckbox(act,R.string.time,Floating.showtime);
    timeshow.setOnCheckedChangeListener( (buttonView,  isChecked) -> {
        Floating.showtime=isChecked;
        Natives.setfloattime(isChecked);
        rewritefloating(act);
        });

//    timeshow.setPaddingRelative(pad,0,0,0);
    transparentview.setPaddingRelative(0,0,pad2,0);

    boolean[] hidden={Natives.gethidefloatinJuggluco()};
    var hide=getcheckbox(act,R.string.floatjuggluco, !hidden[0]);
    hide.setPaddingRelative(0,0,(int)(tk.glucodata.GlucoseCurve.metrics.density*13f),0);
    if(hidden[0]) {
        Floating.makefloat();
        }
    hide.setOnCheckedChangeListener( (buttonView,  isChecked) -> {
        hidden[0]=!isChecked;
        Natives.sethidefloatinJuggluco(!isChecked);
        });
/*   var space1 = new Space(act);
   var space2 = new Space(act); */
   
    var sidemargin=(int)(GlucoseCurve.getwidth()*0.015f);
    getMargins(foreground).setMarginStart(sidemargin);
    getMargins(backgroundview).setMarginEnd(sidemargin);
    getMargins(timeshow).setMarginEnd((int)(GlucoseCurve.getwidth()*0.05f));
//    getMargins(hide).rightMargin=(int)(GlucoseCurve.getwidth()*0.08f);
   if(!useclose)
      close.setVisibility(GONE);
    Layout layout=new Layout(act,(l,w,h)-> { return new int[] {w,h}; },new View[]{touchable},new View[]{sizelabel},new View[]{ fontsizeview}, new View[]{foreground,backgroundview},new View[]{timeshow,hide},new View[]{transparentview},new View[]{close});
   int pad3=(int)(tk.glucodata.GlucoseCurve.metrics.density*5.0);
    layout.setPaddingRelative(pad3,pad3,pad3,pad3*2);
    transparentview.setOnCheckedChangeListener( (buttonView,  isChecked) -> {
        background=true;
        Floating.setbackgroundalpha(isChecked?0:0xff);
        if(isChecked)
            backgroundview.setVisibility(INVISIBLE);
        else
            backgroundview.setVisibility(VISIBLE);
        //rewritefloating(act);
        Floating.invalidatefloat();

    });
    layout.setBackgroundColor(Applic.backgroundcolor);
    foreground.setOnClickListener(v-> {background=false;showcolors(act);});
    backgroundview.setOnClickListener(v-> {background=true;showcolors(act);});
    touchable.setOnCheckedChangeListener( (buttonView,  isChecked) -> {
        Natives.setfloatingTouchable(isChecked);
        if(!isChecked) {
            int y= (int) Floating.yview;
            Natives.setfloatingPos(((int)Floating.xview)|(0xFFFFFFFF&(y<< 16)));
            }
        rewritefloating(act);

        });

   var scroll=new ScrollView(act);
    scroll.setFillViewport(true);
    scroll.setSmoothScrollingEnabled(false);
   scroll.setScrollbarFadingEnabled(true);
   scroll.setVerticalScrollBarEnabled(true);
   scroll.addView(layout);
    act.addMyContentView(scroll, new ViewGroup.LayoutParams(MATCH_PARENT,MATCH_PARENT));


    act.setonback(()-> {
        view.setVisibility(VISIBLE);
        removeContentView(layout); 
      Floating.setfloatglucose(act, wasfloating);
      if(hidden[0]) {
            Floating.removeFloating();
            }
        });
    close.setOnClickListener(v->{
        act.doonback();
    });
}

static public void showcolors(MainActivity act) {
    int initialColor= getcolor();


    int height=GlucoseCurve.getheight();
    var width=GlucoseCurve.getwidth();
    AmbilWarnaDialog dialog = new AmbilWarnaDialog(act, initialColor,c-> {
    {if(doLog) {Log.i(LOG_ID,String.format(usedlocale,"col=%x",c));};};
        setcolor(c);
        Floating.invalidatefloat();
    }, v-> {
          int h=v.getMeasuredHeight();
                int w=v.getMeasuredWidth();
                v.setY((int)((height-h)*.5));
                v.setX((int)((width-w)*.57));

        }
    );
    View view=dialog.getview();
    var layout=new FrameLayout(act);
    layout.addView(view, new ViewGroup.LayoutParams((int)(width*0.72), (int)(height*0.72)));
    act.addMyContentView(layout,  new ViewGroup.LayoutParams(MATCH_PARENT,MATCH_PARENT));
    layout.setBackgroundColor(Applic.backgroundcolor);
    layout.setOnTouchListener(new BackGesture(act));
    var ok=useclose?getbutton(act,"Ok"):null;
    if(ok!=null) {
        ok.setOnClickListener(v->MainActivity.doonback());
        //act.addMyContentView(ok, new ViewGroup.LayoutParams(WRAP_CONTENT,WRAP_CONTENT));

        var okparams= new FrameLayout.LayoutParams( WRAP_CONTENT, WRAP_CONTENT, Gravity.TOP|Gravity.CENTER_HORIZONTAL);
        act.addMyContentView(ok, okparams);


       // ok.setX((int)(width*.45));
        int padx=(int)(width*.04f);
        int pady=(int)(width*.015f);
        ok.setPaddingRelative(padx,pady,padx,pady);
        }
    act.setonback(()-> { 
        if(ok!=null)
            removeContentView(ok); 
        removeContentView(layout); });

    };
}

