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

import static android.bluetooth.BluetoothGatt.GATT_SUCCESS;
import static tk.glucodata.Log.doLog;

import android.annotation.SuppressLint;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothProfile;


import java.io.ByteArrayOutputStream;
import java.util.Arrays;
import java.util.UUID;


public class SiGattCallback extends SuperGattCallback {

    static private final String LOG_ID = "SiGattCallback";

    public SiGattCallback(String SerialNumber, long dataptr) {
        super(SerialNumber, dataptr, 0x10);
        Log.d(LOG_ID, "SiGattCallback(..)");
    }


    static void showCharacter(String label, BluetoothGattCharacteristic characteristic) {
        byte[] value = characteristic.getValue();
        Log.showbytes(label + " UUID: " + characteristic.getUuid().toString(), value);
    }


    @SuppressLint("MissingPermission")
    @Override // android.bluetooth.BluetoothGattCallback
    public void onDescriptorWrite(BluetoothGatt bluetoothGatt, BluetoothGattDescriptor bluetoothGattDescriptor, int status) {
        super.onDescriptorWrite(bluetoothGatt, bluetoothGattDescriptor, status);
		long tim = System.currentTimeMillis();
        if (doLog) {
            BluetoothGattCharacteristic characteristic = bluetoothGattDescriptor.getCharacteristic();
            byte[] value = bluetoothGattDescriptor.getValue();
            Log.showbytes("onDescriptorWrite char: " + characteristic.getUuid().toString() + " desc: " + bluetoothGattDescriptor.getUuid().toString()+" status="+status, value);
        }
        if (status == BluetoothGatt.GATT_SUCCESS) {
            final byte[] data=Natives.getSiWriteCharacter(dataptr);
            if(data==null) {
               var mess="getSiWriteCharacter==null";
                handshake=mess;
               wrotepass[1] = tim;
               Log.e(LOG_ID,mess);
			       disconnect();
               return;
                } 
            service2.setValue(data);
//            service2.setWriteType(2);
            bluetoothGatt.writeCharacteristic(service2);
			   wrotepass[0] = tim;

        }
       else {
         var mess="onDescriptorWrite failed";
         handshake=mess;
			   wrotepass[1] = tim;
            Log.e(LOG_ID,mess);
			   disconnect();

         }
    }

    @SuppressLint("MissingPermission")
    @Override
    public void onConnectionStateChange(BluetoothGatt bluetoothGatt, int status, int newState) {
		if(stop) {
			Log.i(LOG_ID,"onConnectionStateChange stop==true");
			return;
			}
		long tim = System.currentTimeMillis();
        if (doLog) {
            final String[] state = {"DISCONNECTED", "CONNECTING", "CONNECTED", "DISCONNECTING"};
            Log.i(LOG_ID, SerialNumber + " onConnectionStateChange, status:" + status + ", state: " + (newState < state.length ? state[newState] : newState));
        }
        if (newState == BluetoothProfile.STATE_CONNECTED) {
				constatchange[0] = tim;
            if (!bluetoothGatt.discoverServices()) {
                Log.e(LOG_ID, "bluetoothGatt.discoverServices()  failed");
			      disconnect();
            }
        } else {
            if (newState == BluetoothProfile.STATE_DISCONNECTED) {
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
				constatstatus = status;
				constatchange[1] = tim;
        }
    }


    static private final UUID serviceUUID = UUID.fromString("0000ff30-0000-1000-8000-00805f9b34fb");
    static private final UUID service1UID = UUID.fromString("0000ff31-0000-1000-8000-00805f9b34fb");
    static private final UUID service2UID = UUID.fromString("0000ff32-0000-1000-8000-00805f9b34fb");
    private BluetoothGattCharacteristic service1, service2;

    private boolean discover(BluetoothGatt bluetoothGatt) {
       Log.i(LOG_ID,"discover");
           BluetoothGattService service = bluetoothGatt.getService(serviceUUID);
           if (service == null) {
               var mess="getService(serviceUUID)==null";
               Log.i(LOG_ID, mess);
            handshake = mess;
               wrotepass[1] = System.currentTimeMillis();
               return false;
           }
           service1 = service.getCharacteristic(service1UID);
           service2 = service.getCharacteristic(service2UID);
           if (service1 == null || service2 == null) {
               var mess=(service1 == null ? "service1==null " : "") + (service2 == null ? "service2==null" : "");
               Log.i(LOG_ID, mess);
               wrotepass[1] =  System.currentTimeMillis();
               return false;
           }


        return enablenotifications(bluetoothGatt);
    }

    private static final UUID descriptor = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb");

    private boolean enablenotifications(BluetoothGatt bluetoothGatt) {
      Log.i(LOG_ID,"enablenotifications");

        var des = service1.getDescriptor(descriptor);
        if (des == null) {
		      final		var tim = System.currentTimeMillis();
            var mes="service1.getDescriptor(descriptor)==null";
            Log.i(LOG_ID,mes);
				handshake = mes;
				wrotepass[1] = tim;
            return false;
        }
        if(bluetoothGatt.setCharacteristicNotification(service1, true)) {
          des.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
          BluetoothGattCharacteristic characteristic = des.getCharacteristic();
          int writeType = characteristic.getWriteType();
          characteristic.setWriteType(2);
          boolean writeDescriptor = bluetoothGatt.writeDescriptor(des);
          characteristic.setWriteType(writeType);
          if(!writeDescriptor) {
               var mess="writeDescriptor failed";
             Log.e(LOG_ID,mess);
               handshake = mess;
               wrotepass[1] = System.currentTimeMillis();
               return false;
               }
          Log.i(LOG_ID,"writeDescriptor="+writeDescriptor);
          }
        else {
         var mess="setCharacteristicNotification(service1,true) failed";
          Log.e(LOG_ID,mess);
            handshake = mess;
            wrotepass[1] = System.currentTimeMillis();
            return false;
         }
         Log.i(LOG_ID,"enablenotifications succeeded");
       return true;
        }
//private static final UUID l = UUID.fromString("00002A05-0000-1000-8000-00805f9b34fb");

	@Override // android.bluetooth.BluetoothGattCallback
	public void onServicesDiscovered(BluetoothGatt bluetoothGatt, int status) {
		Log.i(LOG_ID, "BLE onServicesDiscovered invoked, status: " + status);
		if(status != GATT_SUCCESS||!discover(bluetoothGatt)) {
			disconnect();
			}

		}


	@Override
	public void onCharacteristicWrite(BluetoothGatt bluetoothGatt, BluetoothGattCharacteristic bluetoothGattCharacteristic, int status) {
		Log.d(LOG_ID, bluetoothGatt.getDevice().getAddress() + " onCharacteristicWrite, status:" + status + " UUID:" + bluetoothGattCharacteristic.getUuid().toString());
	}


boolean novalue=false;
@Override // android.bluetooth.BluetoothGattCallback
public void onCharacteristicChanged(BluetoothGatt bluetoothGatt, BluetoothGattCharacteristic bluetoothGattCharacteristic) {
	byte[] value = bluetoothGattCharacteristic.getValue();
//	var uuid=bluetoothGattCharacteristic.getUuid();
//   processData(value,System.currentTimeMillis());
   long timmsec=System.currentTimeMillis();
  long res=Natives.SIprocessData(dataptr, value,timmsec);
  if(res==3L) {
      Log.e(LOG_ID,"3: disconnect");
      disconnect();
      return;
     } 
  if(res==2L) {
      novalue=true;
		Applic.app.getHandler().postDelayed( ()->   {
            if(novalue) {
               Log.e(LOG_ID,"2: postDelayed disconnect");
               disconnect();
               }},30*1000L);
      return;
      }
   novalue=false;
   if(res==1L) return;
      handleGlucoseResult(res,timmsec);
    }

@Override
public void onReadRemoteRssi(BluetoothGatt gatt, int rssi, int status)  {
	Log.i(LOG_ID,"onReadRemoteRssi(BluetoothGatt,"+ rssi+","+status+(status==GATT_SUCCESS?" SUCCESS":" FAILURE"));
	if(status==GATT_SUCCESS) {
		readrssi=rssi;
		}
	}

@Override
public boolean matchDeviceName(String deviceName) {
	final var len=deviceName.length();
//   final String bluetoothNum=Natives.getSiBluetoothNum(dataptr);
   final String bluetoothNum=SerialNumber;
	if(bluetoothNum.regionMatches(0,deviceName, len-4,4)) {
//      this.deviceName=deviceName;
      Natives.saveDeviceName(dataptr,deviceName);
      return true;
      }
     return false;
	}
}
