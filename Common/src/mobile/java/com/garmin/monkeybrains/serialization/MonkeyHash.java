package com.garmin.monkeybrains.serialization;

import java.io.UnsupportedEncodingException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/* loaded from: classes.jar:com/garmin/monkeybrains/serialization/MonkeyHash.class */
public class MonkeyHash extends MonkeyType<HashMap<MonkeyType<?>, MonkeyType<?>>> implements MonkeyContainer {
    private HashMap<MonkeyType<?>, MonkeyType<?>> mHash = new HashMap<>();
    private int mHashSize;

    public MonkeyHash(Map<Object, Object> hash) {
        super((byte) 11);
        for (Map.Entry<Object, Object> entry : hash.entrySet()) {
            Object k = entry.getKey();
            Object v = entry.getValue();
            MonkeyType<?> key = MonkeyType.fromJava(k);
            MonkeyType<?> value = MonkeyType.fromJava(v);
            this.mHash.put(key, value);
        }
    }

    public MonkeyHash(byte[] bytes) throws UnsupportedEncodingException {
        super(bytes[0]);
        this.mHashSize = ByteBuffer.wrap(bytes, 1, 4).getInt();
    }

    @Override // com.garmin.monkeybrains.serialization.MonkeyType
    public int getNumBytes() {
        int byteCount = 5;
        for (Map.Entry<MonkeyType<?>, MonkeyType<?>> entry : this.mHash.entrySet()) {
            byteCount += entry.getKey().getNumBytes() + entry.getValue().getNumBytes();
        }
        return byteCount;
    }

    @Override // com.garmin.monkeybrains.serialization.MonkeyType
    public byte[] serialize() {
        ByteBuffer bb = ByteBuffer.allocate(5);
        bb.put((byte) 11);
        bb.putInt(this.mHash.size());
        return bb.array();
    }

    /* JADX WARN: Can't rename method to resolve collision */
    @Override // com.garmin.monkeybrains.serialization.MonkeyType
    public HashMap<MonkeyType<?>, MonkeyType<?>> toJava() {
        return this.mHash;
    }

    public HashMap<MonkeyType<?>, MonkeyType<?>> getHashMap() {
        return this.mHash;
    }

    @Override // com.garmin.monkeybrains.serialization.MonkeyContainer
    public List<MonkeyType<?>> getChildren() {
        List<MonkeyType<?>> children = new ArrayList<>();
        for (Map.Entry<MonkeyType<?>, MonkeyType<?>> entry : this.mHash.entrySet()) {
            children.add(entry.getKey());
            children.add(entry.getValue());
        }
        return children;
    }

    @Override // com.garmin.monkeybrains.serialization.MonkeyContainer
    public int getChildCount() {
        return this.mHashSize;
    }
}
