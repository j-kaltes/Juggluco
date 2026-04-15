package tk.glucodata;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.ParcelFileDescriptor;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import androidx.wear.watchfacepush.WatchFacePushManager;
import androidx.wear.watchfacepush.WatchFacePushManager.ListWatchFacesResponse;
import androidx.wear.watchfacepush.WatchFacePushManager.WatchFaceDetails;
import androidx.wear.watchfacepush.WatchFacePushManagerFactory;

import com.google.common.util.concurrent.FutureCallback;
import com.google.common.util.concurrent.Futures;
import com.google.common.util.concurrent.ListenableFuture;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.List;
import java.util.concurrent.Executor;

import kotlin.Unit;
import kotlinx.coroutines.BuildersKt;
import kotlinx.coroutines.CoroutineScope;
import kotlinx.coroutines.CoroutineStart;
import kotlinx.coroutines.Dispatchers;
import kotlinx.coroutines.guava.ListenableFutureKt;

public final class WatchFacePushHelper {

    private static final String LOG_ID = "WatchFacePushHelper";
    private static final String ASSET_NAME = "default_watchface.apk";

    // Your watch face package inside the bundled/default_watchface.apk
    private static final String WFF_PACKAGE = "tk.glucodata.watchfacepush.watchface3";

    private static final String PREFS_NAME = "watchface_push";
    private static final String KEY_ACTIVATION_ALREADY_ASKED = "activation_already_asked";

    // Runtime permission string used for setWatchFaceAsActive()
    public static final String PERMISSION_SET_ACTIVE =
            "com.google.wear.permission.SET_PUSHED_WATCH_FACE_AS_ACTIVE";

    private WatchFacePushHelper() {
    }

    public interface SyncCallback {
        void onComplete(@NonNull SyncResult result);
        void onError(@NonNull Throwable error);
    }

    public interface SimpleCallback {
        void onSuccess();
        void onError(@NonNull Throwable error);
    }

    public static final class SyncResult {
        @Nullable
        private final WatchFaceDetails details;
        private final boolean added;
        private final boolean updated;
        private final boolean shouldOfferActivation;

        public SyncResult(
                @Nullable WatchFaceDetails details,
                boolean added,
                boolean updated,
                boolean shouldOfferActivation
        ) {
            this.details = details;
            this.added = added;
            this.updated = updated;
            this.shouldOfferActivation = shouldOfferActivation;
        }

        @Nullable
        public WatchFaceDetails getDetails() {
            return details;
        }

        public boolean isAdded() {
            return added;
        }

        public boolean isUpdated() {
            return updated;
        }

        public boolean shouldOfferActivation() {
            return shouldOfferActivation;
        }
    }

    public static void syncBundledWatchFace( @NonNull Context context, @NonNull String validationToken, @NonNull SyncCallback callback) {
        final Context appContext = context.getApplicationContext();

        if (!WatchFacePushManagerFactory.isSupported()) {
            Log.d(LOG_ID, "Watch Face Push not supported on this device");
            callback.onComplete(new SyncResult(null, false, false, false));
            return;
        }
        Log.i(LOG_ID,"syncBundledWatchFace");

        final WatchFacePushManager manager = WatchFacePushManagerFactory.createWatchFacePushManager(appContext);
        final Executor mainExecutor = ContextCompat.getMainExecutor(appContext);
        final CoroutineScope ioScope =kotlinx.coroutines.CoroutineScopeKt.CoroutineScope(Dispatchers.getIO());

        final ListenableFuture<ListWatchFacesResponse> listFuture =
                ListenableFutureKt.asListenableFuture(
                        BuildersKt.async(
                                ioScope,
                                ioScope.getCoroutineContext(),
                                CoroutineStart.DEFAULT,
                                (coroutineScope, continuation) -> {
                                        try {
                                        return manager.listWatchFaces(continuation) ;
                                        }
                                        catch(Throwable th) {
                                            Log.stack(LOG_ID,"listWatchFaces",th);
                                            return null;

                                        }
                                }) );
        Futures.addCallback(listFuture, new FutureCallback<ListWatchFacesResponse>() {
            @Override
            public void onSuccess(ListWatchFacesResponse response) {
                if (response == null) {
                    callback.onError(new IllegalStateException("listWatchFaces() returned null"));
                    return;
                }

                final WatchFaceDetails installed = findInstalled(response.getInstalledWatchFaceDetails());

                final long bundledVersion;
                try {
                    bundledVersion = readBundledVersionCode(appContext);
                } catch (Throwable t) {
                    callback.onError(t);
                    return;
                }

                if (installed == null) {
                    Log.i(LOG_ID, "Bundled watch face missing, adding it");
                    addBundledWatchFace(appContext, manager, validationToken, callback);
                    return;
                }

                if (bundledVersion > installed.getVersionCode()) {
                    Log.i(LOG_ID, "Bundled watch face is newer. bundled=" + bundledVersion + " installed=" + installed.getVersionCode());
                    checkActiveThenUpdate( appContext, manager, installed, validationToken, callback);
                    return;
                }

                Log.i(LOG_ID, "Bundled watch face already installed and up to date. version=" + installed.getVersionCode());

                checkActiveThenFinish( appContext, manager, installed, false, false, callback);
            }

            @Override
            public void onFailure(Throwable t) {
                Log.stack(LOG_ID, "syncBundledWatchFace",t);
                callback.onError(t);
            }
        }, mainExecutor);
    }

    /**
     * Call this exactly when you decide to show the one activation ask.
     * After this returns true once, your app will never ask again.
     */
    public static boolean consumeActivationAsk(@NonNull Context context) {
        return true;
       }

    public static boolean wasActivationAlreadyAsked(@NonNull Context context) {
        return false;
    }

    public static void activateInstalledWatchFace(
            @NonNull Context context,
            @NonNull String slotId,
            @NonNull SimpleCallback callback
    ) {
        final Context appContext = context.getApplicationContext();

        if (!WatchFacePushManagerFactory.isSupported()) {
            callback.onError(new IllegalStateException("Watch Face Push not supported"));
            return;
        }
       Log.i(LOG_ID,"activateInstalledWatchFace");
        final WatchFacePushManager manager =
                WatchFacePushManagerFactory.createWatchFacePushManager(appContext);
        final Executor mainExecutor = ContextCompat.getMainExecutor(appContext);
        final CoroutineScope ioScope =
                kotlinx.coroutines.CoroutineScopeKt.CoroutineScope(Dispatchers.getIO());

        ListenableFuture<Unit> activeFuture =
                ListenableFutureKt.asListenableFuture(
                        BuildersKt.async(
                                ioScope,
                                ioScope.getCoroutineContext(),
                                CoroutineStart.DEFAULT,
                                (coroutineScope, continuation) ->
                                {
                                    try {
                                        return manager.setWatchFaceAsActive(slotId, continuation);
                                    } catch(WatchFacePushManager.SetWatchFaceAsActiveException e) {
                                        Log.stack(LOG_ID,"setWatchFaceASActit",e);
                                        return null;
                                       }
                                    catch(Throwable t) {
                                        Log.stack(LOG_ID,"setWatchFaceASActit",t);
                                        return null;
                                        }

                                }
                        )
                );

        Futures.addCallback(activeFuture, new FutureCallback<Unit>() {
            @Override
            public void onSuccess(Unit result) {
                Log.i(LOG_ID, "setWatchFaceAsActive succeeded for slotId=" + slotId);
                callback.onSuccess();
            }

            @Override
            public void onFailure(Throwable t) {
                Log.stack(LOG_ID, "setWatchFaceAsActive failed", t);
                callback.onError(t);
            }
        }, mainExecutor);
    }

    private static void addBundledWatchFace(
            @NonNull Context context,
            @NonNull WatchFacePushManager manager,
            @NonNull String validationToken,
            @NonNull SyncCallback callback
    ) {
        final Executor mainExecutor = ContextCompat.getMainExecutor(context);
        final CoroutineScope ioScope =
                kotlinx.coroutines.CoroutineScopeKt.CoroutineScope(Dispatchers.getIO());

        final ParcelFileDescriptor pfd;
        try {
            pfd = openBundledWatchFacePfd(context);
        } catch (Throwable t) {
            callback.onError(t);
            return;
        }

        ListenableFuture<WatchFaceDetails> addFuture =
                ListenableFutureKt.asListenableFuture(
                        BuildersKt.async(
                                ioScope,
                                ioScope.getCoroutineContext(),
                                CoroutineStart.DEFAULT,
                                (coroutineScope, continuation) ->
                                {
                                    try {
                                        return manager.addWatchFace(pfd, validationToken, continuation);
                                    } catch(WatchFacePushManager.AddWatchFaceException e) {
                                        Log.stack(LOG_ID,"addWatchFace",e);
                                        return null;
                                    }
                                 catch(Throwable e) {
                                    Log.stack(LOG_ID,"addWatchFace",e);
                                    return null;
                                      }
                                    }
                        )
                );

        Futures.addCallback(addFuture, new FutureCallback<WatchFaceDetails>() {
            @Override
            public void onSuccess(WatchFaceDetails details) {
                closeQuietly(pfd);

                if (details == null) {
                    callback.onError(new IllegalStateException("addWatchFace() returned null"));
                    return;
                }

                Log.i(LOG_ID, "Added watch face " + details.getPackageName()
                        + " version=" + details.getVersionCode()
                        + " slotId=" + details.getSlotId());

                boolean shouldOfferActivation = !wasActivationAlreadyAsked(context);

                callback.onComplete(new SyncResult(details, true, false, shouldOfferActivation)
                );
            }

            @Override
            public void onFailure(Throwable t) {
                closeQuietly(pfd);
                callback.onError(t);
            }
        }, mainExecutor);
    }

    private static void checkActiveThenUpdate( @NonNull Context context, @NonNull WatchFacePushManager manager, @NonNull WatchFaceDetails installed, @NonNull String validationToken, @NonNull SyncCallback callback) {
        final Executor mainExecutor = ContextCompat.getMainExecutor(context);
        final CoroutineScope ioScope =
                kotlinx.coroutines.CoroutineScopeKt.CoroutineScope(Dispatchers.getIO());

        ListenableFuture<Boolean> activeFuture =
                ListenableFutureKt.asListenableFuture(
                        BuildersKt.async(
                                ioScope,
                                ioScope.getCoroutineContext(),
                                CoroutineStart.DEFAULT,
                                (coroutineScope, continuation) ->
                                {
                                    try {
                                        return manager.isWatchFaceActive(WFF_PACKAGE, continuation);
                                    } catch(WatchFacePushManager.IsWatchFaceActiveException e) {
                                        Log.stack(LOG_ID,"isWatchFaceActive",e);
                                        return null;
                                      }
                                }
                        )
                );

        Futures.addCallback(activeFuture, new FutureCallback<Boolean>() {
            @Override
            public void onSuccess(Boolean isActiveBeforeUpdate) {
                final boolean wasActive = Boolean.TRUE.equals(isActiveBeforeUpdate);
                Log.i(LOG_ID, "checkActiveThenUpdate onSuccess wasActive="+wasActive);
                doUpdate(context, manager, installed, validationToken, wasActive, callback);
            }

            @Override
            public void onFailure(Throwable t) {
                Log.stack(LOG_ID, "checkActiveThenUpdate onFailure",t);
                callback.onError(t);
            }
        }, mainExecutor);
    }

    private static void doUpdate( @NonNull Context context, @NonNull WatchFacePushManager manager, @NonNull WatchFaceDetails installed, @NonNull String validationToken, boolean wasActiveBeforeUpdate, @NonNull SyncCallback callback) {
        Log.i(LOG_ID,"doUpdate wasActiveBeforeUpdate="+ wasActiveBeforeUpdate);
        final Executor mainExecutor = ContextCompat.getMainExecutor(context);
        final CoroutineScope ioScope =
                kotlinx.coroutines.CoroutineScopeKt.CoroutineScope(Dispatchers.getIO());

        final ParcelFileDescriptor pfd;
        try {
            pfd = openBundledWatchFacePfd(context);
        } catch (Throwable t) {
            callback.onError(t);
            return;
        }

        ListenableFuture<WatchFaceDetails> updateFuture =
                ListenableFutureKt.asListenableFuture(
                        BuildersKt.async(
                                ioScope,
                                ioScope.getCoroutineContext(),
                                CoroutineStart.DEFAULT,
                                (coroutineScope, continuation) ->
                                {
                                    try {
                                        return manager.updateWatchFace(
                                                installed.getSlotId(),
                                                pfd,
                                                validationToken,
                                                continuation
                                        );
                                    } catch(Throwable th) {
                                        Log.stack(LOG_ID,"updateWatchFace",th);
                                        return null;
                                    }
                                }
                        )
                );

        Futures.addCallback(updateFuture, new FutureCallback<WatchFaceDetails>() {
            @Override
            public void onSuccess(WatchFaceDetails updatedDetails) {
                closeQuietly(pfd);

                if (updatedDetails == null) {
                    callback.onError(new IllegalStateException("updateWatchFace() returned null"));
                    return;
                }

                Log.i(LOG_ID, "Updated watch face " + updatedDetails.getPackageName()
                        + " version=" + updatedDetails.getVersionCode()
                        + " slotId=" + updatedDetails.getSlotId());

                // Intentionally do NOT call isWatchFaceActive() here.
                boolean shouldOfferActivation =
                        !wasActiveBeforeUpdate && !wasActivationAlreadyAsked(context);

                callback.onComplete(
                        new SyncResult(updatedDetails, false, true, shouldOfferActivation)
                );
            }

            @Override
            public void onFailure(Throwable t) {
                closeQuietly(pfd);
                callback.onError(t);
            }
        }, mainExecutor);
    }

    private static void checkActiveThenFinish( @NonNull Context context, @NonNull WatchFacePushManager manager, @NonNull WatchFaceDetails installed, boolean added, boolean updated, @NonNull SyncCallback callback) {
        Log.i(LOG_ID,"checkActiveThenFinish");
        final Executor mainExecutor = ContextCompat.getMainExecutor(context);
        final CoroutineScope ioScope = kotlinx.coroutines.CoroutineScopeKt.CoroutineScope(Dispatchers.getIO());
        ListenableFuture<Boolean> activeFuture =
                ListenableFutureKt.asListenableFuture(
                        BuildersKt.async(
                                ioScope,
                                ioScope.getCoroutineContext(),
                                CoroutineStart.DEFAULT,
                                (coroutineScope, continuation) ->
                                {
                                    try {
                                        return manager.isWatchFaceActive(WFF_PACKAGE, continuation);
                                    } catch(Throwable e) {
                                        Log.stack(LOG_ID,"isWatchFaceActive",e);
                                        return null;
                                    }
                                }
                        )
                );

        Futures.addCallback(activeFuture, new FutureCallback<Boolean>() {
            @Override
            public void onSuccess(Boolean isActive) {
              Log.i(LOG_ID,"checkActiveThenFinish onSuccess "+isActive);
                boolean shouldOfferActivation =
                        !Boolean.TRUE.equals(isActive) && !wasActivationAlreadyAsked(context);

                callback.onComplete( new SyncResult(installed, added, updated, shouldOfferActivation));
            }

            @Override
            public void onFailure(Throwable t) {
               Log.stack(LOG_ID,"checkActiveThenFinish onFailure",t);
                callback.onError(t);
            }
        }, mainExecutor);
    }

    @Nullable
    private static WatchFaceDetails findInstalled(@Nullable List<WatchFaceDetails> list) {
        if (list == null) {
            return null;
        }
        for (WatchFaceDetails details : list) {
            if (details != null && WFF_PACKAGE.equals(details.getPackageName())) {
                return details;
            }
        }
        return null;
    }

    @NonNull
    private static ParcelFileDescriptor openBundledWatchFacePfd(@NonNull Context context) throws IOException {
        File out = materializeBundledApk(context);
        return ParcelFileDescriptor.open(out, ParcelFileDescriptor.MODE_READ_ONLY);
        }

    private static long readBundledVersionCode(@NonNull Context context) throws IOException {
        File apkFile = materializeBundledApk(context);
        PackageManager pm = context.getPackageManager();

        final PackageInfo packageInfo;
        if (Build.VERSION.SDK_INT >= 33) {
            packageInfo = pm.getPackageArchiveInfo(
                    apkFile.getAbsolutePath(),
                    PackageManager.PackageInfoFlags.of(0)
            );
        } else {
            //noinspection deprecation
            packageInfo = pm.getPackageArchiveInfo(apkFile.getAbsolutePath(), 0);
        }

        if (packageInfo == null) {
            throw new IOException("Could not read PackageInfo from " + apkFile.getAbsolutePath());
        }

        if (Build.VERSION.SDK_INT >= 28) {
            return packageInfo.getLongVersionCode();
        } else {
            //noinspection deprecation
            return packageInfo.versionCode;
        }
    }

    @NonNull
    private static File materializeBundledApk(@NonNull Context context) throws IOException {
        File out = new File(context.getCacheDir(), ASSET_NAME);

        try (InputStream in = context.getAssets().open(ASSET_NAME);
             OutputStream os = new FileOutputStream(out, false)) {

            byte[] buf = new byte[8192];
            int n;
            while ((n = in.read(buf)) > 0) {
                os.write(buf, 0, n);
            }
            os.flush();
        }

        return out;
    }

    private static void closeQuietly(@Nullable ParcelFileDescriptor pfd) {
        if (pfd == null) {
            return;
        }
        try {
            pfd.close();
        } catch (Throwable ignored) {
        }
    }
}
