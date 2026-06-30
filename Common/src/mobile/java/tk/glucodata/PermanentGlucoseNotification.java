package tk.glucodata;

import static tk.glucodata.Log.doLog;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.drawable.Icon;
import android.os.Build;
import android.util.DisplayMetrics;
import android.view.View;
import android.widget.RemoteViews;

/**
 * Persistent, always-on glucose notification — visible whenever the lock screen is shown.
 *
 * Mirrors what GlucoDataHandler calls "PermanentNotification":
 *   • Posted with VISIBILITY_PUBLIC so it shows in full on the lock screen
 *   • setOngoing(true) + FLAG_NO_CLEAR — cannot be dismissed by the user
 *   • Low-importance channel (no sound, no heads-up) — purely informational
 *   • Updated on every new reading via update()
 *
 * Content: glucose value (large, range-coloured) | trend arrow bitmap | Δ delta | elapsed time
 *
 * v7 — foreground service notification: posted via startForeground() so Android cannot
 *       collapse/group it into the lock-screen icon pill regardless of how many other
 *       notifications are pending.
 *
 * Enabled/disabled via SharedPreferences key "permanent_glucose_notification" in "juggluco_dev".
 * Default: enabled.
 */
public class PermanentGlucoseNotification {

    private static final String LOG_ID = "PermanentGlucoseNotif";

    // Notification channel for this permanent notification.
    // Channel ID "glucosePermanent2" supersedes "glucosePermanent" (existing channels
    // cannot have their importance changed at runtime; a new ID forces recreation).
    static final String CHANNEL_ID   = "glucosePermanent2";
    static final int    NOTIF_ID     = 81435;   // well away from other IDs in Notify.java

    private static final String PREFS        = "juggluco_dev";
    private static final String KEY_ENABLED  = "permanent_glucose_notification";

    // ── Preference helpers ────────────────────────────────────────────────────

    public static boolean isEnabled() {
        return Applic.app
                .getSharedPreferences(PREFS, Context.MODE_PRIVATE)
                .getBoolean(KEY_ENABLED, true);   // default ON
    }

    public static void setEnabled(boolean enabled) {
        Applic.app
                .getSharedPreferences(PREFS, Context.MODE_PRIVATE)
                .edit().putBoolean(KEY_ENABLED, enabled).apply();
        if (enabled) {
            update();
        } else {
            cancel();
        }
    }

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    /** Called from keeprunning.onStartCommand() — creates channel and shows first notification. */
    public static void init(Context context) {
        createChannel(context);
        if (isEnabled()) {
            update();
        }
    }

    /** Cancel the notification (called when user disables, or on service stop). */
    public static void cancel() {
        try {
            // If this notification was promoted to the foreground service slot, we must
            // hand the foreground badge back to Notify before cancelling, otherwise the
            // service would be silently demoted to background.
            if (keeprunning.theservice != null) {
                // Restore Notify's foreground notification, then cancel our slot.
                Notify.foregroundnot(keeprunning.theservice);
            }
            NotificationManager nm = (NotificationManager)
                    Applic.app.getSystemService(Context.NOTIFICATION_SERVICE);
            if (nm != null) nm.cancel(NOTIF_ID);
        } catch (Throwable t) {
            Log.stack(LOG_ID, "cancel", t);
        }
    }

    // ── Channel creation ──────────────────────────────────────────────────────

    private static void createChannel(Context context) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationManager nm = (NotificationManager)
                    context.getSystemService(Context.NOTIFICATION_SERVICE);
            if (nm == null) return;
            if (nm.getNotificationChannel(CHANNEL_ID) != null) return; // already created

            NotificationChannel ch = new NotificationChannel(
                    CHANNEL_ID,
                    context.getString(R.string.permanent_notif_channel_name),
                    NotificationManager.IMPORTANCE_DEFAULT);  // shows as card; sound suppressed via setSound(null)
            ch.setDescription(context.getString(R.string.permanent_notif_channel_desc));
            ch.setSound(null, null);
            ch.enableVibration(false);
            ch.setShowBadge(false);
            nm.createNotificationChannel(ch);
        }
    }

    // ── Update (call on every new reading) ────────────────────────────────────

    public static void update() {
        if (!isEnabled()) return;
        try {
            final notGlucose glucose = SuperGattCallback.previousglucose;
            if (glucose == null || glucose.value == null) return;

            final Context ctx = Applic.app;

            final Notification.Builder builder;
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                builder = new Notification.Builder(ctx, CHANNEL_ID);
            } else {
                builder = new Notification.Builder(ctx);
                builder.setPriority(Notification.PRIORITY_DEFAULT);
            }

            // Lock-screen visibility: always show full content
            builder.setVisibility(Notification.VISIBILITY_PUBLIC);
            builder.setOngoing(true);
            builder.setOnlyAlertOnce(true);
            builder.setAutoCancel(false);
            builder.setShowWhen(true);
            builder.setWhen(glucose.time);
            builder.setSmallIcon(R.drawable.novalue);   // same small icon as glucose notification
            builder.setContentIntent(Notify.mkpending());
            builder.setCategory(Notification.CATEGORY_STATUS);

            // Keep plain-text fallback content for system surfaces that ignore custom views.
            builder.setContentTitle(buildSummaryTitle(glucose));

            // Big (expanded) custom view: full card with graph.
            // Collapsed custom view: just the row (no graph).
            final RemoteViews collapsed = buildRemoteView(ctx, glucose, false);
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
                builder.setCustomContentView(collapsed);
                builder.setCustomBigContentView(buildRemoteView(ctx, glucose, true));
                // Do NOT wrap in DecoratedCustomViewStyle — that strips custom text colours in
                // the collapsed row and adds unwanted system chrome around the custom view.
            } else {
                builder.setContent(collapsed);
            }

            Notification notif = builder.build();
            notif.flags |= Notification.FLAG_NO_CLEAR | Notification.FLAG_ONGOING_EVENT;

            // v7: post as a foreground-service notification so Android cannot collapse it
            // into the lock-screen icon pill.  startForeground() with our ID moves the
            // "protected" foreground badge to this notification; the existing Notify
            // foreground notification (ID 81431) becomes a regular ongoing notification.
            if (keeprunning.theservice != null) {
                keeprunning.theservice.startForeground(NOTIF_ID, notif);
            } else {
                final NotificationManager nm = (NotificationManager)
                        ctx.getSystemService(Context.NOTIFICATION_SERVICE);
                if (nm != null) nm.notify(NOTIF_ID, notif);
            }
            if (doLog) Log.i(LOG_ID, "update posted: " + glucose.value);
        } catch (Throwable t) {
            Log.stack(LOG_ID, "update", t);
        }
    }

    // ── Summary title (plain text, shown in grouped/collapsed rows) ───────────

    /**
     * Builds a compact plain-text summary for the notification's collapsed/grouped row:
     *   "231 → Δ +3"  (or "231 ⚠ Δ --" when stale)
     * Android displays this as {@code setContentTitle} when it stacks same-app notifications.
     */
    private static String buildSummaryTitle(notGlucose glucose) {
        final long ageSec = (System.currentTimeMillis() - glucose.time) / 1000L;
        final boolean stale = ageSec > Notify.glucosetimeoutSEC;
        final StringBuilder sb = new StringBuilder();
        sb.append(glucose.value);
        if (!stale && !Float.isNaN(glucose.rate)) {
            // Unicode trend arrows matching what Juggluco uses elsewhere
            final float r = glucose.rate;
            if      (r >  1.6f) sb.append(" ↑↑");
            else if (r >  0.6f) sb.append(" ↑");
            else if (r >  0.2f) sb.append(" ↗");
            else if (r > -0.2f) sb.append(" →");
            else if (r > -0.6f) sb.append(" ↘");
            else if (r > -1.6f) sb.append(" ↓");
            else                sb.append(" ↓↓");
            // delta
            final float delta5 = glucose.rate * 5f;
            final String fmt = Applic.unit == 1 ? "  Δ %+.1f" : "  Δ %+.0f";
            sb.append(String.format(Applic.usedlocale, fmt, delta5));
        } else if (stale) {
            sb.append("  ⚠  Δ --");
        }
        return sb.toString();
    }

    // ── RemoteViews builder ───────────────────────────────────────────────────

    /**
     * @param withGraph when true, renders and shows the pn_graph ImageView (big/expanded view).
     *                  when false, hides it (collapsed/header view).
     */
    private static RemoteViews buildRemoteView(Context ctx, notGlucose glucose, boolean withGraph) {
        final RemoteViews rv = new RemoteViews(ctx.getPackageName(),
                R.layout.notification_glucose_permanent);

        final long now      = System.currentTimeMillis();
        final long ageSec   = (now - glucose.time) / 1000L;
        final boolean stale = ageSec > Notify.glucosetimeoutSEC;

        // ── Glucose value ───────────────────────────────────────────────────
        rv.setTextViewText(R.id.pn_glucose, glucose.value);
        final int glucoseColor = stale
                ? Color.GRAY
                : RemoteGlucose.glucoseRangeColor(SuperGattCallback.previousglucosevalue);
        rv.setTextColor(R.id.pn_glucose, glucoseColor);

        if (stale && Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            // Strike through when stale
            rv.setInt(R.id.pn_glucose, "setPaintFlags",
                    android.graphics.Paint.STRIKE_THRU_TEXT_FLAG |
                    android.graphics.Paint.ANTI_ALIAS_FLAG);
        }

        // ── Trend arrow ─────────────────────────────────────────────────────
        if (stale || Float.isNaN(glucose.rate)) {
            rv.setViewVisibility(R.id.pn_arrow, View.GONE);
        } else {
            rv.setViewVisibility(R.id.pn_arrow, View.VISIBLE);
            // Render a small arrow bitmap (96×96 px)
            final Bitmap arrowBmp = buildArrowBitmap(glucose.rate, glucoseColor);
            if (arrowBmp != null) {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                    rv.setImageViewIcon(R.id.pn_arrow, Icon.createWithBitmap(arrowBmp));
                } else {
                    rv.setImageViewBitmap(R.id.pn_arrow, arrowBmp);
                }
            }
        }

        // ── Delta ───────────────────────────────────────────────────────────
        if (!stale && !Float.isNaN(glucose.rate)) {
            final float delta5 = glucose.rate * 5f;
            final String fmt   = Applic.unit == 1 ? "%+.1f" : "%+.0f";
            final String dStr  = "\u0394 " + String.format(Applic.usedlocale, fmt, delta5);
            rv.setTextViewText(R.id.pn_delta, dStr);
            rv.setTextColor(R.id.pn_delta, glucoseColor);
        } else {
            rv.setTextViewText(R.id.pn_delta, "\u0394 --");
            rv.setTextColor(R.id.pn_delta, Color.GRAY);
        }

        // ── Elapsed time ────────────────────────────────────────────────────
        final long elapsedMin = ageSec / 60L;
        rv.setTextViewText(R.id.pn_time, elapsedMin + " min");
        // Use white for time so it's legible on any background
        final int timeColor = Notify.foregroundcolor != Color.BLACK
                ? Notify.foregroundcolor : Color.WHITE;
        rv.setTextColor(R.id.pn_time, timeColor);

        // ── Graph (big/expanded view only) ──────────────────────────────────
        if (withGraph && Applic.Nativesloaded) {
            final Bitmap graphBmp = buildGraphBitmap(ctx);
            if (graphBmp != null) {
                rv.setImageViewBitmap(R.id.pn_graph, graphBmp);
                rv.setViewVisibility(R.id.pn_graph, View.VISIBLE);
            } else {
                rv.setViewVisibility(R.id.pn_graph, View.GONE);
            }
        } else {
            rv.setViewVisibility(R.id.pn_graph, View.GONE);
        }

        return rv;
    }

    /**
     * Renders the 3-hour glucose history graph using the NanoVG-RT JNI backend.
     * Width is derived from the display width; height is fixed at 100 dp.
     */
    private static Bitmap buildGraphBitmap(Context ctx) {
        try {
            final DisplayMetrics dm = ctx.getResources().getDisplayMetrics();
            // Notification panel is full display width on most phones.
            // Use 95% of display width to leave a small margin.
            final int wPx = Math.round(dm.widthPixels * 0.95f);
            // 100 dp fixed height matches the layout ImageView
            final int hPx = Math.round(100 * dm.density);
            if (wPx < 60 || hPx < 40) return null;
            final int[] pixels = Natives.getWidgetGraphBitmap(wPx, hPx, 3 * 3600);
            if (pixels == null) return null;
            return Bitmap.createBitmap(pixels, wPx, hPx, Bitmap.Config.ARGB_8888);
        } catch (Throwable t) {
            Log.stack(LOG_ID, "buildGraphBitmap", t);
            return null;
        }
    }

    /** Renders a trend arrow bitmap (96×96 px) using the existing CommonCanvas helper. */
    private static Bitmap buildArrowBitmap(float rate, int color) {
        try {
            final int sz = 96;
            final Bitmap bmp = Bitmap.createBitmap(sz, sz, Bitmap.Config.ARGB_8888);
            final Canvas cv  = new Canvas(bmp);
            cv.drawColor(android.graphics.Color.TRANSPARENT, android.graphics.PorterDuff.Mode.CLEAR);
            final Paint p = new Paint();
            p.setAntiAlias(true);
            p.setColor(color);
            p.setStrokeWidth(4f);
            // Arrow tip at (65%, 50%) — same placement as all other widget arrow bitmaps.
            // drawarrow draws shaft backwards from the tip so this centres it in the bitmap.
            CommonCanvas.drawarrow(cv, p, sz / 54f, rate, sz * 0.65f, sz * 0.50f);
            return bmp;
        } catch (Throwable t) {
            Log.stack(LOG_ID, "buildArrowBitmap", t);
            return null;
        }
    }
}
