package com.garmin.android.connectiq;

import com.garmin.android.connectiq.ConnectIQ;

/* loaded from: classes.jar:com/garmin/android/connectiq/ConnectIQStateManager.class */
public class ConnectIQStateManager {
    private static ConnectIQStateManager sInstance;
    private ConnectIQ.IQApplicationInfoListener appInfoListener;
    private ConnectIQ.IQOpenApplicationListener openApplicationListener;
    private ConnectIQ.IQSendMessageListener sendMessageListener;

    public static ConnectIQStateManager getInstance() {
        if (sInstance == null) {
            sInstance = new ConnectIQStateManager();
        }
        return sInstance;
    }

    private ConnectIQStateManager() {
    }

    public void setApplicationInfoListener(ConnectIQ.IQApplicationInfoListener listener) {
        this.appInfoListener = listener;
    }

    public ConnectIQ.IQApplicationInfoListener getApplicationInfoListener() {
        return this.appInfoListener;
    }

    public void setOpenApplicationListener(ConnectIQ.IQOpenApplicationListener listener) {
        this.openApplicationListener = listener;
    }

    public ConnectIQ.IQOpenApplicationListener getOpenApplicationListener() {
        return this.openApplicationListener;
    }

    public void setSendMessageListener(ConnectIQ.IQSendMessageListener listener) {
        this.sendMessageListener = listener;
    }

    public ConnectIQ.IQSendMessageListener getSendMessageListener() {
        return this.sendMessageListener;
    }
}
