package tk.glucodata.NovoPen.opennov.mt;

import static tk.glucodata.NovoPen.opennov.mt.ApoepElement.APOEP;
import tk.glucodata.NovoPen.opennov.HexDump;
import tk.glucodata.NovoPen.opennov.BaseMessage;
import java.nio.ByteBuffer;

/**
 * JamOrHam
 * OpenNov A Request
 */
public class ARequest extends BaseMessage {
    private int protocol;
    private long version;
    private int elements;
    private ApoepElement apoep;

    public boolean valid() {
        return protocol == APOEP && apoep != null && apoep.systemId.length == 8;
    }

    public static ARequest parse(final ByteBuffer buffer) {
        var ar = new ARequest();
        ar.version = getUnsignedInt(buffer);
        ar.elements = getUnsignedShort(buffer);
        var len = getUnsignedShort(buffer);
        for (int i = 0; i < ar.elements; i++) {
            ar.protocol = getUnsignedShort(buffer);
            var bytes = getIndexedBytes(buffer);
            if (d) log("AR Protocol: " + ar.protocol + " " + HexDump.dumpHexString(bytes));
            if (ar.protocol == APOEP) {
                ar.apoep = ApoepElement.parse(bytes);
                if (d) log(ar.apoep.toJson());
            }
        }
        return ar;
    }

    //<editor-fold defaultstate="collapsed" desc="delombok">
    @SuppressWarnings("all")
    public int getProtocol() {
        return this.protocol;
    }

    @SuppressWarnings("all")
    public long getVersion() {
        return this.version;
    }

    @SuppressWarnings("all")
    public int getElements() {
        return this.elements;
    }

    @SuppressWarnings("all")
    public ApoepElement getApoep() {
        return this.apoep;
    }
    //</editor-fold>
}
