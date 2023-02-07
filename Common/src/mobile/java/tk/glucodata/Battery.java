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
import android.content.Intent;
import android.graphics.Color;
import android.os.Build;
import android.os.PowerManager;
import android.text.method.LinkMovementMethod;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

//import com.judemanutd.autostarter.AutoStartPermissionHelper;

import static android.text.Html.TO_HTML_PARAGRAPH_LINES_CONSECUTIVE;
import static android.text.Html.fromHtml;
import static android.view.View.GONE;
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.MainActivity.IGNORE_BATTERY_OPTIMIZATION_SETTINGS;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.util.getbutton;

class Battery {
static private final String LOG_ID="Battery";
	@SuppressWarnings("deprecation")
static void sethtml(TextView view, String text) {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
        view.setText(fromHtml(text,TO_HTML_PARAGRAPH_LINES_CONSECUTIVE));
    }
    else view.setText(fromHtml(text));
      view.setTextIsSelectable(true);
           view.setTextColor(Color.WHITE);


	view.setLinksClickable(true);
	view.setMovementMethod(LinkMovementMethod.getInstance());
    }

static void sethtml(TextView view,int res) {
	sethtml(view,view.getContext().getString(res));
	}


/*
static void askdevicespecific(Context context) {
		final PowerManager pm = (PowerManager)context.getSystemService(Activity.POWER_SERVICE);
		var name=context.getPackageName();
		if(pm.isIgnoringBatteryOptimizations(name)) {
			} 
		AutoStartPermissionHelper.Companion.getInstance().getAutoStartPermission(context,true,true);
		}
		*/
static void batteryscreen(MainActivity context ) {
if(!isWearable) {
	if(android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.M) {
		Button Close=getbutton(context,R.string.closename);
		TextView info=new TextView(context);

		  final CheckBox battery = new CheckBox(context);
		  /*
		  Button dspecific = new Button(context);
		  try {
			  if( AutoStartPermissionHelper.Companion.getInstance().isAutoStartPermissionAvailable(context, false)) {
				 dspecific.setText(R.string.devicespecific);
				 dspecific.setOnClickListener( v-> {
				 		try {
							AutoStartPermissionHelper.Companion.getInstance().getAutoStartPermission(context,true,true);
							}

						 catch(Throwable e) {
							Log.stack(LOG_ID,e);
							Toast.makeText(context, "AutoStartPermissionHelper generated exception", Toast.LENGTH_LONG).show();
						  }

						});
				}
			else {
		//		deviceinfo.setVisibility(GONE);
				dspecific.setVisibility(GONE);
				}
		} catch(Throwable e) {
			Log.stack(LOG_ID,e);
	//		deviceinfo.setVisibility(GONE);
			dspecific.setVisibility(GONE);
		  }
		  */

		   int pad=(int)(GlucoseCurve.metrics.density*7.0);
		  // info.setPadding(pad,pad,pad,pad);
		   info.setPadding(pad,0,pad,0);

		sethtml(info, R.string.battery);


		battery.setText(R.string.dozeupdate);
		final PowerManager pm = (PowerManager)context.getSystemService(Activity.POWER_SERVICE);
		boolean isIgnoringBatteryOptimizations = pm.isIgnoringBatteryOptimizations(context.getPackageName());
		battery.setChecked(isIgnoringBatteryOptimizations);
		final Layout lay=new Layout(context, (l, w, h) -> new int[] {w,h}, new View[]{info},isWearable?new View[]{Close}:new View[]{battery,Close});
		Runnable closerun=()-> {
			lay.setVisibility(GONE);
			removeContentView(lay); 
			};
		context.setonback(closerun);
		battery.setOnClickListener(v-> {
			context.doonback();
			try{
				context.startActivityForResult(new Intent(android.provider.Settings.ACTION_IGNORE_BATTERY_OPTIMIZATION_SETTINGS),IGNORE_BATTERY_OPTIMIZATION_SETTINGS);
				} catch(Throwable th) {
					Log.stack(LOG_ID,"ACTION_IGNORE_BATTERY_OPTIMIZATION_SETTINGS",th);
					}
			});
		Close.setOnClickListener(v-> context.doonback());
	      lay.setBackgroundResource(R.drawable.dialogbackground);
		//context.addContentView(lay, new ViewGroup.LayoutParams((int)(GlucoseCurve.width*.6),ViewGroup.LayoutParams.WRAP_CONTENT));
		ScrollView scroll=new ScrollView(context);
	  	scroll.setFillViewport(true);

		scroll.addView(lay);
//		context.addContentView(scroll, new ViewGroup.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT,ViewGroup.LayoutParams.WRAP_CONTENT));
		context.addContentView(scroll, new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.MATCH_PARENT));
		}
		}
	}


}	
