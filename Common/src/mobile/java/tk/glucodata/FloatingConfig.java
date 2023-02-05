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
import static tk.glucodata.Applic.usedlocale;
import static tk.glucodata.Notify.rewritefloating;
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
import android.widget.EditText;
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
			Natives.setfloatingbackground(c );
			var getc=Natives.getfloatingbackground();
			Log.i(LOG_ID,"getfloatingbackground("+(getc&0xFFFFFFFF)+")");
			}
		else Natives.setfloatingforeground( c);
		}
static public int	getcolor() {
		return background?Natives.getfloatingbackground( ):Natives.getfloatingforeground( );
		}


static public void	setalpha(int alpha) {
	int initialColor= getcolor();
	setcolor(0xFFFFFFFF&((initialColor&0xFFFFFF)|alpha<<24));
	}
static public void show(MainActivity act) {
	int initialColor= getcolor();


	int height=GlucoseCurve.getheight();
    AmbilWarnaDialog dialog = new AmbilWarnaDialog(act, initialColor,c-> {
	Log.i(LOG_ID,String.format(usedlocale,"col=%x",c));
		setcolor(c);
		rewritefloating(act);
    }, v-> {
    	}
	);
	View view=dialog.getview();
	var  sizelabel=getlabel(act,R.string.fontsize);
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
					int maxfont=height*7/10;
					if(siz>maxfont) {
						Toast.makeText(act, act.getString(R.string.fonttoolarge)+maxfont, Toast.LENGTH_SHORT).show();
						return true;
						}
					else  {
						Natives.setfloatingFontsize(siz);
						 rewritefloating(act);
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



	var foregroundswitch=new Switch(act);
	foregroundswitch.setChecked(background);
	foregroundswitch.setText(R.string.foreground);
	var  backgroundlabel=getlabel(act,R.string.background);
	backgroundlabel.setTextColor(WHITE);
	foregroundswitch.setTextColor(WHITE);
	var color=Natives.getfloatingbackground();
	boolean transp= Color.alpha(color)!=0xFF;
	var transparant=getcheckbox(act,R.string.transparent,transp);
	var touch=Natives.getfloatingTouchable();
	var touchable=getcheckbox(act,R.string.touchable,touch);
	if(!background)
			transparant.setVisibility(INVISIBLE);
	else {
		if (transp)
			view.setVisibility(INVISIBLE);
		}
	var close=getbutton(act,R.string.closename);
	var Help=getbutton(act,R.string.helpname);
	Help.setOnClickListener(v-> help.help(R.string.floatingconfig,act));
	var leftlayout=new Layout(act,(l, w, h)-> { return new int[] {w,h}; },new View[]{sizelabel,sizeview},new View[]{touchable,transparant}, new View[]{foregroundswitch,backgroundlabel},new View[]{Help,close});
	leftlayout.setLayoutParams( new ViewGroup.LayoutParams(WRAP_CONTENT,MATCH_PARENT));
	view.setLayoutParams( new ViewGroup.LayoutParams(MATCH_PARENT,MATCH_PARENT));
	var layout=new Layout(act,(l,w,h)-> { return new int[] {w,h}; }, new View[]{view,leftlayout});
	layout.setBackgroundColor(Applic.backgroundcolor);
	transparant.setOnCheckedChangeListener( (buttonView,  isChecked) -> {
		setalpha(isChecked?0:0xff);
		rewritefloating(act);
		removeContentView(layout);
		act.poponback();
		show(act);
	});
	touchable.setOnCheckedChangeListener( (buttonView,  isChecked) -> {
		Natives.setfloatingTouchable(isChecked);
		if(!isChecked) {
			int y= (int) Notify.yview;
			Natives.setfloatingPos(((int)Notify.xview)|(0xFFFFFFFF&(y<< 16)));
			}
		rewritefloating(act);
		});

	foregroundswitch.setOnCheckedChangeListener( (buttonView,  isChecked) -> {
		background=isChecked;
		removeContentView(layout);
		act.poponback();
		show(act);

//		dialog.setColor(getcolor());
//		view.invalidate();
	});

	       act.addContentView(layout, new ViewGroup.LayoutParams(MATCH_PARENT,MATCH_PARENT));
        Button noclose= act.findViewById(R.id.closeambi);
		noclose.setVisibility(GONE);
		noclose.setText("");
        Button nohelp= act.findViewById(R.id.helpambi);
	nohelp.setText("");
	nohelp.setVisibility(GONE);
	act.setonback(()-> { removeContentView(layout); });
	close.setOnClickListener(v->{
		act.doonback();
	});
}

}
