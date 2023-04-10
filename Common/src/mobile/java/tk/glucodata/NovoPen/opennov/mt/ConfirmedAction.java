package tk.glucodata.NovoPen.opennov.mt;

import tk.glucodata.NovoPen.opennov.BaseMessage;
import java.nio.ByteBuffer;

/**
 * JamOrHam
 * OpenNov Confirmed Action
 */
public class ConfirmedAction extends BaseMessage {
    private static final int ALL_SEGMENTS = 1;
    public int handle;
    public int type;
    public byte[] bytes;

    public ConfirmedAction allSegments() {
        var b = ByteBuffer.allocate(6);
        putUnsignedShort(b, ALL_SEGMENTS);
        putUnsignedShort(b, 2);
        putUnsignedShort(b, 0);
        bytes = b.array();
        return this;
    }

    public ConfirmedAction segment(final int segment) {
        var b = ByteBuffer.allocate(2);
        putUnsignedShort(b, segment);
        bytes = b.array();
        return this;
    }

    public byte[] encode() {
        var b = ByteBuffer.allocate(6 + (bytes != null ? bytes.length : 0));
        putUnsignedShort(b, handle);
        putUnsignedShort(b, type);
        putIndexedBytes(b, bytes != null ? bytes : new byte[0]);
        return b.array();
    }

    public static ConfirmedAction parse(final ByteBuffer buffer) {
        return ConfirmedAction.builder().handle(getUnsignedShort(buffer)).type(getUnsignedShort(buffer)).bytes(getIndexedBytes(buffer)).build();
    }




        public ConfirmedAction handle(final int handle) {
            this.handle = handle;
            return this;
        }

        public ConfirmedAction type(final int type) {
            this.type = type;
            return this;
        }

        public ConfirmedAction bytes(final byte[] bytes) {
            this.bytes = bytes;
            return this;
        }

        public ConfirmedAction build() {
            return this;
        }


    public static ConfirmedAction builder() {
        return new ConfirmedAction();
    }

    public ConfirmedAction() {
    	}
}
