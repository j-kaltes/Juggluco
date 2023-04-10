package tk.glucodata.NovoPen.opennov.mt;

import tk.glucodata.NovoPen.opennov.BaseMessage;
import tk.glucodata.NovoPen.opennov.OpenNov;

import java.nio.ByteBuffer;
import java.util.LinkedList;
import java.util.List;

import tk.glucodata.Log;

/**
 * JamOrHam
 * OpenNov Arguments Simple
 */
public class ArgumentsSimple extends BaseMessage {

static    public byte[] encode(int handle) {
        Log.i(OpenNov.TAG,"ArgumentsSimple("+handle+")");
        var b = ByteBuffer.allocate(6);
        putUnsignedShort(b, handle);
        putUnsignedShort(b, 0);
        putUnsignedShort(b, 0);
        return b.array();
    }

}
