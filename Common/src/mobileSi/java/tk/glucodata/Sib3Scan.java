/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2, Libre 3, Dexcom G7/ONE+,              */
/*      Sibionics GS1Sb and Accu-Chek SmartGuide sensors.                            */
/*                                                                                   */
/*      Copyright (C) 2021 Jaap Korthals Altes <jaapkorthalsaltes@gmail.com>         */
/*                                                                                   */
/*      Juggluco is free software: you can redistribute it and/or modify             */
/*      it under the terms of the GNU General Public License as published            */
/*      by the Free Software Foundation, either version 3 of the License, or         */
/*      (at your option) any later version.                                          */
/*                                                                                   */
/*      Juggluco is distributed in the hope that it will be useful, but              */
/*      WITHOUT ANY WARRANTY; without even the implied warranty of                   */
/*      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                         */
/*      See the GNU General Public License for more details.                         */
/*                                                                                   */
/*      You should have received a copy of the GNU General Public License            */
/*      along with Juggluco. If not, see <https://www.gnu.org/licenses/>.            */
/*                                                                                   */
/*      Wed May 06 21:13:17 CEST 2026                                                */
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
static  void readNdef(MainActivity act,Tag tag) {
    Ndef ndef = Ndef.get(tag);
    if (ndef != null) { readViaNdef(act,ndef); return; }  
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
}
