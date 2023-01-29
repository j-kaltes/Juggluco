package com.garmin.android.connectiq;

import android.os.Parcel;
import android.os.Parcelable;

/* loaded from: classes.jar:com/garmin/android/connectiq/IQDevice.class */
public class IQDevice implements Parcelable {
    private long deviceIdentifier;
    private String friendlyName;
    private IQDeviceStatus status;
    public static final Parcelable.Creator<IQDevice> CREATOR = new Parcelable.Creator<IQDevice>() { // from class: com.garmin.android.connectiq.IQDevice.1
        /* JADX WARN: Can't rename method to resolve collision */
        @Override // android.os.Parcelable.Creator
        public IQDevice createFromParcel(Parcel in) {
            return new IQDevice(in);
        }

        /* JADX WARN: Can't rename method to resolve collision */
        @Override // android.os.Parcelable.Creator
        public IQDevice[] newArray(int size) {
            return new IQDevice[size];
        }
    };

    /* loaded from: classes.jar:com/garmin/android/connectiq/IQDevice$IQDeviceStatus.class */
    public enum IQDeviceStatus {
        NOT_PAIRED,
        NOT_CONNECTED,
        CONNECTED,
        UNKNOWN
    }

    public IQDevice(long deviceIdentifier, String friendlyName) {
        this.status = IQDeviceStatus.UNKNOWN;
        this.deviceIdentifier = deviceIdentifier;
        this.friendlyName = friendlyName;
    }

    public IQDevice(Parcel in) {
        this.status = IQDeviceStatus.UNKNOWN;
        this.deviceIdentifier = in.readLong();
        this.friendlyName = in.readString();
        try {
            this.status = IQDeviceStatus.valueOf(in.readString());
        } catch (IllegalArgumentException e) {
            this.status = IQDeviceStatus.UNKNOWN;
        }
    }

    public String getFriendlyName() {
        return this.friendlyName;
    }

    public long getDeviceIdentifier() {
        return this.deviceIdentifier;
    }

    public void setStatus(IQDeviceStatus status) {
        this.status = status;
    }

    public IQDeviceStatus getStatus() {
        return this.status;
    }

    @Override // android.os.Parcelable
    public int describeContents() {
        return 0;
    }

    @Override // android.os.Parcelable
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeLong(this.deviceIdentifier);
        dest.writeString(this.friendlyName);
        dest.writeString(this.status.name());
    }

    public String toString() {
        return this.friendlyName;
    }
}
