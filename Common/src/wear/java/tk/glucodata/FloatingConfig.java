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
/*      Fri Jan 27 15:31:32 CET 2023                                                 */


package tk.glucodata;

import static android.graphics.Color.WHITE;
import static android.view.View.GONE;
import static android.view.View.INVISIBLE;
import static android.view.View.VISIBLE;
import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.Applic.usedlocale;
import static tk.glucodata.Floating.rewritefloating;
import static tk.glucodata.settings.Settings.editoptions;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getcheckbox;
import static tk.glucodata.util.getlabel;

import android.graphics.Color;
import android.text.InputType;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;

import yuku.ambilwarna.AmbilWarnaDialog;

public class FloatingConfig {
private static final String LOG_ID="FloatingConfig";
//   AmbilWarnaDialog(Context context, int color, boolean supportsAlpha, OnAmbilWarnaListener listener)
static private boolean background=true;


static public void	setcolor(int c) {
		Log.i(LOG_ID,"setcolor("+(c&0xFFFFFFFF)+")");
		if(background) { 
			Floating.setbackgroundcolor(c);
			}
		else  {Floating.setforegroundcolor(c);			}
		}
static public int	getcolor() {
		return background?Natives.getfloatingbackground( ):Natives.getfloatingforeground( );
		}

static public void show(MainActivity act) {
	int height=GlucoseCurve.getheight();
	int width=GlucoseCurve.getwidth();
	var  sizelabel=getlabel(act,R.string.fontsize);
	int pad=height/10;
	sizelabel.setPadding(pad,0,0,0);
	var  sizeview= new EditText(act);
              sizeview.setImeOptions(editoptions);
                sizeview.setMinEms(4);
                sizeview.setInputType(InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL);

	int fontsize=Natives.getfloatingFontsize();
	sizeview.setText(fontsize+"");
        TextView.OnEditorActionListener  actlist= new TextView.OnEditorActionListener() {
                    @Override
                    public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                    if (event != null && event.getKeyCode() == KeyEvent.KEYCODE_ENTER || actionId == EditorInfo.IME_ACTION_DONE) {
                                 Log.i(LOG_ID,"onEditorAction");
				 try {
					var siz=Integer.parseInt(String.valueOf(v.getText()));
					int maxfont=Math.min(height*7/10,width*4/10);
					if(siz>maxfont) {
						Toast.makeText(act, act.getString(R.string.fonttoolarge)+maxfont, Toast.LENGTH_SHORT).show();
						return true;
						}
					else  {
						Natives.setfloatingFontsize(siz);
						 rewritefloating(act);
					//	Floating.invalidatefloat();
						 }
					}
				catch(Throwable th) {
					Log.stack(LOG_ID,"parseInt",th);
					}
//                                return true;
                           }
                    return false;
                    }};
	sizeview.setOnEditorActionListener(actlist);



	var touch=Natives.getfloatingTouchable();
	var touchable=getcheckbox(act,R.string.touchable,touch);
	var close=getbutton(act,R.string.closename);

	var color=Natives.getfloatingbackground();
	boolean transp= Color.alpha(color)!=0xFF;
	var transparentview=getcheckbox(act,R.string.transparent,transp);
	var backgroundview=getbutton(act,R.string.background);

	if (transp) backgroundview.setVisibility(INVISIBLE);

	var foreground=getbutton(act,R.string.foreground);




	Layout layout=new Layout(act,(l,w,h)-> { return new int[] {w,h}; },new View[]{touchable},new View[]{transparentview},  new View[]{sizelabel,sizeview},new View[]{foreground,backgroundview},new View[]{close});

	transparentview.setOnCheckedChangeListener( (buttonView,  isChecked) -> {
		background=true;
		Floating.setbackgroundalpha(isChecked?0:0xff);
		if(isChecked)
			backgroundview.setVisibility(INVISIBLE);
		else
			backgroundview.setVisibility(VISIBLE);
		//rewritefloating(act);
		Floating.invalidatefloat();

	});
	layout.setBackgroundColor(Applic.backgroundcolor);
	foreground.setOnClickListener(v-> {background=false;showcolors(act);});
	backgroundview.setOnClickListener(v-> {background=true;showcolors(act);});
	touchable.setOnCheckedChangeListener( (buttonView,  isChecked) -> {
		Natives.setfloatingTouchable(isChecked);
		if(!isChecked) {
			int y= (int) Floating.yview;
			Natives.setfloatingPos(((int)Floating.xview)|(0xFFFFFFFF&(y<< 16)));
			}
		rewritefloating(act);

		});


	       act.addContentView(layout, new ViewGroup.LayoutParams(MATCH_PARENT,MATCH_PARENT));
	act.setonback(()-> { removeContentView(layout); });
	close.setOnClickListener(v->{
		act.doonback();
	});
}

static public void showcolors(MainActivity act) {
	int initialColor= getcolor();


	int height=GlucoseCurve.getheight();
	var width=GlucoseCurve.getwidth();
	AmbilWarnaDialog dialog = new AmbilWarnaDialog(act, initialColor,c-> {
	Log.i(LOG_ID,String.format(usedlocale,"col=%x",c));
		setcolor(c);
		//rewritefloating(act);
		Floating.invalidatefloat();
    }, v-> {
      	int h=v.getMeasuredHeight();
                int w=v.getMeasuredWidth();
//                v.setX((int)(width*.97)-w);
 //               v.setY((height-h)/2);
                v.setY((int)((height-h)*.5));
                v.setX((int)((width-w)*.57));

    	}
	);
	View view=dialog.getview();





//	view.setLayoutParams( new ViewGroup.LayoutParams(MATCH_PARENT,MATCH_PARENT));
/*	var colorlayout=new Layout(act,(l, w, h)-> { return new int[] {w,h}; },new View[]{view});
	colorlayout.setBackgroundColor(Applic.backgroundcolor); */
	var layout=new FrameLayout(act);
	layout.addView(view, new ViewGroup.LayoutParams((int)(width*0.72), (int)(height*0.72)));
	act.addContentView(layout,  new ViewGroup.LayoutParams(MATCH_PARENT,MATCH_PARENT));
	layout.setBackgroundColor(Applic.backgroundcolor);
	  layout.setOnTouchListener(new BackGesture(act));
	act.setonback(()-> { removeContentView(layout); });




	Button noclose= act.findViewById(R.id.closeambi);
	 if(noclose!=null) {
		noclose.setVisibility(GONE);
		noclose.setText("");
		Button nohelp= act.findViewById(R.id.helpambi);
		nohelp.setText("");
		nohelp.setVisibility(GONE);
	  }
	};
}

