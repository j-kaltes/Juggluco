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
/*      Fri Jan 27 15:31:10 CET 2023                                                 */


package tk.glucodata.nums;



import java.io.File;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

import tk.glucodata.Applic;
import tk.glucodata.BuildConfig;
import tk.glucodata.Log;
import tk.glucodata.Natives;
import tk.glucodata.R;

import static android.os.Build.VERSION.RELEASE;
import static java.lang.System.exit;
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.Log.doLog;
import static tk.glucodata.Natives.setDevice;
import static tk.glucodata.Natives.startmeals;
import static tk.glucodata.Natives.startsensors;

import android.os.Build;
import android.text.format.DateFormat;


public class numio   {
private static final String LOG_ID="numio";
public static long[] numptrs={0L,0L};
public static long newhit;

static 	private final String datadir="watch";
static class noAccessToFilesDirException extends RuntimeException {
public noAccessToFilesDirException(String name) { 
	super(name);
	}
	};
static void settingsnull() {
	throw new NullPointerException("settings==null");
	}
static void settingsdatanullmisc() {
	throw new NullPointerException("settings->data()==null");
	}
static void settingsnoaccessfile()  {
	throw new noAccessToFilesDirException("no access settings.dat");
	}
static void settingsnoaccessdir()   {
	throw new noAccessToFilesDirException("no access files");
	}

        static void settingsdataaccessEACCES()    { throw new noAccessToFilesDirException("no access files: EACCES"); }

        static void settingsdataaccessELOOP()   { throw new noAccessToFilesDirException("no access files: ELOOP"); }

        static void settingsdataaccessENAMETOOLONG()   { throw new noAccessToFilesDirException("no access files: ENAMETOOLONG"); }

        static void settingsdataaccessENOENT()   { throw new noAccessToFilesDirException("no access files: ENOENT"); }

        static void settingsdataaccessENOTDIR()   { throw new noAccessToFilesDirException("no access files: ENOTDIR"); }

        static void settingsdataaccessEROFS()   { throw new noAccessToFilesDirException("no access files: EROFS"); }

        static void settingsdataaccessEBADF()   { throw new noAccessToFilesDirException("no access files: EBADF"); }

        static void settingsdataaccessEINVAL()   { throw new noAccessToFilesDirException("no access files: EINVAL"); }

        static void settingsdataaccessETXTBSY()   { throw new noAccessToFilesDirException("no access files: ETXTBSY"); }

static void makefilesfailed()   { throw new noAccessToFilesDirException("no access files: ETXTBSY"); }

static public boolean setlibrary(Applic con) {
if(true) {
	var loc=Locale.getDefault();
       String country=loc.getCountry();
	var locstr=loc.getLanguage();
	Applic.curlang=locstr;
        File files=con.getFilesDir();
	String filespath=files.getAbsolutePath();
	if(!files.isDirectory())  {
		if(!files.mkdirs()) {
			Applic.stopprogram=2;
			return false;
			}
		}
	int ret=130;
	final String nativedir=con.getApplicationInfo().nativeLibraryDir;
        Natives.setlocale(locstr,(Applic.hour24= DateFormat.is24HourFormat(con)));
	switch (ret=Natives.setfilesdir(filespath, country,nativedir)) {
		case 1:
			settingsnull();
			break;
		case 2:
			settingsdatanullmisc();
			break;
		case 3:
			settingsnoaccessdir();
			break;
		case 4:
			settingsnoaccessfile();
			break;
		case 5:
			settingsdataaccessEACCES();
			break;

		case 6:
			settingsdataaccessELOOP();
			break;

		case 7:
			settingsdataaccessENAMETOOLONG();
			break;

		case 8:
			settingsdataaccessENOENT();
			break;

		case 9:
			settingsdataaccessENOTDIR();
			break;

		case 10:
			settingsdataaccessEROFS();
			break;

		case 11:
			settingsdataaccessEBADF();
			break;

		case 12:
			settingsdataaccessEINVAL();
			break;

		case 13:
			settingsdataaccessETXTBSY();
			break;
		default:
	}

	if(doLog) {
		var build=new StringBuilder();
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
			build.append("\nVERSION.BASE_OS: "+ Build.VERSION.BASE_OS);
			}
		build.append("\nVERSION.RELEASE: "+ RELEASE+"\n"+
		"VERSION.SDK_INT: "+ Build.VERSION.SDK_INT+"\n"+
		"BRAND: "+ Build.BRAND+"\n"+
		"CPU_ABI2: "+ Build.CPU_ABI2+"\n"+
		"CPU_ABI: "+ Build.CPU_ABI+"\n"+
		"DEVICE: "+ Build.DEVICE+"\n"+
		"FINGERPRINT: "+ Build.FINGERPRINT+"\n"+
		"MANUFACTURER: "+ Build.MANUFACTURER+"\n"+
		"MODEL: "+ Build.MODEL);

		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
			build.append("\nSUPPORTED_ABIS: ");
			for(var ap: Build.SUPPORTED_ABIS) {
				build.append(ap);
				build.append(", ");
				}
		}
	build.append('\n');
		Natives.log(build.toString());
		 final String version= BuildConfig.VERSION_CODE+" "+ BuildConfig.VERSION_NAME +" "+ BuildConfig.BUILD_TIME+"\n";
		Log.i(LOG_ID,version+locstr+" "+country+" nativeDir="+nativedir);
		}

	if(!isWearable)
		setDevice(Build.MANUFACTURER, Build.MODEL, RELEASE);

 	startsensors( );
	startmeals();
	Natives.startthreads();
	Log.i(LOG_ID,"after startthreads");
	numptrs[1]=Natives.openNums("here",0L);
	newhit=Natives.newhit();
	numptrs[0]=Natives.openNums(datadir,-1L); //order important!!
	Natives.calccurvegegs();
	Log.i(LOG_ID,"end setlibrary");
	return true;
	}
	else {
		Applic.stopprogram=2;
		return false;
		}
	
	}

public static long getident() {
	return Natives.getident(numio.numptrs[0]);
	}
public static void setident(long ident) {
	 Natives.setident(numio.numptrs[0],ident);
	}

static void open(String base,long ident) {
         Log.d(LOG_ID,"open("+base+","+ident+")");
	 if(numio.numptrs[0]!=0L) {
	 	if(ident==-1L||getident()==ident) 
			return;
		Natives.closeNums(numio.numptrs[0]);
	 	}
	numio.numptrs[0]= Natives.openNums(base,ident);
  }

static public  ArrayList<String> getlabels() {
	return Natives.getLabels();
	}
static void close() {
	if(numio.numptrs[0]!=0L)
		Natives.closeNums(numio.numptrs[0]);
	}
static void delete(int base,int pos) {
	Natives.removeNum(numio.numptrs[base],pos);
	}
static void writeAr(int base,int pos, List<Number> dat)  {
	if(dat==null) {
		delete(base,pos);
		return;
		}
	Natives.saveNumpos(numio.numptrs[base],pos,(long)((int)dat.get(0)),(float)dat.get(1),(int)dat.get(2),(dat.size()<4)?0:(int)dat.get(3));
	}

static item readitem(int base,int pos) {
	item uit=Natives.getNumitem(numio.numptrs[base], pos);
//	Log.i(LOG_ID,"readitem("+base+","+pos+")={"+uit.time+","+uit.value+","+uit.label+"}");
	if(uit.label==0xFFFFFFFF)
		return null;
	return uit;
 	}
static List<Number>  readAr(int base,int pos)  {
 	item it=readitem(base,pos) ;
	if(it==null)
		return null;
	if(it.time==0L)
		return null;
	List<Number> uit=new ArrayList<>();
	uit.add((int)(it.time&0xFFFFFFFFL));
	uit.add(it.value);
	uit.add(it.label);
	if(it.mealptr!=0)
		uit.add(it.mealptr);
	return uit;
	}

static public void updated(int base,int last)  {
	Natives.updatedNum(numio.numptrs[base],last);
	}
static public void updatedstartend(int base,int start,int end)  {
	Natives.updatedNumstartend(numio.numptrs[base],start,end);
	}
	/*
static public int readlast(int base)  {
	return Natives.getlastNum(numio.numptrs[base]);
	}
*/
public static int getlastnum(int base) {
	long ptr=numio.numptrs[base];
	return Natives.getlastNum(ptr); 
	}
public static void setlastnum(int base,int last) {
	long ptr=numio.numptrs[base];
	Natives.setlastNum(ptr,last);
	}
	
public static int getlastpollednum(int base) {
	long ptr=numio.numptrs[base];
	return Natives.getlastpolledNum(ptr); 
	}
public static boolean didreceivebackup(int base) {
	long ptr=numio.numptrs[base];
	var res=Natives.receivedbackup(ptr); 
	Log.i(LOG_ID,"didreceivebackup("+base+")="+res);
	return res;
	}
	
}

