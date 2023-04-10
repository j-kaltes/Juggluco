package tk.glucodata.NovoPen.opennov.ll;

import static tk.glucodata.NovoPen.opennov.BaseMessage.d;
import static tk.glucodata.NovoPen.opennov.BaseMessage.log;

import tk.glucodata.NovoPen.opennov.HexDump;

import tk.glucodata.Log;


/**
 * JamOrHam
 * OpenNov link layer helper
 */

public class PHDllHelper {

    public PHDllHelper(T4Transceiver t4Transceiver) {
        this.ts = t4Transceiver;
    }
    private static final String TAG = "OpenNov";
    private static final byte[] EMPTY_NDEF = {(byte)0xD0,(byte)0x00,(byte)0x00};

    private final T4Transceiver ts;
    private int sequence = 0;

    public byte[] extractInnerPacket(byte[] res, boolean sendAck) {
        var p = PHDll.parse(res);
        if (p == null) return null;
        if (p.getSeq() != sequence) {
            Log.e(TAG, "Unexpected sequence " + p.getSeq() + " vs " + sequence);
            return null;
        }
        if (sendAck) {
            ts.writeToLinkLayer(EMPTY_NDEF);
        }
        return p.getInner();
    }

    public int writeInnerPacket(final byte[] inner) {
        if (d) log("inner write: " + HexDump.dumpHexString(inner));
        var outer = PHDll.builder()
                .inner(inner)
                .seq(++sequence)
                .build().encode();
        sequence = (sequence + 1) & 0x0F;
        ts.writeToLinkLayer(outer);
        return inner.length;
    }

}
