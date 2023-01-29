package com.garmin.android.connectiq;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.os.IBinder;
import android.os.RemoteException;
import com.garmin.android.apps.connectmobile.connectiq.IConnectIQService;
import com.garmin.android.connectiq.ConnectIQ;
import com.garmin.android.connectiq.IQDevice;
import com.garmin.android.connectiq.exception.InvalidStateException;
import com.garmin.android.connectiq.exception.ServiceUnavailableException;
import java.util.List;
import java.util.UUID;

/* loaded from: classes.jar:com/garmin/android/connectiq/ConnectIQDeviceStrategy.class */
public class ConnectIQDeviceStrategy extends ConnectIQ {
    private IConnectIQService mRemoteService;
    private boolean mBound = false;
    private ServiceConnection mConnection = new ServiceConnection() { // from class: com.garmin.android.connectiq.ConnectIQDeviceStrategy.1
        @Override // android.content.ServiceConnection
        public void onServiceConnected(ComponentName className, IBinder service) {
            ConnectIQDeviceStrategy.this.mRemoteService = IConnectIQService.Stub.asInterface(service);
            ConnectIQDeviceStrategy.this.mBound = true;
            if (ConnectIQDeviceStrategy.this.mListener != null) {
                ConnectIQDeviceStrategy.this.mListener.onSdkReady();
            }
        }

        @Override // android.content.ServiceConnection
        public void onServiceDisconnected(ComponentName name) {
            ConnectIQDeviceStrategy.this.mRemoteService = null;
            ConnectIQDeviceStrategy.this.mBound = false;
            if (ConnectIQDeviceStrategy.this.mListener != null) {
                ConnectIQDeviceStrategy.this.mListener.onSdkShutDown();
            }
        }
    };

    @Override // com.garmin.android.connectiq.ConnectIQ
    public void initialize(Context context, boolean autoUI, ConnectIQ.ConnectIQListener listener) {
        super.initialize(context, autoUI, listener);
        PackageManager pm = this.mContext.getPackageManager();
        try {
            PackageInfo pi = pm.getPackageInfo("com.garmin.android.apps.connectmobile", 0);
            if (pi.versionCode < 2000) {
                if (this.mListener != null) {
                    this.mListener.onInitializeError(ConnectIQ.IQSdkErrorStatus.GCM_UPGRADE_NEEDED);
                }
                if (autoUI) {
                    String packageName = this.mContext.getPackageName();
                    Resources res = this.mContext.getResources();
                    int titleRes = res.getIdentifier("upgrade_needed_title", "string", packageName);
                    int messageRes = res.getIdentifier("upgrade_needed_message", "string", packageName);
                    int cancelRes = res.getIdentifier("upgrade_cancel", "string", packageName);
                    int yesRes = res.getIdentifier("upgrade_yes", "string", packageName);
                    String title = "Upgrade Needed";
                    String message = "An upgrade to Garmin Connect Mobile is required to use this application. Would you like to upgrade now?";
                    String cancel = "Cancel";
                    String yes = "Upgrade";
                    if (titleRes != 0) {
                        title = this.mContext.getString(titleRes);
                    }
                    if (messageRes != 0) {
                        message = this.mContext.getString(messageRes);
                    }
                    if (cancelRes != 0) {
                        cancel = this.mContext.getString(cancelRes);
                    }
                    if (yesRes != 0) {
                        yes = this.mContext.getString(yesRes);
                    }
                    displayGCMDialog(title, message, cancel, yes);
                    return;
                }
                return;
            }
            Intent serviceIntent = new Intent("com.garmin.android.apps.connectmobile.CONNECTIQ_SERVICE_ACTION");
            serviceIntent.setComponent(new ComponentName("com.garmin.android.apps.connectmobile", "com.garmin.android.apps.connectmobile.connectiq.ConnectIQService"));
            this.mContext.bindService(serviceIntent, this.mConnection, 1);
        } catch (PackageManager.NameNotFoundException e) {
            if (autoUI) {
                String packageName2 = this.mContext.getPackageName();
                Resources res2 = this.mContext.getResources();
                int titleRes2 = res2.getIdentifier("install_needed_title", "string", packageName2);
                int messageRes2 = res2.getIdentifier("install_needed_message", "string", packageName2);
                int cancelRes2 = res2.getIdentifier("install_cancel", "string", packageName2);
                int yesRes2 = res2.getIdentifier("install_yes", "string", packageName2);
                String title2 = "Additional App Required";
                String message2 = "Garmin Connect Mobile is required to use this application. Would you like to install it now?";
                String cancel2 = "Cancel";
                String yes2 = "Yes";
                if (titleRes2 != 0) {
                    title2 = this.mContext.getString(titleRes2);
                }
                if (messageRes2 != 0) {
                    message2 = this.mContext.getString(messageRes2);
                }
                if (cancelRes2 != 0) {
                    cancel2 = this.mContext.getString(cancelRes2);
                }
                if (yesRes2 != 0) {
                    yes2 = this.mContext.getString(yesRes2);
                }
                displayGCMDialog(title2, message2, cancel2, yes2);
            }
            if (this.mListener != null) {
                this.mListener.onInitializeError(ConnectIQ.IQSdkErrorStatus.GCM_NOT_INSTALLED);
            }
            super.initialize(context, autoUI, listener);
        }
    }

    @Override // com.garmin.android.connectiq.ConnectIQ
    public void shutdown(Context context) throws InvalidStateException {
        super.shutdown(context);
        if (this.mBound) {
            this.mContext.unbindService(this.mConnection);
        }
    }

    @Override // com.garmin.android.connectiq.ConnectIQ
    public List<IQDevice> getConnectedDevices() throws InvalidStateException, ServiceUnavailableException {
        verifyInitialized();
        if (!this.mBound) {
            throw new InvalidStateException("SDK not initialized.  Did you forget to call ConnectIQ::initialize()?");
        }
        try {
            List<IQDevice> devices = this.mRemoteService.getConnectedDevices();
            return devices;
        } catch (RemoteException e) {
            throw new ServiceUnavailableException(e.getMessage());
        }
    }

    @Override // com.garmin.android.connectiq.ConnectIQ
    public List<IQDevice> getKnownDevices() throws InvalidStateException, ServiceUnavailableException {
        verifyInitialized();
        if (!this.mBound) {
            throw new InvalidStateException("SDK not initialized.  Did you forget to call ConnectIQ::initialize()?");
        }
        try {
            List<IQDevice> devices = this.mRemoteService.getKnownDevices();
            return devices;
        } catch (RemoteException e) {
            throw new ServiceUnavailableException(e.getMessage());
        }
    }

    @Override // com.garmin.android.connectiq.ConnectIQ
    public IQDevice.IQDeviceStatus getDeviceStatus(IQDevice device) throws InvalidStateException, ServiceUnavailableException {
        verifyInitialized();
        if (!this.mBound) {
            throw new InvalidStateException("SDK not initialized.  Did you forget to call ConnectIQ::initialize()?");
        }
        IQDevice.IQDeviceStatus status = IQDevice.IQDeviceStatus.UNKNOWN;
        try {
            int statusOrdinal = this.mRemoteService.getStatus(device);
            status = IQDevice.IQDeviceStatus.values()[statusOrdinal];
        } catch (RemoteException e) {
            throw new ServiceUnavailableException(e.getMessage());
        } catch (IndexOutOfBoundsException e2) {
        }
        return status;
    }

    @Override // com.garmin.android.connectiq.ConnectIQ
    protected void sendMessageProtocol(IQDevice device, IQApp application, byte[] data, ConnectIQ.IQSendMessageListener listener) throws InvalidStateException, ServiceUnavailableException {
        verifyInitialized();
        if (!this.mBound) {
            throw new InvalidStateException("SDK not initialized.  Did you forget to call ConnectIQ::initialize()?");
        } else if (device != null || listener == null) {
            ConnectIQStateManager.getInstance().setSendMessageListener(listener);
            try {
                IQMessage message = new IQMessage(data, this.mContext.getPackageName(), ConnectIQ.SEND_MESSAGE_STATUS);
                this.mRemoteService.sendMessage(message, device, application);
            } catch (RemoteException e) {
                throw new ServiceUnavailableException(e.getMessage());
            }
        } else {
            listener.onMessageStatus(device, application, ConnectIQ.IQMessageStatus.FAILURE_INVALID_DEVICE);
        }
    }

    @Override // com.garmin.android.connectiq.ConnectIQ
    protected void sendImageProtocol(IQDevice device, IQApp application, byte[] imageData, ConnectIQ.IQSendImageListener listener) {
        throw new UnsupportedOperationException();
    }

    @Override // com.garmin.android.connectiq.ConnectIQ
    public void getApplicationInfo(String applicationId, IQDevice device, ConnectIQ.IQApplicationInfoListener listener) throws InvalidStateException, ServiceUnavailableException {
        verifyInitialized();
        String applicationId2 = applicationId.replaceAll("[\\s\\-]", "");
        if (!this.mBound) {
            throw new InvalidStateException("SDK not initialized.  Did you forget to call ConnectIQ::initialize()?");
        } else if (device != null || listener == null) {
            ConnectIQStateManager.getInstance().setApplicationInfoListener(listener);
            try {
                this.mRemoteService.getApplicationInfo(this.mContext.getPackageName(), ConnectIQ.APPLICATION_INFO, device, applicationId2);
            } catch (RemoteException e) {
                throw new ServiceUnavailableException(e.getMessage());
            }
        } else {
            listener.onApplicationNotInstalled(applicationId2);
        }
    }

    @Override // com.garmin.android.connectiq.ConnectIQ
    public void openApplication(IQDevice device, IQApp app, ConnectIQ.IQOpenApplicationListener listener) throws InvalidStateException, ServiceUnavailableException {
        verifyInitialized();
        if (!this.mBound) {
            throw new InvalidStateException("SDK not initialized.  Did you forget to call ConnectIQ::initialize()?");
        } else if (device != null || listener == null) {
            ConnectIQStateManager.getInstance().setOpenApplicationListener(listener);
            try {
                this.mRemoteService.openApplication(this.mContext.getPackageName(), ConnectIQ.OPEN_APPLICATION, device, app);
            } catch (RemoteException ex) {
                throw new ServiceUnavailableException(ex.getMessage());
            }
        } else {
            listener.onOpenApplicationResponse(device, app, ConnectIQ.IQOpenApplicationStatus.APP_IS_NOT_INSTALLED);
        }
    }

    @Override // com.garmin.android.connectiq.ConnectIQ
    protected void registerForRemoteAppEvents(IQDevice device, IQApp app) throws InvalidStateException, ServiceUnavailableException {
        verifyInitialized();
        if (!this.mBound) {
            throw new InvalidStateException("SDK not initialized.  Did you forget to call ConnectIQ::initialize()?");
        }
        try {
            this.mRemoteService.registerApp(app, ConnectIQ.INCOMING_MESSAGE, this.mContext.getPackageName());
        } catch (RemoteException e) {
            throw new ServiceUnavailableException(e.getMessage());
        }
    }

    @Override // com.garmin.android.connectiq.ConnectIQ
    public boolean openStore(String storeID) throws IllegalArgumentException, InvalidStateException, ServiceUnavailableException,RemoteException {
        verifyInitialized();
        if (!this.mBound) {
            throw new InvalidStateException("SDK not initialized.  Did you forget to call ConnectIQ::initialize()?");
        }
        if (null != storeID) {
            try {
                if (!storeID.isEmpty()) {
                    if (!storeID.contains("-")) {
                        storeID = storeID.replaceAll("(\\w{8})(\\w{4})(\\w{4})(\\w{4})(\\w{12})", "$1-$2-$3-$4-$5");
                    }
                    return this.mRemoteService.openStore(UUID.fromString(storeID).toString());
                }
            } catch (RemoteException e) {
                throw new ServiceUnavailableException(e.getMessage());
            }
        }
        return this.mRemoteService.openStore(null);
    }
}
