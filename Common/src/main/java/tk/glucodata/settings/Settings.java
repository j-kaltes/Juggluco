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


package tk.glucodata.settings;

import android.app.Application;
import android.content.ComponentName;
import android.content.Context;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.text.InputType;
import android.util.TypedValue;
import android.view.Display;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.RadioButton;
import android.widget.ScrollView;
import android.widget.Space;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import java.util.ArrayList;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import tk.glucodata.Applic;
import tk.glucodata.BuildConfig;
import tk.glucodata.GlucoseCurve;
import tk.glucodata.LabelAdapter;
import tk.glucodata.Layout;
import tk.glucodata.Libreview;
import tk.glucodata.Log;
import tk.glucodata.MainActivity;
import tk.glucodata.Natives;
import tk.glucodata.Notify;
import tk.glucodata.R;
import tk.glucodata.SensorBluetooth;

import static android.content.pm.ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE;
import static android.content.pm.ActivityInfo.SCREEN_ORIENTATION_REVERSE_LANDSCAPE;
import static android.content.pm.PackageManager.COMPONENT_ENABLED_STATE_DISABLED;
import static android.content.pm.PackageManager.COMPONENT_ENABLED_STATE_ENABLED;
import static android.content.pm.PackageManager.DONT_KILL_APP;
import static android.view.View.GONE;
import static android.view.View.INVISIBLE;
import static android.view.View.VISIBLE;
import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static tk.glucodata.Applic.isRelease;
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.MainActivity.getscreenwidth;
import static tk.glucodata.Natives.setBlueMessage;
import static tk.glucodata.Natives.setuselibreview;
import static tk.glucodata.NumberView.avoidSpinnerDropdownFocus;
import static tk.glucodata.help.help;
import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getcheckbox;
import static tk.glucodata.util.getlabel;

public class Settings  {
private final static String LOG_ID="Settings";
MainActivity activity;
LabelListAdapter    adapt;
boolean garminwatch=Natives.gethasgarmin();;
ArrayList<String > labels;
//            ArrayList<String > labels= new ArrayList<String>( Arrays.asList("NovoRapid","Carbohydrat","Dextro","Levemir","Fietsen","Lopen","Blood"));

public static String    float2string(Float get) {
    return get.toString();
}
boolean IntentscanEnabled() {
	try{
	Application app= activity.getApplication();
	  PackageManager manage = app.getPackageManager();
	ComponentName  scan= new ComponentName(app, "tk.glucodata.glucodata");
	return manage.getComponentEnabledSetting(scan)!=COMPONENT_ENABLED_STATE_DISABLED;
	}
	catch (Throwable e) {

		Log.stack(LOG_ID,e);
	}
	return false;
	}
void EnableIntentScanning(boolean val) {
	try{
	Application app= activity.getApplication();
	  PackageManager manage = app.getPackageManager();
	ComponentName  scan= new ComponentName(app, "tk.glucodata.glucodata");
	int com=val?COMPONENT_ENABLED_STATE_ENABLED:COMPONENT_ENABLED_STATE_DISABLED;
   	manage.setComponentEnabledSetting(scan,com , DONT_KILL_APP);
	}
	catch (Throwable e) {

		Log.stack(LOG_ID,e);
	}
	}
static private Settings thisone=null;
public static void set(MainActivity act) {
	act.showui=true;
    	act.showSystemUI();
	(thisone=new Settings()).makesettings(act);
	}
private void makesettingsin(MainActivity act) {
    	activity=act;

        colorwindowbackground=Applic.backgroundcolor;
	labels=((Applic)act.getApplication()).getlabels();
	boolean[] issaved={false};
   	mksettings(activity,issaved);

	activity.setonback(() -> {
	/*
		if(tk.glucodata.setNumAlarm.issaved||issaved[0]) {
			Log.i(LOG_ID,"onback issaved");
			Applic.sendsettings();
			}
		else
			Log.i(LOG_ID,"onback not saved"); */
		hidekeyboard();
		finish();});
}
private void makesettings(MainActivity act) {
	Applic.app.getHandler().postDelayed( ()->{
		makesettingsin(act);},1);
		
}


void recreate(boolean[] issaved) {
  issaved[0]=true;
// removeContentView(settinglayout);
    layoutweg();
   mksettings(activity,issaved);

   }
void layoutweg() {
/* 	removeContentView(settinglayout);
	settinglayout=null;*/
	}
public static void closeview() {
	if(thisone!=null)
		thisone.finish();
	}
static void hideSystemUI() {
	}
void finish() {
	layoutweg();
	settinglayout.setVisibility(GONE);
	
	try {
		activity.setRequestedOrientation(Natives.getScreenOrientation());
		}
        catch(       Throwable  error) {
		String mess=error!=null?error.getMessage():null;
		if(mess==null) {
			mess="error";
			}
	       Log.e(LOG_ID ,mess);
	   }
	if(editlabel!=null) 
		 removeContentView(editlabel) ;
	removeContentView(settinglayout);
	thisone=null;

	activity.showui=false;
	Applic.app.getHandler().postDelayed( ()->{
				activity.hideSystemUI();
				},1);
	activity.requestRender();
//	Applic.wakemirrors();
//	activity=null;
	}

   Layout editlabel=null;
    EditText label;
EditText labelprec;
EditText labelweight;
//    Button deletelabel;
   int labelpos=-1;
View okparent=null;
public static	int editoptions=(isWearable?0:(EditorInfo.IME_FLAG_NO_EXTRACT_UI| EditorInfo.IME_FLAG_NO_FULLSCREEN))| EditorInfo.IME_ACTION_DONE;
    void mkchangelabel(MainActivity context,View okpar,Runnable onsave) {
    okpar.setVisibility(INVISIBLE);
	okparent=okpar;	
    if(editlabel==null) {
        TextView labeltext = new TextView(context);
        labeltext.setText(R.string.numlabel);
        label = new EditText(context);
        label.setInputType(InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD);

        label.setImeOptions(editoptions);
        label.setMinEms(7);
	View[] precview=null;
        if(garminwatch) {
            TextView prectext = new TextView(context);
            prectext.setText(R.string.roundto);
            labelprec = new EditText(context);
            labelprec.setInputType(InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL);
            labelprec.setMinEms(3);
	     labelprec.setImeOptions(editoptions);
	     precview=new View[]{prectext,labelprec};
            }
        TextView weighttext = new TextView(context);
        weighttext.setText(R.string.weight);
        labelweight = new EditText(context);
        labelweight.setInputType( InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL);
        labelweight.setMinEms(3);
        labelweight.setImeOptions(editoptions);
        Button help=new Button(context);
        help.setText(R.string.helpname);
        help.setOnClickListener(v->{help(R.string.newlabelhelp,(MainActivity)(v.getContext()));});
        Button cancel=new Button(context);
        cancel.setText(R.string.cancel);
        cancel.setOnClickListener(v->{
		context.doonback();
		});
        Button save=new Button(context);
	if(Natives.staticnum()) {
		save.setVisibility(INVISIBLE);
		}
	else {
		save.setText(R.string.save);
		save.setOnClickListener(v-> {
			float pr=garminwatch?edit2float(labelprec):0;
			float wei=edit2float(labelweight);
			String name=label.getText().toString();
			int pos=(labelpos>=0) ? labelpos:labels.size()-1;

			if(!Natives.setlabel(pos,name,pr,wei)) {
				Toast.makeText(context, name+context.getString(R.string.toolarg), Toast.LENGTH_SHORT).show();
				return;
				}
			if(labelpos>=0) {
			    labels.set(pos,name);
			    }
			else {
			    labels.add(pos,name);
			  if(delete!=null) {
				    delete.setVisibility(VISIBLE);
				    }

			    }
			hidekeyboard(); 
		       editlabel.setVisibility(GONE);
			adapt.notifyDataSetChanged();
    			okparent.setVisibility(VISIBLE);
			onsave.run();
			context.poponback();
			} );
		}
	View [][] views= garminwatch?(new View[][]{new View[]{labeltext,label}, precview,  new View[]{weighttext, labelweight} ,new View[]{help,cancel,save}}):(new View[][]{new View[]{labeltext,label},  new View[]{weighttext, labelweight} ,new View[]{help,cancel,save}});

        editlabel = new Layout(context,
                (l, w, h) -> {
    		hideSystemUI();
			var width=getscreenwidth(context);
			if(width>w)
				  l.setX(( width- w) *.6f);
                    l.setY(0);
			return new int[] {w,h};
                }, views);
	      editlabel.setBackgroundResource(R.drawable.dialogbackground);
	   int pad=(int)(tk.glucodata.GlucoseCurve.metrics.density*5.0);
	   editlabel.setPadding(pad,0,pad,0);
        context.addContentView(editlabel, new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));

    }
    else {
        editlabel.setVisibility(View.VISIBLE);
        editlabel.bringToFront();
	}

	context.setonback(() -> {
		hidekeyboard();
		editlabel.setVisibility(GONE);
		okparent.setVisibility(VISIBLE);
		});
}

 int colorwindowbackground;

static int getbackgroundcolor(Context context) {
    TypedValue typedValue = new TypedValue();
    if (context.getTheme().resolveAttribute(android.R.attr.windowBackground, typedValue, true) && typedValue.type >= TypedValue.TYPE_FIRST_COLOR_INT && typedValue.type <= TypedValue.TYPE_LAST_COLOR_INT) {
        return typedValue.data;
    } else
        return Color.RED;
}

static public class LabelListAdapter extends RecyclerView.Adapter<LabelListHolder> {
    ArrayList<String> labels=null;
    Settings settings;
    View ok;
    Runnable onsave;
    LabelListAdapter(ArrayList<String> labels,Settings set,View ok,Runnable onsave) {
        this.labels=labels;
	this.ok=ok;
	this.onsave=onsave;
        settings=set;
    }

    @NonNull
	@Override
    public LabelListHolder onCreateViewHolder(ViewGroup parent, int viewType) {
    	Button view=new Button( parent.getContext());

	view.setTransformationMethod(null);
        view.setTextSize(TypedValue.COMPLEX_UNIT_SP, 24f);
	view.setLayoutParams(new ViewGroup.LayoutParams(  ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));

        return new LabelListHolder(view,settings,ok,onsave);

    }

	@Override
	public void onBindViewHolder(final LabelListHolder holder, int pos) {
		TextView text=(TextView)holder.itemView;
		text.setText(labels.get(pos));
	    }
        @Override
        public int getItemCount() {
                return labels.size()-1;

        }

}
ScrollView settinglayout=null;
    RadioButton mmolL;
    RadioButton mgdl;

static View[] mkalarm(MainActivity context,String label1,boolean show,Float value,int kind) {

       
    CheckBox yeslow = new CheckBox(context);
    yeslow.setText(label1);
    EditText alow = new EditText(context);

    final int minheight= GlucoseCurve.dpToPx(48);
    alow.setMinimumHeight(minheight);
	alow.setImeOptions(editoptions);
    alow.setInputType(InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL);
//    alow.setImeOptions(editoptions);
    alow.setMinEms(3);
    Button ring=getbutton(context,R.string.ringtonename);


    if(show)  {
         yeslow.setChecked(true);
	    }
   else {
            ring.setVisibility(INVISIBLE);
	    alow.setVisibility(INVISIBLE);
	    }

    alow.setText( float2string(value));
yeslow.setOnCheckedChangeListener(
                 (buttonView,  isChecked) -> {
		 if(isChecked) {
            		alow.setVisibility(VISIBLE);
            		ring.setVisibility(VISIBLE);
			}
		else {
            	    ring.setVisibility(INVISIBLE);
		    alow.setVisibility(INVISIBLE);
		    }
    });

    return new View[]{yeslow, alow,ring};
    }
public static float str2float(String str) {
	if(str!=null) {
	     try {
		return Float.parseFloat(str);
                } catch(Exception e) {};
		}
	return 0.0f;
  }
public static float edit2float(EditText ed) {
	return str2float(ed.getText().toString());
	}

void hidekeyboard() {
 tk.glucodata.help.hidekeyboard(activity) ;
 }
boolean scanenabled=true;
//int addindex=-1;

EditText glow, ghigh ,tlow,thigh;
void setvalues() {
        switch(Natives.getunit()) {
            case 1: mmolL.setChecked(true);break;
            case 2: mgdl.setChecked(true);break;
        }

        glow.setText(float2string(Natives.graphlow()));
        ghigh.setText(float2string(Natives.graphhigh()));
        tlow.setText(float2string(Natives.targetlow()));
        thigh.setText(float2string(Natives.targethigh()));
//        alow.setText( float2string(value));
	}


static public void alarmsettings(MainActivity context,View parview,boolean[] issaved) {
	TextView alarmlow,alarmhigh;
        View[] lowalarm= mkalarm(context,context.getString(R.string.lowglucosealarm),Natives.hasalarmlow(),Natives.alarmlow(),0);
        View[] highalarm=mkalarm(context,context.getString(R.string.highglucosealarm),Natives.hasalarmhigh(),Natives.alarmhigh(),1);
	alarmlow=(TextView)lowalarm[1];
	alarmhigh=(TextView)highalarm[1];
	alarmlow.setText( float2string(Natives.alarmlow()));
	alarmhigh.setText( float2string(Natives.alarmhigh()));
        CheckBox isvalue = new CheckBox(context);
	final boolean hasvalue=Natives.hasvaluealarm();
       isvalue.setChecked(hasvalue); //Value
        isvalue.setText(R.string.valueavailablenotification);
	Button ringisvalue=getbutton(context,R.string.ringtonename);
	Button help=getbutton(context,R.string.helpname);
        help.setOnClickListener(v->{help(R.string.alarmhelp,(MainActivity)(v.getContext())); });
       if(!hasvalue) ringisvalue.setVisibility(INVISIBLE);
	isvalue.setOnCheckedChangeListener(
			 (buttonView,  isChecked) -> {
			 if(isChecked) {
				ringisvalue.setVisibility(VISIBLE);
				}
			else {
				ringisvalue.setVisibility(INVISIBLE);
			    }
			});

	final boolean alarmloss= Natives.hasalarmloss();
		CheckBox lossalarm = new CheckBox(context);
		lossalarm.setChecked(alarmloss); //Value
        lossalarm.setText(R.string.lossofsignalalarm);
	Button ringlossalarm=getbutton(context,R.string.ringtonename);
		EditText losswait = new EditText(context);
		losswait.setInputType(InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL);
//    losswait.setImeOptions(editoptions);
		losswait.setImeOptions(editoptions);
		losswait.setMinEms(3);
		int waitloss=Natives.readalarmsuspension(4);
		losswait.setText(waitloss+"");

		var min=getlabel(context,R.string.minutes);
		lossalarm.setOnCheckedChangeListener(
				(buttonView,  isChecked) -> {
					if(isChecked) {
						ringlossalarm.setVisibility(VISIBLE);
						losswait.setVisibility(VISIBLE);
						min.setVisibility(VISIBLE);

					}
					else {
						ringlossalarm.setVisibility(INVISIBLE);
						losswait.setVisibility(INVISIBLE);
						min.setVisibility(INVISIBLE);
					}
				});
	if(!alarmloss) {
		ringlossalarm.setVisibility(INVISIBLE);
		losswait.setVisibility(INVISIBLE);
		min.setVisibility(INVISIBLE);
		}


	var Save=getbutton(context,R.string.save);
	var Cancel=getbutton(context,R.string.cancel);
/*        CheckBox toucheverywhere = new CheckBox(context);
	toucheverywhere.setText(R.string.offtouch);
	toucheverywhere.setChecked(Natives.gettoucheverywhere());
	*/
	View[][] views;
	if(isWearable) {
		var ala=getlabel(context,R.string.alarms);
		final   int pad=(int)(tk.glucodata.GlucoseCurve.metrics.density*9.0);
   		ala.setPadding(pad,pad,pad,pad);
		views=new View[][]{new View[]{ala},new View[]{lowalarm[0]},new View[]{lowalarm[1],lowalarm[2]}, new View[]{highalarm[0]},new View[]{highalarm[1],highalarm[2]},
new View[]{lossalarm},new View[]{losswait,min,ringlossalarm},
new View[]{isvalue},new View[]{ringisvalue,Cancel},new View[]{Save}};
//new View[]{isvalue},new View[]{ringisvalue},new View[]{Cancel,Save}, new View[] {toucheverywhere}};
		}
	else {
        	View[] lostrow={lossalarm,losswait,min,ringlossalarm};
		View[] row6={isvalue, ringisvalue};
//		View[] row7={showalways,new Space(context)};
		View[] rowshow={help,Cancel,Save};
		views=new View[][]{lowalarm,highalarm,lostrow,row6,rowshow};
		}	
	View lay;
        Layout layout = new Layout(context, (l, w, h) -> {
    		hideSystemUI();
		int[] ret={w,h};
		return ret;
		},views);
	var scroll=new ScrollView(context);	
	scroll.addView(layout);
	scroll.setFillViewport(true);
	scroll.setSmoothScrollingEnabled(false);
        scroll.setVerticalScrollBarEnabled(false);
	lay=scroll;
	/*
	if(isWearable) {
		}
	else
		lay=layout; */
        lay.setBackgroundColor(Applic.backgroundcolor);
	context.addContentView(lay, new ViewGroup.LayoutParams( MATCH_PARENT ,MATCH_PARENT));


	lowalarm[2].setOnClickListener(v->{
		new tk.glucodata.RingTones(0).mkviews(context,context.getString(R.string.lowglucosealarm),lay);
		});
	highalarm[2].setOnClickListener(v->{
		new tk.glucodata.RingTones(1).mkviews(context,context.getString(R.string.highglucosealarm),lay);
		});

	context.setonback(() -> {
		tk.glucodata.help.hidekeyboard(context);
		removeContentView(lay) ;
		});
	Cancel.setOnClickListener(
		v->{
	   	context.poponback();
			tk.glucodata.help.hidekeyboard(context);
		removeContentView(lay) ;
		});

    Save.setOnClickListener(v->{
	  final boolean hasloss= lossalarm.isChecked();
	if(hasloss) {
		String str = losswait.getText().toString();
	     try  {
		if(str != null) {
		     short wa = Short.parseShort(str);
		     Natives.writealarmsuspension(4, wa);
		    }
		tk.glucodata.SuperGattCallback.glucosealarms.sensorinit();
		} catch(Throwable e) {
			Log.stack(LOG_ID,"parseShort",e);
                	Toast.makeText(context,context.getString(R.string.cantsetminutes)+str,Toast.LENGTH_SHORT).show();
			return;
			}
		}
            boolean haslow=((CheckBox) lowalarm[0]).isChecked();
            boolean hashigh=((CheckBox) highalarm[0]).isChecked();
           Natives.setalarms(str2float(((EditText)lowalarm[1]).getText().toString()),
                    str2float(((EditText)highalarm[1]).getText().toString()),
                     haslow, hashigh, isvalue.isChecked(),hasloss);

//	Natives.settoucheverywhere(toucheverywhere.isChecked());
	   context.poponback();
		tk.glucodata.help.hidekeyboard(context);
		removeContentView(lay) ;
		issaved[0]=true;
	    });
	ringisvalue.setOnClickListener(v->{
		new tk.glucodata.RingTones(2).mkviews(context,"Value notification", lay);
		});
	ringlossalarm.setOnClickListener(v->{
		new tk.glucodata.RingTones(4).mkviews(context,"Loss of signal",lay);
		});
}


final private static String  codestr=String.valueOf(BuildConfig.VERSION_CODE);


private	void mksettings(MainActivity context,boolean[] issaved) {

    if(settinglayout==null) {
            TextView unitlabel = new TextView(context);
        unitlabel.setText(R.string.unit);
        mmolL = new RadioButton(context);
        mmolL.setOnClickListener(v-> {
		((Applic) context.getApplication()).setunit(1);
                  mgdl.setChecked(false);
		  recreate(issaved);
                });

            mmolL.setText("mmol/L");
         mgdl = new RadioButton(context);
        mgdl.setOnClickListener(v-> {
		((Applic) context.getApplication()).setunit(2);
 		mmolL.setChecked(false);
		  recreate(issaved);
        });

        mgdl.setText("mg/dL");
        View[] row0 = isWearable?new View[]{new Space(context),mmolL, mgdl,new Space(context)}:new View[]{unitlabel, mmolL, mgdl};

        TextView graphlabel = new TextView(context);
        graphlabel.setText(R.string.graphrange);
        glow = new EditText(context);
        glow.setInputType(InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL);

        glow.setImeOptions(editoptions);
        glow.setMinEms(3);


        TextView line = new TextView(context);
        line.setText("-");
        ghigh = new EditText(context);

        ghigh.setInputType(InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL);
        ghigh.setImeOptions(editoptions);
        ghigh.setMinEms(3);
        View[] row1 = {graphlabel, glow, line, ghigh};

        TextView targetlabel = new TextView(context);
        targetlabel.setText(R.string.targetrange);
        tlow = new EditText(context);


        tlow.setInputType(InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL);
        tlow.setMinEms(3);
        tlow.setImeOptions(editoptions);
        TextView line2=new TextView(context); line2.setText("-");
        thigh = new EditText(context);

        thigh.setInputType(InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL);
        thigh.setMinEms(3);
        thigh.setImeOptions(editoptions);
        View[] row2 = {targetlabel, tlow, line2, thigh};
        Button changelabels=new Button(context);
        Button help =new Button(context);
        help.setText(R.string.helpname);
        help.setOnClickListener(v->{help(R.string.settinghelp,(MainActivity)(v.getContext())); });
       Button cancel =new Button(context);
        cancel.setText(R.string.cancel);
	cancel.setOnClickListener(
		v->{
	   	context.poponback();
		    hidekeyboard();
		finish();});
       Button ok =new Button(context);
        ok.setText(R.string.ok);
	CheckBox levelleft= new CheckBox(context);
	levelleft.setText(R.string.glucoseaxisleft);
	levelleft.setChecked(Natives.getlevelleft());
	CheckBox bluetooth= new CheckBox(context);
        CheckBox globalscan = new CheckBox(context);
	globalscan.setText(R.string.startsapp);
//	var oldxdrip=getbutton(context,"Send old");

        final CheckBox xdrip=(!isWearable)?new CheckBox(context):null;
        final CheckBox libreview=(!isWearable)?new CheckBox(context):null;
        final CheckBox xdripbroadcast=new CheckBox(context);
        final CheckBox jugglucobroadcast=new CheckBox(context);
	xdripbroadcast.setText(R.string.xdripbroadcast);
	xdripbroadcast.setChecked(Natives.getxbroadcast());
	jugglucobroadcast.setText("Glucodata broadcast");
	jugglucobroadcast.setChecked(Natives.getJugglucobroadcast());


	final boolean wasxdrip= isWearable?false:Natives.getuselibreview();

	if(!isWearable) {
		libreview.setText(R.string.libreviewname);
		libreview.setChecked(wasxdrip);
		xdrip.setText(R.string.sendtoxdrip);
		xdrip.setChecked(Natives.getlibrelinkused());
//        	oldxdrip.setOnClickListener(v->{Natives.sendxdripold(); });
		}
	final var hasnfc=MainActivity.hasnfc;
	if(hasnfc)  {
		scanenabled=IntentscanEnabled();
		globalscan.setChecked(scanenabled) ; //Value
		}
	CheckBox camera=!isWearable?new CheckBox(context):null;
	CheckBox reverseorientation =!isWearable?new CheckBox(context):null;
	final int diskey=!isWearable?Natives.camerakey():0;
        CheckBox showalways=isWearable?null:new CheckBox(context);
	CheckBox floatglucose=isWearable?null:new CheckBox(context);
	if(!isWearable) {
		floatglucose.setText(R.string.floatglucose);
		floatglucose.setChecked(Natives.getfloatglucose());
		floatglucose.setOnCheckedChangeListener( (buttonView,  isChecked) -> Notify.setfloatglucose(context,isChecked) ) ;
		showalways.setText(R.string.glucosestatusbar);
		showalways.setChecked(Natives.getshowalways()) ;
		showalways.setOnCheckedChangeListener( (buttonView,  isChecked) -> Notify.glucosestatus(isChecked) );

		if(diskey>0)  {
			camera.setText(R.string.disablecamerakey);
			camera.setChecked(diskey==1);
			}
			
		else
			camera.setVisibility(GONE);


		reverseorientation.setText(R.string.invertscreen);
		}
      final  CheckBox nfcsound=hasnfc?new CheckBox(context):null;
	if(hasnfc) {
		nfcsound.setText(R.string.nfcsound);
		nfcsound.setChecked(Natives.nfcsound());
		}

	TextView scalelabel=getlabel(context,R.string.manuallyscale);
	CheckBox fixatex =new CheckBox(context);

	CheckBox fixatey =new CheckBox(context);

	fixatex.setText(R.string.time);
	fixatey.setText(R.string.glucose);
	fixatex.setChecked(!Natives.getfixatex());
	fixatey.setChecked(!Natives.getfixatey());
	if(!isWearable)
		reverseorientation.setChecked((Natives.getScreenOrientation()&SCREEN_ORIENTATION_REVERSE_LANDSCAPE)!=0);
	bluetooth.setText(R.string.sensorviabluetooth);
	final boolean blueused=Natives.getusebluetooth();

	bluetooth.setChecked(blueused);
	var alarmbut=getbutton(context,R.string.alarms);
        alarmbut.setOnClickListener(v->{
		if(!bluetooth.isChecked()&&Natives.backuphostNr( )<=0) {
                	Toast.makeText(context, R.string.sensorviabluetoothon,Toast.LENGTH_LONG).show();
			}
		alarmsettings(context,settinglayout,issaved);
		});
/*	if(!blueused) alarmbut.setEnabled(false);
	bluetooth.setOnCheckedChangeListener(
                 (buttonView,  isChecked) -> {
			if(!isChecked) {
				alarmbut.setEnabled(false);
				}
			else
				alarmbut.setEnabled(true);

		}); */
/*
	var waitblue=new CheckBox(context);
	waitblue.setText("Wait with enabling streaming");
	waitblue.setChecked(Natives.getwaitwithenablestreaming());
	*/
    ok.setOnClickListener(v->{
    /*
    	if( colors.isChecked()!=Natives.getInvertColors()) {
		Natives.setInvertColors(colors.isChecked());
		} */
	final int wasorient=Natives.getScreenOrientation();
	if(!isWearable) {	
		if( reverseorientation.isChecked()==((wasorient&SCREEN_ORIENTATION_REVERSE_LANDSCAPE)==0)) {
		//	int ori= (MainActivity.wearable?1:0)+(reverseorientation.isChecked()?SCREEN_ORIENTATION_REVERSE_LANDSCAPE:SCREEN_ORIENTATION_LANDSCAPE);
			int ori= (reverseorientation.isChecked()?SCREEN_ORIENTATION_REVERSE_LANDSCAPE:SCREEN_ORIENTATION_LANDSCAPE);
			Natives.setScreenOrientation(ori);
			}
		}

	if(hasnfc)  {
		if(scanenabled!=globalscan.isChecked())
		    EnableIntentScanning(!scanenabled);
		    }
            int unit=mmolL.isChecked()?1:(mgdl.isChecked()?2:0);
            if(unit==0) {
                Toast.makeText(context, R.string.setunitfirst,Toast.LENGTH_SHORT).show();
                return;
            }
            Natives.setsettings(unit,str2float(glow.getText().toString()), str2float(ghigh.getText().toString()), str2float(tlow.getText().toString()), str2float(thigh.getText().toString()));





	    if(blueused!=bluetooth.isChecked()) {
		      Applic.setbluetooth(activity,!blueused);
			}
//	   Natives.setwaitwithenablestreaming(waitblue.isChecked());
	  Natives.setfixatex(!fixatex.isChecked());
	  Natives.setfixatey(!fixatey.isChecked());

	  if(!isWearable) {
		  if(diskey > 0) {
			  final int setto = camera.isChecked() ? 1 : 2;
			  if (diskey != setto) Natives.setcamerakey(setto);
		  }
		if(xdrip.isChecked()!=wasxdrip)  {
//			Natives.setxinfuus(xdrip.isChecked());	
			if(!wasxdrip) {
				if(!bluetooth.isChecked()&&Natives.backuphostNr( )<=0) {
					Toast.makeText(context, R.string.blueormirror,Toast.LENGTH_LONG).show();
					}

				final var 	starttime= Natives.laststarttime();
				if(starttime!=0L) {

				 	tk.glucodata.XInfuus.sendSensorActivateBroadcast(context, Natives.lastsensorname(), starttime);
					}
				}
			}
		  }
	 if(hasnfc) {
		var was=Natives.nfcsound();
		boolean moet=nfcsound.isChecked();
		if(moet!=Natives.nfcsound())  {
			Natives.setnfcsound(moet);
			context.setnfc();
			}
		}
	

	Natives.setlevelleft(levelleft.isChecked());
	   context.poponback();
	    hidekeyboard();
	    finish();
	    });
	    /*
	var library=getbutton(context,"New Library");
        library.setOnClickListener(v-> {
		getlibrary.openlibrary(context) ;
		});*/
	if(!isWearable) {
		changelabels.setText(R.string.numberlabels);
		changelabels.setOnClickListener(v-> {
			    hidekeyboard();
				resizer=false;
			    mklabellayout();});
			  }
	var colbut=getbutton(context,R.string.colors);
        colbut.setOnClickListener(v-> {
		context.doonback();
		SetColors.show(context);
		});
	Button numalarm=getbutton(context,R.string.remindersname);
	Button advanced=null;



	View[] rowglu=isWearable?new View[]{bluetooth}:new View[]{ bluetooth,alarmbut};
	View[][] views;
	final String advhelp=isWearable?null:Natives.advanced();
		Button display=isWearable?getbutton(context,context.getString(R.string.display)):null;
	if(isWearable) {

			View[] camornum=new View[] {alarmbut,numalarm};
			views=new View[][]{new View[]{getlabel(context,R.string.unit)}, row0, row1,new View[]{scalelabel},new View[]{fixatex,fixatey}, row2,new View[]{levelleft},hasnfc?(new View[]{globalscan,nfcsound}):null, new View[]{xdripbroadcast},new View[]{jugglucobroadcast},camornum,rowglu,new View[]{colbut,display},new View[]{cancel,ok},new View[] {getlabel(context,BuildConfig.BUILD_TIME)},new View[]{getlabel(context,BuildConfig.VERSION_NAME)},new View[]{getlabel(context,codestr) }};;
		}
	else {
/*		Button cuts=new Button(context);
		cuts.setText(R.string.shutcuts);
		cuts.setOnClickListener(v-> {
			   hidekeyboard();
			new Shortcuts().mkshortlistview(context);
			});
		if(!garminwatch) {
			cuts.setVisibility(INVISIBLE);
			} */
		View[] row8;
		View[] row9;
		if(advhelp!=null) {
			advanced=new Button(context);
			advanced.setText(R.string.advanced);
			row9=new View[]{advanced,help,cancel,ok};
			}
		else {
			row9=new View[]{help,cancel,ok};
			}
		row8=new View[]{changelabels,numalarm,colbut};
		views=new View[][]{row0, row1,new View[]{scalelabel,fixatex,fixatey}, row2,new View[]{levelleft,camera,reverseorientation},
		hasnfc?new View[]{nfcsound, globalscan}:null,

		new View[]{libreview,xdrip,xdripbroadcast},new View[]{ showalways,floatglucose,jugglucobroadcast}, rowglu,row8,row9};
		}

	help.setFocusableInTouchMode(true);
	help.setFocusable(true);
	 help.requestFocus();
	 help.requestFocusFromTouch();

        Layout lay = new Layout(context, (l, w, h) -> {
    		hideSystemUI(); int[] ret={w,h};
		
		return ret;
		},views);
	if(!isWearable) {
		final boolean[] donothing={false};
		libreview.setOnCheckedChangeListener(
		    (buttonView,  isChecked) -> {
		    	if(!donothing[0]) {
					donothing[0]=true;
					libreview.setChecked(!isChecked);
					Libreview.config(context,lay,libreview,donothing);
					}
		    });
		final boolean[] xdripdonthing={false};
		xdrip.setOnCheckedChangeListener(
		    (buttonView,  isChecked) -> {
		    	if(!xdripdonthing[0]) {
					xdripdonthing[0]=true;
					xdrip.setChecked(!isChecked);
					Broadcasts.setlibrereceivers(context,lay,xdrip,xdripdonthing);
					}
		    });

		if(advhelp!=null) {
			advanced.setOnClickListener(v -> {
				help(advhelp, (MainActivity) (v.getContext()));
			});
			}
		}
	
	final boolean[] xbroadnothing = {false};
	xdripbroadcast.setOnCheckedChangeListener( (buttonView,  isChecked) -> {
				if (!xbroadnothing[0]) {
					xbroadnothing[0] = true;
					xdripbroadcast.setChecked(!isChecked);
					Broadcasts.setxdripreceivers(context, lay, xdripbroadcast, xbroadnothing);
				}
			}
			);

	final boolean[] juggluconothing = {false};
	jugglucobroadcast.setOnCheckedChangeListener( (buttonView,  isChecked) -> {
				if (!juggluconothing[0]) {
					juggluconothing[0] = true;
					jugglucobroadcast.setChecked(!isChecked);
					Broadcasts.setglucodatareceivers(context, lay, jugglucobroadcast, juggluconothing);
				}
			}
			);

        lay.setBackgroundColor(colorwindowbackground);
        settinglayout=new ScrollView(context);
	settinglayout.addView(lay);
	settinglayout.setFillViewport(true);
	settinglayout.setSmoothScrollingEnabled(false);
        settinglayout.setVerticalScrollBarEnabled(false);
	final	int laywidth=MATCH_PARENT;
	final   int pad=(int)(tk.glucodata.GlucoseCurve.metrics.density*7.0);
	   lay.setPadding(pad,pad*2,pad,pad);
//		lay.setPadding(pad,pad*2,pad,pad*6);

	if(isWearable) {
		display.setOnClickListener(v -> {
			tk.glucodata.Display.display(context, settinglayout);
		});
		}

	 context.addContentView(settinglayout, new ViewGroup.LayoutParams( laywidth ,MATCH_PARENT));
	numalarm.setOnClickListener(v-> {
		new tk.glucodata.setNumAlarm().mkviews(context,settinglayout);
		});
        }
    else {

        settinglayout.setVisibility(VISIBLE);

	settinglayout.bringToFront();
    }
setvalues();
}



Button delete=null;

boolean resizer=true;
//ViewGroup labellayout=null;
void    mklabellayout() {
	MainActivity context = activity;
//    if(labellayout==null) {
	Button ok = new Button(context);
	ok.setText(R.string.ok);
	RecyclerView recycle = new RecyclerView(context);
	recycle.setHasFixedSize(true);
	LinearLayoutManager lin = new LinearLayoutManager(context);
	recycle.setLayoutManager(lin);
	Button addnew = new Button(context);

        Spinner spinner=new Spinner(context);
	final int minheight= GlucoseCurve.dpToPx(48);
	spinner.setMinimumHeight(minheight);
	avoidSpinnerDropdownFocus(spinner);
	LabelAdapter<String> numspinadapt=new LabelAdapter<String>(context,Natives.getLabels(),1);
        spinner.setAdapter(numspinadapt);
        spinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
		@Override
		public  void onItemSelected (AdapterView<?> parent, View view, int position, long id) {
			Natives.setmealvar((byte)position);
			}
		@Override
		public  void onNothingSelected (AdapterView<?> parent) {

		} });
	final Runnable onsave= ()->  {
				numspinadapt.setarray(Natives.getLabels());
				spinner.setAdapter(numspinadapt);
				spinner.setSelection(Natives.getmealvar());
				};
	adapt = new LabelListAdapter(labels, this,ok,onsave); //USE
	recycle.setAdapter(adapt);

	delete = new Button(context);
	if (Natives.staticnum()) {
		addnew.setVisibility(INVISIBLE);
		delete.setVisibility(INVISIBLE);
	} else {
		addnew.setOnClickListener(v -> {
			mkchangelabel(activity,ok,onsave); //USE
			label.setText("");
			labelpos = -1;
		});
		addnew.setText(R.string.newname);
		delete.setText(R.string.deletelast);
		delete.setOnClickListener(v -> {
			int nr = labels.size() - 2; //USE
			Log.d(LOG_ID, "delete " + nr);
			if (nr >= 0) {
				Natives.setnrlabel(nr);
				labels.remove(nr); //USE
				adapt.notifyDataSetChanged();
				numspinadapt.setarray(Natives.getLabels());
				spinner.setAdapter(numspinadapt);
				spinner.setSelection(Natives.getmealvar());
			}
			if (nr <= 0)
				delete.setVisibility(INVISIBLE);
		});
	if (labels.size() < 2)  //USE
		delete.setVisibility(INVISIBLE);

	}
	TextView menulabel=getlabel(context,context.getString(R.string.meal));
	spinner.clearAnimation();
	spinner.setSelection(Natives.getmealvar());
        Button help=new Button(context);
        help.setOnClickListener(v->{help(R.string.labelhelp,context); });
        help.setText(R.string.helpname);
	Layout butlay=new Layout(context,new View[]{menulabel,spinner},new View[] {delete},new View[]{help},new View[]{addnew},new View[]{ok});
	butlay.setLayoutParams(new ViewGroup.LayoutParams(   ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.MATCH_PARENT));
		recycle.setLayoutParams(new ViewGroup.LayoutParams(   ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.MATCH_PARENT));
    final ViewGroup  labellayout=new Layout(context,(x,w,h)->{
    			hideSystemUI();
			return new int[] {w,h};
	},new View[] {recycle,butlay});

	Runnable closerun=()-> {
		hidekeyboard();
		Applic app=(tk.glucodata.Applic )context.getApplication();
		if(Natives.shouldsendlabels())  {
		    Applic.wakemirrors();
		    app.sendlabels();
		}
		/*
		if(app.curve!=null&&app.curve.search!=null) {
			app.curve.searchspinadap.setarray(Natives.getLabels());
			app.curve.searchspinner.setAdapter(app.curve.searchspinadap);
			} */
		removeContentView(labellayout) ;
		resizer=true;
		};
	context.setonback(closerun);
    ok.setOnClickListener(v->{
	    context.poponback();
    	closerun.run();
    });

        labellayout.setBackgroundColor(colorwindowbackground);
        context.addContentView(labellayout, new ViewGroup.LayoutParams(MATCH_PARENT, MATCH_PARENT));
   /* }
    else {
        labellayout.setVisibility(VISIBLE); 
          labellayout.bringToFront();
	}*/
}
public static void   removeContentView(View view) {
    ViewGroup parent= (ViewGroup)view.getParent();
    if(parent!=null)
        parent.removeView(view);
    }

//@Override

}
