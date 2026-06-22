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


import android.content.Context;
import android.graphics.Color;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.LayerDrawable;
import android.graphics.drawable.StateListDrawable;
import android.util.TypedValue;
import androidx.appcompat.widget.AppCompatRadioButton;
import androidx.core.content.ContextCompat;

import android.content.res.ColorStateList;
import android.util.TypedValue;
import android.view.View;
import android.widget.RadioButton;
import android.content.Context;
import android.graphics.Color;
import android.os.Build;
import android.text.method.LinkMovementMethod;
import android.widget.Button;

import android.widget.TextView;

import java.text.DateFormat;
import java.util.Date;
import java.util.Locale;

import static android.text.Html.TO_HTML_PARAGRAPH_LINES_CONSECUTIVE;
import static android.text.Html.fromHtml;

import static tk.glucodata.Log.doLog;

import androidx.appcompat.app.AppCompatDelegate;
import androidx.core.content.ContextCompat;

public class util {
private	static DateFormat dformat;
static void mkdataFormat() {
    dformat = DateFormat.getDateTimeInstance(DateFormat.DEFAULT,DateFormat.DEFAULT);
    };
static {
    mkdataFormat(); 
    };
static public String timestring(long tim) {
		return dformat.format(new Date(tim));
		}
public static CheckDirectionBox getcheckbox(Context context, String label, boolean val) {
	var check=new CheckDirectionBox(context);
	check.setText(label);
	check.setChecked(val);
	return check;
	}
	public static CheckDirectionBox getcheckbox(Context context,int res, boolean val) {
		return getcheckbox(context, context.getString(res),val);
	}
public static TextView getlabel(Context act, String text) {
	TextView label=new TextView(act);
	label.setText(text);
	return label;
	}
public static TextView getlabel(Context act, int res) {
	return getlabel(act,act.getString(res));

	}
public static Button getbutton(Context act, String text) {
	Button label=new Button(act);
	label.setText(text);
	return label;
	}


public static Button getbutton(Context act, int res) {
	return getbutton(act,act.getString(res));

	}
	@SuppressWarnings("deprecation")
static void sethtml(TextView view, String text) {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
        view.setText(fromHtml(text,TO_HTML_PARAGRAPH_LINES_CONSECUTIVE));
    }
    else view.setText(fromHtml(text));
    view.setTextIsSelectable(true);
    view.setTextColor(util.getColorFromTheme(view.getContext(), android.R.attr.textColorPrimary));



	view.setLinksClickable(true);
	view.setMovementMethod(LinkMovementMethod.getInstance());
    }

static void sethtml(TextView view,int res) {
	sethtml(view,view.getContext().getString(res));
	}
static  Locale getReslocale() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N){
            return Applic.app.getResources().getConfiguration().getLocales().get(0);
        } else{
            //noinspection deprecation
            return Applic.app.getResources().getConfiguration().locale;
        }
    }
static public Locale getlocale() {
	final var locales= AppCompatDelegate.getApplicationLocales();
	if(locales.isEmpty())  {
//		final var lang=Locale.getDefault();
		final var lang=getReslocale();
		{if(doLog) {Log.i("getlocale","Locale.getDefault()="+lang);};};
		return lang;
		}
	final var lang2=locales.get(0);
	{if(doLog) {Log.i("getlocale" ,"getApplicationLocales()="+lang2);};};
	return lang2;

	}


    // Helper method to safely extract colors from the theme
    public static int getColorFromTheme(Context context, int attributeResId) {
        TypedValue typedValue = new TypedValue();
        if (context.getTheme().resolveAttribute(attributeResId, typedValue, true)) {
            if (typedValue.resourceId != 0) {
                // It's a reference (e.g., @color/my_color)
                return ContextCompat.getColor(context, typedValue.resourceId);
            } else {
                // It's a direct hex code (e.g., #1976D2)
                return typedValue.data;
            }
        }
        // Safe fallback if the attribute is missing entirely
        return Color.BLACK; 
    }

public static int pxToDp(Context context,int px)  {
    float dp = px / context.getResources().getDisplayMetrics().density;
    return (int) dp;
    }
/*
    // 2. Helper to convert DP to Pixels for drawing shapes
    private static int dpToPx(Context context, int dp) {
        return (int) (dp * context.getResources().getDisplayMetrics().density);
    }

    // 3. THE FIX: Create a custom graphic that is immune to OS rendering bugs
    private static Drawable createRadioGraphic(Context context, int checkedColor, int uncheckedColor) {
        int strokeWidth = dpToPx(context, 2);
        int size = dpToPx(context, 20); // Standard radio button size
        int innerPadding = dpToPx(context, 5); // Space between the outer ring and the inner dot

        // Draw the Unchecked State (Hollow Ring)
        GradientDrawable uncheckedRing = new GradientDrawable();
        uncheckedRing.setShape(GradientDrawable.OVAL);
        uncheckedRing.setSize(size, size);
        uncheckedRing.setColor(Color.TRANSPARENT);
        uncheckedRing.setStroke(strokeWidth, uncheckedColor);

        // Draw the Checked State (Outer Ring + Solid Inner Dot)
        GradientDrawable checkedRing = new GradientDrawable();
        checkedRing.setShape(GradientDrawable.OVAL);
        checkedRing.setSize(size, size);
        checkedRing.setColor(Color.TRANSPARENT);
        checkedRing.setStroke(strokeWidth, checkedColor);

        GradientDrawable checkedDot = new GradientDrawable();
        checkedDot.setShape(GradientDrawable.OVAL);
        checkedDot.setSize(size, size);
        checkedDot.setColor(checkedColor);

        // Stack the dot inside the ring using a LayerDrawable
        LayerDrawable checkedState = new LayerDrawable(new Drawable[]{checkedRing, checkedDot});
        checkedState.setLayerInset(1, innerPadding, innerPadding, innerPadding, innerPadding);

        // Combine them into a StateList
        StateListDrawable stateList = new StateListDrawable();
        stateList.addState(new int[]{android.R.attr.state_checked}, checkedState);
        stateList.addState(new int[]{-android.R.attr.state_checked}, uncheckedRing);

        return stateList;
    }

    // 4. Your updated creation method
    public static CheckDirectionRadio getradiobutton(Context context, int res) {
         var radio=new CheckDirectionRadio(context);
       // var radio = new AppCompatRadioButton(context);
        
        // Fetch Colors
        int textColor = getColorFromTheme(context, android.R.attr.textColorPrimary);
        int checkedColor = getColorFromTheme(context, androidx.appcompat.R.attr.colorControlActivated);
        int uncheckedColor = getColorFromTheme(context, androidx.appcompat.R.attr.colorControlNormal);
        
        // Set Text and Text Color
        radio.setTextColor(textColor);
        radio.setText(res);

        // OVERRIDE THE OS BUG: Replace the default button graphic with our custom drawn graphic
        radio.setButtonDrawable(createRadioGraphic(context, checkedColor, uncheckedColor));
        
        // Add a little padding between your new graphic and the text
        radio.setPadding(dpToPx(context, 8), 0, 0, 0);
        
        return radio;
    }
*/
public static  CheckDirectionRadio getradiobutton(Context context, int res) {
         var radio=new CheckDirectionRadio(context);
/*
        int textColor = getColorFromTheme(context, android.R.attr.textColorPrimary);
        radio.setTextColor(textColor);
        
        int checkedColor = getColorFromTheme(context, androidx.appcompat.R.attr.colorControlActivated);
        int uncheckedColor = getColorFromTheme(context, androidx.appcompat.R.attr.colorControlNormal);
        
        // 3. Apply the custom color state list
        radio.setButtonTintList(new ColorStateList(
            new int[][] {
                new int[] { android.R.attr.state_checked },
                new int[] { -android.R.attr.state_checked }
            },
            new int[] { checkedColor, uncheckedColor }
        ));
        */
     //  radio.setLayerType(View.LAYER_TYPE_SOFTWARE, null);
        radio.setText(res);
        return radio;
         }

public static CheckDirectionRadio getradiobuttonId(Context context, int res, int id) {
        var radio=getradiobutton( context,  res);
        radio.setId(id);
        return radio;
        }
	/*
static public String getlanguage(Context context) {
	return context.getString(R.string.language);
	} */
        
static void sleep(long msec) {
   try {
       Thread.sleep(msec);
   } catch (InterruptedException ie) {
       Thread.currentThread().interrupt();
   }
}
}
