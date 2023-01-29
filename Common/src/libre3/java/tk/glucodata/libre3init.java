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
/*      Fri Jan 27 15:26:08 CET 2023                                                 */


package tk.glucodata;


import static tk.glucodata.Applic.isRelease;
import static tk.glucodata.Natives.getlibreAccountIDnumber;
import static tk.glucodata.Natives.processbar;
import static tk.glucodata.Natives.processint;

class libre3init {
private static		final  String LOG_ID="libre3init";
static void init() {
/*
if(BuildConfig.doLog==1) {
Log.i(LOG_ID,"testproc");
new Thread (() -> {
for(int i=0;i<5000;i++) {
	var res2=processint(1,null,null);
//	Log.i(LOG_ID, "Natives.processint(1, 0, null)="+res2);
	byte[] input={(byte)0x01,(byte)0x7A,(byte)0x4E,(byte)0xD5,(byte)0x57,(byte)0x4B,(byte)0x00,(byte)0x7A,(byte)0xE0,(byte)0xB5,(byte)0x4A,(byte)0x04,(byte)0x50,(byte)0x1F,(byte)0x0D,(byte)0x09,(byte)0xE2,(byte)0xA1,(byte)0xF2,(byte)0xF5,(byte)0x3B,(byte)0xF0,(byte)0xF9,(byte)0x47,(byte)0x3D,(byte)0xF1,(byte)0xB7,(byte)0x8D,(byte)0xAA,(byte)0xE9,(byte)0xF7,(byte)0x0E,(byte)0x03,(byte)0x37,(byte)0xE9,(byte)0x20,(byte)0xDC,(byte)0xF9,(byte)0xCE,(byte)0x55,(byte)0xC7,(byte)0xA8,(byte)0x04,(byte)0xDA,(byte)0x33,(byte)0xDC,(byte)0x3C,(byte)0x3D,(byte)0xDC,(byte)0x38,(byte)0xE6,(byte)0x03,(byte)0xD1,(byte)0x61,(byte)0x1C,(byte)0x1B,(byte)0x65,(byte)0xFC,(byte)0x8A,(byte)0xF5,(byte)0xAB,(byte)0x24,(byte)0x7B,(byte)0xE4,(byte)0x42,(byte)0x20,(byte)0x95,(byte)0x1B,(byte)0xCA,(byte)0x6C,(byte)0x84,(byte)0x3A,(byte)0x69,(byte)0x4B,(byte)0xEA,(byte)0x71,(byte)0x4E,(byte)0xB7,(byte)0xE4,(byte)0x31,(byte)0x0F,(byte)0x91,(byte)0x25,(byte)0x8F,(byte)0x58,(byte)0x4C,(byte)0x1A,(byte)0x4A,(byte)0x31,(byte)0xF6,(byte)0xE2,(byte)0x90,(byte)0x39,(byte)0xE2,(byte)0xA6,(byte)0x26,(byte)0x4F,(byte)0x60,(byte)0x84,(byte)0x70,(byte)0x02,(byte)0xFA,(byte)0x06,(byte)0x20,(byte)0x2E,(byte)0x60,(byte)0x7A,(byte)0xE9,(byte)0x7E,(byte)0x6D,(byte)0xA6,(byte)0x1C,(byte)0x26,(byte)0x95,(byte)0x52,(byte)0x3B,(byte)0x30,(byte)0xEE,(byte)0xD5,(byte)0x61,(byte)0xEA,(byte)0x4B,(byte)0xF1,(byte)0x3D,(byte)0x47,(byte)0xA3,(byte)0x47,(byte)0x0A,(byte)0xA0,(byte)0x6D,(byte)0xCC,(byte)0xEB,(byte)0x10,(byte)0x7D,(byte)0xE5,(byte)0x18,(byte)0x78,(byte)0x10,(byte)0x94,(byte)0x4F};
		int res= Natives.processint(4, input, null);
//		Log.i(LOG_ID,"Natives.processint(4, input, null)="+res);
	 }
	Natives.enddebug();
	}).start();
	}

	if(tk.glucodata.ECDHCrypto.verifyinput) {
		try {
		       if(java.security.Security.insertProviderAt(org.conscrypt.Conscrypt.newProvider(), 1) < 0) {
			    Log.i(LOG_ID,"Security.insertProviderAt already installed");
			    }
			 else
				Log.i(LOG_ID,"Security.insertProviderAt installed");
			} catch (Throwable th) {
				Log.stack(LOG_ID,"insertProviderAt",th);
			}
		}
*/	 
	};
}
