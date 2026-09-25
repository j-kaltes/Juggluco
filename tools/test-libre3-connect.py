#!/usr/bin/env python3
"""Exercise production connect/close/deadline methods with a deterministic fake BLE stack.
No Android services, glucose algorithms or sensor credentials are used.
"""
from pathlib import Path
import subprocess, tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'Common/src/main/java/tk/glucodata/SuperGattCallback.java').read_text()
libre = (root / 'Common/src/libre3/java/tk/glucodata/Libre3GattCallback.java').read_text()
ng = 'closeGattTransport()' in source
assert 'watchConnectionAttempt(bluetoothGatt);\n        bluetoothGatt.connect();' in libre, 'existing-GATT reconnect must rearm the deadline'

def method(text, signature):
    start = text.index(signature)
    brace = text.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        if text[end] == '{': depth += 1
        elif text[end] == '}': depth -= 1
        end += 1
    return text[start:end]

methods = [method(source, x) for x in [
    'private void connectionAttemptExpired(', 'protected final void watchConnectionAttempt(', 'private Runnable getConnectDevice(',
    'public synchronized boolean connectDevice(' if ng else 'public boolean connectDevice(',
    'public final synchronized void closeGattTransport(' if ng else 'public void close()',
    'protected final synchronized boolean acceptConnectionAttemptCallback(' if ng else 'protected final boolean acceptConnectionStateChange(',
]]
methods.append(method(libre, 'protected long connectionAttemptTimeoutMillis('))
if ng:
    methods += [method(source,x) for x in ['private synchronized void markConnectRunnableStarted(', 'private synchronized void clearPendingConnect(', 'public synchronized void setPause(']]
    methods += ['public void close() { closeGattTransport(); }']

preamble = r'''
package tk.glucodata;
import java.util.*;
import java.util.concurrent.*;
class SuperGattCallback {
    static boolean doLog=false, isWearable=false, autoconnect=false;
    static String LOG_ID="test";
    static final Object app=new Object();
    boolean stop=false, connectPending=false, ownership=false;
    long dataptr=1L, foundtime=0L, connectTime=0L, reconnectGeneration=0L;
    String SerialNumber="synthetic", mActiveDeviceAddress="synthetic", mDeviceName;
    final Object gattLock=new Object();
    BluetoothDevice mActiveBluetoothDevice=new BluetoothDevice();
    BluetoothGatt mBluetoothGatt, locallyConnectedGatt, reconnectWaitingGatt;
    ScheduledFuture<?> pendingConnectFuture;
    static SensorBluetooth blueone=new SensorBluetooth();
    static class SensorBluetooth { boolean enabled=true; boolean bluetoothIsEnabled(){return enabled;} void scanStarter(int n){} }
    static class CloneSensorRegistry { static boolean clone; static boolean isCloneSensor(String s){return clone;} }
    static class SensorOwnershipRuntime { static boolean blocked; static boolean blocksLocalConnection(String s){return blocked;} }
    static class WearSensorClaim { static void onLocalGattDisconnected(String s){} }
    static class Build { static class VERSION { static int SDK_INT=37; } static class VERSION_CODES { static int M=23; } }
    static class R { static class string { static int turn_on_nearby_devices_permission=0; } }
    static class Log { static void i(String a,String b){} static void d(String a,String b){} static void e(String a,String b){} static void stack(String a,String b,Throwable t){throw new AssertionError(b,t);} }
    static class BluetoothGatt { int closed,disconnected; void disconnect(){disconnected++;} void close(){closed++;} }
    static class BluetoothDevice {
        static int TRANSPORT_LE=2; int calls;
        String getName(){return "fake";} String getAddress(){return "fake";}
        BluetoothGatt connectGatt(Object app,boolean auto,SuperGattCallback cb,int... transport){calls++;return new BluetoothGatt();}
    }
    static class Applic {
        static final Object app=new Object(); static Scheduler scheduler;
        static Applic getContext(){return new Applic();} String getString(int id){return "";} static void Toaster(String s){}
    }
    static class Scheduler extends ScheduledThreadPoolExecutor {
        long now; List<Job> jobs=new ArrayList<>();
        Scheduler(){super(1);}
        public ScheduledFuture<?> schedule(Runnable r,long delay,TimeUnit unit){Job j=new Job(r,now+unit.toMillis(delay));jobs.add(j);return j;}
        void advance(long delta){long target=now+delta; while(true){Job j=jobs.stream().filter(x->!x.done&&x.at<=target).min(Comparator.comparingLong(x->x.at)).orElse(null);if(j==null)break;now=j.at;j.fire();}now=target;}
    }
    static class Job implements ScheduledFuture<Object> {
        Runnable task;long at;boolean done,cancelled;
        Job(Runnable r,long a){task=r;at=a;}
        void fire(){done=true;if(!cancelled)task.run();}
        public boolean cancel(boolean ignored){cancelled=true;done=true;return true;}
        public boolean isCancelled(){return cancelled;}public boolean isDone(){return done;}
        public Object get(){return null;}public Object get(long t,TimeUnit u){return null;}
        public long getDelay(TimeUnit u){return 0;}public int compareTo(Delayed d){return 0;}
    }
    boolean useAutoConnect(){return autoconnect;}
    boolean allowConnectWithoutDataptr(){return false;}
    void armConnectCallbackLatency(){} void setpriority(BluetoothGatt g){} void setGattOptions(BluetoothGatt g){}
'''
field_start=source.index('    private final GattConnectDeadline<BluetoothGatt> connectDeadline')
field_end=source.index('}, this::connectionAttemptExpired);', field_start)+len('}, this::connectionAttemptExpired);')
body=preamble+source[field_start:field_end]+'\n'+'\n'.join(methods)
accept='acceptConnectionAttemptCallback' if ng else 'acceptConnectionStateChange'
body += f'\n boolean result(BluetoothGatt g,int state) {{ return {accept}(g,state); }}\n'
body += r'''
    static void check(boolean b,String message){if(!b)throw new AssertionError(message);}
    static SuperGattCallback fresh(){Applic.scheduler=new Scheduler();CloneSensorRegistry.clone=false;SensorOwnershipRuntime.blocked=false;SuperGattCallback cb=new SuperGattCallback();cb.connectDevice(0);Applic.scheduler.advance(0);check(cb.mActiveBluetoothDevice.calls==1,"initial connect");return cb;}
    public static void main(String[] args){
        SuperGattCallback cb=fresh();BluetoothGatt old=cb.mBluetoothGatt;
        Applic.scheduler.advance(119999);check(cb.mActiveBluetoothDevice.calls==1,"deadline too early");
        Applic.scheduler.advance(1);check(cb.mActiveBluetoothDevice.calls==2&&old.closed==1,"silent attempt must close and retry before first glucose");
        BluetoothGatt current=cb.mBluetoothGatt;
        check(!cb.result(old,0),"retired disconnected callback accepted");
        check(!cb.result(old,2),"retired connected callback accepted");
        check(cb.mBluetoothGatt==current,"retired callback replaced current GATT");
        cb.result(current,2);Applic.scheduler.advance(360000);check(cb.mActiveBluetoothDevice.calls==2,"healthy connection retried");
        cb=fresh();old=cb.mBluetoothGatt;cb.result(old,1);Applic.scheduler.advance(120000);check(cb.mActiveBluetoothDevice.calls==2,"intermediate state cancelled deadline");
        cb=fresh();old=cb.mBluetoothGatt;cb.result(old,0);Applic.scheduler.advance(120000);check(cb.mActiveBluetoothDevice.calls==1,"normal disconnect raced deadline retry");
        cb=fresh();old=cb.mBluetoothGatt;cb.result(old,0);cb.watchConnectionAttempt(old);Applic.scheduler.advance(120000);check(cb.mActiveBluetoothDevice.calls==2,"existing-GATT reconnect missing deadline");
        cb=fresh();old=cb.mBluetoothGatt;cb.result(old,0);cb.watchConnectionAttempt(old);cb.result(old,2);Applic.scheduler.advance(120000);check(cb.mActiveBluetoothDevice.calls==1,"successful existing-GATT reconnect retried");
        cb=fresh();old=cb.mBluetoothGatt;cb.close();Applic.scheduler.advance(120000);check(cb.mActiveBluetoothDevice.calls==1,"closed callback resurrected");
        cb=fresh();cb.stop=true;Applic.scheduler.advance(120000);check(cb.mActiveBluetoothDevice.calls==1,"stopped sensor retried");
        cb=fresh();cb.dataptr=0;Applic.scheduler.advance(120000);check(cb.mActiveBluetoothDevice.calls==1,"removed sensor retried");
        cb=fresh();cb.close();cb.connectDevice(0);cb.stop=true;Applic.scheduler.advance(0);check(cb.mActiveBluetoothDevice.calls==1,"queued connect ignored stop");
        cb=fresh();old=cb.mBluetoothGatt;Job retired=Applic.scheduler.jobs.get(1);cb.close();cb.connectDevice(0);Applic.scheduler.advance(0);current=cb.mBluetoothGatt;retired.task.run();check(cb.mBluetoothGatt==current&&cb.mActiveBluetoothDevice.calls==2,"dispatched cancelled timer disturbed replacement");
        Scheduler timer=new Scheduler();List<Object> expired=new ArrayList<>();Object same=new Object();GattConnectDeadline<Object> deadline=new GattConnectDeadline<>(new Object(),(r,d)->{Job j=(Job)timer.schedule(r,d,TimeUnit.MILLISECONDS);return ()->j.cancel(false);},expired::add);
        deadline.arm(same,120000);Job first=timer.jobs.get(0);deadline.arm(same,120000);first.task.run();check(expired.isEmpty(),"same-object rearm accepts stale ticket");timer.advance(120000);check(expired.size()==1,"replacement deadline must fire exactly once");
'''
if ng:
    body += r'''
        cb=fresh();cb.setPause(true);cb.setPause(false);Applic.scheduler.advance(120000);check(cb.mActiveBluetoothDevice.calls==1,"pause cancellation resurrected by resume");
        cb=fresh();CloneSensorRegistry.clone=true;Applic.scheduler.advance(120000);check(cb.mActiveBluetoothDevice.calls==1,"clone ownership ignored");
        cb=fresh();SensorOwnershipRuntime.blocked=true;Applic.scheduler.advance(120000);check(cb.mActiveBluetoothDevice.calls==1,"released ownership ignored");
'''
body += '\n System.out.println("PASS: production connect/deadline/close lifecycle, stale callbacks, cancellation and stop guards");\n}\n}\n'
with tempfile.TemporaryDirectory(prefix='libre3-connect-') as d:
    d=Path(d);(d/'SuperGattCallback.java').write_text(body)
    (d/'BluetoothProfile.java').write_text('package android.bluetooth; public class BluetoothProfile {public static final int STATE_CONNECTED=2,STATE_DISCONNECTED=0;}')
    subprocess.run(['javac','-d',str(d),str(d/'SuperGattCallback.java'),str(d/'BluetoothProfile.java'),str(root/'Common/src/main/java/tk/glucodata/GattConnectDeadline.java')],check=True)
    subprocess.run(['java','-cp',str(d),'tk.glucodata.SuperGattCallback'],check=True)
