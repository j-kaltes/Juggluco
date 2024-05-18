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
/*      Thu Oct 05 15:29:10 CEST 2023                                                 */


package tk.glucodata;

import static android.graphics.Color.BLACK;
import static android.graphics.Color.WHITE;
import static java.lang.Float.isNaN;
import static java.lang.String.format;

import static tk.glucodata.CommonCanvas.drawarrow;
import static tk.glucodata.Natives.getisalarm;
import static tk.glucodata.Notify.penmutable;
import static tk.glucodata.Notify.stopalarmrequest;
import static tk.glucodata.Notify.unitlabel;

import android.annotation.SuppressLint;
import android.app.PendingIntent;
import android.content.Intent;
import android.content.res.TypedArray;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.Rect;
import android.os.Build;
import android.util.DisplayMetrics;
import android.view.View;
import android.widget.RemoteViews;
import static tk.glucodata.R.id.arrowandvalue;

import java.text.DateFormat;
import java.util.Date;

class RemoteGlucose {
final private static String LOG_ID="RemoteGlucose";
final private Bitmap glucoseBitmap;
final private Canvas canvas;
final private Paint glucosePaint;
final private  float density;
final private float glucosesize;
final private  int notglucosex;
final private int timeHeight;
final private int timesize;
RemoteGlucose(float gl,float notwidth,float xper,int whiteonblack,boolean givetime) {

	glucosesize=gl;
	glucosePaint=new Paint();
	glucosePaint.setAntiAlias(true);
	glucosePaint.setTextAlign(Paint.Align.LEFT);
	float notheight=glucosesize*0.8f;
	notglucosex= (int)(notwidth*xper);
	density= notheight/54.0f;

	if(givetime) {	
		Rect bounds=new Rect();
		timesize= (int)(glucosesize*.2f);
		glucosePaint.setTextSize(timesize);
		glucosePaint.getTextBounds("8.9",0,3, bounds);
		timeHeight=(int)(bounds.height()*1.2f);
		notheight+=timeHeight;
		}
	else {
		timeHeight =  timesize = 0;
	}
	glucosePaint.setTextSize(glucosesize);
	glucoseBitmap = Bitmap.createBitmap((int)notwidth, (int)notheight, Bitmap.Config.ARGB_8888);
	canvas = new Canvas(glucoseBitmap);

	Log.i(LOG_ID,"timesize="+timesize+" timeHeight="+timeHeight+" glucosesize="+glucosesize+" notwidth="+notwidth+" notheight="+notheight+"color="+ format("%x",glucosePaint.getColor()));
	switch(whiteonblack) {
		case 1: 
		case 2: glucosePaint.setColor(WHITE);break;
		default: {
		var style = (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) ? android.R.style.TextAppearance_Material_Notification_Title : android.R.style.TextAppearance_StatusBar_EventContent;
		int[] attrs = {android.R.attr.textColor};
		try {
			@SuppressLint("ResourceType") TypedArray ta = Applic.app.obtainStyledAttributes(style, attrs);
			if(ta != null) {
				int col = ta.getColor(0, Color.TRANSPARENT);
				glucosePaint.setColor(col);
				Notify.foregroundcolor=col;
				ta.recycle();
			}
		}
		catch(Throwable e) {
			Log.stack(LOG_ID,"obtainStyledAttributes",e);
		}
		}
		};
}

final RemoteViews arrowremote(int kind, notGlucose glucose) {
	final boolean alarm=kind<2&&getisalarm();
	RemoteViews remoteViews= new RemoteViews(Applic.app.getPackageName(),alarm?R.layout.alarm:R.layout.arrowandvalue);
	if(alarm) {
		Intent closeintent=new Intent(Applic.app,NumAlarm.class);
		closeintent.setAction(Notify.stopalarm);
		PendingIntent closepending=PendingIntent.getBroadcast(Applic.app, stopalarmrequest, closeintent,PendingIntent.FLAG_UPDATE_CURRENT|penmutable);
		remoteViews.setOnClickPendingIntent(R.id.stopalarm, closepending); 
		}
	else {
//	   remoteViews.setViewVisibility(R.id.stopalarm, View.INVISIBLE);
	   }


	if(glucose==null) {
			return remoteViews;
	}
	var gety = (canvas.getHeight()-timeHeight) * 0.98f;
	var getx = notglucosex;
	var rate = glucose.rate;
	canvas.drawColor(Color.TRANSPARENT, PorterDuff.Mode.CLEAR);
	final var useglsize=glucosesize;
	final var usedensity=density;
	glucosePaint.setTextSize(useglsize);
	if (isNaN(rate)) {
		getx *= 0.82f;
	} else {
		float weightrate=0.0f,arrowy;
		 weightrate = (rate > 1.6 ? -1.0f : (rate < -1.6 ? 1.0f : (rate / -1.6f)));
		 arrowy = gety - useglsize * .4f + weightrate * useglsize * .4f;
		Log.i(LOG_ID, "weightrate=" + weightrate+" arrowy="+arrowy);
		drawarrow(canvas, glucosePaint, usedensity, rate, getx * .85f, arrowy);
	}

	canvas.drawText(glucose.value, getx, gety, glucosePaint);
	if(kind<50) {
		float valwidth = glucosePaint.measureText(glucose.value, 0, glucose.value.length());
		if (kind > 1) {
			glucosePaint.setTextSize(useglsize * .4f);
			canvas.drawText(unitlabel, getx + valwidth + useglsize * .2f, gety - useglsize * .25f, glucosePaint);
		} else {
			glucosePaint.setTextSize(useglsize * .65f);
			canvas.drawText(" " + Applic.app.getString(kind == 0 ? R.string.lowglucoseshort : R.string.highglucoseshort), getx + valwidth + useglsize * .2f, gety - useglsize * .15f, glucosePaint);
		}
	    }
	else {
		var timestr= DateFormat.getTimeInstance(DateFormat.SHORT).format(new Date(glucose.time));
		glucosePaint.setTextSize(timesize);
		canvas.drawText(timestr, usedensity*16, gety+timeHeight, glucosePaint);
		Log.i(LOG_ID,"time: "+timestr);
		}
	canvas.setBitmap(glucoseBitmap);
	remoteViews.setImageViewBitmap(arrowandvalue, glucoseBitmap);
	return remoteViews;
	}
}
