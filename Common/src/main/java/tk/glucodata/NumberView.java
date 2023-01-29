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

import android.app.Activity;
import android.app.AlertDialog;
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
import android.widget.HorizontalScrollView;
import android.widget.ListPopupWindow;
import android.widget.PopupWindow;
import android.widget.ScrollView;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.TimePicker;

import java.lang.reflect.Field;
import java.lang.reflect.Method;
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
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.RingTones.EnableControls;
import static tk.glucodata.settings.Settings.editoptions;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.util.getbutton;

public class NumberView {
final private static String LOG_ID="NumberView";
Calendar cal = Calendar.getInstance();
Layout datepicker=null;
TextView dateview;
DatePicker datepick;
View newnumview=null;
Button deletebutton, savebutton ;
long currentnum=0L;
Spinner spinner;
EditText valueedit;
TextView source=null;
Button timebutton,datebutton;
/*
void rotatekey(float deg) {
	keyboard.setRotation(deg);
//	keyboard.bringToFront();
	newnumview.setRotation(deg);

//	newnumview.bringToFront();

	}*/
int labelsel=-1;
void closenumview() {
	if (newnumview != null) { 
		newnumview.setVisibility(GONE);
		hidekeyboard();
	     }
	}

Button mealbutton;
public void  addnumberview(MainActivity activity, long hitptr) {
	if(currentnum!=0L&&currentnum!=numio.newhit) 
			Natives.freehitptr(currentnum);
	  long time= Natives.hittime(hitptr)*1000L;
	  int bron= Natives.gethitindex(hitptr);
	var type=Natives.hittype(hitptr);
	 addnumberview(activity, bron,time,Natives.hitvalue(hitptr),type,-1);
	if(hitptr!=numio.newhit) {
		if(!Natives.staticnum())
			deletebutton.setVisibility(VISIBLE);
		else
			deletebutton.setVisibility(INVISIBLE);
		currentnum = hitptr;
		setmealbutton(type,bron, Natives.hitmeal(hitptr)) ;
		}
	else {
		deletebutton.setVisibility(INVISIBLE);
		setmealbutton(type,bron, 0) ;
		currentnum=0L;
		}
           if(dateview!=null)
	   	thedate=time;

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
public   View addnumberview(MainActivity context,final int bron,final long time,final float value,final int type,final int tmpmealptr) {
    if(newnumview==null) {
        datebutton = new Button(context);
        datebutton.setOnClickListener(
                v -> getdateview(context));
	source=new TextView(context);
        dateview=datebutton;
        timebutton = new Button(context);
        timebutton.setOnClickListener(
                v -> {
                    gettimeview(context);
                });

        timeview=timebutton;
	mealbutton=getbutton(context,R.string.mealname);
	source.setMinWidth(mealbutton.getMinWidth());
	
	mealbutton.setVisibility(GONE);
	if(isWearable) {
		valueedit=geteditwearos(context);
		}
	else  {
		valueedit = geteditview(context,new editfocus());
		}

	valueedit.setMinEms(4);
        View[] row1 = {getspinner(context), valueedit};
        deletebutton = new Button(context);
        deletebutton.setText(R.string.delete);

        Button cancel = new Button(context);
        cancel.setText(R.string.cancel);
        savebutton = new Button(context);
        savebutton.setText(R.string.save);

        Layout layout = isWearable?new Layout(context,new View[] {source},new View[]{datebutton,timebutton} , row1, new View[]{cancel,savebutton},new View[]{deletebutton}):new Layout(context, (lay, w, h) -> {
		int hei=GlucoseCurve.getheight();
		int wid=GlucoseCurve.getwidth();
		   if(wid>hei) {
		   	if(hei>h)
			    lay.setY((hei - h) / 2);
			 if(wid>w) {
			       int half= wid / 2;
			       int af=(half-w)/4;
				    int posx=half - w-af;
				    if(posx<0) {
					posx=0;
					noroom=true;
					}
				     else
					noroom=false;
				    lay.setX(posx);
				  }
			    }
			else {
			if(wid>w)
			    lay.setX((wid - w)/2);
			 if(hei>h) {
			    int half=hei/2;
			    int af=(half-h)/4;
			    lay.setY(half - h-af);
			    }

			}

			return new int[] {w,h}; }, new View[]{datebutton, mealbutton,source,timebutton}, row1,new View[]{deletebutton, cancel, savebutton});


	layout.setBackgroundColor( Applic.backgroundcolor);




        cancel.setOnClickListener(v -> {
			context.doonback();
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
		ScrollView hori=new ScrollView(context);
		hori.setFillViewport(true);
		hori.addView(layout);
		newnumview=hori;

		}
	else  {
		newnumview=layout;
		} 
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
		//    	Natives.closemeal(newmealptr[0]);
			newnumview.setVisibility(GONE);
			if(!isWearable)
				hidekeyboard();
			((Applic) act.getApplication()). redraw();
		      act.poponback();
		    } 
		    //		    act.clearonback();
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
				if(!isWearable) {
					showkeyboard(context);
					editfocus.setedittext(valueedit);
					}
				mealview[0]=null;

				}	);
			}
	}

	};
	mealbutton.setOnClickListener(menucall);
	lasttime=time;
	Date dat = new Date(time);
	datebutton.setText(DateFormat.getDateInstance(DateFormat.DEFAULT).format(dat));
        timebutton.setText(DateFormat.getTimeInstance(DateFormat.SHORT).format(dat));
	if(value< Float.MAX_VALUE)
		valueedit.setText(String.valueOf(value));
	else
		valueedit.setText("");
	spinner.setSelection(type);
    	editfocus.setedittext(valueedit);
    	source.setText( bron==1?"      ":"           \u231A         ");
	source.setTextAlignment( TEXT_ALIGNMENT_CENTER);
	int pads=(int)(GlucoseCurve.metrics.density*10);
        source.setPadding(0,pads,0,pads);
//	source.setPadding(0,0,0,0);
	if(!Natives.staticnum())
		savebutton.setVisibility(VISIBLE);
	else
		savebutton.setVisibility(GONE);
	 context.setonback(() -> {
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

		    GlucoseCurve.reopener();
		   newnumview.setVisibility(GONE);
		   if(!isWearable)
			   hidekeyboard();
		    });

	if(tmpmealptr>=0) {
		timebutton.setTextColor( Color.YELLOW);
		datebutton.setTextColor( Color.YELLOW);
	 	newmealptr[0]=tmpmealptr;
		mealview[0]=null;
		Log.i(LOG_ID,"onClick");
		menucall.onClick(mealbutton);
	//	setmealbutton(type,bron, tmpmealptr) ;

// mealbutton.setOnClickListener(menucall);
		}
	else {
		timebutton.setTextColor(savebutton.getCurrentTextColor());
		datebutton.setTextColor(savebutton.getCurrentTextColor());
	//	int mptr=((currentnum!=0L)?Natives.hitmeal(currentnum):0);
	//	setmealbutton(type,bron, mptr) ;
	}

	return newnumview;
	}
void deletedialog(View v,int[] mealptr) {
	if(currentnum==0L) {
		newnumview.setVisibility(GONE);
		   hidekeyboard();
		 // ((Applic) ((Activity) v.getContext()).getApplication()). redraw();
		 return;
		 }
//	Applic  context= ((Applic) ((Activity) v.getContext()).getApplication());
	MainActivity  context=  ((MainActivity) v.getContext());

	  long time= Natives.hittime(currentnum)*1000L;
	   float value=Natives.hitvalue(currentnum);
	   int type=Natives.hittype(currentnum);
	ArrayList<String> labels= ((Applic)context.getApplication()).getlabels();
	String mess= DateFormat.getDateTimeInstance(DateFormat.DEFAULT,DateFormat.DEFAULT).format(time)+" "+ labels.get(type)+" "+value;
        AlertDialog.Builder builder = new AlertDialog.Builder(context);
        builder.setTitle(R.string.deletequestion).
	 setMessage(mess).
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
		 newnumview.setVisibility(GONE);

		   hidekeyboard();
		GlucoseCurve.reopener();
		context.poponback();

	//	    context.clearonback();
                    }
                }) .setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {
/*			if(currentnum!=0) {
				if(currentnum!=numio.newhit) 
					Natives.freehitptr(currentnum);
				currentnum=0;
				}
		 newnumview.setVisibility(View.GONE);
		 keyboard.setVisibility(View.GONE);*/
            }
        }).show();
    }
public void addnumberwithmenu(MainActivity context,int mealptr) {
	if(currentnum!=0L)  {
		if(currentnum!=numio.newhit) 
			Natives.freehitptr(currentnum);
		currentnum=0L;
		}
	var type=Natives.getmealvar();
	 addnumberview(context,1,currentTimeMillis(),Float.MAX_VALUE,type,mealptr);
	setmealbutton(type,1, 0) ;
        deletebutton.setVisibility(INVISIBLE);
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
	setmealbutton(0,1, 0) ;
	spinner.performClick();
        deletebutton.setVisibility(INVISIBLE);
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
	     catch(Exception e) {};

	if(currentnum!=0&&currentnum!=numio.newhit) {
		long dat=thedate==0L?Natives.hittime(currentnum)*1000L:thedate;
		    if(timeview!=null) {
			cal.setTimeInMillis(dat);
			int minutes = thetime;
			if(minutes>=0) {
				cal.set(Calendar.HOUR_OF_DAY, minutes / 60);
				cal.set(Calendar.MINUTE, minutes % 60);
				cal.set(Calendar.SECOND,0);
				}
			dat= cal.getTimeInMillis();
			}

		Natives.hitchange(currentnum,dat/1000L,val,labelsel,mealptr);
		int index=Natives.gethitindex(currentnum);
		if(!isWearable) {
			tk.glucodata.nums.AllData  alldata=((Applic) activity.getApplication()).numdata;
			alldata.changedback(index);
			}
		Natives.freehitptr(currentnum);
		}

	else {
//		long dat=thedate==0L?currentTimeMillis():thedate;
		long dat=thedate==0L?lasttime:thedate;
//		DateFormat.getDateTimeInstance(DateFormat.DEFAULT,DateFormat.DEFAULT);
		cal.setTimeInMillis(dat);
	    if(timeview!=null) {
		int minutes = thetime;
		if(minutes>=0) {
			cal.set(Calendar.HOUR_OF_DAY, minutes / 60);
			cal.set(Calendar.MINUTE, minutes % 60);
			cal.set(Calendar.SECOND,0);
			}
		}
		 dat= cal.getTimeInMillis();
		final int index=1;
            Natives.saveNum(numio.numptrs[index],dat/1000,val,labelsel,mealptr);
		if(!isWearable) {
		   tk.glucodata.nums.AllData  alldata=((Applic) activity.getApplication()).numdata;
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
		dateview.setText( DateFormat.getDateInstance( DateFormat.DEFAULT) .format(dat)); } ;

Layout getdateview(MainActivity activity) {
	long tim=(thedate==0L)?currentTimeMillis():thedate;
	return getdateviewal(activity,tim,numdater);
	}


Layout getdateviewal(MainActivity activity,long date,Dater erdate) {
Log.i(LOG_ID, "getdateviewal");
    dater=erdate;
    if(datepicker==null) {
	Log.i(LOG_ID, " new");
	datepick =new DatePicker(activity);
//	datepick.setSpinnersShown(true);
	datepick.setCalendarViewShown(false);
        Button cancel=new Button(activity);
        cancel.setText(R.string.cancel);
        cancel.setOnClickListener(vi -> { 
		activity.doonback();
		});
        Button ok=new Button(activity);
        ok.setText(R.string.ok);
        ok.setOnClickListener(vi -> {
		activity.doonback();
              // datepicker.setVisibility(GONE);
		int day=datepick.getDayOfMonth();
		int month=datepick.getMonth();
		int year=datepick.getYear();
		dater.date(year,month,day);

        });
        datepicker=new Layout(activity,
                (lay, w, h)->{
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

			return new int[] {w,h};
                },new View[] {datepick},new View[] {cancel,ok});
               // },new View[] {text},new View[] {cancel,ok});

	datepicker.setBackgroundColor( ((Applic)  activity.getApplication()).backgroundcolor);
        activity.addContentView(datepicker, new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));
    }
    else {
	Log.i(LOG_ID, " old");
        datepicker.setVisibility(VISIBLE);
	datepicker.bringToFront();
	}

cal.setTimeInMillis(date);

datepick.updateDate( cal.get(Calendar.YEAR), cal.get(Calendar.MONTH), cal.get(Calendar.DAY_OF_MONTH));
//	((View)datepicker.getParent()).invalidate();
// datepicker.invalidate(); 
activity.setonback(()->{ 
	datepicker.setVisibility(GONE);
	if(newnumview!=null)
		EnableControls(newnumview,true);
		});

if(newnumview!=null)
	EnableControls(newnumview,false);
return datepicker;
}

Layout timepicker=null;
TextView timeview=null;
TimePicker pick=null;

ObjIntConsumer<Integer> settime=null;
ObjIntConsumer<Integer>  numsettime=(hour,min)-> {
	thetime= hour*60+min;
        timeview.setText(String.format(Locale.US,"%02d:%02d",hour,min ));
	};
void gettimeview(MainActivity activity) {
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
	gettimepicker(activity,h,m,numsettime);
	}
	@SuppressWarnings("deprecation")
void gettimepicker(MainActivity activity,int hourin, int minin, ObjIntConsumer<Integer> timeset) {
   settime=timeset;
    if(timepicker==null) {
        pick =new TimePicker(activity);
        pick.setIs24HourView( android.text.format.DateFormat.is24HourFormat(activity));
        Button cancel=new Button(activity);
        cancel.setText(R.string.cancel);
        cancel.setOnClickListener(vi -> { 
	//	timepicker.setVisibility(INVISIBLE);
		activity.doonback();

		});
        Button ok=new Button(activity);
        ok.setText(R.string.ok);
        ok.setOnClickListener(vi -> {
		activity.doonback();
        //    timepicker.setVisibility(INVISIBLE);
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
	if(isWearable) {
		views=new View[][]{new View[]{pick},new View[]{cancel,ok}};
		}
	else {
		Layout buttonlay=new Layout(activity,new View[] {cancel},new View[]{ok});
		buttonlay.setLayoutParams(new ViewGroup.LayoutParams(   ViewGroup.LayoutParams.WRAP_CONTENT, MATCH_PARENT));
		 views=new View[][] {new View[] {pick,buttonlay}};
	 };
//		buttonlay.setBackgroundColor( RED);
	pick.setLayoutParams(new ViewGroup.LayoutParams( MATCH_PARENT , ViewGroup.LayoutParams.WRAP_CONTENT));
        Layout layout=new Layout(activity,
				(lay, w, h)-> {
					activity.hideSystemUI();
					int wid = GlucoseCurve.getwidth();
					if(w>=wid) {
						lay.setX(0);
						}
					else {
						int x=(wid-w)/2;
						lay.setX(x);
						Log.i(LOG_ID,"screen width="+wid+" w="+w+" x="+x);
						}
					if(isWearable) {
						int height = GlucoseCurve.getheight();
						if(height>h) {
								lay.setY((height-h)/2);
						}

					}
					return new int[]{w, h};
				}, views);
		layout.setBackgroundColor( Applic.backgroundcolor);
//        activity.addContentView(layout, new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));
//	pick.setLayoutParams(new ViewGroup.LayoutParams( ViewGroup.LayoutParams.WRAP_CONTENT , ViewGroup.LayoutParams.WRAP_CONTENT));
        activity.addContentView(layout,  new ViewGroup.LayoutParams(isWearable?WRAP_CONTENT:MATCH_PARENT, isWearable?WRAP_CONTENT:MATCH_PARENT));
//        activity.addContentView(layout,   new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));
        timepicker=layout;
    }
    else {
//	timepicker.invalidate();
	timepicker.requestLayout();
        timepicker.setVisibility(VISIBLE);
	timepicker.bringToFront();
	}
activity.setonback(
		() -> {
			activity.hideSystemUI();
			timepicker.setVisibility(INVISIBLE);
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

void setmealbutton(int labelsel,int bron,int mealptr) {
	   Log.i(LOG_ID,"bron="+bron+" mealptr="+mealptr);
	    if(labelsel==Natives.getmealvar() &&(bron==1|| mealptr>0)) {
		mealbutton.setVisibility(VISIBLE);
		source.setVisibility(GONE);
		}

	else {
		mealbutton.setVisibility(GONE);
		source.setVisibility(VISIBLE);
		}
  
  	}
void setmealbutton(int labelsel,long hitptr) {
	boolean here=(hitptr==0L||hitptr==numio.newhit);
	setmealbutton(labelsel, here?1:Natives.gethitindex(hitptr),here?1:Natives.hitmeal(hitptr));
	}
Spinner getspinner(Activity context) {
if(spinner==null) {
        spinner=new Spinner(context,isWearable?MODE_DIALOG: MODE_DROPDOWN);
      /* spinner=new Spinner(context, MODE_DROPDOWN);
	var height=GlucoseCurve.getheight();
	spinner.setDropDownHorizontalOffset(height/3);
	spinner.setDropDownVerticalOffset(0);*/
//	 mSpinnerSimple.setDropDownWidth();
	// spinner.setDropDownHeight(height);
	final int minheight=GlucoseCurve.dpToPx(48);
	spinner.setMinimumHeight(minheight);
	avoidSpinnerDropdownFocus(spinner);
       numspinadapt=new LabelAdapter<String>(context,Natives.getLabels(),1);
        spinner.setAdapter(numspinadapt);
    spinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
        @Override
        public  void onItemSelected (AdapterView<?> parent, View view, int position, long id) {
		labelsel=position;
		if(!isWearable)
			setmealbutton(position,currentnum);
        	}
        @Override
        public  void onNothingSelected (AdapterView<?> parent) {
            labelsel=-1;

        } });
	spinner.clearAnimation();
    }
return spinner;

}

static EditText geteditview(Context context,View.OnFocusChangeListener focus) {
//static EditText geteditview(Context context) {
	EditText  under=new EditText(context);// under.setText(str);
	under.setInputType( InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL);
	under.setMinEms(2);
	under.setOnFocusChangeListener(focus);
	under.setSelectAllOnFocus(true);
	under.setMinHeight(GlucoseCurve.dpToPx(50));
/*	under.setSelectAllOnFocus(true);
	under.setOnTouchListener(ontouchedit);
	*/

	closekeyboard(under) ;
//	under.setFocusableInTouchMode(true);
//	under.setFocusable(true); under.requestFocus();
//	under.setTextIsSelectable(true);
    	return under;
}

static EditText geteditwearos(Context context) {
	var valedit=new EditText(context);
//	valedit.setMinEms(3);
	valedit.setInputType(InputType.TYPE_CLASS_NUMBER |InputType.TYPE_NUMBER_FLAG_DECIMAL);//| InputType.IME_FLAG_NO_FULLSCREEN);
        valedit.setImeOptions(editoptions);
	return valedit;
	}
/*
static View.OnTouchListener ontouchedit= new View.OnTouchListener() {

        @Override
        public boolean onTouch(View v, MotionEvent event) {
	Log.v(LOG_ID,"ontouchedit");
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
//	Layout layout=new Layout(context,row0,row1,row2,row3);
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
//	but.setText(Build.VERSION.SDK_INT>=22?"⌫":"Del");
	but.setText(Build.VERSION.SDK_INT>=22?"\u232B":"\u2190");
//	but.setText(Build.VERSION.SDK_INT>=22?"\u232B":"\u21e6");
//	but.setText(Build.VERSION.SDK_INT>=22?"\u232B":"\u27f5");

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
		Log.i(LOG_ID,"height="+height+" hei="+hei);
		}
	else {
		Log.i(LOG_ID,"only height="+height);
		hei=height;
		wid=width;
		}*/
	if(wid>hei) {
		lay.setY((hei-h)/2);
		if(noroom)
			lay.setX(wid-w);
		else {
			int half= wid/2;
			int bij=(half-w)/4;
			lay.setX(half+bij);

		}

//		lay.setX(wid-w);
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

public static void avoidSpinnerDropdownFocus(Spinner spinner) {
    try {
        Field listPopupField = Spinner.class.getDeclaredField("mPopup");
        listPopupField.setAccessible(true);
        Object listPopup = listPopupField.get(spinner);
        if (listPopup instanceof ListPopupWindow) {
	/*		Log.i("SPINNER","listpopupwin="+ ((ListPopupWindow) listPopup).getAnimationStyle());
			((ListPopupWindow) listPopup).setAnimationStyle(0);*/
			Field popupField = ListPopupWindow.class.getDeclaredField("mPopup");
            popupField.setAccessible(true);
            Object popup = popupField.get((ListPopupWindow) listPopup);
            if (popup instanceof PopupWindow) { {
		  PopupWindow popupwin=(PopupWindow) popup;
                  popupwin.setFocusable(false);
//		  Log.i("SPINNER","popanim="+popupwin.getAnimationStyle());
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
	    } catch (Exception e) {
		Log.stack(LOG_ID,e);
	    }
    }
}
