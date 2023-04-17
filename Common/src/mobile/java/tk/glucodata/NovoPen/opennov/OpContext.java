package tk.glucodata.NovoPen.opennov;

import java.util.ArrayList;
import java.util.List;

import tk.glucodata.NovoPen.opennov.mt.ARequest;
import tk.glucodata.NovoPen.opennov.mt.Apdu;
import tk.glucodata.NovoPen.opennov.mt.Configuration;
import tk.glucodata.NovoPen.opennov.mt.EventReport;
import tk.glucodata.NovoPen.opennov.mt.EventRequest;
import tk.glucodata.NovoPen.opennov.mt.SegmentInfoList;
import tk.glucodata.NovoPen.opennov.mt.Specification;
import tk.glucodata.NovoPen.opennov.mt.TrigSegmDataXfer;

/**
 * JamOrHam
 * OpenNov context holder
 */

public class OpContext {

    public Specification specification;
//    public RelativeTime relativeTime;
    public EventRequest eventRequest;
    public EventReport eventReport;
    public Configuration configuration;
    public TrigSegmDataXfer trigSegmDataXfer;
    public SegmentInfoList segmentInfoList;
//    public IdModel model;
    public ARequest aRequest;
    public Apdu apdu;
    public int invokeId = -1;

    static final public class Doses {
        public long referencetime;
        public byte[] rawdoses;

        public Doses(long referencetime, byte[] rawdoses) {
            this.referencetime=referencetime;
            this.rawdoses=rawdoses;
        }
    }
    public final List<Doses> doses=new ArrayList<>();
    public Configuration getConfiguration() {
        if (configuration != null) {
            return configuration;
        } else {
            if (eventReport != null && eventReport.configuration != null) {
                configuration = eventReport.configuration; // cache
                return configuration;
            }
        }
        return null;
    }

    public boolean hasConfiguration() {
        return configuration != null || getConfiguration() != null;
    }

    public boolean isError() {
        return apdu == null || apdu.isError();
    }

    public boolean wantsRelease() {
        return apdu != null && apdu.wantsRelease();
    }

}
