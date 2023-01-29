package com.garmin.monkeybrains.serialization;

/* loaded from: classes.jar:com/garmin/monkeybrains/serialization/MonkeyNull.class */
public class MonkeyNull extends MonkeyType<Object> {
    public MonkeyNull() {
        super((byte) 0);
    }

    public MonkeyNull(Object obj) {
        super((byte) 0);
    }

    public MonkeyNull(byte[] bytes) {
        super((byte) 0);
    }

    @Override // com.garmin.monkeybrains.serialization.MonkeyType
    public int getNumBytes() {
        return 1;
    }

    @Override // com.garmin.monkeybrains.serialization.MonkeyType
    public byte[] serialize() {
        return new byte[]{0};
    }

    @Override // com.garmin.monkeybrains.serialization.MonkeyType
    public Object toJava() {
        return null;
    }
}
