/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2(+), Libre 3(+), Dexcom G7/ONE+,        */
/*      Sibionics GS1Sb and GS3, Accu-Chek SmartGuide, CareSens Air and              */
/*      Aidex X sensors.                                                             */
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
/*      Thu Oct 15 12:51:27 CEST 2026                                                */

package tk.glucodata;

import static tk.glucodata.Applic.Toaster;
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.Applic.useZXing;
import static tk.glucodata.Log.doLog;
import static tk.glucodata.MainActivity.REQUEST_BARCODE;
import static tk.glucodata.PhotoScan.connectSensor;
import static tk.glucodata.watchdrip.tostring;

import static com.google.zxing.integration.android.IntentIntegrator.DATA_MATRIX;
import static com.google.zxing.integration.android.IntentIntegrator.QR_CODE;

import com.google.zxing.integration.android.IntentIntegrator;
import com.google.zxing.integration.android.IntentResult;
import android.app.Activity;
import android.content.Intent;

class ZXing {



    final static String LOG_ID="ZXing";
    static long wassensorptr=0L;
    static void scanZXingAlg(Activity act,int type,long sensorptr) {
         if(!isWearable&&useZXing) {
//             IntentIntegrator intentIntegrator = new IntentIntegrator(act);

                IntentIntegrator intentIntegrator = new IntentIntegrator(act);
                intentIntegrator.setCaptureActivity(MyCaptureActivity.class);

             intentIntegrator.setPrompt(Applic.app.getString(R.string.photomessage));
             intentIntegrator.setOrientationLocked(false); 
             intentIntegrator.setDesiredBarcodeFormats( DATA_MATRIX, QR_CODE);
             intentIntegrator.setRequestCode(type);
//             intentIntegrator.addExtra("sensorptr",sensorptr); //does
             wassensorptr=sensorptr;
             intentIntegrator.addExtra(com.google.zxing.client.android.Intents.Scan.SCAN_TYPE, com.google.zxing.client.android.Intents.Scan.MIXED_SCAN);
             intentIntegrator.initiateScan(); 
             }
          }




//    static void scanZXing(Activity act,int type) { scanZXingAlg(Activity act,REQUEST_BARCODE); }
    static void zXingResult(int resultCode, Intent data,MainActivity act,int type) {
         if(!isWearable&&useZXing) {
                try {
            //      var sensorptr=data.getLongExtra("sensorptr",0L); //is not there
                   long sensorptr=wassensorptr;
                   wassensorptr=0L;
                   Log.i(LOG_ID,"zXingResult(" +resultCode+",data) sensorptr="+sensorptr);
                   IntentResult intentResult = IntentIntegrator.parseActivityResult(resultCode, data);
                   if(intentResult != null) {
                      final var scan=intentResult.getContents();
                      if (scan== null) 
                        Toaster( "Cancelled");
                       else {
                            Log.i(LOG_ID,"Scan: "+scan);
                            Toaster(scan);
                            connectSensor(scan,act,type,sensorptr);
                            return;
                         }
                      }
                     else {
                        Log.i(LOG_ID,"intentResult == null"); 
                        }
                  if(sensorptr!=0L) 
                      PhotoScan.transmitterScanCancelled(sensorptr);
                   }
            catch(Throwable th) {
                Log.stack(LOG_ID,"zXingResult",th);
                }
           return;
           }
         }
    }
