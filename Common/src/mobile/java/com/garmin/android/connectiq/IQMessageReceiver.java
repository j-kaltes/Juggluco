package com.garmin.android.connectiq;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import com.garmin.android.connectiq.ConnectIQ;
import com.garmin.android.connectiq.IQDevice;
import com.garmin.monkeybrains.serialization.MonkeyArray;
import com.garmin.monkeybrains.serialization.MonkeyHash;
import com.garmin.monkeybrains.serialization.MonkeyType;
import com.garmin.monkeybrains.serialization.Serializer;
import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

/* loaded from: classes.jar:com/garmin/android/connectiq/IQMessageReceiver.class */
public class IQMessageReceiver extends BroadcastReceiver {
    private static final String sTAG = "IQMessageReceiver";
    private static final int INVALID_APP_VERSION = 65535;
    private ConnectIQ.IQDeviceEventListener listener;
    private ConnectIQ.IQApplicationEventListener applistener;

    public IQMessageReceiver() {
        setListener(null);
        setAppListener(null);
    }

    public IQMessageReceiver(ConnectIQ.IQDeviceEventListener listener, ConnectIQ.IQApplicationEventListener applistener) {
        setListener(listener);
        setAppListener(applistener);
    }

    public void setListener(ConnectIQ.IQDeviceEventListener listener) {
        this.listener = listener;
    }

    public void setAppListener(ConnectIQ.IQApplicationEventListener applistener) {
        this.applistener = applistener;
    }

    public ConnectIQ.IQDeviceEventListener getListener() {
        return this.listener;
    }

    public ConnectIQ.IQApplicationEventListener getAppListener() {
        return this.applistener;
    }

    @Override // android.content.BroadcastReceiver
    public void onReceive(Context context, Intent intent) {
        if (intent.getAction().equals(ConnectIQ.INCOMING_MESSAGE)) {
            IQDevice device = (IQDevice) intent.getParcelableExtra(ConnectIQ.EXTRA_REMOTE_DEVICE);
            IQApp app = (IQApp) intent.getParcelableExtra(ConnectIQ.EXTRA_REMOTE_APPLICATION);
            byte[] bytes = intent.getByteArrayExtra(ConnectIQ.EXTRA_PAYLOAD);
            if (bytes == null && this.applistener != null) {
                this.applistener.onMessageReceived(device, app, null, ConnectIQ.IQMessageStatus.FAILURE_UNKNOWN);
            }
            try {
                List<MonkeyType<?>> data = Serializer.deserialize(bytes);
                List<Object> javaData = new ArrayList<>();
                for (MonkeyType<?> obj : data) {
                    javaData.add(convertToJava(obj));
                }
                if (this.applistener != null) {
                    this.applistener.onMessageReceived(device, app, javaData, ConnectIQ.IQMessageStatus.SUCCESS);
                }
            } catch (UnsupportedEncodingException e) {
                Log.e("RemoteMessageReceiver", "Error deserializing message", e);
                if (this.applistener != null) {
                    this.applistener.onMessageReceived(device, app, null, ConnectIQ.IQMessageStatus.FAILURE_INVALID_FORMAT);
                }
            }
              catch( Throwable  error) {
		      String mess=error.getMessage();
		      if(mess==null) {
			       mess="OnReceive Exception";
			       }
		      Log.e("IQMessageReceiver",mess);
			if (this.applistener != null) {
			       this.applistener.onMessageReceived(device, app, (List<Object>) null, ConnectIQ.IQMessageStatus.FAILURE_UNKNOWN);
			}
		  }


        } else if (intent.getAction().equals(ConnectIQ.DEVICE_STATUS)) {
            IQDevice device2 = (IQDevice) intent.getParcelableExtra(ConnectIQ.EXTRA_REMOTE_DEVICE);
            int status = intent.getIntExtra(ConnectIQ.EXTRA_STATUS, IQDevice.IQDeviceStatus.UNKNOWN.ordinal());
            IQDevice.IQDeviceStatus deviceStatus = IQDevice.IQDeviceStatus.UNKNOWN;
            try {
                deviceStatus = IQDevice.IQDeviceStatus.values()[status];
            } catch (IndexOutOfBoundsException e2) {
            }
            if (this.listener != null) {
                this.listener.onDeviceStatusChanged(device2, deviceStatus);
            }
        } else if (intent.getAction().equals(ConnectIQ.APPLICATION_INFO)) {
            String appId = intent.getStringExtra(ConnectIQ.EXTRA_APPLICATION_ID);
            int version = intent.getIntExtra(ConnectIQ.EXTRA_APPLICATION_VERSION, INVALID_APP_VERSION);
            if (ConnectIQStateManager.getInstance().getApplicationInfoListener() == null) {
                return;
            }
            if (appId == null || version < 0 || INVALID_APP_VERSION == version) {
                ConnectIQStateManager.getInstance().getApplicationInfoListener().onApplicationNotInstalled(appId);
            } else {
                ConnectIQStateManager.getInstance().getApplicationInfoListener().onApplicationInfoReceived(new IQApp(appId, version));
            }
        } else if (intent.getAction().equals(ConnectIQ.OPEN_APPLICATION)) {
            IQDevice iqDevice = (IQDevice) intent.getParcelableExtra(ConnectIQ.EXTRA_OPEN_APPLICATION_DEVICE);

            String appId2 = intent.getStringExtra(ConnectIQ.EXTRA_OPEN_APPLICATION_ID);
            int resultCode = intent.getIntExtra(ConnectIQ.EXTRA_OPEN_APPLICATION_RESULT_CODE, -1);
            if (!(ConnectIQStateManager.getInstance().getOpenApplicationListener() == null || appId2 == null || resultCode < 0)) {
                IQApp app2 = new IQApp(appId2);
                ConnectIQ.IQOpenApplicationStatus status2 = ConnectIQ.IQOpenApplicationStatus.fromInt(resultCode);
                ConnectIQStateManager.getInstance().getOpenApplicationListener().onOpenApplicationResponse(iqDevice, app2, status2);
            }
        } else if (intent.getAction().equals(ConnectIQ.SEND_MESSAGE_STATUS)) {
            IQDevice iqDevice2 = (IQDevice) intent.getParcelableExtra(ConnectIQ.EXTRA_REMOTE_DEVICE);
            int statusValue = intent.getIntExtra(ConnectIQ.EXTRA_STATUS, 0);
            String appId3 = intent.getStringExtra(ConnectIQ.EXTRA_APPLICATION_ID);
            if (ConnectIQStateManager.getInstance().getSendMessageListener() != null) {
                ConnectIQStateManager.getInstance().getSendMessageListener().onMessageStatus(iqDevice2, new IQApp(appId3), statusValue == 0 ? ConnectIQ.IQMessageStatus.SUCCESS : ConnectIQ.IQMessageStatus.FAILURE_DURING_TRANSFER);
            }
        }
    }

    private Object convertToJava(MonkeyType<?> object) {
        switch (object.mType) {
            case 0:
            case 4:
            case 6:
            case 7:
            case 8:
            case 10:
            case 12:
            case 13:
            case 16:
            case 17:
            case 18:
            default:
                return null;
            case 1:
            case MonkeyType.FLOAT /* 2 */:
            case MonkeyType.STRING /* 3 */:
            case MonkeyType.BOOLEAN /* 9 */:
            case MonkeyType.LONG /* 14 */:
            case MonkeyType.DOUBLE /* 15 */:
            case MonkeyType.CHAR /* 19 */:
                return object.toJava();
            case MonkeyType.ARRAY /* 5 */:
                MonkeyArray array = (MonkeyArray) object;
                List<MonkeyType<?>> iqArray = array.toJava();
                List<Object> javaArray = new ArrayList<>();
                for (MonkeyType<?> obj : iqArray) {
                    javaArray.add(convertToJava(obj));
                }
                return javaArray;
            case MonkeyType.HASH /* 11 */:
                MonkeyHash hash = (MonkeyHash) object;
                HashMap<MonkeyType<?>, MonkeyType<?>> iqHash = hash.toJava();
                HashMap<Object, Object> javaHash = new HashMap<>();
                for (MonkeyType<?> key : iqHash.keySet()) {
                    Object javaKey = convertToJava(key);
                    MonkeyType<?> value = iqHash.get(key);
                    Object javaValue = convertToJava(value);
                    javaHash.put(javaKey, javaValue);
                }
                return javaHash;
        }
    }
}
