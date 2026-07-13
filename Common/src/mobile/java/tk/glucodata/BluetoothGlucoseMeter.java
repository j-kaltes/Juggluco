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
/*      Fri Sep 19 14:52:28 CEST 2025                                                */
package tk.glucodata;


import static android.bluetooth.BluetoothDevice.ACTION_BOND_STATE_CHANGED;
import static android.bluetooth.BluetoothDevice.EXTRA_BOND_STATE;

import static tk.glucodata.Log.doLog;
import static tk.glucodata.SuperGattCallback.bondString;
import static tk.glucodata.util.sleep;


import android.annotation.SuppressLint;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;

public  class BluetoothGlucoseMeter  {
private static final String LOG_ID="BluetoothGlucoseMeter";
private static BroadcastReceiver bondStateReceiver =null;

private static void addBondStateReceiver() {
    bondStateReceiver=new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            final BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
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
           if(!action.equals(ACTION_BOND_STATE_CHANGED)) {
                Log.e(LOG_ID,"Bond Broadcast: not ACTION_BOND_STATE_CHANGED");
                return;
              }
              final int previousBondState = intent.getIntExtra(BluetoothDevice.EXTRA_PREVIOUS_BOND_STATE,BluetoothDevice.ERROR);
              final int bondState = intent.getIntExtra(EXTRA_BOND_STATE, BluetoothDevice.ERROR);
              if(doLog)
                  Log.i(LOG_ID,"Bond Broadcast "+address+": "+bondString(previousBondState)+" to "+bondString(bondState));
              final var gatts=meterGatts;
              if(gatts!=null) {
                 for(var gatt:gatts) {
                    if(gatt.mActiveBluetoothDevice!=null&&device.equals(gatt.mActiveBluetoothDevice)) {
                        gatt.bonded();
                        }
                    }
                }
             }
        };
    try {
        Applic.app.registerReceiver(bondStateReceiver, new IntentFilter(ACTION_BOND_STATE_CHANGED));
        }
    catch(Throwable th) {
            Log.stack(LOG_ID, "registerReceiver ",th);
            }
    }

private static void removeBondStateReceiver() {
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
private static void addReceivers() {
   addBluetoothStateReceiver();
   addBondStateReceiver() ;
    }
private static void removeReceivers() {
   removeBluetoothStateReceiver();
   removeBondStateReceiver() ;
    }

static  GlucoseMeterGatt[] meterGatts;
private static void getDevices() {
    final int[] devices=Natives.getActiveGlucoseMeters( );
    final int len=devices.length;
    meterGatts=new GlucoseMeterGatt[len];
    if(len>0) {
        int it=0;
        for(var dev:devices) {
            meterGatts[it++]=new GlucoseMeterGatt(dev);
            }
         }
    }
public static void removeDevice(int index) {
   int len;
   if(meterGatts!=null&&(len=meterGatts.length)>0) {
        int newlen=len-1;
        var newgatts= new GlucoseMeterGatt[newlen];
        int i=0;
        for(var gatt:meterGatts) {
            if(gatt.meterIndex==index) {
                gatt.stop=true;
                gatt.view=null;
                if(doLog)
                    Log.i(LOG_ID,"removeDevice remove "+index);
                gatt.disconnect();
                }
             else {
                if(i==newlen) {
                        Log.e(LOG_ID,"removeDevice: ERROR device "+index+" not found");
                        return;
                        }
                newgatts[i++]=gatt;
                }
               }
          meterGatts=newgatts;     
//          Log.i(LOG_ID,"removeDevice removed "+index);
          }
      else {
          if(doLog)
                Log.i(LOG_ID,"removeDevice no devices");
        }
   }
static void zeroViews() {
       if(meterGatts==null)     
            return;
       for(var gatt:meterGatts) {
        gatt.view=null;
        }
       } 
public static GlucoseMeterGatt addDevice(int index,BluetoothDevice device) {
    var     gatt=new GlucoseMeterGatt(index);
    if(device!=null)
        gatt.setDevice(device);
   if(meterGatts!=null&&meterGatts.length>0) {
       {
           var oldgatt = getExistingGatt(index);
           if (oldgatt != null) {
                if(doLog)
                   Log.i(LOG_ID, "addDevice already added " + index);
               if (oldgatt.connected)
                   oldgatt.disconnect();
               if(device!=null)
                oldgatt.setDevice(device);
               return oldgatt;
           }
       }
       if(doLog)
                Log.i(LOG_ID,"addDevice later "+index);
        int oldlen=meterGatts.length;
        int len=oldlen+1;
        var tempGatts=new GlucoseMeterGatt[len];
        System.arraycopy(meterGatts,0,tempGatts,0,oldlen);
        tempGatts[oldlen]=gatt;
        meterGatts=tempGatts;
        gatt.connectOrScan(0);
        }
    else {
       if(doLog)
            Log.i(LOG_ID,"addDevice first "+index);
        meterGatts=new GlucoseMeterGatt[1];
        meterGatts[0]=gatt;
        initBluetooth();
        connectAllDevices(0);
        }
    return gatt;
    }
static GlucoseMeterGatt  getExistingGatt(int index) {
        if(meterGatts==null)
                return null;
        for(var gatt:meterGatts) {
            if(gatt.meterIndex==index) {
                return gatt;
                }
            }
        return null; 
        }
private static boolean devicesStarted=false;
public static void startDevices() {
    if(devicesStarted) {
        if(doLog)
            Log.i(LOG_ID,"startDevices: already started");
        return;
        }
    if(Natives.staticnum()) {
        if(doLog)
            Log.i(LOG_ID,"startDevices staticnum don't start");
        return;
        }
    if(meterGatts==null) {
        getDevices();
        if(meterGatts==null||meterGatts.length==0) {
            if(doLog)
               Log.i(LOG_ID,"startDevices no Devices found");
            return;
            }
        else {
            if(doLog)
               Log.i(LOG_ID,"startDevices found devices");
            }
         }
    else {
        if(doLog)
            Log.i(LOG_ID,"startDevices: meterGatts!=null");
        }
    devicesStarted=true;
    initBluetooth();
    connectAllDevices(0);
    }

private static boolean removeDevices() {
   var tmpgatt=meterGatts;
   meterGatts=null;
   if(tmpgatt!=null) {
        for(var gatt:tmpgatt) {
                gatt.stop=true;
                gatt.view=null;
                gatt.disconnect();
                }
         return true;
        }
   return false;
   }
public static boolean stopDevices() {
    if(doLog)
        Log.i(LOG_ID,"stopDevices");

    removeReceivers();
    stopScanner();
    scanner=null;
    var removed=removeDevices();
    devicesStarted=false;
    return removed;
    }
public static void restartDevices() {
    if(stopDevices())
        sleep(40);
    startDevices();
    };
static MeterScanner scanner=null;
public static void startScanner(long msec) {
    if(scanner==null) {
        scanner=new MeterScanner();
        }
    else {
            scanner.reset();
            }
    scanner.scanStarter(msec);
    }
public static void startAdapterScanner(DeviceList.DeviceListViewAdapter adapt,boolean aidexx ) {
     if(scanner!=null) {
        scanner.stopScan(false);
        }

    initBluetooth();
    scanner=new MeterScanner(adapt);
    scanner.takeAidexX=aidexx;
    scanner.scanStarter(0);
    }

public static void stopScanner() {
     if(scanner!=null) {
        scanner.reset();
        scanner.stopScan(false);
        }
     }
public static void connectAllDevices(long delayMillis) {
    boolean scan=false;
    for(final var gatt:meterGatts) {
        scan=scan||!gatt.connectDevice(delayMillis);
        }
  if(scan) {
      startScanner(delayMillis);
      }
  }
public static void connectActiveDevices(long delayMillis) {
    final var gatts=meterGatts;
    if(gatts!=null) { 
        boolean scan=false;
        for(final var gatt:gatts) {
            scan=scan||!gatt.connectActiveDevice(delayMillis);
            }
      if(scan) {
          startScanner(delayMillis);
          }
     }
  }

private static BroadcastReceiver mBluetoothAdapterReceiver =null; ;
private static void removeBluetoothStateReceiver() {
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
static private void addBluetoothStateReceiver() {
    if(mBluetoothAdapterReceiver==null) {
     mBluetoothAdapterReceiver=new BroadcastReceiver() {
        @SuppressLint("MissingPermission")
        @Override
        public void onReceive(Context context, Intent intent) {
            if ("android.bluetooth.adapter.action.STATE_CHANGED".equals(intent.getAction())) {
            int intExtra = intent.getIntExtra("android.bluetooth.adapter.extra.STATE", -1);
            if(intExtra == BluetoothAdapter.STATE_OFF) {
                if(doLog) {Log.v(LOG_ID,"BLUETOOTH switched OFF");};
                final var scann=scanner;
                if(scann!=null)
                    scann.stopScan(false);
               final var gatts=meterGatts;
               if(gatts!=null) {
                    for(var cb: gatts)  {
                        cb.close();
                        }
                     }
                } 
            else if (intExtra == BluetoothAdapter.STATE_ON) {
                if(doLog) {Log.v(LOG_ID,"BLUETOOTH switched ON");};
                connectActiveDevices(500);
              }
            }
          }
        };
        Applic.app.registerReceiver( mBluetoothAdapterReceiver, new IntentFilter("android.bluetooth.adapter.action.STATE_CHANGED"));
        }
    }
static BluetoothManager mBluetoothManager=null;
 static BluetoothAdapter mBluetoothAdapter;
public static boolean initBluetooth() {
    if(mBluetoothManager==null) {
        mBluetoothManager = (BluetoothManager) Applic.app.getSystemService(Context.BLUETOOTH_SERVICE);
        if(mBluetoothManager  == null) {
            if(doLog) {Log.i(LOG_ID, "getSystemService(\"BLUETOOTH_SERVICE\")==null");};
            return false;
        } else {
            mBluetoothAdapter = mBluetoothManager.getAdapter();
            if(doLog)
                Log.i(LOG_ID,"initBluetooth() success");
            addReceivers();
            return true;
            }
        }
    else{
        if(doLog)
            Log.i(LOG_ID,"initBluetooth() already initialized"); 
        return true;
        }
    }

static boolean bluetoothIsEnabled() {
        if (mBluetoothAdapter!= null) {
            return  mBluetoothAdapter.isEnabled();
        }
        return false;
    }

}
