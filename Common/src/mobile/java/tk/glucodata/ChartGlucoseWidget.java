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
import android.util.DisplayMetrics;
import android.view.View;
import android.widget.RemoteViews;

/**
 * Home-screen widget showing:
 *
 *   ┌─────────────────────────────────────┐
 *   │  202  →    │  🕒 3 min  │  Δ +5    │  ← normal/long cell
 *   │─────────────────────────────────────│
 *   │   [  glucose history graph (3 h)  ] │
 *   └─────────────────────────────────────┘
 *
 * Layout variants selected by widget cell size:
 *   • short  (w ≤ 110 dp): value + arrow only (no graph, no time/delta)
 *   • normal (default)   : value | arrow | time | delta | graph
 *   • long   (ratio > 2) : horizontal value+arrow on left, graph on right
 *
 * The graph is rendered off-screen using the NanoVG-RT software backend
 * via {@link Natives#getWidgetGraphBitmap(int, int, int)}.
 *
 * Reuses the identical pattern (range colour, strikethrough, arrow bitmap,
 * tick receiver) from GlucoseDeltaTimeWidget.
 */
public class ChartGlucoseWidget extends AppWidgetProvider {

    private static final String LOG_ID = "ChartGlucoseWidget";

    /** Graph history window — 3 hours, matching GDH's ChartWidget default. */
    static final int GRAPH_DURATION_SECS = 3 * 3600;

    /**
     * Target graph height in dp inside the normal widget.
     * The ImageView uses weight=3 in a vertical LinearLayout whose total height
     * is ~250 dp on a 2-row cell; so ~150 dp is a good estimate.
     */
    private static final int GRAPH_HEIGHT_DP = 130;

    static boolean used = true;

    /** Receives ACTION_TIME_TICK to refresh the elapsed-time counter every minute. */
    private static BroadcastReceiver tickReceiver = null;

    // ── layout selection ──────────────────────────────────────────────────────

    /** Short: narrow cell (≤ 110 dp wide) — glucose + arrow only, no graph */
    static boolean isShort(int widthDp, int heightDp) {
        return widthDp <= 110 || heightDp <= 70;
    }

    /** Long: wide cell (aspect ratio > 2.0) — horizontal split, graph on right */
    static boolean isLong(int widthDp, int heightDp) {
        return heightDp > 0 && ((float) widthDp / heightDp) > 2.0f;
    }

    // ── core update ───────────────────────────────────────────────────────────

    private static void updateOne(AppWidgetManager mgr, int appWidgetId) {
        try {
            // ── gather glucose data ──────────────────────────────────────────
            notGlucose ng = SuperGattCallback.previousglucose;
            if (ng == null) {
                strGlucose glu = Natives.lastglucose();
                if (glu != null) {
                    ng = new notGlucose(glu.time * 1000L, glu.value, glu.rate, glu.sensorgen2);
                    SuperGattCallback.previousglucose = ng;
                }
            }

            // ── read widget cell size ────────────────────────────────────────
            final Bundle options = mgr.getAppWidgetOptions(appWidgetId);
            final int wDp = options.getInt(AppWidgetManager.OPTION_APPWIDGET_MIN_WIDTH, 150);
            final int hDp = options.getInt(AppWidgetManager.OPTION_APPWIDGET_MAX_HEIGHT, 200);
            final boolean shortWidget = isShort(wDp, hDp);
            final boolean longWidget  = !shortWidget && isLong(wDp, hDp);

            // ── pick layout ──────────────────────────────────────────────────
            final int layoutId;
            if (shortWidget) layoutId = R.layout.chart_glucose_widget_short;
            else if (longWidget) layoutId = R.layout.chart_glucose_widget_long;
            else layoutId = R.layout.chart_glucose_widget;

            final RemoteViews views = new RemoteViews(Applic.app.getPackageName(), layoutId);

            // ── tap opens app ────────────────────────────────────────────────
            final Intent tapIntent = new Intent(Applic.app, MainActivity.class);
            final PendingIntent tapPi = PendingIntent.getActivity(
                    Applic.app, 0, tapIntent, PendingIntent.FLAG_UPDATE_CURRENT | penmutable);
            views.setOnClickPendingIntent(R.id.cgw_root, tapPi);

            // ── background ───────────────────────────────────────────────────
            final int bgAlpha = RemoteGlucose.getWidgetBgAlpha();
            views.setInt(R.id.cgw_root, "setBackgroundColor",
                    bgAlpha > 0 ? Color.argb(bgAlpha, 0, 0, 0) : Color.TRANSPARENT);

            // ── no data ──────────────────────────────────────────────────────
            if (ng == null) {
                views.setTextViewText(R.id.cgw_glucose, "- -");
                views.setTextColor(R.id.cgw_glucose, Color.RED);
                views.setViewVisibility(R.id.cgw_arrow, View.GONE);
                if (!shortWidget) {
                    views.setTextViewText(R.id.cgw_time,  "");
                    views.setTextViewText(R.id.cgw_delta, "");
                    views.setViewVisibility(R.id.cgw_graph, View.GONE);
                }
                mgr.updateAppWidget(appWidgetId, views);
                return;
            }

            // ── colours ──────────────────────────────────────────────────────
            final boolean stale = (System.currentTimeMillis() - ng.time) > glucosetimeout;
            final int valueColor = stale
                    ? Color.GRAY
                    : RemoteGlucose.glucoseRangeColor(SuperGattCallback.previousglucosevalue);
            final int textColor = (Notify.foregroundcolor != Color.BLACK)
                    ? Notify.foregroundcolor : Color.WHITE;

            // ── glucose value ─────────────────────────────────────────────────
            views.setTextViewText(R.id.cgw_glucose, ng.value);
            views.setTextColor(R.id.cgw_glucose, valueColor);
            views.setInt(R.id.cgw_glucose, "setPaintFlags",
                    stale ? android.graphics.Paint.STRIKE_THRU_TEXT_FLAG : 0);

            // ── trend arrow ───────────────────────────────────────────────────
            final float rate = ng.rate;
            if (!Float.isNaN(rate) && !stale) {
                views.setImageViewBitmap(R.id.cgw_arrow, buildArrowBitmap(rate, valueColor));
                views.setViewVisibility(R.id.cgw_arrow, View.VISIBLE);
            } else {
                views.setViewVisibility(R.id.cgw_arrow, View.GONE);
            }

            // ── time + delta (not shown in short layout) ──────────────────────
            if (!shortWidget) {
                final long elapsedMin = (System.currentTimeMillis() - ng.time) / 60_000L;
                views.setTextViewText(R.id.cgw_time, "\uD83D\uDD52 " + elapsedMin + " min");
                views.setTextColor(R.id.cgw_time, textColor);

                final float delta5 = rate * 5f;
                final String deltaStr = Float.isNaN(delta5) ? "\u0394 --"
                        : "\u0394 " + String.format(Applic.usedlocale,
                              Applic.unit == 1 ? "%+.1f" : "%+.0f", delta5);
                views.setTextViewText(R.id.cgw_delta, deltaStr);
                views.setTextColor(R.id.cgw_delta, valueColor);

                // ── graph bitmap ──────────────────────────────────────────────
                final Bitmap graphBmp = buildGraphBitmap(wDp, hDp, longWidget);
                if (graphBmp != null) {
                    views.setImageViewBitmap(R.id.cgw_graph, graphBmp);
                    views.setViewVisibility(R.id.cgw_graph, View.VISIBLE);
                } else {
                    views.setViewVisibility(R.id.cgw_graph, View.GONE);
                }
            }

            mgr.updateAppWidget(appWidgetId, views);
        } catch (Throwable th) {
            Log.stack(LOG_ID, "updateOne", th);
        }
    }

    // ── graph rendering ───────────────────────────────────────────────────────

    /**
     * Render the glucose history graph off-screen using NanoVG-RT.
     *
     * <p>For the normal layout the graph takes roughly the bottom 55 % of the
     * widget height.  For the long layout it takes the full height on the right
     * half.  We use density-independent pixels from the AppWidgetManager options
     * and convert to pixels with the display density.
     *
     * <p>Returns null when the native library is not loaded or has no data.
     */
    private static Bitmap buildGraphBitmap(int widgetWDp, int widgetHDp, boolean longLayout) {
        if (!Applic.Nativesloaded) return null;
        final DisplayMetrics dm = Applic.app.getResources().getDisplayMetrics();
        final float density = dm.density;

        final int graphWPx, graphHPx;
        if (longLayout) {
            // Right half of the cell
            graphWPx = Math.round(widgetWDp * density * 0.55f);
            graphHPx = Math.round(widgetHDp * density * 0.90f);
        } else {
            // Bottom 55 % of the cell
            graphWPx = Math.round(widgetWDp  * density * 0.95f);
            graphHPx = Math.round(GRAPH_HEIGHT_DP * density);
        }

        if (graphWPx < 60 || graphHPx < 40) return null;

        try {
            final int[] pixels = Natives.getWidgetGraphBitmap(
                    graphWPx, graphHPx, GRAPH_DURATION_SECS);
            if (pixels == null) return null;
            return Bitmap.createBitmap(pixels, graphWPx, graphHPx, Bitmap.Config.ARGB_8888);
        } catch (Throwable th) {
            Log.stack(LOG_ID, "buildGraphBitmap", th);
            return null;
        }
    }

    // ── arrow bitmap (shared helper) ─────────────────────────────────────────

    private static Bitmap buildArrowBitmap(float rate, int color) {
        final int size = 128;
        final Bitmap bmp = Bitmap.createBitmap(size, size, Bitmap.Config.ARGB_8888);
        final Canvas cv = new Canvas(bmp);
        cv.drawColor(Color.TRANSPARENT, PorterDuff.Mode.CLEAR);
        final Paint p = new Paint();
        p.setAntiAlias(true);
        p.setColor(color);
        p.setStrokeWidth(4f);
        CommonCanvas.drawarrow(cv, p, size / 54f, rate, size * 0.65f, size * 0.50f);
        return bmp;
    }

    // ── batch update ──────────────────────────────────────────────────────────

    private static void updateAll(AppWidgetManager mgr, int[] ids) {
        if (ids == null || ids.length == 0) return;
        used = true;
        for (int id : ids) updateOne(mgr, id);
    }

    /** Called from GlucoseWidget.update() when a new sensor reading arrives. */
    public static void update() {
        if (!used) return;
        final AppWidgetManager mgr = AppWidgetManager.getInstance(Applic.app);
        final int[] ids = mgr.getAppWidgetIds(
                new ComponentName(Applic.app, ChartGlucoseWidget.class));
        if (ids.length > 0) {
            updateAll(mgr, ids);
        } else {
            used = false;
        }
    }

    // ── AppWidgetProvider callbacks ───────────────────────────────────────────

    @Override
    public void onUpdate(Context ctx, AppWidgetManager mgr, int[] ids) {
        {if(doLog) {Log.i(LOG_ID, "onUpdate ids=" + ids.length);};};
        updateAll(mgr, ids);
    }

    @Override
    public void onAppWidgetOptionsChanged(Context ctx, AppWidgetManager mgr,
                                          int id, Bundle opts) {
        {if(doLog) {Log.i(LOG_ID, "onAppWidgetOptionsChanged id=" + id);};};
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

    // ── tick receiver (refreshes elapsed time every minute) ───────────────────

    private static void registerTickReceiver(Context ctx) {
        if (tickReceiver != null) return;
        tickReceiver = new BroadcastReceiver() {
            @Override public void onReceive(Context c, Intent i) { update(); }
        };
        final IntentFilter filter = new IntentFilter(Intent.ACTION_TIME_TICK);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            ctx.getApplicationContext().registerReceiver(
                    tickReceiver, filter, Context.RECEIVER_NOT_EXPORTED);
        } else {
            ctx.getApplicationContext().registerReceiver(tickReceiver, filter);
        }
    }

    private static void unregisterTickReceiver(Context ctx) {
        if (tickReceiver == null) return;
        try { ctx.getApplicationContext().unregisterReceiver(tickReceiver); }
        catch (Throwable ignored) {}
        tickReceiver = null;
    }
}
