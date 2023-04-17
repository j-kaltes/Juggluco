package tk.glucodata.NovoPen.opennov;

import static tk.glucodata.NovoPen.opennov.BaseMessage.d;
import static tk.glucodata.NovoPen.opennov.FSA.Action.WRITE_READ;
import static tk.glucodata.Natives.oldnovopenvalue;

import tk.glucodata.NovoPen.opennov.HexDump;
import tk.glucodata.NovoPen.opennov.mt.EventReport;
//import tk.glucodata.NovoPen.opennov.data.ICompleted;
import tk.glucodata.Log;
import tk.glucodata.Natives;

/**
 * JamOrHam
 * OpenNov state machine
 */
public class Machine {
    private static final String TAG = "OpenNov";
    private static final int MAX_REQUESTS = 100;
    private int requestCounter = 0;
    private long currentSegmentCount = -1;
    private int currentSegment = -1;
    private State state = State.AWAIT_ASSOCIATION_REQ;
   public  final OpContext context = new OpContext();
    private boolean lastSuccessCache = false;


    //<editor-fold defaultstate="collapsed" desc="delombok">
    public enum State {
        AWAIT_ASSOCIATION_REQ, AWAIT_CONFIGURATION, ASK_INFORMATION, AWAIT_INFORMATION, AWAIT_STORAGE_INFO, AWAIT_XFER_CONFIRM, AWAIT_LOG_DATA, AWAIT_CLOSE_DOWN, PROFIT;
    //</editor-fold>
        private static final State[] values = values();

        public State next() {
            var newState = values[(this.ordinal() + 1) % values.length];
            Log.d(TAG, "Asking for next state " + this + " -> " + newState);
            return newState;
        }
    }

    FSA processPayload(final byte[] payload) {
        if (d) Log.d(TAG, "state: " + state + " processing payload: " + HexDump.dumpHexString(payload));
        if (payload != null) {
            var msg = Message.parse(context, payload);
            if (!msg.getContext().isError()) {
                return processState(msg);
            } else {
                Log.e(TAG, "Got error response parsing message");
                return FSA.empty();
            }
        }
        return FSA.empty();
    }

    FSA processState(final Message msg) {
        Log.d(TAG, "Processing state: " + state);
        if (msg.getContext().wantsRelease()) {
            Log.d(TAG, "Remote end requests release");
            return doCloseDown(msg);
        }
        switch (state) {
        case AWAIT_ASSOCIATION_REQ: 
            if (context.aRequest != null && context.aRequest.valid()) {
                state = state.next();
                return new FSA(WRITE_READ, msg.getAResponse());
            }
            break;
        case AWAIT_CONFIGURATION: 
            if (context.getConfiguration() != null) {
                if (context.configuration.isAsExpected()) {
                    if (context.configuration.getNumberOfSegments() > 1) {
                        Log.e(TAG, "Multiple segments @ " + context.configuration.getNumberOfSegments());
                    }
                    state = state.next();
                    return new FSA(WRITE_READ, msg.getAcceptConfig());
                } else {
                    Log.d(TAG, "Configuration not as expected");
                }
            } else {
                Log.d(TAG, "Configuration not found");
            }
            break;
        case ASK_INFORMATION: 
            Log.d(TAG, "Ask information");
            state = state.next();
            return new FSA(WRITE_READ, msg.getAskInformation());
        case AWAIT_INFORMATION: 
            Log.d(TAG, "Await information");
            if (context.specification == null) {
                Log.d(TAG, "Failed to acquire specification - trying again");
                return new FSA(WRITE_READ, msg.getAskInformation());
            }
//            lastSuccessCache = wasLastReadSuccess();
            state = state.next();
            return new FSA(WRITE_READ, msg.getConfirmedAction());
        case AWAIT_STORAGE_INFO: 
            Log.d(TAG, "Await storage information");
            return handleNextSegment(msg);
        case AWAIT_XFER_CONFIRM: 
            Log.d(TAG, "Await transfer information");
            if (msg.getLength() != 0) {
                if (context.trigSegmDataXfer != null && context.trigSegmDataXfer.isOkay()) {
                    context.trigSegmDataXfer = null; // clear for next
                    state = state.next();
                } else {
                    Log.d(TAG, "Transfer information not right - trying again");
                    return handleNextSegment(msg);
                }
            } else {
                Log.d(TAG, "Didn\'t get anything - trying again in state: " + state);
            }
            return FSA.writeNull();
        case AWAIT_LOG_DATA: 
            Log.d(TAG, "Await Log data: msize:" + msg.getLength());
            if (msg.getLength() == 0) return FSA.writeNull();
            if (context.eventReport.count > 0) {
                setLastReadSuccess(false); // started receiving data
            }
        var opcontext=msg.getContext();
            var er = opcontext.eventReport;
        {
            String serial;
            var  doses=opcontext.doses;
            OpContext.Doses dose;
            if(opcontext.specification != null && (serial = opcontext.specification.getSerial())!= null && serial.length() > 3 && (er.count == 0 || (doses.size()>0&&(dose=doses.get(doses.size()-1))!=null&&oldnovopenvalue(dose.referencetime,serial,dose.rawdoses)))) {
                Log.d(TAG, "No new data so requesting close");
                setLastReadSuccess(true); // completed receiving data
                return doCloseDown(msg);
            }
        }
            var tsil = msg.getContext().segmentInfoList;
            if (currentSegmentCount == (er.index + er.count)) {
                Log.d(TAG, "Segment " + er.instance + " complete @ " + currentSegmentCount);
                tsil.markProcessed(er.instance);
                if (!tsil.hasUnprocessed()) {
                    Log.d(TAG, "All segments processed");
                    setLastReadSuccess(true); // completed receiving data
                } else {
                    Log.e(TAG, "Segments remain unprocessed");
                    return handleNextSegment(msg);
                }
            }
            return FSA.writeRead(msg.getConfirmedXfer(er.instance, (int) er.index, (int) er.count, false, false));


        case AWAIT_CLOSE_DOWN: 
            if (msg.isClosed()) {
                Log.d(TAG, "Closed down successfully");
                return FSA.empty();
            } else {
                Log.e(TAG, "Didn\'t close down this time");
                return FSA.empty();
            }
        default: 
            Log.e(TAG, "Unknown state: " + state);
        }
        return FSA.empty();
    }

    private FSA doCloseDown(final Message msg) {
        state = State.AWAIT_CLOSE_DOWN;
        return FSA.writeRead(msg.getCloseDown());
    }

    FSA handleNextSegment(final Message msg) {
        requestCounter++;
        if (requestCounter > MAX_REQUESTS) {
            Log.e(TAG, "Exceeded max requests");
        } else {
            var sil = msg.getContext().segmentInfoList;
            if (sil.isTypical()) {
                currentSegment = sil.getNextUnprocessedId();
                state = State.AWAIT_XFER_CONFIRM;
                if (currentSegment >= 0) {
                    Log.d(TAG, "Requesting next segment: " + currentSegment);
                    currentSegmentCount = sil.getNextUnprocessedCount();
                    return FSA.writeRead(msg.getXferAction(currentSegment));
                } else {
                    // no more segments
                    Log.d(TAG, "No more segments");
                    return doCloseDown(msg);
                }
            } else {
                Log.e(TAG, "Non typical segments - please contact developers");
            }
        }
        return FSA.empty();
    }


    private void setLastReadSuccess(final boolean result) {
            Log.i(TAG,"setLastReadSuccess("+result+")");
    }



}
