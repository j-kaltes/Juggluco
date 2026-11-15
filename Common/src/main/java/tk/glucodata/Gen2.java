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
/*      Fri Jan 27 15:30:31 CET 2023                                                 */


package tk.glucodata;

import android.nfc.Tag;
import android.nfc.tech.NfcV;

import java.util.Arrays;

import static tk.glucodata.AlgNfcV.goodnfc;
import static tk.glucodata.AlgNfcV.issuenfc;
import static tk.glucodata.Natives.USenabledStreaming;
import static tk.glucodata.Natives.abbottinit;

class Gen2 {
    static final boolean USE_ROOT_SLOT_METADATA = true;

    private static final String LOG_ID = "Gen2";
    private static final byte NFC_FLAGS = 2;
    private static final byte ABBOTT_COMMAND = (byte) 0xa1;
    private static final byte ABBOTT_MANUFACTURER = 7;
    private static final byte ACTIVATE = 0x1b;
    private static final byte ENABLE_STREAMING = 0x1e;
    private static final byte AUTHENTICATE_FRAM_READ = 0x1f;
    private static final byte READ_CHALLENGE = 0x20;
    private static final byte READ_BLOCKS = 0x21;
    private static final byte READ_ROOT_SLOT = 0x22;
    private static final int BLOCK_BYTES = 8;
    private static final int AUTH_REQUEST_BYTES = 19;
    private static final int BLOCK_REPLY_HEADER_BYTES = 9;

    static int getversion(byte[] info) {
        return Natives.getinfogen(info);
    }

    /** Read the sensor challenge and fill the command that answers it. */
    private static int prepareNfcCommand(NfcV nfc, byte[] uid, byte command, byte[] request) {
        int rootSlot = 0;
        if (USE_ROOT_SLOT_METADATA) {
            byte[] rootReply = issuenfc(nfc, new byte[]{NFC_FLAGS, ABBOTT_COMMAND, uid[6], READ_ROOT_SLOT});
            if (!goodnfc(rootReply) || rootReply.length < 5) {
                Log.e(LOG_ID, "Read authentication root slot failed");
                return -1;
            }
            rootSlot = (rootReply[3] & 0xff) | ((rootReply[4] & 0xff) << 8);
        }
        byte[] challengeReply = issuenfc(nfc, new byte[]{NFC_FLAGS, ABBOTT_COMMAND, uid[6], READ_CHALLENGE});
        if (!goodnfc(challengeReply)) return -1;

        byte[] challenge = Arrays.copyOfRange(challengeReply, 1, challengeReply.length);
        int session = Gen2Protocol.openSession(rootSlot, uid);
        if (session < 0) return session;
        boolean prepared = false;
        try {
            byte[] authentication = Gen2Protocol.buildNfcAuthRequest(session, command, challenge);
            if (authentication == null || authentication.length != AUTH_REQUEST_BYTES) return -99;
            System.arraycopy(authentication, 0, request, 0, authentication.length);
            request[0] = NFC_FLAGS;
            request[1] = ABBOTT_COMMAND;
            request[2] = ABBOTT_MANUFACTURER;
            request[3] = command;
            prepared = true;
            return session;
        } finally {
            if (!prepared) Gen2Protocol.closeSession(session);
        }
    }

    static byte[] readAuthenticatedFram(NfcV nfc, byte[] uid, int offset, int length) {
        if (!abbottinit()) {
            Log.e(LOG_ID, "abbottinit failed");
            return null;
        }
        byte[] request = new byte[AUTH_REQUEST_BYTES];
        int session = prepareNfcCommand(nfc, uid, AUTHENTICATE_FRAM_READ, request);
        if (session < 0) return null;
        try {
            byte[] response = issuenfc(nfc, request);
            if (!goodnfc(response)) return null;
            int status = Gen2Protocol.acceptNfcAuthentication(
                    session, Arrays.copyOfRange(response, 1, response.length));
            if (status < 0) {
                Log.e(LOG_ID, "FRAM authentication failed: " + status);
                return null;
            }
            return readFramBlocks(nfc, session, offset, length);
        } finally {
            if (session > 0) Gen2Protocol.closeSession(session);
        }
    }

    private static byte[] readFramBlocks(NfcV nfc, int session, int offset, int length) {
        int firstBlock = offset / BLOCK_BYTES;
        int startInBlock = offset % BLOCK_BYTES;
        int endInBuffer = startInBlock + length;
        int blockCount = (endInBuffer + BLOCK_BYTES - 1) / BLOCK_BYTES;
        byte[] encrypted = new byte[blockCount * BLOCK_BYTES];

        for (int block = 0; block < blockCount;) {
            int count = Math.min(3, blockCount - block);
            int bytes = count * BLOCK_BYTES;
            byte[] reply = issuenfc(nfc, new byte[]{NFC_FLAGS, ABBOTT_COMMAND, ABBOTT_MANUFACTURER,
                    READ_BLOCKS, (byte) (firstBlock + block), (byte) (count - 1)});
            if (!goodnfc(reply) || reply.length < BLOCK_REPLY_HEADER_BYTES + bytes) return null;
            System.arraycopy(reply, BLOCK_REPLY_HEADER_BYTES, encrypted, block * BLOCK_BYTES, bytes);
            block += count;
        }
        byte[] fram = Gen2Protocol.decryptNfcBlocks(session, firstBlock, blockCount, encrypted);
        if (fram == null || fram.length < endInBuffer) return null;
        if (startInBlock == 0 && endInBuffer == fram.length) return fram;
        return Arrays.copyOfRange(fram, startInBlock, endInBuffer);
    }

    static boolean activate(Tag tag) {
        try (NfcV nfc = NfcV.get(tag)) {
            nfc.connect();
            byte[] request = new byte[AUTH_REQUEST_BYTES];
            int session = prepareNfcCommand(nfc, tag.getId(), ACTIVATE, request);
            if (session < 0) return false;
            try {
                byte[] response = issuenfc(nfc, request);
                if (response == null || response.length == 0) return false;
                byte[] reply = Gen2Protocol.readNfcCommandReply(
                        session, Arrays.copyOfRange(response, 1, response.length), 5);
                return reply != null && reply.length == 5;
            } finally {
                Gen2Protocol.closeSession(session);
            }
        } catch (Throwable error) {
            Log.stack(LOG_ID, "Sensor activation failed", error);
            return false;
        }
    }

    static boolean enableStreaming(Tag tag) {
        try (NfcV nfc = NfcV.get(tag)) {
            nfc.connect();
            byte[] uid = tag.getId();
            byte[] request = new byte[AUTH_REQUEST_BYTES];
            int session = prepareNfcCommand(nfc, uid, ENABLE_STREAMING, request);
            if (session < 0) return false;
            try {
                byte[] response = issuenfc(nfc, request);
                if (!goodnfc(response)) return false;
                byte[] reply = Gen2Protocol.readNfcCommandReply(
                        session, Arrays.copyOfRange(response, 1, response.length), 9);
                if (reply == null || reply.length != 9) return false;
                int metadataLength = USE_ROOT_SLOT_METADATA ? 9 : 7;
                byte[] metadata = Gen2Protocol.exportStreamingMetadata(session, metadataLength);
                if (metadata == null || metadata.length != metadataLength) return false;

                // BLE address comes first in the reply. The other three reply
                // bytes follow the seven-byte prefix and salt in provisioning.
                byte[] address = Arrays.copyOf(reply, 6);
                byte[] provisioning = new byte[metadataLength + 3];
                System.arraycopy(metadata, 0, provisioning, 0, 7);
                System.arraycopy(reply, 6, provisioning, 7, 3);
                if (USE_ROOT_SLOT_METADATA) {
                    System.arraycopy(metadata, 7, provisioning, 10, 2);
                }
                USenabledStreaming(uid, provisioning, address);
                return true;
            } finally {
                if (session > 0) Gen2Protocol.closeSession(session);
            }
        } catch (Throwable error) {
            Log.stack(LOG_ID, "Enable streaming failed", error);
            return false;
        }
    }
}
