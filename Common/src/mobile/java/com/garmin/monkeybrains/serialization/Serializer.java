package com.garmin.monkeybrains.serialization;

import java.io.UnsupportedEncodingException;
import java.util.List;

/* loaded from: classes.jar:com/garmin/monkeybrains/serialization/Serializer.class */
public class Serializer {
    private SerializedObject mSerializedObject;

    public static List<MonkeyType<?>> deserialize(byte[] bytes) throws UnsupportedEncodingException {
        SerializedObject obj = new SerializedObject(bytes);
        return obj.getValues();
    }

    public static <T> byte[] serialize(T obj) throws Exception {
        Serializer s = new Serializer(obj);
        return s.serialize();
    }

    public Serializer() {
        this.mSerializedObject = new SerializedObject();
    }

    public Serializer(Object obj) throws UnsupportedEncodingException {
        this();
        addObject(obj);
    }

    public <T> void addObject(T obj) throws UnsupportedEncodingException {
        MonkeyType<?> t = MonkeyType.fromJava(obj);
        this.mSerializedObject.addObject(t);
    }

    public byte[] serialize() throws Exception {
        return this.mSerializedObject.serialize();
    }
}
