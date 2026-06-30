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

import android.app.Activity;
import android.app.NotificationManager;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.Paint;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.provider.Settings;
import android.util.DisplayMetrics;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;

/**
 * Full-screen activity that appears on the lock screen when a glucose alarm
 * fires.  Can be enabled/disabled from the Settings screen via
 * {@link #setEnabled(boolean)}.  State is persisted in SharedPreferences.
 *
 * Inspired by GlucoDataHandler's {@code LockscreenActivity.kt}.
 *
 * Features:
 *  - Dark charcoal background (#CC121212) matching Material dark surface
 *  - Large glucose value (range-coloured) + Unicode trend arrow beside it
 *  - Δ delta, 🕒 elapsed time, active-snooze status
 *  - Hold-to-dismiss button (press and hold 1.5 s) — prevents accidental dismissal
 *  - Snooze section — reveals snooze-duration buttons after tapping "Snooze"
 *    (configurable in Settings, default 60/90/120 min)
 *  - Auto-finishes after {@link #MAX_LIFETIME_MS} or when the alarm stops
 */
public class AlarmLockScreenActivity extends Activity {

    private static final String LOG_ID  = "AlarmLockScreenActivity";
    private static final String PREFS   = "juggluco_dev";
    private static final String KEY_ALS = "alarm_lockscreen_enabled";

    // ── enabled state (persisted) ─────────────────────────────────────────────

    private static SharedPreferences prefs() {
        return Applic.app.getSharedPreferences(PREFS, Context.MODE_PRIVATE);
    }

    /** Returns true when the alarm lock-screen activity is enabled (default: on). */
    public static boolean isEnabled() {
        return prefs().getBoolean(KEY_ALS, true);
    }

    /** Toggle from the Settings screen; persists immediately. */
    public static void setEnabled(boolean on) {
        prefs().edit().putBoolean(KEY_ALS, on).apply();
    }

    /**
     * Returns true when the app has permission to fire full-screen intents.
     * On Android 14+ (API 34) this requires explicit user approval.
     * On older versions it is always granted.
     */
    public static boolean hasFullScreenIntentPermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.UPSIDE_DOWN_CAKE) {
            final NotificationManager nm = (NotificationManager)
                    Applic.app.getSystemService(Context.NOTIFICATION_SERVICE);
            return nm != null && nm.canUseFullScreenIntent();
        }
        return true;
    }

    /**
     * Opens the system Settings page where the user can grant the
     * USE_FULL_SCREEN_INTENT permission (Android 14+ only).
     * Safe to call on older versions — it is a no-op there.
     */
    public static void requestFullScreenIntentPermission(Context context) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.UPSIDE_DOWN_CAKE) {
            try {
                final Intent intent = new Intent(
                        Settings.ACTION_MANAGE_APP_USE_FULL_SCREEN_INTENT,
                        Uri.parse("package:" + context.getPackageName()));
                intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                context.startActivity(intent);
            } catch (Throwable t) {
                Log.stack("AlarmLockScreenActivity", "requestFullScreenIntentPermission", t);
            }
        }
    }

    /** Intent extra: alarm kind integer (maps to {@link Notify#alarmtext(int)}). */
    public static final String EXTRA_ALARM_KIND = "alarm_kind";

    /** How long (ms) the activity stays open before auto-finishing. */
    private static final long MAX_LIFETIME_MS = 5 * 60 * 1000L;   // 5 minutes

    /**
     * How long (ms) the user must hold the dismiss button before the alarm stops.
     * Matching GDH's slide-to-act feel: long enough to be intentional.
     */
    private static final long HOLD_TO_DISMISS_MS = 1500L;

    /** Tick interval for the hold-to-dismiss progress animation (ms). */
    private static final long HOLD_TICK_MS = 30L;

    private long createTime;
    private int  alarmKind = -1;

    // ── hold-to-dismiss state ─────────────────────────────────────────────────
    private long     holdStartMs   = 0L;
    private boolean  holdActive    = false;
    private ProgressBar dismissProgress;
    private final Handler holdHandler = new Handler(Looper.getMainLooper());
    private final Runnable holdTicker = new Runnable() {
        @Override public void run() {
            if (!holdActive) return;
            final long elapsed = System.currentTimeMillis() - holdStartMs;
            final int pct = (int) Math.min(100, elapsed * 100 / HOLD_TO_DISMISS_MS);
            dismissProgress.setProgress(pct);
            if (elapsed >= HOLD_TO_DISMISS_MS) {
                holdActive = false;
                {if(doLog) {Log.i(LOG_ID, "hold-to-dismiss completed");}}
                Notify.stopalarm();
                finish();
            } else {
                holdHandler.postDelayed(this, HOLD_TICK_MS);
            }
        }
    };

    /** Handler used for the 1-minute live-refresh loop. */
    private final Handler refreshHandler = new Handler(Looper.getMainLooper());
    private final Runnable refreshRunnable = new Runnable() {
        @Override public void run() {
            if (currentInstance != AlarmLockScreenActivity.this) return;
            updateDisplay();
            if (!Natives.getisalarm()) {
                {if(doLog) {Log.i(LOG_ID, "refreshRunnable: alarm stopped, finishing");}}
                finish();
                return;
            }
            refreshHandler.postDelayed(this, 60_000L);
        }
    };

    /** Allows {@link Notify#stopalarm()} to close this activity remotely. */
    private static AlarmLockScreenActivity currentInstance = null;

    /** Close from outside (e.g. when alarm stop is triggered via notification). */
    public static void close() {
        final var act = currentInstance;
        if (act != null) {
            act.runOnUiThread(() -> {
                try { act.finish(); } catch (Throwable ignored) {}
            });
        }
    }

    public static boolean isActive() {
        return currentInstance != null;
    }

    // ── lifecycle ─────────────────────────────────────────────────────────────

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        try {
            {if(doLog) {Log.i(LOG_ID, "onCreate");}}
            currentInstance = this;
            showWhenLockedAndTurnScreenOn();
            super.onCreate(savedInstanceState);
            setContentView(R.layout.activity_alarm_lockscreen);
            hideSystemUI();

            createTime = System.currentTimeMillis();

            if (getIntent() != null && getIntent().hasExtra(EXTRA_ALARM_KIND)) {
                alarmKind = getIntent().getIntExtra(EXTRA_ALARM_KIND, -1);
            }

            dismissProgress = findViewById(R.id.lockscreen_dismiss_progress);
            updateDisplay();
            wireButtons();
            // Start 1-minute live-refresh loop so glucose value and elapsed time stay current
            refreshHandler.postDelayed(refreshRunnable, 60_000L);

        } catch (Throwable t) {
            Log.stack(LOG_ID, "onCreate", t);
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        updateDisplay();
        // Auto-finish if the alarm was already stopped while we were paused
        if (!Natives.getisalarm()) {
            {if(doLog) {Log.i(LOG_ID, "onResume: alarm no longer active, finishing");}}
            finish();
        }
    }

    @Override
    protected void onDestroy() {
        holdHandler.removeCallbacks(holdTicker);
        refreshHandler.removeCallbacks(refreshRunnable);
        if (currentInstance == this) {
            currentInstance = null;
        }
        super.onDestroy();
    }

    // ── UI helpers ────────────────────────────────────────────────────────────

    private void updateDisplay() {
        try {
            final var ng = SuperGattCallback.previousglucose;

            // Alarm label
            final TextView lblAlarm = findViewById(R.id.lockscreen_alarm_label);
            if (alarmKind >= 0) {
                lblAlarm.setText("\u26A0 " + Notify.alarmtext(alarmKind));
            }

            if (ng == null) return;

            // Glucose value — range-coloured or gray when stale
            final boolean stale =
                (System.currentTimeMillis() - ng.time) > Notify.glucosetimeout;
            final int valueColor = stale ? android.graphics.Color.LTGRAY
                : RemoteGlucose.glucoseRangeColor(SuperGattCallback.previousglucosevalue);

            final TextView tvValue = findViewById(R.id.lockscreen_glucose_value);
            tvValue.setText(ng.value);
            tvValue.setTextColor(valueColor);
            if (stale) {
                tvValue.setPaintFlags(tvValue.getPaintFlags() | Paint.STRIKE_THRU_TEXT_FLAG);
            } else {
                tvValue.setPaintFlags(tvValue.getPaintFlags() & ~Paint.STRIKE_THRU_TEXT_FLAG);
            }

            // Trend arrow — Unicode character matching Juggluco's rate thresholds
            final TextView tvArrow = findViewById(R.id.lockscreen_trend_arrow);
            tvArrow.setText(rateToArrow(ng.rate));
            tvArrow.setTextColor(valueColor);

            // Δ delta
            final float delta5 = ng.rate * 5f;
            final String deltaStr;
            if (Float.isNaN(delta5)) {
                deltaStr = "\u0394 --";
            } else {
                final String fmt = Applic.unit == 1 ? "%+.1f" : "%+.0f";
                deltaStr = "\u0394 " + String.format(Applic.usedlocale, fmt, delta5);
            }
            final TextView tvDelta = findViewById(R.id.lockscreen_delta);
            tvDelta.setText(deltaStr);
            tvDelta.setTextColor(valueColor);

            // Elapsed time with clock emoji (matches GDH)
            final long elapsedMin = (System.currentTimeMillis() - ng.time) / 60_000L;
            final TextView tvTime = findViewById(R.id.lockscreen_time);
            tvTime.setText("\uD83D\uDD52 " + elapsedMin + " min");

            // Active snooze label
            final TextView tvSnoozeStatus = findViewById(R.id.lockscreen_snooze_status);
            if (AlarmSnooze.isActive()) {
                tvSnoozeStatus.setVisibility(View.VISIBLE);
                tvSnoozeStatus.setText(
                    getString(R.string.lockscreen_snoozed_until) + " " + AlarmSnooze.snoozeUntilText());
            } else {
                tvSnoozeStatus.setVisibility(View.GONE);
            }

            // 3-hour glucose history graph
            final ImageView ivGraph = findViewById(R.id.lockscreen_graph);
            if (ivGraph != null) {
                if (Applic.Nativesloaded) {
                    final Bitmap graphBmp = buildGraphBitmap();
                    if (graphBmp != null) {
                        ivGraph.setImageBitmap(graphBmp);
                        ivGraph.setVisibility(View.VISIBLE);
                    } else {
                        ivGraph.setVisibility(View.GONE);
                    }
                } else {
                    ivGraph.setVisibility(View.GONE);
                }
            }

            // Auto-finish after MAX_LIFETIME_MS
            if ((System.currentTimeMillis() - createTime) >= MAX_LIFETIME_MS) {
                finish();
            }
        } catch (Throwable t) {
            Log.stack(LOG_ID, "updateDisplay", t);
        }
    }

    /**
     * Convert glucose rate (mmol/L per min or mg/dL per min) to a Unicode direction arrow.
     * Thresholds mirror Juggluco's own {@code RemoteGlucose.arrowremote()} logic:
     *  rate > 1.6  → steep up    (↑↑)
     *  rate > 0.6  → up          (↑)
     *  rate > 0.2  → slight up   (↗)
     *  rate > -0.2 → flat        (→)
     *  rate > -0.6 → slight down (↘)
     *  rate > -1.6 → down        (↓)
     *  rate ≤ -1.6 → steep down  (↓↓)
     *  NaN         → ?
     */
    private static String rateToArrow(float rate) {
        if (Float.isNaN(rate)) return "?";
        if (rate >  1.6f) return "\u21C8"; // ⇈
        if (rate >  0.6f) return "\u2191"; // ↑
        if (rate >  0.2f) return "\u2197"; // ↗
        if (rate > -0.2f) return "\u2192"; // →
        if (rate > -0.6f) return "\u2198"; // ↘
        if (rate > -1.6f) return "\u2193"; // ↓
        return "\u21CA";                    // ⇊
    }

    /**
     * Renders a 3-hour glucose history graph using the NanoVG-RT JNI backend.
     * Width is the full display width; height is 180 dp.
     * Returns null if the native library is not loaded or the JNI call fails.
     */
    private Bitmap buildGraphBitmap() {
        try {
            final DisplayMetrics dm = getResources().getDisplayMetrics();
            final int wPx = dm.widthPixels;
            final int hPx = Math.round(180 * dm.density);
            if (wPx < 60 || hPx < 40) return null;
            final int[] pixels = Natives.getWidgetGraphBitmap(wPx, hPx, 3 * 3600);
            if (pixels == null) return null;
            return Bitmap.createBitmap(pixels, wPx, hPx, Bitmap.Config.ARGB_8888);
        } catch (Throwable t) {
            Log.stack(LOG_ID, "buildGraphBitmap", t);
            return null;
        }
    }

    @SuppressWarnings("ClickableViewAccessibility")
    private void wireButtons() {
        final Button btnDismiss = findViewById(R.id.lockscreen_dismiss_btn);
        final Button btnSnoozeReveal = findViewById(R.id.lockscreen_snooze_reveal_btn);
        final LinearLayout snoozeLayout = findViewById(R.id.lockscreen_snooze_layout);

        // Label the Dismiss button with the re-fire interval and the hold instruction
        final String dismissLabel;
        if (alarmKind >= 0) {
            final int reFireMin = Natives.readalarmsuspension(alarmKind);
            dismissLabel = getString(R.string.lockscreen_dismiss_hold)
                    + (reFireMin > 0 ? " (+" + reFireMin + " min)" : "");
        } else {
            dismissLabel = getString(R.string.lockscreen_dismiss_hold);
        }
        btnDismiss.setText(dismissLabel);

        // Hold-to-dismiss: start countdown on ACTION_DOWN, cancel on ACTION_UP/CANCEL
        btnDismiss.setOnTouchListener((v, event) -> {
            switch (event.getAction()) {
                case MotionEvent.ACTION_DOWN:
                    {if(doLog) {Log.i(LOG_ID, "dismiss hold started");}}
                    holdStartMs = System.currentTimeMillis();
                    holdActive  = true;
                    dismissProgress.setProgress(0);
                    holdHandler.post(holdTicker);
                    return true;
                case MotionEvent.ACTION_UP:
                case MotionEvent.ACTION_CANCEL:
                    if (holdActive) {
                        {if(doLog) {Log.i(LOG_ID, "dismiss hold cancelled");}}
                        holdActive = false;
                        holdHandler.removeCallbacks(holdTicker);
                        dismissProgress.setProgress(0);
                    }
                    return true;
            }
            return false;
        });

        // Snooze — reveal the duration buttons
        btnSnoozeReveal.setOnClickListener(v -> {
            {if(doLog) {Log.i(LOG_ID, "snooze reveal tapped");}}
            snoozeLayout.setVisibility(View.VISIBLE);
            btnSnoozeReveal.setVisibility(View.GONE);
        });

        // Snooze duration buttons — populated dynamically from user's selection
        final int[] btnIds = {
            R.id.lockscreen_snooze_btn1,
            R.id.lockscreen_snooze_btn2,
            R.id.lockscreen_snooze_btn3
        };
        final java.util.List<Long> snoozeValues = AlarmSnooze.getSnoozeButtons();
        for (int i = 0; i < btnIds.length; i++) {
            final Button btn = findViewById(btnIds[i]);
            if (btn == null) continue;
            if (i < snoozeValues.size()) {
                final long minutes = snoozeValues.get(i);
                btn.setText(String.valueOf(minutes));
                btn.setVisibility(View.VISIBLE);
                btn.setOnClickListener(v -> {
                    {if(doLog) {Log.i(LOG_ID, "snooze " + minutes + " min tapped");}}
                    AlarmSnooze.set(minutes);
                    Notify.stopalarm();
                    finish();
                });
            } else {
                btn.setVisibility(View.GONE);
            }
        }
    }

    // ── window flags ──────────────────────────────────────────────────────────

    @SuppressWarnings("deprecation")
    private void showWhenLockedAndTurnScreenOn() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O_MR1) {
            setShowWhenLocked(true);
            setTurnScreenOn(true);
        }
        getWindow().addFlags(
            WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON
            | WindowManager.LayoutParams.FLAG_ALLOW_LOCK_WHILE_SCREEN_ON
            | WindowManager.LayoutParams.FLAG_SHOW_WHEN_LOCKED
            | WindowManager.LayoutParams.FLAG_TURN_SCREEN_ON
            | WindowManager.LayoutParams.FLAG_DISMISS_KEYGUARD
        );
    }

    @SuppressWarnings("deprecation")
    private void hideSystemUI() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            final var wic = getWindow().getInsetsController();
            if (wic != null) {
                wic.hide(WindowInsets.Type.statusBars() | WindowInsets.Type.navigationBars());
                wic.setSystemBarsBehavior(
                    WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
            }
        } else {
            getWindow().getDecorView().setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_FULLSCREEN
                | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
            );
        }
    }
}
