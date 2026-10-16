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
/*      Fri Jan 27 15:32:11 CET 2023                                                 */


package tk.glucodata.nums;

import android.app.Application;
import android.bluetooth.BluetoothDevice;
import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import android.widget.Toast;

import com.garmin.android.connectiq.ConnectIQ;
import com.garmin.android.connectiq.ConnectIQ.IQApplicationEventListener;
import com.garmin.android.connectiq.ConnectIQ.IQApplicationInfoListener;
import com.garmin.android.connectiq.ConnectIQ.IQConnectType;
import com.garmin.android.connectiq.ConnectIQ.IQDeviceEventListener;
import com.garmin.android.connectiq.ConnectIQ.IQMessageStatus;
import com.garmin.android.connectiq.ConnectIQ.IQOpenApplicationListener;
import com.garmin.android.connectiq.ConnectIQ.IQOpenApplicationStatus;
import com.garmin.android.connectiq.ConnectIQ.IQSendMessageListener;
import com.garmin.android.connectiq.IQApp;
import com.garmin.android.connectiq.IQDevice;
import com.garmin.android.connectiq.IQDevice.IQDeviceStatus;
import com.garmin.android.connectiq.exception.InvalidStateException;
import com.garmin.android.connectiq.exception.ServiceUnavailableException;

import static consts.consts.COLORBLACK;
import static consts.consts.GOTGLUCOSE;
import static consts.consts.GOTSTOPALARM;
import static consts.consts.LIBRE3DIRECT;
import static consts.consts.NUMBERROLE;
import static consts.consts.STOPALARM;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.ListIterator;
import java.util.Queue;

import androidx.annotation.NonNull;
import tk.glucodata.Applic;
import tk.glucodata.BuildConfig;
import tk.glucodata.GarminLibre3;
import tk.glucodata.GarminAlarms;
import tk.glucodata.GarminLibre3Lifecycle;
import tk.glucodata.Log;
import tk.glucodata.Natives;
import tk.glucodata.Notify;

import static consts.consts.DELETE;
import static consts.consts.DELETED;
import static consts.consts.DIDSETENDNUM;
import static consts.consts.GETENDNUM;
import static consts.consts.GLUCOSE;
import static consts.consts.GOTNUMS;
import static consts.consts.HAVENUMS;
import static consts.consts.MORENUMS;
import static consts.consts.NOMORENUMS;
import static consts.consts.NUMS;
import static consts.consts.PUTLABELS;
import static consts.consts.PUTNUMS;
import static consts.consts.PUTPRECISION;
import static consts.consts.RECEIVEDCUTS;
import static consts.consts.RECEIVEDPRECISION;
import static consts.consts.RECEIVEDUNITS;
import static consts.consts.SENDERROR;
import static consts.consts.SETENDNUM;
import static consts.consts.SHORTCUTS;
import static consts.consts.START;
//import static consts.consts.ASKLOWEST;
import static consts.consts.STOP;
import static consts.consts.STRING;
import static consts.consts.maxstorage;
import static consts.consts.GLUNITS;
import static tk.glucodata.Log.doLog;
import static tk.glucodata.nums.numio.didreceivebackup;

//public class AllData extends AndroidViewModel {
public class AllData  {
   String infostr="";
//    private int  lastheart = 0, lastpress = 0;
//    private File numdatfile;

    private final IQlisten IQlistener = new IQlisten();
//    private Object ups[]=null;// = new List<?>[]{Arrays.asList(NUMS, lastnums), Arrays.asList(HEART, lastheart)};
//    private int upiter = 0;
//    NumFragment numfrags[]={null,null};
//    DeviceActivity activity=null;
public    List<IQDevice> devices=null;
   public int devused=0;

    private static final String LOG_ID = "Alldata";

    ConnectIQ mConnectIQ;
    private GarminSdkContext garminSdkContext;
    // When Garmin Connect is started after Juggluco has already fallen back to
    // Direct BLE, its broadcasts can arrive before Garmin's ConnectIQ singleton
    // has completed initialize(). Keep Direct as owner while the SDK is restarted.
    private boolean garminSdkRestartPending;
    // A full SDK shutdown/initialize cycle is the recovery that manually changing
    // Garmin Status to "Via Garmin Connect" was observed to perform successfully.
    // Rate-limit automatic use so a broken Garmin service cannot create a restart loop.
    private long lastGarminSdkHardRestart;

    // Kerfstok and the larger Libre3/Kerfstok superset use the same application
    // protocol but have distinct Connect IQ application ids. Selection is per watch.
    private static final String LIBRE3_GARMIN_RELEASE_APP_ID = "d3d41e50b9d44e24a63b4158e8b84bb4";
    private static final String LIBRE3_GARMIN_DEBUG_APP_ID = "524467d21c0645d0a81ca3ef2a8d172a";
    public static final String LIBRE3_GARMIN_APP_ID = BuildConfig.isReleaseID==1 ?
          LIBRE3_GARMIN_RELEASE_APP_ID : LIBRE3_GARMIN_DEBUG_APP_ID;
    private static final String GARMIN_APP_PREFIX = "app:";
    private IQApp mMyApp;       // ordinary/small Kerfstok
    private IQApp mLibre3App;   // Kerfstok + direct Libre3
    private boolean mAppIsOpen = false;

    // Garmin transport selection.  AUTOMATIC keeps Garmin Connect as the
    // first choice while it is healthy.  A Connect-IQ transaction is healthy
    // only when an application message comes back from Kerfstok; the SDK's
    // sendMessage SUCCESS callback by itself is not an acknowledgement.
    public static final int GARMIN_TRANSPORT_AUTOMATIC = 0;
    public static final int GARMIN_TRANSPORT_CONNECT = 1;
    public static final int GARMIN_TRANSPORT_DIRECT = 2;
    private static final String GARMIN_TRANSPORT_PREFS = "garmintransport";
    private static final String GARMIN_TRANSPORT_KEY = "mode";
    private static final long AUTOMATIC_DIRECT_DELAY_MS = 12000L;
    private static final long GARMIN_SDK_HARD_RESTART_DELAY_MS = 1200L;
    private static final long GARMIN_SDK_HARD_RESTART_COOLDOWN_MS = 30000L;
    // START can legitimately take several seconds while Kerfstok is opening.
    private static final long AUTOMATIC_REPLY_TIMEOUT_MS = 15000L;
    // Reinit first uses the least destructive recovery available.  If that does
    // not produce a real Kerfstok START reply, automatically escalate to the
    // restart-level recovery instead of making the user press Reinit again.
    private static final long REINIT_HARD_ESCALATION_MS = AUTOMATIC_REPLY_TIMEOUT_MS+2000L;
    // A failback probe is sent only while Direct BLE is paused. Keep this short:
    // a healthy Garmin Connect path normally returns START well within a second.
    private static final long GARMIN_CONNECT_PROBE_TIMEOUT_MS = 5000L;
    // The auxiliary Direct-GATT client first completes a stranded Kerfstok
    // return channel. If ATT itself is stalled, it still leaves that unanswered
    // request alive through Android's native watchdog. The SDK retry is later
    // so it cannot pre-empt either recovery path.
    private static final long GARMIN_LINK_RECOVERY_TIMEOUT_MS =
          DirectGarminIQ.ATT_DISCOVERY_TIMEOUT_MS+4000L;
    private static final long GARMIN_RETRY_WITHOUT_PROBE_MS = 30000L;
    private static final long GARMIN_LINK_RESET_SETTLE_MS = 2500L;
    private static final int GLUCOSE_ACK_TIMESTAMP = 1;
    private static final int GARMIN_LIBRE3_MINUTE = GarminLibre3.LIBRE3_MINUTE;
    private static final int GARMIN_LIBRE3_CLINICAL = GarminLibre3.LIBRE3_CLINICAL;
    private static final int GARMIN_GOT_LIBRE3_MINUTE = GarminLibre3.GOT_LIBRE3_MINUTE;
    private static final int NUMBER_ROLE_CAPABILITY = 2;
    private static final int DISPLAY_THEME_CAPABILITY = 256;
    private static final int DISPLAY_THEME_DARK = 512;

    private static final String GARMIN_PEERS_PREFS = "garminpeers";
    private static final String GARMIN_NUMBERS_ENABLED = "numbers_enabled";
    private static final String GARMIN_DARK_PREFIX = "dark:";
    private static final String GARMIN_ACTIVE_PREFIX = "active:";
    private static final String GARMIN_GLUCOSE_PREFIX = "glucose:";
    private static final String GARMIN_LIBRE3_DIRECT_PREFIX = "libre3direct:";
    private static final long NO_PEER = Long.MIN_VALUE;
    private int numbersGeneration;
    private boolean numbersEnabled=true;
    private boolean numbersPreferenceLoaded;
    private boolean numbersSyncPending;
    // Selecting a different Numbers watch is not an ordinary two-way sync.
    // Juggluco contains the authoritative, longer history; initialize the new
    // watch from Juggluco before accepting history from that watch.
    private boolean numbersAuthorityPushPending;
    private boolean numbersAuthorityPushActive;

    private static final class GarminPeer {
        final long id;
        String name;
        IQDevice device;
        String directAddress;
        boolean connected;
        // User-controlled, local to this phone. Inactive peers remain visible but
        // must not own a Garmin transport or receive application traffic.
        boolean active=true;
        // User preference: whether this phone should send glucose to this watch.
        // Separate from glucoseReady, which is only the current Kerfstok session state.
        boolean glucoseEnabled=true;
        // Explicit per-watch ownership of direct Libre3. Merely installing or
        // running the large app never enables sensor BLE.
        boolean libre3Direct=false;
        // Installation evidence is independent of the selected app/Direct role.
        int libre3Installed=-1;
        boolean libre3InfoPending;
        int libre3InfoGeneration;
        boolean glucoseReady;
        boolean watchStopped;
        boolean startPending;
        boolean timestampedGlucoseAck;
        boolean numberRoleCapable;
        boolean alarmCapable;
        boolean alarmSoundConfigCapable, alarmToneCapable;
        boolean alarmOutputConfigCapable, alarmVibrationCapable;
        int alarmAcknowledgedRevision, alarmRemoteRevision;
        long alarmLastAttempt;
        GarminAlarmConfig alarmConfig;
        int requestedNumberRole=-1;
        int confirmedNumberRole=-1;
        long applicationStateTime;
        int applicationSession;
        List<Object> inflightGlucose;
        List<Object> queuedGlucose;
        List<Object> acknowledgedGlucose;
        int glucoseGeneration;
        int connectGeneration;
        int startGeneration;
        boolean applicationBootstrapPending;
        int applicationBootstrapGeneration;
        // One-shot openApplication request, armed only by the Reinit button.
        boolean launchRequested;
        long lastSend;
        long lastReceived;
        // Measurement time and receipt time are different during backlog replay.
        // Only validated, advancing Libre3 records refresh this in-memory guard.
        // It is checked by the existing loss alarm, never by a new timer.
        long lastLibre3MinuteTime;
        long lastLibre3ClinicalTime;
        long lastLibre3ReturnProgressElapsed=-1L;
        long lastAcknowledged;
        long lastGlucoseAcknowledged;
        long acknowledgedGlucoseTime;
        String lastError;
        long lastStatusTime;
        IQMessageStatus lastStatus;
        int appVersion=-1;
        String appId; // selected Garmin app for this watch; null means ordinary Kerfstok
        boolean appFallbackTried;
        boolean setupRecoveryUsed;
        boolean sdkRetryPending;
        int sdkRetryGeneration;
        // Consecutive Garmin Connect operations that reached the watch transport
        // but received no application reply. Recent GCM versions can remain
        // CONNECTED while Connect IQ messaging is dead; Automatic must then
        // be allowed to hand this peer to Direct BLE.
        int sdkNoReplyCount;
        // While Direct owns the watch, a one-shot Garmin Connect START probe
        // verifies application messaging before ownership is handed back.
        boolean sdkProbePending;
        boolean sdkProbeReinit;
        boolean sdkProbeWasGlucoseReady;
        int sdkProbeGeneration;
        // A Direct -> Garmin Connect handoff must replay a retained Numbers
        // transaction after the new START session, otherwise glucose stays blocked.
        boolean retryNumbersAfterStart;
        // Explicit Reinit is considered complete only after Kerfstok proves the
        // application path with START.  If the conservative first attempt does
        // not get that proof, a guarded timer performs a full restart-level reset.
        boolean reinitRecoveryPending;
        boolean reinitRecoveryWaitingForSetup;
        int reinitRecoveryGeneration;

        GarminPeer(long id,String name) {
            this.id=id;
            this.name=name==null?Long.toString(id):name;
        }
    }

    public static final class GarminDeviceInfo {
        public final long id;
        public final String name;
        public final boolean connected;
        public final boolean direct;
        public final boolean active;
        public final boolean glucose;
        public final boolean libre3Direct;
        public final boolean libre3Installed;
        public final boolean numbers;
        public final long lastSend;
        public final long lastReceived;
        public final long lastStatusTime;
        public final IQMessageStatus lastStatus;
        public final int appVersion;
        public final String communicationStatus;
        public final boolean watchStopped;
        public final boolean timestampedGlucoseAck;
        public final long lastAcknowledged;
        public final long lastGlucoseAcknowledged;
        public final long acknowledgedGlucoseTime;
        public final String lastError;
        GarminDeviceInfo(long id,String name,boolean connected,boolean direct,boolean active,boolean glucose,boolean libre3Direct,boolean libre3Installed,boolean numbers,
                         long lastSend,long lastReceived,long lastStatusTime,IQMessageStatus lastStatus,int appVersion,
                         String communicationStatus,boolean watchStopped,boolean timestampedGlucoseAck,
                         long lastAcknowledged,long lastGlucoseAcknowledged,long acknowledgedGlucoseTime,String lastError) {
            this.id=id; this.name=name; this.connected=connected; this.direct=direct; this.active=active; this.glucose=glucose; this.libre3Direct=libre3Direct; this.numbers=numbers;
            this.libre3Installed=libre3Installed;
            this.lastSend=lastSend; this.lastReceived=lastReceived; this.lastStatusTime=lastStatusTime;
            this.lastStatus=lastStatus; this.appVersion=appVersion;
            this.communicationStatus=communicationStatus; this.watchStopped=watchStopped;
            this.timestampedGlucoseAck=timestampedGlucoseAck; this.lastAcknowledged=lastAcknowledged;
            this.lastGlucoseAcknowledged=lastGlucoseAcknowledged; this.acknowledgedGlucoseTime=acknowledgedGlucoseTime;
            this.lastError=lastError;
        }
        @Override public String toString() {
            return name + (numbers?" [numbers]":"") + (direct?" - Direct BLE":(connected?" - CONNECTED":" - NOT_CONNECTED"));
        }
    }

    private final LinkedHashMap<Long,GarminPeer> garminPeers=new LinkedHashMap<>();
    private final HashMap<Long,DirectGarminIQ> directGarmins=new HashMap<>();
    private final HashMap<Long,DirectGarminIQ> garminLinkProbes=new HashMap<>();
    private final HashMap<Long,Object> directRetryMessages=new HashMap<>();
    private final Handler transportHandler = new Handler(Looper.getMainLooper());
    private int transportMode = -1;
    private int transportGeneration = 0;
    private int automaticTransactionGeneration = 0;
    private Object automaticPendingMessage = null;
    private long automaticPendingPeerId = NO_PEER;
    private Object automaticRetryMessage = null;
    private long automaticRetryPeerId = NO_PEER;
    private Context transportContext = null;
    private int reinitGeneration=0;
    private volatile boolean transportRestarting=false;
    // The designated numbers peer shares one application channel with glucose.
    // SDK SUCCESS only means transport completion; keep this slot until the
    // matching Kerfstok reply (or a no-reply completion).
    private Object numberPendingMessage;


private static String peerName(IQDevice device) {
   if(device==null) return "Garmin";
   try {
      String friendly=device.getFriendlyName();
      return friendly==null?Long.toString(device.getDeviceIdentifier()):friendly;
      }
   catch(Throwable th) { return "Garmin"; }
   }

private static String normalizeGarminName(String name) {
   if(name==null) return "";
   // GCM commonly appends a duplicate counter, e.g. "vívoactive 3 (2)",
   // while Android's bonded-device name is simply "vívoactive3".
   String low=name.toLowerCase(Locale.US).replaceAll("\\s*\\(\\d+\\)\\s*$","");
   StringBuilder out=new StringBuilder();
   for(int i=0;i<low.length();i++) {
      char c=low.charAt(i);
      if(Character.isLetterOrDigit(c)) out.append(c);
      }
   return out.toString();
   }

private static long directFallbackId(String address) {
   long h=0xcbf29ce484222325L;
   if(address!=null) for(int i=0;i<address.length();i++) {
      h^=(long)address.charAt(i);
      h*=0x100000001b3L;
      }
   if(h==-1L || h==NO_PEER) h^=0x5a5a5a5a5a5a5a5aL;
   return h;
   }

private static String normalizeAppId(String id) {
   return id==null?"":id.replace("-","").toLowerCase(Locale.US);
   }
private static String kerfstokAppId() {
   String configured=normalizeAppId(Natives.getgarminid());
   // Early Libre3 setup used the old global app-ID setting. Libre3 now has
   // its own per-watch endpoint: that legacy setting must not replace ordinary
   // Kerfstok as well, or neither transport can reach the store app.
   // Recognize both legacy Libre3 ids here, so a saved release id cannot
   // become the ordinary Kerfstok endpoint in a debug-id build (or vice versa).
   return (LIBRE3_GARMIN_RELEASE_APP_ID.equals(configured) || LIBRE3_GARMIN_DEBUG_APP_ID.equals(configured))?
         normalizeAppId(Natives.getdefaultid()):configured;
   }
private static boolean supportedGarminAppId(String id) {
   String n=normalizeAppId(id);
   return n.equals(normalizeAppId(kerfstokAppId())) || n.equals(LIBRE3_GARMIN_APP_ID);
   }
private String appIdFor(GarminPeer gp) {
   return gp!=null && supportedGarminAppId(gp.appId) ? normalizeAppId(gp.appId) : normalizeAppId(kerfstokAppId());
   }
private IQApp appFor(GarminPeer gp) {
   return appIdFor(gp).equals(LIBRE3_GARMIN_APP_ID) ? mLibre3App : mMyApp;
   }
private boolean isLibre3App(GarminPeer gp) { return appIdFor(gp).equals(LIBRE3_GARMIN_APP_ID); }
private String alternateAppIdFor(GarminPeer gp) {
   return isLibre3App(gp)?normalizeAppId(kerfstokAppId()):LIBRE3_GARMIN_APP_ID;
   }

private String garminAppKey(GarminPeer gp) {
   if(gp!=null && gp.directAddress!=null && gp.directAddress.length()!=0)
      return GARMIN_APP_PREFIX+"addr:"+gp.directAddress.toUpperCase(Locale.US);
   return GARMIN_APP_PREFIX+"id:"+(gp==null?NO_PEER:gp.id);
   }
private void loadGarminApp(Context context,GarminPeer gp) {
   if(context==null || gp==null) return;
   android.content.SharedPreferences prefs=context.getApplicationContext()
         .getSharedPreferences(GARMIN_PEERS_PREFS,Context.MODE_PRIVATE);
   String idkey=GARMIN_APP_PREFIX+"id:"+gp.id;
   String key=garminAppKey(gp);
   String id=prefs.getString(key,null);
   if(id==null && !key.equals(idkey)) id=prefs.getString(idkey,null);
   gp.appId=supportedGarminAppId(id)?normalizeAppId(id):normalizeAppId(kerfstokAppId());
   if(gp.libre3Installed<0) gp.libre3Installed=prefs.getInt(libre3InstalledKey(gp),
         prefs.getInt("libre3_installed:id:"+gp.id,-1));
   }

private String libre3InstalledKey(GarminPeer gp) {
   return "libre3_installed:"+(gp.directAddress==null?"id:"+gp.id:"addr:"+gp.directAddress.toUpperCase(Locale.US));
   }

private synchronized void recordLibre3Installation(GarminPeer gp,String appId,boolean installed) {
   if(gp==null || !LIBRE3_GARMIN_APP_ID.equals(normalizeAppId(appId))) return;
   gp.libre3InfoPending=false;
   ++gp.libre3InfoGeneration;
   int value=installed?1:0;
   if(gp.libre3Installed==value) return;
   gp.libre3Installed=value;
   if(transportContext!=null) transportContext.getSharedPreferences(GARMIN_PEERS_PREFS,Context.MODE_PRIVATE)
         .edit().putInt(libre3InstalledKey(gp),value).putInt("libre3_installed:id:"+gp.id,value).apply();
   }

/** Query installation without selecting or launching that app. Existing
 * selected-app bootstrap owns its own app-info listener, so do not replace it. */
private synchronized void queryLibre3Installation(GarminPeer gp) {
   if(gp==null || gp.device==null || !gp.connected || gp.libre3InfoPending ||
         !acceptsGarminConnectPeer(gp.id) || !sdkready() || mConnectIQ==null ||
         (gp.applicationBootstrapPending && isLibre3App(gp))) return;
   final ConnectIQ sdk=mConnectIQ;
   final int token=++gp.libre3InfoGeneration;
   gp.libre3InfoPending=true;
   try {
      sdk.getApplicationInfo(LIBRE3_GARMIN_APP_ID,gp.device,new IQApplicationInfoListener() {
         private void result(String id,boolean installed) {
            synchronized(AllData.this) {
               if(sdk!=mConnectIQ || token!=gp.libre3InfoGeneration) return;
               recordLibre3Installation(gp,id,installed);
               }
            }
         @Override public void onApplicationInfoReceived(IQApp app) {
            if(app!=null) result(app.getApplicationId(),true);
            }
         @Override public void onApplicationNotInstalled(String id) { result(id,false); }
         });
      transportHandler.postDelayed(() -> {
         synchronized(AllData.this) { if(token==gp.libre3InfoGeneration) {
            gp.libre3InfoPending=false; ++gp.libre3InfoGeneration;
            } }
         },15000L);
      }
   catch(Throwable th) {
      gp.libre3InfoPending=false; ++gp.libre3InfoGeneration;
      Log.stack(LOG_ID,"Libre3 installed-app query "+gp.name,th);
      }
   }
private void persistGarminApp(Context context,GarminPeer gp) {
   if(context==null || gp==null) return;
   android.content.SharedPreferences.Editor edit=context.getApplicationContext()
         .getSharedPreferences(GARMIN_PEERS_PREFS,Context.MODE_PRIVATE).edit()
         .putString(GARMIN_APP_PREFIX+"id:"+gp.id,appIdFor(gp));
   if(gp.directAddress!=null && gp.directAddress.length()!=0)
      edit.putString(GARMIN_APP_PREFIX+"addr:"+gp.directAddress.toUpperCase(Locale.US),appIdFor(gp));
   edit.apply();
   }

private void registerBothAppEvents(ConnectIQ sdk,GarminPeer gp) {
   if(!Natives.getusegarmin() || sdk==null || gp==null || gp.device==null) return;
   if(mMyApp!=null) try { sdk.registerForAppEvents(gp.device,mMyApp,IQlistener); }
      catch(Throwable th) { Log.stack(LOG_ID,"register Kerfstok app "+gp.name,th); }
   if(mLibre3App!=null) try { sdk.registerForAppEvents(gp.device,mLibre3App,IQlistener); }
      catch(Throwable th) { Log.stack(LOG_ID,"register Libre3 app "+gp.name,th); }
   }
private synchronized void unregisterBothAppEvents(ConnectIQ sdk,GarminPeer gp) {
   if(sdk==null || gp==null || gp.device==null) return;
   // GCM can still accept watch transfers while Direct is reconnecting. Its
   // physical subscription survives SDK listener removal, so keep the route
   // into Juggluco for Libre3 records even while Direct owns outgoing traffic.
   if(acceptsGarminConnectReturn(gp)) return;
   // SDK 2.4.0 unregisterForApplicationEvents(device,app) also calls the
   // Garmin Connect service's unregisterApp(app,package): that registration
   // is shared by every watch, despite the device argument of the public API.
   // Keep it while another peer needs SDK delivery (including a failback probe).
   // Incoming guards still reject inactive peers and late command replies.
   for(GarminPeer other:garminPeers.values()) {
      if(other.id!=gp.id && other.device!=null && acceptsGarminConnectIncoming(other.id)) {
         if(doLog) Log.i(LOG_ID,"Keep shared Garmin app events for "+other.name+
               " while detaching "+gp.name);
         return;
         }
      }
   unregisterSharedAppEvents(sdk,gp);
   }

/** Use after the last SDK consumer leaves, or during complete SDK shutdown. */
private void unregisterSharedAppEvents(ConnectIQ sdk,GarminPeer gp) {
   if(sdk==null || gp==null || gp.device==null) return;
   if(mMyApp!=null) try { sdk.unregisterForApplicationEvents(gp.device,mMyApp); } catch(Throwable ignored) {}
   if(mLibre3App!=null) try { sdk.unregisterForApplicationEvents(gp.device,mLibre3App); } catch(Throwable ignored) {}
   }

/** Select which of the two compatible Garmin apps receives ordinary Kerfstok traffic.
 * GarminLibre3 calls this when it provisions the large app. A START from the other
 * app can switch the selection back while Garmin Connect owns the transport. */
public synchronized boolean selectGarminApplication(Context context,long peerId,String appId) {
   if(!supportedGarminAppId(appId)) return false;
   GarminPeer gp=garminPeers.get(peerId);
   if(gp==null) return false;
   String next=normalizeAppId(appId);
   if(appIdFor(gp).equals(next)) return true;
   if(doLog) Log.i(LOG_ID,"Garmin app["+gp.name+"] "+appIdFor(gp)+" -> "+next);
   DirectGarminIQ direct=directGarmins.get(peerId);
   // Both compatible UUIDs live on the same physical Garmin link. Do not tear
   // that link down merely because the foreground Connect-IQ app changes.
   if(direct!=null && !direct.selectApplication(next)) {
      directGarmins.remove(peerId);
      try { direct.stop(); } catch(Throwable ignored) {}
      direct=null;
      }
   stopGarminLinkProbe(peerId);
   gp.appId=next;
   gp.appFallbackTried=false;
   persistGarminApp(context,gp);
   gp.glucoseReady=false;
   gp.startPending=false;
   gp.applicationBootstrapPending=false;
   ++gp.applicationBootstrapGeneration;
   ++gp.startGeneration;
   ++gp.glucoseGeneration;
   if(mConnectIQ!=null && gp.device!=null && currentTransportMode()!=GARMIN_TRANSPORT_DIRECT)
      registerBothAppEvents(mConnectIQ,gp);
   return true;
   }

/** Raw Garmin SDK broadcasts are accepted only for these two Juggluco apps.
 * While Direct BLE owns a watch, accept only the app whose UUID Direct owns. */
synchronized boolean acceptsGarminApplication(long peerId,String appId) {
   if(!supportedGarminAppId(appId)) return false;
   GarminPeer gp=garminPeers.get(peerId);
   if(gp==null) return true;
   if(directActive(gp)) return appIdFor(gp).equals(normalizeAppId(appId));
   return true;
   }

private GarminPeer peer(long id) { return garminPeers.get(id); }
private GarminPeer peer(IQDevice device) {
   if(device==null) return null;
   return garminPeers.get(device.getDeviceIdentifier());
   }
private long numbersPeerId() { return numbersEnabled?numio.getident():NO_PEER; }
private boolean isNumbersPeer(long id) { return id!=NO_PEER && id==numbersPeerId(); }
private GarminPeer numbersPeer() { return garminPeers.get(numbersPeerId()); }

private void loadNumbersPreference(Context context) {
   if(numbersPreferenceLoaded) return;
   numbersEnabled=context.getApplicationContext().getSharedPreferences(GARMIN_PEERS_PREFS,Context.MODE_PRIVATE)
         .getBoolean(GARMIN_NUMBERS_ENABLED,true);
   numbersPreferenceLoaded=true;
   }

private void ensureNumbersPeer() {
   // Keep the stored watch/history identity even when no watch uses Numbers.
   // Discovery must never undo the user's separate disabled setting.
   long id=numio.getident();
   if(id!=-1L && garminPeers.containsKey(id)) {
      updateDevused();
      return;
      }
   if(id==-1L && !garminPeers.isEmpty()) {
      GarminPeer first=garminPeers.values().iterator().next();
      numio.setident(first.id);
      }
   updateDevused();
   }

private void updateDevused() {
   devused=0;
   if(devices==null) return;
   long id=numio.getident();
   for(int i=0;i<devices.size();i++) {
      if(devices.get(i).getDeviceIdentifier()==id) { devused=i; return; }
      }
   }

private GarminPeer soleSdkPeer() {
   GarminPeer one=null;
   for(GarminPeer gp:garminPeers.values()) if(gp.device!=null) {
      if(one!=null) return null;
      one=gp;
      }
   return one;
   }

private void mergeSamePhysicalPeer(GarminPeer from,GarminPeer to) {
   if(from==null || to==null || from==to) return;
   if(to.directAddress==null) to.directAddress=from.directAddress;
   if(to.queuedGlucose==null) to.queuedGlucose=from.queuedGlucose;
   if(to.inflightGlucose==null) to.inflightGlucose=from.inflightGlucose;
   to.glucoseEnabled=from.glucoseEnabled;
   to.glucoseReady|=from.glucoseReady;
   if(from.applicationStateTime>to.applicationStateTime) {
      to.applicationStateTime=from.applicationStateTime;
      to.watchStopped=from.watchStopped;
      to.glucoseReady=from.glucoseReady;
      to.startPending=from.startPending;
      to.timestampedGlucoseAck=from.timestampedGlucoseAck;
      to.numberRoleCapable=from.numberRoleCapable;
      to.requestedNumberRole=from.requestedNumberRole;
      to.confirmedNumberRole=from.confirmedNumberRole;
      }
   to.lastAcknowledged=Math.max(to.lastAcknowledged,from.lastAcknowledged);
   if(from.lastGlucoseAcknowledged>to.lastGlucoseAcknowledged) {
      to.lastGlucoseAcknowledged=from.lastGlucoseAcknowledged;
      to.acknowledgedGlucoseTime=from.acknowledgedGlucoseTime;
      }
   to.lastSend=Math.max(to.lastSend,from.lastSend);
   to.lastReceived=Math.max(to.lastReceived,from.lastReceived);
   to.lastLibre3MinuteTime=Math.max(to.lastLibre3MinuteTime,from.lastLibre3MinuteTime);
   to.lastLibre3ClinicalTime=Math.max(to.lastLibre3ClinicalTime,from.lastLibre3ClinicalTime);
   to.lastLibre3ReturnProgressElapsed=Math.max(to.lastLibre3ReturnProgressElapsed,
         from.lastLibre3ReturnProgressElapsed);
   to.setupRecoveryUsed|=from.setupRecoveryUsed;
   to.launchRequested|=from.launchRequested;
   if(from.lastStatusTime>to.lastStatusTime) { to.lastStatusTime=from.lastStatusTime; to.lastStatus=from.lastStatus; }
   if(to.appVersion<0) to.appVersion=from.appVersion;
   DirectGarminIQ stale=directGarmins.remove(from.id);
   if(stale!=null) try { stale.stop(); } catch(Throwable ignored) {}
   DirectGarminIQ staleProbe=garminLinkProbes.remove(from.id);
   if(staleProbe!=null) try { staleProbe.stop(); } catch(Throwable ignored) {}
   Object retry=directRetryMessages.remove(from.id);
   if(retry!=null && !directRetryMessages.containsKey(to.id)) directRetryMessages.put(to.id,retry);
   if(numio.getident()==from.id) numio.setident(to.id);
   garminPeers.remove(from.id);
   }

private void setDirectAddress(GarminPeer target,String addr) {
   if(target==null || addr==null) return;
   // A physical BLE address can belong to exactly one logical Garmin peer.
   // Clear stale aliases before assigning it.  Without this guard two
   // DirectGarminIQ instances can connect to the same watch and interleave the
   // Connect IQ application protocol.
   for(GarminPeer gp:new ArrayList<>(garminPeers.values())) {
      if(gp==target || gp.directAddress==null || !addr.equalsIgnoreCase(gp.directAddress)) continue;
      if(doLog) Log.i(LOG_ID,"clear duplicate Direct Garmin address "+addr+
            " from peer="+gp.id+" "+gp.name+"; owner="+target.id+" "+target.name);
      gp.directAddress=null;
      DirectGarminIQ stale=directGarmins.remove(gp.id);
      if(stale!=null) try { stale.stop(); } catch(Throwable ignored) {}
      DirectGarminIQ staleProbe=garminLinkProbes.remove(gp.id);
      if(staleProbe!=null) try { staleProbe.stop(); } catch(Throwable ignored) {}
      }
   target.directAddress=addr;
   }

private void associateBondedGarmins(Context context) {
   loadNumbersPreference(context);
   final List<BluetoothDevice> bonded=DirectGarminIQ.bondedGarmins(context);
   final android.content.SharedPreferences prefs=context.getApplicationContext()
         .getSharedPreferences(GARMIN_PEERS_PREFS,Context.MODE_PRIVATE);
   final long existingNumbers=numio.getident();
   final GarminPeer soleSdk=soleSdkPeer();
   for(BluetoothDevice bt:bonded) {
      String addr=bt.getAddress();
      String bname=DirectGarminIQ.bondedName(bt);
      long mapped=prefs.getLong("addr:"+addr,NO_PEER);
      GarminPeer mappedPeer=mapped==NO_PEER?null:garminPeers.get(mapped);
      GarminPeer target=null;

      // Prefer the physical watch name before any single-device fallback.
      // This is essential while one of several watches is temporarily unbonded:
      // the remaining bonded watch must not be attached to the Numbers peer just
      // because it is the only entry returned by getBondedDevices().
      {
         String norm=normalizeGarminName(bname);
         GarminPeer unique=null;
         for(GarminPeer gp:garminPeers.values()) {
            if(normalizeGarminName(gp.name).equals(norm)) {
               if(unique!=null) { unique=null; break; }
               unique=gp;
               }
            }
         target=unique;
         }

      // One GCM device + one bonded Garmin is unambiguous only when the name did
      // not already identify another peer.
      if(target==null && bonded.size()==1 && soleSdk!=null) target=soleSdk;

      // Preserve the old direct-only Numbers identity only in the genuinely
      // single-peer case.  With two watches, "one currently bonded" does NOT
      // mean that bonded watch is the Numbers peer (the other watch may simply
      // have been unpaired for re-pairing).
      if(target==null && bonded.size()==1 && soleSdk==null && existingNumbers!=-1L &&
            garminPeers.size()==1) {
         target=garminPeers.get(existingNumbers);
         if(target==null) {
            target=new GarminPeer(existingNumbers,bname);
            garminPeers.put(existingNumbers,target);
            }
         }

      // A persisted mapping to a real GCM peer remains authoritative when the
      // topology is ambiguous (for example two identical Edge units).
      if(target==null && mappedPeer!=null && mappedPeer.device!=null) target=mappedPeer;

      if(target==null) {
         long id=mapped==NO_PEER?directFallbackId(addr):mapped;
         target=garminPeers.get(id);
         if(target==null) {
            target=new GarminPeer(id,bname);
            garminPeers.put(id,target);
            }
         }

      if(mappedPeer!=null && mappedPeer!=target && mappedPeer.device==null)
         mergeSamePhysicalPeer(mappedPeer,target);

      // Pairing/scanner versions could persist the Numbers selection under a
      // direct-only logical peer id.  After removing Juggluco pairing support,
      // that transient peer may no longer exist when this bonded watch is
      // rediscovered as its Garmin SDK peer.  The persisted address mapping (or
      // the deterministic direct fallback id) still identifies the same physical
      // watch, so migrate the Numbers identity before overwriting addr:<address>.
      // Do this even while Numbers is disabled: numio keeps the remembered watch
      // independently of the enabled flag.
      final long storedNumbers=numio.getident();
      if(storedNumbers!=-1L && target.id!=storedNumbers &&
            (mapped==storedNumbers || directFallbackId(addr)==storedNumbers)) {
         if(doLog) Log.i(LOG_ID,"migrate Numbers Garmin peer "+storedNumbers+
               " -> "+target.id+" for "+bname+" "+addr);
         numio.setident(target.id);
         }

      setDirectAddress(target,addr);
      if((target.name==null || target.name.length()==0 || target.name.equals(Long.toString(target.id))) && bname!=null)
         target.name=bname;
      prefs.edit().putLong("addr:"+addr,target.id).apply();
      }
   }

private String garminActiveKey(GarminPeer gp) {
   if(gp!=null && gp.directAddress!=null && gp.directAddress.length()!=0)
      return GARMIN_ACTIVE_PREFIX+"addr:"+gp.directAddress.toUpperCase(Locale.US);
   return GARMIN_ACTIVE_PREFIX+"id:"+(gp==null?NO_PEER:gp.id);
   }

private void loadGarminActive(Context context,GarminPeer gp) {
   if(context==null || gp==null) return;
   android.content.SharedPreferences prefs=context.getApplicationContext()
         .getSharedPreferences(GARMIN_PEERS_PREFS,Context.MODE_PRIVATE);
   String key=garminActiveKey(gp);
   String idkey=GARMIN_ACTIVE_PREFIX+"id:"+gp.id;
   final boolean was=gp.active;
   if(prefs.contains(key)) gp.active=prefs.getBoolean(key,true);
   else if(!key.equals(idkey) && prefs.contains(idkey)) {
      gp.active=prefs.getBoolean(idkey,true);
      prefs.edit().putBoolean(key,gp.active).apply();
      }
   else gp.active=true;
   if(doLog && was!=gp.active)
      Log.i(LOG_ID,"loaded Garmin peer "+gp.name+" active="+gp.active+" key="+key);
   }

/** Enforce the persisted Active switch on transport objects which may have been
 *  created before a peer was re-associated with its bonded BLE address.  Merely
 *  loading active=false is not sufficient: DirectGarminIQ owns its own reconnect
 *  timer and would otherwise keep reconnecting an already inactive watch. */
private void stopInactiveGarminTransports() {
   for(GarminPeer gp:new ArrayList<>(garminPeers.values())) {
      if(gp.active) continue;
      ++gp.connectGeneration;
      DirectGarminIQ direct=directGarmins.remove(gp.id);
      if(direct!=null) {
         if(doLog) Log.i(LOG_ID,"stop Direct Garmin for inactive peer "+gp.name);
         try { direct.stop(); } catch(Throwable ignored) {}
         }
      DirectGarminIQ probe=garminLinkProbes.remove(gp.id);
      if(probe!=null) {
         if(doLog) Log.i(LOG_ID,"stop Garmin recovery probe for inactive peer "+gp.name);
         try { probe.stop(); } catch(Throwable ignored) {}
         }
      directRetryMessages.remove(gp.id);
      if(automaticRetryPeerId==gp.id) { automaticRetryPeerId=NO_PEER; automaticRetryMessage=null; }
      if(automaticPendingPeerId==gp.id) cancelAutomaticApplicationReply();
      gp.sdkRetryPending=false;
      gp.applicationBootstrapPending=false;
      gp.startPending=false;
      gp.glucoseReady=false;
      ++gp.sdkRetryGeneration;
      ++gp.applicationBootstrapGeneration;
      ++gp.startGeneration;
      }
   }

private void persistGarminActive(Context context,GarminPeer gp) {
   if(context==null || gp==null) return;
   android.content.SharedPreferences.Editor edit=context.getApplicationContext()
         .getSharedPreferences(GARMIN_PEERS_PREFS,Context.MODE_PRIVATE).edit()
         .putBoolean(GARMIN_ACTIVE_PREFIX+"id:"+gp.id,gp.active);
   if(gp.directAddress!=null && gp.directAddress.length()!=0)
      edit.putBoolean(GARMIN_ACTIVE_PREFIX+"addr:"+gp.directAddress.toUpperCase(Locale.US),gp.active);
   edit.apply();
   }

private String garminGlucoseKey(GarminPeer gp) {
   if(gp!=null && gp.directAddress!=null && gp.directAddress.length()!=0)
      return GARMIN_GLUCOSE_PREFIX+"addr:"+gp.directAddress.toUpperCase(Locale.US);
   return GARMIN_GLUCOSE_PREFIX+"id:"+(gp==null?NO_PEER:gp.id);
   }

private void loadGarminGlucose(Context context,GarminPeer gp) {
   if(context==null || gp==null) return;
   android.content.SharedPreferences prefs=context.getApplicationContext()
         .getSharedPreferences(GARMIN_PEERS_PREFS,Context.MODE_PRIVATE);
   String key=garminGlucoseKey(gp);
   String idkey=GARMIN_GLUCOSE_PREFIX+"id:"+gp.id;
   if(prefs.contains(key)) gp.glucoseEnabled=prefs.getBoolean(key,true);
   else if(!key.equals(idkey) && prefs.contains(idkey)) {
      gp.glucoseEnabled=prefs.getBoolean(idkey,true);
      prefs.edit().putBoolean(key,gp.glucoseEnabled).apply();
      }
   else gp.glucoseEnabled=true; // preserve the historical default: glucose is sent once Kerfstok is ready
   }

private void persistGarminGlucose(Context context,GarminPeer gp) {
   if(context==null || gp==null) return;
   android.content.SharedPreferences.Editor edit=context.getApplicationContext()
         .getSharedPreferences(GARMIN_PEERS_PREFS,Context.MODE_PRIVATE).edit()
         .putBoolean(GARMIN_GLUCOSE_PREFIX+"id:"+gp.id,gp.glucoseEnabled);
   if(gp.directAddress!=null && gp.directAddress.length()!=0)
      edit.putBoolean(GARMIN_GLUCOSE_PREFIX+"addr:"+gp.directAddress.toUpperCase(Locale.US),gp.glucoseEnabled);
   edit.apply();
   }

public synchronized boolean canConfigureGarminAlarms(long peerId) {
   GarminPeer gp=garminPeers.get(peerId);
   return gp!=null && (isLibre3App(gp) || gp.libre3Installed>0);
   }

public synchronized GarminAlarmConfig getGarminAlarms(Context context,long peerId) {
   GarminPeer gp=garminPeers.get(peerId);
   if(gp==null || context==null) return null;
   gp.alarmConfig=GarminAlarms.load(context,gp.id,gp.directAddress);
   return gp.alarmConfig;
   }

public synchronized boolean setGarminAlarms(Context context,long peerId,GarminAlarmConfig config) {
   GarminPeer gp=garminPeers.get(peerId);
   if(gp==null || context==null || config==null) return false;
   GarminAlarmConfig old=getGarminAlarms(context,peerId);
   int revision=Math.max(old.revision,gp.alarmRemoteRevision);
   if(revision==Integer.MAX_VALUE) return false;
   GarminAlarmConfig next=config.withRevision(revision+1);
   if(!GarminAlarms.save(context,gp.id,gp.directAddress,next)) return false;
   gp.alarmConfig=next;
   gp.alarmAcknowledgedRevision=0;
   gp.alarmLastAttempt=0L;
   flushGarminAlarms(gp);
   return true;
   }

public synchronized int garminAlarmStatus(long peerId) {
   GarminPeer gp=garminPeers.get(peerId);
   if(gp==null || !gp.alarmCapable) return tk.glucodata.R.string.garmin_alarm_open;
   if(gp.alarmConfig!=null && gp.alarmAcknowledgedRevision==gp.alarmConfig.revision &&
         !gp.alarmOutputConfigCapable &&
         ((gp.alarmConfig.flags&~gp.alarmConfig.vibrationFlags)!=0 ||
          ((gp.alarmConfig.flags&1)!=0 && gp.alarmConfig.lowRepeat==0) ||
          ((gp.alarmConfig.flags&2)!=0 && gp.alarmConfig.highRepeat==0)))
      return tk.glucodata.R.string.garmin_alarm_outputs_update;
   if(gp.alarmConfig!=null && gp.alarmAcknowledgedRevision==gp.alarmConfig.revision &&
         gp.alarmConfig.soundFlags!=0 && !gp.alarmSoundConfigCapable)
      return tk.glucodata.R.string.garmin_alarm_sound_update;
   if(gp.alarmConfig!=null && gp.alarmAcknowledgedRevision==gp.alarmConfig.revision)
      return tk.glucodata.R.string.garmin_alarm_saved;
   if(gp.alarmConfig!=null && gp.alarmRemoteRevision>gp.alarmConfig.revision)
      return tk.glucodata.R.string.garmin_alarm_newer;
   return tk.glucodata.R.string.garmin_alarm_waiting;
   }

/** -1: not reported, 0: older app, 1: no tone hardware, 2: tones supported. */
public synchronized int garminAlarmSoundSupport(long peerId) {
   GarminPeer gp=garminPeers.get(peerId);
   if(gp==null || !gp.alarmCapable) return -1;
   if(!gp.alarmSoundConfigCapable) return 0;
   return gp.alarmToneCapable?2:1;
   }

/** -1: not reported, 0: older app, 1: no vibration hardware, 2: selectable vibration. */
public synchronized int garminAlarmVibrationSupport(long peerId) {
   GarminPeer gp=garminPeers.get(peerId);
   if(gp==null || !gp.alarmCapable) return -1;
   if(!gp.alarmOutputConfigCapable) return 0;
   return gp.alarmVibrationCapable?2:1;
   }

private static int alarmMessageVersion(GarminPeer gp) {
   return gp.alarmOutputConfigCapable?3:gp.alarmSoundConfigCapable?2:1;
   }

private void flushGarminAlarms(GarminPeer gp) {
   if(gp==null || !gp.active || gp.watchStopped || !gp.alarmCapable || !isLibre3App(gp) || transportContext==null) return;
   if(gp.alarmConfig==null) getGarminAlarms(transportContext,gp.id);
   if(gp.alarmConfig==null || gp.alarmAcknowledgedRevision==gp.alarmConfig.revision ||
         gp.alarmRemoteRevision>gp.alarmConfig.revision) return;
   long now=SystemClock.elapsedRealtime();
   if(gp.alarmLastAttempt!=0L && now-gp.alarmLastAttempt<60000L) return;
   gp.alarmLastAttempt=now;
   if(!sendPeerObject(gp,gp.alarmConfig.message(alarmMessageVersion(gp)),true,false)) gp.alarmLastAttempt=0L;
   }

private void receiveGarminAlarmAck(GarminPeer gp,List<?> reply) {
   if(gp.alarmConfig==null || !GarminAlarmConfig.replyMatches(gp.alarmConfig.message(alarmMessageVersion(gp)),reply)) return;
   int status=(Integer)reply.get(2), revision=(Integer)reply.get(3);
   gp.alarmRemoteRevision=Math.max(gp.alarmRemoteRevision,revision);
   if(status==0 && revision==gp.alarmConfig.revision) {
      gp.alarmAcknowledgedRevision=revision;
      if(doLog) Log.i(LOG_ID,"Watch alarms saved peer="+gp.id+" revision="+revision);
      }
   else if(doLog) Log.i(LOG_ID,"Watch alarms not saved peer="+gp.id+" status="+status+" revision="+revision);
   GarminAlarms.statusChanged(this,gp.id);
   }

private String garminLibre3DirectKey(GarminPeer gp) {
   if(gp!=null && gp.directAddress!=null && gp.directAddress.length()!=0)
      return GARMIN_LIBRE3_DIRECT_PREFIX+"addr:"+gp.directAddress.toUpperCase(Locale.US);
   return GARMIN_LIBRE3_DIRECT_PREFIX+"id:"+(gp==null?NO_PEER:gp.id);
   }

private void loadGarminLibre3Direct(Context context,GarminPeer gp) {
   if(context==null || gp==null) return;
   android.content.SharedPreferences prefs=context.getApplicationContext()
         .getSharedPreferences(GARMIN_PEERS_PREFS,Context.MODE_PRIVATE);
   String key=garminLibre3DirectKey(gp);
   String idkey=GARMIN_LIBRE3_DIRECT_PREFIX+"id:"+gp.id;
   if(prefs.contains(key)) gp.libre3Direct=prefs.getBoolean(key,false);
   else if(!key.equals(idkey) && prefs.contains(idkey)) {
      gp.libre3Direct=prefs.getBoolean(idkey,false);
      prefs.edit().putBoolean(key,gp.libre3Direct).apply();
      }
   else gp.libre3Direct=false;
   }

private void persistGarminLibre3Direct(Context context,GarminPeer gp) {
   if(context==null || gp==null) return;
   android.content.SharedPreferences.Editor edit=context.getApplicationContext()
         .getSharedPreferences(GARMIN_PEERS_PREFS,Context.MODE_PRIVATE).edit()
         .putBoolean(GARMIN_LIBRE3_DIRECT_PREFIX+"id:"+gp.id,gp.libre3Direct);
   if(gp.directAddress!=null && gp.directAddress.length()!=0)
      edit.putBoolean(GARMIN_LIBRE3_DIRECT_PREFIX+"addr:"+gp.directAddress.toUpperCase(Locale.US),gp.libre3Direct);
   edit.apply();
   }

private void recomputeSendtowatch() {
   boolean any=false;
   for(GarminPeer gp:garminPeers.values()) if(gp.active && gp.glucoseEnabled) { any=true; break; }
   sendtowatch=any;
   }

private void recomputeUsewatch() {
   boolean any=false;
   for(GarminPeer gp:garminPeers.values()) if(gp.active && (gp.device!=null || gp.directAddress!=null)) { any=true; break; }
   usewatch=any;
   }

/** Rebuild logical peers from Garmin Connect and Android's already-bonded watches. */
private void rebuildGarminPeers(Context context) {
   LinkedHashMap<Long,GarminPeer> old=new LinkedHashMap<>(garminPeers);
   garminPeers.clear();
   if(devices!=null) {
      for(IQDevice d:devices) {
         long id=d.getDeviceIdentifier();
         GarminPeer gp=old.get(id);
         final boolean existing=gp!=null;
         if(gp==null) gp=new GarminPeer(id,peerName(d));
         gp.device=d;
         gp.name=peerName(d);
         // getKnownDevices() returns fresh IQDevice objects whose getStatus() can
         // lag behind the IQDeviceEventListener.  Do not turn an existing live
         // CONNECTED peer into NOT_CONNECTED merely because Garmin Status
         // refreshed the device list.  A real status callback remains the
         // authority for disconnecting an existing peer.  For a new peer, and
         // for a positive CONNECTED observation, the snapshot is still useful.
         try {
            boolean snapshotConnected=d.getStatus()==IQDeviceStatus.CONNECTED;
            if(!existing || snapshotConnected) gp.connected=snapshotConnected;
            } catch(Throwable ignored) {}
         garminPeers.put(id,gp);
         }
      }
   // Preserve direct-only peers and then discover newly bonded Garmin devices.
   for(GarminPeer gp:old.values()) if(gp.directAddress!=null && !garminPeers.containsKey(gp.id))
      garminPeers.put(gp.id,gp);
   associateBondedGarmins(context);
   for(GarminPeer gp:garminPeers.values()) { loadGarminActive(context,gp); loadGarminGlucose(context,gp); loadGarminLibre3Direct(context,gp); loadGarminApp(context,gp); }
   stopInactiveGarminTransports();
   ensureNumbersPeer();
   recomputeUsewatch();
   recomputeSendtowatch();
   }

public synchronized List<GarminDeviceInfo> getGarminDeviceInfos() {
   ArrayList<GarminDeviceInfo> out=new ArrayList<>();
   long numbers=numbersPeerId();
   for(GarminPeer gp:garminPeers.values()) {
      DirectGarminIQ direct=directGarmins.get(gp.id);
      boolean activeDirect=gp.active && direct!=null && direct.isActive();
      boolean connected=gp.active && (activeDirect?direct.isReady():gp.connected && sdkready());
      out.add(new GarminDeviceInfo(gp.id,gp.name,connected,activeDirect,gp.active,gp.glucoseEnabled,gp.libre3Direct,gp.libre3Installed==1,gp.id==numbers,
            gp.lastSend,gp.lastReceived,gp.lastStatusTime,gp.lastStatus,gp.appVersion,
            communicationStatus(gp,connected,activeDirect),gp.watchStopped,gp.timestampedGlucoseAck,
            gp.lastAcknowledged,gp.lastGlucoseAcknowledged,gp.acknowledgedGlucoseTime,gp.lastError));
      }
   return out;
   }

private String communicationStatus(GarminPeer gp,boolean connected,boolean direct) {
   if(gp.watchStopped) return "Kerfstok closed";
   if(gp.sdkRetryPending || garminLinkProbes.containsKey(gp.id)) return "Recovering communication";
   if(gp.lastError!=null) return direct?"Recovering communication":"Communication error";
   if(!connected) return direct?"Connecting":"Not connected";
   if(gp.applicationBootstrapPending || gp.startPending) return "Waiting for Kerfstok START";
   if(!gp.glucoseReady) return "Connected; Kerfstok not yet ready";
   if(gp.inflightGlucose!=null) return "Waiting for glucose acknowledgement";
   if(isNumbersPeer(gp.id) && numberPendingMessage!=null) return "Waiting for history acknowledgement";
   if(gp.lastAcknowledged>0) return "Two-way communication confirmed";
   if(gp.lastReceived>0) return "Receiving; replies not yet confirmed";
   if(gp.lastSend>0) return "Sending; replies not yet confirmed";
   return "Connected; communication not yet confirmed";
   }

private void peerAcknowledged(GarminPeer gp) {
   if(gp==null) return;
   gp.lastAcknowledged=System.currentTimeMillis();
   gp.lastError=null;
   // A late valid reply proves that Garmin Connect's application channel is
   // working again.  Cancel both the passive bearer probe and its stale retry;
   // otherwise the already acknowledged operation could be sent a second time.
   if(gp.sdkRetryPending) {
      gp.sdkRetryPending=false;
      ++gp.sdkRetryGeneration;
      directRetryMessages.remove(gp.id);
      if(automaticRetryPeerId==gp.id) {
         automaticRetryPeerId=NO_PEER;
         automaticRetryMessage=null;
         }
      }
   stopGarminLinkProbe(gp.id);
   }

public long getNumbersDeviceId() { return numbersPeerId(); }
public String getNumbersDeviceName() {
   GarminPeer gp=numbersPeer();
   return gp==null?"None":gp.name;
   }

public synchronized boolean setNumbersDevice(Context context,long id) {
   return setNumbersDeviceEnabled(context,id,true);
   }

public synchronized boolean setNumbersDeviceEnabled(Context context,long id,boolean enabled) {
   loadNumbersPreference(context);
   GarminPeer gp=garminPeers.get(id);
   if(gp==null) return false;
   long old=numbersPeerId();
   if(enabled?old==id:old!=id) return true;
   {if(doLog) {Log.i(LOG_ID,"numbers device "+old+" -> "+(enabled?id:"off"));};};
   cancelAutomaticApplicationReply();
   clearsyncgegs();
   ++numbersGeneration;
   directRetryMessages.clear();
   if(enabled && numio.getident()!=id) numio.setident(id);
   numbersEnabled=enabled;
   // A newly selected/re-enabled Numbers watch must first be initialized from
   // Juggluco.  The ordinary sync starts by asking the watch for NUMS and would
   // let its much shorter history overwrite the local database.
   numbersAuthorityPushPending=enabled;
   numbersAuthorityPushActive=false;
   numbersSyncPending=false;
   context.getApplicationContext().getSharedPreferences(GARMIN_PEERS_PREFS,Context.MODE_PRIVATE)
         .edit().putBoolean(GARMIN_NUMBERS_ENABLED,enabled).apply();
   updateDevused();
   // A running capable watch can apply the role directly. An extra START
   // would pause glucose and add another round trip before the actual change.
   for(GarminPeer p:garminPeers.values())
      if(p.active && p.numberRoleCapable && !p.watchStopped) sendPeerRole(p);
   if(enabled && gp.active && (!gp.numberRoleCapable || gp.watchStopped)) requestPeerStart(gp);
   return true;
   }

private DirectGarminIQ directFor(long peerId) { return directGarmins.get(peerId); }
private boolean directActive(GarminPeer gp) {
   DirectGarminIQ direct=gp==null?null:directGarmins.get(gp.id);
   return direct!=null && direct.isActive();
   }
private boolean directReady(GarminPeer gp) {
   DirectGarminIQ direct=gp==null?null:directGarmins.get(gp.id);
   return direct!=null && direct.isReady();
   }

private boolean sendPeerObject(GarminPeer gp,Object message,boolean expectReply,boolean numberTransaction) {
   // Per-watch Active preferences survive the main Kerfstok switch being off.
   // Sending an alarm dismissal must not recreate a stopped Garmin transport.
   if(!Natives.getusegarmin() || gp==null || !gp.active || gp.watchStopped) return false;
   int mode=currentTransportMode();
   DirectGarminIQ direct=directGarmins.get(gp.id);
   if(mode==GARMIN_TRANSPORT_DIRECT || (mode==GARMIN_TRANSPORT_AUTOMATIC && direct!=null && direct.isActive())) {
      if(direct==null) {
         if(transportContext!=null) startDirectPeer(transportContext,gp);
         direct=directGarmins.get(gp.id);
         }
      if(direct==null) return false;
      gp.lastSend=System.currentTimeMillis();
      return expectReply?direct.send(message):direct.sendNoReply(message);
      }
   if(mConnectIQ==null || !sdkready() || gp.device==null || !gp.connected) {
      if(mode==GARMIN_TRANSPORT_AUTOMATIC && transportContext!=null && gp.directAddress!=null && !directActive(gp))
         startDirectPeer(transportContext,gp);
      return false;
      }
   try {
      gp.lastSend=System.currentTimeMillis();
      final GarminPeer target=gp;
      final Object sent=message;
      final int startToken=gp.startGeneration;
      final int sessionToken=gp.applicationSession;
      if(numberTransaction && expectReply && mode!=GARMIN_TRANSPORT_DIRECT)
         armAutomaticApplicationReply(gp.id,message);
      mConnectIQ.sendMessage(gp.device,appFor(gp),message,new IQSendMessageListener() {
         @Override public void onMessageStatus(IQDevice device, IQApp app, IQMessageStatus status) {
            if(target.watchStopped || sessionToken!=target.applicationSession || !acceptsGarminConnectPeer(target.id)) return;
            sendstatus=status;
            statustime=System.currentTimeMillis();
            target.lastStatus=status;
            target.lastStatusTime=statustime;
            if(doLog) Log.i(LOG_ID,"mConnectIQ.sendMessage["+target.name+"]:"+sent+" "+status.name());
            if(status!=IQMessageStatus.SUCCESS) {
               if(numberTransaction) {
                  if(numberPendingMessage!=sent) return;
                  if(expectReply) automaticGarminSendFailed(target.id,sent,status.name());
                  else automaticFallbackForFailedMessage(target.id,sent,"Garmin Connect send "+status.name());
                  }
               else if(isGlucoseMessage(sent) && ((List<?>)sent).get(1)==target.inflightGlucose)
                  peerGarminSendFailed(target,"glucose "+status.name());
               else if((messageKind(sent)==START || messageKind(sent)==NUMBERROLE) &&
                     startToken==target.startGeneration && target.startPending)
                  peerStartGarminSendFailed(target,"START "+status.name());
               }
            else if(numberTransaction && !expectReply) onGarminNoReplyComplete(target.id,sent);
            }
         });
      return true;
      }
   catch(Throwable th) {
      Log.stack(LOG_ID,"sendPeerObject "+gp.name,th);
      if(numberTransaction) {
         if(expectReply) automaticGarminSendFailed(gp.id,message,th.getClass().getSimpleName());
         else automaticFallbackForFailedMessage(gp.id,message,th.getClass().getSimpleName());
         }
      else if(isGlucoseMessage(message)) peerGarminSendFailed(gp,"glucose exception");
      return false;
      }
   }

private static int messageKind(Object message) {
   if(!(message instanceof List<?>)) return Integer.MIN_VALUE;
   List<?> li=(List<?>)message;
   return !li.isEmpty() && li.get(0) instanceof Integer ? (Integer)li.get(0) : Integer.MIN_VALUE;
   }
private static boolean isGlucoseMessage(Object message) { return messageKind(message)==GLUCOSE; }

int numbersGeneration() { return numbersGeneration; }

boolean acceptsQueuedGarminMessage(long peerId,Object message,int generation) {
   if(!Natives.getusegarmin()) return false;
   switch(messageKind(message)) {
      case GarminAlarmConfig.SET:
         GarminPeer alarmPeer=garminPeers.get(peerId);
         return alarmPeer!=null && alarmPeer.alarmCapable && isLibre3App(alarmPeer) &&
               alarmPeer.alarmConfig!=null && alarmPeer.alarmConfig.message(alarmMessageVersion(alarmPeer)).equals(message);
      case LIBRE3DIRECT:
         GarminPeer directPeer=garminPeers.get(peerId);
         List<?> directSetting=(List<?>)message;
         // A queued OFF from before a handoff must not undo the current ON.
         return directPeer!=null && directSetting.size()==2 &&
               Integer.valueOf(directPeer.libre3Direct?1:0).equals(directSetting.get(1));
      case GarminLibre3Lifecycle.COMMAND:
         return GarminLibre3Lifecycle.current(peerId,message);
      case NUMS: case MORENUMS: case PUTNUMS: case GETENDNUM: case SETENDNUM:
      case DELETE: case DELETED: case PUTLABELS: case PUTPRECISION: case SHORTCUTS:
         // Do not replay a former authority's history after changing watches,
         // even if the same watch is selected again before its queue drains.
         return generation==numbersGeneration && isNumbersPeer(peerId);
      case NUMBERROLE:
         GarminPeer gp=garminPeers.get(peerId);
         List<?> role=(List<?>)message;
         return gp!=null && gp.numberRoleCapable && role.size()==2 &&
               Integer.valueOf(isNumbersPeer(peerId)?1:0).equals(role.get(1));
      default: return true;
      }
   }

// A reverse connection is also used for watch actions, not just replies. Keep
// an unrelated incoming action from completing an outstanding Direct request.
static boolean directReplyMatches(Object sent,Object received) {
   int request=messageKind(sent), reply=messageKind(received);
   if(request==Integer.MIN_VALUE || reply==Integer.MIN_VALUE) return false;
   if(reply==SENDERROR) return true;
   switch(request) {
      case GarminAlarmConfig.SET: return GarminAlarmConfig.replyMatches(sent,received);
      case START: return reply==START;
      case NUMBERROLE:
         if(reply!=START) return false;
         // A legacy reply also ends this request after a watch downgrade.
         // onPeerMessage then clears the advertised capability.
         if(!hasStartCapability((List<?>)received,NUMBER_ROLE_CAPABILITY)) return true;
         List<?> roleRequest=(List<?>)sent;
         return roleRequest.size()==2 && roleRequest.get(1) instanceof Integer &&
               ((Integer)roleRequest.get(1))==startNumberRole((List<?>)received);
      case GLUCOSE: return reply==START || glucoseReplyMatches(sent,received,false);
      case STOPALARM: return reply==GOTSTOPALARM;
      case COLORBLACK: return reply==COLORBLACK;
      case PUTLABELS: return reply==consts.consts.RECEIVEDLABELS;
      case PUTPRECISION: return reply==RECEIVEDPRECISION;
      case SHORTCUTS: return reply==RECEIVEDCUTS;
      case consts.consts.GLUNITS: return reply==RECEIVEDUNITS;
      case SETENDNUM: return reply==DIDSETENDNUM;
      case GETENDNUM:
         return reply==COLORBLACK || (reply==SETENDNUM && sameMessageBase(sent,received));
      case PUTNUMS: return reply==GOTNUMS && sameMessageRange(sent,received);
      case DELETE: return reply==DELETED && sameMessageRange(sent,received);
      case NUMS:
      case MORENUMS:
         return (reply==NUMS || reply==NOMORENUMS) && sameMessageBase(sent,received);
      default: return false;
      }
   }

private static boolean glucoseReplyMatches(Object sent,Object received,boolean requireTimestamp) {
   if(messageKind(sent)!=GLUCOSE || messageKind(received)!=GOTGLUCOSE) return false;
   List<?> request=(List<?>)sent,ack=(List<?>)received;
   if(request.size()!=2 || !(request.get(1) instanceof List<?>)) return false;
   List<?> sample=(List<?>)request.get(1);
   if(sample.size()<2 || !(sample.get(1) instanceof Number)) return false;
   if(ack.size()==1) return !requireTimestamp;
   if(ack.size()!=2 || !(ack.get(1) instanceof Integer || ack.get(1) instanceof Long)) return false;
   return ((Number)ack.get(1)).longValue()==((Number)sample.get(1)).longValue();
   }

boolean matchesGarminReply(long peerId,Object sent,Object received) {
   GarminPeer gp=garminPeers.get(peerId);
   if(messageKind(sent)==GLUCOSE && messageKind(received)==GOTGLUCOSE)
      return glucoseReplyMatches(sent,received,gp!=null && gp.timestampedGlucoseAck);
   return directReplyMatches(sent,received);
   }

private static boolean sameMessageBase(Object a,Object b) {
   List<?> left=(List<?>)a,right=(List<?>)b;
   return left.size()>1 && right.size()>1 && left.get(1) instanceof Integer && left.get(1).equals(right.get(1));
   }

private static boolean sameMessageRange(Object a,Object b) {
   List<?> left=(List<?>)a,right=(List<?>)b;
   return sameMessageBase(a,b) && left.size()>3 && right.size()>3 &&
         left.get(2).equals(right.get(2)) && left.get(3).equals(right.get(3));
   }

static boolean directWatchInitiatedMessage(Object message) {
   switch(messageKind(message)) {
      case GarminAlarmConfig.ACK:
      case START: case STOP: case STOPALARM: case STRING: case SENDERROR:
      case NUMS: case HAVENUMS: case DELETE: case GARMIN_LIBRE3_MINUTE: case GARMIN_LIBRE3_CLINICAL: case GarminLibre3Lifecycle.ACK:
         return true;
      default: return false;
      }
   }

private static boolean messageNeedsReply(Object message) {
   // [DELETED,base,index] acknowledges a watch-side deletion. Kerfstok's
   // deletedsend() stores the acknowledgement and sends nothing back.
   return !(messageKind(message)==DELETED && ((List<?>)message).size()==3);
   }

private void sendPeerRole(GarminPeer gp) {
   if(gp==null || !gp.active || !gp.numberRoleCapable) return;
   int role=isNumbersPeer(gp.id)?1:0;
   // Changing one watch's role must not send a redundant NUMBERROLE to every
   // other watch. Apart from unnecessary radio traffic, a reply from an
   // unchanged peer can collide with Garmin Connect's legacy return channel.
   if(gp.confirmedNumberRole==role) return;
   if(gp.startPending && gp.requestedNumberRole==role) return;
   sendPeerStartMessage(gp,Arrays.asList(NUMBERROLE,role),role);
   }

private void sendPeerStart(GarminPeer gp) {
   // START only opens the phone session. A local false preference (including
   // on another phone) is not an instruction to stop the watch's sensor.
   // Always discover support from Kerfstok itself, including after reinstall.
   sendPeerStartMessage(gp,Arrays.asList(START),-1);
   }

private static boolean hasStartCapability(List<?> message,int capability) {
   return message.size()>2 && message.get(2) instanceof Integer &&
         (((Integer)message.get(2)) & capability)!=0;
   }

private static int startNumberRole(List<?> message) {
   if(message.size()<4 || !(message.get(3) instanceof Integer)) return -1;
   int role=(Integer)message.get(3);
   return role==0 || role==1 ? role : -1;
   }

private void sendPeerStartMessage(GarminPeer gp,Object request,int requestedRole) {
   if(gp==null || !gp.active || gp.watchStopped) return;
   // NUMBERROLE does not restart a working glucose session.
   if(requestedRole<0) {
      gp.glucoseReady=false;
      gp.acknowledgedGlucose=null;
      }
   gp.startPending=true;
   gp.requestedNumberRole=requestedRole;
   if(requestedRole>=0) gp.confirmedNumberRole=-1;
   final int token=++gp.startGeneration;
   // Both START and NUMBERROLE require a START reply from Kerfstok. The
   // NUMBERROLE reply must report the applied role before history can resume.
   Runnable sendStart=() -> {
      if(token!=gp.startGeneration) return;
      boolean ok=sendPeerObject(gp,request,true,false);
      if(!ok) {
         if(currentTransportMode()==GARMIN_TRANSPORT_AUTOMATIC && transportContext!=null && !directActive(gp))
            startDirectPeer(transportContext,gp);
         return;
         }
      armPeerStartReply(gp,token);
      };
   if(directActive(gp)) sendStart.run();
   else transportHandler.postDelayed(sendStart,60L);
   }

private void armPeerStartReply(GarminPeer gp,int token) {
   if(gp==null || !gp.active || currentTransportMode()==GARMIN_TRANSPORT_DIRECT || directActive(gp)) return;
   transportHandler.postDelayed(() -> {
      if(currentTransportMode()==GARMIN_TRANSPORT_DIRECT || token!=gp.startGeneration ||
            !gp.startPending || directActive(gp) || transportContext==null) return;
      if(doLog) {
         Log.i(LOG_ID,"Garmin Connect: no START/role acknowledgement from "+gp.name);
         if(garminSdkContext!=null) Log.i(LOG_ID,garminSdkContext.incomingDiagnostics());
         }
      gp.lastError="No Kerfstok START acknowledgement";
      recoverGarminConnectPeer(gp);
      },AUTOMATIC_REPLY_TIMEOUT_MS);
   }

private void peerStartGarminSendFailed(GarminPeer gp,String reason) {
   if(gp==null || !gp.active || currentTransportMode()==GARMIN_TRANSPORT_DIRECT || transportContext==null || directActive(gp)) return;
   ++gp.startGeneration;
   gp.lastError=reason;
   if(doLog) Log.i(LOG_ID,"Garmin Connect: "+gp.name+" "+reason);
   recoverGarminConnectPeer(gp);
   }

private synchronized void queueGlucoseForPeer(GarminPeer gp,List<Object> glucose) {
   if(gp==null || !gp.active || !gp.glucoseEnabled) return;
   // While this watch owns Libre3 directly, its display must be driven by the
   // sensor session, not by a late phone sample. sendglucose() still updates the
   // global glucosemess, so the newest phone value is available immediately when
   // DIRECT is turned off again.
   if(gp.libre3Direct) return;
   // Repeated native broadcasts of one sample must not start repeated [214]
   // transactions, or replace a newer sample already waiting for its turn.
   if(glucose.equals(gp.inflightGlucose) || glucose.equals(gp.acknowledgedGlucose)) return;
   if(!sendtowatch || !gp.glucoseReady || gp.inflightGlucose!=null ||
         (isNumbersPeer(gp.id) && numberPendingMessage!=null)) {
      gp.queuedGlucose=glucose;
      return;
      }
   List<Object> message=Arrays.asList(GLUCOSE,glucose);
   gp.queuedGlucose=null;
   gp.inflightGlucose=glucose;
   DirectGarminIQ direct=directGarmins.get(gp.id);
   // When Automatic Direct shares the physical watch link with Garmin Connect,
   // both clients otherwise answer Kerfstok's [229] return channel with different
   // F105 keys. The VA3 trace shows that race twice and the next app-open then
   // becomes unresponsive. For a glucose-only peer the completed encrypted
   // outbound transfer is sufficient proof of delivery; let Garmin Connect own
   // the redundant [229] return channel and do not race it.
   boolean directTransportOnlyAck=direct!=null && direct.isActive() &&
         direct.shouldAvoidGlucoseReplyRace() && !isNumbersPeer(gp.id);
   if(!sendPeerObject(gp,message,!directTransportOnlyAck,false)) {
      gp.inflightGlucose=null;
      gp.queuedGlucose=glucose;
      if(currentTransportMode()==GARMIN_TRANSPORT_AUTOMATIC && transportContext!=null)
         startDirectPeer(transportContext,gp);
      return;
      }
   if(!directTransportOnlyAck) armPeerGlucoseReply(gp,glucose);
   }

private void flushPeerGlucose(GarminPeer gp) {
   if(gp==null || !gp.active || !gp.glucoseEnabled || !sendtowatch || !gp.glucoseReady || gp.inflightGlucose!=null) return;
   // Only a genuinely newer queued value is sent here.  v9 incorrectly fell
   // back to glucosemess after every [229], creating an endless resend loop.
   List<Object> pending=gp.queuedGlucose;
   if(pending!=null) {
      gp.queuedGlucose=null;
      queueGlucoseForPeer(gp,pending);
      }
   }

private void peerGotGlucose(GarminPeer gp,Object reply) {
   if(gp==null || !gp.active || gp.watchStopped || gp.inflightGlucose==null) return;
   if(!glucoseReplyMatches(Arrays.asList(GLUCOSE,gp.inflightGlucose),reply,gp.timestampedGlucoseAck)) {
      if(doLog) Log.i(LOG_ID,"ignore glucose acknowledgement for another sample["+gp.name+"]: "+reply);
      return;
      }
   peerAcknowledged(gp);
   gp.lastGlucoseAcknowledged=gp.lastAcknowledged;
   gp.acknowledgedGlucoseTime=((Number)gp.inflightGlucose.get(1)).longValue();
   if(gp.queuedGlucose!=null && gp.queuedGlucose.equals(gp.inflightGlucose)) gp.queuedGlucose=null;
   gp.glucoseGeneration++;
   gp.acknowledgedGlucose=gp.inflightGlucose;
   gp.inflightGlucose=null;
   gp.lastReceived=System.currentTimeMillis();
   if(isNumbersPeer(gp.id)) nextmessage();
   flushPeerGlucose(gp);
   }

private void armPeerGlucoseReply(GarminPeer gp,List<Object> glucose) {
   if(currentTransportMode()==GARMIN_TRANSPORT_DIRECT || directActive(gp)) return;
   final int token=++gp.glucoseGeneration;
   transportHandler.postDelayed(() -> {
      if(currentTransportMode()==GARMIN_TRANSPORT_DIRECT || token!=gp.glucoseGeneration ||
            gp.inflightGlucose!=glucose || directActive(gp)) return;
      {if(doLog) {Log.i(LOG_ID,"Garmin Connect: no glucose acknowledgement from "+gp.name);};};
      gp.lastError="No glucose acknowledgement";
      // Keep the transaction identifiable while return-channel recovery runs. A
      // late matching [229,timestamp] can then complete it and cancel recovery.
      // retryGarminConnectPending moves it back to the queue only when a resend
      // is actually necessary; a newer queued sample remains newest-wins.
      recoverGarminConnectPeer(gp);
      },AUTOMATIC_REPLY_TIMEOUT_MS);
   }

private void peerGarminSendFailed(GarminPeer gp,String reason) {
   if(gp==null || !gp.active || currentTransportMode()==GARMIN_TRANSPORT_DIRECT || transportContext==null) return;
   if(gp.queuedGlucose==null) gp.queuedGlucose=gp.inflightGlucose;
   gp.inflightGlucose=null;
   gp.glucoseGeneration++;
   gp.lastError=reason;
   {if(doLog) {Log.i(LOG_ID,"Garmin Connect: "+gp.name+" "+reason);};};
   recoverGarminConnectPeer(gp);
   }

private void broadcastGlucose(List<Object> glucose) {
   boolean any=false;
   for(GarminPeer gp:garminPeers.values()) {
      if(!gp.active || !gp.glucoseEnabled) continue;
      any=true;
      queueGlucoseForPeer(gp,glucose);
      }
   if(!any) glucosemess=glucose;
   }



private void applicationOpened(GarminPeer gp,IQOpenApplicationStatus status) {
           appmissing=-1;
           gp.applicationBootstrapPending=false;
           ++gp.applicationBootstrapGeneration;
           if (status == IQOpenApplicationStatus.APP_IS_ALREADY_RUNNING) {
                Applic.argToaster(getApplication(), "APP_IS_ALREADY_RUNNING", Toast.LENGTH_SHORT);
                mAppIsOpen = true;
            } else {
                Applic.argToaster(getApplication(), "Open App", Toast.LENGTH_SHORT);
                mAppIsOpen = false;
            }
           // Bootstrap this peer independently.  START's reply marks it ready
           // for glucose; only the designated numbers peer enters sync().
           sendPeerStart(gp);
    }
public boolean sendtowatch=false;
private List<Object> glucosemess=null;

private static boolean gotgotglucose=false;
public synchronized void sendglucose(String ident,long tim,float glu,float rate,int off) {
   List<Object> uit = Arrays.asList(ident,tim,glu,rate,off,Applic.unit==1?1:0);
   glucosemess=uit; // latest value for peers that reconnect later
   if(!sendtowatch) return;
   if(transportRestarting) {
      for(GarminPeer gp:garminPeers.values()) if(gp.active && gp.glucoseEnabled) gp.queuedGlucose=uit;
      return;
      }
   broadcastGlucose(uit);
   }

void realsendendnum(int base) {
   {if(doLog) {Log.i(LOG_ID, "realsendendnum "+base);};};
   sendends[base]=false;
   int end= numio.getlastnum( base) ;
   realsendmessage(Arrays.asList(SETENDNUM,base,end));
   }

boolean[] sendends={false,false};

 void sendendnum(int base) {
   if(didreceivebackup(base))  {
      return;
      }
    if(!isSending()) {
       setSending(true);
       realsendendnum(base);
       }
    else {
        log("sendend later "+base);
         sendends[base]=true;
        sendmessages();
    }
   }
/** The one watch selected to own direct Libre3 sensors.  The setting is
 * deliberately independent of whether the large app is installed/running. */
public synchronized long getLibre3DirectPeerId() {
   for(GarminPeer gp:garminPeers.values()) if(gp.libre3Direct) return gp.id;
   return NO_PEER;
   }

public synchronized boolean isGarminLibre3Direct(long peerId) {
   GarminPeer gp=garminPeers.get(peerId);
   return gp!=null && gp.libre3Direct;
   }

/** An empty/incomplete peer table at SDK startup is not a user deselection. */
public synchronized boolean libre3LifecycleReady(long savedPeer) {
   if(transportContext==null || garminPeers.isEmpty() ||
         (savedPeer!=NO_PEER && !garminPeers.containsKey(savedPeer))) return false;
   for(GarminPeer gp:garminPeers.values()) loadGarminLibre3Direct(transportContext,gp);
   return true;
   }

/** Select direct-Libre3 ownership.  For now ownership is exclusive per phone:
 * one watch can own all provisioned Libre3 sensors. This prevents two watches
 * racing for the same sensor while still allowing the large app to run on all. */
public synchronized boolean setGarminLibre3Direct(Context context,long peerId,boolean enabled) {
   if(context==null || peerId==NO_PEER) return false;
   GarminPeer target=garminPeers.get(peerId);
   if(target==null) return false;
   Context app=context.getApplicationContext();
   transportContext=app;

   // Libre3 direct belongs to the integrated Libre3/Kerfstok application.  Do
   // not rely on whichever compatible app happened to send the most recent
   // START: with both apps installed that can leave DIRECT=0 addressed to the
   // small Kerfstok app while the integrated app keeps trying to own the sensor.
   if(!selectGarminApplication(app,peerId,LIBRE3_GARMIN_APP_ID)) return false;
   target=garminPeers.get(peerId);
   if(target==null) return false;

   if(enabled) {
      for(GarminPeer gp:garminPeers.values()) {
         if(gp.id==peerId || !gp.libre3Direct) continue;
         gp.libre3Direct=false;
         persistGarminLibre3Direct(app,gp);
         // Best effort for an old owner. If it is still the integrated app,
         // make it release its sensor before ownership moves to this peer.
         if(isLibre3App(gp)) sendPeerObject(gp,Arrays.asList(LIBRE3DIRECT,0),false,false);
         }
      target.libre3Direct=true;
      persistGarminLibre3Direct(app,target);
      // Ordinary phone glucose must not race the direct sensor during handoff.
      target.inflightGlucose=null;
      target.queuedGlucose=null;
      ++target.glucoseGeneration;
      if(doLog) Log.i(LOG_ID,"Libre3 direct ON for "+target.name+" app="+appIdFor(target));
      // Do not send DIRECT=1 yet. Provisioning must be delivered first so a
      // stale stored sensor cannot be connected during a watch switch.
      GarminLibre3Lifecycle.kick();
      return true;
      }

   target.libre3Direct=false;
   persistGarminLibre3Direct(app,target);
   GarminLibre3.restorePhoneSensorBluetooth(app);
   GarminLibre3Lifecycle.kick();
   if(doLog) Log.i(LOG_ID,"Libre3 direct OFF for "+target.name+"; resume phone glucose through integrated app");

   // Only this explicit OFF action sends DIRECT=0. Ordinary START/reconnect
   // must not turn sensor reception off. The durable lifecycle STOP retries
   // an assigned sensor's release if the transport loses this command.
   sendPeerObject(target,Arrays.asList(LIBRE3DIRECT,0),false,false);
   if(target.glucoseEnabled && glucosemess!=null)
      target.queuedGlucose=glucosemess;
   if(target.active) {
      requestPeerStart(target);
      }
   return true;
   }

/** Send a Libre3-only control/provisioning message to one explicitly selected
 * Garmin watch using the same transport (Garmin Connect or Direct BLE) as the
 * normal Kerfstok protocol. */
public synchronized boolean sendLibre3Message(Context context,long peerId,Object message) {
   GarminPeer gp=garminPeers.get(peerId);
   if(gp==null || !gp.active || !gp.libre3Direct) return false;
   if(!selectGarminApplication(context,peerId,LIBRE3_GARMIN_APP_ID)) return false;
   return sendPeerObject(gp,message,false,false);
   }

/** Lifecycle STOP can still be delivered after the direct role was released.
 * Active remains a hard per-watch switch. The durable caller retries on START. */
public synchronized boolean sendLibre3Lifecycle(long peerId,List<Integer> message) {
   GarminPeer gp=garminPeers.get(peerId);
   if(gp==null || !gp.active || gp.watchStopped || transportContext==null ||
         !GarminLibre3Lifecycle.current(peerId,message)) return false;
   if(message.get(2)==1 && !gp.libre3Direct) return false;
   if(!selectGarminApplication(transportContext,peerId,LIBRE3_GARMIN_APP_ID)) return false;
   return sendPeerObject(gp,message,false,false);
   }

public synchronized boolean setGarminActive(Context context,long peerId,boolean active) {
   if(context==null || peerId==NO_PEER) return false;
   GarminPeer gp=garminPeers.get(peerId);
   if(gp==null) return false;
   final Context app=context.getApplicationContext();
   transportContext=app;
   if(gp.active==active) {
      persistGarminActive(app,gp);
      // Reassert the invariant even when the UI value did not change.  This
      // cleans up a stale DirectGarminIQ created before active=false was loaded.
      if(!active) stopInactiveGarminTransports();
      return true;
      }
   gp.active=active;
   persistGarminActive(app,gp);
   if(doLog) Log.i(LOG_ID,"Garmin peer "+gp.name+" active="+active);

   if(!active) {
      // Active controls phone/watch communication only. Retain Direct and
      // sensor ownership so the watch can keep receiving from its sensor.
      // Only explicit Direct OFF or sensor end enables phone sensor Bluetooth.
      DirectGarminIQ direct=directGarmins.remove(peerId);
      if(direct!=null) try { direct.stop(); } catch(Throwable ignored) {}
      stopGarminLinkProbe(peerId);
      directRetryMessages.remove(peerId);
      if(automaticRetryPeerId==peerId) { automaticRetryPeerId=NO_PEER; automaticRetryMessage=null; }
      if(automaticPendingPeerId==peerId) cancelAutomaticApplicationReply();
      if(isNumbersPeer(peerId)) {
         if(numberPendingMessage!=null) { mqueue.addFirst(numberPendingMessage); numberPendingMessage=null; }
         setSending(false);
         numbersSyncPending=numbersEnabled;
         }
      if(gp.queuedGlucose==null) gp.queuedGlucose=gp.inflightGlucose;
      gp.inflightGlucose=null;
      gp.glucoseReady=false;
      gp.startPending=false;
      gp.applicationBootstrapPending=false;
      gp.sdkRetryPending=false;
      ++gp.startGeneration; ++gp.glucoseGeneration; ++gp.applicationBootstrapGeneration; ++gp.sdkRetryGeneration;
      if(mConnectIQ!=null && gp.device!=null) {
         unregisterBothAppEvents(mConnectIQ,gp);
         try { mConnectIQ.unregisterForDeviceEvents(gp.device); } catch(Throwable ignored) {}
         }
      recomputeUsewatch();
      recomputeSendtowatch();
      return true;
      }

   // Re-enable only this peer. Do not disturb any other watch.
   gp.watchStopped=false;
   gp.lastError=null;
   if(gp.glucoseEnabled && glucosemess!=null) gp.queuedGlucose=glucosemess;
   if(gp.device!=null) {
      try { gp.connected=gp.device.getStatus()==IQDeviceStatus.CONNECTED; } catch(Throwable ignored) {}
      }
   int mode=currentTransportMode();
   if(mode!=GARMIN_TRANSPORT_DIRECT && mConnectIQ!=null && gp.device!=null) {
      try { mConnectIQ.registerForDeviceEvents(gp.device,mDeviceEventListener); } catch(Throwable th) { Log.stack(LOG_ID,"activate register device "+gp.name,th); }
      registerBothAppEvents(mConnectIQ,gp);
      if(gp.connected) registerPeerApplication(app,gp);
      else if(mode==GARMIN_TRANSPORT_AUTOMATIC && gp.directAddress!=null) {
         final int token=++gp.connectGeneration;
         transportHandler.postDelayed(() -> {
            if(gp.active && token==gp.connectGeneration && !gp.connected && !directActive(gp)) startDirectPeer(app,gp);
            },AUTOMATIC_DIRECT_DELAY_MS);
         }
      }
   else if(mode!=GARMIN_TRANSPORT_CONNECT && gp.directAddress!=null) startDirectPeer(app,gp);
   recomputeUsewatch();
   recomputeSendtowatch();
   return true;
   }

public synchronized boolean isGarminActive(long peerId) {
   GarminPeer gp=garminPeers.get(peerId);
   return Natives.getusegarmin() && gp!=null && gp.active;
   }

private String kerfstokDarkKey(long peerId) {
   GarminPeer gp=garminPeers.get(peerId);
   if(gp!=null && gp.directAddress!=null && gp.directAddress.length()!=0)
      return GARMIN_DARK_PREFIX+"addr:"+gp.directAddress.toUpperCase(Locale.US);
   return GARMIN_DARK_PREFIX+"id:"+peerId;
   }

public synchronized boolean getKerfstokBlack(Context context,long peerId) {
   if(context==null || peerId==NO_PEER) return Natives.getkerfstokblack();
   android.content.SharedPreferences prefs=context.getApplicationContext()
         .getSharedPreferences(GARMIN_PEERS_PREFS,Context.MODE_PRIVATE);
   String key=kerfstokDarkKey(peerId);
   // Explicit choices always win, including one made before Direct BLE found
   // the address of a Garmin Connect peer.
   if(prefs.contains(key)) return prefs.getBoolean(key,false);
   String idKey=GARMIN_DARK_PREFIX+"id:"+peerId;
   if(prefs.contains(idKey)) return prefs.getBoolean(idKey,false);
   if(prefs.contains("default:"+key)) return prefs.getBoolean("default:"+key,false);
   if(prefs.contains("default:"+idKey)) return prefs.getBoolean("default:"+idKey,false);
   // An old watch app cannot report its display theme; retain legacy behavior.
   return Natives.getkerfstokblack();
   }

private void receiveKerfstokTheme(GarminPeer gp,List<?> message) {
   if(gp==null || transportContext==null || !hasStartCapability(message,DISPLAY_THEME_CAPABILITY)) return;
   android.content.SharedPreferences prefs=transportContext.getApplicationContext()
         .getSharedPreferences(GARMIN_PEERS_PREFS,Context.MODE_PRIVATE);
   String key=kerfstokDarkKey(gp.id);
   String idKey=GARMIN_DARK_PREFIX+"id:"+gp.id;
   boolean dark=hasStartCapability(message,DISPLAY_THEME_DARK);
   String defaultKey="default:"+key;
   if(!prefs.contains(defaultKey) || prefs.getBoolean(defaultKey,false)!=dark)
      prefs.edit().putBoolean(defaultKey,dark).apply();
   // Keep reports separate from user choices. A fresh install uses the watch's
   // display-dependent theme; a saved choice also survives offline changes or
   // a watch-app reinstall. No extra START/reconnection or timer is needed.
   String choiceKey=prefs.contains(key)?key:idKey;
   if(prefs.contains(choiceKey)) {
      boolean chosen=prefs.getBoolean(choiceKey,false);
      if(chosen!=dark && usewatch && gp.active)
         sendPeerObject(gp,Arrays.asList(COLORBLACK,chosen?1:0),true,false);
      }
   }

public synchronized void setcolor(Context context,long peerId,boolean black) {
   GarminPeer gp=garminPeers.get(peerId);
   if(gp==null || context==null) return;
   context.getApplicationContext().getSharedPreferences(GARMIN_PEERS_PREFS,Context.MODE_PRIVATE)
         .edit().putBoolean(kerfstokDarkKey(peerId),black).apply();
   if(usewatch && gp.active) sendPeerObject(gp,Arrays.asList(COLORBLACK,black?1:0),true,false);
   }

/** Legacy caller: keep broadcast semantics, but Garmin Status uses the peer overload. */
public void setcolor(boolean black) {
   if(!usewatch) return;
   Object message=Arrays.asList(COLORBLACK,black?1:0);
   for(GarminPeer gp:garminPeers.values()) if(gp.active) sendPeerObject(gp,message,true,false);
   }
boolean[] asksendends={false,false};
private void realaskendnum(int base) {
   asksendends[base]=false;
   realsendmessage(Arrays.asList(GETENDNUM,base));
   }
private void askendnum(int base) {
   if(didreceivebackup(base)) 
      return;
    if(!isSending()) {
   setSending(true);
   realaskendnum(base);
       }
    else {
        log("askendend later "+base);
   asksendends[base]=true;
        sendmessages();
    }
   }
   /*
public void resendtowatch() {
   Natives.resendtowatch();
   changedback(1);
   sendendnum(1);
   changedback(0);
   sendendnum(0);
   }*/
void false2(boolean[] ar) {
   ar[0]=ar[1]=false;
   }
void clearsyncgegs() {
   // If transport/Reinit interrupts the one-way initialization, restart it from
   // Juggluco after the next confirmed START/NUMBERROLE instead of falling back
   // to ordinary watch->phone synchronization.
   if(numbersAuthorityPushActive) {
      numbersAuthorityPushActive=false;
      numbersAuthorityPushPending=numbersEnabled;
      }
   false2(askednums);
   false2(asksendends);
   false2(backchanged);
   false2(sendends);
   sendgoal[0]=sendgoal[1]=0;
   mqueue.clear();
   numberPendingMessage=null;
   setSending(false);
}

/**
 * Initialize a newly selected Numbers watch from Juggluco without first reading
 * anything from that watch.  A watch only retains maxstorage records per base,
 * so send the newest locally available watch-sized window and then SETENDNUM to
 * Juggluco's end.  SETENDNUM also truncates stale records when the new watch is
 * ahead of Juggluco.
 */
private void initializeNumbersWatchFromJuggluco() {
   GarminPeer gp=numbersPeer();
   if(!numbersEnabled || gp==null || !gp.active || !gp.glucoseReady) {
      numbersAuthorityPushPending=numbersEnabled;
      numbersAuthorityPushActive=false;
      return;
      }
   if(gp.numberRoleCapable && gp.confirmedNumberRole!=1) {
      numbersAuthorityPushPending=true;
      numbersAuthorityPushActive=false;
      return;
      }

   {if(doLog) {Log.i(LOG_ID,"initialize Numbers watch from Juggluco: "+gp.name);};};
   clearsyncgegs();
   numbersAuthorityPushPending=false;
   numbersAuthorityPushActive=true;

   // The complete authoritative transfer makes pending per-watch delete ranges
   // obsolete; deleted records in Juggluco are sent as null in PUTNUMS.
   dodelete[0][0]=dodelete[0][1]=-1;
   dodelete[1][0]=dodelete[1][1]=-1;

   // Configuration belongs to Juggluco as well. Queue it after the history.
   Natives.setshouldsendlabels(true);
   Natives.setsendcuts(true);

   for(int base=1;base>=0;--base) {
      long ptr=numio.numptrs[base];
      int last=numio.getlastnum(base);
      int first=Natives.getfirstNum(ptr);
      int begin=Math.max(first,Math.max(0,last-maxstorage));
      Object data=putnummer(base,begin,last);
      if(data!=null) sendmessage(data);
      // Do not use sendendnum(): didreceivebackup() is relevant to ordinary
      // merging, but this operation deliberately makes Juggluco authoritative.
      sendmessage(Arrays.asList(SETENDNUM,base,last));
      }

   sendlabels();
   sendshortcuts(Natives.getShortcuts());
   // If both bases and configuration were empty, make sure completion is seen.
   sendmessages();
}


private void setSendlast(int base) {
    long ptr=numio.numptrs[base];
    int last=Natives.getlastNum(ptr); 
    int back=Math.max(0,last-20);
    {if(doLog) {Log.i(LOG_ID,base+" setSendlast last="+last+" back="+back);};};
    Natives.setchangedNum(ptr,back);
    }
private void givebase0nums() {
    Log.i(LOG_ID,"givebase0nums()"); 
    Natives.setshouldsendlabels(true);
    Natives.setsendcuts(true);
    setSendlast(0);
    setSendlast(1);

    final int last = numio.getlastnum(0);
    if(last>0) {
        changedback(0);
        }
    else {
          sendmessage(Arrays.asList(MORENUMS,0, 0));
        }
    // Numbers synchronization must not alter any watch's Glucose preference.
    sync();
    }
public void sync(long peerId) {
   // Number/history synchronization is meaningful only for the watch that the
   // user explicitly designated "Use for numbers".  The selected-watch UI
   // calls this overload so Sync can never silently operate on another watch.
   if(!isNumbersPeer(peerId)) {
      if(doLog) Log.i(LOG_ID,"ignore Sync for non-Numbers peer="+peerId);
      return;
      }
   sync();
   }

public void sync() {
   if(!usewatch || numbersPeer()==null)
      return;

   if(Natives.shouldsendlabels()) 
      sendlabels();
   {if(doLog) {Log.i(LOG_ID,"sync");};};

   getnums(0); 
   askendnum(0);
   changedback(1);
   sendendnum(1);
   changedback(0);
   getnums(1); 
   if(Natives.sendcuts()) 
      sendshortcuts(Natives.getShortcuts());
      
   }
/** Send the display unit to every active Garmin endpoint. This is deliberately
 * outside the single Numbers-history queue: both the small Kerfstok app and the
 * Libre3/Kerfstok superset use GLUNITS and may display glucose independently. */
public synchronized void sendunits(int unitin) {
   if(!usewatch) return;
   int unit=unitin==1?1:0;
   Object message=Arrays.asList(GLUNITS,unit);
   for(GarminPeer gp:garminPeers.values())
      if(gp.active) sendPeerObject(gp,message,true,false);
   }
private void requestPeerStart(GarminPeer gp) {
   if(gp==null || !gp.active) return;
   boolean wasStopped=gp.watchStopped;
   gp.watchStopped=false;
   gp.applicationStateTime=System.currentTimeMillis();
   gp.lastError=null;
   DirectGarminIQ direct=directFor(gp.id);
   if(direct!=null) direct.resumeApplication();
   else if(wasStopped && mConnectIQ!=null && gp.connected && gp.device!=null) {
      registerPeerApplication(transportContext,gp);
      return;
      }
   sendPeerStart(gp);
   }

/** Record progress only AFTER the returned reading was validated and saved.
 * Independent frontiers allow clinical backfill older than the live readings.
 * Duplicate/older records must still be ACKed, but cannot indefinitely prevent
 * loss recovery. Do not substitute this receipt time for the measurement time
 * used by glucose display, storage, notifications or the signal-loss alarm. */
private synchronized void noteLibre3ReturnProgress(GarminPeer gp,int kind,long sampleSeconds) {
   if(gp==null || !gp.active || sampleSeconds<=0L) return;
   if(kind==GARMIN_LIBRE3_MINUTE) {
      if(sampleSeconds<=gp.lastLibre3MinuteTime) return;
      gp.lastLibre3MinuteTime=sampleSeconds;
      }
   else if(kind==GARMIN_LIBRE3_CLINICAL) {
      if(sampleSeconds<=gp.lastLibre3ClinicalTime) return;
      gp.lastLibre3ClinicalTime=sampleSeconds;
      }
   else return;
   gp.lastLibre3ReturnProgressElapsed=SystemClock.elapsedRealtime();
   }

/** The age alarm can run immediately after an OLD reading was saved. Receiving
 * a progressing backlog is not a failed return connection. Reuse the existing
 * glucose timeout as the idle bound, measured across deep sleep, and leave all
 * stale-glucose notifications/age-alarm scheduling unchanged. */
private boolean libre3ReturnIsProgressing(GarminPeer gp) {
   if(gp.lastLibre3ReturnProgressElapsed<0L) return false;
   final long idle=SystemClock.elapsedRealtime()-gp.lastLibre3ReturnProgressElapsed;
   return idle>=0L && idle<Notify.glucosetimeout;
   }

/** Called by GlucoseAlarms.handlealarm(), using the loss-of-signal alarm
 * already scheduled by dowithglucose()/setagealarm(). Do not add a separate
 * silence deadline or wait for a heartbeat before repairing this connection.
 * This is transport recovery, not a user request to launch Kerfstok. */
public synchronized void reconnectLibre3Watch() {
   if(!Natives.getusegarmin() || transportContext==null || transportRestarting) return;
   for(GarminPeer gp:new ArrayList<>(garminPeers.values())) flushGarminAlarms(gp);
   // Direct Libre3 ownership is exclusive, but use a snapshot because the
   // existing recovery can refresh the peer table while resolving an address.
   for(GarminPeer gp:new ArrayList<>(garminPeers.values())) {
      if(!gp.active || !gp.libre3Direct || gp.watchStopped) continue;
      if(libre3ReturnIsProgressing(gp)) {
         if(doLog) Log.i(LOG_ID,"Loss of signal: keep progressing Libre3 return v59 "+gp.name+
               " idleMs="+(SystemClock.elapsedRealtime()-gp.lastLibre3ReturnProgressElapsed));
         continue;
         }
      if(doLog) Log.i(LOG_ID,"Loss of signal: reconnect Libre3 watch "+gp.name);
      if(doLog && garminSdkContext!=null) Log.i(LOG_ID,garminSdkContext.incomingDiagnostics());
      if(currentTransportMode()==GARMIN_TRANSPORT_DIRECT || directActive(gp)) {
         // Reset only this watch, preserving the user's transport selection.
         // false suppresses the one-shot openApplication/launch request.
         reinitPeerAfterStop(transportContext,gp.id,false);
         }
      else {
         // A connected-looking SDK can still have a dead return channel.
         // Rebootstrap it now rather than wait on another Handler timeout.
         // ConnectIQ is shared; independent Direct links are left untouched.
         gp.launchRequested=false;
         requestGarminSdkHardRestart("loss of signal from Libre3 watch "+gp.name,false);
         }
      }
   }

/** A saved Libre3 return may be the first packet after reopening Kerfstok if
 * its startup START was lost. ACK it before negotiating the new session, or
 * the watch keeps retrying this record with every newer minute behind it. */
private void acknowledgeLibre3Return(GarminPeer gp,List<?> returned) {
   if(gp==null || !gp.active) return;
   Object ack=GarminLibre3.returnAcknowledgement(returned);
   if(ack==null) return;
   boolean wasStopped=gp.watchStopped;
   if(wasStopped) {
      gp.watchStopped=false;
      gp.applicationStateTime=System.currentTimeMillis();
      gp.lastError=null;
      if(doLog) Log.i(LOG_ID,"Libre3 return resumed stopped Garmin session for "+gp.name);
      }

   DirectGarminIQ direct=directFor(gp.id);
   // A deliberate SDK probe temporarily owns the application channel. Keep
   // its Direct pause intact when a Libre3 record arrives instead of START.
   // GCM can send this ACK without granting it ownership of ordinary traffic.
   boolean probeOwns=gp.sdkProbePending && direct!=null &&
         currentTransportMode()==GARMIN_TRANSPORT_AUTOMATIC;
   boolean ackQueued=false;
   if(probeOwns) {
      if(mConnectIQ!=null && sdkready() && gp.device!=null) try {
         mConnectIQ.sendMessage(gp.device,appFor(gp),ack,new IQSendMessageListener() {
            @Override public void onMessageStatus(IQDevice device,IQApp app,IQMessageStatus status) {
               if(status!=IQMessageStatus.SUCCESS && doLog)
                  Log.i(LOG_ID,"Libre3 return ACK during Garmin Connect probe failed for "+gp.name+": "+status);
               }
            });
         gp.lastSend=System.currentTimeMillis();
         ackQueued=true;
         }
      catch(Throwable th) { Log.stack(LOG_ID,"Libre3 return ACK during Garmin Connect probe "+gp.name,th); }
      }
   else {
      // Queue the ACK first. DirectGarminIQ posts both operations on the main
      // looper; resumeAfterWatchReturn() therefore sees the queued ACK and can
      // send it before a pending Reinit. A valid saved [235]/[237] proves that
      // Kerfstok is alive, so reopen a stale Direct send gate even when
      // watchStopped was already false.
      ackQueued=sendPeerObject(gp,ack,false,false);
      if(direct!=null) direct.resumeAfterWatchReturn();
      }
   if(!ackQueued && doLog)
      Log.i(LOG_ID,"Libre3 return ACK not queued for "+gp.name+"; watch retry required");

   // Only START confirms capabilities/Numbers role/glucose readiness. Returning
   // old sensor history must not turn on sensor Bluetooth or Glucose.
   if(wasStopped && !probeOwns) sendPeerStart(gp);
   }

private void peerStopped(GarminPeer gp) {
   if(gp==null) return;
   gp.watchStopped=true;
   gp.applicationSession++;
   gp.applicationStateTime=System.currentTimeMillis();
   gp.glucoseReady=false;
   gp.startPending=false;
   gp.requestedNumberRole=-1;
   gp.confirmedNumberRole=-1;
   gp.sdkRetryPending=false;
   ++gp.sdkRetryGeneration;
   stopGarminLinkProbe(gp.id);
   gp.numberRoleCapable=false;
   gp.startGeneration++;
   gp.glucoseGeneration++;
   gp.applicationBootstrapPending=false;
   gp.applicationBootstrapGeneration++;
   gp.inflightGlucose=null;
   gp.queuedGlucose=null;
   gp.acknowledgedGlucose=null;
   gp.lastError=null;
   DirectGarminIQ direct=directFor(gp.id);
   if(isNumbersPeer(gp.id)) {
      cancelAutomaticApplicationReply();
      // Direct pauses with its outstanding request retained. GCM has no such
      // queue under our control, so put that request back for the next START.
      if(direct==null && numberPendingMessage!=null) {
         mqueue.addFirst(numberPendingMessage);
         numberPendingMessage=null;
         setSending(false);
         }
      }
   if(direct!=null) direct.pauseApplication();
   // STOP/reconnect state must not alter the user's per-watch Glucose setting.
   recomputeSendtowatch();
   }

public synchronized boolean setGarminGlucose(Context context,long peerId,boolean enabled) {
   if(context==null || peerId==NO_PEER) return false;
   GarminPeer gp=garminPeers.get(peerId);
   if(gp==null) return false;
   gp.glucoseEnabled=enabled;
   persistGarminGlucose(context,gp);
   recomputeSendtowatch();
   wasglucose=false;
   Applic.unit=Natives.getunit();
   if(enabled) {
      if(gp.active) {
         requestPeerStart(gp);
         if(glucosemess!=null) {
            gp.queuedGlucose=glucosemess;
            flushPeerGlucose(gp);
            }
         if(isNumbersPeer(peerId) && gp.glucoseReady) sync();
         }
      }
   else {
      // Do not send any further glucose for this watch. A packet already handed
      // to the transport cannot be recalled, but its acknowledgement is ignored.
      gp.queuedGlucose=null;
      gp.inflightGlucose=null;
      ++gp.glucoseGeneration;
      }
   if(doLog) Log.i(LOG_ID,"Garmin peer "+gp.name+" glucose="+enabled);
   return true;
   }

public synchronized boolean isGarminGlucose(long peerId) {
   GarminPeer gp=garminPeers.get(peerId);
   return gp!=null && gp.glucoseEnabled;
   }

/** Legacy/global caller: enable glucose for every known Garmin watch. */
public synchronized boolean startglucose() {
   boolean changed=false;
   Context context=transportContext;
   for(GarminPeer gp:garminPeers.values()) {
      if(!gp.glucoseEnabled) changed=true;
      gp.glucoseEnabled=true;
      if(context!=null) persistGarminGlucose(context,gp);
      if(gp.active) requestPeerStart(gp);
      }
   recomputeSendtowatch();
   wasglucose=false;
   Applic.unit=Natives.getunit();
   if(glucosemess!=null) broadcastGlucose(glucosemess);
   return changed;
   }

public void startall() {
   for(GarminPeer gp:garminPeers.values()) if(gp.active) requestPeerStart(gp);
   }
boolean wasglucose=false;
/** Legacy/global caller: disable glucose for every known Garmin watch. */
public synchronized void stopglucose() {
   Context context=transportContext;
   for(GarminPeer gp:garminPeers.values()) {
      gp.glucoseEnabled=false;
      gp.queuedGlucose=null; gp.inflightGlucose=null; ++gp.glucoseGeneration;
      if(context!=null) persistGarminGlucose(context,gp);
      }
   wasglucose=false;
   sendtowatch=false;
}
public void pushglucose() {
   if(sendtowatch)  {
      wasglucose=true;
      sendtowatch=false;
      }
}
public void backglucose() {
   sendtowatch=wasglucose;
//   setSending(false);
   }
private final static Application getApplication() {
   return Applic.app;
    }

static private void senderror(int type) {
   errorm("Senderror "+type);
   }

static private void errorm(String mess) {
    Log.e(LOG_ID,mess);
    Applic.Toaster(mess);
    }
static final int sendchunk = 50;
private int[] sendgoal = {0,0};
public long receivedmessage=0L;
class IQlisten implements IQApplicationEventListener {
@Override
public void onMessageReceived(IQDevice device, IQApp app, List<Object> message, IQMessageStatus status) {
   long peerId=device==null?NO_PEER:device.getDeviceIdentifier();
   onPeerMessage(peerId,device,app,message,status);
   }

void onPeerMessage(long peerId,IQDevice device,IQApp sourceApp,List<Object> message,IQMessageStatus status) {
   synchronized(AllData.this) {
   if(!Natives.getusegarmin() || status!=IQMessageStatus.SUCCESS || message==null || message.isEmpty()) return;
   GarminPeer gp=garminPeers.get(peerId);
   if(gp==null && device!=null) {
      gp=new GarminPeer(peerId,peerName(device));
      gp.device=device;
      garminPeers.put(peerId,gp);
      }
   if(sourceApp!=null && supportedGarminAppId(sourceApp.getApplicationId())) {
      recordLibre3Installation(gp,sourceApp.getApplicationId(),true);
      String incoming=normalizeAppId(sourceApp.getApplicationId());
      if(!incoming.equals(appIdFor(gp))) {
         boolean starts=false, libre3return=false;
         for(Object candidate:message) if(candidate instanceof List<?> && !((List<?>)candidate).isEmpty() &&
               ((List<?>)candidate).get(0) instanceof Integer) {
            int candidateKind=(Integer)((List<?>)candidate).get(0);
            if(candidateKind==START) starts=true;
            if((candidateKind==GARMIN_LIBRE3_MINUTE || candidateKind==GARMIN_LIBRE3_CLINICAL || candidateKind==GarminLibre3Lifecycle.ACK) && incoming.equals(LIBRE3_GARMIN_APP_ID)) libre3return=true;
            }
         if(!starts && !libre3return) {
            if(doLog) Log.i(LOG_ID,"ignore message from non-selected Garmin app "+incoming+" peer="+peerId);
            return;
            }
         if(starts) {
            gp.appId=incoming;
            gp.appFallbackTried=false;
            if(transportContext!=null) persistGarminApp(transportContext,gp);
            if(doLog) Log.i(LOG_ID,"Garmin START selected app "+incoming+" for "+gp.name);
            }
         }
      }
   // Once Automatic has committed this peer to Direct BLE, late GCM replies
   // must not complete or mutate the Direct transaction.
   if(device!=null && !acceptsGarminConnectPeer(peerId)) {
      if(doLog) Log.i(LOG_ID,"ignore late Garmin Connect message from "+(gp==null?peerId:gp.name)+": "+message);
      return;
      }
   if(device!=null && gp!=null) {
      gp.sdkNoReplyCount=0;
      gp.setupRecoveryUsed=false;
      // A successfully delivered Connect IQ application message is definitive
      // evidence that Garmin Connect currently has a usable connection to this
      // watch.  This also protects Automatic against a stale NOT_CONNECTED
      // snapshot returned by a later getKnownDevices() refresh.
      gp.connected=true;
      try { device.setStatus(IQDeviceStatus.CONNECTED); } catch(Throwable ignored) {}
      ++gp.connectGeneration;
      }

   boolean advanceNumbers=false;
   for(Object o:message) {
      if(!(o instanceof List<?>)) {
         Log.e(LOG_ID,"onMessageReceived no List: "+(o==null?"null":o.getClass().getName()));
         continue;
         }
      List<?> li=(List<?>)o;
      if(li.isEmpty() || !(li.get(0) instanceof Integer)) continue;
      int kind=(Integer)li.get(0);
      if(gp!=null && gp.watchStopped && kind!=START && kind!=STOP && kind!=GARMIN_LIBRE3_MINUTE && kind!=GARMIN_LIBRE3_CLINICAL && kind!=GarminLibre3Lifecycle.ACK) {
         if(doLog) Log.i(LOG_ID,"ignore late message after Kerfstok STOP["+gp.name+"]: "+li);
         continue;
         }
      receivedmessage=System.currentTimeMillis();
      if(gp!=null) gp.lastReceived=receivedmessage;
      log("onMessageReceived["+(gp==null?peerId:gp.name)+"] "+kind);

      // Libre3 returns are real application traffic too. Keep this before
      // their continue statements so they cancel pending transport recovery.
      garminLinkTrafficResumed(gp);

      if(kind==GarminAlarmConfig.ACK) {
         if(gp!=null && isLibre3App(gp)) receiveGarminAlarmAck(gp,li);
         continue;
         }
      if(kind==GarminLibre3Lifecycle.ACK) {
         boolean fromLibre3App=sourceApp!=null ?
               normalizeAppId(sourceApp.getApplicationId()).equals(LIBRE3_GARMIN_APP_ID) :
               (gp!=null && isLibre3App(gp));
         if(fromLibre3App) GarminLibre3Lifecycle.receive(peerId,li);
         continue;
         }

      if(kind==GARMIN_LIBRE3_MINUTE || kind==GARMIN_LIBRE3_CLINICAL) {
         // Accept delayed backlog even after Direct was switched off: these are
         // measurements the watch owned while Juggluco sensor Bluetooth was off.
         boolean fromLibre3App=sourceApp!=null ?
               normalizeAppId(sourceApp.getApplicationId()).equals(LIBRE3_GARMIN_APP_ID) :
               (gp!=null && isLibre3App(gp));
         if(fromLibre3App && GarminLibre3.receiveMinuteFromGarmin(peerId,li)) {
            Object timestamp=li.size()>1?li.get(1):null;
            if(gp!=null && timestamp instanceof Number) {
               noteLibre3ReturnProgress(gp,kind,((Number)timestamp).longValue());
               acknowledgeLibre3Return(gp,li);
               flushGarminAlarms(gp);
               }
            }
         continue;
         }

      boolean numberReply=isNumbersPeer(peerId) && numberPendingMessage!=null &&
            directReplyMatches(numberPendingMessage,li);
      if(numberReply) {
         peerAcknowledged(gp);
         numberPendingMessage=null;
         noteAutomaticApplicationReply(peerId,li);
         advanceNumbers=true;
         }

      if(kind==GOTGLUCOSE) {
         gotgotglucose=true;
         peerGotGlucose(gp,li);
         flushGarminAlarms(gp);
         continue;
         }
      if(kind==START) {
         if(gp!=null) {
            if(gp.reinitRecoveryPending) {
               gp.reinitRecoveryPending=false;
               ++gp.reinitRecoveryGeneration;
               if(doLog) Log.i(LOG_ID,"Reinit confirmed by Kerfstok START for "+gp.name);
            }
            boolean roleReply=gp.startPending && gp.requestedNumberRole>=0;
            gp.watchStopped=false;
            if(isLibre3App(gp)) {
               GarminLibre3.sendSensorInfo(gp.id);
               GarminLibre3Lifecycle.onWatchStart(gp.id,li);
               }
            gp.numberRoleCapable=hasStartCapability(li,NUMBER_ROLE_CAPABILITY);
            gp.timestampedGlucoseAck=hasStartCapability(li,GLUCOSE_ACK_TIMESTAMP);
            gp.alarmCapable=isLibre3App(gp) && hasStartCapability(li,GarminAlarmConfig.CAPABILITY);
            gp.alarmSoundConfigCapable=gp.alarmCapable && hasStartCapability(li,GarminAlarmConfig.SOUND_CONFIG_CAPABILITY);
            gp.alarmToneCapable=gp.alarmSoundConfigCapable && hasStartCapability(li,GarminAlarmConfig.TONE_CAPABILITY);
            gp.alarmOutputConfigCapable=gp.alarmSoundConfigCapable && hasStartCapability(li,GarminAlarmConfig.OUTPUT_CONFIG_CAPABILITY);
            gp.alarmVibrationCapable=gp.alarmOutputConfigCapable && hasStartCapability(li,GarminAlarmConfig.VIBRATION_CAPABILITY);
            if(gp.alarmCapable) {
               gp.alarmAcknowledgedRevision=0;
               gp.alarmLastAttempt=0L;
               flushGarminAlarms(gp);
               }
            GarminAlarms.statusChanged(AllData.this,gp.id);
            gp.confirmedNumberRole=gp.numberRoleCapable?startNumberRole(li):-1;
            DirectGarminIQ direct=directFor(peerId);
            if(direct!=null) direct.resumeApplication();
            if(gp.numberRoleCapable && startNumberRole(li)!=(isNumbersPeer(peerId)?1:0)) {
               // Do not treat a delayed pre-change START as the role ACK or
               // queue duplicate role commands while waiting for that ACK.
               sendPeerRole(gp);
               continue;
               }
            if(gp.startPending || gp.inflightGlucose!=null) peerAcknowledged(gp);
            gp.startPending=false;
            gp.requestedNumberRole=-1;
            gp.applicationStateTime=receivedmessage;
            ++gp.startGeneration;
            gp.glucoseReady=true;
            receiveKerfstokTheme(gp,li);
            // Kerfstok 53 can reply START instead of [229] to the first
            // glucose after reopening. Re-send once in the new session so it
            // produces a normal glucose acknowledgement, not an endless wait.
            if(!roleReply && gp.inflightGlucose!=null) {
               if(gp.queuedGlucose==null) gp.queuedGlucose=gp.inflightGlucose;
               gp.inflightGlucose=null;
               ++gp.glucoseGeneration;
               }
            if(!roleReply && gp.glucoseEnabled && gp.queuedGlucose==null && gp.inflightGlucose==null && glucosemess!=null)
               gp.queuedGlucose=glucosemess;
            flushPeerGlucose(gp);
            }
         // A Direct -> Garmin Connect handoff can occur with one Numbers request
         // still outstanding. START establishes the new SDK application session;
         // replay that exact request now so it cannot block glucose forever.
         if(gp!=null && gp.retryNumbersAfterStart) {
            gp.retryNumbersAfterStart=false;
            if(device!=null && isNumbersPeer(peerId) && numberPendingMessage!=null) {
               if(doLog) Log.i(LOG_ID,"retry retained Numbers transaction through Garmin Connect after START: "+numberPendingMessage);
               sendPeerObject(gp,numberPendingMessage,messageNeedsReply(numberPendingMessage),true);
               continue;
               }
            }
         if(!isNumbersPeer(peerId)) {
            // START is a session event, not a user preference. Reconnecting a
            // watch must never switch its Glucose checkbox on.
            if(gp!=null && gp.glucoseEnabled) flushPeerGlucose(gp);
            continue;
            }
         // Re-enabling Numbers must synchronize even if glucose was already
         // running. Wait for the matching role confirmation before doing so.
         boolean initializeFromJuggluco=numbersAuthorityPushPending;
         boolean resumeNumbersSync=numbersSyncPending && !initializeFromJuggluco;
         numbersAuthorityPushPending=false;
         numbersSyncPending=false;
         // A reconnect/start response must not release an outstanding history
         // request. The Direct queue can be retaining that request for retry.
         if(numberPendingMessage==null) setSending(false);
         // On a newly selected watch the authoritative push supersedes the
         // legacy "lowestchange is null" bootstrap. givebase0nums() ends in the
         // ordinary bidirectional sync and would immediately read the new watch.
         if(initializeFromJuggluco) {
            initializeNumbersWatchFromJuggluco();
            continue;
            }
         if(li.size()==1 || !Boolean.TRUE.equals(li.get(1))) {
            // Glucose enablement is per-watch and is not changed by START.
            }
         else {
            givebase0nums();
            return;
            }
         nextmessage();
         if(resumeNumbersSync) sync();
         continue;
         }
      if(kind==STOP) {
         peerStopped(gp);
         continue;
         }
      if(kind==GOTSTOPALARM || kind==COLORBLACK) continue;
      if(kind==STOPALARM) { Notify.stopalarmnotsend(false); continue; }
      if(kind==STRING && li.size()>1) { infostr=String.valueOf(li.get(1)); continue; }
      if(kind==SENDERROR && li.size()>1 && li.get(1) instanceof Integer) { senderror((Integer)li.get(1)); continue; }

      if(!isNumbersPeer(peerId)) {
         if(doLog) Log.i(LOG_ID,"ignore number/config message from glucose-only peer "+(gp==null?peerId:gp.name)+": "+li);
         continue;
         }

      // While a newly selected watch is being seeded, Juggluco is deliberately
      // the sole authority.  Ignore delayed/unsolicited watch history so it can
      // never overwrite the local database during the one-way initialization.
      // Acknowledgements for our PUTNUMS/SETENDNUM/config messages are handled
      // below and are not in this set.
      if(numbersAuthorityPushActive &&
            (kind==NUMS || kind==HAVENUMS || kind==SETENDNUM || kind==DELETE || kind==NOMORENUMS)) {
         if(doLog) Log.i(LOG_ID,"ignore watch history during Juggluco-authoritative initialization: "+li);
         continue;
         }

      // Unsolicited watch edits remain valid, but a stale acknowledgement must
      // neither change history indices nor complete a different transaction.
      if(!numberReply && !directWatchInitiatedMessage(li)) continue;
      switch(li.size()) {
         case 1:
         case 2:
            switch(kind) {
               case GOTSTOPALARM:
               case COLORBLACK:
               case DIDSETENDNUM:
                  break;
               case RECEIVEDPRECISION:
                  Natives.setshouldsendlabels(false);
                  break;
               case RECEIVEDCUTS:
                  Natives.setsendcuts(false);
                  break;
               }
            break;

         case 3: {
            int base=(Integer)li.get(1);
            int num=(Integer)li.get(2);
            switch(kind) {
               case SETENDNUM:
                  numio.setlastnum(base,num);
                  break;
               case DELETE:
                  log("DELETE "+base+" "+num);
                  numio.delete(base,num);
                  Applic.app.redraw();
                  realsendmessage(Arrays.asList(DELETED,base,num));
                  return;
               case NOMORENUMS:
                  log("NOMORENUMS "+base+" "+num);
                  numio.updated(base,num);
                  Applic.app.redraw();
                  break;
               }
            } break;

         case 4: {
            int base=(Integer)li.get(1);
            int begin=(Integer)li.get(2),end=(Integer)li.get(3);
            switch(kind) {
               case DELETED:
                  log("DELETED "+base+" "+begin+"-"+end);
                  break;
               case GOTNUMS:
                  if(!backchanged[base]) Natives.setchangedNumLater(numio.numptrs[base],end);
                  final int sgoal=sendgoal[base];
                  log("GOTNUMS "+base+" "+begin+"-"+end+" sendgoal="+sgoal);
                  if(sgoal>end) {
                     putnums(base,end,sgoal);
                     return;
                     }
                  sendgoal[base]=0;
                  if(base==0 && Natives.shouldsendlabels()) {
                     setSending(false);
                     sendlabels();
                     return;
                     }
                  break;
               }
            } break;

         case 5: {
            int base=(Integer)li.get(1);
            int size=(Integer)li.get(2);
            int end=(Integer)li.get(3);
            switch(kind) {
               case HAVENUMS:
                  log("HAVENUMS "+base+" "+size+" "+end);
                  if((end-size)>numio.getlastnum(base)) {
                     realgetnums(base);
                     return;
                     }
                  // fall through: HAVENUMS and NUMS carry the same data shape.
               case NUMS:
                  log("NUMS "+base+" "+size+" "+end);
                  List<Object> gegs=(List<Object>)li.get(4);
                  if(size!=gegs.size()) {
                     errorm("input said "+size+" got "+gegs.size());
                     break;
                     }
                  int datiter=end-gegs.size(),start=datiter;
                  ListIterator<List<Number>> iter=((List<List<Number>>)li.get(4)).listIterator();
                  while(iter.hasNext()) numio.writeAr(base,datiter++,iter.next());
                  numio.updatedstartend(base,start,datiter);
                  realsendmessage(Arrays.asList(MORENUMS,base,datiter));
                  return;
               }
            } break;

         default:
            errorm("Don't know messages of size="+li.size());
            break;
         }
      }
   if(advanceNumbers) nextmessage();
   }
   }
}
static final private void log(String str) {
   {if(doLog) {Log.i(LOG_ID,str);};};
   }
   /*
public void allback(int base) {
   if(!usewatch)
      return;   


      
   long ptr=numio.numptrs[base];
   int last =  Natives.getlastNum(ptr);
   int first =  Natives.getfirstNum(ptr);
        sendmessage(putnummer(base,first,last));
   }
*/
private boolean[] backchanged=new boolean[2];
private boolean realchangedback(int base) {
   backchanged[base]=false;
   Object mess=getchangedback(base);
   if(mess!=null) {
      realsendmessage(mess);
      return true;
      }
   return false;
   }
public void changedback(int base) {
   if(!usewatch)
      return;

    if(!isSending()) {
      {if(doLog) {Log.i(LOG_ID,"changedback "+base);};};
      setSending(true);
      if(!realchangedback(base)) {
         nextmessage();
         }
      }
   else {
      {if(doLog) {Log.i(LOG_ID,"backchanged "+base+" = true");};};
      backchanged[base]=true;
       } 
}
private Object  getchangedback(int base) {
   long ptr=numio.numptrs[base];
   int last =  Natives.getlastNum(ptr);
   int changed=  Natives.getchangedNum(ptr);
   int one=  Natives.getonechangeNum(ptr);
log(base+" changedback: last="+last+" changed="+changed+" one="+one);
   if(changed<last) {
      if(one>0&&one<changed)
         return putnummer(base,one,last);
      else
         return putnummer(base,changed,last);
      }
   else {
      if(one>0&&one<last)
         return putnummer(base,one,one+1);
      }
   return null;
   }

private  Object putnummer(int base,int beg, int end) {
    log("putnummer("+base+","+beg+","+end+")");
    int len = end - beg;
    if (len > maxstorage) {
       beg = end - maxstorage;
       }
    return     getputnums(base,beg, end);
    }

private void putnums(int base,int beg, int end) {
   Object mess=getputnums(base,beg,end);
   if(mess!=null)
      realsendmessage(mess);
   else
      nextmessage();
   }
private Object getputnums(int base,int begin, int end) {
   int last = numio.getlastnum(base);
   {if(doLog) {Log.i(LOG_ID,"getputnums base="+base+" begin="+begin+" end="+end+" last="+last);};};
   if(last < end) {
       end = last;
       if(begin>=end)
          return null;
       }
    int len = end - begin;
    if(len > sendchunk) {
       sendgoal[base] = end;
       end = begin + sendchunk;
       {if(doLog) {Log.i(LOG_ID,"getputnums len="+len+" > "+sendchunk+" sendgoal="+sendgoal[base]);};};
       }
    List<List<Number>> ar = new ArrayList<>();
    try  {
       for(int pos = begin; pos < end; pos++) {
           var el=   numio.readAr(base,pos);
           ar.add(el);
           }
    } catch (Exception e) {
       Log.stack(LOG_ID,"getsendnums ",e);
       return null;
        }
    return Arrays.asList(PUTNUMS, base,begin,end,ar);
    }
private final int[][] dodelete ={{-1,-1},{-1,-1}};


private void senddeleteone(int base) {
   realsendmessage(Arrays.asList(DELETE,base,dodelete[base][0],dodelete[base][1]));
   dodelete[base][0]=dodelete[base][1];
   }
private boolean deletelater(int base) {
   int last=numio.getlastnum(base);
   if(last>dodelete[base][0])
      dodelete[base][0]=last;
   if(dodelete[base][0]<dodelete[base][1]) {
      senddeleteone(base);
      return true;
      }
   return false;
   }
public void deletelast(int base,int pos,int end ) {
 if(!usewatch)
      return;
  {if(doLog) {Log.i(LOG_ID,"delete "+base+" "+pos+"-"+end);};};
  dodelete[base][0]=pos;
  if(end>dodelete[base][1]) 
       dodelete[base][1]=end;
  if(!isSending()) {
       setSending(true);
       senddeleteone(base);
       }
}
/*
void addmessages(final Object[] upin) {
   if(upiter>0) {
      final Object[] tmp =ups;
      final int total=upin.length+upiter;
       ups= new Object[total];
        System.arraycopy(ups, 0, upin, 0, upin.length);
        System.arraycopy(ups, upin.length, tmp, 0, upiter);
      }
   else {
      ups=upin;
      }
   upiter=ups.length;
   }

    void nextmessage() {
        if (upiter > 0)
            sendmessage(ups[--upiter]);
    }
*/
private final LinkedList<Object> mqueue= new LinkedList<Object>();

private synchronized boolean sendmessage(Object obj) {
    if(numbersPeer()==null) return false;
    if(!isSending()) {
   setSending(true);
        realsendmessage(obj);
   return true;
       }
    else {
        log("sendmessage add to queue");
        mqueue.add(obj);
        sendmessages();
   return false;
    }
   }
   
public void sendmessages() {
      if(!usewatch)
         return;
   if(!isSending()) {
           nextmessage() ;
      }
   }
/*
void addmessages(final Object[] upin) {
   Collections.addAll(mqueue, upin);
   sendmessages();   
   }
   */
private boolean todelete(int base) {
   return dodelete[base][0]<dodelete[base][1] ;
   }
public boolean waiting() {
   if(numbersPeer()==null) return false;
   // glucosemess is only the latest-value cache for reconnecting peers; it is
   // not queued work.  Including it made "Send queue" permanently visible.
   return(!mqueue.isEmpty()||todelete(0)||todelete(1)||backchanged[0]||backchanged[1]||askednums[0]||askednums[1]);
   }

/** The Numbers/history queue belongs only to the selected Numbers watch. */
public boolean waiting(long peerId) {
   return isNumbersPeer(peerId) && waiting();
   }
boolean sendoldglucose() {
   // Glucose fan-out is independent from the single numbers transaction queue.
   return false;
   }
public void nextmessage(long peerId) {
   if(!isNumbersPeer(peerId)) {
      if(doLog) Log.i(LOG_ID,"ignore Send queue for non-Numbers peer="+peerId);
      return;
      }
   nextmessage();
   }

public synchronized void nextmessage() {
   {if(doLog) {Log.i(LOG_ID,"nextmessage");};};
   if(transportRestarting) { setSending(false); return; }
   GarminPeer gp=numbersPeer();
   if(gp==null) { setSending(false); return; }
   if(!gp.glucoseReady) return;
   if(gp.numberRoleCapable && gp.confirmedNumberRole!=1) {
      flushPeerGlucose(gp);
      return;
      }
   if(numberPendingMessage!=null || (gp!=null && gp.inflightGlucose!=null)) return;
   setSending(true);
   // Interleave the latest glucose only at a completed history transaction
   // boundary. Neither path can reset or bypass the other's outstanding ACK.
   flushPeerGlucose(gp);
   if(gp!=null && gp.inflightGlucose!=null) return;
   if(sendoldglucose())
      return;
   Object obj=mqueue.poll();
   if(obj!=null) {
      {if(doLog) {Log.i(LOG_ID,"from queue");};};
      realsendmessage(obj);
      return;
      }

   else  {
      
      if(deletelater(0))
         return;
      if(deletelater(1))
         return;
      if(backchanged[1]&& realchangedback(1))
         return;

      if(askednums[0]) { 
         realgetnums(0);
         return;
         }
      if(asksendends[0])  {
         realaskendnum(0);
         return;
         }
      if(sendends[1]) {
         realsendendnum(1);
         return;
         }

      if(backchanged[0]&& realchangedback(0))
         return;
      if(askednums[1]) { 
         realgetnums(1);
         return;
         }
      if(sendends[0]) {
         realsendendnum(0); 
         return;
         }
      if(asksendends[1]) {
         realaskendnum(1); //Never happens
         return;
         }
//      nexttime=0L;
      setSending(false);
      if(numbersAuthorityPushActive) {
         numbersAuthorityPushActive=false;
         {if(doLog) {Log.i(LOG_ID,"Numbers watch initialization from Juggluco complete");};};
         }
      }
    }


   /*
public void sync() {
   if(!usewatch)
      return;

   if(Natives.shouldsendlabels()) 
      sendlabels();
   {if(doLog) {Log.i(LOG_ID,"sync");};};

   getnums(0); 
   askendnum(0);
   changedback(1);

   sendendnum(1);

   changedback(0);
   getnums(1); 
   if(Natives.sendcuts()) 
      sendshortcuts(Natives.getShortcuts());
      
   } */


public IQMessageStatus sendstatus=null;
static private final long waittime=1000L;

public long statustime=0L,sendtime=0L;
private volatile long nexttime=0L;
public synchronized boolean realsendmessage(Object message) {
   GarminPeer gp=numbersPeer();
   if(gp==null) {
      {if(doLog) {Log.d(LOG_ID,"realsendmessage: configured numbers device is unavailable");};};
      setSending(false);
      return false;
      }
   setSending(true);
   if(!gp.glucoseReady || (gp.numberRoleCapable && gp.confirmedNumberRole!=1) ||
         numberPendingMessage!=null || gp.inflightGlucose!=null) {
      mqueue.add(message);
      return true;
      }
   numberPendingMessage=message;
   {if(doLog) {Log.i(LOG_ID,(directActive(gp)?"direct ":"")+"realsendmessage["+gp.name+"] "+message);};};
   sendtime=System.currentTimeMillis();
   boolean ok=sendPeerObject(gp,message,messageNeedsReply(message),true);
   if(!ok && currentTransportMode()==GARMIN_TRANSPORT_AUTOMATIC && transportContext!=null) {
      synchronized(this) { automaticRetryMessage=message; automaticRetryPeerId=gp.id; }
      startDirectPeer(transportContext,gp);
      return true;
      }
   if(!ok && numberPendingMessage==message) {
      numberPendingMessage=null;
      mqueue.add(message);
      setSending(false);
      }
   return ok;
   }
public void sendshortcuts(ArrayList<ArrayList<Object>> shortcuts) {
   if(!usewatch)
      return;

    sendmessage(Arrays.asList(SHORTCUTS,shortcuts ));
   }
public void sendlabels() {
   if(!usewatch)
      return;
   List<String> labs= Applic.app.getlabels();
   int len=   labs.size()-1;
   if(len>0)
       sendmessage(Arrays.asList(PUTLABELS, labs.subList(0,len)));
   ArrayList<Float> precs   =new ArrayList<> ();
   precs.ensureCapacity(len);
   for(int i=0;i<len;i++)
      precs.add(Natives.getPrecision(i));
    sendmessage(Arrays.asList(PUTPRECISION, precs));
    }
/*
int getlastnum(int base) {
   long ptr=numio.numptrs[base];
   return Natives.getlastNum(ptr); 
   }
int getlastpollednum(int base) {
   long ptr=numio.numptrs[base];
   return Natives.getlastpolledNum(ptr); 
   }
   */
void setSending(boolean val) {
   if(val)
      nexttime=Long.MAX_VALUE;
   else
       nexttime=0L;
   }
public boolean isSending() {   
      return System.currentTimeMillis()<nexttime;
    }
boolean[] askednums=new boolean[2];
private void getnums(int base) {
   if(!usewatch)
      return;
     if(didreceivebackup(base))
      return;
   if(isSending())  {
      {if(doLog) {Log.i(LOG_ID,"asknums "+base);};};
      askednums[base]=true;
      }
   else {
      setSending(true);
      realgetnums(base);
      }
    }

void realgetnums(int base) {
   {if(doLog) {Log.i(LOG_ID,"realgetnums "+base);};};
   askednums[base]=false;
   realsendmessage(Arrays.asList(NUMS, base, numio.getlastpollednum(base)));
   }
//void getnums() { addmessages(new Object[] { Arrays.asList(NUMS, 1, getlastnum(1)),Arrays.asList(NUMS, 0, getlastnum(0)) }); }


/*
    public enum IQDeviceStatus {
        NOT_PAIRED,
        NOT_CONNECTED,
        CONNECTED,
        UNKNOWN
    }

*/
public void stopalarm() {
   if(!Natives.getusegarmin()) return;
   {if(doLog) {Log.i(LOG_ID,"send stopalarm to all Garmin peers");};};
   Object message=Arrays.asList(STOPALARM);
   for(GarminPeer gp:garminPeers.values()) if(gp.active) sendPeerObject(gp,message,true,false);
   }
void      testapppresent() {
   for(GarminPeer gp:garminPeers.values()) if(gp.active) sendPeerStart(gp);
   }
    private IQDeviceEventListener mDeviceEventListener = new IQDeviceEventListener() {
       @Override
       public void onDeviceStatusChanged(IQDevice device, IQDeviceStatus status) {
          if(!Natives.getusegarmin()) return;
          {if(doLog) {Log.i(LOG_ID,"onDeviceStatusChanged("+device.toString()+","+status.toString()+")");};};
          GarminPeer gp=peer(device);
          if(gp==null && transportContext!=null) {
             rebuildGarminPeers(transportContext);
             gp=peer(device);
             }
          if(gp==null || !gp.active) return;
          // After Garmin Connect is force-stopped its old SDK listener can receive
          // a fresh CONNECTED status when the service is started again, without a
          // second onSdkReady().  Use that as a trigger to rebuild the SDK first.
          if(status==IQDeviceStatus.CONNECTED && currentTransportMode()!=GARMIN_TRANSPORT_DIRECT && !sdkready()) {
             gp.connected=true;
             try { device.setStatus(status); } catch(Throwable ignored) {}
             gp.connectGeneration++;
             requestGarminSdkRestartAfterShutdown();
             return;
             }
          // Keep Garmin Connect's physical status current even while Direct owns
          // application traffic. CONNECTED alone is not sufficient to switch
          // automatically (the rare Garmin failure reports CONNECTED too), but
          // it lets an explicit Reinit give Garmin Connect the first chance.
          if(currentTransportMode()==GARMIN_TRANSPORT_AUTOMATIC && directActive(gp)) {
             gp.connected=(status==IQDeviceStatus.CONNECTED);
             try { device.setStatus(status); } catch(Throwable ignored) {}
             gp.connectGeneration++;
             if(doLog) Log.i(LOG_ID,"Garmin Connect status while Direct owns "+gp.name+": "+status);
             if(gp.connected && sdkready()) requestGarminConnectProbe(gp,false);
             return;
             }
          if(!acceptsGarminConnectPeer(gp.id)) return;
          boolean wasConnected=gp.connected;
          gp.connected=(status==IQDeviceStatus.CONNECTED);
          try { device.setStatus(status); } catch(Throwable ignored) {}
          gp.connectGeneration++;
          if(gp.connected) {
             usewatch=true;
             if(!wasConnected || (!gp.glucoseReady && !gp.startPending))
                registerPeerApplication(transportContext,gp);
             }
          else {
             gp.sdkRetryPending=false;
             ++gp.sdkRetryGeneration;
             stopGarminLinkProbe(gp.id);
             gp.glucoseReady=false;
             gp.glucoseGeneration++;
             gp.startGeneration++;
             gp.applicationBootstrapPending=false;
             gp.applicationBootstrapGeneration++;
             if(gp.queuedGlucose==null) gp.queuedGlucose=gp.inflightGlucose;
             gp.inflightGlucose=null;
             if(currentTransportMode()==GARMIN_TRANSPORT_AUTOMATIC && transportContext!=null && !directActive(gp)) {
                final GarminPeer target=gp;
                final int token=gp.connectGeneration;
                transportHandler.postDelayed(() -> {
                   if(currentTransportMode()==GARMIN_TRANSPORT_AUTOMATIC && token==target.connectGeneration &&
                         !target.connected && !directActive(target))
                      startDirectPeer(transportContext,target);
                   },AUTOMATIC_DIRECT_DELAY_MS);
                }
             }
       }
    };

/** Re-read Garmin Connect's device list and Android's bonded Garmin watches.
 *  Pairing is done outside Juggluco, so Garmin Status calls this after the user
 *  has paired a watch in Android Bluetooth settings.  This never launches
 *  Kerfstok; it only discovers/registers the newly visible peer and lets the
 *  configured transport connect to it. */
public synchronized void refreshGarminDevices(Context context) {
   if(context==null) return;
   final Context app=context.getApplicationContext();
   transportContext=app;

   // Refresh is also an explicit opportunity to recover after Garmin Connect was
   // restarted.  A dead SDK otherwise has no getKnownDevices() path to refresh.
   if(currentTransportMode()!=GARMIN_TRANSPORT_DIRECT && mConnectIQ!=null && !sdkready())
      requestGarminSdkRestartAfterShutdown();

   final java.util.HashMap<Long,Boolean> oldSdkPeers=new java.util.HashMap<>();
   for(GarminPeer gp:garminPeers.values())
      if(gp.device!=null) oldSdkPeers.put(gp.id,Boolean.TRUE);

   if(mConnectIQ!=null && sdkready()) {
      try { devices=mConnectIQ.getKnownDevices(); }
      catch(Throwable th) { Log.stack(LOG_ID,"refresh getKnownDevices",th); }
      }

   rebuildGarminPeers(app);

   for(GarminPeer gp:garminPeers.values()) queryLibre3Installation(gp);

   // Register only SDK devices which were not already part of this AllData
   // instance. Re-registering every known watch on each UI refresh can create
   // duplicate callbacks in Garmin Connect.
   if(mConnectIQ!=null && sdkready() && devices!=null) {
      for(IQDevice device:devices) {
         GarminPeer gp=peer(device);
         if(gp==null || !gp.active || oldSdkPeers.containsKey(gp.id)) continue;
         if(doLog) Log.i(LOG_ID,"new Garmin Connect device discovered: "+gp.name+" peer="+gp.id);
         try { mConnectIQ.registerForDeviceEvents(device,mDeviceEventListener); }
         catch(Throwable th) { Log.stack(LOG_ID,"refresh register device "+gp.name,th); }
         if(acceptsGarminConnectIncoming(gp.id)) {
            registerBothAppEvents(mConnectIQ,gp);
            if(gp.connected) registerPeerApplication(app,gp);
            }
         }
      }

   final int mode=currentTransportMode();
   if(mode==GARMIN_TRANSPORT_DIRECT) {
      for(GarminPeer gp:new ArrayList<>(garminPeers.values()))
         if(gp.active && gp.directAddress!=null && !directActive(gp)) startDirectPeer(app,gp);
      }
   else if(mode==GARMIN_TRANSPORT_AUTOMATIC) {
      for(GarminPeer gp:new ArrayList<>(garminPeers.values())) {
         if(!gp.active) continue;
         if(directActive(gp)) {
            // Refresh is discovery, not a transport-health test. Probing every
            // time Garmin Status refreshes creates avoidable Connect-IQ traffic
            // and can itself race a working Direct session. A genuine Garmin
            // CONNECTED transition, real Garmin packet, or explicit Reinit
            // triggers the failback probe instead.
            continue;
            }
         if(gp.connected || gp.directAddress==null) continue;
         final GarminPeer target=gp;
         final int token=++gp.connectGeneration;
         transportHandler.postDelayed(() -> {
            synchronized(AllData.this) {
               GarminPeer current=garminPeers.get(target.id);
               if(currentTransportMode()==GARMIN_TRANSPORT_AUTOMATIC && current==target &&
                     current.active && token==current.connectGeneration && !current.connected &&
                     current.directAddress!=null && !directActive(current))
                  startDirectPeer(app,current);
               }
            },AUTOMATIC_DIRECT_DELAY_MS);
         }
      }

   ensureNumbersPeer();
   recomputeUsewatch();
   recomputeSendtowatch();
   if(doLog) Log.i(LOG_ID,"refreshGarminDevices peers="+garminPeers.size()+
         " bonded="+DirectGarminIQ.bondedGarmins(app).size()+
         " sdk="+(devices==null?0:devices.size()));
   }

public void loadDevices(Context context) {
   {if(doLog) {Log.i(LOG_ID,"loadDevices");};};
   try {
      devices=mConnectIQ.getKnownDevices();
      rebuildGarminPeers(context);
      if(devices!=null) {
         for(IQDevice device:devices) {
            GarminPeer gp=peer(device);
            if(gp==null || !gp.active) continue;
            try { mConnectIQ.registerForDeviceEvents(device,mDeviceEventListener); }
            catch(Throwable th) { Log.stack(LOG_ID,"registerForDeviceEvents "+peerName(device),th); }
            }
         }
      register(context);
      if(currentTransportMode()==GARMIN_TRANSPORT_AUTOMATIC) {
         for(GarminPeer gp:garminPeers.values()) if(gp.active && !gp.connected && gp.directAddress!=null) {
            final GarminPeer target=gp;
            final int token=++gp.connectGeneration;
            transportHandler.postDelayed(() -> {
               if(currentTransportMode()==GARMIN_TRANSPORT_AUTOMATIC && token==target.connectGeneration &&
                     !target.connected && !directActive(target)) startDirectPeer(transportContext,target);
               },AUTOMATIC_DIRECT_DELAY_MS);
            }
         }
      }
   catch(Throwable th) {
      Log.stack(LOG_ID,"loadDevices",th);
      if(currentTransportMode()==GARMIN_TRANSPORT_AUTOMATIC && transportContext!=null)
         startDirectGarmin(transportContext);
      }
   }

public boolean usewatch=false;
public static int appmissing=0;

private boolean bootstrapCurrent(GarminPeer gp,int token,ConnectIQ sdk) {
   return mConnectIQ==sdk && token==gp.applicationBootstrapGeneration &&
         gp.applicationBootstrapPending && acceptsGarminConnectPeer(gp.id);
   }

private synchronized void bootstrapFailed(GarminPeer gp,int token,ConnectIQ sdk,String reason) {
   if(!bootstrapCurrent(gp,token,sdk)) return;
   gp.applicationBootstrapPending=false;
   ++gp.applicationBootstrapGeneration;
   if(doLog) Log.i(LOG_ID,"Kerfstok bootstrap "+gp.name+": "+reason);
   if(currentTransportMode()==GARMIN_TRANSPORT_AUTOMATIC && transportContext!=null) {
      if(gp.connected) sendPeerStart(gp);
      else startDirectPeer(transportContext,gp);
      }
   else if(currentTransportMode()==GARMIN_TRANSPORT_CONNECT)
      sendPeerStart(gp); // A running watch app can answer even if app-info/open did not.
   }

private void registerPeerApplication(Context context,GarminPeer gp) {
   if(gp==null || !gp.active || gp.watchStopped || gp.device==null || mConnectIQ==null || !sdkready() || transportRestarting) return;
   // The bootstrap deadline starts when the SDK reports CONNECTED. Starting
   // while disconnected consumed 11 of its 15 seconds in garmin16.
   if(!gp.connected) return;
   if(!acceptsGarminConnectPeer(gp.id)) return;
   final IQDevice device=gp.device;
   final ConnectIQ sdk=mConnectIQ;
   final int token;
   final boolean launch;
   synchronized(this) {
      if(gp.applicationBootstrapPending) return;
      gp.applicationBootstrapPending=true;
      token=++gp.applicationBootstrapGeneration;
      launch=gp.launchRequested;
      // Reinit grants one launch attempt. Consume it before any asynchronous SDK
      // callback so reconnects cannot display another prompt later.
      gp.launchRequested=false;
      }
   if(!isLibre3App(gp)) queryLibre3Installation(gp);
   try {
      sdk.getApplicationInfo(appIdFor(gp),device,new IQApplicationInfoListener() {
         @Override public void onApplicationInfoReceived(IQApp app) {
            synchronized(AllData.this) {
               if(!bootstrapCurrent(gp,token,sdk)) return;
               if(app!=null) recordLibre3Installation(gp,app.getApplicationId(),true);
               if(!launch) {
                  gp.applicationBootstrapPending=false;
                  ++gp.applicationBootstrapGeneration;
                  }
               }
            appmissing=-1;
            synchronized(AllData.this) {
               gp.appFallbackTried=false;
               if(app!=null && supportedGarminAppId(app.getApplicationId())) {
                  gp.appId=normalizeAppId(app.getApplicationId());
                  if(transportContext!=null) persistGarminApp(transportContext,gp);
                  }
               }
            try {
               try {
                  Object ver=app.getClass().getMethod("getVersion").invoke(app);
                  if(ver instanceof Number) gp.appVersion=((Number)ver).intValue();
                  } catch(Throwable ignored) {}
               if(doLog) Log.i(LOG_ID,"onApplicationInfoReceived "+gp.name+" version="+gp.appVersion+
                     (launch?"; user Reinit requests launch":"; do not launch"));
               if(!launch) {
                  // getApplicationInfo is harmless. Communicate with Kerfstok if it
                  // is already running; only Reinit may call openApplication().
                  sendPeerStart(gp);
                  return;
                  }
               sdk.openApplication(device,app,new IQOpenApplicationListener() {
                  @Override public void onOpenApplicationResponse(IQDevice d,IQApp a,IQOpenApplicationStatus status) {
                     synchronized(AllData.this) {
                        if(d==null || d.getDeviceIdentifier()!=gp.id || !bootstrapCurrent(gp,token,sdk)) return;
                        applicationOpened(gp,status);
                        }
                     }
                  });
               }
            catch(Throwable th) {
               Log.stack(LOG_ID,"openApplication "+gp.name,th);
               bootstrapFailed(gp,token,sdk,"openApplication exception");
               }
            }
         @Override public void onApplicationNotInstalled(String applicationId) {
            synchronized(AllData.this) {
               if(!bootstrapCurrent(gp,token,sdk)) return;
               recordLibre3Installation(gp,applicationId,false);
               gp.applicationBootstrapPending=false;
               ++gp.applicationBootstrapGeneration;
               }
            appmissing=1;
            if(doLog) Log.i(LOG_ID,"onApplicationNotInstalled "+gp.name+" app="+appIdFor(gp));
            boolean retry=false;
            synchronized(AllData.this) {
               if(!gp.appFallbackTried) {
                  gp.appFallbackTried=true;
                  gp.appId=isLibre3App(gp)?normalizeAppId(kerfstokAppId()):LIBRE3_GARMIN_APP_ID;
                  // registerPeerApplication() consumes launchRequested into its local
                  // launch flag. Re-arm it when the first app id was not installed so
                  // an explicit Reinit still opens the compatible fallback app.
                  if(launch) gp.launchRequested=true;
                  if(transportContext!=null) persistGarminApp(transportContext,gp);
                  retry=true;
                  }
               }
            if(retry) registerPeerApplication(context,gp);
            }
         });
      // Clearing a flag alone leaves START unsent indefinitely. Take the same
      // recovery path as an explicit SDK error, once for this bootstrap.
      transportHandler.postDelayed(() -> bootstrapFailed(gp,token,sdk,"app-info/open callback timeout"),
            AUTOMATIC_REPLY_TIMEOUT_MS);
      }
   catch(Throwable th) {
      Log.stack(LOG_ID,"getApplicationInfo "+gp.name,th);
      bootstrapFailed(gp,token,sdk,"getApplicationInfo exception");
      }
   }

void register(Context context) {
   if(devices==null || devices.isEmpty() || mConnectIQ==null) return;
   {if(doLog) {Log.v(LOG_ID,"register: devices="+devices.size());};};
   for(IQDevice device:devices) {
      GarminPeer gp=peer(device);
      if(gp==null || !gp.active) continue;
      if(!acceptsGarminConnectIncoming(gp.id)) continue;
      try {
         registerBothAppEvents(mConnectIQ,gp);
         usewatch=true;
         }
      catch(Throwable th) { Log.stack(LOG_ID,"registerForAppEvents "+gp.name,th); }
      registerPeerApplication(context,gp);
      }
   }

void unregister() {
   usewatch=false;
   final ConnectIQ iq=mConnectIQ;
   final GarminSdkContext sdkContext=garminSdkContext;
   // Make an intentional iq.shutdown() distinguishable from Garmin Connect dying
   // underneath us.  The old listener may receive onSdkShutDown asynchronously.
   mListener=null;
   if(iq==null) {
      if(sdkContext!=null) sdkContext.close();
      garminSdkContext=null;
      return;
      }
   try {
      try {
         iq.unregisterAllForEvents();
         if(doLog) Log.d(LOG_ID,"unregisterAllForEvents");
         }
      catch(Throwable th) { Log.stack(LOG_ID,"unregisterAllForEvents",th); }
      if(devices!=null) {
         for(IQDevice device:devices) {
            GarminPeer gp=peer(device);
            if(gp!=null) unregisterSharedAppEvents(iq,gp);
            }
         }
      }
   finally {
      try { iq.shutdown(sdkContext==null?getApplication():sdkContext); }
      catch(Throwable th) { Log.stack(LOG_ID,"ConnectIQ shutdown",th); }
      if(sdkContext!=null) sdkContext.close();
      if(garminSdkContext==sdkContext) garminSdkContext=null;
      if(mConnectIQ==iq) mConnectIQ=null;
      }
   }

public void   onCleared() {
        try {
      numio.close();
        } catch (Throwable th) {
            Log.stack(LOG_ID,"onCleared",th);
        }
   finally {
      stop();
      }
   }
/*
void StartKerfstok() {
            try {
       if(devices!=null&&devices.size()>0)
      mConnectIQ.openApplication(devices.get(devused), mMyApp, mOpenAppListener);

            } catch (Exception ex) {
            }
//   return mAppIsOpen;
   }

 */
public boolean sdkready() {
   return mListener!=null&& mListener.sdkready();
   }   
MyConnectIQListener mListener=null; 

boolean isCurrentGarminListener(MyConnectIQListener listener) {
   return Natives.getusegarmin() && mListener==listener && mConnectIQ!=null && !transportRestarting &&
         currentTransportMode()!=GARMIN_TRANSPORT_DIRECT;
   }

synchronized boolean acceptsGarminConnectPeer(long peerId) {
   GarminPeer gp=garminPeers.get(peerId);
   return Natives.getusegarmin() && gp!=null && gp.active && !transportRestarting &&
         currentTransportMode()!=GARMIN_TRANSPORT_DIRECT && !directActive(gp);
   }

/** Keep Libre3 record reception available in Automatic, including Direct's
 * reconnect backoff. This grants no ownership of command replies to the SDK. */
private boolean acceptsGarminConnectReturn(GarminPeer gp) {
   return Natives.getusegarmin() && gp!=null && gp.active && !transportRestarting &&
         currentTransportMode()==GARMIN_TRANSPORT_AUTOMATIC &&
         directActive(gp) && isLibre3App(gp);
   }

/** GarminSdkContext uses this before decoding a raw incoming packet. Allow a
 * deliberate Direct->Garmin verification probe and the Libre3 receive route;
 * onGarminConnectMessage() filters records from command replies after decoding. */
synchronized boolean acceptsGarminConnectIncoming(long peerId) {
   GarminPeer gp=garminPeers.get(peerId);
   if(!Natives.getusegarmin() || gp==null || !gp.active || transportRestarting || currentTransportMode()==GARMIN_TRANSPORT_DIRECT) return false;
   if(!directActive(gp)) return true;
   return acceptsGarminConnectReturn(gp) ||
         (currentTransportMode()==GARMIN_TRANSPORT_AUTOMATIC && gp.sdkProbePending);
   }

/** Called by GarminSdkContext before it decodes an incoming Connect IQ payload.
 * In Automatic mode, actual application traffic proves that Garmin Connect is
 * healthy enough to be preferred over Direct BLE. CONNECTED status alone is
 * deliberately not enough because that is also how the persistent Garmin
 * Connect messaging failure presents itself.
 *
 * Returns true when the payload may be decoded by the current Garmin Connect
 * owner/probe or the Libre3 receive route. Other Direct-owned packets can
 * trigger a one-shot Garmin Connect START probe, but Direct is kept as the
 * fallback until the probe receives its matching START reply. */
boolean prepareGarminConnectIncoming(IQDevice device) {
   if(!Natives.getusegarmin() || device==null) return false;
   final long peerId=device.getDeviceIdentifier();
   GarminPeer gp;
   synchronized(this) {
      gp=garminPeers.get(peerId);
      if(gp==null) {
         gp=new GarminPeer(peerId,peerName(device));
         garminPeers.put(peerId,gp);
         if(transportContext!=null) { loadGarminActive(transportContext,gp); loadGarminGlucose(transportContext,gp); loadGarminApp(transportContext,gp); }
         }
      if(!gp.active || transportRestarting || currentTransportMode()==GARMIN_TRANSPORT_DIRECT) return false;
      gp.device=device;
      gp.connected=true;
      try { device.setStatus(IQDeviceStatus.CONNECTED); } catch(Throwable ignored) {}
      if(currentTransportMode()!=GARMIN_TRANSPORT_AUTOMATIC) return !directActive(gp);
      if(!directActive(gp)) return true;
      // Decode Libre3 packets before deciding whether to probe. A saved sensor
      // record needs its ACK immediately; it must not be discarded as the
      // trigger for an ownership transition or as a non-START probe reply.
      if(acceptsGarminConnectReturn(gp)) return true;
      // A packet belonging to an already-running verification probe must be
      // decoded so onGarminConnectMessage() can recognize the START reply.
      if(gp.sdkProbePending) return true;
      // A raw Garmin broadcast proves that Garmin Connect is alive, but not that
      // its SDK object is ready. Keep Direct until initialization really succeeds.
      if(!sdkready()) {
         if(doLog) Log.i(LOG_ID,"Automatic: Garmin Connect Kerfstok traffic seen for "+gp.name+
               " but SDK is not ready; reinitialize SDK and keep Direct BLE");
         requestGarminSdkRestartKeepingDirect();
         return false;
         }
   }
   // Do not hand ownership over on one possibly stale packet. Send a deliberate
   // START through Garmin Connect and require its START reply while Direct keeps
   // carrying normal traffic.
   requestGarminConnectProbe(gp,false);
   return false;
   }

/** Verify Garmin Connect application messaging without first dropping a working
 * Direct connection. The overlap is limited to one START round trip. */
private void requestGarminConnectProbe(GarminPeer gp,boolean reinit) {
   final ConnectIQ sdk;
   final IQDevice device;
   final DirectGarminIQ direct;
   final int token;
   synchronized(this) {
      if(gp==null || !gp.active || gp.watchStopped || transportRestarting ||
            currentTransportMode()!=GARMIN_TRANSPORT_AUTOMATIC || !directActive(gp) ||
            !sdkready() || mConnectIQ==null || gp.device==null || !gp.connected) {
         if(reinit && gp!=null) reinitDirectInPlace(gp.id);
         return;
         }
      if(gp.sdkProbePending) {
         if(reinit) gp.sdkProbeReinit=true;
         return;
         }
      direct=directGarmins.get(gp.id);
      // Do not disturb a live Direct application transaction merely to test
      // whether Garmin Connect has recovered. Wait for a clean idle boundary;
      // otherwise pauseApplication() can close/reset the very Direct session we
      // need to keep as fallback (seen on VA4 after Garmin Connect restart).
      if(direct==null || !direct.isIdleForGarminConnectProbe()) {
         final long id=gp.id;
         transportHandler.postDelayed(() -> {
            GarminPeer again;
            synchronized(AllData.this) { again=garminPeers.get(id); }
            if(again!=null) requestGarminConnectProbe(again,reinit);
            },250L);
         return;
         }
      gp.sdkProbePending=true;
      gp.sdkProbeReinit=reinit;
      gp.sdkProbeWasGlucoseReady=gp.glucoseReady;
      gp.glucoseReady=false;
      if(gp.queuedGlucose==null) gp.queuedGlucose=gp.inflightGlucose;
      gp.inflightGlucose=null;
      ++gp.glucoseGeneration;
      token=++gp.sdkProbeGeneration;
      sdk=mConnectIQ;
      device=gp.device;
      }
   // Temporarily suppress Direct's application replies while Garmin Connect's
   // one-shot START probe is in flight, but do not close/reset any Direct
   // application channel. The probe starts only at an idle boundary above.
   if(direct!=null) direct.pauseForGarminConnectProbe();
   if(doLog) Log.i(LOG_ID,"Automatic: verify Garmin Connect messaging for "+gp.name+
         " while Direct BLE remains active"+(reinit?" (Reinit)":""));
   // DirectGarminIQ and transportHandler both use the main looper. Posting the
   // send lets pauseApplication() run first, so Direct cannot race Garmin Connect
   // for the START reply's incoming application channel.
   transportHandler.postDelayed(() -> {
      synchronized(AllData.this) {
         GarminPeer current=garminPeers.get(gp.id);
         if(current!=gp || !gp.sdkProbePending || token!=gp.sdkProbeGeneration ||
               sdk!=mConnectIQ || !sdkready()) return;
         }
      try {
         // Ordinary Kerfstok Direct peers detach application events, whereas
         // Libre3 keeps its record receive route. Ensure registration for START;
         // otherwise the START can reach the watch but its reply can never reach
         // IQlistener and every failback probe is guaranteed to time out.
         registerBothAppEvents(sdk,gp);
         sdk.sendMessage(device,appFor(gp),Arrays.asList(START),new IQSendMessageListener() {
            @Override public void onMessageStatus(IQDevice d,IQApp app,IQMessageStatus status) {
               if(status!=IQMessageStatus.SUCCESS) failGarminConnectProbe(gp.id,token,"START send "+status.name());
               }
            });
         transportHandler.postDelayed(() -> failGarminConnectProbe(gp.id,token,"no START reply"),
               GARMIN_CONNECT_PROBE_TIMEOUT_MS);
         }
      catch(Throwable th) {
         failGarminConnectProbe(gp.id,token,"START/register "+th.getClass().getSimpleName());
         }
      },80L);
   }

private void failGarminConnectProbe(long peerId,int token,String reason) {
   final DirectGarminIQ direct;
   final boolean reinit;
   final boolean wasGlucoseReady;
   final ConnectIQ sdk;
   final IQDevice device;
   synchronized(this) {
      GarminPeer gp=garminPeers.get(peerId);
      if(gp==null || !gp.sdkProbePending || token!=gp.sdkProbeGeneration) return;
      gp.sdkProbePending=false;
      reinit=gp.sdkProbeReinit;
      gp.sdkProbeReinit=false;
      wasGlucoseReady=gp.sdkProbeWasGlucoseReady;
      gp.sdkProbeWasGlucoseReady=false;
      gp.glucoseReady=wasGlucoseReady;
      ++gp.sdkProbeGeneration;
      direct=directGarmins.get(peerId);
      sdk=mConnectIQ;
      device=gp.device;
      if(doLog) Log.i(LOG_ID,"Automatic: Garmin Connect probe failed for "+gp.name+": "+reason+
            "; keep Direct BLE");
       if(direct!=null && direct.isActive() && sdk!=null && device!=null) {
          unregisterBothAppEvents(sdk,gp);
          direct.resumeApplication();
          }
      }
   // Ordinary Kerfstok drops the probe registration again. Libre3 keeps its
   // record receive route. Device-status events remain available for failback.
   if(reinit) reinitDirectInPlace(peerId);
   else {
      synchronized(this) {
         GarminPeer gp=garminPeers.get(peerId);
         if(gp!=null && gp.glucoseReady) flushPeerGlucose(gp);
         }
      }
   }

private void succeedGarminConnectProbe(long peerId,Object startReply,IQDevice device,IQApp app) {
   final GarminPeer gp;
   final DirectGarminIQ direct;
   final ConnectIQ sdk;
   final boolean reinit;
   synchronized(this) {
      gp=garminPeers.get(peerId);
      if(gp==null || !gp.sdkProbePending || !directActive(gp) ||
            currentTransportMode()!=GARMIN_TRANSPORT_AUTOMATIC || mConnectIQ==null || !sdkready()) return;
      sdk=mConnectIQ;
      gp.sdkProbePending=false;
      reinit=gp.sdkProbeReinit;
      gp.sdkProbeReinit=false;
      gp.sdkProbeWasGlucoseReady=false;
      ++gp.sdkProbeGeneration;
      direct=directGarmins.remove(peerId);
      gp.device=device;
      gp.connected=true;
      gp.sdkNoReplyCount=0;
      gp.sdkRetryPending=false;
      ++gp.sdkRetryGeneration;
      gp.setupRecoveryUsed=false;
      stopGarminLinkProbe(peerId);
      directRetryMessages.remove(peerId);
      if(automaticRetryPeerId==peerId) { automaticRetryPeerId=NO_PEER; automaticRetryMessage=null; }
      if(automaticPendingPeerId==peerId) cancelAutomaticApplicationReply();
      if(gp.queuedGlucose==null) gp.queuedGlucose=gp.inflightGlucose;
      gp.inflightGlucose=null;
      gp.glucoseReady=false;
      gp.startPending=false;
      gp.requestedNumberRole=-1;
      gp.retryNumbersAfterStart=isNumbersPeer(peerId) && numberPendingMessage!=null;
      ++gp.applicationSession;
      ++gp.startGeneration;
      ++gp.glucoseGeneration;
      gp.applicationBootstrapPending=false;
      ++gp.applicationBootstrapGeneration;
      if(reinit) {
         if(isNumbersPeer(peerId)) {
            clearsyncgegs();
            numbersSyncPending=numbersEnabled;
            }
         resetPeerForReinit(gp);
         gp.retryNumbersAfterStart=isNumbersPeer(peerId) && numberPendingMessage!=null;
         gp.launchRequested=true;
         }
      }
   if(doLog) Log.i(LOG_ID,"Automatic: Garmin Connect START round trip succeeded for "+gp.name+
         "; stop Direct BLE and use Garmin Connect");
   if(direct!=null) try { direct.stop(); } catch(Throwable ignored) {}
   // requestGarminConnectProbe() already registered Kerfstok application events,
   // and Automatic keeps the device-status listener while Direct owns the watch.
   // Do not register either listener a second time on successful failback.
   usewatch=true;
   if(reinit) registerPeerApplication(transportContext,gp);
   else IQlistener.onMessageReceived(device,app,Collections.singletonList(startReply),IQMessageStatus.SUCCESS);
   }

private void finishGarminConnectPreference(long peerId) {
   final GarminPeer gp;
   final ConnectIQ sdk;
   synchronized(this) {
      gp=garminPeers.get(peerId);
      sdk=mConnectIQ;
      if(gp==null || !gp.active || gp.watchStopped || transportRestarting ||
            currentTransportMode()!=GARMIN_TRANSPORT_AUTOMATIC || directActive(gp) ||
            sdk==null || !sdkready() || gp.device==null) return;
      gp.connected=true;
      }
   try { sdk.registerForDeviceEvents(gp.device,mDeviceEventListener); }
   catch(Throwable th) { Log.stack(LOG_ID,"prefer Garmin Connect register device "+gp.name,th); }
   try { registerBothAppEvents(sdk,gp); }
   catch(Throwable th) {
      Log.stack(LOG_ID,"prefer Garmin Connect register app "+gp.name,th);
      if(transportContext!=null) startDirectPeer(transportContext,gp,true);
      return;
      }
   usewatch=true;
   registerPeerApplication(transportContext,gp);
   }

/** Reinitialize Garmin's SDK after GCM appears while Direct BLE is carrying the
 * watch.  Do not stop Direct here: only onSdkReady() plus a subsequent real
 * Kerfstok packet is allowed to hand ownership back to Garmin Connect. */
private void requestGarminSdkRestartKeepingDirect() {
   final Context app;
   synchronized(this) {
      if(garminSdkRestartPending || currentTransportMode()!=GARMIN_TRANSPORT_AUTOMATIC ||
            transportContext==null || sdkready()) return;
      garminSdkRestartPending=true;
      app=transportContext;
      }
   transportHandler.post(() -> {
      synchronized(AllData.this) {
         if(currentTransportMode()!=GARMIN_TRANSPORT_AUTOMATIC || transportContext==null) {
            garminSdkRestartPending=false;
            return;
            }
         }
      if(doLog) Log.i(LOG_ID,"Automatic: reinitialize Garmin Connect SDK while Direct BLE remains active");
      try { unregister(); }
      catch(Throwable th) { Log.stack(LOG_ID,"restart Garmin Connect SDK shutdown",th); }
      // unregister() only tears down the SDK side. Recompute this legacy flag so
      // active Direct peers continue to be treated as usable during initialization.
      synchronized(AllData.this) { recomputeUsewatch(); }
      initGarminConnect(app);
      });
   }

/** Garmin Connect's service disappeared underneath an initialized SDK (for
 * example after force-stop).  Explicit Garmin-Connect mode must stay strict,
 * while Automatic may use Direct after the normal grace period.  In both modes
 * a later CONNECTED event/Refresh will reinitialize the SDK. */
void onGarminSdkShutDown(MyConnectIQListener listener) {
   final Context app;
   final int mode;
   synchronized(this) {
      if(mListener!=listener || transportRestarting || transportContext==null ||
            currentTransportMode()==GARMIN_TRANSPORT_DIRECT) return;
      garminSdkRestartPending=false;
      app=transportContext;
      mode=currentTransportMode();
      for(GarminPeer gp:garminPeers.values()) if(gp.active) {
         // connected is Garmin-Connect SDK state.  Do not destroy an independent
         // Direct application's proven START/glucose state merely because the
         // Garmin Connect service disappeared.  Doing so leaves the live Direct
         // link able to send commands such as COLORBLACK, while glucose is
         // silently queued forever behind glucoseReady=false.
         gp.connected=false;
         if(directActive(gp)) {
            if(doLog) Log.i(LOG_ID,"Garmin Connect SDK stopped while Direct owns "+gp.name+
                  "; preserve Direct Kerfstok session");
            continue;
            }
         gp.glucoseReady=false;
         gp.startPending=false;
         gp.applicationBootstrapPending=false;
         ++gp.applicationBootstrapGeneration;
         ++gp.startGeneration;
         ++gp.glucoseGeneration;
         if(gp.queuedGlucose==null) gp.queuedGlucose=gp.inflightGlucose;
         gp.inflightGlucose=null;
         gp.lastError="Garmin Connect SDK stopped";
         }
      }
   if(doLog) Log.i(LOG_ID,"Garmin Connect SDK stopped; hard SDK restart armed"+
         (mode==GARMIN_TRANSPORT_AUTOMATIC?"; Direct BLE fallback also armed":""));
   // Do not wait for a device-status callback from an SDK instance that is
   // already dead.  This was the reason Reinit could later see stale
   // connected=true yet do nothing.  Try one full SDK bootstrap proactively.
   transportHandler.postDelayed(() ->
         requestGarminSdkHardRestart("SDK shutdown",false),
         GARMIN_SDK_HARD_RESTART_DELAY_MS);
   if(mode==GARMIN_TRANSPORT_AUTOMATIC) {
      final MyConnectIQListener dead=listener;
      transportHandler.postDelayed(() -> {
         synchronized(AllData.this) {
            if(currentTransportMode()!=GARMIN_TRANSPORT_AUTOMATIC || transportContext==null || sdkready() ||
                  (mListener!=dead && mListener!=null && mListener.sdkready())) return;
            }
         if(doLog) Log.i(LOG_ID,"Automatic: Garmin Connect SDK still unavailable after shutdown; use Direct BLE");
         startDirectGarmin(app);
         },AUTOMATIC_DIRECT_DELAY_MS);
      }
   }

/** Perform the same complete ConnectIQ shutdown/initialize cycle that changing
 * the Garmin Status transport to "Via Garmin Connect" performs.  A mere
 * registerForAppEvents() against the old singleton is insufficient after some
 * Garmin Connect failures: the SDK can be present and a peer can still carry a
 * stale CONNECTED value while no application traffic is possible.
 *
 * Direct sessions are deliberately not stopped here.  In Automatic they remain
 * the fallback until the newly initialized SDK proves a peer with real Kerfstok
 * traffic/a START probe. */
private boolean requestGarminSdkHardRestart(String reason,boolean ignoreCooldown) {
   final Context app;
   synchronized(this) {
      if(currentTransportMode()==GARMIN_TRANSPORT_DIRECT || transportContext==null || transportRestarting)
         return false;
      if(garminSdkRestartPending) return true;
      final long now=System.currentTimeMillis();
      if(!ignoreCooldown && lastGarminSdkHardRestart!=0L &&
            now-lastGarminSdkHardRestart<GARMIN_SDK_HARD_RESTART_COOLDOWN_MS)
         return false;
      garminSdkRestartPending=true;
      lastGarminSdkHardRestart=now;
      app=transportContext;
      }
   transportHandler.post(() -> {
      synchronized(AllData.this) {
         if(currentTransportMode()==GARMIN_TRANSPORT_DIRECT || transportContext==null || transportRestarting) {
            garminSdkRestartPending=false;
            return;
            }
         }
      if(doLog) Log.i(LOG_ID,"Garmin Connect hard SDK rebootstrap: "+reason);
      try { unregister(); }
      catch(Throwable th) { Log.stack(LOG_ID,"hard Garmin Connect SDK shutdown",th); }
      // unregister() touches only the Garmin SDK. Direct peers must remain usable
      // while initialize() is pending/failing.
      synchronized(AllData.this) { recomputeUsewatch(); }
      initGarminConnect(app);
      });
   return true;
   }

private void requestGarminSdkRestartAfterShutdown() {
   requestGarminSdkHardRestart("service/status returned after SDK shutdown",false);
   }

void onGarminSdkReady(MyConnectIQListener listener) {
   synchronized(this) {
      if(!isCurrentGarminListener(listener)) return;
      garminSdkRestartPending=false;
      }
   }

void onGarminConnectMessage(IQDevice device,IQApp app,Object decoded) {
   if(device==null) return;
   final long peerId=device.getDeviceIdentifier();
   boolean verifyConnect=false;
   synchronized(this) {
      GarminPeer gp=garminPeers.get(peerId);
      if(gp==null || !gp.active || transportRestarting ||
            currentTransportMode()==GARMIN_TRANSPORT_DIRECT || app==null ||
            !acceptsGarminApplication(peerId,app.getApplicationId())) return;
      if(acceptsGarminConnectReturn(gp)) {
         int kind=messageKind(decoded);
         if(kind==GARMIN_LIBRE3_MINUTE || kind==GARMIN_LIBRE3_CLINICAL) {
            // These records do not complete a pending command or transfer
            // ownership. Reuse sensor validation/save and ACK through the
            // current owner, including Direct's queue while it reconnects.
            IQlistener.onPeerMessage(peerId,null,app,Collections.singletonList(decoded),IQMessageStatus.SUCCESS);
            return;
            }
         verifyConnect=!gp.sdkProbePending;
         }
      if(!verifyConnect) {
         if(gp.sdkProbePending && directActive(gp)) {
            if(messageKind(decoded)!=START) {
               if(doLog) Log.i(LOG_ID,"ignore non-START Garmin Connect packet during probe for "+gp.name);
               return;
               }
            // Complete outside the synchronized block; it stops Direct and may
            // register SDK callbacks.
         } else if(!acceptsGarminConnectPeer(peerId)) return;
         }
      }
   GarminPeer gp=garminPeers.get(peerId);
   if(verifyConnect) {
      if(!sdkready()) requestGarminSdkRestartKeepingDirect();
      else requestGarminConnectProbe(gp,false);
      return;
      }
   if(gp!=null && gp.sdkProbePending && directActive(gp) && messageKind(decoded)==START) {
      succeedGarminConnectProbe(peerId,decoded,device,app);
      return;
      }
   if(!acceptsGarminConnectPeer(peerId)) return;
   IQlistener.onMessageReceived(device,app,Collections.singletonList(decoded),IQMessageStatus.SUCCESS);
   }
private int currentTransportMode() {
   return transportMode<0 ? GARMIN_TRANSPORT_AUTOMATIC : transportMode;
   }

public int getGarminTransportMode(Context context) {
   if(transportMode<0) {
      int mode=GARMIN_TRANSPORT_AUTOMATIC;
      try {
         mode=context.getApplicationContext().getSharedPreferences(GARMIN_TRANSPORT_PREFS,Context.MODE_PRIVATE)
               .getInt(GARMIN_TRANSPORT_KEY,GARMIN_TRANSPORT_AUTOMATIC);
         }
      catch(Throwable th) {
         Log.stack(LOG_ID,"getGarminTransportMode",th);
         }
      if(mode<GARMIN_TRANSPORT_AUTOMATIC || mode>GARMIN_TRANSPORT_DIRECT)
         mode=GARMIN_TRANSPORT_AUTOMATIC;
      transportMode=mode;
      }
   return transportMode;
   }

public String garminTransportName(Context context) {
   switch(getGarminTransportMode(context)) {
      case GARMIN_TRANSPORT_CONNECT: return "Via Garmin Connect";
      case GARMIN_TRANSPORT_DIRECT: return "Direct BLE";
      default: return "Automatic";
      }
   }

public void setGarminTransportMode(Context context,int mode) {
   if(mode<GARMIN_TRANSPORT_AUTOMATIC || mode>GARMIN_TRANSPORT_DIRECT)
      mode=GARMIN_TRANSPORT_AUTOMATIC;
   int old=getGarminTransportMode(context);
   if(old==mode) return;
   transportMode=mode;
   try {
      context.getApplicationContext().getSharedPreferences(GARMIN_TRANSPORT_PREFS,Context.MODE_PRIVATE)
            .edit().putInt(GARMIN_TRANSPORT_KEY,mode).apply();
      }
   catch(Throwable th) {
      Log.stack(LOG_ID,"setGarminTransportMode",th);
      }
   {if(doLog) {Log.i(LOG_ID,"Garmin transport mode="+garminTransportName(context));};};
   restartGarmin(context);
   }

private void cancelAutomaticFallback() {
   ++transportGeneration;
   }

private synchronized void cancelAutomaticApplicationReply() {
   ++automaticTransactionGeneration;
   automaticPendingMessage=null;
   automaticPendingPeerId=NO_PEER;
   }

private long automaticReplyTimeout(Object message) {
   if(message instanceof List<?>) {
      List<?> li=(List<?>)message;
      if(!li.isEmpty() && li.get(0) instanceof Integer) {
         int kind=(Integer)li.get(0);
         if(kind==PUTNUMS || kind==NUMS || kind==MORENUMS) return 60000L;
         }
      }
   return AUTOMATIC_REPLY_TIMEOUT_MS;
   }

private void armAutomaticApplicationReply(long peerId,Object message) {
   if(currentTransportMode()==GARMIN_TRANSPORT_DIRECT || directGarmins.containsKey(peerId)) return;
   final int token;
   synchronized(this) {
      automaticPendingMessage=message;
      automaticPendingPeerId=peerId;
      token=++automaticTransactionGeneration;
      }
   transportHandler.postDelayed(() -> {
      Object failed;
      long failedPeer;
      synchronized(AllData.this) {
         if(token!=automaticTransactionGeneration || currentTransportMode()==GARMIN_TRANSPORT_DIRECT ||
               automaticPendingMessage!=message || automaticPendingPeerId!=peerId || directGarmins.containsKey(peerId)) return;
         failed=automaticPendingMessage;
         failedPeer=automaticPendingPeerId;
         automaticPendingMessage=null;
         automaticPendingPeerId=NO_PEER;
         ++automaticTransactionGeneration;
         }
      automaticFallbackForFailedMessage(failedPeer,failed,"no Kerfstok acknowledgement");
      },automaticReplyTimeout(message));
   }

private void noteAutomaticApplicationReply(long peerId,Object reply) {
   synchronized(this) {
      if(currentTransportMode()==GARMIN_TRANSPORT_DIRECT || automaticPendingMessage==null ||
            automaticPendingPeerId!=peerId || directGarmins.containsKey(peerId) ||
            !directReplyMatches(automaticPendingMessage,reply)) return;
      {if(doLog) {Log.i(LOG_ID,"Automatic: Kerfstok acknowledged "+peerId+" Garmin Connect transaction");};};
      automaticPendingMessage=null;
      automaticPendingPeerId=NO_PEER;
      ++automaticTransactionGeneration;
      }
   }

private boolean automaticGarminSendFailed(long peerId,Object message,String reason) {
   if(currentTransportMode()==GARMIN_TRANSPORT_DIRECT || transportContext==null || directGarmins.containsKey(peerId))
      return false;
   synchronized(this) {
      if(automaticPendingMessage!=message || automaticPendingPeerId!=peerId) return false;
      automaticPendingMessage=null;
      automaticPendingPeerId=NO_PEER;
      ++automaticTransactionGeneration;
      }
   automaticFallbackForFailedMessage(peerId,message,"Garmin Connect send "+reason);
   return true;
   }

private void automaticFallbackForFailedMessage(long peerId,Object message,String reason) {
   if(currentTransportMode()==GARMIN_TRANSPORT_DIRECT || transportContext==null) return;
   GarminPeer gp=garminPeers.get(peerId);
   if(gp==null || !gp.active) return;
   synchronized(this) {
      automaticRetryMessage=message;
      automaticRetryPeerId=peerId;
      directRetryMessages.put(peerId,message);
      }
   {if(doLog) {Log.i(LOG_ID,"Garmin Connect: "+gp.name+" "+reason+"; retain transaction: "+message);};};
   gp.lastError=reason;
   recoverGarminConnectPeer(gp);
   }

// Return true when an SDK initialization error is consumed by Automatic mode.
boolean onGarminConnectInitializationError(ConnectIQ.IQSdkErrorStatus status) {
   synchronized(this) { garminSdkRestartPending=false; }
   if(currentTransportMode()!=GARMIN_TRANSPORT_AUTOMATIC || transportContext==null) return false;
   {if(doLog) {Log.i(LOG_ID,"Automatic: Garmin Connect initialization "+status+"; use Direct BLE");};};
   transportHandler.post(() -> startDirectGarmin(transportContext));
   return true;
   }

/** Start the transport selected in Garmin Status. */
public void initIQ(Context context) {
   if(!Natives.getusegarmin()) return;
   transportContext=context.getApplicationContext();
   GarminLibre3Lifecycle.install(this,transportHandler);
   loadNumbersPreference(context);
   if(doLog) Log.v(LOG_ID,"initIQ v10.11 numbers="+numbersEnabled);
   int mode=getGarminTransportMode(context);
   if(mode==GARMIN_TRANSPORT_DIRECT) {
      devices=null;
      rebuildGarminPeers(context);
      startDirectGarmin(context);
      return;
      }
   initGarminConnect(context);
   }

private void initGarminConnect(Context context) {
   if(!Natives.getusegarmin()) return;
   try {
      mMyApp=new IQApp(kerfstokAppId());
      mLibre3App=new IQApp(LIBRE3_GARMIN_APP_ID);
      mConnectIQ=ConnectIQ.getInstance(getApplication(),IQConnectType.WIRELESS);
      if(doLog) Log.v(LOG_ID,"initIQ");
      mListener=new MyConnectIQListener(context,this);
      garminSdkContext=new GarminSdkContext(getApplication(),this);
      mConnectIQ.initialize(garminSdkContext,false,mListener);
      armGarminInitializationTimeout(mListener);
      }
   catch(Throwable error) {
      synchronized(this) { garminSdkRestartPending=false; }
      Log.stack(LOG_ID,"initIQ",error);
      if(currentTransportMode()==GARMIN_TRANSPORT_AUTOMATIC && transportContext!=null)
         startDirectGarmin(transportContext);
      }
   }

private void armGarminInitializationTimeout(MyConnectIQListener listener) {
   final int token=transportGeneration;
   transportHandler.postDelayed(() -> {
      if(token!=transportGeneration || !isCurrentGarminListener(listener) || listener.sdkready() ||
            currentTransportMode()!=GARMIN_TRANSPORT_AUTOMATIC || transportContext==null) return;
      synchronized(AllData.this) { garminSdkRestartPending=false; }
      if(doLog) Log.i(LOG_ID,"Automatic: Garmin Connect initialization callback timeout; keep/use Direct BLE");
      startDirectGarmin(transportContext);
      },AUTOMATIC_DIRECT_DELAY_MS);
   }

/** Start Direct BLE for every bonded/mapped Garmin peer. */
public boolean startDirectGarmin(Context context) { return startDirectGarmin(context,null); }

/** Start Direct BLE to an explicit already-bonded Garmin address. */
public boolean startDirectGarmin(Context context,String address) {
   if(!Natives.getusegarmin() || context==null) return false;
   try {
      transportContext=context.getApplicationContext();
      if(currentTransportMode()==GARMIN_TRANSPORT_DIRECT) {
         // Explicit Direct mode owns all Garmin application traffic.  Automatic
         // deliberately leaves GCM alive for healthy peers.
         unregister();
         mListener=null;
         }
      associateBondedGarmins(context);
      for(GarminPeer gp:garminPeers.values()) { loadGarminActive(context,gp); loadGarminGlucose(context,gp); loadGarminLibre3Direct(context,gp); loadGarminApp(context,gp); }
      ensureNumbersPeer();
      recomputeUsewatch();
      recomputeSendtowatch();
      boolean started=false;
      if(address!=null) {
         for(GarminPeer gp:garminPeers.values()) if(gp.active && address.equalsIgnoreCase(gp.directAddress))
            started|=startDirectPeer(context,gp);
         }
      else {
         for(GarminPeer gp:new ArrayList<>(garminPeers.values()))
            if(gp.active && gp.directAddress!=null) started|=startDirectPeer(context,gp);
         }
      usewatch=usewatch||started;
      return started;
      }
   catch(Throwable th) {
      Log.stack(LOG_ID,"startDirectGarmin",th);
      return false;
      }
   }

private boolean startDirectPeer(Context context,GarminPeer gp) {
   return startDirectPeer(context,gp,false);
   }

private synchronized boolean startDirectPeer(Context context,GarminPeer gp,boolean forceConnectedSdk) {
   if(!Natives.getusegarmin() || context==null || gp==null || !gp.active || gp.watchStopped || transportRestarting) return false;
   DirectGarminIQ old=directGarmins.get(gp.id);
   if(old!=null && old.isActive()) {
      // An active Direct GATT link may survive a Garmin-Connect SDK shutdown.
      // If some earlier recovery path nevertheless cleared glucoseReady, do not
      // wait for a reconnect callback that will never come: re-establish the
      // harmless START session probe on the already-live Direct connection.
      if(old.isReady() && !gp.glucoseReady && !gp.startPending && !gp.sdkProbePending && !old.hasPendingMessages()) {
         if(doLog) Log.i(LOG_ID,"Direct already ready but Kerfstok session not marked ready; START "+gp.name);
         sendPeerStart(gp);
         }
      return true;
      }
   // Resolve the bonded address before deciding between an application-owning
   // Direct session and an auxiliary recovery client on Garmin Connect's ACL.
   if(gp.directAddress==null) {
      final long peerId=gp.id;
      associateBondedGarmins(context);
      gp=garminPeers.get(peerId);
      if(gp!=null) { loadGarminActive(context,gp); loadGarminGlucose(context,gp); loadGarminApp(context,gp); }
      }
   if(gp==null || !gp.active) {
      if(doLog) Log.i(LOG_ID,"No active Garmin peer remained after bonded-device association");
      return false;
      }
   // SDK listener removal does not disconnect Garmin Connect's GATT client.
   // A connected but unresponsive SDK peer must not acquire a competing
   // Direct client on the same physical link (garmin16).
   if(!forceConnectedSdk && currentTransportMode()==GARMIN_TRANSPORT_AUTOMATIC && mConnectIQ!=null &&
         sdkready() && gp.device!=null && gp.connected) {
      scheduleGarminConnectRetry(gp);
      return false;
      }
   if(gp==null || gp.directAddress==null) {
      if(doLog) Log.i(LOG_ID,"No Direct BLE address mapped for "+(gp==null?"peer":gp.name));
      return false;
      }
   stopGarminLinkProbe(gp.id);
   DirectGarminIQ direct=new DirectGarminIQ(context,this,appIdFor(gp),alternateAppIdFor(gp),gp.directAddress,gp.id);
   if(forceConnectedSdk) direct.reuseExistingNotificationSubscription();
   if(gp.launchRequested) {
      // Consume the explicit Reinit request when handing it to this Direct
      // session. The Direct object itself guarantees it is not re-armed on reconnect.
      gp.launchRequested=false;
      direct.launchApplicationOnInitialConnection();
      }
   directGarmins.put(gp.id,direct);
   gp.glucoseReady=false;
   gp.sdkRetryPending=false;
   gp.sdkNoReplyCount=0;
   ++gp.sdkRetryGeneration;
   gp.acknowledgedGlucose=null;
   if(gp.queuedGlucose==null) gp.queuedGlucose=gp.inflightGlucose;
   gp.inflightGlucose=null;
   if(isNumbersPeer(gp.id) && numberPendingMessage!=null)
      directRetryMessages.put(gp.id,numberPendingMessage);
   // Installing the direct owner first makes the broadcast guard effective even
   // when shared SDK app registrations stay for another watch or Libre3 record
   // reception. Unregistration cannot stop GCM's physical BLE subscription.
   gp.applicationBootstrapPending=false;
   ++gp.applicationBootstrapGeneration;
   ++gp.startGeneration;
   ++gp.glucoseGeneration;
   if(mConnectIQ!=null && gp.device!=null) {
      unregisterBothAppEvents(mConnectIQ,gp);
      // In Automatic keep the inexpensive device-status callback registered.
      // Otherwise Juggluco cannot notice that Garmin Connect reconnects while
      // Direct owns application traffic, so failback would require a UI Refresh.
      if(currentTransportMode()==GARMIN_TRANSPORT_DIRECT) {
         try { mConnectIQ.unregisterForDeviceEvents(gp.device); }
         catch(Throwable th) { Log.stack(LOG_ID,"detach Garmin Connect device "+gp.name,th); }
         }
      }
   usewatch=true;
   if(doLog) Log.i(LOG_ID,"startDirectGarmin "+gp.name+" "+gp.directAddress);
   direct.start();
   return true;
   }

// Repair only the shared Garmin return channel/bearer. This object is kept out
// of directGarmins: Garmin Connect remains the application owner, incoming SDK
// messages remain accepted, and the status UI does not claim Direct BLE.
private synchronized boolean startGarminLinkProbe(GarminPeer gp) {
   if(!Natives.getusegarmin() || gp==null || !gp.active || gp.watchStopped || transportRestarting || transportContext==null ||
         currentTransportMode()==GARMIN_TRANSPORT_DIRECT || directGarmins.containsKey(gp.id) ||
         gp.directAddress==null) return false;
   DirectGarminIQ old=garminLinkProbes.get(gp.id);
   if(old!=null && old.isActive()) return true;
   if(old!=null) try { old.stop(); } catch(Throwable ignored) {}
   try {
      DirectGarminIQ probe=DirectGarminIQ.linkProbe(transportContext,this,appIdFor(gp),
            gp.directAddress,gp.id);
      garminLinkProbes.put(gp.id,probe);
      if(doLog) Log.i(LOG_ID,"Garmin Connect recovery: repair Kerfstok return channel for "+gp.name);
      probe.start();
      return true;
      }
   catch(Throwable th) {
      garminLinkProbes.remove(gp.id);
      Log.stack(LOG_ID,"start Garmin link probe "+gp.name,th);
      return false;
      }
   }

private synchronized void stopGarminLinkProbe(long peerId) {
   DirectGarminIQ probe=garminLinkProbes.remove(peerId);
   if(probe!=null) try { probe.stop(); } catch(Throwable ignored) {}
   }

private void garminLinkTrafficResumed(GarminPeer gp) {
   if(gp==null || !gp.active) return;
   final DirectGarminIQ probe;
   final boolean retry;
   final int token;
   final int session;
   final ConnectIQ sdk;
   synchronized(this) {
      probe=garminLinkProbes.remove(gp.id);
      retry=probe!=null && gp.sdkRetryPending;
      token=gp.sdkRetryGeneration;
      session=gp.applicationSession;
      sdk=mConnectIQ;
      }
   if(probe!=null) try { probe.stop(); } catch(Throwable ignored) {}
   // A matching reply will invalidate this token in peerAcknowledged below.
   // Unrelated but valid traffic proves the bearer is usable, so retry the
   // retained transaction promptly instead of waiting for the long fallback.
   if(retry) transportHandler.postDelayed(() -> retryGarminConnectPending(gp,token,session,sdk),100L);
   }

private synchronized void stopAllGarminLinkProbes() {
   ArrayList<DirectGarminIQ> probes=new ArrayList<>(garminLinkProbes.values());
   garminLinkProbes.clear();
   for(DirectGarminIQ probe:probes) if(probe!=null) try { probe.stop(); } catch(Throwable ignored) {}
   }

private void stopDirectGarminGracefully(final Runnable done) {
   stopAllGarminLinkProbes();
   final ArrayList<DirectGarminIQ> active=new ArrayList<>();
   synchronized(this) {
      for(DirectGarminIQ direct:directGarmins.values()) if(direct!=null && direct.isActive()) active.add(direct);
      }
   if(active.isEmpty()) {
      synchronized(this) { directGarmins.clear(); }
      if(done!=null) done.run();
      return;
      }
   final int[] left={active.size()};
   for(DirectGarminIQ direct:active) direct.stopGracefully(() -> {
      boolean last;
      synchronized(AllData.this) { last=(--left[0]==0); }
      if(last) {
         synchronized(AllData.this) { directGarmins.clear(); directRetryMessages.clear(); }
         if(done!=null) done.run();
         }
      });
   }

public synchronized void stopDirectGarmin() {
   stopAllGarminLinkProbes();
   for(DirectGarminIQ direct:directGarmins.values()) try { direct.stop(); } catch(Throwable ignored) {}
   directGarmins.clear();
   directRetryMessages.clear();
   setSending(false);
   }

public synchronized boolean directGarminActive() {
   for(DirectGarminIQ direct:directGarmins.values()) if(direct!=null && direct.isActive()) return true;
   return false;
   }

public synchronized boolean directGarminReady() {
   for(DirectGarminIQ direct:directGarmins.values()) if(direct!=null && direct.isReady()) return true;
   return false;
   }

void onDirectGarminAppInfo(long peerId,String appId,int version) {
   GarminPeer gp=garminPeers.get(peerId);
   if(gp==null || !gp.active || !supportedGarminAppId(appId)) return;
   boolean installed=version!=0xffff;
   recordLibre3Installation(gp,appId,installed);
   if(!installed) {
      if(appIdFor(gp).equals(normalizeAppId(appId))) gp.appVersion=-1;
      return;
      }
   String found=normalizeAppId(appId);
   if(!found.equals(appIdFor(gp))) {
      gp.appId=found;
      gp.appFallbackTried=false;
      if(transportContext!=null) persistGarminApp(transportContext,gp);
      if(doLog) Log.i(LOG_ID,"Direct installed Garmin app selected "+found+" for "+gp.name);
      }
   gp.appVersion=version;
   }

void onDirectGarminReady(long peerId) {
   GarminPeer gp=garminPeers.get(peerId);
   if(!Natives.getusegarmin() || gp==null || !gp.active || gp.watchStopped) return;
   usewatch=true;
   if(doLog) Log.i(LOG_ID,"Direct Garmin ready "+gp.name);
   // A slow GATT setup must finish under its own deadline, not be torn down
   // by the shorter application-recovery timer. Resume that timer only when
   // the setup/launch callbacks have actually opened the application send gate.
   if(gp.reinitRecoveryPending && gp.reinitRecoveryWaitingForSetup && transportContext!=null)
      armReinitHardEscalation(transportContext,gp);
   DirectGarminIQ direct=directGarmins.get(peerId);
   // Direct retains its active request and the following messages across a
   // reconnect. Do not duplicate them or insert a new START into that sequence.
   if(direct!=null && direct.hasPendingMessages()) return;

   Object retry;
   synchronized(this) {
      retry=directRetryMessages.remove(peerId);
      if(automaticRetryPeerId==peerId) {
         automaticRetryMessage=null;
         automaticRetryPeerId=NO_PEER;
         }
      }
   sendPeerStart(gp);
   if(retry!=null && isNumbersPeer(peerId)) {
      setSending(true);
      if(doLog) Log.i(LOG_ID,"resend outstanding by Direct BLE["+gp.name+"] "+retry);
      if(direct==null || !(messageNeedsReply(retry)?direct.send(retry):direct.sendNoReply(retry))) setSending(false);
      }
   }

void onDirectGarminDisconnected(long peerId) {
   GarminPeer gp=garminPeers.get(peerId);
   if(gp==null || !gp.active || gp.watchStopped) return;
   if(doLog) Log.i(LOG_ID,"Direct Garmin disconnected "+gp.name);
   gp.glucoseReady=false;
   gp.startPending=false;
   gp.glucoseGeneration++;
   gp.startGeneration++;
   if(gp.queuedGlucose==null) gp.queuedGlucose=gp.inflightGlucose;
   gp.inflightGlucose=null;
   if(isNumbersPeer(peerId)) setSending(false);
   }

void onDirectGarminSendSuccess(long peerId) {
   GarminPeer gp=garminPeers.get(peerId);
   sendstatus=IQMessageStatus.SUCCESS;
   statustime=System.currentTimeMillis();
   if(gp!=null) { gp.lastStatus=IQMessageStatus.SUCCESS; gp.lastStatusTime=statustime; }
   if(doLog) Log.i(LOG_ID,"Direct Garmin transport SUCCESS "+(gp==null?peerId:gp.name));
   }

synchronized void onGarminNoReplyComplete(long peerId,Object message) {
   GarminPeer gp=garminPeers.get(peerId);
   if(gp==null || !gp.active) return;
   if(isGlucoseMessage(message) && message instanceof List<?> && ((List<?>)message).size()>1 &&
         gp.inflightGlucose!=null && gp.inflightGlucose.equals(((List<?>)message).get(1))) {
      // Direct's encrypted transfer reached Kerfstok. This path is used only
      // when Garmin Connect shares the bearer and is intentionally left to own
      // the redundant [229] return connection.
      if(gp.queuedGlucose!=null && gp.queuedGlucose.equals(gp.inflightGlucose)) gp.queuedGlucose=null;
      gp.acknowledgedGlucose=gp.inflightGlucose;
      gp.inflightGlucose=null;
      ++gp.glucoseGeneration;
      flushPeerGlucose(gp);
      return;
      }
   if(isNumbersPeer(peerId) && numberPendingMessage==message && !messageNeedsReply(message)) {
      numberPendingMessage=null;
      nextmessage();
      }
   }

void onDirectGarminMessage(long peerId,String sourceAppId,Object decoded) {
   try {
      GarminPeer gp=garminPeers.get(peerId);
      if(gp==null || !gp.active) return;
      if(supportedGarminAppId(sourceAppId)) {
         recordLibre3Installation(gp,sourceAppId,true);
         String incoming=normalizeAppId(sourceAppId);
         if(!incoming.equals(appIdFor(gp))) {
            gp.appId=incoming;
            gp.appFallbackTried=false;
            if(transportContext!=null) persistGarminApp(transportContext,gp);
            if(doLog) Log.i(LOG_ID,"Direct foreground Garmin app selected "+incoming+" for "+gp.name);
         }
      }
      List<Object> envelope=Collections.singletonList(decoded);
      IQlistener.onPeerMessage(peerId,null,null,envelope,IQMessageStatus.SUCCESS);
      }
   catch(Throwable th) {
      Log.stack(LOG_ID,"onDirectGarminMessage",th);
      if(isNumbersPeer(peerId)) setSending(false);
      }
   }

/** A failed setup is also a completion event. Once Direct has finished closing
 * its failed client, perform the deferred hard recovery instead of leaving the
 * explicit Reinit dependent on repeated setup retries. */
private void resumeReinitAfterDirectSetupFailure(GarminPeer gp,DirectGarminIQ direct) {
   if(gp==null || !gp.active || !gp.reinitRecoveryPending || !gp.reinitRecoveryWaitingForSetup ||
         transportContext==null || direct==null || direct.isReady() || direct.isConnectionSetupPending()) return;
   final Context app=transportContext;
   final int token=gp.reinitRecoveryGeneration;
   transportHandler.post(() -> hardReinitIfStillUnproven(app,gp.id,token));
   }

void onDirectGarminError(long peerId,Object failedMessage,String message) {
   GarminPeer gp=garminPeers.get(peerId);
   if(gp==null || !gp.active || gp.watchStopped) return;
   if(gp!=null) gp.lastError=message;
   Log.e(LOG_ID,"Direct Garmin["+(gp==null?peerId:gp.name)+"] "+message);
   DirectGarminIQ direct=directGarmins.get(peerId);
   resumeReinitAfterDirectSetupFailure(gp,direct);
   if(direct!=null && direct.hasPendingMessages()) {
      // The transport owns the retry, including START and glucose. Keep each
      // application's in-flight state and any newer queued glucose intact.
      if(doLog) Log.i(LOG_ID,"Direct retains outstanding work for retry "+peerId);
      return;
      }
   if(gp!=null && isGlucoseMessage(failedMessage)) {
      try {
         List<?> m=(List<?>)failedMessage;
         if(m.size()>1 && m.get(1) instanceof List<?>) gp.queuedGlucose=(List<Object>)m.get(1);
         }
      catch(Throwable ignored) {}
      gp.inflightGlucose=null;
      gp.glucoseGeneration++;
      }
   int failedKind=messageKind(failedMessage);
   if(failedMessage!=null && isNumbersPeer(peerId) && failedKind!=NUMBERROLE && failedKind!=START && failedKind!=GLUCOSE) {
      synchronized(this) { directRetryMessages.put(peerId,failedMessage); }
      setSending(true); // keep the number queue blocked until reconnect/retry
      }
   else if(isNumbersPeer(peerId) && failedKind!=GLUCOSE) setSending(false);
   }

private void recoverGarminConnectPeer(GarminPeer gp) {
   if(gp==null || !gp.active || transportContext==null || currentTransportMode()==GARMIN_TRANSPORT_DIRECT) return;
   if(currentTransportMode()==GARMIN_TRANSPORT_CONNECT) {
      // Explicit "Via Garmin Connect" remains strict: never create a Direct BLE
      // connection behind the user's back. Retry/reinitialize only through the SDK.
      scheduleGarminConnectRetry(gp);
      return;
      }

   // In Automatic, one missing application reply may still be a transient GCM
   // return-channel stall, so give the existing recovery one chance. A second
   // consecutive timeout while the SDK still says CONNECTED is the signature
   // of the current GCM failure mode: transport status is healthy but Connect
   // IQ application messaging is not. At that point Direct BLE must be allowed
   // to take ownership even though gp.connected remains true.
   int failures=++gp.sdkNoReplyCount;
   if(failures<2) {
      if(gp.inflightGlucose!=null) {
         // A SUCCESS send followed by no [229] often means the per-watch
         // Connect-IQ application session is wedged while the physical Garmin
         // connection remains healthy. Reset only this Kerfstok session with
         // START immediately. A START reply requeues/resends the exact glucose;
         // a missing START reply becomes the second failure and permits Direct.
         if(doLog) Log.i(LOG_ID,"Automatic: Garmin Connect glucose reply missing for "+gp.name+
               "; reset Kerfstok session with START");
         sendPeerStart(gp);
         }
      else {
         if(doLog) Log.i(LOG_ID,"Automatic: Garmin Connect application reply missing for "+gp.name+
               "; retry once through Garmin Connect");
         scheduleGarminConnectRetry(gp);
         }
      return;
      }
   if(doLog) Log.i(LOG_ID,"Automatic: Garmin Connect remains CONNECTED but application messaging is unresponsive for "+
         gp.name+"; switch to Direct BLE");
   if(!startDirectPeer(transportContext,gp,true)) {
      // No mapped address / Direct setup could not start. Keep the old recovery
      // rather than dropping the pending START/glucose transaction.
      scheduleGarminConnectRetry(gp);
      }
   }

// Retry the exact retained operation through Garmin Connect.  The generation
// guards make the scheduled fallback and the link-probe callback race-safe.
private void retryGarminConnectPending(GarminPeer gp,int token,int session,ConnectIQ sdk) {
   synchronized(AllData.this) {
      if(token!=gp.sdkRetryGeneration || !gp.sdkRetryPending) return;
      gp.sdkRetryPending=false;
      ++gp.sdkRetryGeneration; // invalidate the probe callback/absolute fallback sibling
      stopGarminLinkProbe(gp.id);
      if(session!=gp.applicationSession || sdk!=mConnectIQ || gp.watchStopped ||
            !gp.connected || !acceptsGarminConnectPeer(gp.id)) return;
      directRetryMessages.remove(gp.id);
      if(automaticRetryPeerId==gp.id) {
         automaticRetryMessage=null;
         automaticRetryPeerId=NO_PEER;
         }
      if(gp.startPending || !gp.glucoseReady) {
         if(gp.numberRoleCapable && gp.requestedNumberRole>=0)
            sendPeerStartMessage(gp,Arrays.asList(NUMBERROLE,isNumbersPeer(gp.id)?1:0),isNumbersPeer(gp.id)?1:0);
         else sendPeerStart(gp);
         return;
         }
      if(isNumbersPeer(gp.id) && numberPendingMessage!=null) {
         sendPeerObject(gp,numberPendingMessage,messageNeedsReply(numberPendingMessage),true);
         return;
         }
      if(gp.inflightGlucose!=null) {
         if(gp.queuedGlucose==null) gp.queuedGlucose=gp.inflightGlucose;
         gp.inflightGlucose=null;
         ++gp.glucoseGeneration;
         }
      flushPeerGlucose(gp);
      if(isNumbersPeer(gp.id)) nextmessage();
      }
   }

// DirectGarminIQ invokes this only for its auxiliary recovery client. It either
// completed the stranded Kerfstok return transfer, found the bearer responsive, or let
// Android's native watchdog reset an actually stalled shared bearer.
void onGarminLinkProbeFinished(long peerId,DirectGarminIQ source,int result,String detail) {
   final GarminPeer gp;
   final int token;
   final int session;
   final ConnectIQ sdk;
   synchronized(this) {
      if(garminLinkProbes.get(peerId)!=source) return;
      garminLinkProbes.remove(peerId);
      gp=garminPeers.get(peerId);
      if(gp==null || !gp.sdkRetryPending || gp.watchStopped || transportRestarting) return;
      token=gp.sdkRetryGeneration;
      session=gp.applicationSession;
      sdk=mConnectIQ;
      }
   if(doLog) Log.i(LOG_ID,"Garmin Connect recovery: link probe "+gp.name+" result="+result+" "+detail);
   if(result==DirectGarminIQ.LINK_PROBE_RESPONSIVE ||
         result==DirectGarminIQ.LINK_PROBE_RETURN_RELEASED) {
      transportHandler.postDelayed(() -> retryGarminConnectPending(gp,token,session,sdk),100L);
      }
   else if(result==DirectGarminIQ.LINK_PROBE_RESET) {
      // Prefer the SDK's DISCONNECTED/CONNECTED callbacks.  If Garmin Connect
      // hides the brief physical reset, this guarded retry still resumes work.
      transportHandler.postDelayed(() -> retryGarminConnectPending(gp,token,session,sdk),
            GARMIN_LINK_RESET_SETTLE_MS);
      }
   else if(result==DirectGarminIQ.LINK_PROBE_FAILED) {
      // The probe may fail for a local permission/service-cache reason even
      // though Garmin Connect can still send. Do not wait for the long service
      // discovery safety deadline before retrying the retained SDK operation.
      transportHandler.postDelayed(() -> retryGarminConnectPending(gp,token,session,sdk),1000L);
      }
   }

// Retry outstanding work through the connected SDK without replacing its
// application ownership. Keep the existing retry pacing; a healthy outgoing
// setting command does not explain or repair a missing incoming glucose record.
private synchronized void scheduleGarminConnectRetry(GarminPeer gp) {
   if(gp==null || !gp.active || gp.sdkRetryPending || gp.watchStopped || transportRestarting ||
         currentTransportMode()==GARMIN_TRANSPORT_DIRECT || !gp.connected || mConnectIQ==null) return;
   gp.sdkRetryPending=true;
   final int token=++gp.sdkRetryGeneration;
   final int session=gp.applicationSession;
   final ConnectIQ sdk=mConnectIQ;
   if(doLog) Log.i(LOG_ID,"Garmin Connect recovery: keep application ownership for "+gp.name+
         "; retry through Garmin Connect and retain pending work");
   // Never open an auxiliary Direct GATT client while Garmin Connect owns the
   // watch. Besides unnecessary traffic, the second client can interfere with
   // Garmin Connect (notably on VA3). Retry only through Garmin Connect here.
   transportHandler.postDelayed(() -> retryGarminConnectPending(gp,token,session,sdk),
         GARMIN_RETRY_WITHOUT_PROBE_MS);
   }

// A failed Direct setup may yield to the SDK once per explicit initialization.
// Reinitializing the entire SDK every minute caused the garmin16 switch loop.
synchronized boolean recoverGarminConnectAfterDirectSetupFailure(long peerId,DirectGarminIQ source) {
   GarminPeer gp=garminPeers.get(peerId);
   if(gp==null || !gp.active || directGarmins.get(peerId)!=source || gp.watchStopped || transportRestarting ||
         currentTransportMode()!=GARMIN_TRANSPORT_AUTOMATIC || transportContext==null ||
         gp.device==null || mConnectIQ==null) return false;
   if(gp.setupRecoveryUsed) return false;
   gp.setupRecoveryUsed=true;
   // Keep other watches' SDK registrations and Direct sessions intact.
   if(doLog) Log.i(LOG_ID,"Automatic: repeated Direct setup failure for "+gp.name+"; retry this peer through Garmin Connect");
   source.stop();
   directGarmins.remove(peerId);
   directRetryMessages.remove(peerId);
   if(automaticRetryPeerId==peerId) {
      automaticRetryPeerId=NO_PEER;
      automaticRetryMessage=null;
      }
   if(isNumbersPeer(peerId)) {
      cancelAutomaticApplicationReply();
      if(numberPendingMessage!=null) mqueue.addFirst(numberPendingMessage);
      numberPendingMessage=null;
      setSending(false);
      }
   if(gp.queuedGlucose==null) gp.queuedGlucose=gp.inflightGlucose;
   gp.inflightGlucose=null;
   gp.glucoseReady=false;
   gp.startPending=false;
   gp.requestedNumberRole=-1;
   gp.applicationSession++;
   gp.startGeneration++;
   gp.glucoseGeneration++;
   gp.applicationBootstrapPending=false;
   gp.applicationBootstrapGeneration++;
   // The captured failure showed that manually selecting "Via Garmin Connect"
   // fixed VA3 whereas this old per-peer re-registration did not.  Perform one
   // complete SDK bootstrap first; it refreshes Garmin's device objects and all
   // app/device registrations without stopping unrelated Direct sessions.
   if(requestGarminSdkHardRestart("repeated Direct setup failure for "+gp.name,false))
      return true;

   // A hard restart may be rate-limited because another peer already requested
   // one. If that bootstrap is not pending and the SDK is genuinely ready, keep
   // the cheaper per-peer restoration as a fallback. Never call it on a dead SDK.
   if(sdkready()) {
      try {
         mConnectIQ.registerForDeviceEvents(gp.device,mDeviceEventListener);
         registerBothAppEvents(mConnectIQ,gp);
         usewatch=true;
         registerPeerApplication(transportContext,gp);
         }
      catch(Throwable th) {
         Log.stack(LOG_ID,"restore Garmin Connect peer "+gp.name,th);
         startDirectPeer(transportContext,gp);
         }
      }
   else if(transportContext!=null) {
      // No usable SDK and no restart could be started because of the cooldown.
      // Return to Direct after a short settle rather than issuing SDK calls to a
      // stale singleton.
      final GarminPeer target=gp;
      transportHandler.postDelayed(() -> {
         synchronized(AllData.this) {
            if(currentTransportMode()!=GARMIN_TRANSPORT_AUTOMATIC || !target.active ||
                  directActive(target) || sdkready()) return;
            }
         startDirectPeer(transportContext,target);
         },1500L);
      }
   return true;
   }

public void stop() {
     // Invalidate a pending graceful restart before Direct.stop() runs its
     // completion callback. Cancel this owner's queued recovery work as well.
     ++reinitGeneration;
     transportContext=null;
     transportRestarting=false;
     garminSdkRestartPending=false;
     transportHandler.removeCallbacksAndMessages(null);
     cancelAutomaticFallback();
     cancelAutomaticApplicationReply();
     synchronized(this) {
        automaticRetryMessage=null;
        automaticRetryPeerId=NO_PEER;
        directRetryMessages.clear();
        }
     stopDirectGarmin();
     unregister();
     devices=null;
     mListener=null;
     for(GarminPeer gp:garminPeers.values()) {
        gp.connected=false;
        gp.glucoseReady=false;
        gp.sdkRetryPending=false;
        ++gp.sdkRetryGeneration;
        gp.reinitRecoveryPending=false;
        ++gp.reinitRecoveryGeneration;
        }
     }

private void resetPeerForReinit(GarminPeer gp) {
   gp.glucoseReady=false;
   if(gp.queuedGlucose==null) gp.queuedGlucose=gp.inflightGlucose;
   gp.inflightGlucose=null;
   gp.acknowledgedGlucose=null;
   gp.watchStopped=false;
   gp.startPending=false;
   gp.timestampedGlucoseAck=false;
   gp.numberRoleCapable=false;
   gp.requestedNumberRole=-1;
   gp.confirmedNumberRole=-1;
   gp.setupRecoveryUsed=false;
   gp.sdkRetryPending=false;
   ++gp.sdkRetryGeneration;
   gp.sdkProbePending=false;
   gp.sdkProbeReinit=false;
   gp.sdkProbeWasGlucoseReady=false;
   gp.retryNumbersAfterStart=false;
   ++gp.sdkProbeGeneration;
   gp.applicationSession++;
   gp.lastError=null;
   gp.applicationStateTime=System.currentTimeMillis();
   gp.startGeneration++;
   gp.applicationBootstrapPending=false;
   gp.applicationBootstrapGeneration++;
   }

private void armReinitHardEscalation(Context app,GarminPeer gp) {
   final int token;
   synchronized(this) {
      gp.reinitRecoveryPending=true;
      gp.reinitRecoveryWaitingForSetup=false;
      token=++gp.reinitRecoveryGeneration;
      }
   transportHandler.postDelayed(() -> hardReinitIfStillUnproven(app,gp.id,token),
         REINIT_HARD_ESCALATION_MS);
   }

/** Second stage of an explicit Reinit.  A normal Juggluco process restart closes
 * its Direct GATT clients and recreates the ConnectIQ SDK/device registrations.
 * The first Reinit stage intentionally avoids that disruption when a live link
 * might be recoverable in place.  If Kerfstok still has not answered START, do
 * the restart-level work automatically: fresh selected-watch Direct GATT, or a
 * complete Garmin Connect SDK shutdown/initialize cycle.  Existing Direct's
 * unanswered-open recovery can additionally perform BluetoothGatt.refresh(),
 * which is stronger than an ordinary Juggluco restart. */
private void hardReinitIfStillUnproven(Context app,long peerId,int token) {
   final GarminPeer gp;
   final DirectGarminIQ oldDirect;
   final int mode;
   synchronized(this) {
      gp=garminPeers.get(peerId);
      if(gp==null || !gp.active || !gp.reinitRecoveryPending ||
            token!=gp.reinitRecoveryGeneration) return;
      DirectGarminIQ settingUp=directGarmins.get(peerId);
      if(settingUp!=null && settingUp.isConnectionSetupPending()) {
         // Do not cancel Android's in-flight ATT request before its response
         // timeout. Direct already owns setup failure/retry; its ready/error
         // callbacks resume escalation without another polling timer.
         gp.reinitRecoveryWaitingForSetup=true;
         if(doLog) Log.i(LOG_ID,"Reinit: defer application reset until Direct Bluetooth setup finishes for "+gp.name);
         return;
         }
      gp.reinitRecoveryPending=false;
      gp.reinitRecoveryWaitingForSetup=false;
      ++gp.reinitRecoveryGeneration;
      mode=currentTransportMode();
      oldDirect=directGarmins.remove(peerId);
      stopGarminLinkProbe(peerId);
      directRetryMessages.remove(peerId);
      if(automaticRetryPeerId==peerId) { automaticRetryPeerId=NO_PEER; automaticRetryMessage=null; }
      if(automaticPendingPeerId==peerId) cancelAutomaticApplicationReply();
      if(isNumbersPeer(peerId)) {
         clearsyncgegs();
         numbersSyncPending=numbersEnabled;
         }
      // Escalation is still the same button press. Keep the request if it has
      // not yet been handed to a transport (e.g. an Automatic probe), or transfer
      // an unspent Direct request; never re-arm one already sent to the watch.
      if(oldDirect!=null) gp.launchRequested|=oldDirect.hasPendingApplicationLaunch();
      resetPeerForReinit(gp);
      }
   // Do not start native service discovery with refresh() immediately before
   // closing its GATT owner. Direct's cache recovery now runs in place and owns
   // that ATT operation until completion; hard escalation only closes/recreates.
   if(oldDirect!=null) try { oldDirect.stop(); } catch(Throwable ignored) {}
   if(doLog) Log.i(LOG_ID,"Reinit hard escalation for "+gp.name+
         ": recreate "+(mode==GARMIN_TRANSPORT_DIRECT?"Direct GATT":
               (mode==GARMIN_TRANSPORT_AUTOMATIC?"Garmin Connect SDK + Direct fallback":"Garmin Connect SDK")));

   if(mode==GARMIN_TRANSPORT_DIRECT) {
      // Refresh the physical mapping just as Direct startup does, then create a
      // completely new BluetoothGatt/application session for the selected watch.
      synchronized(this) { associateBondedGarmins(app); }
      GarminPeer current;
      synchronized(this) { current=garminPeers.get(peerId); }
      if(current!=null && current.active) {
         startDirectPeer(app,current);
         }
      return;
      }

   // ConnectIQ is process-global, so restart-level recovery necessarily rebuilds
   // the SDK registrations for all Garmin devices. Direct links for other peers
   // are left alone.
   requestGarminSdkHardRestart("Reinit hard escalation for "+gp.name,true);

   if(mode==GARMIN_TRANSPORT_AUTOMATIC) {
      // Reinit has already waited for the in-place START attempt and therefore
      // must not leave this peer dependent on the same stale Garmin-Connect
      // status. After the SDK restart runnable has been queued, recreate Direct
      // immediately as the Automatic fallback. Whichever transport proves a
      // real Kerfstok START first owns application traffic; the existing
      // failback path still gives preference to Garmin Connect.
      transportHandler.post(() -> {
         GarminPeer current;
         synchronized(AllData.this) {
            if(currentTransportMode()!=GARMIN_TRANSPORT_AUTOMATIC) return;
            associateBondedGarmins(app);
            current=garminPeers.get(peerId);
            if(current==null || !current.active || current.watchStopped ||
                  current.glucoseReady || directActive(current)) return;
            }
         startDirectPeer(app,current,true);
         });
      }
   }

/** Reinitialize only one selected watch. This is the only watch-specific action
 * that grants permission to launch Kerfstok. Other Garmin peers keep running. */
public void reinit(Context context,long peerId) {
   if(context==null || peerId==NO_PEER) return;
   final Context app=context.getApplicationContext();
   transportContext=app;
   final GarminPeer gp;
   final DirectGarminIQ direct;
   final int mode;
   synchronized(this) {
      gp=garminPeers.get(peerId);
      if(gp==null || !gp.active) return;
      // Retain this button press even while Automatic probes Garmin Connect
      // before handing the one-shot launch to either transport.
      gp.launchRequested=true;
      direct=directGarmins.get(peerId);
      mode=currentTransportMode();
      }
   // Reinit is not just a hint.  The first attempt may preserve a live Direct
   // bearer or reuse a healthy-looking SDK, but lack of a real START response
   // automatically escalates to a full restart-level recovery.
   armReinitHardEscalation(app,gp);

   if(direct!=null && direct.isActive()) {
      // Do not tear down a working Direct GATT connection merely to reinitialize
      // Kerfstok. On VA3 a new CCCD write can stall while Garmin Connect also has
      // a physical link. If Garmin Connect currently looks connected, first verify
      // its application messaging with one START round trip; otherwise reinit the
      // existing Direct session in place.
      if(mode==GARMIN_TRANSPORT_AUTOMATIC) {
         if(sdkready() && gp.device!=null && gp.connected) {
            requestGarminConnectProbe(gp,true);
            return;
            }
         // Reinit is explicit user recovery.  If the preferred transport is not
         // convincingly alive, reproduce the full SDK restart that manually
         // choosing "Via Garmin Connect" used to provide, while keeping this
         // working/partly-working Direct link as fallback.
         requestGarminSdkHardRestart("Reinit Automatic for "+gp.name,true);
         }
      reinitDirectInPlace(peerId);
      return;
      }

   synchronized(this) { gp.launchRequested=true; }
   transportHandler.post(() -> reinitPeerAfterStop(app,peerId,true));
   }

private void reinitDirectInPlace(long peerId) {
   final GarminPeer gp;
   final DirectGarminIQ direct;
   synchronized(this) {
      gp=garminPeers.get(peerId);
      direct=directGarmins.get(peerId);
      if(gp==null || !gp.active || direct==null || !direct.isActive()) return;
      stopGarminLinkProbe(peerId);
      directRetryMessages.remove(peerId);
      if(automaticRetryPeerId==peerId) { automaticRetryPeerId=NO_PEER; automaticRetryMessage=null; }
      if(automaticPendingPeerId==peerId) cancelAutomaticApplicationReply();
      if(isNumbersPeer(peerId)) {
         clearsyncgegs();
         numbersSyncPending=numbersEnabled;
         }
      gp.launchRequested=false; // DirectGarminIQ owns the one-shot launch below.
      resetPeerForReinit(gp);
      }
   if(doLog) Log.i(LOG_ID,"Reinit Direct for "+gp.name+
         (direct.isReady()?"; keep ready GATT connection":"; recover transport now"));
   direct.reinitApplication();
   // START is prioritized by DirectGarminIQ and runs immediately after the
   // application-info/openApplication part of Reinit has completed.
   sendPeerStart(gp);
   }

private synchronized void reinitPeerAfterStop(Context app,long peerId,boolean launchKerfstok) {
   GarminPeer gp=garminPeers.get(peerId);
   if(gp==null || !gp.active) return;

   DirectGarminIQ old=directGarmins.remove(peerId);
   final boolean wasDirect=old!=null && old.isActive();
   if(wasDirect) try { old.stop(); } catch(Throwable ignored) {}
   stopGarminLinkProbe(peerId);
   directRetryMessages.remove(peerId);
   if(automaticRetryPeerId==peerId) { automaticRetryPeerId=NO_PEER; automaticRetryMessage=null; }
   if(automaticPendingPeerId==peerId) cancelAutomaticApplicationReply();
   if(isNumbersPeer(peerId)) {
      clearsyncgegs();
      numbersSyncPending=numbersEnabled;
      }

   // Only an explicit Reinit may launch Kerfstok. Loss-of-signal recovery
   // uses the same transport reset without displaying a watch launch prompt.
   gp.launchRequested=launchKerfstok;
   resetPeerForReinit(gp);

   int mode=currentTransportMode();
   // An alarm is not the user's Reinit button. Repair the current Direct return
   // owner instead of spending another SDK timeout on every stale-data alarm.
   // Automatic still hands back through its existing proven-SDK-reply path;
   // explicit Reinit below still retries the preferred Garmin Connect route.
   final boolean keepDirect=!launchKerfstok && wasDirect && mode==GARMIN_TRANSPORT_AUTOMATIC;
   if(mode==GARMIN_TRANSPORT_DIRECT || keepDirect) {
      if(keepDirect && doLog) Log.i(LOG_ID,"Loss of signal: retain Direct return owner v59 "+gp.name);
      // Override the connected-SDK gate only when that gate actually applies.
      // Without an SDK owner, perform normal Direct notification setup.
      final boolean sharedSdk=keepDirect && mConnectIQ!=null && sdkready() &&
            gp.device!=null && gp.connected;
      startDirectPeer(app,gp,sharedSdk);
      return;
      }

   if(mode!=GARMIN_TRANSPORT_DIRECT &&
         (!sdkready() || mConnectIQ==null || gp.device==null ||
          (mode==GARMIN_TRANSPORT_AUTOMATIC && !gp.connected))) {
      requestGarminSdkHardRestart("Reinit Automatic SDK/device refresh for "+gp.name,true);
      // Automatic is allowed to keep trying Direct concurrently.  The newly
      // initialized SDK will later reclaim only peers whose Kerfstok messaging
      // is actually proven healthy.
      if(mode==GARMIN_TRANSPORT_AUTOMATIC && gp.directAddress!=null)
         startDirectPeer(app,gp);
      return;
      }

   if(mConnectIQ!=null && gp.device!=null) {
      // Reinit in Automatic is also an explicit request to retry the preferred
      // transport. Refresh Garmin Connect's current status after Direct has been
      // stopped instead of recreating Direct just because it was previously in use.
      if(mode==GARMIN_TRANSPORT_AUTOMATIC) {
         try { gp.connected=gp.device.getStatus()==IQDeviceStatus.CONNECTED; } catch(Throwable ignored) {}
         if(doLog) Log.i(LOG_ID,"Reinit Automatic: try Garmin Connect first for "+gp.name+
               " connected="+gp.connected);
         }
      unregisterBothAppEvents(mConnectIQ,gp);
      try { mConnectIQ.registerForDeviceEvents(gp.device,mDeviceEventListener); }
      catch(Throwable th) { Log.stack(LOG_ID,"reinit register device "+gp.name,th); }
      registerBothAppEvents(mConnectIQ,gp); usewatch=true;
      if(gp.connected) {
         registerPeerApplication(app,gp);
         return;
         }
      if(mode==GARMIN_TRANSPORT_CONNECT) return;
      }

   // Garmin Connect is not currently connected/usable. Automatic falls back to
   // Direct immediately; if Garmin Connect later emits real Kerfstok traffic,
   // prepareGarminConnectIncoming() will hand ownership back automatically.
   if(mode==GARMIN_TRANSPORT_AUTOMATIC && gp.directAddress!=null)
      startDirectPeer(app,gp);
   }

public void reinit(Context context) {
   restartGarmin(context,true);
   }

/** Restart Garmin transport without launching Kerfstok. */
public void restartGarmin(Context context) {
   restartGarmin(context,false);
   }

private void restartGarmin(Context context,boolean launchKerfstok) {
   if(!Natives.getusegarmin()) return;
   final Context app=context.getApplicationContext();
   transportRestarting=true;
   // This is the only place that arms openApplication. It is reached with true
   // only from the explicit Reinit button. Other restarts also clear any stale
   // one-shot permission that has not yet been consumed.
   synchronized(this) {
      for(GarminPeer gp:garminPeers.values()) gp.launchRequested=launchKerfstok;
      }
   transportContext=app;
   final int token=++reinitGeneration;
   cancelAutomaticFallback();
   cancelAutomaticApplicationReply();
   // Do not cut a Direct Connect-IQ transfer in the middle.  v9 could leave the
   // watch protocol half-open and the next GCM send then failed with
   // FAILURE_DURING_TRANSFER.  Drain the finite Direct queue first.
   stopDirectGarminGracefully(() -> {
      if(token!=reinitGeneration) return;
      unregister();
      devices=null;
      mListener=null;
      for(GarminPeer gp:garminPeers.values()) {
         gp.connected=false;
         gp.glucoseReady=false;
         gp.inflightGlucose=null;
         gp.watchStopped=false;
         gp.startPending=false;
         gp.timestampedGlucoseAck=false;
         gp.numberRoleCapable=false;
         gp.requestedNumberRole=-1;
         gp.confirmedNumberRole=-1;
         gp.setupRecoveryUsed=false;
         gp.sdkRetryPending=false;
         ++gp.sdkRetryGeneration;
         gp.applicationSession++;
         gp.lastError=null;
         gp.applicationStateTime=System.currentTimeMillis();
         gp.startGeneration++;
         gp.applicationBootstrapPending=false;
         gp.applicationBootstrapGeneration++;
         gp.reinitRecoveryPending=false;
         ++gp.reinitRecoveryGeneration;
         }
      synchronized(AllData.this) {
         automaticRetryMessage=null; automaticRetryPeerId=NO_PEER; directRetryMessages.clear();
         }
      clearsyncgegs();
      numbersSyncPending=numbersEnabled;
      transportRestarting=false;
      // Recreate the selected transport. Only a user Reinit has armed a one-shot
      // openApplication request; ordinary transport restarts do not launch Kerfstok.
      initIQ(app);
      });
   }
}
