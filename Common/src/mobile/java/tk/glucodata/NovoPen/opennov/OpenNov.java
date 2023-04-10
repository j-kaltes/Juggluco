/*
All files in this directory tree are a modification of files of xDrip
https://github.com/NightscoutFoundation/xDrip
*/
package tk.glucodata.NovoPen.opennov;

import static tk.glucodata.NovoPen.opennov.BaseMessage.d;
import static tk.glucodata.NovoPen.opennov.BaseMessage.log;

import android.nfc.Tag;
import android.nfc.tech.IsoDep;

import tk.glucodata.NovoPen.opennov.HexDump;
import tk.glucodata.NovoPen.opennov.buffer.MyByteBuffer;
import tk.glucodata.NovoPen.opennov.ll.PHDllHelper;
import tk.glucodata.NovoPen.opennov.ll.T4Transceiver;

import java.io.IOException;

import tk.glucodata.Log;


/**
 * JamOrHam
 * OpenNov implementation
 */


//@RequiresApi(api = Build.VERSION_CODES.KITKAT)
public class OpenNov extends MyByteBuffer {

    public static final String TAG = "OpenNov";
    private static final int MAX_ERRORS = 3;

    private T4Transceiver ts;
    private PHDllHelper ph;


    public OpContext processTag(final Tag tag) {
        try {
            var isoDep = IsoDep.get(tag);
            this.ts = new T4Transceiver(isoDep);
            this.ph = new PHDllHelper(this.ts);

            isoDep.connect();
            isoDep.setTimeout(1000);

            if (ts.doNeededSelection()) {
                Log.d(TAG, "Selection okay");

                int errors = 0;
                int transactions = 0;
                FSA fsa = FSA.read();
                var machine = new Machine();
                while (fsa.doRead()
                        && errors < MAX_ERRORS
                        && transactions < 200) {
                    transactions++;
                    var res = ts.readFromLinkLayer();
                    if (d) log("link layer read: " + HexDump.dumpHexString(res));
                    var payload = ph.extractInnerPacket(res, true);
                    if (payload != null) {
                        fsa = machine.processPayload(payload);
                        Log.d(TAG, "Got fsa action: " + fsa.action);
                        switch (fsa.action) {

                            case WRITE_READ:
                                ph.writeInnerPacket(fsa.payload);
                                break;

                            case DONE:
                                Log.d(TAG, "All done"); //TODO: why no isoDep.close()?
                                isoDep.close();

                                return machine.context;
                        }
                    } else {
                        errors++;
                        Log.d(TAG, "Read cycle got null errors @ " + errors);
                    } // if payload
                } // end while

                if (fsa.doRead()) {
                    Log.d(TAG, "Overall failure to read");
                    isoDep.close();
                    return null;
                }

                isoDep.close();
                return machine.context;
            }
            isoDep.close();
        } catch (IOException e) {
            Log.d(TAG, "Could not connect: " + e);
        } catch (Exception e) {
            Log.stack(TAG, "Got crash in handler: " , e);
        }
        return null;
    }

}
