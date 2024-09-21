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
/*      Thu Mar 23 21:04:47 CET 2023                                                 */


package tk.glucodata;
import static android.view.View.GONE;
import static java.net.HttpURLConnection.HTTP_OK;

import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.Backup.getedit;
import static tk.glucodata.Log.stackline;
import static tk.glucodata.Natives.setNightUploader;
import static tk.glucodata.RingTones.EnableControls;
import static tk.glucodata.bluediag.datestr;
import static tk.glucodata.help.help;
import static tk.glucodata.settings.Settings.editoptions;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getcheckbox;
import static tk.glucodata.util.getlabel;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.text.InputType;
import android.text.method.PasswordTransformationMethod;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.ScrollView;

import androidx.annotation.Keep;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;

import tk.glucodata.settings.LibreNumbers;

public class NightPost  {
	private static final String LOG_ID="NightPost";

static String getstring(HttpURLConnection con)  throws IOException{
	try(BufferedReader in = new BufferedReader(new InputStreamReader(con.getInputStream()))) {
		StringBuffer response = new StringBuffer();
		String inputLine;
		while ((inputLine = in.readLine()) != null) {
			response.append(inputLine);
			}
		return response.toString();
		}
	finally {
		con.disconnect();
		}
	}

private static  String getstart(HttpURLConnection con,int max)  throws IOException{
	try(var in = con.getInputStream()) {
		int len=max;
		byte[] buf=new byte[len];
		int res=in.read(buf,0,len);
		return new String(buf,0,res);
		}
	finally {
		con.disconnect();
		}
	}

final  static String nothing=Applic.app.getString(R.string.triednothing).intern();
final static String success=Applic.app.getString(R.string.success).intern();
static private String uploadstatus=nothing;
@Keep
static public boolean deleteUrl(String urlstring,String secret) {
	uploadtime=System.currentTimeMillis();
	Log.i(LOG_ID,"deleteUrl "+urlstring+" "+ secret);
	try {
		URL url = new URL(urlstring);
		if(url==null)  {
			uploadstatus="URL("+urlstring+")==null";
			return false;
			}
    uploadstatus=" start deleteURL "+urlstring;
		HttpURLConnection urlConnection = (HttpURLConnection) url.openConnection();
		urlConnection.setConnectTimeout(10000);
		urlConnection.setReadTimeout(60000);
		if(secret!=null)
				urlConnection.setRequestProperty("api-secret", secret);
		else
			urlConnection.setRequestProperty("Authorization", gettoken(uploadtime));
		urlConnection.setRequestProperty("Content-Type", "application/json");
		urlConnection.setRequestMethod("DELETE");

		final int code=urlConnection.getResponseCode();
		String res=getstring(urlConnection);
		if(code==HTTP_OK) {
			Log.i(LOG_ID,"deleteUrl success "+res);
			uploadstatus=success;
			return true;
			}
		else {
			String delerror="deleteUrl "+urlstring+" failure code="+code+'\n'+res;
			Log.e(LOG_ID,delerror);
			uploadstatus=delerror;
			return false;
			}

		}
	catch(Throwable th) {
		String error ="deleteUrl error:\n"+stackline(th);
		uploadstatus=error;
		Log.e(LOG_ID,error);
		return false;
		}
	}

/*
{
  "token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhY2Nlc3NUb2tlbiI6ImFhcHMtOTQ0Y2YzZGVkYTMxMTkxNCIsImlhdCI6MTcwODg1NDE1NiwiZXhwIjoxNzA4ODgyOTU2fQ.YrNGSUPiz-3zxv6ZxfOO_Sm98bKrK0eDjZYIR6LPQUY",
  "sub": "aaps",
  "permissionGroups": [
    [
      "*"
    ],
    []
  ],
  "iat": 1708854156,
  "exp": 1708882956
} */
static private long  expire=0L;
static private String token="";

static JSONObject  readJSONObject(HttpURLConnection urlConnection)  throws IOException, JSONException {
	String ant=getstring(urlConnection);
	Log.format("%s: readJSONObject len=%d %s",LOG_ID,ant.length(),ant);
 	return new JSONObject(ant);
	}

private static String gettoken(long now) {
	if(now<expire)
		return token;
	var Nighturl=Natives.getnightuploadurl();
	var secret=Natives.getnightuploadsecret();
	var authstr=Nighturl+ "/api/v2/authorization/request/"+secret;
	try {

		URL url = new URL(authstr);
		HttpURLConnection  urlConnection = (HttpURLConnection) url.openConnection();
		urlConnection.setConnectTimeout(10000);
		urlConnection.setReadTimeout(60000);
		urlConnection.setRequestMethod("GET");
		final int code=urlConnection.getResponseCode();
		if(code==HTTP_OK) {
			JSONObject object =  readJSONObject(urlConnection) ;
			final String tokenin=object.getString( "token");
			final var expirein=object.getLong( "exp");
			expire=expirein*1000L;
			token="Bearer "+tokenin;
			return token;
			}
		else {
			uploadstatus="gettoken failed code="+code;
			Log.e(LOG_ID,uploadstatus);
			return "";
			}

		}
	catch(Throwable th) {
		uploadstatus="gettoken:\n"+(th==null?"Network error ":th.getMessage());
		Log.e(LOG_ID,uploadstatus);
		return "";
		}
	}

private static long uploadtime=System.currentTimeMillis();
@Keep
static public int upload(String httpurl,byte[] postdata,String secret,boolean put) {
	uploadtime=System.currentTimeMillis();
	Log.i(LOG_ID,"upload("+httpurl+",#"+postdata.length+","+ secret+","+(put?"PUT":"POST")+")");
	try {

      uploadstatus="start upload "+httpurl;
		URL url = new URL(httpurl);
		HttpURLConnection urlConnection = (HttpURLConnection) url.openConnection();
		urlConnection.setConnectTimeout(10000);
		urlConnection.setReadTimeout(60000);
		urlConnection.setRequestMethod(put?"PUT":"POST");
		urlConnection.setDoOutput(true);
		if(secret!=null)
			urlConnection.setRequestProperty("api-secret", secret);
		else
			urlConnection.setRequestProperty("Authorization", gettoken(uploadtime));
		urlConnection.setRequestProperty("Content-Type", "application/json");
	       urlConnection.setRequestProperty( "Content-Length", Integer.toString( postdata.length ));

		OutputStream outputPost = new BufferedOutputStream(urlConnection.getOutputStream());
		outputPost.write(postdata);
		outputPost.flush();
		outputPost.close();
		final int code=urlConnection.getResponseCode();
		String res=getstring(urlConnection);
		final String resstr="upload ResponseCode="+code+"\n"+res;
		if(code!=200&&code!=201) {
			uploadstatus=resstr;
			Log.e(LOG_ID,resstr);
			}
		else {
			uploadstatus=success;
			Log.i(LOG_ID,resstr);
			}
		return code;
		 }
	catch(Throwable th) {
		final String posterror="upload failure:\n"+stackline(th);
		uploadstatus=posterror;
		Log.e(LOG_ID,posterror);
		return -1;
		}
 	}
private static	void askclearupload(Context context) {
        AlertDialog.Builder builder = new AlertDialog.Builder(context);
        builder.setTitle(R.string.resendquestion).
           setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
			Natives.resetuploader();
                    }
                }) .setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {
            }
        }).show().setCanceledOnTouchOutside(false);
	}

public static void  config(MainActivity act, View settingsview) {
	EnableControls(settingsview,false);
	var urllabel=getlabel(act,"Nightscout URL");
	var url=getedit(act, Natives.getnightuploadurl());
	final int minems=isWearable?12:16;
        url.setMinEms(minems);
	var secretlabel=getlabel(act,R.string.secret);
	secretlabel.setPadding(0,0,0,0);
    var editsecret= new EditText(act);
        editsecret.setImeOptions(editoptions);
        editsecret.setInputType(InputType.TYPE_TEXT_VARIATION_PASSWORD);
        editsecret.setTransformationMethod(new PasswordTransformationMethod());
	 editsecret.setMinEms(minems);
	String secretwas=Natives.getnightuploadsecret();
	if(secretwas!=null) {
		editsecret.setText(secretwas);
		}
	var save=getbutton(act,R.string.save);
	var cancel=getbutton(act,R.string.cancel);
	var clear=getbutton(act,R.string.resenddata);
	clear.setOnClickListener(v->  askclearupload(act));
	var wake=getbutton(act,act.getString(R.string.sendnow));
	wake.setOnClickListener(v-> Natives.wakeuploader());
	Button help;
	CheckBox treatments;
	if(!isWearable) {
		help=getbutton(act,R.string.helpname);
		help.setOnClickListener(v-> help(R.string.NightPost,act));
		treatments=getcheckbox(act,R.string.sendamounts,Natives.getpostTreatments());
		}
	final CheckBox v3box=!isWearable?getcheckbox(act,"test V3",Natives.getnightscoutV3()):null;
	boolean useuploader=Natives.getuseuploader();
	var activebox=getcheckbox(act,R.string.active,useuploader);
       var visible = new CheckBox(act);
       visible.setText(R.string.visible);
	int pad= (int)tk.glucodata.GlucoseCurve.metrics.density*7;
	visible.setPadding(0,0,pad,0);
        visible.setOnCheckedChangeListener( (buttonView,  isChecked)-> {
                        editsecret.setInputType(isChecked?InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD:InputType.TYPE_TEXT_VARIATION_PASSWORD);
                        if(isChecked)
                                        editsecret.setTransformationMethod(null);
                        else
                                        editsecret.setTransformationMethod(new PasswordTransformationMethod());
                        });

	  var statusview=getlabel(act,datestr(uploadtime)+": "+uploadstatus);
	  int statuspad=  (int)tk.glucodata.GlucoseCurve.metrics.density*7;
	statusview.setPadding(statuspad,statuspad,statuspad,statuspad);

	final Layout layout=isWearable?new Layout(act, (lay, w, h) -> { return new int[] {w,h};}, new View[]{secretlabel},new View[]{visible},new View[]{editsecret},new View[]{urllabel},new View[]{url},new View[]{statusview},new View[]{clear},new View[]{wake},new View[]{activebox,cancel},new View[]{save}):new Layout(act, (lay, w, h) -> {
		var height=GlucoseCurve.getheight();
		var width=GlucoseCurve.getwidth();
                        if(w>=width||h>=height) {
                                lay.setX(0);
                                }
                        else {
                                lay.setX((width-w)/2); 
                                };

			lay.setY(MainActivity.systembarTop);
                        return new int[] {w,h};}, new View[]{urllabel,url},new View[]{secretlabel,visible,editsecret},new View[]{statusview},new View[]{activebox,v3box,clear,wake},new View[]{treatments,help,cancel,save});

		int laypar;
		final View allview=isWearable?new ScrollView(act):layout;
		if(isWearable) {
			((ScrollView)allview).addView(layout);
			laypar=ViewGroup.LayoutParams.MATCH_PARENT;
			layout.setBackgroundColor(tk.glucodata.Applic.backgroundcolor);
		} else {
			int[] nochangeamounts={0};
			treatments.setOnCheckedChangeListener( (buttonView,  isChecked) -> {
				switch(nochangeamounts[0])  {
					case 0: {
						++nochangeamounts[0];
						treatments.setChecked(!isChecked);
						LibreNumbers.mklayout(act,1,treatments,nochangeamounts,layout);
						};break;
					case  2: Natives.setpostTreatments(isChecked);break;

					};
				});
			laypar=ViewGroup.LayoutParams.WRAP_CONTENT;
		      allview.setBackgroundResource(R.drawable.dialogbackground);
		       allview.setPadding(pad,pad,pad,pad);
	       	}

		act.addContentView(allview, new ViewGroup.LayoutParams(laypar,laypar));
	Runnable closerun=()-> {
		allview.setVisibility(GONE);
		removeContentView(allview);
		EnableControls(settingsview,true);
		};
	act.setonback(closerun);
	cancel.setOnClickListener(v->  {
			act.poponback();
			closerun.run();
			});
	save.setOnClickListener(v-> {
			act.poponback();
			closerun.run();
			setNightUploader(url.getText().toString(),editsecret.getText().toString(),activebox.isChecked(),isWearable?false:v3box.isChecked());
			});
	
	}
 }
