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

import java.io.ByteArrayOutputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;

/** Fragmentation independent of Android's Bluetooth classes, so it can be tested. */
final class BleMirrorCodec {
    static final int HEADER_SIZE=12;
    static final int MAX_MESSAGE_SIZE=1024*1024;
    // Bluetooth permits an attribute value of at most 512 bytes.  An ATT MTU
    // of 517 has 514 payload bytes, but older Android GATT servers can accept
    // a 514-byte indication and then never report onNotificationSent().
    static final int MAX_ATTRIBUTE_VALUE=512;
    private static final byte MAGIC0='J';
    private static final byte MAGIC1='G';
    private static final byte VERSION=1;

    static final class Message {
        private final String path;
        private final byte[] data;
        private final int session;
        private final int messageId;

        Message(String path,byte[] data,int session,int messageId) {
            this.path=path;
            this.data=data;
            this.session=session;
            this.messageId=messageId;
        }
        String path() { return path; }
        byte[] data() { return data; }
        int session() { return session; }
        int messageId() { return messageId; }
    }

    static List<byte[]> fragments(String path,byte[] data,int mtu,int session,int messageId) {
        if(path==null||data==null)
            throw new NullPointerException("BLE mirror path and data are required");
        final byte[] pathBytes=path.getBytes(StandardCharsets.UTF_8);
        if(pathBytes.length==0||pathBytes.length>255)
            throw new IllegalArgumentException("BLE mirror path length is invalid");
        if(data.length>MAX_MESSAGE_SIZE-1-pathBytes.length)
            throw new IllegalArgumentException("BLE mirror message is too large");
        final int logicalLength=1+pathBytes.length+data.length;
        final byte[] logical=new byte[logicalLength];
        logical[0]=(byte)pathBytes.length;
        System.arraycopy(pathBytes,0,logical,1,pathBytes.length);
        System.arraycopy(data,0,logical,1+pathBytes.length,data.length);

        // An ATT value has MTU-3 bytes, and the Bluetooth attribute-value
        // limit is 512 bytes. A 23-byte default MTU still leaves eight bytes
        // per fragment with this compact header.
        final int valueSize=Math.min(MAX_ATTRIBUTE_VALUE,mtu-3);
        final int bodySize=Math.max(1,valueSize-HEADER_SIZE);
        final int count=Math.max(1,(logicalLength+bodySize-1)/bodySize);
        if(count>0xffff)
            throw new IllegalArgumentException("BLE mirror message needs too many fragments: "+count);
        final ArrayList<byte[]> result=new ArrayList<>(count);
        for(int index=0,offset=0;index<count;++index) {
            final int length=Math.min(bodySize,logicalLength-offset);
            final byte[] fragment=new byte[HEADER_SIZE+length];
            final ByteBuffer header=ByteBuffer.wrap(fragment).order(ByteOrder.LITTLE_ENDIAN);
            header.put(MAGIC0).put(MAGIC1).put(VERSION);
            header.put((byte)((index==0?1:0)|(index==count-1?2:0)));
            header.putShort((short)session).putShort((short)messageId);
            header.putShort((short)index).putShort((short)count);
            System.arraycopy(logical,offset,fragment,HEADER_SIZE,length);
            offset+=length;
            result.add(fragment);
        }
        return result;
    }

    /**
     * Return the logical path when {@code fragment} is a complete one-fragment
     * message, otherwise {@code null}. The path is outside the protected payload,
     * so it can be inspected before an authenticated session exists.
     */
    static String singleFragmentPath(byte[] fragment) {
        if(fragment==null||fragment.length<HEADER_SIZE+2)
            return null;
        final ByteBuffer header=ByteBuffer.wrap(fragment).order(ByteOrder.LITTLE_ENDIAN);
        if(header.get()!=MAGIC0||header.get()!=MAGIC1||header.get()!=VERSION)
            return null;
        final int flags=header.get()&0xff;
        header.getShort(); // session
        header.getShort(); // message id
        final int index=header.getShort()&0xffff;
        final int count=header.getShort()&0xffff;
        if(flags!=3||index!=0||count!=1)
            return null;
        final int pathLength=fragment[HEADER_SIZE]&0xff;
        if(pathLength==0||HEADER_SIZE+1+pathLength>fragment.length)
            return null;
        return new String(fragment,HEADER_SIZE+1,pathLength,StandardCharsets.UTF_8);
    }

    /**
     * True when this is fragment zero of a message whose path can still be
     * {@code expectedPath}.  At the default ATT MTU (23), /blehello itself is
     * fragmented and only a prefix of its path is present in fragment zero.
     * Requiring a complete one-fragment path here made a fresh MTU-23 handshake
     * look like stale unauthenticated traffic and the server answered ATT error 6.
     */
    static boolean firstFragmentCanBePath(byte[] fragment,String expectedPath) {
        if(fragment==null||expectedPath==null||fragment.length<HEADER_SIZE+2)
            return false;
        final byte[] expected=expectedPath.getBytes(StandardCharsets.UTF_8);
        final ByteBuffer header=ByteBuffer.wrap(fragment).order(ByteOrder.LITTLE_ENDIAN);
        if(header.get()!=MAGIC0||header.get()!=MAGIC1||header.get()!=VERSION)
            return false;
        final int flags=header.get()&0xff;
        header.getShort(); // session
        header.getShort(); // message id
        final int index=header.getShort()&0xffff;
        final int count=header.getShort()&0xffff;
        if(index!=0||count==0||(flags&1)==0)
            return false;
        final int pathLength=fragment[HEADER_SIZE]&0xff;
        if(pathLength!=expected.length)
            return false;
        final int available=Math.min(expected.length,fragment.length-(HEADER_SIZE+1));
        if(available<=0)
            return false;
        for(int i=0;i<available;++i)
            if(fragment[HEADER_SIZE+1+i]!=expected[i])
                return false;
        return true;
    }

    static final class Decoder {
        private int session=-1;
        private int messageId=-1;
        private int fragmentCount;
        private int nextFragment;
        private ByteArrayOutputStream body;

        void reset() {
            session=-1;
            messageId=-1;
            fragmentCount=0;
            nextFragment=0;
            body=null;
        }

        boolean inProgress() {
            return body!=null;
        }

        Message accept(byte[] fragment) {
            if(fragment==null||fragment.length<HEADER_SIZE)
                return invalid();
            final ByteBuffer header=ByteBuffer.wrap(fragment).order(ByteOrder.LITTLE_ENDIAN);
            if(header.get()!=MAGIC0||header.get()!=MAGIC1||header.get()!=VERSION)
                return invalid();
            final int flags=header.get()&0xff;
            final int incomingSession=header.getShort()&0xffff;
            final int incomingMessage=header.getShort()&0xffff;
            final int index=header.getShort()&0xffff;
            final int count=header.getShort()&0xffff;
            if((flags&~3)!=0||count==0||index>=count||((index==0)!=((flags&1)!=0))||
                    ((index==count-1)!=((flags&2)!=0)))
                return invalid();

            if(index==0) {
                if((flags&1)==0)
                    return invalid();
                session=incomingSession;
                messageId=incomingMessage;
                fragmentCount=count;
                nextFragment=0;
                body=new ByteArrayOutputStream(Math.min(MAX_MESSAGE_SIZE,count*Math.max(1,fragment.length-HEADER_SIZE)));
            }
            if(body==null||session!=incomingSession||messageId!=incomingMessage||
                    fragmentCount!=count||index!=nextFragment)
                return invalid();
            final int length=fragment.length-HEADER_SIZE;
            if(body.size()+length>MAX_MESSAGE_SIZE)
                return invalid();
            body.write(fragment,HEADER_SIZE,length);
            ++nextFragment;
            if(index!=count-1)
                return null;
            if((flags&2)==0)
                return invalid();

            final byte[] logical=body.toByteArray();
            final int completedSession=session;
            final int completedMessage=messageId;
            reset();
            if(logical.length<1)
                return null;
            final int pathLength=logical[0]&0xff;
            if(pathLength==0||1+pathLength>logical.length)
                return null;
            final String path=new String(logical,1,pathLength,StandardCharsets.UTF_8);
            final byte[] data=new byte[logical.length-1-pathLength];
            System.arraycopy(logical,1+pathLength,data,0,data.length);
            return new Message(path,data,completedSession,completedMessage);
        }

        private Message invalid() {
            reset();
            return null;
        }
    }
}
