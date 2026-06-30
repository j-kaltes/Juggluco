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
 * Home-screen widget showing glucose value (auto-sized, range-coloured),
 * a trend arrow, Δ delta, and elapsed time.
 *
 * Uses native RemoteViews TextViews with autoSizeTextType="uniform" so the
 * glucose value always fills the available cell height — exactly like GDH's
 * GlucoseTrendDeltaTimeWidget.  The trend arrow is rendered as a small bitmap
 * via CommonCanvas.drawarrow() and set on an ImageView.
 */
public class GlucoseDeltaTimeWidget extends AppWidgetProvider {

    private static final String LOG_ID = "GlucoseDeltaTimeWidget";
    static boolean used = true;

    /** Receives ACTION_TIME_TICK to refresh the widget display every minute. */
    private static BroadcastReceiver tickReceiver = null;

    private static int getLayout(AppWidgetManager mgr, int appWidgetId) {
        final Bundle options = mgr.getAppWidgetOptions(appWidgetId);
        final int minWidth = options.getInt(AppWidgetManager.OPTION_APPWIDGET_MIN_WIDTH);
        final int maxHeight = options.getInt(AppWidgetManager.OPTION_APPWIDGET_MAX_HEIGHT);
        final float ratio = maxHeight > 0 ? (minWidth / (float) maxHeight) : Float.MAX_VALUE;
        if (minWidth <= 110 || ratio < 0.5f) {
            return R.layout.glucose_delta_widget_short;
        }
        if (ratio > 2.8f) {
            return R.layout.glucose_delta_widget_long;
        }
        return R.layout.glucose_delta_widget;
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

            if (ng == null) {
                RemoteViews views = new RemoteViews(Applic.app.getPackageName(), layout);
                views.setTextViewText(R.id.dtw_glucose, "- -");
                views.setTextColor(R.id.dtw_glucose, Color.RED);
                views.setTextViewText(R.id.dtw_delta, "");
                views.setTextViewText(R.id.dtw_time, "");
                Intent noDataIntent = new Intent(Applic.app, MainActivity.class);
                PendingIntent noDataPi = PendingIntent.getActivity(
                        Applic.app, 0, noDataIntent, PendingIntent.FLAG_UPDATE_CURRENT | penmutable);
                views.setOnClickPendingIntent(R.id.glucose_delta_widget_root, noDataPi);
                mgr.updateAppWidget(appWidgetId, views);
                return;
            }

            final boolean stale = (System.currentTimeMillis() - ng.time) > glucosetimeout;
            final int valueColor = stale
                    ? Color.GRAY
                    : RemoteGlucose.glucoseRangeColor(SuperGattCallback.previousglucosevalue);
            // Time/delta use the system foreground color (white on dark, black on light)
            // — same as GDH's transparent_widget_textcolor
            final int textColor = Notify.foregroundcolor != Color.BLACK
                    ? Notify.foregroundcolor : Color.WHITE;

            RemoteViews views = new RemoteViews(Applic.app.getPackageName(), layout);

            // Background tint (0 = transparent by default)
            final int bgAlpha = RemoteGlucose.getWidgetBgAlpha();
            if (bgAlpha > 0) {
                views.setInt(R.id.glucose_delta_widget_root, "setBackgroundColor",
                        Color.argb(bgAlpha, 0, 0, 0));
            } else {
                views.setInt(R.id.glucose_delta_widget_root, "setBackgroundColor",
                        Color.TRANSPARENT);
            }

            // ── glucose value ──────────────────────────────────────────────
            views.setTextViewText(R.id.dtw_glucose, ng.value);
            views.setTextColor(R.id.dtw_glucose, valueColor);
            if (stale) {
                views.setInt(R.id.dtw_glucose, "setPaintFlags",
                        android.graphics.Paint.STRIKE_THRU_TEXT_FLAG);
            } else {
                views.setInt(R.id.dtw_glucose, "setPaintFlags", 0);
            }

            // ── trend arrow bitmap ─────────────────────────────────────────
            final float rate = ng.rate;
            if (!Float.isNaN(rate)) {
                final Bitmap arrowBmp = buildArrowBitmap(rate, valueColor);
                views.setImageViewBitmap(R.id.dtw_arrow, arrowBmp);
                views.setViewVisibility(R.id.dtw_arrow, View.VISIBLE);
            } else {
                views.setViewVisibility(R.id.dtw_arrow, View.GONE);
            }

            // ── elapsed time (🕒 prefix like GDH) ─────────────────────────
            final long elapsedMin = (System.currentTimeMillis() - ng.time) / 60_000L;
            views.setTextViewText(R.id.dtw_time, "\uD83D\uDD52 " + elapsedMin + " min");
            views.setTextColor(R.id.dtw_time, textColor);

            // ── delta ──────────────────────────────────────────────────────
            final float delta5 = ng.rate * 5f;
            final String deltaStr = Float.isNaN(delta5) ? "\u0394 --"
                    : "\u0394 " + String.format(Applic.usedlocale,
                          Applic.unit == 1 ? "%+.1f" : "%+.0f", delta5);
            views.setTextViewText(R.id.dtw_delta, deltaStr);
            views.setTextColor(R.id.dtw_delta, valueColor);

            // ── tap opens app ──────────────────────────────────────────────
            Intent intent = new Intent(Applic.app, MainActivity.class);
            PendingIntent pi = PendingIntent.getActivity(
                    Applic.app, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT | penmutable);
            views.setOnClickPendingIntent(R.id.glucose_delta_widget_root, pi);

            mgr.updateAppWidget(appWidgetId, views);
        } catch (Throwable th) {
            Log.stack(LOG_ID, "updateOne", th);
        }
    }

    /**
     * Render the trend arrow into a square bitmap.
     * Size is 128×128 px — RemoteViews ImageView will scale it with fitCenter.
     */
    private static Bitmap buildArrowBitmap(float rate, int color) {
        final int size = 128;
        final Bitmap bmp = Bitmap.createBitmap(size, size, Bitmap.Config.ARGB_8888);
        final Canvas cv = new Canvas(bmp);
        cv.drawColor(Color.TRANSPARENT, PorterDuff.Mode.CLEAR);
        final Paint p = new Paint();
        p.setAntiAlias(true);
        p.setColor(color);
        p.setStrokeWidth(4f);
        // drawarrow density: arrow length ≈ 40% of size
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

    // ── AppWidgetProvider callbacks ───────────────────────────────────────────

    @Override
    public void onUpdate(Context ctx, AppWidgetManager mgr, int[] ids) {
        {if(doLog) {Log.i(LOG_ID, "onUpdate");};};
        updateAll(mgr, ids);
    }

    @Override
    public void onAppWidgetOptionsChanged(Context ctx, AppWidgetManager mgr,
                                          int id, Bundle opts) {
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

    // ── Called from GlucoseWidget.update() when new data arrives ─────────────

    public static void update() {
        if (!used) return;
        final var mgr = AppWidgetManager.getInstance(Applic.app);
        final int[] ids = mgr.getAppWidgetIds(
                new ComponentName(Applic.app, GlucoseDeltaTimeWidget.class));
        if (ids.length > 0) {
            updateAll(mgr, ids);
        } else {
            used = false;
        }
    }
}
