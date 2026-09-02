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
import android.app.Activity;
import androidx.appcompat.app.AlertDialog;
import android.content.DialogInterface;
import android.text.InputType;
import android.util.TypedValue;
import android.view.GestureDetector;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.Space;
import android.widget.Spinner;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.Locale;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import static android.view.View.GONE;
import static android.view.View.INVISIBLE;
import static android.view.View.VISIBLE;
import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static android.widget.LinearLayout.VERTICAL;
import static android.widget.Spinner.MODE_DIALOG;
import static android.widget.Spinner.MODE_DROPDOWN;
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.Applic.usedlocale;
import static tk.glucodata.Log.doLog;
import static tk.glucodata.Natives.getInvertColors;
import static tk.glucodata.Natives.getNumAlarm;
import static tk.glucodata.NumberView.avoidSpinnerDropdownFocus;
import static tk.glucodata.RingTones.EnableControls;
import static tk.glucodata.Specific.useclose;
import static tk.glucodata.help.helplight;
import static tk.glucodata.help.hidekeyboard;
import static tk.glucodata.settings.Settings.editoptions;
import static tk.glucodata.settings.Settings.float2string;
import static tk.glucodata.settings.Settings.getGenSpin;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.util.getbutton;

public class setNumAlarm {
    Layout genlayout=null;
NumAlarmAdapter numadapt;
private final static String LOG_ID="setNumAlarm";
    //static final private String LOG_ID="setNumAlarm";
public static boolean issaved;
/*class ScrollListener extends GestureDetector.SimpleOnGestureListener {
@Override
   public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {
      {if(doLog) {Log.i(LOG_ID,"onScroll dX="+distanceX+" dY="+distanceY);};};
      return false;
      }
@Override
      public boolean onFling (MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {
      {if(doLog) {Log.i(LOG_ID,"onFling volX="+velocityX+"volY="+velocityY);};};
      return false;
      }
};*/
@SuppressLint("ClickableViewAccessibility")
public void mkviews(MainActivity act, View set) {
issaved=false;
{if(doLog) {Log.i(LOG_ID,"mkviews");};};
set.setVisibility(GONE);
if(genlayout==null) {
    Button ok=getbutton(act,R.string.closename);
    Button newone=getbutton(act,R.string.newname);
        Button help=new Button(act);
        help.setText(R.string.helpname);
        Button ring=getbutton(act,isWearable?R.string.ringshort:R.string.ringtonename);
//    recycle.setLayoutParams(new ViewGroup.LayoutParams(  MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
    View[][] views;
    if(isWearable) {
      var list=getbutton(act,R.string.list);
       views=new View[][]{new View[]{list},new View[]{ring,newone},new View[]{ok}};
      genlayout= new Layout(act, (l, w, h) -> { int[] ret={w,h}; return ret; },views);
      numadapt = new NumAlarmAdapter(genlayout); //USE recycle.setAdapter(numadapt);
      list.setOnClickListener(v->{
            var listclose=getbutton(act,R.string.closename);
    	    var recycle = new RecyclerView(act);
            LinearLayoutManager lin = new LinearLayoutManager(act);
            recycle.setLayoutManager(lin);
            var height=GlucoseCurve.getheight();
            recycle.setLayoutParams(new ViewGroup.LayoutParams(  WRAP_CONTENT,height));
            recycle.setPadding(0,0,0,(int)(tk.glucodata.GlucoseCurve.metrics.density*10.0f)); 
          if(!useclose) {
              recycle.setPadding(0,(int)(tk.glucodata.GlucoseCurve.metrics.density*10.0f),0,(int)(tk.glucodata.GlucoseCurve.metrics.density*10.0f));
             listclose.setVisibility(GONE);
                }
         else  {
                  recycle.setPadding(0,0,0,(int)(tk.glucodata.GlucoseCurve.metrics.density*10.0f)); 
              }
          var listlay= new Layout(act, (l, w, h) -> { int[] ret={w,h}; return ret; },new  View[]{listclose},new View[]{recycle});
           listlay.setPadding(0,(int)(tk.glucodata.GlucoseCurve.metrics.density*1.0f),0,0);
           act.addMyContentView(listlay, new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.MATCH_PARENT));
           listlay.setBackgroundColor(Applic.backgroundcolor);
            act.setonback( () -> {
    		         removeContentView(listlay);
                  });

             listclose.setOnClickListener(v3->{ MainActivity.doonback();});
               recycle.setAdapter(numadapt);
               });

       }
    else {
    	RecyclerView recycle = new RecyclerView(act);
    	    recycle.setLayoutParams(new ViewGroup.LayoutParams(  MATCH_PARENT, WRAP_CONTENT));
         recycle.setPaddingRelative((int)(tk.glucodata.GlucoseCurve.metrics.density*15.0f),0,0,0);
            LinearLayoutManager lin = new LinearLayoutManager(act);
            recycle.setLayoutManager(lin);
     views=new View[][]{new View[]{recycle},new View[]{ring,help,newone,ok}};
    genlayout= new Layout(act, (l, w, h) -> {
    	int[] ret={w,h};
    	return ret;
    	},views).portraitLayout(new View[]{recycle},new View[]{ring,help},new View[]{newone,ok});
         numadapt = new NumAlarmAdapter(genlayout); //USE recycle.setAdapter(numadapt);
         recycle.setAdapter(numadapt);
    }
    act.lightBars(!getInvertColors( ));
    ring.setOnClickListener(v->{
    	new tk.glucodata.RingTones(3).mkviews(act,null,genlayout);
    	});
    ok.setOnClickListener(v->{
    	act.doonback();
    	});
    help.setOnClickListener(v->{
    	helplight(R.string.reminders,act);	
    	});
    newone.setOnClickListener(v->{
    	mkitemlayout(act,genlayout);
    	emptyitemlayout();
    	});
    ViewGroup.LayoutParams layparm;
    if(isWearable) {
         final int pad=(int)(tk.glucodata.GlucoseCurve.metrics.density*2.0f);
         genlayout.setPadding(pad,pad,pad,pad);
    	layparm = new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);

    	}
    else {
    	layparm=  new FrameLayout.LayoutParams( WRAP_CONTENT, WRAP_CONTENT, Gravity.BOTTOM| Gravity.CENTER_HORIZONTAL);
        var height=GlucoseCurve.getheight();
        ((FrameLayout.LayoutParams) layparm).bottomMargin=(int)(height*.1f);
    	}

        act.addMyContentView(genlayout,layparm);

        genlayout.setBackgroundColor(Applic.backgroundcolor);
    }
else {
    genlayout.setVisibility(VISIBLE); 
    genlayout.bringToFront();
    
    }
act.setonback( () -> {
    	set.setVisibility(VISIBLE);
    	if(itemlayout!=null) {
    		removeContentView(itemlayout);
    		}
    	removeContentView(genlayout);
    	NumAlarm.handlealarm(act.getApplication());
       act.themeLightBars();
    	});
}


    int alarmpos=-1;
public class NumAlarmHolder extends RecyclerView.ViewHolder {

    public NumAlarmHolder(View view,View ok) {
       super(view);
       view.setOnClickListener(v -> {
            int pos=getAbsoluteAdapterPosition();
            mkitemlayout((MainActivity)v.getContext(),ok);
            fillitemlayout(pos) ;
            alarmpos=pos;
            });

    }

}

public class NumAlarmAdapter extends RecyclerView.Adapter<NumAlarmHolder> {
   final private ArrayList<String> labels;
    final private View ok;
    NumAlarmAdapter(View ok) {
        this.ok=ok;
    labels=Natives.getLabels();
    }
    @NonNull
    @Override
    public NumAlarmHolder onCreateViewHolder(ViewGroup parent, int viewType) {
         var view=new TextView( parent.getContext());
          view.setTransformationMethod(null);
          view.setTextSize(TypedValue.COMPLEX_UNIT_PX, Applic.largefontsize);
          view.setLayoutParams(new ViewGroup.LayoutParams(  MATCH_PARENT, WRAP_CONTENT));
          if(isWearable)
              view.setGravity(Gravity.CENTER);
          else
              view.setGravity(Gravity.LEFT);
           return new NumAlarmHolder(view,ok);
          }

    @Override
    public void onBindViewHolder(final NumAlarmHolder holder, int pos) {
    	TextView text=(TextView)holder.itemView;
    	 Object[] alarmobj=getNumAlarm(pos);
    	 float value=(Float)alarmobj[0];
    	 short[] rest=(short[])alarmobj[1];
    	final short type= rest[3];
    	final String lab=(type<labels.size())?labels.get(type):"UNLABELED";
      if(isWearable)  {
    	      text.setText(String.format(usedlocale,"%s  %s", float2string(value),lab) );
            /*
            if(pos==0)
              text.setPadding(0,(int)(tk.glucodata.GlucoseCurve.metrics.density*10.0f),0,(int)(tk.glucodata.GlucoseCurve.metrics.density*10.0f));
         else
              text.setPadding(0,0,0,0); */
            }
      else {
         short start=rest[0];
         short alarm=rest[1];
         text.setText(String.format(usedlocale,"%s  %s %02d:%02d-%02d:%02d", float2string(value),lab , (start/60), (start%60), (alarm/60), (alarm%60))); 
         }
    	}
        @Override
        public int getItemCount() {
    	return Natives.getNumAlarmCount();

        }

}

static void settime(TextView but,int min) {
    but.setText(String.format(usedlocale,"%02d:%02d",min/60,min%60));
    }
int[] minutes=new int[2];
static Button gettimeview(MainActivity act,int[] minutes,int ind,View[] parent) {
    Button but=new Button(act);
    but.setOnClickListener(
            v->  {
                parent[0].setVisibility(INVISIBLE);
                hidekeyboard(act);
                act.getnumberview().gettimepicker(act,minutes[ind]/60, minutes[ind]%60,
                (hour,min) -> {
                        minutes[ind]=hour*60+min;

                        but.setText(String.format(Locale.US,"%02d:%02d",hour,min));
                   },()-> parent[0].setVisibility(VISIBLE));
         });
    return but;
    }


int labelsel=-1;
ViewGroup itemlayout=null;
EditText value;
Spinner spinner;
Button startbut,alarmbut;
Button Delete;


void dodelete(View parent,int alarmpos) {
    	int nr=Natives.getNumAlarmCount();
    	Natives.delNumAlarm(alarmpos);
    	if(nr>0&&alarmpos<nr) { 
    		{if(doLog) {Log.i(LOG_ID,"alarmpos="+alarmpos+ " nr="+nr+" new nr="+Natives.getNumAlarmCount());};};
    		numadapt.notifyItemRemoved(alarmpos);
    		numadapt.notifyDataSetChanged();
    		}
    	this.alarmpos=-1;
    	itemlayout.setVisibility(GONE); 
      if(!isWearable)
         EnableControls(parent,true);
      MainActivity.poponback();
    	}
private void askdelete( View parent,int alarmpos) {
     Object[] alarmobj=getNumAlarm(alarmpos);
     float flvalue=(Float)alarmobj[0];
     short[] rest=(short[])alarmobj[1];
    short type=rest[3];
    spinner.setSelection(type);
    var value=float2string(flvalue);
    var label=Natives.getLabels().get(type);
    var act=parent.getContext();
        AlertDialog.Builder builder = new AlertDialog.Builder(act);
        builder.setTitle(R.string.deletereminder).
     setMessage(label+" "+value).
        setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
    	 		dodelete(parent,alarmpos);
                    }
                }) .setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {
            }
        }).show().setCanceledOnTouchOutside(false);
    }

void  mkitemlayout(MainActivity act,View parent) {
  if(itemlayout==null) {
        spinner=getGenSpin(act);
//        if(isWearable) spinner.setDropDownVerticalOffset((int)(GlucoseCurve.getheight()*.54));
       LabelAdapter<String> labelspinadapt=new LabelAdapter<String>(act,Natives.getLabels(),1);
       spinner.setAdapter(labelspinadapt);
       spinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
              @Override
              public  void onItemSelected (AdapterView<?> parent, View view, int position, long id) {
                  labelsel=position;
              }
              @Override
              public  void onNothingSelected (AdapterView<?> parent) {
                  labelsel=-1;

              } });
//         spinner.clearAnimation();
         spinner.setOnTouchListener(new View.OnTouchListener() {
             @Override
             public boolean onTouch(View view, MotionEvent motionEvent) {
               hidekeyboard(act);
            return false;
             }
         });
    value=new EditText(act);
    value.setInputType( InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL);
    value.setMinEms(isWearable?1:2);
    value.setImeOptions(editoptions);
    View[] layoutar=new View[1];
    startbut=gettimeview(act,minutes,0,layoutar);
    alarmbut=gettimeview(act,minutes,1,layoutar);
    Delete=getbutton(act,R.string.delete);
    Button Cancel=getbutton(act,R.string.cancel);
    Button Save=getbutton(act,R.string.save);
    View[][] views=null;
    if(isWearable) {
      var space1=new Space(act);
      var space2=new Space(act);
      if(useclose)
    	views=new View[][] {new View[]{space1,startbut,alarmbut,space2},new View[] {spinner,value},new View[]{Cancel,Save},new View[]{Delete}};
      else {
//         var space3=new Space(act);
 //        var space4=new Space(act);
         //views=new View[][] {new View[]{space1,startbut,alarmbut,space2},new View[] {spinner,value},new View[]{space3,Delete,Save,space4}};
         views=new View[][] {new View[]{startbut,alarmbut},new View[] {spinner,value},new View[]{Delete,Save}};
         }
      }
    else
    	views=new View[][] {new View[] {spinner,value},new View[]{startbut,alarmbut},new View[]{Delete,Cancel,Save}};
    itemlayout= new Layout(act, (l, w, h) -> {
    	int[] ret={w,h};
    	return ret;
    	}, views);
  ViewGroup.LayoutParams layparm;
   if(isWearable) {
      if(useclose) {
         var scroll=new ScrollView(act);
         scroll.setFillViewport(true);
         scroll.setSmoothScrollingEnabled(false);
         scroll.setScrollbarFadingEnabled(true);
         scroll.setVerticalScrollBarEnabled(true);
    	 var itempar=  new FrameLayout.LayoutParams( WRAP_CONTENT, WRAP_CONTENT, Gravity.CENTER| Gravity.CENTER_HORIZONTAL);
//         scroll.addView(itemlayout,new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));
         scroll.addView(itemlayout,itempar);
         itemlayout.setPadding(0,(int)(tk.glucodata.GlucoseCurve.metrics.density*15.0f),0,0);
         itemlayout=scroll;
      }
    else {
         var frame =new FrameLayout(act);
         frame.addView(itemlayout,new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));
         itemlayout=frame;
         itemlayout.setBackgroundColor(Applic.backgroundcolor);
         }

        layparm=new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.MATCH_PARENT);
//    	layparm=  new FrameLayout.LayoutParams( WRAP_CONTENT, WRAP_CONTENT, Gravity.TOP| Gravity.CENTER_HORIZONTAL);
//    	layparm=  new FrameLayout.LayoutParams( WRAP_CONTENT, WRAP_CONTENT, Gravity.CENTER| Gravity.CENTER_HORIZONTAL);
      }
    else  {
    	layparm=  new FrameLayout.LayoutParams( WRAP_CONTENT, WRAP_CONTENT, Gravity.TOP| Gravity.CENTER_HORIZONTAL);
        var height=GlucoseCurve.getheight();
        ((FrameLayout.LayoutParams) layparm).topMargin=MainActivity.systembarTop;
        }
    layoutar[0]=itemlayout;
        //itemlayout.setBackgroundColor(Applic.backgroundcolor);
        if(!isWearable)
           itemlayout.setBackgroundResource(R.drawable.dialogbackground);
         else
            itemlayout.setBackgroundColor(Applic.backgroundcolor);
       int pad=(int)(tk.glucodata.GlucoseCurve.metrics.density*4.5);
       itemlayout.setPadding(pad,0,pad,0);
    Cancel.setOnClickListener(v->{ 
      MainActivity.doonback();
//    	genlayout.setVisibility(VISIBLE); 
    	});



    Delete.setOnClickListener(v->{ 
    	if(alarmpos>=0) {
    		askdelete(parent,alarmpos);
    		}

        	hidekeyboard(act);
    	});
    Save.setOnClickListener( v-> {

      issaved=true;
       hidekeyboard((MainActivity)v.getContext());
      if(labelsel<0) {
         Log.e(LOG_ID,"labelsel="+labelsel);
         return;
         }
      float val;
      try {
         val=Float.parseFloat(value.getText().toString());
         }
         catch(Exception e) {
         {if(doLog) {Log.i(LOG_ID,"parsefloat exception "+value.getText().toString());};};
         return;
         };
      if(minutes[0]==minutes[1])
         return;
      if(alarmpos>=0) {
         Natives.delNumAlarm(alarmpos);
         alarmpos=-1;
         }
      
      {if(doLog) {Log.i(LOG_ID,"save "+labelsel+" "+val+" "+tstring(minutes[0])+ " "+tstring(minutes[1]));};};
      Natives.setNumAlarm( labelsel,val,minutes[0],minutes[1]);

      numadapt.notifyDataSetChanged();
      itemlayout.setVisibility(GONE); 
   //    genlayout.setVisibility(VISIBLE); 
   //        parok.setVisibility(VISIBLE);
      if(!isWearable)
            EnableControls(parent,true);
      MainActivity.poponback();
      }

    );
        //act.addMyContentView(itemlayout,isWearable?new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.MATCH_PARENT):new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));


        act.addMyContentView(itemlayout,layparm);
;

    }
else  {
    itemlayout.setVisibility(VISIBLE); 
    itemlayout.bringToFront();
    }
MainActivity.setonback(()-> {
          hidekeyboard(act);
         itemlayout.setVisibility(GONE); 
   //    	parok.setVisibility(VISIBLE);
         if(!isWearable) EnableControls(parent,true);
         });
//    	genlayout.setVisibility(VISIBLE); 
//    parok.setVisibility(INVISIBLE);
   if(!isWearable)
      EnableControls(parent,false);
    }	

String tstring(int min) {
      return String.format(usedlocale,"%02d:%02d",min/60,min%60);
      }
    /*
struct amountalarm {
        float value;
        uint16_t start,alarm,end;
        uint16_t type;
        };
*/
void emptyitemlayout() {
    value.setText("");
    minutes[0]=0;
    minutes[1]=0;
    settime(startbut,0);
    settime(alarmbut,0);
    alarmpos=-1;
    spinner.setSelection(0);
    //Delete.setVisibility(isWearable?GONE:INVISIBLE); 
    Delete.setVisibility(INVISIBLE); 
    }
void fillitemlayout(int pos) {
    Object[] alarmobj=getNumAlarm(pos);
    float flvalue=(Float)alarmobj[0];
    short[] rest=(short[])alarmobj[1];
    short start=rest[0];
    short alarm=rest[1];
    short type=rest[3];
    spinner.setSelection(type);
    value.setText(float2string(flvalue));
    minutes[0]=start;
    minutes[1]=alarm;
    settime(startbut,start);
    settime(alarmbut,alarm);
    Delete.setVisibility(VISIBLE); 
    }
}
