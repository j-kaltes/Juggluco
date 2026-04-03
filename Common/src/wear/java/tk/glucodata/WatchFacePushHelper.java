package tk.glucodata;

import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.os.ParcelFileDescriptor;
import androidx.core.content.ContextCompat;
import androidx.wear.watchfacepush.WatchFacePushManager;
import androidx.wear.watchfacepush.WatchFacePushManager.ListWatchFacesResponse;
import androidx.wear.watchfacepush.WatchFacePushManagerFactory;
import com.google.common.util.concurrent.FutureCallback;
import com.google.common.util.concurrent.Futures;
import com.google.common.util.concurrent.ListenableFuture;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.concurrent.Executor;

import kotlinx.coroutines.BuildersKt;
import kotlinx.coroutines.CoroutineScope;
import kotlinx.coroutines.CoroutineStart;
import kotlinx.coroutines.Dispatchers;
import kotlinx.coroutines.guava.ListenableFutureKt;
import tk.glucodata.Log;
import tk.glucodata.R;

public class WatchFacePushHelper {

    private static final String LOG_ID = "WatchFacePushHelper";
    private static final String WFF_PACKAGE = "tk.glucodata.watchfacepush.watchface2";

    public static void restoreWatchFaceIfMissing(Context context, String token) {
        // 1. Create the manager once
        WatchFacePushManager manager = WatchFacePushManagerFactory.createWatchFacePushManager(context);
        if (!WatchFacePushManagerFactory.isSupported()) {
            Log.d(LOG_ID, "Push API not supported on this device.");
            return;
        }
        Executor mainExecutor = ContextCompat.getMainExecutor(context);
        CoroutineScope scope = kotlinx.coroutines.CoroutineScopeKt.CoroutineScope(Dispatchers.getMain());

        // 2. Wrap listWatchFaces() into a ListenableFuture

ListenableFuture<ListWatchFacesResponse> listFuture = ListenableFutureKt.asListenableFuture(
    BuildersKt.async(scope, scope.getCoroutineContext(), CoroutineStart.DEFAULT,
    (coroutineScope, continuation) -> {
        try {
            return manager.listWatchFaces(continuation);
        } catch(Throwable th) {
            Log.stack(LOG_ID,"listWatchFaces",th);
            return null;
          }
    }) // Added coroutineScope and passed continuation
);


        Futures.addCallback(listFuture, new FutureCallback<ListWatchFacesResponse>() {
            @Override
            public void onSuccess(ListWatchFacesResponse response) {
                if(response==null) {
                    Log.e(LOG_ID,"ListWatchFacesResponse response==null");
                    return;
                     }
                boolean isInstalled = false;
                for (WatchFacePushManager.WatchFaceDetails details : response.getInstalledWatchFaceDetails()) {
                    if (details.getPackageName().equals(WFF_PACKAGE)) {
                        isInstalled = true;
                        break;
                    }
                }

                // 3. Only proceed with installation if it's missing
                if (!isInstalled) {
                    performInstallation(context, manager, token);
                }
            }

            @Override
            public void onFailure(Throwable t) {
                // Log: Status check failed
            }
        }, mainExecutor);
    }
private static ParcelFileDescriptor openBundledWatchFacePfd(Context context) throws IOException {
    File out = new File(context.getCacheDir(), "default_watchface.apk");

    try (InputStream in = context.getAssets().open("default_watchface.apk");
         OutputStream os = new FileOutputStream(out)) {
        byte[] buf = new byte[8192];
        int n;
        while ((n = in.read(buf)) > 0) {
            os.write(buf, 0, n);
        }
    }

    return ParcelFileDescriptor.open(out, ParcelFileDescriptor.MODE_READ_ONLY);
}



    private static void performInstallation(Context context, WatchFacePushManager manager, String token) {
        Executor mainExecutor = ContextCompat.getMainExecutor(context);
        CoroutineScope scope = kotlinx.coroutines.CoroutineScopeKt.CoroutineScope(Dispatchers.getIO());

        try { 
            ParcelFileDescriptor pfd = openBundledWatchFacePfd(context);

        ListenableFuture<WatchFacePushManager.WatchFaceDetails> addFuture = ListenableFutureKt.asListenableFuture(
            BuildersKt.async(scope, scope.getCoroutineContext(), CoroutineStart.DEFAULT,
            (coroutineScope, continuation) -> {
                try {
                    return manager.addWatchFace(pfd, token, continuation);
                } catch(Throwable th) {
                    Log.stack(LOG_ID,"addWatchFace",th);
                    return null;
                }
            }) // Passed continuation here too
        );


Futures.addCallback(addFuture, new FutureCallback<WatchFacePushManager.WatchFaceDetails>() {
    @Override
    public void onSuccess(WatchFacePushManager.WatchFaceDetails result) {
        if (result == null) {
            Log.e(LOG_ID, "addWatchFace returned null");
            return;
        }

        final String slotId = result.getSlotId();   // or the exact type your dependency exposes
        Log.d(LOG_ID, "Added " + result.getPackageName() + " in slot " + slotId);

        ListenableFuture<kotlin.Unit> activeFuture = ListenableFutureKt.asListenableFuture(
            BuildersKt.async(scope, scope.getCoroutineContext(), CoroutineStart.DEFAULT,
                (coroutineScope, continuation) -> {
                    try {
                        return manager.setWatchFaceAsActive(slotId, continuation);
                    } catch(Throwable e) {
                        Log.stack(LOG_ID,"setWatchFaceAsActive",e);
                        throw new RuntimeException(e);
                    }
                }
            )
        );

        Futures.addCallback(activeFuture, new FutureCallback<kotlin.Unit>() {
            @Override
            public void onSuccess(kotlin.Unit ignored) {
                Log.d(LOG_ID, "Activation request sent successfully");
            }

            @Override
            public void onFailure(Throwable t) {
                Log.stack(LOG_ID, "Activation onFailure", t);
            }
        }, mainExecutor);
    }

    @Override
    public void onFailure(Throwable t) {
        Log.stack(LOG_ID, "addWatchFace failed", t);
    }
}, mainExecutor);




        } catch (Throwable e) {
            Log.stack(LOG_ID,"performInstallation",e)        ;
        }
    }
}


