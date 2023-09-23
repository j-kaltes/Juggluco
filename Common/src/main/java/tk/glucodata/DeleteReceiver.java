package tk.glucodata;

import static tk.glucodata.Log.stack;
import static tk.glucodata.Notify.penmutable;

import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public class DeleteReceiver extends BroadcastReceiver {
    static private final String clearnotification = "clearnotification";
    static private final String LOG_ID = "DeleteReceiver";

    @Override
    public void onReceive(Context context, Intent intent) {
        final String action = intent.getAction();
        try {
            if (action != null && action.equals(clearnotification)) {
                Log.i(LOG_ID, clearnotification + " Stop Alarm");
                Notify.stopalarm();
            } else
                Log.i(LOG_ID, clearnotification);
        } catch(Throwable th) {
                stack(LOG_ID,"onReceive",th);
        }
    }

    static PendingIntent getDeleteIntent() {
        Intent intent = new Intent(Applic.app, DeleteReceiver.class);
        intent.setAction(clearnotification);
	final int deleterequest = 124;
        return PendingIntent.getBroadcast(Applic.app, deleterequest, intent, PendingIntent.FLAG_UPDATE_CURRENT|penmutable);
    }

}
