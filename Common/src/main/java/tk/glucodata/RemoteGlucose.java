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
import static tk.glucodata.Log.doLog;
import static tk.glucodata.Natives.getisalarm;
import static tk.glucodata.Notify.penmutable;
import static tk.glucodata.Notify.stopalarmrequest;
import static tk.glucodata.Notify.unitlabel;

import android.graphics.Color;

import android.annotation.SuppressLint;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
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

import static tk.glucodata.NumberView.minhourstr;
import static tk.glucodata.R.id.arrowandvalue;

import java.text.DateFormat;
import java.util.Date;

public class RemoteGlucose {
final private static String LOG_ID="RemoteGlucose";

private static final String PREFS = "juggluco_dev";

/**
 * Returns a colour for the glucose value text, mirroring GlucoDataHandler's
 * four-tier alarm colour scheme:
 *
 *   GREEN  — within the in-range target window (targetlow .. targethigh)
 *   YELLOW — outside the target window but within the alarm range
 *            (alarmlow .. targetlow  OR  targethigh .. alarmhigh)
 *   RED    — at or beyond the alarm boundary (≤ alarmlow  OR  ≥ alarmhigh)
 *            also covers very-low / very-high thresholds
 *
 * Falls back:
 *   • if target thresholds are zero/unset, treat alarm thresholds as the
 *     target (green zone = alarmlow..alarmhigh, no yellow band).
 *   • if alarm thresholds are also unset, return WHITE.
 *
 * Unit safety: all native threshold values are already in the user's chosen
 * display unit (the C layer converts before storing).
 */
static int glucoseRangeColor(float glValue) {
    final float alarmLow  = Natives.alarmlow();
    final float alarmHigh = Natives.alarmhigh();
    // No alarm thresholds configured — can't colour
    if (alarmLow <= 0f && alarmHigh <= 0f) return WHITE;

    // RED: at or beyond alarm boundary
    if (alarmLow  > 0f && glValue <= alarmLow)  return Color.RED;
    if (alarmHigh > 0f && glValue >= alarmHigh) return Color.RED;

    // Target (inner) range — use if configured, else fall back to alarm range
    float targetLow  = Natives.targetlow();
    float targetHigh = Natives.targethigh();
    if (targetLow  <= 0f) targetLow  = alarmLow;
    if (targetHigh <= 0f) targetHigh = alarmHigh;

    // GREEN: within target range
    if (glValue > targetLow && glValue < targetHigh) return Color.GREEN;

    // YELLOW: between target and alarm boundary (warn zone)
    return Color.YELLOW;
}

// ── Widget background alpha ───────────────────────────────────────────────
private static final String KEY_BG_ALPHA = "widget_bg_alpha";

/**
 * Returns the stored widget background alpha (0-255). 0 = fully transparent
 * (default, same as before). Any non-zero value adds a semi-transparent black
 * background behind the widget content.
 */
public static int getWidgetBgAlpha() {
    return Applic.app
            .getSharedPreferences(PREFS, Context.MODE_PRIVATE)
            .getInt(KEY_BG_ALPHA, 0);
}

/** Persists a new widget background alpha value (0-255). */
public static void setWidgetBgAlpha(int alpha) {
    Applic.app
            .getSharedPreferences(PREFS, Context.MODE_PRIVATE)
            .edit().putInt(KEY_BG_ALPHA, Math.max(0, Math.min(255, alpha))).apply();
}
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
   if(notwidth<=0.0f)
        notwidth=1.0f;
   if(notheight<=0.0f)
        notheight=1.0f;
   glucoseBitmap = Bitmap.createBitmap((int)notwidth, (int)notheight, Bitmap.Config.ARGB_8888);
   canvas = new Canvas(glucoseBitmap);

   {if(doLog) {Log.i(LOG_ID,"timesize="+timesize+" timeHeight="+timeHeight+" glucosesize="+glucosesize+" notwidth="+notwidth+" notheight="+notheight+"color="+ format("%x",glucosePaint.getColor()));};};
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

static final String stopalarmAction= "StopAlarm";
final RemoteViews arrowremote(int kind, notGlucose glucose,final boolean alarm) {
   RemoteViews remoteViews= new RemoteViews(Applic.app.getPackageName(),alarm?R.layout.alarm:R.layout.arrowandvalue);
   if(alarm) {
      Intent closeintent=new Intent(Applic.app,NumAlarm.class);
      closeintent.setAction(stopalarmAction);
      PendingIntent closepending=PendingIntent.getBroadcast(Applic.app, stopalarmrequest, closeintent,PendingIntent.FLAG_UPDATE_CURRENT|penmutable);
      remoteViews.setOnClickPendingIntent(R.id.stopalarm, closepending);
      }
   if(glucose==null||glucose.value==null) {
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
      {if(doLog) {Log.i(LOG_ID, "weightrate=" + weightrate+" arrowy="+arrowy);};};
      drawarrow(canvas, glucosePaint, usedensity, rate, getx * .85f, arrowy);
   }

   // Apply range colour; alarm widgets keep the original paint colour
   if (!alarm) {
       glucosePaint.setColor(glucoseRangeColor(SuperGattCallback.previousglucosevalue));
   }
   canvas.drawText(glucose.value, getx, gety, glucosePaint);
   final boolean glucosealarm=kind<2||kind>4;
   if(kind<50) {
      float valwidth = glucosePaint.measureText(glucose.value, 0, glucose.value.length());
      if(!glucosealarm) {
         glucosePaint.setTextSize(useglsize * .4f);
         canvas.drawText(unitlabel, getx + valwidth + useglsize * .2f, gety - useglsize * .25f, glucosePaint);
      } else {
         glucosePaint.setTextSize(useglsize * .65f);
         canvas.drawText(" " + Notify.alarmtext(kind ), getx + valwidth + useglsize * .2f, gety - useglsize * .15f, glucosePaint);
      }
       }
   else {
      var timestr= minhourstr(glucose.time);
      glucosePaint.setTextSize(timesize);
      canvas.drawText(timestr, usedensity*16, gety+timeHeight, glucosePaint);
      {if(doLog) {Log.i(LOG_ID,"time: "+timestr);};};
      }
   canvas.setBitmap(glucoseBitmap);
   remoteViews.setImageViewBitmap(arrowandvalue, glucoseBitmap);
   return remoteViews;
   }

/**
 * Renders three rows: glucose value (range-coloured), Δ delta, and elapsed-time minutes.
 * Uses the same bitmap/canvas as arrowremote so the same RemoteGlucose instance works.
 *
 * @param glucose   current reading (notGlucose)
 * @param stale     true when the reading is older than glucosetimeout — paints gray with strikethrough
 */
final RemoteViews deltaTimeRemote(notGlucose glucose, boolean stale) {
    return deltaTimeRemote(glucose, stale, 0, 0);
}

/**
 * Renders three rows (glucose value, Δ delta, elapsed time) into a fresh bitmap
 * sized to the actual widget cell dimensions so the image fills the cell cleanly.
 *
 * @param glucose    current reading
 * @param stale      true when reading is older than glucosetimeout
 * @param bitmapW    widget width in pixels (0 = fall back to shared bitmap width)
 * @param bitmapH    widget height in pixels (0 = fall back to square)
 */
final RemoteViews deltaTimeRemote(notGlucose glucose, boolean stale,
                                   int bitmapW, int bitmapH) {
   RemoteViews remoteViews = new RemoteViews(Applic.app.getPackageName(), R.layout.glucose_delta_widget);
   if (glucose == null || glucose.value == null) {
       return remoteViews;
   }

   // Use a fresh, correctly-sized bitmap so the image fills the widget cell.
   final int bw = bitmapW > 0 ? bitmapW : glucoseBitmap.getWidth();
   final int bh = bitmapH > 0 ? bitmapH : bw;
   final Bitmap bmp = Bitmap.createBitmap(bw, bh, Bitmap.Config.ARGB_8888);
   final Canvas cv  = new Canvas(bmp);
   cv.drawColor(Color.TRANSPARENT, PorterDuff.Mode.CLEAR);

   final float w    = bw;
   final float h    = bh;

   // Layout proportions (matching GDH style):
   //  top 50%: glucose value (large) + trend arrow side by side
   //  next 25%: Δ delta (same color as value, bold)
   //  bottom 25%: elapsed time (white/bright, bold, visible on any background)

   final float basePx    = Math.min(w, h);
   final float valueSize = basePx * 0.40f;   // large glucose value
   final float smallSize = basePx * 0.20f;   // delta row
   final float timeSize  = basePx * 0.18f;   // elapsed time row

   final Paint paint = new Paint();
   paint.setAntiAlias(true);
   paint.setFakeBoldText(true);
   paint.setTextAlign(Paint.Align.CENTER);
   paint.setTypeface(glucosePaint.getTypeface());

   final int valueColor = stale ? Color.GRAY : glucoseRangeColor(SuperGattCallback.previousglucosevalue);

   // ── glucose value (top area, horizontally centred) ───────────
   paint.setTextSize(valueSize);
   paint.setColor(valueColor);
   final Rect vBounds = new Rect();
   paint.getTextBounds(glucose.value, 0, glucose.value.length(), vBounds);
   // Place value baseline so text is in the top 50% of the cell
   final float valueTopPad = h * 0.04f;
   final float valueY = valueTopPad + vBounds.height();

   // Draw trend arrow to the right of the glucose number
   final float rate = glucose.rate;
   final float arrowAreaW = w * 0.38f;   // right 38% reserved for arrow
   final float glucoseCx  = w * 0.38f;   // centre of glucose text area
   cv.drawText(glucose.value, glucoseCx, valueY, paint);

   // Strikethrough if stale
   if (stale) {
       final float strikeY = valueY - vBounds.height() * 0.35f;
       final float sw = paint.getStrokeWidth();
       paint.setStrokeWidth(Math.max(3f, valueSize * 0.06f));
       cv.drawLine(glucoseCx - vBounds.width() * 0.55f, strikeY,
                   glucoseCx + vBounds.width() * 0.55f, strikeY, paint);
       paint.setStrokeWidth(sw);
   }

   // ── trend arrow ───────────────────────────────────────────────
   if (!Float.isNaN(rate)) {
       paint.setColor(valueColor);
       paint.setTextSize(valueSize);
       // density for drawarrow: based on arrow area height
       final float arrowDensity = valueSize / 54.0f;
       final float arrowX = w * 0.78f;
       final float weightrate = (rate > 1.6f ? -1.0f : (rate < -1.6f ? 1.0f : (rate / -1.6f)));
       final float arrowY = valueY - valueSize * 0.4f + weightrate * valueSize * 0.4f;
       CommonCanvas.drawarrow(cv, paint, arrowDensity, rate, arrowX, arrowY);
   }

   // ── delta row ─────────────────────────────────────────────────
   paint.setTextSize(smallSize);
   paint.setColor(valueColor);
   paint.setFakeBoldText(true);
   final float delta5 = glucose.rate * 5f;
   final String deltaStr;
   if (Float.isNaN(delta5)) {
       deltaStr = "\u0394 --";
   } else {
       final String fmt = Applic.unit == 1 ? "%+.1f" : "%+.0f";
       deltaStr = "\u0394 " + String.format(Applic.usedlocale, fmt, delta5);
   }
   final Rect dBounds = new Rect();
   paint.getTextBounds(deltaStr, 0, deltaStr.length(), dBounds);
   final float deltaY = h * 0.62f + dBounds.height() * 0.5f;
   cv.drawText(deltaStr, w * 0.5f, deltaY, paint);

   // ── elapsed-time row — bold, WHITE so it's always readable ───
   paint.setTextSize(timeSize);
   paint.setFakeBoldText(true);
   // Use the notification foreground color (white on dark) for maximum visibility
   paint.setColor(Notify.foregroundcolor != android.graphics.Color.BLACK
           ? Notify.foregroundcolor : Color.WHITE);
   final long elapsedMin = (System.currentTimeMillis() - glucose.time) / 60_000L;
   final String timeStr  = elapsedMin + " min";
   final Rect tBounds = new Rect();
   paint.getTextBounds(timeStr, 0, timeStr.length(), tBounds);
   final float timeY = h * 0.88f + tBounds.height() * 0.5f;
   cv.drawText(timeStr, w * 0.5f, timeY, paint);

   remoteViews.setImageViewBitmap(arrowandvalue, bmp);
   return remoteViews;
}

/**
 * Renders the last known glucose value with a horizontal strikethrough line,
 * used when the reading is stale (older than glucosetimeout).
 */
final RemoteViews staleRemote(notGlucose glucose) {
   RemoteViews remoteViews = new RemoteViews(Applic.app.getPackageName(), R.layout.arrowandvalue);
   if (glucose == null || glucose.value == null) {
       return remoteViews;
   }
   canvas.drawColor(Color.TRANSPARENT, PorterDuff.Mode.CLEAR);
   glucosePaint.setTextSize(glucosesize);
   glucosePaint.setColor(Color.GRAY);

   // Draw the value text
   final var gety = (canvas.getHeight() - timeHeight) * 0.98f;
   canvas.drawText(glucose.value, notglucosex, gety, glucosePaint);

   // Overlay a strikethrough line across the text bounds
   final float valwidth = glucosePaint.measureText(glucose.value);
   final float lineY = gety - glucosesize * 0.35f;
   final float strokeW = glucosePaint.getStrokeWidth();
   glucosePaint.setStrokeWidth(Math.max(3f, glucosesize * 0.06f));
   canvas.drawLine(notglucosex - glucosesize * 0.05f, lineY,
                   notglucosex + valwidth + glucosesize * 0.1f, lineY,
                   glucosePaint);
   glucosePaint.setStrokeWidth(strokeW);

   canvas.setBitmap(glucoseBitmap);
   remoteViews.setImageViewBitmap(arrowandvalue, glucoseBitmap);
   return remoteViews;
}
}
