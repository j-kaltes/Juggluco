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
import static tk.glucodata.Layout.getMargins;
import static tk.glucodata.Log.doLog;
import static tk.glucodata.settings.Settings.editoptions;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getcheckbox;
import static tk.glucodata.util.getlabel;
import static tk.glucodata.MainActivity.screenheight;

import android.graphics.Color;
import android.os.Build;
import android.text.InputType;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.widget.Button;

import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.RadioButton;
import android.widget.SeekBar;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;

import yuku.ambilwarna.AmbilWarnaDialog;

public class FloatingConfig {
private static final String LOG_ID="FloatingConfig";
//   AmbilWarnaDialog(Context context, int color, boolean supportsAlpha, OnAmbilWarnaListener listener)
static private boolean background=true;
static public void    setcolor(int c) {
        if(doLog) {Log.i(LOG_ID,"setcolor("+(c&0xFFFFFFFF)+")");};
        if(background) { 
                Floating.setbackgroundcolor(c);
                }
        else  {
              Floating.setforegroundcolor(c);
              }
        }
static public int    getcolor() {
        return background?Natives.getfloatingbackground( ):Natives.getfloatingforeground( );
        }

private static class FloatingLayout extends LinearLayout {
    private final MainActivity context;
    private final View colorPicker;
    private final View controls;
    private final SeekBar fontSize;
    private int currentOrientation=-1;
    private int currentFontWidth=-1;
    private int currentFontMax=-1;

    FloatingLayout(MainActivity context,View colorPicker,View controls,SeekBar fontSize,int width,int height) {
        super(context);
        this.context=context;
        this.colorPicker=colorPicker;
        this.controls=controls;
        this.fontSize=fontSize;
        addView(colorPicker);
        addView(controls);
        updateLayout(width,height);
        }

    private void updateFontSize(int width,int height) {
        int minimumWidth=width/2;
        if(currentFontWidth!=minimumWidth) {
            currentFontWidth=minimumWidth;
            fontSize.setMinimumWidth(minimumWidth);
            }
        int maxFont=Math.max(5,height*7/10);
        int maxProgress=(maxFont-5)*100;
        if(currentFontMax!=maxProgress) {
            currentFontMax=maxProgress;
            fontSize.setMax(maxProgress);
            }
        int selectedFont=Natives.getfloatingFontsize();
        if(selectedFont>maxFont) {
            Natives.setfloatingFontsize(maxFont);
            fontSize.setProgress(maxProgress);
            rewritefloating(context);
            }
        }

    private void updateLayout(int width,int height) {
        updateFontSize(width,height);
        int orientation=width>height?HORIZONTAL:VERTICAL;
        if(currentOrientation==orientation)
            return;
        currentOrientation=orientation;
        setOrientation(orientation);
        if(orientation==VERTICAL) {
            colorPicker.setLayoutParams(new LinearLayout.LayoutParams(MATCH_PARENT,WRAP_CONTENT));
            controls.setLayoutParams(new LinearLayout.LayoutParams(MATCH_PARENT,WRAP_CONTENT));
            }
        else {
            colorPicker.setLayoutParams(new LinearLayout.LayoutParams(0,MATCH_PARENT,1.0f));
            controls.setLayoutParams(new LinearLayout.LayoutParams(WRAP_CONTENT,MATCH_PARENT));
            }
        }

    @Override
    protected void onMeasure(int widthMeasureSpec,int heightMeasureSpec) {
        updateLayout(View.MeasureSpec.getSize(widthMeasureSpec),View.MeasureSpec.getSize(heightMeasureSpec));
        super.onMeasure(widthMeasureSpec,heightMeasureSpec);
        }
    }



static public void show(MainActivity act,View parent) {
    parent.setVisibility(INVISIBLE);
    int initialColor= getcolor();


    int height=GlucoseCurve.getheight();
    int width=GlucoseCurve.getwidth();
    AmbilWarnaDialog dialog = new AmbilWarnaDialog(act, initialColor,c-> {
    {if(doLog) {Log.i(LOG_ID,String.format(usedlocale,"col=%x",c));};};
        setcolor(c);
        //rewritefloating(act);
        Floating.invalidatefloat();
    }, v-> {
        }
    );
    View view=dialog.getview();
   final String fontstring=act.getString(R.string.fontsize)+ " ";
    var  sizelabel=getlabel(act,fontstring);


   final int maxfont=Math.max(5,height*7/10);

    int currentfont=Natives.getfloatingFontsize();
     if(currentfont<5||currentfont>(int)(screenheight*.8)) {
                currentfont=(int)Notify.glucosesize; 
                }

   SeekBar fontsizeview=new SeekBar(act);
     Applic.ifRTLseekbar(fontsizeview);
      fontsizeview.setMax((maxfont-5)*100);
      fontsizeview.setProgress((int)((currentfont-5)*100.0));
//      var fwidth=(int)(width*0.8f);
      fontsizeview.setMinimumWidth(width/2);
//      final int minimumvalue=500;
/*    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
        fontsizeview.setMin(minimumvalue);
    }*/
    fontsizeview.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
        @Override
        public  void onProgressChanged (SeekBar seekBar, int progress, boolean fromUser) {
            if(!fromUser)
                return;
//         int newprogress=progress+minimumvalue; 
            var siz=(int)Math.round(progress/100.0)+5;
//         if(doLog) sizelabel.setText(fontstring+siz);
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
                sizeview.setMinEms(4);
                sizeview.setInputType(InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL);

    int fontsize=Natives.getfloatingFontsize();
    sizeview.setText(fontsize+"");
        TextView.OnEditorActionListener  actlist= new TextView.OnEditorActionListener() {
                    @Override
                    public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                    if (event != null && event.getKeyCode() == KeyEvent.KEYCODE_ENTER || actionId == EditorInfo.IME_ACTION_DONE) {
                                 {if(doLog) {Log.i(LOG_ID,"onEditorAction");};};
                 try {
                    var siz=Integer.parseInt(String.valueOf(v.getText()));
                    int maxfont=height*7/10;
                    if(siz>maxfont) {
                        Applic.argToaster(act, act.getString(R.string.fonttoolarge)+maxfont, Toast.LENGTH_SHORT);
                        return true;
                        }
                    else  {
                        Natives.setfloatingFontsize(siz);
                         rewritefloating(act);
                        //Floating.invalidatefloat();
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



    var color=Natives.getfloatingbackground();
    boolean transp= Color.alpha(color)!=0xFF;
    var transparant=getcheckbox(act,R.string.transparent,transp);
    var touch=Natives.getfloatingTouchable();
    var touchable=getcheckbox(act,R.string.touchable,touch);
    if(!background)
            transparant.setVisibility(INVISIBLE);
    else {
        if (transp)
            view.setVisibility(INVISIBLE);
        }
    var close=getbutton(act,R.string.closename);
//    CompoundButton foregroundswitch;

    CheckDirectionBox floatglucose=new CheckDirectionBox(act);
    floatglucose.setText(R.string.active);
    floatglucose.setChecked(Natives.getfloatglucose());
    floatglucose.setOnCheckedChangeListener( (buttonView,  isChecked) -> Floating.setfloatglucose(act,isChecked) ) ;
    var Help=getbutton(act,R.string.helpname);
    Help.setOnClickListener(v-> help.help(R.string.floatingconfig,act));


    var    foregroundbutton = new CheckDirectionRadio(act);
      var   backgroundbutton = new CheckDirectionRadio(act);
        foregroundbutton.setText(R.string.foreground);
        backgroundbutton.setText(R.string.background);
    foregroundbutton.setChecked(!background);
    backgroundbutton.setChecked(background);
    backgroundbutton.setTextColor(util.getColorFromTheme(act, android.R.attr.textColorPrimary));
    foregroundbutton.setTextColor(util.getColorFromTheme(act, android.R.attr.textColorPrimary));



    var timeshow=getcheckbox(act,R.string.time,Floating.showtime);
    timeshow.setOnCheckedChangeListener( (buttonView,  isChecked) -> {
        Floating.showtime=isChecked;
        Natives.setfloattime(isChecked);
        rewritefloating(act);
        });
    boolean[] hidden={Natives.gethidefloatinJuggluco()};
    var hide=getcheckbox(act,R.string.floatjuggluco, !hidden[0]);
    if(hidden[0]) {
        Floating.makefloat();
        }
    hide.setOnCheckedChangeListener( (buttonView,  isChecked) -> {
        hidden[0]=!isChecked;
        Natives.sethidefloatinJuggluco(!isChecked);
        });



    var leftlayout=new Layout(act,new View[]{sizelabel},new View[]{fontsizeview},new View[]{foregroundbutton,touchable}, new View[]{backgroundbutton,transparant},new View[]{hide,timeshow,floatglucose},new View[]{Help,close});
   final var density= tk.glucodata.GlucoseCurve.metrics.density;
   view.setPadding(0,MainActivity.systembarTop+ (int)(density*10) ,0,0);
   getMargins(close).setMarginEnd((int)(GlucoseCurve.metrics.density*20.0f));
   leftlayout.setPaddingRelative(0,MainActivity.systembarTop/2+ (int)(density*5) ,0,0);
    FloatingLayout layout=new FloatingLayout(act,view,leftlayout,fontsizeview,width,height);
    int addright,addleft;
    if(MainActivity.rtl&&Applic.supportsRtl) {
        addright=10;
        addleft=12;
        
        } 
    else {
        addright=12;
        addleft=10;
        }

    layout.setPadding(MainActivity.systembarLeft+(int)(density*addleft),0,MainActivity.systembarRight+(int)(density*addright),MainActivity.systembarBottom+(int)(density*5));
    layout.setBackgroundColor(Applic.backgroundcolor);
    transparant.setOnCheckedChangeListener( (buttonView,  isChecked) -> {
        Floating.setbackgroundalpha(isChecked?0:0xff);
        Floating.invalidatefloat();
        removeContentView(layout);
        act.poponback();
        show(act,parent);
    });
    touchable.setOnCheckedChangeListener( (buttonView,  isChecked) -> {
        Floating.setTouchable(isChecked);
        });
/*
    foregroundswitch.setOnCheckedChangeListener( (buttonView,  isChecked) -> {
        background=isChecked;
        removeContentView(layout);
        act.poponback();
        show(act,parent);

    });*/


        foregroundbutton.setOnCheckedChangeListener( (buttonView,  isChecked) -> {
            {if(doLog) {Log.i(LOG_ID,"foregroundbutton "+isChecked);};};
            backgroundbutton.setChecked(!isChecked);
            background=!isChecked;
            removeContentView(layout);
            act.poponback();
            show(act,parent);
            });

        backgroundbutton.setOnCheckedChangeListener( (buttonView,  isChecked) -> {
            {if(doLog) {Log.i(LOG_ID,"backgroundbutton "+isChecked);};};
            foregroundbutton.setChecked(!isChecked);
            });





           act.addMyContentView(layout, new ViewGroup.LayoutParams(MATCH_PARENT,MATCH_PARENT));
    Button noclose= act.findViewById(R.id.closeambi);
     if(noclose!=null) {
        noclose.setVisibility(GONE);
        noclose.setText("");
        Button nohelp= act.findViewById(R.id.helpambi);
        nohelp.setText("");
        nohelp.setVisibility(GONE);
      }
    act.setonback(()-> { 
        parent.setVisibility(VISIBLE);
        removeContentView(layout); 
        if(hidden[0]) {
            Floating.removeFloating();
            }

        });
    close.setOnClickListener(v->{
        act.doonback();
    });
}

}
