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
import android.app.PendingIntent;
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
import java.util.concurrent.TimeUnit;

import androidx.annotation.Keep;
import androidx.annotation.RequiresApi;

import static android.bluetooth.BluetoothDevice.PHY_LE_1M;
import static android.bluetooth.BluetoothDevice.PHY_LE_2M;
import static android.bluetooth.BluetoothDevice.PHY_LE_CODED;
import static android.bluetooth.BluetoothGatt.CONNECTION_PRIORITY_BALANCED;
import static android.bluetooth.BluetoothGatt.CONNECTION_PRIORITY_HIGH;
import static android.bluetooth.BluetoothGatt.GATT_SUCCESS;
import static android.content.Context.POWER_SERVICE;
import static java.util.Objects.nonNull;
import static tk.glucodata.Applic.app;
import static tk.glucodata.Applic.isWearable;
//import static tk.glucodata.DexGattCallback.setalarm;
import static tk.glucodata.Log.doLog;
import static tk.glucodata.LossOfSensorAlarm.cancelalarm;
import static tk.glucodata.Natives.abbottinit;
import static tk.glucodata.Natives.getstreamingAuthenticationData;
import static tk.glucodata.Natives.hasalarmloss;
import static tk.glucodata.Natives.processTooth;


public class Libre2GattCallback extends SuperGattCallback {
	private int conphase = 0;

       static private final UUID mADCCustomServiceUUID = UUID.fromString("0000fde3-0000-1000-8000-00805f9b34fb");
	static private final String LOG_ID = "Libre2GattCallback";

	public Libre2GattCallback(String SerialNumber, long dataptr) {
		super(SerialNumber,dataptr,Natives.getLibre2sensorgen(dataptr));
		{if(doLog) {Log.d(LOG_ID, "Libre2GattCallback(..)");};};
	}


	boolean pack1 = false, pack2 = false;
	final byte[] packet = new byte[46];


	@SuppressLint("MissingPermission")
	final void writeBLELogin() {
		conphase = 2;
		byte[] bArr = new byte[1];
		bArr[0] = 32;
		try {
			if(doLog) {Log.i(LOG_ID, "Sent read security challenge");};
			if(!BLELogincharacteristic.setValue(bArr)) {
				var mess="BLELogincharacteristic.setValue";
				wrotepass[1] = System.currentTimeMillis();
				handshake = mess;
				Log.e(LOG_ID,SerialNumber+" "+mess);
				var gatt= mBluetoothGatt;
				if(gatt!=null)
					gatt.disconnect();
				return;
				}
			if(!mBluetoothGatt.writeCharacteristic(BLELogincharacteristic)) {
				var mess="writeCharacteristic(BLELogincharacteristic)";
				Log.e(LOG_ID,SerialNumber+" "+mess);
				handshake = mess;
				wrotepass[1] = System.currentTimeMillis();
				var gatt= mBluetoothGatt;
				if(gatt!=null)
					gatt.disconnect();
				}
		} catch(Throwable e) {
			Log.stack(LOG_ID, SerialNumber+" onDescriptorWrite", e);
			if (Build.VERSION.SDK_INT > 30 && !Applic.mayscan())
				Applic.Toaster(R.string.turn_on_nearby_devices_permission);
			var gatt= mBluetoothGatt;
			if(gatt!=null)
				gatt.disconnect();

		}
	}
static void showCharacter(String label, BluetoothGattCharacteristic characteristic) {
	byte[] value=characteristic.getValue();
        if(doLog){Log.showbytes(label + " UUID: " + characteristic.getUuid().toString(), value);};
    }
@Override // android.bluetooth.BluetoothGattCallback
	public synchronized void onDescriptorWrite(BluetoothGatt bluetoothGatt, BluetoothGattDescriptor bluetoothGattDescriptor, int status) {
        if (bluetoothGatt != mBluetoothGatt || !canConnectSensor()) return;
		super.onDescriptorWrite(bluetoothGatt, bluetoothGattDescriptor, status);
		if(doLog)  {
		   BluetoothGattCharacteristic characteristic = bluetoothGattDescriptor.getCharacteristic();
			showCharacter(SerialNumber+" onDescriptorWrite status="+status, characteristic);
          }
      if(status!=GATT_SUCCESS ) {
				bluetoothGatt.disconnect();
            return;
         }
         
		if (sensorgen == 2 && conphase == 1) {
			writeBLELogin();
		}
	}
private boolean connected=false;
private PendingIntent onalarm=null;
	@SuppressLint("MissingPermission")
	@Override
	public synchronized void onConnectionStateChange(BluetoothGatt bluetoothGatt, int status, int newState) {
        if(!acceptConnectionStateChange(bluetoothGatt,newState))
            return;
		endBLEHandler();
		if(stop || !canConnectSensor()) {
			if(doLog) {Log.i(LOG_ID,"onConnectionStateChange stop==true");};
            close();
			return;
			}
		long tim = System.currentTimeMillis();
		try {
			if (doLog) {
				final String[] state = {"DISCONNECTED", "CONNECTING", "CONNECTED", "DISCONNECTING"};
				Log.i(LOG_ID, SerialNumber + " onConnectionStateChange, status:" + status + ", state: " + (newState < state.length ? state[newState] : newState));
				}
			if (newState == BluetoothProfile.STATE_CONNECTED) {
               connected=true;
				if(!bluetoothGatt.discoverServices()) {
					Log.e(LOG_ID,"bluetoothGatt.discoverServices()  failed");
					}
				constatchange[0] = tim;
				setpriority(bluetoothGatt);
			} else {
               connected=false;
				if(newState == BluetoothProfile.STATE_DISCONNECTED) {
					if(status == 19) {
						if(justenablednotification) {
							Natives.resetbluetooth(dataptr);
							justenablednotification = false;
							}
						else {
							{if(doLog) {Log.i(LOG_ID,"!justenablednotification");};};
							}
					    }
					;
					if(!autoconnect) {
						if(!closeCurrentGatt(bluetoothGatt))
						    return;
						if(!stop) {
							var sensorbluetooth=SensorBluetooth.blueone;
							if(sensorbluetooth!=null)  {
/*
                                if(isWearable&&Natives.getDisconnectSensor()) {
                                    final long alreadywaited = tim - datatime;
                                    final long mmsectimebetween = 60 * 1000;
                                    long stillwait = mmsectimebetween - alreadywaited - 30000;
                                    if(doLog) {Log.i(LOG_ID, "alreadywaited=" + alreadywaited + " stillwait=" + stillwait);};
                                    if(stillwait>0)
                                        onalarm=setalarm(tim+stillwait,onalarm,SerialNumber);
                                     else
                                        sensorbluetooth.connectToActiveDevice(this, 0);
                                    }
                               else */
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
					conphase = 0;
				}
				setConStatus(status);
				constatchange[1] = tim;
			}
		} catch (Throwable e) {
			Log.stack(LOG_ID, SerialNumber+" onConnectionStateChange", e);
			if (Build.VERSION.SDK_INT > 30 && !Applic.mayscan())
				Applic.Toaster(R.string.turn_on_nearby_devices_permission);
			setConStatus(status);
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
			BluetoothGattService service = mBluetoothGatt.getService(mADCCustomServiceUUID);
			if (service != null) {
				BLELogincharacteristic = service.getCharacteristic(mCharacteristicUUID_BLELogin);
				CompositeRawDatacharacteristic = service.getCharacteristic(mCharacteristicUUID_CompositeRawData);
				if (BLELogincharacteristic != null && CompositeRawDatacharacteristic != null) {
					if (sensorgen == 2) {
						{if(doLog) {Log.i(LOG_ID, "Using security generation 2");};};
						conphase = 1;
						boolean isEnabled = asknotification(BLELogincharacteristic);
						{if(doLog) {Log.i(LOG_ID, "Enabled Security notification: " + isEnabled);};};
						return true;
					}
					{if(doLog) {Log.i(LOG_ID, "Using security generation 1");};};

					long tim = System.currentTimeMillis();
					byte[] key = Natives.sensorUnlockKey(dataptr);
					if (key != null) {
						conphase = 5;
						if(BLELogincharacteristic.setValue(key)) {
							if(mBluetoothGatt.writeCharacteristic(BLELogincharacteristic)) {
								if(doLog) {
									{if(doLog) {Log.v(LOG_ID, SerialNumber + " writeCharacteristic passcode: " + new String(showhex.hexstr(key, 0, key.length)));};};
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
					{if(doLog) {Log.i(LOG_ID,BLELogincharacteristic==null?"BLELogincharacteristic==null ":""+ CompositeRawDatacharacteristic==null?"CompositeRawDatacharacteristic==null":"");};};
					return false;

					}
			}
			else {
				{if(doLog) {Log.i(LOG_ID,"mBluetoothGatt.getService(mADCCustomServiceUUID)==null");};};
				return false;
				}
		} catch (Throwable e) {
			Log.stack(LOG_ID, "onServicesDiscovered", e);
			if (Build.VERSION.SDK_INT > 30 && !Applic.mayscan())
				Applic.Toaster(R.string.turn_on_nearby_devices_permission);
			return false;
		}
	}


	@SuppressLint("MissingPermission")
    @Override // android.bluetooth.BluetoothGattCallback
	public synchronized void onServicesDiscovered(BluetoothGatt bluetoothGatt, int status) {
        if (bluetoothGatt != mBluetoothGatt || !canConnectSensor()) return;
		{if(doLog) {Log.i(LOG_ID, "BLE onServicesDiscovered invoked, status: " + status);};};
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
private	boolean justenablednotification = false;
 private   BluetoothGattCharacteristic characteristic;
private   boolean failedbefore=false;
	@Override
	public synchronized void onCharacteristicWrite(BluetoothGatt bluetoothGatt, BluetoothGattCharacteristic bluetoothGattCharacteristic, int status) {
        if (bluetoothGatt != mBluetoothGatt || !canConnectSensor()) return;
		if(doLog) {
             showCharacter(LOG_ID + " "+ bluetoothGatt.getDevice().getAddress() + " onCharacteristicWrite, status:" + status ,bluetoothGattCharacteristic);
            };
		if (sensorgen == 2)
			return;
		try {
			var sensorbluetooth=SensorBluetooth.blueone;
			if(sensorbluetooth==null) {
				{if(doLog) {Log.i(LOG_ID, "onCharacteristicWrite sensorbluetooth==null");};};
				return ;
				}
            BluetoothGattService service;
			BluetoothGattDescriptor descriptor;
			//noinspection MissingPermission
			boolean success = true;
			long tim = System.currentTimeMillis();
			justenablednotification = true;
			if(status == GATT_SUCCESS &&
                (service = bluetoothGatt.getService(mADCCustomServiceUUID)) != null &&
                (characteristic = service.getCharacteristic(mCharacteristicUUID_CompositeRawData)) != null &&
                bluetoothGatt.setCharacteristicNotification(characteristic, true) &&
                (descriptor = characteristic.getDescriptor(mCharacteristicConfigDescriptor)) != null) {
                        if(!descriptor.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE)) {
                            Log.e(LOG_ID,"descriptor.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE)  failed");
                            }
                        success = bluetoothGatt.writeDescriptor(descriptor);
                        } 
            else success = false;
			if (!success) {
				wrotepass[1] = tim;
				handshake = "Enabling notification failed";
				{if(doLog) {Log.i(LOG_ID, SerialNumber + " onCharacteristicWrite:  enabling notification failed");};};
                if(failedbefore) {
                   Natives.resetbluetooth(dataptr);
                   failedbefore=false;
                   }
                else failedbefore=true;
                bluetoothGatt.disconnect();
                return;
                }

         failedbefore=false;
			conphase = 4;
			wrotepass[0] = tim;
			pack1 = false;
			pack2 = false;
		} catch (Throwable e) {
			Log.stack(LOG_ID, SerialNumber+" onCharacteristicWrite", e);
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
/*
private boolean setDescriptor(BluetoothGattCharacteristic ch, byte[] type) {
       var gatt=mBluetoothGatt;
        BluetoothGattDescriptor descriptor = ch.getDescriptor(mCharacteristicConfigDescriptor);
        if(!descriptor.setValue(type)) {
            Log.e(LOG_ID, SerialNumber + " " + "descriptor.setValue())  failed");
            return false;
        }
        return gatt.writeDescriptor(descriptor);
        }
        */
private long datatime=0L;
private	void oldonCharacteristicChanged(byte[] value) {
		justenablednotification = false;
		int length = value.length;
		{if(doLog) {Log.d(LOG_ID, SerialNumber + " oldonCharacteristicChanged " + length);};};
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
                        /*
                    if(isWearable) {
                        if(Natives.getDisconnectSensor()) {
                            Log.i(LOG_ID,"enableNotification(mBluetoothGatt, characteristic)");
                            setDescriptor(characteristic,  BluetoothGattDescriptor.DISABLE_NOTIFICATION_VALUE);
                            Applic.scheduler.schedule(()-> {
                                if(connected) {
                                   setDescriptor(characteristic,  BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
                                    }
                                    }, 30, TimeUnit.SECONDS);
                                    
                            }
                        } */
					final long timmsec = System.currentTimeMillis();

					pack1 = false;
					pack2 = false;
					System.arraycopy(value, 0, packet, 38, 8);
					final var newpacket= sensorgen==2?Gen2Protocol.decryptBleFrame(gen2Session, packet):packet;
					if(newpacket!=null) {
						long res = processTooth(dataptr, newpacket);
						if(res!=1L) {
							handleGlucoseResult(res,timmsec);
							}
						}
                    datatime=timmsec;
					if(wakeLock!=null)
						wakeLock.release();
                  /*  if(isWearable) {
                        if(Natives.getDisconnectSensor()&&!autoconnect) {
                            disconnect();  
                            }
                        } */
				}
			}
			;
			break;
			default: {
				{if(doLog) {Log.i(LOG_ID, SerialNumber + " onCharacteristicChanged: wrong length=" + value.length);};};
			}
			;
		}
		;
	}

    private int gen2Session;

    private void closeGen2Session() {
        int session = gen2Session;
        gen2Session = 0;
        if (session > 0) Gen2Protocol.closeSession(session);
    }

    private byte[] failStreamAuthentication(String reason) {
        handshake = reason;
        wrotepass[1] = System.currentTimeMillis();
        Log.e(LOG_ID, SerialNumber + " " + reason);
        var gatt = mBluetoothGatt;
        if (gatt != null) gatt.disconnect();
        return null;
    }

    private byte[] authenticateStream(byte[] challenge) {
        closeGen2Session();
        byte[] uid = Natives.getsensorident(dataptr);
        if (uid == null) return failStreamAuthentication("No ident");

        byte[] provisioning = getstreamingAuthenticationData(dataptr);
        if (provisioning == null) return failStreamAuthentication("No Auth");
        if (provisioning.length < 10) {
            return failStreamAuthentication("Authenticationdata: len=" + provisioning.length);
        }

        int rootSlot = 0;
        if (Gen2.USE_ROOT_SLOT_METADATA) {
            // The older ten-byte provisioning record uses the default root.
            rootSlot = provisioning.length == 12
                    ? (provisioning[10] & 0xff) | ((provisioning[11] & 0xff) << 8)
                    : -1;
        }
        int session = Gen2Protocol.openSession(rootSlot, uid);
        if (session <= 0) return failStreamAuthentication("Auth = " + session);

        try {
            byte[] request = Gen2Protocol.buildBleAuthRequest(session, (byte) 0x1f,
                    provisioning, challenge);
            if (request == null || request.length != 19) {
                return failStreamAuthentication("Build BLE authentication request failed");
            }
            gen2Session = session;
            return request;
        } finally {
            if (gen2Session != session) Gen2Protocol.closeSession(session);
        }
    }

	//mCharacteristicUUID_BLELogin.toString().equals(bluetoothGattCharacteristic.getUuid().toString())
	private final byte[] authResponse = new byte[25];
	public final void phase2(byte[] value) {
		if (value.length != 14) {
			Log.e(LOG_ID, SerialNumber+" phase2 wrong " + value);
			var gatt = mBluetoothGatt;
			if (gatt != null)
				gatt.disconnect();
			return;
		}
		try {
			byte[] a = authenticateStream(value);
			if(a==null)  {
                		Log.e(LOG_ID, SerialNumber+" phase2 wrong (a == null)");
				var gatt = mBluetoothGatt;
				if (gatt != null)
					gatt.disconnect();
				return;
				}
		    mBLELoginHandler = () -> {
			BLELogincharacteristic.setValue(a);
            var gatt = mBluetoothGatt;
            if(gatt==null)
                return;
			if(!gatt.writeCharacteristic(BLELogincharacteristic)) {
			    var mess = "phase2 retry=" + BLELoginposted + " writeCharacteristic(BLELogincharacteristic)";
			    Log.e(LOG_ID, SerialNumber+" "+mess);
			    handshake = mess;
			    wrotepass[1] = System.currentTimeMillis();
			    if(BLELoginposted < 5) {
                    ++BLELoginposted;
                    Applic.app.getHandler().postDelayed(mBLELoginHandler, 100);
                    return;
                    }
			  //  var gatt = mBluetoothGatt;
			    if(gatt != null)
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
			Log.e(LOG_ID, SerialNumber+" "+handshake);
			wrotepass[1] = System.currentTimeMillis();
			var gatt = mBluetoothGatt;
			if (gatt != null)
				gatt.disconnect();
		}
	}


    public final void phase3(byte[] value) {
        if (value.length == 7) {
            System.arraycopy(value, 0, authResponse, 0, value.length);
            return;
        }
        if (value.length != 18) {
            failStreamAuthentication("Erroneous authentication response length " + value.length);
            return;
        }

        System.arraycopy(value, 0, authResponse, 7, value.length);
        int status = Gen2Protocol.acceptBleAuthentication(gen2Session, authResponse);
        if (status != 0 || gen2Session <= 0) {
            closeGen2Session();
            failStreamAuthentication("Accept BLE authentication failed: " + status);
            return;
        }
        if (doLog) Log.i(LOG_ID, "Gen2 session " + gen2Session);
        conphase = 4;

        mBLELoginHandler = () -> {
            if (!asknotification(CompositeRawDatacharacteristic)) {
                Log.e(LOG_ID, SerialNumber + " phase3 retry=" + BLELoginposted + " enableNotification failed");
                handshake = "Enable CompositeRawDatacharacteristic failed";
                wrotepass[1] = System.currentTimeMillis();
                if (BLELoginposted < 5) {
                    ++BLELoginposted;
                    Applic.app.getHandler().postDelayed(mBLELoginHandler, 100);
                    return;
                }
                conphase = 0;
                var gatt = mBluetoothGatt;
                if (gatt != null) gatt.disconnect();
                return;
            }
            BLELoginposted = 0;
            mBLELoginHandler = null;
            wrotepass[0] = System.currentTimeMillis();
        };
        mBLELoginHandler.run();
    }

@Override // android.bluetooth.BluetoothGattCallback
public synchronized void onCharacteristicChanged(BluetoothGatt bluetoothGatt, BluetoothGattCharacteristic bluetoothGattCharacteristic) {
        if (bluetoothGatt != mBluetoothGatt || !canConnectSensor()) return;
	byte[] value = bluetoothGattCharacteristic.getValue();
	var uuid=bluetoothGattCharacteristic.getUuid();
	var uuidstr=uuid.toString();
    if(doLog)
	  Log.showbytes(LOG_ID+ " "+SerialNumber + " onCharacteristicChanged " + uuidstr,value);
	if(sensorgen != 2) {
		if (!uuid.equals(mCharacteristicUUID_CompositeRawData)) {
			if(doLog) {
				{if(doLog) {Log.i(LOG_ID, SerialNumber + " onCharacteristicChanged: wrong UUID:" + uuidstr);};};
				}
			return;
			}
		oldonCharacteristicChanged(value);
		return;
		}

	if(doLog) {
//      boolean str=uuidstr.equals(mCharacteristicUUID_CompositeRawData.toString());
      final boolean isRawData=uuid.equals(mCharacteristicUUID_CompositeRawData);
      Log.i(LOG_ID, "UUID: "+(isRawData?"CompositeRawData, ":"")+ " conphase=" + conphase);
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
	public synchronized void close() {
	   endBLEHandler();
           closeGen2Session();
      final var gatt=mBluetoothGatt;
      if(nonNull(gatt)) {
         disablenotification(gatt,BLELogincharacteristic);
         disablenotification(gatt, CompositeRawDatacharacteristic) ;
         }
		super.close();
		}
@Override
public void onReadRemoteRssi(BluetoothGatt gatt, int rssi, int status)  {
	{if(doLog) {Log.i(LOG_ID,"onReadRemoteRssi(BluetoothGatt,"+ rssi+","+status+(status==GATT_SUCCESS?" SUCCESS":" FAILURE"));};};
	if(status==GATT_SUCCESS) {
		readrssi=rssi;
		}
	}

@Override
public boolean matchDeviceName(String deviceName,String address) {
	if(deviceName==null||!deviceName.startsWith("ABBOTT")) {
		return false;
		}
	return SerialNumber.equals(deviceName.substring(6));
	}
@Override
synchronized void free() {
    super.free();
    cancelalarm();
    }
@Override
protected boolean canConnectSensor() {
    return !stop;// && !Libre2WatchOwner.paused(SerialNumber);
}

@Override
public synchronized long resetdataptr() {
    return super.resetdataptr();
}
/*
synchronized byte[] garminSetup() {
    return Natives.garminLibre2Setup(dataptr, Natives.getunit());
}

synchronized long garminFrame(byte[] frame, int seconds) {
    long result = Natives.garminLibre2Frame(dataptr, frame, seconds);
    if (result > 1) handleGlucoseResult(result, seconds * 1000L);
    return result;
}
*/
@Override
public UUID getService() {
   return mADCCustomServiceUUID;
   }
/*@Override
public void setGattOptions(BluetoothGatt gatt) {
	if(Build.VERSION.SDK_INT >= 26 ) {
        if(doLog) {Log.i(LOG_ID,"setGattOptions(BluetoothGatt gatt) setPreferredPhy PHY_LE_2M_MASK");};
        gatt.setPreferredPhy(
         BluetoothDevice.PHY_LE_2M_MASK,
         BluetoothDevice.PHY_LE_2M_MASK,
         BluetoothDevice.PHY_OPTION_NO_PREFERRED); 
         }
        } */

}

