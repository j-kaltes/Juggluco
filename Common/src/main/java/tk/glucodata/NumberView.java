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

import android.view.Gravity;
import android.widget.LinearLayout;
import android.app.Activity;
import androidx.appcompat.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.graphics.Color;
import android.os.Build;
import android.text.Editable;
import android.text.InputType;
import android.text.Selection;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.Button;

import android.widget.DatePicker;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.HorizontalScrollView;
import android.widget.ListPopupWindow;
import android.widget.PopupWindow;
import android.widget.ScrollView;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.TimePicker;

import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.text.DateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;
import java.util.Locale;


import tk.glucodata.nums.AllData;
import tk.glucodata.nums.numio;

import static android.widget.LinearLayout.VERTICAL;
import static android.view.View.GONE;
import static android.view.View.INVISIBLE;
import static android.view.View.TEXT_ALIGNMENT_CENTER;
import static android.view.View.VISIBLE;
import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static android.widget.Spinner.MODE_DIALOG;
import static android.widget.Spinner.MODE_DROPDOWN;
import static java.lang.System.currentTimeMillis;
//import static tk.glucodata.Applic.smallScreen;
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.Applic.systemtimeformat;
import static tk.glucodata.GlucoseCurve.mktime;
import static tk.glucodata.Layout.getMargins;
import static tk.glucodata.Log.doLog;
import static tk.glucodata.MainActivity.systembarLeft;
import static tk.glucodata.MainActivity.systembarRight;
import static tk.glucodata.RingTones.EnableControls;
import static tk.glucodata.Specific.useclose;
import static tk.glucodata.settings.Settings.editoptions;
import static tk.glucodata.settings.Settings.getGenSpin;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getcheckbox;
import static tk.glucodata.util.getlabel;


public class NumberView {
private static final boolean SmallShowKeyboard=false;
public static  boolean smallScreen=false;
final private static String LOG_ID="NumberView";
Calendar cal = Calendar.getInstance();
Layout datepicker=null;
TextView dateview;
DatePicker datepick;
View newnumview=null;
Button deletebutton, savebutton,cancelbutton ;
long currentnum=0L;
Spinner spinner;
EditText valueedit;
TextView source=null;
Button timebutton,datebutton;
void deleteviews() {
    Log.i(LOG_ID,"deleteviews()");
    closenumview();
    spinner=null;
    if(newnumview!=null) {
        removeContentView(newnumview);
        newnumview=null;
        }
    if(datepicker!=null) {
        removeContentView(datepicker);
        datepicker=null;
        }
    if(timepicker!=null) {
        removeContentView(timepicker);
        timepicker=null;
        Log.i(LOG_ID,"timepicker=null");
        }
    if(keyboard!=null) {
        removeContentView(keyboard);
        keyboard=null;
        }
   cal = Calendar.getInstance();
    }
/*
/*
void rotatekey(float deg) {
    keyboard.setRotation(deg);
//    keyboard.bringToFront();
    newnumview.setRotation(deg);

//    newnumview.bringToFront();

    }*/
int labelsel=-1;
void closenumview() {
    if (newnumview != null) { 
        newnumview.setVisibility(GONE);
        hidekeyboard();
         }
    }

Button mealbutton;
CheckDirectionBox excludebox;
public void  addnumberview(MainActivity activity, long hitptr) {
    if(currentnum!=0L&&currentnum!=numio.newhit) 
            Natives.freehitptr(currentnum);
    long time= Natives.hittime(hitptr)*1000L;
    lasttime=time;
    int bron= Natives.gethitindex(hitptr);
    var type=Natives.hittype(hitptr);
    var exclude=Natives.hitexclude(hitptr);
    addnumberview(activity, bron,time,Natives.hitvalue(hitptr),type,-1);
    if(hitptr!=numio.newhit) {
        if(!Natives.staticnum()) {
            seedelete();
            }
        else {
            nodelete();
            }
        currentnum = hitptr;
        setmealbutton(type,bron, Natives.hitmeal(hitptr),exclude) ;
        }
    else {

        nodelete();
        setmealbutton(type,bron, 0,shouldexclude) ;
        currentnum=0L;
        }
           if(dateview!=null) {
               thedate = time;
              }

            if(timeview!=null) {
                cal.setTimeInMillis(time);
                int hour = cal.get(Calendar.HOUR_OF_DAY);
                int min = cal.get(Calendar.MINUTE);
                thetime=hour * 60 + min;
                }
        }
float roundto(float get,float ro) {
    return Math.round(get/ro)*ro;
    }
final private int[] newmealptr={0};
final private Layout[] mealview={null};
private long lasttime=0L;
private TextView messagetext;
boolean shouldexclude=false;
void  setExcludeTime(long time){
      shouldexclude = Natives.shouldExclude(time);
      EnableControls(excludebox, !shouldexclude);
      excludebox.setChecked(shouldexclude);
       }
//public static String minhourstr(Date dat) {
public static String minhourstr(long mmsec) {
   if(systemtimeformat())
      return Notify.timef.format(new Date(mmsec));
   else {
      Calendar cal = Calendar.getInstance();
      cal.setTimeInMillis(mmsec);
      return mktime(cal.get(Calendar.HOUR_OF_DAY),cal.get(Calendar.MINUTE));
      }
   }
public   View addnumberview(MainActivity context,final int bron,final long time,final float value,final int type,final int tmpmealptr) {
    if(newnumview==null) {
       // var mat = new MaterialButton(context); mat.setCornerRadius(GlucoseCurve.dpToPx(30)); datebutton=mat; 
      datebutton = new Button(context);
        datebutton.setOnClickListener(
                v -> getdateview(context));
    source=new TextView(context);
        dateview=datebutton;
        timebutton = new Button(context);

//        var mat2 = new MaterialButton(context); timebutton=mat2; mat2.setCornerRadius(GlucoseCurve.dpToPx(30));

        timeview=timebutton;
    mealbutton=getbutton(context,R.string.mealname);
    excludebox=getcheckbox(context,R.string.exclude,true);
    source.setMinWidth(mealbutton.getMinWidth());
    messagetext=getlabel(context,R.string.dontchangeamounts);
    mealbutton.setVisibility(GONE);
    excludebox.setVisibility(GONE);
    if(smallScreen) {
        valueedit=geteditwearos(context);
        }
    else  {
        valueedit = geteditview(context,new editfocus());
        }
    valueedit.setMinEms(isWearable?2:4);

        deletebutton = new Button(context);

        deletebutton.setText(R.string.delete);

        cancelbutton = new Button(context);
        cancelbutton.setText(R.string.cancel);
        savebutton = new Button(context);
        savebutton.setText(R.string.save);

    Button helpbutton;
    if(!isWearable) {
        helpbutton=getbutton(context,R.string.helpname);
        helpbutton.setOnClickListener(v-> help.helplight(R.string.newamount,context));
        }
    else {
       if(!useclose) cancelbutton.setVisibility(GONE);
        }
      Layout layout;

    if(isWearable) {
        int height=GlucoseCurve.getheight();
        int width=GlucoseCurve.getwidth();
        int hormarg= (int)(width*0.08f);
        getMargins(timebutton).setMarginEnd(hormarg);
        getMargins(datebutton).setMarginStart(hormarg); 
      if(useclose)  {
          layout = new Layout(context, (lay,w,h) -> { 
          return new int[]{w,h}; }, new View[]{datebutton,timebutton} ,new View[]{getspinner(context), valueedit}, new View[]{excludebox},new View[]{messagetext,savebutton,deletebutton},new View[]{cancelbutton});
      }
      else {
       layout = new Layout(context, (lay,w,h) -> {
   return new int[]{w,h};
   }, new View[]{datebutton,timebutton} ,new View[]{getspinner(context), valueedit},new View[]{excludebox}, new View[]{messagetext,savebutton,deletebutton});
   }


        if(true) {
          layout.setPaddingRelative((int)(width*0.01f),(int)(height*.15f),(int)(width*0.01f),(int)(height*.01f));
           ScrollView scroll=new ScrollView(context);
           scroll.setFillViewport(true);
        scroll.setSmoothScrollingEnabled(false);
          scroll.setScrollbarFadingEnabled(true);
          scroll.setVerticalScrollBarEnabled(true);
//           scroll.addView(layout, new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));
           scroll.addView(layout, new ViewGroup.LayoutParams(MATCH_PARENT, MATCH_PARENT));
           newnumview=scroll;
           }
         else {
           var frame=new FrameLayout(context);
           frame.addView(layout, new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));
           newnumview=frame;
             }
             /*
      newnumview.measure(MATCH_PARENT, MATCH_PARENT);
      var h=newnumview.getMeasuredHeight();
      var w=newnumview.getMeasuredWidth();
       if(height>h)
               newnumview.setY((height-h)*.5f+GlucoseCurve.metrics.density*10.0f);
        if(width>w)
           newnumview.setX((width-w)*.5f);  */
      }
  else { 
   layout=new Layout(context, (lay, w, h) -> {
        int wid=GlucoseCurve.getwidth()- systembarRight;
        if(!smallScreen) {
            {if(doLog) {Log.i(LOG_ID,"no smallScreen");};};
            int hei=GlucoseCurve.getheight();
               if(wid>hei) {
                  int minleft=systembarLeft*3/4;
                if(hei>h)
                    lay.setY((hei - h) / 2);
                   else
                    lay.setY(MainActivity.systembarTop*3/4);
                 if(wid>w) {
                    int half= wid / 2;
                    int af=(half-w)/4;
                    int posx=half - w-af;
                    if(posx<minleft) {
                    posx=minleft;
                    noroom=true;
                    }
                    else
                    noroom=false;
                    lay.setX(posx);
                  }
                    else
                        lay.setX(minleft);
                }
            else {
                if(wid>w)
                    lay.setX((wid - w)/2);
                  else {
                    lay.setX(0);
                      }
                 if(hei>h) {
                    int half=hei/2;
                    int af=(half-h)/4;
                    lay.setY(half - h-af);
                    }
                   else

                    lay.setY(MainActivity.systembarTop*3/4);

                }
            }
        else {
            {if(doLog) {Log.i(LOG_ID,"smallScreen");};};
            if(wid>w)
                lay.setX((wid - w)/2);
              lay.setY(MainActivity.systembarTop*3/4);
            }

            return new int[] {w,h}; },new View[]{helpbutton,getspinner(context), valueedit},new View[]{datebutton, excludebox,mealbutton,source,timebutton},new View[]{cancelbutton,messagetext,deletebutton, savebutton});
         }

        timebutton.setOnClickListener(
                v -> {
                  layout.setVisibility(GONE);
                  gettimeview(context,()-> {
                     layout.setVisibility(VISIBLE);
                     if(keyboard!=null)
                        keyboard.setVisibility(VISIBLE);
                  });
                });





        deletebutton.setOnClickListener( v-> {
            if(mealview[0]!=null) {
                removeContentView(mealview[0]);
                mealview[0]=null;
                ((MainActivity)v.getContext()).poponback();
                }
            deletedialog(v,newmealptr);

            });
    if(isWearable) {

        }
    else  {
        newnumview=layout;
        } 
    newnumview.setBackgroundColor( Applic.backgroundcolor);
  savebutton.setOnClickListener(v -> {
        MainActivity act=(MainActivity)v.getContext();
        GlucoseCurve.reopener();
        if(saveamount(act,timeview,  valueedit,newmealptr[0],lasttime)) {
            if(mealview[0]!=null) {
                removeContentView(mealview[0]);
                mealview[0]=null;
                    act.poponback();
                }
                newmealptr[0]=0;
        //        Natives.closemeal(newmealptr[0]);
             final var nview=newnumview;
             if(nview!=null)
                nview.setVisibility(GONE);
            if(!isWearable)
                hidekeyboard();
            if(smallScreen)
                help.hidekeyboard(act);
                
            ((Applic) act.getApplication()). redraw();
              MainActivity.poponback();

             if(Menus.on) {
                if(deletebutton.getVisibility()==GONE) {
                        Menus.show(context);
                        }
                }
            } 
            //            act.clearonback();
        });

    context.addContentView(newnumview,isWearable? new ViewGroup.LayoutParams(MATCH_PARENT, MATCH_PARENT):new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));
        }
    else  {
    numspinadapt.setarray(Natives.getLabels());
        newnumview.setVisibility(VISIBLE);
    }
    valueedit.requestFocus();
    editfocus.setedittext(valueedit);

    View.OnClickListener menucall= v -> {

        if(!isWearable) {
            hidekeyboard();
        if(mealview[0]==null) {
            EnableControls(newnumview,false);
//            newnumview.setVisibility(GONE);
            int mptr=newmealptr[0]==0?((currentnum!=0L)?Natives.hitmeal(currentnum):0):newmealptr[0];
            if(mptr==0)
                mptr=Natives.getnewmealptr();
            mealview[0]=tk.glucodata.Meal.menuview(NumberView.this, context,mptr, (carb,mealptr)->{
                float roundt=Natives.getroundto();
                if(roundt>0.0f)
                    valueedit.setText(Float.toString(roundto(carb,roundt)));
                else
                    valueedit.setText(Float.toString(carb));
                newmealptr[0]=mealptr;
            },()->{
                EnableControls(newnumview,true);
                valueedit.requestFocus();

                if(!smallScreen) {
                    if(SmallShowKeyboard)
                            showkeyboard(context);
                    editfocus.setedittext(valueedit);
                    }
               else {
                    tk.glucodata.help.showkeyboard(context,valueedit);
                  }

            //    newnumview.setVisibility(VISIBLE);
                mealview[0]=null;

                }    );
            }
    }

    };
    mealbutton.setOnClickListener(menucall);
    lasttime=time;
    Date dat = new Date(time);
   if(isWearable)
      datebutton.setText(DateFormat.getDateInstance(DateFormat.SHORT).format(dat));
   else
       datebutton.setText(DateFormat.getDateInstance(DateFormat.DEFAULT).format(dat));
    timebutton.setText(minhourstr(time));
    if(value< Float.MAX_VALUE)
        valueedit.setText(String.valueOf(value));
    else
        valueedit.setText("");
    spinner.setSelection(type);
    editfocus.setedittext(valueedit);
    source.setText( bron==1?"      ":"           \u231A         ");
    source.setTextAlignment( TEXT_ALIGNMENT_CENTER);
    int pads=(int)(GlucoseCurve.metrics.density*10);
        source.setPaddingRelative(0,pads,0,pads);
    if(!Natives.staticnum()) {
        messagetext.setVisibility(GONE);
        savebutton.setVisibility(VISIBLE);
        }
    else  {
        savebutton.setVisibility(GONE);
        messagetext.setVisibility(VISIBLE);
        }
     final Runnable closerRun= () -> {
                if(newmealptr[0]!=0) {
                if(currentnum!=0&&(currentnum!=numio.newhit)) {
                    Natives.hitsetmealptr(currentnum,newmealptr[0]);
                    }
                else
                    Natives.deletemeal(newmealptr[0]);
                newmealptr[0]=0;
                }
            if(mealview[0]!=null) {
                removeContentView(mealview[0]);
                mealview[0]=null;
                }

        if(currentnum!=0) {
            if(currentnum!=numio.newhit) 
                    Natives.freehitptr(currentnum);

            currentnum=0L;
            }
        else {
             if(Menus.on) {
                Menus.show(context);
                }
            }

           GlucoseCurve.reopener();
           final var nview=newnumview;
           if(nview!=null)
               nview.setVisibility(GONE);
           if(!isWearable)  {
               hidekeyboard();
            if(smallScreen)
                help.hidekeyboard(context);
              }
            };
        cancelbutton.setOnClickListener(v -> {
            MainActivity.poponback();
            closerRun.run();
            });

     context.setonback(closerRun);

    if(tmpmealptr>=0) {
        timebutton.setTextColor( Color.YELLOW);
        datebutton.setTextColor( Color.YELLOW);
         newmealptr[0]=tmpmealptr;
        mealview[0]=null;
        {if(doLog) {Log.i(LOG_ID,"onClick");};};
        menucall.onClick(mealbutton);
        }
    else {
        timebutton.setTextColor(savebutton.getCurrentTextColor());
        datebutton.setTextColor(savebutton.getCurrentTextColor());
    }

    setExcludeTime(time);
    return newnumview;
    }
void deletedialog(View v,int[] mealptr) {
    if(currentnum==0L) {
        newnumview.setVisibility(GONE);
        hidekeyboard();
        return;
        }
    MainActivity  context=  ((MainActivity) v.getContext());

    long time= Natives.hittime(currentnum)*1000L;
    float value=Natives.hitvalue(currentnum);
    int type=Natives.hittype(currentnum);
    ArrayList<String> labels= ((Applic)context.getApplication()).getlabels();
    String mess= DateFormat.getDateTimeInstance(DateFormat.DEFAULT,DateFormat.DEFAULT).format(time)+" "+ labels.get(type)+" "+value;
        AlertDialog.Builder builder = new AlertDialog.Builder(context);
        builder.setTitle(R.string.deletequestion).setMessage(mess).
           setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
            if(mealptr[0]!=0)
                 Natives.deletemeal(mealptr[0]);
                 mealptr[0]=0;
            if(currentnum!=0) {
                if(currentnum!=numio.newhit) {
                    int index=Natives.gethitindex(currentnum);
                    int waslast= numio.getlastnum(index);
                    int pos=Natives.hitremove(currentnum);
                    int last=numio.getlastnum(index);
                    if(!isWearable) {
                        AllData alldata=((Applic) ((Activity) v.getContext()).getApplication()).numdata;
                        alldata.deletelast(index,last,waslast);
                        if(pos<last)
                             alldata.changedback(index);
                        }
                        Natives.freehitptr(currentnum);
                      ((Applic) ((Activity) v.getContext()).getApplication()). redraw();
                      }
                currentnum=0L;
                }
                /*
            else  {
             if(Menus.on) {
                Menus.show(context);
                }
                } */
         newnumview.setVisibility(GONE);

           hidekeyboard();
        GlucoseCurve.reopener();
        context.poponback();

                    }
                }) .setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {
            }
        }).show().setCanceledOnTouchOutside(false);
    }


private void nodelete() {
    deletebutton.setVisibility(GONE);
    if(isWearable)
        getMargins(savebutton).setMarginStart(0);
    }

private void seedelete() {
    deletebutton.setVisibility(VISIBLE);
    if(isWearable) {
        int width=GlucoseCurve.getwidth();
        int hormarg=useclose?(int)(width*0.05f):(int)(width*0.12f);
        getMargins(deletebutton).setMarginEnd(hormarg);
        getMargins(savebutton).setMarginStart(hormarg);
        }
    }
public void addnumberwithmenu(MainActivity context,int mealptr) {
    if(currentnum!=0L)  {
        if(currentnum!=numio.newhit) 
            Natives.freehitptr(currentnum);
        currentnum=0L;
        }
    var type=Natives.getmealvar();
     addnumberview(context,1,currentTimeMillis(),Float.MAX_VALUE,type,mealptr);
    setmealbutton(type,1, 0,shouldexclude) ;
    nodelete();
    thetime=-1;
    thedate=0L;
    }
public View addnumberview(MainActivity context) {
    if(currentnum!=0L)  {
        if(currentnum!=numio.newhit) 
            Natives.freehitptr(currentnum);
        currentnum=0L;
        }
    View lay=  addnumberview(context,1,currentTimeMillis(),Float.MAX_VALUE,0,-1);
    setmealbutton(0,1, 0,shouldexclude) ;
    
if(SmallShowKeyboard&&smallScreen) {
    valueedit.requestFocus();
    tk.glucodata.help.showkeyboard(context,valueedit);
  }
else
    spinner.performClick();
    nodelete();
    thetime=-1;
    thedate=0L;
    return lay;
    }


public void  addnumberview(MainActivity activity, int bron, int pos) {
     addnumberview(activity, Natives.mkhitptr(numio.numptrs[bron],pos)); 
    }


long thedate=0;
int thetime=-1;
private boolean saveamount(Activity activity,TextView timeview,TextView value,int mealptr,long lasttime) {
    final String strval= value.getText().toString();
    float val=0.0f;
    try {
        val=(strval.length()==0)?0:Float.parseFloat(strval);
        }
    catch(Throwable e) {
        Log.stack(LOG_ID,"parseFloat "+strval,e);
        };

    if(labelsel==Natives.getbloodvar()) { 
        mealptr=excludebox.isChecked()?1:0;
        }
    if(currentnum!=0&&currentnum!=numio.newhit) {
//        long dat=thedate==0L?Natives.hittime(currentnum)*1000L:thedate;
        long dat=thedate==0L?lasttime:thedate;
/*        if(timeview!=null) {
            cal.setTimeInMillis(dat);
            int minutes = thetime;
            if(minutes>=0) {
                cal.set(Calendar.HOUR_OF_DAY, minutes / 60);
                cal.set(Calendar.MINUTE, minutes % 60);
                cal.set(Calendar.SECOND,0);
                }
            dat= cal.getTimeInMillis();
            } */
        Natives.hitchange(currentnum,dat/1000L,val,labelsel,mealptr);
        int index=Natives.gethitindex(currentnum);
        if(!isWearable) {
            tk.glucodata.nums.AllData  alldata=Applic.app.numdata;
            alldata.changedback(index);
            }
        Natives.freehitptr(currentnum);
        }

    else {
        long dat=thedate==0L?lasttime:thedate;
        /* cal.setTimeInMillis(dat);
        if(timeview!=null) {
            int minutes = thetime;
            if(minutes>=0) {
                cal.set(Calendar.HOUR_OF_DAY, minutes / 60);
                cal.set(Calendar.MINUTE, minutes % 60);
                cal.set(Calendar.SECOND,0);
                }
            }
        dat= cal.getTimeInMillis(); */
        final int index=1;
        Natives.saveNum(numio.numptrs[index],dat/1000,val,labelsel,mealptr);
        if(!isWearable) {
           tk.glucodata.nums.AllData  alldata=Applic.app.numdata;
            alldata.changedback(index);
            }
        }

    currentnum=0L;
    return true;
    }
Dater dater=null;

Dater numdater=(year,month,day)-> {
         cal.set(Calendar.YEAR,year);
         cal.set(Calendar.MONTH,month);
         cal.set(Calendar.DAY_OF_MONTH,day);
         long dat= cal.getTimeInMillis();
        thedate=dat;
        setExcludeTime(dat);
        dateview.setText( DateFormat.getDateInstance( DateFormat.DEFAULT) .format(dat)); } ;

Layout getdateview(MainActivity activity) {
    long tim=(thedate==0L)?currentTimeMillis():thedate;
    return getdateviewal(activity,tim,numdater);
    }


public Layout getdateviewal(MainActivity activity, long date, Dater erdate) {
{if(doLog) {Log.i(LOG_ID, "getdateviewal");};};
    dater=erdate;
    if(datepicker==null) {
    {if(doLog) {Log.i(LOG_ID, " new");};};
    datepick =new DatePicker(activity);
    datepick.setCalendarViewShown(false);
        Button cancel=new Button(activity);

        cancel.setText(R.string.cancel);
        cancel.setOnClickListener(vi -> { 
        activity.doonback();
        });
        Button ok=new Button(activity);
        ok.setText(R.string.ok);
 //      ok.setBackgroundResource(R.drawable.button_selector2);
        ok.setOnClickListener(vi -> {
        activity.doonback();
        if(keyboard!=null)
            EnableControls(keyboard,true);
        datepicker.setVisibility(GONE);
        if(newnumview!=null) EnableControls(newnumview,true);
        int day=datepick.getDayOfMonth();
        int month=datepick.getMonth();
        int year=datepick.getYear();
        dater.date(year,month,day);

        });
   ViewGroup.LayoutParams datparams;
    if(isWearable) {
         if(!useclose)
            cancel.setVisibility(GONE);
        datepicker=new Layout(activity,
                (lay, w, h)->{
            return new int[] {w,h};
                },new View[]{cancel},new View[] {datepick},new View[] {ok});
        int laypar= MATCH_PARENT;
       datepicker.setPaddingRelative(0,(int)(GlucoseCurve.metrics.density*5.0),0,(int)(GlucoseCurve.metrics.density*2.0));
       datparams=new ViewGroup.LayoutParams(laypar,laypar);
        }
    else {
        datepicker=new Layout(activity,
                (lay, w, h)->{
/*
            int height=GlucoseCurve.getheight();
            int width=GlucoseCurve.getwidth();
        if(w>width||h>height) {
            lay.setX(0);
            lay.setY(0);
            }
        else {
                    lay.setX((width-w)/2);
                    lay.setY((height-h)/2);
            }
*/
            return new int[] {w,h};
                },new View[] {datepick},new View[] {cancel,ok});
        datparams =    new FrameLayout.LayoutParams( WRAP_CONTENT, WRAP_CONTENT, Gravity.CENTER|Gravity.CENTER_HORIZONTAL);
        }

    datepicker.setBackgroundColor( Applic.app.backgroundcolor);
    activity.addContentView(datepicker, datparams);
    }
    else {
    {if(doLog) {Log.i(LOG_ID, " old");};};
        datepicker.setVisibility(VISIBLE);
    datepicker.bringToFront();
    }

cal.setTimeInMillis(date);

datepick.updateDate( cal.get(Calendar.YEAR), cal.get(Calendar.MONTH), cal.get(Calendar.DAY_OF_MONTH));
activity.setonback(()->{ 
    if(keyboard!=null)
        EnableControls(keyboard,true);
    datepicker.setVisibility(GONE);

    if(newnumview!=null)
        EnableControls(newnumview,true);
    else {
        if(Menus.on)
                    Menus.show(activity);
        }
        });

if(newnumview!=null)
    EnableControls(newnumview,false);
if(keyboard!=null)
    EnableControls(keyboard,false);
return datepicker;
}

Layout timepicker=null;
TextView timeview=null;
TimePicker pick=null;

ObjIntConsumer<Integer> settime=null;
ObjIntConsumer<Integer>  numsettime=(hour,min)-> {
    thetime= hour*60+min;
    cal.setTimeInMillis(thedate==0?lasttime:thedate);
    int minutes = thetime;
    if(minutes>=0) {
        cal.set(Calendar.HOUR_OF_DAY, minutes / 60);
        cal.set(Calendar.MINUTE, minutes % 60);
        cal.set(Calendar.SECOND,0);
    }
    thedate= cal.getTimeInMillis();
     setExcludeTime(thedate);
    timeview.setText(String.format(Locale.US,"%02d:%02d",hour,min ));
    };
void gettimeview(MainActivity activity,Runnable parent) {
    int id=thetime; 
    int h,m;
    if(id>=0)  {
        h=id/60;
        m=id%60;
        }
    else {
        cal.setTimeInMillis(currentTimeMillis());
         h=cal.get(Calendar.HOUR_OF_DAY);
         m=cal.get(Calendar.MINUTE);
        }
    if(keyboard!=null) {
        keyboard.setVisibility(GONE);
        }
    gettimepicker(activity,h,m,numsettime,parent);
    }
    @SuppressWarnings("deprecation")
//Layout buttonlay;
public void gettimepicker(MainActivity activity,int hourin, int minin, ObjIntConsumer<Integer> timeset,Runnable onclose) {
final  boolean buttonsunder=false;
   settime=timeset;
    if(timepicker==null) {
        Log.i(LOG_ID,"new gettimepicker");
        pick =new TimePicker(activity);
//        pick.setIs24HourView( android.text.format.DateFormat.is24HourFormat(activity));
        Button cancel=new Button(activity);
        cancel.setText(R.string.cancel);
        cancel.setOnClickListener(vi -> { 
        activity.doonback();

        });
        Button ok=new Button(activity);
        ok.setText(R.string.ok);
        ok.setOnClickListener(vi -> {
        activity.doonback();
            int hour,min;
            if(Build.VERSION.SDK_INT < 23) {
                 hour=pick.getCurrentHour();
                 min=pick.getCurrentMinute(); }
            else {
                 hour=pick.getHour();
                 min=pick.getMinute();
                }
        settime.accept(hour,min);

        });
    View[][] views;
     int layparwidth,layparheight;
if(isWearable) {
      if(!useclose) cancel.setVisibility(GONE);
           views=new View[][]{new View[]{cancel},new View[]{pick},new View[]{ok}};
         layparheight=layparwidth=MATCH_PARENT;

         }
else {
   layparheight=ViewGroup.LayoutParams.WRAP_CONTENT;
       if(buttonsunder) {
           views=new View[][]{new View[]{pick},new View[]{cancel,ok}};
         layparwidth=WRAP_CONTENT;
           }
       else {
         //layparwidth=MATCH_PARENT;
         layparwidth=WRAP_CONTENT;
        //  buttonlay=new Layout(activity,new View[] {cancel},new View[]{ok});
        //buttonlay.usebaseline=false;
         // buttonlay.setLayoutParams(new ViewGroup.LayoutParams(  WRAP_CONTENT , ViewGroup.LayoutParams.MATCH_PARENT));
         var buttons=new LinearLayout(activity);
         buttons.setOrientation(VERTICAL);
         buttons.addView(cancel);
         buttons.addView(ok);
         views=new View[][] {new View[] {pick,buttons}};
        };
       };
//        buttonlay.setBackgroundColor( RED);
//     var laypar=smallScreen?WRAP_CONTENT:MATCH_PARENT;
    pick.setLayoutParams(new ViewGroup.LayoutParams(layparwidth ,ViewGroup.LayoutParams.WRAP_CONTENT ));
//    pick.setLayoutParams(new ViewGroup.LayoutParams(WRAP_CONTENT , ViewGroup.LayoutParams.WRAP_CONTENT));
        Layout layout=new Layout(activity,
                (lay, w, h)-> {
                    activity.hideSystemUI();
                    /*
                    int wid = GlucoseCurve.getwidth();
                    if(w>=wid) {
                        lay.setX(0);
                        }
                    else {
                        int x=(wid-w)/2;
                        lay.setX(x);
                        {if(doLog) {Log.i(LOG_ID,"screen width="+wid+" w="+w+" x="+x);};};
                        } 
                    if(isWearable) {
                        int height = GlucoseCurve.getheight();
                        if(height>h) {
                                lay.setY((height-h)/2);
                        }
    
                    }
                    */
                    return new int[]{w, h};
                }, views);


        layout.setBackgroundColor( Applic.backgroundcolor);
    //    activity.addContentView(layout,  new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));
    var  params =    new FrameLayout.LayoutParams(layparwidth,layparwidth, Gravity.CENTER_HORIZONTAL);

    //l act.addContentView(layout, params);
        activity.addContentView(layout,  params);
        timepicker=layout;
   if(isWearable)
          layout.setPaddingRelative(0,(int)(GlucoseCurve.metrics.density*5.0),0,(int)(GlucoseCurve.metrics.density*2.0));
    }
    else {
        Log.i(LOG_ID,"old gettimepicker");
    timepicker.requestLayout();
    timepicker.setVisibility(VISIBLE);
    timepicker.bringToFront();
    }

  //    timepicker.setPaddingRelative(systembarLeft,MainActivity.systembarTop, systembarRight,MainActivity.systembarBottom);
if(!isWearable) {
    if(buttonsunder) 
        timepicker.setPaddingRelative(systembarLeft,MainActivity.systembarTop, systembarRight,MainActivity.systembarBottom);
   else  {
         pick.setPaddingRelative(systembarLeft,MainActivity.systembarTop,0,MainActivity.systembarBottom);
        //buttonlay.setPaddingRelative(0,MainActivity.systembarTop/2, systembarRight,MainActivity.systembarBottom);
        }
     }

     pick.setIs24HourView(Applic.hour24);
activity.setonback(
        () -> {
            onclose.run();
            activity.hideSystemUI();
            timepicker.setVisibility(GONE);
            if(newnumview!=null)
                EnableControls(newnumview,true);

        }
    );

if(newnumview!=null)
    EnableControls(newnumview,false);
pick.setCurrentHour(hourin);
pick.setCurrentMinute(minin);
}
LabelAdapter<String> numspinadapt;

void setmealbutton(int labelsel,int bron,int mealptr,boolean exclude) {
       if(doLog) {Log.i(LOG_ID,"bron="+bron+" mealptr="+mealptr);};
        if(!isWearable&&labelsel==Natives.getmealvar() &&(bron==1|| mealptr>0)) {
            mealbutton.setVisibility(VISIBLE);
            source.setVisibility(GONE);
            excludebox.setVisibility(GONE);
            }

    else {
       if(labelsel==Natives.getbloodvar()) {
            mealbutton.setVisibility(GONE);
            source.setVisibility(GONE);
            excludebox.setVisibility(VISIBLE);
            if(shouldexclude)
                excludebox.setChecked(true);
            else
                excludebox.setChecked(exclude);
            }
        else  {
            mealbutton.setVisibility(GONE);
            source.setVisibility(VISIBLE);
            excludebox.setVisibility(GONE);
            }
            }
  
      }
void setmealbutton(int labelsel,long hitptr) {
    boolean here=(hitptr==0L||hitptr==numio.newhit);
    setmealbutton(labelsel, here?1:Natives.gethitindex(hitptr),here?1:Natives.hitmeal(hitptr),here?shouldexclude:Natives.hitexclude(hitptr));
    }
Spinner getspinner(Activity context) {
if(spinner==null) {
    spinner=getGenSpin(context);
    numspinadapt=new LabelAdapter<String>(context,Natives.getLabels(),1);
    spinner.setAdapter(numspinadapt);
    spinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
        @Override
        public  void onItemSelected (AdapterView<?> parent, View view, int position, long id) {
            labelsel=position;
            setmealbutton(position,currentnum);
            }
        @Override
        public  void onNothingSelected (AdapterView<?> parent) {
            labelsel=-1;

        } });
    }
return spinner;

}

static EditText geteditview(Context context,View.OnFocusChangeListener focus) {
    EditText  under=new EditText(context);// under.setText(str);
    under.setInputType( InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL);
    under.setMinEms(2);
    under.setOnFocusChangeListener(focus);
    under.setSelectAllOnFocus(true);
    under.setMinHeight(GlucoseCurve.dpToPx(50));
    closekeyboard(under) ;
    return under;
}

static EditText geteditwearos(Context context) {
    var valedit=new EditText(context);
    valedit.setInputType(InputType.TYPE_CLASS_NUMBER |InputType.TYPE_NUMBER_FLAG_DECIMAL);//| InputType.IME_FLAG_NO_FULLSCREEN);
    valedit.setImeOptions(editoptions);
    return valedit;
    }
/*
static View.OnTouchListener ontouchedit= new View.OnTouchListener() {

        @Override
        public boolean onTouch(View v, MotionEvent event) {
    {if(doLog) {Log.v(LOG_ID,"ontouchedit");};};
            v.onTouchEvent(event);
//      EditText ed= (EditText)v; editfocus.setedit(ed.getText());

    MainActivity act=(MainActivity)v.getContext();
            InputMethodManager imm = (InputMethodManager)act.getSystemService(Context.INPUT_METHOD_SERVICE);

            if (imm != null) {
                imm.hideSoftInputFromWindow(v.getWindowToken(), 0);
            }
        
        act.hideSystemUI();

            return true;
        }
    };
*/
    Layout keyboard;

static class numlisten implements View.OnClickListener {


    @Override
    public void onClick(View v) {
    final Editable edit= editfocus.getedit();
    if(edit!=null) {
        try {
            int start= Selection.getSelectionStart(edit);
            int end=Selection.getSelectionEnd(edit);
            Button but=(Button)v;
            edit.replace(start, end, but.getText());
            Selection.setSelection(edit, start + 1);
            } catch(Throwable e) {
                Log.stack(LOG_ID,e);
                }
    }

    }
}
boolean noroom=false;
Layout getkeyboard(Context context) {

   numlisten click=new numlisten();
//    Layout layout=new Layout(context,row0,row1,row2,row3);
    View [][] views=new View[4][];
    for(int i=2,num=1;i>=0;i--) {
        views[i]=new View[3];
        for(int j=0;j<3;j++) {
            Button but=new Button(context);
            views[i][j]=but;
            but.setText(String.valueOf(num++));
            but.setOnClickListener(click);
            }
        }
    View[] tmp=views[3]=new View[3];
    Button but= new Button(context);
    but.setOnClickListener(click);
    tmp[0]=but;
    but.setText("0");
    tmp[1]=but=new Button(context);
//    but.setText(Build.VERSION.SDK_INT>=22?"⌫":"Del");
    but.setText(Build.VERSION.SDK_INT>=22?"\u232B":"\u2190");
//    but.setText(Build.VERSION.SDK_INT>=22?"\u232B":"\u21e6");
//    but.setText(Build.VERSION.SDK_INT>=22?"\u232B":"\u27f5");

    but.setContentDescription("Backspace");
    but.setOnClickListener(v->{
        int start= Selection.getSelectionStart(editfocus.getedit());
        int end=Selection.getSelectionEnd(editfocus.getedit());
        if(end>start) {
            editfocus.getedit().replace(start, end, "");
            }
        else {
            if(start>0)
                editfocus.getedit().replace(--start, end, "");
            }
        Selection.setSelection(editfocus.getedit(), start);
    } );
    tmp[2]=but=new Button(context);
    but.setText(".");
    but.setContentDescription("Decimal point");
    but.setOnClickListener(click);
    Layout layout=new Layout(context, (lay, w, h)->{
    int hei=GlucoseCurve.getheight();
    int wid=GlucoseCurve.getwidth();
    /*
    if(curve!=null)  {
        hei=curve.getHeight();
        wid=curve.getWidth();
        {if(doLog) {Log.i(LOG_ID,"height="+height+" hei="+hei);};};
        }
    else {
        {if(doLog) {Log.i(LOG_ID,"only height="+height);};};
        hei=height;
        wid=width;
        }*/
    if(wid>hei) {
        lay.setY((hei-h)/2);
      int mostright=wid-w-systembarRight;
        if(noroom)
            lay.setX(mostright);
        else {
            int half= (wid-systembarRight)/2;
            int bij=(half-w)/4;
         int xpos=half+bij;
         if(xpos>mostright)
            xpos=mostright;
         lay.setX(xpos);

        }

//        lay.setX(wid-w);
        }
    else {
        int half=hei/2;
        int bij=(half-h)/4;
        lay.setY(half+bij);
        lay.setX((wid-w)/2); 

    }

            return new int[] {w,h};
        }, views) ;
        
    layout.setBackgroundColor( Applic.backgroundcolor);
    layout.post(layout::requestLayout);
    return layout;
    }

public    void showkeyboard(Activity context) {
if(!isWearable) {
    if(keyboard==null) {
    keyboard=getkeyboard(context);
    context.addContentView(keyboard, new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));
    }
    else {
        keyboard.setVisibility(VISIBLE);
    keyboard.bringToFront();
    }
    }
    }
public    void hidekeyboard() {
if(!isWearable) {
    if(keyboard!=null)
        keyboard.setVisibility(GONE);
        }
    }
    /*
private static void setMode(TimePicker timepicker,int mode) {
        try {
        Field mModeField = timepicker.getClass().getDeclaredField("mMode");
//        Field mModeField = timepicker.getClass().getField("mMode");
        mModeField.setAccessible(true);
          Field modifiersField = Field.class.getDeclaredField("modifiers");
          modifiersField.setAccessible(true);
          modifiersField.setInt(mModeField, mModeField.getModifiers() & ~Modifier.FINAL);
        mModeField.setInt(timepicker, mode);
        } catch (Throwable e) {
        Log.stack(LOG_ID,e);
        }
     } */

public static void avoidSpinnerDropdownFocus(Spinner spinner) {
    try {
        Field listPopupField = Spinner.class.getDeclaredField("mPopup");
        listPopupField.setAccessible(true);
        Object listPopup = listPopupField.get(spinner);
        if (listPopup instanceof ListPopupWindow) {
    /*        {if(doLog) {Log.i("SPINNER","listpopupwin="+ ((ListPopupWindow) listPopup).getAnimationStyle());};};
            ((ListPopupWindow) listPopup).setAnimationStyle(0);*/
            Field popupField = ListPopupWindow.class.getDeclaredField("mPopup");
            popupField.setAccessible(true);
            Object popup = popupField.get((ListPopupWindow) listPopup);
            if (popup instanceof PopupWindow) { {
          PopupWindow popupwin=(PopupWindow) popup;
                  popupwin.setFocusable(false);
//          {if(doLog) {Log.i("SPINNER","popanim="+popupwin.getAnimationStyle());};};
        }
            }
        }
    } catch (Throwable e) {
        Log.stack(LOG_ID,e);
    }
}

//https://www.programmersought.com/article/75522638732/
static public void closekeyboard(EditText view) {
        ((Activity)view.getContext()).getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_HIDDEN);
        try {
            Class<EditText> cls = EditText.class;
            Method setSoftInputShownOnFocus;
            setSoftInputShownOnFocus = cls.getMethod("setShowSoftInputOnFocus", boolean.class);
            setSoftInputShownOnFocus.setAccessible(true);
            setSoftInputShownOnFocus.invoke(view, false);
        } catch(Throwable e) {
            Log.stack(LOG_ID,e);
        }
    }
}
