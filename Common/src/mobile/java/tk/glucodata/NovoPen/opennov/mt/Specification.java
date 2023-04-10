package tk.glucodata.NovoPen.opennov.mt;

import tk.glucodata.NovoPen.opennov.BaseMessage;
import java.nio.ByteBuffer;

/**
 * JamOrHam
 * OpenNov specification element
 */
public class Specification extends BaseMessage {
    private static final int SERIAL_NUMBER = 1;
    private static final int PART_NUMBER = 2;
    private static final int HW_VERSION = 3;
    private static final int SW_VERSION = 4;
    private String serial;

    private String partNumber;
    private String softWareRevision;
    private String hardWareRevision;

    public static Specification parse(final ByteBuffer buffer) {
        var r = new Specification();
        var scount = getUnsignedShort(buffer);
        var ssize = getUnsignedShort(buffer);
        log("Specification: " + scount + " size:" + ssize);
        for (int i = 0; i < scount; i++) {
            var specType = getUnsignedShort(buffer);
            var componentId = getUnsignedShort(buffer);
            var s = getIndexedString(buffer);
            log("spectype: " + specType + " component: " + componentId + " " + s);
            switch (specType) {
            case SERIAL_NUMBER: 
                r.serial = s;
                break;
            case PART_NUMBER: 
                r.partNumber = s;
                break;
            case HW_VERSION: 
                r.hardWareRevision = s;
                break;
            case SW_VERSION: 
                r.softWareRevision = s;
                break;
            default: 
                log("Unknown specification type: " + specType);
            }
        }
        return r;
    }

    public static Specification parse(final byte[] bytes) {
        return parse(ByteBuffer.wrap(bytes));
    }

    public String getSerial() {
        return this.serial;
    }
}
