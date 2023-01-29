package com.garmin.monkeybrains.serialization;

import java.nio.ByteBuffer;

/* loaded from: classes.jar:com/garmin/monkeybrains/serialization/MonkeyBool.class */
public class MonkeyBool extends MonkeyType<Boolean> {
    private boolean mValue;

    public MonkeyBool(boolean bool) {
        super((byte) 9);
        this.mValue = bool;
    }

    public MonkeyBool(byte[] bytes) {
        super(bytes[0]);
        ByteBuffer bb = ByteBuffer.wrap(bytes, 1, bytes.length - 1);
        this.mValue = bb.get() > 0;
    }

    @Override // com.garmin.monkeybrains.serialization.MonkeyType
    public int getNumBytes() {
        return 2;
    }

    @Override // com.garmin.monkeybrains.serialization.MonkeyType
    public byte[] serialize() {
        byte[] bArr = new byte[2];
        bArr[0] = 9;
        bArr[1] = (byte) (this.mValue ? 1 : 0);
        return bArr;
    }

    /* JADX WARN: Can't rename method to resolve collision */
    @Override // com.garmin.monkeybrains.serialization.MonkeyType
    public Boolean toJava() {
        return Boolean.valueOf(this.mValue);
    }
}
