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
/*      Fri Jan 27 15:32:11 CET 2023                                                 */


package tk.glucodata.settings;

import android.app.Activity;
import android.content.Context;
import android.text.InputType;
import android.text.method.DigitsKeyListener;

import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import java.util.ArrayList;

import tk.glucodata.Applic;
import tk.glucodata.Layout;
import tk.glucodata.Log;
import tk.glucodata.MainActivity;
import tk.glucodata.Natives;
import tk.glucodata.R;

import static android.view.View.GONE;
import static android.view.View.INVISIBLE;
import static android.view.View.VISIBLE;
import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.MainActivity.getscreenwidth;
import static tk.glucodata.help.hidekeyboard;
import static tk.glucodata.settings.Settings.editoptions;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getlabel;

public class Shortcuts {
public void hideSystemUI(Context cont) {}

ViewGroup shortlistview=null;
Layout shortlist=null;

ArrayList<ArrayList<Object>> shortcuts;
ArrayList<Object> current=null;

/*
String chars2str(ArrayList<Byte> chars) {
  StringBuilder sb = new StringBuilder();
        for (byte ch: chars) {
            sb.append(ch);
        }
	return sb.toString();
	}
*/ 
/*
String chars2str(ArrayList<Byte> chars) {
	final int len= chars.size();
	byte[] byt=new byte[len];
	for(int i=0;i<len;i++)
		byt[i]=chars.get(i);
	return 	new String(byt, StandardCharsets.UTF_8);
	}
	*/
void addrow(Context act,int index) {
		ArrayList<Object>  el=shortcuts.get(index); 
	 	Button lab=getbutton(act,(String)el.get(0));
		lab.setTransformationMethod(null);
           	lab.setOnClickListener(v->{
			current=el;
			mkshort((MainActivity)v.getContext());
			setvalues((String)el.get(0),(String)el.get(1));
			delete.setVisibility(VISIBLE);
		});

		lab.setMinEms(10);
		String values =  (String)el.get(1);
		TextView value=getlabel(act,values);
		value.setMinEms(4);
		shortlist.addrow(new View[]{lab,value});
		}
private static String LOG_ID="Shortcuts";
void mkshortlist(Context act) {

	shortcuts= Natives.getShortcuts();
	 int len=shortcuts.size();
	 Log.i(LOG_ID,"mkshortlist "+len);
	 shortlist= new Layout(act,(l, w, h)->{

			return new int[] {w,h};
	 }, len);
	 for(int i=0;i<len;i++) {
	 	addrow(act,i);
	 	}
	}
void saveall(View v) {
	MainActivity main=(MainActivity) v.getContext();
	 if(shortedit!=null)  {
		final int nr= shortcuts.size();
		for(int i=0;i<nr;i++) {
			ArrayList<Object> el=shortcuts.get(i);
			int ret;
			if((ret=Natives.setShortcut(i,(String)el.get(0),(String)el.get(1)))!=-1) {
				if(ret==-5)
					Toast.makeText(v.getContext(), "index "+i+" too large", Toast.LENGTH_SHORT).show();
				else
					Toast.makeText(v.getContext(), (String)el.get(ret)+" too long", Toast.LENGTH_SHORT).show();
				return;
				}
			}
		Natives.setnrshortcuts(nr);
		if(!isWearable) {
			((Applic)(((Activity)v.getContext()).getApplication())).numdata.sendshortcuts(shortcuts);
			}
		 Applic.wakemirrors();
		 removeContentView(shortedit);
		 }
	 removeContentView(shortlistview);
	main.poponback();
	}
public void mkshortlistview(MainActivity act) {
	if(shortlistview==null) {
		Button add=new Button(act),ok=new Button(act);
		add.setText(R.string.newname);
           	add.setOnClickListener(v->{
			current=null;
			mkshort(act);
			setvalues("","");
			delete.setVisibility(INVISIBLE);
			});
		ok.setText(R.string.save);
           	ok.setOnClickListener(this::saveall);
		Button cancel=getbutton(act,R.string.cancel);
           	cancel.setOnClickListener(v->{
				act.doonback();
			});
		Button help=getbutton(act,R.string.helpname);
		help.setOnClickListener(v->{tk.glucodata.help.help(R.string.shortcuthelp,act); });
		ScrollView scroll=new ScrollView(act);
		scroll.setSmoothScrollingEnabled(false);
		scroll.setVerticalScrollBarEnabled(false);
		scroll.setHorizontalScrollBarEnabled(false);
		
		scroll.setLayoutParams(new ViewGroup.LayoutParams(   ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.MATCH_PARENT));
		mkshortlist(act);
		scroll.addView(shortlist);
		Layout butview= new Layout(act,new View[]{add},new View[]{help},new View[]{cancel},new View[]{ok});
		butview.setLayoutParams(new ViewGroup.LayoutParams(   ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.MATCH_PARENT));
		shortlistview= new Layout(act,(a,w,h)->{
			hideSystemUI(act);
			return new int[] {w,h}; },new View[]{scroll,butview});
		shortlistview.setBackgroundColor(tk.glucodata.Applic.backgroundcolor);
		act.addContentView(shortlistview, new ViewGroup.LayoutParams(MATCH_PARENT, MATCH_PARENT));
		}
	else {
        	shortlistview.setVisibility(VISIBLE);
		}
	Runnable closerun=() -> {
		shortlistview.setVisibility(GONE);
		if(shortedit!=null)
			removeContentView(shortedit);
		removeContentView(shortlistview);
		};
	act.setonback(closerun);
	}
ViewGroup shortedit=null;
EditText labedit=null, valedit=null;
void deleteshort(View v) {
	int index=shortcuts.indexOf(current);
	if(index>=0) {
		shortcuts.remove(index);
		shortlist.delrow(index);
		}
	 endshortedit() ;
	 ((MainActivity)v.getContext()).poponback();
	}
void endshortedit() {
	shortedit.setVisibility(GONE);
	hidekeyboard((Activity)shortedit.getContext()); //USE
	}
void saveshort(View v) {
	String label=labedit.getText().toString();
	String values=valedit.getText().toString();
	if(current==null) {
		ArrayList<Object> el=new ArrayList<>();
		el.add(label);
		el.add(values);
		shortcuts.add(el);
		addrow(v.getContext(),shortcuts.size()-1);
		current=el;
		}
	else {
		int index=shortcuts.indexOf(current);
		if(index>=0) {
			current.set(0,label);
			current.set(1,values);
			View[] views=shortlist.getrow(index);
			((TextView)views[0]).setText(label);
			((TextView)views[1]).setText(values);
			}
		}
	
	 endshortedit() ;
	 ((MainActivity)v.getContext()).poponback();
	}
void setvalues(String name,String value) {
	labedit.setText(name);
	valedit.setText(value);
	}
Button delete=null;
void mkshort(MainActivity act) {
if(shortedit==null) {
	TextView label=getlabel(act,R.string.shortcut);
	labedit=new EditText(act);
	labedit.setInputType(InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD);

	labedit.setImeOptions( EditorInfo.IME_FLAG_NO_EXTRACT_UI| EditorInfo.IME_FLAG_NO_FULLSCREEN| EditorInfo.IME_ACTION_DONE);
	labedit.setMinEms(7);
	TextView value=getlabel(act,R.string.value);
	valedit=new EditText(act);
	valedit.setMinEms(3);
	valedit.setInputType(InputType.TYPE_CLASS_NUMBER |InputType.TYPE_NUMBER_FLAG_DECIMAL);//| InputType.IME_FLAG_NO_FULLSCREEN);

        valedit.setImeOptions(editoptions);
//	valedit.setKeyListener(DigitsKeyListener.getInstance("^*/+-().0123456789"));
//	valedit.setImeOptions( EditorInfo.IME_FLAG_NO_EXTRACT_UI| EditorInfo.IME_FLAG_NO_FULLSCREEN| EditorInfo.IME_ACTION_DONE);
	Button save=getbutton(act,R.string.ok);
	save.setOnClickListener(this::saveshort);
	delete=getbutton(act,R.string.delete);
	delete.setOnClickListener(this::deleteshort);
	Button cancel=getbutton(act,R.string.cancel);
	cancel.setOnClickListener(v->{ 
			act.doonback();
			 }); 
	shortedit=new Layout(act, (l, w, h) -> {
			hideSystemUI(act);
			var width= getscreenwidth(act);
			if(width>w)
			    l.setX(( width- w)* 0.7f);
			    l.setY(0);

			return new int[] {w,h};
			    }, new View[] {label,labedit},new View[] {value,valedit},new View[] {delete,cancel,save});
//	shortedit.setBackgroundColor(tk.glucodata.Applic.backgroundcolor);

	      shortedit.setBackgroundResource(R.drawable.dialogbackground);
	   int pad=(int)(tk.glucodata.GlucoseCurve.metrics.density*5.0);
	   shortedit.setPadding(pad,0,pad,0);
	act.addContentView(shortedit, new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));
    }
    else {
        shortedit.setVisibility(VISIBLE);
	}
   act.setonback(() -> endshortedit() );
   }
   }
