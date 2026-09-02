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
import android.bluetooth.le.ScanRecord;
import android.bluetooth.le.ScanResult;
import android.bluetooth.le.ScanSettings;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.os.ParcelUuid;
import android.os.PowerManager;

import androidx.core.content.ContextCompat;

import java.nio.ByteOrder;
import java.security.GeneralSecurityException;
import java.security.MessageDigest;
import java.security.SecureRandom;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * BLE GATT carrier for the existing mirror byte protocol.
 *
 * AutoQR already gives both devices a random connection label and password.
 * Those values authenticate discovery and a mutual protocol-v3 handshake, and
 * derive a fresh AES-GCM session key. Android bonding is deliberately not used:
 * it added duplicate confirmation dialogs on some devices and was redundant
 * with the user's explicit QR consent. One process can keep several independent
 * Bluetooth mirror links active at the same time. Direct Bluetooth rows select
 * this carrier permanently. Automatic rows keep an authenticated BLE rendezvous/fallback available while
 * QR/cached TCP/IP remains universal. The authenticated BLE session can also
 * refresh the same IP candidates used by sender.cpp. Wear OS keeps MessageClient
 * as its first non-TCP fallback and can use direct BLE when messages fail.
 */
public final class BleMirror {
    private static final String LOG_ID="BleMirror";
    // v15 introduced the d20 token-addressed UUID namespace.  Use that same
    // token-addressed ATT database on Android 5: this is the layout that was
    // demonstrably working on XA1 -> J5 before the v24 regression.  d30 below is
    // retained only so a v7 client can migrate from the short-lived v5/v6 builds.
    private static final UUID SERVICE_UUID=UUID.fromString("e65f6d20-3f4b-4d2a-9f75-6a7567676c75");
    private static final UUID RX_UUID=UUID.fromString("e65f6d21-3f4b-4d2a-9f75-6a7567676c75");
    private static final UUID TX_UUID=UUID.fromString("e65f6d22-3f4b-4d2a-9f75-6a7567676c75");
    // Compatibility namespace emitted by v5/v6 Android-5 builds.  v7 no longer
    // advertises or registers this shared service, but clients still recognize it
    // so peers can be upgraded independently.
    private static final UUID ANDROID5_SERVICE_UUID=UUID.fromString("e65f6d30-3f4b-4d2a-9f75-6a7567676c75");
    private static final UUID ANDROID5_RX_UUID=UUID.fromString("e65f6d31-3f4b-4d2a-9f75-6a7567676c75");
    private static final UUID ANDROID5_TX_UUID=UUID.fromString("e65f6d32-3f4b-4d2a-9f75-6a7567676c75");
    // Match d20 (normal token service) and d30 (Android-5 shared service) with
    // one hardware scan filter; these UUIDs differ in exactly bit 36 of the MSB.
    private static final ParcelUuid CURRENT_DISCOVERY_MASK=new ParcelUuid(
            new UUID(~0x1000000000L,-1L));
    private static final UUID LEGACY_SERVICE_UUID=UUID.fromString("e65f6d10-3f4b-4d2a-9f75-6a7567676c75");
    private static final UUID LEGACY_RX_UUID=UUID.fromString("e65f6d11-3f4b-4d2a-9f75-6a7567676c75");
    private static final UUID LEGACY_TX_UUID=UUID.fromString("e65f6d12-3f4b-4d2a-9f75-6a7567676c75");
    private static final UUID CCCD_UUID=UUID.fromString("00002902-0000-1000-8000-00805f9b34fb");
    private static final ParcelUuid SERVICE_PARCEL_UUID=new ParcelUuid(LEGACY_SERVICE_UUID);
    // d10 is retained only to recognize and log stale on-air advertisements.
    // Current discovery accepts d20 token services and d30 Android-5 shared-service
    // advertisements; both carry the password-derived token in the low 32 bits.
    private static final ParcelUuid DISCOVERY_SERVICE_DATA_UUID=
            ParcelUuid.fromString("00006d10-0000-1000-8000-00805f9b34fb");
    private static final int AD_TYPE_SERVICE_UUID_128_PARTIAL=0x06;
    private static final int AD_TYPE_SERVICE_UUID_128_COMPLETE=0x07;
    private static final int AD_TYPE_SERVICE_DATA_128=0x21;
    private static final String HELLO_PATH="/blehello";
    private static final String READY_PATH="/bleready";
    private static final String PROBE_PATH="/bleprobe";
    private static final String ADDRESSES_PATH="/bleips";
    // Authenticated post-handshake capability and runtime GATT-role coordination.
    // These are intentionally ordinary encrypted BLE messages so mixed-version
    // peers simply ignore them instead of making the /blehello wire format incompatible.
    private static final String CAPABILITIES_PATH="/blecaps";
    private static final String RESOURCE_ROLE_PATH="/blerole";
    private static final byte BLE_CAPABILITIES_VERSION=1;
    private static final byte RESOURCE_ROLE_BECOME_CLIENT=1;

    public static final int TRANSPORT_AUTOMATIC=0;
    public static final int TRANSPORT_TCP=1;
    public static final int TRANSPORT_MESSAGES=2;
    public static final int TRANSPORT_BLUETOOTH=3;

    public static String transportName(int transport) {
        return switch(transport) {
            case TRANSPORT_AUTOMATIC -> "Automatic";
            case TRANSPORT_TCP -> "TCP/IP only";
            case TRANSPORT_MESSAGES -> "Messages only";
            case TRANSPORT_BLUETOOTH -> "Direct Bluetooth";
            default -> "Invalid";
        };
    }

    private static final int ROLE_SERVER=1;
    private static final int ROLE_CLIENT=2;
    private static final int MAX_REMOTE_HOSTS=8;
    private static final int MAX_PENDING_SERVER_CONNECTIONS=12;
    private static final int REQUESTED_MTU=517;
    private static final long INITIAL_MTU_TIMEOUT_MILLIS=2500L;
    // A missing onServicesDiscovered callback is a GATT-stack failure, not authentication.
    // Recover it well before the generic 30-second handshake timeout.
    private static final long SERVICE_DISCOVERY_TIMEOUT_MILLIS=10L*1000L;
    private static final int MAX_QUEUED_BYTES=1024*1024;
    private static final long HANDSHAKE_TIMEOUT_MILLIS=30L*1000L;
    private static final long SUBSCRIPTION_TIMEOUT_MILLIS=8L*1000L;
    // Android 5.0/5.1 can accept BluetoothGatt.writeDescriptor() locally but
    // then neither transmit the CCCD write nor deliver onDescriptorWrite().
    // A working CCCD write normally completes in a fraction of a second, so
    // do not spend eight seconds on this known-dead setup path before trying
    // the opposite physical GATT role.
    private static final long LEGACY_SUBSCRIPTION_TIMEOUT_MILLIS=2500L;
    private static final long CACHE_REFRESH_SETTLE_MILLIS=1000L;
    // Some older/vendor BLE controllers reject LE Create Connection while
    // application scanning/advertising is still active. Serialize the radio
    // roles briefly after a discovery match so the controller is idle before
    // Android starts the initiating procedure.
    private static final long CLIENT_CONNECT_SETTLE_MILLIS=200L;
    // Some controllers (observed on the Huawei M5) accept LE Create Connection
    // but never complete it when Android internally uses the Filter Accept List.
    // Do not spend the full handshake timeout on that dead end. After this
    // interval a preferred client link offers the GATT server role instead so
    // the peer can initiate. A role is persisted only after authentication.
    private static final long CLIENT_ESTABLISH_TIMEOUT_MILLIS=10L*1000L;
    // A failed outgoing connection must not immediately monopolize the shared
    // BLE radio again. This is especially important when the same phone also
    // has server-role mirrors: while connectGatt() is pending we intentionally
    // stop local advertising for old/vendor controllers. Give those server
    // links an advertising window before another client attempt is allowed.
    private static final long CLIENT_RETRY_BACKOFF_MILLIS=5L*1000L;
    // Android 8.1 and older have vendor Bluetooth stacks where repeatedly
    // registering/unregistering a second GATT client can silently stop
    // onCharacteristicChanged() delivery on an already healthy first client.
    // XA1 showed exactly that: its Realme client kept accepting writes while
    // repeated J5 status-133 attempts destroyed only the inbound indication path.
    private static final int FRAGILE_MULTI_CLIENT_GATT_MAX_SDK=27;
    private static final long CONCURRENT_CLIENT_RETRY_BACKOFF_MILLIS=30L*1000L;
    private static final long CONCURRENT_CLIENT_RETRY_MAX_MILLIS=60L*1000L;
    // XA1/Android 8 can transfer reliably over either phone mirror separately, but
    // simultaneous GATT client sessions repeatedly damage one another.  Keep each
    // proven client direction and time-share the physical central slot instead of
    // changing GATT roles.  Mirror synchronization resumes from acknowledged
    // offsets after each reconnect, so a short lease is sufficient to drain data.
    private static final long FRAGILE_CLIENT_SLOT_LEASE_MILLIS=18L*1000L;
    private static final long FRAGILE_CLIENT_SLOT_IDLE_MILLIS=1500L;
    // Give the authenticated /blerole request time to leave the old client GATT
    // before XA1 closes it and re-opens the same logical mirror as a peripheral.
    private static final long RESOURCE_ROLE_SWITCH_SETTLE_MILLIS=600L;
    private static final long RESOURCE_REVERSE_PROBE_DELAY_MILLIS=750L;
    // HCI error 0x3E surfaced by Android as GATT status 62. In these traces it
    // occurs when another logical Juggluco mirror already owns the usable
    // physical LE connection to the same phone. This is a controller/physical-
    // link establishment conflict, not evidence that the configured client/
    // server direction is wrong.
    private static final int GATT_STATUS_CONNECTION_ESTABLISHMENT_FAILED=0x3E;
    // A role reversal caused only by setup/connect failures is a runtime probe,
    // not a learned device property.  If that reversed role never completes an
    // authenticated handshake, return to the configured client role and keep
    // trying both directions.  This is essential for a newly QR-created mirror
    // with no TCP/IP endpoints: there is no TCP path that can rescue a BLE
    // bootstrap which became stranded in an unproven reversed-server role.
    private static final long TEMPORARY_REVERSE_ROLE_LEASE_MILLIS=75L*1000L;
    // A persisted side/direction is the first and preferred physical topology.
    // Give it a real chance before the normally-server side starts reverse
    // scans. The client's concrete connect/setup failures may still trigger a
    // temporary reversal earlier; the server then meets it after this interval.
    private static final long PREFERRED_ROLE_TRY_MILLIS=45L*1000L;
    // One failed Android GATT establishment is not evidence that the persisted
    // physical direction is bad.  In particular, vendor Bluetooth services can
    // restart underneath a connectGatt() attempt.  Require several complete
    // establishment failures before experimenting with the opposite role.
    private static final int CLIENT_ESTABLISHMENT_FAILURES_BEFORE_REVERSE=3;
    // Compact, password-authenticated IPv4 endpoint hint carried in the scan
    // response for Automatic mirrors.  A truncated HMAC made from the immutable
    // QR-created mirror password protects the endpoint bytes; TCP itself still
    // performs the normal full mirror authentication.  The hint is supplemental:
    // learning it must never postpone the BLE GATT connection, because the local
    // device may currently have no usable IP transport (for example Wi-Fi off).
    private static final int ENDPOINT_HINT_VERSION=1;
    private static final int ENDPOINT_HINT_BYTES=11;
    private static final int SOCKADDR_IN6_BYTES=28;
    // A preferred server periodically pauses advertising and scans briefly for
    // a peer that had to reverse the physical GATT roles. This timer is GLOBAL,
    // not tied to one advertisement generation: with multiple server links the
    // advertised identity rotates every 8 seconds, which must not perpetually
    // cancel the 12-second reverse-role probe.
    private static final long REVERSE_PROBE_DELAY_MILLIS=12L*1000L;
    // Do not let two peers that both have server-capable mirror rows settle into
    // the same fixed advertise/probe cadence. With a 12 s delay + 2 s probe on
    // both sides, their 14-second cycles can phase-lock forever: one side scans
    // exactly while the wanted identity on the other side is not advertised.
    // Randomizing only active non-legacy fallback discovery keeps old one-slot
    // devices and idle mirrors on their existing cadence while making a persistent
    // fixed-phase lock unstable on modern peers.
    private static final int REVERSE_PROBE_JITTER_MILLIS=3000;
    // Once TCP/Message failure has explicitly requested BLE, a server-role peer
    // should not wait twelve seconds between its chances to become GATT central.
    // A three-second gap plus the existing two-second probe keeps enough
    // advertising airtime for other peers while fitting several probes inside
    // an Android-5 peer's 18-second exclusive discovery lease.
    private static final long ACTIVE_REVERSE_PROBE_DELAY_MILLIS=3000L;
    private static final long REVERSE_PROBE_MILLIS=2000L;
    // Legacy/vendor advertisers may not release their hardware advertising
    // instance synchronously, so never stop/start back-to-back.  When Android
    // reports that all advertiser slots are occupied, retry quickly with jitter:
    // another Juggluco process periodically yields its advertiser specifically
    // to provide a short acquisition window.
    private static final long ADVERTISE_RESTART_SETTLE_MILLIS=350L;
    // A device may expose fewer legacy advertiser instances than it can sustain
    // established LE links (Huawei M5 does exactly this).  Separate Juggluco
    // processes cannot coordinate their advertiser rotation in memory, so every
    // unconnected advertisement periodically yields the radio for a short,
    // randomized interval.  A process that receives TOO_MANY_ADVERTISERS retries
    // on a much shorter randomized cadence and can therefore acquire that gap.
    private static final long ADVERTISE_CROSS_PROCESS_YIELD_MIN_MILLIS=1800L;
    private static final int ADVERTISE_CROSS_PROCESS_YIELD_JITTER_MILLIS=1700;
    private static final long ADVERTISE_RESOURCE_RETRY_MIN_MILLIS=650L;
    private static final int ADVERTISE_RESOURCE_RETRY_JITTER_MILLIS=850;
    private static final int ADVERTISE_SLICE_JITTER_MILLIS=2500;
    // ConnectivityManager.onLost()/onLinkPropertiesChanged() can run before the
    // kernel has removed or installed the interface address.  M5 still returned
    // wlan0/192.168.2.253 about 10 ms after onLost(), then returned no current IP
    // roughly 350 ms later.  Re-check after the network stack has settled so an
    // Automatic mirror cannot miss its TCP->BLE transition because of that race.
    private static final long NETWORK_CHANGE_SETTLE_MILLIS=750L;
    // Android 5.x vendor BLE stacks are frequently unable to sustain two useful
    // simultaneous GATT mirror sessions.  In particular the Samsung J5 can keep
    // one authenticated mirror alive while every second client/server setup
    // stalls until Bluetooth is restarted.  Treat API 21-22 as a one-session
    // radio and time-share that session between peers instead of poisoning the
    // controller with concurrent GATT work.
    private static final int LEGACY_SINGLE_GATT_MAX_SDK=22;
    private static final long LEGACY_STANDBY_RELEASE_MILLIS=1500L;
    private static final long LEGACY_SLOT_IDLE_MILLIS=2500L;
    // A reversed-server peer may need to remain advertised until the other side
    // reaches its 12-second reverse-role probe, so a peer that receives the old
    // Android slot gets an exclusive discovery lease long enough for that probe.
    private static final long LEGACY_SLOT_LEASE_MILLIS=18L*1000L;
    private static final long FRAGMENT_TIMEOUT_MILLIS=15L*1000L;
    private static final long PROBE_IDLE_MILLIS=20L*1000L;
    // Keep independent mirror liveness probes from phase-locking on the same
    // 20-second boundary.  The jitter is chosen again after each completed
    // probe cycle, so several long-lived links gradually spread their ATT
    // control traffic instead of converging on one controller event window.
    private static final long PROBE_IDLE_JITTER_MILLIS=5L*1000L;
    // A standby server-side logical session can disappear without a physical
    // Bluetooth disconnect when another Juggluco process on the same remote phone
    // keeps the shared ACL/GATT device alive.  Do not, however, use this timer to
    // kill a selected data carrier: the Realme -> M5 trace had a healthy Automatic
    // BLE carrier deliberately closed after 65 seconds without server-side traffic.
    // Client-side bidirectional /bleprobe already verifies a selected carrier.
    private static final long SERVER_LOGICAL_SESSION_TIMEOUT_MILLIS=65L*1000L;
    private static final long PROBE_BUSY_RETRY_MILLIS=1000L;
    // A liveness write can race with a just-arrived indication on vendor GATT
    // stacks.  If peer data arrived this recently, an immediate write rejection
    // is evidence of controller busy state, not of a dead mirror.
    private static final long PROBE_RECENT_RX_GRACE_MILLIS=5000L;
    // Current peers answer a one-byte /bleprobe request with a one-byte
    // authenticated /bleprobe acknowledgement. This verifies the reverse
    // indication path rather than merely proving that client writes still work.
    // Older peers harmlessly ignore the request payload, so strict reply checks
    // are enabled only after this connection has successfully seen an ACK once.
    private static final byte PROBE_REQUEST=1;
    private static final byte PROBE_ACK=2;
    private static final long PROBE_REPLY_TIMEOUT_MILLIS=12L*1000L;
    // A logical BLE message may legitimately take much longer than 65 seconds.
    // Realme -> XA1 sent a 47 kB catch-up block as 95 indications at MTU 517,
    // and as 5889 fragments at MTU 23.  The old total-duration timeout killed
    // those transfers while every fragment was still completing successfully.
    // Timeout only when no fragment has made progress for this long.
    private static final long SEND_PROGRESS_TIMEOUT_MILLIS=65L*1000L;
    private static final long ADVERTISE_SLICE_MILLIS=12L*1000L;
    // A learned reversed-server cannot initiate the physical GATT connection;
    // its peer gets only a short periodic reverse-role scan. Keep that identity
    // on air for longer than one complete 12 s + 2 s rendezvous cycle before
    // rotating to another mirror identity.
    private static final long REVERSED_SERVER_ADVERTISE_SLICE_MILLIS=18L*1000L;
    // Once a remote central has physically connected to our GATT server, avoid
    // advertiser/scanner churn until its CCCD + authenticated hello setup has
    // either completed or had ample time to fail. Several vendor stacks lose
    // the descriptor transaction if we rotate advertising or start a scan here.
    private static final long SERVER_HANDSHAKE_HOLD_MILLIS=12L*1000L;
    // Some vendor Bluetooth stacks report STATE_ON before a newly opened GATT
    // server is actually connectable. Advertising can already be visible while
    // every central connectGatt() fails with status 133 and the server receives
    // no onConnectionStateChange callback. Let the restarted stack settle before
    // recreating the token-addressed server/advertiser namespace.
    private static final long BLUETOOTH_ON_SETTLE_MILLIS=3000L;
    // Some vendor Bluetooth stacks report STATE_ON before a new GATT server is
    // really usable. Rebuild only after this settling interval; the persisted
    // side/pair-direction preference itself is retained across Bluetooth restart.
    private static final long DISCOVERY_BOOST_MILLIS=60L*1000L;
    private static final long DISCOVERY_LOG_MILLIS=10L*1000L;
    private static final long SCAN_WATCHDOG_MILLIS=20L*1000L;
    private static final long SCAN_STANDBY_PROBE_MILLIS=5L*1000L;
    private static final long RETRY_MILLIS=5000L;

    private static final Object lock=new Object();
    private static final Handler handler=new Handler(Looper.getMainLooper());
    private static PowerManager.WakeLock legacySlotWakeLock;
    private static final ExecutorService incoming=Executors.newSingleThreadExecutor(runnable -> {
        final Thread thread=new Thread(runnable,"BLE mirror receive");
        thread.setDaemon(true);
        return thread;
    });
    private static final SecureRandom random=new SecureRandom();
    private static final AtomicInteger nextMessage=new AtomicInteger();
    private static final LinkedHashMap<String,Link> linksByLabel=new LinkedHashMap<>();
    private static final HashMap<Integer,Link> linksByIndex=new HashMap<>();
    // A physical Bluetooth device can host more than one Juggluco installation.
    // Therefore a server-side logical session is identified by device address +
    // mirror link, not by the Bluetooth address alone.
    private static final HashMap<String,Connection> serverConnections=new HashMap<>();
    private static final HashMap<UUID,ServerEndpoint> serverEndpoints=new HashMap<>();
    // Android 5 can report the wrong parent service when several services reuse
    // identical characteristic UUIDs. New token-specific RX/TX UUIDs let the
    // callback identify the logical endpoint from the characteristic itself.
    private static final HashMap<UUID,ServerEndpoint> serverCharacteristics=new HashMap<>();
    private static final ArrayDeque<ServerEndpoint> serverServiceQueue=new ArrayDeque<>();
    private static final HashMap<String,Integer> serverDeviceMtu=new HashMap<>();
    // Physical server-side connections exist before the token-addressed CCCD
    // write tells us which logical mirror service they selected. Hold BLE
    // discovery scheduling stable during that short unauthenticated interval.
    private static final HashMap<String,Long> serverHandshakeHoldUntil=new HashMap<>();
    // Android reports physical LE links to every registered GATT server, even
    // when the central is actually using another app/process on this device.
    // Remember physical links that outlive our handshake hold without ever
    // selecting one of this process' token-addressed services. They must not
    // be cancelled blindly, but their later disconnect is an important radio
    // resource transition: on some Huawei stacks a previous advertising enable
    // has already failed in HCI while BluetoothLeAdvertiser still delivered
    // onStartSuccess(), leaving "advertising" stuck true in this process.
    private static final HashSet<String> expiredUnauthenticatedServerAddresses=new HashSet<>();
    private static BluetoothManager bluetoothManager;
    private static BluetoothAdapter adapter;
    private static boolean initialized;
    private static long advertiseRestartNotBeforeMillis;
    private static boolean advertiseRestartScheduled;
    private static boolean receiverRegistered;
    private static long configurationGeneration;
    private static long networkChangeGeneration;
    private static long nextConnectionGeneration;
    // Invalidates delayed STATE_ON recovery work when Bluetooth changes state
    // again before the vendor stack has finished settling.
    private static long bluetoothOnRecoveryGeneration;
    // Android may report STATE_ON before the restarted Bluetooth process/controller
    // is ready for a new scanner, advertiser, GATT server and GATT client. Keep a
    // hard process-wide gate so an Automatic fallback request cannot bypass the
    // existing BLUETOOTH_ON_SETTLE_MILLIS recovery delay.
    private static long bluetoothRestartSettleUntilMillis;
    private static long lastModernReverseProbeSuppressedLogMillis;

    private static BluetoothGattServer gattServer;
    private static Connection serverSendingConnection;
    // One app-originated ATT operation at a time across all BLE mirror links in
    // this process. Android already serializes server indications, but client
    // writes used to run concurrently with them. On a busy Realme this produced
    // bursts of ATT work on several physical links in the same few milliseconds.
    // The arbiter covers characteristic writes and server indications; control
    // traffic is selected ahead of bulk /data at every fragment boundary.
    private static Connection gattOperationConnection;
    private static long gattSchedulerCursorGeneration;
    private static boolean serverServiceReady;
    private static long serverGeneration;

    private static BluetoothLeAdvertiser advertiser;
    private static AdvertiseCallback advertiseCallback;
    private static Link advertisedLink;
    private static boolean advertising;
    private static long advertiseGeneration;
    // API21/22: while a locally required peer is being advertised, keep that
    // connectable identity continuously on air long enough for the remote side's
    // serialized reverse-role scan to see it. Otherwise both peers can phase-lock:
    // one scans exactly while the other has stopped advertising to scan a second
    // peer, producing endless immediate GATT status 133 failures.
    private static long legacyAdvertiseHoldUntilMillis;
    private static int advertiseCursor;
    private static long advertiseRotationScheduledGeneration=-1;

    private static BluetoothLeScanner scanner;
    private static boolean scanRunning;
    private static boolean scanLowLatency;
    private static long scanGeneration;
    private static long scanSoftwareProbeUntilMillis;
    private static long scanResultCount;
    private static long scanServiceDataResultCount;
    private static long lastUnmatchedScanLogMillis;
    private static long reverseProbeUntilMillis;
    private static long reverseProbeGeneration;
    private static boolean reverseProbeScheduled;

    private BleMirror() {}

    // Transport-selection messages and their replies must leave on the old
    // authenticated connection before a changed transport/role rebuilds GATT.
    // Nested holds cover simultaneous local and remote configuration updates.
    private static int wearControlConfigurationHolds;
    private static long wearControlConfigurationGeneration;

    public static boolean holdConfigurationForWearControl(String label) {
        synchronized(lock) {
            if(!canSendWearControlLocked(linksByLabel.get(label)))
                return false;
            ++wearControlConfigurationHolds;
            ++wearControlConfigurationGeneration;
            return true;
        }
    }

    public static void releaseConfigurationForWearControl() {
        synchronized(lock) {
            if(wearControlConfigurationHolds==0)
                return;
            if(--wearControlConfigurationHolds!=0)
                return;
        }
        configurationChanged();
    }

    private static boolean canReconfigureLocked() {
        if(!initialized)
            return false;
        if(wearControlConfigurationHolds!=0) {
            Log.i(LOG_ID,"Deferring BLE configuration until Wear control completes");
            return false;
        }
        return true;
    }

    public static void init(Context ignored) {
        // Bluetooth starts only after Applic has established a permission snapshot.
        // This is a no-op when initbluetooth()/finepermission() already refreshed it.
        Applic.ensureBluetoothPermissions(Applic.app);
        synchronized(lock) {
            if(initialized)
                return;
            bluetoothManager=(BluetoothManager)Applic.app.getSystemService(Context.BLUETOOTH_SERVICE);
            adapter=bluetoothManager==null?null:bluetoothManager.getAdapter();
            initialized=true;
            registerReceiverLocked();
        }
        if(Applic.isWearable)
            Natives.isGalaxyWatch(false);
        configurationChanged();
    }

    /** Re-read all mirror rows and reconcile Direct and Automatic fallback links. */
    public static void configurationChanged() {
        final long controlGeneration;
        synchronized(lock) {
            if(!canReconfigureLocked())
                return;
            controlGeneration=wearControlConfigurationGeneration;
        }
        try {
            Natives.applyMirrorTransports();
        }
        catch(Throwable error) {
            Log.stack(LOG_ID,"apply mirror transports",error);
        }

        final ArrayList<Config> configurations=new ArrayList<>();
        final HashMap<String,Integer> labelCounts=new HashMap<>();
        try {
            final int count=Natives.backuphostNr();
            for(int index=0;index<count;++index) {
                final boolean deactivated=Natives.getHostDeactivated(index);
                final int transport=Natives.getbackuptransport(index);
                final boolean side=Natives.getbackupside(index);
                final String label=Natives.getbackuplabel(index);
                final boolean wearPeer=Natives.isWearOS(index);
                final boolean nearbyMirror=Natives.getICElabel(index)==null;
                final boolean pairReverse=nearbyMirror&&!wearPeer&&Natives.getbackupblereverse(index);
                final boolean directionProven=!nearbyMirror||wearPeer||
                        !Natives.getbackupbleunproven(index);
                final boolean client=nearbyMirror&&!wearPeer?
                        ((!side)^pairReverse):Natives.getbackupbleclient(index);
                Log.i(LOG_ID,"configuration entry: index="+index+" label="+label+
                        " enabled="+(!deactivated)+" transport="+transportName(transport)+"("+transport+")"+
                        " side="+(side?"r2":"r1")+
                        " bleRole="+(client?"client":"server")+
                        " direction="+(nearbyMirror&&!wearPeer?(pairReverse?"reversed":"normal"):
                                (client==!side?"normal":"reversed"))+
                        " proven="+directionProven);
                // ICE is a relayed Internet connection, not a nearby Home-net
                // peer. Old ICE rows have the default transport/role bits and
                // cannot form a complementary GATT pair; advertising them only
                // steals discovery time from real phone-to-phone mirrors.
                final boolean automaticBleFallback=transport==TRANSPORT_AUTOMATIC&&nearbyMirror;
                if(!deactivated&&nearbyMirror&&(transport==TRANSPORT_BLUETOOTH||automaticBleFallback)) {
                    final String normalized=label==null?"":label.trim();
                    labelCounts.put(normalized,labelCounts.getOrDefault(normalized,0)+1);
                    configurations.add(new Config(index,label,Natives.getbackuppassword(index),side,client,
                            pairReverse,directionProven,wearPeer,transport));
                }
            }
        }
        catch(Throwable error) {
            Log.stack(LOG_ID,"read BLE mirror configuration",error);
        }
        for(Config config:configurations) {
            if(labelCounts.getOrDefault(config.normalizedLabel,0)>1)
                config.invalidate("Bluetooth mirror labels must be unique");
            config.finishSecurity();
            Log.i(LOG_ID,"Bluetooth discovery identity: index="+config.hostIndex+
                    " label="+config.label+" token="+tokenFingerprint(config.discoveryToken)+
                    " state="+(config.error==null?"usable":config.error));
        }

        synchronized(lock) {
            // A control may have started while native configuration was read.
            if(!canReconfigureLocked()||controlGeneration!=wearControlConfigurationGeneration)
                return;
            final HashMap<Integer,Link> oldLinks=new HashMap<>(linksByIndex);
            final HashSet<Link> reused=new HashSet<>();
            final LinkedHashMap<String,Link> newByLabel=new LinkedHashMap<>();
            final HashMap<Integer,Link> newByIndex=new HashMap<>();
            boolean changed=oldLinks.size()!=configurations.size();

            for(Config config:configurations) {
                final Link old=oldLinks.get(config.hostIndex);
                final Link link=old!=null&&old.matches(config)?old:new Link(config);
                if(link==old)
                    reused.add(old);
                else {
                    changed=true;
                    if(old!=null)
                        disconnectLinkLocked(old,"Bluetooth mirror configuration changed");
                }
                newByIndex.put(link.hostIndex,link);
                if(link.configError==null)
                    newByLabel.put(link.label,link);
            }
            for(Link old:oldLinks.values()) {
                if(!reused.contains(old)&&newByIndex.get(old.hostIndex)!=old)
                    disconnectLinkLocked(old,"Bluetooth mirror is no longer configured");
            }
            linksByIndex.clear();
            linksByIndex.putAll(newByIndex);
            linksByLabel.clear();
            linksByLabel.putAll(newByLabel);
            ++configurationGeneration;

            if(changed) {
                reverseProbeUntilMillis=0;
                reverseProbeScheduled=false;
                ++reverseProbeGeneration;
                stopScanLocked();
                stopAdvertisingLocked();
                // Never rebuild an already-open GATT server merely because another
                // mirror row was enabled/disabled. On Android 5 this changes ATT
                // handle numbers underneath remote caches and can turn a correct
                // token-specific service into a different logical mirror after
                // reconnect. Keep existing services/handles stable and append any
                // newly required token service in ensureServerLocked().
                if(gattServer!=null)
                    Log.i(LOG_ID,"Bluetooth mirror configuration changed; preserving live GATT server and stable service handles");
            }
            reconcileOperationsLocked();
            if(configurations.isEmpty())
                Log.i(LOG_ID,"configurationChanged: no enabled Direct Bluetooth or Automatic BLE fallback mirror");
            else
                Log.i(LOG_ID,"configurationChanged: "+configurations.size()+
                        " Bluetooth mirror link(s), "+linksByLabel.size()+" usable");
        }
        requestAutomaticPhoneFallbacksWithoutLocalNetwork("configuration changed");
    }

    /** Called after a user saves a mirror; optionally propagate the choice to Wear OS. */
    public static void configurationChanged(int index,boolean informWearPeer) {
        if(informWearPeer&&index>=0) {
            try {
                if(Applic.isWearable)
                    Natives.setMirrorWearOS(index);
                final String label=Natives.getbackuplabel(index);
                final int transport=Natives.getbackuptransport(index);
                final boolean remoteClient=!Natives.getbackupbleclient(index);
                if(label!=null&&(Applic.isWearable||Natives.isWearOS(index))) {
                    if(holdConfigurationForWearControl(label)) {
                        try {
                            MessageSender.sendMirrorTransport(label,transport,remoteClient,
                                    BleMirror::releaseConfigurationForWearControl);
                        }
                        catch(Throwable error) {
                            releaseConfigurationForWearControl();
                            throw error;
                        }
                        return;
                    }
                    MessageSender.sendMirrorTransport(label,transport,remoteClient);
                }
            }
            catch(Throwable error) {
                Log.stack(LOG_ID,"send mirror transport selection",error);
            }
        }
        configurationChanged();
    }

    public static String statusForConnection(int index) {
        synchronized(lock) {
            final Link link=linksByIndex.get(index);
            return link==null?null:link.status;
        }
    }

    public static String blockingStatusForConnection(int index) {
        synchronized(lock) {
            final Link link=linksByIndex.get(index);
            if(link==null)
                return null;
            if(link.configError!=null)
                return link.configError;
            return operationBlockerLocked(link);
        }
    }

    public static boolean isReady() {
        synchronized(lock) {
            for(Link link:linksByIndex.values())
                if(isCarrierReadyLocked(link))
                    return true;
            return false;
        }
    }

    public static boolean isReady(String target) {
        synchronized(lock) {
            return isCarrierReadyLocked(linksByLabel.get(target));
        }
    }

    /** Preferred target for the legacy target-less phone/watch entry points. */
    public static String getLinkId() {
        synchronized(lock) {
            final Link link=preferredLinkLocked(false);
            return link==null?null:link.label;
        }
    }

    public static boolean shouldUse(String target) {
        synchronized(lock) {
            if(target!=null)
                return isDataCarrierSelectedLocked(linksByLabel.get(target));
            return preferredLinkLocked(true)!=null;
        }
    }

    /** True when an authenticated BLE link can carry a /messages switch command. */
    public static boolean canSendControl(String target) {
        synchronized(lock) {
            return target!=null&&isReadyLocked(linksByLabel.get(target));
        }
    }

    private static boolean canSendWearControlLocked(Link link) {
        // Wear controls may change sensor ownership, settings, or transport.
        // Require an existing row and an authenticated opposite device type.
        return link!=null&&link.wearOS&&isReadyLocked(link)&&
                Applic.isWearable!=link.connection.remoteIsWearable;
    }

    public static boolean canSendWearControl(String target) {
        synchronized(lock) {
            return target!=null&&canSendWearControlLocked(linksByLabel.get(target));
        }
    }

    /** Ready Wear mirrors, including when Google has no reachable Wear nodes. */
    public static String[] wearControlLabels() {
        synchronized(lock) {
            final ArrayList<String> labels=new ArrayList<>();
            for(Link link:linksByLabel.values())
                if(canSendWearControlLocked(link))
                    labels.add(link.label);
            return labels.toArray(new String[0]);
        }
    }

    /** Ready mirrors for transport-independent wake requests. */
    public static String[] controlLabels() {
        synchronized(lock) {
            final ArrayList<String> labels=new ArrayList<>();
            for(Link link:linksByLabel.values())
                if(isReadyLocked(link))
                    labels.add(link.label);
            return labels.toArray(new String[0]);
        }
    }

    /**
     * Make a configured link actively discoverable after native TCP/IP reports
     * a failure or a Direct-Bluetooth send finds no authenticated GATT peer.
     *
     * Automatic links normally remain low-power standby links. A failed TCP
     * attempt raises only that mirror for one minute: a server advertises that
     * identity continuously and a client uses a low-latency scan. Repeated
     * native retries extend the window without repeatedly restarting Android's
     * scanner. This also gives us a safe recovery path when a vendor silently
     * stops a long-running BLE scan without calling onScanFailed().
     */
    public static boolean requestConnection(String target) {
        synchronized(lock) {
            final Link link=findConfiguredLinkLocked(target);
            if(link==null) {
                Log.e(LOG_ID,"Bluetooth discovery request has no configured mirror: "+target);
                return false;
            }
            if(link.configError!=null) {
                Log.e(LOG_ID,"Bluetooth discovery request is blocked: label="+link.label+
                        " reason="+link.configError);
                return false;
            }
            if(link.transport==TRANSPORT_AUTOMATIC) {
                final boolean newlyRequested=!link.automaticFallbackRequested;
                link.automaticFallbackRequested=true;
                // Start the preferred-direction trial when fallback is actually
                // requested, not when the process happened to create this Link.
                // Otherwise a long-running app can reverse immediately on the
                // first failure because its 45-second startup interval expired
                // hours ago. Both r1 and r2 must first keep their persisted,
                // complementary roles for the complete trial interval.
                if(newlyRequested) {
                    link.oppositeRoleNotBeforeMillis=monotonicMillis()+PREFERRED_ROLE_TRY_MILLIS;
                    schedulePreferredClientNoPeerRecoveryLocked(link);
                }
            }
            boostDiscoveryLocked(link,"carrier requested");
            // On fragile Android centrals the currently connected client may
            // have become ready before this second mirror requested BLE.  Do
            // not rely on that earlier handshake to have noticed future demand:
            // explicitly arm the slot handoff whenever a waiting mirror asks for
            // Bluetooth.  This fixes the XA1 trace where J5 held the slot for
            // 10-20 minutes although Realme fallback remained requested.
            if(fragileClientSlotLocked()) {
                final Connection existing=otherReadyClientConnectionLocked(link);
                if(existing!=null)
                    scheduleFragileClientSlotCheckLocked(existing,
                            FRAGILE_CLIENT_SLOT_IDLE_MILLIS);
            }
            // Do not report that discovery started when Android has already
            // told us that Bluetooth or a required permission is unavailable.
            // boostDiscoveryLocked() retained the request and exposed the
            // concrete blocker in mirror information for recovery after the
            // adapter/permission state changes.
            return operationBlockerLocked(link)==null;
        }
    }

    public static boolean isPeer(String target) {
        synchronized(lock) {
            return findConfiguredLinkLocked(target)!=null;
        }
    }

    public static boolean isConfiguredPeer(String target) {
        synchronized(lock) {
            return target!=null&&isDataCarrierSelectedLocked(linksByLabel.get(target));
        }
    }

    public static boolean isAutomaticPhonePeer(String target) {
        synchronized(lock) {
            final Link link=findConfiguredLinkLocked(target);
            return link!=null&&link.transport==TRANSPORT_AUTOMATIC;
        }
    }

    /** True for an Automatic mirror whose remote side is a Wear OS peer. */
    public static boolean isAutomaticWearPeer(String target) {
        synchronized(lock) {
            final Link link=findConfiguredLinkLocked(target);
            return link!=null&&link.transport==TRANSPORT_AUTOMATIC&&
                    (Applic.isWearable||link.wearOS);
        }
    }

    /** Single Automatic Wear BLE label; useful on a watch where the phone node ID is a different name. */
    public static String automaticWearLinkId() {
        synchronized(lock) {
            Link found=null;
            for(Link link:linksByIndex.values()) {
                if(link.configError!=null||link.transport!=TRANSPORT_AUTOMATIC||
                        !(Applic.isWearable||link.wearOS))
                    continue;
                if(found!=null&&found!=link)
                    return null;
                found=link;
            }
            return found==null?null:found.label;
        }
    }

    public static boolean isAutomaticCarrierActive(String target) {
        synchronized(lock) {
            final Link link=findConfiguredLinkLocked(target);
            return link!=null&&link.transport==TRANSPORT_AUTOMATIC&&link.carrierActive;
        }
    }

    /** True while TCP/Message failure still requires this Automatic peer to acquire BLE. */
    public static boolean isAutomaticFallbackRequested(String target) {
        synchronized(lock) {
            final Link link=findConfiguredLinkLocked(target);
            return link!=null&&link.transport==TRANSPORT_AUTOMATIC&&
                    link.automaticFallbackRequested;
        }
    }

    /** Cancel a pending BLE fallback because TCP/IP has actually become usable again. */
    public static boolean cancelAutomaticFallback(String target) {
        synchronized(lock) {
            final Link link=findConfiguredLinkLocked(target);
            if(link==null||link.transport!=TRANSPORT_AUTOMATIC)
                return false;
            link.automaticFallbackRequested=false;
            link.discoveryBoostUntilMillis=0L;
            link.legacyResumeCarrier=false;
            if(link.connection!=null&&!link.carrierActive)
                maybeScheduleLegacySlotCheckLocked(link.connection);
            reconcileOperationsLocked();
            return true;
        }
    }

    private static Link findConfiguredLinkLocked(String target) {
        if(target==null)
            return null;
        final Link usable=linksByLabel.get(target);
        if(usable!=null)
            return usable;
        for(Link link:linksByIndex.values())
            if(target.equals(link.label))
                return link;
        return null;
    }

    /** Local mirror row authenticated by this BLE connection, or -1. */
    public static int hostIndex(String target) {
        synchronized(lock) {
            final Link link=target==null?null:linksByLabel.get(target);
            return link==null?-1:link.hostIndex;
        }
    }

    /** Side-effect-free validation used to order /messagesack before data starts. */
    public static boolean canSetAutomaticCarrier(String target,boolean active) {
        synchronized(lock) {
            final Link link=target==null?null:linksByLabel.get(target);
            return link!=null&&link.transport==TRANSPORT_AUTOMATIC&&
                    (!active||isReadyLocked(link));
        }
    }

    /**
     * Select or release the BLE data carrier for an Automatic phone mirror.
     * The physical GATT connection remains up in standby when TCP/IP is used.
     */
    public static boolean setAutomaticCarrier(String target,boolean active) {
        final int local;
        final int remote;
        final boolean phonePeer;
        final boolean retryTcpAfterActivation;
        synchronized(lock) {
            final Link link=target==null?null:linksByLabel.get(target);
            if(link==null||link.transport!=TRANSPORT_AUTOMATIC) {
                Log.e(LOG_ID,"Automatic BLE carrier switch has no matching link: "+target);
                return false;
            }
            if(active&&!isReadyLocked(link)) {
                link.status="TCP/IP failed; waiting for Bluetooth fallback "+link.label;
                Log.e(LOG_ID,"Cannot activate Automatic BLE fallback before authentication: "+link.label);
                return false;
            }
            if(link.carrierActive==active) {
                link.automaticFallbackRequested=active;
                if(!active&&legacySingleGattLocked()&&link.connection!=null) {
                    link.discoveryBoostUntilMillis=0L;
                    maybeScheduleLegacySlotCheckLocked(link.connection);
                }
                return true;
            }
            link.carrierActive=active;
            link.automaticFallbackRequested=active;
            if(legacySingleGattLocked()||fragileClientSlotLocked()) {
                if(active&&link.connection!=null)
                    link.legacySlotLeaseUntilMillis=Math.max(link.legacySlotLeaseUntilMillis,
                            monotonicMillis()+(legacySingleGattLocked()?LEGACY_SLOT_LEASE_MILLIS:
                                    FRAGILE_CLIENT_SLOT_LEASE_MILLIS));
                else if(!active)
                    link.legacySlotLeaseUntilMillis=0L;
            }
            // Explicit carrier selection supersedes a pending legacy-slot
            // resume. A forced slot handoff sets the flag directly before
            // disconnecting and does not pass through this method.
            link.legacyResumeCarrier=false;
            if(!active)
                link.discoveryBoostUntilMillis=0L;
            retryTcpAfterActivation=active&&link.newTcpCandidates;
            if(retryTcpAfterActivation)
                link.newTcpCandidates=false;
            local=link.hostIndex;
            remote=link.connection==null?-1:link.connection.remoteHostIndex;
            phonePeer=link.connection!=null&&!Applic.isWearable&&!link.connection.remoteIsWearable;
            link.status=link.connection==null?
                    "Bluetooth fallback disconnected: "+link.label:connectedStatus(link,link.connection);
            if(legacySingleGattLocked()&&link.connection!=null)
                maybeScheduleLegacySlotCheckLocked(link.connection);
            else if(fragileClientSlotLocked()&&link.connection!=null)
                maybeScheduleFragileClientSlotCheckLocked(link.connection);
        }
        if(active)
            Natives.mirrorTransportReady(local,remote,phonePeer);
        else
            Natives.mirrorTransportDisconnected(local,remote);
        Log.i(LOG_ID,"Automatic mirror carrier changed: label="+target+" carrier="+
                (active?"Bluetooth":"TCP/IP")+" local="+local+" remote="+remote);
        if(retryTcpAfterActivation)
            MessageSender.preferTcpAfterAddressUpdate(target,local);
        return true;
    }

    public static boolean isPreferred() {
        synchronized(lock) {
            return preferredLinkLocked(false)!=null;
        }
    }

    public static void permissionsChanged() {
        // Explicit permission-result boundary: refresh once, then all BLE hot paths
        // consume the cached snapshot without calling checkSelfPermission().
        Applic.refreshBluetoothPermissions(Applic.app);
        Log.i(LOG_ID,"Bluetooth permission state changed");
        configurationChanged();
    }

    public static boolean send(String target,String path,byte[] data) {
        final Outbound outbound;
        final Connection connection;
        synchronized(lock) {
            final Link link=target==null?preferredLinkLocked(true):linksByLabel.get(target);
            connection=link==null?null:link.connection;
            if(connection==null||!connection.ready) {
                if(link!=null)
                    boostDiscoveryLocked(link,"send "+path+" found no authenticated carrier");
                return false;
            }
            outbound=enqueueLocked(connection,path,data);
            if(outbound==null)
                return false;
        }
        try {
            for(;;) {
                final long lastProgress=outbound.lastProgressMillis;
                final long remaining=SEND_PROGRESS_TIMEOUT_MILLIS-
                        Math.max(0L,monotonicMillis()-lastProgress);
                if(remaining>0L&&outbound.finished.await(remaining,TimeUnit.MILLISECONDS))
                    return outbound.success;
                if(outbound.finished.getCount()==0L)
                    return outbound.success;
                // A completion can race the timeout wake-up.  Only fail if the
                // same logical message has made no successful fragment progress
                // for the complete inactivity interval.
                final long silentFor=Math.max(0L,monotonicMillis()-outbound.lastProgressMillis);
                if(silentFor<SEND_PROGRESS_TIMEOUT_MILLIS)
                    continue;
                disconnectForFailure(connection,"Timeout sending "+path+
                        " over BLE GATT after "+silentFor+" ms without fragment progress");
                return false;
            }
        }
        catch(InterruptedException interrupted) {
            Thread.currentThread().interrupt();
            disconnectForFailure(connection,"BLE send interrupted");
            return false;
        }
    }

    public static boolean sendAsync(String target,String path,byte[] data) {
        synchronized(lock) {
            final Link link=target==null?preferredLinkLocked(true):linksByLabel.get(target);
            if(link==null)
                return false;
            if(link.connection==null||!link.connection.ready) {
                boostDiscoveryLocked(link,"send "+path+" found no authenticated carrier");
                return false;
            }
            return enqueueLocked(link.connection,path,data)!=null;
        }
    }

    /** Compatibility overload for the single phone/watch link. */
    public static boolean sendAsync(String path,byte[] data) {
        return sendAsync(null,path,data);
    }

    private static boolean isReadyLocked(Link link) {
        return link!=null&&link.connection!=null&&link.connection.alive&&link.connection.ready;
    }

    private static boolean legacySingleGattLocked() {
        return Build.VERSION.SDK_INT<=LEGACY_SINGLE_GATT_MAX_SDK;
    }

    /** Any physical BLE mirror session, including a connection still authenticating. */
    private static Connection legacyOccupiedConnectionLocked() {
        if(!legacySingleGattLocked())
            return null;
        for(Link link:linksByIndex.values())
            if(link.connection!=null&&link.connection.alive)
                return link.connection;
        for(Connection connection:serverConnections.values())
            if(connection!=null&&connection.alive)
                return connection;
        return null;
    }

    private static boolean legacyLinkSuppressedLocked(Link link) {
        return legacySingleGattLocked()&&link!=null&&
                link.legacySlotYieldUntilMillis>monotonicMillis();
    }

    /**
     * A legacy discovery lease exists only to give one peer an uninterrupted
     * chance to establish the single reliable Android-5 GATT session. Once that
     * peer has authenticated, keeping the previous peer suppressed until the
     * original 18-second deadline only adds avoidable mirror latency. End the
     * fulfilled lease immediately; the authenticated connection itself owns the
     * physical slot until its payload has drained and the short idle guard expires.
     */
    private static void completeLegacyDiscoveryLeaseLocked(Link connected,boolean activeCarrier) {
        if(!legacySingleGattLocked()||connected==null)
            return;
        final long now=monotonicMillis();
        final boolean fulfilled=connected.legacySlotLeaseUntilMillis>now;

        // The discovery lease used to be cleared as soon as authentication
        // succeeded.  That made an active Android-5 carrier eligible for a
        // forced handoff after only LEGACY_SLOT_IDLE_MILLIS (2.5 s).  Keep an
        // authenticated carrier on the single reliable GATT slot for at least
        // one full lease before another active fallback may evict it.
        connected.legacySlotLeaseUntilMillis=activeCarrier?
                Math.max(connected.legacySlotLeaseUntilMillis,
                        now+LEGACY_SLOT_LEASE_MILLIS):0L;

        if(!fulfilled)
            return;
        int released=0;
        for(Link link:linksByIndex.values()) {
            if(link==connected)
                continue;
            if(link.legacySlotYieldUntilMillis>now) {
                link.legacySlotYieldUntilMillis=0L;
                ++released;
            }
        }
        Log.i(LOG_ID,"Android 5 BLE discovery lease fulfilled by "+connected.label+
                "; activeCarrier="+activeCarrier+
                " leaseRemaining="+Math.max(0L,connected.legacySlotLeaseUntilMillis-now)+
                " ms; released "+released+" waiting peer suppression(s)");
    }

    private static Link legacyLeasedLinkLocked() {
        if(!legacySingleGattLocked())
            return null;
        final long now=monotonicMillis();
        Link best=null;
        for(Link link:linksByIndex.values()) {
            if(link.legacySlotLeaseUntilMillis<=now||link.connection!=null||link.configError!=null)
                continue;
            if(best==null||link.legacySlotLeaseUntilMillis>best.legacySlotLeaseUntilMillis)
                best=link;
        }
        return best;
    }

    private static boolean legacyNeedsSlotLocked(Link link) {
        if(link==null||link.configError!=null||link.connection!=null||
                operationBlockerLocked(link)!=null||legacyLinkSuppressedLocked(link))
            return false;
        // A preferred-client Automatic standby does not need the scarce Android-5
        // central/GATT slot until the local TCP/Message path asks for fallback.
        return link.transport==TRANSPORT_BLUETOOTH||link.automaticFallbackRequested||
                discoveryBoostedLocked(link);
    }

    /**
     * A server-capable Automatic peer is different: the REMOTE side can be the
     * side whose TCP/IP failed.  In that case this phone never receives a local
     * discovery boost, yet it still has to advertise occasionally or the remote
     * client can scan forever without seeing it (observed J5 server <-> S5).
     *
     * This is used only by the API-21/22 single-slot scheduler.  Newer Android
     * keeps the normal concurrent discovery behavior.
     */
    private static boolean legacyNeedsDiscoverySlotLocked(Link link) {
        if(link==null||link.configError!=null||link.connection!=null||
                operationBlockerLocked(link)!=null||legacyLinkSuppressedLocked(link))
            return false;
        if(link.transport==TRANSPORT_BLUETOOTH||link.automaticFallbackRequested||
                discoveryBoostedLocked(link))
            return true;
        return link.transport==TRANSPORT_AUTOMATIC&&offersGattServerLocked(link);
    }

    private static Link legacyWaitingPeerLocked(Link current) {
        if(!legacySingleGattLocked())
            return null;

        // An authenticated Android-5 BLE carrier must not be torn down merely
        // to advertise an unrelated idle Automatic server row.  That used to
        // disconnect working links almost exactly LEGACY_SLOT_IDLE_MILLIS
        // (2.5 s) after their last payload.  A live carrier yields only when
        // another mirror has an actual/urgent need for the single GATT slot.
        final boolean currentCarrier=current!=null&&
                (current.transport==TRANSPORT_BLUETOOTH||current.carrierActive||
                        current.legacyResumeCarrier);

        Link best=null;
        int bestPriority=Integer.MIN_VALUE;
        for(Link link:linksByIndex.values()) {
            if(link==current)
                continue;
            final boolean needs=currentCarrier?
                    legacyNeedsSlotLocked(link):legacyNeedsDiscoverySlotLocked(link);
            if(!needs)
                continue;
            int priority=advertisementPriorityLocked(link);
            if(link.role==ROLE_CLIENT&&!link.reverseGattRole)
                ++priority;
            if(best==null||priority>bestPriority) {
                best=link;
                bestPriority=priority;
            }
        }
        return best;
    }

    /**
     * If the next legacy peer is only temporarily suppressed by the previous
     * lease, remember when it becomes eligible.  Without this recheck a slot
     * check can fire a few seconds too early, see no waiting peer, and leave
     * the current Android-5 connection occupying the slot indefinitely.
     */
    private static long legacySuppressedPeerDelayLocked(Link current) {
        if(!legacySingleGattLocked())
            return -1L;
        final long now=monotonicMillis();
        long best=Long.MAX_VALUE;
        for(Link link:linksByIndex.values()) {
            if(link==current||link==null||link.configError!=null||link.connection!=null||
                    operationBlockerLocked(link)!=null)
                continue;
            final boolean wants=link.transport==TRANSPORT_BLUETOOTH||
                    link.automaticFallbackRequested||discoveryBoostedLocked(link)||
                    (link.transport==TRANSPORT_AUTOMATIC&&offersGattServerLocked(link));
            if(!wants||link.legacySlotYieldUntilMillis<=now)
                continue;
            best=Math.min(best,link.legacySlotYieldUntilMillis-now);
        }
        return best==Long.MAX_VALUE?-1L:Math.max(250L,best);
    }

    private static boolean isPayloadPath(String path) {
        return path!=null&&!HELLO_PATH.equals(path)&&!READY_PATH.equals(path)&&
                !PROBE_PATH.equals(path)&&!ADDRESSES_PATH.equals(path)&&
                !CAPABILITIES_PATH.equals(path)&&!RESOURCE_ROLE_PATH.equals(path);
    }

    /** Control-plane messages get the next free ATT slot before bulk /data. */
    private static boolean isGattControlPath(String path) {
        return path!=null&&!"/data".equals(path);
    }

    private static int gattOperationPriorityLocked(Connection connection) {
        if(connection==null||connection.sending==null)
            return Integer.MIN_VALUE;
        return isGattControlPath(connection.sending.path)?1:0;
    }

    private static boolean isDataCarrierSelectedLocked(Link link) {
        return link!=null&&(link.transport==TRANSPORT_BLUETOOTH||link.carrierActive);
    }

    private static boolean isCarrierReadyLocked(Link link) {
        return isDataCarrierSelectedLocked(link)&&isReadyLocked(link);
    }

    private static boolean discoveryBoostedLocked(Link link) {
        return link!=null&&link.discoveryBoostUntilMillis>monotonicMillis();
    }

    private static boolean urgentDiscoveryLocked(Link link) {
        return link!=null&&(link.transport==TRANSPORT_BLUETOOTH||
                link.automaticFallbackRequested||link.legacyResumeCarrier||
                discoveryBoostedLocked(link));
    }

    private static long legacyAdvertiseHoldRemainingLocked() {
        if(!legacySingleGattLocked()||!advertising||advertisedLink==null)
            return 0L;
        return Math.max(0L,legacyAdvertiseHoldUntilMillis-monotonicMillis());
    }

    /** The configured role is only the preferred physical GATT role. */
    private static boolean offersGattServerLocked(Link link) {
        return link!=null&&(link.role==ROLE_SERVER||
                (link.role==ROLE_CLIENT&&link.reverseGattRole));
    }

    private static boolean reverseProbeActiveLocked() {
        return reverseProbeUntilMillis>monotonicMillis();
    }

    /**
     * Server-role peers only scan during a short serialized reverse-role probe.
     * This lets a phone with a broken central/initiator path fall back to being
     * the peripheral without making old phones scan and advertise concurrently.
     *
     * Automatic links must participate too. A peer can have learned the
     * reversed-server role while the other side later returns to an unboosted
     * Automatic server. If Automatic servers never probe, both sides advertise
     * indefinitely and no GATT central exists (observed on XA1 <-> J5).
     */
    private static boolean reverseProbeCandidateLocked(Link link) {
        if(link==null||link.role!=ROLE_SERVER||link.connection!=null||isReadyLocked(link)||
                link.configError!=null||monotonicMillis()<link.oppositeRoleNotBeforeMillis)
            return false;
        // A proven server normally never experiments with the opposite role.  The
        // exception is an authenticated resource-balancing request from a fragile
        // peer (XA1): that peer explicitly moved this one logical connection to its
        // peripheral role so it can keep a different outgoing GATT client alive.
        if(link.directionProven) {
            if(link.resourceReverseClientRequested)
                return true;
            // A fragile peer may be unable to open this as its second outgoing
            // GATT client and therefore temporarily advertises despite its proven
            // client direction.  There is no authenticated control channel yet in
            // that ordering.  While an Automatic fallback is genuinely pending,
            // modern Android server-role peers periodically scan so they can find
            // and initiate that resource-balanced peripheral connection.
            return Build.VERSION.SDK_INT>FRAGILE_MULTI_CLIENT_GATT_MAX_SDK&&
                    link.transport==TRANSPORT_AUTOMATIC&&
                    link.automaticFallbackRequested;
        }
        return link.transport==TRANSPORT_BLUETOOTH||
                link.transport==TRANSPORT_AUTOMATIC||discoveryBoostedLocked(link);
    }

    private static boolean canReverseScanLocked() {
        if(Build.VERSION.SDK_INT>=31)
            return Applic.bluetoothPermissionGranted(Manifest.permission.BLUETOOTH_SCAN);
        return Applic.mayscan();
    }

    private static boolean canReverseAdvertiseLocked() {
        if(adapter==null)
            return false;
        if(Build.VERSION.SDK_INT>=31&&
                !Applic.bluetoothPermissionGranted(Manifest.permission.BLUETOOTH_ADVERTISE))
            return false;
        try { return adapter.getBluetoothLeAdvertiser()!=null; }
        catch(Throwable error) {
            Log.e(LOG_ID,"reverse BLE advertising unavailable: "+
                    error.getClass().getSimpleName());
            return false;
        }
    }

    private static void boostDiscoveryLocked(Link link,String reason) {
        if(link==null||link.configError!=null)
            return;
        final long now=monotonicMillis();
        final boolean newlyBoosted=link.discoveryBoostUntilMillis<=now;
        link.discoveryBoostUntilMillis=Math.max(link.discoveryBoostUntilMillis,
                now+DISCOVERY_BOOST_MILLIS);
        final String blocker=operationBlockerLocked(link);
        if(blocker!=null) {
            link.status=blocker;
            if(newlyBoosted||now-link.lastDiscoveryLogMillis>=DISCOVERY_LOG_MILLIS) {
                link.lastDiscoveryLogMillis=now;
                Log.e(LOG_ID,"Cannot discover Bluetooth mirror: label="+link.label+
                        " role="+(link.role==ROLE_CLIENT?"client":"server")+
                        " token="+tokenFingerprint(link.discoveryToken)+" reason="+blocker);
            }
            return;
        }
        if(!isReadyLocked(link))
            link.status=waitingStatus(link);
        if(newlyBoosted||now-link.lastDiscoveryLogMillis>=DISCOVERY_LOG_MILLIS) {
            link.lastDiscoveryLogMillis=now;
            Log.i(LOG_ID,"Bluetooth discovery boosted: label="+link.label+
                    " role="+(link.role==ROLE_CLIENT?"client":"server")+
                    " transport="+transportName(link.transport)+" token="+
                    tokenFingerprint(link.discoveryToken)+" reason="+reason+
                    " newlyBoosted="+newlyBoosted);
        }
        if(legacySingleGattLocked()) {
            final Connection occupied=legacyOccupiedConnectionLocked();
            if(occupied!=null&&occupied.link!=null&&occupied.link!=link)
                maybeScheduleLegacySlotCheckLocked(occupied);
        }
        if(link.role==ROLE_CLIENT&&!link.reverseGattRole) {
            // Upgrade a balanced standby scan immediately. A low-latency scan
            // is left running and its watchdog performs bounded restarts. A
            // newly boosted second link must also replace the existing filter
            // set; otherwise it could remain invisible until the watchdog.
            if(scanRunning&&(!scanLowLatency||newlyBoosted))
                stopScanLocked();
            refreshScanLocked();
        }
        else {
            // Normal server role, or a client that learned that this device's
            // central/initiator path is unreliable and reversed the GATT roles.
            ensureServerLocked();
            // Restart an Automatic standby advertisement even when it already
            // has the right identity so the active request upgrades Android's
            // advertising mode from balanced to low latency. If another
            // boosted link is already being advertised, leave it alone: the
            // paced rotation will select this link next. Restarting on every
            // concurrent native send made Android advertisers thrash and some
            // vendors then stopped putting either identity on air.
            final boolean advertisedBoosted=discoveryBoostedLocked(advertisedLink);
            if(serverServiceReady&&advertising&&advertisedLink!=link&&!advertisedBoosted) {
                stopAdvertisingLocked();
                startAdvertisingLocked();
            }
            else if(serverServiceReady&&!advertising)
                startAdvertisingLocked();
            else if(serverServiceReady&&advertising&&advertisedLink==link&&newlyBoosted)
                // Do not stop/start the same advertiser merely to change BALANCED to
                // LOW_LATENCY.  On the Huawei M5 the restart returned onStartSuccess
                // after S5 disconnected, yet S5 then saw zero Juggluco advertisements
                // even in unfiltered scans.  Keeping the already-on-air identity is
                // more important than upgrading the advertising interval.
                Log.i(LOG_ID,"keeping existing Bluetooth advertisement on air while boosting discovery: label="+
                        link.label);
        }
    }

    private static Link preferredLinkLocked(boolean requireReady) {
        Link only=null;
        Link wear=null;
        int count=0;
        int wearCount=0;
        for(Link link:linksByIndex.values()) {
            if(link.configError!=null||!isDataCarrierSelectedLocked(link)||
                    (requireReady&&!isReadyLocked(link)))
                continue;
            ++count;
            only=link;
            if(Applic.isWearable||link.wearOS) {
                ++wearCount;
                wear=link;
            }
        }
        if(wearCount==1)
            return wear;
        return count==1?only:null;
    }

    private static Outbound enqueueLocked(Connection connection,String path,byte[] data) {
        if(connection==null||!connection.alive||path==null||data==null)
            return null;
        final boolean handshake=HELLO_PATH.equals(path)||READY_PATH.equals(path);
        final byte[] transported;
        try {
            transported=handshake?data:BleMirrorSecurity.seal(connection.sessionKey,
                    !connection.serverSide,++connection.sendCounter,path,data);
        }
        catch(GeneralSecurityException error) {
            Log.stack(LOG_ID,"protect "+path,error);
            handler.post(() -> disconnectForFailure(connection,"BLE mirror encryption failed"));
            return null;
        }
        final long requestedBytes=(long)transported.length+path.length()+1L;
        if(requestedBytes>MAX_QUEUED_BYTES||connection.queuedBytes+requestedBytes>MAX_QUEUED_BYTES) {
            if(connection.link!=null)
                connection.link.status="BLE mirror send queue is full";
            return null;
        }
        final List<byte[]> fragments;
        try {
            fragments=BleMirrorCodec.fragments(path,transported,connection.mtu,connection.fragmentSession,
                    nextMessage.incrementAndGet());
        }
        catch(Throwable error) {
            Log.stack(LOG_ID,"fragment "+path,error);
            return null;
        }
        final Outbound outbound=new Outbound(path,fragments,(int)requestedBytes);
        connection.outgoing.add(outbound);
        connection.queuedBytes+=(int)requestedBytes;
        Log.i(LOG_ID,"queue "+path+": label="+connectionLabel(connection)+" plainBytes="+data.length+
                " protectedBytes="+transported.length+" fragments="+fragments.size()+
                " mtu="+connection.mtu+" maxGattValue="+BleMirrorCodec.MAX_ATTRIBUTE_VALUE);
        sendNextLocked(connection);
        return outbound;
    }

    private static void sendNextLocked(Connection connection) {
        if(connection==null||!transportWritableLocked(connection)||connection.sending!=null)
            return;
        connection.sending=connection.outgoing.poll();
        if(connection.sending!=null)
            kickGattOperationLocked();
    }

    private static boolean transportWritableLocked(Connection connection) {
        if(!connection.alive||!connection.subscribed)
            return false;
        return connection.serverSide?
                gattServer!=null&&connection.device!=null&&connection.serverTx!=null:
                connection.gatt!=null&&connection.rx!=null;
    }

    /**
     * Pick exactly one pending mirror ATT fragment for the process. Control
     * messages win over /data, and equal-priority connections are round-robin
     * by connection generation so a high-volume mirror cannot monopolize the
     * controller. This deliberately does not block a thread/Semaphore while
     * holding {@link #lock}; completion callbacks simply release the slot and
     * schedule the next fragment.
     */
    private static void kickGattOperationLocked() {
        if(gattOperationConnection!=null)
            return;
        final ArrayList<Connection> candidates=new ArrayList<>();
        for(Link link:linksByIndex.values()) {
            final Connection connection=link==null?null:link.connection;
            if(connection!=null&&!connection.serverSide&&connection.alive&&
                    connection.sending!=null&&transportWritableLocked(connection))
                candidates.add(connection);
        }
        for(Connection connection:serverConnections.values())
            if(connection!=null&&connection.serverSide&&connection.alive&&
                    connection.sending!=null&&transportWritableLocked(connection)&&
                    !candidates.contains(connection))
                candidates.add(connection);
        Connection best=null;
        int bestPriority=Integer.MIN_VALUE;
        long bestDistance=Long.MAX_VALUE;
        for(Connection connection:candidates) {
            final int priority=gattOperationPriorityLocked(connection);
            long distance=connection.generation-gattSchedulerCursorGeneration;
            if(distance<=0L)
                distance=Long.MAX_VALUE/2L+connection.generation;
            if(best==null||priority>bestPriority||
                    (priority==bestPriority&&distance<bestDistance)) {
                best=connection;
                bestPriority=priority;
                bestDistance=distance;
            }
        }
        if(best!=null) {
            gattSchedulerCursorGeneration=best.generation;
            sendFragmentLocked(best);
        }
    }

    @SuppressWarnings("deprecation")
    @SuppressLint("MissingPermission")
    private static void sendFragmentLocked(Connection connection) {
        final Outbound sending=connection.sending;
        if(sending==null||gattOperationConnection!=null)
            return;
        if(sending.index>=sending.fragments.size()) {
            finishSendingLocked(connection,true);
            return;
        }
        final byte[] value=sending.fragments.get(sending.index);
        final int fragmentNumber=sending.index+1;
        final int fragmentCount=sending.fragments.size();
        final String carrier=connection.serverSide?"indication":"write";
        Log.i(LOG_ID,"send "+sending.path+" label="+connectionLabel(connection)+" fragment="+
                fragmentNumber+"/"+fragmentCount+" valueBytes="+value.length+" carrier="+carrier);
        boolean accepted=false;
        try {
            if(connection.serverSide&&gattServer!=null&&connection.device!=null&&connection.serverTx!=null) {
                if(Build.VERSION.SDK_INT>=33)
                    accepted=gattServer.notifyCharacteristicChanged(connection.device,connection.serverTx,true,value)==
                            BluetoothGatt.GATT_SUCCESS;
                else {
                    connection.serverTx.setValue(value);
                    accepted=gattServer.notifyCharacteristicChanged(connection.device,connection.serverTx,true);
                }
            }
            else if(!connection.serverSide&&connection.gatt!=null&&connection.rx!=null) {
                if(Build.VERSION.SDK_INT>=33)
                    accepted=connection.gatt.writeCharacteristic(connection.rx,value,
                            BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT)==BluetoothGatt.GATT_SUCCESS;
                else {
                    connection.rx.setWriteType(BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT);
                    connection.rx.setValue(value);
                    accepted=connection.gatt.writeCharacteristic(connection.rx);
                }
            }
        }
        catch(Throwable error) {
            Log.stack(LOG_ID,"sendFragment",error);
        }
        if(!accepted) {
            if(connection.serverSide&&serverSendingConnection==connection)
                serverSendingConnection=null;
            gattOperationConnection=null;
            final String reason="GATT rejected "+sending.path+" fragment "+fragmentNumber+"/"+
                    fragmentCount+" (valueBytes="+value.length+", mtu="+connection.mtu+
                    ", carrier="+carrier+")";
            if(!connection.serverSide&&PROBE_PATH.equals(sending.path)&&connection.ready&&
                    connection.lastFragmentActivityMillis>0L&&
                    monotonicMillis()-connection.lastFragmentActivityMillis<=PROBE_RECENT_RX_GRACE_MILLIS) {
                // The peer is demonstrably alive: an indication arrived only
                // moments ago.  Some Android stacks reject a client write while
                // they are still completing that indication.  Dropping the
                // diagnostic probe is safer than tearing down an active /data
                // stream and forcing the native mirror to synchronize again.
                Log.w(LOG_ID,reason+"; ignoring liveness-probe rejection because peer data was just received");
                finishSendingLocked(connection,false);
                scheduleProbeLocked(connection,PROBE_IDLE_MILLIS);
                return;
            }
            failQueueLocked(connection);
            handler.post(() -> disconnectForFailure(connection,reason));
            return;
        }
        gattOperationConnection=connection;
        if(connection.serverSide)
            serverSendingConnection=connection;
        final int pendingIndex=sending.index;
        final long epoch=connection.epoch;
        handler.postDelayed(() -> {
            boolean timedOut=false;
            synchronized(lock) {
                if(connection.alive&&connection.epoch==epoch&&connection.sending==sending&&
                        sending.index==pendingIndex&&gattOperationConnection==connection) {
                    failQueueLocked(connection);
                    timedOut=true;
                }
            }
            if(timedOut)
                disconnectForFailure(connection,"BLE GATT timeout for "+sending.path+" fragment "+
                        (pendingIndex+1)+"/"+fragmentCount+" (valueBytes="+value.length+
                        ", mtu="+connection.mtu+", carrier="+carrier+")");
        },FRAGMENT_TIMEOUT_MILLIS);
    }

    private static void fragmentFinished(Connection connection,int statusCode) {
        synchronized(lock) {
            if(gattOperationConnection!=connection) {
                Log.i(LOG_ID,"GATT completion ignored outside arbiter slot: label="+
                        connectionLabel(connection)+" status="+statusCode);
                return;
            }
            // Release the process-wide ATT slot before deciding what comes next.
            // The current connection remains in connection.sending until the
            // fragment/message bookkeeping below advances it.
            gattOperationConnection=null;
            if(connection.serverSide&&serverSendingConnection==connection)
                serverSendingConnection=null;
            if(!connection.alive||connection.sending==null) {
                Log.i(LOG_ID,"GATT completion ignored: label="+connectionLabel(connection)+
                        " status="+statusCode);
                kickGattOperationLocked();
                return;
            }
            final Outbound sending=connection.sending;
            final int fragmentNumber=sending.index+1;
            final int fragmentCount=sending.fragments.size();
            Log.i(LOG_ID,"confirmed "+sending.path+" label="+connectionLabel(connection)+
                    " fragment="+fragmentNumber+"/"+fragmentCount+" status="+statusCode);
            if(statusCode!=BluetoothGatt.GATT_SUCCESS) {
                final String path=sending.path;
                failQueueLocked(connection);
                handler.post(() -> disconnectForFailure(connection,"GATT fragment failed for "+path+
                        " fragment "+fragmentNumber+"/"+fragmentCount+": status="+statusCode));
                return;
            }
            if(connection.ready) {
                connection.lastActivityMillis=monotonicMillis();
                if(isPayloadPath(sending.path))
                    connection.lastPayloadActivityMillis=connection.lastActivityMillis;
            }
            sending.lastProgressMillis=monotonicMillis();
            ++sending.index;
            if(sending.index==fragmentCount)
                finishSendingLocked(connection,true);
            // Whether this message continues or another connection has control
            // traffic waiting, every confirmed fragment yields to the global
            // arbiter. This is the cross-client/server fairness that was missing
            // from the old serverSendingConnection-only serializer.
            kickGattOperationLocked();
        }
    }

    private static void finishSendingLocked(Connection connection,boolean success) {
        final Outbound completed=connection.sending;
        if(completed==null)
            return;
        connection.sending=null;
        if(connection.serverSide&&serverSendingConnection==connection)
            serverSendingConnection=null;
        connection.queuedBytes=Math.max(0,connection.queuedBytes-completed.bytes);
        completed.success=success;
        completed.finished.countDown();
        Log.i(LOG_ID,(success?"sent ":"failed ")+completed.path+" label="+
                connectionLabel(connection)+" fragments="+completed.fragments.size()+
                " queuedBytes="+connection.queuedBytes);
        if(success&&connection.ready) {
            if(!connection.serverSide) {
                connection.lastActivityMillis=monotonicMillis();
                if(PROBE_PATH.equals(completed.path))
                    armProbeReplyCheckLocked(connection);
                scheduleProbeLocked(connection,PROBE_IDLE_MILLIS);
            }
            if(isPayloadPath(completed.path))
                connection.lastPayloadActivityMillis=monotonicMillis();
            maybeScheduleLegacySlotCheckLocked(connection);
        }
        // Prepare this connection's next logical message, if any. The global
        // ATT arbiter decides whether it or another connection gets the slot.
        sendNextLocked(connection);
        kickGattOperationLocked();
    }

    private static void failQueueLocked(Connection connection) {
        if(gattOperationConnection==connection)
            gattOperationConnection=null;
        if(connection.serverSide&&serverSendingConnection==connection)
            serverSendingConnection=null;
        if(connection.sending!=null) {
            connection.sending.success=false;
            connection.sending.finished.countDown();
            connection.sending=null;
        }
        Outbound failed;
        while((failed=connection.outgoing.poll())!=null) {
            failed.success=false;
            failed.finished.countDown();
        }
        connection.queuedBytes=0;
        kickGattOperationLocked();
    }

    private static void receivedFragment(Connection connection,byte[] value) {
        final BleMirrorCodec.Message complete;
        final long epoch;
        synchronized(lock) {
            if(!connection.alive)
                return;
            complete=connection.decoder.accept(value);
            // Activity must be recorded per fragment, not only after a complete
            // logical message.  A single /data message can take tens of seconds
            // on a slow GATT link.  Treating that interval as idle made the
            // client inject /bleprobe writes into an actively arriving stream;
            // on the Realme stack one such write was rejected and caused a
            // needless disconnect/resynchronization.
            final long now=monotonicMillis();
            connection.lastFragmentActivityMillis=now;
            ++connection.inboundGeneration;
            if(connection.probeReplyPending) {
                connection.probeReplyPending=false;
                ++connection.probeReplyGeneration;
                connection.reversePathProbeMisses=0;
            }
            if(connection.ready) {
                connection.lastActivityMillis=now;
                if(connection.serverSide)
                    scheduleServerSessionLivenessLocked(connection,SERVER_LOGICAL_SESSION_TIMEOUT_MILLIS);
            }
            epoch=connection.epoch;
        }
        if(complete==null)
            return;
        Log.i(LOG_ID,"received "+complete.path()+" label="+connectionLabel(connection)+
                " bytes="+complete.data().length);
        incoming.execute(() -> {
            synchronized(lock) {
                if(!connection.alive||epoch!=connection.epoch)
                    return;
            }
            try {
                handleMessage(connection,complete.path(),complete.data());
            }
            catch(Throwable error) {
                Log.stack(LOG_ID,"receive "+complete.path(),error);
                disconnectForFailure(connection,"Invalid BLE mirror message");
            }
        });
    }

    private static void handleMessage(Connection connection,String path,byte[] data)
            throws GeneralSecurityException {
        if(HELLO_PATH.equals(path)) {
            handleHello(connection,data);
            return;
        }
        if(READY_PATH.equals(path)) {
            handleReady(connection,data);
            return;
        }
        final String label;
        final boolean remoteWearable;
        final byte[] plain;
        synchronized(lock) {
            if(!connection.alive)
                return;
            if(!connection.ready||connection.link==null)
                throw new GeneralSecurityException("Unauthenticated BLE message before /blehello");
            final BleMirrorSecurity.Opened opened=BleMirrorSecurity.open(connection.sessionKey,
                    connection.serverSide,connection.receiveCounter,path,data);
            connection.receiveCounter=opened.counter;
            plain=opened.data;
            label=connection.link.label;
            remoteWearable=connection.remoteIsWearable;
            if(connection.serverSide)
                persistPreferredGattRoleLocked(connection,"server received authenticated post-handshake message");
            connection.lastActivityMillis=monotonicMillis();
            if(connection.serverSide)
                scheduleServerSessionLivenessLocked(connection,SERVER_LOGICAL_SESSION_TIMEOUT_MILLIS);
            if(isPayloadPath(path))
                connection.lastPayloadActivityMillis=monotonicMillis();
            maybeScheduleLegacySlotCheckLocked(connection);
        }
        if(CAPABILITIES_PATH.equals(path)) {
            if(plain.length>=3&&plain[0]==BLE_CAPABILITIES_VERSION) {
                final int remoteSdk=(plain[1]&0xff)|((plain[2]&0xff)<<8);
                synchronized(lock) {
                    if(connection.alive&&connection.ready&&connection.link!=null)
                        connection.link.remoteSdk=remoteSdk;
                }
                Log.i(LOG_ID,"authenticated BLE peer capabilities: label="+label+
                        " sdk="+remoteSdk);
            }
            return;
        }
        if(RESOURCE_ROLE_PATH.equals(path)) {
            if(plain.length==1&&plain[0]==RESOURCE_ROLE_BECOME_CLIENT) {
                synchronized(lock) {
                    final Link link=connection.link;
                    if(link!=null&&link.role==ROLE_SERVER) {
                        // The fragile peer is about to stop being a GATT central for
                        // this logical mirror so it can preserve another client.  Keep
                        // our persisted pair direction untouched, but remember that
                        // this process should initiate the replacement connection.
                        link.resourceReverseClientRequested=true;
                        if(link.transport==TRANSPORT_AUTOMATIC)
                            link.automaticFallbackRequested=true;
                        link.discoveryBoostUntilMillis=Math.max(link.discoveryBoostUntilMillis,
                                monotonicMillis()+DISCOVERY_BOOST_MILLIS);
                        link.oppositeRoleNotBeforeMillis=0L;
                        Log.w(LOG_ID,"authenticated peer requested resource-balanced reverse GATT role: label="+
                                link.label+"; will become client after current session closes");
                    }
                }
            }
            return;
        }
        if(PROBE_PATH.equals(path)) {
            if(plain.length==1&&plain[0]==PROBE_ACK) {
                synchronized(lock) {
                    if(connection.alive&&connection.ready) {
                        connection.bidirectionalProbeSupported=true;
                        connection.probeReplyPending=false;
                        ++connection.probeReplyGeneration;
                        connection.reversePathProbeMisses=0;
                    }
                }
                Log.i(LOG_ID,"authenticated bidirectional BLE liveness acknowledgement: label="+label);
                return;
            }
            Log.i(LOG_ID,"authenticated BLE liveness probe accepted: label="+label);
            if(plain.length==1&&plain[0]==PROBE_REQUEST) {
                synchronized(lock) {
                    if(connection.alive&&connection.ready&&
                            enqueueLocked(connection,PROBE_PATH,new byte[]{PROBE_ACK})!=null)
                        Log.i(LOG_ID,"queued authenticated bidirectional BLE liveness acknowledgement: label="+label);
                }
            }
            // Also makes endpoint refresh robust against vendor tethering/hotspot
            // changes that do not produce a useful ConnectivityManager callback.
            sendCurrentAddresses(connection,"BLE liveness");
            return;
        }
        if(ADDRESSES_PATH.equals(path)) {
            final int localIndex;
            synchronized(lock) {
                localIndex=connection.link==null?-1:connection.link.hostIndex;
            }
            // Zero records means that the peer has no usable TCP/IP endpoint
            // right now.  It is deliberately *not* an instruction to forget the
            // last learned IPs: those remain useful bootstrap candidates when
            // Wi-Fi returns, and the TCP password handshake validates identity.
            final boolean noCurrentTcpEndpoints=plain.length==2&&
                    (plain[0]&0xff)==1&&(plain[1]&0xff)==0;
            if(noCurrentTcpEndpoints) {
                if(localIndex>=0)
                    Natives.setMirrorAddresses(localIndex,plain);
                Log.i(LOG_ID,"authenticated BLE endpoint update: label="+label+
                        " peer currently has no TCP/IP endpoint; keeping remembered IPs; "+
                        "requesting Bluetooth carrier");
                MessageSender.peerNetworkUnavailable(label);
                return;
            }
            final boolean changed=localIndex>=0&&Natives.setMirrorAddresses(localIndex,plain);
            boolean retryTcpNow=false;
            synchronized(lock) {
                if(changed&&connection.link!=null&&
                        connection.link.transport==TRANSPORT_AUTOMATIC) {
                    connection.link.newTcpCandidates=true;
                    if(connection.link.carrierActive) {
                        connection.link.newTcpCandidates=false;
                        retryTcpNow=true;
                    }
                }
            }
            Log.i(LOG_ID,"authenticated BLE endpoint update: label="+label+
                    " bytes="+plain.length+" changed="+changed+
                    " retryTcpNow="+retryTcpNow);
            if(retryTcpNow)
                MessageSender.preferTcpAfterAddressUpdate(label,localIndex);
            return;
        }
        MessageReceiver.receiveBle(label,path,plain,remoteWearable);
    }

    private static void handleHello(Connection connection,byte[] data) throws GeneralSecurityException {
        if(!connection.serverSide)
            throw new GeneralSecurityException("A BLE client received /blehello");
        final String label=BleMirrorSecurity.peekLabel(data);
        final Link link;
        synchronized(lock) {
            link=label==null?null:linksByLabel.get(label);
            if(link==null||!offersGattServerLocked(link))
                throw new GeneralSecurityException("Unknown BLE mirror label or GATT role");
        }
        final BleMirrorSecurity.Identity identity=BleMirrorSecurity.verifyHello(link.masterKey,data);
        if(identity.hostIndex>=MAX_REMOTE_HOSTS)
            throw new GeneralSecurityException("Invalid remote mirror index");
        synchronized(lock) {
            if(!connection.alive||linksByLabel.get(label)!=link)
                return;
            if(connection.link!=null&&connection.link!=link) {
                /*
                 * Android 5 can retain ATT handles across a stable multi-service
                 * database.  After another token service has been used, XA1 may
                 * discover the correct UUID but its cached RX/CCCD handles can still
                 * address a different token service on J5.  /blehello is independently
                 * authenticated with the password-derived key selected by its embedded
                 * label, so the authenticated identity is authoritative.  Keep using
                 * the physical characteristic/CCCD that the central subscribed to, but
                 * bind the logical session to the verified Link instead of rejecting it.
                 */
                Log.w(LOG_ID,"authenticated BLE hello arrived through stale Android GATT service handle; rebinding logical session: addressed="+
                        connection.link.label+" authenticated="+link.label+
                        " address="+safeAddress(connection.device));
            }
            if(link.connection!=null&&link.connection!=connection)
                throw new GeneralSecurityException("This BLE mirror already has a peer");
            connection.link=link;
            link.connection=connection;
            connection.clientNonce=identity.nonce;
            connection.serverNonce=BleMirrorSecurity.randomNonce(random);
            connection.sessionKey=BleMirrorSecurity.sessionKey(link.masterKey,
                    connection.clientNonce,connection.serverNonce);
            connection.remoteHostIndex=identity.hostIndex;
            connection.remoteIsWearable=identity.wearable;
            connection.sendCounter=0;
            connection.receiveCounter=0;
            final byte[] readyPacket=BleMirrorSecurity.ready(link.masterKey,link.label,Applic.isWearable,
                    link.hostIndex,connection.serverNonce,connection.clientNonce);
            connection.ready=true;
            link.clientRetryNotBeforeMillis=0;
            link.status=connectedStatus(link,connection);
            if(enqueueLocked(connection,READY_PATH,readyPacket)==null)
                throw new GeneralSecurityException("Could not queue BLE ready response");
            Log.i(LOG_ID,"server handshake accepted: label="+link.label+" local="+link.hostIndex+
                    " remote="+connection.remoteHostIndex+" wearable="+
                    connection.remoteIsWearable+" address="+safeAddress(connection.device)+
                    " physicalRole="+(link.role==ROLE_CLIENT?"reversed-server":"preferred-server"));
            finishServerHandshakeHoldLocked(safeAddress(connection.device),"authenticated");
            if(serverHandshakeHoldRemainingLocked()==0L) {
                stopAdvertisingLocked();
                startAdvertisingLocked();
                // onConnectionStateChange() stops scanning while an incoming
                // central performs its token-addressed handshake.  A successful
                // handshake cancels the timeout which would otherwise call
                // reconcileOperationsLocked(), so resume discovery explicitly.
                // Without this, another preferred-client mirror can remain at
                // "Looking for automatic Bluetooth fallback" forever.
                refreshScanLocked();
            }
        }
        onReady(connection);
    }

    private static void handleReady(Connection connection,byte[] data) throws GeneralSecurityException {
        if(connection.serverSide)
            throw new GeneralSecurityException("A BLE server received /bleready");
        final Link link;
        final byte[] clientNonce;
        synchronized(lock) {
            link=connection.link;
            clientNonce=connection.clientNonce;
            if(link==null||clientNonce==null)
                throw new GeneralSecurityException("Unexpected BLE ready response");
        }
        final BleMirrorSecurity.Identity identity=BleMirrorSecurity.verifyReady(link.masterKey,data,clientNonce);
        if(!link.label.equals(identity.label)||identity.hostIndex>=MAX_REMOTE_HOSTS)
            throw new GeneralSecurityException("BLE mirror identity does not match");
        synchronized(lock) {
            if(!connection.alive||connection.link!=link)
                return;
            connection.serverNonce=identity.nonce;
            connection.sessionKey=BleMirrorSecurity.sessionKey(link.masterKey,
                    connection.clientNonce,connection.serverNonce);
            connection.remoteHostIndex=identity.hostIndex;
            connection.remoteIsWearable=identity.wearable;
            connection.sendCounter=0;
            connection.receiveCounter=0;
            connection.ready=true;
            link.clientRetryNotBeforeMillis=0;
            persistPreferredGattRoleLocked(connection,"client authenticated /bleready");
            link.status=connectedStatus(link,connection);
            Log.i(LOG_ID,"client handshake accepted: label="+link.label+" local="+link.hostIndex+
                    " remote="+connection.remoteHostIndex+" wearable="+
                    connection.remoteIsWearable+" address="+safeAddress(connection.device)+
                    " physicalRole="+(link.role==ROLE_SERVER?"reversed-client":"preferred-client"));
        }
        onReady(connection);
    }

    private static void onReady(Connection connection) {
        final String label;
        final int local;
        final int remote;
        final boolean phonePeer;
        final boolean activateNative;
        final boolean resumeLegacyCarrier;
        synchronized(lock) {
            if(!connection.alive||!connection.ready||connection.link==null)
                return;
            label=connection.link.label;
            local=connection.link.hostIndex;
            remote=connection.remoteHostIndex;
            phonePeer=!Applic.isWearable&&!connection.remoteIsWearable;
            activateNative=connection.link.transport==TRANSPORT_BLUETOOTH;
            resumeLegacyCarrier=connection.link.transport==TRANSPORT_AUTOMATIC&&
                    (connection.link.automaticFallbackRequested||connection.link.legacyResumeCarrier);
            connection.link.carrierActive=activateNative;
            completeLegacyDiscoveryLeaseLocked(connection.link,
                    activateNative||resumeLegacyCarrier);
            connection.link.status=connectedStatus(connection.link,connection);
            connection.lastPayloadActivityMillis=monotonicMillis();
            connection.lastActivityMillis=monotonicMillis();
            if(connection.serverSide)
                scheduleServerSessionLivenessLocked(connection,SERVER_LOGICAL_SESSION_TIMEOUT_MILLIS);
            else {
                scheduleProbeLocked(connection,PROBE_IDLE_MILLIS);
                refreshScanLocked();
                // Client initiation temporarily pauses local advertising for
                // controller compatibility. Resume it only after authentication
                // has completed, when the initiating procedure is over.
                if(serverServiceReady)
                    startAdvertisingLocked();
            }
        }
        if(activateNative) {
            Natives.mirrorTransportReady(local,remote,phonePeer);
            Log.i(LOG_ID,"native Direct Bluetooth carrier ready: label="+label+" local="+local+
                    " remote="+remote+" phonePeer="+phonePeer);
        }
        else
            Log.i(LOG_ID,"Automatic Bluetooth fallback authenticated and standing by: label="+
                    label+" local="+local+" remote="+remote+"; TCP/IP remains preferred");
        sendCurrentAddresses(connection,"BLE handshake");
        synchronized(lock) {
            if(connection.alive&&connection.ready) {
                final int sdk=Build.VERSION.SDK_INT;
                final byte[] capabilities=new byte[]{BLE_CAPABILITIES_VERSION,
                        (byte)(sdk&0xff),(byte)((sdk>>>8)&0xff)};
                if(enqueueLocked(connection,CAPABILITIES_PATH,capabilities)!=null)
                    Log.i(LOG_ID,"queued BLE peer capabilities: label="+label+" sdk="+sdk);
            }
        }
        // Automatic phone mirrors need BLE when either endpoint has no usable
        // TCP/IP address.  Previously only the *remote* zero-address /bleips
        // packet promoted the carrier.  A passive/local no-network endpoint
        // could therefore sit on an authenticated standby GATT link until the
        // other phone happened to lose Wi-Fi.  Promote from our own authoritative
        // address snapshot as well; returning Wi-Fi does not immediately demote
        // a working BLE carrier (the authenticated TCP probe does that safely).
        requestAutomaticCarrierIfLocalNetworkUnavailable(connection);
        // Establish reverse-path probe capability while the freshly subscribed
        // indication path is known to be alive. If the peer is an older build it
        // simply accepts this as the legacy probe and no strict ACK requirement
        // is enabled for this connection.
        if(!connection.serverSide) {
            synchronized(lock) {
                if(connection.alive&&connection.ready&&enqueueProbeRequestLocked(connection))
                    Log.i(LOG_ID,"queued initial bidirectional BLE liveness capability probe: label="+label);
            }
        }
        if(resumeLegacyCarrier) {
            Log.i(LOG_ID,"authenticated Automatic peer still needs Bluetooth carrier: label="+label+
                    " legacyResume="+connection.link.legacyResumeCarrier);
            MessageSender.resumeAutomaticBleCarrier(label);
        }
        synchronized(lock) {
            if(connection.alive&&connection.ready) {
                maybeScheduleLegacySlotCheckLocked(connection);
                maybeScheduleFragileClientSlotCheckLocked(connection);
            }
        }
        if(activateNative&&Applic.isWearable)
            MessageSender.sendAskForStart(label);
    }

    @SuppressLint("WakelockTimeout")
    private static void keepLegacySlotSchedulerAwakeLocked(long millis) {
        if(!legacySingleGattLocked())
            return;
        try {
            if(legacySlotWakeLock==null) {
                final PowerManager manager=(PowerManager)Applic.app.getSystemService(Context.POWER_SERVICE);
                if(manager==null)
                    return;
                legacySlotWakeLock=manager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK,
                        "Juggluco::BleMirrorLegacySlot");
                legacySlotWakeLock.setReferenceCounted(false);
            }
            // Android 5 devices can suspend before a delayed Handler callback runs.
            // Keep the CPU awake only around the short post-sync handoff window.
            legacySlotWakeLock.acquire(Math.max(1000L,millis));
        }
        catch(Throwable error) {
            Log.e(LOG_ID,"Could not hold legacy BLE slot wake lock: "+
                    error.getClass().getSimpleName());
        }
    }

    private static void scheduleLegacySlotCheckLocked(Connection connection,long delayMillis) {
        if(!legacySingleGattLocked()||connection==null||!connection.alive||!connection.ready||
                connection.link==null||connection.legacySlotCheckScheduled)
            return;
        connection.legacySlotCheckScheduled=true;
        final long epoch=connection.epoch;
        // Twelve-second idle timers were observed to turn into multi-minute
        // delays once an Android-5 phone went to sleep.  The slot handoff is
        // deliberately short and bounded, so keep the CPU awake through it.
        keepLegacySlotSchedulerAwakeLocked(Math.max(1L,delayMillis)+3000L);
        handler.postDelayed(() -> runLegacySlotCheck(connection,epoch),Math.max(1L,delayMillis));
    }

    private static void runLegacySlotCheck(Connection connection,long epoch) {
        synchronized(lock) {
            if(connection.epoch!=epoch) {
                connection.legacySlotCheckScheduled=false;
                return;
            }
            connection.legacySlotCheckScheduled=false;
            if(!legacySingleGattLocked()||!connection.alive||!connection.ready||
                    connection.link==null)
                return;
            final Link current=connection.link;
            final long now=monotonicMillis();

            // On API 21-22 an unselected Automatic BLE rendezvous must not sit
            // on the only reliable GATT slot. QR/cached TCP remains the baseline;
            // a later TCP/Message failure will boost this link and rediscover it.
            if(current.transport==TRANSPORT_AUTOMATIC&&!current.carrierActive&&
                    !current.automaticFallbackRequested&&!current.legacyResumeCarrier&&
                    !discoveryBoostedLocked(current)) {
                if(connection.sending!=null||!connection.outgoing.isEmpty()) {
                    scheduleLegacySlotCheckLocked(connection,500L);
                    return;
                }
                Log.i(LOG_ID,"releasing Android 5 BLE standby slot: label="+current.label);
                disconnectConnectionLocked(connection,"Android 5 BLE standby slot released",true);
                reconcileOperationsLocked();
                return;
            }

            final Link waiting=legacyWaitingPeerLocked(current);
            if(waiting==null) {
                final long suppressedDelay=legacySuppressedPeerDelayLocked(current);
                if(suppressedDelay>=0L) {
                    Log.i(LOG_ID,"Android 5 BLE peer is still in its discovery lease; rechecking slot in "+
                            suppressedDelay+" ms");
                    scheduleLegacySlotCheckLocked(connection,suppressedDelay);
                }
                return;
            }

            final boolean currentCarrier=current.transport==TRANSPORT_BLUETOOTH||
                    current.carrierActive||current.legacyResumeCarrier;
            final long carrierLeaseRemaining=currentCarrier?
                    current.legacySlotLeaseUntilMillis-now:0L;
            if(carrierLeaseRemaining>0L) {
                Log.i(LOG_ID,"deferring Android 5 BLE slot handoff for active carrier lease: label="+
                        current.label+" waiting="+waiting.label+
                        " remaining="+carrierLeaseRemaining+" ms");
                scheduleLegacySlotCheckLocked(connection,carrierLeaseRemaining);
                return;
            }
            if(connection.sending!=null||!connection.outgoing.isEmpty()) {
                scheduleLegacySlotCheckLocked(connection,500L);
                return;
            }
            if(connection.decoder.inProgress()) {
                Log.i(LOG_ID,"deferring Android 5 BLE slot handoff during incoming fragmented message: label="+
                        current.label);
                scheduleLegacySlotCheckLocked(connection,500L);
                return;
            }
            final long lastPayload=connection.lastPayloadActivityMillis==0L?
                    now:connection.lastPayloadActivityMillis;
            final long lastTraffic=Math.max(lastPayload,connection.lastFragmentActivityMillis);
            final long idle=Math.max(0L,now-lastTraffic);
            if(idle<LEGACY_SLOT_IDLE_MILLIS) {
                scheduleLegacySlotCheckLocked(connection,LEGACY_SLOT_IDLE_MILLIS-idle);
                return;
            }

            final long leaseUntil=now+LEGACY_SLOT_LEASE_MILLIS;
            current.legacySlotYieldUntilMillis=Math.max(current.legacySlotYieldUntilMillis,leaseUntil);
            waiting.legacySlotLeaseUntilMillis=Math.max(waiting.legacySlotLeaseUntilMillis,leaseUntil);
            // A reverse-probe timer may have been armed for the old discovery
            // owner before this GATT connection was established.  Give the new
            // peer a fresh lease: advertise/scan its normal role first, then let
            // its own 12-second reverse probe run if it is still needed.
            reverseProbeUntilMillis=0;
            reverseProbeScheduled=false;
            ++reverseProbeGeneration;
            if(current.transport==TRANSPORT_AUTOMATIC&&current.carrierActive) {
                current.legacyResumeCarrier=true;
                Log.i(LOG_ID,"preserving Automatic Bluetooth carrier intent across Android 5 slot handoff: label="+
                        current.label);
            }
            Log.i(LOG_ID,"yielding Android 5 BLE slot: current="+current.label+
                    " next="+waiting.label+" lease="+LEGACY_SLOT_LEASE_MILLIS+" ms");
            disconnectConnectionLocked(connection,"Yielding Android 5 BLE slot to "+waiting.label,true);
            reconcileOperationsLocked();
            final long generation=configurationGeneration;
            handler.postDelayed(() -> {
                synchronized(lock) {
                    if(generation!=configurationGeneration)
                        return;
                    reconcileOperationsLocked();
                    final Connection occupied=legacyOccupiedConnectionLocked();
                    if(occupied!=null&&occupied.ready)
                        maybeScheduleLegacySlotCheckLocked(occupied);
                }
            },LEGACY_SLOT_LEASE_MILLIS);
        }
    }

    private static void maybeScheduleLegacySlotCheckLocked(Connection connection) {
        if(!legacySingleGattLocked()||connection==null||!connection.alive||!connection.ready||
                connection.link==null)
            return;
        // The API-21/22 restriction is on Juggluco acting as a GATT *central*.
        // J5's controller demonstrably accepts a second incoming peripheral
        // connection while another authenticated server-side mirror is alive.
        // Do not time-slice/disconnect healthy incoming server sessions merely
        // because another server service also needs Bluetooth.
        if(connection.serverSide)
            return;
        final Link link=connection.link;
        if(link.legacyResumeCarrier) {
            // Give the acknowledged /messages=true resume job time to run.
            scheduleLegacySlotCheckLocked(connection,LEGACY_SLOT_IDLE_MILLIS);
            return;
        }
        if(link.transport==TRANSPORT_AUTOMATIC&&!link.carrierActive&&
                !link.automaticFallbackRequested&&!discoveryBoostedLocked(link)) {
            scheduleLegacySlotCheckLocked(connection,LEGACY_STANDBY_RELEASE_MILLIS);
            return;
        }
        if(legacyWaitingPeerLocked(link)!=null)
            scheduleLegacySlotCheckLocked(connection,LEGACY_SLOT_IDLE_MILLIS);
    }

    /**
     * Automatic phone/tablet mirrors must not wait for a failed TCP connect when
     * this device already knows that it has no current IP endpoint.  Request BLE
     * discovery immediately, even before a GATT session exists.  This is what a
     * forced-Bluetooth row effectively did in the J5 <-> S5 test.
     */
    private static void requestAutomaticPhoneFallbacksWithoutLocalNetwork(String reason) {
        final ArrayList<Integer> indexes=new ArrayList<>();
        final ArrayList<String> labels=new ArrayList<>();
        synchronized(lock) {
            for(Link link:linksByIndex.values()) {
                if(link.configError!=null||link.transport!=TRANSPORT_AUTOMATIC||
                        link.carrierActive||Applic.isWearable||link.wearOS)
                    continue;
                indexes.add(link.hostIndex);
                labels.add(link.label);
            }
        }
        if(indexes.isEmpty())
            return;
        final byte[] addresses;
        try {
            // The payload describes this device's interfaces and is therefore
            // identical for every phone-phone mirror row. Query it once.
            addresses=Natives.getMirrorAddresses(indexes.get(0));
        }
        catch(Throwable error) {
            Log.stack(LOG_ID,"get local Automatic mirror addresses for carrier selection",error);
            return;
        }
        if(addresses==null||addresses.length!=2||(addresses[0]&0xff)!=1||
                (addresses[1]&0xff)!=0)
            return;
        for(String label:labels) {
            Log.i(LOG_ID,"Automatic mirror has no local TCP/IP endpoint; requesting Bluetooth immediately: label="+
                    label+" reason="+reason);
            MessageSender.localNetworkUnavailable(label);
        }
    }

    private static void requestAutomaticCarrierIfLocalNetworkUnavailable(Connection connection) {
        final int localIndex;
        final String label;
        synchronized(lock) {
            if(connection==null||!connection.alive||!connection.ready||connection.link==null||
                    connection.link.transport!=TRANSPORT_AUTOMATIC||
                    Applic.isWearable||connection.remoteIsWearable)
                return;
            localIndex=connection.link.hostIndex;
            label=connection.link.label;
        }
        final byte[] addresses;
        try {
            addresses=Natives.getMirrorAddresses(localIndex);
        }
        catch(Throwable error) {
            Log.stack(LOG_ID,"get local BLE mirror addresses for carrier selection",error);
            return;
        }
        if(addresses!=null&&addresses.length==2&&(addresses[0]&0xff)==1&&
                (addresses[1]&0xff)==0) {
            Log.i(LOG_ID,"Automatic BLE peer has no local TCP/IP endpoint; requesting Bluetooth carrier: label="+
                    label);
            MessageSender.localNetworkUnavailable(label);
        }
    }

    private static void sendCurrentAddresses(Connection connection,String reason) {
        final int localIndex;
        synchronized(lock) {
            if(connection==null||!connection.alive||!connection.ready||connection.link==null)
                return;
            localIndex=connection.link.hostIndex;
        }
        final byte[] addresses;
        try {
            addresses=Natives.getMirrorAddresses(localIndex);
        }
        catch(Throwable error) {
            Log.stack(LOG_ID,"get BLE mirror addresses",error);
            return;
        }
        if(addresses==null||addresses.length<2)
            return;
        synchronized(lock) {
            if(!connection.alive||!connection.ready||connection.link==null)
                return;
            if(Arrays.equals(connection.lastAddressPayload,addresses))
                return;
            if(enqueueLocked(connection,ADDRESSES_PATH,addresses)!=null) {
                connection.lastAddressPayload=addresses.clone();
                Log.i(LOG_ID,"queued authenticated BLE endpoint update: label="+
                        connection.link.label+" bytes="+addresses.length+" reason="+reason);
            }
        }
    }

    /**
     * Publish the network state that is visible *now*.  Do not stop/restart an
     * already-on-air advertisement merely to update its compact IP hint.  On the
     * Huawei M5 such a successful-looking restart could leave the identity
     * invisible to the remote scanner for minutes.  The hint is only an
     * optimization: authenticated /bleips carries the authoritative current
     * addresses after BLE connects, while remembered IPs remain available for
     * future TCP authentication.
     */
    private static void refreshNetworkState(String reason) {
        final ArrayList<Connection> ready=new ArrayList<>();
        synchronized(lock) {
            for(Link link:linksByIndex.values())
                if(link.connection!=null&&link.connection.alive&&link.connection.ready)
                    ready.add(link.connection);
            if(advertising&&advertisedLink!=null&&
                    advertisedLink.transport==TRANSPORT_AUTOMATIC&&
                    advertisedLink.connection==null)
                Log.i(LOG_ID,reason+
                        "; keeping existing Automatic BLE advertisement on air; endpoint hint refresh deferred: label="+
                        advertisedLink.label);
        }
        for(Connection connection:ready)
            sendCurrentAddresses(connection,reason);
        requestAutomaticPhoneFallbacksWithoutLocalNetwork(reason);
    }

    /** Resend current TCP candidates after Android reports an address/network change. */
    public static void networkChanged() {
        final long generation;
        synchronized(lock) {
            generation=++networkChangeGeneration;
        }
        refreshNetworkState("network changed");

        // Android often delivers onLost() before wlan0 has actually lost its
        // address.  The immediate pass above may therefore still see the old IP
        // and conclude that TCP is available.  Re-run once after the interface
        // state settles.  A later network callback invalidates this delayed pass.
        handler.postDelayed(() -> {
            synchronized(lock) {
                if(generation!=networkChangeGeneration)
                    return;
            }
            Log.i(LOG_ID,"network change settled; rechecking Automatic carrier availability");
            refreshNetworkState("settled network change");
        },NETWORK_CHANGE_SETTLE_MILLIS);
    }

    /**
     * Android can retain a half-working client-side GATT object: writes still
     * succeed while onCharacteristicChanged() silently stops. Current peers
     * therefore acknowledge /bleprobe over the indication path. Older peers
     * harmlessly accept the request but do not ACK it; strict reverse-path
     * checks are enabled only after an ACK has proved support on this session.
     */
    private static void scheduleServerSessionLivenessLocked(Connection connection,long delayMillis) {
        if(connection==null||!connection.serverSide||!connection.alive||!connection.ready||
                connection.serverLivenessScheduled)
            return;
        connection.serverLivenessScheduled=true;
        final long epoch=connection.epoch;
        handler.postDelayed(() -> runServerSessionLiveness(connection,epoch),
                Math.max(1L,delayMillis));
    }

    private static void runServerSessionLiveness(Connection connection,long epoch) {
        synchronized(lock) {
            if(connection.epoch!=epoch) {
                connection.serverLivenessScheduled=false;
                return;
            }
            connection.serverLivenessScheduled=false;
            if(!connection.serverSide||!connection.alive||!connection.ready||connection.link==null)
                return;
            final long idle=Math.max(0L,monotonicMillis()-connection.lastActivityMillis);
            // A selected Direct/Automatic BLE data carrier must not be expired only
            // because this server has not seen an application packet recently.
            // The remote central owns the bidirectional probe timer and will reconnect
            // if the GATT path really dies.  This also avoids converting an M5 with
            // Wi-Fi off to a fictitious TCP/IP carrier after an arbitrary 65 seconds.
            if(isDataCarrierSelectedLocked(connection.link)) {
                scheduleServerSessionLivenessLocked(connection,
                        SERVER_LOGICAL_SESSION_TIMEOUT_MILLIS);
                return;
            }
            if(idle<SERVER_LOGICAL_SESSION_TIMEOUT_MILLIS) {
                scheduleServerSessionLivenessLocked(connection,
                        SERVER_LOGICAL_SESSION_TIMEOUT_MILLIS-idle);
                return;
            }
            final String address=safeAddress(connection.device);
            Log.w(LOG_ID,"expiring silent logical BLE server session: label="+
                    connection.link.label+" address="+address+" idleMillis="+idle+
                    " physicalPeerStillShared="+hasOtherServerSessionForAddressLocked(connection,address));
            disconnectConnectionLocked(connection,"BLE mirror logical session timed out",true);
            reconcileOperationsLocked();
        }
    }

    private static boolean hasOtherServerSessionForAddressLocked(Connection except,String address) {
        for(Connection other:serverConnections.values())
            if(other!=except&&other.alive&&sameAddress(other,address))
                return true;
        return false;
    }

    private static boolean enqueueProbeRequestLocked(Connection connection) {
        if(connection==null||connection.serverSide||!connection.alive||!connection.ready)
            return false;
        connection.probeRequestBaselineInboundGeneration=connection.inboundGeneration;
        return enqueueLocked(connection,PROBE_PATH,new byte[]{PROBE_REQUEST})!=null;
    }

    private static void armProbeReplyCheckLocked(Connection connection) {
        if(connection==null||connection.serverSide||!connection.alive||!connection.ready||
                !connection.bidirectionalProbeSupported)
            return;
        if(connection.inboundGeneration!=connection.probeRequestBaselineInboundGeneration) {
            connection.reversePathProbeMisses=0;
            return;
        }
        connection.probeReplyPending=true;
        final long generation=++connection.probeReplyGeneration;
        final long epoch=connection.epoch;
        handler.postDelayed(() -> checkProbeReply(connection,epoch,generation),
                PROBE_REPLY_TIMEOUT_MILLIS);
    }

    private static void checkProbeReply(Connection connection,long epoch,long generation) {
        boolean reconnect=false;
        synchronized(lock) {
            if(!connection.alive||connection.serverSide||!connection.ready||
                    connection.epoch!=epoch||connection.probeReplyGeneration!=generation||
                    !connection.probeReplyPending)
                return;
            connection.probeReplyPending=false;
            ++connection.probeReplyGeneration;
            ++connection.reversePathProbeMisses;
            reconnect=true;
        }
        if(reconnect) {
            // Once indications have disappeared, re-subscribing the CCCD inside the
            // same encrypted session is not sufficient.  The server may already have
            // advanced its authenticated send counter for indications Android dropped;
            // the first indication after a CCCD refresh then correctly fails the strict
            // counter check.  Reconnect instead: this creates a fresh subscription and
            // fresh session counters without accepting a counter gap.
            Log.w(LOG_ID,"BLE reverse indication path stopped responding; reconnecting GATT session: label="+
                    connectionLabel(connection)+" address="+safeAddress(connection.device));
            disconnectForFailure(connection,"BLE reverse indication path stopped responding");
        }
    }

    private static void scheduleProbeLocked(Connection connection,long delayMillis) {
        if(connection==null||connection.serverSide||!connection.alive||!connection.ready||
                connection.probeScheduled)
            return;
        long actualDelay=delayMillis;
        if(delayMillis==PROBE_IDLE_MILLIS) {
            connection.probeIdleTargetMillis=PROBE_IDLE_MILLIS+
                    (PROBE_IDLE_JITTER_MILLIS<=0L?0L:
                            (long)random.nextInt((int)PROBE_IDLE_JITTER_MILLIS+1));
            actualDelay=connection.probeIdleTargetMillis;
        }
        connection.probeScheduled=true;
        final long epoch=connection.epoch;
        handler.postDelayed(() -> runProbe(connection,epoch),Math.max(1L,actualDelay));
    }

    private static void runProbe(Connection connection,long epoch) {
        boolean enqueueFailed=false;
        synchronized(lock) {
            if(connection.epoch!=epoch) {
                connection.probeScheduled=false;
                return;
            }
            connection.probeScheduled=false;
            if(connection.serverSide||!connection.alive||!connection.ready)
                return;
            final long idleMillis=Math.max(0L,
                    monotonicMillis()-connection.lastActivityMillis);
            final long idleTarget=connection.probeIdleTargetMillis>0L?
                    connection.probeIdleTargetMillis:PROBE_IDLE_MILLIS;
            if(idleMillis<idleTarget) {
                scheduleProbeLocked(connection,idleTarget-idleMillis);
                return;
            }
            if(connection.serviceRefreshInProgress||connection.sending!=null||
                    !connection.outgoing.isEmpty()) {
                scheduleProbeLocked(connection,PROBE_BUSY_RETRY_MILLIS);
                return;
            }
            Log.i(LOG_ID,"probing idle BLE connection: label="+connectionLabel(connection)+
                    " idleMillis="+idleMillis+" bidirectional="+
                    connection.bidirectionalProbeSupported);
            enqueueFailed=!enqueueProbeRequestLocked(connection);
        }
        if(enqueueFailed)
            disconnectForFailure(connection,"Could not queue BLE liveness probe");
    }

    private static long monotonicMillis() {
        return TimeUnit.NANOSECONDS.toMillis(System.nanoTime());
    }

    private static String connectedStatus(Link link,Connection connection) {
        final String address=" ("+safeAddress(connection.device)+")";
        if(link.transport==TRANSPORT_AUTOMATIC)
            return link.carrierActive?
                    "Bluetooth fallback active: "+link.label+address:
                    "Bluetooth fallback ready; TCP/IP preferred: "+link.label+address;
        return "BLE mirror connected: "+link.label+address;
    }

    private static String waitingStatus(Link link) {
        final boolean offering=offersGattServerLocked(link);
        if(link.transport==TRANSPORT_AUTOMATIC)
            return (offering?"Offering automatic Bluetooth fallback ":
                    "Looking for automatic Bluetooth fallback ")+link.label;
        return (offering?"Offering direct Bluetooth mirror ":
                "Looking for direct Bluetooth mirror ")+link.label;
    }

    private static String connectionLabel(Connection connection) {
        synchronized(lock) {
            return connection.link==null?"authenticating":connection.link.label;
        }
    }

    private static String safeAddress(BluetoothDevice device) {
        if(device==null)
            return "unknown address";
        try {
            final String address=device.getAddress();
            return address==null||address.isEmpty()?"unknown address":address;
        }
        catch(Throwable ignored) {
            return "unknown address";
        }
    }

    /** A non-secret diagnostic for the HMAC token already sent on the air. */
    private static String tokenFingerprint(byte[] token) {
        if(token==null||token.length==0)
            return "unavailable";
        final char[] hex="0123456789abcdef".toCharArray();
        final int bytes=Math.min(4,token.length);
        final char[] output=new char[bytes*2];
        for(int index=0;index<bytes;++index) {
            final int value=token[index]&0xff;
            output[index*2]=hex[value>>>4];
            output[index*2+1]=hex[value&0xf];
        }
        return new String(output);
    }

    /**
     * Persist a physical BLE direction only after it has proved itself through
     * authentication. side remains immutable pair identity. Ordinary nearby
     * peers persist the SAME pair-wide blereverse value, so their opposite side
     * values mathematically guarantee complementary client/server roles.
     */
    private static void persistPreferredGattRoleLocked(Connection connection,String proof) {
        if(connection==null||!connection.alive||!connection.ready||connection.link==null)
            return;
        final Link link=connection.link;
        if(link.wearOS)
            return;
        final boolean actualClient=!connection.serverSide;
        final int actualRole=actualClient?ROLE_CLIENT:ROLE_SERVER;
        final boolean actualReverse=actualClient==link.side;
        link.temporaryReverseUntilMillis=0L;
        ++link.temporaryReverseGeneration;
        link.transientClientFailures=0;
        link.advertiseResourceFailures=0;

        // Existing/previously authenticated directions are authoritative.  A
        // transient Android failure must never cause a connection that has
        // worked for hours or days to rewrite its persisted direction.  This
        // is the central invariant of v21.
        if(link.directionProven) {
            if(actualRole!=link.role||actualReverse!=link.persistedPairReverse)
                Log.w(LOG_ID,"authenticated BLE session used the opposite physical role, but the proven persisted direction remains locked: label="+
                        link.label+" side="+(link.side?"r2":"r1")+
                        " persisted="+(link.persistedPairReverse?"reversed":"normal")+
                        " actual="+(actualReverse?"reversed":"normal")+
                        " proof="+proof);
            return;
        }

        // Only a newly created, still-unproven connection may learn a physical
        // direction.  Its first authenticated direction becomes permanent.
        final boolean savedDirection=Natives.setbackupblereverse(link.hostIndex,actualReverse);
        Natives.setbackupbleclient(link.hostIndex,actualClient);
        final boolean savedProof=Natives.setbackupbleunproven(link.hostIndex,false);
        final int oldRole=link.role;
        final boolean oldReverse=link.persistedPairReverse;
        link.persistedPairReverse=actualReverse;
        link.directionProven=true;
        link.role=actualRole;
        link.reverseGattRole=false;
        link.clientRetryNotBeforeMillis=0L;
        link.oppositeRoleNotBeforeMillis=Long.MAX_VALUE;
        Log.i(LOG_ID,"locked first proven physical Bluetooth GATT direction: label="+link.label+
                " side="+(link.side?"r2":"r1")+
                " old="+(oldRole==ROLE_CLIENT?"client":"server")+
                " new="+(actualClient?"client":"server")+
                " direction="+(actualReverse?"reversed":"normal")+
                " persistedDirectionWas="+(oldReverse?"reversed":"normal")+
                " proof="+proof+" savedDirection="+savedDirection+
                " savedProof="+savedProof);
    }

    private static long serverHandshakeHoldRemainingLocked() {
        if(serverHandshakeHoldUntil.isEmpty())
            return 0L;
        final long now=monotonicMillis();
        long remaining=0L;
        final ArrayList<String> expired=new ArrayList<>();
        for(java.util.Map.Entry<String,Long> entry:serverHandshakeHoldUntil.entrySet()) {
            final long left=entry.getValue()-now;
            if(left<=0L)
                expired.add(entry.getKey());
            else
                remaining=Math.max(remaining,left);
        }
        for(String address:expired) {
            serverHandshakeHoldUntil.remove(address);
            // This helper can observe expiry just before the delayed expiry job
            // runs. Preserve the same orphan marker here so that race cannot
            // lose the later advertiser-recovery trigger.
            if(!hasServerSessionForAddressLocked(address))
                expiredUnauthenticatedServerAddresses.add(address);
        }
        return remaining;
    }

    private static void beginServerHandshakeHoldLocked(String address,long generation) {
        final long until=monotonicMillis()+SERVER_HANDSHAKE_HOLD_MILLIS;
        expiredUnauthenticatedServerAddresses.remove(address);
        serverHandshakeHoldUntil.put(address,until);
        handler.postDelayed(() -> {
            synchronized(lock) {
                if(!currentServerLocked(generation))
                    return;
                final Long current=serverHandshakeHoldUntil.get(address);
                if(current==null||current!=until)
                    return;
                serverHandshakeHoldUntil.remove(address);
                final boolean logicalSession=hasServerSessionForAddressLocked(address);
                // Do not cancel a purely physical/foreign GATT device here.
                // Android associates an incoming LE ACL with every registered
                // GATT server. BluetoothGattServer.cancelConnection() reaches
                // GATT_Disconnect and can therefore destroy a healthy session
                // owned by another Juggluco process or another application.
                //
                // What we do need to remember is that a physical connection can
                // coincide with a controller-level advertising failure. The M5
                // HCI trace showed LE Set Advertising Enable returning 0x09 while
                // Java still called onStartSuccess(). When such a physical link
                // later disconnects we force a real advertiser restart rather
                // than trust advertising=true. We never evict it merely because
                // this process did not authenticate it: another GATT server/process
                // may legitimately own the same physical ACL.
                if(!logicalSession)
                    expiredUnauthenticatedServerAddresses.add(address);
                Log.i(LOG_ID,"incoming BLE handshake discovery hold expired: address="+address+
                        " logicalSession="+logicalSession+
                        (logicalSession?"":"; preserving foreign physical link and watching for advertiser recovery"));
                reconcileOperationsLocked();
            }
        },SERVER_HANDSHAKE_HOLD_MILLIS);
    }

    private static void finishServerHandshakeHoldLocked(String address,String reason) {
        expiredUnauthenticatedServerAddresses.remove(address);
        if(serverHandshakeHoldUntil.remove(address)!=null)
            Log.i(LOG_ID,"incoming BLE handshake discovery hold released: address="+address+
                    " reason="+reason);
    }

    private static long bluetoothRestartSettleRemainingLocked() {
        if(bluetoothRestartSettleUntilMillis<=0L)
            return 0L;
        final long remaining=bluetoothRestartSettleUntilMillis-monotonicMillis();
        if(remaining<=0L) {
            bluetoothRestartSettleUntilMillis=0L;
            return 0L;
        }
        return remaining;
    }

    private static void reconcileOperationsLocked() {
        final long restartSettle=bluetoothRestartSettleRemainingLocked();
        if(restartSettle>0L) {
            stopScanLocked();
            stopAdvertisingLocked();
            for(Link link:linksByIndex.values())
                if(link!=null&&link.configError==null&&!isReadyLocked(link))
                    link.status="Waiting for Bluetooth service restart to settle";
            return;
        }
        boolean wantServer=false;
        boolean wantClient=false;
        for(Link link:linksByIndex.values()) {
            if(link.configError!=null) {
                link.status=link.configError;
                continue;
            }
            final String blocker=operationBlockerLocked(link);
            if(blocker!=null) {
                if(link.connection!=null)
                    disconnectConnectionLocked(link.connection,blocker,true);
                link.status=blocker;
                Log.e(LOG_ID,"link "+link.label+" blocked: "+blocker);
                continue;
            }
            if(offersGattServerLocked(link))
                wantServer=true;
            if(link.role==ROLE_CLIENT&&!link.reverseGattRole)
                wantClient=true;
            if(isReadyLocked(link))
                link.status=connectedStatus(link,link.connection);
            else
                link.status=waitingStatus(link);
        }
        if(wantServer)
            ensureServerLocked();
        else
            stopServerLocked("Bluetooth mirror server is not configured");
        final boolean incomingHandshake=serverHandshakeHoldRemainingLocked()>0L;
        if(!incomingHandshake&&(wantClient||reverseProbeActiveLocked()))
            refreshScanLocked();
        else
            stopScanLocked();
        if(wantServer&&serverServiceReady&&!reverseProbeActiveLocked()&&!incomingHandshake)
            startAdvertisingLocked();
    }

    private static String operationBlockerLocked(Link link) {
        if(adapter==null||bluetoothManager==null)
            return "Bluetooth LE is unavailable on this device";
        if(Build.VERSION.SDK_INT>=31) {
            if(!Applic.bluetoothPermissionGranted(Manifest.permission.BLUETOOTH_CONNECT))
                return "Nearby devices permission for Bluetooth connections is not granted";
            if(link.role==ROLE_CLIENT&&!link.reverseGattRole&&
                    !Applic.bluetoothPermissionGranted(Manifest.permission.BLUETOOTH_SCAN))
                return "Nearby devices permission for Bluetooth scanning is not granted";
            if(offersGattServerLocked(link)&&
                    !Applic.bluetoothPermissionGranted(Manifest.permission.BLUETOOTH_ADVERTISE))
                return "Nearby devices permission for Bluetooth advertising is not granted";
        }
        else if(link.role==ROLE_CLIENT&&!link.reverseGattRole&&!Applic.mayscan())
            return "Location permission for Bluetooth scanning is not granted";
        try {
            if(!adapter.isEnabled())
                return "Bluetooth is switched off";
        }
        catch(SecurityException error) {
            return "Nearby devices permission for Bluetooth is not granted";
        }
        catch(Throwable error) {
            Log.stack(LOG_ID,"read Bluetooth adapter state",error);
            return "Bluetooth adapter state could not be read";
        }
        return null;
    }

    private static ServerEndpoint makeServerEndpointLocked(Link link) {
        final UUID serviceUuid=discoveryUuid(link.discoveryToken);
        final BluetoothGattService service=new BluetoothGattService(serviceUuid,
                BluetoothGattService.SERVICE_TYPE_PRIMARY);
        final BluetoothGattCharacteristic rx=new BluetoothGattCharacteristic(
                rxUuid(link.discoveryToken),BluetoothGattCharacteristic.PROPERTY_WRITE,
                BluetoothGattCharacteristic.PERMISSION_WRITE);
        final BluetoothGattCharacteristic tx=new BluetoothGattCharacteristic(
                txUuid(link.discoveryToken),BluetoothGattCharacteristic.PROPERTY_INDICATE,
                BluetoothGattCharacteristic.PERMISSION_READ);
        final BluetoothGattDescriptor cccd=new BluetoothGattDescriptor(CCCD_UUID,
                BluetoothGattDescriptor.PERMISSION_READ|BluetoothGattDescriptor.PERMISSION_WRITE);
        tx.addDescriptor(cccd);
        service.addCharacteristic(rx);
        service.addCharacteristic(tx);
        return new ServerEndpoint(link,service,rx,tx);
    }

    private static boolean currentServerEndpointLocked(ServerEndpoint endpoint) {
        if(endpoint==null||!endpoint.active||endpoint.link==null)
            return false;
        return linksByIndex.get(endpoint.link.hostIndex)==endpoint.link&&
                endpoint.link.configError==null;
    }

    private static boolean allCurrentServerServicesReadyLocked() {
        for(Link link:linksByIndex.values()) {
            if(link.configError!=null)
                continue;
            final ServerEndpoint endpoint=serverEndpoints.get(discoveryUuid(link.discoveryToken));
            if(endpoint==null||!endpoint.active||endpoint.link!=link||!endpoint.ready)
                return false;
        }
        return true;
    }

    /**
     * Reconcile the logical mirror set with an already-open GATT database without
     * deleting installed services. BluetoothGatt handles are cached aggressively
     * by old Android clients. Clearing/re-adding services while another mirror is
     * connected can therefore make an old handle point at a different token
     * service. Existing services are retained, newly enabled tokens are appended,
     * and retired token services remain installed but inactive until the server is
     * naturally recreated (Bluetooth restart/app process restart).
     */
    private static void ensureCurrentServerServicesLocked(long generation) {
        if(!currentServerLocked(generation)||gattServer==null)
            return;

        for(ServerEndpoint endpoint:serverEndpoints.values())
            endpoint.active=false;

        final boolean queueWasEmpty=serverServiceQueue.isEmpty();
        int appended=0;
        for(Link link:linksByIndex.values()) {
            if(link.configError!=null)
                continue;
            final UUID serviceUuid=discoveryUuid(link.discoveryToken);
            ServerEndpoint endpoint=serverEndpoints.get(serviceUuid);
            if(endpoint!=null) {
                endpoint.link=link;
                endpoint.active=true;
                serverCharacteristics.put(endpoint.rx.getUuid(),endpoint);
                serverCharacteristics.put(endpoint.tx.getUuid(),endpoint);
                continue;
            }
            endpoint=makeServerEndpointLocked(link);
            serverEndpoints.put(serviceUuid,endpoint);
            serverCharacteristics.put(endpoint.rx.getUuid(),endpoint);
            serverCharacteristics.put(endpoint.tx.getUuid(),endpoint);
            serverServiceQueue.add(endpoint);
            ++appended;
        }

        if(appended>0) {
            serverServiceReady=false;
            Log.i(LOG_ID,"appending "+appended+
                    " token-addressed BLE mirror service(s) without renumbering existing GATT handles"+
                    (legacySingleGattLocked()?" (Android 5 stable database)":""));
            if(queueWasEmpty)
                addNextServerServiceLocked(generation);
        }
        else if(serverServiceQueue.isEmpty())
            serverServiceReady=allCurrentServerServicesReadyLocked();
    }

    @SuppressLint("MissingPermission")
    private static void ensureServerLocked() {
        if(bluetoothRestartSettleRemainingLocked()>0L)
            return;
        if(gattServer!=null) {
            ensureCurrentServerServicesLocked(serverGeneration);
            return;
        }
        try {
            final long generation=++serverGeneration;
            serverServiceReady=false;
            serverEndpoints.clear();
            serverCharacteristics.clear();
            serverServiceQueue.clear();
            serverDeviceMtu.clear();
            gattServer=bluetoothManager.openGattServer(Applic.app,newServerCallback(generation));
            if(gattServer==null) {
                setServerStatusesLocked("This device cannot open a BLE GATT server");
                return;
            }
            // A newly opened server has no live mirror handles. Build the initial
            // token database once; later configuration changes only append/rebind.
            try {
                gattServer.clearServices();
                Log.i(LOG_ID,"cleared BLE GATT services on newly opened mirror server");
            }
            catch(Throwable error) {
                Log.w(LOG_ID,"could not clear BLE GATT services on new server: "+
                        error.getClass().getSimpleName());
            }
            ensureCurrentServerServicesLocked(generation);
            if(serverServiceQueue.isEmpty()) {
                serverServiceReady=allCurrentServerServicesReadyLocked();
                return;
            }
            Log.i(LOG_ID,"opening stable token-addressed GATT server for "+serverServiceQueue.size()+
                    " Bluetooth mirror service(s)"+
                    (legacySingleGattLocked()?" (Android 5 serialized radio)":""));
            // ensureCurrentServerServicesLocked() already started the first queued
            // service when the queue was initially empty.
        }
        catch(Throwable error) {
            Log.stack(LOG_ID,"startServer",error);
            setServerStatusesLocked("Could not start BLE mirror server");
            ++serverGeneration;
            if(gattServer!=null)
                try { gattServer.close(); } catch(Throwable ignored) {}
            gattServer=null;
            serverServiceReady=false;
            serverEndpoints.clear();
            serverCharacteristics.clear();
            serverServiceQueue.clear();
            serverDeviceMtu.clear();
        }
    }

    private static void dropFailedServerEndpointLocked(ServerEndpoint endpoint,String reason) {
        if(endpoint==null)
            return;
        serverServiceQueue.remove(endpoint);
        endpoint.ready=false;
        endpoint.active=false;
        if(serverEndpoints.get(endpoint.service.getUuid())==endpoint)
            serverEndpoints.remove(endpoint.service.getUuid());
        if(serverCharacteristics.get(endpoint.rx.getUuid())==endpoint)
            serverCharacteristics.remove(endpoint.rx.getUuid());
        if(serverCharacteristics.get(endpoint.tx.getUuid())==endpoint)
            serverCharacteristics.remove(endpoint.tx.getUuid());
        if(endpoint.link!=null&&linksByIndex.get(endpoint.link.hostIndex)==endpoint.link)
            endpoint.link.status=reason;
        Log.e(LOG_ID,reason+"; preserving already-installed BLE mirror services");
    }

    private static void addNextServerServiceLocked(long generation) {
        if(!currentServerLocked(generation)||gattServer==null)
            return;
        final ServerEndpoint endpoint=serverServiceQueue.peek();
        if(endpoint==null) {
            serverServiceReady=allCurrentServerServicesReadyLocked();
            int active=0;
            for(ServerEndpoint candidate:serverEndpoints.values())
                if(currentServerEndpointLocked(candidate)&&candidate.ready)
                    ++active;
            Log.i(LOG_ID,"all current token-addressed BLE mirror GATT services are ready: "+
                    active+" active, "+serverEndpoints.size()+" stable handle slot(s)"+
                    (legacySingleGattLocked()?" (Android 5 serialized radio)":""));
            startAdvertisingLocked();
            return;
        }
        if(!gattServer.addService(endpoint.service)) {
            dropFailedServerEndpointLocked(endpoint,"Could not add BLE mirror service "+
                    endpoint.service.getUuid());
            addNextServerServiceLocked(generation);
        }
    }

    private static void setServerStatusesLocked(String status) {
        for(Link link:linksByIndex.values())
            if(offersGattServerLocked(link)&&link.configError==null&&!isReadyLocked(link))
                link.status=status;
    }

    private static List<Link> allAdvertisableServerLinksLocked() {
        final ArrayList<Link> result=new ArrayList<>();
        final Connection legacyOccupied=legacySingleGattLocked()?
                legacyOccupiedConnectionLocked():null;
        // API 21/22 must not start advertising while its scarce *central* slot is
        // occupied by an outgoing GATT client.  A ready incoming/server connection
        // is different: Android can keep that peripheral connection alive while the
        // same GATT server advertises another already-registered token service.
        // Returning an empty list here starved J5 -> S5 indefinitely whenever the
        // existing XA1 -> J5 server session was alive.
        if(legacyOccupied!=null&&(!legacyOccupied.serverSide||!legacyOccupied.ready))
            return result;
        final Link leased=legacyLeasedLinkLocked();
        for(Link link:linksByIndex.values()) {
            if(leased!=null&&link!=leased)
                continue;
            if(legacyLinkSuppressedLocked(link)||
                    (legacySingleGattLocked()&&leased==null&&
                            !legacyNeedsDiscoverySlotLocked(link)))
                continue;
            if(offersGattServerLocked(link)&&link.configError==null&&
                    operationBlockerLocked(link)==null&&!isReadyLocked(link)) {
                if(link.role==ROLE_CLIENT&&link.reverseGattRole&&!canReverseAdvertiseLocked())
                    continue;
                result.add(link);
            }
        }
        return result;
    }

    private static int advertisementPriorityLocked(Link link) {
        if(link==null)
            return -1;
        int priority=0;
        // Direct Bluetooth has no other carrier and must not disappear behind
        // an Automatic standby advertisement.
        if(link.transport==TRANSPORT_BLUETOOTH)
            priority+=4;
        // A native sender that is actively asking for the fallback should be
        // promoted above idle Automatic standby links.
        if(discoveryBoostedLocked(link)||link.automaticFallbackRequested)
            priority+=2;
        // A client that learned the reversed server role is waiting for its
        // peer to initiate; give it a slight preference over an otherwise
        // equivalent preferred-server link.
        if(link.role==ROLE_CLIENT&&link.reverseGattRole)
            priority+=1;
        return priority;
    }

    /**
     * Return every server link that still needs discovery. Higher-priority
     * links are placed first, but lower-priority links remain in the rotation.
     * The old implementation returned only boosted links, which allowed one
     * disconnected mirror to starve every other identity on the shared legacy
     * advertiser.
     */
    private static List<Link> advertisableServerLinksLocked() {
        final List<Link> all=allAdvertisableServerLinksLocked();
        if(all.size()<2)
            return all;
        final ArrayList<Link> ordered=new ArrayList<>(all.size());
        // Max priority is 7 with the current flags. Avoid List.sort() so this
        // remains unproblematic on the API-21 build.
        for(int priority=7;priority>=0;--priority)
            for(Link link:all)
                if(advertisementPriorityLocked(link)==priority)
                    ordered.add(link);
        return ordered;
    }

    private static UUID tokenUuid(UUID base,byte[] token) {
        if(token==null||token.length<4)
            return base;
        final long token32=((long)(token[0]&0xff)<<24)|
                ((long)(token[1]&0xff)<<16)|
                ((long)(token[2]&0xff)<<8)|(long)(token[3]&0xff);
        return new UUID(base.getMostSignificantBits(),
                (base.getLeastSignificantBits()&0xffffffff00000000L)|token32);
    }

    private static UUID discoveryUuid(byte[] token) {
        return tokenUuid(SERVICE_UUID,token);
    }

    private static UUID android5DiscoveryUuid(byte[] token) {
        return tokenUuid(ANDROID5_SERVICE_UUID,token);
    }

    private static UUID rxUuid(byte[] token) {
        return tokenUuid(RX_UUID,token);
    }

    private static UUID txUuid(byte[] token) {
        return tokenUuid(TX_UUID,token);
    }

    private static UUID legacyDiscoveryUuid(byte[] token) {
        return tokenUuid(LEGACY_SERVICE_UUID,token);
    }

    private static UUID legacyRxUuid(byte[] token) {
        return tokenUuid(LEGACY_RX_UUID,token);
    }

    private static UUID legacyTxUuid(byte[] token) {
        return tokenUuid(LEGACY_TX_UUID,token);
    }

    private static ParcelUuid discoveryParcelUuid(byte[] token) {
        return new ParcelUuid(discoveryUuid(token));
    }

    private static ParcelUuid android5DiscoveryParcelUuid(byte[] token) {
        return new ParcelUuid(android5DiscoveryUuid(token));
    }

    private static boolean hasUuid(ScanRecord record,UUID expected) {
        if(record==null||expected==null)
            return false;
        final List<ParcelUuid> uuids=record.getServiceUuids();
        if(uuids!=null)
            for(ParcelUuid uuid:uuids)
                if(uuid!=null&&expected.equals(uuid.getUuid()))
                    return true;

        // Some vendor ScanRecord implementations have incomplete parsing. The
        // bytes are still available, so also recognize AD types 0x06/0x07 here.
        final byte[] raw=record.getBytes();
        if(raw==null)
            return false;
        int pos=0;
        while(pos<raw.length) {
            final int length=raw[pos++]&0xff;
            if(length==0)
                break;
            if(pos+length>raw.length)
                break;
            final int type=raw[pos++]&0xff;
            final int dataLength=length-1;
            if(type==AD_TYPE_SERVICE_UUID_128_PARTIAL||
                    type==AD_TYPE_SERVICE_UUID_128_COMPLETE) {
                for(int off=0;off+16<=dataLength;off+=16) {
                    long lsb=0,msb=0;
                    for(int i=0;i<8;i++)
                        lsb|=(long)(raw[pos+off+i]&0xff)<<(8*i);
                    for(int i=0;i<8;i++)
                        msb|=(long)(raw[pos+off+8+i]&0xff)<<(8*i);
                    if(lsb==expected.getLeastSignificantBits()&&
                            msb==expected.getMostSignificantBits())
                        return true;
                }
            }
            pos+=dataLength;
        }
        return false;
    }

    private static boolean hasDiscoveryUuid(ScanRecord record,byte[] token) {
        return hasUuid(record,discoveryUuid(token));
    }

    private static boolean hasAndroid5DiscoveryUuid(ScanRecord record,byte[] token) {
        return hasUuid(record,android5DiscoveryUuid(token));
    }

    private static boolean hasLegacyDiscoveryUuid(ScanRecord record,byte[] token) {
        return hasUuid(record,legacyDiscoveryUuid(token));
    }

    private static void scheduleAdvertisingRestartLocked(long delayMillis) {
        if(advertiseRestartScheduled)
            return;
        advertiseRestartScheduled=true;
        final long configuration=configurationGeneration;
        handler.postDelayed(() -> {
            synchronized(lock) {
                advertiseRestartScheduled=false;
                if(configuration!=configurationGeneration)
                    return;
                startAdvertisingLocked();
            }
        },Math.max(1L,delayMillis));
    }

    /** Build one compact IPv4+port hint from the native v1 address payload. */
    private static byte[] automaticIpv4EndpointHintLocked(Link link) {
        if(link==null||link.transport!=TRANSPORT_AUTOMATIC||link.discoveryToken==null||
                link.discoveryToken.length<BleMirrorSecurity.DISCOVERY_TOKEN_BYTES)
            return null;
        final byte[] addresses;
        try {
            addresses=Natives.getMirrorAddresses(link.hostIndex);
        }
        catch(Throwable error) {
            Log.stack(LOG_ID,"get BLE advertisement endpoint hint",error);
            return null;
        }
        if(addresses==null||addresses.length<2||(addresses[0]&0xff)!=1)
            return null;
        final int nr=addresses[1]&0xff;
        if(nr<=0||addresses.length!=2+nr*SOCKADDR_IN6_BYTES)
            return null;
        final boolean little=ByteOrder.nativeOrder()==ByteOrder.LITTLE_ENDIAN;
        for(int index=0;index<nr;++index) {
            final int off=2+index*SOCKADDR_IN6_BYTES;
            final int family=little?
                    (addresses[off]&0xff)|((addresses[off+1]&0xff)<<8):
                    ((addresses[off]&0xff)<<8)|(addresses[off+1]&0xff);
            if(family!=10/* AF_INET6 */||(addresses[off+2]==0&&addresses[off+3]==0))
                continue;
            boolean mapped=true;
            for(int pos=0;pos<10;++pos)
                if(addresses[off+8+pos]!=0) { mapped=false; break; }
            if(!mapped||(addresses[off+18]&0xff)!=0xff||(addresses[off+19]&0xff)!=0xff)
                continue;
            final byte[] hint=new byte[ENDPOINT_HINT_BYTES];
            hint[0]=(byte)ENDPOINT_HINT_VERSION;
            System.arraycopy(addresses,off+20,hint,1,4);
            hint[5]=addresses[off+2];
            hint[6]=addresses[off+3];
            try {
                final byte[] tag=BleMirrorSecurity.endpointHintTag(link.masterKey,hint,7);
                System.arraycopy(tag,0,hint,7,4);
                return hint;
            }
            catch(GeneralSecurityException error) {
                Log.stack(LOG_ID,"authenticate BLE advertisement endpoint hint",error);
                return null;
            }
        }
        return null;
    }

    /**
     * Accept a compact endpoint hint only when its password-derived HMAC matches.
     * Expand it back into the native v1 sockaddr_in6 wire format and wake TCP/IP.
     */
    private static boolean acceptAutomaticIpv4EndpointHintLocked(Link link,ScanRecord record) {
        if(link==null||record==null||link.transport!=TRANSPORT_AUTOMATIC||
                link.discoveryToken==null||link.discoveryToken.length<8)
            return false;
        byte[] hint=record.getServiceData(discoveryParcelUuid(link.discoveryToken));
        if(hint==null)
            hint=record.getServiceData(android5DiscoveryParcelUuid(link.discoveryToken));
        if(hint==null||hint.length!=ENDPOINT_HINT_BYTES||
                (hint[0]&0xff)!=ENDPOINT_HINT_VERSION)
            return false;
        try {
            final byte[] expected=BleMirrorSecurity.endpointHintTag(link.masterKey,hint,7);
            if(!MessageDigest.isEqual(expected,Arrays.copyOfRange(hint,7,11)))
                return false;
        }
        catch(GeneralSecurityException error) {
            Log.stack(LOG_ID,"verify BLE advertisement endpoint hint",error);
            return false;
        }
        if(hint[5]==0&&hint[6]==0)
            return false;
        final byte[] addresses=new byte[2+SOCKADDR_IN6_BYTES];
        addresses[0]=1;
        addresses[1]=1;
        final int off=2;
        if(ByteOrder.nativeOrder()==ByteOrder.LITTLE_ENDIAN)
            addresses[off]=10; // AF_INET6
        else
            addresses[off+1]=10;
        addresses[off+2]=hint[5];
        addresses[off+3]=hint[6];
        addresses[off+18]=(byte)0xff;
        addresses[off+19]=(byte)0xff;
        System.arraycopy(hint,1,addresses,off+20,4);
        final boolean changed;
        try { changed=Natives.setMirrorAddresses(link.hostIndex,addresses); }
        catch(Throwable error) {
            Log.stack(LOG_ID,"apply BLE advertisement endpoint hint",error);
            return false;
        }
        if(!changed)
            return false;
        link.newTcpCandidates=true;
        link.status="Learned TCP/IP endpoint from Bluetooth advertisement "+link.label;
        Log.i(LOG_ID,"accepted password-bound BLE IPv4 endpoint hint: label="+link.label+
                "; waking TCP/IP while continuing Bluetooth GATT establishment");
        handler.post(Applic::wakemirrors);
        return true;
    }

    @SuppressLint("MissingPermission")
    private static void startAdvertisingLocked() {
        if(bluetoothRestartSettleRemainingLocked()>0L)
            return;
        final long incomingHandshakeHold=serverHandshakeHoldRemainingLocked();
        if(advertising) {
            if(incomingHandshakeHold>0L) {
                scheduleAdvertisementRotationLocked(advertiseGeneration,incomingHandshakeHold+50L);
                return;
            }
            final List<Link> candidates=advertisableServerLinksLocked();
            if(candidates.isEmpty()) {
                stopAdvertisingLocked();
                return;
            }
            final Link preferred=candidates.get(0);
            // The set of unready server links can grow while an advertisement
            // is already running (for example when another peer disconnects).
            // If the current identity is no longer eligible, or a newly
            // disconnected Direct/reversed link has strictly higher priority,
            // switch immediately instead of waiting for an 8-second slice.
            // This is the M5 multi-peer failure: it kept advertising the idle
            // Automatic "comp" link after realmenew disconnected, so Realme's
            // reverse-role scans could never see realmenew again.
            if(advertisedLink==null||!candidates.contains(advertisedLink)||
                    advertisementPriorityLocked(preferred)>
                            advertisementPriorityLocked(advertisedLink)) {
                Log.i(LOG_ID,"preempting Bluetooth advertisement: previous="+
                        (advertisedLink==null?"none":advertisedLink.label)+
                        " next="+preferred.label+" previousPriority="+
                        advertisementPriorityLocked(advertisedLink)+" nextPriority="+
                        advertisementPriorityLocked(preferred));
                // startAdvertisingLocked() normally advances advertiseCursor for
                // fair rotation. Candidate membership changed underneath that
                // cursor, however, so force this preemption to start with the
                // newly selected highest-priority identity. Without this reset a
                // previous one-link cursor value of 1 could select the old
                // Automatic advertisement again immediately after preemption.
                advertiseCursor=0;
                stopAdvertisingLocked();
                startAdvertisingLocked();
                return;
            }
            // Arm rotation here as well as in onStartSuccess(), otherwise a
            // newly unready equal-priority peer can remain invisible forever.
            scheduleAdvertisementRotationLocked(advertiseGeneration);
            scheduleReverseProbeLocked(null);
            return;
        }
        final long advertiseDelay=advertiseRestartNotBeforeMillis-monotonicMillis();
        if(advertiseDelay>0) {
            scheduleAdvertisingRestartLocked(advertiseDelay);
            return;
        }
        if(gattServer==null||!serverServiceReady||adapter==null||incomingHandshakeHold>0L||
                hasPendingClientConnectionLocked()||
                (legacySingleGattLocked()&&reverseProbeActiveLocked()))
            return;
        final List<Link> candidates=advertisableServerLinksLocked();
        if(candidates.isEmpty())
            return;
        if(advertiseCursor>=candidates.size())
            advertiseCursor=0;
        final Link link=candidates.get(advertiseCursor++);
        advertiser=adapter.getBluetoothLeAdvertiser();
        if(advertiser==null) {
            setServerStatusesLocked("BLE advertising is not supported by this device");
            return;
        }
        final boolean reversedServerRendezvous=link.role==ROLE_CLIENT&&link.reverseGattRole;
        final int advertiseMode=link.transport==TRANSPORT_AUTOMATIC&&
                !discoveryBoostedLocked(link)&&!reversedServerRendezvous?
                AdvertiseSettings.ADVERTISE_MODE_BALANCED:
                AdvertiseSettings.ADVERTISE_MODE_LOW_LATENCY;
        final AdvertiseSettings settings=new AdvertiseSettings.Builder()
                .setAdvertiseMode(advertiseMode)
                .setConnectable(true).setTimeout(0)
                .setTxPowerLevel(AdvertiseSettings.ADVERTISE_TX_POWER_MEDIUM).build();
        // API 21-22 also advertises the token-addressed v15 UUID.  Keep d30 only
        // as receive-side compatibility with the short-lived v5/v6 builds.
        final ParcelUuid discoveryUuid=discoveryParcelUuid(link.discoveryToken);
        final AdvertiseData data=new AdvertiseData.Builder()
                .addServiceUuid(discoveryUuid)
                .setIncludeDeviceName(false).build();
        final byte[] endpointHint=automaticIpv4EndpointHintLocked(link);
        final AdvertiseData scanResponse=endpointHint==null?null:new AdvertiseData.Builder()
                .addServiceData(discoveryUuid,endpointHint)
                .setIncludeDeviceName(false).build();
        final long generation=++advertiseGeneration;
        final AdvertiseCallback callback=new AdvertiseCallback() {
            @Override public void onStartSuccess(AdvertiseSettings ignored) {
                synchronized(lock) {
                    if(generation!=advertiseGeneration||advertiseCallback!=this)
                        return;
                    link.status=waitingStatus(link);
                    link.advertiseResourceFailures=0;
                    Log.i(LOG_ID,"advertising started: index="+link.hostIndex+" label="+link.label+
                            " token="+tokenFingerprint(link.discoveryToken)+
                            " physicalRole="+(link.role==ROLE_CLIENT?"reversed-server":"preferred-server")+
                            " boosted="+discoveryBoostedLocked(link)+
                            " rotating="+(allAdvertisableServerLinksLocked().size()>1)+
                            " crossProcessYield=true");
                    if(legacySingleGattLocked()&&urgentDiscoveryLocked(link)) {
                        legacyAdvertiseHoldUntilMillis=monotonicMillis()+LEGACY_SLOT_LEASE_MILLIS;
                        // A three-second reverse probe may have been armed before
                        // this advertisement actually became connectable. Cancel
                        // that stale schedule and give the peer one uninterrupted
                        // rendezvous window instead.
                        if(reverseProbeScheduled) {
                            reverseProbeScheduled=false;
                            ++reverseProbeGeneration;
                        }
                        Log.i(LOG_ID,"holding Android 5 BLE advertisement for active peer: label="+
                                link.label+" hold="+LEGACY_SLOT_LEASE_MILLIS+" ms");
                    }
                    else
                        legacyAdvertiseHoldUntilMillis=0L;
                    scheduleAdvertisementRotationLocked(generation);
                    scheduleReverseProbeLocked(link);
                }
            }

            @Override public void onStartFailure(int errorCode) {
                synchronized(lock) {
                    if(generation!=advertiseGeneration||advertiseCallback!=this)
                        return;
                    advertising=false;
                    advertisedLink=null;
                    advertiseCallback=null;
                    advertiser=null;
                    link.status="BLE advertising failed: "+errorCode;
                    final boolean resourceFailure=errorCode==
                            AdvertiseCallback.ADVERTISE_FAILED_TOO_MANY_ADVERTISERS;
                    final long retry=resourceFailure?
                            ADVERTISE_RESOURCE_RETRY_MIN_MILLIS+
                                    random.nextInt(ADVERTISE_RESOURCE_RETRY_JITTER_MILLIS+1):
                            RETRY_MILLIS;
                    // Enforce the advertised retry delay for every start
                    // failure. Otherwise unrelated reconciliation can immediately
                    // retry Android-5 INTERNAL_ERROR (4) in a tight loop.
                    advertiseRestartNotBeforeMillis=Math.max(advertiseRestartNotBeforeMillis,
                            monotonicMillis()+retry);
                    if(resourceFailure)
                        ++link.advertiseResourceFailures;
                    else
                        link.advertiseResourceFailures=0;
                    final boolean retryAsClient=link.role==ROLE_CLIENT&&link.reverseGattRole;
                    if(retryAsClient) {
                        // This is an unproven temporary server experiment. A
                        // persisted server preference is represented by
                        // role==ROLE_SERVER and is not silently discarded.
                        link.reverseGattRole=false;
                        link.transientClientFailures=0;
                        final long now=monotonicMillis();
                        link.clientRetryNotBeforeMillis=Math.max(link.clientRetryNotBeforeMillis,
                                now+1000L);
                        // This temporary reverse attempt has been consumed.  Give the persisted
                        // client direction another full protected trial and, while Automatic
                        // fallback remains requested, arm another no-peer recovery attempt.
                        link.oppositeRoleNotBeforeMillis=now+PREFERRED_ROLE_TRY_MILLIS;
                        if(link.automaticFallbackRequested)
                            schedulePreferredClientNoPeerRecoveryLocked(link);
                        link.status="Reversed Bluetooth role could not advertise; retrying client "+
                                link.label;
                        Log.w(LOG_ID,"temporary reversed GATT role failed to advertise; returning to client: label="+
                                link.label+" error="+errorCode);
                        link.advertiseResourceFailures=0;
                    }
                    Log.e(LOG_ID,"advertising failed: label="+link.label+" error="+errorCode+
                            " retryMillis="+retry+
                            (resourceFailure?" crossProcessAdvertiserContention=true":""));
                    final long config=configurationGeneration;
                    handler.postDelayed(() -> retryOperations(config),
                            retryAsClient?1000L:retry);
                    if(retryAsClient)
                        reconcileOperationsLocked();
                }
            }
        };
        try {
            advertisedLink=link;
            advertiseCallback=callback;
            if(scanResponse==null)
                advertiser.startAdvertising(settings,data,callback);
            else
                advertiser.startAdvertising(settings,data,scanResponse,callback);
            advertising=true;
            Log.i(LOG_ID,"startAdvertising: index="+link.hostIndex+" label="+link.label+
                    " token="+tokenFingerprint(link.discoveryToken)+
                    " discoveryUuid="+discoveryUuid.getUuid()+
                    " endpointHint="+(endpointHint==null?"none":"ipv4")+
                    " physicalRole="+(link.role==ROLE_CLIENT?"reversed-server":"preferred-server")+
                    " mode="+(link.transport==TRANSPORT_AUTOMATIC&&!discoveryBoostedLocked(link)&&
                            !reversedServerRendezvous?"balanced standby":"low-latency rendezvous"));
        }
        catch(Throwable error) {
            Log.stack(LOG_ID,"startAdvertising",error);
            advertising=false;
            advertisedLink=null;
            advertiseCallback=null;
            link.status="Could not advertise the BLE mirror service";
        }
    }

    /**
     * Give the normally-peripheral peer a short chance to become the central.
     * This is a fallback for controllers such as the Huawei M5 trace where the
     * local direct GATT connect is accepted by HCI but never finds the already
     * visible connectable advertiser. Advertising is stopped for the whole
     * probe, so old controllers are never asked to scan and advertise at once.
     */
    private static boolean hasReverseProbeCandidateLocked() {
        if(!canReverseScanLocked())
            return false;
        for(Link link:linksByIndex.values())
            if(reverseProbeCandidateLocked(link)&&operationBlockerLocked(link)==null)
                return true;
        return false;
    }

    /**
     * If a persisted server role is still inside its first-choice interval,
     * remember when the earliest one becomes eligible for a reverse probe.
     */
    private static long reverseProbeEligibilityDelayLocked() {
        if(!canReverseScanLocked())
            return -1L;
        final long now=monotonicMillis();
        long best=Long.MAX_VALUE;
        for(Link link:linksByIndex.values()) {
            if(link==null||link.role!=ROLE_SERVER||link.connection!=null||isReadyLocked(link)||
                    link.configError!=null||operationBlockerLocked(link)!=null)
                continue;
            final boolean eligible=link.directionProven?link.resourceReverseClientRequested:
                    (link.transport==TRANSPORT_BLUETOOTH||link.transport==TRANSPORT_AUTOMATIC||
                            discoveryBoostedLocked(link));
            if(!eligible)
                continue;
            if(link.oppositeRoleNotBeforeMillis>now)
                best=Math.min(best,link.oppositeRoleNotBeforeMillis-now);
        }
        return best==Long.MAX_VALUE?-1L:Math.max(50L,best);
    }

    private static boolean hasUrgentReverseProbeCandidateLocked() {
        if(!canReverseScanLocked())
            return false;
        for(Link link:linksByIndex.values()) {
            if(!reverseProbeCandidateLocked(link)||operationBlockerLocked(link)!=null)
                continue;
            if(link.transport==TRANSPORT_BLUETOOTH||link.automaticFallbackRequested||
                    discoveryBoostedLocked(link))
                return true;
        }
        return false;
    }

    private static boolean hasResourceReverseProbeCandidateLocked() {
        if(!canReverseScanLocked())
            return false;
        for(Link link:linksByIndex.values())
            if(link!=null&&link.resourceReverseClientRequested&&
                    reverseProbeCandidateLocked(link)&&operationBlockerLocked(link)==null)
                return true;
        return false;
    }

    /**
     * Schedule one process-wide reverse-role probe. Advertisement identities may
     * rotate many times before this fires; those rotations deliberately do not
     * alter reverseProbeGeneration or the scheduled timer.
     */
    private static boolean hasAuthenticatedBleConnectionLocked() {
        for(Link link:linksByIndex.values())
            if(link!=null&&isReadyLocked(link))
                return true;
        return false;
    }

    private static void scheduleReverseProbeLocked(Link ignored) {
        // On modern/vendor stacks, repeatedly scanning for an opposite-role
        // Automatic peer while other authenticated BLE links are alive can
        // destabilize the controller's legacy advertiser. In the failing traces
        // this was followed by BluetoothLeAdvertiser state anomalies and a full
        // Bluetooth-service restart, which destroys every healthy mirror link.
        // Keep preferred-role advertising/scanning running, but do not add the
        // optional process-wide reverse scan while a healthy link exists. The
        // authenticated resource-role request used by the fragile XA1 handoff is
        // exempt because that is an explicit peer-directed role change.
        if(Build.VERSION.SDK_INT>FRAGILE_MULTI_CLIENT_GATT_MAX_SDK&&
                hasAuthenticatedBleConnectionLocked()&&
                !hasResourceReverseProbeCandidateLocked()) {
            final long now=monotonicMillis();
            if(now-lastModernReverseProbeSuppressedLogMillis>=DISCOVERY_LOG_MILLIS) {
                lastModernReverseProbeSuppressedLogMillis=now;
                Log.i(LOG_ID,"suppressing optional modern reverse-role scan probe while authenticated BLE links are healthy");
            }
            return;
        }
        // API 21-22 has one reliable BLE mirror radio/GATT slot.  Do not pause
        // or repurpose that radio for another peer while a live/authenticating
        // session owns it.  v5 kept running this process-wide probe every 12 s,
        // which repeatedly disturbed J5 <-> XA1 while S5 was still disconnected.
        if(legacySingleGattLocked()&&legacyOccupiedConnectionLocked()!=null)
            return;
        if(reverseProbeScheduled||reverseProbeActiveLocked()||
                hasPendingClientConnectionLocked())
            return;
        if(!hasReverseProbeCandidateLocked()) {
            final long eligibilityDelay=reverseProbeEligibilityDelayLocked();
            if(eligibilityDelay<0L)
                return;
            reverseProbeScheduled=true;
            final long waitingGeneration=reverseProbeGeneration;
            handler.postDelayed(() -> {
                synchronized(lock) {
                    if(waitingGeneration!=reverseProbeGeneration)
                        return;
                    reverseProbeScheduled=false;
                    scheduleReverseProbeLocked(null);
                }
            },eligibilityDelay);
            Log.i(LOG_ID,"preferred BLE side retained for another "+eligibilityDelay+
                    " ms before opposite-role probing is allowed");
            return;
        }
        reverseProbeScheduled=true;
        final long generation=reverseProbeGeneration;
        // The 3-second urgent cadence exists only for the API-21/22 single-slot
        // handoff.  Applying it process-wide on newer Android caused phones with
        // several mirror rows (notably Realme/M5) to stop advertising and scan
        // every three seconds, starving otherwise working reversed-server peers.
        // Newer stacks keep the stable 12-second serialized reverse probe.
        final boolean urgent=hasUrgentReverseProbeCandidateLocked();
        final boolean resourceReverse=hasResourceReverseProbeCandidateLocked();
        final boolean legacyUrgent=legacySingleGattLocked()&&urgent;
        final boolean modernUrgent=!legacySingleGattLocked()&&urgent;
        final long probeDelay;
        if(resourceReverse&&!legacySingleGattLocked())
            probeDelay=RESOURCE_REVERSE_PROBE_DELAY_MILLIS;
        else if(legacyUrgent)
            probeDelay=ACTIVE_REVERSE_PROBE_DELAY_MILLIS;
        else if(modernUrgent) {
            // A fixed 12 s delay plus the fixed 2 s probe gives every modern
            // device the same 14-second period. The Realme <-> Boox trace showed
            // this can remain phase-locked for more than twenty minutes: each
            // Realme scan started just as Boox rotated away from the boox UUID.
            // Jitter only active Direct/fallback discovery so idle multi-mirror
            // phones keep their existing low-power/stable cadence.
            final int span=REVERSE_PROBE_JITTER_MILLIS*2+1;
            final long jitter=random.nextInt(span)-REVERSE_PROBE_JITTER_MILLIS;
            probeDelay=REVERSE_PROBE_DELAY_MILLIS+jitter;
        }
        else
            probeDelay=REVERSE_PROBE_DELAY_MILLIS;
        final long rendezvousHold=legacyAdvertiseHoldRemainingLocked();
        final long delay=Math.max(probeDelay,rendezvousHold);
        handler.postDelayed(() -> beginReverseProbe(generation),delay);
        Log.i(LOG_ID,"scheduled shared reverse-role scan probe in "+delay+" ms"+
                (rendezvousHold>probeDelay?" (holding active Android 5 rendezvous)":
                        resourceReverse?" (authenticated resource-role request)":
                        legacyUrgent?" (active BLE demand)":
                                modernUrgent?" (active BLE demand; jittered to avoid peer phase lock)":""));
    }

    private static void beginReverseProbe(long generation) {
        synchronized(lock) {
            if(generation!=reverseProbeGeneration)
                return;
            final long incomingHold=serverHandshakeHoldRemainingLocked();
            if(incomingHold>0L) {
                Log.i(LOG_ID,"deferring reverse-role scan during incoming BLE handshake for "+
                        incomingHold+" ms");
                handler.postDelayed(() -> beginReverseProbe(generation),incomingHold+50L);
                return;
            }
            reverseProbeScheduled=false;
            if((legacySingleGattLocked()&&legacyOccupiedConnectionLocked()!=null)||
                    reverseProbeActiveLocked()||hasPendingClientConnectionLocked()||
                    !hasReverseProbeCandidateLocked())
                return;
            reverseProbeUntilMillis=monotonicMillis()+REVERSE_PROBE_MILLIS;
            Log.i(LOG_ID,"starting serialized reverse-role scan probe for "+
                    REVERSE_PROBE_MILLIS+" ms across all eligible server links"+
                    (legacySingleGattLocked()?" (advertising paused)":" (advertising kept active)"));
            // API 21/22 needs exclusive radio roles. Modern Android supports
            // simultaneous LE advertising + scanning; keeping the advertiser
            // alive avoids vendor stacks that report onStartSuccess after a
            // restart but silently stop putting packets on air.
            if(legacySingleGattLocked())
                stopAdvertisingLocked();
            stopScanLocked();
            startScanLocked();
            handler.postDelayed(() -> endReverseProbe(generation),REVERSE_PROBE_MILLIS);
        }
    }

    private static void endReverseProbe(long generation) {
        synchronized(lock) {
            if(generation!=reverseProbeGeneration)
                return;
            reverseProbeUntilMillis=0;
            if(hasPendingClientConnectionLocked())
                return;
            Log.i(LOG_ID,"serialized reverse-role scan probe finished");
            stopScanLocked();
            refreshScanLocked();
            if(serverServiceReady&&!advertising)
                startAdvertisingLocked();
            // If one or more reversed peers are still missing, keep probing at
            // the normal cadence. startAdvertisingLocked() will also call this
            // after onStartSuccess(), but the scheduled flag prevents duplicates.
            scheduleReverseProbeLocked(null);
        }
    }

    private static long advertisementSliceMillisLocked(Link link) {
        final long base=link!=null&&link.role==ROLE_CLIENT&&link.reverseGattRole?
                REVERSED_SERVER_ADVERTISE_SLICE_MILLIS:ADVERTISE_SLICE_MILLIS;
        // Different Juggluco processes otherwise tend to keep identical fixed
        // 12/18-second phases. Jitter the on-air slice as well as the yield.
        return base+random.nextInt(ADVERTISE_SLICE_JITTER_MILLIS+1);
    }

    private static long advertisementYieldMillisLocked(Link link) {
        long delay=ADVERTISE_CROSS_PROCESS_YIELD_MIN_MILLIS+
                random.nextInt(ADVERTISE_CROSS_PROCESS_YIELD_JITTER_MILLIS+1);
        // Direct Bluetooth has no TCP carrier, so keep its voluntary gap shorter
        // while still giving another process a real chance to acquire the slot.
        if(link!=null&&link.transport==TRANSPORT_BLUETOOTH)
            delay=Math.max(ADVERTISE_RESTART_SETTLE_MILLIS+200L,delay/2L);
        return delay;
    }

    private static void scheduleAdvertisementRotationLocked(long generation) {
        scheduleAdvertisementRotationLocked(generation,advertisementSliceMillisLocked(advertisedLink));
    }

    private static void scheduleAdvertisementRotationLocked(long generation,long delayMillis) {
        // Rotate multiple identities inside this process, and also yield even
        // when there is only one unconnected identity.  The latter is required
        // for fairness between separate Juggluco processes which share a scarce
        // system advertiser but cannot share this in-memory rotation state.
        if(allAdvertisableServerLinksLocked().isEmpty()||
                advertiseRotationScheduledGeneration==generation)
            return;
        advertiseRotationScheduledGeneration=generation;
        handler.postDelayed(() -> {
            synchronized(lock) {
                if(advertiseRotationScheduledGeneration==generation)
                    advertiseRotationScheduledGeneration=-1;
                if(generation!=advertiseGeneration||!advertising)
                    return;
                final long incomingHold=serverHandshakeHoldRemainingLocked();
                if(incomingHold>0L) {
                    Log.i(LOG_ID,"deferring advertisement rotation during incoming BLE handshake: label="+
                            (advertisedLink==null?"none":advertisedLink.label)+
                            " remaining="+incomingHold+" ms");
                    scheduleAdvertisementRotationLocked(generation,incomingHold+50L);
                    return;
                }
                if(reverseProbeActiveLocked()) {
                    final long remaining=Math.max(50L,reverseProbeUntilMillis-monotonicMillis()+50L);
                    Log.i(LOG_ID,"deferring advertisement rotation until reverse-role scan finishes: label="+
                            (advertisedLink==null?"none":advertisedLink.label)+
                            " remaining="+remaining+" ms");
                    scheduleAdvertisementRotationLocked(generation,remaining);
                    return;
                }
                final long hold=legacyAdvertiseHoldRemainingLocked();
                if(hold>0L) {
                    Log.i(LOG_ID,"deferring Android 5 advertisement rotation for active peer: label="+
                            (advertisedLink==null?"none":advertisedLink.label)+
                            " remaining="+hold+" ms");
                    scheduleAdvertisementRotationLocked(generation,hold);
                    return;
                }
                final List<Link> candidates=advertisableServerLinksLocked();
                if(candidates.isEmpty())
                    return;
                final Link previous=advertisedLink;
                final long yield=advertisementYieldMillisLocked(previous);
                Log.i(LOG_ID,(candidates.size()>1?
                        "rotating Bluetooth advertisement after rendezvous slice":
                        "yielding sole Bluetooth advertisement for cross-process fairness")+
                        ": previous="+(previous==null?"none":previous.label)+
                        " yield="+yield+" ms candidates="+candidates.size());
                // stopAdvertisingLocked() itself enforces the short controller
                // settle. Extend that gate to a real cross-process yield window.
                advertiseRestartNotBeforeMillis=Math.max(advertiseRestartNotBeforeMillis,
                        monotonicMillis()+yield);
                stopAdvertisingLocked();
                startAdvertisingLocked();
            }
        },Math.max(1L,delayMillis));
    }

    @SuppressLint("MissingPermission")
    private static void stopAdvertisingLocked() {
        ++advertiseGeneration;
        advertiseRotationScheduledGeneration=-1;
        final boolean hadAdvertisement=advertising&&advertiser!=null&&advertiseCallback!=null;
        if(hadAdvertisement) {
            try { advertiser.stopAdvertising(advertiseCallback); }
            catch(Throwable error) { Log.stack(LOG_ID,"stopAdvertising",error); }
            advertiseRestartNotBeforeMillis=Math.max(advertiseRestartNotBeforeMillis,
                    monotonicMillis()+ADVERTISE_RESTART_SETTLE_MILLIS);
        }
        advertising=false;
        advertisedLink=null;
        advertiseCallback=null;
        legacyAdvertiseHoldUntilMillis=0L;
        // Bluetooth process/controller restarts invalidate the framework proxy.
        // Always obtain a fresh advertiser before the next start.
        advertiser=null;
    }

    @SuppressLint("MissingPermission")
    private static void startScanLocked() {
        if(scanRunning||adapter==null)
            return;
        final List<Link> candidates=scannableClientLinksLocked();
        final ArrayList<ScanFilter> filters=new ArrayList<>();
        boolean carrierActivePending=reverseProbeActiveLocked();
        for(Link link:candidates) {
            Log.i(LOG_ID,"BLE scan identity: index="+link.hostIndex+" label="+link.label+
                    " token="+tokenFingerprint(link.discoveryToken)+
                    " discoveryUuid="+discoveryUuid(link.discoveryToken)+
                    " android5DiscoveryUuid="+android5DiscoveryUuid(link.discoveryToken)+
                    " boosted="+discoveryBoostedLocked(link));
            if(link.transport==TRANSPORT_BLUETOOTH||discoveryBoostedLocked(link))
                carrierActivePending=true;
        }
        if(candidates.isEmpty())
            return;
        final long now=monotonicMillis();
        final boolean standbySoftwareProbe=!carrierActivePending&&
                scanSoftwareProbeUntilMillis>now;
        // Current Juggluco versions advertise a private 128-bit discovery UUID
        // containing the 4-byte password-derived token. Android 5.x parses 128-bit
        // service UUID lists correctly. Active discovery remains unfiltered so it
        // can also recognize the immediately preceding Service Data formats.
        // Automatic standby can use an exact service-UUID filter.
        final boolean softwareMatching=carrierActivePending||standbySoftwareProbe;
        final boolean lowLatencyScan=carrierActivePending||standbySoftwareProbe;
        // Several Android vendors register an exact 128-bit service-data
        // filter successfully but never deliver matching results. Active
        // discovery therefore scans without an offloaded hardware filter and
        // performs the same password-derived token comparison in foundDevice.
        // Balanced Automatic standby retains exact filters for battery use.
        if(!softwareMatching)
            for(Link link:candidates)
                filters.add(new ScanFilter.Builder().setServiceUuid(
                        discoveryParcelUuid(link.discoveryToken),CURRENT_DISCOVERY_MASK).build());
        scanner=adapter.getBluetoothLeScanner();
        if(scanner==null) {
            setClientStatusesLocked("BLE scanning is not available");
            return;
        }
        final int scanMode=lowLatencyScan?ScanSettings.SCAN_MODE_LOW_LATENCY:
                ScanSettings.SCAN_MODE_BALANCED;
        final ScanSettings settings=new ScanSettings.Builder().setScanMode(scanMode).build();
        try {
            ++scanGeneration;
            scanResultCount=0;
            scanServiceDataResultCount=0;
            scanner.startScan(filters.isEmpty()?null:filters,settings,scanCallback);
            scanRunning=true;
            scanLowLatency=lowLatencyScan;
            for(Link link:candidates)
                if(!isReadyLocked(link)&&link.configError==null)
                    link.status=waitingStatus(link);
            Log.i(LOG_ID,"startScan: candidates="+candidates.size()+
                    " filters="+filters.size()+" match="+
                    (softwareMatching?"software token":"hardware discovery UUID")+
                    " mode="+(carrierActivePending?"low-latency active":
                            standbySoftwareProbe?"low-latency standby compatibility probe":
                            "balanced automatic standby"));
            scheduleScanWatchdogLocked(scanGeneration);
            if(standbySoftwareProbe)
                scheduleStandbyProbeEndLocked(scanGeneration,
                        scanSoftwareProbeUntilMillis-now);
        }
        catch(Throwable error) {
            Log.stack(LOG_ID,"startScan",error);
            setClientStatusesLocked("Could not scan for Bluetooth mirrors");
        }
    }

    private static boolean fragileMultiClientGattLocked() {
        return Build.VERSION.SDK_INT<=FRAGILE_MULTI_CLIENT_GATT_MAX_SDK;
    }

    private static Connection otherReadyClientConnectionLocked(Link except) {
        for(Link link:linksByIndex.values()) {
            if(link==null||link==except)
                continue;
            final Connection connection=link.connection;
            if(connection!=null&&!connection.serverSide&&connection.alive&&connection.ready)
                return connection;
        }
        return null;
    }

    /** Android 6-8: serialize outgoing GATT clients but keep the proven directions. */
    private static boolean fragileClientSlotLocked() {
        return fragileMultiClientGattLocked()&&!legacySingleGattLocked();
    }

    private static Link fragileWaitingClientLocked(Link current) {
        if(!fragileClientSlotLocked())
            return null;
        Link best=null;
        int bestPriority=Integer.MIN_VALUE;
        for(Link link:linksByIndex.values()) {
            if(link==null||link==current||link.configError!=null||link.connection!=null||
                    link.role!=ROLE_CLIENT||link.reverseGattRole||operationBlockerLocked(link)!=null)
                continue;
            // Time-share the scarce outgoing-client slot only for a link that
            // actually needs BLE as its data carrier. A normal Automatic standby
            // discovery boost must never evict a healthy data connection.
            if(!(link.transport==TRANSPORT_BLUETOOTH||link.automaticFallbackRequested))
                continue;
            final int priority=advertisementPriorityLocked(link);
            if(best==null||priority>bestPriority) {
                best=link;
                bestPriority=priority;
            }
        }
        return best;
    }

    /**
     * Remember demand even while the waiting link is transiently blocked.  The
     * old slot timer stopped permanently whenever fragileWaitingClientLocked()
     * returned null at one instant.  XA1 then left J5 on the client slot for
     * 10-20 minutes until some unrelated scan/network event happened to arm a
     * new check.
     */
    private static boolean fragileOtherClientDemandLocked(Link current) {
        if(!fragileClientSlotLocked())
            return false;
        for(Link link:linksByIndex.values()) {
            if(link==null||link==current||link.configError!=null||link.connection!=null||
                    link.role!=ROLE_CLIENT||link.reverseGattRole)
                continue;
            if(link.transport==TRANSPORT_BLUETOOTH||link.automaticFallbackRequested)
                return true;
        }
        return false;
    }

    private static void scheduleFragileClientSlotCheckLocked(Connection connection,long delayMillis) {
        if(!fragileClientSlotLocked()||connection==null||connection.serverSide||
                !connection.alive||!connection.ready||connection.link==null||
                connection.legacySlotCheckScheduled)
            return;
        connection.legacySlotCheckScheduled=true;
        final long epoch=connection.epoch;
        final long configuration=configurationGeneration;
        handler.postDelayed(() -> {
            synchronized(lock) {
                if(configuration!=configurationGeneration||connection.epoch!=epoch) {
                    connection.legacySlotCheckScheduled=false;
                    return;
                }
                connection.legacySlotCheckScheduled=false;
                if(!fragileClientSlotLocked()||!connection.alive||!connection.ready||
                        connection.serverSide||connection.link==null)
                    return;
                final Link current=connection.link;
                final Link waiting=fragileWaitingClientLocked(current);
                if(waiting==null) {
                    // A permission/radio/retry transition can make a demanded
                    // peer temporarily ineligible.  Keep one lightweight check
                    // armed instead of losing the handoff indefinitely.
                    if(fragileOtherClientDemandLocked(current))
                        scheduleFragileClientSlotCheckLocked(connection,1000L);
                    return;
                }
                final long now=monotonicMillis();
                final long leaseRemaining=current.legacySlotLeaseUntilMillis-now;
                if(leaseRemaining>0L) {
                    scheduleFragileClientSlotCheckLocked(connection,leaseRemaining);
                    return;
                }
                if(connection.sending!=null||!connection.outgoing.isEmpty()||
                        connection.decoder.inProgress()) {
                    scheduleFragileClientSlotCheckLocked(connection,500L);
                    return;
                }
                final long lastTraffic=Math.max(connection.lastPayloadActivityMillis,
                        connection.lastFragmentActivityMillis);
                final long idle=lastTraffic==0L?FRAGILE_CLIENT_SLOT_IDLE_MILLIS:
                        Math.max(0L,now-lastTraffic);
                if(idle<FRAGILE_CLIENT_SLOT_IDLE_MILLIS) {
                    scheduleFragileClientSlotCheckLocked(connection,
                            FRAGILE_CLIENT_SLOT_IDLE_MILLIS-idle);
                    return;
                }
                final long yieldUntil=now+FRAGILE_CLIENT_SLOT_LEASE_MILLIS;
                current.clientRetryNotBeforeMillis=Math.max(current.clientRetryNotBeforeMillis,
                        yieldUntil);
                if(current.transport==TRANSPORT_AUTOMATIC&&current.carrierActive)
                    current.legacyResumeCarrier=true;
                waiting.clientRetryNotBeforeMillis=0L;
                // Preserve the native carrier request exactly as it is. In
                // particular, do not turn an Automatic standby/discovery link into
                // an active BLE fallback merely because it is waiting for this slot.
                waiting.discoveryBoostUntilMillis=Math.max(waiting.discoveryBoostUntilMillis,
                        yieldUntil);
                waiting.status="Waiting for serialized Bluetooth slot "+waiting.label;
                Log.w(LOG_ID,"serializing fragile outgoing BLE clients: yielding "+
                        current.label+" to "+waiting.label+" for "+
                        FRAGILE_CLIENT_SLOT_LEASE_MILLIS+" ms");
                disconnectConnectionLocked(connection,
                        "Yielding fragile BLE client slot to "+waiting.label,true);
                reconcileOperationsLocked();
            }
        },Math.max(1L,delayMillis));
    }

    private static void maybeScheduleFragileClientSlotCheckLocked(Connection connection) {
        if(!fragileClientSlotLocked()||connection==null||connection.serverSide||
                !connection.alive||!connection.ready||connection.link==null)
            return;
        // Always arm one post-handshake check.  A waiting mirror can request
        // fallback between authentication and carrier activation; relying on the
        // exact state at either callback lost the return handoff in the overnight
        // XA1 trace.  The callback exits immediately when there is no demand.
        scheduleFragileClientSlotCheckLocked(connection,FRAGILE_CLIENT_SLOT_IDLE_MILLIS);
    }

    /**
     * A fragile Android central (XA1) cannot keep two outgoing GATT clients alive.
     * If the already-connected client is an old peer (for example J5), there is no
     * authenticated channel to the still-waiting modern peer on which /blerole can
     * be sent.  In that ordering, make only the waiting link a temporary peripheral.
     * Its peer already has Automatic fallback pending and modern Android can perform
     * the normal short reverse-role scan to initiate this connection.  Persisted QR
     * direction is never changed.
     */
    private static boolean offerResourceBalancedServerRoleLocked(Link waiting,Connection existing) {
        if(waiting==null||existing==null||waiting.role!=ROLE_CLIENT||waiting.wearOS||
                waiting.transport!=TRANSPORT_AUTOMATIC||waiting.connection!=null)
            return false;
        if(waiting.resourceReverseGattRole)
            return true;
        waiting.resourceReverseGattRole=true;
        waiting.reverseGattRole=true;
        waiting.resourceRoleSwapPending=false;
        waiting.clientRetryNotBeforeMillis=0L;
        waiting.oppositeRoleNotBeforeMillis=0L;
        waiting.automaticFallbackRequested=true;
        waiting.discoveryBoostUntilMillis=Math.max(waiting.discoveryBoostUntilMillis,
                monotonicMillis()+DISCOVERY_BOOST_MILLIS);
        waiting.status="Offering resource-balanced Bluetooth server role "+waiting.label;
        Log.w(LOG_ID,"fragile Android GATT client limit: existing client cannot be moved; "+
                "offering waiting Automatic mirror as runtime server instead: waiting="+
                waiting.label+" existing="+connectionLabel(existing)+" existingPeerSdk="+
                (existing.link==null?0:existing.link.remoteSdk));
        if(scanRunning)
            stopScanLocked();
        ensureServerLocked();
        boostDiscoveryLocked(waiting,"fragile Android second-client resource balance");
        return true;
    }

    /**
     * XA1/Android <= 8.1 can keep one outgoing GATT client healthy, but creating
     * a second client destroyed the first link's indication path.  If the already
     * authenticated client is an Automatic link to a modern peer, move that one
     * runtime-only to the peripheral role before opening the second client.
     * The authenticated /blerole control tells the modern peer to initiate the
     * replacement connection. Persisted pair direction is deliberately unchanged.
     */
    private static boolean scheduleResourceBalanceLocked(Connection existing,Link waiting) {
        if(existing==null||!existing.alive||!existing.ready||existing.serverSide||
                existing.link==null||waiting==null)
            return false;
        final Link move=existing.link;
        if(move==waiting||move.wearOS||move.transport!=TRANSPORT_AUTOMATIC||
                move.remoteSdk<=FRAGILE_MULTI_CLIENT_GATT_MAX_SDK)
            return false;
        if(move.resourceReverseGattRole||move.resourceRoleSwapPending)
            return true;
        final Outbound request=enqueueLocked(existing,RESOURCE_ROLE_PATH,
                new byte[]{RESOURCE_ROLE_BECOME_CLIENT});
        if(request==null)
            return false;
        move.resourceRoleSwapPending=true;
        final long generation=++move.resourceRoleSwapGeneration;
        waiting.clientRetryNotBeforeMillis=Math.max(waiting.clientRetryNotBeforeMillis,
                monotonicMillis()+RESOURCE_ROLE_SWITCH_SETTLE_MILLIS+CLIENT_CONNECT_SETTLE_MILLIS);
        waiting.status="Waiting for BLE GATT role balancing before connecting "+waiting.label;
        Log.w(LOG_ID,"fragile Android GATT client limit: requesting runtime role balance for existing Automatic mirror: existing="+
                move.label+" peerSdk="+move.remoteSdk+" waiting="+waiting.label);
        final long configuration=configurationGeneration;
        final long deadline=monotonicMillis()+3000L;
        handler.postDelayed(() -> finishResourceBalanceAfterControl(move,waiting,existing,request,
                generation,configuration,deadline),RESOURCE_ROLE_SWITCH_SETTLE_MILLIS);
        return true;
    }

    private static void finishResourceBalanceAfterControl(Link move,Link waiting,Connection existing,
            Outbound request,long generation,long configuration,long deadline) {
        synchronized(lock) {
            if(configuration!=configurationGeneration||generation!=move.resourceRoleSwapGeneration||
                    linksByIndex.get(move.hostIndex)!=move)
                return;
            if(request.finished.getCount()!=0L&&existing.alive&&monotonicMillis()<deadline) {
                handler.postDelayed(() -> finishResourceBalanceAfterControl(move,waiting,existing,request,
                        generation,configuration,deadline),150L);
                return;
            }
            move.resourceRoleSwapPending=false;
            if(request.finished.getCount()!=0L||!request.success) {
                Log.w(LOG_ID,"resource-balanced GATT role request was not delivered; preserving existing client: label="+
                        move.label+" waiting="+waiting.label);
                waiting.clientRetryNotBeforeMillis=Math.max(waiting.clientRetryNotBeforeMillis,
                        monotonicMillis()+CONCURRENT_CLIENT_RETRY_BACKOFF_MILLIS);
                handler.postDelayed(() -> retryOperations(configuration),
                        CONCURRENT_CLIENT_RETRY_BACKOFF_MILLIS);
                return;
            }
            // The peer authenticated and accepted /blerole before this connection
            // is closed.  Switch only in memory: persisted client/server preference
            // remains untouched and will be retried after a process restart.
            move.resourceReverseGattRole=true;
            move.reverseGattRole=true;
            move.oppositeRoleNotBeforeMillis=0L;
            if(move.transport==TRANSPORT_AUTOMATIC)
                move.automaticFallbackRequested=true;
            Log.w(LOG_ID,"resource-balanced GATT role request delivered; moving local link to runtime server role: label="+
                    move.label+" waiting="+waiting.label);
            if(move.connection!=null&&move.connection.alive&&!move.connection.serverSide)
                disconnectConnectionLocked(move.connection,
                        "Rebalancing fragile Android BLE GATT client roles",true);
            boostDiscoveryLocked(move,"fragile Android multi-client role balance");
            waiting.clientRetryNotBeforeMillis=0L;
            boostDiscoveryLocked(waiting,"retry after GATT client role balance");
            reconcileOperationsLocked();
        }
    }

    private static long clientRetryDelayLocked(Link link) {
        // Old Android stacks such as XA1 can keep one GATT client healthy but become
        // unstable when a second peer is opened/closed every few seconds.  Do not
        // suppress discovery of that second Automatic mirror: the remote peer may be
        // the side that lost TCP/IP, which is unknowable until we see its advertisement.
        // Instead keep discovery enabled and merely space physical connect attempts.
        if(link!=null&&fragileMultiClientGattLocked()&&
                link.transport==TRANSPORT_AUTOMATIC&&
                otherReadyClientConnectionLocked(link)!=null) {
            final int failures=Math.max(1,link.concurrentClientFailures);
            final int shift=Math.min(1,failures-1);
            return Math.min(CONCURRENT_CLIENT_RETRY_MAX_MILLIS,
                    CONCURRENT_CLIENT_RETRY_BACKOFF_MILLIS<<shift);
        }
        return CLIENT_RETRY_BACKOFF_MILLIS;
    }

    private static boolean hasPendingClientConnectionLocked() {
        for(Link link:linksByIndex.values())
            if(link.connection!=null&&!link.connection.serverSide&&link.connection.alive&&
                    !link.connection.ready)
                return true;
        return false;
    }

    private static List<Link> scannableClientLinksLocked() {
        final ArrayList<Link> all=new ArrayList<>();
        // Keep the controller out of application scanning while any outgoing
        // GATT connection is initiating/authenticating. On API 21-22 also keep
        // it out while *any* BLE mirror session owns the single reliable slot.
        if(hasPendingClientConnectionLocked()||
                (legacySingleGattLocked()&&legacyOccupiedConnectionLocked()!=null))
            return all;
        final Link leased=legacyLeasedLinkLocked();
        final boolean reverseProbe=reverseProbeActiveLocked();
        boolean hasActive=false;
        for(Link link:linksByIndex.values()) {
            if(leased!=null&&link!=leased)
                continue;
            if((legacySingleGattLocked()&&leased==null&&!legacyNeedsSlotLocked(link))||
                    legacyLinkSuppressedLocked(link)||link.configError!=null||link.connection!=null||
                    isReadyLocked(link)||operationBlockerLocked(link)!=null||
                    (link.role==ROLE_CLIENT&&!link.reverseGattRole&&
                            link.clientRetryNotBeforeMillis>monotonicMillis()))
                continue;
            // Keep Automatic peers discoverable even when another client GATT is
            // already healthy.  The need for Bluetooth can originate on the remote
            // side (for example J5 has no TCP/IP while XA1 still does), so suppressing
            // this scan candidate makes fallback impossible.  clientRetryDelayLocked()
            // protects fragile old stacks by throttling actual second-GATT attempts.
            final boolean preferredClient=link.role==ROLE_CLIENT&&!link.reverseGattRole;
            final boolean reverseClient=reverseProbe&&reverseProbeCandidateLocked(link);
            if(!preferredClient&&!reverseClient)
                continue;
            if(reverseClient&&!canReverseScanLocked())
                continue;
            all.add(link);
            if(link.transport==TRANSPORT_BLUETOOTH||link.automaticFallbackRequested||
                    discoveryBoostedLocked(link)||reverseClient)
                hasActive=true;
        }
        if(!hasActive)
            return all;
        final ArrayList<Link> active=new ArrayList<>();
        for(Link link:all)
            if(link.transport==TRANSPORT_BLUETOOTH||link.automaticFallbackRequested||
                    discoveryBoostedLocked(link)||
                    (reverseProbe&&reverseProbeCandidateLocked(link)))
                active.add(link);
        return active;
    }

    private static void refreshScanLocked() {
        if(bluetoothRestartSettleRemainingLocked()>0L) {
            stopScanLocked();
            return;
        }
        if(serverHandshakeHoldRemainingLocked()>0L||hasPendingClientConnectionLocked()) {
            stopScanLocked();
            return;
        }
        if(!scannableClientLinksLocked().isEmpty()) {
            startScanLocked();
            return;
        }
        stopScanLocked();
    }

    private static void setClientStatusesLocked(String status) {
        for(Link link:scannableClientLinksLocked())
            if(link.configError==null&&!isReadyLocked(link))
                link.status=status;
    }

    @SuppressLint("MissingPermission")
    private static void stopScanLocked() {
        ++scanGeneration;
        if(scanRunning&&scanner!=null) {
            try { scanner.stopScan(scanCallback); }
            catch(Throwable error) { Log.stack(LOG_ID,"stopScan",error); }
        }
        scanRunning=false;
        scanLowLatency=false;
        // Do not retain a scanner proxy across Bluetooth-service restarts.
        scanner=null;
    }

    private static void scheduleScanWatchdogLocked(long generation) {
        handler.postDelayed(() -> {
            synchronized(lock) {
                if(!scanRunning||generation!=scanGeneration)
                    return;
                if(scannableClientLinksLocked().isEmpty())
                    return;
                if(!scanLowLatency) {
                    scanSoftwareProbeUntilMillis=monotonicMillis()+
                            SCAN_STANDBY_PROBE_MILLIS;
                    Log.i(LOG_ID,"BLE scan watchdog starting a "+
                            SCAN_STANDBY_PROBE_MILLIS+
                            " ms unfiltered Automatic-standby compatibility probe");
                }
                else
                    Log.i(LOG_ID,"BLE scan watchdog restarting a silent active scan after "+
                            SCAN_WATCHDOG_MILLIS+" ms; callbacks="+scanResultCount+
                            " JugglucoDiscovery="+scanServiceDataResultCount);
                stopScanLocked();
                startScanLocked();
            }
        },SCAN_WATCHDOG_MILLIS);
    }

    private static void scheduleStandbyProbeEndLocked(long generation,long delayMillis) {
        handler.postDelayed(() -> {
            synchronized(lock) {
                if(!scanRunning||generation!=scanGeneration)
                    return;
                final long remaining=scanSoftwareProbeUntilMillis-monotonicMillis();
                if(remaining>0) {
                    scheduleStandbyProbeEndLocked(generation,remaining);
                    return;
                }
                for(Link link:scannableClientLinksLocked())
                    if(link.transport==TRANSPORT_BLUETOOTH||discoveryBoostedLocked(link)) {
                        scanSoftwareProbeUntilMillis=0;
                        return;
                    }
                scanSoftwareProbeUntilMillis=0;
                Log.i(LOG_ID,"BLE Automatic-standby compatibility probe finished; "+
                        "callbacks="+scanResultCount+" JugglucoDiscovery="+
                        scanServiceDataResultCount+"; returning to filtered balanced scan");
                stopScanLocked();
                startScanLocked();
            }
        },Math.max(1L,delayMillis));
    }

    /**
     * Return Juggluco's password-derived discovery token from a scan record.
     *
     * Legacy discovery-token reader. Current builds encode the token in a
     * private 128-bit service UUID and match that before calling this method.
     * Continue accepting the preceding 16-bit and 128-bit Service Data formats
     * so mixed-version peers still have a chance to connect. Android 5.x-7.x
     * cannot expose 128-bit Service Data through getServiceData(), so retain the
     * raw AD type 0x21 parser as a best-effort fallback.
     */
    private static byte[] discoveryToken(ScanRecord record) {
        if(record==null)
            return null;
        byte[] token=record.getServiceData(SERVICE_PARCEL_UUID);
        if(token!=null)
            return token;
        token=record.getServiceData(DISCOVERY_SERVICE_DATA_UUID);
        if(token!=null)
            return token;
        final byte[] raw=record.getBytes();
        if(raw==null)
            return null;
        int pos=0;
        while(pos<raw.length) {
            final int length=raw[pos++]&0xff;
            if(length==0)
                break;
            if(pos+length>raw.length)
                break;
            final int type=raw[pos++]&0xff;
            final int dataLength=length-1;
            if(type==AD_TYPE_SERVICE_DATA_128&&dataLength>=16) {
                // Bluetooth UUIDs are little-endian in advertising packets.
                // Avoid BluetoothUuid (hidden API) by comparing the two 64-bit
                // halves directly against the legacy service UUID.
                long lsb=0,msb=0;
                for(int i=0;i<8;i++)
                    lsb|=(long)(raw[pos+i]&0xff)<<(8*i);
                for(int i=0;i<8;i++)
                    msb|=(long)(raw[pos+8+i]&0xff)<<(8*i);
                if(lsb==LEGACY_SERVICE_UUID.getLeastSignificantBits()&&
                        msb==LEGACY_SERVICE_UUID.getMostSignificantBits())
                    return Arrays.copyOfRange(raw,pos+16,pos+dataLength);
            }
            pos+=dataLength;
        }
        return null;
    }

    @SuppressLint("MissingPermission")
    private static void foundDevice(ScanResult result) {
        if(result==null)
            return;
        final ScanRecord record=result.getScanRecord();
        synchronized(lock) {
            if(!scanRunning)
                return;
            ++scanResultCount;
            Link matched=null;
            boolean android5SharedProtocol=false;
            // v7 emits d20 again on Android 5 because that is the known-working
            // pre-v24 layout. Prefer d20 if both generations are briefly visible
            // while upgrading; d30 is receive-side compatibility only.
            for(Link link:scannableClientLinksLocked()) {
                if(link.configError==null&&link.connection==null&&
                        hasDiscoveryUuid(record,link.discoveryToken)) {
                    matched=link;
                    break;
                }
            }
            if(matched==null)
                for(Link link:scannableClientLinksLocked()) {
                    if(link.configError==null&&link.connection==null&&
                            hasAndroid5DiscoveryUuid(record,link.discoveryToken)) {
                        matched=link;
                        android5SharedProtocol=true;
                        break;
                    }
                }
            if(matched==null) {
                // Do not reconnect through any v13/v14 advertisement, even when
                // its token matches a configured current peer. Android 5 was
                // observed to emit stale legacy and current advertisements for
                // the same address alternately; accepting the stale one routed
                // /blehello through obsolete handles and poisoned rendezvous.
                Link stale=null;
                for(Link link:scannableClientLinksLocked()) {
                    if(link.configError==null&&link.connection==null&&
                            hasLegacyDiscoveryUuid(record,link.discoveryToken)) {
                        stale=link;
                        break;
                    }
                }
                byte[] legacyToken=null;
                if(stale==null) {
                    legacyToken=discoveryToken(record);
                    if(legacyToken!=null)
                        for(Link link:scannableClientLinksLocked())
                            if(link.configError==null&&link.connection==null&&
                                    MessageDigest.isEqual(link.discoveryToken,legacyToken)) {
                                stale=link;
                                break;
                            }
                }
                if(stale!=null) {
                    ++scanServiceDataResultCount;
                    final long now=monotonicMillis();
                    if(now-lastUnmatchedScanLogMillis>=DISCOVERY_LOG_MILLIS) {
                        lastUnmatchedScanLogMillis=now;
                        Log.w(LOG_ID,"ignoring stale legacy BLE mirror advertisement: label="+
                                stale.label+" token="+tokenFingerprint(stale.discoveryToken)+
                                " address="+safeAddress(result.getDevice())+
                                " currentRequired="+discoveryUuid(stale.discoveryToken));
                    }
                }
                else if(legacyToken!=null) {
                    ++scanServiceDataResultCount;
                    final long now=monotonicMillis();
                    if(now-lastUnmatchedScanLogMillis>=DISCOVERY_LOG_MILLIS) {
                        lastUnmatchedScanLogMillis=now;
                        Log.i(LOG_ID,"scan saw unmatched legacy Juggluco service data: token="+
                                tokenFingerprint(legacyToken)+" address="+
                                safeAddress(result.getDevice()));
                    }
                }
                return;
            }
            ++scanServiceDataResultCount;
            final BluetoothDevice device=result.getDevice();
            final UUID matchedDiscoveryUuid=android5SharedProtocol?
                    android5DiscoveryUuid(matched.discoveryToken):
                    discoveryUuid(matched.discoveryToken);
            Log.i(LOG_ID,"scan matched: index="+matched.hostIndex+" label="+matched.label+
                    " address="+safeAddress(device)+" rssi="+result.getRssi()+
                    " discoveryUuid="+matchedDiscoveryUuid+
                    " protocol="+(android5SharedProtocol?"v16-android5-shared":"v15-token")+
                    " physicalRole="+
                    (matched.role==ROLE_SERVER?"reverse-client":"preferred-client"));
            if(acceptAutomaticIpv4EndpointHintLocked(matched,record)) {
                // Endpoint discovery is supplemental.  In particular, the local
                // device may currently have no usable IP transport at all (for
                // example Wi-Fi is off) even though the peer advertises a valid
                // address.  Do not let learning that address suppress the BLE
                // connection that delivered the hint.  TCP is woken in parallel
                // and may become preferred later after it really authenticates.
                Log.i(LOG_ID,"BLE endpoint hint accepted; continuing GATT connect: label="+
                        matched.label);
            }
            connectClientLocked(matched,device,true,android5SharedProtocol);
            refreshScanLocked();
        }
    }

    @SuppressLint("MissingPermission")
    private static void connectClientLocked(Link link,BluetoothDevice device,boolean currentProtocol,
            boolean android5SharedProtocol) {
        if(bluetoothRestartSettleRemainingLocked()>0L) {
            if(link!=null)
                link.status="Waiting for Bluetooth service restart to settle";
            return;
        }
        if(link.connection!=null||device==null)
            return;
        if(fragileClientSlotLocked()) {
            final Connection existing=otherReadyClientConnectionLocked(link);
            if(existing!=null) {
                // Do not change either proven GATT direction. XA1 repeatedly failed
                // both with two simultaneous clients and with one client plus one
                // reversed server. Time-share the reliable outgoing slot instead.
                link.status="Waiting for serialized Bluetooth slot "+link.label;
                link.clientRetryNotBeforeMillis=0L;
                Log.i(LOG_ID,"deferring second outgoing BLE GATT client until current slot yields: label="+
                        link.label+" existing="+connectionLabel(existing));
                scheduleFragileClientSlotCheckLocked(existing,FRAGILE_CLIENT_SLOT_IDLE_MILLIS);
                return;
            }
        }
        if(legacySingleGattLocked()) {
            final Connection occupied=legacyOccupiedConnectionLocked();
            if(occupied!=null) {
                Log.i(LOG_ID,"deferring Android 5 BLE client; slot occupied: label="+
                        link.label+" by="+connectionLabel(occupied));
                maybeScheduleLegacySlotCheckLocked(occupied);
                return;
            }
        }
        final Connection connection=new Connection(false,device,link);
        connection.currentGattProtocol=currentProtocol;
        connection.android5SharedGattProtocol=android5SharedProtocol;
        link.connection=connection;
        link.clientRetryNotBeforeMillis=0;
        link.status=(link.transport==TRANSPORT_AUTOMATIC?
                "Connecting automatic Bluetooth fallback ":"Connecting direct Bluetooth mirror ")+
                link.label;

        // Do not overlap our scanner/advertiser with the controller's initiating
        // procedure. The Huawei M5 HCI trace showed LE Create Connection being
        // rejected with 0x0C (Command Disallowed) while both were re-enabled.
        // A short settle delay lets stopScan()/stopAdvertising() reach the
        // controller before connectGatt() asks Android to initiate.
        stopScanLocked();
        stopAdvertisingLocked();
        final long epoch=connection.epoch;
        Log.i(LOG_ID,"BLE radio roles paused before connect: index="+link.hostIndex+
                " label="+link.label+" settle="+CLIENT_CONNECT_SETTLE_MILLIS+" ms");
        handler.postDelayed(() -> beginClientConnection(connection,epoch),
                CLIENT_CONNECT_SETTLE_MILLIS);
    }

    @SuppressLint("MissingPermission")
    private static void beginClientConnection(Connection connection,long epoch) {
        synchronized(lock) {
            if(!connection.alive||connection.epoch!=epoch||connection.link==null||
                    connection.link.connection!=connection)
                return;
            final Link link=connection.link;
            final BluetoothGattCallback callback=newClientCallback(connection);
            try {
                connection.gatt=Build.VERSION.SDK_INT>=23?
                        connection.device.connectGatt(Applic.app,false,callback,
                                BluetoothDevice.TRANSPORT_LE):
                        connection.device.connectGatt(Applic.app,false,callback);
                if(connection.gatt==null) {
                    disconnectConnectionLocked(connection,
                            "Could not create a BLE GATT connection",false);
                    reconcileOperationsLocked();
                    return;
                }
                Log.i(LOG_ID,"connectGatt: index="+link.hostIndex+" label="+link.label+
                        " address="+safeAddress(connection.device)+
                        " physicalRole="+(link.role==ROLE_SERVER?"reverse-client":"preferred-client")+
                        " (Android bonding intentionally unused; scanner/advertiser paused)");
                scheduleClientEstablishmentTimeoutLocked(connection);
                scheduleHandshakeTimeoutLocked(connection);
            }
            catch(Throwable error) {
                Log.stack(LOG_ID,"connectGatt",error);
                disconnectConnectionLocked(connection,"Could not connect the BLE mirror",false);
                reconcileOperationsLocked();
            }
        }
    }

    /**
     * A configured client can otherwise scan forever when the two databases
     * temporarily disagree about the pair direction: client/client produces no
     * advertisement, therefore there is no connectGatt() failure from which the
     * existing role-reversal logic can recover.  When native Automatic fallback
     * is actively waiting, give the persisted client direction its full protected
     * trial interval.  If it has still not even produced a physical connection,
     * temporarily offer the opposite GATT role.  The passive peer continues its
     * normal standby client scan and can then connect.  Authentication remains
     * the only event that persists the resulting pair direction.
     */
    private static void schedulePreferredClientNoPeerRecoveryLocked(Link link) {
        if(link==null||link.directionProven||link.transport!=TRANSPORT_AUTOMATIC||
                !link.automaticFallbackRequested||link.role!=ROLE_CLIENT||
                link.reverseGattRole)
            return;
        final long generation=++link.automaticFallbackGeneration;
        final long delay=Math.max(1L,link.oppositeRoleNotBeforeMillis-monotonicMillis()+50L);
        handler.postDelayed(() -> {
            synchronized(lock) {
                if(linksByIndex.get(link.hostIndex)!=link||
                        generation!=link.automaticFallbackGeneration||
                        link.directionProven||!link.automaticFallbackRequested||link.role!=ROLE_CLIENT||
                        link.reverseGattRole||link.connection!=null||isReadyLocked(link))
                    return;
                Log.w(LOG_ID,"preferred BLE client found no peer during protected trial; "+
                        "temporarily offering opposite GATT role: label="+link.label+
                        " side="+(link.side?"r2":"r1")+
                        " direction="+(link.persistedPairReverse?"reversed":"normal"));
                if(activateReverseGattRoleLocked(link,
                        "preferred client discovery timed out without a peer advertisement"))
                    reconcileOperationsLocked();
            }
        },delay);
    }

    private static boolean activateReverseGattRoleLocked(Link link,String reason) {
        if(link==null||link.role!=ROLE_CLIENT)
            return false;
        if(link.directionProven) {
            link.reverseGattRole=false;
            link.transientClientFailures=0;
            Log.w(LOG_ID,"proven Bluetooth direction is locked; refusing automatic role reversal: label="+
                    link.label+" side="+(link.side?"r2":"r1")+
                    " direction="+(link.persistedPairReverse?"reversed":"normal")+
                    " reason="+reason);
            return false;
        }
        if(link.reverseGattRole) {
            scheduleTemporaryReverseRoleExpiryLocked(link);
            return true;
        }
        final long now=monotonicMillis();
        if(now<link.oppositeRoleNotBeforeMillis) {
            final long remaining=link.oppositeRoleNotBeforeMillis-now;
            link.status=waitingStatus(link);
            Log.i(LOG_ID,"preferred Bluetooth direction retained after failure: label="+
                    link.label+" remaining="+remaining+" ms reason="+reason);
            return false;
        }
        if(!canReverseAdvertiseLocked()) {
            Log.w(LOG_ID,"cannot reverse GATT role for "+link.label+
                    ": BLE advertising is unavailable");
            // Do not consume the only client/client recovery opportunity.  Retry the
            // persisted client direction first, then attempt the reverse role again later.
            link.oppositeRoleNotBeforeMillis=now+PREFERRED_ROLE_TRY_MILLIS;
            if(link.automaticFallbackRequested)
                schedulePreferredClientNoPeerRecoveryLocked(link);
            return false;
        }
        link.reverseGattRole=true;
        link.discoveryBoostUntilMillis=Math.max(link.discoveryBoostUntilMillis,
                monotonicMillis()+DISCOVERY_BOOST_MILLIS);
        link.status="Offering reversed Bluetooth GATT role "+link.label;
        Log.w(LOG_ID,"reversing physical Bluetooth GATT role: index="+link.hostIndex+
                " label="+link.label+" reason="+reason);
        scheduleTemporaryReverseRoleExpiryLocked(link);
        ensureServerLocked();
        return true;
    }

    /**
     * A failed preferred-client setup may temporarily reverse a logical client
     * into a GATT server. Do not let that unproven role become sticky merely
     * because its advertisement succeeds. Authentication persists the physical
     * direction through persistPreferredGattRoleLocked().
     */
    private static void scheduleTemporaryReverseRoleExpiryLocked(Link link) {
        if(link==null||link.role!=ROLE_CLIENT||!link.reverseGattRole)
            return;
        final long now=monotonicMillis();
        if(link.temporaryReverseUntilMillis>now)
            return;
        link.temporaryReverseUntilMillis=now+TEMPORARY_REVERSE_ROLE_LEASE_MILLIS;
        final long generation=++link.temporaryReverseGeneration;
        Log.i(LOG_ID,"temporary reversed GATT role lease armed: label="+link.label+
                " lease="+TEMPORARY_REVERSE_ROLE_LEASE_MILLIS+" ms");
        handler.postDelayed(() -> {
            synchronized(lock) {
                if(linksByIndex.get(link.hostIndex)!=link||
                        generation!=link.temporaryReverseGeneration)
                    return;
                link.temporaryReverseUntilMillis=0L;
                if(!link.reverseGattRole)
                    return;
                // A successful handshake would already have persisted the role.
                // An unauthenticated logical server session must not keep the
                // temporary direction alive indefinitely.
                if(link.connection!=null)
                    disconnectConnectionLocked(link.connection,
                            "Unproven reversed Bluetooth role expired",true);
                link.reverseGattRole=false;
                link.transientClientFailures=0;
                final long retryNow=monotonicMillis();
                link.clientRetryNotBeforeMillis=Math.max(link.clientRetryNotBeforeMillis,
                        retryNow+1000L);
                link.discoveryBoostUntilMillis=Math.max(link.discoveryBoostUntilMillis,
                        retryNow+DISCOVERY_BOOST_MILLIS);
                // v19 armed the no-peer recovery only once.  If that one server experiment
                // happened to coincide with a Bluetooth-stack restart on the peer, the link
                // returned to client and scanned forever.  Re-arm a complete preferred-client
                // trial while native Automatic fallback is still waiting.
                link.oppositeRoleNotBeforeMillis=retryNow+PREFERRED_ROLE_TRY_MILLIS;
                if(link.automaticFallbackRequested)
                    schedulePreferredClientNoPeerRecoveryLocked(link);
                link.status="Reversed Bluetooth role did not authenticate; retrying client "+
                        link.label;
                Log.w(LOG_ID,"temporary reversed GATT role expired without authentication; "+
                        "returning to preferred client and re-arming recovery: label="+link.label);
                reconcileOperationsLocked();
            }
        },TEMPORARY_REVERSE_ROLE_LEASE_MILLIS);
    }

    private static void scheduleClientEstablishmentTimeoutLocked(Connection connection) {
        final long epoch=connection.epoch;
        handler.postDelayed(() -> {
            synchronized(lock) {
                if(!connection.alive||connection.serverSide||connection.connected||
                        connection.epoch!=epoch)
                    return;
                final Link link=connection.link;
                final String reason="BLE physical connection did not establish within "+
                        CLIENT_ESTABLISH_TIMEOUT_MILLIS+" ms";
                Log.e(LOG_ID,reason+" label="+connectionLabel(connection)+
                        " address="+safeAddress(connection.device));
                disconnectConnectionLocked(connection,reason,true);
                if(link!=null&&link.role==ROLE_CLIENT&&!link.reverseGattRole) {
                    if(link.directionProven) {
                        link.transientClientFailures=0;
                        Log.w(LOG_ID,"BLE physical establishment timeout on proven direction; retrying the same client role: label="+
                                link.label);
                    }
                    else {
                        ++link.transientClientFailures;
                        if(link.transientClientFailures<CLIENT_ESTABLISHMENT_FAILURES_BEFORE_REVERSE) {
                            Log.w(LOG_ID,"BLE physical establishment timeout is treated as transient; "+
                                    "retrying preferred client role: label="+link.label+
                                    " attempt="+link.transientClientFailures+"/"+
                                    CLIENT_ESTABLISHMENT_FAILURES_BEFORE_REVERSE);
                        }
                        else if(activateReverseGattRoleLocked(link,reason))
                            Log.i(LOG_ID,"unproven client repeatedly failed to establish; now advertising "+
                                    "the opposite physical role: label="+link.label);
                    }
                }
                reconcileOperationsLocked();
            }
        },CLIENT_ESTABLISH_TIMEOUT_MILLIS);
    }

    private static void scheduleHandshakeTimeoutLocked(Connection connection) {
        final long epoch=connection.epoch;
        handler.postDelayed(() -> {
            boolean timedOut;
            synchronized(lock) {
                timedOut=connection.alive&&!connection.ready&&connection.epoch==epoch;
            }
            if(timedOut)
                disconnectForFailure(connection,"BLE mirror authentication timed out");
        },HANDSHAKE_TIMEOUT_MILLIS);
    }

    /**
     * True while an outgoing connection has reached the physical link but has
     * not yet got far enough to send /blehello.  Failures in this interval are
     * GATT-client setup failures (service discovery, notification/CCCD setup),
     * not Juggluco authentication failures.  They are therefore useful evidence
     * that the peer should be tried with the physical GATT roles reversed.
     */
    private static boolean canReverseAfterClientSetupFailureLocked(Connection connection) {
        if(connection==null||connection.serverSide||!connection.connected||connection.ready||
                connection.clientNonce!=null)
            return false;
        final Link link=connection.link;
        return link!=null&&link.role==ROLE_CLIENT&&!link.reverseGattRole;
    }

    private static void scheduleSubscriptionTimeoutLocked(Connection connection) {
        final long epoch=connection.epoch;
        final long timeout=Build.VERSION.SDK_INT<=22?
                LEGACY_SUBSCRIPTION_TIMEOUT_MILLIS:SUBSCRIPTION_TIMEOUT_MILLIS;
        handler.postDelayed(() -> {
            synchronized(lock) {
                // A ready connection can legitimately be here while recovering
                // from Android's GATT Service Changed indication.  In that case
                // the old CCCD/attribute handles may no longer deliver server
                // indications even though client writes still succeed.  Treat a
                // failed re-subscription just like a normal CCCD setup failure.
                if(!connection.alive||connection.subscribed||
                        !connection.subscriptionRequested||connection.epoch!=epoch)
                    return;
                connection.subscriptionRequested=false;
                // Some Samsung/vendor stacks complete the CCCD write on the peer
                // but lose only onDescriptorWrite(). M5 explicitly logged the
                // subscription as accepted while S5 later timed out here. During
                // initial setup, try the authenticated hello before destroying that
                // otherwise valid ATT session. If the CCCD really was not applied,
                // /bleready will not arrive and the normal handshake timeout still
                // reconnects cleanly.
                if(!connection.ready&&!connection.serviceRefreshInProgress&&
                        connection.connected&&connection.clientNonce==null) {
                    connection.subscribed=true;
                    Log.w(LOG_ID,"BLE CCCD callback missing after "+timeout+
                            " ms; attempting authenticated hello before reconnect: label="+
                            connectionLabel(connection));
                    handler.post(() -> sendHello(connection));
                    return;
                }
                connection.serviceRefreshInProgress=false;
                final boolean reverse=canReverseAfterClientSetupFailureLocked(connection);
                final Link link=connection.link;
                final boolean refreshed=refreshGattCacheLocked(connection);
                final String reason="BLE indication setup timed out; GATT cache refresh "+
                        (refreshed?"accepted":"unavailable");
                Log.e(LOG_ID,reason+" label="+connectionLabel(connection)+
                        " address="+safeAddress(connection.device)+
                        " reverseRole="+reverse+" timeout="+timeout+" ms");
                disconnectConnectionLocked(connection,reason,true);
                if(reverse&&activateReverseGattRoleLocked(link,reason))
                    Log.i(LOG_ID,"GATT setup failed before BLE authentication; trying opposite "+
                            "physical role: label="+link.label);
                final long configuration=configurationGeneration;
                handler.postDelayed(() -> retryOperations(configuration),
                        CACHE_REFRESH_SETTLE_MILLIS);
                reconcileOperationsLocked();
            }
        },timeout);
    }

    /**
     * Android can retain attribute handles after the peer application has
     * rebuilt its GATT server. In that state writeDescriptor() returns success
     * locally but neither its callback nor the server request ever arrives.
     * Android exposes cache invalidation only as the hidden refresh() method,
     * so use it narrowly after that exact, timed-out state and retain the normal
     * reconnect fallback when an Android release blocks reflective access.
     */
    private static boolean refreshGattCacheLocked(Connection connection) {
        final BluetoothGatt gatt=connection.gatt;
        if(gatt==null)
            return false;
        try {
            final Object result=BluetoothGatt.class.getMethod("refresh").invoke(gatt);
            final boolean accepted=!(result instanceof Boolean)||((Boolean)result);
            Log.i(LOG_ID,"requested Bluetooth GATT cache refresh: label="+
                    connectionLabel(connection)+" accepted="+accepted);
            return accepted;
        }
        catch(Throwable error) {
            Log.e(LOG_ID,"Bluetooth GATT cache refresh unavailable: label="+
                    connectionLabel(connection)+" error="+error.getClass().getSimpleName());
            return false;
        }
    }

    private static void retryOperations(long expectedConfiguration) {
        synchronized(lock) {
            if(expectedConfiguration!=configurationGeneration)
                return;
            reconcileOperationsLocked();
        }
    }

    private static void disconnectForFailure(Connection connection,String reason) {
        synchronized(lock) {
            if(!connection.alive)
                return;
            final boolean reverse=canReverseAfterClientSetupFailureLocked(connection);
            final Link link=connection.link;
            Log.e(LOG_ID,reason+" label="+connectionLabel(connection)+
                    " address="+safeAddress(connection.device)+
                    " reverseRole="+reverse);
            disconnectConnectionLocked(connection,reason,true);
            if(reverse&&activateReverseGattRoleLocked(link,reason))
                Log.i(LOG_ID,"GATT setup failed before BLE authentication; trying opposite "+
                        "physical role: label="+link.label);
            reconcileOperationsLocked();
        }
    }

    @SuppressLint("MissingPermission")
    private static void disconnectConnectionLocked(Connection connection,String reason,boolean cancelGatt) {
        if(connection==null||!connection.alive)
            return;
        connection.alive=false;
        ++connection.epoch;
        failQueueLocked(connection);
        connection.decoder.reset();
        final Link link=connection.link;
        final int remote=connection.remoteHostIndex;
        final boolean wasReady=connection.ready;
        final boolean hadMapping=wasReady&&link!=null&&link.carrierActive;
        connection.ready=false;
        connection.subscribed=false;
        connection.subscriptionRequested=false;
        connection.serviceRefreshInProgress=false;
        if(link!=null&&link.connection==connection) {
            link.carrierActive=false;
            link.connection=null;
            if(wasReady)
                link.oppositeRoleNotBeforeMillis=monotonicMillis()+PREFERRED_ROLE_TRY_MILLIS;
            if(!connection.serverSide&&!wasReady&&!link.reverseGattRole) {
                final long retryDelay=clientRetryDelayLocked(link);
                link.clientRetryNotBeforeMillis=Math.max(link.clientRetryNotBeforeMillis,
                        monotonicMillis()+retryDelay);
                Log.i(LOG_ID,"client retry cooldown: label="+link.label+" delay="+
                        retryDelay+" ms concurrentFailures="+link.concurrentClientFailures);
                final long retryConfiguration=configurationGeneration;
                handler.postDelayed(() -> retryOperations(retryConfiguration),retryDelay);
            }
            link.status=reason;
        }
        if(connection.serverSide) {
            if(link!=null&&link.resourceReverseGattRole) {
                link.resourceReverseGattRole=false;
                link.reverseGattRole=false;
                link.resourceRoleSwapPending=false;
                link.clientRetryNotBeforeMillis=Math.max(link.clientRetryNotBeforeMillis,
                        monotonicMillis()+1000L);
                Log.i(LOG_ID,"resource-balanced server session ended; restoring proven client role: label="+
                        link.label);
            }
            final String address=safeAddress(connection.device);
            // Shared API-21/22 sessions are inserted under the physical address
            // before /blehello binds connection.link. Remove by identity so the
            // map key does not depend on whether authentication has happened.
            serverConnections.values().remove(connection);
            if(cancelGatt&&gattServer!=null&&connection.device!=null&&!hasServerSessionForAddressLocked(address))
                try { gattServer.cancelConnection(connection.device); } catch(Throwable ignored) {}
        }
        else if(connection.gatt!=null) {
            if(cancelGatt)
                try { connection.gatt.disconnect(); } catch(Throwable ignored) {}
            try { connection.gatt.close(); } catch(Throwable ignored) {}
            connection.gatt=null;
            connection.rx=null;
            connection.tx=null;
        }
        Log.i(LOG_ID,"connection closed: reason="+reason+" label="+
                (link==null?"not authenticated":link.label)+" address="+safeAddress(connection.device));
        if(hadMapping) {
            final int local=link.hostIndex;
            handler.post(() -> {
                synchronized(lock) {
                    final Link current=linksByIndex.get(local);
                    if(current!=null&&current.carrierActive) {
                        Log.i(LOG_ID,"Skipping stale native disconnect for "+link.label+
                                "; a newer Bluetooth carrier is active");
                        return;
                    }
                }
                Natives.mirrorTransportDisconnected(local,remote);
            });
        }
    }

    private static void disconnectLinkLocked(Link link,String reason) {
        if(link.connection!=null)
            disconnectConnectionLocked(link.connection,reason,true);
        link.status=reason;
    }

    @SuppressLint("MissingPermission")
    private static void stopServerLocked(String reason) {
        // Advertising is logically part of the server, but after a Bluetooth
        // process failure the GATT-server proxy may already be null while our
        // advertiser bookkeeping still says it is active. Clear advertising
        // before the early-out so a later STATE_ON can always start it anew.
        stopAdvertisingLocked();
        if(gattServer==null&&serverConnections.isEmpty()) {
            serverSendingConnection=null;
            serverServiceReady=false;
            serverEndpoints.clear();
            serverCharacteristics.clear();
            serverServiceQueue.clear();
            serverDeviceMtu.clear();
            serverHandshakeHoldUntil.clear();
            expiredUnauthenticatedServerAddresses.clear();
            return;
        }
        ++serverGeneration;
        final ArrayList<Connection> connections=new ArrayList<>(serverConnections.values());
        for(Connection connection:connections)
            disconnectConnectionLocked(connection,reason,false);
        serverConnections.clear();
        if(gattServer!=null) {
            try {
                gattServer.clearServices();
                Log.i(LOG_ID,"cleared BLE GATT services while stopping mirror server");
            }
            catch(Throwable ignored) {}
            try { gattServer.close(); } catch(Throwable ignored) {}
        }
        gattServer=null;
        serverSendingConnection=null;
        serverServiceReady=false;
        serverEndpoints.clear();
        serverCharacteristics.clear();
        serverServiceQueue.clear();
        serverDeviceMtu.clear();
        serverHandshakeHoldUntil.clear();
        expiredUnauthenticatedServerAddresses.clear();
    }

    @SuppressLint("MissingPermission")
    private static void stopAllConnectionsLocked(String reason) {
        reverseProbeUntilMillis=0;
        reverseProbeScheduled=false;
        ++reverseProbeGeneration;
        stopScanLocked();
        stopServerLocked(reason);
        final ArrayList<Link> links=new ArrayList<>(linksByIndex.values());
        for(Link link:links)
            if(link.connection!=null)
                disconnectConnectionLocked(link.connection,reason,true);
        for(Link link:links)
            link.status=reason;
    }

    private static void registerReceiverLocked() {
        if(receiverRegistered)
            return;
        final IntentFilter filter=new IntentFilter(BluetoothAdapter.ACTION_STATE_CHANGED);
        ContextCompat.registerReceiver(Applic.app,bluetoothReceiver,filter,ContextCompat.RECEIVER_EXPORTED);
        receiverRegistered=true;
    }

    private static final BroadcastReceiver bluetoothReceiver=new BroadcastReceiver() {
        @Override public void onReceive(Context context,Intent intent) {
            if(!BluetoothAdapter.ACTION_STATE_CHANGED.equals(intent.getAction()))
                return;
            final int state=intent.getIntExtra(BluetoothAdapter.EXTRA_STATE,BluetoothAdapter.ERROR);
            final int previous=intent.getIntExtra(BluetoothAdapter.EXTRA_PREVIOUS_STATE,
                    BluetoothAdapter.ERROR);
            Log.i(LOG_ID,"Bluetooth adapter state changed: "+previous+" -> "+state);
            if(state==BluetoothAdapter.STATE_ON) {
                final long recoveryGeneration;
                synchronized(lock) {
                    // A vendor Bluetooth process can die/restart without every old
                    // Java proxy reporting an error. onStartSuccess() from an old
                    // advertiser is therefore not proof that the controller is still
                    // transmitting. Rebuild the complete BLE mirror side whenever the
                    // adapter reaches ON, even if OFF was not observed by this process.
                    stopAllConnectionsLocked("Bluetooth adapter restarted");
                    bluetoothManager=(BluetoothManager)Applic.app.getSystemService(
                            Context.BLUETOOTH_SERVICE);
                    adapter=bluetoothManager==null?null:bluetoothManager.getAdapter();
                    advertiser=null;
                    scanner=null;
                    advertiseCursor=0;
                    advertiseRestartNotBeforeMillis=0;
                    advertiseRestartScheduled=false;
                    scanSoftwareProbeUntilMillis=0;
                    bluetoothRestartSettleUntilMillis=monotonicMillis()+
                            BLUETOOTH_ON_SETTLE_MILLIS;
                    recoveryGeneration=++bluetoothOnRecoveryGeneration;
                    Log.i(LOG_ID,"Bluetooth adapter ON: discarded stale BLE mirror objects; "+
                            "strictly deferring all BLE mirror radio operations for "+
                            BLUETOOTH_ON_SETTLE_MILLIS+" ms before rebuilding");
                }
                handler.postDelayed(() -> {
                    synchronized(lock) {
                        if(recoveryGeneration!=bluetoothOnRecoveryGeneration)
                            return;
                        try {
                            if(adapter==null||!adapter.isEnabled())
                                return;
                        }
                        catch(Throwable error) {
                            Log.stack(LOG_ID,"verify Bluetooth after STATE_ON settle",error);
                            return;
                        }
                        bluetoothRestartSettleUntilMillis=0L;
                    }
                    Log.i(LOG_ID,"Bluetooth STATE_ON settle complete; rebuilding BLE mirror operations");
                    configurationChanged();
                },BLUETOOTH_ON_SETTLE_MILLIS);
            }
            else if(state==BluetoothAdapter.STATE_TURNING_OFF||
                    state==BluetoothAdapter.STATE_OFF) {
                synchronized(lock) {
                    ++bluetoothOnRecoveryGeneration;
                    bluetoothRestartSettleUntilMillis=0L;
                    stopAllConnectionsLocked("Bluetooth is switched off");
                    advertiser=null;
                    scanner=null;
                    advertiseCursor=0;
                    advertiseRestartNotBeforeMillis=0;
                    advertiseRestartScheduled=false;
                }
            }
        }
    };

    private static final ScanCallback scanCallback=new ScanCallback() {
        @Override public void onScanResult(int callbackType,ScanResult result) {
            foundDevice(result);
        }

        @Override public void onBatchScanResults(List<ScanResult> results) {
            for(ScanResult result:results)
                foundDevice(result);
        }

        @Override public void onScanFailed(int errorCode) {
            synchronized(lock) {
                scanRunning=false;
                scanLowLatency=false;
                setClientStatusesLocked("BLE scan failed: "+errorCode);
                Log.e(LOG_ID,"scan failed: "+errorCode);
                final long config=configurationGeneration;
                handler.postDelayed(() -> retryOperations(config),RETRY_MILLIS);
            }
        }
    };

    private static boolean currentServerLocked(long generation) {
        return generation==serverGeneration&&gattServer!=null;
    }

    private static String serverConnectionKey(String address,Link link) {
        // Shared API-21/22 service binds to a Link only after /blehello.
        return link==null?address:address+"#"+link.hostIndex;
    }

    private static boolean sameAddress(Connection connection,String address) {
        return connection!=null&&address.equals(safeAddress(connection.device));
    }

    private static boolean hasServerSessionForAddressLocked(String address) {
        for(Connection connection:serverConnections.values())
            if(connection.alive&&sameAddress(connection,address))
                return true;
        return false;
    }

    private static ServerEndpoint serverEndpointLocked(BluetoothGattCharacteristic characteristic) {
        if(characteristic==null)
            return null;
        final ServerEndpoint byCharacteristic=serverCharacteristics.get(characteristic.getUuid());
        if(byCharacteristic!=null)
            return byCharacteristic;
        // Current characteristics are globally unique per token. Never infer
        // an unknown characteristic from its parent service: Android 5 can
        // report the wrong parent when several GATT services coexist.
        return null;
    }

    private static Connection serverConnectionLocked(BluetoothDevice device,ServerEndpoint endpoint,
            boolean create) {
        if(device==null||endpoint==null||!currentServerEndpointLocked(endpoint))
            return null;
        final String address=safeAddress(device);
        final String key=serverConnectionKey(address,endpoint.link);
        Connection connection=serverConnections.get(key);
        if(connection!=null||!create)
            return connection;
        if(serverConnections.size()>=MAX_PENDING_SERVER_CONNECTIONS)
            return null;
        connection=new Connection(true,device,endpoint.link);
        connection.serverTx=endpoint.tx;
        connection.mtu=Math.max(23,serverDeviceMtu.getOrDefault(address,23));
        serverConnections.put(key,connection);
        scheduleHandshakeTimeoutLocked(connection);
        if(endpoint.link==null)
            Log.i(LOG_ID,"opened shared-service BLE server session awaiting authenticated /blehello: "+
                    "address="+address+" service="+endpoint.service.getUuid());
        else
            Log.i(LOG_ID,"opened logical BLE server session: label="+endpoint.link.label+
                    " address="+address+" service="+endpoint.service.getUuid());
        return connection;
    }

    private static void closeServerSessionsForAddressLocked(String address,String reason) {
        final ArrayList<Connection> closing=new ArrayList<>();
        for(Connection connection:serverConnections.values())
            if(sameAddress(connection,address))
                closing.add(connection);
        for(Connection connection:closing)
            disconnectConnectionLocked(connection,reason,false);
    }

    private static BluetoothGattServerCallback newServerCallback(final long generation) {
        return new BluetoothGattServerCallback() {
            @Override @SuppressLint("MissingPermission") public void onServiceAdded(int statusCode,
                    BluetoothGattService service) {
                synchronized(lock) {
                    if(!currentServerLocked(generation))
                        return;
                    final ServerEndpoint endpoint=serverEndpoints.get(service.getUuid());
                    if(statusCode!=BluetoothGatt.GATT_SUCCESS||endpoint==null) {
                        final String reason="Adding BLE mirror service failed: "+statusCode+
                                " uuid="+service.getUuid();
                        if(endpoint!=null) {
                            dropFailedServerEndpointLocked(endpoint,reason);
                            addNextServerServiceLocked(generation);
                        }
                        else
                            Log.e(LOG_ID,reason+"; callback did not match a known endpoint");
                        return;
                    }
                    endpoint.ready=true;
                    if(serverServiceQueue.peek()==endpoint)
                        serverServiceQueue.poll();
                    else
                        serverServiceQueue.remove(endpoint);
                    Log.i(LOG_ID,endpoint.link==null?
                            "GATT mirror retained service added: uuid="+service.getUuid():
                            "GATT mirror service added: label="+endpoint.link.label+
                                    " uuid="+service.getUuid()+" active="+endpoint.active);
                    addNextServerServiceLocked(generation);
                }
            }

            @Override @SuppressLint("MissingPermission") public void onConnectionStateChange(
                    BluetoothDevice device,int statusCode,int newState) {
                final String address=safeAddress(device);
                Log.i(LOG_ID,"server connection state: status="+statusCode+" state="+newState+
                        " address="+address+" (logical mirror selected by service UUID or authenticated /blehello)");
                synchronized(lock) {
                    if(!currentServerLocked(generation))
                        return;
                    if(newState==BluetoothProfile.STATE_CONNECTED&&statusCode==BluetoothGatt.GATT_SUCCESS) {
                        if(legacySingleGattLocked()) {
                            final Connection occupied=legacyOccupiedConnectionLocked();
                            if(occupied!=null&&!sameAddress(occupied,address)&&
                                    (!occupied.serverSide||!occupied.ready)) {
                                // Keep serializing an outgoing/central operation, or an
                                // incoming connection that has not authenticated yet.  But
                                // multiple authenticated centrals may share this one GATT
                                // server; J5 reaches STATE_CONNECTED for the second device
                                // before the old code cancelled it.
                                Log.i(LOG_ID,"rejecting concurrent Android 5 BLE GATT device while exclusive slot is busy: address="+
                                        address+" occupiedBy="+connectionLabel(occupied)+
                                        " serverSide="+occupied.serverSide+" ready="+occupied.ready);
                                try { gattServer.cancelConnection(device); } catch(Throwable ignored) {}
                                maybeScheduleLegacySlotCheckLocked(occupied);
                                return;
                            }
                            if(occupied!=null&&!sameAddress(occupied,address)&&
                                    occupied.serverSide&&occupied.ready)
                                Log.i(LOG_ID,"accepting additional Android 5 incoming BLE GATT device alongside authenticated server peer: address="+
                                        address+" existing="+connectionLabel(occupied));
                        }
                        serverDeviceMtu.put(address,23);
                        // openGattServer() immediately reports devices that were
                        // already physically connected for other apps/GATT
                        // clients.  Before our token-addressed services are ready
                        // those callbacks cannot be a new mirror handshake.
                        // Holding discovery for every such inherited connection
                        // caused a newly reversed third Juggluco instance to wait
                        // 12 seconds before it could even advertise.
                        if(serverServiceReady) {
                            beginServerHandshakeHoldLocked(address,generation);
                            stopScanLocked();
                            // Keep the current advertiser stable while the central
                            // discovers services and writes the CCCD. Restarting it
                            // here was observed to make that descriptor callback
                            // disappear on another phone.
                            Log.i(LOG_ID,"server accepted physical GATT device; holding discovery scheduling while awaiting token-addressed service handshake: "+address);
                        }
                        else
                            Log.i(LOG_ID,"server inherited already-connected GATT device while services are being built; no handshake hold: "+address);
                    }
                    else if(newState==BluetoothProfile.STATE_DISCONNECTED) {
                        // Preserve this before finishServerHandshakeHoldLocked(),
                        // which deliberately removes the marker.
                        final boolean expiredUnauthenticated=
                                expiredUnauthenticatedServerAddresses.contains(address);
                        finishServerHandshakeHoldLocked(address,"physical disconnect");
                        closeServerSessionsForAddressLocked(address,"BLE mirror disconnected (status "+statusCode+")");
                        serverDeviceMtu.remove(address);
                        if(expiredUnauthenticated) {
                            // Huawei M5 evidence: while an unrelated/unauthenticated
                            // physical LE connection occupied the controller limit,
                            // HCI LE Set Advertising Enable returned 0x09 even though
                            // BluetoothLeAdvertiser subsequently called onStartSuccess.
                            // Consequently advertising==true is not proof that packets
                            // are on air. Once that physical resource is released,
                            // invalidate the framework bookkeeping and issue a fresh
                            // stop/start so a waiting S5-class central can see us now,
                            // instead of waiting for Android's next private-address
                            // rotation to accidentally restart the advertiser.
                            Log.w(LOG_ID,"unauthenticated physical BLE link released; hard-restarting mirror advertising: address="+
                                    address+" advertised="+(advertisedLink==null?"none":advertisedLink.label));
                            stopAdvertisingLocked();
                            startAdvertisingLocked();
                        }
                        else
                            startAdvertisingLocked();
                        // If this device disconnected before authenticating, the
                        // handshake hold has been released early. The timeout
                        // will no longer reconcile operations, so make sure any
                        // other client-role mirrors resume scanning now.
                        refreshScanLocked();
                    }
                }
            }

            @Override public void onMtuChanged(BluetoothDevice device,int newMtu) {
                synchronized(lock) {
                    if(!currentServerLocked(generation))
                        return;
                    final String address=safeAddress(device);
                    final int mtu=Math.max(23,newMtu);
                    serverDeviceMtu.put(address,mtu);
                    for(Connection connection:serverConnections.values())
                        if(sameAddress(connection,address))
                            connection.mtu=mtu;
                    Log.i(LOG_ID,"server MTU changed: mtu="+mtu+" address="+address);
                }
            }

            @Override @SuppressLint("MissingPermission") public void onDescriptorWriteRequest(
                    BluetoothDevice device,int requestId,BluetoothGattDescriptor descriptor,
                    boolean preparedWrite,boolean responseNeeded,int offset,byte[] value) {
                synchronized(lock) {
                    if(!currentServerLocked(generation))
                        return;
                    final ServerEndpoint endpoint=descriptor==null?null:
                            serverEndpointLocked(descriptor.getCharacteristic());
                    final Connection connection=serverConnectionLocked(device,endpoint,true);
                    final boolean correctDevice=connection!=null&&connection.alive;
                    final boolean correctDescriptor=CCCD_UUID.equals(descriptor.getUuid())&&
                            endpoint!=null&&endpoint.isTx(descriptor.getCharacteristic());
                    final boolean correctValue=Arrays.equals(value,
                            BluetoothGattDescriptor.ENABLE_INDICATION_VALUE);
                    final boolean accepted=correctDevice&&correctDescriptor&&!preparedWrite&&offset==0&&correctValue;
                    if(accepted) {
                        connection.subscribed=true;
                        // Use the exact TX characteristic the remote subscribed
                        // to (token-specific on modern Android, fixed on API 21-22).
                        connection.serverTx=descriptor.getCharacteristic();
                    }
                    if(responseNeeded)
                        gattServer.sendResponse(device,requestId,accepted?BluetoothGatt.GATT_SUCCESS:
                                BluetoothGatt.GATT_REQUEST_NOT_SUPPORTED,offset,null);
                    Log.i(LOG_ID,"server indication subscription: accepted="+accepted+
                            " label="+(connection==null?"unknown":connectionLabel(connection))+
                            " service="+(endpoint==null?"unknown":endpoint.service.getUuid())+
                            " address="+safeAddress(device));
                }
            }

            @Override @SuppressLint("MissingPermission") public void onCharacteristicWriteRequest(
                    BluetoothDevice device,int requestId,BluetoothGattCharacteristic characteristic,
                    boolean preparedWrite,boolean responseNeeded,int offset,byte[] value) {
                synchronized(lock) {
                    if(!currentServerLocked(generation))
                        return;
                    final ServerEndpoint endpoint=serverEndpointLocked(characteristic);
                    final Connection connection=serverConnectionLocked(device,endpoint,true);
                    final String initialPath=connection!=null&&!connection.ready?
                            BleMirrorCodec.singleFragmentPath(value):null;
                    final boolean helloFragment=connection!=null&&!connection.ready&&
                            (connection.decoder.inProgress()||
                                    BleMirrorCodec.firstFragmentCanBePath(value,HELLO_PATH));
                    final boolean staleUnauthenticatedTraffic=connection!=null&&connection.alive&&
                            !connection.ready&&!helloFragment;
                    final boolean accepted=connection!=null&&connection.alive&&endpoint!=null&&
                            endpoint.isRx(characteristic)&&!preparedWrite&&offset==0&&value!=null&&
                            !staleUnauthenticatedTraffic;
                    if(responseNeeded)
                        gattServer.sendResponse(device,requestId,accepted?BluetoothGatt.GATT_SUCCESS:
                                BluetoothGatt.GATT_REQUEST_NOT_SUPPORTED,offset,null);
                    Log.i(LOG_ID,"onCharacteristicWriteRequest: accepted="+accepted+" label="+
                            (connection==null?"unknown":connectionLabel(connection))+" valueBytes="+
                            (value==null?-1:value.length)+" responseNeeded="+responseNeeded+
                            " service="+(endpoint==null?"unknown":endpoint.service.getUuid())+
                            " address="+safeAddress(device)+
                            (staleUnauthenticatedTraffic?" staleUnauthenticatedPath="+initialPath:""));
                    if(staleUnauthenticatedTraffic) {
                        // A client can retain a physical BluetoothGatt after this
                        // process has lost/rebuilt its logical authenticated session.
                        // Successful ATT writes would otherwise convince the client
                        // that the link is alive for many minutes even though this
                        // server cannot decrypt them.  Rejecting the write makes the
                        // existing client failure path close and reconnect, which
                        // immediately starts a fresh /blehello handshake.
                        Log.w(LOG_ID,"rejecting stale unauthenticated BLE traffic; fresh /blehello required: label="+
                                connectionLabel(connection)+" path="+initialPath+
                                " address="+safeAddress(device));
                    }
                    else if(accepted)
                        receivedFragment(connection,value);
                }
            }

            @Override public void onNotificationSent(BluetoothDevice device,int statusCode) {
                final Connection connection;
                synchronized(lock) {
                    final String address=safeAddress(device);
                    connection=currentServerLocked(generation)&&gattOperationConnection!=null&&
                            gattOperationConnection.serverSide&&sameAddress(gattOperationConnection,address)?
                            gattOperationConnection:null;
                }
                Log.i(LOG_ID,"onNotificationSent: status="+statusCode+" label="+
                        (connection==null?"unknown":connectionLabel(connection))+" address="+
                        safeAddress(device));
                if(connection!=null)
                    fragmentFinished(connection,statusCode);
            }
        };
    }

    @SuppressLint("MissingPermission")
    private static void discoverServices(Connection connection) {
        final long epoch;
        synchronized(lock) {
            if(!connection.alive||connection.gatt==null||connection.servicesRequested)
                return;
            connection.servicesRequested=true;
            epoch=connection.epoch;
            if(!connection.gatt.discoverServices()) {
                connection.servicesRequested=false;
                handler.post(() -> disconnectForFailure(connection,"BLE service discovery was rejected"));
                return;
            }
        }
        handler.postDelayed(() -> {
            synchronized(lock) {
                if(!connection.alive||connection.epoch!=epoch||!connection.servicesRequested||
                        connection.gatt==null)
                    return;
                connection.servicesRequested=false;
                final Link link=connection.link;
                if(link!=null)
                    // Do not get trapped permanently in the alternate default-MTU
                    // bootstrap. The next attempt should negotiate MTU normally.
                    link.skipInitialMtu=false;
                final boolean refreshed=refreshGattCacheLocked(connection);
                final String reason="BLE service discovery timed out; GATT cache refresh "+
                        (refreshed?"accepted":"unavailable");
                Log.e(LOG_ID,reason+" label="+connectionLabel(connection)+
                        " address="+safeAddress(connection.device));
                disconnectConnectionLocked(connection,reason,true);
                final long configuration=configurationGeneration;
                handler.postDelayed(() -> retryOperations(configuration),
                        CACHE_REFRESH_SETTLE_MILLIS);
                reconcileOperationsLocked();
            }
        },SERVICE_DISCOVERY_TIMEOUT_MILLIS);
    }

    /**
     * Recover an already-authenticated client after Android reports that the
     * remote GATT database changed.  Multiple Juggluco processes on one Android
     * device share one controller/GATT database, so another process opening or
     * rebuilding its server can invalidate this client's cached CCCD handles.
     * Client writes may keep succeeding while incoming indications silently stop.
     *
     * Keep the authenticated Juggluco session and native mirror socket intact;
     * only rediscover attributes and re-enable the indication CCCD.
     */
    private static void refreshAuthenticatedClientServices(Connection connection) {
        synchronized(lock) {
            if(!connection.alive||connection.serverSide||connection.gatt==null||!connection.ready)
                return;
            if(connection.serviceRefreshInProgress) {
                Log.i(LOG_ID,"authenticated GATT service refresh already in progress: label="+
                        connectionLabel(connection));
                return;
            }
            connection.serviceRefreshInProgress=true;
            connection.servicesRequested=false;
            connection.subscriptionRequested=false;
            // Stop starting new queued messages, but keep the currently cached
            // characteristics until any already-submitted client write has
            // delivered its callback. Android may report Service Changed while
            // such a write is in flight. Clearing rx here would make
            // onCharacteristicWrite() ignore that callback and leave
            // connection.sending stuck until FRAGMENT_TIMEOUT_MILLIS.
            connection.subscribed=false;
            connection.link.status="BLE connected; refreshing GATT services "+connection.link.label;
            Log.w(LOG_ID,"refreshing authenticated BLE GATT services and CCCD: label="+
                    connectionLabel(connection)+" address="+safeAddress(connection.device));
        }

        // Do not issue discoverServices from inside the Service Changed callback
        // itself.  A short post also lets any just-completing characteristic write
        // leave Android's one-operation-at-a-time GATT queue first.
        handler.postDelayed(() -> {
            synchronized(lock) {
                if(!connection.alive||!connection.ready||!connection.serviceRefreshInProgress||
                        connection.gatt==null)
                    return;
                // If an outgoing write was already in flight when Service Changed
                // arrived, wait for its callback before starting discovery.
                if(connection.sending!=null) {
                    handler.postDelayed(() -> refreshAuthenticatedClientServicesContinue(connection),
                            100L);
                    return;
                }
            }
            refreshAuthenticatedClientServicesContinue(connection);
        },100L);
    }

    private static void refreshAuthenticatedClientServicesContinue(Connection connection) {
        synchronized(lock) {
            if(!connection.alive||!connection.ready||!connection.serviceRefreshInProgress||
                    connection.gatt==null)
                return;
            if(connection.sending!=null) {
                handler.postDelayed(() -> refreshAuthenticatedClientServicesContinue(connection),
                        100L);
                return;
            }
            // No Android GATT operation is outstanding now. Only at this point
            // invalidate the old handles. If Service Changed arrived during a
            // write, its callback has already been processed by
            // onCharacteristicWrite(); a failed write therefore reconnects
            // immediately instead of becoming an artificial 15-second timeout.
            connection.rx=null;
            connection.tx=null;
            connection.servicesRequested=false;
        }
        discoverServices(connection);
    }

    @SuppressWarnings("deprecation")
    @SuppressLint("MissingPermission")
    private static void subscribeClient(Connection connection) {
        synchronized(lock) {
            if(!connection.alive||connection.gatt==null||connection.link==null)
                return;
            if(!connection.currentGattProtocol) {
                handler.post(() -> disconnectForFailure(connection,
                        "Legacy BLE mirror GATT endpoint is no longer accepted"));
                return;
            }
            final UUID tokenServiceUuid=discoveryUuid(connection.link.discoveryToken);
            BluetoothGattService service;
            BluetoothGattCharacteristic rx;
            BluetoothGattCharacteristic tx;
            boolean sharedService=connection.android5SharedGattProtocol;
            if(sharedService) {
                service=connection.gatt.getService(ANDROID5_SERVICE_UUID);
                rx=service==null?null:service.getCharacteristic(ANDROID5_RX_UUID);
                tx=service==null?null:service.getCharacteristic(ANDROID5_TX_UUID);
            }
            else {
                service=connection.gatt.getService(tokenServiceUuid);
                rx=service==null?null:
                        service.getCharacteristic(rxUuid(connection.link.discoveryToken));
                tx=service==null?null:
                        service.getCharacteristic(txUuid(connection.link.discoveryToken));
                if(service==null||rx==null||tx==null) {
                    // Compatibility with the immediately preceding build which
                    // advertised d20 but used a fixed d20 service on Android 5.
                    service=connection.gatt.getService(SERVICE_UUID);
                    rx=service==null?null:service.getCharacteristic(RX_UUID);
                    tx=service==null?null:service.getCharacteristic(TX_UUID);
                    sharedService=service!=null&&rx!=null&&tx!=null;
                }
            }
            if(service==null||rx==null||tx==null) {
                final boolean refreshed=refreshGattCacheLocked(connection);
                final String reason="current BLE GATT endpoint missing; cache refresh "+
                        (refreshed?"accepted":"unavailable");
                Log.w(LOG_ID,reason+" label="+connectionLabel(connection)+
                        " tokenService="+tokenServiceUuid+" android5SharedService="+
                        ANDROID5_SERVICE_UUID+" previousSharedService="+SERVICE_UUID);
                disconnectConnectionLocked(connection,reason,true);
                final long configuration=configurationGeneration;
                handler.postDelayed(() -> retryOperations(configuration),
                        CACHE_REFRESH_SETTLE_MILLIS);
                reconcileOperationsLocked();
                return;
            }
            connection.rx=rx;
            connection.tx=tx;
            Log.i(LOG_ID,"selected BLE GATT endpoint: label="+connectionLabel(connection)+
                    " protocol="+(connection.android5SharedGattProtocol?"v16-android5-shared":
                            (sharedService?"v15-previous-shared":"v15-token"))+
                    " service="+service.getUuid()+" rx="+rx.getUuid()+" tx="+tx.getUuid());
            if(!connection.gatt.setCharacteristicNotification(connection.tx,true)) {
                handler.post(() -> disconnectForFailure(connection,
                        "BLE mirror characteristics are missing"));
                return;
            }
            final BluetoothGattDescriptor cccd=tx.getDescriptor(CCCD_UUID);
            if(cccd==null) {
                handler.post(() -> {
                    synchronized(lock) {
                        if(!connection.alive)
                            return;
                        final boolean refreshed=refreshGattCacheLocked(connection);
                        final String reason="BLE mirror indication descriptor is missing; GATT cache refresh "+
                                (refreshed?"accepted":"unavailable");
                        Log.w(LOG_ID,reason+" label="+connectionLabel(connection)+
                                " address="+safeAddress(connection.device));
                        disconnectConnectionLocked(connection,reason,true);
                        final long configuration=configurationGeneration;
                        handler.postDelayed(() -> retryOperations(configuration),
                                CACHE_REFRESH_SETTLE_MILLIS);
                        reconcileOperationsLocked();
                    }
                });
                return;
            }
            final boolean accepted;
            connection.subscriptionRequested=true;
            if(Build.VERSION.SDK_INT>=33)
                accepted=connection.gatt.writeDescriptor(cccd,
                        BluetoothGattDescriptor.ENABLE_INDICATION_VALUE)==BluetoothGatt.GATT_SUCCESS;
            else {
                cccd.setValue(BluetoothGattDescriptor.ENABLE_INDICATION_VALUE);
                accepted=connection.gatt.writeDescriptor(cccd);
            }
            if(!accepted) {
                connection.subscriptionRequested=false;
                handler.post(() -> disconnectForFailure(connection,
                        "Could not enable BLE mirror indications"));
            }
            else {
                connection.link.status="BLE connected; enabling indications "+connection.link.label;
                Log.i(LOG_ID,"CCCD write accepted; waiting for descriptor callback: label="+
                        connectionLabel(connection));
                scheduleSubscriptionTimeoutLocked(connection);
            }
        }
    }

    private static void sendHello(Connection connection) {
        synchronized(lock) {
            if(!connection.alive||connection.serverSide||!connection.subscribed||
                    connection.link==null||connection.clientNonce!=null)
                return;
            try {
                connection.clientNonce=BleMirrorSecurity.randomNonce(random);
                final byte[] hello=BleMirrorSecurity.hello(connection.link.masterKey,
                        connection.link.label,Applic.isWearable,connection.link.hostIndex,
                        connection.clientNonce);
                Log.i(LOG_ID,"sending authenticated BLE mirror hello: index="+
                        connection.link.hostIndex+" label="+connection.link.label);
                if(enqueueLocked(connection,HELLO_PATH,hello)==null)
                    handler.post(() -> disconnectForFailure(connection,
                            "Could not queue BLE mirror authentication"));
            }
            catch(GeneralSecurityException error) {
                Log.stack(LOG_ID,"create BLE hello",error);
                handler.post(() -> disconnectForFailure(connection,
                        "Could not create BLE mirror authentication"));
            }
        }
    }

    private static BluetoothGattCallback newClientCallback(final Connection connection) {
        return new BluetoothGattCallback() {
            @Override @SuppressLint("MissingPermission") public void onConnectionStateChange(
                    BluetoothGatt gatt,int statusCode,int newState) {
                Log.i(LOG_ID,"client connection state: label="+connectionLabel(connection)+
                        " status="+statusCode+" state="+newState+" address="+
                        safeAddress(gatt.getDevice()));
                if(newState==BluetoothProfile.STATE_CONNECTED&&statusCode==BluetoothGatt.GATT_SUCCESS) {
                    synchronized(lock) {
                        if(!connection.alive||gatt!=connection.gatt)
                            return;
                        connection.connected=true;
                        connection.link.concurrentClientFailures=0;
                        connection.link.status="BLE connected; authenticating "+connection.link.label;
                        connection.mtu=23;
                        // Do not force CONNECTION_PRIORITY_HIGH for ordinary phone/tablet
                        // mirrors. Android's GATT implementation can couple that request to
                        // aggressive connection parameters and a short supervision timeout.
                        // With several concurrent BLE links this was previously observed to
                        // destabilize otherwise healthy phone-phone/tablet links and leave
                        // repeated status-133 reconnect failures. Wear links still benefit
                        // from the low-latency request.
                        if(Applic.isWearable||connection.link.wearOS) {
                            try { gatt.requestConnectionPriority(BluetoothGatt.CONNECTION_PRIORITY_HIGH); }
                            catch(Throwable ignored) {}
                        }
                        else
                            Log.i(LOG_ID,"keeping default BLE connection parameters for phone/tablet mirror: label="+
                                    connection.link.label);
                        if(connection.link.skipInitialMtu) {
                            // This is a one-shot escape from a previously wedged MTU ATT
                            // procedure.  Keeping it forever proved just as bad on S5: for
                            // one Realme peer discoverServices() then stopped returning on
                            // every reconnect until some unrelated radio/network event
                            // changed the controller state.  Alternate back to the normal
                            // MTU-first bootstrap after this single default-MTU attempt.
                            connection.link.skipInitialMtu=false;
                            Log.i(LOG_ID,"skipping initial MTU negotiation once after earlier timeout: label="+
                                    connection.link.label);
                            discoverServices(connection);
                        }
                        else if(!gatt.requestMtu(REQUESTED_MTU))
                            discoverServices(connection);
                        else {
                            connection.mtuRequested=true;
                            final long epoch=connection.epoch;
                            handler.postDelayed(() -> {
                                synchronized(lock) {
                                    if(!connection.alive||connection.epoch!=epoch||connection.ready||
                                            connection.servicesRequested||!connection.mtuRequested||
                                            connection.gatt!=gatt)
                                        return;
                                    connection.mtuRequested=false;
                                    connection.link.skipInitialMtu=true;
                                    final String reason="Initial BLE MTU negotiation timed out; retrying with default MTU";
                                    Log.w(LOG_ID,reason+": label="+connection.link.label+
                                            " address="+safeAddress(connection.device));
                                    disconnectConnectionLocked(connection,reason,true);
                                    final long configuration=configurationGeneration;
                                    handler.postDelayed(() -> retryOperations(configuration),500L);
                                    reconcileOperationsLocked();
                                }
                            },INITIAL_MTU_TIMEOUT_MILLIS);
                        }
                    }
                }
                else if(newState==BluetoothProfile.STATE_DISCONNECTED) {
                    synchronized(lock) {
                        if(connection.alive&&gatt==connection.gatt) {
                            final Link link=connection.link;
                            final boolean neverConnected=!connection.connected;
                            // If Android drops a physically connected link while the
                            // first MTU ATT procedure is still outstanding, remember
                            // that this peer/controller combination cannot safely use
                            // MTU-first bootstrap. The next connection starts service
                            // discovery immediately at the default MTU instead of
                            // repeating the same 30-second status-22 failure.
                            if(neverConnected&&link!=null&&statusCode==133&&
                                    fragileMultiClientGattLocked()&&
                                    otherReadyClientConnectionLocked(link)!=null) {
                                ++link.concurrentClientFailures;
                                Log.w(LOG_ID,"GATT status 133 while another authenticated client is active; "+
                                        "protecting its indication path with extended retry backoff: label="+
                                        link.label+" failures="+link.concurrentClientFailures);
                            }
                            else if(neverConnected&&link!=null&&statusCode!=133)
                                link.concurrentClientFailures=0;
                            if(connection.connected&&connection.mtuRequested&&
                                    !connection.servicesRequested&&link!=null) {
                                connection.mtuRequested=false;
                                link.skipInitialMtu=true;
                                Log.w(LOG_ID,"BLE disconnected with initial MTU request still pending; "+
                                        "using default MTU on next attempt: label="+link.label+
                                        " status="+statusCode);
                            }
                            disconnectConnectionLocked(connection,"BLE mirror disconnected (status "+
                                    statusCode+")",false);
                            if(neverConnected&&link!=null) {
                                if(link.directionProven) {
                                    // A connection which has already authenticated in this
                                    // persisted direction is never allowed to infer a role
                                    // change from Android GATT errors. Retry the same role.
                                    link.transientClientFailures=0;
                                    link.oppositeRoleNotBeforeMillis=Math.max(
                                            link.oppositeRoleNotBeforeMillis,
                                            monotonicMillis()+PREFERRED_ROLE_TRY_MILLIS);
                                    Log.w(LOG_ID,"GATT connect failed on proven Bluetooth direction; keeping client role: label="+
                                            link.label+" status="+statusCode);
                                }
                                else if(statusCode==GATT_STATUS_CONNECTION_ESTABLISHMENT_FAILED) {
                                    // 0x3E is a physical connection-resource conflict, not
                                    // evidence that an unproven role choice is wrong.
                                    link.transientClientFailures=0;
                                    link.oppositeRoleNotBeforeMillis=Math.max(
                                            link.oppositeRoleNotBeforeMillis,
                                            monotonicMillis()+PREFERRED_ROLE_TRY_MILLIS);
                                    Log.w(LOG_ID,"GATT status 62/0x3E is a physical connection establishment conflict; keeping preferred client role: label="+
                                            link.label);
                                }
                                else {
                                    ++link.transientClientFailures;
                                    final boolean retrySameRole=statusCode==133&&
                                            link.transientClientFailures<
                                                    CLIENT_ESTABLISHMENT_FAILURES_BEFORE_REVERSE;
                                    if(retrySameRole)
                                        Log.w(LOG_ID,"transient GATT status 133 on unproven connection; retrying client before role reversal: label="+
                                                link.label+" attempt="+link.transientClientFailures);
                                    else
                                        activateReverseGattRoleLocked(link,
                                                "GATT connect failed before STATE_CONNECTED (status "+
                                                        statusCode+")");
                                }
                            }
                            reconcileOperationsLocked();
                        }
                    }
                }
            }

            @Override public void onMtuChanged(BluetoothGatt gatt,int newMtu,int statusCode) {
                Log.i(LOG_ID,"MTU result: label="+connectionLabel(connection)+" status="+
                        statusCode+" mtu="+newMtu);
                synchronized(lock) {
                    if(connection.alive&&gatt==connection.gatt) {
                        connection.mtuRequested=false;
                        if(statusCode==BluetoothGatt.GATT_SUCCESS)
                            connection.mtu=Math.max(23,newMtu);
                    }
                }
                discoverServices(connection);
            }

            @Override public void onServicesDiscovered(BluetoothGatt gatt,int statusCode) {
                Log.i(LOG_ID,"services discovered: label="+connectionLabel(connection)+
                        " status="+statusCode);
                synchronized(lock) {
                    if(!connection.alive||gatt!=connection.gatt)
                        return;
                    connection.servicesRequested=false;
                }
                if(statusCode==BluetoothGatt.GATT_SUCCESS)
                    subscribeClient(connection);
                else
                    disconnectForFailure(connection,"BLE service discovery failed: "+statusCode);
            }

            @Override public void onServiceChanged(BluetoothGatt gatt) {
                final boolean authenticated;
                synchronized(lock) {
                    if(!connection.alive||gatt!=connection.gatt)
                        return;
                    authenticated=connection.ready;
                }
                if(authenticated) {
                    // Service Changed invalidates cached attribute/CCCD handles.
                    // Keeping them (v9) leaves a deceptive half-working link:
                    // client writes such as /bleprobe still succeed, and the
                    // server even reports successful onNotificationSent(), but
                    // this client no longer receives the indications.  Rediscover
                    // and re-subscribe without dropping the authenticated mirror
                    // session or its native carrier.
                    refreshAuthenticatedClientServices(connection);
                    return;
                }
                Log.i(LOG_ID,"remote GATT services changed during setup: label="+
                        connectionLabel(connection)+" address="+safeAddress(gatt.getDevice()));
                disconnectForFailure(connection,"BLE mirror service changed during setup; reconnecting");
            }

            @Override public void onDescriptorWrite(BluetoothGatt gatt,
                    BluetoothGattDescriptor descriptor,int statusCode) {
                Log.i(LOG_ID,"descriptor written: label="+connectionLabel(connection)+
                        " status="+statusCode+" uuid="+descriptor.getUuid());
                synchronized(lock) {
                    if(!connection.alive||gatt!=connection.gatt)
                        return;
                    connection.subscriptionRequested=false;
                }
                if(CCCD_UUID.equals(descriptor.getUuid())&&statusCode==BluetoothGatt.GATT_SUCCESS) {
                    final boolean alreadyReady;
                    synchronized(lock) {
                        connection.subscribed=true;
                        alreadyReady=connection.ready;
                        if(connection.serviceRefreshInProgress)
                            connection.serviceRefreshInProgress=false;
                    }
                    if(alreadyReady) {
                        Log.i(LOG_ID,"authenticated BLE GATT service/CCCD refresh complete: label="+
                                connectionLabel(connection)+" address="+safeAddress(gatt.getDevice()));
                        synchronized(lock) {
                            if(connection.alive) {
                                connection.lastActivityMillis=monotonicMillis();
                                sendNextLocked(connection);
                                if(connection.bidirectionalProbeSupported&&
                                        enqueueProbeRequestLocked(connection))
                                    Log.i(LOG_ID,"verifying BLE reverse indication path after CCCD refresh: label="+
                                            connectionLabel(connection));
                                scheduleProbeLocked(connection,PROBE_IDLE_MILLIS);
                            }
                        }
                    }
                    else
                        sendHello(connection);
                }
                else if(CCCD_UUID.equals(descriptor.getUuid())&&
                        (statusCode==1/* GATT_INVALID_HANDLE */||
                         statusCode==BluetoothGatt.GATT_REQUEST_NOT_SUPPORTED)) {
                    synchronized(lock) {
                        if(!connection.alive||gatt!=connection.gatt)
                            return;
                        connection.serviceRefreshInProgress=false;
                        /*
                         * A stale Android GATT cache does not always surface as
                         * GATT_INVALID_HANDLE.  XA1 against the Android-5 J5
                         * discovered the correct token service but wrote its
                         * CCCD using an old ATT handle.  J5 therefore received
                         * the descriptor request on another token service and
                         * replied GATT_REQUEST_NOT_SUPPORTED (6).  Refresh the
                         * cache for both status values and retry the same,
                         * already-proven physical role.
                         */
                        final boolean refreshed=refreshGattCacheLocked(connection);
                        final String reason="BLE indication CCCD uses stale/invalid GATT handle "+
                                "(status "+statusCode+"); cache refresh "+
                                (refreshed?"accepted":"unavailable");
                        Log.w(LOG_ID,reason+" label="+connectionLabel(connection)+
                                " address="+safeAddress(gatt.getDevice()));
                        disconnectConnectionLocked(connection,reason,true);
                        final long configuration=configurationGeneration;
                        handler.postDelayed(() -> retryOperations(configuration),
                                CACHE_REFRESH_SETTLE_MILLIS);
                        reconcileOperationsLocked();
                    }
                }
                else
                    disconnectForFailure(connection,"BLE indication setup failed: "+statusCode);
            }

            @Override public void onCharacteristicWrite(BluetoothGatt gatt,
                    BluetoothGattCharacteristic characteristic,int statusCode) {
                Log.i(LOG_ID,"onCharacteristicWrite: label="+connectionLabel(connection)+
                        " status="+statusCode+" uuid="+characteristic.getUuid());
                synchronized(lock) {
                    if(!connection.alive||gatt!=connection.gatt)
                        return;
                }
                synchronized(lock) {
                    if(gattOperationConnection!=connection)
                        return;
                }
                fragmentFinished(connection,statusCode);
            }

            @Override @SuppressWarnings("deprecation") public void onCharacteristicChanged(
                    BluetoothGatt gatt,BluetoothGattCharacteristic characteristic) {
                synchronized(lock) {
                    if(!connection.alive||gatt!=connection.gatt)
                        return;
                }
                if(Build.VERSION.SDK_INT<33&&connection.tx!=null&&
                        connection.tx.getUuid().equals(characteristic.getUuid())) {
                    final byte[] value=characteristic.getValue();
                    Log.i(LOG_ID,"onCharacteristicChanged: label="+connectionLabel(connection)+
                            " valueBytes="+(value==null?-1:value.length));
                    receivedFragment(connection,value);
                }
            }

            @Override public void onCharacteristicChanged(BluetoothGatt gatt,
                    BluetoothGattCharacteristic characteristic,byte[] value) {
                synchronized(lock) {
                    if(!connection.alive||gatt!=connection.gatt)
                        return;
                }
                if(connection.tx!=null&&connection.tx.getUuid().equals(characteristic.getUuid())) {
                    Log.i(LOG_ID,"onCharacteristicChanged: label="+connectionLabel(connection)+
                            " valueBytes="+(value==null?-1:value.length));
                    receivedFragment(connection,value);
                }
            }
        };
    }

    private static final class Config {
        final int hostIndex;
        final String label;
        final String normalizedLabel;
        final String password;
        final boolean side;
        final int role;
        final boolean pairReverse;
        final boolean directionProven;
        final boolean wearOS;
        final int transport;
        byte[] masterKey;
        byte[] discoveryToken;
        String error;

        Config(int hostIndex,String label,String password,boolean side,boolean client,boolean pairReverse,boolean directionProven,boolean wearOS,int transport) {
            this.hostIndex=hostIndex;
            this.label=label==null?"":label;
            this.normalizedLabel=this.label.trim();
            this.password=password;
            this.side=side;
            this.role=client?ROLE_CLIENT:ROLE_SERVER;
            this.pairReverse=pairReverse;
            this.directionProven=directionProven;
            this.wearOS=wearOS;
            this.transport=transport;
            if(normalizedLabel.isEmpty())
                error="Bluetooth mirror requires a connection label";
            else if(this.label.getBytes(java.nio.charset.StandardCharsets.UTF_8).length>255)
                error="Bluetooth mirror connection label is too long";
            else if(password==null||password.isEmpty())
                error="Bluetooth mirror requires a mirror password";
        }

        void invalidate(String reason) {
            if(error==null)
                error=reason;
        }

        void finishSecurity() {
            if(error!=null)
                return;
            try {
                masterKey=BleMirrorSecurity.masterKey(label,password);
                discoveryToken=BleMirrorSecurity.discoveryToken(masterKey);
            }
            catch(GeneralSecurityException securityError) {
                error="Could not initialize Bluetooth mirror security";
                Log.stack(LOG_ID,"initialize security for "+label,securityError);
            }
        }
    }

    private static final class Link {
        final int hostIndex;
        final String label;
        final boolean side;
        int role;
        // Persisted pair-wide direction. Keep this separate from the current
        // in-memory role: a preferred server/client may already have the right
        // local role while the persisted pair direction is still stale.
        boolean persistedPairReverse;
        // Existing rows are proven by default. Only rows created by this
        // version begin unproven and may automatically experiment with the
        // opposite physical GATT direction. Once authentication succeeds this
        // becomes true permanently.
        boolean directionProven;
        final boolean wearOS;
        final int transport;
        final byte[] masterKey;
        final byte[] discoveryToken;
        final String configError;
        volatile String status;
        boolean carrierActive;
        // Automatic TCP/Message failure is a persistent demand for the BLE
        // carrier.  Discovery on API 21/22 can legitimately take longer than
        // the old 20-second MessageSender job because another peer may own the
        // single GATT lease first.  Keep this true until BLE is selected or a
        // real TCP success explicitly cancels the fallback.
        boolean automaticFallbackRequested;
        // Invalidates delayed no-peer recovery jobs across separate Automatic
        // fallback requests.
        long automaticFallbackGeneration;
        boolean newTcpCandidates;
        long discoveryBoostUntilMillis;
        long lastDiscoveryLogMillis;
        long clientRetryNotBeforeMillis;
        long oppositeRoleNotBeforeMillis;
        long legacySlotYieldUntilMillis;
        long legacySlotLeaseUntilMillis;
        // A scheduler-induced Android-5 slot handoff must not forget that this
        // Automatic mirror had already selected BLE as its data carrier.  The
        // next authenticated turn reissues /messages=true and resumes native
        // synchronization instead of becoming an idle standby connection.
        boolean legacyResumeCarrier;
        // A single status-133 before STATE_CONNECTED is often transient.  Do
        // not permanently flip an otherwise usable client to server after one
        // such failure; genuine establishment timeouts still reverse at once.
        int transientClientFailures;
        // Status-133 failures of a second client while another authenticated
        // client remains healthy. Old Android stacks need a much slower retry
        // cadence here to avoid corrupting the first client's indication path.
        int concurrentClientFailures;
        // SDK reported by the authenticated peer over /blecaps. It is runtime-only
        // and used solely to avoid asking another old stack to become the central
        // when balancing XA1's one reliable outgoing-client slot.
        int remoteSdk;
        // Runtime resource-role balancing. These never rewrite the persisted pair
        // direction; they exist only until this process restarts/reconfigures.
        boolean resourceReverseGattRole;
        boolean resourceReverseClientRequested;
        boolean resourceRoleSwapPending;
        long resourceRoleSwapGeneration;
        // Some Android stacks wedge a second GATT client during the initial MTU
        // exchange. Once observed for this mirror, reconnect and bootstrap at
        // the default MTU instead of repeating the same deadlock.
        boolean skipInitialMtu;
        // Consecutive ADVERTISE_FAILED_TOO_MANY_ADVERTISERS results for this
        // logical link; temporary role experiments use it for diagnostics.
        int advertiseResourceFailures;
        // Runtime-only opposite physical role. It becomes the persisted
        // preferred role only after an authenticated session proves it works.
        boolean reverseGattRole;
        // Runtime-only lease for an unproven reversed-server experiment.
        long temporaryReverseUntilMillis;
        long temporaryReverseGeneration;
        Connection connection;

        Link(Config config) {
            hostIndex=config.hostIndex;
            label=config.label;
            side=config.side;
            role=config.role;
            persistedPairReverse=config.pairReverse;
            directionProven=config.directionProven;
            wearOS=config.wearOS;
            transport=config.transport;
            masterKey=config.masterKey;
            discoveryToken=config.discoveryToken;
            configError=config.error;
            oppositeRoleNotBeforeMillis=monotonicMillis()+PREFERRED_ROLE_TRY_MILLIS;
            if(transport==TRANSPORT_BLUETOOTH)
                discoveryBoostUntilMillis=monotonicMillis()+DISCOVERY_BOOST_MILLIS;
            status=config.error==null?
                    (transport==TRANSPORT_AUTOMATIC?
                            (role==ROLE_CLIENT?"Looking for automatic Bluetooth fallback ":
                                    "Offering automatic Bluetooth fallback "):
                            (role==ROLE_CLIENT?"Looking for direct Bluetooth mirror ":
                                    "Offering direct Bluetooth mirror "))+label:config.error;
        }

        boolean matches(Config config) {
            return hostIndex==config.hostIndex&&label.equals(config.label)&&side==config.side&&
                    role==config.role&&persistedPairReverse==config.pairReverse&&
                    directionProven==config.directionProven&&
                    wearOS==config.wearOS&&transport==config.transport&&
                    equalsBytes(masterKey,config.masterKey)&&
                    equalsBytes(discoveryToken,config.discoveryToken)&&
                    (configError==null?config.error==null:configError.equals(config.error));
        }

        private static boolean equalsBytes(byte[] one,byte[] two) {
            return one==null?two==null:Arrays.equals(one,two);
        }
    }

    private static final class ServerEndpoint {
        // The ATT service/handles live for the lifetime of this BluetoothGattServer.
        // A configuration refresh may re-bind the same token service to a new Link
        // object without deleting/re-adding the service and renumbering handles.
        Link link;
        final BluetoothGattService service;
        final BluetoothGattCharacteristic rx;
        final BluetoothGattCharacteristic tx;
        boolean ready;
        boolean active=true;

        ServerEndpoint(Link link,BluetoothGattService service,BluetoothGattCharacteristic rx,
                BluetoothGattCharacteristic tx) {
            this.link=link;
            this.service=service;
            this.rx=rx;
            this.tx=tx;
        }

        boolean isRx(BluetoothGattCharacteristic characteristic) {
            return characteristic!=null&&rx.getUuid().equals(characteristic.getUuid());
        }

        boolean isTx(BluetoothGattCharacteristic characteristic) {
            return characteristic!=null&&tx.getUuid().equals(characteristic.getUuid());
        }
    }

    private static final class Connection {
        final boolean serverSide;
        final BluetoothDevice device;
        final long generation=++nextConnectionGeneration;
        final BleMirrorCodec.Decoder decoder=new BleMirrorCodec.Decoder();
        final ArrayDeque<Outbound> outgoing=new ArrayDeque<>();
        Link link;
        BluetoothGatt gatt;
        BluetoothGattCharacteristic rx;
        BluetoothGattCharacteristic tx;
        BluetoothGattCharacteristic serverTx;
        // Set when discovery matched one of the accepted current UUID namespaces
        // (d20 token service or d30 Android-5 shared service).
        boolean currentGattProtocol;
        boolean android5SharedGattProtocol;
        boolean alive=true;
        boolean subscribed;
        boolean subscriptionRequested;
        boolean servicesRequested;
        boolean serviceRefreshInProgress;
        boolean connected;
        boolean ready;
        boolean remoteIsWearable;
        int remoteHostIndex=-1;
        int mtu=23;
        boolean mtuRequested;
        int fragmentSession=1+random.nextInt(0xfffe);
        int queuedBytes;
        long epoch=1;
        long lastActivityMillis;
        long lastPayloadActivityMillis;
        long lastFragmentActivityMillis;
        long inboundGeneration;
        long probeRequestBaselineInboundGeneration;
        long probeReplyGeneration;
        boolean probeReplyPending;
        boolean bidirectionalProbeSupported;
        int reversePathProbeMisses;
        boolean legacySlotCheckScheduled;
        long sendCounter;
        long receiveCounter;
        boolean probeScheduled;
        long probeIdleTargetMillis;
        boolean serverLivenessScheduled;
        byte[] clientNonce;
        byte[] serverNonce;
        byte[] sessionKey;
        byte[] lastAddressPayload;
        Outbound sending;

        Connection(boolean serverSide,BluetoothDevice device,Link link) {
            this.serverSide=serverSide;
            this.device=device;
            this.link=link;
            connected=serverSide;
        }
    }

    private static final class Outbound {
        final String path;
        final List<byte[]> fragments;
        final int bytes;
        final CountDownLatch finished=new CountDownLatch(1);
        int index;
        volatile boolean success;
        volatile long lastProgressMillis=monotonicMillis();

        Outbound(String path,List<byte[]> fragments,int bytes) {
            this.path=path;
            this.fragments=fragments;
            this.bytes=bytes;
        }
    }
}
