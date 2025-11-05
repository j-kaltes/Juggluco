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
import android.app.PendingIntent;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.os.Looper;
import android.os.PowerManager;

//import java.security.SecureRandom;
import java.util.Queue;
import java.util.UUID;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;

import static android.bluetooth.BluetoothGatt.CONNECTION_PRIORITY_BALANCED;
import static android.bluetooth.BluetoothGatt.CONNECTION_PRIORITY_HIGH;
import static android.bluetooth.BluetoothGatt.GATT_SUCCESS;
import static android.bluetooth.BluetoothProfile.STATE_CONNECTED;
import static android.bluetooth.BluetoothProfile.STATE_DISCONNECTED;
import static android.content.Context.POWER_SERVICE;
import static java.lang.System.arraycopy;
import static java.util.Arrays.copyOfRange;
import static java.util.Objects.isNull;
import static tk.glucodata.Applic.app;
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.DexGattCallback.setalarm;
import static tk.glucodata.Log.doLog;
import static tk.glucodata.LossOfSensorAlarm.cancelalarm;
import static tk.glucodata.Natives.endcrypt;
import static tk.glucodata.Natives.initcrypt;
import static tk.glucodata.Natives.intDecrypt;
import static tk.glucodata.Natives.intEncrypt;
import static tk.glucodata.Log.showbytes;
import static tk.glucodata.util.sleep;


public class Libre3GattCallback extends SuperGattCallback {
    static final private boolean doTEST=false; //TODO
    static private final String LOG_ID = "Libre3GattCallback";
    private boolean shouldenablegattCharCommandResponse = false;
    private boolean isServicesDiscovered = false;
    private final long sensorptr;
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
    super.free();
    {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"free");};};
    cancelalarm();
    var tmp=cryptptr;
    cryptptr=0L;
    endcrypt(tmp);
    }
    public Libre3GattCallback(String SerialNumber, long dataptr)  {
        super(SerialNumber,dataptr,3);
        {if(doLog) {Log.d(LOG_ID, SerialNumber + ": "+ "Libre3GattCallback(..)");};};
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
    @Override 
    public void onCharacteristicRead(BluetoothGatt bluetoothGatt, BluetoothGattCharacteristic bluetoothGattCharacteristic, int i2) {
        checkBluetoothGatt(bluetoothGatt);
        if (bluetoothGattCharacteristic.getUuid().equals(LIBRE3_CHAR_PATCH_STATUS)) {
            //    libre3BLESensor.access$700(libre3BLESensor.this, bluetoothGattCharacteristic);
            {if(doLog){showbytes(LOG_ID + " "+SerialNumber+" onCharacteristicRead " + bluetoothGattCharacteristic.getUuid().toString(), bluetoothGattCharacteristic.getValue());};}
        }
    }

    @Override 
    public void onCharacteristicWrite(BluetoothGatt bluetoothGatt, BluetoothGattCharacteristic bluetoothGattCharacteristic, int i2) {
        checkBluetoothGatt(bluetoothGatt);
        oncharwrite(bluetoothGattCharacteristic);
        var value = bluetoothGattCharacteristic.getValue();
        {if(doLog){showbytes(LOG_ID + " "+SerialNumber+" onCharacteristicWrite " + bluetoothGattCharacteristic.getUuid().toString(), value);};}
    }

//    private boolean wasConnected = false;
private boolean connected=false;
    @SuppressLint("MissingPermission")
    @Override 
    public void onConnectionStateChange(BluetoothGatt bluetoothGatt, int status, int newState) {
        checkBluetoothGatt(bluetoothGatt);


        if(stop) {
            {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"onConnectionStateChange stop==true");};};
            return;
            }
        if(doLog) {
                        String[] state = {"DISCONNECTED", "CONNECTING", "CONNECTED", "DISCONNECTING"};
                        {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+ " onConnectionStateChange, status:" + status + ", state: " + (newState < state.length ? state[newState] : newState));};};
                        }
         long tim = System.currentTimeMillis();
        if(newState == STATE_CONNECTED) {
            //resetGlucose=0; 
            connected=true;
            setpriority(bluetoothGatt);
            constatchange[0] = tim;
            //wasConnected = true;
            if (!isServicesDiscovered||!getservices()) {
                if(!mBluetoothGatt.discoverServices()) {
                                        Log.e(LOG_ID, SerialNumber + ": "+"discoverServices()  failed");
                                        }
                else {
                    {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"discoverServices() success");};};
                    }

            } 
            } else if (newState == STATE_DISCONNECTED) {
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
            bluetoothGatt.close();
            mBluetoothGatt = null;
            }
            }
        }

        @Override 
        public void onDescriptorRead(BluetoothGatt bluetoothGatt, BluetoothGattDescriptor bluetoothGattDescriptor, int status) {
        {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+ "onDescriptorRead status="+status);};};
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
        public void onMtuChanged(BluetoothGatt bluetoothGatt, int i2, int i3) {
        {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"onMtuChanged");};};
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
    var encrypted= Natives.processbar(7,nonce1,uit);
    {if(doLog){showbytes(SerialNumber+" processbar(7,nonce1,uit)",encrypted);};}
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
    byte[] decr=Natives.processbar(8,nonce,first);
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
//    byte[] AuthKey=ECDHCrypto.exportAuthorizationKey();
    byte[] AuthKey=Natives.processbar(9,null,null);
    //securityContext=new BCrypt(kEnc,ivEnc);
    cryptptr=initcrypt(cryptptr,kEnc,ivEnc);
    Natives.setLibre3kAuth(sensorptr,AuthKey);
    enableNotification(mBluetoothGatt,gattCharPatchDataControl);
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
//boolean sendSecurityCommand(1,null) after com.adc.trident.app.frameworks.mobileservices.libre3.security.Libre3SKBCryptoLib::initECDH=1
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
    cryptolib.setPatchCertificate(rdtData);
    if(sendSecurityCommand( (byte)0x0D)) {
        commandphase=4;
        }
    }
private boolean    generateKAuth(byte[] input) {
    {if(doLog){showbytes(LOG_ID+ " "+SerialNumber +" generateKAuth",input);};}
    //Saves something?
    return Natives.processint(6,input,null)!=0;
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

        checkBluetoothGatt(bluetoothGatt);
    onCharacteristicChanged33(bluetoothGatt, bluetoothGattCharacteristic, bluetoothGattCharacteristic.getValue());
    }
static final private String charglucosedata= "CHAR_GLUCOSE_DATA".intern();
        @SuppressLint("MissingPermission")
//        @Override 
//static int final usewakelock=false;
private  void logcharacter(UUID uuid,String str,byte[] value) {
        final long timmsec = System.currentTimeMillis();
       if(str!=charglucosedata) setsuccess(timmsec,str);
            {if(doLog){showbytes(LOG_ID+ " "+SerialNumber +" onCharacteristicChanged  "+uuid.toString()+" "+str, value);};}
        }
private void onCharacteristicChanged33(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, byte[] value) {
       var wakelock=    Applic.usewakelock?(((PowerManager) app.getSystemService(POWER_SERVICE)).newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "Juggluco::Libre3")):null;
       if(wakelock!=null)
           wakelock.acquire();
            UUID uuid = characteristic.getUuid();
//      {if(doLog){      showbytes(LOG_ID+" onCharacteristicChanged Start "+uuid.toString(), value);};}
            if(uuid.equals(LIBRE3_CHAR_GLUCOSE_DATA)) {
                logcharacter(uuid,charglucosedata,value);
                glucose_data(value);
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

private final ECDHCrypto cryptolib=new ECDHCrypto();
//int    securityState=0;
private boolean    isPreAuthorized=false;
private void onConnectGatt() {
    isPreAuthorized=false;
    }
private void handleMSLibre3SecurityNotificationsEnabledEvent() {
    {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"handleMSLibre3SecurityNotificationsEnabledEvent");};};
    if(isPreAuthorized) {
        //securityState=2;
        sendSecurityCommand(17);
        }
    else {

            var exportedKAuth = Natives.getLibre3kAuth(sensorptr);
        if(cryptolib.initECDH(exportedKAuth ,1)) {
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
            if(cryptolib.initECDH(exportedKAuth ,1)) {
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
        bluetoothGatt.connect();
        return;
        }
    else {
        bluetoothGatt.close();
        mBluetoothGatt = null;
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
    {if(doLog){showbytes(LOG_ID+" "+ SerialNumber +" access1100",decr);};}
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

    var evikeys=Natives.processbar(5,null,null);
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
        info("oncharwrite access$800");
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
private    void glucose_data(byte[] value) {
        if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"start glucose_data");};
        int len = value.length;

        System.arraycopy(value, 0, this.oneMinuteRawData, this.oneMinuteReadingSize, len);
        oneMinuteReadingSize +=len;
        if(oneMinuteReadingSize >= oneMinuteRawData.length) {
           long timmsec = System.currentTimeMillis();
           this.oneMinuteReadingSize = 0;
           byte[] decr = intDecrypt(cryptptr,3, oneMinuteRawData);
           if(decr == null) {
                Log.e(LOG_ID, SerialNumber + ": "+"intDecrypt(cryptptr,3, oneMinuteRawData)==null");
                return;
               }
           long res=Natives.saveLibre3MinuteL(this.sensorptr, decr);
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

}

