package com.garmin.monkeybrains.serialization;

import java.nio.ByteBuffer;

/* loaded from: classes.jar:com/garmin/monkeybrains/serialization/MonkeyFloat.class */
public class MonkeyFloat extends MonkeyType<Float> {
    private float mValue;

    public MonkeyFloat(float value) {
        super((byte) 2);
        this.mValue = value;
    }

    public MonkeyFloat(byte[] bytes) {
        super(bytes[0]);
        ByteBuffer bb = ByteBuffer.wrap(bytes, 1, bytes.length - 1);
        this.mValue = bb.getFloat();
    }

    @Override // com.garmin.monkeybrains.serialization.MonkeyType
    public int getNumBytes() {
        return 5;
    }

    @Override // com.garmin.monkeybrains.serialization.MonkeyType
    public byte[] serialize() {
        ByteBuffer bb = ByteBuffer.allocate(getNumBytes());
        bb.put((byte) 2);
        bb.putFloat(this.mValue);
        return bb.array();
    }

    /* JADX WARN: Can't rename method to resolve collision */
    @Override // com.garmin.monkeybrains.serialization.MonkeyType
    public Float toJava() {
        return Float.valueOf(this.mValue);
    }
}
