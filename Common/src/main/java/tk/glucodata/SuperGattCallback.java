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
import android.content.Context;
import android.content.Intent;
import android.os.Build;

import java.util.UUID;

import static android.bluetooth.BluetoothGatt.CONNECTION_PRIORITY_BALANCED;
import static android.bluetooth.BluetoothGatt.CONNECTION_PRIORITY_HIGH;
import static tk.glucodata.Applic.app;
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.Applic.mgdLmult;
import static tk.glucodata.Natives.thresholdchange;
import static tk.glucodata.SensorBluetooth.blueone;

public abstract class SuperGattCallback extends BluetoothGattCallback {
volatile protected	boolean stop=false;
public static boolean doWearInt=false;
public static boolean doGadgetbridge=false;
	private static final String LOG_ID="SuperGattCallback";
	static final private int  use_priority=CONNECTION_PRIORITY_HIGH;
	static  boolean autoconnect=false;

		 protected static final UUID mCharacteristicConfigDescriptor = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb");

	protected static final UUID mCharacteristicUUID_BLELogin = UUID.fromString("0000f001-0000-1000-8000-00805f9b34fb");
	protected static final UUID mCharacteristicUUID_CompositeRawData = UUID.fromString("0000f002-0000-1000-8000-00805f9b34fb");
    public static final UUID LIBRE3_DATA_SERVICE = UUID.fromString("089810cc-ef89-11e9-81b4-2a2ae2dbcce4");
    public static final UUID SIG_SERVICE_DEVICE_INFO = UUID.fromString("0000180a-0000-1000-8000-00805f9b34fb");
    public static final UUID LIBRE3_SECURITY_SERVICE = UUID.fromString("0898203a-ef89-11e9-81b4-2a2ae2dbcce4");
    public static final UUID LIBRE3_DEBUG_SERVICE = UUID.fromString("08982400-ef89-11e9-81b4-2a2ae2dbcce4");
    public static final UUID LIBRE3_CHAR_BLE_LOGIN = UUID.fromString("0000f001-0000-1000-8000-00805f9b34fb");
    public static final UUID LIBRE3_CHAR_PATCH_CONTROL = UUID.fromString("08981338-ef89-11e9-81b4-2a2ae2dbcce4");
    public static final UUID LIBRE3_CHAR_PATCH_STATUS = UUID.fromString("08981482-ef89-11e9-81b4-2a2ae2dbcce4");
    public static final UUID LIBRE3_CHAR_EVENT_LOG = UUID.fromString("08981bee-ef89-11e9-81b4-2a2ae2dbcce4");
    public static final UUID LIBRE3_CHAR_GLUCOSE_DATA = UUID.fromString("0898177a-ef89-11e9-81b4-2a2ae2dbcce4");
    public static final UUID LIBRE3_CHAR_HISTORIC_DATA = UUID.fromString("0898195a-ef89-11e9-81b4-2a2ae2dbcce4");
    public static final UUID LIBRE3_CHAR_CLINICAL_DATA = UUID.fromString("08981ab8-ef89-11e9-81b4-2a2ae2dbcce4");
    public static final UUID LIBRE3_CHAR_FACTORY_DATA = UUID.fromString("08981d24-ef89-11e9-81b4-2a2ae2dbcce4");
    public static final UUID LIBRE3_SEC_CHAR_COMMAND_RESPONSE = UUID.fromString("08982198-ef89-11e9-81b4-2a2ae2dbcce4");
    public static final UUID LIBRE3_SEC_CHAR_CHALLENGE_DATA = UUID.fromString("089822ce-ef89-11e9-81b4-2a2ae2dbcce4");
    public static final UUID LIBRE3_SEC_CHAR_CERT_DATA = UUID.fromString("089823fa-ef89-11e9-81b4-2a2ae2dbcce4");
	//    private final UUID mCharacteristicUUID_ManufacturerNameString = UUID.fromString("00002a29-0000-1000-8000-00805f9b34fb");
//    private final UUID mCharacteristicUUID_SerialNumberString = UUID.fromString("00002a25-0000-1000-8000-00805f9b34fb");
//    private final UUID mSIGDeviceInfoServiceUUID = UUID.fromString("0000180a-0000-1000-8000-00805f9b34fb");
	final long starttime = System.currentTimeMillis();
	String SerialNumber;
	public String mActiveDeviceAddress;
	protected long dataptr = 0L;
	public BluetoothDevice mActiveBluetoothDevice;
	long foundtime = 0L;
	protected BluetoothGatt mBluetoothGatt;
	boolean superseded=false;
	public final int sensorgen;
	int readrssi=9999;
	private long sensorstartmsec;
//	public SuperGattCallback(SensorBluetooth sensorbluetooth, String SerialNumber, long dataptr);
protected	SuperGattCallback(String SerialNumber,long dataptr,int gen) {
	this.SerialNumber = SerialNumber;
	this.dataptr = dataptr;
	mActiveDeviceAddress = Natives.getDeviceAddress(dataptr);
	sensorstartmsec=Natives.getSensorStartmsec(dataptr);
	sensorgen=gen;
	Log.i(LOG_ID, "new SuperGattCallback " + SerialNumber + " " + ((mActiveDeviceAddress != null) ? mActiveDeviceAddress : "null"));
	}
public void disconnect() {
	final var thegatt= mBluetoothGatt;
	if(thegatt!=null) {
		Log.i(LOG_ID,"Disconnect");
		thegatt.disconnect();
		}
	}
public void reconnect(long old) {
	if(charcha[1]<old)  {
		Log.i(LOG_ID,"reconnect "+SerialNumber);

		final var thegatt= mBluetoothGatt;
		if(thegatt!=null) 
			thegatt.disconnect();
		connectDevice(0);
		}
	}

void shouldreconnect(long old) {
	if(starttime<old&&charcha[0]<old)  
		reconnect(old);
	}

	long[] constatchange = {0L, 0L};
	int constatstatus = -1;
	String handshake = "";
	long[] wrotepass = {0L, 0L};
	long[] charcha = {0L, 0L};


	static final long thefuture = 0x7FFFFFFFFFFFFFFFL;
	static  long oldtime = thefuture;
static	long lastfound() {
		return SuperGattCallback.oldtime - GlucoseAlarms.showtime;
	}

	static long[] nextalarm = {0L, 0L};

	static public void writealarmsuspension(int kind, short wa) {
		short prevsus = Natives.readalarmsuspension(kind);
		Natives.writealarmsuspension(kind, wa);
		int versch = wa - prevsus;
		nextalarm[kind] += versch * 60;
	}


static final int mininterval=55;
static long nexttime=0L; //secs
public static tk.glucodata.GlucoseAlarms glucosealarms=null;
static notGlucose previousglucose=null;
static void init(Application app) {
       if(glucosealarms==null) glucosealarms=new tk.glucodata.GlucoseAlarms(app);
	if(!isWearable) {
		Talker.getvalues();
		if(Talker.shouldtalk())
			newtalker(null);
		}
	}

static Talker talker;
static boolean dotalk=false;
static void newtalker(Context context) {
	if(!isWearable) {
		if (talker != null)
			talker.destruct();
		talker = new Talker(context);
	}
	}
static void endtalk() {
	if(!isWearable) {
		dotalk = false;
		if (talker != null) {
			talker.destruct();
			talker = null;
		} 
		}
	}


	static void dowithglucose(String SerialNumber, int mgdl, float gl, float rate, int alarm, long timmsec,long sensorstartmsec) {
		if(gl==0.0)
			return;
		if (glucosealarms == null)
			return;
		glucosealarms.setagealarm(timmsec);
		final long tim = timmsec / 1000L;
		boolean waiting = false;
		var sglucose=new notGlucose(timmsec, String.format(Applic.usedlocale,Notify.pureglucoseformat, gl),  rate);
		previousglucose=sglucose;
		final var fview=Floating.floatview;
//		MainActivity.showmessage=null;
		boolean alarmspeak=false;
		if(fview!=null) 
			fview.postInvalidate();

		try {
			switch (alarm) {
				case 4:
					if(!Natives.hasalarmhigh()) {
						Notify.onenot.normalglucose(sglucose,gl, rate,false);
						break;
						}
				case 6: {
					final boolean alarmtime = tim > nextalarm[1];
					Notify.onenot.highglucose(sglucose,gl, rate,alarmtime);
					if (alarmtime) {
						nextalarm[1] = tim + Natives.readalarmsuspension(1) * 60;
						alarm |= 8;
						if(!isWearable) {
							if((alarmspeak=Natives.speakalarms())) talker.nexttime = 0L;
						}
					}
				}
				;
				break;
				case 5:
					if(!Natives.hasalarmlow()) {
						Notify.onenot.normalglucose(sglucose,gl, rate,false);
						break;
						}
				case 7: {
					final boolean alarmtime = tim > nextalarm[0];
					Notify.onenot.lowglucose(sglucose,gl, rate,alarmtime);
					if (alarmtime) {
						nextalarm[0] = tim + Natives.readalarmsuspension(0) * 60;
						alarm |= 8;
						if(!isWearable) {
							if((alarmspeak=Natives.speakalarms())) talker.nexttime = 0L;
						}
					}
				}
				;
				break;
				case 3:
					waiting = true;
				default:
					Notify.onenot.normalglucose(sglucose,gl, rate,waiting);
			}
			;
		} catch (Throwable e) {
			Log.stack(LOG_ID,SerialNumber, e);
		}
		Log.v(LOG_ID, SerialNumber + " "+tim+" glucose=" + gl + " " + rate);
		Applic.updatescreen();

		if(!isWearable) {
			if (dotalk||alarmspeak)  {
				talker.selspeak(sglucose.value);
				}
			}


		if(Natives.getJugglucobroadcast())
			JugglucoSend.broadcastglucose(SerialNumber,mgdl,gl,rate,alarm,timmsec);
		if(!isWearable) {
			app.numdata.sendglucose(SerialNumber, tim, gl, thresholdchange(rate), alarm|0x10);
			GlucoseWidget.update();
			}
		if(tim>nexttime) {
			nexttime=tim+mininterval;
			if(!isWearable) {
				if(Natives.getlibrelinkused())
					XInfuus.sendGlucoseBroadcast(SerialNumber, mgdl, rate, timmsec);

				}
			if(Natives.getxbroadcast())
				SendLikexDrip.broadcastglucose(mgdl,rate,timmsec,sensorstartmsec);
			if(!isWearable) {
				if(doWearInt)
					tk.glucodata.WearInt.sendglucose(mgdl, rate, alarm, timmsec);

				if(doGadgetbridge)
					Gadgetbridge.sendglucose(sglucose.value,mgdl,gl,rate,timmsec);
				} 
			}

	}
boolean stopHealth=false;
private boolean	dohealth(SuperGattCallback one) {
if(!isWearable) {
		var blue=blueone;
		if(blue==null)
			return true; //false?
	   final var gatts=blue.gattcallbacks;
	   boolean other=gatts.size()>1;
	   if(!other) {
	   	return true; //TODO stopHealth=false
		}
	   if(stopHealth)
	   	return false;
	   for(var el:gatts) {
	   	if(el!=one)
			el.stopHealth=true;
	   	}
	return true;
	}
else {
	return false;
	}
	}
protected void handleGlucoseResult(long res,long timmsec) {
		int glumgdl = (int) (res & 0xFFFFFFFFL);
		if (glumgdl != 0) {
			int alarm = (int) ((res >> 48) & 0xFFL);
			Log.i(LOG_ID, SerialNumber + " alarm=" + alarm);
			float gl = Applic.unit == 1 ? glumgdl / mgdLmult : glumgdl;
			short ratein = (short) ((res >> 32) & 0xFFFFL);
			float rate = ratein / 1000.0f;
			dowithglucose(SerialNumber, glumgdl, gl, rate, alarm, timmsec,sensorstartmsec);
			charcha[0] = timmsec;
			if(!isWearable) {
				if(Natives.gethealthConnect( )) {
				    if(Build.VERSION.SDK_INT >= 28) {
					if(dohealth(this)) {
							final long sensorptr = Natives.getsensorptr(dataptr);//TODO: set sensorptr in SuperGattCallback?
							HealthConnection.Companion.writeAll(sensorptr,SerialNumber);
							}
						}
					}
				}
			SensorBluetooth.othersworking(this,timmsec);
		} else {
			Log.i(LOG_ID, SerialNumber + " onCharacteristicChanged: Glucose failed");
			charcha[1] = timmsec;
		}
	}

public void searchforDeviceAddress() {
	Log.i(LOG_ID,SerialNumber+" searchforDeviceAddress()");

	setDeviceAddress(null);
	close();
	}	
	String getinfo() {
		if(dataptr!=0L)
			return Natives.getsensortext(dataptr);
		return "";
		}
	public void resetdataptr() {
		Natives.freedataptr(dataptr);
		close();
		dataptr = Natives.getdataptr(SerialNumber);
		mActiveDeviceAddress = Natives.getDeviceAddress(dataptr);
	}


	public void setDevice(BluetoothDevice device) {

		mActiveBluetoothDevice = device;
		if(device!=null) {
			String address=device.getAddress();
			Log.i(LOG_ID,SerialNumber+"setDevice("+address+")");
			setDeviceAddress(address);
			}
		else  {
			Log.i(LOG_ID, SerialNumber +" "+"setDevice("+null+")");
			setDeviceAddress(null);
			}
	}

	public void setDeviceAddress(String address) {
		Log.i(LOG_ID, SerialNumber +" "+"setDeviceAddress("+ address+")");
		mActiveDeviceAddress = address;
		Natives.setDeviceAddress(dataptr, address);
	}
	void free() {
		stop=true;
		Log.i(LOG_ID,"free "+SerialNumber);
		close();
		Natives.freedataptr(dataptr);
		dataptr = 0L;
	 	//sensorbluetooth=null;
	}
	boolean streamingEnabled() {//TODO: libre3?
		return Natives.askstreamingEnabled(dataptr);
		}
	void finishSensor() {
		Natives.finishSensor(dataptr);
		}
	public void close() {
		Log.i(LOG_ID,"close "+SerialNumber);
		var tmpgatt=mBluetoothGatt ;
		if (tmpgatt != null) {
			try {
				tmpgatt.disconnect();
				tmpgatt.close();
			} catch (SecurityException se) {
				var mess = se.getMessage();
				mess = mess == null ? "" : mess;
				String uit = ((Build.VERSION.SDK_INT > 30) ? Applic.app.getString(R.string.turn_on_nearby_devices_permission)  : mess) ;
				Applic.Toaster(uit);
				Log.stack(LOG_ID, SerialNumber +" "+ "BluetoothGatt.close()", se);
			}
		finally {	
			mBluetoothGatt = null;
			}
		}

	}
	private Runnable getConnectDevice(long delayMillis) {
		var cb = this;
		close();
		if (cb.mActiveBluetoothDevice == null) {
			Log.i(LOG_ID, SerialNumber +" "+"cb.mActiveBluetoothDevice == null");
			foundtime = 0L;
			return null;
		}
		return () -> {
			Log.i(LOG_ID,"getConnectDevice Runnable "+ SerialNumber);
			var device= cb.mActiveBluetoothDevice;
			var sensorbluetooth= blueone;
			if(sensorbluetooth==null) {
				Log.e(LOG_ID, SerialNumber +" "+"sensorbluetooth==null");
				return;
				}
			if(!sensorbluetooth.bluetoothIsEnabled()) {
				Log.e(LOG_ID, SerialNumber +" "+"!sensorbluetooth.bluetoothIsEnabled()");
				return ;
				}
			if(device==null) {
				Log.e(LOG_ID, SerialNumber +" "+"device==null");
				return;
				}
		
			if (cb.mBluetoothGatt != null) {
				Log.d(LOG_ID, SerialNumber + " cb.mBluetoothGatt!=null");
				return;
				} 
				if (tk.glucodata.Log.doLog) {
					Log.d(LOG_ID, SerialNumber + " Try connection to " + device.getAddress());
					}
				try {
					if(isWearable)  {
						cb.mBluetoothGatt = device.connectGatt(Applic.app, autoconnect, cb, BluetoothDevice.TRANSPORT_LE);
						}
					else {
						if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
							cb.mBluetoothGatt = device.connectGatt(Applic.app, autoconnect, cb, BluetoothDevice.TRANSPORT_LE);
						} else {
							cb.mBluetoothGatt = device.connectGatt(Applic.app, autoconnect, cb);
							}
						}
					setpriority(cb.mBluetoothGatt);
				Log.i(LOG_ID,SerialNumber+" after connectGatt");
				} catch (SecurityException se) {
					var mess = se.getMessage();
					mess = mess == null ? "" : mess;
					String uit = ((Build.VERSION.SDK_INT > 30) ? Applic.app.getString(R.string.turn_on_nearby_devices_permission)  : mess) ;
					Applic.Toaster(uit);

					Log.stack(LOG_ID, SerialNumber +" "+ "connectGatt", se);
				} catch (Throwable e) {
					Log.stack(LOG_ID, SerialNumber +" "+ "connectGatt", e);

					}
		};
	}

   public boolean connectDevice(long delayMillis) {
      Log.i(LOG_ID,"connectDevice("+delayMillis+") "+ SerialNumber);
	Runnable connect=getConnectDevice(delayMillis);
	if(connect==null) 
		return false;
	if(delayMillis>0)
		Applic.app.getHandler().postDelayed(connect, delayMillis);
	else
		Applic.app.getHandler().post(connect);
	return true;
	}
	@SuppressLint("MissingPermission")
	void setpriority(BluetoothGatt bluegatt) {
		if(bluegatt!=null) {
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
				if (Natives.getpriority())
					bluegatt.requestConnectionPriority(use_priority);
				else
					bluegatt.requestConnectionPriority(CONNECTION_PRIORITY_BALANCED);
			}
			}
		else {
			Log.e(LOG_ID, SerialNumber +" "+"setpriority BluetoothGatt==null");
			}
	}

@SuppressLint("MissingPermission")
 protected  final boolean enableNotification(BluetoothGatt mBluetoothGatt, BluetoothGattCharacteristic bluetoothGattCharacteristic) {
	Log.i(LOG_ID, SerialNumber +" "+	"enableNotification");
        if(!mBluetoothGatt.setCharacteristicNotification(bluetoothGattCharacteristic, true)) {
		Log.e(LOG_ID, SerialNumber +" "+"setCharacteristicNotification("+bluetoothGattCharacteristic.getUuid().toString()+",true) failed");

		}
        BluetoothGattDescriptor descriptor = bluetoothGattCharacteristic.getDescriptor(mCharacteristicConfigDescriptor);
        if(!descriptor.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE)) {
		Log.e(LOG_ID, SerialNumber +" "+"descriptor.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE))  failed");
		}
        if(!mBluetoothGatt.writeDescriptor(descriptor)) {
		Log.e(LOG_ID, SerialNumber +" "+"mBluetoothGatt.writeDescriptor(descriptor))  failed");
		return false;
		}
	return true;
        }

protected final boolean asknotification(BluetoothGattCharacteristic charac) {
		return enableNotification(mBluetoothGatt, charac);
	}
public boolean matchDeviceName(String deviceName) {
	return false;
	}
}
