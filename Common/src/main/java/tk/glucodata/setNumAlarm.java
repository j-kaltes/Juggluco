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
import android.content.DialogInterface;
import android.text.InputType;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.EditText;
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
import static android.widget.Spinner.MODE_DIALOG;
import static android.widget.Spinner.MODE_DROPDOWN;
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.Applic.usedlocale;
import static tk.glucodata.Natives.getNumAlarm;
import static tk.glucodata.NumberView.avoidSpinnerDropdownFocus;
import static tk.glucodata.help.help;
import static tk.glucodata.help.hidekeyboard;
import static tk.glucodata.settings.Settings.editoptions;
import static tk.glucodata.settings.Settings.float2string;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.util.getbutton;

public class setNumAlarm {
	Layout genlayout=null;
NumAlarmAdapter numadapt;
private final static String LOG_ID="setNumAlarm";
	//static final private String LOG_ID="setNumAlarm";
public static boolean issaved;
public void mkviews(MainActivity act, View set) {
issaved=false;
Log.i(LOG_ID,"mkviews");
set.setVisibility(GONE);
if(genlayout==null) {
	RecyclerView recycle = new RecyclerView(act);
	LinearLayoutManager lin = new LinearLayoutManager(act);
	recycle.setLayoutManager(lin);
	Button ok=getbutton(act,R.string.closename);
	numadapt = new NumAlarmAdapter(ok); //USE
	recycle.setAdapter(numadapt);
	Button newone=getbutton(act,R.string.newname);
        Button help=new Button(act);
        help.setText(R.string.helpname);
    	Button ring=getbutton(act,isWearable?R.string.ringshort:R.string.ringtonename);
	ring.setOnClickListener(v->{
		new tk.glucodata.RingTones(3).mkviews(act,null,null);
		});
//	recycle.setLayoutParams(new ViewGroup.LayoutParams(  MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
	recycle.setLayoutParams(new ViewGroup.LayoutParams(  MATCH_PARENT, isWearable?MATCH_PARENT:ViewGroup.LayoutParams.WRAP_CONTENT));
	View[][] views;
	if(isWearable) {
	   ring.setPadding(0,0,0,0);
	   newone.setPadding(0,0,0,0);
	   ok.setPadding(0,0,0,0);
	var width=GlucoseCurve.getwidth();
		var height=GlucoseCurve.getheight();
	   int butwidth=(int)(width*0.33);
	   int butheight=(int)(height*0.17);
//	   var butwidth=0;
//	   var butheight=0;
	   ring.setMinHeight(butheight);
	   ring.setMinWidth(butwidth);
	   ok.setMinHeight(butheight);
	   ok.setMinWidth(butwidth);
	   newone.setMinHeight(butheight);
	   newone.setMinWidth(butwidth);

	   ring.setMinimumHeight(butheight);
	   ring.setMinimumWidth(butwidth);
	   ok.setMinimumHeight(butheight);
	   ok.setMinimumWidth(butwidth);
	   newone.setMinimumHeight(butheight);
	   newone.setMinimumWidth(butwidth);
		views=new View[][]{new View[]{recycle},new View[]{ring,newone},new View[]{ok}};

	   }
	else
	 views=new View[][]{new View[]{recycle},new View[]{ring,help,newone,ok}};
	genlayout= new Layout(act, (l, w, h) -> {
		if(!isWearable) {
			var height=GlucoseCurve.getheight();
			if(height>h)
				l.setY(height*.9f-h);
			var width=GlucoseCurve.getwidth();
			if(width>w)
				l.setX((width-w)/2);
			}
		int[] ret={w,h};
		return ret;
		},views);
	ok.setOnClickListener(v->{ 
		act.doonback();
		});
	help.setOnClickListener(v->{
		help(R.string.reminders,act);	
		});
	newone.setOnClickListener(v->{
//		genlayout.setVisibility(GONE);
		mkitemlayout(act,ok);
		emptyitemlayout();
		});
//        act.addContentView(genlayout, new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.MATCH_PARENT));
	ViewGroup.LayoutParams layparm;
	if(isWearable) {
		layparm = new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
		}
	else {
		layparm=new ViewGroup.LayoutParams( ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
		}

        act.addContentView(genlayout,layparm);
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
		});
}


	int alarmpos=-1;
public class NumAlarmHolder extends RecyclerView.ViewHolder {

    public NumAlarmHolder(View view,View ok) {
        super(view);
       view.setOnClickListener(v -> {
//		genlayout.setVisibility(GONE);
		int pos=getAbsoluteAdapterPosition();
		mkitemlayout((Activity)v.getContext(),ok);
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
    	Button view=new Button( parent.getContext());
	view.setTransformationMethod(null);
        view.setTextSize(TypedValue.COMPLEX_UNIT_SP, 24f);
	view.setLayoutParams(new ViewGroup.LayoutParams(  MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
       view.setGravity(Gravity.LEFT);
        return new NumAlarmHolder(view,ok);

    }

	@Override
	public void onBindViewHolder(final NumAlarmHolder holder, int pos) {
		TextView text=(TextView)holder.itemView;
		 Object[] alarmobj=getNumAlarm(pos);
		 float value=(Float)alarmobj[0];
		 short[] rest=(short[])alarmobj[1];
		short start=rest[0];
		short alarm=rest[1];
		final short type= rest[3];
		final String lab=(type<labels.size())?labels.get(type):"UNLABELED";
		text.setText(String.format(usedlocale,"%s  %s %02d:%02d-%02d:%02d", float2string(value),lab , (start/60), (start%60), (alarm/60), (alarm%60))); 
		}
        @Override
        public int getItemCount() {
		return Natives.getNumAlarmCount();

        }

}

void settime(TextView but,int min) {
	but.setText(String.format(usedlocale,"%02d:%02d",min/60,min%60));
	}
int[] minutes=new int[2];
Button gettimeview(Activity act,int ind) {
	Button but=new Button(act);
        but.setOnClickListener(
                v->  {
    			hidekeyboard(act);
    			MainActivity main=(MainActivity) act;
			main.getnumberview().gettimepicker(main,minutes[ind]/60, minutes[ind]%60,
			(hour,min) -> {
				minutes[ind]=hour*60+min;

				but.setText(String.format(Locale.US,"%02d:%02d",hour,min));
			   }); 
		});
	return but;
	}


int labelsel=-1;
ViewGroup itemlayout=null;
EditText value;
Spinner spinner;
Button startbut,alarmbut;
Button Delete;


void dodelete(View parok,int alarmpos) {
		int nr=Natives.getNumAlarmCount();
		Natives.delNumAlarm(alarmpos);
		if(nr>0&&alarmpos<nr) { 
			Log.i(LOG_ID,"alarmpos="+alarmpos+ " nr="+nr+" new nr="+Natives.getNumAlarmCount());
			numadapt.notifyItemRemoved(alarmpos);
			numadapt.notifyDataSetChanged();
			}
		alarmpos=-1;
		itemlayout.setVisibility(GONE); 
	    parok.setVisibility(VISIBLE);
		}
private void askdelete( View parok,int alarmpos) {
	 Object[] alarmobj=getNumAlarm(alarmpos);
	 float flvalue=(Float)alarmobj[0];
	 short[] rest=(short[])alarmobj[1];
	short type=rest[3];
	spinner.setSelection(type);
	var value=float2string(flvalue);
	var label=Natives.getLabels().get(type);
	var act=parok.getContext();
        AlertDialog.Builder builder = new AlertDialog.Builder(act);
        builder.setTitle(R.string.deletereminder).
	 setMessage(label+" "+value).
        setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
		 		dodelete(parok,alarmpos);
                    }
                }) .setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {
            }
        }).show();
	}

void  mkitemlayout(Activity act,View parok) {
  if(itemlayout==null) {
	//spinner=new Spinner(act);
        spinner=new Spinner(act,isWearable?MODE_DIALOG: MODE_DROPDOWN);
	avoidSpinnerDropdownFocus(spinner);
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
	spinner.clearAnimation();
	spinner.setOnTouchListener(new View.OnTouchListener() {
		    @Override
		    public boolean onTouch(View view, MotionEvent motionEvent) {
    			hidekeyboard(act);
			return false;
		    }
		});
	value=new EditText(act);
	value.setInputType( InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL);
	value.setMinEms(2);
	value.setImeOptions(editoptions);

	startbut=gettimeview(act,0);
	alarmbut=gettimeview(act,1);
	Delete=getbutton(act,R.string.delete);
	Button Cancel=getbutton(act,R.string.cancel);
	Button Save=getbutton(act,R.string.save);
	View[][] views=null;
	if(isWearable)
		views=new View[][] {new View[]{startbut,alarmbut},new View[] {spinner,value},new View[]{Cancel,Save},new View[]{Delete}};
	else
		views=new View[][] {new View[] {spinner,value},new View[]{startbut,alarmbut},new View[]{Delete,Cancel,Save}};
	itemlayout= new Layout(act, (l, w, h) -> {
//		l.setY((GlucoseCurve.getheight()-h)/2);
		var height=GlucoseCurve.getheight();
		if(isWearable&&height>h) {
			l.setY((height-h)/2);
		}else
			l.setY(0);
		var width=GlucoseCurve.getwidth();
		if(width>w)
			l.setX((width-w)/2);
		int[] ret={w,h};
		return ret;
		}, views);

        //itemlayout.setBackgroundColor(Applic.backgroundcolor);
        itemlayout.setBackgroundResource(R.drawable.dialogbackground);
	   int pad=(int)(tk.glucodata.GlucoseCurve.metrics.density*4.5);
	   itemlayout.setPadding(pad,0,pad,0);
	Cancel.setOnClickListener(v->{ 
    		hidekeyboard(act);
		itemlayout.setVisibility(GONE); 
		parok.setVisibility(VISIBLE);
//		genlayout.setVisibility(VISIBLE); 
		});



	Delete.setOnClickListener(v->{ 
		if(alarmpos>=0) {
			askdelete(parok,alarmpos);
			}

    		hidekeyboard(act);
		});
	Save.setOnClickListener(

 v-> {
   issaved=true;
    hidekeyboard((Activity)v.getContext());
	if(labelsel<0) {
		Log.e(LOG_ID,"labelsel="+labelsel);
		return;
		}
	float val;
	try {
 	   val=Float.parseFloat(value.getText().toString());
		}
		catch(Exception e) {
		Log.i(LOG_ID,"parsefloat exception "+value.getText().toString());
		return;
		};
	if(minutes[0]==minutes[1])
		return;
	if(alarmpos>=0) {
		Natives.delNumAlarm(alarmpos);
		alarmpos=-1;
		}
	
	Log.i(LOG_ID,"save "+labelsel+" "+val+" "+tstring(minutes[0])+ " "+tstring(minutes[1]));
	Natives.setNumAlarm( labelsel,val,minutes[0],minutes[1]);

	numadapt.notifyDataSetChanged();
	itemlayout.setVisibility(GONE); 
//	genlayout.setVisibility(VISIBLE); 
	    parok.setVisibility(VISIBLE);
	}


	);
        act.addContentView(itemlayout, new ViewGroup.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT,ViewGroup.LayoutParams.WRAP_CONTENT));
	}
else  {
	itemlayout.setVisibility(VISIBLE); 
	itemlayout.bringToFront();
	}

parok.setVisibility(INVISIBLE);
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
	Delete.setVisibility(isWearable?GONE:INVISIBLE); 
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
