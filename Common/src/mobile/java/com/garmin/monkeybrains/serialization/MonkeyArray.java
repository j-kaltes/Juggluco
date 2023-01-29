package com.garmin.monkeybrains.serialization;

import java.io.UnsupportedEncodingException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;

/* loaded from: classes.jar:com/garmin/monkeybrains/serialization/MonkeyArray.class */
public class MonkeyArray extends MonkeyType<List<MonkeyType<?>>> implements MonkeyContainer {
    private List<MonkeyType<?>> mList = new ArrayList();
    private int mChildCount;

    public <T> MonkeyArray(List<T> list) {
        super((byte) 5);
        for (T obj : list) {
            this.mList.add(MonkeyType.fromJava(obj));
        }
    }

    public MonkeyArray(byte[] bytes) throws UnsupportedEncodingException {
        super(bytes[0]);
        this.mChildCount = ByteBuffer.wrap(bytes, 1, 4).getInt();
    }

    @Override // com.garmin.monkeybrains.serialization.MonkeyType
    public int getNumBytes() {
        int numBytes = 5;
        for (MonkeyType<?> type : this.mList) {
            numBytes += type.getNumBytes();
        }
        return numBytes;
    }

    @Override // com.garmin.monkeybrains.serialization.MonkeyType
    public byte[] serialize() {
        ByteBuffer bb = ByteBuffer.allocate(5);
        bb.put((byte) 5);
        bb.putInt(this.mList.size());
        return bb.array();
    }

    /* JADX WARN: Can't rename method to resolve collision */
    @Override // com.garmin.monkeybrains.serialization.MonkeyType
    public List<MonkeyType<?>> toJava() {
        return this.mList;
    }

    public List<MonkeyType<?>> getValues() {
        return this.mList;
    }

    @Override // com.garmin.monkeybrains.serialization.MonkeyContainer
    public List<MonkeyType<?>> getChildren() {
        return this.mList;
    }

    @Override // com.garmin.monkeybrains.serialization.MonkeyContainer
    public int getChildCount() {
        return this.mChildCount;
    }
}
