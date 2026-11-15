package tk.glucodata;

public class Libre3GattCallback extends SuperGattCallback {
    public static byte[] getSavedGarminProvisioningSecret(String serial) { return null; }
    public static boolean saveGarminLibre3Minute(String serial,byte[] data,long time) { return false; }
    public static boolean saveGarminLibre3Clinical(String serial,byte[] data) { return false; }
    public Libre3GattCallback(String name, long dataptr) {
        super(name,dataptr,0);
    }
}

