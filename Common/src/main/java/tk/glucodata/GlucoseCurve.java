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

import android.app.Activity;
import android.content.Context;
import android.content.res.Configuration;
import android.content.res.ColorStateList;
import android.graphics.Color;
import android.graphics.Typeface;
import android.hardware.display.DisplayManager;
import android.opengl.GLSurfaceView;
import android.text.InputType;
import android.util.DisplayMetrics;
import android.util.TypedValue;
import android.view.GestureDetector;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.Button;

import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import java.util.Calendar;

import androidx.annotation.Keep;
import androidx.annotation.UiThread;
import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsControllerCompat;

import tk.glucodata.nums.numio;
import tk.glucodata.settings.Settings;

import static tk.glucodata.Layout.getMargins;
import static android.util.TypedValue.COMPLEX_UNIT_PX;
import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static java.lang.Math.abs;
import static java.lang.System.currentTimeMillis;
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.Applic.usedlocale;
import static tk.glucodata.BuildConfig.SiBionics;
import static tk.glucodata.Log.doLog;
import static tk.glucodata.MainActivity.REQUEST_BARCODE;
import static tk.glucodata.MainActivity.systembarBottom;
import static tk.glucodata.MainActivity.systembarLeft;
import static tk.glucodata.MainActivity.systembarRight;
import static tk.glucodata.MainActivity.systembarTop;
import static tk.glucodata.Natives.getInvertColors;
import static tk.glucodata.Natives.turnoffalarm;
import static tk.glucodata.NumberView.geteditview;
import static tk.glucodata.NumberView.geteditwearos;
import static tk.glucodata.NumberView.smallScreen;
import static tk.glucodata.RingTones.EnableControls;
import static tk.glucodata.settings.Settings.editoptions;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.util.getlabel;

public class GlucoseCurve extends GLSurfaceView {
Button summarybutton=null;
boolean statspresent=false;
@Keep
void summaryready() {
    statspresent=true;
    if(doLog) {Log.i(LOG_ID,"summaryready");};;
    Applic.RunOnUiThread(()-> {
       if(doLog) {
           Log.i(LOG_ID,"UIThread summaryready");
           }
        Button tmp= summarybutton;
        summarybutton=null;
        if(tmp!=null) {
            {if(doLog) {Log.i(LOG_ID,"set Visible");};};
            tmp.setVisibility(VISIBLE);
            tmp.bringToFront();
            }
        });
   if(doLog) {
           Log.i(LOG_ID,"end summaryready");
           }
    
    }

@Keep
void showsensorinfo(String text,long sensorptr) {
    Applic.RunOnUiThread(()-> {
//        bluediag.showsensorinfo(text,(MainActivity )getContext());
        Sensors.show((MainActivity )getContext(),text,sensorptr);
        });
    }
static View[] reopen=new View[8]; //6 needed
static int reopennr=0;
    //    SearchLayout search;
    Layout search;
    public Dialogs dialogs;
    private static final String LOG_ID = "GlucoseCurve";
static   public float smallfontsize;
    Calendar cal = Calendar.getInstance();
 final   private ScaleGestureDetector mScaleDetector;
 final   private GestureDetector mGestureDetector;
    static final int STEPBACK = 1;
    boolean waitnfc = false;
    MyRenderer render;
    static int height=0,width=0;
    private DisplayManager displayManager;
    private int lastDisplayRotation=-1;
    private final boolean reverseDefaultRotation;
    private final DisplayManager.DisplayListener displayListener=new DisplayManager.DisplayListener() {
        @Override public void onDisplayAdded(int displayId) {}
        @Override public void onDisplayRemoved(int displayId) {}
        @Override public void onDisplayChanged(int displayId) {
            final android.view.Display display=getDisplay();
            if(display!=null&&display.getDisplayId()==displayId)
                updateDisplayRotation(true);
            }
        };


NumberView  numberview= new NumberView();

Layout numcontrol=null;
private void removeSearchEditor() {
    saveSearchEditorState();
    numberview.hidekeyboard();
    editfocus.clearedittext(under);
    editfocus.clearedittext(above);
    editfocus.clearedittext(mealquantity);
    if(meallayout!=null) {
        removeContentView(meallayout);
        meallayout=null;
        }
    if(search!=null) {
        removeContentView(search);
        search=null;
        }
    under=null;
    above=null;
    mealingredient=null;
    mealquantity=null;
    searchspinner=null;
    searchspinadap=null;
    scansearch=null;
    historysearch=null;
    streamsearch=null;
    streamcalibratedsearch=null;
    historycalibratedsearch=null;
    fromtime=null;
    totime=null;
    }
void startsearch() {
if(!isWearable) {
    MainActivity activity = (MainActivity) getContext();
    if(searchcontrol!=null) {
    Natives.stopsearch();
    searchcontrol.setVisibility(View.GONE);
    }
    removeSearchEditor();
    search = getsearchlayout(activity);
    if(!smallScreen)
        numberview.showsearchkeyboard(activity,search);

    activity.setonback(()-> {
        activity.showui=false;
          activity.hideSystemUI();
        tk.glucodata.help.hidekeyboard(activity);
        removeSearchEditor();
        reopener();
    if(Menus.on)
        Menus.show(activity);

    } );
    }
    }



private final static void hidesave(View v) {
    if(v.getVisibility()==VISIBLE) {
        reopen[reopennr++]=v;
        v.setVisibility(INVISIBLE);
        }
    }

void setminheight(View[] views,int minheight) {
    for(View v:views)
        v.setMinimumHeight(minheight);
    }
//void getnumcontrol(MainActivity activity,int width,int height) {
void getnumcontrol(MainActivity activity) {
   {if(doLog) {Log.i(LOG_ID,"getnumcontrol start");};};

    if(numcontrol==null) {
           ImageButton first=new ImageButton(activity);
           first.setImageResource( R.drawable.baseline_first_page_24);
           first.setOnClickListener(v-> {
        Natives.firstpage();
                  requestRender();
           });
        first.setContentDescription("First number");

           ImageButton back=new ImageButton(activity);
           back.setImageResource( R.drawable.baseline_arrow_back_24);
           back.setOnClickListener(v-> {
        Natives.backwardnumlist();
                  requestRender();

           });
        back.setContentDescription("Page back");
            


     ImageButton search=new ImageButton(activity);
     search.setImageResource( android.R.drawable.ic_menu_search);
        search.setContentDescription("Search");
//     search.setImageResource( android.R.attr.actionModeWebSearchDrawable);

       search.setOnClickListener(v-> {
           hidesave(numcontrol);
        startsearch();
        selectnumbers();
        hidesave(scansearch);
        hidesave(streamsearch);
        hidesave(streamcalibratedsearch);
        hidesave(historycalibratedsearch);
        hidesave(historysearch);
           });
//    s/\(first[^6]*.6\)/(int)(\1)/g
    search.setPadding((int)(first.getPaddingLeft()*.69),(int)(first.getPaddingTop()*.69),(int)(first.getPaddingRight()*.69),(int)(first.getPaddingBottom()*.69));
     ImageButton closecontrol=new ImageButton(activity);

     closecontrol.setImageResource( android.R.drawable.ic_menu_close_clear_cancel);
           closecontrol.setOnClickListener(v-> {
           activity.doonback();
//           activity.poponback();
           });

    closecontrol.setContentDescription(activity.getString(R.string.closename));
    closecontrol.setPadding((int)(first.getPaddingLeft()*.69),(int)(first.getPaddingTop()*.69),(int)(first.getPaddingRight()*.69),(int)(first.getPaddingBottom()*.69));

           ImageButton next=new ImageButton(activity);

           next.setImageResource( R.drawable.baseline_arrow_forward_24);
        next.setContentDescription(getContext().getString(R.string.nextpage));
           next.setOnClickListener(v-> {
            Natives.forwardnumlist();
                 requestRender();

       });
           ImageButton last=new ImageButton(activity);
       last.setContentDescription("Last number");
           last.setImageResource( R.drawable.baseline_last_page_24);

           last.setOnClickListener(v-> {

        Natives.lastpage();
                  requestRender();
           });


        final View[] controls={first,
                back,
                search,
                closecontrol,
                next,
                last};
        final    int minheight=(int)(metrics.density*48.0f);
        setminheight(controls,minheight);
       numcontrol= new Layout(activity,(v,w,h) -> {
           final int width=getWidth();
           final int height=getHeight();
           final int columns=Natives.numcontrol(w,h);
           final int top=systembarTop;
           final int bottom=systembarBottom;

           int over;
           final float y;
           if(top>0) {
               y=top;
               over=Math.min(h,height-top);
               }
           else {
               y=height>h?(height-h)/2.0f:0.0f;
               over=h;
               }

           final float x;
           if(width>w) {
               if(columns==1)
                   x=width-w-systembarRight;
               else
                   x=(width-w-systembarRight+systembarLeft)/2.0f;
               }
           else
               x=0.0f;

           v.setX(x);
           v.setY(y);
           requestRender();
           over=Math.max(0,over-bottom);
           return new int[] {w,over};
          },new View[]{first}, new View[]{back}, new View[]{search},new View[]{closecontrol}, new View[]{next}, new View[]{last}
          );

//      numcontrol.setLayoutDirection(View.LAYOUT_DIRECTION_LTR);
           activity.addMyContentView(numcontrol, new ViewGroup.LayoutParams(WRAP_CONTENT,MATCH_PARENT));
        numcontrol.post(numcontrol::requestLayout);
       }
      else {
        numcontrol.setVisibility(VISIBLE);
        numcontrol.requestLayout();
        }
    activity.setonback(()-> {
        numcontrol.setVisibility(GONE);
        Natives.endnumlist();
         if(Menus.on) {
             Menus.show(activity);
             }
        else
             requestRender();
         });
   {if(doLog) {Log.i(LOG_ID,"getnumcontrol end");};};
    }

    void showkeyboard(MainActivity context) {
       numberview.showkeyboard(context);
        }
    void hidekeyboard() {
        numberview.hidekeyboard();
            }
        
    /*OnBackPressedCallback callback = new OnBackPressedCallback(true) {
        @Override
        public void handleOnBackPressed() {
            if ((render.stepresult & STEPBACK) == STEPBACK) {
                {if(doLog) {Log.d(LOG_ID,"GlucoseCurve: back");};};
                render.stepresult = 0;
//                ((MainActivity)getContext()).hideSystem=true;
                ((MainActivity)getContext()).hideSystemUI();
//                Natives.hidescanresults();
                requestRender();
            } else
                ((Activity) getContext()).finish();
        }
    };*/
static public DisplayMetrics metrics;
static public float getDensity() {
    if(metrics==null||metrics.density<=0.0f) {
        metrics= Applic.app.getResources().getDisplayMetrics();
        }
    return metrics.density;
    }
public GlucoseCurve(MainActivity context) {
    super(context);
    {if(doLog) {Log.i(LOG_ID,"GlucoseCurve "+MainActivity.openglversion);};};
    mScaleDetector = new ScaleGestureDetector(context, mScaleListener);
    final  GestureListener gestureListener = new GestureListener();
    mGestureDetector = new GestureDetector(context, gestureListener);
    final int reverseRotationId=context.getResources().getIdentifier("config_reverseDefaultRotation","bool","android");
    reverseDefaultRotation=reverseRotationId!=0&&context.getResources().getBoolean(reverseRotationId);
    render = new MyRenderer(this);
    setEGLContextClientVersion(MainActivity.openglversion);
    setEGLConfigChooser(8, 8, 8, 8, 16, 1);
    setRenderer(render);
    setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
    metrics= getResources().getDisplayMetrics();
    dialogs=new Dialogs(metrics.density);
    }
@Override
protected void onAttachedToWindow() {
    super.onAttachedToWindow();
    if(!isWearable) {
        displayManager=(DisplayManager)getContext().getSystemService(Context.DISPLAY_SERVICE);
        if(displayManager!=null)
            displayManager.registerDisplayListener(displayListener,null);
        updateDisplayRotation(false);
        }
    }
@Override
protected void onDetachedFromWindow() {
    if(displayManager!=null) {
        displayManager.unregisterDisplayListener(displayListener);
        displayManager=null;
        }
    super.onDetachedFromWindow();
    }
    public static int dpToPx(float dp) {
        return (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, dp, metrics);
    }

public static int getheight() {
    return height;
    }
public static int getwidth() {
    return width;
    }
public static int getheight(Context context) {
    int current=context.getResources().getDisplayMetrics().heightPixels;
    return current>0?current:height;
    }
public static int getwidth(Context context) {
    int current=context.getResources().getDisplayMetrics().widthPixels;
    return current>0?current:width;
    }
public static boolean isLandscape(Context context) {
    int orientation=context.getResources().getConfiguration().orientation;
    if(orientation==Configuration.ORIENTATION_LANDSCAPE)
        return true;
    if(orientation==Configuration.ORIENTATION_PORTRAIT)
        return false;
    return getwidth(context)>getheight(context);
    }
static void setgeo(int w,int h) {
    width=w;
    height=h;
    }

/*
 * Native and Java use the same graph transform code:
 *   0 identity
 *   1 counter-clockwise quarter turn
 *   2 half turn
 *   3 clockwise quarter turn
 *
 * rotateText=false is deliberately not tied to portrait: the transform also
 * becomes 180 degrees when Android rotates to the landscape side opposite to
 * settings->data()->orientation. Thus the NanoVG graph stays fixed relative to
 * the phone, while Android Views and non-graph NanoVG screens rotate normally.
 */
int currentDisplayRotation() {
    final android.view.Display display=getDisplay();
    return display==null?Surface.ROTATION_0:display.getRotation();
    }
void syncNativeDisplayRotation() {
    if(Applic.Nativesloaded)
        Natives.setDisplayRotation(currentDisplayRotation(),reverseDefaultRotation,
                MainActivity.graphLockedToFirstLandscape,
                MainActivity.graphUsesCurrentOrientationAsLandscape);
    }
void refreshNativeGeometry() {
    if(!Applic.Nativesloaded)
        return;
    final int w=getWidth();
    final int h=getHeight();
    if(w<=0||h<=0)
        return;
    queueEvent(()-> {
        syncNativeDisplayRotation();
        Natives.resize(w,h,Applic.initscreenwidth);
        });
    requestRender();
    requestOverlayLayout();
    }
private void updateDisplayRotation(boolean redraw) {
    final int rotation=currentDisplayRotation();
    if(rotation==lastDisplayRotation)
        return;
    lastDisplayRotation=rotation;
    syncNativeDisplayRotation();
    if(redraw)
        requestRender();
    }
private int graphRotationMode() {
    if(isWearable)
        return 0;

    syncNativeDisplayRotation();
    final int mode=Natives.getGraphRotationMode();
    // The separate entered-record list is not a graph. It always keeps the
    // Android surface coordinate system, independent of rotateText.
    final boolean numberListActive=numcontrol!=null&&numcontrol.getVisibility()!=GONE;
    return numberListActive?0:mode;
    }
private boolean graphQuarterTurn() {
    final int mode=graphRotationMode();
    return mode==1||mode==3;
    }
private float graphX(float physicalX,float physicalY) {
    switch(graphRotationMode()) {
        case 1: return getHeight()-physicalY;
        case 2: return getWidth()-physicalX;
        case 3: return physicalY;
        default:return physicalX;
        }
    }
private float graphY(float physicalX,float physicalY) {
    switch(graphRotationMode()) {
        case 1: return physicalX;
        case 2: return getHeight()-physicalY;
        case 3: return getWidth()-physicalX;
        default:return physicalY;
        }
    }
private float graphDistanceX(float physicalDistanceX,float physicalDistanceY) {
    switch(graphRotationMode()) {
        case 1: return -physicalDistanceY;
        case 2: return -physicalDistanceX;
        case 3: return physicalDistanceY;
        default:return physicalDistanceX;
        }
    }
private float graphDistanceY(float physicalDistanceX,float physicalDistanceY) {
    switch(graphRotationMode()) {
        case 1: return physicalDistanceX;
        case 2: return -physicalDistanceY;
        case 3: return -physicalDistanceX;
        default:return physicalDistanceY;
        }
    }
private void requestOverlayLayoutNow() {
if(!isWearable) {
    if(search!=null)
        search.requestLayout();
    if(meallayout!=null)
        meallayout.requestLayout();
    if(searchcontrol!=null)
        searchcontrol.requestLayout();
    if(numcontrol!=null)
        numcontrol.requestLayout();
    numberview.requestOverlayLayout();
    }
    }
private final Runnable overlayRelayout=this::requestOverlayLayoutNow;
void requestOverlayLayout() {
        if(!isWearable) {
            requestOverlayLayoutNow();
            removeCallbacks(overlayRelayout);
            post(overlayRelayout);
            }
    }
void configurationChanged(MainActivity activity) {
if(!isWearable) {
    numberview.configurationChanged(activity);
    updateDisplayRotation(true);
    requestOverlayLayout();
    }
    }
long multitime=0L;
    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if(turnoffalarm())
        Notify.stopalarm();
        if((render.stepresult&STEPBACK)!=0) {
            final float x = graphX(event.getX(),event.getY());
            final float y = graphY(event.getX(),event.getY());
    
            if(Natives.isbutton(x, y)) {
                render.badscan=0;
                if(Menus.on)
                    Menus.show((MainActivity)getContext());
                else
                    requestRender();
            }
            return false;
        }

        if (event.getPointerCount() > 1) {
        multitime=System.currentTimeMillis();
            return mScaleDetector.onTouchEvent(event);
            }
        else
            return mGestureDetector.onTouchEvent(event);

    }

    boolean down = false;
final    private ScaleGestureDetector.SimpleOnScaleGestureListener mScaleListener = new ScaleGestureDetector.SimpleOnScaleGestureListener() {
        float focusx;

        @Override
        public boolean onScaleBegin(ScaleGestureDetector detector) {
            switch(graphRotationMode()) {
                case 1: focusx=getHeight()-detector.getFocusY(); break;
                case 2: focusx=getWidth()-detector.getFocusX(); break;
                case 3: focusx=detector.getFocusY(); break;
                default:focusx=detector.getFocusX(); break;
                }
            return true;
        }

        @Override
        public void onScaleEnd(ScaleGestureDetector detector) {


        }

        @Override
        public boolean onScale(ScaleGestureDetector detector) {
            final boolean quarterTurn=graphQuarterTurn();
            float scalex = quarterTurn
                    ? detector.getCurrentSpanY()/detector.getPreviousSpanY()
                    : detector.getCurrentSpanX()/detector.getPreviousSpanX();
      {if(doLog) {Log.i(LOG_ID,"onScale SpanX="+detector.getCurrentSpanX()+" PreviousSpanX="+ detector.getPreviousSpanX()+" scalex="+scalex);};};
            Natives.xscale(scalex, focusx);
            requestRender();
            down = false;
            return true;
        }
    };

    long reldate;
     void startdatepick(long tim) {
        reldate=tim;
        numberview.getdateviewal((MainActivity)getContext(),tim,    (year,month,day)-> {
            Natives.movedate(reldate, year, month, day);
            requestRender();
        });

    }
class GestureListener implements GestureDetector.OnGestureListener {
        @Override
        public boolean onDown(MotionEvent e) {
            down = true;
            return true;
        }

        @Override
        public void onShowPress(MotionEvent e) {

        }


      /*
void startlibrelink(String lang) {
    Activity act = (Activity) getContext();
    ComponentName cn = new ComponentName("com.freestylelibre.app."+lang, "com.librelink.app.ui.SplashActivity");
    //                            ComponentName cn = new ComponentName("com.freestylelibre.app.nl","com.librelink.app.ui.common.ScanSensorActivity");
    Intent infoIntent = new Intent();
    infoIntent.setComponent(cn);
    infoIntent.setAction("android.intent.action.MAIN");
    act.startActivity(infoIntent);
}
*/
//GarminStatus status=null;
//bluediag bluestatus=null;
static private boolean startv1=true;
        @UiThread
        @Override
        public boolean onSingleTapUp(MotionEvent event) {
            {if(doLog) {Log.d(LOG_ID,"onSingleTapUp");};};
            if (down ) {
                final float x=graphX(event.getX(),event.getY());
                final float y=graphY(event.getX(),event.getY());
                long choice = Natives.tap(x, y);
                if(choice==-2L) 
                    return true;
                if(choice!=-1L) {
                    int menu = (int) (choice & 0xf);
                    int item = (int) (choice >> 4);
                    {if(doLog) {Log.i(LOG_ID,"menu="+menu+" item="+item);};};
                switch(menu) {
                     case 0:
                        switch (item) {
                            case 0: ((MainActivity) getContext()).selectionSystemUI(); break;
                            case 1: Menus.show((MainActivity) getContext());break;
                            case 2: {
                            MainActivity activity = (MainActivity) getContext();
                            if(!isWearable) {
                                tk.glucodata.Watch.show(activity);
                                }
                            else {
                                }
                                tk.glucodata.Display.show(activity);
                                }

                                break;
                              case 3: 

                                bluediag.start((MainActivity)getContext()); 
                                  break;
                              case 4: {
                                MainActivity activity = (MainActivity) getContext(); 
                                Settings.set(activity);
                                };break;

                            case 5: {
                                if(!isWearable) {
                                    MainActivity activity = (MainActivity) getContext();
                                    if(SiBionics==1)
                                        PhotoScan.scan(activity,REQUEST_BARCODE);
                                    else
                                        doabout(activity);
                                    }


                                break;
                                }
                                            case 6: ((Activity) getContext()).moveTaskToBack(true);break; //keeps current state 
                                            case 7:  Notify.stopalarm();break;
                                            default:
                                    }

                            break;
             case 1: {
                switch(item&0xF) {
                                    case 0: dialogs.showexport(( MainActivity)getContext(),getWidth(),getHeight(),null); break;


                   case 1: (new Backup()).mkbackupview(( MainActivity)getContext());break;
                case 2: {
                       MainActivity activity = (MainActivity) getContext();
                    if(Natives.staticnum()) {
                  if(isWearable)
                     Specific.blockedNum(activity);
                  else   {
                           activity.themeLightBars();
                            help.help(R.string.staticnum,activity,l->activity.lightBars(!Natives.getInvertColors( ))); 
                            }
                        }
                    else {
                        numberview.addnumberview(activity);
                        if(!smallScreen)
                            showkeyboard(activity);
                        }
                    }; break;
                case 3: getnumcontrol((MainActivity) getContext());return true;
                case 4: Stats.mkstats((MainActivity) getContext());break;
                case 5: tk.glucodata.Talker.config((MainActivity) getContext());break;
                case 6:  Floating.setfloatglucose((MainActivity) getContext(),!Natives.getfloatglucose()) ;break;
                };
                };break;
            case 2: {
                var light=item==0;
                var main=(MainActivity) getContext();
                main.lightBars(light);
                };break;
            case 3:
                switch (item) {
                    case 1: startsearch();
                                    break;
                    case 2:
                                startdatepick(Natives.getstarttime());
                                    break;
                            };break;
                    case 0xe: {
              if(reopennr>0)
                  return true;
                MainActivity act = (MainActivity) getContext();
                               int pos=(int)(choice>>16);
                            int base =(int)((choice>>8)&0xF);
                            {if(doLog) {Log.i(LOG_ID,"tap pos="+pos+" base="+base);};};
                if(numcontrol!=null) hidesave(numcontrol);
                numberview.addnumberview(act, base, pos) ;
                if(!Natives.staticnum()) {
                    if(!smallScreen)
                        numberview.showkeyboard(act);
                    }
                };
                return true;
                        default:
                    }
                }
            requestRender();
            return true;
        }
            return false;
}



        @Override
        public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {
//          {if(doLog) {Log.i(LOG_ID,"onScroll dX="+distanceX+" dY="+distanceY);};};
        if(down) {
            if((render.stepresult&STEPBACK)==0)  {
                if(e1.isFromSource(InputDevice.SOURCE_MOUSE) && e1.isButtonPressed(MotionEvent.BUTTON_PRIMARY) && (e1.getMetaState() & KeyEvent.META_CTRL_ON)==KeyEvent.META_CTRL_ON){
                   final float logicalDistanceX=graphDistanceX(distanceX,distanceY);
                   final float oldx=graphX(e1.getX(),e1.getY());
                   final float newx=graphX(e2.getX(),e2.getY());
                   if(Natives.mouseScale(logicalDistanceX,oldx,newx)!=0)
                        requestRender();
                   }
                else {
                    final float logicalDistanceX=graphDistanceX(distanceX,distanceY);
                    final float logicalDistanceY=graphDistanceY(distanceX,distanceY);
                    final float oldy=graphY(e1.getX(),e1.getY());
                    final float newy=graphY(e2.getX(),e2.getY());
                    if(Natives.translate(logicalDistanceX,logicalDistanceY,oldy,newy)!=0)
                        requestRender();
                        }
                    }
            return true;
            }
        return false;
        }


        @Override
        public void onLongPress(MotionEvent event) {
            {if(doLog) {Log.d(LOG_ID,"OnLongPress" + (down?"":" not") + " down");};};
        if(down) {
        long nutime=System.currentTimeMillis();
        if((nutime-multitime)<1000)
            return;
                final float wgrens=smallfontsize*3;
                final float x=graphX(event.getX(),event.getY());
                final float y=graphY(event.getX(),event.getY());
                final float rgrens=(graphQuarterTurn()?getHeight():getWidth())-wgrens;
            if(x<wgrens) {
                Natives.prevday(1);
                }
            else {
                if(x>rgrens) {
                    Natives.nextday(1);
                    }
                else {
                    long hitptr=Natives.longpress(x, y);
                   if(hitptr!=0) {
                if((hitptr&3)!=0) {
                 return;
                 }
            else {
                 MainActivity activity = (MainActivity) getContext();
                if(Natives.staticnum()&&hitptr== numio.newhit) {
                    help.help(R.string.staticnum,activity);
                    }
                else {
                    numberview.addnumberview(activity,hitptr);
                    if(!Natives.staticnum()) {
                        if(!smallScreen) {
                            showkeyboard(activity);
                            }
                        }
                    }
                }
                }
                    }
                }

                requestRender();
            }

        }
        /*
        @Override
        public boolean    onDoubleTap(MotionEvent e) {
            Log.d(LOG_ID,"public boolean    onDoubleTap(MotionEvent e) {");return false;

        }

        @Override
        public boolean    onDoubleTapEvent(MotionEvent e) {
            Log.d(LOG_ID,"public boolean    onDoubleTapEvent(MotionEvent e) {");return false;

        }

        @Override
        public   boolean    onSingleTapConfirmed(MotionEvent e) {
            Log.d(LOG_ID,"public   boolean    onSingleTapConfirmed(MotionEvent e) {");return false;
        }
*/
        @Override
        public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {{if(doLog) {Log.d(LOG_ID,"onFling");};};
            // {if(doLog) {Log.i(LOG_ID,"onFling volX="+velocityX+"volY="+velocityY);};};
            if(down) {
                final int rotation=graphRotationMode();
                final float logicalVelocityX;
                final float logicalVelocityY;
                switch(rotation) {
                    case 1: logicalVelocityX=-velocityY; logicalVelocityY= velocityX; break;
                    case 2: logicalVelocityX=-velocityX; logicalVelocityY=-velocityY; break;
                    case 3: logicalVelocityX= velocityY; logicalVelocityY=-velocityX; break;
                    default:logicalVelocityX= velocityX; logicalVelocityY= velocityY; break;
                    }
                float absx=abs(logicalVelocityX);
                if(absx>2000.0&&absx>abs(logicalVelocityY)) {
                    Natives.flingX(logicalVelocityX);
                    requestRender();
                }
                return true;
            }
    return false;
        }
    };//end class GestureListener 



private int[] minutes={-1,-1};


static String mktime(int hour,int min) {
     if(Applic.hour24)  {
                return String.format(usedlocale,"%02d:%02d", hour, min);
        }
    else {
            var daypart = (hour >= 12)?"pm":"am";
            var hour12 = hour % 12;
            if(hour12 == 0) hour12 = 12;
            return   String.format(usedlocale,"%d:%02d%s", hour12, min,daypart);
        }
    }

private void mktimedialog( Button but,final int num ,View parent) {
        but.setOnClickListener(
                v->  {
        parent.setVisibility(GONE);
        var keys=numberview.keyboard;
        if(keys!=null) {
            keys.setVisibility(INVISIBLE);
            }
        int starthour,startmin;
        if(minutes[num]>=0) {
            starthour=minutes[num]/60;
            startmin=minutes[num]%60;
            }
        else {
            cal.setTimeInMillis(currentTimeMillis());
             starthour=cal.get(Calendar.HOUR_OF_DAY);
             startmin=cal.get(Calendar.MINUTE);
             }
        numberview.gettimepicker((MainActivity)getContext(),starthour, startmin,
        (hour,min) -> {
            TextView text=((TextView) v);
                        text.setText(mktime( hour, min));
//            v.setBackgroundColor(Color.RED);
            text.setTextColor( Color.RED);
            text.setTypeface( null,Typeface.BOLD);
            text.setTextSize(COMPLEX_UNIT_PX,oldsize*1.5f);
                          minutes[num] = hour * 60 + min;
           },()-> {
            parent.setVisibility(VISIBLE);
                        if(keys!=null) {
                            keys.setVisibility(VISIBLE);
                        }


                    });
        });
    }


    EditText under,above;

    // Search editor Views are intentionally destroyed after starting/leaving a search.
    // Keep the criteria separately so reopening the editor continues with the same search.
    private boolean searchStateValid=false;
    private String savedSearchUnder="0";
    private String savedSearchAbove="999";
    private String savedMealIngredient="";
    private String savedMealQuantity="";
    private boolean savedScanSearch=false;
    private boolean savedHistorySearch=false;
    private boolean savedStreamSearch=true;
    private boolean savedStreamCalibratedSearch=false;
    private boolean savedHistoryCalibratedSearch=false;
    private int savedSearchLabel=-1;

    private void saveSearchEditorState() {
        if(under==null||above==null)
            return;

        savedSearchUnder=under.getText().toString();
        savedSearchAbove=above.getText().toString();
        savedSearchLabel=labelsel;

        if(scansearch!=null)
            savedScanSearch=scansearch.isChecked();
        if(historysearch!=null)
            savedHistorySearch=historysearch.isChecked();
        if(streamsearch!=null)
            savedStreamSearch=streamsearch.isChecked();
        if(streamcalibratedsearch!=null)
            savedStreamCalibratedSearch=streamcalibratedsearch.isChecked();
        if(historycalibratedsearch!=null)
            savedHistoryCalibratedSearch=historycalibratedsearch.isChecked();

        // Keep the meal criteria even when the meal row is currently hidden.
        if(mealingredient!=null)
            savedMealIngredient=mealingredient.getText().toString();
        if(mealquantity!=null)
            savedMealQuantity=mealquantity.getText().toString();

        searchStateValid=true;
    }

    private void setSearchTimeButton(Button button,int minute) {
        if(minute<0) {
            if(Applic.hour24)
                button.setText(button==fromtime?"00:00":"23:59");
            else
                button.setText(button==fromtime?"12:00am":"12:59pm");
            button.setTextColor(oldColors);
            button.setTextSize(COMPLEX_UNIT_PX,oldsize);
            button.setTypeface(null,Typeface.NORMAL);
        }
        else {
            button.setText(mktime(minute/60,minute%60));
            button.setTextColor(Color.RED);
            button.setTextSize(COMPLEX_UNIT_PX,oldsize*1.5f);
            button.setTypeface(null,Typeface.BOLD);
        }
    }

    private void restoreSearchEditorState(MainActivity context) {
        under.setText(savedSearchUnder);
        above.setText(savedSearchAbove);

        scansearch.setChecked(savedScanSearch);
        historysearch.setChecked(savedHistorySearch);
        streamsearch.setChecked(savedStreamSearch);
        streamcalibratedsearch.setChecked(savedStreamCalibratedSearch);
        historycalibratedsearch.setChecked(savedHistoryCalibratedSearch);

        int last=searchspinner.getCount()-1;
        int selection=savedSearchLabel;
        if(selection<0||selection>last)
            selection=last;
        labelsel=selection;
        searchspinner.setSelection(selection);

        if(selection==Natives.getmealvar()) {
            mkmealsearch(context);
            if(mealingredient!=null)
                mealingredient.setText(savedMealIngredient);
            if(mealquantity!=null)
                mealquantity.setText(savedMealQuantity);
        }

        setSearchTimeButton(fromtime,minutes[0]);
        setSearchTimeButton(totime,minutes[1]);

        under.requestFocus();
        editfocus.setedittext(under);
    }

    ImageButton prev=null,next=null;
   void searchaway() {
       removeSearchEditor();
     if(searchcontrol!=null)
         searchcontrol.setVisibility(GONE);
       reopener();
       Natives.stopsearch();
       requestRender();
   }
static void reopener() {
    for(int i=0;i<reopennr;i++) {
        if(reopen[i]!=null)
            reopen[i].setVisibility(VISIBLE);
        reopen[i]=null;
        }
    reopennr=0;
    }
int labelsel=-1;
void clearsearch(View view) {
if(!isWearable) {
    if(mealquantity!=null) {
         mealquantity.setText("");
         mealingredient.setText("");
         }
    savedMealIngredient="";
    savedMealQuantity="";
      under.setText("0");
        above.setText("999");
        labelsel=searchspinner.getCount()-1;
        searchspinner.setSelection(labelsel);
        scansearch.setChecked(false);
        streamsearch.setChecked(true);
        streamcalibratedsearch.setChecked(false);
        historycalibratedsearch.setChecked(false);
        historysearch.setChecked(false);
    if(Applic.hour24)  {
        fromtime.setText("00:00");
        totime.setText("23:59");
        }
    else {
        fromtime.setText("12:00am");
        totime.setText("12:59pm");
        }
    fromtime.setTextColor(oldColors);
    totime.setTextColor(oldColors);
    totime.setTextSize(COMPLEX_UNIT_PX,oldsize);
    fromtime.setTextSize(COMPLEX_UNIT_PX,oldsize);
    fromtime.setTypeface(null,Typeface.NORMAL);
    totime.setTypeface(null,Typeface.NORMAL);
        minutes[0]=-1;
        minutes[1]=-1;

//    above.requestFocus();
    under.requestFocus();
    editfocus.setedittext(under);
    }
    }
View searchcontrol=null;
//    void search(View view) {
void search(boolean forward) {
    ((MainActivity)getContext()).hideSystemUI();
    if(smallScreen) {
        help.hidekeyboard((MainActivity)getContext());
        }
        float funder=0.0f;
        try {
                 funder= Float.parseFloat(under.getText().toString());
            } catch(Exception e) {};
        float fabove=0.0f;
        try {
                 fabove= Float.parseFloat(above.getText().toString());
            } catch(Exception e) {};
        float ingamount=0.0f;
        try {
            if(mealquantity!=null)
                ingamount= Float.parseFloat(mealquantity.getText().toString());
            } catch(Exception e) {};
        String ingsearch=null;
        if(mealingredient!=null) {
            ingsearch=mealingredient.getText().toString();
            if(ingsearch.length()==0)
                ingsearch=null;
                }
           int glsearch=((historysearch.isChecked()?0x40000002:0)| (scansearch.isChecked()?0x40000001:0))|(streamsearch.isChecked()?0x40000004:0)| (streamcalibratedsearch.isChecked()?0x40000008:0)|(historycalibratedsearch.isChecked()?0x40000010:0);


       if(Natives.search(glsearch==0?labelsel:glsearch,funder,fabove,minutes[0],minutes[1],forward,ingsearch,ingamount)==0) {

           removeSearchEditor();
           requestRender();
            MainActivity activity=(MainActivity)getContext();
        activity.poponback();
           if(searchcontrol==null) {

            /*  prev= new Button(activity); prev.setText("Earlier");*/

               prev=new ImageButton(activity);
               prev.setImageResource( R.drawable.baseline_navigate_before_white_36);
               prev.setOnClickListener(v-> {
                   if (0 == Natives.earliersearch()) {
                       next.setVisibility(VISIBLE);
                        requestRender();
                     }
                    else {
                       if (next.getVisibility() != VISIBLE)
                           searchaway();
                       else
                           v.setVisibility(INVISIBLE);
                    }
            });

    //           next= new Button(activity); next.setText("Later");
               next=new ImageButton(activity);
               next.setImageResource( R.drawable.baseline_navigate_next_white_36);
               next.setOnClickListener(v-> {if(0== Natives.latersearch()) {
                   prev.setVisibility(VISIBLE);
                   requestRender();
               }else
                   {
                    if(prev.getVisibility()!=VISIBLE)
                        searchaway();
                    else
                                v.setVisibility(INVISIBLE);
                   }
               });
         ImageButton closecontrol=new ImageButton(activity);
         closecontrol.setImageResource( android.R.drawable.ic_menu_close_clear_cancel);
             closecontrol.setOnClickListener(v-> {
             activity.poponback();
                    searchaway();
                     });
        int closepad= (int)(closecontrol.getPaddingLeft()*.67);
        int pad= (int)(closecontrol.getPaddingLeft()*.5);
        closecontrol.setPadding(closepad,closepad,closepad,closepad);
        next.setPadding(pad,pad,pad,pad);
        prev.setPadding(pad,pad,pad,pad);
        searchcontrol= new Layout(activity,(v,w,h) -> {
           int height=getHeight();
           int width=getWidth()-systembarLeft-systembarRight;
           int columns=Natives.getcolumns(w);
           if(height>h && width>w) {
               v.setY((height-h)*.5f);
               if(columns==1)  {
                   v.setX(width-w+systembarLeft);
                }
               else {
                   v.setX((width-w)*.5f+systembarLeft);
                   }
            }
          requestRender();
          return new int[] {w,h};
          },new View[]{prev},new View[] {closecontrol},new View[] {next});
               activity.addMyContentView(searchcontrol, new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));
             
          searchcontrol.post(searchcontrol::requestLayout);
           }

           prev.setVisibility(VISIBLE);
           next.setVisibility(VISIBLE);
           searchcontrol.setVisibility(VISIBLE);
           activity.setonback(this::searchaway);
           }
           else
        Applic.argToaster(getContext(), getContext().getString(R.string.notfound), Toast.LENGTH_SHORT);
        //((MainActivity)getContext()).curve.requestRender();
}

 
// Disable spell check (hex strings look like words to Android)

//CheckDirectionRadio numbers;

    CheckDirectionBox scansearch,historysearch,streamsearch,streamcalibratedsearch, historycalibratedsearch;

 

    Button fromtime, totime;

//https://gist.github.com/kakajika/a236ba721a5c0ad3c1446e16a7423a63
    /*
void radiolisten( CheckDirectionRadio one,CheckDirectionRadio other) {
         one.setOnClickListener(v-> {
             ((CheckDirectionRadio) v).setChecked(true);
             other.setChecked(false);
             if(numbers.isChecked())
                 spinner.setVisibility(VISIBLE);
             else
                 spinner.setVisibility(GONE);

         });
}
*/
void selectnumbers() {
            scansearch.setChecked(false);
            historysearch.setChecked(false);
            streamsearch.setChecked(false);
            streamcalibratedsearch.setChecked(false);
            historycalibratedsearch.setChecked(false);
       //     spinner.setVisibility(VISIBLE);
        }
void glucoselisten(CompoundButton one) {
    one.setOnClickListener(v -> {
        if(historysearch.isChecked()||scansearch.isChecked()||streamsearch.isChecked()||streamcalibratedsearch.isChecked() ||historycalibratedsearch.isChecked()) {

            labelsel=searchspinner.getCount()-1;
            searchspinner.setSelection(labelsel);
            }
    });
}
Spinner searchspinner;
LabelAdapter<String> searchspinadap;
Spinner getsearchspinner(MainActivity context) {
if(searchspinner==null) {
    /*
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
        searchspinner=new Spinner(context, null,0,R.style.MySpinnerStyle , -1);
    }
    else */
        searchspinner=new Spinner(context);
//        searchspinner=new Spinner(context, null,R.style.MySpinnerStyle );
//        searchspinner=new Spinner(context,R.style.spinner_style);
//        searchspinner=new Spinner(context,R.style.MySpinnerStyle2);
/*
    int minheight=GlucoseCurve.dpToPx(48);
    searchspinner.setMinimumHeight(minheight);
*/
    searchspinner.setContentDescription("Amount type selector");
   NumberView.avoidSpinnerDropdownFocus(searchspinner);
    searchspinadap= new LabelAdapter<String>(context,Natives.getLabels(),0);
        searchspinner.setAdapter(searchspinadap);
    searchspinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
        @Override
        public  void onItemSelected (AdapterView<?> parent, View view, int position, long id) {
       {if(doLog) {Log.i(LOG_ID,"onItemSelected "+position);};};
            if(position!=(searchspinner.getCount()-1)) {
                selectnumbers();
        if(position==Natives.getmealvar()) {
            mkmealsearch(context);
            }
        else
            hidemealsearch();
        }
        else
        hidemealsearch();
         labelsel=position;
        }
        @Override
        public  void onNothingSelected (AdapterView<?> parent) {
            labelsel=searchspinner.getCount()-1;

        } });
    searchspinner.clearAnimation();
    }
return searchspinner;

}
ColorStateList oldColors;
float oldsize;
Layout meallayout=null;
void hidemealsearch() {
    if(meallayout!=null) 
        meallayout.setVisibility(GONE);
    }
EditText mealingredient=null,mealquantity=null;
void mkmealsearch(MainActivity act) {
    if(meallayout==null) {
        var inglabel=getlabel(act,R.string.ingredient);
        var qualabel=getlabel(act,R.string.minimum);
        mealingredient=new EditText(act);
        mealingredient.setInputType(InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD);
        mealingredient.setImeOptions(editoptions);
        mealingredient.setOnFocusChangeListener(new editUIfocus());
        /*
    TextView.OnEditorActionListener     actlist= new TextView.OnEditorActionListener() {
            @Override
            public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
            if (event != null && event.getKeyCode() == KeyEvent.KEYCODE_ENTER
                    || actionId == EditorInfo.IME_ACTION_DONE) {
                act.hideSystemUI();
                 tk.glucodata.help.hidekeyboard(act);
                 {if(doLog) {Log.i(LOG_ID,"onEditorAction");};};
// hidekeyboard();
                return true;
               }
            return false;
            }};
        mealingredient.setOnEditorActionListener(actlist);
            */

//        mealingredient.setMinEms(10);
        ViewGroup.LayoutParams params= new ViewGroup.LayoutParams(  MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        mealingredient.setLayoutParams(params);
        if(smallScreen) {
            mealquantity=geteditwearos(act);
            }
        else
            mealquantity=geteditview(act,new editclosefocus());
        mealquantity.setMinEms(3);
        if(searchStateValid) {
            mealingredient.setText(savedMealIngredient);
            mealquantity.setText(savedMealQuantity);
        }
        int pad= (int)(tk.glucodata.GlucoseCurve.metrics.density*5);
             inglabel.setPadding(pad,0,pad,0);
        qualabel.setPadding(pad,0,pad,0);

//        mealquantity.setPadding(0,0,0,(int)metrics.density*8);
//        mealquantity.setMinEms(2);
//        mealingredient.setImeOptions(EditorInfo.IME_FLAG_NO_EXTRACT_UI| EditorInfo.IME_FLAG_NO_FULLSCREEN| EditorInfo.IME_ACTION_SEARCH);
//        mealquantity.setOnEditorActionListener(actlist);
      Log.i(LOG_ID,"systembarTop="+systembarTop);
        meallayout=new Layout(act,
(lay, w, h)->{
            lay.setY(systembarTop*4/5);
            lay.setX(systembarLeft);

        return new int[] {w- systembarLeft- systembarRight,h};
        },

      //pick.setPadding(MainActivity.systembarLeft,MainActivity.systembarTop,0,MainActivity.systembarBottom);
      //buttonlay.setPadding(0,MainActivity.systembarTop/2,MainActivity.systembarRight,MainActivity.systembarBottom);


        new View[] {inglabel,mealingredient,qualabel,mealquantity});
        act.addMyContentView(meallayout, new ViewGroup.LayoutParams(MATCH_PARENT,WRAP_CONTENT));
            meallayout.setBackgroundColor(Applic.backgroundcolor);
        }
    else {
        meallayout.setVisibility(VISIBLE);
        meallayout.bringToFront();
        }
    

    }

private Layout getsearchlayout(MainActivity context) {
if(isWearable)
return null;
else {
    editfocus focus=new editfocus();
        if(smallScreen) {
            under=geteditwearos(context);
            }
        else
              under= geteditview(context,focus);
    int editwidth=(int)( metrics.density*48.0);
    under.setMinWidth(editwidth);


    TextView line=new TextView(context);line.setText("-");;

    if(smallScreen) {
            above=geteditwearos(context);
            }
        else
            above= geteditview(context,focus);
    above.setMinWidth(editwidth);
    scansearch=new CheckDirectionBox(context); scansearch.setText(R.string.scansname);
    final String historystr=context.getString(R.string.historyname);
    final String streamstr=context.getString(R.string.streamname);
    final String calibrated=context.getString(R.string.calibrated);
     historysearch=new CheckDirectionBox(context); historysearch.setText(historystr);
     streamsearch=new CheckDirectionBox(context); streamsearch.setText(streamstr);
     streamcalibratedsearch=new CheckDirectionBox(context); streamcalibratedsearch.setText(streamstr+" "+calibrated);
     historycalibratedsearch=new CheckDirectionBox(context); historycalibratedsearch.setText(historystr+" "+calibrated);
     
        glucoselisten(scansearch) ;
        glucoselisten(historysearch) ;
        glucoselisten(streamsearch) ;
        glucoselisten(streamcalibratedsearch) ;
        glucoselisten(historycalibratedsearch) ;
        var hormarg=(int)metrics.density*10;
    getMargins(scansearch).setMarginEnd(hormarg);
    getMargins(historycalibratedsearch).setMarginEnd(hormarg);
    getMargins(streamcalibratedsearch).setMarginEnd(hormarg);

    fromtime =new Button(context); //fromtime.setText("00:00");
    TextView gline=new TextView(context);gline.setText(" - ");

      totime=new Button(context); //totime.setText("23:59"); 
      oldColors=totime.getTextColors();
      oldsize=totime.getTextSize();
        Button clear=new Button(context);clear.setText(R.string.resetname);

    ImageButton helpbut=new ImageButton(context);

        helpbut.setImageResource( android.R.drawable.ic_menu_help);

    helpbut.setContentDescription(getContext().getString(R.string.helpname));
        helpbut.setOnClickListener(v-> { 
            context.themeLightBars();
            help.help(R.string.searchhelp,context,l->context.lightBars(!getInvertColors( ))); 
        });

    Button cancel=new Button(context);cancel.setText(R.string.cancel);



    ImageButton backward=new ImageButton(context);

    backward.setContentDescription(getContext().getString(R.string.searchtopast));
        backward.setImageResource( R.drawable.baseline_navigate_before_white_36);

        backward.setOnClickListener(v-> {
        search(false);} );

        ImageButton forward=new ImageButton(context);

    forward.setContentDescription("Search forward");

        forward.setImageResource( R.drawable.baseline_navigate_next_white_36);
        forward.setOnClickListener(v-> {
        search(true) ;});


    Layout layout=new Layout(context,(lay, w, h)->{
    int width=GlucoseCurve.getwidth(context);

    int height=GlucoseCurve.getheight(context);
    int ymax=height-MainActivity.systembarBottom-h;
    int ypos=(int)((height - h) *.65f);
    if(ypos>ymax) {
        if(ymax<0)
            ypos=0;
        else
            ypos=ymax;
        }
if(!smallScreen) {
//    boolean rtl=Natives.getRTL();
    if(height>h&&width>w) {
           if(GlucoseCurve.isLandscape(context)) {
                lay.setY(ypos);
//                lay.setY((height - h) / 2);
                int posx=numberview.searchLandscapeX(width,w);
                lay.setX(posx);
               {if(doLog) {Log.i(LOG_ID,"search h="+h+" height="+height+" w="+w+" width="+width+" posx="+posx);};};
                }
            else {
                int half=height/2;
                int af=(half-h)/4;
                 var xpos= (width - w)/2;
                lay.setX(xpos);
                lay.setY(Math.max(systembarTop,half-h-af));
                {if(doLog) {Log.i(LOG_ID,"search h="+h+" height="+height+" w="+w+" width="+width+" posx="+xpos);};};
            }
        }
    else {
        w=width;h=height;
        lay.setX(0);
//        lay.setY(0);
        }
        }
    else {
 //     final var ypos=(int)((height-h)/2.5f);
  //      lay.setY(ypos);
      final var xpos=width>w?(width - w)/2:0;
        lay.setX(xpos);
      {if(doLog) {Log.i(LOG_ID,"smallScreen search h="+h+" height="+height+" w="+w+" width="+width+" posx="+xpos+" posy="+ypos);};};
        }
        return new int[] {w,h};
        }, new Object[]{clear,new View[]{under,line,above}},new View[]{getsearchspinner(context),scansearch},new View[]{historysearch,historycalibratedsearch},new View[]{streamsearch,streamcalibratedsearch},new View[]{fromtime,totime,helpbut},new Object[] {new View[]{backward,cancel, forward}});

         mktimedialog( fromtime,0 ,layout);
      mktimedialog( totime,1 ,layout);
    context.addMyContentView(layout, new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));
        layout.setBackgroundColor(Applic.backgroundcolor);

            
        cancel.setOnClickListener(v -> {
        context.doonback();
        } );
    
    clear.setOnClickListener(this::clearsearch );
    if(searchStateValid)
        restoreSearchEditorState(context);
    else
        clearsearch(clear);
     layout.post(layout::requestLayout);
//       editfocus.setedittext(under);

    return layout;
    }
}


//Editable edit;



@Override
public void onResume() {
    {if(doLog) {Log.i(LOG_ID,"onResume()");};};
    super.onResume();
    Applic app = Applic.app;

    app.setcurve(this);
    app.setmintime();

    if(!isWearable) {
       if(SiBionics==1)  {
            if(MainActivity.tocalendarapp) {
                final String name=Natives.getUsedSensorName();
                if(name!=null) {
                    ScanNfcV.calendar((MainActivity)getContext(), 0, name);
                    MainActivity.tocalendarapp=false;
                    }
                }
            }
       }
    }

@Override
public void onPause() {
    {if(doLog) {Log.i(LOG_ID,"onPause()");};};
     Applic app = Applic.app;
     app.cancelmintime();
     app.setcurve(null);
     super.onPause();
    }
@Override
public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
    {if(doLog) {Log.i(LOG_ID,"surfaceChanged format="+format+", width="+w+", height="+h);};};
    setgeo(w,h);
    super.surfaceChanged(holder,format,w,h);
    requestOverlayLayout();
    }
@Override
public void surfaceCreated(SurfaceHolder holder) {
    {if(doLog) {Log.i(LOG_ID,"surfaceCreated(SurfaceHolder holder)");};};
    super.surfaceCreated(holder);
    ((MainActivity)getContext()).onceshowintro();
    }
@Override
public void surfaceDestroyed(SurfaceHolder holder) {
   {if(doLog) {Log.i(LOG_ID,"surfaceDestroyed(SurfaceHolder holder)");};};
   super.surfaceDestroyed(holder);
    }
static public void    doabout(MainActivity activity) {
if(!isWearable) {
    String about=activity.getString(R.string.about)+"<p>Version Code: "+ BuildConfig.VERSION_CODE+"<br>Version Name: "+ 
        BuildConfig.VERSION_NAME +"<br>"+Natives.getCPUarch()+"<br>Build time: "+ BuildTime.TIME +"</p>";
    
    help.help(about, activity,l->{});
    }
    }
void removeviews() {
        numberview.deleteviews();    
        removeSearchEditor();
        if(searchcontrol!=null) {
            removeContentView(searchcontrol);
            searchcontrol=null;
            }
       Applic.setremoveviews=false;
       }
}
