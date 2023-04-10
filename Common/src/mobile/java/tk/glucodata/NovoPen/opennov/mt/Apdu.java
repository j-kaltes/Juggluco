package tk.glucodata.NovoPen.opennov.mt;

import tk.glucodata.NovoPen.opennov.BaseMessage;
import java.nio.ByteBuffer;

/**
 * JamOrHam
 * OpenNov APDU Type
 */
public class Apdu extends BaseMessage {
    public int at;
    public int choiceLength;
    byte[] choicePayload;

    private Apdu() {
    }
    public static Apdu builder() {
    	return new Apdu();
    	}

    public byte[] encode() {
        var b = ByteBuffer.allocate((choicePayload != null ? choicePayload.length : 0) + 4);
        putUnsignedShort(b, at);
        putIndexedBytes(b, choicePayload);
        return b.array();
    }

    public static Apdu parse(final ByteBuffer buffer) {
        var apdu = new Apdu();
        apdu.at = getUnsignedShort(buffer);
        if((apdu.at&0xff)!=0||apdu.at<ApduType.AarqApdu||apdu.at>ApduType.PrstApdu) {
            error("Cannot parse apdu type: " + apdu.at);
            return null;
        }
        apdu.choiceLength = getUnsignedShort(buffer);
        return apdu;
    }

    public boolean isError() {
        return at == ApduType.AbrtApdu;
    }

    public boolean wantsRelease() {
        return at == ApduType.RlrqApdu;
    }

static   public final class ApduType {
	public static final int AarqApdu=0xE200;
	public static final int AareApdu=0xE300;
	public static final int RlrqApdu=0xE400;
	public static final int RlreApdu=0xE500;
	public static final int AbrtApdu=0xE600;
	public static final int PrstApdu=0xE700;

    }

        public Apdu at(final int at) {
            this.at = at;
            return this;
        }

        public Apdu choicePayload(final byte[] choicePayload) {
            this.choicePayload = choicePayload;
            return this;
        }


        public Apdu build() {
            return this;
        }

}
