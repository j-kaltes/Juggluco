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
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.Gen2.errorP1;
import static tk.glucodata.Gen2.v1check;
import static tk.glucodata.Log.doLog;
import static tk.glucodata.Natives.V1;
import static tk.glucodata.Natives.V2;
import static tk.glucodata.Natives.abbottinit;
import static tk.glucodata.Natives.getstreamingAuthenticationData;
import static tk.glucodata.Natives.hasalarmloss;
import static tk.glucodata.Natives.processTooth;


public class MyGattCallback extends SuperGattCallback {
	private int conphase = 0;

	static private final String LOG_ID = "MyGattCallback";

	public MyGattCallback(String SerialNumber, long dataptr) {
		super(Natives.getsensorgen(dataptr));
		Log.d(LOG_ID, "MyGattCallback(..)");
		this.SerialNumber = SerialNumber;
		this.dataptr = dataptr;
		mActiveDeviceAddress = Natives.getDeviceAddress(dataptr);
		Log.i(LOG_ID, "new MyGattCallback " + SerialNumber + " " + ((mActiveDeviceAddress != null) ? mActiveDeviceAddress : "null"));
		
	}


	boolean pack1 = false, pack2 = false;
	final byte[] packet = new byte[46];


	@SuppressLint("MissingPermission")
	final void writeBLELogin() {
		conphase = 2;
		byte[] bArr = new byte[1];
		bArr[0] = 32;
		try {
			Log.i(LOG_ID, "Sent read security challenge");
			if(!BLELogincharacteristic.setValue(bArr)) {
				var mess="BLELogincharacteristic.setValue";
				wrotepass[1] = System.currentTimeMillis();
				handshake = mess;
				Log.e(LOG_ID,mess);
				var gatt= mBluetoothGatt;
				if(gatt!=null)
					gatt.disconnect();

				}
			if(!mBluetoothGatt.writeCharacteristic(BLELogincharacteristic)) {
				var mess="writeCharacteristic(BLELogincharacteristic)";
				Log.e(LOG_ID,mess);
				handshake = mess;
				wrotepass[1] = System.currentTimeMillis();
				var gatt= mBluetoothGatt;
				if(gatt!=null)
					gatt.disconnect();
				}
		} catch(Throwable e) {
			Log.stack(LOG_ID, "onDescriptorWrite", e);
			if (Build.VERSION.SDK_INT > 30 && !Applic.mayscan())
				Applic.Toaster(R.string.turn_on_nearby_devices_permission);
			var gatt= mBluetoothGatt;
			if(gatt!=null)
				gatt.disconnect();

		}
	}
static void showCharacter(String label, BluetoothGattCharacteristic characteristic) {
	byte[] value=characteristic.getValue();
        Log.showbytes(label + " UUID: " + characteristic.getUuid().toString(), value);
    }
@Override // android.bluetooth.BluetoothGattCallback
	public void onDescriptorWrite(BluetoothGatt bluetoothGatt, BluetoothGattDescriptor bluetoothGattDescriptor, int i) {
		super.onDescriptorWrite(bluetoothGatt, bluetoothGattDescriptor, i);
		BluetoothGattCharacteristic characteristic = bluetoothGattDescriptor.getCharacteristic();
		if(doLog)
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
	var sensorbluetooth=SensorBluetooth.blueone;
	if(sensorbluetooth!=null)
		sensorbluetooth.connectToActiveDevice(this, 0);
	}	
	@SuppressLint("MissingPermission")
	@Override
	public void onConnectionStateChange(BluetoothGatt bluetoothGatt, int status, int newState) {
		endBLEHandler();
		if(stop) {
			Log.i(LOG_ID,"onConnectionStateChange stop==true");
			return;
			}
		long tim = System.currentTimeMillis();
		try {
			if (doLog) {
				String[] state = {"DISCONNECTED", "CONNECTING", "CONNECTED", "DISCONNECTING"};
				Log.i(LOG_ID, SerialNumber + " onConnectionStateChange, status:" + status + ", state: " + (newState < state.length ? state[newState] : newState));
				}
			if (newState == BluetoothProfile.STATE_CONNECTED) {
//				readrssi=9999; mBluetoothGatt.readRemoteRssi();
				if(!bluetoothGatt.discoverServices()) {
					Log.e(LOG_ID,"bluetoothGatt.discoverServices()  failed");
					}
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
							var sensorbluetooth=SensorBluetooth.blueone;
							if(sensorbluetooth!=null)
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
				Applic.Toaster(R.string.turn_on_nearby_devices_permission);
			constatstatus = status;
			constatchange[1] = tim;
			if(bluetoothGatt!=null)
				bluetoothGatt.disconnect();
			return;
		}
	}

	BluetoothGattCharacteristic BLELogincharacteristic;
	BluetoothGattCharacteristic CompositeRawDatacharacteristic;


	private  boolean m2831x() {
		try {
			var sensorbluetooth=SensorBluetooth.blueone;
			if(sensorbluetooth==null)
				return false;
			BluetoothGattService service = mBluetoothGatt.getService(sensorbluetooth.mADCCustomServiceUUID);
			if (service != null) {
				BLELogincharacteristic = service.getCharacteristic(mCharacteristicUUID_BLELogin);
				CompositeRawDatacharacteristic = service.getCharacteristic(mCharacteristicUUID_CompositeRawData);
				if (BLELogincharacteristic != null && CompositeRawDatacharacteristic != null) {
					if (sensorgen == 2) {
						Log.i(LOG_ID, "Using security generation 2");
						conphase = 1;
						boolean isEnabled = enableNotification(BLELogincharacteristic);
						Log.i(LOG_ID, "Enabled Security notification: " + isEnabled);
						return true;
					}
					Log.i(LOG_ID, "Using security generation 1");

					long tim = System.currentTimeMillis();
					byte[] key = Natives.sensorUnlockKey(dataptr);
					if (key != null) {
						conphase = 5;
						if(BLELogincharacteristic.setValue(key)) {
							//noinspection MissingPermission
							if(mBluetoothGatt.writeCharacteristic(BLELogincharacteristic)) {
								if(doLog) {
									Log.v(LOG_ID, SerialNumber + " writeCharacteristic passcode: " + new String(showhex.hexstr(key, 0, key.length)));
									}
								return true;
								}
							else {
								Log.e(LOG_ID,SerialNumber + " writeCharacteristic failed passcode: " + new String(showhex.hexstr(key, 0, key.length)));
								wrotepass[1] = tim;
								handshake="writeCharacteristic";
								return false;
								}

							}
						else {
							wrotepass[1] = tim;
							handshake="setValue";
						Log.e(LOG_ID,SerialNumber + " setValue failed: passcode: " + new String(showhex.hexstr(key, 0, key.length)));
							return false;
							}
						} 
					else {
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
				Applic.Toaster(R.string.turn_on_nearby_devices_permission);
			return false;
		}
	}


	@Override // android.bluetooth.BluetoothGattCallback
	public void onServicesDiscovered(BluetoothGatt bluetoothGatt, int status) {
		Log.i(LOG_ID, "BLE onServicesDiscovered invoked, status: " + status);
//		readrssi=9999; mBluetoothGatt.readRemoteRssi();
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
		Log.d(LOG_ID, bluetoothGatt.getDevice().getAddress() + " onCharacteristicWrite, status:" + status + " UUID:" + bluetoothGattCharacteristic.getUuid().toString());
		if (sensorgen == 2)
			return;
		try {
			var sensorbluetooth=SensorBluetooth.blueone;
			if(sensorbluetooth==null) {
				Log.i(LOG_ID, "onCharacteristicWrite sensorbluetooth==null");
				return ;
				}
			BluetoothGattService service;
			BluetoothGattCharacteristic characteristic;
			BluetoothGattDescriptor descriptor;
			//noinspection MissingPermission
			boolean success = true;
			long tim = System.currentTimeMillis();
			justenablednotification = true;
			if (status == GATT_SUCCESS &&
					(service = bluetoothGatt.getService(sensorbluetooth.mADCCustomServiceUUID)) != null &&
					(characteristic = service.getCharacteristic(mCharacteristicUUID_CompositeRawData)) != null &&
					//noinspection MissingPermission
					bluetoothGatt.setCharacteristicNotification(characteristic, true) &&
					(descriptor = characteristic.getDescriptor(mCharacteristicConfigDescriptor)) != null) {

				if(!descriptor.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE)) {
					Log.e(LOG_ID,"descriptor.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE)  failed");
					}
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
				Applic.Toaster(R.string.turn_on_nearby_devices_permission);
			bluetoothGatt.disconnect();
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

	int contactint = 0;

	private int authlastv1v2(int i, byte[] bArr, byte[] bArr2, byte[] bArr3,int numb) {
		int P1res = V1(28960, numb, bArr, null);
		if (P1res < 0) {
			return P1res;
		}
		byte[] bArr4 = new byte[2];
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
		if(i > 0) {
			V1(37400, i, null, null);
			contactint = 0;
			}
		byte[] Ew = Natives.getsensorident(dataptr); //uid sensorident
		if(Ew == null) { 
			handshake= "No ident";
			wrotepass[1] = System.currentTimeMillis();
			var gatt = mBluetoothGatt;
			if (gatt != null)
				gatt.disconnect();
			return null;
			}

		if(doLog) {
			Log.i(LOG_ID, "sensdorident: " + showhex.showbytes(Ew));
			}
//  streamingAuthenticationData
		byte[] auth = getstreamingAuthenticationData(dataptr);
		if(auth == null ) { 
			handshake= "No Auth";
			wrotepass[1] = System.currentTimeMillis();
			var gatt = mBluetoothGatt;
			if (gatt != null)
				gatt.disconnect();
			return null;
			}
		if(doLog) {
			Log.i(LOG_ID, "Authenticationdata: " + showhex.showbytes(auth));
			}
		int numb;
		if(Gen2.newVersion) {
			if(auth.length == 12) {
			   numb = ((auth[11] << 8) | auth[10]) & 0xFFFF;
			   } 
			else {
			var mess="Authenticationdata: len="+auth.length;
			   if (auth.length < 10) {
				Log.e(LOG_ID,mess);
				handshake= mess;
				wrotepass[1] = System.currentTimeMillis();
				var gatt = mBluetoothGatt;
				if (gatt != null)
					gatt.disconnect();
				return null;
			   } else {
			   	Log.i(LOG_ID,mess);
			       numb = -1; //-1
				}
			    }
			   }
		else {
			numb=0;
			}

		byte[] bArr2 = new byte[bArr.length+10];
		System.arraycopy(auth, 0, bArr2, 0, 10);
		System.arraycopy(bArr, 0, bArr2, 10, bArr.length);
		byte[] bArr3 = new byte[19];
		contactint = authlastv1v2(31, Ew, bArr2, bArr3,numb);
		if(contactint > 0)
			return bArr3;
		handshake= "Auth <=0";
		wrotepass[1] = System.currentTimeMillis();
		var gatt = mBluetoothGatt;
		if (gatt != null)
			gatt.disconnect();
		return null;
	}


	//mCharacteristicUUID_BLELogin.toString().equals(bluetoothGattCharacteristic.getUuid().toString())
	byte[] buf25 = new byte[25];// this.f14472dGb = new byte[25];
	public final void phase2(byte[] value) {
		if (value.length != 14) {
			Log.e(LOG_ID, "phase2 wrong " + value);
			var gatt = mBluetoothGatt;
			if (gatt != null)
				gatt.disconnect();
			return;
		}
		try {
			byte[] a = authenticateStream(value);
			if(a==null)  {
                		Log.e(LOG_ID, "phase2 wrong (a == null)");
				var gatt = mBluetoothGatt;
				if (gatt != null)
					gatt.disconnect();
				return;
				}
//            this.f14472dGb = new byte[25];
		    mBLELoginHandler = () -> {
			BLELogincharacteristic.setValue(a);
			if (!mBluetoothGatt.writeCharacteristic(BLELogincharacteristic)) {
			    var mess = "phase2 retry=" + BLELoginposted + " writeCharacteristic(BLELogincharacteristic)";
			    Log.e(LOG_ID, mess);
			    handshake = mess;
			    wrotepass[1] = System.currentTimeMillis();
			    if (BLELoginposted < 5) {
				++BLELoginposted;
				Applic.app.getHandler().postDelayed(mBLELoginHandler, 100);
				return;
			    }
			    var gatt = mBluetoothGatt;
			    if (gatt != null)
				gatt.disconnect();
			    return;
			}
			conphase = 3;
			justenablednotification = true;
			BLELoginposted = 0;
			mBLELoginHandler = null;
		    };
		    mBLELoginHandler.run();
		} catch (Exception e) {
			handshake = "streamingUnlock failed";
			Log.e(LOG_ID, handshake);
			wrotepass[1] = System.currentTimeMillis();
			var gatt = mBluetoothGatt;
			if (gatt != null)
				gatt.disconnect();
		}
	}

	private void contactintZero(int i) {
		V1(37400, i, null, null);
		contactint = 0;
	}

	int tovalue;

static	public final boolean enableNotification(BluetoothGatt bluetoothGatt,BluetoothGattCharacteristic bluetoothGattCharacteristic) {
	bluetoothGatt.setCharacteristicNotification(bluetoothGattCharacteristic, true);
	BluetoothGattDescriptor descriptor = bluetoothGattCharacteristic.getDescriptor(mCharacteristicConfigDescriptor);
	descriptor.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
	return bluetoothGatt.writeDescriptor(descriptor);
	}
private final boolean enableNotification(BluetoothGattCharacteristic bluetoothGattCharacteristic) {
	return enableNotification(mBluetoothGatt, bluetoothGattCharacteristic);
	}

	public final void phase3(byte[] value) {
		int i;
		if (value.length == 7) {
			System.arraycopy(value, 0, buf25, 0, value.length);
		} 
	else {
		if (value.length == 18) {
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
				var gatt = mBluetoothGatt;
				if (gatt != null)
					gatt.disconnect();
				return;
				}
			Log.i(LOG_ID, "Gen2 session " + i);
			conphase = 4;

            mBLELoginHandler = () -> {
                if (!enableNotification(CompositeRawDatacharacteristic)) {
                    Log.e(LOG_ID, "phase3 retry=" + BLELoginposted + " enableNotification failed");
                    handshake = "Enable CompositeRawDatacharacteristic failed";
                    wrotepass[1] = System.currentTimeMillis();
                    if (BLELoginposted < 5) {
                        ++BLELoginposted;
                        Applic.app.getHandler().postDelayed(mBLELoginHandler, 100);
                        return;
                    }
                    conphase = 0;
                    var gatt = mBluetoothGatt;
                    if (gatt != null)
                        gatt.disconnect();
                    return;
                }
                BLELoginposted = 0;
                mBLELoginHandler = null;
                wrotepass[0] = System.currentTimeMillis();
            };
            mBLELoginHandler.run();

		} else {
			handshake= "phase3 wrong length";
			wrotepass[1] = System.currentTimeMillis();
			Log.e(LOG_ID, "Erroneous response length " + value.length);
			var gatt = mBluetoothGatt;
			if (gatt != null)
				gatt.disconnect();
		}
		}
	}

@Override // android.bluetooth.BluetoothGattCallback
public void onCharacteristicChanged(BluetoothGatt bluetoothGatt, BluetoothGattCharacteristic bluetoothGattCharacteristic) {
	byte[] value = bluetoothGattCharacteristic.getValue();
	var uuid=bluetoothGattCharacteristic.getUuid();
	var uuidstr=uuid.toString();
	if(sensorgen != 2) {
		if (!uuid.equals(mCharacteristicUUID_CompositeRawData)) {
			if(doLog) {
				Log.i(LOG_ID, SerialNumber + " onCharacteristicChanged: wrong UUID:" + uuidstr);
				}
			return;
			}
		oldonCharacteristicChanged(value);
		return;
		}

	boolean str=uuidstr.equals(mCharacteristicUUID_CompositeRawData.toString());
	boolean withoutstr=uuid.equals(mCharacteristicUUID_CompositeRawData);
	if(doLog) {
            	Log.i(LOG_ID, "UUID: with str=" + str + " without=" + withoutstr + " conphase=" + conphase);
		Log.i(LOG_ID, "onCharacteristicChanged " + uuidstr);
	     }
	if (!mCharacteristicUUID_BLELogin.equals(uuid)) {
		if (uuid.equals(mCharacteristicUUID_CompositeRawData) && conphase == 4) {
			oldonCharacteristicChanged(value);
		}
		}
	else {
		if(conphase == 2) {
			phase2(value);
			}
		else  {
			if(conphase == 3) {
				phase3(value);
				}
			}; 
		}
	}
private int BLELoginposted=0;
private Runnable		mBLELoginHandler=null; 

private void endBLEHandler() {
        BLELoginposted = 0;
        if (mBLELoginHandler != null) {
            Applic.app.getHandler().removeCallbacks(mBLELoginHandler);
            mBLELoginHandler = null;
        }
    }
@Override
	public void close() {
		Log.i(LOG_ID,"close");
		endBLEHandler();
		super.close();
		}
@Override
public void onReadRemoteRssi(BluetoothGatt gatt, int rssi, int status)  {
	Log.i(LOG_ID,"onReadRemoteRssi(BluetoothGatt,"+ rssi+","+status+(status==GATT_SUCCESS?" SUCCESS":" FAILURE"));
	if(status==GATT_SUCCESS) {
		readrssi=rssi;
		}
	}
}

