package com.garmin.monkeybrains.serialization;

import java.io.UnsupportedEncodingException;
import java.nio.BufferUnderflowException;
import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.List;
import java.util.Map;

/* loaded from: classes.jar:com/garmin/monkeybrains/serialization/SerializedObject.class */
public class SerializedObject {
    private static final int STRING_BLOCK_SENTINEL = -1412584499;
    private static final int DATA_BLOCK_SENTINEL = -629482886;
    private StringBlock mStringBlock;
    private DataBlock mDataBlock;

    public SerializedObject() {
        this.mStringBlock = new StringBlock();
        this.mDataBlock = new DataBlock();
    }

    public SerializedObject(byte[] bytes) throws UnsupportedEncodingException {
        int endOffset = 0;
        int stringBlockSize = getStringBlock(bytes);
        if (stringBlockSize > 0) {
            int begOffset = 0 + 8;
            endOffset = begOffset + stringBlockSize;
            if (endOffset > bytes.length) {
                throw new BufferUnderflowException();
            }
            this.mStringBlock = new StringBlock(Arrays.copyOfRange(bytes, begOffset, endOffset));
        }
        int begOffset2 = endOffset;
        int endOffset2 = begOffset2 + 8;
        if (endOffset2 > bytes.length) {
            throw new BufferUnderflowException();
        }
        ByteBuffer bb = ByteBuffer.wrap(bytes, begOffset2, 8);
        bb.getInt();
        int dataBlockSize = bb.getInt();
        int begOffset3 = begOffset2 + 8;
        int endOffset3 = begOffset3 + dataBlockSize;
        if (endOffset3 > bytes.length) {
            throw new BufferUnderflowException();
        }
        this.mDataBlock = new DataBlock(Arrays.copyOfRange(bytes, begOffset3, endOffset3), this.mStringBlock);
    }

    public void addObject(MonkeyType<?> type) throws UnsupportedEncodingException {
        this.mDataBlock.add(type);
        addToStrings(type);
    }

    private void addToStrings(MonkeyType<?> type) throws UnsupportedEncodingException {
        if (type instanceof MonkeyString) {
            int offset = this.mStringBlock.addString((MonkeyString) type);
            ((MonkeyString) type).setOffset(offset);
        } else if (type instanceof MonkeyArray) {
            for (MonkeyType<?> t : ((MonkeyArray) type).getValues()) {
                addToStrings(t);
            }
        } else if (type instanceof MonkeyHash) {
            for (Map.Entry<MonkeyType<?>, MonkeyType<?>> entry : ((MonkeyHash) type).getHashMap().entrySet()) {
                addToStrings(entry.getKey());
                addToStrings(entry.getValue());
            }
        }
    }

    public byte[] serialize() throws Exception {
        int size;
        byte[] stringBytes = this.mStringBlock.serialize();
        byte[] dataBytes = this.mDataBlock.serialize();
        if (stringBytes.length > 0) {
            size = 8 + stringBytes.length + 4 + 4 + dataBytes.length;
        } else {
            size = 8 + dataBytes.length;
        }
        ByteBuffer bb = ByteBuffer.allocate(size);
        if (stringBytes.length > 0) {
            bb.putInt(STRING_BLOCK_SENTINEL);
            bb.putInt(stringBytes.length);
            bb.put(stringBytes);
        }
        bb.putInt(DATA_BLOCK_SENTINEL);
        bb.putInt(dataBytes.length);
        bb.put(dataBytes);
        return bb.array();
    }

    private int getStringBlock(byte[] bytes) {
        ByteBuffer bb = ByteBuffer.wrap(bytes);
        if (bb.getInt() == STRING_BLOCK_SENTINEL) {
            return bb.getInt();
        }
        return 0;
    }

    public List<MonkeyType<?>> getValues() {
        return this.mDataBlock;
    }
}
