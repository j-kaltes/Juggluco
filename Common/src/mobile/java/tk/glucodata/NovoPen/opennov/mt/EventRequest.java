package tk.glucodata.NovoPen.opennov.mt;

import tk.glucodata.NovoPen.opennov.BaseMessage;
import java.nio.ByteBuffer;

/**
 * JamOrHam
 * OpenNov Event Request
 */
public class EventRequest extends BaseMessage {
    private static final int FIRST = 128;
    private static final int MIDDLE = 0;
    private static final int LAST = 64;
    private static final int MARK = 128;
    public int handle;
    public long currentTime;
    public int type;
    public int replyLen;
    public int reportId=-1;
    public int reportResult=-1;
    public int instance=-1;
    public int index=-1;
    public int count=-1;
    public int block=-1;
    public boolean confirmed=false;



    public EventRequest() {
    }

    public byte[] encode() {
        var b = ByteBuffer.allocate(10 + replyLen);
        putUnsignedShort(b, handle);
        putUnsignedInt(b, currentTime);
        putUnsignedShort(b, type);
        putUnsignedShort(b, replyLen);
        if (replyLen == 4) {
            putUnsignedShort(b, reportId);
            putUnsignedShort(b, reportResult);
        } else if (replyLen == 12) {
            putUnsignedShort(b, instance);
            putUnsignedShort(b, 0);
            putUnsignedShort(b, index);
            putUnsignedShort(b, 0);
            putUnsignedShort(b, count);
            putUnsignedByte(b, block);
            putUnsignedByte(b, confirmed ? MARK : 0);
        } else if (replyLen != 0) {
            error("Unsupported replyLen: " + replyLen);
            return null;
        }
        return b.array();
    }

    public EventRequest firstBlock() {
        block = FIRST;
        return this;
    }

    public EventRequest middleBlock() {
        block = MIDDLE;
        return this;
    }

    public EventRequest lastBlock() {
        block = LAST;
        return this;
    }

    public static EventRequest parse(final ByteBuffer buffer) {
        var e = new EventRequest();
        e.handle = getUnsignedShort(buffer);
        e.currentTime = getUnsignedInt(buffer);
        e.type = getUnsignedShort(buffer);
        e.replyLen = getUnsignedShort(buffer);
        if (e.replyLen == 4) {
            e.reportId = getUnsignedShort(buffer);
            e.reportResult = getUnsignedShort(buffer);
        }
        return e;
    }


        public EventRequest handle(final int handle) {
            this.handle = handle;
            return this;
        }

        public EventRequest currentTime(final long currentTime) {
            this.currentTime = currentTime;
            return this;
        }

        public EventRequest type(final int type) {
            this.type = type;
            return this;
        }

        public EventRequest replyLen(final int replyLen) {
            this.replyLen = replyLen;
            return this;
        }

        public EventRequest reportId(final int reportId) {
            this.reportId = reportId;
            return this;
        }

        public EventRequest reportResult(final int reportResult) {
            this.reportResult = reportResult;
            return this;
        }

        public EventRequest instance(final int instance) {
            this.instance = instance;
            return this;
        }

        public EventRequest index(final int index) {
            this.index = index;
            return this;
        }

        public EventRequest count(final int count) {
            this.count = count;
            return this;
        }

        public EventRequest block(final int block) {
            this.block = block;
            return this;
        }

        public EventRequest confirmed(final boolean confirmed) {
            this.confirmed = confirmed;
            return this;
        }

        public EventRequest build() {
		return this;
        }


    public static EventRequest builder() {
        return new EventRequest();
    }

    public EventRequest(final int handle, final long currentTime, final int type, final int replyLen, final int reportId, final int reportResult, final int instance, final int index, final int count, final int block, final boolean confirmed) {
        this.handle = handle;
        this.currentTime = currentTime;
        this.type = type;
        this.replyLen = replyLen;
        this.reportId = reportId;
        this.reportResult = reportResult;
        this.instance = instance;
        this.index = index;
        this.count = count;
        this.block = block;
        this.confirmed = confirmed;
    }
}
