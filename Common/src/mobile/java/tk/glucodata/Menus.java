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

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CheckBox;

import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.Log.doLog;
import static tk.glucodata.MainActivity.REQUEST_BARCODE;
import static tk.glucodata.Natives.getInvertColors;
import static tk.glucodata.Natives.getshowscans;
import static tk.glucodata.Natives.getsystemui;
import static tk.glucodata.Natives.setsystemui;
import static tk.glucodata.NumberView.smallScreen;
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
      act.lightBars(false);
	act.setonback(() -> {
   		act.lightBars(!getInvertColors( ));
			   {if(doLog) {Log.i(LOG_ID,"onback");};};
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

   		act.lightBars(!getInvertColors( ));
			removeContentView(view);
				act.requestRender();
		}); 
        var watchview=view.findViewById(R.id.watch);watchview.setOnClickListener(v ->{

				if(!isWearable) {
   		   act.lightBars(!getInvertColors( ));
					removeContentView(view);
					tk.glucodata.Watch.show(act);
					}

	}); 
        var sensorview=view.findViewById(R.id.sensor);sensorview.setOnClickListener(v ->{

   		   act.lightBars(!getInvertColors( ));
				removeContentView(view);
			       bluediag.start(act);
		}); 
        var settingsview=view.findViewById(R.id.settings);settingsview.setOnClickListener(v ->{
					removeContentView(view);
					Settings.set(act);
	}); 
        Button aboutview=view.findViewById(R.id.about);
        if(tk.glucodata.BuildConfig.SiBionics==1) {
         aboutview.setText(R.string.photo);
        aboutview.setOnClickListener(v ->
              PhotoScan.scan(act,REQUEST_BARCODE));
            }
        else  {
           aboutview.setOnClickListener(v ->{
                       var c=Applic.app.curve;
                       if(c!=null)
                        c.doabout(act);
                  }); 
                 }
        var closeview=view.findViewById(R.id.close);closeview.setOnClickListener(v ->{
		act.moveTaskToBack(true);
	}); 
        var exportview=view.findViewById(R.id.export);exportview.setOnClickListener(v ->{
		var c=Applic.app.curve;
		  if(c!=null) {
			  {if(doLog) {Log.i(LOG_ID,"EXPORT");};};
   		   act.lightBars(!getInvertColors( ));
		     removeContentView(view);
		     c.dialogs.showexport(act,c.getWidth(),c.getHeight(),null); 
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
   		         act.lightBars(!getInvertColors( ));
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
   		         act.lightBars(!getInvertColors( ));
					removeContentView(view);
					Natives.makenumbers();
					act.requestRender();
					c.getnumcontrol(act);
				}
			}
						);
        var statisticsview=view.findViewById(R.id.statistics);statisticsview.setOnClickListener(v ->{

			if(Natives.makepercentages()) {
   		         act.lightBars(!getInvertColors( ));
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
               act.lightBars(!getInvertColors( ));
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

    CheckBox calibratedscansview=view.findViewById(R.id.calibratedscans);calibratedscansview.setOnCheckedChangeListener( (buttonView,  isChecked)->{
		Natives.setshowcalibratedscans(isChecked);
				act.requestRender();
		}); 
	calibratedscansview.setChecked(Natives.getshowcalibratedscans() );


    CheckBox calibratedstreamview=view.findViewById(R.id.calibratedstream);calibratedstreamview.setOnCheckedChangeListener( (buttonView,  isChecked)->{
		Natives.setshowcalibratedstream(isChecked);
				act.requestRender();
		}); 
	calibratedstreamview.setChecked(Natives.getshowcalibratedstream() );

     CheckBox calibratedhistoryview=view.findViewById(R.id.calibratedhistory);
     calibratedhistoryview.setOnCheckedChangeListener( (buttonView,  isChecked)->{
             Natives.setshowcalibratedhistories(isChecked); 
			act.requestRender();
	        }); 
	calibratedhistoryview.setChecked(Natives.getshowcalibratedhistories()) ;

     CheckBox historyview=view.findViewById(R.id.history);
     historyview.setOnCheckedChangeListener( (buttonView,  isChecked)->{
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
	darkmodeview.setChecked( getInvertColors());
        var nowview=view.findViewById(R.id.now);nowview.setOnClickListener(v ->{

               act.lightBars(!getInvertColors( ));
		removeContentView(view);
	Natives.settonow();
				act.requestRender();

	});
        var searchview=view.findViewById(R.id.search);searchview.setOnClickListener(v ->{
		  var c=Applic.app.curve;
		  if(c!=null) {
               act.lightBars(!getInvertColors( ));
			removeContentView(view);
			c.startsearch();
			}
		}); 

        var dateview=view.findViewById(R.id.date);dateview.setOnClickListener(v ->{
		  var c=Applic.app.curve;
		  if(c!=null) {
               act.lightBars(!getInvertColors( ));
			removeContentView(view);
			  c.startdatepick(Natives.getstarttime());
			  }
		
		}); 
        var daybackview=view.findViewById(R.id.dayback);daybackview.setOnClickListener(v ->{
               act.lightBars(!getInvertColors( ));
			removeContentView(view);
		Natives.prevday(1);
				act.requestRender();
		}); 
        var daylaterview=view.findViewById(R.id.daylater);daylaterview.setOnClickListener(v ->{
               act.lightBars(!getInvertColors( ));
			removeContentView(view);
		Natives.nextday(1);
				act.requestRender();
		}); 
        var weekbackview=view.findViewById(R.id.weekback);weekbackview.setOnClickListener(v ->{

               act.lightBars(!getInvertColors( ));
			removeContentView(view);
		Natives.prevday(7);
				act.requestRender();
	}); 
        var weeklaterview=view.findViewById(R.id.weeklater);weeklaterview.setOnClickListener(v ->{

               act.lightBars(!getInvertColors( ));
			removeContentView(view);
		Natives.nextday(7);
				act.requestRender();
		}); 
   
	  // view.setPadding(0,MainActivity.systembarTop,0,0);
  	view.setPadding(MainActivity.systembarLeft,MainActivity.systembarTop*3/4,MainActivity.systembarRight,MainActivity.systembarBottom);

	act.addContentView(view, new ViewGroup.LayoutParams(MATCH_PARENT, MATCH_PARENT));

    }


};



