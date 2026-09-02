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
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
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

import android.text.Editable;
import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.TextWatcher;
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
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.regex.PatternSyntaxException;


public class help {
static private final String LOG_ID="help";
//tatic   Layout helplayout=null;
static WeakReference<ViewGroup> whelplayout=null;
static    WeakReference<TextView> whelpview=null;
static    WeakReference<HelpSearch> whelpsearch=null;
public static void reset() {
     whelplayout=null;
     whelpview=null;
     whelpsearch=null;
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

    textView.setText(cleaned, TextView.BufferType.SPANNABLE);
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

private static final class HelpSearch {
    private static final int HIT_COLOR = 0xFFFFEB3B;
    private static final int CURRENT_HIT_COLOR = 0xFFFF9800;

    final ContextThemeWrapper act;
    final ScrollView scroll;
    final TextView text;
    final Button searchButton;
    final View normalButtons;
    final LinearLayout bar;
    final EditText query;
    final TextView counter;
    final TextView previous;
    final TextView next;
    final TextView close;

    final ArrayList<int[]> hits = new ArrayList<>();
    final ArrayList<Object> searchSpans = new ArrayList<>();
    int selected = -1;
    boolean resetting = false;

    HelpSearch(ContextThemeWrapper act, ScrollView scroll, TextView text, Button searchButton, View normalButtons) {
        this.act = act;
        this.scroll = scroll;
        this.text = text;
        this.searchButton = searchButton;
        this.normalButtons = normalButtons;

        searchButton.setText(R.string.search);

        bar = new LinearLayout(act);
        bar.setOrientation(LinearLayout.HORIZONTAL);
        bar.setGravity(Gravity.CENTER_VERTICAL);

        query = new EditText(act);
        query.setSingleLine(true);
        query.setHint(R.string.search);
        query.setImeOptions(EditorInfo.IME_ACTION_SEARCH
                | EditorInfo.IME_FLAG_NO_EXTRACT_UI
                | EditorInfo.IME_FLAG_NO_FULLSCREEN);

        counter = new TextView(act);
        counter.setGravity(Gravity.CENTER);
        counter.setText("0/0");
        int smallPad = (int)(GlucoseCurve.getDensity() * 3.0f);
        counter.setPadding(smallPad, 0, smallPad, 0);

        previous = searchIcon("‹");
        next = searchIcon("›");
        close = searchIcon("×");

        bar.addView(query, new LinearLayout.LayoutParams(0, WRAP_CONTENT, 1.0f));
        bar.addView(counter, new LinearLayout.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));
        bar.addView(previous, iconParams());
        bar.addView(next, iconParams());
        bar.addView(close, iconParams());
        bar.setVisibility(GONE);

        searchButton.setOnClickListener(v -> open());
        close.setOnClickListener(v -> close());
        previous.setOnClickListener(v -> {
            hideSearchKeyboard();
            selectRelative(-1);
        });
        next.setOnClickListener(v -> {
            hideSearchKeyboard();
            selectRelative(1);
        });

        query.addTextChangedListener(new TextWatcher() {
            @Override public void beforeTextChanged(CharSequence s, int start, int count, int after) {}
            @Override public void onTextChanged(CharSequence s, int start, int before, int count) {}
            @Override public void afterTextChanged(Editable s) {
                if (!resetting)
                    find(s.toString());
            }
        });

        query.setOnEditorActionListener((v, actionId, event) -> {
            if (actionId == EditorInfo.IME_ACTION_SEARCH) {
                hideSearchKeyboard();
                scrollToSelected();
                return true;
            }
            return false;
        });
    }

    private TextView searchIcon(String symbol) {
        TextView icon = new TextView(act);
        icon.setText(symbol);
        icon.setGravity(Gravity.CENTER);
        icon.setTextSize(24.0f);
        icon.setTextColor(util.getColorFromTheme(act, android.R.attr.textColorPrimary));
        icon.setPadding(0, 0, 0, 0);
        icon.setMinWidth(0);
        icon.setMinimumWidth(0);
        icon.setMinHeight(0);
        icon.setMinimumHeight(0);
        icon.setClickable(true);
        icon.setFocusable(true);
        return icon;
    }

    private LinearLayout.LayoutParams iconParams() {
        // Keep the glyph itself small, but make each touch/spacing area 1.8x wider.
        int width = (int)(GlucoseCurve.getDensity() * (28.0f * 1.8f));
        return new LinearLayout.LayoutParams(width, MATCH_PARENT);
    }

    private void hideSearchKeyboard() {
        query.clearFocus();
        InputMethodManager imm = (InputMethodManager)act.getSystemService(INPUT_METHOD_SERVICE);
        if (imm != null)
            imm.hideSoftInputFromWindow(query.getWindowToken(), 0);
    }

    void open() {
        int normalHeight = normalButtons.getHeight();
        if (normalHeight > 0)
            bar.setMinimumHeight(normalHeight);
        normalButtons.setVisibility(GONE);
        bar.setVisibility(VISIBLE);
        query.requestFocus();
        query.post(() -> {
            query.setSelection(query.length());
            showkeyboard(act, query);
        });
    }

    void close() {
        hideSearchKeyboard();
        clearQueryAndHighlights();
        bar.setVisibility(GONE);
        normalButtons.setVisibility(VISIBLE);
    }

    void resetForNewText() {
        hideSearchKeyboard();
        clearQueryAndHighlights();
        bar.setVisibility(GONE);
        normalButtons.setVisibility(VISIBLE);
    }

    private void clearQueryAndHighlights() {
        resetting = true;
        query.setText("");
        resetting = false;
        query.setError(null);
        hits.clear();
        selected = -1;
        clearSearchSpans();
        updateCounter();
    }

    private void find(String expression) {
        clearSearchSpans();
        hits.clear();
        selected = -1;
        query.setError(null);

        if (expression.length() == 0) {
            updateCounter();
            return;
        }

        final Pattern pattern;
        try {
            pattern = Pattern.compile(expression,
                    Pattern.CASE_INSENSITIVE | Pattern.UNICODE_CASE | Pattern.MULTILINE);
        }
        catch (PatternSyntaxException error) {
            query.setError("Invalid regular expression");
            updateCounter();
            return;
        }

        Matcher matcher = pattern.matcher(text.getText().toString());
        while (matcher.find()) {
            // A zero-length regular-expression match cannot be visibly highlighted.
            if (matcher.start() != matcher.end())
                hits.add(new int[]{matcher.start(), matcher.end()});
        }

        if (!hits.isEmpty())
            selected = 0;

        applyHighlights();
        updateCounter();
        scrollToSelected();
    }

    private void selectRelative(int difference) {
        if (hits.isEmpty())
            return;
        selected = (selected + difference) % hits.size();
        if (selected < 0)
            selected += hits.size();
        applyHighlights();
        updateCounter();
        scrollToSelected();
    }

    private Spannable spannableText() {
        CharSequence current = text.getText();
        if (current instanceof Spannable)
            return (Spannable)current;

        SpannableStringBuilder copy = new SpannableStringBuilder(current);
        text.setText(copy, TextView.BufferType.SPANNABLE);
        return (Spannable)text.getText();
    }

    private void clearSearchSpans() {
        Spannable content = spannableText();
        for (Object span : searchSpans)
            content.removeSpan(span);
        searchSpans.clear();
        text.invalidate();
    }

    private void applyHighlights() {
        clearSearchSpans();
        if (hits.isEmpty())
            return;

        Spannable content = spannableText();
        for (int i = 0; i < hits.size(); ++i) {
            int[] hit = hits.get(i);
            BackgroundColorSpan background = new BackgroundColorSpan(
                    i == selected ? CURRENT_HIT_COLOR : HIT_COLOR);
            ForegroundColorSpan foreground = new ForegroundColorSpan(Color.BLACK);
            content.setSpan(background, hit[0], hit[1], Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
            content.setSpan(foreground, hit[0], hit[1], Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
            searchSpans.add(background);
            searchSpans.add(foreground);
        }
        text.invalidate();
    }

    private void updateCounter() {
        counter.setText(selected < 0 ? "0/" + hits.size() : (selected + 1) + "/" + hits.size());
        previous.setEnabled(!hits.isEmpty());
        next.setEnabled(!hits.isEmpty());
    }

    private void scrollToSelected() {
        if (selected < 0 || selected >= hits.size())
            return;

        final int offset = hits.get(selected)[0];
        text.post(() -> {
            android.text.Layout layout = text.getLayout();
            if (layout == null)
                return;

            int line = layout.getLineForOffset(Math.min(offset, text.length()));
            int top = text.getTotalPaddingTop() + layout.getLineTop(line);
            int bottom = text.getTotalPaddingTop() + layout.getLineBottom(line);
            int visibleTop = scroll.getScrollY();
            int visibleBottom = visibleTop + scroll.getHeight();
            int extra = (int)(GlucoseCurve.getDensity() * 8.0f);

            if (top < visibleTop)
                scroll.smoothScrollTo(0, Math.max(0, top - extra));
            else if (bottom > visibleBottom)
                scroll.smoothScrollTo(0,
                        Math.max(0, bottom - scroll.getHeight() + extra));
        });
    }
}

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
      HelpSearch helpSearch = null;
      if(whelplayout==null||((helplayout=whelplayout.get())==null)||act!=helplayout.getContext()||( (ok=okbutton.get())==null) ) {
         ScrollView helpscroll=new ScrollView(act);
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
         final int pad=(int)(GlucoseCurve.getDensity()*7.0);
         helpview.setPadding(pad,pad,pad,pad+systembarBottom);

         // The outer FrameLayout supplies helpbackground.  Do not also put it on
         // the nested ScrollView: a drawable edge/stroke here becomes a visible
         // separator immediately above the help text (and below the search bar).
         helpscroll.setBackgroundColor(Color.TRANSPARENT);
         helpscroll.addView(helpview, new ViewGroup.LayoutParams(MATCH_PARENT, WRAP_CONTENT));

         Button searchButton = new Button(act);

         LinearLayout buttons = new LinearLayout(act);
         buttons.setOrientation(LinearLayout.HORIZONTAL);
         buttons.setGravity(MainActivity.rtl ? Gravity.LEFT : Gravity.RIGHT);
         buttons.addView(searchButton, new LinearLayout.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));
         buttons.addView(ok, new LinearLayout.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));

         helpSearch = new HelpSearch(act, helpscroll, helpview, searchButton, buttons);
         whelpsearch = new WeakReference<HelpSearch>(helpSearch);

         // Normal mode: Search/OK float over the help text, as OK did originally.
         // Search mode: the search bar becomes a real row above the ScrollView so
         // the help text starts below it while the search controls are visible.
         LinearLayout content = new LinearLayout(act);
         content.setOrientation(LinearLayout.VERTICAL);
         content.addView(helpSearch.bar, new LinearLayout.LayoutParams(MATCH_PARENT, WRAP_CONTENT));
         content.addView(helpscroll, new LinearLayout.LayoutParams(MATCH_PARENT, 0, 1.0f));

         FrameLayout root = new FrameLayout(act);
         root.setBackgroundResource(R.drawable.helpbackground);
         root.addView(content, new FrameLayout.LayoutParams(MATCH_PARENT, MATCH_PARENT));
         root.addView(buttons, new FrameLayout.LayoutParams(MATCH_PARENT, WRAP_CONTENT, Gravity.TOP));

         helplayout=root;
         helplayout.setLayoutParams(params);
         helplayout.requestLayout();
         var activity=getActivity(act);
         activity.addContentView(helplayout, params);
         DynamicThemeUtils.applyTheme(ok);
         DynamicThemeUtils.applyTheme(searchButton);

         final FrameLayout rootForInsets=root;
         final View buttonsForInsets=buttons;
         final View searchBarForInsets=helpSearch.bar;
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

             setHelpInsets(rootForInsets, helpview, buttonsForInsets, searchBarForInsets, pad, left, top, right, bottom);
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
               if(whelpsearch!=null)
                   helpSearch=whelpsearch.get();
                }
       }

     if(isWearable)
             ok.setVisibility(useclose?View.VISIBLE:View.INVISIBLE);
    TextView textview=whelpview.get();
    if(MainActivity.rtl) {
         textview.setGravity(Gravity.RIGHT);
         textview.setTextDirection(View.TEXT_DIRECTION_RTL);
         }
    else {
         textview.setGravity(Gravity.LEFT);
         textview.setTextDirection(View.TEXT_DIRECTION_LTR);
         }
    setHtmlIgnoringHtmlColors(textview,text);
    if(!isWearable && helpSearch!=null)
        helpSearch.resetForNewText();
    final HelpSearch finalHelpSearch=helpSearch;
     Runnable closerun=() -> {
         if(finalHelpSearch!=null)
             finalHelpSearch.close();
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

private static void setHelpInsets(ViewGroup helplayout, TextView helpview,
                                  View normalButtons, View searchBar,
                                  int pad, int left, int top, int right, int bottom) {
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

    // Keep the ScrollView viewport edge-to-edge.  The first help line starts
    // below the status bar, but when the user scrolls, text can move through
    // the transparent status-bar area just as it did before search was added.
    if (helplayout.getPaddingTop() != 0)
        helplayout.setPadding(0, 0, 0, 0);

    helpview.setPadding(pad, pad + top, pad, pad + bottom);

    // The floating Search/OK controls themselves should remain below the
    // status bar even though the help ScrollView extends underneath it.
    ViewGroup.LayoutParams np = normalButtons.getLayoutParams();
    if (np instanceof ViewGroup.MarginLayoutParams marg && marg.topMargin != top) {
        marg.topMargin = top;
        normalButtons.setLayoutParams(marg);
    }

    // In search mode the search toolbar is a real row.  Give that row the
    // status-bar inset; its GONE state consumes no space in normal mode.
    ViewGroup.LayoutParams sp = searchBar.getLayoutParams();
    if (sp instanceof ViewGroup.MarginLayoutParams marg && marg.topMargin != top) {
        marg.topMargin = top;
        searchBar.setLayoutParams(marg);
    }
    }

}




