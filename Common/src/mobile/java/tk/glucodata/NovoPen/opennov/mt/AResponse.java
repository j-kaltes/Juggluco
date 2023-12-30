package tk.glucodata.NovoPen.opennov.mt;

import static tk.glucodata.NovoPen.opennov.mt.ApoepElement.APOEP;
import static tk.glucodata.NovoPen.opennov.mt.ApoepElement.SYS_TYPE_MANAGER;
import static tk.glucodata.NovoPen.opennov.OpenNov.TAG;
import tk.glucodata.Log;
import tk.glucodata.NovoPen.opennov.BaseMessage;
import java.lang.reflect.Field;
import java.lang.reflect.Modifier;
import java.nio.ByteBuffer;

/**
 * JamOrHam
 * OpenNov A Response
 */
public class AResponse extends BaseMessage {
    int result;
    int protocol;
    ApoepElement apoep;

    public AResponse() {
    }


    public AResponse(final ARequest request) {
        this.result = 3;
        this.protocol = APOEP;
        this.apoep = new ApoepElement(request.getApoep());
    }

    public byte[] encode() {
        apoep.recMode = 0;
        apoep.configId = 0;
        apoep.systemType = SYS_TYPE_MANAGER;
        apoep.olistCount = 0;
        apoep.olistLen = 0;
        var a = apoep.encode();
        var b = ByteBuffer.allocate(6 + a.length);
        putUnsignedShort(b, result);
        putUnsignedShort(b, protocol);
        putIndexedBytes(b, a);
        return b.array();
    }

    public static AResponse parse(final ByteBuffer buffer) {
        var a = new AResponse();
        a.result = getUnsignedShort(buffer);
        a.protocol = getUnsignedShort(buffer);
        if (a.protocol == APOEP) {
            a.apoep = ApoepElement.parse(getIndexedBytes(buffer));
            if (d) log(a.apoep.toJson());
        }
        return a;
    }

    public static AResponse parse(final byte[] bytes) {
        return parse(ByteBuffer.wrap(bytes));
    }


}
