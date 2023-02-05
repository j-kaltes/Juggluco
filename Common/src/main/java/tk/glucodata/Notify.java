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
import android.app.AlertDialog;
import android.app.Application;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.res.TypedArray;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.PorterDuff;
import android.graphics.Rect;
import android.media.Ringtone;
import android.media.RingtoneManager;
import android.net.Uri;
import android.os.Build;
import android.os.VibrationEffect;
import android.os.Vibrator;
import android.os.VibratorManager;
import android.provider.Settings;
import android.text.style.MetricAffectingSpan;
import android.util.DisplayMetrics;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.ImageView;
import android.widget.RemoteViews;

import java.text.DateFormat;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;

import androidx.annotation.ColorInt;

import static android.app.Notification.FLAG_ONGOING_EVENT;
import static android.app.Notification.VISIBILITY_PUBLIC;
import static android.content.Context.NOTIFICATION_SERVICE;
import static android.content.Context.VIBRATOR_SERVICE;
import static android.graphics.BlendMode.COLOR;
import static android.graphics.Color.BLACK;
import static android.graphics.Color.BLUE;
import static android.graphics.Color.RED;
import static android.graphics.Color.WHITE;
import static android.graphics.PorterDuff.Mode.DST;
import static android.graphics.PorterDuff.Mode.DST_OVER;
import static android.graphics.drawable.Icon.createWithAdaptiveBitmap;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static android.view.WindowManager.*;
import static android.view.WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE;
import static java.lang.Float.isNaN;
import static java.lang.String.format;
import static tk.glucodata.Applic.TargetSDK;
import static tk.glucodata.Applic.app;
import static tk.glucodata.Applic.isRelease;
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.Applic.usedlocale;
import static tk.glucodata.CommonCanvas.drawarrow;
import static tk.glucodata.MainActivity.OVERLAY_PERMISSION_REQUEST_CODE;
import static tk.glucodata.MainActivity.screenheight;
import static tk.glucodata.Natives.getalarmdisturb;
import static tk.glucodata.Natives.getisalarm;
import static tk.glucodata.Natives.setisalarm;
import static tk.glucodata.R.id.arrowandvalue;
import static tk.glucodata.ScanNfcV.vibrates;

//import android.app.Notification;
//import	androidx.core.app.Notification.Builder;
//import android.app.NotificationManagerCompat;
//import androidx.core.app.NotificationManagerCompat;
//import tk.glucodata.Natives;

public class Notify {
    static final private String LOG_ID="Notify";
static Notify onenot=null;
static void init() {
	if(onenot==null) {
		onenot = new Notify();

		}
	}

static String glucoseformat=null;
static String pureglucoseformat=null;
static String unitlabel=null;
//public static int unit=0;
static void mkunitstr(int unit) {
	Applic.unit=unit;	
	pureglucoseformat=unit==1?"%.1f":"%.0f";
	if(isWearable) {
        	glucoseformat=pureglucoseformat;
		}
	else  {
        	glucoseformat=unit==1?"%.1f mmol/L":"%.0f mg/dL";
		unitlabel=unit==1?"mmol/L":"mg/dL";
		}

	}
@SuppressLint("NewApi")
Ringtone setring(String uristr, int res) {
	if(uristr==null||uristr.length()==0) {
		uristr= "android.resource://" + Applic.app.getPackageName() + "/" + res;
		}
	Uri uri=Uri.parse(uristr);
        Ringtone ring = RingtoneManager.getRingtone(Applic.app, uri);
	if(ring==null) {
		Log.i(LOG_ID,"ring==null default");
		uristr= "android.resource://" + Applic.app.getPackageName() + "/" + res;
		uri=Uri.parse(uristr);
		ring = RingtoneManager.getRingtone(Applic.app, uri);
		}
	try {
		if(Build.VERSION.SDK_INT >=23)
			ring.setLooping(true);
		    } catch(Exception e) {
		    	}

	return ring;
	}
static final private int[] defaults ={ R.raw.siren, R.raw.classic, R.raw.ghost, R.raw.nudge,R.raw.elves};

public static Ringtone getring(int kind) {
	return	mkrings(Natives.readring(kind),kind);
	}
Ringtone mkring(String uristr,int kind) {
	Log.i(LOG_ID,"ringtone "+kind+" "+uristr);
	return setring(uristr,defaults[kind]);
	}
static public Ringtone mkrings(String uristr,int res) {
	if(onenot!=null)
		return onenot.mkring(uristr,res);
	return null;
	}

private static Bitmap glucoseBitmap,floatBitmap;
private static Canvas canvas,floatCanvas;
private static Paint glucosePaint,floatPaint;
private static float density,floatdensity;
static float glucosesize;
private static int notglucosex,floatglucosex;
//final private static boolean whiteonblack=isRelease?true:false;
final private static boolean whiteonblack=false;
@ColorInt private static int foregroundcolor=BLACK;
private static WindowManager windowMana;
	private static ImageView floatview=null;

	/*
public static void floattransparent() {
	floatPaint.setTypeface(PixelFormat.TRANSPARENT);
	} */
public static void rewritefloating(Activity context) {
	setfloatglucose(context,false);
	setfloatglucose(context,true);
	}
	public static void rewritefloating() {
		if(floatview!=null) {
			windowMana.removeView(floatview);
			floatview = null;
		}
		makefloat();
		}


	public static void setfloatglucose(Activity context, boolean val) {
	if(!isWearable)  {
		if(val) {
			if(!makefloat()) {
				var settingsIntent = new Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION);
				context.startActivityForResult(settingsIntent, OVERLAY_PERMISSION_REQUEST_CODE);
				return;
			}
		}
		else {
			if(floatview!=null) {
				windowMana.removeView(floatview);
				floatview=null;
			Natives.setfloatglucose(val);
			}
		}
		}
	}

	public static void removefloating() {
		if(floatview!=null) {
			windowMana.removeView(floatview);
			floatview=null;
			Natives.setfloatglucose(false);
		}
		}
	static float xview=0.0f;
	static float yview=0.0f;
	static int transnr=0;
	static void translate(float dx,float dy) {
//		if(transnr++%3==0) 
		//if(transnr++%2==0) 
		{
			//xview -= dx * 2.2f;
			//yview -= dy * 2.2f;
			xview += dx ;
			yview += dy ;
			final var metrics = Applic.app.getResources().getDisplayMetrics();
			var screenwidth = metrics.widthPixels;
			var screenheight = metrics.heightPixels;
			var maxx=screenwidth;
			var maxy=screenheight;
			if(xview<0)
				xview=0;
			if(xview>maxx)
				xview=maxx;
			if(yview<0)
				yview=0;
			if(yview>maxy)
				yview=maxy;
			var params = makeparams(screenwidth, screenheight);
			windowMana.updateViewLayout(floatview, params);
		}
	}
	private static LayoutParams  makeparams(int screenwidth, int screenheight){
		var xpos= -screenwidth*.5f+xview;
		var ypos= -screenheight*.5f+yview;
		var type = (Build.VERSION.SDK_INT < Build.VERSION_CODES.O)?WindowManager.LayoutParams.TYPE_SYSTEM_ALERT: LayoutParams.TYPE_APPLICATION_OVERLAY;
//		var type = (Build.VERSION.SDK_INT < Build.VERSION_CODES.O)? LayoutParams.TYPE_SYSTEM_OVERLAY: LayoutParams.TYPE_APPLICATION_OVERLAY;
//		var flags = LayoutParams.FLAG_NOT_FOCUSABLE|WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE;
		var flags = FLAG_NOT_FOCUSABLE|(Natives.getfloatingTouchable()?0:WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE);
		var params = new WindowManager.LayoutParams( WRAP_CONTENT, WRAP_CONTENT,(int) xpos, (int)ypos, type, flags, PixelFormat.TRANSLUCENT);
		return params;
	}
static boolean cannotoverlay()  {
	if(!isWearable)  {
		if( Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && !Settings.canDrawOverlays(Applic.app)) {
			return true;
		}
		}
		return false;

		}
static int floatingbackground=WHITE;
static int floatingforeground=BLACK;
static int floatfontsize;
	static boolean makefloat() {
	if(!isWearable) {
		if (cannotoverlay()) return false;


		try {
			windowMana = (WindowManager) Applic.app.getSystemService(Context.WINDOW_SERVICE);
			floatview = new ImageView(Applic.app);
			floatview.setOnTouchListener(new Gesture());
			var metrics = Applic.app.getResources().getDisplayMetrics();
			int screenwidth = metrics.widthPixels;
			int screenheight = metrics.heightPixels;
			floatPaint = new Paint();
			floatfontsize = Natives.getfloatingFontsize();
			floatingforeground=Natives.getfloatingforeground();
			floatingbackground=Natives.getfloatingbackground();
			Log.format(LOG_ID+" Natives.getfloatingforeground()=0x%x\n",floatingforeground);
			Log.format(LOG_ID+" Natives.getfloatingbackground()=0x%x\n",floatingbackground);
			//Log.i(LOG_ID,String.format("Natives.getfloatingbackground()=0x%x",floatingbackground));
			if(floatfontsize<5||floatfontsize>(int)(screenheight*.8))
				floatfontsize=(int)glucosesize;
			floatPaint.setTextSize(floatfontsize);
			floatPaint.setAntiAlias(true);
			floatPaint.setTextAlign(Paint.Align.LEFT);
			float notheight = floatfontsize * 0.8f;
			var notwidth = notheight * 3.40;
			floatglucosex = (int) (notwidth * .272f);
			floatBitmap = Bitmap.createBitmap((int) notwidth, (int) notheight, Bitmap.Config.ARGB_8888);
			floatCanvas = new Canvas(floatBitmap);
			windowMana.addView(floatview, makeparams(screenwidth, screenheight));
	//		floatingbackground=WHITE;
	//		floatingforeground=BLACK;
			Log.format(LOG_ID+" Natives.getfloatingforeground()=0x%x\n",floatingforeground);
			Log.format(LOG_ID+" Natives.getfloatingbackground()=0x%x\n",floatingbackground);
		//	floatCanvas.drawColor(floatingbackground, PorterDuff.Mode.CLEAR);
//			floatCanvas.drawColor(floatingbackground);
//			floatPaint.setColor(floatingforeground);
			//	floatCanvas.drawColor(WHITE);
			//	floatPaint.setColor(BLACK);

			floatdensity = notheight / 54.0f;
			var prev = SuperGattCallback.previousglucose;
			if (onenot != null) {
				if(prev==null)  {
					var last=Natives.lastglucose();
					if(last!=null) {
						prev=new notGlucose(last.time*1000L, last.value,  last.rate);
						}
					}
				if (prev != null)
					onenot.floatglucose(prev);
				else

					onenot.repeadoldmessage();
			}
			Natives.setfloatglucose(true);
		} catch (Throwable th) {
			Log.stack(LOG_ID, "makefloat", th);
			floatview = null;
		}
		}
		return true;
	}


	static void mkpaint() {
		if(!isWearable) {
			glucosePaint=new Paint();
			glucosePaint.setTextSize(glucosesize);
			glucosePaint.setAntiAlias(true);
			glucosePaint.setTextAlign(Paint.Align.LEFT);
			DisplayMetrics metrics= Applic.app.getResources().getDisplayMetrics();
			Log.i(LOG_ID,"metrics.density="+ metrics.density+ " width="+metrics.widthPixels+" height="+metrics.heightPixels);
			var notwidth=Math.min(metrics.widthPixels,metrics.heightPixels);
			float notheight=glucosesize*0.8f;
			notglucosex= (int)(notwidth*.12f);
			glucoseBitmap = Bitmap.createBitmap((int)notwidth, (int)notheight, Bitmap.Config.ARGB_8888);
			canvas = new Canvas(glucoseBitmap);

			Log.i(LOG_ID,"glucosesize="+glucosesize+"notwidth="+notwidth+" notheight="+notheight+"color="+ format("%x",glucosePaint.getColor()));
			density= notheight/54.0f;
			if(whiteonblack)
				glucosePaint.setColor(WHITE);
			else {
				var style = (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) ? android.R.style.TextAppearance_Material_Notification_Title : android.R.style.TextAppearance_StatusBar_EventContent;
				int resultColor;
				int[] attrs = {android.R.attr.textColor};
				try {
					@SuppressLint("ResourceType") TypedArray ta = Applic.app.obtainStyledAttributes(style, attrs);
					if(ta != null) {
						int col = ta.getColor(0, Color.TRANSPARENT);
						glucosePaint.setColor(col);
						foregroundcolor=col;
						ta.recycle();
					}
				}
				catch(Throwable e) {
					Log.stack(LOG_ID,"obtainStyledAttributes",e);
				}
			}
		}
	}
	Notify() {
		Log.i(LOG_ID,"showalways="+showalways);
		showalways=Natives.getshowalways();
		mkunitstr(Natives.getunit());
		notificationManager =(NotificationManager) Applic.app.getSystemService(NOTIFICATION_SERVICE);
		createNotificationChannel(Applic.app);
		mkpaint();
	}


	private static final String NUMALARM = "MedicationReminder";
	private static final String GLUCOSEALARM = "glucoseAlarm";
	//  private static final String LOSSALARM = "LossofSensorAlarm";
	private static final String GLUCOSENOTIFICATION = "glucoseNotification";

/*
private void allowbubbel(NotificationChannel  channel) {
	if(!isWearable) {
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
			try {
				channel.setAllowBubbles(true);
			} catch (Throwable th) {
				Log.stack(LOG_ID, th);
			}
		}
	}
	   }*/

	private void createNotificationChannel(Context context) {
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
			String description = context.getString(R.string.numalarm_description);
			int importance = NotificationManager.IMPORTANCE_HIGH;
			NotificationChannel channel = new NotificationChannel(NUMALARM, NUMALARM, importance);
			channel.setSound(null, null);
			channel.setDescription(description);
			// allowbubbel(channel);
			notificationManager.createNotificationChannel(channel);

			description = context.getString(R.string.alarm_description);
			importance = NotificationManager.IMPORTANCE_HIGH;
			channel = new NotificationChannel(GLUCOSEALARM,GLUCOSEALARM, importance);
			channel.setSound(null, null);
			channel.setDescription(description);
			// allowbubbel(channel);
			notificationManager.createNotificationChannel(channel);

			description = context.getString(R.string.notification_description);
			importance = NotificationManager.IMPORTANCE_HIGH;
			channel = new NotificationChannel(GLUCOSENOTIFICATION, GLUCOSENOTIFICATION, importance);
			//allowbubbel(channel);
			channel.setSound(null, null);
			channel.setDescription(description);
			notificationManager.createNotificationChannel(channel);
		}

	}

	//	    channel.setShowBadge(false);
	void lowglucose(notGlucose strgl,float gl,float rate,boolean alarm) {
		arrowglucosealarm(0,GlucoseDraw.getgludraw(gl), format(usedlocale,glucoseformat, gl)+Applic.app.getString(isWearable?R.string.lowglucoseshort:R.string.lowglucose), strgl,GLUCOSEALARM,alarm);
	}
	void highglucose(notGlucose strgl,float gl,float rate,boolean alarm) {
		arrowglucosealarm(1,GlucoseDraw.getgludraw(gl), format(usedlocale,glucoseformat, gl)+Applic.app.getString(isWearable?R.string.highglucoseshort:R.string.highglucose), strgl,GLUCOSEALARM,alarm);


	}
	static private final int glucosenotificationid=81431;
	static private final int glucosealarmid=81432;
	static boolean alertwatch=false;
	static private boolean showalways=Natives.getshowalways();

	static public void glucosestatus(boolean val)  {
		showalways=val;
		Natives.setshowalways(val);
		if(!val) {
			if(onenot!=null)
				onenot.novalue();
		}
	}
	boolean hasvalue=false;
	void normalglucose(notGlucose strgl,float gl,float rate,boolean waiting) {
		var act=MainActivity.thisone;
		if(act!=null)
			act.cancelglucosedialog();
		Log.i(LOG_ID,"normalglucose waiting="+waiting);
		if(waiting)
			arrowglucosealarm(2,GlucoseDraw.getgludraw(gl), format(usedlocale,glucoseformat, gl), strgl,GLUCOSENOTIFICATION ,true);

		else if(!isWearable){
			Log.i(LOG_ID,"arrowglucosenotification  alertwatch="+alertwatch+" showalways="+showalways);
			if(showalways||alertwatch) {
				arrowglucosenotification(2,GlucoseDraw.getgludraw(gl), format(usedlocale,glucoseformat,gl),strgl,GLUCOSENOTIFICATION ,!alertwatch);
				}
			else {
				floatglucose(strgl);
				if(hasvalue) {
					if(keeprunning.started)
						novalue();
					else
						notificationManager.cancel(glucosenotificationid);
				}
			}
		}
		else {
			notificationManager.cancel(glucosealarmid);
		}
	}



	NotificationManager notificationManager;

	//private static boolean isalarm=false;
	private  static Runnable runstopalarm=null;
	private static ScheduledFuture<?> stopschedule=null;
	static public void stopalarm() {
		if(!getisalarm()) {
			Log.d(LOG_ID,"stopalarm not is alarm");
			return;
		}
		Log.d(LOG_ID,"stopalarm is alarm");
		final var stopper=stopschedule;
		if(stopper!=null) {
			stopper.cancel(false);
			stopschedule=null;
		}
		var runner=runstopalarm;
		if(runner!=null) {
			runner.run();
		}
	}
//static int alarmnr=0;

	public static void playring(Ringtone ring,int duration,boolean sound,boolean flash,boolean vibrate,boolean disturb,int kind) {
		if(onenot==null)
			return;
		onenot.playringhier(ring,duration,sound,flash,vibrate,disturb,kind) ;
	}


	Vibrator vibrator = null;
	void vibratealarm(int kind) {
		var context= Applic.app;
		if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.S) {
			vibrator =  ((VibratorManager)(context.getSystemService(Context.VIBRATOR_MANAGER_SERVICE))).getDefaultVibrator();
		}
		else
			vibrator=(Vibrator) context.getSystemService(VIBRATOR_SERVICE);
		if(android.os.Build.VERSION.SDK_INT < 26) {
			if(kind!=0)
				vibrator.vibrate( new long[]  {0, 100, 10,50,50} , 1);
			else
				vibrator.vibrate(new long[] {0, 1000, 500,100,500,500,500,100,100},1);
		}
		else {
			if(kind!=0) {
				final long[] vibrationPatternstart = {0, 70, 50,50,50,50,50};
				final int[] amplitude={0,  255,150,0,255,50,0};
				vibrates(vibrator,vibrationPatternstart,amplitude);
			}
			else {
				final long[] vibrationPatternstart = {0, 1000, 500,100,500,500,500,100,100};
				final int[] amplitude={0,  0xff,128,255,0,255,0,255,50};
				vibrates(vibrator,vibrationPatternstart,amplitude);
			}

			// vibrator.vibrate(VibrationEffect.createWaveform (vibrationPatternstart,amplitude, 1));
//		vibrates(vibrator,vibrationPatternstart,amplitude); 
		}
	}
	//final static private boolean  vibrate=true;
	void stopvibratealarm() {
		vibrator.cancel();
	}
	private synchronized void playringhier(Ringtone ring,int duration,boolean sound,boolean flash,boolean vibrate,boolean disturb,int kind) {
		stopalarm();
		final int[] curfilter={-1};
		final boolean[] doplaysound={true};
		if(sound) {
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
				int filt=notificationManager.getCurrentInterruptionFilter();
				Log.i(LOG_ID,"getCurrentInterruptionFilter()="+filt);

				if(filt!=NotificationManager.INTERRUPTION_FILTER_ALL) {
					if(disturb) {
						if(notificationManager.isNotificationPolicyAccessGranted()) {
							curfilter[0]=filt;
							notificationManager.setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_ALL);
						}
					}
					else
						doplaysound[0]=false;
				}
			}
			if(!isRelease) {
				Log.d(LOG_ID,"play "+ring.getTitle(app));
			}
			if(doplaysound[0])
				ring.play();
		}
		if(!isWearable) {
			if(flash) Flash.start(app);
		}
		if(vibrate) {
			vibratealarm(kind);
		}
		runstopalarm= () -> {
			if(getisalarm()) {
				Log.d(LOG_ID,"runstopalarm  isalarm");
				if(sound) {
					if(doplaysound[0])  {
						if(!isRelease) {
							Log.d(LOG_ID,"stop sound "+ring.getTitle(app));
						}
						ring.stop();
						if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
							if (curfilter[0] != -1) {
								if (notificationManager.isNotificationPolicyAccessGranted()) {
									notificationManager.setInterruptionFilter(curfilter[0]);
								}
							}
						}
					}
				}

				if(!isWearable) {
					if(flash) Flash.stop();
				}
				if(vibrate) {
					stopvibratealarm();
				}
				setisalarm(false);
			}
			else  {
				if(!isRelease) {
					Log.d(LOG_ID,"runstopalarm not isalarm "+ring.getTitle(app));
				}
			}
		};
		setisalarm(true);
//	isalarm=true;
		Log.d(LOG_ID,"schedule stop");
		stopschedule=Applic.scheduler.schedule(runstopalarm, duration, TimeUnit.SECONDS);

	}
	void mksound(int kind) {
		final Ringtone ring=//rings[kind];
				mkring(Natives.readring(kind),kind);
		final int duration=Natives.readalarmduration(kind);
		final boolean flash=Natives.alarmhasflash(kind);
		final boolean sound=Natives.alarmhassound(kind);
		final boolean vibration=Natives.alarmhasvibration(kind);
		final boolean dist= isWearable || getalarmdisturb(kind);

		playringhier(ring,duration,sound,flash,vibration,dist,kind);
	}

	private static void showpopupalarm(String message,Boolean cancel) {
		var act=MainActivity.thisone;
//	if(act==null||!act.active) {
		if(act==null) {
			Log.i(LOG_ID,"showpopupalarm Intent "+message);
			//	Intent intent = new Intent(Applic.app, MainActivity.class);
			Intent intent = new Intent("android.intent.category.LAUNCHER");
			intent.putExtra("alarmMessage", message);
			intent.putExtra("Cancel", cancel);
			intent.setClassName("tk.glucodata", "tk.glucodata.MainActivity");
			intent.addCategory(Intent. CATEGORY_LAUNCHER ) ;
			intent.setAction(Intent.ACTION_MAIN ) ;
			intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK );
			Applic.app.startActivity(intent);
		}
		else {
			Log.i(LOG_ID,"showpopupalarm direct "+message);
			act.runOnUiThread(() -> act.showindialog( message,cancel));
		}
	}
	private void soundalarm(int kind,int draw,String message,String type,boolean alarm) {
		placelargenotification(draw,message,type,!alarm);
		if(alarm) {
			Log.d(LOG_ID,"soundalarm "+kind);
			mksound(kind);
		}
	}

	private void arrowsoundalarm(int kind,int draw,String message,notGlucose sglucose,String type,boolean alarm) {
		arrowplacelargenotification(kind,draw,message,sglucose,type,!alarm);
		if(alarm) {
			Log.d(LOG_ID,"arrowsoundalarm "+kind);
			mksound(kind);
		}
	}

	private void glucosealarm(int kind,int draw,String message,String type,boolean alarm) {
		if(alarm) {
			if(kind!=2)
				showpopupalarm(message,true);
		}
		else {
			final var act=MainActivity.thisone;
			if(act!=null) {
				act.replaceDialogMessage(message);
			}
		}
		if(!alarm&&alertwatch)
			glucosenotification(draw,message,GLUCOSENOTIFICATION ,false);
		else
			soundalarm(kind,draw,message,type,alarm);
	}
	private void arrowglucosealarm(int kind,int draw,String message,notGlucose strglucose,String type,boolean alarm) {
		if(alarm) {
			if(kind!=2)
				showpopupalarm(message,true);
		}
		else {
			final var act=MainActivity.thisone;
			if(act!=null) {
				act.replaceDialogMessage(message);
			}
		}
		if(!alarm&&alertwatch) {
			Log.i(LOG_ID,"arrowglucosealarm alertwatch="+alertwatch);
			arrowglucosenotification(kind,draw,message,strglucose,GLUCOSENOTIFICATION ,false);
		}
		else
			arrowsoundalarm(kind,draw,message,strglucose,type,alarm);
	}

	private void canceller() {
		notificationManager.cancel(glucosenotificationid);
		notificationManager.cancel( numalarmid);
	}
	static public void cancelmessages() {
		if(onenot!=null)
			onenot.canceller();

	}



//Notification.Builder  GluNotBuilder=null;

	static final String fromnotification="FromNotification";
	final static int forcecloserequest=7812;
	static final String closename= "ForceClose";
	final int penmutable= android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.M? PendingIntent.FLAG_IMMUTABLE:0;

private void  floatglucose(notGlucose glucose) {
	if(!isWearable)  {
	if(floatview!=null) {
			var gety = floatCanvas.getHeight() * 0.98f;
		//	floatCanvas.drawColor(floatingbackground, PorterDuff.Mode.CLEAR);
			floatBitmap.eraseColor(floatingbackground);
//			flaotCanvas.drawColor(Color.TRANSPARENT, PorterDuff.Mode.CLEAR);
			floatPaint.setColor(floatingforeground);
			Log.i(LOG_ID,"floatview.setImageBitmap");
			var xpos=floatglucosex;
			var rate=glucose.rate;
			if (!isNaN(rate))  {
				 float weightrate = (rate > 1.6 ? -1.0f : (rate < -1.6 ? 1.0f : (rate / -1.6f)));
				 float arrowy = gety - floatfontsize * .4f + (CommonCanvas.glnearnull(rate) ? 0.0f : (weightrate * floatfontsize * .4f));
				drawarrow(floatCanvas, floatPaint, floatdensity, rate, xpos*.85f, arrowy);
			}
			floatCanvas.drawText(glucose.value, xpos, gety, floatPaint);
			floatCanvas.setBitmap(floatBitmap);
			Applic.RunOnUiThread(()-> {
				floatview.setImageBitmap(floatBitmap);
				});
			}
		else {
			Log.i(LOG_ID,"floatview==null");

			}
		}
	}
static void testold() {
	long time = System.currentTimeMillis()-1000*60*5;
	final String tformat= timef.format(time);
	Notify.onenot.oldfloatmessage(tformat,false);
	}
private static String oldmessagetime=null;
private static boolean oldmessagealarm=false;
private void repeadoldmessage() {
	if(oldmessagetime!=null)
		oldfloatmessage(oldmessagetime,oldmessagealarm);
	}
private void	oldfloatmessage(String tformat,boolean alarm)  {
	if(!isWearable) {
		oldmessagetime=tformat;
		oldmessagealarm=alarm;
		if(floatview!=null) {

			if(alarm) {
//				floatCanvas.drawColor(floatingforeground);
				floatBitmap.eraseColor(floatingforeground);
				floatPaint.setColor(floatingbackground);
				}
			else  {
//				floatCanvas.drawColor(floatingbackground);

			//	floatCanvas.drawColor(floatingbackground, PorterDuff.Mode.CLEAR);
				floatBitmap.eraseColor(floatingbackground);
				floatPaint.setColor(floatingforeground);
				}
			float fontsize= floatfontsize;

			var gety = floatCanvas.getHeight() * 0.37f;
			floatPaint.setTextSize(fontsize*.3f);
			var xpos=0.2f;
			String message=Applic.app.getString(R.string.newnewvalue);
			floatCanvas.drawText(message, xpos, gety, floatPaint);
			gety = floatCanvas.getHeight() * 0.88f;
			floatCanvas.drawText(tformat, xpos, gety, floatPaint);
			floatCanvas.setBitmap(floatBitmap);
			Applic.RunOnUiThread(()-> {
				floatview.setImageBitmap(floatBitmap);
				});
			floatPaint.setTextSize(fontsize);
			}
		else {
			Log.i(LOG_ID,"floatview==null");

			}
		}
	}

	private Notification  makearrownotification(int kind,int draw,String message,notGlucose glucose,String type,boolean once) {
		var intent =mkpending();
		var GluNotBuilder=mkbuilderintent(type,intent);
		Log.i(LOG_ID,"makearrownotification setOnlyAlertOnce("+once+") "+glucose.value);
		GluNotBuilder.setSmallIcon(draw).setOnlyAlertOnce(once).setContentTitle(message);
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
			GluNotBuilder.setVisibility(VISIBILITY_PUBLIC);
		}
//	GluNotBuilder.setUsesChronometer(true);
		if(!isWearable) {
			RemoteViews remoteViews= new RemoteViews(app.getPackageName(),R.layout.arrowandvalue);
			if(whiteonblack) {
				if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
					GluNotBuilder.setColorized(true);
					GluNotBuilder.setColor(BLACK);
				}
				else
					remoteViews.setInt(arrowandvalue, "setBackgroundColor", BLACK);
			}
			long unixtime = System.currentTimeMillis() / 1000L;
			var gety = canvas.getHeight() * 0.98f;
			var getx = notglucosex;
			var rate = glucose.rate;
//		 canvas.drawColor(0);
			if(whiteonblack)
				canvas.drawColor(BLACK);
			else
				canvas.drawColor(Color.TRANSPARENT, PorterDuff.Mode.CLEAR);
			glucosePaint.setTextSize(glucosesize);
			if (isNaN(rate)) {
//					getx=width*0.45f
				getx *= 0.82f;
			} else {
				float weightrate=0.0f,arrowy;
				 weightrate = (rate > 1.6 ? -1.0f : (rate < -1.6 ? 1.0f : (rate / -1.6f)));
				Log.i(LOG_ID, "weightrate=" + weightrate);
				 arrowy = gety - glucosesize * .4f + (CommonCanvas.glnearnull(rate) ? 0.0f : (weightrate * glucosesize * .4f));
				drawarrow(canvas, glucosePaint, density, rate, getx * .85f, arrowy);
			}

			canvas.drawText(glucose.value, getx, gety, glucosePaint);
			float valwidth = glucosePaint.measureText(glucose.value, 0, glucose.value.length());
			if (kind > 1) {
				glucosePaint.setTextSize(glucosesize * .4f);
				canvas.drawText(unitlabel, getx + valwidth + glucosesize * .2f, gety - glucosesize * .25f, glucosePaint);
			} else {
				glucosePaint.setTextSize(glucosesize * .65f);
				canvas.drawText(" " + app.getString(kind == 0 ? R.string.lowglucoseshort : R.string.highglucoseshort), getx + valwidth + glucosesize * .2f, gety - glucosesize * .15f, glucosePaint);
			}

			canvas.setBitmap(glucoseBitmap);
		remoteViews.setImageViewBitmap(arrowandvalue, glucoseBitmap);
		if(TargetSDK<31||Build.VERSION.SDK_INT < 31) {
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
				GluNotBuilder.setStyle(new Notification.DecoratedCustomViewStyle());
			}
			}


		GluNotBuilder.setShowWhen(true);
		
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
			GluNotBuilder.setCustomContentView(remoteViews);
		} else
			GluNotBuilder.setContent(remoteViews);

		floatglucose(glucose);
		}
	if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
		GluNotBuilder.setTimeoutAfter(glucosetimeout);
	}
	if(isWearable||alertseperate) {GluNotBuilder.setAutoCancel(true);}
	if(once)
		GluNotBuilder.setPriority(Notification.PRIORITY_DEFAULT);
	else  {
	//	GluNotBuilder.setPriority(Notification.PRIORITY_DEFAULT);
		GluNotBuilder.setPriority(Notification.PRIORITY_HIGH);
//		GluNotBuilder.setPriority(Notification.PRIORITY_MAX);
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
			GluNotBuilder.setCategory(Notification.CATEGORY_ALARM);
		}
	}

	 Log.i(LOG_ID,(once?"":"not ")+"only once");

	 Notification notif= GluNotBuilder.build();
	notif.when= glucose.time;
	 return notif;


    }
@SuppressWarnings({"deprecation"})
private PendingIntent mkpending() {
	Intent notifyIntent = new Intent(Applic.app,MainActivity.class);
	notifyIntent.putExtra(fromnotification,true);
	notifyIntent.addCategory(Intent. CATEGORY_LAUNCHER ) ;
	notifyIntent.setAction(Intent. ACTION_MAIN ) ;
	notifyIntent.setFlags(Intent. FLAG_ACTIVITY_CLEAR_TOP | Intent. FLAG_ACTIVITY_SINGLE_TOP );
	return   PendingIntent.getActivity(Applic.app, 0, notifyIntent, PendingIntent.FLAG_UPDATE_CURRENT|penmutable);
	}
private Notification.Builder   mkbuilderintent(String type,PendingIntent notifyPendingIntent) {
	Notification.Builder  GluNotBuilder;
 	if(true) {
		 if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.O) {
			 GluNotBuilder = new Notification.Builder(Applic.app, type);
		 }
		 else {
					 GluNotBuilder = new Notification.Builder(Applic.app);
		 }
		}
	else {
		 if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
			GluNotBuilder.setChannelId(type);
		}
	GluNotBuilder.setContentIntent(notifyPendingIntent).setOnlyAlertOnce(true);
	if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT_WATCH) {
		GluNotBuilder.setGroup("aa2");
		}
	return GluNotBuilder;
	}
private Notification.Builder   mkbuilder(String type) {
	return   mkbuilderintent( type,mkpending()) ;
	}


static final private boolean  alertseperate=false;
void fornotify(Notification notif) {
	Log.i(LOG_ID, "fornotify ");
	if(isWearable) {
              notificationManager.notify(glucosealarmid,notif);
	    }
	else  {
		if(alertseperate)  {
			notificationManager.cancel(glucosealarmid);
			notificationManager.notify(glucosealarmid,notif);
			}
		else {
			notificationManager.cancel(glucosenotificationid);
			if(keeprunning.theservice!=null) {
				keeprunning.theservice.startForeground(glucosenotificationid,notif);
			       }
			else 
			       notificationManager.notify(glucosenotificationid,notif);
		       }
		}
	}
//static final long glucosetimeout=1000*60*5;
static final long glucosetimeout=1000*60*3;

	/*
	@SuppressWarnings("deprecation")
void oldnotification(long time) {
	String message= Applic.app.getString(R.string.newnewvalue)+ timef.format(time);
	Log.i(LOG_ID,"oldnotification "+message);
	var GluNotBuilder=mkbuilder(GLUCOSENOTIFICATION);
	if (Build.VERSION.SDK_INT < 31) {
			GluNotBuilder.setStyle(new Notification.DecoratedCustomViewStyle());
		}
	GluNotBuilder.setContentTitle(message).setSmallIcon(R.drawable.novalue).setPriority(Notification.PRIORITY_DEFAULT);
	if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
		GluNotBuilder.setVisibility(VISIBILITY_PUBLIC);
		}
	if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
		GluNotBuilder.setTimeoutAfter(glucosetimeout);
		}
//	RemoteViews remoteViews= new RemoteViews(Applic.app.getPackageName(),R.layout.smallnotification);
//	GluNotBuilder.setContent(remoteViews);
	Notification notif= GluNotBuilder.build();
	fornotify(notif);
	Log.i(LOG_ID,"end oldnotification");
	}
*/
void oldnotification(long time) {
	final String tformat= timef.format(time);
	String message = Applic.app.getString(R.string.newnewvalue) + tformat;
	oldfloatmessage(tformat,false) ;
	 placelargenotification(R.drawable.novalue, message,GLUCOSENOTIFICATION,true);
	}
	@SuppressWarnings("deprecation")
private Notification  makenotification(int draw,String message,String type,boolean once) {
	var GluNotBuilder=mkbuilder(type);
	if (Build.VERSION.SDK_INT < 31) {
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
			GluNotBuilder.setStyle(new Notification.DecoratedCustomViewStyle());
		}
	}
	Log.i(LOG_ID,"makenotification "+message);
        //GluNotBuilder.setSmallIcon(draw).setOnlyAlertOnce(once).setContentTitle(message);
        GluNotBuilder.setSmallIcon(draw).setOnlyAlertOnce(once).setContentTitle(message).setShowWhen(true);

	if(!isWearable) {
		RemoteViews remoteViews = new RemoteViews(app.getPackageName(), R.layout.text);
//		remoteViews.setInt(R.id.content, "setBackgroundColor", WHITE);
		remoteViews.setTextColor(R.id.content, foregroundcolor);
		remoteViews.setTextViewText(R.id.content, message);
//	RemoteViews remoteViews=null;
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
			GluNotBuilder.setCustomContentView(remoteViews);
		} else
			GluNotBuilder.setContent(remoteViews);
	}
	if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
		GluNotBuilder.setVisibility(VISIBILITY_PUBLIC);
	}
	if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
		GluNotBuilder.setTimeoutAfter(glucosetimeout);
	}
	if(isWearable) {GluNotBuilder.setAutoCancel(true);}
	if(once)
		GluNotBuilder.setPriority(Notification.PRIORITY_DEFAULT);
	else  {
	//	GluNotBuilder.setPriority(Notification.PRIORITY_HIGH);
		GluNotBuilder.setPriority(Notification.PRIORITY_MAX);
//		GluNotBuilder.setDefaults(Notification.DEFAULT_VIBRATE);
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
			GluNotBuilder.setCategory(Notification.CATEGORY_ALARM);
		}
	}

	 Log.i(LOG_ID,(once?"":"not ")+"only once");

	 Notification notif= GluNotBuilder.build();
	 notif.when = System.currentTimeMillis();
	 return notif;

    }




Notification getforgroundnotification() {
	final String mess= app.getString(SensorBluetooth.blueone!=null?R.string.connectwithsensor:R.string.exchangedata);
	Notification not=makenotification(R.drawable.novalue,mess,GLUCOSENOTIFICATION,true);
	not.flags|= FLAG_ONGOING_EVENT;
	return not;
	}

static public void shownovalue() {
	init();
	onenot.novalue();
	}
private void novalue() {
	Log.i(LOG_ID,"novalue");

	fornotify(getforgroundnotification());
//	notificationManager.notify(glucosenotificationid,getforgroundnotification());
	}
public void foregroundno(Service service) {
	Notification not=getforgroundnotification();
     	service.startForeground(glucosenotificationid, not);
	Log.i(LOG_ID,"startforeground");
	}
static public void foregroundnot(Service service) {
//	Application app=service.getApplication();
	init();
	onenot.foregroundno(service);
	}	
 public void  placelargenotification(int draw,String message,String type,boolean once) {
        hasvalue=true;
	fornotify(makenotification(draw,message,type,once));

    }
static int testtimes=1;
static void testnot() {
	float gl=11.4f;
	var timmsec= System.currentTimeMillis()-1000;
	float rate=(float)(1.6*Math.pow(-1,testtimes));
	--testtimes;
	boolean waiting=false;
	var sglucose=new notGlucose(timmsec, format(Applic.usedlocale,Notify.pureglucoseformat, gl) , rate);
//	Notify.onenot.normalglucose(sglucose,gl, rate,waiting);
	var dr=GlucoseDraw.getgludraw(gl);
//	Notify.onenot.makearrownotification(2,dr,"message",sglucose,GLUCOSENOTIFICATION ,false);
 }

static void test2() {
	float gl=7.8f;
	float rate=0.0f;
	SuperGattCallback.dowithglucose("Serialnumber", (int)(gl*18f), gl,rate, 0,System.currentTimeMillis()) ;
	}

 public void  arrowplacelargenotification(int kind,int draw,String message,notGlucose glucose,String type,boolean once) {
        hasvalue=true;
	fornotify(makearrownotification(kind,draw,message,glucose,type,once));

    }
 public void  glucosenotification(int draw,String message,String type,boolean once) {
 	Log.i(LOG_ID,"notify "+message);
	fornotify(makenotification(draw,message,type,once));
	}
 public void  arrowglucosenotification(int kind,int draw,String message,notGlucose glucose,String type,boolean once) {
 	Log.i(LOG_ID,"notify "+message);
	fornotify(makearrownotification( kind,draw, message, glucose, type, once)) ;
	}

final private 	int numalarmid=81432;


static DateFormat timef = DateFormat.getTimeInstance(DateFormat.SHORT);

Notification.Builder  NumNotBuilder=null;
	@SuppressWarnings("deprecation")
public void  notifyer(int draw,String message,String type,int notid) {
	 if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
		 NumNotBuilder = new Notification.Builder(Applic.app, type);
	 }
	 else
		 NumNotBuilder = new Notification.Builder(Applic.app);

//	notificationManager.cancel(glucosenotificationid);

	 Intent notifyIntent = new Intent(Applic.app,MainActivity.class);
	notifyIntent.putExtra(fromnotification,true);
        notifyIntent.addCategory(Intent. CATEGORY_LAUNCHER ) ;
        notifyIntent.setAction(Intent. ACTION_MAIN ) ;
        notifyIntent.setFlags(Intent. FLAG_ACTIVITY_CLEAR_TOP | Intent. FLAG_ACTIVITY_SINGLE_TOP );
	PendingIntent notifyPendingIntent = PendingIntent.getActivity(Applic.app, 0, notifyIntent, PendingIntent.FLAG_UPDATE_CURRENT|penmutable);

	NumNotBuilder.setAutoCancel(true).setContentIntent(notifyPendingIntent).setContentTitle(message);
//	NumNotBuilder.setAutoCancel(true).setContentIntent(notifyPendingIntent).setSubText(message);
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
			NumNotBuilder.setVisibility(VISIBILITY_PUBLIC);
			NumNotBuilder.setCategory(Notification.CATEGORY_ALARM);
		}
	var timemess=			timef.format(System.currentTimeMillis()) + ": " + message;
	showpopupalarm(timemess,false);
	if(!isWearable) {
		RemoteViews NumRemoteViewss = new RemoteViews(Applic.app.getPackageName(), R.layout.numalarm);
		NumRemoteViewss.setInt(R.id.text, "setBackgroundColor", WHITE);
		NumRemoteViewss.setTextColor(R.id.text, BLACK);
		NumRemoteViewss.setTextViewText(R.id.text, timemess);
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
			NumNotBuilder.setCustomContentView(NumRemoteViewss);
		} else
			NumNotBuilder.setContent(NumRemoteViewss);
		}
//	NumNotBuilder.setSmallIcon(draw).setPriority(Notification.PRIORITY_HIGH);
	NumNotBuilder.setSmallIcon(draw).setPriority(Notification.PRIORITY_MAX);

		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
			NumNotBuilder.setTimeoutAfter(1000L*60*60*2);
		}

		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT_WATCH) {
		NumNotBuilder.setGroup("aa1");
	}
	notificationManager.notify(notid,NumNotBuilder.build());
	}
 public void  amountalarm(String message) {
 	try {
		mksound(3);
		notifyer( R.drawable.numalarm ,message,NUMALARM,numalarmid);
		}
	catch(Throwable e) {
		Log.stack(LOG_ID,e);
		}
	}
//final private 	int lossalarmid=77332;
 public void  lossalarm(long time) {
 	Log.i(LOG_ID,"lossalarm");
	final String tformat= timef.format(time);
	final String message= "***  "+Applic.app.getString(R.string.newnewvalue)+tformat+" ***";

	oldfloatmessage(tformat, true) ;
	glucosealarm(4,R.drawable.loss ,message, GLUCOSENOTIFICATION ,true);
	}
	



}
