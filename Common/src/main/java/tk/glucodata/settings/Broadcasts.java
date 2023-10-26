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
/*      Fri Jan 27 15:31:32 CET 2023                                                 */


package tk.glucodata.settings;

import static android.view.View.GONE;
import static android.view.View.INVISIBLE;
import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.MainActivity.getscreenheight;
import static tk.glucodata.MainActivity.getscreenwidth;
import static tk.glucodata.Natives.getJugglucobroadcast;
import static tk.glucodata.RingTones.EnableControls;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getcheckbox;
import static tk.glucodata.util.getlabel;

import android.content.Context;
import android.content.Intent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CheckBox;
import android.widget.ScrollView;
import android.widget.Toast;

import java.util.ArrayList;

import tk.glucodata.Applic;
import tk.glucodata.Consumer;
import tk.glucodata.JugglucoSend;
import tk.glucodata.Layout;
import tk.glucodata.MainActivity;
import tk.glucodata.Natives;
import tk.glucodata.R;
import tk.glucodata.SendLikexDrip;
import tk.glucodata.XInfuus;

public class Broadcasts {
static private  ArrayList<String> actionListeners(String action) {
        final Context context= Applic.app;
        Intent intent = new Intent(action);
   	var receivers = context.getPackageManager().queryBroadcastReceivers(intent, 0);
	ArrayList<String> names=new ArrayList<String>();
	for(var resolveInfo : receivers) {
	   	String name=resolveInfo.activityInfo.packageName;
		if(name!=null) {
			names.add(name);
		      }
	   	}
	return names;
	}


static private boolean isElement(String name,String[] strs) {
	if(strs==null)
		return false;
	for(var s:strs) {
		if(name.equals(s))
			return true;
		}
	return false;
	}


static private void  getselected(MainActivity context, View parent,String title,String[] selected, ArrayList<String> names, Consumer<String[]> saveproc) {

	int len= names.size();
	if(len==0) {
	//	if(!isWearable)	EnableControls(parent,true);
		Applic.argToaster(context, context.getString(R.string.noapplisteningto)+title, Toast.LENGTH_SHORT);
		String[] niets=new String[0];
		saveproc.accept(niets);
		return;
		}
	var views=new View[len+2][];
	final int startids=1;
	final int endlen=len+startids;
	views[0]=new View[]{getlabel(context,title)};
	for(int i=0;i<len;i++) {
		final var name=names.get(i);
		views[i+startids]=new View[]{getcheckbox(context,name,isElement(name,selected))};
		}
	var save=getbutton(context, R.string.save);

	var cancel=getbutton(context, R.string.cancel);
	views[endlen]=new View[]{cancel,save};
        var layout = new Layout(context,
                (l, w, h) -> {
			if(!isWearable) {
				var width=getscreenwidth(context);
				var height=getscreenheight(context);
				if(width>w)
					  l.setX(( width- w)/2);
				if(height>h)
					  l.setY(( height- h)/5);
					 }
			return new int[] {w,h};
                }, views);
	if(len==0)
		save.setVisibility(GONE);
	View theview;
	if(!isWearable) {
	   layout.setBackgroundResource(R.drawable.dialogbackground);
	   int pad=(int)(tk.glucodata.GlucoseCurve.metrics.density*5.0);
	   layout.setPadding(pad,pad,pad*2,pad);
	   theview=layout;
	   }
	  else {
	   	int pad=(int)(getscreenwidth(context)*.16);
		if(len>0) {
			cancel.setPadding(pad,0,0,0);
			save.setPadding(0,0,pad,0);
			}
        	layout.setBackgroundColor(Applic.backgroundcolor);
		final var scroll=new ScrollView(context);
		scroll.addView(layout);
		scroll.setFillViewport(true);
		scroll.setSmoothScrollingEnabled(false);
		scroll.setVerticalScrollBarEnabled(false);
		theview=scroll;
	  	}
	var laysize=isWearable?MATCH_PARENT:WRAP_CONTENT;
        context.addContentView(theview, new ViewGroup.LayoutParams(laysize,laysize));
	Runnable closerun=()-> {
		if(!isWearable)	EnableControls(parent,true);
		removeContentView(theview) ;
		saveproc.accept(null);
		};
	context.setonback(closerun);
	save.setOnClickListener(v->{
		if(!isWearable)	EnableControls(parent,true);
		int checked=0;
		for(int i=startids;i<endlen;i++) {
			CheckBox box=(CheckBox)views[i][0];
			if(box.isChecked())
				++checked;
			}
		int uititer=0;
		String[] newselect=new String[checked];
		for(int i=startids;i<endlen;i++) {
			CheckBox box=(CheckBox)views[i][0];
			if(box.isChecked()) {
				newselect[uititer++]=box.getText().toString();
				}
			}
		removeContentView(theview) ;
		context.poponback();
		saveproc.accept(newselect);
		});
	cancel.setOnClickListener(v->{
		context.doonback();
		});
	if(!isWearable)	
		EnableControls(parent,false);
	}

static public void setlibrereceivers(MainActivity context,View settingsview,CheckBox box,boolean[] dont) {
	var selected= Natives.librelinkRecepters();
	var all=actionListeners(XInfuus.glucoseaction);
	/*
	if(all.size()==0) {
		Applic.argToaster(context, context.getString(R.string.noapplisteningto)+XInfuus.glucoseaction, Toast.LENGTH_SHORT);
		Natives.setlibrelinkRecepters(null);
		XInfuus.setlibrenames();
		box.setChecked(false);
		dont[0]=false;
		return;
		} */
	getselected(context,settingsview,"Patched Libre",selected, all,newselected-> {
			if(newselected!=null) {
				Natives.setlibrelinkRecepters(newselected);
				XInfuus.setlibrenames();
				box.setChecked(newselected.length>0);
				}
			dont[0]=false;
			}
		);
	
	}
static public void setxdripreceivers(MainActivity context,View settingsview,CheckBox box,boolean[] dont) {
	var selected= Natives.xdripRecepters();
	var all=actionListeners(SendLikexDrip.ACTION);
 	getselected(context,settingsview,"xDrip",selected, all,newselected-> {
			if(newselected!=null) {
				Natives.setxdripRecepters(newselected);
				SendLikexDrip.setreceivers();
				box.setChecked(newselected.length>0);
				}
			dont[0]=false;
			}
		);
	}

static public void setglucodatareceivers(MainActivity context,View settingsview,CheckBox box,boolean[] dont) {
	var all=actionListeners(JugglucoSend.ACTION);
	var selected= Natives.glucodataRecepters();
 	getselected(context,settingsview,"Glucodata",selected, all,newselected-> {
			if(newselected!=null) {
				Natives.setglucodataRecepters(newselected);
				JugglucoSend.setreceivers();
				box.setChecked(newselected.length>0);
				}
			dont[0]=false;
			}
		);
	}
static public void updateall() {
	if(Natives.getxbroadcast()) {
		var xlis=actionListeners(SendLikexDrip.ACTION);
		Natives.setxdripRecepters(xlis.toArray(new String[xlis.size()]));
		}
	if(getJugglucobroadcast()) {
		var juglis=actionListeners(JugglucoSend.ACTION);
		Natives.setglucodataRecepters(juglis.toArray(new String[juglis.size()]));
		}
	}
}
