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

import android.app.WallpaperManager;
import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.os.Build;
import android.util.DisplayMetrics;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import java.io.IOException;

/**
 * Lock-screen overlay: renders a GDH-style card by inflating
 * {@link R.layout#lockscreen_wallpaper_view} into an off-screen View, then
 * measuring/drawing it to a bitmap that is placed on a full-screen transparent
 * canvas and set as the FLAG_LOCK wallpaper layer.
 *
 * Card layout (inspired by GDH wallpaper.xml):
 *   Row 1 (75 %): large glucose value (auto-sized, range-coloured)  |  trend arrow
 *   Row 2 (25 %): 🕒 N min  |  Δ +/-value
 *   Background   : semi-transparent dark rounded rectangle (#88000000)
 *
 * The bitmap outside the card is fully transparent so the user's wallpaper
 * photo (home-screen layer managed by the launcher) shows through.
 *
 * On disable: WallpaperManager.clear(FLAG_LOCK) reverts to the home-screen wallpaper.
 */
public class LockScreenWallpaper {

    private static final String LOG_ID = "LockScreenWallpaper";
    private static final String PREFS  = "juggluco_dev";
    private static final String KEY_WP = "lockscreen_wallpaper_enabled";

    /** Vertical position of the card centre, as % of screen height (default 50%). */
    private static final float CARD_Y_PERCENT = 0.50f;

    private static boolean enabled = false;

    // ── persistence ───────────────────────────────────────────────────────────

    private static SharedPreferences prefs() {
        return Applic.app.getSharedPreferences(PREFS, Context.MODE_PRIVATE);
    }

    public static boolean isEnabled() {
        return prefs().getBoolean(KEY_WP, false);
    }

    public static void setEnabled(boolean on) {
        if (on == enabled) return;
        enabled = on;
        prefs().edit().putBoolean(KEY_WP, on).apply();
        if (on) {
            update();
        } else {
            new Thread(LockScreenWallpaper::clearWallpaper).start();
        }
    }

    public static void restoreOnBoot() {
        enabled = isEnabled();
        if (enabled) update();
    }

    // ── public update API ─────────────────────────────────────────────────────

    public static void update() {
        if (!enabled) return;
        final notGlucose ng = SuperGattCallback.previousglucose;
        if (ng == null) return;
        new Thread(() -> applyWallpaper(ng)).start();
    }

    // ── rendering ─────────────────────────────────────────────────────────────

    private static void applyWallpaper(notGlucose ng) {
        try {
            final WallpaperManager wm = WallpaperManager.getInstance(Applic.app);
            final Bitmap bmp = buildBitmap(ng);
            if (bmp == null) return;
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
                wm.setBitmap(bmp, null, false, WallpaperManager.FLAG_LOCK);
            } else {
                wm.setBitmap(bmp);
            }
            bmp.recycle();
            if (doLog) Log.i(LOG_ID, "Lock-screen wallpaper updated: " + ng.value);
        } catch (IOException e) {
            Log.stack(LOG_ID, "applyWallpaper", e);
        } catch (Throwable t) {
            Log.stack(LOG_ID, "applyWallpaper", t);
        }
    }

    private static void clearWallpaper() {
        try {
            final WallpaperManager wm = WallpaperManager.getInstance(Applic.app);
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
                wm.clear(WallpaperManager.FLAG_LOCK);
            }
            if (doLog) Log.i(LOG_ID, "Lock-screen wallpaper cleared.");
        } catch (Throwable t) {
            Log.stack(LOG_ID, "clearWallpaper", t);
        }
    }

    /**
     * Inflates {@link R.layout#lockscreen_wallpaper_view}, populates it with
     * the current glucose reading, measures it, draws it to a Bitmap, then
     * composites that Bitmap onto a full-screen transparent canvas so the card
     * floats over the user's wallpaper photo.
     */
    private static Bitmap buildBitmap(notGlucose ng) {
        try {
            final Context ctx = Applic.app;
            final DisplayMetrics dm = ctx.getResources().getDisplayMetrics();
            final int screenW = dm.widthPixels;
            final int screenH = dm.heightPixels;

            // ── inflate the card layout ────────────────────────────────────────
            final View card = LayoutInflater.from(ctx)
                    .inflate(R.layout.lockscreen_wallpaper_view, null);

            final TextView tvGlucose  = card.findViewById(R.id.lswp_glucose);
            final ImageView ivArrow   = card.findViewById(R.id.lswp_arrow);
            final TextView tvTime     = card.findViewById(R.id.lswp_time);
            final TextView tvDelta    = card.findViewById(R.id.lswp_delta);
            final ImageView ivGraph   = card.findViewById(R.id.lswp_graph);

            // ── glucose value ──────────────────────────────────────────────────
            final boolean stale =
                    (System.currentTimeMillis() - ng.time) > Notify.glucosetimeout;
            final int valueColor = stale
                    ? Color.GRAY
                    : RemoteGlucose.glucoseRangeColor(SuperGattCallback.previousglucosevalue);

            tvGlucose.setText(ng.value);
            tvGlucose.setTextColor(valueColor);
            if (stale) {
                tvGlucose.setPaintFlags(
                        tvGlucose.getPaintFlags() | Paint.STRIKE_THRU_TEXT_FLAG);
            } else {
                tvGlucose.setPaintFlags(
                        tvGlucose.getPaintFlags() & ~Paint.STRIKE_THRU_TEXT_FLAG);
            }

            // ── trend arrow ────────────────────────────────────────────────────
            final float rate = ng.rate;
            if (!Float.isNaN(rate)) {
                ivArrow.setImageBitmap(buildArrowBitmap(rate, valueColor));
                ivArrow.setVisibility(View.VISIBLE);
            } else {
                ivArrow.setVisibility(View.GONE);
            }

            // ── elapsed time ───────────────────────────────────────────────────
            final long elapsedMin = (System.currentTimeMillis() - ng.time) / 60_000L;
            tvTime.setText("\uD83D\uDD52 " + elapsedMin + " min");
            tvTime.setTextColor(Color.WHITE);

            // ── delta ──────────────────────────────────────────────────────────
            final float delta5 = rate * 5f;
            final String deltaStr;
            if (Float.isNaN(delta5)) {
                deltaStr = "\u0394 --";
            } else {
                final String fmt = Applic.unit == 1 ? "%+.1f" : "%+.0f";
                deltaStr = "\u0394 " + String.format(Applic.usedlocale, fmt, delta5);
            }
            tvDelta.setText(deltaStr);
            tvDelta.setTextColor(valueColor);

            // ── graph ──────────────────────────────────────────────────────────
            if (ivGraph != null && Applic.Nativesloaded) {
                // Card is 500dp wide; use actual pixel width at current density.
                // Height of the graph row is weight-2 of total card height (480dp),
                // minus padding, minus the two text rows (~196dp combined).
                // We use a fixed 140dp height to keep the JNI call simple.
                final int gW = Math.round(500 * dm.density * 0.95f);
                final int gH = Math.round(140 * dm.density);
                if (gW >= 60 && gH >= 40) {
                    try {
                        final int[] pixels = Natives.getWidgetGraphBitmap(gW, gH, 3 * 3600);
                        if (pixels != null) {
                            final Bitmap graphBmp = Bitmap.createBitmap(
                                    pixels, gW, gH, Bitmap.Config.ARGB_8888);
                            ivGraph.setImageBitmap(graphBmp);
                            ivGraph.setVisibility(View.VISIBLE);
                        }
                    } catch (Throwable t) {
                        Log.stack(LOG_ID, "buildBitmap graph", t);
                    }
                }
            }

            // ── measure & lay out the card ────────────────────────────────────
            // Use a fixed card width = 80% of screen width, free height
            final int cardMaxW = (int) (screenW * 0.80f);
            card.measure(
                    View.MeasureSpec.makeMeasureSpec(cardMaxW, View.MeasureSpec.AT_MOST),
                    View.MeasureSpec.makeMeasureSpec(0, View.MeasureSpec.UNSPECIFIED));
            final int cardW = card.getMeasuredWidth();
            final int cardH = card.getMeasuredHeight();
            card.layout(0, 0, cardW, cardH);

            // Render card to its own bitmap
            final Bitmap cardBmp = Bitmap.createBitmap(cardW, cardH, Bitmap.Config.ARGB_8888);
            card.draw(new Canvas(cardBmp));

            // ── composite onto a full-screen transparent bitmap ────────────────
            // Card is centred horizontally; vertical centre at CARD_Y_PERCENT.
            final Bitmap wallpaper =
                    Bitmap.createBitmap(screenW, screenH, Bitmap.Config.ARGB_8888);
            final Canvas cv = new Canvas(wallpaper);
            final float xOff = (screenW - cardW) * 0.5f;
            final float yOff = Math.max(0f, screenH * CARD_Y_PERCENT - cardH * 0.5f);
            cv.drawBitmap(cardBmp, xOff, yOff, new Paint(Paint.ANTI_ALIAS_FLAG));
            cardBmp.recycle();

            return wallpaper;
        } catch (Throwable t) {
            Log.stack(LOG_ID, "buildBitmap", t);
            return null;
        }
    }

    /** Creates a small transparent Bitmap with the trend arrow drawn on it. */
    private static Bitmap buildArrowBitmap(float rate, int color) {
        final int size = 128;
        final Bitmap bmp = Bitmap.createBitmap(size, size, Bitmap.Config.ARGB_8888);
        final Canvas cv  = new Canvas(bmp);
        cv.drawColor(Color.TRANSPARENT, PorterDuff.Mode.CLEAR);
        final Paint p = new Paint();
        p.setAntiAlias(true);
        p.setColor(color);
        // density keeps the arrow proportional to GDH's 54 px baseline
        final float density = size / 54f;
        final float cx = size * 0.65f;
        final float cy = size * 0.50f;
        CommonCanvas.drawarrow(cv, p, density, rate, cx, cy);
        return bmp;
    }
}
