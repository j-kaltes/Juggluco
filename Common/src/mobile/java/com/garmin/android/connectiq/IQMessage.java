package com.garmin.android.connectiq;

import android.os.Parcel;
import android.os.Parcelable;

/* loaded from: classes.jar:com/garmin/android/connectiq/IQMessage.class */
public class IQMessage implements Parcelable {
    public byte[] messageData;
    public String notificationPackage;
    public String notificationAction;
    public static final Parcelable.Creator<IQMessage> CREATOR = new Parcelable.Creator<>() { // from class: com.garmin.android.connectiq.IQMessage.1
        /* JADX WARN: Can't rename method to resolve collision */
        @Override // android.os.Parcelable.Creator
        public IQMessage createFromParcel(Parcel in) {
            return new IQMessage(in);
        }

        /* JADX WARN: Can't rename method to resolve collision */
        @Override // android.os.Parcelable.Creator
        public IQMessage[] newArray(int size) {
            return new IQMessage[size];
        }
    };

    public IQMessage(byte[] messageData, String notificationPackage, String notificationAction) {
        this.messageData = new byte[messageData.length];
        System.arraycopy(messageData, 0, this.messageData, 0, messageData.length);
        this.notificationPackage = notificationPackage;
        this.notificationAction = notificationAction;
    }

    public IQMessage(Parcel in) {
        int messageLength = in.readInt();
        if (messageLength > 0) {
            this.messageData = in.createByteArray();
        }
        this.notificationPackage = in.readString();
        this.notificationAction = in.readString();
    }

    @Override // android.os.Parcelable
    public int describeContents() {
        return 0;
    }

    @Override // android.os.Parcelable
    public void writeToParcel(Parcel dest, int flags) {
        if (this.messageData != null) {
            int length = this.messageData.length;
            dest.writeInt(length);
            dest.writeByteArray(this.messageData);
        } else {
            dest.writeInt(0);
        }
        dest.writeString(this.notificationPackage);
        dest.writeString(this.notificationAction);
    }
}
