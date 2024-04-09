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


import static tk.glucodata.Applic.Toaster;

import android.app.Activity;
import android.widget.Toast;

import com.google.mlkit.vision.barcode.common.Barcode;
import com.google.mlkit.vision.codescanner.GmsBarcodeScannerOptions;
import com.google.mlkit.vision.codescanner.GmsBarcodeScanning;

public class Sibionics {
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
  private static void    connectdevice(String scantag) {
         String name=Natives.addSIscangetName(scantag);
         SensorBluetooth.resetDevice(name);
            }
private static void connectSensor(final String scantag) {
        if (scantag.contains("0697283164")) {
/*		final var len=scantag.length() ;
		final var deviceName=scantag.substring(len - 12, len - 8);  //GEPD8
		final var after=scantag.substring(len - 8, len - 4); // 802J */
               connectdevice(scantag);
        } else {
           	wrongtag(); 
        }
    }


public static void scan(Activity act) {
	final var options = new GmsBarcodeScannerOptions.Builder().setBarcodeFormats( Barcode.FORMAT_DATA_MATRIX,Barcode.FORMAT_QR_CODE).build();
	var scanner = GmsBarcodeScanning.getClient(act, options);
	scanner.startScan().addOnSuccessListener(
	       barcode -> {
	       var rawValue = barcode.getRawValue();
		var message="Scanned: "+rawValue;
		   Log.i(LOG_ID,message);
//		   Toast.makeText(act, rawValue, Toast.LENGTH_LONG).show();
		   connectSensor(rawValue);
		 // Task completed successfully
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
		Toast.makeText(act, message, Toast.LENGTH_LONG).show();
		 // Task failed with an exception
	       });

	}

};
