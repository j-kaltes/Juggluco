package com.garmin.monkeybrains.serialization;

import java.io.UnsupportedEncodingException;
import java.util.List;
import java.util.Map;

/* loaded from: classes.jar:com/garmin/monkeybrains/serialization/MonkeyType.class */
public abstract class MonkeyType<T> {
    public static final byte NULL = 0;
    public static final byte INT = 1;
    public static final byte FLOAT = 2;
    public static final byte STRING = 3;
    public static final byte ARRAY = 5;
    public static final byte BOOLEAN = 9;
    public static final byte HASH = 11;
    public static final byte LONG = 14;
    public static final byte DOUBLE = 15;
    public static final byte CHAR = 19;
    public byte mType;

    public abstract int getNumBytes();

    public abstract byte[] serialize();

    public abstract T toJava();

    public static MonkeyType<?> deserialize(byte[] bytes) throws UnsupportedEncodingException {
        switch (bytes[0]) {
            case 0:
                return new MonkeyNull();
            case 1:
                return new MonkeyInt(bytes);
            case FLOAT /* 2 */:
                return new MonkeyFloat(bytes);
            case STRING /* 3 */:
                return new MonkeyString(bytes);
            case 4:
            case 6:
            case 7:
            case 8:
            case 10:
            case 12:
            case 13:
            case 16:
            case 17:
            case 18:
            default:
                return null;
            case ARRAY /* 5 */:
                return new MonkeyArray(bytes);
            case BOOLEAN /* 9 */:
                return new MonkeyBool(bytes);
            case HASH /* 11 */:
                return new MonkeyHash(bytes);
            case LONG /* 14 */:
                return new MonkeyLong(bytes);
            case DOUBLE /* 15 */:
                return new MonkeyDouble(bytes);
            case CHAR /* 19 */:
                return new MonkeyChar(bytes);
        }
    }

    public static <T> MonkeyType<?> fromJava(T obj) {
        if (obj == null) {
            return new MonkeyNull((byte[]) null);
        }
        if (obj instanceof MonkeyType) {
            return (MonkeyType) obj;
        }
        if (obj instanceof Integer) {
            return new MonkeyInt(((Integer) obj).intValue());
        }
        if (obj instanceof Long) {
            if (((Number) obj).longValue() > 2147483647L || ((Number) obj).longValue() < -2147483648L) {
                return new MonkeyLong(((Long) obj).longValue());
            }
            return new MonkeyInt(((Number) obj).intValue());
        } else if (obj instanceof Float) {
            return new MonkeyFloat(((Float) obj).floatValue());
        } else {
            if (obj instanceof Double) {
                if (Math.abs(((Number) obj).floatValue() - ((Number) obj).doubleValue()) < 1.0E-5d) {
                    return new MonkeyFloat(((Number) obj).floatValue());
                }
                return new MonkeyDouble(((Double) obj).doubleValue());
            } else if (obj instanceof Long) {
                return new MonkeyInt(((Long) obj).intValue());
            } else {
                if (obj instanceof String) {
                    return new MonkeyString((String) obj);
                }
                if (obj instanceof List) {
                    return new MonkeyArray((List) obj);
                }
                if (obj instanceof Boolean) {
                    return new MonkeyBool(((Boolean) obj).booleanValue());
                }
                if (obj instanceof Map) {
                    return new MonkeyHash((Map) obj);
                }
                return new MonkeyNull((byte[]) null);
            }
        }
    }

    public MonkeyType(byte type) {
        this.mType = type;
    }

    public boolean equals(Object obj) {
        if (!(obj instanceof MonkeyType)) {
            return false;
        }
        if (obj instanceof MonkeyNull) {
            return this instanceof MonkeyNull;
        }
        return toJava().equals(((MonkeyType) obj).toJava());
    }

    public int hashCode() {
        return toJava().hashCode();
    }
}
