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

import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.channels.FileChannel;
import java.util.List;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

import static android.view.View.GONE;
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.Natives.getShownintro;
import static tk.glucodata.Natives.setShownintro;

public class getlibrary {
static final private String LOG_ID="getlibrary";
//static  DownloadLibrary download;
static final boolean dotest=false;
public static class librarythread extends Thread {
	MainActivity context;
	File cali;
	librarythread(MainActivity act,File cali) {
		context=act;
		this.cali=cali;
		}
@Override
public void run() {
	String libfile=Natives.getLibraryName();
    PackageManager manage = context.getPackageManager();
    
    if(!dotest) {
	    List<PackageInfo> pack = manage.getInstalledPackages(0);

	    for (PackageInfo p : pack) {
		String name = p.packageName;
		if(name.startsWith("com.freestylelibre")) {
		    Log.d(LOG_ID,p.packageName);
		    Log.d(LOG_ID,p.versionName);
		    ApplicationInfo app = p.applicationInfo;
		    Log.d(LOG_ID,app.name);
		    Log.d(LOG_ID,"uid=" + app.uid);
		    Log.d(LOG_ID,"nativeLibraryDir="+app.nativeLibraryDir);
		   // String[] splits=app.splitSourceDirs;
			String[] splits= null;
			if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.LOLLIPOP) {
				splits = app.splitPublicSourceDirs;
			}
			if(splits!=null&&splits.length>0) {
		    	for(var s:splits) {
			    Log.d(LOG_ID,s);
			    if(unzipper(new File(s), libfile, cali)) {
				if(rightlib.testrightlib(context,cali)!=0)  {
					return;
					}
				}
			else {
				Log.d(LOG_ID,"no lib");
				}
				}
		    	}
		else {
		         Log.d(LOG_ID,app.sourceDir);
			    if(unzipper(new File(app.sourceDir), libfile, cali)) {
				if(rightlib.testrightlib(context,cali)!=0)  {
					return;
					}
				}
			}
		  }
    		}
	}	
//    download= new DownloadLibrary(context,libfile,cali); 

     getlibrary.openlibrary(context,libfile,cali) ;
     /*
      context.openfile=new Openfile(libfile,cali);

	 Applic.RunOnUiThread(() -> {   context.openfile.showchoice(context); });
	 */
       }
    }


 static void openlibrary(MainActivity context,String libfile,File cali) {
      context.openfile=new Openfile(libfile,cali);
      Applic.RunOnUiThread(() -> {   context.openfile.showchoice(context,true); });
       }

public static void openlibrary(MainActivity context) {
    File files = context.getFilesDir();
    File cali = new File(files, "libcalibrate.so");
     String libfile=Natives.getLibraryName();
      context.openfile=new Openfile(libfile,cali);
      Applic.RunOnUiThread(() -> {   context.openfile.showchoice(context,false); });
       }


public static boolean getlibrary(MainActivity context) {
	if(Applic.includeLib) {
		if(!isWearable) {
			if(!getShownintro()) {
				help.help(R.string.introhelp,context,l->setShownintro(true));
				}
		  }
		return true;
		}
    File files = context.getFilesDir();
    File cali = new File(files, "libcalibrate.so");
    if(Natives.gethaslibrary()) {
		if(!dotest) {
			if(cali.exists()) return true;
			}
		Natives.sethaslibrary(false);
		}


   help.help(R.string.introhelp,context);
   help.sethelpbutton(GONE); 
    if (!files.exists())
        files.mkdirs();
  new librarythread(context,cali).start();
    return false; 
}








static final byte[] end = {'P','K',(byte)(0xFF&0x1),(byte)(0xFF&0x2),(byte)(0xFF&0x1e),(byte)(0xFF&0x3),(byte)(0xFF&0xa),(byte)(0xFF&0x0),(byte)(0xFF&0x0),(byte)(0xFF&0x0),(byte)(0xFF&0x0),(byte)(0xFF&0x0),(byte)(0xFF&0xee),(byte)(0xFF&0xa1),'8','R','A',(byte)(0xFF&0xe4),(byte)(0xFF&0xa9),(byte)(0xFF&0xb2),(byte)(0xFF&0xd),(byte)(0xFF&0x0),(byte)(0xFF&0x0),(byte)(0xFF&0x0),(byte)(0xFF&0xd),(byte)(0xFF&0x0),(byte)(0xFF&0x0),(byte)(0xFF&0x0),(byte)(0xFF&0x9),(byte)(0xFF&0x0),(byte)(0xFF&0x18),(byte)(0xFF&0x0),(byte)(0xFF&0x0),(byte)(0xFF&0x0),(byte)(0xFF&0x0),(byte)(0xFF&0x0),(byte)(0xFF&0x0),(byte)(0xFF&0x0),(byte)(0xFF&0x0),(byte)(0xFF&0x0),(byte)(0xFF&0xb4),(byte)(0xFF&0x81),(byte)(0xFF&0x0),(byte)(0xFF&0x0),(byte)(0xFF&0x0),(byte)(0xFF&0x0),(byte)(0xFF&0x0),(byte)(0xFF&0x0),(byte)(0xFF&0x0),(byte)(0xFF&0x0),(byte)(0xFF&0x0),(byte)(0xFF&0x0),(byte)(0xFF&0x0),(byte)(0xFF&0x0),(byte)(0xFF&0x0),'U','T',(byte)(0xFF&0x5),(byte)(0xFF&0x0),(byte)(0xFF&0x3),'O',(byte)(0xFF&0xc7),(byte)(0xFF&0xd),(byte)(0xFF&0x60),'u','x',(byte)(0xFF&0xb),(byte)(0xFF&0x0),(byte)(0xFF&0x1),(byte)(0xFF&0x4),(byte)(0xFF&0xe8),(byte)(0xFF&0x3),(byte)(0xFF&0x0),(byte)(0xFF&0x0),(byte)(0xFF&0x4),(byte)(0xFF&0xe8),(byte)(0xFF&0x3),(byte)(0xFF&0x0),(byte)(0xFF&0x0),'P','K',(byte)(0xFF&0x5),(byte)(0xFF&0x6),(byte)(0xFF&0x0),(byte)(0xFF&0x0),(byte)(0xFF&0x0),(byte)(0xFF&0x0),(byte)(0xFF&0x1),(byte)(0xFF&0x0),(byte)(0xFF&0x1),(byte)(0xFF&0x0),'O',(byte)(0xFF&0x0),(byte)(0xFF&0x0),(byte)(0xFF&0x0),'x','V','4',(byte)(0xFF&0x12),(byte)(0xFF&0x0),(byte)(0xFF&0x0)};

private static boolean match(byte[] in,int off,byte[] el) {
	if(in.length<(off+el.length))
		return false;
	for(int i=0;i<el.length;i++) {
		if(in[off+i]!=el[i])
			return false;
		}
	return true;
	}
	/*
private static int max(int one,int two) {
	return Math.max(one, two);
	}*/
private static boolean replace(byte[] in,byte[] el,byte[] repl) {
	boolean hit=false;
	int end=in.length-Math.max(el.length,repl.length);
	for(int i=0;i<end;i++) {
		if(match(in,i,el)) {
			System.arraycopy(repl, 0, in, i, repl.length)	;
			hit=true;
			}
		}
	return hit;
	}


private static	void    putint(byte[] end,int putpos,int endpos) {
        int get=endpos;
        for(int i=0;i<4;i++) {
                end[putpos+i]=(byte)(get&0xFF);
                get>>=8;
                }

        }
private static boolean equal(ByteBuffer  one, int off1,ByteBuffer two, int off2,int len) {
	for(int i=0;i<len;i++)
		if(one.get(off1+i)!=two.get(off2+i))
			return false;
	return true;
	}

private static boolean unzipper(File ar, String file, File uit) {

	try(FileInputStream arstr = new FileInputStream(ar)) {
		return unzipper(arstr,file,uit);
		}
	 catch(Exception e) {
		Log.stack(LOG_ID,"first",e);
		return false;
	 }
	 }

static boolean process(FileInputStream arstr,ZipInputStream zf, int size,File uit) throws IOException {
	byte[] buf= new byte[size];
	for(int nam,over=size,it=0;(nam=zf.read(buf,it,over))>0;it+=nam)  {
		over-=nam;
		}
		
//	byte[] find={'l','i','b','c','.','s','o',0};
	byte[] find2={'l','i','b','l','o','g','.','s','o',0};
	byte[] repl={'l','i','b','g','.','s','o',0};
//	replace(buf,find,repl);
	replace(buf,find2,repl);
	byte[] find3={'l','i','b','D','a','t','a','P','r','o','c','e','s','s','i','n','g','.','s','o',0};
	byte[] repl3={'l','i','b','c','a','l','i','b','r','a','t','e','.','s','o',0};
	replace(buf,find3,repl3);
 	if(!uit.setWritable(true, true)) {
		Log.e(LOG_ID,"setWritable "+uit.toString()+" failed");
		}
	try(FileOutputStream out = new FileOutputStream(uit)) {
	  out.write(buf);
	  int 	endpos;
	  try(final FileChannel archan=arstr.getChannel()) {
		 long filesize=archan.size();
		 archan.position(filesize-6);
		ByteBuffer posbuf=ByteBuffer.allocate(4);
		  posbuf.order(ByteOrder.LITTLE_ENDIAN);
		 archan.read(posbuf);
		 int pos=posbuf.getInt(0);
		 if(pos<24) {
		 	Log.e(LOG_ID,"pos<24");
		 	return false;
			}
		 archan.position(pos-24);
		 posbuf.clear();
		 archan.read(posbuf);
		 int signlen=posbuf.getInt(0)+8;
		 archan.position(pos-signlen);
		ByteBuffer signbuf=ByteBuffer.allocate(signlen);
		 int res=archan.read(signbuf);
		 if(res!=signlen) {
		 	Log.e(LOG_ID,res+"!="+signlen);
			return false;
		 	}
		if(!equal(signbuf,0,signbuf,signlen-24,8)) {
		 	Log.e(LOG_ID,"No signature");
			return false;
			}
		out.write(signbuf.array());
		endpos=size+signlen;
		}
	int putpos=end.length-6;
	putint(end,putpos,endpos);
	out.write(end);
 	out.flush();
        out.getFD().sync();
	 }
	 Log.i(LOG_ID,"unzipped");
	 return true;
	}

//static boolean unzipper(InputStream arstr, String file, File uit) 
static boolean unzipper(FileInputStream arstr, String file, File uit) {
	Log.i(LOG_ID,file);
	try(ZipInputStream zf = new ZipInputStream(arstr)) {
		ZipEntry entry;		
		while(true) {
		    if((entry=zf.getNextEntry())==null)
		       return false;
		    if(file.equals(entry.getName())) {
			int size = (int)entry.getSize();
			Log.i(LOG_ID,entry.getName()+" size="+size);
		    	return process(arstr,zf,size,uit);
			}
		    zf.closeEntry();
		    }
		}
	 catch(Exception e) {
   		Log.stack(LOG_ID,"second",e);
		return false;
		 }
//	return true;
	} 

}
