/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2(+), Libre 3(+), Dexcom G7/ONE+,        */
/*      Sibionics GS1Sb and GS3, Accu-Chek SmartGuide, CareSens Air and              */
/*      Aidex X sensors.                                                             */
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
/*      Thu Oct 15 12:44:39 CEST 2026                                                */

package tk.glucodata;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Handler;
import android.os.Looper;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import tk.glucodata.nums.AllData;

/** Durable, address-specific sensor lifecycle commands. Work runs on AllData's
 * existing main looper. UI finish actions apply local settings before returning;
 * background notifications are posted, never processed in a GATT callback. */
public final class GarminLibre3Lifecycle {
    public static final int COMMAND=240, ACK=241;
    private static final int STOP=0, ADD=1;
    private static final long NO_PEER=Long.MIN_VALUE, RETRY_MS=30000L;
    private static final String TAG="GarminL3Lifecycle";
    private static volatile AllData owner;
    private static volatile Handler handler;
    private static final Runnable PUMP=GarminLibre3Lifecycle::pump;
    private GarminLibre3Lifecycle() {}

    private static SharedPreferences prefs() {
        return Applic.app.getSharedPreferences("garmin_libre3_lifecycle",Context.MODE_PRIVATE);
    }

    public static void install(AllData all,Handler transportHandler) {
        Handler previous=handler;
        if(previous!=null) previous.removeCallbacks(PUMP);
        owner=all;
        handler=transportHandler;
        SensorLifecycle.listen(new SensorLifecycle.Listener() {
            public void changed() { kick(); }
            public void ended() { sensorEnded(); }
        });
    }

    private static void sensorEnded() {
        Handler h=handler;
        if(h!=null && Looper.myLooper()==h.getLooper()) {
            // confirmFinish opens bluediag immediately after sensorEnded().
            // Run now so it reads the updated Use Bluetooth / Direct settings.
            h.removeCallbacks(PUMP);
            pump();
        } else kick();
    }

    public static void kick() {
        // Keep other notifications asynchronous, including those caused by
        // enabling Bluetooth and clearing Direct inside the finish action.
        Handler h=handler;
        if(h!=null) { h.removeCallbacks(PUMP); h.post(PUMP); }
    }

    /** START capability 4 appends the actual watch state and sensor address:
     * [START, ..., capabilities, numbersRole, state, addressHigh, addressLow].
     * State bits: Direct selected=1, credentials present=2, BLE session=4.
     * A transport reconnect alone cannot repair a lost Direct setting. */
    public static void onWatchStart(long peer,List<?> message) {
        int state=-1, first=0, last=0;
        if(message.size()>2 && message.get(2) instanceof Number &&
                (((Number)message.get(2)).intValue()&4)!=0) {
            if(message.size()!=7) { kick(); return; }
            for(int i=4;i<7;i++) if(!(message.get(i) instanceof Number)) { kick(); return; }
            state=((Number)message.get(4)).intValue();
            first=((Number)message.get(5)).intValue();
            last=((Number)message.get(6)).intValue();
            if(state<0 || state>7 || (last&65535)!=0) { kick(); return; }
        }
        final int reported=state, high=first, low=last;
        Handler h=handler;
        if(h!=null) h.post(() -> reconcileWatch(peer,reported,high,low));
    }

    private static void reconcileWatch(long peer,int state,int first,int last) {
        AllData all=owner;
        if(all==null || Applic.app==null || Applic.app.numdata!=all ||
                !all.libre3LifecycleReady(peer) || !all.isGarminActive(peer)) return;
        try {
            migrate(all);
            SharedPreferences p=prefs();
            // Never override an explicit OFF, sensor end, a different owner,
            // or a command already awaiting its application acknowledgement.
            if(all.getLibre3DirectPeerId()!=peer || p.getLong("owner",NO_PEER)!=peer ||
                    p.getInt("pending_token",0)!=0) { kick(); return; }
            String serial=p.getString("serial",null), addr=p.getString("address",null);
            int[] expected=words(addr);
            if(!active(serial) || expected==null) { kick(); return; }
            if(state==7 && first==expected[0] && last==expected[1]) { kick(); return; }
            // Legacy watches do not report state. Reassert the same address
            // through idempotent ADD; a live matching sensor is left connected.
            Log.i(TAG,"reconcile watch peer="+peer+" state="+state+" sensor="+serial);
            if(begin(ADD,peer,serial,addr,false)) sendPending(all);
        } catch(Throwable th) {
            Log.stack(TAG,"Reconcile watch ownership",th);
            retry();
        }
    }

    /** Manual handoffs remain explicit replacements. Keep history routing in
     * GarminLibre3's separate address map when this assignment is released. */
    public static void assigned(long peer,String serial,String address) {
        prefs().edit().putLong("owner",peer).putString("serial",serial)
                .putString("address",address).putBoolean("migrated",true).remove("pending_token").commit();
    }

    private static boolean active(String serial) {
        return GarminLibre3.isActiveLibre3Sensor(serial);
    }

    private static String address(String serial) {
        long ptr=Natives.getdataptr(serial);
        if(ptr==0L) return null;
        try { return Natives.getDeviceAddress(ptr,false); }
        finally { Natives.freedataptr(ptr); }
    }

    private static int[] words(String address) {
        if(address==null) return null;
        String[] parts=address.split(":");
        if(parts.length!=6) return null;
        int[] out=new int[2];
        try {
            for(int i=0;i<6;i++) {
                if(parts[i].length()!=2) return null;
                int b=Integer.parseInt(parts[i],16);
                if(b<0 || b>255) return null;
                out[i/4]|=b<<((3-i%4)*8);
            }
        } catch(NumberFormatException ex) { return null; }
        return out;
    }

    private static String address(int first,int last) {
        if((last&65535)!=0) return null;
        return String.format(Locale.US,"%02X:%02X:%02X:%02X:%02X:%02X",
                first>>>24,(first>>>16)&255,(first>>>8)&255,first&255,last>>>24,(last>>>16)&255);
    }

    private static int nextToken(SharedPreferences p,int floor) {
        // Monotonic across process restarts and a fresh phone installation.
        long token=Math.max(Math.max(p.getInt("sequence",0),floor),System.currentTimeMillis()/1000L)+1;
        if(token>Integer.MAX_VALUE) throw new IllegalStateException("Libre3 lifecycle sequence exhausted");
        return (int)token;
    }

    private static boolean begin(int op,long peer,String serial,String address,boolean ended) {
        if(words(address)==null) return false;
        SharedPreferences p=prefs();
        int token=nextToken(p,0);
        boolean saved=p.edit().putInt("sequence",token).putInt("pending_token",token)
                .putInt("pending_op",op).putLong("pending_peer",peer)
                .putString("pending_serial",serial).putString("pending_address",address)
                .putBoolean("pending_end",ended).putBoolean("pending_end_applied",false).commit();
        if(saved) Log.i(TAG,(op==STOP?"stop":"add")+" pending sensor="+serial+" peer="+peer+" token="+token);
        return saved;
    }

    /** Apply a sensor-end STOP's local policy once, without waiting for the
     * watch. Persisted work survives a crash during the local transition.
     * A later user choice must not be overwritten by retries. */
    private static boolean applyEndPolicy(AllData all) {
        SharedPreferences p=prefs();
        if(p.getInt("pending_op",ADD)!=STOP) return true;
        if(!p.contains("pending_end")) {
            // A STOP queued by v42 can be waiting when this version starts.
            String serial=p.getString("pending_serial",null);
            if(!p.edit().putBoolean("pending_end",serial!=null && !serial.isEmpty() && !active(serial)).commit())
                return false;
        }
        if(!p.getBoolean("pending_end",false) || p.getBoolean("pending_end_applied",false)) return true;
        long peer=p.getLong("pending_peer",NO_PEER);
        long selected=all.getLibre3DirectPeerId();
        // Stopping an old owner must not disturb another explicitly selected watch.
        // Also complete the local Bluetooth transition if Direct was already
        // cleared before an interruption. Both exits use the same ON policy.
        if(selected==peer || selected==NO_PEER)
            GarminLibre3.restorePhoneSensorBluetooth(Applic.app);
        if(selected==peer && !all.setGarminLibre3Direct(Applic.app,peer,false)) return false;
        return p.edit().putBoolean("pending_end_applied",true).commit();
    }

    private static void migrate(AllData all) {
        SharedPreferences p=prefs();
        if(p.contains("migrated")) return;
        SharedPreferences old=Applic.app.getSharedPreferences("garmin_libre3_handoff",Context.MODE_PRIVATE);
        String serial=old.getString("return_sensor_serial",null), address=old.getString("return_sensor_address",null);
        long peer=all.getLibre3DirectPeerId();
        if(peer==NO_PEER && old.getBoolean("sensor_bluetooth_suppressed",false)) return;
        SharedPreferences.Editor edit=p.edit().putBoolean("migrated",true);
        if(!p.contains("owner") && peer!=NO_PEER && serial!=null && address!=null &&
                old.getBoolean("sensor_bluetooth_suppressed",false))
            edit.putLong("owner",peer).putString("serial",serial).putString("address",address);
        edit.commit();
    }

    private static String candidate() {
        String last=SensorLifecycle.lastAdded();
        if(active(last)) return last;
        String[] sensors=Natives.activeSensors();
        String found=null;
        if(sensors!=null) for(String serial:sensors) if(active(serial)) {
            if(found!=null) return null; // Do not choose arbitrarily among sensors.
            found=serial;
        }
        return found;
    }

    private static void pump() {
        AllData all=owner;
        if(all==null || Applic.app==null || Applic.app.numdata!=all) return;
        try {
            SharedPreferences p=prefs();
            long savedPeer=p.getInt("pending_token",0)!=0 ? p.getLong("pending_peer",NO_PEER) : p.getLong("owner",NO_PEER);
            if(!all.libre3LifecycleReady(savedPeer)) return;
            migrate(all);
            int token=p.getInt("pending_token",0);
            long selected=all.getLibre3DirectPeerId();
            if(token!=0) {
                // End or deselection supersedes an ADD, even if its ACK was
                // lost. The higher STOP token also invalidates queued ADDs.
                if(p.getInt("pending_op",STOP)==ADD) {
                    boolean ended=!active(p.getString("pending_serial",null));
                    if((ended || selected!=p.getLong("pending_peer",NO_PEER)) &&
                            !begin(STOP,p.getLong("pending_peer",NO_PEER),p.getString("pending_serial",null),p.getString("pending_address",null),ended)) return;
                }
                sendPending(all);
                return;
            }
            long assigned=p.getLong("owner",NO_PEER);
            String serial=p.getString("serial",null);
            if(assigned!=NO_PEER) {
                // An unknown address reported BUSY by the watch is occupied
                // too. Only an explicit release/manual handoff may replace it.
                boolean ended=serial!=null && !serial.isEmpty() && !active(serial);
                if(ended || selected!=assigned) {
                    if(begin(STOP,assigned,serial,p.getString("address",null),ended)) sendPending(all);
                }
                return;
            }
            if(selected==NO_PEER) return;
            // V42 could already have acknowledged an ended sensor's STOP and
            // cleared its owner while leaving Direct selected. Finish that old
            // transition once; v43 operations always record pending_end.
            if(!p.contains("pending_end") && p.getInt("pending_op",ADD)==STOP &&
                    selected==p.getLong("pending_peer",NO_PEER)) {
                String previous=p.getString("pending_serial",null);
                if(previous!=null && !previous.isEmpty() && !active(previous)) {
                    if(begin(STOP,selected,previous,p.getString("pending_address",null),true)) sendPending(all);
                    return;
                }
            }
            serial=candidate();
            if(serial==null) return;
            String addr=address(serial);
            // Wait for normal phone authorization on a new sensor. Its saved
            // KAuth notification kicks us again; no Bluetooth object is kept.
            if(words(addr)==null || Libre3GattCallback.getSavedGarminProvisioningSecret(serial)==null) return;
            if(begin(ADD,selected,serial,addr,false)) sendPending(all);
        } catch(Throwable th) {
            Log.stack(TAG,"Sensor lifecycle",th);
            retry();
        }
    }

    private static void retry() {
        Handler h=handler;
        if(h!=null) { h.removeCallbacks(PUMP); h.postDelayed(PUMP,RETRY_MS); }
    }

    private static void sendPending(AllData all) {
        SharedPreferences p=prefs();
        int token=p.getInt("pending_token",0), op=p.getInt("pending_op",STOP);
        if(token==0) return;
        if(op==STOP && !applyEndPolicy(all)) { retry(); return; }
        long peer=p.getLong("pending_peer",NO_PEER);
        String serial=p.getString("pending_serial",null), addr=p.getString("pending_address",null);
        int[] a=words(addr);
        if(a==null) return;
        List<Integer> info=GarminLibre3.sensorInfo(token,op,serial,addr,p.getBoolean("pending_end",false));
        if(info!=null) all.sendLibre3Lifecycle(peer,info);
        ArrayList<Integer> message=new ArrayList<>();
        message.add(COMMAND);message.add(token);message.add(op);message.add(a[0]);message.add(a[1]);
        if(op==ADD) {
            byte[] secret=Libre3GattCallback.getSavedGarminProvisioningSecret(serial);
            if(secret==null) { retry(); return; }
            // The 180 secret bytes occupy 45 words instead of 180 array items.
            // Total request: 53 numbers; no nested modules or new watch timers.
            for(int i=0;i<180;i+=4) message.add((secret[i]&255)<<24 | (secret[i+1]&255)<<16 |
                    (secret[i+2]&255)<<8 | secret[i+3]&255);
            message.add(Natives.getunit()==1?1:0);
            List<Integer> seed=GarminLibre3.backfillSeed(serial,addr);
            message.add(seed==null?-1:seed.get(3));message.add(seed==null?0:seed.get(4));
            // A live return can precede the lifecycle ACK.
            GarminLibre3.rememberReturnSensor(Applic.app,serial,addr);
        }
        if(all.sendLibre3Lifecycle(peer,message)) Log.i(TAG,"sent op="+op+" token="+token+" peer="+peer);
        retry(); // Application acknowledgement, not send success, clears work.
    }

    /** Used by the Direct queue to remove superseded lifecycle work. */
    public static boolean current(long peer,Object message) {
        if(!(message instanceof List<?>)) return false;
        List<?> m=(List<?>)message;
        SharedPreferences p=prefs();
        return m.size()>1 && m.get(1) instanceof Number && p.getLong("pending_peer",NO_PEER)==peer &&
                p.getInt("pending_token",0)!=0 && p.getInt("pending_token",0)==((Number)m.get(1)).intValue();
    }

    public static void receive(long peer,List<?> incoming) {
        if(incoming.size()!=8) return;
        int[] m=new int[8];
        for(int i=0;i<8;i++) { if(!(incoming.get(i) instanceof Number)) return; m[i]=((Number)incoming.get(i)).intValue(); }
        Handler h=handler;
        if(h!=null) h.post(() -> acknowledged(peer,m));
    }

    private static void acknowledged(long peer,int[] m) {
        SharedPreferences p=prefs();
        int token=p.getInt("pending_token",0);
        String addr=p.getString("pending_address",null), serial=p.getString("pending_serial",null);
        int[] expected=words(addr);
        if(token==0 || m[0]!=ACK || m[1]!=token || p.getLong("pending_peer",NO_PEER)!=peer ||
                expected==null || m[3]!=expected[0] || m[4]!=expected[1]) return;
        int op=p.getInt("pending_op",STOP);
        if(op==STOP && !applyEndPolicy(owner)) { retry(); return; }
        // Recheck the native lifecycle before accepting an ADD reply that raced
        // finishSensor / useAgain / the Libre3 Direct checkbox.
        if(op==ADD && (!active(serial) || !owner.libre3LifecycleReady(peer) || owner.getLibre3DirectPeerId()!=peer)) { kick(); return; }
        if(m[2]==2) { // The watch retained a newer sequence across a phone reinstall.
            int next=nextToken(p,m[7]);
            if(p.edit().putInt("sequence",next).putInt("pending_token",next).commit()) kick();
            return;
        }
        if(m[2]==0) {
            if(op==ADD) {
                assigned(peer,serial,addr);
                GarminLibre3.suppressPhoneSensorBluetooth(Applic.app);
            } else {
                p.edit().remove("owner").remove("serial").remove("address").remove("pending_token").commit();
                long selected=owner.getLibre3DirectPeerId();
                if(selected==NO_PEER || selected==peer)
                    GarminLibre3.releasePhoneSensorBluetoothIfSuppressed(Applic.app);
            }
            Log.i(TAG,(op==STOP?"stop":"add")+" acknowledged sensor="+serial+" peer="+peer+" token="+token);
            kick();
        } else if(m[2]==1 && op==ADD) {
            String occupied=address(m[5],m[6]);
            if(occupied==null) return;
            String occupiedSerial=GarminLibre3.returnSensorSerial(occupied);
            assigned(peer,occupiedSerial==null?"":occupiedSerial,occupied);
            Log.i(TAG,"watch already owns sensor="+occupied+"; automatic add skipped");
            kick();
        } else Log.e(TAG,"watch lifecycle rejected token="+token+" status="+m[2]);
    }
}
