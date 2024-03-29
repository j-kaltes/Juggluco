package tk.glucodata.NovoPen.opennov.mt;

import tk.glucodata.NovoPen.opennov.BaseMessage;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;

/**
 * JamOrHam
 * OpenNov segment info list
 */
public class SegmentInfoList extends BaseMessage {
    private int scount;
    private int slength;
    private final List<SegmentInfo> items = new ArrayList<>();

    public boolean isTypical() {
        if (items.size() != 1) {
            error("Non typical segment info size: " + items.size());
            return false;
        }
        for (var i : items) {
            if (!i.isTypical()) {
                error("Non typical segment info: " + i.toJson());
                return false;
            }
        }
        return true;
    }

    public int getNextUnprocessedId() {
        for (var i : items) {
            if (!i.isProcessed()) return i.getInstnum();
        }
        return -1;
    }

    public long getNextUnprocessedCount() {
        for (var i : items) {
            if (!i.isProcessed()) return i.getUsage();
        }
        return -1;
    }

    public boolean hasUnprocessed() {
        return getNextUnprocessedId() >= 0;
    }

    public void markProcessed(final int which) {
        if (which == -1) {
            error("Attempt to mark invalid segment processed");
            return;
        }
        for (var i : items) {
            if (i.getInstnum() == which) {
                i.setProcessed(true);
            }
        }
    }

    public static SegmentInfoList parse(final ByteBuffer buffer) {
        var r = new SegmentInfoList();
        r.scount = getUnsignedShort(buffer);
        r.slength = getUnsignedShort(buffer);
        log("Scount: " + r.scount + " Slen:" + r.slength);
        for (int i = 0; i < r.scount; i++) {
            var si = SegmentInfo.parse(buffer);
            r.items.add(si);
            log("Segment: " + si.toJson());
        }
        return r;
    }

    //<editor-fold defaultstate="collapsed" desc="delombok">
    @SuppressWarnings("all")
    public int getScount() {
        return this.scount;
    }

    @SuppressWarnings("all")
    public List<SegmentInfo> getItems() {
        return this.items;
    }
    //</editor-fold>
}
