package tk.glucodata;

import static tk.glucodata.Natives.gs3nfc;
import static tk.glucodata.ScanNfcV.failure;
import static tk.glucodata.ScanNfcV.getvibrator;
import static tk.glucodata.ScanNfcV.startvibration;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.nio.charset.Charset;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;

import android.nfc.Tag;
import android.nfc.tech.Ndef;
import android.nfc.FormatException;
import android.nfc.NdefMessage;
import android.nfc.NdefRecord;
import android.nfc.tech.MifareUltralight;
import android.nfc.tech.Ndef;
import android.nfc.tech.NfcA;
import android.os.VibrationEffect;

class Sib3Scan {
    private static final String LOG_ID="Sib3Scan";
static boolean hasTech(String[] techs, String want) {
    for (String t : techs) if (want.equals(t)) return true;
    return false;
}
static  void readNdef(MainActivity act,Tag tag) {
    Ndef ndef = Ndef.get(tag);
    if(ndef != null) { 
        readViaNdef(act,ndef); 
        return; 
        }  
    String[] techs = tag.getTechList();
    if(hasTech(techs, "android.nfc.tech.NfcA")) {
        readNdefFromNfcA(act,tag);
    } else {
        Log.d(LOG_ID, "No supported tech for manual NDEF: " + Arrays.toString(techs));
    }
}

private static void readViaNdef(MainActivity act,Ndef ndef) {
    var vib= getvibrator(act);
    startvibration(vib);
    try {
        ndef.connect();
        NdefMessage msg = ndef.getNdefMessage();   // fresh read
        Log.d(LOG_ID, "type=" + ndef.getType() + " maxSize=" + ndef.getMaxSize() + " writable=" + ndef.isWritable());
        if(msg != null) {
            for(NdefRecord r : msg.getRecords()) 
                dumpRecord(act,r);
            vib.cancel();
            final long[] newsensorVib =  {50, 150,50,50,12,8,15,73};
            if(android.os.Build.VERSION.SDK_INT < 26) 
                vib.vibrate(newsensorVib, -1);
            else
                vib.vibrate(VibrationEffect.createWaveform(newsensorVib, -1));
           }
        else {
            vib.cancel();
            failure(vib);
            }
    } catch (Throwable e) {
        Log.stack(LOG_ID, "readNdef", e);
        vib.cancel();
        failure(vib);
        }
     finally {
        try { ndef.close(); } catch (IOException ignored) {}
    }
}

private static void dumpRecord(MainActivity act,NdefRecord r) {
    short tnf = r.getTnf();
    byte[] type = r.getType();
    byte[] payload = r.getPayload();

    if (tnf == NdefRecord.TNF_WELL_KNOWN && Arrays.equals(type, NdefRecord.RTD_TEXT)) {
        int status = payload[0] & 0xFF;
        int langLen = status & 0x3F;
        Charset cs = StandardCharsets.UTF_8;
        Log.i(LOG_ID,"status="+status+ " langLen="+langLen+ " length="+payload.length);
        Log.d(LOG_ID, "Text: " + new String(payload, 1 + langLen, payload.length - 1 - langLen, cs));
        var name=gs3nfc(payload); 
        if(name!=null&& !name.isEmpty())  {
            act.runOnUiThread(() -> GetGS3ID.gs3Number(name,act));
            }
        }
    else {
        Log.d(LOG_ID, "TNF=" + tnf + " type=" + new String(type, StandardCharsets.US_ASCII) + " payload=" + payload.length + " bytes");
        }
   }
/*
static private void readNdefFromUltralight(MainActivity act,Tag tag) {
    var vib= getvibrator(act);
    startvibration(vib);
    MifareUltralight mfu = MifareUltralight.get(tag);
    if(mfu == null) {
        vib.cancel();
        failure(vib);
        return;
        }
    try {
        mfu.connect();
        // Capability Container lives in page 3; readPages returns 4 pages = 16 bytes
        byte[] cc = mfu.readPages(3);
        if ((cc[0] & 0xFF) != 0xE1) {
            Log.d(LOG_ID, String.format("Not NDEF formatted (CC magic=0x%02X)", cc[0]));
            return;
        }
        int capacity = (cc[2] & 0xFF) * 8;       // total user memory in bytes
        Log.d(LOG_ID, "NDEF capacity=" + capacity + " bytes, RW=" 
                + String.format("0x%02X", cc[3]));

        ByteArrayOutputStream buf = new ByteArrayOutputStream();
        int page = 4;
        int remaining = capacity;
        while (remaining > 0) {
            byte[] chunk = mfu.readPages(page);  // 16 bytes per call
            int take = Math.min(chunk.length, remaining);
            buf.write(chunk, 0, take);
            page += 4;
            remaining -= take;
        }
        NdefMessage msg = parseNdefTlv(buf.toByteArray());
        if (msg != null) {
            for (NdefRecord r : msg.getRecords()) dumpRecord(act,r);
            vib.cancel();
            final long[] newsensorVib =  {50, 150,50,50,12,8,15,73};
            if(android.os.Build.VERSION.SDK_INT < 26) 
                vib.vibrate(newsensorVib, -1);
            else
                vib.vibrate(VibrationEffect.createWaveform(newsensorVib, -1));
        } else {
            vib.cancel();
            failure(vib);
            Log.d(LOG_ID, "No NDEF Message TLV found");
        }
    } catch (IOException | FormatException e) {
        Log.stack(LOG_ID, "readNdefFromUltralight", e);
        vib.cancel();
        failure(vib);
    } finally {
        try { mfu.close(); } catch (IOException ignored) {}
    }
}
*/
// Same idea, but using raw NfcA READ command (0x30 + page) for tags that
// don't expose MifareUltralight tech. Returns 16 bytes per command.
static private void readNdefFromNfcA(MainActivity act,Tag tag) {
    var vib= getvibrator(act);
    startvibration(vib);
    NfcA nfca = NfcA.get(tag);
    if(nfca == null) {
        vib.cancel();
        failure(vib);
        return;
        }
    try {
        nfca.connect();
        byte[] cc = nfca.transceive(new byte[]{(byte) 0x30, 0x03});
        if ((cc[0] & 0xFF) != 0xE1) {
            Log.d(LOG_ID, String.format("Not NDEF formatted (CC magic=0x%02X)", cc[0]));
            vib.cancel();
            failure(vib);
            return;
           }
        int capacity = (cc[2] & 0xFF) * 8;
        ByteArrayOutputStream buf = new ByteArrayOutputStream();
        int page = 4;
        int remaining = capacity;
        while (remaining > 0) {
            byte[] chunk = nfca.transceive(new byte[]{(byte) 0x30, (byte) page});
            int take = Math.min(chunk.length, remaining);
            buf.write(chunk, 0, take);
            page += 4;
            remaining -= take;
        }
        NdefMessage msg = parseNdefTlv(buf.toByteArray());
        if(msg == null) {
            Log.i(LOG_ID,"parseNdefTlv()==null");
            vib.cancel();
            failure(vib);
            return;
            }
         else {
            for (NdefRecord r : msg.getRecords()) dumpRecord(act,r);
            vib.cancel();
            final long[] newsensorVib =  {50, 150,50,50,12,8,15,73};
            if(android.os.Build.VERSION.SDK_INT < 26) 
                vib.vibrate(newsensorVib, -1);
            else
                vib.vibrate(VibrationEffect.createWaveform(newsensorVib, -1));
            return;
            }

        } 
    catch(Throwable e) {
        Log.stack(LOG_ID, "readNdefFromNfcA", e);
        vib.cancel();
        failure(vib);
       } 
    finally {
        try { nfca.close(); 
            } 
        catch(Throwable th) {
             Log.stack(LOG_ID,"close",th);
            }
        }
    }

// User memory is a sequence of TLV blocks. We care about T=0x03 (NDEF Message).
private static NdefMessage parseNdefTlv(byte[] data) throws FormatException {
    int i = 0;
    while (i < data.length) {
        int t = data[i++] & 0xFF;
        if (t == 0x00) continue;                 // NULL TLV — padding
        if (t == 0xFE) return null;              // Terminator TLV — done
        // Length: 1 byte, or 0xFF followed by 2-byte big-endian length
        int len = data[i++] & 0xFF;
        if (len == 0xFF) {
            len = ((data[i++] & 0xFF) << 8) | (data[i++] & 0xFF);
        }
        if (t == 0x03) {                         // NDEF Message TLV
            return new NdefMessage(Arrays.copyOfRange(data, i, i + len));
        }
        i += len;                                // skip Lock Control (0x01), Memory Control (0x02), unknown
    }
    return null;
}




}
