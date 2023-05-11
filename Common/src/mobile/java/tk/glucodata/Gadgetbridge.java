package tk.glucodata;

import android.content.Intent;

import nodomain.freeyourgadget.gadgetbridge.model.WeatherSpec;

public class Gadgetbridge {

    public final static String WEATHER_EXTRA="WeatherSpec";
    public final static String WEATHER_ACTION="de.kaffeemitkoffein.broadcast.WEATHERDATA";


   static void sendglucose(String glstr,int mgdl,float gl,float rate,long timmsec)  {
    	 WeatherSpec weatherSpec = new WeatherSpec();
	 final int code=librecode(rate);
            weatherSpec.location             = librearrows[code]+glstr;
	    weatherSpec.timestamp            = (int) (timmsec/1000L);
	   weatherSpec.currentCondition=weatherSpec.location;
	weatherSpec.currentConditionCode=libreweather[code];
	   weatherSpec.windSpeed=0;
    	weatherSpec.currentTemp=(int)Math.round(gl)+273;
	int trunc=(int)gl;
    	weatherSpec.todayMaxTemp= trunc+273;
       weatherSpec.todayMinTemp=((int)Math.round(gl*10))%10+273;

            Intent intent = new Intent();
            intent.putExtra(WEATHER_EXTRA,weatherSpec);
            intent.setPackage("nodomain.freeyourgadget.gadgetbridge");
            intent.setFlags(Intent.FLAG_INCLUDE_STOPPED_PACKAGES);
            intent.setAction(WEATHER_ACTION);
           Applic.app.sendBroadcast(intent);
        }

private static final int Undetermined=0;
private static final int Falling_Quickly=1;
private static final int Falling=2;
private static final int Steady =3;
private static final int Rising =4;
private static final int Rising_Quickly=5;

private static     int librecode(float rate) {
        if (rate <= -2.0f) {
            return Falling_Quickly;
        }
        if (rate <= -1.0f) {
            return Falling;
        }
        if (rate <= 1.0f) {
            return Steady;
        }
        if (rate <= 2.0f) {
            return Rising;
        }
        if (Float.isNaN(rate)) {
            return Undetermined;
        }
        return Rising_Quickly;
    }
//private static final String[] librenames={"Undetermined","Falling_Quickly","Falling","Steady","Rising","Rising_Quickly"};
private static final String[] librearrows={" ","\u2193", "\u2198","\u2192", "\u2197", "\u2191"};
private static final int[] libreweather=  {701,			602,		600,	800,	    500,	502};
//					   mist		heavy snow	snow	clear	rain heavy rain

/*
static     String librelabel(float rate) {
        if (rate <= -2.0f) {
            return  "\u2193";
        }
        if (rate <= -1.0f) {
            return "\u2198";
        }
        if (rate <= 1.0f) {
            return "\u2192";
        }
        if (rate <= 2.0f) {
            return "\u2197";
        }
        if (Float.isNaN(rate)) {
            return " ";
        }
        return "\u2191";
    }
*/

}


