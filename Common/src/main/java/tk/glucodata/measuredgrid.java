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

import android.content.Context;
import android.os.Build;
import android.util.AttributeSet;
import android.widget.GridLayout;

import androidx.annotation.RequiresApi;

public class measuredgrid extends GridLayout {


    public measuredgrid(Context context) {
        super(context);
    }

    public measuredgrid(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public measuredgrid(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    @RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
    public measuredgrid(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes) {
            super(context, attrs, defStyleAttr, defStyleRes);
    }
    private measured measure=null;
public void setmeasure(measured mea) {
	measure=mea;
	}
    @Override
    protected void onMeasure(int widthSpec, int heightSpec) {
        super.onMeasure(widthSpec,heightSpec);
     int height= getMeasuredHeight(); //also possible in callback
        int width= getMeasuredWidth();
	if(measure!=null) {
        measure.measure(this,width,height);
	/*
        int[] make=measure.measure(this,width,height);
      if(make!=null&&make.length==2)
         setMeasuredDimension(make[0],make[1]);
      else
         setMeasuredDimension(width,height);*/
      }

    }



};
