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

import static android.content.pm.ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE;
import static android.content.pm.ActivityInfo.SCREEN_ORIENTATION_REVERSE_LANDSCAPE;
import static android.content.pm.PackageManager.COMPONENT_ENABLED_STATE_DISABLED;
import static android.content.pm.PackageManager.COMPONENT_ENABLED_STATE_ENABLED;
import static android.content.pm.PackageManager.DONT_KILL_APP;
import static android.view.View.GONE;
import static android.view.View.INVISIBLE;
import static android.view.View.VISIBLE;
import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.widget.Spinner.MODE_DIALOG;
import static android.widget.Spinner.MODE_DROPDOWN;
import static androidx.core.os.LocaleListCompat.getEmptyLocaleList;
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.Natives.getRTL;
import static tk.glucodata.NumberView.avoidSpinnerDropdownFocus;
import static tk.glucodata.RingTones.EnableControls;
import static tk.glucodata.help.help;
import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getcheckbox;
import static tk.glucodata.util.getlabel;
import static tk.glucodata.util.getlocale;

import android.app.Application;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.os.Build;
import android.text.InputType;
import android.util.TypedValue;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.HorizontalScrollView;
import android.widget.RadioButton;
import android.widget.ScrollView;
import android.widget.Space;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatDelegate;
import androidx.core.os.LocaleListCompat;
import androidx.core.widget.NestedScrollView;

import java.text.DecimalFormatSymbols;
import java.util.Arrays;
import java.util.List;

import tk.glucodata.Applic;
import tk.glucodata.BuildConfig;
import tk.glucodata.Floating;
import tk.glucodata.GlucoseCurve;
import tk.glucodata.HealthConnection;
import tk.glucodata.LabelAdapter;
import tk.glucodata.Layout;
import tk.glucodata.Libreview;
import tk.glucodata.Log;
import tk.glucodata.MainActivity;
import tk.glucodata.Menus;
import tk.glucodata.Natives;
import tk.glucodata.Notify;
import tk.glucodata.R;
import java.text.DecimalFormat;
import java.util.Locale;

public class Settings  {
private final static boolean RU=true,SPANISH=false;
private final static String LOG_ID="Settings";
MainActivity activity;

/*
public static String    oldfloat2string(Float get) {
    return get.toString();
} */
private static final DecimalFormat df1 = new DecimalFormat("#.#",new DecimalFormatSymbols(Locale.US));
public static String    float2string(Float get) {
    return df1.format(get);
} 
/*
public static String    float2string(Float get) {
      return String.format("%.1f",get);
	}  */
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
	boolean[] issaved={false};
   	mksettings(activity,issaved);

	activity.setonback(() -> {

		hidekeyboard();
		finish();
		if(tk.glucodata.Menus.on)
			tk.glucodata.Menus.show(activity);

		});
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
//	if(editlabel!=null) removeContentView(editlabel) ;
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

//    Button deletelabel;
public static	int editoptions=(isWearable?0:(EditorInfo.IME_FLAG_NO_EXTRACT_UI| EditorInfo.IME_FLAG_NO_FULLSCREEN))| EditorInfo.IME_ACTION_DONE;

 int colorwindowbackground;

static int getbackgroundcolor(Context context) {
    TypedValue typedValue = new TypedValue();
    if (context.getTheme().resolveAttribute(android.R.attr.windowBackground, typedValue, true) && typedValue.type >= TypedValue.TYPE_FIRST_COLOR_INT && typedValue.type <= TypedValue.TYPE_LAST_COLOR_INT) {
        return typedValue.data;
    } else
        return Color.RED;
}

HorizontalScrollView settinglayout=null;
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
    alow.setMinEms(2);
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
	parview.setVisibility(INVISIBLE);
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


	var usealarm=getcheckbox(context, R.string.USE_ALARM, Natives.getUSEALARM());
	/*
	usealarm.setOnCheckedChangeListener(
			 (buttonView,  isChecked) -> {
				Natives.setUSEALARM(isChecked);
			    }
			); */
	final boolean alarmloss= Natives.hasalarmloss();
		CheckBox lossalarm = new CheckBox(context);
		lossalarm.setChecked(alarmloss); //Value
        lossalarm.setText(R.string.lossofsignalalarm);
	Button ringlossalarm=getbutton(context,R.string.ringtonename);
		EditText losswait = new EditText(context);
		losswait.setInputType(InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL);
//    losswait.setImeOptions(editoptions);
		losswait.setImeOptions(editoptions);
		losswait.setMinEms(2);
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
new View[]{isvalue},new View[]{ringisvalue,Cancel},new View[]{usealarm},new View[]{Save}};
//new View[]{isvalue},new View[]{ringisvalue},new View[]{Cancel,Save}, new View[] {toucheverywhere}};
		}
	else {
        	View[] lostrow={lossalarm,losswait,min,ringlossalarm};
		View[] row6={usealarm,isvalue, ringisvalue};
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
		parview.setVisibility(VISIBLE);
		tk.glucodata.help.hidekeyboard(context);
		removeContentView(lay) ;
		});
	Cancel.setOnClickListener(
		v->{
		parview.setVisibility(VISIBLE);
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
                	Applic.argToaster(context,context.getString(R.string.cantsetminutes)+str,Toast.LENGTH_SHORT);
			return;
			}
		}
            boolean haslow=((CheckBox) lowalarm[0]).isChecked();
            boolean hashigh=((CheckBox) highalarm[0]).isChecked();
           Natives.setalarms(str2float(((EditText)lowalarm[1]).getText().toString()),
                    str2float(((EditText)highalarm[1]).getText().toString()),
                     haslow, hashigh, isvalue.isChecked(),hasloss);
	Natives.setUSEALARM(usealarm.isChecked());
	   context.poponback();
		parview.setVisibility(VISIBLE);
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


//static private final List<String> supportedlanguages= Arrays.asList("Language","be","de","en","fr","it","nl","pl","pt","uk","zh");
//  static private final List<String> supportedlanguages= Arrays.asList("Language","be","de","en","fr","it","nl","pl","pt","uk");
static private final List<String> supportedlanguages= RU?Arrays.asList("Language","be","de","en","fr","it","nl","pl","pt","ru","uk","zh"):(SPANISH?Arrays.asList("Language","be","de","en","es","fr","it","nl","pl","pt","uk","zh"):Arrays.asList("Language","be","de","en","fr","it","nl","pl","pt","uk","zh"));

//static private final List<String> supportedlanguages= IWRU?Arrays.asList("Language","be","de","en","es","fr","it","iw","nl","pl","pt","ru","uk"):Arrays.asList("Language","be","de","en","es","fr","it","nl","pl","pt","uk");
static private Spinner languagespinner(MainActivity context, int[] spinpos) {
	var spin=  new Spinner(context,isWearable?MODE_DIALOG: MODE_DROPDOWN);


	spin.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
		@Override
		public  void onItemSelected (AdapterView<?> parent, View view, int position, long id) {
			spinpos[0]=position;
			}
		@Override
		public  void onNothingSelected (AdapterView<?> parent) {

		} });

	avoidSpinnerDropdownFocus(spin);
	supportedlanguages.set(0,context.getString(R.string.languagename));
	spin.setAdapter(new LabelAdapter<String>(context,supportedlanguages,0));
	var locales=AppCompatDelegate.getApplicationLocales();
	int pos;
	if(locales.isEmpty()||(pos=supportedlanguages.indexOf(locales.get(0).getLanguage()))<1)
		pos=0;
	
//	var pos=supportedlanguages.indexOf(getlocale().getLanguage()); if(pos<0) pos=0;
	spin.setSelection(pos);

//	   spin.setPadding(0,0,0,0);
	spinpos[0]=pos;
	return spin;
	}

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

            mmolL.setText(R.string.mmolL);
         mgdl = new RadioButton(context);
        mgdl.setOnClickListener(v-> {
		((Applic) context.getApplication()).setunit(2);
 		mmolL.setChecked(false);
		  recreate(issaved);
        });

        mgdl.setText(R.string.mgdL);
        View[] row0 = isWearable?new View[]{new Space(context),mmolL, mgdl,new Space(context)}:new View[]{unitlabel, mmolL, mgdl};

        TextView graphlabel = new TextView(context);
        graphlabel.setText(R.string.graphrange);
        glow = new EditText(context);
        glow.setInputType(InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL);

        glow.setImeOptions(editoptions);
        glow.setMinEms(2);


        TextView line = new TextView(context);
        line.setText("-");
        ghigh = new EditText(context);

        ghigh.setInputType(InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL);
        ghigh.setImeOptions(editoptions);
        ghigh.setMinEms(2);
        View[] row1 = {graphlabel, glow, line, ghigh};

        TextView targetlabel = new TextView(context);
        targetlabel.setText(R.string.targetrange);
        tlow = new EditText(context);


        tlow.setInputType(InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL);
        tlow.setMinEms(2);
        tlow.setImeOptions(editoptions);
        TextView line2=new TextView(context); line2.setText("-");
        thigh = new EditText(context);

        thigh.setInputType(InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL);
        thigh.setMinEms(2);
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
		finish();
		if(tk.glucodata.Menus.on)
			tk.glucodata.Menus.show(context);

		});
       Button ok =new Button(context);
        ok.setText(R.string.save);
	CheckBox levelleft= new CheckBox(context);
	levelleft.setText(R.string.glucoseaxisleft);
	levelleft.setChecked(Natives.getlevelleft());
	CheckBox bluetooth= new CheckBox(context);
        CheckBox globalscan = new CheckBox(context);
	globalscan.setText(R.string.startsapp);

        final CheckBox librelinkbroadcast=(!isWearable)?new CheckBox(context):null;

        final CheckBox libreview=(!isWearable)?new CheckBox(context):null;
        final CheckBox xdripbroadcast=new CheckBox(context);

        final CheckBox everSensebroadcast=(!isWearable)?new CheckBox(context):null;

        final CheckBox jugglucobroadcast=new CheckBox(context);


	xdripbroadcast.setText(R.string.xdripbroadcast);
	xdripbroadcast.setChecked(Natives.getxbroadcast());
	jugglucobroadcast.setText("Glucodata broadcast");
	jugglucobroadcast.setChecked(Natives.getJugglucobroadcast());


	final var healthconnect=(isWearable||Build.VERSION.SDK_INT <28) ?null:getcheckbox(context, "Health Connect", Natives.gethealthConnect());


	final boolean wasxdrip= isWearable?false:Natives.getuselibreview();

	final boolean usedlibrebroad= isWearable?false:Natives.getlibrelinkused();
	if(!isWearable) {
		libreview.setText(R.string.libreviewname);
		libreview.setChecked(wasxdrip);
		librelinkbroadcast.setText(R.string.sendtoxdrip);

		librelinkbroadcast.setChecked(usedlibrebroad);
		everSensebroadcast.setText(R.string.everSensebroadcast);
		everSensebroadcast.setChecked(Natives.geteverSensebroadcast());

		 if(Build.VERSION.SDK_INT >= 28) {
			healthconnect.setOnCheckedChangeListener( (buttonView,  isChecked) -> {
					Natives.sethealthConnect(isChecked);
					if(isChecked) {
				   		MainActivity.tryHealth = 5;
						HealthConnection.Companion.init(context);
						}
					else {
				   		MainActivity.tryHealth = 0;
						HealthConnection.Companion.stop();
						}
					}
				);
			}
		}
	final var hasnfc=MainActivity.hasnfc;
	if(hasnfc)  {
		scanenabled=IntentscanEnabled();
		globalscan.setChecked(scanenabled) ; //Value
		}
	CheckBox camera=!isWearable?new CheckBox(context):null;
	CheckBox reverseorientation =!isWearable?new CheckBox(context):null;
	final int diskey=!isWearable?Natives.camerakey():0;
	if(!isWearable) {

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
                	Applic.argToaster(context, R.string.sensorviabluetoothon,Toast.LENGTH_LONG);
			}
		alarmsettings(context,settinglayout,issaved);
		});

	int[] spinpos={0};
	var langspin=languagespinner(context,spinpos);
	var startspinpos=spinpos[0];
	Log.i(LOG_ID,"startspinpos="+startspinpos);
/*	final var pastRTL= getRTL();
	var checkRTL=getcheckbox(context, "RTL", pastRTL);
	*/
    ok.setOnClickListener(v->{
		final int wasorient=Natives.getScreenOrientation();
		if(!isWearable) {	
			if( reverseorientation.isChecked()==((wasorient&SCREEN_ORIENTATION_REVERSE_LANDSCAPE)==0)) {
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
			Applic.argToaster(context, R.string.setunitfirst,Toast.LENGTH_SHORT);
			return;
		    }
		    Natives.setsettings(unit,str2float(glow.getText().toString()), str2float(ghigh.getText().toString()), str2float(tlow.getText().toString()), str2float(thigh.getText().toString()));





		    if(blueused!=bluetooth.isChecked()) {
				 activity.setbluetoothmain( !blueused);
				}
	//	   Natives.setwaitwithenablestreaming(waitblue.isChecked());
		  Natives.setfixatex(!fixatex.isChecked());
		  Natives.setfixatey(!fixatey.isChecked());

		if(!bluetooth.isChecked()&&Natives.backuphostNr( )<=0) {
			Applic.argToaster(context, R.string.blueormirror,Toast.LENGTH_LONG);
			}
		  if(!isWearable) {
			  if(diskey > 0) {
				  final int setto = camera.isChecked() ? 1 : 2;
				  if (diskey != setto) Natives.setcamerakey(setto);
			  }
			if(librelinkbroadcast.isChecked()!=usedlibrebroad)  {
				if(!usedlibrebroad) {
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
//		var rtl=checkRTL.isChecked();
		if(tk.glucodata.Menus.on)
			tk.glucodata.Menus.show(context);
		if(spinpos[0]!=startspinpos) {
/*			Log.i(LOG_ID,"setpinpos[0]="+spinpos[0]+" getRTL()=="+pastRTL+" checked="+rtl);
			Natives.setRTL(rtl);  */

			var newlocale=(spinpos[0]==0)?getEmptyLocaleList():LocaleListCompat.forLanguageTags(supportedlanguages.get(spinpos[0]));
			AppCompatDelegate.setApplicationLocales(newlocale);
			}
/*		else {
			Log.i(LOG_ID,"getRTL()=="+pastRTL+" checked="+rtl);
			if(rtl!=pastRTL) {
				Natives.setRTL(rtl); 
				Intent intent = context.getIntent();
				context.finish();
				context.startActivity(intent);
				}
			} */
			});

	Layout[] thelayout=new Layout[1];
	if(!isWearable) {
		changelabels.setText(R.string.numberlabels);
		changelabels.setOnClickListener(v-> {
			    hidekeyboard();
			    new LabelsClass(context).mklabellayout(thelayout[0]);});
			  }
	var colbut=getbutton(context,R.string.colors);
        colbut.setOnClickListener(v-> {
		hidekeyboard();
		finish();
		SetColors.show(context);
		});
	Button numalarm=getbutton(context,R.string.remindersname);
	Button advanced=null;


	View[][] views;
	final String advhelp=isWearable?null:Natives.advanced();
		Button display=isWearable?getbutton(context,context.getString(R.string.display)):null;
	       var fixed=getcheckbox(context,R.string.clampnow,Natives.getcurrentRelative());
		fixed.setOnCheckedChangeListener( (buttonView,  isChecked) -> Natives.setcurrentRelative(isChecked));
	if(isWearable) {

	       var uploader=getbutton(context,R.string.uploader);
	       var floatconfig=getbutton(context,R.string.floatglucoseshort);

	       floatconfig.setOnClickListener(v-> tk.glucodata.FloatingConfig.show(context,thelayout[0]));
		CheckBox floatglucose=new CheckBox(context);
		floatglucose.setText("      " );


		floatglucose.setChecked(Natives.getfloatglucose());
		floatglucose.setOnCheckedChangeListener( (buttonView,  isChecked) -> Floating.setfloatglucose(context,isChecked) ) ;
			View[] rowglu=new View[]{bluetooth};

			View[] camornum=new View[] {alarmbut,numalarm};
			views=new View[][]{new View[]{getlabel(context,R.string.unit)}, row0, row1,new View[]{scalelabel},new View[]{fixatex,fixatey}, row2,new View[]{levelleft},hasnfc?(new View[]{globalscan,nfcsound}):null, new View[]{xdripbroadcast},new View[]{jugglucobroadcast},new View[]{fixed,uploader},new View[]{floatconfig,floatglucose},camornum,rowglu,new View[]{colbut,display},new View[]{langspin},new View[]{cancel,ok},new View[] {getlabel(context,BuildConfig.BUILD_TIME)},new View[]{getlabel(context,BuildConfig.VERSION_NAME)},new View[]{getlabel(context,codestr) }};;
	       uploader.setOnClickListener(v-> tk.glucodata.NightPost.config(context,thelayout[0]));
		}
	else {
		View[] row8;
		View[] row9;
		var about=getbutton(context,R.string.aboutname);
	       about.setOnClickListener(v-> tk.glucodata.GlucoseCurve.doabout(context));
		if(advhelp!=null) {
			advanced=new Button(context);
			advanced.setText(R.string.advanced);
			row9=new View[]{advanced,help,about,cancel,ok};
			}
		else {
			row9=new View[]{help,about,cancel,ok};
			}

//      var oldxdrip=getbutton(context,"send old"); oldxdrip.setOnClickListener(v-> tk.glucodata.Natives.sendxdripold());
        	CheckBox showalways=new CheckBox(context);
		showalways.setText(R.string.glucosestatusbar);
		showalways.setChecked(Natives.getshowalways()) ;
		showalways.setOnCheckedChangeListener( (buttonView,  isChecked) -> Notify.glucosestatus(isChecked) );
	       var webserver=getbutton(context,R.string.webserver);
	       var uploader=getbutton(context,R.string.uploader);
	       var iob=getcheckbox(context,"IOB",Natives.getIOB());
/*		var streamhistory=getcheckbox(context,R.string.streamhistory,Natives.getStreamHistory( ));
		streamhistory.setOnCheckedChangeListener( (buttonView,  isChecked) -> Natives.setStreamHistory(isChecked) );*/
	       var floatconfig=getbutton(context,R.string.floatglucose);


	       floatconfig.setOnClickListener(v-> tk.glucodata.FloatingConfig.show(context,thelayout[0]));
		final View[] librerow=
		 (Build.VERSION.SDK_INT >= 28)? 
			new View[]{showalways,healthconnect,libreview}
			:
			new View[]{showalways,libreview};
		View[] rowglu=new View[]{ bluetooth,floatconfig,alarmbut};
		row8=new View[]{changelabels,langspin,numalarm,colbut};
		views=new View[][]{row0, row1,new View[]{scalelabel,fixatex,fixatey}, row2,new View[]{levelleft,camera,reverseorientation},

		hasnfc?new View[]{nfcsound, globalscan}:null,librerow,new View[] {librelinkbroadcast,everSensebroadcast, xdripbroadcast ,jugglucobroadcast},new View[] {webserver,iob,fixed,uploader }, rowglu,row8,row9};
	       webserver.setOnClickListener(v-> tk.glucodata.Nightscout.show(context,thelayout[0]));
	       uploader.setOnClickListener(v-> tk.glucodata.NightPost.config(context,thelayout[0]));
		iob.setOnCheckedChangeListener( (buttonView,  isChecked) -> {
				if(!Natives.setIOB(isChecked)) {
					iob.setChecked(false);
					EnableControls(thelayout[0],false);
					tk.glucodata.help.help(R.string.IOB,context,l->EnableControls(thelayout[0],true) );
					}
				}
			);
		}

	help.setFocusableInTouchMode(true);
	help.setFocusable(true);
	 help.requestFocus();
	 help.requestFocusFromTouch();

        Layout lay = new Layout(context, (l, w, h) -> {
    		hideSystemUI(); int[] ret={w,h};
		
		return ret;
		},views);
	thelayout[0]=lay;
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
		librelinkbroadcast.setOnCheckedChangeListener(
		    (buttonView,  isChecked) -> {
		    	if(!xdripdonthing[0]) {
					xdripdonthing[0]=true;
					librelinkbroadcast.setChecked(!isChecked);
					Broadcasts.setlibrereceivers(context,lay,librelinkbroadcast,xdripdonthing);
					}
		    });

		final boolean[] everSensenothing = {false};
		everSensebroadcast.setOnCheckedChangeListener( (buttonView,  isChecked) -> {
					if (!everSensenothing[0]) {
						everSensenothing[0] = true;
						everSensebroadcast.setChecked(!isChecked);
						Broadcasts.seteverSensereceivers(context, lay, everSensebroadcast, everSensenothing);
					}
				}
				);
		if(advhelp!=null) {
			advanced.setOnClickListener(v -> {
				EnableControls(thelayout[0],false);
				help(advhelp, (MainActivity) (v.getContext()),l->
					EnableControls(thelayout[0],true)
					
					);
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
	NestedScrollView scroller=new NestedScrollView(context);
	scroller.addView(lay);
	scroller.setSmoothScrollingEnabled(false);
        scroller.setVerticalScrollBarEnabled(false);
	int width=GlucoseCurve.getwidth();
	int heightU=GlucoseCurve.getheight();
	//scroller.setMinimumWidth(width);
	scroller.setFillViewport(true);
	settinglayout= new HorizontalScrollView(context);
	settinglayout.addView(scroller);
	settinglayout.setSmoothScrollingEnabled(false);
      settinglayout.setVerticalScrollBarEnabled(false);
	settinglayout.setHorizontalScrollBarEnabled(false);
	//settinglayout.setMinimumHeight(heightU);
	settinglayout.setFillViewport(true);

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




//ViewGroup labellayout=null;



public static void   removeContentView(View view) {
    ViewGroup parent= (ViewGroup)view.getParent();
    if(parent!=null)
        parent.removeView(view);
    }

//@Override

}
