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

import android.annotation.SuppressLint;
import android.app.Activity;
import android.graphics.Color;
import android.os.Build;
import android.text.method.LinkMovementMethod;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.ScrollView;
import android.widget.TextView;

import java.lang.ref.WeakReference;

import static android.content.Context.INPUT_METHOD_SERVICE;
import static android.text.Html.TO_HTML_PARAGRAPH_LINES_CONSECUTIVE;
import static android.text.Html.fromHtml;
import static android.view.View.GONE;
import static android.view.View.VISIBLE;
import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static tk.glucodata.MainActivity.doonback;
import static tk.glucodata.MainActivity.poponback;
import static tk.glucodata.MainActivity.setonback;
import static tk.glucodata.settings.Settings.removeContentView;

public class help {
static private final String LOG_ID="help";
//tatic   Layout helplayout=null;
static WeakReference<Layout> whelplayout=null;
static    WeakReference<TextView> whelpview=null;
public static   void help(int res,Activity act,Consumer<Layout> okproc) {
	help(act.getString(res),act,okproc);
	}
public static   void help(int res,Activity act) {
	help(res,act,l->{});
	}
static    WeakReference<Button> okbutton=null;
public static void hide() {
	if(whelplayout==null) 
		return;
	Layout lay=whelplayout.get();
	if(lay==null)
		return;
	lay.setVisibility(GONE);
	}
public static void show() {
	if(whelplayout==null) 
		return;
	Layout lay=whelplayout.get();
	if(lay==null)
		return;
	lay.setVisibility(VISIBLE);
	}

public static   void basehelp(int res,Activity act,Consumer<Layout> okproc) {
	basehelp(act.getString(res),act,okproc);
	}

	public static   void  basehelp(String text,Activity act,Consumer<Layout>  okproc) {
		  basehelp(text,act,okproc,(v,w,h)-> new int[] {w,h},new ViewGroup.MarginLayoutParams(MATCH_PARENT, MATCH_PARENT)) ;
		}
	@SuppressWarnings("deprecation")
	public static   void  basehelp(String text,Activity act,Consumer<Layout>  okproc,Placer place, ViewGroup.MarginLayoutParams params) {
       hidekeyboard(act);
	    ScrollView       helpscroll=new ScrollView(act);
           TextView helpview=new TextView(act);

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
        helpview.setText(fromHtml(text,TO_HTML_PARAGRAPH_LINES_CONSECUTIVE));
    }
    else {
			helpview.setText(fromHtml(text));
	}
	   int pad=(int)(GlucoseCurve.getDensity()*7.0);
	   helpview.setPadding(pad,pad,pad,pad);
           helpview.setTextColor(Color.WHITE);
           helpview.setTextIsSelectable(true);
	helpview.setMovementMethod(LinkMovementMethod.getInstance());
 	    helpview.setLinksClickable(true);
           helpscroll.addView(helpview);
       helpscroll.setVerticalScrollBarEnabled(Applic.scrollbar);
      helpscroll.setScrollbarFadingEnabled(false);
	   
           Button ok=new Button(act);
           ok.setText(R.string.ok);

           final Layout  helplayout=new Layout(act,

	   (l,w,h)-> {

          var af=MainActivity.systembarTop*3/4;
            l.setY(af);
	       return place.place(l,w,h -af); 
		}, new View[]{helpscroll},new View[]{ok});
           ok.setOnClickListener(v->{
		poponback();
		 okproc.accept(helplayout);
		removeContentView(helplayout);
	   	});
        params.setMargins(
            MainActivity.systembarLeft,
            0,
            MainActivity.systembarRight,

           MainActivity.systembarBottom
		);
      helplayout.setLayoutParams(params);
        helplayout.requestLayout();
	      helplayout.setBackgroundResource(R.drawable.helpbackground);
           act.addContentView(helplayout, params);


     Runnable closerun=() -> {
			 okproc.accept(helplayout);
		removeContentView(helplayout);
			 
		 };

	setonback(closerun);
};

	@SuppressLint("deprecation")
	public static   void help(String text,Activity act,Consumer<Layout>  okproc,Placer place, ViewGroup.MarginLayoutParams params) {
		Log.i(LOG_ID,"help");
       hidekeyboard(act);
      Button ok;
    Layout helplayout;

       if(whelplayout==null||((helplayout=whelplayout.get())==null)||act!=helplayout.getContext()||( (ok=	okbutton.get())==null) ) {

	    ScrollView       helpscroll=new ScrollView(act);
           TextView helpview=new TextView(act);
	   int pad=(int)(GlucoseCurve.getDensity()*7.0);
	   helpview.setPadding(pad,pad,pad,pad);

           helpview.setTextColor(Color.WHITE);
           helpview.setTextIsSelectable(true);
	    whelpview=new WeakReference<TextView>(helpview);
		helpview.setMovementMethod(LinkMovementMethod.getInstance());
 	    helpview.setLinksClickable(true);
           helpscroll.addView(helpview);
       helpscroll.setVerticalScrollBarEnabled(Applic.scrollbar);
      helpscroll.setScrollbarFadingEnabled(false);
	   
           ok=new Button(act);
           ok.setText(R.string.ok);
	   okbutton=new WeakReference<Button>(ok);
	   
	var width=GlucoseCurve.getwidth();

           helplayout=new Layout(act,

	   (l,w,h)-> {


          var af=MainActivity.systembarTop*3/4;
            l.setY(af);
	       return place.place(l,w,h -af); 
		}
	   ,new View[]{helpscroll},new View[]{ok});
	     whelplayout=new WeakReference<Layout>(helplayout);

	      helplayout.setBackgroundResource(R.drawable.helpbackground);
        params.setMargins(
            MainActivity.systembarLeft,
            0,
            MainActivity.systembarRight,
           MainActivity.systembarBottom
		);
            
         helplayout.setLayoutParams(params);
        helplayout.requestLayout();
           act.addContentView(helplayout, params);
       }
       else {
           helplayout.setVisibility(VISIBLE);
           helplayout.bringToFront();
        ViewGroup.MarginLayoutParams marg = (ViewGroup.MarginLayoutParams) helplayout.getLayoutParams();
         marg.width=params.width; 
         marg.height=params.height; 
        marg.setMargins(
            MainActivity.systembarLeft,
            0,
            MainActivity.systembarRight,
           MainActivity.systembarBottom
		);
         helplayout.setLayoutParams(marg);
        helplayout.requestLayout();
       }
     //   ViewGroup.MarginLayoutParams marg = (ViewGroup.MarginLayoutParams) helplayout.getLayoutParams();
//       whelpview.get().setText(Html.fromHtml(text));
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
        whelpview.get().setText(fromHtml(text,TO_HTML_PARAGRAPH_LINES_CONSECUTIVE));
    }
    else {
		whelpview.get().setText(fromHtml(text));
	}

     Runnable closerun=() -> {
		 if (whelplayout != null) {
			 Layout helplayout2 = whelplayout.get();
			 if (helplayout2 != null) {
				 helplayout2.setVisibility(GONE);
				 okproc.accept(helplayout2);
			 	}
		 }

	 };
		setonback(closerun);
		ok.setOnClickListener(v->{
			Log.i(LOG_ID,"Ok pressed");
			doonback() ;
		});
}

	public static   void help(String text,Activity act,Consumer<Layout>  okproc) {
	 help( text, act, okproc,(v,w,h)-> {
	 	return new int[] {w,h};
		}, new ViewGroup.MarginLayoutParams(MATCH_PARENT, WRAP_CONTENT));
	}
public static   void help(String text,Activity act) {
	help(text,act,l->{});
	}
public static void hidekeyboard(Activity activity) {
      if(activity==null)
            return;
        InputMethodManager imm = (InputMethodManager) activity.getSystemService(INPUT_METHOD_SERVICE);
           View focus= activity.getCurrentFocus();
	   if(focus==null)
			focus=activity.findViewById(android.R.id.content);
           if(focus!=null)
		    imm.hideSoftInputFromWindow(focus.getWindowToken(), 0);
        }
public static void showkeyboard(Activity activity,View focus) {
        InputMethodManager imm = (InputMethodManager) activity.getSystemService(INPUT_METHOD_SERVICE);
			imm.showSoftInput(focus, 0);
        }

public static void sethelpbutton(int vis) {
	if(okbutton!=null) {
		Button ok = okbutton.get();
		if (ok != null) {
            Applic.RunOnUiThread(()-> ok.setVisibility(vis));
		}
	}
	}
   }




