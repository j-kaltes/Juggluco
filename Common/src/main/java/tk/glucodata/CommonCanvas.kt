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

		@JvmStatic
		public fun drawarrow(
			canvas: Canvas,
			paint: Paint,
			density: Float,
			ratein: Float,
			getx: Float,
			gety: Float
		): Boolean {
			if (!ratein.isNaN()) {
				val rate = Natives.thresholdchange(ratein);
				val x1: Double = (getx - density * 40.0)
				val y1: Double = (gety + rate * density * 30.0)
				paintArrow(canvas, paint, density, rate, x1, y1, getx.toDouble(), gety.toDouble());
				return true
			}
			return false
		}
		/*
	@JvmStatic
public fun drawarrow(canvas: Canvas,paint:Paint,density:Float, ratein:Float, getx:Float, gety:Float):Boolean {
	if(!ratein.isNaN()) {
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
	*/

private fun paintArrow( canvas: Canvas, paint: Paint, density: Float, rate: Float, x1: Double, y1: Double, getx: Double, gety: Double) {

		var rx: Double = getx - x1
		var ry: Double = gety - y1
		val rlen = sqrt(rx.pow(2.0) + ry.pow(2.0))
		rx /= rlen
		ry /= rlen

		val l: Double = density * 12.0;

		val addx = l * rx;
		val addy = l * ry;
		val tx1 = getx - 2 * addx;
		val ty1 = gety - 2 * addy;
		val xtus: Float = (getx - 1.5 * addx).toFloat();
		val ytus: Float = (gety - 1.5 * addy).toFloat();
		val hx = ry;
		val hy = -rx;
		val sx1: Float = (tx1 + l * hx).toFloat();
		val sy1: Float = (ty1 + l * hy).toFloat();
		val sx2: Float = (tx1 - l * hx).toFloat();
		val sy2: Float = (ty1 - l * hy).toFloat();
		paint.strokeWidth = density.toFloat() * 5.0f
		canvas.drawLine(x1.toFloat(), y1.toFloat(), xtus, ytus, paint)
		canvas.drawPath(Path().apply {
			moveTo(sx1, sy1);
			lineTo(getx.toFloat(), gety.toFloat());
			lineTo(sx2, sy2);
			lineTo(xtus, ytus);
			close()
		}, paint)
	}

		//{{x -> h - (4 h)/Sqrt[16 + 9 rate^2], y -> h + (3 h rate)/Sqrt[16 + 9 rate^2]}, {x -> h + (4 h)/Sqrt[16 + 9 rate^2], y -> h - (3 h rate)/Sqrt[16 + 9 rate^2]}}

	@JvmStatic public fun testcircle( canvas: Canvas, paint: Paint,density:Float) {
		val height = canvas.height.toDouble(); //assume square
		val half = height * .5f
/*		paint.setStyle(Paint.Style.STROKE)
		paint.setStrokeWidth(height*.05f);
		canvas.drawCircle(half,half,half, paint); */
		val rate=0.0f;
		paintArrow(canvas, paint, density, rate, 0.0, half, height, half);
		}

/*{{x -> -((2 h w)/Sqrt[16 h^2 + 9 rate^2 w^2]), 
  y -> -((3 h rate w)/(2 Sqrt[16 h^2 + 9 rate^2 w^2]))}, {x -> (
   2 h w)/Sqrt[16 h^2 + 9 rate^2 w^2], 
  y -> (3 h rate w)/(2 Sqrt[16 h^2 + 9 rate^2 w^2])}} */

		@JvmStatic
		public fun drawarrowcircle(
			canvas: Canvas,
			paint: Paint,
			density: Float,
			ratein: Float
		): Boolean {
			if (!ratein.isNaN()) {
				val height: Double = canvas.height.toDouble();
				val width: Double = canvas.width.toDouble()
				val rate = Natives.thresholdchange(ratein)
//				val common = 1.0 / sqrt(16.0 + 9.0 * rate.pow(2));
				val common =  width*height/sqrt(16*height.pow(2.0)+9 * rate.pow(2)*width.pow(2))
				val xcom = (2.0 * common);
				val halfW = width * .5;
				val x1 = halfW - xcom;
				val x2 = halfW + xcom;
				val ycom = (1.5 * rate * common);
				val halfH = height * .5;
				val y1 = halfH + ycom;
				val y2 = halfH - ycom;
				paintArrow(canvas, paint, density, rate, x1, y1, x2, y2);
				return true
			}
			return false
		}
	}
}
