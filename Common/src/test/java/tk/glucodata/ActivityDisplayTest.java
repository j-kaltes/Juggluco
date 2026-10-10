// SPDX-License-Identifier: GPL-3.0-or-later
package tk.glucodata;

import java.util.Locale;

/** Plain JVM regression checks; call main with ActivityFormat on the classpath. */
public final class ActivityDisplayTest {
    private static void equal(String expected,String actual) {
        if(!expected.equals(actual))throw new AssertionError(expected+" != "+actual);
    }
    public static void main(String[] args) {
        equal("1:58:54",ActivityFormat.duration(7134,Locale.US));
        equal("1:59:14",ActivityFormat.duration(7154,Locale.US));
        equal("0:00:00",ActivityFormat.duration(0,Locale.US));
        equal("0:01:00",ActivityFormat.duration(59.9,Locale.US));
        equal("23:59:59",ActivityFormat.duration(86399,Locale.US));
        equal("1:00:00:00",ActivityFormat.duration(86400,Locale.US));
        equal("2:01:01:01",ActivityFormat.duration(176461,Locale.US));
        equal("29.6",ActivityFormat.fieldValue(1,29562.3,false,Locale.US));
        equal("29,6",ActivityFormat.fieldValue(1,29562.3,false,Locale.GERMANY));
        equal("18.4",ActivityFormat.fieldValue(1,29562.3,true,Locale.US));
        equal("15.5",ActivityFormat.fieldValue(7,4.3,false,Locale.US));
        equal("9.6",ActivityFormat.fieldValue(7,4.3,true,Locale.US));
        equal("1:58:54",ActivityFormat.fieldValue(5,7134,false,Locale.US));
        equal("0:00:00",ActivityFormat.fieldValue(5,0,false,Locale.US));
        equal("99.2",ActivityFormat.fieldValue(3,99.2,false,Locale.US));
        equal("1234",ActivityFormat.fieldValue(8,1234,false,Locale.US));
        System.out.println("Activity durations, metric/imperial units and decimal formatting: passed");
    }
}
