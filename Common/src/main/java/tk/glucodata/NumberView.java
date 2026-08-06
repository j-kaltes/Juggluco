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
    if(datepicker!=null) {
        removeContentView(datepicker);
        datepicker=null;
        datepick=null;
        }
    if(timepicker!=null) {
        removeContentView(timepicker);
        timepicker=null;
        pick=null;
        Log.i(LOG_ID,"timepicker=null");
        }
   cal = Calendar.getInstance();
    }
void requestOverlayLayout() {
    if(newnumview!=null)
        newnumview.requestLayout();
    if(keyboard!=null)
        keyboard.requestLayout();
    if(datepicker!=null)
        datepicker.requestLayout();
    if(timepicker!=null)
        timepicker.requestLayout();
    }
private static boolean replacePicker(Layout parent,View oldPicker,View newPicker) {
    int index=parent.indexOfChild(oldPicker);
    if(index<0)
        return false;
    Object row=oldPicker.getTag(R.id.layoutrow);
    ViewGroup.LayoutParams params=oldPicker.getLayoutParams();
    parent.removeViewAt(index);
    if(params!=null)
        newPicker.setLayoutParams(params);
    newPicker.setTag(R.id.layoutrow,row);
    parent.addView(newPicker,index);
    return true;
    }
@SuppressWarnings("deprecation")
void configurationChanged(MainActivity activity) {
    if(datepicker!=null) {
        if(datepicker.getVisibility()!=VISIBLE||datepick==null) {
            removeContentView(datepicker);
            datepicker=null;
            datepick=null;
            }
        else {
            int year=datepick.getYear();
            int month=datepick.getMonth();
            int day=datepick.getDayOfMonth();
            DatePicker fresh=new DatePicker(activity);
            fresh.setCalendarViewShown(false);
            fresh.updateDate(year,month,day);
            if(replacePicker(datepicker,datepick,fresh))
                datepick=fresh;
            }
        }
    if(timepicker!=null) {
        if(timepicker.getVisibility()!=VISIBLE||pick==null) {
            removeContentView(timepicker);
            timepicker=null;
            pick=null;
            }
        else {
            int hour=Build.VERSION.SDK_INT<23?pick.getCurrentHour():pick.getHour();
            int minute=Build.VERSION.SDK_INT<23?pick.getCurrentMinute():pick.getMinute();
            TimePicker fresh=newTimePicker(activity);
            fresh.setIs24HourView(Applic.hour24);
            fresh.setCurrentHour(hour);
            fresh.setCurrentMinute(minute);
            if(replacePicker(timepicker,pick,fresh))
                pick=fresh;
            }
        }
    requestOverlayLayout();
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
    hidekeyboard();
    if(mealview[0]!=null) {
        removeContentView(mealview[0]);
        mealview[0]=null;
        }
    if (newnumview != null) {
        removeContentView(newnumview);
        newnumview=null;
        }
    editfocus.clearedittext(valueedit);
    spinner=null;
    numspinadapt=null;
    valueedit=null;
    source=null;
    timebutton=null;
    timeview=null;
    datebutton=null;
    dateview=null;
    mealbutton=null;
    excludebox=null;
    messagetext=null;
    deletebutton=null;
    savebutton=null;
    cancelbutton=null;
    }

Button mealbutton;
CheckDirectionBox excludebox;
public void  addnumberview(MainActivity activity, long hitptr) {
    final boolean oldnum=hitptr!=numio.newhit;
    if(currentnum!=0L&&currentnum!=numio.newhit)  {
            Natives.freehitptr(currentnum);
            }
    long time= Natives.hittime(hitptr)*1000L;
    lasttime=time;
    int bron= Natives.gethitindex(hitptr);
    var type=Natives.hittype(hitptr);
    var exclude=Natives.hitexclude(hitptr);
    addnumberview(activity, bron,time,Natives.hitvalue(hitptr),type,-1);
    if(oldnum) {
        boolean staticnum=Natives.staticnum();
       // Log.i(LOG_ID,"addnumberview oldnum staticnum="+staticnum);
        if(!staticnum) {
            seedelete();
            }
        else {
            nodelete();
            }
        currentnum = hitptr;
        setmealbutton(type,bron, Natives.hitmeal(hitptr),exclude) ;
        }
    else {
        //Log.i(LOG_ID,"addnumberview new num");
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
    closenumview();
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
        int height=GlucoseCurve.getheight(context);
        int width=GlucoseCurve.getwidth(context);
        int hormarg= (int)(width*0.08f);
        getMargins(timebutton).setMarginEnd(hormarg);
       // getMargins(savebutton).setMarginStart(hormarg); 
        getMargins(datebutton).setMarginStart(hormarg); 
//        getMargins(deletebutton).setMarginEnd(hormarg);
        //Log.i(LOG_ID,"addNumberView width="+width+" hormarg="+hormarg);
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
           scroll.addView(layout, new ViewGroup.LayoutParams(MATCH_PARENT, MATCH_PARENT));
           newnumview=scroll;
           }
         else {
           var frame=new FrameLayout(context);
           frame.addView(layout, new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));
           newnumview=frame;
             }
      }
  else { 
   layout=new Layout(context, (lay, w, h) -> {
        int wid=GlucoseCurve.getwidth(context)- systembarRight;
        if(!smallScreen) {
            {if(doLog) {Log.i(LOG_ID,"no smallScreen");};};
            int hei=GlucoseCurve.getheight(context);
               if(GlucoseCurve.isLandscape(context)) {
                int ypos=(int)((hei-h)*.65f);
                int maxy=hei-MainActivity.systembarBottom-h;
                lay.setY(Math.max(MainActivity.systembarTop,Math.min(ypos,maxy)));
                lay.setX(numberLandscapeX(wid,w));
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
	                    lay.setY(Math.max(MainActivity.systembarTop,half-h-af));
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
            final boolean showMenu=Menus.on&&deletebutton.getVisibility()==GONE;
            closenumview();
            if(smallScreen)
                help.hidekeyboard(act);
                
            ((Applic) act.getApplication()). redraw();
              MainActivity.poponback();

             if(showMenu)
                Menus.show(context);
            } 
            //            act.clearonback();
        });

    context.addMyContentView(newnumview,isWearable? new ViewGroup.LayoutParams(MATCH_PARENT, MATCH_PARENT):new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));
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
           closenumview();
           if(!isWearable&&smallScreen)
                help.hidekeyboard(context);
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
        closenumview();
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
        closenumview();
        GlucoseCurve.reopener();
        context.poponback();

                    }
                }) .setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {
            }
        }).show().setCanceledOnTouchOutside(false);
    }

static void setMarginStart(ViewGroup.MarginLayoutParams params,int start) {
        if(MainActivity.rtl) 
            params.rightMargin=start;
        else
            params.leftMargin=start;
    }
static void setMarginEnd(ViewGroup.MarginLayoutParams params,int end) {
        if(MainActivity.rtl) 
            params.leftMargin=end;
        else
            params.rightMargin=end;
    }
private void nodelete() {
    deletebutton.setVisibility(GONE);
    if(isWearable) {
//        Log.i(LOG_ID,"nodelete");
        setMarginStart(getMargins(savebutton),0);

       // getMargins(savebutton).setMarginStart(0); // works only the first time
        }
    }

private void seedelete() {
    deletebutton.setVisibility(VISIBLE);
    if(isWearable) {
 //       Log.i(LOG_ID,"seedelete");
        int width=GlucoseCurve.getwidth();
        int hormarg=useclose?(int)(width*0.05f):(int)(width*0.12f);

//        getMargins(deletebutton).setMarginEnd(hormarg);
        setMarginEnd(getMargins(deletebutton),hormarg);
       // getMargins(savebutton).setMarginStart(hormarg);

        setMarginStart(getMargins(savebutton),hormarg);
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
//        Layout.addSystemMargins(datepicker);
        }

    datepicker.setBackgroundColor( Applic.app.backgroundcolor);
    activity.addMyContentView(datepicker, datparams);
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

private static class ContentTimePicker extends TimePicker {
    ContentTimePicker(Context context) {
        super(context);
        }

    @Override
    protected void onMeasure(int widthMeasureSpec,int heightMeasureSpec) {
        if(!GlucoseCurve.isLandscape(getContext())) {
            super.onMeasure(widthMeasureSpec,heightMeasureSpec);
            return;
            }
        int widthMode=View.MeasureSpec.getMode(widthMeasureSpec);
        if(widthMode==View.MeasureSpec.UNSPECIFIED) {
            super.onMeasure(widthMeasureSpec,heightMeasureSpec);
            return;
            }
        int available=View.MeasureSpec.getSize(widthMeasureSpec);
        int safeWidth=GlucoseCurve.getwidth(getContext())-systembarLeft-systembarRight;
        if(safeWidth>0)
            available=Math.min(available,safeWidth);
        if(available<=0) {
            super.onMeasure(widthMeasureSpec,heightMeasureSpec);
            return;
            }
        super.onMeasure(View.MeasureSpec.makeMeasureSpec(0,View.MeasureSpec.UNSPECIFIED),heightMeasureSpec);
        int contentWidth=getMeasuredWidth();
        if(contentWidth<=0||contentWidth>available)
            contentWidth=available;
        super.onMeasure(View.MeasureSpec.makeMeasureSpec(contentWidth,View.MeasureSpec.EXACTLY),heightMeasureSpec);
        }
    }

private static TimePicker newTimePicker(Context context) {
    return isWearable?new TimePicker(context):new ContentTimePicker(context);
    }

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

    //    Log.i(LOG_ID,"new gettimepicker");
        pick =newTimePicker(activity);
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
       views=new View[][]{new View[]{pick},new View[]{cancel,ok}};
       layparwidth=WRAP_CONTENT;
       };
    pick.setLayoutParams(new ViewGroup.LayoutParams(layparwidth ,ViewGroup.LayoutParams.WRAP_CONTENT ));
//    pick.setLayoutParams(new ViewGroup.LayoutParams(WRAP_CONTENT , ViewGroup.LayoutParams.WRAP_CONTENT));
    Layout layout=new Layout(activity, (lay, w, h)-> {
        activity.hideSystemUI();
        if(!isWearable) {
            int width=GlucoseCurve.getwidth(activity);
            int left=systembarLeft;
            int right=width-systembarRight;
            int xpos=left;
            if(right-left>w)
                xpos=left+(right-left-w)/2;
            else if(right<w)
                xpos=Math.max(0,right-w);
            lay.setX(xpos);
            int maxy=GlucoseCurve.getheight(activity)-MainActivity.systembarBottom-h;
            lay.setY(Math.max(0,Math.min(MainActivity.systembarTop,maxy)));
            }
        return new int[]{w,h};
        }, views);
    layout.setBackgroundColor( Applic.backgroundcolor);
    var params=new FrameLayout.LayoutParams(layparwidth,layparheight,Gravity.TOP|Gravity.LEFT);
   if(isWearable)
          layout.setPaddingRelative(0,(int)(GlucoseCurve.metrics.density*5.0),0,(int)(GlucoseCurve.metrics.density*2.0));
    activity.addMyContentView(layout,  params);
    timepicker=layout;
    }
    else {
        timepicker.requestLayout();
        timepicker.setVisibility(VISIBLE);
        timepicker.bringToFront();
    }

  //    timepicker.setPaddingRelative(systembarLeft,MainActivity.systembarTop, systembarRight,MainActivity.systembarBottom);
  /*
if(!isWearable) {

    if(buttonsunder) 
        timepicker.setPaddingRelative(systembarLeft,MainActivity.systembarTop, systembarRight,MainActivity.systembarBottom);
   else  {
         pick.setPaddingRelative(systembarLeft,MainActivity.systembarTop,0,MainActivity.systembarBottom);
        //buttonlay.setPaddingRelative(0,MainActivity.systembarTop/2, systembarRight,MainActivity.systembarBottom);
        }
     }
*/
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
//       if(doLog) {Log.i(LOG_ID,"bron="+bron+" mealptr="+mealptr);};
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
    private static final int KEYBOARD_GENERIC=0;
    private static final int KEYBOARD_NUMBER=1;
    private static final int KEYBOARD_SEARCH=2;
    int activeKeyboardMode=KEYBOARD_GENERIC;
    int landscapeNumberShift=0;
    int landscapeSearchShift=0;

private int landscapeEditorX(int rightEdge,int viewWidth,int shift) {
    int minleft=systembarLeft+GlucoseCurve.dpToPx(8);
    int mostright=rightEdge-viewWidth;
    if(mostright<=minleft) {
        noroom=true;
        return Math.max(0,mostright);
        }
    int half=systembarLeft+(rightEdge-systembarLeft)/2;
    int af=(half-systembarLeft-viewWidth)/4;
    int xpos=half-viewWidth-af;
    if(xpos<minleft) {
        noroom=true;
        return minleft;
        }
    noroom=false;
    xpos+=shift;
    return Math.min(mostright,xpos);
    }
int numberLandscapeX(int rightEdge,int viewWidth) {
    int shift=activeKeyboardMode==KEYBOARD_NUMBER?landscapeNumberShift:0;
    return landscapeEditorX(rightEdge,viewWidth,shift);
    }
int searchLandscapeX(int screenWidth,int viewWidth) {
    int shift=activeKeyboardMode==KEYBOARD_SEARCH?landscapeSearchShift:0;
    return landscapeEditorX(screenWidth-systembarRight,viewWidth,shift);
    }

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
Layout getkeyboard(Context context,View anchor,int keyboardMode) {

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
        final Editable edit=editfocus.getedit();
        if(edit==null)
            return;
        int start= Selection.getSelectionStart(edit);
        int end=Selection.getSelectionEnd(edit);
        if(start<0||end<0)
            return;
        if(end>start) {
            edit.replace(start, end, "");
            }
        else {
            if(start>0)
                edit.replace(--start, end, "");
            }
        Selection.setSelection(edit, start);
    } );
    tmp[2]=but=new Button(context);
    but.setText(".");
    but.setContentDescription("Decimal point");
    but.setOnClickListener(click);
    Layout layout=new Layout(context, (lay, w, h)->{
            int hei=GlucoseCurve.getheight(context);
            int wid=GlucoseCurve.getwidth(context);
            if(GlucoseCurve.isLandscape(context)) {
              boolean numberEditor=keyboardMode==KEYBOARD_NUMBER;
              boolean searchEditor=keyboardMode==KEYBOARD_SEARCH;
              if(numberEditor) {
                  int ypos=(int)((hei-h)*.65f);
                  int maxy=hei-MainActivity.systembarBottom-h;
                  lay.setY(Math.max(MainActivity.systembarTop,Math.min(ypos,maxy)));
                  }
              else
                  lay.setY((int)((hei-h)*.65f));
              int rightEdge=wid-systembarRight;
              int mostright=rightEdge-w;
              boolean noRoom=numberEditor?noroom:
                      searchEditor&&anchor!=null&&anchor.getMeasuredWidth()>=(rightEdge-systembarLeft)/2;
                if(noRoom) {
                    if(numberEditor)
                        landscapeNumberShift=0;
                    else if(searchEditor)
                        landscapeSearchShift=0;
                    lay.setX(mostright);
                    }
                else {
                    int half=numberEditor?systembarLeft+(rightEdge-systembarLeft)/2:rightEdge/2;
                    int halfWidth=numberEditor?rightEdge-half:half;
                    int bij=(halfWidth-w)/4;
                    int xpos=Math.min(mostright,half+bij);
                    if(numberEditor) {
                        int rightGap=Math.max(0,mostright-xpos);
                        landscapeNumberShift=rightGap/4;
                        xpos+=rightGap/2;
                        if(anchor!=null&&anchor.getMeasuredWidth()>0)
                            anchor.setX(numberLandscapeX(rightEdge,anchor.getMeasuredWidth()));
                        }
                    else if(searchEditor) {
                        int rightGap=Math.max(0,mostright-xpos);
                        landscapeSearchShift=rightGap/4;
                        if(anchor!=null&&anchor.getMeasuredWidth()>0)
                            anchor.setX(searchLandscapeX(wid,anchor.getMeasuredWidth()));
                        }
                    if(!numberEditor) {
                        int rightHalf=systembarLeft+(rightEdge-systembarLeft)/2;
                        int centered=rightHalf+(rightEdge-rightHalf-w)/2;
                        xpos=Math.min(mostright,centered);
                        }
                    lay.setX(xpos);

                }

        //        lay.setX(wid-w);
                }
            else {
                int half=hei/2;
                int bij=(half-h)/4;
                int ypos=half+bij;
                if(anchor!=null&&anchor.getMeasuredHeight()>0)
                    ypos=Math.round(anchor.getY())+anchor.getMeasuredHeight()+GlucoseCurve.dpToPx(8);
                int maxy=hei-MainActivity.systembarBottom-h;
                if(ypos>maxy)
                    ypos=maxy;
                lay.setY(Math.max(MainActivity.systembarTop,ypos));
                int xpos=(wid-w)/2;
                int maxx=wid-systembarRight-w;
                if(xpos>maxx)
                    xpos=maxx;
                lay.setX(Math.max(systembarLeft,xpos));

            }

                    return new int[] {w,h};
        }, views) ;
        
    layout.setBackgroundColor( Applic.backgroundcolor);
    layout.post(layout::requestLayout);
    return layout;
    }

public    void showkeyboard(MainActivity context) {
    showkeyboard(context,newnumview,KEYBOARD_NUMBER);
    }
public    void showkeyboard(MainActivity context,View anchor) {
    showkeyboard(context,anchor,KEYBOARD_GENERIC);
    }
void showsearchkeyboard(MainActivity context,View anchor) {
    showkeyboard(context,anchor,KEYBOARD_SEARCH);
    }
private void showkeyboard(MainActivity context,View anchor,int keyboardMode) {
if(!isWearable) {
    hidekeyboard();
    activeKeyboardMode=keyboardMode;
    keyboard=getkeyboard(context,anchor,keyboardMode);
    context.addMyContentView(keyboard, new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));
    }
    }
public    void hidekeyboard() {
if(!isWearable) {
    if(keyboard!=null) {
        removeContentView(keyboard);
        keyboard=null;
        }
    activeKeyboardMode=KEYBOARD_GENERIC;
    landscapeNumberShift=0;
    landscapeSearchShift=0;
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
        Log.stack(LOG_ID,"avoidSpinnerDropdownFocus",e);
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
