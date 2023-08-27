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


package tk.glucodata.nums;

import android.app.AlertDialog;
import android.content.ComponentName;
import android.content.Context;
import android.content.pm.PackageManager;


import com.garmin.android.connectiq.ConnectIQ;

import tk.glucodata.Log;
import tk.glucodata.MainActivity;
import tk.glucodata.Natives;

import static android.content.pm.PackageManager.COMPONENT_ENABLED_STATE_DISABLED;
import static android.content.pm.PackageManager.COMPONENT_ENABLED_STATE_ENABLED;
import static android.content.pm.PackageManager.DONT_KILL_APP;

public class MyConnectIQListener implements ConnectIQ.ConnectIQListener {
    private  final String LOG_ID = "MyConnectIQListener";
	String init_error="";
	Context context;
	private boolean mSdkReady = false;
	public boolean sdkready() {
		return mSdkReady;
		}
	public MyConnectIQListener(Context context) {
		this.context=context;
		}

        @Override
        public void onInitializeError(ConnectIQ.IQSdkErrorStatus errStatus) {
	
	    setdata();
            init_error=tk.glucodata.R.string.garmin_initialization_error + errStatus.name();
            Log.v(LOG_ID, init_error);

            mSdkReady = false;
            String message;
            switch(errStatus) {
                case GCM_NOT_INSTALLED:
		boolean hasgarmin=Natives.gethasgarmin();
		Natives.sethasgarmin(false);
		if(hasgarmin) {
			message="Garmin Connect Mobile needs to be installed"; 
			break;
			}
		else
	    		Log.i(LOG_ID,"GCM_NOT_INSTALLED");
			return;
                case GCM_UPGRADE_NEEDED: message="Garmin Connect Mobile needs to be upgraded";break;
                case SERVICE_ERROR: message="Service Error";break;
                default: message="Some Error";
            };
	    Log.i(LOG_ID,message);
            AlertDialog.Builder dialog = new AlertDialog.Builder(context);
            dialog.setTitle(errStatus.name());
            dialog.setMessage(message);
            dialog.setPositiveButton(android.R.string.ok, null);
            dialog.show().setCanceledOnTouchOutside(false);
        }
	void setdata() {
		tk.glucodata.Applic app=(tk.glucodata.Applic)context.getApplicationContext();
		app.numdata.loadDevices(context); 
//	    if(context instanceof DeviceActivity) ((DeviceActivity)context).usedev();
	}
        @Override
        public void onSdkReady() {
	     Natives.sethasgarmin(true);
            Log.v(LOG_ID, "ConnectIQ Ready");
/*        	PackageManager manage = context.getPackageManager();
		manage.setComponentEnabledSetting(new ComponentName(context.getApplicationContext(), "tk.glucodata.garmin"), COMPONENT_ENABLED_STATE_ENABLED, DONT_KILL_APP);

 */
            mSdkReady = true;
	    setdata();
        }

        @Override
        public void onSdkShutDown() {
            Log.v(LOG_ID, "ConnectIQ ShutDown");
            mSdkReady = false;
        }

    };

