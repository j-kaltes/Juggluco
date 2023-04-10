package tk.glucodata.NovoPen.opennov.ll;

import tk.glucodata.NovoPen.opennov.buffer.MyByteBuffer;
import java.nio.ByteBuffer;
import java.util.Arrays;

/**
 * JamOrHam
 * OpenNov link layer codec
 */
public class PHDll extends MyByteBuffer {
    private static final int MB = 1 << 7;
    private static final int ME = 1 << 6;
    private static final int CF = 1 << 5;
    private static final int SR = 1 << 4;
    private static final int IL = 1 << 3;
    private static final int WELL_KNOWN = 1;
    private static final byte[] typeID = "PHD".getBytes();
    private static final int defaultOpcode = 209;
    int opcode=-1;
    int typeLen=-1;
    int payloadLen=-1;
    int idHeaderLen=-1;
    int seq=-1;
    int chk=0;
    byte[] idHeader;
    byte[] inner;

    public PHDll() {
    }

    public boolean checkBit(final int bit) {
        return (((chk >> bit) & 1) != 0);
    }

    public void setCheckBit(final int bit) {
        chk |= (1 << bit);
    }

    public byte[] encode() {
        var ilen = inner != null ? inner.length : 0;
        var idLen = idHeader != null ? idHeader.length : 0;
        var hasId = idLen > 0;
        var b = ByteBuffer.allocate(ilen + 7);
        putUnsignedByte(b, MB | ME | SR | (hasId ? IL : 0) | WELL_KNOWN);
        putUnsignedByte(b, typeID.length);
        putUnsignedByte(b, ilen + 1);
        if (hasId) {
            b.put(idHeader);
        }
        b.put(typeID);
        putUnsignedByte(b, (seq & 15) | 128 | chk);
        if (ilen > 0) {
            b.put(inner);
        }
        return b.array();
    }

    public static PHDll parse(final byte[] bytes) {
        if (bytes == null) return null;
        var b = ByteBuffer.wrap(bytes);
        var phd = new PHDll();
        phd.opcode = getUnsignedByte(b);
        var hasID = (phd.opcode & IL) != 0;
        phd.typeLen = getUnsignedByte(b);
        phd.payloadLen = getUnsignedByte(b) - 1;
        if (hasID) {
            phd.idHeaderLen = getUnsignedByte(b);
        }
        var pprotoId = new byte[3];
        b.get(pprotoId);
        if (!Arrays.equals(pprotoId, typeID)) {
            return null;
        }
        if (hasID) {
            phd.idHeader = new byte[phd.idHeaderLen];
            b.get(phd.idHeader);
        }
        phd.chk = getUnsignedByte(b);
        phd.seq = phd.chk & 15;
        phd.inner = new byte[phd.payloadLen];
        b.get(phd.inner);
        return phd;
    }

/*
    private static int $default$opcode() { return -1; }
    private static int $default$typeLen() {
        return -1;
    }

    @SuppressWarnings("all")
    private static int $default$payloadLen() {
        return -1;
    }

    @SuppressWarnings("all")
    private static int $default$idHeaderLen() {
        return -1;
    }

    @SuppressWarnings("all")
    private static int $default$seq() {
        return -1;
    }

    @SuppressWarnings("all")
    private static int $default$chk() {
        return 0;
    }
*/


        public PHDll opcode(final int opcode) {
            this.opcode = opcode;
            return this;
        }

        public PHDll typeLen(final int typeLen) {
            this.typeLen = typeLen;
            return this;
        }

        public PHDll payloadLen(final int payloadLen) {
            this.payloadLen = payloadLen;
            return this;
        }

        public PHDll idHeaderLen(final int idHeaderLen) {
            this.idHeaderLen = idHeaderLen;
            return this;
        }

        public PHDll seq(final int seq) {
            this.seq = seq;
            return this;
        }

        public PHDll chk(final int chk) {
            this.chk = chk;
            return this;
        }

        public PHDll idHeader(final byte[] idHeader) {
            this.idHeader = idHeader;
            return this;
        }

        public PHDll inner(final byte[] inner) {
            this.inner = inner;
            return this;
        }

        public PHDll build() {
            return this;
        }


    public static PHDll builder() {
        return new PHDll();
    }
/*
    public PHDll(final int opcode, final int typeLen, final int payloadLen, final int idHeaderLen, final int seq, final int chk, final byte[] idHeader, final byte[] inner) {
        this.opcode = opcode;
        this.typeLen = typeLen;
        this.payloadLen = payloadLen;
        this.idHeaderLen = idHeaderLen;
        this.seq = seq;
        this.chk = chk;
        this.idHeader = idHeader;
        this.inner = inner;
    } */

    @SuppressWarnings("all")
    public int getSeq() {
        return this.seq;
    }

    @SuppressWarnings("all")
    public byte[] getInner() {
        return this.inner;
    }
    //</editor-fold>
}
