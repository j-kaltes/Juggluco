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

/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2 and 3 sensors.                         */

package tk.glucodata;

import android.content.Context;

import java.util.Arrays;
import java.util.List;
import java.util.Locale;

import tk.glucodata.nums.AllData;

import static consts.consts.GLUNITS;
import static consts.consts.LIBRE3DIRECT;

/**
 * Provisioning/ownership handoff for the large Kerfstok+Libre3 Garmin app.
 *
 * The important rule is that installation/running state of the large app does not
 * grant sensor ownership.  GarminStatus selects one watch with "Libre 3 direct";
 * only that peer receives provisioning and DIRECT=1.  Other watches may run the
 * same app as ordinary Kerfstok without touching a Libre3 sensor.
 */
public final class GarminLibre3 {
    private static final String LOG_ID = "GarminLibre3";
    private static final String PREFS = "garmin_libre3_handoff";
    // V44 and earlier saved the pre-handoff setting. Only remove that old key;
    // leaving Direct now always enables phone sensor Bluetooth.
    private static final String LEGACY_RESTORE_BLUETOOTH = "restore_sensor_bluetooth";
    private static final String PREF_BLUETOOTH_SUPPRESSED = "sensor_bluetooth_suppressed";
    private static final String PREF_RETURN_SERIAL = "return_sensor_serial";
    private static final String PREF_RETURN_ADDRESS = "return_sensor_address";
    // Watch -> phone direct-Libre3 minute protocol. Kept local here/AllData so
    // the existing generated Kerfstok constants do not need renumbering.
    public static final int LIBRE3_MINUTE = 235;
    public static final int GOT_LIBRE3_MINUTE = 236;
    public static final int LIBRE3_CLINICAL = 237;
    public static final int GOT_LIBRE3_CLINICAL = 238;
    public static final int LIBRE3_BACKFILL_SEED = 239;
    public static final int LIBRE3_SENSOR_INFO = 242;
    private static volatile String returnSerial;
    private static volatile String returnAddress;
    private static volatile Transfer activeTransfer;

    private GarminLibre3() {}

    /** Independent optional metadata, leaving credential/STOP/ADD compatible
     * with older watches. SuperGattCallback.dowithglucose() passes SerialNumber
     * unchanged to AllData.sendglucose(); use exactly that identifier here too. */
    static List<Integer> sensorInfo(int token,int op,String serial,String address,boolean ended) {
        if(serial==null || serial.isEmpty() || address==null) return null;
        String[] parts=address.split(":");
        if(parts.length!=6) return null;
        int first=0,last=0;
        try {
            for(int i=0;i<6;i++) {
                int b=Integer.parseInt(parts[i],16);
                if(b<0 || b>255) return null;
                if(i<4) first|=b<<((3-i)*8); else last|=b<<((7-i)*8);
            }
        } catch(NumberFormatException e) { return null; }
        if(serial.length()>32) return null;
        String shown=serial;
        java.util.ArrayList<Integer> info=new java.util.ArrayList<>(6+shown.length());
        info.add(LIBRE3_SENSOR_INFO);info.add(token);info.add(op);
        info.add(first);info.add(last);info.add(ended?1:0);
        for(int i=0;i<shown.length();i++) {
            char c=shown.charAt(i);
            if(c<33 || c>126) return null;
            info.add((int)c);
        }
        return info;
    }

    /** Replay display identity after a watch restart/update, without reconnecting
     * the sensor or relying on a phone GATT callback. */
    public static void sendSensorInfo(long peerId) {
        if(Applic.app==null || Applic.app.numdata==null ||
                Applic.app.numdata.getLibre3DirectPeerId()!=peerId) return;
        var p=Applic.app.getSharedPreferences(PREFS,Context.MODE_PRIVATE);
        List<Integer> info=sensorInfo(0,1,p.getString(PREF_RETURN_SERIAL,null),
                p.getString(PREF_RETURN_ADDRESS,null),false);
        if(info!=null) Applic.app.numdata.sendLibre3Message(Applic.app,peerId,info);
    }

    /** Disable Juggluco's sensor Bluetooth only after the Garmin handoff has
     * succeeded. Keep the ownership marker for release/restart handling. */
    static synchronized void suppressPhoneSensorBluetooth(Context context) {
        if (context == null) return;
        final Context app = context.getApplicationContext();
        final android.content.SharedPreferences prefs =
                app.getSharedPreferences(PREFS, Context.MODE_PRIVATE);
        prefs.edit().remove(LEGACY_RESTORE_BLUETOOTH)
                .putBoolean(PREF_BLUETOOTH_SUPPRESSED, true).apply();
        if (Natives.getusebluetooth()) {
            Log.i(LOG_ID, "Garmin direct owns sensor; disable sensor BLE but keep Garmin transport alive");
            // Do not use Applic.setbluetooth(false) here.  That calls
            // Applic.dontusebluetooth(), which stops keeprunning when there is
            // no mirror/backup host.  A mirror receiver naturally keeps that
            // service alive through backuphostNr(); Garmin Direct has no such
            // host, but still needs Juggluco's DirectGarminIQ/phone transport
            // alive to receive glucose returned by the watch.
            Natives.setusebluetooth(false);
            SensorBluetooth.destructor();
            Applic.app.redraw();
        }
        // A new lifecycle handoff may start with sensor Bluetooth already off.
        Applic.updateservice(app, true);
    }

    /** Explicitly leaving Direct (including sensor end)
     * always enables phone sensor Bluetooth, even if no handoff completed. */
    public static synchronized void restorePhoneSensorBluetooth(Context context) {
        if (context == null) return;
        final Context app = context.getApplicationContext();
        app.getSharedPreferences(PREFS, Context.MODE_PRIVATE).edit()
                .remove(LEGACY_RESTORE_BLUETOOTH).remove(PREF_BLUETOOTH_SUPPRESSED).apply();
        if (!Natives.getusebluetooth()) {
            Log.i(LOG_ID, "Garmin direct released sensor; enable Juggluco sensor Bluetooth");
            Applic.setbluetooth(app, true);
        }
        // The watch can still owe its STOP ACK and collected history.
        Applic.updateservice(app, true);
        Applic.app.redraw();
    }

    /** A delayed STOP ACK is not a new user action. Release only an outstanding
     * suppression; an explicit Direct OFF already removed it, and the user may
     * have changed Use Bluetooth again while the watch reply was pending. */
    static synchronized void releasePhoneSensorBluetoothIfSuppressed(Context context) {
        if (context == null) return;
        final Context app = context.getApplicationContext();
        if (app.getSharedPreferences(PREFS, Context.MODE_PRIVATE)
                .getBoolean(PREF_BLUETOOTH_SUPPRESSED, false))
            restorePhoneSensorBluetooth(app);
    }

    /** Compatibility hook for existing callers. Callback presence is only a
     * hint to refresh; the native active-sensor list determines availability. */
    public static void setSensorAvailable(boolean available) {
        SensorLifecycle.changed();
    }

    public static boolean sensorAvailable() {
        return findProvisionableSensor(false) != null;
    }

    /**
     * UI entry point: hand the currently active/provisionable Libre 3 sensor to
     * the watch selected with "Libre 3 direct", including while phone sensor
     * Bluetooth is off and there are no GATT callbacks.
     */
    public static synchronized boolean switchCurrentSensor() {
        final ProvisionableSensor sensor = findProvisionableSensor(true);
        if (sensor == null) return false;
        return switchSensor(sensor.serial, sensor.address, sensor.secret);
    }

    /** Own only Java copies; no callback, streamdata* or SensorGlucoseData* is
     * retained after reading the native store. */
    private static final class ProvisionableSensor {
        final String serial, address;
        final byte[] secret;
        ProvisionableSensor(String serial, String address, byte[] secret) {
            this.serial = serial;
            this.address = address;
            this.secret = secret;
        }
    }

    /** activeSensor() alone consults cached usedsensors in g.cpp. Refresh that
     * list with activeSensors() before checking an end/add or manual handoff. */
    static boolean isActiveLibre3Sensor(String serial) {
        if (serial == null || serial.isEmpty()) return false;
        String[] active = Natives.activeSensors();
        if (active == null || !Arrays.asList(active).contains(serial)) return false;
        long dataptr = Natives.getdataptr(serial);
        if (dataptr == 0L) return false;
        try {
            long sensorptr = Natives.getsensorptr(dataptr);
            return sensorptr != 0L && Natives.getLibreVersion(dataptr) == 3 && Natives.activeSensor(sensorptr);
        } finally { Natives.freedataptr(dataptr); }
    }

    /** The caller has just refreshed the native active list. Saved KAuth is
     * expanded in a temporary security context by the existing static helper. */
    private static ProvisionableSensor savedProvisioning(String serial) {
        if (serial == null || serial.isEmpty()) return null;
        long dataptr = Natives.getdataptr(serial);
        if (dataptr == 0L) return null;
        try {
            long sensorptr = Natives.getsensorptr(dataptr);
            if (sensorptr == 0L || Natives.getLibreVersion(dataptr) != 3 || !Natives.activeSensor(sensorptr))
                return null;
            String address = Natives.getDeviceAddress(dataptr, false);
            if (address == null || address.isEmpty()) return null;
            byte[] secret = Libre3GattCallback.getSavedGarminProvisioningSecret(serial);
            if (secret == null || secret.length != 180) return null;
            return new ProvisionableSensor(serial, address, secret);
        } finally { Natives.freedataptr(dataptr); }
    }

    /** Return the only provisionable active Libre3 sensor. The Garmin app can
     * store one sensor record; never choose arbitrarily between two of them. */
    private static ProvisionableSensor findProvisionableSensor(boolean reportError) {
        try {
            final String[] sensors = Natives.activeSensors();
            if (sensors == null || sensors.length == 0) {
                if (reportError) Applic.Toaster(R.string.garmin_libre3_no_active_sensor);
                return null;
            }

            ProvisionableSensor found = null;
            for (String serial : sensors) {
                final ProvisionableSensor sensor = savedProvisioning(serial);
                if (sensor == null) continue;
                if (found != null && !found.serial.equals(sensor.serial)) {
                    if (reportError) Applic.Toaster(R.string.garmin_libre3_multiple_sensors);
                    return null;
                }
                found = sensor;
            }

            if (found == null && reportError)
                Applic.Toaster(R.string.garmin_libre3_not_authorized);
            return found;
        } catch (Throwable th) {
            Log.stack(LOG_ID, "Finding active Libre 3 sensor", th);
            if (reportError) Applic.Toaster(R.string.garmin_libre3_find_failed);
            return null;
        }
    }

    private static String garminAddress(List<?> message) {
        if(message==null || message.size()!=23) return null;
        StringBuilder out=new StringBuilder(17);
        for(int i=0;i<6;i++) {
            Object obj=message.get(2+i);
            if(!(obj instanceof Number)) return null;
            int value=((Number)obj).intValue();
            if(value<0 || value>255) return null;
            if(i!=0) out.append(':');
            if(value<16) out.append('0');
            out.append(Integer.toHexString(value).toUpperCase(Locale.US));
        }
        return out.toString();
    }

    /** Resolve a sensor from Juggluco's persistent native sensor list, without
     * depending on SensorBluetooth or a live GATT callback. */
    static String sensorSerialForAddress(String address) {
        if(address==null) return null;
        try {
            String[] sensors=Natives.activeSensors();
            if(sensors==null) return null;
            for(String serial:sensors) {
                if(serial==null) continue;
                long dataptr=0L;
                try {
                    dataptr=Natives.getdataptr(serial);
                    if(dataptr==0L) continue;
                    String sensorAddress=Natives.getDeviceAddress(dataptr,false);
                    if(sensorAddress!=null && sensorAddress.equalsIgnoreCase(address))
                        return serial;
                } finally {
                    if(dataptr!=0L) Natives.freedataptr(dataptr);
                }
            }
        } catch(Throwable th) {
            Log.stack(LOG_ID,"Finding Garmin-return Libre3 sensor",th);
        }
        return null;
    }

    static void rememberReturnSensor(Context context,String serial,String address) {
        if(context==null || serial==null || address==null) return;
        var prefs=context.getApplicationContext().getSharedPreferences(PREFS,Context.MODE_PRIVATE);
        var edit=prefs.edit();
        String oldAddress=prefs.getString(PREF_RETURN_ADDRESS,null);
        String oldSerial=prefs.getString(PREF_RETURN_SERIAL,null);
        if(oldAddress!=null && oldSerial!=null)
            edit.putString("return_sensor:"+oldAddress.toUpperCase(Locale.US),oldSerial);
        returnSerial=serial;
        returnAddress=address;
        edit.putString("return_sensor:"+address.toUpperCase(Locale.US),serial)
                .putString(PREF_RETURN_SERIAL,serial)
                .putString(PREF_RETURN_ADDRESS,address).apply();
    }

    /** Return the serial number that owns this Garmin-return address. The value
     * survives destruction of SensorBluetooth and process restarts. */
    static String returnSensorSerial(String address) {
        String serial=returnSerial;
        String knownAddress=returnAddress;
        if(serial!=null && knownAddress!=null && knownAddress.equalsIgnoreCase(address))
            return serial;
        try {
            Context context=Applic.app==null?null:Applic.app.getApplicationContext();
            if(context!=null) {
                android.content.SharedPreferences prefs=context.getSharedPreferences(PREFS,Context.MODE_PRIVATE);
                String mapped=prefs.getString("return_sensor:"+address.toUpperCase(Locale.US),null);
                if(mapped!=null) return mapped;
                serial=prefs.getString(PREF_RETURN_SERIAL,null);
                knownAddress=prefs.getString(PREF_RETURN_ADDRESS,null);
                if(serial!=null && knownAddress!=null && knownAddress.equalsIgnoreCase(address)) {
                    returnSerial=serial;
                    returnAddress=knownAddress;
                    return serial;
                }
            }
        } catch(Throwable th) {
            Log.stack(LOG_ID,"Reading Garmin-return Libre3 sensor",th);
        }
        // Compatibility with a handoff made before the persisted return identity
        // existed: resolve it from the native sensor list, still without Bluetooth.
        serial=sensorSerialForAddress(address);
        if(serial!=null && Applic.app!=null)
            Applic.app.getSharedPreferences(PREFS,Context.MODE_PRIVATE).edit()
                    .putString("return_sensor:"+address.toUpperCase(Locale.US),serial).apply();
        return serial;
    }

    /** Compatibility for the address-less v21-v28 live return protocol. */
    private static String returnSensorSerial() {
        String serial=returnSerial;
        if(serial!=null) return serial;
        try {
            Context context=Applic.app==null?null:Applic.app.getApplicationContext();
            if(context!=null) {
                android.content.SharedPreferences prefs=context.getSharedPreferences(PREFS,Context.MODE_PRIVATE);
                serial=prefs.getString(PREF_RETURN_SERIAL,null);
                if(serial!=null) {
                    returnSerial=serial;
                    returnAddress=prefs.getString(PREF_RETURN_ADDRESS,null);
                    return serial;
                }
            }
        } catch(Throwable th) {
            Log.stack(LOG_ID,"Reading Garmin-return Libre3 sensor",th);
        }
        return null;
    }

    /** v29 adds two packed address words to each returned record. Message 235
     * carries the 15 live fields; 237 carries the complete 14-byte fastData.
     * Continue accepting the six-word live format from earlier watch versions. */
    public static synchronized boolean receiveMinuteFromGarmin(long peerId,List<?> message) {
        if(message==null || (message.size()!=6 && message.size()!=8) ||
                !(message.get(0) instanceof Number) || !(message.get(1) instanceof Number)) return false;
        int kind=((Number)message.get(0)).intValue();
        boolean clinical=kind==LIBRE3_CLINICAL;
        if(kind!=LIBRE3_MINUTE && !clinical) return false;
        if(clinical && message.size()!=8) return false;
        long seconds=((Number)message.get(1)).longValue();
        long now=System.currentTimeMillis()/1000L;
        if(seconds<1577836800L || seconds>now+300L) {
            Log.e(LOG_ID,"Garmin Libre3 minute has bad timestamp "+seconds);
            return false;
        }
        byte[] compact=new byte[clinical?14:15];
        try {
            int out=0;
            for(int wi=2;wi<6;wi++) {
                Object obj=message.get(wi);
                if(!(obj instanceof Number)) return false;
                int word=((Number)obj).intValue();
                int count=(wi==5)?(clinical?2:3):4;
                for(int bi=0;bi<count;bi++) compact[out++]=(byte)(word >>> ((3-bi)*8));
            }
        } catch(Throwable th) {
            Log.stack(LOG_ID,"Decoding compact Garmin Libre3 minute",th);
            return false;
        }
        byte[] decr=compact;
        if(!clinical) {
            decr=new byte[29];
            System.arraycopy(compact,0,decr,0,6);
            System.arraycopy(compact,6,decr,10,9);
        }
        String serial;
        if(message.size()==8) {
            if(!(message.get(6) instanceof Number) || !(message.get(7) instanceof Number)) return false;
            int a=((Number)message.get(6)).intValue(), b=((Number)message.get(7)).intValue();
            if((b&0xffff)!=0) return false;
            String address=String.format(java.util.Locale.US,"%02X:%02X:%02X:%02X:%02X:%02X",
                    (a>>>24)&255,(a>>>16)&255,(a>>>8)&255,a&255,(b>>>24)&255,(b>>>16)&255);
            serial=returnSensorSerial(address);
        } else serial=returnSensorSerial();
        if(serial==null) {
            Log.e(LOG_ID,"No persisted Libre3 sensor for Garmin-return peer="+peerId);
            return false; // no ACK: watch retains and retries this record
        }
        boolean saved=clinical ? Libre3GattCallback.saveGarminLibre3Clinical(serial,decr) :
                Libre3GattCallback.saveGarminLibre3Minute(serial,decr,seconds*1000L);
        if(saved) Log.i(LOG_ID,"Garmin Libre3 "+(clinical?"clinical":"minute")+" saved sensor="+serial+" time="+seconds);
        return saved;
    }

    public static List<?> returnAcknowledgement(List<?> message) {
        int kind=((Number)message.get(0)).intValue()==LIBRE3_CLINICAL ? GOT_LIBRE3_CLINICAL : GOT_LIBRE3_MINUTE;
        if(message.size()==6) return Arrays.asList(kind,message.get(1));
        int word=((Number)message.get(2)).intValue();
        int life=((word>>>24)&255)|((word>>>8)&0xff00);
        return Arrays.asList(kind,message.get(1),message.get(6),message.get(7),life);
    }

    static List<Integer> backfillSeed(String serial,String address) {
        long dataptr=0L;
        try {
            dataptr=Natives.getdataptr(serial);
            if(dataptr==0L) return null;
            long sensorptr=Natives.getsensorptr(dataptr);
            if(sensorptr==0L) return null;
            int from=Natives.getlastLifeCountReceived(sensorptr);
            int start=(int)(Natives.getSensorStartmsec(dataptr)/1000L);
            if(from<0 || from>65535 || start<=0) return null;
            String[] parts=address.split(":");
            if(parts.length!=6) return null;
            int first=0,last=0;
            for(int i=0;i<6;i++) {
                int b=Integer.parseInt(parts[i],16);
                if(b<0 || b>255) return null;
                if(i<4) first|=b<<((3-i)*8); else last|=b<<((7-i)*8);
            }
            return Arrays.asList(LIBRE3_BACKFILL_SEED,first,last,from,start);
        } finally { if(dataptr!=0L) Natives.freedataptr(dataptr); }
    }

    public static synchronized boolean switchSensor(final Libre3GattCallback sensor) {
        if (sensor == null) return false;
        // Keep this entry point for Libre3GattCallback.switchToGarmin(), but
        // read only its stable identity. Its native handles may have been freed.
        final String serial = sensor.SerialNumber;
        if (!isActiveLibre3Sensor(serial)) return false;
        final ProvisionableSensor saved = savedProvisioning(serial);
        return saved != null && switchSensor(saved.serial, saved.address, saved.secret);
    }

    /** Handoff from saved address/KAuth-derived provisioning; no live GATT is required. */
    public static synchronized boolean switchSensor(final String address, final byte[] secret180) {
        return switchSensor(sensorSerialForAddress(address), address, secret180);
    }

    private static boolean switchSensor(final String serial, final String address, final byte[] secret180) {
        if (activeTransfer != null) {
            Applic.Toaster("Already sending a Libre 3 sensor to Garmin");
            return false;
        }
        if (secret180 == null || secret180.length != 180 || address == null) {
            Applic.Toaster("Libre 3 authorization data is not available");
            return false;
        }
        if (serial == null) {
            Applic.Toaster("Libre 3 sensor identity is not available");
            Log.e(LOG_ID,"No sensor serial for Garmin handoff address="+address);
            return false;
        }
        if (!isActiveLibre3Sensor(serial)) {
            Applic.Toaster(R.string.garmin_libre3_no_active_sensor);
            return false;
        }

        final AllData all = Applic.app == null ? null : Applic.app.numdata;
        if (all == null) {
            Applic.Toaster("Garmin communication is not available");
            return false;
        }
        final long peerId = all.getLibre3DirectPeerId();
        if (peerId == Long.MIN_VALUE) {
            Applic.Toaster("Select Libre 3 direct for a Garmin watch first");
            return false;
        }
        Log.i(LOG_ID, "handoff start peer=" + peerId + " address=" + address);

        final List<Integer> message;
        try {
            message = GarminLibre3Provisioning.makeMessage(address, secret180);
        } catch (Throwable th) {
            Log.stack(LOG_ID, "Making Libre 3 Garmin provisioning data", th);
            Applic.Toaster("Can't make Libre 3 Garmin provisioning data");
            return false;
        }

        activeTransfer = new Transfer(peerId, serial, address, message);
        return activeTransfer.start();
    }

    private static synchronized void finished(Transfer transfer) {
        if (activeTransfer == transfer) activeTransfer = null;
    }

    private static final class Transfer {
        final long peerId;
        final String serial;
        final String address;
        final List<Integer> provisioning;
        final Context context;
        boolean done;

        Transfer(long peerId, String serial, String address, List<Integer> provisioning) {
            this.peerId = peerId;
            this.serial = serial;
            this.address = address;
            this.provisioning = provisioning;
            this.context = Applic.app.getApplicationContext();
        }

        boolean start() {
            try {
                AllData all = Applic.app.numdata;

                // Keep direct mode OFF while replacing provisioning.  The watch
                // stores L3G2 but does not start BLE until the final DIRECT=1.
                if (!all.sendLibre3Message(context, peerId, provisioning)) {
                    fail("Can't send Libre 3 sensor to the selected Garmin watch", null);
                    return false;
                }

                List<Integer> info=sensorInfo(0,1,serial,address,false);
                if(info!=null) all.sendLibre3Message(context,peerId,info);

                int unit = 0;
                try { unit = Natives.getunit() == 1 ? 1 : 0; }
                catch (Throwable th) { Log.stack(LOG_ID, "Reading glucose unit", th); }
                if (!all.sendLibre3Message(context, peerId, Arrays.asList(GLUNITS, unit))) {
                    fail("Can't send glucose unit to the selected Garmin watch", null);
                    return false;
                }

                List<Integer> seed=backfillSeed(serial,address);
                if(seed!=null && !all.sendLibre3Message(context,peerId,seed)) {
                    fail(context.getString(R.string.garmin_libre3_backfill_send_failed),null);
                    return false;
                }

                // DIRECT=1 is deliberately last: if the watch held credentials
                // for another sensor, it cannot race to connect to that old sensor.
                if (!all.sendLibre3Message(context, peerId, Arrays.asList(LIBRE3DIRECT, 1))) {
                    fail("Can't enable direct Libre 3 on the selected Garmin watch", null);
                    return false;
                }

                // Preserve the native sensor identity before disabling Bluetooth;
                // Applic.setbluetooth(false) destroys SensorBluetooth and its callbacks.
                rememberReturnSensor(context,serial,address);
                GarminLibre3Lifecycle.assigned(peerId,serial,address);
                suppressPhoneSensorBluetooth(context);
                succeed();
                return true;
            } catch (Throwable th) {
                fail("Sending Libre 3 sensor to Garmin failed", th);
                return false;
            }
        }

        void succeed() {
            if (done) return;
            done = true;
            Applic.Toaster("Libre 3 transferred to selected Garmin watch");
            finished(this);
        }

        void fail(String text, Throwable th) {
            if (done) return;
            done = true;
            if (th != null) Log.stack(LOG_ID, text, th);
            else Log.e(LOG_ID, text);
            Applic.Toaster(text);
            finished(this);
        }
    }
}
