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
import android.app.Application;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothProfile;
import android.os.Build;
import android.os.PowerManager;

import java.util.UUID;

import androidx.annotation.Keep;
import androidx.annotation.RequiresApi;

import static android.bluetooth.BluetoothDevice.PHY_LE_1M;
import static android.bluetooth.BluetoothDevice.PHY_LE_2M;
import static android.bluetooth.BluetoothDevice.PHY_LE_CODED;
import static android.bluetooth.BluetoothGatt.CONNECTION_PRIORITY_BALANCED;
import static android.bluetooth.BluetoothGatt.CONNECTION_PRIORITY_HIGH;
import static android.bluetooth.BluetoothGatt.GATT_SUCCESS;
import static android.content.Context.POWER_SERVICE;
import static tk.glucodata.Applic.app;
import static tk.glucodata.Applic.isRelease;
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.Gen2.errorP1;
import static tk.glucodata.Gen2.v1check;
import static tk.glucodata.Natives.V1;
import static tk.glucodata.Natives.V2;
import static tk.glucodata.Natives.abbottinit;
import static tk.glucodata.Natives.getstreamingAuthenticationData;
import static tk.glucodata.Natives.hasalarmloss;
import static tk.glucodata.Natives.processTooth;

//s/\<log\([a-z]\)(/Log.\1(LOG_ID,/g

public class MyGattCallback extends SuperGattCallback {
//	private static final boolean autoconnect=true;
	private int conphase = 0;

	static private final String LOG_ID = "MyGattCallback";

	//    private static final    long  MIN_BACKOFF_MILLIS=10000L;
	//String SerialNumber;
//	public String mActiveDeviceAddress;
	public MyGattCallback(SensorBluetooth sensorbluetooth, String SerialNumber, long dataptr) {
		super(Natives.getsensorgen(dataptr));
		Log.d(LOG_ID, "MyGattCallback(..)");
		this.sensorbluetooth = sensorbluetooth;
		this.SerialNumber = SerialNumber;
		this.dataptr = dataptr;
		mActiveDeviceAddress = Natives.getDeviceAddress(dataptr);
		Log.i(LOG_ID, "new MyGattCallback " + SerialNumber + " " + ((mActiveDeviceAddress != null) ? mActiveDeviceAddress : "null"));
//		if(sensorgen==2) abbottinit();
	}

	//void savestate() { Natives.saveState(dataptr); }
//    void setdataptr(long ptr) { dataptr=ptr; }





	boolean pack1 = false, pack2 = false;
	final byte[] packet = new byte[46];

//static boolean backuprunning=false;

	/*@BinderThread
	static	void startbackup() {
			if(!backuprunning) {
				backuprunning=true;
				Natives.startbackup();
				}

			}
			*/

	@SuppressLint("MissingPermission")
	final void writeBLELogin() {
		conphase = 2;
		byte[] bArr = new byte[1];
		bArr[0] = 32;
		try {
			BLELogincharacteristic.setValue(bArr);
			mBluetoothGatt.writeCharacteristic(BLELogincharacteristic);
			Log.i(LOG_ID, "Sent read security challenge");
		} catch(Throwable e) {
			Log.stack(LOG_ID, "onDescriptorWrite", e);
			if (Build.VERSION.SDK_INT > 30 && !Applic.mayscan())
				Applic.Toaster("Turn on NEARBY DEVICES permission");
		}
	}
static void showCharacter(String label, BluetoothGattCharacteristic characteristic) {
	byte[] value=characteristic.getValue();
	Log.showbytes("onDescriptorWrite",value);
    }
@Override // android.bluetooth.BluetoothGattCallback
	public void onDescriptorWrite(BluetoothGatt bluetoothGatt, BluetoothGattDescriptor bluetoothGattDescriptor, int i) {
		super.onDescriptorWrite(bluetoothGatt, bluetoothGattDescriptor, i);
		BluetoothGattCharacteristic characteristic = bluetoothGattDescriptor.getCharacteristic();
		showCharacter("onDescriptorWrite", characteristic);
		if (sensorgen == 2 && conphase == 1) {
			writeBLELogin();
		}
	}

void reconnect() {
	var gatt=mBluetoothGatt;
	if(gatt!=null) {
		gatt.disconnect();
		gatt.close();
		mBluetoothGatt = null;
		}
	sensorbluetooth.connectToActiveDevice(this, 0);
	}	
	@SuppressLint("MissingPermission")
	@Override
	public void onConnectionStateChange(BluetoothGatt bluetoothGatt, int status, int newState) {
		long tim = System.currentTimeMillis();
		try {
			if (!tk.glucodata.Applic.isRelease) {
				String[] state = {"DISCONNECTED", "CONNECTING", "CONNECTED", "DISCONNECTING"};
				Log.i(LOG_ID, SerialNumber + " onConnectionStateChange, status:" + status + ", state: " + (newState < state.length ? state[newState] : newState));
				}
			if (newState == BluetoothProfile.STATE_CONNECTED) {
				readrssi=9999;
				mBluetoothGatt.readRemoteRssi();
				bluetoothGatt.discoverServices();
				constatchange[0] = tim;
				setpriority(bluetoothGatt);
			} else {
				if(newState == BluetoothProfile.STATE_DISCONNECTED) {
					if(status == 19) {
						if(justenablednotification) {
							Natives.resetbluetooth(dataptr);
							justenablednotification = false;
							}
						else {
							Log.i(LOG_ID,"!justenablednotification");
							}
					    }
					;
					if(!autoconnect) {
						bluetoothGatt.close();
						mBluetoothGatt = null;
						if(!stop) {
							stop=true;
							sensorbluetooth.connectToActiveDevice(this, 0);
							}
						}
					else {
						if(!stop) {
							bluetoothGatt.connect();
							}
						}
					conphase = 0;
				}
				constatstatus = status;
				constatchange[1] = tim;
			}
		} catch (Throwable e) {
			Log.stack(LOG_ID, "onConnectionStateChange", e);
			if (Build.VERSION.SDK_INT > 30 && !Applic.mayscan())
				Applic.Toaster("Turn on NEARBY DEVICES permission");
			constatstatus = status;
			constatchange[1] = tim;
			return;
		}
	}

	BluetoothGattCharacteristic BLELogincharacteristic;
	BluetoothGattCharacteristic CompositeRawDatacharacteristic;

	/*
	@Override // android.bluetooth.BluetoothGattCallback
        public void onServicesDiscovered(BluetoothGatt bluetoothGatt, int status) {
			try {
				BluetoothGattService service;
				BluetoothGattCharacteristic characteristic;
				Log.d(LOG_ID, SerialNumber + " onServicesDiscovered, status: " + status);
				if (status == GATT_SUCCESS && (service = bluetoothGatt.getService(sensorbluetooth.mADCCustomServiceUUID)) != null && (characteristic = service.getCharacteristic(mCharacteristicUUID_BLELogin)) != null) {
					long tim = System.currentTimeMillis();
					byte[] key = Natives.sensorUnlockKey(dataptr);
					if (key != null) {
						characteristic.setValue(key);
						Log.v(LOG_ID, SerialNumber + " writeCharacteristic passcode: " + new String(showhex.hexstr(key, 0, key.length)));
						//noinspection MissingPermission
						bluetoothGatt.writeCharacteristic(characteristic);
					} else {
						wrotepass[1] = tim;
						//testage(tim);
						handshake = 0;
					}
				}
			} catch (Throwable e) {
				Log.stack(LOG_ID, "onServicesDiscovered", e);
				if (Build.VERSION.SDK_INT > 30 && !Applic.mayscan())
					Applic.Toaster("Turn on NEARBY DEVICES permission");
				return;
			}
		}
*/


	private  boolean m2831x() {
		try {
			BluetoothGattService service = mBluetoothGatt.getService(sensorbluetooth.mADCCustomServiceUUID);
			if (service != null) {
				BLELogincharacteristic = service.getCharacteristic(mCharacteristicUUID_BLELogin);
				CompositeRawDatacharacteristic = service.getCharacteristic(mCharacteristicUUID_CompositeRawData);
				if (BLELogincharacteristic != null && CompositeRawDatacharacteristic != null) {
					if (sensorgen == 2) {
						Log.i(LOG_ID, "Using security generation 2");
						conphase = 1;
						enableNotification(BLELogincharacteristic);
						Log.i(LOG_ID, "Enabled Security notification");
						return true;
					}
					Log.i(LOG_ID, "Using security generation 1");

					long tim = System.currentTimeMillis();
					byte[] key = Natives.sensorUnlockKey(dataptr);
					if (key != null) {
						conphase = 5;
						BLELogincharacteristic.setValue(key);
						if(!tk.glucodata.Applic.isRelease) 
						{
							Log.v(LOG_ID, SerialNumber + " writeCharacteristic passcode: " + new String(showhex.hexstr(key, 0, key.length)));
							}
						//noinspection MissingPermission
						mBluetoothGatt.writeCharacteristic(BLELogincharacteristic);
						return true;
					} else {
						wrotepass[1] = tim;
						handshake ="No key";
						return false;
					}
				}
				else {
					Log.i(LOG_ID,BLELogincharacteristic==null?"BLELogincharacteristic==null ":""+ CompositeRawDatacharacteristic==null?"CompositeRawDatacharacteristic==null":"");
					return false;

					}
			}
			else {
				Log.i(LOG_ID,"mBluetoothGatt.getService(sensorbluetooth.mADCCustomServiceUUID)==null");
				return false;
				}
		} catch (Throwable e) {
			Log.stack(LOG_ID, "onServicesDiscovered", e);
			if (Build.VERSION.SDK_INT > 30 && !Applic.mayscan())
				Applic.Toaster("Turn on NEARBY DEVICES permission");
			return false;
		}
	}


	@Override // android.bluetooth.BluetoothGattCallback
	public void onServicesDiscovered(BluetoothGatt bluetoothGatt, int status) {
		Log.i(LOG_ID, "BLE onServicesDiscovered invoked, status: " + status);
		readrssi=9999;
		mBluetoothGatt.readRemoteRssi();
		if(status != GATT_SUCCESS||! m2831x()) {
			mBluetoothGatt.disconnect();
			}

	}


	/*
public void onCharacteristicWrite (BluetoothGatt gatt,
                BluetoothGattCharacteristic characteristic,
                int status)
Callback indicating the result of a characteristic write operation.

If this callback is invoked while a reliable write transaction is in progress, the value of the characteristic represents the value reported by the remote device. An application should compare this value to the desired value to be written. If the values don't match, the application must abort the reliable write transaction.

Parameters
gatt	BluetoothGatt: GATT client invoked BluetoothGatt#writeCharacteristic
characteristic	BluetoothGattCharacteristic: Characteristic that was written to the associated remote device.
status	int: The result of the write operation BluetoothGatt#GATT_SUCCESS if the operation succeeds.
*/
	boolean justenablednotification = false;

	@Override
	public void onCharacteristicWrite(BluetoothGatt bluetoothGatt, BluetoothGattCharacteristic bluetoothGattCharacteristic, int status) {
		if (sensorgen == 2)
			return;
		try {
			BluetoothGattService service;
			BluetoothGattCharacteristic characteristic;
			BluetoothGattDescriptor descriptor;
			//noinspection MissingPermission
			Log.d(LOG_ID, bluetoothGatt.getDevice().getName() + " onCharacteristicWrite, status:" + status);
			boolean success = true;
			long tim = System.currentTimeMillis();
			justenablednotification = true;
			if (status == GATT_SUCCESS &&
					(service = bluetoothGatt.getService(sensorbluetooth.mADCCustomServiceUUID)) != null &&
					(characteristic = service.getCharacteristic(mCharacteristicUUID_CompositeRawData)) != null &&
					//noinspection MissingPermission
					bluetoothGatt.setCharacteristicNotification(characteristic, true) &&
					(descriptor = characteristic.getDescriptor(mCharacteristicConfigDescriptor)) != null) {

				descriptor.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
				//noinspection MissingPermission
				success = bluetoothGatt.writeDescriptor(descriptor);

				//justenablednotification = true; //PREVIOUS pos

			} else success = false;
			if (!success) {
				wrotepass[1] = tim;
				handshake = "Enabling notification failed";
				Log.i(LOG_ID, SerialNumber + " onCharacteristicWrite:  enabling notification failed");
				bluetoothGatt.disconnect();
				return;
			}

			conphase = 4;
			wrotepass[0] = tim;
			pack1 = false;
			pack2 = false;
		} catch (Throwable e) {
			Log.stack(LOG_ID, "onCharacteristicWrite", e);
			if (Build.VERSION.SDK_INT > 30 && !Applic.mayscan())
				Applic.Toaster("Turn on NEARBY DEVICES permission");
		}
	}











private static PowerManager.WakeLock getwakelock() {
		return ((PowerManager) app.getSystemService(POWER_SERVICE)).newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "Juggluco::processTooth");
		}
	/*
private static PowerManager.WakeLock wakeLock=null; 
private static PowerManager.WakeLock getwakelock() {
		if(wakeLock==null) {
			PowerManager powerManager= (PowerManager) app.getSystemService(POWER_SERVICE);
			wakeLock = powerManager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "Juggluco::processTooth");
			}
		return wakeLock;
		}
		*/


private	void oldonCharacteristicChanged(byte[] value) {
		justenablednotification = false;
		int length = value.length;
		Log.d(LOG_ID, SerialNumber + " oldonCharacteristicChanged " + length);
		switch (length) {
			case 20: {
				System.arraycopy(value, 0, packet, 0, 20);
				pack1 = true;
				pack2 = false;
			}
			;
			break;
			case 18: {
				if (pack1) {
					System.arraycopy(value, 0, packet, 20, 18);
					pack2 = true;
				}
			}
			;
			break;
			case 8: {
				if (pack1 && pack2) {
					final var wakeLock=Natives.hasRootcheck()?getwakelock():null;
					if(wakeLock!=null)
						wakeLock.acquire();

					pack1 = false;
					pack2 = false;
					System.arraycopy(value, 0, packet, 38, 8);
					final long timmsec = System.currentTimeMillis();
					final var newpacket= sensorgen==2?V2(773, tovalue, packet, null):packet;
					if(newpacket!=null) {
						long res = processTooth(dataptr, newpacket);
						if(res!=1L) {
							handleGlucoseResult(res,timmsec);
							}
						}
					if(wakeLock!=null)
						wakeLock.release();
				}
			}
			;
			break;
			default: {
				Log.i(LOG_ID, SerialNumber + " onCharacteristicChanged: wrong length=" + value.length);
			}
			;
		}
		;
	}

	int contactint = 0;// this.f8268nSa = 0;

	private int authlastv1v2(int i, byte[] bArr, byte[] bArr2, byte[] bArr3) {
		byte[] bArr4 = new byte[2];
		int P1res = V1(28960, 0, bArr, null);
		if (P1res < 0) {
			return P1res;
		}
		bArr4[0] = 1;
		bArr4[1] = (byte) i;
		byte[] P2res = V2(6505, P1res, bArr4, bArr2);
		if (P2res == null) {
			errorP1(P1res);
			return -99;
		}
		System.arraycopy(P2res, 0, bArr3, 0, P2res.length);
		return P1res;
	}

	private byte[] authenticateStream(byte[] bArr) {
		int i = contactint;
		if (i > 0) {
			V1(37400, i, null, null);
			contactint = 0;
		}
		byte[] Ew = Natives.getsensorident(dataptr); //uid sensorident
		if ( Ew == null) { 
			handshake= "No ident";
			wrotepass[1] = System.currentTimeMillis();
			return null;
			}

		if(!tk.glucodata.Applic.isRelease) {
			Log.i(LOG_ID, "sensdorident: " + showhex.showbytes(Ew));
			}
//  streamingAuthenticationData
		byte[] Cw = getstreamingAuthenticationData(dataptr);
		if(Cw == null ) { 
			handshake= "No Auth";
			wrotepass[1] = System.currentTimeMillis();
			return null;
			}
		if(!tk.glucodata.Applic.isRelease) {
			Log.i(LOG_ID, "Authenticationdata: " + showhex.showbytes(Cw));
			}
		byte[] bArr2 = new byte[Cw.length + bArr.length];
		System.arraycopy(Cw, 0, bArr2, 0, Cw.length);
		System.arraycopy(bArr, 0, bArr2, Cw.length, bArr.length);
		byte[] bArr3 = new byte[19];
		contactint = authlastv1v2(31, Ew, bArr2, bArr3);
		if (contactint > 0)
			return bArr3;
		handshake= "Auth >0";
		wrotepass[1] = System.currentTimeMillis();
		return null;
	}


	//mCharacteristicUUID_BLELogin.toString().equals(bluetoothGattCharacteristic.getUuid().toString())
	byte[] buf25 = new byte[25];// this.f14472dGb = new byte[25];
	public final void phase2(byte[] value) {
		if (value.length != 14) {
			Log.e(LOG_ID, "phase2 wrong " + value);
			return;
		}
		try {
			byte[] a = authenticateStream(value);
			conphase = 3;
//            this.f14472dGb = new byte[25];
			BLELogincharacteristic.setValue(a);
			mBluetoothGatt.writeCharacteristic(BLELogincharacteristic);
			justenablednotification = true;
		} catch (Exception e) {
			handshake= "streamingUnlock failed";
			Log.e(LOG_ID,handshake);
			wrotepass[1] = System.currentTimeMillis();
		}
	}

	private void contactintZero(int i) {
		V1(37400, i, null, null);
		contactint = 0;
	}

	int tovalue;

static	public final boolean enableNotification(BluetoothGatt mBluetoothGatt,BluetoothGattCharacteristic bluetoothGattCharacteristic) {
	mBluetoothGatt.setCharacteristicNotification(bluetoothGattCharacteristic, true);
	BluetoothGattDescriptor descriptor = bluetoothGattCharacteristic.getDescriptor(mCharacteristicConfigDescriptor);
	descriptor.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
	return mBluetoothGatt.writeDescriptor(descriptor);
	}
private final boolean enableNotification(BluetoothGattCharacteristic bluetoothGattCharacteristic) {
	return enableNotification(mBluetoothGatt, bluetoothGattCharacteristic);
	}

	public final void phase3(byte[] value) {
		int i;
		if (value.length == 7) {
			System.arraycopy(value, 0, buf25, 0, value.length);
		} else if (value.length == 18) {
			System.arraycopy(value, 0, buf25, 7, value.length);
			byte[] bArr = buf25;
			if (v1check(contactint, 1, bArr) != 0) {
				contactintZero(contactint);
				i = 0;
			} else
				i = contactint;
			tovalue = i;
			if (i <= 0) {
				handshake= "phase3 i<=0";
				wrotepass[1] = System.currentTimeMillis();
				Log.e(LOG_ID, "Error creating session context");
				return;
			}
			Log.i(LOG_ID, "Gen2 session " + i);
			conphase = 4;
			if (!enableNotification(CompositeRawDatacharacteristic)) {
				handshake= "Enable failed";
				wrotepass[1] = System.currentTimeMillis();
				conphase = 0;
				mBluetoothGatt.disconnect();
			}
			else  {
				wrotepass[0] = System.currentTimeMillis();
				}

		} else {
			handshake= "phase3 wrong length";
			wrotepass[1] = System.currentTimeMillis();
			Log.e(LOG_ID, "Erroneous response length " + value.length);
		}
	}
	/*
	android13:
public void onCharacteristicChanged (BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, byte[] value) {
	} */


/*
static void logcharist(BluetoothGattCharacteristic bluetoothGattCharacteristic)   {
	byte[] value = bluetoothGattCharacteristic.getValue();
	var uuid=bluetoothGattCharacteristic.getUuid();
	var uuidstr=uuid.toString();
	Log.showbyte(uuidstr,value);
	}
	*/
@Override // android.bluetooth.BluetoothGattCallback
public void onCharacteristicChanged(BluetoothGatt bluetoothGatt, BluetoothGattCharacteristic bluetoothGattCharacteristic) {
	byte[] value = bluetoothGattCharacteristic.getValue();
	var uuid=bluetoothGattCharacteristic.getUuid();
	var uuidstr=uuid.toString();
	if(sensorgen != 2) {
		if (!uuid.equals(mCharacteristicUUID_CompositeRawData)) {
			if(!tk.glucodata.Applic.isRelease) {
				Log.i(LOG_ID, SerialNumber + " onCharacteristicChanged: wrong UUID:" + uuidstr);
				}
			return;
			}
		oldonCharacteristicChanged(value);
		return;
		}

	boolean str=uuidstr.equals(mCharacteristicUUID_CompositeRawData.toString());
	boolean withoutstr=uuid.equals(mCharacteristicUUID_CompositeRawData);
	if(!tk.glucodata.Applic.isRelease) {
		Log.i(LOG_ID,"UUID: with str="+str+" without="+withoutstr);
		Log.i(LOG_ID, "onCharacteristicChanged " + uuidstr);
	     }
	if (!mCharacteristicUUID_BLELogin.equals(uuid)) {
		if (uuid.equals(mCharacteristicUUID_CompositeRawData) && conphase == 4) {
			oldonCharacteristicChanged(value);
		}
		} 
	else if (conphase == 2) {
		phase2(value); } 
	else if (conphase == 3) {
		phase3(value);
		}
	}
@Override
public void onReadRemoteRssi(BluetoothGatt gatt, int rssi, int status)  {
	Log.i(LOG_ID,"onReadRemoteRssi(BluetoothGatt,"+ rssi+","+status+(status==GATT_SUCCESS?" SUCCESS":" FAILURE"));
	if(status==GATT_SUCCESS) {
		readrssi=rssi;
		}
	}
	/*
private boolean askedphy=false;
@Override
	public void onPhyRead(BluetoothGatt gatt, int txPhy, int rxPhy, int status) {
		if(status==GATT_SUCCESS) {
				Log.i(LOG_ID,"onPhyRead(,"+rxPhy+","+rxPhy+","+status+") success");
				askedphy=true;
				}
		else
			Log.i(LOG_ID,"onPhyRead(,"+rxPhy+","+rxPhy+","+status+") failed");
	}
	*/
}

