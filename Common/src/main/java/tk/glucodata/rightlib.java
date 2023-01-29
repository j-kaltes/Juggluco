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

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.os.Build;

import java.io.File;
import java.util.concurrent.TimeUnit;

import static android.view.View.VISIBLE;
import static java.util.concurrent.TimeUnit.SECONDS;
import static android.os.Build.CPU_ABI;

class rightlib {
	static private final String LOG_ID="rightlib";

static public boolean waitForever(long timeout, TimeUnit unit, final Process process) throws InterruptedException
    {
	if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
	    	return process.waitFor(timeout,unit);
    }
else {
		long startTime = System.nanoTime();
		long rem = unit.toNanos(timeout);

		do {
			try {
				process.exitValue();
				return true;
			} catch (IllegalThreadStateException ex) {
				if (rem > 0)
					Thread.sleep(Math.min(TimeUnit.NANOSECONDS.toMillis(rem) + 1, 100));
			}
			rem = unit.toNanos(timeout) - (System.nanoTime() - startTime);
		} while (rem > 0);
		return false;
	}
    }

static int runonce(String[] args,String[] env) {
	Process test ;
		try {
		    Log.i(LOG_ID,env[0]+" "+args[0]+ " "+args[1]);
		    test=Runtime.getRuntime().exec(args,env,null);
	     } catch(Throwable e) {
			Log.stack(LOG_ID,e);
		     return 0;
		 }
	try {
//			if(waitForever(120, SECONDS,test)) {
			if(waitForever(60, SECONDS,test)) {
				int ret=0;
				if(((ret=test.exitValue())&0xD0)==0xD0) {
					return ret;
					}
				Log.i(LOG_ID,"returned "+ret);
			 }
			  else {
			  	Log.i(LOG_ID,"Time out, destroy");
			   test.destroy();
				return 0;
			 }

	} catch(Throwable e) {
			Log.stack(LOG_ID,e);
		    return 0;
		}
		return 0;
		}

	public	static int rightlib(Context context) {
		ApplicationInfo appinfo=context.getApplicationInfo();
		 String testname=appinfo.nativeLibraryDir+"/libnative.so";
 	String[] args={testname,context.getFilesDir().toString()};
	String[] env={"LD_LIBRARY_PATH="+appinfo.nativeLibraryDir};
		int ret=0;
		Log.i(LOG_ID,"debug");
		if((ret=runonce(args,env))!=0)
			return ret;
 	String[] args2={testname,context.getFilesDir().toString(),"again"};
 	Log.i(LOG_ID,"Try next run");
		return runonce(args2,env);
	    }
	public static int testrightlib(Context context, File cali) {
		int ret=rightlib(context);
		Log.i(LOG_ID,"rightlib "+ret);
		if(ret!=0) {
			try {
			if(!cali.setReadOnly()) {
				Log.i(LOG_ID,"setReadOnly failed");
				}
			} catch(Throwable e) {
				Log.stack(LOG_ID,"setReadonly",e);
				}
				
			help.sethelpbutton(VISIBLE);

			Natives.setnodebug(ret==208);
			return ret;
			}
		else {
			cali.delete();
			}
		return 0;
		}
}
