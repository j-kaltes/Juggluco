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
/*      Sun Apr 16 20:59:10 CEST 2023                                                 */


package tk.glucodata;

import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static tk.glucodata.settings.Settings.removeContentView;

import android.annotation.SuppressLint;
import android.view.ViewGroup;
import android.widget.TextView;

public class Specific {
static void start(Object context) { }
static    void splash(Object act) { }
static void initScreen(Object obj) {}
static void settext(String str) {}
static void      rmlayout() {}

static void   blockedNum(MainActivity  act) {
			var width=GlucoseCurve.getwidth();
		help.basehelp(Applic.app.getString(R.string.staticnum),act,xzy->{ }, (l,w,h)-> {
			var height=GlucoseCurve.getheight();
			if(height>h)
				l.setY((height-h)/2);
			if(width>w)
				l.setX((width-w)/2);
			return new int[] {w,h};
			}, new ViewGroup.MarginLayoutParams((int)(width*0.85), WRAP_CONTENT));
		}

static public boolean useclose=false;
static public void setclose(boolean val) {
   useclose=val;
   }
static void wearnosensors(MainActivity act) {
    Switch.wearnosensors(act);
    }
static void installwatchface(Object context) { }

public static final class WatchFaceActivation {
    public static void onRequestPermissionsResult( Object obj, boolean granted) { }
    }
};
