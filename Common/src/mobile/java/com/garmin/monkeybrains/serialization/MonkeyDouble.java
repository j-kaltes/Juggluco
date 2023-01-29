package com.garmin.monkeybrains.serialization;

import java.nio.ByteBuffer;

/* loaded from: classes.jar:com/garmin/monkeybrains/serialization/MonkeyDouble.class */
public class MonkeyDouble extends MonkeyType<Double> {
    private double mValue;

    public MonkeyDouble(double value) {
        super((byte) 15);
        this.mValue = value;
    }

    public MonkeyDouble(byte[] bytes) {
        super(bytes[0]);
        ByteBuffer bb = ByteBuffer.wrap(bytes, 1, bytes.length - 1);
        this.mValue = bb.getDouble();
    }

    @Override // com.garmin.monkeybrains.serialization.MonkeyType
    public int getNumBytes() {
        return 9;
    }

    @Override // com.garmin.monkeybrains.serialization.MonkeyType
    public byte[] serialize() {
        ByteBuffer bb = ByteBuffer.allocate(getNumBytes());
        bb.put((byte) 15);
        bb.putDouble(this.mValue);
        return bb.array();
    }

    /* JADX WARN: Can't rename method to resolve collision */
    @Override // com.garmin.monkeybrains.serialization.MonkeyType
    public Double toJava() {
        return Double.valueOf(this.mValue);
    }
}
