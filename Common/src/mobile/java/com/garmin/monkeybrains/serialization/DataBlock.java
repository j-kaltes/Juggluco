package com.garmin.monkeybrains.serialization;

import java.io.UnsupportedEncodingException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.Map;

/* loaded from: classes.jar:com/garmin/monkeybrains/serialization/DataBlock.class */
public class DataBlock extends ArrayList<MonkeyType<?>> {
    private static final long serialVersionUID = -8058543222042601383L;
    private int mTotalBytes;
    private LinkedList<MonkeyType<?>> mSerialQueue;
    private LinkedList<MonkeyType<?>> mDeserialQueue;
    private LinkedList<MonkeyType<?>> mComplexDeserialQueue;

    public DataBlock() {
        this.mSerialQueue = new LinkedList<>();
    }

    public DataBlock(byte[] bytes, StringBlock stringBlock) throws UnsupportedEncodingException {
        this.mDeserialQueue = new LinkedList<>();
        this.mComplexDeserialQueue = new LinkedList<>();
        int i = 0;
        while (true) {
            int i2 = i;
            if (i2 < bytes.length) {
                byte[] bs = Arrays.copyOfRange(bytes, i2, bytes.length);
                MonkeyType<?> type = MonkeyType.deserialize(bs);
                if (type == null) {
                    throw new UnsupportedEncodingException("Failed to deserialize MonkeyC objects");
                }
                this.mDeserialQueue.add(type);
                if (stringBlock != null && (type instanceof MonkeyString)) {
                    attachStrings(type, stringBlock);
                }
                i = i2 + type.getNumBytes();
            } else {
                processDeserializeQueue();
                return;
            }
        }
    }

    private void processDeserializeQueue() {
        MonkeyType<?> type;
        do {
            type = this.mDeserialQueue.poll();
            if (type != null) {
                if (type instanceof MonkeyArray) {
                    processArray((MonkeyArray) type);
                } else if (type instanceof MonkeyHash) {
                    processHash((MonkeyHash) type);
                }
                add(type);
            }
        } while (type != null);
    }

    private void processComplexDeserializeQueue() {
        MonkeyType<?> type;
        do {
            type = this.mComplexDeserialQueue.poll();
            if (type != null) {
                if (type instanceof MonkeyArray) {
                    processArray((MonkeyArray) type);
                } else if (type instanceof MonkeyHash) {
                    processHash((MonkeyHash) type);
                }
            }
        } while (type != null);
    }

    private void queueComplex(MonkeyType<?> type) {
        this.mComplexDeserialQueue.add(type);
    }

    private void processArray(MonkeyArray array) {
        for (int i = 0; i < array.getChildCount(); i++) {
            MonkeyType<?> value = this.mDeserialQueue.poll();
            if (value instanceof MonkeyContainer) {
                queueComplex(value);
            }
            array.getChildren().add(value);
        }
        processComplexDeserializeQueue();
    }

    private void processHash(MonkeyHash hash) {
        for (int i = 0; i < hash.getChildCount(); i++) {
            MonkeyType<?> key = this.mDeserialQueue.poll();
            if (key instanceof MonkeyContainer) {
                queueComplex(key);
            }
            MonkeyType<?> value = this.mDeserialQueue.poll();
            if (value instanceof MonkeyContainer) {
                queueComplex(value);
            }
            hash.getHashMap().put(key, value);
        }
        processComplexDeserializeQueue();
    }

    public boolean add(MonkeyType<?> type) {
        boolean changed = super.add( type);
        if (changed) {
            try {
                this.mTotalBytes += type.getNumBytes();
            } catch (Exception e) {
            }
        }
        return changed;
    }

    public byte[] serialize() throws Exception {
        MonkeyType<?> type;
        this.mSerialQueue.clear();
        this.mSerialQueue.addAll(this);
        ByteBuffer bb = ByteBuffer.allocate(this.mTotalBytes);
        do {
            type = this.mSerialQueue.poll();
            if (type != null) {
                bb.put(type.serialize());
                if (type instanceof MonkeyContainer) {
                    this.mSerialQueue.addAll(((MonkeyContainer) type).getChildren());
                }
            }
        } while (type != null);
        return bb.array();
    }

    private void attachStrings(MonkeyType<?> type, StringBlock stringBlock) {
        if (type instanceof MonkeyString) {
            HashMap<Integer, String> map = stringBlock.getDeserializedStrings();
            ((MonkeyString) type).setValue(map.get(Integer.valueOf(((MonkeyString) type).getOffset())));
        } else if (type instanceof MonkeyArray) {
            for (MonkeyType<?> t : ((MonkeyArray) type).getValues()) {
                attachStrings(t, stringBlock);
            }
        } else if (type instanceof MonkeyHash) {
            for (Map.Entry<MonkeyType<?>, MonkeyType<?>> entry : ((MonkeyHash) type).getHashMap().entrySet()) {
                attachStrings(entry.getKey(), stringBlock);
                attachStrings(entry.getValue(), stringBlock);
            }
        }
    }
}
