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
import android.content.res.Configuration;
import android.os.Build;

import androidx.core.graphics.Insets;
import androidx.core.view.WindowInsetsCompat;
import androidx.core.view.ViewCompat;

import static androidx.core.view.ViewCompat.setOnApplyWindowInsetsListener;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
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
static private boolean waitingForLandscape=false;
static private View currentView=null;
static private MainActivity owner=null;
static private View waitingDecor=null;
static private ViewTreeObserver.OnPreDrawListener waitingPreDraw=null;
static private int stableWidth=-1,stableHeight=-1,stableRotation=-1;
static private int stableLeft=-1,stableTop=-1,stableRight=-1,stableBottom=-1;
static private int stableFrames=0;
static private final String LOG_ID="Menus";

private static void resetStableWindow() {
    stableWidth=stableHeight=stableRotation=-1;
    stableLeft=stableTop=stableRight=stableBottom=-1;
    stableFrames=0;
    }

private static void cancelWindowWait() {
    if(waitingDecor!=null && waitingPreDraw!=null) {
        ViewTreeObserver obs=waitingDecor.getViewTreeObserver();
        if(obs.isAlive())
            obs.removeOnPreDrawListener(waitingPreDraw);
        }
    waitingDecor=null;
    waitingPreDraw=null;
    resetStableWindow();
    }

static void newTask() {
    cancelWindowWait();
    on=false;
    waitingForLandscape=false;
    currentView=null;
    owner=null;
    }

private static boolean stableMenuWindow(MainActivity act,View decor) {
    if(owner!=act || !on || act.isHandlingConfigurationChange()) {
        resetStableWindow();
        return false;
        }
    Configuration config=act.getResources().getConfiguration();
    int width=decor.getWidth();
    int height=decor.getHeight();
    final boolean requireLandscape=act.canForceMenuLandscape(config);
    if(width<=0 || height<=0 ||
       (requireLandscape &&
        (config.orientation!=Configuration.ORIENTATION_LANDSCAPE || width<=height))) {
        resetStableWindow();
        return false;
        }

    WindowInsetsCompat wi=ViewCompat.getRootWindowInsets(decor);
    if(wi==null) {
        resetStableWindow();
        return false;
        }
    Insets bars=wi.getInsets(WindowInsetsCompat.Type.systemBars());
    int rotation=act.getWindowManager().getDefaultDisplay().getRotation();

    if(width==stableWidth && height==stableHeight && rotation==stableRotation &&
       bars.left==stableLeft && bars.top==stableTop &&
       bars.right==stableRight && bars.bottom==stableBottom) {
        ++stableFrames;
        }
    else {
        stableWidth=width;
        stableHeight=height;
        stableRotation=rotation;
        stableLeft=bars.left;
        stableTop=bars.top;
        stableRight=bars.right;
        stableBottom=bars.bottom;
        stableFrames=1;
        }
    return stableFrames>=2;
    }

private static void waitForLandscapeWindow(MainActivity act) {
    waitingForLandscape=true;
    View decor=act.getWindow().getDecorView();
    if(waitingDecor==decor && waitingPreDraw!=null)
        return;

    cancelWindowWait();
    waitingDecor=decor;
    waitingPreDraw=new ViewTreeObserver.OnPreDrawListener() {
        @Override public boolean onPreDraw() {
            if(owner!=act || !on || currentView!=null && currentView.isAttachedToWindow()) {
                cancelWindowWait();
                return true;
                }
            Configuration config=act.getResources().getConfiguration();
            if(act.canForceMenuLandscape(config) &&
               config.orientation!=Configuration.ORIENTATION_LANDSCAPE)
                act.setMenuForcesLandscape(true);

            if(stableMenuWindow(act,decor)) {
                /* Remove the observer before adding the overlay.  Two equal
                 * pre-draws prove that the real window geometry and insets
                 * have reached a stable landscape state. */
                ViewTreeObserver obs=decor.getViewTreeObserver();
                if(obs.isAlive())
                    obs.removeOnPreDrawListener(this);
                waitingDecor=null;
                waitingPreDraw=null;
                waitingForLandscape=false;
                decor.post(() -> {
                    if(owner==act && on && !act.isHandlingConfigurationChange() &&
                       (currentView==null || !currentView.isAttachedToWindow()))
                        showNow(act);
                    });
                }
            else if(stableFrames>0) {
                /* A static screen is not guaranteed to get another frame.
                 * Explicitly schedule the second pre-draw needed by the
                 * stability test.  If geometry changes on that frame,
                 * stableLandscapeWindow() resets the candidate and another
                 * verification frame is scheduled. */
                decor.postOnAnimation(decor::invalidate);
                }
            return true;
            }
        };
    decor.getViewTreeObserver().addOnPreDrawListener(waitingPreDraw);
    ViewCompat.requestApplyInsets(decor);
    decor.invalidate();
    }

static void configurationChanged(MainActivity act, Configuration config) {
    /* Menus.on also remains true while a child screen is on top. Only a menu
     * explicitly waiting for its own reconstruction may take orientation
     * ownership here. */
    if(isWearable || owner!=act || !on || !waitingForLandscape)
        return;
    if(act.canForceMenuLandscape(config) &&
       config.orientation!=Configuration.ORIENTATION_LANDSCAPE)
        act.setMenuForcesLandscape(true);
    waitForLandscapeWindow(act);
    }

static public void show(MainActivity act) {
    on=true;

    if(owner!=act) {
        cancelWindowWait();
        owner=act;
        currentView=null;
        }

    if(!isWearable) {
        act.setMenuForcesLandscape(true);
        if(currentView!=null && currentView.isAttachedToWindow()) {
            waitingForLandscape=false;
            return;
            }
        waitForLandscapeWindow(act);
        return;
        }
    showNow(act);
    }

static private void showNow(MainActivity act) {
    waitingForLandscape=false;
    resetStableWindow();
    LayoutInflater flater= LayoutInflater.from(act);
    View view = flater.inflate(R.layout.menus, null, false);
    currentView=view;
      view.setLayoutDirection(View.LAYOUT_DIRECTION_LTR);
 //       view.setTextDirection(View.TEXT_DIRECTION_LTR);


	view.setAccessibilityDelegate(Layout.accessDeli);
        view.setBackgroundColor( Applic.backgroundcolor);

    if(!isWearable) {
        /* MainActivity enables edge-to-edge only on Android 11/API 30+.
         * On older Android versions the decor/content area already excludes
         * visible status/navigation bars. Padding with systemBars() there
         * would therefore count the navigation bar a second time. */
        if(Build.VERSION.SDK_INT >= 30) {
            setOnApplyWindowInsetsListener(view,(v,windowInsets) -> {
                Insets bars=windowInsets.getInsets(WindowInsetsCompat.Type.systemBars());
                v.setPadding(bars.left,bars.top*3/4,bars.right,bars.bottom);
                return windowInsets;
                });
            }
        view.addOnAttachStateChangeListener(new View.OnAttachStateChangeListener() {
            @Override
            public void onViewAttachedToWindow(View v) { }

            @Override
            public void onViewDetachedFromWindow(View v) {
                if(currentView==v)
                    currentView=null;

                /* Ignore callbacks belonging to an Activity that has already
                 * been replaced by another MainActivity instance. */
                if(owner!=act)
                    return;

                /*
                 * removeconfig() deliberately removes overlays. That is not
                 * the same as leaving Menus: keep ownership and rebuild after
                 * the new geometry has settled.
                 */
                if(act.isHandlingConfigurationChange()) {
                    if(on)
                        waitingForLandscape=true;
                    return;
                    }

                /*
                 * Ordinary detach means either Menus was closed or a child
                 * screen was opened. The child screen should follow the normal
                 * orientation policy until Menus.show() is called on return.
                 */
                act.setMenuForcesLandscape(false);
                }
            });
        }
      act.themeLightBars();
	act.setonback(() -> {
        if(owner!=act) {
            removeContentView(view);
            return;
            }
        if(act.isHandlingConfigurationChange()) {
            /*
             * doonback() is also used by removeconfig().  Preserve the fact
             * that Menus is logically open and recreate it after geometry has
             * settled instead of treating this as a real Back press.
             */
            waitingForLandscape=true;
            removeContentView(view);
            return;
            }
   		act.lightBars(!getInvertColors( ));
			   {if(doLog) {Log.i(LOG_ID,"onback");};};
			   on=false;
            waitingForLandscape=false;
            cancelWindowWait();
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
               waitingForLandscape=false;
               cancelWindowWait();

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
    if(!isWearable) {
        if(Build.VERSION.SDK_INT >= 30) {
            /* Edge-to-edge: the overlay occupies the whole window, so keep
             * its controls out of the system bars explicitly. */
            WindowInsetsCompat wi=ViewCompat.getRootWindowInsets(act.getWindow().getDecorView());
            if(wi!=null) {
                Insets bars=wi.getInsets(WindowInsetsCompat.Type.systemBars());
                view.setPadding(bars.left,bars.top*3/4,bars.right,bars.bottom);
                }
            }
        else {
            /* Pre edge-to-edge: Android has already inset the content view. */
            view.setPadding(0,0,0,0);
            }
        }
    else
        view.setPadding(MainActivity.systembarLeft,MainActivity.systembarTop*3/4,MainActivity.systembarRight,MainActivity.systembarBottom);

	act.addMyContentView(view, new ViewGroup.LayoutParams(MATCH_PARENT, MATCH_PARENT));
    if(!isWearable)
        view.requestApplyInsets();

    }


};



