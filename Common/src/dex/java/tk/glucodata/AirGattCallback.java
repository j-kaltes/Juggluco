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

import static android.app.PendingIntent.getBroadcast;
import static android.bluetooth.BluetoothDevice.BOND_BONDED;
import static android.bluetooth.BluetoothDevice.BOND_NONE;
import static android.bluetooth.BluetoothDevice.TRANSPORT_LE;
import static android.bluetooth.BluetoothGatt.GATT_INSUFFICIENT_AUTHENTICATION;
import static android.bluetooth.BluetoothGatt.GATT_SUCCESS;
import static android.bluetooth.BluetoothGattCharacteristic.FORMAT_SINT16;
import static android.bluetooth.BluetoothGattCharacteristic.FORMAT_SINT8;
import static android.bluetooth.BluetoothGattCharacteristic.FORMAT_UINT16;
import static android.bluetooth.BluetoothGattCharacteristic.FORMAT_UINT32;
import static android.bluetooth.BluetoothGattCharacteristic.FORMAT_UINT8;
import static android.bluetooth.BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE;
import static android.content.Context.ALARM_SERVICE;
import static android.content.Context.POWER_SERVICE;
import static java.util.Objects.nonNull;
import static tk.glucodata.Applic.TEST;
import static tk.glucodata.Applic.app;
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.DexGattCallback.setalarm;
import static tk.glucodata.Log.doLog;
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
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.Charset;
import java.nio.charset.StandardCharsets;
import java.text.SimpleDateFormat;
import java.util.Arrays;
import java.util.TimeZone;
import java.util.UUID;
import java.util.concurrent.TimeUnit;

import static tk.glucodata.util.sleep;
import static tk.glucodata.util.timestring;

import javax.crypto.Cipher;
import javax.crypto.spec.IvParameterSpec;
import javax.crypto.spec.SecretKeySpec;

class AirGattCallback extends SuperGattCallback {
private final boolean askExtraInfo=doLog;
    static private final String LOG_ID = "AirGattCallback";
  private  boolean unusedSensor;
  private boolean didRun=false;
AirGattCallback(String SerialNumber, long dataptr) {
        super(SerialNumber, dataptr, 0x30);
        if(doLog) {Log.d(LOG_ID, SerialNumber + " AirGattCallback(..)");};
        showtime=6*60*1000L;
       unusedSensor=Natives.airGetLast(dataptr)<=0;
    }
private final String csairKey = "tq1Tg265o4UFD8tfPvNqUCiYyCxkhdZV";
private String swRevision ="";
private  boolean  receiveNotes=false;
final private String AppID="csair";
@SuppressLint("MissingPermission")
@Override // android.bluetooth.BluetoothGattCallback
public void onDescriptorWrite(BluetoothGatt bluetoothGatt, BluetoothGattDescriptor bluetoothGattDescriptor, int status) {
        super.onDescriptorWrite(bluetoothGatt, bluetoothGattDescriptor, status);
        BluetoothGattCharacteristic characteristic = bluetoothGattDescriptor.getCharacteristic();
        if (doLog) {
            byte[] value = bluetoothGattDescriptor.getValue();
            {if(doLog){Log.showbytes(LOG_ID+" onDescriptorWrite char: " + characteristic.getUuid().toString() + " desc: " + bluetoothGattDescriptor.getUuid().toString() + " status=" + status, value);};}
        }
        if(status != BluetoothGatt.GATT_SUCCESS) {
            if(status == GATT_INSUFFICIENT_AUTHENTICATION) {
                Log.e(LOG_ID,"onDescriptorWrite GATT_INSUFFICIENT_AUTHENTICATION");
                return;
            }
          //  wrotepass[1] = System.currentTimeMillis();
          //  handshake = "onDescriptorWrite "+characteristic.getUuid().toString();
            return;
           }
        String uuidstr = bluetoothGattDescriptor.getCharacteristic().getUuid().toString();
        if(uuidstr.equals(UUIDchar11)) {
            enableNotification(bluetoothGatt,charact21);
            return;
            }
        if(uuidstr.equals(UUIDchar21)) {
            sleep(500L);
            charact21.setWriteType(WRITE_TYPE_NO_RESPONSE);
            if(swRevision.compareTo("1.4") < 0) {
                byte b2=0;
                charact21.setValue(new byte[]{-64, 1, (byte) 16, (byte) 39, b2, b2, (byte) (b2 & 255), (byte) ((b2 >> 8) & 255), (byte) 1, b2, b2});
                }
            else {
                String serial = sensorSerial;
                final int seriallen=serial.length();
                final var lastsix=serial.substring(seriallen - 6);
                StringBuilder sb2 = new StringBuilder();
                sb2.append(lastsix);
                sb2.append(lastsix);
                sb2.append(serial.substring(seriallen - 4));
                String badnonse = sb2.toString();
                ByteBuffer byteBufferAllocate2 = ByteBuffer.allocate(18);
                byteBufferAllocate2.order(ByteOrder.LITTLE_ENDIAN);
                try {
                    Charset charset = StandardCharsets.UTF_8;
                    SecretKeySpec secretKeySpec = new SecretKeySpec(csairKey.getBytes(charset), "AES");
                    Cipher cipher = Cipher.getInstance("AES/CBC/PKCS5Padding");
                    cipher.init(1, secretKeySpec, new IvParameterSpec(badnonse.getBytes(charset)));
                    byte[] bArrDoFinal = cipher.doFinal(serial.getBytes(charset));
                    byteBufferAllocate2.put((byte) -64);
                    byteBufferAllocate2.put((byte) 1);
                    for (byte b10 : bArrDoFinal) {
                        byteBufferAllocate2.put((byte) (b10 & 255));
                    }
                } catch (Throwable th) {
                    Log.stack(LOG_ID,"onDescriptorWrite encrypt",th);
                    disconnect();
                    return;
                    }
                charact21.setValue(byteBufferAllocate2.array());
               }
            receiveNotes=false;
            bluetoothGatt.writeCharacteristic(charact21);
            return;
            }
        if(uuidstr.equals(UUIDchar22)) {
                sleep(100L);
                charact22.setWriteType(WRITE_TYPE_NO_RESPONSE);
                byte[] buf=new byte[35];
                final byte[] start={-64,3,(byte)'c',(byte)'s',(byte)'a',(byte)'i',(byte)'r'};
                System.arraycopy(start,0,buf,0,start.length);
                if(unusedSensor) 
                    buf[34]=1; 
                charact22.setValue(buf);
                bluetoothGatt.writeCharacteristic(charact22);
                receiveNotes=false;
                return;
                }
    }

//private long connectedtime=0L;
//private ArrayList<String> triedsensors=new ArrayList<>();

private PendingIntent onalarm=null;
private void cancelalarm() {
    if(onalarm!=null) {
        {if(doLog) {Log.i(LOG_ID,"cancelalarm");};};
        AlarmManager manager= (AlarmManager) Applic.app.getSystemService(ALARM_SERVICE);
        manager.cancel(onalarm);
        onalarm=null;//TODO: ?????
        }
    }
private boolean connected=false;
    @SuppressLint("MissingPermission")
    @Override
    public void onConnectionStateChange(BluetoothGatt bluetoothGatt, int status, int newState) {
        if (stop) {
             constatstatusstr="Stopped"; //"
            if(doLog) {Log.i(LOG_ID, "onConnectionStateChange stop==true");};
            return;
        }
        long tim = System.currentTimeMillis();
        final var bondstate = bluetoothGatt.getDevice().getBondState();
        if (doLog) {
                final String[] state = {"DISCONNECTED", "CONNECTING", "CONNECTED", "DISCONNECTING"};
                {if(doLog) {Log.i(LOG_ID, SerialNumber + " onConnectionStateChange, status:" + status + ", state: " + (newState < state.length ? state[newState] : newState) + " bondstate=" + bondstate);};};

        }
        if(newState == BluetoothProfile.STATE_CONNECTED) {
              bluetoothGatt.requestMtu(512);
              constatchange[0] = tim;
              connected=true;
        } else {
            resetValues();
            connected=false;
            setConStatus(status);
            constatchange[1] = tim;
            if(newState == BluetoothProfile.STATE_DISCONNECTED) {
               if(!autoconnect) {
                   bluetoothGatt.close();
                   mBluetoothGatt = null;
                   if(!stop) {
                        var sensorbluetooth = SensorBluetooth.blueone;
                        if(sensorbluetooth!=null) {
                                final long alreadywaited = tim - datatime;
                                if(getalarmclock()) {
                                    final long mmsectimebetween = 5 * 60 * 1000;
                                    long stillwait = mmsectimebetween - alreadywaited - 1000;
                                    {if(doLog) {Log.i(LOG_ID,  " alreadywaited=" + alreadywaited + " stillwait=" + stillwait);};};
                                    if(stillwait>0)
                                        onalarm=setalarm(tim+stillwait,onalarm,SerialNumber );
                                     else
                                        sensorbluetooth.connectToActiveDevice(this, 0);
                                } else {
                                    sensorbluetooth.connectToActiveDevice(this, 0);
                                }
                            }
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
    private static final String UUIDservice1 = "c4de7bda-5a9d-11e9-8647-d663bd873d93";
    private static final String UUIDservice2 = "c4de9a20-5a9d-11e9-8647-d663bd873d93";
    private static final String UUIDservice3 = "c4de9dc2-5a9d-11e9-8647-d663bd873d93";
    private static final String UUIDchar1 = "c4de7e96-5a9d-11e9-8647-d663bd873d93";
    private static final String UUIDchar2 = "c4de83c8-5a9d-11e9-8647-d663bd873d93";
    private static final String UUIDchar3 = "c4de8544-5a9d-11e9-8647-d663bd873d93";
    private static final String UUIDchar4 = "c4de86a2-5a9d-11e9-8647-d663bd873d93";
    private static final String UUIDchar5 = "c4de87e2-5a9d-11e9-8647-d663bd873d93";
    private static final String UUIDchar6 = "c4de89ae-5a9d-11e9-8647-d663bd873d93";
    private static final String UUIDchar7 = "c4de8af8-5a9d-11e9-8647-d663bd873d93";
    private static final String UUIDchar11 = "c4de9b74-5a9d-11e9-8647-d663bd873d93";
    private static final String UUIDchar21 = "c4de9ee4-5a9d-11e9-8647-d663bd873d93";
    private static final String UUIDchar22 = "c4dec61c-5a9d-11e9-8647-d663bd873d93";

    private BluetoothGattCharacteristic charact1;
    private BluetoothGattCharacteristic charact2;
    private BluetoothGattCharacteristic charact3;
    private BluetoothGattCharacteristic charact4;
    private BluetoothGattCharacteristic charact5;
    private BluetoothGattCharacteristic charact6;
    private BluetoothGattCharacteristic charact7;
    private BluetoothGattCharacteristic charact11;
    private BluetoothGattCharacteristic charact21;
    private BluetoothGattCharacteristic charact22;


    private boolean discover(BluetoothGatt bluetoothGatt) {
        for (BluetoothGattService bluetoothGattService : bluetoothGatt.getServices()) {
            if(doLog)
                Log.i(LOG_ID, "Service: "+bluetoothGattService.getUuid().toString());
            for(var s: bluetoothGattService.getCharacteristics()) {
                var uuid=s.getUuid().toString();
                if(doLog)
                    Log.i(LOG_ID,"Characteristic: "+uuid);
                switch(uuid) {
                    case UUIDchar1: charact1=s;break;
                    case UUIDchar2: charact2=s;break;
                    case UUIDchar3: charact3=s;break;
                    case UUIDchar4: charact4=s;break;
                    case UUIDchar5: charact5=s;break;
                    case UUIDchar6: charact6=s;break;
                    case UUIDchar7: charact7=s;break;
                    case UUIDchar11: charact11=s;break;
                    case UUIDchar21: charact21=s;break;
                    case UUIDchar22: charact22=s;break;
                   }
            }
        }
       if(didRun) {
            afterReads(bluetoothGatt);
            }
        else  {
            if(askExtraInfo)
                if(charact1 != null) {
                     bluetoothGatt.readCharacteristic(charact1);
                     }
                else  {
                    Log.e(LOG_ID,"discover: ERROR: "+UUIDchar1 + " missing");
                    return false;
                    }
            else  {
                if(charact3 != null) {
                    bluetoothGatt.readCharacteristic(charact3);
                    }
                else  {
                    Log.e(LOG_ID,"discover: ERROR: "+UUIDchar3 + " missing");
                    return false;
                    }
                }
            }
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


   @Override 
   public void onCharacteristicRead(BluetoothGatt bluetoothGatt, BluetoothGattCharacteristic bluetoothGattCharacteristic, int status) {
   switch(status) {
    case GATT_SUCCESS: 
          if(doLog) {Log.i(LOG_ID, "onCharacteristicRead success");};
            characterRead(bluetoothGatt,  bluetoothGattCharacteristic) ;
        return;
    case GATT_INSUFFICIENT_AUTHENTICATION:
            if(doLog) {Log.i(LOG_ID, "onCharacteristicRead GATT_INSUFFICIENT_AUTHENTICATION");};
            break;
    default:
            Log.e(LOG_ID, "onCharacteristicRead "+bluetoothGattCharacteristic.getUuid()+ " status="+ status);
            break;
        }
    wrotepass[1] = System.currentTimeMillis();
    handshake = "CharacteristicRead "+bluetoothGattCharacteristic.getUuid().toString();

    }
private String sensorSerial=null;


private void afterReads(BluetoothGatt bluetoothGatt) {
      if(doLog)
            Log.i(LOG_ID,"afterReads");
        if(swRevision.compareTo("1.5") >= 0) {
           Applic.scheduler.schedule(() -> {enableNotification(bluetoothGatt,charact22); }, 100, TimeUnit.MILLISECONDS);
            return;
           }
        if(mActiveBluetoothDevice.getBondState() == BOND_BONDED) {
            wrotepass[0] = System.currentTimeMillis();
            enableNotification(bluetoothGatt, charact11);
            return;
            }
        Applic.postDelayed(()->{ mActiveBluetoothDevice.createBond();} , 1000L);
        }

    private void characterRead(BluetoothGatt bluetoothGatt, BluetoothGattCharacteristic bluetoothGattCharacteristic) {
        final UUID uuid = bluetoothGattCharacteristic.getUuid();
        final String uuidstr=uuid.toString();
        switch(uuidstr) {
            case UUIDchar1: {
                var bytes=bluetoothGattCharacteristic.getValue();
                if(doLog)
                        Log.showbytes(LOG_ID +" charact1",bytes);
                if(charact2 != null) {
                    bluetoothGatt.readCharacteristic(charact2);
                    return;
                    }
                return;
                }
            case UUIDchar2: {
                var modelNumber = bluetoothGattCharacteristic.getStringValue(0);
                if(doLog)
                    Log.i(LOG_ID,"ModelNumber: "+modelNumber);
                if (charact4 != null) {
                    bluetoothGatt.readCharacteristic(charact4);
                    return;
                }
                return;
                }
            case UUIDchar3: {
                sensorSerial = bluetoothGattCharacteristic.getStringValue(0);
                if(doLog)
                    Log.i(LOG_ID,"sensorSerial/charact3: "+sensorSerial);
                if (charact6 != null) {
                    bluetoothGatt.readCharacteristic(charact6);
                    return;
                }
                return;
                }
            case UUIDchar4: {
                String stringValue = bluetoothGattCharacteristic.getStringValue(0);
                if (stringValue.compareTo("500.300.1.1") >= 0) {
                    if(doLog)
                        Log.i(LOG_ID,"Accepted fwRevision: "+stringValue);
                    if (charact5 != null) {
                        bluetoothGatt.readCharacteristic(charact5);
                        return;
                    }
                    return;
                }
                Log.e(LOG_ID,"WRONG fwRevision: "+stringValue);
                bluetoothGatt.disconnect();
                return;
            }
            case UUIDchar5: {
                var str = bluetoothGattCharacteristic.getStringValue(0);
                if(doLog)
                    Log.i(LOG_ID,"hwRevision : "+str);
                if(charact7 != null) {
                    bluetoothGatt.readCharacteristic(charact7);
                    return;
                }
                return;
            }
            case UUIDchar7: {
                var manu = bluetoothGattCharacteristic.getStringValue(0);
                if(doLog)
                    Log.i(LOG_ID,"manufacturer: "+manu);
                if (charact3 != null) {
                    bluetoothGatt.readCharacteristic(charact3);
                    return;
                    }
                 return;
              }
            case UUIDchar6: {
               swRevision = bluetoothGattCharacteristic.getStringValue(0);
               if(doLog)
                   Log.i(LOG_ID,"swRevision: "+swRevision);
               didRun=true;
          //      if (swRevision.compareTo("1.1") >= 0) {
                       afterReads( bluetoothGatt);
//                     }
//                Log.i(LOG_ID,"ERROR swRevision: "+swRevision);
 //               bluetoothGatt.disconnect();
                return;
            }
          }
    }

//private String ownDeviceName;

    @Override
    public void onCharacteristicWrite(BluetoothGatt bluetoothGatt, BluetoothGattCharacteristic bluetoothGattCharacteristic, int status) {
       if(doLog)
          showCharacter("onCharacteristicWrite " + bluetoothGatt.getDevice().getAddress() + " status:" + status + " ", bluetoothGattCharacteristic);
         receiveNotes=status==GATT_SUCCESS ;
    }

    @SuppressWarnings("unused")
    public void onConnectionUpdated(BluetoothGatt gatt, int interval, int latency, int timeout, int status) {
        {if(doLog) {Log.i(LOG_ID, "onConnectionUpdated interval=" + interval + " latency=" + latency + " timeout=" + timeout + " status=" + status);};};
    }




private void onChar22Changed(BluetoothGatt bluetoothGatt, byte[] value) {
        if(value.length < 35) {
            Log.e(LOG_ID,"onCharacteristicChanged: value charact22 < 35: "+value.length);
            return;
           }
        if((value[0]&0xFF) == 192 && value[1] == 3) {
            final byte last=value[value.length-1];
            if(last > 0) {
                unusedSensor=false;
                switch(last) {
                    case 1: 
                        handshake="DEVICE MATCH FAILED";
                        break;
                    case 2: 
                        handshake="APPID MATCH FAILED";
                        break;
                    default:
                        handshake="RECONNECT FAILED";
                        break;
                    };
                wrotepass[1] = System.currentTimeMillis();
                Log.e(LOG_ID,"onChar22Changed: "+handshake);
                disconnect();
                return;
                }
           String strTrim = new String(value,2,value.length-3).trim();
           if(!AppID.equals(strTrim)) {
                wrotepass[1] = System.currentTimeMillis();
                handshake="appId different "+strTrim+" != "+AppID+ " last="+last;
                Log.e(LOG_ID,handshake);
                disconnect();
                return;
                }
            if(mActiveBluetoothDevice.getBondState() == BOND_BONDED) {
                wrotepass[0] = System.currentTimeMillis();
                Applic.scheduler.schedule(() -> { enableNotification(bluetoothGatt,charact11); }, 100, TimeUnit.MILLISECONDS);
                return;
                }
            Log.i(LOG_ID,"createBond");
            Applic.postDelayed(()->{ mActiveBluetoothDevice.createBond();} , 1000L);
            return;
           }
        return;
        }

private int storedRecords=-1;
private boolean answered=false;
private boolean noticedNumberRecords=false;
private boolean syncTime=false;
private void resetValues() {
    answered=false;
    syncTime=false;
    noticedNumberRecords=false;
    storedRecords=-1;
    forCRCbuf=null;
    }
private final void numberRecords(BluetoothGatt gatt) {
    if(doLog)
        Log.i(LOG_ID,"numberRecords");
    BluetoothGattCharacteristic bluetoothGattCharacteristic = charact11;
    bluetoothGattCharacteristic.setWriteType(WRITE_TYPE_NO_RESPONSE);
    bluetoothGattCharacteristic.setValue(new byte[2]);
    bluetoothGattCharacteristic.setValue(197, FORMAT_UINT8, 0);
    bluetoothGattCharacteristic.setValue(1, FORMAT_UINT8, 1);
    receiveNotes=false;
    gatt.writeCharacteristic(bluetoothGattCharacteristic);
   }
private int redrawer=0;
private void onChar11Changed(BluetoothGattCharacteristic bluetoothGattCharacteristic, BluetoothGatt bluetoothGatt, byte[] value) {
   var timmsec=System.currentTimeMillis();
   long[] timeptr={timmsec};
   long res=Natives.airProcessData( dataptr,value,timeptr);
   if(res==3L) {
        if(!noticedNumberRecords) {
            numberRecords(bluetoothGatt); 
            noticedNumberRecords=true;
            }
         return;
        }
    if(res==2L) { 
        disconnect();
        return;
        }
    if(res==1L) { 
        if(redrawer++%10==9)
            Applic.app.redraw();
        return;
        }
    long uittime=timeptr[0];
    handleGlucoseResult(res,uittime);
    if(res!=0L) {
        datatime=uittime;
        disconnect();
        }
//    if(TEST) enableNotification(bluetoothGatt, charact11); 
   }
private long datatime=0L;
private static void settwobytesplusints(BluetoothGattCharacteristic bluetoothGattCharacteristic, int i, int i2, int... numArr) {
        bluetoothGattCharacteristic.setValue(new byte[2+numArr.length*4]);
        bluetoothGattCharacteristic.setValue(i, FORMAT_UINT8, 0);
        bluetoothGattCharacteristic.setValue(i2, FORMAT_UINT8, 1);
        if(numArr.length > 0) {
            int i10 = 2;
            for (int num : numArr) {
                bluetoothGattCharacteristic.setValue(num,FORMAT_UINT32, i10);
                i10 += 4;
            }
        }
    }

   // public final void g() {
private final void askSensorInfo(BluetoothGatt bluetoothGatt ) {
        Log.i(LOG_ID,"askSensorInfo");
        BluetoothGattCharacteristic bluetoothGattCharacteristic = charact21;
        bluetoothGattCharacteristic.setWriteType(WRITE_TYPE_NO_RESPONSE);
        bluetoothGattCharacteristic.setValue(new byte[2]);
        bluetoothGattCharacteristic.setValue(194, FORMAT_UINT8, 0);
        bluetoothGattCharacteristic.setValue(1, FORMAT_UINT8, 1);
        receiveNotes = false;
     bluetoothGatt.writeCharacteristic(charact21);
    }
private final void setAppInfo(BluetoothGatt bluetoothGatt) {
        Log.i(LOG_ID,"setApplicationInformation userID: 0");
        charact21.setWriteType(WRITE_TYPE_NO_RESPONSE);
        settwobytesplusints(charact21, 192, 2, 0);
        receiveNotes=false;
       bluetoothGatt.writeCharacteristic(charact21);
    }

public final void sendSyncTime(BluetoothGatt bluetoothGatt) {
        BluetoothGattCharacteristic bluetoothGattCharacteristic = charact21;
        bluetoothGattCharacteristic.setWriteType(WRITE_TYPE_NO_RESPONSE);
        long msecs = System.currentTimeMillis();
        long secs = msecs / 1000;
        Log.i(LOG_ID,"sendSyncTime: " + timestring(msecs) + " Seconds: " + secs);
        bluetoothGattCharacteristic.setValue(new byte[]{-61, 2, (byte) ((int) (secs & 0xFF)), (byte) ((int) ((secs >> 8) & 0xFF)), (byte) ((int) ((secs >> 16) & 0xFF)), (byte) ((int) (0xFF & (secs >> 24)))});
        bluetoothGatt.writeCharacteristic(bluetoothGattCharacteristic);
        receiveNotes = false;
    }
private final void requestData(BluetoothGatt bluetoothGatt ) {
        int lastval = Natives.airGetLast(dataptr);;
        if(lastval<0) {
            disconnect();
            return;
            }
        Log.i(LOG_ID,"requestData last received: "+ lastval);
        charact11.setWriteType(WRITE_TYPE_NO_RESPONSE);
        settwobytesplusints(charact11, 196, 1,lastval);
        receiveNotes = false;
        bluetoothGatt.writeCharacteristic(charact11);
    }


private  ByteBuffer forCRCbuf=null;
private int dataCountperSet=30;
private void onChar21Changed(BluetoothGattCharacteristic bluetoothGattCharacteristic, BluetoothGatt bluetoothGatt, byte[] value) {
        int firstByte = bluetoothGattCharacteristic.getIntValue(FORMAT_UINT8, 0).intValue();
        int secondByte = bluetoothGattCharacteristic.getIntValue(FORMAT_UINT8, 1).intValue();
        if (firstByte == 192 && secondByte == 1) {
            long secs = System.currentTimeMillis() / 1000;
            long deviceTimeSecs = bluetoothGattCharacteristic.getIntValue(FORMAT_UINT32, 2).intValue();
            int userID = bluetoothGattCharacteristic.getIntValue(FORMAT_UINT32, 6).intValue();
            dataCountperSet = bluetoothGattCharacteristic.getIntValue(FORMAT_UINT8, 10).intValue();
            int AdcInterval = bluetoothGattCharacteristic.getIntValue(FORMAT_UINT16, 11).intValue();
            if (Math.abs(secs - deviceTimeSecs)>= AdcInterval/5000 ) {
                Log.i(LOG_ID,"onChar21Changed timeSync: currentTime:" + secs + " deviceTimeSeconds:" + deviceTimeSecs);
                syncTime = true;
            } else {
                syncTime = false;
            }
            int charactOff = 30;
            int QcResultFlag;
            int KeyCheckResult;
            if(swRevision.compareTo("1.5") >= 0) {
                QcResultFlag = bluetoothGattCharacteristic.getIntValue(FORMAT_UINT8, charactOff).intValue();
                KeyCheckResult = bluetoothGattCharacteristic.getIntValue(FORMAT_UINT8, charactOff + 1).intValue();
            } else {
                QcResultFlag = 0;
                KeyCheckResult = 1;
              }
            if (QcResultFlag != 0 && QcResultFlag != 16) {
                Log.e(LOG_ID,"ERROR QcResultFlag = " + QcResultFlag);
               }
            if (KeyCheckResult == 0) {
                Log.e(LOG_ID, "KeyCheckResult = 0");
                unbond();
                return;
                }
            Log.i(LOG_ID,"getApplicationInfo deviceTime: " + timestring(deviceTimeSecs*1000L) + ", UserID: " + userID + ", DataCountPerSet: " + dataCountperSet + ", AdcInterval: " + AdcInterval);
            if(Natives.airGetLast(dataptr)<=0) {
                setAppInfo(bluetoothGatt);
                return;
            } else  {
                 if (syncTime) {
                      sendSyncTime(bluetoothGatt); 
                      return;
                 } else {
                      requestData(bluetoothGatt); 
                      return;
                    }
                 }
        }
        if (firstByte == 192 && secondByte == 2) {
            float eapp,vref;
            if(swRevision.compareTo("1.3") >= 0) {
                ByteBuffer byteBufferAllocate2 = ByteBuffer.allocate(value.length);
                byteBufferAllocate2.order(ByteOrder.LITTLE_ENDIAN);
                byteBufferAllocate2.put(value);
                eapp = byteBufferAllocate2.getFloat(2);
                vref = byteBufferAllocate2.getFloat(6);
            } else {
                eapp = bluetoothGattCharacteristic.getIntValue(FORMAT_UINT32, 2).intValue() / 1.0E7f;
                vref = bluetoothGattCharacteristic.getIntValue(FORMAT_UINT32, 6).intValue() / 1.0E7f;
                }
            int elapsedSecs = bluetoothGattCharacteristic.getIntValue(FORMAT_UINT32, 10).intValue();
            Natives.airSaveStartSensor(dataptr,eapp,vref,elapsedSecs);
            askSensorInfo(bluetoothGatt);
            return;
        }

    if (firstByte == 0xC2 && secondByte == 1) {
        if(!Natives.airSaveSensorInfo(dataptr,value)) {
            disconnect();
            return;
            }
        ByteBuffer byteBufferAllocate4 = ByteBuffer.allocate(726);
        byteBufferAllocate4.order(ByteOrder.BIG_ENDIAN);
        byteBufferAllocate4.put(value,2,value.length-2);
        forCRCbuf = byteBufferAllocate4;
      return;
      }
    if(firstByte == 0xC2 && secondByte == 2) {
        ByteBuffer byteBuffer = forCRCbuf;
        if(byteBuffer != null) {
            int length = value.length - 2;
            byteBuffer.put(value,2,length);
            }
        Natives.airSaveSensorInfo2(dataptr,value);
        return;
        }
    if(firstByte ==0xC2 && secondByte == 3) {
        Log.i(LOG_ID,"onChar21Changed 0xC2 0x03: crc");
        ByteBuffer byteBuffer6 = forCRCbuf;
        final byte[] array= byteBuffer6.array();
        int length2 = array.length - 2;
        int iE = crcver1(array,length2);
        int i20 = byteBuffer6.getShort(length2) & 65535;
        if (iE != i20) {
            handshake="crc != qcCrc DISCONNECT";
            if (crcver2(array,length2) != i20) {
                disconnect();
                return;
                }
            wrotepass[1] = System.currentTimeMillis();
             Log.e(LOG_ID,handshake);
        } else {
            if(doLog)
                Log.i(LOG_ID,"crc SUCCESS");
            }
        if (syncTime) {
            sendSyncTime(bluetoothGatt);
        } else {
            requestData(bluetoothGatt);
            }
        return;
        }
  if(firstByte == 195 && secondByte == 2) {
        Log.i(LOG_ID,"setTimeSynchronization");
        requestData(bluetoothGatt);
        return;
        }
  if(firstByte == 204 && secondByte == 2) {
       handshake="Sensor Ended " + sensorSerial;
       wrotepass[1] = System.currentTimeMillis();
       if(doLog)
         Log.i(LOG_ID,handshake);
       disconnect();
       unbond();
       return;
       }
    if(firstByte == 198 && secondByte == 1) {
        Log.i(LOG_ID,"setTimeSynchronization 0xC6");
        return;
       }
    if (firstByte == 0xC6 && secondByte == 2) {
         Log.i(LOG_ID,"setAppInfo 0xC6");
        return;
        }
    if(firstByte == 205 && secondByte == 2) {
        Log.i(LOG_ID,"setTransmitterReset");
        unbond();
        disconnect();
        return;
        }

    }

    @Override // android.bluetooth.BluetoothGattCallback
    public void onCharacteristicChanged(@NonNull BluetoothGatt bluetoothGatt, @NonNull BluetoothGattCharacteristic bluetoothGattCharacteristic, @NonNull byte[] value) {
        final UUID uuid = bluetoothGattCharacteristic.getUuid();
        final String uuidstr=uuid.toString();
        if(doLog)
            if(doLog){Log.showbytes("AirGattCallback receiveNotes="+receiveNotes+" onCharacteristicChanged UUID: " + uuidstr, value);};

       if(!receiveNotes) {
             return;
             }
        switch(uuidstr) {
            case UUIDchar11: onChar11Changed(bluetoothGattCharacteristic, bluetoothGatt,  value);return;
            case UUIDchar21: onChar21Changed(bluetoothGattCharacteristic, bluetoothGatt,  value);return;
            case UUIDchar22: onChar22Changed( bluetoothGatt,  value);return;
            } 
    }

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
            bluetoothGatt.discoverServices();
        }
      else {
            Log.e(LOG_ID,"onMtuChanged failed");
            }
    }


    @Override
    public boolean matchDeviceName(String nameDevice, String address) {
        final String start="CSAir ";
        return nameDevice.startsWith(start)&& nameDevice.regionMatches(6,SerialNumber, SerialNumber.length()-4, 4);
        }

//    C1Q470A02339

    static private final UUID ScanServiceUUID = UUID.fromString("c4de9a20-5a9d-11e9-8647-d663bd873d93");
    @Override
    public UUID getService() {
        return ScanServiceUUID;
    }
@Override
public boolean pairingRequest() {
    Log.i(LOG_ID,"pairingRequest() before airGetPin");
    var pin=Natives.airGetPin(dataptr);
    if(pin!=null) {
         if(mActiveBluetoothDevice==null) {
                Log.e(LOG_ID,"pairingRequest mActiveBluetoothDevice==null");
                return false;
                }
        if(doLog)
             Log.showbytes(LOG_ID+" setPin",pin);
         mActiveBluetoothDevice.setPin(pin);
         return true;
         }
     return false;
    }

@Override
public void close() {
   super.close();
   }

@Override
public void bonded() {
        wrotepass[0] = System.currentTimeMillis();
        enableNotification(mBluetoothGatt, charact11);
        }
@Override
public void searchforDeviceAddress() {
    removedBond=false;
    super.searchforDeviceAddress();
    }

private static int crcver1(byte[] bArr,int len) {
    int total = 65535;
    for(int index=0;index<len;++index) {
        byte b2= bArr[index];
        int i2 = (((total << 8) | (total >>> 8)) & 65535) ^ (b2 & 255);
        int i10 = i2 ^ ((i2 & 255) >> 4);
        int i11 = i10 ^ ((i10 << 12) & 65535);
        total = i11 ^ (((i11 & 255) << 5) & 65535);
       }
    return total & 65535;
  }
private static int crcver2(byte[] bArr,int len) {
        int total = 65535;
        for(int index=0;index<len;++index) {
            byte b2= bArr[index];
            total ^= (b2 << 8) & 65535;
            for (int i2 = 0; i2 < 8; i2++) {
                int i10 = 32768 & total;
                int i11 = total << 1;
                if (i10 != 0) {
                    i11 ^= 4129;
                }
                total = i11 & 65535;
            }
        }
        return total;
    }

    @Override
    public void free() {
        cancelalarm();
        unbond();
        super.free();
    }
}



