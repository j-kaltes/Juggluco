package tk.glucodata.NovoPen.opennov.mt;

import tk.glucodata.NovoPen.opennov.BaseMessage;
import java.nio.ByteBuffer;

/**
 * JamOrHam
 * OpenNov implementation
 */
public class TrigSegmDataXfer extends BaseMessage {
//    int infoLength;
    int segmentId;
    int responseCode;

    public TrigSegmDataXfer() {
    }

    public boolean isOkay() {
        return segmentId != 0 && responseCode == 0;
    }

    public byte[] encode() {
        var b = ByteBuffer.allocate(4);
        putUnsignedShort(b, segmentId);
        putUnsignedShort(b, responseCode);
        return b.array();
    }

    public static TrigSegmDataXfer parse(final ByteBuffer buffer) {
        var x = new TrigSegmDataXfer();
        x.segmentId = getUnsignedShort(buffer);
        x.responseCode = getUnsignedShort(buffer);
        if (d) log("TrigSeg: seg: " + x.segmentId + " response: " + x.responseCode + " ok: " + x.isOkay());
        return x;
    }


}
