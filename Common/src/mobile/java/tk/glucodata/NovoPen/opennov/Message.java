package tk.glucodata.NovoPen.opennov;

import static tk.glucodata.NovoPen.opennov.mt.Apdu.ApduType.AareApdu;
import static tk.glucodata.NovoPen.opennov.mt.Apdu.ApduType.AarqApdu;
import static tk.glucodata.NovoPen.opennov.mt.Apdu.ApduType.AbrtApdu;
import static tk.glucodata.NovoPen.opennov.mt.Apdu.ApduType.PrstApdu;
import static tk.glucodata.NovoPen.opennov.mt.Apdu.ApduType.RlreApdu;
import static tk.glucodata.NovoPen.opennov.mt.Apdu.ApduType.RlrqApdu;
import static tk.glucodata.NovoPen.opennov.mt.EventReport.MDC_NOTI_CONFIG;
import static tk.glucodata.NovoPen.opennov.mt.EventReport.MDC_NOTI_SEGMENT_DATA;
import static tk.glucodata.Natives.showbytes;
import tk.glucodata.NovoPen.opennov.HexDump;
import tk.glucodata.NovoPen.opennov.mt.ARequest;
import tk.glucodata.NovoPen.opennov.mt.AResponse;
import tk.glucodata.NovoPen.opennov.mt.Apdu;
import tk.glucodata.NovoPen.opennov.mt.ArgumentsSimple;
import tk.glucodata.NovoPen.opennov.mt.Attribute;
import tk.glucodata.NovoPen.opennov.mt.ConfirmedAction;
import tk.glucodata.NovoPen.opennov.mt.DataApdu;
import tk.glucodata.NovoPen.opennov.mt.EventReport;
import tk.glucodata.NovoPen.opennov.mt.EventRequest;
import tk.glucodata.NovoPen.opennov.mt.SegmentInfoList;
import tk.glucodata.NovoPen.opennov.mt.Specification;
import tk.glucodata.NovoPen.opennov.mt.TrigSegmDataXfer;
import java.nio.ByteBuffer;

/**
 * JamOrHam
 * OpenNov message processing
 */
public class Message extends BaseMessage {
    private static final int EVENT_REPORT_CHOSEN = 256;
    private static final int CONFIRMED_EVENT_REPORT_CHOSEN = 257;
    private static final int SCONFIRMED_EVENT_REPORT_CHOSEN = 513;
    private static final int GET_CHOSEN = 515;
    private static final int SGET_CHOSEN = 259;
    private static final int CONFIRMED_ACTION = 263;
    private static final int CONFIRMED_ACTION_CHOSEN = 519;
    private static final int MDC_ACT_SEG_GET_INFO = 3085;
    private static final int MDC_ACT_SEG_TRIG_XFER = 3100;
    private static final int STORE_HANDLE = 256;
    private boolean valid = false;
    private int invokeId = -1;
    private boolean closed = false;
    private final OpContext context;
    private int length = -1;

    public static Message parse(final OpContext context, final byte[] payload) {
        if (payload == null) {
            return null;
        }
        var m = new Message(context);
        m.length = payload.length;
        if (m.length < 4) {
            return m;
        }
        var buffer = ByteBuffer.wrap(payload);
        var apdu = Apdu.parse(buffer);
        if (apdu == null) return null;
        m.getContext().apdu = apdu;
        if (d) log("Choice length: " + apdu.choiceLength);
        switch (apdu.at) {
        case AarqApdu: 
            context.aRequest = ARequest.parse(buffer);
            if (d) log(context.aRequest.toJson());
            break;
        case AareApdu: 
            log(AResponse.parse(buffer).toJson());
            break;
        case RlrqApdu: 
            log("Release request");
            break;
        case RlreApdu:
            m.closed = true;
            log("closed");
            break;
        case AbrtApdu: 
            error("Received error reply");
            break;
        case PrstApdu: 
            var dpdu = DataApdu.parse(buffer);
            m.invokeId = dpdu.invokeId;
            context.invokeId = m.invokeId;
            log("olen: " + dpdu.olen + " invokeid:" + m.invokeId + " dchoice:" + Integer.toHexString(dpdu.dchoice) + " dlen:" + dpdu.dlen);
            switch (dpdu.dchoice) {
            case CONFIRMED_ACTION_CHOSEN: 
                // confirmed action
                var handle = getUnsignedShort(buffer);
                var actionType = getUnsignedShort(buffer);
                var actionLen = getUnsignedShort(buffer);
                log("handle: " + Integer.toHexString(handle) + " atype:" + Integer.toHexString(actionType));
                switch (actionType) {
                case MDC_ACT_SEG_GET_INFO: 
                    context.segmentInfoList = SegmentInfoList.parse(buffer);
                    ;
                    break;
                case MDC_ACT_SEG_TRIG_XFER: 
                    context.trigSegmDataXfer = TrigSegmDataXfer.parse(buffer);
                    break;
                default: 
                    log("Unknown action type: " + actionType);
                }
                break;
            case CONFIRMED_EVENT_REPORT_CHOSEN: 
                var er = EventReport.parse(buffer,context.doses);
                if (er != null) {
/*                    for (var ds : er.doses) {
                        if (d) log("dose: " + ds.toJson() + " " + tk.glucodata.util.timestring(ds.absoluteTime) + " valid: " + ds.isValid());
                    } */
                    context.eventReport = er;
                }
                break;
            case SCONFIRMED_EVENT_REPORT_CHOSEN: 
                var e = EventRequest.parse(buffer);
                log("SConfirmed: " + e.toJson());
                context.eventRequest = e;
                break;
            case GET_CHOSEN: 
            case SGET_CHOSEN: 
                var handlex = getUnsignedShort(buffer);
                var count = getUnsignedShort(buffer);
                var len = getUnsignedShort(buffer);
                if (d) log("gchosen: " + count + " Slen:" + len + " :: handle: " + handlex);
                for (int i = 0; i < count; i++) {
                    var a = Attribute.parse(buffer);
                    if (d) log(a.toJson());
                    switch (a.atype) {
                    case MDC_ATTR_ID_PROD_SPECN: 
                        context.specification = Specification.parse(a.bytes);
//                        log(context.specification.toJson());
                        break;
                    case MDC_ATTR_TIME_REL: 
//                        context.relativeTime = RelativeTime.parse(a.bytes);
 //                       log(context.relativeTime.toJson());
                        break;
                    case MDC_ATTR_ID_MODEL: 
//                        context.model = IdModel.parse(a.bytes);
 //                       log(context.model.toJson());
                        break;
                    }
                }
                break;
            case CONFIRMED_ACTION: 
                var ca = ConfirmedAction.parse(buffer);
                log("Confirmed action: handle:" + ca.handle + " type:" + ca.type + " bytes:" + HexDump.toHexString(ca.bytes));
                break;
            default: 
                log("Unknown dchoice: " + Integer.toHexString(dpdu.dchoice));
            }
            break;
        default: 
            error("Unhandled Apdu type: " + apdu.at);
        }
        return m;
    }

    public static Message parse(final byte[] payload) {
        return parse(new OpContext(), payload);
    }

    public byte[] getAResponse() {
        var apdu = Apdu.builder().at(Apdu.ApduType.AareApdu).choicePayload(new AResponse(context.aRequest).encode()).build();
        return apdu.encode();
    }

    public byte[] getAcceptConfig() {
        if (context.hasConfiguration()) {
            var dat = DataApdu.builder().invokeId(context.invokeId).dataPayload(EventRequest.builder().currentTime(0).type(MDC_NOTI_CONFIG).replyLen(4).reportId(context.getConfiguration().getId()).reportResult(0).build().encode()).dchoice(SCONFIRMED_EVENT_REPORT_CHOSEN).build().encode();
            showbytes("getAcceptConfig DataApdu", dat);
            var res =  //DataApdu encode
            Apdu.builder().at(Apdu.ApduType.PrstApdu).choicePayload(dat).build().encode();
            showbytes("getAcceptConfig res", res);
            return res;
        } else {
            return null;
        }
    }

    public byte[] getAskInformation() {
        if (context.hasConfiguration()) {
            return Apdu.builder().at(Apdu.ApduType.PrstApdu).choicePayload(DataApdu.builder().invokeId(context.invokeId).dchoice(SGET_CHOSEN).dataPayload(ArgumentsSimple.encode(context.eventReport.handle)).build().encode()).build().encode();
        } else {
            return null;
        }
    }

    public byte[] getConfirmedAction() {
        if (context.hasConfiguration()) {
            return Apdu.builder().at(Apdu.ApduType.PrstApdu).choicePayload(DataApdu.builder().invokeId(context.invokeId).dchoice(CONFIRMED_ACTION).dataPayload(ConfirmedAction.builder().handle(STORE_HANDLE).type(MDC_ACT_SEG_GET_INFO).build().allSegments().encode()).build().encode()).build().encode();
        } else {
            return null;
        }
    }

    public byte[] getXferAction(final int segment) {
        if (context.hasConfiguration()) {
            return Apdu.builder().at(Apdu.ApduType.PrstApdu).choicePayload(DataApdu.builder().invokeId(context.invokeId).dchoice(CONFIRMED_ACTION).dataPayload(ConfirmedAction.builder().handle(STORE_HANDLE).type(MDC_ACT_SEG_TRIG_XFER).build().segment(segment).encode()).build().encode()).build().encode();
        } else {
            return null;
        }
    }

    public byte[] getConfirmedXfer(final int instance, final int index, final int count, final boolean lastBlock, final boolean specifyBlock) {
        if (d) log("getConfirmedXfer: i:" + index + " c:" + count + " last:" + lastBlock);
        if (context.hasConfiguration()) {
            var er = EventRequest.builder().handle(STORE_HANDLE).currentTime(-1).replyLen(12).type(MDC_NOTI_SEGMENT_DATA).instance(instance).index(index).count(count).confirmed(true).build();
            if (specifyBlock) {
                if (!lastBlock) {
                    if (index == 0) {
                        er.firstBlock();
                    } else {
                        er.middleBlock();
                    }
                } else {
                    er.lastBlock();
                }
            } else {
                er.middleBlock();
            }
            return Apdu.builder().at(Apdu.ApduType.PrstApdu).choicePayload(DataApdu.builder().invokeId(invokeId).dataPayload(er.encode()).dchoice(SCONFIRMED_EVENT_REPORT_CHOSEN).build().encode()).build().encode();
        } else {
            return null;
        }
    }

    public byte[] getCloseDown() {
        return new byte[] {(byte) 228, (byte) 0, (byte) 0, (byte) 2, (byte) 0, (byte) 0};
    }

    public Message(final OpContext context) {
        this.context = context;
    }

    public boolean isValid() {
        return this.valid;
    }


    public boolean isClosed() {
        return this.closed;
    }

    public OpContext getContext() {
        return this.context;
    }

    public int getLength() {
        return this.length;
    }
}
