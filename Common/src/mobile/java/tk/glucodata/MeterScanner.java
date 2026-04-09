/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2, Libre 3, Dexcom G7/ONE+,              */
/*      Sibionics GS1Sb and Accu-Chek SmartGuide sensors.                            */
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
/*      Fri Sep 19 14:46:04 CEST 2025                                                */

package tk.glucodata;

import static android.bluetooth.BluetoothProfile.GATT;
import static tk.glucodata.BluetoothGlucoseMeter.bluetoothIsEnabled;
import static tk.glucodata.BluetoothGlucoseMeter.mBluetoothAdapter;
import static tk.glucodata.BluetoothGlucoseMeter.mBluetoothManager;
import static tk.glucodata.BluetoothGlucoseMeter.meterGatts;
import static tk.glucodata.Log.doLog;
import static tk.glucodata.SensorBluetooth.scanStarts;

import android.annotation.SuppressLint;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanFilter;
import android.bluetooth.le.ScanResult;
import android.bluetooth.le.ScanSettings;
import android.os.Build;
import android.os.ParcelUuid;
import android.os.SystemClock;

import androidx.annotation.RequiresApi;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;

class MeterScanner  {
   boolean takeAidexX=false;
   List<BluetoothDevice> devices=new ArrayList<BluetoothDevice>();
   List<String> deviceNames=new ArrayList<String>();
   private static final String LOG_ID="MeterScanner";
   private boolean mScanning=false;
   private BluetoothLeScanner mBluetoothLeScanner=null;
   boolean knowName=true;
   MeterScanner() {
        knowName=true;
        }
   MeterScanner(DeviceList.DeviceListViewAdapter adapt) {
        knowName=false;
        adapter=adapt;
        }
   void clear() {
       devices=new ArrayList<BluetoothDevice>();
       deviceNames=new ArrayList<String>();
   }

   void reset() {
        takeAidexX=false;
        knowName=true;
        clear();
      adapter=null;
     }
   DeviceList.DeviceListViewAdapter adapter=null;
   private final ScanCallback mScanCallback = new ScanCallback() {
       private GlucoseMeterGatt  getCallback(BluetoothDevice device) {
            try {
                @SuppressLint("MissingPermission") String deviceName = device.getName();
                if(doLog) {Log.i(LOG_ID, "deviceName=" + deviceName);};
                if(deviceName == null) {
                    if(doLog) {Log.d(LOG_ID, "Scan returns device without name");};
                    return null;
                    }
                for (var cb : meterGatts) {
                    String deviceNameCand=cb.getDeviceName();
                    if(deviceName.equals(deviceNameCand)) {
                        cb.setDevice(device);
                        if(doLog) {Log.d(LOG_ID, "getCallBack match "+deviceName);};
                        return cb;
                        }
                    if(doLog) {Log.d(LOG_ID, "not: " + deviceNameCand);};
                    }
                return null;
            } catch(Throwable e) {
                Log.stack(LOG_ID,    "getCallback",e);
                if(!Applic.canBluetooth())
                    Applic.Toaster(R.string.turn_on_nearby_devices_permission);
                return null;
            }
            }
    @SuppressLint("MissingPermission")
    private boolean checkdevice(BluetoothDevice device) {
        try {
            GlucoseMeterGatt cb = getCallback(device);
            if(cb != null) {
                boolean newdev = true;
                if(cb.foundtime == 0L) {
                    cb.foundtime = System.currentTimeMillis();
                    int state;
                    if(cb.mBluetoothGatt != null && cb.mActiveBluetoothDevice == device && ((state = mBluetoothManager.getConnectionState(device, GATT)) == BluetoothGatt.STATE_CONNECTED || state == BluetoothGatt.STATE_CONNECTING)) {
                        newdev = false;
                        {if(doLog) {Log.i(LOG_ID,"old device connected state="+state);};};
                        }
                } else  {
                    newdev = false;
                    {if(doLog) {Log.i(LOG_ID,"old device connected foundtime="+cb.foundtime);};};
                    }

                boolean ret = true;
                cb.setDevice(device);
                for (GlucoseMeterGatt one : meterGatts) {
                    if (one.mActiveBluetoothDevice == null) {
                        if(doLog) {Log.i(LOG_ID, one.meterIndex + " not found");};
                            
                        ret = false;
                        break;
                    }
                }
                if(ret) stopScan(false);
                if(newdev) {
                    return ret&&cb.connectActiveDevice(0);
                    } return ret;
            }
            if(doLog) {Log.d(LOG_ID, "BLE unknown device");};;
            return false;
        } catch (Throwable e) {
            Log.stack(LOG_ID, "checkdevice", e);
            if (Build.VERSION.SDK_INT > 30 && !Applic.mayscan())
                Applic.Toaster(R.string.turn_on_nearby_devices_permission);
            return true;
        }
    }
private void addDevice(BluetoothDevice device) {
        var name=device.getName();
        if(name==null) {
                Log.i(LOG_ID,"addDevice skip: no name");
                return;
                }

         {
                if(deviceNames.contains(name)) {
                        Log.i(LOG_ID,"device already present "+name);
                        return;
                        }
                    Log.i(LOG_ID, "addDevice "+name);
                    devices.add(device);
                    deviceNames.add(name);
                    var adapt=adapter;
                    if(adapt!=null)
                        adapt.notifyDataSetChanged();
                   }
        }


       private synchronized boolean processScanResult(ScanResult scanResult) {
           if(!mScanning) {
                Log.i(LOG_ID,"!mScanning"); 
                return true;
                }
            var device=scanResult.getDevice();
            if(knowName)
                return checkdevice(device);
            else {
                addDevice(device);
                return false;
            }
            
            }

        @Override
        public void onScanResult(int callbackType, ScanResult scanResult) {
            if(doLog) {Log.d(LOG_ID,"onScanResult");};
            processScanResult(scanResult);
           }

        @Override
        public void onBatchScanResults(List<ScanResult> list) {
             if(doLog) {Log.v(LOG_ID,"onBatchScanResults");};
             final var len=list.size();
             for(int i=0;i < len&& !processScanResult(list.get(i));++i)
                               ;
             }
        @Override
        public void onScanFailed(int errorCode) {
              if(doLog) {
                       final String[] scanerror={"SCAN_0",
                               "SCAN_FAILED_ALREADY_STARTED",
                               "SCAN_FAILED_APPLICATION_REGISTRATION_FAILED",
                               "SCAN_FAILED_INTERNAL_ERROR",
                               "SCAN_FAILED_FEATURE_UNSUPPORTED" };
                       {if(doLog) {Log.d(LOG_ID,"BLE SCAN ERROR: scan failed with error code: " + ((errorCode<scanerror.length)?scanerror[errorCode]:"")+" "+errorCode);};};
                 }
              if(errorCode != SCAN_FAILED_ALREADY_STARTED) {
                stopScan(false);
                if(errorCode != SCAN_FAILED_FEATURE_UNSUPPORTED) {
                   scanStarter(scaninterval) ;
                   }
                  }
                 }
         };

private int scanTries=0;
private static final boolean alwaysfilter=true;
@SuppressLint("MissingPermission")
@RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
public boolean scanStarter()  {
        Log.i(LOG_ID,"scanStarter");
        mBluetoothLeScanner = mBluetoothAdapter.getBluetoothLeScanner();
        if(mBluetoothLeScanner!=null) {
            ScanSettings.Builder builder = new ScanSettings.Builder();
            builder.setReportDelay(0);
            var mScanSettings = builder.build();
           if(doLog) {Log.i(LOG_ID,"start");};
           List<ScanFilter> mScanFilters;
           if(alwaysfilter||scanTries++%2==0) {
               mScanFilters=new ArrayList<>();
               if(doLog) {Log.d(LOG_ID,"SCAN: starting scan.");};
               if(takeAidexX) {
                       var builder2 = new ScanFilter.Builder();
                       builder2.setServiceUuid(new ParcelUuid(AidexXGattCallback.ScanServiceUUID));
                       mScanFilters.add(builder2.build());
                       }
                else {
                       final UUID GLUCOSE_SERVICE =      UUID.fromString("00001808-0000-1000-8000-00805f9b34fb");
                       ScanFilter.Builder builder2 = new ScanFilter.Builder();
                       builder2.setServiceUuid(new ParcelUuid(GLUCOSE_SERVICE));
                       mScanFilters.add(builder2.build());
                       final var VERIO_F7A1_SERVICE = UUID.fromString("af9df7a1-e595-11e3-96b4-0002a5d5c51b");
                       builder2 = new ScanFilter.Builder();
                       builder2.setServiceUuid(new ParcelUuid(VERIO_F7A1_SERVICE));
                       mScanFilters.add(builder2.build());
                       }
              }
           else {
                  mScanFilters=null;
                  }
            try {
                 mBluetoothLeScanner.startScan(mScanFilters, mScanSettings, mScanCallback);
                 } 
             catch (Throwable e) {
                Log.stack(LOG_ID, e);
                if (Build.VERSION.SDK_INT > 30 && !Applic.mayscan()) Applic.Toaster(R.string.turn_on_nearby_devices_permission);
                return false;
                }
           return true;
           }
        return false;
         }
    @SuppressLint("MissingPermission")
    @RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
    public void scanStopper() {
        if(mBluetoothLeScanner!=null) {
            if(doLog) {Log.i(LOG_ID,"scanStopper");};
            try {
                mBluetoothLeScanner.stopScan(mScanCallback);
                } 
            catch (Throwable e) {
                    Log.stack(LOG_ID, "stopScan", e);
                  }
             };
        };
ScheduledFuture<?> scanFuture=null,timeoutFuture=null;

private static final int  scaninterval=60000;
public void stopScan(boolean retry) {
    if(doLog) {Log.d(LOG_ID,"Stop scanning "+(retry?"retry":"don't retry"));};
     if(scanFuture!=null) {
        scanFuture.cancel(true);
        scanFuture=null;
        }
     if(timeoutFuture!=null) {
        timeoutFuture.cancel(true);
        timeoutFuture=null;
        }
    if (mScanning) {
        mScanning = false;
        scanStopper();
        if(retry) {
           if(bluetoothIsEnabled()) {
                scanStarter( scaninterval);
                }
            }
        }
    }

final private Runnable mScanTimeoutRunnable = () -> {
    if(doLog) {Log.i(LOG_ID,"Timeout scanning");};
    stopScan(true);
};

private static final int scantimeout = 390000;
final private Runnable scanRunnable = new Runnable() {
   @Override 
   public void run() {
       if(doLog) {Log.i(LOG_ID,"scanRunnable");};;
       if(scanStarter()) {
               mScanning = true;
                timeoutFuture=Applic.scheduler.schedule(mScanTimeoutRunnable, scantimeout, TimeUnit.MILLISECONDS);
              }
        else {
              if(doLog) {Log.d(LOG_ID,"Start scan failed");};
              return;
             }
     }

 };


private void startScanGuarded() {
    long delay =SensorBluetooth.delayUntilScanAllowed();
    if(delay > 0) {
        scanFuture=Applic.scheduler.schedule(this::startScanGuarded, delay, TimeUnit.MILLISECONDS);
        return;
        }
    final  long elapsed= SystemClock.elapsedRealtime();
    scanStarts.addLast(elapsed);

    scanRunnable.run();
   }

public     boolean scanStarter(long delayMillis) {

    scanFuture=Applic.scheduler.schedule(this::startScanGuarded, delayMillis, TimeUnit.MILLISECONDS);
    return false;
    }
}
