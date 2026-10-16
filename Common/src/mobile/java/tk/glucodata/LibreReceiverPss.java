/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2(+), Libre 3(+), Dexcom G7/ONE+,        */
/*      Sibionics GS1Sb and GS3, Accu-Chek SmartGuide, CareSens Air and              */
/*      Aidex X sensors.                                                             */
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
/*      Thu Oct 15 12:44:39 CEST 2026                                                */

package tk.glucodata;

import java.math.BigInteger;
import java.security.GeneralSecurityException;
import java.security.MessageDigest;
import java.security.PublicKey;
import java.security.interfaces.RSAPublicKey;
import java.util.Arrays;

/** Public-key verification for Android 5.x, which has no guaranteed PSS provider.
 * RFC 8017 sections 8.1.2, 9.1.2 and B.2.1: SHA-256, MGF1/SHA-256, salt length 32.
 * Does not sign, import SKB keys, or alter the authenticated message bytes.
 */
final class LibreReceiverPss {
    private LibreReceiverPss() {}

    static void verify(PublicKey key, byte[] prefix, byte[] plain, byte[] signature)
            throws GeneralSecurityException {
        if (!(key instanceof RSAPublicKey) || prefix==null || plain==null || signature==null)
            throw rejected();
        RSAPublicKey rsa=(RSAPublicKey)key;
        BigInteger n=rsa.getModulus(), e=rsa.getPublicExponent();
        if (n==null || e==null || n.signum()<=0 || !n.testBit(0) ||
                n.bitLength()<1024 || n.bitLength()>4096 ||
                e.compareTo(BigInteger.ONE)<=0 || !e.testBit(0) || e.compareTo(n)>=0)
            throw rejected();
        int emBits=n.bitLength()-1, emLen=(emBits+7)/8;
        if (signature.length!=(n.bitLength()+7)/8 || emLen<66) throw rejected();
        BigInteger s=new BigInteger(1,signature);
        if (s.compareTo(n)>=0) throw rejected();
        BigInteger m=s.modPow(e,n);
        if (m.bitLength()>8*emLen) throw rejected();
        byte[] magnitude=m.toByteArray(), em=new byte[emLen];
        // BigInteger may include a leading sign byte. I2OSP pads on the left.
        int count=Math.min(magnitude.length,emLen);
        System.arraycopy(magnitude,magnitude.length-count,em,emLen-count,count);

        int dbLen=emLen-33, unused=8*emLen-emBits;
        int firstMask=0xff>>>unused;
        if ((em[emLen-1]&255)!=0xbc || ((em[0]&255)&~firstMask)!=0) throw rejected();
        byte[] h=Arrays.copyOfRange(em,dbLen,dbLen+32);
        byte[] db=Arrays.copyOf(em,dbLen);
        MessageDigest hash=MessageDigest.getInstance("SHA-256");
        for (int offset=0,counter=0; offset<dbLen; offset+=32,++counter) {
            hash.update(h);
            hash.update(new byte[]{(byte)(counter>>>24),(byte)(counter>>>16),
                    (byte)(counter>>>8),(byte)counter});
            byte[] mask=hash.digest();
            for (int j=0;j<32 && offset+j<dbLen;++j) db[offset+j]^=mask[j];
        }
        db[0]&=(byte)firstMask;
        int delimiter=dbLen-33;
        int invalid=(db[delimiter]&255)^1;
        for (int i=0;i<delimiter;++i) invalid|=db[i]&255;
        if (invalid!=0) throw rejected();
        hash.update(prefix); hash.update(plain);
        byte[] mHash=hash.digest();
        hash.update(new byte[8]);
        hash.update(mHash);
        hash.update(db,dbLen-32,32);
        if (!MessageDigest.isEqual(h,hash.digest())) throw rejected();
    }

    // Synthetic public test vector; no Abbott/account key or credential.
    static void selftest() throws GeneralSecurityException {
        PublicKey key=java.security.KeyFactory.getInstance("RSA").generatePublic(
                new java.security.spec.RSAPublicKeySpec(new BigInteger(
                        "f78b5ec9b5c3baae284cf5b19380db0bb5cb6693910cdd121df04f8cea3ee3bb" +
                        "64f862f77b4bee3b53ae9f3d853a10fa3d84e55b1072146717494cc7be9dce31" +
                        "a9fa3d41e7079309521b63d8600620a96c038d9e2d04bdab96a8334925f85c24" +
                        "61e5fdfb27a487e89ccc5c70cc4ec4305f0331e517e92b24e5eda755b284b9dd" +
                        "d93146568843c8c362d8571a64e1831e7e0558512f60b54bdd743b73ab91e205" +
                        "c94e65e68c48834ae5755055dd345be8d5198871dde98ee0c7d0223b67e2e76f" +
                        "456254497ab6a7f81243c97a3fdd724a83f7cd6fb0ac528e6e272b57e4a95c3d" +
                        "d7589a2017af793bdbf333dde23ec9067eb29b51e9a24aa74a9b1afd4ce01b99",16),BigInteger.valueOf(65537)));
        byte[] signature=new BigInteger("66e3a1ea9f6aeaef0929cea8eb2d4eda7ff57b913d3d0308dbd7c2db661a461e" +
                        "ee4573bdf2779c8b95cea2f4cc0746375c29e84bede4bf361f8dd8c1f30aeebe" +
                        "343a8f142d445566d132bb46ad10db2f527fcbe285245c499e6455330c6dad55" +
                        "7cc324434f6a82e42f6032ec3a8ef65f9aaf26b3d61b02a532f281365d1c22c7" +
                        "7a42dcb9c3a3ed411adf4299dae699707ce24a029105c02af3c46531992ab059" +
                        "69587cd78585ddaae8708162e991231577693fdf8fd5bd46460bd8c1b7a9ec21" +
                        "ed5ef96135dc98a6032ceaad6a68d581d3daeec521050e7fe3dd5504ea07f522" +
                        "dbf48b38f6213f3a0ca459468a67deff3061516fb9cf0454fad2ddfcc6daf04b",16).toByteArray();
        if (signature.length==257) signature=Arrays.copyOfRange(signature,1,257);
        if (signature.length<256) {
            byte[] padded=new byte[256];
            System.arraycopy(signature,0,padded,256-signature.length,signature.length);
            signature=padded;
        }
        byte[] plain="Libre receiver PSS offline self-test".getBytes(java.nio.charset.StandardCharsets.UTF_8);
        verify(key,new byte[0],plain,signature);
        plain[0]^=1;
        try { verify(key,new byte[0],plain,signature); }
        catch (GeneralSecurityException expected) { return; }
        throw new GeneralSecurityException("PSS self-test accepted altered plaintext");
    }

    private static GeneralSecurityException rejected() {
        return new GeneralSecurityException("Server RSA-PSS signature rejected");
    }
}
