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


import static android.bluetooth.BluetoothDevice.BOND_BONDED;
import static android.bluetooth.BluetoothDevice.BOND_NONE;
import static android.bluetooth.BluetoothGatt.GATT_INSUFFICIENT_AUTHENTICATION;
import static android.bluetooth.BluetoothGatt.GATT_SUCCESS;
import static android.webkit.ConsoleMessage.MessageLevel.LOG;
import static tk.glucodata.AccuGattCallback.ManufacturerNameCharUUID;
//import static tk.glucodata.BluetoothGlucoseMeter.bondString;
import static tk.glucodata.Applic.app;
import static tk.glucodata.BluetoothGlucoseMeter.mBluetoothAdapter;
import static tk.glucodata.Libre2GattCallback.showCharacter;
import static tk.glucodata.Log.doLog;
import static tk.glucodata.Natives.showbytes;
import static tk.glucodata.SuperGattCallback.bondString;
import static tk.glucodata.SuperGattCallback.enableIndicationnote;
import static tk.glucodata.SuperGattCallback.enableNotificationnote;
import static tk.glucodata.util.sleep;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothProfile;
import android.media.Ringtone;
import android.media.RingtoneManager;
import android.net.Uri;
import android.os.Build;

import androidx.annotation.NonNull;

import java.util.concurrent.TimeUnit;

public  class GlucoseMeterGatt  extends BluetoothGattCallback {
    MeterList.MeterView view=null;
    final static private String LOG_ID="GlucoseMeterGatt";
    static    final boolean autoconnect=true;
    protected BluetoothGatt mBluetoothGatt;
    public BluetoothDevice mActiveBluetoothDevice;
    public final int meterIndex;
    static private final boolean useConnect=(Build.VERSION.SDK_INT >26);
    public GlucoseMeterGatt(int index) {  
       meterIndex=index;

        }
void updateview() {
        if(view!=null) {
            ((MainActivity)view.getContext()).runOnUiThread(()-> view.setdata(view.meterIndex));
                } 
        }
        /*
public GlucoseMeterGatt(String deviceName) {  
   this(Natives.GlucoseMeterGetIndex(deviceName));
    } */
public String getDeviceAddress() {
        return Natives.GlucoseMeterDeviceAddress(meterIndex);
        }
private boolean setDeviceAddress(String address) {
        return Natives.GlucoseMeterSetDeviceAddress(meterIndex,address);
        }
public String getDeviceName() {
        return Natives.GlucoseMeterDeviceName(meterIndex);
        }
    public void setDevice(BluetoothDevice device) {
        mActiveBluetoothDevice = device;
        if(device!=null) {
            String address = device.getAddress();
            setDeviceAddress(address);
            }
        }
long foundtime=0L;
 private static final String TimeCharUUID = "00002a2b-0000-1000-8000-00805f9b34fb";
 private static final String GlucoseCharUUID = "00002a18-0000-1000-8000-00805f9b34fb";
 private static final String ContextCharUUID = "00002a34-0000-1000-8000-00805f9b34fb";
 private static final String RecordsCharUUID = "00002a52-0000-1000-8000-00805f9b34fb";
 private static final String DateTimeCharUUID = "00002a08-0000-1000-8000-00805f9b34fb";
 private static final String SerialNumberCharUUID = "00002a25-0000-1000-8000-00805f9b34fb";
 private static final String IsensTimeCharUUID ="0000fff1-0000-1000-8000-00805f9b34fb";


/*

Accu-Chek Mobile
service: 00001800-0000-1000-8000-00805f9b34fb
Characteristic: 00002a00-0000-1000-8000-00805f9b34fb
Characteristic: 00002a01-0000-1000-8000-00805f9b34fb
Characteristic: 00002a04-0000-1000-8000-00805f9b34fb
Characteristic: 00002aa6-0000-1000-8000-00805f9b34fb
service: 00001801-0000-1000-8000-00805f9b34fb
Characteristic: 00002a05-0000-1000-8000-00805f9b34fb
service: 0000180a-0000-1000-8000-00805f9b34fb
Characteristic: 00002a29-0000-1000-8000-00805f9b34fb
Characteristic: 00002a24-0000-1000-8000-00805f9b34fb
Characteristic: 00002a25-0000-1000-8000-00805f9b34fb
Characteristic: 00002a27-0000-1000-8000-00805f9b34fb
Characteristic: 00002a26-0000-1000-8000-00805f9b34fb
Characteristic: 00002a28-0000-1000-8000-00805f9b34fb
service: 00001808-0000-1000-8000-00805f9b34fb
Characteristic: 00002a18-0000-1000-8000-00805f9b34fb
Characteristic: 00002a34-0000-1000-8000-00805f9b34fb
Characteristic: 00002a51-0000-1000-8000-00805f9b34fb
Characteristic: 00002a52-0000-1000-8000-00805f9b34fb


*/

private  BluetoothGattCharacteristic TimeChar;
private  BluetoothGattCharacteristic DateTimeChar;
private  BluetoothGattCharacteristic IsensTimeChar;
private  BluetoothGattCharacteristic GlucoseChar;
private  BluetoothGattCharacteristic ContextChar;
private  BluetoothGattCharacteristic RecordsChar;
private  BluetoothGattCharacteristic ManufacturerNameChar;
private  BluetoothGattCharacteristic SerialNumberChar;



private boolean discovered=false;
private boolean accuChekMobile=false;
private int recordsWriteAuthenticationRetries=0;
private boolean discover(BluetoothGatt bluetoothGatt) {
    var services=bluetoothGatt.getServices();
    boolean success=false;
    accuChekMobile=false;
    recordsWriteAuthenticationRetries=0;
    TimeChar=null;
    DateTimeChar=null;
    IsensTimeChar=null;
    GlucoseChar=null;
    ContextChar=null;
    RecordsChar=null;
    ManufacturerNameChar=null;
    SerialNumberChar=null;
    for(var ser:services) {
        if(AidexXGattCallback.ScanServiceUUID.equals(ser.getUuid())) {
             Log.i(LOG_ID,"Aidex X sensor, remove");
                var devname= mActiveBluetoothDevice.getName();
                if(devname!=null) {
                    Natives.GlucoseMeterRemove(devname,null);
                    }
               BluetoothGlucoseMeter.restartDevices();
               return false;
               }
        if(doLog) Log.i(LOG_ID,"service: "+ser.getUuid().toString());
        var chars=ser.getCharacteristics();
        for(var s:chars) {
            var uuid=s.getUuid().toString();
            if(doLog) Log.i(LOG_ID,"Characteristic: "+uuid);
            switch(uuid) {
                case ManufacturerNameCharUUID: ManufacturerNameChar=s;break;
                case TimeCharUUID: TimeChar=s;break;
                case DateTimeCharUUID: DateTimeChar=s;break;
                case IsensTimeCharUUID: IsensTimeChar=s;break;
                case GlucoseCharUUID: GlucoseChar=s;success=true;break;
                case ContextCharUUID: ContextChar=s;break;
                case RecordsCharUUID: RecordsChar=s;break;
                case SerialNumberCharUUID: SerialNumberChar=s;break;
                }
            }
        }
    if(success)  {
        if(doLog) Log.i(LOG_ID,"discover successfull");
        if(ManufacturerNameChar!=null) {
            tryer( ()->
                {
                return bluetoothGatt.readCharacteristic(ManufacturerNameChar);
                });
            }
        else if(TimeChar!=null)
            tryer(()->bluetoothGatt.readCharacteristic(TimeChar));
        else
            startGlucoseService(bluetoothGatt);
        }
    else {
        if(doLog)
            Log.i(LOG_ID,"discover failed");
        }
    discovered=success;
    return success;
    }

long receivedTime=0L;
boolean newvalues=false;
    @Override
    public void onCharacteristicChanged(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic) {
        final var value=characteristic.getValue();
        final var uuid=characteristic.getUuid().toString();
        if(doLog)
            Log.showbytes("Meter: onCharacteristicChanged "+uuid,value);
        switch(uuid) {
            case GlucoseCharUUID:
                if(Natives.GlucoseMeterSave(meterIndex,value)) {
                    newvalues=true;
                    receivedTime=System.currentTimeMillis();
                    updateview();
                    Applic.app.redraw();
                    }
                break;
            case RecordsCharUUID:
                if(Natives.recordCharacteristicChanged(value)) {
                    newvalues=true;
                    Applic.app.redraw();
                    }
                else
                    newvalues=false;
                receivedTime=System.currentTimeMillis();
               updateview();
                break;
            }

    }


private boolean firstRecordonly=false;
static private final boolean newerRecords=true;

protected  final boolean enableNotification(BluetoothGatt bluetoothGatt1, BluetoothGattCharacteristic bluetoothGattCharacteristic) {
        return  enableNotificationnote(""+meterIndex, bluetoothGatt1,  bluetoothGattCharacteristic);
        }
protected  final boolean enableIndication(BluetoothGatt bluetoothGatt1, BluetoothGattCharacteristic bluetoothGattCharacteristic) {
        return  enableIndicationnote(""+meterIndex, bluetoothGatt1, bluetoothGattCharacteristic) ;
        }
private void tryer(Supplier<Boolean> worked) {
        if(worked.get())
            return;
        Applic.scheduler.schedule(() -> { 
             for(int i=0;i<16;i++) {
                  if(!connected) {
                       {if(doLog) {Log.i(LOG_ID,"tryer stops not connected");};};
                      return;
                      }
                  if(worked.get()) return; 
                  sleep(20) ;
                 } }, 20, TimeUnit.MILLISECONDS);
       } 
private void handleManufactory(BluetoothGatt gatt,String manufacturer) {
        if(doLog)
            Log.i(LOG_ID,"handleManufactory "+ manufacturer);
        if(manufacturer.startsWith("Roche")) {
                if(DateTimeChar==null)  {
                    accuChekMobile=true;
                    startGlucoseService(gatt);
                    }
                else {
                    if(doLog)
                            Log.i(LOG_ID,"read DateTime");
                    tryer(()->gatt.readCharacteristic(DateTimeChar));
                    }
                return;
                }
        if(manufacturer.startsWith("TaiDoc")) {
                firstRecordonly=true;
                tryer(()->enableNotification(gatt, GlucoseChar));
                return;
                }
        if(manufacturer.startsWith("i-SENS")) {
               // newerRecords=true;
                tryer(()->enableNotification(gatt, IsensTimeChar));
                return;
                }
        tryer(()->gatt.readCharacteristic(TimeChar));
       }
private void startGlucoseService(BluetoothGatt gatt) {
        if(ContextChar!=null) {
            tryer(()->enableNotification(gatt, ContextChar));
            }
        else {
            tryer(()->enableNotification(gatt, GlucoseChar));
            }
        }
private boolean writeRecordsRequest(BluetoothGatt gatt) {
        if(firstRecordonly) {
              byte[] cmd={1,(byte)0x5};
              return writer(gatt, RecordsChar,cmd);
              }
              /*
        if(accuChekMobile) {
              byte[] cmd={1,(byte)0x1};
              return writer(gatt, RecordsChar,cmd);
              }
              */
        if(newerRecords) {
            byte[] cmd=Natives.getGlucoseMeterNewCMD(meterIndex);
            if(cmd!=null) {
                return writer(gatt, RecordsChar,cmd);
                }
            disconnect();
            return true;
            }
        byte[] cmd={1,(byte)0x1};
        return writer(gatt, RecordsChar,cmd);
        }


private void requestRecords(BluetoothGatt gatt) {
        recordsWriteAuthenticationRetries=0;
        tryer(()->writeRecordsRequest(gatt));
        }
    @Override
    public void onCharacteristicRead(@NonNull BluetoothGatt bluetoothGatt,@NonNull  BluetoothGattCharacteristic bluetoothGattCharacteristic, int status) {
        final var value=bluetoothGattCharacteristic.getValue();
        final var uuid=bluetoothGattCharacteristic.getUuid().toString();
        if(doLog) {Log.d(LOG_ID, bluetoothGatt.getDevice().getAddress() + " onCharacteristicRead, status:" + status + " UUID:" + uuid);};;

      if(status!=GATT_SUCCESS) {
            if(status == GATT_INSUFFICIENT_AUTHENTICATION) { 
                if(doLog)
                    Log.i(LOG_ID,"onCharacteristicRead GATT_INSUFFICIENT_AUTHENTICATION");
                return;
                }
            if(doLog) {
               String message = "CharacteristicRead "+bluetoothGattCharacteristic.getUuid().toString();
               Log.i(LOG_ID,message);
               }
            disconnect();
            return;
            }
        switch(uuid) {
            case ManufacturerNameCharUUID: handleManufactory(bluetoothGatt,new String(value));break;
            case SerialNumberCharUUID:
                if(doLog)
                    Log.i(LOG_ID,"Serial number "+new String(value));
                requestRecords(bluetoothGatt);
                break;
            case DateTimeCharUUID: 
            case TimeCharUUID: 
                Natives.GlucoseMeterSaveTime(meterIndex,value);
                startGlucoseService(bluetoothGatt);
                break;
                 
            default: Log.e(LOG_ID,"onCharacteristicRead: Unknown UUID "+uuid);break;
            };
    }

    @Override
    public void onCharacteristicWrite(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status) {
        final var uuid=characteristic.getUuid().toString();
 //       final var value=characteristic.getValue();
        if(doLog) {
            showCharacter("onCharacteristicWrite",characteristic);
            }
        if(status == GATT_INSUFFICIENT_AUTHENTICATION && RecordsCharUUID.equals(uuid) && recordsWriteAuthenticationRetries++ < 8) {
            if(doLog)
                Log.i(LOG_ID,"retry Record Access Control Point after authentication");
            Applic.scheduler.schedule(() -> {
                if(connected && mBluetoothGatt==gatt)
                    writeRecordsRequest(gatt);
                }, 2, TimeUnit.SECONDS);
            return;
            }
        if(status!=GATT_SUCCESS) {
            if(doLog)
                Log.i(LOG_ID,"onCharacteristicWrite status="+status+" UUID:"+uuid);
            return;
            }
        switch(uuid) {
            case IsensTimeCharUUID: tryer(()->enableNotification(gatt, GlucoseChar));break;
            case RecordsCharUUID: recordsWriteAuthenticationRetries=0;break;

            default:
            }
    }
boolean connected=false;
 boolean stop=false;
 long connectedTime=0L;
 long disconnectedTime=0L;
 boolean isBonded=false;
    @Override
    public void onConnectionStateChange(BluetoothGatt bluetoothGatt, int status, int newState) {
        long tim = System.currentTimeMillis();
        final var bondstate = bluetoothGatt.getDevice().getBondState();
        if (doLog) {
                final String[] state = {"DISCONNECTED", "CONNECTING", "CONNECTED", "DISCONNECTING"};
                Log.i(LOG_ID, meterIndex + " onConnectionStateChange, status:" + status + ", state: " + (newState < state.length ? state[newState] : newState) + " bondstate= "+bondString(bondstate) +" "+ bondstate);
             }
       if(newState == BluetoothProfile.STATE_CONNECTED) {
         isBonded=bondstate==BOND_BONDED;
         connectedTime=tim;
         updateview();
        if(stop) {
            close();
            return;
            }
          discovered=false;
          connected=true;
          if(bondstate == BluetoothDevice.BOND_BONDING) {
              {if(doLog) {Log.i(LOG_ID, "wait BOND_BONDING");};};
              }
          else {
            if(bondstate == BOND_NONE || bondstate == BOND_BONDED) {
                  if(!bluetoothGatt.discoverServices()) {
                         final String mess="bluetoothGatt.discoverServices()  failed";
                         Log.e(LOG_ID, mess);
                         disconnect();
                        }    
                   else
                     Log.i(LOG_ID,"discoverServices() called");

            } else {
                Log.e(LOG_ID, "getBondState() returns unknown state " + bondstate);
                disconnect();
                }
            }
        } else {
           disconnectedTime=tim;
           updateview();
           connected=false;
            if(bondstate == BluetoothDevice.BOND_BONDING) {
                   {if(doLog) {Log.i(LOG_ID, "BOND_BONDING");};};
                   }
            if(newState == BluetoothProfile.STATE_DISCONNECTED) {
                if(stop) {
                        if(doLog)
                           Log.i(LOG_ID,"stopping");
                      close();
                        }
                  else {
                     if(autoconnect)  {
                        if(useConnect)
                            bluetoothGatt.connect();
                        }
                    else
                        connectActiveOrScan(0);
                      }
                 }
         }
    }

    @Override
    public void onDescriptorRead(BluetoothGatt gatt, BluetoothGattDescriptor descriptor, int status) {
        if(doLog)
            Log.i(LOG_ID,"onDescriptorRead/3 "+status);
    }

    @Override
    public void onDescriptorRead(@NonNull BluetoothGatt gatt, @NonNull BluetoothGattDescriptor descriptor, int status, @NonNull byte[] value) {
        if(doLog)
                Log.i(LOG_ID,"onDescriptorRead/4 "+status);
    }

static private          byte[] VerioGetTimeCMD={0x20, 0x02};
static private          byte[] VerioGetTcounterCMD={0x20, 0x02};

  private boolean writer(BluetoothGatt mBluetoothGatt,BluetoothGattCharacteristic cha, byte[] data) {
        if (!cha.setValue(data)) {
            {if(doLog){Log.showbytes(LOG_ID + ": " +cha.getUuid().toString() + " cha.setValue failed", data);};}
            return false;
        }
        if (!mBluetoothGatt.writeCharacteristic(cha)) {
            {if(doLog){Log.showbytes(LOG_ID + ": " +cha.getUuid().toString() + " writeCharacteristic failed", data);};}
            return false;
        }
        {if(doLog){Log.showbytes(LOG_ID + " writeCharacteristic: "+cha.getUuid().toString(), data);};}
        return true;
    }
//s/\<\([a-zA-Z0-9]*\).writeCharacteristic(\([^,]*\),\([^)]*\))/writer(\1,\2,\3)

private void setCareSenseTime(BluetoothGatt bluetoothGatt) {
    byte[] cmd=Natives.careSenseTimeCMD();
    tryer(()-> writer(bluetoothGatt,IsensTimeChar,cmd));
    }
    @Override
    public void onDescriptorWrite(BluetoothGatt bluetoothGatt, BluetoothGattDescriptor bluetoothGattDescriptor, int status) {
        BluetoothGattCharacteristic characteristic = bluetoothGattDescriptor.getCharacteristic();
        var uuid=characteristic.getUuid().toString();
        if (doLog) {
            byte[] value = bluetoothGattDescriptor.getValue();
            {if(doLog){showbytes("GlucoseMeter: onDescriptorWrite char: " + uuid + " desc: " + bluetoothGattDescriptor.getUuid().toString() + " status=" + status, value);};}
            }
        switch(uuid) {
           case GlucoseCharUUID:
                tryer(()->enableIndication(bluetoothGatt, RecordsChar));
                break;
           case ContextCharUUID:
                    tryer(()->enableNotification(bluetoothGatt, GlucoseChar));
                    break;
           case RecordsCharUUID:
                if(accuChekMobile && SerialNumberChar!=null) {
                    tryer(()->bluetoothGatt.readCharacteristic(SerialNumberChar));
                    }
                else
                    requestRecords(bluetoothGatt);
                    break;
          case IsensTimeCharUUID:
                    setCareSenseTime(bluetoothGatt);
                    break;
            }
    }

    @Override
    public void onMtuChanged(BluetoothGatt gatt, int mtu, int status) {
        if(doLog) {
            Log.i(LOG_ID,"onMtuChanged mtu="+mtu+" status="+ status);
            }
    }

    @Override
    public void onPhyRead(BluetoothGatt gatt, int txPhy, int rxPhy, int status) {
        if(doLog) {
            Log.i(LOG_ID,"onPhyRead txPhy="+ txPhy+"  rxPhy="+rxPhy+" status="+ status);
            }
    }

    @Override
    public void onPhyUpdate(BluetoothGatt gatt, int txPhy, int rxPhy, int status) {
        if(doLog) {
            Log.i(LOG_ID,"onPhyUpdate txPhy="+ txPhy+"  rxPhy="+rxPhy+" status="+ status);
            }
       }

    @Override
    public void onReadRemoteRssi(BluetoothGatt gatt, int rssi, int status) {
        if(doLog) {
            Log.i(LOG_ID,"onReadRemoteRssi rssi"+ rssi+" status"+status);
            }
    }

    @Override
    public void onReliableWriteCompleted(BluetoothGatt gatt, int status) {
        if(doLog) {
            Log.i(LOG_ID,"onReliableWriteCompleted "+status);
            }
    }

    @Override
    public void onServiceChanged(@NonNull BluetoothGatt gatt) {
       if(doLog)
        Log.i(LOG_ID,"onServiceChanged ");
    }

    @Override
    public void onServicesDiscovered(BluetoothGatt gatt, int status) {
        if(doLog)
            Log.i(LOG_ID,"onServicesDiscovered "+status);
        discover(gatt);
    }


    private Runnable getConnectDevice() {
        disconnect();
        return () -> {
             close();
            if(doLog) {Log.i(LOG_ID,"getConnectDevice Runnable "+ meterIndex);};
            if(!BluetoothGlucoseMeter.bluetoothIsEnabled()) {
                Log.e(LOG_ID, "!bluetoothIsEnabled() ");
                return ;
                }
            var device= mActiveBluetoothDevice;
            if(device==null) {
                Log.e(LOG_ID, meterIndex +" "+"device==null");
                return;
                }
        
            if (mBluetoothGatt != null) {
                if(doLog) {Log.d(LOG_ID, meterIndex + " mBluetoothGatt!=null");};
                return;
                }
            if(doLog) {
                var devname=device.getName();
                if(devname!=null) {
                        if(doLog)
                            Log.d(LOG_ID, meterIndex + " Try connection to " + device.getAddress()+ " "+devname);;
                        }
                }
            try {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                    mBluetoothGatt = device.connectGatt(app, autoconnect, this, BluetoothDevice.TRANSPORT_LE);
                } else {
                    mBluetoothGatt = device.connectGatt(app, autoconnect, this);
                    }

            if(doLog) {Log.i(LOG_ID,meterIndex+" after connectGatt ="+mBluetoothGatt);};
            } catch (SecurityException se) {
                var mess = se.getMessage();
                mess = mess == null ? "" : mess;
                String uit = ((Build.VERSION.SDK_INT > 30) ? Applic.getContext().getString(R.string.turn_on_nearby_devices_permission)  : mess) ;
                Applic.Toaster(uit);

                Log.stack(LOG_ID, meterIndex +" "+ "connectGatt", se);
            } catch (Throwable e) {
                Log.stack(LOG_ID, meterIndex +" "+ "connectGatt", e);

                }
        };
    }

    public void close() {
       if(doLog) {Log.i(LOG_ID,"close "+meterIndex);};
        var tmpgatt=mBluetoothGatt ;
        if (tmpgatt != null) {
            try {
                tmpgatt.disconnect();
                tmpgatt.close();
            } catch (Throwable se) {
                var mess = se.getMessage();
                mess = mess == null ? "" : mess;
                String uit = ((Build.VERSION.SDK_INT > 30) ? Applic.getContext().getString(R.string.turn_on_nearby_devices_permission)  : mess) ;
                Applic.Toaster(uit);
                Log.stack(LOG_ID, meterIndex +" "+ "BluetoothGatt.close()", se);
            }
        finally {    
            mBluetoothGatt = null;
            }
        }
    else {
        {if(doLog) {Log.i(LOG_ID,"close mBluetoothGatt==null");};};
        }

    }

public void disconnect() {
    final var thegatt= mBluetoothGatt;
    if(thegatt!=null) {
        {if(doLog) {Log.i(LOG_ID,"Disconnect");};};
        thegatt.disconnect();
        }
     else  {
        {if(doLog) {Log.i(LOG_ID,"Disconnect mBluetoothGatt==null");};};
      }
    }
 public boolean connectActiveDevice(long delayMillis) {
    if(doLog) {Log.i(LOG_ID,"connectDevice("+delayMillis+") "+ meterIndex);};
    Runnable connect=getConnectDevice();
    if(connect==null) 
        return false;
    Applic.scheduler.schedule(connect, delayMillis, TimeUnit.MILLISECONDS);
    return true;
    }
 public void connectActiveOrScan(long delayMillis) {
         if(!connectActiveDevice(delayMillis)) {
               BluetoothGlucoseMeter.startScanner(delayMillis);
               }
        }
 public void connectOrScan(long delayMillis) {
         if(!connectDevice(delayMillis)) {
               BluetoothGlucoseMeter.startScanner(delayMillis);
               }
        }
 public boolean connectDevice(long delayMillis) {
        disconnect();
        if(askDevice()) {
                return connectActiveDevice(delayMillis);
                }
        return false;
        }

boolean askDevice() {
    String mActiveDeviceAddress=getDeviceAddress();
    if(mActiveDeviceAddress != null) {
        if(BluetoothAdapter.checkBluetoothAddress(mActiveDeviceAddress)) {
            if(doLog) {Log.i(LOG_ID,"getDevice "+ meterIndex+" checkBluetoothAddress(" +mActiveDeviceAddress +") succeeded");};
            mActiveBluetoothDevice = mBluetoothAdapter.getRemoteDevice(mActiveDeviceAddress);
            return true;
            } 
        if(doLog) {Log.i(LOG_ID, "getDevice "+ meterIndex+" checkBluetoothAddress(" +mActiveDeviceAddress +") failed");};
        setDeviceAddress(null);
        return false;
        }
    if(doLog) {Log.i(LOG_ID,"getDevice no address "+meterIndex);};
    return false;
    }

    public void bonded() {
        final var bondstate = mActiveBluetoothDevice.getBondState();
        switch(bondstate) {
            case BluetoothDevice.BOND_BONDING: {
                if(doLog) {Log.i(LOG_ID,"bonding");};
                    try {
                        var uristr = "android.resource://" + app.getPackageName() + "/" + R.raw.bonded;
                        Uri uri = Uri.parse(uristr);
                        Ringtone ring = RingtoneManager.getRingtone(app, uri);
                        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
                            ring.setLooping(false);
                        }
                        ring.play();

                    } catch (Throwable th) {
                            Log.stack(LOG_ID, "bonded sound", th);
                   }
                };break;
            case  BOND_BONDED:  {
                isBonded=true;
                updateview();
                if(!discovered) {
                  if(mBluetoothGatt==null) {
                     if(doLog)
                          Log.i(LOG_ID,"bonded: mBluetoothGatt");
                      disconnect();
                      return;
                      }
                  if(mBluetoothGatt.discoverServices()) {
                        if(doLog)
                             Log.i(LOG_ID,"bonded: bluetoothGatt.discoverServices()  called");
                        }
                  else {
                         final String mess="bonded: bluetoothGatt.discoverServices()  failed";
                         Log.e(LOG_ID, mess);
                         disconnect();
                        }    
                    } 
                };break;
        }
    }

}
