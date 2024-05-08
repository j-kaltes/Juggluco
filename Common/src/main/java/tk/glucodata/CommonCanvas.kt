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


package tk.glucodata

import android.graphics.Canvas
import android.graphics.Color.*
import android.graphics.Paint
import android.graphics.Paint.Align
import android.graphics.Path
import android.graphics.Rect
import kotlin.math.pow
import kotlin.math.sqrt

    

    

public class CommonCanvas {

    companion object {
        private const val LOG_ID = "CommonCanvas"
//	@JvmStatic
// inline fun glnearnull(rate:Float):Boolean = (rate<.8f&&rate>-.8f)

	@JvmStatic
public fun drawarrow(canvas: Canvas,paint:Paint,density:Float, ratein:Float, getx:Float, gety:Float):Boolean {
	if(!ratein.isNaN()) {
//		val rate= if(glnearnull(ratein)) .0f else ratein
		val rate= Natives.thresholdchange(ratein);
		val x1:Double= (getx-density*40.0)
		val y1:Double= (gety+rate*density*30.0)

		var rx: Double=getx-x1
		var ry:Double=gety-y1
		val rlen= sqrt(rx.pow(2.0) + ry.pow(2.0))
		 rx/=rlen
		 ry/=rlen

		val l:Double=density*12.0;

		val addx= l* rx;
		val addy= l* ry;
		val tx1=getx-2*addx;
		val ty1=gety-2*addy;
		val xtus:Float= (getx-1.5*addx).toFloat();
		val ytus:Float= (gety-1.5*addy).toFloat();
		val hx=ry;
		val hy=-rx;
		val sx1:Float= (tx1+l*hx).toFloat();
		val sy1:Float= (ty1+l*hy).toFloat();
		val sx2:Float = (tx1-l*hx).toFloat();
		val sy2:Float= (ty1-l*hy).toFloat();
              paint.strokeWidth = density.toFloat()*5.0f
	    canvas.drawLine(x1.toFloat(), y1.toFloat(), xtus, ytus,paint)
		canvas.drawPath(Path().apply {
			moveTo(sx1,sy1) ;
			lineTo(getx,gety);
			lineTo(sx2,sy2);
			lineTo( xtus,ytus);
            		close()
			},paint)
           return true
		}
    return false
	}

    }
}
