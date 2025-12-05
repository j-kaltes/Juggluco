/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2, Libre 3, Dexcom G7/ONE+ and           */
/*      Sibionics GS1Sb sensors.                                                     */
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
/*      Thu Apr 17 22:49:22 CEST 2025                                                 */


package tk.glucodata;

import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static tk.glucodata.help.help;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.util.getbutton;

import android.graphics.Bitmap;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;

import com.google.zxing.BarcodeFormat;
import com.google.zxing.MultiFormatWriter;
import com.google.zxing.WriterException;
import com.google.zxing.common.BitMatrix;
import com.journeyapps.barcodescanner.BarcodeEncoder;

import tk.glucodata.GlucoseCurve;
import tk.glucodata.Layout;
import tk.glucodata.MainActivity;
import tk.glucodata.R;

/*
import androidx.compose.ui.graphics.ImageBitmap
import androidx.compose.ui.graphics.asImageBitmap
*/
public class QRmake {
static private final String LOG_ID="QRmake";
private  static Bitmap bitmap(String myStringToEncode,int width,int height) throws WriterException {
            MultiFormatWriter multiFormatWriter = new MultiFormatWriter();
            BitMatrix bitMatrix = multiFormatWriter.encode(myStringToEncode, BarcodeFormat.QR_CODE,width,height);
            BarcodeEncoder barcodeEncoder = new BarcodeEncoder();
            return barcodeEncoder.createBitmap(bitMatrix);
            }
/*
 public   static ImageBitmap imagebitmap(String myStringToEncode) throws WriterException {
        return bitmap(name).asImageBitmap();
        } */
public static void show(MainActivity act, String code) {
     var image=new ImageView(act);
     int height= GlucoseCurve.getheight()-MainActivity.systembarTop-MainActivity.systembarBottom;
     int width=GlucoseCurve.getwidth()-MainActivity.systembarLeft-MainActivity.systembarRight;
     int qrWidth=(int)(width*.7f);
     int qrHeight=Math.min(qrWidth,height);

//     int qrWidth=(int)(width*.7f);
     try {
         image.setImageBitmap(bitmap(code,qrHeight,qrHeight));
         }
     catch(Throwable th) {
        Log.stack(LOG_ID,"setImageBitmap",th);
        }
     var close=getbutton(act, R.string.closename);
     close.setOnClickListener(v -> {
        MainActivity.doonback();
        });
   final var help=getbutton(act,R.string.helpname);
   help.setOnClickListener(v-> help(R.string.QRmirror,act));
   var buttonlayout=new Layout(act,(x, w, h)->{
         return new int[] {w,h};
           },new View[]{help},new View[]{close});

     buttonlayout.setLayoutParams(new ViewGroup.LayoutParams(MATCH_PARENT , MATCH_PARENT));
     Layout.getMargins(close).bottomMargin=(int)(height*.35f);
     Layout.getMargins(help).topMargin=(int)(height*.15f);
     var layout=new Layout(act,(x, w, h)->{
         return new int[] {w,h};
           },new View[]{image,buttonlayout});
      layout.setBackgroundColor( Applic.backgroundcolor);
    layout.usebaseline=false;
      var hor=(int)((height-qrHeight)*.5f);
      var ver =(int)((width-qrWidth)*.4f);

      layout.setPadding(MainActivity.systembarLeft+ver,MainActivity.systembarTop+hor,MainActivity.systembarRight+ver,MainActivity.systembarBottom+hor);
      act.addContentView(layout, new ViewGroup.LayoutParams(MATCH_PARENT, MATCH_PARENT));
      MainActivity.setonback(()-> removeContentView(layout));
      }

   }
