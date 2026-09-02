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
/*      Thu Mar 28 20:14:36 CET 2024                                                 */


package tk.glucodata;

import static android.bluetooth.BluetoothDevice.BOND_BONDED;
import static android.bluetooth.BluetoothGatt.GATT_SUCCESS;

import static tk.glucodata.Applic.app;
import static tk.glucodata.Log.doLog;
import static tk.glucodata.Libre2GattCallback.showCharacter;

import android.os.SystemClock;
import android.annotation.SuppressLint;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.BluetoothStatusCodes;
import android.media.Ringtone;
import android.media.RingtoneManager;
import android.net.Uri;
import android.os.Build;

import androidx.annotation.NonNull;

import java.util.UUID;
import java.util.concurrent.TimeUnit;


class AidexXGattCallback extends SuperGattCallback {
private static final boolean askStartTime=false;
    static private final String LOG_ID = "AidexXGattCallback";
AidexXGattCallback(String SerialNumber, long dataptr) {
        super(SerialNumber, dataptr, 0x50);
        if(doLog) {Log.d(LOG_ID, SerialNumber + " AidexXGattCallback(..)");};
    }


private boolean writeCharDefault(BluetoothGatt bluetoothGatt, BluetoothGattCharacteristic cha,byte[] cmd) {
        return writeCharAlg(bluetoothGatt,cha,cmd,BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT);
        }


private void askKey(BluetoothGatt bluetoothGatt) {
            unbondedwrite=false;
            unbondedchange=false;
            byte[] data=Natives.aidexXaskKey(dataptr);
            writeCharDefault(bluetoothGatt,charactPrivateUnbonded, data) ;
            }

@SuppressLint("MissingPermission")
@Override // android.bluetooth.BluetoothGattCallback
public void onDescriptorWrite(BluetoothGatt bluetoothGatt, BluetoothGattDescriptor bluetoothGattDescriptor, int status) {
    final var cha=bluetoothGattDescriptor.getCharacteristic();
    if(doLog) {
        final var uuidstr=cha.getUuid().toString();
        Log.i(LOG_ID,"onDescriptorWrite "+uuidstr+" status="+status+" bondstate="+mActiveBluetoothDevice.getBondState() );
        }
    final int instance=cha.getInstanceId();
    if(status==BluetoothGatt.GATT_SUCCESS) {
        if(instance==privateBondedinstance)
            enableNotification(bluetoothGatt,charact);
         else {
           if(instance==privateUnbondedinstance) {
                if(askStartTime)
                   bluetoothGatt.readCharacteristic(charactStartTime);
                else {
                    enableNotification(bluetoothGatt,charact);
                    }
                }                
            else {
                if(!keywhenconnecting) {
                    askKey(bluetoothGatt);
                    }
                else
                    bluetoothGatt.readCharacteristic(charact); 
                }
            }
        }
    else {
        if(instance==privateBondedinstance) {
               enableNotification(bluetoothGatt, charactPrivateBonded);
               }
         else {
           if(instance==privateUnbondedinstance) {
               enableNotification(bluetoothGatt, charactPrivateUnbonded);
               }                
            else {
                enableNotification(bluetoothGatt,charact);
                }
           }
       }
    }
//private long connectedtime=0L;
//private ArrayList<String> triedsensors=new ArrayList<>();

private int bondstate;
private boolean keywhenconnecting=false;
private boolean connected=false;
    @SuppressLint("MissingPermission")
    @Override
    public void onConnectionStateChange(BluetoothGatt bluetoothGatt, int status, int newState) {
        if(!acceptConnectionStateChange(bluetoothGatt,newState))
            return;
        if (stop) {
             constatstatusstr="Stopped"; 
            if(doLog) {Log.i(LOG_ID, "onConnectionStateChange stop==true");};
            return;
        }
        long tim = System.currentTimeMillis();
        bondstate = bluetoothGatt.getDevice().getBondState();
        String bondstr=bondString(bondstate);
        if (doLog) {
                final String[] state = {"DISCONNECTED", "CONNECTING", "CONNECTED", "DISCONNECTING"};
                {if(doLog) {Log.i(LOG_ID, SerialNumber + " onConnectionStateChange, status:" + status + ", state: " + (newState < state.length ? state[newState] : newState) + " " + bondstr);};};

        }
        if(newState == BluetoothProfile.STATE_CONNECTED) {
             setUnpairMessage("Connecting "+SerialNumber);
             constatchange[0] = tim;
             connected=true;
             keywhenconnecting=Natives.aidexHasKey(dataptr)&&bondstate==BOND_BONDED;
             bluetoothGatt.discoverServices();
        } else {

            connected=false;
//            setConStatus(status);
            constatstatusstr = "Status="+status+" "+bondstr;
            constatchange[1] = tim;
            if(newState == BluetoothProfile.STATE_DISCONNECTED) {
                if(status==22) {
                     divorcer(false);
                     }
               if(!autoconnect) {
                   searchforDeviceAddress();
                   if(!closeCurrentGatt(bluetoothGatt))
                       return;
                   if(!stop) {
                        var sensorbluetooth = SensorBluetooth.blueone;
                        if(sensorbluetooth!=null) {
                            sensorbluetooth.connectToActiveDevice(this, 0);
                       }
                   }
                  }
               else {
                   if(!stop) {
                       bluetoothGatt.connect();
                       }
                   else {
                       if(!closeCurrentGatt(bluetoothGatt))
                           return;
                       }
                   }
              }
             }          
    }

    private static final String UUIDservice = "0000181f-0000-1000-8000-00805f9b34fb";
    private static final String UUIDchar = "0000f002-0000-1000-8000-00805f9b34fb";
    private static final String UUIDcharPrivateUnbonded = "0000f001-0000-1000-8000-00805f9b34fb";
    private static final String UUIDcharPrivateBonded = "0000f003-0000-1000-8000-00805f9b34fb";
    private static final String UUIDminutesFromStartTime = "00002aa9-0000-1000-8000-00805f9b34fb";
    private static final String UUIDStartTime = "00002aaa-0000-1000-8000-00805f9b34fb";
    private static final String UUIDRunTime = "00002aab-0000-1000-8000-00805f9b34fb";

    private BluetoothGattCharacteristic charact;
    private BluetoothGattCharacteristic charactPrivateUnbonded;
    private BluetoothGattCharacteristic charactPrivateBonded;
    private static final String UUIDota_service="1d14d6ee-fd63-4fa1-bfa4-8f47b42119f0";
    private static final String UUIDOtaControl="f7bf3564-fb6d-4e53-88a4-5e37e0326063";
   private static final String UUIDOtaData="984227f3-34fc-4045-a5d0-2c581f81a153";

    private BluetoothGattCharacteristic charactOtaControl;
    private BluetoothGattCharacteristic charactOtaData;
    private BluetoothGattCharacteristic charactRunTime;
    private BluetoothGattCharacteristic charactStartTime;
    private BluetoothGattCharacteristic charactminutesFromStartTime;


    private int               privateBondedinstance, privateUnbondedinstance, charactInstance, startTimeInstance, minutesFromStartTimeInstance, otaControlinstance;

    private boolean discover(BluetoothGatt bluetoothGatt) {
        for (BluetoothGattService bluetoothGattService : bluetoothGatt.getServices()) {
            if(doLog)
                Log.i(LOG_ID, "Service: "+bluetoothGattService.getUuid().toString());
            for(var s: bluetoothGattService.getCharacteristics()) {
                var uuid=s.getUuid().toString();
                if(doLog)
                    Log.i(LOG_ID,"Characteristic: "+uuid);
                switch(uuid) {
                    case UUIDOtaControl: 
                        charactOtaControl=s;
                        otaControlinstance=charactOtaControl.getInstanceId();
                        break;
                    case UUIDOtaData: charactOtaData=s;break;
                    case UUIDcharPrivateUnbonded: 
                        charactPrivateUnbonded=s;
                        privateUnbondedinstance=s.getInstanceId();
                        break;
                    case UUIDcharPrivateBonded: 
                        charactPrivateBonded=s;
                        privateBondedinstance=s.getInstanceId();
                        break;
                    case UUIDchar: charact=s;
                        charactInstance=s.getInstanceId();

                        break;
                    case UUIDRunTime: charactRunTime=s;break;
                    case UUIDStartTime: 
                        charactStartTime=s;
                        startTimeInstance=s.getInstanceId();
                        break;
                    case UUIDminutesFromStartTime: 
                        charactminutesFromStartTime=s;
                        minutesFromStartTimeInstance= s.getInstanceId();
                        break;

                   }
            }
        }
    if(keywhenconnecting) {
         unbondedchange=true;
        }
    var cha= keywhenconnecting?charactPrivateBonded:charactPrivateUnbonded;

    enableNotification(bluetoothGatt,cha);
    return true;
    }




    @Override // android.bluetooth.BluetoothGattCallback
    public void onServicesDiscovered(BluetoothGatt bluetoothGatt, int status) {
        if(doLog) {Log.i(LOG_ID, "BLE onServicesDiscovered, status: " + status);};
        if(status == GATT_SUCCESS) {
                if(!discover(bluetoothGatt)) 
                         disconnect();
                return;
        }

        disconnect();
    }


private  boolean writeCharAlg(BluetoothGatt bluetoothGatt, BluetoothGattCharacteristic cha,byte[] cmd,int writeType) {
    lastwrite=SystemClock.elapsedRealtime();
     if(android.os.Build.VERSION.SDK_INT >= 33)  {
        final int res=bluetoothGatt.writeCharacteristic( cha, cmd, writeType);
        if(res!= BluetoothStatusCodes.SUCCESS) {
            if(doLog){Log.showbytes(LOG_ID + ": " +cha.getUuid().toString()  + " writeCharacteristic(,,)="+res+" failed ", cmd);};
            return false;
            }
        }
     else {
        cha.setWriteType(writeType);
        if(!cha.setValue(cmd)) {
            if(doLog){Log.showbytes(LOG_ID + ": " +cha.getUuid().toString() + " cha.setValue failed ", cmd);};
            return false;
            }
        if(!bluetoothGatt.writeCharacteristic(cha)) {
            if(doLog){Log.showbytes(LOG_ID + ": " +cha.getUuid().toString()  + " writeCharacteristic/1 failed ", cmd);};
            return false;
            }
        }
        if(doLog){Log.showbytes(LOG_ID + ": " +cha.getUuid().toString()  +" writeType:"+writeType + " writeCharacteristic ", cmd);};
        return true;
        }
private static final boolean useUIthread=false;
private long lastwrite=0L;
private boolean writeChar(BluetoothGatt bluetoothGatt, BluetoothGattCharacteristic cha,byte[] cmd,int shouldWait) {
         final long now=SystemClock.elapsedRealtime();
         final long waited=now-lastwrite;
         Log.i(LOG_ID,"writeChar shouldwait="+shouldWait+" waited="+waited);
         if(waited<shouldWait) {
            final long delay=shouldWait-waited;
            Runnable writer= ()-> writeCharAlg(bluetoothGatt,cha,cmd,BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE); 
            if(useUIthread) 
                        Applic.postDelayed(writer, delay);
            else 
                        Applic.scheduler.schedule(writer, delay, TimeUnit.MILLISECONDS);
            return true;
            }
          if(useUIthread)  {
                Applic.RunOnUiThread(()-> writeCharAlg(bluetoothGatt,cha,cmd,BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE));
                return true;
                }
          else
              return writeCharAlg(bluetoothGatt,cha,cmd,BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE);
          }
private byte[] cmdlater;
    private void characterRead(BluetoothGatt bluetoothGatt, BluetoothGattCharacteristic bluetoothGattCharacteristic) {
       final byte[] value=bluetoothGattCharacteristic.getValue();
       if(doLog) {
            final var uuid=bluetoothGattCharacteristic.getUuid().toString();
            Log.showbytes(LOG_ID+ " onCharacteristicRead "+uuid,value);
            };
       int instance= bluetoothGattCharacteristic.getInstanceId();
       if(instance== charactInstance) {
           var cmd = Natives.aidexXstartCommand(keywhenconnecting,dataptr, value);
           if(cmd == null) {
               handshake = "key failed";
               wrotepass[1] = System.currentTimeMillis();
               disconnect();
               return;
               }
           handshake = "received session key";
           writeChar(bluetoothGatt, charact, cmd,20);
           return;
           }
       if(instance==otaControlinstance) {
           Log.i(LOG_ID, "boot version " + value[3] + "." + value[2] + "." + value[0]);
           didOtaControl = true;
           writeChar(bluetoothGatt, charact, cmdlater,20);
           return;
           }
       if(askStartTime) {        
           if(instance==startTimeInstance) {
               if (doLog) Log.i(LOG_ID, "startTime");
               bluetoothGatt.readCharacteristic(charactminutesFromStartTime);
               return;
                }
           if(instance==minutesFromStartTimeInstance) {
               if (doLog) Log.i(LOG_ID, "minutesFromStartTime");
              // bluetoothGatt.readCharacteristic(charact);
               
               askKey(bluetoothGatt);
               return;
                }
              }
     }
   @Override 
   public void onCharacteristicRead(BluetoothGatt bluetoothGatt, BluetoothGattCharacteristic bluetoothGattCharacteristic, int status) {
     if(status==GATT_SUCCESS) {
            characterRead(bluetoothGatt,  bluetoothGattCharacteristic) ;
            return;
           }
    wrotepass[1] = System.currentTimeMillis();
    handshake = "CharacteristicRead "+bluetoothGattCharacteristic.getUuid().toString();
    }

//private String ownDeviceName;
private boolean unbondedwrite=false;
private boolean unbondedchange=false;
    @Override
    public void onCharacteristicWrite(BluetoothGatt bluetoothGatt, BluetoothGattCharacteristic bluetoothGattCharacteristic, int status) {
       if(doLog)
          showCharacter("onCharacteristicWrite " + bluetoothGatt.getDevice().getAddress() + " status:" + status + " ", bluetoothGattCharacteristic);
    if(status==BluetoothGatt.GATT_SUCCESS) {
        int inst=bluetoothGattCharacteristic.getInstanceId();
        if(inst== privateUnbondedinstance) {
            if(unbondedchange) {
                Log.i(LOG_ID,"readCharacteristic "+UUIDchar);
                bluetoothGatt.readCharacteristic(charact);
                   //  enableNotification(bluetoothGatt,charactPrivateBonded);
                }
             else 
                unbondedwrite=true;
            }
          }
    }
/*
    @Override
    public void onConnectionUpdated(BluetoothGatt gatt, int interval, int latency, int timeout, int status) {
        {if(doLog) {Log.i(LOG_ID, "onConnectionUpdated interval=" + interval + " latency=" + latency + " timeout=" + timeout + " status=" + status);};};
    }
*/

private int changed=0;
private boolean didOtaControl=false;
private static final boolean retrywrite=false;
BooleanConsumer  divorce=null;
void unpair(BooleanConsumer runa) {
    Log.i(LOG_ID,"unpair");
    divorce=runa;
    Natives.aidexXunpair(dataptr);
    disconnect();
     Applic.scheduler.schedule(() -> { 
               setUnpairReady(Applic.app.getString(R.string.timedout),false);
           },60, TimeUnit.SECONDS);
    }
private void divorcer(boolean success) {
       final var di=divorce;
       if(di!=null) {
            Log.i(LOG_ID,"divorcer divorce!=null");
            divorce=null;
            di.accept(success);
            }
        else
            Log.i(LOG_ID,"divorcer divorce=null");
        }
void clear(BooleanConsumer runa) {
    Log.i(LOG_ID,"clear");
    divorce=runa;
    Natives.aidexXclear(dataptr);
    disconnect();
     Applic.scheduler.schedule(() -> { 
               setUnpairReady(Applic.app.getString(R.string.timedout),false);
           },60, TimeUnit.SECONDS);
    }
private int refreshiter=0;
    @Override // android.bluetooth.BluetoothGattCallback
    public void onCharacteristicChanged(@NonNull BluetoothGatt bluetoothGatt, @NonNull BluetoothGattCharacteristic bluetoothGattCharacteristic, @NonNull byte[] value) {
        final UUID uuid = bluetoothGattCharacteristic.getUuid();
        final String uuidstr=uuid.toString();
        if(doLog)
            {Log.showbytes("AidexXGattCallback  onCharacteristicChanged UUID: " + uuidstr, value);};
        int inst=bluetoothGattCharacteristic.getInstanceId();
          if(inst== privateBondedinstance) {
                final long timmsec = System.currentTimeMillis();
                long[] timear={timmsec};
                final long res=Natives.aidexXprocessCurrentData(dataptr,value,timear);
                if(res==1L) {
                    disconnect();
                    return;
                    }
                if(res==2L) {
                    return;
                    }
                final long newtime= timear[0];
                handleGlucoseResult(res,newtime);
                return;
                }
        if(inst==charactInstance) {
                ++changed;
                final byte[] cmd=Natives.aidexXprocessHistoryData(dataptr, mActiveBluetoothDevice.getBondState()!=BluetoothDevice.BOND_NONE,value);
                if(cmd!=null) {
                    if(refreshiter++%13==12)
                        Applic.app.redraw();
                    switch(cmd.length) {
                        case 0: 
                            Applic.app.redraw();
                            return;
                        case 1: 
                            switch(cmd[0]) {
                               case 1: {
                                   final String mess=SerialNumber+Applic.app.getString(R.string.unpairsuccess);
                                   Log.i(LOG_ID,mess);
                                   setUnpairReady(mess,true);
//                                   Applic.Toast(mess, Toast.LENGTH_LONG);
                                   disconnect();
                                   return;
                                   }
                               case 0: {
                                   final String mess=SerialNumber+Applic.app.getString(R.string.unpairfailed);
                                   setUnpairReady(mess,false);
                                   Log.i(LOG_ID,mess);
//                                 Applic.Toast(mess, Toast.LENGTH_LONG);
                                   disconnect();
                                   return;
                                   }
                               case 2:  {
                                   final String mess=SerialNumber+Applic.app.getString(R.string.clearsuccess);
                                   Log.i(LOG_ID,mess);
                                   setUnpairReady(mess,true);
//                                   Applic.Toast(mess, Toast.LENGTH_LONG);
                                   unbond();
                                   disconnect();
                                   return;
                                   }
                               case 3: {
                                   final String mess=SerialNumber+Applic.app.getString(R.string.clearfailed);
                                   setUnpairReady(mess,false);
                                   Log.i(LOG_ID,mess);
//                                 Applic.Toast(mess, Toast.LENGTH_LONG);
                                   disconnect();
                                   return;
                                   }
                               default: Log.e(LOG_ID,"aidexXprocessHistoryData="+cmd[0]); 
                                return;
                                }
                        case 4, 20:break;
                        case 13: handshake="starttime "+ tk.glucodata.util.timestring(Natives.getSensorStartmsec(dataptr));break;
                        case 14: handshake="item";break;
                        default: 
                           wrotepass[0] = System.currentTimeMillis();
                            handshake="old values";

                        };
//                    if(!keywhenconnecting&&!didOtaControl) {
                    if(false) {
                        bluetoothGatt.readCharacteristic(charactOtaControl);
                        if(doLog)
                            Log.i(LOG_ID, "readCharacteristic OtaControl");
                        cmdlater=cmd;
                        }
                     else {

                       writeChar(bluetoothGatt,charact, cmd,20);
if(retrywrite)  {
                        final int waschanged=changed;
                       Applic.scheduler.schedule(() -> { 
                            if(waschanged==changed) {
                               writeChar(bluetoothGatt,charact, cmd,0);
                               }
                           },400, TimeUnit.MILLISECONDS);
                        }
                        }
                    }
                else {
                   handshake = "asked disconnect";
                   wrotepass[1] = System.currentTimeMillis();
                   disconnect();
                   }
                return;
                }
        if(inst== privateUnbondedinstance) {
                if(!unbondedchange) {
                    if(!Natives.aidexXSaveKey(dataptr,value)) {
                         handshake = "aidexXSaveKey error";
                         wrotepass[1] = System.currentTimeMillis();
                         disconnect();
                         return;
                         }
                      }
                   handshake = "Masterkey";
                   if(unbondedwrite) {
                      Log.i(LOG_ID,"readCharacteristic "+UUIDchar);
                      bluetoothGatt.readCharacteristic(charact);
                    }
                  else
                        unbondedchange=true;
                        
                return;
                }
    };


    @Override // android.bluetooth.BluetoothGattCallback
    public void onCharacteristicChanged(BluetoothGatt bluetoothGatt, BluetoothGattCharacteristic bluetoothGattCharacteristic) {
        {if(doLog) {Log.i(LOG_ID, "onCharacteristicChanged/2");};};
        onCharacteristicChanged(bluetoothGatt, bluetoothGattCharacteristic, bluetoothGattCharacteristic.getValue());
    } 


    @Override
    public void onReadRemoteRssi(BluetoothGatt gatt, int rssi, int status) {
        {if(doLog) {Log.i(LOG_ID, "onReadRemoteRssi(BluetoothGatt," + rssi + "," + status + (status == GATT_SUCCESS ? " SUCCESS" : " FAILURE"));};};
        if (status == GATT_SUCCESS) {
            readrssi = rssi;
        }
    }
  @Override 
    public final void onMtuChanged(BluetoothGatt bluetoothGatt, int mtu, int status) {
        if(status==BluetoothGatt.GATT_SUCCESS) {
           if(doLog)
                Log.i(LOG_ID,"onMtuChanged "+mtu+" SUCCESS");
        }
      else {
            Log.e(LOG_ID,"onMtuChanged failed");
            }
    }


       // public boolean regionMatches(int toffset, String other, int ooffset, int len)
private static final String  startAidexX="AiDEX X-";
private static final String  startLinX="LinX-";
private static final String  startLumi="Lumi-";
private static final String  startSmart="Smart-";


public static boolean hasDeviceName(String deviceName) {
       return deviceName.startsWith(AidexXGattCallback.startLumi)|| deviceName.startsWith(AidexXGattCallback.startSmart) || deviceName.startsWith(AidexXGattCallback.startLinX)||deviceName.startsWith(AidexXGattCallback.startAidexX) ;
       }
public static String deviceName2name(String deviceName) {
        final int namelen=deviceName.length();
        if(deviceName.startsWith(AidexXGattCallback.startLumi)) {
                return "i"+deviceName.substring(namelen-10);
                }
      if(deviceName.startsWith(AidexXGattCallback.startLinX)) {
                return "L"+deviceName.substring(namelen-10);
                 }
        return "x"+deviceName.substring(namelen-10);
       }

    @Override
    public boolean matchDeviceName(String nameDevice, String address) {
    /*
        if(!nameDevice.startsWith(startAidexX)) {
            Log.i(LOG_ID,"not Aidex X: "+nameDevice);
            return false;
            }
            */
        int len=Natives.getSerialLength(dataptr);
        if(!nameDevice.regionMatches(nameDevice.length()-len,SerialNumber, SerialNumber.length()-len, len)) {
            Log.i(LOG_ID,"Doesnt end with "+SerialNumber+" "+nameDevice);
            return false;
            }
        return true;
        }


    static final UUID ScanServiceUUID = UUID.fromString(UUIDservice);
    @Override
    public UUID getService() {
        return  ScanServiceUUID;
        }
@Override
public void close() {
   super.close();
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
            };break;
    }
}

@Override
public void free() {
        unbond();
        super.free();
    }

/*
@Override
protected void unbond() {
    Log.i(LOG_ID,"unbond() called, not done");
    }
    */
@Override
boolean asAdvertised(byte[] value) {
     long[] timebuf={System.currentTimeMillis()};
     long res=Natives.aidexXscanBytes(dataptr,value,timebuf);
     if(res>3L)
               handleGlucoseResult(res,timebuf[0]);
      return true;
    }








static boolean addbyDeviceName(MainActivity act,String deviceName) {
    String name=Natives.aidexXaddSensorByDeviceName(deviceName);
    if(name!=null&& !name.isEmpty())  {
       MainActivity.tocalendarapp=true;
       if(Natives.getusebluetooth()) {
           var res=SensorBluetooth.updateDevices();
           SuperGattCallback.glucosealarms.setLossAlarm();
           if(res) {
                act.finepermission(); 
                }
              else
                act.systemlocation();
            }
        else {
            Natives.updateUsedSensors();
            }
       Applic.wakemirrors();
       act.requestRender();
       return true;
       }
    return false;
    }
private UnpairOverlayHost overlayHost;

/*
public void startUnpair(MainActivity act,Runnable ended) {
    startUnpair(new UnpairOverlayHost(act),ended);
    }
*/
public void startUnpair(UnpairOverlayHost host,Predicate<Boolean> ended) {
    overlayHost=host;
    setUnpairMessage(Applic.app.getString(R.string.unpairing)+SerialNumber); 
    unpair(success -> {
            if(ended.test(success)) 
                host.postCloser();
            });
    }
public void startClear(UnpairOverlayHost host,Predicate<Boolean> ended) {
    overlayHost=host;
    setUnpairMessage(Applic.app.getString(R.string.clearing)+SerialNumber); 
    clear(success -> {
            if(ended.test(success)) 
                host.postCloser();
            });
    }
private void setUnpairMessage(String message) {
    UnpairOverlayHost host = overlayHost;
    if (host != null) {
        host.postMessage(message);
    }
}

private void       setUnpairReady(String mess,boolean success) {
    UnpairOverlayHost host = overlayHost;
    if(host != null) {
         host.postFinished(mess,  success);
         overlayHost=null;
         }
    divorcer(success);
   }
}



