package com.garmin.monkeybrains.serialization;

import java.io.UnsupportedEncodingException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;

/* loaded from: classes.jar:com/garmin/monkeybrains/serialization/StringBlock.class */
public class StringBlock extends ArrayList<MonkeyString> {
    private static final long serialVersionUID = 7158590947509522494L;
    private int mTotalBytes;
    private HashMap<Integer, String> mDeserializedStrings;

    public StringBlock() {
    }

    public StringBlock(byte[] bytes) throws UnsupportedEncodingException {
        this.mDeserializedStrings = new HashMap<>();
        int size = bytes.length;
        int i = 0;
        while (i < size) {
            int offset = i;
            int stringSize = ByteBuffer.wrap(bytes, i, 2).getShort();
            int i2 = i + 2;
            byte[] stringBytes = Arrays.copyOfRange(bytes, i2, (i2 + stringSize) - 1);
            i = i2 + stringSize;
            this.mDeserializedStrings.put(Integer.valueOf(offset), new String(stringBytes, "UTF-8"));
        }
    }

    public int addString(MonkeyString type) throws UnsupportedEncodingException {
        int offset = getOffsetFor(type);
        type.setOffset(offset);
        if (!contains(type)) {
            add(type);
            this.mTotalBytes += 2 + type.getValue().getBytes("UTF-8").length + 1;
        }
        return offset;
    }

    private int getOffsetFor(MonkeyString aString) throws UnsupportedEncodingException {
        int offset = 0;
        for (int i = 0; i < size() && !get(i).equals(aString); i++) {
            offset += 2 + get(i).getValue().getBytes("UTF-8").length + 1;
        }
        return offset;
    }

    public byte[] serialize() throws UnsupportedEncodingException {
        ByteBuffer bb = ByteBuffer.allocate(this.mTotalBytes);
        for (int i = 0; i < size(); i++) {
            MonkeyString s = get(i);
            bb.put(s.serializeString());
        }
        return bb.array();
    }

    public HashMap<Integer, String> getDeserializedStrings() {
        return this.mDeserializedStrings;
    }
}
