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
/*      Fri Jan 27 15:32:11 CET 2023                                                 */


package tk.glucodata;

import android.content.Context;
import android.content.pm.PackageManager;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraManager;
import android.os.Build;

import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;

import androidx.annotation.RequiresApi;

public class Flash  {
static private final String LOG_ID="Flash";
    private interface Flasher {
        void on();
        void off();
        void end();
    };

    @SuppressWarnings("deprecation")
    static private class oldflash implements Flasher {
        private Camera.Parameters parameters;
        private Camera mCamera;

       @SuppressWarnings("deprecation")
        oldflash(Context context) {
            try {
                mCamera = Camera.open();
                SurfaceTexture dummy = new SurfaceTexture(0);
                mCamera.setPreviewTexture(dummy);
                parameters = mCamera.getParameters();
            }
            catch(Throwable e) {
		       mCamera=null;
                    e.printStackTrace();
            }
        }
       @SuppressWarnings("deprecation")
        public void on() {
            Log.i(LOG_ID,"Flash on");
		if(mCamera==null)
			return;
            try {
                mCamera.reconnect();
                //noinspection deprecation
                parameters.setFlashMode(Camera.Parameters.FLASH_MODE_TORCH);
                //noinspection deprecation
                mCamera.setParameters(parameters);
                mCamera.startPreview();
            }catch (Throwable e) {
               e.printStackTrace();
            }
        }

       @SuppressWarnings("deprecation")
        public void off() {
		if(mCamera==null)
			return;
            Log.i(LOG_ID,"Flash off");
            try {
                //noinspection deprecation
                parameters = mCamera.getParameters();
                //noinspection deprecation
                parameters.setFlashMode(Camera.Parameters.FLASH_MODE_OFF);
                //noinspection deprecation
                mCamera.setParameters(parameters);
                mCamera.stopPreview();
            } catch (Throwable e) {
                e.printStackTrace();
            }
        }

       @SuppressWarnings("deprecation")
public    void end() {
	if(mCamera==null)
		return;
        mCamera.release();
	mCamera=null;

    }
    };

    @RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
static    class flash23 implements Flasher {

        private CameraManager camManager ;
        String cameraId = null;
        public void end() {

        };
        @RequiresApi(api = Build.VERSION_CODES.M)
        flash23(Context context) {
            try {
                camManager = (CameraManager) context.getSystemService(Context.CAMERA_SERVICE);
                if (camManager != null) {
                    var camas=camManager.getCameraIdList();
                    if(camas!=null&&camas.length>0) {
                        for (var c : camas) {
                            CameraCharacteristics cha = camManager.getCameraCharacteristics(c);
                            if (cha.get(CameraCharacteristics.FLASH_INFO_AVAILABLE)) {
                                cameraId = c;
                                break;
                            }
                        }
                        if (cameraId == null) {
                            Log.i(LOG_ID, "No cameraId=");
                            cameraId = camas[0];
                        }

                        Log.i(LOG_ID, "cameraId=" + cameraId);
                    }

                }
            } catch (CameraAccessException e) {
                    e.printStackTrace();
            }
        }
        @RequiresApi(api = Build.VERSION_CODES.M)
        void set(boolean val) {
            try {
                if(cameraId!=null) {
            	    Log.i(LOG_ID,"set("+val+")");
                    camManager.setTorchMode(cameraId, val);
		    }
		 else {
            	    Log.i(LOG_ID,"cameraID==null");
		 	}
            }
            catch(Throwable e) {
                    e.printStackTrace();
            }
        }
        @RequiresApi(api = Build.VERSION_CODES.M)
        public void on() {
            set(true);
        }
        @RequiresApi(api = Build.VERSION_CODES.M)
        public void off() {
            set(false);
        }
    };

static public boolean hasFlash(Context context) {
	try {
        if (context.getPackageManager().hasSystemFeature(PackageManager.FEATURE_CAMERA_ANY)&& context.getPackageManager().hasSystemFeature(
                    PackageManager.FEATURE_CAMERA_FLASH)) {
            // this device has a camera
            Log.i(LOG_ID,"Can Flash");
            return true;
        } else {
            Log.i(LOG_ID,"Can not Flash");
            // no camera on this device
            return false;
        }
	}
	catch(Throwable e) {
            e.printStackTrace();
            return false;
		}
    }
static    final ScheduledExecutorService scheduler = Executors.newScheduledThreadPool(1);
   static boolean onflag=false;
   static volatile boolean stopflash=false;;
static     Flasher flash=null;
   static  Runnable flashmore=() ->{
   	if(stopflash)
		return;
 //       tk.glucodata.Applic.RunOnUiThread(()-> {
 	var wasflash=flash;
	if(wasflash!=null) {
	    if (onflag) {
		wasflash.off();
	    } else wasflash.on();
	    onflag=!onflag;
	    }
  //      });
};
static    ScheduledFuture<?> flashschedule=null;
static void start(Context context) {

	if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.M) {
	    flash = new flash23(context);
	    }
	else {
		flash =new oldflash(context);
	}

	stopflash=false;
	flashschedule  = scheduler.scheduleAtFixedRate(flashmore, 0,150 , TimeUnit.MILLISECONDS);
	}
static synchronized void stop() {
	Log.i(LOG_ID,"stop");
	stopflash=true;
	onflag=true;
    if(flashschedule !=null)  {
            flashschedule.cancel(true);
	    flashschedule=null;
	    onflag=true;
 	//    Log.i(LOG_ID,"cancel");
	    }
    if(flash!=null) {
//tk.glucodata.Applic.RunOnUiThread(()-> {
        flash.off();
        flash.end();
        flash=null;
//	});
 	Log.i(LOG_ID,"endstop");
	}
	}

}
