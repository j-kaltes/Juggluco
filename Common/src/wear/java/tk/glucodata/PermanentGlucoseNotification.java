package tk.glucodata;
/** Wear OS stub — persistent glucose notification is phone-only. */
public class PermanentGlucoseNotification {
    public static boolean isEnabled() { return false; }
    public static void setEnabled(boolean e) {}
    public static void init(android.content.Context c) {}
    public static void cancel() {}
    public static void update() {}
}
