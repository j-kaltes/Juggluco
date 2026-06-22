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
/*                                                                                   */
/*      Sun Apr 16 20:59:10 CEST 2023                                                 */


package tk.glucodata;

import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static tk.glucodata.Applic.RunOnUiThread;
import static tk.glucodata.MainActivity.REQ_SET_PUSHED_WATCH_FACE_ACTIVE;
import static tk.glucodata.settings.Settings.removeContentView;

import android.content.Context;

import android.annotation.SuppressLint;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.splashscreen.SplashScreen;

import android.view.LayoutInflater;
import android.view.ViewGroup;

import android.app.Activity;
import android.content.pm.PackageManager;

import androidx.annotation.NonNull;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
public class Specific {
final static private String LOG_ID="Specific";

static void start(Object context) { }

static    void splash(AppCompatActivity act) {
       SplashScreen.installSplashScreen(act);
      }
@SuppressLint("StaticFieldLeak")
static ViewGroup layout=null;
@SuppressLint("StaticFieldLeak")
static TextView text=null;

static boolean settext(String str) {
   var t=text;
   if(t!=null) {
       t.setText(str);
       return true;
    }
   return false;
   }
private static void rmlayoutUI() {
   var lay=layout;
   if(lay!=null) {
      text=null;
      layout=null;
      removeContentView(lay); 
      tk.glucodata.glucosecomplication.GlucoseValue.updateall();
      }
   }

static void rmlayout() {
     RunOnUiThread(Specific::rmlayoutUI);
     }
static void initScreen(MainActivity act) {
    LayoutInflater flater= LayoutInflater.from(act);
    ViewGroup layout = (ViewGroup) flater.inflate(R.layout.startview ,null, false);
    text=layout.findViewById(R.id.text2);
    Specific.layout=layout;
    act.addMyContentView(layout, new ViewGroup.LayoutParams(MATCH_PARENT, MATCH_PARENT));
   }

static void   blockedNum(MainActivity  act) {
    help.basehelp(R.string.staticnum,act,xzy->{ });
    }

static public boolean useclose=false;
static public void setclose(boolean val) {
   useclose=val;
   }

static void wearnosensors(MainActivity act) {
    Switch.wearnosensors(act);
    }
public static boolean isPackageInstalled(Context context, String packageName) {
    PackageManager pm = context.getPackageManager();
    try {
        if(Build.VERSION.SDK_INT >= 33) {
            pm.getPackageInfo(packageName, PackageManager.PackageInfoFlags.of(0));
           } 
        else {
           pm.getPackageInfo(packageName, 0);
           }
        return true;
        } 
    catch(Throwable th) {
        Log.stack(LOG_ID,"getPackageInfo",th);
        return false;
        }
  }

private static void playGoogle(Context context,String packageName) {
    var intent = new Intent(Intent.ACTION_VIEW);
    intent.setData(Uri.parse("https://play.google.com/store/apps/details?id="+packageName));
    intent.setPackage("com.android.vending");
    intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
    context.startActivity(intent);
   }
//private static final String watchface="tk.glucodata.watchfacepush.watchface2";
private static final String watchface="tk.glucodata.watchfacepush.watchface3";
static void installwatchface(Context context) {
    if(isPackageInstalled(context, watchface)) { return; }
    playGoogle(context,watchface); 
    Natives.setaskedWatchFace(true);
    }



    public static final class WatchFaceActivation {

        private static final String LOG_ID = "WfActivation";

        private static String pendingActivationSlotId = null;

        private WatchFaceActivation() {
        }

        public static void syncFromUi(@NonNull Activity activity) {
           Log.i(LOG_ID,"syncFromUi");
            WatchFacePushHelper.syncBundledWatchFace( activity, activity.getString(R.string.default_wf_token), new WatchFacePushHelper.SyncCallback() {
                        @Override
                        public void onComplete(@NonNull WatchFacePushHelper.SyncResult result) {
                            if (result.getDetails() == null || !result.shouldOfferActivation()) {
                                return;
                            }

                            // Ask only once ever.
                            if (!WatchFacePushHelper.consumeActivationAsk(activity)) {
                                Log.i(LOG_ID,"!consumeActivationAsk");
                                return;
                            }

                            pendingActivationSlotId = result.getDetails().getSlotId();

                            if (ContextCompat.checkSelfPermission( activity, WatchFacePushHelper.PERMISSION_SET_ACTIVE) == PackageManager.PERMISSION_GRANTED) {
                                        activatePending(activity);
                            } else {
                                Log.i(LOG_ID,"requestPermissions "+ WatchFacePushHelper.PERMISSION_SET_ACTIVE);

                                ActivityCompat.requestPermissions(
                                        activity,
                                        new String[]{WatchFacePushHelper.PERMISSION_SET_ACTIVE},
                                        REQ_SET_PUSHED_WATCH_FACE_ACTIVE
                                );
                            }
                        }

                        @Override
                        public void onError(@NonNull Throwable error) {
                            Log.stack(LOG_ID, "syncFromUi", error);
                        }
                    }
            );
        }

        public static void onRequestPermissionsResult( @NonNull Activity activity, boolean granted) {
            if (granted) {
                activatePending(activity);
            } else {
                Natives.setaskedWatchFace(true);
                pendingActivationSlotId = null;
                }
        }

        private static void activatePending(@NonNull Activity activity) {
            final String slotId = pendingActivationSlotId;
            pendingActivationSlotId = null;

            if (slotId == null) {
                Log.i(LOG_ID,"activatePending slotId==null");
                return;
            }
            Log.i(LOG_ID,"activatePending");
            WatchFacePushHelper.activateInstalledWatchFace( activity, slotId, new WatchFacePushHelper.SimpleCallback() { 
                      @Override
                        public void onSuccess() {
                            Natives.setaskedWatchFace(true);
                            Log.i(LOG_ID, "Watch face activated");
                        }

                        @Override
                        public void onError(@NonNull Throwable error) {
                            Log.stack(LOG_ID, "activatePending", error);
                        }
                    }
            );
        }
    }


};
