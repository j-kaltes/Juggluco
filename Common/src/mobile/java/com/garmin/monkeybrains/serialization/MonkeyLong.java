package com.garmin.monkeybrains.serialization;

import java.nio.ByteBuffer;

/* loaded from: classes.jar:com/garmin/monkeybrains/serialization/MonkeyLong.class */
public class MonkeyLong extends MonkeyType<Long> {
    private long mValue;

    public MonkeyLong(long value) {
        super((byte) 14);
        this.mValue = value;
    }

    public MonkeyLong(byte[] bytes) {
        super(bytes[0]);
        ByteBuffer bb = ByteBuffer.wrap(bytes, 1, bytes.length - 1);
        this.mValue = bb.getLong();
    }

    @Override // com.garmin.monkeybrains.serialization.MonkeyType
    public int getNumBytes() {
        return 9;
    }

    @Override // com.garmin.monkeybrains.serialization.MonkeyType
    public byte[] serialize() {
        ByteBuffer bb = ByteBuffer.allocate(getNumBytes());
        bb.put((byte) 14);
        bb.putLong(this.mValue);
        return bb.array();
    }

    /* JADX WARN: Can't rename method to resolve collision */
    @Override // com.garmin.monkeybrains.serialization.MonkeyType
    public Long toJava() {
        return Long.valueOf(this.mValue);
    }
}
