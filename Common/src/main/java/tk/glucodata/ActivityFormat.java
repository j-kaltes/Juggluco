// SPDX-License-Identifier: GPL-3.0-or-later
package tk.glucodata;

import java.util.Locale;

/** Display conversions only; the native records retain their original SI values. */
final class ActivityFormat {
    private ActivityFormat() {}
    static String duration(double seconds,Locale locale) {
        long total=Math.max(0,Math.round(seconds));
        long days=total/86400,hours=(total/3600)%24,minutes=(total/60)%60;
        return days==0?String.format(locale,"%d:%02d:%02d",hours,minutes,total%60):
            String.format(locale,"%d:%02d:%02d:%02d",days,hours,minutes,total%60);
    }
    static String fieldValue(int field,double value,boolean miles,Locale locale) {
        switch(field) {
            case 0:case 5:return duration(value,locale);
            case 1:value/=miles?1609.344:1000;break;
            case 7:value*=miles?3600/1609.344:3.6;break;
            case 8:return String.format(locale,"%.0f",value);
        }
        return String.format(locale,"%.1f",value);
    }
}
