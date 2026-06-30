package tk.glucodata;

/** Stub for Wear OS build variant — snooze is only supported on phone. */
public class AlarmSnooze {
    public static void init()             {}
    public static boolean isActive()      { return false; }
    public static void set(long minutes)  {}
    public static long snoozeUntilMs()    { return 0L; }
    public static String snoozeUntilText(){ return ""; }
}
