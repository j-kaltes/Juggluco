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
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.os.PowerManager;

import java.security.SecureRandom;
import java.util.Queue;
import java.util.UUID;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;

import static android.bluetooth.BluetoothGatt.GATT_SUCCESS;
import static android.bluetooth.BluetoothProfile.STATE_CONNECTED;
import static android.bluetooth.BluetoothProfile.STATE_DISCONNECTED;
import static android.content.Context.POWER_SERVICE;
import static java.lang.System.arraycopy;
import static java.util.Arrays.copyOfRange;
import static tk.glucodata.Applic.app;
import static tk.glucodata.Natives.endcrypt;
import static tk.glucodata.Natives.initcrypt;
import static tk.glucodata.Natives.intDecrypt;
import static tk.glucodata.Natives.intEncrypt;
import static tk.glucodata.Natives.showbytes;


public class Libre3GattCallback extends SuperGattCallback {
	static private final String LOG_ID = "Libre3GattCallback";
	private boolean shouldenablegattCharCommandResponse = false;
	private boolean isServicesDiscovered = false;
	private final long sensorptr;
//	Queue<byte[]> sendqueue = new ArrayDeque<byte[]>();
private final Queue<byte[]> sendqueue = new ConcurrentLinkedQueue<byte[]>();
//	int currentControlCommand=0;
private int    lastEventReceived=0;
//	public BluetoothGattService gattServiceADC = null;
//	private BluetoothGattService gattServiceDeviceInfo = null;
//	private BluetoothGattService gattServiceSecurity = null;
//	private BluetoothGattService gattServiceDebug = null;
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
//	static private final boolean exportNULL=false;
private void info(String in) {
	Log.i(LOG_ID,in);
	}
@Override
void free() {
	super.free();
	Log.i(LOG_ID,"free");
	var tmp=cryptptr;
	cryptptr=0L;
	endcrypt(tmp);
	}
	public Libre3GattCallback(SensorBluetooth sensorbluetooth, String SerialNumber, long dataptr)  {
		super(3);
		Log.d(LOG_ID, "Libre3GattCallback(..)");
		this.sensorbluetooth = sensorbluetooth;
		this.SerialNumber = SerialNumber;
		this.dataptr = dataptr;
		sensorptr = Natives.getsensorptr(dataptr);
		mActiveDeviceAddress = Natives.getDeviceAddress(dataptr);
		Log.i(LOG_ID, "new Libre3GattCallback " + SerialNumber + " " + ((mActiveDeviceAddress != null) ? mActiveDeviceAddress : "null"));

		var thr=new Thread(()-> init());
		thr.start();
		try {
			thr.join();
		} catch(Throwable th) {
			Log.stack(LOG_ID,"init",th);
		}
	}

	@Override 
	public void onCharacteristicRead(BluetoothGatt bluetoothGatt, BluetoothGattCharacteristic bluetoothGattCharacteristic, int i2) {
		if (bluetoothGattCharacteristic.getUuid().equals(LIBRE3_CHAR_PATCH_STATUS)) {
			//    libre3BLESensor.access$700(libre3BLESensor.this, bluetoothGattCharacteristic);
			showbytes(LOG_ID + " onCharacteristicRead " + bluetoothGattCharacteristic.getUuid().toString(), bluetoothGattCharacteristic.getValue());
		}
	}

	@Override 
	public void onCharacteristicWrite(BluetoothGatt bluetoothGatt, BluetoothGattCharacteristic bluetoothGattCharacteristic, int i2) {
		oncharwrite(bluetoothGattCharacteristic);
		var value = bluetoothGattCharacteristic.getValue();
		showbytes(LOG_ID + " onCharacteristicWrite " + bluetoothGattCharacteristic.getUuid().toString(), value);
	}

//	private boolean wasConnected = false;

	@SuppressLint("MissingPermission")
	@Override 
	public void onConnectionStateChange(BluetoothGatt bluetoothGatt, int status, int newState) {
		if (!tk.glucodata.Applic.isRelease) {
                        String[] state = {"DISCONNECTED", "CONNECTING", "CONNECTED", "DISCONNECTING"};
                        Log.i(LOG_ID, SerialNumber + " onConnectionStateChange, status:" + status + ", state: " + (newState < state.length ? state[newState] : newState));
                        }
	     long tim = System.currentTimeMillis();
		if(newState == STATE_CONNECTED) {
		    setpriority();
		      constatchange[0] = tim;
			//wasConnected = true;
			if (!isServicesDiscovered||!getservices()) {
				Log.i(LOG_ID,"discoverServices() ");
				mBluetoothGatt.discoverServices(); 
			} 
            } else if (newState == STATE_DISCONNECTED) {
                Log.e(LOG_ID, "onConnectionStateChange ERROR: disconnected with status : " + status);
               // libre3BLESensor.access$600(libre3BLESensor.this, status);
 		constatchange[1] = tim;
		constatstatus = status;
		if(lastphase5) {
			if(status==19) {
				isPreAuthorized=false;
				Natives.setLibre3kAuth(sensorptr,null);
				}
			} 
		 realdisconnected(status);
            }
        }

        @Override // android.bluetooth.BluetoothGattCallback
        public void onDescriptorRead(BluetoothGatt bluetoothGatt, BluetoothGattDescriptor bluetoothGattDescriptor, int status) {
		Log.i(LOG_ID, "onDescriptorRead status="+status);
        }



        @Override // android.bluetooth.BluetoothGattCallback
        public void onDescriptorWrite(BluetoothGatt bluetoothGatt, BluetoothGattDescriptor bluetoothGattDescriptor, int status) {
           // libre3BLESensor.access$1900(libre3blesensor, characteristic, status);
		Log.i(LOG_ID, "onDescriptorWrite status="+status);
	    BluetoothGattCharacteristic characteristic = bluetoothGattDescriptor.getCharacteristic();
            handleonDescriptorWrite(characteristic);
        }

        @Override // android.bluetooth.BluetoothGattCallback
        public void onMtuChanged(BluetoothGatt bluetoothGatt, int i2, int i3) {
		Log.i(LOG_ID,"onMtuChanged");
        }

        @Override // android.bluetooth.BluetoothGattCallback
        public void onReadRemoteRssi(BluetoothGatt bluetoothGatt, int rssi, int status) {
            if (status != GATT_SUCCESS) {
                Log.e(LOG_ID, "Error reading RSSI, error " + status);
                rssi = 999;
            }
		readrssi=rssi;
		if(shouldenablegattCharCommandResponse) {
			enablegattCharCommandResponse();
			shouldenablegattCharCommandResponse=false;
			}
		}

        @Override // android.bluetooth.BluetoothGattCallback
        public void onServicesDiscovered(BluetoothGatt bluetoothGatt, int status) {
	   Log.i(LOG_ID, "onServicesDiscovered status="+status);
            if (status == GATT_SUCCESS) {
                if(!getservices()) {
			bluetoothGatt.disconnect();
			disconnected(status);
			}
		}
             else {
                Log.e(LOG_ID, "BLE: onServicesDiscovered error: " + status);
		bluetoothGatt.disconnect();
		disconnected(status);
            }
        }

private   int rdtBytes =0;
private        int rdtSequence = 0;
private  int rdtLength =0;
private	byte[] rdtData;
//libre3BLESensor.access$1200(libre3BLESensor.this, bluetoothGattCharacteristic)
    int getsecdata(byte[] value) {
        if (value.length < 1) {
 	   var message="getsecdata unknown command length=" + value.length;
            Log.e( LOG_ID, message);
	    setfailure(message);
	    mBluetoothGatt.disconnect();
            return rdtLength;
        }
        int i2 = value[0] & 0xFF;
        if (i2 != rdtSequence + 1) {
            var message= "getsecdata secu Sequence=" + i2 + "!=" + rdtSequence + "-1 (rdtSequence-1)";
            Log.e( LOG_ID, message);
	    setfailure(message);
	    mBluetoothGatt.disconnect();
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
  //  libre3BLESensor.access$1400(libre3blesensor3, new MSLibre3ChallengeDataReadEvent(libre3blesensor3.rdtData));
private final byte[] r1=new byte[16];
private final byte[] r2=new byte[16];
private final byte[] nonce1=new byte[7];

private  void	randomr2() {
      (new SecureRandom()).nextBytes(r2);
	//vul r2 met random bytes
	}
private void setr1none(byte[] rdtData) {
	Log.i(LOG_ID,"setr1none");
	arraycopy(rdtData,0,r1,0,16);
	arraycopy(rdtData,16,nonce1,0,7);
	randomr2();
	mknonceback();
	}
private byte[] wrtData;
private int wrtOffset;
private void mknonceback() {
	Log.i(LOG_ID,"mknonceback");
	byte[] uit=new byte[36];
	arraycopy(r1,0,uit,0,16);
	arraycopy(r2,0,uit,16,16);
	byte[] pin=Natives.getpin(sensorptr);
	arraycopy(pin,0,uit,32,4);
	var encrypted= Natives.processbar(7,nonce1,uit);
	showbytes("processbar(7,nonce1,uit)",encrypted);
	wrtData=encrypted;
	wrtOffset=0;
	writedata(gattCharChallengeData);

/*	 NOTUSED:
 	var plusnone=new byte[47];
	arraycopy(encrypted,0,plusnone,0,40);
	arraycopy(nonce1,0,plusnone,40,7); */

	}
//byte[] kEnc;
//byte[] ivEnc;
/*
static class Libre3BCSecurityContext {
    public static final byte[][] packetDescriptor = null;
    private final int NONCE_COUNTER_OFFSET;
    private final int NONCE_IV_OFFSET;
    private final int NONCE_PACKET_DESCRIPTOR_OFFSET;
    private byte[] iv_enc;
    public byte[] key;
    public byte[] nonce;
    private int outCryptoSequence;

   Libre3BCSecurityContext(byte[] key,byte[] iv) {
   	 this.key=key;
	 this.iv_enc=iv;
	 nonce=new byte[13];
	 arraycopy(iv,0,nonce,5,8);
   	}
    };
    */
//private static final UUID mCharacteristicConfigDescriptor= java.util.UUID.fromString("00002902-0000-1000-8000-00805f9b34fb");

//private static               final UUID CONFIG_DESCRIPTOR= mCharacteristicConfigDescriptor;
//Libre3SKBCryptoLib

//BCrypt securityContext=null;
private long cryptptr=0L;
private void challenge67() {
	Log.i(LOG_ID,"challenge67()");
	byte[] first=new byte[60];
	byte[] nonce=new byte[7];
	arraycopy(rdtData,0,first,0,60);
	arraycopy(rdtData,60,nonce,0,7);
	byte[] decr=Natives.processbar(8,nonce,first);
	var backr2=copyOfRange(decr,0,16);
	if(!java.util.Arrays.equals(r2,backr2)) {
		Log.i(LOG_ID,"r2!=backr2");
		//TODO disconnect?
		}
	var backr1=copyOfRange(decr,16,32);
	if(!java.util.Arrays.equals(r1,backr1)) {
		Log.i(LOG_ID,"r1!=backr1");
		//TODO disconnect?
		}
	var kEnc=copyOfRange(decr,32,48);
	var ivEnc=copyOfRange(decr,48,56);
//	byte[] AuthKey=ECDHCrypto.exportAuthorizationKey();
	byte[] AuthKey=Natives.processbar(9,null,null);
	//securityContext=new BCrypt(kEnc,ivEnc);
	cryptptr=initcrypt(cryptptr,kEnc,ivEnc);
	Natives.setLibre3kAuth(sensorptr,AuthKey);
	MyGattCallback.enableNotification(mBluetoothGatt,gattCharPatchDataControl);
	}
private void receivedCHALLENGE_DATA() {
	switch(rdtLength) {
		case 23: setr1none(rdtData); break;
		case 67: challenge67();break;
		default: {
			var message="receivedCHALLENGE_DATA unknown length="+rdtLength;
		 	mBluetoothGatt.disconnect();
			Log.i(LOG_ID,message);
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
	Log.i(LOG_ID,"sendSecurityCommand "+b);
	byte[] com={(byte)b};
	gattCharCommandResponse.setValue(com);
	/*
	synchronized(syncObject) {
		isNotificationSuspended=true;
		} */
	if(!mBluetoothGatt.writeCharacteristic(gattCharCommandResponse)) {
		Log.e(LOG_ID, "writeCharacteristic(gattCharCommandResponse) failed");
		mBluetoothGatt.disconnect(); //TODO: or try again?
		return false;
		}
	return true; 
	}
private int commandphase=1;
private void setCertificate140() {
	Log.i(LOG_ID,"setCertificate140");
	cryptolib.setPatchCertificate(rdtData);
	if(sendSecurityCommand( (byte)0x0D)) {
		commandphase=4;
		}
	}
private boolean	generateKAuth(byte[] input) {
	showbytes(LOG_ID+" generateKAuth",input);
	//Saves something?
	return Natives.processint(6,input,null)!=0;
	}
private boolean setCertificate65() {
	Log.i(LOG_ID,"setCertificate65");
	byte[]	patchEphemeral=rdtData;
	if(generateKAuth(patchEphemeral)) //TODO failure?
		return sendSecurityCommand((byte)17);
	return false;
	}
private void receivedCERT_DATA() {
	switch(rdtLength) {
		case 140: setCertificate140();break;
		case 65: setCertificate65();break;
		default: {
			var message="receivedCERT_DATA unknown length="+rdtLength;
			Log.i(LOG_ID,message);
			setfailure(message);  //TODO disconnect?
			}
		};
	}
static final private boolean notsuspended=true;
  void enablegattCharCommandResponse() {
  	if(notsuspended) {
		Log.i(LOG_ID,"enablegattCharCommandResponse");
		 MyGattCallback.enableNotification(mBluetoothGatt,gattCharCommandResponse);
		 }
  }
//19156 00000 00013 01036 19156 00019





//public static native byte[] intEncrypt(long cryptptr, int kind,byte[] plain);
//public static native byte[] intDecrypt(long cryptptr, int kind,byte[] encrypted);

private    void save_history(byte[] value) {
	byte[] olddec=intDecrypt(cryptptr,4, value);
        Natives.saveLibre3History(this.sensorptr, olddec);
    }
@Override 
public void onCharacteristicChanged(BluetoothGatt bluetoothGatt, BluetoothGattCharacteristic bluetoothGattCharacteristic) {
	onCharacteristicChanged33(bluetoothGatt, bluetoothGattCharacteristic, bluetoothGattCharacteristic.getValue());
	}
static final private String charglucosedata= "CHAR_GLUCOSE_DATA".intern();
        @SuppressLint("MissingPermission")
//		@Override 
//static int final usewakelock=false;
private void onCharacteristicChanged33(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, byte[] value) {
	   var wakelock=	Applic.usewakelock?(((PowerManager) app.getSystemService(POWER_SERVICE)).newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "Juggluco::Libre3")):null;
	   if(wakelock!=null)
		   wakelock.acquire();
	    long timmsec = System.currentTimeMillis();
            String str;
            UUID uuid = characteristic.getUuid();
            showbytes(LOG_ID+" onCharacteristicChanged Start "+uuid.toString(), value);
            if (uuid.equals(LIBRE3_CHAR_GLUCOSE_DATA)) {
                glucose_data(value);
                str = charglucosedata;
            } else if (uuid.equals(LIBRE3_CHAR_PATCH_STATUS)) {
        //        libre3BLESensor.access$700(libre3BLESensor.this, bluetoothGattCharacteristic);
		receivedpatchstatus(value);
                str = "CHAR_PATCH_STATUS";
            } else if (uuid.equals(LIBRE3_CHAR_HISTORIC_DATA)) {
                save_history(value);
               // libre3BLESensor.access$1000(libre3blesensor, bluetoothGattCharacteristic);
                str = "CHAR_HISTORIC_DATA";
            } else if (uuid.equals(LIBRE3_CHAR_PATCH_CONTROL)) {
               // libre3BLESensor.access$1100(libre3BLESensor.this, bluetoothGattCharacteristic);
		access1100(value);
                str = "CHAR_PATCH_CONTROL";
            } else if (uuid.equals(LIBRE3_SEC_CHAR_CERT_DATA)) {
                if (getsecdata(value) <= 0) {
		   receivedCERT_DATA();
                  //  libre3BLESensor.access$1400(libre3blesensor2, new MSLibre3CertificateReadEvent(libre3blesensor2.rdtData));
                }
                str = "SEC_CHAR_CERT_DATA";
            } else if (uuid.equals(LIBRE3_SEC_CHAR_CHALLENGE_DATA)) {
                if (getsecdata(value) <= 0) {
		    receivedCHALLENGE_DATA();
                  //  libre3BLESensor.access$1400(libre3blesensor3, new MSLibre3ChallengeDataReadEvent(libre3blesensor3.rdtData));
                }
                str = "SEC_CHAR_CHALLENGE_DATA";
            } else if (uuid.equals(LIBRE3_SEC_CHAR_COMMAND_RESPONSE)) {
		lastphase5=false;
                preparedata(value);
                str = "SEC_CHAR_COMMAND_RESPONSE";
            } else if (uuid.equals(LIBRE3_CHAR_EVENT_LOG)) {
//	    libre3BLESensor.access$1600(libre3BLESensor.this, bluetoothGattCharacteristic);
		logevent(value);
                str = "CHAR_EVENT_LOG";
            } else if (uuid.equals(LIBRE3_CHAR_FACTORY_DATA)) {
               // libre3BLESensor.access$1700(libre3BLESensor.this, bluetoothGattCharacteristic);
		//		access1700(value) ;
                str = "CHAR_FACTORY_DATA";
            } else if (uuid.equals(LIBRE3_CHAR_CLINICAL_DATA)) {
//                libre3BLESensor libre3blesensor4 = libre3BLESensor.this; libre3BLESensor.access$1800(libre3blesensor4, bluetoothGattCharacteristic);
                fast_data(value);
                str = "CHAR_CLINICAL_DATA";
            } else {
		 mBluetoothGatt.disconnect();
		 disconnected(1042);
                str = "Unknown";
            }
	   if(str!=charglucosedata)
		    setsuccess(timmsec,str);
            showbytes(LOG_ID+" onCharacteristicChanged end "+str, value);
	   if(wakelock!=null)
		wakelock.release();
        }


//source /n/ojka/tmp/libre3.3.0/sensor/newsensor/working

@SuppressLint("MissingPermission")
static  private  boolean enableNotification(BluetoothGatt mBluetoothGatt, BluetoothGattCharacteristic bluetoothGattCharacteristic) {
	Log.i(LOG_ID,	"enableNotification");
        mBluetoothGatt.setCharacteristicNotification(bluetoothGattCharacteristic, true);
        BluetoothGattDescriptor descriptor = bluetoothGattCharacteristic.getDescriptor(mCharacteristicConfigDescriptor);
        descriptor.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
        return mBluetoothGatt.writeDescriptor(descriptor);
        }

private boolean asknotification(BluetoothGattCharacteristic charac) {
		return enableNotification(mBluetoothGatt, charac);
	}

private	void fast_data(byte[] encryp) {
	byte[] decr=intDecrypt(cryptptr,5,encryp);
        if (decr == null) {
            info("fast_data decrypt went wrong"); //TODO: DISCONNECT?
        } else {
            Natives.saveLibre3fastData(sensorptr, decr);
        }
    }

private final ECDHCrypto cryptolib=new ECDHCrypto();
//int	securityState=0;
private boolean	isPreAuthorized=false;
private void onConnectGatt() {
	isPreAuthorized=false;
	}
private void handleMSLibre3SecurityNotificationsEnabledEvent() {
	Log.i(LOG_ID,"handleMSLibre3SecurityNotificationsEnabledEvent");
	if(isPreAuthorized) {
		//securityState=2;
		sendSecurityCommand(17);
		}
	else {

	    	var exportedKAuth = Natives.getLibre3kAuth(sensorptr);
		if(cryptolib.initECDH(exportedKAuth ,1)) {
			if(exportedKAuth==null) {
				Log.i(LOG_ID,"exportedKAuth==null");
				sendSecurityCommand(1);
				commandphase=1;
				}
			else  {
				Log.i(LOG_ID,"exportedKAuth!=null");
				isPreAuthorized=true;
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
//s/^.*Set.*Field.com.adc.trident.app.frameworks.mobileservices.libre3.libre3BLESensor,\(.*\),\(.*\))$/\1=\2;/g
private void init() {
//    exportedKAuth=Natives.getLibre3kAuth(sensorptr);
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



//private	boolean sendEphemeralKeys=false;
@SuppressLint("MissingPermission")
private void realdisconnected(int status) {
	//sendEphemeralKeys=false;
	Log.i(LOG_ID,"disconnected "+status);
	/*
	timerPatchStatus=0x0 ;
	timerReading=0x0 ;
	timerAuthentication=0x0 ;
	timerRSSI=0x0 ;
	pendingTermination=0;
	terminateReason=0;
	inShutdown=0;
	shutdownEvent=0x0 ;
	timerCommandRetry=0x0 ; */
	oneMinuteReadingSize=0;
//	oneMinutePacketNumber=0;
	//historicalRecordCount=0;
//	rssiCompletionEvent=0x0 ;
//	currentControlCommand=0;
	backFillInProgress=false;
	shouldenablegattCharCommandResponse=false;
//	endcrypt(cryptptr); cryptptr=0L;
	close();
	isServicesDiscovered=false;
	/*
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
        mActiveBluetoothDevice.connectGatt(app, autoconnect, this, BluetoothDevice.TRANSPORT_LE);
    }
    else
        mActiveBluetoothDevice.connectGatt(app, autoconnect, this);
	*/
	init();
	wrotecharacter=false;
	sendqueue.clear();
	connectDevice(0);//TODO:  What if it fails?
	}

private void disconnected(int status) {
	realdisconnected(status); //TODO remove this
	}

private  void  setsuccess(long timmsec,String str) {
	 wrotepass[0]=timmsec;
	handshake =str;
	}
private  void  setfailure(String str) {
	 wrotepass[1]= System.currentTimeMillis();
	handshake =str;
	}
private void handleonDescriptorWrite(BluetoothGattCharacteristic characteristic) {
        final var uuid = characteristic.getUuid();
	String struuid=uuid.toString();
	Log.i(LOG_ID,"handleonDescriptorWrite "+struuid);
	 long timmsec = System.currentTimeMillis();
	 setsuccess(timmsec,struuid);
        if (LIBRE3_CHAR_PATCH_CONTROL.equals(uuid)) {
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
                            } else {
                                if (LIBRE3_CHAR_PATCH_STATUS.equals(uuid)) {
				/*
                                    if (workoffEvents()) {
                                        sendevent(new MSLibre3NotificationEnabledEvent());
                                        if (timerReading != null) {
                                            timerReading.cancel();
                                            timerReading = AppTimer.Timeout(90000L, 1, this);
                                        }
                                    }
				*/
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
	showbytes(LOG_ID+" access1700",decr);
        factoryData.write(decr,1,decr.length-1);
        }
*/
private void access1100(byte[] value) {
 //       byte[] decr=com.adc.trident.app.frameworks.mobileservices.libre3.security.Libre3BCSecurityContext::decrypt(1,value);
        byte[] decr=intDecrypt(cryptptr,1,value); //USED for what??
	showbytes(LOG_ID+" access1100",decr);
//        bluetoothGattCharacteristic.setValue(decr);//Slaat nergens op
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
	fromqueue();
	}

private    void preparedata(byte[] value) {
    	showbytes(LOG_ID+" preparedata",value);
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
	    mBluetoothGatt.disconnect();
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
	    mBluetoothGatt.disconnect();
	   disconnected(1023);
            return;
        }
//        sendevent(mSLibre3Event);
    }

    @SuppressLint("MissingPermission")
  private  int writedata(BluetoothGattCharacteristic bluetoothGattCharacteristic) {
      if(wrtData==null) {
        Log.e(LOG_ID,"writedata wrtData==null"+ bluetoothGattCharacteristic.getUuid().toString());
	    mBluetoothGatt.disconnect();
	   disconnected(1099);
       	return 0;
         }
      else  {
	      Log.i(LOG_ID,"writedata "+ bluetoothGattCharacteristic.getUuid().toString());
	       }
        
        int length = this.wrtData.length - this.wrtOffset;
        if(length > 0) {
            int min = Math.min(length, 18);
            byte[] bArr = new byte[20];
            System.arraycopy(this.wrtData, this.wrtOffset, bArr, 2, min);
            showbytes("writedata  wrtOffset="+wrtOffset+" length="+min,bArr);
            bluetoothGattCharacteristic.setValue(bArr);
            bluetoothGattCharacteristic.setValue(this.wrtOffset, 18, 0);
            this.wrtOffset += min;
            if(this.mBluetoothGatt.writeCharacteristic(bluetoothGattCharacteristic))
		    return 1;
	else {
		Log.e(LOG_ID,"writeCharacteristic(bluetoothGattCharacteristic) failed");
		return 0; //TODO disconnect?
		}
        }
	Log.i(LOG_ID,"writedata all written");
        return 2;
    }
private int getcomphase() {
	return commandphase;
	}
/*
static byte[]		generateEphemeralKeysorig() {
	return com.adc.trident.app.frameworks.mobileservices.libre3.security.Libre3SKBCryptoLib.generateEphemeralKeys();
	} */
private static byte[]           generateEphemeralKeys() {

	var evikeys=Natives.processbar(5,null,null);
	var uit=new byte[evikeys.length+1];
	arraycopy(evikeys,0,uit,1,evikeys.length);
	uit[0]=(byte)0x4;
	showbytes(LOG_ID+ " generateEphemeralKeys()",uit);
	return uit;
	}

private boolean sendSecurityCert(byte[] cert) {
	Log.i(LOG_ID,"sendSecurityCert");
	wrtOffset=0;
	wrtData	=cert;
	return writedata(gattCharCertificateData)!=0;
	}


private boolean	lastphase5=false;

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
				}
				;
				break;
				case 3:
					return;
				case 4: {
					if (sendSecurityCert(generateEphemeralKeys()))
						commandphase = 5;
				}
				;
				break;
				case 5:
					lastphase5=true;
					return;
			}


/*
            MSLibre3CommandSentEvent mSLibre3CommandSentEvent = new MSLibre3CommandSentEvent();
            mSLibre3CommandSentEvent.setDevice(this.msDevice);
            this.stateMachine.handleEvent(mSLibre3CommandSentEvent);
            ServiceAdaptor serviceAdaptor = ServiceAdaptor.getInstance();
            serviceAdaptor.adaptorCallback.onEvent(mSLibre3CommandSentEvent);
            synchronized (syncObject) {
                while (true) {
                    MSLibre3Event poll = this.pendingEvents.poll();
                    if (poll == null) {
                        break;
                    }
                    this.stateMachine.handleEvent(poll);
                    serviceAdaptor.adaptorCallback.onEvent(poll);
                }
            }
            if (this.pendingEvents.size() == 0) {
                this.isNotificationSuspended = false;
            }
	    */
/*            if (sendqueue.size() == 0) {
                isNotificationSuspended = false;
            }*/
        } else {
//	     fromqueue();
            info("oncharwrite else");
        }
        info("oncharwrite end");
    }

    @SuppressLint("MissingPermission")
   private boolean getservices() {
       Log.i(LOG_ID,"getservices");
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
       	   Log.i(LOG_ID,"getservices failure");
        isServicesDiscovered = false;
            return false;
        }
        isServicesDiscovered = true;
	shouldenablegattCharCommandResponse=true;
        this.mBluetoothGatt.readRemoteRssi();
       Log.i(LOG_ID,"getservices success");
       return true;
    }



    private int oneMinuteReadingSize = 0;
//    private int oneMinutePacketNumber = 0;
    private final byte[] oneMinuteRawData = new byte[35];

    @SuppressLint("MissingPermission")
private    void glucose_data(byte[] value) {
        Log.i(LOG_ID,"start glucose_data");
        int len = value.length;

//        this.oneMinutePacketNumber++;
        System.arraycopy(value, 0, this.oneMinuteRawData, this.oneMinuteReadingSize, len);
	oneMinuteReadingSize +=len;
        if (oneMinuteReadingSize >= oneMinuteRawData.length) {
	    long timmsec = System.currentTimeMillis();

//            this.oneMinutePacketNumber = 0;
            this.oneMinuteReadingSize = 0;
            byte[] decr = intDecrypt(cryptptr,3, oneMinuteRawData);
            if (decr == null) {
	       Log.e(LOG_ID,"intDecrypt(cryptptr,3, oneMinuteRawData)==null");
                return;
            }
            long res=Natives.saveLibre3MinuteL(this.sensorptr, decr);
	    handleGlucoseResult(res,timmsec);

	    /*
		if(!libre3blesensor.isWaitingForPatchStatus.get() && !libre3blesensor.backFillInProgress) {
		    libre3blesensor.initParam.lastLifeCountReceived = aBT_GlucoseData.lifeCount;
		    libre3blesensor.initParam.lastHistoricLifeCountReceived = aBT_GlucoseData.historicalLifeCount;
		} */

            this.mBluetoothGatt.readRemoteRssi();
        }
        Log.i(LOG_ID,"end glucose_data");
    }

private ScheduledFuture<?> retrytimer=null;
private void setretrytimer() {
    if(retrytimer==null) {
    	Log.i(LOG_ID,"set timer");
	retrytimer=Applic.scheduler.schedule(()-> { 
		retrytimer=null;
		Log.i(LOG_ID,"timer went off");
		fromqueue(); 
		}, 5, TimeUnit.SECONDS);
		}
	else
		Log.i(LOG_ID,"already timer");
	}
private void cancelretrytimer() {
	var tmp=retrytimer;
	retrytimer=null;
	if(tmp!=null) {
		tmp.cancel(false);
		}
	}
private boolean wrotecharacter=false;
@SuppressLint("MissingPermission")
private boolean qsendcommand(byte[] command) {
	showbytes(LOG_ID+" qsendcommand",command);
	onqueue(command);
	if(!wrotecharacter)
		return fromqueue();
	return false;	
	}
private boolean sendcommandonly(byte[] encr) {
	gattCharPatchDataControl.setValue(encr);
	wrotecharacter=true;
	if(mBluetoothGatt.writeCharacteristic(gattCharPatchDataControl)) {
		showbytes(LOG_ID+" qsendcommand written",encr);
		return true;
		}
	else  {
		setretrytimer();
		}
	return false;
	}
private void onqueue(byte[] command) {
	showbytes(LOG_ID+" onqueue sizebefore="+sendqueue.size(),command);
	byte[] encr= intEncrypt(cryptptr,0,command);
	sendqueue.offer(encr);
	}

private boolean fromqueue() {
	Log.i(LOG_ID,"fromqueue size="+sendqueue.size());
//	wrotecharacter=false;
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
/*
void fromqueue() {
	if(!pendingTermination) {
		commandQueLock.lock();
		 try {
		 if(controlCommandQue.isEmpty())
			currentControlCommand=0;
		else {
			Set<Map.Entry<Integer,byte[]>> assign= controlCommandQue.entrySet();
			for(var el:assign) {
				currentControlCommand=el.getKey();
				if(sendcommand(el.getValue())) {
					controlCommandQue.remove( currentControlCommand);
					}
				else {
				//WHAT?
					}

				}

			}
		} finally {
	       lock.unlock()
	     }
	    }
	    }
*/



//access$700
private boolean backFillInProgress=false;
//private int backFillStartLifeCount=0;
//private int		backFillStartHistoricLifeCount= 0;

//int lastLifeCountReceived=0;
//int lastHistoricLifeCountReceived=0;

//private boolean isWaitingForPatchStatus=true;
//	boolean firstConnect=false; //true als net geactiveerd met NFC
private void fillHistory(int backFillStartHistoricLifeCount) {
		int lastHistoricLifeCountReceived=Natives.getlastHistoricLifeCountReceived(sensorptr);
		if(backFillStartHistoricLifeCount<=lastHistoricLifeCountReceived) {
			Log.i(LOG_ID,"no history needed  lastHistoricLifeCountReceived ("+lastHistoricLifeCountReceived+")>=backFillStartHistoricLifeCount ("+backFillStartHistoricLifeCount +")");
			}
	   	else {
			Log.i(LOG_ID,"get History: lastHistoricLifeCountReceived ("+lastHistoricLifeCountReceived+")<backFillStartHistoricLifeCount ("+backFillStartHistoricLifeCount +")");
			int takelast=Math.max(lastHistoricLifeCountReceived,5);
			byte[] command=Natives.libre3ControlHistory(1, takelast);
		//	currentControlCommand=1;
			if(qsendcommand(command))
				backFillInProgress=true;
			}
		}
private void	fillClinical(int backFillStartLifeCount) {
	  int lastLifeCountReceived=Natives.getlastLifeCountReceived(sensorptr);
	  Log.i(LOG_ID,"getlastLifeCountReceived(sensorptr)="+lastLifeCountReceived+" backFillStartLifeCount="+ backFillStartLifeCount);

	  if(lastLifeCountReceived<backFillStartLifeCount) {
		var command=Natives.libre3ClinicalControl(1,lastLifeCountReceived);
	//	currentControlCommand=2;
		if(qsendcommand(command))
			backFillInProgress=true;
		}
	}
private void receivedpatchstatus(byte[] value) {
	Log.i(LOG_ID,"receivedpatchstatus");
//	com.adc.trident.app.frameworks.mobileservices.libre3.libre3BLESensor.timerPatchStatus=null;
//	byte[] value=character.getValue();
	byte[] decr= intDecrypt(cryptptr,2,value);
//         var status=new com.adc.trident.app.frameworks.mobileservices.libre3.libre3DPCRLInterface$ABT_PatchStatus(this);
 //       int ret=com.adc.trident.app.frameworks.mobileservices.libre3.libre3DPCRLInterface::DPProcessPatchStatus(decr,status);
	int res=Natives.libre3processpatchstatus(sensorptr,decr);
    short currentLifeCount= (short) (res&0xFFFF);
    short index= (short) (res>>16);

	if(currentLifeCount<0)  {
		Log.e(LOG_ID,"currentLifeCount<0");
		return;
		}
		/*
	if(isNotificationSuspended) {
		pendingEvents.add(MSLibre3PatchStatusEvent);
		}
	currentGlucoseDateTime=com.adc.trident.app.frameworks.core.AppClock.getCurrentTime();
	*/
	if(!backFillInProgress) {
		int backFillStartLifeCount=currentLifeCount;
		int backFillStartHistoricLifeCount= ((backFillStartLifeCount-16)/5)*5;
		fillHistory(backFillStartHistoricLifeCount);
		fillClinical(backFillStartLifeCount);
		
		int getevent=index+1;
		if(getevent>lastEventReceived) {
	//		var command=new byte[7]; DPGetEventLogCommand(lastEventReceived,command);
			byte[] command=Natives.libre3EventLogControl(lastEventReceived);
			qsendcommand(command);
			}
			/*
		if(firstConnect) {
			byte command[]={6,0,0,0,0,0,0};
			qsendcommand(command);
			} */
		}
	}
}

