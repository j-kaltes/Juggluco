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
/*      Thu Oct 15 12:50:12 CEST 2026                                                */

/* SPDX-License-Identifier: GPL-3.0-or-later */
package tk.glucodata;

import java.math.BigInteger;
import java.security.AlgorithmParameters;
import java.security.GeneralSecurityException;
import java.security.KeyFactory;
import java.security.KeyPair;
import java.security.KeyPairGenerator;
import java.security.MessageDigest;
import java.security.PrivateKey;
import java.security.PublicKey;
import java.security.Signature;
import java.security.interfaces.ECPublicKey;
import java.security.spec.ECFieldFp;
import java.security.spec.ECGenParameterSpec;
import java.security.spec.ECParameterSpec;
import java.security.spec.ECPoint;
import java.security.spec.ECPrivateKeySpec;
import java.security.spec.ECPublicKeySpec;
import java.util.Arrays;

import javax.crypto.AEADBadTagException;
import javax.crypto.Cipher;
import javax.crypto.KeyAgreement;
import javax.crypto.spec.SecretKeySpec;

/** Independent Lingo version-3 authentication. Sensor data uses the existing native CCM path. */
final class LingoCrypto {
    private static final byte[] SAVED_MAGIC = {'J', 'L', 'N', 'G', 'K', 'A', '0', '1'};
    private static final int SAVED_SIZE = 149;
    private static final int CHECKSUM_OFFSET = SAVED_SIZE - 32;
    private final ECParameterSpec curve;
    private final KeyFactory keyFactory;
    private final PrivateKey appPrivate;
    private final PublicKey patchSigningKey;
    private PublicKey patchPublic;
    private KeyPair ephemeral;
    private byte[] authorizationKey;
    private int securityVersion = -1;

    LingoCrypto() throws GeneralSecurityException {
        AlgorithmParameters parameters = AlgorithmParameters.getInstance("EC");
        parameters.init(new ECGenParameterSpec("secp256r1"));
        curve = parameters.getParameterSpec(ECParameterSpec.class);
        keyFactory = KeyFactory.getInstance("EC");
        appPrivate = keyFactory.generatePrivate(new ECPrivateKeySpec(
                new BigInteger(1, LingoCredentials.APP_PRIVATE), curve));
        patchSigningKey = publicKey(LingoCredentials.PATCH_SIGNING_PUBLIC);
    }

    static int getKeyIndexFromVersion(int version) {
        return version == 3 ? 0 : -1;
    }

    void initECDH(byte[] saved, int version) throws GeneralSecurityException {
        clearAuthorization();
        ephemeral = null;
        patchPublic = null;
        securityVersion = -1;
        if (getKeyIndexFromVersion(version) < 0)
            throw new GeneralSecurityException("Unsupported Lingo security version " + version);
        if (saved != null) {
            if (!canResume(saved, version))
                throw new GeneralSecurityException("Lingo authorization record needs full authentication");
            authorizationKey = Arrays.copyOfRange(saved, 12, 28);
        }
        securityVersion = version;
    }

    static boolean canResume(byte[] saved, int version) {
        if (getKeyIndexFromVersion(version) < 0 || saved == null || saved.length != SAVED_SIZE)
            return false;
        for (int i = 0; i < SAVED_MAGIC.length; ++i)
            if (saved[i] != SAVED_MAGIC[i]) return false;
        if (saved[8] != version || saved[9] != 0 || saved[10] != 0 || saved[11] != 0)
            return false;
        try {
            MessageDigest digest = MessageDigest.getInstance("SHA-256");
            digest.update(saved, 0, CHECKSUM_OFFSET);
            return MessageDigest.isEqual(digest.digest(),
                    Arrays.copyOfRange(saved, CHECKSUM_OFFSET, SAVED_SIZE));
        } catch (GeneralSecurityException e) {
            return false;
        }
    }

    byte[] getAppCertificate() throws GeneralSecurityException {
        requireInitialized();
        return LingoCredentials.APP_CERTIFICATE.clone();
    }

    void setPatchCertificate(byte[] certificate) throws GeneralSecurityException {
        requireInitialized();
        patchPublic = null;
        ephemeral = null;
        clearAuthorization();
        if (certificate == null || certificate.length != 140)
            throw new GeneralSecurityException("Invalid Lingo sensor certificate length");
        Signature signature = Signature.getInstance("SHA256withECDSA");
        signature.initVerify(patchSigningKey);
        signature.update(certificate, 0, 76);
        if (!signature.verify(derSignature(certificate, 76)))
            throw new GeneralSecurityException("Invalid Lingo sensor certificate signature");
        patchPublic = publicKey(Arrays.copyOfRange(certificate, 11, 76));
    }

    byte[] generateEphemeralKeys() throws GeneralSecurityException {
        requireInitialized();
        if (patchPublic == null) throw new GeneralSecurityException("Missing sensor certificate");
        clearAuthorization();
        ephemeral = null;
        KeyPairGenerator generator = KeyPairGenerator.getInstance("EC");
        generator.initialize(new ECGenParameterSpec("secp256r1"));
        ephemeral = generator.generateKeyPair();
        ECPoint point = ((ECPublicKey) ephemeral.getPublic()).getW();
        byte[] result = new byte[65];
        result[0] = 4;
        writeCoordinate(point.getAffineX(), result, 1);
        writeCoordinate(point.getAffineY(), result, 33);
        return result;
    }

    void generateKAuth(byte[] peer) throws GeneralSecurityException {
        clearAuthorization();
        if (ephemeral == null || patchPublic == null)
            throw new GeneralSecurityException("Incomplete Lingo key agreement");
        byte[] ephemeralSecret = null, staticSecret = null, hash = null;
        try {
            ephemeralSecret = sharedSecret(ephemeral.getPrivate(), publicKey(peer));
            staticSecret = sharedSecret(appPrivate, patchPublic);
            MessageDigest digest = MessageDigest.getInstance("SHA-256");
            digest.update(new byte[] {0, 0, 0, 1});
            digest.update(ephemeralSecret);
            digest.update(staticSecret);
            hash = digest.digest();
            authorizationKey = Arrays.copyOf(hash, 16);
        } finally {
            wipe(ephemeralSecret);
            wipe(staticSecret);
            wipe(hash);
            ephemeral = null;
        }
    }

    byte[] encrypt(byte[] nonce, byte[] plain) throws GeneralSecurityException {
        requireAuthorization();
        checkNonce(nonce);
        if (plain == null || plain.length > 65535)
            throw new GeneralSecurityException("Invalid Lingo challenge length");
        Cipher aes = aesCipher();
        byte[] tag = authenticationTag(aes, nonce, plain);
        byte[] output = Arrays.copyOf(plain, plain.length + 4);
        counterXor(aes, nonce, output, plain.length);
        byte[] mask = aes.doFinal(counterBlock(nonce, 0));
        for (int i = 0; i < 4; ++i) output[plain.length + i] = (byte) (tag[i] ^ mask[i]);
        wipe(tag);
        wipe(mask);
        return output;
    }

    byte[] decrypt(byte[] nonce, byte[] ciphertext) throws GeneralSecurityException {
        requireAuthorization();
        checkNonce(nonce);
        if (ciphertext == null || ciphertext.length < 4 || ciphertext.length > 65539)
            throw new GeneralSecurityException("Invalid Lingo response length");
        Cipher aes = aesCipher();
        int size = ciphertext.length - 4;
        byte[] plain = Arrays.copyOf(ciphertext, size);
        counterXor(aes, nonce, plain, size);
        byte[] tag = authenticationTag(aes, nonce, plain);
        byte[] mask = aes.doFinal(counterBlock(nonce, 0));
        int difference = 0;
        for (int i = 0; i < 4; ++i)
            difference |= (tag[i] ^ mask[i] ^ ciphertext[size + i]) & 255;
        wipe(tag);
        wipe(mask);
        if (difference != 0) {
            wipe(plain);
            throw new AEADBadTagException("Lingo response authentication failed");
        }
        return plain;
    }

    byte[] exportAuthorizationKey() throws GeneralSecurityException {
        requireAuthorization();
        // Preserve the native sensor settings' fixed 149-byte storage contract.
        byte[] result = new byte[SAVED_SIZE];
        System.arraycopy(SAVED_MAGIC, 0, result, 0, SAVED_MAGIC.length);
        result[8] = (byte) securityVersion;
        System.arraycopy(authorizationKey, 0, result, 12, 16);
        MessageDigest digest = MessageDigest.getInstance("SHA-256");
        digest.update(result, 0, CHECKSUM_OFFSET);
        System.arraycopy(digest.digest(), 0, result, CHECKSUM_OFFSET, 32);
        return result;
    }

    private PublicKey publicKey(byte[] encoded) throws GeneralSecurityException {
        if (encoded == null || encoded.length != 65 || encoded[0] != 4)
            throw new GeneralSecurityException("Invalid uncompressed P-256 point");
        BigInteger x = new BigInteger(1, Arrays.copyOfRange(encoded, 1, 33));
        BigInteger y = new BigInteger(1, Arrays.copyOfRange(encoded, 33, 65));
        BigInteger p = ((ECFieldFp) curve.getCurve().getField()).getP();
        if (x.compareTo(p) >= 0 || y.compareTo(p) >= 0 ||
                !y.multiply(y).mod(p).equals(x.multiply(x).multiply(x)
                        .add(curve.getCurve().getA().multiply(x))
                        .add(curve.getCurve().getB()).mod(p)))
            throw new GeneralSecurityException("Point is not on P-256");
        return keyFactory.generatePublic(new ECPublicKeySpec(new ECPoint(x, y), curve));
    }

    private static byte[] sharedSecret(PrivateKey own, PublicKey peer) throws GeneralSecurityException {
        KeyAgreement agreement = KeyAgreement.getInstance("ECDH");
        agreement.init(own);
        agreement.doPhase(peer, true);
        byte[] result = agreement.generateSecret();
        if (result.length != 32) {
            wipe(result);
            throw new GeneralSecurityException("Unexpected P-256 secret length");
        }
        return result;
    }

    private static void writeCoordinate(BigInteger value, byte[] output, int offset) {
        byte[] encoded = value.toByteArray();
        int size = Math.min(encoded.length, 32);
        System.arraycopy(encoded, encoded.length - size, output, offset + 32 - size, size);
    }

    private static byte[] derSignature(byte[] certificate, int offset) {
        byte[] r = new BigInteger(1, Arrays.copyOfRange(certificate, offset, offset + 32)).toByteArray();
        byte[] s = new BigInteger(1, Arrays.copyOfRange(certificate, offset + 32, offset + 64)).toByteArray();
        byte[] result = new byte[6 + r.length + s.length];
        result[0] = 0x30;
        result[1] = (byte) (result.length - 2);
        result[2] = 2;
        result[3] = (byte) r.length;
        System.arraycopy(r, 0, result, 4, r.length);
        result[4 + r.length] = 2;
        result[5 + r.length] = (byte) s.length;
        System.arraycopy(s, 0, result, 6 + r.length, s.length);
        return result;
    }

    private Cipher aesCipher() throws GeneralSecurityException {
        Cipher cipher = Cipher.getInstance("AES/ECB/NoPadding");
        cipher.init(Cipher.ENCRYPT_MODE, new SecretKeySpec(authorizationKey, "AES"));
        return cipher;
    }

    private static byte[] authenticationTag(Cipher aes, byte[] nonce, byte[] plain)
            throws GeneralSecurityException {
        byte[] block = counterBlock(nonce, plain.length);
        block[0] = 0x0f; // CCM: no AAD, four-byte tag, eight-byte length.
        byte[] mac = aes.doFinal(block);
        for (int offset = 0; offset < plain.length; offset += 16) {
            int size = Math.min(16, plain.length - offset);
            for (int i = 0; i < size; ++i) mac[i] ^= plain[offset + i];
            byte[] next = aes.doFinal(mac);
            wipe(mac);
            mac = next;
        }
        return mac;
    }

    private static void counterXor(Cipher aes, byte[] nonce, byte[] buffer, int length)
            throws GeneralSecurityException {
        for (int offset = 0; offset < length; offset += 16) {
            byte[] stream = aes.doFinal(counterBlock(nonce, offset / 16 + 1));
            for (int i = 0; i < Math.min(16, length - offset); ++i)
                buffer[offset + i] ^= stream[i];
            wipe(stream);
        }
    }

    private static byte[] counterBlock(byte[] nonce, long value) {
        byte[] block = new byte[16];
        block[0] = 7;
        System.arraycopy(nonce, 0, block, 1, 7);
        for (int i = 15; i >= 8; --i) {
            block[i] = (byte) value;
            value >>>= 8;
        }
        return block;
    }

    private static void checkNonce(byte[] nonce) throws GeneralSecurityException {
        if (nonce == null || nonce.length != 7)
            throw new GeneralSecurityException("Lingo nonce must contain seven bytes");
    }

    private void requireInitialized() throws GeneralSecurityException {
        if (securityVersion < 0) throw new GeneralSecurityException("Lingo authentication not initialized");
    }

    private void requireAuthorization() throws GeneralSecurityException {
        requireInitialized();
        if (authorizationKey == null) throw new GeneralSecurityException("Missing Lingo authorization key");
    }

    private void clearAuthorization() {
        wipe(authorizationKey);
        authorizationKey = null;
    }

    private static void wipe(byte[] bytes) {
        if (bytes != null) Arrays.fill(bytes, (byte) 0);
    }
}
