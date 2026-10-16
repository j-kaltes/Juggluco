/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2(+), Libre 3(+), Dexcom G7/ONE+,        */
/*      Sibionics GS1Sb and GS3, Accu-Chek SmartGuide, CareSens Air and              */
/*      Aidex X sensors.                                                             */
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
/*      Thu Oct 15 12:48:42 CEST 2026                                                */

package tk.glucodata;

import android.app.Activity;
import android.app.Application;
import android.app.Dialog;
import android.content.Context;
import android.graphics.Color;
import android.graphics.Rect;
import android.graphics.drawable.ColorDrawable;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import android.util.Log;
import android.util.TypedValue;
import android.view.ContextThemeWrapper;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.concurrent.atomic.AtomicLong;

/**
 * Permission-free messages. Android 5.0+, Java 8, compileSdk 29+.
 *
 * Timed messages use real Toasts with the application context, never Dialogs.
 * A custom TextView is used when custom Toasts are permitted. On Android 11+
 * with targetSdk 30+, calls without a known resumed Activity use makeText()
 * immediately instead. That fallback retains Android's text-Toast limits.
 * Custom Toasts are deprecated, non-interactive, and may still be blocked by
 * Android if the application loses foreground status before actual display.
 *
 * Call init(this) early in Application.onCreate(). The callbacks only select
 * the permitted renderer and the acknowledgement host. They never replay or
 * release pending messages. Without early init, the conservative fallback is
 * an ordinary Toast until a resumed Activity is observed.
 *
 * There is no application-managed message queue. A newer timed call cancels
 * the previous timed Toast. Calls more than MAX_START_DELAY_MS old on arrival
 * at the main thread are dropped. Android still owns its system Toast queue,
 * rate limits and display timing; this class cannot guarantee instant display.
 *
 * Acknowledgements need a currently resumed Activity. They are compact, modal,
 * scrollable Dialogs, not Toasts. A missing host is reported via OnShowFailure
 * or Log.e (default); no background Activity is launched and nothing is queued.
 * Further acknowledgement messages prepend to the displayed panel; OK closes
 * all of them. Leaving the host removes the panel without saving it for later.
 * All public message methods accept an application Context and any thread.
 */
public final class FullToasts {
    private static final String TAG = "FullToasts";
    private static final int MAX_WIDTH_DP = 480;
    private static final int HORIZONTAL_PADDING_DP = 12;
    private static final int VERTICAL_PADDING_DP = 8;
    private static final long MAX_START_DELAY_MS = 3000;

    private static final AtomicLong REQUEST = new AtomicLong();

    // Main-thread-only state. Weak references do not retain Activities.
    private static final ArrayList<WeakReference<Activity>> RESUMED = new ArrayList<>();
    private static Toast lastToast;
    private static Dialog acknowledgement;
    private static WeakReference<Activity> acknowledgementHost = new WeakReference<>(null);
    private static String acknowledgementText;

    private FullToasts() {}

    /** Call once, before Activities start. Repeated calls are harmless. */

    /** Drop-in timed entry point. No overlay permission and no Activity token. */
    public static void argToaster(Context context, String message, int duration) {
        Context app = applicationContext(context);
        if (message == null || message.isEmpty()) return;
        long generatedAt = SystemClock.elapsedRealtime();
        long request = REQUEST.incrementAndGet();
        Applic.RunOnUiThread(() -> {
            if (request != REQUEST.get()) return;
            cancelLastToast();
            if (SystemClock.elapsedRealtime() - generatedAt > MAX_START_DELAY_MS) return;
            int length = duration == Toast.LENGTH_LONG ? Toast.LENGTH_LONG : Toast.LENGTH_SHORT;
            boolean customAllowed = Build.VERSION.SDK_INT < 30 || app.getApplicationInfo().targetSdkVersion < 30 || Applic.getActivity() != null;
            Toast next;
            try {
                next = customAllowed ? makeCustomToast(app, message, length)
                        : Toast.makeText(app, message, length);
            } catch (RuntimeException error) {
                // A custom-view construction failure should not swallow feedback.
                Log.w(TAG, "Using ordinary Toast after view construction failure", error);
                next = Toast.makeText(app, message, length);
            }
            // A worker may have submitted a newer message during construction.
            if (request != REQUEST.get()
                    || SystemClock.elapsedRealtime() - generatedAt > MAX_START_DELAY_MS) return;
            lastToast = next;
            next.show();
        });
    }

    public static void argToasterWait(Context context, String message) {
        argToasterWait(context, message, null);
       }

    public static void argToasterWait(Context context, String message, OnShowFailure onFailure) {
        if (message == null || message.isEmpty()) return;
        long generatedAt = SystemClock.elapsedRealtime();
        Applic.RunOnUiThread(() -> {
            RuntimeException failure = null;
            try {
                if (SystemClock.elapsedRealtime() - generatedAt > MAX_START_DELAY_MS) {
                    throw new IllegalStateException("Acknowledgement request is stale");
                }
                Activity activity = Applic.getActivity();
                if (activity == null) {
                    throw new IllegalStateException(
                            "Acknowledgement requires a resumed Activity; message was not queued");
                }
                String combined = acknowledgement != null && acknowledgementText != null
                        ? message + "\n\n" + acknowledgementText : message;
                dismissAcknowledgementOnMain();
                showAcknowledgement(activity, combined);
            } catch (RuntimeException error) {
                dismissAcknowledgementOnMain();
                failure = error;
            }
            if (failure != null) {
                Log.e(TAG, "Acknowledgement could not be shown", failure);
                if (onFailure != null) onFailure.onFailure(failure);
            }
        });
    }

    public interface OnShowFailure {
        void onFailure(RuntimeException error);
    }

    public static void cancelTimed() {
        long request = REQUEST.incrementAndGet();
        Applic.RunOnUiThread(() -> {
            if (request == REQUEST.get()) cancelLastToast();
        });
    }

    public static void dismissAcknowledgements() {
        Applic.RunOnUiThread(FullToasts::dismissAcknowledgementOnMain);
    }

    private static void cancelLastToast() {
        if (lastToast != null) {
            lastToast.cancel();
            lastToast = null;
        }
    }

    @SuppressWarnings("deprecation")
    private static Toast makeCustomToast(Context app, String message, int duration) {
        Context ui = new ContextThemeWrapper(app, android.R.style.Theme_Material_Light);
        TextView text = messageView(ui, message, false);
        int maxWidth = Math.max(1, Math.min(dp(ui, MAX_WIDTH_DP),
                ui.getResources().getDisplayMetrics().widthPixels - dp(ui, 32)));
        text.setMaxWidth(maxWidth);
        text.setLayoutParams(new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        Toast toast = new Toast(app);
        toast.setView(text);
        toast.setDuration(duration);
        return toast;
    }

    private static TextView messageView(Context context, String message, boolean selectable) {
        TextView text = new TextView(context, null, 0);
        text.setTextColor(Color.BLACK);
        text.setBackgroundColor(Color.WHITE);
        text.setTextSize(TypedValue.COMPLEX_UNIT_SP, 16);
        text.setGravity(Gravity.TOP | Gravity.START);
        text.setTextIsSelectable(selectable);
        text.setSingleLine(false);
        text.setHorizontallyScrolling(false);
        text.setMaxLines(Integer.MAX_VALUE);
        text.setEllipsize(null);
        text.setMinWidth(0);
        text.setMinimumWidth(0);
        text.setMinHeight(0);
        text.setMinimumHeight(0);
        text.setIncludeFontPadding(true);
        text.setPadding(dp(context, HORIZONTAL_PADDING_DP), dp(context, VERTICAL_PADDING_DP),
                dp(context, HORIZONTAL_PADDING_DP), dp(context, VERTICAL_PADDING_DP));
        if (Build.VERSION.SDK_INT >= 29) text.setForceDarkAllowed(false);
        text.setText(message);
        return text;
    }

private static int desiredTextWidth(TextView text, String message) {
    float widest = 0;
    int start = 0;
    int length = message.length();

    for (int i = 0; i <= length; ++i) {
        if (i == length || message.charAt(i) == '\n') {
            widest = Math.max(widest, text.getPaint().measureText(message, start, i));
            start = i + 1;
            }
        }

    return (int) Math.ceil(widest) + text.getPaddingLeft() + text.getPaddingRight();
   }
    private static void showAcknowledgement(Activity activity, String message) {
        Dialog dialog = new Dialog(activity, android.R.style.Theme_Material_Light_Dialog_NoActionBar);
        dialog.requestWindowFeature(Window.FEATURE_NO_TITLE);
        dialog.setCancelable(false);
        dialog.setCanceledOnTouchOutside(false);
        Context ui = dialog.getContext();
        int[] usable = usableWindowSize(activity);
        boolean landscape = usable[0] > usable[1];
        int maxWidth = Math.max(1, landscape
                ? Math.round(usable[0] * 0.75f)
                : usable[0]);
        int maxHeight = Math.max(1, usable[1] - dp(ui, 64));

        TextView text = messageView(ui, message, true);
        Button ok = new Button(ui, null, android.R.attr.borderlessButtonStyle);
        ok.setText(android.R.string.ok);
        ok.setTextColor(Color.BLACK);
        ok.setTextSize(TypedValue.COMPLEX_UNIT_SP, 16);
        ok.setAllCaps(false);
        ok.setPadding(dp(ui, 12), dp(ui, 8), dp(ui, 12), dp(ui, 8));
        ok.setMinWidth(dp(ui, 48));
        ok.setMinimumWidth(dp(ui, 48));
        ok.setMinHeight(dp(ui, 48));
        ok.setMinimumHeight(dp(ui, 48));
        ok.measure(View.MeasureSpec.makeMeasureSpec(maxWidth, View.MeasureSpec.AT_MOST),
                View.MeasureSpec.makeMeasureSpec(0, View.MeasureSpec.UNSPECIFIED));
        ok.setOnClickListener(v -> dismissIfCurrent(dialog));

        int naturalTextWidth = desiredTextWidth(text, message);
        int width = Math.max(ok.getMeasuredWidth(), Math.min(maxWidth, naturalTextWidth));

        int maxMessageHeight = Math.max(1, Math.min(Math.round(usable[1] * 0.65f), maxHeight - ok.getMeasuredHeight()));
        LimitedScrollView scroll = new LimitedScrollView(ui, maxMessageHeight);
        scroll.setFillViewport(false);
        scroll.setHorizontalScrollBarEnabled(false);
        scroll.setVerticalScrollBarEnabled(true);
        scroll.setScrollBarStyle(View.SCROLLBARS_INSIDE_OVERLAY);
        scroll.addView(text, new ScrollView.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        LinearLayout content = new LinearLayout(ui, null, 0);
        content.setOrientation(LinearLayout.VERTICAL);
        content.setBackgroundColor(Color.WHITE);
        content.addView(scroll, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        LinearLayout.LayoutParams bp = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        bp.gravity = Gravity.END;
        content.addView(ok, bp);
        if (Build.VERSION.SDK_INT >= 29) content.setForceDarkAllowed(false);
        dialog.setContentView(content, new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        content.measure(View.MeasureSpec.makeMeasureSpec(width, View.MeasureSpec.EXACTLY),
                View.MeasureSpec.makeMeasureSpec(maxHeight, View.MeasureSpec.AT_MOST));
        Window window = dialog.getWindow();
        if (window != null) {
            window.setBackgroundDrawable(new ColorDrawable(Color.WHITE));
            View decor = window.getDecorView();
            decor.setPadding(0, 0, 0, 0);
            decor.setMinimumHeight(0);
            if (Build.VERSION.SDK_INT >= 29) decor.setForceDarkAllowed(false);
            window.setLayout(width, WindowManager.LayoutParams.WRAP_CONTENT);
           }
        dialog.setOnKeyListener((d, code, event) -> {
            if (code != KeyEvent.KEYCODE_ENTER && code != KeyEvent.KEYCODE_NUMPAD_ENTER
                    && code != KeyEvent.KEYCODE_DPAD_CENTER) return false;
            if (event.getAction() == KeyEvent.ACTION_UP && !event.isCanceled()) {
                dismissIfCurrent(dialog);
            }
            return true;
        });
        acknowledgement = dialog;
        acknowledgementText = message;
        acknowledgementHost = new WeakReference<>(activity);
        dialog.setOnDismissListener(d -> {
            if (acknowledgement == dialog) clearAcknowledgement();
        });
        dialog.show();
        if (window != null) window.setLayout(width, WindowManager.LayoutParams.WRAP_CONTENT);
    }



    private static void dismissIfCurrent(Dialog dialog) {
        if (acknowledgement == dialog) dismissAcknowledgementOnMain();
    }

    private static void clearAcknowledgement() {
        acknowledgement = null;
        acknowledgementText = null;
        acknowledgementHost.clear();
    }

    private static void dismissAcknowledgementOnMain() {
        Dialog dialog = acknowledgement;
        clearAcknowledgement();
        if (dialog == null) return;
        dialog.setOnDismissListener(null);
        try {
            dialog.dismiss();
        } catch (IllegalArgumentException error) {
            Log.w(TAG, "Acknowledgement window already removed", error);
        }
    }




    private static Context applicationContext(Context context) {
        if (context == null) throw new NullPointerException("context");
        Context app = context.getApplicationContext();
        if (app == null) throw new IllegalArgumentException("No application context");
        return app;
    }


    private static int dp(Context context, int value) {
        return Math.round(value * context.getResources().getDisplayMetrics().density);
    }

    private static int[] usableWindowSize(Activity activity) {
        View decor = activity.getWindow().getDecorView();
        Rect frame = new Rect();
        decor.getWindowVisibleDisplayFrame(frame);
        int width = decor.getWidth();
        int height = decor.getHeight();
        if (frame.width() > 0) width = width > 0 ? Math.min(width, frame.width()) : frame.width();
        if (frame.height() > 0) height = height > 0 ? Math.min(height, frame.height()) : frame.height();
        if (width <= 0) width = activity.getResources().getDisplayMetrics().widthPixels;
        if (height <= 0) height = activity.getResources().getDisplayMetrics().heightPixels;
        return new int[] { Math.max(1, width), Math.max(1, height) };
    }

    private static final class LimitedScrollView extends ScrollView {
        private final int maxHeight;
        LimitedScrollView(Context context, int maxHeight) {
            super(context, null, 0);
            this.maxHeight = maxHeight;
        }
        @Override protected void onMeasure(int widthSpec, int heightSpec) {
            int mode = MeasureSpec.getMode(heightSpec);
            int size = MeasureSpec.getSize(heightSpec);
            if (mode == MeasureSpec.UNSPECIFIED
                    || (mode == MeasureSpec.AT_MOST && size > maxHeight)) {
                heightSpec = MeasureSpec.makeMeasureSpec(maxHeight, MeasureSpec.AT_MOST);
            }
            super.onMeasure(widthSpec, heightSpec);
        }
    }
}



