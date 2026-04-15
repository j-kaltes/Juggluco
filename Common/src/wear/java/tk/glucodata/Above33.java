package tk.glucodata;

import androidx.wear.watchfacepush.WatchFacePushManagerFactory;

public class Above33 {
    private static final String LOG_ID="Above33";
static void checkAndRestoreWatchFace(MainActivity act) {
    if(Natives.getaskedWatchFace()) { return ; }
try {
    if(!WatchFacePushManagerFactory.isSupported()) {
        Log.i(LOG_ID,"WatchFacePushManagerFactory not supported");
        Specific.installwatchface(act);
        return ;
        }
  Specific.WatchFaceActivation.syncFromUi(act);
  }
   catch(Throwable th) {
      Log.stack(LOG_ID,"syncFromUi",th);
      }
  return ;
}

}
