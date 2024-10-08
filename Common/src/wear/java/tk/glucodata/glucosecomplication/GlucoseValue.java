package tk.glucodata.glucosecomplication;

import static android.graphics.Color.BLACK;
import static android.graphics.Color.BLUE;
import static android.graphics.Color.GREEN;
import static android.graphics.Color.WHITE;
import static android.graphics.Color.YELLOW;
import static android.graphics.Color.LTGRAY;
import static android.graphics.Color.DKGRAY;
import static android.graphics.Color.CYAN;
import static android.graphics.Color.MAGENTA;
import static android.graphics.Typeface.BOLD;
import static android.graphics.Typeface.NORMAL;

import static java.lang.Float.isNaN;
import static java.lang.String.format;

import static tk.glucodata.CommonCanvas.drawarrow;
import static tk.glucodata.CommonCanvas.drawarrowcircle;
import static tk.glucodata.Notify.unitlabel;
import static tk.glucodata.glucosecomplication.ColorConfig.defcol;

import android.annotation.SuppressLint;
import android.content.res.TypedArray;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.Rect;
import android.graphics.Typeface;
import android.os.Build;
import tk.glucodata.Log;

import tk.glucodata.Applic;
import tk.glucodata.Natives;
import tk.glucodata.Notify;
import tk.glucodata.R;

public class GlucoseValue {
final private static String LOG_ID="GlucoseValue";
//64 good
//final static float mapwidth=512,mapheight=512;
//final static float mapwidth=100,mapheight=100;
final  float mapwidth;
final float mapheight;
final  float half;
final  private float density;
    final private float timeoffy;
    final private float numoffy;
    final private float timesize;
    private float fontsize;

    final private Bitmap glucoseBitmap;
    final private Canvas canvas;
    final private Paint glucosePaint=new Paint();
   final private Typeface normaltype, boldtype;
//    final private Paint agePaint=new Paint();
GlucoseValue(int w,int h) {
	mapwidth=w;
	mapheight=h;
    half=0.5f*mapwidth;
    density= mapheight/70.0f;
     fontsize=mapwidth*0.63f;
   glucosePaint.setTextAlign(Paint.Align.CENTER);
  glucosePaint.setTextSize(fontsize);
  //agePaint.setARGB(0xFF,0xFF,0,0xFF);
  timesize=mapwidth*0.15f;
  numoffy=mapwidth*0.12f;
     if(Applic.hour24) {
        timeoffy=mapwidth*0.036f;
      }
      else {
    //    timesize=mapwidth*0.11f;
        timeoffy=mapwidth*0.066f;
   //     numoffy=mapwidth*0.1f;
      }

   Log.i(LOG_ID,"fontsize="+fontsize+" timesize="+timesize);


	glucosePaint.setAntiAlias(true);

	glucoseBitmap = Bitmap.createBitmap((int)mapwidth, (int)mapheight, Bitmap.Config.ARGB_8888);
	canvas = new Canvas(glucoseBitmap);

	Log.i(LOG_ID," mapwidth="+mapwidth+" mapheight="+mapheight+"color="+ format("%x",glucosePaint.getColor()));
//   glucosePaint.setColor(WHITE);
   final var basistype=Typeface.SANS_SERIF;
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
        boldtype= Typeface.create(basistype,750, false);
        normaltype= Typeface.create(basistype,350, false);
//        boldtype= Typeface.create(basistype,1000, false);
 //       normaltype= Typeface.create(basistype,200, false);
    }
    else {
        boldtype= Typeface.create(basistype,Typeface.BOLD);
        normaltype= Typeface.create(basistype,Typeface.NORMAL);
       }

//   glucosePaint.setTypeface(normaltype);
   }
int wasbackground=0;
static int newbackground=1;
private void setcolor() {
    final int background=Natives.getComplicationBackgroundColor( );
   if((background&0xff000000)!=0)
	   canvas.drawColor(background);
    else
      canvas.drawColor(Color.TRANSPARENT, PorterDuff.Mode.CLEAR);
    wasbackground=newbackground;
     }

void  setbackground() {
	if(wasbackground!=newbackground) {
		setcolor();
		}
	}
float  drawcenter(String value) {
        var bounds=new Rect();
        glucosePaint.setTextSize(fontsize);
        glucosePaint.getTextBounds(value, 0, value.length(),  bounds);
        float fsize=fontsize*mapwidth/bounds.right*.87f;
        glucosePaint.setTextSize(fsize);
        var des=glucosePaint.descent();
        var as=glucosePaint.ascent();
        Log.i(LOG_ID,"descent()="+des+" ascent()="+as+" fontsize="+fontsize);
        var y = half - (des + as)*.5f;
       canvas.drawText(value,0,value.length(),half,y,glucosePaint);
       return fsize;
   }
void mknovalue() {
   var novalue=Applic.app.getString(R.string.novalue);
   drawcenter(novalue);
   }
Bitmap getnovalue() {
   mknovalue();
    return glucoseBitmap;
    }

void setNumberBitmap(String value,long time,int index) {
      Log.i(LOG_ID,"setNumberBitmap "+value);
      if(index>=0) {
              glucosePaint.setTextSize(timesize);
             if(index!=0) {
               String instr=index+""; 
               canvas.drawText(instr,0,instr.length(),half,numoffy,glucosePaint);
               }
            var timestr=tk.glucodata.NumberView.minhourstr(time);
            canvas.drawText(timestr,0,timestr.length(),half,mapheight-timeoffy,glucosePaint);
            }
   fontsize=drawcenter(value);
	}


Bitmap getNumberBitmap(String value,long time,int index,long now) {
    setbackground();
    glucosePaint.setColor(getTextColor());
    if((now-time)<tk.glucodata.Notify.glucosetimeout) {
        setNumberBitmap(value, time, index);
       }
   else {
        mknovalue();
      }
    return glucoseBitmap;
	}

Bitmap getArrowBitmap(Float rate) {
   setbackground();
	Log.i(LOG_ID,"getBitmap");
	if(isNaN(rate)) {
		Log.i(LOG_ID,"rate=nan");
      		mknovalue();
	} else {
      glucosePaint.setColor(getArrowColor());
      drawarrowcircle(canvas,glucosePaint,density,rate);
 //tk.glucodata.CommonCanvas.testcircle(canvas,glucosePaint,density);
	}
	 return glucoseBitmap;
	}
	
Bitmap getArrowBitmap() {
   var glucose = Natives.lastglucose();
   if(glucose==null) {
     Log.d(LOG_ID,"lastglucose()==null");
      return getnovalue();
      }

    if((System.currentTimeMillis()-(glucose.time*1000L))>=tk.glucodata.Notify.glucosetimeout) {
         Log.d(LOG_ID,"oldvalue "+glucose.time);
         return getnovalue();
         }
    return getArrowBitmap(glucose.rate);
   }
static int getTextColor( ) {
   int col=Natives.getComplicationTextColor( );
   return col==0?defcol[1]:col;
   }
static int getArrowColor( ) {
   int col=Natives.getComplicationArrowColor( );
   return col==0?defcol[0]:col;
   }

static int getBackgroundColor( ) {
   int col=Natives.getComplicationBackgroundColor( );
   return col==0?defcol[2]:col;
   }
   /*
static int getTextBorderColor( ) {
   int col=Natives.getComplicationTextBorderColor( );
   return col==0?getBackgroundColor():col;
   } */
Bitmap getArrowValueBitmap(String value,long time,int index,float rate) {
   final var now = System.currentTimeMillis();
   if((now-time)>=tk.glucodata.Notify.glucosetimeout) {
       return getnovalue();
      }
   getArrowBitmap(rate);
//   glucosePaint.setTypeface(Typeface.defaultFromStyle(BOLD));
   glucosePaint.setTypeface(boldtype);
   glucosePaint.setColor(getBackgroundColor( ));
   setNumberBitmap(value,time, -1);
   glucosePaint.setColor(getTextColor());
 //  glucosePaint.setTypeface(Typeface.defaultFromStyle(NORMAL));
   glucosePaint.setTypeface(normaltype);
   setNumberBitmap(value,time, index);
   return glucoseBitmap;
   }


Bitmap previewbitmap() {
        String value;
        float rate;
        int index;
        final var glucose = Natives.lastglucose();
         var now = System.currentTimeMillis();
        long  time;
        if(glucose != null&&((time = glucose.time * 1000L)!=0L&&(now-time)<tk.glucodata.Notify.glucosetimeout)) {
            value = glucose.value;
            index = glucose.index;
            rate = glucose.rate;
        } else {
            value = (Applic.unit == 1)?"5.6": "101";
            time = now;
            rate = 1.0f;
            index = 0;
        }
	return getArrowValueBitmap( value, time, index, rate);
	}


static public void updateall() {
	ArrowValueDataSourceService.Companion.update();
	NumberDataSourceService.Companion.update();
	ArrowDataSourceService.Companion.update();
     	}

}
