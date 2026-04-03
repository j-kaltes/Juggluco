
package tk.glucodata;

import static tk.glucodata.MainActivity.WATCHFACE_PERMISSION_REQUEST_CODE;

import android.content.pm.PackageManager;

import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

public class Above33 {
    private static final String LOG_ID="Above33";
static void checkAndRestoreWatchFace(MainActivity act) {
    String permission = "com.google.wear.permission.SET_PUSHED_WATCH_FACE_AS_ACTIVE";
    
    if (ContextCompat.checkSelfPermission(act, permission) == PackageManager.PERMISSION_GRANTED) {
          setwatchface(act);
    } else {
        ActivityCompat.requestPermissions(act, new String[]{permission},WATCHFACE_PERMISSION_REQUEST_CODE);
    }
}

static void setwatchface(MainActivity act) {
    try {
        if (android.os.Build.VERSION.SDK_INT >= 33) {
           tk.glucodata.WatchFacePushHelper.restoreWatchFaceIfMissing(act,act.getString(R.string.default_wf_token));
            }
        }
     catch(Throwable th) {
        Log.stack(LOG_ID,"restoreWatchFaceIfMissing",th);
        }
    }
    }
