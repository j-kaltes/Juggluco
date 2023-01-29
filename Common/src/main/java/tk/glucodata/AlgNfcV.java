/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2 and 3 sensors.                         */
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
/*      Fri Jan 27 15:31:05 CET 2023                                                 */


package tk.glucodata;


import android.nfc.Tag;
import android.nfc.tech.NfcV;
import android.os.Build;

import java.io.IOException;
import java.util.Arrays;

import static tk.glucodata.Gen2.gen2enablestreaming;
import static tk.glucodata.Gen2.getuspatch;
import static tk.glucodata.Gen2.getversion;
import static tk.glucodata.Natives.bluetoothOnKey;


public class AlgNfcV {
    //    private static final String LOG_ID = packa.ge.name + AlgNfcV.class.getSimpleName();
    private static final String LOG_ID = "AlgNfcV";
    private static final long nfcReadTimeout = 3000; // [ms]

//static boolean NFC_USE_MULTI_BLOCK_READ=true ;

    private static void log(final String str) {
        Log.d(LOG_ID, str);
    }

    public static boolean goodnfc(final byte[] data) {
        return data != null && data.length > 0 && (data[0] & 1) == 0;
    }

    static boolean nfctry(final Tag tag) {


        log("nfctry");
        return nfccmd(tag, new byte[]{38, 1, 0}) != null;
    }
    static boolean enableStreaming(final Tag tag, byte[] info) {
	if(getversion(info)==2) 
		return gen2enablestreaming(tag);
	return EUenableStreaming(tag, info);
	}

    static boolean EUenableStreaming(final Tag tag, byte[] info) {
        byte[] uid = tag.getId();
        byte[] payload = bluetoothOnKey(uid, info);
        if (payload != null) {
	   byte[] addr=enableStreaming2(tag, payload);
            if (addr!=null) {
                Natives.enabledStreaming(uid, info, 1,((Build.VERSION.SDK_INT < 26)&&Applic.mayscan())?null:addr);
                return true;
            }
            Natives.bluetoothback(uid, info);
            return false;
        }
        Natives.enabledStreaming(uid, info,2,null); //don't try to enable again
        log("payload==null");
        return false;
    }

    static byte[] enableStreaming2(final Tag tag, byte[] payload) {
        byte[] cmd = {2, -95, 7};
        byte[] all = new byte[(cmd.length + payload.length)];
        System.arraycopy(cmd, 0, all, 0, cmd.length);
        System.arraycopy(payload, 0, all, cmd.length, payload.length);
        return nfccmd(tag, all) ;
    }


    /*
    static String activate(final Tag tag) {
    log("start activate");
          if (!nfctry(tag))
              return null;
          byte[] info=nfcinfo(tag);
          if(info==null)
              return null;
          byte activationCommand=Natives.activationcommand(info);
        log("command="+activationCommand);
          byte[] uid = tag.getId();
          byte person=0;
          byte [] activationPayload=Natives.activationpayload(uid,info,person);
          log("payload len="+(activationPayload==null?"null":activationPayload.length));
          for(byte b:activationPayload)
                log(b+" ");
          byte[] precmd = {2, activationCommand, 7};
          byte[] cmd = new byte[(precmd.length + activationPayload.length)];
          System.arraycopy(precmd, 0, cmd, 0, precmd.length);
          System.arraycopy(activationPayload, 0, cmd, precmd.length, activationPayload.length);
        log("activate cmd");
          if(nfccmd(tag, cmd)==null)  {
              log("activation failed");
              return null;
        }
        if(enableStreaming(tag, info) ) {
                log("Streaming enabled");
                return	Natives.getserial(uid,info);
                }
       else
        log("Enable streaming failed");
       return null;
      }
    */
    static boolean activate(final Tag tag, byte[] info, byte[] uid) {
        log("start activate");
        if (!nfctry(tag)) //Overbodig?
            return false;
        byte activationCommand = Natives.activationcommand(info);
        log("command=" + activationCommand);
        byte person = 0;
        byte[] activationPayload = Natives.activationpayload(uid, info, person);
        byte[] precmd = {2, activationCommand, 7};
        byte[] cmd = new byte[(precmd.length + activationPayload.length)];
        System.arraycopy(precmd, 0, cmd, 0, precmd.length);
        System.arraycopy(activationPayload, 0, cmd, precmd.length, activationPayload.length);
        log("activate cmd");
        if (nfccmd(tag, cmd) == null) {
            log("activation failed");
            return false;
        }
        if(enableStreaming(tag, info)) {
            log("Streaming enabled");
            return true;
        } else
            log("Enable streaming failed");
        return false;
    }


    static public byte[] wholenfccmd(final Tag tag, final byte[] cmd) {
        log("nfccmd");
	for(int outi=0;outi<10;outi++) {
        final NfcV nfcvTag = NfcV.get(tag);
        try {
		    nfcvTag.connect();
		    final long endReadingTime = System.currentTimeMillis() + nfcReadTimeout;
		    byte[] infodata = null;
		    int it = 10;
		    do {
			try {
			    infodata = nfcvTag.transceive(cmd);
			    } 
			catch(Throwable  error) {
				String mess=error!=null?error.toString():null;
				if(mess==null) {
					mess="error";
					}
				Log.i(LOG_ID,"transceive "+ mess);
				 if ((System.currentTimeMillis() > endReadingTime)) {
					Log.i(LOG_ID, "tag read timeout " + System.currentTimeMillis());
					return null;
					}
				}
			if(goodnfc(infodata))  {
			    return infodata;
			    }
		    } while (--it != 0);
		    Log.i(LOG_ID, "tried 10 times");
		    return null;
		} 
		catch (Exception e) {
		    Log.i(LOG_ID,"connect: "+ e.toString());
			} 
		finally {

		    try {
			Log.i(LOG_ID,"nfcvTag.close()");
			nfcvTag.close();
		    } catch (Exception e) {
			Log.stack(LOG_ID, "Error closing tag ", e);
		    }
		}
		}
		return null;
    }

static private byte[] nfccmd(final Tag tag, final byte[] cmd) {
    byte[] res=wholenfccmd(tag, cmd);
    if(res==null)
            return null;
     return Arrays.copyOfRange(res, 1, res.length);
	}
    static byte[] nfcinfo(final Tag tag) {
        log("nfcinfo");
        return nfccmd(tag, new byte[]{2, -95, 7});
    }


    static public byte[] issuenfc(NfcV nfc, byte[] cmd) {
        final int retries = 5;
        long startReadingTime = System.currentTimeMillis();
        for(int i=0;i<retries;i++) {
            log("t ");
            try {
                byte[] data = nfc.transceive(cmd);
                if (goodnfc(data)) {
                    return data;
                }
            } catch (Throwable e) {
                if ((System.currentTimeMillis() > startReadingTime + nfcReadTimeout)) {
                    Log.i(LOG_ID, "tag read timeout");
                    return null;
                }
            }
        }
        return null;
    }

    static byte[] readNfcTag(Tag tag, byte[] uid, byte[] info) {
        log("readNfcTag");
        try {
            NfcV nfcvTag = NfcV.get(tag);
            nfcvTag.connect();
            byte[] uit = readpatchdata(nfcvTag, uid, info);
            nfcvTag.close();
            return uit;
        } catch (IOException e) {
            Log.stack(LOG_ID, e);
            return null;
        }
    }

static public byte[] readpatchdata(NfcV nfc, byte[] uid, byte[] info) {
	final  int start=0,len=344;
	final boolean us=getversion(info)==2;
	return  us?getuspatch(nfc, uid,start, len):readoncedata(nfc, uid,start, len);
	}






static    private byte[] readoncedata(NfcV nfc, byte[] uid,int start, int len) {
	Log.i(LOG_ID,"readoncedata "+start+" "+len);
        int startinblocks = start / 8;
        int startblock = start % 8;
        int totlen = len + startblock;
        int blocks = (totlen + 7) / 8;
        int lenwholeblocks = blocks * 8;
        byte[] buf = new byte[lenwholeblocks];
        int iter = 0;
        while (iter < blocks) {
            int curblock = startinblocks + iter;
            int min = Math.min(3, blocks - iter);
            byte[] received = issuenfc(nfc, new byte[]{2, 35, (byte) curblock, (byte) (min - 1)});
            if (!goodnfc(received) || received.length < (min * 8) + 1) {
                return null;
            }
            System.arraycopy(received, 1, buf, iter * 8, received.length - 1);
            iter+=min;
        }
        return Arrays.copyOfRange(buf, startblock, totlen);
    }

}



/*
    private byte[] readFullPatchContents(Tag tag, byte[] Uid, byte[] info) {
        byte[] result = new byte[this.dataProcessing.getTotalMemorySize()];
        int i = 0;
        while (true) {
            MemoryRegion nextRegionToRead = this.dataProcessing.getNextRegionToRead(Uid, info, result, i);
            if (nextRegionToRead == null) {
                return result;
            }
            i++;
            int startAddress = nextRegionToRead.getStartAddress();
            byte[] scanPatch = this.f464rf.scanPatch(tag, startAddress, nextRegionToRead.getNumberOfBytes());
            if (scanPatch == null) {
                return null;
            }
            for (int i2 = 0; i2 < scanPatch.length; i2++) {
                result[startAddress + i2] = scanPatch[i2];
            }
        }
    }

 */
