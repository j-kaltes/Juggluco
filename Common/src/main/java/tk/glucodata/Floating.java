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
/*      Wed Feb 08 11:40:32 CET 2023                                                 */


package tk.glucodata;
import static android.graphics.Color.BLACK;
import static android.graphics.Color.WHITE;
import static android.view.WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE;
import static java.lang.Float.isNaN;
import static java.util.Collections.swap;
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.CommonCanvas.drawarrow;
import static tk.glucodata.MainActivity.OVERLAY_PERMISSION_REQUEST_CODE;
import static tk.glucodata.Notify.timef;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.Intent;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.PorterDuff;
import android.graphics.Rect;
import android.os.Build;
import android.provider.Settings;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.ImageView;
import android.widget.TextView;

import java.text.DateFormat;
import java.util.Date;

import tk.glucodata.Applic;
import tk.glucodata.Notify;
import tk.glucodata.R;

public class Floating extends View {

	static private final String LOG_ID="Floating";
static private final int oldage=60*3;
static void init() {
	Log.i(LOG_ID,"init()");
	var pos=Natives.getfloatingPos( );
	final var metrics = Applic.app.getResources().getDisplayMetrics();
	density=metrics.density;
	Log.i(LOG_ID,"density="+density);
	if(pos!=0) {
		Floating.xview=pos&0xFFFF;
		Floating.yview=pos>>16;
		}
	else {
		var h = metrics.heightPixels;
		var w = metrics.widthPixels;
		if(h > w) {
			xview = h * .5f;
			yview = w * .5f;
		} else {
			xview = w * .5f;
			yview = h * .5f;
		}
		}
	Floating.showtime= Natives.getfloattime();
	}
	public Floating(Context context) {
		super(context);
	}
public	static void setTouchable(boolean isChecked) {
	Natives.setfloatingTouchable(isChecked);
	if(!isChecked) {
		int y= (int) yview;
		Natives.setfloatingPos(((int)xview)|(0xFFFFFFFF&(y<< 16)));
		}
	rewritefloating();
	}
static void setbackgroundcolor(int c)  {
		Natives.setfloatingbackground(c );
		var getc=Natives.getfloatingbackground();
		Log.i(LOG_ID,"getfloatingbackground("+(getc&0xFFFFFFFF)+")");
		floatingbackground=c;
		}
static void setforegroundcolor(int c)  {
	Natives.setfloatingforeground( c);
	floatingforeground=c;;
	}
static public void	setbackgroundalpha(int alpha) {
	int initialColor= Natives.getfloatingbackground( );
	setbackgroundcolor(0xFFFFFFFF&((initialColor&0xFFFFFF)|alpha<<24));
	}
	public static Paint floatPaint;
private static WindowManager windowMana;
	static float floatdensity;
	public static int floatglucosex;
	static Floating floatview=null;
public static void invalidatefloat() {
	final var view=floatview;
	if(view!=null)
		view.postInvalidate();
	}

public static void rewritefloating(Activity context) {
	setfloatglucose(context,true);
	}
	public static void rewritefloating() {
		makefloat();
		}

 static void shoulduseadb(Context context) {
	    final var builder = new AlertDialog.Builder(context);
	    builder.setTitle(R.string.overlaypermission);
	    var dialog=builder.setMessage(R.string.overlaypermissionmessage).setPositiveButton(R.string.ok, (dia, id) -> {
			Log.i(LOG_ID,"now ask overlay permission");
		}) .show();
                dialog.setCanceledOnTouchOutside(false);
	    TextView messageText = (TextView)dialog.findViewById(android.R.id.message);
		final var metrics = Applic.app.getResources().getDisplayMetrics();
		var screenwidth = metrics.widthPixels;
		density=metrics.density;
		int pad=(int)(screenwidth*.05);
	    messageText.setGravity(Gravity.LEFT);
	    messageText.setPadding(pad,0,pad,0);
	}

	public static void setfloatglucose(Activity context, boolean val) {
	//if(!isWearable)  
	{
		if(val) {
			if(!makefloat()) {
				if(isWearable) {
					shoulduseadb(context);
					}
				else 
				{
				try {
					var settingsIntent = new Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION);
					context.startActivityForResult(settingsIntent, OVERLAY_PERMISSION_REQUEST_CODE); 
				} catch(Throwable th) {
					Log.stack(LOG_ID,"Settings.ACTION_MANAGE_OVERLAY_PERMISSION",th);
					shoulduseadb(context);
					}
				}
			return;
			}
		}
		else {
	 		turnoffFloating();
			}
	}
	}

	public static void removeFloating() {
		if(floatview!=null) {
			Log.i(LOG_ID,"removeFloating()");
			windowMana.removeView(floatview);
			floatview=null;
			}
		}

	private static void turnoffFloating() {
		removeFloating();
		Natives.setfloatglucose(false);
		}
	private static void hidefloating() {
		Log.i(LOG_ID,"hidefloating() density="+density); 
		hide=true;
		removeFloating();	
		windowMana = (WindowManager) Applic.app.getSystemService(Context.WINDOW_SERVICE);
		floatview=new Floating(Applic.app);
		floatPaint = new Paint();
		floatPaint.setTextAlign(Paint.Align.LEFT);
		floatPaint.setColor(floatingforeground);
 		final int nrdens=(isWearable&&density<2)?20:14;

		final int floatingheight=(int)(density*nrdens),floatingwidth=floatingheight*2;
		floatPaint.setTextSize(floatingheight);
		final var type = (Build.VERSION.SDK_INT < Build.VERSION_CODES.O)?WindowManager.LayoutParams.TYPE_SYSTEM_ALERT: WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY;
		final var flags = FLAG_NOT_FOCUSABLE;
		var params= new WindowManager.LayoutParams( floatingwidth,(int)(floatingheight),(int) floatingx, (int)floatingy, type, flags, PixelFormat.TRANSLUCENT);
		windowMana.addView(floatview, params);
		}
	static float xview;
	static float yview;
	static int transnr=0;
static private void translate(float dx,float dy) {
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
		var params = makeparams(screenwidth, (int)(screenheight));
		if(floatview==null)  {
			Log.e(LOG_ID,"floatview==null");
			return;
			}
		windowMana.updateViewLayout(floatview, params);
		}
static float floatingx,floatingy;
private static WindowManager.LayoutParams makeparams(int screenwidth, int screenheight){
		var xpos= -screenwidth*.5f+xview;
		var ypos= -screenheight*.5f+yview;
		floatingx=xpos;
		floatingy=ypos;

		var type = (Build.VERSION.SDK_INT < Build.VERSION_CODES.O)?WindowManager.LayoutParams.TYPE_SYSTEM_ALERT: WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY;
		var flags = FLAG_NOT_FOCUSABLE|(Natives.getfloatingTouchable()?0:WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE);
		return	 new WindowManager.LayoutParams( floatingwidth,(int)(floatingheight),(int) xpos, (int)ypos, type, flags, PixelFormat.TRANSLUCENT);
	}
static boolean cannotoverlay()  {
		if( Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && !Settings.canDrawOverlays(Applic.app)) {
			return true;
		}

		return false;
		}
static int floatingbackground=WHITE;
static int floatingforeground=BLACK;

static int			floatingwidth;
static int			floatingheight;
private static int timesize;
private static int timeHeight;
public static boolean showtime=true;
private static float density;
public static int floatfontsize;
	static boolean makefloat() {
	{
	Log.i(LOG_ID,"makefloat");

		hide=false;

		if(floatview!=null) {
			windowMana.removeView(floatview);
			floatview=null;
			}
		if (cannotoverlay()) return false;


		try {
			windowMana = (WindowManager) Applic.app.getSystemService(Context.WINDOW_SERVICE);
			floatview = new Floating(Applic.app);
			var metrics = Applic.app.getResources().getDisplayMetrics();
			int screenwidth = metrics.widthPixels;
			int screenheight = metrics.heightPixels;
			floatPaint = new Paint();
			floatfontsize = Natives.getfloatingFontsize();
			floatingforeground=Natives.getfloatingforeground();
			floatingbackground=Natives.getfloatingbackground();
			Log.format(LOG_ID+" Natives.getfloatingforeground()=0x%x\n",floatingforeground);
			Log.format(LOG_ID+" Natives.getfloatingbackground()=0x%x\n",floatingbackground);
			if(floatfontsize<5||floatfontsize>(int)(screenheight*.8)) {
				floatfontsize=(int)Notify.glucosesize; }
			floatPaint.setAntiAlias(true);
			floatPaint.setTextAlign(Paint.Align.LEFT);
			float notheight = floatfontsize * 0.8f;
			var notwidth = notheight * 3.40;

			floatdensity = notheight / 54.0f;
			if(showtime) {	
				Rect bounds=new Rect();
				timesize= (int)(floatfontsize*.25f);
				floatPaint.setTextSize(timesize);
				floatPaint.getTextBounds(":58",0,3, bounds);
				timeHeight=(int)(bounds.height()*1.2f);
				notheight+=timeHeight;
				}
			else {
				timeHeight =  timesize = 0;
			} 
			floatPaint.setTextSize(floatfontsize);

			floatglucosex = (int) (notwidth * .272f);
			floatingwidth=(int)notwidth;

			floatingheight=(int)(notheight);

			windowMana.addView(floatview, makeparams(screenwidth, (int)(screenheight)));

			Natives.setfloatglucose(true);
		} catch (Throwable th) {
			Log.stack(LOG_ID, "makefloat", th);
			floatview = null;
		}
		}
		return true;
	}




	private void	oldfloatmessage(Canvas floatCanvas, long time)  {
		floatCanvas.drawColor(floatingbackground);
		floatPaint.setColor(floatingforeground);
	final String tformat= timef.format(time*1000L);
	float fontsize= floatfontsize;
	var gety = floatCanvas.getHeight() * 0.37f;
	floatPaint.setTextSize(fontsize*.3f);
	var xpos=0.2f;
	String message= Applic.app.getString(R.string.nonewvalue);
	floatCanvas.drawText(message, xpos, gety, floatPaint);
	gety = floatCanvas.getHeight() * 0.88f;
	floatCanvas.drawText(tformat, xpos, gety, floatPaint);
	floatPaint.setTextSize(fontsize);
	}

static boolean hide=false;
@Override 
protected void onDraw(Canvas floatCanvas) {
 	super.onDraw(floatCanvas);
	Log.i(LOG_ID,"onDraw");
	final strGlucose  glucose= Natives.lastglucose();
	if(glucose!=null) {
		final var now=System.currentTimeMillis()/1000L;
		final var age=now-glucose.time;
		if(age<oldage) {
			if(hide) {
				Log.i(LOG_ID,"onDraw hide");
				final var gety = floatCanvas.getHeight() * 0.85f;
				final var xpos=0.2f;
				floatCanvas.drawColor(floatingbackground);
				floatPaint.setColor(floatingforeground);
			//	floatCanvas.drawText("27.8", xpos, gety, floatPaint);
				floatCanvas.drawText(glucose.value, xpos, gety, floatPaint);
				return;
				}
			floatCanvas.drawColor(floatingbackground);
			floatPaint.setColor(floatingforeground);
			var gety = (floatCanvas.getHeight()-timeHeight) * 0.98f;
			var xpos=floatglucosex;
			var rate=glucose.rate;
			if (!isNaN(rate))  {
				 float weightrate = (rate > 1.6 ? -1.0f : (rate < -1.6 ? 1.0f : (rate / -1.6f)));
				 float arrowy = gety - floatfontsize * .4f + (CommonCanvas.glnearnull(rate) ? 0.0f : (weightrate * floatfontsize * .4f));
				drawarrow(floatCanvas, floatPaint, floatdensity, rate, xpos*.85f, arrowy);
//				drawarrow(floatCanvas, floatPaint, floatdensity, rate, showtime?xpos:(.85f*xpos), arrowy);
				}
			floatCanvas.drawText(glucose.value, xpos, gety*.9659f, floatPaint);
			if(showtime)  {
				var timestr= DateFormat.getTimeInstance(DateFormat.SHORT).format(new Date(glucose.time*1000L));
				floatPaint.setTextSize(timesize);
				floatCanvas.drawText(timestr, density, gety+timeHeight, floatPaint);
				floatPaint.setTextSize(floatfontsize);
				Log.i(LOG_ID,"time: "+glucose.time+" "+timestr);
				}
		return;
		}
		else {
			if(!hide) {
				oldfloatmessage(floatCanvas,glucose.time);
				return;
				}
			}

		}
	else {
		if(!hide) {
			final var gety = floatCanvas.getHeight() * 0.8f;
			final var xpos=0.2f;
			final float fontsize= floatfontsize;
			floatCanvas.drawColor(floatingbackground);
			floatPaint.setColor(floatingforeground);
			floatPaint.setTextSize(fontsize*.68f);
			floatCanvas.drawText(Applic.app.getString(R.string.novalue), xpos, gety, floatPaint);
			floatPaint.setTextSize(fontsize);
			return;
			}
		}
	Log.i(LOG_ID,"onDraw hide");
	final var gety = floatCanvas.getHeight() * 0.82f;
	final var xpos=0.2f;
	floatCanvas.drawColor(floatingbackground);
	floatPaint.setColor(floatingforeground);
	floatCanvas.drawText(" X ", xpos, gety, floatPaint);
	

	}

	private   boolean moved =false;
	private long downstart;
	private int mindowntime=400;
	private int maxdoubletime=500;
	private float startX,startY;
@Override 
public void	onScreenStateChanged(int state) {
	switch(state) { 
		case SCREEN_STATE_ON: {
			Log.i(LOG_ID,"onScreenStateChanged(int state)"); 
			invalidatefloat(); 
			}
		};

	
	}
private static final float movethreshold=6.0f;
public boolean onTouchEvent(MotionEvent event) {
	if(Natives.turnoffalarm()) Notify.stopalarm();
        Log.i(LOG_ID,event.toString());
	try {
        switch (event.getAction() & MotionEvent.ACTION_MASK) {
		case MotionEvent.ACTION_BUTTON_PRESS:
            case MotionEvent.ACTION_DOWN:

                Log.i(LOG_ID,"Down");
		startX= event.getX();
		startY= event.getY();
		Log.i(LOG_ID,"startX="+startX+" xview="+xview+" floatglucosex="+floatglucosex);
	    	if(hide) {
			Log.i(LOG_ID,"unhide");
			hide=false;
			if(Natives.getfloatglucose( ))
				makefloat();
			moved =false;
			downstart=event.getEventTime();
			return true;
			} 
		else {
			if(startX< floatglucosex ) {
				Log.i(LOG_ID,"<floatglucosex");
				var act=MainActivity.thisone;
				if(act!=null) {
					Log.i(LOG_ID,"startActivityIfNeeded( new Intent(Applic.app,MainActivity)). active="+act.active);

					act.startActivityIfNeeded( new Intent(Applic.app,MainActivity.class),0);
					//act.runOnUiThread(() -> act.startActivityIfNeeded( new Intent(Applic.app,MainActivity.class),0));
					}
				else {
					Log.i(LOG_ID,"MainActivity.thisone=null");
					var intent=new Intent(Applic.app,MainActivity.class);
					 intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TOP);
					keeprunning.theservice.startActivity( intent);
					}
				}
			if((event.getEventTime()-downstart)<maxdoubletime) {
				setTouchable(false);
				}
			}
                moved =false;
		downstart=event.getEventTime();
                break;

	case	MotionEvent.ACTION_BUTTON_RELEASE:
	case MotionEvent.ACTION_POINTER_UP:
	case MotionEvent.ACTION_UP:
		if(!moved) {
			if((event.getEventTime()-downstart)>mindowntime) {
			    hidefloating();
			    downstart=0;
			  	} 
			else {
				if(!isWearable)  {
					long time=Natives.saylastglucose();
					if(time>=0) {
						var talker=SuperGattCallback.talker;
						if(talker==null) {
							SuperGattCallback.newtalker(null);
							talker=SuperGattCallback.talker;
							}
						talker.speak(time==0L?Applic.app.getString(R.string.novalue):Applic.app.getString(R.string.nonewvalue) + timef.format(time));
						}
					}
				}
			}
		else 
			 downstart=0;
		break;
   case MotionEvent.ACTION_MOVE: {
		var distanceX= event.getX() - startX;
		var distanceY= event.getY() - startY;
	        Log.i(LOG_ID,"DRAG dx="+distanceX+" dy="+distanceY);
		var absX=Math.abs(distanceX);
		var absY=Math.abs(distanceY);
		if(absX>movethreshold||absY>movethreshold) {
		    moved =true;
		    translate(distanceX*.45f,distanceY*.45f);
		    }
	  	else {
			if(!moved) {
				if((event.getEventTime()-downstart)>mindowntime)
				    hidefloating();
				    }
			}
			

                }
                break;
        }
		} catch(Throwable th) {
			Log.stack(LOG_ID,"onTouchEvent",th);
			}
        return true; 
    }


};

