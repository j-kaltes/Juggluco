package com.garmin.monkeybrains.serialization;

import java.nio.ByteBuffer;

/* loaded from: classes.jar:com/garmin/monkeybrains/serialization/MonkeyInt.class */
public class MonkeyInt extends MonkeyType<Integer> {
    private int mValue;

    public MonkeyInt(int value) {
        super((byte) 1);
        this.mValue = value;
    }

    public MonkeyInt(byte[] bytes) {
        super(bytes[0]);
        ByteBuffer bb = ByteBuffer.wrap(bytes, 1, bytes.length - 1);
        this.mValue = bb.getInt();
    }

    @Override // com.garmin.monkeybrains.serialization.MonkeyType
    public int getNumBytes() {
        return 5;
    }

    @Override // com.garmin.monkeybrains.serialization.MonkeyType
    public byte[] serialize() {
        ByteBuffer bb = ByteBuffer.allocate(getNumBytes());
        bb.put(this.mType);
        bb.putInt(this.mValue);
        return bb.array();
    }

    /* JADX WARN: Can't rename method to resolve collision */
    @Override // com.garmin.monkeybrains.serialization.MonkeyType
    public Integer toJava() {
        return Integer.valueOf(this.mValue);
    }
}
