package tk.glucodata.NovoPen.opennov;

import tk.glucodata.NovoPen.opennov.buffer.MyByteBuffer;

import java.nio.ByteBuffer;

import tk.glucodata.Log;


/**
 * JamOrHam
 * OpenNov base message helper
 */

public class BaseMessage extends MyByteBuffer {

    private static final String TAG = "OpenNov";
    public static boolean d = true;


    protected static byte[] getIndexedBytes(final ByteBuffer buffer) {
       var blen = getUnsignedShort(buffer);
        var bytes = new byte[blen];
        buffer.get(bytes, 0, blen);
        return bytes;
    }

    protected static void putIndexedBytes(final ByteBuffer buffer, final byte[] bytes) {
        putUnsignedShort(buffer, bytes != null ? bytes.length : 0);
        if (bytes != null) buffer.put(bytes);
    }

    protected static String getIndexedString(final ByteBuffer buffer) {
        return new String(getIndexedBytes(buffer)).replace("\0", "");
    }

    protected static long getIvalue(final byte[] bytes, final int len) {
        var buffer = ByteBuffer.wrap(bytes);
        switch (len) {
            case 4:
                return getUnsignedInt(buffer);
            case 2:
                return getUnsignedShort(buffer);
            default:
                return -1;
        }
    }

    public String toJson() {
        return this.toString();
    }

    protected static String getTAG() {
        var fullClassName = Thread.currentThread().getStackTrace()[3].getClassName();
        return fullClassName.substring(fullClassName.lastIndexOf('.') + 1);
    }

    public static void log(final String msg) {
        if (d) {
                Log.d(TAG, msg);
            }
    }

    protected static void error(final String msg) {
        if (d) {
            System.out.println(getTAG() + ":: " + msg);
        } else {
            Log.e(TAG, msg);
        }
    }

}
