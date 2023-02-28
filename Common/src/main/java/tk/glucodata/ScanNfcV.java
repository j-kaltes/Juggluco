/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2 and 3 sensors.                         */
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
/*      Fri Jan 27 15:31:05 CET 2023                                                 */


package tk.glucodata;


import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.media.AudioAttributes;
import android.nfc.Tag;
import android.os.VibrationAttributes;
import android.os.VibrationEffect;
import android.os.Vibrator;
import android.os.VibratorManager;
import android.provider.CalendarContract;
import android.util.TypedValue;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.TextView;

import java.util.Arrays;

import tk.glucodata.settings.Settings;

import static android.content.Context.VIBRATOR_SERVICE;
import static android.view.View.GONE;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static tk.glucodata.Applic.isRelease;
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.BuildConfig.libreVersion;
import static tk.glucodata.Gen2.getversion;
import static tk.glucodata.Libre3.libre3NFC;
import static tk.glucodata.Natives.showbytes;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getlabel;

public class ScanNfcV   {
    private static final String LOG_ID = "ScanNfcV";


//        if(android.os.Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.LOLLIPOP)
//           else vibrator.vibrate( vibrationPatternstart , 1, new AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_ALARM) .build());

//static int enablestreaming=0;
static byte[] newdevice=null;
	@SuppressWarnings("deprecation")
static void failure(Vibrator vibrator) {
	final long[] vibrationPatternFailure = {0, 500}; // [ms]
        if(android.os.Build.VERSION.SDK_INT < 26)
		vibrator.vibrate(vibrationPatternFailure, -1);
	else
		vibrator.vibrate(VibrationEffect.createWaveform(vibrationPatternFailure, -1));
	}



/*
static Ringtone getringtone(Context context, int res) {
	String	uristr= "android.resource://" + context.getPackageName() + "/" + res;
	Uri uri=Uri.parse(uristr);
        Ringtone ring = RingtoneManager.getRingtone(context, uri);
	try {
		if(Build.VERSION.SDK_INT >=23)
			ring.setLooping(true);
		    } catch(Exception e) {
		    	}
	return ring;
	}*/

static			boolean mayEnablestreaming(Tag tag,byte[] uid,byte[] info) {
	if(!Natives.streamingAllowed()) {
		Log.d(LOG_ID,"!Natives.streamingAllowed()");
		return false;
		}
	if(!AlgNfcV.enableStreaming(tag, info)) {
		Log.d(LOG_ID, "Enable streaming failed");
		return false;
		} 

	String sensorident = Natives.getserial(uid, info);
	Log.d(LOG_ID, "Streaming enabled, resetDevice " + sensorident);
	if(SensorBluetooth.resetDevice(sensorident))
		askpermission=true;
	return true;
	}


private static boolean righttag(byte[] id) {
         return (id.length == 8 && id[7] == -32 && id[6] == 7) ;
        }


static 	AudioAttributes audioattributes;
static {
        if(android.os.Build.VERSION.SDK_INT >= 26)
 		audioattributes=new AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_ALARM) .build();
	};
static	VibrationAttributes vibrationattributes=android.os.Build.VERSION.SDK_INT <33?null:new VibrationAttributes.Builder().setUsage(VibrationAttributes.USAGE_ALARM) .build();
static void vibrates(Vibrator vibrator,final long[] vibrationPatternstart ,final int[]  amplitude) {
	if( android.os.Build.VERSION.SDK_INT <33) {
		vibrator.vibrate(VibrationEffect.createWaveform(vibrationPatternstart,amplitude, 1),audioattributes);
		}
	else {
		vibrator.vibrate(VibrationEffect.createWaveform(vibrationPatternstart,amplitude, 1),vibrationattributes);
		}
	}
static	boolean askpermission=false;
	@SuppressWarnings("deprecation")
static public void scan(GlucoseCurve curve,Tag tag) {
	 askpermission=false;
        MainActivity main= (MainActivity)(curve.getContext());
	Vibrator vibrator = null;
	if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.S) {
		vibrator =  ((VibratorManager)(curve.getContext()).getSystemService(Context.VIBRATOR_MANAGER_SERVICE)).getDefaultVibrator();
	}
	else
		vibrator=(Vibrator) (curve.getContext()).getSystemService(VIBRATOR_SERVICE);
	curve.render.stepresult=GlucoseCurve.STEPBACK;
        if(android.os.Build.VERSION.SDK_INT < 26)
	        vibrator.vibrate( new long[]  {0, 100, 10,50,50} , 1);
	    else {
		final long[] vibrationPatternstart = {0, 70, 50,50,50,50,50};
		final int[] amplitude={0,  255,150,0,255,50,0};
		vibrates(vibrator,vibrationPatternstart,amplitude); 
		/*
		if( android.os.Build.VERSION.SDK_INT <33) {
	       		vibrator.vibrate(VibrationEffect.createWaveform(vibrationPatternstart,amplitude, 1),audioattributes);
			}
		else {
	       		vibrator.vibrate(VibrationEffect.createWaveform(vibrationPatternstart,amplitude, 1),vibrationattributes);
			}
	       */	
    		}
        {
	if(!Natives.gethaslibrary()) {
		vibrator.cancel();
		failure(vibrator);
		if(main.openfile!=null)
			Applic.RunOnUiThread(() -> {   main.openfile.showchoice(main,true); });
		return;
		}
            int value=0;
            int ret = 0x100000;
	    try {
		   byte[] uid=tag.getId();
		    if(!isRelease) {
				String sensid="";
				for(var e:uid) {
					sensid=String.format("%02X",(0xFF&e))+sensid;
				}
			Log.i(LOG_ID,"TAG::sensid="+sensid);
			}


		if(uid.length==8&&uid[6]!=7) {
			long streamptr;
			streamptr=libre3NFC(tag);
			vibrator.cancel();
			if(streamptr==2L) {
			 	Log.i(LOG_ID,"streamptr==2");
				ret= 0xFD;
				}
			else {
			   if(libreVersion == 3) {
			        if(streamptr>=0L&&streamptr<7L) {
					switch((int)(streamptr&0xFFFFFFF)) {
						case 1: {
							Log.i(LOG_ID,"streamptr==1");
							ret=0xFB;
							};break;
						case 5: {
							Log.i(LOG_ID,"terminated");
							ret=13;
							break;
							}
						case 6: {
							Log.i(LOG_ID,"ended");
							ret=4;
							break;
							}
						default: {
							 if(streamptr>=0L&&streamptr<4L) {
								Log.i(LOG_ID,"p<streamptr<4");
								  ret=0xFA;
								  }
							}
						}
					}
				 else {
					 var name = Natives.getSensorName(streamptr);
					 if(name==null){
						Log.i(LOG_ID,"name==null");
						ret=0xFA;
						}
					 else{
						Log.i(LOG_ID,"scanned "+name);
						if(SensorBluetooth.resetDevice(streamptr, name))
							askpermission = true;
						ret = 0xFC;
						value=1;
						curve.render.badscan =calendar(main, ret, name);
						}
					}
					}
				else {
			 		Log.i(LOG_ID,"libreVersion!=3");
					ret = 0xFE;
					}
			}
			if(ret!=0xFC)

				failure(vibrator);
		   	}
	        else {
			byte[] info = AlgNfcV.nfcinfo(tag);
			if(info==null) {
				ret=17;
				Log.i(LOG_ID,"Read Tag Info Error");
				vibrator.cancel();
				}
			else  {
				byte[] data;

				if((data = AlgNfcV.readNfcTag(tag,uid,info)) != null) {
					curve.render.badscan =0xff;
					curve.requestRender();
					Log.d(LOG_ID,"Read Tag");
					/*showbytes("uid",uid);
					showbytes("info",info);
					showbytes("data",data); */
					int uit = Natives.nfcdata(uid, info, data);
					value = uit & 0xFFFF;
					Log.format("glucose=%.1f\n",(float)value/18.0f);
					ret = uit >> 16;
					if(newdevice!=null&& Arrays.equals(newdevice,uid)&& Applic.app.canusebluetooth() ) {
						if(value!=0|| (ret&0xFF)==5||(ret&0xFF)==7) {
							if(SensorBluetooth.resetDevice(Natives.getserial(uid,info)))
								askpermission=true;
							newdevice=null;
							}
						}
					Log.d(LOG_ID,"Badscan "+ret);
					vibrator.cancel();
					switch(ret&0xFF) {
						case 8: {
							mayEnablestreaming(tag,uid,info);
							ret=0;
							break;
							}
						case 9: {
							String sensorident = Natives.getserial(uid, info);
							Log.d(LOG_ID, "Streaming enabled, resetDevice " + sensorident);
							if(SensorBluetooth.resetDevice(sensorident))
								askpermission=true;
							}
							ret=0;
								break;
						case 4: {
							 SensorBluetooth.sensorEnded(Natives.getserial(uid, info));
							};break;
						case 3: {
							if (value == 0) {
							
								boolean actsuccess = AlgNfcV.activate(tag, info, uid);
								if(actsuccess) {
									final long[] needsactivationthrill = {20, 10, 40, 5,   2, 15,  35, 7, 12}; // [ms]
									if(android.os.Build.VERSION.SDK_INT < 26) {
										vibrator.vibrate(needsactivationthrill, -1);
										}
									else{
										final int[] needsactivationthrillamp = {0,  255, 0, 255, 0, 255, 0, 255, 0}; // 
										vibrator.vibrate(VibrationEffect.createWaveform(needsactivationthrill,needsactivationthrillamp, -1));
										}
									newdevice = uid;
								} else {
									failure(vibrator);
								}
								main.runOnUiThread(() -> {
									main.activateresult(actsuccess);

								});

								ret=0;
							}
							;
						} ;break;
							case 0x85: mayEnablestreaming(tag,uid,info); ret&=~0x80;

							case 5: {
								final long[] newsensorwait = {50, 300, 100, 10};
								if(android.os.Build.VERSION.SDK_INT < 26) 
									vibrator.vibrate(newsensorwait, -1);
								else
									vibrator.vibrate(VibrationEffect.createWaveform(newsensorwait, -1));
								String sensorident = Natives.getserial(uid, info);
								curve.render.badscan = calendar(main,ret,sensorident);
		//						ret=0;
							};break;
							case 0x87:  mayEnablestreaming(tag,uid,info); ret&=~0x80;


							case 7:
								final long[] newsensor =  {50, 150,50,50,12,8,15,73};
								if(android.os.Build.VERSION.SDK_INT < 26) 
									vibrator.vibrate(newsensor, -1);
								else
									vibrator.vibrate(VibrationEffect.createWaveform(newsensor, -1));
								String sensorident = Natives.getserial(uid, info);
								curve.render.badscan =calendar(main,ret,sensorident);
		//						ret=0;
								break;
					};
				    }

			   else  {
				ret=18;
				vibrator.cancel();
				Log.i(LOG_ID,"Read Tag Data Error");
				if(getversion(info)==2&&!Natives.switchgen2()) {
						Openfile.reinstall=true;
						Natives.closedynlib();
						Applic.RunOnUiThread(() -> { getlibrary.openlibrary(main);    });
					   }

			   }
			 }
			 }
		 }
    	catch( Throwable  error) {
			ret=19;
		       vibrator.cancel();
			String mess=error.getMessage();
			if(mess==null)
				mess="unknown error";
       			Log.stack(LOG_ID,mess,error);

			failure(vibrator);

			}
	if(value==0) {
		curve.render.badscan = ret;
		failure(vibrator);
		}
            }
    curve.requestRender();
  if(curve.waitnfc) {
	  curve.waitnfc = false;
	  ((MainActivity) curve.getContext()).setnfc();
  	  }
    main.runOnUiThread(() -> {
	if(main.curve.numcontrol!=null) 
		main.curve.numcontrol.setVisibility(GONE);
	    main.curve.numberview.closenumview(); Settings.closeview();
	    if(askpermission)
			main.finepermission();
	    });
    }

static private void newsensor(Activity act,String text,String name) {
	if(!isWearable) {
		XInfuus.sendSensorActivateBroadcast(act, name, Natives.laststarttime());
	}

//    int width=getscreenwidth(act);
    int width=GlucoseCurve.getwidth();
    int pad=width/30;
    act.runOnUiThread(() -> {
	TextView tv=getlabel(act,text);
	tv.setTextSize(TypedValue.COMPLEX_UNIT_SP, 24f);

        CheckBox calendar = new CheckBox(act);
    calendar.setPadding(0,pad,0,pad);
	calendar.setChecked(true);
	calendar.setText(R.string.addsensorenddate);
	Button ok=getbutton(act,R.string.ok);
	Layout lay=new Layout(act, (l, w, h) -> {
		if(width>2)
			l.setX((width-w)/2);
		var height=GlucoseCurve.getheight();
		if(height>h)
			l.setY((height-h)/2);

			return new int[] {w,h};
		},new View[]{tv},new View[]{calendar},new View[]{ok});
    	ok.setOnClickListener(v->{
	   	removeContentView(lay); 
		if(calendar.isChecked()) {
			insertcalendar(act,name) ;
			}

		});
	lay.setPadding(pad,pad,pad,pad);
    lay.setBackgroundColor(Applic.backgroundcolor);
	act.addContentView(lay, new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));
	});
	}
static boolean askcalendar=true;
private static int calendar(Activity act,int ret,String name) {
	if(askcalendar)  {
		int waitmin=(ret&0xff)==5?ret>>8:0;
		String mess=(waitmin>0) ?"Sensor "+name+" ready in "+waitmin+" minutes":"Sensor ready for use";
		newsensor(act,mess,name);
		askcalendar=false;
		return 0;
		}
	else
		return ret;
	}
private static void insertcalendar(Activity act,String name) {
	long endtime=Natives.sensorends()*1000L;
	if(endtime<= System.currentTimeMillis())
		return;
	try {
		Intent intent = new Intent(Intent.ACTION_INSERT)
		.putExtra(CalendarContract.Events.TITLE, act.getString(R.string.enddatesensor)+name)
		.putExtra(CalendarContract.Events.DESCRIPTION, "sensor "+name+" ends at this time")
		.setData(CalendarContract.Events.CONTENT_URI)
		.putExtra(CalendarContract.EXTRA_EVENT_BEGIN_TIME, endtime)
		.putExtra(CalendarContract.EXTRA_EVENT_END_TIME, endtime+1000L);
		act.startActivity(intent);
		} 
	catch(Throwable error) {
			String mess=error.getMessage();
			if(mess==null) mess="";
       			Log.stack(LOG_ID,mess,error);
			}
	}

}
