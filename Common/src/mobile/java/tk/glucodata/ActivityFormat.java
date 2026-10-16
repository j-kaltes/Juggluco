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

// SPDX-License-Identifier: GPL-3.0-or-later
package tk.glucodata;

import java.util.Locale;
import java.util.Date;
import java.text.SimpleDateFormat;

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
            case 7:case 13:value*=miles?3600/1609.344:3.6;break;
            case 8:return String.format(locale,"%.0f",value);
        }
        return String.format(locale,"%.1f",value);
    }
    static String quantity(int field,double value,boolean miles,Locale locale) {
        String unit;
        switch(field) {
            case 1:unit=miles?"mi":"km";break;
            case 2:case 3:case 4:case 12:unit="bpm";break;
            case 6:unit="kcal";break;
            case 7:case 13:unit=miles?"mi/h":"km/h";break;
            case 9:unit="m";break;
            case 10:unit="W";break;
            case 11:unit="rpm";break;
            default:unit="";
        }
        String valueText=fieldValue(field,value,miles,locale);
        return unit.isEmpty()?valueText:valueText+" "+unit;
    }
    static String peakQuantity(int field,double value,long time,boolean miles,Locale locale) {
        String text=quantity(field,value,miles,locale);
        return time>0?text+" "+new SimpleDateFormat("HH:mm",locale).format(new Date(time)):text;
    }
    static float roundTo(String text) {
        float value=Float.parseFloat(text.trim().replace(',','.'));
        if(!Float.isFinite(value)||value<=0)throw new IllegalArgumentException("Invalid rounding increment");
        return value;
    }
    static boolean roundingChanged(String text,float original) {
        try{return roundTo(text)!=original;}catch(IllegalArgumentException e){return true;}
    }
    static String sourceOrigin(String source) {
        int separator=source.indexOf(" · ");
        return separator<0?source:source.substring(0,separator);
    }
    static String sourceDevice(String source) {
        int separator=source.indexOf(" · ");
        return separator<0?"":source.substring(separator+3);
    }
    static String deviceName(String device) {
        // The stored suffix is Health Connect's device type, not a watch ID.
        // Type 2 means phone. Keep all metadata internally for source matching.
        if(device.endsWith(" [2]"))return "";
        return device.replaceFirst("\\s*\\[\\d+\\]$","").trim();
    }
    static String sourceLabel(String source,boolean distinguishDevice) {
        String origin=sourceOrigin(source);
        String device=distinguishDevice?deviceName(sourceDevice(source)):"";
        return device.isEmpty()?origin:origin+" · "+device;
    }
    static boolean sameSource(String origin,String device,String heartSource) {
        if(!origin.equals(sourceOrigin(heartSource)))return false;
        String activityDevice=deviceName(device),heartDevice=deviceName(sourceDevice(heartSource));
        return activityDevice.isEmpty()||heartDevice.isEmpty()||activityDevice.equals(heartDevice);
    }
}
