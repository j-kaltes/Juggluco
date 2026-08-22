/*      This file is part of Juggluco, an Android app to receive and display
 *      glucose values from continuous glucose monitors.
 *
 *      Copyright (C) 2026 Jaap Korthals Altes <jaapkorthalsaltes@gmail.com>
 *
 *      Juggluco is free software: you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation, either version 3 of the License, or
 *      (at your option) any later version.
 */
package tk.glucodata;

import android.Manifest;
import android.annotation.SuppressLint;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattServer;
import android.bluetooth.BluetoothGattServerCallback;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.le.AdvertiseCallback;
import android.bluetooth.le.AdvertiseData;
import android.bluetooth.le.AdvertiseSettings;
import android.bluetooth.le.BluetoothLeAdvertiser;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanFilter;
import android.bluetooth.le.ScanResult;
import android.bluetooth.le.ScanSettings;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.os.ParcelUuid;
import android.os.SystemClock;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AlertDialog;
import androidx.core.content.ContextCompat;

import java.nio.charset.StandardCharsets;
import java.security.SecureRandom;
import java.util.ArrayDeque;
import java.util.Collections;
import java.util.List;
import java.util.Locale;
import java.util.UUID;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * A consent-controlled BLE GATT carrier for the existing mirror byte protocol.
 *
 * The phone normally advertises a GATT server and a Wear OS device scans as a
 * client. Phone-to-phone use is supported by explicitly choosing Offer on one
 * phone and Find on the other. Only one Juggluco BLE mirror peer is stored for
 * now. Android bonding plus encrypted GATT permissions protect the automatic
 * mirror-password exchange performed by the net-info handshake.
 */
public final class BleMirror {
    private static final String LOG_ID="BleMirror";
    private static final UUID SERVICE_UUID=UUID.fromString("e65f6d10-3f4b-4d2a-9f75-6a7567676c75");
    private static final UUID RX_UUID=UUID.fromString("e65f6d11-3f4b-4d2a-9f75-6a7567676c75");
    private static final UUID TX_UUID=UUID.fromString("e65f6d12-3f4b-4d2a-9f75-6a7567676c75");
    private static final UUID CCCD_UUID=UUID.fromString("00002902-0000-1000-8000-00805f9b34fb");
    private static final String HELLO_PATH="/blehello";
    private static final String READY_PATH="/bleready";
    private static final int PROTOCOL_VERSION=1;
    private static final int ROLE_NONE=0;
    private static final int ROLE_SERVER=1;
    private static final int ROLE_CLIENT=2;
    private static final long PAIRING_MILLIS=2L*60L*1000L;
    private static final long HANDSHAKE_TIMEOUT_MILLIS=30L*1000L;
    private static final long FRAGMENT_TIMEOUT_MILLIS=15L*1000L;
    private static final long SEND_TIMEOUT_SECONDS=65L;
    private static final int MAX_QUEUED_BYTES=1024*1024;
    private static final int REQUESTED_MTU=517;
    private static final String PREFS="ble_mirror";
    private static final String PREF_ACTIVE="active";
    private static final String PREF_PREFERRED="preferred";
    private static final String PREF_ROLE="role";
    private static final String PREF_ADDRESS="address";
    private static final String PREF_LINK_ID="link_id";

    private static final Object lock=new Object();
    private static final Handler handler=new Handler(Looper.getMainLooper());
    private static final ExecutorService incoming=Executors.newSingleThreadExecutor(runnable -> {
        final Thread thread=new Thread(runnable,"BLE mirror receive");
        thread.setDaemon(true);
        return thread;
    });
    private static final SecureRandom random=new SecureRandom();
    private static final AtomicInteger nextMessage=new AtomicInteger();
    private static final ArrayDeque<Outbound> outgoing=new ArrayDeque<>();
    private static final BleMirrorCodec.Decoder decoder=new BleMirrorCodec.Decoder();

    private static SharedPreferences preferences;
    private static BluetoothManager bluetoothManager;
    private static BluetoothAdapter adapter;
    private static boolean initialized;
    private static boolean receiverRegistered;
    private static boolean active;
    private static boolean preferred;
    private static int role;
    private static String approvedAddress;
    private static String linkId;
    private static boolean remoteIsWearable;
    private static volatile String status="BLE mirror is off";
    private static long pairingUntil;
    private static boolean scanRunning;
    private static BluetoothDevice bondCandidate;

    private static BluetoothGattServer gattServer;
    private static BluetoothGattCharacteristic serverTx;
    private static BluetoothDevice serverPeer;
    private static BluetoothLeAdvertiser advertiser;
    private static boolean advertising;
    private static boolean serverSubscribed;

    private static BluetoothGatt clientGatt;
    private static BluetoothGattCharacteristic clientRx;
    private static BluetoothLeScanner scanner;
    private static boolean clientSubscribed;
    private static boolean servicesRequested;

    private static boolean ready;
    private static boolean restoringPreferred;
    private static int mtu=23;
    private static int session=1;
    private static int queuedBytes;
    private static Outbound sending;
    private static long connectionEpoch;
    private static long serverGeneration;

    private BleMirror() {}

    public static void init(Context context) {
        synchronized(lock) {
            if(initialized)
                return;
            //app=context.getApplicationContext();
            preferences=Applic.app.getSharedPreferences(PREFS,Context.MODE_PRIVATE);
            active=preferences.getBoolean(PREF_ACTIVE,false);
            preferred=preferences.getBoolean(PREF_PREFERRED,false);
            role=preferences.getInt(PREF_ROLE,ROLE_NONE);
            approvedAddress=preferences.getString(PREF_ADDRESS,null);
            linkId=preferences.getString(PREF_LINK_ID,null);
            if(active&&(approvedAddress==null||linkId==null)) {
                active=false;
                preferred=false;
                role=ROLE_NONE;
                preferences.edit().putBoolean(PREF_ACTIVE,false).putBoolean(PREF_PREFERRED,false)
                        .putInt(PREF_ROLE,ROLE_NONE).remove(PREF_ADDRESS).apply();
                status="BLE mirror pairing was interrupted";
            }
            bluetoothManager=(BluetoothManager)Applic.app.getSystemService(Context.BLUETOOTH_SERVICE);
            adapter=bluetoothManager==null?null:bluetoothManager.getAdapter();
            initialized=true;
            registerReceiverLocked();
        }
        // A Google-free Wear connection has no Data Layer node name from which
        // to infer the vendor. The current bidirectional net-info protocol does
        // not depend on this distinction; MessageSender can refine it later.
        if(Applic.isWearable)
            Natives.isGalaxyWatch(false);
        startSavedConnection();
    }

    /** Prefer the Wear Data Layer node id so BLE and MessageClient use one host. */
    public static void suggestLinkId(String suggested) {
        if(suggested==null||suggested.trim().isEmpty())
            return;
        final String usable=suggested.length()>16?suggested.substring(0,16):suggested;
        synchronized(lock) {
            if(linkId==null&&approvedAddress==null) {
                linkId=usable;
                preferences.edit().putString(PREF_LINK_ID,linkId).apply();
            }
        }
    }

    private static String ensureLinkIdLocked() {
        if(linkId==null) {
            final byte[] id=new byte[8];
            random.nextBytes(id);
            final StringBuilder builder=new StringBuilder(16);
            for(byte one:id)
                builder.append(String.format(Locale.US,"%02x",one&0xff));
            linkId=builder.toString();
            preferences.edit().putString(PREF_LINK_ID,linkId).apply();
        }
        return linkId;
    }

    public static boolean isReady() {
        synchronized(lock) { return ready; }
    }

    public static String getLinkId() {
        synchronized(lock) { return ready?linkId:null; }
    }

    public static String getPreferredPeer() {
        synchronized(lock) { return preferred?linkId:null; }
    }

    public static boolean shouldUse(String target) {
        synchronized(lock) {
            return preferred&&ready&&linkId!=null&&(target==null||linkId.equals(target));
        }
    }

    public static boolean isPeer(String target) {
        synchronized(lock) { return ready&&linkId!=null&&linkId.equals(target); }
    }

    public static boolean peerIsWearable() {
        synchronized(lock) { return ready&&remoteIsWearable; }
    }

    public static boolean localCreatesHost() {
        synchronized(lock) { return role==ROLE_CLIENT; }
    }

    public static boolean isPreferred() {
        synchronized(lock) { return preferred; }
    }

    public static void restorePreferredTransport(String peer) {
        final long epoch;
        synchronized(lock) {
            if(!ready||!preferred||role!=ROLE_CLIENT||restoringPreferred||linkId==null||!linkId.equals(peer))
                return;
            restoringPreferred=true;
            epoch=connectionEpoch;
        }
        new Thread(() -> {
            final boolean selected=MessageSender.sendNameMessageOn(peer,true);
            final boolean current;
            synchronized(lock) {
                current=selected&&ready&&preferred&&connectionEpoch==epoch&&linkId!=null&&linkId.equals(peer);
                if(connectionEpoch==epoch)
                    restoringPreferred=false;
                if(!selected&&connectionEpoch==epoch)
                    status="Could not restore the BLE mirror carrier";
            }
            if(current) {
                Natives.setBlueMessage(peer,true);
                Natives.resetMessageConnection(peer);
            }
        },"BLE mirror restore").start();
    }

    public static void setPreferred(boolean value) {
        synchronized(lock) {
            preferred=value;
            if(preferences!=null)
                preferences.edit().putBoolean(PREF_PREFERRED,value).apply();
        }
    }

    public static void permissionsChanged() {
        synchronized(lock) {
            if(!initialized)
                return;
        }
        startSavedConnection();
    }

    public static boolean send(String target,String path,byte[] data) {
        final Outbound outbound;
        synchronized(lock) {
            if(!ready||linkId==null||(target!=null&&!linkId.equals(target)))
                return false;
            outbound=enqueueLocked(path,data);
            if(outbound==null)
                return false;
        }
        try {
            if(!outbound.finished.await(SEND_TIMEOUT_SECONDS,TimeUnit.SECONDS)) {
                Log.e(LOG_ID,"Timeout sending "+path+" over BLE GATT");
                disconnectForFailure("BLE send timeout");
                return false;
            }
            return outbound.success;
        }
        catch(InterruptedException interrupted) {
            Thread.currentThread().interrupt();
            disconnectForFailure("BLE send interrupted");
            return false;
        }
    }

    public static boolean sendAsync(String path,byte[] data) {
        synchronized(lock) {
            return ready&&enqueueLocked(path,data)!=null;
        }
    }

    private static Outbound enqueueLocked(String path,byte[] data) {
        if(path==null||data==null)
            return null;
        final long requestedBytes=(long)data.length+path.length()+1L;
        if(requestedBytes>MAX_QUEUED_BYTES||queuedBytes+requestedBytes>MAX_QUEUED_BYTES) {
            status="BLE mirror send queue is full";
            return null;
        }
        final int newBytes=(int)requestedBytes;
        final List<byte[]> fragments;
        try {
            fragments=BleMirrorCodec.fragments(path,data,mtu,session,nextMessage.incrementAndGet());
        }
        catch(Throwable error) {
            Log.stack(LOG_ID,"fragment",error);
            return null;
        }
        final Outbound outbound=new Outbound(fragments,newBytes);
        outgoing.add(outbound);
        queuedBytes+=newBytes;
        sendNextLocked();
        return outbound;
    }

    private static void sendNextLocked() {
        if(!transportWritableLocked()||sending!=null)
            return;
        sending=outgoing.poll();
        if(sending==null)
            return;
        sendFragmentLocked();
    }

    private static boolean transportWritableLocked() {
        return (role==ROLE_SERVER&&gattServer!=null&&serverPeer!=null&&serverSubscribed)||
                (role==ROLE_CLIENT&&clientGatt!=null&&clientRx!=null&&clientSubscribed);
    }

    @SuppressWarnings("deprecation")
    @SuppressLint("MissingPermission")
    private static void sendFragmentLocked() {
        if(sending==null||sending.index>=sending.fragments.size()) {
            finishSendingLocked(true);
            return;
        }
        final byte[] value=sending.fragments.get(sending.index);
        boolean accepted=false;
        try {
            if(role==ROLE_SERVER&&gattServer!=null&&serverPeer!=null&&serverTx!=null&&serverSubscribed) {
                if(Build.VERSION.SDK_INT>=33)
                    accepted=gattServer.notifyCharacteristicChanged(serverPeer,serverTx,true,value)==BluetoothGatt.GATT_SUCCESS;
                else {
                    serverTx.setValue(value);
                    accepted=gattServer.notifyCharacteristicChanged(serverPeer,serverTx,true);
                }
            }
            else if(role==ROLE_CLIENT&&clientGatt!=null&&clientRx!=null&&clientSubscribed) {
                if(Build.VERSION.SDK_INT>=33)
                    accepted=clientGatt.writeCharacteristic(clientRx,value,BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT)==BluetoothGatt.GATT_SUCCESS;
                else {
                    clientRx.setWriteType(BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT);
                    clientRx.setValue(value);
                    accepted=clientGatt.writeCharacteristic(clientRx);
                }
            }
        }
        catch(Throwable error) {
            Log.stack(LOG_ID,"sendFragment",error);
        }
        if(!accepted) {
            failQueueLocked();
            handler.post(() -> disconnectForFailure("GATT rejected a mirror fragment"));
        }
        else {
            final Outbound pending=sending;
            final int pendingIndex=sending.index;
            final long epoch=connectionEpoch;
            handler.postDelayed(() -> {
                boolean timedOut=false;
                synchronized(lock) {
                    if(connectionEpoch==epoch&&sending==pending&&sending.index==pendingIndex) {
                        failQueueLocked();
                        timedOut=true;
                    }
                }
                if(timedOut)
                    disconnectForFailure("BLE GATT fragment timed out");
            },FRAGMENT_TIMEOUT_MILLIS);
        }
    }

    private static void fragmentFinished(int statusCode) {
        synchronized(lock) {
            if(sending==null)
                return;
            if(statusCode!=BluetoothGatt.GATT_SUCCESS) {
                failQueueLocked();
                handler.post(() -> disconnectForFailure("GATT fragment failed: "+statusCode));
                return;
            }
            ++sending.index;
            if(sending.index==sending.fragments.size())
                finishSendingLocked(true);
            else
                sendFragmentLocked();
        }
    }

    private static void finishSendingLocked(boolean success) {
        if(sending==null)
            return;
        final Outbound completed=sending;
        sending=null;
        queuedBytes=Math.max(0,queuedBytes-completed.bytes);
        completed.success=success;
        completed.finished.countDown();
        sendNextLocked();
    }

    private static void failQueueLocked() {
        if(sending!=null) {
            final Outbound failed=sending;
            sending=null;
            failed.success=false;
            failed.finished.countDown();
        }
        Outbound failed;
        while((failed=outgoing.poll())!=null) {
            failed.success=false;
            failed.finished.countDown();
        }
        queuedBytes=0;
    }

    private static void receivedFragment(byte[] value) {
        final BleMirrorCodec.Message complete;
        final long epoch;
        synchronized(lock) {
            complete=decoder.accept(value);
            epoch=connectionEpoch;
        }
        if(complete!=null) {
            incoming.execute(() -> {
                synchronized(lock) {
                    if(epoch!=connectionEpoch)
                        return;
                }
                try {
                    handleMessage(complete.path(),complete.data());
                }
                catch(Throwable error) {
                    Log.stack(LOG_ID,"receive "+complete.path(),error);
                }
            });
        }
    }

    private static void handleMessage(String path,byte[] data) {
        if(HELLO_PATH.equals(path)) {
            handleHello(data);
            return;
        }
        if(READY_PATH.equals(path)) {
            handleReady(data);
            return;
        }
        final String peer;
        final boolean peerWear;
        synchronized(lock) {
            if(!ready)
                return;
            peer=linkId;
            peerWear=remoteIsWearable;
        }
        MessageReceiver.receiveBle(peer,path,data,peerWear);
    }

    private static byte[] identityMessage(String id,boolean wearable) {
        final byte[] name=id.getBytes(StandardCharsets.UTF_8);
        final byte[] result=new byte[3+name.length];
        result[0]=PROTOCOL_VERSION;
        result[1]=(byte)(wearable?1:0);
        result[2]=(byte)name.length;
        System.arraycopy(name,0,result,3,name.length);
        return result;
    }

    private static Identity parseIdentity(byte[] data) {
        if(data.length<4||(data[0]&0xff)!=PROTOCOL_VERSION)
            return null;
        final int length=data[2]&0xff;
        if(length<1||length>16||3+length!=data.length)
            return null;
        final String id=new String(data,3,length,StandardCharsets.UTF_8);
        if(id.trim().isEmpty())
            return null;
        return new Identity(id,data[1]!=0);
    }

    @SuppressLint("MissingPermission")
    private static void handleHello(byte[] data) {
        final Identity identity=parseIdentity(data);
        if(identity==null) {
            disconnectForFailure("Invalid BLE mirror hello");
            return;
        }
        boolean accepted=false;
        synchronized(lock) {
            if(role!=ROLE_SERVER||serverPeer==null||serverPeer.getBondState()!=BluetoothDevice.BOND_BONDED)
                return;
            final String address=serverPeer.getAddress();
            final boolean pairing=pairingActiveLocked();
            final boolean knownAddress=approvedAddress!=null&&address.equalsIgnoreCase(approvedAddress);
            if((pairing||knownAddress)&&(pairing||linkId==null||linkId.equals(identity.id))) {
                approvedAddress=address;
                linkId=identity.id;
                remoteIsWearable=identity.wearable;
                active=true;
                ready=true;
                pairingUntil=0;
                status="BLE mirror connected to "+serverPeer.getName();
                savePeerLocked(ROLE_SERVER);
                enqueueLocked(READY_PATH,identityMessage(linkId,Applic.isWearable));
                accepted=true;
            }
        }
        if(accepted)
            onReady();
        else
            disconnectForFailure("Unapproved BLE mirror peer");
    }

    private static void handleReady(byte[] data) {
        final Identity identity=parseIdentity(data);
        if(identity==null) {
            disconnectForFailure("Invalid BLE mirror ready response");
            return;
        }
        boolean accepted=false;
        synchronized(lock) {
            if(role!=ROLE_CLIENT||clientGatt==null||linkId==null||!linkId.equals(identity.id))
                return;
            approvedAddress=clientGatt.getDevice().getAddress();
            bondCandidate=null;
            remoteIsWearable=identity.wearable;
            active=true;
            ready=true;
            pairingUntil=0;
            status="BLE mirror connected to "+clientGatt.getDevice().getName();
            savePeerLocked(ROLE_CLIENT);
            accepted=true;
        }
        if(accepted)
            onReady();
    }

    private static void onReady() {
        final String id;
        synchronized(lock) { id=linkId; }
        if(id==null)
            return;
        MessageSender.sendnetinfo(id);
        if(Applic.isWearable)
            MessageSender.sendAskForStart(id);
    }

    private static void savePeerLocked(int newRole) {
        role=newRole;
        preferences.edit()
                .putBoolean(PREF_ACTIVE,true)
                .putInt(PREF_ROLE,role)
                .putString(PREF_ADDRESS,approvedAddress)
                .putString(PREF_LINK_ID,linkId)
                .apply();
    }

    private static boolean pairingActiveLocked() {
        return pairingUntil>SystemClock.elapsedRealtime();
    }

    private static void startSavedConnection() {
        final int savedRole;
        synchronized(lock) {
            if(!active||role==ROLE_NONE||approvedAddress==null)
                return;
            savedRole=role;
        }
        if(!canOperate()) {
            status="BLE mirror waits for Bluetooth permission";
            return;
        }
        if(savedRole==ROLE_SERVER)
            startServer();
        else if(savedRole==ROLE_CLIENT)
            startScan(false);
    }

    private static boolean canOperate() {
        if(adapter==null) {
            Log.i(LOG_ID,"canOperate: adapter==null");
            return false;
            }
        if(Build.VERSION.SDK_INT>=31) {
            if(ContextCompat.checkSelfPermission(Applic.app,Manifest.permission.BLUETOOTH_CONNECT)!=PackageManager.PERMISSION_GRANTED||
                    ContextCompat.checkSelfPermission(Applic.app,Manifest.permission.BLUETOOTH_SCAN)!=PackageManager.PERMISSION_GRANTED||
                    ContextCompat.checkSelfPermission(Applic.app,Manifest.permission.BLUETOOTH_ADVERTISE)!=PackageManager.PERMISSION_GRANTED)   {
               Log.i(LOG_ID,"canOperate: no permission");
                return false;
                }
        }
        else if(!Applic.mayscan())
            return false;
        try {
            return adapter.isEnabled();
        }
        catch(Throwable the) {
           Log.stack(LOG_ID,"conOperate",the);
            return false;
        }
    }

    @SuppressLint("MissingPermission")
    public static void offerPairing() {
        if(!canOperate()) {
            status="Bluetooth or Nearby devices permission is unavailable";
            return;
        }
        final String oldSelected;
        synchronized(lock) { oldSelected=preferred?linkId:null; }
        stopClient();
        stopServer();
        synchronized(lock) {
            connectionLostLocked("Preparing BLE mirror pairing");
            active=true;
            preferred=false;
            role=ROLE_SERVER;
            approvedAddress=null;
            linkId=null;
            remoteIsWearable=false;
            bondCandidate=null;
            pairingUntil=SystemClock.elapsedRealtime()+PAIRING_MILLIS;
            preferences.edit().putBoolean(PREF_ACTIVE,true).putBoolean(PREF_PREFERRED,false).putInt(PREF_ROLE,role)
                    .remove(PREF_ADDRESS).remove(PREF_LINK_ID).apply();
            status="BLE pairing is allowed for two minutes";
        }
        if(oldSelected!=null)
            Natives.setBlueMessage(oldSelected,false);
        startServer();
        schedulePairingEnd();
    }

    @SuppressLint("MissingPermission")
    public static void findPairingPeer() {
        if(!canOperate()) {
            status="Bluetooth or Nearby devices permission is unavailable";
            return;
        }
        final String oldSelected;
        synchronized(lock) { oldSelected=preferred?linkId:null; }
        stopClient();
        stopServer();
        synchronized(lock) {
            connectionLostLocked("Preparing BLE mirror pairing");
            active=true;
            preferred=false;
            role=ROLE_CLIENT;
            approvedAddress=null;
            remoteIsWearable=false;
            bondCandidate=null;
            pairingUntil=SystemClock.elapsedRealtime()+PAIRING_MILLIS;
            if(!Applic.isWearable)
                linkId=null;
            ensureLinkIdLocked();
            preferences.edit().putBoolean(PREF_ACTIVE,true).putBoolean(PREF_PREFERRED,false)
                    .putInt(PREF_ROLE,role).remove(PREF_ADDRESS).apply();
            status="Looking for a Juggluco BLE mirror for two minutes";
        }
        if(oldSelected!=null)
            Natives.setBlueMessage(oldSelected,false);
        startScan(true);
        schedulePairingEnd();
    }

    private static void schedulePairingEnd() {
        handler.postDelayed(() -> {
            int expiredRole=ROLE_NONE;
            synchronized(lock) {
                if(pairingUntil!=0&&pairingUntil<=SystemClock.elapsedRealtime()) {
                    pairingUntil=0;
                    if(!ready) {
                        expiredRole=role;
                        active=false;
                        preferences.edit().putBoolean(PREF_ACTIVE,false).apply();
                        status="BLE mirror pairing timed out";
                    }
                }
            }
            if(expiredRole==ROLE_CLIENT)
                stopClient();
            else if(expiredRole==ROLE_SERVER)
                stopServer();
        },PAIRING_MILLIS+1000L);
    }

    @SuppressLint("MissingPermission")
    private static void startServer() {
        synchronized(lock) {
            if(gattServer!=null) {
                startAdvertisingLocked();
                return;
            }
            try {
                final long generation=++serverGeneration;
                gattServer=bluetoothManager.openGattServer(Applic.app,newServerCallback(generation));
                if(gattServer==null) {
                    status="This device cannot open a BLE GATT server";
                    return;
                }
                final BluetoothGattService service=new BluetoothGattService(SERVICE_UUID,BluetoothGattService.SERVICE_TYPE_PRIMARY);
                final BluetoothGattCharacteristic rx=new BluetoothGattCharacteristic(RX_UUID,
                        BluetoothGattCharacteristic.PROPERTY_WRITE,
                        BluetoothGattCharacteristic.PERMISSION_WRITE_ENCRYPTED);
                serverTx=new BluetoothGattCharacteristic(TX_UUID,
                        BluetoothGattCharacteristic.PROPERTY_INDICATE,
                        BluetoothGattCharacteristic.PERMISSION_READ_ENCRYPTED);
                final BluetoothGattDescriptor cccd=new BluetoothGattDescriptor(CCCD_UUID,
                        BluetoothGattDescriptor.PERMISSION_READ_ENCRYPTED|
                                BluetoothGattDescriptor.PERMISSION_WRITE_ENCRYPTED);
                serverTx.addDescriptor(cccd);
                service.addCharacteristic(rx);
                service.addCharacteristic(serverTx);
                if(!gattServer.addService(service)) {
                    status="Could not add the BLE mirror service";
                    ++serverGeneration;
                    gattServer.close();
                    gattServer=null;
                }
            }
            catch(Throwable error) {
                Log.stack(LOG_ID,"startServer",error);
                status="Could not start BLE mirror server";
                ++serverGeneration;
                if(gattServer!=null)
                    gattServer.close();
                gattServer=null;
            }
        }
    }

    @SuppressLint("MissingPermission")
    private static void startAdvertisingLocked() {
        if(advertising||adapter==null||gattServer==null||!active||role!=ROLE_SERVER||
                (!pairingActiveLocked()&&approvedAddress==null))
            return;
        advertiser=adapter.getBluetoothLeAdvertiser();
        if(advertiser==null) {
            status="BLE advertising is not supported by this device";
            return;
        }
        final AdvertiseSettings settings=new AdvertiseSettings.Builder()
                .setAdvertiseMode(pairingActiveLocked()?AdvertiseSettings.ADVERTISE_MODE_LOW_LATENCY:AdvertiseSettings.ADVERTISE_MODE_LOW_POWER)
                .setConnectable(true).setTimeout(0).setTxPowerLevel(AdvertiseSettings.ADVERTISE_TX_POWER_MEDIUM).build();
        final AdvertiseData data=new AdvertiseData.Builder().addServiceUuid(new ParcelUuid(SERVICE_UUID)).setIncludeDeviceName(false).build();
        try {
            advertiser.startAdvertising(settings,data,advertiseCallback);
            advertising=true;
        }
        catch(Throwable error) {
            Log.stack(LOG_ID,"startAdvertising",error);
            status="Could not advertise the BLE mirror service";
        }
    }

    @SuppressLint("MissingPermission")
    private static void stopAdvertisingLocked() {
        if(advertising&&advertiser!=null) {
            try { advertiser.stopAdvertising(advertiseCallback); }
            catch(Throwable error) { Log.stack(LOG_ID,"stopAdvertising",error); }
        }
        advertising=false;
    }

    @SuppressLint("MissingPermission")
    private static void startScan(boolean pairing) {
        synchronized(lock) {
            if(scanRunning||adapter==null||!active||role!=ROLE_CLIENT||
                    (pairing&&!pairingActiveLocked())||(!pairing&&approvedAddress==null))
                return;
            scanner=adapter.getBluetoothLeScanner();
            if(scanner==null) {
                status="BLE scanning is not available";
                return;
            }
            final ScanFilter filter=new ScanFilter.Builder().setServiceUuid(new ParcelUuid(SERVICE_UUID)).build();
            final ScanSettings settings=new ScanSettings.Builder()
                    .setScanMode(pairing?ScanSettings.SCAN_MODE_LOW_LATENCY:ScanSettings.SCAN_MODE_LOW_POWER).build();
            try {
                scanner.startScan(Collections.singletonList(filter),settings,scanCallback);
                scanRunning=true;
                status=pairing?"Looking for a Juggluco BLE mirror":"Looking for the paired BLE mirror";
            }
            catch(Throwable error) {
                Log.stack(LOG_ID,"startScan",error);
                status="Could not scan for the BLE mirror";
            }
        }
    }

    @SuppressLint("MissingPermission")
    private static void stopScanLocked() {
        if(scanRunning&&scanner!=null) {
            try { scanner.stopScan(scanCallback); }
            catch(Throwable error) { Log.stack(LOG_ID,"stopScan",error); }
        }
        scanRunning=false;
    }

    @SuppressLint("MissingPermission")
    private static void foundDevice(BluetoothDevice device) {
        synchronized(lock) {
            if(!scanRunning)
                return;
            final boolean pairing=pairingActiveLocked();
            if(!pairing&&(approvedAddress==null||!approvedAddress.equalsIgnoreCase(device.getAddress())))
                return;
            stopScanLocked();
            if(!pairing&&device.getBondState()!=BluetoothDevice.BOND_BONDED) {
                status="Android's Bluetooth bond was removed; pair the BLE mirror again";
                return;
            }
            bondCandidate=device;
            status=(pairing?"Pairing with ":"Connecting to ")+device.getName();
            if(device.getBondState()==BluetoothDevice.BOND_BONDED) {
                connectClientLocked(device);
            }
            else if(!device.createBond()) {
                // Some stacks return false when bonding has already started.
                if(device.getBondState()==BluetoothDevice.BOND_BONDING)
                    status="Confirm the Bluetooth pairing on both devices";
                else {
                    status="Could not start Bluetooth bonding";
                    pairingUntil=0;
                    active=false;
                    preferences.edit().putBoolean(PREF_ACTIVE,false).apply();
                }
            }
            else
                status="Confirm the Bluetooth pairing on both devices";
        }
    }

    @SuppressLint("MissingPermission")
    private static void connectClientLocked(BluetoothDevice device) {
        closeClientLocked();
        servicesRequested=false;
        clientSubscribed=false;
        ready=false;
        restoringPreferred=false;
        decoder.reset();
        ++connectionEpoch;
        session=1+random.nextInt(0xfffe);
        try {
            clientGatt=Build.VERSION.SDK_INT>=23?
                    device.connectGatt(Applic.app,false,clientCallback,BluetoothDevice.TRANSPORT_LE):
                    device.connectGatt(Applic.app,false,clientCallback);
            if(clientGatt==null) {
                status="Could not create a BLE GATT connection";
                scheduleClientReconnect();
            }
            else {
                status="Connecting BLE mirror";
                scheduleHandshakeTimeout(connectionEpoch);
            }
        }
        catch(Throwable error) {
            Log.stack(LOG_ID,"connectGatt",error);
            status="Could not connect the BLE mirror";
            scheduleClientReconnect();
        }
    }

    @SuppressLint("MissingPermission")
    private static void discoverServices(BluetoothGatt gatt) {
        synchronized(lock) {
            if(gatt!=clientGatt||servicesRequested)
                return;
            servicesRequested=true;
            if(!gatt.discoverServices()) {
                status="Could not discover the BLE mirror service";
                disconnectForFailure("discoverServices rejected");
            }
        }
    }

    @SuppressWarnings("deprecation")
    @SuppressLint("MissingPermission")
    private static void subscribeClient(BluetoothGatt gatt) {
        synchronized(lock) {
            if(gatt!=clientGatt)
                return;
            final BluetoothGattService service=gatt.getService(SERVICE_UUID);
            if(service==null) {
                disconnectForFailure("BLE mirror service is missing");
                return;
            }
            clientRx=service.getCharacteristic(RX_UUID);
            final BluetoothGattCharacteristic tx=service.getCharacteristic(TX_UUID);
            if(clientRx==null||tx==null||!gatt.setCharacteristicNotification(tx,true)) {
                disconnectForFailure("BLE mirror characteristics are missing");
                return;
            }
            final BluetoothGattDescriptor cccd=tx.getDescriptor(CCCD_UUID);
            if(cccd==null) {
                disconnectForFailure("BLE mirror indication descriptor is missing");
                return;
            }
            final boolean accepted;
            if(Build.VERSION.SDK_INT>=33)
                accepted=gatt.writeDescriptor(cccd,BluetoothGattDescriptor.ENABLE_INDICATION_VALUE)==BluetoothGatt.GATT_SUCCESS;
            else {
                cccd.setValue(BluetoothGattDescriptor.ENABLE_INDICATION_VALUE);
                accepted=gatt.writeDescriptor(cccd);
            }
            if(!accepted)
                disconnectForFailure("Could not enable BLE mirror indications");
        }
    }

    private static void sendHello() {
        synchronized(lock) {
            if(role!=ROLE_CLIENT||!clientSubscribed)
                return;
            enqueueLocked(HELLO_PATH,identityMessage(ensureLinkIdLocked(),Applic.isWearable));
        }
    }

    private static void connectionLostLocked(String reason) {
        final String oldLink=linkId;
        ++connectionEpoch;
        ready=false;
        restoringPreferred=false;
        serverSubscribed=false;
        clientSubscribed=false;
        decoder.reset();
        failQueueLocked();
        status=reason;
        if(oldLink!=null)
            handler.post(() -> Natives.resetMessageConnection(oldLink));
    }

    @SuppressLint("MissingPermission")
    private static void disconnectForFailure(String reason) {
        synchronized(lock) {
            Log.e(LOG_ID,reason);
            connectionLostLocked(reason);
            if(role==ROLE_CLIENT&&clientGatt!=null) {
                try { clientGatt.disconnect(); }
                catch(Throwable ignored) {}
            }
            else if(role==ROLE_SERVER&&gattServer!=null&&serverPeer!=null) {
                try { gattServer.cancelConnection(serverPeer); }
                catch(Throwable ignored) {}
            }
        }
    }

    private static void scheduleClientReconnect() {
        handler.postDelayed(() -> {
            synchronized(lock) {
                if(!active||role!=ROLE_CLIENT||ready)
                    return;
            }
            startScan(false);
        },3000L);
    }

    private static void scheduleHandshakeTimeout(long epoch) {
        scheduleHandshakeTimeout(epoch,HANDSHAKE_TIMEOUT_MILLIS);
    }

    private static void scheduleHandshakeTimeout(long epoch,long delay) {
        handler.postDelayed(() -> {
            final boolean timedOut;
            synchronized(lock) {
                timedOut=active&&!ready&&connectionEpoch==epoch&&
                        (clientGatt!=null||serverPeer!=null);
            }
            if(timedOut)
                disconnectForFailure("BLE mirror handshake timed out");
        },delay);
    }

    @SuppressLint("MissingPermission")
    public static void pause() {
        synchronized(lock) {
            active=false;
            preferences.edit().putBoolean(PREF_ACTIVE,false).apply();
            connectionLostLocked("BLE mirror paused");
        }
        stopClient();
        stopServer();
    }

    @SuppressLint("MissingPermission")
    public static void resume() {
        synchronized(lock) {
            if(role==ROLE_NONE||approvedAddress==null)
                return;
            active=true;
            preferences.edit().putBoolean(PREF_ACTIVE,true).apply();
        }
        startSavedConnection();
    }

    @SuppressLint("MissingPermission")
    public static void forget() {
        final String oldSelected;
        synchronized(lock) { oldSelected=preferred?linkId:null; }
        pause();
        synchronized(lock) {
            approvedAddress=null;
            linkId=null;
            remoteIsWearable=false;
            role=ROLE_NONE;
            preferred=false;
            pairingUntil=0;
            status="Juggluco BLE mirror pairing forgotten";
            preferences.edit().clear().apply();
        }
        if(oldSelected!=null)
            Natives.setBlueMessage(oldSelected,false);
        // Deliberately do not call removeBond(): the peer can be the user's
        // system-paired watch and Juggluco must not unpair the watch itself.
    }

    @SuppressLint("MissingPermission")
    private static void stopClient() {
        synchronized(lock) {
            stopScanLocked();
            closeClientLocked();
        }
    }

    @SuppressLint("MissingPermission")
    private static void closeClientLocked() {
        if(clientGatt!=null) {
            try { clientGatt.disconnect(); }
            catch(Throwable ignored) {}
            try { clientGatt.close(); }
            catch(Throwable ignored) {}
        }
        clientGatt=null;
        clientRx=null;
        clientSubscribed=false;
        servicesRequested=false;
    }

    @SuppressLint("MissingPermission")
    private static void stopServer() {
        synchronized(lock) {
            ++serverGeneration;
            stopAdvertisingLocked();
            if(gattServer!=null) {
                if(serverPeer!=null) {
                    try { gattServer.cancelConnection(serverPeer); }
                    catch(Throwable ignored) {}
                }
                try { gattServer.close(); }
                catch(Throwable ignored) {}
            }
            gattServer=null;
            serverPeer=null;
            serverTx=null;
            serverSubscribed=false;
        }
    }

    private static void registerReceiverLocked() {
        if(receiverRegistered)
            return;
        final IntentFilter filter=new IntentFilter();
        filter.addAction(BluetoothDevice.ACTION_BOND_STATE_CHANGED);
        filter.addAction(BluetoothAdapter.ACTION_STATE_CHANGED);
        ContextCompat.registerReceiver(Applic.app,bluetoothReceiver,filter,ContextCompat.RECEIVER_EXPORTED);
        receiverRegistered=true;
    }

    @SuppressLint("MissingPermission")
    public static void showDialog(MainActivity activity) {
        init(activity);
        if(!canOperate()) {
            activity.finepermission();
            Applic.argToaster(activity,"Give Nearby devices permission and turn on Bluetooth, then open BLE mirror again",Toast.LENGTH_LONG);
            return;
        }
        final float density=activity.getResources().getDisplayMetrics().density;
        final LinearLayout content=new LinearLayout(activity);
        content.setOrientation(LinearLayout.VERTICAL);
        final int padding=(int)(16*density);
        content.setPadding(padding,padding,padding,padding);
        final TextView state=new TextView(activity);
        state.setText(statusText());
        content.addView(state);
        final TextView instructions=new TextView(activity);
        instructions.setText("For a phone and watch, allow pairing on the phone and tap Find on the watch. For two phones, choose either one to allow pairing. Confirm Android's Bluetooth pairing, wait for Connected, then tap Use BLE mirror now. Juggluco stores one BLE mirror peer.");
        content.addView(instructions);

        final Button offer=new Button(activity);
        offer.setText("Allow pairing for 2 minutes");
        content.addView(offer);
        final Button find=new Button(activity);
        find.setText(Applic.isWearable?"Find phone":"Find another phone");
        content.addView(find);
        final Button useBle=new Button(activity);
        useBle.setText("Use BLE mirror now");
        content.addView(useBle);
        final Button tryTcp=new Button(activity);
        tryTcp.setText("Try TCP/IP again");
        content.addView(tryTcp);
        final Button toggle=new Button(activity);
        synchronized(lock) { toggle.setText(active?"Pause BLE mirror":"Resume BLE mirror"); }
        content.addView(toggle);
        final Button forget=new Button(activity);
        forget.setText("Forget BLE mirror peer");
        content.addView(forget);
        final ScrollView scroll=new ScrollView(activity);
        scroll.addView(content);

        final AlertDialog dialog=new AlertDialog.Builder(activity)
                .setTitle("BLE GATT mirror")
                .setView(scroll)
                .setNegativeButton(R.string.closename,null)
                .create();
        offer.setOnClickListener(view -> { offerPairing(); state.setText(statusText()); });
        find.setOnClickListener(view -> { findPairingPeer(); state.setText(statusText()); });
        useBle.setOnClickListener(view -> switchAlternative(true,state));
        tryTcp.setOnClickListener(view -> switchAlternative(false,state));
        toggle.setOnClickListener(view -> {
            final boolean wasActive;
            synchronized(lock) { wasActive=active; }
            if(wasActive) pause(); else resume();
            synchronized(lock) { toggle.setText(active?"Pause BLE mirror":"Resume BLE mirror"); }
            state.setText(statusText());
        });
        forget.setOnClickListener(view -> { forget(); state.setText(statusText()); });
        final Runnable[] refresh=new Runnable[1];
        refresh[0]=() -> {
            if(!dialog.isShowing())
                return;
            state.setText(statusText());
            synchronized(lock) { toggle.setText(active?"Pause BLE mirror":"Resume BLE mirror"); }
            handler.postDelayed(refresh[0],1000L);
        };
        dialog.setOnDismissListener(ignored -> handler.removeCallbacks(refresh[0]));
        dialog.show();
        handler.post(refresh[0]);
    }

    private static void switchAlternative(boolean use,TextView state) {
        final String id;
        synchronized(lock) { id=ready?linkId:null; }
        if(id==null) {
            state.setText("Pair and connect the BLE mirror first");
            return;
        }
        final boolean previousPreference=isPreferred();
        if(use)
            setPreferred(true);
        new Thread(() -> {
            final boolean sent=MessageSender.sendNameMessageOn(id,use);
            if(sent) {
                if(!use)
                    setPreferred(false);
                Natives.setBlueMessage(id,use);
                if(use)
                    Natives.resetMessageConnection(id);
            }
            else if(use)
                setPreferred(previousPreference);
            handler.post(() -> state.setText(sent?(use?"BLE mirror transport selected":"TCP/IP retry requested"):"Could not switch mirror transport"));
        },"BLE mirror switch").start();
    }

    private static String statusText() {
        synchronized(lock) {
            final String peer=approvedAddress==null?"No approved peer":"Peer "+approvedAddress;
            final String alternative=Applic.isWearable||remoteIsWearable?
                    "TCP/IP with Wear-message fallback selected":"TCP/IP selected";
            return status+'\n'+peer+'\n'+(preferred?"BLE GATT carrier selected":alternative);
        }
    }

    private static final AdvertiseCallback advertiseCallback=new AdvertiseCallback() {
        @Override public void onStartSuccess(AdvertiseSettings settings) {
            synchronized(lock) { status=pairingActiveLocked()?"BLE mirror is ready to pair":"BLE mirror is waiting for its peer"; }
        }
        @Override public void onStartFailure(int errorCode) {
            synchronized(lock) { advertising=false; status="BLE advertising failed: "+errorCode; }
        }
    };

    private static final ScanCallback scanCallback=new ScanCallback() {
        @Override public void onScanResult(int callbackType,ScanResult result) { foundDevice(result.getDevice()); }
        @Override public void onBatchScanResults(List<ScanResult> results) {
            if(!results.isEmpty()) foundDevice(results.get(0).getDevice());
        }
        @Override public void onScanFailed(int errorCode) {
            synchronized(lock) { scanRunning=false; status="BLE scan failed: "+errorCode; }
        }
    };

    private static final BroadcastReceiver bluetoothReceiver=new BroadcastReceiver() {
        @Override @SuppressLint("MissingPermission") public void onReceive(Context context,Intent intent) {
            final String action=intent.getAction();
            if(BluetoothAdapter.ACTION_STATE_CHANGED.equals(action)) {
                final int state=intent.getIntExtra(BluetoothAdapter.EXTRA_STATE,BluetoothAdapter.ERROR);
                if(state==BluetoothAdapter.STATE_ON)
                    startSavedConnection();
                else if(state==BluetoothAdapter.STATE_OFF) {
                    synchronized(lock) { connectionLostLocked("Bluetooth is off"); }
                    stopClient();
                    stopServer();
                }
                return;
            }
            if(!BluetoothDevice.ACTION_BOND_STATE_CHANGED.equals(action))
                return;
            final BluetoothDevice device=Build.VERSION.SDK_INT>=33?
                    intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE,BluetoothDevice.class):
                    intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
            if(device==null)
                return;
            final int bond=intent.getIntExtra(BluetoothDevice.EXTRA_BOND_STATE,BluetoothDevice.BOND_NONE);
            synchronized(lock) {
                if(bondCandidate==null||!bondCandidate.getAddress().equals(device.getAddress()))
                    return;
                if(bond==BluetoothDevice.BOND_BONDED) {
                    if(!active||role!=ROLE_CLIENT||
                            (!pairingActiveLocked()&&(approvedAddress==null||!approvedAddress.equalsIgnoreCase(device.getAddress()))))
                        return;
                    status="Bluetooth bonded; connecting Juggluco";
                    connectClientLocked(device);
                }
                else if(bond==BluetoothDevice.BOND_NONE) {
                    status="Bluetooth pairing was not accepted";
                    bondCandidate=null;
                    pairingUntil=0;
                    active=false;
                    preferences.edit().putBoolean(PREF_ACTIVE,false).apply();
                }
            }
        }
    };

    private static boolean currentServerLocked(long generation) {
        return generation==serverGeneration&&gattServer!=null&&role==ROLE_SERVER;
    }

    private static BluetoothGattServerCallback newServerCallback(final long generation) {
        return new BluetoothGattServerCallback() {
        @Override @SuppressLint("MissingPermission") public void onServiceAdded(int statusCode,BluetoothGattService service) {
            synchronized(lock) {
                if(!currentServerLocked(generation))
                    return;
                if(statusCode==BluetoothGatt.GATT_SUCCESS)
                    startAdvertisingLocked();
                else
                    status="Adding BLE mirror service failed: "+statusCode;
            }
        }

        @Override @SuppressLint("MissingPermission") public void onConnectionStateChange(BluetoothDevice device,int statusCode,int newState) {
            synchronized(lock) {
                if(!currentServerLocked(generation))
                    return;
                if(newState==BluetoothProfile.STATE_CONNECTED&&statusCode==BluetoothGatt.GATT_SUCCESS) {
                    final boolean allowed=(approvedAddress!=null&&approvedAddress.equalsIgnoreCase(device.getAddress()))||pairingActiveLocked();
                    if(!allowed||(serverPeer!=null&&!device.equals(serverPeer))) {
                        gattServer.cancelConnection(device);
                        return;
                    }
                    serverPeer=device;
                    serverSubscribed=false;
                    ready=false;
                    restoringPreferred=false;
                    decoder.reset();
                    failQueueLocked();
                    ++connectionEpoch;
                    final long handshakeDelay=pairingActiveLocked()?
                            Math.max(HANDSHAKE_TIMEOUT_MILLIS,pairingUntil-SystemClock.elapsedRealtime()+1000L):
                            HANDSHAKE_TIMEOUT_MILLIS;
                    scheduleHandshakeTimeout(connectionEpoch,handshakeDelay);
                    session=1+random.nextInt(0xfffe);
                    mtu=23;
                    stopAdvertisingLocked();
                    status="BLE peer connected; waiting for encrypted handshake";
                }
                else if(newState==BluetoothProfile.STATE_DISCONNECTED&&device.equals(serverPeer)) {
                    serverPeer=null;
                    connectionLostLocked("BLE mirror disconnected");
                    if(active&&role==ROLE_SERVER)
                        startAdvertisingLocked();
                }
            }
        }

        @Override public void onMtuChanged(BluetoothDevice device,int newMtu) {
            synchronized(lock) {
                if(currentServerLocked(generation)&&device.equals(serverPeer))
                    mtu=Math.max(23,newMtu);
            }
        }

        @Override @SuppressLint("MissingPermission") public void onDescriptorWriteRequest(BluetoothDevice device,int requestId,
                BluetoothGattDescriptor descriptor,boolean preparedWrite,boolean responseNeeded,int offset,byte[] value) {
            int response=BluetoothGatt.GATT_REQUEST_NOT_SUPPORTED;
            synchronized(lock) {
                if(!currentServerLocked(generation))
                    return;
                if(device.equals(serverPeer)&&CCCD_UUID.equals(descriptor.getUuid())&&!preparedWrite&&offset==0&&
                        java.util.Arrays.equals(value,BluetoothGattDescriptor.ENABLE_INDICATION_VALUE)) {
                    serverSubscribed=true;
                    response=BluetoothGatt.GATT_SUCCESS;
                }
                if(responseNeeded)
                    gattServer.sendResponse(device,requestId,response,offset,null);
            }
        }

        @Override @SuppressLint("MissingPermission") public void onCharacteristicWriteRequest(BluetoothDevice device,int requestId,
                BluetoothGattCharacteristic characteristic,boolean preparedWrite,boolean responseNeeded,int offset,byte[] value) {
            synchronized(lock) {
                if(!currentServerLocked(generation))
                    return;
                final boolean accepted=device.equals(serverPeer)&&RX_UUID.equals(characteristic.getUuid())&&!preparedWrite&&offset==0&&value!=null;
                if(responseNeeded)
                    gattServer.sendResponse(device,requestId,accepted?BluetoothGatt.GATT_SUCCESS:BluetoothGatt.GATT_REQUEST_NOT_SUPPORTED,offset,null);
                if(accepted)
                    receivedFragment(value);
            }
        }

        @Override public void onNotificationSent(BluetoothDevice device,int statusCode) {
            synchronized(lock) {
                if(!currentServerLocked(generation)||!device.equals(serverPeer))
                    return;
                fragmentFinished(statusCode);
            }
        }
        };
    }

    private static final BluetoothGattCallback clientCallback=new BluetoothGattCallback() {
        @Override @SuppressLint("MissingPermission") public void onConnectionStateChange(BluetoothGatt gatt,int statusCode,int newState) {
            if(newState==BluetoothProfile.STATE_CONNECTED&&statusCode==BluetoothGatt.GATT_SUCCESS) {
                synchronized(lock) {
                    if(gatt!=clientGatt)
                        return;
                    status="BLE connected; discovering mirror service";
                    mtu=23;
                    try { gatt.requestConnectionPriority(BluetoothGatt.CONNECTION_PRIORITY_HIGH); }
                    catch(Throwable ignored) {}
                    if(!gatt.requestMtu(REQUESTED_MTU))
                        discoverServices(gatt);
                    else
                        handler.postDelayed(() -> discoverServices(gatt),3000L);
                }
            }
            else if(newState==BluetoothProfile.STATE_DISCONNECTED) {
                boolean reconnect=false;
                synchronized(lock) {
                    if(gatt==clientGatt) {
                        try { gatt.close(); } catch(Throwable ignored) {}
                        clientGatt=null;
                        clientRx=null;
                        connectionLostLocked("BLE mirror disconnected (status "+statusCode+")");
                        reconnect=true;
                    }
                }
                if(reconnect)
                    scheduleClientReconnect();
            }
        }

        @Override public void onMtuChanged(BluetoothGatt gatt,int newMtu,int statusCode) {
            synchronized(lock) {
                if(gatt==clientGatt&&statusCode==BluetoothGatt.GATT_SUCCESS)
                    mtu=Math.max(23,newMtu);
            }
            discoverServices(gatt);
        }

        @Override public void onServicesDiscovered(BluetoothGatt gatt,int statusCode) {
            synchronized(lock) { if(gatt!=clientGatt) return; }
            if(statusCode==BluetoothGatt.GATT_SUCCESS)
                subscribeClient(gatt);
            else
                disconnectForFailure("BLE service discovery failed: "+statusCode);
        }

        @Override public void onDescriptorWrite(BluetoothGatt gatt,BluetoothGattDescriptor descriptor,int statusCode) {
            synchronized(lock) { if(gatt!=clientGatt) return; }
            if(CCCD_UUID.equals(descriptor.getUuid())&&statusCode==BluetoothGatt.GATT_SUCCESS) {
                synchronized(lock) { clientSubscribed=true; }
                sendHello();
            }
            else
                disconnectForFailure("BLE indication setup failed: "+statusCode);
        }

        @Override public void onCharacteristicWrite(BluetoothGatt gatt,BluetoothGattCharacteristic characteristic,int statusCode) {
            synchronized(lock) { if(gatt!=clientGatt) return; }
            if(RX_UUID.equals(characteristic.getUuid()))
                fragmentFinished(statusCode);
        }

        @Override @SuppressWarnings("deprecation") public void onCharacteristicChanged(BluetoothGatt gatt,BluetoothGattCharacteristic characteristic) {
            synchronized(lock) { if(gatt!=clientGatt) return; }
            if(Build.VERSION.SDK_INT<33&&TX_UUID.equals(characteristic.getUuid()))
                receivedFragment(characteristic.getValue());
        }

        @Override public void onCharacteristicChanged(BluetoothGatt gatt,BluetoothGattCharacteristic characteristic,byte[] value) {
            synchronized(lock) { if(gatt!=clientGatt) return; }
            if(TX_UUID.equals(characteristic.getUuid()))
                receivedFragment(value);
        }
    };

    private static final class Outbound {
        final List<byte[]> fragments;
        final int bytes;
        final CountDownLatch finished=new CountDownLatch(1);
        int index;
        volatile boolean success;
        Outbound(List<byte[]> fragments,int bytes) {
            this.fragments=fragments;
            this.bytes=bytes;
        }
    }

    private static final class Identity {
        final String id;
        final boolean wearable;
        Identity(String id,boolean wearable) { this.id=id; this.wearable=wearable; }
    }
}
