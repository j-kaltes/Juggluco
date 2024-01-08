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
import static tk.glucodata.Notify.unitlabel;

import android.annotation.SuppressLint;
import android.content.res.TypedArray;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.os.Build;
import android.util.DisplayMetrics;
import android.widget.RemoteViews;
import static tk.glucodata.R.id.arrowandvalue;

class RemoteGlucose {
final private static String LOG_ID="RemoteGlucose";
final private Bitmap glucoseBitmap;
final private Canvas canvas;
final private Paint glucosePaint;
final private  float density;
final private float glucosesize;
final private  int notglucosex;

RemoteGlucose(float gl,float notwidth,float xper,int whiteonblack) {

	glucosesize=gl;
	glucosePaint=new Paint();
	glucosePaint.setTextSize(glucosesize);
	glucosePaint.setAntiAlias(true);
	glucosePaint.setTextAlign(Paint.Align.LEFT);
	float notheight=glucosesize*0.8f;
	notglucosex= (int)(notwidth*xper);
	glucoseBitmap = Bitmap.createBitmap((int)notwidth, (int)notheight, Bitmap.Config.ARGB_8888);
	canvas = new Canvas(glucoseBitmap);

	Log.i(LOG_ID,"glucosesize="+glucosesize+"notwidth="+notwidth+" notheight="+notheight+"color="+ format("%x",glucosePaint.getColor()));
	density= notheight/54.0f;
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
	RemoteViews remoteViews= new RemoteViews(Applic.app.getPackageName(),R.layout.arrowandvalue);
	if(glucose==null) {
			return remoteViews;
	}
	var gety = canvas.getHeight() * 0.98f;
	var getx = notglucosex;
	var rate = glucose.rate;
	canvas.drawColor(Color.TRANSPARENT, PorterDuff.Mode.CLEAR);
	glucosePaint.setTextSize(glucosesize);
	if (isNaN(rate)) {
		getx *= 0.82f;
	} else {
		float weightrate=0.0f,arrowy;
		 weightrate = (rate > 1.6 ? -1.0f : (rate < -1.6 ? 1.0f : (rate / -1.6f)));
		 arrowy = gety - glucosesize * .4f + (CommonCanvas.glnearnull(rate) ? 0.0f : (weightrate * glucosesize * .4f));
		Log.i(LOG_ID, "weightrate=" + weightrate+" arrowy="+arrowy);
		drawarrow(canvas, glucosePaint, density, rate, getx * .85f, arrowy);
	}

	canvas.drawText(glucose.value, getx, gety, glucosePaint);
	float valwidth = glucosePaint.measureText(glucose.value, 0, glucose.value.length());
	if(kind<50) {
		if (kind > 1) {
			glucosePaint.setTextSize(glucosesize * .4f);
			canvas.drawText(unitlabel, getx + valwidth + glucosesize * .2f, gety - glucosesize * .25f, glucosePaint);
		} else {
			glucosePaint.setTextSize(glucosesize * .65f);
			canvas.drawText(" " + Applic.app.getString(kind == 0 ? R.string.lowglucoseshort : R.string.highglucoseshort), getx + valwidth + glucosesize * .2f, gety - glucosesize * .15f, glucosePaint);
		}
	    }
	canvas.setBitmap(glucoseBitmap);
	remoteViews.setImageViewBitmap(arrowandvalue, glucoseBitmap);
	return remoteViews;
	}
}
