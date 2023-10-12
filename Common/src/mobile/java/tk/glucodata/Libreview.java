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
/*      Fri Jan 27 15:32:11 CET 2023                                                 */


package tk.glucodata;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Build;
import android.text.InputType;
import android.text.method.PasswordTransformationMethod;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.Toast;

import com.google.android.gms.security.ProviderInstaller;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.Locale;
import java.util.UUID;

import androidx.annotation.Keep;
import tk.glucodata.settings.LibreNumbers;

import static android.view.View.GONE;
import static android.view.View.INVISIBLE;
import static android.view.View.VISIBLE;
import static java.net.HttpURLConnection.HTTP_OK;
import static tk.glucodata.Backup.getedit;
import static tk.glucodata.Log.stackline;
import static tk.glucodata.Natives.clearlibreview;
import static tk.glucodata.Natives.getlibreDeviceID;
import static tk.glucodata.Natives.getlibrebaseurl;
import static tk.glucodata.Natives.getlibreemail;
import static tk.glucodata.Natives.getlibrepass;
import static tk.glucodata.Natives.getnewYuApiKey;
import static tk.glucodata.Natives.getuselibreview;
import static tk.glucodata.Natives.savelibrerubbish;
import static tk.glucodata.Natives.setlibreAccountID;
import static tk.glucodata.Natives.setlibreDeviceID;
import static tk.glucodata.Natives.setlibrebaseurl;
import static tk.glucodata.Natives.setlibreemail;
import static tk.glucodata.Natives.setlibrepass;
import static tk.glucodata.Natives.setnewYuApiKey;
import static tk.glucodata.Natives.setuselibreview;
import static tk.glucodata.Natives.wakelibreview;
import static tk.glucodata.RingTones.EnableControls;
import static tk.glucodata.bluediag.datestr;
import static tk.glucodata.help.help;
import static tk.glucodata.settings.Settings.editoptions;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getcheckbox;
import static tk.glucodata.util.getlabel;
import static tk.glucodata.NightPost.getstring;
import static tk.glucodata.util.getlocale;

public class Libreview  {
	private static final String LOG_ID="Libreview";
private static String getputtext(String sensorid,String usertoken,String gateway) {
 return "{\"DomainData\":\"{\\\"activeSensor\\\":\\\""+sensorid+"\\\"}\",\"UserToken\":\""+usertoken +"\",\"Domain\":\"Libreview\",\"GatewayType\":\""+gateway+"\"}";
 }


private static String getDeviceID(boolean libre3) {
	String id=getlibreDeviceID(libre3);
	if(id==null||id.length()<36)  {
		id=UUID.randomUUID().toString();
		setlibreDeviceID(libre3,id);
		}
	return id;
	}
	/*
private static int getalldata(HttpURLConnection urlConnection,byte[] buf) throws IOException {
	try(InputStream in = urlConnection.getInputStream()) {
		int off=0,len;
		while((len=in.read(buf,off,alllen-off))>0) {
			off+=len;
			}
		return off;
		}
	finally {
		urlConnection.disconnect();
		}

	}
static JSONObject  readJSONObject(HttpURLConnection urlConnection)  throws IOException, JSONException {
	byte[] buf=new byte[10*4096];
	int len=getalldata(urlConnection,buf);
	String ant=new String(getSlice(buf, 0, len));
	Log.i(LOG_ID,"readJSONObject len="len+" "+ant);
 	return new JSONObject(ant);
	}*/

static JSONObject  readJSONObject(HttpURLConnection urlConnection)  throws IOException, JSONException {
	String ant=getstring(urlConnection);
	Log.format("%s: readJSONObject len=%d %s",LOG_ID,ant.length(),ant);
 	return new JSONObject(ant);
	}
final private static String success="Success".intern();
final private static String nothing="Tried nothing".intern();

private static String librestatus=nothing;
@Keep
static boolean putsensor(boolean libre3,byte[] textbytes) {
	if(librestatus==nothing||librestatus==success)
		librestatus=datestr(System.currentTimeMillis())+" start putsensor";
	try {
		final String gateway=getlibregateway(libre3);
		final String baseurl=getlibrebaseurl(libre3);
		URL url = new URL(baseurl+"/api/nisperson");
		HttpURLConnection urlConnection = (HttpURLConnection) url.openConnection();
		urlConnection.setDoOutput(true);
	       urlConnection.setRequestMethod("PUT");
		urlConnection.setRequestProperty("Content-Type","application/json; charset=UTF-8");
		String usertoken=Natives.getlibreUserToken(libre3);
		urlConnection.setRequestProperty( "Content-Length", Integer.toString( textbytes.length ));
		OutputStream outputPost = new BufferedOutputStream(urlConnection.getOutputStream());
		outputPost.write(textbytes);
		outputPost.flush();
		outputPost.close();
 		JSONObject object = readJSONObject(urlConnection);
		final int status=object.getInt("status");
		if(status!=0) {
			String reason=object.getString("reason");
			librestatus="putsensor: status="+status+reason==null?"":(" reason="+reason);
			}
		return status==0;
		}  
	catch(Throwable th) {
		librestatus="putsensor "+ stackline(th);
		Log.e(LOG_ID,librestatus);
		return false;
		}
	}
static String getlibregateway(boolean libre3) {
	if(libre3)
		return "FSLibreLink3.Android";
	return "FSLibreLink.Android";
	}
static private boolean gettermversion(String lang) {
	try {
		if(termsofuseversionurl==null) {
				Log.d(LOG_ID, "termsofuseversionurl==null");
				return false;
			}
		String rep=termsofuseversionurl.replace("<locale>",lang);

		URL url = new URL(rep);
		HttpURLConnection  urlConnection = (HttpURLConnection) url.openConnection();
		urlConnection.setRequestMethod("GET");
		final int code=urlConnection.getResponseCode();
		if(code==HTTP_OK) {
			Log.i(LOG_ID,"gettermversion  success");
			return true;
			}
		else {
			Log.e(LOG_ID,"gettermversion code="+code);
			return false;
			}

		}
	catch(Throwable th) {
		Log.stack(LOG_ID,"gettermversion",th);
		return false;
		}
	}

static boolean postgetauth(boolean libre3) {
	String gateway=getlibregateway(libre3);
	String one=getDeviceID(libre3);

	String password=getlibrepass();

	String login=getlibreemail();
	Log.i(LOG_ID,"postgetauth "+login+" "+password);

	var loc= Locale.getDefault();
	String language=loc.getLanguage()+'-'+loc.getCountry();
	String culture=language;
	String setdevice="false";
	while(true) {
		try {
		final String baseurl=getlibrebaseurl(libre3);
		URL url = new URL(baseurl+"/api/nisperson/getauthentication");
		HttpURLConnection urlConnection = (HttpURLConnection) url.openConnection();
		urlConnection.setRequestMethod("POST");
		urlConnection.setDoOutput(true);
		String getauthtext;
		if(libre3) {

			urlConnection.setRequestProperty("Content-Type", "application/json");
			urlConnection.setRequestProperty("Platform","Android");
			urlConnection.setRequestProperty("Version","3.3.0");
			urlConnection.setRequestProperty("Abbott-ADC-App-Platform","Android/"+((Object) Build.VERSION.RELEASE) +"/FSL3/3.3.0.9092");
			urlConnection.setRequestProperty("Accept-Language",language);
			final String newYuApiKey=getnewYuApiKey(libre3);
			urlConnection.setRequestProperty("x-api-key", newYuApiKey);
			urlConnection.setRequestProperty("x-newyu-token",""); 
			getauthtext="{\n"+
			"  \"Culture\": \""+culture+"\",\n"+
			"  \"DeviceId\": \""+one+"\",\n"+
			"  \"Password\": \""+password+"\",\n"+
			"  \"SetDevice\": "+setdevice+",\n"+
			"  \"UserName\": \""+login+"\",\n"+
			"  \"Domain\": \"Libreview\",\n"+
			"  \"GatewayType\": \""+ gateway+ "\"\n"+
			"}\n";
			}
		else {
			urlConnection.setRequestProperty("Abbott-ADC-App-Platform", "Android/"+((Object) Build.VERSION.RELEASE)+"/FSLL/2.10.1.10406");


			urlConnection.setRequestProperty("Accept-Language",language+", "+loc.getLanguage()+";q=0.8");
			urlConnection.setRequestProperty("Content-Type", "application/json; charset=utf-8");
			getauthtext="{\"Culture\":\""+culture+"\",\"DeviceId\":\""+one+"\",\"Password\":\""+password+"\",\"SetDevice\":"+setdevice+",\"UserName\":\""+login+"\",\"Domain\":\"Libreview\",\"GatewayType\":\""+ gateway+ "\""+ "}";

			}
		byte[] textbytes=getauthtext.getBytes();
		Log.i(LOG_ID,"postauth: "+getauthtext);
	       urlConnection.setRequestProperty( "Content-Length", Integer.toString( textbytes.length ));

	//	Log.i(LOG_ID,getauthtext);
		OutputStream outputPost = new BufferedOutputStream(urlConnection.getOutputStream());
		outputPost.write(textbytes);
		outputPost.flush();
		outputPost.close();
		final int code=urlConnection.getResponseCode();
		
		Log.i(LOG_ID,"ResponseCode="+code);
		if(code==HTTP_OK) {
			JSONObject object = readJSONObject(urlConnection);
			int status=object.getInt("status");
			if(status!=0) {
				String reason=object.getString("reason");
				String poststatus="postgetauth: status="+status+" reason="+reason;
				Log.e(LOG_ID,poststatus);
				if(status==20) {
					if("wrongDeviceForUser".equals(reason)) {
						setdevice="true";
						continue;	
						}
					}
				librestatus=poststatus;
				return false;
				}
			Log.i(LOG_ID,"getauth Success");
			JSONObject result=object.getJSONObject("result");
			String usertoken=result.getString("UserToken");
			Natives.setlibreUserToken(libre3,usertoken);
			String accountid=result.getString("AccountId");
			setlibreAccountID(accountid);
			librestatus="Received AccountID";
			if(libre3) {//TODO enkel als send to libreview aanstaat?
				String DateOfBirth=result.getString("DateOfBirth");
				int dat=Integer.parseInt(DateOfBirth);
				String FirstName=result.getString("FirstName");
				String LastName=result.getString("LastName");
				String GuardianLastName=result.getString("GuardianLastName");
				String GuardianFirstName=result.getString("GuardianFirstName");
				savelibrerubbish(FirstName,LastName,dat,GuardianFirstName,GuardianLastName);
				String UiLanguage=result.getString("UiLanguage");
				gettermversion(UiLanguage);
				}
			else {
				URL url2 = new URL(baseurl+"/api/nisperson/getAccountInfo"); //IS this really needed?
				HttpURLConnection urlConnection2 = (HttpURLConnection) url2.openConnection();
				urlConnection2.setRequestMethod("POST");
				urlConnection2.setDoOutput(true);

				urlConnection2.setRequestProperty("Abbott-ADC-App-Platform", "Android/"+((Object) Build.VERSION.RELEASE)+"/FSLL/2.10.1.10406");
				urlConnection2.setRequestProperty("Accept-Language",language+", "+loc.getLanguage()+";q=0.8");
				urlConnection2.setRequestProperty("Content-Type", "application/json; charset=utf-8");
				String notneeded="{\"UserToken\":\""+usertoken+"\",\"Domain\":\"Libreview\",\"GatewayType\":\"FSLibreLink.Android\"}";
				byte[] notneedbytes=notneeded.getBytes();
				Log.i(LOG_ID,"postauth: "+notneeded);
			       urlConnection2.setRequestProperty( "Content-Length", Integer.toString( notneedbytes.length ));
				OutputStream outputPost2 = new BufferedOutputStream(urlConnection2.getOutputStream());
				outputPost2.write(notneedbytes);
				outputPost2.flush();
				outputPost2.close();
				final int code2=urlConnection2.getResponseCode();
				Log.i(LOG_ID,"ResponseCode="+code2);
				if(code2!=HTTP_OK) {
						librestatus="getAccountInfo: getResponseCode()="+code2;
						}
				}
			return true;
			}
		else {
			librestatus="postgetauth: urlConnection.getResponseCode()="+code;
			return false;
			}
		 }
		catch(Throwable th) {
			librestatus="postgetauth:\t"+ stackline(th);

			Log.e(LOG_ID,librestatus);
			return false;
			}
	}
 }
 /*
@Keep
static boolean postmeasurements(byte[] measurementdata) {
	return postmeasurements(false, measurementdata);
	}*/

static String posttime=null;
@Keep
static boolean postmeasurements(boolean libre3,byte[] measurementdata) {
	String nowstr=datestr(System.currentTimeMillis());
	if(librestatus==nothing||librestatus==success)
		librestatus=nowstr+" start posting";
	try {
	for(int i=0;i<3;i++) {
		final String baseurl=getlibrebaseurl(libre3);
		URL url = new URL(baseurl+"/api/measurements");
		HttpURLConnection urlConnection = (HttpURLConnection) url.openConnection();
		urlConnection.setConnectTimeout(10000);
		urlConnection.setReadTimeout(60000);
		urlConnection.setRequestMethod("POST");
		urlConnection.setDoOutput(true);
		urlConnection.setRequestProperty("Content-Type", "application/json");
	       urlConnection.setRequestProperty( "Content-Length", Integer.toString( measurementdata.length ));
		if(libre3) {
			urlConnection.setRequestProperty("Platform","Android");
			urlConnection.setRequestProperty("Version","3.3.0");
			urlConnection.setRequestProperty("Abbott-ADC-App-Platform","Android/"+((Object) Build.VERSION.RELEASE) +"/FSL3/3.3.0.9092");
 
			var loc= Locale.getDefault();
			String language=loc.getLanguage()+'-'+loc.getCountry();
			urlConnection.setRequestProperty("Accept-Language",language);
			final String newYuApiKey=getnewYuApiKey(libre3);
			urlConnection.setRequestProperty("x-api-key", newYuApiKey);
			final String usertoken=Natives.getlibreUserToken(libre3);
			urlConnection.setRequestProperty("x-newyu-token",usertoken); 
			}
		OutputStream outputPost = new BufferedOutputStream(urlConnection.getOutputStream());
		outputPost.write(measurementdata);
		outputPost.flush();
		outputPost.close();
		final int code=urlConnection.getResponseCode();
		if(code==HTTP_OK) {
			JSONObject object = readJSONObject(urlConnection);
			int status=object.getInt("status");
			if(status!=0) {
				Log.e(LOG_ID,"Post with status "+status);
				String reason=object.getString("reason");
				if(status==20) {
					if("wrongDeviceInToken".equals(reason)) {
						switch(i) {
							case 0:{
								if(!postgetauth(libre3)) {
									if(!libreconfig(libre3,false))
										return false;
									i=1;
									}
								};break;
							case 1: {
								if(!libreconfig(libre3,false))
									return false;
								};break;

							}
//						return postmeasurements(libre3,measurementdata);
						continue;
						}
					}
				librestatus="postmeasurements status="+code+" reason="+reason;
				return false;
				}
			posttime=nowstr;
			librestatus=success;
			return true;
			}
		else {
			librestatus="postmeasurements ResponseCode="+code;
			Log.i(LOG_ID,librestatus);
			return false;
			}
			}
		return false;
		 }
	catch(Throwable th) {
		final String posterror="postmeasurements\n"+stackline(th);
		librestatus=posterror;
		Log.e(LOG_ID,posterror);
		return false;
		}
 }
 /*TODO: where:
		  try {
			  ProviderInstaller.installIfNeeded(Applic.app);
		  }
		catch(Throwable th) {
			librestatus= "ProviderInstaller.installIfNeeded: \n"+stackline(th);
			 Log.e(LOG_ID,librestatus);
			  }
*/
//	https://fsll3.freestyleserver.com/Payloads/Mobile/FFSLibre3/Android/Assets/3.3.0%2FDE.json
private static String termsofuseversionurl=null;
private static final String libre3start="https://fsll3.freestyleserver.com/Payloads/Mobile/FSLibre3/Android/Assets/3.3.0/DE.json";
private static String  libre3getconfigURL() {
	try {

		URL url = new URL(libre3start);
		HttpURLConnection  urlConnection = (HttpURLConnection) url.openConnection();
		urlConnection.setRequestMethod("GET");
		final int code=urlConnection.getResponseCode();
		if(code==HTTP_OK) {
			JSONObject object =  readJSONObject(urlConnection) ;
			final String conurl=object.getString( "Configuration");
			try {
			   termsofuseversionurl=object.getString( "TermsOfUseVersion");
			 }
			 catch(Throwable th) {
				librestatus="libre3getconfigURL 1:\n"+stackline(th);
				Log.e(LOG_ID,librestatus);
				}
			finally {
				return conurl;
				} 
			}
		else {
			librestatus="libre3getconfigURL failed code="+code;
			Log.e(LOG_ID,librestatus);
			return null;
			}

		}
	catch(Throwable th) {
		librestatus="libre3getconfigURL:\n"+(th==null?"Network error ":th.getMessage());
		Log.e(LOG_ID,librestatus);
		return null;
		}
	}
	/*
public static void testlibre3() { 
	String url=libre3getconfigURL();
	Log.i(LOG_ID,"libre3getconfigURL()="+(url==null?"null":url));
	}*/
//https://fsll.freestyleserver.com/Payloads/Mobile/Android/FSLibreLink/Config/FreeStyleLibreLink_Android_2.3_DE_config.json
@Keep
public static boolean libreconfig(boolean libre3,boolean restart){
	if(restart||librestatus==nothing||librestatus==success)
		librestatus=datestr(System.currentTimeMillis())+" libreconfig";
	Log.i(LOG_ID,librestatus);
	  try {
		  ProviderInstaller.installIfNeeded(Applic.app);
	  }
	catch(Throwable th) {
		librestatus= "ProviderInstaller.installIfNeeded: \n"+stackline(th);
		 Log.e(LOG_ID,librestatus);
		  }

//	final String libre23url= "https://www.google.com";
//	final String libre23url= "https://fsll.freestyleserver.com/Payloads/Mobile/Android/FSLibreLink/Config/FreeStyleLibreLink_Android_2.3_DE_config.json";
final String libre210url=Natives.isLibreMmol()?
"https://fsll.freestyleserver.com/Payloads/Mobile/FSLibreLink/Android/Config/FSLibreLink_Android_2.10_GB_config.json":
"https://fsll.freestyleserver.com/Payloads/Mobile/FSLibreLink/Android/Config/FSLibreLink_Android_2.10_FR_config.json";

//final String libre33url="https://fsll3.freestyleserver.com/Payloads/Mobile/FSLibre3/Android/Config/FSLibre3_Android_3.3_DE_config_production.json";
	String urlstring;
	if(libre3) {
		urlstring=libre3getconfigURL();
		if(urlstring==null) {
			return false;
			}
		}
	else
		urlstring=libre210url;

	try {
		URL url = new URL(urlstring);
		if(url==null)  {
			return false;
			}
		HttpURLConnection urlConnection = (HttpURLConnection) url.openConnection();
		urlConnection.setRequestMethod("GET");

		final int code=urlConnection.getResponseCode();
		if(code==HTTP_OK) {
			JSONObject object =  readJSONObject(urlConnection) ;
			final String baseurl=object.getString( "newYuUrl");
			setlibrebaseurl(libre3,baseurl);
			final var jobj = object.opt("newYuApiKey");

			if(jobj!=null)  {
				String value=jobj instanceof String?(String)jobj: String.valueOf(jobj);
				setnewYuApiKey(libre3,value);
			}
			return postgetauth(libre3);
			}
		else {
			librestatus="urlConnection.getResponseCode()="+code;
			Log.e(LOG_ID,librestatus);
			return false;
			}

		}
	catch(Throwable th) {
		librestatus="libreconfig:\n"+stackline(th);

		Log.e(LOG_ID,librestatus);
		return false;
		}
	}

private static	void askclearlibreview(Context context) {
        AlertDialog.Builder builder = new AlertDialog.Builder(context);
        var dialog=builder.setTitle(R.string.resendquestion).
	 setMessage(R.string.resendmessage).
           setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
			clearlibreview() ;

                    }
                }) .setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {
            }
        }).create();
	dialog.setCanceledOnTouchOutside(false);
	dialog.show();
	}

private static void		confirmGetAccountID(MainActivity context) {
        AlertDialog.Builder builder = new AlertDialog.Builder(context);
       var dialog=builder.setTitle(R.string.getaccountidquestion).
	 setMessage(R.string.getaccountidmessage).
           setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
			Natives.setlibreAccountIDnumber(-1L);
			Natives.askServerforAccountID();
                    }
                }) .setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {
            }
        }).create();
	dialog.setCanceledOnTouchOutside(false);
	dialog.show();
	}



private static void getAccountid(MainActivity context, 	Predicate<Boolean> getgegs,View settingsview,CheckBox sendto,boolean[] donothing) {
	var setmanually=Natives.manualLibreAccountIDnumber()!=-1L;
	var manual=getcheckbox(context,R.string.manual, setmanually);;

	long accountidnum=Natives.getlibreAccountIDnumber();
	var editid = new EditText(context);
	editid.setText(accountidnum+"");
	editid.setImeOptions(tk.glucodata.settings.Settings.editoptions);
	editid.setInputType(InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD);
	     editid.setMinEms(5);
	var fromlibreview=getbutton(context,R.string.fromlibreview);
	var save=getbutton(context,R.string.save);
	var close=getbutton(context,R.string.closename);

	var help=getbutton(context,R.string.helpname);

	help.setOnClickListener(v-> help(R.string.getaccountidhelp,context));
	Consumer<Boolean> domanual= isChecked -> {
		if(!isChecked) {
			fromlibreview.setVisibility(VISIBLE);
			editid.setVisibility(INVISIBLE);
			save.setVisibility(INVISIBLE);

		}
		else {
			editid.setVisibility(VISIBLE);
			fromlibreview.setVisibility(INVISIBLE);
			save.setVisibility(VISIBLE);
		}
	};
	domanual.accept(setmanually);
	manual.setOnCheckedChangeListener(
			(buttonView,  isChecked) ->
				domanual.accept(isChecked)


	);
      manual.setPadding(0,0,(int)(tk.glucodata.GlucoseCurve.metrics.density*10),0);
	final Layout layout=new Layout(context, (lay, w, h) -> {
		var height=GlucoseCurve.getheight();
		var width=GlucoseCurve.getwidth();
                        if(w>=width||h>=height) {
                                lay.setX(0);
                                lay.setY(0);
                                }
                        else {
                                lay.setX((width-w)/2); lay.setY(0);
                                };
                        return new int[] {w,h};}, new View[]{manual,editid,help},new View[]{fromlibreview,close,save});

      layout.setBackgroundResource(R.drawable.dialogbackground);
      int pad= (int)tk.glucodata.GlucoseCurve.metrics.density*7;
	 layout.setPadding(pad,pad,pad,pad);
	context.addContentView(layout, new ViewGroup.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT,ViewGroup.LayoutParams.WRAP_CONTENT));
	Runnable closerun=()-> {
		layout.setVisibility(GONE);
		removeContentView(layout);
		config(context,  settingsview,sendto, donothing);
		};
	save.setOnClickListener(v->  {
		if(manual.isChecked()) {
			String idstr = editid.getText().toString();
			try {
				if (idstr.length() > 0) {
					long id = Long.parseLong(idstr);
					Natives.setlibreAccountIDnumber(id);

					Toast.makeText(context, context.getString(R.string.saved)+ " "+ id, Toast.LENGTH_SHORT).show();
				} else {

					Toast.makeText(context, context.getString(R.string.noaccountidspecified), Toast.LENGTH_SHORT).show();
					return;
				}
			} catch (Throwable th) {
				Toast.makeText(context, context.getString(R.string.wrongformat) + idstr, Toast.LENGTH_SHORT).show();
				Log.stack(LOG_ID, "parse account id", th);
				return;
			}
		}
		else {
			Natives.setlibreAccountIDnumber(-1L);
			}

		context.doonback();
		});
	
	fromlibreview.setOnClickListener(v->   {
		if(!getgegs.test(true))
				return;

		confirmGetAccountID(context) ;
		}
		);

	context.setonback(closerun);
	close.setOnClickListener(v->  {
			context.doonback();
			});
	}
//		Natives.askServerforAccountID();
public static void  config(MainActivity act, View settingsview,CheckBox sendto,boolean[] donothing) {
	EnableControls(settingsview,false);
	var emaillabel=getlabel(act,R.string.email);
	var email=getedit(act, getlibreemail());
        email.setMinEms(12);

	var passlabel=getlabel(act,act.getString(R.string.password)+":");
	var      editpass= new EditText(act);
        editpass.setImeOptions(editoptions);
        editpass.setInputType(InputType.TYPE_TEXT_VARIATION_PASSWORD);
        editpass.setTransformationMethod(new PasswordTransformationMethod());
        editpass.setMinEms(12);
	String passwas=getlibrepass();
	if(passwas!=null) {
		editpass.setText(passwas);
		}
	var send=getbutton(act,act.getString(R.string.sendnow));
	var ok=getbutton(act,R.string.ok);
	var cancel=getbutton(act,R.string.cancel);
	var help=getbutton(act,R.string.helpname);
	help.setOnClickListener(v-> help(R.string.libreview,act));
	boolean usedlibre= getuselibreview();
	var sendtolibreview=getcheckbox(act,R.string.uselibreview,usedlibre);
	var librecurrent=getcheckbox(act,R.string.librecurrent,Natives.getLibreCurrent());
	librecurrent.setOnCheckedChangeListener( (buttonView,  isChecked) -> {
		Natives.setLibreCurrent(isChecked);
		});
	var libreisviewed=getcheckbox(act,R.string.libreisviewed,Natives.getLibreIsViewed());
	libreisviewed.setOnCheckedChangeListener( (buttonView,  isChecked) -> {
		Natives.setLibreIsViewed(isChecked);
		});

	int[] nochangeamounts={0};
	var numbers=getcheckbox(act,R.string.sendamounts,Natives.getSendNumbers());
	sendtolibreview.setOnCheckedChangeListener( (buttonView,  isChecked) -> {
		numbers.setVisibility(isChecked?VISIBLE:INVISIBLE);;
		});
	var clear=getbutton(act,R.string.resenddata);
	  var statusview=getlabel(act,librestatus==success?(posttime+": "+librestatus):librestatus);
	  int statuspad=  (int)tk.glucodata.GlucoseCurve.metrics.density*7;
	statusview.setPadding(statuspad,statuspad,statuspad,statuspad);
	clear.setOnClickListener(v->  {
			askclearlibreview(act);
			});
	
//	  clear.setPadding(0,0,0,pad*5);
	long accountidnum=Natives.getlibreAccountIDnumber();
	var accountid=getlabel(act, String.valueOf(accountidnum));
	var getaccountid=getbutton(act,R.string.getaccountid);
var space=getlabel(act,"        ");
	final Layout layout=new Layout(act, (lay, w, h) -> {
		var height=GlucoseCurve.getheight();
		var width=GlucoseCurve.getwidth();
                        if(w>=width||h>=height) {
                                lay.setX(0);
                                lay.setY(0);
                                }
                        else {
                                lay.setX((width-w)/2); lay.setY(0);
                                };
                        return new int[] {w,h};}, new View[]{emaillabel,email},new View[]{passlabel,editpass},new View[]{clear,accountid,getaccountid},new View[]{statusview},new View[]{sendtolibreview,librecurrent,libreisviewed,numbers},new View[]{send,help,cancel,ok});
	if(usedlibre) {
		send.setOnClickListener(v-> wakelibreview(0));
		}
	else  {
		numbers.setVisibility(INVISIBLE);;
		send.setVisibility(INVISIBLE);
		}

	numbers.setOnCheckedChangeListener( (buttonView,  isChecked) -> {
		switch(nochangeamounts[0])  {
			case 0: {
				++nochangeamounts[0];
				numbers.setChecked(!isChecked);
				LibreNumbers.mklayout(act,0,numbers,nochangeamounts,layout);
				};break;
			case  2: Natives.setSendNumbers(isChecked);break;
			};
		});
     

	Runnable closerun=()-> {
		layout.setVisibility(GONE);
		removeContentView(layout);
		EnableControls(settingsview,true);
		sendto.setChecked(usedlibre);
		donothing[0]=false;
		};
	act.setonback(closerun);
	cancel.setOnClickListener(v->  {
			act.poponback();
			closerun.run();
			});
 	Predicate<Boolean> getgegs= use -> {
			String emailstr=email.getText().toString();
			String passstr=editpass.getText().toString();
			if(use) {
				if(emailstr.length()<3) {
					Toast.makeText(act, act.getString( R.string.emailaddresstooshort)+emailstr, Toast.LENGTH_SHORT).show();
					return false;
					}
				if(emailstr.length()>255) {
					Toast.makeText(act,  act.getString( R.string.emailaddresstoolong)+emailstr, Toast.LENGTH_SHORT).show();
					return false;
					}
				if(passstr.length()<3) {
					Toast.makeText(act, act.getString(R.string.password8)+passstr, Toast.LENGTH_SHORT).show();
					return false;
					}
				if(passstr.length()>36) {
					Toast.makeText(act,  act.getString(R.string.password36)+passstr, Toast.LENGTH_SHORT).show();
					return false;
					}
				}	
			setlibreemail(emailstr);
			setlibrepass(passstr);
			if((emailstr.length()==0&&passstr.length()==0)) {
				clearlibreview();
				}
			return true;
		};
	ok.setOnClickListener(v-> {
			boolean turnonlibre=sendtolibreview.isChecked();
			if(!getgegs.test(turnonlibre))
				return;
			setuselibreview(turnonlibre);
				
			act.poponback();
			layout.setVisibility(GONE);
			removeContentView(layout);
			EnableControls(settingsview,true);
			sendto.setChecked(turnonlibre);
			donothing[0]=false;
			});
	getaccountid.setOnClickListener(v->  {
//		confirmGetAccountID(act);
			boolean turnonlibre=sendtolibreview.isChecked();
			getgegs.test(false);

			layout.setVisibility(GONE);
			removeContentView(layout);
		 getAccountid(act,getgegs,  settingsview,sendto, donothing);
//		Natives.askServerforAccountID();
		});
	      layout.setBackgroundResource(R.drawable.dialogbackground);
	      int pad= (int)tk.glucodata.GlucoseCurve.metrics.density*7;
		  layout.setPadding(pad,pad,pad,pad);
		act.addContentView(layout, new ViewGroup.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT,ViewGroup.LayoutParams.WRAP_CONTENT));
	
	}



}





