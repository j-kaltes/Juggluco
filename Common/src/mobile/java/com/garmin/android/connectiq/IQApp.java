package com.garmin.android.connectiq;

import android.os.Parcel;
import android.os.Parcelable;

/* loaded from: classes.jar:com/garmin/android/connectiq/IQApp.class */
public class IQApp implements Parcelable {
    private String applicationID;
    private IQAppStatus status;
    private String displayName;
    private int version;
    public static final Parcelable.Creator<IQApp> CREATOR = new Parcelable.Creator<>() { // from class: com.garmin.android.connectiq.IQApp.1
        /* JADX WARN: Can't rename method to resolve collision */
        @Override // android.os.Parcelable.Creator
        public IQApp createFromParcel(Parcel in) {
            return new IQApp(in);
        }

        /* JADX WARN: Can't rename method to resolve collision */
        @Override // android.os.Parcelable.Creator
        public IQApp[] newArray(int size) {
            return new IQApp[size];
        }
    };

    /* loaded from: classes.jar:com/garmin/android/connectiq/IQApp$IQAppStatus.class */
    public enum IQAppStatus {
        UNKNOWN,
        INSTALLED,
        NOT_INSTALLED,
        NOT_SUPPORTED
    }

    public IQApp(String applicationID) {
        this.applicationID = applicationID.replaceAll("[\\s\\-]", "");
        this.status = IQAppStatus.UNKNOWN;
        this.displayName = "";
        this.version = 0;
    }

    public IQApp(String applicationID, String displayName, int version) {
        this(applicationID.replaceAll("[\\s\\-]", ""), IQAppStatus.UNKNOWN, displayName, version);
    }

    public IQApp(String applicationID, IQAppStatus status, String displayName, int version) {
        this.applicationID = applicationID.replaceAll("[\\s\\-]", "");
        this.status = status;
        this.displayName = displayName;
        this.version = version;
    }

    public IQApp(String applicationID, int version) {
        this(applicationID.replaceAll("[\\s\\-]", ""), IQAppStatus.INSTALLED, "", version);
    }

    public IQApp(Parcel in) {
        this.version = in.readInt();
        try {
            this.status = IQAppStatus.values()[in.readInt()];
        } catch (IndexOutOfBoundsException e) {
            this.status = IQAppStatus.UNKNOWN;
        }
        this.applicationID = in.readString();
        this.displayName = in.readString();
    }

    public String getApplicationId() {
        return this.applicationID;
    }

    public IQAppStatus getStatus() {
        return this.status;
    }

    public String getDisplayName() {
        return this.displayName;
    }

    public int version() {
        return this.version;
    }

    @Override // android.os.Parcelable
    public int describeContents() {
        return 0;
    }

    @Override // android.os.Parcelable
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeInt(this.version);
        dest.writeInt(this.status.ordinal());
        dest.writeString(this.applicationID);
        dest.writeString(this.displayName);
    }

    public String toString() {
        return this.displayName;
    }
}
