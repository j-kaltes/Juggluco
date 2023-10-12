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
/*      Sun Oct 08 20:48:20 CEST 2023                                                */





package tk.glucodata;

import android.app.Activity;
import android.app.AlertDialog;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothManager;
import android.content.Context;
import android.content.DialogInterface;
import android.graphics.Paint;
import android.os.Build;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.HorizontalScrollView;
import android.widget.Spinner;
import android.widget.TextView;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.TimeUnit;

import androidx.annotation.Keep;

import static android.bluetooth.BluetoothGatt.CONNECTION_PRIORITY_BALANCED;
import static android.bluetooth.BluetoothGatt.CONNECTION_PRIORITY_HIGH;
import static android.graphics.Color.BLACK;
import static android.graphics.Color.BLUE;
import static android.graphics.Color.RED;
import static android.graphics.Color.WHITE;
import static android.graphics.Color.YELLOW;
import static android.view.View.GONE;
import static android.view.View.INVISIBLE;
import static android.view.View.VISIBLE;
import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.Natives.getshowscans;
import static tk.glucodata.Natives.getsystemui;
import static tk.glucodata.Natives.setsystemui;
import static tk.glucodata.NumberView.avoidSpinnerDropdownFocus;
import static tk.glucodata.NumberView.smallScreen;
import static tk.glucodata.help.help;
import static tk.glucodata.settings.Settings.removeContentView;

import tk.glucodata.settings.Settings;

public class Menus {
static public boolean on=false;
static private final String LOG_ID="Menus";
static public void show(MainActivity act) {
	on=true;
	LayoutInflater flater= LayoutInflater.from(act);
	View view = flater.inflate(R.layout.menus, null, false);
	view.setAccessibilityDelegate(Layout.accessDeli);
        view.setBackgroundColor( Applic.backgroundcolor);
	act.setonback(() -> {
			   Log.i(LOG_ID,"onback");
			   on=false;
			removeContentView(view);
				act.requestRender();
			});

    var menusviewview=view.findViewById(R.id.menusview);menusviewview.setOnClickListener(v ->{}); 
        CheckBox systemuiview=view.findViewById(R.id.systemui);
	systemuiview.setOnCheckedChangeListener( (buttonView,  isChecked)-> {
				setsystemui(isChecked);
			    	act.selectionSystemUI(); 
				});
	systemuiview.setChecked(getsystemui( ));
        var menusview=view.findViewById(R.id.menus);menusview.setOnClickListener(v ->{
				act.poponback();
			   on=false;
			removeContentView(view);
				act.requestRender();
		}); 
        var watchview=view.findViewById(R.id.watch);watchview.setOnClickListener(v ->{

				if(!isWearable) {
					removeContentView(view);
					tk.glucodata.Watch.show(act);
					}

	}); 
        var sensorview=view.findViewById(R.id.sensor);sensorview.setOnClickListener(v ->{

				removeContentView(view);
			       new bluediag(act);
		}); 
        var settingsview=view.findViewById(R.id.settings);settingsview.setOnClickListener(v ->{
					removeContentView(view);
					Settings.set(act);
	}); 
        var aboutview=view.findViewById(R.id.about);aboutview.setOnClickListener(v ->{
					if(!isWearable) {
						  var c=Applic.app.curve;
						  if(c!=null)
							c.doabout(act);
						}


	}); 
        var closeview=view.findViewById(R.id.close);closeview.setOnClickListener(v ->{
		act.moveTaskToBack(true);
	}); 
        var exportview=view.findViewById(R.id.export);exportview.setOnClickListener(v ->{
		var c=Applic.app.curve;
		  if(c!=null) {
			  Log.i(LOG_ID,"EXPORT");
		     removeContentView(view);
		     c.dialogs.showexport(act,c.getWidth(),c.getHeight()); 
		     }

	}); 
        var mirrorview=view.findViewById(R.id.mirror);mirrorview.setOnClickListener(v ->{
		     removeContentView(view);
			(new Backup()).mkbackupview(act);

	}); 
        var newamountview=view.findViewById(R.id.newamount);newamountview.setOnClickListener(v ->{
					if(Natives.staticnum()) {
        					help.help(R.string.staticnum,act);
						}
					else {
						var c=Applic.app.curve;
						if (c != null) {
							removeContentView(view);
							c.numberview.addnumberview(act);
							if (!smallScreen)
								c.showkeyboard(act);
						}
					}

	}); 
        var listview=view.findViewById(R.id.list);listview.setOnClickListener(v -> {
				var c = Applic.app.curve;
				if (c != null) {
					removeContentView(view);
					Natives.makenumbers();
					act.requestRender();
					c.getnumcontrol(act);
				}
			}
						);
        var statisticsview=view.findViewById(R.id.statistics);statisticsview.setOnClickListener(v ->{

			if(Natives.makepercentages()) {
				removeContentView(view);
				act.requestRender();
				Stats.mkstats(act);
				}
		}

			);
        var talkview=view.findViewById(R.id.talk);talkview.setOnClickListener(v ->{
		removeContentView(view);
		tk.glucodata.Talker.config(act);}); 
        CheckBox glucosefloatview=view.findViewById(R.id.glucosefloat);glucosefloatview.setOnCheckedChangeListener( (buttonView,  isChecked)->{
		Floating.setfloatglucose(act,isChecked);
	}); 
	glucosefloatview.setChecked(Natives.getfloatglucose());

        var lastscanview=view.findViewById(R.id.lastscan);lastscanview.setOnClickListener(v ->{
		if(Natives.showlastscan()) {
			removeContentView(view);
			act.requestRender();
			}
	}); 

        CheckBox scansview=view.findViewById(R.id.scans);scansview.setOnCheckedChangeListener( (buttonView,  isChecked)->{
		Natives.setshowscans(isChecked);
				act.requestRender();
		}); 
 	scansview.setChecked(getshowscans()) ;
        CheckBox streamview=view.findViewById(R.id.stream);streamview.setOnCheckedChangeListener( (buttonView,  isChecked)->{
		Natives.setshowstream(isChecked);
				act.requestRender();
		}); 
	streamview.setChecked(Natives.getshowstream() );

        CheckBox historyview=view.findViewById(R.id.history);historyview.setOnCheckedChangeListener( (buttonView,  isChecked)->{
		 Natives.setshowhistories(isChecked); 
				act.requestRender();
	}); 
	historyview.setChecked(Natives.getshowhistories()) ;
        CheckBox amountsview=view.findViewById(R.id.amounts);amountsview.setOnCheckedChangeListener( (buttonView,  isChecked)->{
	 Natives.setshownumbers(isChecked); 
				act.requestRender();

	}); 
	amountsview.setChecked(Natives. getshownumbers()) ;


        CheckBox mealsview=view.findViewById(R.id.meals);mealsview.setOnCheckedChangeListener( (buttonView,  isChecked)->{
		Natives.setshowmeals(isChecked);
				act.requestRender();
		}); 
 	mealsview.setChecked(Natives.getshowmeals()) ;



        CheckBox darkmodeview=view.findViewById(R.id.darkmode);darkmodeview.setOnCheckedChangeListener( (buttonView,  isChecked)->{

	Natives.setInvertColors(isChecked);
	});
	darkmodeview.setChecked( Natives.getInvertColors());
        var nowview=view.findViewById(R.id.now);nowview.setOnClickListener(v ->{

		removeContentView(view);
	Natives.settonow();
				act.requestRender();

	});
        var searchview=view.findViewById(R.id.search);searchview.setOnClickListener(v ->{
		  var c=Applic.app.curve;
		  if(c!=null) {
			removeContentView(view);
			c.startsearch();
			}
		}); 

        var dateview=view.findViewById(R.id.date);dateview.setOnClickListener(v ->{
		  var c=Applic.app.curve;
		  if(c!=null) {
			removeContentView(view);
			  c.startdatepick(Natives.getstarttime());
			  }
		
		}); 
        var daybackview=view.findViewById(R.id.dayback);daybackview.setOnClickListener(v ->{
			removeContentView(view);
		Natives.prevday(1);
				act.requestRender();
		}); 
        var daylaterview=view.findViewById(R.id.daylater);daylaterview.setOnClickListener(v ->{
			removeContentView(view);
		Natives.nextday(1);
				act.requestRender();
		}); 
        var weekbackview=view.findViewById(R.id.weekback);weekbackview.setOnClickListener(v ->{

			removeContentView(view);
		Natives.prevday(7);
				act.requestRender();
	}); 
        var weeklaterview=view.findViewById(R.id.weeklater);weeklaterview.setOnClickListener(v ->{

			removeContentView(view);
		Natives.nextday(7);
				act.requestRender();
		}); 

//	act.addContentView(view, new ViewGroup.LayoutParams( MATCH_PARENT,WRAP_CONTENT));
	act.addContentView(view, new ViewGroup.LayoutParams(MATCH_PARENT, MATCH_PARENT));

    }


};



