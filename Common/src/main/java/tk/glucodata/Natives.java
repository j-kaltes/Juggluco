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



import com.eveningoutpost.dexdrip.services.broadcastservice.models.GraphLine;

import java.util.ArrayList;

import tk.glucodata.nums.item;


public class Natives  {
    static final private String LOG_ID="Natives";
//   static long numptr=0;
static  {
       try {
	android.util.Log.d(LOG_ID,"before LoadLibrary");
           System.loadLibrary("g");
	android.util.Log.d(LOG_ID,"after LoadLibrary");
   	Applic.Nativesloaded=true;
       }

    catch(       Throwable  error) {
        Applic.Nativesloaded=false;
	String mess=error!=null?error.getMessage():null;
	if(mess==null) {
		mess="error";
		}
        android.util.Log.e(LOG_ID,mess);
   }


	}

	public static  native int nfcdata(byte[] uid,byte[] info,byte[] dat);
	//public stratic native boolean shouldenableStreaming(byte[] uid);
//	public static native void enabledStreaming(byte[] uid,byte[] info,boolean val);
	public static native void enabledStreaming(byte[] uid,byte[] info,int val,byte[] address); // address!=null enables, address==null don't ask again
public static native boolean hasBluetooth(byte[] sensorident,byte[] patchinfo);
     public static native void initopengl(boolean started);
     public static native int openglversion();
	
//    public static native void showlast();
//    public static native void hidescanresults();
    public static native void resize(int width, int height,int initscreenwidth);
     public static native void setfontsize(float small,float menu,float density,float head);
     public static native int setfilesdir(String dir,String country,String nativedir);
     public static native void calccurvegegs();
  public static native void startthreads();
public static native void startmeals();
public static native void startsensors( );


	public static native void setlocale(String loc,boolean hour24);
     public static native int step();
    public static native int badscan(int kind);
//    public static native long lastpoll();
    //    public static native void clear();
    public static native void prevscr();
     public static native void nextscr();
     public static native void prevday(int day);
     public static native void nextday(int day);
//       public static native void savestate();
    public static native int translate(float dx,float dy,float yold,float y);
    public static native void flingX(float vol);
    public static native void xscale(float scalex, float midx);
    public static native boolean isbutton(float x, float y);
    public static native long tap(float x, float y);
    public static native long longpress(float x, float y);
    public static native long getstarttime();

    public static native long oldestdatatime();

    public static native void movedate(long date,int year,int month,int day);
     public static native long getendtime();
//     public static native void search(time[0],time[1],labelsel,funder,fabove,fgunder,fgabove,minutes[0],minutes[1]);
    public static native int search(int label,float vunder,float vabove,int frommin,int tomin,boolean forward,String regexingr,float amount); 

    public static native int earliersearch();
    public static native int latersearch();
    public static native void stopsearch();
    public static native byte  activationcommand(byte[] info);
    public static native byte [] activationpayload(byte[] id,byte[] info,byte person);
    public static native long getdataptr(String sensorname);

public static native long getsensorptr(long dataptr);
    public static native boolean sameSensor(long one,long two);
   public static native String getSensorName(long dataptr);

    public static native void    freedataptr(long dataptr);
public static native void setDeviceAddress(long dataptr,String deviceAddress );
//public static native void setDeviceAddressB(long dataptr,byte[] address );
 
public static native String getDeviceAddress(long dataptr);
//public static native void backupstream(long dataptr);

    public static native byte[]    sensorUnlockKey(long dataptr);
	public static native void resetbluetooth(long dataptr);
//    public static native void saveState(long dataptr);

    public static native long  processTooth(long dataptr, byte[] bluetoothdata);
 //   public static native boolean    iswaiting(long dataptr);
//    public static native void    setwaiting(long dataptr,boolean val);
    public static native String lastsensorname();
	public static native long laststarttime();

    public static native String[] activeSensors();
    public static native byte[] bluetoothOnKey(byte[] sensorident, byte[] patchinfo);
    public static native void bluetoothback( byte[] sensorident,byte[] info);
//    public static native void setglucose(long time,float glu,String sensor,int index);
    public static native void setmaxsensors(int nr);
    public static native float freey();    
    public static native long	openNums(String base,long ident);
    public static native long getident(long ptr);

    public static native void setident(long numptr, long ident);
    public static native void closeNums(long ptr);
    public static native void saveNum(long ptr,long time,float value,int type,int mealptr);
    public static native void saveNumpos(long ptr,int pos,long time,float value,int type,int mealptr);
    public static native void removeNum(long ptr,int pos);
    public static native item getNumitem(long ptr, int pos);
    public static native int getfirstNum(long ptr);
    public static native int getlastNum(long ptr);
    public static native int getlastpolledNum(long ptr);
    public static native void setlastpolledNum(long ptr,int pos);
    public static native int getchangedNum(long ptr);
    public static native int getonechangeNum(long ptr);
 public static native boolean receivedbackup(long ptr);

public static native boolean isreceiving( );
//    public static native void setfirstNum(long ptr,int pos); //unused
    public static native void setlastNum(long ptr,int pos); //unused

    public static native void setchangedNum(long ptr,int pos);
    public static native void updatedNum(long ptr,int pos);
	public static native void updatedNumstartend(long ptr,int start,int end);
  	public static native int getNumindex(long ptr);	


//Operate on results
  public static native long newhit();	
  public static native long mkhitptr(long ptr,int pos);	
  public static native long hittime(long hitptr);	
  public static native float hitvalue(long hitptr);	
  public static native int hitmeal(long hitptr); 


  public static native int hitremove(long hitptr);	
  public static native int hittype(long hitptr);	
    public static native void hitchange(long hitptr,long time,float value,int type,int mealptr);
//    public static native boolean hitneedsync(long hitptr);
  	public static native int gethitindex(long ptr);	
  public static native void freehitptr(long hitptr);	

    
    public static native ArrayList<String> getLabels();
public static native float getPrecision(int i);

public static native void setsettings(int unit,float glow, float ghigh, float tlow, float thigh);
public static native void setalarms(float alow, float ahigh, boolean lowalarm, boolean highalarm, boolean availablealarm,boolean lossalarm);



public static native void setunit(int unit);
public static native boolean setlabel(int pos,String label, float prec,float weight);
public static native void setnrlabel(int nr);
public static native boolean shouldsendlabels();
public static native void havesendlabels();
public static native void receivedcuts();
public static native int getunit();
public static native float graphlow( );
public static native float graphhigh( );
public static native float targetlow( );
public static native float targethigh( );
public static native float alarmlow( );
public static native float alarmhigh( );
public static native boolean hasalarmhigh( );
public static native boolean hasalarmlow( );
public static native boolean hasalarmloss( );
public static native boolean hasvaluealarm( );
//public static native boolean toCalendarApp();
public static native float getweight(int pos);
public static native float getprec(int pos);
public static native void log(String mess);
public static native void dolog(boolean val);
public static native boolean islogging( );

public static native String advanced();
public static native boolean exportdata(int type,int fd,int showdays);
//public static native void setlibversion(String version);
public static native int getcolumns(int width);

public static native int  numcontrol(int w,int h);
public static native void endnumlist( );
public static native void forwardnumlist( );
public static native void backwardnumlist( );
public static native void firstpage( );

public static native void lastpage( );
public static native String getLibraryName();
//s/^extern.*JNIEXPORT[         ]*j*\([a-zA-Z]*\)[ ]*JNICALL[      ]*fromjava(\([^)]*\))(JNIEnv[^,]*,[^,)]*[,)]\([^){]*\)[^a-zA-Z0-9]*$/public static native \1 \2(\3);/g

public static native long sensorends();
public static native String getserial(byte[] uid,byte[] info);
public static native ArrayList<ArrayList<Object>> getShortcuts();
public static native int setShortcut(int index,String name,String value);
public static native void setnrshortcuts(int nr);
public static native boolean sendcuts();
public static native void setnodebug(boolean val);
public static native void sethaslibrary(boolean val);
//public static native void sethaslib(boolean val);
public static native boolean gethaslibrary( );
public static native int getScreenOrientation( );
public static native void setScreenOrientation(int val);
public static native boolean gethasgarmin( );
public static native void sethasgarmin(boolean val);
public static native void setusebluetooth(boolean val);
public static native boolean getusebluetooth( );
public static native boolean streamingAllowed();
//public static native void systemUI(boolean val);
public static native boolean getsystemUI();
//public static native void startbackup();

public static native boolean backuphasrestore( );
public static native int backuphostNr( );
public static native String[] getbackuphostnames(int pos);
public static native String getbackuphostport(int pos);
public static native String getbackuppassword(int pos);
public static native boolean isWearOS(int pos);
//public static native int changebackuphost(int pos,String name,String port,boolean nums,boolean stream,boolean scans );
//public static native int changebackuphost(int pos,String name,String port,boolean nums,boolean stream,boolean scans,boolean receive,boolean reconnect,String pass);
//public static native int changebackuphost(int pos,String[] names,int nr,boolean dodetect,String port,boolean nums,boolean stream,boolean scans,boolean recover,boolean receive,boolean reconnect,boolean accepts,String pass,long starttime);

public static native int changebackuphost(int pos,String[] names,int nr,boolean detect,String port,boolean nums,boolean stream,boolean scans,boolean recover,boolean receive,boolean activeonly,boolean passiveonly,String pass,long starttime,String label,boolean testip);


public static native boolean detectIP(int pos);
public static native long lastuptodate(int pos);

//public static native int addbackuphost(String name,String port,boolean nums,boolean stream,boolean scans );
public static native boolean getbackuphostnums(int pos);
public static native boolean getbackuphoststream(int pos);
public static native boolean getbackuphostscans(int pos);
public static native int getbackuphostreceive(int pos);
public static native boolean getbackuphostactive(int pos);
public static native boolean getbackuphostpassive(int pos);
public static native boolean getbackuptestip(int pos);
public static native String getbackuplabel(int pos);




public static native void wakestreamsender( );
public static native void wakebackup( );
public static native void wakestreamhereonly();
public static native void wakehereonly( );
public static native void resetbackuphost(int pos);



public static native void deletebackuphost(int pos);
public static native boolean resetbylabel(String label,boolean galaxy);

public static native void setreceiveport(String port);
public static native String getreceiveport( );
public static native void networkpresent( );
public static native void networkabsent();
public static native void resetnetwork();
public static native void setpaused(GlucoseCurve val);

//public static native boolean usemeal();
public static native boolean staticnum();
public static native void setstaticnum(boolean val);
public static native int[] numAlarmEvents( );

public static native long firstAlarm( );
public static native void setNumAlarm(int type,float value,int start,int end);
public static native Object[] getNumAlarm(int pos);
public static native void delNumAlarm(int pos);
public static native int getNumAlarmCount();
public static native int camerakey();
public static native void setcamerakey(int on);
/*
public static native String readlowring( );
public static native void writelowring(String juristr);
public static native String readhighring( );
public static native void writehighring(String juristr);
public static native String readagainring( );
public static native void writeagainring(String juristr);
public static native String readnumring( );
public static native void writenumring(String juristr);
*/
//s/^extern.*JNIEXPORT[         ]*j*\([a-zA-Z]*\)[ ]*JNICALL[      ]*fromjava(\([^)]*\))(JNIEnv[^,]*,[^,)]*[,)]\([^){]*\)[^a-zA-Z0-9]*$/public static native \1 \2(\3);/g

public static native boolean askstreamingEnabled(long dataptr);

public static native boolean getshowalways( );
public static native void setshowalways(boolean val);

public static native String readring(int type);
public static native boolean writering(int type,String uristr,boolean sound,boolean flash,boolean vibrate);
public static native boolean alarmhasflash(int type);
public static native boolean alarmhassound(int type);
public static native boolean alarmhasvibration(int type);
public static native boolean getalarmdisturb(int type);
public static native void setalarmdisturb(int type,boolean dist);


public static native int readalarmduration(int type);

public static native void writealarmduration(int type,int dur);
public static native short readalarmsuspension(int type);

public static native void writealarmsuspension(int type,short sus);
public static native boolean getInvertColors( );
public static native void setInvertColors(boolean val);
public static native boolean getfixatex( );
public static native void setfixatex(boolean val);
public static native boolean getfixatey( );
public static native void setfixatey(boolean val);

public static native void pressedback();
//public static native String chmod(String filename,int mode);
//public static native void resendtowatch();
public static native void saveingredient(int pos,String name,String unit,float carb);
public static native void deleteingredient(int pos);
public static native boolean ingredientdeleteable(int pos);
public static native int ingredientNr();
public static native float ingredientCarb(int index);
//public static native int ingredientUsed(int index);

public static native String ingredientUnitName(int index);
public static native int ingredientUnit(int index);
public static native String ingredientName(int index);
//public static native int additemtomeal(int mealptr,int ingr,float amount);
public static native int changemealitem(int mealptr,int pos,int ingre,float amount);

public static native float getitemamount(int mealptr,int pos);
public static native int getitemingredient(int mealptr,int pos);
public static native String getitemingredientname(int mealptr,int pos);
public static native int getmealitemnr(int mealptr);
public static native float carbinmeal(int mealptr);
public static native void deletemeal(int mealptr);
public static native int getnewmealptr( );
public static native void closemeal(int mealptr);
public static native int deletefrommeal(int mealptr,int pos);
public static native void hitsetmealptr(long ptr,int mealptr);

public static native void setmealvar(byte val);
public static native byte getmealvar( );
public static native ArrayList<String> getunits();
public static native float getroundto( );
public static native void setroundto(float val);
public static native long foodsearch( String zoek);
public static native void freefoodptr( long ptr);
public static native int foodhitnr( long ptr);
public static native String foodlabel( long ptr,int pos);
public static native int[] getcomponents(int ingr);
public static native String[] getcomponentlabels( );
public static native String[] getcomponentunits( );
public static native int getfoodid( long ptr,int pos);
public static native String idfoodlabel( int ind);
public static native int foodnr( );
public static native int cpmeal(int mealptr);


public static native void setlastcolor(int color);
public static native int getlastcolor();

public static native void summarygraph(boolean val);
public static native void analysedays(int days);
public static native void endstats( );
public static native boolean setgarminid(String id);
public static native String getgarminid( );
public static native String getdefaultid( );

public static native boolean getuseflash( );
public static native void setuseflash(boolean val);


//public static native void zeromeal( );
public static native boolean getwaitwithenablestreaming();
public static native void setwaitwithenablestreaming(boolean val);
public static native void setscreenwidthcm(float val);
public static native boolean nfcsound();
public static native void setnfcsound(boolean val);
//public static native void setxinfuus(boolean val);
public static native boolean getxbroadcast();
//public static native void setxbroadcast(boolean val);
public static native boolean getJugglucobroadcast();
//public static native void setJugglucobroadcast(boolean val);




public static native void setlevelleft(boolean val);
public static native boolean getlevelleft();
/*
public static native long nfcptr(byte[] uid,byte[] info);
public static native int nfcstart(long ptr);
public static native int nfcadd(long ptr,byte[] scanned);
public static native byte[] nfcgetresults(long ptr); */
public static native boolean getisalarm();
public static native void setisalarm(boolean val);
//public static native boolean gettoucheverywhere( );
//public static native void settoucheverywhere(boolean val);
public static native boolean turnoffalarm( );
public static native void maintenance( );
public static native boolean shouldwakesender();

public static native boolean getusegarmin( );
public static native void setusegarmin(boolean val);
public static native boolean getusexdripwebserver( );
public static native void setusexdripwebserver(boolean val);
//public static native void startwatchserver( );
//public static native void stopwatchserver( );
public static native int V1( int i, int i2,byte[]  ar1,byte[]  ar2);
public static native byte[] V2(int i, int i2, byte[] ar1, byte[] ar2);
public static native boolean abbottinit();
public static native boolean abbottreinit();
public static native byte[] getstreamingAuthenticationData(long dataptr);
public static native byte[] getsensorident(long dataptr);
public static native int getsensorgen(long dataptr);
public static native int getLibreVersion(long dataptr);

public static native void USenabledStreaming(byte[] sensorident,byte[] jauth,byte[] address);
public static native void closedynlib( );
public static native strGlucose lastglucose();
public static native byte[] getmynetinfo(String name,boolean create,int watchhassensor,boolean galaxy);
public static native boolean setmynetinfo(String name,byte[] jar,boolean galaxy);

public static native long lastglucosetime( );
public static native byte[] bytesettings();
public static native boolean ontbytesettings(byte[]  ar);
public static native int directsensorwatch(String ident );
public static native boolean getshowscans();
public static native boolean getshowhistories();
public static native boolean getshowstream();
public static native boolean getshownumbers();

public static native boolean getshowmeals();
public static native void setshowmeals(boolean val);

public static native void setshowscans(boolean val);
public static native void setshowhistories(boolean val);
public static native void setshowstream(boolean val);
public static native void setshownumbers(boolean val);
public static native String getsensortext(long dataptr);
public static native void reenableStreaming();
public static native boolean getaskedNotify( );
public static native void setaskedNotify(boolean val);


/*
public static native boolean getuseWearos( );
public static native void setuseWearos(boolean val);
*/

//public static native byte[] glucose2bytearray( Glucose val);
//public static native Glucose bytearray2glucose( byte[] in);

public static native boolean hasstreamed( );
public static native void setXdripServerLocal(boolean val);
public static native boolean getXdripServerLocal( );

//s/^extern.*JNIEXPORT[         ]*\([a-zA-Z]*\)[ ]*JNICALL[      ]*fromjava(\([^)]*\)) *(JNIEnv[^,]*,[^,)]*[,)]\([^){]*\)[^a-zA-Z0-9]*$/public static native \1 \2(\3);/g
//public static native int getlastGen( );
//public static native void setlastGen(int val);
public static native boolean switchgen2( );

//public static native void logBluetoothGattCharacteristic(String jmess, BluetoothGattCharacteristic bluetoothGattCharacteristic);

public static native void clearlibreview( );
public static native void wakelibreview(int min );


public static native void setlibrebaseurl(boolean l3,String baseurl);
public static native String getlibrebaseurl(boolean l3 );

public static native void setlibreemail(String email);
public static native String getlibreemail( );

public static native void setlibrepass(String pass);
public static native String getlibrepass( );
public static native void setlibreDeviceID(boolean l3,String DeviceID);
public static native String getlibreDeviceID( boolean l3);

public static native void setlibreAccountID(String AccountID);
public static native void askServerforAccountID();
public static native long getlibreAccountIDnumber();
public static native void setlibreAccountIDnumber(long num);

public static native String getlibreAccountID( );

public static native void setlibreUserToken(boolean l3, String UserToken);
public static native String getlibreUserToken(boolean l3 );
public static native void setnewYuApiKey(boolean l3,String jpass);
public static native String getnewYuApiKey(boolean l3 );
public static native boolean getuselibreview( );
public static native void setuselibreview(boolean val);
public static native void savelibrerubbish(String FirstName,String LastName,int dat,String GardianFirstName,String GuardianLastName);
/*
public static native int getDateOfBirth( );
public static native String getLastName( );
public static native String getFirstName( );
public static native String getGuardianFirstNameName( );
public static native String getGuardianLastNameName( );
 */       


//public static native long interpret3NFC1(byte[] b);
public static native long interpret3NFC2(byte[] nfc1,byte[] b,long time);
public static native long getLibre3secs(byte[]  nfc1ar);
public static native long getLibre3Sensorptr(String sensorid,long starttime);
public static native long getLibre3SensorptrPD(String sensorid,long starttime,byte[] pin,String deviceaddress);
public static native long saveLibre3MinuteL(long sensorptr,byte[] mindata);
public static native boolean saveLibre3fastData(long sensorptr,byte[] mindata);
public static native boolean saveLibre3History(long sensorptr,byte[] mindata);
public static native int libre3EventLog(long sensorptr,byte[] logs);
public static native void setLibre3kAuth(long sensorptr,byte[] kauthin);
public static native byte[] getLibre3kAuth(long sensorptr);
public static native int libre3processpatchstatus( long sensorptr,byte[] status);
public static native byte[] getpin(long sensorptr);
public static native int getlastLifeCountReceived(long sensorptr);
public static native int getlastHistoricLifeCountReceived(long sensorptr);


public static native byte[] libre3ControlHistory( int arg,int from);
public static native byte[] libre3ClinicalControl( int arg,int from);
public static native byte[] libre3EventLogControl( int arg);


public static native void initjuggluco(String filesdir);

public static native boolean getpriority( );
public static native void setpriority(boolean val);
public static native boolean stopWifi();
public static native void setWifi(boolean val);
public static native boolean getWifi( );
public static native void showbytes(String mess,byte[] ar);
//public static native boolean loadECDHCrypto( );
//public static native boolean loadNFC( );
public static native int startTimeIDsum(byte[] uit, long time, long accountID);

public static native int processint(int i2, byte[] bArr, byte[] bArr2);
public static native byte[] processbar(int i2, byte[] bArr, byte[] bArr2);
public static native boolean getShownintro( );
public static native void setShownintro(boolean val);
public static native void setDevice(String mani,String model,String release);
public static native String getxDripTrendName(float rate);
public static native boolean getkerfstokblack( );
public static native void setkerfstokblack(boolean val);
public static native long initcrypt(long prev,byte[] key,byte[] iv);
public static native void endcrypt(long jptr);
public static native byte[] intEncrypt(long cryptptr, int kind,byte[] plain);
public static native byte[] intDecrypt(long cryptptr, int kind,byte[] encrypted);
public static native float thresholdchange(float drate);
//public static native void enddebug();
public static native boolean message(byte[] data);
public static native boolean getBlueMessage(int index);
public static native void setBlueMessage(String name,boolean val);
                
public static native void setwakelock(boolean val);
public static native boolean getwakelock( );
public static native boolean hasRootcheck( );
public static native void setheartrate(boolean val);
public static native boolean getheartrate( );
public static native String[] librelinkRecepters( );
public static native void setlibrelinkRecepters( String[] jnames);
public static native boolean getlibrelinkused();
public static native String[] xdripRecepters( );
public static native void setxdripRecepters( String[] jnames);
public static native String[] glucodataRecepters( );
public static native void setglucodataRecepters( String[] jnames);
public static native void setinitVersion(int val);
public static native int getinitVersion( );

public static native void setfloatglucose(boolean val);
public static native boolean getfloatglucose( );
public static native void setfloatingFontsize(int val);
public static native int getfloatingFontsize( );
public static native void setfloatingbackground(int val);
public static native int getfloatingbackground( );
public static native void setfloatingforeground(int val);
public static native int getfloatingforeground( );

        
public static native void setfloatingTouchable(boolean val);
public static native boolean getfloatingTouchable( );

public static native void setfloatingPos(int val);
public static native int getfloatingPos( );
public static native void finishSensor(long dataptr);

public static native long manualLibreAccountIDnumber( );
public static native void setApiSecret(String japisecret);
public static native String getApiSecret( );
public static native String setuseSSL(boolean val);
public static native boolean getuseSSL( );

public static native void setsslport(int val);
public static native int getsslport( );

public static native void setsaytreatments(boolean val);
public static native boolean getsaytreatments( );
public static native boolean getuseuploader( );
public static native String getnightuploadurl( );
public static native String getnightuploadsecret( );
public static native void setNightUploader(String jurl,String Secret,boolean active);
public static native void wakeuploader( );
public static native void resetuploader( );
public static native void setAndroid13(boolean val);
public static native boolean getAndroid13( );
public static native long[] getlastGlucose( );
public static native void setwatchdrip(boolean val);
public static native boolean getwatchdrip( );

public static native boolean mkWearIntgraph(long start, long end, GraphLine low, GraphLine within, GraphLine high);
public static native void isGalaxyWatch(boolean val);

public static native long novopentype(String serial);
public static native int savenovopen(long time,String serial,int type,byte[] gegs,boolean last);
//public static native long lasttimenovopen(String serial);
public static native boolean oldnovopenvalue(long referencetime,String serial,byte[] rawdoses);

public static native void setnovopenttimeandtype(long time,int type,String serial);

public static native String mirrorStatus(int allindex);
public static native void setWearosdefaults(String jident,boolean galaxy);
public static native void setSeparate(boolean val);
public static native boolean getSeparate( );

public static native void setLibreCurrent(boolean val);
public static native boolean getLibreCurrent( );
public static native String getCPUarch();

public static native void setUSEALARM(boolean val);
public static native boolean getUSEALARM( );
public static native void setgadgetbridge(boolean val);
public static native boolean getgadgetbridge( );
public static native void saveVoice(float speed,float pitch,int voicesep,int voice,boolean active);
public static native int getVoiceTalker( );
public static native int getVoiceSeparation( );
public static native float getVoiceSpeed( );
public static native float getVoicePitch( );
public static native boolean getVoiceActive( );

public static native boolean getSendNumbers();
public static native void setSendNumbers(boolean val);

public static native void setlibrenum(int night,int index,int kind, float weight);
public static native int getlibrenumkind(int night,int index);
public static native float getlibrefoodweight(int night,int index);
public static native boolean canSendNumbers(int night);
public static native int getinfogen(byte[] info);
public static native boolean isLibreMmol( );
public static native void setStreamHistory(boolean val);
public static native boolean getStreamHistory( );
public static native void setLibreIsViewed(boolean val);
public static native boolean getLibreIsViewed( );
public static native void setRTL(boolean val);
public static native boolean getRTL( );
public static native boolean makepercentages( );
public static native void makenumbers();
public static native void settouchtalk(boolean val);
public static native boolean gettouchtalk( );
public static native boolean getsystemui( );
public static native void setsystemui(boolean val);
public static native void settonow();
public static native boolean showlastscan( );
public static native void saylastglucose();
public static native void setspeakmessages(boolean val);

public static native boolean speakmessages( );
public static native void setspeakalarms(boolean val);

public static native boolean speakalarms( );

//s/^extern.*JNIEXPORT[         ]*\([a-zA-Z]*\)[ ]*JNICALL[      ]*fromjava(\([^)]*\)) *(JNIEnv[^,]*,[^,)]*[,)]\([^){]*\)[^a-zA-Z0-9]*$/public static native \1 \2(\3);/g
//public static native void sendxdripold();
/*
public static boolean speakmessages() {
	return gettouchtalk();
	}
public static boolean speakalarms() {
	return SuperGattCallback.dotalk;
	}*/

}


