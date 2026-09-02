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
/*      Wed May 06 21:13:35 CEST 2026                                                */


package tk.glucodata;

import static android.bluetooth.BluetoothGatt.GATT_SUCCESS;
import static android.bluetooth.BluetoothStatusCodes.SUCCESS;
import static tk.glucodata.Libre2GattCallback.showCharacter;
import static tk.glucodata.Log.doLog;

import android.annotation.SuppressLint;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothProfile;
import android.os.Build;
import android.os.ParcelUuid;


import androidx.annotation.NonNull;

import java.nio.charset.StandardCharsets;
import java.util.ArrayDeque;
import java.util.Deque;
import java.util.UUID;

import static tk.glucodata.Natives.gs3Glucose;

public class Si3GattCallback extends SuperGattCallback {

    private boolean opInFlight = false;
    private final Deque<Runnable> opQueue = new ArrayDeque<>();
    public static final class DeviceInfo {
        public int    batteryPercent;
        public String manufacturer;
        public String modelNumber;
        public byte[] systemId;
        public String firmwareRevision;
        public String hardwareRevision;
        public String softwareRevision;
        public String serialVendor;        // 0x2ABE
    }
    private final DeviceInfo info = new DeviceInfo();



    private static final ParcelUuid SVC_DATA_UUID =
            ParcelUuid.fromString("00005347-0000-1000-8000-00805f9b34fb");

    private static final UUID DEVICE_INFO_SERVICE =
            UUID.fromString("0000180a-0000-1000-8000-00805f9b34fb");
    private static final UUID BATTERY_SERVICE =
            UUID.fromString("0000180f-0000-1000-8000-00805f9b34fb");
    private static final UUID CUSTOM_SERVICE_FF30 =
            UUID.fromString("0000ff30-0000-1000-8000-00805f9b34fb");

    private static final UUID CHAR_BATTERY_LEVEL =
            UUID.fromString("00002a19-0000-1000-8000-00805f9b34fb");
    private static final UUID CHAR_MODEL_NUMBER =
            UUID.fromString("00002a24-0000-1000-8000-00805f9b34fb");
    private static final UUID CHAR_SYSTEM_ID =
            UUID.fromString("00002a25-0000-1000-8000-00805f9b34fb");
    private static final UUID CHAR_FW_REVISION =
            UUID.fromString("00002a26-0000-1000-8000-00805f9b34fb");
    private static final UUID CHAR_HW_REVISION =
            UUID.fromString("00002a27-0000-1000-8000-00805f9b34fb");
    private static final UUID CHAR_SW_REVISION =
            UUID.fromString("00002a28-0000-1000-8000-00805f9b34fb");
    private static final UUID CHAR_MANUFACTURER =
            UUID.fromString("00002a29-0000-1000-8000-00805f9b34fb");
    private static final UUID CHAR_SERIAL_NUMBER_VENDOR =
            UUID.fromString("00002abe-0000-1000-8000-00805f9b34fb");

    /** Notify-only characteristic the peer pushes responses on. */
    private static final UUID CHAR_FF31_NOTIFY =
            UUID.fromString("0000ff31-0000-1000-8000-00805f9b34fb");
    /** Write / write-without-response characteristic the phone sends commands on. */
    private static final UUID CHAR_FF32_WRITE  =
            UUID.fromString("0000ff32-0000-1000-8000-00805f9b34fb");


    static private final String LOG_ID = "Si3GattCallback";
    private BluetoothGattCharacteristic ff32Write;

    public Si3GattCallback(String SerialNumber, long dataptr) {
       super(SerialNumber, dataptr, 0x15);
       {if(doLog) {Log.d(LOG_ID, SerialNumber+" Si3GattCallback(..)");};};
    }




  boolean connected=false;

    private static final int MTU_REQUEST = 247;          // matches log line 14

    private interface ReadHandler { void accept(byte[] value); }
    private ReadHandler pendingRead;

    @SuppressLint("MissingPermission")
    private void queueRead(BluetoothGattCharacteristic ch, ReadHandler h) {
        if (ch == null) return;
        enqueue(() -> {
            pendingRead = h;
            if(!mBluetoothGatt.readCharacteristic(ch)) { Log.w(LOG_ID, "readCharacteristic returned false");
            finishOp();
            }
        });
    }

    private void handleRead(BluetoothGattCharacteristic ch, byte[] value, int status) {
        Log.i(LOG_ID, "read " + ch.getUuid() + " status=" + status + " bytes=" + hex(value));
        ReadHandler h = pendingRead;
        pendingRead = null;
        if (h != null && status == BluetoothGatt.GATT_SUCCESS && value != null) 
            h.accept(value);
        finishOp();
        }

    @SuppressLint("MissingPermission")
    private void queueEnableNotify(BluetoothGattCharacteristic ch) {
        if (ch == null) return;
        enqueue(() -> {
                    if(!enableNotification(mBluetoothGatt,ch))
                        finishOp();
                });
        }

    private void deliverNotification(@NonNull UUID uuid, byte[] value) {
        if(value == null)  {
            Log.i(LOG_ID,"onCharacteristicChanged value==null");
            disconnect();
            return;
            }
        Log.showbytes(LOG_ID+ " notify " + uuid ,value);
        /*
        if (uuid.equals(CHAR_BATTERY_LEVEL)) {
            var batteryPercent = value[0] & 0xFF;
            info.batteryPercent= batteryPercent;
            Log.i(LOG_ID,"battery="+batteryPercent);
            return;
            } */
	long tim = System.currentTimeMillis();
     GS3Data uit=gs3Glucose(dataptr,value,tim);
     if(uit.message!=null) {
            handshake=uit.message;
            }
     if(uit.cmd!=null) {
            write2(uit.cmd);
            }
     long res=uit.result;
     if(res<4L) {
        Applic.app.redraw();
            switch((int)(res&0xFFFFFL)) {
                case 2: wrotepass[1] = tim;
                    if(uit.cmd==null)
                        disconnect();
                    return;
                case 1: return;
                case 3: wrotepass[0] = tim;
                        return;
                };
           }
     handleGlucoseResult(res,uit.msecs);
    }


    private void enqueue(@NonNull Runnable op) {
        synchronized (opQueue) {
            opQueue.addLast(op);
            if (!opInFlight) drain();
        }
    }
    private void finishOp() {
        synchronized (opQueue) { 
            opInFlight = false; 
            drain(); 
            }
    }
    private void drain() {
        synchronized (opQueue) {
            if (opInFlight) return;
            Runnable next = opQueue.pollFirst();
            if (next == null) return;
            opInFlight = true;
            Applic.getHandler().post(next);
        }
    }


    private static String utf8(byte[] v) {
        return v == null ? "" : new String(v, StandardCharsets.UTF_8).replace("\u0000", "");
    }
    private static String hex(byte[] v) {
        if (v == null) return "null";
        StringBuilder sb = new StringBuilder(v.length * 2);
        for (byte b : v) sb.append(String.format("%02x", b));
        return sb.toString();
    }
    private void fail(String msg) {
        Log.e(LOG_ID, msg);
    }
private boolean getinfo=doLog;
    @SuppressLint("MissingPermission")
    @Override public void onMtuChanged(BluetoothGatt g, int mtu, int status) {
        Log.i(LOG_ID, "onMtuChanged mtu=" + mtu + " status=" + status);
        g.discoverServices();
       }

        @SuppressLint("MissingPermission")
        @Override public void onServicesDiscovered(BluetoothGatt g, int status) {
            if (status != BluetoothGatt.GATT_SUCCESS) { fail("discoverServices " + status); return; }
            Log.i(LOG_ID, "services discovered");

            BluetoothGattService cust = g.getService(CUSTOM_SERVICE_FF30);
            if(cust==null) {
                Log.e(LOG_ID,"No  FF30 service,finish");
                return;
                }
            ff32Write = cust.getCharacteristic(CHAR_FF32_WRITE);
            BluetoothGattCharacteristic ff31 = cust.getCharacteristic(CHAR_FF31_NOTIFY);

            if(getinfo) {
                BluetoothGattService dis  = g.getService(DEVICE_INFO_SERVICE);
                if(dis!=null) {
                    queueRead(dis.getCharacteristic(CHAR_MANUFACTURER), v -> {
                        info.manufacturer    = utf8(v);
                        if(info.manufacturer.endsWith("CN")) 
                            Natives.isChinese(dataptr); 
                        });
                    queueRead(dis.getCharacteristic(CHAR_MODEL_NUMBER), v -> info.modelNumber     = utf8(v));
                    queueRead(dis.getCharacteristic(CHAR_SYSTEM_ID),    v -> info.systemId        = v);
                    queueRead(dis.getCharacteristic(CHAR_FW_REVISION),  v -> info.firmwareRevision= utf8(v));
                    queueRead(dis.getCharacteristic(CHAR_HW_REVISION),  v -> info.hardwareRevision= utf8(v));
                    queueRead(dis.getCharacteristic(CHAR_SW_REVISION),  v -> info.softwareRevision= utf8(v));
                    queueRead(dis.getCharacteristic(CHAR_SERIAL_NUMBER_VENDOR), v -> info.serialVendor    = utf8(v));
                    }
                 else {
                    Log.e(LOG_ID,"no DEVICE_INFO_SERVICE");
                    }
                BluetoothGattService bat  = g.getService(BATTERY_SERVICE);
                if(bat!=null) {
                    BluetoothGattCharacteristic battery = bat.getCharacteristic(CHAR_BATTERY_LEVEL);
                    queueRead(battery,                                 v -> info.batteryPercent   = (v.length > 0) ? (v[0] & 0xFF) : -1);
                   //    queueEnableNotify(battery);
                    }
                 else {
                    Log.e(LOG_ID,"no BATTERY_SERVICE");
                    }
                }

            queueEnableNotify(ff31);

            enqueue(() -> {
                    if(getinfo) {
                            Log.i(LOG_ID, "battery="    + info.batteryPercent + "%");
                            Log.i(LOG_ID, "manufacturer="      + info.manufacturer);
                            Log.i(LOG_ID, "model="      + info.modelNumber);
                            Log.i(LOG_ID, "system id="      + hex(info.systemId));
                            Log.i(LOG_ID, "firmware revision="         + info.firmwareRevision);
                            Log.i(LOG_ID, "hardware revision="         + info.hardwareRevision);
                            Log.i(LOG_ID, "software revision="         + info.softwareRevision);
                            Log.i(LOG_ID, "serialVendor(2abe)=" + info.serialVendor);
                            getinfo=false;
                            }
                Applic.getHandler().post(() -> {
                  final var bytes=Natives.siAuthBytes(dataptr);
                  if(bytes!=null)
                      write2(bytes);
                });
                finishOp();
            });

               handshake = "Services Discovered";
        }

        @Override public void onCharacteristicRead(@NonNull BluetoothGatt g, @NonNull BluetoothGattCharacteristic ch,
                                                   @NonNull byte[] value, int status) {
            handleRead(ch, value, status);
        }
        @SuppressWarnings("deprecation")
        @Override public void onCharacteristicRead(BluetoothGatt g,
                                                   BluetoothGattCharacteristic ch, int status) {
            handleRead(ch, ch.getValue(), status);
        }

        @Override public void onDescriptorWrite(BluetoothGatt g,
                                                BluetoothGattDescriptor d, int status) {
            Log.i(LOG_ID, "descriptor write " + d.getUuid() + " status=" + status);
            finishOp();
        }

	@Override
	public void onCharacteristicWrite(BluetoothGatt bluetoothGatt, BluetoothGattCharacteristic bluetoothGattCharacteristic, int status) {
       if(doLog)
          showCharacter(LOG_ID+" onCharacteristicWrite " + bluetoothGatt.getDevice().getAddress() + " status:" + status + " ", bluetoothGattCharacteristic);


//        handshake = "wrote " +bluetoothGattCharacteristic.getValue().length;
        finishOp();
	    }

        @Override public void onCharacteristicChanged(@NonNull BluetoothGatt g,
                                                      BluetoothGattCharacteristic ch,
                                                      @NonNull byte[] value) {
            deliverNotification(ch.getUuid(), value);
        }
        @SuppressWarnings("deprecation")
        @Override public void onCharacteristicChanged(BluetoothGatt g,
                                                      BluetoothGattCharacteristic ch) {
            deliverNotification(ch.getUuid(), ch.getValue());
        }


    @SuppressLint("MissingPermission")
    @Override
    public void onConnectionStateChange(BluetoothGatt bluetoothGatt, int status, int newState) {
        if(!acceptConnectionStateChange(bluetoothGatt,newState))
            return;
		if(stop) {
			{if(doLog) {Log.i(LOG_ID,"onConnectionStateChange stop==true");};};
			return;
			}
		long tim = System.currentTimeMillis();
        if (doLog) {
            final String[] state = {"DISCONNECTED", "CONNECTING", "CONNECTED", "DISCONNECTING"};
            {if(doLog) {Log.i(LOG_ID, SerialNumber + " onConnectionStateChange, status:" + status + ", state: " + (newState < state.length ? state[newState] : newState));};};
        }
        if(newState == BluetoothProfile.STATE_CONNECTED) {
			constatchange[0] = tim;
            bluetoothGatt.requestMtu(MTU_REQUEST);
            connected=true;
            Natives.EverSenseClear(dataptr);
        } else {
            connected=false;
            if (newState == BluetoothProfile.STATE_DISCONNECTED) {
		   if(!autoconnect) {
			   if(!closeCurrentGatt(bluetoothGatt))
			       return;
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
				   if(!closeCurrentGatt(bluetoothGatt))
				       return;
				   }
			   }
	 	}
	   setConStatus(status);
	   constatchange[1] = tim;
	   }
    }




@SuppressLint("MissingPermission")

private boolean write2(byte[] bytes) {
   if(ff32Write==null) {
      disconnect();
      return false;
      }
        
   ff32Write.setValue(bytes);
    var blue=mBluetoothGatt;
    if(blue!=null)
       return blue.writeCharacteristic(ff32Write);
    else disconnect();
    return false;
   }





@Override
public void onReadRemoteRssi(BluetoothGatt gatt, int rssi, int status)  {
	{if(doLog) {Log.i(LOG_ID,"onReadRemoteRssi(BluetoothGatt,"+ rssi+","+status+(status==GATT_SUCCESS?" SUCCESS":" FAILURE"));};};
	if(status==GATT_SUCCESS) {
		readrssi=rssi;
		}
	}
    //Serial: 25090118AAFZBJ64  
// deviceName=AAC25B18AAFZ

@Override
public boolean matchDeviceName(String deviceName,String address) {
	if(deviceName==null)
		return false;
	final var devlen=deviceName.length();
    final var serlen=SerialNumber.length();
    final int matchlen=6;
	if(SerialNumber.regionMatches(serlen-matchlen,deviceName, devlen-matchlen,matchlen)) {
	      Natives.siSaveDeviceName(dataptr,deviceName);
	      return true;
	      }
     return false;
	}

@Override
public void free() {
	super.free();
	}


private int tries=1;
@Override
public UUID getService() {
   if(tries++%2==1)
       return  CUSTOM_SERVICE_FF30;
   return null;
   }
}
