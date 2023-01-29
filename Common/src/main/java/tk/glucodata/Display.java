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

import android.app.Activity;
import android.view.View;
import android.view.ViewGroup;

import tk.glucodata.Applic;
import tk.glucodata.Layout;

import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static tk.glucodata.Natives.getshowhistories;
import static tk.glucodata.Natives.getshownumbers;
import static tk.glucodata.Natives.getshowscans;
import static tk.glucodata.Natives.getshowstream;
import static tk.glucodata.RingTones.EnableControls;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getcheckbox;

public class Display {

public static void display(MainActivity act,View parent) {
	EnableControls(parent,false);
 	var Scans=getcheckbox(act,R.string.scansname,getshowscans()) ;
 	var History=getcheckbox(act,R.string.historyname,getshowhistories()) ;
 	var Stream=getcheckbox(act,R.string.streamname,getshowstream()) ;
 	var Amounts=getcheckbox(act,R.string.amountsname,getshownumbers()) ;

Scans.setOnCheckedChangeListener( (buttonView,  isChecked) -> { Natives.setshowscans(isChecked); });
	History.setOnCheckedChangeListener( (buttonView,  isChecked) -> { Natives.setshowhistories(isChecked); });
	Stream.setOnCheckedChangeListener( (buttonView,  isChecked) -> { Natives.setshowstream(isChecked); });
	Amounts.setOnCheckedChangeListener( (buttonView,  isChecked) -> { Natives.setshownumbers(isChecked); });

	var Close=getbutton(act,R.string.closename);
	Layout layout=new Layout(act, (l,w,h)->{
				var height=GlucoseCurve.getheight();
				var width=GlucoseCurve.getwidth();
				if(width>w) {
					l.setX((width - w) / 2);
				};
				if(height>h) {
					l.setY((height - h) / 2);
				}
			return new int[]{w,h}; },new View[]{Scans,History},new View[]{Stream,Amounts},new View[]{Close});
	layout.setBackgroundColor( Applic.backgroundcolor);
	int pads=(int)(GlucoseCurve.metrics.density*10);
        layout.setPadding(pads,pads,pads,pads);
        layout.setBackgroundResource(R.drawable.dialogbackground);
	act.addContentView(layout,new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));
	Close.setOnClickListener(v-> {
		act.doonback();
		});

	 act.setonback(() -> {
		removeContentView(layout);
		EnableControls(parent,true);
	 	});
	}


};
