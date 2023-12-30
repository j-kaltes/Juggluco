package tk.glucodata.NovoPen.opennov.mt;

import tk.glucodata.NovoPen.opennov.BaseMessage;
import java.nio.ByteBuffer;

/**
 * JamOrHam
 * OpenNov APOEP Element
 */
public class ApoepElement extends BaseMessage {
    public static final int APOEP = 20601;
    public static final int SYS_TYPE_MANAGER = -2147483648;
    public static final int SYS_TYPE_AGENT = 8388608;
    public long version = -1;
    public int encoding = -1;
    public long nomenclature = -1;
    public long functional = -1;
    public long systemType = -1;
    public byte[] systemId = null;
    public int configId = -1;
    public long recMode = -1;
    public int olistCount = -1;
    public int olistLen = -1;

    public ApoepElement() {
    }

    public byte[] encode() {
        var b = ByteBuffer.allocate(30 + systemId.length);
        putUnsignedInt(b, version);
        putUnsignedShort(b, encoding);
        putUnsignedInt(b, nomenclature);
        putUnsignedInt(b, functional);
        putUnsignedInt(b, systemType);
        putIndexedBytes(b, systemId);
        putUnsignedShort(b, configId);
        putUnsignedInt(b, recMode);
        putUnsignedShort(b, olistCount);
        putUnsignedShort(b, olistLen);
        return b.array();
    }

    public static ApoepElement parse(final ByteBuffer buffer) {
        var r = new ApoepElement();
        r.version = getUnsignedInt(buffer);
        r.encoding = getUnsignedShort(buffer);
        r.nomenclature = getUnsignedInt(buffer);
        r.functional = getUnsignedInt(buffer);
        r.systemType = getUnsignedInt(buffer);
        r.systemId = getIndexedBytes(buffer);
        r.configId = getUnsignedShort(buffer);
        r.recMode = getUnsignedInt(buffer);
        r.olistCount = getUnsignedShort(buffer);
        r.olistLen = getUnsignedShort(buffer);
        return r;
    }

    public static ApoepElement parse(final byte[] bytes) {
        return parse(ByteBuffer.wrap(bytes));
    }

    public ApoepElement(final long version, final int encoding, final long nomenclature, final long functional, final long systemType, final byte[] systemId, final int configId, final long recMode, final int olistCount, final int olistLen) {
        this.version = version;
        this.encoding = encoding;
        this.nomenclature = nomenclature;
        this.functional = functional;
        this.systemType = systemType;
        this.systemId = systemId;
        this.configId = configId;
        this.recMode = recMode;
        this.olistCount = olistCount;
        this.olistLen = olistLen;
    }
    public ApoepElement(ApoepElement orig) {
        this.version =orig.version;
        this.encoding =orig.encoding;
        this.nomenclature =orig.nomenclature;
        this.functional =orig.functional;
        this.systemType =orig.systemType;
        this.systemId =orig.systemId;
        this.configId =orig.configId;
        this.recMode =orig.recMode;
        this.olistCount =orig.olistCount;
        this.olistLen =orig.olistLen;

    }
}
