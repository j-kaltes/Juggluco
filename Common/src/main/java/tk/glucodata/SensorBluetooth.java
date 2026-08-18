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
/*      Fri Jan 27 15:31:05 CET 2023                                                 */


package tk.glucodata;

import android.annotation.SuppressLint;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothManager;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanFilter;
import android.bluetooth.le.ScanResult;
import android.bluetooth.le.ScanSettings;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Build;
import android.os.ParcelUuid;
import android.os.SystemClock;

import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;

import androidx.annotation.RequiresApi;

import static android.bluetooth.BluetoothDevice.ACTION_BOND_STATE_CHANGED;
import static android.bluetooth.BluetoothDevice.BOND_BONDED;
import static android.bluetooth.BluetoothDevice.BOND_BONDING;
import static android.bluetooth.BluetoothDevice.BOND_NONE;
import static android.bluetooth.BluetoothDevice.EXTRA_BOND_STATE;
import static android.bluetooth.BluetoothProfile.GATT;
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.BuildConfig.libreVersion;
import static tk.glucodata.Log.doLog;
//import static tk.glucodata.Log.showScanSettings;
//import static tk.glucodata.Log.showScanfilters;

public class SensorBluetooth {
static void    setAutoconnect(boolean val) {
    Natives.setAndroid13(val);
//    if(!isWearable)
        SuperGattCallback.autoconnect=val;
    }
public static SensorBluetooth blueone=null;
public static void startscan() {
    if(blueone!=null)
        blueone.scanStarter(0L);
    }
    private static final String LOG_ID = "SensorBluetooth";
    private static final int scantimeout = 390000;
    private static final int  scaninterval=60000;

//   public Applic Applic.app;
 static   private BluetoothAdapter mBluetoothAdapter;
    private BroadcastReceiver mBluetoothAdapterReceiver =null; ;
static    private BluetoothManager mBluetoothManager=null;

 @SuppressLint("MissingPermission")
 void enableBluetooth() {
           if(!mBluetoothAdapter.isEnabled()) {
                mBluetoothAdapter.enable();
            } 
    }

static public void reconnectall() {
   Log.i(LOG_ID,"reconnectall");
   final var wasblue=blueone;
   if(wasblue !=null) {
       boolean shouldnotscan=true;
       final var now=System.currentTimeMillis();
        for(var cb: wasblue.gattcallbacks)     {
            shouldnotscan=(cb.reconnect(now)&&shouldnotscan);
            }
       if(!shouldnotscan)  {
            if(mBluetoothManager!=null) {
                 wasblue.stopScan(false);
                 wasblue.scanStarter(0L);
                 }
             }
        }
   }

static void othersworking(SuperGattCallback current ,long timmsec) {
   final var blue=blueone;
   if(blue!=null) {
       final var gatts=blue.gattcallbacks;
       if(gatts.size()>1) {
           for(var g:gatts) {
             if(g!=current)
                  g.shouldreconnect(timmsec);
             }
           }
         }
  }

private boolean connectToAllActiveDevices(long delayMillis) {
    if(doLog) {Log.i(LOG_ID, "connectToAllActiveDevices("+delayMillis+")");};
    if(!bluetoothIsEnabled()) {
        Applic.Toaster(R.string.enable_bluetooth);
        return false;
        }
    boolean scan=false;
    for(var cb: gattcallbacks)    
        if(!cb.connectDevice(delayMillis))  {
           scan=true;
           }
    if(scan) {
        return scanStarter(delayMillis);
        }
    return false;
    }
 public boolean connectToActiveDevice(SuperGattCallback cb,long delayMillis) {
    if(doLog) {Log.i(LOG_ID,"connectToActiveDevice("+cb.SerialNumber+"," + delayMillis+")");};
    if(!cb.connectDevice(delayMillis)&&!mScanning) {
        return scanStarter(delayMillis);
        }
    return false;
    }
//long unknownfound=0L;
//String unknownname="";
private SuperGattCallback  getCallback(BluetoothDevice device) {
    try {
        @SuppressLint("MissingPermission") String deviceName = device.getName();
        {if(doLog) {Log.i(LOG_ID, "deviceName=" + deviceName);};};
        if(deviceName == null) {
            {if(doLog) {Log.d(LOG_ID, "Scan returns device without name");};};
            return null;
            }
        String address = device.getAddress();
        for (var cb : gattcallbacks) {
            if (address.equals(cb.mActiveDeviceAddress))
                return cb;
            if(cb.matchDeviceName(deviceName,address)) {
                cb.mDeviceName=deviceName;
                return cb;
                }
            {if(doLog) {Log.d(LOG_ID, "not: " + cb.SerialNumber);};};
            }
        return null;
    } catch(Throwable e) {
        Log.stack(LOG_ID,    "getCallback",e);
        if(!Applic.canBluetooth())
            Applic.Toaster(R.string.turn_on_nearby_devices_permission);
        return null;
    }
}

//long foundtime=0L;

@SuppressLint("MissingPermission")
private boolean checkdevice(BluetoothDevice device,byte[] advertised) {
    try {
        SuperGattCallback cb = getCallback(device);
        if (cb != null&&cb.asAdvertised(advertised)) {
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
            for (SuperGattCallback one : gattcallbacks) {
                if (one.mActiveBluetoothDevice == null) {
                    {if(doLog) {Log.i(LOG_ID, one.SerialNumber + " not found");};};
                        
                    ret = false;
                    break;
                }
            }
            if(ret) SensorBluetooth.this.stopScan(false);
            if(newdev) {
                SensorBluetooth.this.connectToActiveDevice(cb, 0);
                }
            return ret;
        }
        if(doLog) {Log.d(LOG_ID, "BLE unknown device");};
        return false;
    } catch (Throwable e) {
        Log.stack(LOG_ID, "checkdevice", e);
        if (Build.VERSION.SDK_INT > 30 && !Applic.mayscan())
            Applic.Toaster(R.string.turn_on_nearby_devices_permission);
        return true;
    }
}

long scantimeouttime=0L;

boolean mScanning = false;
class Scanner21 implements Scanner  {
   final    private ScanSettings mScanSettings;
   private BluetoothLeScanner mBluetoothLeScanner=null;
     @RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
     private final ScanCallback mScanCallback = new ScanCallback() {
    @RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
       private synchronized boolean processScanResult(ScanResult scanResult) {
       if(!mScanning) {
            Log.i(LOG_ID,"!mScanning"); 
            return true;
            }
       if(gattcallbacks.size()<1) {
            {if(doLog) {Log.w(LOG_ID,"No Sensors to search for");};};
            SensorBluetooth.this.stopScan(false);
            return true;
             }

        return checkdevice(scanResult.getDevice(),scanResult.getScanRecord().getBytes());
        }
//    private  boolean resultbusy=false;

        @RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
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
            SensorBluetooth.this.stopScan(false);
            if(errorCode != SCAN_FAILED_FEATURE_UNSUPPORTED) {
               SensorBluetooth.this.scanStarter(scaninterval) ;
               }
              }
             }
         };
private static final  boolean alwaysfilter=true;
    @RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
    Scanner21() {
        ScanSettings.Builder builder = new ScanSettings.Builder();
        builder.setReportDelay(0);
        // Low-power (the default) delivers scan results slowly with the screen off; the reconnect
        // scan is a short-lived fallback, so low-latency rediscovers the sensor much faster.
        builder.setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY);
        mScanSettings = builder.build();
        {if(doLog) {Log.i(LOG_ID,"Scanner21");};};
        }
    @RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
    public boolean init() {
        {if(doLog) {Log.i(LOG_ID,"Scanner21.init");};};
           return ( (mBluetoothLeScanner = SensorBluetooth.mBluetoothAdapter.getBluetoothLeScanner())!=null);
           }


private int scanTries=0;
    @SuppressLint("MissingPermission")
    @RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
    public boolean start()  {
        if(mBluetoothLeScanner!=null) {
           if(doLog) {Log.i(LOG_ID,"Scanner21.start");};
           List<ScanFilter> mScanFilters=new ArrayList<>();
           if(alwaysfilter||scanTries++%2==0) {
               if(doLog) {Log.d(LOG_ID,"SCAN: starting scan.");};
               for(var cb: gattcallbacks)   {
                   if(doLog) {
                         final var address=Natives.getDeviceAddress(cb.dataptr,false);
                         if(address!= null) {
                              Log.d(LOG_ID,"serial number: " + cb.SerialNumber+" address: " + address);
                             }
                        else {
                             Log.d(LOG_ID,"serial number: " + cb.SerialNumber+" no address");
                             }
                         }
                   final var service=cb.getService();
                   if(service==null) {
                      if(doLog) {Log.i(LOG_ID,"SCAN: getService should return UUID");};
                      mScanFilters=null;
                      }
                   else {
                      if(mScanFilters!=null) {
                          if(doLog) {Log.i(LOG_ID,"SCAN: filter "+service.toString());};
                          ScanFilter.Builder builder2 = new ScanFilter.Builder();
                          builder2.setServiceUuid(new ParcelUuid(service));
                          mScanFilters.add(builder2.build());
                          }
                      }
                  }
              }
           else {
                  mScanFilters=null;
                  if(doLog) {Log.i(LOG_ID,"SCAN: start no filter ");};

                  }
            try {
                 this.mBluetoothLeScanner.startScan(mScanFilters, mScanSettings, mScanCallback);
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
    public void stop() {
        if(mBluetoothLeScanner!=null) {
            {if(doLog) {Log.i(LOG_ID,"Scanner21.stop");};};
            try {
                mBluetoothLeScanner.stopScan(mScanCallback);
                } 
            catch (Throwable e) {
                    Log.stack(LOG_ID,  e);
                }
            }
        };

        };
    @SuppressWarnings("deprecation")
class ArchScanner  implements Scanner {
   BluetoothAdapter.LeScanCallback mLeScanCallback= new BluetoothAdapter.LeScanCallback () {
        @Override
    public  void onLeScan (BluetoothDevice device, int rssi, byte[] scanRecord) {
        checkdevice(device,scanRecord);
        }
    }    ;
    public boolean init() {
        {if(doLog) {Log.i(LOG_ID,"ArchScanner.init");};};
        return true;
           }
    @SuppressLint("MissingPermission")
    public boolean start()  {
       if(doLog) {Log.d(LOG_ID,"SCAN: starting scan.");};
       switch(gattcallbacks.size()) {
             case 0: Log.e(LOG_ID,"nothing to scan for");return false;
            case 1:
                final var service=gattcallbacks.get(0).getService();
                if(service!=null) {
                      return SensorBluetooth.mBluetoothAdapter.startLeScan(new UUID[] {service},mLeScanCallback);
                      }
            }
        return SensorBluetooth.mBluetoothAdapter.startLeScan(mLeScanCallback);
          }
    @SuppressLint("MissingPermission")
    public void stop() {
         SensorBluetooth.mBluetoothAdapter.stopLeScan( mLeScanCallback);
        };

};

Scanner scanner=Build.VERSION.SDK_INT >= 21?new Scanner21():new ArchScanner();

final private Runnable mScanTimeoutRunnable = () -> {
    {if(doLog) {Log.i(LOG_ID,"Timeout scanning");};};
    scantimeouttime=System.currentTimeMillis();
    SensorBluetooth.this.stopScan(true);
};




static boolean bluetoothIsEnabled() {
var adapt=mBluetoothAdapter;
if(adapt!= null) {
    return  adapt.isEnabled();
    }
   
   if(!Natives.getusebluetooth()) {
        return false;
        }
//   start(true);
    if(mBluetoothManager ==null) {
        if(blueone==null) {
               blueone=new tk.glucodata.SensorBluetooth();
            if(blueone==null)
                 return false;
              }
        blueone.initializeBluetooth();
        adapt=mBluetoothAdapter;
        if(adapt!= null) 
                return  adapt.isEnabled();
        }
     return false;
   }


static public void sensorEnded(String str) {
   if(blueone!=null)  {
      blueone.removeDevice(str) ;
      }
    }



private boolean scanstart=false;    
long scantime=0L;
final private Runnable scanRunnable = new Runnable() {
   @Override 
   public void run() {
       if(doLog) {Log.i(LOG_ID,"scanRunnable");};;
       scantime=System.currentTimeMillis();
       SensorBluetooth sensorBluetooth = SensorBluetooth.this;
       if (bluetoothIsEnabled() && gattcallbacks.size() != 0) {
           if (!scanner.init()) {
                 return;
               }
           if(scanner.start()) {
               mScanning = true;
               if(scanOnUI) {
                  Applic.app.getHandler().postDelayed(mScanTimeoutRunnable, scantimeout);
                  }
             else {
                timeoutFuture=Applic.scheduler.schedule(mScanTimeoutRunnable, scantimeout, TimeUnit.MILLISECONDS);
                }
              }
            else {
                  if(doLog) {Log.d(LOG_ID,"Start scan failed");};
                  return;
                 }
           }        
     }

 };

  static private final   boolean scanOnUI=false;
ScheduledFuture<?> scanFuture=null,timeoutFuture=null;

private static final int MAX_SCAN_STARTS = 5;
private static final long WINDOW_MS = 31_000L;
static  final ArrayDeque<Long> scanStarts = new ArrayDeque<>();


static long delayUntilScanAllowed() {
    // Prune the sliding window, check the Samsung ~5-starts/31s budget, and record this start
    // atomically under the scanStarts monitor. Without the lock the scheduler-thread prune/peek
    // races the main-thread clear() in the BT-state receiver, mis-counting the window and either
    // over-delaying reconnect or tripping Samsung's silent scan throttle (~30s of no results).
    synchronized(scanStarts) {
        long now = SystemClock.elapsedRealtime();
        while(!scanStarts.isEmpty() && (now - scanStarts.peekFirst()) >= WINDOW_MS) {
            scanStarts.removeFirst();
        }
        if (scanStarts.size() < MAX_SCAN_STARTS) {
            scanStarts.addLast(now);
            return 0L;
            }
        return WINDOW_MS - (now - scanStarts.peekFirst());
        }
}

private void startScanGuarded() {
    long delay = delayUntilScanAllowed();
    if(delay > 0) {
        Log.i(LOG_ID,"delayUntilScanAllowed()="+delay);
        if(scanOnUI) {
            Applic.getHandler().postDelayed(this::startScanGuarded, delay);
             }
         else {
            scanFuture=Applic.scheduler.schedule(this::startScanGuarded, delay, TimeUnit.MILLISECONDS);
            }
        return;
        }
    // This scan start was already recorded atomically inside delayUntilScanAllowed() above.
    scanRunnable.run();
   // bluetoothLeScanner.startScan(scanCallback);
   }


private     boolean scanStarter(long delayMillis) {
      {if(doLog) {Log.i(LOG_ID,"scanStarter("+delayMillis+")");};};
      var main=MainActivity.thisone;
        if(!((main==null&&Applic.mayscan())||(main!=null&&main.finepermission())) ) {
          Applic.Toaster((Build.VERSION.SDK_INT > 30)?R.string.turn_on_nearby_devices_permission: R.string.turn_on_location_permission );
         return true;
         }

    if(!bluetoothIsEnabled()) {
        Applic.Toaster(R.string.bluetooth_is_turned_off);
        return false;
        }
         scanstart=true;    

    if(scanOnUI) {
        if(delayMillis>0)
            Applic.getHandler().postDelayed(this::startScanGuarded,delayMillis);
        else
            Applic.getHandler().post(this::startScanGuarded);
        }
    else {
        scanFuture=Applic.scheduler.schedule(this::startScanGuarded, delayMillis, TimeUnit.MILLISECONDS);
        }
    return false;
    }
long stopscantime=0L;
private static final int startincreasedwait=300000;
private int increasedwait=startincreasedwait;
private void stopScan(boolean retry) {
        if(doLog) {Log.d(LOG_ID,"Stop scanning "+(retry?"retry":"don't retry"));};
        if(scanOnUI) {
            Applic.app.getHandler().removeCallbacks(this.scanRunnable);
            Applic.app.getHandler().removeCallbacks(this.mScanTimeoutRunnable);
            }
       else {
             if(scanFuture!=null) {
                scanFuture.cancel(true);
                scanFuture=null;
                }
             if(timeoutFuture!=null) {
                timeoutFuture.cancel(true);
                timeoutFuture=null;
                }
           }
        if (this.mScanning) {
            stopscantime=System.currentTimeMillis();
            this.mScanning = false;
            scanner.stop();
            if(retry) {
               if(bluetoothIsEnabled()) {
                    int waitscan=scaninterval;
                    if(scantime>0L) {
                       for(SuperGattCallback cb:gattcallbacks) {
                           if(cb.foundtime>scantime&&SuperGattCallback.lastfound()>cb.foundtime) {
                               increasedwait*=2;
                               waitscan=increasedwait;
                               }
                            }
                        }
                    scanStarter( waitscan);
                    }
                }
        }
    scanstart=false;
    }


ArrayList<SuperGattCallback> gattcallbacks=new ArrayList<SuperGattCallback>();

static  ArrayList<SuperGattCallback>  mygatts() {
    if(blueone==null|| blueone.gattcallbacks.size()==0)
        return null;
    return blueone.gattcallbacks;
    }
private void removeDevice(String str) {
    for(int i=0;i<gattcallbacks.size();i++) {
        var gatt= gattcallbacks.get(i);
        if(str.equals(gatt.SerialNumber)) {
            {if(doLog) {Log.i(LOG_ID,"removeDevice "+ gatt.SerialNumber);};};
            gatt.free();
            gattcallbacks.remove(i);
            final int len=gattcallbacks.size();
            Natives.setmaxsensors(len);
            if(len==0) {
                LossOfSensorAlarm.cancelalarm();
                }
            for(;i<gattcallbacks.size();++i) {
                gatt= gattcallbacks.get(i);
                gatt.stopHealth=false;
                }
            return;
            }
        else {
            gatt.stopHealth=false;
            }
        }    
    {if(doLog) {Log.i(LOG_ID,"removeDevice: didn't remove"+ str);};};
    }
private void removeDevices() {
    {if(doLog) {Log.i(LOG_ID,"removeDevices()");};};
    for(int i=0;i<gattcallbacks.size();i++) {
        gattcallbacks.get(i).free();
        }
    gattcallbacks.clear();
    Natives.setmaxsensors(0);
    }

private void destruct() {
    removeReceivers();
    if(mBluetoothManager!=null)   {
        stopScan(false);
        removeDevices();
        }
    }

public static void destructor() {
    var bluetmp=blueone;
    if(bluetmp!=null) {
        {if(doLog) {Log.i(LOG_ID,"destructor blueone!=null");};};
        bluetmp.destruct();
        blueone=null;
        }
    else
        {if(doLog) {Log.i(LOG_ID,"destructor blueone==null");};};

    }
//static boolean nullKAuth=false;

private void setDevices(String[] names) {
    for(String name:names)  { 
        if(name!=null) {
            {if(doLog) {Log.i(LOG_ID,"setDevice "+name);};};
            long dataptr=Natives.getdataptr(name);
            if(dataptr!=0) {
                gattcallbacks.add(getGattCallback(name,dataptr));
                }
            increasedwait=startincreasedwait;
            }
        }
    Natives.setmaxsensors(gattcallbacks.size());
    }
public void startDevices(String[] names) {
    setDevices(names);
    initializeBluetooth();
    }

public boolean resetDevices() {
    if(!Natives.getusebluetooth()) {
        {if(doLog) {Log.d(LOG_ID,"resetDevices !getusebluetooth()");};};
        return false;
        }
    if(mBluetoothManager!=null)   
        stopScan(false);
    removeDevices() ;
    setDevices(Natives.activeSensors());
    return initializeBluetooth();
    }

static <T> int indexOf(final T[] ar,final T el) {
    for(int i=0;i<ar.length;i++)
        if(el.equals(ar[i]))
            return i;
    return -1;
    }

public void connectNamedDevice(String id,long delayMillis) {
      for(var cb: gattcallbacks)    {
         if(id.equals(cb.SerialNumber)) {
            if(!cb.connectDevice(delayMillis))  {
                 scanStarter(delayMillis);
               }
             return;
            }
          }
        }
       /*
public void refreshNamedDevice(String id) {
    Log.i(LOG_ID,"refreshNamedDevice "+id);
      for(var cb: gattcallbacks)    {
         if(id.equals(cb.SerialNumber)) {
             if(cb instanceof Libre3GattCallback)
                ((Libre3GattCallback) cb).doSomething() ;
             return;
            }
          }
        } */
public boolean connectDevices(long delayMillis) {
    Log.i(LOG_ID,"connectDevices "+delayMillis);
    if(!bluetoothIsEnabled()) {
        Applic.Toaster(R.string.enable_bluetooth);
        return false;
        }
    boolean scan=false;
    for(var cb: gattcallbacks)    {
        if(checkandconnect( cb,delayMillis))
            scan=true;
    }
    if(scan) {
        return scanStarter(delayMillis);
    }
  return false;
    }
boolean updateDevicers() {
    if(!Natives.getusebluetooth()) {
        {if(doLog) {Log.d(LOG_ID,"updateDevicers !getusebluetooth()");};};
        destruct(); 
        blueone=null;
        return false;
        }
    {if(doLog) {Log.i(LOG_ID,"updateDevicers");};};
    String[] devs=Natives.activeSensors();
    ArrayList<Integer> rem = new ArrayList<>();
    int gatnr=gattcallbacks.size();
    {if(doLog) {Log.i(LOG_ID,"updateDevicers nr active: "+gatnr);};};
    if(devs==null) {
        for(int i=0;i<gatnr;i++) {
            String was= gattcallbacks.get(i).SerialNumber;
            {if(doLog) {Log.i(LOG_ID,"null: can remove "+ was);};};
            rem.add(i);    
            }
        if(rem.size()==0)  {
            {if(doLog) {Log.i(LOG_ID,"Nothing to remove, no active sensors");};};
            return false;
            }
        }
    else {
        
        int heb=0;

        for(int i=0;i<gatnr;i++) {
            var gatt= gattcallbacks.get(i);
            String was= gatt.SerialNumber;
            int instr=was==null?-1:indexOf(devs,was);
            if(instr<0) {
                {if(doLog) {Log.i(LOG_ID,"can remove "+ was);};};
                rem.add(i);    
                }
            else {
                gatt.stopHealth=false;
                {if(doLog) {Log.i(LOG_ID,"keep "+ was);};};
                heb++;
                devs[instr]=null;        
                }
            }
        if(devs.length==heb&&rem.size()==0) {
            {if(doLog) {Log.i(LOG_ID,"Nothing has to be done");};};
            return false;
            }
        }
    {if(doLog) {Log.i(LOG_ID,"needs update sensors");};};
    if(mBluetoothManager!=null)   
        stopScan(false);
//    rem.sort((x,y)->{return x-y;});
     Collections.sort(rem, (x, y)->{return x-y;});

    for(int el=rem.size()-1;el>=0;el--) {
        int weg=rem.get(el);
        {if(doLog) {Log.i(LOG_ID,"remove "+ gattcallbacks.get(weg).SerialNumber);};};
        gattcallbacks.get(weg).free();
        gattcallbacks.remove(weg);
        }
    int index=gattcallbacks.size();
    if(devs!=null){
        for(String dev:devs) {
            if(dev!=null) {
                {if(doLog) {Log.i(LOG_ID,"add "+dev);};};
                long dataptr=Natives.getdataptr(dev);
                if(dataptr!=0L) {
                    gattcallbacks.add(getGattCallback(dev,dataptr));
                    increasedwait=startincreasedwait;
                    index++;
                    }
                }
            }
        }

//    nullKAuth=false;
//    Natives.setmaxsensors(gattcallbacks.size());
    if(mBluetoothManager ==null) {
        return initializeBluetooth();
        }
    else {
        addReceivers();
        return connectDevices(0);
        }
//           scanStarter(0);
    }

static boolean updateDevices() {
    if(blueone==null) {
        return false;
        }
    return  blueone.updateDevicers();
    }

boolean checkandconnect(SuperGattCallback  cb,long delay) {
    if(doLog) {Log.i(LOG_ID,"checkandconnect("+cb.SerialNumber+","+ delay+")");};
    if(cb.mActiveDeviceAddress != null) {
        if(BluetoothAdapter.checkBluetoothAddress(cb.mActiveDeviceAddress)) {
            {if(doLog) {Log.i(LOG_ID, cb.SerialNumber+" checkBluetoothAddress(" +cb.mActiveDeviceAddress +") succeeded");};};
            cb.mActiveBluetoothDevice = mBluetoothAdapter.getRemoteDevice(cb.mActiveDeviceAddress);
            connectToActiveDevice(cb, delay);
            return false;
          }     
       if(doLog) {Log.i(LOG_ID, cb.SerialNumber+" checkBluetoothAddress(" +cb.mActiveDeviceAddress +") failed");};
       cb.setDeviceAddress(null);
       }

    var main=MainActivity.thisone;
    if((main==null&&Applic.mayscan())||main.finepermission()) {
        connectToActiveDevice(cb, delay);
        return false;
        }
    return true;
    }
SuperGattCallback getGattCallback(String name, long dataptr) {
    if(libreVersion==3||tk.glucodata.BuildConfig.SiBionics==1||tk.glucodata.BuildConfig.DexCom==1) {
        int vers = Natives.getLibreVersion(dataptr);
        if(libreVersion==3) {
            if (vers == 3) {
                return new Libre3GattCallback(name, dataptr);
                }
            }
        if(tk.glucodata.BuildConfig.DexCom==1) {
            if(vers==0x40) {
                return new DexGattCallback(name, dataptr);
                }
            if(vers==0x20) {
                return new AccuGattCallback(name, dataptr);
                }
            if(vers==0x30) {
                return new AirGattCallback(name, dataptr);
                }
            if(vers==0x50) {
                return new AidexXGattCallback(name, dataptr);
                }
            }
        if(tk.glucodata.BuildConfig.SiBionics==1) {
            if(vers==0x15) {
                return new Si3GattCallback(name, dataptr);
                }
            if(vers==0x10) {
                return new SiGattCallback(name, dataptr);
                }
            }
        }
    return  new Libre2GattCallback(name,dataptr);
    }
private boolean addDevice(String str,long dataptr) {
    {if(doLog) {Log.d(LOG_ID,"addDevice "+str);};};
    int index=gattcallbacks.size();
    if(dataptr!=0L) {
        SuperGattCallback  cb= getGattCallback(str,dataptr);
//        nullKAuth=false;
        gattcallbacks.add(cb);
        Natives.setmaxsensors(gattcallbacks.size());
        increasedwait=startincreasedwait;
        if(mBluetoothManager ==null) {
            return initializeBluetooth();
            }
        else  {
            addReceivers();
            return checkandconnect( cb,0);
            }
        }
    else {
        Log.e(LOG_ID,"dataptr==0L");
     }    
    return     false;


    }
private boolean resetDevicer(long streamptr,String name) {
    if(mBluetoothManager!=null)   
        stopScan(false);
    for(int i=0;i<gattcallbacks.size();i++) {
        SuperGattCallback  cb= gattcallbacks.get(i);
       if(Natives.sameSensor(streamptr,cb.dataptr)) {
         {if(doLog) {Log.d(LOG_ID,"reset free "+name);};};
         cb.resetdataptr();
         return checkandconnect(cb,0);
         }
      }
    return addDevice(name,streamptr);
    }

static public boolean resetDeviceOrFree(long ptr,String name) {
    if(blueone!=null) {
        return blueone.resetDevicer(ptr,name);
        }
    else
        Natives.updateUsedSensors( );
    Natives.freedataptr(ptr);
    return false;
    }
private boolean resetDevicer(String str,long[] ptrptr) {
    if(str==null) {
        ptrptr[0]=0L;
        return false;
        }
    if(mBluetoothManager!=null)   
        stopScan(false);
    for(int i=0;i<gattcallbacks.size();i++) {
        if(str.equals(gattcallbacks.get(i).SerialNumber)) {
            if(doLog) {Log.d(LOG_ID,"reset free "+str);};
            SuperGattCallback  cb= gattcallbacks.get(i);
            ptrptr[0]=cb.resetdataptr();
            return checkandconnect(cb,0);
            }
        }

    {if(doLog) {Log.d(LOG_ID,"reset add "+str);};};
    final long dataptr=Natives.getdataptr(str);
    ptrptr[0]=dataptr;
    return addDevice(str,dataptr);
    }

static public boolean resetDevice(String str) {
    long[] ptrptr={0L};
    var ret=resetDevicePtr(str,ptrptr);
    SuperGattCallback.glucosealarms.setLossAlarm();
    return ret;
    }
static private boolean resetDevicePtr(String str,long[] ptrptr) {
    {if(doLog) {Log.v(LOG_ID,"resetDevice("+str+")");};};
    if(!Natives.getusebluetooth()) {
        Natives.updateUsedSensors( );
        return false;
        }
    if(blueone==null) {
           blueone=new tk.glucodata.SensorBluetooth();
          }
    return blueone.resetDevicer(str,ptrptr);
    }


static public   void goscan() { 
    if(blueone!=null) {
        blueone.connectToAllActiveDevices(0);
        }
    }

    public SensorBluetooth() {
        if(doLog) {Log.v(LOG_ID,"SensorBluetooth");};
        SuperGattCallback.autoconnect=Natives.getAndroid13();

    }

static void start(boolean usebluetooth) {
    final var sensors=Natives.activeSensors();
    final boolean hasSensors= sensors!=null&&sensors.length>0; 
    if(hasSensors) {
            SuperGattCallback.glucosealarms.setLossAlarm();
            }
    if(doLog) {Log.v(LOG_ID,"SensorBluetooth.start("+usebluetooth+")");};
    if(usebluetooth) {
        if(SensorBluetooth.blueone==null) {
            blueone=new tk.glucodata.SensorBluetooth();
            if(blueone!=null&&hasSensors) {
                blueone.startDevices(sensors);
                }
            }
        else {
            if(hasSensors)
                 blueone.connectDevices(0);
            }
        }
    }
static final boolean keepBluetooth=false;

private void removeBluetoothStateReceiver() {
    var rec= mBluetoothAdapterReceiver;
    mBluetoothAdapterReceiver=null;
    if(rec!=null) {
        try {
            Applic.app.unregisterReceiver(rec);
            }
        catch(Throwable th) {
            Log.stack(LOG_ID, "removeBluetoothStateReceiver",th);
            }
        }
    }
private void addBluetoothStateReceiver() {
    if(mBluetoothAdapterReceiver==null) {
     mBluetoothAdapterReceiver=new BroadcastReceiver() {
//        private boolean wasScanning=false;
        @SuppressLint("MissingPermission")
        @Override
        public void onReceive(Context context, Intent intent) {
            if ("android.bluetooth.adapter.action.STATE_CHANGED".equals(intent.getAction())) {
            int intExtra = intent.getIntExtra("android.bluetooth.adapter.extra.STATE", -1);
            if (intExtra == BluetoothAdapter.STATE_OFF) {
                {if(doLog) {Log.v(LOG_ID,"BLUETOOTH switched OFF");};};

                // wasScanning=mScanning; 
                SensorBluetooth.this.stopScan(false);
                for(var cb: gattcallbacks)  {
                    if(cb.constatchange[1]<cb.constatchange[0] ) {
                        cb.constatchange[1] = System.currentTimeMillis();
                        cb.constatstatusstr="Bluetooth off"; //"
                        }
                    cb.close();
                    }
                 synchronized(scanStarts) { scanStarts.clear(); }
                if(keepBluetooth) mBluetoothAdapter.enable();
                } 
            else if (intExtra == BluetoothAdapter.STATE_ON) {
                {if(doLog) {Log.v(LOG_ID,"BLUETOOTH switched ON");};};
                if(!isWearable) {
                    Applic.app.numdata.startall();
                    }
//                if(wasScanning) { SensorBluetooth.this.scanStarter(250L); }
                SensorBluetooth.this.connectToAllActiveDevices(500);
            }
            }
        }
        };
        Applic.app.registerReceiver( mBluetoothAdapterReceiver, new IntentFilter("android.bluetooth.adapter.action.STATE_CHANGED"));
        }
    }

private  BroadcastReceiver pairingRequestReceiver=null;

private void addPairingRequestReceiver() {
    removePairingRequestReceiver();
    try {
         pairingRequestReceiver = new BroadcastReceiver() {
            @Override
    public void onReceive(Context context, Intent intent) {
        try {
        if(doLog)
                Log.i(LOG_ID,"onReceive ACTION_PAIRING_REQUEST");;
        final BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
        if(doLog)
            Log.i(LOG_ID,"PAIRING_REQUEST got device");
        final var tmp=blueone;
        if(tmp==null) {
            {if(doLog) {Log.i(LOG_ID,"Pairing: no SensorBluetooth");};};
            return;
            }
        if(device==null) {
            Log.e(LOG_ID,"Pairing: BluetoothDevice.EXTRA_DEVICE ==null");
            return ;
            }
        String address=device.getAddress();
        if(address==null) {
            Log.e(LOG_ID,"Pairing: device.getAddress()==null");
            return;
            }
        if(doLog)
            Log.i(LOG_ID,"PAIRING_REQUEST "+address);
        final String action = intent.getAction();
        if(action==null) {
            Log.e(LOG_ID,"Pairing: action==null");
            return;
            }
        Log.i(LOG_ID,"PAIRING_REQUEST "+action);
        for(var cb: tmp.gattcallbacks)     {
            if(cb.mActiveDeviceAddress!=null) {
                if(address.equals(cb.mActiveDeviceAddress)) {
                    if(cb.pairingRequest())
                        abortBroadcast();
                    return;
                    }
                }
            }
        if(doLog) {Log.i(LOG_ID,"Pairing: no sensor matches address "+address);};

        } catch(Throwable th) {
            Log.stack(LOG_ID,"PAIRING_REQUEST",th);
            }


            }
        };
         Applic.app.registerReceiver(pairingRequestReceiver, new IntentFilter(BluetoothDevice.ACTION_PAIRING_REQUEST));
         } 
    catch (Throwable e) {
        Log.stack(LOG_ID, "registerReceiver ACTION_PAIRING_REQUEST", e);
        }
    }
private void removePairingRequestReceiver() {
    var rec=pairingRequestReceiver;
    if(rec!=null) {
        try {
            Applic.app.unregisterReceiver(rec);
             } 
        catch (Throwable e) {
            Log.stack(LOG_ID, "unregisterReceiver ACTION_PAIRING_REQUEST", e);
            }
       finally {
        pairingRequestReceiver=null;
        }
       }
    }


private void addReceivers() {
   addBluetoothStateReceiver();
   addBondStateReceiver() ;
//   if(Build.VERSION.SDK_INT <26)
   addPairingRequestReceiver();
    }
private void removeReceivers() {
   removeBluetoothStateReceiver();
   removeBondStateReceiver() ;
   // addReceivers() registers the pairing receiver on every API level, so always unregister it.
   // The old SDK<26 guard leaked one registration per teardown on API >= 26 (the S24 FE).
   removePairingRequestReceiver();
    }
private BroadcastReceiver bondStateReceiver =null;
private void addBondStateReceiver() {
    // Idempotent: addReceivers() can run more than once; without this each call leaked a
    // BroadcastReceiver registration (removeBondStateReceiver can only drop the latest one).
    if(bondStateReceiver!=null)
        removeBondStateReceiver();
    bondStateReceiver=new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
        final BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
        final var tmp=blueone;
        if(tmp==null) {
            {if(doLog) {Log.i(LOG_ID,"Bond Broadcast: no SensorBluetooth");};};
            return;
            }
        if(device==null) {
            Log.e(LOG_ID,"Bond Broadcast: BluetoothDevice.EXTRA_DEVICE ==null");
            return ;
            }
        String address=device.getAddress();
        if(address==null) {
            Log.e(LOG_ID,"Bond Broadcast: device.getAddress()==null");
            return;
            }
        final String action = intent.getAction();
        if(action==null) {
            Log.e(LOG_ID,"Bond Broadcast: action==null");
            return;
            }
        for(var cb: tmp.gattcallbacks)     {
            if(cb.mActiveDeviceAddress!=null) {
                if(address.equals(cb.mActiveDeviceAddress)) {
                   if(action.equals(ACTION_BOND_STATE_CHANGED)) {
                      final int bondState = intent.getIntExtra(EXTRA_BOND_STATE, BluetoothDevice.ERROR);
                      final int previousBondState = intent.getIntExtra(BluetoothDevice.EXTRA_PREVIOUS_BOND_STATE, -1);
                      switch (bondState) {
                            case BOND_BONDING:
                                {if(doLog) {Log.i(LOG_ID,"Broadcast: BOND_BONDING "+address);};};
                                break;
                            case BOND_BONDED:
                                {if(doLog) {Log.i(LOG_ID,"Broadcast: BOND_BONDED "+address);};};
                                break;
                            case BOND_NONE:
                                {if(doLog) {Log.i(LOG_ID,"Broadcast: BOND_NONE "+address);};};
                                break;
                            case BluetoothDevice.ERROR:
                                {if(doLog) {Log.i(LOG_ID,"Broadcast: ERROR "+address);};};
                                break;
                              default:
                                {if(doLog) {Log.i(LOG_ID,"Broadcast: "+bondState+ " "+address);};};
                            }
                    }
                    cb.bonded();
                    return;
                    }
                }
            }
        {if(doLog) {Log.i(LOG_ID,"Bond Broadcast: no sensor matches address "+address);};};
        }
    };
    try {
        Applic.app.registerReceiver(bondStateReceiver, new IntentFilter(ACTION_BOND_STATE_CHANGED));
        }
    catch(Throwable th) {
        Log.stack(LOG_ID, "addBondStateReceiver", th);
        }
    }

private void removeBondStateReceiver() {
    final var rec=bondStateReceiver;
    bondStateReceiver=null;
    if(rec!=null) {
        try {
            Applic.app.unregisterReceiver(rec);
            }
        catch(Throwable th) {
            Log.stack(LOG_ID, "removeBondStateReceiver",th);
            }
        }
    }

private boolean initializeBluetooth() {
        {if(doLog) {Log.v(LOG_ID,"initializeBluetooth");};};
        if(!Applic.canBluetooth()) {
                Applic.Toaster(R.string.turn_on_nearby_devices_permission);
                Log.e(LOG_ID,"No Blueotooth permission");
                return false;
                } 
//        mBluetoothManager = (BluetoothManager) Applic.app.getSystemService("bluetooth");
        mBluetoothManager = (BluetoothManager) Applic.app.getSystemService(Context.BLUETOOTH_SERVICE);
        if (mBluetoothManager  == null) {
            {if(doLog) {Log.i(LOG_ID, "getSystemService(\"BLUETOOTH_SERVICE\")==null");};};
        } else {
            mBluetoothAdapter = mBluetoothManager.getAdapter();
            if (mBluetoothAdapter == null) {
                if(doLog) {Log.i(LOG_ID, "bluetoothManager.getAdapter()==null");};
            } else {
                if(gattcallbacks.size()!=0) {
                    if(doLog) {Log.i(LOG_ID,"initializeBluetooth gattcallbacks");};
                    for(SuperGattCallback cb: gattcallbacks) {
                        if(cb.mActiveDeviceAddress!=null)  {
                            if (BluetoothAdapter.checkBluetoothAddress(cb.mActiveDeviceAddress)) {
                                Log.i(LOG_ID,"checkBluetoothAddress("+cb.mActiveDeviceAddress+") succeeded") ;
                                cb.mActiveBluetoothDevice = mBluetoothAdapter.getRemoteDevice(cb.mActiveDeviceAddress);
                            } else {
                                Log.i(LOG_ID,"checkBluetoothAddress("+cb.mActiveDeviceAddress+") failed") ;
                                cb.setDeviceAddress(null);
                            }
                            }
                        }
                    addReceivers();
                    return connectToAllActiveDevices(0);
                    }
                else
                    if(doLog) {Log.i(LOG_ID,"initializeBluetooth no gattcallbacks");};
                    }
            }

    return false;
    }
    /*
static boolean hasAidexX() {
   var one=blueone;
   if(one==null)
      return false;
    for(var cb: one.gattcallbacks)  {
        if(cb.sensorgen==0x50)
            return true;
        };
    return false;
    } */
static void afterUnpair(MainActivity act,BooleanConsumer runnable) {
       
       var one=blueone;
       if(one!=null) {
         var gatts=one.gattcallbacks;
         int tot=gatts.size();
        AidexXGattCallback[] ais=new AidexXGattCallback[tot];
         int iter=0;
         for(var cb: gatts)  {
                if(cb.sensorgen==0x50)
                    ais[iter++]=(AidexXGattCallback)cb;
                };
        if(iter>0) {
            Log.i(LOG_ID,"afterUnpair iter="+iter+ " activity="+(act!=null));
            var up=act!=null?new UnpairOverlayHost(act,R.string.releasingsensor):null;
            if(iter==1) {
                ais[0].startUnpair(up,res->{
                    Log.i(LOG_ID,"unpair 1");
                    runnable.accept(res);
                    return true;
                    });
                }
           else {
             AtomicInteger remaining = new AtomicInteger(iter);
             for(int i=0;i<iter;++i) {
                ais[i].startUnpair(up,res ->{
                    Log.i(LOG_ID,"unpair 2");
                  if(remaining.decrementAndGet() == 0) {
                     runnable.accept(res);
                     return true;
                     }
                    return false;
                     });

                }
               }
            return;
            }
        }
     runnable.accept(false);
     }

static void unpairWatch(Context context,String sourceId,MessageSender sender) {
          afterUnpair(MainActivity.thisone,res-> {
                                Log.i(LOG_ID,"unpairWatch "+sourceId);
                                Applic.setbluetooth(context,false);
                                sender.sendunpair(sourceId,res);
                                });
              }
}
