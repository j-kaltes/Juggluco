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
/*      Thu Apr 04 20:10:13 CEST 2024                                                 */


package tk.glucodata;


import static com.google.zxing.integration.android.IntentIntegrator.DATA_MATRIX;
import static com.google.zxing.integration.android.IntentIntegrator.QR_CODE;
import static tk.glucodata.Applic.Toaster;
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.MainActivity.REQUEST_BARCODE;

import android.app.Activity;
import android.content.Intent;
import android.widget.Toast;

//import com.google.mlkit.vision.barcode.BarcodeScannerOptions;
//import com.google.mlkit.vision.barcode.common.Barcode;
//import com.google.mlkit.vision.barcode.common.Barcode;
//import com.google.mlkit.vision.barcode.common.Barcode;
import com.google.zxing.integration.android.IntentIntegrator;
import com.google.zxing.integration.android.IntentResult;

import java.util.Locale;
//import com.google.mlkit.vision.codescanner.GmsBarcodeScannerOptions;
//import com.google.mlkit.vision.codescanner.GmsBarcodeScanning;

public class Sibionics {
private	static final boolean bundled=true;
private static final String LOG_ID="Sibionics";


private static void 	wrongtag() {
	Toaster("Wrong QR code") ;
	}




/*
E007-0M0063KNUJ0....
      LT2309GEPD   
802JPPLT2309GEPD

GEPD802J
31108 GEPD802J PP7
Sensorname
sensorgegs
int len=sensorgegs.size();
startpos=len-19

longserialnumber 


(01) 06972831640165
(11) 231209
(17) 241208
(10) LT41 231108 C

(21) 231108 GEPD802J PP76

0106972831640165112312091724120810LT41231108C21231108GEPD802JPP76 
*/
//LT2309GEPD
  private static boolean    connectdevice(String scantag) {
     if(!isWearable) {
            String name=Natives.addSIscangetName(scantag);
            if(name!=null)  {
               SensorBluetooth.resetDevice(name);
               return true;
               }
               }
          return false;
         }


static void connectSensor(final String scantag) {
     if(!isWearable) {
        if(!connectdevice(scantag)) {
           	wrongtag(); 
        }
        }
    }

private static void scanZXing(Activity act) {
     if(!isWearable) {
		IntentIntegrator intentIntegrator = new IntentIntegrator(act);
		intentIntegrator.setPrompt("To use a Sibionic sensor, scan its Data Matrix or QR Code");
		intentIntegrator.setOrientationLocked(true); 
		intentIntegrator.setDesiredBarcodeFormats( DATA_MATRIX, QR_CODE);
		intentIntegrator.setRequestCode(REQUEST_BARCODE);
		intentIntegrator.initiateScan(); 
      }
      }
static void zXingResult(int requestCode, int resultCode, Intent data) {
       IntentResult intentResult = IntentIntegrator.parseActivityResult(requestCode, resultCode, data);
       if (intentResult != null) {
          final var scan=intentResult.getContents();
          if ( scan== null) Toaster( "Cancelled");
           else {
                Log.i(LOG_ID,"Scan: "+scan);
                connectSensor(scan);
             }
          }
       }
       


public static void scan(Activity act) {
     if(!isWearable) {
	  final var country= Locale.getDefault().getCountry();
      if("CN".equalsIgnoreCase(country))
            scanZXing(act);
       else
         scanGoogle(act);
         }
      }
private static void scanGoogle(Activity act) {
     if(!isWearable) {
	final var options =  new com.google.mlkit.vision.codescanner.GmsBarcodeScannerOptions.Builder().setBarcodeFormats( com.google.mlkit.vision.barcode.common.Barcode.FORMAT_DATA_MATRIX, com.google.mlkit.vision.barcode.common.Barcode.FORMAT_QR_CODE).build();
	final var scanner =  com.google.mlkit.vision.codescanner.GmsBarcodeScanning.getClient(act, options);
	scanner.startScan().addOnSuccessListener(
	       barcode -> {
	       var rawValue = barcode.getRawValue();
		var message="Scanned: "+rawValue;
		   Log.i(LOG_ID,message);
		   connectSensor(rawValue);
	       })
	   .addOnCanceledListener(
	       () -> {
		var message="Scan canceled";

		Log.i(LOG_ID,message);
		Toast.makeText(act, message, Toast.LENGTH_LONG).show();
		 // Task canceled
	       })
	   .addOnFailureListener(
	       e -> {
            var message=e.getMessage();
            Log.i(LOG_ID,message);
            Toast.makeText(act, message, Toast.LENGTH_SHORT).show();
            Toast.makeText(act, "Move to zXing", Toast.LENGTH_SHORT).show();
            scanZXing(act);
        
		 // Task failed with an exception
	       });

   }
	}

};
