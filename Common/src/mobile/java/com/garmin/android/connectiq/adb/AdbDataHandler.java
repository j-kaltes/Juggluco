package com.garmin.android.connectiq.adb;

import android.content.Context;
import android.content.Intent;
import com.garmin.android.connectiq.ConnectIQ;
import com.garmin.android.connectiq.IQApp;
import com.garmin.android.connectiq.IQDevice;

/* loaded from: classes.jar:com/garmin/android/connectiq/adb/AdbDataHandler.class */
public class AdbDataHandler {
    public static void handleMessage(Context context, byte[] data) {
        Intent broadcast = new Intent(ConnectIQ.INCOMING_MESSAGE);
        broadcast.putExtra(ConnectIQ.EXTRA_REMOTE_DEVICE, new IQDevice(12345L, "Simulator"));
        broadcast.putExtra(ConnectIQ.EXTRA_REMOTE_APPLICATION, new IQApp("", "Simulator App", 1));
        broadcast.putExtra(ConnectIQ.EXTRA_PAYLOAD, data);
        context.sendBroadcast(broadcast);
    }
}
