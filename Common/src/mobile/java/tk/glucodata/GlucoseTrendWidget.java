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

package tk.glucodata;

import static tk.glucodata.Log.doLog;
import static tk.glucodata.Notify.glucosetimeout;
import static tk.glucodata.Notify.penmutable;

import android.app.PendingIntent;
import android.appwidget.AppWidgetManager;
import android.appwidget.AppWidgetProvider;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.os.Build;
import android.os.Bundle;
import android.view.View;
import android.widget.RemoteViews;

/**
 * Home-screen widget showing glucose value (auto-sized, range-coloured) and a trend arrow.
 * Equivalent to GDH's GlucoseTrendWidget.
 */
public class GlucoseTrendWidget extends AppWidgetProvider {

    private static final String LOG_ID = "GlucoseTrendWidget";
    static boolean used = true;

    /** Receives ACTION_TIME_TICK to refresh the widget display every minute. */
    private static BroadcastReceiver tickReceiver = null;

    private static int getLayout(AppWidgetManager mgr, int appWidgetId) {
        final Bundle options = mgr.getAppWidgetOptions(appWidgetId);
        final int minWidth = options.getInt(AppWidgetManager.OPTION_APPWIDGET_MIN_WIDTH);
        final int maxHeight = options.getInt(AppWidgetManager.OPTION_APPWIDGET_MAX_HEIGHT);
        final float ratio = maxHeight > 0 ? (minWidth / (float) maxHeight) : Float.MAX_VALUE;
        if (minWidth <= 110 || ratio < 0.5f) {
            return R.layout.glucose_trend_widget_short;
        }
        if (ratio > 2.8f) {
            return R.layout.glucose_trend_widget_long;
        }
        return R.layout.glucose_trend_widget;
    }

    private static void updateOne(AppWidgetManager mgr, int appWidgetId) {
        try {
            strGlucose glu;
            notGlucose ng = SuperGattCallback.previousglucose;
            if (ng == null) {
                glu = Natives.lastglucose();
                if (glu != null) {
                    ng = new notGlucose(glu.time * 1000L, glu.value, glu.rate, glu.sensorgen2);
                    SuperGattCallback.previousglucose = ng;
                }
            }

            final int layout = getLayout(mgr, appWidgetId);
            RemoteViews views = new RemoteViews(Applic.app.getPackageName(), layout);

            if (ng == null) {
                views.setTextViewText(R.id.gtw_glucose, "- -");
                views.setTextColor(R.id.gtw_glucose, Color.RED);
                views.setViewVisibility(R.id.gtw_arrow, View.GONE);
                Intent noDataIntent = new Intent(Applic.app, MainActivity.class);
                PendingIntent noDataPi = PendingIntent.getActivity(
                        Applic.app, 0, noDataIntent, PendingIntent.FLAG_UPDATE_CURRENT | penmutable);
                views.setOnClickPendingIntent(R.id.glucose_trend_widget_root, noDataPi);
                mgr.updateAppWidget(appWidgetId, views);
                return;
            }

            final boolean stale = (System.currentTimeMillis() - ng.time) > glucosetimeout;
            final int valueColor = stale
                    ? Color.GRAY
                    : RemoteGlucose.glucoseRangeColor(SuperGattCallback.previousglucosevalue);

            // Background tint (0 = transparent by default)
            final int bgAlpha = RemoteGlucose.getWidgetBgAlpha();
            if (bgAlpha > 0) {
                views.setInt(R.id.glucose_trend_widget_root, "setBackgroundColor",
                        Color.argb(bgAlpha, 0, 0, 0));
            } else {
                views.setInt(R.id.glucose_trend_widget_root, "setBackgroundColor",
                        Color.TRANSPARENT);
            }

            views.setTextViewText(R.id.gtw_glucose, ng.value);
            views.setTextColor(R.id.gtw_glucose, valueColor);
            if (stale) {
                views.setInt(R.id.gtw_glucose, "setPaintFlags",
                        android.graphics.Paint.STRIKE_THRU_TEXT_FLAG);
            } else {
                views.setInt(R.id.gtw_glucose, "setPaintFlags", 0);
            }

            final float rate = ng.rate;
            if (!Float.isNaN(rate)) {
                views.setImageViewBitmap(R.id.gtw_arrow, buildArrowBitmap(rate, valueColor));
                views.setViewVisibility(R.id.gtw_arrow, View.VISIBLE);
            } else {
                views.setViewVisibility(R.id.gtw_arrow, View.GONE);
            }

            Intent intent = new Intent(Applic.app, MainActivity.class);
            PendingIntent pi = PendingIntent.getActivity(
                    Applic.app, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT | penmutable);
            views.setOnClickPendingIntent(R.id.glucose_trend_widget_root, pi);

            mgr.updateAppWidget(appWidgetId, views);
        } catch (Throwable th) {
            Log.stack(LOG_ID, "updateOne", th);
        }
    }

    private static Bitmap buildArrowBitmap(float rate, int color) {
        final int size = 128;
        final Bitmap bmp = Bitmap.createBitmap(size, size, Bitmap.Config.ARGB_8888);
        final Canvas cv = new Canvas(bmp);
        cv.drawColor(Color.TRANSPARENT, PorterDuff.Mode.CLEAR);
        final Paint p = new Paint();
        p.setAntiAlias(true);
        p.setColor(color);
        p.setStrokeWidth(4f);
        final float density = size / 54f;
        final float cx = size * 0.65f;
        final float cy = size * 0.50f;
        CommonCanvas.drawarrow(cv, p, density, rate, cx, cy);
        return bmp;
    }

    private static void updateAll(AppWidgetManager mgr, int[] ids) {
        if (ids == null || ids.length == 0) return;
        used = true;
        for (int id : ids) updateOne(mgr, id);
    }

    @Override
    public void onUpdate(Context ctx, AppWidgetManager mgr, int[] ids) {
        {if(doLog) {Log.i(LOG_ID, "onUpdate");};};
        updateAll(mgr, ids);
    }

    @Override
    public void onAppWidgetOptionsChanged(Context ctx, AppWidgetManager mgr, int id, Bundle opts) {
        {if(doLog) {Log.i(LOG_ID, "onAppWidgetOptionsChanged");};};
        used = true;
        updateOne(mgr, id);
    }

    @Override
    public void onEnabled(Context ctx) {
        used = true;
        registerTickReceiver(ctx);
    }

    @Override
    public void onDisabled(Context ctx) {
        used = false;
        unregisterTickReceiver(ctx);
    }

    private static void registerTickReceiver(Context ctx) {
        if (tickReceiver != null) return;
        tickReceiver = new BroadcastReceiver() {
            @Override public void onReceive(Context c, Intent i) { update(); }
        };
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            ctx.getApplicationContext().registerReceiver(tickReceiver,
                    new IntentFilter(Intent.ACTION_TIME_TICK),
                    Context.RECEIVER_NOT_EXPORTED);
        } else {
            ctx.getApplicationContext().registerReceiver(tickReceiver,
                    new IntentFilter(Intent.ACTION_TIME_TICK));
        }
    }

    private static void unregisterTickReceiver(Context ctx) {
        if (tickReceiver == null) return;
        try { ctx.getApplicationContext().unregisterReceiver(tickReceiver); }
        catch (Throwable ignored) {}
        tickReceiver = null;
    }

    public static void update() {
        if (!used) return;
        final var mgr = AppWidgetManager.getInstance(Applic.app);
        final int[] ids = mgr.getAppWidgetIds(
                new ComponentName(Applic.app, GlucoseTrendWidget.class));
        if (ids.length > 0) {
            updateAll(mgr, ids);
        } else {
            used = false;
        }
    }
}
