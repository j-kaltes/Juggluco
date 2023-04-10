package tk.glucodata.NovoPen.opennov.mt;

import tk.glucodata.NovoPen.opennov.BaseMessage;
import java.nio.ByteBuffer;

/**
 * JamOrHam
 * OpenNov Data Apdu
 */
public class DataApdu extends BaseMessage {
    public int olen;
    public int invokeId;
    public int dchoice;
    public int dlen;
    byte[] dataPayload;

    private DataApdu() {
    }

    public byte[] encode() {
        var b = ByteBuffer.allocate((dataPayload != null ? dataPayload.length : 0) + 8);
        putUnsignedShort(b, b.capacity() - 2);
        putUnsignedShort(b, invokeId);
        putUnsignedShort(b, dchoice);
        putIndexedBytes(b, dataPayload);
        return b.array();
    }

    public static DataApdu parse(final ByteBuffer buffer) {
        var a = new DataApdu();
        a.olen = getUnsignedShort(buffer);
        a.invokeId = getUnsignedShort(buffer);
        a.dchoice = getUnsignedShort(buffer);
        a.dlen = getUnsignedShort(buffer);
        return a;
    }


        public DataApdu olen(final int olen) {
            this.olen = olen;
            return this;
        }

        public DataApdu invokeId(final int invokeId) {
            this.invokeId = invokeId;
            return this;
        }

        public DataApdu dchoice(final int dchoice) {
            this.dchoice = dchoice;
            return this;
        }

        public DataApdu dlen(final int dlen) {
            this.dlen = dlen;
            return this;
        }

        public DataApdu dataPayload(final byte[] dataPayload) {
            this.dataPayload = dataPayload;
            return this;
        }

        public DataApdu build() {
            return this;
        }

    public static DataApdu builder() {
        return new DataApdu();
    }

    public DataApdu(final int olen, final int invokeId, final int dchoice, final int dlen, final byte[] dataPayload) {
        this.olen = olen;
        this.invokeId = invokeId;
        this.dchoice = dchoice;
        this.dlen = dlen;
        this.dataPayload = dataPayload;
    }
}
