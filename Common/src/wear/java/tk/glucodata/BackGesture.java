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
/*      Wed Feb 08 11:41:14 CET 2023                                                 */



package tk.glucodata;


import android.graphics.PointF;
import android.view.MotionEvent;
import android.view.View;

public class BackGesture implements View.OnTouchListener {
	private static final String LOG_ID="BackGesture";
private final MainActivity main;
private final float 	miny,maxy,minx,maxx;
BackGesture(MainActivity act) {
	int height=GlucoseCurve.getheight();
	miny=height*.1f;
	maxy=height*.9f;
	int width=GlucoseCurve.getwidth();
	minx=width*.1f;
	maxx=width*.9f;
	main=act;
	}
private boolean backed=false;
    public boolean onTouch(View view, MotionEvent event) {
        Log.i(LOG_ID,event.toString());
        switch (event.getAction() & MotionEvent.ACTION_MASK) {
		case MotionEvent.ACTION_BUTTON_PRESS:
            case MotionEvent.ACTION_DOWN:
	    	if(!backed) {
			final var x=event.getX();
			final var y=event.getY();
			if(x<minx||x>maxx||y<miny||y>maxy) {
				backed=true;
				main.doonback();
				}
			}
                break;

        	}
        return true; 
    }

}
