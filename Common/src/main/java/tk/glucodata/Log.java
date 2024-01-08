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

import static tk.glucodata.Applic.usedlocale;
public class Log {
public static final boolean doLog=BuildConfig.doLog==1;
static final private String LOG_ID="Log";
private static void log(String type,String one,String two) {
   	if(!Applic.Nativesloaded)  {
		android.util.Log.e(one, two);
		return;
		}

	if(doLog) {
	 StringBuilder sb = new StringBuilder();
	  sb.append(type);
	  sb.append("/");
	  sb.append(one);
	  sb.append(" ");
	  sb.append(two);
	  sb.append("\n");
	  try {
	  	String str=sb.toString();
	  	Natives.log(str);
		}
	catch(Throwable e) {
		android.util.Log.e(LOG_ID, android.util.Log.getStackTraceString(e));
		android.util.Log.e(one, two);
		}
		}
	  }
public static void info(String str) {
	if(doLog) {
		if(Applic.Nativesloaded)  {
			Natives.log(str+'\n');
			}
		else
			android.util.Log.e(LOG_ID, str);
		}
	}
   public static void stack(String id,Throwable e) {
	stack(id,"",e);
   	}
public static String stackline(Throwable e) {
	String str=android.util.Log.getStackTraceString(e);
	final int len=str.length();
	if(len>100) {
		int end=str.indexOf("\n");
		if(end>=0&&end<len)
			return str.substring(0,end);
		}
	return str;
	}
public static String stackstring(String mess,Throwable e) {
	String str=android.util.Log.getStackTraceString(e);
	String all=str==null?mess+" Exception":mess+" "+str;
	return all;
	}
   public static void stack(String id,String mess,Throwable e) {
	String all=Log.stackstring(mess,e);
	if(!doLog||!Applic.Nativesloaded)
		android.util.Log.e(id,all);
	else
		log("E",id,all);

   	}
public static void e(String one,String two) {
	if(!doLog||!Applic.Nativesloaded)
		android.util.Log.e(one,two);
	else
		log("E",one,two);
		};
public static void format(String format, Object... args) {
	if(doLog) {
		String form=String.format(usedlocale,format,args);
		if(Applic.Nativesloaded) 
			Natives.log(form);
		else
			android.util.Log.i(LOG_ID,form);
		}
	}
//s/public static void \([a-z]\).*\(log.*\);/public static void \1(String one,String two) 	{ if(Applic.Nativesloaded) {\2 else android.util.Log.\1(one,two);};/g

public static void d(String one,String two) 	{ if(doLog) {if(Applic.Nativesloaded) {log("D",one,two);} else android.util.Log.d(one,two);}};
public static void w(String one,String two) 	{ if(doLog) {if(Applic.Nativesloaded) {log("W",one,two);} else android.util.Log.w(one,two);}};
public static void v(String one,String two) 	{ if(doLog) {if(Applic.Nativesloaded) {log("V",one,two);} else android.util.Log.v(one,two);}};
public static void i(String one,String two) 	{ if(doLog) {if(Applic.Nativesloaded) {log("I",one,two);} else android.util.Log.i(one,two);}};
public static void  showbytes(String mess,byte[] ar) {
	if(doLog)
		Natives.showbytes(mess,ar);
	}
};
