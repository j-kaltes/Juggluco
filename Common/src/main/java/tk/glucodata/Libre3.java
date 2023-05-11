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


import android.nfc.Tag;

import static tk.glucodata.BuildConfig.libreVersion;
import static tk.glucodata.Log.showbytes;

public class  Libre3 {
private static final String LOG_ID="Libre3";
public static byte[] firstnfc(Tag tag) {
	final byte[] firstcom={(byte)0x02,(byte)0xA1,(byte)0x7A};
         var res=AlgNfcV.wholenfccmd(tag,firstcom );
	 showbytes("NFC res: ",res);
	return res;
	 }

public static	long   	libre3NFC(Tag tag) {
	byte[] res = firstnfc(tag);
	if(res == null) {
		Log.i(LOG_ID,"firstnfc==null");
		return 2L;
		}
	if(libreVersion == 3) {
		long streamptr=tk.glucodata.libre3.NFC.second(res,tag);
//		SensorBluetooth.resetDevice(streamptr);
		Log.i(LOG_ID,"streamptr="+streamptr);
		return streamptr;
		}
	Log.i(LOG_ID,"libreVersion!=3");
	return 1L;
  	}

}
