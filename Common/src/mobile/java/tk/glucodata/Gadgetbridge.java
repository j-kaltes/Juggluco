package tk.glucodata;

import android.content.Intent;

import java.util.Calendar;

import nodomain.freeyourgadget.gadgetbridge.model.WeatherSpec;

public class Gadgetbridge {
    private static final String LOG_ID="Gadgetbridge";

    public final static String WEATHER_EXTRA="WeatherSpec";
    public final static String WEATHER_ACTION="de.kaffeemitkoffein.broadcast.WEATHERDATA";
/*
private static float kelvin(float input) {
	return ((input-32f)*5f)/9f+273.15f;
	} */
private static float kelvin(float input) {
	return input+273.15f;
	}
static final private float[] speeds= {2, 6, 12, 20, 29, 39, 50, 62, 75, 89, 103, 118,150};
   static void sendglucose(String glstr,int mgdl,float gl,float rate,long timmsec)  {
    	 WeatherSpec weatherSpec = new WeatherSpec();
	 final int code=librecode(rate);
            weatherSpec.location             = librearrows[code]+glstr;
	    weatherSpec.timestamp            = (int) (timmsec/1000L);
	   weatherSpec.currentCondition=weatherSpec.location;
	weatherSpec.currentConditionCode=libreweather[code];
	   var cal = Calendar.getInstance();
          cal.setTimeInMillis(timmsec);
	weatherSpec.currentHumidity= cal.get(Calendar.MINUTE);
	   weatherSpec.windSpeed=speeds[cal.get(Calendar.HOUR)];
//	weatherSpec.windDirection=cal.get(Calendar.MINUTE);
	    
		
   	weatherSpec.currentTemp=(int)kelvin(Math.round(rate*10));
	if(Applic.unit==1) {
		float trunc=(float)Math.floor(gl);
		if((gl-trunc)>=.95f) {
			weatherSpec.todayMaxTemp= (int)kelvin((float)Math.ceil(gl));
			weatherSpec.todayMinTemp=273;
			}
		else   {
		    weatherSpec.todayMaxTemp= (int)kelvin(trunc);
		       weatherSpec.todayMinTemp=(int)kelvin(Math.round((gl*10)%10));
		       }
		 }
	   else  {
		weatherSpec.todayMaxTemp= (int)kelvin((float)Math.floor(gl/10.0f));
		weatherSpec.todayMinTemp=(int)kelvin(gl%10);
	   	}

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
//private static final int[] libreweather=  {603,			603,		603,	603,	    603,	603};
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


