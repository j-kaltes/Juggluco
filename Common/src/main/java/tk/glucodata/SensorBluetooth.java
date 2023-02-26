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
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothManager;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanFilter;
import android.bluetooth.le.ScanResult;
import android.bluetooth.le.ScanSettings;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Build;
import android.os.ParcelUuid;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.UUID;

import androidx.annotation.RequiresApi;

import static android.bluetooth.BluetoothProfile.GATT;
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.BuildConfig.libreVersion;

public class SensorBluetooth {
public static SensorBluetooth blueone=null;
public static void startscan() {
	if(blueone!=null)
		blueone.startScan(0L);
	}
    private static final String LOG_ID = "SensorBluetooth";
    private static final int scantimeout = 390000;
    private static final int  scaninterval=60000;
    protected static final UUID mADCCustomServiceUUID = UUID.fromString("0000fde3-0000-1000-8000-00805f9b34fb");

//   public Applic Applic.app;
 static   private BluetoothAdapter mBluetoothAdapter;
    private BroadcastReceiver mBluetoothAdapterReceiver =null; ;
static    private BluetoothManager mBluetoothManager=null;

static public void reconnectall() {
   final var wasblue=blueone;
   if(wasblue !=null) {
   	final var old=System.currentTimeMillis()-1000L*59*3;
        for(var cb: wasblue.gattcallbacks)     {
		cb.reconnect(old);
		}
	    }
   }

    public boolean connectToActiveDevice(long delayMillis) {
    Log.i(LOG_ID, "connectToActiveDevice("+delayMillis+")");
	if(!bluetoothIsEnabled()) {
		Applic.Toaster("Enable Bluetooth");
		return false;
		}
        boolean scan=false;
        for(var cb: gattcallbacks)    
		if(!cb.connectDevice(delayMillis))
			scan=true;
        if(scan) {
		return startScan(delayMillis);
	    }
	  return false;
    	}
    public boolean connectToActiveDevice(SuperGattCallback cb,long delayMillis) {
	Log.i(LOG_ID,"connectToActiveDevice(SuperGattCallback cb,"+ delayMillis+")");
	if(!cb.connectDevice(delayMillis)&&!mScanning) {
		return startScan(delayMillis);
		}
	return false;
    }
long unknownfound=0L;
String unknownname="";
private SuperGattCallback  getCallback(BluetoothDevice device) {
	try {
		@SuppressLint("MissingPermission") String deviceName = device.getName();
		Log.i(LOG_ID, "deviceName=" + deviceName);
		if(deviceName == null) {
			Log.d(LOG_ID, "Scan returns device without name");
			return null;
		}
		if(!deviceName.startsWith("ABBOTT")) {
			return null;
		}
		String address = device.getAddress();
		String SerialNumber = deviceName.substring(6);
		for (var cb : gattcallbacks) {
			if (cb.mActiveDeviceAddress != null && address.equals(cb.mActiveDeviceAddress))
				return cb;
			if (SerialNumber.equals(cb.SerialNumber))
				return cb;
			Log.d(LOG_ID, "not: " + cb.SerialNumber);
		}
		unknownfound = System.currentTimeMillis();
		unknownname = SerialNumber;
		return null;
	} catch(Throwable e) {
		Log.stack(LOG_ID,	"getCallback",e);
		if(Build.VERSION.SDK_INT > 30&&!Applic.mayscan())
			Applic.Toaster("Turn on NEARBY DEVICES permission");
		return null;
	}
}

//long foundtime=0L;

@SuppressLint("MissingPermission")
private boolean checkdevice(BluetoothDevice device) {
	try {
		SuperGattCallback cb = getCallback(device);
		if (cb != null) {
			boolean newdev = true;
			if (cb.foundtime == 0L) {
				cb.foundtime = System.currentTimeMillis();
				int state;
				if (cb.mBluetoothGatt != null && cb.mActiveBluetoothDevice == device && ((state = mBluetoothManager.getConnectionState(device, GATT)) == BluetoothGatt.STATE_CONNECTED || state == BluetoothGatt.STATE_CONNECTING))
					newdev = false;
			} else
				newdev = false;

			boolean ret = true;
			cb.setDevice(device);
			for (SuperGattCallback one : gattcallbacks) {
				if (one.mActiveBluetoothDevice == null) {
					Log.i(LOG_ID, one.SerialNumber + " not found");
					ret = false;
					break;
				}
			}
			if (ret) SensorBluetooth.this.stopScan(false);
			if (newdev) {
				SensorBluetooth.this.connectToActiveDevice(cb, 0);
			}
			return ret;
		}
		Log.d(LOG_ID, "BLE unknown device");
		return false;
	} catch (Throwable e) {
		Log.stack(LOG_ID, "checkdevice", e);
		if (Build.VERSION.SDK_INT > 30 && !Applic.mayscan())
			Applic.Toaster("Turn on NEARBY DEVICES permission");
		return true;
	}
}

long scantimeouttime=0L;

boolean mScanning = false;
class Scanner21 implements Scanner  {
final	private List<ScanFilter> mScanFilters = new ArrayList<>();
final	private ScanSettings mScanSettings;
	private BluetoothLeScanner mBluetoothLeScanner=null;
	@RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
	private final ScanCallback mScanCallback = new ScanCallback() {
		@RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
		private boolean processScanResult(ScanResult scanResult) {
		    if (gattcallbacks.size()<1) {
			Log.w(LOG_ID,"No Sensors to search for");
			SensorBluetooth.this.stopScan(false);
			return true;
		    }
			return checkdevice(scanResult.getDevice());
		}
		private  boolean resultbusy=false;

		@RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
		@Override
		public void onScanResult(int callbackType, ScanResult scanResult) {
		    Log.d(LOG_ID,"onScanResult");
		    if(!resultbusy) {
			resultbusy=true;
			processScanResult(scanResult);
			resultbusy=false;
		    }
		}

		@Override 
		public void onBatchScanResults(List<ScanResult> list) {
		    int i = 0;
		    Log.v(LOG_ID,"onBatchScanResults");
		    while (i < list.size() && !processScanResult(list.get(i))) {
			i++;
		    }
		}

		@Override 
		public void onScanFailed(int errorCode) {
   		   if(!tk.glucodata.Applic.isRelease) {
			    final String[] scanerror={"SCAN_0",
				    "SCAN_FAILED_ALREADY_STARTED",
				    "SCAN_FAILED_APPLICATION_REGISTRATION_FAILED",
				    "SCAN_FAILED_INTERNAL_ERROR",
				    "SCAN_FAILED_FEATURE_UNSUPPORTED" };
			    Log.d(LOG_ID,"BLE SCAN ERROR: scan failed with error code: " + ((errorCode<scanerror.length)?scanerror[errorCode]:"")+" "+errorCode);
		      }
		    if (errorCode != SCAN_FAILED_ALREADY_STARTED) {
			SensorBluetooth.this.stopScan(false);
			if (errorCode != SCAN_FAILED_FEATURE_UNSUPPORTED) {

			    SensorBluetooth.this.startScan(scaninterval) ;
			}
		    }
		}
	    };
	@RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
	Scanner21() {
		ScanSettings.Builder builder = new ScanSettings.Builder();
		builder.setReportDelay(0);
		mScanSettings = builder.build();
		ScanFilter.Builder builder2 = new ScanFilter.Builder();
		builder2.setServiceUuid(new ParcelUuid(SensorBluetooth.mADCCustomServiceUUID));
		mScanFilters.add(builder2.build());
		Log.i(LOG_ID,"Scanner21");
		}
	@RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
	public boolean init() {
		Log.i(LOG_ID,"Scanner21.init");
		   return ( (mBluetoothLeScanner = SensorBluetooth.mBluetoothAdapter.getBluetoothLeScanner())!=null);
		   }
	@SuppressLint("MissingPermission")
	@RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
	public boolean start()  {
		if(mBluetoothLeScanner!=null) {
			Log.i(LOG_ID,"Scanner21.start");
				try {
		       this.mBluetoothLeScanner.startScan(mScanFilters, mScanSettings, mScanCallback);
			} catch (Throwable e) {
				Log.stack(LOG_ID, e);
				if (Build.VERSION.SDK_INT > 30 && !Applic.mayscan())
					Applic.Toaster("Turn on NEARBY DEVICES permission");
				return false;
		}
			   return true;
			   }
		return false;
		    }
	@SuppressLint("MissingPermission")
	@RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
	public void stop() {
		if(mBluetoothLeScanner!=null) {
			Log.i(LOG_ID,"Scanner21.stop");
			try {
				mBluetoothLeScanner.stopScan(mScanCallback);
				} catch (Throwable e) {
					Log.stack(LOG_ID,  e);
					if (Build.VERSION.SDK_INT > 30 && !Applic.mayscan())
						Applic.Toaster("Turn on NEARBY DEVICES permission");
				}
			}
		};

};
	@SuppressWarnings("deprecation")
class ArchScanner  implements Scanner {
   BluetoothAdapter.LeScanCallback mLeScanCallback= new BluetoothAdapter.LeScanCallback () {
        @Override
	public  void onLeScan (BluetoothDevice device, int rssi, byte[] scanRecord) {
		checkdevice(device);
		}
	}	;
	public boolean init() {
		Log.i(LOG_ID,"ArchScanner.init");
		return true;
		   }
	@SuppressLint("MissingPermission")
	public boolean start()  {
//		return SensorBluetooth.mBluetoothAdapter.startLeScan(new UUID[] {SensorBluetooth.this.mADCCustomServiceUUID},mLeScanCallback);
		return SensorBluetooth.mBluetoothAdapter.startLeScan(mLeScanCallback);
		  }
	@SuppressLint("MissingPermission")
	public void stop() {
		 SensorBluetooth.mBluetoothAdapter.stopLeScan( mLeScanCallback);
		};

};

Scanner scanner=Build.VERSION.SDK_INT >= 21?new Scanner21():new ArchScanner();

final private Runnable mScanTimeoutRunnable = () -> {
	Log.i(LOG_ID,"Timeout scanning");
scantimeouttime=System.currentTimeMillis();
	SensorBluetooth.this.stopScan(true);
};




static boolean bluetoothIsEnabled() {
        if (mBluetoothAdapter!= null) {
            return  mBluetoothAdapter.isEnabled();
        }
        return false;
    }


static public void sensorEnded(String str) {
   if(blueone!=null)
  	blueone.removeDevice(str) ;
    }



private boolean scanstart=false;	
long scantime=0L;
final private Runnable scanRunnable = new Runnable() {
            @Override 
            public void run() {
		scantime=System.currentTimeMillis();
                    SensorBluetooth sensorBluetooth = SensorBluetooth.this;
                if (bluetoothIsEnabled() && gattcallbacks.size() != 0) {
                    if (!scanner.init()) {
                        return;
                    }
                    Log.d(LOG_ID,"SCAN: starting scan.");
			for(var cb: gattcallbacks)   {
                    		Log.d(LOG_ID,"serial number: " + cb.SerialNumber);
                    		if (cb.mActiveDeviceAddress != null) {
					Log.d(LOG_ID,"address: " + cb.mActiveDeviceAddress);
				    }
				   }
		    if(scanner.start()) {
			   mScanning = true;
			    Applic.app.getHandler().postDelayed(sensorBluetooth.mScanTimeoutRunnable, scantimeout);
			    }
		   else {
                    	Log.d(LOG_ID,"Start scan failed");
			return;
                	}
		}		
            }

    };
     boolean startScan(long delayMillis) {

	    if(!Applic.mayscan()) {
	        Applic.Toaster((Build.VERSION.SDK_INT > 30)?"Turn on NEARBY DEVICES permission":"Turn on Location permission");
		 return true;
		 }

	if(!bluetoothIsEnabled()) {
		Applic.Toaster("Bluetooth is turned off");
		return false;
		}
     	scanstart=true;	
	if(delayMillis>0)
		Applic.app.getHandler().postDelayed(scanRunnable , delayMillis);
	else
		Applic.app.getHandler().post(scanRunnable);
	return false;
    }
long stopscantime=0L;
int startincreasedwait=300000;
int increasedwait=startincreasedwait;
public void stopScan(boolean retry) {
        Log.d(LOG_ID,"Stop scanning");
        Applic.app.getHandler().removeCallbacks(this.scanRunnable);
        Applic.app.getHandler().removeCallbacks(this.mScanTimeoutRunnable);
        if (this.mScanning) {
            stopscantime=System.currentTimeMillis();
            this.mScanning = false;
            if (bluetoothIsEnabled()) {
		scanner.stop();
		if(retry) {
			int waitscan=scaninterval;
			if(scantime>0L) {
			   for(SuperGattCallback cb:gattcallbacks) {
				if(cb.foundtime>scantime&&SuperGattCallback.lastfound()>cb.foundtime) {
					increasedwait*=2;
					waitscan=increasedwait;
					}

				}
				}
				startScan( waitscan);
				}
			}
            	}
    scanstart=false;
    }


ArrayList<SuperGattCallback> gattcallbacks=new ArrayList<SuperGattCallback>();

static  ArrayList<SuperGattCallback>  mygatts() {
	if(blueone==null|| blueone.gattcallbacks.size()==0)
		return null;
	return blueone.gattcallbacks;
	}
private void removeDevice(String str) {
	for(int i=0;i<gattcallbacks.size();i++) {
		if(str.equals(gattcallbacks.get(i).SerialNumber)) {
			Log.i(LOG_ID,"removeDevice "+ gattcallbacks.get(i).SerialNumber);
			gattcallbacks.get(i).free();
			gattcallbacks.remove(i);
			Natives.setmaxsensors(gattcallbacks.size());
			return;
			}
		}	
	Log.i(LOG_ID,"removeDevice: didn't remove"+ str);
	}
public void removeDevices() {
	for(int i=0;i<gattcallbacks.size();i++) {
		gattcallbacks.get(i).free();
		}
	gattcallbacks.clear();
	Natives.setmaxsensors(0);
	}

private void destruct() {
	if(mBluetoothManager!=null)   {
		stopScan(false);
		removeDevices();
		}
	}
public static void destructor() {
	if(blueone!=null) {
		blueone.destruct();
		blueone=null;
		}
	}
//static boolean nullKAuth=false;

private void setDevices(String[] names) {
	for(String name:names)  { 
		if(name!=null) {
			Log.i(LOG_ID,"setDevice "+name);
//			int index=gattcallbacks.size();
			long dataptr=Natives.getdataptr(name);
			if(dataptr!=0) {
				gattcallbacks.add(getGattCallback(name,dataptr));
				}
			increasedwait=startincreasedwait;
			}
		}
//	nullKAuth=false;

	Natives.setmaxsensors(gattcallbacks.size());
	}
public void startDevices(String[] names) {
	setDevices(names);
	initializeBluetooth();
	}

public boolean resetDevices() {
	if(!Natives.getusebluetooth()) {
		Log.d(LOG_ID,"resetDevices !getusebluetooth()");
		return false;
		}
	if(mBluetoothManager!=null)   
		stopScan(false);
	removeDevices() ;
	setDevices(Natives.activeSensors());
	return initializeBluetooth();
	}

static <T> int indexOf(final T[] ar,final T el) {
	for(int i=0;i<ar.length;i++)
		if(el.equals(ar[i]))
			return i;
	return -1;
	}

public boolean connectDevices(long delayMillis) {
	if(!bluetoothIsEnabled()) {
		Applic.Toaster("Enable Bluetooth");
		return false;
		}
        boolean scan=false;
        for(var cb: gattcallbacks)    {
		if(checkandconnect( cb,delayMillis))
			scan=true;
		}
        if(scan) {
		return startScan(delayMillis);
	    }
	  return false;
    	}
private boolean updateDevicers() {
	if(!Natives.getusebluetooth()) {
		Log.d(LOG_ID,"updateDevicers !getusebluetooth()");
		return false;
		}
	Log.i(LOG_ID,"updateDevicers");
	String[] devs=Natives.activeSensors();
	ArrayList<Integer> rem = new ArrayList<>();
	int gatnr=gattcallbacks.size();
	Log.i(LOG_ID,"updateDevicers nr active: "+gatnr);
	if(devs==null) {
		for(int i=0;i<gatnr;i++) {
			String was= gattcallbacks.get(i).SerialNumber;
			Log.i(LOG_ID,"can remove "+ was);
			rem.add(i);	
			}
		if(rem.size()==0)  {
			Log.i(LOG_ID,"Nothing to remove, no active sensors");
			return false;
			}
		}
	else {
		
		int heb=0;
		for(int i=0;i<gatnr;i++) {
			String was= gattcallbacks.get(i).SerialNumber;
			int instr=was==null?-1:indexOf(devs,was);
			if(instr<0) {
				Log.i(LOG_ID,"can remove "+ was);
				rem.add(i);	
				}
			else {
				Log.i(LOG_ID,"keep "+ was);
				heb++;
				devs[instr]=null;		
				}
			}
		if(devs.length==heb&&rem.size()==0) {
			Log.i(LOG_ID,"Nothing has to be done");
			return false;
			}
		}
	Log.i(LOG_ID,"needs update sensors");
	if(mBluetoothManager!=null)   
		stopScan(false);
//	rem.sort((x,y)->{return x-y;});
	 Collections.sort(rem, (x, y)->{return x-y;});

	for(int el=rem.size()-1;el>=0;el--) {
		int weg=rem.get(el);
		Log.i(LOG_ID,"remove "+ gattcallbacks.get(weg).SerialNumber);
		gattcallbacks.get(weg).free();
		gattcallbacks.remove(weg);
		}
	int index=gattcallbacks.size();
	if(devs!=null){
		for(String dev:devs) {
			if(dev!=null) {
				Log.i(LOG_ID,"add "+dev);
				long dataptr=Natives.getdataptr(dev);
				if(dataptr!=0L) {
					gattcallbacks.add(getGattCallback(dev,dataptr));
					increasedwait=startincreasedwait;
					index++;
					}
				}
			}
		}

//	nullKAuth=false;
//	Natives.setmaxsensors(gattcallbacks.size());
	if(mBluetoothManager ==null) {
		return initializeBluetooth();
		}
	else {
		addreceiver();
		return connectDevices(0);
		}
//	       startScan(0);
	}

static boolean updateDevices() {
	if(blueone==null) {
		return false;
		}
	return  blueone.updateDevicers();
	}

boolean checkandconnect(SuperGattCallback  cb,long delay) {
	Log.i(LOG_ID,"checkandconnect(SuperGattCallback  cb,"+ delay+")");
	if (cb.mActiveDeviceAddress != null) {
		if(BluetoothAdapter.checkBluetoothAddress(cb.mActiveDeviceAddress)) {
			Log.i(LOG_ID, "checkBluetoothAddress(" +cb.mActiveDeviceAddress +") succeeded");
			cb.mActiveBluetoothDevice = mBluetoothAdapter.getRemoteDevice(cb.mActiveDeviceAddress);
			connectToActiveDevice(cb, delay);
			return false;
		  }	 
		Log.i(LOG_ID, "checkBluetoothAddress(" +cb.mActiveDeviceAddress +") failed");
		 cb.setDeviceAddress(null);
		}
	if(Applic.mayscan()) {
		connectToActiveDevice(cb, delay);
		return false;
		}
	return true;
	}
SuperGattCallback getGattCallback(String name, long dataptr) {
	if(libreVersion==3) {
		int vers = Natives.getLibreVersion(dataptr);
		if (vers == 3) {
			return new Libre3GattCallback(this, name, dataptr);
			}
		}
	return  new MyGattCallback(this,name,dataptr);
	}
private boolean addDevice(String str,long dataptr) {
	Log.d(LOG_ID,"reset add "+str);
	int index=gattcallbacks.size();
	if(dataptr!=0L) {
		SuperGattCallback  cb= getGattCallback(str,dataptr);
//		nullKAuth=false;
		gattcallbacks.add(cb);
		Natives.setmaxsensors(gattcallbacks.size());
		increasedwait=startincreasedwait;
		if(mBluetoothManager ==null) {
			return initializeBluetooth();
			}
		else  {
			addreceiver();
			return checkandconnect( cb,0);
			}
		}
	else {
		Log.e(LOG_ID,"dataptr==0L");
	 }	
	return 	false;


	}
private boolean resetDevicer(long streamptr,String name) {
	if(mBluetoothManager!=null)   
		stopScan(false);
	for(int i=0;i<gattcallbacks.size();i++) {
		SuperGattCallback  cb= gattcallbacks.get(i);
	   if(Natives.sameSensor(streamptr,cb.dataptr)) {
		Log.d(LOG_ID,"reset free "+name);
		cb.resetdataptr();
		return checkandconnect(cb,0);
		}
	  }
	return addDevice(name,streamptr);
    }

static public boolean resetDevice(long ptr,String name) {
	if(blueone!=null) {
		return blueone.resetDevicer(ptr,name);
		}
	return false;
	}
private boolean resetDevicer(String str) {
	if(str==null)
		return false;
		
	if(mBluetoothManager!=null)   
		stopScan(false);
	for(int i=0;i<gattcallbacks.size();i++) {
		if(str.equals(gattcallbacks.get(i).SerialNumber)) {
			Log.d(LOG_ID,"reset free "+str);
			SuperGattCallback  cb= gattcallbacks.get(i);
			cb.resetdataptr();
			return checkandconnect(cb,0);
			}
		}

	Log.d(LOG_ID,"reset add "+str);
	long dataptr=Natives.getdataptr(str);
	return addDevice(str,dataptr);
    }

static public boolean resetDevice(String str) {
	Log.v(LOG_ID,"resetDevice("+str+")");
	if(blueone!=null) {
		return blueone.resetDevicer(str);
		}
	return false;
	}

static public   void goscan() { 
	if(blueone!=null) {
		blueone.connectToActiveDevice(0);
		}
	}

    public SensorBluetooth() {
        Log.v(LOG_ID,"SensorBluetooth");
        SuperGattCallback.glucosealarms.sensorinit();
    }

static void start() {
	if(SensorBluetooth.blueone==null) {
		blueone=new tk.glucodata.SensorBluetooth();
		if(blueone!=null) {
        		SuperGattCallback.glucosealarms.sensorinit();
			blueone.startDevices( Natives.activeSensors());
			}
		}
	else {
	 	blueone.connectDevices(0);
		}
	}
private void addreceiver() {
	if(mBluetoothAdapterReceiver==null) {
	 mBluetoothAdapterReceiver=new BroadcastReceiver() {
		@Override 
		public void onReceive(Context context, Intent intent) {
		    if ("android.bluetooth.adapter.action.STATE_CHANGED".equals(intent.getAction())) {
			int intExtra = intent.getIntExtra("android.bluetooth.adapter.extra.STATE", -1);
			if (intExtra == BluetoothAdapter.STATE_OFF) {
			    Log.v(LOG_ID,"BLUETOOTH switched OFF");
			    SensorBluetooth.this.stopScan(false);
			for(var cb: gattcallbacks)  
				cb.close();
			} else if (intExtra == BluetoothAdapter.STATE_ON) {
			    Log.v(LOG_ID,"BLUETOOTH switched ON");
			    if(!isWearable) {
				    Applic.app.numdata.sync();
				    }
			    SensorBluetooth.this.connectToActiveDevice(500);
			}
		    }
		}
	    };
		Applic.app.registerReceiver( mBluetoothAdapterReceiver, new IntentFilter("android.bluetooth.adapter.action.STATE_CHANGED"));
		}
	}
private boolean initializeBluetooth() {


        Log.v(LOG_ID,"initializeBluetooth");
//        mBluetoothManager = (BluetoothManager) Applic.app.getSystemService("bluetooth");
        mBluetoothManager = (BluetoothManager) Applic.app.getSystemService(Context.BLUETOOTH_SERVICE);
        if (mBluetoothManager  == null) {
            Log.i(LOG_ID, "getSystemService(\"BLUETOOTH_SERVICE\")==null");
        } else {
            mBluetoothAdapter = mBluetoothManager.getAdapter();
            if (mBluetoothAdapter == null) {
                Log.i(LOG_ID, "bluetoothManager.getAdapter()==null");
            } else {
                if (gattcallbacks.size()!=0) {
			Log.i(LOG_ID,"initializeBluetooth gattcallbacks");
			for(SuperGattCallback cb: gattcallbacks) {
				if(cb.mActiveDeviceAddress!=null)  {
				    if (BluetoothAdapter.checkBluetoothAddress(cb.mActiveDeviceAddress)) {
				    	Log.i(LOG_ID,"checkBluetoothAddress("+cb.mActiveDeviceAddress+") succeeded") ;

					cb.mActiveBluetoothDevice = mBluetoothAdapter.getRemoteDevice(cb.mActiveDeviceAddress);
				    } else {
				    	Log.i(LOG_ID,"checkBluetoothAddress("+cb.mActiveDeviceAddress+") failed") ;
				        cb.setDeviceAddress(null);
				    }
				    }
				}
			addreceiver();
			return connectToActiveDevice(0);
			}
		else
			Log.i(LOG_ID,"initializeBluetooth no gattcallbacks");
            }
        }

	return false;
    }

}
