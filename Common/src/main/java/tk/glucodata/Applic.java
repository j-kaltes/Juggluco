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

import static android.graphics.Color.BLACK;
import static android.graphics.Color.RED;
import static android.graphics.Color.BLUE;
import static android.graphics.Color.WHITE;
import static android.net.NetworkCapabilities.TRANSPORT_BLUETOOTH;
import static android.net.NetworkCapabilities.TRANSPORT_WIFI;
import static android.net.NetworkCapabilities.TRANSPORT_WIFI_AWARE;
import static android.provider.Settings.ACTION_REQUEST_IGNORE_BATTERY_OPTIMIZATIONS;
import static android.view.View.INVISIBLE;
import static java.util.Locale.US;
import static tk.glucodata.GlucoseCurve.STEPBACK;
import static tk.glucodata.GlucoseCurve.smallfontsize;
import static tk.glucodata.MessageSender.initwearos;
import static tk.glucodata.SuperGattCallback.endtalk;
import static tk.glucodata.util.getlocale;

import android.Manifest;
import android.app.Activity;
import android.app.Application;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.net.ConnectivityManager;
import android.net.LinkProperties;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkRequest;
import android.net.Uri;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.os.PowerManager;
import android.text.format.DateFormat;
import android.util.TypedValue;
import android.widget.Toast;

import androidx.annotation.Keep;
import androidx.annotation.MainThread;
import androidx.core.content.ContextCompat;

import java.util.ArrayList;
import java.util.Locale;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;

import tk.glucodata.nums.AllData;
import tk.glucodata.nums.numio;
import tk.glucodata.settings.Broadcasts;
//import static tk.glucodata.MessageSender.messagesender;

public class Applic extends Application {

//public static tk.glucodata.MessageSender messagesender=null;
   static boolean Nativesloaded=false;
public static boolean hour24=true;
static public final int TargetSDK=BuildConfig.targetSDK;
static public final boolean isWearable= BuildConfig.isWear==1;
//static public final boolean includeLib= isWearable;
static public final boolean includeLib= true;
static public final boolean isRelease= BuildConfig.isRelease==1;
//static public final boolean isRelease= !BuildConfig.DEBUG;
static final String JUGGLUCOIDENT=isWearable?"juggluco":"jugglucowatch";
public static final Locale usedlocale=US;
//final public static boolean usemeal=Natives.usemeal();
// boolean usebluetooth=true;
final private static String LOG_ID="Applic";
//ArrayList<String> labels = null;
public  ArrayList<String> getlabels() {
        return  Natives.getLabels();
	}
public tk.glucodata.nums.AllData numdata=null;
public GlucoseCurve curve=null;
	public static int unit=0;
public void redraw() {
	var tmpcurve=curve;
	if(tmpcurve!=null)
		tmpcurve.requestRender();
	}

final private Handler mHandler;
private final long uiThreadId;
public Handler getHandler() {
	return mHandler;
	}
static boolean canuseflash=false;
static public void useflash(boolean val) {
	canuseflash=val;
	
	if(val&&tk.glucodata.RingTones.one!=null&&tk.glucodata.RingTones.one.permission!=null)
		tk.glucodata.RingTones.one.permission.setVisibility(INVISIBLE);
	/*
	if(!val&&tk.glucodata.RingTones.one!=null&&tk.glucodata.RingTones.one.flashview!=null)
		tk.glucodata.RingTones.one.flashview.setChecked(false);
		*/
	}
MainActivity getActivity() {
	if(curve!=null)
		return (MainActivity)curve.getContext();
	return null;
	}
static public void Toaster(String mess) {
	RunOnUiThread(()-> { Applic.argToaster(app,mess, Toast.LENGTH_SHORT);}) ;
	}

static public	void argToaster(Context context,int res,int duration) {
	 argToaster(context,context.getString(res), duration);
}
static public	void argToaster(Context context,String message,int duration) {
	Toast.makeText(context,message, duration).show();
	if(!isWearable) {
		if(Natives.speakmessages()) 
			speak(message);
		}
	}
private void RunOnUiThreader(Runnable action) {
	if (Thread.currentThread().getId() != uiThreadId) {
		mHandler.post(action);
	  } else {
		action.run();
		}
	}
static  public void RunOnUiThread(Runnable action) {
 	app.RunOnUiThreader(action);
	}
public static Applic app;
private final IntentFilter mintimefilter ;
@MainThread
public Applic() {
	super();
	android.util.Log.e(LOG_ID,"start tk.glucodata");
	mintimefilter = new IntentFilter();
    	mintimefilter.addAction(Intent.ACTION_TIME_TICK);
       app=this;
        mHandler = new Handler(Looper.getMainLooper());
	uiThreadId=Thread.currentThread().getId();
	if(!isWearable) {
		numdata=new AllData(this);
		}
	android.util.Log.i("Applic","Applic.Applic");

	}
void setnotify(boolean on) {
	Notify.alertwatch=on;
	Log.i(LOG_ID,"setnotify="+on);
	}
public void setunit(int unit)  {
	Natives.setunit(unit);
	Notify.mkunitstr(unit);
	}
public void sendlabels() {
	if(!isWearable) {
		if(Natives.gethasgarmin())
			numdata.sendlabels();
		}
	}
void setcurve(GlucoseCurve curve) {
	this.curve=curve;
	}
//void savestate() { if(blue!=null) blue.savestate(); }
public static String curlang=null;
@Override
public void onConfigurationChanged(Configuration newConfig) {
	super.onConfigurationChanged(newConfig);
   	if(Nativesloaded)  {
		var lang=getlocale().getLanguage();

		var new24 = DateFormat.is24HourFormat(this);
		Log.i(LOG_ID,"Applic.onConfigurationChanged new="+lang+" cur="+curlang+ (Applic.hour24?" 24uur":" 12uur"));
		if(!lang.equals(curlang)) {

			curlang=lang;
			if(SuperGattCallback.dotalk)	
				SuperGattCallback.newtalker(null);
			}
		else  {
			if(Applic.hour24 ==new24)
				return;
			}

		Applic.hour24=new24;
		Natives.setlocale(lang, new24);

		}
} 
@Override
public void onTerminate () {
	super.onTerminate();
	if(!isWearable) {
		if (numdata != null) {
			numdata.onCleared();
		}
	}
}

static final String[] scanpermissions=( (Build.VERSION.SDK_INT >= 31)? (new String[]{Manifest.permission.BLUETOOTH_SCAN, Manifest.permission.BLUETOOTH_ADVERTISE, Manifest.permission.BLUETOOTH_CONNECT}):new String[]{Manifest.permission.ACCESS_COARSE_LOCATION,Manifest.permission.ACCESS_FINE_LOCATION});


static String[] hasPermissions(Context context, String[] perms) {
	var over=new ArrayList<String>();
	Log.i(LOG_ID,"hasPermissions: ");
	for(var p:perms) {
		if(ContextCompat.checkSelfPermission(context, p)!= PackageManager.PERMISSION_GRANTED) {
			over.add(p);
			Log.i(LOG_ID,p+" not granted");	
			}
		else  {
			Log.i(LOG_ID,p+" granted");	
				}
		}
	var uit=new String[over.size()];
	Log.i(LOG_ID,"no perm="+over.size());
	return over.toArray(uit);
	}
static String[] noPermissions(Context context) {
	String[] noperms=  hasPermissions(context, scanpermissions) ;
	if(Log.doLog) {
		Log.i(LOG_ID,"nopermissions:");
		for(var el:noperms) {
			Log.i(LOG_ID,el);
			}
		}
	return noperms;
	}
static boolean mayscan() {
        if (Build.VERSION.SDK_INT >= 23) {
			return Applic.hasPermissions(app, scanpermissions).length==0 ;
		}
	return true;
	}
public static void updateservice(Context context,boolean usebluetooth) {
		if(usebluetooth||Natives.backuphostNr( )>0) {
			if(keeprunning.start(context))
				Log.i(LOG_ID,"updateservice: keeprunning started");
			else
				Log.i(LOG_ID,"updateservice keeprunning not started="+keeprunning.started);

			}
		else
			keeprunning.stop();
		}
private static void dontusebluetooth() {
	Log.i(LOG_ID,"dontusebluetooth()");
	SensorBluetooth.destructor();
	if(Natives.backuphostNr( )<=0) {
		keeprunning.stop();
		}
	}


boolean usingbluetooth=false;
boolean canusebluetooth() {
	return usingbluetooth&&Natives.getusebluetooth();
	}

static	 void explicit(Context context) {
	if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.M) {
		try {
		final PowerManager pm = (PowerManager)context.getSystemService(Activity.POWER_SERVICE);
		var name=context.getPackageName();
		if(pm.isIgnoringBatteryOptimizations(name))
			return;
		 Intent intent = new Intent(ACTION_REQUEST_IGNORE_BATTERY_OPTIMIZATIONS);
		 intent.setData(Uri.parse("package:" + name));
		context.startActivity(intent);
		} catch(Throwable e) {
			Log.stack(LOG_ID,"explicit",e);
			}
		}
	}
void initbluetooth(boolean usebluetooth,Context context,boolean frommain) {
	usingbluetooth=usebluetooth;
	Log.i(LOG_ID,"initbluetooth "+usebluetooth);

	if(!isWearable) {
	     if(Natives.getusegarmin()&&numdata.devices==null)
		      numdata.initIQ(context);
	    }
	 if(usebluetooth)
		SensorBluetooth.start();
	
	if(!keeprunning.started) {
		if(usebluetooth||Natives.backuphostNr( )>0) {
			if(keeprunning.start(context))
				Log.i(LOG_ID,"keeprunning started");
			else
				Log.i(LOG_ID,"keeprunning not started="+keeprunning.started);
			if(frommain)
				((MainActivity)context).askNotify();
			}
		}
	}
static boolean possiblybluetooth(Context context) {
	Log.i(LOG_ID,"possiblybluetooth");
//	boolean useblue=Natives.getusebluetooth()&& hasPermissions(context, Applic.scanpermissions).length==0;
	final boolean useblue=Natives.getusebluetooth();
	((Applic)context.getApplicationContext()).initbluetooth(useblue,context,false);
	return useblue;
	}
public static boolean hasip() {
	String[] ips=Backup.gethostnames();
	return ips.length>0&&ips[ips.length-1]!=null;
	}
//@RequiresApi(api = Build.VERSION_CODES.M)
private static boolean hasonAvailable=false;

public static void sendsettings() {
	  Log.i(LOG_ID,"sendsettings");
	     if(!MessageSender.cansend()) {
               	    Log.i(LOG_ID,"!cansend()");
		    return;
		    }
		var sender=tk.glucodata.MessageSender.getMessageSender();
		if(sender!=null) {
			sender.sendsettings();
			}
		}

static public boolean useWearos() {
	if(isWearable)
		return true;
	else
		return tk.glucodata.Wearos.MessageReceiverEnabled();
	}
private void initialize() {
        if (android.os.Build.VERSION.SDK_INT >=  android.os.Build.VERSION_CODES.LOLLIPOP) {
			ConnectivityManager connectivityManager = (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);
			connectivityManager.registerNetworkCallback((new NetworkRequest.Builder()).build(), new ConnectivityManager.NetworkCallback() {
		@Override
		public void onAvailable(Network network) {
			hasonAvailable=true;
		       Log.i(LOG_ID, "network: onAvailable(" + network+")");
		       if(useWearos()) {
				Natives.networkpresent();
				MessageSender.sendnetinfo();
   		 		Applic.wakemirrors();
				}
		       else {
			  if(hasip()) {
				Natives.networkpresent();
				}
			   }
		
		}
		@Override
		public void onUnavailable () {
			Log.i(LOG_ID,"network: onUnavailable()");
			}
		@Override
		public void onLosing (Network network, int maxMsToLive) {
			Log.i(LOG_ID,"network: OnLosing("+network+","+maxMsToLive+")");
			}
		@Override
		public void onCapabilitiesChanged (Network network, NetworkCapabilities networkCapabilities) {
			var down=networkCapabilities.getLinkDownstreamBandwidthKbps();
			var up=networkCapabilities.getLinkUpstreamBandwidthKbps();
			var wifi=networkCapabilities.hasTransport(TRANSPORT_WIFI);
			var blue=networkCapabilities.hasTransport(TRANSPORT_BLUETOOTH);
			boolean aware= false;
			if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.O) {
				aware = networkCapabilities.hasTransport(TRANSPORT_WIFI_AWARE);
			}
			Log.i(LOG_ID, "network: onCapabilitiesChanged(" + network+"downstream "+down+" Kbps up="+up+" Kbps "+(wifi?"":"no ")+" wifi, "+(blue?"":"no ")+" bluetooth, "+(aware?"":" no ")+"WIFI aware)");
			}
		@Override
		public void onLinkPropertiesChanged (Network network, LinkProperties linkProperties) {
			Log.i(LOG_ID, "network: onLinkPropertiesChanged("+network+", LinkProperties linkProperties) ");
			}
		@Override
		public void onLost(Network network) {
			var sender=tk.glucodata.MessageSender.getMessageSender();
			if(sender!=null)
				sender.nulltimes();
		   Log.i(LOG_ID, "onLost(" + network+")");
//		   if(hasonAvailable) 
			Natives.networkabsent();
		       if(useWearos()) {
   		 	Applic.wakemirrors();
			}
		}
	    });

	    if(hasip()) Natives.networkpresent();
        }
	else
		Natives.networkpresent();
}



public static int initscreenwidth;

static private final BroadcastReceiver minTimeReceiver = new BroadcastReceiver() {
    @Override
    public void onReceive(Context context, Intent intent) {
    	Applic.app.domintime();
        }
};
void domintime() {
	Log.i(LOG_ID,"TICK");
	if (curve != null) {
		if((curve.render.stepresult & STEPBACK) == 0) {
			Log.i(LOG_ID,"requestRender()");
			curve.requestRender();
			if (!isWearable) {
				numdata.sendmessages();
			}
		}
	}
}
    static final ScheduledExecutorService scheduler =Executors.newScheduledThreadPool(1);
	/*
     final Runnable mintimer = () -> {
     	domintime();
      };
     ScheduledFuture<?> mintimeHandle;
    void setmintime() {
        final long minute=1000*60;
        long onderm=currentTimeMillis()%minute;
        long delay = minute-onderm;
        mintimeHandle  = scheduler.scheduleAtFixedRate(mintimer, delay,minute ,TimeUnit.MILLISECONDS);
        }
    void cancelmintime() {
        mintimeHandle.cancel(false);
        } */
void setmintime() {
  registerReceiver(minTimeReceiver, mintimefilter);   
  }
void cancelmintime() {
  	unregisterReceiver(minTimeReceiver);
	}
/*
void startstrictmode() {
	if(BuildConfig.isRelease!=1) {
		StrictMode.setThreadPolicy(new StrictMode.ThreadPolicy.Builder() .detectAll().penaltyLog().build());
		StrictMode.setVmPolicy(new StrictMode.VmPolicy.Builder().detectAll() .penaltyLog() .build());
		}
	}*/
static boolean initproccalled=false;
/*
public static void initwearos(Context app) {
	Log.i(LOG_ID,"before new MessageSender");
	messagesender=new MessageSender(app);
	Log.i(LOG_ID,"before sendnetinfo");
	MessageSender.sendnetinfo();
	}*/
void initproc() {
	if(!initproccalled) {
//		DisplayMetrics metrics= getResources().getDisplayMetrics();
	//	initscreenwidth= metrics.widthPixels;
	//	Log.i("Applic","initproc width="+initscreenwidth);
		if(!numio.setlibrary(this))
			return;
		if(isWearable) {
			if(Natives.getWifi())
				UseWifi.usewifi();
			}
		if(tk.glucodata.Applic.useWearos()) {
			initwearos(this);
			}
		 needsnatives();
		Log.i("Applic","initproc width="+initscreenwidth);
		libre3init.init();
		SuperGattCallback.init(this);
		initialize();
		NumAlarm.handlealarm(this);
		Maintenance.setMaintenancealarm(this);
		initbroadcasts();
		initproccalled=true;
		MessageSender.sendnetinfo();
		Specific.start(this);
		}
	}

public static void setbluetooth(Context activity,boolean on) {
	Log.i(LOG_ID,"setbluetooth(Context activity,"+on+")");
	Natives.setusebluetooth(on);
	if(on)  {
		Applic.app.initbluetooth(on,activity,activity instanceof MainActivity);
		}
	else {
		Applic.dontusebluetooth();
		}
	app.redraw();
	}
public static	int stopprogram=0;
    @Override
    public void onCreate() {
        super.onCreate();
	if(DiskSpace.check(this)) {
		initproc();
		}
	else {	
		android.util.Log.e(LOG_ID,"Stop program");
		stopprogram=1;
		}
	}

//static public int backgroundcolor= BLACK;
static public int backgroundcolor= RED;
void setbackgroundcolor(Context context) {
   if(Build.VERSION.SDK_INT >= 21) {
	    TypedValue typedValue = new TypedValue();
	    if(context.getTheme().resolveAttribute(android.R.attr.windowBackground, typedValue, true) && typedValue.type >= TypedValue.TYPE_FIRST_COLOR_INT && typedValue.type <= TypedValue.TYPE_LAST_COLOR_INT) 
		backgroundcolor= typedValue.data;
		}
     	
    }

static private Runnable updater=null;
static private ArrayList<Runnable> updaters=new ArrayList<Runnable>();
public static void  setscreenupdater(Runnable up) {
	updaters.add(up);	
//	updater=up;
	}
public static void  removescreenupdater(Runnable up) {
	updaters.remove(up);	
//	updater=up;
	}
static void updatescreen() {
	if(app.curve != null)
		app.curve.requestRender();
	for(var el:updaters)
		el.run();
	updaters.clear();
	}
static float headfontsize;

boolean needsnatives() {
       Log.i(LOG_ID,"needsnatives");
	var res=getResources();
        headfontsize = res.getDimension(R.dimen.abc_text_size_display_4_material);
	Notify.glucosesize=Applic.app.headfontsize*.35f;
		


        final float menufontsize = res.getDimension(R.dimen.abc_text_size_menu_material);
        smallfontsize = res.getDimension(R.dimen.abc_text_size_small_material);
        GlucoseCurve.metrics= res.getDisplayMetrics();
	initscreenwidth= GlucoseCurve.metrics.widthPixels;
        Natives.setfontsize(smallfontsize, menufontsize, GlucoseCurve.metrics.density, headfontsize);
	boolean smallsize=((initscreenwidth/smallfontsize)<44.69);
	boolean ret;
	if(smallsize!= NumberView.smallScreen) {
		NumberView.smallScreen=smallsize;
		ret=true;
		}
	else
		ret=false;

		

	Notify.mkpaint();
	return ret;
	}

@Keep
static boolean bluetoothEnabled() {	
	return SensorBluetooth.bluetoothIsEnabled();
	}
static final boolean usewakelock=false;
@Keep
static void doglucose(String SerialNumber, int mgdl, float gl, float rate, int alarm, long timmsec,boolean wasblueoff) {
	var wakelock=	usewakelock?(((PowerManager) app.getSystemService(POWER_SERVICE)).newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "Juggluco::Applic")):null;
	if(wakelock!=null)
		wakelock.acquire();
	if(!wasblueoff) {
		Applic.dontusebluetooth();
		}
	SuperGattCallback.dowithglucose( SerialNumber,  mgdl,  gl, rate,  alarm,  timmsec);
	if(wakelock!=null)
		wakelock.release();
	}
@Keep
static boolean updateDevices() { //Rename to reset
	if(tk.glucodata.SensorBluetooth.blueone!=null) {
		 tk.glucodata.SensorBluetooth.blueone.resetDevices();
		 return true;
		}
	Log.e(LOG_ID,"tk.glucodata.SensorBluetooth.blueone==null");
	return false;
	 }
	 /*
	@Override
	protected void attachBaseContext(Context base) {
		super.attachBaseContext(base);
		MultiDex.install(this);
	} */

public static void wakemirrors() {
	MessageSender.sendwake();
	Natives.wakebackup();
	}

static	void initbroadcasts() {

	if(Natives.getinitVersion()<19) {
		if(Natives.getinitVersion()<14) {
			if(Natives.getinitVersion()<13) {
				Broadcasts.updateall();
				}
			if(Notify.arrowNotify!=null)
				Natives.setfloatingFontsize((int) Notify.glucosesize);
			Natives.setfloatingbackground(WHITE);
			 Natives.setfloatingforeground(BLACK);
			}
		Natives.setinitVersion(19);
		}
	var pos=Natives.getfloatingPos( );
	if(pos!=0) {
		Floating.xview=pos&0xFFFF;
		Floating.yview=pos>>16;
		}

	XInfuus.setlibrenames();
	JugglucoSend.setreceivers();
	SendLikexDrip.setreceivers();
        }

static public  boolean	talkbackrunning=false;
static	void		talkbackon(Context cont) {
	SuperGattCallback.newtalker(cont);
	talkbackrunning=true;

	Natives.settouchtalk(true);
	}

static	void		talkbackoff() {
	talkbackrunning=false;
	}
@Keep
static public void speak(String message) {
	if(!isWearable) {
		var talker=SuperGattCallback.talker;
		if(talker==null) {
			SuperGattCallback.newtalker(null);
			talker=SuperGattCallback.talker;
			}
		talker.speak(message);
		}
	}
}

