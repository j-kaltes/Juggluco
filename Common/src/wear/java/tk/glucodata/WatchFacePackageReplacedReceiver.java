package tk.glucodata;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

import androidx.annotation.NonNull;

public final class WatchFacePackageReplacedReceiver extends BroadcastReceiver {

    private static final String LOG_ID = "WatchFacePackageReplacedReceiver";

    @Override
    public void onReceive(Context context, Intent intent) {
        if(intent == null || !Intent.ACTION_MY_PACKAGE_REPLACED.equals(intent.getAction())) {
            return;
            }
        final PendingResult pendingResult = goAsync();
        Log.i(LOG_ID,"onReceive");
        WatchFacePushHelper.syncBundledWatchFace(context,context.getString(R.string.default_wf_token),
                new WatchFacePushHelper.SyncCallback() {
                    @Override
                    public void onComplete(@NonNull WatchFacePushHelper.SyncResult result) {
                        Log.i(LOG_ID, "sync complete added=" + result.isAdded() + " updated=" + result.isUpdated() + " offerActivation=" + result.shouldOfferActivation());
                        pendingResult.finish();
                         }

                    @Override
                    public void onError(@NonNull Throwable error) {
                        Log.stack(LOG_ID, "sync failed", error);
                        pendingResult.finish();
                        }
                });
    }
}
