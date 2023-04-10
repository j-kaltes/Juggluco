package tk.glucodata.NovoPen.opennov.mt;

import tk.glucodata.NovoPen.opennov.BaseMessage;
import java.nio.ByteBuffer;
import java.util.LinkedList;
import java.util.List;

/**
 * JamOrHam
 * OpenNov Configuration message
 */
public class Configuration extends BaseMessage {
    private static final int MDC_DIM_INTL_UNIT = 5472;
    private int id = -1;
    private int handle = -1;
    private long numberOfSegments = -1;
    private long totalStoredEntries = -1;
    private long totalStorageCapacity = -1;
    private long unitCode = -1;
//    private final List<ValueMap> valueMaps = new LinkedList<>();

    public boolean isAsExpected() {
        return unitCode == MDC_DIM_INTL_UNIT && numberOfSegments >= 0 && totalStoredEntries >= 0;
    }

    public static Configuration parse(final ByteBuffer buffer) {
        var id = getUnsignedShort(buffer);
        var count = getUnsignedShort(buffer);
        var len = getUnsignedShort(buffer);
        Configuration configuration = null;
        log("Configuration: " + id + " c:" + count + " len:" + len);
        for (int i = 0; i < count; i++) {
            var cls = getUnsignedShort(buffer);
            var handle = getUnsignedShort(buffer);
            var acount = getUnsignedShort(buffer);
            var alen = getUnsignedShort(buffer);
            for (int j = 0; j < acount; j++) {
                if (configuration == null) {
                    configuration = new Configuration();
                    configuration.id = id;
                    configuration.handle = handle;
                }
                var a = Attribute.parse(buffer);
                if (d) log("Class: " + cls + " " + a.toJson());
                if (a.atype != null) {
                    switch (a.atype) {
                    case MDC_ATTR_NUM_SEG: 
                        configuration.numberOfSegments = a.ivalue;
                        break;
                    case MDC_ATTR_METRIC_STORE_USAGE_CNT: 
                        configuration.totalStoredEntries = a.ivalue;
                        break;
                    case MDC_ATTR_UNIT_CODE: 
                        configuration.unitCode = a.ivalue;
                        break;
                    case MDC_ATTR_METRIC_STORE_CAPAC_CNT: 
                        configuration.totalStorageCapacity = a.ivalue;
                        break;
                    case MDC_ATTR_ATTRIBUTE_VAL_MAP: 
//                        configuration.valueMaps.add(ValueMap.parse(a.bytes));
                        break;
                    }
                }
            }
        }
        return configuration;
    }

    public static Configuration parse(final byte[] bytes) {
        return parse(ByteBuffer.wrap(bytes));
    }

    //<editor-fold defaultstate="collapsed" desc="delombok">
    @SuppressWarnings("all")
    public int getId() {
        return this.id;
    }

    @SuppressWarnings("all")
    public int getHandle() {
        return this.handle;
    }

    @SuppressWarnings("all")
    public long getNumberOfSegments() {
        return this.numberOfSegments;
    }

    @SuppressWarnings("all")
    public long getTotalStoredEntries() {
        return this.totalStoredEntries;
    }

    @SuppressWarnings("all")
    public long getTotalStorageCapacity() {
        return this.totalStorageCapacity;
    }

    @SuppressWarnings("all")
    public long getUnitCode() {
        return this.unitCode;
    }

    /*
    @SuppressWarnings("all")
    public List<ValueMap> getValueMaps() {
        return this.valueMaps;
    } */
    //</editor-fold>
}
