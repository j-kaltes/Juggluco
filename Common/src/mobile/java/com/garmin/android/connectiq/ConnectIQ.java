package com.garmin.android.connectiq;

import android.app.AlertDialog;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.Uri;
import android.os.Handler;
import android.os.RemoteException;

import com.garmin.android.connectiq.IQDevice;
import com.garmin.android.connectiq.adb.AdbConnection;
import com.garmin.android.connectiq.exception.InvalidStateException;
import com.garmin.android.connectiq.exception.ServiceUnavailableException;
import com.garmin.monkeybrains.serialization.MonkeyType;
import com.garmin.monkeybrains.serialization.Serializer;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

/* loaded from: classes.jar:com/garmin/android/connectiq/ConnectIQ.class */
public abstract class ConnectIQ {
    public static final String INCOMING_MESSAGE = "com.garmin.android.connectiq.INCOMING_MESSAGE";
    public static final String DEVICE_STATUS = "com.garmin.android.connectiq.DEVICE_STATUS";
    public static final String APPLICATION_INFO = "com.garmin.android.connectiq.APPLICATION_INFO";
    public static final String OPEN_APPLICATION = "com.garmin.android.connectiq.OPEN_APPLICATION";
    public static final String SEND_MESSAGE_STATUS = "com.garmin.android.connectiq.SEND_MESSAGE_STATUS";
    public static final String EXTRA_REMOTE_DEVICE = "com.garmin.android.connectiq.EXTRA_REMOTE_DEVICE";
    public static final String EXTRA_REMOTE_APPLICATION = "com.garmin.android.connectiq.EXTRA_REMOTE_APPLICATION";
    public static final String EXTRA_PAYLOAD = "com.garmin.android.connectiq.EXTRA_PAYLOAD";
    public static final String EXTRA_STATUS = "com.garmin.android.connectiq.EXTRA_STATUS";
    public static final String EXTRA_APPLICATION_ID = "com.garmin.android.connectiq.EXTRA_APPLICATION_ID";
    public static final String EXTRA_APPLICATION_VERSION = "com.garmin.android.connectiq.EXTRA_APPLICATION_VERSION";
    public static final String EXTRA_OPEN_APPLICATION_DEVICE = "com.garmin.android.connectiq.EXTRA_OPEN_APPLICATION_DEVICE";
    public static final String EXTRA_OPEN_APPLICATION_ID = "com.garmin.android.connectiq.EXTRA_OPEN_APPLICATION_ID";
    public static final String EXTRA_OPEN_APPLICATION_RESULT_CODE = "com.garmin.android.connectiq.EXTRA_OPEN_APPLICATION_RESULT_CODE";
    protected static final int MIN_GCM_VERSION = 2000;
    protected static final String GCM_PACKAGE_NAME = "com.garmin.android.apps.connectmobile";
    protected static final String GCM_CONNECTIQ_SERVICE_ACTION = "com.garmin.android.apps.connectmobile.CONNECTIQ_SERVICE_ACTION";
    protected static final String GCM_CONNECTIQ_SERVICE_CLASS = "com.garmin.android.apps.connectmobile.connectiq.ConnectIQService";
    protected static ConnectIQ sInstance = null;
    protected static IQConnectType sConnectType = IQConnectType.WIRELESS;
    protected Context mContext;
    protected ConnectIQListener mListener;
    protected boolean mInitialized = false;
    private HashMap<String, DeviceRegistryEntry> mDeviceRegistry = new HashMap<>();
    private Handler mHandler;
    private IQMessageReceiver mMessageReceiver;

    /* loaded from: classes.jar:com/garmin/android/connectiq/ConnectIQ$ConnectIQListener.class */
    public interface ConnectIQListener {
        void onSdkReady();

        void onInitializeError(IQSdkErrorStatus iQSdkErrorStatus);

        void onSdkShutDown();
    }

    /* loaded from: classes.jar:com/garmin/android/connectiq/ConnectIQ$IQApplicationEventListener.class */
    public interface IQApplicationEventListener {
        void onMessageReceived(IQDevice iQDevice, IQApp iQApp, List<Object> list, IQMessageStatus iQMessageStatus);
    }

    /* loaded from: classes.jar:com/garmin/android/connectiq/ConnectIQ$IQApplicationInfoListener.class */
    public interface IQApplicationInfoListener {
        void onApplicationInfoReceived(IQApp iQApp);

        void onApplicationNotInstalled(String str);
    }

    /* loaded from: classes.jar:com/garmin/android/connectiq/ConnectIQ$IQConnectType.class */
    public enum IQConnectType {
        TETHERED,
        WIRELESS
    }

    /* loaded from: classes.jar:com/garmin/android/connectiq/ConnectIQ$IQDeviceEventListener.class */
    public interface IQDeviceEventListener {
        void onDeviceStatusChanged(IQDevice iQDevice, IQDevice.IQDeviceStatus iQDeviceStatus);
    }

    /* loaded from: classes.jar:com/garmin/android/connectiq/ConnectIQ$IQMessageStatus.class */
    public enum IQMessageStatus {
        SUCCESS,
        FAILURE_UNKNOWN,
        FAILURE_INVALID_FORMAT,
        FAILURE_MESSAGE_TOO_LARGE,
        FAILURE_UNSUPPORTED_TYPE,
        FAILURE_DURING_TRANSFER,
        FAILURE_INVALID_DEVICE,
        FAILURE_DEVICE_NOT_CONNECTED
    }

    /* loaded from: classes.jar:com/garmin/android/connectiq/ConnectIQ$IQOpenApplicationListener.class */
    public interface IQOpenApplicationListener {
        void onOpenApplicationResponse(IQDevice iQDevice, IQApp iQApp, IQOpenApplicationStatus iQOpenApplicationStatus);
    }

    /* loaded from: classes.jar:com/garmin/android/connectiq/ConnectIQ$IQSdkErrorStatus.class */
    public enum IQSdkErrorStatus {
        GCM_NOT_INSTALLED,
        GCM_UPGRADE_NEEDED,
        SERVICE_ERROR
    }

    /* loaded from: classes.jar:com/garmin/android/connectiq/ConnectIQ$IQSendImageListener.class */
    public interface IQSendImageListener {
        void onImageStatus(IQDevice iQDevice, IQApp iQApp, IQMessageStatus iQMessageStatus);
    }

    /* loaded from: classes.jar:com/garmin/android/connectiq/ConnectIQ$IQSendMessageListener.class */
    public interface IQSendMessageListener {
        void onMessageStatus(IQDevice iQDevice, IQApp iQApp, IQMessageStatus iQMessageStatus);
    }

    public abstract List<IQDevice> getConnectedDevices() throws InvalidStateException, ServiceUnavailableException;

    public abstract List<IQDevice> getKnownDevices() throws InvalidStateException, ServiceUnavailableException;

    public abstract IQDevice.IQDeviceStatus getDeviceStatus(IQDevice iQDevice) throws InvalidStateException, ServiceUnavailableException;

    protected abstract void sendMessageProtocol(IQDevice iQDevice, IQApp iQApp, byte[] bArr, IQSendMessageListener iQSendMessageListener) throws InvalidStateException, ServiceUnavailableException;

    protected abstract void sendImageProtocol(IQDevice iQDevice, IQApp iQApp, byte[] bArr, IQSendImageListener iQSendImageListener) throws InvalidStateException, ServiceUnavailableException;

    protected abstract void registerForRemoteAppEvents(IQDevice iQDevice, IQApp iQApp) throws InvalidStateException, ServiceUnavailableException;

    /* loaded from: classes.jar:com/garmin/android/connectiq/ConnectIQ$IQOpenApplicationStatus.class */
    public enum IQOpenApplicationStatus {
        PROMPT_SHOWN_ON_DEVICE,
        PROMPT_NOT_SHOWN_ON_DEVICE,
        APP_IS_NOT_INSTALLED,
        APP_IS_ALREADY_RUNNING,
        UNKNOWN_FAILURE;

        public static IQOpenApplicationStatus fromInt(int code) {
            if (code < values().length - 1) {
                return values()[code];
            }
            return UNKNOWN_FAILURE;
        }
    }

    /* JADX INFO: Access modifiers changed from: private */
    /* loaded from: classes.jar:com/garmin/android/connectiq/ConnectIQ$DeviceRegistryEntry.class */
    public class DeviceRegistryEntry {
        public IQMessageReceiver receiver;
        public List<String> appIds = new ArrayList();

        public DeviceRegistryEntry() {
        }
    }

    /* JADX INFO: Access modifiers changed from: package-private */
    /* renamed from: com.garmin.android.connectiq.ConnectIQ$3  reason: invalid class name */
    /* loaded from: classes.jar:com/garmin/android/connectiq/ConnectIQ$3.class */
    public static /* synthetic */ class AnonymousClass3 {
        static final /* synthetic */ int[] $SwitchMap$com$garmin$android$connectiq$ConnectIQ$IQConnectType = new int[IQConnectType.values().length];

        static {
            try {
                $SwitchMap$com$garmin$android$connectiq$ConnectIQ$IQConnectType[IQConnectType.TETHERED.ordinal()] = 1;
            } catch (NoSuchFieldError e) {
            }
            try {
                $SwitchMap$com$garmin$android$connectiq$ConnectIQ$IQConnectType[IQConnectType.WIRELESS.ordinal()] = 2;
            } catch (NoSuchFieldError e2) {
            }
        }
    }

    public static ConnectIQ getInstance() {
        if (sInstance == null) {
            switch (AnonymousClass3.$SwitchMap$com$garmin$android$connectiq$ConnectIQ$IQConnectType[sConnectType.ordinal()]) {
                case 1:
                    sInstance = new ConnectIQAdbStrategy();
                    break;
                case MonkeyType.FLOAT /* 2 */:
                default:
                    sInstance = new ConnectIQDeviceStrategy();
                    break;
            }
        }
        return sInstance;
    }

    public static ConnectIQ getInstance(Context context, IQConnectType connectType) {
        if (!(sInstance == null || sConnectType == connectType)) {
            try {
                sInstance.shutdown(context);
            } catch (InvalidStateException e) {
            }
            sInstance = null;
        }
        sConnectType = connectType;
        return getInstance();
    }

    public void setAdbPort(int port) throws IllegalArgumentException {
        if (port < 1025 || port > 65535) {
            throw new IllegalArgumentException("Invalid port number.  Must be between 1025 - 65535");
        }
        AdbConnection.getInstance().setPort(port);
    }

    public int getAdbPort() {
        return AdbConnection.getInstance().getPort();
    }

    public void initialize(Context context, boolean autoUI, ConnectIQListener listener) {
        this.mContext = context;
        this.mListener = listener;
        this.mHandler = new Handler();
        this.mMessageReceiver = new IQMessageReceiver();
        IntentFilter filter = new IntentFilter();
        filter.addAction(APPLICATION_INFO);
        filter.addAction(OPEN_APPLICATION);
        filter.addAction(SEND_MESSAGE_STATUS);
        context.registerReceiver(this.mMessageReceiver, filter);
        this.mInitialized = true;
    }

    public void shutdown(Context context) throws InvalidStateException {
        unregisterAllForEvents();
        if (this.mListener != null) {
            this.mListener.onSdkShutDown();
        }
        context.unregisterReceiver(this.mMessageReceiver);
        this.mMessageReceiver = null;
        this.mInitialized = false;
    }

    /* JADX INFO: Access modifiers changed from: protected */
    public void displayGCMDialog(final String title, final String message, final String negButton, final String posButton) {
        if (this.mHandler != null) {
            this.mHandler.post(new Runnable() { // from class: com.garmin.android.connectiq.ConnectIQ.1
                @Override // java.lang.Runnable
                public void run() {
                    AlertDialog.Builder builder = new AlertDialog.Builder(ConnectIQ.this.mContext);
                    builder.setTitle(title).setMessage(message).setNegativeButton(negButton, (DialogInterface.OnClickListener) null).setPositiveButton(posButton, new DialogInterface.OnClickListener() { // from class: com.garmin.android.connectiq.ConnectIQ.1.1
                        @Override // android.content.DialogInterface.OnClickListener
                        public void onClick(DialogInterface dialog, int which) {
                            try {
                                ConnectIQ.this.mContext.startActivity(new Intent("android.intent.action.VIEW", Uri.parse("market://details?id=com.garmin.android.apps.connectmobile")));
                            } catch (ActivityNotFoundException e) {
                                ConnectIQ.this.mContext.startActivity(new Intent("android.intent.action.VIEW", Uri.parse("https://play.google.com/store/apps/details?id=com.garmin.android.apps.connectmobile")));
                            }
                        }
                    });
                    builder.create().show();
                }
            });
        }
    }

    public void registerForEvents(IQDevice device, IQDeviceEventListener listener, IQApp app, IQApplicationEventListener applistener) throws InvalidStateException {
        verifyInitialized();
        registerForDeviceEvents(device, listener);
        registerForAppEvents(device, app, applistener);
    }

    public void registerForDeviceEvents(IQDevice device, IQDeviceEventListener listener) throws InvalidStateException {
        verifyInitialized();
        String registryKey = device.getDeviceIdentifier() + "";
        DeviceRegistryEntry entry = this.mDeviceRegistry.get(registryKey);
        if (entry == null) {
            entry = new DeviceRegistryEntry();
        }
        if (entry.receiver != null) {
            IQDeviceEventListener eventlistener = entry.receiver.getListener();
            if (eventlistener != listener) {
                entry.receiver.setListener(listener);
                return;
            }
            return;
        }
        entry.receiver = new IQMessageReceiver(listener, null);
        this.mDeviceRegistry.put(registryKey, entry);
        IntentFilter filter = new IntentFilter();
        filter.addAction(DEVICE_STATUS);
        filter.addAction(INCOMING_MESSAGE);
        this.mContext.registerReceiver(entry.receiver, filter);
        if (listener != null) {
            IQDevice.IQDeviceStatus status = IQDevice.IQDeviceStatus.UNKNOWN;
            try {
                status = getDeviceStatus(device);
            } catch (InvalidStateException e) {
            } catch (ServiceUnavailableException e2) {
                status = IQDevice.IQDeviceStatus.NOT_CONNECTED;
            }
            listener.onDeviceStatusChanged(device, status);
        }
    }

    public void registerForAppEvents(IQDevice device, IQApp app, IQApplicationEventListener applistener) throws InvalidStateException {
        verifyInitialized();
        try {
            registerForRemoteAppEvents(device, app);
        } catch (InvalidStateException e) {
        } catch (ServiceUnavailableException e2) {
        }
        String registryKey = device.getDeviceIdentifier() + "";
        DeviceRegistryEntry entry = this.mDeviceRegistry.get(registryKey);
        if (entry == null) {
            entry = new DeviceRegistryEntry();
        }
        if (entry.receiver != null) {
            IQApplicationEventListener eventlistener = entry.receiver.getAppListener();
            if (eventlistener != applistener) {
                entry.receiver.setAppListener(applistener);
                return;
            }
            return;
        }
        if (!entry.appIds.contains(app.getApplicationId())) {
            entry.appIds.add(app.getApplicationId());
        }
        entry.receiver = new IQMessageReceiver(null, applistener);
        this.mDeviceRegistry.put(registryKey, entry);
        IntentFilter filter = new IntentFilter();
        filter.addAction(INCOMING_MESSAGE);
        filter.addAction(DEVICE_STATUS);
        this.mContext.registerReceiver(entry.receiver, filter);
    }

    public void unregisterAllForEvents() throws InvalidStateException {
        verifyInitialized();
        for (String registryKey : this.mDeviceRegistry.keySet()) {
            DeviceRegistryEntry entry = this.mDeviceRegistry.get(registryKey);
            if (!(entry == null || entry.receiver == null)) {
                try {
                    this.mContext.unregisterReceiver(entry.receiver);
                } catch (IllegalArgumentException e) {
                }
            }
        }
        this.mDeviceRegistry.clear();
    }

    public void unregisterForEvents(IQDevice device) throws InvalidStateException {
        verifyInitialized();
        String registryKey = device.getDeviceIdentifier() + "";
        DeviceRegistryEntry entry = this.mDeviceRegistry.get(registryKey);
        if (entry != null && entry.receiver != null) {
            this.mContext.unregisterReceiver(entry.receiver);
            this.mDeviceRegistry.remove(registryKey);
        }
    }

    public void unregisterForDeviceEvents(IQDevice device) throws InvalidStateException {
        verifyInitialized();
        String registryKey = device.getDeviceIdentifier() + "";
        DeviceRegistryEntry entry = this.mDeviceRegistry.get(registryKey);
        if (entry != null) {
            if (entry.receiver != null) {
                entry.receiver.setListener(null);
                if (entry.receiver.getAppListener() == null) {
                    this.mContext.unregisterReceiver(entry.receiver);
                    entry.receiver = null;
                }
            }
            if (entry.receiver == null) {
                this.mDeviceRegistry.remove(registryKey);
            }
        }
    }

    public void unregisterForApplicationEvents(IQDevice device, IQApp application) throws InvalidStateException {
        verifyInitialized();
        String registryKey = device.getDeviceIdentifier() + "";
        DeviceRegistryEntry entry = this.mDeviceRegistry.get(registryKey);
        if (entry != null) {
            entry.appIds.remove(application.getApplicationId());
            if (entry.receiver != null && entry.appIds.size() == 0) {
                entry.receiver.setAppListener(null);
                if (entry.receiver.getListener() == null) {
                    this.mContext.unregisterReceiver(entry.receiver);
                    entry.receiver = null;
                }
            }
            if (entry.receiver == null) {
                this.mDeviceRegistry.remove(registryKey);
            }
        }
    }

    public boolean openStore(String storeID) throws IllegalArgumentException, InvalidStateException, ServiceUnavailableException, RemoteException {
        throw new UnsupportedOperationException();
    }

    public void getApplicationInfo(String applicationId, IQDevice device, IQApplicationInfoListener listener) throws InvalidStateException, ServiceUnavailableException {
        throw new UnsupportedOperationException();
    }

    public void openApplication(IQDevice device, IQApp application, IQOpenApplicationListener listener) throws InvalidStateException, ServiceUnavailableException {
        throw new UnsupportedOperationException();
    }

    public void sendMessage(IQDevice device, IQApp application, Object javaObject, IQSendMessageListener listener) throws InvalidStateException, ServiceUnavailableException {
        verifyInitialized();
        byte[] bytes = null;
        try {
            bytes = Serializer.serialize(javaObject);
        } catch (Exception e) {
            if (listener != null) {
                listener.onMessageStatus(device, application, IQMessageStatus.FAILURE_INVALID_FORMAT);
            }
        }
        if (bytes != null) {
            sendMessage(device, application, bytes, listener);
        }
    }

    private void sendMessage(IQDevice device, IQApp application, byte[] data, IQSendMessageListener listener) throws InvalidStateException, ServiceUnavailableException {
        if (data.length > 16384 && listener != null) {
            listener.onMessageStatus(device, application, IQMessageStatus.FAILURE_MESSAGE_TOO_LARGE);
        }
        sendMessageProtocol(device, application, data, listener);
    }

    /* JADX INFO: Access modifiers changed from: protected */
    public void deviceStatusChanged(final IQDevice device, final IQDevice.IQDeviceStatus newStatus) {
        if (this.mHandler != null) {
            this.mHandler.post(new Runnable() { // from class: com.garmin.android.connectiq.ConnectIQ.2
                @Override // java.lang.Runnable
                public void run() {
                    IQDeviceEventListener listener;
                    String registryKey = device.getDeviceIdentifier() + "";
                    DeviceRegistryEntry entry = (DeviceRegistryEntry) ConnectIQ.this.mDeviceRegistry.get(registryKey);
                    if (entry != null && entry.receiver != null && (listener = entry.receiver.getListener()) != null) {
                        listener.onDeviceStatusChanged(device, newStatus);
                    }
                }
            });
        }
    }

    /* JADX INFO: Access modifiers changed from: protected */
    public void verifyInitialized() throws InvalidStateException {
        if (!this.mInitialized) {
            throw new InvalidStateException("SDK not initialized");
        }
    }
}
