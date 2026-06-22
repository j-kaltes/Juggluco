/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2, Libre 3, Dexcom G7/ONE+,              */
/*      Sibionics GS1Sb and Accu-Chek SmartGuide sensors.                            */
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
/*      Wed May 06 21:12:16 CEST 2026                                                */
package tk.glucodata;
import static java.net.HttpURLConnection.HTTP_OK;

import static tk.glucodata.Log.doLog;
import static tk.glucodata.Log.stackline;
import static tk.glucodata.NightPost.readJSONObject;

import java.io.BufferedOutputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.TimeZone;
import org.json.JSONException;
import org.json.JSONObject;
import static tk.glucodata.NightPost.nothing;

import android.os.Build;

public class GS3ID  {
static private final String LOG_ID="GS3ID";
static  String  GS3IDstatus=nothing;


private static String getSibAgent() {
      String brand= Build.BRAND;
      if(brand==null)
        brand="NULL";
      else {
        if("&".equals(brand))
            brand="";
        }
      String model= Build.MODEL;
      if(model==null)
        model="NULL";
      else {
        if("&".equals(model))
            model="";
        }
      String release= Build.VERSION.RELEASE;
      if(release==null)
        release="NULL";
    return "SIBIONICSGS3&01.08.00.00&android&android "+release+"&"+brand+" "+model;
    }

private static boolean getId(String urlbase,String auth) {
      try {
          final var urlstr=urlbase+"/user/app/info";
          URL url = new URL(urlstr);
          HttpURLConnection urlConnection = (HttpURLConnection) url.openConnection();
          urlConnection.setConnectTimeout(10000);
          urlConnection.setReadTimeout(60000);
          urlConnection.setRequestProperty("lang","en_US");
          urlConnection.setRequestProperty("log-header", "I am the log request header.");
          urlConnection.setRequestProperty("Authorization", auth);
          urlConnection.setRequestProperty("Sib-Agent", getSibAgent());
          urlConnection.setRequestProperty("timeZone",TimeZone.getDefault().getID());
          urlConnection.setRequestProperty("User-Agent","okhttp/3.12.10");
          urlConnection.setRequestProperty("Connection", "Keep-Alive");
          urlConnection.setRequestMethod("GET");
          final int code=urlConnection.getResponseCode();
          if(code==HTTP_OK) {
                JSONObject object =  readJSONObject(urlConnection) ;
                if(object.getBoolean("success")) {
                     JSONObject data=object.getJSONObject("data");
                     var idstr=data.getString("id");
                     long id = Long.parseLong(idstr);
                     var mess="id="+id;
                     Log.i(LOG_ID,mess);
                     Natives.saveGS3id(id);
                     GS3IDstatus="Success: "+mess;
                     return true;
                     }
                 else {
                     var msg=object.getString("msg");
                     GS3IDstatus=msg;
                     Log.e(LOG_ID, msg);
                     return false;
                    }
                }
          else {
             GS3IDstatus="getId: urlConnection.getResponseCode()="+code;
             return false;
             }
         }
     catch(Throwable th) {
         GS3IDstatus="getId:\t"+ stackline(th);
         Log.stack(LOG_ID, "getId",th);
         return false;
         }
    }


static boolean postgetauth(String login,String md5pass) {
  try {
      URL url = new URL("https://cgm.sibionics.io/center/app/user/login");
      HttpURLConnection urlConnection = (HttpURLConnection) url.openConnection();
      urlConnection.setRequestMethod("POST");
      urlConnection.setDoOutput(true);
      urlConnection.setRequestProperty("lang","en_US");
      urlConnection.setRequestProperty("Content-Type", "application/json; charset=utf-8");
      urlConnection.setRequestProperty("log-header", "I am the log request header.");
      urlConnection.setRequestProperty("Sib-Agent", getSibAgent());
      urlConnection.setRequestProperty("timeZone",TimeZone.getDefault().getID());
      urlConnection.setRequestProperty("User-Agent","okhttp/3.12.10");
      var getauthtext= "{\"email\":\""+login+"\",\"loginType\":0,\"password\":\""+md5pass+"\"}";
      {if(doLog) {Log.i(LOG_ID,"postauth: "+getauthtext);};};
      byte[] textbytes=getauthtext.getBytes();
      urlConnection.setRequestProperty( "Content-Length", Integer.toString( textbytes.length ));

      OutputStream outputPost = new BufferedOutputStream(urlConnection.getOutputStream());
      outputPost.write(textbytes);
      outputPost.flush();
      outputPost.close();
      final int code=urlConnection.getResponseCode();
      
      {if(doLog) {Log.i(LOG_ID,"ResponseCode="+code);};};
      if(code==HTTP_OK) {
         JSONObject object = readJSONObject(urlConnection);
         if(doLog) {Log.i(LOG_ID,"getauth Success");};
         if(object.getBoolean("success")) {
             JSONObject data=object.getJSONObject("data");
             var access=data.getString("access_token");
             if(doLog)
                 Log.i(LOG_ID,"access_token="+access);
             var host=data.getString("regionDomain");
             if(doLog)
                 Log.i(LOG_ID,"regionDomain="+host);
             if(host==null||host.length()<4) {
                   host="https://cgm-ce.sisensing.com";
                  }
             return getId(host,access);
             }
         else {
             var msg=object.getString("msg");
             GS3IDstatus=msg;
             Log.e(LOG_ID, msg);
             return false;
            }
         }
      else {
         GS3IDstatus="postgetauth: urlConnection.getResponseCode()="+code;
         return false;
         }
       }
      catch(Throwable th) {
         GS3IDstatus="postgetauth:\t"+ stackline(th);
         Log.stack(LOG_ID, "postgetauth",th);
         return false;
         }
   }
}
