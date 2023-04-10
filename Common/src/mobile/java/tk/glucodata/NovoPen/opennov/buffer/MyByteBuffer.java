package tk.glucodata.NovoPen.opennov.buffer;


import java.nio.ByteBuffer;

/**
 * JamOrHam
 * Base buffer helper
 */

public class MyByteBuffer {

    public static int getUnsignedByte(final ByteBuffer data) {
        return data.get() & 0xff;
    }

    public static void putUnsignedByte(final ByteBuffer data, int s) {
        data.put((byte) (s & 0xff));
    }

    public static int getUnsignedShort(final ByteBuffer data) {
        return data.getShort() & 0xffff;
    }

    public static void putUnsignedShort(final ByteBuffer data, int s) {
        data.putShort((short) (s & 0xffff));
    }

    public static long getUnsignedInt(final ByteBuffer data) {
        return data.getInt() & 0xffffffffL;
    }

    public static void putUnsignedInt(final ByteBuffer data, long s) {
        data.putInt((int) (s & 0xffffffff));
    }

    public static byte[] getBytes(final ByteBuffer data, int len) {
        final byte[] bytes = new byte[len];
        data.get(bytes);
        return bytes;
    }
    private static byte[] reverseBytes(byte[] source) {
        byte[] dest = new byte[source.length];
        for (int i = 0; i < source.length; i++) {
            dest[(source.length - i) - 1] = source[i];
        }
        return dest;
    }

    public static MyBitSet getBits(final ByteBuffer data, int byteLen, boolean reverse) {
        byte[] bytes = new byte[byteLen];
        data.get(bytes);
        if (reverse) {
            bytes = reverseBytes(bytes);
        }
        return MyBitSet.valueOf(bytes);
    }

    public static MyBitSet getBits(final ByteBuffer data, int byteLen) {
        return getBits(data, byteLen, false);
    }

}
