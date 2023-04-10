package tk.glucodata.NovoPen.opennov.mt;

import tk.glucodata.NovoPen.opennov.BaseMessage;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;

/**
 * JamOrHam
 * OpenNov segment info
 */
public class SegmentInfo extends BaseMessage {
    private int instnum;
    private long usage = -1;
    private int acount;
    private int alength;
    private boolean processed = false;
    private SegmentInfoMap map;
    private final List<Attribute> items = new ArrayList<>();

    public boolean isTypical() {
        return usage >= 0 && map != null && map.isTypical();
    }

    public static SegmentInfo parse(final ByteBuffer buffer) {
        var r = new SegmentInfo();
        r.instnum = getUnsignedShort(buffer);
        r.acount = getUnsignedShort(buffer);
        r.alength = getUnsignedShort(buffer);
        log("Acount: " + r.acount);
        for (int i = 0; i < r.acount; i++) {
            var a = Attribute.parse(buffer);
            switch (a.atype) {
            case MDC_ATTR_PM_SEG_MAP: 
                r.map = SegmentInfoMap.parse(a.bytes);
                break;
            case MDC_ATTR_SEG_USAGE_CNT: 
                r.usage = a.ivalue;
                break;
            }
            r.items.add(a);
        }
        return r;
    }

    //<editor-fold defaultstate="collapsed" desc="delombok">
    @SuppressWarnings("all")
    public int getInstnum() {
        return this.instnum;
    }

    @SuppressWarnings("all")
    public long getUsage() {
        return this.usage;
    }

    @SuppressWarnings("all")
    public boolean isProcessed() {
        return this.processed;
    }

    @SuppressWarnings("all")
    public void setProcessed(final boolean processed) {
        this.processed = processed;
    }

    @SuppressWarnings("all")
    public SegmentInfoMap getMap() {
        return this.map;
    }
    //</editor-fold>
}
