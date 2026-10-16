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
/*      Sat Oct 19 12:27:31 CEST 2024                                                 */


package tk.glucodata;

import static android.graphics.Typeface.BOLD;
import static android.graphics.Typeface.SANS_SERIF;
import static java.lang.String.format;


import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.Rect;
import android.graphics.Typeface;
import android.graphics.drawable.Icon;
import android.os.Build;
import tk.glucodata.Log;

public class StatusIcon {
final private static String LOG_ID="StatusIcon";
final static float mapwidth=48,mapheight=48;

final static float half=0.5f*mapwidth;
    private float fontsize;

    final private Bitmap iconBitmap;
    final private Canvas canvas;
    final private Paint iconPaint=new Paint();
//    final private Paint agePaint=new Paint();
StatusIcon() {
   fontsize=mapwidth*0.8f;
   iconPaint.setTextAlign(Paint.Align.CENTER);
    iconPaint.setTextSize(fontsize);
	iconPaint.setAntiAlias(true);
	iconBitmap = Bitmap.createBitmap((int)mapwidth, (int)mapheight, Bitmap.Config.ARGB_8888);
	canvas = new Canvas(iconBitmap);

	//{if(doLog) {Log.i(LOG_ID," mapwidth="+mapwidth+" mapheight="+mapheight+"color="+ format("%x",iconPaint.getColor()));};};
//   iconPaint.setTypeface(Typeface.defaultFromStyle(BOLD));
//      Typeface normaltype=Typeface.create("Arial",Typeface.NORMAL);
    Typeface basistype=
    (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)?
 Applic.app.getResources().getFont(R.font.opensans): SANS_SERIF;
//      ontFace=Typeface.createFromAsset(getAssets(),"font/comic.TTF"); Typeface face = Typeface.create(fontFace, Typeface.BOLD);
Typeface     normaltype;
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
        normaltype= Typeface.create(basistype,1000, false);
    }
    else {
        normaltype= Typeface.create(basistype,Typeface.BOLD);
       }
//var     normaltype= Typeface.create(basistype,Typeface.BOLD);
   iconPaint.setTypeface(normaltype);

 //  iconPaint.setColor(Color.BLACK);

//   canvas.drawColor(Color.WHITE);
   }
private float  drawcenter(String value) {
   	canvas.drawColor(Color.TRANSPARENT, PorterDuff.Mode.CLEAR);
        var bounds=new Rect();
        iconPaint.setTextSize(fontsize);
        iconPaint.getTextBounds(value, 0, value.length(),  bounds);
   float fsize;
   //{if(doLog) {Log.i(LOG_ID,"bounds: "+bounds);};};

        var h= bounds.bottom-bounds.top;
        var w=bounds.right-bounds.left;
        var scale=Math.max(h,w);
         fsize=1.011f*fontsize*mapwidth/scale;
         if(value.charAt(value.length()-1)=='1') {
             fsize *= 0.948f;
         }
        iconPaint.setTextSize(fsize);
        var des=iconPaint.descent();
        var as=iconPaint.ascent();
        //{if(doLog) {Log.i(LOG_ID,"descent()="+des+" ascent()="+as+" fontsize="+fontsize);};};
        var y = half - (des + as)*.5f-0.01f*fsize;
       canvas.drawText(value,0,value.length(),half-0.005f*fsize,y,iconPaint);
       return fsize;
   }
synchronized Icon getIcon(String value) {
       fontsize=drawcenter(value);
       return Icon.createWithBitmap(iconBitmap);
       }
        /*
Icon getIcon(float fl) {
   
       fontsize=drawcenter(value);
        return Icon.createWithBitmap(bitmap);
        } */


}
