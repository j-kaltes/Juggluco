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
import android.view.ContextThemeWrapper;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.FrameLayout;
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
import static tk.glucodata.Applic.backgroundcolor;
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.GlucoseCurve.getheight;
import static tk.glucodata.Layout.getMargins;
import static tk.glucodata.Log.doLog;
import static tk.glucodata.MainActivity.addMyContentView;
import static tk.glucodata.MainActivity.doonback;
import static tk.glucodata.MainActivity.poponback;
import static tk.glucodata.MainActivity.setonback;
import static tk.glucodata.MainActivity.systembarBottom;
import static tk.glucodata.MainActivity.systembarLeft;
import static tk.glucodata.MainActivity.systembarRight;
import static tk.glucodata.MainActivity.systembarTop;
import static tk.glucodata.Specific.useclose;
import static tk.glucodata.settings.Settings.removeContentView;

import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.style.BackgroundColorSpan;
import android.text.style.ForegroundColorSpan;
import android.widget.TextView;
import android.view.WindowInsets;
import android.text.Html;
//import android.text.Layout;
import android.text.Spanned;
import android.text.SpannableStringBuilder;
import android.text.style.BackgroundColorSpan;
import android.text.style.ForegroundColorSpan;
import android.text.style.TabStopSpan;
import android.text.style.TypefaceSpan;

import java.util.ArrayList;


public class help {
static private final String LOG_ID="help";
//tatic   Layout helplayout=null;
static WeakReference<ViewGroup> whelplayout=null;
static    WeakReference<TextView> whelpview=null;
public static void reset() {
     whelplayout=null;
     whelpview=null;
     okbutton=null;
    }
public static   void help(int res, ContextThemeWrapper act,Consumer<ViewGroup> okproc) {
    help(act.getString(res),act,okproc);
    }
public static   void help(int res,ContextThemeWrapper act) {
    help(res,act,l->{});
    }
public static   void helplight(int res,MainActivity act) {
    act.themeLightBars();
    help(res,act, l->act.lightBars(!Natives.getInvertColors()));
    }
public static    WeakReference<Button> okbutton=null;
public static void hide() {
    if(whelplayout==null) 
        return;
    ViewGroup lay=whelplayout.get();
    if(lay==null)
        return;
    lay.setVisibility(GONE);
    }
public static void show() {
    if(whelplayout==null) 
        return;
    ViewGroup lay=whelplayout.get();
    if(lay==null)
        return;
    lay.setVisibility(VISIBLE);
    }
public static   void basehelp(int res,ContextThemeWrapper act,Consumer<ViewGroup> okproc) {
    basehelp(act.getString(res),act,okproc);
    }

    public static   void  basehelp(String text,ContextThemeWrapper act,Consumer<ViewGroup>  okproc) {
          basehelp(text,act,okproc,(v,w,h)-> new int[] {w,h},new ViewGroup.MarginLayoutParams(MATCH_PARENT, MATCH_PARENT)) ;
        }

public static void setHtmlIgnoringHtmlColors(TextView textView, String html) {
    Spanned spanned =
            Build.VERSION.SDK_INT >= Build.VERSION_CODES.N
            ? fromHtml(html, TO_HTML_PARAGRAPH_LINES_CONSECUTIVE)
            : fromHtml(html);

    SpannableStringBuilder cleaned = new SpannableStringBuilder(spanned);

    ForegroundColorSpan[] fgSpans =
            cleaned.getSpans(0, cleaned.length(), ForegroundColorSpan.class);
    for (ForegroundColorSpan span : fgSpans) {
        cleaned.removeSpan(span);
    }

    BackgroundColorSpan[] bgSpans =
            cleaned.getSpans(0, cleaned.length(), BackgroundColorSpan.class);
    for (BackgroundColorSpan span : bgSpans) {
        cleaned.removeSpan(span);
    }

    textView.setText(cleaned);
}
/*
public static void setHtmlIgnoringHtmlColors(TextView textView, String html) {
    Spanned spanned =
            Build.VERSION.SDK_INT >= Build.VERSION_CODES.N
            ? Html.fromHtml(html, Html.FROM_HTML_MODE_COMPACT)
            : Html.fromHtml(html);

    SpannableStringBuilder cleaned = new SpannableStringBuilder(spanned);

    ForegroundColorSpan[] fgSpans =
            cleaned.getSpans(0, cleaned.length(), ForegroundColorSpan.class);
    for (ForegroundColorSpan span : fgSpans) {
        cleaned.removeSpan(span);
    }

    BackgroundColorSpan[] bgSpans =
            cleaned.getSpans(0, cleaned.length(), BackgroundColorSpan.class);
    for (BackgroundColorSpan span : bgSpans) {
        cleaned.removeSpan(span);
    }

    alignMenuTabs(textView, cleaned);

    textView.setText(cleaned);
}


private static void alignMenuTabs(TextView textView,
                                  SpannableStringBuilder text) {
    float widest = 0.0f;
    boolean found = false;

    int lineStart = 0;

    for (int i = 0; i <= text.length(); ++i) {
        if (i == text.length() || text.charAt(i) == '\n') {
            int tab = -1;

            for (int j = lineStart; j < i; ++j) {
                if (text.charAt(j) == '\t') {
                    tab = j;
                    break;
                }
            }

            if (tab >= 0) {
                float width = android.text.Layout.getDesiredWidth(
                        text,
                        lineStart,
                        tab,
                        textView.getPaint());

                if (width > widest)
                    widest = width;

                found = true;
            }

            lineStart = i + 1;
        }
    }

    if (!found)
        return;

    // Distance between longest label and [x]/[ ].
    int gap = Math.round(textView.getTextSize() * 0.6f);

    int checkX = Math.round(widest) + gap;

    text.setSpan(
            new TabStopSpan.Standard(checkX),
            0,
            text.length(),
            Spanned.SPAN_PARAGRAPH);
}

private static void alignMenuChecks(TextView textView,
                                    SpannableStringBuilder text) {

    ArrayList<int[]> replacements = new ArrayList<>();

    String str = text.toString();
    int pos = 0;

    while (pos < str.length()) {
        int newline = str.indexOf('\n', pos);
        int end = newline < 0 ? str.length() : newline;

        int realEnd = end;

        // Ignore whitespace at end of line.
        while (realEnd > pos && isMenuSpace(str.charAt(realEnd - 1))) {
            --realEnd;
        }

        if (realEnd - pos >= 3) {
            int check = realEnd - 3;

            boolean isCheck =
                    str.charAt(check) == '[' &&
                    (str.charAt(check + 1) == 'x' ||
                     str.charAt(check + 1) == 'X' ||
                     str.charAt(check + 1) == ' ') &&
                    str.charAt(check + 2) == ']';

            if (isCheck) {
                int begin = check;

                while (begin > pos &&
                       isMenuSpace(str.charAt(begin - 1))) {
                    --begin;
                }

                replacements.add(new int[]{begin, check});
            }
        }

        if (newline < 0)
            break;

        pos = newline + 1;
    }

    for (int i = replacements.size() - 1; i >= 0; --i) {
        int[] r = replacements.get(i);

        // If there were no spaces this inserts a TAB.
        text.replace(r[0], r[1], "\t");
    }


    str = text.toString();

    ArrayList<int[]> checkLines = new ArrayList<>();
    float widest = 0.0f;

    pos = 0;

    while (pos < str.length()) {
        int newline = str.indexOf('\n', pos);
        int end = newline < 0 ? str.length() : newline;

        int tab = str.lastIndexOf('\t', end - 1);

        if (tab >= pos && tab + 3 < str.length()) {
            int check = tab + 1;

            if (check + 2 < end &&
                str.charAt(check) == '[' &&
                (str.charAt(check + 1) == 'x' ||
                 str.charAt(check + 1) == 'X' ||
                 str.charAt(check + 1) == ' ') &&
                str.charAt(check + 2) == ']') {

                float width = android.text.Layout.getDesiredWidth(
                        text, pos, tab, textView.getPaint());

                if (width > widest)
                    widest = width;

                checkLines.add(new int[]{
                        pos,
                        newline < 0 ? end : end + 1,
                        check
                });
            }
        }

        if (newline < 0)
            break;

        pos = newline + 1;
    }

    if (checkLines.isEmpty())
        return;


    int gap = Math.round(textView.getTextSize() * 0.6f);
    int tabPosition = Math.round(widest) + gap;


    for (int[] line : checkLines) {
        int begin = line[0];
        int end   = line[1];
        int check = line[2];

        text.setSpan(
                new TabStopSpan.Standard(tabPosition),
                begin,
                end,
                Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);

        text.setSpan(
                new TypefaceSpan("monospace"),
                check,
                check + 3,
                Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
    }
}


private static boolean isMenuSpace(char c) {
    return c == ' '       ||
           c == '\t'      ||
           c == '\u00A0'  ||  // &nbsp;
           c == '\u2007'  ||
           c == '\u202F';
}
*/
/*
public static void setHtmlIgnoringHtmlColors(TextView textView, String html) {
    Spanned spanned = Build.VERSION.SDK_INT >= Build.VERSION_CODES.N?fromHtml(html,TO_HTML_PARAGRAPH_LINES_CONSECUTIVE):fromHtml(html);
    SpannableStringBuilder cleaned = new SpannableStringBuilder(spanned);

    ForegroundColorSpan[] fgSpans =
            cleaned.getSpans(0, cleaned.length(), ForegroundColorSpan.class);
    for (ForegroundColorSpan span : fgSpans) {
        cleaned.removeSpan(span);
    }

    // Optional: remove inline background colors too.
    BackgroundColorSpan[] bgSpans =
            cleaned.getSpans(0, cleaned.length(), BackgroundColorSpan.class);
    for (BackgroundColorSpan span : bgSpans) {
        cleaned.removeSpan(span);
    }

    textView.setText(cleaned);
}
*/
    @SuppressWarnings("deprecation")
  public static   void  basehelp(String text,ContextThemeWrapper act,Consumer<ViewGroup>  okproc,Placer place, ViewGroup.MarginLayoutParams params) {
    hidekeyboard((MainActivity) getActivity(act));
    ScrollView       helpscroll=new ScrollView(act);
    TextView helpview=new TextView(act);
    setHtmlIgnoringHtmlColors(helpview,text);
         helpview.setTextColor(util.getColorFromTheme(act, android.R.attr.textColorPrimary));
     helpview.setTextIsSelectable(true);
     helpview.setScroller(null);
     helpview.setMovementMethod(LinkMovementMethod.getInstance());
         helpview.setLinksClickable(true);
       helpscroll.setVerticalScrollBarEnabled(Applic.scrollbar);
      helpscroll.setScrollbarFadingEnabled(true);
        helpscroll.setFillViewport(true);
       
       Button ok=new Button(act);
       ViewGroup helplayout;
       if(isWearable) {
              var width=MainActivity.screenwidth;
              int sidepad=(int)(width*.12);
              helpview.setPadding(sidepad,0,sidepad,(int)(MainActivity.screenheight*.20));
              if(useclose)
                   ok.setText(R.string.ok);
              else
                   ok.setVisibility(View.INVISIBLE);
              ViewGroup  layout=new Layout(act, place::place,new View[]{ok}, new View[]{helpview});

              params.setMargins( 0, (int)( GlucoseCurve.getDensity() *3.0), 0,0);
              layout.setBackgroundColor(backgroundcolor);
              helpscroll.addView(layout,params);
              helpscroll.setBackgroundColor(backgroundcolor);
              helplayout=helpscroll; 
              addMyContentView(getActivity(act),helplayout,new ViewGroup.LayoutParams(MATCH_PARENT,MATCH_PARENT));
            }
       else {
           ok.setText(R.string.ok);
         int pad=(int)(GlucoseCurve.getDensity()*7.0);
         helpview.setPadding(pad,pad,pad,pad);
           helpscroll.addView(helpview);
          
           Layout layouttmp=new Layout(act/*, (l,w,h)-> {
                var af=MainActivity.systembarTop*3/4; l.setY(af);
                return place.place(l,w,h -af); 
            }*/ ,new View[]{helpscroll},new View[]{ok});
            helplayout=layouttmp;

/*        params.setMargins(
            MainActivity.systembarLeft,
            0,
            MainActivity.systembarRight,

           MainActivity.systembarBottom*3/4
        ); */
       layouttmp.setLayoutParams(params);
          layouttmp.systembarMargins((left,top,right,bottom)->new int[]{left,top*3/4,right,bottom*3/4});
       layouttmp.requestLayout();
        layouttmp.setBackgroundResource(R.drawable.helpbackground);
           addMyContentView(getActivity(act),layouttmp, params);

          }
final var helplayout2=helplayout;
       
           ok.setOnClickListener(v->{
        poponback();
         okproc.accept(helplayout2);
        removeContentView(helplayout2);
           });


     Runnable closerun=() -> {
        okproc.accept(helplayout2);
        removeContentView(helplayout2);
        };

    setonback(closerun);
};

    @SuppressLint("deprecation")
public static   void help(String text,ContextThemeWrapper act,Consumer<ViewGroup>  okproc,Placer place, ViewGroup.MarginLayoutParams params) {
    if(doLog) {
          var len=text.length();
          if(doLog) {Log.i(LOG_ID,"help "+((len==0?"":text.substring(0,Math.min(20,len)))));};
          }
      hidekeyboard((MainActivity) getActivity(act));
      Button ok;
      ViewGroup helplayout;
      if(whelplayout==null||((helplayout=whelplayout.get())==null)||act!=helplayout.getContext()||( (ok=    okbutton.get())==null) ) {
         ScrollView       helpscroll=new ScrollView(act);
         TextView helpview=new TextView(act);
         helpview.setTextColor(util.getColorFromTheme(act, android.R.attr.textColorPrimary));
         helpview.setTextIsSelectable(true);
         whelpview=new WeakReference<TextView>(helpview);
         helpview.setMovementMethod(LinkMovementMethod.getInstance());

        helpscroll.setFillViewport(true);
         helpview.setLinksClickable(true);
         helpscroll.setVerticalScrollBarEnabled(Applic.scrollbar);
         helpscroll.setScrollbarFadingEnabled(false);
         ok=new Button(act);
        okbutton=new WeakReference<Button>(ok);
       ok.setText(R.string.ok);
       if(isWearable) {
              helpview.setPadding(0,0,0,(int)(MainActivity.screenheight*.20));
              ViewGroup  layout=new Layout(act, place::place,new View[]{ok}, new View[]{helpview});

              var width=MainActivity.screenwidth;
              int sidepad=(int)(width*.12);
              {if(doLog) {Log.i(LOG_ID,"width="+width+" sidepad="+sidepad);};};
              params.setMargins( sidepad, (int)( GlucoseCurve.getDensity()*3.0), sidepad,0);
              layout.setBackgroundColor(backgroundcolor);
              helpscroll.addView(layout,params);
              helpscroll.setBackgroundColor(backgroundcolor);
              helplayout=helpscroll; 
              addMyContentView(getActivity(act),helplayout,new ViewGroup.LayoutParams(MATCH_PARENT,MATCH_PARENT));
             helplayout.setBackgroundColor(backgroundcolor);
            }

    else  {
       //  getMargins(ok).topMargin=systembarTop;
         final int pad=(int)(GlucoseCurve.getDensity()*7.0);
         helpview.setPadding(pad,pad+systembarTop,pad,pad+systembarBottom);

        helpscroll.setBackgroundResource(R.drawable.helpbackground);

         helpscroll.addView(helpview, new ViewGroup.LayoutParams( MATCH_PARENT,MATCH_PARENT));
         helplayout=helpscroll;

//        params.setMargins( MainActivity.systembarLeft, 0,//    MainActivity.systembarTop*3/4, MainActivity.systembarRight, 0//  MainActivity.systembarBottom*3/4);
         helplayout.setLayoutParams(params);
        helplayout.requestLayout();
        var activity=getActivity(act);
       activity.addContentView(helplayout, params);

       var  okparams =    new FrameLayout.LayoutParams( WRAP_CONTENT, WRAP_CONTENT, MainActivity.rtl?Gravity.LEFT:Gravity.RIGHT| Gravity.TOP);
/*       okparams.topMargin=(int)(MainActivity.systembarTop*.71f);
       okparams.rightMargin=MainActivity.systembarRight;
       okparams.leftMargin=MainActivity.systembarLeft; */
           activity.addContentView(ok, okparams);
          DynamicThemeUtils.applyTheme(ok);
        var tmpok=ok;
    helplayout.setOnApplyWindowInsetsListener((v, insets) -> { int left, top, right, bottom;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            var bars = insets.getInsets(WindowInsets.Type.systemBars());
            left   = bars.left;
            top    = bars.top;
            right  = bars.right;
            bottom = bars.bottom;
        }
        else {
            left   = insets.getSystemWindowInsetLeft();
            top    = insets.getSystemWindowInsetTop();
            right  = insets.getSystemWindowInsetRight();
            bottom = insets.getSystemWindowInsetBottom();
        }

        setHelpInsets( helpscroll, helpview, tmpok, pad, left, top, right, bottom);
        return insets;
    });

helplayout.requestApplyInsets();

          }
        whelplayout=new WeakReference<ViewGroup>(helplayout);
       }
       else {
           helplayout.setVisibility(VISIBLE);
           helplayout.bringToFront();
           if(!isWearable) {
               ok.setVisibility(VISIBLE);
               ok.bringToFront();
               /*
               ViewGroup.MarginLayoutParams marg = (ViewGroup.MarginLayoutParams) helplayout.getLayoutParams();
                 marg.width=params.width; 
                 marg.height=params.height; 
                marg.setMargins( MainActivity.systembarLeft, 0, MainActivity.systembarRight, 0 );
                 helplayout.setLayoutParams(marg);
                helplayout.requestLayout();
                */
                }
       }

     if(isWearable)
             ok.setVisibility(useclose?View.VISIBLE:View.INVISIBLE);
     else {
         var width=GlucoseCurve.getwidth();
         if(width<=10)
              width=MainActivity.screenwidth;
        place.place(ok,width,getheight());
         }
    TextView textview=whelpview.get();
    if(MainActivity.rtl) {
         textview.setGravity(Gravity.RIGHT);
         textview.setTextDirection(View.TEXT_DIRECTION_RTL);
         }
    setHtmlIgnoringHtmlColors(textview,text);
     Runnable closerun=() -> {
         if (whelplayout != null) {
             ViewGroup helplayout2 = whelplayout.get();
             if (helplayout2 != null) {
                 helplayout2.setVisibility(GONE);
                 okproc.accept(helplayout2);
                 }
         }
         if(okbutton != null) {
             View but = okbutton.get();
             if(but != null) {
                 but.setVisibility(GONE);
                };
              }
             };
        setonback(closerun);
        ok.setOnClickListener(v->{
            {if(doLog) {Log.i(LOG_ID,"Ok pressed");};};
            doonback() ;
        });
}

    public static   void help(String text,ContextThemeWrapper act,Consumer<ViewGroup>  okproc) {
     help( text, act, okproc,(v,w,h)-> {
         return new int[] {w,h};
        }, new ViewGroup.MarginLayoutParams(MATCH_PARENT, MATCH_PARENT));
    }
public static   void help(String text,ContextThemeWrapper act) {
    help(text,act,l->{});
    }
public static void hidekeyboard(MainActivity activity) {
      if(activity==null)
            return;
        InputMethodManager imm = (InputMethodManager) activity.getSystemService(INPUT_METHOD_SERVICE);
           View focus= activity.getCurrentFocus();
       if(focus==null)
            focus=activity.findViewById(android.R.id.content);
           if(focus!=null)
            imm.hideSoftInputFromWindow(focus.getWindowToken(), 0);
        }
public static Activity getActivity(ContextThemeWrapper context) {
    do {
        if(context instanceof Activity)
            return (Activity) context;
        context=(ContextThemeWrapper)context.getBaseContext();
        } while(context!=null);
    return null;
    }
public static void showkeyboard(ContextThemeWrapper activity,View focus) {
        Log.i(LOG_ID,"showkeyboard");
        InputMethodManager imm = (InputMethodManager) activity.getSystemService(INPUT_METHOD_SERVICE);
            imm.showSoftInput(focus, 0);
        }

public static void sethelpbutton(int vis) {
if(useclose)
    if(okbutton!=null) {
        Button ok = okbutton.get();
        if (ok != null) {
            Applic.RunOnUiThread(()-> ok.setVisibility(vis));
        }
    }
    }

private static void setHelpInsets(ViewGroup helplayout, TextView helpview, Button ok, int pad, int left, int top, int right, int bottom) {
    ViewGroup.LayoutParams lp = helplayout.getLayoutParams();
    if (lp instanceof ViewGroup.MarginLayoutParams marg) {
        if (marg.leftMargin != left ||
                marg.topMargin != 0 ||
                marg.rightMargin != right ||
                marg.bottomMargin != 0) {
            marg.setMargins(left, 0, right, 0);
            helplayout.setLayoutParams(marg);
        }
    }

    helpview.setPadding(
            pad,
            pad + top,
            pad,
            pad + bottom);

    ViewGroup.LayoutParams op = ok.getLayoutParams();
    if (op instanceof ViewGroup.MarginLayoutParams marg) {
        if (marg.topMargin != top ||
                marg.leftMargin != left ||
                marg.rightMargin != right) {
            marg.topMargin = top;
            marg.leftMargin = left;
            marg.rightMargin = right;
            ok.setLayoutParams(marg);
        }
    }
    }

}




