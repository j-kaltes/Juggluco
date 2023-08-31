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


import static android.provider.Settings.ACTION_NOTIFICATION_POLICY_ACCESS_SETTINGS;
import static android.provider.Settings.ACTION_REQUEST_SCHEDULE_EXACT_ALARM;
import static android.view.View.GONE;
import static tk.glucodata.Applic.TargetSDK;
import static tk.glucodata.Applic.app;
import static tk.glucodata.Applic.explicit;
import static tk.glucodata.Applic.isRelease;
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.Applic.scanpermissions;
import static tk.glucodata.Applic.useflash;
import static tk.glucodata.Floating.setfloatglucose;
import static tk.glucodata.Floating.shoulduseadb;
import static tk.glucodata.GlucoseCurve.STEPBACK;
import static tk.glucodata.Natives.hasstreamed;
import static tk.glucodata.Log.showbytes;
import static tk.glucodata.Natives.wakelibreview;
import static tk.glucodata.help.hidekeyboard;
import static tk.glucodata.settings.Settings.removeContentView;

import android.Manifest;
import android.app.Activity;
import android.app.ActivityManager;
import android.app.AlarmManager;
import android.app.AlertDialog;
import android.app.NotificationManager;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ConfigurationInfo;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.media.RingtoneManager;
import android.net.Uri;
import android.nfc.NfcAdapter;
import android.nfc.Tag;
import android.os.Build;
import android.os.Bundle;
import android.os.Looper;
import android.os.ParcelFileDescriptor;
import android.provider.Settings;
import android.util.DisplayMetrics;
import android.view.KeyEvent;
import android.view.View;
import android.widget.Toast;

import androidx.activity.ComponentActivity;
import androidx.annotation.UiThread;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.app.AppCompatDelegate;
import androidx.core.content.ContextCompat;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayDeque;
import java.util.Deque;

//import com.google.android.apps.auto.sdk.CarActivity;
;

//import static tk.glucodata.Natives.hidescanresults;


//public class MainActivity extends ComponentActivity implements NfcAdapter.ReaderCallback  {
public class MainActivity extends AppCompatActivity implements NfcAdapter.ReaderCallback  {
// Extending from AppCompatActivity requires to use an AppCompat theme for the Activity.
// In the manifest, for the activity, use android:theme="@style/Theme.AppCompat"

//public class MainActivity extends CarActivity implements NfcAdapter.ReaderCallback  {
//    boolean    hideSystem=true;
    GlucoseCurve curve=null;
//    Button okbutton=null;
    private static final String LOG_ID = "MainActivity";
    private NfcAdapter mNfcAdapter=null;
boolean started=false;
void startall() {
	Log.d(LOG_ID,"startall");
	if(!started)  {
		startdisplay();
		// started=true;
		netinitstep();
		 }
	 }
void askNotify() {
        if(Build.VERSION.SDK_INT >=33)  {
		var perm= Manifest.permission.POST_NOTIFICATIONS;
		if(ContextCompat.checkSelfPermission(this, perm)!= PackageManager.PERMISSION_GRANTED)  {
			var permar=new String[]{perm};
			if(shouldShowRequestPermissionRationale(perm) ) {
				 help.help(R.string.notificationpermission,this,l-> {
					requestPermissions(permar, NOTIFICATION_PERMISSION_REQUEST_CODE);
					});
				}
			else   {
				requestPermissions(permar, NOTIFICATION_PERMISSION_REQUEST_CODE);
				}
			return;
			}
		}
	explicit(this);
	}
void startdisplay() {
  Log.i(LOG_ID,"startdisplay");
Applic app=	(Applic)getApplication();
	app.setbackgroundcolor(this) ;
	if(Applic.Nativesloaded)
	    app.needsnatives() ;
	curve = new GlucoseCurve(this);

	setContentView(curve);



   // setSystemUI(false) ;
    try {
	setRequestedOrientation(Natives.getScreenOrientation( ));
	//setRequestedOrientation(SCREEN_ORIENTATION_PORTRAIT);
	}
    catch(       Throwable  error) {
	String mess=error!=null?error.getMessage():null;
	if(mess==null) {
		mess="error";
		}
       Log.stack(LOG_ID ,mess,error);
   }

       getlibrary.getlibrary(this);//after setfilesdir for settings
     handleIntent(getIntent());
	final int unit=Natives.getunit();
	if(!(unit==1||unit==2)) {
		tk.glucodata.settings.Settings.set(this);
		}

	var lang=getString(R.string.language);
	Log.i(LOG_ID,"curlang="+Applic.curlang+" newlang="+lang+" locale="+util.getlocale().getLanguage());
	if(!lang.equals(Applic.curlang)) {
		Natives.setlocale(lang,(tk.glucodata.Applic.hour24=android.text.format.DateFormat.is24HourFormat(Applic.app)));
		Applic.curlang=lang;
		if(SuperGattCallback.dotalk)	
			SuperGattCallback.newtalker(this);
		}
}
static int openglversion=0;
boolean glversion() {
	if(openglversion==0) {
		ActivityManager activityManager = (ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);
		ConfigurationInfo configurationInfo = activityManager.getDeviceConfigurationInfo();
		openglversion=Natives.openglversion();
		try {
		Log.i(LOG_ID,"OpenGl "+Double.parseDouble(configurationInfo.getGlEsVersion()));
		if(( configurationInfo.reqGlEsVersion>>16)<openglversion) {
			help.help("This program requires OpenGL ES "+openglversion+".0 or higher. Your smartphone has OpenGL ES "+ Double.parseDouble(configurationInfo.getGlEsVersion())+" so can't be used.\n\n\n\n\n",this,l->finish());
			return false;
			}
		   } catch(Throwable e){
		   	Log.stack(LOG_ID,"glversion",e);
		   	
		   	}
	//	if(configurationInfo.reqGlEsVersion >=0x30000) openglversion=3;
		}
	return true;
	}
//static int initscreenwidth;
//FragmentManager fragmentManager = getSupportFragmentManager();
//public static boolean wearable=false;
static MainActivity thisone=null;
static void alarmsExact(Context context) {
	if(TargetSDK>30) {
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
			AlarmManager manager= (AlarmManager) context.getSystemService(ALARM_SERVICE);
				if(manager.canScheduleExactAlarms()){
				   Log.d(LOG_ID, "canScheduleExactAlarms");
				   }
				else {
				   Log.d(LOG_ID, "not canScheduleExactAlarms");
				   try {
					   Intent intent=new Intent();
					   intent.setAction(ACTION_REQUEST_SCHEDULE_EXACT_ALARM);
					   context.startActivity(intent);
					   } catch(Throwable th) {
					   	Log.stack(LOG_ID,"ACTION_REQUEST_SCHEDULE_EXACT_ALARM",th);

					   	}
				   }
		}
	}
	}
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
	if(Applic.app.stopprogram>0){
		android.util.Log.e(LOG_ID,"Stop program");
		if(Applic.app.stopprogram==1)
			outofStorageSpace();
		else {
			makefilesfailed();
			}
		return;
		}
	DisplayMetrics metrics= this.getResources().getDisplayMetrics();
	screenheight= metrics.heightPixels;
	screenwidth= metrics.widthPixels;
//	float xcm =2.54f*screenwidth/metrics.xdpi;
	//Natives.setscreenwidthcm(xcm);
//	wearable=xcm<5.8f;


//	hideSystemUI(); initscreenwidth=getscreenwidth(this);
       // setAmbientEnabled();
        Log.i(LOG_ID,"onCreate start target="+ TargetSDK);
	if(TargetSDK>30) {
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
			alarmsExact(this);
			}
		}
//	setTheme(R.style.AppTheme);
//	setTheme(R.style.AppTheme);
	showSystemUI();
	if(!glversion()) return;

        startall();
	thisone=this;
	/*
	if(!isWearable) {
		new Thread(() -> {
			Libreview.testlibre3();
		}).start();
	} */
       Log.i(LOG_ID,"onCreate end");
      // if(!isRelease) test.test();
//       if(!isRelease) Test.test();
    }
void handleIntent(Intent intent) {
	if(intent==null)
		return;
	final Bundle extras = intent.getExtras();
	if(extras!=null)  {
		if(extras.getBoolean(Notify.fromnotification, false)) {
			Notify.stopalarm();
			return;
			}
		else   {
			if(extras.containsKey(setbluetoothon)) {
			    Applic.setbluetooth(this,extras.getBoolean(setbluetoothon,false) );
			    return;
			  }
			var message=extras.getString("alarmMessage");
        		if(message!=null) {
				var cancel=extras.getBoolean("Cancel",false);
				showindialog(message,cancel);
				return;
				}
			}
		}

       if ((intent.getFlags() & Intent.FLAG_ACTIVITY_LAUNCHED_FROM_HISTORY) == 0&&"android.nfc.action.TECH_DISCOVERED".equals(intent.getAction())) {
           curve.waitnfc=true;
           Log.d(LOG_ID,"TECH_DISCOVERED");
           Tag tag = intent.getParcelableExtra(NfcAdapter.EXTRA_TAG);
           startnfc(tag);
       }
//       else hideSystem=true;
   }
static final String setbluetoothon="setbluetoothon";

    @Override
    protected void onNewIntent(Intent intent) {
        Log.d(LOG_ID,"onNewIntent");
        super.onNewIntent(intent);
	handleIntent(intent);
    }
    @Override
    public void onStart() {
        super.onStart();

    }

public static boolean hasnfc=false;

private static boolean askNFC=true;

private static	final int nfcflags=NfcAdapter.FLAG_READER_NFC_V | NfcAdapter.FLAG_READER_NFC_A|NfcAdapter.FLAG_READER_NFC_B|NfcAdapter.FLAG_READER_NFC_F|NfcAdapter.FLAG_READER_SKIP_NDEF_CHECK| NfcAdapter.FLAG_READER_NFC_BARCODE; //=415. Activation of sensor was only possible if app not at the foreground, so I add some flags
public void setnfc() {
try {
    if (mNfcAdapter == null) {
        mNfcAdapter = NfcAdapter.getDefaultAdapter(this);
    }

    if (mNfcAdapter == null) {
    	if(askNFC) {
		Log.i(LOG_ID, "No NFC adapter found!");
		Toast.makeText(this, getResources().getString(R.string.error_nfc_device_not_supported), Toast.LENGTH_SHORT).show();
		askNFC=false;
		return;
		}
    } else {
        if (!mNfcAdapter.isEnabled()) {
	    if(!isWearable) {
		if(askNFC) {
			Toast.makeText(this, getResources().getString(R.string.error_nfc_disabled), Toast.LENGTH_LONG).show();
			if(Natives.backuphostNr( )==0) {
				try {
				   startActivity(new Intent(Settings.ACTION_NFC_SETTINGS));
				   }
				   catch(Throwable th) {
				   	Log.stack(LOG_ID,"Settings.ACTION_NFC_SETTINGS",th);
				   	}
				   }
			askNFC=false;
		       }
		      }

	    return;
        } else {
	
//            mNfcAdapter.enableReaderMode(this, this,  NfcAdapter.FLAG_READER_NO_PLATFORM_SOUNDS|NfcAdapter.FLAG_READER_NFC_V , null);

//	int nosound=NfcAdapter.FLAG_READER_NO_PLATFORM_SOUNDS;
//	final int flags=NfcAdapter.FLAG_READER_NO_PLATFORM_SOUNDS|NfcAdapter.FLAG_READER_NFC_V | NfcAdapter.FLAG_READER_NFC_A|NfcAdapter.FLAG_READER_NFC_B|NfcAdapter.FLAG_READER_NFC_F|NfcAdapter.FLAG_READER_SKIP_NDEF_CHECK| NfcAdapter.FLAG_READER_NFC_BARCODE; //=415. Activation of sensor was only possible if app not at the foreground, so I add some flags
	final int flags=(Natives.nfcsound()?0:NfcAdapter.FLAG_READER_NO_PLATFORM_SOUNDS)|nfcflags; //=415. Activation of sensor was only possible if app not at the foreground, so I add some flags
	Log.i(LOG_ID,"mNfcAdapter.enableReaderMode(this, this,"+ flags+", null)"); 
//	final int flags=NfcAdapter.FLAG_READER_NFC_V ;
	    mNfcAdapter.enableReaderMode(this, this, flags, null); 
	    hasnfc=true;
        }

    }
    } catch(Throwable error) {
	String mess=error!=null?error.getMessage():null;
	if(mess==null) {
		mess="error";
		}
	Log.e(LOG_ID,"setnfc "+mess);
    	}
}

boolean active=false;
private static int resumenr=isRelease?10:2;
    @Override
    protected void onResume() {
        super.onResume();
	if(Applic.stopprogram>0)
		return;
	if(!DiskSpace.check(this)) {
		android.util.Log.e(LOG_ID,"Stop program");
		Applic.stopprogram=1;
		outofStorageSpace();
		return;
		}
	if(!Applic.Nativesloaded)
		return;
 	selectionSystemUI();
	hidekeyboard(this);
	active=true;
	Natives.setpaused(curve);
        if(curve!=null) {
            if (!curve.waitnfc) {
                Log.d(LOG_ID,"onResume setnfc");
                setnfc();
                } 
	    else
                Log.d(LOG_ID,"onResume no setnfc");
          curve.onResume();
        }
	if(openfile!=null) {
		openfile.showchoice(this,true);
//		if(Applic.messagesender!=null) Applic.messagesender.finddevices();
		}
	else   {
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
			 if(!Natives.getaskedNotify( )) {
				if(hasstreamed()) {
					Log.i(LOG_ID,"resumenr="+resumenr);
					if(--resumenr<=0) {
						asknotificationAccess();
					}
					}
				else
					Log.i(LOG_ID,"!hasstreamed");
				}
			 else
				 Log.i(LOG_ID,"Natives.getaskedNotify( )");
			 }
		}

//	if(!isRelease) Notify.testnot();
//	Notify.stopalarm();

 //	Notify.testnot();
//	Notify.test2();
//	Notify.testold();
    }
long nexttime= 0L;

synchronized void   startnfc(Tag tag) {
	long nu=System.currentTimeMillis();
	if(nu<nexttime)
		return;
	nexttime=nu+1000*5;

	    runOnUiThread( ()-> {
		if (curve != null) {
		    curve.searchaway();
		    if(curve.numberview!=null) {
			if(curve.numberview.datepicker!=null) 
				curve.numberview.datepicker.setVisibility(GONE);
			if(curve.numberview.timepicker!=null) 
				curve.numberview.timepicker.setVisibility(GONE);
			}

		} }

	    );
	var techs = tag.getTechList();
	String all="onTagDiscovered: ";

	if(techs!=null) {
		for(var  t:techs) {
			all+=t;
			all+=" ";
			}
		Log.i(LOG_ID,all);

		if(!isWearable) {
			if(techs.length>0) {
				switch(techs[0] ) {
					case "android.nfc.tech.IsoDep":
							showbytes("tag", tag.getId());
							tk.glucodata.NovoPen.Scan.onTag(this,tag);
							return;
					}
				}
			}
		}
	else
		Log.i(LOG_ID,all);
	if(Thread.currentThread().equals( Looper.getMainLooper().getThread() )) {
		new Thread(()->
				ScanNfcV.scan(curve,tag)
			).start();
		}
	else
		ScanNfcV.scan(curve,tag);
	}

private void outofStorageSpace() {
    String message= "Not enough Storage Space!!";
    Toast.makeText(this,message,Toast.LENGTH_SHORT).show();
    AlertDialog.Builder builder = new AlertDialog.Builder(this);
    builder.setNegativeButton(R.string.ok, (dialog, id) -> { // User cancelled the dialog
    	finish();	
	keeprunning.stop();
	System.exit(7);
	}).setTitle("Juggluco has to exit").setMessage(message).show().setCanceledOnTouchOutside(false);
   
   }
private void makefilesfailed() {
 	File files=getFilesDir();
	String filespath=files.getAbsolutePath();
    String message= "Can't create directory:\n"+filespath+(files.isFile()?"\nA regular file with that name exists":"\nNot enough storage space?");
    Toast.makeText(this,message,Toast.LENGTH_SHORT).show();
    AlertDialog.Builder builder = new AlertDialog.Builder(this);
    builder.setNegativeButton(R.string.ok, (dialog, id) -> { // User cancelled the dialog
    	finish();	
	keeprunning.stop();
	System.exit(8);
	}).setTitle("Juggluco has to exit").setMessage(message).show().setCanceledOnTouchOutside(false);
   
   }

void activateresult(boolean res) {
    String message= "Activation "+(res?"successfull":"failed");
    Toast.makeText(this,message,Toast.LENGTH_SHORT).show();
    AlertDialog.Builder builder = new AlertDialog.Builder(this);
    builder.setNegativeButton(R.string.ok, (dialog, id) -> { // User cancelled the dialog
	requestRender();
	}).setMessage(message).show().setCanceledOnTouchOutside(false);
}

    @Override
    public  void onTagDiscovered(Tag tag) {
	startnfc(tag);
    }
    @Override
    protected void onPause() {
        super.onPause();
	 active=false;
	Log.i(LOG_ID,"onPause");
	if(!Applic.Nativesloaded)
		return;

	Natives.setpaused(null);
        if (mNfcAdapter != null) {
	   try {
                mNfcAdapter.disableReaderMode(this);
		}
	catch(Throwable error) {
		String mess=error!=null?error.getMessage():null;
		if(mess==null) {
			mess="error";
			}
		Log.e(LOG_ID,"mNfcAdapter.disableReaderMode "+mess);
		}
            }

        if(curve != null)
            curve.onPause();

	if(!isWearable) {
		wakelibreview(20);
		if(Natives.getlibrelinkused()) {
			final var 	starttime= Natives.laststarttime();
			if(starttime!=0L) {
				XInfuus.sendSensorActivateBroadcast(app, Natives.lastsensorname(), starttime);
				}
			}
		}
	else {
		if(Natives.stopWifi()) {
			Log.i(LOG_ID,"stopWifi");
			UseWifi.stopusewifi();
			}
		}
    }
    @Override
    protected void onDestroy() {
       Log.i(LOG_ID,"onDestroy()");
	thisone=null;
        super.onDestroy();
    }
    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        Log.d(LOG_ID,"onWindowFocusChanged(hasFocus="+hasFocus+")");
//        if (hasFocus&&hideSystem) 
        if (hasFocus) {
		hideSystemUI();
		requestRender();
        	}
    }

@UiThread
@SuppressWarnings("deprecation")
void setSystemUI(boolean val) {
	try {
    View decorView = getWindow().getDecorView();
final    int hideuiflags = View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
            | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
            | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
            | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
            | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
            | View.SYSTEM_UI_FLAG_FULLSCREEN;
    decorView.setSystemUiVisibility(val? 0 : hideuiflags);
    
	} catch(Throwable e) {
		Log.stack(LOG_ID,"setSystemUI",e);
		}

}
/*
void switchSystemUI() {
    View decorView = getWindow().getDecorView();
    setSystemUI(decorView.getSystemUiVisibility()!=0);
}*/

public boolean showui=false;
void selectionSystemUI() {
	setSystemUI(showui||Natives.getsystemUI());
	}
        // Enables regular immersive mode.
        // For "lean back" mode, remove SYSTEM_UI_FLAG_IMMERSIVE.
        // Or for "sticky immersive," replace it with SYSTEM_UI_FLAG_IMMERSIVE_STICKY
public   void  hideSystemUI() {
	Log.d(LOG_ID,"hideSystemUI");
	if(!(Natives.getsystemUI()||showui))
	    setSystemUI(false) ;
    }
//    int: Bitwise-or of flags SYSTEM_UI_FLAG_LOW_PROFILE, SYSTEM_UI_FLAG_HIDE_NAVIGATION, SYSTEM_UI_FLAG_FULLSCREEN, SYSTEM_UI_FLAG_LAYOUT_STABLE, SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION, SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN, SYSTEM_UI_FLAG_IMMERSIVE, and SYSTEM_UI_FLAG_IMMERSIVE_STICKY.
    // Shows the system bars by removing all the flags
// except for the ones that make the content appear under the system bars.
public     void showSystemUI() {
	setSystemUI(true);
	}

/*private void openBluetoothSettings() {
        Intent intent = new Intent();
        intent.setAction("android.settings.BLUETOOTH_SETTINGS");
        startActivity(intent);
    }
    */
public static int screenwidth=0;
public static int screenheight=0;
public static void setsizes(Context context) {
	DisplayMetrics metrics= context.getResources().getDisplayMetrics();
	screenheight= metrics.heightPixels;
	screenwidth= metrics.widthPixels;
	}
public static int getscreenheight(Context context) {
	if(screenwidth==0) {
		setsizes(context);
		}
	return screenheight;
	}
public static int getscreenwidth(Context context) {
	if(screenwidth==0) {
		setsizes(context);

		}
	return screenwidth;
	}
//static public ViewGroup settings=null;
@Override
public void onConfigurationChanged(Configuration newConfig) {
    super.onConfigurationChanged(newConfig);
    Log.i(LOG_ID,"onConfigurationChanged height=" +newConfig.screenHeightDp+" width=" +newConfig.screenWidthDp + " sw="+newConfig.smallestScreenWidthDp);


	screenwidth=0;
	if(Applic.Nativesloaded)
	    if(app.needsnatives() ) {
	    	if(curve!=null) {
			while(doonback() )
				;
			curve.numberview.deleteviews();	
			curve.searchspinner=null;
			if(curve.search!=null) {
				removeContentView(curve.search);
				curve.search=null;
				}
			if(curve.searchcontrol!=null) {
				removeContentView(curve.searchcontrol);
				curve.searchcontrol=null;
				}
			}
	    	}


//	if(settings!=null) settings.invalidate();
}
public void requestRender() {
	if(curve!=null)
		curve.requestRender();
	}

private void netinitstep() {
	if(!started) {
		if (Build.VERSION.SDK_INT < 26||Build.VERSION.SDK_INT>30) {
			useBluetooth(Natives.getusebluetooth()&&finepermission());
			}
		else
			useBluetooth(Natives.getusebluetooth());
		started=true;
		}
}
private boolean gaverational=false;


boolean finepermission() {
	MainActivity act=this;
	Log.i(LOG_ID,"finepermission");
        if (Build.VERSION.SDK_INT >= 23) {
		var noperm=Applic.hasPermissions( act, scanpermissions);
		if(noperm.length==0) {
			return true;
			}
		for(var scanpermission:noperm) {
			if(shouldShowRequestPermissionRationale(scanpermission)) {
				gaverational=true;
			    help.help(Build.VERSION.SDK_INT>30?R.string.nearbypermission:R.string.locationpermission,act,l-> {
				if(help.whelplayout!=null&&help.whelplayout.get()!=null) {
					removeContentView(help.whelplayout.get()); //setContentView makes view inaccessable
					help.whelplayout=null;
					}
				requestPermissions(noperm, LOCATION_PERMISSION_REQUEST_CODE);
				});
				break;
				}
			else   {
				requestPermissions( noperm, LOCATION_PERMISSION_REQUEST_CODE);
				break;
				}
			}

		return false;
		}
	else
		return true;
	}

public static final String[] flashpermissions={ Manifest.permission.CAMERA};
/*
	Log.i(LOG_ID,"flashpermission");
        if(!Natives.getuseflash()) {
	     useflash(false);
	      return -1;
	      }*/
public int flashpermission() {
        if (Build.VERSION.SDK_INT >= 23) {
		var noperm=Applic.hasPermissions( this, flashpermissions);
		if(noperm.length==0) {
	    		useflash(true);
			return 1;
			}
		for(var flashperm:noperm) {
			if(shouldShowRequestPermissionRationale(flashperm)) {
			    help.help(R.string.flashpermission,this,l-> {
				if(help.whelplayout!=null&&help.whelplayout.get()!=null) {
					removeContentView(help.whelplayout.get()); //setContentView makes view inaccessable
					help.whelplayout=null;
					}
					requestPermissions(noperm, FLASH_PERMISSION_REQUEST_CODE);
					});
				return 0 ;
				}
			else   {
				requestPermissions( noperm, FLASH_PERMISSION_REQUEST_CODE);
				return 0 ;
				}
			}
	   return 1;
		}
	else  {
	  		useflash(true);
			return 1;
			}
	}

static public final int SENSOR_PERMISSION_REQUEST_CODE=0x23457;
static final int FLASH_PERMISSION_REQUEST_CODE=0x11224;

static final int LOCATION_PERMISSION_REQUEST_CODE=0x942365;
private static final int NOTIFICATION_PERMISSION_REQUEST_CODE=0x8878;

//private final int STORAGE_PERMISSION_REQUEST_CODE=0x445533;
@Override
public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
	super.onRequestPermissionsResult(requestCode, permissions, grantResults);
	var granted=grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED;
	switch (requestCode) {
		 case NOTIFICATION_PERMISSION_REQUEST_CODE: {
			if (granted) {
				Log.i(LOG_ID,"Required Notification permission");
				}
			else {
				Log.i(LOG_ID,"Required NO Notification permission");
				}
			explicit(this);	
		 	};break;

	        case FLASH_PERMISSION_REQUEST_CODE: {
			Log.i(LOG_ID,"FLASH_PERMISSION_REQUEST_CODE");
			if (granted) {
				useflash(true);
			} else {
				Log.i(LOG_ID,"Flash denied");
				useflash(false);
				}
			} return;
		case LOCATION_PERMISSION_REQUEST_CODE:
			Log.i(LOG_ID,"LOCATION_PERMISSION_REQUEST_CODE");
			if (granted) {
				if(Natives.getusebluetooth())  {
					if(Build.VERSION.SDK_INT <26||Build.VERSION.SDK_INT>30 ) 
						useBluetooth(true);
					else
					    SensorBluetooth.goscan();
					    }
			} else {
				Log.i(LOG_ID,"denied");
				if(!gaverational) {
					bluediag.returntoblue=false;
					if(Build.VERSION.SDK_INT <26)  {
						help.basehelp(R.string.locationpermission,
								this, l -> {
									useBluetooth(false);
								});
						}
					else {
						if(Build.VERSION.SDK_INT>30)
							help.basehelp(R.string.nearbypermission,
								this, l -> {
									useBluetooth(false);
								});

						else
							help.help(R.string.locationpermission, this);
						}
					}
				}
			if(bluediag.returntoblue) {
				bluediag.returntoblue=false;
			        new bluediag(this); 
				}
			return;
	}
	// Other 'case' lines to check for other
	// permissions this app might request.
}


static final String[] keys={"privkey.pem","fullchain.pem"};
public static final int PRIVATE_REQUEST=0x80;
public static final int CHAIN_REQUEST=0x81;
private static  final int REQUEST_NOTIFICATION=0x50;
public static final int REQUEST_EXPORT=0x70;
//public static final int REQUEST_EXPORT=0x54e806d0;
public static final int REQUEST_RINGTONE=0x60;
public static final int REQUEST_LIB=0x200;
public static final int REQUEST_MASK=0xFFFFFF00;
public static final int IGNORE_BATTERY_OPTIMIZATION_SETTINGS=0x100;
static final int OVERLAY_PERMISSION_REQUEST_CODE=0x40;
//public static final int REQUEST_IGNORE_BATTERY_OPTIMIZATIONS=0x300;
Openfile openfile=null;

private void	savekey(FileInputStream input,String outkeystr) {
	File outkey=new File(getFilesDir(),outkeystr);
	byte[] buf=new byte[2*4096];
	int total=0;
	try(FileOutputStream out = new FileOutputStream(outkey)) {
		while(true) {
			int res=input.read(buf);
			if(res<=0) {
				 out.flush(); //TODO: remove
				 out.getFD().sync(); //TODO: remove

				return;
				}
			total+=res;
			out.write(buf,0,res);
			}
	  	}
	catch(Throwable th) {
		Log.stack(LOG_ID,"savekey "+outkeystr,th);
		}
	finally {
		try {
			input.close();
			Log.i(LOG_ID, "wrote " + total + " to " + outkeystr);
			if (total < 50) {
				outkey.delete();
			}
		} catch(Throwable th) {
				Log.stack(LOG_ID,"savekey finally "+outkeystr,th);
		}
		}
	}
@Override
protected void onActivityResult(int requestCode, int resultCode, Intent data) {
	super.onActivityResult(requestCode, resultCode, data);
	Log.format("Main.onActivityResult %x\n",requestCode);
	switch(requestCode) {
		case OVERLAY_PERMISSION_REQUEST_CODE: {
			Log.i(LOG_ID, "OVERLAY_PERMISSION_REQUEST_CODE ");
			if(Floating.cannotoverlay()  ) {
				shoulduseadb(this);
				return ;
				}
			setfloatglucose(this, true);
			};break;
		case REQUEST_NOTIFICATION: {
			Log.i(LOG_ID,"Notification");	
			Natives.setaskedNotify(true);
			};break;
		case IGNORE_BATTERY_OPTIMIZATION_SETTINGS: {
		if(!isWearable) {
			Battery.batteryscreen(this);
			}
			} ; return;
		case REQUEST_LIB: {
			if (resultCode == Activity.RESULT_OK&&openfile!=null) 
				openfile.getlib(data,this);
		}; return;
	      case PRIVATE_REQUEST:
		case CHAIN_REQUEST:
		if (resultCode == Activity.RESULT_OK) {
			Uri uri; 
			if(data==null||(uri= data.getData()) == null) { 
				//TODO error
				return;
				}
			try {
				var input=new FileInputStream( getContentResolver().openFileDescriptor(uri, "r").getFileDescriptor());
				savekey(input,keys[requestCode&~PRIVATE_REQUEST]);
				}
			 catch(Throwable th) {
			 	Log.stack(LOG_ID,"openFileDescriptor",th);
			 	}
			}

	};
	if(!isWearable) {
		if ((requestCode & (REQUEST_MASK | REQUEST_EXPORT)) == REQUEST_EXPORT) {
			try {
				if (resultCode == Activity.RESULT_OK) {
					int type = requestCode & 0xF;
					Uri uri;
					if (data == null || (uri = data.getData()) == null) {
						curve.dialogs.exportlabel.setText(R.string.nodata);
						return;
					}
					int fd = -1;
					try {
						ParcelFileDescriptor parcelFileDescriptor = getContentResolver().openFileDescriptor(uri, "w");
						if (parcelFileDescriptor != null)
							fd = parcelFileDescriptor.detachFd();
						else {
							curve.dialogs.exportlabel.setText("Can't save: parcelFileDescriptor == null");
							return;
						}

					} catch (IOException e) {

						Log.stack(LOG_ID, e);
						curve.dialogs.exportlabel.setText(R.string.failedbyexception);
						return;
					}
					if (Natives.exportdata(type, fd)) {
						curve.dialogs.exportlabel.setText(R.string.saved);
					} else {
						curve.dialogs.exportlabel.setText(R.string.savefailed);
					}
				} else {

					curve.dialogs.exportlabel.setText(R.string.notsaved);
				}

			} catch (Throwable th) {
				Log.stack(LOG_ID, "onActivityResult", th);
			}
			hidekeyboard(this);
			return;
		}
	}
		if ((requestCode & (REQUEST_MASK|REQUEST_RINGTONE)) == REQUEST_RINGTONE) {
			if (resultCode == Activity.RESULT_OK) {
				try {
					Uri uri = data.getParcelableExtra(RingtoneManager.EXTRA_RINGTONE_PICKED_URI);
					String uristr = uri.toString();
					if (uristr != null) {
						int kind = requestCode & 0xF;
						tk.glucodata.RingTones.setring(kind, uristr);
					}
				} catch(Throwable e) {
						Log.stack(LOG_ID,e);
				}
			   }
			}
	}

@Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
    Log.v(LOG_ID,"OnKeyDown");
  if(isWearable) {
	if(!backinapp()) moveTaskToBack(true);
	return true;
    }	
    else {
	    if(keyCode== KeyEvent.KEYCODE_CAMERA) {
			final int camkey=Natives.camerakey();
			switch(camkey) {
				case 0: Natives.setcamerakey(2);break;
				case 1: {
					curve.render.badscan=8;
					curve.requestRender();
				       Log.v(LOG_ID, "Camara");
				     }; return true;
				default:break;
			}
			 }
		}
        return super.onKeyDown(keyCode, event);

    }
    
@Override
    public boolean onKeyLongPress(int keyCode, KeyEvent event) {
        Log.v(LOG_ID,"OnLongKeyPress");
	if(keyCode== KeyEvent.KEYCODE_CAMERA) {
		final int camkey=Natives.camerakey();
		switch(camkey) {
			case 0: Natives.setcamerakey(2);break;
			case 1: {
				curve.render.badscan=8;
				curve.requestRender();
				Log.v(LOG_ID, "Camara");
			}; return true;
			default:break;
		}
	}
	return super.onKeyLongPress(keyCode,event);
        }

public NumberView getnumberview() {
	return curve.numberview;
	}

//public Stack<Runnable> backrun=new Stack<Runnable>() ;
final private Deque<Runnable> backrun = new ArrayDeque<>();
public void setonback(Runnable run) {
	Log.i(LOG_ID,"setonback");
	backrun.addFirst(run);
	}
public boolean doonback() {
	Log.i(LOG_ID,"doonback");
	if(!backrun.isEmpty()) {
//		Runnable wasback=backrun.pop();
		Runnable wasback=backrun.removeFirst();
		wasback.run();
		return true;
		}
	return false;
	}
				
public void poponback() {
	if(!backrun.isEmpty()) 
		backrun.removeFirst();
	}
public void clearonback() {
	backrun.clear();
	}
boolean backinapp()  {
	Log.d(LOG_ID,"backinapp");
	    if(curve!=null&&(curve.render.stepresult & STEPBACK) == STEPBACK) {
		Natives.pressedback();
		curve.render.stepresult = 0;
		hideSystemUI();
		curve.requestRender();
		return true;
		} 
	    else {
		return doonback();
		}
	   }
	@Override
	public	void onBackPressed() {
		Log.d(LOG_ID,"onBackPressed");
		if(!backinapp())
			  moveTaskToBack(true);
	  }
void tonotaccesssettings() {
	Log.i(LOG_ID,"tonotaccesssettings()");
	try {
		Intent intent = new Intent(ACTION_NOTIFICATION_POLICY_ACCESS_SETTINGS);
		startActivityForResult(intent, REQUEST_NOTIFICATION);
		}
	catch(Throwable th) {
		Log.stack(LOG_ID,"ACTION_NOTIFICATION_POLICY_ACCESS_SETTINGS)",th);

		}
}
	public void asknotificationAccess() {
	if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
		try {
	   	  var context = this;
		  NotificationManager n = (NotificationManager) context.getApplicationContext().getSystemService(Context.NOTIFICATION_SERVICE);
		  if(n != null && !n.isNotificationPolicyAccessGranted()) {
			if(isWearable)
				tonotaccesssettings();
			else {
				Log.i(LOG_ID,"disturbe");
				help.basehelp(R.string.disturbhelp, this, l ->  tonotaccesssettings());
				}
			return;
			}
		else {
			Log.i(LOG_ID,"don't ask disturbe");
			Natives.setaskedNotify(true);
			}
		} catch (Throwable th) {
		Log.stack("ACTION_NOTIFICATION_POLICY_ACCESS_SETTINGS", th);
		}
		}
	}
AlertDialog shownglucosealert=null;
void  cancelglucosedialog() {
	if(shownglucosealert!=null) {
		shownglucosealert.dismiss();
		shownglucosealert=null;
		}
	}
void replaceDialogMessage(String message) {
	if(shownglucosealert!=null) {
		shownglucosealert.setMessage(message);
		}
	}
void showindialog(String message,boolean cancel) {
	Log.i(LOG_ID,"showindialog "+message);
	var cont=this;
	if(cancel) {
		cancelglucosedialog();
		}
	 AlertDialog.Builder builder = new AlertDialog.Builder(cont);
	 var dialog=builder.setNegativeButton(R.string.cancel, (dia, id) -> {
		if(cancel) {
			shownglucosealert=null;
			}
		   Notify.stopalarm() ;
	    }).setMessage(message).create();;
	   dialog.setCanceledOnTouchOutside(false);
	    dialog.show();

	if(cancel) shownglucosealert=dialog;
	}

call fineres=null;
public void setfineres(call proc) {
	fineres=proc;
	}
public void useBluetooth(boolean val) {
	Applic.app.initbluetooth(val,this,true);
	if(fineres!=null)
		fineres.call();
	}

public  void setbluetoothmain(boolean on) {
 	Applic.setbluetooth(this, on) ;
	if(fineres!=null)
		fineres.call();
	}
}


