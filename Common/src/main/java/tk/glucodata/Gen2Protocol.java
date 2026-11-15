package tk.glucodata;

/** Names for the operations shared by the Abbott and independent Gen2 backends. */
final class Gen2Protocol {
    private static final byte NFC = 0;
    private static final byte BLE = 1;
    private static final int OPEN_SESSION = 0x7120;
    private static final int ACCEPT_AUTHENTICATION = 0x7319;
    private static final int CLOSE_SESSION = 0x9218;
    private static final int BUILD_NFC_AUTH_REQUEST = 0x1928;
    private static final int BUILD_BLE_AUTH_REQUEST = 0x1969;
    private static final int READ_COMMAND_REPLY = 0x5731;
    private static final int EXPORT_STREAMING_METADATA = 0x1801;
    private static final int DECRYPT_NFC_BLOCKS = 0x1978;
    private static final int DECRYPT_BLE_FRAME = 0x0305;

    private Gen2Protocol() {}

    static int openSession(int rootSlot, byte[] uid) {
        return Natives.V1(OPEN_SESSION, rootSlot, uid, null);
    }

    static void closeSession(int session) {
        Natives.V1(CLOSE_SESSION, session, null, null);
    }

    static byte[] buildNfcAuthRequest(int session, byte command, byte[] challenge) {
        return Natives.V2(BUILD_NFC_AUTH_REQUEST, session, new byte[]{NFC, command}, challenge);
    }

    static byte[] buildBleAuthRequest(int session, byte command, byte[] provisioning,
                                      byte[] challenge) {
        final int prefixLength = 10;
        byte[] input = new byte[prefixLength + challenge.length];
        System.arraycopy(provisioning, 0, input, 0, prefixLength);
        System.arraycopy(challenge, 0, input, prefixLength, challenge.length);
        return Natives.V2(BUILD_BLE_AUTH_REQUEST, session, new byte[]{BLE, command}, input);
    }

    static int acceptNfcAuthentication(int session, byte[] response) {
        return Natives.V1(ACCEPT_AUTHENTICATION, session, new byte[]{NFC}, response);
    }

    static int acceptBleAuthentication(int session, byte[] response) {
        return Natives.V1(ACCEPT_AUTHENTICATION, session, new byte[]{BLE}, response);
    }

    static byte[] readNfcCommandReply(int session, byte[] response, int payloadLength) {
        return Natives.V2(READ_COMMAND_REPLY, session,
                          new byte[]{NFC, (byte) payloadLength}, response);
    }

    static byte[] exportStreamingMetadata(int session, int length) {
        return Natives.V2(EXPORT_STREAMING_METADATA, session, new byte[]{(byte) length}, null);
    }

    static byte[] decryptNfcBlocks(int session, int firstBlock, int count, byte[] encrypted) {
        return Natives.V2(DECRYPT_NFC_BLOCKS, session,
                          new byte[]{(byte) firstBlock, (byte) count}, encrypted);
    }

    static byte[] decryptBleFrame(int session, byte[] encrypted) {
        return Natives.V2(DECRYPT_BLE_FRAME, session, encrypted, null);
    }
}
