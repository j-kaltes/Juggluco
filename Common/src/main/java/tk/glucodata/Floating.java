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
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.CommonCanvas.drawarrow;
import static tk.glucodata.MainActivity.OVERLAY_PERMISSION_REQUEST_CODE;
import static tk.glucodata.Notify.glucosesize;
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
import android.os.Build;
import android.provider.Settings;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.ImageView;
import android.widget.TextView;

import tk.glucodata.Applic;
import tk.glucodata.Notify;
import tk.glucodata.R;

public class Floating extends View {

	static private final String LOG_ID="Floating";
static private final int oldage=60*3;

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

	/*
public static void floattransparent() {
	floatPaint.setTypeface(PixelFormat.TRANSPARENT);
	} */
public static void rewritefloating(Activity context) {
//	setfloatglucose(context,false);
	setfloatglucose(context,true);
	}
	public static void rewritefloating() {
		makefloat();
		}

 static void shoulduseadb(Context context) {
	    final var builder = new AlertDialog.Builder(context);
	    builder.setTitle(R.string.overlaypermission);
	    var dialog=builder.setMessage(R.string.overlaypermissionmessage)
	    .setPositiveButton(R.string.ok, (dia, id) -> {
			Log.i(LOG_ID,"now ask overlay permission");
		}) .show();
	    TextView messageText = (TextView)dialog.findViewById(android.R.id.message);
		final var metrics = Applic.app.getResources().getDisplayMetrics();
		var screenwidth = metrics.widthPixels;
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
				else {
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
			removefloating();
			}
	}
	}

	public static void removefloating() {
		if(floatview!=null) {
			windowMana.removeView(floatview);
			floatview=null;
			}
		Natives.setfloatglucose(false);
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
	private static WindowManager.LayoutParams makeparams(int screenwidth, int screenheight){
		var xpos= -screenwidth*.5f+xview;
		var ypos= -screenheight*.5f+yview;
		var type = (Build.VERSION.SDK_INT < Build.VERSION_CODES.O)?WindowManager.LayoutParams.TYPE_SYSTEM_ALERT: WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY;
//		var type = (Build.VERSION.SDK_INT < Build.VERSION_CODES.O)? LayoutParams.TYPE_SYSTEM_OVERLAY: LayoutParams.TYPE_APPLICATION_OVERLAY;
//		var flags = LayoutParams.FLAG_NOT_FOCUSABLE|WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE;
		var flags = FLAG_NOT_FOCUSABLE|(Natives.getfloatingTouchable()?0:WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE);
//		var format= ((floatingbackground&0xFF000000)==0)? PixelFormat.TRANSLUCENT:PixelFormat.OPAQUE;
		return	 new WindowManager.LayoutParams( floatingwidth,floatingheight,(int) xpos, (int)ypos, type, flags, PixelFormat.TRANSLUCENT);
//		var params = new WindowManager.LayoutParams( WRAP_CONTENT, WRAP_CONTENT,(int) xpos, (int)ypos, type, flags);
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
public static int floatfontsize;
	static boolean makefloat() {
	{

		if(floatview!=null) {
			windowMana.removeView(floatview);
			floatview=null;
			}
		if (cannotoverlay()) return false;


		try {
			windowMana = (WindowManager) Applic.app.getSystemService(Context.WINDOW_SERVICE);
			floatview = new Floating(Applic.app);
//			floatview.setOnTouchListener(new Gesture());
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
			floatingwidth=(int)notwidth;
			floatingheight=(int)notheight;
//			var floatBitmap = Bitmap.createBitmap((int) notwidth, (int) notheight, Bitmap.Config.ARGB_8888);
//			floatview.setImageBitmap(floatBitmap);

			windowMana.addView(floatview, makeparams(screenwidth, screenheight));

			floatdensity = notheight / 54.0f;
			/*
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
			} */
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
	String message= Applic.app.getString(R.string.newnewvalue);
	floatCanvas.drawText(message, xpos, gety, floatPaint);
	gety = floatCanvas.getHeight() * 0.88f;
	floatCanvas.drawText(tformat, xpos, gety, floatPaint);
	floatPaint.setTextSize(fontsize);
	}


@Override 
protected void onDraw(Canvas floatCanvas) {
 	super.onDraw(floatCanvas);
	Log.i(LOG_ID,"onDraw");
	final var glucose= Natives.lastglucose();
	if(glucose!=null) {
		final var now=System.currentTimeMillis()/1000L;
		final var age=now-glucose.time;
		if(age<oldage) {
			var gety = floatCanvas.getHeight() * 0.98f;
			floatCanvas.drawColor(floatingbackground);
			floatPaint.setColor(floatingforeground);
			var xpos=floatglucosex;
			var rate=glucose.rate;
			if (!isNaN(rate))  {
				 float weightrate = (rate > 1.6 ? -1.0f : (rate < -1.6 ? 1.0f : (rate / -1.6f)));
				 float arrowy = gety - floatfontsize * .4f + (CommonCanvas.glnearnull(rate) ? 0.0f : (weightrate * floatfontsize * .4f));
				drawarrow(floatCanvas, floatPaint, floatdensity, rate, xpos*.85f, arrowy);
				}
			floatCanvas.drawText(glucose.value, xpos, gety, floatPaint);
			
			}
		else
			oldfloatmessage(floatCanvas,glucose.time);

		}
	else {
		final var gety = floatCanvas.getHeight() * 0.8f;
		final var xpos=0.2f;
		final float fontsize= floatfontsize;
		floatCanvas.drawColor(floatingbackground);
		floatPaint.setColor(floatingforeground);
		floatPaint.setTextSize(fontsize*.68f);
		floatCanvas.drawText("No value", xpos, gety, floatPaint);
		floatPaint.setTextSize(fontsize);
		}
	

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

public boolean onTouchEvent (MotionEvent event) {
	if(Natives.turnoffalarm()) Notify.stopalarm();
        Log.i(LOG_ID,event.toString());
        switch (event.getAction() & MotionEvent.ACTION_MASK) {
		case MotionEvent.ACTION_BUTTON_PRESS:
            case MotionEvent.ACTION_DOWN:
                Log.i(LOG_ID,"Down");
		startX= event.getX();
		startY= event.getY();
                moved =false;
		if((event.getEventTime()-downstart)<maxdoubletime) {
			setTouchable(false);
			}
		downstart=event.getEventTime();
                break;

	case	MotionEvent.ACTION_BUTTON_RELEASE:
	case MotionEvent.ACTION_POINTER_UP:
	case MotionEvent.ACTION_UP:
		if(!moved) {
			if((event.getEventTime()-downstart)>mindowntime) {
			    removefloating();
			    downstart=0;
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
//		if(distanceX!=0.0||distanceY!=0.0)  {
		   if(absX>5.0||absY>5.0) {
		    moved =true;
		    translate(distanceX*.45f,distanceY*.45f);
		    }
	  	else {
			if(!moved) {
				if((event.getEventTime()-downstart)>mindowntime)
				    removefloating();
				    }
			}
			

                }
                break;
        }
        return true; 
    }


};

