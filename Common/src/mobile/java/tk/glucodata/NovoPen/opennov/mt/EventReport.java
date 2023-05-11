package tk.glucodata.NovoPen.opennov.mt;


import static tk.glucodata.Log.showbytes;

import tk.glucodata.NovoPen.opennov.BaseMessage;
import tk.glucodata.NovoPen.opennov.OpContext;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;



/**
 * JamOrHam
 * OpenNov Event Report
 */

public class EventReport extends BaseMessage {

    public static final int MDC_NOTI_CONFIG = 3356;
    public static final int MDC_NOTI_SEGMENT_DATA = 3361;

    public int handle = -1;
    public int instance = -1;
    public long index = -1;
    public int count = -1;
    public Configuration configuration;


    public static EventReport parse(final ByteBuffer buffer, List<OpContext.Doses> doses) {

        var handle = getUnsignedShort(buffer);
        var relativeTime = getUnsignedInt(buffer);
        var eventType = getUnsignedShort(buffer);
        var len = getUnsignedShort(buffer);

        log("EventReport: handle: " + handle + " rt: " + relativeTime + " " + eventType);

        long referencetime=(System.currentTimeMillis()/1000L)-relativeTime;
        var er = new EventReport();
        er.handle = handle;

        switch (eventType) {
            case MDC_NOTI_SEGMENT_DATA:
                er.instance = getUnsignedShort(buffer);
                er.index = getUnsignedInt(buffer);
                er.count = (int)getUnsignedInt(buffer);
                var status = getUnsignedShort(buffer);
                var bcount = getUnsignedShort(buffer);

                if (d) log("EventReport: segment data: " + len + " instance: " + er.instance + " index: " + er.index + " count: " + er.count + " status: " + Integer.toHexString(status) + " bcount: " + bcount);
/*
                for (int i = 0; i < er.count; i++) {
                    er.doses.add(InsulinDose.parse(buffer, relativeTime));
                } */
                final int totmem=12*er.count;
                byte[] rawdoses=new byte[totmem];
                buffer.get(rawdoses,0,totmem);
                int doseslen=doses.size();
                showbytes("dose"+doseslen,rawdoses);
                doses.add(new OpContext.Doses(referencetime,rawdoses));

                return er;

            case MDC_NOTI_CONFIG:
                er.configuration = Configuration.parse(buffer);
                return er;
            default:
                log("Unknown event report type: " + eventType);
                break;
        }

        return null;
    }


}
