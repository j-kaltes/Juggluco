/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2, Libre 3, Dexcom G7/ONE+ and           */
/*      Sibionics GS1Sb sensors.                                                     */
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
/*      Fri Sep 12 18:00:55 CEST 2025                                                */


package tk.glucodata;

import static android.app.PendingIntent.getBroadcast;
import static android.bluetooth.BluetoothDevice.BOND_BONDED;
import static android.bluetooth.BluetoothDevice.BOND_BONDING;
import static android.bluetooth.BluetoothDevice.BOND_NONE;
import static android.bluetooth.BluetoothDevice.TRANSPORT_LE;
import static android.bluetooth.BluetoothGatt.GATT_INSUFFICIENT_AUTHENTICATION;
import static android.bluetooth.BluetoothGatt.GATT_SUCCESS;
import static android.bluetooth.BluetoothGattCharacteristic.PERMISSION_WRITE_ENCRYPTED;
import static android.bluetooth.BluetoothGattCharacteristic.PROPERTY_WRITE;
import static android.content.Context.ALARM_SERVICE;
import static android.content.Context.POWER_SERVICE;
import static java.util.Objects.nonNull;
import static tk.glucodata.Applic.RunOnUiThread;
import static tk.glucodata.Applic.app;
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.Log.doLog;
import static tk.glucodata.Log.showbytes;
import static tk.glucodata.Libre2GattCallback.showCharacter;
import static tk.glucodata.Natives.dexKnownSensor;
import static tk.glucodata.Natives.getalarmclock;

import android.annotation.SuppressLint;
import android.app.AlarmManager;
import android.app.PendingIntent;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothProfile;
import     android.bluetooth.BluetoothDevice;
import android.content.Context;
import android.content.Intent;
import android.media.Ringtone;
import android.media.RingtoneManager;
import android.net.Uri;
import android.os.Build;
import android.os.PowerManager;


import androidx.annotation.NonNull;

import java.lang.reflect.Method;
import java.util.Arrays;
import java.util.UUID;
import java.util.concurrent.TimeUnit;

import static tk.glucodata.util.sleep;

public class AccuGattCallback extends SuperGattCallback {
static private final String LOG_ID="AccuGattCallback";
//See: https://gist.github.com/sam016/4abe921b5a9ee27f67b3686910293026
//https://blog.naver.com/leevisual/222198512058
//protected static final UUID mCharacteristicConfigDescriptor = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb");
/*
private final UUID GenericAttributeServiceUUID= UUID.fromString("00001801-0000-1000-8000-00805f9b34fb");

private final UUID GenericAccessServiceUUID=UUID.fromString("00001800-0000-1000-8000-00805f9b34fb");

*/
    static private final String DeviceNameCharUUID ="00002a00-0000-1000-8000-00805f9b34fb";
    static private final String AppearanceCharUUID="00002a01-0000-1000-8000-00805f9b34fb";


//public static final String SIG_SERVICE_DEVICE_INFO ="0000180a-0000-1000-8000-00805f9b34fb"; 
    static  final String ManufacturerNameCharUUID ="00002a29-0000-1000-8000-00805f9b34fb";
    static private final String modelnumberCharUUID="00002a24-0000-1000-8000-00805f9b34fb";
    static private final String SerialNumberCharUUID ="00002a25-0000-1000-8000-00805f9b34fb";
    static private final String HardwareRevisionStringCharUUID="00002a27-0000-1000-8000-00805f9b34fb";
    static private final String FirmwareRevisionStringCharUUID="00002a26-0000-1000-8000-00805f9b34fb";
    static private final String SystemIDCharUUID="00002a23-0000-1000-8000-00805f9b34fb";

    static private final String CGMMeasurementCharUUID="00002aa7-0000-1000-8000-00805f9b34fb";
    static private final String CGMFeatureCharUUID="00002aa8-0000-1000-8000-00805f9b34fb";
    static private final String CGMStatusCharUUID="00002aa9-0000-1000-8000-00805f9b34fb";
    static private final String CGMSessionStartTimeCharUUID="00002aaa-0000-1000-8000-00805f9b34fb";
    static private final String CGMSessionRunTimeCharUUID="00002aab-0000-1000-8000-00805f9b34fb";
   static private final String RecordAccessControlPointCharUUID="00002a52-0000-1000-8000-00805f9b34fb";
    static private final String CGMcontrolCharUUID="00002aac-0000-1000-8000-00805f9b34fb";

private  BluetoothGattCharacteristic DeviceNameChar;
private  BluetoothGattCharacteristic AppearanceChar;


private  BluetoothGattCharacteristic ManufacturerNameChar;
private  BluetoothGattCharacteristic modelnumberChar;
private  BluetoothGattCharacteristic SerialNumberChar;
private  BluetoothGattCharacteristic HardwareRevisionStringChar;
private  BluetoothGattCharacteristic FirmwareRevisionStringChar;
private  BluetoothGattCharacteristic SystemIDChar;

private  BluetoothGattCharacteristic CGMMeasurementChar;
private  BluetoothGattCharacteristic CGMFeatureChar;
private  BluetoothGattCharacteristic CGMStatusChar;
private  BluetoothGattCharacteristic CGMSessionStartTimeChar;
private  BluetoothGattCharacteristic CGMSessionRunTimeChar;
private  BluetoothGattCharacteristic RecordAccessControlPointChar;
private  BluetoothGattCharacteristic CGMcontrolChar;

private boolean discover(BluetoothGatt bluetoothGatt) {
    var services=bluetoothGatt.getServices();
    boolean success=false;
    for(var ser:services) {
        if(doLog)
            Log.i(LOG_ID,"service: "+ser.getUuid().toString());
        var chars=ser.getCharacteristics();
        for(var s:chars) {
            var uuid=s.getUuid().toString();
            if(doLog)
                Log.i(LOG_ID,"Characteristic: "+uuid);
            switch(uuid) {
                case DeviceNameCharUUID: DeviceNameChar=s;break;
                case AppearanceCharUUID: AppearanceChar=s;break;
                case ManufacturerNameCharUUID: ManufacturerNameChar=s;break;
                case modelnumberCharUUID: modelnumberChar=s;break;
                case SerialNumberCharUUID: SerialNumberChar=s;break;
                case HardwareRevisionStringCharUUID: HardwareRevisionStringChar=s;break;
                case FirmwareRevisionStringCharUUID: FirmwareRevisionStringChar=s;break;
                case SystemIDCharUUID: SystemIDChar=s;break;
                case CGMMeasurementCharUUID: CGMMeasurementChar=s;break;
                case CGMFeatureCharUUID: CGMFeatureChar=s;break;
                case CGMStatusCharUUID: CGMStatusChar=s;break;
                case CGMSessionStartTimeCharUUID: CGMSessionStartTimeChar=s;break;
                case CGMSessionRunTimeCharUUID: CGMSessionRunTimeChar=s;break;
                case RecordAccessControlPointCharUUID: RecordAccessControlPointChar=s;break;
                case CGMcontrolCharUUID: CGMcontrolChar=s;success=true;break;
                }
            }
        }
    if(success)  {
        if(doLog)
           Log.i(LOG_ID,"discover successfull");
        if(isBonded) {
            tryer(()->enableNotification(bluetoothGatt, CGMMeasurementChar));
            }
        else {
            tryer( ()->
                {
                var props=CGMStatusChar.getProperties();
                if(doLog)
                        Log.i(LOG_ID,"CGMStatusChar.getProperties()="+props+(BluetoothGattCharacteristic.PERMISSION_READ==(props&BluetoothGattCharacteristic.PERMISSION_READ)?" PERMISSION_READ":""));
                return bluetoothGatt.readCharacteristic(CGMStatusChar);
                });
           }

        }
    else {
        if(doLog)
            Log.i(LOG_ID,"discover failed");
        }
    discovered=success;
    return success;
    }

    public AccuGattCallback(String SerialNumber, long dataptr) {
        super(SerialNumber, dataptr, 0x20);
        if(doLog) {Log.d(LOG_ID, SerialNumber + " AccuGattCallback(..)");};;
        showtime=6*60*1000L;
    }
private void askValues(BluetoothGatt mBluetoothGatt) {
    byte[] cmd=Natives.accuAskValues(dataptr);
    if(cmd!=null) {
        tryer(()->writer(mBluetoothGatt,RecordAccessControlPointChar,cmd));
        }
    else {
        disconnect();
        }
    }
static private boolean writer(BluetoothGatt mBluetoothGatt, BluetoothGattCharacteristic cha, byte[] data) {
        if (!cha.setValue(data)) {
            {if(doLog){showbytes(LOG_ID + ": "+ cha.getUuid().toString() + " cha.setValue failed", data);};}
            return false;
        }
        if(!mBluetoothGatt.writeCharacteristic(cha)) {
            {if(doLog){showbytes(LOG_ID + ": " + cha.getUuid().toString()  + " writeCharacteristic failed", data);};}
            return false;
        }
        {if(doLog){showbytes(LOG_ID + " writeCharacteristic: " + cha.getUuid().toString(), data);};}
        return true;
    }
    @SuppressLint("MissingPermission")
    @Override // android.bluetooth.BluetoothGattCallback
    public void onDescriptorWrite(BluetoothGatt bluetoothGatt, BluetoothGattDescriptor bluetoothGattDescriptor, int status) {
        super.onDescriptorWrite(bluetoothGatt, bluetoothGattDescriptor, status);
        BluetoothGattCharacteristic characteristic = bluetoothGattDescriptor.getCharacteristic();
        var uuid=characteristic.getUuid().toString();
        if (doLog) {
            byte[] value = bluetoothGattDescriptor.getValue();
            {if(doLog){showbytes("Accu: onDescriptorWrite char: " + uuid + " desc: " + bluetoothGattDescriptor.getUuid().toString() + " status=" + status, value);};}
        }
     switch(uuid) {
        case CGMMeasurementCharUUID: 
//        PROPERTY_NOTIFY
                tryer(()->enableIndication(bluetoothGatt, RecordAccessControlPointChar));
                break;
        case RecordAccessControlPointCharUUID:
                tryer(()->enableIndication(bluetoothGatt, CGMcontrolChar));
                break;

        case CGMcontrolCharUUID:
            if(isBonded) {
                bluetoothGatt.readCharacteristic(CGMStatusChar);
                }
             else {
                final var props=CGMcontrolChar.getProperties();
                if((props&PERMISSION_WRITE_ENCRYPTED)!=0) {
                    if(doLog)
                        Log.i(LOG_ID,"CGMcontrolChar PERMISSION_WRITE_ENCRYPTED");
                    }
                if((props&PROPERTY_WRITE)!=0) {
                    if(doLog)
                        Log.i(LOG_ID,"CGMcontrolChar PROPERTY_WRITE");
                    }
                 byte[] data={(byte)0x02,(byte)0x95, (byte)0x2C};
                 tryer(()->writer(bluetoothGatt,CGMcontrolChar,data));
                 }
             break;
             }
    }




private boolean connected=false;

  private boolean isBonded=false;

@SuppressLint("MissingPermission")
@Override
public void onConnectionStateChange(BluetoothGatt bluetoothGatt, int status, int newState) {
    long tim = System.currentTimeMillis();
    if(stop) {
        constatchange[1] = tim; //Needed? ever displayed?
        constatstatusstr="Stopped"; //"
        connected=false;
        if(doLog) {Log.i(LOG_ID, "onConnectionStateChange stop==true");}
        return;
       }
    final var bondstate = bluetoothGatt.getDevice().getBondState();
    if (doLog) {
         final String[] state = {"DISCONNECTED", "CONNECTING", "CONNECTED", "DISCONNECTING"};
        if(doLog) {Log.i(LOG_ID, SerialNumber + " onConnectionStateChange, status:" + status + ", state: " + (newState < state.length ? state[newState] : newState) + " "+ bondString(bondstate) + "("+ bondstate +")");};

        }
    if(newState == BluetoothProfile.STATE_CONNECTED) {
         constatchange[0] = tim;
        isBonded=bondstate==BOND_BONDED;
         phase=isBonded?2:0;
         connected=true;
          if(bondstate == BluetoothDevice.BOND_BONDING) {
              if(doLog) {Log.i(LOG_ID, "wait BOND_BONDING");};

              }
         else {
             if(!discover(bluetoothGatt)) {
                    tryer(()->bluetoothGatt.discoverServices());
                    }
              }

         }
     else {
        setConStatus(status);
         constatchange[1] = tim;
          connected=false;
        if(newState == BluetoothProfile.STATE_DISCONNECTED) {
		   if(!autoconnect) {
			   bluetoothGatt.close();
			   mBluetoothGatt = null;
			   if(!stop) {
				   var sensorbluetooth=SensorBluetooth.blueone;
				   if(sensorbluetooth!=null)
					   sensorbluetooth.connectToActiveDevice(this, 0);
				   }
			   }
		   else {
			   if(!stop) {
				   bluetoothGatt.connect();
				   }
			   else {
				   bluetoothGatt.close();
				   mBluetoothGatt = null;
				   }
			   }
          }
        }
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
private boolean discovered=false;

    @Override // android.bluetooth.BluetoothGattCallback
    public void onServicesDiscovered(BluetoothGatt bluetoothGatt, int status) {
        {if(doLog) {Log.i(LOG_ID, "BLE onServicesDiscovered invoked, status: " + status);};};
        if (status == GATT_SUCCESS) {
            if(!discovered) {
                if(!discover(bluetoothGatt)) 
                      disconnect();
                return;
                }
        }

        disconnect();
    }

private int phase=0;
    @Override
    public void onCharacteristicRead(BluetoothGatt bluetoothGatt, BluetoothGattCharacteristic bluetoothGattCharacteristic, int status) {
        final var value=bluetoothGattCharacteristic.getValue();
        final var uuid=bluetoothGattCharacteristic.getUuid().toString();
        if(doLog) {Log.d(LOG_ID, bluetoothGatt.getDevice().getAddress() + " onCharacteristicRead, status:" + status + " UUID:" + uuid);};;

      if(status!=GATT_SUCCESS) {
            if(status == GATT_INSUFFICIENT_AUTHENTICATION) { 
                if(doLog)
                    Log.i(LOG_ID,"onCharacteristicRead GATT_INSUFFICIENT_AUTHENTICATION");
                return;
                }
            wrotepass[1] = System.currentTimeMillis();
            if(uuid.equals(CGMStatusCharUUID)) {
                final var bondstate = bluetoothGatt.getDevice().getBondState();
                if(bondstate!=BOND_BONDED) {
                    handshake = "BONDING failed";
                    if(doLog)
                       Log.i(LOG_ID,handshake+" bondstate="+bondstate);
                    return;
                    }
                 }
            handshake = "CharacteristicRead "+bluetoothGattCharacteristic.getUuid().toString();
            if(doLog)
               Log.i(LOG_ID,handshake);
            disconnect();
            return;
            }
        switch(uuid) {
            case CGMStatusCharUUID: 
                {if(doLog){showbytes("Accu: phase="+phase+" CGM status ",value);};}
                Natives.accuSetStartTime(dataptr,value);
                switch(phase) {
                  case 0: bluetoothGatt.readCharacteristic(FirmwareRevisionStringChar);break;
                  case 1: bluetoothGatt.readCharacteristic(CGMSessionStartTimeChar);break;
                  default: {
                    byte[] data={(byte)0x05,(byte)0xFF,(byte)0xFF,(byte)0x36,(byte)0xF0};
                    tryer(()->writer(bluetoothGatt,CGMcontrolChar,data));
                    break;
                    }
                  };
                break;
            case FirmwareRevisionStringCharUUID:
                {if(doLog){showbytes("Accu: FirmwareRevision",value);};}
                bluetoothGatt.readCharacteristic(HardwareRevisionStringChar);
                break;
            case HardwareRevisionStringCharUUID:
                {if(doLog){showbytes("Accu: HardwareRevision",value);};}
                bluetoothGatt.readCharacteristic(ManufacturerNameChar);
                break;
            case ManufacturerNameCharUUID:
                {if(doLog){showbytes("Accu: ManufacturerName",value);};}
                bluetoothGatt.readCharacteristic(SystemIDChar);
                break;
            case SystemIDCharUUID:
                {if(doLog){showbytes("Accu: SystemID",value);};}
                bluetoothGatt.readCharacteristic(modelnumberChar);
                break;
            case modelnumberCharUUID:
                {if(doLog){showbytes("Accu: modelnumber",value);};}
                bluetoothGatt.readCharacteristic(SerialNumberChar);
                break;
            case SerialNumberCharUUID:
                {if(doLog){showbytes("Accu: SerialNumber",value);};}
                tryer(()->enableNotification(bluetoothGatt, CGMMeasurementChar));
                break;
            case CGMFeatureCharUUID:
                phase=1;
                {if(doLog){showbytes("Accu: CGMFeature",value);};}
                bluetoothGatt.readCharacteristic(AppearanceChar);
                break;
            case AppearanceCharUUID:
                {if(doLog){showbytes("Accu: AppearanceChar",value);};}
                bluetoothGatt.readCharacteristic(CGMStatusChar);
                break;
            case CGMSessionStartTimeCharUUID:
                {if(doLog){showbytes("Accu: CGMSessionStartTime",value);};}
                bluetoothGatt.readCharacteristic(CGMSessionRunTimeChar);
            case CGMSessionRunTimeCharUUID:
                {if(doLog){showbytes("Accu: CGMSessionRunTime",value);};}
                byte[] data={(byte)0x05,(byte)0x00,(byte)0x00,(byte)0x8E,(byte)0x00};
                tryer(()->writer(bluetoothGatt,CGMcontrolChar,data));

            };
    }


    @Override
    public void onCharacteristicWrite(BluetoothGatt bluetoothGatt, BluetoothGattCharacteristic bluetoothGattCharacteristic, int status) {
//        {if(doLog) {Log.d(LOG_ID, bluetoothGatt.getDevice().getAddress() + " onCharacteristicWrite, status:" + status + " UUID:" + bluetoothGattCharacteristic.getUuid().toString());};};
      if(status!=GATT_SUCCESS) {
            wrotepass[1] = System.currentTimeMillis();
            handshake = "CharacteristicWrite "+bluetoothGattCharacteristic.getUuid().toString();
            }
        
        showCharacter("onCharacteristicWrite " + bluetoothGatt.getDevice().getAddress() + " status:" + status + " ", bluetoothGattCharacteristic);
    }
/*
    @SuppressWarnings("unused")
    public void onConnectionUpdated(BluetoothGatt gatt, int interval, int latency, int timeout, int status) {
        {if(doLog) {Log.i(LOG_ID, "onConnectionUpdated interval=" + interval + " latency=" + latency + " timeout=" + timeout + " status=" + status);};};
    }
*/
private int redrawer=0;
private  void    processChanged(byte[] value) {
  long timmsec=System.currentTimeMillis();
  long res=Natives.accuProcessData(dataptr, value,timmsec);
  if(res==1L) {
      if(redrawer++%15==13)
            Applic.app.redraw();
     return;
    };
  handleGlucoseResult(res,timmsec);
  }

    @Override 
    public void onCharacteristicChanged(@NonNull BluetoothGatt gatt, @NonNull BluetoothGattCharacteristic bluetoothGattCharacteristic, @NonNull byte[] value) {
        final var uuid=bluetoothGattCharacteristic.getUuid().toString();
        {if(doLog){showbytes("AccuGattCallback phase="+phase+" onCharacteristicChanged UUID: " + uuid, value);};}
        switch(uuid) {
            case CGMMeasurementCharUUID:
                processChanged(value);
                break;
             case CGMcontrolCharUUID:
                if(value.length==4) {
                    final byte[] should={(byte)0x03,(byte)0x05,(byte)0x7D,(byte)0x8D};
                    if(doLog) {
                        if(!java.util.Arrays.equals(should,value)) {
                            if(doLog)
                                        Log.i(LOG_ID,"value different from 03 05 7D 8D");
                            }
                        }
                    gatt.readCharacteristic(CGMFeatureChar);
                    }
                else {
                    if(phase==1) {
                        phase=2;
                        gatt.readCharacteristic(CGMStatusChar);
                        }
                   else {
                    if(phase==2) {
                        phase=3;
                        wrotepass[0] = System.currentTimeMillis();
                        askValues(gatt);
                        }
                      }
                    }
                break;

            }
    }

    @Override // android.bluetooth.BluetoothGattCallback
    public void onCharacteristicChanged(BluetoothGatt bluetoothGatt, BluetoothGattCharacteristic bluetoothGattCharacteristic) {
        onCharacteristicChanged(bluetoothGatt, bluetoothGattCharacteristic, bluetoothGattCharacteristic.getValue());
    }


    @Override
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
                wrotepass[0] = System.currentTimeMillis();
                if(doLog) {Log.i(LOG_ID,"bonded");};
                if(Build.VERSION.SDK_INT < 26) {
                    var gatt=mBluetoothGatt;
                    if(gatt!=null)
                        tryer(()->gatt.readCharacteristic(CGMStatusChar));
                    }

                };break;
        }
    }

@Override
public boolean matchDeviceName(String deviceName,String address) {
    return 	deviceName.startsWith("AC-")&&deviceName.endsWith(SerialNumber);
    }

private final UUID CGMserviceUUID=UUID.fromString("0000181f-0000-1000-8000-00805f9b34fb");
@Override
public UUID getService() {
   return CGMserviceUUID;
   }
}

