package tk.glucodata.NovoPen.opennov.ll;


import static tk.glucodata.NovoPen.opennov.BaseMessage.d;

import android.nfc.TagLostException;
import android.nfc.tech.IsoDep;

import tk.glucodata.NovoPen.opennov.HexDump;
import tk.glucodata.NovoPen.opennov.buffer.MyByteBuffer;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;

import tk.glucodata.Log;


/**
 * JamOrHam
 * OpenNov type 4 transceive helper
 */

public class T4Transceiver extends MyByteBuffer {

    private static final int MAX_READ_SIZE_PARAMETER = 255;
    private static final long RETRY_PAUSE = 50;
    private static final int MAX_RETRY = 3;
    private static final String TAG = "OpenNov";
    /*
    private static final byte[] SA = T4Select.builder().build().aSelect().encode();
    private static final byte[] SC = T4Select.builder().build().ccSelect().encode();
    private static final byte[] SN = T4Select.builder().build().ndefSelect().encode();
*/
    public T4Transceiver(IsoDep isoDep) {
        this.tag = isoDep;
    }
//00 A4 04 00 07 D2 76 00 00 85 01 01 00
    static private final byte[] SA = {(byte) 0x00, (byte) 0xA4, (byte) 0x04, (byte) 0x00, (byte) 0x07, (byte) 0xD2, (byte) 0x76, (byte) 0x00, (byte) 0x00, (byte) 0x85, (byte) 0x01, (byte) 0x01, (byte) 0x00};
//    00 A4 00 0C 02 E1 03
    static private final byte[] SC = {(byte) 0x00, (byte) 0xA4, (byte) 0x00, (byte) 0x0C, (byte) 0x02, (byte) 0xE1, (byte) 0x03};

    static private final byte[] SN = {(byte) 0x00, (byte) 0xA4, (byte) 0x00, (byte) 0x0C, (byte) 0x02, (byte) 0xE1, (byte) 0x04};

    private final IsoDep tag;

    private int mlcMax = -1;
    private int mleMax = -1;


    public T4Reply t4Transceive(byte[] bytes, final T4Reply reply) {
        if (bytes == null) return null;
        try {
	    byte[] res=tag.transceive(bytes);
	     Log.i(TAG,"transceive(" +HexDump.toHexString(bytes)+")="+HexDump.toHexString(res));
            return T4Reply.parse(res, reply);
        } catch (TagLostException e) {
            Log.d(TAG, "Tag was lost");
            try {
                tag.close();
            } catch (IOException ioException) {
                Log.e(TAG, "Failure to close lost tag: " + e);
            }
        } catch (IOException e) {
            Log.stack(TAG, "Exception during transceive: ", e);
        }
        return null;
    }

    public T4Reply t4Transceive(byte[] bytes) {
        return t4Transceive(bytes, null);
    }

    public boolean transceiveOkay(final byte[] bytes, final String failureMsg) {
        var result = t4Transceive(bytes);
        var okay = result != null && result.isOkay();
        if (!okay) {
            Log.d(TAG, failureMsg);
        }
	else {
		Log.i(TAG,"transceiveOkay");
		}
        return okay;
    }


static    public byte[] T4Updateencode(byte[] bytes,int offset,boolean firstFragment,boolean lastFragment) {
        if (bytes == null) bytes = new byte[0];
        var isFragment = offset > 0;
        var hasDlen = offset == 0 || firstFragment;
        var len = lastFragment ? 7 : (bytes.length + (hasDlen ? 7 : 5));
        var b = ByteBuffer.allocate(len);
      final int CLA = 0;
     final int UPDATE_COMMAND = 214;
        putUnsignedByte(b, CLA);
        putUnsignedByte(b, UPDATE_COMMAND);
        putUnsignedShort(b, isFragment ? (offset + 2) : 0);
        putUnsignedByte(b, lastFragment ? 2 : bytes.length + (hasDlen ? 2 : 0));
        if (hasDlen) {
            putUnsignedShort(b, firstFragment ? 0 : bytes.length);
        }
        if (!lastFragment) {
            b.put(bytes);
        }
        return b.array();
    }


    static    private List<byte[]> updateEncodeForMtu(byte[] bytes, final int mtu) {
        if (bytes == null) return null;
        var blist = new LinkedList<byte[]>();
        int offset = 0;
        var b = ByteBuffer.wrap(bytes);
        while (b.remaining() > 0) {
            var chunkSize = Math.min(b.remaining(), mtu - 7);
            var pBytes = new byte[chunkSize];
            b.get(pBytes);
            var chunk =
                   T4Updateencode(pBytes,offset,offset == 0 && b.remaining() > 0,false) ;
//                    builder().bytes(pBytes).offset(offset).firstFragment(offset == 0 && b.remaining() > 0).build().encode();
            if (chunk != null) {
                blist.add(chunk);
            }
            offset += chunkSize;
        }
        if (blist.size() > 1) {
            blist.addLast(
                    T4Updateencode(bytes,0,false,true));
//                    builder().bytes(bytes).lastFragment(true).build().encode());
        } else
            // fragmented
            if (blist.size() == 0) {
                //  empty
                blist.addFirst( T4Updateencode(null,0,false,false));
            }
        return blist;
    }

    public void writeToLinkLayer(final byte[] bytes) {
        if (!tag.isConnected()) {
            Log.d(TAG, "Tag lost (write)");
            return;
        }
//        var packets = T4Update.builder().bytes(bytes).build().encodeForMtu(mlcMax);
        var packets = updateEncodeForMtu(bytes,mlcMax);
        for (var p : packets) {
            if (!transceiveOkay(p, "Error during packet write")) {
                break;
            }
        }
    }
    /*
    private byte[] getT4read(int offset, int len) {
        return T4Read.builder().offset(offset).length(len).build().encode();
    } */

static   private byte[] getT4read(int offset, int len) {
                 byte[] uit={(byte)0,(byte)176,(byte)(0xFF&((0xFF00&offset)>>8)),(byte)(offset&0xFF),(byte)len};
                 return uit;
        }
 static private List<byte[]> readEncodeForMtu(int offset,int length,final int mtu) {
      //  var blist = new LinkedList<byte[]>();
     int arlen= (length+mtu-1)/mtu;
        var blist = new ArrayList<byte[]>(arlen);

        int thisLength = length;
        int thisOffset = offset;
        while (thisLength > 0) {
            var thisRead = Math.min(thisLength, mtu);
            blist.add(getT4read(thisOffset,thisRead));
            thisOffset += thisRead;
            thisLength -= thisRead;
        }
        Log.i(TAG,"arlen="+arlen+" blist.size()="+blist.size());
        return blist;
    }

    public byte[] readFromLinkLayer() {
       var readLength = getT4read(0,2);
        for (int i = 0; i < MAX_RETRY; i++) {
            if (!tag.isConnected()) {
                Log.d(TAG, "Tag lost");
                return null;
            } else {
                tag.setTimeout(3000);
            }
               var lengthResult = t4Transceive(readLength);
               if (lengthResult == null) {
                   Log.d(TAG, "Failed to transceive when reading length");
                   return null;
               }
               if (!lengthResult.isOkay()) {
                   Log.d(TAG, "Invalid response when reading length");
                   continue;
               }

                var readLen = lengthResult.asInteger();
               Log.d(TAG, "Reading data of length: " + readLen);

            /*var read = new T4Read(2,readLen);
            var reads = read.encodeForMtu(Math.min(MAX_READ_SIZE_PARAMETER, mleMax)); */
            var reads= readEncodeForMtu(2,readLen,Math.min(MAX_READ_SIZE_PARAMETER, mleMax));

            T4Reply reply = null;
            for (var readCmd : reads) {
                for (int retry = 0; retry < MAX_RETRY; retry++) {
                    reply = t4Transceive(readCmd, reply);
                    if (reply == null) {
                        Log.d(TAG, "Read transceive fully failed");
                        continue;
                        }
                    if (reply.isOkay())
                            break;
                    try {
                        Thread.sleep(RETRY_PAUSE); //TODO: called RETRY, but result is added to reply
                    } catch (InterruptedException e) {
                        //
                        Log.i(TAG,"Interupted");
                        }
                    }
            }

            if (reply != null && reply.isOkay()) {
                var blen = reply.bytes.length; //bytes of failed attempt also added
                if (blen == readLen) {
                    Log.d(TAG, "Successfully read: " + blen + " bytes");
                    return reply.bytes;
                } else {
                    Log.e(TAG, "Read length mismatch " + blen + " vs " + readLen);
                }
            }

        } // end for
        return null;
    }

    public boolean readContainerData() {
        var containerSize = 15;
        var reply = t4Transceive(getT4read(0,containerSize));
        if (reply == null) {
            Log.d(TAG, "Failed to read container data (null reply)");
            return false;
        }
        if (!reply.isOkay()) {
            Log.d(TAG, "Failed to read container data (not okay)");
            return false;
        }
        var data = reply.bytes;
        if (d) Log.d(TAG, "Container data: " + HexDump.dumpHexString(data));

        if (data.length == containerSize) {
            var b = ByteBuffer.wrap(data);
            var cclen = getUnsignedShort(b);
            var mapping = getUnsignedByte(b);
            mleMax = getUnsignedShort(b);
            Log.d(TAG, "mleMax: " + mleMax);
            mlcMax = getUnsignedShort(b);
            Log.d(TAG, "mlcMax: " + mlcMax);
            var t = getUnsignedByte(b);
            var l = getUnsignedByte(b);
            var ident = getUnsignedShort(b);
            var nmax = getUnsignedShort(b);
            var rsec = getUnsignedByte(b);
            var wsec = getUnsignedByte(b);
            return true;
        } else {
            Log.e(TAG, "Read container data fails length check: " + HexDump.dumpHexString(data));
        }
        return false;
    }

    public boolean doNeededSelection() {
        return (transceiveOkay(SA, "Failed to select application")
                && transceiveOkay(SC, "Failed to select container")
                && readContainerData()
                && transceiveOkay(SN, "Failed to select ndef"));
    }
}
