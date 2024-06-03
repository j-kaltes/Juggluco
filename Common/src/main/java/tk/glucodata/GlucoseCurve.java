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
import android.content.res.ColorStateList;
import android.graphics.Color;
import android.graphics.Typeface;
import android.opengl.GLSurfaceView;
import android.text.InputType;
import android.util.DisplayMetrics;
import android.util.TypedValue;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import java.util.Calendar;

import androidx.annotation.Keep;
import androidx.annotation.UiThread;

import tk.glucodata.nums.numio;
import tk.glucodata.settings.Settings;

import static android.util.TypedValue.COMPLEX_UNIT_PX;
import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static java.lang.Math.abs;
import static java.lang.System.currentTimeMillis;
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.Applic.usedlocale;
import static tk.glucodata.BuildConfig.SiBionics;
import static tk.glucodata.Natives.turnoffalarm;
import static tk.glucodata.NumberView.geteditview;
import static tk.glucodata.NumberView.geteditwearos;
import static tk.glucodata.NumberView.smallScreen;
import static tk.glucodata.RingTones.EnableControls;
import static tk.glucodata.settings.Settings.editoptions;
import static tk.glucodata.util.getlabel;

public class GlucoseCurve extends GLSurfaceView {
Button summarybutton=null;
boolean statspresent=false;
@Keep
void summaryready() {
	statspresent=true;
	Log.i(LOG_ID,"summaryready");
	Applic.RunOnUiThread(()-> {
		Button tmp= summarybutton;
		summarybutton=null;
		if(tmp!=null) {
			Log.i(LOG_ID,"set Visible");
			tmp.setVisibility(VISIBLE);
			tmp.bringToFront();
			}
		});
	
	}

@Keep
void showsensorinfo(String text) {
	Applic.RunOnUiThread(()-> {
		bluediag.showsensorinfo(text,(MainActivity )getContext());
		});
	}
static View[] reopen=new View[5];
static int reopennr=0;
    //    SearchLayout search;
    Layout search;
    Dialogs dialogs;
    private static final String LOG_ID = "GlucoseCurve";
static    float smallfontsize;
    Calendar cal = Calendar.getInstance();
 final   private ScaleGestureDetector mScaleDetector;
 final   private GestureDetector mGestureDetector;
    static final int STEPBACK = 1;
    boolean waitnfc = false;
    MyRenderer render = new MyRenderer();
    static int height,width;



NumberView  numberview= new NumberView();

Layout numcontrol=null;
void startsearch() {
    MainActivity activity = (MainActivity) getContext();
    if(searchcontrol!=null) {
	Natives.stopsearch();
	searchcontrol.setVisibility(View.GONE);
	}
    if (search == null) {
	search = getsearchlayout(activity);
    } else {
	var labels=Natives.getLabels();
	if(!searchspinadap.getarray().equals(labels))  {
		searchspinadap.setarray(labels);
		searchspinner.setAdapter(searchspinadap);
		}
	search.setVisibility(View.VISIBLE);
         if(labelsel==Natives.getmealvar())
	 	mkmealsearch(activity);
    }
    if(!smallScreen)
	    showkeyboard(activity);

    activity.setonback(()-> {
		activity.showui=false;
          activity.hideSystemUI();
		tk.glucodata.help.hidekeyboard(activity);
	search.setVisibility(View.GONE); 
        hidemealsearch();

	hidekeyboard(); reopener();
	if(Menus.on)
		Menus.show(activity);

	} );
    }



static void hidesave(View v) {
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
   Log.i(LOG_ID,"getnumcontrol start");
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
//	 search.setImageResource( android.R.attr.actionModeWebSearchDrawable);

           search.setOnClickListener(v-> {
	   	hidesave(numcontrol);
		startsearch();
		selectnumbers();
		hidesave(scansearch);
		hidesave(streamsearch);
		hidesave(historysearch);
	   	});
//	s/\(first[^6]*.6\)/(int)(\1)/g
	search.setPadding((int)(first.getPaddingLeft()*.69),(int)(first.getPaddingTop()*.69),(int)(first.getPaddingRight()*.69),(int)(first.getPaddingBottom()*.69));
	 ImageButton closecontrol=new ImageButton(activity);

	 closecontrol.setImageResource( android.R.drawable.ic_menu_close_clear_cancel);
           closecontrol.setOnClickListener(v-> {
	   	activity.doonback();
//	   	activity.poponback();
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
        final	int minheight=(int)(metrics.density*48.0f);
        setminheight(controls,minheight);
	   numcontrol= new Layout(activity,(v,w,h) -> {
		   final int width=getWidth();
		   final int height=getHeight();
		   int columns=Natives.numcontrol(w,h);
		   if(height>h)
			   v.setY((height-h)/2.0f);
		if(width>w) {
			   if(columns==1)  {
				   v.setX(width-w);
				}
			   else {
	//		   	   setminheight(controls,0);

				   v.setX(((width-w)/2.0f));
				   }
			   }
            	    requestRender();

			return new int[] {w,h};
		  },new View[]{first}, new View[]{back}, new View[]{search},new View[]{closecontrol}, new View[]{next}, new View[]{last}
		  );
           activity.addContentView(numcontrol, new ViewGroup.LayoutParams(WRAP_CONTENT,MATCH_PARENT));
	   }
  	else
		numcontrol.setVisibility(VISIBLE);
	activity.setonback(()-> {
		numcontrol.setVisibility(GONE);
		Natives.endnumlist();
		 if(Menus.on) {
		 	Menus.show(activity);
		 	}
		else
			 requestRender();
		 });
   Log.i(LOG_ID,"getnumcontrol end");
	}

    void showkeyboard(Activity context) {
   	numberview.showkeyboard(context);
	    }
    void hidekeyboard() {
    	numberview.hidekeyboard();
		    }
    	
    /*OnBackPressedCallback callback = new OnBackPressedCallback(true) {
        @Override
        public void handleOnBackPressed() {
            if ((render.stepresult & STEPBACK) == STEPBACK) {
                Log.d(LOG_ID,"GlucoseCurve: back");
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
	if(metrics==null) {
		metrics= Applic.app.getResources().getDisplayMetrics();
		}
	return metrics.density;
	}
    public GlucoseCurve(MainActivity context) {
        super(context);
	Log.i(LOG_ID,"GlucoseCurve "+MainActivity.openglversion);
        mScaleDetector = new ScaleGestureDetector(context, mScaleListener);
        final  GestureListener gestureListener = new GestureListener();
        mGestureDetector = new GestureDetector(context, gestureListener);
	setEGLContextClientVersion(MainActivity.openglversion);
	setEGLConfigChooser(8, 8, 8, 8, 16, 1);
	setRenderer(render);
	setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
/*	while(true) {
		try {/*
/*			break;
		}
		catch(Throwable e) {
			Log.stack(LOG_ID,"OpenGL init",e);
			}

		}; */


        metrics= getResources().getDisplayMetrics();
	dialogs=new Dialogs(metrics.density);

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
static void setgeo(int w,int h) {
 	if(smallScreen||w>=h) {
		width=w;
		height=h;
		}
	}
long multitime=0L;
    @Override
    public boolean onTouchEvent(MotionEvent event) {
    	if(turnoffalarm())
		Notify.stopalarm();
        if((render.stepresult&STEPBACK)!=0) {
            final float x = event.getX();
            final float y = event.getY();
	
            if(Natives.isbutton(x, y)) {
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
            focusx = detector.getFocusX();
            return true;
        }

        @Override
        public void onScaleEnd(ScaleGestureDetector detector) {


        }

        @Override
        public boolean onScale(ScaleGestureDetector detector) {
            float scalex = detector.getCurrentSpanX() / detector.getPreviousSpanX();
	  Log.i(LOG_ID,"onScale SpanX="+detector.getCurrentSpanX()+" PreviousSpanX="+ detector.getPreviousSpanX()+" scalex="+scalex);
            Natives.xscale(scalex, focusx);
            requestRender();
            down = false;
            return true;
        }
    };

	long reldate;
	 void startdatepick(long tim) {
		reldate=tim;
		numberview.getdateviewal((MainActivity)getContext(),tim,	(year,month,day)-> {
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
        @UiThread
        @Override
        public boolean onSingleTapUp(MotionEvent event) {
            Log.d(LOG_ID,"onSingleTapUp");
            if (down ) {
		final float x=event.getX();
		final float y=event.getY();
                long choice = Natives.tap(x, y);
                if(choice==-2L) 
			return true;
                if(choice!=-1L) {
                    int menu = (int) (choice & 0xf);
                    int item = (int) (choice >> 4);
		    Log.i(LOG_ID,"menu="+menu+" item="+item);
                    switch (menu) {
                        case 0:
                            switch (item) {
			    	case 0: ((MainActivity) getContext()).selectionSystemUI(); break;
				case 1: Menus.show((MainActivity) getContext());break;
				case 2: {
				if(!isWearable) {
					MainActivity activity = (MainActivity) getContext();
					tk.glucodata.Watch.show(activity);
					}
					}
					break;
			      case 3: new bluediag((MainActivity)getContext()); break;
			        case 4: {
					MainActivity activity = (MainActivity) getContext(); 
					Settings.set(activity);
					};break;

                                case 5: {
					if(!isWearable) {
						MainActivity activity = (MainActivity) getContext();
						if(SiBionics==1)
							Sibionics.scan(activity);
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
                                case 0: dialogs.showexport(( MainActivity)getContext(),getWidth(),getHeight()); break;


   				case 1: (new Backup()).mkbackupview(( MainActivity)getContext());break;
				case 2: {
				       MainActivity activity = (MainActivity) getContext();
					if(Natives.staticnum()) {
        					help.help(R.string.staticnum,activity);
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
		    case 3:
                            switch (item) {
                                case 1:
				   startsearch();
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
                            Log.i(LOG_ID,"tap pos="+pos+" base="+base);
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
        if(down) {
            if((render.stepresult&STEPBACK)==0)
                if(Natives.translate(distanceX, distanceY, e1.getRawY(), e2.getRawY())!=0)
                    requestRender();
            return true;
            }
        return false;
        }


        @Override
        public void onLongPress(MotionEvent event) {
            Log.d(LOG_ID,"OnLongPress" + (down?"":" not") + " down");
	    if(down) {
		long nutime=System.currentTimeMillis();
		if((nutime-multitime)<1000)
			return;
                final float wgrens=smallfontsize*3;
                final float rgrens=getWidth()-wgrens;
                final float x=event.getX();
                final float y=event.getY();
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
        public boolean	onDoubleTap(MotionEvent e) {
            Log.d(LOG_ID,"public boolean	onDoubleTap(MotionEvent e) {");return false;

        }

        @Override
        public boolean	onDoubleTapEvent(MotionEvent e) {
            Log.d(LOG_ID,"public boolean	onDoubleTapEvent(MotionEvent e) {");return false;

        }

        @Override
        public   boolean	onSingleTapConfirmed(MotionEvent e) {
            Log.d(LOG_ID,"public   boolean	onSingleTapConfirmed(MotionEvent e) {");return false;
        }
*/
        @Override
        public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {Log.d(LOG_ID,"onFling");
            if(down) {
                float absx=abs(velocityX);
                if(absx>2000.0&&absx>abs(velocityY)) {
                    Natives.flingX(velocityX);
                    requestRender();
                }
                return true;
            }
    return false;
        }
    };//end class GestureListener 



private int[] minutes={-1,-1};

private void mktimedialog( Button but,final int num ,View parent) {
        but.setOnClickListener(
                v->  {
		parent.setVisibility(INVISIBLE);
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
            text.setText(String.format(usedlocale,"%02d:%02d", hour, min));
//			v.setBackgroundColor(Color.RED);
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
    ImageButton prev=null,next=null;
   void searchaway() {
       if(search!=null) {
           search.setVisibility(GONE);
       hidemealsearch();
           hidekeyboard();
 	if(searchcontrol!=null)
		 searchcontrol.setVisibility(GONE);
	reopener();
	   Natives.stopsearch();
       requestRender();
       }
   }
static void reopener() {
	for(int i=0;i<reopennr;i++)
		reopen[i].setVisibility(VISIBLE);
	reopennr=0;
	}
int labelsel=-1;
    private void clearsearch(View view) {
    if(mealquantity!=null) {
	     mealquantity.setText("");
	     mealingredient.setText("");
	     }
      under.setText("0");
        above.setText("999");
        labelsel=searchspinner.getCount()-1;
        searchspinner.setSelection(labelsel);
        scansearch.setChecked(true);
        streamsearch.setChecked(true);
        historysearch.setChecked(true);
        fromtime.setText("00:00");
        totime.setText("23:59");
	fromtime.setTextColor(oldColors);
	totime.setTextColor(oldColors);
	totime.setTextSize(COMPLEX_UNIT_PX,oldsize);
	fromtime.setTextSize(COMPLEX_UNIT_PX,oldsize);
//	fromtime.setTypeface(null,0);
//	totime.setTypeface(null,0);
        minutes[0]=-1;
        minutes[1]=-1;

//	above.requestFocus();
	under.requestFocus();
	editfocus.setedittext(under);
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
       int glsearch=((historysearch.isChecked()?0x40000002:0)| (scansearch.isChecked()?0x40000001:0))|(streamsearch.isChecked()?0x40000004:0);
   if(Natives.search(glsearch==0?labelsel:glsearch,funder,fabove,minutes[0],minutes[1],forward,ingsearch,ingamount)==0) {

       search.setVisibility(GONE);
       hidemealsearch();
       hidekeyboard();
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
		   int width=getWidth();
		   int columns=Natives.getcolumns(w);
		   if(height>h && width>w) {
			   v.setY((height-h)/2);
			   if(columns==1)  {
				   v.setX(width-w);
				}
			   else {
				   v.setX((width-w)/2);
				   }
			}
            	    requestRender();
			return new int[] {w,h};
		  },new View[]{prev},new View[] {closecontrol},new View[] {next});
           activity.addContentView(searchcontrol, new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));
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

//RadioButton numbers;

    CheckBox scansearch,historysearch,streamsearch;
    Button fromtime, totime;

//https://gist.github.com/kakajika/a236ba721a5c0ad3c1446e16a7423a63
    /*
void radiolisten( RadioButton one,RadioButton other) {
         one.setOnClickListener(v-> {
             ((RadioButton) v).setChecked(true);
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
       //     spinner.setVisibility(VISIBLE);
	    }
void glucoselisten(CompoundButton one) {
    one.setOnClickListener(v -> {
        if(historysearch.isChecked()||scansearch.isChecked()||streamsearch.isChecked()) {
           // numbers.setChecked(false);
        //    spinner.setVisibility(GONE);
        labelsel=searchspinner.getCount()-1;
        searchspinner.setSelection(labelsel);
        }
        else {
          //  numbers.setChecked(true);
         //   spinner.setVisibility(VISIBLE);

        }
//        but.setChecked(!but.isChecked()); Log.d(LOG_ID,"now "+ but.isChecked());
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
//		searchspinner=new Spinner(context, null,R.style.MySpinnerStyle );
//        searchspinner=new Spinner(context,R.style.spinner_style);
//        searchspinner=new Spinner(context,R.style.MySpinnerStyle2);

	int minheight=GlucoseCurve.dpToPx(48);
	searchspinner.setMinimumHeight(minheight);

	searchspinner.setContentDescription("Amount type selector");
   NumberView.avoidSpinnerDropdownFocus(searchspinner);
	searchspinadap= new LabelAdapter<String>(context,Natives.getLabels(),0);
        searchspinner.setAdapter(searchspinadap);
    searchspinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
        @Override
        public  void onItemSelected (AdapterView<?> parent, View view, int position, long id) {
	   Log.i(LOG_ID,"onItemSelected "+position);
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
	TextView.OnEditorActionListener	 actlist= new TextView.OnEditorActionListener() {
		    @Override
		    public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
		    if (event != null && event.getKeyCode() == KeyEvent.KEYCODE_ENTER
				    || actionId == EditorInfo.IME_ACTION_DONE) {
				act.hideSystemUI();
				 tk.glucodata.help.hidekeyboard(act);
				 Log.i(LOG_ID,"onEditorAction");
// hidekeyboard();
				return true;
			   }
		    return false;
		    }};
		mealingredient.setOnEditorActionListener(actlist);
		    */

//		mealingredient.setMinEms(10);
		ViewGroup.LayoutParams params= new ViewGroup.LayoutParams(  MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
		mealingredient.setLayoutParams(params);
		if(smallScreen) {
			mealquantity=geteditwearos(act);
			}
		else
			mealquantity=geteditview(act,new editclosefocus());
		mealquantity.setMinEms(3);
		int pad= (int)(tk.glucodata.GlucoseCurve.metrics.density*5);
     		inglabel.setPadding(pad,0,pad,0);
		qualabel.setPadding(pad,0,pad,0);

//		mealquantity.setPadding(0,0,0,(int)metrics.density*8);
//		mealquantity.setMinEms(2);
//		mealingredient.setImeOptions(EditorInfo.IME_FLAG_NO_EXTRACT_UI| EditorInfo.IME_FLAG_NO_FULLSCREEN| EditorInfo.IME_ACTION_SEARCH);
//		mealquantity.setOnEditorActionListener(actlist);
		meallayout=new Layout(act,
/*(lay, w, h)->{
//		    lay.setX(MyRenderer.widthdiff);

		return new int[] {w-MyRenderer.widthdiff,h};
		},*/


		new View[] {inglabel,mealingredient,qualabel,mealquantity});
		act.addContentView(meallayout, new ViewGroup.LayoutParams(MATCH_PARENT,WRAP_CONTENT));
    		meallayout.setBackgroundColor(Applic.backgroundcolor);
		}
	else {
		meallayout.setVisibility(VISIBLE);
		meallayout.bringToFront();
		}

	}

private Layout getsearchlayout(MainActivity context) {
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
    scansearch=new CheckBox(context); scansearch.setText(R.string.scanname);
     historysearch=new CheckBox(context); historysearch.setText(R.string.historyname);
     streamsearch=new CheckBox(context); streamsearch.setText(R.string.streamname);
     streamsearch.setPadding(0,0,(int)metrics.density*8,0);
        glucoselisten(scansearch) ;
        glucoselisten(historysearch) ;
        glucoselisten(streamsearch) ;

    View[] glucoseline={scansearch,historysearch,streamsearch};
    fromtime =new Button(context); //fromtime.setText("00:00");
	TextView gline=new TextView(context);gline.setText(" - ");

      totime=new Button(context); //totime.setText("23:59"); 
      oldColors=totime.getTextColors();
      oldsize=totime.getTextSize();
        Button clear=new Button(context);clear.setText(R.string.resetname);

    View[] timeline={clear,fromtime,totime};

        ImageButton helpbut=new ImageButton(context);
        helpbut.setImageResource( android.R.drawable.ic_menu_help);

	helpbut.setContentDescription(getContext().getString(R.string.helpname));
        helpbut.setOnClickListener(v-> { help.help(R.string.searchhelp,context); });

    Button cancel=new Button(context);cancel.setText(R.string.cancel);
    View[] buttonline={getsearchspinner(context),under,line,above};



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

	View[] goline={backward,cancel,helpbut, forward};

	Layout layout=new Layout(context,(lay, w, h)->{
	int width=GlucoseCurve.getwidth();
	int height=GlucoseCurve.getheight();
if(!smallScreen) {
//	boolean rtl=Natives.getRTL();
	if(height>h&&width>w) {
		   if(width>height) {
			    lay.setY((height - h) / 2);
			    
			    int half= width / 2;
			    int af=(half-w)/4;
			    int posx= half - w-af;
			    if(posx<0) {
			    	posx=0;
				numberview.noroom=true;
				}
			    else
				numberview.noroom=false;
			    lay.setX(posx);
			   Log.i(LOG_ID,"search h="+h+" height="+height+" w="+w+" width="+width+" posx="+posx);
			    }
			else {

			int half=height/2;
			int af=(half-h)/4;
			 var xpos= (width - w)/2;
			    lay.setX(xpos);
			    lay.setY(half - h-af);

         Log.i(LOG_ID,"search h="+h+" height="+height+" w="+w+" width="+width+" posx="+xpos);
			}
		}
	else {
		w=width;h=height;
		    lay.setX(0);
		    lay.setY(0);
		}
		}
	else {
      final var ypos=(int)((height-h)/2.5f);
		lay.setY(ypos);
      final var xpos=width>w?(width - w)/2:0;
		lay.setX(xpos);
      Log.i(LOG_ID,"smallScreen search h="+h+" height="+height+" w="+w+" width="+width+" posx="+xpos+" posy="+ypos);
		}
		return new int[] {w,h};
		},buttonline,glucoseline, timeline,goline);

     	mktimedialog( fromtime,0 ,layout);
      mktimedialog( totime,1 ,layout);
	context.addContentView(layout, new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));
    	layout.setBackgroundColor(Applic.backgroundcolor);

            
        cancel.setOnClickListener(v -> {
		context.doonback();
		} );
	
    clear.setOnClickListener(this::clearsearch );
    clearsearch(clear);
//       editfocus.setedittext(under);

    return layout;
}


//Editable edit;



@Override
public void onResume() {
    super.onResume();

/*	setPivotX(getWidth()/2);
	setPivotY(getHeight()/2); */
//	setRotation(90);

    Applic app = ((Applic) ((Activity) getContext()).getApplication());

     app.setcurve(this);
     app.setmintime();

}

    @Override
    public void onPause() {
     Applic app = ((Applic) ((Activity) getContext()).getApplication());
     app.cancelmintime();
     app.setcurve(null);
//    app.savestate();
      super.onPause();
    }

static public void	doabout(MainActivity activity) {
if(!isWearable) {
	String about=activity.getString(R.string.about)+"<p>Version Code: "+ BuildConfig.VERSION_CODE+"<br>Version Name: "+ 
    	BuildConfig.VERSION_NAME +"<br>"+Natives.getCPUarch()+"<br>Build time: "+ BuildConfig.BUILD_TIME +"</p>";
	
	help.help(about, activity);
	}
	}
}
