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
import android.text.InputType;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import static android.view.View.VISIBLE;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;

//import org.w3c.dom.Text;

class Dialogs {
float density;
ViewGroup exportscreen=null;
TextView exportlabel=null;
Dialogs(float density) {
	this.density=density;
	}
Button exportbutton(Activity activity,String label, int type) {
	Button but=new Button(activity);
	but.setText(label);
        but.setOnClickListener(
		v-> {
		if(type==4)
		    (( MainActivity)v.getContext()).algexporter(type,label,".html");
		   else
		    (( MainActivity)v.getContext()).exporter(type,label);
		    });
    return but;
	}


void showexport(MainActivity activity,int width,int height) {
	if(exportscreen==null) {
		Button num=exportbutton(activity,activity.getString(R.string.amountsname),0);
		Button scan=exportbutton(activity,activity.getString(R.string.scansname),1);
		Button stream=exportbutton(activity,activity.getString(R.string.streamname),2);
		Button hist=exportbutton(activity,activity.getString(R.string.historyname),3);
		Button meals=exportbutton(activity,activity.getString(R.string.mealsname),4);
		exportlabel=new TextView(activity);
//		exportlabel.setElegantTextHeight(true);
		exportlabel.setInputType(InputType.TYPE_TEXT_FLAG_MULTI_LINE);
		exportlabel.setSingleLine(false);
		final int rand=Math.round(5*density);
		exportlabel.setPadding(0,rand,0,rand);
		Button close=new Button(activity);
		close.setText(R.string.closename);
		close.setOnClickListener(v-> activity.doonback());
		View[] gviews={scan,hist,stream};
		View[] lviews={num,meals,close};
//		exportlabel.requestFocus();
		exportscreen=new Layout(activity, (lay, w, h) -> {
			if(w>=width||h>=height) {
				lay.setX(0);
				lay.setY(0);
				}
			else {
				lay.setX((width-w)/2); lay.setY((height-h)/2);
				}
			return new int[] {w,h};

		}, gviews,new View[]{exportlabel},lviews);
		exportscreen.setPadding(rand,rand,rand,rand);
		exportscreen.setBackgroundColor( Applic.backgroundcolor);
		activity.addContentView(exportscreen, new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));
//		exportlabel.requestFocus();
		}
	else {
        	exportscreen.setVisibility(VISIBLE);
		}
	exportlabel.setText(R.string.exporthelp);
	activity.setonback(() ->
        		exportscreen.setVisibility(View.GONE));
	}

	}
