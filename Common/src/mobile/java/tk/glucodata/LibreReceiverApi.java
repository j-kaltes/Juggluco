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

import android.content.Context;
import android.os.Build;
import android.util.Base64;
import androidx.annotation.Keep;
import dalvik.system.BaseDexClassLoader;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.ByteArrayOutputStream;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.nio.charset.CharacterCodingException;
import java.nio.charset.CodingErrorAction;
import java.nio.charset.StandardCharsets;
import java.security.GeneralSecurityException;
import java.security.KeyFactory;
import java.security.MessageDigest;
import java.security.PublicKey;
import java.security.SecureRandom;
import java.security.Signature;
import java.security.spec.MGF1ParameterSpec;
import java.security.spec.PSSParameterSpec;
import java.security.spec.X509EncodedKeySpec;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;
import javax.net.ssl.HttpsURLConnection;
import java.net.URL;

/** The application-key migration protocol tested with Libre NL 1.4.0 (2058).
 * Not the normal-login/device-key protocol. A signed API-region response can move
 * validation/conversion to the LibreView region belonging to the account.
 */
final class LibreReceiverApi {
    private static final String LOG_ID = "Libreview";
    static final String BASE = "https://libreapi-c-nl.libreview.io";
    static final String VERSION = "1.4.0.2058";
    private static final int MAX_BODY = 1024 * 1024;
    private static final PSSParameterSpec PSS = new PSSParameterSpec(
            "SHA-256", "MGF1", MGF1ParameterSpec.SHA256, 32, 1);

    interface Crypto {
        byte[] sign(byte[] bytes) throws IOException;
        byte[] encrypt(byte[] plain) throws IOException;
        byte[] decrypt(byte[] record) throws IOException;
    }
    interface Transport {
        default void setBase(String base) {}
        Response post(String endpoint, Map<String,String> headers, byte[] body)
                throws IOException;
    }
    static final class Response {
        final int status;
        final byte[] body;
        Response(int status, byte[] body) { this.status=status; this.body=body; }
    }
    private final Crypto crypto;
    private final PublicKey serverKey;
    private final Transport transport;
    private final String release;
    private String apiBase=BASE;

    LibreReceiverApi(Context context) throws IOException, GeneralSecurityException {
        this(NativeCrypto.open(context), publicKey(), new HttpTransport(), Build.VERSION.RELEASE);
    }
    // Package-private dependency injection for offline tests. Production uses the constructor above.
    LibreReceiverApi(Crypto crypto, PublicKey serverKey, Transport transport, String release) {
        this.crypto=crypto; this.serverKey=serverKey; this.transport=transport; this.release=release;
    }

    private static void log(String text) {
        if (Log.doLog) Log.i(LOG_ID,text);
    }
    private static String headerText(Map<String,? extends List<String>> headers) {
        StringBuilder out=new StringBuilder();
        for (Map.Entry<String,? extends List<String>> h:headers.entrySet()) {
            if (out.length()!=0) out.append("\n");
            if (h.getKey()==null) out.append(h.getValue());
            else out.append(h.getKey()).append(": ").append(h.getValue());
        }
        return out.toString();
    }

    private static String normalizeApiBase(String apiUrl) throws IOException {
        try {
            URL url=new URL(apiUrl);
            String path=url.getPath();
            if (!"https".equalsIgnoreCase(url.getProtocol()) || url.getUserInfo()!=null ||
                    url.getPort()!=-1 || url.getQuery()!=null || url.getRef()!=null ||
                    url.getHost()==null || url.getHost().isEmpty() ||
                    !(path==null || path.isEmpty() || path.equals("/")))
                throw new IOException("Invalid Libre receiver API region URL: "+apiUrl);
            return "https://"+url.getHost();
        } catch (java.net.MalformedURLException ex) {
            throw new IOException("Invalid Libre receiver API region URL: "+apiUrl,ex);
        }
    }

    String validate(String legacyToken, String expectedEmail) throws Exception {
        requireToken(legacyToken);
        log("Libre receiver validate legacyToken="+legacyToken+" expectedEmail="+expectedEmail);
        JSONObject body = new JSONObject().put("tokens", new JSONObject().put("fsl3",legacyToken));
        for (int regionChanges=0;;++regionChanges) {
            JSONObject result = post("validate-legacy-product-tokens",body,null);
            log("Libre receiver validate result="+result.toString());
            final JSONObject fsl3;
            try { fsl3=result.getJSONObject("tokens").getJSONObject("fsl3"); }
            catch (JSONException ex) { throw new IOException("Validation returned no usable fsl3 token"); }

            JSONObject error=fsl3.optJSONObject("error");
            if (error!=null && "UnsupportedCountryInAPIRegionError".equals(error.optString("error"))) {
                final String nextBase;
                try { nextBase=normalizeApiBase(text(error,"apiUrl")); }
                catch (JSONException ex) { throw new IOException("Validation region error has no usable apiUrl"); }
                String country=error.optString("unsupportedCountry","");
                if (nextBase.equals(apiBase) || regionChanges>=2)
                    throw new IOException("Libre receiver API region redirect did not resolve"+
                            (country.isEmpty()?"":" country "+country));
                log("Libre receiver API region redirect country="+country+
                        " from="+apiBase+" to="+nextBase);
                apiBase=nextBase;
                transport.setBase(apiBase);
                continue;
            }

            final String email, token;
            try {
                email = text(fsl3,"email");
                token = text(fsl3,"token");
            } catch (JSONException ex) { throw new IOException("Validation returned no usable fsl3 token"); }
            // A token left over from a previously configured account must not be converted.
            if (!email.trim().equalsIgnoreCase(expectedEmail.trim()))
                throw new IOException("Validated token belongs to another email; authenticate the configured account first");
            requireToken(token);
            log("Libre receiver validate email="+email+" validatedToken="+token);
            return token;
        }
    }

    String convert(String validatedToken, String applicationId) throws Exception {
        requireToken(validatedToken);
        log("Libre receiver convert validatedToken="+validatedToken+" applicationId="+applicationId);
        if (applicationId == null || !applicationId.matches(
                "[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}"))
            throw new IOException("The Libre 3 device ID does not have the expected UUID text format");
        JSONObject body = new JSONObject().put("product","fsl3").put("token",validatedToken);
        JSONObject result = post("convert-legacy-product-token",body,applicationId);
        log("Libre receiver convert result="+result.toString());
        try {
            String receiver = text(result,"receiverID");
            long receiverNumber=receiverNumber(receiver); // Validate before allowing anything to be saved.
            log("Libre receiver receiverID="+receiver+" numeric="+receiverNumber);
            return receiver;
        } catch (JSONException ex) { throw new IOException("Conversion returned no receiverID; configs fallback is not implemented"); }
        // Do not copy access_token into Juggluco's legacy UserToken.
    }

    JSONObject post(String endpoint, JSONObject body, String applicationId) throws Exception {
        if (!endpoint.equals("validate-legacy-product-tokens") && !endpoint.equals("convert-legacy-product-token"))
            throw new IOException("Unsupported receiver-ID endpoint");
        String stamp = Long.toString(System.currentTimeMillis()/1000L);
        String plainText=body.toString();
        log("Libre receiver request endpoint="+endpoint+" plaintext="+plainText);
        byte[] plain = plainText.getBytes(StandardCharsets.UTF_8);
        byte[] prefix = (stamp+VERSION).getBytes(StandardCharsets.UTF_8);
        byte[] signed = concat(prefix,plain);
        byte[] signature, record;
        try { signature=crypto.sign(signed); record=crypto.encrypt(plain); }
        finally { Arrays.fill(signed,(byte)0); Arrays.fill(plain,(byte)0); }
        if (signature.length!=256) throw new IOException("Unexpected request RSA signature length");
        if (record.length<28) throw new IOException("Invalid native GCM record");
        JSONObject envelope = new JSONObject().put("data",frame(record)).put("signature",b64(signature));
        Map<String,String> headers = new LinkedHashMap<>();
        headers.put("Accept","application/json");
        headers.put("Accept-Encoding","identity");
        headers.put("Content-Type","application/json");
        headers.put("User-Agent","okhttp/4.9.2");
        headers.put("X-User-Agent","libre1;"+VERSION+";Android;"+release);
        headers.put("X-LAPI-ID",stamp); // No @device suffix on either migration endpoint.
        headers.put("X-LAPI-V",VERSION);
        headers.put("X-LAPI-SV","1");
        if (applicationId!=null) headers.put("X-Device-ID",applicationId);
        byte[] wireBody=envelope.toString().getBytes(StandardCharsets.UTF_8);
        transport.setBase(apiBase);
        log("Libre receiver request URL="+apiBase+"/v1/"+endpoint);
        for (Map.Entry<String,String> h:headers.entrySet())
            log("Libre receiver request header "+h.getKey()+": "+h.getValue());
        log("Libre receiver request header Content-Length: "+wireBody.length);
        log("Libre receiver request body len="+wireBody.length+" "+new String(wireBody,StandardCharsets.UTF_8));
        Response response = transport.post(endpoint,headers,wireBody);
        if (response.body.length>MAX_BODY) throw new IOException("Response too large");
        log("Libre receiver response status="+response.status);
        log("Libre receiver response body len="+response.body.length+" "+new String(response.body,StandardCharsets.UTF_8));
        JSONObject outer;
        try { outer=new JSONObject(utf8(response.body)); }
        catch (JSONException | CharacterCodingException ex) {
            throw new IOException("HTTP "+response.status+": non-JSON response (possibly a gateway block); no API result");
        }
        byte[] encrypted, serverSignature;
        try { encrypted=unframe(text(outer,"data")); serverSignature=unb64(text(outer,"signature")); }
        catch (JSONException ex) { throw new IOException("HTTP "+response.status+": response has no signed envelope"); }
        byte[] decoded=crypto.decrypt(encrypted);
        try {
            log("Libre receiver decrypted response len="+decoded.length+" "+new String(decoded,StandardCharsets.UTF_8));
            verify(serverKey,prefix,decoded,serverSignature);
            log("Libre receiver server RSA-PSS signature verified");
            final JSONObject result;
            try { result=new JSONObject(utf8(decoded)); }
            catch (JSONException | CharacterCodingException ex) { throw new IOException("Verified response is not UTF-8 JSON"); }
            if (response.status<200 || response.status>=300) {
                // The known API errors use these symbolic details. Never include raw JSON/tokens.
                String detail=result.optString("details","");
                if (!detail.matches("[A-Za-z0-9_.-]{1,120}")) detail="";
                throw new IOException("HTTP "+response.status+": verified API rejection"+
                        (detail.isEmpty()?"":" ("+detail+")"));
            }
            return result;
        } finally { Arrays.fill(decoded,(byte)0); }
    }

    static void selftest(Context context) throws Exception {
        Crypto crypto=NativeCrypto.open(context);
        byte[] plain=new byte[256];
        for(int i=0;i<plain.length;++i) plain[i]=(byte)i;
        byte[] record=crypto.encrypt(plain);
        if(!Arrays.equals(plain,crypto.decrypt(record))) throw new IOException("Native GCM round trip failed");
        record[record.length-1]^=1;
        boolean rejected=false;
        try { crypto.decrypt(record); } catch(IOException expected) { rejected=true; }
        if(!rejected) throw new IOException("Native GCM accepted a corrupted tag");
        if(crypto.sign(plain).length!=256) throw new IOException("Native RSA signature length is wrong");
        publicKey(); // Verify that the packaged public key parses locally as well.
        if (Build.VERSION.SDK_INT<23) LibreReceiverPss.selftest();
    }

    static long receiverNumber(String receiver) throws IOException {
        if (receiver==null) throw new IOException("Missing receiverID");
        // No UUID parsing, hyphen removal, lowercasing or trimming.
        byte[] bytes;
        try {
            ByteBuffer encoded=StandardCharsets.UTF_8.newEncoder()
                    .onMalformedInput(CodingErrorAction.REPORT).onUnmappableCharacter(CodingErrorAction.REPORT)
                    .encode(java.nio.CharBuffer.wrap(receiver));
            bytes=new byte[encoded.remaining()]; encoded.get(bytes);
        } catch (CharacterCodingException ex) { throw new IOException("Invalid receiverID text"); }
        if (bytes.length==0 || bytes.length>4096 || (bytes.length&3)!=0)
            throw new IOException("receiverID byte length is not a nonzero multiple of four");
        long sum=0;
        for (int i=0;i<bytes.length;i+=4) {
            long word=((bytes[i]&255L)<<24)|((bytes[i+1]&255L)<<16)|
                    ((bytes[i+2]&255L)<<8)|(bytes[i+3]&255L);
            sum=(sum+word)&0xffffffffL;
        }
        return sum;
    }
    static void verify(PublicKey key, byte[] prefix, byte[] plain, byte[] signature)
            throws GeneralSecurityException {
        if (Build.VERSION.SDK_INT<23) {
            LibreReceiverPss.verify(key,prefix,plain,signature);
            return;
        }
        Signature verifier;
        try { verifier=Signature.getInstance("SHA256withRSA/PSS"); }
        catch (java.security.NoSuchAlgorithmException ex) { verifier=Signature.getInstance("RSASSA-PSS"); }
        verifier.initVerify(key);
        verifier.setParameter(PSS);
        verifier.update(prefix); verifier.update(plain);
        if (!verifier.verify(signature)) throw new GeneralSecurityException("Server RSA-PSS signature rejected");
    }
    static PublicKey publicKey() throws GeneralSecurityException,IOException {
        return KeyFactory.getInstance("RSA").generatePublic(new X509EncodedKeySpec(unb64(SERVER_PUBLIC_DER)));
    }
    private static String text(JSONObject object,String key) throws JSONException {
        Object value=object.get(key);
        if (!(value instanceof String) || ((String)value).isEmpty()) throw new JSONException("Missing string field");
        return (String)value;
    }
    static void requireToken(String token) throws IOException {
        if (token==null || token.isEmpty() || token.length()>65536) throw new IOException("Missing or oversized legacy token");
        for (int i=0;i<token.length();++i) if (token.charAt(i)<=32 || token.charAt(i)>126)
            throw new IOException("Invalid token characters");
    }
    static String utf8(byte[] b) throws CharacterCodingException {
        return StandardCharsets.UTF_8.newDecoder().onMalformedInput(CodingErrorAction.REPORT)
                .onUnmappableCharacter(CodingErrorAction.REPORT).decode(ByteBuffer.wrap(b)).toString();
    }
    static byte[] concat(byte[] a,byte[] b) {
        byte[] r=Arrays.copyOf(a,a.length+b.length); System.arraycopy(b,0,r,a.length,b.length); return r;
    }
    static String b64(byte[] b) { return Base64.encodeToString(b,Base64.NO_WRAP); }
    static byte[] unb64(String s) throws IOException {
        if (s.length()>MAX_BODY*2 || (s.length()&3)!=0 || !s.matches("[A-Za-z0-9+/]*={0,2}"))
            throw new IOException("Invalid base64 field");
        try {
            byte[] b=Base64.decode(s,Base64.NO_WRAP);
            if (!b64(b).equals(s)) throw new IOException("Noncanonical base64 field");
            return b;
        } catch (IllegalArgumentException ex) { throw new IOException("Invalid base64 field"); }
    }
    static String frame(byte[] record) {
        return b64(Arrays.copyOfRange(record,0,12))+"."+
                b64(Arrays.copyOfRange(record,12,record.length-16))+"."+
                b64(Arrays.copyOfRange(record,record.length-16,record.length));
    }
    static byte[] unframe(String s) throws IOException {
        String[] parts=s.split("\\.",-1);
        if (parts.length!=3) throw new IOException("Invalid GCM envelope");
        byte[] iv=unb64(parts[0]),ct=unb64(parts[1]),tag=unb64(parts[2]);
        if (iv.length!=12 || tag.length!=16) throw new IOException("Invalid GCM IV or tag length");
        return concat(concat(iv,ct),tag);
    }
    static byte[] readBounded(InputStream in,int limit) throws IOException {
        ByteArrayOutputStream out=new ByteArrayOutputStream(); byte[] buf=new byte[8192];
        int n;
        while ((n=in.read(buf))!=-1) {
            if (out.size()>limit-n) throw new IOException("Response or file exceeds limit");
            out.write(buf,0,n);
        }
        return out.toByteArray();
    }
    private static final class HttpTransport implements Transport {
        private String base=BASE;
        @Override public void setBase(String base) { this.base=base; }
        public Response post(String endpoint,Map<String,String> headers,byte[] body) throws IOException {
            HttpsURLConnection c=(HttpsURLConnection)new URL(base+"/v1/"+endpoint).openConnection();
            try {
                c.setInstanceFollowRedirects(false);
                c.setConnectTimeout(15000); c.setReadTimeout(30000);
                c.setUseCaches(false); c.setRequestMethod("POST"); c.setDoOutput(true);
                for (Map.Entry<String,String> h:headers.entrySet()) c.setRequestProperty(h.getKey(),h.getValue());
                c.setFixedLengthStreamingMode(body.length);
                if (Log.doLog) {
                    Map<String,List<String>> actualHeaders=c.getRequestProperties();
                    Log.i(LOG_ID,"Libre receiver HttpsURLConnection request headers:\n"+headerText(actualHeaders));
                }
                try (OutputStream out=c.getOutputStream()) { out.write(body); }
                int status=c.getResponseCode();
                if (Log.doLog)
                    Log.i(LOG_ID,"Libre receiver response headers:\n"+headerText(c.getHeaderFields()));
                InputStream response=status>=400?c.getErrorStream():c.getInputStream();
                if (response==null) return new Response(status,new byte[0]);
                try (InputStream in=response) { return new Response(status,readBounded(in,MAX_BODY)); }
            } finally { c.disconnect(); }
        }
    }

    /** No System.loadLibrary on Abbott's library: that would call its JNI_OnLoad. */
    @Keep
    static final class NativeCrypto implements Crypto {
        private static NativeCrypto instance;
        private static final SecureRandom random=new SecureRandom();
        static synchronized NativeCrypto open(Context unused) throws IOException,GeneralSecurityException {
            if (instance!=null) return instance;
            ClassLoader loader=LibreReceiverApi.class.getClassLoader();
            if (!(loader instanceof BaseDexClassLoader)) throw new IOException("No Android native-library loader");
            String path=((BaseDexClassLoader)loader).findLibrary("libre_receiver_skb");
            if (path==null) throw new IOException("Missing packaged liblibre_receiver_skb.so for this process ABI");
           // pinLibrary(path);
            Natives.libreReceiverInit(path,unb64(PRIVATE_SKB),unb64(WRAP_SKB));
            instance=new NativeCrypto();
            return instance;
        }
        public synchronized byte[] sign(byte[] b) throws IOException { return Natives.libreReceiverCrypt(0,b,null); }
        public synchronized byte[] encrypt(byte[] b) throws IOException {
            byte[] iv=new byte[12]; random.nextBytes(iv); return Natives.libreReceiverCrypt(1,b,iv);
        }
        public synchronized byte[] decrypt(byte[] b) throws IOException { return Natives.libreReceiverCrypt(2,b,null); }
      }
/*
        private static void pinLibrary(String path) throws IOException,GeneralSecurityException {
            // findLibrary can refer to an extracted file OR an entry loaded directly from an APK.
            int zipAt=path.indexOf("!/");
            MessageDigest digest=MessageDigest.getInstance("SHA-256");
            if (zipAt>=0) {
                try (ZipFile zip=new ZipFile(path.substring(0,zipAt))) {
                    ZipEntry entry=zip.getEntry(path.substring(zipAt+2));
                    if (entry==null) throw new IOException("Missing SKB library in APK");
                    try (InputStream in=zip.getInputStream(entry)) { hashStream(digest,in); }
                }
            } else try (InputStream in=new FileInputStream(path)) { hashStream(digest,in); }
            StringBuilder actual=new StringBuilder();
            for(byte b:digest.digest()) actual.append(String.format(java.util.Locale.ROOT,"%02x",b&255));
            // The class loader selects the process ABI; both exact files are pinned.
            // Process.is64Bit() is unavailable on Android 5.x.
            String fingerprint=actual.toString();
            if (!SKB_SHA256.equals(fingerprint) && !SKB_SHA256_ARM32.equals(fingerprint))
                throw new IOException("SKB library differs from tested version; do not strip it");
        }
        private static void hashStream(MessageDigest digest,InputStream in) throws IOException {
            byte[] b=new byte[32768]; int n; long total=0;
            while((n=in.read(b))!=-1) { total+=n; if(total>40L*1024*1024) throw new IOException("SKB library too large"); digest.update(b,0,n); }
            }
    // Fixed APPLICATION-key containers from the supplied NL APK, not user credentials.
    // They are SKB containers, not plaintext PKCS#8 or AES keys.
          private static final String SKB_SHA256= "214c4d01609cb6c8bf2ca5f9f894142d356d5a8177e5faa9246237c2c3a10c05";
    private static final String SKB_SHA256_ARM32= "b8b2fad33c68e46762a11b6e08fa0e2e38a3f60d5acdae165af0097109877492";
    */
    private static final String PRIVATE_SKB=
            "gyw6TgIAAAACAAABAAAAAADiy/6rDM4EgCUkdZ1caQ4LjU+xMcHNvK4hcIhzudEuhU9LsL4HuJJx68PLgzQFX5YAAAACn9qV" +
            "/f/khPy4lT0zdRQuu+TldiuC3GMEAAADAEW0e1UMxEEDM4NL6XFhaDt0oVmukuF1yY64R4TT/ppt1FZidVczgiZXrT1iEn4I" +
            "ie3T2pgvWVXUm7ZaKr5HVhTPHzyLryezCuv08+H71qtBlXF+q/4SP5eeoTFazbr2dv07U2jLfBnrOQWnNzx1HZNEmn2xlthK" +
            "D3sA10H6JPmyH4xRz78+P1AMpv//GhD0ORihZ9extL6beLk1Eee3ER9hATM/nLSg4pHE/oD7SOv143qZsUcM64VrlMLSsMBt" +
            "V+/Pd0jj9hhclGubRYNl6p18pW7s3qrWtaY0a0DIrWTdgiX0uAR3R5O3Yq2whiMbkhwiCdPd2+cf06Zgx/K1brqHGNoe2z4N" +
            "jbipSdWjeDUJUzHikSnRp9UKawF49GTscngjMLhJUkZu7G4m4IE+lgUIsmTBGNMDHO8ApOA2a3DWgieClnQ9B/GbLtMQ7bD8" +
            "CVixg/tBtKt0tcXeMSMgGoDoJDwb5bs0/KUIIgpaW+P7w3wRgwinvKTA8WVVOeTh5x/0yu4A6nfa3ltHNeI5REouzBZOSC0S" +
            "1tlX2ta/I6ld7qrCUo2RHIyh7uAohSVcES0iutbg8LPv9xkIJmvuibHi/tiI+H0zXpkbTrtqvBR100ck31twjKwMpwGZ8CW9" +
            "IddAKc34sJrz9qIVdEbzTULzzvfAsr8s7STajb9xXyFJFa2Llrc16SCUJw+i1mY8fiYy4WmIowlawIAwnVpKja3HBwauR9Nx" +
            "vQMik3JgeiVjmcPjxbuRTApBNslFeRMeVLKa4FZurf3SzCctA+YUyNKXpeYcnyf6zqibe5g3e9ZXVW/gduafprybEVNne7+F" +
            "SW7rWG2J71FKXYlTztbeBGg0WJ/SkgjRxO//H1wIQIyOTGCwqFkDutaLFd6EoZQC16dKuem2C0x62mD2atpbVT2FZAMn9qcl" +
            "u9GUtYsE9NpgS3sW650zK0M0dPuTPjjhYXz1CswSr/9w22cPvAQuw86TZXllX8Ql3MP4ucPlewYk68Tu3flu/iLYZB2eTTbe" +
            "MgAAAAAAAAAAAAAAAAAAAAA3W/pHVOfWjqCgskfbhtUqK/w0xyHeei8AAAEYAAAAAQAAAQBnul/5DnpDUo8KcdHWnOD78wYs" +
            "dC4V0lmcbuN7lFdUP5XRvUq7YHQeuoWDgsMy/oDMGRGh1xjIC1ytCjWsT8fCYaxufeDScpHSu51HSKcDnQQWkt4rw73HRjGo" +
            "klAGi2nmwBqoBulUA8DI3UsqKuLT4S3aUr5LNY6BMp9s2m2f/BiHwn09EGvyWeEw8EePVcbMtnEGDw6/zx9vdQ2ikKv4xZpZ" +
            "yyG4Dn2KJhx1WbnnQZ8aDPpFHraz5AiXq3cIyjIIsnOZpPg7SW3/PU/Us5kDCmmFzKokP8DJXZrQcHWX/ZZ/0n4bCaZfsoSa" +
            "3qjaeXdZiEKoSbW15tNiwN/0jwcmMiqinbaxM8FIVJW9aJUGqPU+o5/JvDCvpYMddm0W7tj26nHv";
    private static final String WRAP_SKB=
            "HG4gjQIAAAAAAAAAIAAAAADiy/6rDM4EgCUkdZ1caQ4LjU+xMcHNvK4hcIhzudEuhcZ0fKDflPaeHU0x1xANO1QAAAAB3M9z" +
            "RsbnRp0umRqkWQwRZp/2wwlhiEmTAAAAIEFdddZrRWyPNeMMV3BwJeGMEA13iGb9UjkoNWCS36zsAAAAAAAAAAAAAAAAAAAA" +
            "AN+OLon2XXCgXZW9tXhN6mqtl8R1";
    private static final String SERVER_PUBLIC_DER="MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAxhUMvw5dgTqoScMYHoH5PM/Zv+koHwpYAJbpL3oMAd32PKIpEI6L9zVUmA18LVN0eX4k4djwEPu0EzTAD0t0cEuDWWeJUES2AXIBG17299EMWorGglQSkTbnnfEv/6z0D6cUKhRsZDOLGfMHqTlOepO2Nzir/ENsh+qw5E1RHfWZUqhBFJ63VctsUr7UiKNImySg7WQ/hoKCBJbQG1j/6snEUDvNHsw5QR1FIrEs762S/KH+K7Vy1s5zCVlYWQko6H4xxp5o67Fdx73FK6lkFAHmrcZII2jJXbbnznhjiQlGapQ4wekIOZdpxLCp61gUOPCkwlF/KZ6Tt+BA34jhcQIDAQAB";
}
