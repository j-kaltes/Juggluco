package com.garmin.android.apps.connectmobile.connectiq;

import android.os.Binder;
import android.os.IBinder;
import android.os.IInterface;
import android.os.Parcel;
import android.os.RemoteException;
import com.garmin.android.connectiq.IQApp;
import com.garmin.android.connectiq.IQDevice;
import com.garmin.android.connectiq.IQMessage;
import java.util.List;

/* loaded from: classes.jar:com/garmin/android/apps/connectmobile/connectiq/IConnectIQService.class */
public interface IConnectIQService extends IInterface {
    boolean openStore(String str) throws RemoteException;

    List<IQDevice> getConnectedDevices() throws RemoteException;

    List<IQDevice> getKnownDevices() throws RemoteException;

    int getStatus(IQDevice iQDevice) throws RemoteException;

    void getApplicationInfo(String str, String str2, IQDevice iQDevice, String str3) throws RemoteException;

    void openApplication(String str, String str2, IQDevice iQDevice, IQApp iQApp) throws RemoteException;

    void sendMessage(IQMessage iQMessage, IQDevice iQDevice, IQApp iQApp) throws RemoteException;

    void sendImage(IQMessage iQMessage, IQDevice iQDevice, IQApp iQApp) throws RemoteException;

    void registerApp(IQApp iQApp, String str, String str2) throws RemoteException;

    /* loaded from: classes.jar:com/garmin/android/apps/connectmobile/connectiq/IConnectIQService$Default.class */
    public static class Default implements IConnectIQService {
        @Override // com.garmin.android.apps.connectmobile.connectiq.IConnectIQService
        public boolean openStore(String applicationID) throws RemoteException {
            return false;
        }

        @Override // com.garmin.android.apps.connectmobile.connectiq.IConnectIQService
        public List<IQDevice> getConnectedDevices() throws RemoteException {
            return null;
        }

        @Override // com.garmin.android.apps.connectmobile.connectiq.IConnectIQService
        public List<IQDevice> getKnownDevices() throws RemoteException {
            return null;
        }

        @Override // com.garmin.android.apps.connectmobile.connectiq.IConnectIQService
        public int getStatus(IQDevice device) throws RemoteException {
            return 0;
        }

        @Override // com.garmin.android.apps.connectmobile.connectiq.IConnectIQService
        public void getApplicationInfo(String notificationPackage, String notificationAction, IQDevice device, String applicationID) throws RemoteException {
        }

        @Override // com.garmin.android.apps.connectmobile.connectiq.IConnectIQService
        public void openApplication(String notificationPackage, String notificationAction, IQDevice device, IQApp app) throws RemoteException {
        }

        @Override // com.garmin.android.apps.connectmobile.connectiq.IConnectIQService
        public void sendMessage(IQMessage message, IQDevice device, IQApp app) throws RemoteException {
        }

        @Override // com.garmin.android.apps.connectmobile.connectiq.IConnectIQService
        public void sendImage(IQMessage image, IQDevice device, IQApp app) throws RemoteException {
        }

        @Override // com.garmin.android.apps.connectmobile.connectiq.IConnectIQService
        public void registerApp(IQApp app, String notificationAction, String notificationPackage) throws RemoteException {
        }

        @Override // android.os.IInterface
        public IBinder asBinder() {
            return null;
        }
    }

    /* loaded from: classes.jar:com/garmin/android/apps/connectmobile/connectiq/IConnectIQService$Stub.class */
    public static abstract class Stub extends Binder implements IConnectIQService {
        private static final String DESCRIPTOR = "com.garmin.android.apps.connectmobile.connectiq.IConnectIQService";
        static final int TRANSACTION_openStore = 1;
        static final int TRANSACTION_getConnectedDevices = 2;
        static final int TRANSACTION_getKnownDevices = 3;
        static final int TRANSACTION_getStatus = 4;
        static final int TRANSACTION_getApplicationInfo = 5;
        static final int TRANSACTION_openApplication = 6;
        static final int TRANSACTION_sendMessage = 7;
        static final int TRANSACTION_sendImage = 8;
        static final int TRANSACTION_registerApp = 9;

        public Stub() {
            attachInterface(this, DESCRIPTOR);
        }

        public static IConnectIQService asInterface(IBinder obj) {
            if (obj == null) {
                return null;
            }
            IInterface iin = obj.queryLocalInterface(DESCRIPTOR);
            if (iin == null || !(iin instanceof IConnectIQService)) {
                return new Proxy(obj);
            }
            return (IConnectIQService) iin;
        }

        @Override // android.os.IInterface
        public IBinder asBinder() {
            return this;
        }

        @Override // android.os.Binder
        public boolean onTransact(int code, Parcel data, Parcel reply, int flags) throws RemoteException {
            IQApp _arg0;
            IQMessage _arg02;
            IQDevice _arg1;
            IQApp _arg2;
            IQMessage _arg03;
            IQDevice _arg12;
            IQApp _arg22;
            IQDevice _arg23;
            IQApp _arg3;
            IQDevice _arg24;
            IQDevice _arg04;
            switch (code) {
                case 1:
                    data.enforceInterface(DESCRIPTOR);
                    String _arg05 = data.readString();
                    boolean _result = openStore(_arg05);
                    reply.writeNoException();
                    reply.writeInt(_result ? 1 : 0);
                    return true;
                case 2:
                    data.enforceInterface(DESCRIPTOR);
                    List<IQDevice> _result2 = getConnectedDevices();
                    reply.writeNoException();
                    reply.writeTypedList(_result2);
                    return true;
                case 3:
                    data.enforceInterface(DESCRIPTOR);
                    List<IQDevice> _result3 = getKnownDevices();
                    reply.writeNoException();
                    reply.writeTypedList(_result3);
                    return true;
                case TRANSACTION_getStatus /* 4 */:
                    data.enforceInterface(DESCRIPTOR);
                    if (0 != data.readInt()) {
                        _arg04 = IQDevice.CREATOR.createFromParcel(data);
                    } else {
                        _arg04 = null;
                    }
                    int _result4 = getStatus(_arg04);
                    reply.writeNoException();
                    reply.writeInt(_result4);
                    return true;
                case 5:
                    data.enforceInterface(DESCRIPTOR);
                    String _arg06 = data.readString();
                    String _arg13 = data.readString();
                    if (0 != data.readInt()) {
                        _arg24 = IQDevice.CREATOR.createFromParcel(data);
                    } else {
                        _arg24 = null;
                    }
                    String _arg32 = data.readString();
                    getApplicationInfo(_arg06, _arg13, _arg24, _arg32);
                    return true;
                case TRANSACTION_openApplication /* 6 */:
                    data.enforceInterface(DESCRIPTOR);
                    String _arg07 = data.readString();
                    String _arg14 = data.readString();
                    if (0 != data.readInt()) {
                        _arg23 = IQDevice.CREATOR.createFromParcel(data);
                    } else {
                        _arg23 = null;
                    }
                    if (0 != data.readInt()) {
                        _arg3 = IQApp.CREATOR.createFromParcel(data);
                    } else {
                        _arg3 = null;
                    }
                    openApplication(_arg07, _arg14, _arg23, _arg3);
                    return true;
                case TRANSACTION_sendMessage /* 7 */:
                    data.enforceInterface(DESCRIPTOR);
                    if (0 != data.readInt()) {
                        _arg03 = IQMessage.CREATOR.createFromParcel(data);
                    } else {
                        _arg03 = null;
                    }
                    if (0 != data.readInt()) {
                        _arg12 = IQDevice.CREATOR.createFromParcel(data);
                    } else {
                        _arg12 = null;
                    }
                    if (0 != data.readInt()) {
                        _arg22 = IQApp.CREATOR.createFromParcel(data);
                    } else {
                        _arg22 = null;
                    }
                    sendMessage(_arg03, _arg12, _arg22);
                    return true;
                case TRANSACTION_sendImage /* 8 */:
                    data.enforceInterface(DESCRIPTOR);
                    if (0 != data.readInt()) {
                        _arg02 = IQMessage.CREATOR.createFromParcel(data);
                    } else {
                        _arg02 = null;
                    }
                    if (0 != data.readInt()) {
                        _arg1 = IQDevice.CREATOR.createFromParcel(data);
                    } else {
                        _arg1 = null;
                    }
                    if (0 != data.readInt()) {
                        _arg2 = IQApp.CREATOR.createFromParcel(data);
                    } else {
                        _arg2 = null;
                    }
                    sendImage(_arg02, _arg1, _arg2);
                    return true;
                case 9:
                    data.enforceInterface(DESCRIPTOR);
                    if (0 != data.readInt()) {
                        _arg0 = IQApp.CREATOR.createFromParcel(data);
                    } else {
                        _arg0 = null;
                    }
                    String _arg15 = data.readString();
                    String _arg25 = data.readString();
                    registerApp(_arg0, _arg15, _arg25);
                    return true;
                case 1598968902:
                    reply.writeString(DESCRIPTOR);
                    return true;
                default:
                    return super.onTransact(code, data, reply, flags);
            }
        }

        /* JADX INFO: Access modifiers changed from: private */
        /* loaded from: classes.jar:com/garmin/android/apps/connectmobile/connectiq/IConnectIQService$Stub$Proxy.class */
        public static class Proxy implements IConnectIQService {
            private IBinder mRemote;
            public static IConnectIQService sDefaultImpl;

            Proxy(IBinder remote) {
                this.mRemote = remote;
            }

            @Override // android.os.IInterface
            public IBinder asBinder() {
                return this.mRemote;
            }

            public String getInterfaceDescriptor() {
                return Stub.DESCRIPTOR;
            }

            @Override // com.garmin.android.apps.connectmobile.connectiq.IConnectIQService
            public boolean openStore(String applicationID) throws RemoteException {
                Parcel _data = Parcel.obtain();
                Parcel _reply = Parcel.obtain();
                try {
                    _data.writeInterfaceToken(Stub.DESCRIPTOR);
                    _data.writeString(applicationID);
                    boolean _status = this.mRemote.transact(1, _data, _reply, 0);
                    if (_status || Stub.getDefaultImpl() == null) {
                        _reply.readException();
                        boolean _result = 0 != _reply.readInt();
                        return _result;
                    }
                    boolean openStore = Stub.getDefaultImpl().openStore(applicationID);
                    _reply.recycle();
                    _data.recycle();
                    return openStore;
                } finally {
                    _reply.recycle();
                    _data.recycle();
                }
            }

            @Override // com.garmin.android.apps.connectmobile.connectiq.IConnectIQService
            public List<IQDevice> getConnectedDevices() throws RemoteException {
                Parcel _data = Parcel.obtain();
                Parcel _reply = Parcel.obtain();
                try {
                    _data.writeInterfaceToken(Stub.DESCRIPTOR);
                    boolean _status = this.mRemote.transact(2, _data, _reply, 0);
                    if (_status || Stub.getDefaultImpl() == null) {
                        _reply.readException();
                        List<IQDevice> _result = _reply.createTypedArrayList(IQDevice.CREATOR);
                        _reply.recycle();
                        _data.recycle();
                        return _result;
                    }
                    List<IQDevice> connectedDevices = Stub.getDefaultImpl().getConnectedDevices();
                    _reply.recycle();
                    _data.recycle();
                    return connectedDevices;
                } catch (Throwable th) {
                    _reply.recycle();
                    _data.recycle();
                    throw th;
                }
            }

            @Override // com.garmin.android.apps.connectmobile.connectiq.IConnectIQService
            public List<IQDevice> getKnownDevices() throws RemoteException {
                Parcel _data = Parcel.obtain();
                Parcel _reply = Parcel.obtain();
                try {
                    _data.writeInterfaceToken(Stub.DESCRIPTOR);
                    boolean _status = this.mRemote.transact(3, _data, _reply, 0);
                    if (_status || Stub.getDefaultImpl() == null) {
                        _reply.readException();
                        List<IQDevice> _result = _reply.createTypedArrayList(IQDevice.CREATOR);
                        _reply.recycle();
                        _data.recycle();
                        return _result;
                    }
                    List<IQDevice> knownDevices = Stub.getDefaultImpl().getKnownDevices();
                    _reply.recycle();
                    _data.recycle();
                    return knownDevices;
                } catch (Throwable th) {
                    _reply.recycle();
                    _data.recycle();
                    throw th;
                }
            }

            @Override // com.garmin.android.apps.connectmobile.connectiq.IConnectIQService
            public int getStatus(IQDevice device) throws RemoteException {
                Parcel _data = Parcel.obtain();
                Parcel _reply = Parcel.obtain();
                try {
                    _data.writeInterfaceToken(Stub.DESCRIPTOR);
                    if (device != null) {
                        _data.writeInt(1);
                        device.writeToParcel(_data, 0);
                    } else {
                        _data.writeInt(0);
                    }
                    boolean _status = this.mRemote.transact(Stub.TRANSACTION_getStatus, _data, _reply, 0);
                    if (_status || Stub.getDefaultImpl() == null) {
                        _reply.readException();
                        int _result = _reply.readInt();
                        _reply.recycle();
                        _data.recycle();
                        return _result;
                    }
                    int status = Stub.getDefaultImpl().getStatus(device);
                    _reply.recycle();
                    _data.recycle();
                    return status;
                } catch (Throwable th) {
                    _reply.recycle();
                    _data.recycle();
                    throw th;
                }
            }

            @Override // com.garmin.android.apps.connectmobile.connectiq.IConnectIQService
            public void getApplicationInfo(String notificationPackage, String notificationAction, IQDevice device, String applicationID) throws RemoteException {
                Parcel _data = Parcel.obtain();
                try {
                    _data.writeInterfaceToken(Stub.DESCRIPTOR);
                    _data.writeString(notificationPackage);
                    _data.writeString(notificationAction);
                    if (device != null) {
                        _data.writeInt(1);
                        device.writeToParcel(_data, 0);
                    } else {
                        _data.writeInt(0);
                    }
                    _data.writeString(applicationID);
                    boolean _status = this.mRemote.transact(5, _data, null, 1);
                    if (_status || Stub.getDefaultImpl() == null) {
                        _data.recycle();
                        return;
                    }
                    Stub.getDefaultImpl().getApplicationInfo(notificationPackage, notificationAction, device, applicationID);
                    _data.recycle();
                } catch (Throwable th) {
                    _data.recycle();
                    throw th;
                }
            }

            @Override // com.garmin.android.apps.connectmobile.connectiq.IConnectIQService
            public void openApplication(String notificationPackage, String notificationAction, IQDevice device, IQApp app) throws RemoteException {
                Parcel _data = Parcel.obtain();
                try {
                    _data.writeInterfaceToken(Stub.DESCRIPTOR);
                    _data.writeString(notificationPackage);
                    _data.writeString(notificationAction);
                    if (device != null) {
                        _data.writeInt(1);
                        device.writeToParcel(_data, 0);
                    } else {
                        _data.writeInt(0);
                    }
                    if (app != null) {
                        _data.writeInt(1);
                        app.writeToParcel(_data, 0);
                    } else {
                        _data.writeInt(0);
                    }
                    boolean _status = this.mRemote.transact(Stub.TRANSACTION_openApplication, _data, null, 1);
                    if (_status || Stub.getDefaultImpl() == null) {
                        _data.recycle();
                        return;
                    }
                    Stub.getDefaultImpl().openApplication(notificationPackage, notificationAction, device, app);
                    _data.recycle();
                } catch (Throwable th) {
                    _data.recycle();
                    throw th;
                }
            }

            @Override // com.garmin.android.apps.connectmobile.connectiq.IConnectIQService
            public void sendMessage(IQMessage message, IQDevice device, IQApp app) throws RemoteException {
                Parcel _data = Parcel.obtain();
                try {
                    _data.writeInterfaceToken(Stub.DESCRIPTOR);
                    if (message != null) {
                        _data.writeInt(1);
                        message.writeToParcel(_data, 0);
                    } else {
                        _data.writeInt(0);
                    }
                    if (device != null) {
                        _data.writeInt(1);
                        device.writeToParcel(_data, 0);
                    } else {
                        _data.writeInt(0);
                    }
                    if (app != null) {
                        _data.writeInt(1);
                        app.writeToParcel(_data, 0);
                    } else {
                        _data.writeInt(0);
                    }
                    boolean _status = this.mRemote.transact(Stub.TRANSACTION_sendMessage, _data, null, 1);
                    if (_status || Stub.getDefaultImpl() == null) {
                        _data.recycle();
                        return;
                    }
                    Stub.getDefaultImpl().sendMessage(message, device, app);
                    _data.recycle();
                } catch (Throwable th) {
                    _data.recycle();
                    throw th;
                }
            }

            @Override // com.garmin.android.apps.connectmobile.connectiq.IConnectIQService
            public void sendImage(IQMessage image, IQDevice device, IQApp app) throws RemoteException {
                Parcel _data = Parcel.obtain();
                try {
                    _data.writeInterfaceToken(Stub.DESCRIPTOR);
                    if (image != null) {
                        _data.writeInt(1);
                        image.writeToParcel(_data, 0);
                    } else {
                        _data.writeInt(0);
                    }
                    if (device != null) {
                        _data.writeInt(1);
                        device.writeToParcel(_data, 0);
                    } else {
                        _data.writeInt(0);
                    }
                    if (app != null) {
                        _data.writeInt(1);
                        app.writeToParcel(_data, 0);
                    } else {
                        _data.writeInt(0);
                    }
                    boolean _status = this.mRemote.transact(Stub.TRANSACTION_sendImage, _data, null, 1);
                    if (_status || Stub.getDefaultImpl() == null) {
                        _data.recycle();
                        return;
                    }
                    Stub.getDefaultImpl().sendImage(image, device, app);
                    _data.recycle();
                } catch (Throwable th) {
                    _data.recycle();
                    throw th;
                }
            }

            @Override // com.garmin.android.apps.connectmobile.connectiq.IConnectIQService
            public void registerApp(IQApp app, String notificationAction, String notificationPackage) throws RemoteException {
                Parcel _data = Parcel.obtain();
                try {
                    _data.writeInterfaceToken(Stub.DESCRIPTOR);
                    if (app != null) {
                        _data.writeInt(1);
                        app.writeToParcel(_data, 0);
                    } else {
                        _data.writeInt(0);
                    }
                    _data.writeString(notificationAction);
                    _data.writeString(notificationPackage);
                    boolean _status = this.mRemote.transact(9, _data, null, 1);
                    if (_status || Stub.getDefaultImpl() == null) {
                        _data.recycle();
                        return;
                    }
                    Stub.getDefaultImpl().registerApp(app, notificationAction, notificationPackage);
                    _data.recycle();
                } catch (Throwable th) {
                    _data.recycle();
                    throw th;
                }
            }
        }

        public static boolean setDefaultImpl(IConnectIQService impl) {
            if (Proxy.sDefaultImpl != null) {
                throw new IllegalStateException("setDefaultImpl() called twice");
            } else if (impl == null) {
                return false;
            } else {
                Proxy.sDefaultImpl = impl;
                return true;
            }
        }

        public static IConnectIQService getDefaultImpl() {
            return Proxy.sDefaultImpl;
        }
    }
}
