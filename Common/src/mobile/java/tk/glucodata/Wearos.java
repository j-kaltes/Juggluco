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

import android.app.Application;
import android.content.ComponentName;
import android.content.Context;
import android.content.pm.PackageManager;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.Space;
import android.widget.Spinner;
import android.widget.Toast;

import com.google.android.gms.wearable.Node;

import java.util.ArrayList;

import static android.content.pm.PackageManager.COMPONENT_ENABLED_STATE_DISABLED;
import static android.content.pm.PackageManager.COMPONENT_ENABLED_STATE_ENABLED;
import static android.content.pm.PackageManager.DONT_KILL_APP;
import static android.view.View.INVISIBLE;
import static android.view.View.VISIBLE;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static tk.glucodata.Applic.isRelease;
import static tk.glucodata.Natives.setBlueMessage;
import static tk.glucodata.NumberView.avoidSpinnerDropdownFocus;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getcheckbox;
import static tk.glucodata.util.getlabel;


class Wearos {
static private final String LOG_ID="Wearos";
/*
static CheckBox getcheckbox(Context context, String label) {
	var check=new CheckBox(context);
	check.setText(label);
	return check;
	} */

static boolean MessageReceiverEnabled() {
	try{
	Application app= Applic.app;
	  PackageManager manage = app.getPackageManager();
	ComponentName scan= new ComponentName(app, "tk.glucodata.MessageReceiver");
	return manage.getComponentEnabledSetting(scan)!=COMPONENT_ENABLED_STATE_DISABLED;
	}
	catch (Throwable e) {

		Log.stack(LOG_ID,e);
	}
	return false;
	}
static public void setuseWearos(boolean value) {
	enableMessageReceiver(value);
//	return Natives.setuseWearos(value);
	}
static int nodenum=-1;
static	ArrayList<Node> nodeslist=null;
	static Spinner mkspinner(MainActivity context) {
	var spin=new Spinner(context);
	

	var send=tk.glucodata.MessageSender.getMessageSender();
	if(send!=null) {
		var nodes=send.getNodes();
		if(nodes!=null) {
			nodeslist=new ArrayList<>(nodes);
			var adap = new RangeAdapter<com.google.android.gms.wearable.Node>(nodeslist, context, node -> {
				if (node != null)
					return node.getDisplayName()+" - "+node.getId();
				return "Error";
			});
			spin.setAdapter(adap);
		}
		}
	else {
		return null;
		}

	spin.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
		@Override
		public  void onItemSelected (AdapterView<?> parent, View view, int position, long id) {
			Log.i(LOG_ID,"onItemSelected");
			try {
			if(nodeslist!=null&&nodeslist.size()>position) {
				nodenum=position;
				remake();
//				var node = nodeslist.get(position);
	//			showinfo(node);
				}
				}
			catch(Throwable e) {
				Log.stack(LOG_ID,e);
				}
		}
		@Override
		public  void onNothingSelected (AdapterView<?> parent) {
			nodenum=-1;
			remake();
		} });
	avoidSpinnerDropdownFocus(spin);
	return spin;
	}

static CheckBox direct=null;
static Button start=null;

static void remake() {
	int dirval;
	if(nodenum<0) {
		dirval=-1;
		}
	else {
		var node=nodeslist.get(nodenum);
		dirval=Natives.directsensorwatch(makenodename(node));
		}
	if(dirval<0)  {
		direct.setEnabled(false);
		}
	else  {
		direct.setEnabled(true);
		direct.setChecked(dirval!=0);
		}
	if(dirval==1) {
		start.setEnabled(false);
		}
	else
		start.setEnabled(true);
	}
static public void show(MainActivity context) {
	nodenum=-1;
	start=getbutton(context,R.string.initwatchapp);
	direct=getcheckbox(context, context.getString(R.string.directconnection),false);
	var Ok=getbutton(context,R.string.closename);
	var Help=getbutton(context,R.string.helpname);
	Help.setOnClickListener(v-> help.help(R.string.wearosinfo,context));
	var spin=mkspinner(context);
	if(spin==null) {
        	Toast.makeText(context, R.string.nowatchesfound , Toast.LENGTH_SHORT).show();
		return;
		}
	float density=GlucoseCurve.metrics.density;
	var off=(int)(density*10.0f);
	direct.setPadding(0,off,0,off);

	remake();
	var layout=new Layout(context,(l,w,h)-> {
		var width=GlucoseCurve.getwidth();
		var height=GlucoseCurve.getheight();
		if(width>w)
			l.setX((width-w)/2);
		if(height>h)
			l.setY((height-h)/2);
		return new int[] {w,h};
		}, new View[]{spin},new View[]{direct},new View[]{start},new View[]{Help,Ok} );
	int laypad=(int)(density*4.0);
	layout.setPadding(laypad*2,laypad*2,laypad*2,laypad);

	layout.setBackgroundColor( Applic.backgroundcolor);
	context.addContentView(layout, new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));
	Ok.setOnClickListener(v -> {
		direct=null;
		nodeslist=null;
		nodenum=-1;
		removeContentView(layout);
		context.hideSystemUI(); });
		start.setOnClickListener(v -> {
			if(nodenum>=0) {
				var sender=tk.glucodata.MessageSender.getMessageSender();

				if(sender!=null) {
					Log.i(LOG_ID,"Init watch app");
					var nod=nodeslist.get(nodenum);
					Natives.resetbylabel(makenodename(nod));
					sender.startActivity(nod);
					}
				}
		 });
	direct.setOnCheckedChangeListener(
			 (buttonView,  isChecked) -> {
			 	if(nodenum>=0) {
					var node=nodeslist.get(nodenum);
					var name=makenodename(node);
					byte[] netinfo=Natives.getmynetinfo(name,true,isChecked?1:-1);
					if(netinfo!=null) {
						var sender=tk.glucodata.MessageSender.getMessageSender();
						if(sender!=null) {
							sender.sendnetinfo(node,netinfo);
							sender.sendbluetooth(node,isChecked);
							var main=MainActivity.thisone;
							boolean here= !isChecked;
//							if(here) SensorBluetooth.nullKAuth=true;
							Applic.setbluetooth(main==null?Applic.app:main,here);
							return;
							}
						}
					}
				direct.setChecked(!isChecked);
				return;
				}
			 );



	context.setonback(()-> { 
		removeContentView(layout);
		context.hideSystemUI(); }
		);

	}
static String makenodename(Node node) {
	return node.getId();
	}
/*	
static String makenodename(Node node) {
	var id=node.getId();
	var name=node.getDisplayName();
	int len=Math.min(15-id.length()-1,name.length());
	return id+'-'+name.substring(0,len);
	}
	*/

static private void enableMessageReceiver(boolean val) {
	try{
	Application app= Applic.app;
	  PackageManager manage = app.getPackageManager();
//	ComponentName  scan= new ComponentName(app, "tk.glucodata.MessageReceiver");
	ComponentName  scan= new ComponentName(app, tk.glucodata.MessageReceiver.class);
	int com=val?COMPONENT_ENABLED_STATE_ENABLED:COMPONENT_ENABLED_STATE_DISABLED;
   	manage.setComponentEnabledSetting(scan,com , DONT_KILL_APP);
	}
	catch (Throwable e) {

		Log.stack(LOG_ID,e);
	}
	}
}



