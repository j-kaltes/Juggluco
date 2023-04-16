package tk.glucodata.NovoPen.opennov.ll;

import static java.lang.System.arraycopy;


import tk.glucodata.NovoPen.opennov.buffer.MyByteBuffer;

import java.nio.ByteBuffer;

import tk.glucodata.Log;


/**
 * JamOrHam
 * OpenNov type 4 reply
 */

public class T4Reply extends MyByteBuffer {
static final String TAG="OpenNov";
    private static final int COMMAND_COMPLETED = 0x9000;

    public byte[] bytes;
    int lastShort;

    public boolean isOkay() {
        return lastShort == COMMAND_COMPLETED;
    }

    public int asInteger() {
        if (bytes != null) {
            var b = ByteBuffer.wrap(bytes);
            if (bytes.length == 2) {
                return getUnsignedShort(b);
            } else if (bytes.length == 4) {
                return b.getInt();
            } else {
                return -1;
            }
        } else {
            return -1;
        }
    }
    public static T4Reply parse(final byte[] bytes, T4Reply r) {
        if (bytes == null) return null;
        var blen = bytes.length - 2;
        if (blen < 0) return null;
        if (r == null) {
            r = new T4Reply();
        }
        r.lastShort = ((0xFFFF & ((bytes[blen]&0xFF) << 8))) | (0xFF & bytes[blen + 1]);
        if(!r.isOkay())
                return r;
        if (blen > 0) {
            int xlen = 0;
            if (r.bytes != null) {
                final byte[] holding = r.bytes;
                xlen = holding.length;
                r.bytes = new byte[blen + xlen];
                arraycopy(holding, 0, r.bytes, 0, xlen);
            } else {
                r.bytes = new byte[blen];
            }
            arraycopy(bytes,0,r.bytes,xlen,blen);
//            b.get(r.bytes, xlen, blen);
        }
 //       r.lastShort = getUnsignedShort(b);
        return r;
    }

/*
    public static T4Reply parse(final byte[] bytes, T4Reply r) {
        if (bytes == null) return null;
        var b = ByteBuffer.wrap(bytes);
        if (r == null) {
            r = new T4Reply();
        }
        var blen = b.remaining() - 2;
        if (blen < 0) return null;
        if (blen > 0) {
            int xlen = 0;
            if (r.bytes != null) {
                final byte[] holding = r.bytes;
                xlen = holding.length;
                r.bytes = new byte[blen + xlen];
                System.arraycopy(holding, 0, r.bytes, 0, xlen);
            } else {
                r.bytes = new byte[blen];
            }
            b.get(r.bytes, xlen, blen);
        }
        r.lastShort = getUnsignedShort(b);
        return r;
    } */

    public static T4Reply parse(final byte[] bytes) {
        return parse(bytes, null);
    }

}
