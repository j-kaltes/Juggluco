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
import android.os.SystemClock;

//import java.security.SecureRandom;
import java.io.File;
import java.lang.reflect.Method;
import java.util.Queue;
import java.util.UUID;
import java.util.concurrent.ConcurrentLinkedQueue;

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
    private boolean subscriptionsReady = false;
    private boolean isServicesDiscovered = false;
    private final long sensorptr;
    private long securityContext=0L;
    // Zero selects the existing Libre 3 implementation; Lingo uses JCA crypto.
    private int lingoSecurityVersion=0;
    private LingoSKB lingoskb=null;
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
synchronized void free() {
    // Garmin handoff now uses only saved sensor data. Normal destruction must
    // cancel pending recovery before releasing this callback's handles.
    stop=true;
    mActiveBluetoothDevice=null;
    if(onalarm!=null) {
        try {
            AlarmManager manager=(AlarmManager)app.getSystemService(ALARM_SERVICE);
            if(manager!=null) manager.cancel(onalarm);
            }
        catch(Throwable th) { Log.stack(LOG_ID,SerialNumber+" cancel reconnect alarm",th); }
        onalarm=null;
        }
    super.free();
    {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"free");};};
    var security=securityContext;
    securityContext=0L;
    lingoskb=null;
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
// All GATT/lifecycle work is serialized on this callback. Checking only the
// GATT identity without that serialization leaves a race with close()/free().
private boolean checkBluetoothGatt(BluetoothGatt gatt) {
    if(gatt != null && gatt == mBluetoothGatt && !stop && dataptr != 0L)
        return true;
    info("ignore stale callback gatt="+gatt+" current="+mBluetoothGatt+" session="+session);
    return false;
    }

private static final long CONNECT_TIMEOUT_MS=75000L;
private static final long CONNECT_RETRY_WAIT_MS=5000L;
private static final long SENSOR_MINUTE_MS=60000L;
private static final long CONNECT_ADVANCE_MS=5000L;
private static final long SETUP_TIMEOUT_MS=90000L;
private static final long SETUP_WAKELOCK_MS=SETUP_TIMEOUT_MS+15000L;
// Exact alarm recovery is opt-in. Android's own autoConnect path must never
// be replaced by application-managed retries.
private boolean timedRecoveryEnabled() {
    return !autoconnect && Natives.getalarmclock();
    }
private PendingIntent onalarm=null;
private final GattRecoveryAlarm recoveryAlarm=new GattRecoveryAlarm(SerialNumber);
private volatile long recoveryGeneration=0L;
private long session=0L;
private long attemptStarted=0L;
private long lastConnectedElapsed=-1L;
private long pendingSince=0L;
private int failedAttempts=0;
private String phase="idle";
private String pendingOperation="none";
private UUID pendingDescriptor=null;
private UUID pendingWrite=null;
private boolean firstMinuteHandled=false;
private boolean sensorStatusHandled=false;
private boolean sessionSucceeded=false;
private PowerManager.WakeLock setupWakeLock=null;

private void pending(String operation) {
    pendingOperation=operation;
    pendingSince=SystemClock.elapsedRealtime();
    }

private boolean receptionEnabled() {
    return !stop && dataptr!=0L && SensorBluetooth.blueone!=null &&
            Natives.getusebluetooth() && Natives.activeSensor(sensorptr) &&
            SensorBluetooth.bluetoothIsEnabled();
    }

private void cancelRecovery() {
    ++recoveryGeneration;
    recoveryAlarm.cancel();
    }

private boolean scheduleRecoveryEvent(long delay,Runnable action) {
    cancelRecovery();
    final long generation=recoveryGeneration;
    boolean scheduled=recoveryAlarm.schedule(Math.max(0L,delay),() -> {
        synchronized(Libre3GattCallback.this) {
            if(generation!=recoveryGeneration) return;
            if(!timedRecoveryEnabled()) { cancelRecovery(); return; }
            if(!receptionEnabled()) { close(); return; }
            action.run();
            }
        });
    if(!scheduled) {
        Log.e(LOG_ID,SerialNumber+": cannot schedule "+phase+" recovery; existing loss-of-signal alarm remains active");
        // In particular, never silently substitute a Java timer on Wear OS.
        phase="idle"; // Allow the existing loss-of-signal path to try again.
        }
    return scheduled;
    }

private void acquireSetupWakeLock() {
    releaseSetupWakeLock("new connection");
    try {
        PowerManager pm=(PowerManager)app.getSystemService(POWER_SERVICE);
        setupWakeLock=pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK,"Juggluco::Libre3Setup");
        setupWakeLock.setReferenceCounted(false);
        setupWakeLock.acquire(SETUP_WAKELOCK_MS);
        info("session="+session+" setup wake lock acquired; idle="+
                (Build.VERSION.SDK_INT>=23 && pm.isDeviceIdleMode())+" exempt="+
                (Build.VERSION.SDK_INT<23 || pm.isIgnoringBatteryOptimizations(app.getPackageName())));
        }
    catch(Throwable th) { Log.stack(LOG_ID,SerialNumber+" acquire setup wake lock",th); }
    }

private void releaseSetupWakeLock(String reason) {
    final var lock=setupWakeLock;
    setupWakeLock=null;
    if(lock!=null) {
        try { if(lock.isHeld()) lock.release(); }
        catch(Throwable th) { Log.stack(LOG_ID,SerialNumber+" release setup wake lock",th); }
        info("session="+session+" setup wake lock released: "+reason);
        }
    }

private void resetSession() {
    connected=false;
    isServicesDiscovered=false;
    subscriptionsReady=false;
    firstMinuteHandled=false;
    sensorStatusHandled=false;
    sessionSucceeded=false;
    pendingDescriptor=null;
    pendingWrite=null;
    oneMinuteReadingSize=0;
    backFillInProgress=false;
    wrotecharacter=false;
    savedAuthChallengeSent=false;
    wrtData=null;
    wrtOffset=0;
    rdtData=null;
    rdtBytes=rdtLength=0;
    rdtSequence=-1;
    sendqueue.clear();
    }

@Override
public synchronized void close() {
    // close() detaches the GATT before its DISCONNECTED callback can update
    // Sensors. Record a local connection end here, once, in the Connection row.
    if(mBluetoothGatt!=null && connected) {
        constatchange[1]=System.currentTimeMillis();
        setConStatus(GATT_SUCCESS);
        }
    cancelRecovery();
    releaseSetupWakeLock("close");
    resetSession();
    phase="idle";
    pending("none");
    super.close();
    }

@Override
public synchronized long resetdataptr() {
    // Base resetdataptr frees the old native handle before calling close().
    close();
    return super.resetdataptr();
    }

@Override
synchronized void finishSensor() {
    stop=true;
    close();
    if(dataptr!=0L) super.finishSensor();
    }

@Override
public synchronized void setDeviceAddress(String address) {
    if(!stop && dataptr!=0L) super.setDeviceAddress(address);
    }

@Override
public synchronized void setDevice(BluetoothDevice device) {
    if(!stop && dataptr!=0L) super.setDevice(device);
    }

@Override
public synchronized void searchforDeviceAddress() {
    if(!stop && dataptr!=0L) super.searchforDeviceAddress();
    }

// handleGlucoseResult -> othersworking must not acquire another sensor's
// monitor while holding this one (two simultaneous readings could deadlock).
@Override
void shouldreconnect(long now) {
    if(!timedRecoveryEnabled()) {
        super.shouldreconnect(now);
        return;
        }
    final long generation=recoveryGeneration;
    Applic.scheduler.execute(() -> {
        synchronized(Libre3GattCallback.this) {
            if(generation==recoveryGeneration && !stop && dataptr!=0L)
                super.shouldreconnect(now);
            }
        });
    }

private void setupComplete(String reason) {
    if(sessionSucceeded || !subscriptionsReady) return;
    sessionSucceeded=true;
    failedAttempts=0;
    phase="receiving";
    cancelRecovery();
    releaseSetupWakeLock(reason);
    info("session="+session+" setup complete: "+reason+" elapsed="+
            (SystemClock.elapsedRealtime()-attemptStarted)+"ms");
    }

private void setupDeadline() {
    // Warmup/temporarily unavailable glucose can still have a fully working,
    // authenticated link. A decoded patch status must not cause a retry loop.
    if(subscriptionsReady && sensorStatusHandled) {
        setupComplete("authenticated sensor status; no current minute yet");
        return;
        }
    recover(mBluetoothGatt,"setup deadline",false,0L);
    }

private void recover(BluetoothGatt gatt,String reason,boolean wasWorking,long minimumDelay) {
    if(gatt==null || gatt!=mBluetoothGatt) return;
    if(!timedRecoveryEnabled()) {
        setfailure(reason);
        try { gatt.disconnect(); }
        catch(Throwable th) {
            Log.stack(LOG_ID,SerialNumber+" disconnect after "+reason,th);
            normalDisconnected(gatt,-1,System.currentTimeMillis());
            }
        return;
        }
    if(connected)
        setfailure(reason);
    else {
        constatchange[1]=System.currentTimeMillis();
        constatstatusstr=reason;
        }
    recoverConnection(gatt,reason,wasWorking,minimumDelay);
    }

// Connection events and routine disconnects must not replace Handshake details.
private void recoverConnection(BluetoothGatt gatt,String reason,boolean wasWorking,long minimumDelay) {
    if(gatt==null || gatt!=mBluetoothGatt) return;
    final long elapsed=SystemClock.elapsedRealtime()-attemptStarted;
    final boolean waitedForConnection=phase.equals("connecting") && elapsed>=CONNECT_RETRY_WAIT_MS;
    Log.e(LOG_ID,SerialNumber+": recovery session="+session+" phase="+phase+
            " pending="+pendingOperation+" age="+(SystemClock.elapsedRealtime()-pendingSince)+
            "ms reason="+reason);
    close(); // Detach now: do not wait for DISCONNECTED to schedule recovery.
    if(!receptionEnabled()) return;
    final long spacing;
    if(wasWorking) {
        failedAttempts=0;
        spacing=0L;
        }
    else {
        failedAttempts=Math.min(failedAttempts+1,3);
        spacing=5000L << (failedAttempts-1); // 5, 10, then 20 seconds.
        }
    // A timed-out connection attempt must be replaced immediately. In particular,
    // a 10-second timeout plus a 10-second gap can miss the same part of every
    // sensor minute forever. Keep backoff for rapid failures and setup errors.
    long delay=Math.max(minimumDelay,Math.max(0L,spacing-elapsed));
    if(waitedForConnection) delay=minimumDelay;
    if(wasWorking && isWearable && Natives.getDisconnectSensor() && lastConnectedElapsed>=0L) {
        final long sinceConnected=SystemClock.elapsedRealtime()-lastConnectedElapsed;
        // Only predict the next minute of a recent working connection. If that
        // deadline has passed, try now and use continuous timeout recovery.
        if(sinceConnected>=0L && sinceConnected<SENSOR_MINUTE_MS-CONNECT_ADVANCE_MS)
            delay=Math.max(delay,SENSOR_MINUTE_MS-CONNECT_ADVANCE_MS-sinceConnected);
        }
    info("session="+session+" failures="+failedAttempts+" retry in "+delay+"ms");
    phase="retry";
    if(!scheduleRecoveryEvent(delay,this::startConnection)) startConnection();
    }

private boolean enableRequiredNotification(BluetoothGattCharacteristic characteristic) {
    final var gatt=mBluetoothGatt;
    if(!checkBluetoothGatt(gatt)) return false;
    pendingDescriptor=characteristic==null?null:characteristic.getUuid();
    pending("descriptor "+pendingDescriptor);
    if(characteristic!=null && enableNotification(gatt,characteristic)) return true;
    recover(gatt,"notification request rejected: "+pendingDescriptor,false,0L);
    return false;
    }

private boolean writeRequiredCharacteristic(BluetoothGattCharacteristic characteristic) {
    final var gatt=mBluetoothGatt;
    if(!checkBluetoothGatt(gatt)) return false;
    pendingWrite=characteristic.getUuid();
    pending("write "+pendingWrite);
    try {
        if(gatt.writeCharacteristic(characteristic)) return true;
        }
    catch(Throwable th) { Log.stack(LOG_ID,SerialNumber+" writeCharacteristic",th); }
    recover(gatt,"characteristic write rejected: "+pendingWrite,false,0L);
    return false;
    }

private void startConnection() {
    if(!receptionEnabled()) { close(); return; }
    if(mActiveBluetoothDevice==null || mActiveDeviceAddress==null) {
        phase="idle";
        foundtime=0L;
        Applic.scheduler.execute(SensorBluetooth::reconnectall);
        return;
        }
    // SuperGattCallback owns connectGatt(), its BluetoothGatt lifecycle,
    // autoConnect and priority handling. Do not duplicate any of those here.
    ++session;
    if(!super.connectDevice(0)) {
        connectionStartFailed("connectDevice rejected");
        return;
        }
    attemptStarted=SystemClock.elapsedRealtime();
    phase="connecting";
    pending("connectGatt");
    info("session="+session+" connectGatt autoconnect="+autoconnect);
    scheduleRecoveryEvent(CONNECT_TIMEOUT_MS,() -> {
        if(mBluetoothGatt!=null)
            recover(mBluetoothGatt,"connection deadline",false,0L);
        else
            connectionStartFailed("connection deadline");
        });
    }

private void connectionStartFailed(String reason) {
    close();
    constatchange[1]=System.currentTimeMillis();
    constatstatusstr=reason;
    if(!receptionEnabled()) return;
    failedAttempts=Math.min(failedAttempts+1,3);
    long delay=5000L << (failedAttempts-1);
    phase="retry";
    info("session="+session+" "+reason+"; retry in "+delay+"ms");
    if(!scheduleRecoveryEvent(delay,this::startConnection)) {
        phase="idle";
        Applic.scheduler.execute(SensorBluetooth::reconnectall);
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
public synchronized void onConnectionUpdated(BluetoothGatt gatt, int interval, int latency, int timeout, int status) {
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
    public synchronized void onSubrateChange( @NonNull BluetoothGatt gatt,  int subrateMode,  int status) {
     if(doLog) {
        Log.i(LOG_ID,"onSubrateChange  subrateMode="+subrateMode+" status="+status);
        }
    }


    @Override 
    public synchronized void onCharacteristicRead( @NonNull BluetoothGatt gatt, @NonNull BluetoothGattCharacteristic characteristic, @NonNull byte[] value, int status) {
            if(!checkBluetoothGatt(gatt)) return;
            if(doLog)
                showbytes(LOG_ID + " "+SerialNumber+" onCharacteristicRead status="+status+" " + characteristic.getUuid().toString(), value);

    }


    @Override 
    public synchronized void onCharacteristicRead(BluetoothGatt bluetoothGatt, BluetoothGattCharacteristic bluetoothGattCharacteristic, int status) {
        if(!checkBluetoothGatt(bluetoothGatt)) return;
        if(doLog)
            {showbytes(LOG_ID + " "+SerialNumber+" onCharacteristicRead status="+status+" " + bluetoothGattCharacteristic.getUuid().toString(), bluetoothGattCharacteristic.getValue());}

       /* if(bluetoothGattCharacteristic.getUuid().equals(LIBRE3_CHAR_PATCH_STATUS)) {
            } */
    }

    @Override 
    public synchronized void onCharacteristicWrite(BluetoothGatt bluetoothGatt, BluetoothGattCharacteristic bluetoothGattCharacteristic, int i2) {
        if(!checkBluetoothGatt(bluetoothGatt)) return;
        if(doLog)
            showCharacter(LOG_ID + " "+SerialNumber+" onCharacteristicWrite " , bluetoothGattCharacteristic);

        if(i2!=GATT_SUCCESS) {
            recover(bluetoothGatt,"characteristic callback status="+i2,false,0L);
            return;
            }
        if(!bluetoothGattCharacteristic.getUuid().equals(pendingWrite)) return;
        pendingWrite=null;
        pending("security/control response");
        oncharwrite(bluetoothGattCharacteristic);
        if(checkBluetoothGatt(bluetoothGatt)) fromqueue();
//        var value = bluetoothGattCharacteristic.getValue();
 //       {if(doLog){showbytes(LOG_ID + " "+SerialNumber+" onCharacteristicWrite " + bluetoothGattCharacteristic.getUuid().toString(), value);};}
    }

//    private boolean wasConnected = false;
private boolean connected=false;

//private    boolean waitingForMtu=false;
//private int updated=0;
    @SuppressLint("MissingPermission")
    @Override 
    public synchronized void onConnectionStateChange(BluetoothGatt bluetoothGatt, int status, int newState) {
        if(!acceptConnectionStateChange(bluetoothGatt,newState)) return;
        if(!checkBluetoothGatt(bluetoothGatt)) return;
        long tim=System.currentTimeMillis();
        info(" onConnectionStateChange, status:"+status+", state: "+
                (newState==STATE_CONNECTED?"CONNECTED":newState==STATE_DISCONNECTED?"DISCONNECTED":Integer.toString(newState)));
        info("session="+session+" onConnectionStateChange status="+status+" state="+newState);
        if(stop || dataptr==0L || (timedRecoveryEnabled() && !receptionEnabled())) { close(); return; }
        if(newState==STATE_CONNECTED && status==GATT_SUCCESS) {
            if(connected) return;
            connected=true;
            lastConnectedElapsed=SystemClock.elapsedRealtime();
            phase="setup";
            constatchange[0]=tim;
            if(timedRecoveryEnabled()) {
                acquireSetupWakeLock();
                scheduleRecoveryEvent(SETUP_TIMEOUT_MS,this::setupDeadline);
                }
            try {
                setpriority(bluetoothGatt);
                if(timedRecoveryEnabled()) startServices(bluetoothGatt);
                else if(!isServicesDiscovered || !getservices()) {
                    if(!bluetoothGatt.discoverServices())
                        Log.e(LOG_ID, SerialNumber + ": "+"discoverServices()  failed");
                    else if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"discoverServices() success");};
                    }
                }
            catch(Throwable th) {
                Log.stack(LOG_ID,SerialNumber+" start services",th);
                recover(bluetoothGatt,"service discovery exception",false,0L);
                }
            }
        else if(newState==STATE_DISCONNECTED || status!=GATT_SUCCESS) {
            connected=false; // Preserve the reported status when close() runs.
            constatchange[1]=tim;
            setConStatus(status);
            if(savedAuthChallengeSent && !subscriptionsReady && status==19 && tim-datatime>=59000L) {
                isPreAuthorized=false;
                Natives.setLibre3kAuth(sensorptr,null);
                }
            if(!timedRecoveryEnabled())
                normalDisconnected(bluetoothGatt,status,tim);
            else {
                long delay=isWearable && Natives.getDisconnectSensor() ? Math.max(0L,5000L-(tim-datatime)) : 0L;
                recoverConnection(bluetoothGatt,"onConnectionStateChange status="+status+" state="+newState,sessionSucceeded,delay);
                }
            }
        }

        @Override 
        public synchronized void onDescriptorRead(BluetoothGatt bluetoothGatt, BluetoothGattDescriptor bluetoothGattDescriptor, int status) {
        {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+ "onDescriptorRead status="+status);};};
        }


private void normalDisconnected(BluetoothGatt gatt,int status,long tim) {
    info("disconnected "+status);
    cancelRecovery();
    releaseSetupWakeLock("disconnected");
    resetSession();
    phase="idle";
    init();
    if(autoconnect) {
        // Re-use the original GATT and Android's autoConnect reconnection.
        try { gatt.connect(); }
        catch(Throwable th) { Log.stack(LOG_ID,SerialNumber+" reconnect",th); }
        return;
        }
    if(!closeCurrentGatt(gatt)) return;
    if(isWearable && Natives.getDisconnectSensor()) {
        // Original Wear OS short delay, independent of the optional new timer.
        long alreadywaited=tim-datatime;
        long stillwait=60000L-alreadywaited-55000L;
        if(doLog) {Log.i(LOG_ID,"alreadywaited="+alreadywaited+" stillwait="+stillwait);};
        if(stillwait>0L) {
            onalarm=setalarm(tim+stillwait,onalarm,SerialNumber);
            if(onalarm==null) super.connectDevice(0);
            }
        else super.connectDevice(0);
        }
    else super.connectDevice(0);
    }

private void startServices(BluetoothGatt gatt) {
    pending("discoverServices");
    if(!gatt.discoverServices())
        recover(gatt,"discoverServices rejected",false,0L);
    }

        @Override 
        public synchronized void onDescriptorWrite(BluetoothGatt bluetoothGatt, BluetoothGattDescriptor bluetoothGattDescriptor, int status) {
        if(!checkBluetoothGatt(bluetoothGatt)) return;
           // libre3BLESensor.access$1900(libre3blesensor, characteristic, status);
        {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+ "onDescriptorWrite status="+status);};};
        if(status!=GATT_SUCCESS) {
            recover(bluetoothGatt,"descriptor callback status="+status,false,0L);
            return;
            }
        BluetoothGattCharacteristic characteristic = bluetoothGattDescriptor.getCharacteristic();
        if(!characteristic.getUuid().equals(pendingDescriptor)) return;
        pendingDescriptor=null;
        pending("authentication/first glucose");
        handleonDescriptorWrite(characteristic);
        if(checkBluetoothGatt(bluetoothGatt)) fromqueue();
        }

        @Override // android.bluetooth.BluetoothGattCallback
        public synchronized void onMtuChanged(BluetoothGatt bluetoothGatt, int mtu, int status) {
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
        public synchronized void onReadRemoteRssi(BluetoothGatt bluetoothGatt, int rssi, int status) {
        if(!checkBluetoothGatt(bluetoothGatt)) return;
        readrssi=status==GATT_SUCCESS?rssi:999;
        info("RSSI="+readrssi+" status="+status);
        }

        @Override // android.bluetooth.BluetoothGattCallback
     public synchronized void onServicesDiscovered(BluetoothGatt bluetoothGatt, int status) {
        if(!checkBluetoothGatt(bluetoothGatt)) return;
        if(isServicesDiscovered) return;
          {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+ "onServicesDiscovered status="+status);};};
          if (status == GATT_SUCCESS) {
                if(!getservices()) {
                  recover(bluetoothGatt,"required services/characteristics missing",false,0L);
                  }
              }
             else {
                Log.e(LOG_ID, SerialNumber + ": "+ "BLE: onServicesDiscovered error: " + status);
               recover(bluetoothGatt,"service discovery callback status="+status,false,0L);
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
            return Integer.MAX_VALUE;
        }
        int i2 = value[0] & 0xFF;
        if (i2 != rdtSequence + 1) {
            var message= "getsecdata secu Sequence=" + i2 + "!=" + rdtSequence + "-1 (rdtSequence-1)";
            Log.e( LOG_ID, SerialNumber + ": "+ message);
        setfailure(message);
        dodisconnect(mBluetoothGatt);
            return Integer.MAX_VALUE;
        }
        info("getsecdata num=" + i2 + " rdtSequence=" + rdtSequence);
        int length = value.length - 1;
        if(rdtData==null || rdtBytes+length>rdtLength) {
            recover(mBluetoothGatt,"invalid security fragment length",false,0L);
            return Integer.MAX_VALUE;
            }
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



var encrypted = lingoskb!=null ? lingoskb.encrypt(nonce1,uit) : Natives.libre3EncryptChallengeReply(securityContext,nonce1,uit);
    if(encrypted==null || encrypted.length!=40) {
        recover(mBluetoothGatt,"challenge encryption failed",false,0L);
        return;
        }






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
    byte[] decr=lingoskb!=null ? lingoskb.decrypt(nonce,first) : Natives.libre3DecryptChallengeResponse(securityContext,nonce,first);
    if(decr==null || decr.length!=56) {
        recover(mBluetoothGatt,"challenge authentication failed",false,0L);
        return;
        }
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
    byte[] savedAuthorization=lingoskb!=null ? lingoskb.exportAuthorizationKey() : Natives.libre3ExportSavedAuthorization(securityContext);
    if(savedAuthorization==null || savedAuthorization.length!=149) {
        recover(mBluetoothGatt,"authorization export failed",false,0L);
        return;
        }
    Log.showbytes("challenge67 savedAuthorization",savedAuthorization);
    //securityContext=new BCrypt(kEnc,ivEnc);
    cryptptr=initcrypt(cryptptr,kEnc,ivEnc);
    if(cryptptr==0L) {
        recover(mBluetoothGatt,"data cipher initialization failed",false,0L);
        return;
        }
    Natives.setLibre3kAuth(sensorptr,savedAuthorization);
    // A newly scanned sensor can now be provisioned without retaining this GATT.
    SensorLifecycle.changed();
    enableRequiredNotification(gattCharPatchDataControl);
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
    if(dataptr==0L || Natives.getLingoSecurityVersion(sensorptr)!=0) return null;
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
        if(sensorptr==0L || !Natives.activeSensor(sensorptr) ||
                Natives.getLingoSecurityVersion(sensorptr)!=0) return null;
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
public synchronized void stopForGarmin() {
    if(!isWearable) {
        stop=true;
        // Also defeats a connectDevice Runnable that may already have been queued.
        mActiveBluetoothDevice=null;
        close();
        }
    }

@Override
public synchronized boolean reconnect(long now,long delay) {
    if(!timedRecoveryEnabled()) return super.reconnect(now,delay);
    if(!receptionEnabled()) { close(); return true; }
    if(!phase.equals("idle") && !phase.equals("receiving")) return true;
    final long old=now-showtime+20;
    if(charcha[1]<old && connectTime<(now-60000L)) {
        if(mBluetoothGatt!=null) {
            connected=false;
            constatchange[1]=now;
            constatstatusstr="Loss of signal";
            recoverConnection(mBluetoothGatt,"loss of signal",sessionSucceeded,delay);
            }
        else
            return connectDevice(delay);
        }
    return true;
    }

@Override
public synchronized boolean connectDevice(long delayMillis) {
    if(!timedRecoveryEnabled()) return super.connectDevice(delayMillis);
    if(!receptionEnabled()) { close(); return true; }
    if(mActiveDeviceAddress==null || mActiveBluetoothDevice==null) {
        foundtime=0L;
        return false;
        }
    // Duplicate scan/age events must not defer an already armed retry.
    if(!phase.equals("idle") && !phase.equals("receiving")) return true;
    close();
    phase="retry";
    if(delayMillis<=0L) startConnection();
    else if(!scheduleRecoveryEvent(delayMillis,this::startConnection)) startConnection();
    return true;
    }

@Override
public synchronized void disconnect() {
    if(!timedRecoveryEnabled()) {
        super.disconnect();
        return;
        }
    if(mBluetoothGatt!=null) {
        long delay=isWearable && Natives.getDisconnectSensor() ?
                Math.max(0L,5000L-(System.currentTimeMillis()-datatime)) : 0L;
        recoverConnection(mBluetoothGatt,"disconnect requested",sessionSucceeded,delay);
        }
    else close();
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
    if(!writeRequiredCharacteristic(gattCharCommandResponse)) {
        var message="writeCharacteristic(gattCharCommandResponse) failed "+b;
        Log.e(LOG_ID, SerialNumber + ": "+ message);
        setfailure(message);  
        dodisconnect(mBluetoothGatt); //TODO: or try again?
        return false;
        }
    if(b==8 && isPreAuthorized) savedAuthChallengeSent=true;
    return true; 
    }
private int commandphase=1;
private void setCertificate140() {
    {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"setCertificate140");};};
    if(lingoskb!=null) {
        if(!lingoskb.setPatchCertificate(rdtData)) {
            recover(mBluetoothGatt,"Lingo sensor certificate verification failed",false,0L);
            return;
            }
        }
    else cryptolib.setPatchCertificate(securityContext,rdtData);
//    Libre3Emulator.captureCertificate(SerialNumber,rdtData);
    if(sendSecurityCommand( (byte)0x0D)) {
        commandphase=4;
        }
    }
private boolean    generateKAuth(byte[] input) {
    {if(doLog){showbytes(LOG_ID+ " "+SerialNumber +" generateKAuth",input);};}
    //Saves something?
    return lingoskb!=null ? lingoskb.generateKAuth(input) : (Natives.libre3DeriveAuthorizationRoot(securityContext,input)==1);
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
         enableRequiredNotification(gattCharCommandResponse);
         }
  }
//19156 00000 00013 01036 19156 00019






private    void save_history(byte[] value) {
    byte[] olddec=intDecrypt(cryptptr,4, value);
    if(lingoSecurityVersion!=0)
        Natives.saveLingoHistory(this.sensorptr, olddec);
    else Natives.saveLibre3History(this.sensorptr, olddec);
    }
@Override 
public synchronized void onCharacteristicChanged(BluetoothGatt bluetoothGatt, BluetoothGattCharacteristic bluetoothGattCharacteristic) {
    if(!checkBluetoothGatt(bluetoothGatt)) return;
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
public synchronized void onCharacteristicChanged(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, byte[] value) {
       final long nowmsec= System.currentTimeMillis();
       if(!checkBluetoothGatt(gatt)) return;
       PowerManager.WakeLock wakelock=null;
       try {
           if(Applic.usewakelock) {
               wakelock=((PowerManager)app.getSystemService(POWER_SERVICE)).newWakeLock(PowerManager.PARTIAL_WAKE_LOCK,"Juggluco::Libre3");
               wakelock.acquire(30000L);
               }
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
       }
       catch(Throwable th) {
           Log.stack(LOG_ID,SerialNumber+" notification session="+session,th);
           recover(gatt,"notification processing exception",false,0L);
           }
       finally {
           if(wakelock!=null && wakelock.isHeld()) wakelock.release();
           }
    }



//source /n/ojka/tmp/libre3.3.0/sensor/newsensor/working


private    void fast_data(byte[] encryp) {
    byte[] decr=intDecrypt(cryptptr,5,encryp);
        if (decr == null) {
            info("fast_data decrypt went wrong"); 
            dodisconnect(mBluetoothGatt); 
        } else {
            if(lingoSecurityVersion!=0)
                Natives.saveLingoFastData(sensorptr, decr);
            else Natives.saveLibre3fastData(sensorptr, decr);
        }
    }

private final KEYSCrypto cryptolib=new KEYSCrypto();
//int    securityState=0;
private boolean    isPreAuthorized=false;
private void onConnectGatt() {
    isPreAuthorized=false;
    }
private synchronized boolean initSecurityKeys(byte[] savedAuthorization,int level) {
    if(lingoSecurityVersion!=0) {
        if(lingoskb==null)
            lingoskb=LingoSKB.create(Applic.getContext(),SerialNumber);
        return lingoskb!=null && lingoskb.initECDH(savedAuthorization,lingoSecurityVersion);
        }
    lingoskb=null;
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
    // NFC rescanning clears the stored authorization, but may reuse this
    // callback object. Reload at the start of each authentication so an old
    // in-memory root cannot override that explicit request for fresh pairing.
    savedAuthChallengeSent=false;
    isPreAuthorized=false;
    resolveLingoVersion();
    var exportedKAuth = Natives.getLibre3kAuth(sensorptr);
    if(lingoSecurityVersion!=0 && exportedKAuth!=null &&
            !LingoSKB.canResume(exportedKAuth,lingoSecurityVersion)) {
        // Keep the old record until a valid challenge completes fresh pairing.
        exportedKAuth=null;
        }
    if(initSecurityKeys(exportedKAuth,1)) {
        if(exportedKAuth==null) {
            {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"exportedKAuth==null");};};
            commandphase=1;
            sendSecurityCommand(1);
            }
        else  {
            {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"exportedKAuth!=null");};};
            isPreAuthorized=true;
            commandphase=5;
            sendSecurityCommand(17);
            }
        }
    else recover(mBluetoothGatt,"security initialization failed",false,0L);

    }
private void logevent(byte[] value) {
    byte[] decr=intDecrypt(cryptptr,6,value);
    int last=Natives.libre3EventLog(sensorptr,decr);
    if(last<0)
            return;
    lastEventReceived=last;
    }
private void init() {
    resolveLingoVersion();
    var exportedKAuth = Natives.getLibre3kAuth(sensorptr);
    if(lingoSecurityVersion!=0 && !LingoSKB.canResume(exportedKAuth,lingoSecurityVersion))
        exportedKAuth=null;
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

private void resolveLingoVersion() {
    int version=Natives.getLingoSecurityVersion(sensorptr);
    if(version!=lingoSecurityVersion) {
        lingoSecurityVersion=version;
        lingoskb=null;
        Natives.libre3FreeSecurityContext(securityContext);
        securityContext=0L;
        oneMinuteRawData=new byte[version!=0?57:35];
        oneMinuteReadingSize=0;
        }
    }

private void dodisconnect(BluetoothGatt gatt) {
    if(!timedRecoveryEnabled()) {
        Log.e(LOG_ID, SerialNumber + ": "+"disconnect()");
        if(gatt!=null) gatt.disconnect();
        }
    else recover(gatt,"protocol/setup failure: "+handshake,false,0L);
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
            enableRequiredNotification(gattCharEventLog);
        } else {
            if (LIBRE3_CHAR_EVENT_LOG.equals(uuid)) {
                enableRequiredNotification(gattCharHistoricData);
            } else {
                if (LIBRE3_CHAR_HISTORIC_DATA.equals(uuid)) {
                    enableRequiredNotification(gattCharClinicalData);
                } else {
                    if (LIBRE3_CHAR_CLINICAL_DATA.equals(uuid)) {
                        enableRequiredNotification(gattCharFactoryData);
                    } else {
                        if (LIBRE3_CHAR_FACTORY_DATA.equals(uuid)) {
                            enableRequiredNotification(gattCharGlucoseData);
                        } else {
                            if (LIBRE3_CHAR_GLUCOSE_DATA.equals(uuid)) {
                                enableRequiredNotification(gattCharPatchStatus);
                            /*
                               switch(resetGlucose) {
                                case 0: enableRequiredNotification(gattCharPatchStatus);break;
                                case 1: enableRequiredNotification(gattCharGlucoseData);++resetGlucose;break;
                                default: resetGlucose=0; break;
                                };
                                */
                            } else {
                                if (LIBRE3_CHAR_PATCH_STATUS.equals(uuid)) {
                                    subscriptionsReady=true;
                                    pending("first current-glucose packet");
                                    if(firstMinuteHandled) setupComplete("first current-glucose packet handled");
                                } else {
                                    if (LIBRE3_SEC_CHAR_COMMAND_RESPONSE.equals(uuid)) {
                                        enableRequiredNotification(gattCharCertificateData);
                                        //enableRequiredNotification(gattCharCertificateData);


                                    } else {
                                        if (LIBRE3_SEC_CHAR_CERT_DATA.equals(uuid)) {
                                            enableRequiredNotification(gattCharChallengeData);
                                            //enableRequiredNotification(gattCharChallengeData);


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
        if(value.length==0) { recover(mBluetoothGatt,"empty security response",false,0L); return; }
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
            if(!bluetoothGattCharacteristic.setValue(bArr) ||
                    !bluetoothGattCharacteristic.setValue(this.wrtOffset, 18, 0)) {
                recover(mBluetoothGatt,"security fragment setValue failed",false,0L);
                return 0;
                }
            this.wrtOffset += min;
            if(writeRequiredCharacteristic(bluetoothGattCharacteristic))
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
    if(lingoskb!=null) return lingoskb.generateEphemeralKeys();

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


// Set when the saved-authorization challenge is submitted, not when the
// command-write callback arrives. Notifications may precede write callbacks.
private boolean savedAuthChallengeSent=false;

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
                    if(sendSecurityCert(lingoskb!=null ? lingoskb.getAppCertificate() : cryptolib.getAppCertificate())) { //TODO what with failure?
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
        if (z || z2 || gattCharPatchDataControl==null || gattCharPatchStatus==null ||
                gattCharEventLog==null || gattCharGlucoseData==null || gattCharHistoricData==null ||
                gattCharClinicalData==null || gattCharFactoryData==null || gattCharCommandResponse==null ||
                gattCharChallengeData==null || gattCharCertificateData==null) {
              {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"getservices failure");};};
        isServicesDiscovered = false;
            return false;
        }
        isServicesDiscovered = true;
        // RSSI is optional. Read it after a minute packet, never before auth.
        enablegattCharCommandResponse();
       {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"getservices success");};};
       return true;
    }



    private int oneMinuteReadingSize = 0;
//    private int oneMinutePacketNumber = 0;
    private byte[] oneMinuteRawData = new byte[35];

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

        if(len==0 || oneMinuteReadingSize+len>oneMinuteRawData.length) {
            recover(mBluetoothGatt,"invalid current-glucose fragment length",false,0L);
            return;
            }
        System.arraycopy(value, 0, this.oneMinuteRawData, this.oneMinuteReadingSize, len);
        oneMinuteReadingSize +=len;
        if(oneMinuteReadingSize >= oneMinuteRawData.length) {
           this.oneMinuteReadingSize = 0;
           byte[] decr = intDecrypt(cryptptr,3, oneMinuteRawData);
           if(decr == null || decr.length!=(lingoSecurityVersion!=0?51:29)) {
                recover(mBluetoothGatt,"current-glucose decryption failed",false,0L);
                return;
               }
           long res=lingoSecurityVersion!=0 ? Natives.saveLingoMinuteL(this.sensorptr,decr,timmsec) :
                   Natives.saveLibre3MinuteL(this.sensorptr,decr,timmsec);
           handleGlucoseResult(res,timmsec);
           datatime=timmsec;
           firstMinuteHandled=true;
           releaseSetupWakeLock("current-glucose packet handled");
           setupComplete("current-glucose packet handled (including duplicate/unavailable)");
           // Optional: neither a rejection nor a missing callback blocks setup.
           try {
               if(pendingDescriptor==null && pendingWrite==null && !mBluetoothGatt.readRemoteRssi())
                   info("optional RSSI request rejected");
               }
           catch(Throwable th) { Log.stack(LOG_ID,SerialNumber+" optional RSSI",th); }
           }
        if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"end glucose_data");};
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
    if(!gattCharPatchDataControl.setValue(encr)) {
        recover(mBluetoothGatt,"control setValue failed",false,0L);
        return false;
        }
    wrotecharacter=true;
    return writeRequiredCharacteristic(gattCharPatchDataControl);
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
    if(!connected || wrotecharacter || pendingDescriptor!=null || pendingWrite!=null) return false;
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
            byte[] command=lingoSecurityVersion!=0 ? Natives.lingoControlHistory(1,takelast) :
                    Natives.libre3ControlHistory(1,takelast);
            if(qsendcommand(command))
                backFillInProgress=true;
            }
        }
private void    fillClinical(int backFillStartLifeCount) {
      int lastLifeCountReceived=Natives.getlastLifeCountReceived(sensorptr);
      {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"getlastLifeCountReceived(sensorptr)="+lastLifeCountReceived+" backFillStartLifeCount="+ backFillStartLifeCount);};};

      if(lastLifeCountReceived<backFillStartLifeCount) {
        var command=lingoSecurityVersion!=0 ? Natives.lingoClinicalControl(1,lastLifeCountReceived) :
                Natives.libre3ClinicalControl(1,lastLifeCountReceived);
        if(qsendcommand(command))
            backFillInProgress=true;
        }
    }
private void receivedpatchstatus(byte[] value) {
    {if(doLog) {Log.i(LOG_ID, SerialNumber + ": "+"receivedpatchstatus");};};
    byte[] decr= intDecrypt(cryptptr,2,value);
    if(decr==null) { recover(mBluetoothGatt,"patch status decrypt failed",false,0L); return; }
    int res=Natives.libre3processpatchstatus(sensorptr,decr);
    short currentLifeCount= (short) (res&0xFFFF);
    short index= (short) (res>>16);

    if(currentLifeCount<0)  {
        Log.e(LOG_ID, SerialNumber + ": "+"currentLifeCount<0");
        return;
        }
    sensorStatusHandled=true;
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
public synchronized boolean matchDeviceName(String deviceName,String address) {
    if(stop || dataptr==0L) return false;
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
