package com.garmin.monkeybrains.serialization;

import java.io.UnsupportedEncodingException;
import java.nio.ByteBuffer;

/* loaded from: classes.jar:com/garmin/monkeybrains/serialization/MonkeyString.class */
public class MonkeyString extends MonkeyType<String> {
    private int mOffset;
    private String mValue;

    public MonkeyString(String value) {
        super((byte) 3);
        this.mValue = value;
    }

    public MonkeyString(byte[] bytes) {
        super(bytes[0]);
        ByteBuffer bb = ByteBuffer.wrap(bytes, 1, bytes.length - 1);
        this.mOffset = bb.getInt();
    }

    @Override // com.garmin.monkeybrains.serialization.MonkeyType
    public int getNumBytes() {
        return 5;
    }

    @Override // com.garmin.monkeybrains.serialization.MonkeyType
    public byte[] serialize() {
        ByteBuffer bb = ByteBuffer.allocate(getNumBytes());
        bb.put((byte) 3);
        bb.putInt(this.mOffset);
        return bb.array();
    }

    public byte[] serializeString() throws UnsupportedEncodingException {
        ByteBuffer bb = ByteBuffer.allocate(2 + this.mValue.getBytes("UTF-8").length + 1);
        bb.putShort((short) (this.mValue.getBytes("UTF-8").length + 1));
        bb.put(this.mValue.getBytes("UTF-8"));
        bb.put((byte) 0);
        return bb.array();
    }

    /* JADX WARN: Can't rename method to resolve collision */
    @Override // com.garmin.monkeybrains.serialization.MonkeyType
    public String toJava() {
        return this.mValue;
    }

    public void setOffset(int offset) {
        this.mOffset = offset;
    }

    public String getValue() {
        return this.mValue;
    }

    public void setValue(String string) {
        this.mValue = string;
    }

    public int getOffset() {
        return this.mOffset;
    }

    @Override // com.garmin.monkeybrains.serialization.MonkeyType
    public int hashCode() {
        int hash = (89 * 5) + (this.mValue != null ? this.mValue.hashCode() : 0);
        return hash;
    }

    @Override // com.garmin.monkeybrains.serialization.MonkeyType
    public boolean equals(Object obj) {
        if (obj instanceof MonkeyString) {
            return ((MonkeyString) obj).getValue().equals(this.mValue);
        }
        return false;
    }
}
