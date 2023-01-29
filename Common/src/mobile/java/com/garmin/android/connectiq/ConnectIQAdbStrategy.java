package com.garmin.android.connectiq;

import android.content.Context;
import com.garmin.android.connectiq.ConnectIQ;
import com.garmin.android.connectiq.IQDevice;
import com.garmin.android.connectiq.adb.AdbConnection;
import com.garmin.android.connectiq.exception.InvalidStateException;
import com.garmin.android.connectiq.exception.ServiceUnavailableException;
import java.util.ArrayList;
import java.util.List;

/* loaded from: classes.jar:com/garmin/android/connectiq/ConnectIQAdbStrategy.class */
public class ConnectIQAdbStrategy extends ConnectIQ {
    private AdbConnection.AdbConnectionListener connListener = new AdbConnection.AdbConnectionListener() { // from class: com.garmin.android.connectiq.ConnectIQAdbStrategy.1
        @Override // com.garmin.android.connectiq.adb.AdbConnection.AdbConnectionListener
        public void onConnectionStatusChanged(IQDevice device, IQDevice.IQDeviceStatus newStatus) {
            ConnectIQAdbStrategy.this.deviceStatusChanged(device, newStatus);
        }
    };

    @Override // com.garmin.android.connectiq.ConnectIQ
    public void initialize(Context context, boolean autoUI, ConnectIQ.ConnectIQListener listener) {
        AdbConnection connection = AdbConnection.getInstance();
        connection.setContext(context);
        if (!connection.isActive()) {
            connection.establishConnection(this.connListener);
        }
        super.initialize(context, autoUI, listener);
        if (listener != null) {
            listener.onSdkReady();
        }
    }

    @Override // com.garmin.android.connectiq.ConnectIQ
    public List<IQDevice> getConnectedDevices() throws InvalidStateException {
        verifyInitialized();
        if (!AdbConnection.getInstance().isConnected()) {
            return new ArrayList<>();
        }
        List<IQDevice> devices = new ArrayList<>(1);
        devices.add(new IQDevice(12345L, "Simulator"));
        return devices;
    }

    @Override // com.garmin.android.connectiq.ConnectIQ
    public List<IQDevice> getKnownDevices() throws InvalidStateException {
        verifyInitialized();
        List<IQDevice> devices = new ArrayList<>(1);
        devices.add(new IQDevice(12345L, "Simulator"));
        return devices;
    }

    @Override // com.garmin.android.connectiq.ConnectIQ
    public IQDevice.IQDeviceStatus getDeviceStatus(IQDevice device) throws InvalidStateException, ServiceUnavailableException {
        IQDevice.IQDeviceStatus status;
        verifyInitialized();
        IQDevice.IQDeviceStatus iQDeviceStatus = IQDevice.IQDeviceStatus.UNKNOWN;
        if (AdbConnection.getInstance().isConnected()) {
            status = IQDevice.IQDeviceStatus.CONNECTED;
        } else {
            status = IQDevice.IQDeviceStatus.NOT_CONNECTED;
        }
        return status;
    }

    @Override // com.garmin.android.connectiq.ConnectIQ
    public void getApplicationInfo(String applicationId, IQDevice device, ConnectIQ.IQApplicationInfoListener listener) throws InvalidStateException, ServiceUnavailableException {
        verifyInitialized();
        if (!AdbConnection.getInstance().isConnected()) {
            throw new InvalidStateException("SDK not initialized. Did you forget to call ConnectIQ::initialize()?");
        } else if (null != device || null == listener) {
            listener.onApplicationInfoReceived(new IQApp(applicationId));
        } else {
            listener.onApplicationNotInstalled(applicationId);
        }
    }

    @Override // com.garmin.android.connectiq.ConnectIQ
    public void openApplication(IQDevice device, IQApp app, ConnectIQ.IQOpenApplicationListener listener) throws InvalidStateException {
        verifyInitialized();
        if (!AdbConnection.getInstance().isConnected()) {
            throw new InvalidStateException("SDK not initialized. Did you forget to call ConnectIQ::initialize()?");
        } else if (device == null || listener == null) {
            listener.onOpenApplicationResponse(device, app, ConnectIQ.IQOpenApplicationStatus.PROMPT_NOT_SHOWN_ON_DEVICE);
        } else {
            listener.onOpenApplicationResponse(device, app, ConnectIQ.IQOpenApplicationStatus.APP_IS_NOT_INSTALLED);
        }
    }

    @Override // com.garmin.android.connectiq.ConnectIQ
    protected void sendMessageProtocol(IQDevice device, IQApp application, byte[] data, ConnectIQ.IQSendMessageListener listener) {
        if (AdbConnection.getInstance().sendMessage(data) && listener != null) {
            listener.onMessageStatus(device, application, ConnectIQ.IQMessageStatus.SUCCESS);
        }
        if (listener != null) {
            listener.onMessageStatus(device, application, ConnectIQ.IQMessageStatus.FAILURE_UNKNOWN);
        }
    }

    @Override // com.garmin.android.connectiq.ConnectIQ
    protected void sendImageProtocol(IQDevice device, IQApp application, byte[] imageData, ConnectIQ.IQSendImageListener listener) {
        throw new UnsupportedOperationException();
    }

    @Override // com.garmin.android.connectiq.ConnectIQ
    protected void registerForRemoteAppEvents(IQDevice device, IQApp app) {
    }
}
