/*
 * Experimental direct legacy Connect IQ transport for older Garmin watches
 * (tested/protocol-derived from vivoactive 3 + Garmin Connect 5.28.1).
 *
 * Garmin Connect Mobile can still share the underlying notification stream.
 * GarminSdkContext enforces receive ownership inside Juggluco; the public Mobile
 * SDK cannot turn off Garmin Connect's separate Bluetooth client.
 */
package tk.glucodata.nums;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.security.SecureRandom;
import java.lang.reflect.Method;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.Queue;
import java.util.Set;
import java.util.UUID;

import tk.glucodata.Log;

import static tk.glucodata.Log.doLog;

final class DirectGarminIQ {
    private static final String LOG_ID = "DirectGarminIQ";

    // Legacy phone<->watch application-message channel used by the vivoactive 3.
    // Garmin Connect writes every 00xx/F1xx Connect-IQ packet to 4c80 and receives
    // the corresponding replies/return traffic as notifications on cd28.
    private static final UUID SERVICE = UUID.fromString("6a4e8022-667b-11e3-949a-0800200c9a66");
    private static final UUID WRITE   = UUID.fromString("6a4e4c80-667b-11e3-949a-0800200c9a66");
    private static final UUID NOTIFY  = UUID.fromString("6a4ecd28-667b-11e3-949a-0800200c9a66");
    private static final UUID CCCD    = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb");

    // Legacy VA3 packets contain 19 payload bytes after the connection/sequence byte.
    private static final int CHUNK = 19;
    private static final long TIMEOUT_MS = 15000L;
    // Android's fixed-ATT-bearer response watchdog is 30 seconds.  Closing this
    // BluetoothGatt before that deadline unregisters the client and cancels the
    // native watchdog, leaving a shared but poisoned Garmin ACL alive.  Keep an
    // unanswered service-discovery/CCCD request registered long enough for the
    // Bluetooth stack to tear down that bearer, with margin for its callback.
    static final long ATT_RESPONSE_TIMEOUT_MS = 38000L;
    // AOSP retries an all-primary-services discovery twice at the same 30-second
    // deadline before disconnecting.  A stall before the cached Garmin service
    // is returned therefore needs a separate, longer guard.
    static final long ATT_DISCOVERY_TIMEOUT_MS = 98000L;
    private static final long OPEN_TIMEOUT_MS = 3500L;
    private static final long TAKEOVER_SETTLE_MS = 450L;
    private static final long APP_INFO_TIMEOUT_MS = 900L;
    private static final long APP_OPEN_TIMEOUT_MS = 900L;
    // garmin17 captured Garmin Connect receiving a complete Kerfstok STOP but
    // failing to acknowledge F101 after two F106 requests produced two XXTEA
    // keys. Kerfstok's short watch-to-phone transfers use legacy connection 1,
    // sequence 2. Completing that stranded return transfer releases
    // Communications.transmit() without taking application ownership.
    static final long STRANDED_RETURN_CLOSE_TIMEOUT_MS = 650L;
    private static final int KERFSTOK_REPLY_CONNECTION_ID = 1;
    private static final int KERFSTOK_SHORT_REPLY_END_SEQUENCE = 2;
    // Eight-number Libre3 returns encrypt to 56 bytes: three data packets.
    // noconnect(1).tar ends one such return at F101/03 without any final ACK.
    private static final int KERFSTOK_LIBRE3_RETURN_END_SEQUENCE = 3;

    static final int LINK_PROBE_RESPONSIVE = 1;
    static final int LINK_PROBE_RESET = 2;
    static final int LINK_PROBE_FAILED = 3;
    static final int LINK_PROBE_RETURN_RELEASED = 4;

    private final Context context;
    private final AllData owner;
    private final long peerId;
    // The compatible applications share one physical Garmin link.
    private final List<String> appIds;
    private final byte[][] appIdBytes;
    private final boolean[] appInfoTried;
    private int appIndex;
    private byte[] appId;
    private String appIdHex;
    // Only this query can complete startup. Other app-info replies are
    // installation evidence for Garmin Status.
    private int pendingAppInfo=-1;
    private final String requestedAddress;
    // A link-recovery probe joins the same physical Garmin GATT connection.  It
    // performs service discovery and the idempotent cd28 CCCD write, then sends
    // only the final ACK/close response for a stranded watch->phone reply.  It
    // never opens an application channel, supplies an XXTEA key, or sends a
    // Kerfstok object, so Garmin Connect keeps application ownership.
    private final boolean linkProbeOnly;
    private final Handler handler = new Handler(Looper.getMainLooper());
    private final SecureRandom random = new SecureRandom();
    private final Queue<WriteOp> writes = new ArrayDeque<>();
    private static final class MessageOp {
        final Object message;
        final boolean expectReply;
        final int numbersGeneration;
        final String requiredAppId;
        MessageOp(Object message, boolean expectReply,int numbersGeneration,String requiredAppId) {
            this.message=message;
            this.expectReply=expectReply;
            this.numbersGeneration=numbersGeneration;
            this.requiredAppId=requiredAppId;
        }
    }
    private final ArrayDeque<MessageOp> messages = new ArrayDeque<>();
    private final Queue<BluetoothGattDescriptor> setupDescriptors = new ArrayDeque<>();

    private static final class WriteOp {
        final byte[] value;
        final boolean withResponse;
        final Runnable onSubmitted;
        WriteOp(byte[] value, boolean withResponse) {
            this(value,withResponse,null);
        }
        WriteOp(byte[] value, boolean withResponse,Runnable onSubmitted) {
            this.value=value;
            this.withResponse=withResponse;
            this.onSubmitted=onSubmitted;
        }
    }

    private BluetoothGatt gatt;
    private BluetoothGattCharacteristic writeCharacteristic;
    private BluetoothGattCharacteristic notifyCharacteristic;
    private WriteOp activeWrite;
    private boolean writeBusy;
    private boolean ready;
    private boolean setupAttRequestPending;
    // Handler delays use uptime, not elapsed time. Check this deadline again at
    // Reinit/escalation so a sleeping phone cannot preserve an expired setup.
    private long connectionSetupDeadlineElapsed;
    private String connectionSetupStage;
    // When Automatic hands a still-GCM-connected peer to Direct BLE, Garmin
    // Connect has already subscribed the remote cd28 CCCD. Writing the same
    // CCCD from a second Android GATT client can stall on older watches (VA3).
    // setCharacteristicNotification() is still needed for this local client;
    // only the redundant remote descriptor write is skipped.
    private boolean reuseExistingCccd;
    // A CCCD on a previous physical connection is not proof of subscription
    // after Bluetooth/ACL loss, even when we retain the same Direct object.
    private boolean notificationSubscriptionLost;
    // When Garmin Connect shares the physical bearer, a fire-and-forget glucose
    // transfer deliberately leaves Kerfstok's redundant [229] return channel to
    // Garmin Connect. Ignore those immediate incoming opens so Direct does not
    // send a competing F105 key.
    private long ignoreSharedGlucoseReplyUntil;
    private int setupTimeoutGeneration;
    private int consecutiveSetupFailures;
    // One hard recovery per failure episode. Garmin Connect performs the hidden
    // BluetoothGatt.refresh() after a clean-data device setup; it clears Android
    //'s cached GATT service database for the physical watch.
    private boolean gattCacheRefreshAttempted;
    // While old handles are invalid, ignore application traffic from the old
    // subscription. It must not arm a protocol timeout over native discovery.
    private boolean gattCacheRediscovery;
    private boolean stopped;
    private int generation;
    private int openToken;
    private int openAttempt;
    private int recoveryConnectionId;
    private int appLaunchToken;
    private boolean appLaunchOpenSent;
    // User Reinit can re-run the Kerfstok application bootstrap on the existing
    // GATT connection. This avoids disconnect/reconnect contention with Garmin Connect.
    private boolean reinitRequested;
    // Set only by an explicit user Reinit. Ordinary startup/reconnect must never
    // issue Connect-IQ openApplication (00 06), because that prompts the watch.
    private boolean launchRequested;
    private boolean ownerReadyNotified;
    private boolean applicationPaused;
    // applicationPaused is also used for the brief Automatic Garmin-Connect
    // health probe.  That pause is different from Kerfstok STOP: while probing,
    // Garmin Connect must own an incoming reply channel; after STOP, Direct must
    // still accept a watch-initiated START so a rebooted/reopened Kerfstok cannot
    // leave Communications.transmit() blocked forever.
    private boolean garminConnectProbePaused;
    private long reconnectDelayMs=1500L;
    private int reconnectToken;
    private boolean reconnectPending;
    private boolean bluetoothReceiverRegistered;
    private boolean gracefulStopRequested;
    private Runnable gracefulStopCallback;
    private int gracefulStopToken;

    // One application transaction is kept active until its watch->phone reply closes.
    private MessageOp currentOp;
    private Object currentMessage;
    private byte[] outgoingCipher;
    private byte[] outgoingKey;
    private int outgoingOffset;
    private int outgoingNextSequence;
    // ACKs have no transaction identifier. Accept only the expected command and
    // sequence, and only after its triggering packet was submitted to Android.
    // In particular, a queued F101 must not make another client's END ACK ours.
    private WriteOp outgoingAckWrite;
    private int outgoingAckType=-1;
    private int outgoingAckSequence=-1;
    private boolean outgoingAckSent;
    private int connectionId;
    private int incomingConnectionId;
    private boolean applicationTransaction;
    private enum OutgoingState { IDLE, OPENING, RECOVERING, KEY, START, WINDOW, END, REPLY }
    private OutgoingState outgoingState=OutgoingState.IDLE;

    // A phone restart can lose the final ACK for a watch-initiated transfer
    // while Garmin Connect keeps the physical link alive. Recover only after
    // our START has been delivered but cannot get a reply. This is transport
    // cleanup, never an application acknowledgement of glucose/history.
    private boolean strandedStartupRecoveryAttempted;
    private int strandedStartupConnectionId;
    private int strandedStartupToken;
    private WriteOp strandedStartupAckWrite;

    private byte[] incomingKey;
    // Garmin Connect can have a second Android GATT client subscribed to the same
    // legacy cd28 notification stream.  For watch->phone transfers both clients
    // otherwise race to answer F106 with different F105 XXTEA keys.  The VA3
    // latches the first key it receives.  Pre-arm ours immediately after accepting
    // the 00 03 incoming application connection, before F106 is emitted.
    private int incomingLength = -1;
    private ByteArrayOutputStream incomingCipher;
    private int incomingNextSequence;
    // After acknowledging a complete 0..14 receive window, the watch can
    // retransmit sequence 14 before it observes our F102/01 ACK.  Remember
    // that exact packet so the retransmission can be ACKed again instead of
    // being mistaken for sequence 14 of the next window.
    private byte[] incomingLastWindowTail;
    // Garmin Connect can be subscribed to the same legacy application channel while
    // Direct BLE is active. The VA3 can then retransmit the same F101 end packet.
    // Remember completion until 00 04 so one application reply reaches AllData once.
    private boolean incomingCompleted;
    private int incomingCompletedConnectionId;
    private int incomingCompletedEndSeq=-1;
    private int incomingCompletedCrc=-1;
    private int linkRecoveryToken;
    private int linkRecoveryConnectionId=KERFSTOK_REPLY_CONNECTION_ID;

    DirectGarminIQ(Context context, AllData owner, String appIdHex, List<String> appIds,
                   String address, long peerId) {
        this(context,owner,appIdHex,appIds,address,peerId,false);
    }

    private DirectGarminIQ(Context context, AllData owner, String selectedAppIdHex,
                           List<String> appIds, String address, long peerId,
                           boolean linkProbeOnly) {
        this.context = context.getApplicationContext();
        this.owner = owner;
        this.peerId = peerId;
        this.appIds = new ArrayList<>(appIds);
        appIdBytes = new byte[appIds.size()][];
        appInfoTried = new boolean[appIds.size()];
        for(int i=0;i<appIds.size();i++) {
            appIdBytes[i]=GarminIQCodec.hex(appIds.get(i));
            if(appIdBytes[i].length!=16) throw new IllegalArgumentException("Garmin app id must be 16 bytes");
        }
        appIndex=appIds.indexOf(selectedAppIdHex);
        if(appIndex<0) throw new IllegalArgumentException("Unknown Garmin app id");
        appId=appIdBytes[appIndex];
        appIdHex=selectedAppIdHex;
        this.requestedAddress = address;
        this.linkProbeOnly = linkProbeOnly;
    }

    static DirectGarminIQ linkProbe(Context context, AllData owner, String appIdHex,
                                    String address, long peerId) {
        return new DirectGarminIQ(context,owner,appIdHex,Collections.singletonList(appIdHex),address,peerId,true);
    }

    boolean isActive() { return !stopped; }
    boolean isReady() { return ready; }
    // isActive() also includes a disconnected transport waiting for its retry.
    // An expired setup is not protected from an explicit reset just because a
    // Handler callback has not run yet (e.g. while the phone was asleep).
    boolean isConnectionSetupPending() {
        return !stopped && !ready && gatt!=null && !connectionSetupExpired();
    }
    private boolean connectionSetupExpired() {
        return connectionSetupDeadlineElapsed!=0 &&
                SystemClock.elapsedRealtime()>=connectionSetupDeadlineElapsed;
    }
    boolean hasPendingApplicationLaunch() { return launchRequested; }
    boolean isLinkProbe() { return linkProbeOnly; }
    boolean hasPendingMessages() { return currentOp!=null || !messages.isEmpty(); }

    // Use only for an Automatic takeover while Garmin Connect still owns the
    // physical watch connection. The remote CCCD is then already enabled by
    // Garmin Connect; duplicating that descriptor write can block on VA3.
    void reuseExistingNotificationSubscription() { reuseExistingCccd=true; }

    // STOP is an intentional app close, not a failed handshake. Keep listening
    // for a later watch START, retain history, and cancel glucose/start retries.
    /** Pause only application-level ownership for a one-shot Garmin Connect
     * health probe. This must be non-destructive: unlike pauseApplication(), it
     * does not close/reset Direct's current Connect-IQ protocol state. Call only
     * when isIdleForGarminConnectProbe() is true. */
    boolean isIdleForGarminConnectProbe() {
        return !stopped && ready && currentOp==null && messages.isEmpty() && writes.isEmpty() &&
                outgoingState==OutgoingState.IDLE && connectionId==0 && incomingConnectionId==0 &&
                !applicationTransaction;
    }

    void pauseForGarminConnectProbe() {
        handler.post(() -> {
            if(stopped) return;
            garminConnectProbePaused=true;
            applicationPaused=true;
        });
    }

    void pauseApplication() {
        handler.post(() -> {
            if(stopped) return;
            cancelStrandedStartupRecovery();
            garminConnectProbePaused=false;
            applicationPaused=true;
            resetAppLaunchState();
            ownerReadyNotified=ready;
            generation++;
            openToken++;
            recoveryConnectionId=0;
            if(currentOp!=null && !isApplicationStateMessage(currentOp.message)) messages.addFirst(currentOp);
            java.util.Iterator<MessageOp> iter=messages.iterator();
            while(iter.hasNext()) if(isApplicationStateMessage(iter.next().message)) iter.remove();
            java.util.Iterator<WriteOp> pendingWrites=writes.iterator();
            while(pendingWrites.hasNext()) {
                byte[] value=pendingWrites.next().value;
                if(value.length>1 && value[0]==0 && (value[1]==1 || value[1]==5 || value[1]==6)) pendingWrites.remove();
            }
            if(connectionId!=0) queueWrite(new byte[]{0,2,(byte)connectionId});
            currentOp=null;
            currentMessage=null;
            outgoingKey=null;
            outgoingCipher=null;
            connectionId=0;
            outgoingState=OutgoingState.IDLE;
            // Preserve the incoming STOP's ACK and close handshake. Clearing the
            // entire protocol here would discard the reply to the closing watch.
            applicationTransaction=incomingConnectionId!=0;
            if(applicationTransaction) armTimeout("close after Kerfstok STOP");
        });
    }

    private static boolean isNumberRole(Object message) {
        return message instanceof List<?> && !((List<?>)message).isEmpty() &&
                Integer.valueOf(consts.consts.NUMBERROLE).equals(((List<?>)message).get(0));
    }

    private static boolean isStart(Object message) {
        return message instanceof List<?> && !((List<?>)message).isEmpty() &&
                Integer.valueOf(consts.consts.START).equals(((List<?>)message).get(0));
    }

    private static boolean isLibre3ReturnAck(Object message) {
        if(!(message instanceof List<?>) || ((List<?>)message).size()<2) return false;
        Object kind=((List<?>)message).get(0);
        return Integer.valueOf(236).equals(kind) || Integer.valueOf(238).equals(kind);
    }

    private static boolean isGlucose(Object message) {
        return message instanceof List<?> && !((List<?>)message).isEmpty() &&
                Integer.valueOf(consts.consts.GLUCOSE).equals(((List<?>)message).get(0));
    }

    /** True only for Automatic takeover of a still Garmin-Connect-attached watch. */
    boolean shouldAvoidGlucoseReplyRace() {
        return reuseExistingCccd && !linkProbeOnly;
    }

    private static boolean isApplicationStateMessage(Object message) {
        if(!(message instanceof List<?>)) return false;
        List<?> li=(List<?>)message;
        return !li.isEmpty() && (Integer.valueOf(consts.consts.GLUCOSE).equals(li.get(0)) ||
                Integer.valueOf(consts.consts.START).equals(li.get(0)) ||
                Integer.valueOf(consts.consts.NUMBERROLE).equals(li.get(0)));
    }

    void resumeApplication() {
        handler.post(() -> {
            if(stopped) return;
            garminConnectProbePaused=false;
            if(!applicationPaused) return;
            applicationPaused=false;
            // START/role/glucose actions may resume an existing Kerfstok session,
            // but they must never launch the watch app. Reinit is the sole launcher.
            startNextIfPossible();
        });
    }

    /** The app has just returned a record that AllData accepted. Unlike a
     * generic retry, this proves the selected app is communicating again even
     * if START was lost or a reconnect occurred while STOP-paused. */
    void resumeAfterWatchReturn() {
        handler.post(() -> {
            if(stopped) return;
            garminConnectProbePaused=false;
            applicationPaused=false;
            if(ready) {
                // A paused reconnect may never have completed app-info startup.
                // Reopen the send gate without launching the app. The current
                // incoming ACK/close still completes before queued outgoing data.
                appLaunchToken++;
                ownerReadyNotified=true;
            }
            startNextIfPossible();
        });
    }

    // Must be called before start(); AllData invokes this only for an explicit
    // user Reinit. The request is one-shot and is not re-armed by BLE reconnects.
    void launchApplicationOnInitialConnection() {
        launchRequested=true;
    }

    /** Explicit Reinit bypasses disconnected retry backoff immediately. Preserve
     * an existing GATT setup or a working link; never start a second client while
     * service discovery/CCCD setup is pending. On a ready link, finish the current
     * application transaction before getApplicationInfo/openApplication. The
     * one-shot launch request survives a connection failure until it is sent. */
    void reinitApplication() {
        handler.post(() -> {
            if(stopped || !owner.isGarminActive(peerId)) return;
            launchRequested=true;
            reinitRequested=true;
            garminConnectProbePaused=false;
            applicationPaused=false;
            if(strandedStartupConnectionId==0) strandedStartupRecoveryAttempted=false;
            // An explicit button press must not wait for a queued 60-second
            // reconnect (which may itself have been delayed while asleep).
            reconnectToken++;
            reconnectPending=false;
            reconnectDelayMs=1500L;
            if(gatt==null) {
                log("Reinit: reconnect Direct Garmin now; bypass retry backoff peer="+peerId);
                connect();
                return;
            }
            if(!ready) {
                if(connectionSetupExpired()) {
                    log("Reinit: reset expired Direct Bluetooth setup peer="+peerId+
                            " stage="+connectionSetupStage);
                    connectionFailure("Reinit: expired Bluetooth setup "+connectionSetupStage);
                    // connectionFailure normally schedules backoff. This is the
                    // user's button press: cancel it and reconnect immediately.
                    reconnectToken++;
                    reconnectPending=false;
                    reconnectDelayMs=1500L;
                    connect();
                } else {
                    log("Reinit: keep pending Direct Bluetooth setup peer="+peerId+
                            " stage="+connectionSetupStage+" remainingMs="+
                            Math.max(0L,connectionSetupDeadlineElapsed-SystemClock.elapsedRealtime()));
                }
                return;
            }
            if(!recoverStrandedStartupReply()) startNextIfPossible();
        });
    }

    void start() {
        stopped = false;
        gracefulStopRequested=false;
        registerBluetoothReceiver();
        handler.post(new Runnable() { @Override public void run() { connect(); }});
    }

    /**
     * A Bluetooth service restart invalidates every BluetoothGatt client, but
     * some Android versions never deliver the corresponding GATT disconnect
     * callback. Listen for the adapter/physical-link broadcasts as an
     * independent recovery signal so a stale "ready" Direct connection cannot
     * prevent watch-initiated Libre 3 return messages from reaching Juggluco.
     */
    private final BroadcastReceiver bluetoothReceiver=new BroadcastReceiver() {
        @Override public void onReceive(Context ignored,Intent intent) {
            if(intent==null) return;
            final String action=intent.getAction();
            if(BluetoothAdapter.ACTION_STATE_CHANGED.equals(action)) {
                final int state=intent.getIntExtra(BluetoothAdapter.EXTRA_STATE,BluetoothAdapter.ERROR);
                handler.post(() -> onBluetoothAdapterState(state));
                return;
            }
            if(BluetoothDevice.ACTION_ACL_DISCONNECTED.equals(action)) {
                final BluetoothDevice device=intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
                if(matchesRequestedDevice(device))
                    handler.post(() -> onBluetoothTransportLost("watch ACL disconnected"));
            }
        }
    };

    private boolean matchesRequestedDevice(BluetoothDevice device) {
        if(device==null) return false;
        try {
            if(requestedAddress!=null && !requestedAddress.isEmpty())
                return requestedAddress.equalsIgnoreCase(device.getAddress());
            final BluetoothGatt active=gatt;
            return active!=null && active.getDevice()!=null &&
                    active.getDevice().getAddress().equalsIgnoreCase(device.getAddress());
        } catch(Throwable ignored) { return false; }
    }

    private void registerBluetoothReceiver() {
        if(bluetoothReceiverRegistered) return;
        try {
            final IntentFilter filter=new IntentFilter(BluetoothAdapter.ACTION_STATE_CHANGED);
            filter.addAction(BluetoothDevice.ACTION_ACL_DISCONNECTED);
            // This receiver contains only protected system broadcasts, so it
            // needs no exported/not-exported flag on Android 14 and remains
            // source-compatible with Juggluco builds using an older SDK.
            context.registerReceiver(bluetoothReceiver,filter);
            bluetoothReceiverRegistered=true;
        } catch(Throwable t) {
            log("register Bluetooth recovery receiver: "+t);
        }
    }

    private void unregisterBluetoothReceiver() {
        if(!bluetoothReceiverRegistered) return;
        bluetoothReceiverRegistered=false;
        try { context.unregisterReceiver(bluetoothReceiver); }
        catch(Throwable ignored) {}
    }

    private void onBluetoothAdapterState(int state) {
        if(stopped) return;
        if(state==BluetoothAdapter.STATE_OFF || state==BluetoothAdapter.STATE_TURNING_OFF) {
            onBluetoothTransportLost("Bluetooth adapter state="+state);
        } else if(state==BluetoothAdapter.STATE_ON && gatt==null) {
            log("Bluetooth adapter ON; reconnect Direct Garmin now");
            reconnectToken++;
            reconnectPending=false;
            reconnectDelayMs=1500L;
            connect();
        }
    }

    private void onBluetoothTransportLost(String reason) {
        if(stopped) return;
        notificationSubscriptionLost=true;
        strandedStartupRecoveryAttempted=false;
        if(gatt!=null) connectionFailure(reason);
        else if(!reconnectPending) reconnectLater();
    }

    private boolean protocolIdle() {
        return currentMessage==null && currentOp==null && messages.isEmpty() && !applicationTransaction &&
                incomingConnectionId==0 && writes.isEmpty() && !writeBusy;
    }

    void stopGracefully(Runnable done) {
        handler.post(new Runnable() { @Override public void run() {
            if(stopped) { if(done!=null) done.run(); return; }
            gracefulStopRequested=true;
            gracefulStopCallback=done;
            final int token=++gracefulStopToken;
            checkGracefulStop(token,System.currentTimeMillis()+12000L);
        }});
    }

    private void checkGracefulStop(final int token,final long deadline) {
        if(stopped || token!=gracefulStopToken) return;
        // Queued messages cannot drain before service/CCCD setup completes.
        // Still drain a reverse IQ channel if notifications arrived early on
        // a shared link, before our descriptor callback.
        boolean setupOnly=!ready && !applicationTransaction && incomingConnectionId==0 &&
                currentMessage==null && currentOp==null && writes.isEmpty() && !writeBusy;
        if(setupOnly || protocolIdle() || System.currentTimeMillis()>=deadline) {
            if(!setupOnly && !protocolIdle()) log("graceful stop timeout; force disconnect");
            stop();
            return;
        }
        handler.postDelayed(new Runnable() { @Override public void run() {
            checkGracefulStop(token,deadline);
        }},50L);
    }

    void stop() {
        if(gatt!=null) log("close Direct GATT peer="+peerId+" reason=stop ready="+ready+
                " setup="+connectionSetupStage);
        gattCacheRediscovery=false;
        stopped = true;
        unregisterBluetoothReceiver();
        reconnectToken++;
        reconnectPending=false;
        generation++;
        ready = false;
        writes.clear();
        messages.clear();
        currentOp = null;
        currentMessage = null;
        applicationTransaction = false;
        outgoingState=OutgoingState.IDLE;
        resetIncomingState();
        openToken++;
        recoveryConnectionId=0;
        writeBusy = false;
        activeWrite = null;
        setupAttRequestPending=false;
        cancelSetupTimeout();
        setupDescriptors.clear();
        resetAppLaunchState();
        reinitRequested=false;
        applicationPaused=false;
        garminConnectProbePaused=false;
        final BluetoothGatt old = gatt;
        gatt = null;
        writeCharacteristic = null;
        notifyCharacteristic = null;
        if (old != null) {
            try { old.disconnect(); } catch (Throwable ignored) {}
            try { old.close(); } catch (Throwable ignored) {}
        }
        final Runnable done=gracefulStopCallback;
        gracefulStopCallback=null;
        gracefulStopRequested=false;
        ++gracefulStopToken;
        if(done!=null) done.run();
    }

    /** Queue a normal Connect-IQ object.  The existing AllData application-level
     * acknowledgement remains authoritative; this method only replaces GCM transport. */
    boolean send(Object message) {
        return enqueueMessage(message,true,null);
    }

    /** Send a Connect-IQ object whose application is not expected to reply. */
    boolean sendNoReply(Object message) {
        return sendNoReply(message,null);
    }

    boolean sendNoReply(Object message,String requiredAppId) {
        return enqueueMessage(message,false,requiredAppId);
    }

    private boolean acceptsMessage(MessageOp op) {
        return (op.requiredAppId==null || op.requiredAppId.equals(appIdHex)) &&
                owner.acceptsQueuedGarminMessage(peerId,op.message,op.numbersGeneration);
    }

    private boolean enqueueMessage(Object message, boolean expectReply,String requiredAppId) {
        if (linkProbeOnly || stopped || gracefulStopRequested) return false;
        final int numbersGeneration=owner.numbersGeneration();
        handler.post(new Runnable() {
            @Override public void run() {
                if (stopped) return;
                // A changed Numbers choice invalidates queued history/roles.
                // Prune them now, even when a priority role goes ahead of them.
                java.util.Iterator<MessageOp> pending=messages.iterator();
                while(pending.hasNext()) {
                    MessageOp queued=pending.next();
                    if(!acceptsMessage(queued)) pending.remove();
                }
                // Lifecycle retries retain one command while this watch is
                // offline; ending a sensor invalidates an older queued ADD.
                if(message instanceof java.util.List<?> && !((java.util.List<?>)message).isEmpty() &&
                        Integer.valueOf(240).equals(((java.util.List<?>)message).get(0))) {
                    if(!owner.acceptsQueuedGarminMessage(peerId,message,numbersGeneration)) return;
                    if(currentOp!=null && message.equals(currentOp.message)) return;
                    for(MessageOp queued:messages) if(message.equals(queued.message)) return;
                }
                // A retransmitted Libre3 record can arrive while its previous
                // ACK is already queued or on the air. One successful [236]/[238]
                // is enough to release that exact retained watch record, so do
                // not let retries build an unbounded queue of identical ACKs.
                if(isLibre3ReturnAck(message)) {
                    if(currentOp!=null && message.equals(currentOp.message)) {
                        log("Libre3 return ACK already in flight: "+message);
                        return;
                    }
                    for(MessageOp queued:messages) {
                        if(message.equals(queued.message)) {
                            log("Libre3 return ACK already queued: "+message);
                            return;
                        }
                    }
                }
                MessageOp operation=new MessageOp(message,expectReply,numbersGeneration,requiredAppId);
                // Apply the role before retained history after a START reply.
                // Libre3 ACK priority is selected in startNextIfPossible().
                if(isNumberRole(message) || isStart(message)) messages.addFirst(operation);
                else messages.add(operation);
                if (gatt == null && !reconnectPending) connect();
                startNextIfPossible();
            }
        });
        return true;
    }

    private void connect() {
        if (stopped || reconnectPending || gatt != null) return;
        // Active is a hard per-phone ownership switch.  A DirectGarminIQ may
        // outlive a peer-table rebuild and has its own reconnect timer, so it
        // must not rely solely on AllData having stopped it at the moment the
        // preference changed.
        if(!owner.isGarminActive(peerId)) {
            log("Direct Garmin peer inactive; stop before connect peer="+peerId);
            stop();
            return;
        }
        try {
            final BluetoothManager bm = (BluetoothManager)context.getSystemService(Context.BLUETOOTH_SERVICE);
            final BluetoothAdapter adapter = bm != null ? bm.getAdapter() : BluetoothAdapter.getDefaultAdapter();
            if (adapter == null || !adapter.isEnabled()) {
                connectionFailure("Bluetooth disabled");
                return;
            }
            final BluetoothDevice device = findDevice(adapter);
            if (device == null) {
                fail("No Garmin watch address available");
                return;
            }
            if(device.getBondState()!=BluetoothDevice.BOND_BONDED) {
                fail("Garmin watch is not paired in Android Bluetooth settings");
                return;
            }
            connectGatt(device);
        } catch (Throwable t) {
            connectionFailure("connectGatt: " + t);
        }
    }

    private void connectGatt(BluetoothDevice device) {
        if(stopped || device==null || gatt!=null) return;
        try {
            log((linkProbeOnly?"recovery probe ":"")+"connectGatt " + safeName(device) + " " + device.getAddress());
            if (Build.VERSION.SDK_INT >= 23)
                gatt = device.connectGatt(context, false, callback, BluetoothDevice.TRANSPORT_LE);
            else
                gatt = device.connectGatt(context, false, callback);
            connectionSetupStage="connect";
            connectionSetupDeadlineElapsed=SystemClock.elapsedRealtime()+TIMEOUT_MS;
            armTimeout("connect");
        } catch(Throwable t) {
            connectionFailure("connectGatt: " + t);
        }
    }

    static List<BluetoothDevice> bondedGarmins(Context context) {
        final ArrayList<BluetoothDevice> out=new ArrayList<>();
        try {
            final BluetoothManager bm=(BluetoothManager)context.getApplicationContext().getSystemService(Context.BLUETOOTH_SERVICE);
            final BluetoothAdapter adapter=bm!=null?bm.getAdapter():BluetoothAdapter.getDefaultAdapter();
            if(adapter==null) return out;
            final Set<BluetoothDevice> bonded=adapter.getBondedDevices();
            for(BluetoothDevice d:bonded) {
                final String n=safeName(d).toLowerCase(java.util.Locale.US);
                if(n.contains("garmin") || n.contains("vivoactive") || n.contains("vívoactive") ||
                        n.contains("edge") || n.contains("fenix") || n.contains("forerunner") ||
                        n.contains("venu") || n.contains("instinct") || n.contains("marq"))
                    out.add(d);
            }
        } catch(Throwable t) {
            log("bondedGarmins: "+t);
        }
        return out;
    }

    static String bondedName(BluetoothDevice d) { return safeName(d); }

    private BluetoothDevice findDevice(BluetoothAdapter adapter) {
        if (requestedAddress != null && requestedAddress.length() > 0) {
            try { return adapter.getRemoteDevice(requestedAddress); }
            catch (Throwable t) { log("Bad direct Garmin address " + requestedAddress + ": " + t); }
        }
        final String saved = context.getSharedPreferences("directgarmin", Context.MODE_PRIVATE)
                .getString("address", null);
        if (saved != null) {
            try { return adapter.getRemoteDevice(saved); } catch (Throwable ignored) {}
        }
        final Set<BluetoothDevice> bonded = adapter.getBondedDevices();
        BluetoothDevice only = null;
        int count = 0;
        for (BluetoothDevice d : bonded) {
            count++;
            only = d;
            final String n = safeName(d).toLowerCase(java.util.Locale.US);
            if (n.contains("vivoactive") || n.contains("vívoactive") || n.contains("garmin")) return d;
        }
        return count == 1 ? only : null;
    }

    private static String safeName(BluetoothDevice d) {
        try { final String n=d.getName(); return n==null?"":n; }
        catch (Throwable t) { return ""; }
    }

    private final BluetoothGattCallback callback = new BluetoothGattCallback() {
        @Override public void onConnectionStateChange(BluetoothGatt g, int status, int newState) {
            handler.post(new Runnable() { @Override public void run() {
                if (stopped || g != gatt) {
                    log("ignore stale GATT state peer="+peerId+" status="+status+" new="+newState);
                    return;
                }
                log("state peer="+peerId+" status="+status+" new="+newState+
                        " ready="+ready+" setup="+connectionSetupStage);
                if (status == BluetoothGatt.GATT_SUCCESS && newState == BluetoothProfile.STATE_CONNECTED) {
                    generation++; // cancel the connectGatt deadline
                    setupAttRequestPending=true;
                    armSetupTimeout("discoverServices",ATT_DISCOVERY_TIMEOUT_MS);
                    if (!g.discoverServices()) {
                        setupAttRequestPending=false;
                        connectionFailure("discoverServices returned false");
                    }
                } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                    notificationSubscriptionLost=true;
                    final boolean resetWhileWaiting=setupAttRequestPending;
                    setupAttRequestPending=false;
                    if(linkProbeOnly && resetWhileWaiting)
                        finishLinkProbe(LINK_PROBE_RESET,"ATT watchdog disconnected stalled Garmin bearer status="+status);
                    else connectionFailure("disconnected status="+status);
                } else if (status != BluetoothGatt.GATT_SUCCESS) {
                    connectionFailure("GATT connection status=" + status);
                }
            }});
        }

        @Override public void onServicesDiscovered(BluetoothGatt g, int status) {
            handler.post(new Runnable() { @Override public void run() {
                if (stopped || g != gatt) return;
                cancelSetupTimeout();
                setupAttRequestPending=false;
                if (status != BluetoothGatt.GATT_SUCCESS) { connectionFailure("service discovery status=" + status); return; }
                final BluetoothGattService service = g.getService(SERVICE);
                if (service == null) { fail("Garmin legacy 6a4e8022 service missing"); return; }
                writeCharacteristic = service.getCharacteristic(WRITE);
                notifyCharacteristic = service.getCharacteristic(NOTIFY);
                if (writeCharacteristic == null || notifyCharacteristic == null) {
                    fail("Garmin legacy 4c80/cd28 characteristic missing");
                    return;
                }
                log("4c80 properties=0x" + Integer.toHexString(writeCharacteristic.getProperties()) +
                        " cd28 properties=0x" + Integer.toHexString(notifyCharacteristic.getProperties()));
                writeCharacteristic.setWriteType(BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE);

                setupDescriptors.clear();
                if (!g.setCharacteristicNotification(notifyCharacteristic, true)) {
                    connectionFailure("setCharacteristicNotification cd28 failed");
                    return;
                }
                if(reuseExistingCccd && !notificationSubscriptionLost && !linkProbeOnly) {
                    log("reuse Garmin Connect cd28 CCCD subscription");
                    finishNotificationSetup(g);
                    return;
                }
                final BluetoothGattDescriptor cccd = notifyCharacteristic.getDescriptor(CCCD);
                if (cccd == null) { fail("Garmin cd28 notification CCCD missing"); return; }
                cccd.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
                setupDescriptors.add(cccd);
                writeNextSetupDescriptor(g);
            }});
        }

        @Override public void onDescriptorWrite(BluetoothGatt g, BluetoothGattDescriptor descriptor, int status) {
            handler.post(new Runnable() { @Override public void run() {
                if (stopped || g != gatt) return;
                cancelSetupTimeout();
                setupAttRequestPending=false;
                if (status != BluetoothGatt.GATT_SUCCESS) { connectionFailure("CCCD write status=" + status + " uuid=" + descriptor.getCharacteristic().getUuid()); return; }
                log("CCCD enabled " + descriptor.getCharacteristic().getUuid());
                if (!setupDescriptors.isEmpty()) {
                    writeNextSetupDescriptor(g);
                    return;
                }
                finishNotificationSetup(g);
            }});
        }

        private void finishNotificationSetup(BluetoothGatt g) {
            setupAttRequestPending=false;
            cancelSetupTimeout();
            notificationSubscriptionLost=false;
            gattCacheRediscovery=false;
            if(linkProbeOnly) {
                generation++;
                beginStrandedReturnRecovery();
                return;
            }
            ready = true;
            consecutiveSetupFailures=0;
            try {
                context.getSharedPreferences("directgarmin", Context.MODE_PRIVATE).edit()
                        .putString("address", g.getDevice().getAddress()).apply();
            } catch (Throwable ignored) {}
            generation++;
            log("ready v58 Reinit discovery recovery peer="+peerId);
            if(applicationPaused)
                finishDirectReady();
            else
                beginApplicationInfo();
        }

        @Override public void onCharacteristicWrite(BluetoothGatt g, BluetoothGattCharacteristic characteristic, int status) {
            handler.post(new Runnable() { @Override public void run() {
                if (stopped || g != gatt || gattCacheRediscovery) return;
                if (status != BluetoothGatt.GATT_SUCCESS) {
                    writeBusy=false;
                    activeWrite=null;
                    connectionFailure("characteristic write status=" + status);
                    return;
                }
                if (activeWrite != null && activeWrite.withResponse) {
                    if (doLog) Log.d(LOG_ID,"ATT write acknowledged " + hex(activeWrite.value));
                    writeBusy=false;
                    activeWrite=null;
                    pumpWrite();
                }
            }});
        }

        @Override public void onCharacteristicChanged(BluetoothGatt g, BluetoothGattCharacteristic characteristic) {
            final byte[] value = characteristic.getValue();
            if (value != null) characteristicChanged(g, characteristic, value);
        }

    };

    private void writeNextSetupDescriptor(BluetoothGatt g) {
        final BluetoothGattDescriptor d=setupDescriptors.poll();
        if (d == null) {
            fail("internal setup descriptor queue empty");
            return;
        }
        if (doLog) Log.d(LOG_ID,"enable CCCD " + d.getCharacteristic().getUuid());
        setupAttRequestPending=true;
        armSetupTimeout("enable notifications",ATT_RESPONSE_TIMEOUT_MS);
        if (!g.writeDescriptor(d)) {
            setupAttRequestPending=false;
            connectionFailure("write notification CCCD failed " + d.getCharacteristic().getUuid());
        }
    }

    private void characteristicChanged(final BluetoothGatt g,
                                       final BluetoothGattCharacteristic characteristic,
                                       final byte[] value) {
        if (value == null) return;
        final byte[] copy=Arrays.copyOf(value,value.length);
        final UUID uuid=characteristic.getUuid();
        handler.post(new Runnable() { @Override public void run() {
            if (stopped || g != gatt) return;
            if (doLog) Log.d(LOG_ID,"RX " + uuid + " " + hex(copy));
            if (NOTIFY.equals(uuid)) {
                if(gattCacheRediscovery) {
                    log("ignore application notification during cache rediscovery peer="+peerId);
                    return;
                }
                if(linkProbeOnly) handleLinkRecovery(copy);
                else handle(copy);
            }
        }});
    }

    private void beginStrandedReturnRecovery() {
        if(!linkProbeOnly || stopped) return;
        linkRecoveryConnectionId=KERFSTOK_REPLY_CONNECTION_ID;
        final int token=++linkRecoveryToken;
        // A missing application ACK after SDK send SUCCESS means Kerfstok tried
        // to answer.  In garmin17 that answer reached F101/sequence 2, but GCM's
        // duplicate-key race omitted this one final transport ACK.  This packet
        // is idempotent when GCM already completed or no such transfer exists.
        log("complete possible stranded Kerfstok return transfer peer="+peerId+" id="+
                linkRecoveryConnectionId+" endSeq="+KERFSTOK_SHORT_REPLY_END_SEQUENCE);
        queueWrite(transferEndAck(linkRecoveryConnectionId,KERFSTOK_SHORT_REPLY_END_SEQUENCE));
        handler.postDelayed(new Runnable() { @Override public void run() {
            if(stopped || token!=linkRecoveryToken) return;
            finishLinkProbe(LINK_PROBE_RESPONSIVE,
                    "Garmin ATT answered; no stranded Kerfstok close observed");
        }},STRANDED_RETURN_CLOSE_TIMEOUT_MS);
    }

    private static byte[] transferEndAck(int cid,int endSeq) {
        return new byte[]{(byte)(0xf0|(cid&0x0f)),2,2,0,(byte)(endSeq&0x0f)};
    }

    private void handleLinkRecovery(byte[] p) {
        if(!linkProbeOnly || stopped || p==null || p.length<2) return;
        final int b0=p[0]&0xff;
        if((b0&0xf0)==0xf0 && (b0&0x0f)!=0 && (p[1]&0xff)==1 && p.length>=3) {
            // If the watch retransmits F101 while the recovery client is
            // listening, use its exact connection/sequence instead of the
            // short-reply fixture.  Do not decrypt or deliver through Direct.
            linkRecoveryConnectionId=b0&0x0f;
            final int endSeq=p[2]&0x0f;
            log("ack retransmitted stranded Kerfstok F101 peer="+peerId+" id="+
                    linkRecoveryConnectionId+" endSeq="+endSeq);
            queueWrite(transferEndAck(linkRecoveryConnectionId,endSeq));
            return;
        }
        if(b0==0 && (p[1]&0xff)==4 && p.length>=3) {
            final int cid=p[2]&0x0f;
            if(cid==0 || cid!=linkRecoveryConnectionId) return;
            // The watch only emits 00 04 after accepting the final F102 ACK.
            // Finish its close before detaching this auxiliary GATT client.
            ++linkRecoveryToken;
            queueWrite(new byte[]{0,4,0,(byte)cid});
            log("released stranded Kerfstok return transfer peer="+peerId+" id="+cid);
            final int token=linkRecoveryToken;
            handler.postDelayed(new Runnable() { @Override public void run() {
                if(stopped || token!=linkRecoveryToken) return;
                finishLinkProbe(LINK_PROBE_RETURN_RELEASED,
                        "stranded Kerfstok return channel released id="+cid);
            }},80L);
        }
    }

    private boolean recoverStrandedStartupReply() {
        if(stopped || gracefulStopRequested || linkProbeOnly || !ready || applicationPaused ||
                strandedStartupRecoveryAttempted || strandedStartupConnectionId!=0 ||
                currentOp==null || !currentOp.expectReply || !isStart(currentMessage) ||
                outgoingState!=OutgoingState.REPLY || incomingConnectionId!=0 ||
                writeBusy || !writes.isEmpty()) return false;
        strandedStartupRecoveryAttempted=true;
        strandedStartupConnectionId=KERFSTOK_REPLY_CONNECTION_ID;
        log("START delivered without reply; recover possible stranded watch return peer="+peerId+
                " id="+strandedStartupConnectionId);
        // Retain a bounded deadline even if Android never accepts the write.
        armTimeout("stranded watch return cleanup");
        sendStrandedStartupAck(KERFSTOK_LIBRE3_RETURN_END_SEQUENCE,KERFSTOK_SHORT_REPLY_END_SEQUENCE);
        return true;
    }

    private void sendStrandedStartupAck(final int endSeq,final int nextSeq) {
        if(stopped || strandedStartupConnectionId==0 || incomingConnectionId!=0) return;
        removeUnsentStrandedStartupAck();
        final int token=++strandedStartupToken;
        final int cid=strandedStartupConnectionId;
        log("stranded watch return final ACK peer="+peerId+" id="+cid+" endSeq="+endSeq);
        strandedStartupAckWrite=new WriteOp(transferEndAck(cid,endSeq),false,() -> {
            if(stopped || token!=strandedStartupToken || cid!=strandedStartupConnectionId) return;
            strandedStartupAckWrite=null;
            // Start this wait only after Android accepted the write. A busy
            // queue must not consume the recovery's response window.
            handler.postDelayed(() -> {
                if(stopped || token!=strandedStartupToken || cid!=strandedStartupConnectionId) return;
                if(nextSeq>=0) sendStrandedStartupAck(nextSeq,-1);
                else failMessage("no close after stranded watch return cleanup");
            },STRANDED_RETURN_CLOSE_TIMEOUT_MS);
        });
        writes.add(strandedStartupAckWrite);
        pumpWrite();
    }

    private void removeUnsentStrandedStartupAck() {
        final WriteOp pending=strandedStartupAckWrite;
        strandedStartupAckWrite=null;
        if(pending==null) return;
        writes.remove(pending);
        if(activeWrite==pending) {
            activeWrite=null;
            writeBusy=false;
        }
    }

    private void cancelStrandedStartupRecovery() {
        ++strandedStartupToken;
        strandedStartupConnectionId=0;
        removeUnsentStrandedStartupAck();
    }

    private void finishStrandedStartupRecovery(int cid) {
        cancelStrandedStartupRecovery();
        // Finish the old return's close before opening the retried START.
        // The shared FIFO preserves this order even if Android is busy.
        queueWrite(new byte[]{0,4,0,(byte)cid});
        log("stranded watch return released peer="+peerId+" id="+cid+"; retry START");
        if(currentOp!=null) messages.addFirst(currentOp);
        currentOp=null;
        currentMessage=null;
        outgoingCipher=null;
        outgoingKey=null;
        connectionId=0;
        outgoingState=OutgoingState.IDLE;
        applicationTransaction=false;
        generation++;
        startNextIfPossible();
    }

    // 00 05 getApplicationInfo is safe on ordinary startup/reconnect.  The
    // separate 00 06 openApplication operation is allowed only when an explicit
    // user Reinit armed launchRequested.
    private void beginApplicationInfo() {
        Arrays.fill(appInfoTried,false);
        queryApplicationInfo(appIndex);
    }

    private void writeApplicationInfo(int index) {
        final byte[] query=new byte[18];
        query[1]=5;
        System.arraycopy(appIdBytes[index],0,query,2,16);
        log("query Kerfstok application info peer="+peerId+" app="+appIds.get(index));
        queueWrite(query);
    }

    private void queryApplicationInfo(int index) {
        if (stopped || applicationPaused || !ready) return;
        appLaunchOpenSent=false;
        ownerReadyNotified=false;
        pendingAppInfo=index;
        appInfoTried[index]=true;
        writeApplicationInfo(index);
        final int token=++appLaunchToken;
        handler.postDelayed(new Runnable() { @Override public void run() {
            if (stopped || !ready || token != appLaunchToken || ownerReadyNotified) return;
            pendingAppInfo=-1;
            if(launchRequested) {
                // A CCCD write only proves ATT setup, not a working CIQ
                // application-control channel. In nostart2, 00 05 and the
                // blind 00 06 both went unanswered and the launch was lost.
                // Keep the one-shot user request until the watch answers 00 05.
                log("application info unanswered after Reinit; retain launch and repair link");
                if(!recoverGattCacheInPlace("unanswered Reinit application info"))
                    connectionFailure("application info unanswered; launch still pending");
            }
            else {
                log("application info unanswered; continue without launching Kerfstok");
                finishDirectReady();
            }
        }}, launchRequested ? OPEN_TIMEOUT_MS : APP_INFO_TIMEOUT_MS);
    }

    private void discoverOtherApps() {
        for(int i=0;i<appIds.size();i++) {
            if(!appInfoTried[i]) writeApplicationInfo(i);
        }
    }

    void refreshAppInstallations() {
        handler.post(() -> {
            if(stopped || !ready || pendingAppInfo>=0 || reinitRequested || appLaunchOpenSent) return;
            for(int i=0;i<appIds.size();i++) writeApplicationInfo(i);
        });
    }

    private void sendApplicationLaunch() {
        if (stopped || applicationPaused || !ready || ownerReadyNotified || appLaunchOpenSent || !launchRequested) return;
        // Reserve this attempt now, but only consume its permission once Android
        // accepts the write. A busy queue or a lost GATT before submission must
        // not discard the user's request. Acceptance is not proof of display;
        // after submission we still never re-prompt just because a reply is lost.
        appLaunchOpenSent=true;
        final byte[] open=new byte[18];
        open[0]=0; open[1]=6;
        System.arraycopy(appId,0,open,2,16);
        final int token=++appLaunchToken;
        log("queue Kerfstok launch peer="+peerId);
        writes.add(new WriteOp(open,false,() -> {
            if(stopped || !ready || token!=appLaunchToken) return;
            launchRequested=false;
            log("request Kerfstok launch peer="+peerId+" accepted by Android");
            handler.postDelayed(() -> {
                if (stopped || !ready || token != appLaunchToken || ownerReadyNotified) return;
                // A missing status must not make Direct messaging unusable.
                log("launch response unanswered; continue direct startup");
                finishDirectReady();
            },APP_OPEN_TIMEOUT_MS);
        }));
        pumpWrite();
    }

    private void finishDirectReady() {
        if (stopped || !ready || ownerReadyNotified) return;
        ownerReadyNotified=true;
        reinitRequested=false;
        pendingAppInfo=-1;
        appLaunchOpenSent=false;
        appLaunchToken++;
        discoverOtherApps();
        owner.onDirectGarminReady(peerId);
        startNextIfPossible();
    }

    private void resetAppLaunchState() {
        appLaunchToken++;
        appLaunchOpenSent=false;
        pendingAppInfo=-1;
        ownerReadyNotified=false;
    }

    private void startNextIfPossible() {
        if (stopped || applicationPaused || !ready || applicationTransaction || currentMessage != null || incomingConnectionId != 0) return;

        // A saved Libre3 record's ACK must precede Reinit and every ordinary
        // queued message. Kerfstok intentionally keeps the oldest returned
        // record in-flight until [236]/[238] arrives; if Reinit goes first, all
        // newer sensor minutes can remain trapped behind that one record.
        //
        // acknowledgeLibre3Return() queues the ACK before posting
        // resumeAfterWatchReturn(). If a stale ownerReady gate is still closed,
        // leave Reinit pending for this one looper turn; the proven watch return
        // then reopens ownerReady and calls us again to send the ACK first.
        java.util.Iterator<MessageOp> pending=messages.iterator();
        while(currentOp==null && pending.hasNext()) {
            MessageOp next=pending.next();
            if(!isLibre3ReturnAck(next.message)) continue;
            if(!ownerReadyNotified) {
                log("Libre3 return ACK waiting for watch-return resume: "+next.message+
                        " reinit="+reinitRequested);
                return;
            }
            pending.remove();
            if(acceptsMessage(next)) {
                currentOp=next;
                log("send priority Libre3 return ACK: "+next.message+
                        " reinit="+reinitRequested);
            }
        }

        if(currentOp==null && reinitRequested) {
            // Reinit is an application-level operation. Do it before ordinary
            // queued traffic, but never before an ACK which releases the watch's
            // retained Libre3 return queue.
            if(writeBusy || !writes.isEmpty()) return;
            reinitRequested=false;
            resetAppLaunchState();
            beginApplicationInfo();
            return;
        }
        if(currentOp==null && (!ownerReadyNotified || messages.isEmpty())) return;
        while(currentOp==null && !messages.isEmpty()) {
            MessageOp next=messages.poll();
            if(acceptsMessage(next)) {
                currentOp=next;
                break;
            }
            log("discard queued message after Numbers role change: "+next.message);
        }
        if(currentOp==null) return;
        currentMessage = currentOp.message;
        try {
            final byte[] plain = GarminIQCodec.encode(currentMessage);
            outgoingKey = new byte[16];
            random.nextBytes(outgoingKey);
            outgoingCipher = GarminIQCodec.encrypt(plain, outgoingKey);
            applicationTransaction = true;
            connectionId = 0;
            outgoingState=OutgoingState.IDLE;
            openAttempt = 0;
            recoveryConnectionId = 0;
            log("open app, payload=" + plain.length + " encrypted=" + outgoingCipher.length);
            sendOpen();
        } catch (Throwable t) {
            // Retrying an invalid local object cannot repair it. Retain neither
            // it nor an unbounded retry loop; surface the terminal error.
            fail("encode " + currentMessage + ": " + t);
        }
    }

    private void sendOpen() {
        if (stopped || currentMessage == null || !applicationTransaction) return;
        final byte[] open = new byte[19];
        open[0] = 0; open[1] = 1;
        System.arraycopy(appId, 0, open, 2, 16);
        open[18] = 0;
        openAttempt++;
        outgoingState=OutgoingState.OPENING;
        log("open Kerfstok attempt " + openAttempt);
        queueWrite(open);
        final int token=++openToken;
        handler.postDelayed(new Runnable() { @Override public void run() {
            if (stopped || token != openToken || outgoingState!=OutgoingState.OPENING || currentMessage == null) return;
            if (openAttempt == 1) {
                // Retain the VA3 unanswered-open recovery from v10.2. An explicit
                // CONNECTION_EXISTS response below instead supplies the real id.
                recoverOpen(1,"open unanswered; possible stale outgoing connection");
            } else {
                // Clearing Garmin Connect data was observed to make GCM call the
                // hidden BluetoothGatt.refresh() once for each watch.  A normal
                // force-stop/restart did not.  Before abandoning a BLE-ready Direct
                // connection that still cannot open Kerfstok, reproduce only that
                // low-level, device-local part: clear Android's cached GATT database
                // and rediscover on this GATT without immediately closing it.  This does not touch Garmin Connect's private
                // data and is deliberately limited to one attempt per failure episode.
                if(recoverGattCacheInPlace("repeated unanswered Kerfstok open")) return;
                // If the cache refresh did not help (or Android blocks the hidden
                // API), do not reconnect Direct forever. In Automatic mode yield
                // this peer back to Garmin Connect once.
                if(owner.recoverGarminConnectAfterDirectSetupFailure(peerId,DirectGarminIQ.this)) return;
                failMessage("timeout: open Kerfstok after stale-outgoing reset");
            }
        }}, OPEN_TIMEOUT_MS);
    }

    private void recoverOpen(final int cid, String reason) {
        // A phone process can disappear after 00 01/F105 while GCM keeps the
        // physical BLE link alive. The watch then rejects every new 00 01 with
        // CONNECTION_EXISTS (3). Restarting our GATT client does not close that
        // application channel. Close only the indicated outgoing channel, then
        // retry the same unsent message once; do not reset number-sync state.
        if (cid < 1 || cid > 15 || openAttempt != 1) {
            failMessage("Kerfstok " + reason + " id="+cid+" after attempt "+openAttempt);
            return;
        }
        outgoingState=OutgoingState.RECOVERING;
        recoveryConnectionId=cid;
        final int token=++openToken;
        log(reason+" peer="+peerId+"; close outgoing connection id="+cid+" before retry");
        queueWrite(new byte[]{0,2,(byte)cid});
        handler.postDelayed(new Runnable() { @Override public void run() {
            // If the close ACK was lost, one new open can still confirm that
            // the watch released it. A second rejection is reported, not looped.
            retryAfterClose(token,"outgoing close unanswered");
        }}, OPEN_TIMEOUT_MS);
    }

    private void retryAfterClose(int token, String reason) {
        if (stopped || token!=openToken || outgoingState!=OutgoingState.RECOVERING || currentMessage==null) return;
        recoveryConnectionId=0;
        final int retryToken=++openToken;
        log(reason+"; retry Kerfstok open after settle");
        handler.postDelayed(new Runnable() { @Override public void run() {
            if (stopped || retryToken!=openToken || outgoingState!=OutgoingState.RECOVERING || currentMessage==null) return;
            sendOpen();
        }}, TAKEOVER_SETTLE_MS);
    }

    private void handle(byte[] p) {
        if (p.length == 0) return;
        final int b0 = p[0] & 0xff;
        if (b0 == 0) { handleApplicationControl(p); return; }
        final int cid = b0 & 0x0f;
        final int seq = (b0 >>> 4) & 0x0f;
        if (cid==0 || (cid!=incomingConnectionId && cid!=connectionId)) return;
        if (applicationTransaction || incomingConnectionId!=0) armTimeout("protocol");
        if (seq == 0x0f) handleTransferControl(cid, p);
        else handleIncomingChunk(cid, seq, p);
    }

    private void handleApplicationControl(byte[] p) {
        if (p.length < 2) return;
        final int cmd=p[1]&0xff;
        if (cmd == 5 && p.length >= 18) {
            // 00 05 <app uuid> <version-le16> is getApplicationInfo().
            final int found=knownApp(p,2);
            if(found<0) return;
            final String foundId=appIds.get(found);
            final boolean startup=found==pendingAppInfo && !applicationTransaction && incomingConnectionId==0;
            if (p.length >= 20) {
                final int version=(p[18]&0xff)|((p[19]&0xff)<<8);
                log("Kerfstok application info peer="+peerId+" app="+foundId+" version="+version);
                owner.onDirectGarminAppInfo(peerId,foundId,version,startup);
                if(!startup) return;
                if(version==0xffff) {
                    pendingAppInfo=-1;
                    appLaunchToken++;
                    for(int i=0;i<appIds.size();i++) {
                        if(!appInfoTried[i] && owner.canChangeGarminApp(peerId,appIds.get(i))) {
                            queryApplicationInfo(i);
                            return;
                        }
                    }
                    // Keep the link for a later Reinit or START. Other UUIDs
                    // can still be discovered while a sensor is assigned.
                    discoverOtherApps();
                    boolean allTried=true;
                    for(boolean tried:appInfoTried) allTried&=tried;
                    owner.onDirectGarminError(peerId,null,allTried?"No compatible Kerfstok app installed":
                            "Assigned Kerfstok app not installed: "+appIdHex);
                    return;
                }
                if(!owner.canChangeGarminApp(peerId,foundId)) return;
                setApplication(found);
            } else {
                if(!startup || found!=appIndex) return;
                log("Kerfstok application info " + hex(p));
            }
            pendingAppInfo=-1;
            appLaunchToken++;
            if(launchRequested) sendApplicationLaunch();
            else finishDirectReady();
            return;
        }
        if (cmd == 6 && p.length >= 19 && sameApp(p,2) && appLaunchOpenSent) {
            // openApplication response.  Garmin exposes several symbolic statuses
            // (prompt shown/not shown, already running, ...); the legacy wire value
            // is kept numeric because only completion matters here.
            log("Kerfstok launch status=" + (p[18]&0xff));
            appLaunchToken++;
            finishDirectReady();
            return;
        }
        if (cmd == 1 && p.length >= 20) {
            // 00 01 status <app uuid> connection-id
            final int status=p[2]&0xff;
            if (!sameApp(p,3)) { log("open response for other app"); return; }
            if(applicationPaused) {
                final int cid=p[19]&0xff;
                if((status==0 || status==3) && cid>0 && cid<=15) queueWrite(new byte[]{0,2,(byte)cid});
                return;
            }
            if (currentMessage == null || outgoingKey == null || outgoingState!=OutgoingState.OPENING) {
                log("stale open response ignored");
                return;
            }
            if (status == 3) {
                // GCM's CommandHandler names this exact response
                // CONNECTION_EXISTS in garminapp/logs (2026-09-03).
                recoverOpen(p[19]&0xff,"CONNECTION_EXISTS");
                return;
            }
            if (status != 0) { failMessage("Kerfstok open status=" + status); return; }
            openToken++;
            connectionId=p[19]&0x0f;
            if (connectionId == 0) { failMessage("invalid connection id 0"); return; }
            outgoingState=OutgoingState.KEY;
            armTimeout("outgoing key");
            final byte[] keyPacket=new byte[19];
            keyPacket[0]=(byte)(0xf0|connectionId);
            keyPacket[1]=5;
            System.arraycopy(outgoingKey,0,keyPacket,2,16);
            keyPacket[18]=0;
            queueOutgoingAckWrite(keyPacket,5,255);
            return;
        }
        if (cmd == 2 && p.length >= 4) {
            final int status=p[2]&0xff;
            final int cid=p[3]&0xff;
            log("close response status=" + status + " id=" + cid);
            if (outgoingState==OutgoingState.RECOVERING && recoveryConnectionId!=0 && cid==recoveryConnectionId) {
                if (status==0) retryAfterClose(openToken,"stale outgoing connection closed id="+cid);
                else failMessage("stale outgoing close rejected status="+status+" id="+cid);
            }
            return;
        }
        if (cmd == 3 && p.length >= 19) {
            final int cid=p[18]&0x0f;
            // A newly announced channel has a current owner. Even a different
            // app must not receive a guessed final ACK for the old channel.
            if(cid!=0 && cid==strandedStartupConnectionId) cancelStrandedStartupRecovery();
            // Watch opens an application channel, including unsolicited number
            // entries. Follow the foreground app while no sensor assignment
            // or transaction binds this Direct link to its current app.
            final int appMatch=knownApp(p,2);
            if(appMatch<0) return;
            if(appMatch!=appIndex && (!owner.canChangeGarminApp(peerId,appIds.get(appMatch)) ||
                    (launchRequested && pendingAppInfo>=0) || appLaunchOpenSent ||
                    garminConnectProbePaused || !selectApplication(appIds.get(appMatch)))) return;
            // A shared-bearer glucose transfer can be completed from Direct's
            // outbound transport acknowledgement alone. Kerfstok still emits [229],
            // but Garmin Connect is also subscribed and will answer its return
            // channel. Do not send a second F105/00-03 from Direct for that reply.
            if(ignoreSharedGlucoseReplyUntil!=0) {
                if(System.currentTimeMillis()<ignoreSharedGlucoseReplyUntil) {
                    log("leave glucose reply channel to Garmin Connect id="+cid);
                    return;
                }
                ignoreSharedGlucoseReplyUntil=0;
            }
            // applicationPaused has two meanings.  During a one-shot Garmin
            // Connect health probe, Garmin Connect must own the reply channel, so
            // Direct must stay out of the way.  After a Kerfstok STOP, however,
            // the next watch-initiated message is commonly START (notably after a
            // watch power cycle).  Ignoring its 00-03 open leaves the watch-side
            // Communications.transmit() pending and can prevent every later START
            // reply, even across a fresh phone GATT connection.  Accept that
            // channel normally; AllData already ignores non-START late messages
            // while watchStopped is true.
            if(applicationPaused && garminConnectProbePaused) {
                log("leave incoming app connection to Garmin Connect probe id="+cid);
                return;
            }
            if(applicationPaused)
                log("accept incoming app connection while STOP-paused id="+cid);
            if (cid==0) { log("ignore incoming connection id 0"); return; }
            if (incomingConnectionId!=0) {
                if (cid!=incomingConnectionId) { log("ignore overlapping incoming connection " + cid); return; }
                // Retrying 00 03 must not change the already agreed key or erase
                // data/completion state for the transfer currently being received.
                queueWrite(incomingKeyPacket(cid));
                queueWrite(new byte[]{0,3,0,(byte)cid});
                log("duplicate incoming open peer="+peerId+" id="+cid);
                return;
            }
            // A new, known-app return is normal traffic. Cancel any unsent
            // cleanup ACK so it cannot acknowledge this new transfer instead.
            cancelStrandedStartupRecovery();
            resetIncomingState();
            incomingConnectionId=cid;
            armTimeout("incoming Kerfstok message");
            log("accept incoming app connection peer="+peerId+" id="+cid+
                    (currentMessage==null?" watch initiated":" while request pending"));
            // Generate and write our return key before accepting the channel.  HCI
            // traces from v9 show that when Garmin Connect is also subscribed it can
            // answer F106 ~10-20 ms before our post-accept write reaches ATT; the VA3
            // then encrypts with Garmin Connect's key and Direct BLE cannot decode it.
            // The watch already allocated the connection id in 00 03, so pre-writing
            // F105 here gives our key first chance.  F106 still sends the same key
            // again, preserving the normal handshake if this early write is ignored.
            incomingKey=new byte[16];
            random.nextBytes(incomingKey);
            queueWrite(incomingKeyPacket(incomingConnectionId));
            queueWrite(new byte[]{0,3,0,(byte)incomingConnectionId});
            log("pre-armed incoming XXTEA key before accept for connection " + incomingConnectionId);
            return;
        }
        if (cmd == 4 && p.length >= 3) {
            // Watch closes its return channel; acknowledge before releasing next app transaction.
            final int cid=p[2]&0x0f;
            if(cid!=0 && cid==strandedStartupConnectionId && incomingConnectionId==0) {
                finishStrandedStartupRecovery(cid);
                return;
            }
            if (cid==0 || cid!=incomingConnectionId) {
                log("ignore close for unowned incoming connection " + cid);
                return;
            }
            queueWrite(new byte[]{0,4,0,(byte)cid});
            resetIncomingState();
            // An unrelated watch action must not finish an outstanding request.
            if (currentMessage==null) {
                applicationTransaction=false;
                generation++;
            } else armTimeout("Kerfstok application reply");
            startNextIfPossible();
            return;
        }
        // 00 02 00 <id> is the response to our outgoing close.
    }

    private static boolean matchesApp(byte[] p, int off, byte[] wanted) {
        if(wanted==null || off+16>p.length) return false;
        for(int i=0;i<16;i++) if(p[off+i]!=wanted[i]) return false;
        return true;
    }

    private boolean sameApp(byte[] p, int off) { return matchesApp(p,off,appId); }

    private int knownApp(byte[] p,int off) {
        for(int i=0;i<appIdBytes.length;i++) if(matchesApp(p,off,appIdBytes[i])) return i;
        return -1;
    }

    private void setApplication(int index) {
        appIndex=index;
        appId=appIdBytes[index];
        appIdHex=appIds.get(index);
    }

    /** Switch the application channel on an idle Direct link. */
    synchronized boolean selectApplication(String wantedHex) {
        int index=appIds.indexOf(wantedHex);
        if(index==appIndex) return true;
        if(index<0 || currentMessage!=null || connectionId!=0 || incomingConnectionId!=0 ||
                applicationTransaction) return false;
        setApplication(index);
        resetAppLaunchState();
        ownerReadyNotified=ready;
        log("Direct Garmin app changed to "+appIdHex);
        return true;
    }

    private byte[] incomingKeyPacket(int cid) {
        final byte[] keyPacket=new byte[19];
        keyPacket[0]=(byte)(0xf0|cid);
        keyPacket[1]=5;
        System.arraycopy(incomingKey,0,keyPacket,2,16);
        keyPacket[18]=2;
        return keyPacket;
    }

    private void handleTransferControl(int cid, byte[] p) {
        if (p.length < 2) return;
        final int cmd=p[1]&0xff;
        if(cmd==1 && p.length>=5 && cid==strandedStartupConnectionId && cid!=0 && incomingConnectionId==0) {
            // A retransmitted end marker supplies the exact sequence; replace
            // the two short-message candidates and invalidate their timers.
            sendStrandedStartupAck(p[2]&0x0f,-1);
            return;
        }
        final boolean incoming=cid!=0 && cid==incomingConnectionId;
        final boolean outgoing=cid!=0 && cid==connectionId && outgoingCipher!=null;
        if ((cmd==0 || cmd==1 || cmd==6) ? !incoming : (cmd==2 && !outgoing)) {
            // Transfer commands cannot create their own ownership. Only a valid
            // Kerfstok application open authorizes the corresponding direction.
            log("ignore unowned transfer command " + cmd + " peer="+peerId+" id=" + cid);
            return;
        }
        switch (cmd) {
            case 6: // F106: watch requests a key for a watch->phone transfer.
                if (incomingKey == null) {
                    incomingKey=new byte[16];
                    random.nextBytes(incomingKey);
                }
                // If the pre-armed F105 was accepted this is just a duplicate of the
                // same key.  If it was too early and ignored, this is the ordinary
                // Garmin handshake and supplies it at the expected point.
                queueWrite(incomingKeyPacket(cid));
                break;
            case 0: // F100 start transfer
                if (p.length < 7) return;
                final int length=(p[3]&0xff)|((p[4]&0xff)<<8)|((p[5]&0xff)<<16)|((p[6]&0xff)<<24);
                if (p[2]!=2 || length<8 || length>GarminIQCodec.MAX_MESSAGE_BYTES || (length&3)!=0) {
                    failMessage("bad incoming format/length peer="+peerId+" id="+cid+" length="+length); return;
                }
                if (incomingLength!=-1) {
                    if (incomingLength!=length) { failMessage("incoming transfer length changed"); return; }
                    log("duplicate incoming F100 peer="+peerId+" id="+cid);
                } else {
                    incomingLength=length;
                    incomingCipher=new ByteArrayOutputStream(incomingLength);
                    incomingNextSequence=0;
                }
                queueWrite(new byte[]{(byte)(0xf0|cid),2,0,0,(byte)0xff});
                break;
            case 1: // F101 end transfer from watch
                if (p.length < 5) return;
                final int endSeq=p[2]&0x0f;
                final int expectedCrc=(p[3]&0xff)|((p[4]&0xff)<<8);
                if (incomingCompleted && cid == incomingCompletedConnectionId
                        && endSeq == incomingCompletedEndSeq && expectedCrc == incomingCompletedCrc) {
                    // ACK retransmission, but do not deliver the same application reply twice.
                    queueWrite(new byte[]{(byte)(0xf0|cid),2,2,0,(byte)endSeq});
                    log("duplicate incoming F101 ignored id=" + cid + " crc=" + Integer.toHexString(expectedCrc));
                    break;
                }
                if (incomingCipher == null || incomingKey == null) return;
                if (endSeq!=incomingNextSequence) {
                    failMessage("incoming end sequence "+endSeq+" expected "+incomingNextSequence); return;
                }
                final byte[] encrypted=incomingCipher.toByteArray();
                if (encrypted.length != incomingLength) {
                    failMessage("incoming size " + encrypted.length + " expected " + incomingLength); return;
                }
                final int crc=GarminIQCodec.crc16Arc(encrypted);
                if (crc != expectedCrc) { failMessage("incoming CRC " + Integer.toHexString(crc) + " expected " + Integer.toHexString(expectedCrc)); return; }
                queueWrite(new byte[]{(byte)(0xf0|cid),2,2,0,(byte)endSeq});
                incomingCompleted=true;
                incomingCompletedConnectionId=cid;
                incomingCompletedEndSeq=endSeq;
                incomingCompletedCrc=expectedCrc;
                // Completed ciphertext must not remain decodable after a repeated F101.
                incomingCipher=null;
                try {
                    final byte[] plain=GarminIQCodec.decrypt(encrypted,incomingKey);
                    final Object decoded=GarminIQCodec.decode(plain);
                    final boolean reply=outgoingState==OutgoingState.REPLY &&
                            owner.matchesGarminReply(peerId,currentMessage,decoded);
                    if (!reply && !AllData.directWatchInitiatedMessage(decoded)) {
                        log("ignore unmatched application reply peer="+peerId+" id="+cid+": "+decoded);
                        break;
                    }
                    log("received peer="+peerId+" id="+cid+(reply?" reply ":" watch initiated ")+decoded);
                    // The callback can queue more work. Complete only the request
                    // that this decoded object actually answers.
                    if (reply) {
                        // GATT readiness alone is not proof that Kerfstok can
                        // answer. Only a matching application reply resets backoff
                        // and permits a future one-shot cache refresh if a new
                        // independent failure episode develops.
                        reconnectDelayMs=1500L;
                        strandedStartupRecoveryAttempted=false;
                        gattCacheRefreshAttempted=false;
                        currentOp=null;
                        currentMessage=null;
                        outgoingState=OutgoingState.IDLE;
                    }
                    if(ready && !ownerReadyNotified && !launchRequested && !appLaunchOpenSent) {
                        // A valid incoming message also proves this app is reachable.
                        pendingAppInfo=-1;
                        appLaunchToken++;
                        ownerReadyNotified=true;
                        discoverOtherApps();
                    }
                    owner.onDirectGarminMessage(peerId,appIdHex,decoded);
                } catch (Throwable t) {
                    failMessage("decode incoming: " + t);
                }
                break;
            case 2: // F102 acknowledgement
                if (p.length < 5) return;
                final int ack=p[2]&0xff;
                final int status=p[3]&0xff;
                final int sequence=p[4]&0xff;
                if (status!=0) {
                    // A data NACK can arrive before our window tail/F101 is
                    // sent. Do not advance or leave its remaining fragments in
                    // the write queue: reconnect and retry the entire message.
                    if ((ack==1 && (outgoingState==OutgoingState.WINDOW || outgoingState==OutgoingState.END)) ||
                            (ack==outgoingAckType && outgoingAckSent)) {
                        failMessage("outgoing NACK ack="+ack+" status="+status+" sequence="+sequence);
                    } else log("ignore unrelated NACK "+hex(p));
                    break;
                }
                if (!outgoingAckSent || ack!=outgoingAckType || sequence!=outgoingAckSequence) {
                    log("ignore unexpected outgoing ACK "+hex(p)+" expected="+outgoingAckType+
                            "/"+outgoingAckSequence+" submitted="+outgoingAckSent);
                    break;
                }
                outgoingAckWrite=null;
                outgoingAckType=-1;
                outgoingAckSequence=-1;
                outgoingAckSent=false;
                if (ack == 5 && outgoingState==OutgoingState.KEY) {
                    // Key accepted: announce encrypted transfer size.
                    final int n=outgoingCipher.length;
                    outgoingState=OutgoingState.START;
                    queueOutgoingAckWrite(new byte[]{(byte)(0xf0|cid),0,2,(byte)n,(byte)(n>>>8),(byte)(n>>>16),(byte)(n>>>24)},0,255);
                } else if (ack == 0 && outgoingState==OutgoingState.START) {
                    outgoingOffset=0;
                    outgoingNextSequence=0;
                    sendOutgoingWindow(cid);
                } else if (ack == 1 && outgoingState==OutgoingState.WINDOW) {
                    // A full window contains sequence numbers 0..14.  The VA3
                    // acknowledges that window with F102 01 00 0e, then sequence
                    // numbering restarts at zero for the next window.
                    outgoingNextSequence=0;
                    if (outgoingOffset >= outgoingCipher.length) sendOutgoingEnd(cid);
                    else sendOutgoingWindow(cid);
                } else if (ack == 2 && outgoingState==OutgoingState.END) {
                    // Transport complete. Close outgoing app channel; applicationTransaction
                    // remains set until Kerfstok's reply channel is closed with 00 04.
                    queueWrite(new byte[]{0,2,(byte)cid});
                    outgoingCipher=null;
                    outgoingKey=null;
                    outgoingState=OutgoingState.REPLY;
                    owner.onDirectGarminSendSuccess(peerId);
                    if (currentOp != null && !currentOp.expectReply) {
                        // For shared-bearer glucose, Kerfstok still sends [229].
                        // Leave that return connection to Garmin Connect instead
                        // of racing it with a second independently generated key.
                        if(reuseExistingCccd && isGlucose(currentMessage))
                            ignoreSharedGlucoseReplyUntil=System.currentTimeMillis()+1500L;
                        final Object completedMessage=currentMessage;
                        currentOp=null;
                        currentMessage=null;
                        applicationTransaction=false;
                        outgoingState=OutgoingState.IDLE;
                        generation++;
                        handler.postDelayed(new Runnable() { @Override public void run() {
                            if(stopped) return;
                            owner.onGarminNoReplyComplete(peerId,completedMessage);
                            startNextIfPossible();
                        }},80L);
                    }
                }
                break;
            default:
                log("unknown F1 command " + cmd + " " + hex(p));
        }
    }

    private void sendOutgoingWindow(int cid) {
        final byte[] encrypted=outgoingCipher;
        outgoingState=OutgoingState.WINDOW;
        int packets=0;
        while (outgoingOffset < encrypted.length && packets < 15) {
            final int n=Math.min(CHUNK, encrypted.length-outgoingOffset);
            final byte[] packet=new byte[n+1];
            packet[0]=(byte)(((outgoingNextSequence&0x0f)<<4)|(cid&0x0f));
            System.arraycopy(encrypted,outgoingOffset,packet,1,n);
            if (packets==14) queueOutgoingAckWrite(packet,1,14);
            else queueWrite(packet);
            outgoingOffset += n;
            outgoingNextSequence++;
            packets++;
        }
        if (outgoingOffset >= encrypted.length && outgoingNextSequence < 15)
            sendOutgoingEnd(cid);
        // If exactly 15 packets were needed, the watch first acknowledges the
        // 0..14 window (F102/01).  sendOutgoingEnd() is then called from that ACK.
    }

    private void sendOutgoingEnd(int cid) {
        final int crc=GarminIQCodec.crc16Arc(outgoingCipher);
        outgoingState=OutgoingState.END;
        final int endSequence=outgoingNextSequence&0x0f;
        queueOutgoingAckWrite(new byte[]{(byte)(0xf0|cid),1,(byte)endSequence,(byte)crc,(byte)(crc>>>8)},2,endSequence);
    }

    private void handleIncomingChunk(int cid, int seq, byte[] p) {
        if (incomingCipher == null || cid != incomingConnectionId) return;
        if (seq != incomingNextSequence) {
            if (incomingNextSequence==0 && seq==14 && incomingLastWindowTail!=null &&
                    Arrays.equals(p,incomingLastWindowTail)) {
                // The VA4 can repeat the final packet of a full window while our
                // F102/01 ACK is crossing on the shared Garmin ATT bearer.  The
                // ciphertext was already appended, so just repeat the ACK.
                queueWrite(new byte[]{(byte)(0xf0|cid),2,1,0,14});
                log("duplicate incoming window tail ignored id="+cid+" seq=14");
                return;
            }
            failMessage("incoming sequence " + seq + " expected " + incomingNextSequence);
            return;
        }
        if (seq==0) incomingLastWindowTail=null;
        final int left=incomingLength-incomingCipher.size();
        final int n=p.length-1;
        if (n<=0 || n>left) {
            failMessage("incoming chunk exceeds announced size peer="+peerId+" id="+cid); return;
        }
        incomingCipher.write(p,1,n);
        incomingNextSequence++;
        if (incomingNextSequence == 15) {
            // Full 0..14 window.  Garmin waits for this ACK before either sending
            // another window or, for an exact 15-packet ending, the final F101.
            // Keep the exact tail until sequence 0 of the next window arrives,
            // because some watches retransmit sequence 14 once before seeing ACK.
            incomingLastWindowTail=Arrays.copyOf(p,p.length);
            queueWrite(new byte[]{(byte)(0xf0|cid),2,1,0,14});
            incomingNextSequence=0;
        }
    }

    private void queueWrite(byte[] p) {
        queueWrite(p,false);
    }

    private void queueWriteRequest(byte[] p) {
        queueWrite(p,true);
    }

    private void queueWrite(byte[] p, boolean withResponse) {
        if (stopped) return;
        writes.add(new WriteOp(p,withResponse));
        pumpWrite();
    }

    private void queueOutgoingAckWrite(byte[] p,int ack,int sequence) {
        if (stopped) return;
        final WriteOp op=new WriteOp(p,false);
        outgoingAckWrite=op;
        outgoingAckType=ack;
        outgoingAckSequence=sequence;
        outgoingAckSent=false;
        writes.add(op);
        pumpWrite();
    }

    private void pumpWrite() {
        if (stopped || writeBusy || gatt == null || writeCharacteristic == null) return;
        if (writes.isEmpty()) {
            // Reinit waits in startNextIfPossible() until the final transport
            // write drains. In particular, an incoming close queues 00 04 and
            // checks Reinit while that write is still busy. Resume from its
            // completion instead of waiting indefinitely for another packet.
            // The existing dispatcher retains ACK priority and session guards.
            if (reinitRequested) startNextIfPossible();
            return;
        }
        // Keep the head until Android accepts it. Requeueing a busy write at
        // the tail changed 0,1,2,3,4,...,14 into 0,1,2,4,...,14,3 in mailbox.tar.xz.
        final WriteOp op=writes.peek();
        final byte[] p=op.value;
        try {
            activeWrite=op;
            writeCharacteristic.setValue(p);
            writeCharacteristic.setWriteType(op.withResponse ? BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
                    : BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE);
            writeBusy=true;
            if (!gatt.writeCharacteristic(writeCharacteristic)) {
                // Hold the queue during the retry delay as well, so newly queued
                // packets cannot bypass this write or start additional retries.
                handler.postDelayed(new Runnable() { @Override public void run() {
                    if (activeWrite != op) return;
                    activeWrite=null;
                    writeBusy=false;
                    pumpWrite();
                }},20L);
            } else {
                writes.remove();
                if (op==outgoingAckWrite) outgoingAckSent=true;
                if (doLog) Log.d(LOG_ID,(op.withResponse?"TX(req) ":"TX ") + hex(p));
                if(op.onSubmitted!=null) op.onSubmitted.run();
                if (!op.withResponse) {
                    // WRITE_TYPE_NO_RESPONSE callbacks are not equally reliable on old Android releases.
                    // Pace writes ourselves at approximately the cadence seen in the VA3 capture.
                    handler.postDelayed(new Runnable() { @Override public void run() {
                        if (activeWrite != op) return;
                        activeWrite=null;
                        writeBusy=false;
                        pumpWrite();
                    }},18L);
                }
            }
        } catch (Throwable t) {
            activeWrite=null;
            writeBusy=false;
            failMessage("write: " + t);
        }
    }

    private void armTimeout(final String what) {
        armTimeout(what,TIMEOUT_MS);
    }

    private void armTimeout(final String what,long timeoutMs) {
        final int token=++generation;
        handler.postDelayed(new Runnable() { @Override public void run() {
            if (!stopped && token == generation) {
                if(!recoverStrandedStartupReply()) failMessage("timeout: " + what);
            }
        }}, timeoutMs);
    }

    private void armSetupTimeout(final String what,long timeoutMs) {
        connectionSetupStage=what;
        connectionSetupDeadlineElapsed=SystemClock.elapsedRealtime()+timeoutMs;
        final int token=++setupTimeoutGeneration;
        handler.postDelayed(new Runnable() { @Override public void run() {
            if(!stopped && setupAttRequestPending && token==setupTimeoutGeneration)
                failMessage("timeout: "+what);
        }},timeoutMs);
    }

    private void cancelSetupTimeout() {
        ++setupTimeoutGeneration;
        connectionSetupDeadlineElapsed=0L;
        connectionSetupStage=null;
    }

    private void resetProtocolState(boolean clearQueuedMessages) {
        cancelStrandedStartupRecovery();
        generation++;
        writes.clear();
        writeBusy=false;
        activeWrite=null;
        openToken++;
        openAttempt=0;
        recoveryConnectionId=0;
        currentOp=null;
        currentMessage=null;
        outgoingCipher=null;
        outgoingKey=null;
        outgoingOffset=0;
        outgoingNextSequence=0;
        outgoingAckWrite=null;
        outgoingAckType=-1;
        outgoingAckSequence=-1;
        outgoingAckSent=false;
        connectionId=0;
        outgoingState=OutgoingState.IDLE;
        resetIncomingState();
        applicationTransaction=false;
        if (clearQueuedMessages) messages.clear();
    }

    private void resetIncomingState() {
        incomingConnectionId=0;
        incomingCipher=null;
        incomingKey=null;
        incomingLength=-1;
        incomingNextSequence=0;
        incomingLastWindowTail=null;
        incomingCompleted=false;
        incomingCompletedConnectionId=0;
        incomingCompletedEndSeq=-1;
        incomingCompletedCrc=-1;
    }


    /** Refresh can itself start service discovery on a connected watch. Never
     * refresh and immediately close that client: nostart2 records the discovery
     * request followed by unregisterApp(), and no discovery completion on the
     * shared bearer. Retain this GATT and its setup deadline until completion.
     * Queue discoverServices() to obtain the normal Java completion callback.
     * No Bluetooth toggle, bond changes or other watch's client is involved.
     */
    private boolean recoverGattCacheInPlace(String reason) {
        if(stopped || linkProbeOnly || !ready || gatt==null || gattCacheRefreshAttempted)
            return false;
        final BluetoothGatt current=gatt;
        gattCacheRefreshAttempted=true;
        gattCacheRediscovery=true;
        if(currentOp!=null) messages.addFirst(currentOp);
        ready=false;
        resetAppLaunchState();
        resetProtocolState(false);
        setupDescriptors.clear();
        writeCharacteristic=null;
        notifyCharacteristic=null;
        // Discovery may reveal changed handles; explicitly subscribe again.
        notificationSubscriptionLost=true;
        setupAttRequestPending=true;
        armSetupTimeout("cache rediscovery",ATT_DISCOVERY_TIMEOUT_MS);
        log("repair Direct Garmin in place; preserve GATT during cache rediscovery peer="+
                peerId+" reason="+reason+" pendingLaunch="+launchRequested);
        if(!refreshGattCache(current)) {
            connectionFailure("GATT cache refresh unavailable: "+reason);
            return true;
        }
        try {
            if(!current.discoverServices()) {
                // refresh may already have started a native ATT operation.
                // Do not immediately unregister its owner even if the Java
                // search request was rejected; the setup guard remains armed.
                log("discoverServices after refresh returned false; retain GATT for pending discovery");
            }
        } catch(Throwable t) {
            log("discoverServices after refresh failed; retain GATT for pending discovery: "+t);
        }
        return true;
    }

    /**
     * Clear Android's cached GATT service database for this physical watch.
     * BluetoothGatt.refresh() is a hidden Android API; Garmin Connect itself
     * called it for both watches immediately after its app data had been cleared
     * in the ClearData capture.  The system cache is keyed by device address, so
     * invoking it through Juggluco's own GATT client targets the same cached
     * database without accessing Garmin Connect's private files.
     */
    private boolean refreshGattCache(BluetoothGatt target) {
        if(target==null) return false;
        try {
            Method method=BluetoothGatt.class.getDeclaredMethod("refresh");
            method.setAccessible(true);
            Object result=method.invoke(target);
            boolean ok=!(result instanceof Boolean) || ((Boolean)result).booleanValue();
            log("BluetoothGatt.refresh() cache recovery peer="+peerId+" result="+ok);
            return ok;
        } catch(Throwable t) {
            log("BluetoothGatt.refresh() unavailable for cache recovery peer="+peerId+": "+t);
            return false;
        }
    }

    private void failMessage(String s) {
        // A failed START used to be discarded here, leaving glucoseReady false
        // until Reinit. Reset the session and retry the same transaction first.
        connectionFailure(s);
    }

    private void reconnectLater() {
        if (stopped || gracefulStopRequested || reconnectPending) return;
        if(!owner.isGarminActive(peerId)) {
            log("Direct Garmin peer inactive; suppress reconnect peer="+peerId);
            stop();
            return;
        }
        final long delay=reconnectDelayMs;
        reconnectDelayMs=Math.min(60000L,reconnectDelayMs*2L);
        reconnectPending=true;
        final int token=++reconnectToken;
        log("reconnect in "+delay+" ms; retained messages="+messages.size());
        handler.postDelayed(new Runnable() { @Override public void run() {
            if (stopped || gracefulStopRequested || token!=reconnectToken) return;
            reconnectPending=false;
            if(!owner.isGarminActive(peerId)) {
                log("Direct Garmin peer became inactive; cancel delayed reconnect peer="+peerId);
                stop();
                return;
            }
            if (gatt == null) connect();
        }}, delay);
    }

    private void connectionFailure(String s) {
        if(linkProbeOnly) {
            finishLinkProbe(LINK_PROBE_FAILED,s);
            return;
        }
        log("recover Direct GATT peer="+peerId+" reason="+s+" ready="+ready+
                " setup="+connectionSetupStage+" pendingLaunch="+launchRequested);
        gattCacheRediscovery=false;
        final boolean setupFailure=!ready;
        if(setupFailure) ++consecutiveSetupFailures;
        setupAttRequestPending=false;
        cancelSetupTimeout();
        final Object failed=currentMessage;
        if(currentOp!=null) messages.addFirst(currentOp);
        ready=false;
        resetAppLaunchState();
        resetProtocolState(false);
        setupDescriptors.clear();
        final BluetoothGatt old=gatt;
        gatt=null;
        writeCharacteristic=null;
        notifyCharacteristic=null;
        if (old != null) {
            try { old.disconnect(); } catch (Throwable ignored) {}
            try { old.close(); } catch (Throwable ignored) {}
        }
        if(!hasPendingMessages()) owner.onDirectGarminDisconnected(peerId);
        owner.onDirectGarminError(peerId,failed,s);
        if(setupFailure && consecutiveSetupFailures>=2 &&
                owner.recoverGarminConnectAfterDirectSetupFailure(peerId,this)) return;
        reconnectLater();
    }

    private void fail(String s) {
        if(linkProbeOnly) {
            finishLinkProbe(LINK_PROBE_FAILED,s);
            return;
        }
        log(s);
        gattCacheRediscovery=false;
        final Object failed=currentMessage;
        ready=false;
        setupAttRequestPending=false;
        cancelSetupTimeout();
        reconnectToken++;
        reconnectPending=false;
        resetAppLaunchState();
        resetProtocolState(true);
        final BluetoothGatt old=gatt;
        gatt=null;
        writeCharacteristic=null;
        notifyCharacteristic=null;
        if (old != null) {
            try { old.disconnect(); } catch (Throwable ignored) {}
            try { old.close(); } catch (Throwable ignored) {}
        }
        owner.onDirectGarminError(peerId,failed,s);
    }

    private void finishLinkProbe(int result,String detail) {
        if(!linkProbeOnly || stopped) return;
        ++linkRecoveryToken;
        log("link probe peer="+peerId+" result="+result+" "+detail);
        stop();
        owner.onGarminLinkProbeFinished(peerId,this,result,detail);
    }

    private static String hex(byte[] p) {
        final StringBuilder b=new StringBuilder(p.length*2);
        for (byte v:p) b.append(String.format(java.util.Locale.US,"%02x",v&0xff));
        return b.toString();
    }

    private static void log(String s) {
        if (doLog) Log.i(LOG_ID,s);
    }
}
