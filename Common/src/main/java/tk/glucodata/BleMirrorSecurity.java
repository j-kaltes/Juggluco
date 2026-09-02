/*      This file is part of Juggluco, an Android app to receive and display
 *      glucose values from continuous glucose monitors.
 *
 *      Copyright (C) 2026 Jaap Korthals Altes <jaapkorthalsaltes@gmail.com>
 *
 *      Juggluco is free software: you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation, either version 3 of the License, or
 *      (at your option) any later version.
 */
package tk.glucodata;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.security.GeneralSecurityException;
import java.security.MessageDigest;
import java.security.SecureRandom;
import java.util.Arrays;

import javax.crypto.Cipher;
import javax.crypto.Mac;
import javax.crypto.spec.GCMParameterSpec;
import javax.crypto.spec.SecretKeySpec;

/** Application authentication and encryption for an unbonded BLE mirror link. */
final class BleMirrorSecurity {
    static final int PROTOCOL_VERSION=3;
    static final int DISCOVERY_TOKEN_BYTES=8;
    static final int HANDSHAKE_NONCE_BYTES=16;
    static final int HANDSHAKE_TAG_BYTES=16;
    static final int MESSAGE_COUNTER_BYTES=8;
    static final int MESSAGE_TAG_BYTES=16;

    private static final byte[] KEY_CONTEXT="Juggluco BLE mirror key v3".getBytes(StandardCharsets.UTF_8);
    private static final byte[] DISCOVERY_CONTEXT="Juggluco BLE discovery v3".getBytes(StandardCharsets.UTF_8);
    private static final byte[] ENDPOINT_HINT_CONTEXT="Juggluco BLE endpoint hint v3".getBytes(StandardCharsets.UTF_8);
    private static final byte[] HELLO_CONTEXT="Juggluco BLE hello v3".getBytes(StandardCharsets.UTF_8);
    private static final byte[] READY_CONTEXT="Juggluco BLE ready v3".getBytes(StandardCharsets.UTF_8);
    private static final byte[] SESSION_CONTEXT="Juggluco BLE session v3".getBytes(StandardCharsets.UTF_8);
    private static final byte[] MESSAGE_CONTEXT="Juggluco BLE message v3".getBytes(StandardCharsets.UTF_8);
    private static final int CLIENT_NONCE_PREFIX=0x434c4e54; // CLNT
    private static final int SERVER_NONCE_PREFIX=0x53525652; // SRVR

    private BleMirrorSecurity() {}

    static byte[] randomNonce(SecureRandom random) {
        final byte[] nonce=new byte[HANDSHAKE_NONCE_BYTES];
        random.nextBytes(nonce);
        return nonce;
    }

    static byte[] masterKey(String label,String password) throws GeneralSecurityException {
        if(label==null||label.isEmpty()||password==null||password.isEmpty())
            throw new GeneralSecurityException("A BLE mirror label and password are required");
        final MessageDigest digest=MessageDigest.getInstance("SHA-256");
        digest.update(KEY_CONTEXT);
        digest.update((byte)0);
        digest.update(label.getBytes(StandardCharsets.UTF_8));
        digest.update((byte)0);
        digest.update(password.getBytes(StandardCharsets.UTF_8));
        return digest.digest();
    }

    static byte[] discoveryToken(byte[] masterKey) throws GeneralSecurityException {
        return Arrays.copyOf(hmac(masterKey,DISCOVERY_CONTEXT),DISCOVERY_TOKEN_BYTES);
    }

    /** Short integrity tag for the compact scan-response IPv4 endpoint hint. */
    static byte[] endpointHintTag(byte[] masterKey,byte[] hint,int length)
            throws GeneralSecurityException {
        if(masterKey==null||hint==null||length<=0||length>hint.length)
            throw new GeneralSecurityException("Invalid BLE endpoint hint");
        final Mac mac=newMac(masterKey);
        mac.update(ENDPOINT_HINT_CONTEXT);
        mac.update((byte)0);
        mac.update(hint,0,length);
        return Arrays.copyOf(mac.doFinal(),4);
    }

    static byte[] hello(byte[] masterKey,String label,boolean wearable,int hostIndex,byte[] clientNonce)
            throws GeneralSecurityException {
        return identity(masterKey,label,wearable,hostIndex,clientNonce,null,false);
    }

    static byte[] ready(byte[] masterKey,String label,boolean wearable,int hostIndex,
                        byte[] serverNonce,byte[] clientNonce) throws GeneralSecurityException {
        return identity(masterKey,label,wearable,hostIndex,serverNonce,clientNonce,true);
    }

    private static byte[] identity(byte[] masterKey,String label,boolean wearable,int hostIndex,
                                   byte[] nonce,byte[] peerNonce,boolean ready)
            throws GeneralSecurityException {
        if(hostIndex<0||hostIndex>0xffff||nonce==null||nonce.length!=HANDSHAKE_NONCE_BYTES)
            throw new GeneralSecurityException("Invalid BLE mirror identity");
        if(ready&&(peerNonce==null||peerNonce.length!=HANDSHAKE_NONCE_BYTES))
            throw new GeneralSecurityException("Missing BLE mirror client nonce");
        final byte[] labelBytes=label.getBytes(StandardCharsets.UTF_8);
        if(labelBytes.length==0||labelBytes.length>255)
            throw new GeneralSecurityException("Invalid BLE mirror label length");
        final int unsignedLength=5+HANDSHAKE_NONCE_BYTES+labelBytes.length;
        final byte[] result=new byte[unsignedLength+HANDSHAKE_TAG_BYTES];
        final ByteBuffer output=ByteBuffer.wrap(result).order(ByteOrder.LITTLE_ENDIAN);
        output.put((byte)PROTOCOL_VERSION).put((byte)(wearable?1:0)).putShort((short)hostIndex)
                .put((byte)labelBytes.length).put(nonce).put(labelBytes);
        final byte[] tag=handshakeTag(masterKey,ready,peerNonce,result,unsignedLength);
        System.arraycopy(tag,0,result,unsignedLength,HANDSHAKE_TAG_BYTES);
        return result;
    }

    static String peekLabel(byte[] data) {
        if(data==null||data.length<5+HANDSHAKE_NONCE_BYTES+1+HANDSHAKE_TAG_BYTES||
                (data[0]&0xff)!=PROTOCOL_VERSION)
            return null;
        final int length=data[4]&0xff;
        if(length==0||data.length!=5+HANDSHAKE_NONCE_BYTES+length+HANDSHAKE_TAG_BYTES)
            return null;
        return new String(data,5+HANDSHAKE_NONCE_BYTES,length,StandardCharsets.UTF_8);
    }

    static Identity verifyHello(byte[] masterKey,byte[] data) throws GeneralSecurityException {
        return verifyIdentity(masterKey,data,null,false);
    }

    static Identity verifyReady(byte[] masterKey,byte[] data,byte[] clientNonce)
            throws GeneralSecurityException {
        return verifyIdentity(masterKey,data,clientNonce,true);
    }

    private static Identity verifyIdentity(byte[] masterKey,byte[] data,byte[] peerNonce,boolean ready)
            throws GeneralSecurityException {
        final String label=peekLabel(data);
        if(label==null)
            throw new GeneralSecurityException("Invalid BLE mirror identity packet");
        if(ready&&(peerNonce==null||peerNonce.length!=HANDSHAKE_NONCE_BYTES))
            throw new GeneralSecurityException("Missing BLE mirror client nonce");
        final int unsignedLength=data.length-HANDSHAKE_TAG_BYTES;
        final byte[] expected=handshakeTag(masterKey,ready,peerNonce,data,unsignedLength);
        final byte[] supplied=Arrays.copyOfRange(data,unsignedLength,data.length);
        if(!MessageDigest.isEqual(expected,supplied))
            throw new GeneralSecurityException("BLE mirror authentication failed");
        final ByteBuffer input=ByteBuffer.wrap(data).order(ByteOrder.LITTLE_ENDIAN);
        input.get();
        final boolean wearable=input.get()!=0;
        final int hostIndex=input.getShort()&0xffff;
        final int length=input.get()&0xff;
        final byte[] nonce=new byte[HANDSHAKE_NONCE_BYTES];
        input.get(nonce);
        input.position(input.position()+length);
        return new Identity(label,wearable,hostIndex,nonce);
    }

    private static byte[] handshakeTag(byte[] masterKey,boolean ready,byte[] peerNonce,
                                       byte[] packet,int unsignedLength) throws GeneralSecurityException {
        final Mac mac=newMac(masterKey);
        mac.update(ready?READY_CONTEXT:HELLO_CONTEXT);
        mac.update((byte)0);
        if(ready)
            mac.update(peerNonce);
        mac.update(packet,0,unsignedLength);
        return Arrays.copyOf(mac.doFinal(),HANDSHAKE_TAG_BYTES);
    }

    static byte[] sessionKey(byte[] masterKey,byte[] clientNonce,byte[] serverNonce)
            throws GeneralSecurityException {
        if(clientNonce==null||clientNonce.length!=HANDSHAKE_NONCE_BYTES||
                serverNonce==null||serverNonce.length!=HANDSHAKE_NONCE_BYTES)
            throw new GeneralSecurityException("Invalid BLE mirror session nonces");
        final Mac mac=newMac(masterKey);
        mac.update(SESSION_CONTEXT);
        mac.update((byte)0);
        mac.update(clientNonce);
        mac.update(serverNonce);
        return mac.doFinal();
    }

    static byte[] seal(byte[] sessionKey,boolean fromClient,long counter,String path,byte[] plaintext)
            throws GeneralSecurityException {
        if(counter<=0)
            throw new GeneralSecurityException("Invalid BLE mirror message counter");
        final byte[] nonce=messageNonce(fromClient,counter);
        final Cipher cipher=messageCipher(Cipher.ENCRYPT_MODE,sessionKey,nonce,path);
        final byte[] encrypted=cipher.doFinal(plaintext);
        final ByteBuffer output=ByteBuffer.allocate(MESSAGE_COUNTER_BYTES+encrypted.length)
                .order(ByteOrder.BIG_ENDIAN);
        output.putLong(counter).put(encrypted);
        return output.array();
    }

    static Opened open(byte[] sessionKey,boolean fromClient,long previousCounter,String path,byte[] protectedData)
            throws GeneralSecurityException {
        if(protectedData==null||protectedData.length<MESSAGE_COUNTER_BYTES+MESSAGE_TAG_BYTES)
            throw new GeneralSecurityException("Truncated BLE mirror protected message");
        final ByteBuffer input=ByteBuffer.wrap(protectedData).order(ByteOrder.BIG_ENDIAN);
        final long counter=input.getLong();
        if(counter!=previousCounter+1L||counter<=0)
            throw new GeneralSecurityException("Unexpected BLE mirror message counter");
        final byte[] encrypted=new byte[input.remaining()];
        input.get(encrypted);
        final Cipher cipher=messageCipher(Cipher.DECRYPT_MODE,sessionKey,
                messageNonce(fromClient,counter),path);
        return new Opened(counter,cipher.doFinal(encrypted));
    }

    private static Cipher messageCipher(int mode,byte[] sessionKey,byte[] nonce,String path)
            throws GeneralSecurityException {
        if(sessionKey==null||sessionKey.length<16)
            throw new GeneralSecurityException("Missing BLE mirror session key");
        final Cipher cipher=Cipher.getInstance("AES/GCM/NoPadding");
        cipher.init(mode,new SecretKeySpec(Arrays.copyOf(sessionKey,16),"AES"),
                new GCMParameterSpec(MESSAGE_TAG_BYTES*8,nonce));
        cipher.updateAAD(MESSAGE_CONTEXT);
        cipher.updateAAD(new byte[]{0});
        cipher.updateAAD(path.getBytes(StandardCharsets.UTF_8));
        return cipher;
    }

    private static byte[] messageNonce(boolean fromClient,long counter) {
        return ByteBuffer.allocate(12).order(ByteOrder.BIG_ENDIAN)
                .putInt(fromClient?CLIENT_NONCE_PREFIX:SERVER_NONCE_PREFIX).putLong(counter).array();
    }

    private static byte[] hmac(byte[] key,byte[] data) throws GeneralSecurityException {
        final Mac mac=newMac(key);
        return mac.doFinal(data);
    }

    private static Mac newMac(byte[] key) throws GeneralSecurityException {
        final Mac mac=Mac.getInstance("HmacSHA256");
        mac.init(new SecretKeySpec(key,"HmacSHA256"));
        return mac;
    }

    static final class Identity {
        final String label;
        final boolean wearable;
        final int hostIndex;
        final byte[] nonce;

        Identity(String label,boolean wearable,int hostIndex,byte[] nonce) {
            this.label=label;
            this.wearable=wearable;
            this.hostIndex=hostIndex;
            this.nonce=nonce;
        }
    }

    static final class Opened {
        final long counter;
        final byte[] data;

        Opened(long counter,byte[] data) {
            this.counter=counter;
            this.data=data;
        }
    }
}
