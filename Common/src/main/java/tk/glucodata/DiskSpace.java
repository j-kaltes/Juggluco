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
import android.os.Environment;
import android.os.StatFs;

import java.io.File;

class DiskSpace {


static boolean check(Context context) {
	StatFs sta;
	try {
		String datadir= Environment.getDataDirectory().getAbsolutePath();
//		context.getDataDir().getAbsolutePath();
		sta=new StatFs(datadir);
		}
	catch(Throwable th) {
		File files=context.getFilesDir();
		String filespath=files.getAbsolutePath();
		if(!files.isDirectory())  {
			if(!files.mkdirs()) {
				throw new RuntimeException("Can't access "+filespath);
				}
			}
		sta=new StatFs(filespath);
		}
	var available=sta.getAvailableBytes();
	Log.i("Available:",""+available);
	return available>(6*1024*1024L);
//	return false;
	}

}
