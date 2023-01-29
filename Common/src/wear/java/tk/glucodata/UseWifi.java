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
/*      Fri Jan 27 15:32:56 CET 2023                                                 */


package tk.glucodata;

import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkRequest;

import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;

class UseWifi {
private static UseWifi thisone=null;
	final static private String LOG_ID="UseWifi";
private	final ConnectivityManager connectivityManager = (ConnectivityManager) Applic.app.getSystemService(Context.CONNECTIVITY_SERVICE);
private ScheduledFuture<?> wifinotused=null;
private void disusewifi() {
	Log.i(LOG_ID,"disusewifi");
	connectivityManager.bindProcessToNetwork(null);
	connectivityManager.unregisterNetworkCallback(WiFiCallback);
	}
private	final ConnectivityManager.NetworkCallback WiFiCallback = new ConnectivityManager.NetworkCallback() {
		public void onAvailable(Network network) {
			super.onAvailable(network);
   			Natives.resetnetwork();
			Log.i(LOG_ID," WiFiCallback onAvailable(network) ");
			if(wifinotused!=null) {
				wifinotused.cancel(false);
				wifinotused=null;
				}
			connectivityManager.bindProcessToNetwork(network);
   			Applic.wakemirrors();
//			Applic.scheduler.schedule(()-> { disusewifi(); }, 120, TimeUnit.MINUTES);
		}

		public void onLost(Network network) {
			super.onLost(network);
			Log.i(LOG_ID," WiFiCallback onLost(network) ");
		}
	};


private void wifinotstarted() {
	wifinotused=null;
	Log.i(LOG_ID,"start network settings");
	var main=MainActivity.thisone;
	if( main!=null) {
		main.startActivity(new Intent("com.google.android.clockwork.settings.connectivity.wifi.ADD_NETWORK_SETTINGS"));
		}
	else
		wifinotused=Applic.scheduler.schedule(()-> { wifinotstarted(); }, 4, TimeUnit.SECONDS);
	}
private void startusewifi() {
	Log.i(LOG_ID,"usewifi");
	if(wifinotused==null) {
		try {
			connectivityManager.requestNetwork(new NetworkRequest.Builder().addTransportType(NetworkCapabilities.TRANSPORT_WIFI).build(), WiFiCallback);
			wifinotused=Applic.scheduler.schedule(()-> { wifinotstarted(); }, 4, TimeUnit.SECONDS);
			}
		catch(Throwable th) {
			Log.stack("wifinotstarted",th);
			}
		}
	};
static void usewifi() {
	thisone=new UseWifi();
	thisone.startusewifi();
	}
	/*
static boolean usingWifi() {
	return thisone!=null;
	} */
static void stopusewifi() {
	if(thisone!=null) {
		thisone.disusewifi();
		thisone=null;
			}
		}
	}
