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
        equal("2:21:07",ActivityFormat.quantity(0,8467,false,Locale.US));
        equal("40.0 km",ActivityFormat.quantity(1,40000,false,Locale.US));
        equal("24.9 mi",ActivityFormat.quantity(1,40000,true,Locale.US));
        equal("16.9 km/h",ActivityFormat.quantity(7,16.9/3.6,false,Locale.US));
        equal("10.5 mi/h",ActivityFormat.quantity(7,16.9/3.6,true,Locale.US));
        equal("5",ActivityFormat.quantity(8,5,false,Locale.US));
        equal("742.9 kcal",ActivityFormat.quantity(6,742.9,false,Locale.US));
        equal("74.0 m",ActivityFormat.quantity(9,74,false,Locale.US));
        equal("48.0 bpm",ActivityFormat.quantity(2,48,false,Locale.US));
        equal("110,2 bpm",ActivityFormat.quantity(3,110.2,false,Locale.GERMANY));
        equal("9.9 bpm",ActivityFormat.quantity(4,9.9,false,Locale.US));
        equal("2:21:07",ActivityFormat.quantity(5,8467,false,Locale.US));
        equal("150.0 W",ActivityFormat.quantity(10,150,false,Locale.US));
        equal("80.0 rpm",ActivityFormat.quantity(11,80,false,Locale.US));
        equal("com.garmin",ActivityFormat.sourceLabel("com.garmin ·  [1]",true));
        equal("com.strava",ActivityFormat.sourceLabel("com.strava · Google Pixel 6 Pro [2]",true));
        equal("com.samsung",ActivityFormat.sourceLabel("com.samsung · Galaxy Watch [1]",false));
        equal("com.samsung · Galaxy Watch",ActivityFormat.sourceLabel("com.samsung · Galaxy Watch [1]",true));
        equal("com.samsung",ActivityFormat.sourceLabel("com.samsung",true));
        equal("",ActivityFormat.deviceName("[1]"));
        if(!ActivityFormat.sameSource("com.garmin"," [1]","com.garmin ·  [1]"))throw new AssertionError("Repeated main source");
        if(ActivityFormat.sameSource("com.strava","Pixel [2]","com.garmin ·  [1]"))throw new AssertionError("Hidden cross-app source");
        if(ActivityFormat.sameSource("com.garmin","Watch A [1]","com.garmin · Watch B [1]"))throw new AssertionError("Conflated watches");
        System.out.println("Activity durations, trailing units, locales and source/device labels: passed");
    }
}
