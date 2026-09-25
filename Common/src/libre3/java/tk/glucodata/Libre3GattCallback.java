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
/*      Fri Jan 27 15:26:08 CET 2023                                                 */


package tk.glucodata;

import android.annotation.SuppressLint;
import android.app.AlarmManager;
import android.app.PendingIntent;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.os.Looper;
import android.os.PowerManager;

//import java.security.SecureRandom;
import java.io.File;
import java.lang.reflect.Method;
import java.util.Queue;
import java.util.UUID;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;

import static android.app.PendingIntent.getBroadcast;
import static android.bluetooth.BluetoothDevice.PHY_LE_1M_MASK;
import static android.bluetooth.BluetoothDevice.PHY_OPTION_NO_PREFERRED;
import static android.bluetooth.BluetoothGatt.CONNECTION_PRIORITY_BALANCED;
import static android.bluetooth.BluetoothGatt.CONNECTION_PRIORITY_HIGH;
import static android.bluetooth.BluetoothGatt.GATT_SUCCESS;
import static android.bluetooth.BluetoothProfile.STATE_CONNECTED;
import static android.bluetooth.BluetoothProfile.STATE_DISCONNECTED;
import static android.content.Context.ALARM_SERVICE;
import static android.content.Context.POWER_SERVICE;
import static java.lang.System.arraycopy;
import static java.util.Arrays.copyOfRange;
import static java.util.Objects.isNull;
import static tk.glucodata.Applic.app;
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.DexGattCallback.setalarm;
import static tk.glucodata.Libre2GattCallback.showCharacter;
import static tk.glucodata.Log.doLog;
import static tk.glucodata.LossOfSensorAlarm.cancelalarm;
import static tk.glucodata.Natives.endcrypt;
import static tk.glucodata.Natives.initcrypt;
import static tk.glucodata.Natives.intDecrypt;
import static tk.glucodata.Natives.intEncrypt;
import static tk.glucodata.Log.showbytes;
import static tk.glucodata.util.sleep;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;


public class Libre3GattCallback extends SuperGattCallback {
    static final private boolean doTEST=false; //TODO
    static private final String LOG_ID = "Libre3GattCallback";
    private boolean shouldenablegattCharCommandResponse = false;
    private boolean isServicesDiscovered = false;
    private final long sensorptr;
    private long securityContext=0L;
private final Queue<byte[]> sendqueue = new ConcurrentLinkedQueue<byte[]>();
private int    lastEventReceived=0;
    private BluetoothGattCharacteristic gattCharPatchDataControl = null;
    private BluetoothGattCharacteristic gattCharPatchStatus = null;
    private BluetoothGattCharacteristic gattCharEventLog = null; //TODO:remove?
    private BluetoothGattCharacteristic gattCharGlucoseData = null;
    private BluetoothGattCharacteristic gattCharHistoricData = null;
    private BluetoothGattCharacteristic gattCharClinicalData = null;
    private BluetoothGattCharacteristic gattCharFactoryData = null;
    private BluetoothGattCharacteristic gattCharCommandResponse = null;
    private BluetoothGattCharacteristic gattCharChallengeData = null;
    private BluetoothGattCharacteristic gattCharCertificateData = null;
private  final void info(String in) {
    {if(doLog) {Log.i(LOG_ID,SerialNumber +": "+ in);};};
    }
@Override
void free() {
    // Garmin handoff now uses only saved sensor data. Normal destruction must
    // cancel pending command retries before releasing this callback's handles.
    stop=true;
    connected=false;
    cancelretrytimer();
    mActiveBluetoothDevice=null;
    super.free();
    {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"free");};};
    cancelalarm();
    var security=securityContext;
    securityContext=0L;
    Natives.libre3FreeSecurityContext(security);
    var tmp=cryptptr;
    cryptptr=0L;
    endcrypt(tmp);
    }
    public Libre3GattCallback(String SerialNumber, long dataptr)  {
        super(SerialNumber,dataptr,3);
        {if(doLog) {Log.format(LOG_ID+" "+ SerialNumber + ": "+ "Libre3GattCallback(0x%x)\n",dataptr);};};
        sensorptr = Natives.getsensorptr(dataptr);

        if(Thread.currentThread().equals( Looper.getMainLooper().getThread() )) {
            var thr=new Thread(()-> init());
            thr.start();
            try {
                thr.join();
            } catch(Throwable th) {
                Log.stack(LOG_ID, SerialNumber + ": "+"init",th);
            }
            }
        else
            init();
    }
private final void checkBluetoothGatt(BluetoothGatt bluetoothGatt) {
    if(doLog) {
        if(bluetoothGatt!=mBluetoothGatt) {
            {if(doLog) {Log.i(LOG_ID,SerialNumber+" bluetoothGatt!=mBluetoothGatt"+(bluetoothGatt==null?" bluetoothGatt==null":(mBluetoothGatt==null?" mBluetoothGatt==null":"")));};};
            }
        }
    }


private static boolean requestLeConnectionUpdateHidden(
        BluetoothGatt gatt,
        int minInterval,
        int maxInterval,
        int latency,
        int timeout,
        int minConnEventLen,
        int maxConnEventLen) {
    try {
        Method m = BluetoothGatt.class.getDeclaredMethod(
                "requestLeConnectionUpdate",
                int.class, int.class, int.class, int.class, int.class, int.class);
        m.setAccessible(true);
        Object r = m.invoke( gatt, minInterval, maxInterval, latency, timeout, minConnEventLen, maxConnEventLen);
        boolean ret= (r instanceof Boolean) && (Boolean) r;
        Log.i(LOG_ID,"requestLeConnectionUpdate="+ret);
        return ret;
    } catch (Throwable t) {
        Log.stack(LOG_ID, "requestLeConnectionUpdate failed", t);
        return false;
    }
}
@SuppressWarnings("unused")
@Keep
public void onConnectionUpdated(BluetoothGatt gatt, int interval, int latency, int timeout, int status) {
        {if(doLog) {Log.i(LOG_ID, "onConnectionUpdated interval=" + interval + " latency=" + latency + " timeout=" + timeout + " status=" + status);};};
        /*
        if(isWearable) {
            if(interval==12) 
                requestLeConnectionUpdateHidden( gatt, 6, 6, 0, 500, 0, 0);
            else {
                if(interval==6)
                    requestLeConnectionUpdateHidden( gatt, 24, 24, 0, 500, 0, 0);
                if(interval>300) {
    //                requestLeConnectionUpdateHidden(gatt,315, 315, 4, 600, 0, 0);
    //                requestLeConnectionUpdateHidden(gatt, 300, 300, 4, 600, 0, 0);
                    requestLeConnectionUpdateHidden( gatt, 300, 300, 6, 600, 0, 0);
                    }
                 }
            }
            */
      }

@SuppressWarnings("unused")
@Keep
    public void onSubrateChange( @NonNull BluetoothGatt gatt,  int subrateMode,  int status) {
     if(doLog) {
        Log.i(LOG_ID,"onSubrateChange  subrateMode="+subrateMode+" status="+status);
        }
    }


    @Override 
    public void onCharacteristicRead( @NonNull BluetoothGatt gatt, @NonNull BluetoothGattCharacteristic characteristic, @NonNull byte[] value, int status) {
            checkBluetoothGatt(gatt);
            if(doLog)
                showbytes(LOG_ID + " "+SerialNumber+" onCharacteristicRead status="+status+" " + characteristic.getUuid().toString(), value);

    }


    @Override 
    public void onCharacteristicRead(BluetoothGatt bluetoothGatt, BluetoothGattCharacteristic bluetoothGattCharacteristic, int status) {
        checkBluetoothGatt(bluetoothGatt);
        if(doLog)
            {showbytes(LOG_ID + " "+SerialNumber+" onCharacteristicRead status="+status+" " + bluetoothGattCharacteristic.getUuid().toString(), bluetoothGattCharacteristic.getValue());}

       /* if(bluetoothGattCharacteristic.getUuid().equals(LIBRE3_CHAR_PATCH_STATUS)) {
            } */
    }

    @Override 
    public void onCharacteristicWrite(BluetoothGatt bluetoothGatt, BluetoothGattCharacteristic bluetoothGattCharacteristic, int i2) {
        checkBluetoothGatt(bluetoothGatt);
        if(doLog)
            showCharacter(LOG_ID + " "+SerialNumber+" onCharacteristicWrite " , bluetoothGattCharacteristic);

        oncharwrite(bluetoothGattCharacteristic);
//        var value = bluetoothGattCharacteristic.getValue();
 //       {if(doLog){showbytes(LOG_ID + " "+SerialNumber+" onCharacteristicWrite " + bluetoothGattCharacteristic.getUuid().toString(), value);};}
    }

//    private boolean wasConnected = false;
private boolean connected=false;

//private    boolean waitingForMtu=false;
//private int updated=0;
    @Override
    protected long connectionAttemptTimeoutMillis() {
        // Recovery policy: retire a silent connection attempt after two minutes.
        return 120_000L;
    }

    @SuppressLint("MissingPermission")
    @Override 
    public void onConnectionStateChange(BluetoothGatt bluetoothGatt, int status, int newState) {
        if(!acceptConnectionStateChange(bluetoothGatt,newState))
            return;


        if(stop) {
            {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"onConnectionStateChange stop==true");};};
            return;
            }
        if(doLog) {
             checkBluetoothGatt(bluetoothGatt);
                        String[] state = {"DISCONNECTED", "CONNECTING", "CONNECTED", "DISCONNECTING"};
                        {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+ " onConnectionStateChange, status:" + status + ", state: " + (newState < state.length ? state[newState] : newState));};};
                        }
         long tim = System.currentTimeMillis();
        if(newState == STATE_CONNECTED) {
            //resetGlucose=0; 
           // updated=0;
            connected=true;
            setpriority(bluetoothGatt);
            /*
            if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                bluetoothGatt.setPreferredPhy(PHY_LE_1M_MASK, PHY_LE_1M_MASK, PHY_OPTION_NO_PREFERRED);
            } */
            constatchange[0] = tim;
            //wasConnected = true;
            /*
          if (isWearable) {
                    waitingForMtu = bluetoothGatt.requestMtu(517);
                    Log.i(LOG_ID, SerialNumber + " requestMtu(517)=" + waitingForMtu);
                    if (waitingForMtu)
                        return;
                }
                */

            startServices(bluetoothGatt);
            } else if (newState == STATE_DISCONNECTED) {

//                cancelrefreshalarm();
                connected=false;
                cancelretrytimer();
                Log.e(LOG_ID, SerialNumber + ": "+ "onConnectionStateChange ERROR: disconnected with status : " + status);
               // libre3BLESensor.access$600(libre3BLESensor.this, status);
            constatchange[1] = tim;
            setConStatus(status);
            if(lastphase5) {
                if(status==19) {
                    if((tim-datatime)>=59000) {
                        isPreAuthorized=false;
                        Natives.setLibre3kAuth(sensorptr,null);
                        }
                     }
                }  
            if(!stop)  {
                 realdisconnected(bluetoothGatt,status,tim);
                 }
            else {
                if(!closeCurrentGatt(bluetoothGatt))
                    return;
                }
            }
        }

        @Override 
        public void onDescriptorRead(BluetoothGatt bluetoothGatt, BluetoothGattDescriptor bluetoothGattDescriptor, int status) {
        {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+ "onDescriptorRead status="+status);};};
        }


private void startServices(BluetoothGatt mBluetoothGatt) {
            if (!isServicesDiscovered||!getservices()) {
                if(!mBluetoothGatt.discoverServices()) {
                          Log.e(LOG_ID, SerialNumber + ": "+"discoverServices()  failed");
                        }
                else {
                    {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"discoverServices() success");};};
                    }
                }

             }

        @Override 
        public void onDescriptorWrite(BluetoothGatt bluetoothGatt, BluetoothGattDescriptor bluetoothGattDescriptor, int status) {
        checkBluetoothGatt(bluetoothGatt);
           // libre3BLESensor.access$1900(libre3blesensor, characteristic, status);
        {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+ "onDescriptorWrite status="+status);};};
        BluetoothGattCharacteristic characteristic = bluetoothGattDescriptor.getCharacteristic();
            handleonDescriptorWrite(characteristic);
        }

        @Override // android.bluetooth.BluetoothGattCallback
        public void onMtuChanged(BluetoothGatt bluetoothGatt, int mtu, int status) {
        {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"onMtuChanged mtu="+mtu+" status="+status);};};
        /*
        if(isWearable) {
             if(waitingForMtu) {
                waitingForMtu=false;
                startServices(bluetoothGatt);
                }
             }
             */
        }

        @Override // android.bluetooth.BluetoothGattCallback
        public void onReadRemoteRssi(BluetoothGatt bluetoothGatt, int rssi, int status) {
            if (status != GATT_SUCCESS) {
                Log.e(LOG_ID, SerialNumber + ": "+ "Error reading RSSI, error " + status);
                rssi = 999;
            }
        readrssi=rssi;
        if(shouldenablegattCharCommandResponse) {
            Log.i(LOG_ID,"onReadRemoteRssi "+rssi+" ablegattCharCommandResponse");
            checkBluetoothGatt(bluetoothGatt);
            enablegattCharCommandResponse();
            shouldenablegattCharCommandResponse=false;
            }
        else {
            Log.i(LOG_ID,"onReadRemoteRssi "+rssi+" not ablegattCharCommandResponse");
            }
        }

        @Override // android.bluetooth.BluetoothGattCallback
     public void onServicesDiscovered(BluetoothGatt bluetoothGatt, int status) {
      checkBluetoothGatt(bluetoothGatt);
          {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+ "onServicesDiscovered status="+status);};};
          if (status == GATT_SUCCESS) {
                if(!getservices()) {
                  dodisconnect(bluetoothGatt);
                  disconnected(status);
                  }
              }
             else {
                Log.e(LOG_ID, SerialNumber + ": "+ "BLE: onServicesDiscovered error: " + status);
               dodisconnect(bluetoothGatt);
               disconnected(status);
            }
        }

private   int rdtBytes =0;
private        int rdtSequence = 0;
private  int rdtLength =0;
private    byte[] rdtData;
    int getsecdata(byte[] value) {
        if (value.length < 1) {
        var message="getsecdata unknown command length=" + value.length;
            Log.e( LOG_ID, SerialNumber + ": "+ message);
        setfailure(message);
        dodisconnect(mBluetoothGatt);
            return rdtLength;
        }
        int i2 = value[0] & 0xFF;
        if (i2 != rdtSequence + 1) {
            var message= "getsecdata secu Sequence=" + i2 + "!=" + rdtSequence + "-1 (rdtSequence-1)";
            Log.e( LOG_ID, SerialNumber + ": "+ message);
        setfailure(message);
        dodisconnect(mBluetoothGatt);
            return rdtLength;
        }
        info("getsecdata num=" + i2 + " rdtSequence=" + rdtSequence);
        int length = value.length - 1;
        arraycopy(value, 1, rdtData, rdtBytes, length);
        int i3 = rdtBytes + length;
        rdtBytes = i3;
        rdtSequence = i2;
        return rdtLength - i3;
    }
private final byte[] r1=new byte[16];
private final byte[] r2=new byte[16];
private final byte[] nonce1=new byte[7];

private  void    randomr2() {
      Random.fillbytes(r2);
    }
private void setr1none(byte[] rdtData) {
    {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"setr1none");};};
    arraycopy(rdtData,0,r1,0,16);
    arraycopy(rdtData,16,nonce1,0,7);
    randomr2();
    mknonceback();
    }
private byte[] wrtData;
private int wrtOffset;
private void mknonceback() {
    {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"mknonceback");};};
    byte[] uit=new byte[36];
    arraycopy(r1,0,uit,0,16);
    arraycopy(r2,0,uit,16,16);
    byte[] pin=Natives.getpin(sensorptr);
    arraycopy(pin,0,uit,32,4);



var encrypted = Natives.libre3EncryptChallengeReply(securityContext,nonce1,uit);






    wrtData=encrypted;
    wrtOffset=0;
    writedata(gattCharChallengeData);


    }
private long cryptptr=0L;
private void challenge67() {
    {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"challenge67()");};};
    byte[] first=new byte[60];
    byte[] nonce=new byte[7];
    arraycopy(rdtData,0,first,0,60);
    arraycopy(rdtData,60,nonce,0,7);
    byte[] decr=Natives.libre3DecryptChallengeResponse(securityContext,nonce,first);
    Log.showbytes("challenge67 decr",decr);
    var backr2=copyOfRange(decr,0,16);
    if(!java.util.Arrays.equals(r2,backr2)) {
        {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"r2!=backr2");};};
        dodisconnect(mBluetoothGatt); //TODO: or try again?
        return;
        }
    var backr1=copyOfRange(decr,16,32);
    if(!java.util.Arrays.equals(r1,backr1)) {
        {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"r1!=backr1");};};
        dodisconnect(mBluetoothGatt); //TODO: or try again?
        return;
        }
    var kEnc=copyOfRange(decr,32,48);
    var ivEnc=copyOfRange(decr,48,56);
//    byte[] AuthKey=KEYSCrypto.exportAuthorizationKey();
    byte[] savedAuthorization=Natives.libre3ExportSavedAuthorization(securityContext);
    Log.showbytes("challenge67 savedAuthorization",savedAuthorization);
    //securityContext=new BCrypt(kEnc,ivEnc);
    cryptptr=initcrypt(cryptptr,kEnc,ivEnc);
    Natives.setLibre3kAuth(sensorptr,savedAuthorization);
    // A newly scanned sensor can now be provisioned without retaining this GATT.
    SensorLifecycle.changed();
    enableNotification(mBluetoothGatt,gattCharPatchDataControl);
    }



/**
 * Returns the minimum state needed by the direct-BLE Garmin app after this
 * connection has completed Libre 3 authorization. Layout: PIN[4] followed by
 * the 176-byte expanded challenge context. The returned buffer is a new copy.
 *
 * The security context is also reconstructed from the saved KAuth during
 * init().  Do not require a live GATT/data cipher here: that would make the
 * saved authorization unusable precisely when the phone cannot reconnect and
 * the user wants to hand ownership to Garmin.
 */
public synchronized byte[] getGarminProvisioningSecret() {
    if(!isWearable) {
        byte[] context=securityContext==0L ? null :
                Natives.libre3ExportChallengeContext(securityContext);
        if(context==null || context.length!=176) {
            // Normally init() has already imported this record. Retry explicitly
            // for callbacks constructed while Bluetooth/native setup was changing.
            byte[] savedAuthorization=Natives.getLibre3kAuth(sensorptr);
            if(savedAuthorization==null || connected ||
                    !initSecurityKeys(savedAuthorization,1))
                return null;
            context=Natives.libre3ExportChallengeContext(securityContext);
            }
        byte[] pin=Natives.getpin(sensorptr);
        if(context==null || context.length!=176 || pin==null || pin.length!=4)
            return null;
        byte[] out=new byte[180];
        arraycopy(pin,0,out,0,4);
        arraycopy(context,0,out,4,176);
        return out;
        }
    return null;
    }

/** Reconstruct provisioning from the saved authorization without creating a
 * Bluetooth callback. Both temporary native handles are freed on every path. */
public static byte[] getSavedGarminProvisioningSecret(String serial) {
if(!isWearable) {
    long dataptr=0L, security=0L;
    try {
        dataptr=Natives.getdataptr(serial);
        if(dataptr==0L || Natives.getLibreVersion(dataptr)!=3) return null;
        long sensorptr=Natives.getsensorptr(dataptr);
        if(sensorptr==0L || !Natives.activeSensor(sensorptr)) return null;
        byte[] saved=Natives.getLibre3kAuth(sensorptr), pin=Natives.getpin(sensorptr);
        if(saved==null || pin==null || pin.length!=4) return null;
        security=Natives.libre3BeginSecurityHandshake(0L);
        if(security==0L || Natives.libre3SelectAppKeyAndSavedAuthorization(security,1,saved)!=1)
            return null;
        byte[] context=Natives.libre3ExportChallengeContext(security);
        if(context==null || context.length!=176) return null;
        byte[] secret=new byte[180];
        arraycopy(pin,0,secret,0,4);
        arraycopy(context,0,secret,4,176);
        return secret;
    } finally {
        if(security!=0L) Natives.libre3FreeSecurityContext(security);
        if(dataptr!=0L) Natives.freedataptr(dataptr);
    }
    }
  return null;
}
    

/** Switch this already-authorized Libre 3 sensor to the dedicated Garmin app. */
//public boolean switchToGarmin() { return GarminLibre3.switchSensor(this); }

/** Stop this callback owning/reconnecting the sensor after Garmin accepted it. */
public void stopForGarmin() {
    if(!isWearable) {
        stop=true;
        connected=false;
        cancelretrytimer();
        // Also defeats a connectDevice Runnable that may already have been queued.
        mActiveBluetoothDevice=null;
        close();
        }
    }

@Override
public boolean reconnect(long now,long delay) {
    return stop || super.reconnect(now,delay);
    }

@Override
public boolean connectDevice(long delayMillis) {
    return stop || super.connectDevice(delayMillis);
    }




private void receivedCHALLENGE_DATA() {
    switch(rdtLength) {
        case 23: setr1none(rdtData); break;
        case 67: challenge67();break;
        default: {
            var message="receivedCHALLENGE_DATA unknown length="+rdtLength;
             dodisconnect(mBluetoothGatt);
            {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+message);};};
            setfailure(message);
            }

        }
    }
/*
Waarschijnlijk wordt er ook iets opgeslagen
*/

//Libre3SKBCryptoLib cryptoLib;
//BluetoothGattCharacteristic gattCharCommandResponse = null;
//boolean sendSecurityCommand(1,null) after com.adc.trident.app.frameworks.mobileservices.libre3.security.Libre3SKBCryptoLib::initKEYS=1
private boolean sendSecurityCommand(int b) {
        return sendSecurityCommand((byte)b);
    }
@SuppressLint("MissingPermission")
private boolean sendSecurityCommand(byte b) {
    {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"sendSecurityCommand "+b);};};
    byte[] com={(byte)b};
    if(!gattCharCommandResponse.setValue(com) ) {
        var message="gattCharCommandResponse.setValue("+b+") failed";
        Log.e(LOG_ID, SerialNumber + ": "+message);
        setfailure(message);  
        dodisconnect(mBluetoothGatt); 
        return false;
        }
    /*
    synchronized(syncObject) {
        isNotificationSuspended=true;
        } */
    if(!mBluetoothGatt.writeCharacteristic(gattCharCommandResponse)) {
        var message="writeCharacteristic(gattCharCommandResponse) failed "+b;
        Log.e(LOG_ID, SerialNumber + ": "+ message);
        setfailure(message);  
        dodisconnect(mBluetoothGatt); //TODO: or try again?
        return false;
        }
    return true; 
    }
private int commandphase=1;
private void setCertificate140() {
    {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"setCertificate140");};};
    cryptolib.setPatchCertificate(securityContext,rdtData);
    if(sendSecurityCommand( (byte)0x0D)) {
        commandphase=4;
        }
    }
private boolean    generateKAuth(byte[] input) {
    {if(doLog){showbytes(LOG_ID+ " "+SerialNumber +" generateKAuth",input);};}
    //Saves something?
    return Natives.libre3DeriveAuthorizationRoot(securityContext,input)==1;
    }
private boolean setCertificate65() {
    {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"setCertificate65");};};
    byte[]    patchEphemeral=rdtData;
    if(generateKAuth(patchEphemeral)) //TODO failure?
        return sendSecurityCommand((byte)17);
    var message= "generateKAuth(patchEphemeral) failed";
    Log.e(LOG_ID, SerialNumber + ": "+ message);
    setfailure(message);  
    dodisconnect(mBluetoothGatt); 
    return false;
    }
private void receivedCERT_DATA() {
    switch(rdtLength) {
        case 140: setCertificate140();break;
        case 65: setCertificate65();break;
        default: {
            var message="receivedCERT_DATA unknown length="+rdtLength;
            {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+message);};};
            setfailure(message);  
            dodisconnect(mBluetoothGatt); 
            }
        };
    }
final private boolean notsuspended=true;
  void enablegattCharCommandResponse() {
      if(notsuspended) {
        {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"enablegattCharCommandResponse");};};
         enableNotification(mBluetoothGatt,gattCharCommandResponse);
         }
  }
//19156 00000 00013 01036 19156 00019






private    void save_history(byte[] value) {
    byte[] olddec=intDecrypt(cryptptr,4, value);
        Natives.saveLibre3History(this.sensorptr, olddec);
    }
@Override 
public void onCharacteristicChanged(BluetoothGatt bluetoothGatt, BluetoothGattCharacteristic bluetoothGattCharacteristic) {
    if(doLog)
        checkBluetoothGatt(bluetoothGatt);
    onCharacteristicChanged(bluetoothGatt, bluetoothGattCharacteristic, bluetoothGattCharacteristic.getValue());
    }
static final private String charglucosedata= "CHAR_GLUCOSE_DATA".intern();
        @SuppressLint("MissingPermission")
//        @Override 
//static int final usewakelock=false;
private  void logcharacter(UUID uuid,String str,byte[] value) {
        final long timmsec = System.currentTimeMillis();
       if(str!=charglucosedata) setsuccess(timmsec,str);
       if(doLog){showbytes(LOG_ID+ " "+SerialNumber +" onCharacteristicChanged  "+uuid.toString()+" "+str, value);};
       }

@Override 
public void onCharacteristicChanged(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, byte[] value) {
       final long nowmsec= System.currentTimeMillis();
       var wakelock=    Applic.usewakelock?(((PowerManager) app.getSystemService(POWER_SERVICE)).newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "Juggluco::Libre3")):null;
       if(wakelock!=null)
           wakelock.acquire();

            UUID uuid = characteristic.getUuid();
//      {if(doLog){      showbytes(LOG_ID+" onCharacteristicChanged Start "+uuid.toString(), value);};}
            if(uuid.equals(LIBRE3_CHAR_GLUCOSE_DATA)) {
                logcharacter(uuid,charglucosedata,value);
                glucose_data(value,nowmsec);
            } else if (uuid.equals(LIBRE3_CHAR_PATCH_STATUS)) {
                logcharacter(uuid,"CHAR_PATCH_STATUS",value);
                receivedpatchstatus(value);
            } else if(uuid.equals(LIBRE3_CHAR_HISTORIC_DATA)) {
                logcharacter(uuid,"CHAR_HISTORIC_DATA",value);
                save_history(value);
            } else if(uuid.equals(LIBRE3_CHAR_PATCH_CONTROL)) {
                logcharacter(uuid,"CHAR_PATCH_CONTROL",value);
                access1100(value);
            } else if(uuid.equals(LIBRE3_SEC_CHAR_CERT_DATA)) {
                logcharacter(uuid,"SEC_CHAR_CERT_DATA",value);
                if(getsecdata(value) <= 0) {
                   receivedCERT_DATA();
                }
            } else if (uuid.equals(LIBRE3_SEC_CHAR_CHALLENGE_DATA)) {
                logcharacter(uuid,"SEC_CHAR_CHALLENGE_DATA",value);
                if(getsecdata(value) <= 0) {
                    receivedCHALLENGE_DATA();
                }
            } else if (uuid.equals(LIBRE3_SEC_CHAR_COMMAND_RESPONSE)) {
                logcharacter(uuid,"SEC_CHAR_COMMAND_RESPONSE",value);
        lastphase5=false;
                preparedata(value);
            } else if (uuid.equals(LIBRE3_CHAR_EVENT_LOG)) {
                logcharacter(uuid,"CHAR_EVENT_LOG",value);
        logevent(value);
            } else if (uuid.equals(LIBRE3_CHAR_FACTORY_DATA)) {
                logcharacter(uuid,"CHAR_FACTORY_DATA",value);
            } else if (uuid.equals(LIBRE3_CHAR_CLINICAL_DATA)) {
                logcharacter(uuid,"CHAR_CLINICAL_DATA",value);
                fast_data(value);
            } else {
                logcharacter(uuid,"Unknown",value);
                dodisconnect(mBluetoothGatt);
                disconnected(1042);
                }
       if(wakelock!=null)
        wakelock.release();
    {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"onCharacteristicChanged end");};};
        }


//source /n/ojka/tmp/libre3.3.0/sensor/newsensor/working


private    void fast_data(byte[] encryp) {
    byte[] decr=intDecrypt(cryptptr,5,encryp);
        if (decr == null) {
            info("fast_data decrypt went wrong"); 
            dodisconnect(mBluetoothGatt); 
        } else {
            Natives.saveLibre3fastData(sensorptr, decr);
        }
    }

private final KEYSCrypto cryptolib=new KEYSCrypto();
//int    securityState=0;
private boolean    isPreAuthorized=false;
private void onConnectGatt() {
    isPreAuthorized=false;
    }
private synchronized boolean initSecurityKeys(byte[] savedAuthorization,int level) {
    long context=Natives.libre3BeginSecurityHandshake(securityContext);
    if(context==0L) {
        securityContext=0L;
        return false;
        }
    securityContext=context;
    return cryptolib.initKEYS(securityContext,savedAuthorization,level);
    }
private void handleMSLibre3SecurityNotificationsEnabledEvent() {
    {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"handleMSLibre3SecurityNotificationsEnabledEvent");};};
    if(isPreAuthorized) {
        //securityState=2;
        sendSecurityCommand(17);
        }
    else {
        var exportedKAuth = Natives.getLibre3kAuth(sensorptr);
        if(initSecurityKeys(exportedKAuth,1)) {
            if(exportedKAuth==null) {
                {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"exportedKAuth==null");};};
                sendSecurityCommand(1);
                commandphase=1;
                }
            else  {
                {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"exportedKAuth!=null");};};
                isPreAuthorized=true;
                sendSecurityCommand(17);
                }
            }
        }

    }
private void logevent(byte[] value) {
    byte[] decr=intDecrypt(cryptptr,6,value);
    int last=Natives.libre3EventLog(sensorptr,decr);
    if(last<0)
            return;
    lastEventReceived=last;
    }
private void init() {
    var exportedKAuth = Natives.getLibre3kAuth(sensorptr);
    if(!isPreAuthorized) {
        if(exportedKAuth!=null) {
            if(initSecurityKeys(exportedKAuth,1)) {
                isPreAuthorized=true;
                commandphase = 5;
                }
            else
                isPreAuthorized=false;
            }
        else
            isPreAuthorized=false;
        }


  }

//private    boolean sendEphemeralKeys=false;
@SuppressLint("MissingPermission")
private PendingIntent onalarm=null;

private void realdisconnected(BluetoothGatt bluetoothGatt,int status,long tim) {
    {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"disconnected "+status);};};
    oneMinuteReadingSize=0;
    backFillInProgress=false;
    shouldenablegattCharCommandResponse=false;
    isServicesDiscovered=false;
    init();
    wrotecharacter=false;
    sendqueue.clear();
//    if(autoconnect&&status!=19) 
    if(autoconnect) {
        watchConnectionAttempt(bluetoothGatt);
        bluetoothGatt.connect();
        return;
        }
    else {
        if(!closeCurrentGatt(bluetoothGatt))
            return;
        if(isWearable&&Natives.getDisconnectSensor()) {
            final long alreadywaited = tim - datatime;
            final long mmsectimebetween = 60 * 1000;
            long stillwait = mmsectimebetween - alreadywaited - 55000;
            if(doLog) {Log.i(LOG_ID, "alreadywaited=" + alreadywaited + " stillwait=" + stillwait);};
            if(stillwait>0)
                onalarm=setalarm(tim+stillwait,onalarm,SerialNumber);
             else
                connectDevice(0);
             }
        else
            connectDevice(0);
        }
    }

private final void dodisconnect(BluetoothGatt bluetoothGatt) {
    Log.e(LOG_ID, SerialNumber + ": "+"disconnect()");
    bluetoothGatt.disconnect();
    }
private void disconnected(int status) {
    {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"disconnected("+status+")");};};
    }

private  void  setsuccess(long timmsec,String str) {
    wrotepass[0]=timmsec;
    handshake =str;
    }
private  void  setfailure(String str) {
    wrotepass[1]= System.currentTimeMillis();
    handshake =str;
    }

       /*
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
private int resetGlucose=0;
private void resetGlucoseCharacter(BluetoothGatt bluetoothGatt) {
        if(isNull(bluetoothGatt)) {
            return;
            }
        final var  characteristic=gattCharGlucoseData;
        resetGlucose=1;
        tryer(()-> disableNoCheck(bluetoothGatt, characteristic));
        } */
private void handleonDescriptorWrite(BluetoothGattCharacteristic characteristic) {
        final var uuid = characteristic.getUuid();
    String struuid=uuid.toString();
    {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"handleonDescriptorWrite "+struuid);};};
     long timmsec = System.currentTimeMillis();
     setsuccess(timmsec,struuid);
  /*  if(false) {
        }
    else { */
        if(LIBRE3_CHAR_PATCH_CONTROL.equals(uuid)) {
            enableNotification(mBluetoothGatt, gattCharEventLog);
        } else {
            if (LIBRE3_CHAR_EVENT_LOG.equals(uuid)) {
                enableNotification(mBluetoothGatt, gattCharHistoricData);
            } else {
                if (LIBRE3_CHAR_HISTORIC_DATA.equals(uuid)) {
                    asknotification(gattCharClinicalData);
                } else {
                    if (LIBRE3_CHAR_CLINICAL_DATA.equals(uuid)) {
                        asknotification(gattCharFactoryData);
                    } else {
                        if (LIBRE3_CHAR_FACTORY_DATA.equals(uuid)) {
                            asknotification(gattCharGlucoseData);
                        } else {
                            if (LIBRE3_CHAR_GLUCOSE_DATA.equals(uuid)) {
                                asknotification(gattCharPatchStatus);
                            /*
                               switch(resetGlucose) {
                                case 0: asknotification(gattCharPatchStatus);break;
                                case 1: asknotification(gattCharGlucoseData);++resetGlucose;break;
                                default: resetGlucose=0; break;
                                };
                                */
                            } else {
                                if (LIBRE3_CHAR_PATCH_STATUS.equals(uuid)) {
                                } else {
                                    if (LIBRE3_SEC_CHAR_COMMAND_RESPONSE.equals(uuid)) {
                                        enableNotification(mBluetoothGatt, gattCharCertificateData);
                                        //asknotification(gattCharCertificateData);


                                    } else {
                                        if (LIBRE3_SEC_CHAR_CERT_DATA.equals(uuid)) {
                                            enableNotification(mBluetoothGatt, gattCharChallengeData);
                                            //asknotification(gattCharChallengeData);


                                        } else {
                                            if(LIBRE3_SEC_CHAR_CHALLENGE_DATA.equals(uuid)) {
                                              //  sendevent(new com.adc.trident.app.frameworks.mobileservices.libre3.events.MSLibre3SecurityNotificationsEnabledEvent());

                    handleMSLibre3SecurityNotificationsEnabledEvent() ;


                                            }

                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
/*
private ByteArrayOutputStream factoryData=new ByteArrayOutputStream();
void access1700(byte[] value) {
        byte[] decr=intDecrypt(cryptptr,7,value);
    {if(doLog){showbytes(LOG_ID+" access1700",decr);};}
        factoryData.write(decr,1,decr.length-1);
        }
*/
private void access1100(byte[] value) {
    byte[] decr=intDecrypt(cryptptr,1,value); //USED for what??
    if(doLog){showbytes(LOG_ID+" "+ SerialNumber +" access1100",decr);};
    if(decr[0]==1)
        Applic.app.redraw();
//    gattCharPatchDataControl.setValue(decr);//Slaat nergens op TODO: remove
/*
        switch(currentControlCommand) {
            case 1: {
                lastHistoricLifeCountReceived=backFillStartHistoricLifeCount;
                };break;
            case 2: {
                lastLifeCountReceived=backFillStartLifeCount;
                     };break;
            }; */
    backFillInProgress=false;
    wrotecharacter=false;
    if(sendqueue.isEmpty()) {
        Log.i(LOG_ID,"access1100 !fromqueue");
       if(isWearable) {
            if(Natives.getDisconnectSensor())
                disconnect();  
            }
/*          if(isWearable) {
                  mBluetoothGatt.requestConnectionPriority(balanced?CONNECTION_PRIORITY_BALANCED:CONNECTION_PRIORITY_HIGH);
                  balanced=!balanced;
                  } */
//        mBluetoothGatt.connect();

//           if(isWearable) resetGlucoseCharacter(mBluetoothGatt);
        }
    else {
        fromqueue();
        }
    }

private    void preparedata(byte[] value) {
        {if(doLog){showbytes(LOG_ID+ " "+SerialNumber +" preparedata",value);};}
//        MSLibre3Event mSLibre3Event;
        int i2 = value[0] & 0xFF;
        if (value.length == 1) {
            if (i2 == 4) {
                info("preparedata sig=" + i2 + " MSLibre3CertificateAcceptedEvent");
//                sendevent(new MSLibre3CertificateAcceptedEvent());
         sendSecurityCommand(9);
                return;
            }
            info("preparedata unimplemented " + i2);
        dodisconnect(mBluetoothGatt);
       disconnected(9788);
            return;
        }
        int i3 = value[1] & 0xFF;
        info("preparedata sig=" + i2 + " num=" + i3);
        this.rdtLength = i3;
        this.rdtData = new byte[i3];
        this.rdtSequence = -1;
        this.rdtBytes = 0;
        if (i2 == 8) {
           // mSLibre3Event = new MSLibre3ChallengeLoadDoneEvent(); 
        //nothing
        } else if (i2 == 10) {
           // mSLibre3Event = new MSLibre3CertificateReadyEvent();
       //nothing
        } else if (i2 == 15) {
           // mSLibre3Event = new MSLibre3EphemeralReadyEvent();
       //nothing
        } else {
            info("prepare date unknown sig=" + i2 + " num=" + i3);
        dodisconnect(mBluetoothGatt);
       disconnected(1023);
            return;
        }
//        sendevent(mSLibre3Event);
    }

    @SuppressLint("MissingPermission")
  private  int writedata(BluetoothGattCharacteristic bluetoothGattCharacteristic) {
      if(wrtData==null) {
        Log.e(LOG_ID, SerialNumber + ": "+"writedata wrtData==null"+ bluetoothGattCharacteristic.getUuid().toString());
        dodisconnect(mBluetoothGatt);
       disconnected(1099);
           return 0;
         }
      else  {
          {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"writedata "+ bluetoothGattCharacteristic.getUuid().toString());};};
           }
        
        int length = this.wrtData.length - this.wrtOffset;
        if(length > 0) {
            int min = Math.min(length, 18);
            byte[] bArr = new byte[20];
            System.arraycopy(this.wrtData, this.wrtOffset, bArr, 2, min);
            {if(doLog){showbytes(SerialNumber+" writedata  wrtOffset="+wrtOffset+" length="+min,bArr);};}
            bluetoothGattCharacteristic.setValue(bArr);
            bluetoothGattCharacteristic.setValue(this.wrtOffset, 18, 0);
            this.wrtOffset += min;
            if(this.mBluetoothGatt.writeCharacteristic(bluetoothGattCharacteristic))
            return 1;
    else {
        Log.e(LOG_ID, SerialNumber + ": "+"writeCharacteristic(bluetoothGattCharacteristic) failed");
            dodisconnect(mBluetoothGatt);
        return 0; 
        }
        }
    {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"writedata all written");};};
        return 2;
    }
private int getcomphase() {
    return commandphase;
    }
private  byte[]           generateEphemeralKeys() {

    var evikeys=Natives.libre3CreateEphemeralPublicKey(securityContext);
    if(evikeys==null || evikeys.length!=64) {
        Log.e(LOG_ID, SerialNumber + ": libre3CreateEphemeralPublicKey failed");
        return null;
        }
    var uit=new byte[evikeys.length+1];
    arraycopy(evikeys,0,uit,1,evikeys.length);
    uit[0]=(byte)0x4;
    {if(doLog){showbytes(LOG_ID+ " "+SerialNumber + " generateEphemeralKeys()",uit);};}
    return uit;
    }

private boolean sendSecurityCert(byte[] cert) {
    {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"sendSecurityCert");};};
    wrtOffset=0;
    wrtData    =cert;
    return writedata(gattCharCertificateData)!=0;
    }


private boolean    lastphase5=false;

   private void oncharwrite(BluetoothGattCharacteristic bluetoothGattCharacteristic) {
        UUID uuid = bluetoothGattCharacteristic.getUuid();
        if(uuid.equals(LIBRE3_CHAR_BLE_LOGIN)) {
            info("LIBRE3_CHAR_BLE_LOGIN");
            if (writedata(bluetoothGattCharacteristic)==2) {
              //  sendevent(new MSLibre3BLELoginEvent());
            }
        } else if(uuid.equals(LIBRE3_SEC_CHAR_CERT_DATA)) {
            info("LIBRE3_SEC_CHAR_CERT_DATA");
            if(writedata(bluetoothGattCharacteristic)==2) {
//                sendevent(new MSLibre3CertificateSentEvent());
    if(commandphase == 5)
        sendSecurityCommand(14);
    else
        sendSecurityCommand(3);
            }
        } else if(uuid.equals(LIBRE3_SEC_CHAR_CHALLENGE_DATA)) {
            info("start LIBRE3_SEC_CHAR_CHALLENGE_DATA");
            if (writedata(bluetoothGattCharacteristic)==2) {
            sendSecurityCommand(8);
               // sendevent(new MSLibre3ChallengeDataSentEvent());
                }
        } else if(uuid.equals(LIBRE3_SEC_CHAR_COMMAND_RESPONSE)) {
            info("start LIBRE3_SEC_CHAR_COMMAND_RESPONSE commandphase="+commandphase);
            switch(getcomphase()) {
                case 1:
                    if (sendSecurityCommand(2)) {
                        commandphase = 2;
                    }
                    ;
                    break;
                case 2: {
                    if(sendSecurityCert(cryptolib.getAppCertificate())) { //TODO what with failure?
                        commandphase = 3;
                        }
                    else {
                        Log.e(LOG_ID, SerialNumber + ": "+"sendSecurityCert(cryptolib.getAppCertificate()) failed");
                        //TODO disconnect
                        }

                }
                ;
                break;
                case 3:
                    return;
                case 4: {
                    if (sendSecurityCert(generateEphemeralKeys()))
                        commandphase = 5;
                    else {
                        Log.e(LOG_ID, SerialNumber + ": "+"sendSecurityCert(generateEphemeralKeys()))");
                        //TODO disconnect
                        }
                }
                ;
                break;
                case 5:
                    lastphase5=true;
                    return;
            }

        } else {
//         fromqueue();
            info("oncharwrite else");
        }
        info("oncharwrite end");
    }

    @SuppressLint("MissingPermission")
   private boolean getservices() {
       {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"getservices");};};
        boolean z = true;
        boolean z2 = true;
        for(BluetoothGattService bluetoothGattService : this.mBluetoothGatt.getServices()) {
            if (bluetoothGattService != null) {
                UUID uuid = bluetoothGattService.getUuid();
                Log.i(LOG_ID,"service "+uuid.toString());
                if (z && uuid.equals(LIBRE3_DATA_SERVICE)) {
//                    this.gattServiceADC = bluetoothGattService;
                    this.gattCharPatchDataControl = bluetoothGattService.getCharacteristic(LIBRE3_CHAR_PATCH_CONTROL);
                    this.gattCharPatchStatus = bluetoothGattService.getCharacteristic(LIBRE3_CHAR_PATCH_STATUS);
                    this.gattCharEventLog = bluetoothGattService.getCharacteristic(LIBRE3_CHAR_EVENT_LOG);
                    this.gattCharGlucoseData = bluetoothGattService.getCharacteristic(LIBRE3_CHAR_GLUCOSE_DATA);
                    this.gattCharHistoricData = bluetoothGattService.getCharacteristic(LIBRE3_CHAR_HISTORIC_DATA);
                    this.gattCharClinicalData = bluetoothGattService.getCharacteristic(LIBRE3_CHAR_CLINICAL_DATA);
                    this.gattCharFactoryData = bluetoothGattService.getCharacteristic(LIBRE3_CHAR_FACTORY_DATA);
                    z = false;
                } else if (z2 && uuid.equals(LIBRE3_SECURITY_SERVICE)) {
//                    this.gattServiceSecurity = bluetoothGattService;
                    this.gattCharCommandResponse = bluetoothGattService.getCharacteristic(LIBRE3_SEC_CHAR_COMMAND_RESPONSE);
                    this.gattCharChallengeData = bluetoothGattService.getCharacteristic(LIBRE3_SEC_CHAR_CHALLENGE_DATA);
                    this.gattCharCertificateData = bluetoothGattService.getCharacteristic(LIBRE3_SEC_CHAR_CERT_DATA);
                    z2 = false;
                }
            }
        }
        if (z || z2) {
              {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"getservices failure");};};
        isServicesDiscovered = false;
            return false;
        }
        isServicesDiscovered = true;
        shouldenablegattCharCommandResponse=true;
        this.mBluetoothGatt.readRemoteRssi();
       {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"getservices success");};};
       return true;
    }



    private int oneMinuteReadingSize = 0;
//    private int oneMinutePacketNumber = 0;
    private final byte[] oneMinuteRawData = new byte[35];

    @SuppressLint("MissingPermission")
private long datatime=0L;

/**
 * Feed a Libre3 one-minute packet decrypted by the selected Garmin watch into
 * Juggluco without requiring SensorBluetooth or a Libre3GattCallback instance.
 * getdataptr() supplies a temporary native handle for the persistent sensor;
 * the ordinary native save path and Applic.doglucose() then perform the same
 * storage/current-value processing used for externally received glucose.
 */
public static synchronized boolean saveGarminLibre3Minute(String serial,byte[] decr,long timmsec) {
    if(serial==null || decr==null || decr.length!=29 || timmsec<=0L) {
        Log.e(LOG_ID,"bad Garmin Libre3 minute");
        return false;
        }
    long dataptr=0L;
    try {
        dataptr=Natives.getdataptr(serial);
        if(dataptr==0L) {
            Log.e(LOG_ID,serial+": no dataptr for Garmin Libre3 minute");
            return false;
            }
        long sensorptr=Natives.getsensorptr(dataptr);
        if(sensorptr==0L) {
            Log.e(LOG_ID,serial+": no sensorptr for Garmin Libre3 minute");
            return false;
            }
        long res=Natives.saveLibre3MinuteL(sensorptr,decr,timmsec);
        int glumgL=(int)(res&0xFFFFFFFFL);
        if(glumgL!=0) {
            int alarm=(int)((res>>48)&0xFFL);
            short ratein=(short)((res>>32)&0xFFFFL);
            float rate=ratein/1000.0f;
            float gl=Applic.unit==1?glumgL/(Applic.mgdLmult*10.0f):glumgL/10.0f;
            long sensorstartmsec=Natives.getSensorStartmsec(dataptr);
            // Garmin Direct itself owns the Bluetooth-off policy. Passing true
            // prevents a delayed backlog packet from disabling Bluetooth again
            // after the user has switched Direct off. Libre3 is sensor generation 3.
            Applic.doglucose(serial,(int)Math.round(glumgL/10.0f),gl,rate,alarm,
                    timmsec,true,sensorstartmsec,sensorptr,3);
            }
        return true;
        }
    catch(Throwable th) {
        Log.stack(LOG_ID,serial+": saving Garmin Libre3 minute",th);
        return false;
        }
    finally {
        if(dataptr!=0L) Natives.freedataptr(dataptr);
        }
    }

/** Historical data must not pass through the live-value alarm/display path.
 * This is the static equivalent of fast_data() after kind-5 decryption. */
public static synchronized boolean saveGarminLibre3Clinical(String serial,byte[] decr) {
    if(serial==null || decr==null || decr.length!=14) return false;
    long dataptr=0L;
    try {
        dataptr=Natives.getdataptr(serial);
        if(dataptr==0L) return false;
        long sensorptr=Natives.getsensorptr(dataptr);
        if(sensorptr==0L) return false;
        // false also means "already present" in this native function. After a
        // valid native call acknowledge duplicates, otherwise a lost ACK would
        // leave the same record at the head of the watch's queue indefinitely.
        Natives.saveLibre3fastData(sensorptr,decr);
        if(Applic.app!=null) Applic.app.redraw();
        return true;
    } catch(Throwable th) {
        Log.stack(LOG_ID,serial+": saving Garmin Libre3 clinical data",th);
        return false;
    } finally { if(dataptr!=0L) Natives.freedataptr(dataptr); }
}

private    void glucose_data(byte[] value,long timmsec) {
        if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"start glucose_data");};
        int len = value.length;

        System.arraycopy(value, 0, this.oneMinuteRawData, this.oneMinuteReadingSize, len);
        oneMinuteReadingSize +=len;
        if(oneMinuteReadingSize >= oneMinuteRawData.length) {
           this.oneMinuteReadingSize = 0;
           byte[] decr = intDecrypt(cryptptr,3, oneMinuteRawData);
           if(decr == null) {
                Log.e(LOG_ID, SerialNumber + ": "+"intDecrypt(cryptptr,3, oneMinuteRawData)==null");
                return;
               }
           long res=Natives.saveLibre3MinuteL(this.sensorptr, decr,timmsec);
           handleGlucoseResult(res,timmsec);
           datatime=timmsec;
           this.mBluetoothGatt.readRemoteRssi();
           }
        if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"end glucose_data");};
    }

private ScheduledFuture<?> retrytimer=null;
private void setretrytimer() {
    if(retrytimer==null) {
        if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"set timer");};
        retrytimer=Applic.scheduler.schedule(()-> { 
            retrytimer=null;
            if(connected) {
                if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"timer went off");};
                fromqueue(); 
                }
            else {
                if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"timer went off NOT connected");};
                }
            }, 20, TimeUnit.MILLISECONDS);
        }
    else
        {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"already timer");};};
    }
private void cancelretrytimer() {
    Log.i(LOG_ID,"cancelretrytimer()");
    var tmp=retrytimer;
    retrytimer=null;
    if(tmp!=null) {
        tmp.cancel(false);
        }
    }
private boolean wrotecharacter=false;
@SuppressLint("MissingPermission")
private boolean qsendcommand(byte[] command) {
    {if(doLog){showbytes(LOG_ID+ " "+SerialNumber +" qsendcommand",command);};}
    onqueue(command);
    if(!wrotecharacter)
        return fromqueue();
    return false;    
    }
private boolean sendcommandonly(byte[] encr) {
    gattCharPatchDataControl.setValue(encr);
    wrotecharacter=true;
    if(mBluetoothGatt.writeCharacteristic(gattCharPatchDataControl)) {
        {if(doLog){showbytes(LOG_ID+ " "+SerialNumber +" qsendcommand written",encr);};}
        return true;
        }
    else  {
        setretrytimer();
        }
    return false;
    }
private void onqueue(byte[] command) {
    {if(doLog){showbytes(LOG_ID+ " "+SerialNumber +" onqueue sizebefore="+sendqueue.size(),command);};}
    byte[] encr= intEncrypt(cryptptr,0,command);
    sendqueue.offer(encr);
    }

private boolean fromqueue() {
    {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"fromqueue size="+sendqueue.size());};};
//    wrotecharacter=false;
//lock
    var com=sendqueue.peek();
    if(com!=null) {
        if(sendcommandonly(com)) {
            sendqueue.poll();
            return true;
            }
        return false;    
        }
//unlock
      return true; 
    }



private boolean backFillInProgress=false;
private void fillHistory(int backFillStartHistoricLifeCount) {
        int lastHistoricLifeCountReceived=Natives.getlastHistoricLifeCountReceived(sensorptr);
        if(backFillStartHistoricLifeCount<=lastHistoricLifeCountReceived) {
            {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"no history needed  lastHistoricLifeCountReceived ("+lastHistoricLifeCountReceived+")>=backFillStartHistoricLifeCount ("+backFillStartHistoricLifeCount +")");};};
            }
           else {
            {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"get History: lastHistoricLifeCountReceived ("+lastHistoricLifeCountReceived+")<backFillStartHistoricLifeCount ("+backFillStartHistoricLifeCount +")");};};
            int takelast=Math.max(lastHistoricLifeCountReceived,5);
            byte[] command=Natives.libre3ControlHistory(1, takelast);
            if(qsendcommand(command))
                backFillInProgress=true;
            }
        }
private void    fillClinical(int backFillStartLifeCount) {
      int lastLifeCountReceived=Natives.getlastLifeCountReceived(sensorptr);
      {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"getlastLifeCountReceived(sensorptr)="+lastLifeCountReceived+" backFillStartLifeCount="+ backFillStartLifeCount);};};

      if(lastLifeCountReceived<backFillStartLifeCount) {
        var command=Natives.libre3ClinicalControl(1,lastLifeCountReceived);
        if(qsendcommand(command))
            backFillInProgress=true;
        }
    }
private void receivedpatchstatus(byte[] value) {
    {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"receivedpatchstatus");};};
    byte[] decr= intDecrypt(cryptptr,2,value);
    int res=Natives.libre3processpatchstatus(sensorptr,decr);
    short currentLifeCount= (short) (res&0xFFFF);
    short index= (short) (res>>16);

    if(currentLifeCount<0)  {
        Log.e(LOG_ID, SerialNumber + ": "+"currentLifeCount<0");
        return;
        }
    if(!backFillInProgress) {
        int backFillStartLifeCount=currentLifeCount;
        int backFillStartHistoricLifeCount= ((backFillStartLifeCount-16)/5)*5;
        fillHistory(backFillStartHistoricLifeCount);
        fillClinical(backFillStartLifeCount);
    if(!doTEST) {    
        int getevent=index+1;
        if(getevent>lastEventReceived) {
            byte[] command=Natives.libre3EventLogControl(lastEventReceived);
            qsendcommand(command);
            } 
        }
            /*
        if(firstConnect) {
            byte command[]={6,0,0,0,0,0,0};
            qsendcommand(command);
            } */
        }
    }

@Override
public boolean matchDeviceName(String deviceName,String address) {
    final var thisaddress = Natives.getDeviceAddress(dataptr,false);
    return thisaddress!=null&&address!=null&&address.equals(thisaddress);
    }

@Override
public UUID getService() {
   return  LIBRE3_DATA_SERVICE;
   }

/*
@Override
public void setGattOptions(BluetoothGatt gatt) {
        if(doLog) {Log.i(LOG_ID,"setGattOptions(BluetoothGatt gatt) setPreferredPhy PHY_LE_2M_MASK");};
        gatt.setPreferredPhy(
         BluetoothDevice.PHY_LE_2M_MASK,
         BluetoothDevice.PHY_LE_2M_MASK,
         BluetoothDevice.PHY_OPTION_NO_PREFERRED); 
        } */


/*
static private PendingIntent mkintents(Context context,String id,int alarmrequest) {
       final int alarmflags;
       if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.M) {
               alarmflags = PendingIntent.FLAG_IMMUTABLE;
               }
       else
                alarmflags = 0;
       Intent alarmintent = new Intent(context, RefreshReceiver.class);
       alarmintent.setAction(id);
       return getBroadcast(context, alarmrequest++, alarmintent, alarmflags);
     }

private PendingIntent refreshalarm=null;
static private int alarmrequest=15;
static  PendingIntent  setrefreshalarm2(long alarmtime,PendingIntent refreshalarm,String SerialNumber) {
    try {
            Context context=Applic.app;
            if(refreshalarm==null)
               refreshalarm= mkintents(context,SerialNumber,alarmrequest);
            {if(doLog) {Log.i(LOG_ID,"set refreshalarm "+alarmtime);};};
            AlarmManager manager= (AlarmManager) context.getSystemService(ALARM_SERVICE);
            manager.setAlarmClock(new AlarmManager.AlarmClockInfo(alarmtime, refreshalarm), refreshalarm);
            return refreshalarm;
            }
       catch(Throwable e) {
           Log.stack(LOG_ID,"setrefreshalarm", e);
           return null;
           }
    finally {
        {if(doLog) {Log.i(LOG_ID,"after setrefreshalarm");};};
        }
    }
void refreshalarm(long alarmtime) {
        refreshalarm=setrefreshalarm2( alarmtime,refreshalarm, SerialNumber);
        }
private void cancelrefreshalarm() {
    if(refreshalarm!=null) {
        {if(doLog) {Log.i(LOG_ID,"cancelalarm");};};
        AlarmManager manager= (AlarmManager) Applic.app.getSystemService(ALARM_SERVICE);
        manager.cancel(refreshalarm);
        refreshalarm=null;//TODO: ?????
        }
    }
void doSomething() {
    var bluetoothGatt = mBluetoothGatt;
            if(bluetoothGatt != null)  {
                Log.i(LOG_ID,"readDescriptor");
                var charact= gattCharGlucoseData;
                if(gattCharGlucoseData!=null) {
                    BluetoothGattDescriptor descriptor = charact.getDescriptor(mCharacteristicConfigDescriptor);
                    bluetoothGatt.readDescriptor(descriptor);
                    }
                }
        }
        */
}
