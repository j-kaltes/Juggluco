package tk.glucodata;

import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

/** Stub for non-mobile build variants — snooze notification actions not supported here. */
public class SnoozeReceiver extends BroadcastReceiver {
    static final String ACTION_SNOOZE = "tk.glucodata.SNOOZE_ALARM";
    static final String EXTRA_MINUTES = "snooze_minutes";

    @Override
    public void onReceive(Context context, Intent intent) {}

    static PendingIntent pendingSnooze(long minutes, int requestCode) { return null; }
}
