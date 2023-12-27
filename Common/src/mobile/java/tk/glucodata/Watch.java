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

import static android.view.View.INVISIBLE;
import static android.view.View.VISIBLE;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static tk.glucodata.Applic.mgdLmult;
import static tk.glucodata.Log.doLog;
import static tk.glucodata.MessageSender.initwearos;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getcheckbox;

import android.view.View;
import android.view.ViewGroup;

class Watch {
static private final boolean TestBridge=doLog;
static private float glucose=80f;
static private float trend=-5.2f;
static private final  String LOG_ID="Watch";
static public void show(MainActivity context) {
	/*
	var pens=getbutton(context,"try Pens");
	pens.setOnClickListener(v->{
 			if(addnovopen("A serial",3))
				Log.i(LOG_ID,"Add serial succeeded");
			else
				Log.i(LOG_ID,"Add serial failed");;

			}); */
       var notify=getcheckbox(context,context.getString(R.string.notify), Notify.alertwatch);
	
	notify.setOnCheckedChangeListener(
		(buttonView,  isChecked) ->  {
			Applic.app.setnotify(isChecked);
			});

       var separate=getcheckbox(context,context.getString(R.string.separate), Notify.alertseparate);
	separate.setOnCheckedChangeListener(
		(buttonView,  isChecked) ->  {
			Notify.alertseparate=isChecked;
			Natives.setSeparate(isChecked);
			});

	       var watchdrip=getcheckbox(context,"Watchdrip", SuperGattCallback.doWearInt);
		watchdrip.setOnCheckedChangeListener(
			(buttonView,  isChecked) ->  {
				Natives.setwatchdrip(isChecked);
				tk.glucodata.watchdrip.set(isChecked);
				});
       var gadget=getcheckbox(context,"GadgetBridge", SuperGattCallback.doGadgetbridge);
		gadget.setOnCheckedChangeListener(
			(buttonView,  isChecked) ->  {
				Natives.setgadgetbridge(isChecked);
				SuperGattCallback.doGadgetbridge=isChecked;
				});
	var test=TestBridge?getbutton(context,"Test"):null;
	View[] mibandrow;
	if(TestBridge) {
		test.setOnClickListener(v-> {
			int mgdl;
			trend += 0.6f;
			if (trend > 5f)
				trend = -5f;
			if(Applic.unit==1) {
				glucose += 0.6f;
				if (glucose > 28f)
					glucose = 2.2f;

			mgdl=(int)Math.round(glucose*mgdLmult);
			}
			else {
				glucose += 13f;
				if (glucose > 500f)
					glucose = 40f;
				mgdl=(int)glucose;

			}
				
				Gadgetbridge.sendglucose(""+glucose,mgdl,glucose,trend, System.currentTimeMillis());
				});

		mibandrow=new View[]{watchdrip,gadget,test};
		}
	else  {
		mibandrow=new View[]{watchdrip,gadget};
		}
	var usexdripserver=Natives.getusexdripwebserver();
	var server=getcheckbox(context,R.string.webserver,usexdripserver);
	server.setOnCheckedChangeListener( (buttonView,  isChecked) -> Natives.setusexdripwebserver(isChecked));
/*
	var local=getcheckbox(context,"local only",Natives.getXdripServerLocal( ));
	local.setOnCheckedChangeListener(
			 (buttonView,  isChecked) -> {
				Natives.setXdripServerLocal(isChecked);
			 });*/
	var serverconfig=getbutton(context,R.string.config);
	var usegarmin=Natives.getusegarmin();
	var kerfstok=getcheckbox(context,"Kerfstok",usegarmin); 
	var status=getbutton(context,R.string.status);
	status.setVisibility(usegarmin?VISIBLE:INVISIBLE);
	kerfstok.setOnCheckedChangeListener(
			 (buttonView,  isChecked) -> {
			 if(isChecked&&!usegarmin)
				Applic.app.numdata.reinit(context);
			status.setVisibility(isChecked?VISIBLE:INVISIBLE);
			 });
	var useWearos=Applic.useWearos();
	if(useWearos) {
		var sender=tk.glucodata.MessageSender.getMessageSender();
	        if(sender!=null) sender.finddevices();
 		Natives.networkpresent();
		}

	var wearbox=getcheckbox( context, "WearOS", useWearos);
	var wearossettings=getbutton(context,R.string.config);
	wearossettings.setVisibility(useWearos?VISIBLE:INVISIBLE);
	wearbox.setOnCheckedChangeListener(
			 (buttonView,  isChecked) -> {
			Wearos.setuseWearos(isChecked);
			 if(isChecked) {
			 	if(!useWearos) {
					initwearos(Applic.app);
					}
				else  {
					var sender=tk.glucodata.MessageSender.getMessageSender();
					if(sender!=null) sender.finddevices();
					}
				Natives.networkpresent();
				}
			wearossettings.setVisibility(isChecked?VISIBLE:INVISIBLE);
			 });


	var Ok=getbutton(context,R.string.ok);
	var Help=getbutton(context,R.string.helpname);
	Help.setOnClickListener(v-> help.help(R.string.watchinfo,context));

	var layout=new Layout(context,(l,w,h)-> {
		var width= GlucoseCurve.getwidth();
		if(width>w)
			l.setX((width-w)/2);
		return new int[] {w,h};
		},mibandrow,new View[] {notify,separate},new View[]{wearbox,wearossettings},new View[]{server,serverconfig},new View[]{kerfstok,status},new View[]{Help,Ok} );
	float density=GlucoseCurve.metrics.density;
	int laypad=(int)(density*4.0);
	layout.setPadding(laypad*2,laypad*2,laypad*2,laypad);

	layout.setBackgroundColor( Applic.backgroundcolor);
	context.addContentView(layout, new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));
	status.setOnClickListener(v->{
			new GarminStatus(context,Applic.app.numdata,layout);
			});
	wearossettings.setOnClickListener(v->{
			Wearos.show(context,layout);
			});
	Ok.setOnClickListener(
		v -> {
		   if(usegarmin!=kerfstok.isChecked()) {
		   	Natives.setusegarmin(!usegarmin);
			if(usegarmin) {
				Natives.sethasgarmin(false);
				Applic.app.numdata.stop();
				}
		   	}
		context.poponback();
		removeContentView(layout);
		context.hideSystemUI(); 
		if(Menus.on) {
			Menus.show(context);
			}
		});



	context.setonback(()-> { removeContentView(layout);
		context.hideSystemUI(); });

	serverconfig.setOnClickListener(v->{
			Nightscout.show(context,layout);
			});
	}
	

}



